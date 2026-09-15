<#
Convert the simulator's flattened PNG resource set to LVGL v8 binary images.

The target is the board configuration in Project/LVGL/lv_conf.h:
  LV_COLOR_DEPTH = 16, LV_COLOR_16_SWAP = 0, little-endian CPU.

Each output gets an LVGL 4-byte image header followed by RGB565 pixels. Images
with non-opaque source pixels use CF_TRUE_COLOR_ALPHA (RGB565 + alpha byte).
#>
[CmdletBinding()]
param(
    [string]$InputDirectory,
    [string]$OutputDirectory
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($InputDirectory)) {
    $InputDirectory = Join-Path $PSScriptRoot 'build\msvc-x64\ui'
}
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $PSScriptRoot 'ui_bin'
}

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Runtime.InteropServices

$cfTrueColor = [uint32]4
$cfTrueColorAlpha = [uint32]5

function Get-BgraPixels {
    param([System.Drawing.Bitmap]$Bitmap)

    $rectangle = [System.Drawing.Rectangle]::new(0, 0, $Bitmap.Width, $Bitmap.Height)
    $bits = $Bitmap.LockBits($rectangle, [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $stride = [Math]::Abs($bits.Stride)
        $bytes = [byte[]]::new($stride * $Bitmap.Height)
        [Runtime.InteropServices.Marshal]::Copy($bits.Scan0, $bytes, 0, $bytes.Length)
        return @{ Bytes = $bytes; Stride = $stride }
    }
    finally {
        $Bitmap.UnlockBits($bits)
    }
}

function Convert-LvglPng {
    param([System.IO.FileInfo]$SourceFile, [string]$Output)

    $source = [System.Drawing.Bitmap]::new($SourceFile.FullName)
    $bitmap = $null
    $graphics = $null
    try {
        $bitmap = [System.Drawing.Bitmap]::new($source.Width, $source.Height,
            [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        $graphics.DrawImageUnscaled($source, 0, 0)
        $pixels = Get-BgraPixels -Bitmap $bitmap
        $hasAlpha = $false

        for ($y = 0; $y -lt $bitmap.Height -and -not $hasAlpha; $y++) {
            $row = $y * $pixels.Stride
            for ($x = 0; $x -lt $bitmap.Width; $x++) {
                if ($pixels.Bytes[$row + $x * 4 + 3] -ne 255) {
                    $hasAlpha = $true
                    break
                }
            }
        }

        $cf = if ($hasAlpha) { $cfTrueColorAlpha } else { $cfTrueColor }
        $header = [uint32]$cf -bor ([uint32]$bitmap.Width -shl 10) -bor
            ([uint32]$bitmap.Height -shl 21)
        $stream = [System.IO.File]::Open($Output, [System.IO.FileMode]::Create,
            [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
        $writer = [System.IO.BinaryWriter]::new($stream)
        try {
            $writer.Write([uint32]$header)
            for ($y = 0; $y -lt $bitmap.Height; $y++) {
                $row = $y * $pixels.Stride
                for ($x = 0; $x -lt $bitmap.Width; $x++) {
                    $offset = $row + $x * 4
                    $blue = [uint16]$pixels.Bytes[$offset]
                    $green = [uint16]$pixels.Bytes[$offset + 1]
                    $red = [uint16]$pixels.Bytes[$offset + 2]
                    $rgb565 = [uint16]((($red -band 0xF8) -shl 8) -bor
                        (($green -band 0xFC) -shl 3) -bor ($blue -shr 3))
                    $writer.Write($rgb565)
                    if ($hasAlpha) {
                        $writer.Write($pixels.Bytes[$offset + 3])
                    }
                }
            }
        }
        finally {
            $writer.Dispose()
        }
        $mode = if ($hasAlpha) { 'CF_TRUE_COLOR_ALPHA' } else { 'CF_TRUE_COLOR' }
        [PSCustomObject]@{
            Name = $SourceFile.BaseName
            Width = $bitmap.Width
            Height = $bitmap.Height
            Format = $mode
            Bytes = (Get-Item -LiteralPath $Output).Length
        }
    }
    finally {
        if ($graphics) { $graphics.Dispose() }
        if ($bitmap) { $bitmap.Dispose() }
        $source.Dispose()
    }
}

if (-not (Test-Path -LiteralPath $InputDirectory -PathType Container)) {
    throw "PNG input directory not found: $InputDirectory"
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null

$pngs = @(Get-ChildItem -LiteralPath $InputDirectory -File -Filter '*.png' | Sort-Object Name)
$results = @(foreach ($png in $pngs) {
    $destination = Join-Path $OutputDirectory ($png.BaseName + '.bin')
    Convert-LvglPng -SourceFile $png -Output $destination
})

$results | Format-Table -AutoSize
$total = ($results | Measure-Object -Property Bytes -Sum).Sum
Write-Host ("Converted {0} files, {1:N2} MiB -> {2}" -f $results.Count, ($total / 1MB), $OutputDirectory)

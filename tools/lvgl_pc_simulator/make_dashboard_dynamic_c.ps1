<#
Generate opaque RGB565 C arrays for the four small dashboard state images.
They are linked into the firmware, so LVGL draws them without the U3 file
system. Semi-transparent PC strokes are strengthened before RGB565 output.
#>
[CmdletBinding()]
param(
    [string]$InputDirectory,
    [string]$OutputFile
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($InputDirectory)) {
    $InputDirectory = Join-Path $PSScriptRoot 'assets'
}
if ([string]::IsNullOrWhiteSpace($OutputFile)) {
    $OutputFile = Join-Path $PSScriptRoot '..\..\application\task_lvgl\src\dashboard_dynamic_assets.h'
}

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Runtime.InteropServices

$assets = @(
    @{ Source = '大灯\worker-out.png'; Symbol = 'lvgl_dashboard_headlight_off' },
    @{ Source = '大灯\bigdeng.png';    Symbol = 'lvgl_dashboard_headlight_on'  },
    @{ Source = '语音\voice0.png';     Symbol = 'lvgl_dashboard_voice_idle'    },
    @{ Source = '语音\voice1.png';     Symbol = 'lvgl_dashboard_voice_active'  }
)

$outputParent = Split-Path -Parent $OutputFile
New-Item -ItemType Directory -Path $outputParent -Force | Out-Null
$text = [System.Text.StringBuilder]::new()
[void]$text.AppendLine('#ifndef __DASHBOARD_DYNAMIC_ASSETS_H__')
[void]$text.AppendLine('#define __DASHBOARD_DYNAMIC_ASSETS_H__')
[void]$text.AppendLine()

foreach ($asset in $assets) {
    $path = Join-Path $InputDirectory $asset.Source
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Dashboard image not found: $path"
    }

    $source = [System.Drawing.Bitmap]::new($path)
    try {
        $upper = $asset.Symbol.ToUpperInvariant()
        [void]$text.AppendLine("#define $($upper)_W $($source.Width)u")
        [void]$text.AppendLine("#define $($upper)_H $($source.Height)u")
        [void]$text.AppendLine("static const unsigned short __attribute__((aligned(4))) $($asset.Symbol)_map[] = {")
        for ($y = 0; $y -lt $source.Height; $y++) {
            $line = [System.Text.StringBuilder]::new('    ')
            for ($x = 0; $x -lt $source.Width; $x++) {
                $pixel = $source.GetPixel($x, $y)
                # The PC assets are entirely semi-transparent.  Boost alpha
                # before flattening so RGB565 LCD gamma does not erase their
                # thin strokes against the dashboard's dark background.
                $alpha = [Math]::Min(255, ([int]$pixel.A * 5) / 2)
                $inverseAlpha = 255 - $alpha
                $red = [uint16]((([int]$pixel.R * $alpha) + (0 * $inverseAlpha)) / 255)
                $green = [uint16]((([int]$pixel.G * $alpha) + (8 * $inverseAlpha)) / 255)
                $blue = [uint16]((([int]$pixel.B * $alpha) + (17 * $inverseAlpha)) / 255)
                $rgb565 = [uint16]((($red -band 0xF8) -shl 8) -bor
                    (($green -band 0xFC) -shl 3) -bor ($blue -shr 3))
                [void]$line.AppendFormat('0x{0:X4}, ', $rgb565)
            }
            [void]$text.AppendLine($line.ToString())
        }
        [void]$text.AppendLine('};')
        [void]$text.AppendLine()
    }
    finally {
        $source.Dispose()
    }
}

[void]$text.AppendLine('#endif')
[IO.File]::WriteAllText($OutputFile, $text.ToString(),
    [Text.UTF8Encoding]::new($false))
Write-Host "Wrote $OutputFile"

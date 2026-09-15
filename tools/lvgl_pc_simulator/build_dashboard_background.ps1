<#
Flatten the static dashboard PNG layers into one opaque 1024x600 background.
The board reads this single RGB565 asset from U3, avoiding a SPI-NOR read and
alpha blend for every individual material whenever LVGL redraws a region.
The finished composition is cropped to the dashboard-only view and enlarged
back to 1024x600, removing the surrounding black frame.
#>
[CmdletBinding()]
param(
    [string]$InputDirectory,
    [string]$OutputFile,
    [string]$ShellFile
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($InputDirectory)) {
    $InputDirectory = Join-Path $PSScriptRoot 'build\msvc-x64\ui'
}
if ([string]::IsNullOrWhiteSpace($OutputFile)) {
    $OutputFile = Join-Path $PSScriptRoot 'ui_release\dashboard.png'
}
if ([string]::IsNullOrWhiteSpace($ShellFile)) {
    $ShellFile = Join-Path (Split-Path -Parent $OutputFile) 'shell.png'
}

Add-Type -AssemblyName System.Drawing
$OutputWidth = 1024
$OutputHeight = 600

function Save-StateTile {
    param(
        [System.Drawing.Bitmap]$Background,
        [string]$MaterialPath,
        [string]$TilePath,
        [int]$X,
        [int]$Y
    )

    $material = [System.Drawing.Image]::FromFile($MaterialPath)
    $tile = [System.Drawing.Bitmap]::new($material.Width, $material.Height,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $tileGraphics = [System.Drawing.Graphics]::FromImage($tile)
    try {
        $source = [System.Drawing.Rectangle]::new($X, $Y, $material.Width, $material.Height)
        $target = [System.Drawing.Rectangle]::new(0, 0, $material.Width, $material.Height)
        $tileGraphics.DrawImage($Background, $target, $source,
            [System.Drawing.GraphicsUnit]::Pixel)
        $tileGraphics.DrawImageUnscaled($material, 0, 0)
        $tile.Save($TilePath, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $tileGraphics.Dispose()
        $tile.Dispose()
        $material.Dispose()
    }
}

$layers = @(
    @('001.png', 0, 12), @('003.png', 46, 87), @('015.png', 46, 233),
    @('021.png', 734, 431), @('t40.png', 345, 278), @('t41.png', 346, 169),
    @('t42.png', 268, 170), @('t46.png', 418, 437), @('t47.png', 420, 446),
    @('t48.png', 305, 272), @('w51.png', 743, 139), @('052.png', 139, 139),
    @('m60.png', 361, 151), @('m61.png', 405, 310), @('m62.png', 404, 55),
    @('m64.png', 546, 158), @('067.png', 481, 200), @('070.png', 574, 99),
    @('075.png', 364, 98), @('b80.png', 611, 171), @('b81.png', 610, 217),
    @('b82.png', 677, 227), @('b84.png', 669, 211), @('b85.png', 601, 443),
    @('b86.png', 603, 446), @('b87.png', 683, 207), @('088.png', 714, 250),
    @('095.png', 436, 225), @('m65.png', 478, 395), @('m66.png', 526, 395),
    @('s99.png', 496, 325), @('100.png', 463, 94), @('105.png', 747, 192),
    @('i35.png', 375, 472), @('i33.png', 421, 472), @('i37.png', 466, 472), @('i36.png', 557, 472),
    @('i34.png', 603, 473), @('i38.png', 650, 472), @('t43.png', 271, 322),
    @('t45.png', 330, 274), @('w50.png', 729, 137), @('i32.png', 500, 473)
)

if (-not (Test-Path -LiteralPath $InputDirectory -PathType Container)) {
    throw "PNG input directory not found: $InputDirectory"
}
New-Item -ItemType Directory -Path (Split-Path -Parent $OutputFile) -Force | Out-Null

$canvas = [System.Drawing.Bitmap]::new(1024, 600,
    [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [System.Drawing.Graphics]::FromImage($canvas)
$cropped = $null
$cropGraphics = $null
$outputCanvas = $null
$outputGraphics = $null
$shellCanvas = $null
$shellGraphics = $null
$shellCropped = $null
$shellCropGraphics = $null
$shellOutputCanvas = $null
$shellOutputGraphics = $null
try {
    $graphics.Clear([System.Drawing.Color]::FromArgb(255, 0, 8, 17))
    foreach ($layer in $layers) {
        $path = Join-Path $InputDirectory $layer[0]
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Dashboard material not found: $path"
        }
        $image = [System.Drawing.Image]::FromFile($path)
        try {
            $graphics.DrawImageUnscaled($image, [int]$layer[1], [int]$layer[2])
        }
        finally {
            $image.Dispose()
        }
    }

    $batteryPath = Join-Path $InputDirectory 'b83.png'
    $battery = [System.Drawing.Image]::FromFile($batteryPath)
    try {
        $source = [System.Drawing.Rectangle]::new(0, 0, 20, 72)
        $target = [System.Drawing.Rectangle]::new(677, 289, 20, 72)
        $graphics.DrawImage($battery, $target, $source,
            [System.Drawing.GraphicsUnit]::Pixel)
    }
    finally {
        $battery.Dispose()
    }

    # Coordinates match the red frame approved for the full-screen dashboard.
    $cropRect = [System.Drawing.Rectangle]::new(24, 41, 970, 516)
    $cropped = [System.Drawing.Bitmap]::new($cropRect.Width, $cropRect.Height,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $cropGraphics = [System.Drawing.Graphics]::FromImage($cropped)
    $cropGraphics.DrawImage($canvas,
        [System.Drawing.Rectangle]::new(0, 0, $cropRect.Width, $cropRect.Height),
        $cropRect, [System.Drawing.GraphicsUnit]::Pixel)

    $outputCanvas = [System.Drawing.Bitmap]::new($OutputWidth, $OutputHeight,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $outputGraphics = [System.Drawing.Graphics]::FromImage($outputCanvas)
    $outputGraphics.Clear([System.Drawing.Color]::FromArgb(255, 0, 8, 17))
    $outputGraphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $outputGraphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $outputGraphics.DrawImage($cropped,
        [System.Drawing.Rectangle]::new(0, 0, $outputCanvas.Width, $outputCanvas.Height))
    $outputCanvas.Save($OutputFile, [System.Drawing.Imaging.ImageFormat]::Png)

    # shell.png is the non-dashboard page base.  It deliberately covers the
    # complete instrument content with the original 003/009_plane material,
    # then restores only the permanent status strip and bottom navigation.
    $shellCanvas = [System.Drawing.Bitmap]::new($canvas.Width, $canvas.Height,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $shellGraphics = [System.Drawing.Graphics]::FromImage($shellCanvas)
    $shellGraphics.DrawImageUnscaled($canvas, 0, 0)
    $plane = [System.Drawing.Image]::FromFile((Join-Path $InputDirectory '003.png'))
    try {
        $shellGraphics.DrawImageUnscaled($plane, 46, 87)
    }
    finally {
        $plane.Dispose()
    }
    # Keep the supplied top status presentation exact instead of recreating
    # it as labels.  This band contains date, user, WLAN and time only.
    $statusRect = [System.Drawing.Rectangle]::new(330, 80, 365, 55)
    $shellGraphics.DrawImage($canvas, $statusRect, $statusRect,
        [System.Drawing.GraphicsUnit]::Pixel)
    # The bottom strip remains fixed across all embedded pages.
    $bottomRect = [System.Drawing.Rectangle]::new(345, 455, 355, 55)
    $shellGraphics.DrawImage($canvas, $bottomRect, $bottomRect,
        [System.Drawing.GraphicsUnit]::Pixel)
    foreach ($nav in @(
            @('i35.png', 375, 472), @('i33.png', 421, 472),
            @('i37.png', 466, 472), @('i32.png', 500, 473),
            @('i36.png', 557, 472), @('i34.png', 603, 473),
            @('i38.png', 650, 472))) {
        $navImage = [System.Drawing.Image]::FromFile((Join-Path $InputDirectory $nav[0]))
        try {
            $shellGraphics.DrawImageUnscaled($navImage, [int]$nav[1], [int]$nav[2])
        }
        finally {
            $navImage.Dispose()
        }
    }
    $shellCropped = [System.Drawing.Bitmap]::new($cropRect.Width, $cropRect.Height,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $shellCropGraphics = [System.Drawing.Graphics]::FromImage($shellCropped)
    $shellCropGraphics.DrawImage($shellCanvas,
        [System.Drawing.Rectangle]::new(0, 0, $cropRect.Width, $cropRect.Height),
        $cropRect, [System.Drawing.GraphicsUnit]::Pixel)
    $shellOutputCanvas = [System.Drawing.Bitmap]::new($OutputWidth, $OutputHeight,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $shellOutputGraphics = [System.Drawing.Graphics]::FromImage($shellOutputCanvas)
    $shellOutputGraphics.Clear([System.Drawing.Color]::FromArgb(255, 0, 8, 17))
    $shellOutputGraphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $shellOutputGraphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $shellOutputGraphics.DrawImage($shellCropped,
        [System.Drawing.Rectangle]::new(0, 0, $shellOutputCanvas.Width, $shellOutputCanvas.Height))
    $shellOutputCanvas.Save($ShellFile, [System.Drawing.Imaging.ImageFormat]::Png)
    $outputDirectory = Split-Path -Parent $OutputFile
    Save-StateTile -Background $canvas -MaterialPath (Join-Path $InputDirectory 'h33off.png') -TilePath (Join-Path $outputDirectory 'headlight_off.png') -X 419 -Y 473
    Save-StateTile -Background $canvas -MaterialPath (Join-Path $InputDirectory 'h33on.png') -TilePath (Join-Path $outputDirectory 'headlight_on.png') -X 419 -Y 473
    Save-StateTile -Background $canvas -MaterialPath (Join-Path $InputDirectory 'v0.png') -TilePath (Join-Path $outputDirectory 'voice_idle.png') -X 500 -Y 532
    Save-StateTile -Background $canvas -MaterialPath (Join-Path $InputDirectory 'v1.png') -TilePath (Join-Path $outputDirectory 'voice_active.png') -X 500 -Y 532
}
finally {
	if ($shellOutputGraphics) { $shellOutputGraphics.Dispose() }
	if ($shellOutputCanvas) { $shellOutputCanvas.Dispose() }
	if ($shellCropGraphics) { $shellCropGraphics.Dispose() }
	if ($shellCropped) { $shellCropped.Dispose() }
	if ($shellGraphics) { $shellGraphics.Dispose() }
	if ($shellCanvas) { $shellCanvas.Dispose() }
	if ($outputGraphics) { $outputGraphics.Dispose() }
	if ($outputCanvas) { $outputCanvas.Dispose() }
	if ($cropGraphics) { $cropGraphics.Dispose() }
	if ($cropped) { $cropped.Dispose() }
    $graphics.Dispose()
    $canvas.Dispose()
}

Write-Host "Wrote $OutputFile"

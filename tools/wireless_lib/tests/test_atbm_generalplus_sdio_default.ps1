$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$workspace = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$driverHeaders = @(
    'gplib\liba_wifi_src\atbm\atbm6165\include\atbm_config.h',
    'gplib\liba_wifi_src\atbm\atbm6165\os\freertos\include\atbm_os_mem.h',
    'gplib\liba_wifi_src\atbm\atbm6165\os\freertos\include\atbm_os_sdio.h',
    'gplib\liba_wifi_src\atbm\atbm6165\os\freertos\include\os_api.h'
)

foreach ($relativePath in $driverHeaders) {
    $path = Join-Path $workspace $relativePath
    $source = Get-Content -LiteralPath $path -Raw
    if ($source -notmatch '(?s)#ifdef\s+ATBM_GENERALPLUS_SDIO\s+#undef\s+ATBM_GENERALPLUS_SDIO\s+#endif\s+#define\s+ATBM_GENERALPLUS_SDIO\s+1') {
        throw "$relativePath does not force ATBM_GENERALPLUS_SDIO=1"
    }
}

$projectFiles = Get-ChildItem -LiteralPath (Join-Path $workspace 'Project') -Recurse -File |
    Where-Object { $_.Extension -in @('.gpcmprj', '.mk') }
foreach ($projectFile in $projectFiles) {
    if ((Get-Content -LiteralPath $projectFile.FullName -Raw) -match '-DATBM_GENERALPLUS_SDIO(?:=1)?') {
        throw "Redundant ATBM_GENERALPLUS_SDIO option remains in $($projectFile.FullName)"
    }
}

Write-Host 'ATBM Generalplus SDIO default tests passed.'

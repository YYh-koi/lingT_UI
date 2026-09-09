$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$propertyPath = Join-Path $root 'btstack/core/btstack_property.c'

if (-not (Test-Path -LiteralPath $propertyPath)) {
    throw "Missing required file: $propertyPath"
}

$property = Get-Content -Raw -LiteralPath $propertyPath

$checks = @(
    @{
        Name = 'default Classic and BLE local name is GP-BT-ATBM6165'
        Pass = $property -match '#define\s+BTSTACK_PROPERTY_DEFAULT_NAME\s+"GP-BT-ATBM6165"'
    },
    @{
        Name = 'legacy default local name is removed'
        Pass = $property -notmatch '#define\s+BTSTACK_PROPERTY_DEFAULT_NAME\s+"BTstack Adapter"'
    },
    @{
        Name = 'runtime local-name setter remains available to AT backend'
        Pass = $property -match 'int\s+btstack_property_set_local_name\s*\('
    }
)

$failed = @($checks | Where-Object { -not $_.Pass })
if ($failed.Count -ne 0) {
    foreach ($check in $failed) {
        Write-Error "Missing: $($check.Name)"
    }
    exit 1
}

Write-Host 'BT xStack default local name is GP-BT-ATBM6165 and remains runtime-configurable'

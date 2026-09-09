$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (
    Split-Path -Parent (
        Split-Path -Parent $PSScriptRoot))
$configPath = Join-Path $root 'btstack/include/btstack_config.h'
$typesPath = Join-Path $root 'btstack/include/adapter/btstack_types.h'
$makePath = Join-Path $root 'btstack/Makefile.inc'
$gattClientPath = Join-Path $root 'btstack/src/ble/gatt_client.c'
$apiBuildPath = Join-Path $root 'Project/WirelessLibraries/BTstack_API_Library/build_btstack_api.ps1'
$gcc = 'D:\mingw32\bin\gcc.exe'
$failures = @()

function Require-Match {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text -notmatch $Pattern) {
        $script:failures += $Message
    }
}

foreach ($path in @($configPath, $typesPath, $makePath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        $failures += "missing $path"
    }
}
if (-not (Test-Path -LiteralPath $gcc)) {
    $failures += "missing host compiler $gcc"
}

if ($failures.Count -eq 0) {
    $config = Get-Content -Raw -LiteralPath $configPath
    $types = Get-Content -Raw -LiteralPath $typesPath
    $make = Get-Content -Raw -LiteralPath $makePath
    $apiBuild = Get-Content -Raw -LiteralPath $apiBuildPath

    $constants = [ordered]@{
        BTSTACK_LE_CONNECTION_MAX       = '1u'
        BTSTACK_GATT_SERVICE_MAX        = '4u'
        BTSTACK_GATT_CHARACTERISTIC_MAX = '16u'
        BTSTACK_GATT_DESCRIPTOR_MAX     = '8u'
        BTSTACK_GATT_DB_MAX             = '1024u'
        BTSTACK_GATT_VALUE_POOL_MAX     = '1024u'
        BTSTACK_GATT_VALUE_MAX          = '244u'
        BTSTACK_LE_BOND_MAX             = '4u'
    }
    foreach ($entry in $constants.GetEnumerator()) {
        Require-Match $config (
            '#define\s+' + [regex]::Escape($entry.Key) +
            '\s+' + [regex]::Escape($entry.Value)) (
            "missing BLE capacity $($entry.Key)=$($entry.Value)")
    }

    Require-Match $config (
        '(?s)#if\s+!BTSTACK_LE\s*&&\s*\(\s*BTSTACK_GATT_CLIENT\s*\|\|\s*' +
        'BTSTACK_GATT_SERVER\s*\|\|\s*BTSTACK_HOGP_DEVICE\s*\).*?#error') (
        'LE-disabled profile dependency check is missing')
    Require-Match $config (
        '(?s)#if\s+BTSTACK_HOGP_DEVICE\s*&&\s*!BTSTACK_GATT_SERVER.*?#error') (
        'HOGP-to-GATT-Server dependency check is missing')
    Require-Match $types 'BTSTACK_STATUS_SECURITY_ERROR\s*=\s*-9' (
        'BLE security terminal status is missing')
    Require-Match $types 'BTSTACK_STATUS_DISCONNECTED\s*=\s*-10' (
        'BLE disconnected terminal status is missing')
    Require-Match $apiBuild "'bt_xstack_le\.h'" (
        'API build must stage the LE public header')
    Require-Match $apiBuild "'bt_xstack_gatt\.h'" (
        'API build must stage the GATT public header')
    Require-Match $apiBuild 'include\\core' (
        'API build must stage canonical BLE headers, not wrapper paths')

    $sources = @(
        'src/ble/att_db.c',
        'src/ble/att_db_util.c',
        'src/ble/att_dispatch.c',
        'src/ble/att_server.c',
        'src/ble/gatt_client.c',
        'src/ble/sm.c',
        'src/ble/le_device_db_memory.c',
        'src/btstack_crypto.c',
        'src/btstack_tlv.c'
    )
    Require-Match $make 'BTSTACK_BLE_CORE_SOURCES\s*:=' (
        'BTSTACK_BLE_CORE_SOURCES group is missing')
    foreach ($source in $sources) {
        $normalized = [regex]::Escape(
            '$(BTSTACK_ROOT)/' + $source.Replace('\', '/'))
        $count = [regex]::Matches($make, $normalized).Count
        if ($count -ne 1) {
            $failures += "BLE source $source count=$count expected=1"
        }
        if (-not (Test-Path -LiteralPath (
                Join-Path (Join-Path $root 'btstack') $source))) {
            $failures += "missing BLE source $source"
        }
    }

    if (Test-Path -LiteralPath $gattClientPath) {
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $gattClientPath).Hash
        $expectedHash = '771D75A9F7549894854823A5E1D7E367F4125906DFA65CEBF4D2FA8347AC91C9'
        if ($hash -ne $expectedHash) {
            $failures += "gatt_client.c SHA256=$hash expected=$expectedHash"
        }
    }
}

if (($failures.Count -eq 0) -and (Test-Path -LiteralPath $gcc)) {
    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
        'btstack_ble_config_' + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $tempRoot | Out-Null
    try {
        $fixture = Join-Path $tempRoot 'fixture.c'
        Set-Content -LiteralPath $fixture -Encoding Ascii -Value @'
#include "btstack_config.h"
int main(void) { return 0; }
'@
        $matrix = @(
            @{ Name='off'; Le=0; Client=0; Server=0; Hogp=0; Pass=$true },
            @{ Name='client_without_le'; Le=0; Client=1; Server=0; Hogp=0; Pass=$false },
            @{ Name='client'; Le=1; Client=1; Server=0; Hogp=0; Pass=$true },
            @{ Name='server'; Le=1; Client=0; Server=1; Hogp=0; Pass=$true },
            @{ Name='hogp_without_server'; Le=1; Client=0; Server=0; Hogp=1; Pass=$false },
            @{ Name='hogp'; Le=1; Client=0; Server=1; Hogp=1; Pass=$true }
        )
        foreach ($case in $matrix) {
            $arguments = @(
                '-std=c99', '-Wall', '-Wextra', '-Werror', '-fsyntax-only',
                '-I', (Join-Path $root 'btstack/include'),
                "-DBTSTACK_LE=$($case.Le)",
                "-DBTSTACK_GATT_CLIENT=$($case.Client)",
                "-DBTSTACK_GATT_SERVER=$($case.Server)",
                "-DBTSTACK_HOGP_DEVICE=$($case.Hogp)",
                $fixture
            )
            $savedErrorActionPreference = $ErrorActionPreference
            $ErrorActionPreference = 'Continue'
            & $gcc @arguments 2>$null
            $passed = $LASTEXITCODE -eq 0
            $ErrorActionPreference = $savedErrorActionPreference
            if ($passed -ne $case.Pass) {
                $failures += "config matrix $($case.Name) pass=$passed expected=$($case.Pass)"
            }
        }
    } finally {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}

if ($failures.Count -ne 0) {
    $failures | ForEach-Object { Write-Host "FAIL: $_" }
    exit 1
}

Write-Host 'BTstack BLE configuration and source boundary PASS'

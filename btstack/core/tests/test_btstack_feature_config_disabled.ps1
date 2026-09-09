$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$gcc = 'D:\mingw32\bin\gcc.exe'
if (-not (Test-Path -LiteralPath $gcc -PathType Leaf)) {
    throw "Missing host compiler: $gcc"
}

$outputs = @()
$sources = @(
    (Join-Path $PSScriptRoot 'test_btstack_feature_config_disabled.c'),
    (Join-Path $root 'btstack/profiles/btstack_a2dp_media.c'),
    (Join-Path $root 'btstack/profiles/btstack_a2dp_sink.c'),
    (Join-Path $root 'btstack/profiles/btstack_avrcp.c'),
    (Join-Path $root 'btstack/profiles/btstack_avrcp_controller.c'),
    (Join-Path $root 'btstack/profiles/btstack_avrcp_target.c'),
    (Join-Path $root 'btstack/profiles/btstack_gatt_client.c'),
    (Join-Path $root 'btstack/profiles/btstack_gatt_server.c'),
    (Join-Path $root 'btstack/profiles/btstack_hfp_hf.c'),
    (Join-Path $root 'btstack/profiles/btstack_hid_device.c'),
    (Join-Path $root 'btstack/profiles/btstack_hogp_device.c'),
    (Join-Path $root 'btstack/profiles/btstack_pbap_client.c')
)
$allDisabledDefines = @(
    '-DBTSTACK_CLASSIC=0', '-DBTSTACK_LE=0',
    '-DBTSTACK_A2DP_SINK=0', '-DBTSTACK_HFP_HF=0',
    '-DBTSTACK_AVRCP_CONTROLLER=0', '-DBTSTACK_AVRCP_TARGET=0',
    '-DBTSTACK_PBAP_CLIENT=0', '-DBTSTACK_HID_DEVICE=0',
    '-DBTSTACK_HOGP_DEVICE=0', '-DBTSTACK_GATT_CLIENT=0',
    '-DBTSTACK_GATT_SERVER=0', '-DBTSTACK_BONDING=0'
)
$profileDisabledDefines = @(
    '-DBTSTACK_CLASSIC=1', '-DBTSTACK_LE=1',
    '-DBTSTACK_A2DP_SINK=0', '-DBTSTACK_HFP_HF=0',
    '-DBTSTACK_AVRCP_CONTROLLER=0', '-DBTSTACK_AVRCP_TARGET=0',
    '-DBTSTACK_PBAP_CLIENT=0', '-DBTSTACK_HID_DEVICE=0',
    '-DBTSTACK_HOGP_DEVICE=0', '-DBTSTACK_GATT_CLIENT=0',
    '-DBTSTACK_GATT_SERVER=0', '-DBTSTACK_BONDING=0'
)
$includes = @(
    ('-I' + (Join-Path $root 'btstack/include')),
    ('-I' + (Join-Path $root 'btstack/src')),
    ('-I' + (Join-Path $root 'btstack/profiles')),
    ('-I' + (Join-Path $root 'btstack/transport'))
)

try {
    $index = 0
    foreach ($defines in @($allDisabledDefines, $profileDisabledDefines)) {
        $output = Join-Path $env:TEMP "btstack_feature_config_disabled_$index.exe"
        $outputs += $output
        & $gcc -std=c99 -Wall -Wextra -Werror @defines @includes @sources -o $output
        if ($LASTEXITCODE -ne 0) { throw "Disabled-profile fixture $index did not compile" }
        & $output
        if ($LASTEXITCODE -ne 0) { throw "Disabled-profile APIs $index did not return NOT_SUPPORTED" }
        $index++
    }
} finally {
    foreach ($output in $outputs) {
        Remove-Item -LiteralPath $output -Force -ErrorAction SilentlyContinue
    }
}

Write-Output 'PASS: family-off and profile-off APIs remain linkable without native libraries'

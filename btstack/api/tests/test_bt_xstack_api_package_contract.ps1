$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$sourceHeaders = Join-Path $repoRoot 'btstack\include\api'
$sourceCoreHeaders = Join-Path $repoRoot 'btstack\include\core'
$package = Join-Path $repoRoot 'gplib\liba_wifi_src\atbm\atbm6165\bt_api'
$library = Join-Path $package 'libxstack_api.a'
$mapping = Join-Path $repoRoot 'btstack\README_API.md'
$commandTable = Join-Path $repoRoot 'btstack\at\btstack_at_command_table.c'
if (-not (Test-Path -LiteralPath $commandTable)) {
    $commandTable = Join-Path $repoRoot 'btstack\at\btstack_at_command_table.c'
}

if (-not (Test-Path -LiteralPath $library)) {
    throw 'Missing libxstack_api.a package artifact'
}

foreach ($header in @(
        'bt_xstack.h',
        'bt_xstack_types.h',
        'bt_xstack_service.h',
        'bt_xstack_phone.h',
        'bt_xstack_audio.h',
        'bt_xstack_phonebook.h')) {
    $source = Join-Path $sourceHeaders $header
    $packaged = Join-Path $package $header
    if (-not (Test-Path -LiteralPath $packaged)) {
        throw "Missing packaged public header: $header"
    }
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $source).Hash -ne
        (Get-FileHash -Algorithm SHA256 -LiteralPath $packaged).Hash) {
        throw "Packaged public header is stale: $header"
    }
}
foreach ($header in @('bt_xstack_le.h', 'bt_xstack_gatt.h')) {
    $source = Join-Path $sourceCoreHeaders $header
    $packaged = Join-Path $package $header
    if (-not (Test-Path -LiteralPath $packaged)) {
        throw "Missing packaged public header: $header"
    }
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $source).Hash -ne
        (Get-FileHash -Algorithm SHA256 -LiteralPath $packaged).Hash) {
        throw "Packaged public header is stale: $header"
    }
}

$mappingText = (Get-Content -Raw -LiteralPath $mapping) + "`n" +
    (Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'btstack\README_AT.md'))
$commandText = Get-Content -Raw -LiteralPath $commandTable
$commands = [regex]::Matches($commandText, 'COMMAND_ENTRY\("([A-Z0-9]+)"') |
    ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique
foreach ($command in $commands) {
    if ($mappingText -notmatch ('`' + [regex]::Escape($command))) {
        throw "Missing documented C API mapping for AT command: $command"
    }
}

$nm = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-nm.exe'
if (-not (Test-Path -LiteralPath $nm -PathType Leaf)) {
    throw "Required ARM nm tool is missing: $nm"
}
$symbols = (& $nm -g --defined-only $library) -join "`n"
foreach ($symbol in @(
        'bt_xstack_init',
        'bt_xstack_enable',
        'bt_xstack_get_state',
        'bt_xstack_hfp_dial',
        'bt_xstack_avrcp_play',
        'bt_xstack_phonebook_sync',
        'bt_xstack_phonebook_cancel',
        'bt_xstack_phonebook_authenticate',
        'bt_xstack_phonebook_get_state')) {
    if ($symbols -notmatch ("\b" + $symbol + "\b")) {
        throw "Missing exported library symbol: $symbol"
    }
}
foreach ($oldSymbol in @(
        'bt_xstack_pbap_sync',
        'bt_xstack_pbap_cancel',
        'bt_xstack_pbap_authenticate',
        'bt_xstack_get_pbap_state')) {
    if ($symbols -match ("\b" + $oldSymbol + "\b")) {
        throw "Packaged library still exports old public symbol: $oldSymbol"
    }
}
if ($symbols -match '\bbtstack_at_' -or
    $symbols -match '\bbt_xstack_send_at\b') {
    throw 'Packaged library exports an AT facade symbol'
}

Write-Output "xStack API package contains $($commands.Count) documented AT mappings"

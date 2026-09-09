$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$internalPath = Join-Path $root 'btstack/at/btstack_at_internal.h'
$commandHeaderPath = Join-Path $root 'btstack/at/btstack_at_command_table.h'
$commandSourcePath = Join-Path $root 'btstack/at/btstack_at_command_table.c'
$dispatchHeaderPath = Join-Path $root 'btstack/at/btstack_at_dispatch_table.h'
$dispatchSourcePath = Join-Path $root 'btstack/at/btstack_at_dispatch_table.c'

foreach ($path in @($internalPath, $commandHeaderPath, $commandSourcePath,
        $dispatchHeaderPath, $dispatchSourcePath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing AT table file: $path"
    }
}

$internal = Get-Content -Raw -LiteralPath $internalPath
$commandHeader = Get-Content -Raw -LiteralPath $commandHeaderPath
$commandSource = Get-Content -Raw -LiteralPath $commandSourcePath
$dispatchHeader = Get-Content -Raw -LiteralPath $dispatchHeaderPath
$dispatchSource = Get-Content -Raw -LiteralPath $dispatchSourcePath

$enumMatch = [regex]::Match(
    $internal,
    '(?s)typedef\s+enum\s*\{(?<body>.*?)\}\s*btstack_at_command_id_t\s*;')
if (-not $enumMatch.Success) {
    throw 'Cannot locate btstack_at_command_id_t'
}

$commands = @([regex]::Matches(
    $enumMatch.Groups['body'].Value,
    'BTSTACK_AT_COMMAND_[A-Z0-9_]+') | ForEach-Object { $_.Value } |
    Where-Object { $_ -ne 'BTSTACK_AT_COMMAND_UNSUPPORTED' } |
    Select-Object -Unique)

if ($commands.Count -lt 40) {
    throw "Unexpected AT command count: $($commands.Count)"
}

foreach ($command in $commands) {
    $count = [regex]::Matches(
        $commandSource,
        "\b$([regex]::Escape($command))\b").Count
    if ($count -ne 1) {
        throw "Command descriptor count for $command is $count, expected 1"
    }
}

foreach ($required in @(
        'btstack_at_validation_kind_t',
        'btstack_at_command_descriptor_t',
        'allowed_forms',
        'validation',
        'btstack_at_command_find')) {
    if (($commandHeader + $commandSource) -notmatch $required) {
        throw "Missing command-table contract: $required"
    }
}

$generalEnd = [array]::IndexOf($commands, 'BTSTACK_AT_COMMAND_SPKVOL')
$hfpEnd = [array]::IndexOf($commands, 'BTSTACK_AT_COMMAND_HFPVR')
if (($generalEnd -lt 0) -or ($hfpEnd -le $generalEnd)) {
    throw 'Cannot derive AT command domain boundaries'
}

for ($index = 0; $index -lt $commands.Count; $index++) {
    $domain = if (($commands[$index] -like 'BTSTACK_AT_COMMAND_PBAP*') -or
        ($commands[$index] -like 'BTSTACK_AT_COMMAND_LE*') -or
        ($commands[$index] -like 'BTSTACK_AT_COMMAND_GATT*')) {
        'BTSTACK_AT_DOMAIN_GENERAL'
    } elseif ($index -le $generalEnd) {
        'BTSTACK_AT_DOMAIN_GENERAL'
    } elseif ($index -le $hfpEnd) {
        'BTSTACK_AT_DOMAIN_HFP'
    } else {
        'BTSTACK_AT_DOMAIN_A2DP_AVRCP'
    }
    $pattern = 'DISPATCH_ENTRY\s*\(\s*' +
        [regex]::Escape($commands[$index]) + '\s*,\s*' +
        [regex]::Escape($domain) + '\s*\)'
    if ([regex]::Matches($dispatchSource, $pattern).Count -ne 1) {
        throw "Missing unique domain mapping: $($commands[$index]) -> $domain"
    }
}

foreach ($required in @(
        'btstack_at_command_domain_t',
        'btstack_at_command_get_domain')) {
    if (($dispatchHeader + $dispatchSource) -notmatch $required) {
        throw "Missing dispatch-table contract: $required"
    }
}

if ($commandSource -match '\b(?:malloc|calloc|realloc|free)\s*\(') {
    throw 'AT command table must not use heap memory'
}

Write-Host "AT command descriptor table covers $($commands.Count) commands"

param(
    [string]$ToolchainDirectory = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$nm = Join-Path $ToolchainDirectory 'arm-none-eabi-nm.exe'
if (-not (Test-Path -LiteralPath $nm)) {
    throw "ARM nm tool is missing: $nm"
}

function Get-SymbolTable([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Selected-build artifact is missing: $path"
    }
    $lines = @(& $nm -S $path)
    if ($LASTEXITCODE -ne 0) { throw "ARM nm failed: $path" }
    $table = @{}
    foreach ($line in $lines) {
        if ($line -match '^\s*[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+[bBdD]\s+(\S+)\s*$') {
            $sizeHex = $Matches[1]
            $name = $Matches[2]
            if ($name -notmatch '^(?:workers|btstack_(?:api|event|at)_(?:control|stack))$') {
                continue
            }
            if ($table.ContainsKey($name)) {
                throw "Duplicate static worker resource symbol $name in $path"
            }
            $table[$name] = [Convert]::ToUInt32($sizeHex, 16)
        }
    }
    return $table
}

function Assert-ExactResources(
    [string]$name,
    [hashtable]$symbols,
    [string[]]$required,
    [string[]]$forbidden,
    [int]$expectedTotal) {
    $total = 0
    foreach ($symbol in $required) {
        if (-not $symbols.ContainsKey($symbol)) {
            throw "$name is missing static worker resource $symbol"
        }
        $total += [int]$symbols[$symbol]
    }
    foreach ($symbol in $forbidden) {
        if ($symbols.ContainsKey($symbol)) {
            throw "$name retains unselected static worker resource $symbol"
        }
    }
    if ($total -ne $expectedTotal) {
        throw "$name static worker RAM=$total, expected=$expectedTotal"
    }
    return $total
}

$apiRequired = @(
    'workers',
    'btstack_api_control',
    'btstack_event_control',
    'btstack_api_stack',
    'btstack_event_stack'
)
$atRequired = @(
    'workers',
    'btstack_at_control',
    'btstack_at_stack'
)
$apiForbidden = @('btstack_at_control', 'btstack_at_stack')
$atForbidden = @(
    'btstack_api_control',
    'btstack_event_control',
    'btstack_api_stack',
    'btstack_event_stack'
)

$artifacts = @(
    [pscustomobject]@{
        Name = 'API object'
        Path = 'Project\WirelessLibraries\BTstack_API_Library\obj\Debug\btstack\ports\gpcv2178\btstack_rtos_gpcv2178.o'
        Required = $apiRequired
        Forbidden = $apiForbidden
        Total = 12600
    },
    [pscustomobject]@{
        Name = 'AT object'
        Path = 'Project\WirelessLibraries\BTstack_AT_Library\obj\Debug\btstack\ports\gpcv2178\btstack_rtos_gpcv2178.o'
        Required = $atRequired
        Forbidden = $atForbidden
        Total = 8396
    },
    [pscustomobject]@{
        Name = 'Legacy AT object'
        Path = 'Project\WirelessLibraries\BTstack_Library\obj\Debug\btstack\ports\gpcv2178\btstack_rtos_gpcv2178.o'
        Required = $atRequired
        Forbidden = $atForbidden
        Total = 8396
    },
    [pscustomobject]@{
        Name = 'API archive'
        Path = 'gplib\liba_wifi_src\atbm\atbm6165\bt_api\libxstack_api.a'
        Required = $apiRequired
        Forbidden = $apiForbidden
        Total = 12600
    },
    [pscustomobject]@{
        Name = 'AT archive'
        Path = 'gplib\liba_wifi_src\atbm\atbm6165\bt_at\libxstack_at.a'
        Required = $atRequired
        Forbidden = $atForbidden
        Total = 8396
    },
    [pscustomobject]@{
        Name = 'Legacy AT archive'
        Path = 'gplib\liba_wifi_src\atbm\atbm6165\bt\libxstack.a'
        Required = $atRequired
        Forbidden = $atForbidden
        Total = 8396
    },
    [pscustomobject]@{
        Name = 'API final ELF'
        Path = 'Project\GPCV2178_Turnkey_SDK\prjDebug\bin\Debug\project.elf'
        Required = $apiRequired
        Forbidden = $apiForbidden
        Total = 12600
    }
)

foreach ($artifact in $artifacts) {
    $table = Get-SymbolTable (Join-Path $repoRoot $artifact.Path)
    $total = Assert-ExactResources $artifact.Name $table `
        $artifact.Required $artifact.Forbidden $artifact.Total
    if ([int]$table['workers'] -ne 96) {
        throw "$($artifact.Name) worker descriptor RAM=$($table['workers']), expected=96"
    }
    Write-Output "$($artifact.Name): static worker RAM=$total bytes"
}

Write-Output 'PASS: ARM objects, archives, and final ELF retain only selected static worker RAM'

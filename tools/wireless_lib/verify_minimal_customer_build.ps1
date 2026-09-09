[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$CustomerRoot,

    [string]$ToolchainDirectory = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin',

    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'
$toolDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$workspaceRoot = (Resolve-Path (Join-Path $toolDirectory '..\..')).Path
$customer = (Resolve-Path $CustomerRoot).Path
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $runName = 'run-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' +
        [guid]::NewGuid().ToString('N').Substring(0, 8)
    $OutputRoot = Join-Path $workspaceRoot "output\wireless_libs\minimal-customer-verify\$runName"
}
else {
    $OutputRoot = [IO.Path]::GetFullPath($OutputRoot)
}

Import-Module (Join-Path $toolDirectory 'WirelessLibTools.psm1') -Force

$projectFile = Join-Path $customer `
    'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj'
$inputs = Get-DebugProjectInputs -ProjectFile $projectFile -WorkspaceRoot $customer
$configuration = Get-DebugCompilerConfiguration -ProjectFile $projectFile
if ($inputs.CompileUnits.Count -ne 309) {
    throw "Unexpected candidate compile unit count: $($inputs.CompileUnits.Count)"
}
if ($inputs.Archives.Count -ne 20) {
    throw "Unexpected candidate archive count: $($inputs.Archives.Count)"
}
if ($inputs.LinkerScripts.Count -ne 1) {
    throw "Unexpected candidate linker script count: $($inputs.LinkerScripts.Count)"
}

foreach ($path in @(
    $inputs.ProjectFile
    $inputs.CompileUnits
    $inputs.Archives
    $inputs.LinkerScripts
    $configuration.IncludeDirectories
)) {
    Convert-PathBetweenRoots `
        -Path $path `
        -SourceRoot $customer `
        -DestinationRoot $OutputRoot | Out-Null
}

$gcc = Join-Path $ToolchainDirectory 'arm-none-eabi-gcc.exe'
$gxx = Join-Path $ToolchainDirectory 'arm-none-eabi-g++.exe'
$nm = Join-Path $ToolchainDirectory 'arm-none-eabi-nm.exe'
$sizeTool = Join-Path $ToolchainDirectory 'arm-none-eabi-size.exe'
foreach ($tool in @($gcc, $gxx, $nm, $sizeTool)) {
    if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) {
        throw "Required verification tool not found: $tool"
    }
}

$objectRoot = Join-Path $OutputRoot 'obj'
New-Item -ItemType Directory -Path $objectRoot -Force | Out-Null
$includeArguments = @($configuration.IncludeDirectories |
    ForEach-Object { "-I$_" })
$objectPaths = @()
$compileCount = $inputs.CompileUnits.Count
for ($index = 0; $index -lt $compileCount; $index++) {
    $source = $inputs.CompileUnits[$index]
    $mappedPath = Convert-PathBetweenRoots `
        -Path $source `
        -SourceRoot $customer `
        -DestinationRoot $objectRoot
    $objectPath = [IO.Path]::ChangeExtension($mappedPath, '.o')
    $objectDirectory = Split-Path -Parent $objectPath
    if (-not (Test-Path -LiteralPath $objectDirectory -PathType Container)) {
        New-Item -ItemType Directory -Path $objectDirectory -Force | Out-Null
    }

    $extension = [IO.Path]::GetExtension($source)
    $compiler = if ($extension -ieq '.cpp') { $gxx } else { $gcc }
    $languageArguments = if ($extension -ceq '.S') {
        @('-x', 'assembler-with-cpp')
    }
    else {
        @()
    }
    Invoke-CheckedProcess `
        -FilePath $compiler `
        -Arguments @(
            $configuration.Options +
            $includeArguments +
            $languageArguments +
            @('-c', $source, '-o', $objectPath)
        ) `
        -Description "Compile $source" `
        -Quiet | Out-Null
    $objectPaths += $objectPath

    if ($index -eq 0 -or ($index + 1) % 25 -eq 0 -or $index + 1 -eq $compileCount) {
        Write-Host ("Compiled {0}/{1}" -f ($index + 1), $compileCount)
    }
}

$elfPath = Join-Path $OutputRoot 'minimal_customer_sdk.elf'
$mapPath = Join-Path $OutputRoot 'minimal_customer_sdk.map'
$responsePath = Join-Path $OutputRoot 'link.rsp'
$linkArguments = @(
    '-Wl,-Map',
    "-Wl,$mapPath",
    '-Wl,--gc-sections',
    '-n',
    '-Wl,-cref',
    '-mcpu=cortex-a7',
    "-T$($inputs.LinkerScripts[0])",
    '-specs=nosys.specs',
    '-mfpu=neon-vfpv4',
    '-mfloat-abi=softfp'
) + $objectPaths + $inputs.Archives + @('-lm', '-o', $elfPath)
$responseArguments = @($linkArguments | ForEach-Object {
    ([string]$_).Replace('\', '/')
})
[IO.File]::WriteAllLines(
    $responsePath,
    $responseArguments,
    (New-Object Text.UTF8Encoding($false))
)

Invoke-CheckedProcess `
    -FilePath $gcc `
    -Arguments @('@' + $responsePath.Replace('\', '/')) `
    -Description 'Link minimal customer SDK' `
    -Quiet | Out-Null

$sizeOutput = Invoke-CheckedProcess `
    -FilePath $sizeTool `
    -Arguments @($elfPath) `
    -Description 'Measure minimal customer ELF' `
    -Quiet
$symbolOutput = Invoke-CheckedProcess `
    -FilePath $nm `
    -Arguments @('-g', '--defined-only', $elfPath) `
    -Description 'Inspect minimal customer ELF symbols' `
    -Quiet
$requiredElfSymbols = @(
    'atbm_wifi_start_ap',
    'btstack_gpcv2178_start',
    'btstack_run_loop_gpcv2178_get_instance'
)
foreach ($symbol in $requiredElfSymbols) {
    if (-not (@($symbolOutput) -match "\s$([regex]::Escape($symbol))$")) {
        throw "Required linked wireless symbol is not defined: $symbol"
    }
}

$wifiArchive = @($inputs.Archives |
    Where-Object { [IO.Path]::GetFileName($_) -eq 'libatbm_wifi.a' })
if ($wifiArchive.Count -ne 1) {
    throw "Expected one libatbm_wifi.a archive, found $($wifiArchive.Count)."
}
$wifiSymbolOutput = Invoke-CheckedProcess `
    -FilePath $nm `
    -Arguments @('-g', '--defined-only', $wifiArchive[0]) `
    -Description 'Inspect customer WiFi archive symbols' `
    -Quiet
$requiredWifiApiSymbols = @(
    'atbm_wifi_start_ap',
    'atbm_wifi_stop_ap',
    'atbm_wifi_start_sta',
    'atbm_wifi_stop_sta',
    'atbm_wifi_start_p2p',
    'atbm_wifi_stop_p2p',
    'atbm_wifi_scan',
    'atbm_wifi_get_status',
    'atbm_wifi_get_mac',
    'atbm_wifi_register_event_callback',
    'atbm_wifi_deinit'
)
foreach ($symbol in $requiredWifiApiSymbols) {
    if (-not (@($wifiSymbolOutput) -match "\s$([regex]::Escape($symbol))$")) {
        throw "Required WiFi archive symbol is not defined: $symbol"
    }
}

$elfHash = (Get-FileHash -LiteralPath $elfPath -Algorithm SHA256).Hash
Write-Host "Minimal customer build verified."
Write-Host "Compile units: $compileCount"
Write-Host "Archives: $($inputs.Archives.Count)"
Write-Host "ELF: $elfPath"
Write-Host "ELF SHA-256: $elfHash"
@($sizeOutput) | ForEach-Object { Write-Host $_ }

[PSCustomObject]@{
    CustomerRoot = $customer
    OutputRoot = $OutputRoot
    CompileUnitCount = $compileCount
    ArchiveCount = $inputs.Archives.Count
    ElfPath = $elfPath
    MapPath = $mapPath
    ElfSha256 = $elfHash
    RequiredElfSymbols = $requiredElfSymbols
    RequiredWifiApiSymbols = $requiredWifiApiSymbols
}

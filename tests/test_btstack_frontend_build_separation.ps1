$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$apiProject = Join-Path $root 'Project\WirelessLibraries\BTstack_API_Library\BTstack_API_Library.gpcmprj'
$atProject = Join-Path $root 'Project\WirelessLibraries\BTstack_AT_Library\BTstack_AT_Library.gpcmprj'

foreach ($path in @($apiProject, $atProject)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing frontend library project: $path"
    }
}

function Get-SourceSet([string]$path) {
    $text = Get-Content -Raw -LiteralPath $path
    $matches = [regex]::Matches($text, 'filename="[^\"]*btstack/([^\"]+\.(?:c|S))"')
    return @($matches | ForEach-Object { $_.Groups[1].Value.Replace('\', '/') } | Sort-Object -Unique)
}

$apiSources = Get-SourceSet $apiProject
$atSources = Get-SourceSet $atProject
if ($apiSources.Count -eq 0 -or $atSources.Count -eq 0) {
    throw 'Frontend projects must compile sources from the unified btstack tree'
}
if ($apiSources | Where-Object { $_ -like 'at/*' }) {
    throw 'API project compiles AT frontend sources'
}
if ($atSources | Where-Object { $_ -like 'api/*' }) {
    throw 'AT project compiles API frontend sources'
}
if (-not ($apiSources | Where-Object { $_ -like 'api/*' })) {
    throw 'API project does not compile the API frontend'
}
if (-not ($atSources | Where-Object { $_ -like 'at/*' })) {
    throw 'AT project does not compile the AT frontend'
}

$apiCommon = @($apiSources | Where-Object { $_ -notlike 'api/*' })
$atCommon = @($atSources | Where-Object { $_ -notlike 'at/*' })
$difference = Compare-Object $apiCommon $atCommon
if ($difference) {
    throw "API and AT projects do not share the same common source manifest:`n$($difference | Out-String)"
}

Write-Output 'API and AT projects share one kernel and select one frontend'


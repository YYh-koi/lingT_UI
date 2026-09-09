[CmdletBinding()]
param(
    [string]$WorkspaceRoot,
    [string]$ProjectFile,
    [string]$ManifestFile,
    [string]$ToolchainDirectory = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin',
    [string]$OutputDirectory,
    [string]$CustomerWirelessRoot
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$toolDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
Import-Module (Join-Path $toolDirectory 'WirelessLibTools.psm1') -Force

if (-not $WorkspaceRoot) {
    $WorkspaceRoot = [IO.Path]::GetFullPath((Join-Path $toolDirectory '..\..'))
}
if (-not $ProjectFile) {
    $ProjectFile = Join-Path $WorkspaceRoot 'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj'
}
if (-not $ManifestFile) {
    $ManifestFile = Join-Path $toolDirectory 'wireless_sources.json'
}
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $WorkspaceRoot 'output\wireless_libs'
}
if (-not $CustomerWirelessRoot) {
    $CustomerWirelessRoot = Join-Path $WorkspaceRoot `
        'gplib\liba_wifi_src\atbm\atbm6165'
}

$manifest = Read-WirelessSourceManifest -Path $ManifestFile -WorkspaceRoot $WorkspaceRoot
$configuration = Get-DebugCompilerConfiguration -ProjectFile $ProjectFile
$plan = New-WirelessCompilePlan `
    -Manifest $manifest `
    -Configuration $configuration `
    -ToolchainDirectory $ToolchainDirectory `
    -OutputDirectory $OutputDirectory `
    -WorkspaceRoot $WorkspaceRoot

$results = Invoke-WirelessArchiveBuild `
    -Plan $plan `
    -CustomerWirelessRoot $CustomerWirelessRoot

$results | Format-Table Name, ObjectCount, InternalPath, CustomerPath -AutoSize
Write-Host 'Wireless library build completed.'

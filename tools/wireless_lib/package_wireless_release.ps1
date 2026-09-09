[CmdletBinding()]
param(
    [string]$WorkspaceRoot,
    [string]$StageRoot,
    [string]$ToolchainDirectory = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin'
)

$ErrorActionPreference = 'Stop'
$toolDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
if ([string]::IsNullOrWhiteSpace($WorkspaceRoot)) {
    $WorkspaceRoot = (Resolve-Path (Join-Path $toolDirectory '..\..')).Path
}
else {
    $WorkspaceRoot = (Resolve-Path $WorkspaceRoot).Path
}
if ([string]::IsNullOrWhiteSpace($StageRoot)) {
    $workspaceParent = Split-Path -Parent $WorkspaceRoot
    $workspaceName = Split-Path -Leaf $WorkspaceRoot
    $StageRoot = Join-Path $workspaceParent ($workspaceName + '_customer')
}

$stripTool = Join-Path $ToolchainDirectory 'arm-none-eabi-strip.exe'
$ranlibTool = Join-Path $ToolchainDirectory 'arm-none-eabi-ranlib.exe'
$projectFile = Join-Path $WorkspaceRoot `
    'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj'

Import-Module (Join-Path $toolDirectory 'WirelessLibTools.psm1') -Force
$inputs = Get-DebugProjectInputs `
    -ProjectFile $projectFile `
    -WorkspaceRoot $WorkspaceRoot
$configuration = Get-DebugCompilerConfiguration -ProjectFile $projectFile
Write-Host "Resolving dependencies for $($inputs.CompileUnits.Count) Debug compile units..."
$dependencies = Get-GccProjectDependencies `
    -Inputs $inputs `
    -Configuration $configuration `
    -ToolchainDirectory $ToolchainDirectory `
    -WorkspaceRoot $WorkspaceRoot
Write-Host "Project-local dependency files: $($dependencies.Count)"

$result = New-MinimalWirelessCustomerStage `
    -WorkspaceRoot $WorkspaceRoot `
    -StageRoot $StageRoot `
    -ProjectFile $projectFile `
    -DependencyPaths $dependencies `
    -CustomerStripTool $stripTool `
    -CustomerRanlibTool $ranlibTool

Write-Host "Customer SDK staged: $($result.StageRoot)"
Write-Host "Files checksummed: $($result.FileCount)"
Write-Host "Headers staged: $($result.HeaderCount)"
Write-Host "Checksum manifest: $($result.ChecksumPath)"

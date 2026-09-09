[CmdletBinding()]
param(
    [string]$WorkspaceRoot,
    [string]$MainProject,
    [string]$Manifest
)

$ErrorActionPreference = 'Stop'
$toolDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
if ([string]::IsNullOrWhiteSpace($WorkspaceRoot)) {
    $WorkspaceRoot = (Resolve-Path (Join-Path $toolDirectory '..\..')).Path
}
else {
    $WorkspaceRoot = (Resolve-Path $WorkspaceRoot).Path
}
if ([string]::IsNullOrWhiteSpace($MainProject)) {
    $MainProject = Join-Path $WorkspaceRoot 'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj'
}
if ([string]::IsNullOrWhiteSpace($Manifest)) {
    $Manifest = Join-Path $toolDirectory 'wireless_sources.json'
}

Import-Module (Join-Path $toolDirectory 'WirelessLibTools.psm1') -Force
$sourceManifest = Read-WirelessSourceManifest -Path $Manifest -WorkspaceRoot $WorkspaceRoot
$configuration = Get-DebugCompilerConfiguration -ProjectFile $MainProject

function Get-ProjectRelativePath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BaseDirectory,

        [Parameter(Mandatory = $true)]
        [string]$TargetPath
    )

    $basePath = [IO.Path]::GetFullPath($BaseDirectory).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $fullTargetPath = [IO.Path]::GetFullPath($TargetPath)
    $baseUri = New-Object Uri($basePath)
    $targetUri = New-Object Uri($fullTargetPath)
    [Uri]::UnescapeDataString($baseUri.MakeRelativeUri($targetUri).ToString())
}

function Write-WirelessLibraryProject {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Title,

        [Parameter(Mandatory = $true)]
        [string]$ProjectPath,

        [Parameter(Mandatory = $true)]
        [string]$OutputPath,

        [Parameter(Mandatory = $true)]
        [string[]]$Sources,

        [string[]]$AdditionalCompilerOptions = @(),

        [string[]]$AdditionalIncludeDirectories = @(),

        [string[]]$ExcludedIncludeDirectories = @()
    )

    $projectDirectory = Split-Path -Parent $ProjectPath
    $wirelessProjectRoot = [IO.Path]::GetFullPath((Join-Path $WorkspaceRoot 'Project\WirelessLibraries'))
    $fullProjectDirectory = [IO.Path]::GetFullPath($projectDirectory)
    if (-not $fullProjectDirectory.StartsWith(
        $wirelessProjectRoot + [IO.Path]::DirectorySeparatorChar,
        [StringComparison]::OrdinalIgnoreCase
    )) {
        throw "Project output is outside Project/WirelessLibraries: $ProjectPath"
    }
    if ($Sources.Count -eq 0) {
        throw "Project contains no sources: $Title"
    }
    if (@($Sources | Sort-Object -Unique).Count -ne $Sources.Count) {
        throw "Project contains duplicate sources: $Title"
    }
    $missingSources = @($Sources | Where-Object { -not (Test-Path -LiteralPath $_ -PathType Leaf) })
    if ($missingSources.Count -ne 0) {
        throw "Project contains missing source: $($missingSources[0])"
    }
    if (-not (Test-Path -LiteralPath $projectDirectory -PathType Container)) {
        New-Item -ItemType Directory -Path $projectDirectory -Force | Out-Null
    }

    $settings = New-Object Xml.XmlWriterSettings
    $settings.Encoding = New-Object Text.UTF8Encoding($false)
    $settings.Indent = $true
    $settings.IndentChars = "`t"
    $settings.NewLineChars = "`r`n"
    $settings.NewLineHandling = [Xml.NewLineHandling]::Replace
    $settings.OmitXmlDeclaration = $false

    $writer = [Xml.XmlWriter]::Create($ProjectPath, $settings)
    try {
        $writer.WriteStartDocument($true)
        $writer.WriteStartElement('emIDE_project_file')
        $writer.WriteStartElement('FileVersion')
        $writer.WriteAttributeString('major', '1')
        $writer.WriteAttributeString('minor', '7')
        $writer.WriteEndElement()
        $writer.WriteStartElement('Project')

        foreach ($projectOption in @(
            [ordered]@{ title = $Title },
            [ordered]@{ execution_dir = '.' },
            [ordered]@{ pch_mode = '2' },
            [ordered]@{ compiler = 'armelfgcc' },
            [ordered]@{ DownloadLog = '-1' }
        )) {
            $writer.WriteStartElement('Option')
            foreach ($attribute in $projectOption.GetEnumerator()) {
                $writer.WriteAttributeString($attribute.Key, [string]$attribute.Value)
            }
            $writer.WriteEndElement()
        }

        $writer.WriteStartElement('Build')
        $writer.WriteStartElement('Target')
        $writer.WriteAttributeString('title', 'Debug')
        foreach ($targetOption in @(
            [ordered]@{ output = $OutputPath; prefix_auto = '1'; extension_auto = '1' },
            [ordered]@{ working_dir = '' },
            [ordered]@{ object_output = 'obj/Debug/' },
            [ordered]@{ type = '0' },
            [ordered]@{ compiler = 'armelfgcc' },
            [ordered]@{ createDefFile = '1' }
        )) {
            $writer.WriteStartElement('Option')
            foreach ($attribute in $targetOption.GetEnumerator()) {
                $writer.WriteAttributeString($attribute.Key, [string]$attribute.Value)
            }
            $writer.WriteEndElement()
        }

        $writer.WriteStartElement('Compiler')
        foreach ($option in @($configuration.Options + $AdditionalCompilerOptions)) {
            $writer.WriteStartElement('Add')
            $writer.WriteAttributeString('option', $option)
            $writer.WriteEndElement()
        }
        $includeDirectories = @($configuration.IncludeDirectories | Where-Object {
            $candidate = $_
            -not @($ExcludedIncludeDirectories | Where-Object {
                $excludedRoot = [IO.Path]::GetFullPath($_).TrimEnd('\', '/')
                $candidate.Equals($excludedRoot, [StringComparison]::OrdinalIgnoreCase) -or
                    $candidate.StartsWith(
                        $excludedRoot + [IO.Path]::DirectorySeparatorChar,
                        [StringComparison]::OrdinalIgnoreCase)
            }).Count
        })
        foreach ($additionalIncludeDirectory in $AdditionalIncludeDirectories) {
            if ($includeDirectories -notcontains $additionalIncludeDirectory) {
                $includeDirectories += $additionalIncludeDirectory
            }
        }
        foreach ($includeDirectory in $includeDirectories) {
            $writer.WriteStartElement('Add')
            $writer.WriteAttributeString(
                'directory',
                (Get-ProjectRelativePath -BaseDirectory $projectDirectory -TargetPath $includeDirectory)
            )
            $writer.WriteEndElement()
        }
        $writer.WriteEndElement()

        $writer.WriteStartElement('Linker')
        $writer.WriteEndElement()
        $writer.WriteStartElement('PostOutput')
        foreach ($postOutput in @(
            [ordered]@{ PostOutputCreate = '0' },
            [ordered]@{ PostOutputType = 'HEX' },
            [ordered]@{ PostOutputFile = '${TARGET_OUTPUT_DIR}${TARGET_OUTPUT_BASENAME}' }
        )) {
            $writer.WriteStartElement('ConvertedOutput')
            foreach ($attribute in $postOutput.GetEnumerator()) {
                $writer.WriteAttributeString($attribute.Key, [string]$attribute.Value)
            }
            $writer.WriteEndElement()
        }
        $writer.WriteEndElement()
        $writer.WriteEndElement()
        $writer.WriteEndElement()

        foreach ($source in $Sources) {
            $writer.WriteStartElement('Unit')
            $writer.WriteAttributeString(
                'filename',
                (Get-ProjectRelativePath -BaseDirectory $projectDirectory -TargetPath $source)
            )
            $writer.WriteStartElement('Option')
            $writer.WriteAttributeString('compilerVar', 'CC')
            $writer.WriteEndElement()
            $writer.WriteEndElement()
        }

        $writer.WriteStartElement('Extensions')
        $writer.WriteStartElement('projectwizard')
        $writer.WriteAttributeString('TargetDevice', 'GPA7740A')
        $writer.WriteAttributeString('isCreated', 'true')
        $writer.WriteEndElement()
        $writer.WriteStartElement('code_completion')
        $writer.WriteEndElement()
        $writer.WriteEndElement()

        $writer.WriteEndElement()
        $writer.WriteEndElement()
        $writer.WriteEndDocument()
    }
    finally {
        $writer.Dispose()
    }

    Write-Host "Generated $(Split-Path -Leaf $ProjectPath) with $($Sources.Count) source units"
}

$wifiProjectPath = Join-Path $WorkspaceRoot 'Project\WirelessLibraries\ATBM_WiFi_Library\ATBM_WiFi_Library.gpcmprj'
$btProjectPath = Join-Path $WorkspaceRoot 'Project\WirelessLibraries\BTstack_Library\BTstack_Library.gpcmprj'

Write-WirelessLibraryProject `
    -Title 'ATBM_WiFi_Library' `
    -ProjectPath $wifiProjectPath `
    -OutputPath '../../../gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi' `
    -Sources @($sourceManifest.Wifi) `
    -ExcludedIncludeDirectories @(
        (Join-Path $WorkspaceRoot 'btstack\3rd-party\bluedroid\decoder\include')
    )

Write-WirelessLibraryProject `
    -Title 'BTstack_Library' `
    -ProjectPath $btProjectPath `
    -OutputPath '../../../gplib/liba_wifi_src/atbm/atbm6165/bt/xstack' `
    -Sources @($sourceManifest.Bluetooth) `
    -AdditionalCompilerOptions @(
        '-DBTSTACK_BUILD_VARIANT_AT=1',
        '-include ../../../btstack/ports/gpcv2178/btstack_gpcv2178_config.h'
    ) `
    -AdditionalIncludeDirectories @(
        (Join-Path $WorkspaceRoot 'btstack\ports\gpcv2178'),
        (Join-Path $WorkspaceRoot 'btstack\include'),
        (Join-Path $WorkspaceRoot 'btstack\src'),
        (Join-Path $WorkspaceRoot 'btstack\3rd-party\bluedroid\decoder\include'),
        (Join-Path $WorkspaceRoot 'btstack\3rd-party\md5'),
        (Join-Path $WorkspaceRoot 'btstack\3rd-party\yxml'),
        (Join-Path $WorkspaceRoot 'btstack\transport'),
        (Join-Path $WorkspaceRoot 'btstack\gap'),
        (Join-Path $WorkspaceRoot 'btstack\core'),
        (Join-Path $WorkspaceRoot 'btstack\at'),
        (Join-Path $WorkspaceRoot 'btstack\include\core'),
        (Join-Path $WorkspaceRoot 'btstack\include\at')
    ) `
    -ExcludedIncludeDirectories @(
        (Join-Path $WorkspaceRoot 'gplib\liba_wifi_src\atbm\atbm6165\platform\generalplus\sdio')
    )

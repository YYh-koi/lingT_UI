Set-StrictMode -Version Latest

function Get-NormalizedFullPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$BasePath
    )

    if ([IO.Path]::IsPathRooted($Path)) {
        return [IO.Path]::GetFullPath($Path)
    }

    return [IO.Path]::GetFullPath((Join-Path $BasePath $Path))
}

function Read-WirelessSourceManifest {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$WorkspaceRoot
    )

    $manifestPath = Get-NormalizedFullPath -Path $Path -BasePath (Get-Location).Path
    $rootPath = Get-NormalizedFullPath -Path $WorkspaceRoot -BasePath (Get-Location).Path
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw "Wireless source manifest not found: $manifestPath"
    }

    $data = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($data.schema_version -ne 1) {
        throw "Unsupported wireless source manifest schema: $($data.schema_version)"
    }

    $wifiRelative = @($data.wifi | ForEach-Object { [string]$_ })
    $bluetoothRelative = @($data.bluetooth | ForEach-Object { [string]$_ })
    if ($wifiRelative.Count -ne [int]$data.expected_counts.wifi) {
        throw "WiFi manifest count mismatch: expected $($data.expected_counts.wifi), got $($wifiRelative.Count)"
    }
    if ($bluetoothRelative.Count -ne [int]$data.expected_counts.bluetooth) {
        throw "Bluetooth manifest count mismatch: expected $($data.expected_counts.bluetooth), got $($bluetoothRelative.Count)"
    }

    $allRelative = @($wifiRelative + $bluetoothRelative)
    $duplicates = @($allRelative | Group-Object | Where-Object Count -gt 1)
    if ($duplicates.Count -ne 0) {
        throw "Duplicate wireless source path: $($duplicates[0].Name)"
    }

    foreach ($relativePath in $allRelative) {
        if ([IO.Path]::GetExtension($relativePath) -ne '.c') {
            throw "Wireless source is not a C file: $relativePath"
        }
        $fullPath = Get-NormalizedFullPath -Path $relativePath -BasePath $rootPath
        if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
            throw "Wireless source not found: $relativePath"
        }
        if (-not $fullPath.StartsWith($rootPath, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Wireless source escapes workspace: $relativePath"
        }
    }

    return [PSCustomObject]@{
        SchemaVersion = 1
        Project = [string]$data.project
        Configuration = [string]$data.configuration
        WifiRelative = $wifiRelative
        BluetoothRelative = $bluetoothRelative
        Wifi = @($wifiRelative | ForEach-Object { Get-NormalizedFullPath -Path $_ -BasePath $rootPath })
        Bluetooth = @($bluetoothRelative | ForEach-Object { Get-NormalizedFullPath -Path $_ -BasePath $rootPath })
    }
}

function Get-DebugCompilerConfiguration {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$ProjectFile
    )

    $projectPath = Get-NormalizedFullPath -Path $ProjectFile -BasePath (Get-Location).Path
    if (-not (Test-Path -LiteralPath $projectPath -PathType Leaf)) {
        throw "Project file not found: $projectPath"
    }

    [xml]$projectXml = Get-Content -LiteralPath $projectPath -Raw
    $target = @($projectXml.emIDE_project_file.Project.Build.Target |
        Where-Object { [string]$_.title -eq 'Debug' }) | Select-Object -First 1
    if ($null -eq $target) {
        throw "Debug target not found in project: $projectPath"
    }

    $options = @($target.Compiler.Add |
        Where-Object { $_.HasAttribute('option') } |
        ForEach-Object { [string]$_.option })
    $requiredOptions = @(
        '-O2',
        '-mcpu=cortex-a7',
        '-mfpu=neon-vfpv4',
        '-mfloat-abi=softfp',
        '-DWIFI_VENDOR=3'
    )
    foreach ($requiredOption in $requiredOptions) {
        if ($options -notcontains $requiredOption) {
            throw "Required Debug compiler option missing: $requiredOption"
        }
    }

    $projectDirectory = Split-Path -Parent $projectPath
    $includeDirectories = @($target.Compiler.Add |
        Where-Object { $_.HasAttribute('directory') } |
        ForEach-Object {
            Get-NormalizedFullPath -Path ([string]$_.directory) -BasePath $projectDirectory
        })

    return [PSCustomObject]@{
        ProjectFile = $projectPath
        ProjectDirectory = $projectDirectory
        Options = $options
        IncludeDirectories = $includeDirectories
    }
}

function Get-DebugProjectInputs {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$ProjectFile,

        [Parameter(Mandatory = $true)]
        [string]$WorkspaceRoot
    )

    $workspace = Get-NormalizedFullPath -Path $WorkspaceRoot -BasePath (Get-Location).Path
    $project = Get-NormalizedFullPath -Path $ProjectFile -BasePath (Get-Location).Path
    if (-not (Test-Path -LiteralPath $project -PathType Leaf)) {
        throw "Project file not found: $project"
    }

    $projectDirectory = Split-Path -Parent $project
    [xml]$projectXml = Get-Content -LiteralPath $project -Raw
    $target = @($projectXml.emIDE_project_file.Project.Build.Target |
        Where-Object { [string]$_.title -eq 'Debug' }) | Select-Object -First 1
    if ($null -eq $target) {
        throw "Debug target not found in project: $project"
    }

    $compileExtensions = @('.c', '.cpp', '.s')
    $compileUnits = @($projectXml.emIDE_project_file.Project.Unit |
        Where-Object { $_.HasAttribute('filename') } |
        ForEach-Object {
            $path = Get-NormalizedFullPath `
                -Path ([string]$_.filename) `
                -BasePath $projectDirectory
            $extension = [IO.Path]::GetExtension($path).ToLowerInvariant()
            if ($compileExtensions -contains $extension) {
                if (-not $path.StartsWith($workspace, [StringComparison]::OrdinalIgnoreCase)) {
                    throw "Compile unit escapes workspace: $path"
                }
                if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
                    throw "Compile unit not found: $path"
                }
                $path
            }
        } |
        Sort-Object -Unique)

    $archives = @($target.Linker.Add |
        Where-Object { $_.HasAttribute('library') } |
        ForEach-Object {
            Get-NormalizedFullPath `
                -Path ([string]$_.library) `
                -BasePath $projectDirectory
        })
    foreach ($archive in $archives) {
        if (-not $archive.StartsWith($workspace, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Linked archive escapes workspace: $archive"
        }
        if (-not (Test-Path -LiteralPath $archive -PathType Leaf)) {
            throw "Linked archive not found: $archive"
        }
    }

    $linkerScripts = @($target.Linker.Add |
        Where-Object {
            $_.HasAttribute('option') -and
            [string]$_.option -match '^-T(.+)$'
        } |
        ForEach-Object {
            $option = [string]$_.option
            $scriptPath = $option.Substring(2)
            Get-NormalizedFullPath -Path $scriptPath -BasePath $projectDirectory
        })
    foreach ($linkerScript in $linkerScripts) {
        if (-not $linkerScript.StartsWith($workspace, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Linker script escapes workspace: $linkerScript"
        }
        if (-not (Test-Path -LiteralPath $linkerScript -PathType Leaf)) {
            throw "Linker script not found: $linkerScript"
        }
    }

    [PSCustomObject]@{
        ProjectFile = $project
        ProjectDirectory = $projectDirectory
        CompileUnits = $compileUnits
        Archives = $archives
        LinkerScripts = $linkerScripts
    }
}

function ConvertFrom-GccDependencyOutput {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyString()]
        [string[]]$Output,

        [Parameter(Mandatory = $true)]
        [string]$WorkspaceRoot
    )

    $workspace = Get-NormalizedFullPath -Path $WorkspaceRoot -BasePath (Get-Location).Path
    $joined = (($Output | ForEach-Object { [string]$_ }) -join ' ') -replace '\\\s+', ' '
    $separatorIndex = $joined.IndexOf(':')
    if ($separatorIndex -lt 0) {
        throw 'Invalid GCC dependency output: target separator not found.'
    }

    $payload = $joined.Substring($separatorIndex + 1).Trim()
    $paths = foreach ($token in $payload -split '\s+') {
        if ([string]::IsNullOrWhiteSpace($token)) {
            continue
        }
        $unescaped = $token.Replace('\ ', ' ').Replace('/', '\')
        try {
            $candidate = [IO.Path]::GetFullPath($unescaped)
        }
        catch {
            continue
        }
        if ($candidate.StartsWith($workspace, [StringComparison]::OrdinalIgnoreCase) -and
            (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            $candidate
        }
    }

    return @($paths | Sort-Object -Unique)
}

function New-GccDependencyBatches {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$CompileUnits,

        [ValidateRange(1, 1000)]
        [int]$BatchSize = 40
    )

    $classified = foreach ($source in $CompileUnits) {
        $extension = [IO.Path]::GetExtension($source)
        $language = if ($extension -ieq '.cpp') {
            'c++'
        }
        elseif ($extension -ceq '.S') {
            'assembler-with-cpp'
        }
        elseif ($extension -ceq '.s') {
            'plain-assembly'
        }
        else {
            'c'
        }
        [PSCustomObject]@{
            Language = $language
            Source = $source
        }
    }

    $batches = foreach ($language in @('c', 'c++', 'assembler-with-cpp', 'plain-assembly')) {
        $sources = @($classified |
            Where-Object { $_.Language -eq $language } |
            ForEach-Object { $_.Source } |
            Sort-Object)
        for ($offset = 0; $offset -lt $sources.Count; $offset += $BatchSize) {
            $lastIndex = [Math]::Min($offset + $BatchSize - 1, $sources.Count - 1)
            [PSCustomObject]@{
                Language = $language
                Sources = @($sources[$offset..$lastIndex])
            }
        }
    }

    return @($batches)
}

function Get-GccProjectDependencies {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        $Inputs,

        [Parameter(Mandatory = $true)]
        $Configuration,

        [Parameter(Mandatory = $true)]
        [string]$ToolchainDirectory,

        [Parameter(Mandatory = $true)]
        [string]$WorkspaceRoot
    )

    $toolchain = Get-NormalizedFullPath `
        -Path $ToolchainDirectory `
        -BasePath (Get-Location).Path
    $gcc = Join-Path $toolchain 'arm-none-eabi-gcc.exe'
    $gxx = Join-Path $toolchain 'arm-none-eabi-g++.exe'
    foreach ($compiler in @($gcc, $gxx)) {
        if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) {
            throw "Required dependency compiler not found: $compiler"
        }
    }

    $includeArguments = @($Configuration.IncludeDirectories |
        ForEach-Object { "-I$_" })
    $batches = New-GccDependencyBatches -CompileUnits $Inputs.CompileUnits
    $dependencyPaths = @($batches |
        Where-Object { $_.Language -eq 'plain-assembly' } |
        ForEach-Object { $_.Sources })
    $processEntries = @()
    $responsePaths = @()

    try {
        foreach ($batch in @($batches | Where-Object { $_.Language -ne 'plain-assembly' })) {
            $compiler = if ($batch.Language -eq 'c++') { $gxx } else { $gcc }
            $languageArguments = if ($batch.Language -eq 'assembler-with-cpp') {
                @('-x', 'assembler-with-cpp')
            }
            else {
                @()
            }
            $arguments = @(
                $Configuration.Options +
                $includeArguments +
                $languageArguments +
                @('-MM', '-MT', 'dependency-target') +
                $batch.Sources
            ) | ForEach-Object {
                ([string]$_).Replace('\', '/')
            }

            $responsePath = Join-Path `
                ([IO.Path]::GetTempPath()) `
                ("wireless-dependencies-{0}.rsp" -f [guid]::NewGuid().ToString('N'))
            [IO.File]::WriteAllLines(
                $responsePath,
                $arguments,
                (New-Object Text.UTF8Encoding($false))
            )
            $responsePaths += $responsePath

            $startInfo = New-Object Diagnostics.ProcessStartInfo
            $startInfo.FileName = $compiler
            $startInfo.Arguments = '@' + $responsePath.Replace('\', '/')
            $startInfo.UseShellExecute = $false
            $startInfo.CreateNoWindow = $true
            $startInfo.RedirectStandardOutput = $true
            $startInfo.RedirectStandardError = $true

            $process = New-Object Diagnostics.Process
            $process.StartInfo = $startInfo
            if (-not $process.Start()) {
                throw "Failed to start dependency compiler for $($batch.Language)."
            }
            $processEntries += [PSCustomObject]@{
                Batch = $batch
                Process = $process
                StandardOutput = $process.StandardOutput.ReadToEndAsync()
                StandardError = $process.StandardError.ReadToEndAsync()
            }
        }

        foreach ($entry in $processEntries) {
            $entry.Process.WaitForExit()
            $stdout = $entry.StandardOutput.Result
            $stderr = $entry.StandardError.Result
            if ($entry.Process.ExitCode -ne 0) {
                $tail = @(($stderr -split "\r?\n") | Select-Object -Last 30) -join [Environment]::NewLine
                throw "Dependency generation failed for $($entry.Batch.Language):`n$tail"
            }
            $dependencyPaths += ConvertFrom-GccDependencyOutput `
                -Output @($stdout -split "\r?\n") `
                -WorkspaceRoot $WorkspaceRoot
            $entry.Process.Dispose()
        }
    }
    finally {
        foreach ($responsePath in $responsePaths) {
            if (Test-Path -LiteralPath $responsePath -PathType Leaf) {
                Remove-Item -LiteralPath $responsePath -Force
            }
        }
        foreach ($entry in $processEntries) {
            if ($null -ne $entry.Process) {
                $entry.Process.Dispose()
            }
        }
    }

    return @($dependencyPaths | Sort-Object -Unique)
}

function New-WirelessCompileAction {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SourcePath,

        [Parameter(Mandatory = $true)]
        [string]$RelativePath,

        [Parameter(Mandatory = $true)]
        [string]$ModuleName,

        [Parameter(Mandatory = $true)]
        [string]$ObjectRoot,

        [string[]]$AdditionalArguments = @()
    )

    $objectRelativePath = [IO.Path]::ChangeExtension($RelativePath, '.o')
    return [PSCustomObject]@{
        Module = $ModuleName
        SourcePath = $SourcePath
        RelativePath = $RelativePath
        ObjectPath = [IO.Path]::GetFullPath((Join-Path (Join-Path $ObjectRoot $ModuleName) $objectRelativePath))
        AdditionalArguments = @($AdditionalArguments)
    }
}

function New-WirelessCompilePlan {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        $Manifest,

        [Parameter(Mandatory = $true)]
        $Configuration,

        [Parameter(Mandatory = $true)]
        [string]$ToolchainDirectory,

        [Parameter(Mandatory = $true)]
        [string]$OutputDirectory,

        [Parameter(Mandatory = $true)]
        [string]$WorkspaceRoot
    )

    $rootPath = Get-NormalizedFullPath -Path $WorkspaceRoot -BasePath (Get-Location).Path
    $toolchainPath = Get-NormalizedFullPath -Path $ToolchainDirectory -BasePath (Get-Location).Path
    $outputPath = Get-NormalizedFullPath -Path $OutputDirectory -BasePath $rootPath
    $compiler = Join-Path $toolchainPath 'arm-none-eabi-gcc.exe'
    $archiver = Join-Path $toolchainPath 'arm-none-eabi-ar.exe'
    $ranlib = Join-Path $toolchainPath 'arm-none-eabi-ranlib.exe'
    $strip = Join-Path $toolchainPath 'arm-none-eabi-strip.exe'
    $nm = Join-Path $toolchainPath 'arm-none-eabi-nm.exe'
    foreach ($tool in @($compiler, $archiver, $ranlib, $strip, $nm)) {
        if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) {
            throw "Required tool not found: $tool"
        }
    }

    $objectRoot = Join-Path $outputPath 'obj'
    $wifiActions = for ($index = 0; $index -lt $Manifest.Wifi.Count; $index++) {
        New-WirelessCompileAction `
            -SourcePath $Manifest.Wifi[$index] `
            -RelativePath $Manifest.WifiRelative[$index] `
            -ModuleName 'wifi' `
            -ObjectRoot $objectRoot
    }
    $bluetoothActions = for ($index = 0; $index -lt $Manifest.Bluetooth.Count; $index++) {
        New-WirelessCompileAction `
            -SourcePath $Manifest.Bluetooth[$index] `
            -RelativePath $Manifest.BluetoothRelative[$index] `
            -ModuleName 'bluetooth' `
            -ObjectRoot $objectRoot `
            -AdditionalArguments @(
                '-include',
                (Join-Path $rootPath 'btstack\ports\gpcv2178\btstack_gpcv2178_config.h')
            )
    }

    $allObjects = @($wifiActions.ObjectPath + $bluetoothActions.ObjectPath)
    if (@($allObjects | Sort-Object -Unique).Count -ne $allObjects.Count) {
        throw 'Compile plan contains duplicate object paths'
    }

    return [PSCustomObject]@{
        WorkspaceRoot = $rootPath
        OutputDirectory = $outputPath
        ObjectDirectory = $objectRoot
        InternalLibraryDirectory = (Join-Path $outputPath 'internal')
        Compiler = $compiler
        Archiver = $archiver
        Ranlib = $ranlib
        Strip = $strip
        Nm = $nm
        Options = @($Configuration.Options)
        IncludeDirectories = @(($Configuration.IncludeDirectories + @(
            (Join-Path $rootPath 'btstack\ports\gpcv2178'),
            (Join-Path $rootPath 'btstack\include'),
            (Join-Path $rootPath 'btstack\src'),
            (Join-Path $rootPath 'btstack\3rd-party\bluedroid\decoder\include'),
            (Join-Path $rootPath 'btstack\3rd-party\md5'),
            (Join-Path $rootPath 'btstack\3rd-party\yxml'),
            (Join-Path $rootPath 'btstack\transport'),
            (Join-Path $rootPath 'btstack\gap'),
            (Join-Path $rootPath 'btstack\core'),
            (Join-Path $rootPath 'btstack\at'),
            (Join-Path $rootPath 'btstack\include\core'),
            (Join-Path $rootPath 'btstack\include\at')
        )) | Select-Object -Unique)
        WifiActions = @($wifiActions)
        BluetoothActions = @($bluetoothActions)
    }
}

function Invoke-CheckedProcess {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,

        [string]$Description = $FilePath,

        [switch]$Quiet
    )

    if (-not (Test-Path -LiteralPath $FilePath -PathType Leaf)) {
        throw "Executable not found: $FilePath"
    }

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $processOutput = @(& $FilePath @Arguments 2>&1)
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
    if (-not $Quiet -and $processOutput.Count -ne 0) {
        $processOutput | ForEach-Object { Write-Host $_ }
    }
    if ($exitCode -ne 0) {
        $tail = @($processOutput | Select-Object -Last 30) -join [Environment]::NewLine
        throw "$Description failed with exit code ${exitCode}:`n$tail"
    }

    return $processOutput
}

function Invoke-WirelessCompileActions {
    param(
        [Parameter(Mandatory = $true)]
        $Plan,

        [Parameter(Mandatory = $true)]
        [object[]]$Actions,

        [Parameter(Mandatory = $true)]
        [string]$ModuleName
    )

    $includeArguments = @($Plan.IncludeDirectories | ForEach-Object { "-I$_" })
    $count = $Actions.Count
    for ($index = 0; $index -lt $count; $index++) {
        $action = $Actions[$index]
        $objectDirectory = Split-Path -Parent $action.ObjectPath
        if (-not (Test-Path -LiteralPath $objectDirectory -PathType Container)) {
            New-Item -ItemType Directory -Path $objectDirectory -Force | Out-Null
        }
        Write-Host ("[{0} {1}/{2}] {3}" -f $ModuleName, ($index + 1), $count, $action.RelativePath)
        $arguments = @($Plan.Options + $includeArguments + $action.AdditionalArguments + @(
            '-c',
            $action.SourcePath,
            '-o',
            $action.ObjectPath
        ))
        Invoke-CheckedProcess `
            -FilePath $Plan.Compiler `
            -Arguments $arguments `
            -Description "Compile $($action.RelativePath)" `
            -Quiet | Out-Null
    }
}

function New-ArchiveArguments {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$ArchivePath,

        [Parameter(Mandatory = $true)]
        [string[]]$ObjectPaths
    )

    return @(@('rcsD', $ArchivePath) + $ObjectPaths)
}

function Invoke-WirelessArchiveBuild {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        $Plan,

        [Parameter(Mandatory = $true)]
        [string]$CustomerWirelessRoot
    )

    $customerRoot = Get-NormalizedFullPath `
        -Path $CustomerWirelessRoot `
        -BasePath $Plan.WorkspaceRoot
    $wifiDirectory = Join-Path $customerRoot 'wlan_lib'
    $bluetoothDirectory = Join-Path $customerRoot 'bt'
    foreach ($directory in @(
        $Plan.InternalLibraryDirectory,
        $wifiDirectory,
        $bluetoothDirectory
    )) {
        if (-not (Test-Path -LiteralPath $directory -PathType Container)) {
            New-Item -ItemType Directory -Path $directory -Force | Out-Null
        }
    }

    Invoke-WirelessCompileActions -Plan $Plan -Actions $Plan.WifiActions -ModuleName 'wifi'
    Invoke-WirelessCompileActions -Plan $Plan -Actions $Plan.BluetoothActions -ModuleName 'bluetooth'

    $archives = @(
        [PSCustomObject]@{
            Name = 'libatbm_wifi.a'
            Objects = @($Plan.WifiActions.ObjectPath)
            DeliveryDirectory = $wifiDirectory
            HeaderSource = Join-Path $Plan.WorkspaceRoot `
                'gplib\liba_wifi_src\atbm\atbm6165\api\atbm_wifi.h'
            HeaderName = 'atbm_wifi.h'
        },
        [PSCustomObject]@{
            Name = 'libxstack.a'
            Objects = @($Plan.BluetoothActions.ObjectPath)
            DeliveryDirectory = $bluetoothDirectory
            HeaderSource = Join-Path $Plan.WorkspaceRoot `
                'btstack\include\adapter\bt_xstack.h'
            HeaderName = 'bt_xstack.h'
        }
    )

    $results = foreach ($archive in $archives) {
        $internalPath = Join-Path $Plan.InternalLibraryDirectory $archive.Name
        $customerPath = Join-Path $archive.DeliveryDirectory $archive.Name
        $customerHeaderPath = Join-Path $archive.DeliveryDirectory $archive.HeaderName
        $archiveArguments = New-ArchiveArguments `
            -ArchivePath $internalPath `
            -ObjectPaths $archive.Objects
        Invoke-CheckedProcess `
            -FilePath $Plan.Archiver `
            -Arguments $archiveArguments `
            -Description "Archive $($archive.Name)" `
            -Quiet | Out-Null
        Invoke-CheckedProcess `
            -FilePath $Plan.Ranlib `
            -Arguments @('-D', $internalPath) `
            -Description "Index $($archive.Name)" `
            -Quiet | Out-Null

        Copy-Item -LiteralPath $internalPath -Destination $customerPath -Force
        Invoke-CheckedProcess `
            -FilePath $Plan.Strip `
            -Arguments @('--strip-debug', $customerPath) `
            -Description "Strip debug data from $($archive.Name)" `
            -Quiet | Out-Null
        Invoke-CheckedProcess `
            -FilePath $Plan.Ranlib `
            -Arguments @('-D', $customerPath) `
            -Description "Reindex stripped $($archive.Name)" `
            -Quiet | Out-Null
        Copy-Item `
            -LiteralPath $archive.HeaderSource `
            -Destination $customerHeaderPath `
            -Force

        [PSCustomObject]@{
            Name = $archive.Name
            InternalPath = $internalPath
            CustomerPath = $customerPath
            CustomerHeaderPath = $customerHeaderPath
            ObjectCount = $archive.Objects.Count
            InternalSha256 = (Get-FileHash -LiteralPath $internalPath -Algorithm SHA256).Hash
            CustomerSha256 = (Get-FileHash -LiteralPath $customerPath -Algorithm SHA256).Hash
        }
    }

    return @($results)
}

function Test-PathWithinRoot {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Root
    )

    $normalizedPath = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    $normalizedRoot = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    return $normalizedPath.Equals($normalizedRoot, [StringComparison]::OrdinalIgnoreCase) -or
        $normalizedPath.StartsWith(
            $normalizedRoot + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase
        )
}

function Convert-PathBetweenRoots {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$SourceRoot,

        [Parameter(Mandatory = $true)]
        [string]$DestinationRoot
    )

    $source = Get-NormalizedFullPath -Path $SourceRoot -BasePath (Get-Location).Path
    $destination = Get-NormalizedFullPath -Path $DestinationRoot -BasePath (Get-Location).Path
    $normalizedPath = Get-NormalizedFullPath -Path $Path -BasePath $source
    if (-not (Test-PathWithinRoot -Path $normalizedPath -Root $source)) {
        throw "Path is outside source root: $normalizedPath"
    }

    $relativePath = $normalizedPath.Substring($source.Length).TrimStart('\', '/')
    return [IO.Path]::GetFullPath((Join-Path $destination $relativePath))
}

function New-MinimalWirelessCustomerStage {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$WorkspaceRoot,

        [Parameter(Mandatory = $true)]
        [string]$StageRoot,

        [Parameter(Mandatory = $true)]
        [string]$ProjectFile,

        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [string[]]$DependencyPaths,

        [string]$CustomerStripTool,

        [string]$CustomerRanlibTool
    )

    $workspace = Get-NormalizedFullPath -Path $WorkspaceRoot -BasePath (Get-Location).Path
    $stage = Get-NormalizedFullPath -Path $StageRoot -BasePath $workspace
    $project = Get-NormalizedFullPath -Path $ProjectFile -BasePath (Get-Location).Path
    if (-not (Test-Path -LiteralPath $workspace -PathType Container)) {
        throw "Workspace root does not exist: $workspace"
    }
    if ($stage -eq $workspace) {
        throw 'Customer stage must not be the workspace root.'
    }
    if (Test-Path -LiteralPath $stage) {
        if (@(Get-ChildItem -LiteralPath $stage -Force).Count -ne 0) {
            throw "Customer stage already exists and is not empty: $stage"
        }
    }
    else {
        New-Item -ItemType Directory -Path $stage -Force | Out-Null
    }

    $inputs = Get-DebugProjectInputs -ProjectFile $project -WorkspaceRoot $workspace
    $atbmCustomerRoot = Join-Path $workspace 'gplib\liba_wifi_src\atbm\atbm6165'
    $atbmNetworkRoot = Join-Path $workspace 'gplib\liba_wifi_src\atbm\network'
    if (-not (Test-Path -LiteralPath $atbmNetworkRoot -PathType Container)) {
        throw "ATBM customer network directory is missing: $atbmNetworkRoot"
    }
    $networkDeliverablePaths = @(
        Get-ChildItem -LiteralPath $atbmNetworkRoot -Recurse -File |
            Select-Object -ExpandProperty FullName
    )
    if ($networkDeliverablePaths.Count -eq 0) {
        throw "ATBM customer network directory is empty: $atbmNetworkRoot"
    }
    $wirelessDeliverablePaths = @(
        Join-Path $atbmCustomerRoot 'bt\libxstack.a'
        Join-Path $atbmCustomerRoot 'bt\bt_xstack.h'
        Join-Path $atbmCustomerRoot 'wlan_lib\libatbm_wifi.a'
        Join-Path $atbmCustomerRoot 'wlan_lib\atbm_wifi.h'
    )
    $keepFiles = @(
        $inputs.ProjectFile
        $inputs.CompileUnits
        $inputs.Archives
        $inputs.LinkerScripts
        $DependencyPaths
        $wirelessDeliverablePaths
        $networkDeliverablePaths
    ) | Sort-Object -Unique

    foreach ($sourcePath in $keepFiles) {
        $normalizedSource = Get-NormalizedFullPath `
            -Path $sourcePath `
            -BasePath $workspace
        if (-not (Test-PathWithinRoot -Path $normalizedSource -Root $workspace)) {
            throw "Customer file escapes workspace: $normalizedSource"
        }
        if (-not (Test-Path -LiteralPath $normalizedSource -PathType Leaf)) {
            throw "Required customer file is missing: $normalizedSource"
        }
        if ($normalizedSource -match '[\\/](btstack|atbm6165)[\\/].+\.c$') {
            throw "Wireless implementation source cannot be staged: $normalizedSource"
        }
        if ((Test-PathWithinRoot -Path $normalizedSource -Root $atbmCustomerRoot) -and
            ($wirelessDeliverablePaths -notcontains $normalizedSource)) {
            throw "Customer dependency uses a non-delivery ATBM file: $normalizedSource"
        }

        $relativePath = $normalizedSource.Substring($workspace.Length).TrimStart('\', '/')
        $destinationPath = Join-Path $stage $relativePath
        $destinationDirectory = Split-Path -Parent $destinationPath
        if (-not (Test-Path -LiteralPath $destinationDirectory -PathType Container)) {
            New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null
        }
        Copy-Item -LiteralPath $normalizedSource -Destination $destinationPath -Force
    }

    if ([string]::IsNullOrWhiteSpace($CustomerStripTool) -xor
        [string]::IsNullOrWhiteSpace($CustomerRanlibTool)) {
        throw 'Customer strip and ranlib tools must be supplied together.'
    }
    if (-not [string]::IsNullOrWhiteSpace($CustomerStripTool)) {
        foreach ($relativePath in @(
            'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libatbm_wifi.a',
            'gplib\liba_wifi_src\atbm\atbm6165\bt\libxstack.a'
        )) {
            $archivePath = Join-Path $stage $relativePath
            if (-not (Test-Path -LiteralPath $archivePath -PathType Leaf)) {
                throw "Required wireless archive is missing from minimal stage: $archivePath"
            }
            Invoke-CheckedProcess `
                -FilePath $CustomerStripTool `
                -Arguments @('--strip-debug', $archivePath) `
                -Description "Strip debug data from $relativePath" `
                -Quiet | Out-Null
            Invoke-CheckedProcess `
                -FilePath $CustomerRanlibTool `
                -Arguments @('-D', $archivePath) `
                -Description "Reindex stripped $relativePath" `
                -Quiet | Out-Null
        }
    }

    $checksumPath = Join-Path $stage 'SHA256SUMS.txt'
    $checksumLines = @(
        Get-ChildItem -LiteralPath $stage -Recurse -File |
            Where-Object { $_.FullName -ne $checksumPath } |
            ForEach-Object {
                $relativePath = $_.FullName.Substring($stage.Length).TrimStart('\', '/').Replace('\', '/')
                [PSCustomObject]@{
                    Path = $relativePath
                    Length = $_.Length
                    Hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
                }
            } |
            Sort-Object Path |
            ForEach-Object { "$($_.Hash)  $($_.Length)  $($_.Path)" }
    )
    [IO.File]::WriteAllLines(
        $checksumPath,
        $checksumLines,
        (New-Object Text.UTF8Encoding($false))
    )

    foreach ($line in Get-Content -LiteralPath $checksumPath) {
        if ($line -notmatch '^([0-9A-F]{64})  ([0-9]+)  (.+)$') {
            throw "Invalid checksum manifest line: $line"
        }
        $filePath = Join-Path $stage $Matches[3].Replace('/', '\')
        $file = Get-Item -LiteralPath $filePath
        if ($file.Length -ne [int64]$Matches[2]) {
            throw "Checksum length mismatch: $($Matches[3])"
        }
        if ((Get-FileHash -LiteralPath $filePath -Algorithm SHA256).Hash -ne $Matches[1]) {
            throw "Checksum mismatch: $($Matches[3])"
        }
    }

    [PSCustomObject]@{
        StageRoot = $stage
        FileCount = $checksumLines.Count
        HeaderCount = @(
            Get-ChildItem -LiteralPath $stage -Recurse -File |
                Where-Object { $_.Extension.ToLowerInvariant() -in @('.h', '.h2', '.hpp', '.inc') }
        ).Count
        ChecksumPath = $checksumPath
    }
}

function New-WirelessCustomerStage {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$WorkspaceRoot,

        [Parameter(Mandatory = $true)]
        [string]$StageRoot,

        [string]$CustomerStripTool,

        [string]$CustomerRanlibTool
    )

    $workspace = Get-NormalizedFullPath -Path $WorkspaceRoot -BasePath (Get-Location).Path
    $stage = Get-NormalizedFullPath -Path $StageRoot -BasePath $workspace
    if (-not (Test-Path -LiteralPath $workspace -PathType Container)) {
        throw "Workspace root does not exist: $workspace"
    }
    if ($stage -eq $workspace) {
        throw 'Customer stage must not be the workspace root.'
    }
    if (Test-Path -LiteralPath $stage) {
        $existingEntries = @(Get-ChildItem -LiteralPath $stage -Force)
        if ($existingEntries.Count -ne 0) {
            throw "Customer stage already exists and is not empty: $stage"
        }
    }
    else {
        New-Item -ItemType Directory -Path $stage -Force | Out-Null
    }

    $wifiRoot = Join-Path $workspace 'gplib\liba_wifi_src\atbm\atbm6165'
    $bluetoothRoot = Join-Path $workspace 'btstack'
    $excludedDirectories = @(
        (Join-Path $workspace 'output'),
        (Join-Path $workspace 'tmp'),
        (Join-Path $workspace '.codebase-memory'),
        (Join-Path $workspace 'atbm_rtos_wifi6'),
        (Join-Path $workspace 'Project\WirelessLibraries'),
        $wifiRoot,
        $bluetoothRoot,
        $stage,
        'prjDebug',
        'tests',
        'backup',
        'backups',
        '.git',
        '.svn'
    )
    $robocopyArguments = @(
        $workspace,
        $stage,
        '/E',
        '/COPY:DAT',
        '/DCOPY:T',
        '/R:1',
        '/W:1',
        '/NFL',
        '/NDL',
        '/NJH',
        '/NJS',
        '/NP',
        '/XF',
        '*.o',
        '*.obj',
        '*.d',
        '*.dep',
        '*.bak',
        '*.tmp',
        '/XD'
    ) + $excludedDirectories
    & robocopy.exe @robocopyArguments | Out-Null
    $robocopyExitCode = $LASTEXITCODE
    if ($robocopyExitCode -ge 8) {
        throw "Customer SDK copy failed with robocopy exit code $robocopyExitCode."
    }

    $requiredFiles = @(
        'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libatbm_wifi.a',
        'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\atbm_wifi.h',
        'gplib\liba_wifi_src\atbm\atbm6165\bt\libxstack.a',
        'gplib\liba_wifi_src\atbm\atbm6165\bt\bt_xstack.h',
        'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj',
        'patches\wireless_static_library_integration.patch',
        'docs\wireless_static_library_integration.md'
    )
    foreach ($relativePath in $requiredFiles) {
        $sourcePath = Join-Path $workspace $relativePath
        $destinationPath = Join-Path $stage $relativePath
        if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
            throw "Required customer artifact is missing: $sourcePath"
        }
        $destinationDirectory = Split-Path -Parent $destinationPath
        if (-not (Test-Path -LiteralPath $destinationDirectory -PathType Container)) {
            New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null
        }
        Copy-Item -LiteralPath $sourcePath -Destination $destinationPath -Force
    }

    if ([string]::IsNullOrWhiteSpace($CustomerStripTool) -xor
        [string]::IsNullOrWhiteSpace($CustomerRanlibTool)) {
        throw 'Customer strip and ranlib tools must be supplied together.'
    }
    if (-not [string]::IsNullOrWhiteSpace($CustomerStripTool)) {
        foreach ($relativePath in @(
            'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libatbm_wifi.a',
            'gplib\liba_wifi_src\atbm\atbm6165\bt\libxstack.a'
        )) {
            $archivePath = Join-Path $stage $relativePath
            Invoke-CheckedProcess `
                -FilePath $CustomerStripTool `
                -Arguments @('--strip-debug', $archivePath) `
                -Description "Strip debug data from $relativePath" `
                -Quiet | Out-Null
            Invoke-CheckedProcess `
                -FilePath $CustomerRanlibTool `
                -Arguments @('-D', $archivePath) `
                -Description "Reindex stripped $relativePath" `
                -Quiet | Out-Null
        }
    }

    $wirelessSources = @(
        Get-ChildItem -LiteralPath $stage -Recurse -File -Filter '*.c' |
            Where-Object {
                $_.FullName -match '[\\/](btstack|atbm6165|atbm_rtos_wifi6)[\\/]'
            }
    )
    if ($wirelessSources.Count -ne 0) {
        throw "Customer stage contains $($wirelessSources.Count) wireless C source file(s)."
    }

    $checksumPath = Join-Path $stage 'SHA256SUMS.txt'
    $checksumLines = @(
        Get-ChildItem -LiteralPath $stage -Recurse -File |
            Where-Object { $_.FullName -ne $checksumPath } |
            ForEach-Object {
                $relativePath = $_.FullName.Substring($stage.Length).TrimStart('\', '/').Replace('\', '/')
                [PSCustomObject]@{
                    Path = $relativePath
                    Length = $_.Length
                    Hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
                }
            } |
            Sort-Object Path |
            ForEach-Object { "$($_.Hash)  $($_.Length)  $($_.Path)" }
    )
    [IO.File]::WriteAllLines($checksumPath, $checksumLines, (New-Object Text.UTF8Encoding($false)))

    foreach ($line in Get-Content -LiteralPath $checksumPath) {
        if ($line -notmatch '^([0-9A-F]{64})  ([0-9]+)  (.+)$') {
            throw "Invalid checksum manifest line: $line"
        }
        $expectedHash = $Matches[1]
        $expectedLength = [int64]$Matches[2]
        $relativePath = $Matches[3].Replace('/', '\')
        $filePath = Join-Path $stage $relativePath
        $file = Get-Item -LiteralPath $filePath
        if ($file.Length -ne $expectedLength) {
            throw "Checksum length mismatch: $relativePath"
        }
        if ((Get-FileHash -LiteralPath $filePath -Algorithm SHA256).Hash -ne $expectedHash) {
            throw "Checksum mismatch: $relativePath"
        }
    }

    [PSCustomObject]@{
        StageRoot = $stage
        FileCount = $checksumLines.Count
        ChecksumPath = $checksumPath
    }
}

Export-ModuleMember -Function @(
    'Read-WirelessSourceManifest',
    'Get-DebugCompilerConfiguration',
    'Get-DebugProjectInputs',
    'ConvertFrom-GccDependencyOutput',
    'New-GccDependencyBatches',
    'Get-GccProjectDependencies',
    'New-WirelessCompilePlan',
    'Invoke-CheckedProcess',
    'Invoke-WirelessArchiveBuild',
    'New-ArchiveArguments',
    'Convert-PathBetweenRoots',
    'New-MinimalWirelessCustomerStage',
    'New-WirelessCustomerStage'
)

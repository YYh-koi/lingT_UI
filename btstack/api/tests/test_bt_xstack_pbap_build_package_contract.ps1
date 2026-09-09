$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$commonMakefilePath = Join-Path $repoRoot 'btstack\Makefile.inc'
$classicMakefilePath = Join-Path $repoRoot 'btstack\src\classic\Makefile.inc'
$projectPath = Join-Path $repoRoot 'Project\WirelessLibraries\BTstack_API_Library\BTstack_API_Library.gpcmprj'
$generatedMakefilePath = Join-Path $repoRoot 'Project\WirelessLibraries\BTstack_API_Library\BTstack_API_Library_Debug.mk'
$buildScriptPath = Join-Path $repoRoot 'Project\WirelessLibraries\BTstack_API_Library\build_btstack_api.ps1'
$readmePath = Join-Path $repoRoot 'btstack\README_API.md'
$packagePath = Join-Path $repoRoot 'gplib\liba_wifi_src\atbm\atbm6165\bt_api'

function Assert-Contains([string]$text, [string]$literal, [string]$message) {
    if (-not $text.Contains($literal)) { throw $message }
}

function Assert-LiteralCount(
    [string]$text,
    [string]$literal,
    [int]$expected,
    [string]$message) {
    $actual = ([regex]::Matches($text, [regex]::Escape($literal))).Count
    if ($actual -ne $expected) {
        throw "$message (expected=$expected actual=$actual)"
    }
}

function Assert-ParagraphContainsTokens(
    [string]$text,
    [string[]]$tokens,
    [string]$message) {
    $matchingParagraphs = @([regex]::Split($text, '(?:\r?\n){2,}') |
        Where-Object {
            $paragraph = $_
            @($tokens | Where-Object { -not $paragraph.Contains($_) }).Count -eq 0
        })
    if ($matchingParagraphs.Count -eq 0) {
        throw "$message (tokens=$($tokens -join ', '))"
    }
}

$commonMakefile = Get-Content -Raw -LiteralPath $commonMakefilePath
$classicMakefile = Get-Content -Raw -LiteralPath $classicMakefilePath
$buildScript = Get-Content -Raw -LiteralPath $buildScriptPath
$readme = Get-Content -Raw -Encoding utf8 -LiteralPath $readmePath
$generatedMakefile = Get-Content -Raw -LiteralPath $generatedMakefilePath
[xml]$project = Get-Content -Raw -LiteralPath $projectPath

foreach ($include in @(
        '$(BTSTACK_ROOT)/3rd-party/md5',
        '$(BTSTACK_ROOT)/3rd-party/yxml')) {
    Assert-Contains $commonMakefile $include "Common Makefile is missing include: $include"
}

foreach ($source in @(
        '$(BTSTACK_ROOT)/profiles/btstack_pbap_client.c',
        '$(BTSTACK_ROOT)/profiles/btstack_vcard_parser.c',
        '$(BTSTACK_ROOT)/src/classic/pbap_client.c',
        '$(BTSTACK_ROOT)/src/classic/goep_client.c',
        '$(BTSTACK_ROOT)/src/classic/obex_message_builder.c',
        '$(BTSTACK_ROOT)/src/classic/obex_parser.c',
        '$(BTSTACK_ROOT)/src/classic/obex_iterator.c',
        '$(BTSTACK_ROOT)/src/classic/obex_srm_client.c',
        '$(BTSTACK_ROOT)/3rd-party/md5/md5.c',
        '$(BTSTACK_ROOT)/3rd-party/yxml/yxml.c')) {
    Assert-LiteralCount $commonMakefile $source 1 `
        "Common Makefile must contain source exactly once: $source"
}

foreach ($source in @(
        'goep_client.c',
        'obex_message_builder.c',
        'obex_parser.c',
        'obex_iterator.c',
        'obex_srm_client.c',
        'pbap_client.c')) {
    Assert-LiteralCount $classicMakefile $source 1 `
        "Classic Makefile must contain source exactly once: $source"
}

$projectDirectory = Split-Path -Parent $projectPath
$projectIncludes = @($project.emIDE_project_file.Project.Build.Target.Compiler.Add |
    Where-Object { $_.HasAttribute('directory') } |
    ForEach-Object { [IO.Path]::GetFullPath((Join-Path $projectDirectory ([string]$_.directory))) })
$projectSources = @($project.emIDE_project_file.Project.Unit |
    ForEach-Object { [IO.Path]::GetFullPath((Join-Path $projectDirectory ([string]$_.filename))) })
$duplicateProjectSources = @($projectSources | Group-Object |
    Where-Object { $_.Count -ne 1 })
if ($duplicateProjectSources.Count -ne 0) {
    throw "BTstack API project contains duplicate source unit: $($duplicateProjectSources[0].Name)"
}
if ($projectSources.Count -lt 1) { throw 'BTstack API project has no source units' }

foreach ($relative in @(
        'btstack\3rd-party\md5',
        'btstack\3rd-party\yxml')) {
    $expected = [IO.Path]::GetFullPath((Join-Path $repoRoot $relative))
    if ($projectIncludes -notcontains $expected) {
        throw "BTstack API project is missing include directory: $relative"
    }
}

$requiredProjectSources = @(
    'btstack\core\btstack_platform.c',
    'btstack\core\btstack_ring.c',
    'btstack\core\btstack_call_audio_tx.c',
    'btstack\ports\gpcv2178\btstack_platform_gpcv2178.c',
    'btstack\ports\gpcv2178\btstack_rtos_gpcv2178.c',
    'btstack\profiles\btstack_pbap_client.c',
    'btstack\profiles\btstack_vcard_parser.c',
    'btstack\src\classic\pbap_client.c',
    'btstack\src\classic\goep_client.c',
    'btstack\src\classic\obex_message_builder.c',
    'btstack\src\classic\obex_parser.c',
    'btstack\src\classic\obex_iterator.c',
    'btstack\src\classic\obex_srm_client.c',
    'btstack\3rd-party\md5\md5.c',
    'btstack\3rd-party\yxml\yxml.c'
)
$generatedLines = @($generatedMakefile -split "\r?\n")
$allObjectRules = @($generatedLines | Where-Object {
    $_.StartsWith('$(OBJDIR_DEBUG)\\') -and
    $_.Contains(': ..\\..\\..\\') -and
    $_.EndsWith('.c')
})
if ($allObjectRules.Count -ne $projectSources.Count) {
    throw "Generated Makefile must contain one object rule per project source (expected=$($projectSources.Count) actual=$($allObjectRules.Count))"
}
$allCompileRules = @($generatedLines | Where-Object {
    $_.Contains(' -c ..\\..\\..\\') -and
    $_.Contains(' -o $(OBJDIR_DEBUG)\\')
})
if ($allCompileRules.Count -ne $projectSources.Count) {
    throw "Generated Makefile must contain one compile rule per project source (expected=$($projectSources.Count) actual=$($allCompileRules.Count))"
}
foreach ($relative in $requiredProjectSources) {
    $expected = [IO.Path]::GetFullPath((Join-Path $repoRoot $relative))
    $projectSourceCount = @($projectSources | Where-Object { $_ -eq $expected }).Count
    if ($projectSourceCount -ne 1) {
        throw "BTstack API project must contain source exactly once: $relative (actual=$projectSourceCount)"
    }
    $makeToken = $relative.Replace('\', '\\')
    $objectToken = ([IO.Path]::ChangeExtension($relative, '.o')).Replace('\', '\\')
    $objectRules = @($generatedLines | Where-Object {
        $_.StartsWith('$(OBJDIR_DEBUG)\\') -and
        $_.Contains("$objectToken`: ..\\..\\..\\") -and
        $_.EndsWith($makeToken)
    })
    if ($objectRules.Count -ne 1) {
        throw "Generated Makefile must contain one object rule for: $relative (actual=$($objectRules.Count))"
    }
    $compileRules = @($generatedLines | Where-Object {
        $_.Contains(" -c ..\\..\\..\\$makeToken -o `$(OBJDIR_DEBUG)\\$objectToken")
    })
    if ($compileRules.Count -ne 1) {
        throw "Generated Makefile must contain one compile rule for: $relative (actual=$($compileRules.Count))"
    }
}

$projectObjects = @()
foreach ($sourcePath in $projectSources) {
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        throw "BTstack API project source is missing: $sourcePath"
    }
    $repoPrefix = $repoRoot.TrimEnd('\') + '\'
    if (-not $sourcePath.StartsWith(
            $repoPrefix,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "BTstack API project source is outside repository: $sourcePath"
    }
    $relative = $sourcePath.Substring($repoPrefix.Length)
    $projectSourceCount = @($projectSources |
        Where-Object { $_ -eq $sourcePath }).Count
    if ($projectSourceCount -ne 1) {
        throw "BTstack API project source must occur exactly once: $relative (actual=$projectSourceCount)"
    }

    $makeToken = $relative.Replace('\', '\\')
    $objectRelative = [IO.Path]::ChangeExtension($relative, '.o')
    $objectToken = $objectRelative.Replace('\', '\\')
    $objectRules = @($generatedLines | Where-Object {
        $_.StartsWith('$(OBJDIR_DEBUG)\\') -and
        $_.Contains("$objectToken`: ..\\..\\..\\") -and
        $_.EndsWith($makeToken)
    })
    if ($objectRules.Count -ne 1) {
        throw "Generated Makefile must contain one object rule for project source: $relative (actual=$($objectRules.Count))"
    }
    $compileRules = @($generatedLines | Where-Object {
        $_.Contains(" -c ..\\..\\..\\$makeToken -o `$(OBJDIR_DEBUG)\\$objectToken")
    })
    if ($compileRules.Count -ne 1) {
        throw "Generated Makefile must contain one compile rule for project source: $relative (actual=$($compileRules.Count))"
    }

    $objectPath = Join-Path $projectDirectory (Join-Path 'obj\Debug' $objectRelative)
    if (-not (Test-Path -LiteralPath $objectPath -PathType Leaf)) {
        throw "BTstack API project object is missing: $objectRelative"
    }
    if ((Get-Item -LiteralPath $objectPath).LastWriteTimeUtc -lt
        (Get-Item -LiteralPath $sourcePath).LastWriteTimeUtc) {
        throw "BTstack API project object is stale: $objectRelative"
    }
    $projectObjects += $objectPath
}

Assert-Contains $generatedMakefile 'btstack\3rd-party\md5' 'Generated Makefile is missing MD5 include directory'
Assert-Contains $generatedMakefile 'btstack\3rd-party\yxml' 'Generated Makefile is missing yxml include directory'
Assert-Contains $buildScript "'bt_xstack_phonebook.h'" 'Build script does not stage the PBAP public header'
Assert-Contains $buildScript "'bt_xstack_audio.h'" 'Build script does not stage the audio public header'

$publicHeader = Join-Path $repoRoot 'btstack\include\api\bt_xstack_phonebook.h'
$packagedHeader = Join-Path $packagePath 'bt_xstack_phonebook.h'
if (-not (Test-Path -LiteralPath $packagedHeader -PathType Leaf)) {
    throw 'PBAP public header is missing from the package'
}
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $publicHeader).Hash -ne
    (Get-FileHash -Algorithm SHA256 -LiteralPath $packagedHeader).Hash) {
    throw 'Packaged PBAP public header is stale'
}

$publicHeaderText = Get-Content -Raw -LiteralPath $publicHeader
$packagedHeaderText = Get-Content -Raw -LiteralPath $packagedHeader
foreach ($headerText in @($publicHeaderText, $packagedHeaderText)) {
    foreach ($publicToken in @(
            'bt_xstack_phonebook_sync',
            'bt_xstack_phonebook_cancel',
            'bt_xstack_phonebook_authenticate',
            'bt_xstack_phonebook_get_state',
            'BT_XSTACK_PHONEBOOK_CONTACTS',
            'BT_XSTACK_PHONEBOOK_INCOMING',
            'BT_XSTACK_PHONEBOOK_OUTGOING',
            'BT_XSTACK_PHONEBOOK_MISSED',
            'BT_XSTACK_PHONEBOOK_ALL')) {
        Assert-Contains $headerText $publicToken "Public phonebook header is missing: $publicToken"
    }
    foreach ($oldPublicToken in @(
            'bt_xstack_pbap_',
            'bt_xstack_get_pbap_state',
            'bt_xstack_pbap_state_t',
            'BT_XSTACK_PBAP_')) {
        if ($headerText.Contains($oldPublicToken)) {
            throw "Public phonebook header still contains old PBAP name: $oldPublicToken"
        }
    }
}

$publicAudioHeader = Join-Path $repoRoot 'btstack\include\api\bt_xstack_audio.h'
$packagedAudioHeader = Join-Path $packagePath 'bt_xstack_audio.h'
if (-not (Test-Path -LiteralPath $packagedAudioHeader -PathType Leaf)) {
    throw 'Audio public header is missing from the package'
}
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $publicAudioHeader).Hash -ne
    (Get-FileHash -Algorithm SHA256 -LiteralPath $packagedAudioHeader).Hash) {
    throw 'Packaged audio public header is stale'
}
$audioHeaderText = Get-Content -Raw -LiteralPath $packagedAudioHeader
foreach ($token in @('bt_xstack_call_audio_write', 'uint32_t length')) {
    Assert-Contains $audioHeaderText $token "Packaged audio header is missing ABI token: $token"
}

$library = Join-Path $packagePath 'libxstack_api.a'
if (-not (Test-Path -LiteralPath $library -PathType Leaf)) {
    throw 'Packaged archive is missing: libxstack_api.a'
}
$libraryTimestamp = (Get-Item -LiteralPath $library).LastWriteTimeUtc
foreach ($archiveInput in @($projectSources) + @($projectObjects)) {
    if ($libraryTimestamp -lt
        (Get-Item -LiteralPath $archiveInput).LastWriteTimeUtc) {
        throw "Packaged archive is stale relative to: $archiveInput"
    }
}
$nm = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-nm.exe'
if (-not (Test-Path -LiteralPath $nm -PathType Leaf)) {
    throw "Required ARM nm tool is missing: $nm"
}
$symbols = (& $nm -g --defined-only $library) -join "`n"
foreach ($symbol in @(
        'bt_xstack_call_audio_write',
        'bt_xstack_phonebook_sync',
        'bt_xstack_phonebook_cancel',
        'bt_xstack_phonebook_authenticate',
        'bt_xstack_phonebook_get_state',
        'btstack_pbap_client_sync',
        'pbap_client_connect',
        'goep_client_connect',
        'obex_message_builder_request_create_connect',
        'obex_parser_init_for_response',
        'MD5_Init',
        'yxml_init')) {
    if ($symbols -notmatch ("\b" + $symbol + "\b")) {
        throw "Packaged archive is missing PBAP symbol: $symbol"
    }
}

$ar = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-ar.exe'
if (-not (Test-Path -LiteralPath $ar -PathType Leaf)) {
    throw "Required ARM ar tool is missing: $ar"
}
$archiveMembers = @(& $ar t $library)
foreach ($member in @(
        'btstack_platform.o',
        'btstack_ring.o',
        'btstack_call_audio_tx.o',
        'btstack_platform_gpcv2178.o',
        'btstack_rtos_gpcv2178.o')) {
    $count = @($archiveMembers | Where-Object { $_ -eq $member }).Count
    if ($count -ne 1) {
        throw "Packaged archive must contain $member exactly once (actual=$count)"
    }
}
$defaultDefinitionCount = @(& $nm -g --defined-only $library |
    Where-Object { $_ -match '\b[TDRB]\s+btstack_platform_default$' }).Count
if ($defaultDefinitionCount -ne 1) {
    throw "Packaged archive must define btstack_platform_default exactly once (actual=$defaultDefinitionCount)"
}
foreach ($oldSymbol in @(
        'bt_xstack_pbap_sync',
        'bt_xstack_pbap_cancel',
        'bt_xstack_pbap_authenticate',
        'bt_xstack_get_pbap_state')) {
    if ($symbols -match ("\b" + $oldSymbol + "\b")) {
        throw "Packaged archive still exports old public symbol: $oldSymbol"
    }
}

foreach ($documentationToken in @(
        'bt_xstack_phonebook_sync',
        'bt_xstack_phonebook_cancel',
        'bt_xstack_phonebook_authenticate',
        'bt_xstack_phonebook_get_state',
        'BT_XSTACK_PHONEBOOK_CONTACTS',
        'BT_XSTACK_PHONEBOOK_INCOMING',
        'BT_XSTACK_PHONEBOOK_OUTGOING',
        'BT_XSTACK_PHONEBOOK_MISSED',
        'BT_XSTACK_PHONEBOOK_ALL',
        'phonebook_callback',
        'BT_XSTACK_PHONE_EVENT_DEVICE',
        'AUTH_REQUIRED',
        'CATEGORY_COMPLETED',
        'COMPLETED',
        'IDLE',
        'BTstack v1.6.2',
        'status-preservation',
        'BT_XSTACK_PHONEBOOK_STATUS_UNSUPPORTED',
        'BT_XSTACK_PHONEBOOK_STATUS_DENIED',
        'BT_XSTACK_PHONEBOOK_STATUS_PROTOCOL_ERROR',
        'HFP `LOST`',
        'BT_XSTACK_PHONEBOOK_NAME_MAX',
        'BT_XSTACK_PHONEBOOK_NUMBER_MAX',
        'BT_XSTACK_PHONEBOOK_NUMBER_COUNT',
        'BT_XSTACK_PHONEBOOK_TIMESTAMP_MAX',
        'truncated != 0')) {
    Assert-Contains $readme $documentationToken "README is missing PBAP documentation: $documentationToken"
}
foreach ($oldDocumentationToken in @(
        'bt_xstack_pbap_sync',
        'bt_xstack_pbap_cancel',
        'bt_xstack_pbap_authenticate',
        'bt_xstack_get_pbap_state')) {
    if ($readme.Contains($oldDocumentationToken)) {
        throw "README still documents old public PBAP API: $oldDocumentationToken"
    }
}

$phonebookSectionStart = $readme.IndexOf('## PBAP')
$phonebookSectionEnd = $readme.IndexOf('## ', $phonebookSectionStart + 3)
if (($phonebookSectionStart -lt 0) -or ($phonebookSectionEnd -lt 0)) {
    throw 'README PBAP section is missing or unterminated'
}
$phonebookSection = $readme.Substring(
    $phonebookSectionStart,
    $phonebookSectionEnd - $phonebookSectionStart)
Assert-ParagraphContainsTokens $phonebookSection @(
    'categories',
    'BT_XSTACK_PHONEBOOK_CONTACTS',
    'BT_XSTACK_PHONEBOOK_INCOMING',
    'BT_XSTACK_PHONEBOOK_OUTGOING',
    'BT_XSTACK_PHONEBOOK_MISSED',
    'BT_XSTACK_PHONEBOOK_ALL',
    'PBAP') 'README must relate the category mask to all selectable categories and the PBAP connection'
Assert-ParagraphContainsTokens $phonebookSection @(
    'event.category',
    'bt_xstack_phonebook_category_t',
    'BT_XSTACK_PHONEBOOK_CATEGORY_CONTACTS',
    'BT_XSTACK_PHONEBOOK_CATEGORY_MISSED',
    'event.category != BT_XSTACK_PHONEBOOK_ALL') `
    'README must define event.category as an actual singular category, never the ALL mask'
Assert-ParagraphContainsTokens $phonebookSection @(
    'bt_xstack_phonebook_sync()',
    'BT_XSTACK_OK',
    'BT_XSTACK_PHONEBOOK_EVENT_ERROR',
    'BT_XSTACK_PHONEBOOK_STATUS_BUSY') `
    'README must explain active-sync admission and the asynchronous BUSY event'
Assert-ParagraphContainsTokens $phonebookSection @(
    'API task',
    'BTstack run loop',
    'event task',
    'phonebook_callback') `
    'README must distinguish request processing from callback delivery tasks'
Assert-ParagraphContainsTokens $phonebookSection @(
    'BT_XSTACK_PHONEBOOK_STATUS_UNSUPPORTED',
    'CATEGORY_COMPLETED',
    'non-fatal',
    'continues later selected categories') `
    'README must explain that unsupported categories do not stop later selected categories'
Assert-ParagraphContainsTokens $phonebookSection @(
    'BT_XSTACK_PHONEBOOK_STATUS_DENIED',
    'BT_XSTACK_PHONEBOOK_STATUS_PROTOCOL_ERROR',
    'ERROR',
    'fatal',
    'do not continue') `
    'README must explain that denied and protocol errors are fatal to the current sync'
$forbiddenEventTaskPhrase =
    '\u4e8b\u4ef6\u4efb\u52a1\u5904\u7406\u8be5\u8bf7\u6c42\u540e'
if ($phonebookSection -match $forbiddenEventTaskPhrase) {
    throw 'README incorrectly assigns phonebook request processing to the event task'
}

$cExamples = @([regex]::Matches($readme, '(?s)```c\s*(.*?)\s*```') |
    ForEach-Object { $_.Groups[1].Value })
$deviceExample = @($cExamples | Where-Object {
    $_ -match 'BT_XSTACK_PHONE_EVENT_DEVICE'
})
if ($deviceExample.Count -ne 1) {
    throw 'README must contain one C example for the HFP DEVICE phone event'
}
$deviceBranchPattern = '(?s)if\s*\(\s*event->id\s*==\s*' +
    'BT_XSTACK_PHONE_EVENT_DEVICE\s*\)\s*\{.*?' +
    'admission_status\s*=\s*bt_xstack_phonebook_sync\s*\(\s*' +
    '&event->address\s*,\s*BT_XSTACK_PHONEBOOK_CONTACTS\s*\|\s*' +
    'BT_XSTACK_PHONEBOOK_MISSED\s*\)\s*;.*?' +
    'if\s*\(\s*admission_status\s*!=\s*BT_XSTACK_OK\s*\)'
if ($deviceExample[0] -notmatch $deviceBranchPattern) {
    throw 'README DEVICE example must submit the CONTACTS|MISSED mask and check queue admission status'
}
Assert-Contains $readme 'queue admission status' `
    'README must explain that the phonebook sync result is queue admission status'

$compiler = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-gcc.exe'
if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) {
    throw "Required ARM C compiler is missing: $compiler"
}
$exampleSource = [IO.Path]::GetTempFileName()
try {
    [IO.File]::WriteAllText(
        $exampleSource,
        $deviceExample[0],
        (New-Object Text.UTF8Encoding($false)))
    $savedErrorPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $compileOutput = @(& $compiler -std=c99 -Wall -Wextra -Werror `
            -fsyntax-only -x c "-I$($repoRoot)\btstack\include\api" `
            $exampleSource 2>&1)
        $compileExitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $savedErrorPreference
    }
    if ($compileExitCode -ne 0) {
        throw "README DEVICE C example does not compile independently:`n$($compileOutput -join "`n")"
    }
}
finally {
    Remove-Item -LiteralPath $exampleSource -Force -ErrorAction SilentlyContinue
}

Write-Output 'PASS: PBAP build and package integration contract'

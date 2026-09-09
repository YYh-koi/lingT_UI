$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$callPattern = '(?i)\b(?:taskENTER_CRITICAL\w*|taskEXIT_CRITICAL\w*|xQueue\w*|xTask\w*|gplib_\w*|drv_l[12]\w*)\s*\('

function Test-ForbiddenInclude([string]$preprocessorLine) {
    if ($preprocessorLine -notmatch '^\s*#\s*include\s*[<"](?<target>[^>"]+)[>"]') { return $false }
    $target = $Matches.target.Replace('\', '/').ToLowerInvariant()
    $segments = @($target -split '/')
    $baseName = $segments[$segments.Count - 1]
    return $baseName -in @('freertos.h', 'task.h', 'queue.h', 'semphr.h') -or
        $baseName.StartsWith('cmsis_os') -or $target -match 'gpcv2178' -or
        @($segments | Where-Object { $_ -like 'gplib_*' }).Count -gt 0
}

function Get-CLogicalLines([string[]]$lines) {
    $logicalLines = @()
    for ($index = 0; $index -lt $lines.Count; $index++) {
        $startLine = $index + 1
        $text = $lines[$index]
        $original = $lines[$index]
        while ($text -match '\\$' -and $index + 1 -lt $lines.Count) {
            $text = $text.Substring(0, $text.Length - 1) + $lines[$index + 1]
            $original += $lines[$index + 1]
            $index++
        }
        $logicalLines += [pscustomobject]@{ StartLine = $startLine; Text = $text; Original = $original }
    }
    return $logicalLines
}

function Get-BTstackPublicIncludeBlock([string]$makefileText) {
    $lines = @($makefileText -split "`r?`n")
    $block = @()
    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = $lines[$index] -replace '\s*#.*$', ''
        if ($line -match '^\s*BTSTACK_INCLUDE_DIRS\s*(?:\?|\+|:)?=') {
            for ($continuation = $index; $continuation -lt $lines.Count; $continuation++) {
                $block += [pscustomobject]@{ LineNumber = $continuation + 1; Text = ($lines[$continuation] -replace '\s*#.*$', '') }
                if ($lines[$continuation] -notmatch '\\\s*$') { break }
            }
        }
    }
    return @($block)
}

function Get-BTstackPublicIncludeViolations([object[]]$publicIncludeBlock) {
    $violations = @()
    foreach ($line in $publicIncludeBlock) {
        if ($line.Text -match '(?i)ports/gpcv2178') {
            $violations += "ports/gpcv2178 at line $($line.LineNumber)"
        }
        foreach ($match in [regex]::Matches($line.Text, '\$\(([^)]+)\)|\$\{([^}]+)\}')) {
            $variableName = if ($match.Groups[1].Success) { $match.Groups[1].Value } else { $match.Groups[2].Value }
            if ($variableName -ne 'BTSTACK_ROOT') {
                $violations += "unsupported variable `$($match.Value) at line $($line.LineNumber)"
            }
        }
    }
    return @($violations)
}

function ConvertTo-CCodeLine([string]$line, [ref]$insideBlockComment) {
    $code = New-Object Text.StringBuilder
    $preprocessor = New-Object Text.StringBuilder
    $inString = $false
    $inCharacter = $false
    $escaped = $false
    for ($index = 0; $index -lt $line.Length; $index++) {
        $character = $line[$index]
        $next = if ($index + 1 -lt $line.Length) { $line[$index + 1] } else { [char]0 }

        if ($insideBlockComment.Value) {
            [void]$code.Append(' ')
            [void]$preprocessor.Append(' ')
            if ($character -eq '*' -and $next -eq '/') {
                [void]$code.Append(' ')
                [void]$preprocessor.Append(' ')
                $insideBlockComment.Value = $false
                $index++
            }
            continue
        }
        if ($inString -or $inCharacter) {
            [void]$code.Append(' ')
            [void]$preprocessor.Append($character)
            if ($escaped) {
                $escaped = $false
            }
            elseif ($character -eq '\') {
                $escaped = $true
            }
            elseif (($inString -and $character -eq '"') -or
                ($inCharacter -and $character -eq "'")) {
                $inString = $false
                $inCharacter = $false
            }
            continue
        }
        if ($character -eq '/' -and $next -eq '/') {
            [void]$code.Append(' ')
            [void]$code.Append(' ')
            [void]$preprocessor.Append(' ')
            [void]$preprocessor.Append(' ')
            while ($index + 1 -lt $line.Length) {
                $index++
                [void]$code.Append(' ')
                [void]$preprocessor.Append(' ')
            }
            break
        }
        if ($character -eq '/' -and $next -eq '*') {
            [void]$code.Append(' ')
            [void]$code.Append(' ')
            [void]$preprocessor.Append(' ')
            [void]$preprocessor.Append(' ')
            $insideBlockComment.Value = $true
            $index++
            continue
        }
        if ($character -eq '"') {
            [void]$code.Append(' ')
            [void]$preprocessor.Append($character)
            $inString = $true
            continue
        }
        if ($character -eq "'") {
            [void]$code.Append(' ')
            [void]$preprocessor.Append($character)
            $inCharacter = $true
            continue
        }
        [void]$code.Append($character)
        [void]$preprocessor.Append($character)
    }
    return [pscustomobject]@{
        Code = $code.ToString()
        Preprocessor = $preprocessor.ToString()
    }
}

function Get-PlatformBoundaryViolations(
    [string]$root,
    [string[]]$relativeDirectories) {
    $violations = @()
    foreach ($relativeDirectory in $relativeDirectories) {
        $directory = Join-Path $root $relativeDirectory
        if (-not (Test-Path -LiteralPath $directory -PathType Container)) {
            throw "Production directory is missing: $relativeDirectory"
        }

        $files = Get-ChildItem -LiteralPath $directory -Recurse -File |
            Where-Object {
                $_.Extension -in '.c', '.h' -and
                $_.FullName -notmatch '(?i)[\\/]tests[\\/]'
            }
        foreach ($file in $files) {
            $lines = @(Get-Content -LiteralPath $file.FullName)
            $insideBlockComment = $false
            foreach ($logicalLine in @(Get-CLogicalLines $lines)) {
                $views = ConvertTo-CCodeLine $logicalLine.Text ([ref]$insideBlockComment)
                if ((Test-ForbiddenInclude $views.Preprocessor) -or $views.Code -match $callPattern) {
                    $rootPrefix = $root.TrimEnd('\', '/')
                    if (-not $file.FullName.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
                        throw "Scanned file is outside root: $($file.FullName)"
                    }
                    $relativePath = $file.FullName.Substring($rootPrefix.Length).TrimStart('\', '/').Replace('\', '/')
                    $violations += '{0}:{1}:{2}' -f $relativePath, $logicalLine.StartLine, $logicalLine.Original.Trim()
                }
            }
        }
    }
    return $violations
}

function Assert-PlatformBoundaryScannerSelfTest {
    $fixtureRoot = Join-Path ([IO.Path]::GetTempPath()) ('btstack-platform-boundary-' + [guid]::NewGuid())
    try {
        $sourceDirectory = Join-Path $fixtureRoot 'btstack\core'
        $testDirectory = Join-Path $sourceDirectory 'tests'
        New-Item -ItemType Directory -Force -Path $testDirectory | Out-Null
        Set-Content -LiteralPath (Join-Path $sourceDirectory 'forbidden_include.c') -Value @(
            '#include "FreeRTOS.h"',
            '#include "vendor/gplib_print_string.h"',
            '#include "freertos/FreeRTOS.h"',
            '#include "vendor/task.h"',
            '#include "any/queue.h"',
            '#include "any/semphr.h"',
            '#include "any/cmsis_os2.h"')
        Set-Content -LiteralPath (Join-Path $sourceDirectory 'forbidden_call.c') -Value @(
            'xQueue(queue, item, 0);',
            'xQueue_send(queue, item, 0);',
            'xTasklower();',
            'taskENTER_CRITICAL_FROM_ISR();',
            'taskEXIT_CRITICAL_custom();',
            'gplib_log();',
            'drv_l1_open();',
            '/* xQueueIgnored(); */',
            'const char *name = "xQueueString(";',
            'const char *url = "http://host"; xQueueAfterUrl();',
            'const char *marker = "/*"; xTaskAfterMarker();',
            'xQueueAfterMarkerNext();',
            "char slash = '/'; char quote = '\''; xQueueAfterChars();",
            '// xQueueLineComment();',
            '/* xTaskBlockComment(); */',
            '/* xQueueBlockStart();',
            ' xTaskBlockEnd(); */',
            'xQueueAfterBlock();')
        Set-Content -LiteralPath (Join-Path $sourceDirectory 'include_comment.c') -Value @(
            '/* comment starts',
            '#include "FreeRTOS.h"',
            '*/',
            '/* note */ #include "FreeRTOS.h"',
            'const char *s = "#include \"FreeRTOS.h\"";',
            '// #include "FreeRTOS.h"')
        Set-Content -LiteralPath (Join-Path $sourceDirectory 'spliced.c') -Value @(
            'xQueueSpliced\',
            '();',
            'const char *s = "xQueueString\',
            '(";')
        Set-Content -LiteralPath (Join-Path $testDirectory 'allowed_test.c') -Value '#include "task.h"`ntaskENTER_CRITICAL();'

        $violations = @(Get-PlatformBoundaryViolations $fixtureRoot @('btstack/core'))
        if ($violations.Count -ne 21) {
            throw "Scanner self-test expected 21 forbidden production hits, found $($violations.Count): $($violations -join '; ')"
        }
        foreach ($expectedHit in @(
                'forbidden_include\.c:1:#include "FreeRTOS\.h"',
                'forbidden_include\.c:2:#include "vendor/gplib_print_string\.h"',
                'forbidden_include\.c:3:#include "freertos/FreeRTOS\.h"',
                'forbidden_include\.c:4:#include "vendor/task\.h"',
                'forbidden_include\.c:5:#include "any/queue\.h"',
                'forbidden_include\.c:6:#include "any/semphr\.h"',
                'forbidden_include\.c:7:#include "any/cmsis_os2\.h"',
                'forbidden_call\.c:1:xQueue\(queue, item, 0\);',
                'forbidden_call\.c:2:xQueue_send\(queue, item, 0\);',
                'forbidden_call\.c:3:xTasklower\(\);',
                'forbidden_call\.c:4:taskENTER_CRITICAL_FROM_ISR\(\);',
                'forbidden_call\.c:5:taskEXIT_CRITICAL_custom\(\);',
                'forbidden_call\.c:6:gplib_log\(\);',
                'forbidden_call\.c:7:drv_l1_open\(\);',
                'forbidden_call\.c:10:.*xQueueAfterUrl\(\);',
                'forbidden_call\.c:11:.*xTaskAfterMarker\(\);',
                'forbidden_call\.c:12:xQueueAfterMarkerNext\(\);',
                'forbidden_call\.c:13:.*xQueueAfterChars\(\);',
                'forbidden_call\.c:18:xQueueAfterBlock\(\);',
                'include_comment\.c:4:/\* note \*/ #include "FreeRTOS\.h"',
                'spliced\.c:1:xQueueSpliced\\\(\);')) {
            if (-not ($violations -match $expectedHit)) {
                throw "Scanner self-test did not detect fixture: $expectedHit"
            }
        }
        if ($violations -match 'forbidden_call\.c:(8|9|14|15|16|17):' -or
            $violations -match 'include_comment\.c:(2|5|6):' -or
            $violations -match 'spliced\.c:3:') {
            throw 'Scanner self-test reported a call-like comment or literal'
        }
        if ($violations -match 'allowed_test\.c') {
            throw 'Scanner self-test scanned a file below a tests directory'
        }
    }
    finally {
        if (Test-Path -LiteralPath $fixtureRoot) {
            Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
        }
    }
}

function Assert-PublicIncludeBlockSelfTest {
    $fixtureMakefile = @'
BTSTACK_INCLUDE_DIRS := \
    $(BTSTACK_ROOT)/include \
    $(BTSTACK_ROOT)/ports/gpcv2178
BTSTACK_GPCV2178_INCLUDE_DIRS := $(BTSTACK_ROOT)/ports/gpcv2178
BTSTACK_INCLUDE_DIRS += \
    $(BTSTACK_ROOT)/ports/gpcv2178
BTSTACK_INCLUDE_DIRS += $(BTSTACK_ROOT)/include # $(BTSTACK_ROOT)/ports/gpcv2178
BTSTACK_GPCV2178_INCLUDE_DIRS := $(BTSTACK_ROOT)/ports/gpcv2178
BTSTACK_INCLUDE_DIRS := $(BTSTACK_GPCV2178_INCLUDE_DIRS)
BTSTACK_INCLUDE_DIRS += ${BTSTACK_GPCV2178_INCLUDE_DIRS}
'@
    $publicBlock = Get-BTstackPublicIncludeBlock $fixtureMakefile
    if ($publicBlock.Count -ne 8 -or @($publicBlock | Where-Object { $_.Text -match 'ports/gpcv2178' }).Count -ne 2) {
        throw 'Public include block self-test did not retain the common continued port include'
    }
    if ($publicBlock.Text -match '^\s*BTSTACK_GPCV2178_INCLUDE_DIRS\s*=') {
        throw 'Public include block self-test included an independent port block'
    }
    if ($publicBlock.Text -match '^\s*#.*ports/gpcv2178') {
        throw 'Public include block self-test retained a Make comment'
    }
    $indirectViolations = @(Get-BTstackPublicIncludeViolations $publicBlock | Where-Object {
            $_ -match 'BTSTACK_GPCV2178_INCLUDE_DIRS'
        })
    if ($indirectViolations.Count -ne 2 -or
        -not ($indirectViolations -match '\$\(BTSTACK_GPCV2178_INCLUDE_DIRS\)') -or
        -not ($indirectViolations -match '\$\{BTSTACK_GPCV2178_INCLUDE_DIRS\}')) {
        throw 'Public include block self-test did not reject both Make variable-reference forms'
    }
}

Assert-PlatformBoundaryScannerSelfTest
Assert-PublicIncludeBlockSelfTest

$violations = @(Get-PlatformBoundaryViolations $repoRoot @(
        'btstack/api',
        'btstack/core',
        'btstack/gap',
        'btstack/profiles',
        'btstack/transport'))

$makefilePath = Join-Path $repoRoot 'btstack\Makefile.inc'
$makefile = Get-Content -Raw -LiteralPath $makefilePath
$publicIncludeBlock = @(Get-BTstackPublicIncludeBlock $makefile)
if ($publicIncludeBlock.Count -eq 0) {
    throw 'BTSTACK_INCLUDE_DIRS public include block is missing'
}
foreach ($publicViolation in @(Get-BTstackPublicIncludeViolations $publicIncludeBlock)) {
    $violations += "btstack/Makefile.inc:BTSTACK_INCLUDE_DIRS:$publicViolation"
}
foreach ($violation in $violations) {
    Write-Output $violation
}

if ($violations.Count -gt 0) {
    throw "BTstack platform boundary violations: $($violations.Count)"
}

Write-Output 'PASS: BTstack common code is free of RTOS and GPCV2178/vendor dependencies'

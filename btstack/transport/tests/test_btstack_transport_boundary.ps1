param()

$ErrorActionPreference = 'Stop'

$testsDir = $PSScriptRoot
$transportDir = Split-Path -Parent $testsDir
$forbiddenHeader = '(?i)(?:^|/)(?:FreeRTOS\.h|task\.h|queue\.h|semphr\.h|cmsis_os2\.h|gplib[^/]*\.h|drv_l1[^/]*\.h)$'
$forbiddenPreprocessorToken = '(?i)(?:FreeRTOS(?:\.h)?|cmsis_os2(?:\.h)?|gplib[A-Za-z0-9_./\\-]*|drv_l1[A-Za-z0-9_./\\-]*|(?:^|[/<"''])(?:task|queue|semphr)\.h)'
$concreteInstance = '\bbtstack_uart_block_(?:windows|embedded|freertos)_instance\s*\('

function Get-CLogicalLines {
    param([string[]] $Lines)
    $result = @()
    $text = ''
    $original = ''
    $startLine = 1
    for ($index = 0; $index -lt $Lines.Count; $index++) {
        $line = [string] $Lines[$index]
        if ($text.Length -eq 0) { $startLine = $index + 1 }
        $original += $line
        if ($line -match '\\\s*$') {
            $text += $line -replace '\\\s*$', ''
            continue
        }
        $text += $line
        $result += [pscustomobject]@{
            StartLine = $startLine
            Text = $text
            Original = $original
        }
        $text = ''
        $original = ''
    }
    if ($text.Length -ne 0) {
        $result += [pscustomobject]@{
            StartLine = $startLine
            Text = $text
            Original = $original
        }
    }
    return $result
}

function ConvertTo-CViews {
    param([string] $Line, [ref] $InsideBlockComment)
    $code = New-Object System.Text.StringBuilder
    $preprocessor = New-Object System.Text.StringBuilder
    $index = 0
    while ($index -lt $Line.Length) {
        if ($InsideBlockComment.Value) {
            if (($index + 1 -lt $Line.Length) -and
                $Line[$index] -eq '*' -and $Line[$index + 1] -eq '/') {
                [void] $code.Append('  ')
                [void] $preprocessor.Append('  ')
                $InsideBlockComment.Value = $false
                $index += 2
            } else {
                [void] $code.Append(' ')
                [void] $preprocessor.Append(' ')
                $index++
            }
            continue
        }
        if (($index + 1 -lt $Line.Length) -and
            $Line[$index] -eq '/' -and $Line[$index + 1] -eq '/') {
            [void] $code.Append(' ' * ($Line.Length - $index))
            [void] $preprocessor.Append(' ' * ($Line.Length - $index))
            break
        }
        if (($index + 1 -lt $Line.Length) -and
            $Line[$index] -eq '/' -and $Line[$index + 1] -eq '*') {
            [void] $code.Append('  ')
            [void] $preprocessor.Append('  ')
            $InsideBlockComment.Value = $true
            $index += 2
            continue
        }
        if (($Line[$index] -eq '"') -or ($Line[$index] -eq "'")) {
            $quote = $Line[$index]
            [void] $code.Append(' ')
            [void] $preprocessor.Append($quote)
            $index++
            while ($index -lt $Line.Length) {
                $current = $Line[$index]
                [void] $code.Append(' ')
                [void] $preprocessor.Append($current)
                $index++
                if (($current -eq '\') -and ($index -lt $Line.Length)) {
                    [void] $code.Append(' ')
                    [void] $preprocessor.Append($Line[$index])
                    $index++
                    continue
                }
                if ($current -eq $quote) { break }
            }
            continue
        }
        [void] $code.Append($Line[$index])
        [void] $preprocessor.Append($Line[$index])
        $index++
    }
    return [pscustomobject]@{
        Code = $code.ToString()
        Preprocessor = $preprocessor.ToString()
    }
}

function Get-TransportBoundaryViolations {
    param(
        [Parameter(Mandatory = $true)] [string] $Directory,
        [Parameter(Mandatory = $true)] [string] $MakefilePath
    )

    $violations = @()
    foreach ($file in Get-ChildItem -LiteralPath $Directory -File | Where-Object {
            $_.Extension -in '.c', '.h'
        }) {
        $insideBlockComment = $false
        $lines = @(Get-Content -LiteralPath $file.FullName)
        foreach ($logicalLine in @(Get-CLogicalLines $lines)) {
            $views = ConvertTo-CViews $logicalLine.Text ([ref] $insideBlockComment)
            $reason = $null
            if ($views.Code -match $concreteInstance) {
                $reason = 'concrete UART instance'
            }
            $trimmed = $views.Preprocessor.Trim()
            if ($trimmed -match '^#\s*include\b(.*)$') {
                $operand = $Matches[1].Trim()
                if ($operand -match '^[<"]([^>"]+)[>"]\s*$') {
                    $target = $Matches[1].Replace('\', '/')
                    if ($target -match $forbiddenHeader) {
                        $reason = 'vendor/RTOS include'
                    }
                } else {
                    $reason = 'non-literal include'
                }
            } elseif (($trimmed -match '^#') -and
                      ($trimmed -match $forbiddenPreprocessorToken)) {
                $reason = 'vendor/RTOS preprocessor token'
            }
            if ($reason) {
                $violations += '{0}:{1}:{2}:{3}' -f $file.Name,
                    $logicalLine.StartLine, $reason, $logicalLine.Original.Trim()
            }
        }
    }

    $makeLineNumber = 0
    foreach ($line in Get-Content -LiteralPath $MakefilePath) {
        $makeLineNumber++
        $active = $line -replace '#.*$', ''
        if ($active -match '(?i)(?:^|[/\\])(?:ports?|platforms?)(?:[/\\]|$)|gpcv2178|freertos|windows') {
            $violations += '{0}:{1}:platform Make entry:{2}' -f `
                ([IO.Path]::GetFileName($MakefilePath)), $makeLineNumber, $line.Trim()
        }
    }
    return $violations
}

function Assert-ScannerSelfTest {
    $fixture = Join-Path ([IO.Path]::GetTempPath()) `
        ('btstack-transport-boundary-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $fixture | Out-Null
    try {
        Set-Content -LiteralPath (Join-Path $fixture 'bad.c') -Encoding Ascii -Value @(
            '#include/**/"vendor/gplib_print_string.h"',
            '#include "freertos/\',
            'FreeRTOS.h"',
            '#define PLATFORM_UART_HEADER "cmsis_os2.h"',
            '#include PLATFORM_UART_HEADER',
            'btstack_uart_block_free\',
            'rtos_instance();',
            '/* #include "task.h"',
            'btstack_uart_block_windows_instance(); */',
            'const char *s = "btstack_uart_block_embedded_instance()";',
            "char c = '('; // #include <queue.h>")
        Set-Content -LiteralPath (Join-Path $fixture 'Makefile.inc') -Encoding Ascii -Value @(
            'TRANSPORT_SOURCES := common.c',
            'PORT_SOURCE := ports/gpcv2178/uart.c',
            '# WINDOWS_SOURCE := windows/uart.c')
        $hits = @(Get-TransportBoundaryViolations $fixture (Join-Path $fixture 'Makefile.inc'))
        if ($hits.Count -ne 6) {
            throw "Transport boundary scanner self-test expected 6 hits, found $($hits.Count): $($hits -join '; ')"
        }
        foreach ($expected in @(
                'bad\.c:1:vendor/RTOS include',
                'bad\.c:2:vendor/RTOS include',
                'bad\.c:4:vendor/RTOS preprocessor token',
                'bad\.c:5:non-literal include',
                'bad\.c:6:concrete UART instance',
                'Makefile\.inc:2:platform Make entry')) {
            if (-not ($hits -match $expected)) {
                throw "Transport boundary scanner missed fixture: $expected"
            }
        }
        if ($hits -match 'bad\.c:(8|9|10|11):' -or $hits -match 'WINDOWS_SOURCE') {
            throw 'Transport boundary scanner reported commented code or a literal'
        }
    }
    finally {
        Remove-Item -LiteralPath $fixture -Recurse -Force
    }
}

Assert-ScannerSelfTest
$violations = @(Get-TransportBoundaryViolations $transportDir `
    (Join-Path $transportDir 'Makefile.inc'))
$violations | ForEach-Object { Write-Output $_ }
if ($violations.Count -ne 0) {
    throw "BTstack transport boundary violations: $($violations.Count)"
}

Write-Output 'PASS: BTstack transport is free of platform and RTOS dependencies'

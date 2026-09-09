param(
    [string]$WorkspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
)

$ErrorActionPreference = 'Stop'
$failures = New-Object System.Collections.Generic.List[string]

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        $script:failures.Add($Message)
    }
}

$configPath = Join-Path $WorkspaceRoot 'Project\Include\FreeRTOSConfig.h'
$threadHeaderPath = Join-Path $WorkspaceRoot `
    'gplib\liba_wifi_src\atbm\atbm6165\os\freertos\include\atbm_os_thread.h'
$irqSourcePath = Join-Path $WorkspaceRoot `
    'gplib\liba_wifi_src\atbm\atbm6165\platform\generalplus\sdio\source\atbm_gp_sdio_card.c'

$configText = Get-Content -Raw -LiteralPath $configPath
$threadHeaderText = Get-Content -Raw -LiteralPath $threadHeaderPath
$irqSourceText = Get-Content -Raw -LiteralPath $irqSourcePath

Assert-True -Condition ($configText -match 'configMAX_PRIORITIES\s+\(\s*7\s*\)') `
    -Message 'GP FreeRTOS priority count must remain 7'
Assert-True -Condition ($irqSourceText -match 'atbm_gp_sdio_irq_thread.*osPriorityRealtime') `
    -Message 'GP SDIO IRQ thread must remain at CMSIS realtime priority (FreeRTOS 6)'

$gpBlockMatch = [regex]::Match(
    $threadHeaderText,
    '#if\s+ATBM_GENERALPLUS_SDIO(?<body>.*?)#else',
    [Text.RegularExpressions.RegexOptions]::Singleline)
Assert-True -Condition $gpBlockMatch.Success `
    -Message 'ATBM thread priorities are missing a Generalplus-specific mapping'

$expected = [ordered]@{
    WORK_TASK_PRIO = 1
    ELOOP_TASK_PRIO = 2
    HIF_TASK_PRIO = 3
    BH_TASK_PRIO = 4
    TX_BH_TASK_PRIO = 5
}

if ($gpBlockMatch.Success) {
    $values = @()
    foreach ($entry in $expected.GetEnumerator()) {
        $match = [regex]::Match(
            $gpBlockMatch.Groups['body'].Value,
            "#define\s+$($entry.Key)\s+(?<value>\d+)")
        Assert-True -Condition $match.Success `
            -Message "GP mapping is missing $($entry.Key)"
        if ($match.Success) {
            $value = [int]$match.Groups['value'].Value
            $values += $value
            Assert-True -Condition ($value -eq $entry.Value) `
                -Message "GP $($entry.Key) is $value, expected $($entry.Value)"
            Assert-True -Condition ($value -lt 7) `
                -Message "GP $($entry.Key) exceeds configMAX_PRIORITIES"
        }
    }
    Assert-True -Condition (@($values | Sort-Object -Unique).Count -eq $expected.Count) `
        -Message 'GP ATBM task priorities must remain distinct'
}

Assert-True -Condition ($threadHeaderText -match '#define\s+RX_BH_TASK_PRIO\s+BH_TASK_PRIO') `
    -Message 'RX must continue to share the BH priority'

if ($failures.Count -ne 0) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Host 'ATBM Generalplus priority mapping checks passed.'

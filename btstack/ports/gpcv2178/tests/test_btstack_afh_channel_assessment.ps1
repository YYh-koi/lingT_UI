$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$configPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_gpcv2178_config.h'
$cmdHeaderPath = Join-Path $root 'btstack/src/hci_cmd.h'
$cmdSourcePath = Join-Path $root 'btstack/src/hci_cmd.c'
$hciHeaderPath = Join-Path $root 'btstack/src/hci.h'
$hciSourcePath = Join-Path $root 'btstack/src/hci.c'
$portSourcePath = Join-Path $root 'btstack/ports/gpcv2178/btstack_platform_gpcv2178.c'

$config = Get-Content -Raw -LiteralPath $configPath
$cmdHeader = Get-Content -Raw -LiteralPath $cmdHeaderPath
$cmdSource = Get-Content -Raw -LiteralPath $cmdSourcePath
$hciHeader = Get-Content -Raw -LiteralPath $hciHeaderPath
$hciSource = Get-Content -Raw -LiteralPath $hciSourcePath
$portSource = Get-Content -Raw -LiteralPath $portSourcePath

$checks = @(
    @{
        Name = 'GPCV2178 enables AFH assessment'
        Pass = $config -match '#define\s+ENABLE_AFH_CHANNEL_ASSESSMENT'
    },
    @{
        Name = 'AFH opcode is 0x0C49'
        Pass = $cmdHeader -match 'HCI_OPCODE_HCI_WRITE_AFH_CHANNEL_ASSESSMENT_MODE\s*=\s*HCI_OPCODE\s*\(\s*OGF_CONTROLLER_BASEBAND\s*,\s*0x49\s*\)'
    },
    @{
        Name = 'AFH command is public'
        Pass = $cmdHeader -match 'extern\s+const\s+hci_cmd_t\s+hci_write_afh_channel_assessment_mode\s*;'
    },
    @{
        Name = 'AFH command takes one byte'
        Pass = $cmdSource -match 'hci_write_afh_channel_assessment_mode\s*=\s*\{[\s\S]*?HCI_OPCODE_HCI_WRITE_AFH_CHANNEL_ASSESSMENT_MODE\s*,\s*"1"'
    },
    @{
        Name = 'AFH initialization waits for completion'
        Pass = $hciHeader -match 'HCI_INIT_WRITE_AFH_CHANNEL_ASSESSMENT_MODE\s*,\s*\r?\n\s*HCI_INIT_W4_WRITE_AFH_CHANNEL_ASSESSMENT_MODE'
    },
    @{
        Name = 'AFH initialization sends enable once'
        Pass = $hciSource -match 'case\s+HCI_INIT_WRITE_AFH_CHANNEL_ASSESSMENT_MODE\s*:[\s\S]*?substate\s*=\s*HCI_INIT_W4_WRITE_AFH_CHANNEL_ASSESSMENT_MODE\s*;[\s\S]*?hci_send_cmd\s*\(\s*&hci_write_afh_channel_assessment_mode\s*,\s*1\s*\)'
    },
    @{
        Name = 'GPCV2178 event handler prints AFH completion status'
        Pass = $portSource -match 'HCI_EVENT_COMMAND_COMPLETE[\s\S]*?HCI_OPCODE_HCI_WRITE_AFH_CHANNEL_ASSESSMENT_MODE[\s\S]*?btstack: AFH channel assessment status=0x%02X'
    },
    @{
        Name = 'generic HCI core has no platform print dependency'
        Pass = $hciSource -notmatch 'BTSTACK_PRINTF\s*\(\s*"btstack: AFH channel assessment status='
    }
)

$failed = @($checks | Where-Object { -not $_.Pass })
if ($failed.Count -ne 0) {
    foreach ($check in $failed) {
        Write-Host "Missing: $($check.Name)"
    }
    exit 1
}

Write-Host 'GPCV2178 initializes AFH channel assessment serially'

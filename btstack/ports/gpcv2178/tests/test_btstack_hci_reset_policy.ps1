$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$configPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_gpcv2178_config.h'
$hciPath = Join-Path $root 'btstack/src/hci.c'
$h5Path = Join-Path $root 'btstack/transport/hci_transport_h5.c'

$config = Get-Content -Raw -LiteralPath $configPath
$hci = Get-Content -Raw -LiteralPath $hciPath
$h5 = Get-Content -Raw -LiteralPath $h5Path

$initialResetMatch = [regex]::Match(
    $hci,
    'case\s+HCI_INIT_SEND_RESET\s*:(?<body>[\s\S]*?)case\s+HCI_INIT_SEND_READ_LOCAL_VERSION_INFORMATION\s*:')

$checks = @(
    @{
        Name = 'GPCV2178 enables logical Reset resend'
        Pass = $config -match '#define\s+HCI_RESET_RESEND_ENABLE\s+1'
    },
    @{
        Name = 'GPCV2178 waits one second before logical Reset resend'
        Pass = $config -match '#define\s+HCI_RESET_RESEND_TIMEOUT_MS\s+1000'
    },
    @{
        Name = 'generic Reset resend remains enabled by default'
        Pass = $hci -match '#ifndef\s+HCI_RESET_RESEND_ENABLE\s*\r?\n#define\s+HCI_RESET_RESEND_ENABLE\s+1\s*\r?\n#endif'
    },
    @{
        Name = 'initial Reset state block is present'
        Pass = $initialResetMatch.Success
    },
    @{
        Name = 'initial Reset timer is compile-time guarded'
        Pass = $initialResetMatch.Success -and
            $initialResetMatch.Groups['body'].Value -match '#if\s+HCI_RESET_RESEND_ENABLE[\s\S]*?btstack_run_loop_add_timer\s*\(\s*&hci_stack->timeout\s*\)[\s\S]*?#endif'
    },
    @{
        Name = 'logical Reset is submitted outside timer setup'
        Pass = $initialResetMatch.Success -and
            $initialResetMatch.Groups['body'].Value -match '#endif[\s\S]*?hci_stack->substate\s*=\s*HCI_INIT_W4_SEND_RESET\s*;[\s\S]*?hci_send_cmd\s*\(\s*&hci_reset\s*\)'
    },
    @{
        Name = 'H5 reliable command transport remains enabled'
        Pass = $h5 -match 'int\s+reliable\s*=\s*hci_packet_type\s*==\s*HCI_SCO_DATA_PACKET\s*\?\s*0\s*:\s*1\s*;'
    }
)

$failed = @($checks | Where-Object { -not $_.Pass })
if ($failed.Count -ne 0) {
    foreach ($check in $failed) {
        Write-Error "Missing: $($check.Name)"
    }
    exit 1
}

Write-Host 'GPCV2178 retries logical HCI Reset after one second while H5 reliability remains enabled'

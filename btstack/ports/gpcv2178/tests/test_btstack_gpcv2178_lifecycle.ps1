$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent (
    Split-Path -Parent (Split-Path -Parent $PSScriptRoot)))
$header = Get-Content -Raw -LiteralPath (Join-Path $root `
    'btstack/ports/gpcv2178/btstack_gpcv2178_port.h')
$port = Get-Content -Raw -LiteralPath (Join-Path $root `
    'btstack/ports/gpcv2178/btstack_gpcv2178_port.c')
$platform = Get-Content -Raw -LiteralPath (Join-Path $root `
    'btstack/ports/gpcv2178/btstack_platform_gpcv2178.c')
$runtime = Get-Content -Raw -LiteralPath (Join-Path $root `
    'btstack/core/btstack_runtime.c')

$checks = @(
    @{ Name = 'compatibility init API'; Pass = $header -match 'int\s+btstack_gpcv2178_init\s*\(void\)' },
    @{ Name = 'compatibility run loop API'; Pass = $header -match 'int\s+btstack_gpcv2178_start_run_loop\s*\(void\)' },
    @{ Name = 'compatibility delegates generic runtime'; Pass = $port -match 'return\s+btstack_runtime_init\s*\(\s*\)' },
    @{ Name = 'strong default descriptor'; Pass = $platform -match 'const\s+btstack_platform_t\s*\*\s*btstack_platform_default\s*\(void\)' },
    @{ Name = 'HCI transport selected by descriptor'; Pass = $platform -match 'get_hci_transport[\s\S]+hci_transport_h[45]_instance' },
    @{ Name = 'HCI diagnostic after hook'; Pass = $platform -match 'after_hci_init[\s\S]+hci_add_event_handler' },
    @{ Name = 'generic HCI setup'; Pass = $runtime -match 'hci_init\s*\(\s*platform->stack\.get_hci_transport' },
    @{ Name = 'generic L2CAP setup'; Pass = $runtime -match 'l2cap_init\s*\(\s*\)' },
    @{ Name = 'generic Profile Manager setup'; Pass = $runtime -match 'btstack_profile_manager_init\s*\(' },
    @{ Name = 'preserved HFP speaker gain default'; Pass = $platform -match 'hfp_speaker_gain\s*=\s*14u' },
    @{ Name = 'generic backend setup'; Pass = $runtime -match 'btstack_core_dispatcher_init\s*\(\s*\)' },
    @{ Name = 'common run-loop-only worker'; Pass = $runtime -match 'btstack_runtime_stack_entry[\s\S]+btstack_run_loop_execute\s*\(\s*\)' },
    @{ Name = 'no automatic power on'; Pass = ($port + $platform + $runtime) -notmatch 'btstack_gap_set_power\s*\(\s*true\s*\)' }
)

$failed = @($checks | Where-Object { -not $_.Pass })
if ($failed.Count -ne 0) {
    foreach ($check in $failed) { Write-Error "Missing: $($check.Name)" }
    exit 1
}

Write-Host 'GPCV2178 descriptor delegates generic BTstack lifecycle'

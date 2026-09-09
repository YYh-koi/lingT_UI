$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$sourcePath = Join-Path $root 'btstack/ports/gpcv2178/btstack_hci_dump_gpcv2178.c'
$headerPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_hci_dump_gpcv2178.h'
$configPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_gpcv2178_config.h'
$portPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_platform_gpcv2178.c'
$h5Path = Join-Path $root 'btstack/transport/hci_transport_h5.c'
$printHeaderPath = Join-Path $root 'gpstdlib/print_string/inc/gplib_print_string.h'
$projectPath = Join-Path $root 'Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj'

foreach ($path in @($sourcePath, $headerPath, $configPath, $portPath, $h5Path, $printHeaderPath, $projectPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        Write-Error "Missing HCI dump artifact: $path"
        exit 1
    }
}

$source = Get-Content -Raw -LiteralPath $sourcePath
$header = Get-Content -Raw -LiteralPath $headerPath
$config = Get-Content -Raw -LiteralPath $configPath
$port = Get-Content -Raw -LiteralPath $portPath
$h5 = Get-Content -Raw -LiteralPath $h5Path
$printHeader = Get-Content -Raw -LiteralPath $printHeaderPath
$project = Get-Content -Raw -LiteralPath $projectPath
$printBufferMatch = [regex]::Match($printHeader, '#define\s+PRINT_BUF_SIZE\s+(\d+)')

$checks = @(
    @{
        Name = 'disabled compile-time switch'
        Pass = $config -match '#define\s+BTSTACK_GPCV2178_HCI_DUMP_ENABLE\s+0'
    },
    @{
        Name = 'dump instance declaration'
        Pass = $header -match 'btstack_hci_dump_gpcv2178_instance'
    },
    @{
        Name = 'command packet filter'
        Pass = $source -match 'HCI_COMMAND_DATA_PACKET'
    },
    @{
        Name = 'event packet filter'
        Pass = $source -match 'HCI_EVENT_PACKET'
    },
    @{
        Name = 'command dump label'
        Pass = $source -match 'HCI TX CMD len='
    },
    @{
        Name = 'event dump label'
        Pass = $source -match 'HCI RX EVT len='
    },
    @{
        Name = 'ACL summary packet selection'
        Pass = $source -match 'case\s+HCI_ACL_DATA_PACKET\s*:' -and
            $source -match 'HCI %s ACL len='
    },
    @{
        Name = 'SCO summary packet selection'
        Pass = $source -match 'case\s+HCI_SCO_DATA_PACKET\s*:' -and
            $source -match 'HCI %s SCO len='
    },
    @{
        Name = 'ISO summary packet selection'
        Pass = $source -match 'case\s+HCI_ISO_DATA_PACKET\s*:' -and
            $source -match 'HCI %s ISO len='
    },
    @{
        Name = 'full command and event line uses one print call'
        Pass = $source -match 'btstack_hci_dump_gpcv2178_log_full[\s\S]*?DBG_PRINT\s*\(\s*"%s%u:%s\\r\\n"'
    },
    @{
        Name = 'debug print buffer holds a maximum full HCI command line'
        Pass = $printBufferMatch.Success -and
            ([int]$printBufferMatch.Groups[1].Value -ge 800)
    },
    @{
        Name = 'legacy H5 ACL payload details are verbose-only'
        Pass = $h5 -notmatch 'BTSTACK_PRINTF\s*\(\s*"btstack h5: rx acl' -and
            $h5 -match 'H5_DIAG_PRINTF\s*\(\s*"btstack h5: rx acl-sig' -and
            $h5 -match 'H5_DIAG_PRINTF\s*\(\s*"btstack h5: rx acl-data'
    },
    @{
        Name = 'conditionally compiled dump initialization'
        Pass = $port -match '#if\s+BTSTACK_GPCV2178_HCI_DUMP_ENABLE[\s\S]*?hci_dump_init\s*\(\s*btstack_hci_dump_gpcv2178_instance\s*\(\s*\)\s*\)'
    },
    @{
        Name = 'library project source entry'
        Pass = $project -match 'btstack_hci_dump_gpcv2178\.c'
    }
)

$failed = @($checks | Where-Object { -not $_.Pass })
if ($failed.Count -ne 0) {
    foreach ($check in $failed) {
        Write-Error "Missing: $($check.Name)"
    }
    exit 1
}

Write-Host 'GPCV2178 HCI dump implementation is present and disabled by default'

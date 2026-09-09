param([string] $Compiler)

$ErrorActionPreference = 'Stop'
$gcc = if ($Compiler) { $Compiler } elseif (
    Test-Path -LiteralPath 'D:\mingw32\bin\gcc.exe') {
    'D:\mingw32\bin\gcc.exe'
} else { (Get-Command gcc.exe -ErrorAction Stop).Source }
$testsDir = $PSScriptRoot
$transportDir = Split-Path -Parent $testsDir
$root = Split-Path -Parent $transportDir
$build = Join-Path ([IO.Path]::GetTempPath()) `
    ('btstack-transport-host-' + [guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_btstack_transport_host.exe'

New-Item -ItemType Directory -Path $build | Out-Null
try {
    $includes = @(
        '-I', (Join-Path $root 'include'),
        '-I', (Join-Path $root 'include\adapter'),
        '-I', (Join-Path $root 'src'),
        '-I', $transportDir)
    & $gcc -std=c99 -Wall -Wextra -Werror `
        -DENABLE_H5 -DHCI_ACL_PAYLOAD_SIZE=1021 `
        -DHCI_INCOMING_PRE_BUFFER_SIZE=14 `
        -DHCI_INCOMING_PACKET_BUFFER_SIZE=1040 `
        -DHCI_OUTGOING_PRE_BUFFER_SIZE=14 `
        -DHCI_OUTGOING_PACKET_BUFFER_SIZE=1040 `
        @includes `
        (Join-Path $transportDir 'hci_transport_h4.c') `
        (Join-Path $transportDir 'hci_transport_h5.c') `
        (Join-Path $transportDir 'btstack_slip.c') `
        (Join-Path $transportDir 'btstack_uart_slip_wrapper.c') `
        (Join-Path $root 'src\btstack_util.c') `
        (Join-Path $testsDir 'test_btstack_transport_host.c') `
        -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw 'BTstack transport host compilation failed'
    }
    & $exe
    if ($LASTEXITCODE -ne 0) {
        throw "BTstack transport host smoke test failed: $LASTEXITCODE"
    }
}
finally {
    if (Test-Path -LiteralPath $build) {
        Remove-Item -LiteralPath $build -Recurse -Force
    }
}

Write-Output 'PASS: BTstack H4, H5, SLIP, and UART wrapper compile on the host'

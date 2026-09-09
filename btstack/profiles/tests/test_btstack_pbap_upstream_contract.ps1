$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path

# SHA-256 values come from the exact BTstack v1.6.2 raw files. Keeping this
# manifest local makes the contract fully offline. pbap_client.c is excluded:
# it is the v1.6.2 base plus the audited status-preservation patch below.
$upstreamFiles = [ordered]@{
    'btstack/src/classic/goep_client.c'                = '92bca31c6dae183f69b8bb78b5bbbdb4ff57daabf0abfee9fe4ee32b954fd536'
    'btstack/src/classic/obex_message_builder.c'       = '8787979b981f9de86ce580e99b536bbc4d4a7c6109167dba8b026107c2ed257b'
    'btstack/src/classic/obex_parser.c'                = '188c042f00d3620ceeb8d15dc1f4bc2875dff20a815744b9df0aa8889ff87df1'
    'btstack/src/classic/obex_iterator.c'              = '34053fd6e692059ca54a882e0e1556cc1e991dcb1a1b24dcdaeda4af8b5e7624'
    'btstack/src/classic/obex_srm_client.c'            = 'fbe55521c7e752ffcdd0e248399023fa77b1b8fb015cdf606729a5b56a9d921f'
    'btstack/src/classic/pbap_client.h'                = '4b6d4a36455dc71fc9af69a1d7ae27fa270c8edcb0732ca2f36667feac3d8dae'
    'btstack/src/classic/goep_client.h'                = 'eb5c68c1b11812d4ec49ffc872ada7416a4778e7d2d35cc2c580112d28838aa5'
    'btstack/src/classic/obex_message_builder.h'       = '4570d0135a99e00a4c19897a85d06a14d2db07a1cc51255c6782e742f88a278d'
    'btstack/src/classic/obex_parser.h'                = '14242265f6b9355464f5ad9d76c183eebc5568a8e53e2552b4ed980e7f50dfeb'
    'btstack/src/classic/obex_iterator.h'              = '57540dee21980d50f00f30171cbbe5b198675891ba07ae257f776c1d56dd60e7'
    'btstack/src/classic/obex_srm_client.h'            = 'afafdbd408afcb833c83cf138519de604b7d999887fa8aba21ed5772faec61ff'
    'btstack/3rd-party/md5/md5.c'                      = '0eb851f59869e6e4e23ccbaff30ba1874c1a0e5a00d3cfef7529561dfcee2550'
    'btstack/3rd-party/md5/md5.h'                      = '2f7fd79f1a8ec20e8f7febccbc445eddc25a654f4d6503f289561178e5aea781'
    'btstack/3rd-party/yxml/yxml.c'                    = '6a57993cb9234b6aa28f5b44edf9ef303840d2c08b6003ec56a6459a2a89803d'
    'btstack/3rd-party/yxml/yxml.h'                    = '4cce715c9529876cdd0b10c2b2bc18ca0b25d5514cf5d99c97aefa6baf72c878'
    'btstack/3rd-party/yxml/COPYING'                   = 'b9b5ca63904b3cf589d5719be2968436550914dc3fdf5d81b37ddbdd67986d06'
}

$pbapBaseSha256 = '0e7ed47854273b0424f45225740a52a21d701a0d076b1ad2af79ff42c55f2cb4'
$pbapPatchedSha256 = '18b2de664a3f30ebe14856ae3d096b382a543ae09377249335de8cd2e4d812c1'
$pbapPatchPath = 'btstack/patches/btstack-1.6.2-pbap-preserve-obex-status.patch'
$pbapPatchReadmePath = 'btstack/patches/README.md'

$upstreamSources = @(
    'btstack/src/classic/pbap_client.c'
    'btstack/src/classic/goep_client.c'
    'btstack/src/classic/obex_message_builder.c'
    'btstack/src/classic/obex_parser.c'
    'btstack/src/classic/obex_iterator.c'
    'btstack/src/classic/obex_srm_client.c'
    'btstack/3rd-party/md5/md5.c'
    'btstack/3rd-party/yxml/yxml.c'
)

function Assert-SourceEntriesAreUnique {
    param([Parameter(Mandatory)][string[]] $Entries)

    $duplicates = @($Entries | Group-Object | Where-Object Count -gt 1)
    if ($duplicates.Count -ne 0) {
        $names = ($duplicates.Name -join ', ')
        throw "PBAP upstream source entries must be listed exactly once; duplicates: $names"
    }
}

foreach ($relativePath in $upstreamFiles.Keys) {
    $fullPath = Join-Path $repoRoot $relativePath
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        throw "Missing BTstack v1.6.2 upstream file: $relativePath"
    }
}

foreach ($entry in $upstreamFiles.GetEnumerator()) {
    if ($entry.Value -notmatch '^[0-9a-f]{64}$') {
        throw "Contract SHA-256 is not finalized for $($entry.Key)"
    }

    $fullPath = Join-Path $repoRoot $entry.Key
    $actual = (Get-FileHash -LiteralPath $fullPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $entry.Value) {
        throw "BTstack v1.6.2 SHA-256 mismatch for $($entry.Key): expected $($entry.Value), got $actual"
    }
}

$pbapClientPath = Join-Path $repoRoot 'btstack/src/classic/pbap_client.c'
$pbapClient = Get-Content -LiteralPath $pbapClientPath -Raw
$actualPatchedSha256 = (Get-FileHash -LiteralPath $pbapClientPath -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actualPatchedSha256 -ne $pbapPatchedSha256) {
    throw "Patched BTstack v1.6.2 pbap_client.c SHA-256 mismatch: expected $pbapPatchedSha256, got $actualPatchedSha256"
}

$phonebookState = [regex]::Match($pbapClient,
    '(?s)case\s+PBAP_CLIENT_W4_PHONEBOOK\s*:\s*switch\s*\(\s*op_info\.response_code\s*\)\s*\{(.*?)case\s+PBAP_CLIENT_W4_GET_PHONEBOOK_SIZE_COMPLETE\s*:')
if (-not $phonebookState.Success) {
    throw 'pbap_client.c is missing the auditable PBAP_CLIENT_W4_PHONEBOOK response switch'
}
$phonebookSwitch = $phonebookState.Groups[1].Value
$requiredPhonebookMappings = [ordered]@{
    'OBEX_RESP_NOT_FOUND' = 'OBEX_NOT_FOUND'
    'OBEX_RESP_UNAUTHORIZED' = 'ERROR_CODE_AUTHENTICATION_FAILURE'
    'OBEX_RESP_FORBIDDEN' = 'ERROR_CODE_AUTHENTICATION_FAILURE'
}
foreach ($mapping in $requiredPhonebookMappings.GetEnumerator()) {
    if ($phonebookSwitch -notmatch "(?s)case\s+$([regex]::Escape($mapping.Key))\s*:.*?pbap_client_emit_operation_complete_event\s*\(\s*client\s*,\s*$([regex]::Escape($mapping.Value))\s*\)") {
        throw "PBAP_CLIENT_W4_PHONEBOOK must preserve $($mapping.Key) as $($mapping.Value)"
    }
}
if ($phonebookSwitch -notmatch '(?s)default\s*:.*?pbap_client_emit_operation_complete_event\s*\(\s*client\s*,\s*OBEX_UNKNOWN_ERROR\s*\)') {
    throw 'PBAP_CLIENT_W4_PHONEBOOK default response must remain OBEX_UNKNOWN_ERROR'
}
foreach ($response in @('OBEX_RESP_NOT_FOUND', 'OBEX_RESP_UNAUTHORIZED', 'OBEX_RESP_FORBIDDEN')) {
    if ([regex]::Matches($phonebookSwitch, "case\s+$response\s*:").Count -ne 1) {
        throw "PBAP_CLIENT_W4_PHONEBOOK must contain exactly one $response case"
    }
}

$patchFullPath = Join-Path $repoRoot $pbapPatchPath
$patchReadmeFullPath = Join-Path $repoRoot $pbapPatchReadmePath
foreach ($artifact in @($patchFullPath, $patchReadmeFullPath)) {
    if (-not (Test-Path -LiteralPath $artifact -PathType Leaf)) {
        throw "Missing audited PBAP local-patch artifact: $artifact"
    }
}
$patchText = Get-Content -LiteralPath $patchFullPath -Raw
$patchReadme = Get-Content -LiteralPath $patchReadmeFullPath -Raw
$apiReadme = Get-Content -LiteralPath (Join-Path $repoRoot 'btstack/README_API.md') -Raw -Encoding utf8
foreach ($token in @('OBEX_RESP_NOT_FOUND', 'OBEX_NOT_FOUND', 'OBEX_RESP_UNAUTHORIZED',
                      'OBEX_RESP_FORBIDDEN', 'ERROR_CODE_AUTHENTICATION_FAILURE')) {
    if ($patchText -notmatch [regex]::Escape($token)) {
        throw "PBAP local patch is missing status-preservation token: $token"
    }
}

$gitCommand = (Get-Command git -ErrorAction Stop).Source
$patchFixture = Join-Path ([IO.Path]::GetTempPath()) `
    ('btstack-pbap-patch-' + [guid]::NewGuid().ToString('N'))
$patchFixtureSource = Join-Path $patchFixture 'src/classic/pbap_client.c'
try {
    New-Item -ItemType Directory -Path (Split-Path -Parent $patchFixtureSource) |
        Out-Null
    Copy-Item -LiteralPath $pbapClientPath -Destination $patchFixtureSource
    Push-Location $patchFixture
    try {
        & $gitCommand apply --check --reverse $patchFullPath
        if ($LASTEXITCODE -ne 0) {
            throw 'PBAP local patch cannot be reverse-applied to the checked-in patched source'
        }
        & $gitCommand apply --reverse $patchFullPath
        if ($LASTEXITCODE -ne 0) {
            throw 'PBAP local patch reverse application failed'
        }
        $reconstructedBaseSha256 =
            (Get-FileHash -LiteralPath $patchFixtureSource -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($reconstructedBaseSha256 -ne $pbapBaseSha256) {
            throw "Reconstructed BTstack v1.6.2 pbap_client.c SHA-256 mismatch: expected $pbapBaseSha256, got $reconstructedBaseSha256"
        }
        & $gitCommand apply --check $patchFullPath
        if ($LASTEXITCODE -ne 0) {
            throw 'PBAP local patch cannot be applied to the reconstructed v1.6.2 base'
        }
        & $gitCommand apply $patchFullPath
        if ($LASTEXITCODE -ne 0) {
            throw 'PBAP local patch forward application failed'
        }
        $reconstructedPatchedSha256 =
            (Get-FileHash -LiteralPath $patchFixtureSource -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($reconstructedPatchedSha256 -ne $pbapPatchedSha256) {
            throw "Reconstructed patched pbap_client.c SHA-256 mismatch: expected $pbapPatchedSha256, got $reconstructedPatchedSha256"
        }
    }
    finally {
        Pop-Location
    }
}
finally {
    if (Test-Path -LiteralPath $patchFixture) {
        Remove-Item -LiteralPath $patchFixture -Recurse -Force
    }
}
foreach ($token in @('BTstack v1.6.2', $pbapBaseSha256, $pbapPatchedSha256,
                      'status-preservation', 'git apply --check --reverse',
                      'No newline at end of file')) {
    if ($patchReadme -notmatch [regex]::Escape($token)) {
        throw "PBAP patch README is missing provenance token: $token"
    }
}
foreach ($token in @('BTstack v1.6.2', 'status-preservation',
                      'BT_XSTACK_PHONEBOOK_STATUS_UNSUPPORTED',
                      'BT_XSTACK_PHONEBOOK_STATUS_DENIED',
                      'BT_XSTACK_PHONEBOOK_STATUS_PROTOCOL_ERROR')) {
    if ($apiReadme -notmatch [regex]::Escape($token)) {
        throw "PBAP API README is missing local-patch/status semantics: $token"
    }
}

foreach ($requiredInclude in @('md5.h', 'classic/pbap_client.h')) {
    if ($pbapClient -notmatch [regex]::Escape($requiredInclude)) {
        throw "pbap_client.c is missing required include: $requiredInclude"
    }
}

$pbapHeader = Get-Content -LiteralPath (Join-Path $repoRoot 'btstack/src/classic/pbap_client.h') -Raw
foreach ($requiredInclude in @('yxml.h', 'classic/obex_srm_client.h')) {
    if ($pbapHeader -notmatch [regex]::Escape($requiredInclude)) {
        throw "pbap_client.h is missing required transitive include: $requiredInclude"
    }
}

$md5Source = Get-Content -LiteralPath (Join-Path $repoRoot 'btstack/3rd-party/md5/md5.c') -Raw
if ($md5Source -notmatch 'This software was written by Alexander Peslyak in 2001') {
    throw 'md5.c is missing its upstream license notice'
}

# This list is consumed by the later build contract. At this stage it asserts
# uniqueness without requiring any build manifest to contain the sources yet.
Assert-SourceEntriesAreUnique -Entries $upstreamSources

Write-Output 'PASS: BTstack PBAP files match v1.6.2 plus the audited local status-preservation patch.'

param([string] $Compiler = 'D:\mingw32\bin\gcc.exe')

$ErrorActionPreference = 'Stop'
$tests = $PSScriptRoot
$root = (Resolve-Path (Join-Path $tests '..\..')).Path
$build = Join-Path ([IO.Path]::GetTempPath()) ('btstack_avrcp_cover_' + [guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_btstack_avrcp_cover.exe'

New-Item -ItemType Directory -Path $build | Out-Null
try {
    & $Compiler -std=c99 -O0 -Wall -Wextra -Werror -DBTSTACK_AVRCP_COVER_ART_TESTING=1 `
        -I (Join-Path $root 'include') -I (Join-Path $root 'include\core') `
        -I $root -I (Join-Path $root 'src') -I (Join-Path $root 'profiles') `
        (Join-Path $root 'profiles\btstack_avrcp_cover_art.c') `
        (Join-Path $tests 'test_btstack_avrcp_cover_art.c') -o $exe
    if ($LASTEXITCODE -ne 0) { throw 'AVRCP Cover Art policy compilation failed' }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw 'AVRCP Cover Art policy tests failed' }

    $sourcePath = Join-Path $root 'profiles\btstack_avrcp_cover_art.c'
    $mutantPath = Join-Path $build 'btstack_avrcp_cover_art_mutant.c'
    $mutantExe = Join-Path $build 'test_btstack_avrcp_cover_mutant.exe'
    $source = Get-Content -Raw -LiteralPath $sourcePath
    $mutant = $source -replace `
        'if \(event->generation != btstack_avrcp_cover_art\.active_generation\) return;', `
        '/* stale-generation guard removed by mutation */'
    if ($mutant -eq $source) { throw 'Stale-generation mutation target not found' }
    [IO.File]::WriteAllText($mutantPath, $mutant)
    & $Compiler -std=c99 -O0 -Wall -Wextra -Werror -DBTSTACK_AVRCP_COVER_ART_TESTING=1 `
        -I (Join-Path $root 'include') -I (Join-Path $root 'include\core') `
        -I $root -I (Join-Path $root 'src') -I (Join-Path $root 'profiles') `
        $mutantPath (Join-Path $tests 'test_btstack_avrcp_cover_art.c') -o $mutantExe
    if ($LASTEXITCODE -ne 0) { throw 'Stale-generation mutant compilation failed' }
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $mutantExe 2>$null
    $mutantExit = $LASTEXITCODE
    $ErrorActionPreference = $savedPreference
    if ($mutantExit -eq 0) { throw 'Stale-generation mutant unexpectedly passed' }

    $controller = Get-Content -Raw -LiteralPath (Join-Path $root 'profiles\btstack_avrcp_controller.c')
    foreach ($token in @('AVRCP_SUBEVENT_NOW_PLAYING_COVER_ART_INFO', 'btstack_avrcp_cover_art_handle_received', 'btstack_avrcp_cover_art_track_changed')) {
        if ($controller -notmatch [regex]::Escape($token)) { throw "AVRCP Controller missing Cover Art integration: $token" }
    }
    $avrcp = Get-Content -Raw -LiteralPath (Join-Path $root 'profiles\btstack_avrcp.c')
    foreach ($token in @('btstack_avrcp_cover_art_connected', 'btstack_avrcp_cover_art_disconnected')) {
        if ($avrcp -notmatch [regex]::Escape($token)) { throw "AVRCP session missing Cover Art integration: $token" }
    }
} finally {
    if (Test-Path -LiteralPath $build) { Remove-Item -LiteralPath $build -Recurse -Force }
}

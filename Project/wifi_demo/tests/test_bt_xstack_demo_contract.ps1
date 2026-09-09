$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$headerPath = Join-Path $root 'Project/Include/bt_xstack_demo.h'
$sourcePath = Join-Path $root 'Project/wifi_demo/bt_xstack_demo.c'
$audioHeaderPath = Join-Path $root 'Project/Include/bt_audio_task.h'
$audioSourcePath = Join-Path $root 'Project/wifi_demo/bt_audio_task.c'
$peripheralHeaderPath = Join-Path $root 'application/task_peripheral_handling/inc/ap_peripheral_handling.h'
$peripheralSourcePath = Join-Path $root 'application/task_peripheral_handling/src/ap_peripheral_handling.c'
$mainPath = Join-Path $root 'Project/GPCV2178_Turnkey_SDK/src/main.c'
$projectPath = Join-Path $root 'Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj'

foreach ($path in @($headerPath, $sourcePath, $audioHeaderPath, $audioSourcePath,
        $peripheralHeaderPath, $peripheralSourcePath, $mainPath, $projectPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing xStack demo input: $path"
    }
}

$header = Get-Content -Raw -LiteralPath $headerPath
$source = Get-Content -Raw -LiteralPath $sourcePath
$audioHeader = Get-Content -Raw -LiteralPath $audioHeaderPath
$audioSource = Get-Content -Raw -LiteralPath $audioSourcePath
$peripheralHeader = Get-Content -Raw -LiteralPath $peripheralHeaderPath
$peripheralSource = Get-Content -Raw -LiteralPath $peripheralSourcePath
$main = Get-Content -Raw -LiteralPath $mainPath
$projectText = Get-Content -Raw -LiteralPath $projectPath
$project = [xml]$projectText
$sourceWithoutConfiguredName = $source `
    -replace [regex]::Escape('"GP-BT-ATBM6165"'), '' `
    -replace [regex]::Escape('"AT+NAME=GP-BT-ATBM6165"'), ''

if ($header -notmatch 'int\s+bt_xstack_demo_start\s*\(\s*void\s*\)') {
    throw 'Missing generic bt_xstack_demo_start declaration'
}
if ($sourceWithoutConfiguredName -match '(?i)atbm|wifi') {
    throw 'xStack demo implementation is coupled to a WiFi vendor or WiFi layer'
}
if ($source -notmatch '#include\s+"bt_xstack\.h"') {
    throw 'xStack demo does not consume the canonical delivery header'
}
if ($source -match '#include\s+"adapter/bt_xstack\.h"') {
    throw 'xStack demo still consumes the internal source-tree header path'
}

$forbiddenDemoPatterns = @(
    '\bbt_xstack_send_at\s*\(',
    '"AT\+',
    '"\+(?:PWRSTAT|HFPSTAT|HFPRING|AVRCPSTAT|A2DPPCMCONFIG)=',
    '\bsscanf\s*\('
)
foreach ($pattern in $forbiddenDemoPatterns) {
    if ($source -match $pattern) {
        throw "Demo still contains AT text behavior: $pattern"
    }
}

foreach ($required in @(
        'bt_xstack_config_t',
        'BT_XSTACK_SERVICE_EVENT_POWER_STATE',
        'BT_XSTACK_PHONE_EVENT_STATE',
        'BT_XSTACK_PHONE_EVENT_RING',
        'BT_XSTACK_PHONE_EVENT_LOST',
        'BT_XSTACK_PROFILE_AVRCP',
        'BT_XSTACK_AUDIO_EVENT_PCM_CONFIG',
        'bt_xstack_set_local_name',
        'bt_xstack_hfp_answer',
        'bt_xstack_hfp_hang_up',
        'bt_xstack_avrcp_previous',
        'bt_xstack_avrcp_play_pause',
        'bt_xstack_avrcp_next',
        'bt_xstack_enable')) {
    if ($source -notmatch ('\b' + [regex]::Escape($required) + '\b')) {
        throw "Typed API demo contract is missing: $required"
    }
}
foreach ($callbackField in @(
        'service_callback', 'connection_callback', 'phone_callback',
        'phonebook_callback', 'audio_event_callback', 'media_audio_callback',
        'call_audio_callback')) {
    if ($source -notmatch ('config\.' + $callbackField + '\s*=')) {
        throw "Demo config does not register $callbackField"
    }
}

$phoneEventMatch = [regex]::Match(
    $source,
    '(?s)static\s+void\s+bt_xstack_demo_phone_event\s*\(.*?(?=static\s+void\s+bt_xstack_demo_phonebook_event)')
if (-not $phoneEventMatch.Success) {
    throw 'Cannot locate typed phone event callback'
}
$phoneEventSource = $phoneEventMatch.Value
$firstCritical = [regex]::Match($phoneEventSource, '\btaskENTER_CRITICAL\s*\(')
if (-not $firstCritical.Success) {
    throw 'Phone event callback has no critical section'
}
$phoneEventBeforeCritical = $phoneEventSource.Substring(0, $firstCritical.Index)
$deviceBranch = [regex]::Match(
    $phoneEventBeforeCritical,
    '(?s)if\s*\(\s*event->id\s*==\s*BT_XSTACK_PHONE_EVENT_DEVICE\s*\)\s*\{(?<body>.*?)\}')
if (-not $deviceBranch.Success) {
    throw 'Phone DEVICE handling must run before the callback critical section'
}
if ($phoneEventBeforeCritical -notmatch '(?s)DBG_PRINT\("xstack demo phone event:.*?DBG_PRINT\("\\r\\n"\s*\);.*?BT_XSTACK_PHONE_EVENT_DEVICE') {
    throw 'Phone DEVICE sync must run after the event is printed'
}
if ($deviceBranch.Groups['body'].Value -notmatch 'status\s*=\s*bt_xstack_phonebook_sync\s*\(\s*&event->address\s*,\s*BT_XSTACK_PHONEBOOK_ALL\s*\)') {
    throw 'Phone DEVICE event does not request every phonebook category'
}
if ($deviceBranch.Groups['body'].Value -notmatch 'DBG_PRINT\("xstack demo phonebook sync status=%d\\r\\n"\s*,\s*status\s*\)') {
    throw 'Phone DEVICE event does not report the phonebook sync result'
}
if ([regex]::Matches($phoneEventSource, '\bbt_xstack_phonebook_sync\s*\(').Count -ne 1) {
    throw 'Phone callback must request phonebook sync exactly once without retries'
}

$phonebookEventMatch = [regex]::Match(
    $source,
    '(?s)static\s+void\s+bt_xstack_demo_phonebook_event\s*\(.*?(?=static\s+void\s+bt_xstack_demo_connection_event)')
if (-not $phonebookEventMatch.Success) {
    throw 'Cannot locate typed phonebook event callback'
}
$phonebookEventSource = $phonebookEventMatch.Value
foreach ($required in @(
        'BT_XSTACK_PHONEBOOK_EVENT_CONTACT',
        'BT_XSTACK_PHONEBOOK_EVENT_CALL',
        'event->data.contact.name',
        'event->data.contact.numbers',
        'event->data.call.name',
        'event->data.call.number',
        'event->data.call.timestamp',
        'DBG_PRINT')) {
    if ($phonebookEventSource -notmatch [regex]::Escape($required)) {
        throw "Phonebook callback does not print each record field: $required"
    }
}
if ($source -notmatch 'BT_XSTACK_DEMO_STARTING') {
    throw 'Demo does not reserve startup ownership before side effects'
}
if ($source -notmatch '(?s)bt_xstack_demo_begin_start\s*\([^)]*\).*?taskENTER_CRITICAL\s*\(\s*\).*?BT_XSTACK_DEMO_IDLE.*?BT_XSTACK_DEMO_STARTING.*?taskEXIT_CRITICAL\s*\(\s*\)') {
    throw 'Demo startup ownership is not protected by one critical section'
}
if ($source -notmatch 'bt_xstack_init\s*\(\s*&config\s*\)') {
    throw 'Demo does not initialize xStack with bt_xstack_config_t'
}
if ($source -notmatch 'bt_xstack_set_local_name\s*\(\s*BT_XSTACK_DEMO_LOCAL_NAME\s*,\s*false\s*\)') {
    throw 'Demo does not set its name from the powered service event'
}
if ($source -notmatch 'bt_audio_task_configure\s*\(') {
    throw 'Typed PCM configuration event is not passed to the audio task'
}
foreach ($writer in @(
        'bt_audio_task_write_media_pcm',
        'bt_audio_task_write_call_pcm')) {
    if ($source -notmatch ($writer + '\s*\(\s*data\s*,\s*length\s*\)')) {
        throw "Typed PCM callback does not use explicit audio writer: $writer"
    }
}
$audioEventMatch = [regex]::Match(
    $source,
    '(?s)static\s+void\s+bt_xstack_demo_audio_event\s*\(.*?(?=static\s+void\s+bt_xstack_demo_pcm_event)')
if (-not $audioEventMatch.Success) {
    throw 'Cannot locate typed audio event callback'
}
$audioEventSource = $audioEventMatch.Value
foreach ($required in @(
        'BT_XSTACK_AUDIO_EVENT_A2DP_STATE',
        'BT_XSTACK_AUDIO_EVENT_A2DP_LOST',
        'BT_XSTACK_A2DP_STATE_STREAMING',
        'bt_audio_task_stop')) {
    if ($audioEventSource -notmatch [regex]::Escape($required)) {
        throw "Audio callback does not stop ended playback: $required"
    }
}
if ($audioEventSource -notmatch 'status\s*=\s*bt_audio_task_configure') {
    throw 'Audio callback silently discards PCM configuration status'
}
foreach ($resumeState in @(
        'bt_xstack_demo_pcm_config_valid',
        'bt_xstack_demo_resume_pending')) {
    if ($source -notmatch $resumeState) {
        throw "Demo does not preserve PCM format across suspend: $resumeState"
    }
}
if ([regex]::Matches(
        $audioEventSource,
        'bt_audio_task_configure\s*\(').Count -lt 2) {
    throw 'A2DP resume does not restore the cached PCM configuration'
}

foreach ($contract in @(
        'AP_PERIPHERAL_CALL_KEY_ANSWER',
        'AP_PERIPHERAL_CALL_KEY_HANGUP',
        'ap_peripheral_call_key_handler_t',
        'ap_peripheral_call_key_handler_register',
        'AP_PERIPHERAL_MEDIA_KEY_PREVIOUS',
        'AP_PERIPHERAL_MEDIA_KEY_PLAY_PAUSE',
        'AP_PERIPHERAL_MEDIA_KEY_NEXT',
        'ap_peripheral_media_key_handler_t',
        'ap_peripheral_media_key_handler_register')) {
    if ($peripheralHeader -notmatch $contract) {
        throw "Missing generic peripheral key API: $contract"
    }
}
if ($peripheralSource -match '#include\s+"(?:bt_xstack|bt_xstack_demo)\.h"') {
    throw 'Peripheral key module is directly coupled to Bluetooth'
}
foreach ($api in @('bt_audio_task_start', 'bt_audio_task_stop',
        'bt_audio_task_configure', 'bt_audio_task_write_pcm',
        'bt_audio_task_write_media_pcm',
        'bt_audio_task_write_call_pcm',
        'bt_audio_task_get_diagnostics')) {
    if ($audioHeader -notmatch $api) {
        throw "Missing platform audio task API: $api"
    }
}
if (($audioSource -notmatch 'xStreamBufferCreateStatic\s*\(') -or
    ($audioSource -notmatch 'xStreamBufferSend\s*\(')) {
    throw 'PCM callback does not copy into its bounded static stream buffer'
}
if ($main -notmatch '#include\s+"bt_xstack_demo\.h"') {
    throw 'Main application does not include the xStack demo interface'
}
if ($main -notmatch '(?s)if\s*\(\s*atbm_wifi_p2p_mode_enable\s*\(\s*\)\s*==\s*STATUS_OK\s*\)\s*\{.*?bt_xstack_demo_start\s*\(\s*\)') {
    throw 'xStack demo is not started inside the successful ATBM WiFi branch'
}

if ($projectText -match 'bt[\\/]libxstack\.a') {
    throw 'Main project still links legacy libxstack.a'
}
foreach ($legacyInclude in @(
        '../../btstack/at',
        '../../btstack/include/adapter')) {
    if ($projectText -match [regex]::Escape($legacyInclude)) {
        throw "Main project still contains legacy include: $legacyInclude"
    }
}
if ($projectText -match '<Add\s+directory="\.\./\.\./gplib/liba_wifi_src/atbm/atbm6165/bt"') {
    throw 'Main project still searches the legacy packaged BT headers'
}
foreach ($targetName in @('Debug', 'Release')) {
    $target = $project.SelectSingleNode("//Target[@title='$targetName']")
    if ($null -eq $target) {
        throw "Main project is missing target: $targetName"
    }
    $targetText = $target.OuterXml
    foreach ($include in @(
            '../../btstack_api/include/api',
            '../../btstack_api/ports/gpcv2178',
            '../../gplib/liba_wifi_src/atbm/atbm6165/bt_api')) {
        if ($targetText -notmatch [regex]::Escape($include)) {
            throw "$targetName target is missing API include: $include"
        }
    }
    if ($targetText -notmatch 'bt_api[\\/]libxstack_api\.a') {
        throw "$targetName target does not link libxstack_api.a"
    }
}

$sourceUnits = @($project.SelectNodes(
    '//Unit[contains(translate(@filename,"\","/"),"wifi_demo/bt_xstack_demo.c")]'))
$headerUnits = @($project.SelectNodes(
    '//Unit[contains(translate(@filename,"\","/"),"Include/bt_xstack_demo.h")]'))
$audioSourceUnits = @($project.SelectNodes(
    '//Unit[contains(translate(@filename,"\","/"),"wifi_demo/bt_audio_task.c")]'))
$audioHeaderUnits = @($project.SelectNodes(
    '//Unit[contains(translate(@filename,"\","/"),"Include/bt_audio_task.h")]'))
if (($sourceUnits.Count -ne 1) -or ($headerUnits.Count -ne 1) -or
    ($audioSourceUnits.Count -ne 1) -or ($audioHeaderUnits.Count -ne 1)) {
    throw 'Demo or audio project units are not present exactly once'
}

Write-Host 'WiFi-gated typed xStack API demo contract is present'

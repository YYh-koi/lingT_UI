# BTstack API Demo Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将现有 WiFi-gated 蓝牙 Demo 从 AT 文本接口完全切换到 `bt_xstack_*` 类型化 API，并让主应用只链接 `libxstack_api.a`。

**Architecture:** 保留 `bt_xstack_demo_start()` 和现有按键/音频任务边界，内部改用 service、connection、phone、audio callback 以及 `bt_xstack_pcm_callback_t`。主工程的 Debug/Release include 路径改指向 `btstack_api`，Debug 链接项改为 `bt_api/libxstack_api.a`；不修改原 `btstack/`。

**Tech Stack:** C99、FreeRTOS critical section、G+IDE `.gpcmprj`、PowerShell 契约测试、ARM GCC/G+IDE 工具链。

---

## 文件结构

- 修改 `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`：定义类型化 Demo 和新库链接的可执行契约。
- 修改 `Project/wifi_demo/bt_xstack_demo.c`：类型化 callback、初始化、按键和 PCM 数据流。
- 修改 `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`：替换 include 路径和静态库。
- 不修改 `Project/Include/bt_xstack_demo.h`：继续只暴露 `int bt_xstack_demo_start(void)`。
- 不修改 `btstack/` 和 `btstack_api/` 协议栈实现。

当前目录不是 Git 工作区，因此每个任务末尾记录验证结果，不执行 Git commit。

### Task 1: 将 Demo 契约切换为类型化 API（RED）

**Files:**
- Modify: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`
- Test: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`

- [ ] **Step 1: 删除旧 AT 行为断言**

删除要求以下内容存在的断言：`bt_xstack_init(bt_xstack_demo_event, NULL)`、
`bt_xstack_send_at()`、`AT+...`、`+PWRSTAT`、`+HFPSTAT`、`+HFPRING`、
`+AVRCPSTAT`、`+A2DPPCMCONFIG` 和按长度打印 AT 文本的循环。

- [ ] **Step 2: 添加“禁止 AT”契约**

在读取 `$source` 后加入：

```powershell
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
```

- [ ] **Step 3: 添加 callback、直接 API 和工程链接契约**

加入以下断言：

```powershell
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
        'audio_callback', 'pcm_callback')) {
    if ($source -notmatch ('config\.' + $callbackField + '\s*=')) {
        throw "Demo config does not register $callbackField"
    }
}

$projectText = Get-Content -Raw -LiteralPath $projectPath
if ($projectText -notmatch 'bt_api[\\/]libxstack_api\.a') {
    throw 'Main project does not link libxstack_api.a'
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
if ($projectText -notmatch [regex]::Escape('../../btstack_api/include/api')) {
    throw 'Main project is missing the packaged API include path'
}
```

- [ ] **Step 4: 运行契约并确认预期失败**

Run:

```powershell
& .\Project\wifi_demo\tests\test_bt_xstack_demo_contract.ps1
```

Expected: FAIL，首个错误指出 Demo 仍含 `bt_xstack_send_at()` 或 AT 字符串，而不是脚本语法错误。

### Task 2: 将 Demo 事件和控制改成类型化 API（GREEN，Demo 部分）

**Files:**
- Modify: `Project/wifi_demo/bt_xstack_demo.c`
- Test: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`

- [ ] **Step 1: 删除 AT 常量、文本解析器和重复状态枚举**

删除所有 `BT_XSTACK_DEMO_*_COMMAND`、`bt_xstack_demo_copy_event_line()`、
`bt_xstack_demo_power_event()`、`bt_xstack_demo_call_state()`、
`bt_xstack_demo_avrcp_state()`、`bt_xstack_demo_pcm_config()`、
`bt_xstack_demo_event()` 以及本地 HFP 数值枚举。状态变量改成公共类型：

```c
#define BT_XSTACK_DEMO_LOCAL_NAME "GP-BT-ATBM6165"

static volatile bt_xstack_demo_start_state_t
    bt_xstack_demo_start_state = BT_XSTACK_DEMO_IDLE;
static volatile bt_xstack_hfp_state_t bt_xstack_demo_hfp_state =
    BT_XSTACK_HFP_STATE_IDLE;
static volatile uint8_t bt_xstack_demo_hfp_ringing;
static volatile uint8_t bt_xstack_demo_avrcp_connected;
```

- [ ] **Step 2: 添加类型化 service callback**

```c
static void bt_xstack_demo_service_event(
    const bt_xstack_service_event_t *event,
    void *context)
{
    int submit_name = 0;
    bt_xstack_result_t status;

    (void)context;
    if ((event == NULL) ||
        (event->id != BT_XSTACK_SERVICE_EVENT_POWER_STATE) ||
        (event->value_count == 0u)) {
        return;
    }
    if (event->values[0] == 0u) {
        taskENTER_CRITICAL();
        if (bt_xstack_demo_start_state != BT_XSTACK_DEMO_IDLE) {
            bt_xstack_demo_start_state = BT_XSTACK_DEMO_WAIT_POWER;
        }
        bt_xstack_demo_hfp_ringing = 0u;
        bt_xstack_demo_hfp_state = BT_XSTACK_HFP_STATE_IDLE;
        bt_xstack_demo_avrcp_connected = 0u;
        taskEXIT_CRITICAL();
        return;
    }
    taskENTER_CRITICAL();
    if (bt_xstack_demo_start_state == BT_XSTACK_DEMO_WAIT_POWER) {
        bt_xstack_demo_start_state = BT_XSTACK_DEMO_SETTING_NAME;
        submit_name = 1;
    }
    taskEXIT_CRITICAL();
    if (!submit_name) return;

    status = bt_xstack_set_local_name(
        BT_XSTACK_DEMO_LOCAL_NAME, false);
    DBG_PRINT("xstack demo name status=%d\r\n", status);
    taskENTER_CRITICAL();
    if (bt_xstack_demo_start_state == BT_XSTACK_DEMO_SETTING_NAME) {
        bt_xstack_demo_start_state =
            (status == BT_XSTACK_OK) ?
            BT_XSTACK_DEMO_STARTED : BT_XSTACK_DEMO_WAIT_POWER;
    }
    taskEXIT_CRITICAL();
}
```

- [ ] **Step 3: 添加 phone、connection、audio 和 PCM callbacks**

```c
static void bt_xstack_demo_phone_event(
    const bt_xstack_phone_event_t *event,
    void *context)
{
    uint32_t value;
    (void)context;
    if (event == NULL) return;

    taskENTER_CRITICAL();
    if ((event->id == BT_XSTACK_PHONE_EVENT_STATE) &&
        (event->value_count > 0u)) {
        value = event->values[0];
        if ((value >= BT_XSTACK_HFP_STATE_IDLE) &&
            (value <= BT_XSTACK_HFP_STATE_MULTIPARTY)) {
            bt_xstack_demo_hfp_state = (bt_xstack_hfp_state_t)value;
            if ((value != BT_XSTACK_HFP_STATE_INCOMING) &&
                (value != BT_XSTACK_HFP_STATE_ACTIVE_WITH_INCOMING)) {
                bt_xstack_demo_hfp_ringing = 0u;
            }
        }
    } else if ((event->id == BT_XSTACK_PHONE_EVENT_RING) &&
        (event->value_count > 0u)) {
        bt_xstack_demo_hfp_ringing = event->values[0] ? 1u : 0u;
    } else if (event->id == BT_XSTACK_PHONE_EVENT_LOST) {
        bt_xstack_demo_hfp_ringing = 0u;
        bt_xstack_demo_hfp_state = BT_XSTACK_HFP_STATE_IDLE;
    }
    taskEXIT_CRITICAL();
}

static void bt_xstack_demo_connection_event(
    const bt_xstack_connection_event_t *event,
    void *context)
{
    (void)context;
    if ((event == NULL) || (event->profile != BT_XSTACK_PROFILE_AVRCP)) {
        return;
    }
    taskENTER_CRITICAL();
    bt_xstack_demo_avrcp_connected =
        (event->state == BT_XSTACK_CONNECTION_CONNECTED) ? 1u : 0u;
    taskEXIT_CRITICAL();
}

static void bt_xstack_demo_audio_event(
    const bt_xstack_audio_event_t *event,
    void *context)
{
    (void)context;
    if ((event == NULL) ||
        (event->id != BT_XSTACK_AUDIO_EVENT_PCM_CONFIG) ||
        (event->value_count < 3u)) {
        return;
    }
    (void)bt_audio_task_configure(
        event->values[0],
        (uint8_t)event->values[1],
        (uint8_t)event->values[2]);
}

static void bt_xstack_demo_pcm_event(
    const uint8_t *data,
    uint32_t length,
    void *context)
{
    (void)context;
    bt_audio_task_write_pcm(data, length);
}
```

- [ ] **Step 4: 将通话按键改为直接 API**

保留现有状态许可条件，将命令选择替换为：

```c
if (action == AP_PERIPHERAL_CALL_KEY_ANSWER) {
    if (!bt_xstack_demo_hfp_ringing &&
        (state != BT_XSTACK_HFP_STATE_INCOMING) &&
        (state != BT_XSTACK_HFP_STATE_ACTIVE_WITH_INCOMING)) {
        return 0u;
    }
    status = bt_xstack_hfp_answer();
} else if (action == AP_PERIPHERAL_CALL_KEY_HANGUP) {
    if (!bt_xstack_demo_hfp_ringing &&
        (state != BT_XSTACK_HFP_STATE_OUTGOING) &&
        (state != BT_XSTACK_HFP_STATE_INCOMING) &&
        (state != BT_XSTACK_HFP_STATE_ACTIVE) &&
        (state != BT_XSTACK_HFP_STATE_HELD) &&
        (state != BT_XSTACK_HFP_STATE_ACTIVE_WITH_INCOMING) &&
        (state != BT_XSTACK_HFP_STATE_ACTIVE_WITH_HELD) &&
        (state != BT_XSTACK_HFP_STATE_MULTIPARTY)) {
        return 0u;
    }
    status = bt_xstack_hfp_hang_up();
} else {
    return 0u;
}
```

- [ ] **Step 5: 将媒体按键改为直接 API**

在已连接检查后使用：

```c
switch (action) {
case AP_PERIPHERAL_MEDIA_KEY_PREVIOUS:
    status = bt_xstack_avrcp_previous();
    break;
case AP_PERIPHERAL_MEDIA_KEY_PLAY_PAUSE:
    status = bt_xstack_avrcp_play_pause();
    break;
case AP_PERIPHERAL_MEDIA_KEY_NEXT:
    status = bt_xstack_avrcp_next();
    break;
default:
    return 0u;
}
```

- [ ] **Step 6: 用 `bt_xstack_config_t` 重写启动入口**

```c
int bt_xstack_demo_start(void)
{
    bt_xstack_config_t config = {0};
    bt_xstack_result_t status;

    if (bt_xstack_demo_get_start_state() != BT_XSTACK_DEMO_IDLE) {
        return BT_XSTACK_OK;
    }
    if (bt_audio_task_start() != 0) {
        return BT_XSTACK_INTERNAL_ERROR;
    }

    config.service_callback = bt_xstack_demo_service_event;
    config.connection_callback = bt_xstack_demo_connection_event;
    config.phone_callback = bt_xstack_demo_phone_event;
    config.audio_callback = bt_xstack_demo_audio_event;
    config.pcm_callback = bt_xstack_demo_pcm_event;
    status = bt_xstack_init(&config);
    if (status != BT_XSTACK_OK) {
        bt_audio_task_stop();
        return status;
    }

    ap_peripheral_call_key_handler_register(
        bt_xstack_demo_call_key_handler);
    ap_peripheral_media_key_handler_register(
        bt_xstack_demo_media_key_handler);
    taskENTER_CRITICAL();
    bt_xstack_demo_start_state = BT_XSTACK_DEMO_WAIT_POWER;
    taskEXIT_CRITICAL();

    status = bt_xstack_enable();
    DBG_PRINT("xstack demo enable status=%d\r\n", status);
    if (status != BT_XSTACK_OK) {
        taskENTER_CRITICAL();
        bt_xstack_demo_start_state = BT_XSTACK_DEMO_IDLE;
        taskEXIT_CRITICAL();
        bt_audio_task_stop();
    }
    return status;
}
```

- [ ] **Step 7: 运行契约并确认只剩工程配置失败**

Run:

```powershell
& .\Project\wifi_demo\tests\test_bt_xstack_demo_contract.ps1
```

Expected: FAIL with `Main project does not link libxstack_api.a` 或旧 include 路径错误；不得再因 Demo 含 AT 文本失败。

### Task 3: 将主应用工程切换到 API 库（GREEN）

**Files:**
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- Test: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`

- [ ] **Step 1: 修改 Debug target include 路径**

将 Debug target 中以下十个目录：

```xml
<Add directory="../../btstack/ports/gpcv2178" />
<Add directory="../../btstack/include" />
<Add directory="../../btstack/at" />
<Add directory="../../btstack/include/adapter" />
<Add directory="../../btstack/src" />
<Add directory="../../btstack/3rd-party/bluedroid/decoder/include" />
<Add directory="../../btstack/transport" />
<Add directory="../../btstack/adapter" />
<Add directory="../../btstack/events" />
<Add directory="../../btstack/gap" />
```

替换为：

```xml
<Add directory="../../btstack_api/ports/gpcv2178" />
<Add directory="../../btstack_api/include/api" />
```

- [ ] **Step 2: 修改 Release target include 路径**

对 Release target 执行相同替换，确保工程 XML 中不再出现
`../../btstack/at` 或 `../../btstack/include/adapter`。

- [ ] **Step 3: 修改 Debug target 静态库**

将：

```xml
<Add library="..\..\gplib\liba_wifi_src\atbm\atbm6165\bt\libxstack.a" />
```

替换为：

```xml
<Add library="..\..\gplib\liba_wifi_src\atbm\atbm6165\bt_api\libxstack_api.a" />
```

Release target 当前没有静态库列表，不新增与本任务无关的 Release 库集合。

- [ ] **Step 4: 运行 Demo 契约并确认通过**

Run:

```powershell
& .\Project\wifi_demo\tests\test_bt_xstack_demo_contract.ps1
```

Expected: `WiFi-gated typed xStack API demo contract is present`。

### Task 4: 编译库和主应用

**Files:**
- Verify: `Project/WirelessLibraries/BTstack_API_Library/build_btstack_api.ps1`
- Verify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- Generated: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release_Debug.mk`

- [ ] **Step 1: 重新构建并打包 API 库**

Run:

```powershell
& .\Project\WirelessLibraries\BTstack_API_Library\build_btstack_api.ps1
```

Expected: exit code 0，并输出 `Built ...\libxstack_api.a and packaged public headers`。允许复制自上游的 `avdtp.c` 既有 `uuid may be used uninitialized` 警告，不允许 Demo/API 新增警告。

- [ ] **Step 2: 用 G+IDE 生成器生成主应用 Debug Makefile**

Run:

```powershell
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\gpcmprj2make.exe' `
  -in '.\Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj' `
  -cfg 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\gpcmprj2make.cfg' `
  -out '.\Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release_Debug.mk' `
  -tp 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin' `
  -windows -targets Debug --with-deps --quote-path auto
```

Expected: exit code 0 and `[DONE]` for makefile generation.

- [ ] **Step 3: 编译主应用 Debug target**

在 `Project/GPCV2178_Turnkey_SDK` 下运行：

```powershell
$env:Path = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin;D:\mingw32\bin;' + $env:Path
& 'D:\mingw32\bin\mingw32-make.exe' `
  -f '.\GPCV2178_turnkey_sdk_release_Debug.mk' -j4
```

Expected: Demo 编译使用 `btstack_api/include/api`，链接命令包含
`bt_api/libxstack_api.a`，并成功生成主应用输出。若仓库原有的非蓝牙对象导致失败，
记录首个原始错误，并继续执行 Task 5 的静态和回归验证。

### Task 5: 最终回归和隔离验证

**Files:**
- Test: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`
- Test: `btstack_api/api/tests/*.ps1`
- Verify: `btstack/`

- [ ] **Step 1: 运行 Demo 和 API 契约**

Run:

```powershell
& .\Project\wifi_demo\tests\test_bt_xstack_demo_contract.ps1
Get-ChildItem .\btstack_api\api\tests\*.ps1 |
    Sort-Object Name | ForEach-Object { & $_.FullName }
```

Expected: Demo 契约和全部 API 契约通过。

- [ ] **Step 2: 验证 Demo 无 AT 依赖、工程只链接新库**

Run:

```powershell
rg -n 'bt_xstack_send_at|"AT\+|"\+(PWRSTAT|HFPSTAT|HFPRING|AVRCPSTAT|A2DPPCMCONFIG)=' `
  .\Project\wifi_demo\bt_xstack_demo.c
rg -n 'libxstack_api\.a|btstack_api/include/api' `
  .\Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj
```

Expected: 第一个命令无匹配；第二个命令只显示新库和新 include 路径。

- [ ] **Step 3: 验证原 `btstack/` 未变化**

使用现有 `tmp/xstack_api_original_manifest.xml` 对 369 个原始文件重新计算 SHA-256。
Expected: 文件数为 369，且无路径或哈希差异。

- [ ] **Step 4: 记录目标板剩余验证**

交付说明中明确列出尚需实机确认的上电、设置名称、来电接听/挂断、上一曲/播放暂停/下一曲、A2DP PCM 配置和连续音频播放。

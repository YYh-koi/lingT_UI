# BTstack 通用 Classic Profile 分层重构实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 HFP、A2DP、AVRCP 和通用 AT 后端从 GPCV2178 port 迁移到可复用的 `btstack/profiles` 与 `btstack/at` 层。

**Architecture:** 每个 Classic Profile 独立拥有原生 BTstack 初始化、操作、状态和事件解析；组合层负责 SDP 和生命周期，通用 AT 后端只依赖 Profile API。GPCV2178 port 最终只负责平台任务、transport、run-loop、control、storage 和通用模块启动。

**Tech Stack:** C11、BTstack Classic、FreeRTOS、PowerShell 契约测试、ARM GCC、GDB ARM simulator。

---

当前目录没有 Git 或 SVN 元数据，所有“提交”检查点改为运行对应契约测试和
目标编译器检查，不执行版本控制命令。

### Task 1: 建立分层边界失败检查

**Files:**
- Create: `btstack/profiles/tests/check_btstack_profile_layer.ps1`
- Modify: `tmp/tests/check_btstack_at_interface.ps1`

- [ ] **Step 1: 编写 Profile 层边界检查**

检查脚本必须要求：

```powershell
$required = @(
    'btstack/profiles/btstack_classic_profiles.c',
    'btstack/profiles/btstack_avrcp.c',
    'btstack/include/profiles/btstack_classic_profiles.h',
    'btstack/include/profiles/btstack_avrcp.h',
    'btstack/at/btstack_at_backend.c'
)
```

并禁止 `btstack/ports/gpcv2178/*.c` 出现：

```text
hfp_hf_
a2dp_sink_
avrcp_controller_
HCI_EVENT_HFP_META
HCI_EVENT_A2DP_META
HCI_EVENT_AVRCP_META
```

同时禁止 `btstack/at/btstack_at_backend.c` 出现上述三组原生 Profile API。

- [ ] **Step 2: 运行检查确认 RED**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File btstack/profiles/tests/check_btstack_profile_layer.ps1
```

Expected: FAIL，报告缺少通用 Classic、AVRCP 和 AT backend 文件，并报告 port
中仍有原生 Profile 操作。

- [ ] **Step 3: 扩展工程集成检查**

`tmp/tests/check_btstack_at_interface.ps1` 必须要求新通用文件存在于
`Makefile.inc` 和 `.gpcmprj`，并要求旧 port 单元不存在。

### Task 2: 定义通用 Classic 和 Profile 接口

**Files:**
- Create: `btstack/include/profiles/btstack_classic_profiles.h`
- Create: `btstack/include/profiles/btstack_avrcp.h`
- Modify: `btstack/include/profiles/btstack_hfp_hf.h`
- Modify: `btstack/include/profiles/btstack_a2dp_sink.h`
- Create: `btstack/profiles/tests/test_btstack_profile_contract.c`

- [ ] **Step 1: 编写公共接口编译测试**

测试必须包含四个头文件并实例化：

```c
btstack_classic_profiles_config_t classic_config;
btstack_hfp_hf_event_t hfp_event;
btstack_a2dp_sink_event_t a2dp_event;
btstack_avrcp_event_t avrcp_event;
```

同时引用以下接口，保证签名稳定：

```c
btstack_classic_profiles_setup(&classic_config);
btstack_hfp_hf_connect(address);
btstack_hfp_hf_disconnect();
btstack_a2dp_sink_connect(address);
btstack_a2dp_sink_disconnect();
btstack_avrcp_control(BTSTACK_AVRCP_CONTROL_PLAY);
```

- [ ] **Step 2: 编译确认 RED**

Run:

```powershell
arm-none-eabi-gcc -std=c11 -Wall -Wextra -Werror -fsyntax-only `
  -Ibtstack/include -Ibtstack/ports/gpcv2178 `
  btstack/profiles/tests/test_btstack_profile_contract.c
```

Expected: FAIL，缺少 Classic/AVRCP 头文件和新增类型。

- [ ] **Step 3: 定义 Classic 配置**

`btstack_classic_profiles_config_t` 必须包含：

```c
const char *device_name;
const char *manufacturer;
uint32_t class_of_device;
uint32_t a2dp_sdp_handle;
uint32_t hfp_sdp_handle;
uint32_t avrcp_controller_sdp_handle;
uint32_t avrcp_target_sdp_handle;
uint8_t hfp_rfcomm_channel;
uint8_t hfp_microphone_gain;
uint8_t hfp_speaker_gain;
```

并声明：

```c
int btstack_classic_profiles_setup(
    const btstack_classic_profiles_config_t *config);
```

- [ ] **Step 4: 定义独立 Profile 事件**

每个 Profile header 声明有界结构化事件及 callback：

```c
typedef void (*btstack_hfp_hf_profile_event_callback_t)(
    const btstack_hfp_hf_event_t *event);
typedef void (*btstack_a2dp_sink_event_callback_t)(
    const btstack_a2dp_sink_event_t *event);
typedef void (*btstack_avrcp_event_callback_t)(
    const btstack_avrcp_event_t *event);
```

事件必须包含 address、状态、必要数值和有界文本，不包含 AT command ID。

- [ ] **Step 5: 编译确认 GREEN**

Expected: 公共接口测试以 `-Werror` 通过。

### Task 3: 实现通用 HFP HF 层

**Files:**
- Modify: `btstack/profiles/btstack_hfp_hf.c`
- Modify: `btstack/include/profiles/btstack_hfp_hf.h`
- Create: `btstack/profiles/tests/check_btstack_hfp_profile.ps1`

- [ ] **Step 1: 编写 HFP 失败检查**

要求 `btstack_hfp_hf.c` 包含：

```text
hfp_hf_init
hfp_hf_register_packet_handler
hfp_hf_establish_service_level_connection
hfp_hf_release_service_level_connection
hfp_hf_dial_number
hfp_hf_send_dtmf_code
hfp_hf_answer_incoming_call
hfp_hf_terminate_call
hfp_hf_establish_audio_connection
hfp_hf_activate_voice_recognition
HFP_SUBEVENT_SERVICE_LEVEL_CONNECTION_ESTABLISHED
HFP_SUBEVENT_AG_INDICATOR_STATUS_CHANGED
```

- [ ] **Step 2: 运行确认 RED**

Expected: 现有占位实现缺少全部原生操作和事件处理。

- [ ] **Step 3: 实现 HFP 初始化和状态**

保存：

```c
static bool setup_done;
static hci_con_handle_t acl_handle;
static btstack_bd_addr_t peer_address;
static btstack_hfp_hf_state_t state;
static btstack_hfp_audio_state_t audio_state;
```

`setup()` 初始化 HFP、feature、codec、增益、packet handler 和 SDP record。

- [ ] **Step 4: 实现 HFP 操作 API**

连接、断开、拨号、DTMF、接听、挂断、多方通话、SCO 和语音识别全部在该文件
调用原生 `hfp_hf_*`。未连接时返回 `BTSTACK_STATUS_NOT_READY`。

- [ ] **Step 5: 实现 HFP 事件映射**

原始 HFP packet handler 更新内部状态并发布通用事件：

```text
STATE
DEVICE
AUDIO
VOICE_RECOGNITION
SIGNAL
NETWORK
ROAM
BATTERY
NUMBER
IN_BAND_RING
RING
LOST
MULTIPART_CALL
```

- [ ] **Step 6: 运行 HFP 检查和 ARM 编译**

Expected: HFP 契约通过；该源文件以 `-Wall -Wextra -Werror` 通过。

### Task 4: 实现通用 A2DP Sink 层

**Files:**
- Modify: `btstack/profiles/btstack_a2dp_sink.c`
- Modify: `btstack/include/profiles/btstack_a2dp_sink.h`
- Create: `btstack/profiles/tests/check_btstack_a2dp_profile.ps1`

- [ ] **Step 1: 编写 A2DP 失败检查**

要求：

```text
a2dp_sink_init
a2dp_sink_create_stream_endpoint
a2dp_sink_register_packet_handler
a2dp_sink_register_media_handler
a2dp_sink_establish_stream
a2dp_sink_disconnect
A2DP_SUBEVENT_SIGNALING_CONNECTION_ESTABLISHED
A2DP_SUBEVENT_STREAM_STARTED
A2DP_SUBEVENT_SIGNALING_CONNECTION_RELEASED
```

- [ ] **Step 2: 运行确认 RED**

Expected: 现有包装层仍是占位控制实现。

- [ ] **Step 3: 实现初始化和控制**

该文件拥有 SBC capabilities/configuration、SDP record、A2DP CID、peer address
和 stream state。新增：

```c
int btstack_a2dp_sink_connect(const btstack_bd_addr_t address);
int btstack_a2dp_sink_disconnect(void);
```

- [ ] **Step 4: 实现事件和媒体处理**

packet handler 发布 CONNECTING、CONNECTED、STREAM_OPEN、STREAMING、
SUSPENDED、IDLE、DEVICE 和 LOST 事件。media callback 为空时丢弃 packet，
不解码。

- [ ] **Step 5: 运行 A2DP 检查和 ARM 编译**

Expected: A2DP 契约和严格编译通过。

### Task 5: 新增通用 AVRCP CT/TG 层

**Files:**
- Create: `btstack/profiles/btstack_avrcp.c`
- Create: `btstack/include/profiles/btstack_avrcp.h`
- Create: `btstack/profiles/tests/check_btstack_avrcp_profile.ps1`

- [ ] **Step 1: 编写 AVRCP 失败检查**

要求初始化、三类 packet handler、连接、全部首期控制、notification、metadata
和 progress timer API。

- [ ] **Step 2: 运行确认 RED**

Expected: AVRCP 源文件不存在。

- [ ] **Step 3: 实现状态和控制**

定义：

```c
typedef enum {
    BTSTACK_AVRCP_CONTROL_PLAY,
    BTSTACK_AVRCP_CONTROL_PAUSE,
    BTSTACK_AVRCP_CONTROL_STOP,
    BTSTACK_AVRCP_CONTROL_NEXT,
    BTSTACK_AVRCP_CONTROL_PREVIOUS,
    BTSTACK_AVRCP_CONTROL_FAST_FORWARD,
    BTSTACK_AVRCP_CONTROL_REWIND,
    BTSTACK_AVRCP_CONTROL_VOLUME_UP,
    BTSTACK_AVRCP_CONTROL_VOLUME_DOWN
} btstack_avrcp_control_t;
```

所有操作调用相应 `avrcp_controller_*`，并在未连接时返回
`BTSTACK_STATUS_NOT_READY`。

- [ ] **Step 4: 实现事件和 timer**

保存 CID、play state、repeat、shuffle、absolute volume、title、artist、album。
连接后启用 playback/track notification。进度 timer 只调用
`avrcp_controller_get_play_status()`，并始终运行在 BTstack run-loop。

- [ ] **Step 5: 运行 AVRCP 检查和 ARM 编译**

Expected: AVRCP 契约和严格编译通过。

### Task 6: 实现 Classic 组合层

**Files:**
- Create: `btstack/profiles/btstack_classic_profiles.c`
- Create: `btstack/include/profiles/btstack_classic_profiles.h`
- Create: `btstack/profiles/tests/check_btstack_classic_setup.ps1`

- [ ] **Step 1: 编写组合层失败检查**

要求组合层调用：

```c
sdp_init();
rfcomm_init();
btstack_a2dp_sink_setup(...);
btstack_avrcp_setup(...);
btstack_hfp_hf_setup(...);
gap_set_local_name(...);
gap_set_class_of_device(...);
```

- [ ] **Step 2: 运行确认 RED**

Expected: 组合层源文件不存在。

- [ ] **Step 3: 实现 setup**

验证配置指针和字符串，按 A2DP、AVRCP、HFP 顺序初始化；任一步失败立即返回，
不启动后续 Profile。最后设置 Classic GAP 默认值。

- [ ] **Step 4: 运行组合层检查和 ARM 编译**

Expected: 组合层契约和严格编译通过。

### Task 7: 迁移为通用 AT 后端

**Files:**
- Create: `btstack/at/btstack_at_backend.c`
- Create: `btstack/at/btstack_at_backend.h`
- Modify: `btstack/at/tests/test_btstack_at_backend_contract.ps1`
- Delete: `btstack/ports/gpcv2178/btstack_at_gpcv2178.c`
- Delete: `btstack/ports/gpcv2178/btstack_at_gpcv2178.h`

- [ ] **Step 1: 修改后端契约并确认 RED**

契约改为检查 `btstack_at_backend.c`，并禁止其中出现：

```text
hfp_hf_
a2dp_sink_establish_stream
avrcp_controller_
HCI_EVENT_HFP_META
HCI_EVENT_A2DP_META
HCI_EVENT_AVRCP_META
```

- [ ] **Step 2: 迁移 General/GAP**

保留现有 RAM 默认值、PIN、SSP、扫描、配对列表、BTEN、COD、PAGE、PAIR 和
DSCA 行为，删除 GPCV2178 命名。

- [ ] **Step 3: 替换 Profile 命令映射**

HFP、A2DP、AVRCP command switch 只调用：

```text
btstack_hfp_hf_*
btstack_a2dp_sink_*
btstack_avrcp_*
```

- [ ] **Step 4: 注册 Profile 结构化事件 callback**

三个 callback 仅将 Profile event 转换为 `btstack_at_event_t` 并调用
`btstack_at_post_event()`；不解析原始 packet。

- [ ] **Step 5: 删除旧 port AT 后端并运行回归**

Expected: AT backend 契约、解析器、格式器和任务契约全部通过。

### Task 8: 清理 port 并接入工程

**Files:**
- Modify: `btstack/ports/gpcv2178/btstack_gpcv2178_port.c`
- Delete: `btstack/ports/gpcv2178/btstack_gpcv2178_profiles.c`
- Delete: `btstack/ports/gpcv2178/btstack_gpcv2178_profiles.h`
- Modify: `btstack/adapter/Makefile.inc`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- Modify: `tmp/tests/check_btstack_at_interface.ps1`

- [ ] **Step 1: 在 port 中构造通用配置**

`btstack_gpcv2178_port.c` 在 `l2cap_init()` 后调用：

```c
btstack_classic_profiles_setup(&classic_config);
btstack_at_backend_init();
```

port 不注册任何 Profile packet handler。

- [ ] **Step 2: 更新 Makefile 和 GPC 工程**

加入：

```text
btstack/profiles/btstack_classic_profiles.c
btstack/profiles/btstack_avrcp.c
btstack/at/btstack_at_backend.c
```

保留 HFP/A2DP 通用源文件，删除四个旧 port 单元。

- [ ] **Step 3: 删除旧 port Profile 文件**

删除前确认工程和源代码已无 include 或符号引用。

- [ ] **Step 4: 运行分层和工程检查**

Expected: port 边界、Makefile、工程 XML 和所有契约通过。

### Task 9: 最终验证

**Files:**
- Modify only if a failing test requires a tested correction.

- [ ] **Step 1: 运行 Profile 契约**

```powershell
powershell -ExecutionPolicy Bypass -File btstack/profiles/tests/check_btstack_profile_layer.ps1
powershell -ExecutionPolicy Bypass -File btstack/profiles/tests/check_btstack_hfp_profile.ps1
powershell -ExecutionPolicy Bypass -File btstack/profiles/tests/check_btstack_a2dp_profile.ps1
powershell -ExecutionPolicy Bypass -File btstack/profiles/tests/check_btstack_avrcp_profile.ps1
powershell -ExecutionPolicy Bypass -File btstack/profiles/tests/check_btstack_classic_setup.ps1
```

- [ ] **Step 2: 运行 AT 回归**

解析器和协议测试必须在 ARM simulator 返回 `$1 = 0`。任务、backend、工程
集成脚本必须全部退出 0。

- [ ] **Step 3: 严格目标编译**

全部新增和修改 C 源文件使用 Cortex-A7：

```text
-std=c11 -mcpu=cortex-a7 -marm -Wall -Wextra -Werror -fsyntax-only
```

Expected: 零警告、零错误。

- [ ] **Step 4: 最终边界扫描**

确认：

- `ports/gpcv2178` 不包含原生 Profile 操作或 meta event 解析。
- AT backend 不包含原生 Profile API。
- Profile 层不包含 BARROT 文本或 AT command ID。
- GPC 工程 XML 可解析。
- A2DP media 和 HFP SCO 音频范围未扩大。

- [ ] **Step 5: 兼容性核对**

逐项核对设计文档的兼容性章节：公共 AT API、命令白名单、BARROT 字节格式、
响应/事件顺序、RAM 默认值、Profile mask 424、A2DP 丢包和 SCO 不接音频均
不得变化。

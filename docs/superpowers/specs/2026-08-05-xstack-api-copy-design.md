# xStack 独立 API 版本设计

## 目标

在不修改现有 `btstack/`、`libxstack.a`、AT 接口和现有应用 Demo 的前提下，完整复制一份协议栈源码到 `btstack_api/`。复制版保持 HCI、H5/UART、L2CAP、SDP、GAP、Classic Profile、SBC 和 PCM 主体逻辑不变，只将面向应用的 AT 文本层替换为强类型 C API。

API 版本输出独立静态库 `libxstack_api.a`。所有现有 AT 命令能力均提供对应 API；控制和设置操作异步入队，查询操作同步读取线程安全状态快照。

## 不修改的内容

- 现有 `btstack/` 源码目录。
- 现有 `Project/WirelessLibraries/BTstack_Library/` 工程。
- 现有 `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a` 和 `bt_xstack.h`。
- 现有 AT 命令、AT 响应、AT 事件格式和应用 Demo。
- 控制器传输、BTstack run-loop、Profile 实现和音频解码路径。

## 交付布局

```text
btstack_api/
├── 3rd-party/
├── adapter/
├── api/
├── events/
├── gap/
├── include/
│   └── api/
│       ├── bt_xstack.h
│       ├── bt_xstack_types.h
│       ├── bt_xstack_service.h
│       ├── bt_xstack_phone.h
│       └── bt_xstack_audio.h
├── ports/
├── profiles/
├── src/
└── transport/

Project/WirelessLibraries/Xstack_API_Library/

gplib/liba_wifi_src/atbm/atbm6165/bt_api/
├── libxstack_api.a
├── bt_xstack.h
├── bt_xstack_types.h
├── bt_xstack_service.h
├── bt_xstack_phone.h
└── bt_xstack_audio.h
```

`btstack_api/` 从当前 `btstack/` 完整复制。复制后，`btstack_api/at/` 不加入 API 库工程，新增 `btstack_api/api/` 替代其应用接入职责。

## API 命名和头文件

参考 `D:/Project/Platform/Generalplus/bt_stack_api` 的服务、电话和音频分层，但不直接导出 `BTSrv_*`、`BTPhoneMgr_*` 或 `BTAudioMgr_*` 名称。所有公开符号统一使用 `bt_xstack_*` 前缀。

外部应用只需包含：

```c
#include "bt_xstack.h"
```

`bt_xstack.h` 是统一入口，并包含四个分类子头文件：

- `bt_xstack_types.h`：结果码、地址、Profile、连接状态和公共数据结构。
- `bt_xstack_service.h`：生命周期、GAP、扫描、配对、设备和 Profile 连接。
- `bt_xstack_phone.h`：HFP 电话状态和控制。
- `bt_xstack_audio.h`：A2DP、AVRCP、媒体信息和 PCM。

## 返回语义

```c
typedef enum {
    BT_XSTACK_OK = 0,
    BT_XSTACK_INVALID_ARGUMENT = -1,
    BT_XSTACK_NOT_INITIALIZED = -2,
    BT_XSTACK_CONFIGURATION_CONFLICT = -3,
    BT_XSTACK_QUEUE_FULL = -4,
    BT_XSTACK_NOT_READY = -5,
    BT_XSTACK_INTERNAL_ERROR = -6
} bt_xstack_result_t;
```

异步控制函数返回 `BT_XSTACK_OK` 只表示参数合法且请求已经复制到请求队列，不表示底层或远端操作成功。API 不产生 `COMMAND_COMPLETE`，run-loop 中的延迟失败和执行超时也不通知应用。应用通过后续状态事件和同步 Getter 判断最终状态。

## 生命周期

```c
bt_xstack_result_t bt_xstack_init(
    const bt_xstack_config_t *config);
bt_xstack_result_t bt_xstack_enable(void);
bt_xstack_result_t bt_xstack_disable(void);
bt_xstack_result_t bt_xstack_get_state(
    bt_xstack_state_t *state);
```

`bt_xstack_init()` 初始化端口、事件路由、静态队列和 API Worker，不启动控制器。`bt_xstack_enable()` 对应 `AT+BTEN=1`，首次调用时启动 BTstack run-loop 并异步上电。`bt_xstack_disable()` 对应 `AT+BTEN=0`。

重复初始化且配置一致返回成功；配置不同返回 `BT_XSTACK_CONFIGURATION_CONFLICT`。

## 回调模型

接口采用业务域回调，结构参考现有 Generalplus API，但所有类型使用 `bt_xstack_*` 前缀：

```c
typedef struct {
    bt_xstack_service_callback_t service_callback;
    bt_xstack_connection_callback_t connection_callback;
    bt_xstack_phone_callback_t phone_callback;
    bt_xstack_audio_callback_t audio_callback;
    bt_xstack_pcm_callback_t pcm_callback;
    void *context;
} bt_xstack_config_t;
```

- Service callback：电源、可发现/可连接状态、扫描、配对和设备事件。
- Connection callback：HFP、A2DP 和 AVRCP 连接状态。
- Phone callback：HFP 电话状态、音频、号码、信号、网络、漫游、电量和来电事件。
- Audio callback：A2DP 状态、PCM 配置、播放状态、播放模式、曲目信息和绝对音量。
- PCM callback：解码后的高频 PCM 数据，独立于普通事件队列。

普通回调固定从 API Worker 执行。普通事件数据只在回调期间有效。PCM 保持现有直接回调路径，回调不得阻塞。

## 现有命令到 API 的覆盖

### Service 和 GAP

- `AT`：`bt_xstack_is_ready()`。
- `PROFILE?` / `PROFILE=`：`bt_xstack_get_profile_mask()` / `bt_xstack_set_profile_mask()`。
- `VER?`：`bt_xstack_get_version()`。
- `BTEN=`：`bt_xstack_enable()` / `bt_xstack_disable()`。
- `ADDR?`：`bt_xstack_get_local_address()`。
- `NAME?` / `NAME=`：`bt_xstack_get_local_name()` / `bt_xstack_set_local_name()`。
- `PIN?` / `PIN=`：`bt_xstack_get_pin()` / `bt_xstack_set_pin()`。
- `SSP?` / `SSP=`：`bt_xstack_get_ssp_mode()` / `bt_xstack_set_ssp_mode()`。
- `CFM=`：`bt_xstack_pairing_confirm()`。
- `PAIR` / `PAIR=`：`bt_xstack_set_discoverable()`。
- `PAGE` / `PAGE=`：`bt_xstack_set_connectable()`。
- `COD?` / `COD=`：`bt_xstack_get_class_of_device()` / `bt_xstack_set_class_of_device()`。
- `PLIST?` / `PLIST=`：配对设备 Getter、单项删除和全部清除 API。
- `TDL?` / `TDL=`：`bt_xstack_get_tdl_mode()` / `bt_xstack_set_tdl_mode()`。
- `SCAN=`：`bt_xstack_scan_start()` / `bt_xstack_scan_stop()`。
- `DSCA`：`bt_xstack_disconnect_all()`。
- `MICMUTE=`：`bt_xstack_set_microphone_mute()`。
- `SPKVOL?` / `SPKVOL=`：扬声器音量 Getter/Setter。

### HFP

- `HFPCFG?` / `HFPCFG=`：HFP 配置 Getter/Setter。
- `HFPSTAT?`：HFP 状态 Getter。
- `HFPCONN` / `HFPCONN=`：连接最近设备/指定设备。
- `HFPDISC`：HFP 断开。
- `HFPDIAL` / `HFPDIAL=`：重拨/号码拨号。
- `HFPDTMF=`：发送 DTMF。
- `HFPANSW`：接听。
- `HFPCHUP`：挂断。
- `HFPMCAL=`：多方通话控制。
- `HFPADTS` / `HFPADTS=`：切换/设置 HFP 音频连接。
- `HFPVR=`：语音识别控制。

### A2DP 和 AVRCP

- `A2DPSTAT?`：A2DP 状态 Getter。
- `A2DPCONN` / `A2DPCONN=`：连接最近设备/指定设备。
- `A2DPDISC`：A2DP 断开。
- `A2DPMUTE=`：A2DP 静音设置。
- `AVRCPCFG?` / `AVRCPCFG=`：AVRCP 配置 Getter/Setter。
- `PLAYPAUSE`、`PLAY`、`PAUSE`、`STOP`：对应 AVRCP 控制 API。
- `NEXT`、`PREV`、`FORWARD`、`BACKWARD`：对应 AVRCP 导航控制 API。
- `REPEAT=`、`SHUFFLE=`：对应播放模式 Setter。
- `SABSUP`、`SABSDOWN`：对应 AVRCP 音量加减 API。

API 版只承诺当前 AT 版本已实现的 Classic、HFP HF、A2DP Sink 和 AVRCP 能力。参考接口中的 BLE、PBAP、SPP、RFCOMM、AVRCP Browsing、Cover Art 和 AAC 不在本次范围内，也不声明固定返回失败的空壳接口。

## 内部组件

```text
Application
    |
    v
bt_xstack_* public API
    |
    v
Static request queue
    |
    v
API Worker Task
    |
    v
btstack_run_loop_execute_on_main_thread()
    |
    +--> GAP
    +--> HFP HF
    +--> A2DP Sink
    +--> AVRCP
    |
    v
Native profile events
    |
    v
API event mapper + static event queue
    |
    v
API Worker Task callbacks
```

`btstack_at_backend.c` 的业务执行 switch 复制到 API 版后，输入从 `btstack_at_command_t` 改成内部强类型请求。AT parser、命令描述表、文本 formatter 和 AT 输出回调不进入 API 库。

API Worker 仍保留内部单请求串行执行和 run-loop 完成握手，防止复用同一个 callback registration 时请求覆盖。该完成握手只供内部推进队列，不暴露给应用。

## 静态资源和并发

沿用当前 AT 层配置：

- 请求队列：8 项。
- 普通事件队列：16 项。
- Worker 栈：2048 words。
- Worker 优先级：`tskIDLE_PRIORITY + 2`。
- 内部执行超时：1000 ms。

异步控制接口只允许普通 FreeRTOS 任务调用，不支持 ISR。地址、字符串和数值参数在返回前完整复制到静态请求项，不保存调用方指针，也不使用动态内存。

普通事件队列满时丢弃新事件并增加内部诊断计数，不阻塞 BTstack run-loop。Getter 在临界区内复制状态快照后返回，不在持锁状态调用应用代码。

## 构建隔离

新增 `Project/WirelessLibraries/Xstack_API_Library/`，只引用 `btstack_api/`。工程输出 `libxstack_api.a`，不修改现有 `BTstack_Library` 工程和 `libxstack.a`。

API 库构建后复制五个公开头文件和静态库到 `gplib/liba_wifi_src/atbm/atbm6165/bt_api/`。API 版验证工程只能链接 `libxstack_api.a`，不得同时链接 `libxstack.a`。

## 错误处理

- 空指针、无效枚举、越界数值和过长文本立即返回 `BT_XSTACK_INVALID_ARGUMENT`。
- 未初始化返回 `BT_XSTACK_NOT_INITIALIZED`。
- 初始化完成但运行条件不满足返回 `BT_XSTACK_NOT_READY`。
- 请求队列满返回 `BT_XSTACK_QUEUE_FULL`。
- 入队后的底层执行失败或超时不通知应用，应用根据状态事件和 Getter 判断。
- 事件丢失后，应用可通过同步 Getter 恢复当前状态。

## 验证策略

1. 复制边界测试：确认 `btstack/` 和现有工程文件未被修改，API 工程只引用 `btstack_api/`。
2. 公开头文件编译测试：C 和 C++ 编译器均能只包含 `bt_xstack.h`。
3. API 覆盖测试：当前命令描述表中的每个受支持 AT 命令均有对应公开 API。
4. 参数测试：空指针、地址、字符串长度、数值范围和枚举边界。
5. 队列测试：输入复制、队列满、请求顺序和单请求串行执行。
6. 线程测试：底层操作只在 BTstack run-loop 执行，普通回调只在 API Worker 执行。
7. Getter 测试：状态快照一致且不返回内部可变指针。
8. 事件映射测试：当前全部 AT 事件均映射到对应 Service、Connection、Phone 或 Audio 结构化事件，状态语义保持一致。
9. 溢出测试：事件队列满不阻塞 run-loop，后续 Getter 可恢复状态。
10. 构建测试：从空对象目录生成 `libxstack_api.a`，检查公开符号并链接 API 验证工程。
11. 回归测试：现有 AT 版测试和 `libxstack.a` 产物保持不变。
12. 板级测试：电源、扫描、配对、HFP 通话、A2DP 播放、AVRCP 控制和 PCM 输出。

## 验收条件

- 当前 AT 版源码、工程、库和行为未改变。
- `btstack_api/` 是独立可构建的完整源码副本。
- 所有当前受支持 AT 命令均有对应 `bt_xstack_*` API。
- 控制操作异步、查询操作同步，无应用可见完成事件。
- 当前全部 AT 异步事件均有结构化 API 表达。
- 所有 BTstack 操作仍在 run-loop 执行。
- 普通事件不在 run-loop 中调用应用。
- API 路径不使用动态内存。
- `libxstack_api.a` 可独立链接并完成板级核心流程。

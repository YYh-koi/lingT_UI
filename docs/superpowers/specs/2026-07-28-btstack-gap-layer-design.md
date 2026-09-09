# BTstack 独立 GAP 层设计

## 1. 目标

在 SDK 中建立独立、通用的 GAP 层，统一管理 Classic GAP 操作、运行时状态以及 HCI/GAP 事件转换，然后通过稳定的 SDK 事件接口提供给 AT Backend。

完成后：

- `btstack/gap/` 是业务 GAP 功能唯一接触原生 BTstack `hci_*`、`gap_*` API 和 HCI/GAP packet 的层；
- AT Backend 只调用 `btstack_gap_*()`，只接收 `btstack_gap_event_t`；
- `ports/` 不注册业务 HCI handler，也不直接控制 HCI 电源；
- HFP、A2DP、AVRCP 继续由 `btstack/profiles/` 管理；
- AT 命令字符串、响应文本、异步事件及 RAM-only 配置行为保持严格兼容。

首期范围仅包含 Classic 通用控制面和状态/事件映射，不增加 BLE GAP 功能，也不实现 A2DP 解码播放或 HFP SCO 音频接入。

## 2. 方案选择

采用独立 GAP 目录方案：扩展现有 `btstack/gap/` 和 `btstack/include/gap/`，使其与 `profiles/`、`at/` 平级。

未采用的方案：

- 将 GAP 放入 `profiles/`：改动较小，但 GAP 不是 Profile，目录和职责语义不准确；
- 只增加 AT 前置事件适配器：无法阻止 AT Backend 继续直接调用原生 GAP API，分层不完整。

## 3. 分层结构

```text
btstack/gap/
├─ btstack_gap.c          GAP总入口、统一状态、HCI事件注册与分发
├─ btstack_gap_link.c     本地设备配置、ACL链路及可连接/可发现控制
├─ btstack_gap_scan.c     Classic Inquiry控制
└─ btstack_gap_security.c PIN、SSP和配对确认

btstack/include/gap/
├─ btstack_gap.h
├─ btstack_gap_link.h
├─ btstack_gap_scan.h
└─ btstack_gap_security.h
```

### 3.1 `btstack_gap.c`

- 由 `btstack_adapter_setup()` 调用并且只初始化一次；
- 作为 SDK GAP 业务层唯一的 `hci_add_event_handler()` 注册点；
- 保存统一 GAP RAM 状态；
- 将原始 `BTSTACK_EVENT_*`、`HCI_EVENT_*`、`GAP_EVENT_*` 转换为 `btstack_gap_event_t`；
- 同步调用已注册的 GAP 上层回调；
- 重复初始化不得重复挂接 HCI handler。

“唯一注册点”只约束 SDK GAP 业务层。BTstack 内部的 L2CAP、HFP 等协议模块仍可注册其自身需要的 HCI handler。

### 3.2 `link/scan/security`

- 只提供 SDK 级 GAP 操作；
- 内部允许调用原生 BTstack GAP/HCI API；
- 不包含任何 AT 命令 ID、BARROT 事件或文本格式；
- 使用 SDK 的 `btstack_status_t` 返回语义。

### 3.3 AT Backend

- 注册一个 `btstack_gap_event_callback_t`；
- 将统一 GAP 事件映射为 `btstack_at_event_t`；
- 继续通过 `btstack_at_post_event()` 将事件复制到 AT 事件队列；
- 不直接解析 HCI/GAP packet；
- 不直接调用原生 `hci_*()` 或 `gap_*()`。

### 3.4 Port与Profile

- Port 只负责平台、传输层、任务和 SDK 启动；
- Port 通过 `btstack_gap_set_power(true)` 上电，不直接调用 `hci_power_control()`；
- Port 移除仅用于打印状态的业务 HCI handler；
- HFP、A2DP、AVRCP 保持在通用 Profile 层，不迁入 GAP 层。

## 4. GAP统一状态

GAP 层持有：

- 协议栈电源和工作状态；
- 本地蓝牙地址；
- 本地名称；
- Class of Device；
- PIN；
- SSP 模式；
- connectable、discoverable、scanning 状态；
- 当前 ACL handle 和对端地址；
- 待确认配对地址及有效标志。

AT Backend 不再重复持有上述 GAP 状态。AT 命令集专属状态，例如 AT peer list、Profile 状态、媒体状态和音量，仍由相应 AT/Profile 层管理。

所有配置首期只保存在 RAM；SDK 或设备重启后恢复编译期/AT Backend 默认值。

## 5. 统一事件接口

```c
typedef enum {
    BTSTACK_GAP_EVENT_STACK_STATE = 0,
    BTSTACK_GAP_EVENT_LOCAL_ADDRESS,
    BTSTACK_GAP_EVENT_ACL_CONNECTED,
    BTSTACK_GAP_EVENT_ACL_DISCONNECTED,
    BTSTACK_GAP_EVENT_INQUIRY_RESULT,
    BTSTACK_GAP_EVENT_INQUIRY_COMPLETE,
    BTSTACK_GAP_EVENT_PAIRING_REQUEST,
    BTSTACK_GAP_EVENT_PAIRING_COMPLETE
} btstack_gap_event_type_t;

typedef struct {
    btstack_gap_event_type_t type;
    int status;
    btstack_bd_addr_t address;
    uint32_t class_of_device;
    int8_t rssi;
    uint8_t stack_working;
    uint8_t name_length;
    char name[BTSTACK_LOCAL_NAME_MAX];
} btstack_gap_event_t;

typedef void (*btstack_gap_event_callback_t)(
    const btstack_gap_event_t *event);
```

事件结构使用固定长度、值语义数据，不向上层暴露原始 packet 指针。事件对象及其中的数据只保证在同步回调期间有效。

`status` 对普通 SDK 操作使用 `btstack_status_t` 语义；配对完成事件保留 Bluetooth 状态值，以维持现有 AT 事件数值兼容。

## 6. 公开操作接口

### 6.1 总入口和状态

```c
int btstack_gap_init(void);
int btstack_gap_set_event_callback(
    btstack_gap_event_callback_t callback);
int btstack_gap_set_power(bool enabled);
bool btstack_gap_is_working(void);
int btstack_gap_get_local_address(btstack_bd_addr_t address);
```

### 6.2 本地设备和链路

```c
int btstack_gap_set_local_name(const char *name);
int btstack_gap_set_class_of_device(uint32_t class_of_device);
int btstack_gap_set_discoverable(bool enabled);
int btstack_gap_set_connectable(bool enabled);
int btstack_gap_disconnect(void);
```

### 6.3 搜索

```c
int btstack_gap_scan_start(uint8_t duration);
int btstack_gap_scan_stop(void);
bool btstack_gap_is_scanning(void);
```

### 6.4 安全和配对

```c
int btstack_gap_set_pin(const char *pin);
int btstack_gap_set_ssp_mode(uint8_t mode);
int btstack_gap_pairing_confirm(
    const btstack_bd_addr_t address,
    bool accept);
```

对外头文件只依赖 SDK 公共类型，不暴露 `hci_con_handle_t`、`bd_addr_t` 或原始 BTstack event getter。

## 7. 初始化与配置时序

```text
hci_init/l2cap_init
        ↓
btstack_adapter_setup
        ↓
btstack_gap_init
        ↓
link/scan/security初始化
        ↓
Classic Profiles初始化
        ↓
AT Backend注册GAP/Profile回调并设置默认GAP配置
        ↓
btstack_gap_set_power(true)
        ↓
BTstack run loop
```

AT Backend 初始化时使用原有默认值调用 GAP setter：

```c
btstack_gap_set_event_callback(btstack_at_backend_gap_event);
btstack_gap_set_local_name(BTSTACK_AT_DEFAULT_NAME);
btstack_gap_set_pin(BTSTACK_AT_DEFAULT_PIN);
btstack_gap_set_class_of_device(BTSTACK_AT_DEFAULT_COD);
btstack_gap_set_ssp_mode(2u);
```

进入 `HCI_STATE_WORKING` 后，GAP 层刷新本地地址，重新应用 RAM 中的名称、COD 和 SSP 配置，然后发出 `BTSTACK_GAP_EVENT_STACK_STATE`。同一次运行中的 HCI 断电再上电继续使用当前 RAM 配置。

初始化函数调用必须先由局部 `ret` 或 `status` 接收结果，再单独传给调试打印函数。

## 8. 配对行为

保持当前 AT 行为：

- SSP 0：关闭 SSP；
- SSP 1：No Input/No Output，自动确认；
- SSP 2：Display Yes/No，自动确认；
- SSP 3：Display Yes/No，发出 `PAIRING_REQUEST` 并等待 AT `CFM`；
- PIN请求由 GAP 层使用当前 RAM PIN 自动响应；
- `btstack_gap_pairing_confirm()` 仅接受当前 pending 地址；
- 配对完成时 GAP 层清除 pending 状态并发出完成事件；
- AT Backend 在成功事件后继续维护命令集需要的 peer list。

## 9. 事件流与顺序

```text
控制器/BTstack原始事件
        ↓
btstack_gap HCI handler
        ↓
更新GAP内部状态
        ↓
同步发出btstack_gap_event_t
        ↓
AT Backend映射为btstack_at_event_t
        ↓
btstack_at_post_event复制入队
        ↓
btstack_at任务格式化、去重并调用统一输出回调
```

GAP 层不增加 FreeRTOS 队列。唯一跨任务事件边界仍为 AT 事件队列。

顺序约束：

- ACL连接成功：先保存 handle 和地址，再发连接事件；
- ACL断开：先保存待上报地址，清除内部链路状态，再发断开事件；
- Inquiry结果逐个转换和上报，最后发完成事件；
- 协议栈状态事件在内部状态与本地配置生效后发出；
- GAP 回调运行在 BTstack 任务上下文，AT 输出回调仍运行在 AT 任务上下文。

## 10. 错误处理

- 空指针、非法名称/PIN长度、非法SSP模式返回 `BTSTACK_STATUS_INVALID_ARGUMENT`；
- GAP层尚未初始化返回 `BTSTACK_STATUS_NOT_READY`；
- 重复开始搜索等冲突操作返回 `BTSTACK_STATUS_BUSY`；
- 无ACL连接时断开返回成功，以保持当前 AT `DSCA` 行为；
- 原生 BTstack 返回码在 GAP 操作边界转换为 SDK 状态；
- 配对完成事件的 Bluetooth 状态不进行通用错误码重写；
- GAP事件回调为空时仅更新状态，不视为错误；
- AT事件队列满的行为保持现状：计数并返回 queue-full，不在 GAP 层重试。

## 11. 测试与验收

实现采用测试先行：

1. GAP handler 重复初始化只注册一次；
2. stack state、ACL连接/断开、Inquiry结果/完成、配对请求/完成正确转换；
3. 回调被调用前，查询API已经能看到新状态；
4. Inquiry结果完整复制地址、COD、RSSI和有界名称；
5. SSP 0、1、2、3 的原生配置和确认行为与现有实现一致；
6. PIN响应使用当前 RAM PIN；
7. setter、scan、disconnect和power API正确包装原生调用及错误；
8. AT Backend不再出现原生HCI/GAP事件解析和原生操作调用；
9. Port不再注册业务HCI handler，也不直接调用 `hci_power_control()`；
10. 现有AT parser、protocol、task和Profile contract测试继续通过；
11. 新增和改动C文件通过严格告警编译检查；
12. 静态边界检查确认 `ports → SDK入口`、`AT → GAP/Profile`、`GAP → 原生BTstack` 的依赖方向。

## 12. 非目标

- 不修改 AT 命令文本、响应格式或异步事件文本；
- 不新增持久化存储；
- 不实现 BLE GAP 控制面；
- 不改变 Profile 媒体/音频数据面；
- 不重构与 GAP 分层无关的 SDK 模块。

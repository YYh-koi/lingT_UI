# xStack 渐进式解耦设计

## 目标

在不改变现有应用接口和 AT 协议文本的前提下，缩短新增蓝牙需求时的修改链路：

- 保持 `bt_xstack_init()`、`bt_xstack_send_at()`、`bt_xstack_a2dp_register_pcm_callback()` 的签名和行为兼容。
- 保持全部现有 AT 命令、`OK`/`ERROR` 响应和 `+事件` 文本兼容。
- 用通用结构化事件路由替代 AT Backend 对 Manager 的直接调用，使 Profile 事件与 AT 输出解耦。
- 将 AT 命令识别、允许形式和业务域映射集中到描述表。
- 修正 Demo 启动顺序：`AT+BTEN=1` 成功上电后，再发送 `AT+NAME=GP-BT-ATBM6165`。
- 保持 PCM `data + len` 独立直接回调，不进入通用事件路由。

## 方案选择

采用渐进式解耦。保留现有 Profile、AT Task、Backend 业务函数和 Protocol 格式化函数，只替换它们之间的耦合点。

不采用完整 Command Bus 重写，因为一次性迁移全部命令、状态和 Profile 会扩大蓝牙回归范围。不采用仅修改 Parser 的方案，因为它不能解决 Profile 事件、Manager 状态和 AT 输出之间的耦合。

## 总体架构

```text
Application
  bt_xstack_demo / bt_audio_task
             |
             v
Public API + Lifecycle
  bt_xstack_manager
             |
       +-----+-----------------------+
       |                             |
       v                             v
AT Task / Parser / Backend       PCM Direct Bridge
       |                             |
       v                             v
GAP / A2DP / HFP / AVRCP        Application PCM callback
       |
       v
typed profile callback
       |
       v
bt_xstack_event_router
       +-----------------+
       |                 |
       v                 v
Manager Sink          AT Sink
power state           AT event queue
                         |
                         v
                    AT Formatter
                         |
                         v
                  bt_event_callback_t
```

## 通用结构化事件

新增内部文件：

- `btstack/events/bt_xstack_event_router.h`
- `btstack/events/bt_xstack_event_router.c`

事件类型从 `btstack_at_internal.h` 移出，改为不绑定 AT 名称的内部类型：

```c
typedef enum {
    BT_XSTACK_EVENT_DEVSTAT = 0,
    BT_XSTACK_EVENT_PWRSTAT,
    BT_XSTACK_EVENT_SCAN_RESULT,
    /* 保留现有事件的顺序和语义 */
    BT_XSTACK_EVENT_ABSVOL
} bt_xstack_event_id_t;

typedef struct {
    bt_xstack_event_id_t id;
    uint32_t values[3];
    uint8_t value_count;
    int32_t signed_value;
    uint8_t address[6];
    uint32_t class_of_device;
    uint8_t address_type;
    char text[64];
    char text2[64];
    char text3[64];
} bt_xstack_event_t;
```

第一阶段保持字段布局与现有 `btstack_at_event_t` 一致，避免同时重写所有事件生产者和格式化代码。AT 内部可保留 typedef 和事件 ID 宏别名作为迁移兼容层，但新代码只使用 `BT_XSTACK_EVENT_*`。

Router 提供：

```c
typedef void (*bt_xstack_event_sink_t)(
    const bt_xstack_event_t *event,
    void *context);

int bt_xstack_event_router_init(void);
int bt_xstack_event_router_register(
    bt_xstack_event_sink_t sink,
    void *context);
int bt_xstack_event_router_publish(
    const bt_xstack_event_t *event);
```

实现约束：

- 使用固定长度静态 Sink 数组，不使用堆内存。
- 同一 `sink + context` 重复注册返回成功，不重复占用槽位。
- 空指针返回 `BTSTACK_STATUS_INVALID_ARGUMENT`。
- Sink 已满返回 `BTSTACK_STATUS_NO_MEMORY`。
- 注册表在临界区内复制，回调在临界区外依次执行。
- 没有 Sink 时 `publish()` 返回成功并安全丢弃事件。
- 单个 Sink 不返回状态，事件仍采用 best-effort，不阻塞 BTstack Run Loop。

Router 在 `btstack_adapter_setup()` 中初始化。Manager 在 `bt_xstack_init()` 中注册状态 Sink；AT Task 在 `btstack_at_start()` 创建完静态队列后、创建任务前注册 AT Sink，确保 Sink 可见时事件队列已经可用。

## Event Router 数据流

GAP、A2DP、HFP 和 AVRCP 仍使用当前 typed callback。第一阶段有意让 `btstack_at_backend.c` 继续承担 Profile typed event 到统一 `bt_xstack_event_t` 的归一化，以避免同时迁移所有 Profile 回调；它不再：

- 直接调用 `bt_xstack_manager_on_power_state()`；
- 直接调用 `btstack_at_post_event()`。

所有归一化事件统一调用 `bt_xstack_event_router_publish()`。

Manager Sink 只处理 `BT_XSTACK_EVENT_PWRSTAT`，更新：

```text
AT_READY -> RUN_LOOP_STARTED   首次接受 AT+BTEN=1
RUN_LOOP_STARTED -> POWERED    收到 PWRSTAT=1
POWERED -> RUN_LOOP_STARTED    收到 PWRSTAT=0，Run Loop 仍存在
非 FAILED 且 Run Loop 未启动 -> AT_READY
```

AT Sink 将事件复制到现有静态 AT Event Queue。队列满时沿用当前丢弃策略，不能在 Run Loop 中等待。

## AT 命令描述表

新增：

- `btstack/at/btstack_at_command_table.h`
- `btstack/at/btstack_at_command_table.c`

描述表集中定义：

```c
typedef enum {
    BTSTACK_AT_VALIDATE_NONE = 0,
    BTSTACK_AT_VALIDATE_UINT,
    BTSTACK_AT_VALIDATE_TEXT,
    BTSTACK_AT_VALIDATE_ADDRESS,
    BTSTACK_AT_VALIDATE_UINT_LIST,
    BTSTACK_AT_VALIDATE_DOMAIN_SPECIFIC
} btstack_at_validation_kind_t;

typedef struct {
    const char *name;
    btstack_at_command_id_t id;
    uint8_t allowed_forms;
    btstack_at_validation_kind_t validation;
} btstack_at_command_descriptor_t;
```

`allowed_forms` 使用 execute/query/set 位掩码。Parser 的职责变为：

1. 分离命令名、形式和参数文本；
2. 在描述表中查找命令；
3. 拒绝描述表未允许的形式；
4. 调用现有参数解析辅助函数完成校验和填充。

复杂命令的专用参数解析保留在 `btstack_at_parser.c`，本次不强制统一成通用参数模型。新增普通命令只需要新增描述表项和所属业务域处理；只有新的参数形状才需要扩展 Parser。

## AT 业务域分发表

新增：

- `btstack/at/btstack_at_dispatch_table.h`
- `btstack/at/btstack_at_dispatch_table.c`

表将命令 ID 映射到：

```c
typedef enum {
    BTSTACK_AT_DOMAIN_GENERAL = 0,
    BTSTACK_AT_DOMAIN_HFP,
    BTSTACK_AT_DOMAIN_A2DP_AVRCP
} btstack_at_command_domain_t;
```

Backend 根据表结果直接调用现有的 General、HFP 或 A2DP/AVRCP 执行函数，去掉当前依靠返回值 `1` 逐级尝试的链式分发。各业务域内部的命令 switch 第一阶段保留，以控制改动规模。

Protocol 中简单的单值事件名称使用表驱动；扫描、配对、设备地址、曲目信息等复杂格式继续使用现有专用 formatter，输出字节必须保持不变。

## Manager 边界

`bt_xstack_manager.c` 继续负责：

- 对外 API；
- 一次性初始化和配置冲突检查；
- `AT+BTEN` Command Gate；
- 首次 `BTEN=1` 时创建 BTstack Run Loop Task；
- Manager Event Sink 和内部状态迁移；
- PCM callback 的注册和轻量桥接。

Manager 不解析 GAP/A2DP/HFP/AVRCP 业务事件，不依赖 AT Backend 的事件回调实现。PCM 独立桥接不纳入 Router，因为 PCM 高频、数据量大且已有明确的单消费者接口。

## Demo 启动状态机

`bt_xstack_demo.c` 增加内部启动状态：

```text
IDLE
  -> WAIT_POWER       提交 AT+BTEN=1 成功
  -> SETTING_NAME     收到 +PWRSTAT=1，提交 AT+NAME
  -> STARTED          NAME 提交成功
```

具体流程：

1. `bt_xstack_init()`；
2. 注册电话按键处理；
3. 启动 `bt_audio_task`；
4. 注册 PCM callback；
5. 只提交 `AT+BTEN=1`；
6. Event callback 收到完整的 `+PWRSTAT=1` 行后，只提交一次 `AT+NAME=GP-BT-ATBM6165`；
7. NAME 入队成功后进入 `STARTED`。

重复 `+PWRSTAT=1` 在 `SETTING_NAME` 或 `STARTED` 状态下不重复发送 NAME。收到 `+PWRSTAT=0` 时清除电话状态并回到 `WAIT_POWER`，允许下一次上电后重新设置名称。

BTEN 提交失败时保持 `IDLE` 并返回错误，调用方可再次调用 `bt_xstack_demo_start()`。NAME 提交失败时保持 `WAIT_POWER`；后续再次收到 `+PWRSTAT=1` 时重试。两类失败都记录日志，不停止已经启动的音频任务，也不新增重试线程或定时器。

默认 Property 名称仍为 `GP-BT-ATBM6165`，因此 HCI 初始化期间已经有正确的兜底名称。

## 兼容性

- 不修改正式交付头文件 `gplib/liba_wifi_src/atbm/atbm6165/bt/bt_xstack.h`。
- 不新增对外事件类型；结构化 Router 完全属于 `libxstack.a` 内部。
- AT 命令 ID 数值、事件顺序和文本输出保持不变。
- `AT+BTEN=1` 仍是启动 Run Loop 和调用 `hci_power_control(HCI_POWER_ON)` 的唯一入口。
- H5、UART0、HCI、L2CAP、Classic Profile、SBC 解码和 PCM callback 路径不改变。
- 完整 HCI dump 保持关闭，独立断链 reason 日志保持有效。

## 构建集成

新源文件加入：

- `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
- `output/wireless_libs/build/Xstack_Library_Debug.mk`

重新从空对象目录生成 `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`，确认没有旧 archive member。随后重新链接主工程 ELF。

## 测试策略

严格按 TDD 分阶段实施：

1. Router 单元测试先失败，再实现注册、去重、发布、空 Sink 和容量边界。
2. Parser 描述表测试覆盖全部现有命令名、允许形式和原有正反例。
3. Dispatch 表测试确保每个命令映射到唯一正确业务域。
4. Protocol 回归测试逐字节比较现有 AT 响应和事件文本。
5. Manager 契约测试验证 Backend 不再直接调用 Manager，Manager 通过 Router 接收 PWRSTAT。
6. Demo 契约测试验证源码顺序和状态：先 BTEN，收到 PWRSTAT=1 后只发送一次 NAME。
7. 运行现有 AT parser/protocol/backend/task、GPCV2178 lifecycle、HCI reset、A2DP PCM、HFP key 和 Demo 合同测试。
8. 干净构建 `libxstack.a` 和主工程，检查最终 ELF 保留断链日志且没有完整 HCI dump 字符串。

## 非目标

- 不重写现有 Profile 实现。
- 不改变 AT 命令语法或对外 callback。
- 不把 PCM 放入通用 Event Router。
- 不新增动态内存、额外长期任务或新的应用线程。
- 不在本阶段把每条 Backend switch case 拆成独立 handler。
- 不新增 BLE 应用 Profile。

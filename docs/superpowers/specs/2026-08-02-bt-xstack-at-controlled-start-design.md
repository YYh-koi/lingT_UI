# BT xStack AT 控制启动设计

## 目标

为上层提供与 ATBM 和 GPCV2178 名称无关的蓝牙入口。系统调用
`bt_xstack_init()` 后只完成协议栈和 AT 控制面的初始化；首次收到
`AT+BTEN=1` 后才启动 BTstack run loop、打开 H5/UART0 transport 并给
HCI 上电。

本阶段的成功标准是：

- 上层只能通过 `bt_xstack_init()` 和 `bt_xstack_send_at()` 使用蓝牙；
- 初始化完成后 UART0 未打开、HCI 保持 OFF、run loop 尚未执行；
- `AT+BTEN=1` 能启动 run loop 并发起 HCI 上电；
- `AT+BTEN=0` 能关闭 HCI/UART0，但保留 run-loop 任务；
- AT 响应和底层异步 AT 事件统一通过 `bt_event_callback_t` 返回；
- BTstack 静态库和主工程 Debug 目标能够完成编译、归档和链接。

## 公共接口

新增公共头文件 `btstack/include/adapter/bt_xstack.h`：

```c
typedef void (*bt_event_callback_t)(
    const char *data,
    uint16_t length,
    void *context);

int bt_xstack_init(
    bt_event_callback_t event_callback,
    void *context);

int bt_xstack_send_at(
    const char *command,
    uint16_t length);
```

`event_callback` 是上层的统一 AT 输出回调。它接收命令响应，例如
`OK`、`ERROR` 和查询结果，也接收 `+PWRSTAT`、`+DEVSTAT`、连接状态、
A2DP、AVRCP、HFP 等异步事件。它不接收 HCI UART 原始数据。

回调在 AT 任务上下文执行。`data` 只在回调期间有效；需要延迟处理时，
上层必须复制数据。`bt_xstack_send_at()` 同样不持有上层命令缓冲区，而是
将一条完整命令复制到内部队列。

公共状态码保持稳定的负值错误语义，并由 `bt_xstack.h` 声明：参数非法、
未初始化、初始化配置冲突、命令过长、输入队列已满、未就绪和内部初始化
失败。`0` 表示调用成功。`bt_xstack_send_at()` 返回 `0` 只表示命令成功
入队，最终命令结果仍通过 `event_callback` 返回。

## 方案选择

采用“AT 分发前生命周期门控”方案。

AT frontend 在解析出 `BTEN` 后、调用正式 AT backend 前，先调用由
`bt_xstack_manager.c` 注册的生命周期门控。首次 `BTEN=1` 时，门控创建
run-loop 任务；任务启动后，原始 `BTEN` 命令继续走现有 backend，并在
BTstack run-loop 上下文执行 `hci_power_control(HCI_POWER_ON)`。

不采用运行时切换临时/正式 backend 的方案，避免 dispatcher 切换竞态；
也不在 `bt_xstack_init()` 中提前执行空闲 run loop，因为这不符合由
`BTEN` 启动 run loop 的要求。

## 组件职责

### `bt_xstack_manager.c`

新增 `btstack/adapter/bt_xstack_manager.c`，不创建 `bt_xstack.c`。Manager
负责：

- 保存 `bt_event_callback_t`、callback context 和公共初始化配置；
- 编排 port、BTstack core、Profile、AT backend 和 AT frontend 的初始化；
- 维护初始化、run loop、HCI 电源请求和 BTEN 过渡状态；
- 保证重复初始化和重复 `BTEN` 不会重复创建任务或重复打开 transport；
- 将 `bt_xstack_send_at()` 转发到 AT 输入队列；
- 为 AT frontend 提供 `BTEN` 生命周期门控。

Manager 的内部状态至少包含：

```text
UNINITIALIZED
AT_READY
RUN_LOOP_STARTED
POWERED
FAILED
```

其中 `POWERED` 表示 BTstack 已收到 stack-working 状态，而不是仅表示
`hci_power_control()` 已返回。上电初始化期间由内部过渡标志阻止并发的
重复上/下电命令。Manager 状态在普通 FreeRTOS 任务之间共享，更新时使用
现有 port 临界区能力保护，不允许在 ISR 中调用公共接口。

### GPCV2178 port

重构 `btstack/ports/gpcv2178/btstack_gpcv2178_port.c`，将当前单一任务入口
拆成两个内部阶段：

1. 协议栈初始化：设置 port ops、初始化 run-loop 数据结构和 BTstack
   memory、选择并注册 H5/UART0 transport、初始化 HCI、设置 control、
   初始化 L2CAP、adapter、Classic profiles 和 AT backend。
2. run-loop 启动：只创建一次 BTstack 任务，并在任务中执行
   `btstack_run_loop_execute()`。

`hci_init()` 只注册 H5/UART0 transport。按照 BTstack 原生生命周期，实际
UART0 driver 初始化、transport open 和 H5 收发启动仍由
`hci_power_control(HCI_POWER_ON)` 触发。因此 `bt_xstack_init()` 完成
H5/UART0 的配置和注册，但不会提前占用或打开 UART0。

### AT frontend 与 backend

现有 parser、格式化、输入队列和事件队列继续复用。AT frontend 增加一个
内部 BTEN 门控回调，不把平台名称暴露到公共 AT API。

门控约定：

- 首次 `BTEN=1`：启动 run loop，成功后继续交给正式 backend；
- run loop 已启动时的 `BTEN=1`：直接继续交给 backend，由 HCI 状态机保证
  幂等；
- run loop 已启动时的 `BTEN=0`：继续交给 backend 执行 HCI 下电；
- run loop 尚未启动时的 `BTEN=0`：作为已满足的关闭请求直接返回 `OK`，
  不创建 run-loop 任务；
- run-loop 任务创建失败：返回 `ERROR`，HCI 保持 OFF，允许下一条
  `BTEN=1` 再次尝试。

正式 backend 仍在 BTstack run-loop 上下文执行命令。除基础 `AT` 和
`BTEN` 外，首次成功 `BTEN=1` 前的其他命令返回 `ERROR`，避免将命令排入
尚未执行的 run loop 后等待超时。

### 构建与产品工程

- BTstack 静态库工程加入 `bt_xstack_manager.c` 和公共头文件路径；
- `btstack/adapter/Makefile.inc` 同步加入 Manager 源文件；
- 主工程 Debug 目标恢复一次 `libbtstack.a` 链接；
- 删除 `Project/wifi_demo/atbm_wifi_probe_demo.c` 中依附 WiFi AP 的 BTstack
  自动启动代码，不再由 WiFi 模式切换隐式启动蓝牙；
- 保持 `C_BTSTACK_ENABLE` 为构建开关，但不将任何 ATBM 名称暴露到新的
  公共接口。

## 数据流

初始化数据流：

```text
application
  -> bt_xstack_init(callback, context)
  -> bt_xstack_manager
  -> GPCV2178 port static initialization
  -> HCI/L2CAP/Profile/AT backend initialization
  -> AT frontend task start
  -> AT_READY
```

首次上电数据流：

```text
application
  -> bt_xstack_send_at("AT+BTEN=1")
  -> AT input queue/parser
  -> BTEN lifecycle gate
  -> create BTstack run-loop task once
  -> normal AT backend dispatch
  -> hci_power_control(HCI_POWER_ON)
  -> UART0/H5 init and open
  -> command response and asynchronous state events
  -> bt_event_callback_t
```

下电和恢复数据流：

```text
AT+BTEN=0 -> HCI power off -> UART0 close -> run loop retained
AT+BTEN=1 -> HCI power on  -> UART0 reopen -> same run loop reused
```

## 错误处理与幂等性

- `bt_xstack_init(NULL, ...)` 返回参数错误，不产生任何任务；
- 相同 callback/context 的重复初始化返回成功，不重复初始化；
- 已初始化后使用不同 callback/context 再次初始化返回配置冲突；
- 初始化任一内部阶段失败时进入 `FAILED`，不启动 AT 任务；为避免对部分
  初始化的 BTstack core 重复初始化，恢复方式是修正原因后重启系统；
- `bt_xstack_send_at()` 在未初始化、参数非法、命令过长或队列已满时立即
  返回对应错误；
- run-loop 任务句柄和状态在任务创建成功后原子更新，重复 `BTEN=1` 不会
  创建第二个任务；
- HCI 上电失败只返回该命令 `ERROR`，run loop 保持运行，后续允许重试；
- `BTEN=0` 只关闭 HCI transport，不销毁 AT 任务或 run-loop 任务；
- 回调不得阻塞较长时间，也不得从 ISR 调用 `bt_xstack_send_at()`。

## 测试与验证

### 主机侧接口和契约测试

- 验证公共头文件只暴露通用名称；
- 验证 Manager 文件名为 `bt_xstack_manager.c`，且不存在新增的
  `bt_xstack.c`；
- 验证空 callback、未初始化发送、零长度、超长命令和队列满错误；
- 验证相同配置重复初始化幂等，不同配置被拒绝；
- 验证 `AT` 可在 HCI OFF 时返回 `OK`；
- 验证首次上电前其他业务命令返回 `ERROR`；
- 验证未启动时 `BTEN=0` 返回 `OK` 且不创建 run-loop 任务；
- 验证首次和重复 `BTEN=1` 只创建一个 run-loop 任务；
- 验证响应和异步事件均经过 `bt_event_callback_t`。

### 构建验证

1. 运行现有 AT parser、protocol、task、backend 和 GAP/Profile 契约测试；
2. 重建 `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`；
3. 使用工具链 `nm` 确认 `libbtstack.a` 导出 `bt_xstack_init` 和
   `bt_xstack_send_at`；
4. 确认主工程只链接一次 `libbtstack.a`；
5. 完成主工程 Debug 目标构建并检查无未解析符号。

### 板端验证

`bt_xstack_init()` 后确认没有 UART0 open 和 HCI power-on 日志。发送
`AT+BTEN=1` 后，期望关键顺序为：

```text
bt_xstack init done
AT+BTEN=1
run loop task started
HCI power on
UART0/H5 opened
OK
+PWRSTAT:1
```

随后发送 `AT+BTEN=0`，确认 UART0 关闭但 run-loop 任务仍存在；再次发送
`AT+BTEN=1`，确认复用同一任务并恢复 HCI。最后至少执行一次地址查询、
Classic inquiry 或 Profile 状态查询，证明命令不是只完成了解析，而是已由
实际 BTstack backend 执行。

## 非目标

本阶段不新增 BLE Profile，不改变 H5 协议参数，不实现 run-loop 任务销毁，
不把原始 HCI 数据暴露给上层，也不将蓝牙生命周期重新绑定到 WiFi AP、
STA 或 P2P 模式。

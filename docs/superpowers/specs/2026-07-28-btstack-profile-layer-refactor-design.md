# BTstack 通用 Classic Profile 分层重构设计

## 1. 目标

将当前位于 `btstack/ports/gpcv2178` 的通用 Classic Profile 和 AT 后端逻辑
迁移到 `btstack/profiles` 与 `btstack/at`，使 GPCV2178 port 只负责平台接入：

- FreeRTOS BTstack 任务创建
- BTstack run-loop
- UART/HCI transport
- 控制器电源、复位和波特率控制
- 平台存储
- 调用通用模块的初始化入口

重构不得改变已确认的 BARROT AT 命令、响应、事件文本、队列模型和首期功能
范围。

## 2. 当前问题

当前结构存在两个并行且不一致的 Profile 层：

- `btstack/profiles/btstack_hfp_hf.c` 和
  `btstack/profiles/btstack_a2dp_sink.c` 是通用接口，但控制函数仍返回
  `BTSTACK_STATUS_NOT_SUPPORTED`。
- `btstack/ports/gpcv2178/btstack_gpcv2178_profiles.c` 直接完成 A2DP、HFP、
  AVRCP 初始化、SDP 注册、状态管理和原始事件处理。
- `btstack/ports/gpcv2178/btstack_at_gpcv2178.c` 同时实现 General/GAP、
  HFP、A2DP、AVRCP 操作和 BARROT 事件映射。

结果是通用 Bluetooth 业务依赖具体 port，通用 Profile API 不能被 AT 后端或
其他应用真正复用。

## 3. 目标目录和职责

```text
btstack/ports/gpcv2178
  btstack_gpcv2178_port.c
  btstack_run_loop_gpcv2178.c
  btstack_uart_gpcv2178.c
  btstack_control_gpcv2178.c
  btstack_storage_gpcv2178.c
  btstack_config.h

btstack/profiles
  btstack_classic_profiles.c
  btstack_hfp_hf.c
  btstack_a2dp_sink.c
  btstack_avrcp.c

btstack/include/profiles
  btstack_classic_profiles.h
  btstack_hfp_hf.h
  btstack_a2dp_sink.h
  btstack_avrcp.h

btstack/at
  btstack_at.c
  btstack_at_parser.c
  btstack_at_protocol.c
  btstack_at_backend.c
```

### 3.1 GPCV2178 port

Port 层只初始化平台资源和通用模块。它可以构造
`btstack_classic_profiles_config_t`，但不能直接：

- 调用 `hfp_hf_*`
- 调用 `a2dp_sink_*`
- 调用 `avrcp_*` 或 `avrcp_controller_*`
- 解析 HFP/A2DP/AVRCP meta event
- 创建 Profile SDP record
- 保存 Profile CID、连接状态、播放状态或媒体元数据
- 生成 BARROT AT 事件

### 3.2 Classic Profile 组合层

`btstack_classic_profiles.c` 负责：

- `sdp_init()` 和 `rfcomm_init()`
- 按顺序初始化 A2DP Sink、AVRCP CT/TG 和 HFP HF
- 传递设备名、厂商名、Class of Device、SDP handle 和 RFCOMM channel
- 注册各 Profile SDP record
- 设置初始 GAP Classic connectable/discoverable 配置
- 提供一个统一的 `btstack_classic_profiles_setup()` 生命周期入口

组合层不解析 AT 命令，也不生成 BARROT 文本。

### 3.3 HFP HF 层

`btstack_hfp_hf.c` 负责：

- 初始化 HFP HF、codec、feature、RFCOMM channel 和 SDP record
- 保存 ACL handle、对端地址、SLC/SCO 状态、呼叫指标和号码
- 实现连接、断开、拨号、重拨、DTMF、接听、挂断、多方通话、SCO 和语音
  识别控制
- 实现扬声器增益控制
- 解析所有 HFP meta event
- 通过通用 HFP Profile 事件回调发布结构化事件

现有 `btstack_hfp_hf_*` 公共函数保留，并从占位实现变为真实 BTstack 实现。
新增连接、断开及缺失控制接口，不把 AT command ID 暴露到 Profile 层。

### 3.4 A2DP Sink 层

`btstack_a2dp_sink.c` 负责：

- 初始化 A2DP Sink、SBC endpoint 和 SDP record
- 保存 A2DP CID、对端地址和流状态
- 实现连接与断开
- 解析 signaling、stream established、started、suspended、released event
- 通过通用 A2DP Profile 事件回调发布结构化事件
- 将媒体 packet 交给已注册的 media callback；callback 为空时直接丢弃

首期仍不解码或播放媒体。

### 3.5 AVRCP 层

新增 `btstack_avrcp.c/.h`，负责：

- 初始化 AVRCP core、Controller、Target 和两个 SDP record
- 保存 AVRCP CID、对端地址、连接状态、播放状态、repeat/shuffle、绝对音量
  和媒体元数据
- 实现播放、暂停、停止、下一首、上一首、快进、快退、repeat、shuffle 和
  音量控制
- 管理 playback、track、metadata 和 volume notification
- 使用 BTstack run-loop timer 查询播放进度
- 解析 common、Controller 和 Target meta event
- 通过通用 AVRCP Profile 事件回调发布结构化事件

浏览、封面和 BIP 功能仍不实现。

## 4. 通用 Profile 接口

Profile 层提供与 AT 无关的操作和事件类型。例如：

```c
int btstack_hfp_hf_connect(const btstack_bd_addr_t address);
int btstack_hfp_hf_disconnect(void);
int btstack_hfp_hf_control(btstack_hfp_hf_control_t control);
int btstack_hfp_hf_dial_number(const char *number);

int btstack_a2dp_sink_connect(const btstack_bd_addr_t address);
int btstack_a2dp_sink_disconnect(void);
btstack_a2dp_sink_state_t btstack_a2dp_sink_get_state(void);

int btstack_avrcp_control(btstack_avrcp_control_t control);
int btstack_avrcp_set_repeat(bool enabled);
int btstack_avrcp_set_shuffle(bool enabled);
int btstack_avrcp_set_progress_interval(uint8_t seconds);
```

每个 Profile 定义自己的结构化事件枚举和回调。事件包含该 Profile 所需的
地址、状态、数值和有界文本，但不包含：

- BARROT 字符串
- AT command ID
- FreeRTOS queue
- GPCV2178 类型或宏

现有 adapter application event API 保持兼容。新增的低层 Profile 回调用于
AT backend 和需要完整 Classic 状态的其他模块。

## 5. 通用 AT 后端

将 `btstack_at_gpcv2178.c` 重构为 `btstack_at_backend.c`。

它负责：

- General/GAP 命令、RAM 配置和 RAM 配对列表
- 通过 `btstack_run_loop_execute_on_main_thread()` 执行标准化命令
- 将 HFP/A2DP/AVRCP command ID 映射到通用 Profile API
- 订阅通用 Profile 结构化事件
- 将结构化 Profile 事件映射为 `btstack_at_event_t`
- 把事件复制到 AT 事件队列

它不再包含原生 `hfp_hf_*`、`a2dp_sink_*`、`avrcp_controller_*` 调用，也不
直接解析任何 Profile meta packet。

公共接口保持不变：

```c
int btstack_at_init(const btstack_at_config_t *config);
int btstack_at_start(void);
int btstack_at_submit(const char *command, uint16_t length);
```

## 6. 数据流和线程边界

### 6.1 命令

```text
上层 FreeRTOS 任务
  -> btstack_at_submit()
  -> AT 输入队列
  -> btstack_at 任务解析
  -> btstack_run_loop_execute_on_main_thread()
  -> BTstack 任务
  -> btstack_at_backend
  -> 通用 HFP/A2DP/AVRCP API
  -> 原生 BTstack API
  -> 完成队列
  -> btstack_at 任务输出响应
```

### 6.2 事件

```text
原生 BTstack Profile callback
  -> 对应通用 Profile 文件解析并更新状态
  -> 通用 Profile 结构化事件回调
  -> btstack_at_backend 映射
  -> AT 事件队列
  -> btstack_at 任务去重和格式化
  -> 统一 output_callback
```

所有原生 BTstack API 和 Profile 状态仍只在 BTstack run-loop 任务中访问。
只有 `btstack_at` 任务可以调用 AT 输出回调。

## 7. 配置和状态归属

- 设备名、厂商名、COD、SDP handle 和 RFCOMM channel：
  `btstack_classic_profiles_config_t`
- GAP AT 配置、PIN、SSP、TDL 和 RAM 配对列表：
  `btstack_at_backend.c`
- HFP handle、状态、号码和对端地址：
  `btstack_hfp_hf.c`
- A2DP CID、stream 状态和对端地址：
  `btstack_a2dp_sink.c`
- AVRCP CID、播放状态、模式、音量、元数据和 timer：
  `btstack_avrcp.c`
- AT 文本队列、响应顺序和状态事件去重：
  `btstack_at.c`

不在多个层中保存可独立变化的同一份 Profile 状态。AT 查询通过 Profile
getter 获取 BTstack 任务中的当前值，并把值复制到命令结果快照。

## 8. 错误处理

- 通用 Profile API 使用 `BTSTACK_STATUS_*`。
- Profile 未初始化返回 `BTSTACK_STATUS_NOT_READY`。
- 未连接时的连接相关控制返回 `BTSTACK_STATUS_NOT_READY`。
- 原生 BTstack 非零状态统一转换为 `BTSTACK_STATUS_ERROR` 或更具体状态。
- AT backend 将非成功状态转换为 BARROT `ERROR`。
- Profile 回调不得阻塞；AT 事件队列满时继续使用现有丢弃和溢出计数策略。
- 明确请求的断开不会生成 `+HFPLOST` 或 `+A2DPLOST`。

## 9. 迁移范围

删除 port 中的通用实现及工程单元：

- `btstack/ports/gpcv2178/btstack_gpcv2178_profiles.c`
- `btstack/ports/gpcv2178/btstack_gpcv2178_profiles.h`
- `btstack/ports/gpcv2178/btstack_at_gpcv2178.c`
- `btstack/ports/gpcv2178/btstack_at_gpcv2178.h`

新增：

- `btstack/profiles/btstack_classic_profiles.c`
- `btstack/include/profiles/btstack_classic_profiles.h`
- `btstack/profiles/btstack_avrcp.c`
- `btstack/include/profiles/btstack_avrcp.h`
- `btstack/at/btstack_at_backend.c`
- Profile 层契约和行为测试

修改：

- `btstack/profiles/btstack_hfp_hf.c`
- `btstack/include/profiles/btstack_hfp_hf.h`
- `btstack/profiles/btstack_a2dp_sink.c`
- `btstack/include/profiles/btstack_a2dp_sink.h`
- `btstack/ports/gpcv2178/btstack_gpcv2178_port.c`
- `btstack/adapter/Makefile.inc`
- GPCV2178 工程文件
- AT 后端契约测试和 Profile 接线检查

## 10. 兼容性

重构后必须保持：

- 已支持 AT 命令集合不变
- BARROT 响应和事件字节格式不变
- `<FF>` 仍为 ASCII `0x0C`
- 命令响应先于命令期间产生的异步事件
- 上层调用接口不变
- RAM 配置和重启恢复默认值策略不变
- Profile mask 仍为 424
- A2DP 媒体仍不播放
- HFP SCO 仍不接麦克风和扬声器
- BLE 不纳入本次迁移

## 11. 验证

验证分为四层：

1. Profile 单元和契约测试
   - 初始化、状态转换、连接前置条件和原始事件映射
   - HFP、A2DP、AVRCP 各自独立验证
2. AT 回归测试
   - 解析、格式化、队列、响应顺序和事件去重全部保持
3. 分层检查
   - `ports/gpcv2178` 不出现原生 Profile 操作或 Profile meta event 解析
   - `btstack_at_backend.c` 不出现原生 Profile API
4. 目标编译与工程检查
   - ARM Cortex-A7 `-Wall -Wextra -Werror`
   - GPC 工程 XML 和全部新源文件单元
   - 现有 Classic Profile 和 AT 集成脚本

板端验收继续覆盖发现、配对、HFP、A2DP、AVRCP 和异步事件顺序。

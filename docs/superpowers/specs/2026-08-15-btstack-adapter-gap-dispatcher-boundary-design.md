# BTstack Adapter、GAP 与 Dispatcher 边界设计

## 目标

在不改变公共 API/AT ABI、任务模型和队列容量的前提下，收紧公共层内部边界：

- `btstack_adapter` 成为 GAP 的唯一公共层入口。
- `gap/` 保留为私有实现，不合并成单个大文件。
- `btstack_core_dispatcher` 只负责请求生命周期、线程切换和路由。
- Profile 功能逻辑回到各自 Profile 模块。
- 跨 Profile 状态与联动统一放入 `btstack_profile_manager`。

本次不新增任务、队列、动态内存、运行时注册表或函数表。

## 非目标

- 不改变 `bt_xstack_*` 公共函数、事件或结构体 ABI。
- 不改变 API/AT 二选一构建方式。
- 不重写 GAP、HFP、A2DP、AVRCP、PBAP 状态机。
- 不把所有功能集中到 `btstack_adapter.c` 或 Profile Manager。
- 不在本次实现 LE Profile 业务，只保留已有 LE/GAP 边界。

## 选定方案

采用明确的小接口，不采用统一的 `execute(request)` 大开关。

被否决的方案：

1. 把全部 GAP 实现移入 `btstack_adapter.c`：文件会同时承担生命周期、链路、安全、扫描、广播数据和状态机，职责过重。
2. 给 adapter 增加统一请求结构和大 `switch`：只是把 dispatcher 搬了一个位置，边界没有变清晰。

## 组件职责

### btstack_adapter

adapter 是公共层访问控制器/GAP 能力的唯一入口，负责：

- property、event、GAP、bond 等基础模块的 setup/deinit。
- 提供类型明确的 GAP 操作接口。
- 订阅私有 GAP 事件，并通过中性的 adapter 事件回调交给 event mapper。
- 隔离底层 BTstack GAP 类型与其他公共层模块。

建议的操作接口按现有能力提供，不引入通用 request 参数：

```c
int btstack_adapter_set_power(bool enabled);
int btstack_adapter_set_name(const char *name);
int btstack_adapter_set_pin(const char *pin);
int btstack_adapter_set_ssp_mode(uint8_t mode);
int btstack_adapter_pairing_confirm(const btstack_bd_addr_t address, bool accept);
int btstack_adapter_set_discoverable(bool enabled);
int btstack_adapter_set_connectable(bool enabled);
int btstack_adapter_set_class_of_device(uint32_t class_of_device);
int btstack_adapter_scan_start(uint8_t duration);
int btstack_adapter_scan_stop(void);
int btstack_adapter_disconnect(void);
bool btstack_adapter_is_working(void);
int btstack_adapter_get_local_address(btstack_bd_addr_t address);
```

adapter 接口只做参数边界、底层调用和状态返回，不承载 Profile 策略。

### gap/

`gap/` 继续按职责拆分：

- `btstack_gap.c`：GAP 生命周期、底层 HCI/GAP 事件和状态。
- `btstack_gap_link.c`：名称、COD、可连接、可发现和基础链路断开。
- `btstack_gap_security.c`：PIN、SSP 和配对确认。
- `btstack_gap_scan.c`：扫描状态机。
- `btstack_gap_ad_data.c`：LE 广播数据构造。
- `btstack_gap_adv.c`、`btstack_gap_privacy.c`：LE GAP 功能边界。

这些接口保留为库内私有实现。生产代码中仅 `gap/` 自身和 `btstack_adapter.c` 可以直接包含 `gap/*.h`。GAP 头文件不进入客户打包公共头集合。

### btstack_core_dispatcher

dispatcher 只保留：

- 固定 command slot 的申请、复制和释放。
- local 请求与 Stack Run Loop 请求的上下文选择。
- 将请求路由到 adapter、Profile 或 Profile Manager。
- 记录 command count、failure count 和 slot 使用水位。
- 完成后通知等待 command slot 的 worker。

每个分支只做必要的字段提取和一次目标模块调用。dispatcher 不读取 Profile 状态来决定功能语义，也不保存扫描、播放、音频或连接策略。

### 各 Profile 模块

- HFP：拨号、重拨、DTMF、接听、挂断、通话控制、音频和音量策略。
- A2DP：连接、断开、静音及媒体状态。
- AVRCP Controller：播放控制、play/pause 默认判断、repeat 和 shuffle。
- PBAP：同步、认证、取消、事件空间恢复和 peer disconnected。

Profile 对外提供明确的高层操作函数；dispatcher 不再翻译 Profile 内部控制枚举或复制其状态机判断。

### btstack_profile_manager

Profile Manager 只负责：

- Classic Profile 的初始化、启用掩码和状态汇总。
- Connect Last、公共 peer 选择和断开全部 Profile。
- HFP 到 PBAP 等跨 Profile 联动。
- Profile 事件协调及向 event mapper 转交中性事件。
- 保留 LE Profile 管理入口，但本次不实现 LE Profile 业务。

单一 Profile 的拨号、播放、同步等功能不进入 Profile Manager。

### btstack_event_mapper

event mapper 只接收 adapter/Profile Manager 提供的中性事件，转换成 API 或 AT 使用的公共事件格式。它不直接包含 GAP 私有头，不调用 GAP 操作，也不执行跨 Profile 策略。

## 依赖方向

```text
API / AT
   |
   v
btstack_core_dispatcher
   |-- GAP 请求 ----------> btstack_adapter ------> gap/*
   |-- 单 Profile 请求 ---> HFP/A2DP/AVRCP/PBAP
   `-- 跨 Profile 请求 ---> btstack_profile_manager

gap/* --private event--> btstack_adapter
profile events ---------> btstack_profile_manager
adapter/manager --------> btstack_event_mapper --------> API/AT event path
```

禁止反向依赖：

- `gap/` 不依赖 dispatcher、Profile Manager、API 或 AT。
- adapter 不依赖 dispatcher、API 或 AT。
- event mapper 不直接依赖 `gap/*.h`。
- Profile 不调用 dispatcher。

## 请求流

1. API worker 或 AT worker 构造 `btstack_core_request_t`。
2. dispatcher 判断请求是否必须进入 Stack Run Loop。
3. 异步请求复制到固定 command slot 后投递，调用方内存不再被引用。
4. Stack Run Loop 中的 dispatcher 分支只调用一个目标模块接口。
5. 目标模块返回原始统一状态；dispatcher 只统计和释放 slot，不改变业务结果。

示例：

```text
SET_NAME      -> dispatcher -> btstack_adapter_set_name()
HFP_CONTROL   -> dispatcher -> btstack_hfp_hf_execute_control()
AVRCP_CONTROL -> dispatcher -> btstack_avrcp_controller_execute()
PBAP_SYNC     -> dispatcher -> btstack_pbap_client_sync()
DISCONNECT_ALL-> dispatcher -> btstack_profile_manager_disconnect_all()
```

## 事件流

GAP 私有事件由 adapter 订阅。adapter 只把底层事件转换成紧凑的中性 adapter 事件，再调用已注册回调；event mapper 完成公共事件映射。

Profile 文件继续订阅自己的底层事件。Profile 内部更新自身状态后通知 Profile Manager；Manager 只做状态汇总和跨 Profile 联动，再把中性事件交给 event mapper。

所有应用 callback 继续走现有 API Event Worker 或 AT Event Ring，不在 Stack Run Loop 中直接调用应用。

## 错误与并发语义

- adapter/Profile 参数错误直接返回统一错误码，不由 dispatcher 重解释。
- command slot 满时保持现有 BUSY/QUEUE_FULL 行为。
- dispatcher 不增加重试；需要状态机重试的功能由所属模块处理。
- 所有异步请求继续深拷贝，禁止保存调用方指针。
- 现有 platform critical、worker signal、ring 和回压语义保持不变。
- 初始化失败继续按 runtime 的既有回滚顺序处理。

## 内存与构建要求

- 不增加静态队列、ring、worker、任务栈或堆分配。
- 不扩大公共事件 envelope。
- 不为路由增加函数表或注册数组；使用直接调用和简单 `switch`。
- API、Public AT、Legacy AT 三种归档继续包含恰好一个 adapter、dispatcher、event mapper 和 Profile Manager 实现。
- GAP 拆分文件数量本身不计为运行时内存；保留独立编译单元便于按段裁剪。

## 测试与验收

1. 增加依赖边界合同：除 `gap/` 和 adapter 外，生产代码不得包含 `gap/*.h`。
2. dispatcher 合同验证其只包含调度、slot、路由和诊断，不含 GAP/Profile 策略判断。
3. adapter 单测覆盖每个明确接口的参数、转发、状态和 GAP 事件转换。
4. Profile 单测覆盖迁回的 HFP、A2DP、AVRCP、PBAP 功能逻辑。
5. Profile Manager 单测覆盖 Connect Last、disconnect all 和 HFP→PBAP 联动。
6. API/AT parity 合同验证相同请求仍到达相同目标模块。
7. 全部现有 API、AT、Profile、core 和 transport 测试保持通过。
8. API/AT/Legacy 三套静态库及主固件重新生成并构建，公共符号集合和打包头 ABI 不变。

## 完成标准

- dispatcher 中不存在直接 GAP 底层调用和 Profile 策略实现。
- core、API、AT、Profile Manager、event mapper 均不直接包含 GAP 私有头。
- adapter 是 GAP 唯一公共层入口，但不包含 GAP 状态机实现。
- 单 Profile 功能由所属模块负责，跨 Profile 联动仅由 Profile Manager 负责。
- 无新增运行时资源，公共 ABI、API/AT 行为及构建变体保持兼容。

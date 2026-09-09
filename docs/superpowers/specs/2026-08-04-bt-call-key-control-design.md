# 蓝牙通话 KEY1/KEY2 控制设计

## 目标

在现有 `bt_xstack_demo` 中增加蓝牙 HFP 通话按键控制：

- 来电时短按 KEY1 接听，发送 `AT+HFPANSW`。
- 来电或通话过程中短按 KEY2 拒接/挂断，发送 `AT+HFPCHUP`。
- 无相关通话状态时，KEY1、KEY2 保留原有短按功能。
- KEY1、KEY2 的长按功能保持不变。
- 按键公共模块不得直接依赖 BTstack、AT 指令或特定芯片厂商名称。

## 硬件按键映射

根据 `GPA7XXXA EMU V2.2.pdf` 第 10 页 AD_KEY 电阻网络及当前 ADC 阈值：

- KEY1 对应 `ADKEY_OK`，进入 `ap_peripheral_ok_key_exe()`。
- KEY2 对应 `ADKEY_PREV`，进入 `ap_peripheral_prev_key_exe()`。

按键拦截只放在两个处理函数的短按分支中，长按判断和已有长按逻辑不做改动。

## 架构

采用“通用按键回调注册”方案：

1. 外设按键模块提供不绑定蓝牙实现的通话按键动作枚举、回调类型和注册函数。
2. `bt_xstack_demo_start()` 注册通话按键回调。
3. KEY1/KEY2 短按时，外设模块分别派发“接听”和“挂断”语义动作。
4. 蓝牙 Demo 根据缓存的 HFP 状态决定是否消费该动作，并在需要时调用 `bt_xstack_send_at()`。
5. 回调返回“未消费”时继续执行原有按键逻辑；返回“已消费”时立即结束本次按键处理。

这样，未启动蓝牙 Demo 或未注册回调的其他工程不会产生新的链接依赖，按键行为也保持原样。

## 接口定义

在 `ap_peripheral_handling.h` 中增加：

```c
typedef enum {
    AP_PERIPHERAL_CALL_KEY_ANSWER = 0,
    AP_PERIPHERAL_CALL_KEY_HANGUP
} ap_peripheral_call_key_action_t;

typedef INT8U (*ap_peripheral_call_key_handler_t)(
    ap_peripheral_call_key_action_t action);

void ap_peripheral_call_key_handler_register(
    ap_peripheral_call_key_handler_t handler);
```

返回值约定：

- `0`：当前动作未被消费，按键模块继续执行原功能。
- 非 `0`：动作已被消费，按键模块不再执行原功能。

注册 `NULL` 等价于取消注册。回调指针默认是 `NULL`。

## HFP 状态管理

`bt_xstack_demo` 在现有事件回调中以长度受限的方式解析：

- `+HFPRING=1/0`：记录当前是否正在振铃。
- `+HFPSTAT=<state>[,...]`：记录当前 HFP 状态。
- `+PWRSTAT=0`：清除振铃和 HFP 状态，防止蓝牙关闭后残留旧状态。

状态值沿用当前 BTstack HFP HF 定义：

- `1` IDLE
- `2` CONNECTING
- `3` CONNECTED
- `4` OUTGOING
- `5` INCOMING
- `6` ACTIVE
- `7` HELD
- `8` ACTIVE_WITH_INCOMING
- `9` ACTIVE_WITH_HELD
- `10` MULTIPARTY

不直接对事件数据调用 `strlen()`，也不假设事件数据带 NUL 结尾。

## 按键行为

### KEY1：接听

当 `+HFPRING=1`，或 HFP 状态为 `INCOMING` / `ACTIVE_WITH_INCOMING` 时：

1. 发送 `AT+HFPANSW`。
2. 消费本次按键，避免同时触发原有 OK/功能键动作。
3. 打印发送成功或失败日志。

其他状态返回未消费，继续原有 KEY1 行为。

### KEY2：拒接或挂断

当正在振铃，或 HFP 状态为以下任一状态时发送 `AT+HFPCHUP`：

- OUTGOING
- INCOMING
- ACTIVE
- HELD
- ACTIVE_WITH_INCOMING
- ACTIVE_WITH_HELD
- MULTIPARTY

无论 AT 指令提交成功还是失败，本次按键均视为已消费，防止在通话场景误触发 PREV 原功能；失败原因通过日志报告。

IDLE、CONNECTING、CONNECTED 状态返回未消费。

## 并发和失败处理

- HFP 状态由蓝牙事件回调写入、按键任务读取，使用小尺寸 `volatile` 状态变量；单次读写在目标平台上是原子的。
- 每次有效短按只提交一条 AT 指令，不增加自动重试；状态变化继续由 BTstack 事件驱动。
- 若回调未注册，按键派发直接返回未消费。
- 若 `bt_xstack_send_at()` 返回错误，记录动作和错误码，但保持按键已消费。

## 测试与验证

1. 增加契约测试，确认 KEY1/`ADKEY_OK` 只在短按分支派发接听动作，KEY2/`ADKEY_PREV` 只在短按分支派发挂断动作。
2. 确认未消费时仍能到达现有按键消息，已消费时提前返回。
3. 增加 Demo 契约测试，确认解析 `+HFPRING`、`+HFPSTAT`、`+PWRSTAT=0`，并发送精确指令 `AT+HFPANSW` / `AT+HFPCHUP`。
4. 编译主工程，确认无新增告警或链接错误。
5. 板端验证：来电日志出现 `+HFPRING=1` 后按 KEY1 接通；通话中按 KEY2 结束通话；无通话时两个按键仍执行原功能。

## 非目标

- 不新增自动接听、自动挂断或按键长按通话功能。
- 不改变 HFP 协议栈控制接口和 AT backend。
- 不处理 SCO 音频路由或麦克风通路。
- 不改变 KEY1/KEY2 的硬件 ADC 阈值。

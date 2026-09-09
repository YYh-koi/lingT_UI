# BTstack AFH Channel Assessment 初始化设计

## 目标

在 GPCV2178 的 BTstack Classic 初始化过程中显式发送 HCI `Write AFH Channel Assessment Mode`，将 Controller 的本地 AFH 信道评估设置为启用，用于验证 WiFi 与 A2DP 并行工作时的蓝牙链路稳定性。

## 范围

- 增加标准 HCI opcode `0x0C49` 和单字节参数命令描述。
- 在 BTstack HCI 初始化状态机中增加发送状态和等待 Command Complete 状态。
- 通过编译配置 `ENABLE_AFH_CHANNEL_ASSESSMENT` 控制功能，仅在 GPCV2178 当前配置中默认开启。
- 参数固定为 `0x01`，表示启用 Channel Assessment。
- 输出一条独立结果日志，不重新开启完整 HCI dump。
- 重新构建正式交付库 `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`，并验证主工程链接。

不修改 AT 接口、A2DP PCM 接口、应用启动流程或 WiFi 初始化流程。

## 初始化流程

AFH 命令接入现有 HCI 初始化状态机的 Classic 初始化段。运行时流程为：

1. 前一条初始化命令收到 Command Complete。
2. 进入 `HCI_INIT_WRITE_AFH_CHANNEL_ASSESSMENT_MODE`。
3. 发送 `49 0C 01 01`，其中前三字节是 opcode 和参数长度，最后一个字节是 `Enable=1`。
4. 进入 `HCI_INIT_W4_WRITE_AFH_CHANNEL_ASSESSMENT_MODE`。
5. 收到对应 Command Complete 后记录 status，并进入下一条初始化命令。

该流程继续遵守“一次提交一条逻辑 HCI 命令，收到完成事件后再发送下一条”的现有策略。H5 协议层可靠重传保持不变。

## 代码边界

- `btstack/src/hci_cmd.h`：定义 opcode，声明命令对象。
- `btstack/src/hci_cmd.c`：定义参数格式为一个 `uint8_t` 的命令对象。
- `btstack/src/hci.h`：增加发送和等待两个初始化状态。
- `btstack/src/hci.c`：在 Classic 初始化段发送命令并处理完成结果。
- `btstack/ports/gpcv2178/btstack_config.h`：定义 `ENABLE_AFH_CHANNEL_ASSESSMENT`。
- `btstack/ports/gpcv2178/tests/`：增加静态回归测试，验证命令编码、配置开关、状态顺序和等待完成机制。

## 状态和错误处理

- Command Complete `status=0x00` 时打印：`btstack: AFH channel assessment status=0x00`。
- 非零 status 同样打印实际状态码，但不阻塞整个蓝牙栈启动，以便兼容不支持该命令的 Controller 固件。
- 功能关闭时不编译对应初始化状态，不改变其他 BTstack port 的初始化序列。
- 不增加超时重发策略；命令完成等待遵循 BTstack 现有初始化机制，H5 仍负责传输层可靠重传。

## 验证

1. 先运行新增静态测试并确认因功能尚未实现而失败。
2. 实现最小改动后确认静态测试通过。
3. 运行现有 HCI Reset、生命周期和 HCI dump 配置测试，确认无回归。
4. 重建 `libxstack.a`，检查归档中包含新的命令和状态机实现。
5. 重建或至少完成主工程链接验证，确认正式交付目录使用的是新库。
6. 板端启动后确认出现 AFH status 日志，并在 WiFi 已启动的情况下重复 A2DP 播放测试；结合现有 disconnect reason 日志判断稳定性是否改善。

## 成功标准

- 初始化过程中只发送一次逻辑 `Write AFH Channel Assessment Mode`。
- 在收到该命令的 Command Complete 前不发送下一条初始化命令。
- Controller 返回 `status=0x00`。
- HCI dump 保持关闭，仍可看到独立 AFH 结果日志。
- 蓝牙初始化、配对、A2DP 连接和播放流程无新增回归。

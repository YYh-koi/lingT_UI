# BTstack API 音频数据面架构设计

日期：2026-08-10  
状态：已取消。A2DP SBC 解码保留在 run-loop；SCO 稳定性工作由 `2026-08-10-sco-downlink-stability-design.md` 取代。

## 1. 目标

本阶段同时优化 A2DP 和 SCO 音频数据面：

1. 将 A2DP SBC 解码从 BTstack run-loop 迁移到 `btstack_api` 内部媒体任务，降低 run-loop 最大占用时间。
2. 为 SCO 下行增加稳定的异步入口、16-bit 字节流重组和 40–60 ms 抖动缓冲，减少杂音、爆音和任务抖动造成的断续。
3. 保持 SBC 解码属于 `btstack_api`，外部应用只接收 PCM。
4. 保持现有 `bt_xstack_*` 异步控制接口、音频 callback 类型和“无 `COMMAND_COMPLETE`”语义不变。
5. 全部使用固定静态内存，不使用动态分配；新增静态 RAM 不超过 24 KB。

## 2. 非目标

- 不增加 mSBC 宽带语音。
- 不将 SBC 压缩数据暴露给应用。
- 不改变 AT 副本之外的原协议栈目录。
- 不在本阶段实现音频上行、回声消除、降噪或自动增益。
- 不在没有上板测量的情况下宣称具体 CPU 降幅。

## 3. 当前实现与问题

### 3.1 A2DP

`btstack_api/profiles/btstack_a2dp_media.c` 当前在 A2DP media packet handler 内直接调用 `btstack_sbc_decoder_process_data()`。该 handler 运行于 BTstack run-loop，因此 RTP 解析、SBC 解码和 PCM callback 均占用 run-loop 时间。

### 3.2 SCO

`btstack_api/src/classic/hfp_hf.c` 当前从 HCI SCO packet handler 直接调用音频桥接 callback。应用 callback 随即将数据复制到 `bt_audio_task` 的 PCM stream。现有链路已经对错误 packet status 补静音，但仍缺少：

- 独立于 run-loop 的音频交付任务；
- 明确的 16-bit 跨包奇数字节续接；
- 可测量的 40–60 ms 起播水位；
- SCO/A2DP 的统一抢占状态机；
- 音频数据面队列水位和错误诊断。

## 4. 架构决定

采用一个位于 `btstack_api` 内部的媒体任务，使用两条独立静态入口通道：

```text
BTstack run-loop
 ├─ A2DP RTP 校验 → A2DP SBC 固定包池
 └─ SCO HCI 校验  → SCO 固定字节环
                         ↓
              btstack_api 内部媒体任务
                ├─ SCO 优先转交
                └─ SBC 解码成 PCM
                         ↓
            现有 media/call PCM callback
                         ↓
                 应用 bt_audio_task
                         ↓
              抖动缓冲、DMA、DAC、喇叭
```

该媒体任务建议使用 `tskIDLE_PRIORITY + 2`，低于 GPCV2178 BTstack run-loop 的 `tskIDLE_PRIORITY + 3`。任务按通知唤醒，不做周期轮询。每解码一个 A2DP 包后必须重新检查 SCO 队列，保证 SCO 到达后不会被连续 SBC 解码长期阻塞。

## 5. 组件职责

### 5.1 A2DP run-loop 入口

run-loop 内仅执行：

1. 校验 RTP fixed header、CSRC、extension 和 padding。
2. 校验 SBC media header，不支持 fragmented SBC，拒绝零 frame count。
3. 将完整 SBC payload 和配置 generation 复制到固定包槽。
4. 更新计数并通知内部媒体任务。

run-loop 内不再调用 SBC decoder，也不再调用应用 media PCM callback。

### 5.2 SCO run-loop 入口

run-loop 内仅执行：

1. 校验 HCI SCO header 和 payload length。
2. 读取 packet status；错误包转换为等长静音数据。
3. 将数据复制到 SCO 固定字节环。
4. 原子切换或确认 SCO 优先模式，并通知内部媒体任务。

run-loop 内不再直接调用应用 call PCM callback。

### 5.3 `btstack_api` 内部媒体任务

媒体任务拥有 SBC decoder context，职责为：

- SCO 模式下优先排空 SCO 输入，并通过现有 call PCM callback 交付。
- A2DP 模式下每次解码一个完整 SBC 包，再重新检查 SCO。
- 在配置 generation 变化、A2DP 重连或 SCO 抢占后重置 decoder。
- 在调用外部 callback 前退出所有临界区；callback 指针和 context 使用短临界区复制。
- 不持有指向 HCI/L2CAP 接收缓存的指针。

### 5.4 应用 `bt_audio_task`

应用任务仍只接收 16-bit PCM，并负责：

- A2DP/SCO 播放模式和 DAC 配置；
- 独立的 2,048-byte SCO 抖动环及 40–60 ms 起播；
- 现有 A2DP PCM stream、双 DMA buffer 和 underflow 补静音；
- SCO 抢占时停止 DAC、清空旧 A2DP PCM，并切换到 8 kHz 单声道；
- SCO 结束后清空旧通话 PCM，只从新的 A2DP PCM 配置和数据恢复。

不得在应用层调用 SBC decoder，也不得让应用解析 RTP/SBC。

## 6. 静态内存预算

新增内存按以下上限设计：

| 项目 | 配置 | 预算 |
|---|---:|---:|
| A2DP SBC 包池 | 12 × 1024 bytes | 12,288 bytes |
| `btstack_api` SCO 输入字节环 | 2,048 bytes | 2,048 bytes |
| 内部媒体任务栈 | 1024 × 4 bytes | 4,096 bytes |
| 应用 SCO 抖动环 | 2,048 bytes | 2,048 bytes |
| 槽位元数据、静态任务控制块、对齐余量和诊断 | 上限 | 1,024 bytes |
| 合计新增静态 RAM | 上限 | 21,504 bytes |

现有 SBC decoder context 由原位置迁移，不重复分配。应用当前 A2DP PCM stream 和 DMA buffer 继续复用，不计入新增预算。SCO 使用独立的 2,048-byte 抖动环，避免复用 16-KB A2DP stream 时形成不可控的通话积压。链接 map 验证必须证明新增静态 RAM 不超过 24 KB。

## 7. 状态与抢占

内部音频模式为：

```text
IDLE → A2DP → SCO_PREEMPT → SCO → IDLE/A2DP
```

规则：

1. SCO 建链事件先触发抢占；如果 SCO 数据包先于控制事件到达，首个有效 SCO 包也必须原子触发抢占。
2. 抢占立即停止接收旧 generation 的 A2DP 数据，清空 SBC 包池，重置 decoder，并要求应用清空旧 A2DP PCM。
3. SCO 活动期间收到的 A2DP media 包直接丢弃并计数，不形成积压。
4. SCO 断开时清空 SCO 输入、奇数字节续接和应用 PCM/DMA 状态。
5. A2DP 只在收到新的 PCM 配置及新 generation 数据后恢复。
6. 通话前的 SBC 或 PCM 数据不得在通话结束后播放。

## 8. SCO 格式与延迟

本阶段只支持窄带 CVSD 通话，Host 侧期望格式为：

- `sco_voice_setting = 0x0060`；
- 空中编码 CVSD；
- Host 数据 Linear PCM；
- 2's complement；
- 16-bit little-endian；
- 8 kHz 单声道。

SCO 建链后必须检查实际活动格式。若不是预期的 16-bit Linear PCM，不得把数据当作 PCM 送入 DAC；记录 format mismatch 并保持静音。透明数据或 mSBC 格式不在本阶段处理。

8 kHz、单声道、16-bit PCM 的速率为 16,000 bytes/s：

- 40 ms = 640 bytes；
- 48 ms = 768 bytes；
- 60 ms = 960 bytes。

应用 SCO 抖动环的默认起播水位为 768 bytes，允许通过内部编译配置在 640–960 bytes 范围调整。2,048-byte 抖动环可提供约 128 ms 的有界容量，但正常工作目标仍为 40–60 ms，不允许持续积压到容量上限。`btstack_api` 内部另有一个 2,048-byte 输入环，用于把 HCI/run-loop 与媒体 callback 解耦；它不作为播放起播水位。

## 9. 队列和错误策略

### 9.1 A2DP

- 包池满：丢弃最旧的完整 SBC 包，接受最新包，避免播放延迟持续增加。
- RTP/SBC header 非法：丢当前包并增加 malformed/unsupported 计数。
- 配置 generation 不匹配：丢旧包。
- 单包异常不停止 A2DP profile，也不把整个 API 标记为失败。
- 配置变化、重连和 SCO 抢占时统一清空队列并重置 decoder。

### 9.2 SCO

- packet status 异常：生成等长静音。
- header 或 payload length 不完整：丢当前包并计数。
- 奇数字节：保存最后一个字节，与下一包第一个字节组合成完整 16-bit sample。
- 输入环溢出：按完整 16-bit sample 丢弃最旧数据。
- DAC/PCM underflow：对缺失部分补静音，保持 DMA 连续。
- 断开、重连、格式变化：清除续接字节、输入环、PCM stream 和 DMA generation。

## 10. 并发与数据所有权

- HCI/L2CAP 接收缓存只在 run-loop callback 期间有效，入队前必须复制。
- A2DP 包槽和 SCO 环的索引在短临界区内修改，复制大块数据时不得长时间关闭中断；实现应使用“预占槽位/复制/提交”的状态转换。
- 消费者只处理已经提交的槽位。
- callback 调用期间的 PCM 指针只保证在本次 callback 返回前有效。
- 现有 `bt_audio_task_write_pcm()`必须立即复制 PCM，不保存传入指针。
- 任何外部 callback 均在退出队列锁和临界区后调用。
- 所有队列、任务栈、decoder context 和诊断结构为静态对象，不使用 `malloc`。

## 11. 公共 API

以下接口保持兼容：

- `bt_xstack_media_audio_callback_t` 继续接收 PCM bytes；
- `bt_xstack_call_audio_callback_t` 继续接收 SCO 的 8 kHz/16-bit/mono PCM bytes；
- 所有现有控制 API、状态事件和 Getter 语义不变；
- 不增加命令完成事件。

新增一个独立、同步、只读 Getter：

```c
bt_xstack_result_t bt_xstack_get_audio_diagnostics(
    bt_xstack_audio_diagnostics_t *diagnostics);
```

诊断至少包含：

- 当前音频模式；
- A2DP 收包、格式错误、丢包、已解码包、decoder reset；
- A2DP 队列当前水位和最高水位；
- SCO 收包、异常包、格式不匹配、补静音字节；
- SCO 队列当前水位和最高水位；
- SCO 输入 overflow、callback 交付间隙和奇数字节续接；
- SCO 抢占 A2DP 次数。

该 Getter 只复制 `btstack_api` 所拥有的缓存计数，不提交命令、不进入 run-loop。DAC underflow 和应用 PCM 丢弃属于 `Project/wifi_demo/bt_audio_task`，不能伪装成协议栈诊断；应用层另提供：

```c
int bt_audio_task_get_diagnostics(
    bt_audio_task_diagnostics_t *diagnostics);
```

它报告当前播放源、PCM/抖动环水位、DMA underflow、PCM 丢弃、补静音字节和实际起播水位。两个 Getter 均为同步缓存读取。

## 12. 自动测试

1. 验证 `btstack_a2dp_media_process()`不再直接调用 SBC decoder。
2. 验证 SBC decoder 只从内部媒体任务上下文调用。
3. 验证 A2DP 包池 FIFO、满队列丢最旧、generation 丢旧包和清空。
4. 验证 RTP CSRC、extension、padding、非法长度、fragmented 和 frame count。
5. 验证 SCO 多个奇数长度输入最终保持 16-bit PCM 对齐。
6. 验证错误 packet status 输出等长静音。
7. 验证独立 SCO 抖动环的 768-byte 起播水位、overflow 丢旧 sample 和 underflow 补静音。
8. 验证 SCO 建链抢占、SCO 首包抢占、断开清理以及只从新 A2DP 数据恢复。
9. 验证 media/call PCM callback 均不在 run-loop 上下文执行。
10. 验证公共头文件 C/C++ 编译兼容、原 API 不变且无 `COMMAND_COMPLETE`。
11. 重建 `libxstack_api.a` 和完整 Debug ELF并检查关键/禁止符号。
12. 从 map 文件核对新增静态 RAM 不超过 24 KB。

## 13. 上板验证

1. 确认活动 SCO voice setting 与 8 kHz、16-bit Linear PCM 格式。
2. 分别播放 44.1 kHz 和 48 kHz A2DP，测试连续播放、暂停、切歌和重连。
3. 普通信号和弱信号下测试 CVSD 通话，观察周期性杂音、爆音和补静音行为。
4. 执行 `A2DP 播放 → 来电 → SCO 通话 → 挂断 → A2DP 恢复` 全流程。
5. 记录 run-loop 最大忙时、音频队列最高水位、丢包、overflow 和 underflow。

## 14. 完成标准

- run-loop 中不存在 SBC 解码和应用音频 callback。
- SBC decoder 仍完全封装在 `btstack_api`。
- 正常信号下 A2DP/SCO 队列不持续满载。
- SCO 不因跨包奇数字节产生 16-bit PCM 错位。
- 异常 SCO 包表现为短静音，不产生明显爆音。
- SCO 抢占和恢复期间不播放旧 SBC/PCM 缓存。
- 新增静态 RAM 不超过 24 KB。
- API/Demo/SCO 回归、API 静态库构建和完整 Debug 工程链接全部通过。
- 上板测量完成前，只声明架构迁移和主机验证结果，不声明实际 CPU 或音质改善幅度。

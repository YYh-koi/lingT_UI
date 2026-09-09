# SCO 下行播放稳定性设计

日期：2026-08-10  
状态：已实现并完成主机与构建验证；等待板端通话音质验证

## 1. 目标

增强“手机到设备喇叭”的 SCO 下行播放稳定性，解决共享 PCM StreamBuffer 可能导致的部分写入、16-bit PCM 错位、无界积压和 underflow 爆音风险。

本阶段只调整 Demo 应用的 `bt_audio_task` 和对应 callback 接入：

- 在 `bt_audio_task` 增加独立的 2,048-byte SCO 环形字节缓冲。
- SCO 缓冲满时丢弃最旧的完整 16-bit samples，保持通话实时性。
- 使用两块 384-byte SCO DMA buffer，在 768 bytes 时起播，目标预缓冲约 48 ms。
- 缺数据时补静音并保持 DAC 连续运行。
- 增加应用侧同步诊断 Getter。

## 2. 保持不变的边界

- A2DP SBC 解码继续在 BTstack run-loop 中执行。
- A2DP media PCM callback 仍由 run-loop 直接调用应用。
- SCO call PCM callback 仍由 run-loop 直接调用应用。
- callback 中只做有界内存复制、计数和任务通知，不做阻塞等待、DAC 操作或复杂计算。
- 不增加 `btstack_api` 内部媒体任务。
- 不增加 mSBC。
- 不修改现有 `bt_xstack_*` 公共 API、callback 类型或“无 `COMMAND_COMPLETE`”语义。

## 3. 当前问题

当前 A2DP 和 SCO 共用一个 16-KB FreeRTOS StreamBuffer，均通过 `bt_audio_task_write_pcm()`写入。该实现有三个风险：

1. `xStreamBufferSend(..., 0)`在空间不足时可能只写入部分数据。
2. 如果部分写入字节数为奇数，后续 16-bit PCM sample 会整体错位，表现为持续杂音。
3. 16-KB 缓冲对于 8 kHz、mono、16-bit SCO 相当于约1秒音频，过载时可能积累明显通话延迟。

现有 SCO DMA 单块为512 bytes，即32 ms；双缓冲起播需要1,024 bytes，约64 ms。

## 4. 方案选择

采用应用侧独立 SCO 环形字节缓冲，不使用第二个 FreeRTOS StreamBuffer，也不新增协议栈媒体任务。

```text
BTstack run-loop
 → HCI SCO packet 长度/status 校验
 → 错误包按原长度补静音
 → call_audio_callback
 → bt_audio_task_write_call_pcm()
 → 2-KB SCO 环形字节缓冲
 → 通知 bt_audio_task
 → 48-ms 起播
 → 双 DMA/DAC
```

A2DP 保持原数据路径：

```text
BTstack run-loop 内 SBC 解码
 → media_audio_callback
 → bt_audio_task_write_media_pcm()
 → 现有 16-KB A2DP StreamBuffer
 → bt_audio_task
 → DMA/DAC
```

## 5. 接口设计

在 `Project/Include/bt_audio_task.h` 增加：

```c
void bt_audio_task_write_media_pcm(
    const uint8_t *data,
    uint32_t len);

void bt_audio_task_write_call_pcm(
    const uint8_t *data,
    uint32_t len);

int bt_audio_task_get_diagnostics(
    bt_audio_task_diagnostics_t *diagnostics);
```

Demo 的 A2DP callback 改调 `bt_audio_task_write_media_pcm()`，SCO callback 改调 `bt_audio_task_write_call_pcm()`。

保留现有 `bt_audio_task_write_pcm()`作为兼容包装；它依据当前已提交的音频 source 路由到对应写入口。新代码不得依赖该隐式路由，必须使用 source-specific 接口。

## 6. SCO 环形缓冲

### 6.1 容量和所有权

- 固定容量：2,048 bytes。
- 单生产者：BTstack run-loop 上下文中的 call PCM callback。
- 单消费者：`bt_audio_task`。
- 不使用动态内存。
- 新增静态 RAM 约2 KB加少量索引和诊断字段，不新增任务栈。

生产者在 overflow 时也会推进读位置，因此实现不能假设生产者永远不修改 consumer index。索引、计数和一次最大255-byte的 HCI SCO payload 复制必须由同一个有界临界区保护，不使用阻塞锁。

### 6.2 写入规则

`bt_audio_task_write_call_pcm()`执行：

1. 拒绝 NULL、零长度、任务未启动或当前 source 不是 SCO 的输入。
2. 计算容纳完整输入所需空间，不允许部分写入。
3. 空间不足时，从最旧数据开始丢弃；丢弃量向上对齐为偶数字节。
4. 完整复制本次输入，更新当前/最高水位和 overflow 诊断。
5. 非零写入后通知 `bt_audio_task`。

HCI SCO payload 最大长度受一个字节长度字段限制，因此单次复制上限为255 bytes，小于2-KB环容量。

### 6.3 16-bit 对齐

- 环中允许暂存一个孤立尾字节。
- consumer 每次只移除偶数字节，孤立尾字节留给后续输入组成完整 sample。
- overflow 只丢弃偶数字节，不能改变剩余数据的16-bit相位。
- stop、reconfigure 和 SCO disconnect 必须清除孤立尾字节及全部旧数据。

## 7. 播放与 DMA

SCO 固定格式为8 kHz、单声道、16-bit PCM，数据率为16,000 bytes/s。

- 单个 SCO DMA buffer：384 bytes，等于24 ms。
- 两个 DMA buffer：768 bytes，等于48 ms。
- SCO ring 水位不足768 bytes时不启动 DAC。
- 达到768 bytes后预填两个 DMA buffer并启动 DAC。
- 播放过程中每次 refill 请求384 bytes。
- 可用完整 samples 少于384 bytes时，读取已有偶数字节，其余全部补零。
- underflow 不停止 DAC；后续真实 PCM 从正确 sample 边界继续播放。

A2DP DMA buffer、起播条件和现有 StreamBuffer 逻辑不变。

## 8. 音频 source 与生命周期

内部 source 至少包含：`IDLE`、`A2DP`、`SCO`。

- 配置8 kHz/mono/16-bit时选择 SCO source，并清空 SCO ring、A2DP stream和旧 DMA generation。
- 配置44.1或48 kHz、16-bit时选择 A2DP source，并清空 SCO ring和旧 DMA generation。
- stop 时停止 DAC并清空两个输入缓冲、孤立尾字节和播放状态。
- `bt_audio_task_write_media_pcm()`只在 A2DP source 下接收数据。
- `bt_audio_task_write_call_pcm()`只在 SCO source 下接收数据。
- source 不匹配的数据整块丢弃并计数，防止通话切换期间旧 A2DP PCM 混入 SCO。

## 9. 错误和过载策略

- HCI packet status 异常继续由 `hfp_hf_sco_packet_handler()`转换为等长静音。
- HCI header/长度不完整继续在进入应用 callback 前丢弃。
- SCO ring overflow：丢弃最旧的完整 samples，接受最新数据。
- SCO DMA underflow：缺失部分补静音，DAC持续运行。
- 输入不匹配当前 source：整块丢弃。
- 任何单包异常都不得停止音频任务或把整个 xStack API 标记为失败。
- 高频 PCM callback 不执行常规日志；诊断日志只在音频任务中限频输出。

如果优化后仍有持续杂音，下一步必须验证控制器实际 Host SCO 数据格式、字节序和 `sco_voice_setting`。缓冲策略不能修复把非16-bit Linear PCM误当作PCM播放的问题。

## 10. 诊断

在应用层定义 `bt_audio_task_diagnostics_t`，至少包含：

- 当前 source；
- SCO 当前水位和历史最高水位；
- SCO overflow 次数；
- 丢弃的最旧 PCM 字节数；
- source mismatch 丢弃次数；
- DMA underflow 次数；
- 补静音字节数；
- 孤立尾字节观察次数。

`bt_audio_task_get_diagnostics()`只在短临界区内复制缓存，不进入 run-loop，不等待音频任务，不触发 callback。

## 11. 自动测试

1. 验证 A2DP/SCO 使用不同写入口和不同缓冲区。
2. 验证 SCO 环固定为2,048 bytes且无动态分配。
3. 验证空间不足时只丢最旧的偶数字节，完整接收新输入。
4. 验证多个奇数长度块跨包后输出顺序和16-bit相位正确。
5. 验证不存在 StreamBuffer 式部分写入。
6. 验证不足768 bytes不启动 SCO DAC，达到768 bytes后预填两个384-byte DMA buffer。
7. 验证播放 underflow 时缺失部分补零且 DAC保持运行。
8. 验证 stop/reconfigure 清空环、孤立尾字节、旧 DMA 和水位状态。
9. 验证 A2DP callback 不写 SCO ring，SCO callback 不写 A2DP stream。
10. 验证诊断 Getter 不提交 API 命令、不进入 run-loop。
11. 运行现有 API、Demo、SCO 回归并重新链接完整 Debug 工程。
12. 验证 A2DP media 代码和 SBC decoder 调用位置没有变化。

## 12. 上板验收

执行：

```text
A2DP播放 → 来电 → SCO建链 → 连续通话
 → 弱信号/异常包 → 挂断 → A2DP恢复
```

验收条件：

- SCO 起播预缓冲约48 ms。
- 正常信号下 SCO ring 不持续满载。
- overflow 后通话保持实时，延迟不持续累积。
- underflow 表现为短静音而不是爆音。
- 不发生由部分写入或奇数字节丢弃造成的持续 PCM 错位杂音。
- SCO 断开后不播放旧通话缓存。
- A2DP SBC 解码位置、API语义和播放效果不发生变化。

## 13. 完成标准

- 新增独立2-KB SCO ring和 source-specific写入口。
- SCO overflow、underflow和生命周期行为符合本规格。
- 应用诊断 Getter 可读取缓冲水位和错误计数。
- 相关自动测试、现有17项控制面/API/Demo/SCO回归、API库构建和完整Debug工程链接通过。
- 上板验证前只声明代码和构建结果，不声明实际音质改善。

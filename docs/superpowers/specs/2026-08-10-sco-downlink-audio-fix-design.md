# SCO 下行播放质量修复设计

## 目标

修复手机到设备扬声器的 HFP SCO 下行播放问题，使当前 CVSD 通话音频按
8 kHz、单声道、16-bit 线性 PCM 正常播放，消除采样率错误、周期性欠载和
错误 SCO 包直接播放导致的杂音。

本次不实现设备麦克风到手机的 SCO 上行，也不加入 mSBC 宽带语音。

## 已确认的根因

1. `bt_xstack_demo` 请求 `bt_audio_task_configure(8000, 1, 16)`，但
   `bt_audio_task` 只接受 44100/48000 Hz，导致 SCO 配置失败或沿用旧的
   A2DP 采样率。
2. 音频任务按队列包数判断预缓冲。SCO 包远小于 A2DP 块，达到包数阈值时
   有效字节仍不足以填满 DMA 缓冲，产生补零、欠载和丢包。
3. HCI SCO Packet Status 未被检查，错误或丢失数据可能作为 PCM 直接送入
   DAC，产生爆音和沙沙声。

## 架构与职责

### BTstack API 层

- HCI/HFP 层从 SCO 包头取得连接句柄、Packet Status 和负载长度。
- 仅当前配置支持的 CVSD 下行被解释为主机侧 8 kHz、单声道、16-bit
  线性 PCM。
- 正常包把 PCM 交给 `call_audio_callback`。
- Packet Status 表示可能错误、无数据或部分丢失时，不传递原始坏数据；
  交付与原负载等长的静音 PCM，以保持播放时间连续。
- 回调继续运行在 BTstack 任务上下文，只允许快速、非阻塞地写入音频缓冲。

### Demo 层

- 通话状态负责请求建立和释放 SCO。
- 收到 HFP AUDIO 已连接事件后，将音频任务配置为 8000 Hz、单声道、
  16-bit，再允许下行 PCM 进入播放路径。
- SCO 释放、HFP 丢失或蓝牙掉电时停止 DAC并清空通话音频。
- A2DP 与 SCO 共用 `bt_audio_task`，但格式切换必须刷新旧 PCM，不能混播。

### `bt_audio_task`

- 增加 8000 Hz、单声道、16-bit 合法配置，同时保留 A2DP 的
  44100/48000 Hz 配置。
- 输入缓冲从“一次回调占一个队列块”改为连续字节流写入：多个 SCO 小包
  可以拼入同一个固定块，一个较大的 A2DP 回调也可拆分到多个块。
- 跟踪 ready 队列中的有效 PCM 字节数。启动 DAC 的预缓冲条件基于字节数，
  不再基于队列项数。
- 缓冲总有效容量至少覆盖双 DMA 缓冲，并留出生产/消费抖动余量。
- DMA 数据不足时补零并累计 underflow；输入空间不足时丢弃新数据并累计
  drop。诊断日志采用节流方式报告累计值。

## 数据流

1. 手机建立 HFP 通话，Demo 请求建立 SCO。
2. HCI 报告 SCO AUDIO 已连接。
3. Demo 配置 `bt_audio_task` 为 `8000/1/16`，配置切换刷新旧数据。
4. HCI 收到 SCO 包并检查 Packet Status。
5. 正常负载或等长静音负载通过 `call_audio_callback` 写入连续 PCM 缓冲。
6. 有效预缓冲达到至少一个 DMA 缓冲后启动 DAC。
7. DMA 双缓冲按 8 kHz 连续消费 PCM；欠载只产生静音，不播放未定义数据。
8. SCO 释放或链路丢失时停止 DAC并清空缓冲。

## 错误处理

- 不支持的采样率、声道数或位宽返回配置失败，不改变当前格式。
- Packet Status 异常时输出等长静音，不向应用暴露损坏 PCM。
- 缓冲满时不阻塞 BTstack 任务，丢弃新输入并记录 drop。
- DMA 欠载时补零并记录 underflow。
- HFP AUDIO 建立后配置 DAC 失败时记录错误，并停止通话播放路径。

## 测试策略

先新增会在旧实现上失败的契约测试，分别要求：

1. `bt_audio_task` 接受 8000 Hz、单声道、16-bit。
2. 预缓冲和容量判断使用有效字节数，而不是队列项数。
3. SCO Packet Status 在原始负载送入应用前被处理。
4. Demo 在 HFP AUDIO 已连接事件后配置通话格式，并在释放时停止音频。

修复后运行：

- SCO 专项契约测试。
- 现有 Demo/API/AT 隔离契约。
- `BTstack_API_Library` 完整构建。
- 主工程 Debug 完整构建和 ELF 符号检查。

## 实机验收

- 日志显示 `DAC started rate=8000 channels=1`。
- 通话语速和音调正常，无周期性大段静音和明显爆音。
- underflow/drop 计数不持续快速增加。
- 挂断、HFP 丢失和蓝牙掉电后扬声器及时停止。
- 通话结束后 A2DP 可重新按 44100/48000 Hz 播放。


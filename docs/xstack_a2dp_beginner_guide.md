# GPCV2178 xstack A2DP 初学者学习笔记

## 1. A2DP 在本工程中做什么

A2DP 是手机把音乐发送给设备的经典蓝牙 Profile。本工程的角色是 **A2DP Sink**：
手机负责播放和发送，本机接收、解码并输出到 DAC。

```text
手机音乐
  → SBC 压缩音频
  → 蓝牙空口
  → ATBM6165 Controller
  → H5 UART
  → 本机 BTstack Host
  → SBC 解码
  → PCM
  → bt_audio_task
  → DAC / 扬声器
```

## 2. 先区分三种内容

| 内容 | 示例 | 应用应该做什么 |
|---|---|---|
| 连接状态 | 已连接、断开、Stream Open、Streaming | 更新业务状态 |
| PCM 格式 | 44100 Hz、2 声道、16 bit | 配置 DAC 和音频缓存 |
| PCM 音频 | 连续的采样数据 | 尽快写入缓存，不能做耗时工作 |

状态和 PCM 格式出现频率低，走普通事件路径；PCM 音频出现频率高，走低延迟直通路径。

## 3. 初始化时注册的两类回调

`btstack_profile_manager_setup_classic()` 配置 A2DP 时，注册：

```c
a2dp_config.event_callback = btstack_profile_manager_a2dp_event;
a2dp_config.audio_sample_callback = btstack_core_event_mapper_media_audio;
```

这就是两条路径从一开始便分开的原因：

```text
event_callback        → A2DP 状态和 PCM 格式
audio_sample_callback → 解码后的 PCM
```

在 A2DP Profile 内部又注册：

```c
a2dp_sink_register_packet_handler(btstack_a2dp_packet_handler);
a2dp_sink_register_media_handler(btstack_a2dp_media_handler);
```

第一个处理信令和状态，第二个处理真正的媒体包。

## 4. 状态事件如何到达应用

```text
手机建立 A2DP 连接
→ btstack_a2dp_packet_handler()
→ btstack_a2dp_emit()
→ btstack_profile_manager_a2dp_event()
→ btstack_core_event_mapper_a2dp_event()
→ critical event queue
→ Event worker
→ bt_xstack_demo_audio_event()
```

应用从 `bt_xstack_demo_audio_event()` 收到的常见事件：

```text
BT_XSTACK_AUDIO_EVENT_A2DP_STATE
BT_XSTACK_AUDIO_EVENT_PCM_CONFIG
BT_XSTACK_AUDIO_EVENT_A2DP_LOST
```

收到 `A2DP_STATE=STREAMING` 表示手机已开始推流；它不表示本机已经有声音，仍需要先收到
有效的 `PCM_CONFIG`，并持续获得 PCM 数据。

## 5. PCM 为什么不走 Event worker

```text
手机 SBC 媒体包
→ btstack_a2dp_media_handler()
→ btstack_a2dp_media_process()       // SBC 解码
→ btstack_a2dp_audio_sample_dispatch()
→ btstack_core_event_mapper_media_audio()
→ bt_xstack_demo_pcm_event()
→ bt_audio_task_write_media_pcm()
```

`bt_xstack_demo_pcm_event()` 只把数据交给音频任务的 StreamBuffer。这样做是为了避免在
Stack worker 中执行 DAC、显示、日志或其他慢操作。

PCM 指针只在回调期间有效：应用必须立即复制或写入缓存，绝不能保存指针留到以后使用。

## 6. 当前工程支持的 PCM

`bt_xstack_demo_audio_event()` 对 A2DP PCM 格式的要求是：

```text
采样率：44100 或 48000 Hz
声道：1 或 2
位宽：16 bit
```

不满足时，示例会停止蓝牙音频任务，避免用错误的 DAC 配置播放。

## 7. A2DP 与 AVRCP 的关系

A2DP 传音乐，AVRCP 传控制命令和歌曲信息：

```text
A2DP：音乐音频流
AVRCP：播放 / 暂停 / 上一首 / 下一首 / 曲目元数据
```

当前 Manager 在 A2DP 连接成功时，会尝试对同一台手机建立 AVRCP 连接。因此“能播放音乐”和
“能从设备控制手机播放器”是两项相关但独立的能力。

## 8. 阅读代码的推荐顺序

1. `Project/wifi_demo/bt_xstack_demo.c`：先看 `bt_xstack_demo_audio_event()` 与 `bt_xstack_demo_pcm_event()`。
2. `btstack/profiles/btstack_a2dp_sink.c`：看 packet handler、media handler 和两个注册语句。
3. `btstack/core/btstack_profile_manager.c`：看 A2DP 的两个回调如何配置。
4. `btstack/core/btstack_event_mapper.c`：看状态如何变成 `BT_XSTACK_AUDIO_EVENT_*`。
5. `Project/wifi_demo/bt_audio_task.c`：最后看 PCM 如何缓冲并送到 DAC。

## 9. 一句话总结

> A2DP 的状态事件用于告诉应用“现在处于什么阶段”，PCM 回调用于交付“真正要播放的音频”；二者都来自同一个 A2DP Profile，但在初始化时就被注册成两条不同路径。

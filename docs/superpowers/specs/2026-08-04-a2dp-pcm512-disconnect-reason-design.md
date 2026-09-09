# A2DP 512 字节 PCM 与断链原因日志设计

## 目标

- 允许最大 512 字节的 SBC 解码 PCM 数据完整通过现有异步事件队列，到达应用注册的 `bt_xstack_a2dp_register_pcm_callback()` 回调。
- 在关闭完整 HCI dump 时，仍对每个 `HCI_EVENT_DISCONNECTION_COMPLETE` 输出一条精简断链日志。
- 不改变 PCM callback 接口签名，不重新开启 HCI Command、Event、ACL、SCO 或 ISO dump。

## PCM 数据路径

当前最大 SBC PCM 帧为：

```text
16 blocks × 8 subbands × 2 channels × sizeof(int16_t) = 512 bytes
```

现有路径为：

```text
A2DP ACL → SBC decoder → btstack_a2dp_media_handle_pcm()
→ btstack_event_notify_a2dp_audio_sample()
→ BTstack event queue → bt_xstack_pcm_bridge()
→ application PCM callback
```

`BTSTACK_EVENT_DATA_MAX` 当前为 256，`btstack_event_notify_a2dp_audio_sample()` 会静默拒绝大于该值的数据。设计将其提高为 512，使最大 SBC PCM 帧保持单事件、单 callback 传递。

事件队列维持 8 个槽位，因此静态内存约增加：

```text
(512 - 256) × 8 = 2048 bytes
```

不拆分 PCM 帧，也不把应用 callback 改为 BTstack run loop 内同步直调。

## 断链原因日志

GPCV2178 port 增加一个独立的 HCI event callback registration，并在 `hci_init()` 完成后注册。回调仅接受：

- `packet_type == HCI_EVENT_PACKET`
- 事件类型为 `HCI_EVENT_DISCONNECTION_COMPLETE`
- 数据长度足以读取 connection handle 和 reason

匹配时只输出：

```text
btstack disconnect handle=0x0001 reason=0x08
```

字段通过 BTstack 生成的访问函数取得：

- `hci_event_disconnection_complete_get_connection_handle()`
- `hci_event_disconnection_complete_get_reason()`

日志不输出 status 或原始十六进制事件，不依赖 `BTSTACK_GPCV2178_HCI_DUMP_ENABLE`，也不调用 `hci_dump_init()`。

注册对象使用静态生命周期，并通过注册标志防止重复加入 HCI handler 链表。

## 边界和错误处理

- 512 字节 PCM 合法并完整入队；513 字节及以上仍按现有边界拒绝，防止队列对象越界。
- 事件队列已满时维持现有丢弃策略，本次不改变背压机制。
- malformed 或长度不足的 HCI event 不打印日志。
- 非断链 HCI event 不打印日志。
- HCI dump 宏保持为 0；精简断链日志始终可用，便于判断 `0x08` supervision timeout、`0x13` remote user terminated 等原因。

## 测试与验证

1. 先修改事件队列单元/契约测试，要求 `BTSTACK_EVENT_DATA_MAX >= 512`，并验证 512 字节 PCM 能完整到达已注册 callback。
2. 验证 513 字节 PCM 不会调用 callback。
3. 增加 GPCV2178 port 契约测试，要求存在独立断链 handler、handle/reason 访问函数和精确日志格式。
4. 验证 HCI dump 配置仍为 0，精简日志实现不调用 `hci_dump_init()`。
5. 完整清理并重建 `libxstack.a`，避免旧 archive member 残留。
6. 检查新 `libxstack.a` 只包含一套 BTstack 对象成员，并重新链接主工程。
7. 验证最终 ELF 包含精简断链日志，不包含启用 HCI dump 的启动日志。
8. 板端播放音乐，确认 PCM callback/DAC 收到数据；断开手机连接时确认输出一条 handle/reason 日志。

## 非目标

- 不重新设计应用侧 `bt_audio_task` 队列。
- 不改变 SBC 解码算法或 A2DP codec negotiation。
- 不增加完整 HCI packet dump。
- 不在本次改动中解释或转换 HCI reason code 为文本。

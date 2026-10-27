# xstack 蓝牙事件链路追踪笔记（A2DP 事件 → 上层）

> 学习记录，纯只读分析，未修改任何代码。
> 对象：GPCV2178 Turnkey SDK 中 `btstack/` 蓝牙协议栈的事件上报链路。
> 参照平台：ATBM6463 BRT blue SDK（`D:\PYH_PRJ\ATBM6463\PYH_wd\others_BRTblueSDK`）。

---

## 0. 结论速览（TL;DR）

1. 协议栈内部到应用有**两条完全独立的链路**：**控制面 = 事件（走队列，结构化结构体）**，**数据面 = PCM（直调，裸指针）**。不存在"一个回调同时上报事件和 PCM"。
2. 这套 SDK 有**两个可互换的前端（frontend）**，同名函数、同名符号，编译期二选一：
   - **原生前端**：`btstack/api/bt_xstack_api_core.c` → 结构化回调
   - **AT 前端**：`btstack/at/` → 把事件翻译成 AT 字符串
   - 本工程实际链接的是**原生前端**（见 §7 证据）。
3. AT 前端的事件投递是**生产者/消费者模型**：`post_event` 只"入环 + 唤醒"，真正的字符串拼装在 **AT worker 任务**里做（`format_event`）。
4. AT 前端中，`btstack_at_emit_event()` 才是 BRT `bt_event_indicate()` 的最接近同位体：它调用 `btstack_at_format_event()` 完成结构化 → 字符串转换，再去重并调用客户输出回调。`btstack_a2dp_emit()` 不是。

---

## 1. 两条链路的总览

```
                       ┌──────────────── 控制面：事件 ────────────────┐
手机/对端 ──空口──> 协议栈 profile 层
                       │  btstack_a2dp_packet_handler()  (信令/连接/配置)
                       ▼
              btstack_a2dp_emit()         ← profile 层"打包上抛"
                       ▼
         btstack_core_event_mapper_a2dp_event()   ← 转换层：枚举重映射 + 一拆二
                       ▼
              btstack_core_post_event()   ← ★ 前端分叉点（两个同名实现）
                       │
        ┌──────────────┴───────────────┐
        ▼                              ▼
  【原生前端】                    【AT 前端】
  bt_xstack_api_core.c:1330       btstack_at_core_adapter.c:651
        │ 入 ring + notify              │ 翻译成 btstack_at_event_t
        ▼                               ▼ 入 ring + notify
  EVENT worker                     AT worker
        ▼                               ▼
  btstack_frontend_deliver_event   btstack_at_format_event
        ▼                               ▼
  config.xxx_callback(&event)      output_callback(char*, len)
  （结构化结构体）                  （"+NAME=v1,v2\r\n" 字符串）

                       └──────────────── 数据面：PCM ────────────────┘
手机 A2DP 媒体流 ──> btstack_a2dp_media_handler()
                       ├─ btstack_media_data_callback(&media)   ← 原始 SBC 包（有队列+memcpy）
                       └─ btstack_a2dp_media_process()          ← SBC 解码
                              ▼
                    btstack_core_event_mapper_media_audio()
                              ▼
                    btstack_core_media_audio_deliver()   ← 直调，不进队列
                              ▼
                    config.media_audio_callback(data, len, ctx)
```

**关键认知**：控制面走队列，数据面必须实时直调。原生 API 前端不做内容级去重；AT 前端会在格式化后做状态文本去重。原生 API 前端的扫描结果等 best-effort 事件可在队列满时丢弃；A2DP 状态和 PCM 配置走 critical event queue，仍应按队列满可能失败处理，但不应与 best-effort 扫描事件混为一谈。

---

## 2. profile 层：事件怎么被"打包上抛"

### 2.1 一对 handler 分别注册

[btstack_a2dp_sink.c#L245-247](../btstack/profiles/btstack_a2dp_sink.c#L245-L247)

```c
a2dp_sink_init();
a2dp_sink_register_packet_handler(btstack_a2dp_packet_handler); // 控制：信令/连接/配置
a2dp_sink_register_media_handler(btstack_a2dp_media_handler);   // 数据：AVDTP 媒体包
```

即：BRT 里 `hlp_a2dp_cbk()` 一个函数管的活，这里被**拆成两个 handler**。

### 2.2 控制面：`btstack_a2dp_packet_handler`

[btstack_a2dp_sink.c#L103-193](../btstack/profiles/btstack_a2dp_sink.c#L103-L193)

- 入口过滤：`packet_type != HCI_EVENT_PACKET` 或 `hci_event_packet_get_type() != HCI_EVENT_A2DP_META` 直接 return
- 取子事件：`hci_event_a2dp_meta_get_subevent_code(packet)`
- `switch (subevent)` 一串 `A2DP_SUBEVENT_*`，映射关系：

| 子事件 | 动作 |
|---|---|
| `SIGNALING_MEDIA_CODEC_SBC_CONFIGURATION` | 取采样率 → `media_configure` → `emit_pcm_config` |
| `SIGNALING_CONNECTION_ESTABLISHED` | 存 cid + peer 地址 → `set_state(CONNECTED)` |
| `STREAM_ESTABLISHED` | `set_state(STREAM_OPEN)` |
| `STREAM_STARTED` | `set_state(STREAMING)` |
| `STREAM_SUSPENDED` | `set_state(SUSPENDED)` |
| `STREAM_RELEASED` | `media_reset` + `set_state_event(CONNECTED, STREAM_CLOSED)` |
| `SIGNALING_CONNECTION_RELEASED` | `media_reset`；主动断开→IDLE，否则 `emit(CONNECTION_LOST)` |
| default | return |

### 2.3 `btstack_a2dp_emit` 做了什么

[btstack_a2dp_sink.c#L45-57](../btstack/profiles/btstack_a2dp_sink.c#L45-L57)

```c
static void btstack_a2dp_emit(btstack_a2dp_sink_event_type_t type)
{
    btstack_a2dp_sink_event_t event;
    memset(&event, 0, sizeof(event));
    event.type = type;                                  // 1. 填本层类型
    memcpy(event.address, btstack_a2dp_peer_address,    // 2. 从静态变量补地址
           sizeof(event.address));
    if (btstack_a2dp_config.event_callback != NULL) {
        btstack_a2dp_config.event_callback(&event);      // 3. 函数指针调出去
    }
}
```

**注意**：它不是"原封不动上抛"。它在**栈上新建结构体**、填 type、补地址；原始 `packet` 在 `packet_handler` 那层已被消化。

它的调用者 [`btstack_a2dp_set_state`](../btstack/profiles/btstack_a2dp_sink.c#L88-L101) 输入的是**状态枚举**，不是原始包：

```c
static void btstack_a2dp_set_state(btstack_a2dp_sink_state_t state)
{
    static const btstack_a2dp_sink_event_type_t event_types[] = {
        BTSTACK_A2DP_SINK_EVENT_DISCONNECTED, BTSTACK_A2DP_SINK_EVENT_CONNECTING,
        BTSTACK_A2DP_SINK_EVENT_CONNECTED,    BTSTACK_A2DP_SINK_EVENT_STREAM_OPEN,
        BTSTACK_A2DP_SINK_EVENT_STREAMING,    BTSTACK_A2DP_SINK_EVENT_SUSPENDED
    };
    btstack_a2dp_set_state_event(state, event_types[(uint8_t)state]);   // 枚举数值当数组下标
}
```

### 2.4 数据面：`btstack_a2dp_media_handler`

[btstack_a2dp_sink.c#L195-227](../btstack/profiles/btstack_a2dp_sink.c#L195-L227)

```c
static void btstack_a2dp_media_handler(uint8_t local_seid, uint8_t *packet, uint16_t size)
{
    btstack_media_packet_t media;
    (void)local_seid;
    if (btstack_a2dp_config.media_data_callback != NULL) {
        memset(&media, 0, sizeof(media));
        media.format = BTSTACK_AUDIO_FORMAT_SBC;
        media.data = packet;
        media.data_len = size;
        btstack_a2dp_config.media_data_callback(&media);   // ① 原始 SBC 包先抛出去
    }
    btstack_a2dp_media_process(packet, size);              // ② 再解码 → PCM
}
```

**① 与 ② 是两条不同通道**，很容易混：

| 通道 | 回调 | 内容 | 是否排队 |
|---|---|---|---|
| 原始媒体包（可选） | `btstack_media_data_callback` | 未解码 SBC 包 | 走队列，入队时 `memcpy` 深拷贝 |
| 解码后 PCM | `btstack_core_media_audio_deliver` | PCM S16 | 直调，不进队列 |

`btstack_media_data_callback` 的入队深拷贝见 [btstack_event_queue.c#L536-565](../btstack/core/btstack_event_queue.c#L536-L565)：

```c
memcpy(item.data, packet->data, packet->data_len);   // 解决"回调外指针失效"
(void)btstack_event_enqueue(&item);
```

> **当前工程的实际配置**：`btstack_profile_manager_setup_classic()` 只给
> `a2dp_config.event_callback` 和 `a2dp_config.audio_sample_callback` 赋值，
> 没有设置 `media_data_callback`。因此原始 SBC 包队列是协议栈提供的可选能力，
> 不是当前 `bt_xstack_demo.c` 的实际数据路径；当前业务只接收解码后的 PCM。

### 2.5 manager 在 setup 期就"配对"注册

[btstack_profile_manager.c#L581-632](../btstack/core/btstack_profile_manager.c#L581-L632)

```c
if (config->classic_profile_mask & BTSTACK_PROFILE_MANAGER_CLASSIC_A2DP_SINK) {
    a2dp_config.event_callback        = btstack_profile_manager_a2dp_event;    // 控制面
    a2dp_config.audio_sample_callback = btstack_core_event_mapper_media_audio; // 数据面
    status = btstack_a2dp_sink_setup(&a2dp_config);
}
...
if (config->classic_profile_mask & BTSTACK_PROFILE_MANAGER_CLASSIC_HFP_HF) {
    hfp_config.event_callback = btstack_profile_manager_hfp_event;             // 控制面
    status = btstack_hfp_hf_setup(&hfp_config);
    btstack_call_audio_tx_init();
    status = btstack_hfp_hf_set_direct_audio_callback(                         // 数据面
        btstack_core_event_mapper_call_audio);
}
```

**分叉点在注册期**：配置结构体里本来就是两个字段、连函数都不同。这是 xstack 与 BRT 的显著差异（BRT 在 `hlp_mgr_a2dp_cbk` 里运行期用 `case EV_A2DP_AV_DATA_IND: break;` 跳过 `event_cbk`）。

---

## 3. 转换层：`btstack_core_event_mapper_a2dp_event`

[btstack_event_mapper.c#L983-1023](../btstack/core/btstack_event_mapper.c#L983-L1023)

职责：把 profile 层的 `btstack_a2dp_sink_event_t` 翻译成前端统一的 `btstack_core_event_envelope_t`。

```c
void btstack_core_event_mapper_a2dp_event(const btstack_a2dp_sink_event_t *event)
{
    btstack_core_event_envelope_t envelope;
    btstack_a2dp_sink_state_t state;
    uint8_t value;

    if (event == NULL) return;
    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_AUDIO;
    btstack_core_copy_address(&envelope.data.audio.address, event->address);

    if (btstack_core_a2dp_event_state(event->type, &state)) {
        value = btstack_core_a2dp_state_value(state);       // 枚举数值重映射
        envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_A2DP_STATE;
        envelope.data.audio.values[0] = value;
        envelope.data.audio.value_count = 1u;
        (void)btstack_core_post_event(&envelope);
        if (event->type == BTSTACK_A2DP_SINK_EVENT_CONNECTED) {      // 一个源事件 → 两个前端事件
            envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_A2DP_DEVICE;
            envelope.data.audio.value_count = 0u;
        } else if (event->type == BTSTACK_A2DP_SINK_EVENT_CONNECTION_LOST) {
            envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_A2DP_LOST;
            envelope.data.audio.value_count = 0u;
        } else return;
        (void)btstack_core_post_event(&envelope);
        return;
    }

    switch (event->type) {
    case BTSTACK_A2DP_SINK_EVENT_PCM_CONFIG:
        envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_PCM_CONFIG;
        envelope.data.audio.values[0] = event->sample_rate;
        envelope.data.audio.values[1] = event->channel_count;
        envelope.data.audio.values[2] = event->bits_per_sample;
        envelope.data.audio.value_count = 3u;
        break;
    default: return;
    }
    (void)btstack_core_post_event(&envelope);
}
```

三个要点：
1. `domain` 字段决定后续走哪个回调（audio/service/phone/phonebook/le/gatt）
2. **枚举数值重映射**：profile 层的 state 数值 ≠ 前端 `BT_XSTACK_AUDIO_EVENT_*` 数值，中间靠 `btstack_core_a2dp_state_value()` 转
3. **一拆二**：`CONNECTED` 会同时产生 `A2DP_STATE` 和 `A2DP_DEVICE` 两个前端事件

对应地，PCM 数据面入口是同文件的 [btstack_event_mapper.c#L1079-1088](../btstack/core/btstack_event_mapper.c#L1079-L1088)：

```c
void btstack_core_event_mapper_media_audio(const btstack_audio_sample_t *sample)
{
    if ((sample == NULL) || (sample->format != BTSTACK_AUDIO_FORMAT_PCM_S16) ||
        (sample->data == NULL) || (sample->data_len == 0u)) return;
    btstack_core_media_audio_deliver(sample->data, sample->data_len);   // 直调
}
```

---

## 4. ★ 前端分叉点：`btstack_core_post_event` 的两个同名实现

### 4.1 原生前端版（本工程实际使用）

[bt_xstack_api_core.c#L1330-1363](../btstack/api/bt_xstack_api_core.c#L1330-L1363)

```c
int btstack_core_post_event(const btstack_core_event_envelope_t *event)
{
    ...
    control_event = bt_xstack_api_is_control_event(event);
    if ((event->domain == BT_XSTACK_EVENT_DOMAIN_PHONEBOOK) && ...) {
        pbap_record = true;
    } else if (/* service SCAN_RESULT 或 LE SCAN_RESULT */) {
        btstack_platform_enter_critical();
        if (btstack_ring_push(&bt_xstack_best_effort_event_queue, event)) result = 0;
        else { bt_xstack_best_effort_event_drop_count++; result = -1; }   // 允许丢
        btstack_platform_exit_critical();
        if (result == 0)
            (void)btstack_platform_notify(BTSTACK_WORKER_EVENT, BTSTACK_WORKER_SIGNAL_WORK);
        return result;
    }
    ...
}
```

### 4.2 AT 前端版

[btstack_at_core_adapter.c#L651-681](../btstack/at/btstack_at_core_adapter.c#L651-L681)

```c
int btstack_core_post_event(const btstack_core_event_envelope_t *source)
{
    btstack_at_event_t destination;
    if (source == NULL) return -1;
    memset(&destination, 0, sizeof(destination));
    switch (source->domain) {
    case BT_XSTACK_EVENT_DOMAIN_SERVICE:
        btstack_at_map_service(&source->data.service, &destination); break;
    case BT_XSTACK_EVENT_DOMAIN_PHONE:
        btstack_at_map_phone(&source->data.phone, &destination); break;
    case BT_XSTACK_EVENT_DOMAIN_AUDIO:
        btstack_at_map_audio(&source->data.audio, &destination); break;
    case BT_XSTACK_EVENT_DOMAIN_PHONEBOOK:
        destination.data.phonebook = source->data.phonebook;
        destination.id = (btstack_at_event_id_t)
            (BTSTACK_AT_EVENT_PBAPSTART + source->data.phonebook.id);
        break;
    case BT_XSTACK_EVENT_DOMAIN_LE:
        destination.id = BTSTACK_AT_EVENT_LE;
        destination.data.le = source->data.le; break;
    case BT_XSTACK_EVENT_DOMAIN_GATT:
        destination.id = BTSTACK_AT_EVENT_GATT;
        destination.data.gatt = source->data.gatt; break;
    case BT_XSTACK_EVENT_DOMAIN_CONNECTION:
    default:
        return 0;          // ← 静默丢弃，但返回值=成功（语义瑕疵）
    }
    return btstack_at_post_event(&destination);
}
```

**两个必须记住的差异**：

| 差异 | 原生前端 | AT 前端 |
|---|---|---|
| 输出形态 | 结构化结构体回调 | AT 字符串 |
| CONNECTION 域 | 有 `connection_callback` | **直接丢弃**，且返回"成功" |
| 重复状态事件 | 客户自己判断 | 前端做**内容级去重** |
| `btstack_frontend_deliver_event` | 真实现 | 空壳 `(void)event;` |

AT 版里的"空壳"（[btstack_at_core_adapter.c#L683-705](../btstack/at/btstack_at_core_adapter.c#L683-L705)）：

```c
void btstack_frontend_deliver_event(const btstack_core_event_envelope_t *event) { (void)event; }
void btstack_core_call_audio_deliver(const uint8_t *data, uint32_t length)
{ (void)data; (void)length; }
void btstack_core_media_audio_deliver(const uint8_t *data, uint32_t length)
{
    if ((btstack_at_pcm_callback != NULL) && (data != NULL) && (length != 0u))
        btstack_at_pcm_callback(data, length);      // PCM 仍然直调，不走 AT 字符串
}
```

**PCM 两条 deliver 在两个前端里都是"直调"**，这一点与 BRT 一致 —— 音频数据永远不能被字符串化和排队。

---

## 5. ★ AT 事件投递完整链路（核心）

### 5.1 全景图

```
[调用者线程] btstack_core_post_event()          adapter:651
      │ 翻译成 btstack_at_event_t
      ▼
[调用者线程] btstack_at_post_event()            btstack_at.c:652
      │ ① btstack_ring_push(&btstack_at_event_ring, event)     入环（临界区内）
      │ ② btstack_platform_notify(BTSTACK_WORKER_AT, WORK)     唤醒（临界区外）
      └─ return OK  ← 函数到此结束，不做任何格式化
   ┈┈┈┈┈┈┈┈┈┈┈┈ 跨任务边界（异步）┈┈┈┈┈┈┈┈┈┈┈┈┈
      ▼
[AT worker 任务] btstack_at_worker()            btstack_at.c:450
      │ for(;;) { 处理 AT 命令; btstack_at_drain_events(); 空闲则 wait }
      ▼
[AT worker 任务] btstack_at_drain_events()      btstack_at.c:284
      │ pending = ring_count()   取有界批量
      │ for (i < pending) { ring_pop(&event); btstack_at_emit_event(&event); }
      ▼
[AT worker 任务] btstack_at_emit_event()        btstack_at.c:257
      │ btstack_at_format_event(...)   ★ 才走到这里
      │ && !btstack_at_state_event_is_duplicate(...)   状态事件内容去重
      └→ btstack_at_emit(length)
      ▼
[AT worker 任务] btstack_at_emit()              btstack_at.c:55
      └→ config.output_callback(btstack_at_output, length, context)   交给客户
```

### 5.2 第 1 步：入队（不做格式化）

[btstack_at.c#L652-672](../btstack/at/btstack_at.c#L652-L672)

```c
int btstack_at_post_event(const btstack_at_event_t *event)
{
    bool pushed;

    if ((event == NULL) || (btstack_at_started == 0)) {
        return BTSTACK_AT_STATUS_NOT_READY;
    }
    btstack_platform_enter_critical();
    pushed = btstack_ring_push(&btstack_at_event_ring, event);      // ① 入环
    if (!pushed) {
        btstack_at_event_overflow_count++;
        btstack_at_event_rejected(event);                           // 环满 → 背压回调
    }
    btstack_platform_exit_critical();
    if (!pushed) {
        return BTSTACK_AT_STATUS_QUEUE_FULL;
    }
    (void)btstack_platform_notify(
        BTSTACK_WORKER_AT, BTSTACK_WORKER_SIGNAL_WORK);             // ② 唤醒 AT worker
    return BTSTACK_AT_STATUS_OK;
}
```

**为什么必须解耦**：`post_event` 可能运行在协议栈/中断上下文，只能做 O(1) 的入环操作；格式化（查表 + 十进制转换 + 去重比较）是"慢活"，必须丢到任务里做。环容量 `BTSTACK_AT_EVENT_QUEUE_LEN = 16`（[btstack_at.h#L18-19](../btstack/include/at/btstack_at.h#L18-L19)）。

### 5.3 第 2 步：何时被消费

[btstack_at.c#L450-479](../btstack/at/btstack_at.c#L450-L479)

```c
static void btstack_at_worker(void *context)
{
    btstack_at_input_item_t item;
    uint32_t signals = 0u;
    int did_work;

    (void)context;
    for (;;) {
        if ((signals & BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE) != 0u) {
            btstack_at_stack_ingress_space_available();
        }
        signals = 0u;
        did_work = 0;
        if (btstack_at_ring_pop(&btstack_at_input_ring, &item)) {   // 先处理 AT 命令
            btstack_at_process_item(&item);
            did_work = 1;
        }
        if (btstack_at_drain_events()) {                            // 再排空事件环
            did_work = 1;
        }
        if (!did_work) {
            signals = btstack_platform_wait(                        // 没事干才阻塞
                BTSTACK_WORKER_AT,
                BTSTACK_WORKER_SIGNAL_WORK | BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE,
                BTSTACK_WORKER_WAIT_FOREVER);
        }
    }
}
```

- worker 由 `btstack_at_start()` 创建，优先级 `BTSTACK_AT_TASK_PRIORITY = 2`，栈 `BTSTACK_AT_TASK_STACK_WORDS = 2048`
- `post_event` 的 `notify` 就是把 `btstack_platform_wait` 里阻塞的 worker 叫醒
- **每一轮循环末尾都会 drain 一次**，不只是被唤醒那一次 —— 这样命令处理期间积压的事件也会被顺带消费

### 5.4 第 3 步：drain（有界批量 pop）

[btstack_at.c#L284-309](../btstack/at/btstack_at.c#L284-L309)

```c
static int btstack_at_drain_events(void)
{
    btstack_at_event_t event;
    uint16_t pending;
    uint16_t index;
    int consumed = 0;

    if (!btstack_at_event_drain_ready()) return 0;      // 背压闸门
    btstack_platform_enter_critical();
    pending = btstack_ring_count(&btstack_at_event_ring);   // 先取条数，形成"有界批量"
    btstack_platform_exit_critical();
    for (index = 0u; index < pending; index++) {
        if (!btstack_at_event_drain_ready()) break;         // 每轮重新检查闸门
        if (!btstack_at_ring_pop(&btstack_at_event_ring, &event)) break;
        btstack_at_emit_event(&event);                      // 消费
        btstack_at_event_consumed(&event);                  // 消费后钩子
        consumed = 1;
    }
    return consumed;
}
```

三个设计点：
1. **有界批量**：`pending` 一开始取一次，避免长期霸占 worker 不让它处理 AT 命令与 notify
2. **闸门 `btstack_at_event_drain_ready()`**（[adapter L71-81](../btstack/at/btstack_at_core_adapter.c#L71-L81)）：PBAP 对端断开流程进行中时**暂停 drain**，保证事件顺序（该流程结束才放行）
3. **消费后钩子 `btstack_at_event_consumed()`**（[adapter L494-...](../btstack/at/btstack_at_core_adapter.c#L494-L520)）：`HFPLOST` 事件消费完才发起 PBAP 对端断开；并在 PBAP 空间曾被阻塞时请求 `PBAP_EVENT_SPACE_AVAILABLE`

### 5.5 第 4 步：format + 去重 + emit

[btstack_at.c#L257-272](../btstack/at/btstack_at.c#L257-L272)

```c
static void btstack_at_emit_event(const btstack_at_event_t *event)
{
    uint16_t length = 0u;

    if (btstack_at_format_event(
            event, btstack_at_output, sizeof(btstack_at_output), &length) == 0 &&
        !btstack_at_state_event_is_duplicate(event->id, btstack_at_output, length)) {
        btstack_at_emit(length);
    }
}
```

[btstack_at.c#L55-63](../btstack/at/btstack_at.c#L55-L63)（末端，真出口）

```c
static void btstack_at_emit(uint16_t length)
{
    if ((length != 0u) && (btstack_at_config.output_callback != NULL)) {
        btstack_at_config.output_callback(
            btstack_at_output, length, btstack_at_config.output_context);
    }
}
```

**状态事件去重**（AT 前端特有）：
- `btstack_at_event_is_state()`（[btstack_at.c#L77-103](../btstack/at/btstack_at.c#L77-L103)）列出 19 个"状态类" id（DEVSTAT / HFPSTAT / A2DPSTAT / PLAYSTAT / ABSVOL ...）
- `btstack_at_state_event_is_duplicate()`（[btstack_at.c#L105-133](../btstack/at/btstack_at.c#L105-L133)）缓存每个 id 上一次的输出文本，**逐字节比较**相同就抑制不报
- 缓存数组按 id 下标索引：`btstack_at_last_state_event[BTSTACK_AT_EVENT_ABSVOL + 1u]`，即缓存容量绑定枚举最大值

**失败语义**：容量不足 / 拼装失败时 `format_event` 返回非 0，**整条事件不发**（不给客户端半截脏数据），而不是截断输出。

---

## 6. `btstack_at_format_event` 详解

位置：[btstack_at_protocol.c#L418-767](../btstack/at/btstack_at_protocol.c#L418-L767)，是**两段式**结构。

### 6.1 第一段：numeric 事件名表短路

[btstack_at_protocol.c#L432-440](../btstack/at/btstack_at_protocol.c#L432-L440)

```c
name = btstack_at_numeric_event_name(event->id);        // 查表
if (name != NULL) {
    btstack_at_writer_prefix(&writer, name);            // "+NAME="
    btstack_at_writer_uint_fields(&writer,              // "v1,v2,v3"
        event->data.numeric.values, event->data.numeric.value_count);
    btstack_at_writer_line_end(&writer);                // "\r\n"
    return btstack_at_writer_finish(&writer, output_length);
}
```

表定义（[L381-400](../btstack/at/btstack_at_protocol.c#L381-L400)），17 项，全是"纯数值"事件：

```
DEVSTAT / PWRSTAT / MICMUTED / HFPAUDIO / HFPVR / HFPSIG / HFPROAM /
HFPBATT / HFPIBR / HFPRING / A2DPSTAT / A2DPPCMCONFIG / AVRCPSTAT /
PLAYSTAT / PLAYMODE / TRACKSTAT / ABSVOL
```

查找方式（[L402-416](../btstack/at/btstack_at_protocol.c#L402-L416)）是**线性遍历**，未命中返回 `NULL`。

举例：A2DP 状态事件 → `+A2DPSTAT=1\r\n`；PCM 配置 → `+A2DPPCMCONFIG=44100,2,16\r\n`。

### 6.2 第二段：大 switch（结构复杂的事件）

命不中表的走 `switch (event->id)`，逐个 case 手工拼字段。已覆盖：

| case | 前缀 | 说明 |
|---|---|---|
| `BTSTACK_AT_EVENT_LE` | `LEADV` / `LESCAN` / `LECONN` / `LEPAIR` | **按 `le->type` 二次派生前缀**，再拼 type、status；SCAN_RESULT 还补 address_type / address / rssi |
| `BTSTACK_AT_EVENT_GATT` | 按 `gatt->type` 派生 Service/Characteristic/Descriptor/MTU/Complete | SERVICE/CHARACTERISTIC/DESCRIPTOR/MTU/CLIENT_COMPLETE/SERVER_COMPLETE |
| `BTSTACK_AT_EVENT_PBAPSTART` | `PBAPSTART` | |
| `BTSTACK_AT_EVENT_PBAPCONTACT` | `PBAPCONTACT` | 电话本条目 |
| `BTSTACK_AT_EVENT_PBAPCALL` | `PBAPCALL` | |
| `BTSTACK_AT_EVENT_PBAPCATEND` | `PBAPCATEND` | |
| `BTSTACK_AT_EVENT_PBAPDONE` | `PBAPDONE` | |
| `BTSTACK_AT_EVENT_PBAPAUTHREQ` | `PBAPAUTHREQ` | |
| `BTSTACK_AT_EVENT_PBAPERROR` | `PBAPERROR` | |
| `BTSTACK_AT_EVENT_SCAN_RESULT` | `SCAN` | 经典蓝牙扫描结果 |
| `BTSTACK_AT_EVENT_SCAN_END` | — | |
| `BTSTACK_AT_EVENT_PAIR_REQUEST` | `PAIRREQ` | |
| `BTSTACK_AT_EVENT_PAIRED` | `PAIRED` | |
| `BTSTACK_AT_EVENT_HFPSTAT` | `HFPSTAT` | |
| `BTSTACK_AT_EVENT_HFPDEV` / `A2DPDEV` | 地址 | |
| `BTSTACK_AT_EVENT_HFPNET` / `HFPMANU` / `HFPNUM` | name 变量 | |
| `BTSTACK_AT_EVENT_HFPLOST` / `A2DPLOST` | 地址 | |
| `BTSTACK_AT_EVENT_HFPMTPX` | `HFPMTPX` | |
| `BTSTACK_AT_EVENT_TRACKINFO` | `TRACKINFO` | title / artist / album 用 `0x0c` 分隔 |
| `default` | — | `writer.failed = 1;`（不输出） |

### 6.3 writer 机制（统一的容量保护）

[btstack_at_protocol.c#L21-33](../btstack/at/btstack_at_protocol.c#L21-L33)

```c
static void btstack_at_writer_byte(btstack_at_writer_t *writer, char value)
{
    if (writer->failed != 0) return;                                  // 已失败，后续全丢
    if (writer->length >= writer->capacity) { writer->failed = 1; return; }
    writer->data[writer->length++] = value;
}
```

配套基础设施：

| 函数 | 作用 |
|---|---|
| `btstack_at_writer_prefix()` [L192-199](../btstack/at/btstack_at_protocol.c#L192-L199) | 输出 `+NAME=` |
| `btstack_at_writer_line_end()` [L201-205](../btstack/at/btstack_at_protocol.c#L201-L205) | 输出 `\r\n` |
| `btstack_at_writer_uint_fields()` [L362-374](../btstack/at/btstack_at_protocol.c#L362-L374) | 输出 `v1,v2,v3`（逗号分隔） |
| `btstack_at_writer_finish()` | 成功返回 0，`failed` 时返回非 0 |

缓冲：`static char btstack_at_output[BTSTACK_AT_OUTPUT_MAX]`，`BTSTACK_AT_OUTPUT_MAX = 600`（[btstack_at.h#L22-23](../btstack/include/at/btstack_at.h#L22-L23)）。

### 6.4 同文件里的其他格式化入口（命令响应，不是事件）

这些是"AT 命令 → 响应"方向用的，与事件方向相对：

| 函数 | 输出形态 |
|---|---|
| `btstack_at_format_simple()` [L215-224](../btstack/at/btstack_at_protocol.c#L215-L224) | `OK\r\n` 或 `ERROR\r\n`（`btstack_at_emit_simple()` 调用） |
| `btstack_at_format_address_response()` [L226-242](../btstack/at/btstack_at_protocol.c#L226-L242) | `+NAME=AA:BB:..\r\nOK\r\n` |
| `btstack_at_format_uint_line()` [L244-263](../btstack/at/btstack_at_protocol.c#L244-L263) | `+NAME=<dec>\r\n[OK\r\n]` |
| `btstack_at_format_text_response()` [L265-281](../btstack/at/btstack_at_protocol.c#L265-L281) | `+NAME=text\r\nOK\r\n` |
| `btstack_at_format_hex_response()` [L283-300](../btstack/at/btstack_at_protocol.c#L283-L300) | `+NAME=0xXXXX\r\nOK\r\n` |
| `btstack_at_format_plist_record()` [L302-326](../btstack/at/btstack_at_protocol.c#L302-L326) | `+PLIST=<i>,<0x0c>,<mask>,<0x0c>,<addr>[<0x0c>name]\r\n` |
| `btstack_at_format_track_info()` [L341-360](../btstack/at/btstack_at_protocol.c#L341-L360) | `+TRACKINFO=title<0x0c>artist<0x0c>album\r\n` |

### 6.5 初始化与入口（AT 前端如何被启用）

[bt_xstack_at_frontend.c#L18-47](../btstack/at/bt_xstack_at_frontend.c#L18-L47)

```c
int bt_xstack_init(bt_event_callback_t event_callback, void *context)
{
    btstack_at_config_t config;
    ...
    config.output_callback = event_callback;    // ← 客户回调直接作为 AT 字符串出口
    config.output_context = context;
    config.task_priority = 0u;                  // 0 = 用默认 BTSTACK_AT_TASK_PRIORITY
    config.task_stack_size = 0u;                // 0 = 用默认 BTSTACK_AT_TASK_STACK_WORDS
    status = btstack_runtime_init();
    status = btstack_at_init(&config);
    status = btstack_at_core_adapter_init();
    btstack_avrcp_cover_art_set_callback_enabled(bt_xstack_at_cover_art_callback != NULL);
    status = btstack_at_start();               // ← 建 ring + 建 worker 任务
    ...
}
```

**AT 前端里 `output_callback` 的签名就是 `bt_event_callback_t`**。但 API 和 AT 两个 frontend
虽然导出同名 `bt_xstack_init`，函数签名并不相同：API 版为
`bt_xstack_init(const bt_xstack_config_t *config)`，AT 版为
`bt_xstack_init(bt_event_callback_t callback, void *context)`。切换 frontend 时必须同时切换
链接库、包含的头文件和应用初始化代码；不能让同一份应用源码不改动地直接换库。

---

## 7. 本工程实际用的是哪个前端？

证据（编译映射文件，非源码）：`prjDebug\bin\Debug\project.elf.map` 中

```
bt_xstack_init    ..\..\gplib\liba_wifi_src\atbm\atbm6165\bt_api\libxstack_api.a(bt_xstack_api_core.o)
```

即链接的是 **`bt_xstack_api_core.o`（原生前端）**，不是 `bt_xstack_at_frontend.o`。

应用侧调用点：[bt_xstack_demo.c#L887](../Project/wifi_demo/bt_xstack_demo.c#L887)

```c
status = bt_xstack_init(&config);    // 创建/初始化 xstack 并把事件出口交给应用
```

**结论**：`btstack/at/` 这套 AT 前端在本工程中是**备用/可选**实现，用于串口透传或上位机联调场景；本工程 UI 走的是原生结构化回调。
> 但理解 AT 链路仍有价值：它把"事件 → 文本"这一步显式摊开，是理解整个事件流的绝佳切口。

---

## 8. 与 BRT blue SDK 的类比对表

| BRT（ATBM6463） | xstack 原生前端 | xstack AT 前端 |
|---|---|---|
| `hlp_a2dp_cbk()`（控制+数据一把抓） | `btstack_a2dp_packet_handler` + `btstack_a2dp_media_handler`（**两个** handler） | 同左 |
| `hlp_mgr_a2dp_cbk()` 运行期 `case EV_A2DP_AV_DATA_IND: break;` 跳过 `event_cbk` | 注册期就分两个字段：`event_callback` / `audio_sample_callback` | 同左 |
| `event_a2dp_indication()` | `btstack_a2dp_emit()` | `btstack_a2dp_emit()` |
| `bt_event_indicate()`（格式化并调用客户回调） | `btstack_frontend_deliver_event()`（结构体交付） | **`btstack_at_emit_event()`（format、去重、输出）** |
| `carlink_bt_callback()` | `config.<domain>_callback()` | `config.output_callback()` |
| PCM 直调 | `btstack_core_media_audio_deliver()` 直调 | 同左 |

### 两个曾走偏的类比（已纠正）

1. ❌ "`btstack_a2dp_emit` 等价 `bt_event_indicate`"
   → 错位一层。`btstack_a2dp_emit` ≈ BRT 的 `event_a2dp_indication`（profile 层打包上抛，还没到"翻译"）。
2. ❌ "`btstack_a2dp_emit` 把事件原封不动给上层"
   → 不成立。它在栈上**新建结构体**、填 `type`、补 `address`；原始包在 `packet_handler` 已消化；真正的"转换"在下一跳 `btstack_core_event_mapper_a2dp_event()`（枚举重映射 + 一拆二 + 填 values[]）。

### 前端末端交付的精确对应

`btstack_frontend_deliver_event()` 与 `btstack_at_format_event()`不处于完全同一层次：前者在
API Event worker 中直接分派结构化事件，后者只负责 AT 文本格式化。若比较两种 frontend 的
完整末端交付，API 侧是 `btstack_frontend_deliver_event()`，AT 侧是
`btstack_at_emit_event()`（其中包含 format、去重和 output callback）。

---

## 9. 值得注意的风险点 / 语义瑕疵

| # | 位置 | 问题 |
|---|---|---|
| 1 | [btstack_at_core_adapter.c#L651-681](../btstack/at/btstack_at_core_adapter.c#L651-L681) | `CONNECTION` 域被静默丢弃，但 `return 0` 表示成功 → 调用方无法感知丢事件 |
| 2 | [btstack_at_core_adapter.c#L620-649](../btstack/at/btstack_at_core_adapter.c#L620-L649) | `btstack_at_map_audio` 用 `ids[source->id]` **直接当数组下标**，无范围检查（枚举数值隐式契约） |
| 3 | [btstack_a2dp_sink.c#L88-101](../btstack/profiles/btstack_a2dp_sink.c#L88-L101) | 同上：`event_types[(uint8_t)state]` 直接下标 |
| 4 | `btstack_core_post_event` 两个同名实现 | **同名符号不能同时链接**；换前端必须换整包 `.a`，不能混编 |
| 5 | [btstack_at.c#L547-550](../btstack/at/btstack_at.c#L547-L550) | 事件环容量仅 16，环满即丢（`btstack_at_event_overflow_count` 计数） |
| 6 | AT 前端状态事件去重 | 若客户依赖"周期收到同一状态"，AT 前端不会给（内容相同被抑制） |

---

## 10. 对本工程蓝牙 UI 的启示

1. **UI 走的是原生前端**，所以 UI 层要接的是 `bt_xstack_config_t` 里那 10 个 domain 回调（[bt_xstack.h#L17-34](../btstack/include/api/bt_xstack.h#L17-L34)），不是 AT 字符串：
   `service_callback / connection_callback / phone_callback / audio_event_callback / phonebook_callback / le_callback / gatt_callback / cover_art_callback / media_audio_callback / call_audio_callback`
2. **PCM 回调是裸指针 + "仅在回调期间有效"契约**，UI 线程不能存指针、不能做重活（要按 `btstack_event_queue.c` 的做法先 `memcpy`）。
3. **事件回调在 worker 上下文**，不是中断；但也不要在里面做耗时 UI 绘制 —— 应由 `bt_xstack_demo.c` 已有回调转成消息投递给显示任务（参考 `bt_ui_events.c`）。
4. **状态去重/同态刷新**由 UI 层自己做（原生前端不做去重），否则重复绘制浪费带宽。
5. **`BT_XSTACK_AUDIO_EVENT_A2DP_STATE` 与 `A2DP_DEVICE` 是两个事件**，UI 状态机要分别处理（前者管连接状态图标，后者管设备名/地址显示）。

---

## 11. 代码索引（本次涉及）

| 文件 | 关键位置 | 内容 |
|---|---|---|
| `btstack/profiles/btstack_a2dp_sink.c` | L45-57 / L88-101 / L103-193 / L195-227 / L245-247 | emit / set_state / packet_handler / media_handler / 注册 |
| `btstack/core/btstack_event_mapper.c` | L983-1023 / L1079-1088 | A2DP 事件翻译 / PCM 数据面入口 |
| `btstack/core/btstack_profile_manager.c` | L581-632 | A2DP / HFP 回调配对注册 |
| `btstack/core/btstack_event_queue.c` | L52-55 / L153-165 / L536-565 | 4 个静态回调指针 / 媒体包出队 / 入队 memcpy |
| `btstack/api/bt_xstack_api_core.c` | L1330-1363 / L1393-1440 / L1455-1477 | 原生前端 post_event / deliver_event / PCM deliver |
| `btstack/include/api/bt_xstack.h` | L17-34 | 10 个回调字段 |
| `btstack/include/api/bt_xstack_audio.h` | L1-60 | audio 事件 id 枚举 + 回调 typedef |
| `btstack/at/btstack_at_core_adapter.c` | L71-81 / L484-520 / L620-649 / L651-681 / L683-705 | drain 闸门 / 消费钩子 / map_audio / post_event / 空壳 |
| `btstack/at/btstack_at.c` | L55-63 / L65-75 / L77-133 / L257-272 / L284-309 / L450-479 / L481-516 / L518- / L652-672 | emit / emit_simple / 状态判定与去重 / emit_event / drain / worker / init / start / post_event |
| `btstack/at/btstack_at_protocol.c` | L21-33 / L192-205 / L215-360 / L362-374 / L381-416 / L418-767 | writer / prefix+line_end / 响应格式化 / uint_fields / 事件名表 / format_event |
| `btstack/at/btstack_at_internal.h` | L157-231 / L317-324 | AT 事件 id 枚举 / btstack_at_event_t / 内部钩子声明 |
| `btstack/include/at/btstack_at.h` | L14-23 / L53-56 | 队列容量与输出缓冲宏 / init + start |
| `btstack/at/bt_xstack_at_frontend.c` | L18-47 / L49-53 / L55-59 | AT 前端 init / send_at / PCM 注册 |
| `Project/wifi_demo/bt_xstack_demo.c` | L879-887 | 回调注册 + `bt_xstack_init` |

---

## 12. 一句话总结

> **`btstack_at_post_event` 只负责"入环 + 叫醒 AT worker"；事件在 AT worker 任务里被 `drain_events` 逐条 pop 出来，交给 `emit_event` → `format_event` 拼成 `+NAME=v1,v2\r\n` 字符串，去重后经 `output_callback` 交付客户。** `post` 在调用者线程，`format` 在 AT worker 线程，两者靠环形队列 + notify 解耦。

# AVRCP 完整链路追踪（车机 ↔ 手机）

> 本文按「向下注册 → 事件上行 → 命令下行 → Cover Art」四站顺序记录 AVRCP 全部链路。
> 涉及文件均在 `btstack/` 黑名单内，仅因学习任务点名而定点只读，未做任何改动。

------

## 0. 先建立 3 个前提认知

**① AVRCP 没有自己的连接。** 它挂在 A2DP 上——A2DP 一 `CONNECTED`，manager 就自动发起 AVRCP：

[btstack_profile_manager.c#L346-L352](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/core/btstack_profile_manager.c#L346-L352)

```c
if ((event->type == BTSTACK_A2DP_SINK_EVENT_CONNECTED) &&
    !btstack_avrcp_is_connected() &&
    (btstack_avrcp_get_state() == BTSTACK_AVRCP_STATE_IDLE)) {
    btstack_core_state_set_profile_peer(BT_XSTACK_PROFILE_AVRCP, &address);
    (void)btstack_avrcp_connect(mapped.address);   // ★ 自动连
}
```

**② AVRCP = 1 个主壳 + 3 个子模块**（与 A2DP/HFP 的「单一模块」不同）：

| 模块 | 文件 | 职责 |
|---|---|---|
| 主壳 | [btstack_avrcp.c](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp.c) | 只管连接生命周期（2 个 case）+ 唯一事件出口 `internal_emit` |
| controller | [btstack_avrcp_controller.c](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_controller.c) | 业务全在这（11 个 case）+ 排队/重试机 + 元数据缓存 |
| target | [btstack_avrcp_target.c](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_target.c) | 极简桩：2 个 case，`PLAY_STATUS_QUERY` 硬编码回 `STOPPED` |
| cover_art | [btstack_avrcp_cover_art.c](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_cover_art.c) | 独立 OBEX/BIP 通路，不是 AVRCP 协议 |

主壳与子模块之间靠 [btstack_avrcp_internal.h](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_internal.h) 通信（`internal_get_cid` / `internal_emit` / `controller_*` / `target_*`）。

**③ 双角色 + 双 SDP 记录。** 车机同时扮演 controller（遥控手机）和 target（被手机遥控），因此有两条 SDP：`controller_sdp_handle` / `target_sdp_handle`（[manager L601-L604](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/core/btstack_profile_manager.c#L601-L604)）。

------

# 第①站：向下注册链

AVRCP 有 **3 条独立注册**（对比 A2DP 只 1 条、HFP 只 2 条）。

### ①-1 入口：manager 填配置 → 交给主壳

[manager L595-L613](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/core/btstack_profile_manager.c#L595-L613)：

```c
memset(&avrcp_config, 0, sizeof(avrcp_config));
avrcp_config.controller_service_name = "AVRCP Controller";
avrcp_config.target_service_name = "AVRCP Target";
avrcp_config.manufacturer = config->manufacturer;
avrcp_config.controller_sdp_handle = config->classic.avrcp_controller_sdp_handle;
avrcp_config.target_sdp_handle = config->classic.avrcp_target_sdp_handle;
avrcp_config.event_callback = btstack_profile_manager_avrcp_event;   // ★ 上行接收入口
btstack_avrcp_setup(&avrcp_config);
btstack_avrcp_cover_art_setup(btstack_core_event_mapper_cover_art);  // ★ 独立通路
```

### ①-2 主壳 `btstack_avrcp_setup`

[btstack_avrcp.c#L126-L175](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp.c#L126-L175)：

```c
btstack_avrcp_config = *config;          // ★ 结构体拷贝 —— 配置就在这里存下来
btstack_avrcp_cid = 0u;
avrcp_init();
avrcp_register_packet_handler(btstack_avrcp_packet_handler);   // ★注册 1：主壳 handler
btstack_avrcp_controller_setup(config);                        // ★注册 2：controller
btstack_avrcp_target_setup(config);                            // ★注册 3：target
```

### ①-3 controller 自己的 setup

[controller L354-L394](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_controller.c#L354-L394)：

```c
btstack_avrcp_play_status = AVRCP_PLAYBACK_STATUS_STOPPED;   // 初值
btstack_avrcp_pending_requests = 0u;
btstack_avrcp_metadata_inflight = false;
memset(title/artist/album, 0, ...);
avrcp_controller_init();
avrcp_controller_register_packet_handler(btstack_avrcp_controller_packet_handler);  // ★注册 2 落地
controller_features = PLAYER_OR_RECORDER | COVER_ART_GET_IMAGE_PROPERTIES | GET_IMAGE | GET_LINKED_THUMBNAIL;
avrcp_controller_create_sdp_record(...);
sdp_register_service(...);
```

**注册链全景**：

```plain
manager btstack_profile_manager_setup_classic (L595)
  └ btstack_avrcp_setup (avrcp.c L126)
      ├ avrcp_register_packet_handler(主壳 handler)        avrcp.c L151  → 只管连接 2 case
      ├ btstack_avrcp_controller_setup
      │   └ avrcp_controller_register_packet_handler(controller handler)  controller.c L370  → 11 case 业务
      └ btstack_avrcp_target_setup
          └ avrcp_target_register_packet_handler(target handler)          target.c → 2 case 桩
  └ btstack_avrcp_cover_art_setup(btstack_core_event_mapper_cover_art)   manager.c L610
      └ btstack_bip_client_init(cover_art_bip_event)                      cover_art.c L161  ← BIP，不是 AVRCP
```

**关键点**：`event_callback = btstack_profile_manager_avrcp_event` 是**唯一上行出口收件人**，三个子模块都不直接调上层，全部经 `btstack_avrcp_internal_emit`。

### ①-4 连接建立时三个子模块被一起唤醒

[avrcp.c L52-L74](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp.c#L52-L74)：

```c
case AVRCP_SUBEVENT_CONNECTION_ESTABLISHED:
    btstack_avrcp_cid = ..._get_avrcp_cid(packet);
    ..._get_bd_addr(packet, btstack_avrcp_peer_address);
    btstack_avrcp_set_state(BTSTACK_AVRCP_STATE_CONNECTED);
    btstack_avrcp_cover_art_connected(btstack_avrcp_peer_address);   // → 起 OBEX 连接
    btstack_avrcp_controller_connected(btstack_avrcp_cid);          // → 挂通知 + 查状态
    btstack_avrcp_target_connected(btstack_avrcp_cid);
```

[controller L396-L411](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_controller.c#L396-L411) 一连上就做 4 件事：

```c
avrcp_controller_get_supported_events(avrcp_cid);                         // 问手机支持哪些通知
avrcp_controller_enable_notification(cid, PLAYBACK_STATUS_CHANGED);       // ★挂通知（one-shot）
avrcp_controller_enable_notification(cid, TRACK_CHANGED);                 // ★挂通知（one-shot）
avrcp_controller_get_play_status(avrcp_cid);                              // 主动查一次播放状态
btstack_avrcp_update_progress_timer();                                    // 起 1s 进度轮询
```

------

# 第②站：事件上行（手机 → 车机）

```plain
手机 → AVCTP/L2CAP → 原生 avrcp.c → HCI_EVENT_AVRCP_META
  → ① 主壳 btstack_avrcp_packet_handler（只接 2 个连接类 case）
  → ② controller btstack_avrcp_controller_packet_handler（11 个业务 case）
  → ③ internal_emit ★唯一总出口（附加元数据）
  → ④ manager btstack_profile_manager_avrcp_event（改地址/存全局/联动）
  → ⑤ mapper btstack_core_event_mapper_avrcp_event（换信封）
  → ⑥ core_post_event ★入事件队列
  ═══ 跨任务边界 ═══
  → ⑦ bt_xstack_event_task 出队（api_core.c L243）
  → ⑧ btstack_frontend_deliver_event（按 domain 分发，L1393）
  → demo audio_event / connection_event 回调
```

### ②-1 主壳 handler：只认连接生命周期

[avrcp.c L37-L93](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp.c#L37-L93)：只处理 `CONNECTION_ESTABLISHED` / `CONNECTION_RELEASED`，其余 `default: return`。

状态变化走 [set_state L27-L35](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp.c#L27-L35)（去重 + 把枚举值直接当事件类型发出去）：

```c
if (btstack_avrcp_state == state) return;      // 去重：同状态不重复发
btstack_avrcp_state = state;
btstack_avrcp_internal_emit((btstack_avrcp_event_type_t)state, NULL, 0u);
```

### ②-2 controller handler：11 个业务 case

[controller L216-L351](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_controller.c#L216-L351)：

| subevent | 动作 | 注意 |
|---|---|---|
| `NOTIFICATION_PLAYBACK_STATUS_CHANGED` | 存 play_status → emit `PLAY_STATUS` → **重挂通知** | L242-245 one-shot 重挂 |
| `PLAY_STATUS` | emit `TRACK_STATUS`（values = 状态/位置秒/总长秒） | 3 个值 |
| `NOTIFICATION_TRACK_CHANGED` | cover_art 复位 → emit `TRACK_CHANGED` → **重挂通知** | L267-272 |
| `SHUFFLE_AND_REPEAT_MODE` | 归一成 0/1 → emit `PLAY_MODE` | 2 个值 |
| `NOTIFICATION_VOLUME_CHANGED` | emit `ABSOLUTE_VOLUME` | |
| `SET_ABSOLUTE_VOLUME_RESPONSE` | emit `ABSOLUTE_VOLUME` | |
| `NOW_PLAYING_TITLE_INFO` | **只存不发** `btstack_avrcp_title` | L301-306 |
| `NOW_PLAYING_ARTIST_INFO` | **只存不发** `btstack_avrcp_artist` | L308-313 |
| `NOW_PLAYING_ALBUM_INFO` | **只存不发** `btstack_avrcp_album` | L315-320 |
| `NOW_PLAYING_INFO_DONE` | `metadata_complete()`，成功才 emit `TRACK_INFO` | L330-337 元数据在这里成组发出 |
| `OPERATION_COMPLETE` | 抠字节兜底（见 ②-4） | L339-346 |

**元数据「搭车」机制**：标题/歌手/专辑三个 case 都只写静态变量、不 emit；等 `INFO_DONE` 时才 emit 一个 `TRACK_INFO`。

### ②-3 `internal_emit`：全 AVRCP 唯一总出口

[avrcp.c L101-L124](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp.c#L101-L124)：

```c
memset(&event, 0, sizeof(event));
event.type = type;
memcpy(event.address, btstack_avrcp_peer_address, sizeof(event.address));
if (value_count > 3u) value_count = 3u;      // 上限 3 个值
for (...) event.values[index] = values[index];
event.value_count = value_count;
btstack_avrcp_controller_copy_metadata(&event);      // ★ 元数据无条件搭车（L120）
if (btstack_avrcp_config.event_callback != NULL)
    btstack_avrcp_config.event_callback(&event);     // → manager
```

[copy_metadata L415-L427](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_controller.c#L415-L427) 把 title/artist/album 拷进每一条事件——所以 `PLAY_STATUS` 这种事件也带着当前歌曲信息。

### ②-4 `OPERATION_COMPLETE` 抠字节兜底

[controller L339-L346](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_controller.c#L339-L346)：

```c
if ((size >= 10u) &&
    (packet[7] == AVRCP_PDU_ID_GET_ELEMENT_ATTRIBUTES) &&
    (packet[9] == ERROR_CODE_PAGE_TIMEOUT) &&
    btstack_avrcp_metadata_inflight) {
    btstack_avrcp_metadata_complete(ERROR_CODE_PAGE_TIMEOUT);
}
```

某些手机会用 `OPERATION_COMPLETE` + PAGE_TIMEOUT 报错、而不发 `NOW_PLAYING_INFO_DONE`。没这段兜底，`metadata_inflight` 会**永久卡 true**，之后所有元数据请求都被门闩挡死。

### ②-5 manager 二次加工

[manager L357-L432](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/core/btstack_profile_manager.c#L357-L432) 做三件事：

```c
// 1) 地址改写：把 AVRCP 视角的地址解析成全局 profile peer 地址
btstack_profile_manager_resolve_peer(BT_XSTACK_PROFILE_AVRCP, mapped.address, &address);
memcpy(mapped.address, address.bytes, BT_XSTACK_ADDRESS_LEN);

// 2) 状态类（type <= CONNECTED）：存全局 + 发一封 CONNECTION 事件 + 主动拉元数据
if (btstack_profile_manager_avrcp_event_state(event->type, &state)) {
    manager_classic_state.avrcp_state = value;
    btstack_core_event_mapper_post_connection(BT_XSTACK_PROFILE_AVRCP, ...);   // ★ 一封变两封
    if (event->type == BTSTACK_AVRCP_EVENT_CONNECTED) {
        (void)btstack_avrcp_request_metadata();     // ★ 连上就拉元数据
    }
}

// 3) 业务类：存全局 + 联动
case BTSTACK_AVRCP_EVENT_TRACK_CHANGED: (void)btstack_avrcp_request_metadata(); break;  // ★ 切歌也拉
```

### ②-6 mapper 换信封

[event_mapper.c L1032-L1077](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/core/btstack_event_mapper.c#L1032-L1077)：把 `btstack_avrcp_event_t` 装进 `btstack_core_event_envelope_t{domain=AUDIO, data.audio{id,values}}`，然后 `btstack_core_post_event(&envelope)` 入环。

**类型映射表**：

| AVRCP 事件 | 信封 domain | `audio.id` |
|---|---|---|
| `type <= CONNECTED`（状态类） | AUDIO | `AVRCP_STATE` |
| `PLAY_STATUS` | AUDIO | `PLAY_STATE` |
| `PLAY_MODE` | AUDIO | `PLAY_MODE` |
| `TRACK_STATUS` | AUDIO | `TRACK_STATE` |
| `TRACK_INFO` | AUDIO | `TRACK_INFO`（带 title/artist/album） |
| `ABSOLUTE_VOLUME` | AUDIO | `ABSOLUTE_VOLUME` |
| `TRACK_CHANGED` | — | **直接 return，不发**（只用于触发拉元数据） |

> 注意 `TRACK_CHANGED` 在 [L1060-L1061](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/core/btstack_event_mapper.c#L1060-L1061) 被丢掉——它的唯一作用是让 manager 去拉元数据。

### ②-7 跨任务 → EVENT worker → 应用回调

[api_core.c L243-L275](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/api/bt_xstack_api_core.c#L243-L275) 出队，[L1393-L1440](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/api/bt_xstack_api_core.c#L1393-L1440) 按 domain 分派给 `bt_xstack_config_t` 里登记的回调：

```c
case BT_XSTACK_EVENT_DOMAIN_AUDIO:
    config.audio_event_callback(&event->data.audio, config.context);   // → demo bt_xstack_demo_audio_event
case BT_XSTACK_EVENT_DOMAIN_CONNECTION:
    config.connection_callback(&event->data.connection, config.context);// → demo connection_event
```

demo 落点：

- [bt_xstack_demo.c L560-L568](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/Project/wifi_demo/bt_xstack_demo.c#L560-L568) `AUDIO_EVENT_AVRCP_STATE` → 写 `bt_xstack_demo_avrcp_connected`（与 connection_event **双写**，兼容只走 audio 事件上报的手机）
- [bt_xstack_demo.c L587-L588](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/Project/wifi_demo/bt_xstack_demo.c#L587-L588) `AUDIO_EVENT_TRACK_INFO` → `bt_ui_post_track_info(title, artist, album)`

**上行动三站记法**（判断链上"现在是哪一站"）：

| 站 | 函数 | 干什么 | 判断标志 |
|---|---|---|---|
| ④ | `profile_manager_avrcp_event` | **记账**：改地址、存全局、联动别的 profile | 输入输出**同类型** |
| ⑤ | `event_mapper_avrcp_event` | **换信封**：填 domain/id/values | **类型变了** ← 看到类型变就到这站 |
| ⑥ | `core_post_event` | **投邮筒**：入环 + notify，不调任何回调 | 无返回处理逻辑 |

------

# 第③站：控制命令下行（车机 → 手机）

> ⚠️ **两条任务边界**，不是一条。中间还有一次 `post_stack_callback` 投递。

### ① 按键入口

```c
ap_peripheral 驱动（按键中断/轮询）
  → bt_xstack_demo_media_key_handler(action)      demo L816-860
```

注册点：[demo L896-L898](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/Project/wifi_demo/bt_xstack_demo.c#L896-L898) `ap_peripheral_media_key_handler_register(bt_xstack_demo_media_key_handler)`

### ② 应用 API 层（薄壳，只包参数）

[api_core.c#L914-L926](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/api/bt_xstack_api_core.c#L914-L926)：

```c
bt_xstack_result_t bt_xstack_avrcp_next(void) {
    return bt_xstack_api_avrcp_control(BT_XSTACK_AVRCP_CONTROL_NEXT);
}
static bt_xstack_result_t bt_xstack_api_avrcp_control(uint32_t control) {
    return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_AVRCP_CONTROL, control);
}
```

→ 组包成 `btstack_core_request_t{ id=REQUEST_AVRCP_CONTROL, values[0]=control }`

### ③ 入队（这一步在**调用者线程**里执行）

[L87-L129](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/api/bt_xstack_api_core.c#L87-L129)：

```c
bt_xstack_api_submit()
  ├ 4 道前置检查（NULL / 未初始化 / failed / runtime 未启动）
  ├ copy.generation = ++bt_xstack_api_generation
  ├ btstack_ring_push(&bt_xstack_request_queue, &copy)      ★入队 L120
  └ btstack_platform_notify(BTSTACK_WORKER_API, SIGNAL_WORK) ★唤醒 L127
```

**到这里按键线程就返回了**，命令还没发出去。

### ④ ═══ 跨任务边界 ① ═══ API worker 任务出队

[L298-L326](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/api/bt_xstack_api_core.c#L298-L326)：

```c
static void bt_xstack_api_task(void *context)
  for (;;) {
      for (;;) {
          ring_pop(&bt_xstack_request_queue, &request)        ★出队 L307-308
          notify(BTSTACK_WORKER_EVENT, SIGNAL_REQUEST_SPACE)  // ★反向通知事件任务
          bt_xstack_api_process_request(&request)             L317
      }
      wait(BTSTACK_WORKER_API, WORK | COMMAND_SLOT, FOREVER)  // 睡
  }
```

### ⑤ 重试壳（处理"后端忙"）

[L277-L296](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/api/bt_xstack_api_core.c#L277-L296)：

```c
do {
    result = btstack_core_dispatch(request);// ★★ L286 —— 就是这一行
    if (result == BT_XSTACK_BACKEND_BUSY) {
        wait(BTSTACK_WORKER_API, COMMAND_SLOT | WORK, FOREVER);   // ★等命令槽位
    }
} while (result == BT_XSTACK_BACKEND_BUSY);                       // ★重试同一条
```

> ⑤→⑥ 就是一次**同任务内的普通函数调用**，二者是同一个 API worker 任务里前后两帧栈。

### ⑥ 分派（不是终点！）

[dispatcher.c#L515-L551](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/core/btstack_core_dispatcher.c#L515-L551) 内部有两条路：

```c
// 路 1：本地请求（改开关、存状态类）—— 直接做完返回
local_status = btstack_core_execute_local_request(request, &handled);
if (handled) { btstack_core_request_completed(request, local_status); return 0; }

// 路 2：协议栈请求 —— ★ 只是「投递」，不执行
if (btstack_core_prepare_stack_request(&prepared, request) != 0) { ... return 0; }
slot = btstack_core_command_slot_acquire();
if (slot == NULL) return BT_XSTACK_BACKEND_BUSY;              // ★ BUSY 源 ①：槽位耗尽
memcpy(&slot->request, &prepared, sizeof(slot->request));      // ★ 请求拷进静态槽
slot->callback.callback = btstack_core_execute;                // ★ 注册回调
slot->callback.context = slot;
if (!btstack_platform_post_stack_callback(&slot->callback)) {  // ★★ L546 投递给 BTstack 任务
    btstack_core_command_slot_release(slot);
    return BT_XSTACK_BACKEND_BUSY;                             // ★ BUSY 源 ②：投递失败
}
return 0;
```

AVRCP 走的正是路 2。

**为什么要跨这一次任务？** BTstack 协议栈**非线程安全**，所有 BTstack API 必须在 run loop 任务里调。

**为什么要「命令槽位」（[L23-L27](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/core/btstack_core_dispatcher.c#L23-L27)）？** 请求要活到另一个任务里执行，不能留在 API worker 的栈上，必须 `memcpy` 到静态槽。

### ⑦ ═══ 跨任务边界 ② ═══ BTstack run loop 任务真正执行

[dispatcher.c#L476-L497](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/core/btstack_core_dispatcher.c#L476-L497)：

```c
static void btstack_core_execute(void *context) {
    slot->state = BT_XSTACK_COMMAND_SLOT_RUNNING;
    status = btstack_core_execute_stack_request(&slot->request);   // ★ L488 真正执行
    btstack_core_stack_command_count++;
    btstack_core_request_completed(&slot->request, status);
    btstack_core_command_slot_release(slot);                       // ★ L496 → 发 COMMAND_SLOT
}
```

[L277-L278](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/core/btstack_core_dispatcher.c#L277-L278) 才是 AVRCP 的分派点：

```c
case BT_XSTACK_REQUEST_AVRCP_CONTROL:
    return btstack_avrcp_execute_control(request->values[0]);
```

### ⑧ 协议包装层

[btstack_avrcp.c L246-L300](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp.c#L246-L300)：

```c
btstack_avrcp_execute_control(control)
  ├ !is_connected() → NOT_READY
  ├ PLAY_PAUSE → 读 play_status 拆成 PLAY/PAUSE      ★ 第一层状态感知
  └ → btstack_avrcp_control()            avrcp.c L241
        └ btstack_avrcp_controller_control()  controller.c L434
              ├ cid==0 → NOT_READY
              ├ switch → avrcp_controller_play / pause / forward / backward / ...
              └ NEXT|PREVIOUS → schedule_track_change_metadata()   ★ L478  600ms 后补查
```

### ⑨ 原生层 → 空口

```c
avrcp_controller_play/pause/forward/backward/...  → AVCTP/L2CAP → 手机
```

**命令无回执**：这里返回的 `status` 只代表"发出去了"，手机是否执行完、执行结果如何，一律靠第②站的事件上行回来。

### 完整链路一图（含任务归属）

```plain
按键线程
  bt_xstack_avrcp_next() → submit_value → submit
      ring_push(bt_xstack_request_queue) + notify(WORKER_API)
─────────────────────── 任务边界 ① ───────────────────────
API worker 任务 (bt_xstack_api_task L298)
  ring_pop → bt_xstack_api_process_request (⑤ L277)
      btstack_core_dispatch (⑥ L515)              ← L286 直接调用
        ├ 路1 本地 → completed → return 0
        └ 路2 协议栈：acquire slot → post_stack_callback   ★投递
─────────────────────── 任务边界 ② ───────────────────────
BTstack run loop 任务 (btstack_run_loop_execute)
  btstack_core_execute (⑦ L476)                  ← 被回调
      btstack_core_execute_stack_request → case REQUEST_AVRCP_CONTROL (L277)
        → btstack_avrcp_execute_control (⑧ L246)
          → controller_control (L434) → avrcp_controller_next
            → AVCTP/L2CAP → 手机
```

### BUSY 闭环（背压）

| 位置 | 事件 | 含义 |
|---|---|---|
| dispatcher L541 | `return BACKEND_BUSY` | 命令槽位全占满 |
| dispatcher L548 | `return BACKEND_BUSY` | 投递失败（栈任务队列满） |
| api_core L288-292 | `wait(COMMAND_SLOT)` | API worker 睡下等槽位 |
| dispatcher L496 | `slot_release()` → notify | 槽位释放，唤醒 API worker |
| api_core L285 | `do { } while (BUSY)` | 醒来重试**同一条**请求 |

------

# 第④站：Cover Art（BIP/OBEX 独立通路）

**它不是 AVRCP。** 回调类型是 `btstack_bip_event_callback_t`，走 OBEX 对象传输。

### ④-1 通路与 AVRCP 完全解耦

```plain
注册：btstack_avrcp_cover_art_setup(btstack_core_event_mapper_cover_art)   manager L610
        └ btstack_bip_client_init(btstack_avrcp_cover_art_bip_event)       cover_art.c L161

触发：AVRCP 只负责「告诉 cover art 有歌了」
        NOTIFICATION_TRACK_CHANGED → btstack_avrcp_cover_art_track_changed()   controller.c L263
        NOW_PLAYING_COVER_ART_INFO → btstack_avrcp_cover_art_handle_received()  controller.c L326
                                      （7 字节 image handle）

下载：handle → btstack_bip_client_set_object → get_linked_thumbnail / get_image_properties
        → OBEX over L2CAP(另一条) → 手机
上行：BIP 事件 → btstack_avrcp_cover_art_bip_event (cover_art.c L92)
        → 上层 callback = btstack_core_event_mapper_cover_art (mapper L1111)
        → btstack_frontend_deliver_cover_art (api_core L1442)  ★ 直接调，不进事件队列
        → bt_xstack_config.cover_art_callback
```

### ④-2 与主链路的关键差异

| 项 | AVRCP 事件 | Cover Art 事件 |
|---|---|---|
| 载体 | AVCTP | OBEX / BIP |
| 中间队列 | **有**（`bt_xstack_critical_event_queue`） | **无** |
| 回调执行任务 | EVENT worker | **BTstack run loop 任务**（回调里不能阻塞） |
| 开关 | 总是开 | 需注册回调才开：[api_core L447](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/api/bt_xstack_api_core.c#L447) / [L998](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/api/bt_xstack_api_core.c#L998) `set_callback_enabled(cb != NULL)` |

### ④-3 两种模式 + generation 防串包

- 模式：`BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL`（默认，取关联缩略图）/ `IMAGE`（先查 properties 再取图，[set_mode L181](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_cover_art.c#L181)）
- 单事务限制：`operation_active` 一次只跑一个；中途来新 handle 就进 `pending_*` 暂存（[L73-90](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_cover_art.c#L73-L90)、[L250-255](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_cover_art.c#L250-L255)）
- `generation` 每次请求递增；[L112](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_cover_art.c#L112) `event->generation != active_generation → return`，防止旧请求的迟到数据污染新封面
- 失败重试一次（`retry_used`，[L132-137](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_cover_art.c#L132-L137)）；OBEX 断则重连（[L149-150](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/profiles/btstack_avrcp_cover_art.c#L149-L150)）

------

# 附录 A：两条方向的对称结构

车机内部有**两个方向相反的队列 + 两个 worker 任务**：

```plain
        ┌──────────── 出向（车机→手机）────────────┐
按键 ──→ bt_xstack_request_queue ──→ BTSTACK_WORKER_API (bt_xstack_api_task)
              ★ L120 入队                ★ L307 出队
                                        → dispatch → post_stack_callback
                                        → BTstack 任务 → execute_control
                                        → avrcp_controller_* → 空口

        ┌──────────── 入向（手机→车机）────────────┐
空口 ──→ HCI 事件 → *_packet_handler → internal_emit → manager → mapper
                                        → bt_xstack_critical_event_queue
                                        → BTSTACK_WORKER_EVENT (bt_xstack_event_task)
                                        → frontend_deliver_event → demo 回调
```

|              | 出向出令                                                     | 入向事件                                                     |
| ------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 队列         | `bt_xstack_request_queue`                                    | `bt_xstack_critical_event_queue`                             |
| worker       | `BTSTACK_WORKER_API`                                         | `BTSTACK_WORKER_EVENT`                                       |
| 任务函数     | [bt_xstack_api_task](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/api/bt_xstack_api_core.c#L298) | [bt_xstack_event_task](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/btstack/api/bt_xstack_api_core.c#L243) |
| 中间分派     | `btstack_core_dispatch`                                      | `btstack_frontend_deliver_event`（按 domain）                |
| 终点         | 原生 `avrcp_controller_*`                                    | 应用 `audio_event` 回调                                      |
| 应用有无回调 | **无**（发出去就不管）                                       | **有**                                                       |

**跨任务次数对比**：

- 出向：**2 次**（按键线程 → API worker → BTstack run loop）
- 入向：**1 次**（BTstack run loop → EVENT worker）
- 例外：Cover Art 入向 **0 次**（回调直接在 BTstack 任务里执行）

### 两个 worker 互相通知（双向背压）

| 位置     | 通知                           | 原因                                     |
| -------- | ------------------------------ | ---------------------------------------- |
| L127     | 按键线程 → API worker          | 有新请求                                 |
| L313-315 | **API worker → 事件 worker**   | 请求队列腾出空间了（事件侧可能正等这个） |
| L288-292 | API worker 自等 `COMMAND_SLOT` | 命令槽位满，等空位再重试                 |

------

# 附录 B：与 HFP 命令下行对照（完全同构）

|          | HFP                                         | AVRCP                                         |
| -------- | ------------------------------------------- | --------------------------------------------- |
| 应用 API | `bt_xstack_hfp_answer()`                    | `bt_xstack_avrcp_play()`                      |
| 入队     | `bt_xstack_request_queue`                   | **同一个队列**                                |
| 分派     | `REQUEST_HFP_CONTROL` → dispatcher L266-268 | `REQUEST_AVRCP_CONTROL` → dispatcher L277-278 |
| 包装层   | `btstack_hfp_hf_execute_control`            | `btstack_avrcp_execute_control`               |
| 原生     | `hfp_hf_answer_incoming_call`               | `avrcp_controller_play`                       |

**结论：所有"应用发命令"都走同一条队列 + 同一个 API worker + 同两次跨任务**，靠 `request->id` 区分。AVRCP 并不特殊。

------

# 附录 C：关键机制清单（都是踩过坑才有的）

| # | 机制 | 位置 | 不这么做会怎样 |
|---|---|---|---|
| 1 | **通知 one-shot，必须立即重挂** | controller L242-245 / L267-272 | 只有第一次播放状态变化能到 UI，之后全哑 |
| 2 | **单事务限制 → 排队机** | controller L132-180 位掩码 `pending_requests` | 手机侧拒绝并发事务 |
| 3 | **`metadata_inflight` 门闩** | controller L145-149 | 同一时刻重复发 GET_ELEMENT_ATTRIBUTES |
| 4 | **20ms / 200ms 重试 + 最多 3 次** | controller L12-16 宏 + L104-130 | 一次失败就永久丢元数据 |
| 5 | **`COMMAND_DISALLOWED` 软退避** | controller L168-175 | 请求被误清，元数据永久丢失 |
| 6 | **`OPERATION_COMPLETE` 抠字节兜底** | controller L339-346 | `metadata_inflight` 永久卡 true |
| 7 | **NEXT/PREVIOUS 600ms 补偿查询** | controller L95-102 + L478 | 部分手机不发 TRACK_CHANGED，歌名不更新 |
| 8 | **元数据搭车** | controller L415-427 + avrcp.c L120 | 标题/歌手得单独再发一封事件 |
| 9 | **PLAY_PAUSE 两层状态感知** | avrcp.c L265-277 + demo L837-849 | 某些手机忽略 toggle，暂停后按不回来 |
| 10 | **1s 进度轮询只在 PLAYING 时跑** | controller L182-214 | 空闲时白耗 AVRCP 事务 |
| 11 | **状态去重** | avrcp.c L29-31 | 重复 CONNECTED 事件刷 UI |
| 12 | **命令槽位 + 2 次跨任务** | dispatcher L23-27 / L515-551 | 请求留在栈上→野指针；且违反 BTstack 线程模型 |

------

# 附录 D：UI 对接点（TP2915 蓝牙界面用）

**车机 → 手机（UI 按钮 → 发命令）**

| UI 动作 | API | 备注 |
|---|---|---|
| 播放/暂停 | `bt_xstack_avrcp_play()` / `bt_xstack_avrcp_pause()` | **优先用这两个**，别用 `play_pause()` |
| 上一首/下一首 | `bt_xstack_avrcp_previous()` / `next()` | 触发 600ms 后自动补查元数据 |
| 停止 | `bt_xstack_avrcp_stop()` | |
| 快进/快退 | `fast_forward()` / `rewind()` | |
| 音量 | `bt_xstack_avrcp_volume_up()` / `volume_down()` | 与 HFP speaker gain 不是一回事 |
| 循环/随机 | `bt_xstack_avrcp_set_repeat()` / `set_shuffle()` | |
| 封面 | `bt_xstack_cover_art_set_mode()` / `get_image()` | |

**发送前必须过门禁**（demo [L816-L860](file:///d:/PYH_PRJ/GPlus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/Project/wifi_demo/bt_xstack_demo.c#L816-L860) 的写法）：`bt_xstack_demo_avrcp_connected` + `bt_xstack_demo_play_state` 都要在临界区里读快照。

**手机 → 车机（UI 显示）**

| 事件 | UI 落点 |
|---|---|
| `AUDIO_EVENT_AVRCP_STATE` | 遥控可用/不可用（同时也要写连接门禁，见 demo L560-568） |
| `AUDIO_EVENT_PLAY_STATE` | 播放/暂停图标 |
| `AUDIO_EVENT_TRACK_STATE` | 进度条（values[1]=位置秒，values[2]=总长秒） |
| `AUDIO_EVENT_TRACK_INFO` | 歌名/歌手/专辑（`bt_ui_post_track_info`） |
| `AUDIO_EVENT_PLAY_MODE` | 循环/随机图标 |
| `AUDIO_EVENT_ABSOLUTE_VOLUME` | 音量条 |
| `connection_event`（AVRCP） | 连接状态图标 |
| `cover_art_callback` | 专辑封面位图 —— **注意回调跑在 BTstack 任务里，必须快速返回** |

**UI 层绝不能做的事**

1. 在回调里直接调 `bt_xstack_avrcp_*()` —— 那是 EVENT worker 里再往 API worker 投递，非死锁但会拖慢事件处理；应往 UI 队列投递。
2. 把 AVRCP 命令的返回值当成"手机已执行" —— 它只代表"发出去了"。
3. 依赖 `TRACK_CHANGED` —— 它在 mapper 里被丢弃，UI 收不到，要用 `TRACK_INFO`。








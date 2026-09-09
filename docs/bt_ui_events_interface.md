# LVGL 与蓝牙事件接口文档（bt\_ui\_events 适配层）

> 日期：2026-08-31
> 工程：GPCV2178 Turnkey SDK（凌阳 GPCV2178 + ATBM6165 蓝牙 xstack）
> 作用：把 `bt_xstack` 上报的原始蓝牙事件归一化成统一的 `bt_ui_event_t`，
> 通过注册回调交给 UI 层（LVGL 或 task\_display OSD）绘制界面。

***

## 1. 架构与数据流

```
蓝牙协议栈 (btstack xstack)
   │ 事件回调 (xstack worker 任务上下文)
   ▼
bt_xstack_demo.c   ← 6 个事件回调（service/phone/connection/audio/phonebook/cover_art）
   │ bt_ui_post_*(event)   ← bt_ui_events.c 适配层（临界区保护分发）
   ▼
bt_ui_set_notify(cb, ctx) 注册的 UI 回调（运行在 xstack 任务上下文）
   │ 只做数据拷贝 + 写入 UI 事件队列
   ▼
LVGL 任务 / task_display 任务  → 绘制
```

关键约束：

- **回调运行在 xstack 任务上下文，不得直接调用 LVGL 绘制 API**，只允许拷贝数据并投递到 UI 任务队列。
- **封面分片数据指针（`data`）仅在回调内有效**，必须在回调内 `memcpy` 保存；首期音乐页不处理封面图事件。
- `media_audio_callback` 与 `call_audio_callback` 是 PCM 音频数据通路，不是 UI 事件来源。

## 2. 涉及文件

| 文件                                                                          | 作用                         |
| --------------------------------------------------------------------------- | -------------------------- |
| `Project/Include/bt_ui_events.h`                                            | 适配层头文件：事件枚举 + 结构 + API（新增） |
| `Project/wifi_demo/bt_ui_events.c`                                          | 适配层实现（新增）                  |
| `Project/wifi_demo/bt_xstack_demo.c`                                        | 各回调接入 UI 上报（修改）            |
| `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`         | 主工程登记新源文件（修改）              |
| `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release_8189FTV.gpcmprj` | 8189FTV 工程登记新源文件（修改）       |

## 3. 事件结构体

```c
#define BT_UI_TEXT_MAX 64u

typedef struct {
    bt_ui_event_type_t type;     /* 事件类型 */
    uint32_t values[4];          /* 数值载荷 */
    uint8_t value_count;         /* 原始事件的数值个数；bt_ui_post_simple() 固定填 3 */
    char text[BT_UI_TEXT_MAX];   /* 主文本（歌名/号码/设备名） */
    char text2[BT_UI_TEXT_MAX];  /* 次文本（歌手） */
    char text3[BT_UI_TEXT_MAX];  /* 第三文本（专辑） */
    const uint8_t *data;         /* 封面分片指针（仅回调内有效） */
    uint32_t data_len;           /* 分片长度 / 封面总长度 */
    uint32_t data_offset;        /* 分片在整图中的偏移 */
    bt_xstack_addr_t addr;       /* 对端蓝牙地址（预留；首期音乐 UI 不依赖） */
    union {
        bt_xstack_phonebook_contact_t contact;  /* 联系人条目 */
        bt_xstack_phonebook_call_t call;        /* 通话记录条目 */
    } pb;
} bt_ui_event_t;
```

## 4. 事件清单（`bt_ui_event_type_t`）

### 4.1 系统与连接

| 事件                         | 值                                                     | 说明             | 对应 UI      |
| -------------------------- | ----------------------------------------------------- | -------------- | ---------- |
| `BT_UI_EVENT_BT_POWER`     | values\[0]=0/1                                        | 蓝牙开关状态         | 开关图标       |
| `BT_UI_EVENT_CONN_STATE`   | values\[0]=profile, \[1]=state, \[2]=unexpected\_loss | 各 profile 连接状态 | 状态栏图标、断连弹窗 |
| `BT_UI_EVENT_PAIR_REQUEST` | values\[0]=数字确认值                                      | 配对请求           | 6 位数字确认弹窗  |
| `BT_UI_EVENT_PAIR_RESULT`  | values\[0]=0 成功                                       | 配对结果           | 提示         |
| `BT_UI_EVENT_SCAN_RESULT`  | values\[0]=rssi, text=设备名                             | 扫描到设备          | 设备列表项      |
| `BT_UI_EVENT_SCAN_DONE`    | —                                                     | 扫描完成           | 停止刷新动画     |

profile 枚举：`BT_XSTACK_PROFILE_HFP_HF=1 / A2DP_SINK=2 / AVRCP=3`
state 枚举：`DISCONNECTED=0 / CONNECTING=1 / CONNECTED=2 / DISCONNECTING=3`

### 4.2 音乐播放（A2DP/AVRCP）

| 事件                            | 值                               | 说明       | 对应 UI                |
| ----------------------------- | ------------------------------- | -------- | -------------------- |
| `BT_UI_EVENT_A2DP_STATE`      | values\[0]=a2dp\_state          | A2DP 流状态 | 播放图标                 |
| `BT_UI_EVENT_PLAY_STATE`      | values\[0]=play\_state          | 播放/暂停/快进 | 播放控制图标               |
| `BT_UI_EVENT_PLAY_MODE`       | values\[0]=repeat, \[1]=shuffle | 播放模式     | 单曲/循环/随机图标           |
| `BT_UI_EVENT_TRACK_INFO`      | text=歌名, text2=歌手, text3=专辑     | 曲目信息     | 歌名/歌手/专辑 label       |
| `BT_UI_EVENT_ABSOLUTE_VOLUME` | values\[0]=0..127               | AVRCP 音量 | 音量条                  |
| `BT_UI_EVENT_COVER_ART_DATA`  | data/len/offset                 | 封面分片     | 拼接缓存                 |
| `BT_UI_EVENT_COVER_ART_DONE`  | data\_len=总长                    | 封面完整     | 解码成 RGB565 → lv\_img |
| `BT_UI_EVENT_COVER_ART_ERROR` | values\[0]=native\_status       | 封面失败     | 提示                   |

a2dp\_state 枚举：`IDLE=1 / CONNECTING=2 / CONNECTED=3 / STREAM_OPEN=4 / STREAMING=5`。

`TRACK_STATE` 由公开 xstack API 仅保证 `values[0]` 为播放状态；不要把其他 values
解释为播放位置或总时长。因此首期进度条不显示伪造的曲目进度。
play\_state 枚举：`STOPPED=0 / PLAYING=1 / PAUSED=2 / FORWARD_SEEK=3 / REVERSE_SEEK=4`

### 4.3 通话（HFP）

| 事件                        | 值                     | 说明       | 对应 UI |
| ------------------------- | --------------------- | -------- | ----- |
| `BT_UI_EVENT_HFP_STATE`   | values\[0]=hfp\_state | 通话状态     | 界面切换  |
| `BT_UI_EVENT_CALL_NUMBER` | text=号码               | 对方号码     | 号码显示  |
| `BT_UI_EVENT_RING`        | values\[0]=0/1        | 响铃       | 响铃提示  |
| `BT_UI_EVENT_CALL_AUDIO`  | values\[0]=0/1        | SCO 音频通路 | 音频指示  |
| `BT_UI_EVENT_SIGNAL`      | values\[0]=等级         | 手机信号     | 信号格   |
| `BT_UI_EVENT_BATTERY`     | values\[0]=等级         | 手机电量     | 电量图标  |
| `BT_UI_EVENT_MIC_MUTE`    | values\[0]=0/1        | 静音       | 静音图标  |

hfp\_state 枚举：`IDLE=1 / CONNECTING=2 / CONNECTED=3 / OUTGOING=4 / INCOMING=5 /
ACTIVE=6 / HELD=7 / ACTIVE_WITH_INCOMING=8 / ACTIVE_WITH_HELD=9 / MULTIPARTY=10`

### 4.4 电话本（PBAP）

| 事件                        | 值                   | 说明     | 对应 UI  |
| ------------------------- | ------------------- | ------ | ------ |
| `BT_UI_EVENT_PB_STARTED`  | values\[0]=category | 同步开始   | 进度提示   |
| `BT_UI_EVENT_PB_CONTACT`  | pb.contact          | 联系人条目  | 联系人列表  |
| `BT_UI_EVENT_PB_CALL_LOG` | pb.call             | 通话记录条目 | 通话记录列表 |
| `BT_UI_EVENT_PB_DONE`     | values\[0]=status   | 同步完成   | 关进度条   |
| `BT_UI_EVENT_PB_AUTH`     | —                   | 需要密码   | 密码输入框  |
| `BT_UI_EVENT_PB_ERROR`    | values\[0]=status   | 同步失败   | 错误提示   |

contact 结构：`name[49]` + `numbers[4]`（value\[33] + type\_flags）+ `number_count` + `truncated`
call 结构：`name[49]` + `number[33]` + `timestamp[24]` + `direction` + `truncated`

## 5. API

```c
/* 注册/注销 UI 通知回调（NULL 注销）。在 UI 初始化时调用。 */
void bt_ui_set_notify(bt_ui_notify_cb_t callback, void *context);

/* 清零事件结构，发送前必须调用。 */
void bt_ui_event_init(bt_ui_event_t *event);

/* 发送完整事件。 */
void bt_ui_post_event(const bt_ui_event_t *event);

/* 便捷发送：纯数值事件。 */
void bt_ui_post_simple(bt_ui_event_type_t type,
                       uint32_t v0, uint32_t v1, uint32_t v2);

/* 便捷发送：数值 + 单个文本（设备名/号码等）。 */
void bt_ui_post_text(bt_ui_event_type_t type,
                     uint32_t v0, uint32_t v1, uint32_t v2,
                     const char *text);

/* 便捷发送：曲目信息（歌名/歌手/专辑）。 */
void bt_ui_post_track_info(const char *title, const char *artist,
                           const char *album);
```

## 6. UI 层（LVGL）对接示例

```c
#include "bt_ui_events.h"

/* UI 初始化时注册，事件即从 xstack 回调送达 */
bt_ui_set_notify(my_ui_handler, NULL);

static void my_ui_handler(const bt_ui_event_t *ev, void *ctx)
{
    switch (ev->type) {
    case BT_UI_EVENT_TRACK_INFO:
        /* ev->text 歌名, ev->text2 歌手, ev->text3 专辑
         * → 拷贝后投递到 UI 任务队列，队列中更新 lv_label */
        break;

    case BT_UI_EVENT_COVER_ART_DATA:
        /* 分片数据，必须在此回调内拷贝，指针仅回调期有效 */
        memcpy(my_cover_buf + ev->data_offset, ev->data, ev->data_len);
        break;

    case BT_UI_EVENT_COVER_ART_DONE:
        /* my_cover_buf 已完整 → 投递到 UI 任务 → 解码成 RGB565 → lv_img */
        break;

    case BT_UI_EVENT_HFP_STATE:
        /* values[0] = hfp_state → 切换通话界面 */
        break;

    case BT_UI_EVENT_PB_CONTACT:
        /* ev->pb.contact.name / numbers[i].value → 联系人列表项 */
        break;

    /* ... 其余事件同理 */
    default:
        break;
    }
}
```

## 7. 线程模型注意事项

1. **回调上下文**：`bt_ui_post_*` 及通知回调都运行在 xstack worker 任务，优先级低于 UI 任务。
2. **禁止直接绘制**：回调内不得调用 `lv_*` 绘制函数，应拷贝数据 + 投递到 UI
   任务队列或受临界区保护的短事件队列；本工程 LVGL 首期采用后者，由 LVGL 任务轮询消费。
3. **封面分片**：`ev->data` 指针仅回调期有效，必须即时 `memcpy`；多次 `COVER_ART_DATA` 按 `data_offset` 拼到缓存，收到 `COVER_ART_DONE` 才完整。
4. **临界区**：适配层内部用 `taskENTER_CRITICAL` 保护回调注册/分发，开销极小。

## 8. 状态跟踪

- [x] 适配层头文件与实现（bt\_ui\_events.h/.c）
- [x] bt\_xstack\_demo.c 七个回调接入 UI 上报
- [x] 封面图回调注册（config.cover\_art\_callback）
- [x] 两个 .gpcmprj 工程登记新源文件
- [ ] 待用户 IDE 编译验证
- [x] LVGL 首期蓝牙音乐播放页（连接、曲目信息、播放状态、音量、AVRCP 按钮）
- [ ] 通话、电话本与封面图 UI

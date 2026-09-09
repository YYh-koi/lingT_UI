#ifndef BT_UI_EVENTS_H
#define BT_UI_EVENTS_H

/*
 * 蓝牙 → UI 事件适配层
 *
 * 作用：把 bt_xstack 各回调上报的原始事件，归一化成统一的
 * bt_ui_event_t，通过 bt_ui_set_notify() 注册的回调交给 UI 层
 * （LVGL 任务或 task_display OSD），由 UI 层决定何时绘制。
 *
 * 线程模型：bt_ui_post_* 运行在 xstack 回调上下文（非 UI 任务），
 * 通知回调内部不得直接调用 LVGL 绘制 API，应把事件拷贝后投递到
 * 自己的 UI 任务队列。
 */

#include <stdint.h>
#include "bt_xstack_types.h"
#include "bt_xstack_phonebook.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BT_UI_TEXT_MAX 64u

typedef enum {
    /* 系统与连接 */
    BT_UI_EVENT_BT_POWER = 0,   /* values[0]=0/1 蓝牙开关 */
    BT_UI_EVENT_CONN_STATE,     /* values[0]=profile, [1]=state, [2]=unexpected_loss */
    BT_UI_EVENT_PAIR_REQUEST,   /* values[0]=6 位数字确认值 */
    BT_UI_EVENT_PAIR_RESULT,    /* values[0]=0 成功 */
    BT_UI_EVENT_SCAN_RESULT,    /* values[0]=rssi, text=设备名 */
    BT_UI_EVENT_SCAN_DONE,      /* 扫描完成 */

    /* 音乐 A2DP/AVRCP */
    BT_UI_EVENT_A2DP_STATE,     /* values[0]=bt_xstack_a2dp_state_t */
    BT_UI_EVENT_PLAY_STATE,     /* values[0]=bt_xstack_play_state_t */
    BT_UI_EVENT_PLAY_MODE,      /* values[0]=repeat, [1]=shuffle */
    BT_UI_EVENT_TRACK_INFO,     /* text=歌名, text2=歌手, text3=专辑 */
    BT_UI_EVENT_ABSOLUTE_VOLUME,/* values[0]=0..127 */
    BT_UI_EVENT_COVER_ART_DATA, /* data/len/offset 封面分片 */
    BT_UI_EVENT_COVER_ART_DONE, /* data_len=封面总长度 */
    BT_UI_EVENT_COVER_ART_ERROR,/* values[0]=native_status */

    /* 通话 HFP */
    BT_UI_EVENT_HFP_STATE,      /* values[0]=bt_xstack_hfp_state_t */
    BT_UI_EVENT_CALL_NUMBER,    /* text=对方号码 */
    BT_UI_EVENT_RING,           /* values[0]=0/1 */
    BT_UI_EVENT_CALL_AUDIO,     /* values[0]=0/1 SCO 通路 */
    BT_UI_EVENT_SIGNAL,         /* values[0]=信号等级 */
    BT_UI_EVENT_BATTERY,        /* values[0]=电量等级 */
    BT_UI_EVENT_MIC_MUTE,       /* values[0]=0/1 */

    /* 电话本 PBAP */
    BT_UI_EVENT_PB_STARTED,     /* values[0]=category 同步开始 */
    BT_UI_EVENT_PB_CONTACT,     /* pb.contact 联系人条目 */
    BT_UI_EVENT_PB_CALL_LOG,    /* pb.call 通话记录条目 */
    BT_UI_EVENT_PB_DONE,        /* values[0]=status 同步完成 */
    BT_UI_EVENT_PB_AUTH,        /* 需要密码 */
    BT_UI_EVENT_PB_ERROR,       /* values[0]=status */

    BT_UI_EVENT_MAX
} bt_ui_event_type_t;

typedef struct {
    bt_ui_event_type_t type;
    uint32_t values[4];
    uint8_t value_count;
    char text[BT_UI_TEXT_MAX];
    char text2[BT_UI_TEXT_MAX];
    char text3[BT_UI_TEXT_MAX];
    const uint8_t *data;        /* 分片数据指针（仅在回调内有效） */
    uint32_t data_len;
    uint32_t data_offset;
    bt_xstack_addr_t addr;
    union {
        bt_xstack_phonebook_contact_t contact;
        bt_xstack_phonebook_call_t call;
    } pb;
} bt_ui_event_t;

typedef void (*bt_ui_notify_cb_t)(
    const bt_ui_event_t *event,
    void *context);

/* 注册/注销 UI 通知回调（NULL 注销）。 */
void bt_ui_set_notify(bt_ui_notify_cb_t callback, void *context);

/* 清零事件结构，发送前必须调用。 */
void bt_ui_event_init(bt_ui_event_t *event);

/* 发送完整事件。 */
void bt_ui_post_event(const bt_ui_event_t *event);

/* 便捷发送：纯数值事件。 */
void bt_ui_post_simple(
    bt_ui_event_type_t type,
    uint32_t v0, uint32_t v1, uint32_t v2);

/* 便捷发送：数值 + 单个文本（设备名/号码等）。 */
void bt_ui_post_text(
    bt_ui_event_type_t type,
    uint32_t v0, uint32_t v1, uint32_t v2,
    const char *text);

/* 便捷发送：曲目信息（歌名/歌手/专辑）。 */
void bt_ui_post_track_info(
    const char *title, const char *artist, const char *album);

#ifdef __cplusplus
}
#endif

#endif /* BT_UI_EVENTS_H */

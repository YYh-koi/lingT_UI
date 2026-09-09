#include <string.h>

#include "application.h"
#include "bt_audio_task.h"
#include "bt_xstack.h"
#include "bt_xstack_demo.h"
#include "bt_ui_events.h"

#include "task.h"

#define BT_XSTACK_DEMO_LOCAL_NAME "GP-BT-ATBM6165"

typedef enum {
    BT_XSTACK_DEMO_IDLE = 0,
    BT_XSTACK_DEMO_STARTING,
    BT_XSTACK_DEMO_WAIT_POWER,
    BT_XSTACK_DEMO_SETTING_NAME,
    BT_XSTACK_DEMO_STARTED
} bt_xstack_demo_start_state_t;

static volatile bt_xstack_demo_start_state_t
    bt_xstack_demo_start_state = BT_XSTACK_DEMO_IDLE;
static volatile bt_xstack_hfp_state_t bt_xstack_demo_hfp_state =
    BT_XSTACK_HFP_STATE_IDLE;
static volatile uint8_t bt_xstack_demo_hfp_ringing;
static volatile uint8_t bt_xstack_demo_avrcp_connected;
static volatile uint8_t bt_xstack_demo_a2dp_streaming;
static volatile uint8_t bt_xstack_demo_pcm_config_valid;
static volatile uint8_t bt_xstack_demo_resume_pending;
static uint32_t bt_xstack_demo_pcm_sample_rate;
static uint8_t bt_xstack_demo_pcm_channel_count;
static uint8_t bt_xstack_demo_pcm_bits_per_sample;

static int bt_xstack_demo_begin_start(void)
{
    int acquired = 0;

    taskENTER_CRITICAL();
    if (bt_xstack_demo_start_state == BT_XSTACK_DEMO_IDLE) {
        bt_xstack_demo_start_state = BT_XSTACK_DEMO_STARTING;
        acquired = 1;
    }
    taskEXIT_CRITICAL();
    return acquired;
}

static void bt_xstack_demo_set_start_state(
    bt_xstack_demo_start_state_t state)
{
    taskENTER_CRITICAL();
    bt_xstack_demo_start_state = state;
    taskEXIT_CRITICAL();
}

/* ── event-id → string helpers ─────────────────────────────────────── */

static const char * bt_xstack_demo_service_event_name(uint32_t id)
{
    switch (id) {
    case 0:  return "DEVICE_STATE";
    case 1:  return "POWER_STATE";
    case 2:  return "SCAN_RESULT";
    case 3:  return "SCAN_COMPLETE";
    case 4:  return "PAIRING_REQUEST";
    case 5:  return "PAIRED";
    default: return "SERVICE_?";
    }
}

static const char * bt_xstack_demo_phone_event_name(uint32_t id)
{
    switch (id) {
    case 0:  return "STATE";
    case 1:  return "DEVICE";
    case 2:  return "AUDIO";
    case 3:  return "VOICE_RECOGNITION";
    case 4:  return "SIGNAL";
    case 5:  return "NETWORK";
    case 6:  return "ROAM";
    case 7:  return "BATTERY";
    case 8:  return "NUMBER";
    case 9:  return "IN_BAND_RING";
    case 10: return "RING";
    case 11: return "LOST";
    case 12: return "MULTIPARTY";
    case 13: return "MICROPHONE_MUTE";
    default: return "PHONE_?";
    }
}

static const char * bt_xstack_demo_phonebook_event_name(uint32_t id)
{
    switch (id) {
    case BT_XSTACK_PHONEBOOK_EVENT_STARTED:            return "STARTED";
    case BT_XSTACK_PHONEBOOK_EVENT_CONTACT:            return "CONTACT";
    case BT_XSTACK_PHONEBOOK_EVENT_CALL:               return "CALL";
    case BT_XSTACK_PHONEBOOK_EVENT_CATEGORY_COMPLETED: return "CATEGORY_COMPLETED";
    case BT_XSTACK_PHONEBOOK_EVENT_COMPLETED:          return "COMPLETED";
    case BT_XSTACK_PHONEBOOK_EVENT_AUTH_REQUIRED:      return "AUTH_REQUIRED";
    case BT_XSTACK_PHONEBOOK_EVENT_ERROR:              return "ERROR";
    default:                                            return "PHONEBOOK_?";
    }
}

static const char * bt_xstack_demo_profile_name(uint32_t profile)
{
    switch (profile) {
    case 1:  return "HFP_HF";
    case 2:  return "A2DP_SINK";
    case 3:  return "AVRCP";
    default: return "PROFILE_?";
    }
}

static const char * bt_xstack_demo_conn_state_name(uint32_t state)
{
    switch (state) {
    case 0:  return "DISCONNECTED";
    case 1:  return "CONNECTING";
    case 2:  return "CONNECTED";
    case 3:  return "DISCONNECTING";
    default: return "CONN_?";
    }
}

static const char * bt_xstack_demo_audio_event_name(uint32_t id)
{
    switch (id) {
    case 0:  return "A2DP_STATE";
    case 1:  return "A2DP_DEVICE";
    case 2:  return "A2DP_LOST";
    case 3:  return "PCM_CONFIG";
    case 4:  return "AVRCP_STATE";
    case 5:  return "PLAY_STATE";
    case 6:  return "PLAY_MODE";
    case 7:  return "TRACK_STATE";
    case 8:  return "TRACK_INFO";
    case 9:  return "ABSOLUTE_VOLUME";
    default: return "AUDIO_?";
    }
}

/* ── callbacks ─────────────────────────────────────────────────────── */

static void bt_xstack_demo_service_event(
    const bt_xstack_service_event_t *event,
    void *context)
{
    int submit_name = 0;
    bt_xstack_result_t status;

    (void)context;
    if (event == NULL) {
        DBG_PRINT("xstack demo service event: NULL\r\n");
        return;
    }

    DBG_PRINT("xstack demo service event: %s(%u) vcnt=%u",
        bt_xstack_demo_service_event_name(event->id),
        (unsigned int)event->id, (unsigned int)event->value_count);
    if (event->value_count > 0u) {
        DBG_PRINT(" v=[%lu,%lu,%lu]",
            event->values[0], event->values[1], event->values[2]);
    }
    DBG_PRINT("\r\n");

    /* UI: 蓝牙开关/扫描/配对事件 */
    switch (event->id) {
    case BT_XSTACK_SERVICE_EVENT_POWER_STATE:
        bt_ui_post_simple(BT_UI_EVENT_BT_POWER,
            (event->value_count > 0u) ? event->values[0] : 0u,
            0u, 0u);
        break;
    case BT_XSTACK_SERVICE_EVENT_SCAN_RESULT:
        bt_ui_post_text(BT_UI_EVENT_SCAN_RESULT,
            (uint32_t)event->rssi, 0u, 0u, event->name);
        break;
    case BT_XSTACK_SERVICE_EVENT_SCAN_COMPLETE:
        bt_ui_post_simple(BT_UI_EVENT_SCAN_DONE, 0u, 0u, 0u);
        break;
    case BT_XSTACK_SERVICE_EVENT_PAIRING_REQUEST:
        bt_ui_post_simple(BT_UI_EVENT_PAIR_REQUEST,
            (event->value_count > 0u) ? event->values[0] : 0u,
            0u, 0u);
        break;
    case BT_XSTACK_SERVICE_EVENT_PAIRED:
        bt_ui_post_simple(BT_UI_EVENT_PAIR_RESULT,
            (event->value_count > 0u) ? event->values[0] : 0u,
            0u, 0u);
        break;
    default:
        break;
    }

    if ((event->id != BT_XSTACK_SERVICE_EVENT_POWER_STATE) ||
        (event->value_count == 0u)) {
        return;
    }

    if (event->values[0] == 0u) {
        taskENTER_CRITICAL();
        if (bt_xstack_demo_start_state != BT_XSTACK_DEMO_IDLE) {
            bt_xstack_demo_start_state = BT_XSTACK_DEMO_WAIT_POWER;
        }
        bt_xstack_demo_hfp_ringing = 0u;
        bt_xstack_demo_hfp_state = BT_XSTACK_HFP_STATE_IDLE;
        bt_xstack_demo_avrcp_connected = 0u;
        bt_xstack_demo_a2dp_streaming = 0u;
        bt_xstack_demo_pcm_config_valid = 0u;
        bt_xstack_demo_resume_pending = 0u;
        taskEXIT_CRITICAL();
        bt_audio_task_stop();
        return;
    }

    taskENTER_CRITICAL();
    if (bt_xstack_demo_start_state == BT_XSTACK_DEMO_WAIT_POWER) {
        bt_xstack_demo_start_state = BT_XSTACK_DEMO_SETTING_NAME;
        submit_name = 1;
    }
    taskEXIT_CRITICAL();
    if (!submit_name) {
        return;
    }

    status = bt_xstack_set_local_name(
        BT_XSTACK_DEMO_LOCAL_NAME, false);
    DBG_PRINT("xstack demo name status=%d\r\n", status);
    taskENTER_CRITICAL();
    if (bt_xstack_demo_start_state == BT_XSTACK_DEMO_SETTING_NAME) {
        bt_xstack_demo_start_state =
            (status == BT_XSTACK_OK) ?
            BT_XSTACK_DEMO_STARTED : BT_XSTACK_DEMO_WAIT_POWER;
    }
    taskEXIT_CRITICAL();
}

static void bt_xstack_demo_phone_event(
    const bt_xstack_phone_event_t *event,
    void *context)
{
    uint32_t value;
    bt_xstack_result_t status;

    (void)context;
    if (event == NULL) {
        DBG_PRINT("xstack demo phone event: NULL\r\n");
        return;
    }

    DBG_PRINT("xstack demo phone event: %s(%u) vcnt=%u",
        bt_xstack_demo_phone_event_name(event->id),
        (unsigned int)event->id, (unsigned int)event->value_count);
    if (event->value_count > 0u) {
        DBG_PRINT(" v=[%lu,%lu]",
            event->values[0], event->values[1]);
    }
    if (event->text[0] != '\0') {
        DBG_PRINT(" text=\"%s\"", event->text);
    }
    DBG_PRINT("\r\n");

    /* UI: HFP 通话状态/号码/响铃/音频/信号/电量/静音 */
    switch (event->id) {
    case BT_XSTACK_PHONE_EVENT_STATE:
        bt_ui_post_simple(BT_UI_EVENT_HFP_STATE,
            (event->value_count > 0u) ? event->values[0] : 0u,
            0u, 0u);
        break;
    case BT_XSTACK_PHONE_EVENT_DEVICE:
        /* demo 在此触发电话本同步，UI 无需处理 */
        break;
    case BT_XSTACK_PHONE_EVENT_AUDIO:
        bt_ui_post_simple(BT_UI_EVENT_CALL_AUDIO,
            (event->value_count > 0u) ? event->values[0] : 0u,
            0u, 0u);
        break;
    case BT_XSTACK_PHONE_EVENT_SIGNAL:
        bt_ui_post_simple(BT_UI_EVENT_SIGNAL,
            (event->value_count > 0u) ? event->values[0] : 0u,
            0u, 0u);
        break;
    case BT_XSTACK_PHONE_EVENT_BATTERY:
        bt_ui_post_simple(BT_UI_EVENT_BATTERY,
            (event->value_count > 0u) ? event->values[0] : 0u,
            0u, 0u);
        break;
    case BT_XSTACK_PHONE_EVENT_NUMBER:
        bt_ui_post_text(BT_UI_EVENT_CALL_NUMBER, 0u, 0u, 0u,
            event->text);
        break;
    case BT_XSTACK_PHONE_EVENT_RING:
        bt_ui_post_simple(BT_UI_EVENT_RING,
            (event->value_count > 0u) ? event->values[0] : 0u,
            0u, 0u);
        break;
    case BT_XSTACK_PHONE_EVENT_MICROPHONE_MUTE:
        bt_ui_post_simple(BT_UI_EVENT_MIC_MUTE,
            (event->value_count > 0u) ? event->values[0] : 0u,
            0u, 0u);
        break;
    default:
        break;
    }

    if (event->id == BT_XSTACK_PHONE_EVENT_DEVICE) {
        status = bt_xstack_phonebook_sync(
            &event->address, BT_XSTACK_PHONEBOOK_ALL);
        DBG_PRINT("xstack demo phonebook sync status=%d\r\n", status);
    }

    taskENTER_CRITICAL();
    if ((event->id == BT_XSTACK_PHONE_EVENT_STATE) &&
        (event->value_count > 0u)) {
        value = event->values[0];
        if ((value >= BT_XSTACK_HFP_STATE_IDLE) &&
            (value <= BT_XSTACK_HFP_STATE_MULTIPARTY)) {
            bt_xstack_hfp_state_t old_state =
                bt_xstack_demo_hfp_state;
            bt_xstack_demo_hfp_state = (bt_xstack_hfp_state_t)value;
            if ((value != BT_XSTACK_HFP_STATE_INCOMING) &&
                (value != BT_XSTACK_HFP_STATE_ACTIVE_WITH_INCOMING)) {
                bt_xstack_demo_hfp_ringing = 0u;
            }

            /* SCO audio: connect when call becomes active */
            if ((value == BT_XSTACK_HFP_STATE_ACTIVE) &&
                (old_state != BT_XSTACK_HFP_STATE_ACTIVE)) {
                bt_xstack_result_t ret;
                taskEXIT_CRITICAL();
                ret = bt_xstack_hfp_set_audio_connected(true);
                DBG_PRINT("xstack demo audio set true ret=%d\r\n", ret);
                taskENTER_CRITICAL();
            }
            /* SCO audio: disconnect when call ends */
            if ((old_state >= BT_XSTACK_HFP_STATE_ACTIVE) &&
                (old_state <= BT_XSTACK_HFP_STATE_MULTIPARTY) &&
                (value == BT_XSTACK_HFP_STATE_CONNECTED)) {
                bt_xstack_result_t ret;
                taskEXIT_CRITICAL();
                ret = bt_xstack_hfp_set_audio_connected(false);
                DBG_PRINT("xstack demo audio set false ret=%d\r\n", ret);
                bt_audio_task_stop();
                taskENTER_CRITICAL();
            }
        }
    } else if ((event->id == BT_XSTACK_PHONE_EVENT_AUDIO) &&
        (event->value_count > 0u)) {
        uint8_t audio_connected =
            (event->values[0] != 0u) ? 1u : 0u;
        taskEXIT_CRITICAL();
        if (audio_connected) {
            int dac_status =
                bt_audio_task_configure(8000u, 1u, 16u);
            DBG_PRINT("xstack demo call dac cfg status=%d\r\n",
                dac_status);
            if (dac_status != 0) {
                bt_audio_task_stop();
            }
        } else {
            bt_audio_task_stop();
        }
        taskENTER_CRITICAL();
    } else if ((event->id == BT_XSTACK_PHONE_EVENT_RING) &&
        (event->value_count > 0u)) {
        bt_xstack_demo_hfp_ringing =
            (event->values[0] != 0u) ? 1u : 0u;
    } else if (event->id == BT_XSTACK_PHONE_EVENT_LOST) {
        bt_xstack_demo_hfp_ringing = 0u;
        bt_xstack_demo_hfp_state = BT_XSTACK_HFP_STATE_IDLE;
        taskEXIT_CRITICAL();
        bt_ui_post_simple(BT_UI_EVENT_HFP_STATE,
            BT_XSTACK_HFP_STATE_IDLE, 0u, 0u);
        bt_xstack_hfp_set_audio_connected(false);
        bt_audio_task_stop();
        taskENTER_CRITICAL();
    }
    taskEXIT_CRITICAL();
}

static void bt_xstack_demo_phonebook_event(
    const bt_xstack_phonebook_event_t *event,
    void *context)
{
    uint8_t i;

    (void)context;
    if (event == NULL) {
        DBG_PRINT("xstack demo phonebook event: NULL\r\n");
        return;
    }

    DBG_PRINT("xstack demo phonebook event: %s(%u) category=%u status=%u"
        " parser_errors=%lu addr=%02x:%02x:%02x:%02x:%02x:%02x\r\n",
        bt_xstack_demo_phonebook_event_name(event->id),
        (unsigned int)event->id,
        (unsigned int)event->category,
        (unsigned int)event->status,
        (unsigned long)event->parser_error_count,
        event->address.bytes[0], event->address.bytes[1],
        event->address.bytes[2], event->address.bytes[3],
        event->address.bytes[4], event->address.bytes[5]);

    /* UI: 电话本/通话记录事件 */
    switch (event->id) {
    case BT_XSTACK_PHONEBOOK_EVENT_STARTED:
        bt_ui_post_simple(BT_UI_EVENT_PB_STARTED,
            (uint32_t)event->category, 0u, 0u);
        break;
    case BT_XSTACK_PHONEBOOK_EVENT_CONTACT: {
        bt_ui_event_t ui_ev;

        bt_ui_event_init(&ui_ev);
        ui_ev.type = BT_UI_EVENT_PB_CONTACT;
        ui_ev.pb.contact = event->data.contact;
        bt_ui_post_event(&ui_ev);
        break;
    }
    case BT_XSTACK_PHONEBOOK_EVENT_CALL: {
        bt_ui_event_t ui_ev;

        bt_ui_event_init(&ui_ev);
        ui_ev.type = BT_UI_EVENT_PB_CALL_LOG;
        ui_ev.pb.call = event->data.call;
        bt_ui_post_event(&ui_ev);
        break;
    }
    case BT_XSTACK_PHONEBOOK_EVENT_COMPLETED:
        bt_ui_post_simple(BT_UI_EVENT_PB_DONE,
            (uint32_t)event->status, 0u, 0u);
        break;
    case BT_XSTACK_PHONEBOOK_EVENT_AUTH_REQUIRED:
        bt_ui_post_simple(BT_UI_EVENT_PB_AUTH, 0u, 0u, 0u);
        break;
    case BT_XSTACK_PHONEBOOK_EVENT_ERROR:
        bt_ui_post_simple(BT_UI_EVENT_PB_ERROR,
            (uint32_t)event->status, 0u, 0u);
        break;
    default:
        break;
    }

    if (event->id == BT_XSTACK_PHONEBOOK_EVENT_CONTACT) {
        DBG_PRINT("xstack demo phonebook contact: name=\"%s\" numbers=%u"
            " truncated=%u\r\n",
            event->data.contact.name,
            (unsigned int)event->data.contact.number_count,
            (unsigned int)event->data.contact.truncated);
        for (i = 0u;
             (i < event->data.contact.number_count) &&
             (i < BT_XSTACK_PHONEBOOK_NUMBER_COUNT);
             i++) {
            DBG_PRINT("xstack demo phonebook contact number[%u]:"
                " value=\"%s\" type=0x%02x\r\n",
                (unsigned int)i,
                event->data.contact.numbers[i].value,
                (unsigned int)event->data.contact.numbers[i].type_flags);
        }
    } else if (event->id == BT_XSTACK_PHONEBOOK_EVENT_CALL) {
        DBG_PRINT("xstack demo phonebook call: name=\"%s\" number=\"%s\""
            " timestamp=\"%s\" direction=%u truncated=%u\r\n",
            event->data.call.name,
            event->data.call.number,
            event->data.call.timestamp,
            (unsigned int)event->data.call.direction,
            (unsigned int)event->data.call.truncated);
    }
}

static void bt_xstack_demo_connection_event(
    const bt_xstack_connection_event_t *event,
    void *context)
{
    (void)context;
    if (event == NULL) {
        DBG_PRINT("xstack demo conn event: NULL\r\n");
        return;
    }

    DBG_PRINT("xstack demo conn event: %s(%u) %s(%u) loss=%u"
        " addr=%02x:%02x:%02x:%02x:%02x:%02x\r\n",
        bt_xstack_demo_profile_name(event->profile),
        (unsigned int)event->profile,
        bt_xstack_demo_conn_state_name(event->state),
        (unsigned int)event->state,
        (unsigned int)event->unexpected_loss,
        event->address.bytes[0], event->address.bytes[1],
        event->address.bytes[2], event->address.bytes[3],
        event->address.bytes[4], event->address.bytes[5]);

    /* UI: 各 profile 连接状态 */
    bt_ui_post_simple(BT_UI_EVENT_CONN_STATE,
        (uint32_t)event->profile,
        (uint32_t)event->state,
        event->unexpected_loss ? 1u : 0u);

    if (event->profile != BT_XSTACK_PROFILE_AVRCP) {
        return;
    }

    taskENTER_CRITICAL();
    bt_xstack_demo_avrcp_connected =
        (event->state == BT_XSTACK_CONNECTION_CONNECTED) ? 1u : 0u;
    taskEXIT_CRITICAL();
}

static void bt_xstack_demo_audio_event(
    const bt_xstack_audio_event_t *event,
    void *context)
{
    uint32_t sample_rate;
    uint32_t channel_count;
    uint32_t bits_per_sample;
    uint8_t was_streaming;
    uint8_t is_streaming;
    uint8_t stop_audio;
    uint8_t restore_pcm;
    int status;

    (void)context;
    if (event == NULL) {
        DBG_PRINT("xstack demo audio event: NULL\r\n");
        return;
    }

    DBG_PRINT("xstack demo audio event: %s(%u) vcnt=%u",
        bt_xstack_demo_audio_event_name(event->id),
        (unsigned int)event->id, (unsigned int)event->value_count);
    if (event->value_count > 0u) {
        DBG_PRINT(" v=[%lu,%lu,%lu]",
            event->values[0], event->values[1], event->values[2]);
    }
    if (event->title[0] != '\0') {
        DBG_PRINT(" title=\"%s\"", event->title);
    }
    if (event->artist[0] != '\0') {
        DBG_PRINT(" artist=\"%s\"", event->artist);
    }
    if (event->album[0] != '\0') {
        DBG_PRINT(" album=\"%s\"", event->album);
    }
    DBG_PRINT("\r\n");

    /* UI: 音乐播放相关事件 */
    switch (event->id) {
    case BT_XSTACK_AUDIO_EVENT_A2DP_STATE:
        bt_ui_post_simple(BT_UI_EVENT_A2DP_STATE,
            (event->value_count > 0u) ? event->values[0] : 0u,
            0u, 0u);
        break;
    case BT_XSTACK_AUDIO_EVENT_A2DP_LOST:
        bt_ui_post_simple(BT_UI_EVENT_A2DP_STATE,
            BT_XSTACK_A2DP_STATE_IDLE, 0u, 0u);
        break;
    case BT_XSTACK_AUDIO_EVENT_AVRCP_STATE:
        /* 连接状态已由 connection 回调上报 */
        break;
    case BT_XSTACK_AUDIO_EVENT_PLAY_STATE:
    case BT_XSTACK_AUDIO_EVENT_TRACK_STATE:
        bt_ui_post_simple(BT_UI_EVENT_PLAY_STATE,
            (event->value_count > 0u) ? event->values[0] : 0u,
            0u, 0u);
        break;
    case BT_XSTACK_AUDIO_EVENT_PLAY_MODE:
        bt_ui_post_simple(BT_UI_EVENT_PLAY_MODE,
            (event->value_count > 0u) ? event->values[0] : 0u,
            (event->value_count > 1u) ? event->values[1] : 0u,
            0u);
        break;
    case BT_XSTACK_AUDIO_EVENT_TRACK_INFO:
        bt_ui_post_track_info(event->title, event->artist, event->album);
        break;
    case BT_XSTACK_AUDIO_EVENT_ABSOLUTE_VOLUME:
        bt_ui_post_simple(BT_UI_EVENT_ABSOLUTE_VOLUME,
            (event->value_count > 0u) ? event->values[0] : 0u,
            0u, 0u);
        break;
    default:
        break;
    }

    if ((event->id == BT_XSTACK_AUDIO_EVENT_A2DP_STATE) &&
        (event->value_count > 0u)) {
        is_streaming =
            (event->values[0] == BT_XSTACK_A2DP_STATE_STREAMING) ? 1u : 0u;
        stop_audio = 0u;
        restore_pcm = 0u;
        taskENTER_CRITICAL();
        was_streaming = bt_xstack_demo_a2dp_streaming;
        bt_xstack_demo_a2dp_streaming = is_streaming;
        if (was_streaming && !is_streaming) {
            stop_audio = 1u;
            if ((event->values[0] > BT_XSTACK_A2DP_STATE_CONNECTED) &&
                bt_xstack_demo_pcm_config_valid) {
                bt_xstack_demo_resume_pending = 1u;
            }
        }
        if (event->values[0] <= BT_XSTACK_A2DP_STATE_CONNECTED) {
            stop_audio = 1u;
            bt_xstack_demo_pcm_config_valid = 0u;
            bt_xstack_demo_resume_pending = 0u;
        } else if (is_streaming && bt_xstack_demo_resume_pending &&
            bt_xstack_demo_pcm_config_valid) {
            sample_rate = bt_xstack_demo_pcm_sample_rate;
            channel_count = bt_xstack_demo_pcm_channel_count;
            bits_per_sample = bt_xstack_demo_pcm_bits_per_sample;
            bt_xstack_demo_resume_pending = 0u;
            restore_pcm = 1u;
        }
        taskEXIT_CRITICAL();
        if (stop_audio) {
            bt_audio_task_stop();
        }
        if (restore_pcm) {
            status = bt_audio_task_configure(
                sample_rate,
                (uint8_t)channel_count,
                (uint8_t)bits_per_sample);
            if (status != 0) {
                DBG_PRINT("xstack demo PCM resume status=%d\r\n", status);
                taskENTER_CRITICAL();
                bt_xstack_demo_pcm_config_valid = 0u;
                bt_xstack_demo_resume_pending = 0u;
                taskEXIT_CRITICAL();
                bt_audio_task_stop();
            }
        }
        return;
    }
    if (event->id == BT_XSTACK_AUDIO_EVENT_A2DP_LOST) {
        taskENTER_CRITICAL();
        bt_xstack_demo_a2dp_streaming = 0u;
        bt_xstack_demo_pcm_config_valid = 0u;
        bt_xstack_demo_resume_pending = 0u;
        taskEXIT_CRITICAL();
        bt_audio_task_stop();
        return;
    }
    if ((event->id != BT_XSTACK_AUDIO_EVENT_PCM_CONFIG) ||
        (event->value_count < 3u)) {
        return;
    }

    sample_rate = event->values[0];
    channel_count = event->values[1];
    bits_per_sample = event->values[2];
    if (((sample_rate != 44100u) && (sample_rate != 48000u)) ||
        ((channel_count != 1u) && (channel_count != 2u)) ||
        (bits_per_sample != 16u)) {
        DBG_PRINT("xstack demo invalid PCM config=%u,%u,%u\r\n",
            (unsigned int)sample_rate,
            (unsigned int)channel_count,
            (unsigned int)bits_per_sample);
        taskENTER_CRITICAL();
        bt_xstack_demo_pcm_config_valid = 0u;
        bt_xstack_demo_resume_pending = 0u;
        taskEXIT_CRITICAL();
        bt_audio_task_stop();
        return;
    }

    status = bt_audio_task_configure(
        sample_rate,
        (uint8_t)channel_count,
        (uint8_t)bits_per_sample);
    if (status != 0) {
        DBG_PRINT("xstack demo PCM configure status=%d\r\n", status);
        taskENTER_CRITICAL();
        bt_xstack_demo_pcm_config_valid = 0u;
        bt_xstack_demo_resume_pending = 0u;
        taskEXIT_CRITICAL();
        bt_audio_task_stop();
        return;
    }
    taskENTER_CRITICAL();
    bt_xstack_demo_pcm_sample_rate = sample_rate;
    bt_xstack_demo_pcm_channel_count = (uint8_t)channel_count;
    bt_xstack_demo_pcm_bits_per_sample = (uint8_t)bits_per_sample;
    bt_xstack_demo_pcm_config_valid = 1u;
    bt_xstack_demo_resume_pending = 0u;
    taskEXIT_CRITICAL();
}

static void bt_xstack_demo_cover_art_event(
    const bt_xstack_cover_art_event_t *event,
    void *context)
{
    (void)context;
    if (event == NULL) {
        return;
    }

    switch (event->type) {
    case BT_XSTACK_COVER_ART_DATA: {
        bt_ui_event_t ui_ev;

        bt_ui_event_init(&ui_ev);
        ui_ev.type = BT_UI_EVENT_COVER_ART_DATA;
        ui_ev.data = event->data.data.data;
        ui_ev.data_len = event->data.data.length;
        ui_ev.data_offset = event->data.data.offset;
        bt_ui_post_event(&ui_ev);
        break;
    }
    case BT_XSTACK_COVER_ART_COMPLETED: {
        bt_ui_event_t ui_ev;

        bt_ui_event_init(&ui_ev);
        ui_ev.type = BT_UI_EVENT_COVER_ART_DONE;
        ui_ev.data_len = event->data.completed.total_length;
        bt_ui_post_event(&ui_ev);
        break;
    }
    case BT_XSTACK_COVER_ART_ERROR: {
        bt_ui_event_t ui_ev;

        bt_ui_event_init(&ui_ev);
        ui_ev.type = BT_UI_EVENT_COVER_ART_ERROR;
        ui_ev.values[0] = (uint32_t)event->data.error.native_status;
        ui_ev.value_count = 1u;
        bt_ui_post_event(&ui_ev);
        break;
    }
    default:
        break;
    }
}

static void bt_xstack_demo_pcm_event(
    const uint8_t *data,
    uint32_t length,
    void *context)
{
    (void)context;
    bt_audio_task_write_media_pcm(data, length);
}

static void bt_xstack_demo_call_pcm_event(
    const uint8_t *data,
    uint32_t length,
    void *context)
{
    (void)context;
    bt_audio_task_write_call_pcm(data, length);
}

/*
 * A microphone producer may submit each captured SCO frame with
 * bt_xstack_call_audio_write(). This demo does not synthesize uplink audio.
 */

static INT8U bt_xstack_demo_call_key_handler(
    ap_peripheral_call_key_action_t action)
{
    bt_xstack_hfp_state_t state;
    uint8_t ringing;
    bt_xstack_result_t status;

    DBG_PRINT("xstack demo call key handler: action=%u\r\n",
        (unsigned int)action);
    taskENTER_CRITICAL();
    state = bt_xstack_demo_hfp_state;
    ringing = bt_xstack_demo_hfp_ringing;
    taskEXIT_CRITICAL();

    if (action == AP_PERIPHERAL_CALL_KEY_ANSWER) {
        if (!ringing &&
            (state != BT_XSTACK_HFP_STATE_INCOMING) &&
            (state != BT_XSTACK_HFP_STATE_ACTIVE_WITH_INCOMING)) {
            return 0u;
        }
        status = bt_xstack_hfp_answer();
    } else if (action == AP_PERIPHERAL_CALL_KEY_HANGUP) {
        if (!ringing &&
            (state != BT_XSTACK_HFP_STATE_OUTGOING) &&
            (state != BT_XSTACK_HFP_STATE_INCOMING) &&
            (state != BT_XSTACK_HFP_STATE_ACTIVE) &&
            (state != BT_XSTACK_HFP_STATE_HELD) &&
            (state != BT_XSTACK_HFP_STATE_ACTIVE_WITH_INCOMING) &&
            (state != BT_XSTACK_HFP_STATE_ACTIVE_WITH_HELD) &&
            (state != BT_XSTACK_HFP_STATE_MULTIPARTY)) {
            return 0u;
        }
        status = bt_xstack_hfp_hang_up();
    } else {
        return 0u;
    }

    DBG_PRINT("xstack demo call key action=%u state=%u status=%d\r\n",
        (unsigned int)action, (unsigned int)state, status);
    return 1u;
}

static INT8U bt_xstack_demo_media_key_handler(
    ap_peripheral_media_key_action_t action)
{
    uint8_t connected;
    bt_xstack_result_t status;

    DBG_PRINT("xstack demo media key handler: action=%u\r\n",
        (unsigned int)action);
    taskENTER_CRITICAL();
    connected = bt_xstack_demo_avrcp_connected;
    taskEXIT_CRITICAL();
    if (!connected) {
        return 0u;
    }

    switch (action) {
    case AP_PERIPHERAL_MEDIA_KEY_PREVIOUS:
        status = bt_xstack_avrcp_previous();
        break;
    case AP_PERIPHERAL_MEDIA_KEY_PLAY_PAUSE:
        status = bt_xstack_avrcp_play_pause();
        break;
    case AP_PERIPHERAL_MEDIA_KEY_NEXT:
        status = bt_xstack_avrcp_next();
        break;
    default:
        return 0u;
    }

    DBG_PRINT("xstack demo media key action=%u status=%d\r\n",
        (unsigned int)action, status);
    return 1u;
}

int bt_xstack_demo_start(void)
{
    bt_xstack_config_t config = {0};
    bt_xstack_result_t status;
    bt_xstack_result_t cover_status;

    if (!bt_xstack_demo_begin_start()) {
        return BT_XSTACK_OK;
    }

    status = (bt_xstack_result_t)bt_audio_task_start();
    DBG_PRINT("xstack demo audio task status=%d\r\n", status);
    if (status != BT_XSTACK_OK) {
        bt_xstack_demo_set_start_state(BT_XSTACK_DEMO_IDLE);
        return BT_XSTACK_INTERNAL_ERROR;
    }

    config.service_callback = bt_xstack_demo_service_event;
    config.connection_callback = bt_xstack_demo_connection_event;
    config.phone_callback = bt_xstack_demo_phone_event;
    config.phonebook_callback = bt_xstack_demo_phonebook_event;
    config.audio_event_callback = bt_xstack_demo_audio_event;
    config.cover_art_callback = bt_xstack_demo_cover_art_event;
    config.media_audio_callback = bt_xstack_demo_pcm_event;
    config.call_audio_callback = bt_xstack_demo_call_pcm_event;
    status = bt_xstack_init(&config);
    DBG_PRINT("xstack demo init status=%d\r\n", status);
    if (status != BT_XSTACK_OK) {
        bt_xstack_demo_set_start_state(BT_XSTACK_DEMO_IDLE);
        bt_audio_task_stop();
        return status;
    }

    ap_peripheral_call_key_handler_register(
        bt_xstack_demo_call_key_handler);
    ap_peripheral_media_key_handler_register(
        bt_xstack_demo_media_key_handler);

    bt_xstack_demo_set_start_state(BT_XSTACK_DEMO_WAIT_POWER);
    status = bt_xstack_enable();
    DBG_PRINT("xstack demo enable status=%d\r\n", status);
    if (status != BT_XSTACK_OK) {
        bt_xstack_demo_set_start_state(BT_XSTACK_DEMO_IDLE);
        bt_audio_task_stop();
    } else {
        cover_status = bt_xstack_cover_art_set_mode(
            BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL);
        DBG_PRINT("xstack demo cover art mode status=%d\r\n", cover_status);
    }
    return status;
}

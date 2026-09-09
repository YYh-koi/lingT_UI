#include "btstack_call_audio_tx.h"

#include <string.h>

#include "port/btstack_platform.h"
#include "profiles/btstack_hfp_hf.h"

#define BT_XSTACK_CALL_AUDIO_TX_BYTES   2048u
#define BT_XSTACK_CALL_AUDIO_WRITE_MAX   255u

static uint8_t btstack_call_audio_tx_bytes[BT_XSTACK_CALL_AUDIO_TX_BYTES];
static uint8_t btstack_call_audio_tx_staging[BT_XSTACK_CALL_AUDIO_WRITE_MAX];
static uint16_t btstack_call_audio_tx_read;
static uint16_t btstack_call_audio_tx_write_index;
static uint16_t btstack_call_audio_tx_count;
static uint32_t btstack_call_audio_tx_generation;
static bool btstack_call_audio_tx_connected;
static uint16_t btstack_call_audio_tx_session_frame_length;
static bool btstack_call_audio_tx_pump_scheduled;
static bool btstack_call_audio_tx_send_in_progress;
static btstack_context_callback_registration_t
    btstack_call_audio_tx_pump_registration;

static bool btstack_call_audio_tx_has_frame_locked(void)
{
    return btstack_call_audio_tx_connected &&
        (btstack_call_audio_tx_session_frame_length > 0u) &&
        (btstack_call_audio_tx_session_frame_length <=
            BT_XSTACK_CALL_AUDIO_WRITE_MAX) &&
        (btstack_call_audio_tx_count >=
            btstack_call_audio_tx_session_frame_length);
}

static void btstack_call_audio_tx_post_pump(bool post)
{
    if (!post) {
        return;
    }
    if (!btstack_platform_post_stack_callback(
            &btstack_call_audio_tx_pump_registration)) {
        btstack_platform_enter_critical();
        btstack_call_audio_tx_pump_scheduled = false;
        btstack_platform_exit_critical();
    }
}

static bool btstack_call_audio_tx_schedule_pump_locked(void)
{
    if (!btstack_call_audio_tx_has_frame_locked() ||
        btstack_call_audio_tx_pump_scheduled) {
        return false;
    }
    btstack_call_audio_tx_pump_scheduled = true;
    return true;
}

static void btstack_call_audio_tx_pump(void *context)
{
    bool request;

    (void)context;
    btstack_platform_enter_critical();
    btstack_call_audio_tx_pump_scheduled = false;
    request = btstack_call_audio_tx_has_frame_locked() &&
        !btstack_call_audio_tx_send_in_progress;
    btstack_platform_exit_critical();
    if (request) {
        (void)btstack_hfp_hf_request_audio_can_send_now();
    }
}

static void btstack_call_audio_tx_ready(void)
{
    uint16_t frame_length;
    uint16_t send_length;
    uint16_t read_snapshot;
    uint32_t generation_snapshot;
    uint16_t index;
    int send_status;
    bool post_pump;

    btstack_platform_enter_critical();
    frame_length = btstack_call_audio_tx_session_frame_length;
    if (btstack_call_audio_tx_send_in_progress ||
        !btstack_call_audio_tx_has_frame_locked()) {
        btstack_platform_exit_critical();
        return;
    }
    btstack_call_audio_tx_send_in_progress = true;
    read_snapshot = btstack_call_audio_tx_read;
    generation_snapshot = btstack_call_audio_tx_generation;
    send_length = frame_length;
    for (index = 0u; index < frame_length; index++) {
        btstack_call_audio_tx_staging[index] =
            btstack_call_audio_tx_bytes[
                (uint16_t)((read_snapshot + index) %
                    BT_XSTACK_CALL_AUDIO_TX_BYTES)];
    }
    btstack_platform_exit_critical();

    send_status = btstack_hfp_hf_send_audio_frame(
        btstack_call_audio_tx_staging, send_length);

    btstack_platform_enter_critical();
    if ((send_status == BTSTACK_STATUS_OK) &&
        (generation_snapshot == btstack_call_audio_tx_generation) &&
        (read_snapshot == btstack_call_audio_tx_read) &&
        (btstack_call_audio_tx_count >= send_length)) {
        btstack_call_audio_tx_read = (uint16_t)(
            (btstack_call_audio_tx_read + send_length) %
                BT_XSTACK_CALL_AUDIO_TX_BYTES);
        btstack_call_audio_tx_count =
            (uint16_t)(btstack_call_audio_tx_count - send_length);
    }
    btstack_call_audio_tx_send_in_progress = false;
    post_pump = btstack_call_audio_tx_schedule_pump_locked();
    btstack_platform_exit_critical();
    btstack_call_audio_tx_post_pump(post_pump);
}

void btstack_call_audio_tx_init(void)
{
    btstack_platform_enter_critical();
    btstack_call_audio_tx_read = 0u;
    btstack_call_audio_tx_write_index = 0u;
    btstack_call_audio_tx_count = 0u;
    btstack_call_audio_tx_generation++;
    btstack_call_audio_tx_connected = false;
    btstack_call_audio_tx_session_frame_length = 0u;
    btstack_call_audio_tx_pump_scheduled = false;
    btstack_call_audio_tx_send_in_progress = false;
    memset(&btstack_call_audio_tx_pump_registration, 0,
        sizeof(btstack_call_audio_tx_pump_registration));
    btstack_call_audio_tx_pump_registration.callback =
        btstack_call_audio_tx_pump;
    btstack_platform_exit_critical();
    (void)btstack_hfp_hf_set_audio_tx_ready_callback(
        btstack_call_audio_tx_ready);
}

void btstack_call_audio_tx_session_changed(
    bool connected,
    uint16_t frame_length)
{
    bool post_pump;

    if ((frame_length == 0u) ||
        (frame_length > BT_XSTACK_CALL_AUDIO_WRITE_MAX)) {
        connected = false;
        frame_length = 0u;
    }
    btstack_platform_enter_critical();
    btstack_call_audio_tx_generation++;
    btstack_call_audio_tx_read = 0u;
    btstack_call_audio_tx_write_index = 0u;
    btstack_call_audio_tx_count = 0u;
    btstack_call_audio_tx_connected = connected;
    btstack_call_audio_tx_session_frame_length = frame_length;
    post_pump = btstack_call_audio_tx_schedule_pump_locked();
    btstack_platform_exit_critical();
    btstack_call_audio_tx_post_pump(post_pump);
}

bt_xstack_result_t btstack_call_audio_tx_write(
    const uint8_t *data,
    uint32_t length)
{
    uint16_t accepted_length;
    uint16_t index;
    bool post_pump;

    if ((data == NULL) || (length == 0u) ||
        (length > BT_XSTACK_CALL_AUDIO_WRITE_MAX)) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    accepted_length = (uint16_t)length;
    btstack_platform_enter_critical();
    if (!btstack_call_audio_tx_connected ||
        (btstack_call_audio_tx_session_frame_length == 0u)) {
        btstack_platform_exit_critical();
        return BT_XSTACK_NOT_READY;
    }
    if (accepted_length > (uint16_t)(BT_XSTACK_CALL_AUDIO_TX_BYTES -
            btstack_call_audio_tx_count)) {
        btstack_platform_exit_critical();
        return BT_XSTACK_QUEUE_FULL;
    }
    for (index = 0u; index < accepted_length; index++) {
        btstack_call_audio_tx_bytes[btstack_call_audio_tx_write_index] =
            data[index];
        btstack_call_audio_tx_write_index = (uint16_t)(
            (btstack_call_audio_tx_write_index + 1u) %
                BT_XSTACK_CALL_AUDIO_TX_BYTES);
    }
    btstack_call_audio_tx_count =
        (uint16_t)(btstack_call_audio_tx_count + accepted_length);
    post_pump = btstack_call_audio_tx_schedule_pump_locked();
    btstack_platform_exit_critical();
    btstack_call_audio_tx_post_pump(post_pump);
    return BT_XSTACK_OK;
}

bt_xstack_result_t bt_xstack_call_audio_write(
    const uint8_t *data,
    uint32_t length)
{
    return btstack_call_audio_tx_write(data, length);
}

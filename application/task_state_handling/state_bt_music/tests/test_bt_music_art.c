#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "application.h"
#include "bt_music_art.h"

#define TEST_FRAME_COUNT 4u
#define TEST_FRAME_BYTE_CAPACITY (160u * 1024u)

static DISP_T test_frames[TEST_FRAME_COUNT];
static uint8_t test_frame_bytes[TEST_FRAME_COUNT][TEST_FRAME_BYTE_CAPACITY];
static DISP_P test_free_frames[TEST_FRAME_COUNT];
static unsigned test_free_count;
static unsigned test_get_count;
static unsigned test_put_count;
static unsigned test_ready_message_count;
static uint32_t test_ready_generation;
static unsigned test_decode_count;
static unsigned test_draw_count;
static const uint32_t *test_input_messages;
static unsigned test_input_message_count;
static unsigned test_input_message_index;
static unsigned test_play_pause_count;
static unsigned test_previous_count;
static unsigned test_next_count;
static unsigned test_state_send_count;
static uint32_t test_last_state;
static unsigned test_clear_count;
static unsigned test_cache_count;
static unsigned test_cache_drain_count;
static INT32U test_cache_drain_buffer;
static INT32U test_cache_drain_size;
static int test_decode_result = STATUS_OK;
static IMAGE_DECODE_STRUCT test_last_decode;
static DISP_P test_last_draw_frame;
static unsigned test_cover_register_count;
static unsigned test_cover_mode_count;

DISP_QUEUE display_free_queue;
int ApQ;
uint8_t ApQ_para[16];
uint32_t ApQ_para_len = sizeof(ApQ_para);
unsigned test_critical_depth;

static void test_pool_init(void)
{
    unsigned i;

    memset(test_frames, 0, sizeof(test_frames));
    memset(test_frame_bytes, 0, sizeof(test_frame_bytes));
    for (i = 0u; i < TEST_FRAME_COUNT; ++i) {
        test_frames[i].base = (uintptr_t)test_frame_bytes[i];
        test_free_frames[i] = &test_frames[i];
    }
    test_free_count = TEST_FRAME_COUNT;
}

DISP_P ap_display_queue_get(DISP_QUEUE queue)
{
    (void)queue;
    assert(test_critical_depth == 0u);
    ++test_get_count;
    if (test_free_count == 0u) {
        return NULL;
    }
    return test_free_frames[--test_free_count];
}

INT32S ap_display_queue_put(DISP_QUEUE queue, DISP_P frame)
{
    (void)queue;
    assert(test_critical_depth == 0u);
    assert(frame != NULL);
    assert(test_free_count < TEST_FRAME_COUNT);
    test_free_frames[test_free_count++] = frame;
    ++test_put_count;
    return STATUS_OK;
}

INT32S msgQSend(int queue, INT32U message, void *parameter,
    INT32U parameter_size, INT32U priority)
{
    (void)queue;
    (void)priority;
    assert(test_critical_depth == 0u);
    if (message == MSG_APQ_BT_COVER_READY) {
        assert(parameter != NULL);
        assert(parameter_size == sizeof(uint32_t));
        memcpy(&test_ready_generation, parameter, sizeof(test_ready_generation));
        ++test_ready_message_count;
    } else {
        assert(!"unexpected application message");
    }
    return STATUS_OK;
}

INT32S msgQReceive(int queue, INT32U *message, void *parameter,
    INT32U parameter_size)
{
    (void)queue;
    (void)parameter;
    (void)parameter_size;
    assert(test_input_message_index < test_input_message_count);
    *message = test_input_messages[test_input_message_index++];
    return STATUS_OK;
}

INT32S state_handling_msg_send(INT32U state, void *parameter,
    INT32U parameter_size, INT32U priority)
{
    (void)parameter;
    (void)parameter_size;
    (void)priority;
    ++test_state_send_count;
    test_last_state = state;
    return STATUS_OK;
}

void ap_state_common_handling(INT32U message)
{
    (void)message;
}

bt_xstack_result_t bt_xstack_avrcp_play_pause(void)
{
    ++test_play_pause_count;
    return BT_XSTACK_OK;
}

bt_xstack_result_t bt_xstack_cover_art_register_callback(
    bt_xstack_cover_art_callback_t callback, void *context)
{
    (void)context;
    assert(callback == bt_music_art_cover_event);
    ++test_cover_register_count;
    return BT_XSTACK_OK;
}

bt_xstack_result_t bt_xstack_cover_art_set_mode(bt_xstack_cover_art_mode_t mode)
{
    assert(mode == BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL);
    ++test_cover_mode_count;
    return BT_XSTACK_OK;
}

bt_xstack_result_t bt_xstack_avrcp_previous(void)
{
    ++test_previous_count;
    return BT_XSTACK_OK;
}

bt_xstack_result_t bt_xstack_avrcp_next(void)
{
    ++test_next_count;
    return BT_XSTACK_OK;
}

uint16_t drv_l2_display_tft_width_get(void)
{
    return 320u;
}

uint16_t drv_l2_display_tft_height_get(void)
{
    return 240u;
}

uint32_t drv_l2_display_tft_fmt_get(void)
{
    return DISP_FMT_RGB565;
}

uint32_t getDispDevBufSize(void)
{
    return 320u * 240u * 2u;
}

void ap_display_clear_screen(INT32U buffer)
{
    ++test_clear_count;
    memset((void *)(uintptr_t)buffer, 0, getDispDevBufSize());
}

INT32S jpeg_buffer_decode_and_scale(IMAGE_DECODE_STRUCT *image,
    INT32U clip_xy, INT32U clip_size)
{
    (void)clip_xy;
    (void)clip_size;
    assert(test_critical_depth == 0u);
    assert(test_cache_drain_count != 0u);
    assert(test_cache_drain_buffer == (INT32U)image->image_source);
    assert(test_cache_drain_size == image->source_size);
    ++test_decode_count;
    test_last_decode = *image;
    return test_decode_result;
}

INT32S display_task_msg_send(INT32U message, void *parameter,
    INT32U parameter_size, INT32U priority)
{
    (void)priority;
    assert(test_critical_depth == 0u);
    assert(message == MSG_DISPLAY_TASK_JPEG_DRAW);
    assert(parameter_size == sizeof(DISP_P));
    memcpy(&test_last_draw_frame, parameter, sizeof(test_last_draw_frame));
    ++test_draw_count;
    return STATUS_OK;
}

void cache_invalid_range(INT32U buffer, INT32U size)
{
    (void)buffer;
    assert(size == getDispDevBufSize());
    ++test_cache_count;
}

INT32S cache_drain_range(INT32U buffer, INT32U size)
{
    assert(test_critical_depth == 0u);
    ++test_cache_drain_count;
    test_cache_drain_buffer = buffer;
    test_cache_drain_size = size;
    return STATUS_OK;
}

static void send_started(uint32_t generation)
{
    bt_xstack_cover_art_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = BT_XSTACK_COVER_ART_STARTED;
    event.generation = generation;
    bt_music_art_cover_event(&event, NULL);
}

static void send_data(uint32_t generation, uint32_t offset,
    const uint8_t *data, uint16_t length)
{
    bt_xstack_cover_art_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = BT_XSTACK_COVER_ART_DATA;
    event.generation = generation;
    event.data.data.data = data;
    event.data.data.length = length;
    event.data.data.offset = offset;
    bt_music_art_cover_event(&event, NULL);
}

static void send_completed(uint32_t generation, int32_t status,
    uint32_t total_length)
{
    bt_xstack_cover_art_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = BT_XSTACK_COVER_ART_COMPLETED;
    event.status = status;
    event.generation = generation;
    event.data.completed.total_length = total_length;
    bt_music_art_cover_event(&event, NULL);
}

static void send_property(uint32_t generation)
{
    bt_xstack_cover_art_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = BT_XSTACK_COVER_ART_PROPERTY;
    event.generation = generation;
    bt_music_art_cover_event(&event, NULL);
}

static void send_error(uint32_t generation)
{
    bt_xstack_cover_art_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = BT_XSTACK_COVER_ART_ERROR;
    event.generation = generation;
    bt_music_art_cover_event(&event, NULL);
}

static DISP_P find_retained_frame(void)
{
    unsigned i;
    unsigned j;

    for (i = 0u; i < TEST_FRAME_COUNT; ++i) {
        int is_free = 0;
        for (j = 0u; j < test_free_count; ++j) {
            if (test_free_frames[j] == &test_frames[i]) {
                is_free = 1;
                break;
            }
        }
        if (!is_free) {
            return &test_frames[i];
        }
    }
    return NULL;
}

static void rebuild_free_pool_excluding(DISP_P retained)
{
    unsigned i;

    test_free_count = 0u;
    for (i = 0u; i < TEST_FRAME_COUNT; ++i) {
        if (&test_frames[i] != retained) {
            test_free_frames[test_free_count++] = &test_frames[i];
        }
    }
}

static void test_receiver_behavior(void)
{
    static const uint8_t jpeg7[] = { 0xffu, 0xd8u, 0xffu, 0xd9u };
    static const uint8_t jpeg12[] = { 1u, 2u, 3u, 4u, 5u };
    uint8_t byte = 0x5au;
    DISP_P retained;
    unsigned before;

    send_property(1u);
    assert(test_get_count == 0u);

    send_started(7u);
    send_data(7u, 0u, jpeg7, (uint16_t)sizeof(jpeg7));
    send_completed(7u, BT_XSTACK_OK, sizeof(jpeg7));
    assert(test_ready_generation == 7u);
    assert(test_ready_message_count == 1u);
    retained = find_retained_frame();
    assert(retained != NULL);
    assert(memcmp((const void *)retained->base, jpeg7, sizeof(jpeg7)) == 0);

    before = test_put_count;
    send_started(8u);
    send_data(8u, 0u, &byte, 1u);
    send_completed(8u, -1, 1u);
    assert(test_put_count == before + 1u);
    assert(test_ready_message_count == 1u);
    assert(memcmp((const void *)retained->base, jpeg7, sizeof(jpeg7)) == 0);

    before = test_put_count;
    send_started(9u);
    send_data(9u, 1u, &byte, 1u);
    assert(test_put_count == before + 1u);

    before = test_put_count;
    send_started(10u);
    send_data(10u, BT_MUSIC_ART_MAX_JPEG_SIZE, &byte, 1u);
    send_completed(10u, BT_XSTACK_OK, BT_MUSIC_ART_MAX_JPEG_SIZE + 1u);
    assert(test_put_count == before + 1u);
    assert(test_ready_message_count == 1u);

    send_started(12u);
    send_data(12u, 0u, jpeg12, (uint16_t)sizeof(jpeg12));
    send_completed(12u, BT_XSTACK_OK, sizeof(jpeg12));
    assert(test_ready_generation == 12u);
    assert(test_ready_message_count == 2u);
    assert(test_put_count == before + 2u);
    retained = find_retained_frame();
    assert(retained != NULL);
    assert(memcmp((const void *)retained->base, jpeg12, sizeof(jpeg12)) == 0);

    before = test_put_count;
    send_error(99u);
    assert(test_put_count == before);

    while (test_free_count != 0u) {
        (void)ap_display_queue_get(display_free_queue);
    }
    send_started(13u);
    assert(test_ready_message_count == 2u);
    assert(test_decode_count == 0u);
    assert(test_draw_count == 0u);
    assert(test_critical_depth == 0u);
}

static void test_state_key_behavior(void)
{
    static const uint32_t messages[] = {
        MSG_APQ_FUNCTION_KEY_ACTIVE,
        MSG_APQ_PREV_KEY_ACTIVE,
        MSG_APQ_NEXT_KEY_ACTIVE,
        MSG_APQ_MODE
    };
    uint32_t previous_state = 0u;

    test_input_messages = messages;
    test_input_message_count = sizeof(messages) / sizeof(messages[0]);
    test_input_message_index = 0u;
    bt_music_art_state_entry(&previous_state);

    assert(test_play_pause_count == 1u);
    assert(test_previous_count == 1u);
    assert(test_next_count == 1u);
    assert(test_state_send_count == 1u);
    assert(test_last_state == STATE_VIDEO_RECORD);
    assert(test_input_message_index == test_input_message_count);
    assert(test_cover_register_count == 1u);
    assert(test_cover_mode_count == 1u);
    assert(test_critical_depth == 0u);
}

static void test_fit_rectangles(void)
{
    bt_music_art_rect_t rect;

    assert(bt_music_art_calculate_fit(200u, 200u, 320u, 240u, &rect) == 0);
    assert(rect.width == 240u && rect.height == 240u);
    assert(rect.x == 40u && rect.y == 0u);

    assert(bt_music_art_calculate_fit(400u, 200u, 320u, 240u, &rect) == 0);
    assert(rect.width == 320u && rect.height == 160u);
    assert(rect.x == 0u && rect.y == 40u);

    assert(bt_music_art_calculate_fit(100u, 200u, 320u, 240u, &rect) == 0);
    assert(rect.width == 120u && rect.height == 240u);
    assert(rect.x == 100u && rect.y == 0u);

    assert(bt_music_art_calculate_fit(0u, 200u, 320u, 240u, &rect) != 0);
    assert(bt_music_art_calculate_fit(200u, 200u, 320u, 240u, NULL) != 0);
}

static void test_jpeg_size_parser(void)
{
    static const uint8_t jpeg_200x100[] = {
        0xffu, 0xd8u,
        0xffu, 0xc0u, 0x00u, 0x0bu, 0x08u,
        0x00u, 0x64u, 0x00u, 0xc8u, 0x01u, 0x01u, 0x11u, 0x00u,
        0xffu, 0xd9u
    };
    static const uint8_t no_soi[] = { 0xffu, 0xc0u, 0x00u, 0x0bu };
    static const uint8_t sos_first[] = {
        0xffu, 0xd8u, 0xffu, 0xdau, 0x00u, 0x02u
    };
    uint16_t width = 0u;
    uint16_t height = 0u;

    assert(bt_music_art_parse_jpeg_size(jpeg_200x100,
        sizeof(jpeg_200x100), &width, &height) == 0);
    assert(width == 200u && height == 100u);
    assert(bt_music_art_parse_jpeg_size(jpeg_200x100, 8u, &width, &height) != 0);
    assert(bt_music_art_parse_jpeg_size(no_soi, sizeof(no_soi), &width, &height) != 0);
    assert(bt_music_art_parse_jpeg_size(sos_first, sizeof(sos_first), &width, &height) != 0);
    assert(bt_music_art_parse_jpeg_size(jpeg_200x100,
        sizeof(jpeg_200x100), NULL, &height) != 0);
}

static void test_deferred_centered_decode(void)
{
    static const uint8_t jpeg_200x100[] = {
        0xffu, 0xd8u,
        0xffu, 0xc0u, 0x00u, 0x0bu, 0x08u,
        0x00u, 0x64u, 0x00u, 0xc8u, 0x01u, 0x01u, 0x11u, 0x00u,
        0xffu, 0xd9u
    };
    static const uint32_t messages[] = {
        MSG_APQ_BT_COVER_READY,
        MSG_APQ_MODE
    };
    DISP_P old_cached = find_retained_frame();
    uint32_t previous_state = 0u;
    unsigned decode_before = test_decode_count;
    unsigned draw_before = test_draw_count;
    unsigned clear_before = test_clear_count;

    assert(old_cached != NULL);
    rebuild_free_pool_excluding(old_cached);
    send_started(20u);
    send_data(20u, 0u, jpeg_200x100, (uint16_t)sizeof(jpeg_200x100));
    send_completed(20u, BT_XSTACK_OK, sizeof(jpeg_200x100));
    assert(test_decode_count == decode_before);

    test_input_messages = messages;
    test_input_message_count = sizeof(messages) / sizeof(messages[0]);
    test_input_message_index = 0u;
    bt_music_art_state_entry(&previous_state);

    assert(test_decode_count == decode_before + 1u);
    assert(test_cache_drain_count == 1u);
    assert(test_draw_count == draw_before + 2u);
    assert(test_clear_count == clear_before + 2u);
    assert(test_cache_count >= 2u);
    assert(test_last_decode.source_size == sizeof(jpeg_200x100));
    assert(test_last_decode.output_buffer_width == 320u);
    assert(test_last_decode.output_buffer_height == 200u);
    assert(test_last_decode.output_image_width == 320u);
    assert(test_last_decode.output_image_height == 160u);
    assert(test_last_decode.output_format == C_SCALER_CTRL_OUT_RGB565);
    assert(test_last_decode.output_buffer_pointer ==
        (uint32_t)(test_last_draw_frame->base + 2u * 40u * 320u));
}

int main(void)
{
    test_pool_init();
    test_receiver_behavior();
    test_state_key_behavior();
    test_fit_rectangles();
    test_jpeg_size_parser();
    test_deferred_centered_decode();
    puts("PASS: Bluetooth music Cover Art receiver");
    return 0;
}

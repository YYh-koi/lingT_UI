#include <stdint.h>
#include <string.h>

#include "application.h"
#include "bt_music_art.h"

typedef struct {
    DISP_P receiving_frame;
    uint32_t receiving_generation;
    uint32_t expected_offset;
    DISP_P cached_frame;
    uint32_t cached_generation;
    uint32_t cached_size;
    DISP_P decoding_frame;
    uint32_t decoding_generation;
    uint8_t receiving;
    uint8_t page_active;
    uint8_t callback_registered;
} bt_music_art_context_t;

static bt_music_art_context_t bt_music_art_context;

int bt_music_art_calculate_fit(uint16_t source_width, uint16_t source_height,
    uint16_t screen_width, uint16_t screen_height, bt_music_art_rect_t *rect)
{
    uint32_t width;
    uint32_t height;

    if (source_width == 0u || source_height == 0u || screen_width == 0u ||
        screen_height == 0u || rect == NULL) {
        return -1;
    }

    if ((uint32_t)screen_width * source_height <=
        (uint32_t)screen_height * source_width) {
        width = screen_width;
        width &= ~7u;
        height = (uint32_t)source_height * width / source_width;
    } else {
        height = screen_height;
        width = (uint32_t)source_width * height / source_height;
        width &= ~7u;
        height = (uint32_t)source_height * width / source_width;
    }

    if (width == 0u || height == 0u) {
        return -1;
    }

    rect->width = (uint16_t)width;
    rect->height = (uint16_t)height;
    rect->x = (uint16_t)((screen_width - width) / 2u);
    rect->y = (uint16_t)((screen_height - height) / 2u);
    return 0;
}

int bt_music_art_parse_jpeg_size(const uint8_t *data, uint32_t length,
    uint16_t *width, uint16_t *height)
{
    uint32_t position = 2u;

    if (data == NULL || width == NULL || height == NULL || length < 4u ||
        data[0] != 0xffu || data[1] != 0xd8u) {
        return -1;
    }

    while (position < length) {
        uint8_t marker;
        uint16_t segment_length;

        if (data[position++] != 0xffu) {
            return -1;
        }
        while (position < length && data[position] == 0xffu) {
            ++position;
        }
        if (position >= length) {
            return -1;
        }
        marker = data[position++];
        if (marker == 0xdau || marker == 0xd9u) {
            return -1;
        }
        if (marker == 0x01u || (marker >= 0xd0u && marker <= 0xd7u)) {
            continue;
        }
        if (position + 2u > length) {
            return -1;
        }
        segment_length = (uint16_t)(((uint16_t)data[position] << 8) |
            data[position + 1u]);
        if (segment_length < 2u || position + segment_length > length) {
            return -1;
        }
        if (marker == 0xc0u || marker == 0xc1u) {
            uint16_t parsed_width;
            uint16_t parsed_height;

            if (segment_length < 8u) {
                return -1;
            }
            parsed_height = (uint16_t)(((uint16_t)data[position + 3u] << 8) |
                data[position + 4u]);
            parsed_width = (uint16_t)(((uint16_t)data[position + 5u] << 8) |
                data[position + 6u]);
            if (parsed_width == 0u || parsed_height == 0u) {
                return -1;
            }
            *width = parsed_width;
            *height = parsed_height;
            return 0;
        }
        position += segment_length;
    }

    return -1;
}

static void bt_music_art_release_frame(DISP_P frame)
{
    if (frame != NULL) {
        (void)ap_display_queue_put(display_free_queue, frame);
    }
}

static void bt_music_art_abort_receive(void)
{
    DISP_P frame;

    taskENTER_CRITICAL();
    frame = bt_music_art_context.receiving_frame;
    bt_music_art_context.receiving_frame = NULL;
    bt_music_art_context.receiving_generation = 0u;
    bt_music_art_context.expected_offset = 0u;
    bt_music_art_context.receiving = 0u;
    taskEXIT_CRITICAL();

    bt_music_art_release_frame(frame);
}

static void bt_music_art_start_receive(uint32_t generation)
{
    DISP_P frame;

    bt_music_art_abort_receive();
    frame = ap_display_queue_get(display_free_queue);
    if (frame == NULL) {
        return;
    }

    taskENTER_CRITICAL();
    bt_music_art_context.receiving_frame = frame;
    bt_music_art_context.receiving_generation = generation;
    bt_music_art_context.expected_offset = 0u;
    bt_music_art_context.receiving = 1u;
    taskEXIT_CRITICAL();
}

static void bt_music_art_publish_complete(void)
{
    DISP_P old_cached;
    uint32_t generation;

    taskENTER_CRITICAL();
    old_cached = bt_music_art_context.cached_frame;
    bt_music_art_context.cached_frame = bt_music_art_context.receiving_frame;
    bt_music_art_context.cached_generation = bt_music_art_context.receiving_generation;
    bt_music_art_context.cached_size = bt_music_art_context.expected_offset;
    generation = bt_music_art_context.cached_generation;
    bt_music_art_context.receiving_frame = NULL;
    bt_music_art_context.receiving_generation = 0u;
    bt_music_art_context.expected_offset = 0u;
    bt_music_art_context.receiving = 0u;
    taskEXIT_CRITICAL();

    bt_music_art_release_frame(old_cached);
    (void)msgQSend(ApQ, MSG_APQ_BT_COVER_READY, &generation,
        sizeof(generation), MSG_PRI_NORMAL);
}

static uint32_t bt_music_art_test_band(uint32_t x, uint32_t width)
{
    uint32_t red_end = (width / 3u) & ~1u;
    uint32_t green_end = ((width * 2u) / 3u) & ~1u;

    if (x < red_end) {
        return 0u;
    }
    if (x < green_end) {
        return 1u;
    }
    return 2u;
}

static void bt_music_art_fill_test_pattern(INT32U base)
{
    static const INT8U yuv[3][3] = {
        {76u, 85u, 255u},
        {150u, 43u, 20u},
        {27u, 255u, 108u}
    };
    static const INT16U rgb565[3] = {0xf800u, 0x07e0u, 0x001fu};
    INT32U format = drv_l2_display_tft_fmt_get();
    INT32U width = drv_l2_display_tft_width_get();
    INT32U height = drv_l2_display_tft_height_get();
    INT32U x;
    INT32U y;

    if (format == DISP_FMT_GP420) {
        INT8U *buffer = (INT8U *)base;

        for (y = 0u; y + 1u < height; y += 2u) {
            INT8U *y0 = buffer + ((y >> 1) * width * 3u);
            INT8U *uv = y0 + width;
            INT8U *y1 = uv + width;

            for (x = 0u; x < width; ++x) {
                uint32_t band = bt_music_art_test_band(x, width);

                y0[x] = yuv[band][0];
                y1[x] = yuv[band][0];
                if (((x & 1u) == 0u) && (x + 1u < width)) {
                    uv[x] = yuv[band][1];
                    uv[x + 1u] = yuv[band][2];
                }
            }
        }
    } else if (format == DISP_FMT_RGB565) {
        INT16U *buffer = (INT16U *)base;

        for (y = 0u; y < height; ++y) {
            for (x = 0u; x < width; ++x) {
                buffer[y * width + x] = rgb565[bt_music_art_test_band(x, width)];
            }
        }
    } else if (format == DISP_FMT_YUYV) {
        INT8U *buffer = (INT8U *)base;

        for (y = 0u; y < height; ++y) {
            INT8U *row = buffer + y * width * 2u;

            for (x = 0u; x < width; ++x) {
                uint32_t band = bt_music_art_test_band(x, width);

                row[x * 2u] = (x & 1u) ? yuv[band][2] : yuv[band][1];
                row[x * 2u + 1u] = yuv[band][0];
            }
        }
    } else {
        ap_display_clear_screen(base);
    }
}

static void bt_music_art_present_test_pattern(void)
{
    DISP_P frame = ap_display_queue_get(display_free_queue);
    INT32S send_status;

    if (frame == NULL) {
        return;
    }
    frame->offset = 0u;
    bt_music_art_fill_test_pattern(frame->base);
    cache_drain_range(frame->base, getDispDevBufSize());
    send_status = display_task_msg_send(MSG_DISPLAY_TASK_JPEG_DRAW, &frame,
        sizeof(frame), MSG_PRI_NORMAL);
    if (send_status == STATUS_FAIL) {
        bt_music_art_release_frame(frame);
    }
}

static void bt_music_art_restore_decoding(DISP_P frame, uint32_t generation,
    uint32_t size)
{
    DISP_P release_frame = NULL;

    taskENTER_CRITICAL();
    bt_music_art_context.decoding_frame = NULL;
    bt_music_art_context.decoding_generation = 0u;
    if (bt_music_art_context.cached_frame == NULL ||
        bt_music_art_context.cached_generation < generation) {
        release_frame = bt_music_art_context.cached_frame;
        bt_music_art_context.cached_frame = frame;
        bt_music_art_context.cached_generation = generation;
        bt_music_art_context.cached_size = size;
    } else {
        release_frame = frame;
    }
    taskEXIT_CRITICAL();

    bt_music_art_release_frame(release_frame);
}

static void bt_music_art_decode_latest(void)
{
    DISP_P compressed_frame;
    DISP_P output_frame = NULL;
    IMAGE_DECODE_STRUCT image;
    bt_music_art_rect_t rect;
    uint32_t generation;
    uint32_t compressed_size;
    uint32_t display_format;
    uint16_t source_width;
    uint16_t source_height;
    uint16_t screen_width;
    uint16_t screen_height;
    INT32S decode_status = STATUS_FAIL;
    INT32S display_status;

    taskENTER_CRITICAL();
    if (!bt_music_art_context.page_active || bt_music_art_context.cached_frame == NULL) {
        taskEXIT_CRITICAL();
        return;
    }
    compressed_frame = bt_music_art_context.cached_frame;
    generation = bt_music_art_context.cached_generation;
    compressed_size = bt_music_art_context.cached_size;
    bt_music_art_context.cached_frame = NULL;
    bt_music_art_context.cached_generation = 0u;
    bt_music_art_context.cached_size = 0u;
    bt_music_art_context.decoding_frame = compressed_frame;
    bt_music_art_context.decoding_generation = generation;
    taskEXIT_CRITICAL();

    if (bt_music_art_parse_jpeg_size((const uint8_t *)(uintptr_t)compressed_frame->base,
        compressed_size, &source_width, &source_height) != 0) {
        goto done;
    }

    screen_width = drv_l2_display_tft_width_get();
    screen_height = drv_l2_display_tft_height_get();
    if (bt_music_art_calculate_fit(source_width, source_height, screen_width,
        screen_height, &rect) != 0) {
        goto done;
    }

    output_frame = ap_display_queue_get(display_free_queue);
    if (output_frame == NULL) {
        goto done;
    }
    output_frame->offset = 0u;
    ap_display_clear_screen(output_frame->base);

    memset(&image, 0, sizeof(image));
    image.image_source = (INT32S)(uintptr_t)compressed_frame->base;
    image.source_size = compressed_size;
    image.source_type = TK_IMAGE_SOURCE_TYPE_BUFFER;
    display_format = drv_l2_display_tft_fmt_get();
    if (display_format == DISP_FMT_RGB565) {
        image.output_format = C_SCALER_CTRL_OUT_RGB565;
        image.output_buffer_pointer = output_frame->base +
            2u * ((uint32_t)rect.y * screen_width + rect.x);
    } else if (display_format == DISP_FMT_GP420) {
        image.output_format = C_SCALER_CTRL_OUT_GP420;
        image.output_buffer_pointer = output_frame->base +
            (uint32_t)rect.y * screen_width + rect.x;
    } else if (display_format == DISP_FMT_YUYV) {
        image.output_format = C_SCALER_CTRL_OUT_YUYV;
        image.output_buffer_pointer = output_frame->base +
            2u * ((uint32_t)rect.y * screen_width + rect.x);
    } else {
        goto done;
    }
    image.output_ratio = 0u;
    image.out_of_boundary_color = 0x008080u;
    image.output_buffer_width = screen_width;
    image.output_buffer_height = (uint16_t)(screen_height - rect.y);
    image.output_image_width = rect.width;
    image.output_image_height = rect.height;

    cache_drain_range(compressed_frame->base, compressed_size);
    decode_status = jpeg_buffer_decode_and_scale(&image, 0u, 0u);
    if (decode_status == STATUS_OK) {
        cache_invalid_range(output_frame->base, getDispDevBufSize());
        display_status = display_task_msg_send(MSG_DISPLAY_TASK_JPEG_DRAW,
            &output_frame, sizeof(output_frame), MSG_PRI_NORMAL);
        if (display_status == STATUS_OK) {
            output_frame = NULL;
        }
    }

done:
    bt_music_art_release_frame(output_frame);
    bt_music_art_restore_decoding(compressed_frame, generation, compressed_size);
}

void bt_music_art_cover_event(const bt_xstack_cover_art_event_t *event, void *context)
{
    uint32_t expected_offset;

    (void)context;
    if (event == NULL) {
        return;
    }

    switch (event->type) {
    case BT_XSTACK_COVER_ART_STARTED:
        bt_music_art_start_receive(event->generation);
        break;

    case BT_XSTACK_COVER_ART_DATA:
        if (!bt_music_art_context.receiving ||
            event->generation != bt_music_art_context.receiving_generation) {
            break;
        }
        expected_offset = bt_music_art_context.expected_offset;
        if (event->data.data.offset != expected_offset ||
            event->data.data.data == NULL ||
            event->data.data.length > BT_MUSIC_ART_MAX_JPEG_SIZE - expected_offset) {
            bt_music_art_abort_receive();
            break;
        }
        memcpy((uint8_t *)(uintptr_t)bt_music_art_context.receiving_frame->base + expected_offset,
            event->data.data.data, event->data.data.length);
        bt_music_art_context.expected_offset = expected_offset + event->data.data.length;
        break;

    case BT_XSTACK_COVER_ART_COMPLETED:
        if (bt_music_art_context.receiving &&
            event->generation == bt_music_art_context.receiving_generation &&
            event->status == BT_XSTACK_OK &&
            event->data.completed.total_length == bt_music_art_context.expected_offset) {
            bt_music_art_publish_complete();
        } else if (bt_music_art_context.receiving &&
            event->generation == bt_music_art_context.receiving_generation) {
            bt_music_art_abort_receive();
        }
        break;

    case BT_XSTACK_COVER_ART_ERROR:
        if (bt_music_art_context.receiving &&
            event->generation == bt_music_art_context.receiving_generation) {
            bt_music_art_abort_receive();
        }
        break;

    case BT_XSTACK_COVER_ART_PROPERTY:
    case BT_XSTACK_COVER_ART_PROPERTIES_COMPLETED:
    default:
        break;
    }
}

void bt_music_art_state_entry(void *previous_state)
{
    EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
    INT32U message;
    bt_xstack_result_t callback_status = BT_XSTACK_OK;
    bt_xstack_result_t mode_status = BT_XSTACK_OK;
    uint32_t generation = 0u;

    (void)previous_state;

    if (!bt_music_art_context.callback_registered) {
        callback_status = bt_xstack_cover_art_register_callback(bt_music_art_cover_event, NULL);
        if (callback_status == BT_XSTACK_OK) {
            mode_status = bt_xstack_cover_art_set_mode(BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL);
            if (mode_status == BT_XSTACK_OK) bt_music_art_context.callback_registered = 1u;
        }
    }
    taskENTER_CRITICAL();
    bt_music_art_context.page_active = 1u;
    if (bt_music_art_context.cached_frame != NULL) {
        generation = bt_music_art_context.cached_generation;
    }
    taskEXIT_CRITICAL();

    bt_music_art_present_test_pattern();
    if (generation != 0u) {
        (void)msgQSend(ApQ, MSG_APQ_BT_COVER_READY, &generation, sizeof(generation), MSG_PRI_NORMAL);
    }

    while (exit_flag == EXIT_RESUME) {
        if (msgQReceive(ApQ, &message, (void *)ApQ_para, ApQ_para_len) == STATUS_FAIL) {
            continue;
        }

        switch (message) {
        case MSG_APQ_MODE:
            (void)state_handling_msg_send(STATE_VIDEO_RECORD, NULL, 0u, MSG_PRI_NORMAL);
            exit_flag = EXIT_BREAK;
            break;

        case MSG_APQ_FUNCTION_KEY_ACTIVE:
            (void)bt_xstack_avrcp_play_pause();
            break;

        case MSG_APQ_PREV_KEY_ACTIVE:
            (void)bt_xstack_avrcp_previous();
            break;

        case MSG_APQ_NEXT_KEY_ACTIVE:
            (void)bt_xstack_avrcp_next();
            break;

        case MSG_APQ_BT_COVER_READY:
            bt_music_art_decode_latest();
            break;

        default:
            ap_state_common_handling(message);
            break;
        }
    }

    taskENTER_CRITICAL();
    bt_music_art_context.page_active = 0u;
    taskEXIT_CRITICAL();
}

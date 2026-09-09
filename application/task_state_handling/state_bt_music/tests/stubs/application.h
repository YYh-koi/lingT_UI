#ifndef TEST_APPLICATION_H
#define TEST_APPLICATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define DBG_PRINT(...) printf(__VA_ARGS__)

typedef int32_t INT32S;
typedef uint32_t INT32U;
typedef uint8_t INT8U;

typedef struct {
    uintptr_t base;
    uint32_t offset;
} DISP_T, *DISP_P;

typedef struct {
    int32_t image_source;
    uint32_t source_size;
    uint32_t source_type;
    uint32_t output_format;
    uint32_t output_ratio;
    uint32_t out_of_boundary_color;
    uint16_t output_buffer_width;
    uint16_t output_buffer_height;
    uint16_t output_image_width;
    uint16_t output_image_height;
    uint32_t output_buffer_pointer;
} IMAGE_DECODE_STRUCT;

typedef int DISP_QUEUE;

typedef enum {
    EXIT_RESUME = 0,
    EXIT_BREAK
} EXIT_FLAG_ENUM;

#define STATUS_OK 0
#define STATUS_FAIL (-1)
#define MSG_APQ_BT_COVER_READY 0x1234u
#define MSG_APQ_FUNCTION_KEY_ACTIVE 0x1235u
#define MSG_APQ_PREV_KEY_ACTIVE 0x1236u
#define MSG_APQ_NEXT_KEY_ACTIVE 0x1237u
#define MSG_APQ_MODE 0x1238u
#define MSG_APQ_MENU_KEY_ACTIVE 0x1239u
#define MSG_DISPLAY_TASK_JPEG_DRAW 0x2234u
#define MSG_PRI_NORMAL 0u
#define STATE_VIDEO_RECORD 2u
#define TK_IMAGE_SOURCE_TYPE_BUFFER 1u
#define DISP_FMT_RGB565 1u
#define DISP_FMT_GP420 2u
#define DISP_FMT_YUYV 3u
#define C_SCALER_CTRL_OUT_RGB565 11u
#define C_SCALER_CTRL_OUT_GP420 12u
#define C_SCALER_CTRL_OUT_YUYV 13u

extern DISP_QUEUE display_free_queue;
extern int ApQ;
extern uint8_t ApQ_para[];
extern uint32_t ApQ_para_len;
extern unsigned test_critical_depth;

DISP_P ap_display_queue_get(DISP_QUEUE queue);
INT32S ap_display_queue_put(DISP_QUEUE queue, DISP_P frame);
INT32S msgQSend(int queue, INT32U message, void *parameter,
    INT32U parameter_size, INT32U priority);
INT32S msgQReceive(int queue, INT32U *message, void *parameter,
    INT32U parameter_size);
INT32S state_handling_msg_send(INT32U state, void *parameter,
    INT32U parameter_size, INT32U priority);
void ap_state_common_handling(INT32U message);
uint16_t drv_l2_display_tft_width_get(void);
uint16_t drv_l2_display_tft_height_get(void);
uint32_t drv_l2_display_tft_fmt_get(void);
uint32_t getDispDevBufSize(void);
void ap_display_clear_screen(INT32U buffer);
INT32S jpeg_buffer_decode_and_scale(IMAGE_DECODE_STRUCT *image,
    INT32U clip_xy, INT32U clip_size);
INT32S display_task_msg_send(INT32U message, void *parameter,
    INT32U parameter_size, INT32U priority);
void cache_invalid_range(INT32U buffer, INT32U size);
INT32S cache_drain_range(INT32U buffer, INT32U size);
#define taskENTER_CRITICAL() do { ++test_critical_depth; } while (0)
#define taskEXIT_CRITICAL() do { --test_critical_depth; } while (0)

#endif

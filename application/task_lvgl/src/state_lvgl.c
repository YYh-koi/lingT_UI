/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2026 by Generalplus Inc.                         *
 *                                                                        *
 *  state_lvgl.c - LVGL 任务（TP2915 1024x600 RGB565 直读链路）           *
 *                                                                        *
 *  显示链路：LVGL 软件渲染 RGB565 -> 全屏 FB(DDR) -> cache 刷回          *
 *        -> display task 每帧 drv_l2_tft_buffer_set(lvgl_display_buf)   *
 *           TFT 直读显示（硬件每帧自动重扫 FBI，无需软件触发刷新）       *
 *  输入：OK/PRE/MODE 三键 GPIO poll -> LVGL keypad 导航                  *
 *        （蓝牙媒体控制复用 SDK 媒体键通路，见 bt_xstack_demo.c）        *
 *  首期页面：蓝牙音乐播放器（连接状态/曲目/进度/控制按钮）               *
 *                                                                        *
 **************************************************************************/
#include "customer.h"   /* 需先包含：C_LVGL_SUPPORT 由命令行 -DLVGL_SUPPORT 在此展开 */
#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
#include "state_lvgl.h"

#include "lvgl.h"

#include "application.h"        /* pTurnkeyArgs / LVGLTaskQ / DBG_PRINT */
#include "bt_ui_events.h"        /* 蓝牙事件适配层 */
#include "bt_xstack_audio.h"     /* AVRCP 控制 */
#include "gp_stdlib.h"          /* gp_malloc_align / gp_free */
#include "gplib_fs.h"           /* FS_NAND_APP / fs_read */
#include "proto.h"              /* fs_lfnopen / fs_fseek / fs_fclose */
#include "turnkey_drv_msg.h"    /* MSG_DISPLAY_TASK_LVGL_DRAW / MSG_LVGL_1SEC */
#include "ap_display_func.h"    /* DISPLAY_PATH_LVGL / 显示路径切换 */
#include "drv_l1_arm_cp15.h"    /* cache_invalid_range */
#include "board_config.h"       /* OK_KEY / PRE_KEY / MODE_KEY / *_ACTIVE */
#include "task.h"                /* xstack 回调与 LVGL 任务间的临界区 */

/**************************************************************************
 *                         M A C R O S                                    *
 **************************************************************************/
#define LVGL_H_RES          1024
#define LVGL_V_RES          600
#define LVGL_DRAW_BUF_DIV   10      /* LVGL draw buffer = 1/10 屏（部分刷新） */
#define LVGL_BT_EVENT_QUEUE_LEN 8u
#define LVGL_COVER_JPEG_MAX_BYTES (64u * 1024u)
#define LVGL_COVER_SIZE       240u
#define LVGL_NAND_FS_LETTER   'U'
#define LVGL_NAND_PATH_MAX    96u

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
/* 显示驱动 */
static lv_disp_draw_buf_t  s_draw_buf;
static lv_disp_drv_t       s_disp_drv;
static lv_color_t         *s_draw_buf_mem;
static INT32U              s_lvgl_fb;     /* 全屏 RGB565 FB（DDR），TFT 直读源 */

/* 输入驱动 */
static lv_indev_drv_t      s_indev_drv;
static lv_group_t         *s_group;

/* 音乐播放器 UI 控件 */
static lv_obj_t *s_ui_conn;     /* 连接状态 */
static lv_obj_t *s_ui_title;    /* 曲目名 */
static lv_obj_t *s_ui_artist;   /* 艺术家 */
static lv_obj_t *s_ui_album;    /* 专辑 */
static lv_obj_t *s_ui_prog;     /* 播放进度 */
static lv_obj_t *s_btn_play;    /* 播放/暂停 */
static lv_obj_t *s_btn_prev;
static lv_obj_t *s_btn_next;
static lv_obj_t *s_ui_volume;
static lv_obj_t *s_ui_cover;
static INT8U     s_playing = 0;

/* Cover Art 数据只在 xstack 回调内拷贝；JPEG 解码及 lv_img 更新均在 LVGL 任务内完成。 */
static uint8_t *s_cover_jpeg;
static lv_color_t *s_cover_rgb565;
static uint32_t s_cover_jpeg_size;
static volatile uint8_t s_cover_receiving;
static volatile uint8_t s_cover_ready;
static volatile uint8_t s_cover_decoding;
static lv_img_dsc_t s_cover_img;

/* U: is a read-only LVGL file-system drive backed by the NAND application
 * area.  Resource files are converted LVGL v8 binary images, e.g.
 * "U:/001.bin".  No image data is linked into the firmware. */
typedef struct {
    INT16S fd;
    INT32U position;
} lvgl_nand_file_t;

static lv_fs_drv_t s_nand_fs_drv;
static INT8U s_nand_fs_mounted;

extern INT32S jpeg_buffer_decode_and_scale(
    IMAGE_DECODE_STRUCT *img_decode_struct,
    INT32U clip_xy,
    INT32U clip_size);
extern INT32S cache_drain_range(INT32U buffer, INT32U size);

static BOOLEAN lvgl_nand_fs_select(void)
{
    if (s_nand_fs_mounted == 0u) {
        if (fs_mount(FS_NAND_APP) < 0) {
            return FALSE;
        }
        s_nand_fs_mounted = 1u;
    }

    return (fs_changedisk(FS_NAND_APP) >= 0) ? TRUE : FALSE;
}

static BOOLEAN lvgl_nand_path_copy(const char *src, CHAR *dst)
{
    INT32U index = 0u;

    while ((*src == '/') || (*src == '\\')) {
        src++;
    }
    while ((*src != '\0') && (index < (LVGL_NAND_PATH_MAX - 1u))) {
        dst[index++] = (*src == '/') ? '\\' : *src;
        src++;
    }
    dst[index] = '\0';
    return (index > 0u) && (*src == '\0');
}

static bool lvgl_nand_ready_cb(lv_fs_drv_t *drv)
{
    (void)drv;
    return lvgl_nand_fs_select() ? true : false;
}

static void *lvgl_nand_open_cb(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    CHAR nand_path[LVGL_NAND_PATH_MAX];
    INT32S fd;
    lvgl_nand_file_t *file;

    (void)drv;
    if ((mode != LV_FS_MODE_RD) || !lvgl_nand_fs_select() ||
        !lvgl_nand_path_copy(path, nand_path)) {
        return NULL;
    }

    fd = fs_lfnopen(nand_path, O_RDONLY, D_NORMAL);
    if (fd < 0) {
        return NULL;
    }

    file = lv_mem_alloc(sizeof(*file));
    if (file == NULL) {
        fs_fclose((INT16S)fd);
        return NULL;
    }
    file->fd = (INT16S)fd;
    file->position = 0u;
    return file;
}

static lv_fs_res_t lvgl_nand_close_cb(lv_fs_drv_t *drv, void *file_p)
{
    lvgl_nand_file_t *file = (lvgl_nand_file_t *)file_p;
    INT16S result;

    (void)drv;
    if (file == NULL) {
        return LV_FS_RES_INV_PARAM;
    }
    if (!lvgl_nand_fs_select()) {
        return LV_FS_RES_HW_ERR;
    }
    result = fs_fclose(file->fd);
    lv_mem_free(file);
    return (result < 0) ? LV_FS_RES_FS_ERR : LV_FS_RES_OK;
}

static lv_fs_res_t lvgl_nand_read_cb(lv_fs_drv_t *drv, void *file_p,
                                     void *buf, uint32_t btr, uint32_t *br)
{
    lvgl_nand_file_t *file = (lvgl_nand_file_t *)file_p;
    INT32S bytes;

    (void)drv;
    if ((file == NULL) || (buf == NULL) || !lvgl_nand_fs_select()) {
        return LV_FS_RES_HW_ERR;
    }
    bytes = fs_read(file->fd, (INT32U)buf, btr);
    if (bytes < 0) {
        return LV_FS_RES_FS_ERR;
    }
    file->position += (INT32U)bytes;
    if (br != NULL) {
        *br = (uint32_t)bytes;
    }
    return LV_FS_RES_OK;
}

static lv_fs_res_t lvgl_nand_seek_cb(lv_fs_drv_t *drv, void *file_p,
                                     uint32_t pos, lv_fs_whence_t whence)
{
    lvgl_nand_file_t *file = (lvgl_nand_file_t *)file_p;
    INT16S origin;
    INT64S result;

    (void)drv;
    if ((file == NULL) || !lvgl_nand_fs_select()) {
        return LV_FS_RES_HW_ERR;
    }
    if (whence == LV_FS_SEEK_SET) {
        origin = SEEK_SET;
    } else if (whence == LV_FS_SEEK_CUR) {
        origin = SEEK_CUR;
    } else if (whence == LV_FS_SEEK_END) {
        origin = SEEK_END;
    } else {
        return LV_FS_RES_INV_PARAM;
    }

    result = fs_fseek(file->fd, (INT64S)pos, origin);
    if (result < 0) {
        return LV_FS_RES_FS_ERR;
    }
    if (whence == LV_FS_SEEK_SET) {
        file->position = pos;
    } else if (whence == LV_FS_SEEK_CUR) {
        file->position += pos;
    } else {
        file->position = (INT32U)result;
    }
    return LV_FS_RES_OK;
}

static lv_fs_res_t lvgl_nand_tell_cb(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    lvgl_nand_file_t *file = (lvgl_nand_file_t *)file_p;

    (void)drv;
    if ((file == NULL) || (pos_p == NULL)) {
        return LV_FS_RES_INV_PARAM;
    }
    *pos_p = file->position;
    return LV_FS_RES_OK;
}

static void lvgl_nand_fs_register(void)
{
    lv_fs_drv_init(&s_nand_fs_drv);
    s_nand_fs_drv.letter = LVGL_NAND_FS_LETTER;
    s_nand_fs_drv.open_cb = lvgl_nand_open_cb;
    s_nand_fs_drv.close_cb = lvgl_nand_close_cb;
    s_nand_fs_drv.read_cb = lvgl_nand_read_cb;
    s_nand_fs_drv.seek_cb = lvgl_nand_seek_cb;
    s_nand_fs_drv.tell_cb = lvgl_nand_tell_cb;
    s_nand_fs_drv.ready_cb = lvgl_nand_ready_cb;
    lv_fs_drv_register(&s_nand_fs_drv);
}

/* xstack 回调只将事件复制进该短队列；LVGL 任务轮询并完成实际绘制。 */
static bt_ui_event_t s_bt_event_queue[LVGL_BT_EVENT_QUEUE_LEN];
static volatile INT8U s_bt_event_head;
static volatile INT8U s_bt_event_tail;

/* LVGL 任务启动门：main 在 wifi_ap_mode_enable() 返回后置 1。
   避免 LVGL 启动期大块 DDR 写入/cache 刷回/首帧渲染与 atbm SDIO init 重叠干扰
   （DEBUG 实验，稳定后可按需保留/移除） */
INT32U g_lvgl_start_ok = 0;

/**************************************************************************
 *               F U N C T I O N    I M P L E M E N T A T I O N S         *
 **************************************************************************/

/* ── LVGL 显示 flush：渲染区 -> 全屏 FB + cache 刷回 ────────────────── */
static void lvgl_disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    INT32U area_w = lv_area_get_width(area);
    INT32U area_h = lv_area_get_height(area);
    lv_color_t *fb = (lv_color_t *)s_lvgl_fb;
    INT32U y;

    (void)drv;

    /* DEBUG: 前几次 + 每 10 片打印一次（整帧=10 片时 flush# 10 的整数倍即帧边界，
       复位时能看出渲染推进到第几片/第几帧） */
    {
        static INT32U s_flush_cnt = 0;
        if ((s_flush_cnt < 3) || ((s_flush_cnt % 10) == 0)) {
            DBG_PRINT("[LVGL] flush#%u area=(%d,%d)-(%d,%d) w=%u h=%u\r\n",
                      s_flush_cnt, area->x1, area->y1, area->x2, area->y2,
                      area_w, area_h);
        }
        s_flush_cnt++;
    }

    /* LVGL 部分刷新，把渲染区逐行拷入全屏 FB 对应位置 */
    for (y = 0; y < area_h; y++) {
        memcpy(fb + (area->y1 + y) * LVGL_H_RES + area->x1,
               color_p + y * area_w,
               area_w * sizeof(lv_color_t));
    }

    /* Cortex-A7 write-back cache：CPU 写 cache，TFT DMA 直读 DDR，必须刷回。
       FB 物理连续，只刷本次 area 覆盖的行区间（area->y1 起的 area_h 行 × 整行宽）。
       刷整行而非仅 x 区间是安全的超集：本区间外的脏行此前已各自刷过，且 LVGL 单任务
       串行写-刷，刷回时不存在未刷的脏数据（多刷到的干净行 clean 为空操作）。
       相比整屏 1.2MB 的 clean+invalidate，cache/总线压力大幅下降。 */
    cache_invalid_range(s_lvgl_fb + area->y1 * LVGL_H_RES * sizeof(lv_color_t),
                        area_h * LVGL_H_RES * sizeof(lv_color_t));

    lv_disp_flush_ready(drv);
}

/* ── LVGL 按键输入：OK=ENTER / PRE=PREV / MODE=NEXT ────────────────── */
static void lvgl_indev_key_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    BOOLEAN ok   = gpio_read_io((INT32U)OK_KEY);
    BOOLEAN pre  = gpio_read_io((INT32U)PRE_KEY);
    BOOLEAN mode = gpio_read_io((INT32U)MODE_KEY);

    (void)drv;

    if (ok == (BOOLEAN)OK_KEY_ACTIVE) {
        data->key   = LV_KEY_ENTER;
        data->state = LV_INDEV_STATE_PR;
    } else if (pre == (BOOLEAN)PRE_KEY_ACTIVE) {
        data->key   = LV_KEY_PREV;
        data->state = LV_INDEV_STATE_PR;
    } else if (mode == (BOOLEAN)MODE_KEY_ACTIVE) {
        data->key   = LV_KEY_NEXT;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->key   = LV_KEY_ENTER;
        data->state = LV_INDEV_STATE_REL;
    }
}

/* ── 音乐播放器 UI ─────────────────────────────────────────────────── */
/* 首期占位：媒体控制后续接入 bt_xstack_demo 的 AVRCP 通路 */
static void lvgl_bt_music_btn_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    bt_xstack_result_t status;

    if (btn == s_btn_play) {
        /* 先更新本地视觉状态；手机 AVRCP 回调抵达后会再以真实状态校正。 */
        s_playing = s_playing ? 0u : 1u;
        lv_label_set_text(lv_obj_get_child(s_btn_play, 0),
                          s_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        status = bt_xstack_avrcp_play_pause();
        DBG_PRINT("[LVGL] AVRCP play/pause status=%d\r\n", status);
        if (status != BT_XSTACK_OK) {
            s_playing = s_playing ? 0u : 1u;
            lv_label_set_text(lv_obj_get_child(s_btn_play, 0),
                              s_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        }
    } else if (btn == s_btn_prev) {
        status = bt_xstack_avrcp_previous();
        DBG_PRINT("[LVGL] AVRCP previous status=%d\r\n", status);
    } else if (btn == s_btn_next) {
        status = bt_xstack_avrcp_next();
        DBG_PRINT("[LVGL] AVRCP next status=%d\r\n", status);
    }
}

static void lvgl_bt_cover_receive_event(const bt_ui_event_t *event)
{
    uint32_t received_size;

    if (event->type == BT_UI_EVENT_COVER_ART_ERROR) {
        s_cover_receiving = 0u;
        s_cover_ready = 0u;
        return;
    }
    if (event->type == BT_UI_EVENT_COVER_ART_DATA) {
        if ((event->data == NULL) || (event->data_len == 0u) ||
            (s_cover_jpeg == NULL) || s_cover_decoding) {
            return;
        }
        if (event->data_offset == 0u) {
            s_cover_jpeg_size = 0u;
            s_cover_ready = 0u;
            s_cover_receiving = 1u;
        }
        received_size = s_cover_jpeg_size;
        if ((!s_cover_receiving) || (event->data_offset != received_size) ||
            (event->data_len > (LVGL_COVER_JPEG_MAX_BYTES - received_size))) {
            s_cover_receiving = 0u;
            return;
        }
        memcpy(s_cover_jpeg + received_size, event->data, event->data_len);
        s_cover_jpeg_size = received_size + event->data_len;
        return;
    }
    if ((event->type == BT_UI_EVENT_COVER_ART_DONE) && s_cover_receiving &&
        (event->data_len == s_cover_jpeg_size)) {
        s_cover_receiving = 0u;
        s_cover_ready = 1u;
        DBG_PRINT("[LVGL] cover received %u bytes\r\n",
                  (unsigned int)s_cover_jpeg_size);
    }
}

static void lvgl_bt_cover_decode_if_ready(void)
{
    IMAGE_DECODE_STRUCT image;
    uint32_t jpeg_size;

    if ((!s_cover_ready) || (s_cover_jpeg == NULL) ||
        (s_cover_rgb565 == NULL)) {
        return;
    }

    taskENTER_CRITICAL();
    jpeg_size = s_cover_jpeg_size;
    s_cover_ready = 0u;
    s_cover_decoding = 1u;
    taskEXIT_CRITICAL();
    if (jpeg_size == 0u) {
        s_cover_decoding = 0u;
        return;
    }

    memset(&image, 0, sizeof(image));
    image.image_source = (INT32S)(uintptr_t)s_cover_jpeg;
    image.source_size = jpeg_size;
    image.source_type = TK_IMAGE_SOURCE_TYPE_BUFFER;
    image.output_format = C_SCALER_CTRL_OUT_RGB565;
    image.output_ratio = 0u;
    image.out_of_boundary_color = 0x008080u;
    image.output_buffer_width = LVGL_COVER_SIZE;
    image.output_buffer_height = LVGL_COVER_SIZE;
    image.output_image_width = LVGL_COVER_SIZE;
    image.output_image_height = LVGL_COVER_SIZE;
    image.output_buffer_pointer = (INT32U)s_cover_rgb565;

    cache_drain_range((INT32U)s_cover_jpeg, jpeg_size);
    if (jpeg_buffer_decode_and_scale(&image, 0u, 0u) != STATUS_OK) {
        DBG_PRINT("[LVGL] cover JPEG decode failed\r\n");
        s_cover_decoding = 0u;
        return;
    }
    cache_invalid_range((INT32U)s_cover_rgb565,
                        LVGL_COVER_SIZE * LVGL_COVER_SIZE * sizeof(lv_color_t));
    s_cover_img.header.cf = LV_IMG_CF_TRUE_COLOR;
    s_cover_img.header.w = LVGL_COVER_SIZE;
    s_cover_img.header.h = LVGL_COVER_SIZE;
    s_cover_img.data_size = LVGL_COVER_SIZE * LVGL_COVER_SIZE * sizeof(lv_color_t);
    s_cover_img.data = (const uint8_t *)s_cover_rgb565;
    lv_img_set_src(s_ui_cover, &s_cover_img);
    DBG_PRINT("[LVGL] cover displayed %ux%u\r\n",
              (unsigned int)LVGL_COVER_SIZE,
              (unsigned int)LVGL_COVER_SIZE);
    s_cover_decoding = 0u;
}

static void lvgl_bt_music_ui_create(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_t *lbl;
    lv_obj_t *cover_frame;
    lv_obj_t *side_bar;
    lv_obj_t *nav_active;
    lv_obj_t *record_status;

    /* 车机媒体页：左侧固定导航，中央只呈现当前歌曲与必要驾驶状态。 */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A0B0E), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x151015), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    side_bar = lv_obj_create(scr);
    lv_obj_set_size(side_bar, 118, LVGL_V_RES);
    lv_obj_set_style_bg_color(side_bar, lv_color_hex(0x111216), 0);
    lv_obj_set_style_bg_opa(side_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(side_bar, 0, 0);
    lv_obj_set_style_radius(side_bar, 0, 0);
    lv_obj_clear_flag(side_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(side_bar, LV_ALIGN_LEFT_MID, 0, 0);

    lbl = lv_label_create(side_bar);
    lv_label_set_text(lbl, LV_SYMBOL_HOME "\nHome");
    lv_obj_set_style_text_font(lbl, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x8C8D93), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 104);

    lbl = lv_label_create(side_bar);
    lv_label_set_text(lbl, LV_SYMBOL_VIDEO "\nRecorder");
    lv_obj_set_style_text_font(lbl, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x8C8D93), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 202);

    nav_active = lv_obj_create(side_bar);
    lv_obj_set_size(nav_active, 92, 86);
    lv_obj_set_style_bg_color(nav_active, lv_color_hex(0x48202A), 0);
    lv_obj_set_style_bg_opa(nav_active, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(nav_active, lv_color_hex(0xE94250), 0);
    lv_obj_set_style_border_width(nav_active, 2, 0);
    lv_obj_set_style_radius(nav_active, 12, 0);
    lv_obj_clear_flag(nav_active, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(nav_active, LV_ALIGN_TOP_MID, 0, 290);
    lbl = lv_label_create(nav_active);
    lv_label_set_text(lbl, LV_SYMBOL_AUDIO "\nMusic");
    lv_obj_set_style_text_font(lbl, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lbl);

    lbl = lv_label_create(side_bar);
    lv_label_set_text(lbl, LV_SYMBOL_SETTINGS "\nSettings");
    lv_obj_set_style_text_font(lbl, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x8C8D93), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 410);

    lbl = lv_label_create(scr);
    lv_label_set_text(lbl, "Bluetooth Music");
    lv_obj_set_style_text_font(lbl, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xF1EEF0), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 154, 28);

    lbl = lv_label_create(scr);
    lv_label_set_text(lbl, LV_SYMBOL_VIDEO "  REC");
    lv_obj_set_style_text_font(lbl, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xF04B58), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_RIGHT, -246, 30);

    cover_frame = lv_obj_create(scr);
    lv_obj_set_size(cover_frame, 272, 272);
    lv_obj_set_style_bg_color(cover_frame, lv_color_hex(0x1B1B20), 0);
    lv_obj_set_style_bg_opa(cover_frame, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(cover_frame, lv_color_hex(0x34333A), 0);
    lv_obj_set_style_border_width(cover_frame, 1, 0);
    lv_obj_set_style_radius(cover_frame, 16, 0);
    lv_obj_set_style_shadow_color(cover_frame, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_width(cover_frame, 14, 0);
    lv_obj_set_style_shadow_opa(cover_frame, LV_OPA_40, 0);
    lv_obj_clear_flag(cover_frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(cover_frame, LV_ALIGN_TOP_LEFT, 154, 116);

    record_status = lv_obj_create(scr);
    lv_obj_set_size(record_status, 220, 116);
    lv_obj_set_style_bg_color(record_status, lv_color_hex(0x16171C), 0);
    lv_obj_set_style_bg_opa(record_status, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(record_status, lv_color_hex(0x35363D), 0);
    lv_obj_set_style_border_width(record_status, 1, 0);
    lv_obj_set_style_radius(record_status, 14, 0);
    lv_obj_clear_flag(record_status, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(record_status, LV_ALIGN_TOP_RIGHT, -32, 82);
    lbl = lv_label_create(record_status);
    lv_label_set_text(lbl, LV_SYMBOL_VIDEO "  Recording");
    lv_obj_set_style_text_font(lbl, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xF04B58), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 18, 20);
    lbl = lv_label_create(record_status);
    lv_label_set_text(lbl, "Front camera active");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x9FA0A6), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 18, 62);

    /* 连接状态（右上角） */
    s_ui_conn = lv_label_create(scr);
    lv_label_set_text(s_ui_conn, "Disconnected");
    lv_obj_set_style_text_font(s_ui_conn, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ui_conn, lv_color_hex(0xFFC5CB), 0);
    lv_obj_set_style_bg_color(s_ui_conn, lv_color_hex(0x4A1B23), 0);
    lv_obj_set_style_bg_opa(s_ui_conn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_ui_conn, 14, 0);
    lv_obj_set_style_pad_left(s_ui_conn, 12, 0);
    lv_obj_set_style_pad_right(s_ui_conn, 12, 0);
    lv_obj_set_style_pad_top(s_ui_conn, 6, 0);
    lv_obj_set_style_pad_bottom(s_ui_conn, 6, 0);
    lv_obj_align(s_ui_conn, LV_ALIGN_TOP_RIGHT, -32, 22);

    /* 曲目名 */
    s_ui_title = lv_label_create(scr);
    lv_label_set_text(s_ui_title, "No Track");
    lv_obj_set_style_text_font(s_ui_title, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(s_ui_title, lv_color_white(), 0);
    lv_obj_set_width(s_ui_title, 405);
    lv_label_set_long_mode(s_ui_title, LV_LABEL_LONG_DOT);
    lv_obj_align(s_ui_title, LV_ALIGN_TOP_LEFT, 472, 180);

    /* 艺术家 */
    s_ui_artist = lv_label_create(scr);
    lv_label_set_text(s_ui_artist, "Waiting for track information");
    lv_obj_set_style_text_font(s_ui_artist, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(s_ui_artist, lv_color_hex(0xD0BBC0), 0);
    lv_obj_set_width(s_ui_artist, 405);
    lv_label_set_long_mode(s_ui_artist, LV_LABEL_LONG_DOT);
    lv_obj_align(s_ui_artist, LV_ALIGN_TOP_LEFT, 472, 228);

    s_ui_album = lv_label_create(scr);
    lv_label_set_text(s_ui_album, "");
    lv_obj_set_style_text_font(s_ui_album, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(s_ui_album, lv_color_hex(0x937C82), 0);
    lv_obj_set_width(s_ui_album, 405);
    lv_label_set_long_mode(s_ui_album, LV_LABEL_LONG_DOT);
    lv_obj_align(s_ui_album, LV_ALIGN_TOP_LEFT, 472, 276);

    s_ui_cover = lv_img_create(cover_frame);
    lv_obj_set_size(s_ui_cover, LVGL_COVER_SIZE, LVGL_COVER_SIZE);
    lv_obj_set_style_bg_color(s_ui_cover, lv_color_hex(0x2A2023), 0);
    lv_obj_set_style_bg_opa(s_ui_cover, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_ui_cover, 10, 0);
    lv_obj_center(s_ui_cover);

    /* 播放进度条 */
    s_ui_prog = lv_bar_create(scr);
    lv_obj_set_size(s_ui_prog, 470, 8);
    lv_obj_set_style_bg_color(s_ui_prog, lv_color_hex(0x34343A), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_ui_prog, lv_color_hex(0xE94250), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_ui_prog, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_radius(s_ui_prog, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_bar_set_range(s_ui_prog, 0, 100);
    lv_bar_set_value(s_ui_prog, 0, LV_ANIM_OFF);
    lv_obj_align(s_ui_prog, LV_ALIGN_TOP_LEFT, 472, 340);

    s_ui_volume = lv_bar_create(scr);
    lv_obj_set_size(s_ui_volume, 120, 6);
    lv_obj_set_style_bg_color(s_ui_volume, lv_color_hex(0x3C2A2E), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_ui_volume, lv_color_hex(0xE94250), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_ui_volume, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_radius(s_ui_volume, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_bar_set_range(s_ui_volume, 0, 100);
    lv_bar_set_value(s_ui_volume, 0, LV_ANIM_OFF);
    lv_obj_align(s_ui_volume, LV_ALIGN_TOP_RIGHT, -32, 66);

    /* 控制按钮：上一曲 / 播放暂停 / 下一曲 */
    s_btn_prev = lv_btn_create(scr);
    lv_obj_set_size(s_btn_prev, 72, 72);
    lv_obj_set_style_bg_color(s_btn_prev, lv_color_hex(0x242024), 0);
    lv_obj_set_style_bg_opa(s_btn_prev, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_btn_prev, 0, 0);
    lv_obj_set_style_radius(s_btn_prev, LV_RADIUS_CIRCLE, 0);
    lbl = lv_label_create(s_btn_prev);
    lv_label_set_text(lbl, LV_SYMBOL_PREV);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xF5E9EB), 0);
    lv_obj_center(lbl);
    lv_obj_align(s_btn_prev, LV_ALIGN_BOTTOM_MID, -12, -54);

    s_btn_play = lv_btn_create(scr);
    lv_obj_set_size(s_btn_play, 88, 88);
    lv_obj_set_style_bg_color(s_btn_play, lv_color_hex(0xE94250), 0);
    lv_obj_set_style_bg_opa(s_btn_play, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_btn_play, 0, 0);
    lv_obj_set_style_radius(s_btn_play, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_color(s_btn_play, lv_color_hex(0xB52C39), 0);
    lv_obj_set_style_shadow_width(s_btn_play, 14, 0);
    lv_obj_set_style_shadow_opa(s_btn_play, LV_OPA_60, 0);
    lbl = lv_label_create(s_btn_play);
    lv_label_set_text(lbl, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);
    lv_obj_align(s_btn_play, LV_ALIGN_BOTTOM_MID, 104, -46);

    s_btn_next = lv_btn_create(scr);
    lv_obj_set_size(s_btn_next, 72, 72);
    lv_obj_set_style_bg_color(s_btn_next, lv_color_hex(0x242024), 0);
    lv_obj_set_style_bg_opa(s_btn_next, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_btn_next, 0, 0);
    lv_obj_set_style_radius(s_btn_next, LV_RADIUS_CIRCLE, 0);
    lbl = lv_label_create(s_btn_next);
    lv_label_set_text(lbl, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xF5E9EB), 0);
    lv_obj_center(lbl);
    lv_obj_align(s_btn_next, LV_ALIGN_BOTTOM_MID, 220, -54);

    lv_obj_add_event_cb(s_btn_play, lvgl_bt_music_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_prev, lvgl_bt_music_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_next, lvgl_bt_music_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 按键导航：PRE/NEXT 移动焦点，OK(ENTER) 触发点击 */
    lv_group_add_obj(s_group, s_btn_prev);
    lv_group_add_obj(s_group, s_btn_play);
    lv_group_add_obj(s_group, s_btn_next);
    lv_group_focus_obj(s_btn_play);
}

static void lvgl_bt_music_apply_event(const bt_ui_event_t *event)
{
    if (event == NULL) {
        return;
    }

    switch (event->type) {
    case BT_UI_EVENT_COVER_ART_DATA:
    case BT_UI_EVENT_COVER_ART_DONE:
    case BT_UI_EVENT_COVER_ART_ERROR:
        lvgl_bt_cover_receive_event(event);
        return;
    case BT_UI_EVENT_CONN_STATE:
        if ((event->values[0] == BT_XSTACK_PROFILE_A2DP_SINK) ||
            (event->values[0] == BT_XSTACK_PROFILE_AVRCP)) {
            if (event->values[1] == BT_XSTACK_CONNECTION_CONNECTED) {
                lv_label_set_text(s_ui_conn, "Connected");
            } else if (event->values[1] == BT_XSTACK_CONNECTION_CONNECTING) {
                lv_label_set_text(s_ui_conn, "Connecting");
            } else if (event->values[1] == BT_XSTACK_CONNECTION_DISCONNECTED) {
                lv_label_set_text(s_ui_conn, "Disconnected");
            }
        }
        break;
    case BT_UI_EVENT_A2DP_STATE:
        if (event->values[0] == BT_XSTACK_A2DP_STATE_STREAMING) {
            lv_label_set_text(s_ui_conn, "Streaming");
        }
        break;
    case BT_UI_EVENT_PLAY_STATE:
        s_playing = (event->values[0] == BT_XSTACK_PLAY_STATE_PLAYING) ? 1u : 0u;
        lv_label_set_text(lv_obj_get_child(s_btn_play, 0),
                          s_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        break;
    case BT_UI_EVENT_TRACK_INFO:
        lv_label_set_text(s_ui_title,
                          event->text[0] != '\0' ? event->text : "Unknown title");
        lv_label_set_text(s_ui_artist,
                          event->text2[0] != '\0' ? event->text2 : "Unknown artist");
        lv_label_set_text(s_ui_album,
                          event->text3[0] != '\0' ? event->text3 : "");
        break;
    case BT_UI_EVENT_ABSOLUTE_VOLUME:
        lv_bar_set_value(s_ui_volume,
                         (int32_t)((event->values[0] * 100u) / 127u),
                         LV_ANIM_OFF);
        break;
    default:
        break;
    }
}

static void lvgl_bt_music_process_events(void)
{
    bt_ui_event_t event;
    INT8U have_event;

    do {
        have_event = 0u;
        taskENTER_CRITICAL();
        if (s_bt_event_tail != s_bt_event_head) {
            event = s_bt_event_queue[s_bt_event_tail];
            s_bt_event_tail = (INT8U)((s_bt_event_tail + 1u) % LVGL_BT_EVENT_QUEUE_LEN);
            have_event = 1u;
        }
        taskEXIT_CRITICAL();
        if (have_event) {
            lvgl_bt_music_apply_event(&event);
        }
    } while (have_event);
}

static void lvgl_bt_ui_notify(const bt_ui_event_t *event, void *context)
{
    INT8U next_head;

    (void)context;
    if (event == NULL) {
        return;
    }

    switch (event->type) {
    case BT_UI_EVENT_CONN_STATE:
    case BT_UI_EVENT_A2DP_STATE:
    case BT_UI_EVENT_PLAY_STATE:
    case BT_UI_EVENT_TRACK_INFO:
    case BT_UI_EVENT_ABSOLUTE_VOLUME:
        break;
    case BT_UI_EVENT_COVER_ART_DATA:
    case BT_UI_EVENT_COVER_ART_DONE:
    case BT_UI_EVENT_COVER_ART_ERROR:
        lvgl_bt_cover_receive_event(event);
        return;
    default:
        return;
    }

    taskENTER_CRITICAL();
    next_head = (INT8U)((s_bt_event_head + 1u) % LVGL_BT_EVENT_QUEUE_LEN);
    if (next_head != s_bt_event_tail) {
        s_bt_event_queue[s_bt_event_head] = *event;
        s_bt_event_head = next_head;
    }
    taskEXIT_CRITICAL();
}

/**************************************************************************
 *               T A S K    I N T E R F A C E                             *
 **************************************************************************/
/* LVGL tick 源：由 FreeRTOS tick hook（port.c vApplicationTickHook）调用，
   configTICK_RATE_HZ=1000 -> 每 1ms 喂一次。主循环无需再手动 lv_tick_inc。 */
void lvgl_tick_inc(void)
{
    lv_tick_inc(1);
}
void lvgl_state_task_init(void)
{
    TASK_APP_ARGUMENT* pLVGLTaskArgs = &pTurnkeyArgs->taskLVGL;

    DBG_PRINT("==Run:%s==\r\n", __FUNCTION__);

    if (pLVGLTaskArgs->taskAppQ == 0) {
        pLVGLTaskArgs->taskAppQ = msgQCreate(pLVGLTaskArgs->taskMsgLen,
                                             pLVGLTaskArgs->taskMsgLen,
                                             pLVGLTaskArgs->taskRecvBufLen);
        if (pLVGLTaskArgs->taskAppQ == 0) {
            ERR_PRINT("fail to create pLVGLTaskArgs->taskAppQ \r\n");
            return;
        }
    }
}

void state_lvgl_entry(void const *arg)
{
    INT32U msg_id = 0;
    INT32U msg_para = 0;
    INT32U fb_size;
    INT32U draw_buf_size;
    INT32U cover_rgb_size;
    lv_indev_t *indev;

    (void)arg;

    /* 0. 等 main 完成 wifi/BT 初始化（置 g_lvgl_start_ok）后再跑，
          避开 atbm SDIO init 窗口（DEBUG 实验） */
    while (g_lvgl_start_ok == 0) {
        vTaskDelay(20);
    }

    /* 1. LVGL 核心初始化 */
    lv_init();
    lvgl_nand_fs_register();
    DBG_PRINT("[LVGL] NAND resource drive U: registered\r\n");

    /* 2. 分配显示缓冲（DDR） */
    fb_size = LVGL_H_RES * LVGL_V_RES * sizeof(lv_color_t);
    draw_buf_size = LVGL_H_RES * (LVGL_V_RES / LVGL_DRAW_BUF_DIV) * sizeof(lv_color_t);
    s_lvgl_fb = (INT32U)gp_malloc_align(fb_size, 64);
    s_draw_buf_mem = (lv_color_t *)gp_malloc_align(draw_buf_size, 64);
    s_cover_jpeg = (uint8_t *)gp_malloc_align(LVGL_COVER_JPEG_MAX_BYTES, 64);
    cover_rgb_size = LVGL_COVER_SIZE * LVGL_COVER_SIZE * sizeof(lv_color_t);
    s_cover_rgb565 = (lv_color_t *)gp_malloc_align(cover_rgb_size, 64);
    if ((s_lvgl_fb == 0) || (s_draw_buf_mem == 0) ||
        (s_cover_jpeg == NULL) || (s_cover_rgb565 == NULL)) {
        ERR_PRINT("[LVGL] fb alloc fail (fb=0x%x buf=0x%x)\r\n",
                  s_lvgl_fb, (INT32U)s_draw_buf_mem);
        return;
    }
    memset((void *)s_lvgl_fb, 0x00, fb_size);       /* 初始全黑 */
    cache_invalid_range(s_lvgl_fb, fb_size);        /* 刷回 DDR */
    DBG_PRINT("[LVGL] fb=0x%x(%dKB) drawbuf=0x%x(%dKB)\r\n",
              s_lvgl_fb, fb_size / 1024, (INT32U)s_draw_buf_mem, draw_buf_size / 1024);

    /* 3. 注册显示驱动 */
    lv_disp_draw_buf_init(&s_draw_buf, s_draw_buf_mem, NULL,
                          LVGL_H_RES * LVGL_V_RES / LVGL_DRAW_BUF_DIV);
    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res  = LVGL_H_RES;
    s_disp_drv.ver_res  = LVGL_V_RES;
    s_disp_drv.flush_cb = lvgl_disp_flush_cb;
    s_disp_drv.draw_buf = &s_draw_buf;   /* 必须绑定：首帧刷新会解引用 draw_buf->buf */
    lv_disp_drv_register(&s_disp_drv);

    /* 4. 注册按键输入（keypad + group 导航） */
    s_group = lv_group_create();
    lv_group_set_default(s_group);
    lv_indev_drv_init(&s_indev_drv);
    s_indev_drv.type    = LV_INDEV_TYPE_KEYPAD;
    s_indev_drv.read_cb = lvgl_indev_key_read_cb;
    indev = lv_indev_drv_register(&s_indev_drv);
    lv_indev_set_group(indev, s_group);

    /* 5. 切显示路径到 LVGL，告知 display task 显示源地址 */
    ap_display_buff_lvgl_draw(&s_lvgl_fb, MSG_DISPLAY_TASK_LVGL_DRAW);
    ap_state_handling_display_path_set(DISPLAY_PATH_LVGL);
    DBG_PRINT("[LVGL] display path -> LVGL\r\n");

    /* 6. 创建音乐播放器页面 */
    lvgl_bt_music_ui_create();
    lv_obj_invalidate(lv_scr_act()); /* 首帧完整覆盖此前显示路径遗留的内容 */
    bt_ui_set_notify(lvgl_bt_ui_notify, NULL);

    /* 7. 主循环：收消息（蓝牙事件/1 秒定时）+ 驱动 LVGL 渲染
       （LVGL tick 由 FreeRTOS tick hook -> lvgl_tick_inc() 每 1ms 喂，这里只跑 timer）*/
    while (1) {
        if (msgQReceive_timeout(LVGLTaskQ, &msg_id, &msg_para, 0, 20) == 0) {
            switch (msg_id) {
            case MSG_LVGL_1SEC:
                break;
            default:
                break;
            }
        }

        lvgl_bt_music_process_events();
        lvgl_bt_cover_decode_if_ready();
        lv_timer_handler();

    }
}
#endif /* (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT) */

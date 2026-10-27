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
#include "dashboard_dynamic_assets.h"

#include "application.h"        /* pTurnkeyArgs / LVGLTaskQ / DBG_PRINT */
#include "bt_ui_events.h"        /* 蓝牙事件适配层 */
#include "bt_xstack_audio.h"     /* AVRCP 控制 */
#include "bt_xstack_phone.h"     /* HFP 拨号 */
#include "bt_xstack_demo.h"      /* PBAP re-sync for phone page entry */
#include "gp_stdlib.h"          /* gp_malloc_align / gp_free */
#include "gplib_fs.h"           /* FS_NAND_APP / fs_read */
#include "proto.h"              /* fs_fread / fs_fwrite / fs_fseek */
#include "turnkey_drv_msg.h"    /* MSG_DISPLAY_TASK_LVGL_DRAW / MSG_LVGL_1SEC */
#include "ap_display_func.h"    /* DISPLAY_PATH_LVGL / 显示路径切换 */
#include "drv_l1_arm_cp15.h"    /* cache_invalid_range */
#include "drv_l1_spifc.h"       /* U3 SPI-NOR read-only probe */
#include "board_config.h"       /* OK_KEY / PRE_KEY / MODE_KEY / *_ACTIVE */
#include "task.h"                /* xstack 回调与 LVGL 任务间的临界区 */

/**************************************************************************
 *                         M A C R O S                                    *
 **************************************************************************/
#define LVGL_H_RES          1024
#define LVGL_V_RES          600
#define LVGL_DRAW_BUF_DIV   1       /* One complete frame: avoids 10 serial flushes on a page change. */
#define LVGL_FLUSH_TRACE_EN 0       /* Keep UART free while measuring render performance. */
#define LVGL_BT_EVENT_QUEUE_LEN 64u
/* Never drain an unbounded Bluetooth burst before giving LVGL a render turn.
 * PBAP can enqueue dozens of records; four messages per pass keeps key focus
 * and AVRCP state visually responsive while that sync is in progress. */
#define LVGL_BT_EVENTS_PER_FRAME 4u
#define LVGL_PLAY_STATE_CONFIRM_TIMEOUT_MS 1500u
#define LVGL_COVER_JPEG_MAX_BYTES (64u * 1024u)
/* The scaler accepts this source/output size reliably. The LVGL object keeps
 * this real size, then centres its 150px visual result with a 160/256 zoom. */
#define LVGL_COVER_SIZE         240u
#define LVGL_COVER_DISPLAY_SIZE 150u
#define LVGL_COVER_ZOOM         ((LVGL_COVER_DISPLAY_SIZE * 256u) / LVGL_COVER_SIZE)
#define LVGL_PHONE_CONTACT_MAX 32u
#define LVGL_PHONE_CALL_MAX    48u
#define LVGL_PHONE_VISIBLE_ROWS 5u
#define LVGL_PHONE_NUMBER_MAX  BT_XSTACK_PHONEBOOK_NUMBER_MAX
#define LVGL_PHONE_TAB_DIAL    0u
#define LVGL_PHONE_TAB_RECENT  1u
#define LVGL_PHONE_TAB_CONTACT 2u
#define LVGL_PHONE_TAB_COUNT   3u
#define LVGL_PHONE_DIAL_KEY_COUNT 14u
/* 通话覆盖页：来电/去电/通话中自动弹出，通话结束自动关闭，不占底部导航槽位。
   对方号码与已同步的 PBAP 通讯录按“数字尾号（>=7 位）”匹配联系人名。 */
#define LVGL_CALL_NUMBER_MIN_DIGITS 7u
#define LVGL_CALL_TICK_MS           1000u
/* The large dashboard is one opaque U3 image. Only the speed arc, headlight
 * and voice state below are redrawn, so board preview animation stays local. */
#define LVGL_DASHBOARD_DEMO_ANIMATION      1u
/* The TP2915 currently scans at about 49 Hz (DCLK DIVIDE_5).  A 30 Hz UI
 * animation remains below that limit while making the dashboard fluid. */
#define LVGL_DASHBOARD_HEADLIGHT_SIM_BLINK 1u
#define LVGL_DASHBOARD_VOICE_SIM_BLINK     0u
#define LVGL_DASHBOARD_TICK_MS             33u
#define LVGL_DASHBOARD_SPEED_STEP_TENTHS   2
/* dashboard.bin is the red-frame crop of the former 1024x600 material,
 * enlarged to fill the panel.  Every separately drawn dashboard object uses
 * these conversions so it stays aligned with that new background. */
#define LVGL_DASHBOARD_CROP_X              24u
#define LVGL_DASHBOARD_CROP_Y              41u
#define LVGL_DASHBOARD_CROP_W              970u
#define LVGL_DASHBOARD_CROP_H              516u
#define LVGL_DASHBOARD_MAP_X(value) \
    ((lv_coord_t)(((((INT32U)(value) - LVGL_DASHBOARD_CROP_X) * LVGL_H_RES) + \
                   (LVGL_DASHBOARD_CROP_W / 2u)) / LVGL_DASHBOARD_CROP_W))
#define LVGL_DASHBOARD_MAP_Y(value) \
    ((lv_coord_t)(((((INT32U)(value) - LVGL_DASHBOARD_CROP_Y) * LVGL_V_RES) + \
                   (LVGL_DASHBOARD_CROP_H / 2u)) / LVGL_DASHBOARD_CROP_H))
#define LVGL_DASHBOARD_MAP_W(value) \
    ((lv_coord_t)((((INT32U)(value) * LVGL_H_RES) + \
                   (LVGL_DASHBOARD_CROP_W / 2u)) / LVGL_DASHBOARD_CROP_W))
#define LVGL_DASHBOARD_MAP_H(value) \
    ((lv_coord_t)((((INT32U)(value) * LVGL_V_RES) + \
                   (LVGL_DASHBOARD_CROP_H / 2u)) / LVGL_DASHBOARD_CROP_H))
#define LVGL_DASHBOARD_VOICE_WAVE_BARS     3u
#define LVGL_DASHBOARD_VOICE_WAVE_BAR_W    LVGL_DASHBOARD_MAP_W(4u)
#define LVGL_DASHBOARD_VOICE_WAVE_GAP      LVGL_DASHBOARD_MAP_W(3u)
#define LVGL_DASHBOARD_VOICE_WAVE_ICON_HALF LVGL_DASHBOARD_MAP_W(12u)
#define LVGL_DASHBOARD_VOICE_WAVE_MIN_H    LVGL_DASHBOARD_MAP_H(6u)
#define LVGL_DASHBOARD_VOICE_WAVE_MAX_H    LVGL_DASHBOARD_MAP_H(26u)
/* 009_plane is the immutable page frame in the original artwork.  Its
 * interior, not the whole 1024x600 root screen, is the switchable area. */
#define LVGL_CONTENT_X       LVGL_DASHBOARD_MAP_X(70u)
#define LVGL_CONTENT_Y       LVGL_DASHBOARD_MAP_Y(126u)
#define LVGL_CONTENT_W       LVGL_DASHBOARD_MAP_W(882u)
#define LVGL_CONTENT_H       LVGL_DASHBOARD_MAP_H(318u)
#define LVGL_PAGE_DASHBOARD  0u
#define LVGL_PAGE_LOCATION   1u
#define LVGL_PAGE_PHONE      2u
#define LVGL_PAGE_MUSIC      3u
#define LVGL_PAGE_SPEED      4u
#define LVGL_PAGE_COUNT      5u
/* 033_location remains in the fixed bottom navigation strip.  The dynamic
 * headlight and voice indicators use the two free slots called out in the
 * approved layout, so none of the icon layers overlap. */
#define LVGL_DASHBOARD_HEADLIGHT_X         LVGL_DASHBOARD_MAP_X(365u)
#define LVGL_DASHBOARD_HEADLIGHT_Y         LVGL_DASHBOARD_MAP_Y(430u)
/* The voice indicator occupies the clear lower area, centred horizontally
 * in that space so it never intersects the fixed navigation strip. */
#define LVGL_DASHBOARD_VOICE_X             LVGL_DASHBOARD_MAP_X(512u)
#define LVGL_DASHBOARD_VOICE_Y             LVGL_DASHBOARD_MAP_Y(505u)
#define LVGL_DASHBOARD_VOICE_WAVE_CENTER_X LVGL_DASHBOARD_MAP_X(524u)
#define LVGL_DASHBOARD_VOICE_WAVE_CENTER_Y LVGL_DASHBOARD_MAP_Y(518u)
#define LVGL_NAND_FS_LETTER   'U'
#define LVGL_NAND_PATH_MAX    96u
#define LVGL_UI_INSTALL_PATH_MAX 96u
#define LVGL_UI_INSTALL_NAME_MAX 64u
#define LVGL_UI_INSTALL_MAX_FILES 256u
#define LVGL_UI_INSTALL_BUF_BYTES 4096u
#define LVGL_UI_MANIFEST_MAGIC 0x55494D47u /* "UIMG", little-endian */
#define LVGL_SPIFC_BYTES          (8u * 1024u * 1024u)
#define LVGL_SPIFC_ERASE_BYTES    4096u
#define LVGL_SPIFC_PAGE_BYTES     256u
#define LVGL_SPIFC_UI_NEED_BYTES  (5u * 1024u * 1024u)
#define LVGL_SPUI_BASE             0x001B0000u
#define LVGL_SPUI_LIMIT            0x006B0000u
#define LVGL_SPUI_ERASE_BYTES      0x00010000u
#define LVGL_SPUI_MAGIC            0x49555053u /* "SPUI" */

#if (LVGL_RUNTIME_LOG_EN == 1)
#define LVGL_RUNTIME_LOG(...)      DBG_PRINT(__VA_ARGS__)
#else
#define LVGL_RUNTIME_LOG(...)      do { } while (0)
#endif

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
static lv_obj_t *s_ui_title;    /* 曲目名 */
static lv_obj_t *s_ui_artist;   /* 艺术家 */
static lv_obj_t *s_btn_play;    /* 播放/暂停 */
static lv_obj_t *s_btn_prev;
static lv_obj_t *s_btn_next;
static lv_obj_t *s_ui_cover;
static lv_obj_t *s_ui_cover_loading;
static volatile INT8U s_playing = 0;
static volatile INT8U s_play_confirmed = 0;
static volatile INT8U s_play_command_pending;
static volatile INT8U s_play_command_target;
static volatile INT8U s_play_command_guard;
static TickType_t s_play_command_tick;
static uint32_t s_play_command_sequence;
static uint32_t s_play_request_event_sequence;
static uint32_t s_play_event_sequence;

/* Cover Art 数据只在 xstack 回调内拷贝；JPEG 解码及 lv_img 更新均在 LVGL 任务内完成。
 * 接收和解码必须使用不同缓冲：解码上一首的 JPEG 时，蓝牙回调仍可能正在传输下一首。 */
static uint8_t *s_cover_jpeg;
static uint8_t *s_cover_decode_jpeg;
static lv_color_t *s_cover_rgb565;
static uint32_t s_cover_jpeg_size;
static uint32_t s_cover_generation;
static volatile uint8_t s_cover_receiving;
static volatile uint8_t s_cover_generation_valid;
static volatile uint8_t s_cover_ready;
static volatile uint8_t s_cover_decoding;
static volatile uint8_t s_cover_cached;
static lv_img_dsc_t s_cover_img;
static TickType_t s_cover_ready_tick;
static TickType_t s_cover_submit_tick;
static uint32_t s_cover_submit_generation;
static uint8_t s_cover_flush_pending;

/* Board port of tools/lvgl_pc_simulator/src/ui_dashboard.c. The large,
 * static material is flattened in U3; small state materials remain separate. */
typedef struct {
    const char *file;
    lv_coord_t x;
    lv_coord_t y;
} lvgl_dashboard_asset_t;

static lv_obj_t *s_dashboard_speed_label;
static lv_obj_t *s_dashboard_main_arc;
static lv_obj_t *s_dashboard_background;
static lv_obj_t *s_dashboard_headlight;
static lv_obj_t *s_dashboard_voice;
static lv_img_dsc_t s_dashboard_headlight_off_img;
static lv_img_dsc_t s_dashboard_headlight_on_img;
static lv_img_dsc_t s_dashboard_voice_idle_img;
static lv_img_dsc_t s_dashboard_voice_active_img;
static lv_obj_t *s_dashboard_voice_wave[LVGL_DASHBOARD_VOICE_WAVE_BARS * 2u];
static INT32S s_dashboard_speed_tenths = 1080;
static INT32S s_dashboard_speed_direction = 1;
static INT16U s_dashboard_headlight_elapsed;
static INT16U s_dashboard_voice_elapsed;
static INT16U s_dashboard_voice_wave_phase;
static INT8U s_dashboard_headlight_is_on;
static INT8U s_dashboard_voice_is_active;
static volatile INT8U s_dashboard_active;
static volatile INT8U s_page_switch_pending;
static lv_timer_t *s_dashboard_timer;
static volatile INT8U s_active_page = LVGL_PAGE_DASHBOARD;
static lv_obj_t *s_content_panel;
static INT8U s_nav_focus = LVGL_PAGE_DASHBOARD;
static lv_obj_t *s_nav_marker;
static char s_music_title[BT_UI_TEXT_MAX] = "No Track";
static char s_music_artist[BT_UI_TEXT_MAX] = "Bluetooth device";
static char s_music_album[BT_UI_TEXT_MAX];

typedef struct {
    char name[BT_XSTACK_PHONEBOOK_NAME_MAX + 1u];
    char number[BT_XSTACK_PHONEBOOK_NUMBER_MAX + 1u];
} lvgl_phone_contact_t;

typedef struct {
    char name[BT_XSTACK_PHONEBOOK_NAME_MAX + 1u];
    char number[BT_XSTACK_PHONEBOOK_NUMBER_MAX + 1u];
    char timestamp[BT_XSTACK_PHONEBOOK_TIMESTAMP_MAX + 1u];
    uint8_t direction;
} lvgl_phone_call_t;

/* Phone page model.  It is fed exclusively from bt_ui_event_t and rendered
 * only by the LVGL task. */
static volatile INT8U s_phone_active;
/* Set after the dialer cursor moves down from its last row into the fixed
 * dashboard navigation strip. */
static INT8U s_phone_nav_active;
static INT8U s_phone_tab;
static INT8U s_phone_cursor;
static INT8U s_phone_list_offset[LVGL_PHONE_TAB_COUNT];
static INT8U s_phone_sync_active;
/* A phone with an empty PBAP database must not trigger a full re-sync each
 * time its recent/contact tab is opened.  This flag is reset on the next
 * HFP connection. */
static INT8U s_phone_sync_attempted;
static INT8U s_phone_contact_count;
static INT8U s_phone_call_count;
static char s_phone_dial[LVGL_PHONE_NUMBER_MAX + 1u];
static char s_phone_status[BT_UI_TEXT_MAX] = "等待手机连接";
static char s_phone_remote_number[BT_UI_TEXT_MAX];
static lvgl_phone_contact_t s_phone_contacts[LVGL_PHONE_CONTACT_MAX];
static lvgl_phone_call_t s_phone_calls[LVGL_PHONE_CALL_MAX];
static lv_obj_t *s_phone_status_label;
static lv_obj_t *s_phone_number_label;
static lv_obj_t *s_phone_tab_btn[LVGL_PHONE_TAB_COUNT];
static lv_obj_t *s_phone_panel[LVGL_PHONE_TAB_COUNT];
static lv_obj_t *s_phone_dial_btn[LVGL_PHONE_DIAL_KEY_COUNT];
static lv_obj_t *s_phone_recent_btn[LVGL_PHONE_VISIBLE_ROWS];
static lv_obj_t *s_phone_recent_label[LVGL_PHONE_VISIBLE_ROWS];
static lv_obj_t *s_phone_contact_btn[LVGL_PHONE_VISIBLE_ROWS];
static lv_obj_t *s_phone_contact_label[LVGL_PHONE_VISIBLE_ROWS];

/* 通话控件模型。事件在 LVGL 任务内更新，控件对象也只由 LVGL 任务创建/销毁。
   s_call_present / s_call_incoming / s_call_talking 会被外围任务（按键上下文）
   读取，故整体保持 volatile 单字节。 */
static volatile INT8U s_call_page_active;
static volatile INT8U s_call_present;
static volatile INT8U s_call_incoming;
static volatile INT8U s_call_talking;
static volatile INT8U s_call_muted;
static INT8U s_call_hfp_state;
static INT8U s_call_ringing;
static INT32U s_call_seconds;
static char s_call_number[LVGL_PHONE_NUMBER_MAX + 1u];
static char s_call_name[BT_UI_TEXT_MAX];
/* 通话控件挂在 lv_layer_top()：该层在每帧最后绘制，切页面也盖不住它。
   控件对象只由 LVGL 任务创建/销毁。 */
static lv_obj_t *s_call_panel;
static lv_obj_t *s_call_caller_label;
static lv_obj_t *s_call_timer_label;
static lv_obj_t *s_call_answer_btn;
static lv_timer_t *s_call_timer;

static void lvgl_phone_refresh(void);
static void lvgl_phone_activate(void);
static void lvgl_phone_copy_text(char *dst, const char *src, INT32U size);

/* U: is a read-only LVGL file-system drive backed by the raw U3 SPI-NOR
 * partition, or its DDR snapshot when preloading succeeds. Resource files
 * remain LVGL v8 binary images, e.g. "U:/001.bin". */
typedef struct {
    INT32U data_addr;
    INT32U size;
    INT32U position;
} lvgl_nand_file_t;

static lv_fs_drv_t s_nand_fs_drv;
static INT8U s_spui_ready;
#if (C_LVGL_UI_RAM_CACHE == CUSTOM_ON)
static INT8U *s_spui_ram;
static INT32U s_spui_ram_size;
#endif

typedef struct {
    INT32U magic;
    INT32U version;
    INT16U file_count;
    INT16U header_size;
    INT32U image_size;
    INT32U table_crc32;
} lvgl_spui_header_t;

typedef struct {
    CHAR name[LVGL_UI_INSTALL_NAME_MAX];
    INT32U offset;
    INT32U size;
    INT32U crc32;
} lvgl_spui_entry_t;

/* SD:/UI/manifest.bin format. It is produced on a PC after image conversion;
 * it contains no image payload, only integrity metadata for the .bin files. */
typedef struct {
    INT32U magic;
    INT32U version;
    INT16U file_count;
    INT16U header_size;
} lvgl_ui_manifest_header_t;

typedef struct {
    CHAR name[LVGL_UI_INSTALL_NAME_MAX];
    INT32U size;
    INT32U crc32;
} lvgl_ui_manifest_entry_t;

static INT8U s_ui_sd_mounted;

extern INT32S jpeg_buffer_decode_and_scale(
    IMAGE_DECODE_STRUCT *img_decode_struct,
    INT32U clip_xy,
    INT32U clip_size);
extern INT32S cache_drain_range(INT32U buffer, INT32U size);
extern INT32S SPIFC_Flash_read_page(INT32U addr, INT8U *buf, INT32U byte_len);
extern INT32S SPIFC_Flash_erase_block(INT32U addr);
extern INT32S SPIFC_Flash_write_page(INT32U addr, INT8U *buf);

#if (LVGL_UI_SPIFC_PROBE == 1)
/* U3 is also used by NVRAM/upgrade code. This probe performs only the JEDEC
 * ID and a 32-byte read at address zero; neither operation changes Flash. */
static void lvgl_spifc_probe(void)
{
    INT8U id[3] = {0};
    INT8U header[32] = {0};
    INT32S id_result;
    INT32S read_result;

    id_result = drv_l1_spifcflash_read_id(id);
    read_result = SPIFC_Flash_read_page(0u, header, sizeof(header));
    DBG_PRINT("[SPIFC probe] id ret=%d: %02x %02x %02x\r\n",
              id_result, id[0], id[1], id[2]);
    DBG_PRINT("[SPIFC probe] hdr ret=%d: %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
              read_result, header[0], header[1], header[2], header[3],
              header[4], header[5], header[6], header[7]);
    DBG_PRINT("[SPIFC probe] hdr[0x15]=%02x (size x64KiB), hdr[0x17]=%02x (offset x64KiB)\r\n",
              header[0x15], header[0x17]);
}
#endif

#if (LVGL_UI_SPIFC_SCAN == 1)
/* Determine only whether a physical erase sector is entirely 0xFF. A blank
 * range is a candidate, not an ownership decision: it is never erased or
 * programmed by this diagnostic. */
static void lvgl_spifc_scan_blank_region(void)
{
    INT8U page[LVGL_SPIFC_PAGE_BYTES];
    INT32U block;
    INT32U page_index;
    INT32U byte_index;
    INT32U run_start = 0u;
    INT32U run_blocks = 0u;
    INT32U best_start = 0u;
    INT32U best_blocks = 0u;
    INT32U read_errors = 0u;
    BOOLEAN blank;

    DBG_PRINT("[SPIFC scan] begin: 8MiB, 4KiB sectors, need 5MiB\r\n");
    for (block = 0u; block < (LVGL_SPIFC_BYTES / LVGL_SPIFC_ERASE_BYTES); block++) {
        blank = TRUE;
        for (page_index = 0u; page_index < (LVGL_SPIFC_ERASE_BYTES / LVGL_SPIFC_PAGE_BYTES);
             page_index++) {
            if (SPIFC_Flash_read_page(block * LVGL_SPIFC_ERASE_BYTES +
                                      page_index * LVGL_SPIFC_PAGE_BYTES,
                                      page, LVGL_SPIFC_PAGE_BYTES) != 0) {
                blank = FALSE;
                read_errors++;
                break;
            }
            for (byte_index = 0u; byte_index < LVGL_SPIFC_PAGE_BYTES; byte_index++) {
                if (page[byte_index] != 0xFFu) {
                    blank = FALSE;
                    break;
                }
            }
            if (!blank) {
                break;
            }
        }

        if (blank) {
            if (run_blocks == 0u) {
                run_start = block;
            }
            run_blocks++;
            if (run_blocks > best_blocks) {
                best_start = run_start;
                best_blocks = run_blocks;
            }
        } else {
            run_blocks = 0u;
        }

        /* Prevent a one-time diagnostic from monopolising the LVGL task. */
        if ((block & 0x07u) == 0x07u) {
            vTaskDelay(1);
        }
    }

    DBG_PRINT("[SPIFC scan] longest blank: 0x%08x..0x%08x (%d KiB), read errors=%d\r\n",
              best_start * LVGL_SPIFC_ERASE_BYTES,
              (best_start + best_blocks) * LVGL_SPIFC_ERASE_BYTES - 1u,
              best_blocks * (LVGL_SPIFC_ERASE_BYTES / 1024u), read_errors);
    DBG_PRINT("[SPIFC scan] candidate %s 5MiB at 0x%08x\r\n",
              (best_blocks * LVGL_SPIFC_ERASE_BYTES >= LVGL_SPIFC_UI_NEED_BYTES) ?
                  "HAS" : "LACKS",
              best_start * LVGL_SPIFC_ERASE_BYTES);
}
#endif

static BOOLEAN lvgl_spui_valid_header(const lvgl_spui_header_t *header)
{
    INT32U minimum;

    if ((header == NULL) || (header->magic != LVGL_SPUI_MAGIC) ||
        (header->header_size != sizeof(*header)) || (header->file_count == 0u) ||
        (header->file_count > LVGL_UI_INSTALL_MAX_FILES)) {
        return FALSE;
    }
    minimum = header->header_size + ((INT32U)header->file_count * sizeof(lvgl_spui_entry_t));
    return (header->image_size >= minimum) &&
           (header->image_size <= (LVGL_SPUI_LIMIT - LVGL_SPUI_BASE)) &&
           ((header->image_size & (LVGL_SPIFC_PAGE_BYTES - 1u)) == 0u);
}

static BOOLEAN lvgl_nand_fs_select(void)
{
    lvgl_spui_header_t header;

    if (s_spui_ready == 0u) {
        if (SPIFC_Flash_read_page(LVGL_SPUI_BASE, (INT8U *)&header,
                                  sizeof(header)) != 0) {
            return FALSE;
        }
        if (!lvgl_spui_valid_header(&header)) {
            return FALSE;
        }
        s_spui_ready = 1u;
    }
    return TRUE;
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

static INT32S lvgl_spui_read(INT32U addr, INT8U *dst, INT32U bytes);

static BOOLEAN lvgl_spui_find(const CHAR *name, lvgl_spui_entry_t *found)
{
    lvgl_spui_header_t header;
    lvgl_spui_entry_t entry;
    INT16U i;
    INT32U table_addr = LVGL_SPUI_BASE + sizeof(header);

    if (!lvgl_nand_fs_select() || (name == NULL) || (found == NULL) ||
        (lvgl_spui_read(LVGL_SPUI_BASE, (INT8U *)&header,
                        sizeof(header)) != 0) || !lvgl_spui_valid_header(&header)) {
        return FALSE;
    }
    for (i = 0u; i < header.file_count; i++) {
        if (lvgl_spui_read(table_addr + ((INT32U)i * sizeof(entry)),
                           (INT8U *)&entry, sizeof(entry)) != 0) {
            return FALSE;
        }
        if (strncmp(entry.name, name, LVGL_UI_INSTALL_NAME_MAX) == 0) {
            if ((entry.offset < (header.header_size +
                                 ((INT32U)header.file_count * sizeof(entry)))) ||
                (entry.offset > header.image_size) ||
                (entry.size > (header.image_size - entry.offset))) {
                return FALSE;
            }
            *found = entry;
            return TRUE;
        }
    }
    return FALSE;
}

static INT32S lvgl_spui_read(INT32U addr, INT8U *dst, INT32U bytes)
{
    INT8U page[LVGL_SPIFC_PAGE_BYTES];
    INT32U page_addr;
    INT32U page_offset;
    INT32U chunk;

    if ((addr < LVGL_SPUI_BASE) || (addr > LVGL_SPUI_LIMIT) ||
        (bytes > (LVGL_SPUI_LIMIT - addr)) ||
        ((dst == NULL) && (bytes != 0u))) {
        return -1;
    }
#if (C_LVGL_UI_RAM_CACHE == CUSTOM_ON)
    if (s_spui_ram != NULL) {
        INT32U offset = addr - LVGL_SPUI_BASE;

        if ((offset > s_spui_ram_size) ||
            (bytes > (s_spui_ram_size - offset))) {
            return -1;
        }
        if (bytes != 0u) {
            memcpy(dst, s_spui_ram + offset, bytes);
        }
        return 0;
    }
#endif
    while (bytes != 0u) {
        page_addr = addr & ~(LVGL_SPIFC_PAGE_BYTES - 1u);
        page_offset = addr - page_addr;
        chunk = LVGL_SPIFC_PAGE_BYTES - page_offset;
        if (chunk > bytes) {
            chunk = bytes;
        }
        if (SPIFC_Flash_read_page(page_addr, page, LVGL_SPIFC_PAGE_BYTES) != 0) {
            return -1;
        }
        memcpy(dst, page + page_offset, chunk);
        addr += chunk;
        dst += chunk;
        bytes -= chunk;
    }
    return 0;
}

#if (C_LVGL_UI_RAM_CACHE == CUSTOM_ON)
static void lvgl_spui_ram_cache_init(void)
{
    lvgl_spui_header_t header;
    INT8U *buffer;
    INT32U offset;
    INT32U chunk;
    TickType_t start_tick;

    if (s_spui_ram != NULL) {
        return;
    }
    if (!lvgl_nand_fs_select() ||
        (lvgl_spui_read(LVGL_SPUI_BASE, (INT8U *)&header, sizeof(header)) != 0) ||
        !lvgl_spui_valid_header(&header)) {
        ERR_PRINT("[U3 cache] pack unavailable; using Flash\r\n");
        return;
    }
    /* Use the SDK DDR allocator, not LVGL's 1MiB object heap. The pack
     * header bounds this allocation to the existing 5MiB UI partition. */
    buffer = (INT8U *)gp_malloc_align(header.image_size, 64);
    if (buffer == NULL) {
        ERR_PRINT("[U3 cache] alloc failed (%d bytes); using Flash\r\n",
                  header.image_size);
        return;
    }
    DBG_PRINT("[U3 cache] loading %d bytes from Flash\r\n", header.image_size);
    start_tick = xTaskGetTickCount();
    for (offset = 0u; offset < header.image_size; offset += chunk) {
        chunk = header.image_size - offset;
        if (chunk > (16u * 1024u)) {
            chunk = 16u * 1024u;
        }
        if (lvgl_spui_read(LVGL_SPUI_BASE + offset, buffer + offset, chunk) != 0) {
            gp_free(buffer);
            ERR_PRINT("[U3 cache] read failed at 0x%08x; using Flash\r\n",
                      LVGL_SPUI_BASE + offset);
            return;
        }
        vTaskDelay(1); /* Let Bluetooth/audio run during the one-time load. */
    }
    if (memcmp(buffer, &header, sizeof(header)) != 0) {
        gp_free(buffer);
        ERR_PRINT("[U3 cache] pack header changed; using Flash\r\n");
        return;
    }
    /* Publish only the complete snapshot. It lives for the LVGL task's
     * lifetime, so image handles remain valid across page switches. */
    s_spui_ram_size = header.image_size;
    s_spui_ram = buffer;
    DBG_PRINT("[U3 cache] ready: %d bytes, %d ms; U: reads DDR\r\n",
              s_spui_ram_size,
              (INT32U)((xTaskGetTickCount() - start_tick) * portTICK_PERIOD_MS));
}
#endif

static void *lvgl_nand_open_cb(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    CHAR nand_path[LVGL_NAND_PATH_MAX];
    lvgl_spui_entry_t entry;
    lvgl_nand_file_t *file;

    (void)drv;
    if ((mode != LV_FS_MODE_RD) || !lvgl_nand_fs_select() ||
        !lvgl_nand_path_copy(path, nand_path) ||
        !lvgl_spui_find(nand_path, &entry)) {
        return NULL;
    }

    file = lv_mem_alloc(sizeof(*file));
    if (file == NULL) {
        return NULL;
    }
    file->data_addr = LVGL_SPUI_BASE + entry.offset;
    file->size = entry.size;
    file->position = 0u;
    return file;
}

static lv_fs_res_t lvgl_nand_close_cb(lv_fs_drv_t *drv, void *file_p)
{
    lvgl_nand_file_t *file = (lvgl_nand_file_t *)file_p;

    (void)drv;
    if (file == NULL) {
        return LV_FS_RES_INV_PARAM;
    }
    lv_mem_free(file);
    return LV_FS_RES_OK;
}

static lv_fs_res_t lvgl_nand_read_cb(lv_fs_drv_t *drv, void *file_p,
                                     void *buf, uint32_t btr, uint32_t *br)
{
    lvgl_nand_file_t *file = (lvgl_nand_file_t *)file_p;
    INT32U bytes;

    (void)drv;
    if ((file == NULL) || (buf == NULL) || !lvgl_nand_fs_select()) {
        return LV_FS_RES_HW_ERR;
    }
    bytes = btr;
    if (bytes > (file->size - file->position)) {
        bytes = file->size - file->position;
    }
    if (lvgl_spui_read(file->data_addr + file->position, (INT8U *)buf, bytes) != 0) {
        return LV_FS_RES_FS_ERR;
    }
    file->position += bytes;
    if (br != NULL) {
        *br = bytes;
    }
    return LV_FS_RES_OK;
}

static lv_fs_res_t lvgl_nand_seek_cb(lv_fs_drv_t *drv, void *file_p,
                                     uint32_t pos, lv_fs_whence_t whence)
{
    lvgl_nand_file_t *file = (lvgl_nand_file_t *)file_p;
    INT32U new_position;

    (void)drv;
    if ((file == NULL) || !lvgl_nand_fs_select()) {
        return LV_FS_RES_HW_ERR;
    }
    if (whence == LV_FS_SEEK_SET) {
        new_position = pos;
    } else if (whence == LV_FS_SEEK_CUR) {
        new_position = file->position + pos;
    } else if (whence == LV_FS_SEEK_END) {
        new_position = file->size + pos;
    } else {
        return LV_FS_RES_INV_PARAM;
    }
    if (new_position > file->size) {
        return LV_FS_RES_FS_ERR;
    }
    file->position = new_position;
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

#if (LVGL_UI_SPIFC_INSTALLER == 1)
static BOOLEAN lvgl_spui_sd_select(void)
{
    INT32S result;

    if (s_ui_sd_mounted == 0u) {
        /* _devicemount() owns both SD1's controller initialisation and FAT
         * mount. Calling drvl2_sd_init() before it makes _devicemount()
         * initialise the live card a second time; the latest board log shows
         * that second initialisation stalls after the successful first one. */
        DBG_PRINT("[U3 install] mount/init FS%d\r\n", FS_SD1);
        result = _devicemount(FS_SD1);
        DBG_PRINT("[U3 install] SD mount result=%d\r\n", result);
        if (result < 0) {
            ERR_PRINT("[U3 install] SD mount failed (%d)\r\n", result);
            return FALSE;
        }
        s_ui_sd_mounted = 1u;
    }
    result = fs_changedisk(FS_SD1);
    DBG_PRINT("[U3 install] select FS%d result=%d\r\n", FS_SD1, result);
    return (result >= 0) ? TRUE : FALSE;
}

/* Header page is committed only after every later page has been read back
 * successfully. An interrupted update leaves U: unavailable rather than
 * exposing a partial asset set. */
static void lvgl_spui_install_from_sd(void)
{
    static CHAR pack_path[] = "\\UI\\u3_ui.bin";
    INT8U first_page[LVGL_SPIFC_PAGE_BYTES];
    INT8U page[LVGL_SPIFC_PAGE_BYTES];
    INT8U verify[LVGL_SPIFC_PAGE_BYTES];
    lvgl_spui_header_t header;
    INT16S fd;
    INT32S bytes;
    INT32U offset;
    INT32U erase_bytes;
    INT32U address;
    INT32S flash_result;
    BOOLEAN page_matches;

    if (!lvgl_spui_sd_select()) {
        return;
    }
    fd = open(pack_path, O_RDONLY);
    if (fd < 0) {
        DBG_PRINT("[U3 install] SD:/UI/u3_ui.bin not found\r\n");
        return;
    }
    bytes = fs_fread(fd, (INT32U)first_page, sizeof(first_page));
    if ((bytes != sizeof(first_page))) {
        ERR_PRINT("[U3 install] cannot read header page (%d)\r\n", bytes);
        close(fd);
        return;
    }
    memcpy(&header, first_page, sizeof(header));
    if (!lvgl_spui_valid_header(&header)) {
        ERR_PRINT("[U3 install] invalid pack header\r\n");
        close(fd);
        return;
    }
    erase_bytes = (header.image_size + LVGL_SPUI_ERASE_BYTES - 1u) &
                  ~(LVGL_SPUI_ERASE_BYTES - 1u);
    if (erase_bytes > (LVGL_SPUI_LIMIT - LVGL_SPUI_BASE)) {
        ERR_PRINT("[U3 install] pack exceeds reserved partition\r\n");
        close(fd);
        return;
    }

    DBG_PRINT("[U3 install] version %d, files %d, bytes %d\r\n",
              header.version, header.file_count, header.image_size);
    for (offset = 0u; offset < erase_bytes; offset += LVGL_SPUI_ERASE_BYTES) {
        /* The SDK locks every raw SPI-NOR operation.  Suspending scheduling
         * here additionally prevents another task from starting a separate
         * transaction while this installer erases one 64 KiB block. */
        vTaskSuspendAll();
        flash_result = SPIFC_Flash_erase_block(LVGL_SPUI_BASE + offset);
        xTaskResumeAll();
        if (flash_result != 0) {
            ERR_PRINT("[U3 install] erase failed at 0x%08x\r\n", LVGL_SPUI_BASE + offset);
            close(fd);
            return;
        }
        vTaskDelay(1);
    }

    for (offset = LVGL_SPIFC_PAGE_BYTES; offset < header.image_size;
         offset += LVGL_SPIFC_PAGE_BYTES) {
        bytes = fs_fread(fd, (INT32U)page, sizeof(page));
        if (bytes != sizeof(page)) {
            ERR_PRINT("[U3 install] SD read failed at 0x%08x\r\n", offset);
            close(fd);
            return;
        }
        address = LVGL_SPUI_BASE + offset;
        /* Keep write and its verification together: no task may access U3
         * between them, otherwise a concurrent page transaction can corrupt
         * the shared SPI controller state. */
        vTaskSuspendAll();
        flash_result = SPIFC_Flash_write_page(address, page);
        if (flash_result == 0) {
            flash_result = SPIFC_Flash_read_page(address, verify, sizeof(verify));
        }
        page_matches = (flash_result == 0) &&
                       (memcmp(page, verify, sizeof(page)) == 0);
        xTaskResumeAll();
        if (!page_matches) {
            ERR_PRINT("[U3 install] write/verify failed at 0x%08x\r\n", address);
            close(fd);
            return;
        }
        if ((offset & (LVGL_SPUI_ERASE_BYTES - 1u)) == 0u) {
            DBG_PRINT("[U3 install] %d/%d KiB\r\n", offset / 1024u,
                      header.image_size / 1024u);
            vTaskDelay(1);
        }
    }
    close(fd);

    vTaskSuspendAll();
    flash_result = SPIFC_Flash_write_page(LVGL_SPUI_BASE, first_page);
    if (flash_result == 0) {
        flash_result = SPIFC_Flash_read_page(LVGL_SPUI_BASE, verify, sizeof(verify));
    }
    page_matches = (flash_result == 0) &&
                   (memcmp(first_page, verify, sizeof(first_page)) == 0);
    xTaskResumeAll();
    if (!page_matches) {
        ERR_PRINT("[U3 install] header commit failed\r\n");
        return;
    }
    s_spui_ready = 0u;
    if (!lvgl_nand_fs_select()) {
        ERR_PRINT("[U3 install] committed pack validation failed\r\n");
        return;
    }
    DBG_PRINT("[U3 install] completed; set LVGL_UI_SPIFC_INSTALLER to 0\r\n");
}
#endif

/* The installer is deliberately compiled out of the normal application
 * image.  It is enabled only in a one-time installer build by setting both
 * LVGL_UI_NAND_INSTALLER and NAND_APP_WRITE_EN to 1 in gplib_cfg.h. */
#if (LVGL_UI_NAND_INSTALLER == 1) && (NAND_APP_WRITE_EN == 1)
static BOOLEAN lvgl_ui_select_disk(INT8U disk, INT8U *mounted)
{
    INT32S result;

    if (*mounted == 0u) {
        if (disk == FS_SD1) {
            /* C_STORAGE_DEVICE is disabled in this product, so the normal
             * storage-service task never performs its usual SD mount. Use
             * exactly that task's supported init + _devicemount sequence;
             * fs_mount(FS_SD1) blocks here on this SDK configuration. */
            DBG_PRINT("[UI install] initialise SD\r\n");
            FS_OS_LOCK();
            result = drvl2_sd_init(STORAGE_SD_DEV);
            FS_OS_UNLOCK();
            if (result < 0) {
                DBG_PRINT("[UI install] SD init failed (%d)\r\n", result);
                return FALSE;
            }
            result = _devicemount(FS_SD1);
        } else {
            /* _devicemount is the SDK's public VFS mount API. fs_mount()
             * rejects FS_NAND_APP with DE_INVLDACC on this platform. */
            result = _devicemount(disk);
        }
        if (result < 0) {
            DBG_PRINT("[UI install] FS%d mount failed (%d)\r\n", disk, result);
            return FALSE;
        }
        *mounted = 1u;
    }
    return (fs_changedisk(disk) >= 0) ? TRUE : FALSE;
}

/* A factory-fresh NAND_APP partition has no FAT boot sector and cannot be
 * mounted. Formatting is only compiled for a one-time installer build; the
 * normal application must keep LVGL_UI_NAND_FORMAT_IF_BLANK at 0. */
static BOOLEAN lvgl_ui_prepare_nand_app(void)
{
    INT16S result;

    if (lvgl_ui_select_disk(FS_NAND_APP, &s_nand_fs_mounted)) {
        return TRUE;
    }

#if (LVGL_UI_NAND_FORMAT_IF_BLANK == 1)
    DBG_PRINT("[UI install] FS%d blank/unmountable; format FAT32\r\n", FS_NAND_APP);
    result = _format(FS_NAND_APP, FAT32_Type);
    if (result < 0) {
        ERR_PRINT("[UI install] FS%d FAT32 format failed (%d)\r\n", FS_NAND_APP, result);
        return FALSE;
    }
    s_nand_fs_mounted = 0u;
    if (!lvgl_ui_select_disk(FS_NAND_APP, &s_nand_fs_mounted)) {
        ERR_PRINT("[UI install] FS%d mount failed after format\r\n", FS_NAND_APP);
        return FALSE;
    }
    DBG_PRINT("[UI install] FS%d FAT32 format completed\r\n", FS_NAND_APP);
    return TRUE;
#else
    return FALSE;
#endif
}

static BOOLEAN lvgl_ui_make_path(const CHAR *name, CHAR *path)
{
    INT32U index = 0u;
    INT32U name_index;

    if ((name == NULL) || (path == NULL) || (name[0] == '\0')) {
        return FALSE;
    }
    path[index++] = '\\';
    path[index++] = 'U';
    path[index++] = 'I';
    path[index++] = '\\';
    for (name_index = 0u; name_index < LVGL_UI_INSTALL_NAME_MAX; name_index++) {
        CHAR ch = name[name_index];

        if (ch == '\0') {
            break;
        }
        if (index >= (LVGL_UI_INSTALL_PATH_MAX - 1u)) {
            return FALSE;
        }

        /* UI files are flat ASCII filenames, e.g. 001.bin. */
        if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') ||
              (ch >= 'a' && ch <= 'z') || ch == '_' || ch == '-' || ch == '.')) {
            return FALSE;
        }
        path[index++] = ch;
    }
    if ((name_index == LVGL_UI_INSTALL_NAME_MAX) || (index == 4u) ||
        (path[4] == '.')) {
        return FALSE;
    }
    path[index] = '\0';
    return TRUE;
}

static INT32U lvgl_ui_crc32_update(INT32U crc, const INT8U *data, INT32U size)
{
    INT32U i;

    while (size-- != 0u) {
        crc ^= *data++;
        for (i = 0u; i < 8u; i++) {
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
        }
    }
    return crc;
}

static BOOLEAN lvgl_ui_crc_file(INT8U disk, INT8U *mounted, CHAR *path,
                                INT32U expected_size, INT32U expected_crc)
{
    INT8U *buffer;
    INT16S fd;
    INT32S read_bytes;
    INT32U total = 0u;
    INT32U crc = 0xFFFFFFFFu;

    if (!lvgl_ui_select_disk(disk, mounted)) {
        return FALSE;
    }
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return FALSE;
    }
    buffer = (INT8U *)gp_malloc_align(LVGL_UI_INSTALL_BUF_BYTES, 32);
    if (buffer == NULL) {
        close(fd);
        return FALSE;
    }
    do {
        read_bytes = fs_fread(fd, (INT32U)buffer, LVGL_UI_INSTALL_BUF_BYTES);
        if (read_bytes > 0) {
            total += (INT32U)read_bytes;
            crc = lvgl_ui_crc32_update(crc, buffer, (INT32U)read_bytes);
        }
    } while (read_bytes > 0);
    close(fd);
    gp_free(buffer);
    return (read_bytes == 0) && (total == expected_size) &&
           ((crc ^ 0xFFFFFFFFu) == expected_crc);
}

static BOOLEAN lvgl_ui_copy_file(CHAR *path, INT32U expected_size, INT32U expected_crc)
{
    INT8U *buffer;
    INT16S source_fd;
    INT16S target_fd;
    INT32S read_bytes;
    INT32S written_bytes;
    INT32U total = 0u;
    INT32U crc = 0xFFFFFFFFu;

    if (!lvgl_ui_select_disk(FS_SD1, &s_ui_sd_mounted)) {
        return FALSE;
    }
    source_fd = open(path, O_RDONLY);
    if (source_fd < 0) {
        return FALSE;
    }
    if (!lvgl_ui_select_disk(FS_NAND_APP, &s_nand_fs_mounted)) {
        close(source_fd);
        return FALSE;
    }
    target_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (target_fd < 0) {
        close(source_fd);
        return FALSE;
    }
    buffer = (INT8U *)gp_malloc_align(LVGL_UI_INSTALL_BUF_BYTES, 32);
    if (buffer == NULL) {
        close(target_fd);
        close(source_fd);
        return FALSE;
    }
    do {
        read_bytes = fs_fread(source_fd, (INT32U)buffer, LVGL_UI_INSTALL_BUF_BYTES);
        if (read_bytes > 0) {
            crc = lvgl_ui_crc32_update(crc, buffer, (INT32U)read_bytes);
            total += (INT32U)read_bytes;
            written_bytes = fs_fwrite(target_fd, (INT32U)buffer, (INT32U)read_bytes);
            if (written_bytes != read_bytes) {
                read_bytes = -1;
            }
        }
    } while (read_bytes > 0);
    close(target_fd);
    close(source_fd);
    gp_free(buffer);
    if ((read_bytes != 0) || (total != expected_size) ||
        ((crc ^ 0xFFFFFFFFu) != expected_crc)) {
        return FALSE;
    }
    return lvgl_ui_crc_file(FS_NAND_APP, &s_nand_fs_mounted, path,
                             expected_size, expected_crc);
}

static BOOLEAN lvgl_ui_read_manifest(INT8U disk, INT8U *mounted,
                                     lvgl_ui_manifest_header_t *header,
                                     lvgl_ui_manifest_entry_t **entries)
{
    CHAR manifest_path[] = "\\UI\\manifest.bin";
    INT16S fd;
    INT32S bytes;
    INT32U entries_size;

    *entries = NULL;
    if (!lvgl_ui_select_disk(disk, mounted)) {
        DBG_PRINT("[UI install] FS%d select failed\r\n", disk);
        return FALSE;
    }
    fd = open(manifest_path, O_RDONLY);
    if (fd < 0) {
        DBG_PRINT("[UI install] FS%d open %s failed (%d)\r\n", disk, manifest_path, fd);
        return FALSE;
    }
    bytes = fs_fread(fd, (INT32U)header, sizeof(*header));
    if ((bytes != sizeof(*header)) || (header->magic != LVGL_UI_MANIFEST_MAGIC) ||
        (header->header_size != sizeof(*header)) || (header->file_count == 0u) ||
        (header->file_count > LVGL_UI_INSTALL_MAX_FILES)) {
        DBG_PRINT("[UI install] FS%d invalid manifest bytes=%d magic=%x head=%d count=%d\r\n",
                  disk, bytes, header->magic, header->header_size, header->file_count);
        close(fd);
        return FALSE;
    }
    entries_size = (INT32U)header->file_count * sizeof(**entries);
    *entries = (lvgl_ui_manifest_entry_t *)gp_malloc_align(entries_size, 32);
    if (*entries == NULL) {
        close(fd);
        return FALSE;
    }
    bytes = fs_fread(fd, (INT32U)*entries, entries_size);
    close(fd);
    if (bytes != (INT32S)entries_size) {
        DBG_PRINT("[UI install] FS%d manifest entries short: %d/%d\r\n",
                  disk, bytes, entries_size);
        gp_free(*entries);
        *entries = NULL;
        return FALSE;
    }
    return TRUE;
}

static INT32U lvgl_ui_installed_version(void)
{
    lvgl_ui_manifest_header_t header;
    lvgl_ui_manifest_entry_t *entries;
    INT32U version = 0u;

    if (lvgl_ui_read_manifest(FS_NAND_APP, &s_nand_fs_mounted, &header, &entries)) {
        version = header.version;
        gp_free(entries);
    }
    return version;
}

static void lvgl_ui_nand_install_from_sd(void)
{
    lvgl_ui_manifest_header_t header;
    lvgl_ui_manifest_entry_t *entries;
    CHAR path[LVGL_UI_INSTALL_PATH_MAX];
    CHAR manifest_path[] = "\\UI\\manifest.bin";
    INT32U installed_version;
    INT32U manifest_size;
    INT32U manifest_crc;
    INT16U i;

    if (!lvgl_ui_read_manifest(FS_SD1, &s_ui_sd_mounted, &header, &entries)) {
        DBG_PRINT("[UI install] no valid SD:/UI/manifest.bin\r\n");
        return;
    }
    if (!lvgl_ui_prepare_nand_app()) {
        DBG_PRINT("[UI install] NAND app mount/provision failed\r\n");
        gp_free(entries);
        return;
    }
    installed_version = lvgl_ui_installed_version();
    if (header.version <= installed_version) {
        DBG_PRINT("[UI install] skip version %d (installed %d)\r\n",
                  header.version, installed_version);
        gp_free(entries);
        return;
    }
    if (!lvgl_ui_prepare_nand_app()) {
        DBG_PRINT("[UI install] NAND app mount failed\r\n");
        gp_free(entries);
        return;
    }
    /* This SDK exports fs_lfnmkdir(), but not the dir_exists() declaration
     * found in gplib_fs.h. Ignore the return here because an existing UI
     * directory is the normal update case; file open/write below is the
     * authoritative check that the directory is usable. */
    (void)fs_lfnmkdir("\\UI");
    DBG_PRINT("[UI install] version %d, files %d\r\n", header.version, header.file_count);
    for (i = 0u; i < header.file_count; i++) {
        if (!lvgl_ui_make_path(entries[i].name, path) ||
            !lvgl_ui_copy_file(path, entries[i].size, entries[i].crc32)) {
            ERR_PRINT("[UI install] failed: %s\r\n", entries[i].name);
            gp_free(entries);
            return;
        }
        DBG_PRINT("[UI install] %d/%d %s\r\n", i + 1u, header.file_count, entries[i].name);
    }
    manifest_size = sizeof(header) + ((INT32U)header.file_count * sizeof(*entries));
    /* The manifest is the commit record: write it only after every asset
     * has been copied and verified from NAND. */
    manifest_crc = lvgl_ui_crc32_update(0xFFFFFFFFu, (INT8U *)&header, sizeof(header));
    manifest_crc = lvgl_ui_crc32_update(manifest_crc, (INT8U *)entries,
                                        manifest_size - sizeof(header)) ^ 0xFFFFFFFFu;
    if (!lvgl_ui_copy_file(manifest_path, manifest_size, manifest_crc)) {
        ERR_PRINT("[UI install] failed to commit manifest\r\n");
        gp_free(entries);
        return;
    }
    DBG_PRINT("[UI install] completed; reboot into normal firmware\r\n");
    gp_free(entries);
}
#endif

/* xstack 回调只将事件复制进该短队列；LVGL 任务轮询并完成实际绘制。 */
static bt_ui_event_t s_bt_event_queue[LVGL_BT_EVENT_QUEUE_LEN];
static volatile INT8U s_bt_event_head;
static volatile INT8U s_bt_event_tail;

/* LVGL 任务启动门：main 在 wifi_ap_mode_enable() 返回后置 1。
   避免 LVGL 启动期大块 DDR 写入/cache 刷回/首帧渲染与 atbm SDIO init 重叠干扰
   （DEBUG 实验，稳定后可按需保留/移除） */
/* Set by main.c after WiFi/BT start-up.  This is read from the LVGL task,
 * so it must remain volatile: without it an optimised build may keep the
 * zero value in a register and wait here forever. */
volatile INT32U g_lvgl_start_ok = 0;

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

#if (LVGL_FLUSH_TRACE_EN == 1)
    /* DEBUG: 前几次 + 每 10 片打印一次（整帧=10 片时 flush# 10 的整数倍即帧边界，
       复位时能看出渲染推进到第几片/第几帧） */
    {
        static INT32U s_flush_cnt = 0;
        if ((s_flush_cnt < 3) || ((s_flush_cnt % 10) == 0)) {
            LVGL_RUNTIME_LOG("[LVGL] flush#%u area=(%d,%d)-(%d,%d) w=%u h=%u\r\n",
                             s_flush_cnt, area->x1, area->y1, area->x2, area->y2,
                             area_w, area_h);
        }
        s_flush_cnt++;
    }
#endif

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
    cache_drain_range(s_lvgl_fb + area->y1 * LVGL_H_RES * sizeof(lv_color_t),
                      area_h * LVGL_H_RES * sizeof(lv_color_t));

    if (s_cover_flush_pending && (s_active_page == LVGL_PAGE_MUSIC) &&
        (s_ui_cover != NULL)) {
        lv_area_t cover_area;

        lv_obj_get_coords(s_ui_cover, &cover_area);
        if ((area->x1 <= cover_area.x2) && (area->x2 >= cover_area.x1) &&
            (area->y1 <= cover_area.y2) && (area->y2 >= cover_area.y1)) {
            s_cover_flush_pending = 0u;
            /* Framebuffer commit, not the panel's later scan-out. */
            LVGL_RUNTIME_LOG("[cover timing] gen=%u fb_ms=%u\r\n",
                             (unsigned int)s_cover_submit_generation,
                             (unsigned int)((xTaskGetTickCount() -
                                             s_cover_submit_tick) * portTICK_PERIOD_MS));
        }
    }
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
static bt_xstack_result_t lvgl_bt_music_play_pause_request(void)
{
    INT8U current_playing;
    INT8U target_playing;
    TickType_t request_tick;
    uint32_t request_sequence;
    bt_xstack_result_t status;

    taskENTER_CRITICAL();
    current_playing = s_play_command_pending ?
        s_play_command_target : s_playing;
    target_playing = current_playing ? 0u : 1u;
    request_tick = xTaskGetTickCount();
    request_sequence = ++s_play_command_sequence;
    s_play_request_event_sequence = s_play_event_sequence;
    s_play_command_guard = 1u;
    s_play_command_target = target_playing;
    s_play_command_tick = request_tick;
    s_play_command_pending = 1u;
    taskEXIT_CRITICAL();

    status = current_playing ? bt_xstack_avrcp_pause() : bt_xstack_avrcp_play();
    if (status != BT_XSTACK_OK) {
        INT8U rollback_playing = 0u;

        taskENTER_CRITICAL();
        if (s_play_command_pending &&
            (s_play_command_sequence == request_sequence)) {
            s_play_command_pending = 0u;
            s_play_command_guard = 0u;
            s_playing = s_play_confirmed;
            rollback_playing = s_play_confirmed;
        }
        taskEXIT_CRITICAL();
        bt_ui_post_simple(BT_UI_EVENT_PLAY_REQUESTED,
            rollback_playing, request_sequence, 0u);
        return status;
    }

    /* The UI task applies this target immediately. Remote PLAY_STATE events
     * confirm it later; the request sequence rejects stale local updates. */
    bt_ui_post_simple(BT_UI_EVENT_PLAY_REQUESTED,
        target_playing, request_sequence, 0u);
    return status;
}

static void lvgl_bt_music_expire_pending_state(void)
{
    TickType_t now = xTaskGetTickCount();
    TickType_t timeout_ticks =
        (TickType_t)(LVGL_PLAY_STATE_CONFIRM_TIMEOUT_MS / portTICK_PERIOD_MS);
    INT8U expired = 0u;

    taskENTER_CRITICAL();
    if (s_play_command_pending &&
        ((now - s_play_command_tick) >= timeout_ticks)) {
        s_play_command_pending = 0u;
        s_play_command_guard = 0u;
        s_playing = s_play_confirmed;
        expired = 1u;
    }
    taskEXIT_CRITICAL();

    if (expired && (s_btn_play != NULL)) {
        lv_label_set_text(lv_obj_get_child(s_btn_play, 0),
                          s_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    }
}

static void lvgl_bt_music_control(lvgl_music_key_t key)
{
    bt_xstack_result_t status;

    if (key == LVGL_MUSIC_KEY_PLAY_PAUSE) {
        status = lvgl_bt_music_play_pause_request();
        LVGL_RUNTIME_LOG("[LVGL] AVRCP play/pause status=%d\r\n", status);
    } else if (key == LVGL_MUSIC_KEY_PREVIOUS) {
        status = bt_xstack_avrcp_previous();
        LVGL_RUNTIME_LOG("[LVGL] AVRCP previous status=%d\r\n", status);
    } else {
        status = bt_xstack_avrcp_next();
        LVGL_RUNTIME_LOG("[LVGL] AVRCP next status=%d\r\n", status);
    }
}

static void lvgl_bt_music_btn_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);

    if (btn == s_btn_play) {
        lvgl_bt_music_control(LVGL_MUSIC_KEY_PLAY_PAUSE);
    } else if (btn == s_btn_prev) {
        lvgl_bt_music_control(LVGL_MUSIC_KEY_PREVIOUS);
    } else if (btn == s_btn_next) {
        lvgl_bt_music_control(LVGL_MUSIC_KEY_NEXT);
    }
}

static void lvgl_bt_cover_receive_event(const bt_ui_event_t *event)
{
    uint32_t received_size = 0u;
    uint8_t *jpeg_buffer;
    uint8_t cover_complete = 0u;

    if (event->type == BT_UI_EVENT_COVER_ART_ERROR) {
        taskENTER_CRITICAL();
        if ((!s_cover_generation_valid) || (event->value_count < 2u) ||
            (event->values[1] == s_cover_generation)) {
            s_cover_receiving = 0u;
            s_cover_ready = 0u;
        }
        taskEXIT_CRITICAL();
        return;
    }
    if (event->type == BT_UI_EVENT_COVER_ART_DATA) {
        if ((event->data == NULL) || (event->data_len == 0u) ||
            (s_cover_jpeg == NULL)) {
            return;
        }

        taskENTER_CRITICAL();
        if (event->data_offset == 0u) {
            s_cover_jpeg_size = 0u;
            s_cover_ready = 0u;
            s_cover_receiving = 1u;
            s_cover_generation = event->values[0];
            s_cover_generation_valid = (event->value_count > 0u) ? 1u : 0u;
            /* Lyrics may update TRACK_INFO many times per second.  Only the
             * first byte of a new Cover Art transfer is authoritative for
             * changing the displayed cover. */
            /* This callback runs in the Bluetooth task.  It only copies
             * bytes: all LVGL object updates stay in the LVGL task, avoiding
             * concurrent redraws while a new cover is received.  A cached
             * cover remains on screen until a whole replacement decodes. */
        }
        received_size = s_cover_jpeg_size;
        if (s_cover_generation_valid &&
            ((event->value_count == 0u) ||
             (event->values[0] != s_cover_generation))) {
            taskEXIT_CRITICAL();
            return;
        }
        if ((!s_cover_receiving) ||
            (event->data_offset != received_size) ||
            (event->data_len > (LVGL_COVER_JPEG_MAX_BYTES - received_size))) {
            s_cover_receiving = 0u;
            taskEXIT_CRITICAL();
            return;
        }
        jpeg_buffer = s_cover_jpeg;
        s_cover_jpeg_size = received_size + event->data_len;
        taskEXIT_CRITICAL();
        memcpy(jpeg_buffer + received_size, event->data, event->data_len);
        return;
    }
    if (event->type == BT_UI_EVENT_COVER_ART_DONE) {
        taskENTER_CRITICAL();
        if (s_cover_receiving &&
            ((!s_cover_generation_valid) ||
             ((event->value_count > 0u) &&
              (event->values[0] == s_cover_generation))) &&
            (event->data_len == s_cover_jpeg_size)) {
            s_cover_receiving = 0u;
            received_size = s_cover_jpeg_size;
            s_cover_ready_tick = xTaskGetTickCount();
            s_cover_ready = 1u;
            cover_complete = 1u;
        }
        taskEXIT_CRITICAL();
        if (!cover_complete) {
            return;
        }
        LVGL_RUNTIME_LOG("[LVGL] cover received %u bytes\r\n",
                         (unsigned int)received_size);
    }
}

static void lvgl_bt_cover_decode_if_ready(void)
{
    IMAGE_DECODE_STRUCT image;
    uint32_t jpeg_size;
    uint8_t *decode_jpeg;
    uint32_t generation;
    TickType_t ready_tick;
    TickType_t decode_start_tick;
    TickType_t decode_end_tick;

    if ((s_active_page != LVGL_PAGE_MUSIC) || (s_cover_jpeg == NULL) ||
        (s_cover_decode_jpeg == NULL) ||
        (s_cover_rgb565 == NULL) || (s_ui_cover == NULL)) {
        return;
    }

    taskENTER_CRITICAL();
    if ((!s_cover_ready) || s_cover_decoding) {
        taskEXIT_CRITICAL();
        return;
    }
    jpeg_size = s_cover_jpeg_size;
    generation = s_cover_generation;
    ready_tick = s_cover_ready_tick;
    s_cover_ready = 0u;
    s_cover_decoding = 1u;
    decode_jpeg = s_cover_jpeg;
    s_cover_jpeg = s_cover_decode_jpeg;
    s_cover_decode_jpeg = decode_jpeg;
    s_cover_jpeg_size = 0u;
    taskEXIT_CRITICAL();
    if (jpeg_size == 0u) {
        s_cover_decoding = 0u;
        return;
    }

    memset(&image, 0, sizeof(image));
    image.image_source = (INT32S)(uintptr_t)decode_jpeg;
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

    decode_start_tick = xTaskGetTickCount();
    cache_drain_range((INT32U)decode_jpeg, jpeg_size);
    if (jpeg_buffer_decode_and_scale(&image, 0u, 0u) != STATUS_OK) {
        ERR_PRINT("[LVGL] cover JPEG decode failed gen=%u elapsed_ms=%u\r\n",
                  (unsigned int)generation,
                  (unsigned int)((xTaskGetTickCount() - decode_start_tick) *
                                  portTICK_PERIOD_MS));
        s_cover_decoding = 0u;
        return;
    }
    cache_invalid_range((INT32U)s_cover_rgb565,
                        LVGL_COVER_SIZE * LVGL_COVER_SIZE * sizeof(lv_color_t));
    decode_end_tick = xTaskGetTickCount();
    s_cover_img.header.cf = LV_IMG_CF_TRUE_COLOR;
    s_cover_img.header.w = LVGL_COVER_SIZE;
    s_cover_img.header.h = LVGL_COVER_SIZE;
    s_cover_img.data_size = LVGL_COVER_SIZE * LVGL_COVER_SIZE * sizeof(lv_color_t);
    s_cover_img.data = (const uint8_t *)s_cover_rgb565;
    lv_img_set_src(s_ui_cover, &s_cover_img);
    /* lv_img_set_src adopts the source's real dimensions. Re-apply the
     * display dimensions and centre after that call, otherwise a larger JPEG
     * source expands toward the lower right of the frame. */
    lv_img_set_zoom(s_ui_cover, LVGL_COVER_ZOOM);
    lv_img_set_pivot(s_ui_cover, LVGL_COVER_SIZE / 2u,
                     LVGL_COVER_SIZE / 2u);
    lv_obj_set_size(s_ui_cover, LVGL_COVER_SIZE, LVGL_COVER_SIZE);
    lv_obj_center(s_ui_cover);
    lv_obj_clear_flag(s_ui_cover, LV_OBJ_FLAG_HIDDEN);
    if (s_ui_cover_loading != NULL) {
        lv_obj_add_flag(s_ui_cover_loading, LV_OBJ_FLAG_HIDDEN);
    }
    s_cover_cached = 1u;
    s_cover_submit_tick = xTaskGetTickCount();
    s_cover_submit_generation = generation;
    s_cover_flush_pending = 1u;
    LVGL_RUNTIME_LOG("[cover timing] gen=%u bytes=%u wait_ms=%u decode_ms=%u submitted\r\n",
                     (unsigned int)generation, (unsigned int)jpeg_size,
                     (unsigned int)((decode_start_tick - ready_tick) * portTICK_PERIOD_MS),
                     (unsigned int)((decode_end_tick - decode_start_tick) * portTICK_PERIOD_MS));
    s_cover_decoding = 0u;
}

/* One opaque background comes from U3.  The four small dynamic assets are
 * added explicitly below and retain the original PC simulator coordinates. */
static const lvgl_dashboard_asset_t s_dashboard_static_assets[] = {
    { "U:/dashboard.bin", 0, 0 }
};

static void lvgl_dashboard_background_set(const char *file)
{
    if ((s_dashboard_background != NULL) && (file != NULL)) {
        lv_img_set_src(s_dashboard_background, file);
        lv_obj_invalidate(s_dashboard_background);
    }
}

static lv_obj_t *lvgl_dashboard_add_image(lv_obj_t *parent, const void *source,
                                          lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *image = lv_img_create(parent);

    lv_img_set_src(image, source);
    lv_obj_set_pos(image, x, y);
    return image;
}

static void lvgl_dashboard_memory_image_init(lv_img_dsc_t *image,
                                             const unsigned short *data,
                                             INT16U width, INT16U height,
                                             INT32U size)
{
    image->header.always_zero = 0u;
    image->header.cf = LV_IMG_CF_TRUE_COLOR;
    image->header.w = width;
    image->header.h = height;
    image->data_size = size;
    image->data = (const uint8_t *)data;
}

static void lvgl_dashboard_memory_images_init(void)
{
    lvgl_dashboard_memory_image_init(&s_dashboard_headlight_off_img,
                                     lvgl_dashboard_headlight_off_map,
                                     LVGL_DASHBOARD_HEADLIGHT_OFF_W,
                                     LVGL_DASHBOARD_HEADLIGHT_OFF_H,
                                     sizeof(lvgl_dashboard_headlight_off_map));
    lvgl_dashboard_memory_image_init(&s_dashboard_headlight_on_img,
                                     lvgl_dashboard_headlight_on_map,
                                     LVGL_DASHBOARD_HEADLIGHT_ON_W,
                                     LVGL_DASHBOARD_HEADLIGHT_ON_H,
                                     sizeof(lvgl_dashboard_headlight_on_map));
    lvgl_dashboard_memory_image_init(&s_dashboard_voice_idle_img,
                                     lvgl_dashboard_voice_idle_map,
                                     LVGL_DASHBOARD_VOICE_IDLE_W,
                                     LVGL_DASHBOARD_VOICE_IDLE_H,
                                     sizeof(lvgl_dashboard_voice_idle_map));
    lvgl_dashboard_memory_image_init(&s_dashboard_voice_active_img,
                                     lvgl_dashboard_voice_active_map,
                                     LVGL_DASHBOARD_VOICE_ACTIVE_W,
                                     LVGL_DASHBOARD_VOICE_ACTIVE_H,
                                     sizeof(lvgl_dashboard_voice_active_map));
}

static const INT8U s_dashboard_voice_wave_table[16] = {
    2u, 5u, 9u, 13u, 15u, 13u, 9u, 5u,
    3u, 6u, 10u, 13u, 10u, 7u, 4u, 2u
};

static lv_obj_t *lvgl_dashboard_add_voice_wave(lv_obj_t *parent, lv_coord_t x)
{
    lv_obj_t *bar = lv_obj_create(parent);

    lv_obj_set_style_bg_color(bar, lv_color_hex(0x19FA29), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, LVGL_DASHBOARD_VOICE_WAVE_BAR_W / 2u, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(bar, LVGL_DASHBOARD_VOICE_WAVE_BAR_W,
                    LVGL_DASHBOARD_VOICE_WAVE_MIN_H);
    lv_obj_set_pos(bar, x, LVGL_DASHBOARD_VOICE_WAVE_CENTER_Y -
                   LVGL_DASHBOARD_VOICE_WAVE_MIN_H / 2u);
    return bar;
}

static void lvgl_dashboard_tick(lv_timer_t *timer)
{
#if (LVGL_DASHBOARD_DEMO_ANIMATION == 0)
    (void)timer;
    return;
#else
    INT32S displayed_speed;
    INT8U i;

    (void)timer;
    if (!s_dashboard_active || (s_active_page != LVGL_PAGE_DASHBOARD)) {
        return;
    }
    s_dashboard_speed_tenths += LVGL_DASHBOARD_SPEED_STEP_TENTHS *
                                s_dashboard_speed_direction;
    if (s_dashboard_speed_tenths >= 1440) {
        s_dashboard_speed_direction = -1;
    }
    if (s_dashboard_speed_tenths <= 680) {
        s_dashboard_speed_direction = 1;
    }

    lv_arc_set_value(s_dashboard_main_arc, s_dashboard_speed_tenths);
    displayed_speed = s_dashboard_speed_tenths / 10;
    lv_label_set_text_fmt(s_dashboard_speed_label, "%d", displayed_speed);

#if LVGL_DASHBOARD_HEADLIGHT_SIM_BLINK
    s_dashboard_headlight_elapsed =
        (INT16U)(s_dashboard_headlight_elapsed + LVGL_DASHBOARD_TICK_MS);
    if (s_dashboard_headlight_elapsed >= 1000u) {
        s_dashboard_headlight_elapsed = 0u;
        s_dashboard_headlight_is_on = (INT8U)!s_dashboard_headlight_is_on;
        lv_img_set_src(s_dashboard_headlight,
                       s_dashboard_headlight_is_on ? &s_dashboard_headlight_on_img :
                                                     &s_dashboard_headlight_off_img);
    }
#endif

#if LVGL_DASHBOARD_VOICE_SIM_BLINK
    s_dashboard_voice_elapsed =
        (INT16U)(s_dashboard_voice_elapsed + LVGL_DASHBOARD_TICK_MS);
    if (s_dashboard_voice_elapsed >= 1000u) {
        s_dashboard_voice_elapsed = 0u;
        s_dashboard_voice_is_active = (INT8U)!s_dashboard_voice_is_active;
        lv_img_set_src(s_dashboard_voice,
                       s_dashboard_voice_is_active ? &s_dashboard_voice_active_img :
                                                     &s_dashboard_voice_idle_img);
    }
#endif

    s_dashboard_voice_wave_phase = (INT16U)(s_dashboard_voice_wave_phase + 1u);
    for (i = 0u; i < LVGL_DASHBOARD_VOICE_WAVE_BARS * 2u; i++) {
        lv_obj_t *bar = s_dashboard_voice_wave[i];
        INT8U amplitude;
        lv_coord_t height;

        if (!s_dashboard_voice_is_active) {
            lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_HIDDEN);
        amplitude = s_dashboard_voice_wave_table[
            (s_dashboard_voice_wave_phase + i * 3u) % 16u];
        height = (lv_coord_t)(LVGL_DASHBOARD_VOICE_WAVE_MIN_H +
                 (amplitude * (LVGL_DASHBOARD_VOICE_WAVE_MAX_H -
                 LVGL_DASHBOARD_VOICE_WAVE_MIN_H)) / 15u);
        lv_obj_set_size(bar, LVGL_DASHBOARD_VOICE_WAVE_BAR_W, height);
        lv_obj_set_y(bar, LVGL_DASHBOARD_VOICE_WAVE_CENTER_Y - height / 2u);
    }

#endif
}

static void lvgl_dashboard_content_set_visible(INT8U visible)
{
    INT32U i;

    if (s_dashboard_main_arc != NULL) {
        if (visible) lv_obj_clear_flag(s_dashboard_main_arc, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(s_dashboard_main_arc, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_dashboard_speed_label != NULL) {
        if (visible) lv_obj_clear_flag(s_dashboard_speed_label, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(s_dashboard_speed_label, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_dashboard_headlight != NULL) {
        if (visible) lv_obj_clear_flag(s_dashboard_headlight, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(s_dashboard_headlight, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_dashboard_voice != NULL) {
        if (visible) lv_obj_clear_flag(s_dashboard_voice, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(s_dashboard_voice, LV_OBJ_FLAG_HIDDEN);
    }
    for (i = 0u; i < LVGL_DASHBOARD_VOICE_WAVE_BARS * 2u; i++) {
        if (s_dashboard_voice_wave[i] != NULL) {
            lv_obj_add_flag(s_dashboard_voice_wave[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static lv_obj_t *lvgl_content_frame_create(void)
{
    lv_obj_t *frame = lv_obj_create(lv_scr_act());

    /* The shell already contains the complete 009_plane.  This object is
     * only an uncluttered drawing coordinate system on that plane, never a
     * card, dialog, border or shadow of its own. */
    lv_obj_set_pos(frame, LVGL_CONTENT_X, LVGL_CONTENT_Y);
    lv_obj_set_size(frame, LVGL_CONTENT_W, LVGL_CONTENT_H);
    lv_obj_set_style_bg_opa(frame, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(frame, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(frame, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(frame, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(frame, 0, LV_PART_MAIN);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return frame;
}

static void lvgl_nav_marker_refresh(void)
{
    static const lv_coord_t nav_center_x[LVGL_PAGE_COUNT] = {
        LVGL_DASHBOARD_MAP_X(519u), /* 032_car */
        LVGL_DASHBOARD_MAP_X(427u), /* 033_location */
        LVGL_DASHBOARD_MAP_X(610u), /* 034_call */
        LVGL_DASHBOARD_MAP_X(565u), /* 036_music */
        LVGL_DASHBOARD_MAP_X(473u)  /* 037_speed */
    };

    if ((s_nav_marker == NULL) || (s_nav_focus >= LVGL_PAGE_COUNT)) return;
    lv_obj_set_x(s_nav_marker, nav_center_x[s_nav_focus] - 18);
    lv_obj_set_style_bg_color(s_nav_marker, lv_color_hex(0x18DDE7), LV_PART_MAIN);
}

static void lvgl_dashboard_ui_create(lv_obj_t *screen)
{
    INT32U i;

    if (s_dashboard_timer != NULL) {
        lv_timer_del(s_dashboard_timer);
        s_dashboard_timer = NULL;
    }
    s_dashboard_active = 1u;
    s_active_page = LVGL_PAGE_DASHBOARD;
    s_phone_active = 0u;
    s_dashboard_headlight_elapsed = 0u;
    s_dashboard_voice_elapsed = 0u;
    s_dashboard_voice_wave_phase = 0u;
    s_dashboard_headlight_is_on = 0u;
    s_dashboard_voice_is_active = 1u;
    lvgl_dashboard_memory_images_init();
    lv_group_remove_all_objs(s_group);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000811), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    for (i = 0u; i < (sizeof(s_dashboard_static_assets) /
                       sizeof(s_dashboard_static_assets[0])); i++) {
        s_dashboard_background = lvgl_dashboard_add_image(screen,
                                                           s_dashboard_static_assets[i].file,
                                                           s_dashboard_static_assets[i].x,
                                                           s_dashboard_static_assets[i].y);
    }
    s_nav_marker = lv_obj_create(screen);
    lv_obj_set_size(s_nav_marker, 36, 3);
    lv_obj_set_y(s_nav_marker, LVGL_DASHBOARD_MAP_Y(492u));
    lv_obj_set_style_border_width(s_nav_marker, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_nav_marker, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_nav_marker, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_nav_marker, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lvgl_nav_marker_refresh();

    s_dashboard_main_arc = lv_arc_create(screen);
    lv_obj_set_size(s_dashboard_main_arc, LVGL_DASHBOARD_MAP_W(282u),
                    LVGL_DASHBOARD_MAP_H(282u));
    lv_obj_set_pos(s_dashboard_main_arc, LVGL_DASHBOARD_MAP_X(370u),
                   LVGL_DASHBOARD_MAP_Y(161u));
    lv_obj_set_style_bg_opa(s_dashboard_main_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_dashboard_main_arc, LVGL_DASHBOARD_MAP_W(14u), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_dashboard_main_arc, lv_color_hex(0x1A3241), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_dashboard_main_arc, LVGL_DASHBOARD_MAP_W(14u), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_dashboard_main_arc, lv_color_hex(0x10DDE5), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(s_dashboard_main_arc, true, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_dashboard_main_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_dashboard_main_arc, 0, LV_PART_MAIN);
    lv_arc_set_range(s_dashboard_main_arc, 0, 1600);
    lv_arc_set_rotation(s_dashboard_main_arc, 135);
    lv_arc_set_bg_angles(s_dashboard_main_arc, 0, 270);
    lv_arc_set_value(s_dashboard_main_arc, s_dashboard_speed_tenths);
    lv_obj_clear_flag(s_dashboard_main_arc, LV_OBJ_FLAG_CLICKABLE);

    s_dashboard_speed_label = lv_label_create(screen);
    lv_label_set_text(s_dashboard_speed_label, "108");
    lv_obj_set_style_text_font(s_dashboard_speed_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_dashboard_speed_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(s_dashboard_speed_label, LVGL_DASHBOARD_MAP_X(466u),
                   LVGL_DASHBOARD_MAP_Y(265u));

    s_dashboard_headlight = lvgl_dashboard_add_image(screen,
                                                      &s_dashboard_headlight_off_img,
                                                      LVGL_DASHBOARD_HEADLIGHT_X,
                                                      LVGL_DASHBOARD_HEADLIGHT_Y);
    s_dashboard_voice = lvgl_dashboard_add_image(screen,
                                                 &s_dashboard_voice_active_img,
                                                 LVGL_DASHBOARD_VOICE_X,
                                                 LVGL_DASHBOARD_VOICE_Y);
    for (i = 0u; i < LVGL_DASHBOARD_VOICE_WAVE_BARS; i++) {
        lv_coord_t step = LVGL_DASHBOARD_VOICE_WAVE_BAR_W +
                          LVGL_DASHBOARD_VOICE_WAVE_GAP;
        lv_coord_t left_x = LVGL_DASHBOARD_VOICE_WAVE_CENTER_X -
                            LVGL_DASHBOARD_VOICE_WAVE_ICON_HALF -
                            LVGL_DASHBOARD_VOICE_WAVE_GAP -
                            LVGL_DASHBOARD_VOICE_WAVE_BAR_W - (lv_coord_t)i * step;
        lv_coord_t right_x = LVGL_DASHBOARD_VOICE_WAVE_CENTER_X +
                             LVGL_DASHBOARD_VOICE_WAVE_ICON_HALF +
                             LVGL_DASHBOARD_VOICE_WAVE_GAP + (lv_coord_t)i * step;
        s_dashboard_voice_wave[i] = lvgl_dashboard_add_voice_wave(screen, left_x);
        s_dashboard_voice_wave[LVGL_DASHBOARD_VOICE_WAVE_BARS + i] =
            lvgl_dashboard_add_voice_wave(screen, right_x);
        lv_obj_add_flag(s_dashboard_voice_wave[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_dashboard_voice_wave[LVGL_DASHBOARD_VOICE_WAVE_BARS + i],
                        LV_OBJ_FLAG_HIDDEN);
    }
    s_dashboard_timer = lv_timer_create(lvgl_dashboard_tick,
                                        LVGL_DASHBOARD_TICK_MS, NULL);
    LVGL_RUNTIME_LOG("[LVGL] dynamic layout H=(%u,%u) V=(%u,%u)\r\n",
                     (INT32U)LVGL_DASHBOARD_HEADLIGHT_X,
                     (INT32U)LVGL_DASHBOARD_HEADLIGHT_Y,
                     (INT32U)LVGL_DASHBOARD_VOICE_X,
                     (INT32U)LVGL_DASHBOARD_VOICE_Y);
    LVGL_RUNTIME_LOG("[LVGL] dynamic source=C arrays, alpha boosted, flush cache=drain\r\n");
    LVGL_RUNTIME_LOG("[LVGL] dashboard: %d flattened U3 image + 4 dynamic C images\r\n",
                     (INT32U)(sizeof(s_dashboard_static_assets) /
                              sizeof(s_dashboard_static_assets[0])));
}

static void lvgl_bt_music_ui_create(lv_obj_t *scr)
{
    lv_obj_t *lbl;
    lv_obj_t *cover_frame;

    /* The player is a flat instrument-panel view inside the fixed frame. */
    lv_group_remove_all_objs(s_group);

    cover_frame = lv_obj_create(scr);
    lv_obj_set_size(cover_frame, 170, 170);
    lv_obj_set_style_bg_color(cover_frame, lv_color_hex(0x0B2632), 0);
    lv_obj_set_style_bg_opa(cover_frame, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(cover_frame, lv_color_hex(0x1BB8C9), 0);
    lv_obj_set_style_border_width(cover_frame, 2, 0);
    lv_obj_set_style_radius(cover_frame, 12, 0);
    lv_obj_set_style_shadow_color(cover_frame, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_width(cover_frame, 10, 0);
    lv_obj_set_style_shadow_opa(cover_frame, LV_OPA_70, 0);
    lv_obj_clear_flag(cover_frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(cover_frame, 96, 62);

    lbl = lv_label_create(scr);
    lv_label_set_text(lbl, LV_SYMBOL_AUDIO "  BLUETOOTH MUSIC");
    lv_obj_set_style_text_font(lbl, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x1ADBE5), 0);
    lv_obj_set_pos(lbl, 50, 20);

    /* Cover and metadata share one horizontal information row. */
    s_ui_title = lv_label_create(scr);
    lv_label_set_text(s_ui_title, s_music_title);
    lv_obj_set_style_text_font(s_ui_title, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(s_ui_title, lv_color_hex(0xE7FBFF), 0);
    lv_obj_set_width(s_ui_title, 560);
    lv_label_set_long_mode(s_ui_title, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(s_ui_title, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_pos(s_ui_title, 350, 112);

    s_ui_artist = lv_label_create(scr);
    lv_label_set_text(s_ui_artist, s_music_artist);
    lv_obj_set_style_text_font(s_ui_artist, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(s_ui_artist, lv_color_hex(0x82ACB6), 0);
    lv_obj_set_width(s_ui_artist, 560);
    lv_label_set_long_mode(s_ui_artist, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(s_ui_artist, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_pos(s_ui_artist, 350, 150);

    lbl = lv_label_create(scr);
    lv_label_set_text(lbl, "CONNECTED DEVICE");
    lv_obj_set_style_text_font(lbl, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x3C7380), 0);
    lv_obj_set_pos(lbl, 350, 190);

    s_ui_cover = lv_img_create(cover_frame);
    lv_obj_set_size(s_ui_cover, LVGL_COVER_SIZE, LVGL_COVER_SIZE);
    lv_img_set_zoom(s_ui_cover, LVGL_COVER_ZOOM);
    lv_img_set_pivot(s_ui_cover, LVGL_COVER_SIZE / 2u,
                     LVGL_COVER_SIZE / 2u);
    lv_obj_set_style_bg_opa(s_ui_cover, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(s_ui_cover, 10, 0);
    lv_obj_center(s_ui_cover);
    if (s_cover_cached && (s_cover_img.data != NULL)) {
        lv_img_set_src(s_ui_cover, &s_cover_img);
        lv_img_set_zoom(s_ui_cover, LVGL_COVER_ZOOM);
        lv_img_set_pivot(s_ui_cover, LVGL_COVER_SIZE / 2u,
                         LVGL_COVER_SIZE / 2u);
        lv_obj_set_size(s_ui_cover, LVGL_COVER_SIZE, LVGL_COVER_SIZE);
        lv_obj_center(s_ui_cover);
    } else {
        lv_obj_add_flag(s_ui_cover, LV_OBJ_FLAG_HIDDEN);
    }

    s_ui_cover_loading = lv_label_create(cover_frame);
    lv_label_set_text(s_ui_cover_loading, LV_SYMBOL_AUDIO "\n加载封面...");
    lv_obj_set_style_text_font(s_ui_cover_loading, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(s_ui_cover_loading, lv_color_hex(0x979797), 0);
    lv_obj_set_style_text_align(s_ui_cover_loading, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(s_ui_cover_loading);
    if (s_cover_cached && (s_cover_img.data != NULL)) {
        lv_obj_add_flag(s_ui_cover_loading, LV_OBJ_FLAG_HIDDEN);
    }

    /* 控制按钮：上一曲 / 播放暂停 / 下一曲 */
    s_btn_prev = lv_btn_create(scr);
    lv_obj_set_size(s_btn_prev, 66, 52);
    lv_obj_set_style_bg_color(s_btn_prev, lv_color_hex(0x0B3542), 0);
    lv_obj_set_style_bg_opa(s_btn_prev, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_btn_prev, lv_color_hex(0x188A9B), 0);
    lv_obj_set_style_border_width(s_btn_prev, 1, 0);
    lv_obj_set_style_radius(s_btn_prev, 10, 0);
    lbl = lv_label_create(s_btn_prev);
    lv_label_set_text(lbl, LV_SYMBOL_PREV);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xC6FAFF), 0);
    lv_obj_center(lbl);
    lv_obj_set_pos(s_btn_prev, 415, 280);

    s_btn_play = lv_btn_create(scr);
    lv_obj_set_size(s_btn_play, 82, 60);
    lv_obj_set_style_bg_color(s_btn_play, lv_color_hex(0x0B7582), 0);
    lv_obj_set_style_bg_opa(s_btn_play, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_btn_play, lv_color_hex(0x23E2EA), 0);
    lv_obj_set_style_border_width(s_btn_play, 2, 0);
    lv_obj_set_style_radius(s_btn_play, 12, 0);
    lv_obj_set_style_shadow_color(s_btn_play, lv_color_hex(0x0D5B67), 0);
    lv_obj_set_style_shadow_width(s_btn_play, 8, 0);
    lv_obj_set_style_shadow_opa(s_btn_play, LV_OPA_50, 0);
    lbl = lv_label_create(s_btn_play);
    lv_label_set_text(lbl, s_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);
    lv_obj_set_pos(s_btn_play, 495, 276);

    s_btn_next = lv_btn_create(scr);
    lv_obj_set_size(s_btn_next, 66, 52);
    lv_obj_set_style_bg_color(s_btn_next, lv_color_hex(0x0B3542), 0);
    lv_obj_set_style_bg_opa(s_btn_next, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_btn_next, lv_color_hex(0x188A9B), 0);
    lv_obj_set_style_border_width(s_btn_next, 1, 0);
    lv_obj_set_style_radius(s_btn_next, 10, 0);
    lbl = lv_label_create(s_btn_next);
    lv_label_set_text(lbl, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xC6FAFF), 0);
    lv_obj_center(lbl);
    lv_obj_set_pos(s_btn_next, 595, 280);

    lv_obj_add_event_cb(s_btn_play, lvgl_bt_music_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_prev, lvgl_bt_music_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_next, lvgl_bt_music_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 按键导航：PRE/NEXT 移动焦点，OK(ENTER) 触发点击 */
    lv_group_add_obj(s_group, s_btn_prev);
    lv_group_add_obj(s_group, s_btn_play);
    lv_group_add_obj(s_group, s_btn_next);
    lv_group_focus_obj(s_btn_play);
}

/* ── 蓝牙电话：拨号 / 最近通话 / 通讯录 ─────────────────────────────── */
static lv_obj_t *lvgl_phone_button_create(lv_obj_t *parent, lv_coord_t x,
                                           lv_coord_t y, lv_coord_t w,
                                           lv_coord_t h, const char *text,
                                           const lv_font_t *font)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_t *label = lv_label_create(button);

    lv_obj_set_size(button, w, h);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x082B38), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(button, lv_color_hex(0x176878), LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(button, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(button, 0, LV_PART_MAIN);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xE8FAFF), LV_PART_MAIN);
    lv_obj_center(label);
    return button;
}

static void lvgl_phone_button_selected(lv_obj_t *button, INT8U selected)
{
    if (button == NULL) {
        return;
    }
    lv_obj_set_style_border_width(button, selected ? 3 : 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(button,
        selected ? lv_color_hex(0x18DDE7) : lv_color_hex(0x176878),
        LV_PART_MAIN);
    lv_obj_set_style_bg_color(button,
        selected ? lv_color_hex(0x0C5A68) : lv_color_hex(0x082B38),
        LV_PART_MAIN);
}

static const char *lvgl_phone_direction_text(uint8_t direction)
{
    switch (direction) {
    case BT_XSTACK_PHONEBOOK_DIRECTION_INCOMING: return "IN";
    case BT_XSTACK_PHONEBOOK_DIRECTION_OUTGOING: return "OUT";
    case BT_XSTACK_PHONEBOOK_DIRECTION_MISSED: return "MISSED";
    default: return "CALL";
    }
}

static void lvgl_phone_status_set(const char *status)
{
    if (status == NULL) {
        return;
    }
    strncpy(s_phone_status, status, BT_UI_TEXT_MAX - 1u);
    s_phone_status[BT_UI_TEXT_MAX - 1u] = '\0';
    if (s_phone_active && (s_phone_status_label != NULL)) {
        lv_label_set_text(s_phone_status_label, s_phone_status);
    }
}

static void lvgl_phone_sync_start_if_needed(void)
{
    int sync_status;

    if (s_phone_sync_active || s_phone_sync_attempted ||
        (s_phone_contact_count != 0u) ||
        (s_phone_call_count != 0u)) {
        return;
    }

    sync_status = bt_xstack_demo_phonebook_sync();
    if (sync_status == BT_XSTACK_OK) {
        s_phone_sync_active = 1u;
        s_phone_sync_attempted = 1u;
        lvgl_phone_status_set("电话本同步中...");
    } else if (sync_status == BT_XSTACK_NOT_READY) {
        lvgl_phone_status_set("请先连接手机");
    }
}

static void lvgl_phone_switch_tab(INT8U tab)
{
    if (tab >= LVGL_PHONE_TAB_COUNT) {
        return;
    }
    s_phone_tab = tab;
    s_phone_cursor = 0u;
    s_phone_list_offset[tab] = 0u;
    if (tab != LVGL_PHONE_TAB_DIAL) {
        /* Dialing does not need PBAP. Defer the long phonebook transfer
         * until the user explicitly opens a data-bearing tab. */
        lvgl_phone_sync_start_if_needed();
    }
    lvgl_phone_refresh();
}

static void lvgl_phone_refresh_list(lv_obj_t **buttons, lv_obj_t **labels,
                                    INT8U count, INT8U contact_list,
                                    INT8U offset)
{
    INT8U i;
    INT8U index;

    for (i = 0u; i < LVGL_PHONE_VISIBLE_ROWS; i++) {
        index = (INT8U)(offset + i);
        if (index < count) {
            if (contact_list) {
                const lvgl_phone_contact_t *entry = &s_phone_contacts[index];
                lv_label_set_text_fmt(labels[i], "%s\n%s",
                    entry->name[0] != '\0' ? entry->name : entry->number,
                    entry->number);
            } else {
                const lvgl_phone_call_t *entry = &s_phone_calls[index];
                lv_label_set_text_fmt(labels[i], "%s  %s\n%s",
                    lvgl_phone_direction_text(entry->direction),
                    entry->name[0] != '\0' ? entry->name : entry->number,
                    entry->number);
            }
            lv_obj_clear_flag(buttons[i], LV_OBJ_FLAG_HIDDEN);
            lvgl_phone_button_selected(buttons[i], (index == s_phone_cursor));
        } else {
            lv_obj_add_flag(buttons[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void lvgl_phone_refresh(void)
{
    INT8U i;
    INT8U dial_selected;

    if (!s_phone_active) {
        return;
    }
    for (i = 0u; i < LVGL_PHONE_TAB_COUNT; i++) {
        lvgl_phone_button_selected(s_phone_tab_btn[i], (i == s_phone_tab));
        if (i == s_phone_tab) {
            lv_obj_clear_flag(s_phone_panel[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_phone_panel[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_phone_status_label != NULL) {
        lv_label_set_text(s_phone_status_label, s_phone_status);
    }
    if (s_phone_number_label != NULL) {
        lv_label_set_text(s_phone_number_label,
            s_phone_dial[0] != '\0' ? s_phone_dial : "输入号码");
    }
    for (i = 0u; i < LVGL_PHONE_DIAL_KEY_COUNT; i++) {
        dial_selected = !s_phone_nav_active &&
                        (s_phone_tab == LVGL_PHONE_TAB_DIAL) &&
                        (i == s_phone_cursor);
        lvgl_phone_button_selected(s_phone_dial_btn[i], dial_selected);
    }
    lvgl_phone_refresh_list(s_phone_recent_btn, s_phone_recent_label,
                            s_phone_call_count, 0u,
                            s_phone_list_offset[LVGL_PHONE_TAB_RECENT]);
    lvgl_phone_refresh_list(s_phone_contact_btn, s_phone_contact_label,
                            s_phone_contact_count, 1u,
                            s_phone_list_offset[LVGL_PHONE_TAB_CONTACT]);
}

static void lvgl_phone_dial_current(void)
{
    bt_xstack_result_t result;

    if (s_phone_dial[0] == '\0') {
        lvgl_phone_status_set("请先输入号码");
        return;
    }
    result = bt_xstack_hfp_dial(s_phone_dial);
    if (result == BT_XSTACK_OK) {
        lvgl_phone_status_set("正在呼叫...");
    } else {
        lvgl_phone_status_set("拨号失败");
    }
}

static void lvgl_phone_activate(void)
{
    static const char dial_keys[] = "123456789*0#";
    INT32U length;

    if (s_phone_tab == LVGL_PHONE_TAB_DIAL) {
        if (s_phone_cursor < 12u) {
            length = strlen(s_phone_dial);
            if (length < LVGL_PHONE_NUMBER_MAX) {
                s_phone_dial[length] = dial_keys[s_phone_cursor];
                s_phone_dial[length + 1u] = '\0';
            }
        } else if (s_phone_cursor == 12u) {
            length = strlen(s_phone_dial);
            if (length > 0u) {
                s_phone_dial[length - 1u] = '\0';
            }
        } else {
            lvgl_phone_dial_current();
        }
    } else if (s_phone_tab == LVGL_PHONE_TAB_RECENT) {
        if (s_phone_cursor < s_phone_call_count) {
            strncpy(s_phone_dial, s_phone_calls[s_phone_cursor].number,
                    LVGL_PHONE_NUMBER_MAX);
            s_phone_dial[LVGL_PHONE_NUMBER_MAX] = '\0';
            lvgl_phone_dial_current();
        } else {
            lvgl_phone_status_set("暂无最近通话");
        }
    } else if (s_phone_cursor < s_phone_contact_count) {
        strncpy(s_phone_dial, s_phone_contacts[s_phone_cursor].number,
                LVGL_PHONE_NUMBER_MAX);
        s_phone_dial[LVGL_PHONE_NUMBER_MAX] = '\0';
        lvgl_phone_dial_current();
    } else {
        lvgl_phone_status_set("暂无联系人，等待手机同步");
    }
    lvgl_phone_refresh();
}

static void lvgl_phone_key_move(lvgl_phone_key_t key)
{
    INT8U count;

    if (!s_phone_active) {
        return;
    }
    if (key == LVGL_PHONE_KEY_SELECT) {
        lvgl_phone_activate();
        return;
    }
    if (s_phone_tab != LVGL_PHONE_TAB_DIAL) {
        count = (s_phone_tab == LVGL_PHONE_TAB_RECENT) ?
                s_phone_call_count : s_phone_contact_count;
        if (key == LVGL_PHONE_KEY_UP) {
            if (s_phone_cursor > 0u) {
                s_phone_cursor--;
                if (s_phone_cursor < s_phone_list_offset[s_phone_tab]) {
                    s_phone_list_offset[s_phone_tab]--;
                }
            }
        } else if (key == LVGL_PHONE_KEY_DOWN) {
            if ((count > 0u) && (s_phone_cursor + 1u < count)) {
                s_phone_cursor++;
                if (s_phone_cursor >= s_phone_list_offset[s_phone_tab] +
                                      LVGL_PHONE_VISIBLE_ROWS) {
                    s_phone_list_offset[s_phone_tab]++;
                }
            }
        } else if (key == LVGL_PHONE_KEY_LEFT) {
            if (s_phone_tab > LVGL_PHONE_TAB_DIAL) lvgl_phone_switch_tab(s_phone_tab - 1u);
        } else if (key == LVGL_PHONE_KEY_RIGHT) {
            if (s_phone_tab + 1u < LVGL_PHONE_TAB_COUNT) lvgl_phone_switch_tab(s_phone_tab + 1u);
        }
        lvgl_phone_refresh();
        return;
    }

    if (key == LVGL_PHONE_KEY_UP) {
        if (s_phone_cursor >= 12u) s_phone_cursor = (INT8U)(9u + s_phone_cursor - 12u);
        else if (s_phone_cursor >= 3u) s_phone_cursor -= 3u;
    } else if (key == LVGL_PHONE_KEY_DOWN) {
        if (s_phone_cursor < 9u) s_phone_cursor += 3u;
        else if (s_phone_cursor == 9u) s_phone_cursor = 12u;
        else if (s_phone_cursor == 10u) s_phone_cursor = 13u;
        else if ((s_phone_cursor == 11u) || (s_phone_cursor == 12u) ||
                 (s_phone_cursor == 13u)) s_phone_nav_active = 1u;
    } else if (key == LVGL_PHONE_KEY_LEFT) {
        if (s_phone_cursor == 13u) s_phone_cursor = 12u;
        else if ((s_phone_cursor < 12u) && ((s_phone_cursor % 3u) != 0u)) s_phone_cursor--;
    } else if (key == LVGL_PHONE_KEY_RIGHT) {
        if (s_phone_cursor == 12u) s_phone_cursor = 13u;
        else if ((s_phone_cursor < 12u) && ((s_phone_cursor % 3u) != 2u)) s_phone_cursor++;
        else if (s_phone_cursor == 2u || s_phone_cursor == 5u ||
                 s_phone_cursor == 8u || s_phone_cursor == 11u) lvgl_phone_switch_tab(LVGL_PHONE_TAB_RECENT);
    }
    lvgl_phone_refresh();
}

static void lvgl_phone_tab_cb(lv_event_t *event)
{
    INT8U i;
    lv_obj_t *target = lv_event_get_target(event);

    for (i = 0u; i < LVGL_PHONE_TAB_COUNT; i++) {
        if (target == s_phone_tab_btn[i]) {
            lvgl_phone_switch_tab(i);
            return;
        }
    }
}

static void lvgl_phone_action_cb(lv_event_t *event)
{
    INT8U i;
    lv_obj_t *target = lv_event_get_target(event);

    for (i = 0u; i < LVGL_PHONE_DIAL_KEY_COUNT; i++) {
        if (target == s_phone_dial_btn[i]) {
            s_phone_tab = LVGL_PHONE_TAB_DIAL;
            s_phone_cursor = i;
            lvgl_phone_activate();
            return;
        }
    }
    for (i = 0u; i < LVGL_PHONE_VISIBLE_ROWS; i++) {
        if (target == s_phone_recent_btn[i]) {
            s_phone_tab = LVGL_PHONE_TAB_RECENT;
            s_phone_cursor = (INT8U)(s_phone_list_offset[LVGL_PHONE_TAB_RECENT] + i);
            lvgl_phone_activate();
            return;
        }
        if (target == s_phone_contact_btn[i]) {
            s_phone_tab = LVGL_PHONE_TAB_CONTACT;
            s_phone_cursor = (INT8U)(s_phone_list_offset[LVGL_PHONE_TAB_CONTACT] + i);
            lvgl_phone_activate();
            return;
        }
    }
}

static void lvgl_phone_ui_create(lv_obj_t *screen)
{
    static const char *tab_text[LVGL_PHONE_TAB_COUNT] = {
        "拨号", "最近通话", "通讯录"
    };
    /* All coordinates below are local to the 009_plane content frame
     * (930 x 370).  The former phone page still used 1024-wide root-screen
     * coordinates and therefore escaped the frame like a pop-up. */
    /* Three equal tabs form a centred, symmetric group in the header. */
    static const lv_coord_t tab_x[LVGL_PHONE_TAB_COUNT] = { 300, 415, 530 };
    static const char *dial_text[LVGL_PHONE_DIAL_KEY_COUNT] = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "0", "#", "删除", "拨号"
    };
    lv_obj_t *label;
    INT8U i;

    s_phone_active = 1u;
    s_phone_nav_active = 0u;
    lv_group_remove_all_objs(s_group);

    label = lv_label_create(screen);
    lv_label_set_text(label, LV_SYMBOL_CALL "  BLUETOOTH CALL");
    lv_obj_set_style_text_font(label, &lv_font_cn_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0x1ADBE5), LV_PART_MAIN);
    lv_obj_set_pos(label, 50, 20);
    s_phone_status_label = lv_label_create(screen);
    lv_obj_set_width(s_phone_status_label, 240);
    lv_label_set_long_mode(s_phone_status_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(s_phone_status_label, &lv_font_cn_18, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_phone_status_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_phone_status_label, lv_color_hex(0x85C8D0), LV_PART_MAIN);
    /* Right-aligned counterpart of the left BLUETOOTH CALL heading. */
    lv_obj_set_pos(s_phone_status_label, 640, 22);

    for (i = 0u; i < LVGL_PHONE_TAB_COUNT; i++) {
        s_phone_tab_btn[i] = lvgl_phone_button_create(screen, tab_x[i],
            18, 100, 38, tab_text[i], &lv_font_cn_18);
        lv_obj_add_event_cb(s_phone_tab_btn[i], lvgl_phone_tab_cb, LV_EVENT_CLICKED, NULL);
        s_phone_panel[i] = lv_obj_create(screen);
        lv_obj_set_size(s_phone_panel[i], 870, 282);
        lv_obj_set_pos(s_phone_panel[i], 30, 66);
        /* 009_plane is the only page background.  Tab panels are transparent
         * layout containers so the dialer is embedded, never a second card. */
        lv_obj_set_style_bg_opa(s_phone_panel[i], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(s_phone_panel[i], 0, LV_PART_MAIN);
        lv_obj_set_style_radius(s_phone_panel[i], 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(s_phone_panel[i], 0, LV_PART_MAIN);
        lv_obj_clear_flag(s_phone_panel[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    s_phone_number_label = lv_label_create(s_phone_panel[LVGL_PHONE_TAB_DIAL]);
    lv_obj_set_width(s_phone_number_label, 360);
    lv_label_set_long_mode(s_phone_number_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(s_phone_number_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_phone_number_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_phone_number_label, lv_color_hex(0xE7FBFF), LV_PART_MAIN);
    lv_obj_set_pos(s_phone_number_label, 255, 8);
    for (i = 0u; i < 12u; i++) {
        s_phone_dial_btn[i] = lvgl_phone_button_create(s_phone_panel[LVGL_PHONE_TAB_DIAL],
            286 + (lv_coord_t)(i % 3u) * 99, 68 + (lv_coord_t)(i / 3u) * 42,
            84, 36, dial_text[i], &lv_font_cn_18);
        lv_obj_add_event_cb(s_phone_dial_btn[i], lvgl_phone_action_cb, LV_EVENT_CLICKED, NULL);
    }
    s_phone_dial_btn[12] = lvgl_phone_button_create(s_phone_panel[LVGL_PHONE_TAB_DIAL],
        292, 240, 136, 34, dial_text[12], &lv_font_cn_18);
    s_phone_dial_btn[13] = lvgl_phone_button_create(s_phone_panel[LVGL_PHONE_TAB_DIAL],
        442, 240, 136, 34, dial_text[13], &lv_font_cn_18);
    lv_obj_set_style_bg_color(s_phone_dial_btn[13], lv_color_hex(0x0D6C75),
                               LV_PART_MAIN);
    lv_obj_set_style_border_color(s_phone_dial_btn[13], lv_color_hex(0x18DDE7),
                                  LV_PART_MAIN);
    for (i = 12u; i < LVGL_PHONE_DIAL_KEY_COUNT; i++) {
        lv_obj_add_event_cb(s_phone_dial_btn[i], lvgl_phone_action_cb, LV_EVENT_CLICKED, NULL);
    }

    for (i = 0u; i < LVGL_PHONE_VISIBLE_ROWS; i++) {
        s_phone_recent_btn[i] = lvgl_phone_button_create(s_phone_panel[LVGL_PHONE_TAB_RECENT],
            24, 10 + (lv_coord_t)i * 52, 822, 44, "", &lv_font_cn_18);
        s_phone_recent_label[i] = lv_obj_get_child(s_phone_recent_btn[i], 0);
        lv_obj_set_width(s_phone_recent_label[i], 770);
        lv_label_set_long_mode(s_phone_recent_label[i], LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(s_phone_recent_label[i], LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_align(s_phone_recent_label[i], LV_ALIGN_LEFT_MID, 18, 0);
        lv_obj_add_event_cb(s_phone_recent_btn[i], lvgl_phone_action_cb, LV_EVENT_CLICKED, NULL);

        s_phone_contact_btn[i] = lvgl_phone_button_create(s_phone_panel[LVGL_PHONE_TAB_CONTACT],
            24, 10 + (lv_coord_t)i * 52, 822, 44, "", &lv_font_cn_18);
        s_phone_contact_label[i] = lv_obj_get_child(s_phone_contact_btn[i], 0);
        lv_obj_set_width(s_phone_contact_label[i], 770);
        lv_label_set_long_mode(s_phone_contact_label[i], LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(s_phone_contact_label[i], LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_align(s_phone_contact_label[i], LV_ALIGN_LEFT_MID, 18, 0);
        lv_obj_add_event_cb(s_phone_contact_btn[i], lvgl_phone_action_cb, LV_EVENT_CLICKED, NULL);
    }
    lvgl_phone_refresh();
}

/* ── 蓝牙通话悬浮控件：来电 / 去电 / 通话中（自动出现/自动消失，不占页面） ── */
static const char *lvgl_call_state_text(INT8U state, INT8U ringing)
{
    switch (state) {
    case BT_XSTACK_HFP_STATE_OUTGOING:
        return "正在呼叫";
    case BT_XSTACK_HFP_STATE_INCOMING:
    case BT_XSTACK_HFP_STATE_ACTIVE_WITH_INCOMING:
        return "来电";
    case BT_XSTACK_HFP_STATE_ACTIVE:
        return "通话中";
    case BT_XSTACK_HFP_STATE_HELD:
        return "通话保持";
    case BT_XSTACK_HFP_STATE_ACTIVE_WITH_HELD:
        return "通话中/一路保持";
    case BT_XSTACK_HFP_STATE_MULTIPARTY:
        return "多方通话";
    default:
        return ringing ? "来电" : "通话中";
    }
}

/* 号码比对：忽略 +86/空格/横线等格式差异，只比数字尾号。 */
static INT8U lvgl_call_tail_match(const char *a, const char *b)
{
    char digits_a[LVGL_PHONE_NUMBER_MAX];
    char digits_b[LVGL_PHONE_NUMBER_MAX];
    INT32U len_a = 0u;
    INT32U len_b = 0u;
    INT32U count;
    INT32U i;

    if ((a == NULL) || (b == NULL)) {
        return 0u;
    }
    while ((*a != '\0') && (len_a < sizeof(digits_a))) {
        if ((*a >= '0') && (*a <= '9')) {
            digits_a[len_a++] = *a;
        }
        a++;
    }
    while ((*b != '\0') && (len_b < sizeof(digits_b))) {
        if ((*b >= '0') && (*b <= '9')) {
            digits_b[len_b++] = *b;
        }
        b++;
    }
    count = (len_a < len_b) ? len_a : len_b;
    if (count < LVGL_CALL_NUMBER_MIN_DIGITS) {
        return 0u;
    }
    for (i = 0u; i < count; i++) {
        if (digits_a[len_a - 1u - i] != digits_b[len_b - 1u - i]) {
            return 0u;
        }
    }
    return 1u;
}

/* 用已同步的 PBAP 通讯录匹配来电号码；匹配不到则清空名字。 */
static void lvgl_call_match_contact(void)
{
    INT8U i;

    s_call_name[0] = '\0';
    if (s_call_number[0] == '\0') {
        return;
    }
    for (i = 0u; i < s_phone_contact_count; i++) {
        if (lvgl_call_tail_match(s_call_number, s_phone_contacts[i].number)) {
            lvgl_phone_copy_text(s_call_name, s_phone_contacts[i].name,
                                 sizeof(s_call_name));
            return;
        }
    }
}

static void lvgl_call_refresh(void)
{
    INT32U minutes;
    INT32U seconds;

    if (!s_call_page_active) {
        return;
    }
    if (s_call_caller_label != NULL) {
        /* 通话人：优先通讯录匹配到的名字，否则显示号码。 */
        lv_label_set_text_fmt(s_call_caller_label, "%s%s",
                              (s_call_name[0] != '\0') ? s_call_name :
                              ((s_call_number[0] != '\0') ? s_call_number :
                                                           "未知号码"),
                              s_call_muted ? " / 已静音" : "");
    }
    if (s_call_timer_label != NULL) {
        if (s_call_talking) {
            minutes = s_call_seconds / 60u;
            seconds = s_call_seconds % 60u;
            lv_label_set_text_fmt(s_call_timer_label, "%02u:%02u",
                                  (unsigned int)minutes, (unsigned int)seconds);
        } else {
            /* 还没接通，用状态文字占住时长位。 */
            lv_label_set_text(s_call_timer_label,
                              lvgl_call_state_text(s_call_hfp_state,
                                                   s_call_ringing));
        }
    }
    if (s_call_answer_btn != NULL) {
        /* 只有来电（还没接通）才需要绿色接听。 */
        if (s_call_incoming != 0u) {
            lv_obj_clear_flag(s_call_answer_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_call_answer_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void lvgl_call_tick(lv_timer_t *timer)
{
    (void)timer;
    if (!s_call_page_active || !s_call_talking) {
        return;
    }
    s_call_seconds++;
    lvgl_call_refresh();
}

static void lvgl_call_panel_destroy(void)
{
    if (s_call_timer != NULL) {
        lv_timer_del(s_call_timer);
        s_call_timer = NULL;
    }
    if (s_call_panel != NULL) {
        lv_obj_del(s_call_panel);
        s_call_panel = NULL;
    }
    s_call_caller_label = NULL;
    s_call_timer_label = NULL;
    s_call_answer_btn = NULL;
}

/* 圆形图标按钮（绿接听 / 红挂断）：只有颜色和图标不同，尺寸样式一致。 */
static lv_obj_t *lvgl_call_icon_btn_create(lv_obj_t *parent, lv_coord_t x_ofs,
                                           lv_color_t color, const char *symbol)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_t *icon;

    lv_obj_set_size(btn, 48, 48);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, x_ofs, 0);
    lv_obj_set_style_bg_color(btn, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    icon = lv_label_create(btn);
    lv_label_set_text(icon, symbol);
    lv_obj_set_style_text_font(icon, &lv_font_cn_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(icon);
    return btn;
}

/* 通话控件：一条悬浮小横幅（通话人 / 时长 / 绿接听 / 红挂断）。
   挂在 lv_layer_top()，可显示在任意页面上方，且不遮挡页面内容。 */
static void lvgl_call_panel_create(void)
{
    s_call_panel = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_call_panel, 660, 76);
    lv_obj_align(s_call_panel, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_bg_color(s_call_panel, lv_color_hex(0x0B2632), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_call_panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_call_panel, lv_color_hex(0x1BB8C9), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_call_panel, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_call_panel, 16, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_call_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_call_panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_call_panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    s_call_caller_label = lv_label_create(s_call_panel);
    lv_obj_set_width(s_call_caller_label, 340);
    lv_label_set_long_mode(s_call_caller_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(s_call_caller_label, &lv_font_cn_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_call_caller_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(s_call_caller_label, LV_ALIGN_LEFT_MID, 24, 0);

    s_call_timer_label = lv_label_create(s_call_panel);
    lv_obj_set_style_text_font(s_call_timer_label, &lv_font_cn_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_call_timer_label, lv_color_hex(0x82ACB6), LV_PART_MAIN);
    lv_obj_align(s_call_timer_label, LV_ALIGN_RIGHT_MID, -184, 0);

    s_call_answer_btn = lvgl_call_icon_btn_create(s_call_panel, -92,
                                                  lv_color_hex(0x22C55E),
                                                  LV_SYMBOL_CALL);
    (void)lvgl_call_icon_btn_create(s_call_panel, -24,
                                    lv_color_hex(0xE8453C), LV_SYMBOL_CLOSE);
}

static void lvgl_call_page_show(void)
{
    bool muted = false;

    if (s_call_page_active) {
        return;
    }
    s_call_seconds = 0u;
    if (bt_xstack_get_microphone_mute(&muted) == BT_XSTACK_OK) {
        s_call_muted = muted ? 1u : 0u;
    }
    lvgl_call_panel_create();
    s_call_page_active = 1u;
    lvgl_call_match_contact();
    lvgl_call_refresh();
    s_call_timer = lv_timer_create(lvgl_call_tick, LVGL_CALL_TICK_MS, NULL);
    lv_obj_invalidate(lv_layer_top());
    LVGL_RUNTIME_LOG("[LVGL] call widget shown state=%u\n",
                     (unsigned int)s_call_hfp_state);
}

static void lvgl_call_page_hide(void)
{
    if (!s_call_page_active) {
        return;
    }
    s_call_page_active = 0u;
    s_call_seconds = 0u;
    lvgl_call_panel_destroy();
    lv_obj_invalidate(lv_layer_top());
    LVGL_RUNTIME_LOG("[LVGL] call widget hidden\n");
}

/* 通话事件驱动悬浮控件：present=去电/来电/通话中，任一到来自动弹出，结束自动关闭。 */
static void lvgl_call_apply_event(const bt_ui_event_t *event)
{
    INT8U present;
    INT8U talking;

    if (event == NULL) {
        return;
    }
    switch (event->type) {
    case BT_UI_EVENT_HFP_STATE:
        s_call_hfp_state = (INT8U)event->values[0];
        if ((s_call_hfp_state != BT_XSTACK_HFP_STATE_INCOMING) &&
            (s_call_hfp_state != BT_XSTACK_HFP_STATE_ACTIVE_WITH_INCOMING)) {
            s_call_ringing = 0u;
        }
        break;
    case BT_UI_EVENT_CALL_NUMBER:
        lvgl_phone_copy_text(s_call_number, event->text, sizeof(s_call_number));
        lvgl_call_match_contact();
        break;
    case BT_UI_EVENT_RING:
        s_call_ringing = (event->values[0] != 0u) ? 1u : 0u;
        break;
    case BT_UI_EVENT_MIC_MUTE:
        s_call_muted = (event->values[0] != 0u) ? 1u : 0u;
        break;
    case BT_UI_EVENT_PB_CONTACT:
    case BT_UI_EVENT_PB_DONE:
        lvgl_call_match_contact();
        break;
    default:
        return;
    }

    present = ((s_call_hfp_state >= BT_XSTACK_HFP_STATE_OUTGOING) &&
               (s_call_hfp_state <= BT_XSTACK_HFP_STATE_MULTIPARTY)) ||
              (s_call_ringing != 0u);
    talking = ((s_call_hfp_state == BT_XSTACK_HFP_STATE_ACTIVE) ||
               (s_call_hfp_state == BT_XSTACK_HFP_STATE_ACTIVE_WITH_INCOMING) ||
               (s_call_hfp_state == BT_XSTACK_HFP_STATE_ACTIVE_WITH_HELD) ||
               (s_call_hfp_state == BT_XSTACK_HFP_STATE_MULTIPARTY));
    s_call_incoming = ((s_call_hfp_state == BT_XSTACK_HFP_STATE_INCOMING) ||
                       (s_call_hfp_state == BT_XSTACK_HFP_STATE_ACTIVE_WITH_INCOMING));
    if (talking && !s_call_talking) {
        s_call_seconds = 0u;    /* 进入通话态才开始计时 */
    }
    s_call_talking = talking ? 1u : 0u;
    s_call_present = present ? 1u : 0u;

    if (!s_call_present) {
        lvgl_call_page_hide();
    } else if (!s_call_page_active) {
        lvgl_call_page_show();
    } else {
        lvgl_call_refresh();
    }
}

static void lvgl_show_music_page(void)
{
    if (!s_dashboard_active || (s_active_page == LVGL_PAGE_MUSIC)) {
        return;
    }
    s_phone_active = 0u;
    s_active_page = LVGL_PAGE_MUSIC;
    s_nav_focus = LVGL_PAGE_MUSIC;
    lvgl_nav_marker_refresh();
    lvgl_dashboard_background_set("U:/shell.bin");
    lvgl_dashboard_content_set_visible(0u);
    if (s_content_panel != NULL) lv_obj_del(s_content_panel);
    s_content_panel = lvgl_content_frame_create();
    lvgl_bt_music_ui_create(s_content_panel);
    lv_obj_invalidate(lv_scr_act());
}

static void lvgl_show_phone_page(void)
{
    if (!s_dashboard_active || (s_active_page == LVGL_PAGE_PHONE)) {
        return;
    }
    s_phone_active = 1u;
    s_active_page = LVGL_PAGE_PHONE;
    s_nav_focus = LVGL_PAGE_PHONE;
    lvgl_nav_marker_refresh();
    lvgl_dashboard_background_set("U:/shell.bin");
    s_phone_tab = LVGL_PHONE_TAB_DIAL;
    s_phone_cursor = 0u;
    s_phone_list_offset[LVGL_PHONE_TAB_RECENT] = 0u;
    s_phone_list_offset[LVGL_PHONE_TAB_CONTACT] = 0u;
    lvgl_dashboard_content_set_visible(0u);
    if (s_content_panel != NULL) lv_obj_del(s_content_panel);
    s_ui_title = s_ui_artist = s_ui_cover = s_ui_cover_loading = NULL;
    s_btn_play = s_btn_prev = s_btn_next = NULL;
    s_content_panel = lvgl_content_frame_create();
    lvgl_phone_ui_create(s_content_panel);
    if (s_phone_sync_active) {
        lvgl_phone_status_set("电话本同步中...");
    } else if ((s_phone_contact_count != 0u) || (s_phone_call_count != 0u)) {
        lvgl_phone_status_set("电话本已同步");
    } else {
        lvgl_phone_status_set("已连接手机");
    }
    lv_obj_invalidate(lv_scr_act());
}

static void lvgl_show_dashboard_page(void)
{
    if (!s_dashboard_active || (s_active_page == LVGL_PAGE_DASHBOARD)) {
        return;
    }
    s_phone_active = 0u;
    s_active_page = LVGL_PAGE_DASHBOARD;
    s_nav_focus = LVGL_PAGE_DASHBOARD;
    lvgl_nav_marker_refresh();
    lvgl_dashboard_background_set("U:/dashboard.bin");
    if (s_content_panel != NULL) {
        lv_obj_del(s_content_panel);
        s_content_panel = NULL;
    }
    s_ui_title = s_ui_artist = s_ui_cover = s_ui_cover_loading = NULL;
    s_btn_play = s_btn_prev = s_btn_next = NULL;
    lvgl_dashboard_content_set_visible(1u);
    lv_obj_invalidate(lv_scr_act());
}

static void lvgl_show_location_page(void)
{
    lv_obj_t *title;
    lv_obj_t *detail;

    if (!s_dashboard_active || (s_active_page == LVGL_PAGE_LOCATION)) return;
    s_phone_active = 0u;
    s_active_page = LVGL_PAGE_LOCATION;
    s_nav_focus = LVGL_PAGE_LOCATION;
    lvgl_nav_marker_refresh();
    lvgl_dashboard_background_set("U:/shell.bin");
    lvgl_dashboard_content_set_visible(0u);
    if (s_content_panel != NULL) lv_obj_del(s_content_panel);
    s_ui_title = s_ui_artist = s_ui_cover = s_ui_cover_loading = NULL;
    s_btn_play = s_btn_prev = s_btn_next = NULL;
    s_content_panel = lvgl_content_frame_create();
    title = lv_label_create(s_content_panel);
    lv_label_set_text(title, LV_SYMBOL_GPS "  位置导航");
    lv_obj_set_style_text_font(title, &lv_font_cn_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0x18DDE7), LV_PART_MAIN);
    lv_obj_set_pos(title, 36, 32);
    detail = lv_label_create(s_content_panel);
    lv_label_set_text(detail, "您的目的地\nSkinner Hollow Road, Grants Pass\n\n导航服务待接入");
    lv_obj_set_style_text_font(detail, &lv_font_cn_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(detail, lv_color_hex(0xC8E8EC), LV_PART_MAIN);
    lv_obj_set_pos(detail, 76, 102);
    lv_obj_invalidate(lv_scr_act());
}

static void lvgl_show_speed_page(void)
{
    lv_obj_t *title;
    lv_obj_t *detail;

    if (!s_dashboard_active || (s_active_page == LVGL_PAGE_SPEED)) return;
    s_phone_active = 0u;
    s_active_page = LVGL_PAGE_SPEED;
    s_nav_focus = LVGL_PAGE_SPEED;
    lvgl_nav_marker_refresh();
    lvgl_dashboard_background_set("U:/shell.bin");
    lvgl_dashboard_content_set_visible(0u);
    if (s_content_panel != NULL) lv_obj_del(s_content_panel);
    s_ui_title = s_ui_artist = s_ui_cover = s_ui_cover_loading = NULL;
    s_btn_play = s_btn_prev = s_btn_next = NULL;
    s_content_panel = lvgl_content_frame_create();
    title = lv_label_create(s_content_panel);
    lv_label_set_text(title, "SPEED ASSIST");
    lv_obj_set_style_text_font(title, &lv_font_cn_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0x18DDE7), LV_PART_MAIN);
    lv_obj_set_pos(title, 50, 36);
    detail = lv_label_create(s_content_panel);
    lv_label_set_text(detail, "速度辅助页面\n功能预留，后续接入车辆数据");
    lv_obj_set_style_text_font(detail, &lv_font_cn_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(detail, lv_color_hex(0xB7D9DE), LV_PART_MAIN);
    lv_obj_set_pos(detail, 90, 120);
    lv_obj_invalidate(lv_scr_act());
}

static void lvgl_nav_show_focused_page(void)
{
    switch (s_nav_focus) {
    case LVGL_PAGE_DASHBOARD:
        lvgl_show_dashboard_page();
        break;
    case LVGL_PAGE_LOCATION:
        lvgl_show_location_page();
        break;
    case LVGL_PAGE_PHONE:
        lvgl_show_phone_page();
        break;
    case LVGL_PAGE_MUSIC:
        lvgl_show_music_page();
        break;
    case LVGL_PAGE_SPEED:
        lvgl_show_speed_page();
        break;
    default:
        break;
    }
}

static void lvgl_nav_key_move(lvgl_phone_key_t key)
{
    static const INT8U nav_order[LVGL_PAGE_COUNT] = {
        LVGL_PAGE_LOCATION, LVGL_PAGE_SPEED, LVGL_PAGE_DASHBOARD,
        LVGL_PAGE_MUSIC, LVGL_PAGE_PHONE
    };
    INT8U i;

    if (s_phone_active) {
        if (!s_phone_nav_active) {
            lvgl_phone_key_move(key);
            return;
        }
        if (key == LVGL_PHONE_KEY_UP) {
            s_phone_nav_active = 0u;
            s_phone_cursor = 13u; /* Return to the green "dial" key. */
            lvgl_phone_refresh();
            return;
        }
        if ((key != LVGL_PHONE_KEY_LEFT) && (key != LVGL_PHONE_KEY_RIGHT)) {
            return;
        }
        /* Key2/Key4 now operate the fixed bottom strip and leave dialer
         * focus; the selected icon switches its page immediately below. */
        s_phone_active = 0u;
        s_phone_nav_active = 0u;
    }

    if ((key != LVGL_PHONE_KEY_LEFT) && (key != LVGL_PHONE_KEY_RIGHT)) {
        return;
    }
    for (i = 0u; i < LVGL_PAGE_COUNT; i++) {
        if (nav_order[i] == s_nav_focus) break;
    }
    if (i >= LVGL_PAGE_COUNT) {
        i = 0u;
    }
    if (key == LVGL_PHONE_KEY_LEFT) {
        i = (i == 0u) ? (LVGL_PAGE_COUNT - 1u) : (INT8U)(i - 1u);
    } else {
        i = (INT8U)((i + 1u) % LVGL_PAGE_COUNT);
    }
    s_nav_focus = nav_order[i];
    lvgl_nav_marker_refresh();
    /* The highlight and the content are one selection.  No confirmation key
     * is needed and no separate page transition is shown. */
    lvgl_nav_show_focused_page();
}

int lvgl_page_show_music(void)
{
    if ((s_dashboard_active == 0u) || (LVGLTaskQ == 0)) {
        DBG_PRINT("[LVGL] music request ignored (dashboard=%u queue=0x%x)\r\n",
                  s_dashboard_active, (INT32U)LVGLTaskQ);
        return 0;
    }
    if (s_page_switch_pending != 0u) {
        return 1;
    }
    s_page_switch_pending = 1u;
    if (msgQSend(LVGLTaskQ, MSG_LVGL_SHOW_MUSIC, NULL, 0u,
                 MSG_PRI_NORMAL) != 0) {
        s_page_switch_pending = 0u;
        DBG_PRINT("[LVGL] music page request failed\r\n");
        return 0;
    }
    DBG_PRINT("[LVGL] -> music page\r\n");
    return 1;
}

int lvgl_page_show_phone(void)
{
    if (LVGLTaskQ == 0) {
        return 0;
    }
    if (s_page_switch_pending != 0u) {
        return 1;
    }
    s_page_switch_pending = 1u;
    if (msgQSend(LVGLTaskQ, MSG_LVGL_SHOW_PHONE, NULL, 0u,
                 MSG_PRI_NORMAL) != 0) {
        s_page_switch_pending = 0u;
        DBG_PRINT("[LVGL] phone page request failed\r\n");
        return 0;
    }
    DBG_PRINT("[LVGL] -> phone page\r\n");
    return 1;
}

int lvgl_phone_key_send(lvgl_phone_key_t key)
{
    INT32U message;

    if ((LVGLTaskQ == 0) || (key > LVGL_PHONE_KEY_SELECT)) {
        return 0;
    }
    if (!s_phone_active) {
        /* Outside the dialer, only Key2/Key4 move the bottom strip.  Consume
         * the other dialer directions so Key1/Key3 cannot also issue the old
         * AVRCP mapping after Key5/Key6/Key8 took over media control. */
        if ((key != LVGL_PHONE_KEY_LEFT) && (key != LVGL_PHONE_KEY_RIGHT)) {
            return (s_dashboard_active != 0u) ? 1 : 0;
        }
    }
    message = MSG_LVGL_PHONE_KEY_UP + (INT32U)key;
    if (msgQSend(LVGLTaskQ, message, NULL, 0u, MSG_PRI_NORMAL) != 0) {
        DBG_PRINT("[LVGL] navigation key %u queue full\r\n", (unsigned int)key);
        return 0;
    }
    return 1;
}

/* 通话控件期间，媒体键改作通话控制：Key5=接听、Key6=挂断/拒接、Key8=静音。
   始终返回 1，把按键留给通话控件，不再下发 AVRCP。 */
static int lvgl_call_key_send(lvgl_music_key_t key)
{
    bt_xstack_result_t status;
    bool muted = false;

    if (key == LVGL_MUSIC_KEY_PREVIOUS) {
        if (s_call_incoming == 0u) {
            return 1;   /* 非来电状态下“接听”无意义 */
        }
        status = bt_xstack_hfp_answer();
    } else if (key == LVGL_MUSIC_KEY_NEXT) {
        status = bt_xstack_hfp_hang_up();
    } else if (key == LVGL_MUSIC_KEY_PLAY_PAUSE) {
        if ((s_call_talking == 0u) ||
            (bt_xstack_get_microphone_mute(&muted) != BT_XSTACK_OK)) {
            return 1;
        }
        status = bt_xstack_set_microphone_mute(!muted);
        if (status == BT_XSTACK_OK) {
            /* 先本地反馈，稍后协议栈的 MICROPHONE_MUTE 事件仍是权威值。 */
            bt_ui_post_simple(BT_UI_EVENT_MIC_MUTE, muted ? 0u : 1u, 0u, 0u);
        }
    } else {
        return 1;
    }
    LVGL_RUNTIME_LOG("[LVGL] call key=%u status=%d\r\n",
                     (unsigned int)key, status);
    return 1;
}

int lvgl_music_key_send(lvgl_music_key_t key)
{
    bt_xstack_result_t status;

    if ((s_dashboard_active == 0u) || (key > LVGL_MUSIC_KEY_PLAY_PAUSE)) {
        return 0;
    }
    if (s_call_present != 0u) {
        /* 来电/通话中优先，物理键不落到音乐页。 */
        return lvgl_call_key_send(key);
    }
    /* The physical key is reserved by this UI while LVGL is active.  It
     * controls AVRCP only on the selected music page, never opens a page. */
    if (s_active_page != LVGL_PAGE_MUSIC) {
        return 1;
    }
    /* This function is called by the peripheral task.  Do not touch LVGL
     * objects here; AVRCP state events later refresh the controls in the
     * LVGL task. */
    if (key == LVGL_MUSIC_KEY_PREVIOUS) {
        status = bt_xstack_avrcp_previous();
    } else if (key == LVGL_MUSIC_KEY_NEXT) {
        status = bt_xstack_avrcp_next();
    } else {
        status = lvgl_bt_music_play_pause_request();
    }
    LVGL_RUNTIME_LOG("[LVGL] physical AVRCP key=%u status=%d\r\n",
                     (unsigned int)key, status);
    return 1;
}

int lvgl_page_show_dashboard(void)
{
    if ((s_dashboard_active == 0u) || (LVGLTaskQ == 0)) {
        DBG_PRINT("[LVGL] dashboard request ignored (dashboard=%u queue=0x%x)\r\n",
                  s_dashboard_active, (INT32U)LVGLTaskQ);
        return 0;
    }
    if (s_page_switch_pending != 0u) {
        return 1;
    }
    s_page_switch_pending = 1u;
    if (msgQSend(LVGLTaskQ, MSG_LVGL_SHOW_DASHBOARD, NULL, 0u,
                 MSG_PRI_NORMAL) != 0) {
        s_page_switch_pending = 0u;
        DBG_PRINT("[LVGL] dashboard page request failed\r\n");
        return 0;
    }
    DBG_PRINT("[LVGL] -> dashboard page\r\n");
    return 1;
}

static void lvgl_phone_copy_text(char *dst, const char *src, INT32U size)
{
    if ((dst == NULL) || (size == 0u)) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, size - 1u);
    dst[size - 1u] = '\0';
}

static void lvgl_phone_apply_event(const bt_ui_event_t *event)
{
    INT8U refresh = 0u;

    switch (event->type) {
    case BT_UI_EVENT_HFP_STATE:
        if (event->value_count > 0u) {
            if (event->values[0] == BT_XSTACK_HFP_STATE_INCOMING) {
                lvgl_phone_status_set("来电");
            } else if (event->values[0] == BT_XSTACK_HFP_STATE_OUTGOING) {
                lvgl_phone_status_set("正在呼叫...");
            } else if (event->values[0] == BT_XSTACK_HFP_STATE_ACTIVE) {
                lvgl_phone_status_set("通话中");
            } else if (event->values[0] == BT_XSTACK_HFP_STATE_CONNECTED) {
                /* A newly connected phone owns a new PBAP cache lifetime. */
                s_phone_sync_attempted = 0u;
                lvgl_phone_status_set("已连接手机");
            }
        }
        refresh = 1u;
        break;
    case BT_UI_EVENT_CALL_NUMBER:
        lvgl_phone_copy_text(s_phone_remote_number, event->text,
                             sizeof(s_phone_remote_number));
        if (s_phone_remote_number[0] != '\0') {
            lvgl_phone_status_set(s_phone_remote_number);
        }
        refresh = 1u;
        break;
    case BT_UI_EVENT_PB_STARTED:
        if (!s_phone_sync_active) {
            s_phone_contact_count = 0u;
            s_phone_call_count = 0u;
            s_phone_sync_active = 1u;
        }
        lvgl_phone_status_set("电话本同步中...");
        refresh = 1u;
        break;
    case BT_UI_EVENT_PB_CONTACT:
        if ((event->pb.contact.number_count > 0u) &&
            (s_phone_contact_count < LVGL_PHONE_CONTACT_MAX)) {
            lvgl_phone_contact_t *contact =
                &s_phone_contacts[s_phone_contact_count++];
            lvgl_phone_copy_text(contact->name, event->pb.contact.name,
                                 sizeof(contact->name));
            lvgl_phone_copy_text(contact->number,
                                 event->pb.contact.numbers[0].value,
                                 sizeof(contact->number));
        }
        refresh = (s_phone_tab == LVGL_PHONE_TAB_CONTACT) ? 1u : 0u;
        break;
    case BT_UI_EVENT_PB_CALL_LOG:
        if (s_phone_call_count < LVGL_PHONE_CALL_MAX) {
            lvgl_phone_call_t *call = &s_phone_calls[s_phone_call_count++];
            lvgl_phone_copy_text(call->name, event->pb.call.name,
                                 sizeof(call->name));
            lvgl_phone_copy_text(call->number, event->pb.call.number,
                                 sizeof(call->number));
            lvgl_phone_copy_text(call->timestamp, event->pb.call.timestamp,
                                 sizeof(call->timestamp));
            call->direction = event->pb.call.direction;
        }
        refresh = (s_phone_tab == LVGL_PHONE_TAB_RECENT) ? 1u : 0u;
        break;
    case BT_UI_EVENT_PB_DONE:
        s_phone_sync_active = 0u;
        lvgl_phone_status_set("电话本已同步");
        refresh = 1u;
        break;
    case BT_UI_EVENT_PB_AUTH:
        s_phone_sync_active = 0u;
        lvgl_phone_status_set("手机需要电话本授权");
        refresh = 1u;
        break;
    case BT_UI_EVENT_PB_ERROR:
        s_phone_sync_active = 0u;
        lvgl_phone_status_set("电话本同步失败");
        refresh = 1u;
        break;
    default:
        break;
    }

    if (s_phone_active && refresh) {
        if (s_phone_tab == LVGL_PHONE_TAB_RECENT &&
            s_phone_cursor >= s_phone_call_count) {
            s_phone_cursor = (s_phone_call_count > 0u) ?
                (s_phone_call_count - 1u) : 0u;
        } else if (s_phone_tab == LVGL_PHONE_TAB_CONTACT &&
                   s_phone_cursor >= s_phone_contact_count) {
            s_phone_cursor = (s_phone_contact_count > 0u) ?
                (s_phone_contact_count - 1u) : 0u;
        }
        lvgl_phone_refresh();
    }
}

static void lvgl_bt_music_apply_event(const bt_ui_event_t *event)
{
    if (event == NULL) {
        return;
    }

    if (event->type == BT_UI_EVENT_TRACK_INFO) {
        strncpy(s_music_title,
                event->text[0] != '\0' ? event->text : "Unknown title",
                BT_UI_TEXT_MAX - 1u);
        s_music_title[BT_UI_TEXT_MAX - 1u] = '\0';
        strncpy(s_music_artist,
                event->text2[0] != '\0' ? event->text2 : "Unknown artist",
                BT_UI_TEXT_MAX - 1u);
        s_music_artist[BT_UI_TEXT_MAX - 1u] = '\0';
        strncpy(s_music_album, event->text3, BT_UI_TEXT_MAX - 1u);
        s_music_album[BT_UI_TEXT_MAX - 1u] = '\0';
    } else if (event->type == BT_UI_EVENT_PLAY_REQUESTED) {
        INT8U requested_playing = (event->values[0] != 0u) ? 1u : 0u;
        INT8U apply_state = 0u;

        taskENTER_CRITICAL();
        if (event->values[1] == s_play_command_sequence) {
            if (s_play_command_pending &&
                (requested_playing == s_play_command_target)) {
                apply_state = (s_playing != requested_playing) ? 1u : 0u;
                s_playing = requested_playing;
            } else if (!s_play_command_pending &&
                       (requested_playing == s_play_confirmed)) {
                apply_state = (s_playing != requested_playing) ? 1u : 0u;
                s_playing = requested_playing;
            }
        }
        taskEXIT_CRITICAL();

        if (apply_state && (s_active_page == LVGL_PAGE_MUSIC) &&
            (s_btn_play != NULL)) {
            lv_label_set_text(lv_obj_get_child(s_btn_play, 0),
                              requested_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        }
        return;
    } else if (event->type == BT_UI_EVENT_PLAY_STATE) {
        INT8U reported_playing =
            (event->values[0] == BT_XSTACK_PLAY_STATE_PLAYING) ? 1u : 0u;
        INT8U apply_state = 1u;
        TickType_t now = xTaskGetTickCount();
        TickType_t timeout_ticks =
            (TickType_t)(LVGL_PLAY_STATE_CONFIRM_TIMEOUT_MS /
                         portTICK_PERIOD_MS);

        taskENTER_CRITICAL();
        if (s_play_command_pending && s_play_command_guard &&
            ((int32_t)(event->values[3] -
                       s_play_request_event_sequence) <= 0)) {
            apply_state = 0u;
        } else {
            s_play_command_guard = 0u;
            if (s_play_command_pending) {
                if ((reported_playing == s_play_command_target) ||
                    ((now - s_play_command_tick) >= timeout_ticks)) {
                    s_play_command_pending = 0u;
                    s_play_confirmed = reported_playing;
                    s_playing = reported_playing;
                } else {
                    apply_state = 0u;
                }
            } else {
                s_play_confirmed = reported_playing;
                s_playing = reported_playing;
            }
        }
        taskEXIT_CRITICAL();

        if (!apply_state) {
            return;
        }
    }

    lvgl_phone_apply_event(event);
    /* 通话悬浮控件与当前页无关：来电/去电/通话中自动出现，结束自动消失。 */
    lvgl_call_apply_event(event);

    if (s_active_page != LVGL_PAGE_MUSIC) {
        return;
    }

    switch (event->type) {
    case BT_UI_EVENT_COVER_ART_DATA:
    case BT_UI_EVENT_COVER_ART_DONE:
    case BT_UI_EVENT_COVER_ART_ERROR:
        lvgl_bt_cover_receive_event(event);
        return;
    case BT_UI_EVENT_CONN_STATE:
    case BT_UI_EVENT_A2DP_STATE:
        /* Connection state is intentionally not shown in the minimal page. */
        break;
    case BT_UI_EVENT_PLAY_STATE:
        if (s_btn_play != NULL) {
            lv_label_set_text(lv_obj_get_child(s_btn_play, 0),
                              s_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        }
        break;
    case BT_UI_EVENT_TRACK_INFO:
        if (s_ui_title != NULL) lv_label_set_text(s_ui_title, s_music_title);
        if (s_ui_artist != NULL) lv_label_set_text(s_ui_artist, s_music_artist);
        break;
    case BT_UI_EVENT_ABSOLUTE_VOLUME:
        break;
    default:
        break;
    }
}

static void lvgl_bt_music_process_events(void)
{
    bt_ui_event_t event;
    INT8U have_event;
    INT8U processed = 0u;

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
            processed++;
        }
    } while (have_event && (processed < LVGL_BT_EVENTS_PER_FRAME));
}

static void lvgl_bt_ui_notify(const bt_ui_event_t *event, void *context)
{
    INT8U next_head;
    INT8U queue_full;

    (void)context;
    if (event == NULL) {
        return;
    }

    switch (event->type) {
    case BT_UI_EVENT_CONN_STATE:
    case BT_UI_EVENT_A2DP_STATE:
    case BT_UI_EVENT_PLAY_STATE:
    case BT_UI_EVENT_PLAY_REQUESTED:
    case BT_UI_EVENT_TRACK_INFO:
    case BT_UI_EVENT_ABSOLUTE_VOLUME:
    case BT_UI_EVENT_HFP_STATE:
    case BT_UI_EVENT_CALL_NUMBER:
    case BT_UI_EVENT_RING:
    case BT_UI_EVENT_MIC_MUTE:
    case BT_UI_EVENT_PB_STARTED:
    case BT_UI_EVENT_PB_CONTACT:
    case BT_UI_EVENT_PB_CALL_LOG:
    case BT_UI_EVENT_PB_DONE:
    case BT_UI_EVENT_PB_AUTH:
    case BT_UI_EVENT_PB_ERROR:
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
    queue_full = (next_head == s_bt_event_tail);
    if (!queue_full || (event->type == BT_UI_EVENT_PLAY_STATE) ||
        (event->type == BT_UI_EVENT_PLAY_REQUESTED) ||
        (event->type == BT_UI_EVENT_TRACK_INFO) ||
        (event->type == BT_UI_EVENT_A2DP_STATE) ||
        (event->type == BT_UI_EVENT_PB_DONE) ||
        (event->type == BT_UI_EVENT_PB_AUTH) ||
        (event->type == BT_UI_EVENT_PB_ERROR)) {
        /* Playback and completion/error events are state transitions.
         * Preserve them even during a burst of hundreds of call-log entries. */
        if (queue_full) {
            s_bt_event_tail = (INT8U)((s_bt_event_tail + 1u) %
                                      LVGL_BT_EVENT_QUEUE_LEN);
        }
        s_bt_event_queue[s_bt_event_head] = *event;
        if (event->type == BT_UI_EVENT_PLAY_STATE &&
            (event->value_count == 0u || event->values[1] != 1u)) {
            s_bt_event_queue[s_bt_event_head].values[3] = ++s_play_event_sequence;
        }
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
    DBG_PRINT("[LVGL] task started\r\n");

#if (LVGL_UI_SPIFC_PROBE == 1)
    /* Let WiFi/BT finish their verbose start-up log first, otherwise the
     * probe's capacity line is interleaved on the shared UART. */
    vTaskDelay(3000);
    lvgl_spifc_probe();
#endif
#if (LVGL_UI_SPIFC_SCAN == 1)
    vTaskDelay(3000);
    lvgl_spifc_scan_blank_region();
#endif

    /* 1. LVGL 核心初始化 */
    lv_init();
    lvgl_nand_fs_register();
    LVGL_RUNTIME_LOG("[LVGL] U3 resource drive U: registered\r\n");
#if (LVGL_UI_SPIFC_INSTALLER == 1)
    lvgl_spui_install_from_sd();
#endif
    /* 2. 分配显示缓冲（DDR） */
    fb_size = LVGL_H_RES * LVGL_V_RES * sizeof(lv_color_t);
    draw_buf_size = LVGL_H_RES * (LVGL_V_RES / LVGL_DRAW_BUF_DIV) * sizeof(lv_color_t);
    s_lvgl_fb = (INT32U)gp_malloc_align(fb_size, 64);
    s_draw_buf_mem = (lv_color_t *)gp_malloc_align(draw_buf_size, 64);
    s_cover_jpeg = (uint8_t *)gp_malloc_align(LVGL_COVER_JPEG_MAX_BYTES, 64);
    s_cover_decode_jpeg = (uint8_t *)gp_malloc_align(LVGL_COVER_JPEG_MAX_BYTES, 64);
    cover_rgb_size = LVGL_COVER_SIZE * LVGL_COVER_SIZE * sizeof(lv_color_t);
    s_cover_rgb565 = (lv_color_t *)gp_malloc_align(cover_rgb_size, 64);
    if ((s_lvgl_fb == 0) || (s_draw_buf_mem == 0) ||
        (s_cover_jpeg == NULL) || (s_cover_decode_jpeg == NULL) ||
        (s_cover_rgb565 == NULL)) {
        ERR_PRINT("[LVGL] fb alloc fail (fb=0x%x buf=0x%x)\r\n",
                  s_lvgl_fb, (INT32U)s_draw_buf_mem);
        return;
    }
    memset((void *)s_lvgl_fb, 0x00, fb_size);       /* 初始全黑 */
    cache_invalid_range(s_lvgl_fb, fb_size);        /* 刷回 DDR */
    LVGL_RUNTIME_LOG("[LVGL] fb=0x%x(%dKB) drawbuf=0x%x(%dKB)\r\n",
                     s_lvgl_fb, fb_size / 1024,
                     (INT32U)s_draw_buf_mem, draw_buf_size / 1024);

#if (C_LVGL_UI_RAM_CACHE == CUSTOM_ON)
    /* Installers have finished and mandatory display/cover buffers already
     * exist. Cache failure leaves the original read-only Flash path usable. */
    lvgl_spui_ram_cache_init();
#endif

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
    LVGL_RUNTIME_LOG("[LVGL] display path -> LVGL\r\n");

    /* 6. 默认进入仪表页；物理页面键由外围任务转发。 */
    lvgl_dashboard_ui_create(lv_scr_act());
    lv_obj_invalidate(lv_scr_act()); /* 首帧完整覆盖此前显示路径遗留的内容 */
    bt_ui_set_notify(lvgl_bt_ui_notify, NULL);

    /* 7. 主循环：收消息（蓝牙事件/1 秒定时）+ 驱动 LVGL 渲染
       （LVGL tick 由 FreeRTOS tick hook -> lvgl_tick_inc() 每 1ms 喂，这里只跑 timer）*/
    while (1) {
        if (msgQReceive_timeout(LVGLTaskQ, &msg_id, &msg_para, 0, 5) == 0) {
            switch (msg_id) {
            case MSG_LVGL_1SEC:
                break;
            case MSG_LVGL_SHOW_MUSIC:
                s_page_switch_pending = 0u;
                lvgl_show_music_page();
                break;
            case MSG_LVGL_SHOW_PHONE:
                s_page_switch_pending = 0u;
                lvgl_show_phone_page();
                break;
            case MSG_LVGL_SHOW_DASHBOARD:
                s_page_switch_pending = 0u;
                lvgl_show_dashboard_page();
                break;
            case MSG_LVGL_PHONE_KEY_UP:
                lvgl_nav_key_move(LVGL_PHONE_KEY_UP);
                break;
            case MSG_LVGL_PHONE_KEY_DOWN:
                lvgl_nav_key_move(LVGL_PHONE_KEY_DOWN);
                break;
            case MSG_LVGL_PHONE_KEY_LEFT:
                lvgl_nav_key_move(LVGL_PHONE_KEY_LEFT);
                break;
            case MSG_LVGL_PHONE_KEY_RIGHT:
                lvgl_nav_key_move(LVGL_PHONE_KEY_RIGHT);
                break;
            case MSG_LVGL_PHONE_KEY_SELECT:
                lvgl_nav_key_move(LVGL_PHONE_KEY_SELECT);
                break;
            default:
                break;
            }
        }

        lvgl_bt_music_process_events();
        lvgl_bt_music_expire_pending_state();
        lvgl_bt_cover_decode_if_ready();
        lv_timer_handler();

    }
}
#endif /* (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT) */

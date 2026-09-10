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
#define LVGL_BT_EVENT_QUEUE_LEN 8u
#define LVGL_COVER_JPEG_MAX_BYTES (64u * 1024u)
#define LVGL_COVER_SIZE       240u
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
static INT8U     s_playing = 0;
static lv_color_t *s_dashboard_snapshot;
static INT8U     s_dashboard_snapshot_valid;

/* Cover Art 数据只在 xstack 回调内拷贝；JPEG 解码及 lv_img 更新均在 LVGL 任务内完成。 */
static uint8_t *s_cover_jpeg;
static lv_color_t *s_cover_rgb565;
static uint32_t s_cover_jpeg_size;
static volatile uint8_t s_cover_receiving;
static volatile uint8_t s_cover_ready;
static volatile uint8_t s_cover_decoding;
static lv_img_dsc_t s_cover_img;

/* Board port of tools/lvgl_pc_simulator/src/ui_dashboard.c.  All static
 * visual layers stay as supplied materials in U3; LVGL only supplies the
 * same speed arc, number and warning animation used by the PC simulator. */
typedef struct {
    const char *file;
    lv_coord_t x;
    lv_coord_t y;
} lvgl_dashboard_asset_t;

static lv_obj_t *s_dashboard_speed_label;
static lv_obj_t *s_dashboard_main_arc;
static lv_obj_t *s_dashboard_warning;
static INT32S s_dashboard_speed_tenths = 1080;
static INT32S s_dashboard_speed_direction = 1;
static INT16U s_dashboard_warning_elapsed;
static volatile INT8U s_dashboard_active;
static volatile INT8U s_page_switch_pending;
static lv_timer_t *s_dashboard_timer;
static char s_music_title[BT_UI_TEXT_MAX] = "No Track";
static char s_music_artist[BT_UI_TEXT_MAX] = "Bluetooth device";
static char s_music_album[BT_UI_TEXT_MAX];

/* U: is a read-only LVGL file-system drive backed by the raw U3 SPI-NOR
 * partition. Resource files are converted LVGL v8 binary images, e.g.
 * "U:/001.bin". No image data is linked into the firmware. */
typedef struct {
    INT32U data_addr;
    INT32U size;
    INT32U position;
} lvgl_nand_file_t;

static lv_fs_drv_t s_nand_fs_drv;
static INT8U s_spui_ready;

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

static BOOLEAN lvgl_nand_fs_select(void)
{
    lvgl_spui_header_t header;

    if (s_spui_ready == 0u) {
        if (SPIFC_Flash_read_page(LVGL_SPUI_BASE, (INT8U *)&header,
                                  sizeof(header)) != 0) {
            return FALSE;
        }
        if ((header.magic != LVGL_SPUI_MAGIC) ||
            (header.header_size != sizeof(header)) ||
            (header.file_count == 0u) ||
            (header.file_count > LVGL_UI_INSTALL_MAX_FILES) ||
            (header.image_size > (LVGL_SPUI_LIMIT - LVGL_SPUI_BASE)) ||
            (header.image_size < (sizeof(header) +
                                  ((INT32U)header.file_count * sizeof(lvgl_spui_entry_t))))) {
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
        (SPIFC_Flash_read_page(LVGL_SPUI_BASE, (INT8U *)&header,
                               sizeof(header)) != 0)) {
        return FALSE;
    }
    for (i = 0u; i < header.file_count; i++) {
        if (lvgl_spui_read(table_addr + ((INT32U)i * sizeof(entry)),
                           (INT8U *)&entry, sizeof(entry)) != 0) {
            return FALSE;
        }
        if (strncmp(entry.name, name, LVGL_UI_INSTALL_NAME_MAX) == 0) {
            if ((entry.offset < header.header_size) ||
                (entry.offset + entry.size > header.image_size)) {
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
        DBG_PRINT("[U3 install] initialise SD\r\n");
        FS_OS_LOCK();
        result = drvl2_sd_init(STORAGE_SD_DEV);
        FS_OS_UNLOCK();
        if (result < 0) {
            ERR_PRINT("[U3 install] SD init failed (%d)\r\n", result);
            return FALSE;
        }
        result = _devicemount(FS_SD1);
        if (result < 0) {
            ERR_PRINT("[U3 install] SD mount failed (%d)\r\n", result);
            return FALSE;
        }
        s_ui_sd_mounted = 1u;
    }
    return (fs_changedisk(FS_SD1) >= 0) ? TRUE : FALSE;
}

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
        LVGL_RUNTIME_LOG("[LVGL] AVRCP play/pause status=%d\r\n", status);
        if (status != BT_XSTACK_OK) {
            s_playing = s_playing ? 0u : 1u;
            lv_label_set_text(lv_obj_get_child(s_btn_play, 0),
                              s_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        }
    } else if (btn == s_btn_prev) {
        status = bt_xstack_avrcp_previous();
        LVGL_RUNTIME_LOG("[LVGL] AVRCP previous status=%d\r\n", status);
    } else if (btn == s_btn_next) {
        status = bt_xstack_avrcp_next();
        LVGL_RUNTIME_LOG("[LVGL] AVRCP next status=%d\r\n", status);
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
        LVGL_RUNTIME_LOG("[LVGL] cover received %u bytes\r\n",
                         (unsigned int)s_cover_jpeg_size);
    }
}

static void lvgl_bt_cover_decode_if_ready(void)
{
    IMAGE_DECODE_STRUCT image;
    uint32_t jpeg_size;

    if (s_dashboard_active || (!s_cover_ready) || (s_cover_jpeg == NULL) ||
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
        ERR_PRINT("[LVGL] cover JPEG decode failed\r\n");
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
    LVGL_RUNTIME_LOG("[LVGL] cover displayed %ux%u\r\n",
                     (unsigned int)LVGL_COVER_SIZE,
                     (unsigned int)LVGL_COVER_SIZE);
    s_cover_decoding = 0u;
}

/* Keep this list byte-for-byte aligned with the PC simulator's static
 * material list.  The only difference is the U: raw SPI-NOR file system
 * prefix and the converted LVGL-v8 .bin extension. */
static const lvgl_dashboard_asset_t s_dashboard_static_assets[] = {
    { "U:/001.bin", 0, 12 },
    { "U:/003.bin", 46, 87 },
    { "U:/015.bin", 46, 233 },
    { "U:/021.bin", 734, 431 },
    { "U:/t40.bin", 345, 278 },
    { "U:/t41.bin", 346, 169 },
    { "U:/t42.bin", 268, 170 },
    { "U:/t46.bin", 418, 437 },
    { "U:/t47.bin", 420, 446 },
    { "U:/t48.bin", 305, 272 },
    { "U:/w51.bin", 743, 139 },
    { "U:/052.bin", 139, 139 },
    { "U:/m60.bin", 361, 151 },
    { "U:/m61.bin", 405, 310 },
    { "U:/m62.bin", 404, 55 },
    { "U:/m64.bin", 546, 158 },
    { "U:/067.bin", 481, 200 },
    { "U:/070.bin", 574, 99 },
    { "U:/075.bin", 364, 98 },
    { "U:/b80.bin", 611, 171 },
    { "U:/b81.bin", 610, 217 },
    { "U:/b82.bin", 677, 227 },
    { "U:/b84.bin", 669, 211 },
    { "U:/b85.bin", 601, 443 },
    { "U:/b86.bin", 603, 446 },
    { "U:/b87.bin", 683, 207 },
    { "U:/088.bin", 714, 250 },
    { "U:/095.bin", 436, 225 },
    { "U:/m65.bin", 478, 395 },
    { "U:/m66.bin", 526, 395 },
    { "U:/s99.bin", 496, 325 },
    { "U:/100.bin", 463, 94 },
    { "U:/105.bin", 747, 192 },
    { "U:/i35.bin", 375, 472 },
    { "U:/i33.bin", 421, 472 },
    { "U:/i37.bin", 466, 472 },
    { "U:/i36.bin", 557, 472 },
    { "U:/i34.bin", 603, 473 },
    { "U:/i38.bin", 650, 472 }
};

static lv_obj_t *lvgl_dashboard_add_image(lv_obj_t *parent, const char *file,
                                          lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *image = lv_img_create(parent);

    lv_img_set_src(image, file);
    lv_obj_set_pos(image, x, y);
    return image;
}

/* The supplied battery material remains an image.  Its transparent parent is
 * only a crop window, exactly as in the PC simulation. */
static lv_obj_t *lvgl_dashboard_add_progress_image(lv_obj_t *parent, const char *file,
                                                    lv_coord_t x, lv_coord_t y,
                                                    lv_coord_t width, lv_coord_t height,
                                                    INT8U percent)
{
    lv_coord_t visible_height = (lv_coord_t)((height * percent + 99u) / 100u);
    lv_obj_t *clip = lv_obj_create(parent);
    lv_obj_t *image;

    lv_obj_set_pos(clip, x, y);
    lv_obj_set_size(clip, width, visible_height);
    lv_obj_set_style_bg_opa(clip, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(clip, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(clip, 0, LV_PART_MAIN);
    lv_obj_clear_flag(clip, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    image = lv_img_create(clip);
    lv_img_set_src(image, file);
    lv_obj_set_pos(image, 0, 0);
    return image;
}

static void lvgl_dashboard_tick(lv_timer_t *timer)
{
    INT32S displayed_speed;

    (void)timer;
    if (!s_dashboard_active) {
        return;
    }
    s_dashboard_speed_tenths += 5 * s_dashboard_speed_direction;
    if (s_dashboard_speed_tenths >= 1440) {
        s_dashboard_speed_direction = -1;
    }
    if (s_dashboard_speed_tenths <= 680) {
        s_dashboard_speed_direction = 1;
    }

    lv_arc_set_value(s_dashboard_main_arc, s_dashboard_speed_tenths);
    displayed_speed = s_dashboard_speed_tenths / 10;
    lv_label_set_text_fmt(s_dashboard_speed_label, "%d", displayed_speed);

    s_dashboard_warning_elapsed = (INT16U)(s_dashboard_warning_elapsed + 16u);
    if (s_dashboard_warning_elapsed >= 120u) {
        s_dashboard_warning_elapsed = 0u;
        if (lv_obj_has_flag(s_dashboard_warning, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(s_dashboard_warning, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_dashboard_warning, LV_OBJ_FLAG_HIDDEN);
        }
    }

}

static void lvgl_dashboard_ui_create(lv_obj_t *screen)
{
    INT32U i;

    s_dashboard_active = 1u;
    lv_group_remove_all_objs(s_group);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000811), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    for (i = 0u; i < (sizeof(s_dashboard_static_assets) /
                       sizeof(s_dashboard_static_assets[0])); i++) {
        lvgl_dashboard_add_image(screen, s_dashboard_static_assets[i].file,
                                 s_dashboard_static_assets[i].x,
                                 s_dashboard_static_assets[i].y);
    }

    lvgl_dashboard_add_image(screen, "U:/t43.bin", 271, 322);
    lvgl_dashboard_add_image(screen, "U:/t45.bin", 330, 274);
    lvgl_dashboard_add_progress_image(screen, "U:/b83.bin", 677, 289, 20, 86, 83u);
    s_dashboard_warning = lvgl_dashboard_add_image(screen, "U:/w50.bin", 729, 137);

    s_dashboard_main_arc = lv_arc_create(screen);
    lv_obj_set_size(s_dashboard_main_arc, 282, 282);
    lv_obj_set_pos(s_dashboard_main_arc, 370, 161);
    lv_obj_set_style_bg_opa(s_dashboard_main_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_dashboard_main_arc, 14, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_dashboard_main_arc, lv_color_hex(0x1A3241), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_dashboard_main_arc, 14, LV_PART_INDICATOR);
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
    lv_obj_set_pos(s_dashboard_speed_label, 466, 265);
    lvgl_dashboard_add_image(screen, "U:/i32.bin", 500, 473);

    if (s_dashboard_timer == NULL) {
        /* Dashboard motion is decorative.  A 100 ms cadence leaves CPU time
         * for physical key scanning and page creation. */
        s_dashboard_timer = lv_timer_create(lvgl_dashboard_tick, 100, NULL);
    }
    LVGL_RUNTIME_LOG("[LVGL] simulator dashboard: %d U3 images\r\n",
                     (INT32U)(sizeof(s_dashboard_static_assets) /
                              sizeof(s_dashboard_static_assets[0])) + 5u);
}

static void lvgl_bt_music_ui_create(lv_obj_t *scr)
{
    lv_obj_t *lbl;
    lv_obj_t *cover_frame;

    /* 保留网易云深色与品牌红，但只呈现播放器本身。 */
    s_dashboard_active = 0u;
    lv_group_remove_all_objs(s_group);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x171719), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x2A171A), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    cover_frame = lv_obj_create(scr);
    lv_obj_set_size(cover_frame, 300, 300);
    lv_obj_set_style_bg_color(cover_frame, lv_color_hex(0x2C2527), 0);
    lv_obj_set_style_bg_opa(cover_frame, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(cover_frame, lv_color_hex(0xEC4141), 0);
    lv_obj_set_style_border_width(cover_frame, 1, 0);
    lv_obj_set_style_radius(cover_frame, 18, 0);
    lv_obj_set_style_shadow_color(cover_frame, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_width(cover_frame, 14, 0);
    lv_obj_set_style_shadow_opa(cover_frame, LV_OPA_40, 0);
    lv_obj_clear_flag(cover_frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(cover_frame, LV_ALIGN_TOP_MID, 0, 56);

    /* 曲目名 */
    s_ui_title = lv_label_create(scr);
    lv_label_set_text(s_ui_title, s_music_title);
    lv_obj_set_style_text_font(s_ui_title, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(s_ui_title, lv_color_white(), 0);
    lv_obj_set_width(s_ui_title, 640);
    lv_label_set_long_mode(s_ui_title, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(s_ui_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_ui_title, LV_ALIGN_TOP_MID, 0, 382);

    /* 艺术家 */
    s_ui_artist = lv_label_create(scr);
    lv_label_set_text(s_ui_artist, s_music_artist);
    lv_obj_set_style_text_font(s_ui_artist, &lv_font_cn_18, 0);
    lv_obj_set_style_text_color(s_ui_artist, lv_color_hex(0xB8B8BC), 0);
    lv_obj_set_width(s_ui_artist, 640);
    lv_label_set_long_mode(s_ui_artist, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(s_ui_artist, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_ui_artist, LV_ALIGN_TOP_MID, 0, 424);

    s_ui_cover = lv_img_create(cover_frame);
    lv_obj_set_size(s_ui_cover, LVGL_COVER_SIZE, LVGL_COVER_SIZE);
    lv_obj_set_style_bg_color(s_ui_cover, lv_color_hex(0x342426), 0);
    lv_obj_set_style_bg_opa(s_ui_cover, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_ui_cover, 12, 0);
    lv_obj_center(s_ui_cover);

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
    lv_obj_align(s_btn_prev, LV_ALIGN_BOTTOM_MID, -112, -46);

    s_btn_play = lv_btn_create(scr);
    lv_obj_set_size(s_btn_play, 88, 88);
    lv_obj_set_style_bg_color(s_btn_play, lv_color_hex(0xEC4141), 0);
    lv_obj_set_style_bg_opa(s_btn_play, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_btn_play, 0, 0);
    lv_obj_set_style_radius(s_btn_play, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_color(s_btn_play, lv_color_hex(0xB52C39), 0);
    lv_obj_set_style_shadow_width(s_btn_play, 14, 0);
    lv_obj_set_style_shadow_opa(s_btn_play, LV_OPA_60, 0);
    lbl = lv_label_create(s_btn_play);
    lv_label_set_text(lbl, s_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);
    lv_obj_align(s_btn_play, LV_ALIGN_BOTTOM_MID, 0, -38);

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
    lv_obj_align(s_btn_next, LV_ALIGN_BOTTOM_MID, 112, -46);

    lv_obj_add_event_cb(s_btn_play, lvgl_bt_music_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_prev, lvgl_bt_music_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_next, lvgl_bt_music_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 按键导航：PRE/NEXT 移动焦点，OK(ENTER) 触发点击 */
    lv_group_add_obj(s_group, s_btn_prev);
    lv_group_add_obj(s_group, s_btn_play);
    lv_group_add_obj(s_group, s_btn_next);
    lv_group_focus_obj(s_btn_play);
}

static void lvgl_show_music_page(void)
{
    lv_obj_t *screen;

    if (!s_dashboard_active) {
        return;
    }
    if (s_dashboard_snapshot != NULL) {
        memcpy(s_dashboard_snapshot, (void *)s_lvgl_fb,
               LVGL_H_RES * LVGL_V_RES * sizeof(lv_color_t));
        s_dashboard_snapshot_valid = 1u;
    }
    s_dashboard_active = 0u;
    screen = lv_obj_create(NULL);
    lvgl_bt_music_ui_create(screen);
    /* Immediate load with auto-delete.  lv_scr_load() keeps every previous
     * root screen alive; this form releases it as soon as the new page is
     * active, preventing switch latency from growing over time. */
    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_NONE, 0u, 0u, true);
    lv_obj_invalidate(screen);
}

static void lvgl_show_dashboard_page(void)
{
    lv_obj_t *screen;

    if (s_dashboard_active) {
        return;
    }
    screen = lv_obj_create(NULL);
    lvgl_dashboard_ui_create(screen);
    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_NONE, 0u, 0u, true);
    if ((s_dashboard_snapshot != NULL) && (s_dashboard_snapshot_valid != 0u)) {
        memcpy((void *)s_lvgl_fb, s_dashboard_snapshot,
               LVGL_H_RES * LVGL_V_RES * sizeof(lv_color_t));
        cache_invalid_range(s_lvgl_fb,
                            LVGL_H_RES * LVGL_V_RES * sizeof(lv_color_t));
    }
    lv_obj_invalidate(screen);
}

int lvgl_page_show_music(void)
{
    if ((s_dashboard_active == 0u) || (LVGLTaskQ == 0)) {
        DBG_PRINT("[LVGL] KEY6 ignored (dashboard=%u queue=0x%x)\r\n",
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
        DBG_PRINT("[LVGL] KEY6 page request failed\r\n");
        return 0;
    }
    DBG_PRINT("[LVGL] KEY6 -> music page\r\n");
    return 1;
}

int lvgl_page_show_dashboard(void)
{
    if ((s_dashboard_active != 0u) || (LVGLTaskQ == 0)) {
        DBG_PRINT("[LVGL] KEY5 ignored (dashboard=%u queue=0x%x)\r\n",
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
        DBG_PRINT("[LVGL] KEY5 page request failed\r\n");
        return 0;
    }
    DBG_PRINT("[LVGL] KEY5 -> dashboard page\r\n");
    return 1;
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
    } else if (event->type == BT_UI_EVENT_PLAY_STATE) {
        s_playing = (event->values[0] == BT_XSTACK_PLAY_STATE_PLAYING) ? 1u : 0u;
    }

    if (s_dashboard_active) {
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
        lv_label_set_text(lv_obj_get_child(s_btn_play, 0),
                          s_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        break;
    case BT_UI_EVENT_TRACK_INFO:
        lv_label_set_text(s_ui_title, s_music_title);
        lv_label_set_text(s_ui_artist, s_music_artist);
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
	if (s_dashboard_active &&
		((event->type == BT_UI_EVENT_COVER_ART_DATA) ||
		 (event->type == BT_UI_EVENT_COVER_ART_DONE) ||
		 (event->type == BT_UI_EVENT_COVER_ART_ERROR))) {
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
    s_dashboard_snapshot = (lv_color_t *)gp_malloc_align(fb_size, 64);
    s_cover_jpeg = (uint8_t *)gp_malloc_align(LVGL_COVER_JPEG_MAX_BYTES, 64);
    cover_rgb_size = LVGL_COVER_SIZE * LVGL_COVER_SIZE * sizeof(lv_color_t);
    s_cover_rgb565 = (lv_color_t *)gp_malloc_align(cover_rgb_size, 64);
    if ((s_lvgl_fb == 0) || (s_draw_buf_mem == 0) ||
        (s_dashboard_snapshot == NULL) ||
        (s_cover_jpeg == NULL) || (s_cover_rgb565 == NULL)) {
        ERR_PRINT("[LVGL] fb alloc fail (fb=0x%x buf=0x%x)\r\n",
                  s_lvgl_fb, (INT32U)s_draw_buf_mem);
        return;
    }
    memset((void *)s_lvgl_fb, 0x00, fb_size);       /* 初始全黑 */
    cache_invalid_range(s_lvgl_fb, fb_size);        /* 刷回 DDR */
    LVGL_RUNTIME_LOG("[LVGL] fb=0x%x(%dKB) drawbuf=0x%x(%dKB)\r\n",
                     s_lvgl_fb, fb_size / 1024,
                     (INT32U)s_draw_buf_mem, draw_buf_size / 1024);

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

    /* 6. 默认进入仪表页；KEY6 右移到蓝牙音乐页，KEY5 左移返回。 */
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
            case MSG_LVGL_SHOW_DASHBOARD:
                s_page_switch_pending = 0u;
                lvgl_show_dashboard_page();
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

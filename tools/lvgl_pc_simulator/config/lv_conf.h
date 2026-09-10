/*
 * PC simulator-only LVGL configuration overlay.
 * Keep the board configuration in Project/LVGL/lv_conf.h unchanged.
 */
#include "../../../Project/LVGL/lv_conf.h"

/* Runtime PNG decoding needs a buffer larger than the board's 1 MiB pool.
 * This is only visible to the PC simulator. */
#undef LV_MEM_SIZE
#define LV_MEM_SIZE (8U * 1024U * 1024U)

#undef LV_FONT_MONTSERRAT_20
#define LV_FONT_MONTSERRAT_20 1
#undef LV_FONT_MONTSERRAT_32
#define LV_FONT_MONTSERRAT_32 1
#undef LV_FONT_MONTSERRAT_48
#define LV_FONT_MONTSERRAT_48 1

#undef LV_USE_FS_STDIO
#define LV_USE_FS_STDIO 1
/* LVGL needs a leading driver letter, while lodepng needs a native Windows
 * filename.  With `D:` + `D:` the path `D:/.../001.png` works for both. */
#define LV_FS_STDIO_LETTER 'D'
#define LV_FS_STDIO_PATH "D:"
#define LV_FS_STDIO_CACHE_SIZE 0

#undef LV_USE_PNG
#define LV_USE_PNG 1

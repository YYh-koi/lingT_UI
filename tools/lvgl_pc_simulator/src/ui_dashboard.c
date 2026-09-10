#include "ui_dashboard.h"

#include "lvgl.h"

typedef struct {
    const char *file;
    lv_coord_t x;
    lv_coord_t y;
} ui_asset_t;

static lv_obj_t *s_speed_label;
static lv_obj_t *s_main_arc;
static lv_obj_t *s_warning;
static int32_t s_speed_tenths = 1080;
static int32_t s_speed_direction = 1;
static uint16_t s_warning_elapsed;

/* Static material layers. Their order follows the PSD layer stack. */
static const ui_asset_t s_static_assets[] = {
    { UI_ASSET_DIR "001.png", 0, 12 },
    { UI_ASSET_DIR "003.png", 46, 87 },
    { UI_ASSET_DIR "015.png", 46, 233 },
    { UI_ASSET_DIR "021.png", 734, 431 },
    { UI_ASSET_DIR "t40.png", 345, 278 },
    { UI_ASSET_DIR "t41.png", 346, 169 },
    { UI_ASSET_DIR "t42.png", 268, 170 },
    { UI_ASSET_DIR "t46.png", 418, 437 },
    { UI_ASSET_DIR "t47.png", 420, 446 },
    { UI_ASSET_DIR "t48.png", 305, 272 },
    { UI_ASSET_DIR "w51.png", 743, 139 },
    { UI_ASSET_DIR "052.png", 139, 139 },
    { UI_ASSET_DIR "m60.png", 361, 151 },
    { UI_ASSET_DIR "m61.png", 405, 310 },
    { UI_ASSET_DIR "m62.png", 404, 55 },
    { UI_ASSET_DIR "m64.png", 546, 158 },
    { UI_ASSET_DIR "067.png", 481, 200 },
    { UI_ASSET_DIR "070.png", 574, 99 },
    { UI_ASSET_DIR "075.png", 364, 98 },
    { UI_ASSET_DIR "b80.png", 611, 171 },
    { UI_ASSET_DIR "b81.png", 610, 217 },
    { UI_ASSET_DIR "b82.png", 677, 227 },
    { UI_ASSET_DIR "b84.png", 669, 211 },
    { UI_ASSET_DIR "b85.png", 601, 443 },
    { UI_ASSET_DIR "b86.png", 603, 446 },
    { UI_ASSET_DIR "b87.png", 683, 207 },
    { UI_ASSET_DIR "088.png", 714, 250 },
    { UI_ASSET_DIR "095.png", 436, 225 },
    { UI_ASSET_DIR "m65.png", 478, 395 },
    { UI_ASSET_DIR "m66.png", 526, 395 },
    { UI_ASSET_DIR "s99.png", 496, 325 },
    { UI_ASSET_DIR "100.png", 463, 94 },
    { UI_ASSET_DIR "105.png", 747, 192 },
    { UI_ASSET_DIR "i35.png", 375, 472 },
    { UI_ASSET_DIR "i33.png", 421, 472 },
    { UI_ASSET_DIR "i37.png", 466, 472 },
    { UI_ASSET_DIR "i36.png", 557, 472 },
    { UI_ASSET_DIR "i34.png", 603, 473 },
    { UI_ASSET_DIR "i38.png", 650, 472 },
};

static lv_obj_t *add_image(const char *file, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *image = lv_img_create(lv_scr_act());
    lv_img_set_src(image, file);
    lv_obj_set_pos(image, x, y);
    return image;
}

/* Keep the image material intact and use its parent only as a crop window.
 * This lets the supplied dashed-PNG represent a percentage without redrawing it. */
static lv_obj_t *add_progress_image(const char *file, lv_coord_t x, lv_coord_t y,
                                    lv_coord_t width, lv_coord_t height,
                                    uint8_t percent)
{
    lv_coord_t visible_height = (lv_coord_t)((height * percent + 99U) / 100U);
    lv_obj_t *clip = lv_obj_create(lv_scr_act());
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

static void dashboard_tick(lv_timer_t *timer)
{
    int32_t displayed_speed;
    (void)timer;

    s_speed_tenths += 5 * s_speed_direction;
    if(s_speed_tenths >= 1440) s_speed_direction = -1;
    if(s_speed_tenths <= 680) s_speed_direction = 1;

    /* The ring receives 0.1 km/h steps at ~60 FPS; the visible number keeps
     * the original whole-kilometre presentation. */
    lv_arc_set_value(s_main_arc, s_speed_tenths);
    displayed_speed = s_speed_tenths / 10;
    lv_label_set_text_fmt(s_speed_label, "%d", displayed_speed);

    s_warning_elapsed = (uint16_t)(s_warning_elapsed + 16U);
    if(s_warning_elapsed >= 120U) {
        s_warning_elapsed = 0;
        if(lv_obj_has_flag(s_warning, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(s_warning, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(s_warning, LV_OBJ_FLAG_HIDDEN);
        }
    }

}

void ui_dashboard_create(void)
{
    uint32_t i;

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000811), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);

    for(i = 0; i < sizeof(s_static_assets) / sizeof(s_static_assets[0]); i++) {
        add_image(s_static_assets[i].file, s_static_assets[i].x, s_static_assets[i].y);
    }

    /* Keep the complete supplied cyan dashed material.  Cropping it at the
     * temperature value visibly cut its lower endpoint. */
    add_image(UI_ASSET_DIR "t43.png", 271, 322);
    add_image(UI_ASSET_DIR "t45.png", 330, 274);

    /* Battery is assembled from its original layers so 083 can be a true
     * 83% progress image instead of part of a static combined PNG. */
    add_progress_image(UI_ASSET_DIR "b83.png", 677, 289, 20, 86, 83);
    s_warning = add_image(UI_ASSET_DIR "w50.png", 729, 137);

    /* A transparent LVGL arc supplies only the value-dependent progress.
     * The base disc, lights, marker and all other visual layers are PNG materials. */
    s_main_arc = lv_arc_create(lv_scr_act());
    lv_obj_set_size(s_main_arc, 282, 282);
    lv_obj_set_pos(s_main_arc, 370, 161);
    lv_obj_set_style_bg_opa(s_main_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_main_arc, 14, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_main_arc, lv_color_hex(0x1A3241), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_main_arc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_main_arc, lv_color_hex(0x10DDE5), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(s_main_arc, true, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_main_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_main_arc, 0, LV_PART_MAIN);
    lv_arc_set_range(s_main_arc, 0, 1600);
    lv_arc_set_rotation(s_main_arc, 135);
    lv_arc_set_bg_angles(s_main_arc, 0, 270);
    lv_arc_set_value(s_main_arc, s_speed_tenths);
    lv_obj_clear_flag(s_main_arc, LV_OBJ_FLAG_CLICKABLE);

    s_speed_label = lv_label_create(lv_scr_act());
    lv_label_set_text(s_speed_label, "108");
    lv_obj_set_style_text_font(s_speed_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_speed_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(s_speed_label, 466, 265);

    add_image(UI_ASSET_DIR "i32.png", 500, 473);
    lv_timer_create(dashboard_tick, 16, NULL);
}

void ui_dashboard_set_track(const char *title, const char *artist, bool playing)
{
	(void)title;
    (void)artist;
	(void)playing;
}

void ui_dashboard_set_call(const char *number, bool ringing, bool active)
{
    (void)number;
    (void)ringing;
    (void)active;
}

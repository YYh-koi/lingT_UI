#include <stdbool.h>
#include <stdint.h>

#include <SDL.h>
#include "lvgl.h"
#include "lvgl/src/extra/libs/fsdrv/lv_fsdrv.h"
#include "lvgl/src/extra/libs/png/lv_png.h"
#include "ui_dashboard.h"

#define SIM_HOR_RES 1024
#define SIM_VER_RES 600
#define SIM_DRAW_LINES 80

static SDL_Window *s_window;
static SDL_Renderer *s_renderer;
static SDL_Texture *s_texture;
static lv_disp_draw_buf_t s_draw_buf;
static lv_color_t s_draw_buf_mem[SIM_HOR_RES * SIM_DRAW_LINES];
static uint8_t s_rgb_frame[SIM_HOR_RES * SIM_VER_RES * 3];

static void sim_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area,
                         lv_color_t *color_p)
{
    SDL_Rect rect;
    int x;
    int y;

    (void)drv;
    rect.x = area->x1;
    rect.y = area->y1;
    rect.w = lv_area_get_width(area);
    rect.h = lv_area_get_height(area);
    for(y = 0; y < rect.h; y++) {
        uint8_t *dst = &s_rgb_frame[((rect.y + y) * SIM_HOR_RES + rect.x) * 3];
        for(x = 0; x < rect.w; x++) {
            lv_color_t pixel = color_p[y * rect.w + x];
            dst[x * 3] = (uint8_t)(LV_COLOR_GET_R(pixel) << 3);
            dst[x * 3 + 1] = (uint8_t)(LV_COLOR_GET_G(pixel) << 2);
            dst[x * 3 + 2] = (uint8_t)(LV_COLOR_GET_B(pixel) << 3);
        }
    }
    SDL_UpdateTexture(s_texture, &rect,
        &s_rgb_frame[(rect.y * SIM_HOR_RES + rect.x) * 3], SIM_HOR_RES * 3);
    SDL_RenderClear(s_renderer);
    SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
    SDL_RenderPresent(s_renderer);
    lv_disp_flush_ready(drv);
}

static void sim_mouse_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    int x;
    int y;
    uint32_t buttons;

    (void)drv;
    buttons = SDL_GetMouseState(&x, &y);
    data->point.x = x;
    data->point.y = y;
    data->state = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) ?
        LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

int main(int argc, char **argv)
{
    lv_disp_drv_t disp_drv;
    lv_indev_drv_t mouse_drv;
    uint32_t last_ticks;
    bool running = true;

    (void)argc;
    (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        return 1;
    }

    s_window = SDL_CreateWindow("GPCV2178 LVGL Simulator", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, SIM_HOR_RES, SIM_VER_RES, 0);
    s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_ACCELERATED);
    s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING, SIM_HOR_RES, SIM_VER_RES);
    if ((s_window == NULL) || (s_renderer == NULL) || (s_texture == NULL)) {
        SDL_Quit();
        return 1;
    }

    lv_init();
    lv_fs_stdio_init();
    lv_png_init();
    lv_disp_draw_buf_init(&s_draw_buf, s_draw_buf_mem, NULL,
        SIM_HOR_RES * SIM_DRAW_LINES);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SIM_HOR_RES;
    disp_drv.ver_res = SIM_VER_RES;
    disp_drv.flush_cb = sim_flush_cb;
    disp_drv.draw_buf = &s_draw_buf;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_init(&mouse_drv);
    mouse_drv.type = LV_INDEV_TYPE_POINTER;
    mouse_drv.read_cb = sim_mouse_read_cb;
    lv_indev_drv_register(&mouse_drv);
    ui_dashboard_create();
    ui_dashboard_set_track("Souvenir", "Helen Hiott", true);

    last_ticks = SDL_GetTicks();
    while (running) {
        SDL_Event event;
        uint32_t now = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        lv_tick_inc(now - last_ticks);
        last_ticks = now;
        lv_timer_handler();
        SDL_Delay(5);
    }

    SDL_DestroyTexture(s_texture);
    SDL_DestroyRenderer(s_renderer);
    SDL_DestroyWindow(s_window);
    SDL_Quit();
    return 0;
}

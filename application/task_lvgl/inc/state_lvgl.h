/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2026 by Generalplus Inc.                         *
 *                                                                        *
 *  state_lvgl.h - LVGL 任务接口                                          *
 *                                                                        *
 *  - lvgl_state_task_init(): 由 main.c InitTurnkeyTaskCfg 调用，创建任务队列
 *  - state_lvgl_entry():     由 main.c InitTask 创建为 lvgl_task 任务入口
 *                                                                        *
 **************************************************************************/

#ifndef __STATE_LVGL_H__
#define __STATE_LVGL_H__

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void lvgl_state_task_init(void);
extern void state_lvgl_entry(void const *arg);
/* Request a page switch; actual LVGL calls remain in the LVGL task. */
extern int lvgl_page_show_dashboard(void);
extern int lvgl_page_show_music(void);
extern int lvgl_page_show_phone(void);

typedef enum
{
    LVGL_PHONE_KEY_UP = 0,
    LVGL_PHONE_KEY_DOWN,
    LVGL_PHONE_KEY_LEFT,
    LVGL_PHONE_KEY_RIGHT,
    LVGL_PHONE_KEY_SELECT
} lvgl_phone_key_t;

typedef enum
{
    LVGL_MUSIC_KEY_PREVIOUS = 0,
    LVGL_MUSIC_KEY_NEXT,
    LVGL_MUSIC_KEY_PLAY_PAUSE
} lvgl_music_key_t;

/* Routes the dialer five-way mapping to the phone page, or Key2/Key4 to the
 * fixed bottom navigation strip on every other page. */
extern int lvgl_phone_key_send(lvgl_phone_key_t key);
/* Key5/Key6/Key8 are consumed by the dashboard UI and control AVRCP only
 * while the embedded music page is selected. */
extern int lvgl_music_key_send(lvgl_music_key_t key);

#endif //__STATE_LVGL_H__

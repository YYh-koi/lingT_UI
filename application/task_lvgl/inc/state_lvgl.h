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

#endif //__STATE_LVGL_H__

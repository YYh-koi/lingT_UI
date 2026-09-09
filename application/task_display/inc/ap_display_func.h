/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2022 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#ifndef __AP_DISPLAY_FUNC_H__
#define __AP_DISPLAY_FUNC_H__

#include "task_display_cfg.h"

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct
{
	INT32U base;
	INT32U offset;
	struct
	{
        INT16U  top;
		INT16U	bottom;
		INT16U	left;
		INT16U	right;
	}padding;

	struct
	{
        INT16U  src_w;// is the same as tft_width of DispCtrl_s
		INT16U	src_h;// is the same as tft_height of DispCtrl_s
		INT16U	tgt_w;// is the same as lcd_width of DispCtrl_s
		INT16U	tgt_h;// is the same as lcd_width of DispCtrl_s
	}scaleup;
} DISP_T, *DISP_P;

typedef DISP_P DISP_QUEUE[DISPLAY_BUF_NUM];

typedef struct
{
	INT8U h;
	INT8U m;
	INT8U s;
} DISPLAY_REC_TIME;

/**************************************************************************
 *                  M A C R O S    &    E N U M S                         *
 **************************************************************************/

#define DISPLAY_ICON_RED_FLAG           1
#define DISPLAY_ICON_BLUE_FLAG          2
#define DISPLAY_ICON_BLUE1_FLAG         3

#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
typedef enum
{
	DISPLAY_PATH_NORMAL = 0,
	DISPLAY_PATH_LVGL,
	DISPLAY_PATH_MAX
} DISPLAY_PATH;
#endif
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern DISP_QUEUE	display_free_queue;
extern DISP_QUEUE	display_ready_queue;
extern DISP_QUEUE	display_rotate_queue;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void ap_display_init(void);
extern void ap_display_clear_screen(INT32U buf);
extern DISP_P ap_display_showing_frame_get(void);
extern void ap_display_icon_draw(INT16U *buf, INT16U buf_w, INT16U buf_h, DISPLAY_ICONSHOW *icon_ptr, INT8U color_flag);
extern void ap_display_icon_draw_gp420(INT16U *buf, INT16U buf_w, INT16U buf_h, DISPLAY_ICONSHOW *icon_ptr, INT8U color_flag);
extern void ap_display_icon_draw_yuyv(INT16U *buf, INT16U buf_w, INT16U buf_h, DISPLAY_ICONSHOW *icon_ptr, INT8U color_flag);

extern DISP_P ap_display_queue_get(DISP_QUEUE queue);
extern INT32S ap_display_queue_put(DISP_QUEUE queue, DISP_P data);
extern DISP_P ap_display_queue_query(DISP_QUEUE queue);
extern void judge_and_release_display_buffer(DISP_P frame_show);
extern DISP_P displayaddr_to_displayframe(INT32U displayaddr);

extern void ap_display_buff_copy_and_draw(DISP_P frame_show, INT32U src_type);
extern void ap_display_rotate_restart(void);
extern INT8U ap_display_rotate_busy(void);

extern void ap_display_effect_init(void);
extern void ap_display_icon_sts_set(INT32U msg);
extern void ap_display_icon_sts_clear(INT32U msg);

extern void ap_display_rec_time_enable(void);
extern void ap_display_left_rec_time_enable(DISPLAY_REC_TIME left_rec_time);
extern void ap_display_play_time_enable(void);
extern void ap_display_rec_time_disable(void);

extern void ap_display_rtc_date_time_enable(void);
extern void ap_display_file_date_time_enable(void);
extern void ap_display_rtc_date_time_disable(void);

extern void ap_display_string_icon_enable(INT16U idx, void *icon_ptr);
extern void ap_display_string_icon_disable(INT16U idx);

#if C_WATER_MARK == CUSTOM_ON
extern void ap_display_water_mark_icon_enable(void);
extern void ap_display_water_mark_icon_disable(void);
#endif

extern INT32U getDispDevBufSize(void);

extern void ap_Switch_Screen(void);
extern INT32U ap_Handle_switch_screen_status(PS_ID PScalerNum,INT32U inW ,INT32U inH);
extern void ap_display_restart(TFT_ISR isr);
extern INT32U ap_switch_tft_ap_mode_get(void);				/*ap mode = record mode or photo mode */
extern void ap_switch_tft_ap_mode_set(INT32U ap_mode);	/* set switch tft ap mode */

#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
extern INT32U lvgl_display_buf;
extern void ap_state_handling_display_path_set(INT32U path);
extern INT32U ap_state_handling_display_path_get(void);
extern void ap_display_buff_lvgl_draw(INT32U* lvglbuf, INT32U msg_id);
#endif
#endif //__AP_DISPLAY_FUNC_H__

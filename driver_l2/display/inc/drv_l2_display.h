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
#ifndef __DRV_L2_DISPLAY_H__
#define __DRV_L2_DISPLAY_H__
#include "board_config.h"

#define DISDEV_NUM		 			(2)			/* setting device number *//* must correspond with customer.h  LCD CONFIG */


// data type
typedef enum
{
	DISDEV_TFT = 0x1,
	DISDEV_TFT2 = 0x2,
	DISDEV_TFT_SPI0 = 0x4,
	DISDEV_TFT_SPI1 = 0x8,
	DISDEV_MAX,

#if (defined(TFT1_ENABLE) && TFT1_ENABLE) &&(defined(TFT2_SPI_ENABLE) && TFT2_SPI_ENABLE)
    DISPLAY_DEVICE = DISDEV_TFT+DISDEV_TFT_SPI1
#elif (defined(TFT2_ENABLE) && TFT2_ENABLE) &&(defined(TFT1_SPI_ENABLE) && TFT1_SPI_ENABLE)
    DISPLAY_DEVICE = DISDEV_TFT2+DISDEV_TFT_SPI0
#elif (defined(TFT1_SPI_ENABLE) && TFT1_SPI_ENABLE)
       DISPLAY_DEVICE = DISDEV_TFT_SPI0
#elif (defined(TFT1_ENABLE) && TFT1_ENABLE)
    DISPLAY_DEVICE = DISDEV_TFT      //Selection DISDEV_TFT/DISDEV_TFT_SPI1/DISDEV_TFT_SPI0 device

#elif (defined(TFT2_ENABLE) && TFT2_ENABLE)
    DISPLAY_DEVICE = DISDEV_TFT2     // Selection DISDEV_TFT2/DISDEV_TFT2_HDMI720P/DISDEV_TFT2_HDMI480P/DISDEV_TFT2_HDMI1080P/DISDEV_TFT2_HDMI1080I
#endif
} DISP_DEV;

typedef enum
{
	DISP_FMT_GP420,
	DISP_FMT_RGB565,
	DISP_FMT_BGRG,
	DISP_FMT_GBGR,
	DISP_FMT_RGBG,
	DISP_FMT_GRGB,
	DISP_FMT_VYUV,
	DISP_FMT_YVYU,
	DISP_FMT_UYVY,
	DISP_FMT_YUYV,
    DISP_FMT_ARGB,
	DISP_FMT_MAX
} DISP_FMT;

typedef enum
{
	UI_FMT_RGB1555		= 0,
	UI_FMT_PALETTE_4,
	UI_FMT_PALETTE_16,
	UI_FMT_PALETTE_256,
	UI_FMT_ARGB444,
	UI_FMT_MAX
} UI_FMT;

typedef enum
{
	TFT_ROT_NONE,
	TFT_ROT_RIGHT,
	TFT_ROT_LEFT,
	TFT_ROT_180,
	TFT_ROT_H_MIRROR,
	TFT_ROT_V_MIRROR
} TFT_ROT;

typedef enum
{
	LCD_IFCE_RGB,
	LCD_IFCE_I80,
	LCD_IFCE_SPI
} LCD_IFCE;

typedef enum
{
	LCD_BL_CTRL_CONSTANT,
	LCD_BL_CTRL_PWM
} LCD_BL_CTRL;

typedef enum
{
	LCD_MULTI_DEVICE1=0,
	LCD_MULTI_DEVICE2,
	LCD_MULTI_DEVICE3

}MULTI_LCD_ID_DEFINE;


#define DISDEV_TFT2_HDMI480P 			0x0
#define DISDEV_TFT2_HDMI720P			0x1
#define DISDEV_TFT2_HDMI1080P			0x2
#define DISDEV_TFT2_HDMI1080I			0x3
#define DISDEV_TFT2_HDMI480P_50HZ		0x4
#define DISDEV_TFT2_HDMI720P_50HZ		0x5
#define DISDEV_TFT2_HDMI1080P_50HZ		0x6
#define DISDEV_TFT2_HDMI1080I_50HZ		0x7
#define DISDEV_TFT2_DVI480P				0x8
#define DISDEV_TFT2_DVI720P				0x9
#define DISDEV_TFT2_DVI1080P			0x0A
#define DISDEV_TFT2_DVI1080I			0x0B

/**********************************************************************/
typedef struct DispCtrl_s
{
	INT16U		lcd_width;
	INT16U		lcd_height;

	INT8U       sync_pin_enable;
    INT8U       de_pin_enable;
    INT8U       te_pin_enable;
    INT8U       tft2_io_enable;

    INT16U      h_off_lines_video;
    INT16U      h_off_lines_cap;

    INT8U       en_crop_video;
    INT8U       en_crop_cap;
    INT8U       pbp_crop_video;
    INT8U       pbp_crop_cap;
    INT8U       back_ratio_video;
    INT8U       back_ratio_cap;

	INT16U		pip_width;
	INT16U		pip_height;
	INT16S		pip_pos_x;
	INT16S		pip_pos_y;

	INT32S		(*init) (INT32U tft_index);
	INT32S		(*uninit) (INT32U tft_index);

	LCD_BL_CTRL	lcd_bl_ctrl;
	INT32U		lcd_pwm_freq;
	INT8U		lcd_pwm_duty;

	DISP_FMT	tft_fmt;
	LCD_IFCE	lcd_ifce;
	TFT_ROT		tft_rot;

	INT16U		tft_width;
	INT16U		tft_height;

} DispCtrl_t;

#if LCD_DRV_EN_st7789_I80
extern const DispCtrl_t TFT_Param_ST7789_I80;
#endif

#if LCD_DRV_EN_st7789_I80_240x240
extern const DispCtrl_t TFT_Param_ST7789_I80_240x240;
#endif

#if LCD_DRV_EN_TPO_TD025THD1
extern const DispCtrl_t TFT_Param_td025thd1;
#endif

#if LCD_DRV_EN_T43
extern const DispCtrl_t TFT_Param_t43;
#endif

#if LCD_DRV_EN_ST7703_QX550H4001
extern const DispCtrl_t TFT_Param_qx550h4001;
#endif

#if LCD_DRV_EN_ILI9486L_CK3510S0
extern const DispCtrl_t TFT_Param_ILI9486L_CK3510S0;
#endif

#if LCD_DRV_EN_WTL096601G06
extern const DispCtrl_t TFT_Param_WTL096601G06;
#endif

#if LCD_DRV_EN_WT_BOE_9707
extern const DispCtrl_t TFT_Param_WT_BOE_9707;
#endif

#if LCD_DRV_EN_WTL096602G02
extern const DispCtrl_t TFT_Param_WTL096602G02;
#endif

#if LCD_DRV_EN_TXDT240C_OTA5182A
extern const DispCtrl_t TFT_Param_TXDT240C_OTA5182A;
#endif

#if LCD_DRV_EN_HDMI
extern const DispCtrl_t TFT_Param_hdmi;
#endif

#if LCD_DRV_EN_st7789_565
extern const DispCtrl_t TFT_Param_st7789_565;
#endif


#if LCD_DRV_EN_AUO_A027DTN019
extern const DispCtrl_t TFT_Param_auo_a027dtn019;
#endif

#if LCD_DRV_EN_HSD_GC9503V
extern const DispCtrl_t TFT_Param_HSD_GC9503V;
#endif

#if LCD_DRV_EN_DST_GC9106
extern const DispCtrl_t TFT_Param_DST_GC9106;
#endif

#if LCD_DRV_EN_TP2915
extern const DispCtrl_t TFT_Param_tp2915;
#endif

#if LCD_DRV_EN_LY101BOE
extern const DispCtrl_t TFT_Param_LY101BOE;
#endif

#if LCD_DRV_EN_KD055HD_IL9881C
extern const DispCtrl_t TFT_Param_KD055HD_IL9881C;
#endif

#if LCD_DRV_EN_JD9366TC
extern const DispCtrl_t TFT_Param_JD9366TC;
#endif

#if LCD_DRV_EN_CK69502S0_FL7703N
extern const DispCtrl_t TFT_Param_CK69502S0_FL7703N;
#endif

/**********************************************************************/
typedef void (*TFT_ISR) (void);

extern void		drv_l2_tft_init(TFT_DEV_ID tft_idx);
extern void		drv_l2_tft_uninit(TFT_DEV_ID tft_idx);

extern INT32S	drv_l2_display_init(void);
extern void	drv_l2_display_uninit(void);
extern void	drv_l2_disp_dev_reinit(void);
extern void     drv_l2_disp_dev_uninit(void);

extern INT32S	drv_l2_display_start(TFT_ISR isr);
extern INT32S	drv_l2_display_stop(void);

/*
extern INT32S	drv_l2_display_update(INT32U buffer);
extern INT32S	drv_l2_display_buffer_update_state_get(INT32U wait);
extern INT32S	drv_l2_display_buffer_set(INT32U buffer);
*/

extern INT32S	drv_l2_ppu_isr_callback_set(void (*callback) (INT32U ppu_event));

extern INT32S	drv_l2_display_UI_init(UI_FMT color_mode, INT8U blend_level, INT32U ui_addr, INT32U pal_addr);
extern INT32S	drv_l2_display_UI_uninit(void);
extern INT32S	drv_l2_display_UI_update(INT8U blend_level, INT32U ui_addr, INT32U pal_addr);
extern INT32S	drv_l2_display_UI_stop(void);

extern INT32S	drv_l2_tft_buffer_set(INT32U buffer);
extern INT32U	drv_l2_tft_buffer_get(void);

extern void     drv_l2_tft_busy_wait(void);
extern void     drv_l2_tft_en_set(INT32U en);
extern void	drv_l2_disable_te_pin(void);

extern DISP_FMT	drv_l2_display_tft_fmt_get(void);
extern TFT_ROT	drv_l2_display_tft_rot_get(void);
extern LCD_IFCE	drv_l2_display_lcd_ifce_get(void);

extern INT16U	drv_l2_display_tft_width_get(void);
extern INT16U	drv_l2_display_tft_height_get(void);

extern INT16U drv_l2_display_off_lines_video_get(void);
extern INT16U drv_l2_display_off_lines_cap_get(void);
extern INT8U drv_l2_display_en_crop_video_get(void);
extern INT8U drv_l2_display_en_crop_cap_get(void);
extern INT8U drv_l2_display_pbp_crop_video_get(void);
extern INT8U drv_l2_display_pbp_crop_cap_get(void);
extern INT8U drv_l2_display_back_ratio_video_get(void);
extern INT8U drv_l2_display_back_ratio_cap_get(void);

extern INT16U	drv_l2_display_pip_width_get(void);
extern INT16U	drv_l2_display_pip_height_get(void);
extern INT16U	drv_l2_display_pip_x_get(void);
extern INT16U	drv_l2_display_pip_y_get(void);

extern void drv_l2_tft_backlight_en_set(BOOLEAN enable);
extern void ppu_set_size(INT16U width, INT16U height);


extern INT32U Displaydevice_assign(void);
#if C_ASYN_DBLE_TFT
extern void drv_l2_asyn_multi_tft_hdmi_plug_status_set(INT32U status);	/*set 1 -> HDMI plug in ; set 0 -> HDMI plug out */
extern INT32U drv_l2_asyn_multi_tft_hdmi_plug_status_get(void);				/* get HDMI plug in/out status */
extern INT32U drv_l2_asyn_multi_tft_maximum_width_get(void);		/* return minimum tft height */
extern INT32U drv_l2_asyn_multi_tft_maximum_height_get(void);				/* return maximum tft height */
extern void	drv_l2_asyn_double_tft_backlight_set(INT32U en);
extern INT32U drv_l2_asyn_double_tft_backligh_get(void);
extern void	drv_l2_asyn_double_tft_backlight_en_set(BOOLEAN display1_bl_enable,BOOLEAN display2_bl_enable);
extern void	drv_l2_asyn_double_tft_backlight_switch(void);
extern INT32U drv_l2_previous_display_width(void);
extern INT32U previous_h_off_lines_video(void);
extern INT32U previous_tft_fmt(void);
extern INT32U	Handle_switch_screen_status(void);
extern INT32U	Switch_screen_ACK(void);
extern void tft_switch_stop_record_ACK(void);
extern void Switch_Screen(TFT_ISR isr);
extern void Switch_screen_set(INT32U val);
extern INT32U Switch_screen_get(void);
extern void tft_start_switch_screen_set(INT32U val);
extern INT32U tft_start_switch_screen_get(void);
extern void tft_recording_tft_switch_set(INT32U val);
extern INT32U tft_recording_tft_switch_get(void);
extern INT32U tft_switch_tft_idx_get(void);
extern INT32U tft_hdmi_idx_get(void);
extern void drv_l2_asyn_multi_tft_init(void);
extern void drv_l2_restart_display_init(void);
extern void Switch_TFT_parameter(void);

#endif

#if (defined C_HDMI_SCALE_UP_EN) && (C_HDMI_SCALE_UP_EN)
extern void drv_l2_display_tft_width_reset(INT16U new_width);
void drv_l2_display_tft_height_reset(INT16U new_height);
#endif

#endif  //__DRV_L2_DISPLAY_H__

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

/*******************************************************
    Include file
*******************************************************/
#include "drv_l2.h"
#include "application.h"

#if ((defined(TFT1_ENABLE) && TFT1_ENABLE) || (defined(TFT2_ENABLE) && TFT2_ENABLE)|| (defined(TFT1_SPI_ENABLE) && TFT1_SPI_ENABLE) || (defined(TFT2_SPI_ENABLE) && TFT2_SPI_ENABLE))


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

//#define SWITCH_SCREEN_TEST

#ifndef DISABLE
#define DISABLE                         0
#endif

#ifndef ENABLE
#define ENABLE                          1
#endif

#define DBG_DISP_ENABLE		            1
#if DBG_DISP_ENABLE == 1
#define DRV_DISP_DBG		            print_string
#else
#define DRV_DISP_DBG(...)
#endif

#define DISP_UNDER_RUN_EN           1

#define DISP_OS_EN                  0
#if DISP_OS_EN
#define DISP_QUEUE_MAX              3
#define DISP_UPDATE_DONE            1
#endif

#define PPU_PPU_V_BLANKING          (1 << 0)
#define PPU_DMA_COMPLETE            (1 << 2)
#define PPU_TFT_UNDER_RUN           (1 << 12)
#define PPU_TFT_V_BLANKING          (1 << 13)
#define PPU_TFT1_V_BLANKING         (1 << 19)
#define PPU_TFT1_UNDER_RUN          (1 << 20)

#define PPU_YUYV_TYPE3				(3 << 20)
#define PPU_YUYV_TYPE2				(2 << 20)
#define PPU_YUYV_TYPE1				(1 << 20)
#define PPU_YUYV_TYPE0				(0 << 20)

#define PPU_RGBG_TYPE3				(3 << 20)
#define PPU_RGBG_TYPE2				(2 << 20)
#define PPU_RGBG_TYPE1				(1 << 20)
#define PPU_RGBG_TYPE0				(0 << 20)

#define ARGB_MODE                   (1 << 24)
#define TFT_LB                      (1 << 24)
#define PPU_LB                      (1 << 19)
#define TV_LB                       (1 << 15)
#define	PPU_RGBA_MODE               (2 << 10)
#define	PPU_YUYV_MODE               (1 << 10)
#define	PPU_RGBG_MODE               (0 << 10)
#define	TFT1_GP420_MODE             (1 << 1)
#define	TFT0_GP420_MODE             (1 << 0)

#define TFT_SIZE_1024X768			(7 << 16)
#define TFT_SIZE_800X600			(6 << 16)
#define TFT_SIZE_800X480			(5 << 16)
#define TFT_SIZE_720X480			(4 << 16)
#define TFT_SIZE_480X272			(3 << 16)
#define TFT_SIZE_480X234			(2 << 16)
#define TFT_SIZE_640X480			(1 << 16)
#define TFT_SIZE_320X240			(0 << 16)
#define TFT_SIZE_MASK				(7 << 16)

#define PPU_YUYV_RGBG_FORMAT_MODE	(1 << 8)
#define PPU_RGB565_MODE				(0 << 8)

#define PPU_FRAME_BASE_MODE			(1 << 7)
#define PPU_VGA_NONINTL_MODE		(0 << 5)

#define PPU_VGA_MODE				(1 << 4)
#define PPU_QVGA_MODE				(0 << 4)

#define PPU_UI_ENABLE				(1 << 0)
#define PPU_UI_HDMI_MODE			(1 << 1)
#define PPU_UI_COLOR_1555			(0 << 2)
#define PPU_UI_COLOR_PAL_4			(1 << 2)
#define PPU_UI_COLOR_PAL_16			(2 << 2)
#define PPU_UI_COLOR_PAL_256		(3 << 2)
#define PPU_UI_COLOR_MASK			(3 << 2)
#define PPU_UI_BLEND_BIT			4
#define PPU_UI_BLEND_MASK			(0xF << PPU_UI_BLEND_BIT)
#define PPU_UI_COLOR_ARGB4444		(1 << 8)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct display_s
{
#if DISP_OS_EN == 1
    osMessageQId    tft_q;
    osMessageQId    tft2_q;
    osMessageQId    rot2_q;
#endif

	/*
	INT32U		tft_update_flag;
	INT32U		tft2_update_flag;
	INT32U		tft_disp_buf;
	INT32U		tft2_disp_buf;
	*/

	INT8U       rot2_LB_init;
    INT32U      rot2_disp_buf;

#if DISP_UNDER_RUN_EN
	INT32U		tft_under_run_flag;
	INT32U		tft2_under_run_flag;
#endif

    /*
	INT32U		ui_color_fmt;
	INT32U		ui_update_flag;
	INT32U		ui_blend_level;
	INT32U		ui_disp_buf;
	*/

    INT32U      disp_dev_init;

} display_t;



#if C_ASYN_DBLE_TFT
extern INT32S tft_st7789_i80_init(INT32U tft_index);
extern INT32S tft2_hdmi_init(INT32U tft_index);
extern INT32S mipi_display_KD055HD_IL9881C_720p_init(INT32U tft_index);
#endif



typedef struct gDisplaydevice_para_s
{
	QueueSetHandle_t  switch_screen_ack;
	QueueSetHandle_t  switch_tft_recording_stop_ack;
	DispCtrl_t*  TFT_Parameter[DISDEV_NUM];			/* display structure from  Displaydevice_Container[] */

	INT32U device[DISDEV_NUM];
	INT32U currnent_screen_idx;						/* it is related with MULTI_LCD_ID_DEFINE(enum) */
	INT32U switch_screen_flag;						/* 1: switch  0: none  */
	INT32U switch_tft_screen_procedure_flag;
	INT32U recording_tft_switch_flag;					/* when record mode is recording -> set '1' */

	INT32U multi_tft_max_width;						/* for setting buffer size @ap_display_init() getDispDevBufSize() */
	INT32U multi_tft_max_height;						/* for setting buffer size @ap_display_init() getDispDevBufSize() */
	INT32U tft_switch_back_light_flag;
	INT32U tft_switch_back_light_cnt;
	INT32U main_screen_idx;   							/* poweron first tft idx */
	INT32U hdmi_screen_idx;							/* HDMI idx */
	INT32U hdmi_plugin_flag;							/* if set 0-> HDMI plugont ; if set 1 -> HDMI plugin */

	INT32U previous_h_off_lines_video;				/* get the previous h_off_lines_video -> solving the  returning buffer of switching different device */
	INT32U previous_display_width;					/* get the previous width -> solving the  returning buffer of switching different device */
	INT32U previous_tft_fmt;							/* get the previous tft_fmt -> solving the  returning buffer of switching different device */

}gDisplaydevice_para_t;

typedef struct gDisplaydevice_s
{
	gDisplaydevice_para_t info;
	INT32U (*current_device_set)(INT32U idx , gDisplaydevice_para_t* deviceinfo );
	INT32U (*current_device_get)(gDisplaydevice_para_t* deviceinfo);

	#if C_ASYN_DBLE_TFT
	INT32S	(*main_tft_device_fp) (INT32U tft_index);	/* the fp of poweron first tft */
	INT32S	(*hdmi_tft_device_fp) (INT32U tft_index);	/* the fp of HDMI */
	#endif

}gDisplaydevice_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static display_t display, *pDisp;

static void (*ppu_isr_callback) (INT32U ppu_event);
static TFT_ISR tft_isr_vblank_callback;
extern TFT_ISR spitft_isr_vblank_callback;

static gDisplaydevice_t Dualdisplay;//={0};
static DispCtrl_t *TFT_Param;
#ifdef SWITCH_SCREEN_TEST
static void DUAL_SCREEN_DEBUG_TEST(void);
#endif

#if C_ASYN_DBLE_TFT
//INT32U tft_switch_flag=0, cnt=0;
#endif

 INT32U Displaydevice_Container[DISDEV_NUM]=
{
#if LCD_DRV_EN_TPO_TD025THD1
//static const DispCtrl_t *TFT_Param = &TFT_Param_td025thd1;
  (INT32U)&TFT_Param_td025thd1,
#endif

#if LCD_DRV_EN_T43
//static const DispCtrl_t *TFT_Param = &TFT_Param_t43;
  (INT32U)&TFT_Param_t43,
#endif

#if LCD_DRV_EN_ST7703_QX550H4001
//static const DispCtrl_t *TFT_Param = &TFT_Param_qx550h4001;
  (INT32U)&TFT_Param_qx550h4001,
#endif

#if LCD_DRV_EN_ILI9486L_CK3510S0
//static const DispCtrl_t *TFT_Param = &TFT_Param_ILI9486L_CK3510S0;
  (INT32U)&TFT_Param_ILI9486L_CK3510S0,
#endif

#if LCD_DRV_EN_WTL096601G06
//static const DispCtrl_t *TFT_Param = &TFT_Param_WTL096601G06;
  (INT32U)&TFT_Param_WTL096601G06,
#endif

#if LCD_DRV_EN_WT_BOE_9707
//static const DispCtrl_t *TFT_Param = &TFT_Param_WT_BOE_9707;
 (INT32U)&TFT_Param_WT_BOE_9707,
#endif

#if LCD_DRV_EN_WTL096602G02
//static const DispCtrl_t *TFT_Param = &TFT_Param_WTL096602G02;
  (INT32U)&TFT_Param_WTL096602G02,
#endif

#if LCD_DRV_EN_TXDT240C_OTA5182A
//static const DispCtrl_t *TFT_Param = &TFT_Param_TXDT240C_OTA5182A;
  (INT32U)&TFT_Param_TXDT240C_OTA5182A,
#endif

#if LCD_DRV_EN_st7789_565
 (INT32U)&TFT_Param_st7789_565,
#endif



#if LCD_DRV_EN_st7789_I80
 (INT32U)&TFT_Param_ST7789_I80,
#endif

#if LCD_DRV_EN_st7789_I80_240x240
 (INT32U)&TFT_Param_ST7789_I80_240x240,
#endif



#if LCD_DRV_EN_AUO_A027DTN019
//static const DispCtrl_t *TFT_Param = &TFT_Param_auo_a027dtn019;
  (INT32U)&TFT_Param_auo_a027dtn019,
#endif

#if LCD_DRV_EN_HSD_GC9503V
//static const DispCtrl_t *TFT_Param = &TFT_Param_HSD_GC9503V;
  (INT32U)&TFT_Param_HSD_GC9503V,
#endif

#if LCD_DRV_EN_DST_GC9106
    (INT32U)&TFT_Param_DST_GC9106,
#endif

#if LCD_DRV_EN_TP2915
	(INT32U)&TFT_Param_tp2915,
#endif

#if LCD_DRV_EN_LY101BOE
	(INT32U)&TFT_Param_LY101BOE,
#endif


#if LCD_DRV_EN_KD055HD_IL9881C
	(INT32U)&TFT_Param_KD055HD_IL9881C,
#endif


#if LCD_DRV_EN_JD9366TC
	(INT32U)&TFT_Param_JD9366TC,
#endif

#if LCD_DRV_EN_HDMI
//static const DispCtrl_t *TFT_Param = &TFT_Param_hdmi;
  (INT32U)&TFT_Param_hdmi,
#endif

#if LCD_DRV_EN_CK69502S0_FL7703N
  (INT32U)&TFT_Param_CK69502S0_FL7703N,
#endif
};

/**************************************************************************
 *                         E X T E R N    F U N C T I O N                 *
 **************************************************************************/




/**************************************************************************
 *                         L O C A L    F U N C T I O N                   *
 **************************************************************************/


/**************************************************************************
 *         F U N C T I O N      I M P L E M E N T A T I O N S             *
 **************************************************************************/
INT32U ParserDevice(gDisplaydevice_t* displaydevice)
{
	INT32U tmp,i;
	tmp = DISPLAY_DEVICE;
	for(i=0;i<DISDEV_NUM;i++)
	{
	      if(tmp&DISDEV_TFT)
		  {
			#if !C_ASYN_DBLE_TFT
			tmp&=~DISDEV_TFT;
			#endif
			displaydevice->info.device[i] = DISDEV_TFT;
			continue;
		}
		if(tmp&DISDEV_TFT2)
		{
			#if !C_ASYN_DBLE_TFT
			tmp&=~DISDEV_TFT2;
			#endif
			displaydevice->info.device[i] = DISDEV_TFT2;
			continue;
		}
		if(tmp&DISDEV_TFT_SPI0)
		{
			#if !C_ASYN_DBLE_TFT
			tmp&=~DISDEV_TFT_SPI0;
			#endif
			displaydevice->info.device[i] = DISDEV_TFT_SPI0;
			continue;
		}
		if(tmp&DISDEV_TFT_SPI1)
		{
			#if !C_ASYN_DBLE_TFT
			tmp&=~DISDEV_TFT_SPI1;
			#endif
			displaydevice->info.device[i] = DISDEV_TFT_SPI1;
		}
	}

#if !C_ASYN_DBLE_TFT
		/* way 1 : find out the main screen by checking the maximun width and height */
		if(displaydevice->info.TFT_Parameter[1]!=NULL)
		{
			if((displaydevice->info.TFT_Parameter[0]->tft_width >=
				displaydevice->info.TFT_Parameter[1]->tft_width )&&
				(displaydevice->info.TFT_Parameter[0]->tft_height >=
				displaydevice->info.TFT_Parameter[1]->tft_height))
			{
				displaydevice->info.main_screen_idx = 0;
			}
			else if
			((displaydevice->info.TFT_Parameter[1]->tft_width >=
				displaydevice->info.TFT_Parameter[0]->tft_width )&&
				(displaydevice->info.TFT_Parameter[1]->tft_height >=
				displaydevice->info.TFT_Parameter[0]->tft_height))
			{
				displaydevice->info.main_screen_idx = 1;
			}
		}
		else
		{
			displaydevice->info.main_screen_idx = 0;
		}
#else	/* C_ASYN_DBLE_TFT */

		/* way 2 :  finding setting the maximum width & height */
		displaydevice->info.multi_tft_max_width=0;
		displaydevice->info.multi_tft_max_height=0;
		for(i=0;i<DISDEV_NUM;i++)
		{
			if(displaydevice->info.TFT_Parameter[i]->tft_width >= displaydevice->info.multi_tft_max_width)
			{
				displaydevice->info.multi_tft_max_width=displaydevice->info.TFT_Parameter[i]->tft_width;
			}

			if(displaydevice->info.TFT_Parameter[i]->tft_height >= displaydevice->info.multi_tft_max_height)
			{
				displaydevice->info.multi_tft_max_height=displaydevice->info.TFT_Parameter[i]->tft_height;
			}
		}

		/* setting the idx of main/hdmi */
		// temporary remarked
		#if 1
		for(i=0;i<DISDEV_NUM;i++)
		{
			if(displaydevice->info.TFT_Parameter[i]->init == displaydevice->main_tft_device_fp)
			{
				displaydevice->info.main_screen_idx = i;
			}

			if(displaydevice->info.TFT_Parameter[i]->init == displaydevice->hdmi_tft_device_fp)
			{
				displaydevice->info.hdmi_screen_idx = i;
			}
		}
		#endif

		DBG_PRINT("main_idx=%d,hdmi_idx=%d\r\n",displaydevice->info.main_screen_idx ,displaydevice->info.hdmi_screen_idx);

#endif	/* C_ASYN_DBLE_TFT */

		return 0;

}

INT32U current_device_set(INT32U dev_idx,gDisplaydevice_para_t* displaydevice)
{
	displaydevice->currnent_screen_idx = dev_idx;
	TFT_Param = displaydevice->TFT_Parameter[dev_idx];
  	return 0;
}

INT32U current_device_get( gDisplaydevice_para_t* displaydevice)
{
	INT32U idx;
	idx = displaydevice->currnent_screen_idx;
	return displaydevice->device[idx];
}

INT32U Displaydevice_assign(void)
{
	INT32U i,idx;

	Dualdisplay.current_device_set = &current_device_set;
	Dualdisplay.current_device_get = &current_device_get;

	/* Who is the main TFT /sub TFT / HDMI  By registering function point */
	#if C_ASYN_DBLE_TFT

	//Dualdisplay.main_tft_device_fp = &tft_st7789_i80_init;			/* set the main tft fp */
	Dualdisplay.main_tft_device_fp = &mipi_display_KD055HD_IL9881C_720p_init;			/* set the main tft fp */

	Dualdisplay.hdmi_tft_device_fp = &tft2_hdmi_init;				/* set the HDMI tft fp */
	#endif

	/* multi display */
	if(Displaydevice_Container[1]!=(INT32U)NULL)	/* means tft settings more than 2 tft device */
  	{
		Dualdisplay.info.switch_screen_ack = xQueueCreate(1, sizeof(INT32S));
		xQueueReset(Dualdisplay.info.switch_screen_ack);

		Dualdisplay.info.switch_tft_recording_stop_ack = xQueueCreate(1, sizeof(INT32S));
		xQueueReset(Dualdisplay.info.switch_tft_recording_stop_ack);

		Dualdisplay.info.currnent_screen_idx=LCD_MULTI_DEVICE1;	/* currnent_screen_idx as LCD_MULTI_DEVICE1 */
		Dualdisplay.info.hdmi_plugin_flag=0;								/* initial HDMI plugin as 0, not plug in*/
  	}

	for(i=0;i<DISDEV_NUM;i++)
	{
	 	if(Displaydevice_Container[i]!=(INT32U)NULL)
			Dualdisplay.info.TFT_Parameter[i]= (DispCtrl_t*)Displaydevice_Container[i];
		else
			Dualdisplay.info.TFT_Parameter[i]= NULL;
	}

	ParserDevice(&Dualdisplay);				/* setting the parameter of display structure */

	idx = Dualdisplay.info.main_screen_idx;		/* Set idx which is the first TFT when power on */
	Dualdisplay.current_device_set(idx,(gDisplaydevice_para_t*)&Dualdisplay.info);

 	/* set the previous TFT parameter of  h_off_lines_video & display_width for  displayaddr_to_displayframe() */
	Dualdisplay.info.previous_display_width	= drv_l2_display_tft_width_get();
	Dualdisplay.info.previous_h_off_lines_video	= drv_l2_display_off_lines_video_get();
	Dualdisplay.info.previous_tft_fmt = drv_l2_display_tft_fmt_get();

	#ifdef SWITCH_SCREEN_TEST
	DUAL_SCREEN_DEBUG_TEST();
	#endif

  return 0;
}

#if C_ASYN_DBLE_TFT
void Switch_screen_set(INT32U val)
{
	Dualdisplay.info.switch_screen_flag = val;
}

INT32U Switch_screen_get(void)
{
	return Dualdisplay.info.switch_screen_flag;
}

void tft_start_switch_screen_set(INT32U val)
{
	Dualdisplay.info.switch_tft_screen_procedure_flag = val;
}

INT32U tft_start_switch_screen_get(void)
{
	return Dualdisplay.info.switch_tft_screen_procedure_flag;
}

void tft_recording_tft_switch_set(INT32U val)
{
	Dualdisplay.info.recording_tft_switch_flag = val;
}

INT32U tft_recording_tft_switch_get(void)
{
	return Dualdisplay.info.recording_tft_switch_flag;
}

INT32U tft_switch_tft_idx_get(void)
{
	return Dualdisplay.info.currnent_screen_idx;
}

INT32U tft_hdmi_idx_get(void)	/* get the HDMI idx */
{
	return 2;
}

INT32U switch_tft_idx=LCD_MULTI_DEVICE1;
INT32U pre_tft_is_hdmi_flag=0;
INT32U Handle_switch_screen_status(void)
{
	if(Switch_screen_get())	/* if get 1 -> need to switch tft */
	{
		/* set the previous TFT parameter of  h_off_lines_video & display_width */
 		/* set the previous TFT parameter of  h_off_lines_video & display_width for  displayaddr_to_displayframe() */
		Dualdisplay.info.previous_display_width	= drv_l2_display_tft_width_get();
		Dualdisplay.info.previous_h_off_lines_video	= drv_l2_display_off_lines_video_get();
		Dualdisplay.info.previous_tft_fmt = drv_l2_display_tft_fmt_get();

		if(drv_l2_asyn_multi_tft_hdmi_plug_status_get())	/* HDMI plug in */
		{
			Dualdisplay.info.currnent_screen_idx=Dualdisplay.info.hdmi_screen_idx;
			switch_tft_idx=Dualdisplay.info.currnent_screen_idx;
			pre_tft_is_hdmi_flag=1;	/* set the previous tft is HDMI flag */
		}
		else /* HDMI not plug in */
		{
			if(pre_tft_is_hdmi_flag)	/* previous tft is HDMI,set the current tft is Main TFT */
			{
				Dualdisplay.info.currnent_screen_idx=Dualdisplay.info.main_screen_idx;
				switch_tft_idx=Dualdisplay.info.currnent_screen_idx;
				pre_tft_is_hdmi_flag=0;
			}
			else	/* normal switching between Main tft /Sub tft */
			{
				switch_tft_idx = Dualdisplay.info.currnent_screen_idx;
				if((++switch_tft_idx)>=(DISDEV_NUM-1))	/* (DISDEV_NUM-1) -> the subtraction(-1) is HDMI not including in swtiching loop */
					switch_tft_idx=LCD_MULTI_DEVICE1;		/* set default (Main TFT) */
				Dualdisplay.info.currnent_screen_idx=switch_tft_idx;
			}
		}

		DBG_PRINT("currnent_screen_idx %d\r\n",Dualdisplay.info.currnent_screen_idx);

		return 1;  /* need to switch screen */
	}
	return 0;
}


void Switch_TFT_parameter(void)	/* renew New TFT_Param */
{
	Dualdisplay.current_device_set(switch_tft_idx,(gDisplaydevice_para_t*)&Dualdisplay.info);
	Switch_screen_set(0);		/* clear switch flag(switching TFT has been finished,so clear flag.) */
}

INT32U Switch_screen_ACK(void)
{
	INT32U msg=C_ACK_SUCCESS;
	DBG_PRINT("Switch_screen_ACK\r\n");
	osMessagePut(Dualdisplay.info.switch_screen_ack, (uint32_t)&msg, (uint32_t)portMAX_DELAY);
	return 0;
}

void tft_switch_stop_record_ACK(void)
{
	INT32U ack_msg=0;
	if(tft_recording_tft_switch_get())
	{
		ack_msg = C_ACK_SUCCESS;
		DBG_PRINT("tft_switch_stop_record_ACK\r\n");
	   	osMessagePut(Dualdisplay.info.switch_tft_recording_stop_ack, (uint32_t)&ack_msg, (uint32_t)portMAX_DELAY);
	}
}

void tft_switch_wait_ACK(void)
{
	INT32U msg;
	if(tft_recording_tft_switch_get())
	{
		DBG_PRINT("tft_switch_wait_ACK 1\r\n");
		xQueueReceive(Dualdisplay.info.switch_tft_recording_stop_ack, (void*)&msg, portMAX_DELAY);
		DBG_PRINT("tft_switch_wait_ACK 2\r\n");
		tft_recording_tft_switch_set(0);		/* recording procedure finished,set 0 */
	}
}

void Draw_Black_Screen(void)
{
	INT32U Device;
	Device = Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info);

	switch(Device)
	{
		case  DISDEV_TFT_SPI0:
		case  DISDEV_TFT:
				R_TFT_FBI_ADDR = 0x70000000;
	    			//   Buf = R_TFT_FBI_ADDR;
	    			//   vic_irq_disable(VIC_TFT1_SPI);
				break;

		case  DISDEV_TFT_SPI1:
		case DISDEV_TFT2:
				R_TV_FBI_ADDR = 0x70000000;
	  			//    Buf = R_TV_FBI_ADDR;
	  			//    vic_irq_disable(VIC_PPU);
	   			break;
	 }
	/*
     	set both callback to NULL , preventing the API
     	another tft interrupt happened and set TFT buf .
     	that means
     	0x0x70000000 will be overritten and will get unexpected screen.
	*/
	spitft_isr_vblank_callback = NULL;
	tft_isr_vblank_callback = NULL;
	//ap_display_clear_screen(Buf);

}

void Switch_Screen(TFT_ISR isr)
{
	INT32U msg=0;

	Draw_Black_Screen();
	Switch_screen_set(1);		/* set 1 , when preview_ps_isr() occurred pscaler A/B done ,checking status in ap_Handle_switch_screen_status()  */

	DBG_PRINT("B[%d]\r\n",Dualdisplay.info.currnent_screen_idx);
	xQueueReceive(Dualdisplay.info.switch_screen_ack, (void*)&msg, portMAX_DELAY);
	DBG_PRINT("A[%d]\r\n",Dualdisplay.info.currnent_screen_idx);

	if(msg==1)
	{
		#if C_ASYN_DBLE_TFT
		drv_l2_asyn_multi_tft_init();
		ap_display_restart(isr);
		drv_l2_asyn_double_tft_backlight_set(1);	/* set backlight enable */
		#endif

		tft_start_switch_screen_set(0);		/* tft switch completed */
	}

}


#ifdef SWITCH_SCREEN_TEST

#define  SWITCH_SCREEN_Period		640									// 128 = 1s
INT32U dual_screen_timerid = 0xff;
void DUAL_SCREEN_DEBUG_TEST(void)
{
    if(dual_screen_timerid == 0xFF)
	{
		dual_screen_timerid = 0x3;//BROWSE_DISPLAY_UPDATE_TIMER_ID;
		drv_l2_sys_timer_set((void *) msgQSend, (void *) dispTaskQ, MSG_DISPLAY_DUAL_SCREEN_SWITCH, dual_screen_timerid, SWITCH_SCREEN_Period);
	}
}

#endif

#endif		/* #if C_ASYN_DBLE_TFT */

 void TFT_SPI1_IRQHandler(void)
{

    INT32U enable,status;
#if 0//TFT_SPI_FIQ_EN == 1
	portBASE_TYPE taskWoken = pdFALSE;
#endif


    status = drv_l1_tft_spi_sts_get(TFT_SPI0);
    drv_l1_tft_spi_sts_set(TFT_SPI0, status);
    enable = drv_l1_tft_spi_inte_get(TFT_SPI0);


  #if 0
    msg = 0;
    if((enable & C_TFT_SPI_TX_INTE_ENABLE) && (status & C_TFT_SPI_TX_DONE))
    {
        msg |= C_TFT_SPI_TX_DONE;
        if(display_isr_callback)
            display_isr_callback(TFT_SPI0, msg);
    }
  #endif
   if((enable & C_TFT_SPI_TX_INTE_ENABLE) && (status & C_TFT_SPI_TX_DONE))
    {
       if(pDisp->rot2_LB_init == 0)
       {
         if(spitft_isr_vblank_callback)
         {
            spitft_isr_vblank_callback();
         }
       }
    }
 #if 0
    pTFT_SPIDev->interrupt_flag = msg;

    if(tft_spi_isr_callback)
    {
        tft_spi_isr_callback(TFT_SPI0, msg);
    }
  #endif
}



void PPU_IRQHandler(void)
{
	INT32U	enable = R_PPU_IRQ_EN;
	INT32U	status = R_PPU_IRQ_STATUS;

	R_PPU_IRQ_STATUS = status;
	enable &= status;

#if 0	//_DRV_L1_PPU == 1
	if((enable & PPU_PPU_V_BLANKING) && (status & PPU_PPU_V_BLANKING) || (enable & PPU_DMA_COMPLETE) && (status & PPU_DMA_COMPLETE))
	{
		if(ppu_isr_callback)    ppu_isr_callback(status);
	}
#endif

	if(enable & PPU_TFT_V_BLANKING)
	{
        /*
		if(pDisp->tft_update_flag)
		{
			INT32U msg;

			R_TFT_FBI_ADDR = pDisp->tft_disp_buf;
			pDisp->tft_update_flag = 0;

        #if DISP_OS_EN == 1
			msg = DISP_UPDATE_DONE;
			osMessagePut(pDisp->tft_q, (uint32_t) &msg, osWaitForever);
        #endif
		} */

		if(pDisp->rot2_LB_init == 0)
        {
            if(tft_isr_vblank_callback)
            {
                tft_isr_vblank_callback();
            }
        }
	}

	#if DISP_UNDER_RUN_EN
	if(enable & PPU_TFT_UNDER_RUN)
	{
		pDisp->tft_under_run_flag++;
		DRV_DISP_DBG("TFT under run\r\n");
	}
	#endif

	if(enable & PPU_TFT1_V_BLANKING)
	{
        /*
		if(pDisp->tft2_update_flag)
		{
			INT32U msg;

			R_TV_FBI_ADDR = pDisp->tft_disp_buf;
			pDisp->tft2_update_flag = 0;

        #if DISP_OS_EN == 1
			msg = DISP_UPDATE_DONE;
			osMessagePut(pDisp->tft2_q, (uint32_t) &msg, osWaitForever);
        #endif
		} */

		if(pDisp->rot2_LB_init == 0)
        {
            if(tft_isr_vblank_callback)
            {
          		#if C_ASYN_DBLE_TFT
			if(tft_start_switch_screen_get())
			{
				vic_irq_disable(VIC_PPU);
				vic_irq_unregister(VIC_PPU);
			}
			#endif
			tft_isr_vblank_callback();

            }
        }
	}

	#if DISP_UNDER_RUN_EN
	if(enable & PPU_TFT1_UNDER_RUN)
	{
		pDisp->tft2_under_run_flag++;
		DRV_DISP_DBG("TFT2 under run\r\n");
	}
	#endif
}

static void Rotator2_LB_Callback(INT8U rot_num, INT32U flag)
{
    /*
    #if DISP_OS_EN == 1
    INT32U msg;
    #endif
    */


    if(flag & C_ROTATOR_END_STATE)
    {
		if(tft_isr_vblank_callback)
		{
			#if C_ASYN_DBLE_TFT
			if(tft_start_switch_screen_get())
			{
				drv_l1_rotator_dram2tft_set(ROTATOR_2, 0);
				drv_l1_rotator_uninit(ROTATOR_2);
			}
			#endif

			tft_isr_vblank_callback();
		}

        /*
        if(pDisp->tft2_rot2_update_flag)
        {
            R_TV_FBI_ADDR = 0;
            drv_l1_rotator_src_img_addr(ROTATOR_2, pDisp->tft2_rot2_disp_buf);

            pDisp->tft2_rot2_update_flag = 0;

            #if DISP_OS_EN == 1
            if(rot2_q)
            {
                msg = DISP_UPDATE_DONE;
                #if 0//DISP_DEV_FIQ_EN == 1
                    xQueueSendFromISR(rot2_q, (const void *)&msg, &taskWoken);
                #else
                    osMessagePut(rot2_q, (uint32_t)&msg, osWaitForever);
                #endif
            }
            #endif
        } */
    }
}



void ppu_set_size(INT16U width, INT16U height)
{
	INT32U reg = R_PPU_ENABLE;
	INT32U reg2 = R_FREE_SIZE;

	reg &= ~TFT_SIZE_MASK;
    reg &= ~PPU_VGA_MODE;
	reg |= PPU_LB;
	if(width == 320 && height == 240)
	{
		reg |= TFT_SIZE_320X240;
	}
	else
	if(width == 480 && height == 234)
	{
		reg |= TFT_SIZE_480X234;
	}
 /*
   1. if use tft spi0 will set  R_PPU_ENABLE = 0x80;
   2. Dual screen case ,ex: one is tft2(480x272) , one is tft spi0
   above this , screen of tft2 will influneced , so I mark below
   condition
 */
 #if 0
	else
	if(width == 480 && height == 272)
	{
		reg |= TFT_SIZE_480X272;
	}
 #endif
	else
	if(width == 640 && height == 480)
	{
		reg |= TFT_SIZE_640X480;
	}
	else
	if(width == 720 && height == 480)
	{
		reg |= TFT_SIZE_720X480;
	}
	else
	if(width == 800 && height == 480)
	{
		reg |= TFT_SIZE_800X480;
	}
	else
	if(width == 800 && height == 600)
	{
		reg |= TFT_SIZE_800X600;
	}
	else
	if(width == 1024 && height == 768)
	{
		reg |= TFT_SIZE_1024X768;
	}
	else
	{
        reg2 &= ~( (0x7FF << 16) | 0x7FF);//due to HDMI interlace mode add at BIT[31:30]
        reg2 |= ((width & 0x7FF) << 16) | (height & 0x7FF);
        R_FREE_SIZE	= reg2;
	}

	R_PPU_ENABLE = reg;
}

static void ppu_set_color_mode(INT32U color_mode)
{
	INT32U reg = R_PPU_ENABLE;
	INT32U misc = R_PPU_MISC;

	reg &= ~(3 << 20);
	misc &= ~ARGB_MODE;
	switch(color_mode)
	{
	case DISP_FMT_RGB565:
		reg &= ~PPU_YUYV_RGBG_FORMAT_MODE;  //RGB565 mode
		reg &= ~PPU_YUYV_MODE;				//RGB565
		break;

	case DISP_FMT_BGRG:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;
		reg &= ~PPU_YUYV_MODE;				//RGBG
		reg |= PPU_RGBG_TYPE0;
		break;

	case DISP_FMT_GBGR:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;
		reg &= ~PPU_YUYV_MODE;				//RGBG
		reg |= PPU_RGBG_TYPE1;
		break;

	case DISP_FMT_RGBG:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;
		reg &= ~PPU_YUYV_MODE;				//RGBG
		reg |= PPU_RGBG_TYPE2;
		break;

	case DISP_FMT_GRGB:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;
		reg &= ~PPU_YUYV_MODE;				//RGBG
		reg |= PPU_RGBG_TYPE3;
		break;

	case DISP_FMT_VYUV:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;
		reg |= PPU_YUYV_MODE;				//YUYV
		reg |= PPU_YUYV_TYPE0;
		break;

	case DISP_FMT_YVYU:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;
		reg |= PPU_YUYV_MODE;				//YUYV
		reg |= PPU_YUYV_TYPE1;
		break;

	case DISP_FMT_UYVY:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;
		reg |= PPU_YUYV_MODE;				//YUYV
		reg |= PPU_YUYV_TYPE2;
		break;

	case DISP_FMT_YUYV:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;
		reg |= PPU_YUYV_MODE;				//YUYV
		reg |= PPU_YUYV_TYPE3;
		break;

	case DISP_FMT_ARGB:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;
		reg |= PPU_RGBA_MODE;				//YUYV
		misc |= ARGB_MODE;
		break;
	}

    R_PPU_MISC = misc;
	R_PPU_ENABLE = reg;
}

static void disp_gp420_enable(INT32U tft_index, INT32U enable)
{
	INT32U device_enable;

    if(tft_index == TFT_2)
        device_enable = TFT1_GP420_MODE;
	else
        device_enable = TFT0_GP420_MODE;

	if(enable) {
        R_PPU_YUV420OUT |= device_enable;
	} else {
        R_PPU_YUV420OUT &= ~device_enable;
	}
}

/*
static INT32U disp_gp420_get(INT32U tft_index)
{
	INT32U device_enable;

    if(tft_index == TFT_2)
        device_enable = TFT1_GP420_MODE;
	else
        device_enable = TFT0_GP420_MODE;

    if(R_PPU_YUV420OUT & device_enable)
        device_enable = 1;
    else
        device_enable = 0;

    return device_enable;
} */


static void ppu_set_enable(INT32U enable)
{
	if(enable)
	{
		R_PPU_ENABLE |= PPU_FRAME_BASE_MODE;
	}
	else
	{
		R_PPU_ENABLE = 0;
		R_PPU_IRQ_EN = 0;
		R_PPU_IRQ_STATUS = 0xFFFF;
	}
}

static INT32U ppu_enable_get(void)
{
	return  (R_PPU_ENABLE & 0x1);
}

static void ppu_tft_long_burst_enable(INT32U enable)
{
	if(enable)
	{
		R_PPU_ENABLE |= TFT_LB;
	}
	else
	{
		R_PPU_ENABLE &= ~TFT_LB;
	}
}

static void ppu_tv_long_burst_enable(INT32U enable)
{
	if(enable) {
		R_PPU_MISC |= TV_LB;
	} else {
		R_PPU_MISC &= ~TV_LB;
	}
}

static void ppu_ui_set_color_mode(UI_FMT color_mode)
{
	INT32U	reg = R_PPU_UI_CTRL;

	switch(color_mode)
	{
	case UI_FMT_RGB1555:
		reg &= ~PPU_UI_COLOR_MASK;
		reg &= ~PPU_UI_COLOR_ARGB4444;
		break;

	case UI_FMT_PALETTE_4:
		reg &= ~PPU_UI_COLOR_MASK;
		reg &= ~PPU_UI_COLOR_ARGB4444;
		reg |= PPU_UI_COLOR_PAL_4;
		break;

	case UI_FMT_PALETTE_16:
		reg &= ~PPU_UI_COLOR_MASK;
		reg &= ~PPU_UI_COLOR_ARGB4444;
		reg |= PPU_UI_COLOR_PAL_16;
		break;

	case UI_FMT_PALETTE_256:
		reg &= ~PPU_UI_COLOR_MASK;
		reg &= ~PPU_UI_COLOR_ARGB4444;
		reg |= PPU_UI_COLOR_PAL_256;
		break;

	case UI_FMT_ARGB444:
        reg &= ~PPU_UI_COLOR_MASK;
        reg |= PPU_UI_COLOR_PAL_256;
        reg |= PPU_UI_COLOR_ARGB4444;
		break;

	case UI_FMT_MAX:
        reg = 0;
        break;
	}

	R_PPU_UI_CTRL = reg;
}

static void ppu_ui_set_blend(INT8U blend_level)
{
	INT32U	reg = R_PPU_UI_CTRL;

	reg &= ~PPU_UI_BLEND_MASK;
	reg |= ((blend_level << PPU_UI_BLEND_BIT) & PPU_UI_BLEND_MASK);

	R_PPU_UI_CTRL = reg;
}

static void ppu_ui_set_mode(DISP_DEV disp_dev)
{
	if(disp_dev == DISDEV_TFT)
	{
		R_PPU_UI_CTRL &= ~PPU_UI_HDMI_MODE;
	}
	else
	{
		R_PPU_UI_CTRL |= PPU_UI_HDMI_MODE;
	}
}

static void ppu_ui_set_addr(INT32U addr)
{
	R_PPU_UI_ADDR = addr;
}

static void ppu_ui_set_enable(INT32U enable)
{
	if(enable)
	{
		R_PPU_UI_CTRL |= PPU_UI_ENABLE;
	}
	else
	{
		R_PPU_UI_CTRL &= ~PPU_UI_ENABLE;
	}
}

static void ppu_ui_pal_update(INT32U pal_addr)
{
	INT32U	temp, org_value = R_PPU_PALETTE_CTRL;

	R_PPU_ENABLE |= 0x1;
	temp = R_PPU_PALETTE_CTRL;
	temp &= ~0xC;
	R_PPU_PALETTE_CTRL = temp;

	// 0xD0501000
	R_PPU_SPRITE_DMA_TARGET = (INT32U) P_PPU_PALETTE_RAM0 & 0x7FFF;
	R_PPU_SPRITE_DMA_SOURCE = (INT32U) pal_addr;
	R_PPU_SPRITE_DMA_NUMBER = 0x100 - 1;
	while(R_PPU_SPRITE_DMA_NUMBER != 0);
	R_PPU_PALETTE_CTRL = org_value;
	R_PPU_ENABLE &= ~0x1;
}

static void tft2_gpio_set_enable(INT32U enable)
{
    INT32U temp = R_TFTIOCTRL;

    if(enable)
        temp |= (0x1 << 31);
    else
        temp &= ~(0x1 << 31);

    R_TFTIOCTRL = temp;
}

/**
 * @brief   init display
 * @param   none
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_init(void)
{
	INT32S ret;
    INT32U i;

#if DISP_OS_EN == 1
	osMessageQDef_t disp_q = { DISP_QUEUE_MAX, sizeof(INT32U), 0 };
#endif

    if(pDisp != 0)
    {
        ret = STATUS_OK;
        goto __exit;
    }

      for(i=0;i<DISDEV_NUM;i++)
    {
      switch(Dualdisplay.info.device[i])
      {
        case DISDEV_TFT_SPI0:
        case DISDEV_TFT:
          drv_l1_tft_init();
          if(Dualdisplay.info.device[1]==(INT32U)NULL)
            drv_l1_disp_dev_uninit(TFT_2);    // only 1 device , do uninit action
        break;
        case DISDEV_TFT_SPI1:
        case DISDEV_TFT2:
          drv_l1_disp_dev_init(TFT_2);
          if(Dualdisplay.info.device[1]==(INT32U)NULL)
            drv_l1_tft_uninit();   // only 1 device , do uninit action
        break;

        default:
         if(Dualdisplay.info.device[i]!=(INT32U)NULL)
         DBG_PRINT("Wrong display device type \r\n");
        break;

      }


    }


	ppu_set_enable(DISABLE);

	pDisp = &display;
	memset((INT8S *) pDisp, 0x00, sizeof(display_t));

	if((TFT_Param->tft_rot != TFT_ROT_NONE)
        && (drv_l2_display_lcd_ifce_get() == LCD_IFCE_RGB))
	{
        pDisp->rot2_LB_init = 1;
        pDisp->rot2_disp_buf = DUMMY_BUFFER_ADDRS;
    }
    else
    {
        pDisp->rot2_LB_init = 0;
    }

#if DISP_OS_EN == 1
	pDisp->tft_q = osMessageCreate(&disp_q, NULL);
	if(pDisp->tft_q == 0)
	{
		ret = STATUS_FAIL;
		goto __exit;
	}

	pDisp->tft2_q = osMessageCreate(&disp_q, NULL);
	if(pDisp->tft2_q == 0)
	{
		ret = STATUS_FAIL;
		goto __exit;
	}

	pDisp->rot2_q = osMessageCreate(&disp_q, NULL);
	if(pDisp->rot2_q == 0)
	{
		ret = STATUS_FAIL;
		goto __exit;
	}
#endif

	if(drv_l2_display_lcd_ifce_get() != LCD_IFCE_I80)
	{
		vic_irq_register(VIC_PPU, PPU_IRQHandler);
		vic_irq_enable(VIC_PPU);
	}

	ret = STATUS_OK;
__exit:
	if(ret < 0)
	{
		drv_l2_display_uninit();
	}

	return ret;
}

/**
 * @brief   uninit display
 * @param   none
 * @return 	none
 */
void drv_l2_display_uninit(void)
{
	if(pDisp == 0)
	{
		return;
	}


    if(DISPLAY_DEVICE == DISDEV_TFT)
    {
        drv_l1_tft_uninit();
    }
    else
    {
        drv_l1_disp_dev_uninit(TFT_2);
    }

    drv_l2_display_stop();

    vic_irq_disable(VIC_PPU);
    vic_irq_register(VIC_PPU, NULL);

#if DISP_OS_EN == 1
    osMessageDelete(pDisp->tft_q);
    osMessageDelete(pDisp->tft2_q);
    osMessageDelete(pDisp->rot2_q);
#endif

	pDisp = 0;
}

static __inline INT16U _drv_l2_display_tft_width_get(void)
{
	return TFT_Param->tft_width ? TFT_Param->tft_width : TFT_Param->lcd_width;
}
static __inline INT16U _drv_l2_display_tft_height_get(void)
{
	return TFT_Param->tft_height ? TFT_Param->tft_height : TFT_Param->lcd_height;
}

/**
 * @brief   display get tft size
 * @param   none
 * @return 	width
 */
INT16U drv_l2_display_tft_width_get(void)
{
	switch(TFT_Param->tft_rot)
	{
		case TFT_ROT_NONE:
		case TFT_ROT_180:
		case TFT_ROT_H_MIRROR:
		case TFT_ROT_V_MIRROR:
			return _drv_l2_display_tft_width_get();
            break;

		default:
		case TFT_ROT_RIGHT:
		case TFT_ROT_LEFT:
			return _drv_l2_display_tft_height_get();
            break;
	}
}

/**
 * @brief   display get tft size
 * @param   none
 * @return 	height
 */
INT16U drv_l2_display_tft_height_get(void)
{
	switch(TFT_Param->tft_rot)
	{
		case TFT_ROT_NONE:
		case TFT_ROT_180:
		case TFT_ROT_H_MIRROR:
		case TFT_ROT_V_MIRROR:
			return _drv_l2_display_tft_height_get();
            break;

		default:
		case TFT_ROT_RIGHT:
		case TFT_ROT_LEFT:
			return _drv_l2_display_tft_width_get();
            break;
	}
}

DISP_FMT drv_l2_display_tft_fmt_get(void)
{
	return TFT_Param->tft_fmt;
}

TFT_ROT drv_l2_display_tft_rot_get(void)
{
	return TFT_Param->tft_rot;
}

LCD_IFCE drv_l2_display_lcd_ifce_get(void)
{
	return TFT_Param->lcd_ifce;
}

INT16U drv_l2_display_off_lines_video_get(void)
{
	return TFT_Param->h_off_lines_video;
}

INT16U drv_l2_display_off_lines_cap_get(void)
{
	return TFT_Param->h_off_lines_cap;
}

INT8U drv_l2_display_en_crop_video_get(void)
{
	return TFT_Param->en_crop_video;
}

INT8U drv_l2_display_en_crop_cap_get(void)
{
	return TFT_Param->en_crop_cap;
}

INT8U drv_l2_display_pbp_crop_video_get(void)
{
	return TFT_Param->pbp_crop_video;
}

INT8U drv_l2_display_pbp_crop_cap_get(void)
{
	return TFT_Param->pbp_crop_cap;
}

INT8U drv_l2_display_back_ratio_video_get(void)
{
	return TFT_Param->back_ratio_video;
}

INT8U drv_l2_display_back_ratio_cap_get(void)
{
	return TFT_Param->back_ratio_cap;
}

INT32S drv_l2_display_set_fmt(DISP_FMT color_mode)
{
     switch(Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info))
     {
       case DISDEV_TFT:
	   case DISDEV_TFT_SPI0:
		if(color_mode == DISP_FMT_GP420)
		{
       //     pDisp->tft_color_fmt = DISP_FMT_YUYV;
            disp_gp420_enable(DISDEV_TFT, 1);
		}
		else
		{
            disp_gp420_enable(DISDEV_TFT, 0);
         //   pDisp->tft_color_fmt = color_mode;
		}
        if(color_mode == DISP_FMT_GP420)
            ppu_set_color_mode(DISP_FMT_YUYV);
        else
            ppu_set_color_mode(color_mode);
		break;
	   default:
		return STATUS_FAIL;


     }
     return STATUS_OK;
}


/**
 * @brief   display start
 * @param   isr routine
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_start(TFT_ISR isr)
{
	DISP_FMT color_mode;

    if(pDisp == (display_t *)NULL)          return STATUS_FAIL;
    if(TFT_Param == (DispCtrl_t *)NULL)     return STATUS_FAIL;

	if(drv_l2_display_lcd_ifce_get() != LCD_IFCE_I80)
	{
		tft_isr_vblank_callback = isr;
		spitft_isr_vblank_callback = isr;
	}
	else
	{
		tft_isr_vblank_callback = NULL;
	}

	color_mode = drv_l2_display_tft_fmt_get();


    switch(Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info))
	{
	case DISDEV_TFT:
		if(color_mode == DISP_FMT_GP420)
		{
            disp_gp420_enable(TFT_1, 1);
		}
		else
		{
            disp_gp420_enable(TFT_1, 0);
		}

        drv_l1_disp_dev_sync_de_te_set(TFT_1, TFT_Param->sync_pin_enable, TFT_Param->de_pin_enable, TFT_Param->te_pin_enable);

		drv_l2_tft_init(TFT_1);

		ppu_set_size(TFT_Param->lcd_width, TFT_Param->lcd_height);

		if(color_mode == DISP_FMT_GP420)
		{
			ppu_set_color_mode(DISP_FMT_YUYV);
		}
		else
		{
			ppu_set_color_mode(color_mode);
		}

        if((TFT_Param->lcd_width % 64) == 0)
        {
            ppu_tft_long_burst_enable(ENABLE);
        }
        else
        {
            ppu_tft_long_burst_enable(DISABLE);
        }

        ppu_set_enable(ENABLE);

		R_PPU_IRQ_STATUS = PPU_TFT_V_BLANKING;
        if(drv_l2_display_lcd_ifce_get() != LCD_IFCE_I80)
        {
            R_PPU_IRQ_EN |= PPU_TFT_V_BLANKING;
        }

		#if DISP_UNDER_RUN_EN
		R_PPU_IRQ_STATUS = PPU_TFT_UNDER_RUN;
		R_PPU_IRQ_EN |= PPU_TFT_UNDER_RUN;
		#endif

		pDisp->disp_dev_init = DISDEV_TFT;
		break;
	case DISDEV_TFT2:
		if(color_mode == DISP_FMT_GP420)
		{
            disp_gp420_enable(TFT_2, 1);
		}
		else
		{
            disp_gp420_enable(TFT_2, 0);
		}

		if((TFT_Param->lcd_width > _drv_l2_display_tft_width_get())
			|| (TFT_Param->lcd_height > _drv_l2_display_tft_height_get())
		)
		{
			drv_l1_tft2_scaling_up_size_set(
				_drv_l2_display_tft_width_get()
				, _drv_l2_display_tft_height_get()
				, 0
				, 1
				, TFT_Param->lcd_width
				, TFT_Param->lcd_height
			);
		}

		drv_l1_disp_dev_sync_de_te_set(TFT_2, TFT_Param->sync_pin_enable, TFT_Param->de_pin_enable, TFT_Param->te_pin_enable);
        if(TFT_Param->tft2_io_enable)
        {
            tft2_gpio_set_enable(1);
        }
        else
        {
            tft2_gpio_set_enable(0);
        }

		drv_l2_tft_init(TFT_2);

		if(pDisp->rot2_LB_init)
		{
            drv_l1_rotator_init(ROTATOR_2);
			drv_l1_rotator_high_priority_set(ROTATOR_2, 1, HIGH_PRY_LEVEL_3);

            switch(TFT_Param->tft_rot)
            {
            case TFT_ROT_RIGHT:
                drv_l1_rotator_mode_set(ROTATOR_2, ROTATOR_90);
                break;

            case TFT_ROT_LEFT:
                drv_l1_rotator_mode_set(ROTATOR_2, ROTATOR_270);
                break;

            case TFT_ROT_180:
                drv_l1_rotator_mode_set(ROTATOR_2, ROTATOR_180);
                break;

            case TFT_ROT_H_MIRROR:
                drv_l1_rotator_mode_set(ROTATOR_2, ROTATOR_HORIZONTAL_MIRROR);
                break;

            case TFT_ROT_V_MIRROR:
                drv_l1_rotator_mode_set(ROTATOR_2, ROTATOR_VERTICAL_MIRROR);
                break;

            default:
                drv_l1_rotator_mode_set(ROTATOR_2, ROTATOR_THETA);
                break;
            }

            if(color_mode == DISP_FMT_GP420)
            {
                drv_l1_rotator_src_img_info(ROTATOR_2, IMAGE_GP420, drv_l2_display_tft_width_get(), drv_l2_display_tft_height_get(), pDisp->rot2_disp_buf);
                drv_l1_rotator_ppu420_set(ROTATOR_2, 1);
            }
            else if(color_mode == DISP_FMT_RGB565)
            {
                drv_l1_rotator_src_img_info(ROTATOR_2, IMAGE_RGB565, drv_l2_display_tft_width_get(), drv_l2_display_tft_height_get(), pDisp->rot2_disp_buf);
            }
            else
            {
                drv_l1_rotator_src_img_info(ROTATOR_2, IMAGE_YUYV, drv_l2_display_tft_width_get(), drv_l2_display_tft_height_get(), pDisp->rot2_disp_buf);
            }

            /*
            if(_drv_l2_display_tft_height_get() < 360)
            {
               drv_l1_rotator_burst16_set(ROTATOR_2, 1);
            }
            else */
            {
               drv_l1_rotator_burst16_set(ROTATOR_2, 0);
            }

            drv_l1_rotator_auto_high_priority_set(ROTATOR_2, 1);
            drv_l1_rotator_dram2tft_set(ROTATOR_2, 1);

            if(drv_l2_display_lcd_ifce_get() != LCD_IFCE_I80)
            {
                drv_l1_rotator_isr_callback_set(ROTATOR_2, Rotator2_LB_Callback);
            }

            drv_l1_rotator_start(ROTATOR_2);
		}

		ppu_set_size(_drv_l2_display_tft_width_get(), _drv_l2_display_tft_height_get());

		if(color_mode == DISP_FMT_GP420)
		{
			ppu_set_color_mode(DISP_FMT_YUYV);
		}
		else
		{
			ppu_set_color_mode(color_mode);
		}

		//When TFT2_H_FACTOR or TFT2_V_FACTOR �� 0, TV_LONG_BURST(TV_LB) cannot be 1,
		//otherwise there might be malfunction at some resolutions.
        if(((_drv_l2_display_tft_width_get() % 64) == 0)
            && (TFT_Param->lcd_width == _drv_l2_display_tft_width_get())
            && (TFT_Param->lcd_height == _drv_l2_display_tft_height_get())
		)
        {
            ppu_tv_long_burst_enable(ENABLE);
        }
        else
        {
            ppu_tv_long_burst_enable(DISABLE);
        }

		ppu_set_enable(ENABLE);

		R_PPU_IRQ_STATUS = PPU_TFT1_V_BLANKING;
        if((drv_l2_display_lcd_ifce_get() != LCD_IFCE_I80) && (pDisp->rot2_LB_init == 0))
        {
            R_PPU_IRQ_EN |= PPU_TFT1_V_BLANKING;
        }

		#if DISP_UNDER_RUN_EN
		R_PPU_IRQ_STATUS = PPU_TFT1_UNDER_RUN;
		R_PPU_IRQ_EN |= PPU_TFT1_UNDER_RUN;
		#endif

		pDisp->disp_dev_init = DISDEV_TFT2;
		break;
    case DISDEV_TFT_SPI0:
    case DISDEV_TFT_SPI1:
       drv_l2_display_set_fmt(color_mode);
      if(Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info)==DISDEV_TFT_SPI0)
      {
           if(!drv_l1_tft_spi_enable_status(TFT_1))
             drv_l2_tft_init(TFT_1);
      }
      if(Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info)==DISDEV_TFT_SPI1)
      {
           if(!drv_l1_tft_spi_enable_status(TFT_2))
             drv_l2_tft_init(TFT_2);
      }
       ppu_set_size(_drv_l2_display_tft_width_get(), _drv_l2_display_tft_height_get());
       if((_drv_l2_display_tft_width_get() % 64) == 0)
       {
          if(Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info)==DISDEV_TFT_SPI0)
             ppu_tft_long_burst_enable(ENABLE);
          if(Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info)==DISDEV_TFT_SPI1)
             ppu_tv_long_burst_enable(ENABLE);
       }
       else
       {
          if(Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info)==DISDEV_TFT_SPI0)
             ppu_tft_long_burst_enable(DISABLE);
          if(Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info)==DISDEV_TFT_SPI1)
             ppu_tv_long_burst_enable(DISABLE);

       }
       ppu_set_enable(ENABLE);

       if(Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info)==DISDEV_TFT_SPI0)
          pDisp->disp_dev_init = DISDEV_TFT_SPI0;
       if(Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info)==DISDEV_TFT_SPI1)
          pDisp->disp_dev_init = DISDEV_TFT_SPI1;
    break;
	default:
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

/**
 * @brief   display stop
 * @param   disp_dev[in]: display device, see DISP_DEV.
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_stop(void)
{
    if(pDisp == (display_t *)NULL)   return  STATUS_FAIL;

	if(pDisp->disp_dev_init == DISDEV_TFT)
	{
		drv_l1_tft_en_set(DISABLE);
		R_PPU_IRQ_EN &= ~PPU_TFT_V_BLANKING;
		R_PPU_IRQ_STATUS = PPU_TFT_V_BLANKING;

		#if DISP_UNDER_RUN_EN
		R_PPU_IRQ_EN &= ~PPU_TFT_UNDER_RUN;
		R_PPU_IRQ_STATUS = PPU_TFT_UNDER_RUN;
		#endif

        ppu_set_enable(DISABLE);
        drv_l2_tft_uninit(TFT_1);

        pDisp->disp_dev_init = 0;
        return STATUS_OK;
    }
	else if(pDisp->disp_dev_init == DISDEV_TFT2)
	{
       if(pDisp->rot2_LB_init)
        {
            drv_l1_rotator_enable_set(ROTATOR_2, 0);
            drv_l1_rotator_dram2tft_set(ROTATOR_2, 0);
            drv_l1_rotator_uninit(ROTATOR_2);
        }

		drv_l1_disp_dev_en_set(TFT_2, DISABLE);
		R_PPU_IRQ_EN &= ~PPU_TFT1_V_BLANKING;
		R_PPU_IRQ_STATUS = PPU_TFT1_V_BLANKING;

        #if DISP_UNDER_RUN_EN
        R_PPU_IRQ_EN &= ~PPU_TFT1_UNDER_RUN;
        R_PPU_IRQ_STATUS = PPU_TFT1_UNDER_RUN;
        #endif

        ppu_set_enable(DISABLE);
        drv_l2_tft_uninit(TFT_2);

        pDisp->disp_dev_init = 0;
        return STATUS_OK;
    }
	return STATUS_FAIL;
}

#if 0

/**
 * @brief   display update frame buffer
 * @param   buffer[in]: frame buffer address
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_update(INT32U buffer)
{
#if DISP_OS_EN == 1
    /*
	INT32S	message;
	INT8U	error;
	*/

	osEvent result;
#endif
	if((buffer == 0) || (pDisp == (display_t *)NULL))
	{
		return STATUS_FAIL;
	}

	switch(DISPLAY_DEVICE)
	{
	case DISDEV_TFT:
		pDisp->tft_disp_buf = buffer;
		pDisp->tft_update_flag = 1;
    #if DISP_OS_EN == 1
		result = osMessageGet(pDisp->tft_q, osWaitForever);
		if((result.status != osEventMessage) || (result.value.v != DISP_UPDATE_DONE))
		{
			return STATUS_FAIL;
		}
    #else
		while(pDisp->tft_update_flag);
    #endif
		break;

	case DISDEV_TFT2:
		pDisp->tft2_disp_buf = buffer;
		pDisp->tft2_update_flag = 1;

    #if DISP_OS_EN == 1
        /*
		message = (INT32S) OSQPend(pDisp->tft2_q, 5 * OS_TICKS_PER_SEC, &error);
		if(error != OS_NO_ERR || message != DISP_UPDATE_DONE)
			return STATUS_FAIL;
        */

		result = osMessageGet(pDisp->tft2_q, osWaitForever);
		if((result.status != osEventMessage) || (result.value.v != DISP_UPDATE_DONE)) {
			return STATUS_FAIL;
		}
    #else
		while(pDisp->tft2_update_flag);
    #endif
		break;

	default:
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

/**
 * @brief   display update frame buffer without wait IRQ end
 * @param   buffer[in]: frame buffer address
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_buffer_set(INT32U buffer)
{
	if((buffer == 0) || (pDisp == (display_t *)NULL))
	{
		return STATUS_FAIL;
	}

    drv_l2_display_buffer_update_state_get(1);

	switch(DISPLAY_DEVICE)
	{
	case DISDEV_TFT:
		pDisp->tft_disp_buf = buffer;
		pDisp->tft_update_flag = 1;
		break;

	case DISDEV_TFT2:
		pDisp->tft2_disp_buf = buffer;
		pDisp->tft2_update_flag = 1;
		break;

	default:
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

/**
 * @brief   display update frame buffer state
 * @param   buffer[in]: frame buffer address
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_buffer_update_state_get(INT32U wait)
{
#if DISP_OS_EN == 1
    /*
	INT32S	message;
	INT8U	error;
	*/

	osEvent result;
#endif

	if(pDisp == (display_t *)NULL)  return STATUS_FAIL;

	switch(DISPLAY_DEVICE)
	{
	case DISDEV_TFT:
    #if DISP_OS_EN == 1
		if(wait)
			result = osMessageGet(pDisp->tft_q, osWaitForever);
		else
			result = osMessageGet(pDisp->tft_q, 100);
		if((result.status != osEventMessage) || (result.value.v != DISP_UPDATE_DONE))
		{
			return STATUS_FAIL;
		}

    #else
		while(pDisp->tft_update_flag);
    #endif
		break;

	case DISDEV_TFT2:
    #if DISP_OS_EN == 1
        /*
		message = (INT32S) OSQPend(pDisp->tft2_q, 5 * OS_TICKS_PER_SEC, &error);
		if(error != OS_NO_ERR)
			return STATUS_FAIL;
        */

		if(wait)
			result = osMessageGet(pDisp->tft2_q, osWaitForever);
		else
			result = osMessageGet(pDisp->tft2_q, 100);
		if((result.status != osEventMessage) || (result.value.v != DISP_UPDATE_DONE)) {
			return STATUS_FAIL;
		}
    #else
		while(pDisp->tft_update_flag);
    #endif
		break;

	default:
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

#endif

/**
 * @brief   init display UI
 * @param   none
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_UI_init(UI_FMT color_mode, INT8U blend_level, INT32U ui_addr, INT32U pal_addr)
{
	INT16U tft_w;
	INT16U tft_h;

	if(ui_addr == 0)
		return STATUS_FAIL;

	ppu_ui_set_mode(DISPLAY_DEVICE);
	ppu_ui_set_color_mode(color_mode);

	if(ppu_enable_get() == 0)
	{
		if(ui_addr && (color_mode != UI_FMT_RGB1555))
			ppu_ui_pal_update(pal_addr);
	}

	tft_w = drv_l2_display_tft_width_get();
	tft_h = drv_l2_display_tft_height_get();

	ppu_set_size(tft_w, tft_h);
	ppu_ui_set_blend(blend_level);
	ppu_ui_set_addr(ui_addr);
	ppu_ui_set_enable(1);

	return 0;
}

/**
 * @brief   uninit display UI
 * @param   none
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_UI_uninit(void)
{
	ppu_ui_set_addr(0);
	ppu_ui_set_enable(0);

	return 0;
}

/**
 * @brief   display update ui frame buffer and blend level
 * @param   blend_level[in]: blend level
 * @param   ui_addr[in]: frame buffer address
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_UI_update(INT8U blend_level, INT32U ui_addr, INT32U pal_addr)
{
	if(ppu_enable_get() == 0)
	{
		if(ui_addr)
			ppu_ui_pal_update(pal_addr);
	}

	if(ui_addr)
		ppu_ui_set_addr(ui_addr);
	ppu_ui_set_blend(blend_level);

	return 0;
}

/**
 * @brief   display UI stop
 * @param   none.
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_UI_stop(void)
{
	ppu_ui_set_enable(0);
	return 0;
}

INT16U drv_l2_display_pip_width_get(void)
{
	return TFT_Param->pip_width;
}

INT16U drv_l2_display_pip_height_get(void)
{
	return TFT_Param->pip_height;
}

INT16U drv_l2_display_pip_x_get(void)
{
	if(TFT_Param->pip_pos_x < 0)
	{
		return drv_l2_display_tft_width_get() + TFT_Param->pip_pos_x;
	}
	else
	{
		return TFT_Param->pip_pos_x;
	}
}

INT16U drv_l2_display_pip_y_get(void)
{
	if(TFT_Param->pip_pos_y < 0)
	{
		return drv_l2_display_tft_height_get() + TFT_Param->pip_pos_y;
	}
	else
	{
		return TFT_Param->pip_pos_y;
	}
}


/**
 * @brief   Register ppu call back function
 * @param   callback[in]: callback function
 * @return 	0: Success, -1: Fail
 */
INT32S drv_l2_ppu_isr_callback_set(void (*callback) (INT32U ppu_event))
{
	ppu_isr_callback = callback;
	return 0;
}


INT32S drv_l2_tft_buffer_set(INT32U buffer)
{
  INT32U device;
    if(pDisp == (display_t *)NULL)          return  STATUS_FAIL;


   device = Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info);
    if((device==DISDEV_TFT)||(device==DISDEV_TFT_SPI0))
	{
        R_TFT_FBI_ADDR = buffer;
    }

    else if((device == DISDEV_TFT2)||(device==DISDEV_TFT_SPI1))
	{
		if(pDisp->rot2_LB_init)
		{
			pDisp->rot2_disp_buf = buffer;
			drv_l1_rotator_src_img_addr(ROTATOR_2, buffer);
			R_TV_FBI_ADDR = 0;
		}
		else
		{
			R_TV_FBI_ADDR = buffer;
		}
	}
	return STATUS_OK;

}

INT32U drv_l2_tft_buffer_get(void)
{
    if(pDisp == (display_t *)NULL)   return  (INT32U)NULL;

    if(DISPLAY_DEVICE == DISDEV_TFT)
	{
		return R_TFT_FBI_ADDR;
    }
	else if(DISPLAY_DEVICE == DISDEV_TFT2)
	{
		if(pDisp->rot2_LB_init)
        {
            return pDisp->rot2_disp_buf;
        }
        else
        {
            return R_TV_FBI_ADDR;
        }
    }

    return (INT32U)NULL;
}

void drv_l2_tft_init(TFT_DEV_ID tft_idx)
{
	if(TFT_Param && TFT_Param->init)
	{
		TFT_Param->init(tft_idx);
	}
}

void drv_l2_tft_uninit(TFT_DEV_ID tft_idx)
{
	if(TFT_Param && TFT_Param->uninit)
	{
		TFT_Param->uninit(tft_idx);
	}
}

void drv_l2_disp_dev_reinit(void)
{
	INT32U dev_mun;
 	dev_mun = Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info);

	switch(dev_mun)
	{
		case  DISDEV_TFT:
			drv_l1_disp_dev_init(TFT_1);
			break;

		case DISDEV_TFT2:
			drv_l1_disp_dev_init(TFT_2);
			break;

		default:
			break;
	}


}


void drv_l2_disp_dev_uninit(void)
{
	INT32U dev_mun;
 	dev_mun = Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info);

	switch(dev_mun)
	{
		case  DISDEV_TFT:
			drv_l1_disp_dev_uninit(TFT_1);
			break;

		case DISDEV_TFT2:
			drv_l1_disp_dev_uninit(TFT_2);
			break;

		default:
			break;
	}

}

void drv_l2_tft_busy_wait(void)
{
    if(pDisp == (display_t *)NULL)   return;

    if(DISPLAY_DEVICE == DISDEV_TFT)
	{
		while(R_TFT_STATUS & 0x00f0)
		{
            osDelay(1);
		}
    }
	else if(DISPLAY_DEVICE == DISDEV_TFT2)
	{
		R_TFT2_CTRL |= (1ul << 31);     //switching to bank1
		while(R_TFT2_STATUS & 0xf0000000)
		{
            osDelay(1);
		}
		R_TFT2_CTRL &= ~(1ul << 31);    //switching to bank0
    }
}

void drv_l2_tft_en_set(INT32U en)
{
    if(pDisp == (display_t *)NULL)   return;

    if(DISPLAY_DEVICE == DISDEV_TFT)
	{
		drv_l1_disp_dev_en_set(TFT_1, en);
    }
	else if(DISPLAY_DEVICE == DISDEV_TFT2)
	{
		drv_l1_disp_dev_en_set(TFT_2, en);
    }
}

void drv_l2_disable_te_pin(void)
{

	INT32U dev_mun;
 	dev_mun = Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info);

	switch(dev_mun)
	{
		case  DISDEV_TFT:
				drv_l1_tft_tepin_disable(TFT_1);
			break;

		case DISDEV_TFT2:
				drv_l1_tft_tepin_disable(TFT_2);
			break;

		default:
			break;
	}
 }

/* Keep the display task's backlight-on request pending until Wi-Fi/BT starts. */
INT32U g_bl_delay_enable = 0;

void drv_l2_tft_backlight_en_set(BOOLEAN enable)
{
	if(enable == TRUE && g_bl_delay_enable == 0)
	{
		DRV_DISP_DBG("[BL-DBG] backlight ON deferred\r\n");
		return;
	}
	if(TFT_Param && TFT_Param->lcd_bl_ctrl == LCD_BL_CTRL_PWM)
	{
		if(enable)
		{
			// PWM_HZ ���A?�����l�� , PWM_DUTY �� duty , 0:��?��high or low
			timer_pwm_setup(
				TIMER_B
				, TFT_Param->lcd_pwm_freq
				, TFT_Param->lcd_pwm_duty
				,0
			);
		}
		else
		{
			// �P�� PWM
			//timer_pwm_stop(TIMER_F);
			timer_stop(TIMER_B);

			#if !C_ASYN_DBLE_TFT
			#ifdef PANEL_IO_BL_ON
			gpio_write_io(PANEL_IO_BL_ON, DATA_LOW);
			#endif

			#ifdef PANEL_IO_BL2_ON
			gpio_write_io(PANEL_IO_BL2_ON, DATA_LOW);
			#endif
			#endif

		}
	}
	else
	{
		#if !C_ASYN_DBLE_TFT
		#ifdef PANEL_IO_BL_ON
		gpio_write_io(PANEL_IO_BL_ON, enable ? DATA_HIGH : DATA_LOW);
		#endif

		#ifdef PANEL_IO_BL2_ON
		gpio_write_io(PANEL_IO_BL2_ON, enable ? DATA_HIGH : DATA_LOW);
		#endif
		#endif

	}
}


#if C_ASYN_DBLE_TFT
void drv_l2_asyn_multi_tft_hdmi_plug_status_set(INT32U status)	/*set 1 -> HDMI plug in ; set 0 -> HDMI plug out */
{
	if(status>=1)
	{
		Dualdisplay.info.hdmi_plugin_flag=1;	/* HDMI plug in */
	}
	else
	{
		Dualdisplay.info.hdmi_plugin_flag=0;	/* HDMI plug out */
	}
}

INT32U drv_l2_asyn_multi_tft_hdmi_plug_status_get(void)	/* get HDMI plug in/out status */
{
	return Dualdisplay.info.hdmi_plugin_flag;
}

INT32U drv_l2_asyn_multi_tft_maximum_width_get(void)	/* return minimum tft height */
{
	return Dualdisplay.info.multi_tft_max_width;
}

INT32U drv_l2_asyn_multi_tft_maximum_height_get(void)	/* return maximum tft height */
{
	return Dualdisplay.info.multi_tft_max_height;
}

void drv_l2_asyn_double_tft_backlight_set(INT32U en)
{
	Dualdisplay.info.tft_switch_back_light_flag=en;		/*  enable=1 ; disable=0 */
}

INT32U drv_l2_asyn_double_tft_backligh_get(void)
{
	return Dualdisplay.info.tft_switch_back_light_flag;;
}

INT32U drv_l2_previous_display_width(void)
{
	return Dualdisplay.info.previous_display_width;
}

INT32U previous_h_off_lines_video(void)
{
	return Dualdisplay.info.previous_h_off_lines_video;
}

INT32U previous_tft_fmt(void)
{
	return Dualdisplay.info.previous_tft_fmt;
}



void drv_l2_asyn_double_tft_backlight_en_set(BOOLEAN display1_bl_enable,BOOLEAN display2_bl_enable)
{
	if(display1_bl_enable == TRUE && g_bl_delay_enable == 0)
	{
		DRV_DISP_DBG("[BL-DBG] async backlight ON deferred\r\n");
		display1_bl_enable = FALSE;
	}
	#ifdef PANEL_IO_BL_ON
		gpio_write_io(PANEL_IO_BL_ON, display1_bl_enable ? DATA_HIGH : DATA_LOW);
	#endif

	#ifdef PANEL_IO_BL2_ON
		gpio_write_io(PANEL_IO_BL2_ON, display2_bl_enable ? DATA_HIGH : DATA_LOW);
	#endif
}

void drv_l2_asyn_double_tft_backlight_switch(void)
{
	if(drv_l2_asyn_double_tft_backligh_get())
	{
		Dualdisplay.info.tft_switch_back_light_cnt++;
		if(Dualdisplay.info.tft_switch_back_light_cnt>=TFT_BUF_NUM)
		{
			#if LCD_DRV_EN_st7789_I80 && LCD_DRV_EN_st7789_I80_240x240
				if(get_st7789_display_for_double_TFT() && !get_st7789_240x240_display_for_double_TFT())
				{
					drv_l2_asyn_double_tft_backlight_en_set(TRUE,FALSE);
				}
				else if(!get_st7789_display_for_double_TFT() && get_st7789_240x240_display_for_double_TFT())
				{
					drv_l2_asyn_double_tft_backlight_en_set(FALSE,TRUE);
				}
				else if(!get_st7789_display_for_double_TFT() && !get_st7789_240x240_display_for_double_TFT())
				{
					drv_l2_asyn_double_tft_backlight_en_set(FALSE,FALSE);	/* HDMI enable , 2 of tft disable backlight */
				}
			#elif LCD_DRV_EN_KD055HD_IL9881C
				if(get_KD055HD_IL9881C_display_for_double_TFT())
				{
					drv_l2_asyn_double_tft_backlight_en_set(TRUE,FALSE);	/* enable */
				}
				else
				{
					drv_l2_asyn_double_tft_backlight_en_set(FALSE,FALSE);	/* disable */
				}
			#else
			drv_l2_asyn_double_tft_backlight_en_set(TRUE,TRUE);	/* all enable */
			#endif


			Dualdisplay.info.tft_switch_back_light_cnt=0;	/* clear cnt */
			drv_l2_asyn_double_tft_backlight_set(0);	/* set backlight disable */

		}
	}
}

void drv_l2_asyn_multi_tft_init(void)
{
	INT32U dev_mun;
	INT32S ret;
	#if DISP_OS_EN == 1
	osMessageQDef_t disp_q = { DISP_QUEUE_MAX, sizeof(INT32U), 0 };
	#endif

 	dev_mun = Dualdisplay.current_device_get((gDisplaydevice_para_t*)&Dualdisplay.info);

	switch(dev_mun)
	{
		case  DISDEV_TFT:
			drv_l1_disp_dev_init(TFT_1);
			break;

		case DISDEV_TFT2:
			drv_l1_disp_dev_init(TFT_2);
			break;

		default:
			break;
	}
		ppu_set_enable(DISABLE);

		pDisp = &display;
		memset((INT8S *) pDisp, 0x00, sizeof(display_t));

		if((TFT_Param->tft_rot != TFT_ROT_NONE)
	        && (drv_l2_display_lcd_ifce_get() == LCD_IFCE_RGB))
		{
	        pDisp->rot2_LB_init = 1;
	        pDisp->rot2_disp_buf = DUMMY_BUFFER_ADDRS;
	    }
	    else
	    {
	        pDisp->rot2_LB_init = 0;
	    }

#if DISP_OS_EN == 1
		pDisp->tft_q = osMessageCreate(&disp_q, NULL);
		if(pDisp->tft_q == 0)
		{
			ret = STATUS_FAIL;
			goto __exit;
		}

		pDisp->tft2_q = osMessageCreate(&disp_q, NULL);
		if(pDisp->tft2_q == 0)
		{
			ret = STATUS_FAIL;
			goto __exit;
		}

		pDisp->rot2_q = osMessageCreate(&disp_q, NULL);
		if(pDisp->rot2_q == 0)
		{
			ret = STATUS_FAIL;
			goto __exit;
		}
#endif

		if(drv_l2_display_lcd_ifce_get() != LCD_IFCE_I80)
		{
			vic_irq_register(VIC_PPU, PPU_IRQHandler);
			vic_irq_enable(VIC_PPU);
		}

		ret = STATUS_OK;

__exit:
	if(ret < 0)
	{
		drv_l2_display_uninit();
	}

	return ret;
}
#endif



#if (defined C_HDMI_SCALE_UP_EN) && (C_HDMI_SCALE_UP_EN)
void
drv_l2_display_tft_width_reset(
	INT16U new_width
)
{
	TFT_Param->tft_width = new_width;
}

void
drv_l2_display_tft_height_reset(
	INT16U new_height
)
{
	TFT_Param->tft_height = new_height;
}
#endif

#endif //#if (defined(TFT1_ENABLE) && TFT1_ENABLE) || (defined(TFT2_ENABLE) && TFT2_ENABLE)

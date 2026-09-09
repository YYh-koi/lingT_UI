/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
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
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park,                      *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#include "application.h"

#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
#include "asr.h"
#include "ap_voice_recg_func.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
typedef enum _ID_Translate_Table
{
   ID_RECORD_START = 0,
   ID_RECORD_STOP,
   ID_OPEN_PANEL,
   ID_CLOSE_PANEL,
   ID_DISPLAY_FRONT,
   ID_DISPLAY_RAER,
   ID_DISPLAY_PIP,
   ID_OPEN_MIC,
   ID_CLOSE_MIC,
   ID_EMERGENCY_RECORD,
   ID_CAPTURE

}ID_Translate_Table;

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct VOICE_RECG_CTRL_S{
	int	 (*voice_recg_start_func)(void);
	int	 (*voice_recg_stop_func)(void);
	int  (*voice_recg_process_func)(short* samples, int n);
	void (*voice_recg_action_func)(ID_Translate_Table actionID);
}VOICE_RECG_CTRL_T;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
#if (VOICE_RECG_MODULE == VOICE_RECG_WANSON)
int voice_recg_wanson_start(void);
int voice_recg_wanson_stop(void);
int voice_recg_wanson_process(short* samples,int n);
#endif

void ID_Translate_Function(ID_Translate_Table actionID);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static VOICE_RECG_CTRL_T voice_recg_module =
{
#if (VOICE_RECG_MODULE == VOICE_RECG_WANSON)
	voice_recg_wanson_start,
	voice_recg_wanson_stop,
	voice_recg_wanson_process,
#endif

//Common function
	ID_Translate_Function
};

VOICE_RECG_CTRL_T* pVoiceRecgCtrl = &voice_recg_module;


#if (VOICE_RECG_MODULE == VOICE_RECG_WANSON)
/* UTF-8 type */
char awson_cmd_start_record[]   = {0xE5,0xBC,0x80,0xE5,0xA7,0x8B,0xE5,0xBD,0x95,0xE5,0x83,0x8F,'\0'};//"开始录像"
char awson_cmd_stop_record[]    = {0xE5,0x81,0x9C,0xE6,0xAD,0xA2,0xE5,0xBD,0x95,0xE5,0x83,0x8F,'\0'};//"停止录像"
char awson_cmd_turnon_display[] = {0xE6,0x89,0x93,0xE5,0xBC,0x80,0xE5,0xB1,0x8F,0xE5,0xB9,0x95,'\0'};//"打开屏幕"
char awson_cmd_turnoff_display[]= {0xE5,0x85,0xB3,0xE9,0x97,0xAD,0xE5,0xB1,0x8F,0xE5,0xB9,0x95,'\0'};//"关闭屏幕"
/*
char awson_cmd_enable_front[]   = {0xE6,0x89,0x93,0xE5,0xBC,0x80,0xE5,0x89,0x8D,0xE8,0xB7,0xAF,'\0'};//"打开前路"
char awson_cmd_enable_rear[]    = {0xE6,0x89,0x93,0xE5,0xBC,0x80,0xE5,0x90,0x8E,0xE8,0xB7,0xAF,'\0'};//"打开后路"
char awson_cmd_enable_pip[]     = {0xE6,0x89,0x93,0xE5,0xBC,0x80,0xE5,0x8F,0x8C,0xE8,0xB7,0xAF,'\0'};//"打开双路"
char awson_cmd_turnon_mic[]     = {0xE6,0x89,0x93,0xE5,0xBC,0x80,0xE5,0xBD,0x95,0xE9,0x9F,0xB3,'\0'};//"打开录音"
char awson_cmd_turnoff_mic[]    = {0xE5,0x85,0xB3,0xE9,0x97,0xAD,0xE5,0xBD,0x95,0xE9,0x9F,0xB3,'\0'};//"关闭录音"
*/
char awson_cmd_enable_front[]   = {0xE6, 0x98, 0xBE, 0xE7, 0xA4, 0xBA, 0xE5, 0x89, 0x8D, 0xE8, 0xB7, 0xAF,'\0'};//"显示前路"
char awson_cmd_enable_rear[]    = {0xE6, 0x98, 0xBE, 0xE7, 0xA4, 0xBA, 0xE5, 0x90, 0x8E, 0xE8, 0xB7, 0xAF, '\0'};//"显示后路"
char awson_cmd_enable_pip[]     = {0xE6, 0x98, 0xBE, 0xE7, 0xA4, 0xBA, 0xE4, 0xB8, 0xBB, 0xE7, 0x95, 0x8C, 0xE9, 0x9D, 0xA2,'\0'};//"显示主界面"
char awson_cmd_turnon_mic[]     = {0xE5, 0xBD, 0x95, 0xE9, 0x9F, 0xB3, 0xE5, 0xBC, 0x80, 0xE5, 0x90, 0xAF, '\0'};//"录音开启"
char awson_cmd_turnoff_mic[]    = {0xE5, 0xBD, 0x95, 0xE9, 0x9F, 0xB3, 0xE5, 0x85, 0xB3, 0xE9, 0x97, 0xAD, '\0'};//"录音关闭"


char awson_cmd_emg_record[]     = {0xE7,0xB4,0xA7,0xE6,0x80,0xA5,0xE5,0xBD,0x95,0xE5,0x83,0x8F,'\0'};//"紧急录像"
char awson_cmd_photo_capture[]  = {0xE6,0x88,0x91,0xE8,0xA6,0x81,0xE6,0x8B,0x8D,0xE7,0x85,0xA7,'\0'};//"我要拍照"
#endif

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
#if (VOICE_RECG_MODULE == VOICE_RECG_WANSON)
int
voice_recg_wanson_start(
	void
)
{
	if (Wanson_Init_GP() < 0)
	{
		ERR_PRINT("Wanson_ASR_Init Failed!\r\n");
		return STATUS_FAIL;
	}

	Wanson_ASR_Reset();
	DBG_PRINT("Wanson_ASR_Init OK!\r\n");

	return STATUS_OK;
}

int
voice_recg_wanson_stop(
	void
)
{
	Wanson_ASR_Release();
	DBG_PRINT("Wanson_ASR_Release OK!\n");

	return STATUS_OK;
}

int
voice_recg_wanson_process(
	short* samples,
	int n
)
{
	int ret, id, i;//, j;
	float score;
	const char *text;

	#if 0 //Debug
	int k;
	int text_size;
	#endif

	//DBG_PRINT("==%s\r\n",__func__);

	for(i=0;i<(n/640);i++)
	{
		ret = Wanson_ASR_Recog(samples+(i*640), 640, &text, &score);
		if(ret == 1) // Hit
		{
			#if 0 //Debug
			text_size = strlen(text);
			DBG_PRINT("%s:%d\r\n",text,text_size);
			for(k=0; k<text_size; k++)
			{
				DBG_PRINT("%02X ",text[k]);
			}
			DBG_PRINT("\r\n");
			#endif

		   	if (strcmp(text, awson_cmd_start_record) == 0)
			{
				id = ID_RECORD_START;
			}
			else if (strcmp(text, awson_cmd_stop_record) == 0)
			{
				id = ID_RECORD_STOP;
			}
			else if (strcmp(text, awson_cmd_turnon_display) == 0)
			{
				id = ID_OPEN_PANEL;
			}
			else if (strcmp(text, awson_cmd_turnoff_display) == 0)
			{
				id = ID_CLOSE_PANEL;
			}
			else if (strcmp(text, awson_cmd_enable_front) == 0)
			{
				id = ID_DISPLAY_FRONT;
			}
			else if (strcmp(text, awson_cmd_enable_rear) == 0)
			{
				id = ID_DISPLAY_RAER;
			}
			else if (strcmp(text, awson_cmd_enable_pip) == 0)
			{
				id = ID_DISPLAY_PIP;
			}
			else if (strcmp(text, awson_cmd_turnon_mic) == 0)
			{
				id = ID_OPEN_MIC;
			}
			else if (strcmp(text, awson_cmd_turnoff_mic) == 0)
			{
				id = ID_CLOSE_MIC;
			}
			else if (strcmp(text, awson_cmd_emg_record) == 0)
			{
				id = ID_EMERGENCY_RECORD;
			}
			else if (strcmp(text, awson_cmd_photo_capture) == 0)
			{
				id = ID_CAPTURE;
			}
			else
			{
				// Miss command
				return STATUS_OK;
			}

			#if 0//Debug
			awson_run_cnt++;
			DBG_PRINT("%s=>%d:[%d]\r\n",text,id,awson_run_cnt);
			#endif

			if(pVoiceRecgCtrl->voice_recg_action_func)
			{
				pVoiceRecgCtrl->voice_recg_action_func(id);
			}
		}
	}

	return STATUS_OK;
}
#endif

/*== A P I ==*/
INT32S
ap_voice_recg_start(
	void
)
{
    INT32S nRet;

	nRet = pVoiceRecgCtrl->voice_recg_start_func();

	return nRet;
}

INT32S
ap_voice_recg_stop(
	void
)
{
    INT32S nRet;

	nRet = pVoiceRecgCtrl->voice_recg_stop_func();

	return nRet;
}

INT32S
ap_voice_recg_process(
	short* samples,
	int n
)
{
    INT32S nRet;

	cache_invalid_range((INT32U)samples, (n<<1));
	nRet = pVoiceRecgCtrl->voice_recg_process_func(samples,n);

	return nRet;
}

void
ID_record_start(
	void
)
{
	INT16U tick_cnt = 0;

	while(present_state != STATE_VIDEO_RECORD)
	{
		//Send MODE-KEY to change APP mode
		ap_peripheral_mode_key_exe(&tick_cnt);
	}

	//Video record is idle
	if((ap_video_record_sts_get() & VIDEO_RECORD_BUSY) == 0)
	{
		//Send OK-KEY to start video record
		ap_peripheral_ok_key_exe(&tick_cnt);
	}
}

void
ID_record_stop(
	void
)
{
	INT16U tick_cnt = 0;

	while(present_state == STATE_VIDEO_RECORD)
	{
		//Video APP is recording
		if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
		{
			//Send OK-KEY to start video record
			ap_peripheral_ok_key_exe(&tick_cnt);
			break;
		}
		else
		{
			break;
		}
	}
}

void
ID_open_panel(
	void
)
{
	INT16U tick_cnt = 0;

	if(ap_peripheral_lcd_backlight_get() == BL_OFF)
	{
		ap_peripheral_pw_key_exe(&tick_cnt);
	}
}

void
ID_close_panel(
	void
)
{
	INT16U tick_cnt = 0;

	if(ap_peripheral_lcd_backlight_get() == BL_ON)
	{
		ap_peripheral_pw_key_exe(&tick_cnt);
	}
}

void
ID_display_front(
	void
)
{
	if(ap_state_config_display_mode_get() != PREVIEW_MODE_FRONT)
	{
		preview_mode = PREVIEW_MODE_FRONT;
		ap_state_config_display_mode_set(preview_mode);
		video_encode_preview_mode_set(preview_mode);
	}
}

void
ID_display_rear(
	void
)
{
	if(back_sensor_plug_status_get() && ap_state_config_display_mode_get() != PREVIEW_MODE_REAR)
	{
		preview_mode = PREVIEW_MODE_REAR;
		ap_state_config_display_mode_set(preview_mode);
		video_encode_preview_mode_set(preview_mode);
	}
}

void
ID_display_pip(
	void
)
{
	if(back_sensor_plug_status_get() && ap_state_config_display_mode_get() != PREVIEW_MODE_PIP_REAR)
	{
		preview_mode = PREVIEW_MODE_PIP_REAR;
		ap_state_config_display_mode_set(preview_mode);
		video_encode_preview_mode_set(preview_mode);
	}
}

void
ID_open_mic(
	void
)
{
	if(present_state == STATE_VIDEO_RECORD)
	{
		ap_state_config_voice_record_switch_set(1);
		ap_state_handling_icon_clear_cmd(ICON_RECORD_MIC_OFF, 0, 0, 0);
	}
}

void
ID_close_mic(
	void
)
{
	if(present_state == STATE_VIDEO_RECORD)
	{
		ap_state_config_voice_record_switch_set(0);
		ap_state_handling_icon_show_cmd(ICON_RECORD_MIC_OFF, 0, 0, 0);
	}
}

void
ID_emergency_record(
	void
)
{
	INT16U tick_cnt = 0;

	if((present_state == STATE_VIDEO_RECORD) && (ap_video_record_sts_get() & VIDEO_RECORD_BUSY))
	{
		ap_peripheral_sos_key_exe(&tick_cnt);
	}
}

void
ID_capture(
	void
)
{
	INT16U tick_cnt = 0;

	if((present_state == STATE_VIDEO_RECORD) && (ap_video_record_sts_get() & VIDEO_RECORD_BUSY))
	{
		tick_cnt = 25;
		ap_peripheral_menu_key_exe(&tick_cnt);
	}

	if(present_state == STATE_PHOTO_CAPTURE)
	{
		ap_peripheral_ok_key_exe(&tick_cnt);
	}
}

void
ID_Translate_Function(
	ID_Translate_Table actionID
)
{
	//DBG_PRINT("==id:%d==\r\n",actionID);

	switch(actionID)
	{
		case ID_RECORD_START:
			ID_record_start();
		break;

		case ID_RECORD_STOP:
			ID_record_stop();
		break;

		case ID_OPEN_PANEL:
			ID_open_panel();
		break;

		case ID_CLOSE_PANEL:
			ID_close_panel();
		break;

		case ID_DISPLAY_FRONT:
			ID_display_front();
		break;

		case ID_DISPLAY_RAER:
			ID_display_rear();
		break;

		case ID_DISPLAY_PIP:
			ID_display_pip();
		break;

		case ID_OPEN_MIC:
			ID_open_mic();
		break;

		case ID_CLOSE_MIC:
			ID_close_mic();
		break;

		case ID_EMERGENCY_RECORD:
			ID_emergency_record();
		break;

		case ID_CAPTURE:
			ID_capture();
		break;
	}
}

#endif //#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)

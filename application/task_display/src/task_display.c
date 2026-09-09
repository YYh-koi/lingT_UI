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
#include "application.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT8U task_display_lock_st;
static INT8U task_display_lock_st_next;
extern INT8U Get_1st_decode_frm;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

INT32S
display_task_msg_send(
    INT32U msg, void *para, INT32U bytes, INT32U prio
)
{
    INT32S nRet;

    if(dispTaskQ == NULL)
    {
        nRet = -1;
        ERR_PRINT("send msg fail\r\n");
    }
    else
    {
        nRet = msgQSend(dispTaskQ, msg, para, bytes, prio);
		if(nRet < 0)
		{
	        ERR_PRINT("send dispTaskQ fail\r\n");
		}
    }
    return nRet;
}

void
display_task_init(
	void
)
{
	TASK_APP_ARGUMENT *pDispTaskArgs = &pTurnkeyArgs->taskDisplay;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pDispTaskArgs->taskAppQ == 0) {
		pDispTaskArgs->taskAppQ = msgQCreate(pDispTaskArgs->taskMsgLen, pDispTaskArgs->taskMsgLen, pDispTaskArgs->taskRecvBufLen);
		if(pDispTaskArgs->taskAppQ == 0) {
			ERR_PRINT("fail to create pDispTaskArgs->taskAppQ\r\n");
			return;
		}

		if(pDispTaskArgs->taskRecvBufLen)
		{
			pDispTaskArgs->taskRecvBuf = (void*)malloc(pDispTaskArgs->taskRecvBufLen);
			if(pDispTaskArgs->taskRecvBuf == 0) {
				ERR_PRINT("fail to malloc pDispTaskArgs->taskRecvBuf\r\n");
				goto ERR_DISPLAY_TASK_INIT;
			}
		}
	}
	return;

ERR_DISPLAY_TASK_INIT:
	if(pDispTaskArgs->taskAppQ != 0)
	{
		msgQDelete(pDispTaskArgs->taskAppQ);
		pDispTaskArgs->taskAppQ = 0;
	}
	return;
}


static void task_display_lock_set(void)
{
	if(task_display_lock_st == task_display_lock_st_next)
	{
		return;
	}

	if(ap_display_rotate_busy())
	{
		return;
	}

	task_display_lock_st = task_display_lock_st_next;
}

static INT8U task_display_lock_get(void)
{
	if(ap_display_rotate_busy())
	{
		return 0;
	}
	else
	if(ap_display_queue_query(display_rotate_queue))
	{
		return 0;
	}
	else
	if(ap_display_queue_query(display_ready_queue))
	{
		return 0;
	}
	else
	{
		return task_display_lock_st;
	}
}

void task_display_lock_on(INT8U wait)
{
    display_task_msg_send(MSG_DISPLAY_TASK_LOCK_ON, NULL, 0, MSG_PRI_NORMAL);
	while(wait && !task_display_lock_get())
	{
		osDelay(1);
	}
}

void task_display_lock_off(INT8U wait)
{
    display_task_msg_send(MSG_DISPLAY_TASK_LOCK_OFF, NULL, 0, MSG_PRI_NORMAL);
	while(wait && task_display_lock_get())
	{
		osDelay(1);
	}
}


void
display_task_entry(
	void const *args
)
{
	TASK_APP_ARGUMENT* pDispTaskArgs;
	INT32U dispRecvBufLen;
	void* dispRecvBuf;
	INT32U  msg_id;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	pDispTaskArgs = &pTurnkeyArgs->taskDisplay;
	dispRecvBufLen = pDispTaskArgs->taskRecvBufLen;
	dispRecvBuf = pDispTaskArgs->taskRecvBuf;

	ap_display_init();
	ap_display_effect_init();

	while(1)
	{
		if(msgQReceive(dispTaskQ, &msg_id, dispRecvBuf, dispRecvBufLen) == STATUS_FAIL)
		{
            DBG_PRINT("display_task_entry() receive err\r\n");
			continue;
		}

		switch(msg_id)
		{
        case MSG_DISPLAY_TASK_JPEG_DRAW:
        case MSG_DISPLAY_TASK_MJPEG_DRAW:
        case MSG_DISPLAY_TASK_SENSOR_DRAW:
        case MSG_DISPLAY_TASK_BACK_A_CAR_DRAW:
			if(task_display_lock_st || task_display_lock_st_next)
			{
				DBG_PRINT("=>lock st(%d,%d)\r\n",task_display_lock_st,task_display_lock_st_next);
				judge_and_release_display_buffer(*(DISP_P *)dispRecvBuf);
            }
            else
            {
			if(msg_id == MSG_DISPLAY_TASK_JPEG_DRAW || Get_1st_decode_frm == 0){

			#if C_ASYN_DBLE_TFT
			if(!tft_start_switch_screen_get())
			{
				ap_display_buff_copy_and_draw(*(DISP_P *)dispRecvBuf, msg_id);
			}
			else
			{
				judge_and_release_display_buffer(*(DISP_P *)dispRecvBuf);
			}
			drv_l2_asyn_double_tft_backlight_switch();

			#else
				ap_display_buff_copy_and_draw(*(DISP_P *)dispRecvBuf, msg_id);

			#endif
                }
            }
            break;

#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
        case MSG_DISPLAY_TASK_LVGL_DRAW:
            ap_display_buff_lvgl_draw((INT32U*)dispRecvBuf, msg_id);
            break;
#endif

	#if C_ASYN_DBLE_TFT
        case MSG_DISPLAY_DUAL_SCREEN_SWITCH:
		ap_Switch_Screen();
            break;
	#endif

        //icons
        case MSG_DISPLAY_TASK_EFFECT_INIT:
            ap_display_effect_init();
            break;

        case MSG_DISPLAY_TASK_ICON_SHOW:
			ap_display_icon_sts_set(*(INT32U *)dispRecvBuf);
            break;

        case MSG_DISPLAY_TASK_ICON_CLEAR:
			ap_display_icon_sts_clear(*(INT32U *)dispRecvBuf);
            break;

        case MSG_DISPLAY_TASK_WARN_ICON_DRAW:
            ap_display_string_icon_enable(WARN_STR_ICON, (void *)(*(INT32U *)dispRecvBuf));
            break;

        case MSG_DISPLAY_TASK_WARN_ICON_CLEAR:
            ap_display_string_icon_disable(WARN_STR_ICON);
            break;

        case MSG_DISPLAY_TASK_RES_ICON_DRAW:
            ap_display_string_icon_enable(RES_STR_ICON, (void *)(*(INT32U *)dispRecvBuf));
            break;

        case MSG_DISPLAY_TASK_RES_ICON_CLEAR:
            ap_display_string_icon_disable(RES_STR_ICON);
            break;

        case MSG_DISPLAY_TASK_PARK_ICON_DRAW:
            ap_display_string_icon_enable(PARK_STR_ICON, (void *)(*(INT32U *)dispRecvBuf));
            break;

        case MSG_DISPLAY_TASK_PARK_ICON_CLEAR:
            ap_display_string_icon_disable(PARK_STR_ICON);
            break;

        #if C_WATER_MARK == CUSTOM_ON
        case MSG_DISPLAY_TASK_WATER_MARK_ICON_DRAW:
            ap_display_water_mark_icon_enable();
            break;

        case MSG_DISPLAY_TASK_WATER_MARK_ICON_CLEAR:
            ap_display_water_mark_icon_disable();
            break;
        #endif

        //display_rec_time
        case MSG_DISPLAY_TASK_REC_TIME_DRAW:
            ap_display_rec_time_enable();
            break;

        case MSG_DISPLAY_TASK_LEFT_REC_TIME_DRAW:
            ap_display_left_rec_time_enable(*(DISPLAY_REC_TIME *)dispRecvBuf);
            break;

        case MSG_DISPLAY_TASK_PLAY_TIME_DRAW:
            ap_display_play_time_enable();
            break;

        case MSG_DISPLAY_TASK_PLAY_TIME_CLEAR:
        case MSG_DISPLAY_TASK_LEFT_REC_TIME_CLEAR:
        case MSG_DISPLAY_TASK_REC_TIME_CLEAR:
            ap_display_rec_time_disable();
            break;

        //display date time
        case MSG_DISPLAY_RTC_DATE_TIME_DRAW:
            ap_display_rtc_date_time_enable();
            break;

        case MSG_DISPLAY_FILE_DATE_TIME_DRAW:
            ap_display_file_date_time_enable();
            break;

        case MSG_DISPLAY_RTC_DATE_TIME_CLEAR:
        case MSG_DISPLAY_FILE_DATE_TIME_CLEAR:
            ap_display_rtc_date_time_disable();
            break;

        //rotator done
		case MSG_DISPLAY_TASK_ROTATE_DONE:
			ap_display_rotate_restart();
            task_display_lock_set();
			break;

        //lock on & off
		case MSG_DISPLAY_TASK_LOCK_ON:
			task_display_lock_st_next = 1;
			task_display_lock_set();
			break;

		case MSG_DISPLAY_TASK_LOCK_OFF:
			task_display_lock_st_next = 0;
			task_display_lock_set();
			break;
        default:
            DBG_PRINT("display_task_entry() unknown msg = 0x%x\r\n", msg_id);
            break;
		}
	}
}



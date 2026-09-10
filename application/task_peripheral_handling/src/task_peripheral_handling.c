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

INT32U	motion_detect_peripheral_cnt;

#if C_GSENSOR_FUNCTION
INT32U	G_sensor_power_on_time;
static INT8U gsensor_power_on_flag = 0;
#endif

#if C_SCREEN_SAVER
INT8U screen_saver_enable;
#endif
#if(defined REVERSING_CHECK_JPEG_HEADER_REAR) && (REVERSING_CHECK_JPEG_HEADER_REAR == CUSTOM_ON)
extern INT32U back_car_detect;
#endif

#if C_WIFI_SEND_FIX_H264I
extern void rtsp_h264_fix_send(void);
#endif



/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
INT8U	power_on_auto_record = 1;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
peripheral_handling_task_init(
	void
)
{
	TASK_APP_ARGUMENT* pPeriTaskArgs = &pTurnkeyArgs->taskPeripheral;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pPeriTaskArgs->taskAppQ == 0) {
		pPeriTaskArgs->taskAppQ = msgQCreate(pPeriTaskArgs->taskMsgLen, pPeriTaskArgs->taskMsgLen, pPeriTaskArgs->taskRecvBufLen);
		if(pPeriTaskArgs->taskAppQ == 0) {
			ERR_PRINT("fail to create pPeriTaskArgs->taskAppQ \r\n");
			return;
		}

		if(pPeriTaskArgs->taskRecvBufLen)
		{
			pPeriTaskArgs->taskRecvBuf = (void*)malloc(pPeriTaskArgs->taskRecvBufLen);
			if(pPeriTaskArgs->taskRecvBuf == 0) {
				ERR_PRINT("fail to malloc pPeriTaskArgs->taskRecvBuf \r\n");
				goto ERR_PERIPHERAL_TASK_INIT;
			}
		}
	}
	return;

ERR_PERIPHERAL_TASK_INIT:
	if(pPeriTaskArgs->taskAppQ != 0)
	{
		msgQDelete(pPeriTaskArgs->taskAppQ);
		pPeriTaskArgs->taskAppQ = 0;
	}
	return;
}

void
task_peripheral_handling_init(
	void
)
{
	DBG_PRINT("%s ... \r\n", __func__);
	ap_peripheral_init();
}

void
task_peripheral_handling_entry(
	void const *arg
)
{
	TASK_APP_ARGUMENT* pPeripheralTaskArgs;
	INT32U peripheral_para_len;
	INT8U* peripheral_para;

	INT32U	msg_id;
	INT32U	tick_current	= 0;
	INT32U	tick_last_32hz	= 0;
	INT32U	tick_last_4hz	= 0;
	INT32U	tick_last_1sec	= 0;
	INT32U	tick_last_2sec	= 0;

    #if C_USB_DEVICE
	INT8U	usb_detect_start = 0;
	#endif

	INT8U	volume_show_time = 0;

	static INT32U battery_charge_icon_blink_cnt;
	static INT32U battery_low_blink_cnt;
	static INT32U display_clear_screen_cnt;

	INT8U back_polling_start = 0;

	pPeripheralTaskArgs = &pTurnkeyArgs->taskPeripheral;
	peripheral_para = pPeripheralTaskArgs->taskRecvBuf;
	peripheral_para_len = pPeripheralTaskArgs->taskRecvBufLen;

	tick_current = xTaskGetTickCount();
    tick_last_32hz = tick_current;
    tick_last_4hz = tick_current;
    tick_last_1sec = tick_current;
    tick_last_2sec = tick_current;

	task_peripheral_handling_init();

	DBG_PRINT("%s ... \r\n", __func__);

	while(1)
	{
		if(msgQReceive(PeripheralTaskQ, &msg_id, (void*)peripheral_para, peripheral_para_len) == STATUS_FAIL)
		{
			continue;
		}

		//DBG_PRINT("==Run:%s==msg_id:0x%x==\r\n",__FUNCTION__,msg_id);

		switch(msg_id)
		{
		case MSG_PERIPHERAL_TASK_KEY_REGISTER:
			ap_peripheral_key_register(peripheral_para[0]);
			break;

        #if C_USB_DEVICE
		case MSG_PERIPHERAL_TASK_USBD_DETECT_INIT:
			usb_detect_start = 1;
			break;
        #endif

		case MSG_PERIPHERAL_TASK_BATTERY_CHARGE_ICON_BLINK_START:
			battery_charge_icon_blink_cnt = 0x002f;
			break;

		case MSG_PERIPHERAL_TASK_BATTERY_CHARGE_ICON_BLINK_STOP:
			battery_charge_icon_blink_cnt = 0;
			break;

		case MSG_PERIPHERAL_TASK_BATTERY_LOW_BLINK_START:
			battery_low_blink_cnt = 0x001f;
			battery_low_blink_cnt |= 0x0a00;
			break;

		case MSG_PERIPHERAL_TASK_BATTERY_LOW_BLINK_STOP:
			if(battery_low_blink_cnt)
			{
				battery_low_blink_cnt = 0;
			}

			break;

		case MSG_PERIPHERAL_TASK_CLEAR_SCREEN_COUNTDOWN:
			display_clear_screen_cnt = 0x006f;
			break;

		case MSG_PERIPHERAL_TASK_TIMER_VOLUME_ICON_SHOW:
			volume_show_time = (peripheral_para[0]) * 20;
			break;

		#if C_GSENSOR_FUNCTION
		case MSG_PERIPHERAL_TASK_G_SENSOR_POWER_ON_START:
			G_sensor_power_on_time = 0x0300;
			break;
		#endif // #if C_GSENSOR_FUNCTION

		case MSG_PERIPHERAL_TASK_POLL_32HZ:
			tick_current = xTaskGetTickCount();

			if(tick_current - tick_last_4hz >= 250)
			{
				tick_last_4hz += 250;
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_POLL_4HZ, NULL, 0, MSG_PRI_NORMAL);
			}

			if(tick_current - tick_last_1sec >= 1000)
			{
				tick_last_1sec += 1000;
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_POLL_1SEC, NULL, 0, MSG_PRI_NORMAL);
			}

			if(tick_current - tick_last_2sec >= 2000)
			{
				tick_last_2sec += 2000;
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_POLL_2SEC, NULL, 0, MSG_PRI_NORMAL);
			}

			if(tick_current - tick_last_32hz <= 3)
			{
				break;
			}
            tick_last_32hz = tick_current;

			ap_peripheral_key_judge();
			#if ADKEY_NUM
			/* The six resistor keys are on ADC_LINE_0.  They remain the
			 * physical input for LVGL as well as for the legacy OSD path. */
			ap_peripheral_ad_key_judge();
			#endif
			ap_peripheral_config_store();

			#ifdef HDMI_HPD_PIN && (LCD_DRV_EN_HDMI==1)
			ap_peripheral_hdmi_hpd_detect();
			#endif

			#if C_WATCHDOG
			drv_l1_sys_wdt_clear();
			#endif

			#if CRAZY_KEY_TEST
			if(crazy_key_enable)
				ap_peripheral_crazy_key_test();
			#endif

            #if C_USB_DEVICE
			if(usb_detect_start == 1)
			{
				ap_peripheral_usbd_io_detection();
			}
            #endif

			
			#if C_WIFI_SEND_FIX_H264I
			rtsp_h264_fix_send();		/* send fix H246 I frame to rtsp_h264_tx_task() */
			#endif


			if(battery_charge_icon_blink_cnt)
			{
				battery_charge_icon_blink_cnt--;
				if(!(battery_charge_icon_blink_cnt & 0xfff))
				{
					if(battery_charge_icon_blink_cnt & 0x8000)
					{
						battery_charge_icon_blink_cnt &= ~0x8000;
						ap_state_handling_icon_show_cmd(ICON_BATTERY_CHARGED, 0, 0, 0);
					}
					else
					{
						battery_charge_icon_blink_cnt |= 0x8000;
						ap_state_handling_icon_clear_cmd(ICON_BATTERY_CHARGED, 0, 0, 0);
					}
					if(present_state == STATE_BROWSE)
						ap_browse_display_update_command();
					battery_charge_icon_blink_cnt |= 0x002f;
				}
			}

			if(battery_low_blink_cnt)
			{
				battery_low_blink_cnt--;
				if(!(battery_low_blink_cnt & 0x00ff))
				{
					if(battery_low_blink_cnt & 0x0f00)
					{
						battery_low_blink_cnt -= 0x0100;
					}
					else
					{
						DBG_PRINT("Battery low Power off!\r\n");
						app_msg_send(MSG_APQ_POWER_KEY_ACTIVE, NULL, 0, MSG_PRI_URGENT);
						battery_low_blink_cnt = 0;
						break;
					}

					if(battery_low_blink_cnt & 0x8000)
					{
						battery_low_blink_cnt &= ~0x8000;
						ap_state_handling_warn_str_icon_draw(STR_BATTERY_LOW, WARN_STR_COLOR);
					}
					else
					{
						battery_low_blink_cnt |= 0x8000;
						ap_state_handling_warn_str_icon_exit();
					}

					battery_low_blink_cnt |= 0x001f;
				}
			}

			if(display_clear_screen_cnt)
			{
				display_clear_screen_cnt--;
				if(!display_clear_screen_cnt)
				{
					ap_state_handling_warn_str_icon_exit();
				}
			}

			#if C_GSENSOR_FUNCTION
			if(G_sensor_power_on_time)
			{
				G_sensor_power_on_time--;
				if((ap_video_record_sts_get() & VIDEO_RECORD_BUSY)&&(gsensor_power_on_flag == 0))
				{
					gsensor_power_on_flag = 1;
				}
				if(gsensor_power_on_flag)
				{
					gsensor_power_on_flag++;
					if(gsensor_power_on_flag == 20)
					{
						DBG_PRINT("LOCK FILE...!\r\n");
						app_msg_send(MSG_APQ_SOS_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
					}
				}
				//check SDC
				if(G_sensor_power_on_time < (0x300 - 0x80))
				{
					if(!ap_peripheral_get_usbd_connection_state())
					{
						if(ap_state_handling_storage_id_get() != STORAGE_DEVICE)
						{
							DBG_PRINT("G sensor on NO SDC Power off!\r\n");
							app_msg_send(MSG_APQ_POWER_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
							G_sensor_power_on_time = 0;
							break;
						}
					}
				}

				if(!G_sensor_power_on_time)
				{
					//check adptor
					if(!ap_peripheral_get_usbd_connection_state())
					{
						DBG_PRINT("G sensor on time over Power off!\r\n");
						app_msg_send(MSG_APQ_POWER_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
						break;
					}
					else
					{
						DBG_PRINT("G sensor on time over restart recording\r\n");
						/*msgQSend(ApQ, MSG_APQ_FUNCTION_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
						msgQSend(ApQ, MSG_APQ_FUNCTION_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);*/
					}
				}
			}
			#endif // #if C_GSENSOR_FUNCTION

			#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
			#if C_MOTION_DET_NO_CANCEL
			if(!state_record_MD_freeze_status_get())
			#endif
			{
				if(motion_detect_peripheral_cnt)
				{
					motion_detect_peripheral_cnt--;
					if(!motion_detect_peripheral_cnt)
					{
						ap_peripheral_motion_detect_judge();
						motion_detect_peripheral_cnt = 2;
					}
				}
			}
			#endif // #if C_MOTION_DETECTION

			if(volume_show_time)
			{
				volume_show_time--;
				if(!volume_show_time)
				{
					app_msg_send(MSG_APQ_VOLUME_SHOW_END, NULL, 0, MSG_PRI_NORMAL);
				}
			}

			break;

		case MSG_PERIPHERAL_TASK_POLL_4HZ:
            //DBG_PRINT("[DEBUG]back_det(tick_P, tick_os)=(%3d, %d)\r\n", tick_current, xTaskGetTickCount());

			if(back_polling_start)
			{
				if(ap_peripheral_back_detect() == STATUS_OK)
				{
					if(power_on_auto_record)
					{
						if(osOK == app_msg_send(MSG_APQ_POWER_ON_AUTO_RECORD, NULL, 0, MSG_PRI_NORMAL))
						{
							power_on_auto_record = 0;
						}
					}
				}
			}

            ap_peripheral_sys_osd_time_set();
			LED_service();
			break;

		case MSG_PERIPHERAL_TASK_POLL_1SEC:
            #if(defined REVERSING_CHECK_JPEG_HEADER_REAR) && (REVERSING_CHECK_JPEG_HEADER_REAR == CUSTOM_ON)
            if(back_car_detect == 0)
            back_car_detect = 1;
            #endif
            #if C_GSENSOR_FUNCTION == CUSTOM_ON
            ap_peripheral_read_gsensor();
            #endif
             #if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
			//lvgl_task_msg_send(MSG_LVGL_1SEC, NULL, 0, MSG_PRI_NORMAL);
			#endif
			break;

		case MSG_PERIPHERAL_TASK_POLL_2SEC:
			#if C_RTC_REBOOT
			ap_peripheral_rtc_alarm_set();
			#endif

			break;

		#if C_MOTION_DETECTION
		//case MSG_PERIPHERAL_TASK_MOTION_DETECT_JUDGE:
		//	ap_peripheral_motion_detect_judge();
		//	break;
		case MSG_PERIPHERAL_TASK_MOTION_DETECT_START:
			ap_peripheral_motion_detect_start();
			motion_detect_peripheral_cnt = 32;
			break;

		case MSG_PERIPHERAL_TASK_MOTION_DETECT_STOP:
			ap_peripheral_motion_detect_stop();
			motion_detect_peripheral_cnt = 0;
			break;

		case MSG_PERIPHERAL_TASK_MOTION_DETECT_DELAY:
			motion_detect_peripheral_cnt = 0xff;
			break;
		#endif // #if C_MOTION_DETECTION

		case MSG_PERIPHERAL_TASK_LCD_BACKLIGHT_SET:
#if C_SCREEN_SAVER
			ap_peripheral_clr_screen_saver_timer();
#endif
			ap_peripheral_lcd_backlight_set(peripheral_para[0]);
			break;

		#if C_SCREEN_SAVER
		case MSG_PERIPHERAL_TASK_SCREEN_SAVER_ENABLE:
			screen_saver_enable = 1;
			break;
		#endif // #if C_SCREEN_SAVER

		case MSG_PERIPHERAL_BACK_SENSOR_POLLING_START:
			back_polling_start = 1;
			break;

		case MSG_PERIPHERAL_BACK_SENSOR_POLLING_STOP:
			back_polling_start = 0;
			break;

		default:
			break;
		}
	}
}

INT32S
peripheral_task_msg_send(
    INT32U msg, void *para, INT32U bytes, INT32U prio
)
{
    INT32S nRet;

    if(PeripheralTaskQ == NULL)
    {
        nRet = -1;
        ERR_PRINT("send msg fail\r\n");
   }
    else
    {
        nRet = msgQSend(PeripheralTaskQ, msg, para, bytes, prio);
    }
    return nRet;
}


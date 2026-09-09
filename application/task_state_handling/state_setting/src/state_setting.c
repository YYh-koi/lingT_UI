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
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern INT32U  main_menu_item;

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC) && C_WIFI_ON_OFF_KEY
extern INT32S wal_ap_mode_disable(void);
#endif

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
Overlap_State_Setting_Function State_Seting;

void overlap_state_setting_function_initial(void)
{
	State_Seting.USB_select_entry_ptr       = &USB_select_entry;
	State_Seting.state_setting_entry_ptr    = &state_setting_entry;
}

void USB_select_entry(void *para)
{
	EXIT_FLAG_ENUM	exit_flag = EXIT_RESUME;
	INT32U			msg_id;
	STAudioConfirm	*audio_temp;

	main_menu_item = ap_state_config_usb_mode_get();
	ap_USB_setting_page_draw();

	DBG_PRINT("%s ... \r\n", __func__);

	while(exit_flag == EXIT_RESUME)
	{
		if(msgQReceive(ApQ, &msg_id, (void *) ApQ_para, ApQ_para_len) == STATUS_FAIL)
		{
			continue;
		}

		switch(msg_id)
		{
		case EVENT_APQ_ERR_MSG:
			audio_temp = (STAudioConfirm *) ApQ_para;
			if((audio_temp->result == AUDIO_ERR_DEC_FINISH) && (audio_temp->source_type == AUDIO_SRC_TYPE_APP_RS))
			{

			}
			else
			{
				audio_confirm_handler((STAudioConfirm *) ApQ_para);
			}

			break;

		case MSG_STORAGE_SERVICE_MOUNT:
			ap_state_handling_storage_id_set(((INT8U *)ApQ_para)[0]);
			DBG_PRINT("[Setting Mount OK]\r\n");
			break;

		case MSG_STORAGE_SERVICE_NO_STORAGE:
			ap_state_handling_storage_id_set(((INT8U *)ApQ_para)[0]);
			DBG_PRINT("[Setting Mount FAIL]\r\n");
			break;

		case MSG_APQ_POWER_KEY_ACTIVE:
		case MSG_APQ_SW_RESET:
			osDelay(10);
			switch(msg_id)
			{
				case MSG_APQ_POWER_KEY_ACTIVE:
					ap_state_handling_power_off(0);
				break;
				case MSG_APQ_SW_RESET:
					ap_state_handling_power_off(1);
				break;
			}
			break;

		case MSG_APQ_DISCONNECT_TO_PC:
			msgQSend(ApQ, MSG_APQ_DISCONNECT_TO_PC, NULL, 0, MSG_PRI_NORMAL);
			exit_flag = EXIT_BREAK;
			break;

		case MSG_APQ_FUNCTION_KEY_ACTIVE:
            if(main_menu_item == 0)
            {
                ap_state_config_usb_mode_set(0);
            }
            else
            {
                ap_state_config_usb_mode_set(1);
            }

            ap_USB_setting_display_ok();

			exit_flag = EXIT_BREAK;
			break;

		case MSG_APQ_NEXT_KEY_ACTIVE:
		case MSG_APQ_PREV_KEY_ACTIVE:
			ap_USB_setting_direction_key_active(msg_id);
			break;

		case MSG_APQ_BACK_SENSOR_PLUG_IN:
		case MSG_APQ_BACK_SENSOR_PLUG_OUT:
			if(msg_id == MSG_APQ_BACK_SENSOR_PLUG_IN)
			{
				back_sensor_plug_status_set(1);

				//restoring from config and active
				DBG_PRINT("BACK SENSOR PLUG IN\r\n");
				preview_mode = ap_state_config_display_mode_get();
			}
			else
			if(msg_id == MSG_APQ_BACK_SENSOR_PLUG_OUT)
			{
				back_sensor_plug_status_set(0);

				//storing config
				DBG_PRINT("BACK SENSOR PLUG OUT\r\n");
				ap_state_config_display_mode_set(PREVIEW_MODE_PIP_REAR);

				//de-activate dual cam preview modes
				preview_mode = PREVIEW_MODE_FRONT;
			}
			break;

		default:
			ap_state_common_handling(msg_id);
			break;
		}
	}
}

void state_setting_init(INT32U prev_state)
{
	DBG_PRINT("%s ... \r\n", __func__);
	DBG_PRINT("setting state init enter\r\n");

	overlap_setting_function_initial();
	Seting.ap_setting_init_ptr(prev_state);
}

void state_setting_exit(INT32U prev_state)
{
	Seting.ap_setting_exit_ptr(prev_state);
	DBG_PRINT("Exit setting state\r\n");
}

void
state_setting_entry(
	void *para
)
{
	EXIT_FLAG_ENUM	exit_flag = EXIT_RESUME;
	STAudioConfirm	*audio_temp;
	INT32U	msg_id, prev_state;
	INT8U temp;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	prev_state = *((INT32U *) para);

	state_setting_init(prev_state);

	DBG_PRINT("%s ... \r\n", __func__);

	while(exit_flag == EXIT_RESUME)
	{
		if(msgQReceive(ApQ, &msg_id, (void *) ApQ_para, ApQ_para_len) == STATUS_FAIL)
		{
			continue;
		}

		switch(msg_id)
		{
		case EVENT_APQ_ERR_MSG:
			audio_temp = (STAudioConfirm *) ApQ_para;
			if((audio_temp->result == AUDIO_ERR_DEC_FINISH) && (audio_temp->source_type == AUDIO_SRC_TYPE_APP_RS))
			{

			}
			else
			{
				audio_confirm_handler((STAudioConfirm *) ApQ_para);
			}

			break;

		case MSG_STORAGE_SERVICE_MOUNT:
			ap_state_handling_storage_id_set(((INT8U *)ApQ_para)[0]);
			DBG_PRINT("[Setting Mount OK]\r\n");
			break;

		case MSG_STORAGE_SERVICE_NO_STORAGE:
			ap_state_handling_storage_id_set(((INT8U *)ApQ_para)[0]);
			DBG_PRINT("[Setting Mount FAIL]\r\n");
			break;

		case MSG_APQ_POWER_KEY_ACTIVE:
		case MSG_APQ_SW_RESET:
			if(g_setting_reply_action_flag == 0)
			{
				osDelay(10);
				msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
				break;
			}

			Seting.ap_setting_exit_ptr(prev_state);
			osDelay(10);
			switch(msg_id)
			{
				case MSG_APQ_POWER_KEY_ACTIVE:
					ap_state_handling_power_off(0);
				break;
				case MSG_APQ_SW_RESET:
					ap_state_handling_power_off(1);
				break;
			}
			break;

		case MSG_APQ_MENU_KEY_ACTIVE:
			if(g_setting_reply_action_flag == 0)
			{
				osDelay(10);
				msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
				break;
			}

			exit_flag = Seting.ap_setting_menu_key_active_ptr(prev_state);
			break;

		case MSG_APQ_MODE:
			if(g_setting_reply_action_flag == 0)
			{
				vTaskDelay(10);
				msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
				break;
			}

			exit_flag = Seting.ap_setting_mode_key_active_ptr(prev_state);
			break;
		#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
		case MSG_APQ_WIFI_CONNECT:
		case MSG_APQ_WIFI_DISCONNECT:

            if(msg_id == MSG_APQ_WIFI_CONNECT)
            {
                if(prev_state ==  STATE_BROWSE)
                state_handling_msg_send(STATE_VIDEO_RECORD, NULL, 0, MSG_PRI_NORMAL);
                else
                state_handling_msg_send(prev_state, NULL, 0, MSG_PRI_NORMAL);
                exit_flag = EXIT_BREAK;
                DBG_PRINT("->connect \r\n");
                state_wifi_symbol_show(1);
            }
            else
            {
                rtsp_h264_stop_service();
                #if C_WIFI_ON_OFF_KEY

                osDelay(100);       //wait sensor frame to stop
                wal_ap_mode_disable();
                {
                    INT8U type = KEY_MODE_NORMAL;
                    peripheral_task_msg_send(MSG_PERIPHERAL_TASK_KEY_REGISTER, &type, sizeof(INT8U), MSG_PRI_NORMAL);
                }
                state_wifi_status_set(FLAG_DISCONNECT);
                #endif
                state_handling_msg_send(present_state, NULL, 0, MSG_PRI_NORMAL);
                exit_flag = EXIT_BREAK;
                DBG_PRINT("->disconnect \r\n");
                state_wifi_symbol_show(0);
            }
            sw_wifi_ack_send(0);
            break;
		#endif
		case MSG_APQ_FUNCTION_KEY_ACTIVE:
			if(g_setting_reply_action_flag == 0)
			{
				osDelay(10);
				msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
				break;
			}

			Seting.ap_setting_func_key_active_ptr();
			break;

		case MSG_APQ_NEXT_KEY_ACTIVE:
		case MSG_APQ_PREV_KEY_ACTIVE:
			if(g_setting_reply_action_flag == 0)
			{
				osDelay(10);
				msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
				break;
			}

			Seting.ap_setting_direction_key_active_ptr(msg_id);
			break;

		case MSG_STORAGE_SERVICE_BROWSE_REPLY:
			g_setting_reply_action_flag = 1;
			close(((STOR_SERV_PLAYINFO *) ApQ_para)->file_handle);
			break;

		#if C_BATTERY_DETECT && C_BATTERY_LOW_POWER_OFF
		case MSG_APQ_BATTERY_LOW_SHOW:
		#endif

		case MSG_APQ_CONNECT_TO_PC:
			if(g_setting_reply_action_flag == 0)
			{
				osDelay(10);
				msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
				break;
			}

			exit_flag = Seting.ap_setting_mode_key_active_ptr(prev_state);
			exit_flag = EXIT_BREAK;
			break;

		case MSG_STORAGE_SERVICE_FORMAT_REPLY:
			g_setting_reply_action_flag = 1;
			Seting.ap_setting_format_reply_ptr();
			DBG_PRINT("setting format done!\r\n");
			storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
			break;

		#if C_SCREEN_SAVER
		case MSG_APQ_KEY_IDLE:
			ap_state_handling_lcd_backlight_switch(0);
			break;

		case MSG_APQ_KEY_WAKE_UP:
			ap_state_handling_lcd_backlight_switch(1);
			break;
		#endif // #if C_SCREEN_SAVER

		case MSG_APQ_SELECT_FILE_DEL_REPLY:
			g_setting_reply_action_flag = 1;
			if(((INT8U *)ApQ_para)[0] == 0x55)
			{
				ap_state_handling_show_protected();
				break;
			}
			ap_setting_other_reply(msg_id);
			msg_id = (STATE_BROWSE | 0x5A5A0000);
			state_handling_msg_send(msg_id, NULL, 0, MSG_PRI_NORMAL);
			//wait for display queue empty
			osDelay(10);
			exit_flag = EXIT_BREAK;
			break;

		case MSG_APQ_FILE_DEL_ALL_REPLY:
		case MSG_APQ_FILE_LOCK_ONE_REPLY:
		case MSG_APQ_FILE_LOCK_ALL_REPLY:
		case MSG_APQ_FILE_UNLOCK_ONE_REPLY:
		case MSG_APQ_FILE_UNLOCK_ALL_REPLY:
			g_setting_reply_action_flag = 1;
			ap_setting_other_reply(msg_id);

			msg_id = STATE_BROWSE;
			state_handling_msg_send(msg_id, NULL, 0, MSG_PRI_NORMAL);

			//wait for display queue empty
			osDelay(10);
			exit_flag = EXIT_BREAK;
			break;

		case MSG_APQ_USER_CONFIG_STORE:
			ap_state_config_store();
			break;

		#if C_BATTERY_DETECT
		case MSG_APQ_BATTERY_CHARGED_SHOW:
			break;

		case MSG_APQ_BATTERY_CHARGED_CLEAR:
			break;
		#endif // #if C_BATTERY_DETECT

		case MSG_APQ_BACK_SENSOR_PLUG_IN:
		case MSG_APQ_BACK_SENSOR_PLUG_OUT:
			if(msg_id == MSG_APQ_BACK_SENSOR_PLUG_IN)
			{
				back_sensor_plug_status_set(1);

				//restoring from config and active
				DBG_PRINT("BACK SENSOR PLUG IN\r\n");
				preview_mode = ap_state_config_display_mode_get();
			}
			else
			if(msg_id == MSG_APQ_BACK_SENSOR_PLUG_OUT)
			{
				back_sensor_plug_status_set(0);

				//storing config
				DBG_PRINT("BACK SENSOR PLUG OUT\r\n");
				ap_state_config_display_mode_set(PREVIEW_MODE_PIP_REAR);

				//de-activate dual cam preview modes
				preview_mode = PREVIEW_MODE_FRONT;
			}

			break;

		case MSG_APQ_BACK_A_CAR_ON:
			if(g_setting_reply_action_flag == 0)
			{
				vTaskDelay(10);
				msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
				break;
			}

			DBG_PRINT("setting status BACK A CAR ON\r\n");

			switch(prev_state)
			{
			case STATE_VIDEO_RECORD:
			case STATE_PHOTO_CAPTURE:
				state_handling_msg_send(prev_state, NULL, 0, MSG_PRI_NORMAL);
				break;

			default:
				state_handling_msg_send(STATE_VIDEO_RECORD, NULL, 0, MSG_PRI_NORMAL);
				break;
			}

			exit_flag = EXIT_BREAK;
			msgQSend(ApQ, MSG_APQ_BACK_A_CAR_ON, NULL, 0, MSG_PRI_NORMAL);
			temp = KEY_MODE_BACK_A_CAR;
			peripheral_task_msg_send(MSG_PERIPHERAL_TASK_KEY_REGISTER, &temp, sizeof(INT8U), MSG_PRI_NORMAL);
			break;

		#if defined(C_FIRMWARE_UPGRADE) && C_FIRMWARE_UPGRADE
		case MSG_STORAGE_FIRMWARE_UPGRADE:
			state_handling_msg_send(STATE_FIRMWARE_UPGRADE, NULL, 0, MSG_PRI_NORMAL);
			exit_flag = EXIT_BREAK;
			break;
		#endif // defined(C_FIRMWARE_UPGRADE) && C_FIRMWARE_UPGRADE

		default:
			ap_state_common_handling(msg_id);
			break;
		}
	}

	state_setting_exit(prev_state);

	switch(msg_id)
	{
		case MSG_APQ_MODE:
		{
			//msgQFlush(ApQ);
			//msgQSend(ApQ, MSG_APQ_MODE, NULL, 0, MSG_PRI_NORMAL);
		}
		break;
		case MSG_APQ_CONNECT_TO_PC:
		{
			msgQFlush(ApQ);
			app_msg_send(MSG_APQ_CONNECT_TO_PC, NULL, 0, MSG_PRI_NORMAL);
		}
		break;

		#if C_BATTERY_DETECT && C_BATTERY_LOW_POWER_OFF
		case MSG_APQ_BATTERY_LOW_SHOW:
		{
			msgQFlush(ApQ);
			app_msg_send(MSG_APQ_BATTERY_LOW_SHOW, NULL, 0, MSG_PRI_NORMAL);
			//msgQSend(ApQ, MSG_APQ_BATTERY_LOW_SHOW, NULL, sizeof(INT8U), MSG_PRI_NORMAL);
		}
		break;
		#endif // #if C_BATTERY_DETECT && C_BATTERY_LOW_POWER_OFF
	}
}

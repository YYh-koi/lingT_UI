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

static INT8U mode_key_enabled;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern STOR_SERV_PLAYINFO browse_curr_file;

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC) && C_WIFI_ON_OFF_KEY
extern INT32S wal_ap_mode_disable(void);
#endif
#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
extern void ap_audio_ui_draw(void);
#endif
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
INT8S	browser_play_speed;
INT8U gPlayback_Start;
INT8U gPower_down = 0;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
state_browse_init(
	INT32U prev_state,
	INT16U play_index
)
{
	DBG_PRINT("%s ... \r\n", __func__);
	gp_cdsp_task_priority(1);
	ap_browse_init(prev_state, play_index);
	browser_play_speed = 0;
}

void
state_browse_entry(
	void *para,
	INT16U play_index
)
{
	INT32U pre_val;
	INT32S ret;
	EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
	STAudioConfirm *audio_temp;
	INT32U msg_id, prev_state, search_type;
	INT8U func_key_act = 0;
	INT32U Q_Len;

	INT8U temp;

	DBG_PRINT("%s ... \r\n", __func__);

	prev_state = *((INT32U *) para);
	if((prev_state == STATE_SETTING) || (prev_state == STATE_THUMBNAIL))
	{
		msgQAccept(ApQ, &msg_id, (void *) ApQ_para, ApQ_para_len);
		if(msg_id == MSG_APQ_CONNECT_TO_PC)
		{
			msgQSend(ApQ, MSG_APQ_CONNECT_TO_PC, NULL, 0, MSG_PRI_NORMAL);
			state_browse_init(prev_state, play_index);
		}
		else
		{

			Q_Len = msgQSizeGet(ApQ);
			// if queue isn't empty.
			if(Q_Len != 0)
			{
				//max size of para is STAudioConfirm
				msgQSend(ApQ, msg_id, (void *) ApQ_para, sizeof(STAudioConfirm), MSG_PRI_NORMAL);
			}

			state_browse_init(prev_state, play_index);
		}
	}
	else
	{
		state_browse_init(prev_state, play_index);
	}

	while(exit_flag == EXIT_RESUME)
	{
		#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
		wifi_reference_state_set(BROWSE_MODE);
		#endif

		if(msgQReceive(ApQ, &msg_id, (void *) ApQ_para, ApQ_para_len) == STATUS_FAIL)
		{
			continue;
		}

		//DBG_PRINT("==Run:%s==msg_id:0x%x==\r\n",__FUNCTION__,msg_id);

		switch(msg_id)
		{
		case EVENT_APQ_ERR_MSG:
			audio_temp = (STAudioConfirm *) ApQ_para;
			if((audio_temp->result == AUDIO_ERR_DEC_FINISH) && (audio_temp->source_type == AUDIO_SRC_TYPE_FS))
			{
				browser_play_speed = 0;
				ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, 0, 0);
				ap_state_handling_icon_show_cmd(ICON_PLAYBACK_PLAY, 0, 0, 0);

				ap_browse_wav_stop(NULL);
#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
				if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
				{
                    search_type = STOR_SERV_SEARCH_ORIGIN;
                    ap_browse_open_file_req((void *) &search_type);
				}
#endif
				ap_state_audio_play_speed_set(1);
				storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
			}
			else
			if((audio_temp->result == AUDIO_ERR_DEC_FINISH) && (audio_temp->source_type == AUDIO_SRC_TYPE_APP_RS))
			{
				if(func_key_act)
				{
					func_key_act = 0;

					if(ap_browse_sts_get() & BROWSE_OPEN_FILE_NOW)
					{
						osDelay(10);
						msgQSend(ApQ, MSG_APQ_FUNCTION_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
						break;
					}

					ap_browse_func_key_active();
				}
			}
			else
			{
				audio_confirm_handler((STAudioConfirm *) ApQ_para);
			}

			break;

		case MSG_STORAGE_SERVICE_MOUNT:
			ap_state_handling_warn_str_icon_exit();
			ap_state_handling_storage_id_set(ApQ_para[0]);

			ap_browse_sts_set(~BROWSE_SD_NOT_EXIST);

			search_type = STOR_SERV_SEARCH_INIT;
			ap_browse_open_file_req((void *) &search_type);

			DBG_PRINT("[State Browse Mount OK]\r\n");
			break;

		case MSG_STORAGE_SERVICE_NO_STORAGE:
			ap_state_handling_storage_id_set(ApQ_para[0]);
			ap_browse_sts_set(BROWSE_SD_NOT_EXIST);
			ap_browse_stop_handle();

			ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, ICON_PLAYBACK_MOVIE, 0);
            display_task_msg_send(MSG_DISPLAY_FILE_DATE_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
			ap_browse_volume_icon_clear_all();
			ap_state_handling_warn_str_icon_exit();

			ap_browse_no_media_show(STR_NO_SD);
            vid_dec_clear_status(C_VIDEO_DECODE_ALL);
			DBG_PRINT("[State Browse Mount FAIL]\r\n");
			break;

		case MSG_APQ_POWER_KEY_ACTIVE:
		case MSG_APQ_SW_RESET:
			gPower_down = 1;
			ap_browse_exit(MSG_APQ_POWER_KEY_ACTIVE);
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
			if(ap_browse_sts_get() & BROWSE_OPEN_FILE_NOW)
			{
				osDelay(10);
				msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
				break;
			}

            if(ap_browse_sts_get() & (BROWSE_PLAYBACK_PAUSE | BROWSE_PLAYBACK_BUSY))
            {
                if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
                {
                    ap_browse_wav_stop(NULL);
                }
                else
                {
                    ap_browse_mjpeg_stop();
                    ap_browse_mjpeg_play_end();
                }
				if(browse_curr_file.file_handle > -1)
				{
					close(browse_curr_file.file_handle);
					browse_curr_file.file_handle = -1;
				}

                audio_effect_play(VOICE_ID_CLICK);
                search_type = STOR_SERV_SEARCH_ORIGIN;
				ap_browse_open_file_req((void *) &search_type);
                break;
            }

			state_handling_msg_send(STATE_SETTING, NULL, 0, MSG_PRI_NORMAL);
			exit_flag = EXIT_BREAK;
			break;

		case MSG_APQ_MODE:
			if(ap_browse_sts_get() & BROWSE_OPEN_FILE_NOW)
			{
				break;
			}

            if(ap_browse_sts_get() & (BROWSE_PLAYBACK_PAUSE | BROWSE_PLAYBACK_BUSY))
            {
                if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
                {
                    ap_browse_wav_stop(NULL);
                }
                else
                {
                    ap_browse_mjpeg_stop();
                    ap_browse_mjpeg_play_end();
                }
				if(browse_curr_file.file_handle > -1)
				{
					close(browse_curr_file.file_handle);
					browse_curr_file.file_handle = -1;
				}

                audio_effect_play(VOICE_ID_CLICK);
                search_type = STOR_SERV_SEARCH_ORIGIN;
				ap_browse_open_file_req((void *) &search_type);
                break;
            }

			state_handling_msg_send(STATE_BT_MUSIC, NULL, 0, MSG_PRI_NORMAL);
			exit_flag = EXIT_BREAK;
			break;

		case MSG_APQ_FUNCTION_KEY_ACTIVE:
			if(ap_browse_sts_get() & BROWSE_OPEN_FILE_NOW)
			{
				osDelay(10);
				app_msg_send(msg_id, NULL, 0, MSG_PRI_NORMAL);
				break;
			}

			if(func_key_act == 0)
			{
				ap_browse_func_key_active();
			}

			break;
		case MSG_APQ_BROWSE_PLAY_FILE:// Send from socket cmd
			gp_memcpy((INT8S *)&play_index, (INT8S *)ApQ_para, sizeof(play_index));
			DBG_PRINT("MSG_APQ_BROWSE_PLAY_FILE=%d\r\n", play_index);

			search_type = STOR_SERV_SEARCH_GIVEN | (play_index << 16);
			ap_browse_open_file_req((void *)&search_type); // Return MSG_STORAGE_SERVICE_BROWSE_REPLY
			break;

		case MSG_APQ_MJPEG_DECODE_PAUSE:
			ap_browse_func_key_active();
			break;

		case MSG_STORAGE_SERVICE_BROWSE_REPLY:
			ap_browse_reply_action((STOR_SERV_PLAYINFO *) ApQ_para);

			#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
		    if((state_wifi_status_get() & FLAG_CONNECT) &&
				(state_wifi_status_get() & FLAG_OFF_DISPLAY)
			)
			{
				if (GPSOCK_SD_Total_File_Get())
					ap_browse_func_key_active();
				else DBG_PRINT("no sd file\r\n");
		   	}
			#endif

			break;

		case MSG_APQ_NEXT_KEY_ACTIVE:
			if(ap_browse_sts_get() & BROWSE_OPEN_FILE_NOW)
			{
				break;
			}

			ap_browse_next_key_active(ApQ_para[0]);
			break;

		case MSG_APQ_FORWARD_FAST_PLAY:
			if((ap_browse_sts_get() == 0) || (ap_browse_sts_get() & BROWSE_PLAYBACK_PAUSE))
			{
                ;
			}
			else
			{
				INT8U	speed_temp = 0;

				if(browser_play_speed >= 3)
				{
					browser_play_speed = 3;
					break;
				}

				browser_play_speed++;

				ap_browse_fast_play_icon_show(browser_play_speed);

				if(browser_play_speed >= 0)
				{
					if(ap_browse_get_curr_file_type() == TK_IMAGE_TYPE_WAV)
					{
						ap_state_audio_reverse_set(0);
						ap_state_audio_play_speed_set(browser_play_speed);
					}
					else
					{
						// 要是 vid_dec_set_reverse_play fail , 就不要再去設定速度-----
               			// 並且回復之前的狀態
					    pre_val = p_vid_dec_para->reverse_play_set;
						ret = vid_dec_set_reverse_play(0);
						if(ret==0)
						{
							p_vid_dec_para->reverse_play_set = 0;
							vid_dec_set_play_speed(0x10000 << browser_play_speed);
						}
						else
						{
							browser_play_speed--;
                        	p_vid_dec_para->reverse_play_set = pre_val;
						}
					}
				}
				else
				{
					//speed_temp = ~browser_play_speed;
                    if(browser_play_speed==-1)
						speed_temp = 1;
                    if(browser_play_speed==-2)
                        speed_temp = 2;
                    if(browser_play_speed==-3)
                        speed_temp = 3;
					if(ap_browse_get_curr_file_type() == TK_IMAGE_TYPE_WAV)
					{
						ap_state_audio_reverse_set(1);
						ap_state_audio_play_speed_set(speed_temp);
					}
					else
					{
					// 要是 vid_dec_set_reverse_play fail , 就不要再去設定速度-----
              	    // 並且回復之前的狀態
                        pre_val = p_vid_dec_para->reverse_play_set;
						ret=vid_dec_set_reverse_play(1);
						if(ret==0)
						{
						 p_vid_dec_para->reverse_play_set = 1;
						 vid_dec_set_play_speed(0x10000 << speed_temp);
						}

						else
						{
                         browser_play_speed--;
						 p_vid_dec_para->reverse_play_set = pre_val;
						}

					}
				}
			}

			break;

		case MSG_APQ_PREV_KEY_ACTIVE:
			if(ap_browse_sts_get() & BROWSE_OPEN_FILE_NOW)
			{
				break;
			}

			ap_browse_prev_key_active(ApQ_para[0]);
			break;

		case MSG_APQ_BACKWORD_FAST_PLAY:
			if((ap_browse_sts_get() == 0) || (ap_browse_sts_get() & BROWSE_PLAYBACK_PAUSE))
			{ }
			else
			{
				INT8U	speed_temp = 0;

				if(browser_play_speed <= -3)
				{
					browser_play_speed = -3;
					break;
				}

				browser_play_speed--;

				ap_browse_fast_play_icon_show(browser_play_speed);

				if(browser_play_speed >= 0)
				{
					if(ap_browse_get_curr_file_type() == TK_IMAGE_TYPE_WAV)
					{
						ap_state_audio_reverse_set(0);
						ap_state_audio_play_speed_set(browser_play_speed);
					}
					else
					{
						// 要是 vid_dec_set_reverse_play fail , 就不要再去設定速度-----
                     	// 並且回復之前的狀態
					    pre_val = p_vid_dec_para->reverse_play_set;
						ret=vid_dec_set_reverse_play(0);
						if(ret==0)
						{
						 p_vid_dec_para->reverse_play_set = 0;
						 vid_dec_set_play_speed(0x10000 << browser_play_speed);
						}
						else
						{
						 browser_play_speed++;
						 p_vid_dec_para->reverse_play_set = pre_val;
						}
					}
				}
				else
				{
					//speed_temp = ~browser_play_speed;
					if(browser_play_speed==-1)
                        speed_temp = 1;
                    if(browser_play_speed==-2)
                        speed_temp = 2;
                    if(browser_play_speed==-3)
                        speed_temp = 3;
					if(ap_browse_get_curr_file_type() == TK_IMAGE_TYPE_WAV)
					{
						ap_state_audio_reverse_set(1);
						ap_state_audio_play_speed_set(speed_temp);
					}
					else
					{
                        // 要是 vid_dec_set_reverse_play fail , 就不要再去設定速度-----
                        // 並且回復之前的狀態
                        pre_val = p_vid_dec_para->reverse_play_set;
						ret=vid_dec_set_reverse_play(1);
                        if(ret==0)
                        {
                            p_vid_dec_para->reverse_play_set = 1;
                            vid_dec_set_play_speed(0x10000 << speed_temp);
                        }
                        else
                        {
                            browser_play_speed++;
                            p_vid_dec_para->reverse_play_set = pre_val;
                        }
					}
				}
			}

			break;

		case MSG_APQ_VOLUME_SHOW_END:
			ap_browse_volume_icon_clear_all();
			break;

		case MSG_APQ_MJPEG_DECODE_END:
			DBG_PRINT("MSG_APQ_MJPEG_DECODE_END \r\n");

            if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
            {
                ap_browse_wav_stop(NULL);
            }
            else
            {
                ap_browse_mjpeg_stop();
                ap_browse_mjpeg_play_end();
            }
			if(browse_curr_file.file_handle > -1)
			{
				close(browse_curr_file.file_handle);
				browse_curr_file.file_handle = -1;
			}

			#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
		    if((state_wifi_status_get() & FLAG_CONNECT) &&
				(state_wifi_status_get() & FLAG_OFF_DISPLAY)
			)
			{
				rtsp_h264_stop_service();
				break;
		    }
			else
			#endif
			{
				if(ap_state_handling_storage_id_get() != STORAGE_DEVICE)
				{
					ap_browse_sts_set(BROWSE_SD_NOT_EXIST);
					ap_browse_stop_handle();
				}
				else
				{
					/*
						Reopen file and show first frame
					*/
					search_type = STOR_SERV_SEARCH_ORIGIN;
					storage_task_msg_send(MSG_STORAGE_SERVICE_BROWSE_REQ, (void *) &search_type, sizeof(INT32U), MSG_PRI_NORMAL);
				}
			}
			break;

		#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
		case MSG_APQ_WIFI_CONNECT:
		case MSG_APQ_WIFI_DISCONNECT:
			//state_browse_exit(msg_id);
            #if C_WIFI_ON_OFF_KEY
            sw_wifi_ack_send(0);
            #endif

            if(msg_id == MSG_APQ_WIFI_CONNECT)
            {
                #if C_WIFI_ON_OFF_KEY == 0
                DBG_PRINT("->connect \r\n");
                state_handling_msg_send(STATE_VIDEO_RECORD, NULL, 0, MSG_PRI_NORMAL);
                exit_flag = EXIT_BREAK;
                state_wifi_symbol_show(1);
                #endif
            }
            else
            {
                DBG_PRINT("->disconnect \r\n");
                if(ap_browse_sts_get() & (BROWSE_PLAYBACK_PAUSE | BROWSE_PLAYBACK_BUSY))
                {
                    if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
                    {
                        ap_browse_wav_stop(NULL);
                    }
                    else
                    {
                        ap_browse_mjpeg_stop();
                        ap_browse_mjpeg_play_end();
                    }
                    if(browse_curr_file.file_handle > -1)
                    {
                        close(browse_curr_file.file_handle);
                        browse_curr_file.file_handle = -1;
                    }
                }
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
                state_handling_msg_send(prev_state, NULL, 0, MSG_PRI_NORMAL);
                exit_flag = EXIT_BREAK;
                state_wifi_symbol_show(0);
            }
            DBG_PRINT("==WIFI SET OK!==\r\n");
            #if C_WIFI_ON_OFF_KEY == 0
            sw_wifi_ack_send(0);
            #endif
            break;
		#endif

        #if(defined(C_USB_DEVICE) && C_USB_DEVICE)
		case MSG_APQ_CONNECT_TO_PC:
			state_browse_exit(msg_id);
			osDelay(100);
            ap_state_handling_connect_to_pc(STATE_BROWSE);
            break;

		case MSG_APQ_DISCONNECT_TO_PC:
			ap_state_handling_disconnect_to_pc();
			osDelay(100);
			video_encode_sensor_off();
			ap_browse_init(STATE_SETTING, play_index);
			break;
        #endif

		#if C_BATTERY_DETECT
		case MSG_APQ_BATTERY_LVL_SHOW:
			ap_state_handling_battery_icon_show(ApQ_para[0]);
			ap_browse_display_update_command(); // give DRAW command
			break;

		case MSG_APQ_BATTERY_CHARGED_SHOW:
			ap_state_handling_charge_icon_show(1);
			ap_browse_display_update_command(); // give DRAW command
			break;

		case MSG_APQ_BATTERY_CHARGED_CLEAR:
			ap_state_handling_charge_icon_show(0);
			ap_browse_display_update_command(); // give DRAW command
			break;

		#endif // #if C_BATTERY_DETECT

		#if C_BATTERY_DETECT && C_BATTERY_LOW_POWER_OFF
		case MSG_APQ_BATTERY_LOW_SHOW:
			ap_state_handling_show_battery_low();
			break;
		#endif

		#if C_SCREEN_SAVER
		case MSG_APQ_KEY_IDLE:
			ap_state_handling_lcd_backlight_switch(0);
			break;

		case MSG_APQ_KEY_WAKE_UP:
			ap_state_handling_lcd_backlight_switch(1);
			break;
		#endif // #if C_SCREEN_SAVER

		case MSG_APQ_INIT_THUMBNAIL:
			if(ap_browse_sts_get() & BROWSE_OPEN_FILE_NOW)
			{
				osDelay(10);
				msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
				break;
			}

			state_handling_msg_send( STATE_THUMBNAIL, NULL, 0, MSG_PRI_NORMAL);
			exit_flag = EXIT_BREAK;
			break;

		case MSG_APQ_USER_CONFIG_STORE:
			ap_state_config_store();
			break;

		case MSG_APQ_AUDIO_EFFECT_OK:
			if((ap_browse_sts_get() == 0) || (ap_browse_sts_get() & BROWSE_SD_NOT_EXIST))
			{
				if((ap_browse_sts_get() & BROWSE_OPEN_FILE_NOW) == 0)
				{
					if(ap_state_common_handling(MSG_APQ_AUDIO_EFFECT_OK) == 0)
					{
						func_key_act = 1;
					}
				}
 			}

			break;

		case MSG_APQ_DISPLAY_DRAW_TIMER:
            //DBG_PRINT("u");
            #if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
            if(ap_browse_get_curr_file_type() == TK_IMAGE_TYPE_WAV)
            ap_audio_ui_draw();
            #endif
			break;

		case MSG_APQ_AUDIO_EFFECT_UP:
		case MSG_APQ_AUDIO_EFFECT_DOWN:
			break;

		case MSG_APQ_AUDIO_EFFECT_MODE:
		case MSG_APQ_AUDIO_EFFECT_MENU:
			mode_key_enabled = 1;
			break;

		case MSG_APQ_BACK_SENSOR_PLUG_IN:
		case MSG_APQ_BACK_SENSOR_PLUG_OUT:
			if(msg_id == MSG_APQ_BACK_SENSOR_PLUG_IN)
			{
				back_sensor_plug_status_set(1);

				//restoring from config
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

				//deactivate dual cam preview modes
				preview_mode = PREVIEW_MODE_FRONT;
			}
			break;

		case MSG_APQ_BACK_A_CAR_ON:
            /* //To fix it
			if(!g_browser_reply_action_flag)
			{
				osDelay(10);
				msgQSend(ApQ, MSG_APQ_BACK_A_CAR_ON, NULL, 0, MSG_PRI_NORMAL);
				break;
			} */

			DBG_PRINT("browse status BACK A CAR ON\r\n");
			state_handling_msg_send(STATE_VIDEO_RECORD, NULL, 0, MSG_PRI_NORMAL);
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

		case MSG_APQ_WIFI_MODE_CHANGE:
			if(ap_browse_sts_get() & BROWSE_OPEN_FILE_NOW)
			{
				osDelay(10);
				msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
				break;
			}

            if((ap_browse_sts_get() & BROWSE_PLAYBACK_PAUSE)||(ap_browse_sts_get() & BROWSE_PLAYBACK_BUSY))
            {

                if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
                {
                    ap_browse_wav_stop(NULL);
                }
                else
                {
                    ap_browse_mjpeg_stop();
                    ap_browse_mjpeg_play_end();
                }
				if(browse_curr_file.file_handle > -1)
				{
					close(browse_curr_file.file_handle);
					browse_curr_file.file_handle = -1;
				}

                audio_effect_play(VOICE_ID_CLICK);
                search_type = STOR_SERV_SEARCH_ORIGIN;
				ap_browse_open_file_req((void *) &search_type);
                 break;
            }

			gp_memcpy((INT8S *)&msg_id, (INT8S *)ApQ_para, sizeof(msg_id));
			state_handling_msg_send(msg_id, NULL, 0, MSG_PRI_NORMAL);
			exit_flag = EXIT_BREAK;
			break;

		default:
			break;
		}
	}

	state_browse_exit(msg_id);
}

void state_browse_exit(INT32U next_state_msg)
{
	DBG_PRINT("%s ... \r\n", __FUNCTION__);

	ap_browse_exit(next_state_msg);
	gp_cdsp_task_priority(0);
	ap_state_handling_warn_str_icon_exit();
	if((next_state_msg != MSG_APQ_INIT_THUMBNAIL) && (next_state_msg != MSG_APQ_CONNECT_TO_PC) && (next_state_msg != MSG_APQ_MENU_KEY_ACTIVE))
	{
		osDelay(10);
	}

	if(mode_key_enabled == 1)
	{
		audio_effect_play(VOICE_ID_CLICK);
		mode_key_enabled = 0;
	}
}


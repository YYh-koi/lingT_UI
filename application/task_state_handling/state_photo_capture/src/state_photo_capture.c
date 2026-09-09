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

typedef enum
{
	CAP_IDLE,
	CAP_CAPING
} CAP_ST;

typedef enum
{
	PEND_NONE=0,
	PEND_MENU,
	PEND_MODE
} PEND_CMD;

static CAP_ST cap_st;
static PEND_CMD pend_cmd;

INT32U photo_check_time_loop_count = 0;
INT32U photo_check_time_preview_count = 0;
INT8U  photo_check_time_begin = 1;

extern INT32U left_pic_num;
extern int g_wifi_notify_pic_done;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
extern INT32S wal_ap_mode_disable(void);
extern void wifi_ap_mode_enable(void);
extern INT32U wifi_transmission_stop_depen_on_mode(void);
extern INT32S wifi_transmission_stop(void);
#endif

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static void state_photo_check_timer_isr(void)
{
	photo_check_time_loop_count++;

	/*
		?有pend_cmd馬上跳離
	*/
	if(pend_cmd)
	{
		photo_check_time_loop_count = photo_check_time_preview_count;
	}

	if(photo_check_time_loop_count > photo_check_time_preview_count)
	{
		photo_check_time_begin = 0;
		time_base_stop(TIME_BASE_A);
	}
}

void state_photo_capture_init(void)
{
	DBG_PRINT("%s ... \r\n", __func__);

	ap_capture_init();
}

void state_photo_capture_exit(EXIT_FLAG_ENUM exit_flag)
{
	DBG_PRINT("%s ... \r\n", __func__);

	if(exit_flag != EXIT_BREAK_MENU_KEY)
	{
		#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
		if(wifi_transmission_stop_depen_on_mode())
		{
			wifi_transmission_stop();
		}
		#endif
		video_encode_preview_off();
		video_encode_sensor_off();
	}
	ap_capture_exit();
}

void
state_photo_capture_entry(
	void *para
)
{
	EXIT_FLAG_ENUM	exit_flag = EXIT_RESUME;
	STAudioConfirm	*audio_temp;
	INT8U	continuous_shooting_count = 0;
	INT32U	msg_id, file_path_addr = 0;
	INT32U	prev_state;
	INT8U	photo_check_time_flag;
	INT32U	photo_check_time_start = 0;
	INT32U	photo_check_time_end = 0;
	INT32U	photo_check_time_count_ms = 0;
	INT32U	photo_check_ui_setting_ms;

#if(defined(C_USB_DEVICE) && C_USB_DEVICE)
	INT8U usb_stream_on = 0;
#endif


	INT8U temp;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	cap_st = CAP_IDLE;
	pend_cmd = PEND_NONE;
	prev_state = *(INT32U *)para;

#if 0 // To fix it
	if (g_wifi_notify_pic_done)
	{	// capture fail or sequence capture fail
		// It should'nt happen.
		DBG_PRINT("reset g_wifi_notify_pic_done\r\n");
		g_wifi_notify_pic_done = 0;
	}
#endif

	/*
	拍照模式下快速檢?
	0	關
	1	2秒
	2	5秒
	*/
	// 連拍不啟動快速檢?
	// not to use burst when no SD card inserted
	if(ap_state_handling_storage_id_get() != STORAGE_DEVICE)
	{
		photo_check_time_flag = ap_state_config_capture_review_get();
	}
	else if(ap_state_config_capture_burst_get())
	{
		photo_check_time_flag = 0;
	}
	else
	{
		photo_check_time_flag = ap_state_config_capture_review_get();
	}

	/* ?作固定不可插拔的後拉 */
	drv_l2_sensor_ops_t *pSensor =  drv_l2_sensor_get_ops(1);
	if(pSensor == NULL)
	{
		back_sensor_plug_status_set(0);
	}

	state_photo_capture_init();
	
	#if C_ASYN_DBLE_TFT
	ap_switch_tft_ap_mode_set(STATE_PHOTO_CAPTURE);
	#endif
	
	if(prev_state == STATE_SETTING)
	{
		video_encode_preview_off();
		video_encode_sensor_off();
	}
#if (defined C_SENSOR_SAVE_RAW_DATA) && (C_SENSOR_SAVE_RAW_DATA)
	video_encode_sensor_on(STATE_SENSOR_SAVE_RAW);
#else
	video_encode_sensor_on(STATE_PHOTO_CAPTURE);
#endif
	video_encode_preview_mode_set(preview_mode);
	ap_capture_cdsp_config();

	DBG_PRINT("%s ... \r\n", __func__);

	while(exit_flag == EXIT_RESUME)
	{
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
		wifi_reference_state_set(CAPTURE_MODE);
#endif
		if (msgQReceive(ApQ, &msg_id, ApQ_para, ApQ_para_len) == STATUS_FAIL)
		{
			continue;
		}

		// exception process when we are taking a photo
		if(cap_st == CAP_CAPING)
		{
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
					DBG_PRINT("[Capture Mount OK]\r\n");
					ap_state_handling_storage_id_set(ApQ_para[0]);

					ap_state_handling_icon_clear_cmd(ICON_NO_SD, 0, 0, 0);
					ap_state_handling_icon_show_cmd(ICON_SD_CARD, 0, 0, 0);
					ap_state_handling_warn_str_icon_exit();
					left_pic_num = ap_capture_left_num_calculate();
					ap_capture_show_left_num(left_pic_num);
					break;

				case MSG_STORAGE_SERVICE_NO_STORAGE:
					DBG_PRINT("[Capture Mount FAIL]\r\n");
					ap_state_handling_storage_id_set(ApQ_para[0]);

					ap_capture_show_left_num(0);
					ap_state_handling_icon_clear_cmd(ICON_SD_CARD, 0, 0, 0);
					ap_state_handling_icon_show_cmd(ICON_NO_SD, 0, 0, 0);
					ap_state_handling_show_insert_sdc();
					break;

					// droping message
				case MSG_APQ_NEXT_KEY_ACTIVE:
				case MSG_APQ_PREV_KEY_ACTIVE:
				case MSG_APQ_AUDIO_EFFECT_UP:
				case MSG_APQ_AUDIO_EFFECT_DOWN:
				case MSG_APQ_AUDIO_EFFECT_OK:
				case MSG_APQ_CONNECT_TO_PC:
				case MSG_APQ_DISCONNECT_TO_PC:
					break;

					// pending message
				case MSG_APQ_MENU_KEY_ACTIVE:
					pend_cmd = PEND_MENU;
					break;

				case MSG_APQ_MODE:
					pend_cmd = PEND_MODE;
					break;

					// other cases
#if 0   //To fix it
					case MSG_APQ_BACK_A_CAR_ON:
					case MSG_APQ_BACK_A_CAR_OFF:
					case MSG_APQ_BACK_SENSOR_PLUG_IN:
					case MSG_APQ_BACK_SENSOR_PLUG_OUT:
#endif
				case MSG_STORAGE_SERVICE_PIC_REPLY:
					video_encode_preview_off();
					if(ap_capture_reply_action((STOR_SERV_FILEINFO *) ApQ_para) < 0)
					{
						cap_st = CAP_IDLE;
						video_encode_preview_mode_set(preview_mode);
						break;
					}
					file_path_addr = ((STOR_SERV_FILEINFO *) ApQ_para)->file_path_addr;
					break;

				case MSG_STORAGE_SERVICE_PIC_DONE:
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
					++g_wifi_notify_pic_done;
#endif
					if(file_path_addr)
					{
						ap_capture_reply_done(ApQ_para[0], file_path_addr);
						file_path_addr = 0;
					}
#if 0 // To fix it
					if((avi_encode_get_status() & C_AVI_ENCODE_USB_WEBCAM))
					{
						_post_message(videoEncodeApQ, MSG_AVI_START_USB_WEBCAM, videoEncodeMsgAck, 5000, MSG_PRI_NORMAL);
					}
#endif

					// 沒有啟動快速檢?,直接跑到MSG_APQ_CAPTURE_PREVIEW_ON
					if(photo_check_time_flag)
					{
						photo_check_time_begin = 1;
						photo_check_time_end = xTaskGetTickCount();
						photo_check_ui_setting_ms = ((photo_check_time_flag * 3) - 1) * 1000;			// ms
						photo_check_time_count_ms = (photo_check_time_end - photo_check_time_start);	// ms

						//DBG_PRINT("cunt_ms:%d, ui_ms:%d\r\n", photo_check_time_count_ms, photo_check_ui_setting_ms);

						// ?拍照花費時間小於UI設定,才啟動timer
						if(photo_check_time_count_ms < photo_check_ui_setting_ms)
						{
							INT8U i;

							photo_check_time_loop_count = 0;
							photo_check_time_preview_count = (photo_check_ui_setting_ms - photo_check_time_count_ms) / 1000;
							time_base_setup(TIME_BASE_A, TMBAS_1HZ, 0, state_photo_check_timer_isr);
							for(i=0;i<50;i++)
							{
								vTaskDelay(100);
								if(photo_check_time_begin == 0)
								break;
							}
						}
					}

					cap_st = CAP_IDLE;
					msgQSend(ApQ, MSG_APQ_CAPTURE_PREVIEW_ON, NULL, 0, MSG_PRI_URGENT);
					break;

				case MSG_APQ_POWER_KEY_ACTIVE:
				case MSG_APQ_SW_RESET:
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

				default:
					ap_state_common_handling(msg_id);
					break;
			}
		}
		else
		{
			switch(pend_cmd)	// resume the last one pended message
			{
				case PEND_MENU:
					msgQSend(ApQ, MSG_APQ_MENU_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
					break;

				case PEND_MODE:
					msgQSend(ApQ, MSG_APQ_MODE, NULL, 0, MSG_PRI_NORMAL);
					break;
				default:
					break;
			}

			pend_cmd = PEND_NONE;

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
					DBG_PRINT("[Capture Mount OK]\r\n");
					ap_state_handling_storage_id_set(ApQ_para[0]);

					ap_state_handling_icon_clear_cmd(ICON_NO_SD, 0, 0, 0);
					ap_state_handling_icon_show_cmd(ICON_SD_CARD, 0, 0, 0);
					ap_state_handling_warn_str_icon_exit();
					left_pic_num = ap_capture_left_num_calculate();
					ap_capture_show_left_num(left_pic_num);
					break;

				case MSG_STORAGE_SERVICE_NO_STORAGE:
					DBG_PRINT("[Capture Mount FAIL]\r\n");
					ap_state_handling_storage_id_set(ApQ_para[0]);

					ap_capture_show_left_num(0);
					ap_state_handling_icon_clear_cmd(ICON_SD_CARD, 0, 0, 0);
					ap_state_handling_show_insert_sdc();
					ap_state_handling_icon_show_cmd(ICON_NO_SD, 0, 0, 0);
					break;

				case MSG_APQ_MENU_KEY_ACTIVE:
					state_handling_msg_send(STATE_SETTING, NULL, 0, MSG_PRI_NORMAL);
					exit_flag = EXIT_BREAK_MENU_KEY;
					break;

                case MSG_APQ_WIFI_PIP_SWITCH:
				case MSG_APQ_NEXT_KEY_ACTIVE:
				case MSG_APQ_PREV_KEY_ACTIVE:
					if(back_a_car_status_get())
					{
						if(msg_id == MSG_APQ_NEXT_KEY_ACTIVE)
						{
							temp = ap_state_config_draw_line_mode_get();
							temp = (1 + temp) % 6;
							ap_state_config_draw_line_mode_set(temp);
						}
						else
						{
							temp = ap_state_config_draw_line_mode_get();
							temp = (temp + 5) % 6;
							ap_state_config_draw_line_mode_set(temp);
						}
					}
					else
					{
						if((msg_id == MSG_APQ_NEXT_KEY_ACTIVE) || (msg_id == MSG_APQ_WIFI_PIP_SWITCH))
						{
							if(back_sensor_plug_status_get())
							{
								preview_mode = (1 + preview_mode) % PREVIEW_MODE_TOTAL;

								// storing config
								ap_state_config_display_mode_set(preview_mode);
								video_encode_preview_mode_set(preview_mode);
								DBG_PRINT("SWITCH to preview mode:%d\r\n", preview_mode);
							}
						}
					}
					break;

			case MSG_APQ_MODE:
			#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
				state_handling_msg_send(STATE_AUDIO_RECORD, NULL, 0, MSG_PRI_NORMAL);
				#else
				#if C_ASYN_DBLE_TFT	 /* customize */
					state_handling_msg_send(STATE_VIDEO_RECORD, NULL, 0, MSG_PRI_NORMAL);
				#else
					state_handling_msg_send(STATE_BROWSE, NULL, 0, MSG_PRI_NORMAL);	
				#endif
				
			#endif
				exit_flag = EXIT_BREAK;
				break;

				case MSG_APQ_FUNCTION_KEY_ACTIVE:
				case MSG_APQ_CAPTURE_CONTINUOUS_SHOOTING:
					if(0
#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
					 || usb_stream_on
#endif
					 ){
						break;
					}
					if(ap_capture_func_key_active() >= 0)
					{
						cap_st = CAP_CAPING;
						photo_check_time_start = xTaskGetTickCount();
					}
					break;

				case MSG_APQ_CAPTURE_PREVIEW_ON:
					if(0
#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
					 || usb_stream_on
#endif
					 ){
						break;
					}
					video_encode_preview_mode_set(preview_mode);
					/*
					拍照模式下連拍
					*/
					if(ap_state_config_capture_burst_get() && (ap_state_handling_storage_id_get() == STORAGE_DEVICE))
					{
						continuous_shooting_count++;
#if C_SENSOR_SAVE_RAW_DATA
						if(continuous_shooting_count < 10)//CAPTURE_CONTINUOUS_SHOOTING_COUNT)
						{
							//wait for update sensor image to screen
							osDelay(150);
							msgQSend(ApQ, MSG_APQ_CAPTURE_CONTINUOUS_SHOOTING, NULL, 0, MSG_PRI_NORMAL);
						}
						else
						{
							audio_effect_play(VOICE_ID_CAMERA);
							continuous_shooting_count = 0;
							storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
						}

#else // #if C_SENSOR_SAVE_RAW_DATA

						if(continuous_shooting_count < CAPTURE_CONTINUOUS_SHOOTING_COUNT)
						{
							audio_effect_play(VOICE_ID_CAMERA);

							//wait for update sensor image to screen
							osDelay(200);
							msgQSend(ApQ, MSG_APQ_CAPTURE_CONTINUOUS_SHOOTING, NULL, 0, MSG_PRI_NORMAL);
						}
						else
						{
							continuous_shooting_count = 0;
#if 0 // To fix it
							g_wifi_notify_pic_done = 0;
#endif
						}
#endif // #if C_SENSOR_SAVE_RAW_DATA
					}
					break;

				case MSG_APQ_POWER_KEY_ACTIVE:
				case MSG_APQ_SW_RESET:
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

#if C_BATTERY_DETECT
				case MSG_APQ_BATTERY_LVL_SHOW:
					ap_state_handling_battery_icon_show(ApQ_para[0]);
					break;

				case MSG_APQ_BATTERY_CHARGED_SHOW:
					ap_state_handling_charge_icon_show(1);
					break;

				case MSG_APQ_BATTERY_CHARGED_CLEAR:
					ap_state_handling_charge_icon_show(0);
					break;
#endif // #if C_BATTERY_DETECT

#if C_BATTERY_DETECT && C_BATTERY_LOW_POWER_OFF
				case MSG_APQ_BATTERY_LOW_SHOW:
					ap_state_handling_show_battery_low();
					break;
#endif

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
				case MSG_APQ_WIFI_CONNECT:
				case MSG_APQ_WIFI_DISCONNECT:
#if C_WIFI_ON_OFF_KEY
					sw_wifi_ack_send(0);
#endif
#if C_TFT_DISPLAY_FUNC
#if C_WIFI_ON_OFF_KEY == 0
					video_encode_sensor_off();
#endif
#endif
					if(msg_id == MSG_APQ_WIFI_CONNECT)
					{
						DBG_PRINT("->connect \r\n");
#if C_TFT_DISPLAY_FUNC
#if C_WIFI_ON_OFF_KEY
						video_encode_sensor_off();
						wifi_ap_mode_enable();
#endif
						state_wifi_symbol_show(1);
#endif
#if C_WIFI_ON_OFF_KEY
						{
							INT8U type = WIFI_MODE_KEY;
							peripheral_task_msg_send(MSG_PERIPHERAL_TASK_KEY_REGISTER, &type, sizeof(INT8U), MSG_PRI_NORMAL);
						}
#endif
					}
					else
					{
						DBG_PRINT("->disconnect \r\n");
						rtsp_h264_stop_service();
#if C_WIFI_ON_OFF_KEY

						osDelay(100);	   //wait sensor frame to stop
						wifi_transmission_stop();
						video_encode_sensor_off();
						wal_ap_mode_disable();
						{
							INT8U type = KEY_MODE_NORMAL;
							peripheral_task_msg_send(MSG_PERIPHERAL_TASK_KEY_REGISTER, &type, sizeof(INT8U), MSG_PRI_NORMAL);
						}
#endif
						state_wifi_status_set(FLAG_DISCONNECT);
#if C_TFT_DISPLAY_FUNC
						state_wifi_symbol_show(0);
#endif
					}
#if C_TFT_DISPLAY_FUNC
					video_encode_sensor_on(STATE_PHOTO_CAPTURE);
					video_encode_preview_mode_set(preview_mode);
#endif
					DBG_PRINT("==WIFI SET OK!==\r\n");
#if C_WIFI_ON_OFF_KEY == 0
					sw_wifi_ack_send(0);
#endif
					break;
#endif

#if(defined(C_USB_DEVICE) && C_USB_DEVICE)
				case MSG_APQ_CONNECT_TO_PC:
					usb_stream_on = 1;
					video_encode_sensor_off();
					ap_state_handling_connect_to_pc(STATE_PHOTO_CAPTURE);
					break;

				case MSG_APQ_DISCONNECT_TO_PC:
					usb_stream_on = 0;
					ap_state_handling_disconnect_to_pc();
					video_encode_sensor_on(STATE_PHOTO_CAPTURE);
					video_encode_preview_mode_set(preview_mode);
					DBG_PRINT("SWITCH to preview mode:%d\r\n", preview_mode);

					if(back_sensor_plug_status_get() && back_a_car_status_get())
					{
						msgQSend(ApQ, MSG_APQ_BACK_A_CAR_ON, NULL, 0, MSG_PRI_NORMAL);
					}
					break;
#endif

				case MSG_APQ_USER_CONFIG_STORE:
					ap_state_config_store();
					break;

				case MSG_APQ_AUDIO_EFFECT_UP:
				case MSG_APQ_AUDIO_EFFECT_DOWN:
					break;

				case MSG_APQ_AUDIO_EFFECT_OK:
					if(ap_state_handling_storage_id_get() == STORAGE_DEVICE)
						audio_effect_play(VOICE_ID_CAMERA);
					break;

#if defined(C_FIRMWARE_UPGRADE) && C_FIRMWARE_UPGRADE
				case MSG_STORAGE_FIRMWARE_UPGRADE:
					state_handling_msg_send(STATE_FIRMWARE_UPGRADE, NULL, 0, MSG_PRI_NORMAL);
					exit_flag = EXIT_BREAK;
					break;
#endif // defined(C_FIRMWARE_UPGRADE) && C_FIRMWARE_UPGRADE

			case MSG_APP_SCREEN_BACKLIGHT_SWITCH:
				if(ap_peripheral_lcd_backlight_get()) // Blacklight off
				{
					ap_peripheral_lcd_backlight_set(BL_ON);

#if C_SCREEN_SAVER
					screen_saver_enable = 0;
#endif
				}
				else
				{
					ap_peripheral_lcd_backlight_set(BL_OFF);
#if C_SCREEN_SAVER
					screen_saver_enable = 1;
#endif
				}
			break;

				case MSG_APQ_BACK_SENSOR_PLUG_IN:
				case MSG_APQ_BACK_SENSOR_PLUG_OUT:
					if(msg_id == MSG_APQ_BACK_SENSOR_PLUG_IN)
					{
						DBG_PRINT("BACK SENSOR PLUG IN\r\n");

						back_sensor_plug_status_set(1);

						// restoring from config and active
						preview_mode = ap_state_config_display_mode_get();
						video_encode_preview_mode_set(preview_mode);
						DBG_PRINT("SWITCH to preview mode:%d\r\n", preview_mode);
					}
					else if(msg_id == MSG_APQ_BACK_SENSOR_PLUG_OUT)
					{
						DBG_PRINT("BACK SENSOR PLUG OUT\r\n");

						back_sensor_plug_status_set(0);

						// de-activate dual cam preview modes
						ap_state_config_display_mode_set(PREVIEW_MODE_PIP_REAR);
						preview_mode = PREVIEW_MODE_FRONT;
						video_encode_preview_mode_set(preview_mode);
					}
					break;

				case MSG_APQ_BACK_A_CAR_ON:
					temp = ap_state_config_reversing_display_mode_get();
					DBG_PRINT("[AI]reversing display mode = %d\r\n",temp);
					if(temp == 0)
					{
						video_encode_preview_mode_set(PREVIEW_MODE_BACK_KEEP_RATIO);
					}
					else if(temp == 1)
					{
						video_encode_preview_mode_set(PREVIEW_MODE_BACK);
					}
					else
					{
						DBG_PRINT("[AI]reversing display mode fail\r\n");
					}
					DBG_PRINT("BACK A CAR ON\r\n");
					temp = KEY_MODE_BACK_A_CAR;
					peripheral_task_msg_send(MSG_PERIPHERAL_TASK_KEY_REGISTER, &temp, sizeof(INT8U), MSG_PRI_NORMAL);
					break;

				case MSG_APQ_BACK_A_CAR_OFF:
					video_encode_preview_mode_set(preview_mode);
#if C_DISP_SLIDE_SAVE
					if(ap_state_config_crop_slide_flag_get())
					{
						ap_state_config_crop_reversing_get();
					}
#endif
					DBG_PRINT("BACK A CAR OFF\r\n");
					if(s_usbd_pin == 0)
					{
						temp = KEY_MODE_NORMAL;
						peripheral_task_msg_send(MSG_PERIPHERAL_TASK_KEY_REGISTER, &temp, sizeof(INT8U), MSG_PRI_NORMAL);
					}
					break;

#if C_SCREEN_SAVER
				case MSG_APQ_KEY_IDLE:
					ap_state_handling_lcd_backlight_switch(0);
					break;

				case MSG_APQ_KEY_WAKE_UP:
					ap_state_handling_lcd_backlight_switch(1);
					break;
#endif // #if C_SCREEN_SAVER

				case MSG_APQ_WIFI_MODE_CHANGE:
					gp_memcpy((INT8S *)&msg_id, (INT8S *)ApQ_para, sizeof(msg_id));
					state_handling_msg_send(msg_id, NULL, 0, MSG_PRI_NORMAL);
					exit_flag = EXIT_BREAK;
					break;

				default:
					ap_state_common_handling(msg_id);
					break;
			}
		}
	}

	state_photo_capture_exit(exit_flag);
	if(photo_check_time_flag)
	{
		time_base_stop(TIME_BASE_A);
	}
}


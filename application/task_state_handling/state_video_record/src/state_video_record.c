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

extern INT8U avi_enc_start_flag;

static INT8U record_MD_freeze;

static enum{
	V_NONE			= 0x0
	, V_BOOT		= 0x1
	, V_MOUNT_DONE	= 0x2
	, V_DET_DONE	= 0x4
	, V_RECORD_START = (V_BOOT | V_MOUNT_DONE | V_DET_DONE)
} auto_record_st = V_NONE;

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
PREVIEW_MODE preview_mode = PREVIEW_MODE_FRONT;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
INT8U state_record_MD_freeze_status_get(void)
{
	return record_MD_freeze;
}

void state_record_MD_freeze_release(void)
{
	record_MD_freeze = 0;
}

INT8S state_video_record_init(void)
{
	DBG_PRINT("%s ... \r\n", __func__);

	if(ap_video_record_init() == STATUS_OK)
	{
		return STATUS_OK;
	}
	else
	{
		return STATUS_FAIL;
	}
}

void state_video_record_exit(INT8U exit_flag)
{
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
	ap_video_record_exit();
	DBG_PRINT("Exit video_record state\r\n");
}

void
state_video_record_entry(
	void *para
)
{
	EXIT_FLAG_ENUM	exit_flag = EXIT_RESUME;
	STAudioConfirm	*audio_temp;
	INT32U msg_id, prev_state;
	INT8U  open_file_cnt = 0;
	INT8U  open_file_max = 1;

#if C_WATER_MARK == CUSTOM_ON
	INT8U draw_type;
#endif

	INT8U temp;

#if C_MOTION_DETECTION
	INT8U md_check_tick = 0, switch_flag = 0;
#endif

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	INT8U wifi_stream_on = 0;
#endif

#if(defined(C_USB_DEVICE) && C_USB_DEVICE)
	INT8U usb_stream_on = 0;
#endif

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	auto_record_st = 0;

	/* é_™CááßMí */

	prev_state = *(INT32U *)para;
	if(prev_state == STATE_STARTUP)
	{
		if(avi_enc_start_flag
#if C_GSENSOR_FUNCTION
			|| gsensor_power_on
#endif
		)
		{
			auto_record_st |= V_BOOT;
		}

		if(ap_state_handling_storage_id_get() != 0)
		{
			auto_record_st |= V_MOUNT_DONE;
		}
	}

	/* ·í§@©T©w¤£¥i´¡©Þªº«á©Ô */
	drv_l2_sensor_ops_t *pSensor =  drv_l2_sensor_get_ops(1);
	if(pSensor == NULL)
	{
		back_sensor_plug_status_set(0);
		auto_record_st |= V_DET_DONE;           //don't need to wait complete of rear sensor detection
	}

	if(back_sensor_plug_status_get())
	{
		open_file_max = 2;
		preview_mode = ap_state_config_display_mode_get();
	}
	else
	{
		open_file_max = 1;
		preview_mode = PREVIEW_MODE_FRONT;
	}

#if C_WATER_MARK == CUSTOM_ON
	if(ap_state_config_plate_number_onoff_get())
	{
		if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
		{
			draw_type = YUV420_DRAW;
		}
		else if(drv_l2_display_tft_fmt_get() == DISP_FMT_YUYV)
		{
			draw_type = YUYV_DRAW;
		}
		else
		{
			draw_type = RGB565_DRAW;
		}
		task_display_lock_on(1);                                                                    //lock display update to ensure icon data will not be accessed
		display_task_msg_send(MSG_DISPLAY_TASK_WATER_MARK_ICON_CLEAR, NULL, 0, MSG_PRI_NORMAL);     //disable icon show
		task_display_lock_off(1);                                                                   //re-enable display update, at this point user can enable water icon showing
		ap_state_handling_prepare_water_mark_icon(&water_mark_icon, draw_type);                     //prepare water mark icon for display

		//assign display position
		water_mark_icon.pos_x = POSITION_RTC_DATE_TIME_X;
		water_mark_icon.pos_y = POSITION_RTC_DATE_TIME_Y - display_num_item[0].icon_h;
		water_mark_icon.pos_x &= ~0x0001;
		water_mark_icon.pos_y &= ~0x0001;

		ap_state_handling_prepare_water_mark_icon(&water_mark_yuyv, YUYV_DRAW);                     //prepare water mark icon for record
		ap_state_handling_prepare_water_mark_icon(&water_mark_gp420, YUV420_DRAW);                  //prepare water mark icon for record
	}
#endif

	#if C_ASYN_DBLE_TFT
	ap_switch_tft_ap_mode_set(STATE_VIDEO_RECORD);
	#endif

	if(prev_state == STATE_SETTING)
	{
		video_encode_preview_off();
		video_encode_sensor_off();
	}

	state_video_record_init();

	video_encode_sensor_on(STATE_VIDEO_RECORD);

	video_encode_preview_mode_set(preview_mode);

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	wifi_reference_state_set(RECORD_MODE);
#endif

#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
    if(ap_state_handling_display_path_get() == DISPLAY_PATH_NORMAL)
    {
        ap_state_handling_display_path_set(DISPLAY_PATH_LVGL);
    }
#endif
	DBG_PRINT("%s ... \r\n", __func__);
	while(exit_flag == EXIT_RESUME)
	{
		if (msgQReceive(ApQ, &msg_id, ApQ_para, ApQ_para_len) == STATUS_FAIL)
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
				else if((audio_temp->result == AUDIO_ERR_DEC_FINISH) && (audio_temp->source_type == AUDIO_SRC_TYPE_FS))
				{
					audio_confirm_handler((STAudioConfirm *) ApQ_para);
				}
				else
				{
					audio_confirm_handler((STAudioConfirm *) ApQ_para);
				}
				break;

			case MSG_STORAGE_SERVICE_MOUNT:
				DBG_PRINT("[Record Mount OK]\r\n");

				ap_state_handling_icon_clear_cmd(ICON_NO_SD, 0, 0, 0);
				ap_state_handling_warn_str_icon_exit();
				ap_state_handling_icon_show_cmd(ICON_SD_CARD, 0, 0, 0);
				ap_video_record_show_left_recording_time();

				ap_video_record_sts_set(~VIDEO_RECORD_UNMOUNT);
				ap_state_handling_storage_id_set(ApQ_para[0]);

#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
				if(ap_state_config_md_get())
				{
					if(!(ap_video_record_sts_get() & VIDEO_RECORD_MD))
					{
						peripheral_task_msg_send(MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, 0, MSG_PRI_NORMAL);
					}
					if(motion_detect_timerid == 0xFF)
					{
						motion_detect_timerid = VIDEO_PREVIEW_TIMER_ID;
						drv_l2_sys_timer_set((void *) msgQSend, (void *) ApQ, MSG_APQ_MOTION_DETECT_TICK, motion_detect_timerid, MOTION_DETECT_CHECK_TIME_INTERVAL);
					}
				}
#endif // C_MOTION_DETECTION
				auto_record_st |= V_MOUNT_DONE;
				if(
					(!(auto_record_st & V_BOOT) || (auto_record_st == V_RECORD_START))
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
					&& !wifi_stream_on
#endif
#if(defined(C_USB_DEVICE) && C_USB_DEVICE)
#if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_OFF))
					&& !usb_stream_on
#endif
#endif
				)
				{
                    #if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT == 0)
					DBG_PRINT("AUTO_Record_start!!!!\r\n");
					auto_record_st &= ~V_BOOT;
					ap_video_record_func_key_active();
					#endif
				}
				break;

			case MSG_STORAGE_SERVICE_NO_STORAGE:
				if(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)
				{
					osDelay(10);
					msgQSend(ApQ, msg_id, &ApQ_para[0], sizeof(INT8U), MSG_PRI_NORMAL);
					break;
				}
				DBG_PRINT("[Record Mount FAIL]\r\n");
				ap_state_handling_storage_id_set(ApQ_para[0]);
#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
				if(ap_video_record_sts_get() & VIDEO_RECORD_MD)
				{
					ap_video_record_md_disable();
#if !C_MOTION_DET_NO_CANCEL
					ap_state_config_md_set(0);
					ap_video_record_md_icon_clear_all();
#endif
				}
#endif
				if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
				{
					ap_video_record_func_key_active();
				}
				ap_video_record_clear_left_recording_time();
				ap_state_handling_warn_str_icon_exit();
				ap_state_handling_icon_clear_cmd(ICON_RECORD_LOCKED, 0, 0, 0);
				ap_state_handling_icon_clear_cmd(ICON_SD_CARD, 0, 0, 0);
				ap_state_handling_icon_show_cmd(ICON_NO_SD, 0, 0, 0);

				g_lock_current_file_flag = 0;
				g_cycle_record_continuing_flag = 0;

				ap_video_record_sts_set(VIDEO_RECORD_UNMOUNT);

				auto_record_st |= V_MOUNT_DONE;
				if(
					(auto_record_st == V_RECORD_START)
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
					&& !wifi_stream_on
#endif
#if(defined(C_USB_DEVICE) && C_USB_DEVICE)
					&& !usb_stream_on
#endif
				)
				{
                    #if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT == 0)
					auto_record_st &= ~V_BOOT;

					DBG_PRINT("AUTO_Record_start!!!!\r\n");
					ap_video_record_func_key_active();
					#endif
				}
				break;

#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
			case MSG_APQ_MOTION_DETECT_TICK:
				if(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)
				{
					osDelay(10);
					msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
					break;
				}
				ap_video_record_md_tick(&md_check_tick);
				break;

			case MSG_APQ_MOTION_DETECT_ACTIVE:
#if C_MOTION_DET_NO_CANCEL
				if(record_MD_freeze)
				{
					DBG_PRINT("md active cancel!\r\n");
					break;
				}
#endif
				if(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)
				{
					osDelay(10);
					msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
					break;
				}
				ap_video_record_md_active(&md_check_tick);
				break;
#endif // #if C_MOTION_DETECTION

			case MSG_APQ_FUNCTION_KEY_ACTIVE:
                #if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
                if(ap_state_handling_display_path_get() == DISPLAY_PATH_LVGL)
                {
                    /* ignore ok key when display LVGL */
                    break;
                }
                #endif
				auto_record_st &= ~V_BOOT;
				if(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)
				{
					osDelay(10);
					msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
					break;
				}

#if C_MOTION_DETECTION
				if(ap_video_record_sts_get() & VIDEO_RECORD_MD)
				{
					ap_video_record_md_disable();
#if !C_MOTION_DET_NO_CANCEL
					ap_state_config_md_set(0);
					ap_video_record_md_icon_clear_all();
#endif
				}
#endif // #if C_MOTION_DETECTION
				ap_video_record_func_key_active();
				break;

			case MSG_APQ_POWER_ON_AUTO_RECORD:
				auto_record_st |= V_DET_DONE;
#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
				if(ap_video_record_sts_get() & VIDEO_RECORD_MD)
				{
					auto_record_st &= ~V_BOOT;
				}
				else
#endif
				{
					if(
						auto_record_st == V_RECORD_START
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
						&& !wifi_stream_on
#endif
#if(defined(C_USB_DEVICE) && C_USB_DEVICE)
						&& !usb_stream_on
#endif
					)
					{
                        #if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT == 0)
						auto_record_st &= ~V_BOOT;
						DBG_PRINT("AUTO_Record_start!!!!\r\n");
						ap_video_record_func_key_active();
						#endif
					}
				}
				break;

			case MSG_APQ_POWER_KEY_ACTIVE:
			case MSG_APQ_SW_RESET:
				if(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)
				{
					osDelay(10);
					msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
					break;
				}

				if((ap_video_record_sts_get() & VIDEO_RECORD_BUSY))
				{
					ap_video_record_func_key_active();
				}

				ap_video_record_clear_left_recording_time();
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

#if C_SUPORT_GET_JPG_BUF
			case MSG_APQ_GET_JPEG_ON_RECORDING:
				if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
				{
					storage_task_msg_send(MSG_STORAGE_SERVICE_PIC_REQ2, NULL, 0, MSG_PRI_NORMAL);
				}
				break;

			case MSG_STORAGE_SERVICE_PIC_REPLY:
				ap_video_record_get_jpeg((STOR_SERV_FILEINFO *) ApQ_para);
				break;
			case MSG_SNAPSHOT_JPEG_DONE:
			case MSG_SNAPSHOT_JPEG_DONE_REAR:
				ap_video_record_get_jpeg_write_file((msg_id==MSG_SNAPSHOT_JPEG_DONE ? 0 : 1),(SNAPSHOT_INFO *) ApQ_para);
				break;
#endif // #if C_SUPORT_GET_JPG_BUF

			case MSG_APQ_MENU_KEY_ACTIVE:
				if((ap_video_record_sts_get() & VIDEO_RECORD_BUSY))
				{
					break;
				}
#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
				if(ap_video_record_sts_get() & VIDEO_RECORD_MD)
				{
					ap_video_record_md_disable();

#if !C_MOTION_DET_NO_CANCEL
					ap_state_config_md_set(0);
					ap_video_record_md_icon_clear_all();
#endif
				}
#endif
				auto_record_st &= ~V_BOOT;

				state_handling_msg_send(STATE_SETTING, NULL, 0, MSG_PRI_NORMAL);
				exit_flag = EXIT_BREAK_MENU_KEY;
				break;

			case MSG_APQ_MODE:
                #if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
				if(ap_state_handling_display_path_get() == DISPLAY_PATH_LVGL)
				{
                    ap_state_handling_display_path_set(DISPLAY_PATH_NORMAL);
				}
				else if(ap_state_handling_display_path_get() == DISPLAY_PATH_NORMAL)
				{
                    ap_video_record_clear_left_recording_time();
                    if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
                    {
                        ap_video_record_func_key_active();
                    }

                    auto_record_st &= ~V_BOOT;

                    state_handling_msg_send(STATE_BROWSE, NULL, 0, MSG_PRI_NORMAL);
                    exit_flag = EXIT_BREAK;
				}
				#else
				if(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)
				{
					osDelay(10);
					msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
					break;
				}
				ap_video_record_clear_left_recording_time();
#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
				if(ap_video_record_sts_get() & VIDEO_RECORD_MD)
				{
					ap_video_record_md_disable();
#if !C_MOTION_DET_NO_CANCEL
					ap_state_config_md_set(0);
					ap_video_record_md_icon_clear_all();
#endif
					osDelay(50);
				}
#endif // #if C_MOTION_DETECTION

				if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
				{
					ap_video_record_func_key_active();
				}

				auto_record_st &= ~V_BOOT;

				state_handling_msg_send(STATE_PHOTO_CAPTURE, NULL, 0, MSG_PRI_NORMAL);
				exit_flag = EXIT_BREAK;
				#endif
				break;

			case MSG_STORAGE_SERVICE_VID_REPLY:
				ap_video_record_reply_action((STOR_SERV_FILEINFO *) ApQ_para);
				break;

#if C_CYCLIC_VIDEO_RECORD
			case MSG_STORAGE_SERVICE_VID_CYCLIC_REPLY:
				ap_video_record_cycle_reply_action((STOR_SERV_FILEINFO *) ApQ_para);
				break;

				/*
				Send MSG_APQ_CYCLIC_VIDEO_RECORD early by packer when the recording time is less than half
				*/
			case MSG_APQ_CYCLIC_VIDEO_RECORD:
				if(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)
				{
					break;
				}
				else if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
				{
#if C_AUTO_DEL_FILE
					INT32U msg;
#endif
#if C_AUTO_DEL_FILE
					storage_task_msg_send_with_ack(MSG_STORAGE_SERVICE_AUTODEL_LOCK,5000);
#endif
					/*
					ëp?•r, file system •þ¨ç´Î é_ A/B file
					Ìí¼Ó open_file_cnt & open_file_max
					žé·À? Packer •rég²î °lËÍ 2 ´Î MSG_APQ_CYCLIC_VIDEO_RECORD
					*/
					open_file_cnt++;
					if(open_file_cnt == open_file_max)
					{
						open_file_cnt = 0;
						ap_video_record_sts_set(VIDEO_RECORD_WAIT);
						storage_task_msg_send(MSG_STORAGE_SERVICE_VID_H264_CYCLIC_REQ, NULL, 0, MSG_PRI_NORMAL);
					}
				}
				break;
#endif // #if C_CYCLIC_VIDEO_RECORD

            case MSG_APQ_WIFI_PIP_SWITCH:
			case MSG_APQ_NEXT_KEY_ACTIVE:
				/*
				//test
				preview_slide_down(5, 1);
				break;
				*/

				if(back_a_car_status_get())
				{
					temp = ap_state_config_draw_line_mode_get();
					temp = (1 + temp) % 6;
					ap_state_config_draw_line_mode_set(temp);
				}
				else
				{
					if(back_sensor_plug_status_get())
					{
						preview_mode = (1 + preview_mode) % PREVIEW_MODE_TOTAL;

						//storing config
						ap_state_config_display_mode_set(preview_mode);
						video_encode_preview_mode_set(preview_mode);
						DBG_PRINT("SWITCH to preview mode:%d\r\n", preview_mode);
					}
					else
					{
						if((ap_video_record_sts_get() & VIDEO_RECORD_BUSY) && ((ap_video_record_sts_get() & VIDEO_RECORD_WAIT) == 0))
						{
							msgQSend(ApQ, MSG_APQ_SOS_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
						}
					}
				}
				break;

			case MSG_APQ_PREV_KEY_ACTIVE:
				/*
				//test
				preview_slide_up(5, 1);
				break;
				*/
				#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
				#else
				if(back_a_car_status_get())
				{
					temp = ap_state_config_draw_line_mode_get();
					temp = (temp + 5) % 6;
					ap_state_config_draw_line_mode_set(temp);
				}
				else
				{
					INT8U temp;

					temp = ap_state_config_voice_record_switch_get();
					if(temp)
					{
						ap_state_config_voice_record_switch_set(0);
						ap_state_handling_icon_show_cmd(ICON_RECORD_MIC_OFF, 0, 0, 0);
					}
					else
					{
						ap_state_config_voice_record_switch_set(1);
						ap_state_handling_icon_clear_cmd(ICON_RECORD_MIC_OFF, 0, 0, 0);
					}
				}
				#endif
				break;

			case MSG_APQ_CAM_MODE_CHANGE:
				break;

#if C_GSENSOR_FUNCTION
			case MSG_APQ_PARK_MODE_SET:
				if(ap_state_config_park_mode_G_sensor_get())
				{
					ap_state_config_park_mode_G_sensor_set(0);
					ap_video_record_clear_park_mode();
				}
				else
				{
					ap_state_config_park_mode_G_sensor_set(1);
					ap_video_record_show_park_mode();
				}
				break;
#endif // #if C_GSENSOR_FUNCTION

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
			case MSG_APQ_WIFI_CONNECT:
			case MSG_APQ_WIFI_DISCONNECT:
				//DBG_PRINT("\r\n==>WIFI_CONECT/DISCONECT(0x%x)\r\n", msg_id);
#if C_WIFI_ON_OFF_KEY
				sw_wifi_ack_send(0);
#endif
				if(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)
				{
					osDelay(10);
					msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
					break;
				}

				if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
				{
					ap_video_record_func_key_active();
					osDelay(100);
				}
				auto_record_st &= ~V_BOOT;
#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
				if(ap_video_record_sts_get() & VIDEO_RECORD_MD)
				{
					ap_video_record_md_disable();
#if !C_MOTION_DET_NO_CANCEL
					ap_state_config_md_set(0);
					ap_video_record_md_icon_clear_all();
#endif
				}
#endif // #if C_MOTION_DETECTION
#if C_TFT_DISPLAY_FUNC
#if C_WIFI_ON_OFF_KEY == 0
				video_encode_sensor_off();
#endif
#endif
				if(msg_id == MSG_APQ_WIFI_CONNECT)
				{
					wifi_stream_on = 1;
					DBG_PRINT("->connect \r\n");
#if C_TFT_DISPLAY_FUNC
#if C_WIFI_ON_OFF_KEY
					video_encode_sensor_off();
					wifi_ap_mode_enable();
#endif
					state_wifi_symbol_show(1);
#endif
#if C_WIFI_ON_OFF_KEY
                    #if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT == 0)
					{
						INT8U type = WIFI_MODE_KEY;
						peripheral_task_msg_send(MSG_PERIPHERAL_TASK_KEY_REGISTER, &type, sizeof(INT8U), MSG_PRI_NORMAL);
					}
					#endif
#endif
				}
				else
				{
					wifi_stream_on = 0;
					DBG_PRINT("->disconnect \r\n");
					rtsp_h264_stop_service();
#if C_WIFI_ON_OFF_KEY
					osDelay(100);       //wait sensor frame to stop
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
				video_encode_sensor_on(STATE_VIDEO_RECORD);
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
				if(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)
				{
					osDelay(10);
					msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
					break;
				}
				if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
				{
					ap_video_record_func_key_active();
				}
				auto_record_st &= ~V_BOOT;
#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
				if(ap_video_record_sts_get() & VIDEO_RECORD_MD)
				{
					ap_video_record_md_disable();
#if !C_MOTION_DET_NO_CANCEL
					ap_state_config_md_set(0);
					ap_video_record_md_icon_clear_all();
#endif
				}
#endif // #if C_MOTION_DETECTION
				usb_stream_on = 1;
				video_encode_sensor_off();
				ap_state_handling_connect_to_pc(STATE_VIDEO_RECORD);
				break;

			case MSG_APQ_DISCONNECT_TO_PC:
				auto_record_st &= ~V_BOOT;

				ap_state_handling_disconnect_to_pc();
				if(ap_state_handling_storage_id_get() == STORAGE_DEVICE)
				{
					ap_video_record_show_left_recording_time();
				}

				video_encode_sensor_on(STATE_VIDEO_RECORD);
				video_encode_preview_mode_set(preview_mode);
				usb_stream_on = 0;
				if(back_sensor_plug_status_get() && back_a_car_status_get())
				{
					msgQSend(ApQ, MSG_APQ_BACK_A_CAR_ON, NULL, 0, MSG_PRI_NORMAL);
				}

#if C_MOTION_DETECTION && C_MOTION_DET_NO_CANCEL
				#if (C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
				if(ap_state_config_md_get())
				{
					//ap_state_handling_icon_show_cmd(ICON_MOTION_DETECT, NULL, NULL);
					ap_state_handling_icon_show_cmd(ICON_RECORD_MOTION_DETECT, 0, 0, 0);
					msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, NULL, MSG_PRI_NORMAL);
					ap_video_record_sts_set(VIDEO_RECORD_MD);
				}
				#endif				
#endif // #if C_MOTION_DETECTION && C_MOTION_DET_NO_CANCEL
				break;
#endif

			case MSG_APQ_RECORD_SWITCH_FILE:
#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
				if(!switch_flag)
				{
					switch_flag = 1;
					ap_video_record_func_key_active();
					ap_video_record_func_key_active();
				}
#endif
				break;

			case MSG_APQ_PACKER_ERROR:
				//DBG_PRINT("REC-PACKER_ERROR\r\n");
				if(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)
				{
					osDelay(10);
					msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
					break;
				}

				if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
				{
					ap_video_record_func_key_active();
				}

				// Dominant add. restart record again.
				if(drvl2_sdc_live_response(STORAGE_SD_DEV) == 0)
				{						// Card exist
					if(ap_state_config_md_get())
					{
						// motion detect mode..... no need restart
						break;
					}
					else
					{
						//restart
						ap_video_record_func_key_active();
					}
				}
				else
				{
#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
					// if no card, md detection function need to disable
					if(ap_video_record_sts_get() & VIDEO_RECORD_MD)
					{
						ap_video_record_md_disable();
#if !C_MOTION_DET_NO_CANCEL
						ap_state_config_md_set(0);
						ap_video_record_md_icon_clear_all();
#endif
					}
#endif
					storage_task_msg_send(MSG_STORAGE_SERVICE_STORAGE_CHECK, NULL, 0, MSG_PRI_NORMAL);
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
				if(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)
				{
					osDelay(10);
					msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
					break;
				}

				if((ap_video_record_sts_get() & VIDEO_RECORD_BUSY))
				{
					ap_video_record_func_key_active();
				}

				ap_state_handling_show_battery_low();
				break;
#endif // #if C_BATTERY_DETECT && C_BATTERY_LOW_POWER_OFF

			case MSG_APQ_USER_CONFIG_STORE:
				ap_state_config_store();
				break;

			case MSG_APQ_SOS_KEY_ACTIVE:
				if((ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
					&& (!(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)))
				{
					ap_state_handling_icon_show_cmd(ICON_RECORD_LOCKED, 0, 0, 0);
					ap_video_record_lock_current_file();
				}
				break;

			case MSG_APQ_BACK_SENSOR_PLUG_IN:
			case MSG_APQ_BACK_SENSOR_PLUG_OUT:
				if(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)
				{
					osDelay(10);
					msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
					break;
				}

				if((msg_id == MSG_APQ_BACK_SENSOR_PLUG_IN) && (back_sensor_plug_status_get() == 0))
				{
					DBG_PRINT("BACK SENSOR PLUG IN\r\n");

					back_sensor_plug_status_set(1);
					open_file_max = 2;

					//restoring from config and active
					preview_mode = ap_state_config_display_mode_get();
					video_encode_preview_mode_set(preview_mode);
				}
				else
				if((msg_id == MSG_APQ_BACK_SENSOR_PLUG_OUT) && (back_sensor_plug_status_get() == 1))
				{
					DBG_PRINT("BACK SENSOR PLUG OUT\r\n");

					back_sensor_plug_status_set(0);
					open_file_max = 1;

					//de-activate dual cam preview modes
					ap_state_config_display_mode_set(PREVIEW_MODE_PIP_REAR);
					preview_mode = PREVIEW_MODE_FRONT;
					video_encode_preview_mode_set(preview_mode);
				}
				else
				{
					//DBG_PRINT("=>msg_id:%x,%d\r\n",msg_id,back_sensor_plug_status_get());
					break;
				}

				if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
				{
					//stop recording
					ap_video_record_func_key_active();

					//restart recording
					ap_video_record_func_key_active();
				}
				break;

			case MSG_APQ_BACK_A_CAR_ON:
				temp = ap_state_config_reversing_display_mode_get() ;
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

#if defined(C_FIRMWARE_UPGRADE) && C_FIRMWARE_UPGRADE
			case MSG_STORAGE_FIRMWARE_UPGRADE:
				state_handling_msg_send(STATE_FIRMWARE_UPGRADE, NULL, 0, MSG_PRI_NORMAL);
				exit_flag = EXIT_BREAK;
				break;
#endif // defined(C_FIRMWARE_UPGRADE) && C_FIRMWARE_UPGRADE

		case MSG_APP_SCREEN_BACKLIGHT_SWITCH:
			if(ap_peripheral_lcd_backlight_get())
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

#if C_SCREEN_SAVER
			case MSG_APQ_KEY_IDLE:
				ap_state_handling_lcd_backlight_switch(0);
				break;

			case MSG_APQ_KEY_WAKE_UP:
				ap_state_handling_lcd_backlight_switch(1);
				break;
#endif // #if C_SCREEN_SAVER

			case MSG_APQ_WIFI_MODE_CHANGE:
				if(ap_video_record_sts_get() & VIDEO_RECORD_WAIT)
				{
					osDelay(10);
					msgQSend(ApQ, msg_id, NULL, 0, MSG_PRI_NORMAL);
					break;
				}
				ap_video_record_clear_left_recording_time();
#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
				if(ap_video_record_sts_get() & VIDEO_RECORD_MD)
				{
					ap_video_record_md_disable();

#if !C_MOTION_DET_NO_CANCEL
					ap_state_config_md_set(0);
					ap_video_record_md_icon_clear_all();
#endif
					osDelay(50);
				}
#endif // #if C_MOTION_DETECTION

				if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
				{
					ap_video_record_func_key_active();
				}

				auto_record_st &= ~V_BOOT;

				gp_memcpy((INT8S *)&msg_id, (INT8S *)ApQ_para, sizeof(msg_id));
				state_handling_msg_send(msg_id, NULL, 0, MSG_PRI_NORMAL);
				exit_flag = EXIT_BREAK;
				break;

			default:
				if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
				{
					if((msg_id == MSG_APQ_AUDIO_EFFECT_OK) || (msg_id == MSG_APQ_AUDIO_EFFECT_UP) || (msg_id == MSG_APQ_AUDIO_EFFECT_DOWN))
					{
						ap_state_common_handling(msg_id);
					}
				}
				else
				{
					ap_state_common_handling(msg_id);
				}
				break;
		}
	}
	state_video_record_exit(exit_flag);
}


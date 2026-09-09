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

STOR_SERV_FILEINFO	curr_file_info;
INT8S				video_record_sts;

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
STOR_SERV_FILEINFO	next_file_info;
static CHAR g_cycle_prev_file_path[24];
static CHAR g_cycle_prev_file_path_b[24];
#endif

#if C_MOTION_DETECTION == CUSTOM_ON
INT8U motion_detect_timerid = 0xFF;
#endif

INT8S g_lock_current_file_flag;
INT8U g_cycle_record_continuing_flag;

static INT32U record_start_time;

extern INT16U lock_file_table[];
extern INT16U h264_file_table[];
extern INT64U cur_lock_file_size;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
#if C_SUPORT_GET_JPG_BUF
extern INT32S video_encode_get_jpeg(void);
extern INT32S video_encode_release_get_jpeg_buf(INT8U src);
#endif

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
DISPLAY_ICONSHOW *allocate_and_draw_ascii_string_icon(const CHAR *string_in, INT16U color)
{
	STRING_ASCII_INFO   string_info = {0};
	t_STR_SIZE_STRUCT   str_size;
    DISPLAY_ICONSHOW    *str_icon;

	string_info.str_ptr = (CHAR *)string_in;
	string_info.font_color = color;
	string_info.font_type = 0;
	string_info.pos_x = 0;
	string_info.pos_y = 0;
    if(ap_state_resource_ascii_string_resolution_get(&string_info, &str_size) == STATUS_FAIL)
    {
        ERR_PRINT("fail to ap_state_resource_ascii_string_resolution_get\r\n");
	   	return NULL;
    }

	if((str_size.string_width == 0) || (str_size.string_height == 0))
	{
        ERR_PRINT("string_width or string_height is 0\r\n");
    	return NULL;
	}

    string_info.buff_w = str_size.string_width;
    string_info.buff_h = str_size.string_height;

	if(string_info.buff_w & 0x0001) string_info.buff_w += 1;
	if(string_info.buff_h & 0x0001) string_info.buff_h += 1;

    str_icon = gp_malloc(sizeof(DISPLAY_ICONSHOW));
    if(str_icon == NULL)
    {
        DBG_PRINT("%s icon malloc fail\r\n", __FUNCTION__);
        return NULL;
    }

    str_icon->transparent = TRANSPARENT_COLOR;
    str_icon->icon_w = string_info.buff_w;
    str_icon->icon_h = string_info.buff_h;
	if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
	{
        str_icon->data = gp_malloc(string_info.buff_w * string_info.buff_h * 3 >> 1);
    }
    else
    {
        str_icon->data = gp_malloc(string_info.buff_w * string_info.buff_h << 1);
    }

    if(str_icon->data == NULL)
    {
        DBG_PRINT("%s icon data malloc fail\r\n", __FUNCTION__);
        gp_free(str_icon);
        return NULL;
    }

	switch(drv_l2_display_tft_fmt_get())
	{
        INT32S i, j, cnt;

		case DISP_FMT_GP420:
			cnt = ((str_icon->icon_h * 3) >> 1);
			for(i = 0; i < cnt; i++)
			{
				if((i % 3) == 1)
				{
					gp_memset((INT8S *)(str_icon->data) + i * str_icon->icon_w, 0x80, str_icon->icon_w);
				}
				else
				{
					//gp_memset((INT8S *)(str_icon->data) + i * str_icon->icon_w, 0, str_icon->icon_w);
					gp_memset((INT8S *)(str_icon->data) + i * str_icon->icon_w, TRANSPARENT_COLOR_GP420, str_icon->icon_w);
				}
			}
            break;

		case DISP_FMT_YUYV:
            for(i = 0; i < str_icon->icon_h; i++)
            {
                for(j = 0; j < str_icon->icon_w; j++)
                {
                    //*(INT16U *)str_icon->data = 0x8000;
                    *(str_icon->data + i * str_icon->icon_w + j) = TRANSPARENT_COLOR_YUYV;
                }
            }
            break;

		default:
            for(i = 0; i < str_icon->icon_h; i++)
            {
                for(j = 0; j < str_icon->icon_w; j++)
                {
                    //*(INT16U *)str_icon->data = 0;
                    *(str_icon->data + i * str_icon->icon_w + j) = TRANSPARENT_COLOR_RGB565;
                }
            }
            break;
	}

    ap_state_resource_string_ascii_draw(str_icon->data, &string_info, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
    return str_icon;
}


void ap_video_record_show_resolution(void)
{
	const CHAR *resv_str[] = {"4K", "2K", "1080FHD", "1080P", "720P", "WVGA", "VGA", "QVGA"};
	INT8U	temp;
    DISPLAY_ICONSHOW    *str_res_icon;

	temp = ap_state_config_video_resolution_get();
	if(temp >= (sizeof(resv_str) / sizeof(CHAR *)))
	{
		temp = (sizeof(resv_str) / sizeof(CHAR *)) - 1;
	}

    str_res_icon = allocate_and_draw_ascii_string_icon(resv_str[temp], WHITE_COLOR);
    if(str_res_icon)
    {
        str_res_icon->pos_x = RECORD_RESV_POS_X;
        str_res_icon->pos_y = RECORD_RESV_POS_Y;
        display_task_msg_send(MSG_DISPLAY_TASK_RES_ICON_DRAW, &str_res_icon, sizeof(void *), MSG_PRI_NORMAL);
    }
}


void ap_video_record_clear_resolution(void)
{
    display_task_msg_send(MSG_DISPLAY_TASK_RES_ICON_CLEAR, NULL, 0, MSG_PRI_NORMAL);
}

void ap_video_record_show_park_mode(void)
{
    DISPLAY_ICONSHOW    *str_park_icon;

    str_park_icon = allocate_and_draw_ascii_string_icon("P", BLUE_COLOR);
    if(str_park_icon)
    {
        str_park_icon->pos_x = RECORD_PARK_POS_X;
        str_park_icon->pos_y = RECORD_PARK_POS_Y;
        display_task_msg_send(MSG_DISPLAY_TASK_PARK_ICON_DRAW, &str_park_icon, sizeof(void *), MSG_PRI_NORMAL);
    }
}

void ap_video_record_clear_park_mode(void)
{
    display_task_msg_send(MSG_DISPLAY_TASK_PARK_ICON_CLEAR, NULL, 0, MSG_PRI_NORMAL);
}

void ap_video_record_show_left_recording_time(void)
{
	const INT32U MB_per_min_front[] = {    MB_PER_MIN_4K,
                                            MB_PER_MIN_2K,
                                            MB_PER_MIN_1080FHD,
											MB_PER_MIN_1080P,
											MB_PER_MIN_720P,
											MB_PER_MIN_WVGA,
											MB_PER_MIN_VGA,
											MB_PER_MIN_QVGA
                                        };
	static const INT8U	temp2[6] = { 10, 1, 2, 3, 5, 10 };			//unit: minute
	INT64U				freespace, left_recording_time, file_size_to_create;
	INT8U				rec_time_config, rec_resolution_config;
	INT32U				MB_per_min, avi_file_size_a;
    INT32U              avi_file_size_b;
    DISPLAY_REC_TIME    disp_time;

	//DBG_PRINT("show left recording time\r\n");

	rec_time_config = ap_state_config_record_time_get();
	if(rec_time_config > 5)
		rec_time_config = 5;

	rec_resolution_config = ap_state_config_video_resolution_get();
	if(rec_resolution_config > 7)
		rec_resolution_config = 7;

	MB_per_min = MB_per_min_front[rec_resolution_config];
	avi_file_size_a = temp2[rec_time_config] * MB_per_min;

	file_size_to_create = avi_file_size_a;

	avi_file_size_b = temp2[rec_time_config] * MB_PER_MIN_BACK;		//75MB per minute
	if(back_sensor_plug_status_get())
	{
		file_size_to_create += avi_file_size_b;
	}

	file_size_to_create <<= 20;

	freespace = vfsFreeSpace(STORAGE_DEVICE);
	left_recording_time = freespace / file_size_to_create;			//number of file
	left_recording_time *= temp2[rec_time_config];					//minutes
	left_recording_time *= 60;										//seconds

    disp_time.s = left_recording_time % 60;
    left_recording_time /= 60;
    disp_time.m = left_recording_time % 60;
    left_recording_time /= 60;
    disp_time.h = left_recording_time % 100;

	display_task_msg_send(MSG_DISPLAY_TASK_LEFT_REC_TIME_DRAW, &disp_time, sizeof(DISPLAY_REC_TIME), MSG_PRI_NORMAL);
}

void ap_video_record_clear_left_recording_time(void)
{
	//DBG_PRINT("clear left recording time\r\n");
	display_task_msg_send(MSG_DISPLAY_TASK_LEFT_REC_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
}

#if C_MOTION_DETECTION
void ap_video_record_md_icon_clear_all(void)
{
	ap_state_handling_icon_clear_cmd(ICON_RECORD_MOTION_DETECT, 0, 0, 0);
}
#endif


INT8S ap_video_record_init(void)
{
	INT8U temp;

	g_lock_current_file_flag = 0;
	g_cycle_record_continuing_flag = 0;

	// clear all icons
	display_task_msg_send(MSG_DISPLAY_TASK_EFFECT_INIT, NULL, 0, MSG_PRI_NORMAL);

	// display RTC date time
    display_task_msg_send(MSG_DISPLAY_RTC_DATE_TIME_DRAW, NULL, 0, MSG_PRI_NORMAL);

    // water mark
    #if C_WATER_MARK == CUSTOM_ON
    if(ap_state_config_plate_number_onoff_get())
    {
        display_task_msg_send(MSG_DISPLAY_TASK_WATER_MARK_ICON_DRAW, NULL, 0, MSG_PRI_NORMAL);
    }
  	#endif

    // Mic on/off
	if(ap_state_config_voice_record_switch_get() == 0)
	{
		ap_state_handling_icon_show_cmd(ICON_RECORD_MIC_OFF, 0, 0, 0);
	}

	// video resolution
	ap_video_record_show_resolution();

    #if C_GSENSOR_FUNCTION
    //Park mode
	temp = ap_state_config_park_mode_G_sensor_get();
	if(temp)
	{
		ap_video_record_show_park_mode();
	}
	#endif

	// Cyclic record time
	temp = ap_state_config_record_time_get();
	if(temp)
	{
		ap_state_handling_icon_show_cmd(ICON_RECORD_RECORD_CYC_1MINUTE + temp - 1, 0, 0, 0);
	}

	// EV
	temp = ap_state_config_record_ev_get();
	ap_state_handling_icon_show_cmd(ICON_EV6 + temp, 0, 0, 0);

	drv_l2_cdsp_set_ev_val(temp);
	//0:+2, 1:+5/3, 2:+4/3, 3:+1.0, 4:+2/3, 5:+1/3, 6:+0.0, 7:-1/3, 8:-2/3, 9:-1.0, 10:-4/3, 11:-5/3, 12:-2.0
	drv_l2_cdsp_set_iso(1);
	// Light frequency
	temp = ap_state_config_light_freq_get();
	drv_l2_user_sensor_frequence(temp);

	#if C_BATTERY_DETECT
	ap_state_handling_current_bat_lvl_show();
	ap_state_handling_current_charge_icon_show();
	#endif

	#if C_CYCLIC_VIDEO_RECORD
	next_file_info.file_handle = -1;
	next_file_info.file_handle_b = -1;
	#endif

	ap_video_record_sts_set(0);
	if(ap_state_handling_storage_id_get() != STORAGE_DEVICE)
	{
		ap_video_record_sts_set(VIDEO_RECORD_UNMOUNT);
		ap_video_record_clear_left_recording_time();
		ap_state_handling_icon_show_cmd(ICON_VIDEO_RECORD, ICON_NO_SD, 0, 0);

		#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
        if(ap_state_handling_storage_id_get() == NO_STORAGE)
        {
            if(ap_state_config_md_get())
            {
                ap_video_record_md_disable();
                ap_state_config_md_set(0);
                ap_video_record_md_icon_clear_all();
            }
        }
		#endif
		#if (C_BLIND_SPOT_DETECTION==CUSTOM_ON)
		ap_state_config_md_set(1);
		if(ap_state_config_md_get())
		{
			peripheral_task_msg_send(MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, 0, MSG_PRI_NORMAL);
			ap_video_record_sts_set(VIDEO_RECORD_MD);
			//ap_state_handling_icon_show_cmd(ICON_RECORD_MOTION_DETECT, 0, 0, 0);
		}
		#endif
		return STATUS_FAIL;
	}
	else
	{
		ap_video_record_sts_set(~VIDEO_RECORD_UNMOUNT);
		ap_video_record_show_left_recording_time();
		ap_state_handling_icon_show_cmd(ICON_VIDEO_RECORD, ICON_SD_CARD, 0, 0);

		#if C_MOTION_DETECTION
		#if (C_BLIND_SPOT_DETECTION==CUSTOM_ON)		
		ap_state_config_md_set(1);
		#endif
		if(ap_state_config_md_get())
		{
			peripheral_task_msg_send(MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, 0, MSG_PRI_NORMAL);
			ap_video_record_sts_set(VIDEO_RECORD_MD);
			#if (C_BLIND_SPOT_DETECTION==CUSTOM_OFF)		
			ap_state_handling_icon_show_cmd(ICON_RECORD_MOTION_DETECT, 0, 0, 0);
			#endif
		}
		#endif

		return STATUS_OK;
	}
}


#if C_MOTION_DETECTION
void ap_video_record_md_tick(INT8U *md_tick)
{
	++ *md_tick;
	if(*md_tick > MOTION_DETECT_STOP_TIME)
	{
		*md_tick = 0;
		if(motion_detect_timerid != 0xFF)
		{
			drv_l2_sys_timer_kill(motion_detect_timerid);
			motion_detect_timerid = 0xFF;
		}

		if(ap_video_record_sts_get() & VIDEO_RECORD_MD)
		{
			if(video_record_sts & VIDEO_RECORD_BUSY)
			{
				ap_video_record_func_key_active();
			}
		}
	}
}

INT8S ap_video_record_md_active(INT8U *md_tick)
{
	*md_tick = 0;
	if(video_record_sts & VIDEO_RECORD_UNMOUNT)
	{
		return STATUS_FAIL;
	}

	if(!(video_record_sts & VIDEO_RECORD_BUSY)&&(video_record_sts & VIDEO_RECORD_MD))
	{
		ap_video_record_func_key_active();
		if(motion_detect_timerid == 0xFF)
		{
			motion_detect_timerid = VIDEO_PREVIEW_TIMER_ID;
			drv_l2_sys_timer_set((void *) msgQSend, (void *) ApQ, MSG_APQ_MOTION_DETECT_TICK, motion_detect_timerid, MOTION_DETECT_CHECK_TIME_INTERVAL);
		}
	}

	return STATUS_OK;
}

void ap_video_record_md_disable(void)
{
	if(motion_detect_timerid != 0xFF)
	{
		drv_l2_sys_timer_kill(motion_detect_timerid);
		motion_detect_timerid = 0xFF;
	}

	peripheral_task_msg_send(MSG_PERIPHERAL_TASK_MOTION_DETECT_STOP, NULL, 0, MSG_PRI_NORMAL);
	ap_video_record_sts_set(~VIDEO_RECORD_MD);
}
#endif //#if C_MOTION_DETECTION

void ap_video_record_exit(void)
{
    DBG_PRINT("%s:%d\r\n", __FUNCTION__, __LINE__);

}

void ap_video_record_sts_set(INT8S sts)
{
	if(sts > 0)
	{
		video_record_sts |= sts;
	}
	else
	{
		video_record_sts &= sts;
	}
}

extern INT16S SDC_ERR;
INT8S
ap_video_record_func_key_active(
	void
)
{
    INT32S temp;
	CHAR  *pdata;
	struct stat_t buf_tmp;

	if(SDC_ERR)
	{
		ap_state_handling_show_sd_err();

		DBG_PRINT("SDC error,please format\r\n");
		return STATUS_FAIL;
	}

	if(video_record_sts & VIDEO_RECORD_BUSY)
	{
		#if C_AUTO_DEL_FILE
		INT32U msg;
		#endif

		#if C_AUTO_DEL_FILE
		storage_task_msg_send_with_ack(MSG_STORAGE_SERVICE_AUTODEL_LOCK,5000);
		#endif

		ap_video_record_show_left_recording_time();

		ap_peripheral_auto_off_force_disable_set(0);
		if(video_encode_record_stop() != START_OK)
		{
			if(curr_file_info.file_handle >= 0)
			{
				close(curr_file_info.file_handle);
			}
			curr_file_info.file_handle = -1;

			if(curr_file_info.file_handle_b >= 0)
			{
				close(curr_file_info.file_handle_b);
				curr_file_info.file_handle_b = -1;
			}

			DBG_PRINT("Video Record Stop Fail\r\n");
		}
		else
		{
			if(g_lock_current_file_flag > 0)
			{
				#if C_RENAME_LOCK_FILE
				CHAR	temp_file_name[24];
				#endif

				_setfattr((CHAR *) curr_file_info.file_path_addr, D_RDONLY);
				if(curr_file_info.file_handle_b >= 0)
				{
					_setfattr((CHAR *) curr_file_info.file_path_addr_b, D_RDONLY);
				}

				g_lock_current_file_flag = 0;

				pdata = (CHAR *) curr_file_info.file_path_addr;
                temp = (*(pdata + 10) - 0x30)*1000;
                temp += (*(pdata + 11) - 0x30)*100;
                temp += (*(pdata + 12) - 0x30)*10;
                temp += (*(pdata + 13) - 0x30);
                if (temp < 10000) {
                    lock_file_table[temp >> 4] |= 1 << (temp & 0xF);
                    h264_file_table[temp >> 4] &= ~(1 << (temp & 0xF));
                    if(stat((CHAR *)curr_file_info.file_path_addr, &buf_tmp) == SUCCESS)
                    {
                        cur_lock_file_size += buf_tmp.st_size/(1024*1024);
                        DBG_PRINT("===lock file size = %lld\r\n", cur_lock_file_size);
                    }
                    else
                    {
                       DBG_PRINT("##########error test#######################\r\n");
                    }
                }

				#if C_RENAME_LOCK_FILE
				gp_memcpy((INT8S *) temp_file_name, (INT8S *) curr_file_info.file_path_addr, sizeof(temp_file_name));
				temp_file_name[6] = 'L';
				temp_file_name[7] = 'O';
				temp_file_name[8] = 'C';
				temp_file_name[9] = 'A';
				_rename((char *) curr_file_info.file_path_addr, temp_file_name);
				gp_memcpy((INT8S *) curr_file_info.file_path_addr, (INT8S *) temp_file_name, sizeof(temp_file_name));

				if(curr_file_info.file_handle_b >= 0)
				{
					gp_memcpy((INT8S *) temp_file_name, (INT8S *) curr_file_info.file_path_addr_b, sizeof(temp_file_name));
					temp_file_name[6] = 'L';
					temp_file_name[7] = 'O';
					temp_file_name[8] = 'C';
					temp_file_name[9] = 'B';
					_rename((char *) curr_file_info.file_path_addr_b, temp_file_name);
					gp_memcpy((INT8S *) curr_file_info.file_path_addr_b, (INT8S *) temp_file_name, sizeof(temp_file_name));
				}
				#endif // #if C_RENAME_LOCK_FILE

			}

			#if C_CYCLIC_VIDEO_RECORD
			if(next_file_info.file_handle > -1)
			{
				DBG_PRINT("Del %s\r\n",next_file_info.file_path_addr);
				close(next_file_info.file_handle);
				next_file_info.file_handle = -1;
				unlink((CHAR *) next_file_info.file_path_addr);
			}

			if(next_file_info.file_handle_b > -1)
			{
				DBG_PRINT("Del %s\r\n",next_file_info.file_path_addr_b);
				close(next_file_info.file_handle_b);
				next_file_info.file_handle_b = -1;
				unlink((CHAR *) next_file_info.file_path_addr_b);
			}
			#endif

		}

		UsrFlushBuffers(STORAGE_DEVICE);

		g_lock_current_file_flag = 0;
		g_cycle_record_continuing_flag = 0;

        ap_state_handling_icon_clear_cmd(ICON_RECORD_REC, ICON_RECORD_LOCKED, 0, 0);
        ap_peripheral_led_blink_set(1);
		ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
		storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
		DBG_PRINT("Video Record Stop\r\n");

		#if C_ASYN_DBLE_TFT
		tft_switch_stop_record_ACK();
		#endif
		
		return STATUS_OK;
	}
	else
	{
		if((video_record_sts & VIDEO_RECORD_UNMOUNT) == 0)
		{
			INT16U	temp;

			if(video_record_sts == 0 || video_record_sts == VIDEO_RECORD_MD)
			{
				ap_video_record_sts_set(VIDEO_RECORD_BUSY + VIDEO_RECORD_WAIT);
				ap_peripheral_auto_off_force_disable_set(1);
				ap_state_handling_warn_str_icon_exit();

				curr_file_info.file_handle = -1;
				curr_file_info.file_handle_b = -1;

				#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
				if(ap_state_config_md_get())
				{
					if(!(video_record_sts & VIDEO_RECORD_MD))
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

                temp = ap_state_config_record_time_get();
				#if C_GSENSOR_FUNCTION
				if((!ap_peripheral_get_usbd_connection_state())
					&& gsensor_power_on
				)
				{
					//temp = 0xff;
					DBG_PRINT("* record st = G-SENSOR\r\n");

					gsensor_power_on = 0;
					g_lock_current_file_flag = 1;
				}
				else
				#endif
				{
					DBG_PRINT("* record time = %d\r\n", temp);
				}

				temp <<= 8;
				temp |= ap_state_config_video_resolution_get();
				storage_task_msg_send(MSG_STORAGE_SERVICE_VID_H264_REQ, &temp, sizeof(INT16U), MSG_PRI_NORMAL);
				return STATUS_OK;
			}
		}
		else
		if(video_record_sts & VIDEO_RECORD_UNMOUNT)
		{
			#if (C_MOTION_DETECTION==CUSTOM_ON && C_BLIND_SPOT_DETECTION==CUSTOM_OFF)
			if(ap_state_config_md_get())
			{
				if(ap_video_record_sts_get() & VIDEO_RECORD_MD)
				{
					ap_video_record_md_disable();
					ap_state_config_md_set(0);
					ap_video_record_md_icon_clear_all();
				}
				else
				{
					peripheral_task_msg_send(MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, 0, MSG_PRI_NORMAL);
					ap_video_record_sts_set(VIDEO_RECORD_MD);
				}
			}
			#endif // C_MOTION_DETECTION

			ap_state_handling_show_insert_sdc();

			ap_peripheral_auto_off_force_disable_set(0);
		}
	}

	return STATUS_OK;
}

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
extern INT32S file_packer_set_file_handle(INT32U src);

void
ap_video_record_cycle_reply_action(
	STOR_SERV_FILEINFO *file_info_ptr
)
{
	ap_video_record_sts_set(~VIDEO_RECORD_WAIT);

	if(file_info_ptr->file_handle > -1)
	{
		gp_memcpy((INT8S *) &next_file_info, (INT8S *) file_info_ptr, sizeof(STOR_SERV_FILEINFO));
		if(__check_file_handle(file_info_ptr) == STATUS_OK)
		{
            INT32S temp;
            CHAR  *pdata;
            struct stat_t buf_tmp;

            file_packer_set_file_handle(PREVIEW_SRC_FRONT);
            file_packer_set_file_handle(PREVIEW_SRC_REAR);

            display_task_msg_send(MSG_DISPLAY_TASK_REC_TIME_DRAW, NULL, 0, MSG_PRI_NORMAL);

            record_start_time = xTaskGetTickCount();
            if(g_lock_current_file_flag > 0)
            {
                #if C_RENAME_LOCK_FILE
                CHAR temp_file_name[24];
                #endif

                _setfattr((CHAR *) curr_file_info.file_path_addr, D_RDONLY);
                if(curr_file_info.file_handle_b >= 0)
                {
                    _setfattr((CHAR *) curr_file_info.file_path_addr_b, D_RDONLY);
                }
                pdata = (CHAR *) curr_file_info.file_path_addr;
                temp = (*(pdata + 10) - 0x30)*1000;
                temp += (*(pdata + 11) - 0x30)*100;
                temp += (*(pdata + 12) - 0x30)*10;
                temp += (*(pdata + 13) - 0x30);
                if (temp < 10000) {
                    lock_file_table[temp >> 4] |= 1 << (temp & 0xF);
                    h264_file_table[temp >> 4] &= ~(1 << (temp & 0xF));
                    if(stat((CHAR *)curr_file_info.file_path_addr, &buf_tmp) == SUCCESS)
                    {
                        cur_lock_file_size += buf_tmp.st_size/(1024*1024);
                        DBG_PRINT("=lock file size = %lld\r\n", cur_lock_file_size);
                    }
                    else
                    {
                       DBG_PRINT("##########error test#######################\r\n");
                    }
                }
                #if C_RENAME_LOCK_FILE
                gp_memcpy((INT8S *) temp_file_name, (INT8S *) curr_file_info.file_path_addr, sizeof(temp_file_name));
                temp_file_name[6] = 'L';
                temp_file_name[7] = 'O';
                temp_file_name[8] = 'C';
                temp_file_name[9] = 'A';
                _rename((char *) curr_file_info.file_path_addr, temp_file_name);

                gp_memcpy((INT8S *) curr_file_info.file_path_addr, (INT8S *) temp_file_name, sizeof(temp_file_name));

                if(curr_file_info.file_handle_b >= 0)
                {
                    gp_memcpy((INT8S *) temp_file_name, (INT8S *) curr_file_info.file_path_addr_b, sizeof(temp_file_name));
                    temp_file_name[6] = 'L';
                    temp_file_name[7] = 'O';
                    temp_file_name[8] = 'C';
                    temp_file_name[9] = 'B';
                    _rename((char *) curr_file_info.file_path_addr_b, temp_file_name);
                    gp_memcpy((INT8S *) curr_file_info.file_path_addr_b, (INT8S *) temp_file_name, sizeof(temp_file_name));
                }
                #endif // #if C_RENAME_LOCK_FILE

            }

            g_lock_current_file_flag--;
            if(g_lock_current_file_flag <= 0)
            {
                g_lock_current_file_flag = 0;
                ap_state_handling_icon_clear_cmd(ICON_RECORD_LOCKED, 0, 0, 0);
            }

            gp_memcpy((INT8S *) g_cycle_prev_file_path, (INT8S *) curr_file_info.file_path_addr, 24);
            gp_memcpy((INT8S *) g_cycle_prev_file_path_b, (INT8S *) curr_file_info.file_path_addr_b, 24);

            gp_memcpy((INT8S *) curr_file_info.file_path_addr, (INT8S *) next_file_info.file_path_addr, 24);
            gp_memcpy((INT8S *) curr_file_info.file_path_addr_b, (INT8S *) next_file_info.file_path_addr_b, 24);

            curr_file_info.file_handle = next_file_info.file_handle;
            curr_file_info.file_handle_b = next_file_info.file_handle_b;

            g_cycle_record_continuing_flag = 1;
            next_file_info.file_handle = -1;
            next_file_info.file_handle_b = -1;

            ap_state_handling_icon_show_cmd(ICON_RECORD_REC, 0, 0, 0);

            #if C_AUTO_DEL_FILE
            if(ap_state_config_record_time_get())
            {
                storage_task_msg_send_with_ack(MSG_STORAGE_SERVICE_AUTODEL_UNLOCK,5000);
            }
            #endif

			return;
		}

		next_file_info.file_handle = -1;
		next_file_info.file_handle_b = -1;
	}

    DBG_PRINT("Cyclic video record open file Fail\r\n");
    if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
    {
        ap_video_record_func_key_active();
    }

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

	ap_peripheral_auto_off_force_disable_set(0);
    ap_video_record_clear_left_recording_time();
    ap_state_handling_show_sd_full();

	storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
}

#endif //#if C_CYCLIC_VIDEO_RECORD

INT8U ap_video_record_sts_get(void)
{
	return video_record_sts;
}

void
ap_video_record_reply_action(
	STOR_SERV_FILEINFO *file_info_ptr
)
{
	ap_video_record_sts_set(~VIDEO_RECORD_WAIT);

	if(file_info_ptr)
	{
		gp_memcpy((INT8S *) &curr_file_info, (INT8S *) file_info_ptr, sizeof(STOR_SERV_FILEINFO));
	}

	if(__check_file_handle(file_info_ptr) == STATUS_OK)
	{
		if(video_encode_record_start() != START_OK)
		{
			ap_video_record_show_left_recording_time();
			ap_state_handling_icon_clear_cmd(ICON_RECORD_REC, 0, 0, 0);
			ap_peripheral_auto_off_force_disable_set(0);

			close(curr_file_info.file_handle);
			curr_file_info.file_handle = -1;

			if(curr_file_info.file_handle_b >= 0)
			{
				close(curr_file_info.file_handle_b);
				curr_file_info.file_handle_b = -1;
			}

			ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
			storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
			DBG_PRINT("Video Record Fail\r\n");
		}
		else
		{
			display_task_msg_send(MSG_DISPLAY_TASK_REC_TIME_DRAW, NULL, 0, MSG_PRI_NORMAL);
			ap_state_handling_icon_show_cmd(ICON_RECORD_REC, 0, 0, 0);

			record_start_time = xTaskGetTickCount();

			#if C_AUTO_DEL_FILE
			if(ap_state_config_record_time_get())
			{
				storage_task_msg_send_with_ack(MSG_STORAGE_SERVICE_AUTODEL_UNLOCK, 5000);
			}
			#endif
            ap_peripheral_led_blink_set(2);
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

		ap_state_handling_show_sd_full();
		ap_video_record_clear_left_recording_time();
		ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
		ap_peripheral_auto_off_force_disable_set(0);

		storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
	}
}

void ap_video_record_lock_current_file(void)
{
	INT8U	record_config = ap_state_config_record_minute_get();
	INT32U	recording_time;
    INT32S temp1;
	CHAR  *pdata;
	struct stat_t buf_tmp;

	recording_time = (xTaskGetTickCount() - record_start_time) / 1000;
#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
	if((recording_time < 10) && g_cycle_record_continuing_flag)
	{
		#if C_RENAME_LOCK_FILE
		CHAR temp_file_name[24];
		#endif

		_setfattr((CHAR *) g_cycle_prev_file_path, D_RDONLY);
		if(curr_file_info.file_handle_b >= 0)
		{
			_setfattr((CHAR *) g_cycle_prev_file_path_b, D_RDONLY);
		}
        pdata = (CHAR *) g_cycle_prev_file_path;
        if(*(pdata + 6) != 'L')
        {
            temp1 = (*(pdata + 10) - 0x30)*1000;
            temp1 += (*(pdata + 11) - 0x30)*100;
            temp1 += (*(pdata + 12) - 0x30)*10;
            temp1 += (*(pdata + 13) - 0x30);
            if (temp1 < 10000) {
                lock_file_table[temp1 >> 4] |= 1 << (temp1 & 0xF);
                h264_file_table[temp1 >> 4] &= ~(1 << (temp1 & 0xF));
                if(stat(g_cycle_prev_file_path, &buf_tmp) == SUCCESS)
                {
                    cur_lock_file_size += buf_tmp.st_size/(1024*1024);
                    DBG_PRINT("==lock file size = %lld\r\n", cur_lock_file_size);
                }
                else
                {
                    DBG_PRINT("##########error test#######################\r\n");
                }
            }
        }else
        {
            DBG_PRINT("prev file had locked.\r\n");
        }

		#if C_RENAME_LOCK_FILE
		gp_memcpy((INT8S *) temp_file_name, (INT8S *) g_cycle_prev_file_path, sizeof(temp_file_name));
		temp_file_name[6] = 'L';
		temp_file_name[7] = 'O';
		temp_file_name[8] = 'C';
		temp_file_name[9] = 'A';
		_rename((char *) g_cycle_prev_file_path, temp_file_name);
		gp_memcpy((INT8S *) g_cycle_prev_file_path, (INT8S *) temp_file_name, sizeof(temp_file_name));

		if(curr_file_info.file_handle_b >= 0)
		{
			gp_memcpy((INT8S *) temp_file_name, (INT8S *) g_cycle_prev_file_path_b, sizeof(temp_file_name));
			temp_file_name[6] = 'L';
			temp_file_name[7] = 'O';
			temp_file_name[8] = 'C';
			temp_file_name[9] = 'B';
			_rename((char *) g_cycle_prev_file_path_b, temp_file_name);
			gp_memcpy((INT8S *) g_cycle_prev_file_path_b, (INT8S *) temp_file_name, sizeof(temp_file_name));
		}
		#endif // C_RENAME_LOCK_FILE
	}
#endif
	if(!record_config)
	{
		g_lock_current_file_flag = 1;
		return;
	}

	if(recording_time > (record_config * 60 - 10))
	{
		g_lock_current_file_flag = 2;
	}
	else
	{
		g_lock_current_file_flag = 1;	//set file attribute only after close file
	}
	DBG_PRINT("g_lock_current_file_flag = %d\r\n",g_lock_current_file_flag);
}

#if C_SUPORT_GET_JPG_BUF
static INT32S
__check_get_jpeg_file_handle(
	STOR_SERV_FILEINFO* file_info
)
{
	INT32S nRet = STATUS_OK;

	VID_INFO*  pVid_info_front = &pStreamArgs->vid_info_front;
	GET_JPEG_ARGS* pGet_jpeg_front = &pVid_info_front->get_jpeg_args;

	VID_INFO*  pVid_info_rear = &pStreamArgs->vid_info_rear;
	GET_JPEG_ARGS* pGet_jpeg_rear = &pVid_info_rear->get_jpeg_args;

	if(pVid_info_front->data_source != VID_SRC_NONE)
	{
		if(file_info->file_handle < 0)
		{
			ERR_PRINT("[Front] file_handle is less than 0 \r\n");
			RETURN(STATUS_FAIL);
		}

		pGet_jpeg_front->w_file_handle = file_info->file_handle;
		pGet_jpeg_front->w_file_name = file_info->file_path_addr;
		DBG_PRINT("[Front]w_file_handle=%d\r\n",file_info->file_handle);
	}

	if((pVid_info_rear->data_source != VID_SRC_NONE)&&(back_sensor_plug_status_get()))
	{
		if(file_info->file_handle_b < 0)
		{
			ERR_PRINT("[Rear] file_handle is less than 0 \r\n");
			RETURN(STATUS_FAIL);
		}

		pGet_jpeg_rear->w_file_handle = file_info->file_handle_b;
		pGet_jpeg_rear->w_file_name = file_info->file_path_addr_b;
		DBG_PRINT("[Rear]w_file_handle=%d\r\n",file_info->file_handle_b);
	}

Return:
	return nRet;
}

void
ap_video_record_get_jpeg(
	STOR_SERV_FILEINFO *file_info_ptr
)
{
	if(__check_get_jpeg_file_handle(file_info_ptr) == STATUS_OK)
	{
		if(video_encode_get_jpeg() == STATUS_OK)
		{
			return;
		}
	}


	if(file_info_ptr->file_handle > -1)
	{
		close(file_info_ptr->file_handle);
	}

	if(file_info_ptr->file_handle_b > -1)
	{
		close(file_info_ptr->file_handle_b);
	}
}

void
ap_video_record_get_jpeg_write_file(
	INT8U src,
	SNAPSHOT_INFO *snapshot_info_ptr
)
{
	if((snapshot_info_ptr->file_w_buf != 0) && (snapshot_info_ptr->file_w_size != 0))
	{
		write(snapshot_info_ptr->file_handle, snapshot_info_ptr->file_w_buf, snapshot_info_ptr->file_w_size);
	}

	close(snapshot_info_ptr->file_handle);
	
	if(snapshot_info_ptr->file_w_size == 0)
	{
		unlink((CHAR *) snapshot_info_ptr->file_name);
	}

	UsrFlushBuffers(STORAGE_DEVICE);

	if(snapshot_info_ptr->file_w_buf != 0)
	{
		video_encode_release_get_jpeg_buf(src);
	}
}
#endif


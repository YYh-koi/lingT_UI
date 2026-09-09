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
INT8U Get_1st_decode_frm = 0;  // 1: get 1st frame 0: get none 1st frame
							  //h264解超過2048,mb2scan需要分2次做,有這個變數是因為rotate,在解完frame ,
							  //pscaler中斷會發一次到copy&draw ( 會作一次rotate)
	                          // 之後會把這個frame的檔名給畫上去,在送一次到copy&draw , 會造成rotate in == rotate out
	                          // 所以這個變數=1時 , 第一次pscaler中斷不會做copy&draw
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT32S		browse_sts = BROWSE_IDLE;
STOR_SERV_PLAYINFO	browse_curr_file;
static INT16U		browse_total_file_num;
static TIME_T		browse_file_date_time;
static INT8U		browse_display_timerid = 0xFF;
static  INT16U 		wifi_playback_file_index = 0;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/
extern INT8U	gPower_down;
extern INT8S	browser_play_speed;
#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
extern INT32U audio_background_image_ptr;
extern INT32U audio_background_image_size;
#endif
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
ap_browse_sts_set(
	INT32S sts
)
{
	if(sts > 0)
	{
		browse_sts |= sts;
	}
	else
	{
		browse_sts &= sts;
	}
}

INT32S
ap_browse_sts_get(
	void
)
{
	return browse_sts;
}

INT32S
ap_browse_open_file_req(
	void* fileIdx
)
{
	if(browse_sts & BROWSE_OPEN_FILE_NOW)
	{
		DBG_PRINT("Not receiving storage task's reply after sending MSG_STORAGE_SERVICE_BROWSE_REQ\r\n");
		return STATUS_FAIL;
	}

	storage_task_msg_send(MSG_STORAGE_SERVICE_BROWSE_REQ, fileIdx, sizeof(INT32U), MSG_PRI_NORMAL);
	browse_sts |= BROWSE_OPEN_FILE_NOW;

	return STATUS_OK;
}

void
ap_browse_init(
	INT32U prev_state,
	INT16U play_index
)
{
	INT32U	search_type_with_index, search_type;
	STOR_SERV_PLAYINFO	play_info;
	STOR_SERV_PLAYINFO	*info_ptr = &play_info;

	browse_sts = 0;

	if(prev_state == STATE_SETTING)
	{
		if(play_index == 0x5A5A)
		{
			search_type = STOR_SERV_SEARCH_PREV;	//Daniel modified for returning to previous one after deleting the current one
		}
		else
		{
			search_type = STOR_SERV_SEARCH_ORIGIN;
		}
	}
	else
	if(prev_state == STATE_BROWSE)
	{
		search_type = STOR_SERV_SEARCH_ORIGIN;
	}
	else
	{
		search_type = STOR_SERV_SEARCH_INIT;
	}

	display_task_msg_send(MSG_DISPLAY_TASK_EFFECT_INIT, NULL, 0, MSG_PRI_NORMAL);
    display_task_msg_send(MSG_DISPLAY_RTC_DATE_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);

    if(ap_peripheral_get_usbd_io_status())
    {
        ap_state_handling_icon_show_cmd(ICON_BATTERY_CHARGED, 0, 0, 0);
    }
    else
    {
        ap_state_handling_icon_clear_cmd(ICON_BATTERY_CHARGED, 0, 0, 0);
    }

    ap_state_handling_icon_show_cmd(ICON_PLAYBACK, 0, 0, 0);

	if(vid_dec_entry() < 0)
	{
		DBG_PRINT("Failed to init motion jpeg task\r\n");
	}

	#if C_BATTERY_DETECT
	ap_state_handling_current_bat_lvl_show();
	ap_state_handling_current_charge_icon_show();
	#endif

	if(ap_state_handling_storage_id_get() != STORAGE_DEVICE)
	{
		ap_browse_sts_set(BROWSE_SD_NOT_EXIST);
	}
	else
	{
		ap_browse_sts_set(~BROWSE_SD_NOT_EXIST);
	}

	#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
    if((state_wifi_status_get() & FLAG_CONNECT) &&
		(state_wifi_status_get() & FLAG_OFF_DISPLAY)
	)
	{
		// RTSP don't get first, so not need to open file
   		return;
   	}
	#endif

	if(prev_state == STATE_THUMBNAIL)
	{
		search_type_with_index = STOR_SERV_SEARCH_GIVEN | (play_index << 16);
		ap_browse_open_file_req((void *) &search_type_with_index);
	}
	else
	{
		if(ap_state_handling_storage_id_get() == STORAGE_DEVICE)
		{
			// STOR_SERV_SEARCH_INIT
			ap_browse_open_file_req((void *) &search_type);
		}
		else
		{
            info_ptr->err_flag = STOR_SERV_NO_MEDIA;
            app_msg_send(MSG_STORAGE_SERVICE_BROWSE_REPLY, info_ptr, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL);
		}
	}
}

void
ap_browse_string_icon_clear(
	void
)
{
	browser_play_speed = 0;
	ap_browse_fast_play_icon_show(browser_play_speed);
	ap_state_handling_warn_str_icon_exit();
	ap_video_record_clear_park_mode();
}

void
ap_browse_exit(
	INT32U next_state_msg
)
{
	#if 0 // To fix it
	display_task_msg_send(MSG_DISPLAY_TASK_PLAY_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
	display_task_msg_send(MSG_DISPLAY_FILE_DATE_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
	ap_browse_string_icon_clear();
	#endif

	//DBG_PRINT("browse_sts=0x%d\r\n", browse_sts);

	if(browse_sts == BROWSE_PLAYBACK_BUSY)
	{
		if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
		{
			audio_wav_pause();
			storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
		}
		else
		if(vid_dec_pause() == STATUS_OK)
		{
			storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
		}
	}

	if(browse_sts & BROWSE_PLAYBACK_BUSY)
	{
		if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
		{
            ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PAUSE, 0, 0, 0);
			ap_state_handling_icon_show_cmd(ICON_PLAYBACK_PLAY, 0, 0, 0);
			ap_browse_wav_stop(NULL);
		}
		else
		{
			//DBG_PRINT("mjpeg dec end\r\n");

			ap_browse_mjpeg_stop();
			ap_browse_mjpeg_play_end();
		}

		if(browse_curr_file.file_handle > -1)
		{
			close(browse_curr_file.file_handle);
			browse_curr_file.file_handle = -1;
		}
		osDelay(10);
	}

	if((next_state_msg == MSG_APQ_MODE) || (next_state_msg == MSG_APQ_POWER_KEY_ACTIVE))
	{
        ap_state_handling_icon_clear_cmd(ICON_PLAYBACK, ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, ICON_PLAYBACK_MOVIE);
	}
	else
	if(next_state_msg == MSG_APQ_CONNECT_TO_PC)
	{
        ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, ICON_PLAYBACK_MOVIE, 0);
	}

	vid_dec_exit();
#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
    if(audio_background_image_ptr!=0)
	{
        gp_free((void *)audio_background_image_ptr);
        audio_background_image_ptr = 0;
	}
#endif
	browse_sts = 0;
}

INT8U
ap_browse_get_curr_file_type(
	void
)
{
	return browse_curr_file.file_type;
}

void
ap_browse_mjpeg_stop(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	ap_browse_sts_set(~(BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE));
	ap_peripheral_auto_off_force_disable_set(0);
    display_task_msg_send(MSG_DISPLAY_TASK_PLAY_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
	vid_dec_stop();
	vid_dec_parser_stop();
}

void
ap_browse_mjpeg_play_end(
	void
)
{
	browser_play_speed = 0;

	ap_browse_fast_play_icon_show(browser_play_speed);
    ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, 0, 0);
	ap_browse_volume_icon_clear_all();
	if(browse_curr_file.file_type != TK_IMAGE_TYPE_JPEG)
	{
		ap_state_handling_icon_show_cmd(ICON_PLAYBACK_PLAY, 0, 0, 0);
	}
}

void
ap_browse_volume_icon_clear_all(
	void
)
{
    ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_VOLUME_WHITE1, ICON_PLAYBACK_VOLUME_WHITE2, ICON_PLAYBACK_VOLUME_WHITE3, ICON_PLAYBACK_VOLUME_WHITE4);
    ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_VOLUME_WHITE5, ICON_PLAYBACK_VOLUME_WHITE6, ICON_PLAYBACK_VOLUME_WHITE7, ICON_PLAYBACK_VOLUME_WHITE8);
    ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_VOLUME_RED1, ICON_PLAYBACK_VOLUME_RED2, ICON_PLAYBACK_VOLUME_RED3, ICON_PLAYBACK_VOLUME_RED4);
    ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_VOLUME_RED5, ICON_PLAYBACK_VOLUME_RED6, ICON_PLAYBACK_VOLUME_RED7, ICON_PLAYBACK_VOLUME_RED8);
    ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_VOLUME_TITLE, 0, 0, 0);
}

void
ap_browse_volume_icon_show(
	INT8U vol
)
{
	INT8U   i, volume_icon_show_time;

	ap_browse_volume_icon_clear_all();
    ap_state_handling_icon_show_cmd(ICON_PLAYBACK_VOLUME_TITLE, 0, 0, 0);

    for(i = 0; i < vol; i++)
    {
        ap_state_handling_icon_show_cmd(ICON_PLAYBACK_VOLUME_RED1 + i, 0, 0, 0);
    }

    for(i = vol; i < 8; i++)
    {
        ap_state_handling_icon_show_cmd(ICON_PLAYBACK_VOLUME_WHITE1 + i, 0, 0, 0);
    }

	volume_icon_show_time = 200;
	peripheral_task_msg_send(MSG_PERIPHERAL_TASK_TIMER_VOLUME_ICON_SHOW, &volume_icon_show_time, sizeof(INT8U), MSG_PRI_NORMAL);
}

void
ap_browse_volume_up_down(
	INT8U direction
)
{
	INT8U	vol;

	vol = ap_state_config_volume_get();
	if(direction)
	{						//vol down
		if(vol > 0)
		{
			vol--;
		}
	}
	else
	{						//vol up
		if(vol < MAX_VOLUME_LEVEL)
		{
			vol++;
		}
	}

	DBG_PRINT("vol = %d \r\n", vol);
	ap_browse_volume_icon_show(vol);
	audio_vol_set(vol);
	ap_state_config_volume_set(vol);
}

void
ap_browse_fast_play_icon_show(
	INT8S play_speed
)
{
    DISPLAY_ICONSHOW    *str_fast_icon;
    const CHAR *ptr = NULL;

	if(play_speed)
	{
		if(play_speed > 0)
		{
			if(play_speed == 3)
			{
				ptr = "8X";
			}
			else
			if(play_speed == 2)
			{
				ptr = "4X";
			}
			else
			if(play_speed == 1)
			{
				ptr = "2X";
			}
		}
		else
		if(play_speed < 0)
		{
			if(play_speed == -1)
			{
				ptr = "-2X";
			}
			else
			if(play_speed == -2)
			{
				ptr = "-4X";
			}
			else
			if(play_speed == -3)
			{
				ptr = "-8X";
			}
		}
	}
	else
	{
        display_task_msg_send(MSG_DISPLAY_TASK_PARK_ICON_CLEAR, NULL, 0, MSG_PRI_NORMAL);
        return;
	}

    str_fast_icon = allocate_and_draw_ascii_string_icon(ptr, WHITE_COLOR);
    if(str_fast_icon)
    {
        str_fast_icon->pos_x = 100;
        str_fast_icon->pos_y = -80;
        display_task_msg_send(MSG_DISPLAY_TASK_PARK_ICON_DRAW, &str_fast_icon, sizeof(void *), MSG_PRI_NORMAL);
    }
}

void
ap_browse_next_key_active(
	INT8U err_flag
)
{
	INT32U	search_type = STOR_SERV_SEARCH_NEXT;

	if(err_flag)
	{
		search_type |= (err_flag << 8);
	}

	if(!(browse_sts & BROWSE_PLAYBACK_PAUSE))
	{
		if(browse_sts & BROWSE_PLAYBACK_BUSY)
		{
			ap_browse_volume_up_down(0);
			return;
		}
	}

	if((browse_sts & BROWSE_SD_NOT_EXIST) || (browse_total_file_num == 1) || (browse_total_file_num == 0))
	{
		if(browse_sts & BROWSE_PLAYBACK_BUSY)
		{
			if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
			{
                ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, 0, 0);
                ap_state_handling_icon_show_cmd(ICON_PLAYBACK_PLAY, 0, 0, 0);
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

			search_type = STOR_SERV_SEARCH_ORIGIN;
			ap_browse_open_file_req((void *) &search_type);
		}

		if(!err_flag)
		{
			audio_effect_play(VOICE_ID_CLICK);
		}

		return;
	}

	if(browse_sts & (BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE))
	{
		if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
		{
			ap_browse_wav_stop(NULL);
		}
		else
		{
			ap_browse_mjpeg_stop();
			browser_play_speed = 0;
			ap_browse_fast_play_icon_show(browser_play_speed);
			ap_browse_volume_icon_clear_all();

			if(browse_curr_file.file_handle > -1)
			{
				close(browse_curr_file.file_handle);
				browse_curr_file.file_handle = -1;
			}
		}
	}

	ap_browse_open_file_req((void *) &search_type);
	if(!err_flag)
	{
		audio_effect_play(VOICE_ID_CLICK);
	}
}

void
ap_browse_prev_key_active(
	INT8U err_flag
)
{
	INT32U	search_type = STOR_SERV_SEARCH_PREV;

	if(err_flag)
	{
		search_type |= (err_flag << 8);
	}

	if(!(browse_sts & BROWSE_PLAYBACK_PAUSE))
	{
		if(browse_sts & BROWSE_PLAYBACK_BUSY)
		{
			ap_browse_volume_up_down(1);
			return;
		}
	}

	if((browse_sts & BROWSE_SD_NOT_EXIST) || (browse_total_file_num == 1) || (browse_total_file_num == 0))
	{
		if(browse_sts & BROWSE_PLAYBACK_BUSY)
		{
			if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
			{
                ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, 0, 0);
                ap_state_handling_icon_show_cmd(ICON_PLAYBACK_PLAY, 0, 0, 0);
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

			search_type = STOR_SERV_SEARCH_ORIGIN;
			ap_browse_open_file_req((void *) &search_type);
		}

		if(!err_flag)
		{
			audio_effect_play(VOICE_ID_CLICK);
		}

		return;
	}

	if(browse_sts & (BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE))
	{
		if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
		{
			ap_browse_wav_stop(NULL);
		}
		else
		{
			ap_browse_mjpeg_stop();
			browser_play_speed = 0;
			ap_browse_fast_play_icon_show(browser_play_speed);
			ap_browse_volume_icon_clear_all();

			if(browse_curr_file.file_handle > -1)
			{
				close(browse_curr_file.file_handle);
				browse_curr_file.file_handle = -1;
			}
		}
	}

	ap_browse_open_file_req((void *) &search_type);
	if(!err_flag)
	{
		audio_effect_play(VOICE_ID_CLICK);
	}
}

void
ap_browse_get_file_date_time(
	TIME_T *osd_time
)
{
	osd_time->tm_year = browse_file_date_time.tm_year;
	osd_time->tm_mon = browse_file_date_time.tm_mon;
	osd_time->tm_mday = browse_file_date_time.tm_mday;
	osd_time->tm_hour = browse_file_date_time.tm_hour;
	osd_time->tm_min = browse_file_date_time.tm_min;
	osd_time->tm_sec = browse_file_date_time.tm_sec;
}

void
ap_browse_decode_file_date_time(
	INT32U dos_date_time,
	INT16U dos_time_ms
)
{
	INT16U	dos_date, dos_time;
	INT16U	year;
	INT8U	month, day;
	INT8U	hour, minute, second;

	dos_date = dos_date_time & 0xffff;
	dos_time = (dos_date_time >> 16) & 0xffff;
	dosdate_decode(dos_date, &year, &month, &day);
	dostime_decode(dos_time, &hour, &minute, &second);
	browse_file_date_time.tm_year = year;
	browse_file_date_time.tm_mon = month;
	browse_file_date_time.tm_mday = day;
	browse_file_date_time.tm_hour = hour;
	browse_file_date_time.tm_min = minute;
	if(dos_time_ms)
	{
        DBG_PRINT("dos_time_ms = %d,second = %d\r\n",dos_time_ms,second);
        second += 1;
    }
	browse_file_date_time.tm_sec = second;

	DBG_PRINT("year = %d, ", browse_file_date_time.tm_year);
	DBG_PRINT("mon = %d, ", browse_file_date_time.tm_mon);
	DBG_PRINT("day = %d, ", browse_file_date_time.tm_mday);
	DBG_PRINT("hour = %d, ", browse_file_date_time.tm_hour);
	DBG_PRINT("min = %d, ", browse_file_date_time.tm_min);
	DBG_PRINT("sec = %d\r\n", browse_file_date_time.tm_sec);
}

void
ap_browse_decode_file_date_time_1(
	INT32U dos_date_time,
	INT16U dos_time_ms,
	TIME_T *time_info
)
{
	INT16U dos_date,dos_time;
	INT16U year;
	INT8U month,day;
	INT8U hour,minute,second;

	dos_date = dos_date_time&0xffff;
	dos_time = (dos_date_time>>16)&0xffff;
	dosdate_decode(dos_date, &year, &month, &day);
	dostime_decode(dos_time, &hour, &minute, &second);
	time_info->tm_year = year;
	time_info->tm_mon = month;
	time_info->tm_mday = day;
	time_info->tm_hour = hour;
	time_info->tm_min = minute;
	if(dos_time_ms)
	second += 1;
	time_info->tm_sec = second;
}

void
ap_browse_resolution_string_draw(
	STOR_SERV_PLAYINFO *info_ptr,
	INT16U wWidth,
	INT16U wHeight
)
{
	const struct
	{
		INT16U	w;
		INT16U	h;
		CHAR*	str;
	} resv_str[] =
	{
		{VID_REC_WIDTH_QVGA,	VID_REC_HEIGHT_QVGA,	"QVGA"},
		{VID_REC_WIDTH_VGA,		VID_REC_HEIGHT_VGA,		"VGA"},
		{VID_REC_WIDTH_D1,		VID_REC_HEIGHT_D1,		"D1"},
		{VID_REC_WIDTH_WVGA,	VID_REC_HEIGHT_WVGA,	"WVGA"},
		{VID_REC_WIDTH_720P,	VID_REC_HEIGHT_720P,	"720P"},
		{VID_REC_WIDTH_1080P,	VID_REC_HEIGHT_1080P,	"1080P"},
		{VID_REC_WIDTH_1080FHD,	VID_REC_HEIGHT_1080FHD,	"1080FHD"},
		{VID_REC_WIDTH_2K,	VID_REC_HEIGHT_2K,	"2K"},
		{VID_REC_WIDTH_4K,	VID_REC_HEIGHT_4K,	"4K"},
		{1280,	480,	"480P"},
		{1280,	960,	"1.3M"},
		{2048,	1536,	"3M"},
		{2592,	1944,	"5M"},
		{3264,	2448,	"8M"},
		{3648,	2736,	"10M"},
		{4032,	3024,	"12M"},
	};

    DISPLAY_ICONSHOW    *str_res_icon;
    INT32S i;
    const CHAR *ptr = NULL;
  	CHAR  str_tmp[10];
    struct stat_t buf_tmp;
	if(info_ptr->file_type != TK_IMAGE_TYPE_JPEG)
	{
		ap_state_handling_icon_show_cmd(ICON_PLAYBACK_MOVIE, ICON_PLAYBACK_PLAY, 0, 0);
        stat((CHAR *) info_ptr->file_path_addr, &buf_tmp);
        if(buf_tmp.st_mode & D_RDONLY)
        {
            ap_state_handling_icon_show_cmd(ICON_PLAYBACK_LOCKED, 0, 0, 0);
        }
        else
        {
            ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_LOCKED, 0, 0, 0);
        }
	}
	else
	{
        ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_LOCKED, 0, 0, 0);
	}

	for(i = 0 ; i < sizeof(resv_str) / sizeof(*resv_str) ; ++i)
	{
		if(wWidth == resv_str[i].w && wHeight== resv_str[i].h)
		{
			ptr = resv_str[i].str;
			break;
		}
	}

	if(info_ptr->file_type == TK_IMAGE_TYPE_JPEG
		&& wHeight == VID_REC_HEIGHT_1080FHD
		&& wWidth == VID_REC_WIDTH_1080FHD
	)
	{
		ptr = "2MHD";
	}

	if(!ptr)
	{
		snprintf(str_tmp, sizeof(str_tmp), "%dx%d", wWidth, wHeight);
		str_tmp[sizeof(str_tmp) - 1] = '\0';
		ptr = &str_tmp[0];
	}
	//DBG_PRINT("\r\n (%s )",ptr );
    str_res_icon = allocate_and_draw_ascii_string_icon(ptr, WHITE_COLOR);
    if(str_res_icon)
    {
        str_res_icon->pos_x = BROWSE_RESV_POS_X;
        str_res_icon->pos_y = BROWSE_RESV_POS_Y;
        display_task_msg_send(MSG_DISPLAY_TASK_RES_ICON_DRAW, &str_res_icon, sizeof(void *), MSG_PRI_NORMAL);
    }
}

void
ap_browse_show_file_name(
	STOR_SERV_PLAYINFO *info_ptr,
	INT8U enable,
	INT32U frame
)
{
	INT32U buf = frame;
	INT16U tft_w = drv_l2_display_tft_width_get();
	INT16U tft_h = drv_l2_display_tft_height_get();

	if(enable)
	{
		STRING_ASCII_INFO ascii_str = {0};
		INT16S pos_offset_x, pos_offset_y;
		INT16U pos_x, pos_y;

		ascii_str.str_ptr = (CHAR *)info_ptr->file_path_addr;
		//if(info_ptr->file_type != TK_IMAGE_TYPE_WAV)
		{
			ascii_str.str_ptr += 6;
		}

		ascii_str.font_color = BROWSE_FILE_NAME_COLOR;
		ascii_str.font_type = 0;
		ascii_str.buff_w = tft_w;
		ascii_str.buff_h = tft_h;

		pos_offset_x = BROWSE_FILE_NAME_POS_X;
		if(pos_offset_x < 0)
		{
			pos_x = pos_offset_x + tft_w;
		}
		else
		{
			pos_x = pos_offset_x;
		}

		pos_offset_y = BROWSE_FILE_NAME_POS_Y;
		if(pos_offset_y < 0)
		{
			pos_y = pos_offset_y + tft_h;
		}
		else
		{
			pos_y = pos_offset_y;
		}

		ascii_str.pos_x = pos_x & ~0x1;
		ascii_str.pos_y = pos_y & ~0x1;

		ap_state_resource_string_ascii_draw((INT16U *)buf, &ascii_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
	}
	else
	{
		IMAGE_DECODE_STRUCT img_info;
		INT32S logo_img_ptr;
		INT32U size;
		INT16U logo_fd;

		logo_fd = nv_open((INT8U *) "POWER_OFF_LOGO.JPG");
		if(logo_fd != 0xFFFF)
		{
			size = nv_rs_size_get(logo_fd);
			logo_img_ptr = (INT32S) gp_malloc(size);
			if(!logo_img_ptr)
			{
				DBG_PRINT("State browser allocate jpeg input buffer fail.[%d]\r\n", size);
				return;
			}

			if(nv_read(logo_fd, (INT32U) logo_img_ptr, size))
			{
				DBG_PRINT("Failed to read resource_header in ap_browse_reply_action()\r\n");
				gp_free((void *) logo_img_ptr);
				return;
			}

			img_info.image_source = (INT32S) logo_img_ptr;
			img_info.source_size = size;
			img_info.source_type = TK_IMAGE_SOURCE_TYPE_BUFFER;

			if(drv_l2_display_tft_fmt_get() == DISP_FMT_RGB565)
			{
				img_info.output_format = C_SCALER_CTRL_OUT_RGB565;
#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
				if(audio_background_image_ptr == 0)
				{
                    audio_background_image_ptr =    (INT32U) gp_malloc(tft_w*tft_h<<2);
                    audio_background_image_size = tft_w*tft_h<<2;
                }
#endif
			}
			else if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
			{
				img_info.output_format = C_SCALER_CTRL_OUT_GP420;
#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
                if(audio_background_image_ptr == 0)
				{
                    audio_background_image_ptr =    (INT32U) gp_malloc(tft_w*tft_h*3>>1);
                    audio_background_image_size = tft_w*tft_h*3>>1;
                }
#endif
			}
			else
			{
				img_info.output_format = C_SCALER_CTRL_OUT_YUYV;
#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
                if(audio_background_image_ptr == 0)
				{
                    audio_background_image_ptr =    (INT32U) gp_malloc(tft_w*tft_h<<2);
                    audio_background_image_size = tft_w*tft_h<<2;
                }
#endif
			}

			img_info.output_ratio = 0;
			img_info.out_of_boundary_color = 0x008080;
			img_info.output_buffer_width = tft_w;
			img_info.output_buffer_height = tft_h;
			img_info.output_image_width = tft_w;
			img_info.output_image_height = tft_h;
			img_info.output_buffer_pointer = buf;
			if(jpeg_buffer_decode_and_scale(&img_info, 0, 0) == STATUS_FAIL)
			{
				gp_free((void *) logo_img_ptr);
				DBG_PRINT("State browser decode AUDIO_REC_BG.jpeg file fail.\r\n");
				return;
			}
#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
			gp_memcpy((INT8S *)audio_background_image_ptr,(INT8S *)buf,audio_background_image_size);
#endif
			gp_free((void *) logo_img_ptr);
		}
	}
}

INT32S
ap_browse_wav_play(
	IMAGE_DECODE_STRUCT *img_ptr
)
{
	storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_STOP, NULL, 0, MSG_PRI_NORMAL);

	browse_curr_file.file_handle = open((CHAR *) browse_curr_file.file_path_addr, O_RDONLY);
	if(browse_curr_file.file_handle > -1)
	{
		audio_wav_play(browse_curr_file.file_handle);
        display_task_msg_send(MSG_DISPLAY_TASK_PLAY_TIME_DRAW, NULL, 0, MSG_PRI_NORMAL);
		ap_peripheral_auto_off_force_disable_set(1);
		ap_browse_display_timer_start();
	}
	else
	{
		storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
	}
	return 0;
}

INT32S
ap_browse_wav_stop(
	STOR_SERV_PLAYINFO *info_ptr
)
{
	ap_browse_sts_set(~(BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE));
	ap_peripheral_auto_off_force_disable_set(0);
	ap_browse_display_timer_stop();
	display_task_msg_send(MSG_DISPLAY_TASK_PLAY_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
	audio_wav_stop();
	return 0;
}

INT8S
ap_browse_stop_handle(
	void
)
{
	if(browse_sts & (BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE))
	{
		if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
		{
			ap_browse_wav_stop(NULL);
		}
		else
		{
			ap_browse_mjpeg_stop();

			if(browse_curr_file.file_type != TK_IMAGE_TYPE_JPEG)
			{
				ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, 0, 0);
			}
		}

		if(browse_curr_file.file_handle > -1)
		{
			close(browse_curr_file.file_handle);
			browse_curr_file.file_handle = -1;
		}
        ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, 0, 0);
	}
	else
	{
        ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, 0, 0, 0);
	}
    return 0;
}

void
ap_browse_display_update_command(
	void
)
{
	INT32U	search_type;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);
	#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
    if((state_wifi_status_get() & FLAG_CONNECT) &&
		(state_wifi_status_get() & FLAG_OFF_DISPLAY)
	)
	{
		// RTSP don't get first, so not need to open file
   		return;
   	}
	#endif

	if((!s_usbd_pin) && ((browse_sts & (BROWSE_PLAYBACK_BUSY + BROWSE_PLAYBACK_PAUSE)) == 0) && (gPower_down == 0))
	{
		if(curr_storage_id == STORAGE_DEVICE)
		{
            search_type = STOR_SERV_SEARCH_ORIGIN;
            ap_browse_open_file_req((void *) &search_type);
        }
	}
}

void
ap_browse_no_media_show(
	INT16U str_type
)
{
	INT32U	display_addr;

    #if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
        if((state_wifi_status_get() & FLAG_CONNECT) &&
        (state_wifi_status_get() & FLAG_OFF_DISPLAY)
        )
        return;
    #endif

	DISP_P	display_frame = ap_display_queue_get(display_free_queue);
	while(display_frame == NULL)
	{
		osDelay(1);
		display_frame = ap_display_queue_get(display_free_queue);
	}

	display_frame->offset = 0;
	display_addr = display_frame->base + display_frame->offset;
	ap_display_clear_screen(display_addr);

	#if C_BATTERY_DETECT == CUSTOM_ON
	ap_state_handling_current_bat_lvl_show();
	ap_state_handling_current_charge_icon_show();
	#endif

	ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_LOCKED, 0, 0);
	ap_state_handling_warn_str_icon_draw(str_type, WARN_STR_COLOR);
	if(display_task_msg_send(MSG_DISPLAY_TASK_MJPEG_DRAW, &display_frame, sizeof(void *), MSG_PRI_NORMAL) == STATUS_FAIL)
	{
        if(ap_display_queue_put(display_free_queue, display_frame) != STATUS_OK)
        {
            DBG_PRINT("[ERROR]display_free_queue full\r\n");
        }
	}
}

void
ap_browse_display_timer_start(
	void
)
{
	if(browse_display_timerid == 0xFF)
	{
		browse_display_timerid = BROWSE_DISPLAY_UPDATE_TIMER_ID;
		drv_l2_sys_timer_set((void *)msgQSend, (void *)ApQ, MSG_APQ_DISPLAY_DRAW_TIMER, browse_display_timerid, BROWSE_DISPLAY_TIMER_INTERVAL);
	}
}

void
ap_browse_display_timer_stop(
	void
)
{
	if(browse_display_timerid != 0xFF)
	{
		drv_l2_sys_timer_kill(browse_display_timerid);
		browse_display_timerid = 0xFF;
	}
}

/*
	en_get_first:
	1: Enable to get first frame and display it on panel
	0: Skip to get first frame
*/
INT32S
ap_browse_reply_get_first_frame(
	STOR_SERV_PLAYINFO *info_ptr,
	INT8U en_get_first,
	INT32U out_buf,
	INT16U out_w,
	INT16U out_h
)
{
	INT32S nRet = STATUS_OK;
	BROWSE_STREAM_CTRL_T* pBrowse_stream_ctrl;
	DISP_P display_frame = NULL;
	IMAGE_DECODE_STRUCT img_info;
	INT16U tft_w;
	INT16U tft_h;
	INT32U dec_out_buf;
	INT8U  flag_output_data = 0;

	if(!en_get_first)
	{
		return STATUS_OK;
	}

	pBrowse_stream_ctrl = ap_browse_ctrl_get(info_ptr->file_type);
	if(!pBrowse_stream_ctrl)
	{
		ERR_PRINT("pBrowse_stream_ctrl is null\r\n");
		return STATUS_FAIL;
	}

	if(pBrowse_stream_ctrl->stream_get_first_func)
	{
		memset(&img_info, 0, sizeof(IMAGE_DECODE_STRUCT));
		img_info.file_handle = info_ptr->file_handle;
		img_info.source_size = info_ptr->file_size;

		if(out_buf) // Output decode data
		{
			flag_output_data = 1;
		}

		if(flag_output_data)
		{
			tft_w = out_w;
			tft_h = out_h;
			dec_out_buf = out_buf;
			img_info.output_format = C_SCALER_CTRL_OUT_GP420;
		}
		else
		{
			tft_w = drv_l2_display_tft_width_get();
			tft_h = drv_l2_display_tft_height_get();
		    do
			{
				display_frame = ap_display_queue_get(display_free_queue);
				osDelay(10);
			} while(!display_frame);

			display_frame->offset = 0;

			if(drv_l2_display_tft_fmt_get() == DISP_FMT_RGB565)
			{
				img_info.output_format = C_SCALER_CTRL_OUT_RGB565;
			}
			else if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
			{
				img_info.output_format = C_SCALER_CTRL_OUT_GP420;
			}
			else
			{
				img_info.output_format = C_SCALER_CTRL_OUT_YUYV;
			}

			dec_out_buf = display_frame->base+display_frame->offset + video_decode_display_offset();
		}

		img_info.source_type = TK_IMAGE_SOURCE_TYPE_FILE;
		img_info.output_ratio = 0;
		img_info.out_of_boundary_color = 0x008080;

		img_info.output_buffer_width = tft_w;
		img_info.output_buffer_height = tft_h;
		img_info.output_image_width = tft_w;
		img_info.output_image_height = tft_h-video_decode_display_h_offset()*2;
		img_info.output_buffer_pointer = dec_out_buf;

		nRet = pBrowse_stream_ctrl->stream_get_first_func(&img_info);
		if(nRet == STATUS_FAIL)
		{
			goto ERR_GET_FIRST;
		}
		Get_1st_decode_frm = 0; //browse get first frame, to do "copy&draw"
        if(info_ptr->file_type != TK_IMAGE_TYPE_WAV){
        	ap_browse_resolution_string_draw(info_ptr, img_info.input_image_width, img_info.input_image_height);
        }else{
       		display_task_msg_send(MSG_DISPLAY_TASK_RES_ICON_CLEAR, NULL, 0, MSG_PRI_NORMAL);
        }

        #if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
 	    if(!((state_wifi_status_get() & FLAG_CONNECT) &&
			(state_wifi_status_get() & FLAG_OFF_DISPLAY))
		)
		#endif
		{
            ap_browse_show_file_name(info_ptr, 1, dec_out_buf);
            display_task_msg_send(MSG_DISPLAY_FILE_DATE_TIME_DRAW, NULL, 0, MSG_PRI_NORMAL);
		}
		if(!flag_output_data)
		{
			if(pBrowse_stream_ctrl->stream_show_func)
			{
				nRet = pBrowse_stream_ctrl->stream_show_func(display_frame);
				if(nRet == STATUS_FAIL)
				{
					goto ERR_GET_FIRST;
				}
			}
		}
	}

	return STATUS_OK;

ERR_GET_FIRST:
	if(display_frame)
	{
		ap_display_queue_put(display_free_queue, display_frame);
	}

	return nRet;
}

void
ap_browse_reply_action(
	STOR_SERV_PLAYINFO *info_ptr
)
{
    #if C_TFT_DISPLAY_FUNC
	INT8U en_get_first = 1;
	#else
	INT8U en_get_first = 0;
	#endif
    struct stat_t buf_tmp;
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	ap_browse_sts_set(~BROWSE_OPEN_FILE_NOW);

	ap_browse_sts_set(BROWSE_DECODE_BUSY);

	if(info_ptr->err_flag == STOR_SERV_OPEN_OK)
	{
		browse_total_file_num = info_ptr->total_file_number;

		ap_state_handling_warn_str_icon_exit();
		ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, ICON_PLAYBACK_MOVIE, 0);

		gp_memcpy((INT8S *) &browse_curr_file, (INT8S *) info_ptr, sizeof(STOR_SERV_PLAYINFO));

		#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	    if((state_wifi_status_get() & FLAG_CONNECT) &&
			(state_wifi_status_get() & FLAG_OFF_DISPLAY)
		)
		{
			// RTSP don't get first
	   		en_get_first = 0;
	   	}
		#endif
        stat((CHAR *) info_ptr->file_path_addr, &buf_tmp);
		ap_browse_decode_file_date_time(buf_tmp.st_mtime,buf_tmp.st_mstime);
		ap_browse_reply_get_first_frame(info_ptr,en_get_first,0,0,0);
		/*
			Close file handle and then reopen file hande
			1. type is mov file at play mode
			2. now is wifi mode
		*/
		if(browse_curr_file.file_handle > -1)
		{
			close(browse_curr_file.file_handle);
			browse_curr_file.file_handle = -1;
		}
	}
	else
	if(info_ptr->err_flag == STOR_SERV_NO_MEDIA)
	{
		browse_total_file_num = 0;

        if(ap_state_handling_storage_id_get() != STORAGE_DEVICE)
        {
            display_task_msg_send(MSG_DISPLAY_TASK_PLAY_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
            ap_browse_no_media_show(STR_NO_SD);
        }
        else
        {
            ap_state_handling_icon_clear_cmd(ICON_NO_SD, 0, 0, 0);
            ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, ICON_PLAYBACK_MOVIE, 0);
            display_task_msg_send(MSG_DISPLAY_TASK_PLAY_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
            ap_browse_no_media_show(STR_NO_FILE);
        }
	}
	else
	{
		browse_total_file_num = 0;
		if(ap_state_handling_storage_id_get() != STORAGE_DEVICE)
		{
			ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, 0, 0);
			display_task_msg_send(MSG_DISPLAY_TASK_PLAY_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
			ap_browse_no_media_show(STR_NO_SD);
		}
		else
		{
			display_task_msg_send(MSG_DISPLAY_TASK_PLAY_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
			display_task_msg_send(MSG_DISPLAY_FILE_DATE_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
			ap_browse_volume_icon_clear_all();
			ap_browse_string_icon_clear();

            ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, ICON_PLAYBACK_MOVIE, ICON_NO_SD);
			ap_browse_no_media_show(STR_NO_FILE);

			if(info_ptr->err_flag == STOR_SERV_DECODE_ALL_FAIL)
			{
				close(info_ptr->file_handle);
			}
		}
	}

	ap_browse_sts_set(~BROWSE_DECODE_BUSY);
	storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);

}

INT32S
ap_browse_func_key_process(
	STOR_SERV_PLAYINFO *info_ptr,
	BROWSE_PLAY_STATE play_status
)
{
	INT32S nRet = STATUS_OK;
	struct sfn_info fd_info;
	BROWSE_STREAM_CTRL_T* pBrowse_stream_ctrl;
	IMAGE_DECODE_STRUCT img_info;
	INT16U out_w;
	INT16U out_h;

	pBrowse_stream_ctrl = ap_browse_ctrl_get(info_ptr->file_type);
	if(!pBrowse_stream_ctrl)
	{
		ERR_PRINT("pBrowse_stream_ctrl is null\r\n");
		return STATUS_FAIL;
	}

	switch(play_status)
	{
		case PLAY_NOW:
		{
			storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_STOP, NULL, 0, MSG_PRI_NORMAL);

			//DBG_PRINT("reopen %s \r\n",info_ptr->file_path_addr);

			info_ptr->file_handle = open((CHAR *) info_ptr->file_path_addr, O_RDONLY);
			if(info_ptr->file_handle < 0)
			{
				ERR_PRINT("fail to reopen %s \r\n",info_ptr->file_path_addr);
				RETURN(STATUS_FAIL);
			}

			if(sfn_stat(info_ptr->file_handle , &fd_info) < 0)
			{
				ERR_PRINT("fail to sfn_stat \r\n");
				RETURN(STATUS_FAIL);
			}
			info_ptr->file_size = fd_info.f_size;

			memset(&img_info, 0, sizeof(IMAGE_DECODE_STRUCT));
			img_info.file_handle = info_ptr->file_handle;
			img_info.source_size = info_ptr->file_size;
			img_info.source_type = TK_IMAGE_SOURCE_TYPE_FILE;

			#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
		    if((state_wifi_status_get() & FLAG_CONNECT) &&
				(state_wifi_status_get() & FLAG_OFF_DISPLAY)
			)
			{
				// Display on WIFI
				img_info.disp_where = DISP_ON_WIFI;
				out_w = wifi_display_width_get();
				out_h = wifi_display_height_get();
				img_info.output_format = C_SCALER_CTRL_OUT_GP420;
		    }
			else
			#endif
			{
				// Display on TFT
				img_info.disp_where = DISP_ON_TFT;
				out_w = drv_l2_display_tft_width_get();
				out_h = drv_l2_display_tft_height_get();

				if(drv_l2_display_tft_fmt_get() == DISP_FMT_RGB565)
				{
					img_info.output_format = C_SCALER_CTRL_OUT_RGB565;
				}
				else if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
				{
					img_info.output_format = C_SCALER_CTRL_OUT_GP420;
				}
				else
				{
					img_info.output_format = C_SCALER_CTRL_OUT_YUYV;
				}
			}

			img_info.output_buffer_width = out_w;
			img_info.output_buffer_height = out_h;
			img_info.output_image_width = out_w;
			img_info.output_image_height = out_h - video_decode_display_h_offset()*2;

			if(pBrowse_stream_ctrl->stream_play_func)
			{
				nRet = pBrowse_stream_ctrl->stream_play_func(&img_info);
				if(nRet == STATUS_FAIL)
				{
					RETURN(STATUS_FAIL);
				}
			}
		}
		break;
		case PLAY_PAUSE:
			if(pBrowse_stream_ctrl->stream_pause_func)
			{
				nRet = pBrowse_stream_ctrl->stream_pause_func();
				if(nRet == STATUS_FAIL)
				{
					RETURN(STATUS_FAIL);
				}
				else
				{
					// In pause mode, turn on MSG_STORAGE_SERVICE_TIMER_START
					RETURN(STATUS_OK);
				}
			}
		break;
		case PLAY_RESUME:
			if(pBrowse_stream_ctrl->stream_resume_func)
			{
				nRet = pBrowse_stream_ctrl->stream_resume_func();
				if(nRet == STATUS_FAIL)
				{
					RETURN(STATUS_FAIL);
				}

				// In resume mode, turn on MSG_STORAGE_SERVICE_TIMER_STOP
				storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_STOP, NULL, 0, MSG_PRI_NORMAL);
			}
		break;

		default:
		break;
	}

	return STATUS_OK;

Return:

	storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);

	return nRet;
}

void
ap_browse_func_key_active(
	void
)
{
	INT32S nRet = STATUS_OK;
	STOR_SERV_PLAYINFO *info_ptr = &browse_curr_file;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	// SD doesn't exist or file type is jpeg
	if(browse_sts & BROWSE_SD_NOT_EXIST)
	{
		return;
	}

	#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
    if((state_wifi_status_get() & FLAG_CONNECT) &&
		(state_wifi_status_get() & FLAG_OFF_DISPLAY)
	)
	{
		;
    }
	else
	#endif
	{
		/*
			Jpeg has shown on TFT by get first function
		*/
		if(info_ptr->file_type == TK_IMAGE_TYPE_JPEG)
		{
			return;
		}
	}

	ap_state_handling_icon_clear_cmd(ICON_PLAYBACK_PLAY, ICON_PLAYBACK_PAUSE, 0, 0);

	if(browse_sts == 0)
	{
		// SD has files
		if(browse_total_file_num)
		{
			// Start play
			ap_browse_sts_set(BROWSE_PLAYBACK_BUSY);
            ap_state_handling_icon_show_cmd(ICON_PLAYBACK_PAUSE, 0, 0, 0);
			nRet = ap_browse_func_key_process(info_ptr,PLAY_NOW);
			if(nRet == STATUS_FAIL)
			{
				ap_browse_sts_set(~(BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE));

                if(browse_curr_file.file_type == TK_IMAGE_TYPE_WAV)
                {
                    ap_browse_wav_stop(NULL);
                }
                else
                {
                    ap_browse_mjpeg_stop();
                    ap_browse_mjpeg_play_end();
                }

				if(info_ptr->file_handle > -1)
				{
					close(info_ptr->file_handle);
					info_ptr->file_handle = -1;
				}
			}

			/*
				WIFI mode :
				JPEG file only has play mode and close file handle at case:MSG_APQ_MJPEG_DECODE_END
			*/
			if(info_ptr->file_type == TK_IMAGE_TYPE_JPEG)
			{
				ap_browse_sts_set(~(BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE));
			}
		}
	}
	// Do pause if playing a file
	else if(browse_sts == BROWSE_PLAYBACK_BUSY)
	{
		ap_browse_sts_set(BROWSE_PLAYBACK_PAUSE);
		ap_state_handling_icon_show_cmd(ICON_PLAYBACK_PLAY, 0, 0, 0);
		ap_peripheral_auto_off_force_disable_set(0);

		osDelay(100);	//wait to update display icon

		ap_browse_func_key_process(info_ptr,PLAY_PAUSE);
	}
	// Do resume from pause state
	else if(browse_sts == (BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE))
	{
		ap_browse_sts_set(~BROWSE_PLAYBACK_PAUSE);
		ap_state_handling_icon_show_cmd(ICON_PLAYBACK_PAUSE, 0, 0, 0);
		ap_peripheral_auto_off_force_disable_set(1);

		osDelay(100);	//wait to update display icon

		ap_browse_func_key_process(info_ptr,PLAY_RESUME);
	}

}

INT32S
ap_browse_wifi_file_index_set(
	INT16U num
)
{
	wifi_playback_file_index = num;
	return 0;
}

INT16U
ap_browse_wifi_file_index_get(
	void
)
{
	return wifi_playback_file_index;
}

#if C_FILE_LIST
void ap_browse_show_N_file_name(STOR_SERV_PLAYINFO *info_ptr)
{

	STRING_ASCII_INFO ascii_str = {0};
	INT32U buf;
	INT8U i, j, type;
	INT16S play_index,m;
	INT16U trace_cnt;
	INT8S k;
	CHAR file_path[24];
	INT8U show_files_num = FLIST_SHOW_NUM;
	INT16U tft_w = drv_l2_display_tft_width_get();
	INT16U tft_h = drv_l2_display_tft_height_get();

	type = info_ptr->search_type;

	if(type == STOR_SERV_SEARCH_ORIGIN)
	{
		return;
	}

	if(type == STOR_SERV_SEARCH_INIT)
	{
		g_UI_File_list_info.enable_flag = 1;
	}

	if(g_UI_File_list_info.enable_flag != 1)
	{
		return;
	}

	//--check if g_play_index in g_file_list_play_index
	g_UI_File_list_info.in_flie_list_flag = -1;

	if(type == STOR_SERV_SEARCH_NEXT)
	{
		for(i=0 ; i<show_files_num ; i++)
		{
			if ((g_UI_File_list_info.file_list_play[i].file_list_play_index == g_play_index)
				&&(g_UI_File_list_info.file_list_play[i].file_foler == g_file_path[4])
			)
			{
				g_UI_File_list_info.in_flie_list_flag = 1;
				g_UI_File_list_info.hit_file = i;

				break;
			}
		}
	}

	if(type == STOR_SERV_SEARCH_PREV)
	{
		i = 0;
		for(k=(show_files_num-1) ; k>=0 ; k--)
		{
			if (g_UI_File_list_info.file_list_play[k].file_list_play_index == g_play_index)
			{
				g_UI_File_list_info.in_flie_list_flag = 1;
				// 因為是從下面開始比 , 所以要用i來存才合理
				g_UI_File_list_info.hit_file = i;

				break;
			}

			i++;
		}
	}

	for(j=0 ; j < display_frame_pool_size ; ++j)  // 準備好 N - 1 塊 displaybuffer , 因為正在顯示的buf不能修改
	{
		//---決定file_path / play_index / trace_cnt ----------------------------
		for(m=0 ; m<24 ; m++)
		{
			file_path[m]= g_file_path[m];
		}

		// g_play_index isn't in g_file_list_play_index
		if(g_UI_File_list_info.in_flie_list_flag == -1)
		{
			if(type == STOR_SERV_SEARCH_NEXT)
			{
				// -1 是因為等下是找next file
				play_index = g_play_index - 1;

				// 這是為了要顯示當前檔案
				g_UI_File_list_info.hit_file = 0;
			}

			if(type == STOR_SERV_SEARCH_PREV)
			{
				// +1 是因為等下是找pre file
				play_index = g_play_index + 1;

				// 這是為了要顯示當前檔案 , 是0 , 不是2 ,是因為重底部開始劃
				g_UI_File_list_info.hit_file = 0;
			}

			// 一開始r進入回放,會送initial,找到最新file path ,所以還是要處理,這邊用pre cmd 來處理
			if(type == STOR_SERV_SEARCH_INIT)
			{
				type = STOR_SERV_SEARCH_PREV;

				// +1 是因為等下是找pre file
				play_index = g_play_index + 1;

				// 這是為了要顯示當前檔案
				g_UI_File_list_info.hit_file = 0;
				k = show_files_num - 1;
			}
		}

		if(type == STOR_SERV_SEARCH_NEXT)
		{
		   // -1 是之前已經 +1 了
			trace_cnt = g_trace_cnt-1;
		}

		if(type == STOR_SERV_SEARCH_PREV)
		{
			// -1 是之前已經 +1 了
			trace_cnt = g_trace_cnt + 1;
			k=show_files_num - 1;
		}

		//-----填黑File list UI 介面的部份-------------------------------

		buf =  display_frame_pool[j].base + display_frame_pool[j].offset;

		// 正在顯示的buf不能修改
		if(drv_l2_tft_buffer_get() == buf)
		{
			g_UI_File_list_info.displaying_buf = buf;
			continue;
	    }
		else
		{
			// UI file list 的部分填黑
			ap_initial_displaybuf(buf);
		}

		//-----填黑File list UI 介面的部份-------------------------------
		ascii_str.pos_x = BROWSE_FILE_NAME_POS_X; // poistion x
		ascii_str.pos_y = BROWSE_FILE_NAME_POS_Y; // poistion y

		if(type == STOR_SERV_SEARCH_PREV)
		{
			// 從下面開始畫
			ascii_str.pos_y = BROWSE_FILE_NAME_POS_Y + (50*(show_files_num+1));
		}

		// 一次 show 3 files
		for(i=0;i<show_files_num;i++)
		{
			// g_play_index isn't in g_file_list_play_index , 所以要重找
			if(g_UI_File_list_info.in_flie_list_flag == -1)
			{
				ascii_str.str_ptr = (CHAR *) ap_storage_service_play_request(type,&play_index,&trace_cnt,&file_path); // file path
				// DBG_PRINT("type = %d play_index=%d g_play_index=%d \r\n",type,play_index,g_play_index);

				if(type == STOR_SERV_SEARCH_NEXT)
				{
					g_UI_File_list_info.file_list_play[i].file_list_play_index = play_index;
					g_UI_File_list_info.file_list_play[i].file_foler = file_path[4];
					g_UI_File_list_info.file_list_play[i].trace_cnt = trace_cnt;
				}

				if(type == STOR_SERV_SEARCH_PREV)
				{
					// 從底下開始填
					g_UI_File_list_info.file_list_play[k].file_list_play_index = play_index;
					g_UI_File_list_info.file_list_play[k].file_foler = file_path[4];
					g_UI_File_list_info.file_list_play[k].trace_cnt = trace_cnt;
					k--;
				}
			}

			// g_play_index in g_file_list_play_index
			if(g_UI_File_list_info.in_flie_list_flag == 1)
			{
				if(type == STOR_SERV_SEARCH_NEXT)
				{
					// -1 是因為等下是找next file
					play_index =  g_UI_File_list_info.file_list_play[i].file_list_play_index - 1;
					file_path[4] = g_UI_File_list_info.file_list_play[i].file_foler;
					trace_cnt = g_UI_File_list_info.file_list_play[i].trace_cnt - 1;
				}

				if(type == STOR_SERV_SEARCH_PREV)
				{
					// +1 是因為等下是找pre file
					play_index =  g_UI_File_list_info.file_list_play[k].file_list_play_index + 1;
					file_path[4] = g_UI_File_list_info.file_list_play[k].file_foler;
					trace_cnt = g_UI_File_list_info.file_list_play[k].trace_cnt + 1;
				}

				ascii_str.str_ptr = (CHAR *) ap_storage_service_play_request(type,&play_index,&trace_cnt,&file_path); // file path
				k--;
			}

			if(browse_curr_file.file_type != TK_IMAGE_TYPE_WAV)
			{
				ascii_str.str_ptr += 6;
			}

			if(g_UI_File_list_info.hit_file == i)
			{
				ascii_str.font_color = BROWSE_FILE_NAME_COLOR;
			}
			else
			{
				ascii_str.font_color = 0xffff;
			}

			ascii_str.font_type = 0;

			if(type == STOR_SERV_SEARCH_PREV)
			{
				// 從下面開始畫
				ascii_str.pos_y -= 50;
			}
			else
			{
				// poistion y
				ascii_str.pos_y += 50;
			}

			ascii_str.buff_w = tft_w;
			ascii_str.buff_h = tft_h;
			ap_state_resource_string_ascii_draw((INT16U*)buf, &ascii_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
		}
	}
}

void
ap_update_last_filelist_ui_buf(
	void
)
{
	INT32U display_buf = 0;

	display_buf = R_ROTATOR_BUF_I_ADDR;
	ap_setting_memcpy(display_buf, g_UI_File_list_info.displaying_buf, getDispDevBufSize());
	g_UI_File_list_info.displaying_buf = 0;
}
#endif

/* customer API for searching file */
void ap_browse_is_file_exist(INT32U *picCnt, INT32U *vidCnt)
{
    INT32U FileNodeBuffer;
    INT32U FolderBuffer;

    INT32U FolderNum;

	INT8S ret = 0;

    struct STFileNodeInfo stFNodeInfo;

    FileNodeBuffer 	= (INT32U)gp_malloc(2000 * 2);
    FolderBuffer	= (INT32U)gp_malloc(2000 * 2);

    if (!FileNodeBuffer || !FolderBuffer) 
	{
        DBG_PRINT("ERROR: Memory allocation failed!\r\n");
        if (FileNodeBuffer) gp_free((void *)FileNodeBuffer);
        if (FolderBuffer) gp_free((void *)FolderBuffer);
        return;
    }

	// 搜索JPG文件
    stFNodeInfo.disk = STORAGE_DEVICE;
    gp_strcpy((INT8S *)stFNodeInfo.extname, (INT8S *)"jpg"); // ?置要搜索的文件?展名? "jpg"
    stFNodeInfo.FileBuffer = (INT16U *)FileNodeBuffer;		 // ?置文件???地址，用于存?找到的JPG文件信息
    stFNodeInfo.FileBufferSize = 2000;						 // ?置文件???大小
    stFNodeInfo.FolderBuffer = (INT16U *)FolderBuffer;		 // ?置文件????地址，用于存?找到的文件?信息
    stFNodeInfo.FolderBufferSize = 2000;						 // ?置文件????大小
    stFNodeInfo.pExtName = NULL;								 // ?置文件?展名指?，NULL表示不使用
    
    ret = GetFileNumEx(&stFNodeInfo, &FolderNum, picCnt);	// 全?搜索jpg文件

    DBG_PRINT("ret: %d jpg file cnt : %d\r\n", ret,*picCnt);

	// 搜索MOV文件
    stFNodeInfo.disk = STORAGE_DEVICE;
    gp_strcpy((INT8S *)stFNodeInfo.extname, (INT8S *)"mov");
    stFNodeInfo.FileBuffer = (INT16U *)FileNodeBuffer;
    stFNodeInfo.FileBufferSize = 2000;
    stFNodeInfo.FolderBuffer = (INT16U *)FolderBuffer;
    stFNodeInfo.FolderBufferSize = 2000;
    stFNodeInfo.pExtName = NULL;
    ret = GetFileNumEx(&stFNodeInfo, &FolderNum, vidCnt);
    DBG_PRINT("ret: %d mov file cnt : %d\r\n", ret,*vidCnt);

    gp_free((void *) FolderBuffer);
    gp_free((void *) FileNodeBuffer);
}


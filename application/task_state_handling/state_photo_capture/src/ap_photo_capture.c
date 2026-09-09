#include "application.h"

//unit: KB
INT32U last_pic_size;
INT32U last_pic_size_temp;
INT32U left_pic_num = 0;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern INT32S video_encode_capture_picture(void);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
INT32U ap_capture_left_num_get(void)
{
	return left_pic_num;
}

void ap_capture_init(void)
{
	display_task_msg_send(MSG_DISPLAY_TASK_EFFECT_INIT, NULL, 0, MSG_PRI_NORMAL);
    display_task_msg_send(MSG_DISPLAY_TASK_REC_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);

	left_pic_num = ap_capture_left_num_calculate();

	ap_capture_show_resolution();
	ap_capture_show_left_num(left_pic_num);

	if(ap_state_handling_storage_id_get() != STORAGE_DEVICE)
	{
		ap_state_handling_icon_show_cmd(ICON_CAPTURE, ICON_NO_SD, 0, 0);
	}
	else
	{
		ap_state_handling_icon_show_cmd(ICON_CAPTURE, ICON_SD_CARD, 0, 0);
	}

	if(ap_state_config_anti_shaking_get())
	{
		ap_state_handling_icon_show_cmd(ICON_CAPTURE_ANTI_SHAKING, 0, 0, 0);
	}

	if(ap_state_config_capture_burst_get())
	{
		ap_state_handling_icon_show_cmd(ICON_CAPTURE_SEQUENCE, 0, 0, 0);
	}

	ap_state_handling_icon_show_cmd(ICON_CAPTURE_QUALITY_FINE + ap_state_config_quality_get(), 0, 0, 0);

	#if C_BATTERY_DETECT
	ap_state_handling_current_bat_lvl_show();
	ap_state_handling_current_charge_icon_show();
	#endif
}

void ap_capture_exit(void)
{
	INT32U temp;

	temp = ap_state_config_white_balance_get();
	if(temp != 0)
	{
		//DBG_PRINT ("capture reset awbmode to auto!!!\r\n");
		drv_l2_cdsp_set_awb_mode(AWB_AUTO_DC);
	}
}

void ap_capture_cdsp_config(void)
{
	INT8U temp_ev;
	INT8U temp_iso;
	INT8U temp_awb;
	INT8U temp_shking;

	/*ev*/
	temp_ev = ap_state_config_capture_ev_get();

	/*iso*/
	temp_iso = ap_state_config_iso_get();

	/*awb*/
	temp_awb = ap_state_config_white_balance_get();

	/*shaking*/
	temp_shking = ap_state_config_anti_shaking_get();

	//0:+2, 1:+5/3, 2:+4/3, 3:+1.0, 4:+2/3, 5:+1/3, 6:+0.0, 7:-1/3, 8:-2/3, 9:-1.0, 10:-4/3, 11:-5/3, 12:-2.0
	ap_state_handling_icon_show_cmd(ICON_EV6 + temp_ev, 0, 0, 0);
	ap_state_handling_icon_show_cmd(ICON_CAPTURE_ISO_AUTO + temp_iso, 0, 0, 0);
	ap_state_handling_icon_show_cmd(ICON_CAPTURE_WHITE_BALANCE_AUTO + temp_awb, 0, 0, 0);

	drv_l2_cdsp_set_ev_val(temp_ev);
	if(temp_shking == 1)
	{
        drv_l2_cdsp_set_iso(4);
    }
    else
    {
        drv_l2_cdsp_set_iso(temp_iso);
    }
	if(temp_awb)
	{
		drv_l2_gp_cdsp_set_awb_by_config(temp_awb);
	}
}

INT32S ap_capture_func_key_active(void)
{
	if(ap_state_handling_storage_id_get() != STORAGE_DEVICE)
	{
		ap_state_handling_show_insert_sdc();
		return STATUS_FAIL;
	}

	if((vfsFreeSpace(STORAGE_DEVICE) >> 20) < CARD_FULL_MB_SIZE)
	{
		ap_capture_show_left_num(0);
		ap_state_handling_show_sd_full();

		storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
		return STATUS_FAIL;
	}

	storage_task_msg_send(MSG_STORAGE_SERVICE_PIC_REQ, NULL, 0, MSG_PRI_NORMAL);
	return STATUS_OK;
}

INT32S
ap_capture_reply_action(
	STOR_SERV_FILEINFO *file_info_ptr
)
{
	if(file_info_ptr->file_handle >= 0)
	{
		if((__check_file_handle(file_info_ptr) == STATUS_OK) && (video_encode_capture_picture() == STATUS_OK))
		{
			return STATUS_OK;
		}
		else
		{
			close(file_info_ptr->file_handle);
			unlink((CHAR *) file_info_ptr->file_path_addr);
			if(file_info_ptr->file_handle_b >= 0)
			{
				close(file_info_ptr->file_handle_b);
				unlink((CHAR *) file_info_ptr->file_path_addr_b);
			}

			return STATUS_FAIL;
		}
	}
	else
	{
		ap_state_handling_show_no_sd();
		storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
		return STATUS_FAIL;
	}
}

void ap_capture_reply_done(INT8U ret, INT32U file_path_addr)
{
	struct stat_t buf_tmp;
	INT64U	temp;
	INT64S	temp1;
	INT64U	free_space;

	UsrFlushBuffers(STORAGE_DEVICE);

	#if C_CREATE_FILE_DEBUG
	FatSpeedUpPrintf(STORAGE_DEVICE);
	DBG_PRINT("\r\n");
	#endif

	stat((CHAR *) file_path_addr, &buf_tmp);
	temp = buf_tmp.st_size >> 10;
	temp1 = temp - last_pic_size_temp;
	last_pic_size_temp = last_pic_size_temp + temp1;
	//DBG_PRINT("Capture OK, Size=%d kbtye\r\n", buf_tmp.st_size >> 10);
    DBG_PRINT("Capture OK, Size=%lld kbtye\r\n", temp);
	free_space = vfsFreeSpace(STORAGE_DEVICE);
	DBG_PRINT("free_space(Mbyte)=%lld\r\n", ((free_space >> 20) - CARD_FULL_MB_SIZE));

	if(left_pic_num != 0)
	{
        left_pic_num -= 1;
	}
	ap_capture_show_left_num(left_pic_num);

	#if !C_SENSOR_SAVE_RAW_DATA
	storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
	#endif
}

INT32U ap_capture_left_num_calculate(void)
{	//unit: KB
	const INT32U pic_size[3][8] =
	{
		{ 1410, 1220, 1030, 753, 529, 385, 227, 88 },	//fine
		{ 1120, 996, 844, 603, 433, 310, 195, 79 },		//normal
		{ 810, 692, 577, 396, 273, 142, 95, 43 }		//economy
	};

	INT32U temp_pic_size, left_num;
	INT64U freespace;

	if(ap_state_handling_storage_id_get() != STORAGE_DEVICE)
	{
		left_num = 0;
	}
	else
	{
		if(last_pic_size_temp)
		{
			last_pic_size = last_pic_size_temp;
		}

		freespace = vfsFreeSpace(STORAGE_DEVICE);

		if(freespace > (CARD_FULL_MB_SIZE << 20))
			freespace -= (CARD_FULL_MB_SIZE << 20);
		else
			freespace = 0;

		if(freespace == 0)
		{
			left_num = 0;
		}
		else
		{
			///if(last_pic_size == 0)
			{
				temp_pic_size = pic_size[ap_state_config_quality_get()][ap_state_config_pic_size_get()];
			}
			/*else
			{
				temp_pic_size = last_pic_size;
			}*/

			left_num = (freespace >> 10) / temp_pic_size;
			if(left_num > 99999)
				left_num = 99999;
		}
	}
	return left_num;
}

void ap_capture_clear_left_num(void)
{
    display_task_msg_send(MSG_DISPLAY_TASK_PARK_ICON_CLEAR, NULL, 0, MSG_PRI_NORMAL);
}

void ap_capture_show_left_num(INT32U left_num)
{
	CHAR	digit[6] = {0};
    DISPLAY_ICONSHOW    *str_park_icon;

	digit[0] = (left_num / 10000) + 0x30;
	left_num %= 10000;

	digit[1] = (left_num / 1000) + 0x30;
	left_num %= 1000;

	digit[2] = (left_num / 100) + 0x30;
	left_num %= 100;

	digit[3] = (left_num / 10) + 0x30;
	left_num %= 10;

	digit[4] = left_num + 0x30;

    str_park_icon = allocate_and_draw_ascii_string_icon(&digit[0], WHITE_COLOR);
    if(str_park_icon)
    {
        str_park_icon->pos_x = CAPTURE_LEFT_NUM_POS_X;
        str_park_icon->pos_y = CAPTURE_LEFT_NUM_POS_Y;
        display_task_msg_send(MSG_DISPLAY_TASK_PARK_ICON_DRAW, &str_park_icon, sizeof(void *), MSG_PRI_NORMAL);
    }
}

void ap_capture_clear_resolution(void)
{
    display_task_msg_send(MSG_DISPLAY_TASK_RES_ICON_CLEAR, NULL, 0, MSG_PRI_NORMAL);
}

void ap_capture_show_resolution(void)
{
	#if C_SENSOR_SAVE_RAW_DATA
	extern INT32U g_raw_path_idx;
	const CHAR *resv_str[] = {"PURE", "LSC", "AWB", "GAMA", "NO", "NO", "NO", "NO"};
	#else
	const CHAR *resv_str[] = {"12M", "10M", "8M", "5M", "3M", "2M", "1.3M", "VGA"};
	#endif
	INT8U	temp;
    DISPLAY_ICONSHOW    *str_res_icon;

	#if C_SENSOR_SAVE_RAW_DATA
	temp = g_raw_path_idx;
	#else
	temp = ap_state_config_pic_size_get();
	#endif

	if(temp >= (sizeof(resv_str) / sizeof(*resv_str)))
	{
		temp = sizeof(resv_str) / sizeof(*resv_str) - 1;
	}

    str_res_icon = allocate_and_draw_ascii_string_icon(resv_str[temp], WHITE_COLOR);
    if(str_res_icon)
    {
        str_res_icon->pos_x = CAPTURE_RESV_POS_X;
        str_res_icon->pos_y = CAPTURE_RESV_POS_Y;
        display_task_msg_send(MSG_DISPLAY_TASK_RES_ICON_DRAW, &str_res_icon, sizeof(void *), MSG_PRI_NORMAL);
    }
}


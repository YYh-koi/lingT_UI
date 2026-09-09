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
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT8U	sh_curr_storage_id;
static INT8U	back_sensor_in_out = 0;		//0: out  1: in

#if C_BATTERY_DETECT
static BAT_LVL	bat_icon = BAT_LVL_3;
static INT8U	charge_icon = 0;
#endif

INT16U present_state;


#if C_WATER_MARK == CUSTOM_ON
DISPLAY_ICONSHOW    water_mark_icon = {0};
DISPLAY_ICONSHOW    water_mark_gp420 = {0};
DISPLAY_ICONSHOW    water_mark_yuyv = {0};
#endif

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
#if C_RTC_REBOOT
extern void ap_peripheral_rtc_alarm_disable(void);
#endif

extern INT32S ap_setting_menu_draw(void);

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

void ap_state_handling_warn_str_icon_exit(void)
{
	display_task_msg_send(MSG_DISPLAY_TASK_WARN_ICON_CLEAR, NULL, 0, MSG_PRI_NORMAL);
}

INT32S ap_state_handling_warn_str_icon_draw(INT16U str_idx, INT16U str_color)
{
	STRING_INFO         str_info = {0};
	t_STR_SIZE_STRUCT   str_size;
    DISPLAY_ICONSHOW    *str_icon;
    INT8U   Y, U, V;
	INT16U	tft_w = drv_l2_display_tft_width_get();
	INT16U	tft_h = drv_l2_display_tft_height_get();

	str_info.str_idx = str_idx;
	str_info.font_color = str_color;
	str_info.font_type = 0;
	str_info.language = ap_state_config_language_get();
    ap_state_resource_string_resolution_get(&str_info, &str_size);

	str_info.buff_w = ((tft_w/2) <= 320)? tft_w/2 : 320;
	str_info.buff_h = ((tft_h/2) <= 120)? tft_h/2 : 120;

    if(str_info.buff_w > str_size.string_width)
    {
        str_info.pos_x = (str_info.buff_w - str_size.string_width)/2;
    }
    else
    {
        str_info.pos_x = 0;
        if(str_size.string_width <= tft_w)
        {
            str_info.buff_w = str_size.string_width;
        }
        else
        {
            str_info.buff_w = tft_w;
        }
    }

	if(str_info.buff_h > str_size.string_height)
	{
        str_info.pos_y = (str_info.buff_h - str_size.string_height)/2;
    }
    else
    {
        str_info.pos_y = 0;
        if(str_size.string_height <= tft_h)
        {
            str_info.buff_h = str_size.string_height;
        }
        else
        {
            str_info.buff_h = tft_h;
        }
    }

	if(str_info.buff_w & 0x0001) str_info.buff_w += 1;
	if(str_info.buff_h & 0x0001) str_info.buff_h += 1;

	str_info.pos_x &= ~0x0001;
	str_info.pos_y &= ~0x0001;

    str_icon = gp_malloc(sizeof(DISPLAY_ICONSHOW));
    if(str_icon == NULL)
    {
        DBG_PRINT("%s icon malloc fail\r\n", __FUNCTION__);
    }

    str_icon->transparent = TRANSPARENT_COLOR;
    str_icon->icon_w = str_info.buff_w;
    str_icon->icon_h = str_info.buff_h;
    str_icon->pos_x = (tft_w - str_info.buff_w)/2;
    str_icon->pos_y = (tft_h - str_info.buff_h)/2;
    str_icon->pos_x &= ~0x0001;
    str_icon->pos_y &= ~0x0001;

	if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
	{
        str_icon->data = gp_malloc(str_info.buff_w * str_info.buff_h * 3 >> 1);
    }
    else
    {
        str_icon->data = gp_malloc(str_info.buff_w * str_info.buff_h << 1);
    }

    if(str_icon->data == NULL)
    {
        DBG_PRINT("%s icon data malloc fail\r\n", __FUNCTION__);
    }

    rgb_to_yuv(WARN_BACK_COLOR, &Y, &U, &V);

	switch(drv_l2_display_tft_fmt_get())
	{
        INT32S i, j, cnt;

		case DISP_FMT_GP420:
			cnt = ((str_icon->icon_h * 3) >> 1);
			for(i = 0; i < cnt; i++)
			{
				if((i % 3) == 1)
				{
                    for(j = 0; j < str_icon->icon_w; j++)
                    {
                        if(j & 0x0001)
                        {
                            *((INT8U *)(str_icon->data) + (i * str_icon->icon_w + j)) = V;
                        }
                        else
                        {
                            *((INT8U *)(str_icon->data) + (i * str_icon->icon_w + j)) = U;
                        }
                    }
				}
				else
				{
					gp_memset((INT8S *)(str_icon->data) + i * str_icon->icon_w, Y, str_icon->icon_w);
				}
			}
		break;

		case DISP_FMT_YUYV:
            for(i = 0; i < str_icon->icon_h; i++)
            {
                for(j = 0; j < str_icon->icon_w; j++)
                {
                    if(j & 0x0001)
                    {
                        *(str_icon->data + i * str_icon->icon_w + j) = (Y << 8) | U;
                    }
                    else
                    {
                        *(str_icon->data + i * str_icon->icon_w + j) = (Y << 8) | V;
                    }
                }
            }
		break;

		default:
            for(i = 0; i < str_icon->icon_h; i++)
            {
                for(j = 0; j < str_icon->icon_w; j++)
                {
                    *(str_icon->data + i * str_icon->icon_w + j) = WARN_BACK_COLOR;
                }
            }
		break;
	}

    ap_state_resource_string_draw(str_icon->data, &str_info, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
    display_task_msg_send(MSG_DISPLAY_TASK_WARN_ICON_DRAW, &str_icon, sizeof(void *), MSG_PRI_NORMAL);
    return 0;
}

void ap_state_handling_show_insert_sdc(void)
{
	DBG_PRINT("please insert sd card\r\n");

	ap_state_handling_warn_str_icon_draw(STR_INSERT_SDC, WARN_STR_COLOR);
	peripheral_task_msg_send(MSG_PERIPHERAL_TASK_CLEAR_SCREEN_COUNTDOWN, NULL, 0, MSG_PRI_NORMAL);
}

void ap_state_handling_show_no_sd(void)
{
	DBG_PRINT("no sd card\r\n");

	ap_state_handling_warn_str_icon_draw(STR_NO_SD, WARN_STR_COLOR);
	peripheral_task_msg_send(MSG_PERIPHERAL_TASK_CLEAR_SCREEN_COUNTDOWN, NULL, 0, MSG_PRI_NORMAL);
}

void ap_state_handling_show_busy(void)
{
	DBG_PRINT("busy...\r\n");

	ap_state_handling_warn_str_icon_draw(STR_PLEASE_WAIT, WARN_STR_COLOR);
}

void ap_state_handling_show_protected(void)
{
	DBG_PRINT("protected\r\n");

	ap_state_handling_warn_str_icon_draw(STR_PROTECTED, WARN_STR_COLOR);
	ap_setting_menu_draw();
}

void ap_state_handling_show_sd_full(void)
{
	DBG_PRINT("sd card full\r\n");

	ap_state_handling_warn_str_icon_draw(STR_SD_FULL, WARN_STR_COLOR);
	peripheral_task_msg_send(MSG_PERIPHERAL_TASK_CLEAR_SCREEN_COUNTDOWN, NULL, 0, MSG_PRI_NORMAL);
}

void ap_state_handling_show_sd_err(void)
{
	DBG_PRINT("sd card error\r\n");

	ap_state_handling_warn_str_icon_draw(STR_SDC_ERR, WARN_STR_COLOR);
	peripheral_task_msg_send(MSG_PERIPHERAL_TASK_CLEAR_SCREEN_COUNTDOWN, NULL, 0, MSG_PRI_NORMAL);
}

void ap_state_handling_show_battery_low(void)
{
	DBG_PRINT("battery low\r\n");

	display_task_msg_send(MSG_DISPLAY_TASK_EFFECT_INIT, NULL, 0, MSG_PRI_NORMAL);   //clear all icons
	ap_state_handling_warn_str_icon_draw(STR_BATTERY_LOW, WARN_STR_COLOR);
	peripheral_task_msg_send(MSG_PERIPHERAL_TASK_BATTERY_LOW_BLINK_START, NULL, 0, MSG_PRI_NORMAL);
}

#if C_WATER_MARK == CUSTOM_ON
void ap_state_handling_prepare_water_mark_icon(DISPLAY_ICONSHOW *pIcon, INT8U draw_type)
{
    STRING_INFO str = {0};
    STRING_ASCII_INFO ascii_str = {0};
    t_STR_SIZE_STRUCT str_res = {0};
    t_STR_SIZE_STRUCT str_res2 = {0};
    INT8U *ptr = ap_state_config_car_plate_number_get();

    if(pIcon->data)
    {
        gp_free(pIcon->data);
        pIcon->data = NULL;
    }

    ptr[8] = 0;     //string end

    str.language = LCD_SCH;
    str.font_color = WHITE_COLOR;
    str.font_type = 0;
    str.str_idx = ptr[0];
    ap_state_resource_string_resolution_get(&str, &str_res);

    ascii_str.str_ptr = (CHAR *)(ptr + 1);
    ascii_str.font_color = WHITE_COLOR;
    ascii_str.font_type = 0;
    ap_state_resource_ascii_string_resolution_get(&ascii_str, &str_res2);

    pIcon->icon_w = str_res.string_width + str_res2.string_width;
    pIcon->icon_h = (str_res.string_height > str_res2.string_height)? str_res.string_height : str_res2.string_height;
    if(pIcon->icon_w & 0x0001) pIcon->icon_w += 1;
    if(pIcon->icon_h & 0x0001) pIcon->icon_h += 1;
    pIcon->icon_h  += 4;
	switch(draw_type)
	{
        INT32S i, j, cnt;

		case YUV420_DRAW:
            pIcon->transparent = TRANSPARENT_COLOR_GP420;
            pIcon->data = gp_malloc(pIcon->icon_w * pIcon->icon_h * 3 >> 1);
            if(pIcon->data == NULL)
            {
                DBG_PRINT("[ERROR]%s:%d", __FUNCTION__, __LINE__);
                return;
            }

			cnt = pIcon->icon_h * 3 >> 1;
			for(i = 0; i < cnt; i++)
			{
				if((i % 3) == 1)
				{
					gp_memset((INT8S *)(pIcon->data) + i * pIcon->icon_w, 0x80, pIcon->icon_w);
				}
				else
				{
					gp_memset((INT8S *)(pIcon->data) + i * pIcon->icon_w, TRANSPARENT_COLOR_GP420, pIcon->icon_w);
				}
			}
            break;

		case YUYV_DRAW:
            pIcon->transparent = TRANSPARENT_COLOR_YUYV;
            pIcon->data = gp_malloc(pIcon->icon_w * pIcon->icon_h << 1);
            if(pIcon->data == NULL)
            {
                DBG_PRINT("[ERROR]%s:%d", __FUNCTION__, __LINE__);
                return;
            }

            for(i = 0; i < pIcon->icon_h; i++)
            {
                for(j = 0; j < pIcon->icon_w; j++)
                {
                    *(pIcon->data + i * pIcon->icon_w + j) = TRANSPARENT_COLOR_YUYV;
                }
            }
            break;

		case RGB565_DRAW:
		default:
            pIcon->transparent = TRANSPARENT_COLOR_RGB565;
            pIcon->data = gp_malloc(pIcon->icon_w * pIcon->icon_h << 1);
            if(pIcon->data == NULL)
            {
                DBG_PRINT("[ERROR]%s:%d", __FUNCTION__, __LINE__);
                return;
            }

            for(i = 0; i < pIcon->icon_h; i++)
            {
                for(j = 0; j < pIcon->icon_w; j++)
                {
                    *(pIcon->data + i * pIcon->icon_w + j) = TRANSPARENT_COLOR_RGB565;
                }
            }
            break;
	}

    str.buff_w = pIcon->icon_w;
    str.buff_h = pIcon->icon_h;
    str.pos_y = 2;
    ap_state_resource_string_draw(pIcon->data, &str, draw_type, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
    ascii_str.pos_x = str.pos_x;
    ascii_str.pos_y = str.pos_y;
    ascii_str.buff_w = pIcon->icon_w;
    ascii_str.buff_h = pIcon->icon_h;
    ap_state_resource_string_ascii_draw(pIcon->data, &ascii_str, draw_type, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
}
#endif

#if C_BATTERY_DETECT
void ap_state_handling_battery_icon_show(BAT_LVL bat_lvl)
{
	if(bat_lvl < BAT_LVL_NUM)
	{
		if(bat_icon != bat_lvl)
		{
			ap_state_handling_icon_clear_cmd((bat_icon + ICON_BATTERY_0), 0, 0, 0);
			bat_icon = bat_lvl;
		}

		if(!charge_icon)
		{
			ap_state_handling_icon_show_cmd((bat_icon + ICON_BATTERY_0), 0, 0, 0);
		}
	}
}

void ap_state_handling_charge_icon_show(INT8U charge_flag)
{
	if(charge_flag)
	{
		charge_icon = 1;
		ap_state_handling_icon_show_cmd(ICON_BATTERY_CHARGED, 0, 0, 0);
		peripheral_task_msg_send(MSG_PERIPHERAL_TASK_BATTERY_CHARGE_ICON_BLINK_START, NULL, 0, MSG_PRI_NORMAL);
		ap_state_handling_icon_clear_cmd((bat_icon + ICON_BATTERY_0), 0, 0, 0);
	}
	else
	{
		charge_icon = 0;
		peripheral_task_msg_send(MSG_PERIPHERAL_TASK_BATTERY_CHARGE_ICON_BLINK_STOP, NULL, 0, MSG_PRI_NORMAL);
		ap_state_handling_icon_clear_cmd(ICON_BATTERY_CHARGED, 0, 0, 0);
		ap_state_handling_icon_show_cmd((bat_icon + ICON_BATTERY_0), 0, 0, 0);
	}
}

void ap_state_handling_current_bat_lvl_show(void)
{
	if(!charge_icon)
	{
		ap_state_handling_icon_show_cmd((bat_icon + ICON_BATTERY_0), 0, 0, 0);
	}
}

void ap_state_handling_current_charge_icon_show(void)
{
	if(charge_icon) {
		ap_state_handling_icon_show_cmd(ICON_BATTERY_CHARGED, 0, 0, 0);
	} else {
		ap_state_handling_icon_clear_cmd(ICON_BATTERY_CHARGED, 0, 0, 0);
	}
}

#endif // #if C_BATTERY_DETECT

//If you want to draw over 4 icons, pls call this function again.
void ap_state_handling_icon_show_cmd(INT8U cmd1, INT8U cmd2, INT8U cmd3, INT8U cmd4)
{
	INT32U icons;

    icons = cmd1 + (cmd2 << 8) + (cmd3 << 16) + (cmd4 << 24);
    display_task_msg_send(MSG_DISPLAY_TASK_ICON_SHOW, &icons, sizeof(INT32U), MSG_PRI_NORMAL);
}

void ap_state_handling_icon_clear_cmd(INT8U cmd1, INT8U cmd2, INT8U cmd3, INT8U cmd4)
{
	INT32U icons;

    icons = cmd1 + (cmd2 << 8) + (cmd3 << 16) + (cmd4 << 24);
    display_task_msg_send(MSG_DISPLAY_TASK_ICON_CLEAR, &icons, sizeof(INT32U), MSG_PRI_NORMAL);
}

#if(defined(C_USB_DEVICE) && C_USB_DEVICE)
INT8U ap_state_handling_connect_to_pc(INT32U prev_state)
{
    INT8U 	type;
    #if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_OFF))
    INT8U   temp;
    #endif

    s_usbd_pin = 1;
    #if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_OFF))
    temp = KEY_MODE_NORMAL;
    peripheral_task_msg_send(MSG_PERIPHERAL_TASK_KEY_REGISTER, &temp, sizeof(INT8U), MSG_PRI_NORMAL);
    State_Seting.USB_select_entry_ptr(&prev_state);
    #else
    ap_state_config_usb_mode_set(1);
    ap_USB_setting_display_ok();
    #endif
    type = KEY_MODE_DISABLE;
    peripheral_task_msg_send(MSG_PERIPHERAL_TASK_KEY_REGISTER, &type, sizeof(INT8U), MSG_PRI_NORMAL);    


    #if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_OFF))
	storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_STOP, NULL, 0, MSG_PRI_NORMAL);
    temp = ap_state_config_usb_mode_get();
    if(temp == 1)
    {
        state_usbd_status_set(1);
        video_encode_sensor_on(STATE_CONNECT_TO_PC);
        video_encode_preview_mode_set(PREVIEW_MODE_FRONT);
        uvc_transmission_start();
        state_usbd_task_msg_send(MSG_USBCAM_PLUG_IN, NULL, 0, MSG_PRI_NORMAL);
    }
    else if(temp == 0)
    {
        state_usbd_task_msg_send(MSG_USBD_PLUG_IN, NULL, 0, MSG_PRI_NORMAL);
    }
    #else
    state_usbd_status_set(1);
    ///video_encode_sensor_on(STATE_CONNECT_TO_PC);
    ///video_encode_preview_mode_set(PREVIEW_MODE_FRONT);
    ///uvc_transmission_start();
    state_usbd_task_msg_send(MSG_USBCAM_PLUG_IN, NULL, 0, MSG_PRI_NORMAL);
    #endif

    return 0;
}

void ap_state_handling_disconnect_to_pc(void)
{
	INT8U 	type = KEY_MODE_NORMAL;
	INT32U	msg_id;
	INT8U   temp;

    temp = ap_state_config_usb_mode_get();
    if(temp == 1)
    {
		state_usbd_status_set(0);
        video_encode_sensor_off();
        osDelay(100);//wait sensor frame to stop
        uvc_transmission_stop();
        msg_id = MSG_USBCAM_PLUG_OUT;
        state_usbd_task_msg_send(msg_id, NULL, 0, MSG_PRI_NORMAL);
    }
    else if(temp == 0)
    {
        msg_id = MSG_USBD_PLUG_OUT;
        state_usbd_task_msg_send(msg_id, NULL, 0, MSG_PRI_NORMAL);
        storage_task_msg_send(MSG_STORAGE_USBD_EXIT, NULL, 0, MSG_PRI_NORMAL);
    }
    s_usbd_pin = 0;
	storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
	peripheral_task_msg_send(MSG_PERIPHERAL_TASK_KEY_REGISTER, &type, sizeof(INT8U), MSG_PRI_NORMAL);
}
#endif

void ap_state_handling_storage_id_set(INT8U stg_id)
{
	sh_curr_storage_id = stg_id;
}

INT8U ap_state_handling_storage_id_get(void)
{
	return sh_curr_storage_id;
}

void ap_state_handling_power_off(INT8U sw_reset)
{
	#if C_WATCHDOG
	INT32S i;
	#endif

#if C_SCREEN_SAVER
	if(screen_saver_enable)
	{
		screen_saver_enable = 0;
		ap_peripheral_lcd_backlight_set(BL_ON);
	}
#endif
	video_encode_sensor_off();

	peripheral_task_msg_send(MSG_PERIPHERAL_TASK_BATTERY_LOW_BLINK_STOP, NULL, 0, MSG_PRI_NORMAL);
    peripheral_task_msg_send(MSG_PERIPHERAL_TASK_BATTERY_CHARGE_ICON_BLINK_STOP, NULL, 0, MSG_PRI_NORMAL);

    storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_STOP, NULL, 0, MSG_PRI_NORMAL);

	#if C_LOGO_POWER_OFF
	display_task_msg_send(MSG_DISPLAY_TASK_EFFECT_INIT, NULL, 0, MSG_PRI_NORMAL);   //clear all icons

	if(!sw_reset)
	{
		INT32U	jpg_buf = 0;
		INT32U	size;
		INT16U	logo_fd;
		INT32S	status;
		DISP_P	display_frame;
		INT32U	display_buf;
		INT16U tft_w = drv_l2_display_tft_width_get();
		INT16U tft_h = drv_l2_display_tft_height_get();

		logo_fd = nv_open((INT8U *) "POWER_OFF_LOGO.JPG");
		if(logo_fd != 0xFFFF)
		{
			INT32S n;

			// wait at most 1s
			for(n = 1000; n > 0; --n)
			{
				display_frame = ap_display_queue_get(display_free_queue);
				if(display_frame == NULL)
				{
					DBG_PRINT("Q..\r\n");
					osDelay(1);
				}
				else
				{
					break;
				}
			};

			if(display_frame != NULL)
			{
				display_frame->offset = 0;
				display_buf = display_frame->base + display_frame->offset;
			}
			else
			{
				DBG_PRINT("Poweroff allocate tft buffer fail.\r\n");
				display_buf = drv_l2_tft_buffer_get();
			}

			size = nv_rs_size_get(logo_fd);
			if(display_buf)
			{
				jpg_buf = (INT32U) gp_malloc(size);
			}

			if(!display_buf)
			{
				DBG_PRINT("Poweroff allocate tft buffer retry fail.\r\n");
			}
			else if(!jpg_buf)
			{
				DBG_PRINT("Poweroff allocate jpeg buffer fail.[%d]\r\n", size);
			}
			else
			{
				if(nv_read(logo_fd, (INT32U) jpg_buf, size))
				{
					DBG_PRINT("Poweroff read resource_header fail.\r\n");
				}
				else
				{
					IMAGE_DECODE_STRUCT img_info;

					img_info.image_source = (INT32S) jpg_buf;
					img_info.source_size = size;
					img_info.source_type = TK_IMAGE_SOURCE_TYPE_BUFFER;

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
					img_info.output_ratio = 0;
					img_info.out_of_boundary_color = 0x008080;
					img_info.output_buffer_width = tft_w;
					img_info.output_buffer_height = tft_h;
					img_info.output_image_width = tft_w;
					img_info.output_image_height = tft_h;

					img_info.output_buffer_pointer = display_buf;

					status = jpeg_buffer_decode_and_scale(&img_info, 0 ,0);
					if(status == STATUS_FAIL)
					{
						DBG_PRINT("Poweroff decode jpeg file fail.\r\n");
					}
					else
					{
						if(display_buf != drv_l2_tft_buffer_get())
						{
							if(display_task_msg_send(MSG_DISPLAY_TASK_JPEG_DRAW, &display_frame, sizeof(void *), MSG_PRI_NORMAL) == STATUS_FAIL)
							{
							    if(ap_display_queue_put(display_free_queue, display_frame) != STATUS_OK)
							    {
							        DBG_PRINT("[ERROR]display_free_queue full\r\n");
							    }
							}
						}
					}
				}

				gp_free((void *) jpg_buf);
			}
		}

        if((ap_state_config_beep_sound_get() == 0) || (audio_effect_play(VOICE_ID_POWER_OFF) != 0))
        {
            osDelay(1000);
        }
        else
        {
            STAudioConfirm  *audio_temp;
            INT32U  msg_id;

            while(1)
            {
                if(msgQReceive(ApQ, &msg_id, (void *)ApQ_para, ApQ_para_len) == STATUS_FAIL)
                {
                    continue;
                }

                if(msg_id == EVENT_APQ_ERR_MSG)
                {
                    audio_temp = (STAudioConfirm *) ApQ_para;
                    if((audio_temp->result == AUDIO_ERR_DEC_FINISH) && (audio_temp->source_type == AUDIO_SRC_TYPE_APP_RS))
                    {
                        break;
                    }
                    else
                    {
                        audio_confirm_handler((STAudioConfirm *) ApQ_para);
                    }
                }
            }

            DBG_PRINT("POWER_OFF audio play finish.\r\n");

            //audio_effect_wait();
        }
	}

	#endif // #if C_LOGO_POWER_OFF

	_deviceunmount(STORAGE_DEVICE);
	//drvl2_sd_card_remove(STORAGE_SD_DEV);

	#if defined(ENABLE_CHECK_RTC) && ENABLE_CHECK_RTC == 1
	ap_state_config_date_time_power_off_check();     // check 存在spi 的 時間 是否太舊了
	ap_state_config_date_time_save_set();            // 將 RTCAPR的時間 , 存入會寫到 SPI 的 ARRAY "Global_User_Optins"
	#endif

    osENTER_CRITICAL();
	ap_state_config_store();
	//osEXIT_CRITICAL();

	DBG_PRINT("POWER OFF CONFIG STORE!\r\n");

	/*
		將 sensor 供電系統重斷開，以免sensor 內部電路把電壓拉住
		==注意:==
		LDO_V28 當作 DDR2 Power的1.8V 輸入,關閉這裡drv_l1_ldo28_sel(LDO_V28, 0);
		打開drv_l1_sys_power_handler()內的drv_l1_ldo28_sel(LDO_V28, 0);
		==注意:==
	*/
	#if 1
	drv_l1_ldo28_sel(LDO_V28, 0);	// LDO_2.8v => OFF
	#endif

	drv_l1_dac_disable();

	drv_l1_adc_vref_enable_set(FALSE);

	#if C_WATCHDOG
	// wait 30s for watch dog reset
	if(sw_reset)    // sw reset ,是為了"要是前鏡頭停止時" , 需要做sw rest的處理
	{
		for(i = 0 ; i < 300 ; ++i)
		{
			osDelay(100);
		}
	}

	#endif // #if C_WATCHDOG

	#if C_RTC_REBOOT
    ap_peripheral_rtc_alarm_disable();
	#endif

	ext_rtc_write(0x40, 0);   // clear rtc int


	#if C_GSENSOR_FUNCTION == CUSTOM_ON
	{
		INT8U gsensor_level;

		gsensor_level = ap_state_config_park_mode_G_sensor_get();
		drv_l2_gsensor_park_mode_init(gsensor_level);
	}
	#endif

	vic_global_mask_set();					// Mask all IRQ & FIQ

	#if defined(BACK_SENSOR_PWR)
	gpio_write_io(BACK_SENSOR_PWR, 0);
	#endif
	#ifdef FRONT_SENSOR_PWR_EN
	gpio_write_io(FRONT_SENSOR_PWR_EN, 0);
	#endif
	/* Turn off LCD backlight and panel */
	#if C_ASYN_DBLE_TFT
	drv_l2_asyn_double_tft_backlight_en_set(FALSE,FALSE);		/* display1 set FALSE ; display2 set FALSE */
	#else
	drv_l2_tft_backlight_en_set(TRUE);
	#endif


	drv_l2_tft_uninit((DISPLAY_DEVICE == DISDEV_TFT) ? TFT_1 : TFT_2);

	timer_stop(0);			/* stop timer0 */
	timer_stop(1);			/* stop timer1 */
	timer_stop(2);			/* stop timer2 */
	timer_stop(3);			/* stop timer2 */
	time_base_stop(0);		/* stop timebase A */
	time_base_stop(1);
	time_base_stop(2);		/* stop timebase C */
	// wait power key release
	if(PW_KEY_TYPE == READ_FROM_GPIO)
	{
		#if PW_KEY_ACTIVE == DATA_HIGH
		while(gpio_read_io(PW_KEY));
		#else
		while(!gpio_read_io(PW_KEY));
		#endif
	}
	else
	{
		while(drv_l1_sys_pwr_key_read(PW_KEY));
	}

    rtc_int_set
    (
        GPX_RTC_HR_IEN | GPX_RTC_MIN_IEN | GPX_RTC_SEC_IEN | GPX_RTC_HALF_SEC_IEN,
        (RTC_DIS & GPX_RTC_HR_IEN) | (RTC_DIS & GPX_RTC_MIN_IEN) | (RTC_DIS & GPX_RTC_SEC_IEN) | (RTC_DIS & GPX_RTC_HALF_SEC_IEN)
    );
	rtc_schedule_disable();
 	drv_l1_cp15_cache_disable();
	drv_l1_sys_power_handler();

}

void ap_state_handling_lcd_backlight_switch(INT8U enable)
{
	INT8U type;

	DBG_PRINT("%s = %d\r\n", __FUNCTION__,enable);
	if(enable)
	{
		type = BL_ON;
	}
	else
	{
		type = BL_OFF;
	}

	peripheral_task_msg_send(MSG_PERIPHERAL_TASK_LCD_BACKLIGHT_SET, &type, sizeof(INT8U), MSG_PRI_NORMAL);
}

INT32S ap_state_common_handling(INT32U msg_id)
{
	switch(msg_id)
	{
	case MSG_APQ_AUDIO_EFFECT_OK:
	case MSG_APQ_AUDIO_EFFECT_MENU:
	case MSG_APQ_AUDIO_EFFECT_UP:
	case MSG_APQ_AUDIO_EFFECT_DOWN:
	case MSG_APQ_AUDIO_EFFECT_MODE:
		return audio_effect_play(VOICE_ID_CLICK);
		break;

	default:
		break;
	}

	return 0;
}

void ap_state_handling_calendar_init(void)
{
	INT8U	dt[3];
	TIME_T	tm;

	ap_state_config_factory_date_get(dt);

	tm.tm_year = dt[0] + 2000;
	tm.tm_mon = dt[1];
	tm.tm_mday = dt[2];

	ap_state_config_factory_time_get(dt);

	tm.tm_hour = dt[0];
	tm.tm_min = dt[1];
	tm.tm_sec = dt[2];

	rtc_ext_to_int_set();

	cal_day_store_get_register(ap_state_config_base_day_set, ap_state_config_base_day_get, ap_state_config_store);
	cal_factory_date_time_set(&tm);
	calendar_init();
}

INT8U back_sensor_plug_status_get(void)
{
	return back_sensor_in_out;
}

void back_sensor_plug_status_set(INT8U in_out)
{
	back_sensor_in_out = in_out;
}

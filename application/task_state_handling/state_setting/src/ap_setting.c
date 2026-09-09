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
INT8U   g_setting_reply_action_flag;

#define MENU_TYPE_MODE_MENU         0x00    //mode menu according to prev_state for video record or capture or browse
#define MENU_TYPE_SYS_MENU          0x01    //system menu
INT8U   menu_type = MENU_TYPE_MODE_MENU;

#define MENU_LEVEL_MAIN_MENU        0x00
#define MENU_LEVEL_SUB_MENU         0x01
#define MENU_LEVEL_SECOND_SUB_MENU  0x02
INT8U   menu_level;                 //main menu, sub_menu, second submenu

INT32U  main_menu_item;             //0: reserved, 1 ~ max valid
SETUP_ITEM_INFO  sub_menu_item;
SETUP_ITEM_INFO  second_sub_menu_item;

INT16U  state_str;                  //STR_VIDEO, STR_CAPTURE or STR_PLAYBACK

INT8U setup_date_time[7];
INT8U date_time_update_flag;        // 0: no change 1: Update

INT8U setup_plate_number[9];
INT8U plate_number_update_flag;     // 0: no change 1: Update

DISP_P setting_frame_base;
DISP_P setting_frame_show;

/**************************************************************************
 *             F U N C T I O N    D E C L A R A T I O N S                 *
 **************************************************************************/
DISPLAY_ICONSHOW *ap_setting_icon_allocate(INT16U icon_width, INT16U icon_height);
INT32S ap_setting_unzip_GPZP_file(INT8U *path, INT32U buf);
void ap_setting_memcpy(INT32U dest, INT32U src, INT32U len);

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
Overlap_Setting_Function  Seting;

void overlap_setting_function_initial(void)
{
	Seting.ap_setting_init_ptr = &ap_setting_init;
	Seting.ap_setting_exit_ptr = &ap_setting_exit;
	Seting.ap_setting_mode_key_active_ptr = &ap_setting_mode_key_active;
	Seting.ap_setting_menu_key_active_ptr = &ap_setting_menu_key_active;
	Seting.ap_setting_func_key_active_ptr = &ap_setting_func_key_active;
	Seting.ap_setting_direction_key_active_ptr = &ap_setting_direction_key_active;
	Seting.ap_setting_format_reply_ptr = &ap_setting_format_reply;
}



INT32S ap_USB_setting_page_draw(void)
{
	DISPLAY_ICONSHOW    *setting_icon;
	DISPLAY_ICONSHOW    *icon_ptr;
	DISPLAY_ICONSHOW    icon;
	t_STR_SIZE_STRUCT	str_res;
	STRING_INFO         str = { 0 };
	INT16U	tft_w = drv_l2_display_tft_width_get();
	INT16U	tft_h = drv_l2_display_tft_height_get();
    DISP_P  display_frame;
    INT32U  display_buf, i;

    ap_state_handling_warn_str_icon_exit();     //disable warning icon display

	display_frame = ap_display_queue_get(display_free_queue);
	while(display_frame == NULL)
	{
		osDelay(1);
		display_frame = ap_display_queue_get(display_free_queue);
	}

	display_frame->offset = 0;
	display_buf = display_frame->base + display_frame->offset;

	ap_display_clear_screen(display_buf);

    // draw background
    setting_icon = ap_setting_icon_allocate(SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT);
    if(!setting_icon)
    {
        DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
        return  STATUS_FAIL;
    }

    if((tft_w < setting_icon->icon_w) || (tft_h < setting_icon->icon_h))
    {
        setting_icon->pos_x = 0;
        setting_icon->pos_y = 0;
    }
    else
    {
        setting_icon->pos_x = (tft_w - setting_icon->icon_w)/2;
        setting_icon->pos_y = (tft_h - setting_icon->icon_h)/2;
        setting_icon->pos_x &= ~0x0001;
        setting_icon->pos_y &= ~0x0001;
    }

    ap_setting_unzip_GPZP_file((INT8U *)"BACKGROUND.GPZP", (INT32U)setting_icon->data);

    //draw head string
	str.font_type = 0;
	str.font_color = WHITE_COLOR;
	str.language = ap_state_config_language_get();
	str.buff_w = SETTING_BACKGROUND_WIDTH;
	str.buff_h = SETTING_BACKGROUND_HEIGHT;
	str.str_idx = STR_USB;
	ap_state_resource_string_resolution_get(&str, &str_res);
	str.pos_x = SETTING_TOP_BAR_POS_X + SETTING_TOP_BAR_WIDTH + 6;
	str.pos_y = SETTING_TOP_BAR_POS_Y + ((SETTING_TOP_BAR_HEIGHT - str_res.string_height) >> 1);
	str.pos_x &= ~0x0001;
	str.pos_y &= ~0x0001;
	ap_state_resource_string_draw((INT16U *)setting_icon->data, &str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

	// draw top bar
    icon_ptr = ap_setting_icon_allocate(SETTING_TOP_BAR_WIDTH, SETTING_TOP_BAR_HEIGHT);
    if(!icon_ptr)
    {
        DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
        return  STATUS_FAIL;
    }

	icon_ptr->pos_x = SETTING_TOP_BAR_POS_X;
	icon_ptr->pos_y = SETTING_TOP_BAR_POS_Y;
	ap_setting_unzip_GPZP_file((INT8U *) "TOPBAR.GPZP", (INT32U)icon_ptr->data);
	ap_display_icon_draw((INT16U *)setting_icon->data, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);
	gp_free((void *)icon_ptr->data);
	gp_free((void *)icon_ptr);
	icon_ptr = NULL;

	// draw setting_base icon
	icon = display_icon_item[ICON_SETTING_BASE];
	icon.pos_x = SETTING_TOP_BAR_POS_X + ((SETTING_TOP_BAR_WIDTH - icon.icon_w) >> 1);
	icon.pos_y = SETTING_TOP_BAR_POS_Y + ((SETTING_TOP_BAR_HEIGHT - icon.icon_h) >> 1);
	ap_display_icon_draw((INT16U *)setting_icon->data, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, DISPLAY_ICON_BLUE1_FLAG);

	// draw up/down/ok
    icon.icon_w = SETTING_BACKGROUND_UP_WIDTH;
    icon.icon_h = SETTING_BACKGROUND_UP_HEIGHT;
	icon.pos_x = SETTING_BACKGROUND_UP_POS_X - 20;
	icon.pos_y	= SETTING_BACKGROUND_UP_POS_Y;
	icon.pos_x &= ~0x0001;
	icon.pos_y &= ~0x0001;

	icon.data = setting_background[1];   //UP
	ap_display_icon_draw((INT16U *)setting_icon->data, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, 0);

    //draw background down icon
	icon.pos_x = SETTING_BACKGROUND_UP_POS_X;
	icon.pos_x &= ~0x0001;
	icon.data = setting_background[2];   //DOWN
	ap_display_icon_draw((INT16U *)setting_icon->data, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, 0);

	//draw background ok icon
	icon.pos_x = SETTING_BACKGROUND_UP_POS_X + 20 + SETTING_BACKGROUND_UP_WIDTH;
	icon.pos_x &= ~0x0001;
	icon.data = setting_background[3];   //OK
	ap_display_icon_draw((INT16U *)setting_icon->data, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, 0);

    //draw select bar
    icon_ptr = ap_setting_icon_allocate(SETTING_SELECT_BAR_WIDTH, SETTING_SELECT_BAR_HEIGHT);
    if(!icon_ptr)
    {
        DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
        return  STATUS_FAIL;
    }

	icon_ptr->pos_x = (SETTING_BACKGROUND_WIDTH - SETTING_SELECT_BAR_WIDTH) >> 1;
	icon_ptr->pos_y = SETTING_SELECT_BAR_POS_Y + (main_menu_item & 0x03) * 40;
	ap_setting_unzip_GPZP_file((INT8U *) "SELECTBAR_LONG.GPZP", (INT32U)icon_ptr->data);
	ap_display_icon_draw((INT16U *)setting_icon->data, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);
	gp_free((void *)icon_ptr->data);
	gp_free((void *)icon_ptr);
	icon_ptr = NULL;

    // draw selection items
	str.font_type = 0;
	str.language = ap_state_config_language_get();
	str.buff_w = SETTING_BACKGROUND_WIDTH;
	str.buff_h = SETTING_BACKGROUND_HEIGHT;
	str.str_idx = STR_MASS_STORAGE;

	for(i = 0; i < 2; i++)
	{
		if(main_menu_item == i)
		{
			str.font_color = BLUE_COLOR;
		}
		else
		{
			str.font_color = WHITE_COLOR;
		}

        ap_state_resource_string_resolution_get(&str, &str_res);
        str.pos_x = (SETTING_BACKGROUND_WIDTH - str_res.string_width) >> 1;
        str.pos_y = SETTING_SELECT_BAR_POS_Y + ((SETTING_SELECT_BAR_POS_Y - str_res.string_height) >> 1) + i * 40;
        str.pos_x &= ~0x0001;
        str.pos_y &= ~0x0001;
        ap_state_resource_string_draw((INT16U *)setting_icon->data, &str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

		str.str_idx++;
	}

    ap_display_icon_draw((INT16U *)display_buf, tft_w, tft_h, setting_icon, 0);
    gp_free((void *)setting_icon->data);
    gp_free((void *)setting_icon);
    setting_icon = NULL;

    if(display_task_msg_send(MSG_DISPLAY_TASK_JPEG_DRAW, &display_frame, sizeof(DISP_P), MSG_PRI_NORMAL) == STATUS_FAIL)
    {
	    if(ap_display_queue_put(display_free_queue, display_frame) != STATUS_OK)
	    {
	        DBG_PRINT("[ERROR]display_free_queue full\r\n");
	    }
    }
	return  STATUS_OK;
}

void ap_USB_setting_display_ok(void)
{
	t_STR_SIZE_STRUCT str_res;
	STRING_INFO str = { 0 };
	INT16U tft_w = drv_l2_display_tft_width_get();
	INT16U tft_h = drv_l2_display_tft_height_get();
	DISP_P display_frame;
	INT32U display_buf;

	display_frame = ap_display_queue_get(display_free_queue);
	while(display_frame == NULL)
	{
		osDelay(1);
		display_frame = ap_display_queue_get(display_free_queue);
	}

	display_frame->offset = 0;
	display_buf = display_frame->base + display_frame->offset;

	ap_display_clear_screen(display_buf);

	str.font_type = 0;
	str.font_color = WHITE_COLOR;
	str.language = ap_state_config_language_get();
	str.buff_w = tft_w;
	str.buff_h = tft_h;

	if(ap_state_config_usb_mode_get() == 0)
	{
		str.str_idx = STR_MASS_STORAGE;
	}
	else
	{
		str.str_idx = STR_PC_CAMERA;
	}
	ap_state_resource_string_resolution_get(&str, &str_res);
	str.pos_x = (tft_w - str_res.string_width) >> 1;
	str.pos_y = (tft_h - str_res.string_height) >> 1;
	ap_state_resource_string_draw((INT16U *) display_buf, &str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

	if(display_task_msg_send(MSG_DISPLAY_TASK_JPEG_DRAW, &display_frame, sizeof(DISP_P), MSG_PRI_NORMAL) == STATUS_FAIL)
	{
	    if(ap_display_queue_put(display_free_queue, display_frame) != STATUS_OK)
	    {
	        DBG_PRINT("[ERROR]display_free_queue full\r\n");
	    }
	}
}


void ap_USB_setting_direction_key_active(INT32U msg_id)
{
    if(msg_id == MSG_APQ_PREV_KEY_ACTIVE)
    {
        if(main_menu_item)  main_menu_item--;
        else    main_menu_item = 1;
    }
    else
    {
        main_menu_item += 1;
        if(main_menu_item >= 2) main_menu_item = 0;
    }

	ap_USB_setting_page_draw();
}


DISPLAY_ICONSHOW *ap_setting_icon_allocate(INT16U icon_width, INT16U icon_height)
{
    DISPLAY_ICONSHOW    *icon_ptr;

    icon_ptr = gp_malloc(sizeof(DISPLAY_ICONSHOW));
    if(icon_ptr == NULL)
    {
        DBG_PRINT("%s icon malloc fail\r\n", __FUNCTION__);
        return NULL;
    }

    icon_ptr->icon_w = icon_width;
    icon_ptr->icon_h = icon_height;
    icon_ptr->transparent = TRANSPARENT_COLOR;

	if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
	{
        icon_ptr->data = gp_malloc(icon_ptr->icon_w * icon_ptr->icon_h * 3 >> 1);
    }
    else //RGB565 or YUYV
    {
        icon_ptr->data = gp_malloc(icon_ptr->icon_w * icon_ptr->icon_h << 1);
    }

    if(icon_ptr->data == NULL)
    {
        DBG_PRINT("%s icon data malloc fail\r\n", __FUNCTION__);

        gp_free(icon_ptr);
        return  NULL;
    }

    return icon_ptr;
}

INT32S ap_setting_unzip_GPZP_file(INT8U *path, INT32U buf)
{
	INT32U	size, read_buf;
	INT16U	logo_fd;
	INT8U	*zip_buf;

	logo_fd = nv_open(path);
	if(logo_fd != 0xFFFF)
	{
		size = nv_rs_size_get(logo_fd);
		read_buf = (INT32U)gp_malloc(size);
		if(!read_buf)
		{
			DBG_PRINT("allocate %s read buffer fail.[%d]\r\n", path, size);
			return STATUS_FAIL;
		}

		if(nv_read(logo_fd, read_buf, size))
		{
			DBG_PRINT("Failed to read icon resource\r\n");
			gp_free((void *) read_buf);
			read_buf = 0;
			return STATUS_FAIL;
		}

		zip_buf = (INT8U *) read_buf;
		if(gpzp_decode((INT8U *)&zip_buf[4], (INT8U *)buf) == STATUS_FAIL)
		{
			DBG_PRINT("Failed to unzip GPZP file\r\n");
			gp_free((void *) read_buf);
			read_buf = 0;
			return STATUS_FAIL;
		}

		gp_free((void *) read_buf);
		read_buf = 0;
		return STATUS_OK;
	}
	else
	{
		return STATUS_FAIL;
	}
}

INT8U CalendarCalculateDays(INT8U month, INT8U year)
{
    INT8U   DaysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    INT32U  temp_year;

    temp_year = year;
    temp_year += 2000;
    if((temp_year & 0x3) || (!(temp_year % 100) && (temp_year % 400)))
    {
        return DaysInMonth[month - 1];
    }
    else
    {
        if(month == 2)
        {
            return 29;
        }
        else
        {
            return DaysInMonth[month - 1];
        }
    }
}

void ap_setting_adjust_year(INT8U up_down_flag)
{
    INT8U temp, days;

    temp = setup_date_time[0];
    if(up_down_flag == SUB_MENU_MOVE_UP)
    {
        temp += 1;
        if(temp > 99)   // 2099 max
        {
            temp = 19;  // 2019 min
        }
    }
    else
    {
        temp -= 1;
        if(temp < 19)   // 2019 min
        {
            temp = 99;  // 2099 max
        }
    }
    setup_date_time[0] = temp;

    days = CalendarCalculateDays(setup_date_time[1], setup_date_time[0]);
    if(setup_date_time[2] > days)
    {
        setup_date_time[2] = days;
    }
}

void ap_setting_adjust_month(INT8U up_down_flag)
{
    INT8U temp, days;

    temp = setup_date_time[1];
    if(up_down_flag == SUB_MENU_MOVE_UP)
    {
        temp += 1;
        if(temp > 12)
        {
            temp = 1;
        }
    }
    else
    {
        temp -= 1;
        if((temp < 1) || (temp > 12))
        {
            temp = 12;
        }
    }
    setup_date_time[1] = temp;

    days = CalendarCalculateDays(setup_date_time[1], setup_date_time[0]);
    if(setup_date_time[2] > days)
    {
        setup_date_time[2] = days;
    }
}

void ap_setting_adjust_day(INT8U up_down_flag)
{
    INT8U temp, days;

    days = CalendarCalculateDays(setup_date_time[1], setup_date_time[0]);

    temp = setup_date_time[2];
    if(up_down_flag == SUB_MENU_MOVE_UP)
    {
        temp += 1;
        if(temp > days)
        {
            temp = 1;
        }
    }
    else
    {
        temp -= 1;
        if((temp < 1) || (temp > days))
        {
            temp = days;
        }
    }
    setup_date_time[2] = temp;
}


void ap_setting_adjust_date_time(INT8U up_down_flag) //up_down_flag: SUB_MENU_MOVE_UP or SUB_MENU_MOVE_DOWN
{
    INT8U temp;

    if(sub_menu_item.index == 6) // date/time mode
    {
        temp = setup_date_time[6];
        if(up_down_flag == SUB_MENU_MOVE_UP)
        {
            temp += 1;
            if(temp > 2)    temp = 0;
        }
        else
        {
            if(temp) temp -= 1;
            else temp = 2;
        }
        setup_date_time[6] = temp;
    }
    else if(sub_menu_item.index == 3) //hour
    {
        temp = setup_date_time[3];
        if(up_down_flag == SUB_MENU_MOVE_UP)
        {
            temp += 1;
            if(temp > 23)    temp = 0;
        }
        else
        {
            if(temp) temp -= 1;
            else temp = 23;
        }
        setup_date_time[3] = temp;
    }
    else if((sub_menu_item.index == 4) //minute
        || (sub_menu_item.index == 5))
    {
        temp = setup_date_time[sub_menu_item.index];
        if(up_down_flag == SUB_MENU_MOVE_UP)
        {
            temp += 1;
            if(temp > 59)    temp = 0;
        }
        else
        {
            if(temp) temp -= 1;
            else temp = 59;
        }
        setup_date_time[sub_menu_item.index] = temp;
    }
    else if(sub_menu_item.index < 3) // year, month, day
    {
        if(setup_date_time[6] == 0)  // YY/MM/DD
        {
            if(sub_menu_item.index == 0) //year
            {
                ap_setting_adjust_year(up_down_flag);
            }
            else if(sub_menu_item.index == 1) //month
            {
                ap_setting_adjust_month(up_down_flag);
            }
            else if(sub_menu_item.index == 2) //day
            {
                ap_setting_adjust_day(up_down_flag);
            }
        }
        else if(setup_date_time[6] == 1)  // DD/MM/YY
        {
            if(sub_menu_item.index == 0) //day
            {
                ap_setting_adjust_day(up_down_flag);
            }
            else if(sub_menu_item.index == 1) //month
            {
                ap_setting_adjust_month(up_down_flag);
            }
            else if(sub_menu_item.index == 2) //year
            {
                ap_setting_adjust_year(up_down_flag);
            }
        }
        else // MM/DD/YY
        {
            if(sub_menu_item.index == 0) //month
            {
                ap_setting_adjust_month(up_down_flag);
            }
            else if(sub_menu_item.index == 1) //day
            {
                ap_setting_adjust_day(up_down_flag);
            }
            else if(sub_menu_item.index == 2) //year
            {
                ap_setting_adjust_year(up_down_flag);
            }
        }
    }
}

INT32S ap_setting_date_time_menu_draw(INT32U buf)
{
    char    curr_str[] = "0000 / 00 / 00";
    INT8U   temp, i, year, inc;

    char    date_str0[] = "2000 / 00 / 00";
    char    date_str1[] = "00 / 00 / 2000";
    char    time_str[] = "00 : 00 : 00";
    char    *dm_str = "YY/MM/DD";
	STRING_ASCII_INFO	ascii_str = {0};
	STRING_ASCII_INFO   temp_str = {0};
	t_STR_SIZE_STRUCT   str_res;
	t_STR_SIZE_STRUCT   temp_str_res;
	t_STR_SIZE_STRUCT   temp_str_res2;
    DISPLAY_ICONSHOW *icon_ptr = NULL;
    DISPLAY_ICONSHOW icon;

    if(setup_date_time[6] == 0)  // YY/MM/DD
    {
        dm_str = "YY/MM/DD";

        //year
        year = setup_date_time[0];
        inc = 0;
        if(year > 99)
        {
            while(year > 99)
            {
                year -= 100;
                inc++;
            }
        }
        date_str0[1] += inc;

        temp = year / 10;
        temp += 0x30;
        date_str0[2] = temp;

        temp = year % 10;
        temp += 0x30;
        date_str0[3] = temp;

        //month
        temp = setup_date_time[1] / 10;
        temp += 0x30;
        date_str0[7] = temp;

        temp = setup_date_time[1] % 10;
        temp += 0x30;
        date_str0[8] = temp;

        //day
        temp = setup_date_time[2] / 10;
        temp += 0x30;
        date_str0[12] = temp;

        temp = setup_date_time[2] % 10;
        temp += 0x30;
        date_str0[13] = temp;
    }
    else if(setup_date_time[6] == 1)  // DD/MM/YY
    {
        dm_str = "DD/MM/YY";

        //year
        year = setup_date_time[0];
        inc = 0;
        if(year > 99)
        {
            while(year > 99)
            {
                year -= 100;
                inc++;
            }
        }
        date_str1[11] += inc;

        temp = year / 10;
        temp += 0x30;
        date_str1[12] = temp;

        temp = year % 10;
        temp += 0x30;
        date_str1[13] = temp;

        //month
        temp = setup_date_time[1] / 10;
        temp += 0x30;
        date_str1[5] = temp;

        temp = setup_date_time[1] % 10;
        temp += 0x30;
        date_str1[6] = temp;

        //day
        temp = setup_date_time[2] / 10;
        temp += 0x30;
        date_str1[0] = temp;

        temp = setup_date_time[2] % 10;
        temp += 0x30;
        date_str1[1] = temp;
    }
    else  // MM/DD/YY
    {
        dm_str = "MM/DD/YY";

        //year
        year = setup_date_time[0];
        inc = 0;
        if(year > 99)
        {
            while(year > 99)
            {
                year -= 100;
                inc++;
            }
        }
        date_str1[11] += inc;

        temp = year / 10;
        temp += 0x30;
        date_str1[12] = temp;

        temp = year % 10;
        temp += 0x30;
        date_str1[13] = temp;

        //month
        temp = setup_date_time[1] / 10;
        temp += 0x30;
        date_str1[0] = temp;

        temp = setup_date_time[1] % 10;
        temp += 0x30;
        date_str1[1] = temp;

        //day
        temp = setup_date_time[2] / 10;
        temp += 0x30;
        date_str1[5] = temp;

        temp = setup_date_time[2] % 10;
        temp += 0x30;
        date_str1[6] = temp;
    }

    //hour
    temp = setup_date_time[3] / 10;
    temp += 0x30;
    time_str[0] = temp;

    temp = setup_date_time[3] % 10;
    temp += 0x30;
    time_str[1] = temp;

    //minute
    temp = setup_date_time[4] / 10;
    temp += 0x30;
    time_str[5] = temp;

    temp = setup_date_time[4] % 10;
    temp += 0x30;
    time_str[6] = temp;

    //second
    temp = setup_date_time[5] / 10;
    temp += 0x30;
    time_str[10] = temp;

    temp = setup_date_time[5] % 10;
    temp += 0x30;
    time_str[11] = temp;

    icon.icon_w = SETTING_BACKGROUND_UP_WIDTH;
    icon.icon_h = SETTING_BACKGROUND_UP_HEIGHT;
    icon.transparent = TRANSPARENT_COLOR;

    /********************************************************************************/
    // draw first line
    ascii_str.font_color = WHITE_COLOR;
    ascii_str.font_type = 0;
    ascii_str.buff_w = SETTING_BACKGROUND_WIDTH;
    ascii_str.buff_h = SETTING_BACKGROUND_HEIGHT;

    if(setup_date_time[6] == 0)         // YY/MM/DD
    {
        ascii_str.str_ptr = date_str0;
    }
    else
    {
        ascii_str.str_ptr = date_str1;
    }

    ap_state_resource_ascii_string_resolution_get(&ascii_str, &str_res);

    ascii_str.pos_x = SETTING_SELECTBOX_SUB_POS_X + ((SETTING_SELECTBOX_SUB_WIDTH - str_res.string_width) >> 1);
    ascii_str.pos_y = SETTING_SELECTBOX_SUB_POS_Y + ((SETTING_SELECTBOX_SUB_HEIGHT - (SETTING_YEAR_SELECTBAR_HEIGHT * 3)) / 4)
                    + ((SETTING_YEAR_SELECTBAR_HEIGHT - str_res.string_height) >> 1);
    ascii_str.pos_x &= ~0x0001;
    ascii_str.pos_y &= ~0x0001;

    temp_str.font_color = WHITE_COLOR;
    temp_str.font_type = 0;
    temp_str.buff_w = SETTING_BACKGROUND_WIDTH;
    temp_str.buff_h = SETTING_BACKGROUND_HEIGHT;

    if(setup_date_time[6] == 0)         // YY/MM/DD
    { //draw select bar
        if(sub_menu_item.index == 0)
        {
            icon_ptr = ap_setting_icon_allocate(SETTING_YEAR_SELECTBAR_WIDTH, SETTING_YEAR_SELECTBAR_HEIGHT);
            if(!icon_ptr)
            {
                DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
                return  STATUS_FAIL;
            }

            for(i = 0; i < 4; i++)  curr_str[i] = date_str0[i];
            curr_str[i] = 0;

            temp_str.str_ptr = curr_str;
            ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res);

            icon_ptr->pos_x = ascii_str.pos_x - ((SETTING_YEAR_SELECTBAR_WIDTH - temp_str_res.string_width) >> 1);
            icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y + (SETTING_SELECTBOX_SUB_HEIGHT - SETTING_YEAR_SELECTBAR_HEIGHT * 3) / 4;
            icon_ptr->pos_x &= ~0x0001;
            icon_ptr->pos_y &= ~0x0001;

            ap_setting_unzip_GPZP_file((INT8U *)"YEAR_SELECTBAR.GPZP", (INT32U)icon_ptr->data);
            ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

            temp_str.pos_x = ascii_str.pos_x;
            temp_str.pos_y = ascii_str.pos_y;
            temp_str.font_color = BLUE_COLOR;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            temp_str = ascii_str;
            temp_str.str_ptr += 4;
            temp_str.pos_x += temp_str_res.string_width;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            icon.pos_x = icon_ptr->pos_x + ((SETTING_YEAR_SELECTBAR_WIDTH - SETTING_BACKGROUND_UP_WIDTH) >> 1);
            icon.pos_x &= ~0x0001;
        }
        else if(sub_menu_item.index == 1)
        {
            icon_ptr = ap_setting_icon_allocate(SETTING_DAY_SELECTBAR_WIDTH, SETTING_DAY_SELECTBAR_HEIGHT);
            if(!icon_ptr)
            {
                DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
                return  STATUS_FAIL;
            }

            temp_str.str_ptr = "00";
            ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res);
            temp_str.str_ptr = "2000 / ";
            ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res2);

            icon_ptr->pos_x = ascii_str.pos_x - ((SETTING_DAY_SELECTBAR_WIDTH - temp_str_res.string_width) >> 1) + temp_str_res2.string_width;
            icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y + (SETTING_SELECTBOX_SUB_HEIGHT - SETTING_DAY_SELECTBAR_HEIGHT * 3) / 4;
            icon_ptr->pos_x &= ~0x0001;
            icon_ptr->pos_y &= ~0x0001;

            ap_setting_unzip_GPZP_file((INT8U *)"DAY_SELECTBAR.GPZP", (INT32U)icon_ptr->data);
            ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

            for(i = 0; i < 7; i++)  curr_str[i] = date_str0[i];
            curr_str[i] = 0;
            temp_str.str_ptr = curr_str;
            temp_str.pos_x = ascii_str.pos_x;
            temp_str.pos_y = ascii_str.pos_y;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            for(i = 0; i < 2; i++)  curr_str[i] = date_str0[i + 7];
            curr_str[i] = 0;
            temp_str.font_color = BLUE_COLOR;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            for(i = 0; i < 5; i++)  curr_str[i] = date_str0[i + 9];
            curr_str[i] = 0;
            temp_str.font_color = WHITE_COLOR;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            icon.pos_x = icon_ptr->pos_x + ((SETTING_DAY_SELECTBAR_WIDTH - SETTING_BACKGROUND_UP_WIDTH) >> 1);
            icon.pos_x &= ~0x0001;
        }
        else if(sub_menu_item.index == 2)
        {
            icon_ptr = ap_setting_icon_allocate(SETTING_DAY_SELECTBAR_WIDTH, SETTING_DAY_SELECTBAR_HEIGHT);
            if(!icon_ptr)
            {
                DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
                return  STATUS_FAIL;
            }

            temp_str.str_ptr = "00";
            ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res);
            temp_str.str_ptr = "2000 / 00 / ";
            ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res2);

            icon_ptr->pos_x = ascii_str.pos_x - ((SETTING_DAY_SELECTBAR_WIDTH - temp_str_res.string_width) >> 1) + temp_str_res2.string_width;
            icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y + (SETTING_SELECTBOX_SUB_HEIGHT - SETTING_DAY_SELECTBAR_HEIGHT * 3) / 4;
            icon_ptr->pos_x &= ~0x0001;
            icon_ptr->pos_y &= ~0x0001;

            ap_setting_unzip_GPZP_file((INT8U *)"DAY_SELECTBAR.GPZP", (INT32U)icon_ptr->data);
            ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

            for(i = 0; i < 12; i++)  curr_str[i] = date_str0[i];
            curr_str[i] = 0;
            temp_str.str_ptr = curr_str;
            temp_str.pos_x = ascii_str.pos_x;
            temp_str.pos_y = ascii_str.pos_y;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            for(i = 0; i < 2; i++)  curr_str[i] = date_str0[i + 12];
            curr_str[i] = 0;
            temp_str.font_color = BLUE_COLOR;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            icon.pos_x = icon_ptr->pos_x + ((SETTING_DAY_SELECTBAR_WIDTH - SETTING_BACKGROUND_UP_WIDTH) >> 1);
            icon.pos_x &= ~0x0001;
        }
    }
    else // DD/MM/YY or MM/DD/YY
    { //draw select bar
        if(sub_menu_item.index == 0)
        {
            icon_ptr = ap_setting_icon_allocate(SETTING_DAY_SELECTBAR_WIDTH, SETTING_DAY_SELECTBAR_HEIGHT);
            if(!icon_ptr)
            {
                DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
                return  STATUS_FAIL;
            }

            for(i = 0; i < 2; i++)  curr_str[i] = date_str1[i];
            curr_str[i] = 0;

            temp_str.str_ptr = curr_str;
            ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res);

            icon_ptr->pos_x = ascii_str.pos_x - ((SETTING_DAY_SELECTBAR_WIDTH - temp_str_res.string_width) >> 1);
            icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y + (SETTING_SELECTBOX_SUB_HEIGHT - SETTING_DAY_SELECTBAR_HEIGHT * 3) / 4;
            icon_ptr->pos_x &= ~0x0001;
            icon_ptr->pos_y &= ~0x0001;

            ap_setting_unzip_GPZP_file((INT8U *)"DAY_SELECTBAR.GPZP", (INT32U)icon_ptr->data);
            ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

            temp_str.pos_x = ascii_str.pos_x;
            temp_str.pos_y = ascii_str.pos_y;
            temp_str.font_color = BLUE_COLOR;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            temp_str = ascii_str;
            temp_str.str_ptr += 2;
            temp_str.pos_x += temp_str_res.string_width;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            icon.pos_x = icon_ptr->pos_x + ((SETTING_DAY_SELECTBAR_WIDTH - SETTING_BACKGROUND_UP_WIDTH) >> 1);
            icon.pos_x &= ~0x0001;
        }
        else if(sub_menu_item.index == 1)
        {
            icon_ptr = ap_setting_icon_allocate(SETTING_DAY_SELECTBAR_WIDTH, SETTING_DAY_SELECTBAR_HEIGHT);
            if(!icon_ptr)
            {
                DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
                return  STATUS_FAIL;
            }

            temp_str.str_ptr = "00";
            ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res);
            temp_str.str_ptr = "00 / ";
            ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res2);

            icon_ptr->pos_x = ascii_str.pos_x - ((SETTING_DAY_SELECTBAR_WIDTH - temp_str_res.string_width) >> 1) + temp_str_res2.string_width;
            icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y + (SETTING_SELECTBOX_SUB_HEIGHT - SETTING_DAY_SELECTBAR_HEIGHT * 3) / 4;
            icon_ptr->pos_x &= ~0x0001;
            icon_ptr->pos_y &= ~0x0001;

            ap_setting_unzip_GPZP_file((INT8U *)"DAY_SELECTBAR.GPZP", (INT32U)icon_ptr->data);
            ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

            for(i = 0; i < 5; i++)  curr_str[i] = date_str1[i];
            curr_str[i] = 0;
            temp_str.str_ptr = curr_str;
            temp_str.pos_x = ascii_str.pos_x;
            temp_str.pos_y = ascii_str.pos_y;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            for(i = 0; i < 2; i++)  curr_str[i] = date_str1[i + 5];
            curr_str[i] = 0;
            temp_str.font_color = BLUE_COLOR;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            for(i = 0; i < 7; i++)  curr_str[i] = date_str1[i + 7];
            curr_str[i] = 0;
            temp_str.font_color = WHITE_COLOR;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            icon.pos_x = icon_ptr->pos_x + ((SETTING_DAY_SELECTBAR_WIDTH - SETTING_BACKGROUND_UP_WIDTH) >> 1);
            icon.pos_x &= ~0x0001;
        }
        else if(sub_menu_item.index == 2)
        {
            icon_ptr = ap_setting_icon_allocate(SETTING_YEAR_SELECTBAR_WIDTH, SETTING_YEAR_SELECTBAR_HEIGHT);
            if(!icon_ptr)
            {
                DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
                return  STATUS_FAIL;
            }

            temp_str.str_ptr = "2000";
            ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res);
            temp_str.str_ptr = "00 / 00 / ";
            ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res2);

            icon_ptr->pos_x = ascii_str.pos_x - ((SETTING_YEAR_SELECTBAR_WIDTH - temp_str_res.string_width) >> 1) + temp_str_res2.string_width;
            icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y + (SETTING_SELECTBOX_SUB_HEIGHT - SETTING_YEAR_SELECTBAR_HEIGHT * 3) / 4;
            icon_ptr->pos_x &= ~0x0001;
            icon_ptr->pos_y &= ~0x0001;

            ap_setting_unzip_GPZP_file((INT8U *)"YEAR_SELECTBAR.GPZP", (INT32U)icon_ptr->data);
            ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

            for(i = 0; i < 10; i++)  curr_str[i] = date_str1[i];
            curr_str[i] = 0;
            temp_str.str_ptr = curr_str;
            temp_str.pos_x = ascii_str.pos_x;
            temp_str.pos_y = ascii_str.pos_y;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            for(i = 0; i < 4; i++)  curr_str[i] = date_str1[i + 10];
            curr_str[i] = 0;
            temp_str.font_color = BLUE_COLOR;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

            icon.pos_x = icon_ptr->pos_x + ((SETTING_YEAR_SELECTBAR_WIDTH - SETTING_BACKGROUND_UP_WIDTH) >> 1);
            icon.pos_x &= ~0x0001;
        }
    }

    //draw cursor
    if(sub_menu_item.index < 3)
    {
        icon.pos_y = icon_ptr->pos_y - SETTING_BACKGROUND_UP_HEIGHT * 3 / 4;
        icon.pos_y &= ~0x0001;

        icon.data = setting_background[1];   //UP
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, 0);

        icon.pos_y = icon_ptr->pos_y + SETTING_YEAR_SELECTBAR_HEIGHT - SETTING_BACKGROUND_UP_HEIGHT / 4;
        icon.pos_y &= ~0x0001;

        icon.data = setting_background[2];   //DOWN
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, 0);

        gp_free(icon_ptr->data);
        gp_free(icon_ptr);
        icon_ptr = NULL;
    }
    else
    { //draw string
        ap_state_resource_string_ascii_draw((INT16U *)buf, &ascii_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
    }

    /********************************************************************************/
    // draw second line
    ascii_str.str_ptr = time_str;
    ap_state_resource_ascii_string_resolution_get(&ascii_str, &str_res);

    ascii_str.pos_x = SETTING_SELECTBOX_SUB_POS_X + ((SETTING_SELECTBOX_SUB_WIDTH - str_res.string_width) >> 1);
    ascii_str.pos_y = SETTING_SELECTBOX_SUB_POS_Y + ((SETTING_SELECTBOX_SUB_HEIGHT - (SETTING_DAY_SELECTBAR_HEIGHT * 3)) * 2 / 4)
                    + SETTING_DAY_SELECTBAR_HEIGHT
                    + ((SETTING_DAY_SELECTBAR_HEIGHT - str_res.string_height) >> 1);
    ascii_str.pos_x &= ~0x0001;
    ascii_str.pos_y &= ~0x0001;

    for(i = 0; i < 2; i++)  curr_str[i] = time_str[i];
    curr_str[i] = 0;

    temp_str.str_ptr = curr_str;
    ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res);

    //draw select bar
    icon_ptr = ap_setting_icon_allocate(SETTING_DAY_SELECTBAR_WIDTH, SETTING_DAY_SELECTBAR_HEIGHT);
    if(!icon_ptr)
    {
        DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
        return  STATUS_FAIL;
    }

    icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y + (SETTING_SELECTBOX_SUB_HEIGHT - SETTING_DAY_SELECTBAR_HEIGHT * 3) * 2 / 4 + SETTING_DAY_SELECTBAR_HEIGHT;
    icon_ptr->pos_y &= ~0x0001;

    if(sub_menu_item.index == 3)
    {
        icon_ptr->pos_x = ascii_str.pos_x - ((SETTING_DAY_SELECTBAR_WIDTH - temp_str_res.string_width) >> 1);
        icon_ptr->pos_x &= ~0x0001;

        ap_setting_unzip_GPZP_file((INT8U *)"DAY_SELECTBAR.GPZP", (INT32U)icon_ptr->data);
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

        temp_str.pos_x = ascii_str.pos_x;
        temp_str.pos_y = ascii_str.pos_y;
        temp_str.font_color = BLUE_COLOR;
        ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

        temp_str = ascii_str;
        temp_str.str_ptr += 2;
        temp_str.pos_x += temp_str_res.string_width;
        ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
    }
    else if(sub_menu_item.index == 4)
    {
        temp_str.str_ptr = "00 / ";
        ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res2);

        icon_ptr->pos_x = ascii_str.pos_x - ((SETTING_DAY_SELECTBAR_WIDTH - temp_str_res.string_width) >> 1) + temp_str_res2.string_width;
        icon_ptr->pos_x &= ~0x0001;

        ap_setting_unzip_GPZP_file((INT8U *)"DAY_SELECTBAR.GPZP", (INT32U)icon_ptr->data);
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

        for(i = 0; i < 5; i++)  curr_str[i] = time_str[i];
        curr_str[i] = 0;
        temp_str.str_ptr = curr_str;
        temp_str.pos_x = ascii_str.pos_x;
        temp_str.pos_y = ascii_str.pos_y;
        ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

        for(i = 0; i < 2; i++)  curr_str[i] = time_str[i + 5];
        curr_str[i] = 0;
        temp_str.font_color = BLUE_COLOR;
        ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

        for(i = 0; i < 5; i++)  curr_str[i] = time_str[i + 7];
        curr_str[i] = 0;
        temp_str.font_color = WHITE_COLOR;
        ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
    }
    else if(sub_menu_item.index == 5)
    {
        temp_str.str_ptr = "00 / 00 / ";
        ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res2);

        icon_ptr->pos_x = ascii_str.pos_x - ((SETTING_DAY_SELECTBAR_WIDTH - temp_str_res.string_width) >> 1) + temp_str_res2.string_width;
        icon_ptr->pos_x &= ~0x0001;

        ap_setting_unzip_GPZP_file((INT8U *)"DAY_SELECTBAR.GPZP", (INT32U)icon_ptr->data);
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

        for(i = 0; i < 10; i++)  curr_str[i] = time_str[i];
        curr_str[i] = 0;
        temp_str.str_ptr = curr_str;
        temp_str.pos_x = ascii_str.pos_x;
        temp_str.pos_y = ascii_str.pos_y;
        ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

        for(i = 0; i < 2; i++)  curr_str[i] = time_str[i + 10];
        curr_str[i] = 0;
        temp_str.font_color = BLUE_COLOR;
        ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
    }

    //draw cursor
    if((sub_menu_item.index >= 3) && (sub_menu_item.index < 6))
    {
        icon.pos_x = icon_ptr->pos_x + ((SETTING_DAY_SELECTBAR_WIDTH - SETTING_BACKGROUND_UP_WIDTH) >> 1);
        icon.pos_y = icon_ptr->pos_y - SETTING_BACKGROUND_UP_HEIGHT * 3 / 4;
        icon.pos_x &= ~0x0001;
        icon.pos_y &= ~0x0001;

        icon.data = setting_background[1];   //UP
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, 0);

        icon.pos_y = icon_ptr->pos_y + SETTING_YEAR_SELECTBAR_HEIGHT - SETTING_BACKGROUND_UP_HEIGHT / 4;
        icon.pos_y &= ~0x0001;

        icon.data = setting_background[2];   //DOWN
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, 0);
    }
    else
    { //draw string
        ap_state_resource_string_ascii_draw((INT16U *)buf, &ascii_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
    }

    /********************************************************************************/
    // draw third line
    ascii_str.str_ptr = dm_str;
    ap_state_resource_ascii_string_resolution_get(&ascii_str, &str_res);

    ascii_str.pos_x = SETTING_SELECTBOX_SUB_POS_X + ((SETTING_SELECTBOX_SUB_WIDTH - str_res.string_width) >> 1);
    ascii_str.pos_y = SETTING_SELECTBOX_SUB_POS_Y + ((SETTING_SELECTBOX_SUB_HEIGHT - (SETTING_DAY_SELECTBAR_HEIGHT * 3)) * 3 / 4)
                    + SETTING_DAY_SELECTBAR_HEIGHT * 2
                    + ((SETTING_DAY_SELECTBAR_HEIGHT - str_res.string_height) >> 1);
    ascii_str.pos_x &= ~0x0001;
    ascii_str.pos_y &= ~0x0001;

    if(sub_menu_item.index == 6)
    {
        icon_ptr->pos_x = ascii_str.pos_x - ((SETTING_DAY_SELECTBAR_WIDTH - temp_str_res.string_width) >> 1);
        icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y + (SETTING_SELECTBOX_SUB_HEIGHT - SETTING_DAY_SELECTBAR_HEIGHT * 3) * 3 / 4 + SETTING_DAY_SELECTBAR_HEIGHT * 2;
        icon_ptr->pos_x &= ~0x0001;
        icon_ptr->pos_y &= ~0x0001;

        ap_setting_unzip_GPZP_file((INT8U *)"DAY_SELECTBAR.GPZP", (INT32U)icon_ptr->data);
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

        //draw string
        for(i = 0; i < 2; i++)  curr_str[i] = dm_str[i];
        curr_str[i] = 0;

        temp_str.str_ptr = curr_str;
        ap_state_resource_ascii_string_resolution_get(&temp_str, &temp_str_res);
        temp_str.pos_x = ascii_str.pos_x;
        temp_str.pos_y = ascii_str.pos_y;
        temp_str.font_color = BLUE_COLOR;
        ap_state_resource_string_ascii_draw((INT16U *)buf, &temp_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
        ascii_str.pos_x += temp_str_res.string_width;
        ascii_str.str_ptr += 2;
        ap_state_resource_string_ascii_draw((INT16U *)buf, &ascii_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

        //draw cursor
        icon.pos_x = icon_ptr->pos_x + ((SETTING_DAY_SELECTBAR_WIDTH - SETTING_BACKGROUND_UP_WIDTH) >> 1);
        icon.pos_y = icon_ptr->pos_y - SETTING_BACKGROUND_UP_HEIGHT * 3 / 4;
        icon.pos_x &= ~0x0001;
        icon.pos_y &= ~0x0001;

        icon.data = setting_background[1];   //UP
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, 0);

        icon.pos_y = icon_ptr->pos_y + SETTING_YEAR_SELECTBAR_HEIGHT - SETTING_BACKGROUND_UP_HEIGHT / 4;
        icon.pos_y &= ~0x0001;

        icon.data = setting_background[2];   //DOWN
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, 0);
    }
    else
    { //draw string
        ap_state_resource_string_ascii_draw((INT16U *)buf, &ascii_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
    }

    gp_free(icon_ptr->data);
    gp_free(icon_ptr);
    icon_ptr = NULL;

    return STATUS_OK;
}



void ap_setting_adjust_plate_number(INT8U up_down_flag)
{
    if(second_sub_menu_item.index == 0)
    {
        if(up_down_flag == SUB_MENU_MOVE_UP)
        {
            setup_plate_number[0] += 1;
            if(setup_plate_number[0] > (STR_MAX + 30))
            {
                setup_plate_number[0] = STR_MAX;
            }
        }
        else if(up_down_flag == SUB_MENU_MOVE_DOWN)
        {
            setup_plate_number[0] -= 1;
            if(setup_plate_number[0] < STR_MAX)  setup_plate_number[0] = STR_MAX + 30;
        }
    }
    else if(second_sub_menu_item.index == 1)
    {
        if(up_down_flag == SUB_MENU_MOVE_UP)
        {
            setup_plate_number[second_sub_menu_item.index] += 1;
            if(setup_plate_number[second_sub_menu_item.index] == 'O' || setup_plate_number[second_sub_menu_item.index] == 'I')   //upcase letter
            {
                setup_plate_number[second_sub_menu_item.index] += 1;
            }
            else if(setup_plate_number[second_sub_menu_item.index] > 'Z')
            {
                setup_plate_number[second_sub_menu_item.index] = 'A';
            }
        }
        else if(up_down_flag == SUB_MENU_MOVE_DOWN)
        {
            setup_plate_number[second_sub_menu_item.index] -= 1;
            if(setup_plate_number[second_sub_menu_item.index] == 'O' || setup_plate_number[second_sub_menu_item.index] == 'I')   //upcase letter
            {
                setup_plate_number[second_sub_menu_item.index] -= 1;
            }
            else if(setup_plate_number[second_sub_menu_item.index] < 'A')
            {
                setup_plate_number[second_sub_menu_item.index] = 'Z';
            }
        }
    }
    else if(second_sub_menu_item.index == 7)
    {
        if(up_down_flag == SUB_MENU_MOVE_UP)
        {
            if(setup_plate_number[second_sub_menu_item.index] == 0)
            {
                setup_plate_number[second_sub_menu_item.index] = '0';   //digit
            }
            else
            {
                setup_plate_number[second_sub_menu_item.index] += 1;
                if(setup_plate_number[second_sub_menu_item.index] == 'O' || setup_plate_number[second_sub_menu_item.index] == 'I')   //upcase letter
                {
                    setup_plate_number[second_sub_menu_item.index] += 1;
                }
                else if(setup_plate_number[second_sub_menu_item.index] > 'Z')
                {
                    setup_plate_number[second_sub_menu_item.index] = 0;
                }
                else if(setup_plate_number[second_sub_menu_item.index] > '9' && setup_plate_number[second_sub_menu_item.index] < 'A')
                {
                    setup_plate_number[second_sub_menu_item.index] = 'A';
                }
            }
        }
        else if(up_down_flag == SUB_MENU_MOVE_DOWN)
        {
            if(setup_plate_number[second_sub_menu_item.index] == 0)
            {
                setup_plate_number[second_sub_menu_item.index] = 'Z';
            }
            else
            {
                setup_plate_number[second_sub_menu_item.index] -= 1;
                if(setup_plate_number[second_sub_menu_item.index] == 'O' || setup_plate_number[second_sub_menu_item.index] == 'I')   //upcase letter
                {
                    setup_plate_number[second_sub_menu_item.index] -= 1;
                }
                else if(setup_plate_number[second_sub_menu_item.index] < '0')   //digit
                {
                    setup_plate_number[second_sub_menu_item.index] = 0;
                }
                else if(setup_plate_number[second_sub_menu_item.index] > '9' && setup_plate_number[second_sub_menu_item.index] < 'A')
                {
                    setup_plate_number[second_sub_menu_item.index] = '9';
                }
            }
        }
    }
    else if(second_sub_menu_item.index > 1 && second_sub_menu_item.index < 7)
    {
        if(up_down_flag == SUB_MENU_MOVE_UP)
        {
            setup_plate_number[second_sub_menu_item.index] += 1;
            if(setup_plate_number[second_sub_menu_item.index] == 'O' || setup_plate_number[second_sub_menu_item.index] == 'I')   //upcase letter
            {
                setup_plate_number[second_sub_menu_item.index] += 1;
            }
            else if(setup_plate_number[second_sub_menu_item.index] > 'Z')
            {
                setup_plate_number[second_sub_menu_item.index] = '0';   //digit
            }
            else if(setup_plate_number[second_sub_menu_item.index] > '9' && setup_plate_number[second_sub_menu_item.index] < 'A')
            {
                setup_plate_number[second_sub_menu_item.index] = 'A';
            }
        }
        else if(up_down_flag == SUB_MENU_MOVE_DOWN)
        {
            setup_plate_number[second_sub_menu_item.index] -= 1;
            if(setup_plate_number[second_sub_menu_item.index] == 'O' || setup_plate_number[second_sub_menu_item.index] == 'I')   //upcase letter
            {
                setup_plate_number[second_sub_menu_item.index] -= 1;
            }
            else if(setup_plate_number[second_sub_menu_item.index] < '0')   //digit
            {
                setup_plate_number[second_sub_menu_item.index] = 'Z';
            }
            else if(setup_plate_number[second_sub_menu_item.index] > '9' && setup_plate_number[second_sub_menu_item.index] < 'A')
            {
                setup_plate_number[second_sub_menu_item.index] = '9';
            }
        }
    }
}

INT32S ap_setting_plate_number_menu_draw(INT32U buf)
{
    DISPLAY_ICONSHOW icon, *icon_ptr;
    STRING_INFO str = {0};
    STRING_ASCII_INFO ascii_str = {0};
    t_STR_SIZE_STRUCT str_res, str_res2;
    INT8U   draw_type;
    INT16S  i, posx_bak;
    CHAR    temp[2] = {0, 0};

    //draw select box
    icon_ptr = ap_setting_icon_allocate(SETTING_SELECTBOX_SUB_WIDTH, SETTING_SELECTBOX_SUB_HEIGHT);
    if(!icon_ptr)
    {
        DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
        return  STATUS_FAIL;
    }
    icon_ptr->pos_x = SETTING_SELECTBOX_SUB_POS_X;
    icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y;
    icon_ptr->pos_x &= ~0x0001;
    icon_ptr->pos_y &= ~0x0001;

    ap_setting_unzip_GPZP_file((INT8U *)"SELECTBOX_SUB.GPZP", (INT32U)icon_ptr->data);
    ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

    // draw select bar
    str.language = LCD_SCH;
    str.font_color = WHITE_COLOR;
    str.font_type = 0;
    str.str_idx = setup_plate_number[0];
    ap_state_resource_string_resolution_get(&str, &str_res);

    ascii_str.str_ptr = "A";
    ascii_str.font_color = WHITE_COLOR;
    ascii_str.font_type = 0;
    ap_state_resource_ascii_string_resolution_get(&ascii_str, &str_res2);

    icon_ptr->icon_w = SETTING_DAY_SELECTBAR_WIDTH;
    icon_ptr->icon_h = SETTING_DAY_SELECTBAR_HEIGHT;
    icon_ptr->pos_x += 6 + second_sub_menu_item.index * 30;
    icon_ptr->pos_y += (SETTING_SELECTBOX_SUB_HEIGHT - SETTING_DAY_SELECTBAR_HEIGHT) >> 1;
    icon_ptr->pos_y &= ~0x0001;
    ap_setting_unzip_GPZP_file((INT8U *)"DAY_SELECTBAR.GPZP", (INT32U)icon_ptr->data);
    ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

    //draw plate number
    str.buff_w = SETTING_BACKGROUND_WIDTH;
    str.buff_h = SETTING_BACKGROUND_HEIGHT;
    str.pos_x = icon_ptr->pos_x - second_sub_menu_item.index * 30;
    str.pos_x += (SETTING_DAY_SELECTBAR_WIDTH - str_res.string_width) >> 1;
    str.pos_y = icon_ptr->pos_y;
    str.pos_y += (SETTING_DAY_SELECTBAR_HEIGHT - str_res.string_height) >> 1;

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
    ap_state_resource_string_draw((INT16U *)buf, &str, draw_type, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

    posx_bak = icon_ptr->pos_x - second_sub_menu_item.index * 30;
    ascii_str.pos_y = icon_ptr->pos_y;
    ascii_str.pos_y += (SETTING_DAY_SELECTBAR_HEIGHT - str_res.string_height) >> 1;
    ascii_str.buff_w = SETTING_BACKGROUND_WIDTH;
    ascii_str.buff_h = SETTING_BACKGROUND_HEIGHT;
    ascii_str.str_ptr = &temp[0];

    for(i = 0; i < 7; i++)
    {
        ascii_str.pos_x = posx_bak + (i + 1) * 30;
        ascii_str.pos_x += (SETTING_DAY_SELECTBAR_WIDTH - str_res2.string_width) >> 1;
        temp[0] = setup_plate_number[i + 1];
        if(temp[0] == 0)    temp[0] = '_';
        ap_state_resource_string_ascii_draw((INT16U *)buf, &ascii_str, draw_type, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
    }

    //draw cursor
    icon.icon_w = SETTING_BACKGROUND_UP_WIDTH;
    icon.icon_h = SETTING_BACKGROUND_UP_HEIGHT;
    icon.transparent = TRANSPARENT_COLOR;
    icon.pos_x = icon_ptr->pos_x + ((SETTING_DAY_SELECTBAR_WIDTH - SETTING_BACKGROUND_UP_WIDTH) >> 1);
    icon.pos_y = icon_ptr->pos_y - SETTING_BACKGROUND_UP_HEIGHT * 3 / 4;
    icon.pos_x &= ~0x0001;
    icon.pos_y &= ~0x0001;

    icon.data = setting_background[1];   //UP
    ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, 0);

    icon.pos_y = icon_ptr->pos_y + SETTING_YEAR_SELECTBAR_HEIGHT - SETTING_BACKGROUND_UP_HEIGHT / 4;
    icon.pos_y &= ~0x0001;

    icon.data = setting_background[2];   //DOWN
    ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, 0);

    gp_free(icon_ptr->data);
    gp_free(icon_ptr);
    icon_ptr = NULL;

    return  STATUS_OK;
}

INT32S  ap_setting_top_bar_draw(INT32U buf)
{
    DISPLAY_ICONSHOW    *top_bar_icon = NULL;
    DISPLAY_ICONSHOW    icon_temp;
	t_STR_SIZE_STRUCT   str_res;
	STRING_INFO         str = {0};

	//show top bar string
	str.font_type = 0;
	str.buff_w = SETTING_BACKGROUND_WIDTH;
	str.buff_h = SETTING_BACKGROUND_HEIGHT;
	str.language = ap_state_config_language_get();
	str.font_color = 0xFFFF;

	if(menu_type == MENU_TYPE_SYS_MENU)
	{
		str.str_idx = STR_SETUP;
		ap_state_resource_string_resolution_get(&str, &str_res);
		str.pos_x = (SETTING_BACKGROUND_WIDTH - str_res.string_width) - 10;
        if(SETTING_TOP_BAR_HEIGHT > str_res.string_height)
        {
            str.pos_y = (SETTING_TOP_BAR_HEIGHT - str_res.string_height) >> 1;
        }
        else
        {
            str.pos_y = 0;
        }
        str.pos_y += 10;    //skip the top space
        str.pos_x &= ~0x0001;
        str.pos_y &= ~0x0001;
		ap_state_resource_string_draw((INT16U *)buf, &str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
	}
	else //MENU_TYPE_MODE_MENU
	{
		str.str_idx = state_str;
		ap_state_resource_string_resolution_get(&str, &str_res);
		str.pos_x = (SETTING_BACKGROUND_WIDTH - str_res.string_width) - 10;
        if(SETTING_TOP_BAR_HEIGHT > str_res.string_height)
        {
            str.pos_y = (SETTING_TOP_BAR_HEIGHT - str_res.string_height) >> 1;
        }
        else
        {
            str.pos_y = 0;
        }
        str.pos_y += 10;    //skip the top space
        str.pos_x &= ~0x0001;
        str.pos_y &= ~0x0001;
		ap_state_resource_string_draw((INT16U *)buf, &str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
	}

	//show top bar icon
    top_bar_icon = ap_setting_icon_allocate(SETTING_TOP_BAR_WIDTH, SETTING_TOP_BAR_HEIGHT);
    if(!top_bar_icon)
    {
        DBG_PRINT("[%s:%d] icon allocate fail\r\n", __FUNCTION__, __LINE__);
        return  STATUS_FAIL;
    }

	top_bar_icon->pos_y	= SETTING_TOP_BAR_POS_Y;
	if(menu_type == MENU_TYPE_SYS_MENU)
	{
		top_bar_icon->pos_x = SETTING_TOP_BAR_POS_X + SETTING_TOP_BAR_WIDTH;
	}
	else //MENU_TYPE_MODE_MENU
	{
		top_bar_icon->pos_x = SETTING_TOP_BAR_POS_X;
	}
	top_bar_icon->pos_x &= ~0x0001;
	top_bar_icon->pos_y &= ~0x0001;

    ap_setting_unzip_GPZP_file((INT8U *)"TOPBAR.GPZP", (INT32U)top_bar_icon->data);
	ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, top_bar_icon, 0);

    gp_free((void *)top_bar_icon->data);
    gp_free((void *)top_bar_icon);
    top_bar_icon = NULL;

	//show mode menu icon
    if(state_str == STR_VIDEO)
    {
        icon_temp = display_icon_item[ICON_VIDEO_RECORD];
    }
    else if(state_str == STR_CAPTURE)
    {
        icon_temp = display_icon_item[ICON_CAPTURE];
    }
    else //if(state_str == STR_PLAYBACK
    {
        icon_temp = display_icon_item[ICON_PLAYBACK];
    }

	icon_temp.pos_y	= SETTING_TOP_BAR_POS_Y;
	icon_temp.pos_x = SETTING_TOP_BAR_POS_X + ((SETTING_TOP_BAR_WIDTH - icon_temp.icon_w) >> 1);
	icon_temp.pos_x &= ~0x0001;
	icon_temp.pos_y &= ~0x0001;

	if(menu_type == MENU_TYPE_SYS_MENU)
    {
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon_temp, 0);
    }
    else
    {
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon_temp, DISPLAY_ICON_BLUE1_FLAG);
    }

	//show system menu icon
    icon_temp = display_icon_item[ICON_SETTING_BASE];
	icon_temp.pos_y	= SETTING_TOP_BAR_POS_Y;
	icon_temp.pos_x = SETTING_TOP_BAR_POS_X + SETTING_TOP_BAR_WIDTH + ((SETTING_TOP_BAR_WIDTH - icon_temp.icon_w) >> 1);
	icon_temp.pos_x &= ~0x0001;
	icon_temp.pos_y &= ~0x0001;

	if(menu_type == MENU_TYPE_SYS_MENU)
    {
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon_temp, DISPLAY_ICON_BLUE1_FLAG);
    }
    else
    {
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon_temp, 0);
    }
    return STATUS_OK;
}

INT32S calculate_total_item_and_current_page(SETTING_ITEM **setting_tbl, INT16U *total_item, INT16U *ptr_curr_page)
{
	INT16U   count_item, current_page;
	SETTING_ITEM *ptr_item;

	if(menu_type == MENU_TYPE_MODE_MENU)
	{
        if(state_str == STR_VIDEO)
        {
            ptr_item = &setting_record_table[0];
        }
        else if(state_str == STR_CAPTURE)
        {
            ptr_item = &setting_capture_table[0];
        }
        else //if(state_str == STR_PLAYBACK
        {
            ptr_item = &setting_play_table[0];
        }
    }
    else //MENU_TYPE_SYS_MENU
    {
        ptr_item = &setting_base_table[0];
    }

    if(setting_tbl)     *setting_tbl = ptr_item;

    count_item = 0;
    while(ptr_item->icon_data != NULL)
    {
        count_item++;
        ptr_item++;
    }

    if(total_item)  *total_item = count_item;

    current_page = main_menu_item? (main_menu_item - 1) : 0; //0 of main_menu_item is reserved
    current_page /= 4;
    if(ptr_curr_page)   *ptr_curr_page = current_page;

    return STATUS_OK;
}


INT32S  ap_setting_bottom_bar_draw(INT32U buf)
{
    DISPLAY_ICONSHOW    bottom_bar_icon;
    STRING_ASCII_INFO   ascii_str = {0};
    t_STR_SIZE_STRUCT   str_res;
    INT16U  num_item, num_page, curr_page;
	CHAR    page_num[] = "1/1";

    calculate_total_item_and_current_page(NULL, &num_item, &curr_page);

    num_page = num_item / 4;
    if(num_item % 4)    num_page += 1;

    curr_page += 1;                  //start with 1 instead of 0

    page_num[0] = curr_page + 0x30;
    page_num[2] = num_page + 0x30;

    ascii_str.font_color = WHITE_COLOR;
    ascii_str.font_type = 0;
    ascii_str.buff_w = SETTING_BACKGROUND_WIDTH;
    ascii_str.buff_h = SETTING_BACKGROUND_HEIGHT;
    ascii_str.str_ptr = page_num;

    ap_state_resource_ascii_string_resolution_get(&ascii_str, &str_res);
    ascii_str.pos_x = SETTING_BACKGROUND_WIDTH - str_res.string_width - 10;
    ascii_str.pos_y = SETTING_BACKGROUND_MODE_POS_Y + 4;
    ap_state_resource_string_ascii_draw((INT16U*)buf, &ascii_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

	//show background mode icon
    bottom_bar_icon.transparent = TRANSPARENT_COLOR;
    bottom_bar_icon.icon_w = SETTING_BACKGROUND_MODE_WIDTH;
    bottom_bar_icon.icon_h = SETTING_BACKGROUND_MODE_HEIGHT;
	bottom_bar_icon.pos_x = SETTING_BACKGROUND_MODE_POS_X;
	bottom_bar_icon.pos_y	= SETTING_BACKGROUND_MODE_POS_Y;
	bottom_bar_icon.pos_x &= ~0x0001;
	bottom_bar_icon.pos_y &= ~0x0001;

	bottom_bar_icon.data = setting_background[4];   //MODE
	ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &bottom_bar_icon, 0);

	//show background up icon
    bottom_bar_icon.icon_w = SETTING_BACKGROUND_UP_WIDTH;
    bottom_bar_icon.icon_h = SETTING_BACKGROUND_UP_HEIGHT;
	bottom_bar_icon.pos_x = SETTING_BACKGROUND_UP_POS_X;
	bottom_bar_icon.pos_y	= SETTING_BACKGROUND_UP_POS_Y;
	bottom_bar_icon.pos_x &= ~0x0001;
	bottom_bar_icon.pos_y &= ~0x0001;

	bottom_bar_icon.data = setting_background[1];   //UP
	ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &bottom_bar_icon, 0);

    //show background down icon
	bottom_bar_icon.pos_x = SETTING_BACKGROUND_UP_POS_X + 20;
	bottom_bar_icon.pos_x &= ~0x0001;
	bottom_bar_icon.data = setting_background[2];   //DOWN
	ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &bottom_bar_icon, 0);

	//show background ok icon
	bottom_bar_icon.pos_x = SETTING_BACKGROUND_UP_POS_X + 60;
	bottom_bar_icon.pos_x &= ~0x0001;
	bottom_bar_icon.data = setting_background[3];   //OK
	ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &bottom_bar_icon, 0);

    return STATUS_OK;
}

INT32S  ap_setting_main_menu_draw(INT32U buf)
{
    SETTING_ITEM    *setting_tbl;
    SETTING_ITEM    *ptr_item;
    DISPLAY_ICONSHOW icon;
    DISPLAY_ICONSHOW *icon_ptr;
    STRING_INFO     str = { 0 };

    INT16U  num_item, curr_page;
    INT16U	draw_cnt, i;

    calculate_total_item_and_current_page(&setting_tbl, &num_item, &curr_page);

    draw_cnt = num_item - curr_page * 4;
    if(draw_cnt > 4)
    {
        draw_cnt = 4;
    }

    if(main_menu_item)  //draw selection bar
    {
        icon_ptr = ap_setting_icon_allocate(SETTING_SELECT_BAR_WIDTH, SETTING_SELECT_BAR_HEIGHT);
        if(!icon_ptr)
        {
            DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
            return  STATUS_FAIL;
        }
		icon_ptr->pos_x = SETTING_SELECT_BAR_POS_X;
		icon_ptr->pos_x &= ~0x0001;
		icon_ptr->pos_y = SETTING_SELECT_BAR_POS_Y + ((main_menu_item - 1) % 4) * 40;
		icon_ptr->pos_y &= ~0x0001;

        ap_setting_unzip_GPZP_file((INT8U *)"SELECTBAR_LONG.GPZP", (INT32U)icon_ptr->data);
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

        gp_free(icon_ptr->data);
        gp_free(icon_ptr);
        icon_ptr = NULL;
    }

    icon.transparent = TRANSPARENT_COLOR;
    icon.icon_w = SETTING_HEAD_WIDTH;
    icon.icon_h = SETTING_HEAD_HEIGHT;
    icon.pos_x = SETTING_HEAD_POS_X;
    icon.pos_y = SETTING_HEAD_POS_Y;

    str.font_type = 0;
    str.language = ap_state_config_language_get();
    str.buff_w = SETTING_BACKGROUND_WIDTH;
    str.buff_h = SETTING_BACKGROUND_HEIGHT;

    ptr_item = setting_tbl + curr_page * 4;

    for(i = 0; i < draw_cnt; i++)
    {
        icon.data = ptr_item->icon_data;

        if(main_menu_item && ((main_menu_item - 1) % 4) == i)
        {
            ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, DISPLAY_ICON_BLUE1_FLAG);
            str.font_color = BLUE_COLOR;
        }
        else
        {
            ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, &icon, 0);
            str.font_color = WHITE_COLOR;
        }

        str.str_idx = ptr_item->str_idx;
        str.pos_x = icon.pos_x + icon.icon_w + 4;
        str.pos_x &= ~0x0001;
        str.pos_y = icon.pos_y + 6;
        str.pos_y &= ~0x0001;

		ap_state_resource_string_draw((INT16U *)buf, &str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

		icon.pos_y += SETTING_SELECT_BAR_HEIGHT;
        ptr_item += 1;
    }

    return STATUS_OK;
}

INT8U Get_cmputer_build_time(INT8U *buf)
{
	INT8U	temp[] = __DATE__;
	INT8U	month[4] = { 0 };
	INT8U	date[3] = { 0 };
	INT8U	year[5] = { 0 };

	INT16U	i, j, k;
	INT8U	Months[12][6] = { "Jan01", "Feb02", "Mar03", "Apr04", "May05", "Jun06", "Jul07", "Aug08", "Sep09", "Oct10", "Nov11", "Dec12" };

	for(i = 0; i < 3; i++)
		month[i] = temp[i];
	for(i = 0; i < 2; i++)
		date[i] = temp[4 + i];
	for(i = 0; i < 4; i++)
		year[i] = temp[7 + i];

	for(j = 0; j < 12; j++)
	{
		for(i = 0; i < 3; i++)
		{
			if(month[i] == Months[j][i])
			{
				if(i == 2)  break;
			}
			else
			{
				i = 0;
				break;
			}
		}

		if(i == 2)
		{
			for(k = 0; k < 2; k++)
			{
				month[k] = Months[j][i + 1 + k];
			}
			break;
		}
	}

	for(i = 0; i < 4; i++)
		*(buf + i) = year[i];
	for(i = 0; i < 2; i++)
		*(buf + 4 + i) = month[i];
	for(i = 0; i < 2; i++)
		*(buf + 6 + i) = date[i];
	*(buf + 8) = ' ';

	return 0;
}

INT32S  ap_setting_sub_menu_draw(INT32U buf)
{
    SETTING_ITEM    *setting_tbl;
    DISPLAY_ICONSHOW *icon_ptr;
    STRING_INFO     sub_str = { 0 };
    t_STR_SIZE_STRUCT   str_res;
    INT16U	draw_cnt, i, str_idx;

    if(menu_level == MENU_LEVEL_SUB_MENU)
    {
        calculate_total_item_and_current_page(&setting_tbl, NULL, NULL);
        str_idx = setting_tbl[main_menu_item - 1].str_idx;

        //draw select box
        icon_ptr = ap_setting_icon_allocate(SETTING_SELECTBOX_SUB_WIDTH, SETTING_SELECTBOX_SUB_HEIGHT);
        if(!icon_ptr)
        {
            DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
            return  STATUS_FAIL;
        }
		icon_ptr->pos_x = SETTING_SELECTBOX_SUB_POS_X;
		icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y;
		icon_ptr->pos_x &= ~0x0001;
		icon_ptr->pos_y &= ~0x0001;

        ap_setting_unzip_GPZP_file((INT8U *)"SELECTBOX_SUB.GPZP", (INT32U)icon_ptr->data);
        ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

        gp_free(icon_ptr->data);
        gp_free(icon_ptr);
        icon_ptr = NULL;

        if(str_idx == STR_VERSION)
        {
            STRING_ASCII_INFO   ascii_str = {0};
            INT8U	date[sizeof(__DATE__)];
            INT16U  i;

            Get_cmputer_build_time(date);

            ascii_str.font_color = WHITE_COLOR;
            ascii_str.font_type = 0;
            ascii_str.pos_x = SETTING_SELECTBOX_SUB_POS_X + 50;
            ascii_str.pos_y = 110;
            ascii_str.str_ptr = "20140115 V1.0";
            for(i = 0; i < 8; i++)
            {
                *(ascii_str.str_ptr + i) = date[i];
            }

            ascii_str.buff_w = SETTING_BACKGROUND_WIDTH;
            ascii_str.buff_h = SETTING_BACKGROUND_HEIGHT;
            ap_state_resource_string_ascii_draw((INT16U *)buf, &ascii_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);
        }
        else if(str_idx == STR_DATE_TIME2)
        {
            ap_setting_date_time_menu_draw(buf);
        }
        else
        {
            //draw select bar
            icon_ptr = ap_setting_icon_allocate(SETTING_SELECTBAR_MIDD_WIDTH, SETTING_SELECTBAR_MIDD_HEIGHT);
            if(!icon_ptr)
            {
                DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
                return  STATUS_FAIL;
            }

            icon_ptr->pos_x = SETTING_SELECTBAR_MIDD_POS_X;
            if((str_idx == STR_DEFAULT_SETTING) || (str_idx == STR_FORMAT))
            {
                icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y + SETTING_SELECTBOX_SUB_HEIGHT / 2;
            }
            else
            {
                icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y + ((SETTING_SELECTBOX_SUB_HEIGHT / 4 - SETTING_SELECTBAR_MIDD_HEIGHT) >> 1);
            }
            icon_ptr->pos_y += (sub_menu_item.index & 0x03) * (SETTING_SELECTBOX_SUB_HEIGHT / 4);

            icon_ptr->pos_x &= ~0x0001;
            icon_ptr->pos_y &= ~0x0001;

            ap_setting_unzip_GPZP_file((INT8U *)"SELECTBAR_MIDD.GPZP", (INT32U)icon_ptr->data);
            ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

            gp_free(icon_ptr->data);
            gp_free(icon_ptr);
            icon_ptr = NULL;

            //draw select box items
            sub_str.font_type = 0;
            sub_str.font_color = WHITE_COLOR;
            sub_str.buff_w = SETTING_BACKGROUND_WIDTH;
            sub_str.buff_h = SETTING_BACKGROUND_HEIGHT;

            if(str_idx != STR_LANGUAGE)
            {
                sub_str.language = ap_state_config_language_get();
                sub_str.str_idx = ((sub_menu_item.index >> 2) << 2) + sub_menu_item.str_idx_start;
                draw_cnt = sub_menu_item.item_max - ((sub_menu_item.index >> 2) << 2);
            }
            else
            {
                sub_str.str_idx = STR_MULTI_LANGUAGE;
                sub_str.language = ((sub_menu_item.index >> 2) << 2) + LCD_EN;
                draw_cnt = sub_menu_item.item_max - ((sub_menu_item.index >> 2) << 2);
            }

            if(draw_cnt > 4)
            {
                draw_cnt = 4;
            }

            ap_state_resource_string_resolution_get(&sub_str, &str_res);

            if((str_idx == STR_DEFAULT_SETTING) || (str_idx == STR_FORMAT))
            {
                sub_str.pos_y = SETTING_SELECTBOX_SUB_POS_Y + SETTING_SELECTBOX_SUB_HEIGHT / 2 + (((SETTING_SELECTBOX_SUB_HEIGHT / 4) - str_res.string_height) >> 1);
            }
            else
            {
                sub_str.pos_y = SETTING_SELECTBOX_SUB_POS_Y + ((SETTING_SELECTBOX_SUB_HEIGHT / 4 - str_res.string_height) >> 1);
            }

            for(i = 0; i < draw_cnt; i++)
            {
                if((sub_menu_item.index & 0x3) == i)
                {
                    sub_str.font_color = BLUE_COLOR;
                }
                else
                {
                    sub_str.font_color = WHITE_COLOR;
                }

                sub_str.pos_x = SETTING_SELECTBOX_SUB_POS_X + ((SETTING_SELECTBOX_SUB_WIDTH - str_res.string_width) >> 1);
                sub_str.pos_x &= ~0x0001;
                ap_state_resource_string_draw((INT16U *)buf, &sub_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

                if(str_idx != STR_LANGUAGE)
                {
                    sub_str.str_idx++;
                }
                else
                {
                    sub_str.language++;
                }

                ap_state_resource_string_resolution_get(&sub_str, &str_res);
                sub_str.pos_y +=  SETTING_SELECTBOX_SUB_HEIGHT / 4;
                sub_str.pos_y &= ~0x0001;
            }

            if((str_idx == STR_DEFAULT_SETTING) || (str_idx == STR_FORMAT))
            {
                sub_str.font_color = 0xFFFF;
                if(str_idx == STR_DEFAULT_SETTING)
                {
                    sub_str.str_idx = STR_DEFAULT_SETTING_SHOW_1;
                }
                else if(str_idx == STR_FORMAT)
                {
                    sub_str.str_idx = STR_FORMAT_SHOW_1;
                }

                ap_state_resource_string_resolution_get(&sub_str, &str_res);
                sub_str.pos_x = SETTING_SELECTBOX_SUB_POS_X + ((SETTING_SELECTBOX_SUB_WIDTH - str_res.string_width) >> 1);
                sub_str.pos_y = SETTING_SELECTBOX_SUB_POS_Y + (((SETTING_SELECTBOX_SUB_HEIGHT / 4) - str_res.string_height) >> 1);

                for(i = 0; i < 2; i++)
                {
                    ap_state_resource_string_draw((INT16U *)buf, &sub_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

                    sub_str.str_idx++;
                    ap_state_resource_string_resolution_get(&sub_str, &str_res);
                    sub_str.pos_x = SETTING_SELECTBOX_SUB_POS_X + ((SETTING_SELECTBOX_SUB_WIDTH - str_res.string_width) >> 1);
                    sub_str.pos_y = sub_str.pos_y + str_res.string_height;
                }
            }
        }
    }

    return STATUS_OK;
}


INT32S  ap_setting_second_sub_menu_draw(INT32U buf)
{
    SETTING_ITEM  *setting_tbl;
    DISPLAY_ICONSHOW *icon_ptr;
    STRING_INFO     sub_str = { 0 };
    t_STR_SIZE_STRUCT   str_res;
    INT16U	draw_cnt, i, str_idx;

    if(menu_level == MENU_LEVEL_SECOND_SUB_MENU)
    {
        calculate_total_item_and_current_page(&setting_tbl, NULL, NULL);
        str_idx = setting_tbl[main_menu_item - 1].str_idx;

        if(str_idx == STR_PLATE_NUMBER)
        {
            ap_setting_plate_number_menu_draw(buf);
        }
        else
        {
            //draw select box
            icon_ptr = ap_setting_icon_allocate(SETTING_SELECTBOX_SUB_WIDTH, SETTING_SELECTBOX_SUB_HEIGHT);
            if(!icon_ptr)
            {
                DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
                return  STATUS_FAIL;
            }
            icon_ptr->pos_x = SETTING_SELECTBOX_SUB_POS_X;
            icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y;
            icon_ptr->pos_x &= ~0x0001;
            icon_ptr->pos_y &= ~0x0001;

            ap_setting_unzip_GPZP_file((INT8U *)"SELECTBOX_SUB.GPZP", (INT32U)icon_ptr->data);
            ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

            gp_free(icon_ptr->data);
            gp_free(icon_ptr);
            icon_ptr = NULL;

            //draw select bar
            icon_ptr = ap_setting_icon_allocate(SETTING_SELECTBAR_MIDD_WIDTH, SETTING_SELECTBAR_MIDD_HEIGHT);
            if(!icon_ptr)
            {
                DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
                return  STATUS_FAIL;
            }

            icon_ptr->pos_x = SETTING_SELECTBAR_MIDD_POS_X;
            icon_ptr->pos_y = SETTING_SELECTBOX_SUB_POS_Y + ((SETTING_SELECTBOX_SUB_HEIGHT / 4 - SETTING_SELECTBAR_MIDD_HEIGHT) >> 1);
            icon_ptr->pos_y += (second_sub_menu_item.index & 0x03) * (SETTING_SELECTBOX_SUB_HEIGHT / 4);
            icon_ptr->pos_x &= ~0x0001;
            icon_ptr->pos_y &= ~0x0001;

            ap_setting_unzip_GPZP_file((INT8U *)"SELECTBAR_MIDD.GPZP", (INT32U)icon_ptr->data);
            ap_display_icon_draw((INT16U *)buf, SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT, icon_ptr, 0);

            gp_free(icon_ptr->data);
            gp_free(icon_ptr);
            icon_ptr = NULL;

            //draw select box items
            sub_str.font_type = 0;
            sub_str.font_color = WHITE_COLOR;
            sub_str.buff_w = SETTING_BACKGROUND_WIDTH;
            sub_str.buff_h = SETTING_BACKGROUND_HEIGHT;
            sub_str.language = ap_state_config_language_get();
            sub_str.str_idx = ((second_sub_menu_item.index >> 2) << 2) + second_sub_menu_item.str_idx_start;
            draw_cnt = second_sub_menu_item.item_max - ((second_sub_menu_item.index >> 2) << 2);

            if(draw_cnt > 4)
            {
                draw_cnt = 4;
            }

            ap_state_resource_string_resolution_get(&sub_str, &str_res);
            sub_str.pos_y = SETTING_SELECTBOX_SUB_POS_Y + ((SETTING_SELECTBOX_SUB_HEIGHT / 4 - str_res.string_height) >> 1);
            sub_str.pos_y &= ~0x0001;

            for(i = 0; i < draw_cnt; i++)
            {
                if((sub_menu_item.index & 0x3) == i)
                {
                    sub_str.font_color = BLUE_COLOR;
                }
                else
                {
                    sub_str.font_color = WHITE_COLOR;
                }

                sub_str.pos_x = SETTING_SELECTBOX_SUB_POS_X + ((SETTING_SELECTBOX_SUB_WIDTH - str_res.string_width) >> 1);
                sub_str.pos_x &= ~0x0001;
                ap_state_resource_string_draw((INT16U *)buf, &sub_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

                sub_str.str_idx++;
                ap_state_resource_string_resolution_get(&sub_str, &str_res);

                sub_str.pos_y +=  SETTING_SELECTBOX_SUB_HEIGHT / 4;
                sub_str.pos_y &= ~0x0001;
            }
        }
    }
    return STATUS_OK;
}


INT32S ap_setting_menu_draw(void)
{
	INT16U	tft_w = drv_l2_display_tft_width_get();
	INT16U	tft_h = drv_l2_display_tft_height_get();
    DISPLAY_ICONSHOW    *setting_icon;

    setting_icon = ap_setting_icon_allocate(SETTING_BACKGROUND_WIDTH, SETTING_BACKGROUND_HEIGHT);
    if(!setting_icon)
    {
        DBG_PRINT("%s icon allocate fail\r\n", __FUNCTION__);
        return  STATUS_FAIL;
    }

    if((tft_w < setting_icon->icon_w) || (tft_h < setting_icon->icon_h))
    {
        setting_icon->pos_x = 0;
        setting_icon->pos_y = 0;
    }
    else
    {
        setting_icon->pos_x = (tft_w - setting_icon->icon_w)/2;
        setting_icon->pos_y = (tft_h - setting_icon->icon_h)/2;
        setting_icon->pos_x &= ~0x0001;
        setting_icon->pos_y &= ~0x0001;
    }

    ap_setting_unzip_GPZP_file((INT8U *)"BACKGROUND.GPZP", (INT32U)setting_icon->data);

    ap_setting_top_bar_draw((INT32U)setting_icon->data);

    ap_setting_bottom_bar_draw((INT32U)setting_icon->data);

    ap_setting_main_menu_draw((INT32U)setting_icon->data);

    ap_setting_sub_menu_draw((INT32U)setting_icon->data);

    ap_setting_second_sub_menu_draw((INT32U)setting_icon->data);

    display_task_msg_send(MSG_DISPLAY_TASK_PARK_ICON_DRAW, &setting_icon, sizeof(void *), MSG_PRI_NORMAL);

    if(setting_frame_base)
    {
        setting_frame_show = ap_display_queue_get(display_free_queue);
        while(setting_frame_show == NULL)
        {
            osDelay(10);
            setting_frame_show = ap_display_queue_get(display_free_queue);
        }

        setting_frame_show->offset = 0;

        ap_setting_memcpy(setting_frame_show->base, setting_frame_base->base, getDispDevBufSize());
        if(display_task_msg_send(MSG_DISPLAY_TASK_MJPEG_DRAW, &setting_frame_show, sizeof(void *), MSG_PRI_NORMAL) == STATUS_FAIL)
        {
		    if(ap_display_queue_put(display_free_queue, setting_frame_show) != STATUS_OK)
		    {
		        DBG_PRINT("[ERROR]display_free_queue full\r\n");
		    }
		}
    }

    return STATUS_OK;
}


void ap_setting_second_sub_menu_active(INT8U act_type)
{
    SETTING_ITEM  *setting_tbl;
    INT16U  item, str_idx, i;
    INT8U   *ptr;

    if(main_menu_item)
    {
        item = main_menu_item - 1;  //start with 0
        calculate_total_item_and_current_page(&setting_tbl, NULL, NULL);
        str_idx = setting_tbl[item].str_idx;

        if(act_type == SUB_MENU_DRAW)
        {
            menu_level = MENU_LEVEL_SECOND_SUB_MENU;

            if(str_idx == STR_PLATE_NUMBER)
            {
                second_sub_menu_item.index = 0;
                second_sub_menu_item.item_max = 8;

                ptr = ap_state_config_car_plate_number_get();
                for(i = 0; i < 8; i++)
                {
                    setup_plate_number[i] = ptr[i];
                }
                setup_plate_number[8] = 0;  //end flag

                if(setup_plate_number[0] < STR_MAX)
                {
                    setup_plate_number[0] = STR_MAX;
                }
                else if(setup_plate_number[0] > (STR_MAX + 30))
                {
                    setup_plate_number[0] = STR_MAX + 30;
                }
            }
            else
            {
                second_sub_menu_item.index = 0;
                second_sub_menu_item.str_idx_start = STR_CANCEL;
                second_sub_menu_item.item_max = 2;
            }
        }
        else if(menu_level == MENU_LEVEL_SECOND_SUB_MENU)
        {
            if(act_type == SUB_MENU_MOVE_UP)
            {
                if(str_idx == STR_PLATE_NUMBER)
                {
                    ap_setting_adjust_plate_number(SUB_MENU_MOVE_UP);
                    plate_number_update_flag = 1;
                }
                else
                {
                    second_sub_menu_item.index += 1;
                    if(second_sub_menu_item.index >= second_sub_menu_item.item_max)
                    {
                        second_sub_menu_item.index = 0;
                    }
                }
            }
            else if(act_type == SUB_MENU_MOVE_DOWN)
            {
                if(str_idx == STR_PLATE_NUMBER)
                {
                    ap_setting_adjust_plate_number(SUB_MENU_MOVE_DOWN);
                    plate_number_update_flag = 1;
                }
                else
                {
                    if(second_sub_menu_item.index)
                    {
                        second_sub_menu_item.index -= 1;
                    }
                    else
                    {
                        second_sub_menu_item.index = second_sub_menu_item.item_max - 1;
                    }
                }
            }
            else if(act_type == FUNCTION_ACTIVE)
            {
                if(str_idx == STR_PLATE_NUMBER)
                {
                    second_sub_menu_item.index++;
                    if(second_sub_menu_item.index >= second_sub_menu_item.item_max)
                    {
                        second_sub_menu_item.index = 0;
                    }
                }
                else
                {
                    if((str_idx == STR_DELETE) && ((second_sub_menu_item.index + second_sub_menu_item.str_idx_start) == STR_OK))
                    {
                        g_setting_reply_action_flag = 0;    //wait task_storage_service to send ack

                        if((sub_menu_item.index + sub_menu_item.str_idx_start) == STR_DELETE_CURRENT)
                        {
                            storage_task_msg_send(MSG_STORAGE_SERVICE_VIDEO_FILE_DEL, NULL, 0, MSG_PRI_NORMAL);
                        }
                        else if((sub_menu_item.index + sub_menu_item.str_idx_start) == STR_DELETE_ALL)
                        {
                            storage_task_msg_send(MSG_STORAGE_SERVICE_FILE_DEL_ALL, NULL, 0, MSG_PRI_NORMAL);
                        }
                    }
                    else if((str_idx == STR_PROTECT) && ((second_sub_menu_item.index + second_sub_menu_item.str_idx_start) == STR_OK))
                    {
                        g_setting_reply_action_flag = 0;    //wait task_storage_service to send ack

                        if((sub_menu_item.index + sub_menu_item.str_idx_start) == STR_LOCK_CURRENT)
                        {
                            storage_task_msg_send(MSG_STORAGE_SERVICE_LOCK_ONE, NULL, 0, MSG_PRI_NORMAL);
                        }
                        else if((sub_menu_item.index + sub_menu_item.str_idx_start) == STR_UNLOCK_CURRENT)
                        {
                            storage_task_msg_send(MSG_STORAGE_SERVICE_UNLOCK_ONE, NULL, 0, MSG_PRI_NORMAL);
                        }
                        else if((sub_menu_item.index + sub_menu_item.str_idx_start) == STR_LOCK_ALL)
                        {
                            storage_task_msg_send(MSG_STORAGE_SERVICE_LOCK_ALL, NULL, 0, MSG_PRI_NORMAL);
                        }
                        else if((sub_menu_item.index + sub_menu_item.str_idx_start) == STR_UNLOCK_ALL)
                        {
                            storage_task_msg_send(MSG_STORAGE_SERVICE_UNLOCK_ALL, NULL, 0, MSG_PRI_NORMAL);
                        }
                    }

                    menu_level = MENU_LEVEL_SUB_MENU;
                }
            }
        }
    }
}

void ap_setting_right_menu_active(INT8U act_type)
{
    SETTING_ITEM  *setting_tbl;
    INT16U  item, str_idx;
    TIME_T  g_time;

    if(main_menu_item)
    {
        item = main_menu_item - 1;  //start with 0
        calculate_total_item_and_current_page(&setting_tbl, NULL, NULL);
        str_idx = setting_tbl[item].str_idx;

        if(act_type == SUB_MENU_DRAW)
        {
            menu_level = MENU_LEVEL_SUB_MENU;

            if(setting_tbl[item].config_get)
            {
                sub_menu_item.index = setting_tbl[item].config_get();
            }
            else
            {
                sub_menu_item.index = 0;
            }

            sub_menu_item.str_idx_start = setting_tbl[item].sub_str_start;
            sub_menu_item.item_max = setting_tbl[item].sub_item_num;

            if(str_idx == STR_RESOLUTION)   //record resolution
            {
                INT32U temp;

                temp = STR_4K + sub_menu_item.index;
                if(temp < sub_menu_item.str_idx_start)  temp = sub_menu_item.str_idx_start;
                else if(temp >= (sub_menu_item.str_idx_start + sub_menu_item.item_max))   temp = sub_menu_item.str_idx_start + sub_menu_item.item_max - 1;

                sub_menu_item.index = temp - sub_menu_item.str_idx_start;
            }
            else
            if(str_idx == STR_DATE_TIME2)
            {
                ap_peripheral_osd_time_get(&g_time);
                setup_date_time[6] = ap_state_config_date_time_mode_get();
                setup_date_time[0] = g_time.tm_year - 2000;
                setup_date_time[1] = g_time.tm_mon;
                setup_date_time[2] = g_time.tm_mday;
                setup_date_time[3] = g_time.tm_hour;
                setup_date_time[4] = g_time.tm_min;
                setup_date_time[5] = g_time.tm_sec;
            }
        }
        else if(menu_level == MENU_LEVEL_SUB_MENU)
        {
            if(act_type == SUB_MENU_MOVE_UP)
            {
                if(str_idx == STR_DATE_TIME2)
                {
                    ap_setting_adjust_date_time(SUB_MENU_MOVE_UP);
                    date_time_update_flag = 1;
                }
                else
                {
                    sub_menu_item.index += 1;
                    if(sub_menu_item.index >= sub_menu_item.item_max)
                    {
                        sub_menu_item.index = 0;
                    }
                }
            }
            else if(act_type == SUB_MENU_MOVE_DOWN)
            {
                if(str_idx == STR_DATE_TIME2)
                {
                    ap_setting_adjust_date_time(SUB_MENU_MOVE_DOWN);
                    date_time_update_flag = 1;
                }
                else
                {
                    if(sub_menu_item.index)
                    {
                        sub_menu_item.index -= 1;
                    }
                    else
                    {
                        sub_menu_item.index = sub_menu_item.item_max - 1;
                    }
                }
            }
            else if(act_type == FUNCTION_ACTIVE)
            {
                if(str_idx == STR_DATE_TIME2)
                {
                    sub_menu_item.index += 1;
                    if(sub_menu_item.index >= sub_menu_item.item_max)
                    {
                        sub_menu_item.index = 0;
                    }
                }
                else if((str_idx == STR_DELETE) || (str_idx == STR_PROTECT))
                {
                    ap_setting_second_sub_menu_active(SUB_MENU_DRAW);
                }
                else
                {
                    if(str_idx == STR_RESOLUTION)
                    {
                        INT32U temp;

                        temp = sub_menu_item.index + sub_menu_item.str_idx_start;
                        sub_menu_item.index = temp - STR_4K;
                    }

                    if(setting_tbl[item].config_set)
                    {
                        setting_tbl[item].config_set(sub_menu_item.index);
                    }

                    if((str_idx == STR_DEFAULT_SETTING) && ((sub_menu_item.index + sub_menu_item.str_idx_start) == STR_OK))
                    {
                        ap_state_config_default_set();
                    }

                    if((str_idx == STR_FORMAT) && ((sub_menu_item.index + sub_menu_item.str_idx_start) == STR_OK))
                    {
                        if(ap_state_handling_storage_id_get() != STORAGE_DEVICE)
                        {
                            ap_state_handling_show_no_sd();
                        }
                        else
                        {
                            ap_state_handling_show_busy();

                            g_setting_reply_action_flag = 0;
                            storage_task_msg_send(MSG_STORAGE_SERVICE_FORMAT_REQ, NULL, 0, MSG_PRI_NORMAL);
                        }
                    }

                    if((str_idx == STR_PLATE_NUMBER) && ((sub_menu_item.index + sub_menu_item.str_idx_start) == STR_ON))
                    {
                        ap_setting_second_sub_menu_active(SUB_MENU_DRAW);
                    }
                    else
                    {
                        menu_level = MENU_LEVEL_MAIN_MENU;      //backward
                    }
                }
            }
        }
    }
}


void ap_setting_format_reply(void)
{
    ap_state_handling_warn_str_icon_exit();     //disable warning icon display
}

void ap_setting_other_reply(INT32U msg_id)
{
    /*if(msg_id == MSG_APQ_SELECT_FILE_DEL_REPLY)
    {
        ap_state_handling_show_protected();
    }
    else*/
    {
        ap_state_handling_warn_str_icon_exit();     //disable warning icon display
    }
}

static INT32S ap_setting_mem_transfer_dma(INT32U dest, INT32U src, INT32U len)
{
	DMA_STRUCT		dma_struct = {0};
	volatile INT8S	done;
	INT32S			result;

	done = C_DMA_STATUS_WAITING;
	dma_struct.s_addr = src;
	dma_struct.t_addr = dest;
	dma_struct.width = DMA_DATA_WIDTH_4BYTE;
	dma_struct.count = len >> 2;
	dma_struct.notify = &done;
	dma_struct.timeout = 0;
	result = drv_l1_dma_transfer(&dma_struct);
	if(result)
	{
		DBG_PRINT("DMA fail drv_l1_dma_transfer() result=%d\r\n", result);
		return STATUS_FAIL;
	}

	while(1)
	{
		if(done != C_DMA_STATUS_WAITING)
		{
			break;
		}
	}

	return STATUS_OK;
}

void ap_setting_memcpy(INT32U dest, INT32U src, INT32U len)
{
	if(ap_setting_mem_transfer_dma(dest, src, len) != STATUS_OK)
	{
		gp_memcpy((INT8S*)dest, (INT8S*)src, len);
	}
}

void ap_setting_init(INT32U prev_state)
{
    SETTING_ITEM *pItem = &setting_record_table[0];
    DISP_P showing_frame;

    menu_type = MENU_TYPE_MODE_MENU;
    menu_level = MENU_LEVEL_MAIN_MENU;
    main_menu_item = 0;
    sub_menu_item.index = 0;
    second_sub_menu_item.index = 0;
    date_time_update_flag = 0;
    plate_number_update_flag = 0;

    g_setting_reply_action_flag = 1;

    ap_state_handling_warn_str_icon_exit();     //disable warning icon display

    //modify record resolution menu
    if(drv_l2_user_sensor_width_get(0) == 3840)  //4K sensor, 3840x2160
    {
        pItem->sub_str_start = STR_4K;
        pItem->sub_item_num = 3;                 		//4K, 2K, 1080FHD
    }
    else if(drv_l2_user_sensor_width_get(0) == 2560)  	//2K sensor, 2560x1440
    {
        pItem->sub_str_start = STR_4K;
        pItem->sub_item_num = 5;                 		//4K, 2K, 1080FHD, 1080P, 720P
    }
    else if(drv_l2_user_sensor_width_get(0) == 1920) 	//1080FHD, 1920x1080
    {
        pItem->sub_str_start = STR_2K;
        pItem->sub_item_num = 4;                 		//2K, 1080FHD, 1080P, 720P
    }
    else if(drv_l2_user_sensor_width_get(0) == 1280) 	//720P sensor, 1280x720
    {
        pItem->sub_str_start = STR_1080FHD;
        pItem->sub_item_num = 3;                 		//1080FHD, 1080P, 720P
    }
    else    											//VGA, 640x480
    {
        pItem->sub_str_start = STR_720P;
        pItem->sub_item_num = 4;                 		//720P, WVGA, VGA, QVGA
    }

    if(prev_state == STATE_VIDEO_RECORD)
    {
        state_str = STR_VIDEO;
        setting_frame_base = NULL;
    }
    else if(prev_state == STATE_PHOTO_CAPTURE)
    {
        state_str = STR_CAPTURE;
        setting_frame_base = NULL;
    }
    else //if(prev_state == STATE_BROWSE)
    {
        state_str = STR_PLAYBACK;

        setting_frame_base = ap_display_queue_get(display_free_queue);
        while(setting_frame_base == NULL)
        {
            osDelay(10);
            setting_frame_base = ap_display_queue_get(display_free_queue);
        }

        setting_frame_base->offset = 0;

        task_display_lock_on(1);
        showing_frame = ap_display_showing_frame_get();
        if(showing_frame)
        {
            ap_setting_memcpy(setting_frame_base->base, showing_frame->base, getDispDevBufSize());
        }
        else
        {
            ap_display_clear_screen(setting_frame_base->base);
        }
        task_display_lock_off(1);
    }

    ap_setting_menu_draw();
}

EXIT_FLAG_ENUM ap_setting_menu_key_active(INT32U prev_state)
{
    SETTING_ITEM  *setting_tbl;
    INT16U  str_idx;

    ap_state_handling_warn_str_icon_exit();

    if(menu_level == MENU_LEVEL_SECOND_SUB_MENU)
    {
        calculate_total_item_and_current_page(&setting_tbl, NULL, NULL);
        str_idx = setting_tbl[main_menu_item - 1].str_idx;

        if(str_idx == STR_PLATE_NUMBER)
        {
            if(plate_number_update_flag)
            {
                ap_state_config_car_plate_number_set(&setup_plate_number[0]);
                plate_number_update_flag = 0;
            }

            menu_level = MENU_LEVEL_MAIN_MENU;          //backward to main menu
        }
        else
        {
            menu_level = MENU_LEVEL_SUB_MENU;           //backward to sub menu
        }
    }
    else if(menu_level == MENU_LEVEL_SUB_MENU)
    {
        // date/time update
        if(date_time_update_flag)
        {
            TIME_T g_time;

            g_time.tm_year = setup_date_time[0] + 2000;
            g_time.tm_mon = setup_date_time[1];
            g_time.tm_mday = setup_date_time[2];
            g_time.tm_hour = setup_date_time[3];
            g_time.tm_min = setup_date_time[4];
            g_time.tm_sec = setup_date_time[5];
            cal_time_set(g_time);

            ap_state_config_date_time_mode_set(setup_date_time[6]);
            date_time_update_flag = 0;
        }

        menu_level = MENU_LEVEL_MAIN_MENU;          //backward to main menu
    }
    else if(menu_level == MENU_LEVEL_MAIN_MENU)     //main menu state
    {
        if(menu_type == MENU_TYPE_MODE_MENU)        //switch to system menu
        {
            menu_type = MENU_TYPE_SYS_MENU;
            menu_level = MENU_LEVEL_MAIN_MENU;
            main_menu_item = 0;
            sub_menu_item.index = 0;
            second_sub_menu_item.index = 0;
        }
        else
        {
            ap_state_config_store();
            state_handling_msg_send(prev_state, NULL, 0, MSG_PRI_NORMAL);
			return EXIT_BREAK;
        }
    }

    ap_setting_menu_draw();
    return EXIT_RESUME;
}

EXIT_FLAG_ENUM ap_setting_mode_key_active(INT32U prev_state)
{
    ap_state_handling_warn_str_icon_exit();

    // date/time update
    if(date_time_update_flag)
    {
        TIME_T g_time;

        g_time.tm_year = setup_date_time[0] + 2000;
        g_time.tm_mon = setup_date_time[1];
        g_time.tm_mday = setup_date_time[2];
        g_time.tm_hour = setup_date_time[3];
        g_time.tm_min = setup_date_time[4];
        g_time.tm_sec = setup_date_time[5];
        cal_time_set(g_time);

        ap_state_config_date_time_mode_set(setup_date_time[6]);
        date_time_update_flag = 0;
    }

    if(plate_number_update_flag)
    {
        ap_state_config_car_plate_number_set(&setup_plate_number[0]);
        plate_number_update_flag = 0;
    }

    ap_state_config_store();
	state_handling_msg_send(prev_state, NULL, 0, MSG_PRI_NORMAL);
    return EXIT_BREAK;
}


void ap_setting_func_key_active(void)
{
    ap_state_handling_warn_str_icon_exit();

    if(menu_level == MENU_LEVEL_MAIN_MENU)
    {
        ap_setting_right_menu_active(SUB_MENU_DRAW);
    }
    else if(menu_level == MENU_LEVEL_SUB_MENU)
    {
        ap_setting_right_menu_active(FUNCTION_ACTIVE);
    }
    else if(menu_level == MENU_LEVEL_SECOND_SUB_MENU)
    {
        ap_setting_second_sub_menu_active(FUNCTION_ACTIVE);
    }

    ap_setting_menu_draw();
}

void ap_setting_direction_key_active(INT32U msg_id)
{
    INT16U  num_item;

    ap_state_handling_warn_str_icon_exit();
    calculate_total_item_and_current_page(NULL, &num_item, NULL);

    if(msg_id == MSG_APQ_PREV_KEY_ACTIVE)
    {
        if(menu_level == MENU_LEVEL_MAIN_MENU)
        {
            if(!main_menu_item) main_menu_item = 1;
            else if(main_menu_item > 1)  main_menu_item--;
            else main_menu_item = num_item;
        }
        else if(menu_level == MENU_LEVEL_SUB_MENU)
        {
            ap_setting_right_menu_active(SUB_MENU_MOVE_DOWN);
        }
        else if(menu_level == MENU_LEVEL_SECOND_SUB_MENU)
        {
            ap_setting_second_sub_menu_active(SUB_MENU_MOVE_DOWN);
        }
    }
    else //MSG_APQ_NEXT_KEY_ACTIVE
    {
        if(menu_level == MENU_LEVEL_MAIN_MENU)
        {
            if(!main_menu_item) main_menu_item = 1;
            else if(main_menu_item < num_item)  main_menu_item++;
            else main_menu_item = 1;
        }
        else if(menu_level == MENU_LEVEL_SUB_MENU)
        {
            ap_setting_right_menu_active(SUB_MENU_MOVE_UP);
        }
        else if(menu_level == MENU_LEVEL_SECOND_SUB_MENU)
        {
            ap_setting_second_sub_menu_active(SUB_MENU_MOVE_UP);
        }
    }

    ap_setting_menu_draw();
    return;
}

void ap_setting_exit(INT32U prev_state)
{
    if(setting_frame_base)
    {
        ap_display_queue_put(display_free_queue, setting_frame_base);
        setting_frame_base = NULL;
    }

	DBG_PRINT("setting exit\r\n");
}


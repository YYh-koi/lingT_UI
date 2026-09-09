#include "application.h"

INT32U				CRC32_tbl[256];
ALIGN64 SYSTEM_USER_OPTION	Global_User_Optins;
USER_ITEMS			user_item;

//	prototypes
void				ap_state_config_default_set(void);
extern INT32S		ext_rtc_time_set(t_rtc *rtc_time);

void ap_state_config_reversing_display_mode_set(INT8U mode);

void ap_state_config_initial(INT32S status)
{
	//default setting
	Global_User_Optins.item.ifdirty = 0;
	if(status == STATUS_FAIL)
	{	/* check nvdb initial status */
		ap_state_config_default_set();
		{
			t_rtc	time;

			time.rtc_day = 0;
			time.rtc_hour = 0;
			time.rtc_min = 0;
			time.rtc_sec = 0;
			ext_rtc_time_set(&time);
		}
	}
}

void ap_state_config_default_set(void)
{
    INT8U   temp[9];

	//some default config is 0
	gp_memset((INT8S *) &Global_User_Optins, 0x00, sizeof(SYSTEM_USER_OPTION));
	//ap_state_resource_user_option_load(&Global_User_Optins);  //SYSTEM_USER_OPTION struct tool using is different from
                                                                // that current code using, so this function is useless
    ap_state_config_language_set(LCD_SCH);
	ap_state_config_voice_record_switch_set(1); //mic on
	ap_state_config_video_resolution_set(4);	//0:4K, 1:2K, 2:1080FHD, 3:1080P, 4:720P

	ap_state_config_record_time_set(2);			//0:off, 1:1min, 2:2min, 3:3min, 4:5min, 5:10min
	ap_state_config_capture_burst_set(0);		//off
	ap_state_config_record_ev_set(6);
	ap_state_config_capture_ev_set(6);
	ap_state_config_beep_sound_set(1);
	ap_state_config_record_date_stamp_set(1);   //only 0 or 1 is acceptable
	ap_state_config_capture_date_stamp_set(2);  //0, 1 or 2 is acceptable

	ap_state_config_display_mode_set(PREVIEW_MODE_PIP_REAR);
	ap_state_config_volume_set(5);

	ap_state_config_plate_number_onoff_set(0);

    temp[0] = STR_MAX + 27;     //Guangdong
    temp[1] = 'B';              //Shenzhen
    temp[2] = '8';
    temp[3] = '8';
    temp[4] = '8';
    temp[5] = '8';
    temp[6] = '8';
    temp[7] = 0;                //reserved for electrical car id
    temp[8] = 0;                //string end
    ap_state_config_car_plate_number_set(&temp[0]);

#if defined(ENABLE_CHECK_RTC) && ENABLE_CHECK_RTC == 1
	ap_state_config_date_time_save_set_default();
#endif

#if C_TIME_LAPSE
    ap_state_config_time_lapse_recording_set(0);
#endif
	ap_state_config_reversing_display_mode_set(0);
#if C_DISP_SLIDE_SAVE
	ap_state_config_crop_slide_load(0);
#endif
	Global_User_Optins.item.ifdirty = 1;
}

void ap_state_config_store(void)
{
#if C_DISP_SLIDE_SAVE
 	if(ap_state_config_crop_slide_flag_get())
	{
		Global_User_Optins.item.ifdirty = 1;
		ap_state_config_crop_slide_get(PREVIEW_SRC_TOTAL);
	}
	if(ap_state_config_crop_slide_reversing_flag_get())
	{
		Global_User_Optins.item.ifdirty = 1;
		ap_state_config_crop_reversing_get();
	}
#endif
	if(Global_User_Optins.item.ifdirty == 1)
	{
		/* check SPIFC exist or not */
		///volatile INT32U *pSYM = (volatile INT32U *) (0xB0000000);
		///if(*pSYM == 0x564E5047)					// "GPNV" 0x564E5047
		{
			/* calculate CRC */
			CRC_cal((INT8U *) &Global_User_Optins.item, sizeof(USER_ITEMS), Global_User_Optins.crc);
			DBG_PRINT("store CRC = %02x %02x %02x %02x\r\n", Global_User_Optins.crc[0], Global_User_Optins.crc[1], Global_User_Optins.crc[2], Global_User_Optins.crc[3]);
			nvmemory_user_sector_store(0, (INT32U *) &Global_User_Optins, 1);
			gp_memcpy((INT8S *) &user_item, (INT8S *) &Global_User_Optins.item, sizeof(USER_ITEMS));
			nvmemory_user_sector_load(0, (INT32U *) &Global_User_Optins, 1);
			if(gp_memcmp((INT8S *) &user_item, (INT8S *) &Global_User_Optins.item, sizeof(USER_ITEMS)) != 0)
			{
				DBG_PRINT("verify failed, store again\r\n");
				gp_memcpy((INT8S *) &Global_User_Optins.item, (INT8S *) &user_item, sizeof(USER_ITEMS));
				CRC_cal((INT8U *) &Global_User_Optins.item, sizeof(USER_ITEMS), Global_User_Optins.crc);
				nvmemory_user_sector_store(0, (INT32U *) &Global_User_Optins, 1);
			}
		}

		Global_User_Optins.item.ifdirty = 0;
	}
}

INT32S ap_state_config_load(void)
{
	INT8U	expect_crc[4];

	nvmemory_user_sector_load(0, (INT32U *) &Global_User_Optins, 1);	//load user setting
	DBG_PRINT("load CRC = %02x %02x %02x %02x\r\n", Global_User_Optins.crc[0], Global_User_Optins.crc[1], Global_User_Optins.crc[2], Global_User_Optins.crc[3]);

	/* calculate expect CRC */
	CRC_cal((INT8U *) &Global_User_Optins.item, sizeof(USER_ITEMS), expect_crc);
	DBG_PRINT("expect CRC = %02x %02x %02x %02x\r\n", expect_crc[0], expect_crc[1], expect_crc[2], expect_crc[3]);

	/* compare CRC and expect CRC */
	if(gp_memcmp((INT8S *) expect_crc, (INT8S *) Global_User_Optins.crc, 4) != 0)
	{
		DBG_PRINT("crc error\r\n");
		return STATUS_FAIL; /* CRC error */
	}
#if C_DISP_SLIDE_SAVE
	ap_state_config_crop_slide_load(1);
#endif
	return STATUS_OK;
}


/*----------------------------------------------*/
void ap_state_config_factory_date_get(INT8U *date)
{
	*date++ = Global_User_Optins.item.factory_date[0];
	*date++ = Global_User_Optins.item.factory_date[1];
	*date = Global_User_Optins.item.factory_date[2];
}

void ap_state_config_factory_time_get(INT8U *time)
{
	*time++ = Global_User_Optins.item.factory_time[0];
	*time++ = Global_User_Optins.item.factory_time[1];
	*time = Global_User_Optins.item.factory_time[2];
}

/*----------------------------------------------*/
INT32U ap_state_config_base_day_get(void)
{
	return Global_User_Optins.item.base_day;
}

void ap_state_config_base_day_set(INT32U day)
{
	if(day != ap_state_config_base_day_get())
	{
		Global_User_Optins.item.base_day = day;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_volume_get(void)
{
	return Global_User_Optins.item.sound_volume;
}

void ap_state_config_volume_set(INT8U sound_volume)
{
	if(sound_volume != ap_state_config_volume_get())
	{
		Global_User_Optins.item.sound_volume = sound_volume;

		audio_vol_set(sound_volume);
		Global_User_Optins.item.ifdirty = 1;
	}
}


// For system settings

/*----------------------------------------------*/
#if C_GSENSOR_FUNCTION == CUSTOM_ON
INT8U ap_state_config_park_mode_G_sensor_get(void)
{
	return Global_User_Optins.item.park_gsensor;
}

void ap_state_config_park_mode_G_sensor_set(INT8U Gsensor)
{
	if(Gsensor != ap_state_config_park_mode_G_sensor_get())
	{
		Global_User_Optins.item.park_gsensor = Gsensor;     //park mode gsensor on/off

		drv_l2_gsensor_sensitive_set(Gsensor);
		Global_User_Optins.item.ifdirty = 1;
	}
}
#endif

/*----------------------------------------------*/
INT8U ap_state_config_auto_off_get(void)
{
	return Global_User_Optins.item.auto_off;
}

void ap_state_config_auto_off_set(INT8U auto_off)
{
	if(auto_off != ap_state_config_auto_off_get())
	{
		Global_User_Optins.item.auto_off = auto_off;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_auto_off_TFT_BL_get(void)
{
	return Global_User_Optins.item.screen_save;
}

void ap_state_config_auto_off_TFT_BL_set(INT8U auto_off_TFT_BL)
{
	if(auto_off_TFT_BL != ap_state_config_auto_off_TFT_BL_get())
	{
		Global_User_Optins.item.screen_save = auto_off_TFT_BL;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_beep_sound_get(void)
{
	return Global_User_Optins.item.beep_sound;
}

void ap_state_config_beep_sound_set(INT8U off)
{
	if(off != ap_state_config_beep_sound_get())
	{
		Global_User_Optins.item.beep_sound = off;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_language_get(void)
{
    return Global_User_Optins.item.language;
}

void ap_state_config_language_set(INT8U language)
{
	if(language != ap_state_config_language_get())
	{
		Global_User_Optins.item.language = language;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_light_freq_get(void)
{
	return Global_User_Optins.item.light_freq;
}

void ap_state_config_light_freq_set(INT8U light_freq)
{
	if(light_freq != ap_state_config_light_freq_get())
	{
		Global_User_Optins.item.light_freq = light_freq;

		drv_l2_user_sensor_frequence(light_freq);
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_flash_LED_get(void)
{
	return Global_User_Optins.item.flash_led;
}

void ap_state_config_flash_LED_set(INT8U off)
{
	if(off != ap_state_config_flash_LED_get())
	{
		Global_User_Optins.item.flash_led = off;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_date_time_mode_get(void)
{
	return Global_User_Optins.item.date_time_mode;
}

void ap_state_config_date_time_mode_set(INT8U dis_mode)
{
	if(dis_mode != ap_state_config_date_time_mode_get())
	{
		Global_User_Optins.item.date_time_mode = dis_mode;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_usb_mode_get(void)
{
	return Global_User_Optins.item.usb_mode;
}

void ap_state_config_usb_mode_set(INT8U usb_mode)
{
	if(usb_mode != ap_state_config_usb_mode_get())
	{
		Global_User_Optins.item.usb_mode = usb_mode;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_plate_number_onoff_get(void)
{
	return Global_User_Optins.item.plate_number_onoff;
}

void ap_state_config_plate_number_onoff_set(INT8U onoff)
{
	if(onoff != ap_state_config_plate_number_onoff_get())
	{
		Global_User_Optins.item.plate_number_onoff = onoff;   //G sensor level
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U *ap_state_config_car_plate_number_get(void)
{
	return &Global_User_Optins.item.car_plate_number[0];
}

void ap_state_config_car_plate_number_set(INT8U *input)
{
	INT16U i;

	for(i = 0; i < 8; i++)
	{
		if(input[i] != Global_User_Optins.item.car_plate_number[i])
		{
			Global_User_Optins.item.car_plate_number[i] = input[i];
			Global_User_Optins.item.ifdirty = 1;
		}
	}

	if(Global_User_Optins.item.car_plate_number[8] != 0)
	{
		Global_User_Optins.item.car_plate_number[8] = 0;
		Global_User_Optins.item.ifdirty = 1;
	}
}


// For photo capture settings

/*----------------------------------------------*/
INT8U ap_state_config_pic_size_get(void)
{
	return Global_User_Optins.item.capture_pic_size;
}

extern INT32U	last_pic_size;					//unit: KB
extern INT32U	last_pic_size_temp;
void ap_state_config_pic_size_set(INT8U pic_size)
{
	if(pic_size != ap_state_config_pic_size_get())
	{
		Global_User_Optins.item.capture_pic_size = pic_size;
		Global_User_Optins.item.ifdirty = 1;

		last_pic_size = 0;
		last_pic_size_temp = 0;
	}
}
/*----------------------------------------------*/
INT8U ap_state_config_capture_burst_get(void)
{
	return Global_User_Optins.item.capture_burst;
}

void ap_state_config_capture_burst_set(INT8U burst)
{
	if(burst != ap_state_config_capture_burst_get())
	{
		Global_User_Optins.item.capture_burst = burst;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_quality_get(void)
{
	return Global_User_Optins.item.capture_quality;
}

void ap_state_config_quality_set(INT8U quality)
{
	if(quality != ap_state_config_quality_get())
	{
		Global_User_Optins.item.capture_quality = quality;
		Global_User_Optins.item.ifdirty = 1;

		last_pic_size = 0;
		last_pic_size_temp = 0;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_sharpness_get(void)
{
	return Global_User_Optins.item.capture_sharpness;
}

void ap_state_config_sharpness_set(INT8U sharpness)
{
	if(sharpness != ap_state_config_sharpness_get())
	{
		Global_User_Optins.item.capture_sharpness = sharpness;

		drv_l2_cdsp_edge_set_by_config(sharpness);
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_white_balance_get(void)
{
	return Global_User_Optins.item.capture_wb;
}


void ap_state_config_white_balance_set(INT8U white_balance)
{
	if(white_balance != ap_state_config_white_balance_get())
	{
		Global_User_Optins.item.capture_wb = white_balance;

		drv_l2_gp_cdsp_set_awb_by_config(white_balance);
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_iso_get(void)
{
	return Global_User_Optins.item.capture_iso;
}

void ap_state_config_iso_set(INT8U iso)
{
	if(iso != ap_state_config_iso_get())
	{
		Global_User_Optins.item.capture_iso = iso;

		if(Global_User_Optins.item.anti_shaking == 1)
			drv_l2_cdsp_set_iso(4);
		else
			drv_l2_cdsp_set_iso(iso);

		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_capture_ev_get(void)
{
	return Global_User_Optins.item.capture_ev;
}

void ap_state_config_capture_ev_set(INT8U ev)
{
	if(ev != ap_state_config_capture_ev_get())
	{
		Global_User_Optins.item.capture_ev = ev;

		drv_l2_cdsp_set_ev_val(ev);
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_anti_shaking_get(void)
{
	return Global_User_Optins.item.anti_shaking;
}

void ap_state_config_anti_shaking_set(INT8U onoff)
{
	if(onoff != ap_state_config_anti_shaking_get())
	{
		Global_User_Optins.item.anti_shaking = onoff;
		if(Global_User_Optins.item.anti_shaking == 1)
		{
			drv_l2_cdsp_set_iso(4);
		}
		else
		{
			if((1 == ap_state_config_iso_get()) || (0 == ap_state_config_iso_get()))
			{
				drv_l2_cdsp_set_iso(1);
			}
			else if(2 == ap_state_config_iso_get())
			{
				drv_l2_cdsp_set_iso(2);
			}
		}

		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_capture_review_get(void)
{
	return Global_User_Optins.item.capture_review;
}


void ap_state_config_capture_review_set(INT8U preview)
{
	if(preview != ap_state_config_capture_review_get())
	{
		Global_User_Optins.item.capture_review = preview;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_capture_date_stamp_get(void)
{
	return Global_User_Optins.item.capture_datestamp;
}

void ap_state_config_capture_date_stamp_set(INT8U date_stamp)
{
	if(date_stamp != ap_state_config_capture_date_stamp_get())
	{
		Global_User_Optins.item.capture_datestamp = date_stamp;
		Global_User_Optins.item.ifdirty = 1;
	}
}


// For video record settings

/*----------------------------------------------*/
INT8U ap_state_config_video_resolution_get(void)
{
	return Global_User_Optins.item.video_resolution;
}

void ap_state_config_video_resolution_set(INT8U resolution)
{
	if(resolution != ap_state_config_video_resolution_get())
	{
		Global_User_Optins.item.video_resolution = resolution;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_record_time_get(void)
{
	return Global_User_Optins.item.record_time;
}

void ap_state_config_record_time_set(INT8U record_time)
{
	if(record_time != ap_state_config_record_time_get())
	{
		Global_User_Optins.item.record_time = record_time;
		Global_User_Optins.item.ifdirty = 1;
	}
}

INT8U ap_state_config_record_minute_get(void)
{
	const INT8U temp[6] = { 0, 1, 2, 3, 5, 10 };
	INT8U minute = temp[ap_state_config_record_time_get()];

	if(minute == 0)
		return 5;
	else
		return minute;
}

/*----------------------------------------------*/
INT8U ap_state_config_record_ev_get(void)
{
	return Global_User_Optins.item.record_ev;
}

// 0:+2, 1:+5/3, 2:+4/3, 3:+1.0, 4:+2/3, 5:+1/3, 6:+0.0, 7:-1/3, 8:-2/3, 9:-1.0, 10:-4/3, 11:-5/3, 12:-2.0
void ap_state_config_record_ev_set(INT8U ev)
{
	if(ev != ap_state_config_record_ev_get())
	{
		Global_User_Optins.item.record_ev = ev;

		drv_l2_cdsp_set_ev_val(ev);
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_md_get(void)
{
	return Global_User_Optins.item.motion_det;
}

void ap_state_config_md_set(INT8U md)
{
	if(md != ap_state_config_md_get())
	{
		Global_User_Optins.item.motion_det = md;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_voice_record_switch_get(void)
{
	return Global_User_Optins.item.record_voice;
}

void ap_state_config_voice_record_switch_set(INT8U mic_switch)
{
	if(mic_switch != ap_state_config_voice_record_switch_get())
	{
		Global_User_Optins.item.record_voice = mic_switch;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_record_date_stamp_get(void)
{
	return Global_User_Optins.item.record_datestamp;
}

void ap_state_config_record_date_stamp_set(INT8U date_stamp)
{
	if(date_stamp != ap_state_config_record_date_stamp_get())
	{
		Global_User_Optins.item.record_datestamp = date_stamp;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
#if C_GSENSOR_FUNCTION == CUSTOM_ON
INT8U ap_state_config_record_G_sensor_get(void)
{
	return Global_User_Optins.item.record_gsensor;
}

void ap_state_config_record_G_sensor_set(INT8U Gsensor)
{
	if(Gsensor != ap_state_config_record_G_sensor_get())
	{
		Global_User_Optins.item.record_gsensor = Gsensor;   //G sensor level
		Global_User_Optins.item.ifdirty = 1;

		drv_l2_gsensor_sensitive_set(Gsensor);
	}
}
#endif

/*----------------------------------------------*/
INT8U ap_state_config_display_mode_get(void)
{
	return Global_User_Optins.item.pip_mode;
}

void ap_state_config_display_mode_set(INT8U dis_mode)
{
	if(dis_mode != ap_state_config_display_mode_get())
	{
		Global_User_Optins.item.pip_mode = dis_mode;
		Global_User_Optins.item.ifdirty = 1;
	}
}

/*----------------------------------------------*/
INT8U ap_state_config_draw_line_mode_get(void)
{
	return Global_User_Optins.item.back_car_line;
}

void ap_state_config_draw_line_mode_set(INT8U mode)
{
	if(mode != ap_state_config_draw_line_mode_get())
	{
		Global_User_Optins.item.back_car_line = mode;
		Global_User_Optins.item.ifdirty = 1;
	}
}

void CRC32tbl_init(void)
{
	INT32U	i, j;
	INT32U	c;

	for(i = 0; i < 256; ++i)
	{
		c = i;
		for(j = 0; j < 8; j++)
		{
			c = (c & 1) ? (c >> 1) ^ CRC32_POLY : (c >> 1);
		}

		CRC32_tbl[i] = c;
	}
}

void CRC_cal(INT8U *pucBuf, INT32U len, INT8U *CRC)
{
	INT32U	i_CRC, value;
	INT32U	i;

	if(!CRC32_tbl[1])
	{
		CRC32tbl_init();
	}

	i_CRC = 0xFFFFFFFF;
	for(i = 0; i < len; i++)
	{
		i_CRC = (i_CRC >> 8) ^ CRC32_tbl[(i_CRC ^ pucBuf[i]) & 0xFF];
	}

	value = ~i_CRC;
	CRC[0] = (value & 0x000000FF) >> 0;
	CRC[1] = (value & 0x0000FF00) >> 8;
	CRC[2] = (value & 0x00FF0000) >> 16;
	CRC[3] = (value & 0xFF000000) >> 24;
}


#if defined(ENABLE_CHECK_RTC) && ENABLE_CHECK_RTC == 1
extern INT32S	ext_rtc_time_get(t_rtc *rtc_time);
extern INT64U	ext_rtc_time_t_rtc_to_sec(t_rtc *rtc_time);

void ap_state_config_date_time_save_set(void)
{
	INT8U	num_tmp0;
	t_rtc	time;
	INT64U	sec1;

	ext_rtc_time_get(&time);
	DBG_PRINT("t_rtc.RTC_day = %d, ", time.rtc_day);
	DBG_PRINT("t_rtc.RTC_hour = %d, ", time.rtc_hour);
	DBG_PRINT("t_rtc.RTC_min = %d, ", time.rtc_min);
	DBG_PRINT("t_rtc.RTC_sec = %d\r\n", time.rtc_sec);

	sec1 = ext_rtc_time_t_rtc_to_sec(&time);
	DBG_PRINT("sec1 = 0x%x\r\n", sec1);

	num_tmp0 = (INT8U) ((time.rtc_day >> 24) & 0xff);
	Global_User_Optins.item.powertime_onoff_struct2.on_hour = num_tmp0;

	num_tmp0 = (INT8U) ((time.rtc_day >> 16) & 0xff);
	Global_User_Optins.item.powertime_onoff_struct1.mode = num_tmp0;

	num_tmp0 = (INT8U) ((time.rtc_day >> 8) & 0xff);
	Global_User_Optins.item.powertime_onoff_struct1.on_hour = num_tmp0;

	// DAY
	num_tmp0 = (INT8U) ((time.rtc_day) & 0xff);
	Global_User_Optins.item.powertime_onoff_struct1.on_minute = num_tmp0;

	// HOUR
	num_tmp0 = (INT8U) (time.rtc_hour);
	Global_User_Optins.item.powertime_onoff_struct1.off_hour = num_tmp0;

	// MINUTE
	num_tmp0 = (INT8U) (time.rtc_min);
	Global_User_Optins.item.powertime_onoff_struct1.off_minute = num_tmp0;

	// SECOND
	num_tmp0 = (INT8U) (time.rtc_sec);
	Global_User_Optins.item.powertime_onoff_struct2.mode = num_tmp0;

	Global_User_Optins.item.ifdirty = 1;
}

#define three_month_sec 7776000
#define ten_day_sec		864000

extern INT32S	ext_rtc_time_add_now_and_set(t_rtc *rtc_time);	// rtc_time -> added
extern INT64U	ext_rtc_time_get_value(void);

INT8U			RTC_TEST_SHOW = 0;
INT8U ap_state_config_date_time_check(void)
{
	t_rtc	time;
	INT64U	sec;
	INT64U	sec1;
	INT32S	num0;

	sec = ext_rtc_time_get_value();

	num0 = 0;
	num0 = Global_User_Optins.item.powertime_onoff_struct2.on_hour;
	time.rtc_day = (num0 << 24) & 0xff000000;

	num0 = 0;
	num0 = Global_User_Optins.item.powertime_onoff_struct1.mode;
	time.rtc_day = time.rtc_day | ((num0 << 16) & 0x00ff0000);
	num0 = 0;
	num0 = Global_User_Optins.item.powertime_onoff_struct1.on_hour;
	time.rtc_day = time.rtc_day | ((num0 << 8) & 0x0000ff00);

	// DAY
	num0 = 0;
	num0 = Global_User_Optins.item.powertime_onoff_struct1.on_minute;
	time.rtc_day = time.rtc_day | (num0 & 0x000000ff);

	// HOUR
	num0 = 0;
	num0 |= Global_User_Optins.item.powertime_onoff_struct1.off_hour;
	time.rtc_hour = num0;

	// MINUTE
	num0 = 0;
	num0 |= Global_User_Optins.item.powertime_onoff_struct1.off_minute;
	time.rtc_min = num0;

	// SECOND
	num0 = 0;
	num0 |= Global_User_Optins.item.powertime_onoff_struct2.mode;
	time.rtc_sec = num0;

	DBG_PRINT("t_rtc.RTC_day = %d, ", time.rtc_day);
	DBG_PRINT("t_rtc.RTC_hour = %d, ", time.rtc_hour);
	DBG_PRINT("t_rtc.RTC_min = %d, ", time.rtc_min);
	DBG_PRINT("t_rtc.RTC_sec = %d\r\n", time.rtc_sec);
	sec1 = ext_rtc_time_t_rtc_to_sec(&time);

	DBG_PRINT("sec = 0x%x\r\n", sec);
	DBG_PRINT("sec1 = 0x%x\r\n", sec1);
	if(sec < sec1)
	{	//fail
		if(sec < three_month_sec)
		{
			ext_rtc_time_add_now_and_set(&time);
			DBG_PRINT("calender_restore......___________________aaaaaaaaaaaaaaaa\r\n");
			if(sec1 - sec > 5)
			{
				DBG_PRINT("power_OFF reset RTC\r\n");
				RTC_TEST_SHOW = 1;
			}
			else
			{
				DBG_PRINT("power_ON reset RTC\r\n");
				RTC_TEST_SHOW = 2;
			}
		}
		else
		{
			ext_rtc_time_set(&time);
			DBG_PRINT("calender_restore......_________ err sec__________aaaaaaaaaaaaaaaa\r\n");
			RTC_TEST_SHOW = 3;
			DBG_PRINT("power_ON RTC ERR sec<  more three month\r\n");
		}

		return 1;
	}
	else if(sec > (sec1 + three_month_sec))
	{
		ext_rtc_time_set(&time);
		DBG_PRINT("calender_restore......_________ err sec__________aaaaaaaaaaaaaaaa\r\n");
		RTC_TEST_SHOW = 3;
		DBG_PRINT("power_ON RTC ERR more three month\r\n");
		return 1;
	}
	else
	{	//OK
		RTC_TEST_SHOW = 0;
		return 0;
	}
}

INT8U ap_state_config_date_time_power_off_check(void)
{
	t_rtc	time;
	INT64U	sec;
	INT64U	sec1;
	INT32S	num0;

	sec = ext_rtc_time_get_value();

	num0 = 0;
	num0 = Global_User_Optins.item.powertime_onoff_struct2.on_hour;
	time.rtc_day = (num0 << 24) & 0xff000000;

	num0 = 0;
	num0 = Global_User_Optins.item.powertime_onoff_struct1.mode;
	time.rtc_day = time.rtc_day | ((num0 << 16) & 0x00ff0000);
	num0 = 0;
	num0 = Global_User_Optins.item.powertime_onoff_struct1.on_hour;
	time.rtc_day = time.rtc_day | ((num0 << 8) & 0x0000ff00);

	// DAY
	num0 = 0;
	num0 = Global_User_Optins.item.powertime_onoff_struct1.on_minute;
	time.rtc_day = time.rtc_day | (num0 & 0x000000ff);

	// HOUR
	num0 = 0;
	num0 |= Global_User_Optins.item.powertime_onoff_struct1.off_hour;
	time.rtc_hour = num0;

	// MINUTE
	num0 = 0;
	num0 |= Global_User_Optins.item.powertime_onoff_struct1.off_minute;
	time.rtc_min = num0;

	// SECOND
	num0 = 0;
	num0 |= Global_User_Optins.item.powertime_onoff_struct2.mode;
	time.rtc_sec = num0;

	DBG_PRINT("t_rtc.RTC_day = %d, ", time.rtc_day);
	DBG_PRINT("t_rtc.RTC_hour = %d, ", time.rtc_hour);
	DBG_PRINT("t_rtc.RTC_min = %d, ", time.rtc_min);
	DBG_PRINT("t_rtc.RTC_sec = %d\r\n", time.rtc_sec);
	sec1 = ext_rtc_time_t_rtc_to_sec(&time);

	DBG_PRINT("sec = 0x%x\r\n", sec);
	DBG_PRINT("sec1 = 0x%x\r\n", sec1);
	if(sec > (sec1 + three_month_sec))
	{
		ext_rtc_time_set(&time);
		DBG_PRINT("calender_restore......_________ err sec__________aaaaaaaaaaaaaaaa\r\n");
		return 1;
	}
	else
	{	//OK
		RTC_TEST_SHOW = 0;
		return 0;
	}
}

void ap_state_config_date_time_save_set_default(void)
{
	INT8U	num_tmp0;

	num_tmp0 = 0;
	Global_User_Optins.item.powertime_onoff_struct2.on_hour = num_tmp0;

	Global_User_Optins.item.powertime_onoff_struct1.mode = num_tmp0;

	Global_User_Optins.item.powertime_onoff_struct1.on_hour = num_tmp0;

	// DAY
	Global_User_Optins.item.powertime_onoff_struct1.on_minute = num_tmp0;

	// HOUR
	Global_User_Optins.item.powertime_onoff_struct1.off_hour = num_tmp0;

	// MINUTE
	Global_User_Optins.item.powertime_onoff_struct1.off_minute = num_tmp0;

	// SECOND
	Global_User_Optins.item.powertime_onoff_struct2.mode = num_tmp0;

	Global_User_Optins.item.ifdirty = 1;
}

#endif // defined(ENABLE_CHECK_RTC) && ENABLE_CHECK_RTC == 1


#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
void ap_state_wifi_ssid_set(INT8U* wifi_ssid, INT8U wifi_ssid_len)
{
	if(wifi_ssid == NULL || wifi_ssid_len == 0 || (wifi_ssid_len > GPSOCK_WiFi_Name_Length))
	{
		return;
	}
	memset(&Global_User_Optins.item.wifi_ssid, 0, sizeof(Global_User_Optins.item.wifi_ssid));
	memcpy(&Global_User_Optins.item.wifi_ssid, wifi_ssid, wifi_ssid_len);
	Global_User_Optins.item.wifi_ssid_len = wifi_ssid_len;
	Global_User_Optins.item.wifi_ssid_maclen = 0;

	print_string("wifi_ssid_set %s len %d\r\n", wifi_ssid, wifi_ssid_len);
	Global_User_Optins.item.ifdirty = 1;
}

void ap_state_wifi_ssid_get(INT8U* wifi_ssid, INT8U* wifi_ssid_len)
{
	if(Global_User_Optins.item.wifi_ssid_len == 0)
	{
		return;
	}

	INT8U len = Global_User_Optins.item.wifi_ssid_len;
	memcpy(wifi_ssid, &Global_User_Optins.item.wifi_ssid, len);
	*(INT8U*)wifi_ssid_len = len;
}

void ap_state_wifi_ssid_maclen_set(INT8U ssid_maclen)
{
	if(ssid_maclen > 12)
	{
		print_string("ssid_maclen fail len %d\r\n", ssid_maclen);
		return;
	}

	Global_User_Optins.item.wifi_ssid_maclen = ssid_maclen;

	Global_User_Optins.item.ifdirty = 1;
}

void ap_state_wifi_ssid_maclen_get(INT8U* ssid_maclen)
{
    *ssid_maclen = Global_User_Optins.item.wifi_ssid_maclen;
}

void ap_state_wifi_password_set(INT8U* wifi_password, INT8U wifi_password_len)
{
	if(wifi_password == NULL || (wifi_password_len > 1 && wifi_password_len < 8) || (wifi_password_len > GPSOCK_WiFi_Passwd_Length))
	{
		print_string("password_set fail len %d\r\n", wifi_password_len);
		return;
	}
	memset(&Global_User_Optins.item.wifi_pw, 0, sizeof(Global_User_Optins.item.wifi_pw));
	memcpy(&Global_User_Optins.item.wifi_pw, wifi_password, wifi_password_len);
	Global_User_Optins.item.wifi_pw_len = wifi_password_len;

	print_string("wifi_password_set %s len %d\n", wifi_password, wifi_password_len);
	Global_User_Optins.item.ifdirty = 1;
}

void ap_state_wifi_password_get(INT8U* wifi_password, INT8U* wifi_password_len)
{
	if(Global_User_Optins.item.wifi_pw_len == 0)
	{
		return;
	}

	INT8U len = Global_User_Optins.item.wifi_pw_len;
	memcpy(wifi_password, &Global_User_Optins.item.wifi_pw, len);
	*(INT8U*)wifi_password_len = len;
}

void ap_state_wifi_channel_set(INT8U channel)
{
	if(channel > 13)
	{
		return;
	}

	Global_User_Optins.item.wifi_channel = channel;

	Global_User_Optins.item.ifdirty = 1;
}

void ap_state_wifi_channel_get(INT8U* channel)
{
	*(INT8U*)channel = Global_User_Optins.item.wifi_channel;
}

void ap_state_wifi_ip_set(INT8U* ip)
{
	if(ip == NULL)
	{
		return;
	}

	memcpy(&Global_User_Optins.item.wifi_IP, ip, 4);

	Global_User_Optins.item.ifdirty = 1;
}

void ap_state_wifi_ip_get(INT8U* ip)
{
	if(ip == NULL)
	{
		return;
	}

	memcpy(ip, &Global_User_Optins.item.wifi_IP, 4);
}
#endif //	#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)

void ap_state_config_time_lapse_recording_set(INT8U time_lapse)
{
#if C_TIME_LAPSE
	if (time_lapse != ap_state_config_time_lapse_recording_get())
	{
		Global_User_Optins.item.time_lapse = time_lapse;
		Global_User_Optins.item.ifdirty = 1;
		DBG_PRINT("set time_lapse = %d\r\n",time_lapse);
	}
#endif
}

INT8U ap_state_config_time_lapse_recording_get(void)
{
    //DBG_PRINT("get time lpase = %d\r\n",Global_User_Optins.item.time_lapse);
#if C_TIME_LAPSE
	return Global_User_Optins.item.time_lapse;
//    return 1;
#else
	return 0;
#endif
	//return 0;
	//0:none
	//1:1 sec
	//2:2 sec
	//3:5 sec
	//4:10 sec
	//5:30 sec
	//6:1 min
}
INT8U ap_state_config_time_lapse_recording_temp_get(void)
{
#if C_TIME_LAPSE
	INT8U temp[]={0,1,2,5,10,30,60};
	return temp[Global_User_Optins.item.time_lapse];
#else
	return 0;
#endif
	//return 0;
	//0:none
	//1:1 sec
	//2:2 sec
	//3:5 sec
	//4:10 sec
	//5:30 sec
	//6:1 min
}
INT8U ap_state_config_time_lapse_fps_get(void)
{
#if C_TIME_LAPSE
	return 25;//time_lapse_fps;
#else
	return 25;
#endif
}

//间隔录影帧率：25/30
void ap_state_config_time_lapse_fps_set(INT8U fps)
{
#if C_TIME_LAPSE
    //time_lapse_fps = fps;
#endif
}

INT8U ap_state_config_reversing_display_mode_get(void)
{
	return Global_User_Optins.item.reversing_display;
}

//倒车显示模式:0:全屏不变形,1:全屏变形
void ap_state_config_reversing_display_mode_set(INT8U mode)
{
	if(mode <= 2)
		Global_User_Optins.item.reversing_display = mode;
}
#if C_DISP_SLIDE_SAVE

extern void sensor_preview_out_crop_flag_get(INT8U *crop_flag);
extern void sensor_preview_out_crop_flag_set(INT8U crop_flag);
extern void sensor_preview_out_crop_reversing_flag_get(INT8U *crop_flag);
extern void sensor_preview_out_crop_reversing_flag_set(INT8U crop_flag);
extern void sensor_preview_out_crop_reversing_set(INT16U out_w_start, INT16U out_h_start, INT16U out_w_start_next, INT16U out_h_start_next);
extern void sensor_preview_out_crop_reversing_get(INT16U *out_w_start,	INT16U *out_h_start, INT16U *out_w_start_next, INT16U *out_h_start_next);
extern void sensor_preview_out_crop_set_init(INT32U src,
                                    INT8U out_crop,
                                    /*INT8U slide_flag, */
                                    INT16U out_off_x,
                                    INT16U out_off_y,
                                    INT16U out_w_start,
                                    INT16U out_h_start,
                                    INT16U out_w_start_next,
                                    INT16U out_h_start_next
);

INT16U ap_state_config_crop_slide_width_get(void)
{
	return Global_User_Optins.item.crop_w;
}

INT16U ap_state_config_crop_slide_height_get(void)
{
	return Global_User_Optins.item.crop_h;
}

void ap_state_config_crop_slide_width_set(INT16U w)
{
	Global_User_Optins.item.crop_w = w;
}

void ap_state_config_crop_slide_height_set(INT16U h)
{
	Global_User_Optins.item.crop_h = h;
}

INT8U ap_state_config_crop_slide_reversing_flag_get(void)
{
	INT8U crop_flag;
	sensor_preview_out_crop_reversing_flag_get(&crop_flag);
	return crop_flag;
}

void ap_state_config_crop_slide_reversing_flag_set(INT8U flag)
{
	sensor_preview_out_crop_reversing_flag_set(flag);
}

INT8U ap_state_config_crop_slide_flag_get(void)
{
	INT8U crop_flag;
	sensor_preview_out_crop_flag_get(&crop_flag);
	return crop_flag;
}

void ap_state_config_crop_slide_flag_set(INT8U flag)
{
	sensor_preview_out_crop_flag_set(flag);
}

void ap_state_config_crop_slide_get(INT8U src)
{
	if(src < PREVIEW_SRC_TOTAL)
	{
		sensor_preview_out_crop_get(src,
										&Global_User_Optins.item.out_crop[src],
										&Global_User_Optins.item.out_off_x[src],
										&Global_User_Optins.item.out_off_y[src],
										&Global_User_Optins.item.crop_slide_w_start[src],
										&Global_User_Optins.item.crop_slide_h_start[src],
										&Global_User_Optins.item.crop_slide_w_start_next[src],
										&Global_User_Optins.item.crop_slide_h_start_next[src]);
	}
	else//PREVIEW_SRC_TOTAL
	{
		INT8U i;
		for(i=PREVIEW_SRC_FRONT;i<PREVIEW_SRC_TOTAL;i++)
		{
			sensor_preview_out_crop_get(i,
											&Global_User_Optins.item.out_crop[i],
											&Global_User_Optins.item.out_off_x[i],
											&Global_User_Optins.item.out_off_y[i],
											&Global_User_Optins.item.crop_slide_w_start[i],
											&Global_User_Optins.item.crop_slide_h_start[i],
											&Global_User_Optins.item.crop_slide_w_start_next[i],
											&Global_User_Optins.item.crop_slide_h_start_next[i]);
		}
		sensor_preview_out_crop_flag_set(0);
	}

}

void ap_state_config_crop_reversing_get(void)
{
	sensor_preview_out_crop_reversing_get(&Global_User_Optins.item.crop_slide_w_start_reversing,&Global_User_Optins.item.crop_slide_h_start_reversing,
											&Global_User_Optins.item.crop_slide_w_start_reversing_next,&Global_User_Optins.item.crop_slide_h_start_reversing_next);
	sensor_preview_out_crop_reversing_flag_set(0);
}

//set slide from Global_User_Optins to global var
void ap_state_config_crop_slide_load(INT8U x)
{
	INT8U i;
	if(x==0)
	{
		for(i=PREVIEW_SRC_FRONT;i<PREVIEW_SRC_TOTAL;i++)
		{
			//sensor_preview_out_crop_set_init(i, 0, 0, 0, 0, 0, 0, 0);
			Global_User_Optins.item.out_crop[i] = 0;
			Global_User_Optins.item.out_off_x[i] = 0;
			Global_User_Optins.item.out_off_y[i] = 0;
			Global_User_Optins.item.crop_slide_w_start[i] = 0;
			Global_User_Optins.item.crop_slide_h_start[i] = 0;
			Global_User_Optins.item.crop_slide_w_start_next[i] = 0;
			Global_User_Optins.item.crop_slide_h_start_next[i] = 0;
		}
		sensor_preview_out_crop_reversing_set(0, 0, 0, 0);
		Global_User_Optins.item.crop_slide_w_start_reversing = 0;
		Global_User_Optins.item.crop_slide_h_start_reversing = 0;
		Global_User_Optins.item.crop_slide_w_start_reversing_next = 0;
		Global_User_Optins.item.crop_slide_h_start_reversing_next = 0;

		//sensor_preview_out_crop_flag_set(0);
		//sensor_preview_out_crop_reversing_flag_set(0);
	}
	else
	{
		for(i=PREVIEW_SRC_FRONT;i<PREVIEW_SRC_TOTAL;i++)
		{
			sensor_preview_out_crop_set_init(i, //Global_User_Optins.item.crop_flag,
												Global_User_Optins.item.out_crop[i],
												Global_User_Optins.item.out_off_x[i],
												Global_User_Optins.item.out_off_y[i],
												Global_User_Optins.item.crop_slide_w_start[i],
												Global_User_Optins.item.crop_slide_h_start[i],
												Global_User_Optins.item.crop_slide_w_start_next[i],
												Global_User_Optins.item.crop_slide_h_start_next[i]);
		}
		sensor_preview_out_crop_reversing_set(Global_User_Optins.item.crop_slide_w_start_reversing,Global_User_Optins.item.crop_slide_h_start_reversing,
												Global_User_Optins.item.crop_slide_w_start_reversing_next,Global_User_Optins.item.crop_slide_h_start_reversing_next);

		sensor_preview_out_crop_flag_set(0);
		sensor_preview_out_crop_reversing_flag_set(0);
	}
}
#endif

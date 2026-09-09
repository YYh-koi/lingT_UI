#ifndef __AP_STATE_CONFIG_H__
#define __AP_STATE_CONFIG_H__

#define CRC32_POLY				0xEDB88320

extern void		ap_state_config_initial(INT32S status);
extern void		ap_state_config_default_set(void);
extern void		ap_state_config_store(void);
extern INT32S	ap_state_config_load(void);

extern void		ap_state_config_factory_date_get(INT8U *date);
extern void		ap_state_config_factory_time_get(INT8U *time);
extern INT32U	ap_state_config_base_day_get(void);
extern void		ap_state_config_base_day_set(INT32U day);
extern INT8U	ap_state_config_volume_get(void);
extern void		ap_state_config_volume_set(INT8U sound_volume);

//For system settings

#if C_GSENSOR_FUNCTION == CUSTOM_ON
extern INT8U	ap_state_config_park_mode_G_sensor_get(void);
extern void		ap_state_config_park_mode_G_sensor_set(INT8U Gsensor);
#endif

extern INT8U	ap_state_config_auto_off_get(void);
extern void		ap_state_config_auto_off_set(INT8U auto_off);
extern INT8U	ap_state_config_auto_off_TFT_BL_get(void);
extern void		ap_state_config_auto_off_TFT_BL_set(INT8U auto_off_TFT_BL);
extern INT8U	ap_state_config_beep_sound_get(void);
extern void		ap_state_config_beep_sound_set(INT8U off);
extern INT8U	ap_state_config_language_get(void);
extern void		ap_state_config_language_set(INT8U language);
extern INT8U	ap_state_config_light_freq_get(void);
extern void		ap_state_config_light_freq_set(INT8U light_freq);
extern INT8U	ap_state_config_flash_LED_get(void);
extern void		ap_state_config_flash_LED_set(INT8U status);
extern INT8U	ap_state_config_date_time_mode_get(void);
extern void		ap_state_config_date_time_mode_set(INT8U dis_mode);
extern INT8U	ap_state_config_usb_mode_get(void);
extern void		ap_state_config_usb_mode_set(INT8U usb_mode);

extern INT8U ap_state_config_plate_number_onoff_get(void);
extern void ap_state_config_plate_number_onoff_set(INT8U onoff);
extern INT8U    *ap_state_config_car_plate_number_get(void);
extern void     ap_state_config_car_plate_number_set(INT8U *car_id_input);

//For photo capture settings
extern INT8U	ap_state_config_pic_size_get(void);
extern void		ap_state_config_pic_size_set(INT8U pic_size);
extern INT8U	ap_state_config_capture_burst_get(void);
extern void		ap_state_config_capture_burst_set(INT8U burst);
extern INT8U	ap_state_config_quality_get(void);
extern void		ap_state_config_quality_set(INT8U quality);
extern INT8U	ap_state_config_sharpness_get(void);
extern void		ap_state_config_sharpness_set(INT8U sharpness);
extern INT8U	ap_state_config_white_balance_get(void);
extern void		ap_state_config_white_balance_set(INT8U white_balance);
extern INT8U	ap_state_config_iso_get(void);
extern void		ap_state_config_iso_set(INT8U iso);
extern INT8U	ap_state_config_capture_ev_get(void);
extern void		ap_state_config_capture_ev_set(INT8U ev);
extern INT8U	ap_state_config_anti_shaking_get(void);
extern void		ap_state_config_anti_shaking_set(INT8U onoff);
extern INT8U	ap_state_config_capture_review_get(void);
extern void		ap_state_config_capture_review_set(INT8U preview);
extern INT8U	ap_state_config_capture_date_stamp_get(void);
extern void		ap_state_config_capture_date_stamp_set(INT8U date_stamp);

//For video record settings
extern INT8U	ap_state_config_video_resolution_get(void);
extern void		ap_state_config_video_resolution_set(INT8U resolution);
extern INT8U	ap_state_config_record_time_get(void);          //get record time config
extern void		ap_state_config_record_time_set(INT8U record_time);     //parameter: record time config
extern INT8U	ap_state_config_record_minute_get(void);        //get real minute
extern INT8U	ap_state_config_record_ev_get(void);
extern void		ap_state_config_record_ev_set(INT8U ev);
extern INT8U	ap_state_config_md_get(void);
extern void		ap_state_config_md_set(INT8U md);
extern INT8U	ap_state_config_voice_record_switch_get(void);
extern void		ap_state_config_voice_record_switch_set(INT8U voice_record_mode);
extern INT8U	ap_state_config_record_date_stamp_get(void);
extern void		ap_state_config_record_date_stamp_set(INT8U date_stamp);

#if C_GSENSOR_FUNCTION == CUSTOM_ON
extern INT8U	ap_state_config_record_G_sensor_get(void);
extern void		ap_state_config_record_G_sensor_set(INT8U Gsensor);
#endif

extern INT8U	ap_state_config_display_mode_get(void);
extern void		ap_state_config_display_mode_set(INT8U dis_mode);
extern INT8U	ap_state_config_draw_line_mode_get(void);
extern void		ap_state_config_draw_line_mode_set(INT8U mode);

extern void		CRC_cal(INT8U *pucBuf, INT32U len, INT8U *CRC);
extern INT8U ap_state_config_reversing_display_mode_get(void);
#if defined(ENABLE_CHECK_RTC) && ENABLE_CHECK_RTC == 1
extern void		ap_state_config_date_time_save_set(void);
extern void		ap_state_config_date_time_save_set_default(void);
extern INT8U	ap_state_config_date_time_check(void);
extern INT8U	ap_state_config_date_time_power_off_check(void);

extern INT8U	RTC_TEST_SHOW;
#endif

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
extern void ap_state_wifi_ssid_set(INT8U* wifi_ssid, INT8U wifi_ssid_len);
extern void ap_state_wifi_ssid_get(INT8U* wifi_ssid, INT8U* wifi_ssid_len);
extern void ap_state_wifi_ssid_maclen_set(INT8U ssid_maclen);
extern void ap_state_wifi_ssid_maclen_get(INT8U* ssid_maclen);
extern void ap_state_wifi_password_set(INT8U* wifi_password, INT8U wifi_password_len);
extern void ap_state_wifi_password_get(INT8U* wifi_password, INT8U* wifi_password_len);
extern void ap_state_wifi_channel_set(INT8U channel);
extern void ap_state_wifi_channel_get(INT8U* channel);
extern void ap_state_wifi_ip_set(INT8U* ip);
extern void ap_state_wifi_ip_get(INT8U* ip);
#endif

#if C_TIME_LAPSE
extern void 	ap_state_config_time_lapse_recording_set(INT8U);
extern INT8U 	ap_state_config_time_lapse_recording_get();
extern INT8U    ap_state_config_time_lapse_recording_temp_get(void);
extern INT8U    ap_state_config_time_lapse_fps_get(void);
extern void     ap_state_config_time_lapse_fps_set(INT8U fps);
#endif
#if C_DISP_SLIDE_SAVE
extern INT16U ap_state_config_crop_slide_width_get(void);
extern INT16U ap_state_config_crop_slide_height_get(void);
extern void ap_state_config_crop_slide_width_set(INT16U w);
extern void ap_state_config_crop_slide_height_set(INT16U h);
extern INT8U ap_state_config_crop_slide_reversing_flag_get(void);
extern void ap_state_config_crop_slide_reversing_flag_set(INT8U flag);
extern INT8U ap_state_config_crop_slide_flag_get(void);
extern void ap_state_config_crop_slide_flag_set(INT8U flag);
extern void ap_state_config_crop_slide_get(INT8U src);
extern void ap_state_config_crop_reversing_get(void);
extern void ap_state_config_crop_slide_load(INT8U x);
#endif
#endif // __AP_STATE_CONFIG_H__

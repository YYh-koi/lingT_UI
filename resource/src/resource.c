#include "application.h"

INT16U *setting_background[] =
{
	NULL,
	icon_background_up,
	icon_background_down,
	icon_background_ok,
	icon_background_mode
};

SETTING_ITEM setting_record_table[] =
{
	{icon_menu_record_resolution, STR_RESOLUTION, STR_1080FHD, 3, ap_state_config_video_resolution_get, ap_state_config_video_resolution_set}
	, {icon_menu_record_looprecording, STR_LOOPRECORDING, STR_OFF, 5, ap_state_config_record_time_get, ap_state_config_record_time_set}
	, {icon_menu_record_EV, STR_EV, STR_P2_0, 13, ap_state_config_record_ev_get, ap_state_config_record_ev_set}
	, {icon_menu_record_motiondetect, STR_MOTIONDETECT, STR_OFF1, 2, ap_state_config_md_get, ap_state_config_md_set}
	, {icon_menu_base_beep_sound, STR_RECORD_AUDIO, STR_OFF1, 2, ap_state_config_voice_record_switch_get, ap_state_config_voice_record_switch_set}
	, {icon_menu_record_date_stamp, STR_DATE_STAMP, STR_OFF1, 2, ap_state_config_record_date_stamp_get, ap_state_config_record_date_stamp_set}
    #if C_GSENSOR_FUNCTION
	, {icon_menu_record_gsensor, STR_GSENSOR, STR_OFF2, 4, ap_state_config_record_G_sensor_get, ap_state_config_record_G_sensor_set}
	#endif
	#if C_WATER_MARK == CUSTOM_ON
	, {icon_menu_record_gsensor, STR_PLATE_NUMBER, STR_OFF1, 2, ap_state_config_plate_number_onoff_get, ap_state_config_plate_number_onoff_set}
	#endif
	, {NULL, 0, 0, 0, NULL, NULL}
};

SETTING_ITEM setting_capture_table[] =
{
	{icon_menu_record_resolution, STR_RESOLUTION2, STR_12M, 7, ap_state_config_pic_size_get, ap_state_config_pic_size_set}
	, {icon_menu_capture_sequence, STR_SEQUENCE, STR_OFF1, 2, ap_state_config_capture_burst_get, ap_state_config_capture_burst_set}
	, {icon_menu_capture_quality, STR_QUALITY, STR_FINE, 3, ap_state_config_quality_get, ap_state_config_quality_set}
	, {icon_menu_capture_sharpness, STR_SHARPNESS, STR_STRONG, 3, ap_state_config_sharpness_get, ap_state_config_sharpness_set}
	, {icon_menu_capture_white_balance, STR_WHITE_BALANCE, STR_AUTO, 5, ap_state_config_white_balance_get, ap_state_config_white_balance_set}
	, {icon_menu_capture_iso, STR_ISO, STR_AUTO2, 3, ap_state_config_iso_get, ap_state_config_iso_set}
	, {icon_menu_record_EV, STR_EV1, STR_P2_0, 13, ap_state_config_capture_ev_get, ap_state_config_capture_ev_set}
	, {icon_menu_capture_anti_shaking, STR_ANTI_SHAKING, STR_OFF1, 2, ap_state_config_anti_shaking_get, ap_state_config_anti_shaking_set}
	, {icon_menu_capture_quick_review, STR_QUICK_REVIEW, STR_OFF4, 3, ap_state_config_capture_review_get, ap_state_config_capture_review_set}
	, {icon_menu_record_date_stamp, STR_CAPTURE_DATE_STAMP, STR_OFF3, 3, ap_state_config_capture_date_stamp_get, ap_state_config_capture_date_stamp_set}
	, {NULL, 0, 0, 0, NULL, NULL}
};

SETTING_ITEM setting_play_table[] =
{
	{icon_menu_play_delete, STR_DELETE, STR_DELETE_CURRENT, 2, NULL, NULL}
	, {icon_menu_play_protect, STR_PROTECT, STR_LOCK_CURRENT, 4, NULL, NULL}
	, {NULL, 0, 0, 0, NULL, NULL}
};

SETTING_ITEM setting_base_table[] =
{
    #if C_GSENSOR_FUNCTION
	{icon_menu_record_gsensor, STR_PARK_MODE, STR_OFF2, 4, ap_state_config_park_mode_G_sensor_get, ap_state_config_park_mode_G_sensor_set},
	#endif
	{icon_menu_base_date_time, STR_DATE_TIME2, 0, 7, NULL, NULL}
	, {icon_menu_base_auto_power_off, STR_AUTO_POWER_OFF, STR_OFF5, 3, ap_state_config_auto_off_get, ap_state_config_auto_off_set}
	, {icon_menu_base_screen_saver, STR_SCREEN_SAVER, STR_OFF6, 4, ap_state_config_auto_off_TFT_BL_get, ap_state_config_auto_off_TFT_BL_set}
	, {icon_menu_base_beep_sound, STR_BEEP_SOUND, STR_OFF1, 2, ap_state_config_beep_sound_get, ap_state_config_beep_sound_set}
	, {icon_menu_base_language, STR_LANGUAGE, STR_MULTI_LANGUAGE, LCD_MAX, ap_state_config_language_get, ap_state_config_language_set}
	, {icon_menu_base_frequency, STR_FREQUENCY, STR_50HZ, 2, ap_state_config_light_freq_get, ap_state_config_light_freq_set}
	, {icon_capture_white_balance_daylight, STR_FLASH, STR_OFF1, 2, ap_state_config_flash_LED_get, ap_state_config_flash_LED_set}
	, {icon_menu_base_format, STR_FORMAT, STR_CANCEL, 2, NULL, NULL}
	, {icon_menu_base_default_setting, STR_DEFAULT_SETTING, STR_CANCEL, 2, NULL, NULL}
	, {icon_menu_base_version, STR_VERSION, 0, 0, NULL, NULL}
	, {NULL, 0, 0, 0, NULL, NULL}
};

DISPLAY_ICONSHOW display_icon_item[ICOM_MAX_NUM] =
{
	{ 0, 0, 0, 0, 0, 0 },

    /* ---------------  common icons -------------------------*/
	//ICON_BATTERY_0
	{icon_battery0, 32, 32, TRANSPARENT_COLOR, -34, -40 },

	//ICON_BATTERY_1
	{icon_battery1, 32, 32, TRANSPARENT_COLOR, -34, -40 },

	//ICON_BATTERY_2
	{icon_battery2, 32, 32, TRANSPARENT_COLOR, -34, -40 },

	//ICON_BATTERY_3
	{icon_battery3, 32, 32, TRANSPARENT_COLOR, -34, -40 },

    //ICON_BATTERY_CHARGED
	{icon_battery_charged, 32, 32, TRANSPARENT_COLOR, -34, -40 },

	//ICON_SD_CARD
	{icon_sd_card, 32, 32, TRANSPARENT_COLOR, -34, -80 },

	//ICON_NO_SD
	{icon_no_sd_card, 32, 32, TRANSPARENT_COLOR, -34, -80 },

	//ICON_EV6
	{icon_video_EV6, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 3 },

	//ICON_EV5
	{icon_video_EV5, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 3 },

	//ICON_EV4
	{icon_video_EV4, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 3 },

	//ICON_EV3
	{icon_video_EV3, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 3 },

	//ICON_EV2
	{icon_video_EV2, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 3 },

	//ICON_EV1
	{icon_video_EV1, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 3 },

	//ICON_EV0
	{icon_video_EV0, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 3 },

	//ICON_EV_1
	{icon_video_EV_1, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 3 },

	//ICON_EV_2
	{icon_video_EV_2, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 3 },

	//ICON_EV_3
	{icon_video_EV_3, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 3 },

	//ICON_EV_4
	{icon_video_EV_4, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 3 },

	//ICON_EV_5
	{icon_video_EV_5, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 3 },

	//ICON_EV_6
	{icon_video_EV_6, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 3 },

    /* ---------------  video record mode -------------------------*/
	//ICON_VIDEO_RECORD
	{icon_video_record, 32, 32, TRANSPARENT_COLOR, 2, 0 },

	//ICON_RECORD_REC
	{icon_rec, 32, 32, TRANSPARENT_COLOR, 40, 0 },

    //ICON_RECORD_LOCKED
	{icon_locked, 32, 32, TRANSPARENT_COLOR, 80, 0 },

	//ICON_RECORD_MOTION_DETECT
	{icon_motion_detect, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 2 },

	//ICON_RECORD_MIC_OFF
	{icon_mic_off, 32, 32, TRANSPARENT_COLOR, 120, 0 },

	//ICON_RECORD_RECORD_CYC_1MINUTE
	{icon_video_record_cyc_1minute, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 1 },

	//ICON_RECORD_RECORD_CYC_2MINUTE
	{icon_video_record_cyc_2minute, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 1 },

	//ICON_RECORD_RECORD_CYC_3MINUTE
	{icon_video_record_cyc_3minute, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 1 },

	//ICON_RECORD_RECORD_CYC_5MINUTE
	{icon_video_record_cyc_5minute, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 1 },

	//ICON_RECORD_RECORD_CYC_10MINUTE
	{icon_video_record_cyc_10minute, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 1 },

    /* ---------------  photo capture mode -------------------------*/
	//ICON_CAPTURE
	{icon_capture, 32, 32, TRANSPARENT_COLOR, 2, 0 },

	//ICON_CAPTURE_WHITE_BALANCE_AUTO
	{icon_menu_capture_white_balance, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 2 },

	//ICON_CAPTURE_WHITE_BALANCE_DAYLIGHT
	{icon_capture_white_balance_daylight, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 2 },

	//ICON_CAPTURE_WHITE_BALANCE_CLOUDY
	{icon_capture_white_balance_cloudy, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 2 },

	//ICON_CAPTURE_WHITE_BALANCE_TUNGSTEN
	{icon_capture_white_balance_tungsten, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 2 },

	//ICON_CAPTURE_WHITE_BALANCE_FLUORESCENT
	{icon_capture_white_balance_fluorescent, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 2 },

	//ICON_CAPTURE_ISO_AUTO
	{icon_capture_ISO_auto, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 4 },

	//ICON_CAPTURE_ISO_100
	{icon_capture_ISO_100, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 4 },

	//ICON_CAPTURE_ISO_200
	{icon_capture_ISO_200, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 4 },

	//ICON_CAPTURE_ISO_400
	{icon_capture_ISO_400, 32, 32, TRANSPARENT_COLOR, 2, C_ICON_OFF_Y * 4 },

	//ICON_CAPTURE_ANTI_SHAKING
	{icon_menu_capture_anti_shaking, 32, 32, TRANSPARENT_COLOR, 80, 0 },

	//ICON_CAPTURE_SEQUENCE
	{icon_menu_capture_sequence, 32, 32, TRANSPARENT_COLOR, 120, 0 },

	//ICON_CAPTURE_QUALITY_FINE
	{icon_capture_quality_fine, 32, 32, TRANSPARENT_COLOR, -40, C_ICON_OFF_Y * 2 },

	//ICON_CAPTURE_QUALITY_NORMAL
	{icon_capture_quality_normal, 32, 32, TRANSPARENT_COLOR, -40, C_ICON_OFF_Y * 2 },

	//ICON_CAPTURE_QUALITY_ECONOMY
	{icon_capture_quality_economy, 32, 32, TRANSPARENT_COLOR, -40, C_ICON_OFF_Y * 2 },

    /* ---------------  play back mode -------------------------*/
	//ICON_PLAYBACK
	{icon_playback, 32, 32, TRANSPARENT_COLOR, 2, 0 },

	//ICON_PLAYBACK_PLAY
	{icon_play, 32, 32, TRANSPARENT_COLOR, 2, -40 },

	//ICON_PLAYBACK_PAUSE
	{icon_pause, 32, 32, TRANSPARENT_COLOR, 2, -40 },

	//ICON_PLAYBACK_MOVIE
	{icon_playback_movie, 32, 32, TRANSPARENT_COLOR, 2, 0 },

	//ICON_PLAYBACK_LOCKED
	{icon_locked, 32, 32, TRANSPARENT_COLOR, 80, 0 },

	//ICON_PLAYBACK_VOLUME_TITLE
	{icon_playback_volume_title, 32, 32, TRANSPARENT_COLOR, 2, -80 },

	//ICON_PLAYBACK_VOLUME_WHITE1
	{icon_playback_volume_white, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 0, -80 },

	//ICON_PLAYBACK_VOLUME_WHITE2
	{icon_playback_volume_white, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 1, -80 },

	//ICON_PLAYBACK_VOLUME_WHITE3
	{icon_playback_volume_white, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 2, -80 },

	//ICON_PLAYBACK_VOLUME_WHITE4
	{icon_playback_volume_white, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 3, -80 },

	//ICON_PLAYBACK_VOLUME_WHITE5
	{icon_playback_volume_white, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 4, -80 },

	//ICON_PLAYBACK_VOLUME_WHITE6
	{icon_playback_volume_white, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 5, -80 },

	//ICON_PLAYBACK_VOLUME_WHITE7
	{icon_playback_volume_white, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 6, -80 },

	//ICON_PLAYBACK_VOLUME_WHITE8
	{icon_playback_volume_white, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 7, -80 },

	//ICON_PLAYBACK_VOLUME_RED1
	{icon_playback_volume_red, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 0, -80 },

	//ICON_PLAYBACK_VOLUME_RED2
	{icon_playback_volume_red, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 1, -80 },

	//ICON_PLAYBACK_VOLUME_RED3
	{icon_playback_volume_red, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 2, -80 },

	//ICON_PLAYBACK_VOLUME_RED4
	{icon_playback_volume_red, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 3, -80 },

	//ICON_PLAYBACK_VOLUME_RED5
	{icon_playback_volume_red, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 4, -80 },

	//ICON_PLAYBACK_VOLUME_RED6
	{icon_playback_volume_red, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 5, -80 },

	//ICON_PLAYBACK_VOLUME_RED7
	{icon_playback_volume_red, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 6, -80 },

	//ICON_PLAYBACK_VOLUME_RED8
	{icon_playback_volume_red, 32, 32, TRANSPARENT_COLOR, 34 + C_ICON_VOL_OFFSET * 7, -80 },

    /* ---------------  setting mode -------------------------*/
	//ICON_SETTING_BASE
	{icon_base_setting, 32, 32, TRANSPARENT_COLOR, 50, 0 },
};

DISPLAY_ICONSHOW display_num_item[NUM_MAX] =
{
	// NUM_TEXT_0
	{ui_text_0, 8, 20, TRANSPARENT_COLOR, 0, 0 },
	// NUM_TEXT_1
	{ui_text_1, 8, 20, TRANSPARENT_COLOR, 0, 0 },
	// NUM_TEXT_2
	{ui_text_2, 8, 20, TRANSPARENT_COLOR, 0, 0 },
	// NUM_TEXT_3
	{ui_text_3, 8, 20, TRANSPARENT_COLOR, 0, 0 },
	// NUM_TEXT_4
	{ui_text_4, 8, 20, TRANSPARENT_COLOR, 0, 0 },
	// NUM_TEXT_5
	{ui_text_5, 8, 20, TRANSPARENT_COLOR, 0, 0 },
	// NUM_TEXT_6
	{ui_text_6, 8, 20, TRANSPARENT_COLOR, 0, 0 },
	// NUM_TEXT_7
	{ui_text_7, 8, 20, TRANSPARENT_COLOR, 0, 0 },
	// NUM_TEXT_8
	{ui_text_8, 8, 20, TRANSPARENT_COLOR, 0, 0 },
	// NUM_TEXT_9
	{ui_text_9, 8, 20, TRANSPARENT_COLOR, 0, 0 },
	// NUM_TEXT_COLON
	{ui_text_colon, 8, 20, TRANSPARENT_COLOR, 0, 0 },
	// NUM_TEXT_SLASH
	{ui_text_klino, 8, 20, TRANSPARENT_COLOR, 0, 0 }
};



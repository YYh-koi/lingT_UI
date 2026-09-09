#ifndef __RESOURCE_GENERAL_H__
#define __RESOURCE_GENERAL_H__

enum
{
	NUM_TEXT_0,
	NUM_TEXT_1,
	NUM_TEXT_2,
	NUM_TEXT_3,
	NUM_TEXT_4,
	NUM_TEXT_5,
	NUM_TEXT_6,
	NUM_TEXT_7,
	NUM_TEXT_8,
	NUM_TEXT_9,
	NUM_TEXT_COLON,
	NUM_TEXT_SLASH,
	NUM_MAX
};

enum
{
    RES_STR_ICON,
    PARK_STR_ICON,
    WARN_STR_ICON,
    STR_ICONS_MAX
};

enum
{
    //common icons
	ICON_BATTERY_0                             = 1,
	ICON_BATTERY_1,
	ICON_BATTERY_2,
	ICON_BATTERY_3,
	ICON_BATTERY_CHARGED,
	ICON_SD_CARD,
	ICON_NO_SD,
	ICON_EV6,
	ICON_EV5,
	ICON_EV4,
	ICON_EV3,
	ICON_EV2,
	ICON_EV1,
	ICON_EV0,
	ICON_EV_1,
	ICON_EV_2,
	ICON_EV_3,
	ICON_EV_4,
	ICON_EV_5,
	ICON_EV_6,

    //video record mode
	ICON_VIDEO_RECORD,
	ICON_RECORD_REC,
    ICON_RECORD_LOCKED,
	ICON_RECORD_MOTION_DETECT,
	ICON_RECORD_MIC_OFF,
	ICON_RECORD_RECORD_CYC_1MINUTE,
	ICON_RECORD_RECORD_CYC_2MINUTE,
	ICON_RECORD_RECORD_CYC_3MINUTE,
	ICON_RECORD_RECORD_CYC_5MINUTE,
	ICON_RECORD_RECORD_CYC_10MINUTE,

	//capture mode
	ICON_CAPTURE,
	ICON_CAPTURE_WHITE_BALANCE_AUTO,
	ICON_CAPTURE_WHITE_BALANCE_DAYLIGHT,
	ICON_CAPTURE_WHITE_BALANCE_CLOUDY,
	ICON_CAPTURE_WHITE_BALANCE_TUNGSTEN,
	ICON_CAPTURE_WHITE_BALANCE_FLUORESCENT,
	ICON_CAPTURE_ISO_AUTO,
	ICON_CAPTURE_ISO_100,
	ICON_CAPTURE_ISO_200,
	ICON_CAPTURE_ISO_400,
	ICON_CAPTURE_ANTI_SHAKING,
	ICON_CAPTURE_SEQUENCE,
	ICON_CAPTURE_QUALITY_FINE,
	ICON_CAPTURE_QUALITY_NORMAL,
	ICON_CAPTURE_QUALITY_ECONOMY,

    //playback_mode
	ICON_PLAYBACK,
	ICON_PLAYBACK_PLAY,
	ICON_PLAYBACK_PAUSE,
	ICON_PLAYBACK_MOVIE,
	ICON_PLAYBACK_LOCKED,
	ICON_PLAYBACK_VOLUME_TITLE,
	ICON_PLAYBACK_VOLUME_WHITE1,
	ICON_PLAYBACK_VOLUME_WHITE2,
	ICON_PLAYBACK_VOLUME_WHITE3,
	ICON_PLAYBACK_VOLUME_WHITE4,
	ICON_PLAYBACK_VOLUME_WHITE5,
	ICON_PLAYBACK_VOLUME_WHITE6,
	ICON_PLAYBACK_VOLUME_WHITE7,
	ICON_PLAYBACK_VOLUME_WHITE8,
	ICON_PLAYBACK_VOLUME_RED1,
	ICON_PLAYBACK_VOLUME_RED2,
	ICON_PLAYBACK_VOLUME_RED3,
	ICON_PLAYBACK_VOLUME_RED4,
	ICON_PLAYBACK_VOLUME_RED5,
	ICON_PLAYBACK_VOLUME_RED6,
	ICON_PLAYBACK_VOLUME_RED7,
	ICON_PLAYBACK_VOLUME_RED8,

	//setting mode
	ICON_SETTING_BASE,
	ICOM_MAX_NUM
};

enum
{
	//=== Multi-language string item must be the first one, below this line. ===
	STR_MULTI_LANGUAGE, //0
	STR_VIDEO,
	STR_CAPTURE,
	STR_PLAYBACK,
	STR_SETUP,
	STR_RESOLUTION,
	STR_LOOPRECORDING,
	STR_EV,
	STR_MOTIONDETECT,
	STR_RECORD_AUDIO,
	STR_DATE_STAMP,		//10
	STR_GSENSOR,
    STR_PLATE_NUMBER,
    STR_4K,
    STR_2K,
	STR_1080FHD,
	STR_1080P,
	STR_720P,
	STR_WVGA,
	STR_VGA,
	STR_QVGA,           //20
	STR_OFF,
	STR_1_MIN,
	STR_2_MIN,
	STR_3_MIN,
	STR_5_MIN,
	STR_10_MIN,
	STR_P2_0,
	STR_P5_3,
	STR_P4_3,
	STR_P1_0,           //30
	STR_P2_3,
	STR_P1_3,
	STR_P0_0,
	STR_N1_3,
	STR_N2_3,
	STR_N1_0,
	STR_N4_3,
	STR_N5_3,
	STR_N2_0,
	STR_OFF1,           //40
	STR_ON,
	STR_OFF2,
	STR_2G,
	STR_4G,
	STR_8G,
	STR_RESOLUTION2,
	STR_SEQUENCE,
	STR_QUALITY,
	STR_SHARPNESS,
	STR_WHITE_BALANCE,  //50
	STR_ISO,
	STR_EV1,
	STR_ANTI_SHAKING,
	STR_QUICK_REVIEW,
	STR_CAPTURE_DATE_STAMP,
	STR_2S_TIMER,
	STR_5S_TIMER,
	STR_10S_TIMER,
	STR_12M,
	STR_10M,            //60
	STR_8M,
	STR_5M,
	STR_3M,
	STR_2MHD,
	STR_1_3M,
	STR_1_M,
	STR_FINE,
	STR_NORMAL,
	STR_ECONOMY,
	STR_STRONG,         //70
	STR_NORMAL1,
	STR_SOFT,
	STR_AUTO,
	STR_DAYLIGHT,
	STR_CLOUDY,
	STR_TUNGSTEN,
	STR_FLUORESCENT,
	STR_AUTO2,
	STR_ISO100,
	STR_ISO200,         //80
	STR_ISO400,
	STR_OFF3,
	STR_DATE_ONLY,
	STR_DATE_TIME,
	STR_DELETE,
	STR_PROTECT,
	STR_DELETE_CURRENT,
	STR_DELETE_ALL,
	STR_CANCEL,
	STR_OK,             //90
	STR_LOCK_CURRENT,
	STR_UNLOCK_CURRENT,
	STR_LOCK_ALL,
	STR_UNLOCK_ALL,
	STR_OFF4,           //for capture review
	STR_2_SECOND,
	STR_5_SECOND,
	STR_8_SECOND,
	STR_PARK_MODE,
	STR_DATE_TIME2,     //100
	STR_AUTO_POWER_OFF,
	STR_SCREEN_SAVER,
	STR_BEEP_SOUND,
	STR_LANGUAGE,
	STR_FREQUENCY,
	STR_FLASH,
	STR_FORMAT,
	STR_DEFAULT_SETTING,
	STR_VERSION,
	STR_OFF5,           //110, for screen saver
	STR_1_MIN1,
	STR_3_MIN1,
	STR_OFF6,           //for auto power off
	STR_3_MIN2,
	STR_5_MIN2,
	STR_10_MIN2,
	STR_50HZ,
	STR_60HZ,
	STR_DEFAULT_SETTING_SHOW_1,
	STR_DEFAULT_SETTING_SHOW_2, //120
	STR_SD_CARD,
	STR_FORMAT_SHOW_1,
	STR_FORMAT_SHOW_2,
	STR_INSERT_SDC,
	STR_NO_SD,
	STR_MASS_STORAGE,
	STR_PC_CAMERA,
	STR_NO_FILE,
	STR_BATTERY_LOW,
	STR_NUM0,           //130
	STR_NUM1,
	STR_NUM2,
	STR_NUM3,
	STR_NUM4,
	STR_NUM5,
	STR_NUM6,
	STR_NUM7,
	STR_NUM8,
	STR_NUM9,
	STR_SD_FULL,        //140
	STR_PLEASE_WAIT,
	STR_PROTECTED,
	STR_USB,
	STR_SDC_ERR,
	STR_MAX
};

#define TRANSPARENT_COLOR_RGB565		0x8c71
#define TRANSPARENT_COLOR_GP420 		0x8c
#define TRANSPARENT_COLOR_YUYV          0x8c80

#define C_ICON_OFF_Y		36
#define C_ICON_VOL_OFFSET	15

#define width_icon	        32
#define width_1_icon        8
#define height_icon	        32

#if RESOURCE_FORMAT == RESOURCE_GP420

#define size_1		        (((width_icon * height_icon) * 3)/4)     // 多除2 是因為 sp_icon 是INT16U , 本來是應該INT8U ,但因為是TOOL轉的, 改不動
#define size_2		        (((width_1_icon * height_icon) * 3)/4)

#define TRANSPARENT_COLOR   TRANSPARENT_COLOR_GP420

#elif RESOURCE_FORMAT == RESOURCE_RGB565

#define size_1		        (width_icon * height_icon)               // 多除2 是因為 sp_icon 是INT16U , 本來是應該INT8U ,但因為是TOOL轉的, 改不動
#define size_2		        (width_1_icon * height_icon)

#define TRANSPARENT_COLOR   TRANSPARENT_COLOR_RGB565

#elif(RESOURCE_FORMAT == RESOURCE_YUYV)

#define size_1		        (width_icon * height_icon)               // 多除2 是因為 sp_icon 是INT16U , 本來是應該INT8U ,但因為是TOOL轉的, 改不動
#define size_2		        (width_1_icon * height_icon)

#define TRANSPARENT_COLOR   TRANSPARENT_COLOR_YUYV

#endif

//----icon_table---------------------------------
extern INT16U  sp_icon[];

#define icon_battery0                               (void*)&sp_icon[0]
#define icon_battery1                               (void*)&sp_icon[size_1]
#define icon_battery2                               (void*)&sp_icon[(size_1*2)]
#define icon_battery3                               (void*)&sp_icon[(size_1*3)]
#define icon_capture                                (void*)&sp_icon[(size_1*4)]
#define icon_video_record                           (void*)&sp_icon[(size_1*5)]
#define icon_playback                               (void*)&sp_icon[(size_1*6)]
#define icon_base_setting                           (void*)&sp_icon[(size_1*7)]
#define icon_motion_detect                          (void*)&sp_icon[(size_1*8)]
#define icon_rec                                    (void*)&sp_icon[(size_1*9)]
#define icon_play                                   (void*)&sp_icon[(size_1*10)]
#define icon_pause                                  (void*)&sp_icon[(size_1*11)]
#define icon_mic_off                                (void*)&sp_icon[(size_1*12)]
#define icon_sd_card                                (void*)&sp_icon[(size_1*13)]
#define icon_no_sd_card                             (void*)&sp_icon[(size_1*14)]
#define icon_video_record_cyc_1minute               (void*)&sp_icon[(size_1*15)]
#define icon_video_record_cyc_2minute               (void*)&sp_icon[(size_1*16)]
#define icon_video_record_cyc_3minute               (void*)&sp_icon[(size_1*17)]
#define icon_video_record_cyc_5minute               (void*)&sp_icon[(size_1*18)]
#define icon_video_record_cyc_10minute              (void*)&sp_icon[(size_1*19)]
#define icon_video_EV6                              (void*)&sp_icon[(size_1*20)]
#define icon_video_EV5                              (void*)&sp_icon[(size_1*21)]
#define icon_video_EV4                              (void*)&sp_icon[(size_1*22)]
#define icon_video_EV3                              (void*)&sp_icon[(size_1*23)]
#define icon_video_EV2                              (void*)&sp_icon[(size_1*24)]
#define icon_video_EV1                              (void*)&sp_icon[(size_1*25)]
#define icon_video_EV0                              (void*)&sp_icon[(size_1*26)]
#define icon_video_EV_1                             (void*)&sp_icon[(size_1*27)]
#define icon_video_EV_2                             (void*)&sp_icon[(size_1*28)]
#define icon_video_EV_3                             (void*)&sp_icon[(size_1*29)]
#define icon_video_EV_4                             (void*)&sp_icon[(size_1*30)]
#define icon_video_EV_5                             (void*)&sp_icon[(size_1*31)]
#define icon_video_EV_6                             (void*)&sp_icon[(size_1*32)]
#define icon_battery_charged                        (void*)&sp_icon[(size_1*33)]
#define icon_locked                                 (void*)&sp_icon[(size_1*34)]
#define icon_menu_capture_white_balance             (void*)&sp_icon[(size_1*35)]
#define icon_capture_white_balance_daylight         (void*)&sp_icon[(size_1*36)]
#define icon_capture_white_balance_cloudy           (void*)&sp_icon[(size_1*37)]
#define icon_capture_white_balance_tungsten         (void*)&sp_icon[(size_1*38)]
#define icon_capture_white_balance_fluorescent      (void*)&sp_icon[(size_1*39)]
#define icon_capture_ISO_auto                       (void*)&sp_icon[(size_1*40)]
#define icon_capture_ISO_100                        (void*)&sp_icon[(size_1*41)]
#define icon_capture_ISO_200                        (void*)&sp_icon[(size_1*42)]
#define icon_capture_ISO_400                        (void*)&sp_icon[(size_1*43)]
#define icon_menu_capture_anti_shaking              (void*)&sp_icon[(size_1*44)]
#define icon_menu_capture_sequence                  (void*)&sp_icon[(size_1*45)]
#define icon_capture_quality_economy                (void*)&sp_icon[(size_1*46)]
#define icon_capture_quality_normal                 (void*)&sp_icon[(size_1*47)]
#define icon_capture_quality_fine                   (void*)&sp_icon[(size_1*48)]
#define icon_playback_movie                         (void*)&sp_icon[(size_1*49)]
#define icon_playback_volume_title                  (void*)&sp_icon[(size_1*50)]
#define icon_playback_volume_white                  (void*)&sp_icon[(size_1*51)]
#define icon_playback_volume_red                    (void*)&sp_icon[(size_1*52)]
//---setting_record_table------------------
#define icon_menu_record_resolution                 (void*)&sp_icon[(size_1*53)]
#define icon_menu_record_looprecording              (void*)&sp_icon[(size_1*54)]
#define icon_menu_record_EV                         (void*)&sp_icon[(size_1*55)]
#define icon_menu_record_motiondetect               (void*)&sp_icon[(size_1*56)]
#define icon_menu_base_beep_sound                   (void*)&sp_icon[(size_1*57)]
#define icon_menu_record_date_stamp                 (void*)&sp_icon[(size_1*58)]
#define icon_menu_record_gsensor                    (void*)&sp_icon[(size_1*59)]
//----setting_capture_table----------------------
#define icon_menu_capture_quality                   (void*)&sp_icon[(size_1*60)]
#define icon_menu_capture_sharpness                 (void*)&sp_icon[(size_1*61)]
#define icon_menu_capture_iso                       (void*)&sp_icon[(size_1*62)]
#define icon_menu_capture_quick_review              (void*)&sp_icon[(size_1*63)]
//----setting_play_table----------------------------------------
#define icon_menu_play_delete                       (void*)&sp_icon[(size_1*64)]
#define icon_menu_play_protect                      (void*)&sp_icon[(size_1*65)]
//----setting_base_table----------------------------------------
#define icon_menu_base_date_time                    (void*)&sp_icon[(size_1*66)]
#define icon_menu_base_auto_power_off               (void*)&sp_icon[(size_1*67)]
#define icon_menu_base_screen_saver                 (void*)&sp_icon[(size_1*68)]
#define icon_menu_base_language                     (void*)&sp_icon[(size_1*69)]
#define icon_menu_base_frequency                    (void*)&sp_icon[(size_1*70)]
#define icon_menu_base_format                       (void*)&sp_icon[(size_1*71)]
#define icon_menu_base_default_setting              (void*)&sp_icon[(size_1*72)]
#define icon_menu_base_version                      (void*)&sp_icon[(size_1*73)]
//---others----------------------------------------------------------
#define icon_background_up                          (void*)&sp_icon[(size_1*74)]
#define icon_background_down                        (void*)&sp_icon[(size_1*75)]
#define icon_background_ok                          (void*)&sp_icon[(size_1*76)]
//----64x32 sp--------------------------------------------------------------
#define icon_background_mode                        (void*)&sp_icon[(size_1*77)]  // 64x32
//---number_table-------------------------------------------------
#define ui_text_0                                   (void*)&sp_icon[(size_1*79)]
#define ui_text_1                                   (void*)&sp_icon[((size_1*79)+size_2*1)]
#define ui_text_2                                   (void*)&sp_icon[((size_1*79)+size_2*2)]
#define ui_text_3                                   (void*)&sp_icon[((size_1*79)+size_2*3)]
#define ui_text_4                                   (void*)&sp_icon[((size_1*79)+size_2*4)]
#define ui_text_5                                   (void*)&sp_icon[((size_1*79)+size_2*5)]
#define ui_text_6                                   (void*)&sp_icon[((size_1*79)+size_2*6)]
#define ui_text_7                                   (void*)&sp_icon[((size_1*79)+size_2*7)]
#define ui_text_8                                   (void*)&sp_icon[((size_1*79)+size_2*8)]
#define ui_text_9                                   (void*)&sp_icon[((size_1*79)+size_2*9)]
#define ui_text_colon                               (void*)&sp_icon[((size_1*79)+size_2*10)]
#define ui_text_klino                               (void*)&sp_icon[((size_1*79)+size_2*11)]



#endif // __RESOURCE_GENERAL_H__

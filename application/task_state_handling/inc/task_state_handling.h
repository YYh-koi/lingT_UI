#ifndef __TASK_STATE_HANDLING_H__
#define __TASK_STATE_HANDLING_H__

#include "msg_manager.h"

#define VIDEO_STREAM			0x63643030
#define AUDIO_STREAM			0x62773130
#define JUNK_DATA				0x4b4e554a

#define CARD_FULL_MB_SIZE		6ULL		// <= 6MB is mean card full

#define AUDIO_PLAY_SRC_FROM_SDRAM      0
typedef enum
{
    VOICE_ID_CLICK      =0,
    VOICE_ID_CAMERA,
    VOICE_ID_BEEP,
    VOICE_ID_POWER_ON,
    VOICE_ID_POWER_OFF,
    VOICE_ID_CAPTURE,
    VOICE_ID_FILE_LOCK,
    VOICE_ID_MIC_ON,
    VOICE_ID_MIC_OFF,
    VOICE_ID_FCW_ALARM,
    VOICE_ID_LDW_RIGHT,
    VOICE_ID_LDW_LEFT,
    VOICE_ID_SnG,
    VOICE_ID_PEDESTRIAN,
    VOICE_ID_VIRTUAL_BUMPER,

    VOICE_ID_MAX_NUM
} VOICE_ID_ITEMS;

typedef struct
{
	INT16U	string_width;
	INT16U	string_height;
} t_STR_SIZE_STRUCT;

typedef struct
{
	INT16U	font_color;
	INT16U	font_type;
	INT16S	pos_x;
	INT16S	pos_y;
	INT16U	buff_w;
	INT16U	buff_h;
	CHAR	*str_ptr;
	INT16U	font_off_h;
} STRING_ASCII_INFO;

typedef struct
{
	INT16U	font_color;
	INT16U	font_type;
	INT16S	pos_x;
	INT16S	pos_y;
	INT16U	buff_w;
	INT16U	buff_h;
	INT16U	language;
	INT16U	str_idx;
	INT16U	font_off_h;
} STRING_INFO;

typedef struct
{
	INT8U	mode;
	INT8U	on_hour;
	INT8U	on_minute;
	INT8U	off_hour;
	INT8U	off_minute;
} POWERONOFF_STRUCT, *pPOWERONOFF_STRUCT;

typedef struct
{
	INT32U	Frequency;
} FM_STATION_FORMAT;

#define FM_STATION_NUM	20

typedef struct
{
	FM_STATION_FORMAT	FM_Station[FM_STATION_NUM];
	INT8U				FM_Station_index;
} FM_INFO;

typedef struct
{
	INT8U               factory_date[3];	//yy,mm,dd
	INT8U               factory_time[3];	//hh:mm:ss
	INT32U              base_day;
	INT8U				sound_volume;

	// For System Settings
#if C_GSENSOR_FUNCTION == CUSTOM_ON
	INT8U               park_gsensor;
#endif
	INT8U				auto_off;
	INT8U               screen_save;
	INT8U               beep_sound;
	INT8U				language;
	INT8U				light_freq;
	INT8U               flash_led;
	INT8U				date_time_mode;
	INT8U				usb_mode;           //0: storage,   1: webcam

	// For Photo Capture
	INT8U				capture_pic_size;
	INT8U				capture_burst;
	INT8U               capture_quality;
	INT8U				capture_sharpness;
	INT8U               capture_wb;
	INT8U               capture_iso;
	INT8U               capture_ev;
	INT8U               anti_shaking;
	INT8U               capture_review;
	INT8U               capture_datestamp;

	// For Video Record
	INT8U               video_resolution;
	INT8U				record_time;
	INT8U				record_ev;
	INT8U               motion_det;
	INT8U               record_voice;
	INT8U				record_datestamp;

#if C_GSENSOR_FUNCTION == CUSTOM_ON
	INT8U               record_gsensor;
#endif

	INT8U               plate_number_onoff;
	INT8U               car_plate_number[9];

	INT8U				pip_mode;
	INT8U				back_car_line;

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	//wifi
	INT8U               wifi_ssid[32];
	INT8U               wifi_ssid_len;
	INT8U               wifi_ssid_maclen;
	INT8U               wifi_pw[32];
	INT8U               wifi_pw_len;
	INT8U               wifi_channel;
	INT8U               wifi_IP[4];
#endif

#if C_TIME_LAPSE
	INT8U               time_lapse;
#endif

#if defined(ENABLE_CHECK_RTC) && ENABLE_CHECK_RTC == 1
	POWERONOFF_STRUCT	powertime_onoff_struct1;
	POWERONOFF_STRUCT	powertime_onoff_struct2;
#endif

	//add for hxwl by antion@20230406
	INT8U		reversing_display;//0:full screen and keep ratio,1:full screen and change the shape
#if C_DISP_SLIDE_SAVE
	INT16U		crop_w;
	INT16U		crop_h;

	INT8U		out_crop[PREVIEW_SRC_TOTAL];
	INT16U		out_off_x[PREVIEW_SRC_TOTAL];
	INT16U		out_off_y[PREVIEW_SRC_TOTAL];

	INT16U		crop_slide_h_start[PREVIEW_SRC_TOTAL];
	INT16U		crop_slide_w_start[PREVIEW_SRC_TOTAL];
	INT16U		crop_slide_h_start_next[PREVIEW_SRC_TOTAL];
	INT16U		crop_slide_w_start_next[PREVIEW_SRC_TOTAL];

	INT16U		crop_slide_h_start_reversing;
	INT16U		crop_slide_w_start_reversing;
	INT16U		crop_slide_h_start_reversing_next;
	INT16U		crop_slide_w_start_reversing_next;
#endif
	INT8U				ifdirty;
} USER_ITEMS;

typedef struct
{
	USER_ITEMS	item;
	INT8U		crc[4];
	INT8U		DUMMY_BYTE[512 - sizeof(USER_ITEMS) - 4];
} SYSTEM_USER_OPTION, *pSYSTEM_USER_OPTION;

/* ap music */
typedef struct
{
	INT8U	aud_state;
	BOOLEAN mute;
	INT8U	volume;
} audio_status_st;

typedef enum
{
	STATE_IDLE,
	STATE_PLAY,
	STATE_PAUSED,
	STATE_AUD_FILE_ERR
} AUDIO_STATE_ENUM;

typedef enum
{
	//	PLAY_ONCE,
	PLAY_SEQUENCE,
	PLAY_REPEAT,
	PLAY_MIDI,
	PLAY_SPECIAL_REPEAT,
	PLAY_SPECIAL_BYNAME_REPEAT,
	PLAY_SHUFFLE_MODE
} AUDIO_PLAY_STYLE;

extern void state_handling_task_init(void);
extern void state_app_init(void);

extern void state_handling_entry(void const *arg);
extern INT32S state_handling_msg_send(INT32U msg, void *para, INT32U bytes, INT32U prio);
extern INT32S app_msg_send(INT32U msg, void *para, INT32U bytes, INT32U prio);

extern INT32U avi_encode_get_jpeg_enc_buf_addr(void);
extern INT32S __check_file_handle(STOR_SERV_FILEINFO* file_info);

#endif //__TASK_STATE_HANDLING_H__

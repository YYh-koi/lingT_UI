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
#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "drv_l2.h"

#include "msg_manager.h"
#include "turnkey_drv_msg.h"
#include "turnkey_sys_msg.h"

// Configuration
#include "application_cfg.h"
#include "task_display_cfg.h"
#include "task_encoder_cfg.h"
#include "mm_gplus.h"

extern mm_type CHUNK_MEM;

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#ifndef max
#define max(a, b)	(((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)	(((a) < (b)) ? (a) : (b))
#endif

/**************************************************************************
 *                      F E A T U R E	O P T I O N S                     *
 **************************************************************************/

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/
#define BL_ON	1
#define BL_OFF	0

typedef enum
{
	BAT_LVL_0,
	BAT_LVL_1,
	BAT_LVL_2,
	BAT_LVL_3,
	BAT_LVL_NUM,
	BAT_LVL_UNDEF
} BAT_LVL;

#define DECODE_USE_JPEG_HW_IDX          JPEGA


#define CAPTURE_CONTINUOUS_SHOOTING_COUNT	5

#define USE_SPIFC_SPEED             18500000
#define USE_SPIFC_BUS_WIDTH         IO_1BIT

/**************************************************************************
 *                      T A S K   S T A C K          	                  *
 **************************************************************************/
#define INIT_TASK_STACKSIZE       				4096
#define DISPLAY_TASK_STACK_SIZE         		4096
#define STATE_HANDLING_STACK_SIZE				4096
#define PERIPHERAL_HANDLING_STACK_SIZE			2048
#define STORAGE_SERVICE_TASK_STACK_SIZE 		4096
#define PIPELINE_FACE_DETECTION_TASK_STACK_SIZE 10240
#define SYS_SOFTWARE_TIMER_STACK_SIZE			1024
#define VIDEO_ENCODE_STATE_STACK_SIZE			4096
#define AUDIO_ENCODE_STATE_STACK_SIZE			4096
#define FILE_PACKER_TASK_STACK_SIZE				8192
#define USB_TASK_STACK_SIZE						2048
#define AUDIO_DEC_TASK_STACK_SIZE				4096
#define AUDIO_DAC_TASK_STACK_SIZE               2048
#define SOCKET_CMD_TASK_STACK_SIZE				4096
#define WIFI_DOWNLOAD_STATE_STACK_SIZE			4096
#define VOICE_RECG_TASK_STACK_SIZE				2048
#define BSD_TASK_STACK_SIZE     				2048
#define LVGL_TASK_STACK_SIZE					8192

/**************************************************************************
 *                      T A S K   P R I O R I T I E S	                  *
 **************************************************************************/
#define PRIORITY_STATE_HANDLING				osPriorityAboveNormal
#define PRIORITY_DISPLAY_TASK				osPriorityHigh
#define PRIORITY_PERIPHERAL_HANDLING		osPriorityBelowNormal
#define PRIORITY_SYS_SOFTWARE_TIMER_TASK	osPriorityHigh
#define PRIORITY_VIDEO_ENC_TASK				osPriorityRealtime
#define PRIORITY_AUDIO_ENC_TASK				osPriorityRealtime

#define PRIORITY_PIPELINE_FACE_DETECTION    osPriorityLow
#define PRIORITY_STORAGE_SERVICE            osPriorityLow
#define PRIORITY_FILE_PACKER				osPriorityNormal
#define PRIORITY_FILE_PACKER_EMER           osPriorityAboveNormal
#define PRIORITY_USB_DEVICE					osPriorityAboveNormal
#define PRIORITY_USB_CAM					osPriorityAboveNormal
#define PRIORITY_RTSP_H264					osPriorityBelowNormal
#define PRIORITY_RTK_SDIO					osPriorityNormal
#define PRIORITY_AUD_DEC_TASK				osPriorityAboveNormal
#define PRIORITY_AUD_DAC_TASK               osPriorityRealtime
#define PRIORITY_SOCKET_CMD					osPriorityBelowNormal
#define PRIORITY_WIFI_DOWNLOAD_TASK			osPriorityAboveNormal
#define PRIORITY_LVGL	    				osPriorityNormal

#define PRIORITY_BROWSE_DEC_STATE			osPriorityHigh
#define PRIORITY_BROWSE_AUD_DEC				osPriorityAboveNormal
#define PRIORITY_BROWSE_VID_DEC				osPriorityRealtime
#define PRIORITY_VIDEO_PASER				osPriorityAboveNormal

#define PRIORITY_VOICE_RECG_TASK			osPriorityLow

#define PRIORITY_BSD_TASK			        osPriorityBelowNormal

/**************************************************************************
 *                      T A S K   C O N F I G U R A T I O N               *
 **************************************************************************/
#define DISPLAY_TASK_QUEUE_MAX_LEN				32

#define PERIPHERAL_TASK_QUEUE_MAX_LEN			64

#define STATE_HANDLING_TASK_QUEUE_MAX_LEN		5

#define STATE_APP_TASK_QUEUE_MAX_LEN			32

#define VIDEO_ENC_TASK_QUEUE_MAX_LEN			128
#define AUDIO_ENC_TASK_QUEUE_MAX_LEN			64

#define STORAGE_SERVICE_TASK_QUEUE_MAX_LEN		32

#define FILE_PACKER_TASK_QUEUE_MAX_LEN			32

#define USB_TASK_QUEUE_MAX						32

#define AUDIO_DEC_QUEUE_MAX						32

#define VOICE_RECG_TASK_QUEUE_MAX_LEN			64

#define BSD_TASK_QUEUE_MAX_LEN			        32

#define LVGL_TASK_QUEUE_MAX_LEN         		32
/**************************************************************************
 *                      S O F T W A R E		T I M E R	                  *
 **************************************************************************/
enum
{
	STORAGE_SERVICE_MOUNT_TIMER_ID = 1,
	STORAGE_SERVICE_FREESIZE_TIMER_ID,
	BROWSE_DISPLAY_UPDATE_TIMER_ID,
	AD_DETECT_TIMER_ID,
	HOME_SELECT_TIMER_ID,
	VIDEO_PREVIEW_TIMER_ID
};

/**************************************************************************
 *                      K E Y 	M O D E					                  *
 **************************************************************************/
enum
{
	KEY_MODE_DISABLE,
	KEY_MODE_NORMAL,

	#if C_BATTERY_LOW_POWER_OFF
	KEY_MODE_BETTERY_LOW,
	#endif

	KEY_MODE_BACK_A_CAR,

	WIFI_MODE_KEY,

	KEY_MODE_TOTAL
};

/**************************************************************************
 *                 M E D I A	F O R M A T				    			  *
 **************************************************************************/
typedef enum
{
	AUD_AUTOFORMAT						= 0,
	MIDI,
	WMA,
	MP3,
	WAV,
	A1800,
	S880,
	A6400,
	A1600,
	IMA_ADPCM,
	MICROSOFT_ADPCM,
	PCM,
	ALAW,
	MULAW,
	AAC,
	OGG
} AUDIO_FORMAT;

typedef enum
{
	IMG_AUTOFORMAT						= 0,
	JPEG,
	JPEG_P,					// JPEG Progressive
	MJPEG_S,				// Single JPEG from M-JPEG video
	GIF,
	BMP
} IMAGE_FORMAT;

typedef enum
{
	VID_AUTOFORMAT						= 0,
	MJPEG,
	MPEG4,
	H264
} VIDEO_FORMAT;

/**************************************************************************
 *                 M E D I A	A R G U M E N T			    			  *
 **************************************************************************/
typedef enum
{
	IMAGE_OUTPUT_FORMAT_RGB565			= 0,
	IMAGE_OUTPUT_FORMAT_RGBG,
	IMAGE_OUTPUT_FORMAT_GRGB,
	IMAGE_OUTPUT_FORMAT_YUYV,
	IMAGE_OUTPUT_FORMAT_YVYU,
	IMAGE_OUTPUT_FORMAT_UYVY,
	IMAGE_OUTPUT_FORMAT_VYUY
} IMAGE_OUTPUT_FORMAT;

/**************************************************************************
 *                 M E D I A	S T A T U S				    			  *
 **************************************************************************/
typedef enum
{
	START_OK							= 0,
	RESOURCE_NO_FOUND_ERROR,
	RESOURCE_READ_ERROR,
	RESOURCE_WRITE_ERROR,
	CHANNEL_ASSIGN_ERROR,
	REENTRY_ERROR,
	AUDIO_ALGORITHM_NO_FOUND_ERROR,
	CODEC_START_STATUS_ERROR_MAX
} CODEC_START_STATUS;

typedef enum
{
	AUDIO_CODEC_PROCESSING				= 0,	// Decoding or Encoding
	AUDIO_CODEC_PROCESS_END,					// Decoded or Encoded End
	AUDIO_CODEC_BREAK_OFF,						// Due to unexpended card-plug-in-out
	AUDIO_CODEC_PROCESS_PAUSED,
	AUDIO_CODEC_STATUS_MAX
} AUDIO_CODEC_STATUS;

/**************************************************************************
 *                 M E D I A	S O U R C E				    			  *
 **************************************************************************/
typedef enum
{
	SOURCE_TYPE_FS						= 0,
	SOURCE_TYPE_SDRAM,
	SOURCE_TYPE_NVRAM,
	SOURCE_TYPE_USER_DEFINE,
	SOURCE_TYPE_FS_RESOURCE_IN_FILE,
	SOURCE_TYPE_MAX
} SOURCE_TYPE;

/**************************************************************************
 *                 E X I T	F L A G						    			  *
 **************************************************************************/
typedef enum
{
	EXIT_RESUME							= 0,	/* no exit */
	EXIT_SUSPEND,		/* prepare to entry state suspend mode *//* after exit, the next state is decision by UMI Task*/
	EXIT_PAUSE_SUSPEND, /* no release resource suspend, like as suspend for storage INT *//* after exit, the next state is decision by UMI Task*/
	EXIT_STG_SUSPEND,	/* no release resource suspend, only for storage INT *//* after exit, the next state is decision by UMI Task*/
	EXIT_USB_SUSPEND,	/* no release resource suspend, only for storage INT *//* after exit, the next state is decision by UMI Task*/
	EXIT_TO_SHORTCUT,
	EXIT_ALARM_SUSPEND,
	EXIT_BREAK_MENU_KEY,
	EXIT_BREAK			/* after exit, the next state is define by current state*/
} EXIT_FLAG_ENUM;

/**************************************************************************
 *                 A U D I O	S A M P L E    R A T E	    			  *
 **************************************************************************/
#define MB_PER_MIN_4K   	360
#define MB_PER_MIN_2K   	200
#define MB_PER_MIN_1080FHD	150
#define MB_PER_MIN_1080P	120
#define MB_PER_MIN_720P		100
#define MB_PER_MIN_WVGA		74
#define MB_PER_MIN_VGA		70
#define MB_PER_MIN_QVGA		65
#define MB_PER_MIN_BACK		MB_PER_MIN_720P


#define AUD_RECORD_SAMPLE_RATE		16000
#define AUD_RECORD_KILO_BYTERATE	(AUD_RECORD_SAMPLE_RATE * 2 / 1024)

#define SND_REC_SAMPLING_RATE		AUD_RECORD_SAMPLE_RATE

#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
	#if (VOICE_RECG_MODULE == VOICE_RECG_WANSON)
	#define SND_REC_SAMPLING_FRAME		3840
	#endif
#else
#define SND_REC_SAMPLING_FRAME		2048
#endif
#define SND_REC_SAMPLING_MS			(1000 * SND_REC_SAMPLING_FRAME / SND_REC_SAMPLING_RATE)

/**************************************************************************
 *                 A U D I O	A R G U M E N T		    			  *
 **************************************************************************/
#define MAX_DAC_BUFFERS			6


typedef enum
{
	AUDIO_SRC_TYPE_FS					= 0,
	AUDIO_SRC_TYPE_SDRAM,
	AUDIO_SRC_TYPE_SPI,
	AUDIO_SRC_TYPE_RS,
	AUDIO_SRC_TYPE_USER_DEFINE,
	AUDIO_SRC_TYPE_FS_RESOURCE_IN_FILE,
	AUDIO_SRC_TYPE_GPRS,
	AUDIO_SRC_TYPE_APP_RS,
	AUDIO_SRC_TYPE_APP_PACKED_RS,
	AUDIO_SRC_TYPE_MAX
} AUDIO_SOURCE_TYPE;

typedef enum
{
	AUDIO_TYPE_NONE						= -1,
	AUDIO_TYPE_WAV,
	AUDIO_TYPE_MP3,
	AUDIO_TYPE_WMA,
	AUDIO_TYPE_AVI,
	AUDIO_TYPE_AVI_MP3,
	AUDIO_TYPE_A1800,
	AUDIO_TYPE_MIDI,
	AUDIO_TYPE_A1600,
	AUDIO_TYPE_A6400,
	AUDIO_TYPE_S880,
	AUDIO_TYPE_AAC,
	AUDIO_TYPE_OGG,
	AUDIO_TYPE_MAX
} AUDIO_TYPE;

typedef enum
{
	AUDIO_ERR_NONE,
	AUDIO_ERR_FAILED,
	AUDIO_ERR_INVALID_FORMAT,
	AUDIO_ERR_OPEN_FILE_FAIL,
	AUDIO_ERR_GET_FILE_FAIL,
	AUDIO_ERR_DEC_FINISH,
	AUDIO_ERR_DEC_FAIL,
	AUDIO_ERR_READ_FAIL,
	AUDIO_ERR_AVI_READ_FAIL,
	AUDIO_ERR_MEM_ALLOC_FAIL
} AUDIO_STATUS;

/**************************************************************************
 *                 S T O R A G E	A R G U M E N T		    			  *
 **************************************************************************/
typedef enum
{
	STOR_SERV_SEARCH_INIT				= 0,
	STOR_SERV_SEARCH_ORIGIN,
	STOR_SERV_SEARCH_PREV,
	STOR_SERV_SEARCH_NEXT,
	STOR_SERV_SEARCH_GIVEN
} STOR_SERV_PLAYSEARCH;

typedef enum
{
	STOR_SERV_IDLE						= 0,
	STOR_SERV_OPEN_FAIL,
	STOR_SERV_OPEN_OK,
	STOR_SERV_DECODE_ALL_FAIL,
	STOR_SERV_NO_MEDIA
} STOR_SERV_PLAYSTS;

#define SD_SLOT_ID		0
#define MAX_SLOT_NUMS	1
#define NO_STORAGE							0xFF

/**************************************************************************
 *                 I M A G E	A R G U M E N T		    			  	  *
 **************************************************************************/
typedef enum
{
	TK_IMAGE_SOURCE_TYPE_FILE			= 0x0,
	TK_IMAGE_SOURCE_TYPE_BUFFER,
	TK_IMAGE_SOURCE_TYPE_NVRAM,
	TK_IMAGE_SOURCE_TYPE_MAX
} TK_IMAGE_SOURCE_TYPE_ENUM;

typedef enum
{
	TK_IMAGE_TYPE_JPEG					= 0x0,
    TK_IMAGE_TYPE_H264,
    TK_IMAGE_TYPE_WAV,
	TK_IMAGE_TYPE_PROGRESSIVE_JPEG,
	TK_IMAGE_TYPE_MOTION_JPEG,
	TK_IMAGE_TYPE_BMP,
	TK_IMAGE_TYPE_GIF,
	TK_IMAGE_TYPE_PNG,
	TK_IMAGE_TYPE_GPZP,
	TK_IMAGE_TYPE_MOV_JPEG,
	TK_IMAGE_TYPE_MAX
} TK_IMAGE_TYPE_ENUM;

#define C_XVID_FORMAT			0x44495658
#define C_MJPG_FORMAT			0x47504A4D
#define C_H264_FORMAT			0x34363248


/**************************************************************************
 *                V I D E O    A R G U M E N T		    			      *
 **************************************************************************/
#define VIDEO_RECORD_UNMOUNT	0x1
#define VIDEO_RECORD_BUSY		0x2
#define VIDEO_RECORD_MD			0x4
#define VIDEO_RECORD_WAIT		0x8

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
/*== R E C O R D ==*/
typedef struct VID_INFO_S
{
	VIDEO_REC_TYPE vid_rec_type;
	VIDEO_SRC_FROM  data_source;
	VIDEO_DATA_FROMAT data_format;
	INT32U	sensor_width;
	INT32U	sensor_height;
	INT32U	clip_width;
	INT32U	clip_height;
	INT32U	target_width;
	INT32U	target_height;
	INT32U	frame_rate;
	INT32U	vid_rec_line;	// 0 is for frame mode , or set 32 fifo line
	INT32U  vid_fifo_start;
	INT32U  vid_fifo_drop;
	INT32S	rec_file_handle;
	INT32U  run_ap_mode;
	INT32U  add_time_stamp;
	GET_JPEG_ARGS get_jpeg_args;

	#if (C_VID_INTERPOLATING_2K)
	INT32U  run_2K_flow_src;
	INT32U  pscaler_up_start;
	INT32U 	pscaler_up_drop;
	#endif

	#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
	HOST_USB_MJPEG_DEC_ARGS host_usb_mjpeg_dec_args;
	#endif
	union
	{
		H264_REC_ARGS	h264_rec_args;
		JPEG_REC_ARGS	jpeg_rec_args;
	};
} VID_INFO;

#if (defined C_Y_ONLY_FUNCTION) && (C_Y_ONLY_FUNCTION)
typedef struct Y_ONLY_INFO_S
{
	Y_ONLY_SRC_FROM  data_source;
	Y_ONLY_DATA_FROMAT  data_format;
	INT16U	sensor_width;
	INT16U	sensor_height;
	INT16U	clip_x_start;
	INT16U	clip_y_start;
	INT16U	clip_width;
	INT16U	clip_height;
	INT16U	target_width;
	INT16U	target_height;
	PIPELINE_SCALER_IN_FIFO_LINES	in_fifo_line;	// 0 is for frame mode , or set 32 fifo line
} Y_ONLY_INFO;
#endif

typedef struct DISPLAY_INFO_S
{
	INT16U display_width;
	INT16U display_height;
	INT32U display_format;
	INT16U display_h_offset;
	INT16U pip_width;
	INT16U pip_height;
	INT16U pip_pos_x;
	INT16U pip_pos_y;
	INT8U en_crop;
	INT8U pbp_crop;
	INT8U back_ratio;
} DISP_INFO;

typedef struct STREAM_ARGUMENT_S
{
	VID_INFO  vid_info_front;
	VID_INFO  vid_info_rear;
	#if (defined C_Y_ONLY_FUNCTION) && (C_Y_ONLY_FUNCTION)
	Y_ONLY_INFO y_only_inf;
	#endif
	#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	VID_INFO  vid_info_wifi;
	#endif
	#if(defined(C_USB_DEVICE) && C_USB_DEVICE)
	VID_INFO  vid_info_uvc;
	#endif
	DISP_INFO disp_info;
} STREAM_ARGUMENT;

typedef struct TASK_APP_ARGUMENT_S
{
	MSG_Q_ID taskAppQ;
	MSG_Q_ID taskAckQ;
	INT32U taskMsgLen;
	void*  taskRecvBuf;
	INT32U taskRecvBufLen;
	void*  taskPrivateData;
	osSemaphoreId taskSem;
} TASK_APP_ARGUMENT;


/*== T A S K ==*/
typedef struct TURNKEY_ARGUMENT_S
{
	TASK_APP_ARGUMENT taskDisplay;

	#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
	TASK_APP_ARGUMENT taskUSBDevice;
	#endif
	TASK_APP_ARGUMENT taskVideoEnc;
	TASK_APP_ARGUMENT taskAudioEnc;
	TASK_APP_ARGUMENT taskPeripheral;
	TASK_APP_ARGUMENT taskStateHdlg;
	TASK_APP_ARGUMENT taskStateApp;
	#if (defined C_STORAGE_DEVICE) && (C_STORAGE_DEVICE)
	TASK_APP_ARGUMENT taskStorage;
	#endif
	TASK_APP_ARGUMENT taskAudioDec; // Play sound effect

	#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
	TASK_APP_ARGUMENT taskVoiceRecg; // Voice recognition
	#endif

	#if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
	TASK_APP_ARGUMENT taskBSD; // Blind Spot Detection
	#endif

    #if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
	TASK_APP_ARGUMENT taskLVGL; // LVGL task
	#endif
} TURNKEY_ARGUMENT;

/*== A U D I O ==*/
/*
	PCM Buffer Format:
	->AUD_CAP_BUF_HEADER
	->Real Address
*/
typedef struct _bufparams {
	INT32U	copies;
	INT32U  reserve[15];
}AUD_CAP_BUF_HEADER;

typedef struct
{
	INT8U	*ring_buf;
	INT8S	*work_mem;
	INT8U	reading;
	INT16S	file_handle;
	INT32U	frame_size;
	INT32U	ring_size;
	INT32U	ri;
	INT32U	wi;
	INT32U	f_last;
	INT32U	file_len;
	INT32U	file_cnt;
	INT32U	try_cnt;
	INT32U	read_secs;
	INT32U	file_end;
} AUDIO_CTRL;

/*== Turnkey Audio Decoder task ==*/
typedef struct
{
	BOOLEAN mute;
	INT8U	volume;
	INT16S	fd;
	INT32U	src_id;
	INT32U	src_type;
	INT32U	audio_format;
	INT32U	file_len;
	INT32U	play_speed;
	INT32U	reverse;
} STAudioTaskPara;

typedef struct
{
	MSG_AUD_ENUM		result_type;
	AUDIO_SOURCE_TYPE	source_type;
	INT32S				result;
} STAudioConfirm;

/*== Turnkey File Server Task ==*/
typedef struct
{
	INT16S	file_handle;
	INT32U	file_path_addr;

	INT16S	file_handle_b;
	INT32U	file_path_addr_b;
} STOR_SERV_FILEINFO;

typedef struct
{
	INT8U	src_id;
	INT8U	src_name[10];
	INT16U	sec_offset;
	INT16U	sec_cnt;
} TK_FILE_SERVICE_SPI_STRUCT, *P_TK_FILE_SERVICE_SPI_STRUCT;

typedef struct
{
	INT16S						fd;
	INT32U						buf_addr;
	INT32U						buf_size;
	INT32U						rev_seek;
	INT8U						FB_seek;
	TK_FILE_SERVICE_SPI_STRUCT	spi_para;
	QueueHandle_t 				result_queue;
} TK_FILE_SERVICE_STRUCT, *P_TK_FILE_SERVICE_STRUCT;

/*== Turnkey Image processing task ==*/
typedef struct
{
	INT16S file_handle;
	INT32S image_source;          	// File handle/resource handle/pointer
	INT32U source_size;             // File size or buffer size
	INT8U source_type;              // 0=File System, 1=SDRAM, 2=NVRAM
	INT8S decode_status;            // 0=ok, others=fail
	INT8U disp_where;				// 0=TFT, 1=WIFI
	INT8U output_ratio;             // 0=Fit to output_buffer_width and output_buffer_height, 1=Maintain ratio and fit to output_buffer_width or output_buffer_height, 2=Same as 1 but without scale up
	INT32U output_format;
	INT16U output_buffer_width;
	INT16U output_buffer_height;
	INT16U input_image_width;
	INT16U input_image_height;
	INT16U output_image_width;
	INT16U output_image_height;
	INT32U out_of_boundary_color;
	INT32U output_buffer_pointer;
	INT32S (*data_reload)(INT16S,INT32U,INT32U);
	INT32U reload_size;
} IMAGE_DECODE_STRUCT, *P_IMAGE_DECODE_STRUCT;

/*== Turnkey SYSTEM Task ==*/
typedef enum
{
	SCAN_FILE_COMPLETE,
	SCAN_FILE_NOT_COMPLETE
} SCAN_FILE_STATUS_ENUM;

typedef struct
{
	INT16S	fd;
	INT8U	f_name[256];
	INT8U	f_extname[4];
	INT32U	f_size;
	INT16U	f_time;
	INT16U	f_date;
} STORAGE_FINFO;

/*== Playback Info ==*/
typedef struct
{
	INT16S	file_handle;
	INT16S	play_index;
	INT16U	total_file_number;
	INT16U	deleted_file_number;
	INT8U	file_type;
	INT8U	search_type;
	INT8S	err_flag;
	INT32U	file_size;
	INT32U	file_path_addr;
} STOR_SERV_PLAYINFO;

/*== Storage Info ==*/
typedef struct
{
	INT16S	file_handle;
	INT16S	play_index;
	INT32U	file_size;
	INT32U	file_path_addr;
	INT32U	file_offset;
} STOR_SERV_DOWNLOAD_FILEINFO;

/*== Snapshot Info ==*/
typedef struct SNAPSHOT_INFO_s
{
	INT32S	file_handle;
	INT32U	file_name;
	INT32U	file_w_buf;
	INT32U	file_w_size;
} SNAPSHOT_INFO;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern INT32U SDRAM_END_ADDR;

extern INT32U fps_audio_dac_dbf_end;
extern TURNKEY_ARGUMENT*  pTurnkeyArgs;

#define dispTaskQ               ((pTurnkeyArgs->taskDisplay).taskAppQ)

#if defined(C_USB_DEVICE) && (C_USB_DEVICE)
#define USBTaskQ                ((pTurnkeyArgs->taskUSBDevice).taskAppQ)
#endif

#define encoderTaskQ            ((pTurnkeyArgs->taskVideoEnc).taskAppQ)
#define encoderAckQ             ((pTurnkeyArgs->taskVideoEnc).taskAckQ)
#define encoderSem              ((pTurnkeyArgs->taskVideoEnc).taskSem)
#define audioEncTaskQ           ((pTurnkeyArgs->taskAudioEnc).taskAppQ)
#define audioEncAckQ            ((pTurnkeyArgs->taskAudioEnc).taskAckQ)
#define PeripheralTaskQ         ((pTurnkeyArgs->taskPeripheral).taskAppQ)
#define StateHandlingQ          ((pTurnkeyArgs->taskStateHdlg).taskAppQ)
#define ApQ                     ((pTurnkeyArgs->taskStateApp).taskAppQ)

#if defined(C_STORAGE_DEVICE) && (C_STORAGE_DEVICE)
#define StorageServiceQ         ((pTurnkeyArgs->taskStorage).taskAppQ)
#define StorageServiceMsgAck    ((pTurnkeyArgs->taskStorage).taskAckQ)
#endif

#define AudioTaskQ              ((pTurnkeyArgs->taskAudioDec).taskAppQ)

#define ApQ_para                ((INT8U *)((pTurnkeyArgs->taskStateApp).taskRecvBuf))
#define ApQ_para_len            ((pTurnkeyArgs->taskStateApp).taskRecvBufLen)

#define pStreamArgs             ((STREAM_ARGUMENT*)((pTurnkeyArgs->taskStateApp).taskPrivateData))

#if defined(C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
#define voiceRecgTaskQ               ((pTurnkeyArgs->taskVoiceRecg).taskAppQ)
#endif

#if defined(C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
#define bsdTaskQ               ((pTurnkeyArgs->taskBSD).taskAppQ)
#endif

#if defined(C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
#define LVGLTaskQ                ((pTurnkeyArgs->taskLVGL).taskAppQ)
#endif
/**************************************************************************
 *                   I N C L U D E		H E A D E R  		              *
 **************************************************************************/
#include "lib_sensor_engine.h"
#include "resource.h"

#include "task_state_handling.h"
#include "ap_state_handling.h"
#include "ap_state_resource.h"
#include "ap_state_config.h"

#include "task_peripheral_handling.h"
#include "ap_peripheral_handling.h"

#include "task_storage_service.h"
#include "ap_storage_service.h"

#include "task_video_decoder.h"

#include "task_audio_wav_dac.h"
#include "task_audio_wav_decoder.h"

#include "task_display.h"
#include "ap_display_func.h"

#include "ap_music.h"

#include "ap_browse.h"
#include "state_browse.h"
#include "ap_browse_engine.h"

#include "ap_video_record.h"
#include "state_video_record.h"

#include "ap_photo_capture.h"
#include "state_photo_capture.h"

#include "ap_startup.h"
#include "state_startup.h"

#include "ap_setting.h"
#include "state_setting.h"

#include "state_usb.h"
#include "ap_usb.h"
#include "usbd_cdc.h"

#include "state_wifi.h"

#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
#include "task_voice_recognition.h"
#endif

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
#include "socket_cmd.h"
#include "h264_server.h"
#include "rtsp.h"
#include "rtp.h"
#endif

#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
#include "state_lvgl.h"
#endif

#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
#include "state_audio_record.h"
#include "ap_audio_record.h"
#endif

#if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
#include "ap_blind_spot_detection.h"
#include "task_blind_spot_detection.h"
#endif

#include "usbd_uvc_cfg.h"
#endif // __APPLICATION_H__

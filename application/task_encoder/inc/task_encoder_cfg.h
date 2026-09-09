/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
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
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park,                      *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#ifndef __TASK_ENCODER_CFG_H__
#define __TASK_ENCODER_CFG_H__

#include "video_codec_dynamic_quality.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/*===VIDEO===*/
#define VID_REC_WIDTH_4K			3840
#define VID_REC_HEIGHT_4K			2160

#define VID_REC_WIDTH_5M			2560
#define VID_REC_HEIGHT_5M			1944

#define VID_REC_WIDTH_2K			2560
#define VID_REC_HEIGHT_2K			1440

#define VID_REC_WIDTH_1080FHD		1920
#define VID_REC_HEIGHT_1080FHD		1080

#define VID_REC_WIDTH_1080P			1440
#define VID_REC_HEIGHT_1080P		1080

#define VID_REC_WIDTH_720P			1280
#define VID_REC_HEIGHT_720P			720

#define VID_REC_WIDTH_WVGA			800
#define VID_REC_HEIGHT_WVGA			480

#define VID_REC_WIDTH_D1			720
#define VID_REC_HEIGHT_D1			480

#define VID_REC_WIDTH_VGA			640
#define VID_REC_HEIGHT_VGA			480

#define VID_REC_WIDTH_QVGA			320
#define VID_REC_HEIGHT_QVGA			240

#define UVC_VID_WIDTH_QVGA          320
#define UVC_VID_HEIGHT_QVGA         240

#define UVC_VID_WIDTH_VGA           640
#define UVC_VID_HEIGHT_VGA          480

#define UVC_VID_WIDTH_720p          1280
#define UVC_VID_HEIGHT_720p         720

/*===Y ONLY===*/
#define Y_ONLY_CLIP_EN				0
#if Y_ONLY_CLIP_EN == 1
#define Y_ONLY_CLIP_WIDTH			1280
#define Y_ONLY_CLIP_HEIGHT			720
#endif

#define Y_ONLY_TARGET_WIDTH			1280
#define Y_ONLY_TARGET_HEIGHT		720

#define Y_ONLY_BUF_COUNT			4

/********************GPS INFO*******************************/
#define GPS_INFO_PACK	            0
#define GPS_INFO_PACK_2	            0

#if GPS_INFO_PACK
#define GPS_MAX_COUNT	(10*60)
#define GPS_INFO_BLK_SIZE (20+132*GPS_MAX_COUNT)

typedef struct GPSInfo_s
{
	char chidentification[15];
	INT8U type;
	INT32U nCount;
	INT32U maxCount;
	INT8U* InfoBlk;
} GPSInfo_t;

#endif

#if GPS_INFO_PACK_2
#define GPS_MAX_COUNT	(512)
#endif
/********************GPS INFO END***************************/

typedef enum
{
	VIDEO_ENCODE_STATUS_SENSOR_OFFSET = 0,
	VIDEO_ENCODE_STATUS_ENCODE_OFFSET,// For state
	VIDEO_ENCODE_STATUS_ENCODE_H264_OFFSET,// For engine
	VIDEO_ENCODE_STATUS_ENCODE_JPEGA_OFFSET,// For engine
	VIDEO_ENCODE_STATUS_ENCODE_JPEGB_OFFSET,// For engine
	VIDEO_ENCODE_STATUS_ENCODE_STOP_OFFSET,
	VIDEO_ENCODE_STATUS_PHOTO_FRAME_READY_OFFSET,
	VIDEO_ENCODE_STATUS_PHOTO_OFFSET,
	VIDEO_ENCODE_STATUS_USB_WEBCAM_OFFSET,
	VIDEO_ENCODE_STATUS_WIFI_TRANSMISSION_OFFSET,
	VIDEO_ENCODE_STATUS_UVC_TRANSMISSION_OFFSET,
	VIDEO_ENCODE_STATUS_GET_JPEG_OFFSET,
	VIDEO_ENCODE_STATUS_INTERP2K_SCALEUP_OFFSET
} VIDEO_ENCODE_STATUS_OFFSET;

enum{
	MOV_STREAM_VID_RECORD,
	MOV_STREAM_VID_NUM
};

#define H264_FRAME_TYPE_I	0
#define H264_FRAME_TYPE_P	1

/*===AUDIO===*/
typedef enum
{
	AUDIO_ENCODE_STATUS_ENCODE_OFFSET = 0
} AUDIO_ENCODE_STATUS_OFFSET;

enum{
	MOV_STREAM_AUD_RECORD,
	MOV_STREAM_AUD_NUM
};

/*===PACKER===*/
#define FILE_PACKER_MAX_RECORD_SIZE	((2ul * 1024 * 1024 * 1024) - (12*1024*1024))	// 2GB

enum{
	MOV_TRACK_VID_RECORD,
	MOV_TRACK_AUD_RECORD,
	MOV_TRACK_VID_DUMMYRECORD, // for dummy frame use
	MOV_TRACK_NUM
};

typedef enum
{
	MSG_FILE_PACKER_WRITE = 0x8000,
	MSG_FILE_PACKER_CLOSE
} TASK_FILE_PACKER_APQ_ENUM;


enum
{
	VIDEO_RECORD_NOTHING,
	VIDEO_RECORD_START			= 0x01,
	VIDEO_RECORD_RECING			= 0x02,
	VIDEO_RECORD_STOP			= 0x04,
	VIDEO_RECORD_PHOTO_START	= 0x08,
	VIDEO_RECORD_PHOTO			= 0x10
};

typedef enum
{
	VID_REC_JPEG = 0,
	VID_REC_H264,
	VID_DEC_JPEG,
	VID_GET_JPEG
} VIDEO_REC_TYPE;

typedef enum
{
	VID_SRC_NONE = 0,
	VID_SRC_CDSP,
	VID_SRC_CSI0,
	VID_SRC_CSI1,
	VID_SRC_USB
} VIDEO_SRC_FROM;

typedef enum
{
	Y_ONLY_SRC_NONE = 0,
	Y_ONLY_SRC_CDSP,
	Y_ONLY_SRC_CSI0,
	Y_ONLY_SRC_DRAM
} Y_ONLY_SRC_FROM;

typedef enum
{
	VID_DATA_422 = 0,
	VID_DATA_420
} VIDEO_DATA_FROMAT;


typedef enum
{
	Y_ONLY_DATA_420 = 0,
	Y_ONLY_DATA_Y
} Y_ONLY_DATA_FROMAT;

/*===PHOTO CAPTURE===*/
enum
{
	PHOTO_QUALITY_BEST	= 0,
	PHOTO_QUALITY_NORMAL,
	PHOTO_QUALITY_LOW
};

#define PHOTO_Q_VALUE_BEST		98
#define PHOTO_Q_VALUE_NORMAL	90
#define PHOTO_Q_VALUE_LOW		70

/*== HOST USB MJPEG CAM / INTERP2K ==*/
typedef enum
{
	HOST_2K_IS_DISABLE = 0,
	HOST_2K_IS_FRONT = (1<<0),
	HOST_2K_IS_REAR = (1<<1),
	HOST_2K_IS_USB = (1<<2)
} HOST_2K_FLOW_ENUM;

enum
{
	GET_JPEG_DONE_JPEG = (1<<0),
	GET_JPEG_DONE_H264 = (1<<1)
};

/*===COMMON===*/
#define	ENCODE_STATUS_IS_BUSY(x,y)		(x & (1 << y))
#define	ENCODE_STATUS_SET_BUSY(x,y)		(x |= (1 << y))
#define	ENCODE_STATUS_CLR_BUSY(x,y)		(x &= ~(1 << y))

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct H264Enc_s
{
	void* handle;
	INT32U input_buffer;
	INT32U input_buffer_idx;
	INT32U input_buffer_hold; 	// 1:don't release 0:release
	INT32U output_buffer;
	INT32U encode_size;
	INT32U PictureType;			// Used by H264 only. 0=H264_FRAME_TYPE_I or MJPEG, 1=H264_FRAME_TYPE_P
	gp_h264_cfg cfg;
	gp_h264enc_rate_ctrl rccfg;
	INT8U flag_h264_trigger;
} H264Enc_t;

typedef struct FB_INFO_S
{
	INT32U  buf;
	INT32U  idx;
	INT32U  ps_buf_time;		// OS time tick when this buffer is received by lib_pscaler_notify_bypass()
}FB_INFO;

typedef struct FB_CFG_S // Config Frame or FIFO Buffer
{
	void* 	fb_cfg_head_addr; // For malloc'ed memory
	INT32U 	fb_cfg_size;
	INT32U 	fb_cfg_blocks;
	INT32U 	fb_cfg_max_count;
	INT32U 	fb_cfg_count_num;
	void*	fb_cfg_cb_addr; // For circular buffer
}FB_CFG;

typedef struct H264_REC_ARGS_S
{
	FB_CFG fb_pscaler_out_cfg;
	FB_CFG fb_h264_out_I_cfg;
	FB_CFG fb_h264_out_P_cfg;
	FB_CFG fb_h264_out_SKIP_cfg;
	FB_CFG fb_msg_notify_cfg;
	#if (C_VID_INTERPOLATING_2K)
	FB_CFG fb_interp2K_ps_out_cfg; // Put pscaler bypass frame buffer
	FB_CFG fb_interp2K_msg_notify_cfg; // Notify interp2K to get pscaler bypass frame buffer
	#endif
	H264Enc_t h264_cfg;
}H264_REC_ARGS;

typedef struct JPEG_REC_ARGS_S
{
	FB_CFG fb_pscaler_out_cfg;
	FB_CFG fb_jpeg_out_cfg;
	FB_CFG fb_msg_notify_cfg;
	JpegEnc_t jpeg_cfg;
	JengDynaQ_t	jpeg_q_cfg;
	INT32U photo_jpeg_outbuf_change;
	INT32U photo_jpeg_write_out_size;
	INT8U flag_jpeg_trigger;
}JPEG_REC_ARGS;

typedef struct JPEG_OUTB_HEADER_S // For jpeg output header
{
	JPEG_Q_TABLE jpeg_q;
	INT8U jpeg_header[C_JPEG_HEADER_SIZE];
}JPEG_OUTB_HEADER;

typedef struct GET_JPEG_ARGS_S
{
	FB_CFG fb_msg_notify_cfg;
	JpegEnc_t jpeg_cfg;
	INT32S	w_file_handle;
	INT32U	w_file_name;
	INT8U	flag_need_enc;  			// 1: Need encode pscaler's bypass buffer to a jpeg.
	INT8U	flag_start_buf_get; 		// 1: Get pscaler's bypass  or jpeg's output. 0: finish getting
	INT8U	flag_start_fifo_buf_get; 	// 1: Get pscaler's bypass frame/fifo buffer
	INT8U 	flag_jpeg_trigger;
	INT8U 	flag_fifo_start;
	INT8U 	flag_get_fail;
	#if (C_VID_INTERPOLATING_2K)
	INT8U 	flag_fifo_done; // 1: JPEG ecncode done 2: H264 encode done
	#endif
}GET_JPEG_ARGS;

typedef struct FILE_PACKER_W_ARGS_S
{
	INT8U src;
	INT8U snapshot;
	INT8U track;
	INT8U type;
	INT32U ret_data;
	INT8U* data;//packet data
	INT32U size; // packet size
	INT32U pts; // ms
	INT32U frameType; 			// 0=H264_FRAME_TYPE_I or MJPEG, 1=H264_FRAME_TYPE_P, 2=Skip frame(not used by H264 engine, only for packer and cbuf_encoder_out[] management)
	INT32U *drop_time;
	INT32U Dummy_frame_cnt;   // how many dummy frm need to insert
}FILE_PACKER_W_ARGS;

typedef struct ENCODE_PARAM_S
{
	INT32U pscaler_bypass_out_cnt;
	INT32U in_data_format;
	INT32U h264_I_out_cnt; // JPEG out cnt in this member variable
	INT32U h264_I_out_size; // JPEG out size in this member variable
	INT32U h264_P_out_cnt;
	INT32U h264_P_out_size;
	INT32U h264_SKIP_out_cnt;
	INT32U h264_SKIP_out_size;
	INT32U bitPerSeconds;
	INT32U gopLen;
	INT32U frameRate;
	INT32U qp_start;
	INT32U qp_max;
	INT32U qp_min;
	INT32U hierPFrames;
	#if (C_VID_INTERPOLATING_2K)
	INT32U interp2K_bypass_out_cnt;
	#endif
}ENCODE_PARAMS;

#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
typedef struct HOST_USB_MJPEG_DEC_ARGS_S
{
	JpegEnc_t jpeg_cfg;
	INT32U 	keep_jpeg_buf;
	INT32U 	keep_jpeg_buf_size;
	INT8U 	flag_jpeg_trigger;
	INT8U 	flag_black_jpeg;
}HOST_USB_MJPEG_DEC_ARGS;
#endif

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#endif //__TASK_ENCODER_CFG_H__

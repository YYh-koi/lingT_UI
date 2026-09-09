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
#ifndef __LIB_SENSOR_ENGINE_H__
#define __LIB_SENSOR_ENGINE_H__

#include "customer.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
enum  {
	PREVIEW_SRC_FRONT = 0,
	PREVIEW_SRC_REAR,
#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
	PREVIEW_SRC_UVC,
#endif
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	PREVIEW_SRC_WIFI,
#endif
	PREVIEW_SRC_TOTAL
};

typedef enum
{
	PREVIEW_AP_MODE_RECORD,
	PREVIEW_AP_MODE_CAPTURE,
	PREVIEW_AP_MODE_TOTAL
} PREVIEW_AP_MODE;

typedef enum
{
	PREVIEW_IN_PATH_NONE,
	PREVIEW_IN_PATH_CDSP,
	PREVIEW_IN_PATH_CSI0,
	PREVIEW_IN_PATH_CSI1,
	PREVIEW_IN_PATH_USB
} PREVIEW_IN_PATH;

typedef enum
{
	Y_ONLY_IN_PATH_NONE,
	Y_ONLY_IN_PATH_CDSP,
	Y_ONLY_IN_PATH_CSI0,
	Y_ONLY_IN_PATH_DRAM
} Y_ONLY_IN_PATH;

typedef enum
{
	//show front image only(0) 
	PREVIEW_MODE_FRONT,

	//show rear image only(1) 
	PREVIEW_MODE_REAR,

	//front image plus a PIP rear image(2) 
	PREVIEW_MODE_PIP_REAR,

	//rear image plus a PIP front image(3) 
	PREVIEW_MODE_PIP_FRONT,

	//divided to 2 horizontal windows(4) 
	PREVIEW_MODE_PBP,

	//show nothing(5) 
	PREVIEW_MODE_NONE,

	//painting back a car lines on rear image keep ratio(6) 
	PREVIEW_MODE_BACK_KEEP_RATIO,

	//painting back a car lines on rear image(7) 
	PREVIEW_MODE_BACK,

	//preview is waiting stop(8) 
	PREVIEW_MODE_STOP_WAIT,

	//preview is stopped(9) 
	PREVIEW_MODE_STOP,

	//a special action to perform mode switching(10) 
	PREVIEW_ACT_SWITCH,
} PREVIEW_MODE;

//number of normal preview mode
#define PREVIEW_MODE_TOTAL	(1 + PREVIEW_MODE_PBP)

//number of valid preview mode
#define PREVIEW_MODE_VALID	(1 + PREVIEW_MODE_BACK)

//number of preview related action
#define PREVIEW_ACT_TOTAL	(1 + PREVIEW_ACT_SWITCH)

#define SCALER_BUF_A_DONE	0x01
#define SCALER_BUF_B_DONE	0x02

typedef enum
{
	ENC_ENGINE_ACT_CHECK = 0,
	ENC_ENGINE_ACT_SET,
	ENC_ENGINE_ACT_CLEAR
} ENC_ENGINE_ACTION;

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef INT32U	(*PREVIEW_BUF_FUN_MALLOC) (void);
typedef void	(*PREVIEW_BUF_FUN_FREE) (INT32U buf);
typedef void	(*PREVIEW_BUF_FUN_SHOW) (INT32U buf, INT32U offset, INT8U car_back, INT16U top, INT16U bot, INT16U left, INT16U right);

typedef struct VID_REC_ARGS_S
{
	INT32U	sensor_width;
	INT32U	sensor_height;
	INT32U	vid_rec_fifo_total_count;	// total count
	INT32U	vid_rec_fifo_count_idx;
	INT32U	vid_rec_fifo_line_len;
	INT32U	vid_rec_fifo_data_size;
	INT32U	vid_timestamp_start_idx;
	INT32U	vid_timestamp_end_idx;
	INT32U (*fifo_ready_notify) (INT32U fifoMsg, INT32U fifoAddrs, INT32U fifoIdx);
	INT32U (*fifo_buffer_addrs_get) (void);
	INT32S (*fifo_buffer_addrs_put) (INT32U addr);
} VID_REC_ARGS;

typedef struct SCALER_ARG_S
{
	INT32U	scaler_in_width;
	INT32U	scaler_in_height;
	INT32U	scaler_out_width;
	INT32U	scaler_out_height;
	INT32U	scaler_in_buffer_addrs;
	INT32U	scaler_out_buffer_addrs;
	INT32U	scaler_fifo_count;
	INT32U	scaler_output_format;
	INT32U (*scaler_ready_notify) (INT32U fifoMsg, INT32U fifoAddrs);
} SCALER_ARG;

typedef struct IMG_CAPTURE_ARGS_S
{
	INT32S		jpeg_file_handle;
	SCALER_ARG	scaler_args;
} IMG_CAPTURE_ARGS;

typedef struct Y_ONLY_IMG_ARGS_S
{
    INT32U address;
    INT16U width;
    INT16U height;
}Y_ONLY_IMG_ARGS;

typedef struct Y_ONLY_POOL_ARGS_S
{
	INT32U *address;
	INT8U *hold;
	INT8U get;
	INT8U free;
	INT8U in_use_size;
	INT8U max_size;
}Y_ONLY_POOL_ARGS;

typedef struct Y_ONLY_QUEUE_S
{
	INT32U *address;
	INT16U *data_index;
	INT16U total_index;
	INT8U push;
	INT8U pull;
	INT8U size;
}Y_ONLY_QUEUE;

typedef struct Y_ONLY_HANDLE_S
{
	Y_ONLY_POOL_ARGS pool;
	Y_ONLY_QUEUE queue;
}Y_ONLY_HANDLE;


/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
#if defined(C_Y_ONLY_FUNCTION) && C_Y_ONLY_FUNCTION == CUSTOM_ON
extern Y_ONLY_HANDLE y_only_out_handle;
extern Y_ONLY_HANDLE y_only_in_handle;
#endif
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void
sensor_preview_buf_func_set(
	PREVIEW_BUF_FUN_MALLOC fun_malloc,
	PREVIEW_BUF_FUN_FREE fun_free,
	PREVIEW_BUF_FUN_SHOW fun_show
);

extern PREVIEW_MODE
sensor_preview_status_get(
	void
);

extern void
sensor_preview_src_path_set(
	INT32U src,
	PREVIEW_IN_PATH src_path
);

extern void
sensor_preview_src_format_set(
	INT32U src,
	INT32U format
);

extern void
sensor_preview_src_pixel_set(
	INT32U src,
	INT32U width,
	INT32U height
);

extern void
sensor_preview_zoom_pixel_set(
	INT32U src,
	INT16U width,
	INT16U height,
	INT16U w_start,
	INT16U h_start
);

extern void
sensor_preview_src_drop_half(
	INT32U src,
	INT32U en
);

extern void
sensor_preview_display_format_set(
	INT32U format
);

extern void
sensor_preview_display_pixel_set(
	INT32U width,
	INT32U height
);

extern void
sensor_preview_pip_pixel_set(
	INT16U width,
	INT16U height,
	INT16U pos_x,
	INT16U pos_y
);

#if defined(C_Y_ONLY_FUNCTION) && C_Y_ONLY_FUNCTION == CUSTOM_ON
extern void
sensor_y_only_src_path_set(
	Y_ONLY_IN_PATH path
);

extern void
sensor_y_only_src_format_set(
	INT32U format
);

extern void
sensor_y_only_in_width_set(
	INT16U width
);

extern void
sensor_y_only_in_height_set(
	INT16U height
);

extern void
sensor_y_only_clip_width_set(
	INT16U width
);

extern void
sensor_y_only_clip_height_set(
	INT16U height
);

extern void
sensor_y_only_out_width_set(
	INT16U width
);

extern void
sensor_y_only_out_height_set(
	INT16U height
);

extern void
sensor_y_only_in_fifo_line_set(
	INT8U line
);
extern void
sensor_y_only_out_pixel_offset_set(
	INT16U line,
	INT16U width
);

extern INT8U
y_only_fifo_malloc(
	Y_ONLY_HANDLE *handle,
	INT32U frame_size,
	INT8U buff_length,
	INT16U total_index
);

extern void
y_only_fifo_free(
	Y_ONLY_HANDLE *handle
);

extern INT32U
y_only_fifo_get_read_address(
	Y_ONLY_HANDLE *handle,
	INT16U *index
);

extern INT8U
y_only_fifo_read_done(
	Y_ONLY_HANDLE *handle,
	INT32U done_address
);
#endif

extern INT32S
sensor_preview_mode_set(
	PREVIEW_MODE mode,
	INT32U wait
);

extern INT32S
sensor_preview_start(
	void
);

extern INT32S
sensor_preview_wait_stop(
	void
);

extern INT32S
sensor_preview_uninit(
	void
);

extern INT32S
preview_q_malloc(
	INT16U buf_cnt
);

extern INT32S
sensor_preview_set_buf_random_time(
	INT32U buf
);

extern INT32U
sensor_record_statue_get_front(
	void
);

extern INT32U
sensor_record_statue_get_rear(
	void
);

extern void
sensor_preview_line_set(
	INT32U src,
	INT32U enc_line
);

extern void
sensor_record_line_set(
	INT32U src,
	INT32U enc_line
);

extern void
sensor_record_start_front(
	VID_REC_ARGS *pVidRecArgs
);

extern void
sensor_record_start_rear(
	VID_REC_ARGS *pVidRecArgs
);

extern void
sensor_record_stop_front(
	void
);

extern void
sensor_record_stop_rear(
	void
);

extern void
sensor_record_stop(
	void
);

extern INT32U
sensor_record_rear_state_get(
	void
);

extern INT32S
sensor_capture_start_front(
	IMG_CAPTURE_ARGS *pImgCaptureArgs
);

extern INT32S
sensor_capture_start_rear(
	IMG_CAPTURE_ARGS *pImgCaptureArg
);

extern INT32U
record_continue_get_front(
	void
);

extern INT32U
record_continue_get_rear(
	void
);

extern void
capture_scale_start_front(
	SCALER_ARG *pScaleArgs
);

extern void
capture_scale_start_rear(
	SCALER_ARG *pScaleArgs
);

extern INT32S
lib_get_jpeg_enc_process(
    INT32U src
);

extern void
sensor_preview_need_display_set(
	INT32U en
);

extern void
sensor_preview_display_offset_set(
	INT32U byte_len
);

extern void
sensor_preview_pbp_crop_back_ratio_set(
	INT8U pbp_crop,
	INT8U back_ratio
);

extern void
sensor_preview_out_crop_set(
	INT32U src,
	INT8U enable,
	INT16U out_w_start,
	INT16U out_h_start,
	INT16U out_off_x,
	INT16U out_off_y
);

extern void
sensor_preview_out_crop_get(
	INT32U src,
	INT8U *out_crop,
	INT16U *out_off_x,
	INT16U *out_off_y,
	INT16U *out_w_start,
	INT16U *out_h_start,
	INT16U *out_w_start_next,
	INT16U *out_h_start_next
);

extern INT32S
lib_h264_setting(
	H264Enc_t *pH264Enc
);

extern INT8U
enc_src_to_jpeg(
	INT32U src
);

extern INT32S
front_stream_init(
	void
);

extern INT32S
rear_stream_init(
	void
);

extern INT32S
front_packer_start(
	MSG_Q_ID notifyQ,
	VID_INFO* args
);

extern INT32S
lib_JPEG_process(
	INT8U src
);


extern INT32S
lib_get_jpeg_done_process(
	INT32U src
);

extern INT32S
lib_photo_write_process(
	INT32U src
);

extern INT32S
lib_pscaler_scaleup_process(
	INT32U src
);

extern INT32S
front_photo_buf_set(
	MSG_Q_ID notifyQ,
	VID_INFO* args
);

extern INT32S
front_photo_start(
	VID_INFO* args
);

extern INT32S
rear_photo_buf_set(
	MSG_Q_ID notifyQ,
	VID_INFO* args
);

extern INT32S
rear_photo_start(
	VID_INFO* args
);

extern INT32S
lib_jpeg_header_create(
	JpegEnc_t *pJpegEnc
);

extern INT32S
lib_jpeg_setting(
	JpegEnc_t *pJpegEnc
);

extern INT32S
front_encode_buf_set(
	MSG_Q_ID notifyQ,
	VID_INFO* args
);

extern INT32S
front_encode_start(
	VID_INFO* args
);

extern INT32S
front_encode_stop(
	void
);

extern INT32S
rear_encode_buf_set(
	MSG_Q_ID notifyQ,
	VID_INFO* args
);

extern INT32S
rear_encode_start(
	VID_INFO* args
);

extern INT32S
rear_encode_stop(
	void
);

extern INT32S
front_encode_buf_release(
	void
);

extern INT32S
front_encode_engine_status_clear(
	VID_INFO* pVid_info
);

extern INT32S
rear_encode_buf_release(
	void
);

extern INT32S
rear_encode_engine_status_clear(
	VID_INFO* pVid_info
);

extern INT32S
lib_pscaler_process(
	INT32U src
);

extern INT32S
lib_H264_process(
	INT8U src
);

extern INT32S
lib_pscaler_stream_init(
	INT32U src
);

extern INT8U
get_idle_jpeg(
	INT8S user_assign
);

extern void
release_used_jpeg(
	JPEG_IDX jpeg_idx
);

#if C_SUPORT_GET_JPG_BUF
extern INT32S
lib_get_jpeg_buf_set(
	INT8U 	src,
	INT32U	blocks,
	FB_CFG* pMsgNotifyCfg
);

extern INT32S
lib_get_jpeg_buf_release(
	INT8U src,
	FB_CFG* pMsgNotifyCfg
);
#endif

#if C_VID_REC_USE_SYS_FIFO
extern INT32S
lib_encode_system_fifo_register(
	INT32U src
);

extern INT32S
lib_encode_system_fifo_release(
	INT32U src
);

extern INT32U
lib_encode_system_fifo_mapping(
	INT32U src,
	INT32U buffer
);

extern INT32U
lib_encode_system_fifo_unmapping(
	INT32U src,
	INT32U buffer
);
#endif

extern INT32S preview_slide_up(INT32U pixel, INT8U wait);
extern INT32S preview_slide_down(INT32U pixel, INT8U wait);
extern INT32S photo_output_buffer_init(VID_INFO* args,INT32U jpeg_out_cnt,INT32U jpeg_out_size);

extern INT32S video_encode_sensor_on(INT32U ap_mode);
extern INT32S video_encode_preview_off(void);
extern INT32S video_encode_sensor_off(void);
extern INT32S video_encode_preview_mode_set(INT32U mode);
extern INT32S video_encode_record_stop(void);
extern INT32S video_encode_record_start(void);

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
extern void wifi_encode_buf_notify(INT32U data);
extern INT32S wifi_stream_init(void);
extern INT32S wifi_encode_buf_set(MSG_Q_ID notifyQ, VID_INFO* args);
extern INT32S wifi_encode_start(VID_INFO* args);
extern INT32S wifi_encode_stop(void);
extern INT32S wifi_encode_buf_release(void);
#endif

#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
extern void uvc_encode_buf_notify(INT32U data);
extern INT32S uvc_stream_init(void);
extern INT32S uvc_encode_buf_set(MSG_Q_ID notifyQ, VID_INFO* args);
extern INT32S uvc_encode_start(VID_INFO* args);
extern INT32S uvc_encode_stop(void);
extern INT32S uvc_encode_buf_release(void);
extern INT32S uvc_transmission_start(void);
extern INT32S uvc_transmission_stop(void);
#endif

#if (defined C_SENSOR_SAVE_RAW_DATA) && (C_SENSOR_SAVE_RAW_DATA)
INT32S lib_sensor_write_raw_process(INT8U src);
INT32S cb_func_sensor_write_raw_process(void);
#endif

/*== HOST USB MJPEG CAM/INTERPOLATING 2K ==*/
#if ((_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K))
extern INT32S front_preview_buffer_set(VID_INFO* args);
extern INT32S rear_preview_buffer_set(VID_INFO* args);
extern INT32S lib_preview_buffer_release(INT32U src,VID_INFO* args);
extern INT32S lib_host_2K_decode_JPEG_process(void);
extern INT32S lib_host_2K_frame_ready_process(INT8U src);
extern INT32S lib_host_2K_preview_done_process(INT8U src);
extern void interp2K_q_init(void);
extern INT32S lib_host_2K_decode_JPEG_finish_check(void);
#endif

/*== INTERPOLATING 2K ==*/
#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
extern INT32S lib_interp2K_scaleup_process(INT32U src);
#endif

/*== HOST USB MJPEG CAM ==*/
#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
extern void uvc_release_keep_buf(INT32U host_id);
extern INT8U uvc_q_ready_is_empty(void);
#endif


#endif // __LIB_SENSOR_ENGINE_H__

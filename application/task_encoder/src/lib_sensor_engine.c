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
#include "circular_buffer.h"
#include "application.h"

/*== E X I F ==*/
#if (defined C_PHOTO_JPEG_ADD_EXIF) && (C_PHOTO_JPEG_ADD_EXIF)
#include "exif_info_define.h"
#endif

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define STATUS_SKIP					-2

#define ENC_H264I_JPEG_IDX			0 	// For H264 I and JPEG. Note that this value must match H264_FRAME_TYPE_I
#define ENC_H264P_IDX				1 	// For H264 P. Note that this value must match H264_FRAME_TYPE_P
#define ENC_H264SKIP_IDX            2
#define ENC_H264_JPEG_IDX_TOTAL		3

typedef enum
{
	ACCESS_NONE,
	ACCESS_HW,
	ACCESS_DONE
} BUF_ST;

static enum
{
	PSCALER_MODE_STOP,
	PSCALER_MODE_DRAM,
	PSCALER_MODE_PIPE,
	PSCALER_MODE_PIPE_BYPASS
} pscaler_mode[PSCALER_NUM];

typedef enum{
	ENC_STREAM_RECORD_H264,
	ENC_STREAM_RECORD_JPEG,
	ENC_STREAM_PHOTO_JPEG,
	ENC_STREAM_NUM
} ENC_STREAM_ENUM;


#define PHOTO_PSCALER_SCALE_UP_LINE_CNT	32

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
typedef enum
{
	HOST_UVC_Q_IDLE, // For HOST_USB
	HOST_UVC_Q_READY,// For HOST_USB
	HOST_UVC_Q_DEC_DONE,// For JPEG DECODE
	HOST_UVC_Q_MAX
} HOST_UVC_Q_ENUM;
#endif

/*== HOST USB MJPEG CAM / INTERP2K IN COMMON ==*/
#if (_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K)
typedef enum
{
	INTERP_2K_CSI_READY,
	INTERP_2K_SCALER_READY,
	INTERP_2K_Q_MAX
} INTERPOLATING_2K_Q_ENUM;
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct ENC_STREAM_INFO_S{
	INT16U	stream_enable;
	INT16U	stream_status;
	INT16U	frame_rate;
	INT16U	frame_pts;					// ms per frame. frame_pts=1000/frame_rate
	INT32U  pack_frame_cnt;				// Total frame count that have been written into MOV packer
	INT32U  stream_start_time;			// OS time tick of first frame that has been written into MOV packer
	INT32U	stream_update_time;			// OS time tick of stream_start_time + pts of all frames that have been written into MOV packer
	INT32U	yuv_update_time;			// OS time tick when this buffer is received by lib_pscaler_notify_bypass()
	INT8U 	h264_previous_frame_type;	// Last frame type that has been written into MOV packer. 0=SLICE_TYPE_P, 1=SLICE_TYPE_HP, 2=SLICE_TYPE_I, 3=SLICE_TYPE_SKIP
	INT8U 	h264_idr_pic_id;			// H264 idr pic id of next I-frame
	INT8U 	h264_frame_num;				// H264 frame_num of next frame that will be written into MOV packer.
	INT8U 	h264_pic_order_cnt;			// H264 pic_order_cnt of next frame that will be written into MOV packer. Starts from 0 (I-frame). Last frame should be (GOP-1)
}ENC_STREAM_INFO_T;

typedef struct ENC_STREAM_CTRL_S{
	INT32S			(*stream_encode_func)(unsigned char src,void* ps_stream_ctrl,FB_INFO* fb_info);
	INT32S			(*stream_write_func)(unsigned char src, void* ps_stream_ctrl);
}ENC_STREAM_CTRL_T;

typedef struct PSCALER_CTRL_S{
	unsigned int	(*ps_get_func)();
	void			(*ps_free_func)(unsigned int data);
	void			(*ps_notify_func)(unsigned char src,void* ps_stream_ctrl,unsigned int data);
	void			(*ps_notify_scaleup_func)(unsigned char src,void* ps_stream_ctrl,unsigned int data);
	#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
	unsigned int	(*interp2K_get_func)();
	void			(*interp2K_free_func)(unsigned int data);
	void			(*interp2K_notify_func)(unsigned char src,void* ps_stream_ctrl,unsigned int data);
	FB_CFG* interp2K_ps_fb_cfg;
	FB_CFG* interp2K_msg_fb_cfg;
	#endif

	ENC_STREAM_INFO_T enc_stream_info;
	ENC_STREAM_CTRL_T *enc_stream_ctrl;
	VID_INFO* pVid_info;
	FB_CFG* ps_fb_cfg;
	FB_CFG* msg_fb_cfg;
	MSG_Q_ID ps_notify_q;
	INT8U flag_sys_fifo;
}PSCALER_STREAM_CTRL_T;

//======================================//
static struct
{
	INT32U	buf_preview;
	BUF_ST	st_main;
	BUF_ST	st_ext;
} *preview_q;

static PREVIEW_BUF_FUN_MALLOC	preview_buf_fun_malloc;
static PREVIEW_BUF_FUN_FREE		preview_buf_fun_free;
static PREVIEW_BUF_FUN_SHOW		preview_buf_fun_show;

/*== HOST USB MJPEG CAM ==*/
#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
struct uvc_ready_q
{
	INT32U	buf;
	INT32U	size;
};
#endif

/*== INTERPOLATING 2K ==*/
#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
static cbuf_handle_t cbuf_interp2K_bypass_out[PREVIEW_SRC_TOTAL];
static cbuf_handle_t cbuf_notify_interp2K[PREVIEW_SRC_TOTAL];
#endif

/*== HOST USB MJPEG CAM / INTERP2K IN COMMON ==*/
#if (_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K)
static cbuf_handle_t cbuf_interp2K_handle[2][INTERP_2K_Q_MAX];

#if (_SENSOR_USB_MJPEG_CAM_FRONT)
static INT32U interp2K_q_buf_FRONT[INTERP_2K_Q_MAX][VID_PSCALER_OUT_CNT_FRONT];
#endif

#if (_SENSOR_USB_MJPEG_CAM_REAR) || (C_VID_INTERPOLATING_2K)
static INT32U interp2K_q_buf_REAR[INTERP_2K_Q_MAX][VID_PSCALER_OUT_CNT_REAR];
#endif
#endif

#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
static volatile INT8U uvc_record_stop_req[PREVIEW_SRC_TOTAL] = {0};
#endif

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
// F R O N T
static unsigned int front_pscaler_get_bypass(void);
static void front_pscaler_free_bypass(unsigned int data);
static void front_pscaler_notify_bypass(unsigned char src,void* p,INT32U data);
static void front_pscaler_notify_scaleup(unsigned char src,void* p,INT32U data);
// R E A R
static unsigned int rear_pscaler_get_bypass(void);
static void rear_pscaler_free_bypass(unsigned int data);
static void rear_pscaler_notify_bypass(unsigned char src,void* p,INT32U data);
static void rear_pscaler_notify_scaleup(unsigned char src,void* p,INT32U data);

// Video Encode H264
static INT32S lib_encode_H264(unsigned char src,void* p,FB_INFO* fb_info);
static INT32S lib_enc_write_H264(unsigned char src,void* p);

// Video Encode JPEG
static INT32S lib_encode_JPEG(unsigned char src,void* p,FB_INFO* fb_info);
static INT32S lib_enc_write_JPEG(unsigned char src,void* p);

// Photo Capture JPEG
static INT32S lib_photo_JPEG(unsigned char src,void* p,FB_INFO* fb_info);
static INT32S lib_photo_write_JPEG(unsigned char src,void* p);
static INT32S lib_photo_process(INT32U src);

// W I F I
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
static unsigned int wifi_pscaler_get_bypass(void);
static void wifi_pscaler_free_bypass(unsigned int data);
static void wifi_pscaler_notify_bypass(unsigned char src,void* p,INT32U data);
static void wifi_stream_buf_release(INT32U data,INT32U frameType);
#endif

// U V C
#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
static unsigned int uvc_pscaler_get_bypass(void);
static void uvc_pscaler_free_bypass(unsigned int data);
static void uvc_pscaler_notify_bypass(unsigned char src,void* p,INT32U data);
//static void uvc_stream_buf_release(INT32U data,INT32U frameType);
#endif

#if C_SUPORT_GET_JPG_BUF
void lib_get_jpeg_buf_notify(unsigned char src,PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl,FB_INFO* fb_info,INT8U msg_send);
void lib_get_jpeg_pscaler_buf_release(unsigned char src,PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl);
#endif

/*== INTERPOLATING 2K ==*/
#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
void lib_interp2K_csi_callback_isr(INT32U CSI_Num,INT32U event);
static unsigned int interp2K_get_bypass(void);
static void interp2K_free_bypass(unsigned int buf);
static void interp2K_notify_bypass(unsigned char src,void* p,INT32U data);
static void interp2K_scale_up_restart(unsigned char src,INT32U buf_idx);
INT32S lib_interp2K_preview_done_process(INT8U src);
INT32S lib_interp2K_frame_ready_process(INT32U src,INT32U ScalerOrPscaler);
INT32S lib_interp2K_frame_check(INT32U src);
void preview_scaler_parm_set(INT32U scaler,INT32U mode,INT32U runtime,void (*ps_isr) (INT32U, INT32U));
INT32S interp2k_preview_image_put(INT32U src,INT32U buf);
void lib_interp2K_scaleup_state_clear(PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl);
static void interp2K_scale_up_err_handle(INT32U src,INT32U buf_idx,VID_INFO* pVid_info);
#endif

/*== HOST USB MJPEG CAM / INTERP2K IN COMMON ==*/
#if (_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K)
INT32U lib_interp2K_scaler_preview_frame_get(INT32U src);
INT32S lib_interp2K_scaler_preview_done_notify(INT32U src, INT32U buf);
#endif

/*== E X I F ==*/
#if (defined C_PHOTO_JPEG_ADD_EXIF) && (C_PHOTO_JPEG_ADD_EXIF)
INT32S lib_photo_exif_add(INT32U src,INT32U src_buf,VID_INFO* pVid_info);
#endif

#if STATISTICS_DUMP
extern INT32U front_wr_cnt;
extern INT32U rear_wr_cnt;
extern INT32U wifi_wr_cnt;
extern INT32U dummy_2k_cnt;
extern INT32U jpeg_done_cnt;
extern INT32U skip_front_wr_cnt;
extern INT32U skip_rear_wr_cnt;
#endif

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static ENC_STREAM_CTRL_T enc_stream[ENC_STREAM_NUM] =
{
//ENC_STREAM_RECORD_H264
	{
		lib_encode_H264,
		lib_enc_write_H264,
	}
	,
//ENC_STREAM_RECORD_JPEG
	{
		lib_encode_JPEG,
		lib_enc_write_JPEG,
	}
	,
//ENC_STREAM_PHOTO_JPEG
	{
		lib_photo_JPEG,
		lib_photo_write_JPEG,
	}
};

static PSCALER_STREAM_CTRL_T pscaler_stream[PREVIEW_SRC_TOTAL] =
{
//PREVIEW_SRC_FRONT
	{
		front_pscaler_get_bypass,
		front_pscaler_free_bypass,
		front_pscaler_notify_bypass,
		front_pscaler_notify_scaleup,
		#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
		interp2K_get_bypass,
		interp2K_free_bypass,
		interp2K_notify_bypass,
		#endif
	}
	,
//PREVIEW_SRC_REAR
	{
		rear_pscaler_get_bypass,
		rear_pscaler_free_bypass,
		rear_pscaler_notify_bypass,
		rear_pscaler_notify_scaleup,
	}
#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
	,
	{
		uvc_pscaler_get_bypass,
		uvc_pscaler_free_bypass,
		uvc_pscaler_notify_bypass,
	}
#endif
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	,
	{
		wifi_pscaler_get_bypass,
		wifi_pscaler_free_bypass,
		wifi_pscaler_notify_bypass,
	}
#endif
};

static cbuf_handle_t cbuf_pscaler_bypass_out[PREVIEW_SRC_TOTAL];
static cbuf_handle_t cbuf_encoder_out[PREVIEW_SRC_TOTAL][ENC_H264_JPEG_IDX_TOTAL];
static cbuf_handle_t cbuf_notify_enc[PREVIEW_SRC_TOTAL];
static INT32U	scalerOutputStatus[PREVIEW_SRC_TOTAL];
static volatile INT8U scalerOutputDone[PREVIEW_SRC_TOTAL];
static volatile INT8U scalerInputDone[PREVIEW_SRC_TOTAL];
static cbuf_handle_t cbuf_notify_get_jpeg[PREVIEW_SRC_TOTAL];

//======================================//
static INT8U 	preview_out_crop[PREVIEW_SRC_TOTAL];
INT32U	pscaler_busy[PSCALER_NUM];

static INT8U    preview_out_pbp_crop;
static INT8U    preview_out_back_ratio;
static INT16U   preview_out_back_off_x;
static INT16U   preview_out_back_off_y;

// drop half of FPS from sensor
static INT8U	preview_src_drop_half[PREVIEW_SRC_TOTAL];

// source image format
static INT32U	preview_src_format[PREVIEW_SRC_TOTAL];
static INT32U	preview_src_width[PREVIEW_SRC_TOTAL];
static INT32U	preview_src_height[PREVIEW_SRC_TOTAL];
static INT32U	preview_src_path[PREVIEW_SRC_TOTAL];

static INT16U	preview_zoom_width[PREVIEW_SRC_TOTAL];
static INT16U	preview_zoom_height[PREVIEW_SRC_TOTAL];
static INT16U	preview_zoom_w_start[PREVIEW_SRC_TOTAL];
static INT16U	preview_zoom_h_start[PREVIEW_SRC_TOTAL];

static volatile INT16U   preview_out_w_start[PREVIEW_SRC_TOTAL];
static volatile INT16U   preview_out_h_start[PREVIEW_SRC_TOTAL];
static volatile INT16U   preview_out_w_start_next[PREVIEW_SRC_TOTAL];
static volatile INT16U   preview_out_h_start_next[PREVIEW_SRC_TOTAL];
#if C_DISP_SLIDE_SAVE
static volatile INT16U   preview_out_w_start_reversing;
static volatile INT16U   preview_out_h_start_reversing;
static volatile INT16U   preview_out_w_start_reversing_next;
static volatile INT16U   preview_out_h_start_reversing_next;
static INT8U preview_slide_been_set;
static INT8U preview_slide_been_set_reversing;
#endif

static INT16U   preview_out_off_x[PREVIEW_SRC_TOTAL];
static INT16U   preview_out_off_y[PREVIEW_SRC_TOTAL];

// y only image format

#if defined(C_Y_ONLY_FUNCTION) && C_Y_ONLY_FUNCTION == CUSTOM_ON
static INT8U	y_only_src_path;
static INT8U	y_only_src_format;
static INT16U	y_only_in_width;
static INT16U	y_only_in_height;
static INT16U	y_only_clip_width;
static INT16U	y_only_clip_height;
static INT16U	y_only_out_width;
static INT16U	y_only_out_height;
static PIPELINE_SCALER_IN_FIFO_LINES	y_only_in_fifo_line;
static INT32U	y_only_pixel_offset;

Y_ONLY_HANDLE y_only_out_handle = {.pool = {0}, .queue = {0}};
Y_ONLY_HANDLE y_only_in_handle = {.pool = {0}, .queue = {0}};
static Y_ONLY_IMG_ARGS y_only_img_args;
#endif

// display image format
static INT32U	preview_display_format;
static INT32U	preview_display_width;
static INT32U	preview_display_height;

// pip window size
static INT16U	preview_pip_width;
static INT16U	preview_pip_height;
static INT16U   preview_pip_x;
static INT16U   preview_pip_y;

//	pscaler bypass line for record
//static INT32U	pscaler_scale_line[PREVIEW_SRC_TOTAL];
static INT32U	pscaler_bypass_line[PREVIEW_SRC_TOTAL];

//inner state, current sensor state
static PREVIEW_MODE _preview_mode = PREVIEW_MODE_STOP;

// PREVIEW_ACT_SWITCH used, prev/next preview mode
static PREVIEW_MODE _preview_mode_from;
static PREVIEW_MODE _preview_mode_to;

static INT32U display_offset_byte_len;

static INT16U preview_q_size;

// next preview buffer to use by scaler
static INT32U	preview_q_head;

// next preview buffer to display
static INT32U	preview_q_tail;

// baking a car now
static INT32U	preview_car_baking;

// PREVIEW_ACT_SWITCH used, is scaler parm set done
static volatile INT8U preview_switch_init[PSCALER_NUM];
static volatile INT8U preview_switch_done[PSCALER_NUM];

static INT32U	preview_need_display = 1;

// photo jpeg
static INT32U photo_devCnt;
static INT32U photo_pscaler_scale_up_buf[PREVIEW_SRC_TOTAL][2];


#if C_SUPORT_GET_JPG_BUF
SNAPSHOT_INFO	gSnapshotInfo_Front;
SNAPSHOT_INFO	gSnapshotInfo_Rear;
#endif

static INT8U 	jpeg_engine_assign[JPEG_IDX_MAX]= {0};

/*== HOST USB MJPEG CAM ==*/
#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
static INT8U  uvc_ready_q_idx[2] = {0};
static cbuf_handle_t cbuf_host_uvc_q_handle[2][HOST_UVC_Q_MAX];

#if(_SENSOR_USB_MJPEG_CAM_FRONT)
static struct uvc_ready_q uvc_put_ready_FRONT[HOST_USB_MJPEG_Q_CNT_FRONT];
static INT32U host_uvc_q_buf_FRONT[HOST_UVC_Q_MAX][HOST_USB_MJPEG_Q_CNT_FRONT];
#endif

#if(_SENSOR_USB_MJPEG_CAM_REAR)
static struct uvc_ready_q uvc_put_ready_REAR[HOST_USB_MJPEG_Q_CNT_REAR];
static INT32U host_uvc_q_buf_REAR[HOST_UVC_Q_MAX][HOST_USB_MJPEG_Q_CNT_REAR];
#endif

static INT32U black_jpg[2];
static INT32U black_jpg_len[2];

#endif// ==HOST USB MJPEG CAM ==

/*== D E B U G ==*/
#if 1
static INT32U no_encode_out_cnt[PREVIEW_SRC_TOTAL][ENC_H264_JPEG_IDX_TOTAL]={{0}};
#endif

/*== fun_sensor_preview_mode_set ==*/
enum  {
	SENSOR_PREVIEW_MODE_SET_SENSOR,
	SENSOR_PREVIEW_MODE_SET_USB_HOST,
	SENSOR_PREVIEW_MODE_SET_TOTAL
};

INT32S sensor_preview_mode_set_for_sensor(PREVIEW_MODE mode,INT32U wait);
INT32S sensor_preview_mode_set_for_usb_host(PREVIEW_MODE mode,INT32U wait);


static INT32S
(*fun_sensor_preview_mode_set[SENSOR_PREVIEW_MODE_SET_TOTAL])(PREVIEW_MODE mode, INT32U wait) =
{
	sensor_preview_mode_set_for_sensor,
	sensor_preview_mode_set_for_usb_host
};

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/
extern void ap_state_resource_time_stamp_position_y_set(INT16S y);
extern void cpu_draw_time_osd(INT32U buf, INT16U buf_w, INT16U buf_h, INT32U ap_mode, INT8U fifo_mode, INT8U draw_type, INT8U state);
extern INT32S file_packer_write_notify(FILE_PACKER_W_ARGS* pFile_packer_args);
extern void adjust_file_packer_prio(void);

#if C_TIME_LAPSE
extern INT32S time_lapse_check(INT8U src);
#endif

/*== HOST USB MJPEG CAM ==*/
#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
extern INT8U black_jpg_VGA[];
extern const INT32U black_jpg_len_VGA;
extern INT8U black_jpg_720P[];
extern const INT32U black_jpg_len_720P;
#endif

/*== HOST USB MJPEG CAM / INTERP2K IN COMMON ==*/
#if (_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K)
extern INT32S host_2K_mjpeg_ready_notify(void);
extern INT32S host_2K_frame_ready_notify(INT8U src);
extern INT32S host_2K_preview_done_notify(INT8U src);
#endif

//static INT32S lib_photo_JPEG_output_restart(unsigned char src, void* p);
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
sensor_preview_need_display_set(
	INT32U en
)
{
	preview_need_display = en;
}

void
sensor_preview_out_crop_set(
	INT32U src,
	INT8U enable,
	INT16U out_w_start,
	INT16U out_h_start,
	INT16U out_off_x,
	INT16U out_off_y
)
{
    preview_out_w_start[src] = out_w_start;
    preview_out_w_start_next[src] = out_w_start;
    preview_out_h_start[src] = out_h_start;
    preview_out_h_start_next[src] = out_h_start;

    preview_out_off_x[src] = out_off_x;
    preview_out_off_y[src] = out_off_y;

	preview_out_crop[src] = enable;
}

void
sensor_preview_pbp_crop_back_ratio_set(
	INT8U pbp_crop,
	INT8U back_ratio
)
{
    preview_out_pbp_crop = pbp_crop;
    preview_out_back_ratio = back_ratio;
}

void
sensor_preview_out_crop_get(
	INT32U src,
	INT8U *out_crop,
	INT16U *out_off_x,
	INT16U *out_off_y,
	INT16U *out_w_start,
	INT16U *out_h_start,
	INT16U *out_w_start_next,
	INT16U *out_h_start_next
)
{
	*out_crop = preview_out_crop[src];
	*out_off_x = preview_out_off_x[src];
	*out_off_y = preview_out_off_y[src];
	*out_w_start = preview_out_w_start[src];
	*out_h_start = preview_out_h_start[src];
	*out_w_start_next = preview_out_w_start_next[src];
	*out_h_start_next = preview_out_h_start_next[src];
}

void
sensor_preview_out_crop_set_init(
	INT32U src,
	INT8U out_crop,
	//INT8U slide_flag,
	INT16U out_off_x,
	INT16U out_off_y,
	INT16U out_w_start,
	INT16U out_h_start,
	INT16U out_w_start_next,
	INT16U out_h_start_next
)
{
	preview_out_crop[src] = out_crop;
	//preview_slide_been_set = slide_flag;
	preview_out_off_x[src] = out_off_x;
	preview_out_off_y[src] = out_off_y;
	preview_out_w_start[src] = out_w_start;
	preview_out_h_start[src] = out_h_start;
	preview_out_w_start_next[src] = out_w_start_next;
	preview_out_h_start_next[src] = out_h_start_next;
}
#if C_DISP_SLIDE_SAVE
void
sensor_preview_out_crop_reversing_set(
	INT16U out_w_start,
	INT16U out_h_start,
	INT16U out_w_start_next,
	INT16U out_h_start_next
)
{
	preview_out_w_start_reversing = out_w_start;
	preview_out_h_start_reversing = out_h_start;
	preview_out_w_start_reversing_next = out_w_start_next;
	preview_out_h_start_reversing_next = out_h_start_next;
}

void
sensor_preview_out_crop_reversing_get(
	INT16U *out_w_start,
	INT16U *out_h_start,
	INT16U *out_w_start_next,
	INT16U *out_h_start_next
)
{
	*out_w_start = preview_out_w_start_reversing;
	*out_h_start = preview_out_h_start_reversing;
	*out_w_start_next = preview_out_w_start_reversing_next;
	*out_h_start_next = preview_out_h_start_reversing_next;
}

void
sensor_preview_out_crop_flag_get(
	INT8U *crop_flag
)
{
	*crop_flag = preview_slide_been_set;
}

void
sensor_preview_out_crop_flag_set(
	INT8U crop_flag
)
{
	preview_slide_been_set = crop_flag;
}

void
sensor_preview_out_crop_reversing_flag_get(
	INT8U *crop_flag
)
{
	*crop_flag = preview_slide_been_set_reversing;
}

void
sensor_preview_out_crop_reversing_flag_set(
	INT8U crop_flag
)
{
	preview_slide_been_set_reversing = crop_flag;
}
#endif
void
sensor_preview_buf_func_set(
	PREVIEW_BUF_FUN_MALLOC fun_malloc,
	PREVIEW_BUF_FUN_FREE fun_free,
	PREVIEW_BUF_FUN_SHOW fun_show
)
{
	preview_buf_fun_free	= fun_free;
	preview_buf_fun_show	= fun_show;
	preview_buf_fun_malloc	= fun_malloc;
}

void
sensor_preview_src_format_set(
	INT32U src,
	INT32U format
)
{
	preview_src_format[src] = format;
}

void
sensor_preview_src_pixel_set(
	INT32U src,
	INT32U width,
	INT32U height
)
{
	preview_src_width[src] = width;
	preview_src_height[src] = height;
}

void
sensor_preview_zoom_pixel_set(
	INT32U src,
	INT16U width,
	INT16U height,
	INT16U w_start,
	INT16U h_start
)
{
	preview_zoom_width[src]		= width;
	preview_zoom_height[src]	= height;
	preview_zoom_w_start[src]	= w_start;
	preview_zoom_h_start[src]	= h_start;
}

void
sensor_preview_display_format_set(
	INT32U format
)
{
	preview_display_format = format;
}

void
sensor_preview_display_pixel_set(
	INT32U width,
	INT32U height
)
{
	preview_display_width = width;
	preview_display_height = height;
}

void
sensor_preview_display_offset_set(
	INT32U byte_len
)
{
    display_offset_byte_len = byte_len;
}

void
sensor_preview_pip_pixel_set(
	INT16U width,
	INT16U height,
	INT16U pos_x,
	INT16U pos_y
)
{
	preview_pip_width = width;
	preview_pip_height = height;
	preview_pip_x = pos_x & ~0x0007;
	preview_pip_y = pos_y & ~0x0001;

	if(preview_pip_width > preview_display_width/2)
	{
        preview_pip_width = preview_display_width/2;
	}

	if(preview_pip_height > preview_display_height/2)
	{
        preview_pip_height = preview_display_height/2;
	}

	if(preview_pip_x > (preview_display_width - 16))
	{
        preview_pip_x = preview_display_width - preview_pip_width;
    }

	if(preview_pip_y > (preview_display_height - 8))
	{
        preview_pip_y = preview_display_height - preview_pip_height;
	}

	if((preview_pip_width + preview_pip_x) > preview_display_width)
	{
        preview_pip_width = preview_display_width - preview_pip_x;
	}

	if((preview_pip_height + preview_pip_y) > preview_display_height)
	{
        preview_pip_height = preview_display_height - preview_pip_y;
	}
}

void
sensor_preview_src_drop_half(
	INT32U src,
	INT32U en
)
{
	preview_src_drop_half[src] = en;
}

void
sensor_preview_src_path_set(
	INT32U src,
	PREVIEW_IN_PATH src_path
)
{
	preview_src_path[src] = src_path;
}

void
sensor_record_line_set(
	INT32U src,
	INT32U enc_line
)
{
	pscaler_bypass_line[src] = enc_line;
}

INT32S
sensor_preview_mode_set_for_sensor(
	PREVIEW_MODE mode,
	INT32U wait
)
{
	#if defined(C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
	VID_INFO*  pVid_info_front = &pStreamArgs->vid_info_front;
	#endif

	INT32U time_out_cnt = 0;
	INT8U err_time_out = 0;
	INT32U ret_addr;
	INT8U i,j;
	DISP_P frame;


	if(mode >= PREVIEW_ACT_TOTAL)
	{
		return STATUS_FAIL;
	}

	if(_preview_mode == PREVIEW_MODE_STOP_WAIT)
	{
		return STATUS_FAIL;
	}

	if(_preview_mode == PREVIEW_MODE_STOP)
	{
		return STATUS_FAIL;
	}

	while(wait && (_preview_mode == PREVIEW_ACT_SWITCH))
	{
		//DBG_PRINT("[TRACE]W1..\r\n");
		++time_out_cnt;
		if(time_out_cnt > 1000)
		{
			err_time_out = 1;
			//DBG_PRINT("TIME-OUT1\r\n");
			break;
		}

		osDelay(1);
	}

	osENTER_CRITICAL();

	if(_preview_mode != PREVIEW_ACT_SWITCH)
	{
		_preview_mode_from = _preview_mode;
	}

	_preview_mode_to = mode;

	// if in swtching, force re-init again
	preview_switch_init[PSCALER_A] = 0;
	preview_switch_init[PSCALER_B] = 0;

	if(err_time_out)
	{
		_preview_mode = PREVIEW_MODE_STOP;
	}
	else
	{
		_preview_mode = PREVIEW_ACT_SWITCH;
	}

	osEXIT_CRITICAL();

	while(wait && (_preview_mode == PREVIEW_ACT_SWITCH))
	{
		//DBG_PRINT("[TRACE]W2..\r\n");
		++time_out_cnt;
		if(time_out_cnt > 1000)
		{
			err_time_out = 1;
			//DBG_PRINT("TIME-OUT2\r\n");
			break;
		}

		osDelay(1);
	}

	if(err_time_out)
	{
		osENTER_CRITICAL();
		_preview_mode = PREVIEW_MODE_STOP;

		/*
			Find PSCALER_A/PSCALER_B output buffer and return display buffer to display_free_queue
			when lose CDSP ISR
		*/
		#if defined(C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
		if(pVid_info_front->run_2K_flow_src) // This value is Greater than 0, that means front do scale-up flow
		{
			// Check SCALER_0
			drv_l1_scaler_output_addr_get(SCALER_0, &ret_addr,NULL,NULL);
			if(ret_addr != DUMMY_BUFFER_ADDRS && ret_addr != 0)
			{
				frame = displayaddr_to_displayframe(ret_addr);
				if(frame)
				{
					if(ap_display_queue_put(display_free_queue, frame) != STATUS_OK)
					{
						//DBG_PRINT("[ERROR]display_free_queue full\r\n");
					}
				}
			}
			drv_l1_scaler_output_addr_set(SCALER_0, DUMMY_BUFFER_ADDRS,DUMMY_BUFFER_ADDRS,DUMMY_BUFFER_ADDRS);

			for(i=PSCALER_A; i<=PSCALER_A; i++)
			{
				for(j=0; j<2; j++) // Buffer A/B
				{
					if(j & 1)
					{
						ret_addr = drv_l1_pscaler_output_B_buffer_get(i);
						if(ret_addr == DUMMY_BUFFER_ADDRS || ret_addr == 0)
						{
							continue;
						}
						drv_l1_pscaler_output_B_buffer_set(i, DUMMY_BUFFER_ADDRS);
					}
					else
					{
						ret_addr = drv_l1_pscaler_output_A_buffer_get(i);
						if(ret_addr == DUMMY_BUFFER_ADDRS || ret_addr == 0)
						{
							continue;
						}
						drv_l1_pscaler_output_A_buffer_set(i, DUMMY_BUFFER_ADDRS);
					}

					//DBG_PRINT("(P,B,A)==(%d,%d,0x%x)\r\n",i,j,ret_addr);

					frame = displayaddr_to_displayframe(ret_addr);
					if(frame)
					{
						if(ap_display_queue_put(display_free_queue, frame) != STATUS_OK)
						{
							//DBG_PRINT("[ERROR]display_free_queue full\r\n");
						}
					}
				}
			}
		}
		else
		#endif
		{
			for(i=PSCALER_A; i<=PSCALER_B; i++)
			{
				for(j=0; j<2; j++) // Buffer A/B
				{
					if(j & 1)
					{
						ret_addr = drv_l1_pscaler_output_B_buffer_get(i);
						if(ret_addr == DUMMY_BUFFER_ADDRS || ret_addr == 0)
						{
							continue;
						}
						drv_l1_pscaler_output_B_buffer_set(i, DUMMY_BUFFER_ADDRS);
					}
					else
					{
						ret_addr = drv_l1_pscaler_output_A_buffer_get(i);
						if(ret_addr == DUMMY_BUFFER_ADDRS || ret_addr == 0)
						{
							continue;
						}
						drv_l1_pscaler_output_A_buffer_set(i, DUMMY_BUFFER_ADDRS);
					}

					//DBG_PRINT("(P,B,A)==(%d,%d,0x%x)\r\n",i,j,ret_addr);

					frame = displayaddr_to_displayframe(ret_addr);
					if(frame)
					{
						if(ap_display_queue_put(display_free_queue, frame) != STATUS_OK)
						{
							//DBG_PRINT("[ERROR]display_free_queue full\r\n");
						}
					}
				}
			}
		}
		osEXIT_CRITICAL();
	}

	return STATUS_OK;
}

INT32S
sensor_preview_mode_set_for_usb_host(
	PREVIEW_MODE mode,
	INT32U wait
)
{
	INT32U time_out_cnt = 0, timeout_time = 2000, scaler;
    INT32S i;

	if(mode >= PREVIEW_ACT_TOTAL)
	{
		return STATUS_FAIL;
	}

	if(_preview_mode == PREVIEW_MODE_STOP)
	{
		return STATUS_FAIL;
	}

	for(scaler = PSCALER_A; scaler <= PSCALER_B; scaler++)
	{
        if(pscaler_busy[scaler])
        {
            while(pscaler_busy[scaler])
            {
                ++time_out_cnt;
                if(time_out_cnt > timeout_time)
                {
                    ERR_PRINT("\r\ntimeout1\r\n");
                    break;
                }

                osDelay(1);
            }
        }
    }

    if(!preview_q || !preview_q_size)
    {
        ERR_PRINT("preview_q error\r\n");
    }

	for(i = preview_q_tail; i != preview_q_head; ++i)
	{
		if(i >= preview_q_size)
		{
			i = 0;
			if(i == preview_q_head)
			{
				break;
			}
		}

		if(preview_q[i].buf_preview)
		{
			if(preview_buf_fun_free)
			{
				preview_buf_fun_free(preview_q[i].buf_preview);
			}
		}

		preview_q[i].buf_preview = 0;
		preview_q[i].st_main = ACCESS_NONE;
		preview_q[i].st_ext = ACCESS_NONE;
	}

	preview_q_tail = 0;
	preview_q_head = 0;

	preview_switch_init[PSCALER_A] = 0;
	preview_switch_init[PSCALER_B] = 0;

	osENTER_CRITICAL();
	_preview_mode = mode;
	osEXIT_CRITICAL();

	//DBG_PRINT("===>_preview_mode:%x\r\n",_preview_mode);

	return STATUS_OK;
}

INT32S
sensor_preview_mode_set(
	PREVIEW_MODE mode,
	INT32U wait
)
{
	#if(_SENSOR_USB_MJPEG_CAM_FRONT)
	VID_INFO*  pVid_info_front = &pStreamArgs->vid_info_front;
	#endif

	#if(_SENSOR_USB_MJPEG_CAM_FRONT)
	if(pVid_info_front->data_source == VID_SRC_USB)
	{
	 return (*fun_sensor_preview_mode_set[SENSOR_PREVIEW_MODE_SET_USB_HOST])(mode, wait);
	}
	else
    #endif
	{
	 return (*fun_sensor_preview_mode_set[SENSOR_PREVIEW_MODE_SET_SENSOR])(mode, wait);
	}

}

static INT32U
preview_src_to_ps(
	INT32U src
)
{
	switch(src)
	{
	case PREVIEW_SRC_FRONT:
		return PSCALER_A;
		break;

	case PREVIEW_SRC_REAR:
		return PSCALER_B;
		break;

	default:
		ERR_PRINT("unknown preview source\r\n");
		return PSCALER_B;
		break;
	}
}

static INT8U
preview_ps_to_src(
	INT32U ps
)
{
	switch(ps)
	{
	case PSCALER_A:
		return PREVIEW_SRC_FRONT;
		break;

	case PSCALER_B:
		return PREVIEW_SRC_REAR;
		break;

	default:
		ERR_PRINT("unknown preview scaler\r\n");
		return PREVIEW_SRC_FRONT;
		break;
	}
}

static INT32U
preview_src_width_get(
	INT32U scaler
)
{
	INT8U src = preview_ps_to_src(scaler);

	return preview_src_width[src];
}

static INT32U
preview_src_height_get(
	INT32U scaler
)
{
	INT8U src = preview_ps_to_src(scaler);

	return preview_src_height[src];
}
static INT32U
preview_zoom_width_get(
	INT32U scaler,
	INT32U mode
)
{
	INT8U src = preview_ps_to_src(scaler);

	switch(mode)
	{
		case PREVIEW_MODE_FRONT:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_zoom_width[src];
			}
			else
			{
				return preview_display_width;
			}
			break;
		case PREVIEW_MODE_BACK_KEEP_RATIO:
		case PREVIEW_MODE_REAR:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_display_width;
			}
			else
			{
				return preview_zoom_width[src];
			}
			break;
		case PREVIEW_MODE_PIP_REAR:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_zoom_width[src];
			}
			else
			{
				return preview_pip_width;
			}
			break;
		case PREVIEW_MODE_PIP_FRONT:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_pip_width;
			}
			else
			{
				return preview_zoom_width[src];
			}
			break;
		case PREVIEW_MODE_PBP:
			if(src == PREVIEW_SRC_FRONT)
			{
				return (preview_display_width >> 1) & ~0x0007;
			}
			else
			{
				return preview_display_width - ((preview_display_width >> 1) & ~0x0007);
			}
			break;
		default:
			return preview_display_width;
			break;
	}
}

static INT32U
preview_zoom_height_get(
	INT32U scaler,
	INT32U mode
)
{
	INT8U src = preview_ps_to_src(scaler);

	switch(mode)
	{
		case PREVIEW_MODE_FRONT:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_zoom_height[src];
			}
			else
			{
				return preview_display_height;
			}
			break;
		case PREVIEW_MODE_BACK_KEEP_RATIO:
		case PREVIEW_MODE_REAR:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_display_height;
			}
			else
			{
				return preview_zoom_height[src];
			}
			break;
		case PREVIEW_MODE_PIP_REAR:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_zoom_height[src];
			}
			else
			{
				return preview_pip_height;
			}
			break;
		case PREVIEW_MODE_PIP_FRONT:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_pip_height;
			}
			else
			{
				return preview_zoom_height[src];
			}
			break;
		case PREVIEW_MODE_PBP:
			return preview_display_height;
			break;
		default:
			return preview_display_height;
			break;
	}
}

static INT32U
preview_zoom_w_start_get(
	INT32U scaler,
	INT32U mode
)
{
	INT8U src = preview_ps_to_src(scaler);

	switch(mode)
	{
		case PREVIEW_MODE_BACK_KEEP_RATIO:
		case PREVIEW_MODE_FRONT:
		case PREVIEW_MODE_REAR:
			return preview_zoom_w_start[src];
			break;
		case PREVIEW_MODE_PIP_REAR:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_zoom_w_start[src];
			}
			else
			{
				return 0;
			}
			break;
		case PREVIEW_MODE_PIP_FRONT:
			if(src == PREVIEW_SRC_FRONT)
			{
				return 0;
			}
			else
			{
				return preview_zoom_w_start[src];
			}
			break;
		case PREVIEW_MODE_PBP:
			return 0;
			break;
		default:
			return 0;
			break;
	}
}

static INT32U
preview_zoom_h_start_get(
	INT32U scaler,
	INT32U mode
)
{
	INT8U src = preview_ps_to_src(scaler);

	switch(mode)
	{
		case PREVIEW_MODE_BACK_KEEP_RATIO:
		case PREVIEW_MODE_FRONT:
		case PREVIEW_MODE_REAR:
			return preview_zoom_h_start[src];
			break;
		case PREVIEW_MODE_PIP_REAR:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_zoom_h_start[src];
			}
			else
			{
				return 0;
			}
			break;
		case PREVIEW_MODE_PIP_FRONT:
			if(src == PREVIEW_SRC_FRONT)
			{
				return 0;
			}
			else
			{
				return preview_zoom_h_start[src];
			}
			break;
		case PREVIEW_MODE_PBP:
			return 0;
			break;
		default:
			return 0;
			break;
	}
}

static INT32U
preview_output_width_get(
	INT32U scaler,
	INT32U mode
)
{
	INT8U src = preview_ps_to_src(scaler);

	switch(mode)
	{
		case PREVIEW_MODE_PIP_REAR:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_display_width;
			}
			else
			{
				return preview_pip_width;
			}
			break;
		case PREVIEW_MODE_PIP_FRONT:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_pip_width;
			}
			else
			{
				return preview_display_width;
			}
			break;
		case PREVIEW_MODE_PBP:
			if(src == PREVIEW_SRC_FRONT)
			{
				return (preview_display_width >> 1) & ~0x0007;
			}
			else
			{
				return preview_display_width - ((preview_display_width >> 1) & ~0x0007);
			}
			break;
		default:
			return preview_display_width;
			break;
	}
}

static INT32U
preview_output_height_get(
	INT32U scaler,
	INT32U mode
)
{
	INT8U src = preview_ps_to_src(scaler);

	switch(mode)
	{
		case PREVIEW_MODE_PIP_REAR:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_display_height;
			}
			else
			{
				return preview_pip_height;
			}
			break;
		case PREVIEW_MODE_PIP_FRONT:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_pip_height;
			}
			else
			{
				return preview_display_height;
			}
			break;
		default:
			return preview_display_height;
			break;
	}
}

static INT32U
preview_buf_offset_get(
	INT32U scaler,
	INT32U mode
)
{
	INT8U	src = preview_ps_to_src(scaler);
	INT32U	offset_x, offset_y, offset = 0;

	switch(mode)
	{
		case PREVIEW_MODE_PIP_REAR:
			if(src == PREVIEW_SRC_FRONT)
			{
				offset_x = preview_out_off_x[src];
				offset_y = preview_out_off_y[src];
			}
			else
			{
				offset_x = preview_pip_x;
				offset_y = preview_pip_y;
			}
			break;
		case PREVIEW_MODE_PIP_FRONT:
			if(src == PREVIEW_SRC_FRONT)
			{
				offset_x = preview_pip_x;
				offset_y = preview_pip_y;
			}
			else
			{
				offset_x = preview_out_off_x[src];
				offset_y = preview_out_off_y[src];
			}
			break;
		case PREVIEW_MODE_PBP:
			if(src == PREVIEW_SRC_FRONT)
			{
				offset_x = 0;
				offset_y = 0;
			}
			else
			{
				offset_x = (preview_display_width >> 1) & ~0x0007;
				offset_y = 0;
			}
			break;
		case PREVIEW_MODE_FRONT:
			if(src == PREVIEW_SRC_FRONT)
			{
				offset_x = preview_out_off_x[src];
				offset_y = preview_out_off_y[src];
			}
			else
			{
				offset_x = 0;
				offset_y = 0;
			}
			break;
		case PREVIEW_MODE_BACK_KEEP_RATIO:
		case PREVIEW_MODE_REAR:
			if(src == PREVIEW_SRC_FRONT)
			{
				offset_x = 0;
				offset_y = 0;
			}
			else
			{
				offset_x = preview_out_off_x[src];
				offset_y = preview_out_off_y[src];
			}
			break;
		case PREVIEW_MODE_BACK:
			if(src == PREVIEW_SRC_FRONT)
			{
				offset_x = 0;
				offset_y = 0;
			}
			else
			{
				offset_x = preview_out_back_off_x;
				offset_y = preview_out_back_off_y;
			}
			break;
		default:
			offset_x = 0;
			offset_y = 0;
			break;
		}

		switch(preview_display_format)
		{
			case PIPELINE_SCALER_OUTPUT_FORMAT_YUYV:
			case PIPELINE_SCALER_OUTPUT_FORMAT_RGB565:
				offset_y *= preview_display_width;
				offset = offset_x + offset_y;
				offset <<= 1;
				break;
			case PIPELINE_SCALER_OUTPUT_FORMAT_GP420:
				offset_y &= ~0x0001;
				offset_y = (offset_y * preview_display_width * 3) >> 1;
				offset = offset_x + offset_y;
				break;
			default:
#if 0
				// debug
				DBG_PRINT("[ERROR] unsupported format @%s:%d\r\n", __func__, __LINE__);
#endif
				break;
	}

	return offset;
}

static INT32U
preview_pixel_offset_get(
	INT32U scaler,
	INT32U mode
)
{
	INT8U src = preview_ps_to_src(scaler);

	switch(mode)
	{
		case PREVIEW_MODE_PIP_REAR:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_out_off_x[src]? (preview_display_width - preview_zoom_width[src]) : 0;
			}
			else
			{
				return preview_display_width - preview_pip_width;
			}
			break;
		case PREVIEW_MODE_PIP_FRONT:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_display_width - preview_pip_width;
			}
			else
			{
				return preview_out_off_x[src]? (preview_display_width - preview_zoom_width[src]) : 0;
			}
			break;
		case PREVIEW_MODE_FRONT:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_out_off_x[src]? (preview_display_width - preview_zoom_width[src]) : 0;
			}
			else
			{
				return 0;
			}
			break;
		case PREVIEW_MODE_BACK_KEEP_RATIO:
		case PREVIEW_MODE_REAR:
			if(src == PREVIEW_SRC_FRONT)
			{
				return 0;
			}
			else
			{
				return preview_out_off_x[src]? (preview_display_width - preview_zoom_width[src]) : 0;
			}
			break;
		case PREVIEW_MODE_PBP:
			if(src == PREVIEW_SRC_FRONT)
			{
				return preview_display_width - ((preview_display_width >> 1) & ~0x0007);
			}
			else
			{
				return (preview_display_width >> 1) & ~0x0007;
			}
			break;
		default:
			return 0;
			break;
	}
}

static INT32U
preview_cut_win_en_get(
	INT32U scaler,
	INT32U mode
)
{
	INT8U src = preview_ps_to_src(scaler);

	switch(mode)
	{
		case PREVIEW_MODE_PIP_REAR:
			if(src == PREVIEW_SRC_FRONT)
			{
				return 1;
			}
			else
			{
				return 0;
			}
			break;

		case PREVIEW_MODE_PIP_FRONT:
			if(src == PREVIEW_SRC_FRONT)
			{
				return 0;
			}
			else
			{
				return 1;
			}
			break;

		default:
			return 0;
			break;
	}
}

static void
preview_pscaler_parm_set(
	INT32U scaler,
	INT32U mode,
	INT32U runtime,
	void (*ps_isr) (PS_ID, PS_ST)
)
{
	INT8U src = preview_ps_to_src(scaler);
	PIP_WINDOW_PARAM	pip_parm;

	#if 0
	FILTER_1D_PARAM		pFilter1DParam = {0, 0, 5, 6, 5, 4};
	#endif

	INT32U	inW, inH;
	INT32U	zoomW, zoomH;
	INT32U	outW, outH;
	INT32U	wFactor, hFactor;
	INT32U	wIntegration, hIntegration;
	INT32U	wOffset;
	INT32U	cutWin;
	INT32U	w_start, h_start;
	INT8U	h_first;
	INT32U	cropW, cropH;
	INT8U   crop_flag;
	INT16U  pip_x, pip_y, pip_width, pip_height;

	pip_x = preview_pip_x;
	pip_y = preview_pip_y;
	pip_width = preview_pip_width;
	pip_height = preview_pip_height;
	if(mode == PREVIEW_MODE_PIP_REAR)
	{
		if(pip_x >= preview_out_off_x[PREVIEW_SRC_FRONT])
		{
			pip_x -= preview_out_off_x[PREVIEW_SRC_FRONT];
		}
		else
		{
			pip_width = pip_width + pip_x - preview_out_off_x[PREVIEW_SRC_FRONT];
			pip_x = 0;
		}

		if(pip_y >= preview_out_off_y[PREVIEW_SRC_FRONT])
		{
			pip_y -= preview_out_off_y[PREVIEW_SRC_FRONT];
		}
		else
		{
			pip_height = pip_height + pip_y - preview_out_off_y[PREVIEW_SRC_FRONT];
			pip_y = 0;
		}
	}
	else if(mode == PREVIEW_MODE_PIP_FRONT)
	{
		if(pip_x >= preview_out_off_x[PREVIEW_SRC_REAR])
		{
			pip_x -= preview_out_off_x[PREVIEW_SRC_REAR];
		}
		else
		{
			pip_width = pip_width + pip_x - preview_out_off_x[PREVIEW_SRC_REAR];
			pip_x = 0;
		}

		if(pip_y >= preview_out_off_y[PREVIEW_SRC_REAR])
		{
			pip_y -= preview_out_off_y[PREVIEW_SRC_REAR];
		}
		else
		{
			pip_height = pip_height + pip_y - preview_out_off_y[PREVIEW_SRC_REAR];
			pip_y = 0;
		}
	}

	pip_parm.PipStartX	= pip_x;
	pip_parm.PipStartY	= pip_y;
	pip_parm.PipEndX	= pip_parm.PipStartX + pip_width;
	pip_parm.PipEndY	= pip_parm.PipStartY + pip_height;

	inW		= preview_src_width_get(scaler);
	inH		= preview_src_height_get(scaler);
	zoomW	= preview_zoom_width_get(scaler, mode);
	zoomH	= preview_zoom_height_get(scaler, mode);
	w_start	= preview_zoom_w_start_get(scaler, mode);
	h_start	= preview_zoom_h_start_get(scaler, mode);
	outW	= preview_output_width_get(scaler, mode);
	outH	= preview_output_height_get(scaler, mode);

	#if 0
	DBG_PRINT("(scaler,mode,inW,inH,zoomW,zoomH,outW,outH, w_start, h_start)=(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\r\n"
		, scaler, mode, inW, inH, zoomW, zoomH, outW, outH, w_start, h_start);
	#endif

	crop_flag = 0;
	if(preview_out_crop[src])
	{
		if(src == PREVIEW_SRC_FRONT)
		{
			switch(mode)
			{
				case PREVIEW_MODE_FRONT:
				case PREVIEW_MODE_PIP_REAR:
					crop_flag = preview_out_crop[src];
					break;
			}
		}
		else if(src == PREVIEW_SRC_REAR)
		{
			switch(mode)
			{
#if C_REV_BACK_SENSOR_FULL_SCREEN == CUSTOM_ON
				case PREVIEW_MODE_BACK://add for hxwl back sensor reversing mode full screen display by antionai@20230328
				case PREVIEW_MODE_BACK_KEEP_RATIO:
#endif
				case PREVIEW_MODE_REAR:
				case PREVIEW_MODE_PIP_FRONT:
					crop_flag = preview_out_crop[src];
					break;
			}
		}
	}

	if(preview_out_pbp_crop && (mode == PREVIEW_MODE_PBP))
	{
		cropW = inW*outH/inH;
		cropH = inH*outW/inW;

		if(cropH > outH)
		{
			zoomH = cropH>>1<<1;
			crop_flag = 1;
		}
		else if(cropW > outW)
		{
			zoomW = cropW>>4<<4;
			crop_flag = 1;
		}
		else
		{
			crop_flag = 0;
		}
	}
#if C_REV_BACK_SENSOR_FULL_SCREEN == CUSTOM_OFF
	if(mode == PREVIEW_MODE_BACK)
	{
		if(preview_out_back_ratio)
		{
			cropW = inW*outH/inH;
			cropH = inH*outW/inW;

			if(cropH > outH)
			{
				zoomW = cropW>>4<<4;
				zoomH = outH;
				crop_flag = 2;
			}
			else if(cropW > outW)
			{
				zoomH = cropH>>1<<1;
				zoomW = outW;
				crop_flag = 2;
			}
		}
		else
		{
			crop_flag = 0;
		}
		preview_out_back_off_x = 0;
		preview_out_back_off_y = 0;
	}
#endif
	if(zoomW >= (inW >> 1))
	{
		wIntegration = 0;
		wFactor = (inW << 16) / zoomW;
		if((inW << 16) % zoomW) ++wFactor;
	}
	else
	{
		wIntegration = 1;
		wFactor = ((zoomW << 16) / (inW - 1)) + 1;
		if((zoomW << 16) % inW) ++wFactor;
	}

	if(zoomH >= (inH >> 1))
	{
		hIntegration = 0;
		hFactor = (inH << 16) / zoomH;
		if((inH << 16) % zoomH) ++hFactor;
	}
	else
	{
		hIntegration = 1;
		hFactor = ((zoomH << 16) / (inH - 1)) + 1;
		if((zoomH << 16) % inH) ++hFactor;
	}

	if(inW > zoomW)
	{
		h_first = 1;
	}
	else
	{
		h_first = 0;
	}

	#if 0
	DBG_PRINT("(w_start, wIntegration, wFactor, h_start, hIntegration, hIntegration) = (%d, %d, %08x, %d, %d, %08x)\r\n"
		, w_start, wIntegration, wFactor, h_start, hIntegration, hFactor);
	#endif

	cutWin = preview_cut_win_en_get(scaler, mode);
	wOffset = preview_pixel_offset_get(scaler, mode);

#if C_REV_BACK_SENSOR_FULL_SCREEN == CUSTOM_OFF
	if((crop_flag == 2) && ((mode == PREVIEW_MODE_BACK)||(mode == PREVIEW_MODE_BACK_KEEP_RATIO)))
	{
		if(zoomW < outW)
		{
			wOffset = outW - zoomW;
			preview_out_back_off_x = wOffset >> 1;
			if(preview_out_back_off_x & 0x0001)
			{
				preview_out_back_off_x += 0x0001;
			}
			preview_out_back_off_y = 0;
		}
		else if(zoomH < outH)
		{
			wOffset = 0;
			preview_out_back_off_x = 0;
			preview_out_back_off_y = (outH - zoomH) >> 1;
			if(preview_out_back_off_y & 0x0001)
			{
				preview_out_back_off_y += 0x0001;
			}
		}
	}
#endif

	if(!runtime)
	{
		drv_l1_pscaler_init(scaler);

	  #if 0		// Enable this to enhance edge on TFT display
		drv_l1_pscaler_outedge_set(
			scaler
			, OUTEDGE_TYPE_1
			, OUTEDGE_CLP_64
			, OUTEDGE_COR_2
			, 8
		);
	  #endif

		switch(preview_src_path[src])
		{
			case PREVIEW_IN_PATH_CDSP:
				drv_l1_CdspSetYuvPscalePath(1);
				pscaler_mode[scaler] = PSCALER_MODE_PIPE_BYPASS;
				drv_l1_pscaler_input_source_set(scaler, PIPELINE_SCALER_INPUT_SOURCE_CDSP);
			break;
			case PREVIEW_IN_PATH_CSI0:
				drv_l1_csi_input_pscaler_set(CSI_0,1);
				pscaler_mode[scaler] = PSCALER_MODE_PIPE_BYPASS;
				drv_l1_pscaler_input_source_set(scaler, PIPELINE_SCALER_INPUT_SOURCE_CSI);
			break;
			case PREVIEW_IN_PATH_CSI1:
				drv_l1_csi_input_pscaler_set(CSI_1,1);
				pscaler_mode[scaler] = PSCALER_MODE_PIPE_BYPASS;
				drv_l1_pscaler_input_source_set(scaler, PIPELINE_SCALER_INPUT_SOURCE_CSI2);
			break;

			default:
				pscaler_mode[scaler] = PSCALER_MODE_DRAM;
				drv_l1_pscaler_input_source_set(scaler, PIPELINE_SCALER_INPUT_SOURCE_DRAM);
				drv_l1_pscaler_input_buffer_set(scaler, DUMMY_BUFFER_ADDRS);
				switch(preview_src_format[src])
				{
					case PIPELINE_SCALER_INPUT_FORMAT_YUYV:
						drv_l1_pscaler_input_format_set(scaler, PIPELINE_SCALER_INPUT_FORMAT_YUYV);
					break;
					case PIPELINE_SCALER_INPUT_FORMAT_GP420:
						drv_l1_pscaler_input_format_set(scaler, PIPELINE_SCALER_INPUT_FORMAT_GP420);
					break;
				}
				drv_l1_pscaler_HSYNC_latch_delay(scaler, 0x20);
			break;
		}

		drv_l1_pscaler_output_format_set(scaler, preview_display_format);

		drv_l1_pscaler_output_A_buffer_set(scaler, DUMMY_BUFFER_ADDRS);
		drv_l1_pscaler_output_B_buffer_set(scaler, DUMMY_BUFFER_ADDRS);
		drv_l1_pscaler_callback_register2(scaler, ps_isr);

		drv_l1_pscaler_bypass_output_A_buffer_set(scaler, DUMMY_BUFFER_ADDRS);
		drv_l1_pscaler_bypass_output_B_buffer_set(scaler, DUMMY_BUFFER_ADDRS);

	}

	switch(pscaler_bypass_line[src])
	{
		case 0:
			drv_l1_pscaler_bypass_output_fifo_line_set(scaler, inH, 0);
		break;
		default:
			drv_l1_pscaler_bypass_output_fifo_line_set(scaler, pscaler_bypass_line[src], 0);
		break;
	}

	if(crop_flag == 1)  //crop
	{
		drv_l1_pscaler_pixels_set(scaler, inW, inH, zoomW, zoomH, wFactor, hFactor);

		if(mode == PREVIEW_MODE_PBP)
		{
			drv_l1_pscaler_outcrop_set(scaler, 1, (zoomW-outW)>>1, (zoomH-outH)>>1, outW, outH);
		}
#if C_REV_BACK_SENSOR_FULL_SCREEN == CUSTOM_ON
		else if(((mode == PREVIEW_MODE_BACK)
                #if !C_DISP_SLIDE_SAVE
				|| (mode == PREVIEW_MODE_BACK_KEEP_RATIO)
                #endif
				)&&(src == PREVIEW_SRC_REAR))//add for hxwl reversing mode back sensor full screen display by antion@20230329
		{
			drv_l1_pscaler_outcrop_set(scaler, 1, preview_out_w_start[src], 0, outW, outH);
		}

        #if C_DISP_SLIDE_SAVE
		else if((mode == PREVIEW_MODE_BACK_KEEP_RATIO)&&(src == PREVIEW_SRC_REAR))//add for hxwl reversing mode back sensor full screen display by antion@20230329
		{
			drv_l1_pscaler_outcrop_set(scaler, 1, preview_out_w_start_reversing, preview_out_h_start_reversing, outW, outH);
		}
        #endif
#endif//C_REV_BACK_SENSOR_FULL_SCREEN
		else
		{
			drv_l1_pscaler_outcrop_set(scaler, 1, preview_out_w_start[src], preview_out_h_start[src], outW, outH);
		}
	}
	else if(crop_flag == 2) //add blackedge
	{
		drv_l1_pscaler_pixels_set(scaler, inW, inH, zoomW, zoomH, wFactor, hFactor);
		drv_l1_pscaler_outcrop_set(scaler, 0, 0, 0, 0, 0);
	}
	else
	{
		drv_l1_pscaler_outcrop_set(scaler, 0, 0, 0, 0, 0);
		drv_l1_pscaler_pixels_set(scaler, inW, inH, outW, outH, wFactor, hFactor);
	}

	drv_l1_pscaler_input_X_start_set(scaler, w_start);
	drv_l1_pscaler_input_Y_start_set(scaler, h_start);

	drv_l1_pscaler_output_fifo_line_set(scaler, outH, 0);

	drv_l1_pscaler_output_pixels_X_offset_set(scaler, wOffset);
	drv_l1_pscaler_PIP_window_set(scaler, cutWin, &pip_parm);

	drv_l1_pscaler_integration_mode_set(scaler, hIntegration, wIntegration);

	drv_l1_pscaler_H_first_enable(scaler, h_first);

	if(preview_src_drop_half[src])
	{
		drv_l1_pscaler_drop_frame_set(scaler, 1, 2);
	}
	else
	{
		drv_l1_pscaler_drop_frame_set(scaler, 0, 0);
	}

	if(!runtime)
	{
		switch(pscaler_mode[scaler])
		{
			case PSCALER_MODE_PIPE_BYPASS:
				drv_l1_pscaler_bypass_mode_set(scaler, 1);
				pscaler_busy[scaler] = 1;
				drv_l1_pscaler_interrupt_enable(scaler, 1);
				drv_l1_pscaler_start(scaler);
			break;
			case PSCALER_MODE_PIPE:
				drv_l1_pscaler_bypass_mode_set(scaler, 0);
				pscaler_busy[scaler] = 1;
				drv_l1_pscaler_interrupt_enable(scaler, 1);
				drv_l1_pscaler_start(scaler);
			break;
			case PSCALER_MODE_DRAM:
				drv_l1_pscaler_bypass_mode_set(scaler, 0);
				drv_l1_pscaler_register_refresh_last_eof_config(scaler, REG_REFRESH_LAST_EOF_SCALE | REG_REFRESH_LAST_EOF_READ);
				drv_l1_pscaler_interrupt_enable(scaler, 1);
			break;
			default:
			break;
		}

	}
	else
	{
		switch(pscaler_mode[scaler])
		{
			case PSCALER_MODE_PIPE_BYPASS:
			case PSCALER_MODE_PIPE:
				preview_switch_init[scaler] = 1;
				preview_switch_done[scaler] = 0;
				drv_l1_pscaler_register_refresh(scaler, REG_REFRESH_SOF);

				#if 0
				DBG_PRINT("[TRACE] drv_l1_pscaler_sw_reset(%d)\r\n", scaler);
				#endif

			break;
			case PSCALER_MODE_DRAM:
				preview_switch_init[scaler] = 1;
				preview_switch_done[scaler] = 1;
			break;
			default:
			break;
		}
	}
}

static INT32U
preview_single_pre(
	INT32U scaler
)
{
	INT32U buf = 0;

	if(preview_buf_fun_malloc && preview_q_size && preview_q)
	{
		buf = preview_buf_fun_malloc();
	}

	if(!buf)
	{
		buf = DUMMY_BUFFER_ADDRS;
	}
	else
	{
		// debug
		if(((preview_q_head + 1) == preview_q_tail)
			|| ((preview_q_head + 1) == (preview_q_tail + preview_q_size))
		)
		{
			#if 0
			DBG_PRINT("[ERROR] preview_q full\r\n");
			#else
			return DUMMY_BUFFER_ADDRS;
			#endif
		}

		preview_q[preview_q_head].buf_preview = buf;
		preview_q[preview_q_head].st_main = ACCESS_HW;
		preview_q[preview_q_head].st_ext = ACCESS_HW;
		if(++preview_q_head >= preview_q_size)
		{
			preview_q_head = 0;
		}
		buf += preview_buf_offset_get(scaler, _preview_mode);
	}

	return buf;
}

static INT32U
preview_single_post(
	INT32U scaler,
	INT32U buf
)
{
	if(buf == DUMMY_BUFFER_ADDRS)
	{
		return 0;
	}

	buf -= preview_buf_offset_get(scaler, _preview_mode);

	if(!preview_q_size || !preview_q)
	{
		return 0;
	}

	// debug
	if(preview_q_head == preview_q_tail)
	{
		#if 0
		DBG_PRINT("[ERROR] preview_q empry\r\n");
		#else
		return 0;
		#endif
	}

	// debug
	if(buf != preview_q[preview_q_tail].buf_preview)
	{
		#if 0
		DBG_PRINT("[WARN] preview got a unknown buffer %x\r\n", buf);
		#endif
		buf = preview_q[preview_q_tail].buf_preview;
	}

	preview_q[preview_q_tail].buf_preview = (INT32U)NULL;
	preview_q[preview_q_tail].st_main = ACCESS_NONE;
	preview_q[preview_q_tail].st_ext = ACCESS_NONE;
	if(++preview_q_tail >= preview_q_size)
	{
		preview_q_tail = 0;
	}

	return buf;
}

static INT32U
preview_dual_main_pre(
	INT32U scaler
)
{
	INT32U buf = 0;
	INT32U i;

	if(!preview_q_size || !preview_q)
	{
		return DUMMY_BUFFER_ADDRS;
	}

	//find a buf who has no background
	for(i = preview_q_tail; i != preview_q_head; ++i)
	{
		if(i >= preview_q_size)
		{
			i = 0;
			if(i == preview_q_head)
			{
				break;
			}
		}

		if(preview_q[i].st_main == ACCESS_NONE)
		{
			buf = preview_q[i].buf_preview;
			preview_q[i].st_main = ACCESS_HW;
			break;
		}
	}

	//nothing got from preview_q
	if(!buf)
	{
		//request a new buf
		if(preview_buf_fun_malloc)
		{
			buf = preview_buf_fun_malloc();
		}

		//inset into preview_q
		if(buf)
		{
			//debug
			if(((preview_q_head + 1) == preview_q_tail)
				|| ((preview_q_head + 1) == (preview_q_tail + preview_q_size))
			)
			{
				#if 0
				DBG_PRINT("[ERROR] preview_q full @%s:%d\r\n", __func__, __LINE__);
				#else
				return DUMMY_BUFFER_ADDRS;
				#endif
			}

			preview_q[preview_q_head].buf_preview = buf;
			preview_q[preview_q_head].st_main = ACCESS_HW;
			preview_q[preview_q_head].st_ext = ACCESS_NONE;
			if(++preview_q_head >= preview_q_size)
			{
				preview_q_head = 0;
			}
		}
	}

	//no buf avaliable now
	if(!buf)
	{
		buf = DUMMY_BUFFER_ADDRS;
	}
	else
	{
		buf += preview_buf_offset_get(scaler, _preview_mode);
	}

	return buf;
}

static INT32U
preview_dual_main_post(
	INT32U scaler,
	INT32U buf
)
{
	INT32U	ret = 0;
	INT32U	i;

	if(buf == DUMMY_BUFFER_ADDRS)
	{
		return 0;
	}

	if(!preview_q_size || !preview_q)
	{
		return 0;
	}

	buf -= preview_buf_offset_get(scaler, _preview_mode);

	//find the buf's index in preview_q
	for(i = preview_q_tail; i != preview_q_head; ++i)
	{
		if(i >= preview_q_size)
		{
			i = 0;
			if(i == preview_q_head)
			{
				break;
			}
		}

		if(buf == preview_q[i].buf_preview)
		{
			break;
		}
	}

	//debug
	if(i == preview_q_head)
	{
		#if 0
		DBG_PRINT("[WARN] preview got a unknown buffer %x\r\n", buf);
		#endif

		i = preview_q_tail;
		buf = preview_q[i].buf_preview;
	}

	//mark as main done
	preview_q[i].st_main = ACCESS_DONE;

	//both background & foreground done, send to display & de-queue
	if(preview_q[preview_q_tail].st_main == ACCESS_DONE && preview_q[preview_q_tail].st_ext == ACCESS_DONE)
	{
		//debug
		if(preview_q_head == preview_q_tail)
		{
			#if 0
			DBG_PRINT("[ERROR] preview_q empry\r\n");
			#else
			return 0;
			#endif
		}

		ret = preview_q[preview_q_tail].buf_preview;

		preview_q[preview_q_tail].buf_preview = (INT32U)NULL;
		preview_q[preview_q_tail].st_main = ACCESS_NONE;
		preview_q[preview_q_tail].st_ext = ACCESS_NONE;
		if(++preview_q_tail >= preview_q_size)
		{
			preview_q_tail = 0;
		}
	}

	return ret;
}

static INT32U
preview_dual_ext_pre(
	INT32U scaler
)
{
	#if defined(C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
	VID_INFO*  pVid_info_front = &pStreamArgs->vid_info_front;
	#endif
	INT32U buf = 0;
	INT32U i;

	if(!preview_q_size || !preview_q)
	{
		return DUMMY_BUFFER_ADDRS;
	}

	//find a buf who has no foreground
	for(i = preview_q_tail; i != preview_q_head; ++i)
	{
		if(i >= preview_q_size)
		{
			i = 0;
			if(i == preview_q_head)
				break;
		}

		if(preview_q[i].st_ext == ACCESS_NONE)
		{
			buf = preview_q[i].buf_preview;
			preview_q[i].st_ext = ACCESS_HW;
			break;
		}
	}

	#if defined(C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
	if(pVid_info_front->run_2K_flow_src) // This value is Greater than 0, that means front do scale-up flow
	{
		if(_preview_mode == PREVIEW_MODE_PIP_FRONT)
		{// Scaler don't have cut-window function, pscaler has to wait until scaler finish work
			if(!buf)
			{
				return DUMMY_BUFFER_ADDRS;
			}
		}
	}
	#endif

	//nothing got from preview_q
	if(!buf)
	{
		//request a new buf
		buf = preview_buf_fun_malloc();

		//inset into preview_q
		if(buf)
		{
			//debug
			if(((preview_q_head + 1) == preview_q_tail)
				|| ((preview_q_head + 1) == (preview_q_tail + preview_q_size))
			)
			{
				#if 0
				DBG_PRINT("[ERROR] preview_q full\r\n");
				#else
				return DUMMY_BUFFER_ADDRS;
				#endif
			}

			preview_q[preview_q_head].buf_preview = buf;
			preview_q[preview_q_head].st_main = ACCESS_NONE;
			preview_q[preview_q_head].st_ext = ACCESS_HW;
			if(++preview_q_head >= preview_q_size)
			{
				preview_q_head = 0;
			}
		}
	}

	//no buf available now
	if(!buf)
	{
		buf = DUMMY_BUFFER_ADDRS;
	}
	else
	{
		buf += preview_buf_offset_get(scaler, _preview_mode);
	}

	return buf;
}

static INT32U
preview_dual_ext_post(
	INT32U scaler,
	INT32U buf
)
{
	INT32U ret = 0;
	INT32U i;

	if(buf == DUMMY_BUFFER_ADDRS)
	{
		return 0;
	}

	if(!preview_q_size || !preview_q)
	{
		return 0;
	}

	buf -= preview_buf_offset_get(scaler, _preview_mode);

	//find the buf's index in preview_q
	for(i = preview_q_tail; i != preview_q_head; ++i)
	{
		if(i >= preview_q_size)
		{
			i = 0;
			if(i == preview_q_head)
			{
				break;
			}
		}

		if(buf == preview_q[i].buf_preview)
		{
			break;
		}
	}

	//debug
	if(i == preview_q_head)
	{
		#if 0
		DBG_PRINT("[WARN] preview got a unknown buffer %x\r\n", buf);
		#endif

		i = preview_q_tail;
	}

	//mark as extra done
	preview_q[i].st_ext = ACCESS_DONE;

	//both background & foreground done, send to display & de-queue
	if(preview_q[preview_q_tail].st_main == ACCESS_DONE && preview_q[preview_q_tail].st_ext == ACCESS_DONE)
	{
		//debug
		if(preview_q_head == preview_q_tail)
		{
			#if 0
			DBG_PRINT("[ERROR] preview_q empry @%s:%d\r\n", __func__, __LINE__);
			#endif

			return 0;
		}

		ret = preview_q[preview_q_tail].buf_preview;

		preview_q[preview_q_tail].buf_preview = 0;
		preview_q[preview_q_tail].st_main = ACCESS_NONE;
		preview_q[preview_q_tail].st_ext = ACCESS_NONE;
		++preview_q_tail;
		if(preview_q_tail >= preview_q_size)
		{
			preview_q_tail = 0;
		}
	}

	return ret;
}

static INT32U
preview_none_pre(
	INT32U scaler
)
{
	return DUMMY_BUFFER_ADDRS;
}

static INT32U
preview_none_post(
	INT32U scaler,
	INT32U buf
)
{
	if(buf == DUMMY_BUFFER_ADDRS)
	{
		return 0;
	}

	if(!preview_q_size || !preview_q)
	{
		return 0;
	}

	if(preview_q_head == preview_q_tail)
	{
		return 0;
	}

	// debug
	if(buf != preview_q[preview_q_tail].buf_preview)
	{
		#if 0
		DBG_PRINT("[WARN] preview got a unknown buffer %x\r\n", buf);
		#endif
		buf = preview_q[preview_q_tail].buf_preview;
	}


	preview_q[preview_q_tail].buf_preview = 0;
	preview_q[preview_q_tail].st_main = ACCESS_NONE;
	preview_q[preview_q_tail].st_ext = ACCESS_NONE;

	if(++preview_q_tail >= preview_q_size)
	{
		preview_q_tail = 0;
	}

	if(preview_buf_fun_free)
	{
		preview_buf_fun_free(buf);
	}

	return 0;
}

static INT32U
preview_switch_post(
	INT32U scaler,
	INT32U buf
)
{
	#if defined(C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
	VID_INFO*  pVid_info_front = &pStreamArgs->vid_info_front;
	VID_INFO*  pVid_info_rear = &pStreamArgs->vid_info_rear;

	if(pVid_info_front->run_2K_flow_src) // This value is Greater than 0, that means front do scale-up flow
	{
		INT32U out_buf = DUMMY_BUFFER_ADDRS;

		if(pVid_info_rear->run_2K_flow_src)
		{
			drv_l1_scaler_output_addr_get(SCALER_0,&out_buf,NULL,NULL);
		}

		//no buffer locked, change mode now
		if(((pscaler_mode[PSCALER_A] == PSCALER_MODE_DRAM) || ((drv_l1_pscaler_output_A_buffer_get(PSCALER_A) == DUMMY_BUFFER_ADDRS) && (drv_l1_pscaler_output_B_buffer_get(PSCALER_A) == DUMMY_BUFFER_ADDRS)))
			&& ((pscaler_mode[PSCALER_B] == PSCALER_MODE_DRAM) || (out_buf == DUMMY_BUFFER_ADDRS))
		)
		{
			if(!preview_switch_init[scaler])
			{
				if(preview_q_size && preview_q)
				{
					INT32U	i;
					for(i = preview_q_tail; i != preview_q_head; ++i)
					{
						if(i >= preview_q_size)
						{
							i = 0;
							if(i == preview_q_head)
							{
								break;
							}
						}

						if(preview_q[i].buf_preview)
						{
							if(preview_buf_fun_free)
							{
								preview_buf_fun_free(preview_q[i].buf_preview);
							}
						}

						preview_q[i].buf_preview = 0;
						preview_q[i].st_main = ACCESS_NONE;
						preview_q[i].st_ext = ACCESS_NONE;
					}

					preview_q_tail = 0;
					preview_q_head = 0;

				}

				switch(_preview_mode_to)
				{
					case PREVIEW_MODE_BACK_KEEP_RATIO:
					case PREVIEW_MODE_BACK:
						preview_car_baking = 1;
						break;

					default:
						preview_car_baking = 0;
						break;
				}

				if(scaler == PSCALER_B) // fake parameter
				{
					preview_scaler_parm_set(SCALER_0, _preview_mode_to, 1, NULL);
				}
				else
				{
					preview_pscaler_parm_set(scaler, _preview_mode_to, 1, NULL);
				}
				//DBG_PRINT("[TRACE] preview_pscaler_parm_set(%d):%d\r\n", scaler,_preview_mode_to);
			}
		}

		if(preview_src_path[PREVIEW_SRC_REAR] == PREVIEW_IN_PATH_NONE)
		{
			if((pscaler_mode[PSCALER_A] == PSCALER_MODE_DRAM) || (preview_switch_init[PSCALER_A] && preview_switch_done[PSCALER_A]))
			{
				_preview_mode = _preview_mode_to;
			}
		}
		else
		{
			if( ((pscaler_mode[PSCALER_A] == PSCALER_MODE_DRAM) || (preview_switch_init[PSCALER_A] && preview_switch_done[PSCALER_A]))
				&& ((pscaler_mode[PSCALER_B] == PSCALER_MODE_DRAM) || (preview_switch_init[PSCALER_B] && preview_switch_done[PSCALER_B]))
			)
			{
				_preview_mode = _preview_mode_to;
			}
		}

		return 0;
	}
	else
	#endif
	{
		//no buffer locked, change mode now
		if(	((pscaler_mode[PSCALER_A] == PSCALER_MODE_DRAM) || ((drv_l1_pscaler_output_A_buffer_get(PSCALER_A) == DUMMY_BUFFER_ADDRS) && (drv_l1_pscaler_output_B_buffer_get(PSCALER_A) == DUMMY_BUFFER_ADDRS)))
			&& ((pscaler_mode[PSCALER_B] == PSCALER_MODE_DRAM) || ((drv_l1_pscaler_output_A_buffer_get(PSCALER_B) == DUMMY_BUFFER_ADDRS) && (drv_l1_pscaler_output_B_buffer_get(PSCALER_B) == DUMMY_BUFFER_ADDRS)))
		)
		{
			if(!preview_switch_init[scaler])
			{
				if(preview_q_size && preview_q)
				{
					INT32U	i;
					for(i = preview_q_tail; i != preview_q_head; ++i)
					{
						if(i >= preview_q_size)
						{
							i = 0;
							if(i == preview_q_head)
							{
								break;
							}
						}

						if(preview_q[i].buf_preview)
						{
							if(preview_buf_fun_free)
							{
								preview_buf_fun_free(preview_q[i].buf_preview);
							}
						}

						preview_q[i].buf_preview = 0;
						preview_q[i].st_main = ACCESS_NONE;
						preview_q[i].st_ext = ACCESS_NONE;
					}

					preview_q_tail = 0;
					preview_q_head = 0;

				}

				switch(_preview_mode_to)
				{
					case PREVIEW_MODE_BACK_KEEP_RATIO:
					case PREVIEW_MODE_BACK:
						preview_car_baking = 1;
						break;

					default:
						preview_car_baking = 0;
						break;
				}

				preview_pscaler_parm_set(scaler, _preview_mode_to, 1, NULL);
				//DBG_PRINT("[TRACE] preview_pscaler_parm_set(%d):%d\r\n", scaler,_preview_mode_to);
			}
		}

		if(preview_src_path[PREVIEW_SRC_REAR] == PREVIEW_IN_PATH_NONE)
		{
			if((pscaler_mode[PSCALER_A] == PSCALER_MODE_DRAM) || (preview_switch_init[PSCALER_A] && preview_switch_done[PSCALER_A]))
			{
				_preview_mode = _preview_mode_to;
			}
		}
		else
		{
			if( ((pscaler_mode[PSCALER_A] == PSCALER_MODE_DRAM) || (preview_switch_init[PSCALER_A] && preview_switch_done[PSCALER_A]))
				&& ((pscaler_mode[PSCALER_B] == PSCALER_MODE_DRAM) || (preview_switch_init[PSCALER_B] && preview_switch_done[PSCALER_B]))
			)
			{
				_preview_mode = _preview_mode_to;
			}
		}
	}

	return 0;
}

static struct
{
	// pre-process before pscaler access
	INT32U (*pre_action) (INT32U);

	// post-process after pscaler access
	INT32U (*post_action) (INT32U, INT32U);
}

preview_process[PREVIEW_ACT_TOTAL][PREVIEW_SRC_TOTAL] =
{
	// PREVIEW_MODE_FRONT
	{	//PREVIEW_SRC_FRONT
		{
			preview_single_pre, preview_single_post
		},

		//PREVIEW_SRC_REAR
		{
			preview_none_pre, preview_none_post
		}
	},
	// PREVIEW_MODE_REAR
	{	//PREVIEW_SRC_FRONT
		{
			preview_none_pre, preview_none_post
		},

		//PREVIEW_SRC_REAR
		{
			preview_single_pre, preview_single_post
		}
	},
	// PREVIEW_MODE_PIP_REAR
	{	//PREVIEW_SRC_FRONT
		{
			preview_dual_main_pre, preview_dual_main_post
		},

		//PREVIEW_SRC_REAR
		{
			preview_dual_ext_pre, preview_dual_ext_post
		}
	},
	// PREVIEW_MODE_PIP_FRONT
	{	//PREVIEW_SRC_FRONT
		{
			preview_dual_ext_pre, preview_dual_ext_post
		},

		//PREVIEW_SRC_REAR
		{
			preview_dual_main_pre, preview_dual_main_post
		}
	},
	// PREVIEW_MODE_PBP
	{	//PREVIEW_SRC_FRONT
		{
			preview_dual_main_pre, preview_dual_main_post
		},

		//PREVIEW_SRC_REAR
		{
			preview_dual_ext_pre, preview_dual_ext_post
		}
	},
	// PREVIEW_MODE_NONE
	{	//PREVIEW_SRC_FRONT
		{
			preview_none_pre, preview_none_post
		},

		//PREVIEW_SRC_REAR
		{
			preview_none_pre, preview_none_post
		}
	},
	// PREVIEW_MODE_BACK_KEEP_RATIO
	{	//PREVIEW_SRC_FRONT
		{
			preview_none_pre, preview_none_post
		},

		//PREVIEW_SRC_REAR
		{
			preview_single_pre, preview_single_post
		}
	},
	// PREVIEW_MODE_BACK
	{	//PREVIEW_SRC_FRONT
		{
			preview_none_pre, preview_none_post
		},

		//PREVIEW_SRC_REAR
		{
			preview_single_pre, preview_single_post
		}
	},
	// PREVIEW_MODE_STOP_WAIT
	{	//PREVIEW_SRC_FRONT
		{
			preview_none_pre, preview_none_post
		},

		//PREVIEW_SRC_REAR
		{
			preview_none_pre, preview_none_post
		}
	},
	// PREVIEW_MODE_STOP
	{	//PREVIEW_SRC_FRONT
		{
			preview_none_pre, preview_none_post
		},

		//PREVIEW_SRC_REAR
		{
			preview_none_pre, preview_none_post
		}
	},
	// PREVIEW_ACT_SWITCH
	{	//PREVIEW_SRC_FRONT
		{
			preview_none_pre, preview_switch_post
		},

		//PREVIEW_SRC_REAR
		{
			preview_none_pre, preview_switch_post
		}
	}
};

/*== HOST USB MJPEG CAM / INTERP2K ==*/
#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
void
preview_interp2K_isr(
	PS_ID scaler,
	PS_ST PScaler_Event,
	INT8U from_scaler
)
{
	INT8U src = preview_ps_to_src(scaler);
	INT32U pend_msg;
	INT32U disp_buf_done = DUMMY_BUFFER_ADDRS;
	INT32U disp_buf_next;
	INT32U scaler_num = scaler;

	if(preview_need_display)
	{
		if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE || PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE)
		{
			//get draw done buffer
			if(from_scaler)
			{
				scaler_num = SCALER_0;
				if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE)
				{
					drv_l1_scaler_output_addr_get(scaler_num,&disp_buf_done,NULL,NULL);
				}
			}
			else
			{
				if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE)
				{
					disp_buf_done = drv_l1_pscaler_output_A_buffer_get(scaler);
				}
				else
				if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE)
				{
					disp_buf_done = drv_l1_pscaler_output_B_buffer_get(scaler);
				}
			}

			if(disp_buf_done < DUMMY_BUFFER_ADDRS)
			{
				disp_buf_done -= display_offset_byte_len;
			}

			//DBG_PRINT("_preview_mode:%x\r\n",_preview_mode);

			pend_msg = preview_process[_preview_mode][src].post_action(scaler, disp_buf_done);

			disp_buf_next = preview_process[_preview_mode][src].pre_action(scaler);

			if(disp_buf_next < DUMMY_BUFFER_ADDRS)
			{
				disp_buf_next += display_offset_byte_len;
			}

			if(from_scaler)
			{
				scaler_num = SCALER_0;
				drv_l1_scaler_output_addr_set(scaler_num,disp_buf_next,DUMMY_BUFFER_ADDRS,DUMMY_BUFFER_ADDRS);
			}
			else
			{
				//set buffer for next frame
				if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE)
				{
					drv_l1_pscaler_output_A_buffer_set(scaler, disp_buf_next);
				}
				else
				if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE)
				{
					drv_l1_pscaler_output_B_buffer_set(scaler, disp_buf_next);
				}
			}

			if(pend_msg)
			{
				#if DBG_FPS_ENABLE
				++fps_ps_update[scaler];
				#endif

				if(preview_buf_fun_show)
				{
					INT16U x_offset = 0, y_offset = 0;

					if((_preview_mode == PREVIEW_MODE_FRONT)||
  					   (_preview_mode == PREVIEW_MODE_PIP_REAR)
					)
					{
						x_offset = preview_out_off_x[PREVIEW_SRC_FRONT];
						y_offset = preview_out_off_y[PREVIEW_SRC_FRONT];
					}
					else if((_preview_mode == PREVIEW_MODE_REAR)||
  					        (_preview_mode == PREVIEW_MODE_PIP_FRONT)||
							(_preview_mode == PREVIEW_MODE_BACK_KEEP_RATIO)
					)
					{
						x_offset = preview_out_off_x[PREVIEW_SRC_REAR];
						y_offset = preview_out_off_y[PREVIEW_SRC_REAR];
					}
					else if(_preview_mode == PREVIEW_MODE_BACK)
					{
						x_offset = preview_out_back_off_x;
						y_offset = preview_out_back_off_y;
					}

                     preview_buf_fun_show(pend_msg, display_offset_byte_len, preview_car_baking,
                     y_offset, y_offset, x_offset, x_offset);
				}
			}
		}
	}
}

void
preview_pscaler_isr(
	PS_ID scaler,
	PS_ST PScaler_Event
)
{// It's for PREVIEW_SRC_FRONT (PSCALER_A), bypass output frame buffer
	INT8U src = PREVIEW_SRC_FRONT;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl;
	ENC_STREAM_INFO_T* pEncStramInfo;
	INT32U fifo_buf_done;
	INT32U fifo_buf_next;
	static INT32U flag_overflow = 0;

	//DBG_PRINT("pscaler:%x\r\n",PScaler_Event);

	if(PScaler_Event & PIPELINE_SCALER_STATUS_OVERFLOW_OCCUR)
	{
        DBG_PRINT("pscaler overflow\r\n");
		flag_overflow = 1;
	}

	if(PScaler_Event & PIPELINE_SCALER_STATUS_REG_REFRESH)
	{
		preview_switch_done[src] = 1;
	}

  	/*=== Display ===*/
	preview_interp2K_isr(scaler,PScaler_Event,0);

   	/*=== Record ===*/
	pPscalerStreamCtrl = &pscaler_stream[src];
	if(!pPscalerStreamCtrl)
	{
		return;
	}

	pEncStramInfo = &pPscalerStreamCtrl->enc_stream_info;
	if(pEncStramInfo->stream_enable == VIDEO_RECORD_NOTHING)
	{
		return;
	}

	/*== Video Record ==*/
	if(pEncStramInfo->stream_enable & (VIDEO_RECORD_START | VIDEO_RECORD_RECING | VIDEO_RECORD_STOP))
	{
		if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_BUF_A_DONE)
		{
			fifo_buf_done = drv_l1_pscaler_bypass_output_A_buffer_get(scaler);
			if(pEncStramInfo->stream_enable & VIDEO_RECORD_STOP)
			{
				fifo_buf_next = DUMMY_BUFFER_ADDRS;
			}
			else
			{
				fifo_buf_next = pPscalerStreamCtrl->interp2K_get_func();
			}

			++pPscalerStreamCtrl->interp2K_ps_fb_cfg->fb_cfg_count_num;

			if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_FRAME_DONE)
			{
				if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_A_buffer_set(scaler, fifo_buf_next))
				{
					drv_l1_pscaler_bypass_output_A_buffer_set(scaler, fifo_buf_done);
					fifo_buf_done = DUMMY_BUFFER_ADDRS;
					if(fifo_buf_next != DUMMY_BUFFER_ADDRS)
					{
						pPscalerStreamCtrl->interp2K_free_func(fifo_buf_next);
					}
				}
			}
			else
			{
				if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_A_buffer_set(scaler, fifo_buf_next))
				{
					drv_l1_pscaler_bypass_output_A_buffer_set(scaler, fifo_buf_done);
					fifo_buf_done = DUMMY_BUFFER_ADDRS;
					if(fifo_buf_next != DUMMY_BUFFER_ADDRS)
					{
						pPscalerStreamCtrl->interp2K_free_func(fifo_buf_next);
					}
				}

				pPscalerStreamCtrl->interp2K_ps_fb_cfg->fb_cfg_count_num = 0;
			}

			if(pEncStramInfo->stream_enable & (VIDEO_RECORD_RECING | VIDEO_RECORD_STOP))
			{
				if(!flag_overflow && pPscalerStreamCtrl->interp2K_notify_func)
				{
					pPscalerStreamCtrl->interp2K_notify_func(src,pPscalerStreamCtrl,fifo_buf_done);
				}
				else
				{
					if(fifo_buf_done != DUMMY_BUFFER_ADDRS)
					{
						pPscalerStreamCtrl->interp2K_free_func(fifo_buf_done);
					}
				}
			}
			else
			{
				if(fifo_buf_done != DUMMY_BUFFER_ADDRS)
				{
					pPscalerStreamCtrl->interp2K_free_func(fifo_buf_done);
				}
			}
		}

		if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_BUF_B_DONE)
		{
			fifo_buf_done = drv_l1_pscaler_bypass_output_B_buffer_get(scaler);
			if(pEncStramInfo->stream_enable & VIDEO_RECORD_STOP)
			{
				fifo_buf_next = DUMMY_BUFFER_ADDRS;
			}
			else
			{
				fifo_buf_next = pPscalerStreamCtrl->interp2K_get_func();
			}

			++pPscalerStreamCtrl->interp2K_ps_fb_cfg->fb_cfg_count_num;

			if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_FRAME_DONE)
			{
				if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_B_buffer_set(scaler, fifo_buf_next))
				{
					drv_l1_pscaler_bypass_output_B_buffer_set(scaler, fifo_buf_done);
					fifo_buf_done = DUMMY_BUFFER_ADDRS;
					if(fifo_buf_next != DUMMY_BUFFER_ADDRS)
					{
						pPscalerStreamCtrl->interp2K_free_func(fifo_buf_next);
					}
				}
			}
			else
			{
				if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_B_buffer_set(scaler, fifo_buf_next))
				{
					drv_l1_pscaler_bypass_output_B_buffer_set(scaler, fifo_buf_done);
					fifo_buf_done = DUMMY_BUFFER_ADDRS;
					if(fifo_buf_next != DUMMY_BUFFER_ADDRS)
					{
						pPscalerStreamCtrl->interp2K_free_func(fifo_buf_next);
					}
				}

				pPscalerStreamCtrl->interp2K_ps_fb_cfg->fb_cfg_count_num = 0;
			}

			if(pEncStramInfo->stream_enable & (VIDEO_RECORD_RECING | VIDEO_RECORD_STOP))
			{
				if(!flag_overflow && pPscalerStreamCtrl->interp2K_notify_func)
				{
					pPscalerStreamCtrl->interp2K_notify_func(src,pPscalerStreamCtrl,fifo_buf_done);
				}
				else
				{
					if(fifo_buf_done != DUMMY_BUFFER_ADDRS)
					{
						pPscalerStreamCtrl->interp2K_free_func(fifo_buf_done);
					}
				}
			}
			else
			{
				if(fifo_buf_done != DUMMY_BUFFER_ADDRS)
				{
					pPscalerStreamCtrl->interp2K_free_func(fifo_buf_done);
				}
			}
		}

		if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_FRAME_DONE)
		{
			// Record
			if(pEncStramInfo->stream_enable == VIDEO_RECORD_START)
			{
				pEncStramInfo->stream_enable = VIDEO_RECORD_RECING;
			}

			pPscalerStreamCtrl->interp2K_ps_fb_cfg->fb_cfg_count_num = 0;
			flag_overflow = 0;
		}
	}// End of video record

	/*== Photo Capture ==*/
	if(pEncStramInfo->stream_enable & (VIDEO_RECORD_PHOTO_START | VIDEO_RECORD_PHOTO))
	{
		if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_BUF_A_DONE)
		{
			fifo_buf_done = drv_l1_pscaler_bypass_output_A_buffer_get(scaler);

			if(fifo_buf_done != DUMMY_BUFFER_ADDRS)
			{
				if(pPscalerStreamCtrl->ps_notify_func)
				{
					pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,fifo_buf_done);
				}
			}

			fifo_buf_next = pPscalerStreamCtrl->ps_get_func();

			if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_A_buffer_set(scaler, fifo_buf_next))
			{
				drv_l1_pscaler_bypass_output_A_buffer_set(scaler, DUMMY_BUFFER_ADDRS);
			}
		}

		if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_BUF_B_DONE)
		{
			fifo_buf_done = drv_l1_pscaler_bypass_output_B_buffer_get(scaler);

			if(fifo_buf_done != DUMMY_BUFFER_ADDRS)
			{
				if(pPscalerStreamCtrl->ps_notify_func)
				{
					pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,fifo_buf_done);
				}
			}

			fifo_buf_next = pPscalerStreamCtrl->ps_get_func();

			if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_B_buffer_set(scaler, fifo_buf_next))
			{
				drv_l1_pscaler_bypass_output_B_buffer_set(scaler, DUMMY_BUFFER_ADDRS);
			}
		}

		if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_FRAME_DONE)
		{
			if(pEncStramInfo->stream_enable == VIDEO_RECORD_PHOTO_START)
			{
				pEncStramInfo->stream_enable = VIDEO_RECORD_PHOTO;
			}
			else
			if((pEncStramInfo->stream_enable == VIDEO_RECORD_PHOTO)
				&& (drv_l1_pscaler_bypass_output_A_buffer_get(scaler) == DUMMY_BUFFER_ADDRS)
				&& (drv_l1_pscaler_bypass_output_B_buffer_get(scaler) == DUMMY_BUFFER_ADDRS)
			)
			{
				pEncStramInfo->stream_enable = VIDEO_RECORD_NOTHING;
				if(msgQSend(pPscalerStreamCtrl->ps_notify_q, (MSG_VID_PSCALER_NOTIFY|src), NULL, 0, MSG_PRI_NORMAL) < 0)
				{
					ERR_PRINT("fail to msgQSend [%d]\r\n",src);
				}
				//DBG_PRINT("PHO[%d]Pscaler Stop\r\n",scaler);
			}
		}
	}// End of photo capture
}

void
preview_scaler_isr(
	INT32U Scaler0_Event,
	INT32U Scaler1_Event
)
{
	INT8U src = PREVIEW_SRC_REAR; 	// It's for PREVIEW_SRC_REAR (SCALER_0)
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];

    INT32U trans_event = 0;

	//DBG_PRINT("scaler:%x\r\n",Scaler0_Event);

	if(Scaler0_Event & C_SCALER_STATUS_DONE)
	{
        INT32U in_buf;

		trans_event |= PIPELINE_SCALER_STATUS_BUF_A_DONE;

		pscaler_busy[src] = 0;

		drv_l1_scaler_input_A_addr_get(SCALER_0,&in_buf,NULL,NULL);
		if(in_buf != DUMMY_BUFFER_ADDRS)
		{
			if(lib_interp2K_scaler_preview_done_notify(src,in_buf) == STATUS_FAIL)
			{
				// Release pscaler bypass output buffer
				pPscalerStreamCtrl->ps_free_func(in_buf);
			}
		}
	}

  	/*=== Display ===*/
	preview_interp2K_isr(PSCALER_B,trans_event,1); // fake parameter (PSCALER_B)
}

void
preview_scaler_parm_set(
	INT32U scaler,// it's scaler_0
	INT32U mode,
	INT32U runtime,
	void (*ps_isr) (INT32U, INT32U)
)
{
	INT8U src = PREVIEW_SRC_REAR; // For getting REAR's paramter
	PIP_WINDOW_PARAM	pip_parm;

	INT32U	inW, inH;
	INT32U	outW, outH;
	INT32U	wOffset;
	INT32U	w_start, h_start;
	INT16U  pip_x, pip_y, pip_width, pip_height;

	pip_x = preview_pip_x;
	pip_y = preview_pip_y;
	pip_width = preview_pip_width;
	pip_height = preview_pip_height;
	if(mode == PREVIEW_MODE_PIP_REAR)
	{
		if(pip_x >= preview_out_off_x[PREVIEW_SRC_FRONT])
		{
			pip_x -= preview_out_off_x[PREVIEW_SRC_FRONT];
		}
		else
		{
			pip_width = pip_width + pip_x - preview_out_off_x[PREVIEW_SRC_FRONT];
			pip_x = 0;
		}

		if(pip_y >= preview_out_off_y[PREVIEW_SRC_FRONT])
		{
			pip_y -= preview_out_off_y[PREVIEW_SRC_FRONT];
		}
		else
		{
			pip_height = pip_height + pip_y - preview_out_off_y[PREVIEW_SRC_FRONT];
			pip_y = 0;
		}
	}
	else if(mode == PREVIEW_MODE_PIP_FRONT)
	{
		if(pip_x >= preview_out_off_x[PREVIEW_SRC_REAR])
		{
			pip_x -= preview_out_off_x[PREVIEW_SRC_REAR];
		}
		else
		{
			pip_width = pip_width + pip_x - preview_out_off_x[PREVIEW_SRC_REAR];
			pip_x = 0;
		}

		if(pip_y >= preview_out_off_y[PREVIEW_SRC_REAR])
		{
			pip_y -= preview_out_off_y[PREVIEW_SRC_REAR];
		}
		else
		{
			pip_height = pip_height + pip_y - preview_out_off_y[PREVIEW_SRC_REAR];
			pip_y = 0;
		}
	}

	pip_parm.PipStartX	= pip_x;
	pip_parm.PipStartY	= pip_y;
	pip_parm.PipEndX	= pip_parm.PipStartX + pip_width;
	pip_parm.PipEndY	= pip_parm.PipStartY + pip_height;

	// Set PSCALER_B to get REAR's paramter
	inW		= preview_src_width_get(PSCALER_B);
	inH		= preview_src_height_get(PSCALER_B);
	w_start	= preview_zoom_w_start_get(PSCALER_B, mode);
	h_start	= preview_zoom_h_start_get(PSCALER_B, mode);
	outW	= preview_output_width_get(PSCALER_B, mode);
	outH	= preview_output_height_get(PSCALER_B, mode);
	wOffset = preview_pixel_offset_get(PSCALER_B, mode);

	#if 0
	DBG_PRINT("(mode,inW,inH,zoomW,zoomH,outW,outH,w_start,h_start,wOffset)=(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\r\n"
		, mode, inW, inH, zoomW, zoomH, outW, outH, w_start, h_start,wOffset);
	#endif

	if(!runtime)
	{
		drv_l1_scaler_init(scaler);

		switch(preview_src_path[src])
		{
			case PREVIEW_IN_PATH_CSI0:
				drv_l1_register_csi_cbk(CSI_0,lib_interp2K_csi_callback_isr);
			break;
			case PREVIEW_IN_PATH_CSI1:
				drv_l1_register_csi_cbk(CSI_1,lib_interp2K_csi_callback_isr);
			break;
		}

		pscaler_mode[src] = PSCALER_MODE_DRAM;

		drv_l1_scaler_input_A_addr_set(scaler,DUMMY_BUFFER_ADDRS,DUMMY_BUFFER_ADDRS,DUMMY_BUFFER_ADDRS);
		drv_l1_scaler_input_B_addr_set(scaler,DUMMY_BUFFER_ADDRS,DUMMY_BUFFER_ADDRS,DUMMY_BUFFER_ADDRS);

		switch(preview_src_format[src])
		{
			case PIPELINE_SCALER_INPUT_FORMAT_YUYV:
				drv_l1_scaler_input_format_set(scaler, C_SCALER_CTRL_IN_YUYV);
			break;
			case PIPELINE_SCALER_INPUT_FORMAT_GP420:
				drv_l1_scaler_input_format_set(scaler, C_SCALER_CTRL_IN_GP420);
			break;
		}

		switch(preview_display_format)
		{
			case PIPELINE_SCALER_OUTPUT_FORMAT_RGB565:
				drv_l1_scaler_output_format_set(scaler, C_SCALER_CTRL_OUT_RGB565);
			break;
			case PIPELINE_SCALER_OUTPUT_FORMAT_GP420:
				drv_l1_scaler_output_format_set(scaler, C_SCALER_CTRL_OUT_GP420);
			break;
			case PIPELINE_SCALER_OUTPUT_FORMAT_YUYV:
				drv_l1_scaler_output_format_set(scaler, C_SCALER_CTRL_OUT_YUYV);
			break;
		}

		drv_l1_scaler_output_addr_set(scaler, DUMMY_BUFFER_ADDRS, DUMMY_BUFFER_ADDRS, DUMMY_BUFFER_ADDRS);
		drv_l1_scaler_isr_callback_set(ps_isr);
	}

	switch(pscaler_bypass_line[src])
	{
		case 0:
			drv_l1_scaler_output_fifo_line_set(scaler,C_SCALER_CTRL_OUT_FIFO_DISABLE);
		break;
		default:
			drv_l1_scaler_output_fifo_line_set(scaler, C_SCALER_CTRL_OUT_FIFO_64LINE);
		break;
	}

	drv_l1_scaler_image_pixels_set(scaler, inW, inH, outW, outH);
	drv_l1_scaler_input_offset_set(scaler, w_start, h_start);
	drv_l1_scaler_output_offset_set(scaler, wOffset);

	if(runtime)
	{
        preview_switch_init[src] = 1;
        preview_switch_done[src] = 1;
	}
}
#endif

static void
preview_ps_isr(
	PS_ID scaler,
	PS_ST PScaler_Event
)
{
	INT8U src = preview_ps_to_src(scaler);
	INT32U fifo_buf_done;
	INT32U fifo_buf_next;
	INT32U pend_msg;
	//INT32U *ptr;
	//FB_INFO* pfb_info;

	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl;
	ENC_STREAM_CTRL_T* pEncStramCtrl;
	ENC_STREAM_INFO_T* pEncStramInfo;

	const INT32U mask_event = PIPELINE_SCALER_STATUS_OVERFLOW_OCCUR |
		PIPELINE_SCALER_STATUS_FRAME_DONE |
		PIPELINE_SCALER_STATUS_BUF_A_DONE |
		PIPELINE_SCALER_STATUS_BUF_B_DONE |
		PIPELINE_SCALER_STATUS_BYPASS_FRAME_DONE |
		PIPELINE_SCALER_STATUS_BYPASS_BUF_A_DONE |
		PIPELINE_SCALER_STATUS_BYPASS_BUF_B_DONE |
		PIPELINE_SCALER_STATUS_AHB_DONE |
		PIPELINE_SCALER_STATUS_REG_REFRESH |
		PIPELINE_SCALER_STATUS_FRAME_DONE_LAST;

	if(!(PScaler_Event & mask_event))
	{
		// debug
		DBG_PRINT("[DEBUG]PS[%d]:other=0x%X\r\n", scaler, PScaler_Event);
	}

	if(PScaler_Event & PIPELINE_SCALER_STATUS_OVERFLOW_OCCUR)
	{
        DBG_PRINT("[TRACE]PS[%d]:overflow\r\n", scaler);
		/*
		over_cnt++;
		if(over_cnt >= 30){
			DBG_PRINT("[TRACE]PS[%d]:overflow\r\n", scaler);
			over_cnt=0;
		} */
	}

	pPscalerStreamCtrl = &pscaler_stream[src];
	if(!pPscalerStreamCtrl)
	{
		return;
	}

	pEncStramInfo = &pPscalerStreamCtrl->enc_stream_info;

	if(preview_need_display)
	{
		if(PScaler_Event & PIPELINE_SCALER_STATUS_REG_REFRESH)
		{
			preview_switch_done[scaler] = 1;
		}

		if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE || PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE)
		{
			#if DBG_FPS_ENABLE
			++fps_ps_frm_end[scaler];
			#endif
			INT32U disp_buf_done = DUMMY_BUFFER_ADDRS;
			INT32U disp_buf_next;
			//get draw done buffer
			if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE)
			{
				disp_buf_done = drv_l1_pscaler_output_A_buffer_get(scaler);
			}
			else
			if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE)
			{
				disp_buf_done = drv_l1_pscaler_output_B_buffer_get(scaler);
			}

			if(disp_buf_done < DUMMY_BUFFER_ADDRS)
			{
				disp_buf_done -= display_offset_byte_len;
			}

			if (_preview_mode != PREVIEW_ACT_SWITCH)
			{
				pend_msg = preview_process[_preview_mode][src].post_action(scaler, disp_buf_done);

				disp_buf_next = preview_process[_preview_mode][src].pre_action(scaler);
			}
			else
			{
				pend_msg = 0;
				disp_buf_next = DUMMY_BUFFER_ADDRS;
			}

			if(disp_buf_next < DUMMY_BUFFER_ADDRS)
			{
				disp_buf_next += display_offset_byte_len;
			}

			//set buffer for next frame
			if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE)
			{
				drv_l1_pscaler_output_A_buffer_set(scaler, disp_buf_next);
			}
			else
			if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE)
			{
				drv_l1_pscaler_output_B_buffer_set(scaler, disp_buf_next);
			}
			if(pEncStramInfo->stream_enable == VIDEO_RECORD_NOTHING)
			{
				if(((src == PREVIEW_SRC_FRONT) && ((_preview_mode == PREVIEW_MODE_FRONT) || (_preview_mode == PREVIEW_MODE_PIP_REAR)))
				|| ((src == PREVIEW_SRC_REAR) &&  ((_preview_mode == PREVIEW_MODE_REAR) || (_preview_mode == PREVIEW_MODE_PIP_FRONT)
#if !C_DISP_SLIDE_SAVE
				|| (_preview_mode == PREVIEW_MODE_BACK_KEEP_RATIO)
#endif
				)))
				{
					INT32U outW = preview_output_width_get(scaler, _preview_mode);
					INT32U outH = preview_output_height_get(scaler, _preview_mode);

					if(preview_out_h_start[src] != preview_out_h_start_next[src])
					{
						preview_out_h_start[src] = preview_out_h_start_next[src];
						drv_l1_pscaler_outcrop_set(scaler, 1, preview_out_w_start[src], preview_out_h_start[src], outW, outH);
						drv_l1_pscaler_register_refresh(scaler, REG_REFRESH_NOW);
					}
					else
					if(preview_out_w_start[src] != preview_out_w_start_next[src])
					{
						preview_out_w_start[src] = preview_out_w_start_next[src];
						drv_l1_pscaler_outcrop_set(scaler, 1, preview_out_w_start[src], preview_out_h_start[src], outW, outH);
						drv_l1_pscaler_register_refresh(scaler, REG_REFRESH_NOW);
					}
				}
#if C_DISP_SLIDE_SAVE
				else if((src == PREVIEW_SRC_REAR) && (_preview_mode == PREVIEW_MODE_BACK_KEEP_RATIO))
				{
					INT32U outW = preview_output_width_get(scaler, _preview_mode);
					INT32U outH = preview_output_height_get(scaler, _preview_mode);

					if(preview_out_h_start_reversing != preview_out_h_start_reversing_next)
					{
						preview_out_h_start_reversing = preview_out_h_start_reversing_next;
						drv_l1_pscaler_outcrop_set(scaler, 1, preview_out_w_start_reversing, preview_out_h_start_reversing, outW, outH);
						drv_l1_pscaler_register_refresh(scaler, REG_REFRESH_NOW);
					}
					else
					if(preview_out_w_start_reversing != preview_out_w_start_reversing_next)
					{
						preview_out_w_start_reversing = preview_out_w_start_reversing_next;
						drv_l1_pscaler_outcrop_set(scaler, 1, preview_out_w_start_reversing,preview_out_h_start_reversing, outW, outH);
						drv_l1_pscaler_register_refresh(scaler, REG_REFRESH_NOW);
					}
				}
#endif
			}

			if(pend_msg)
			{
				#if DBG_FPS_ENABLE
				++fps_ps_update[scaler];
				#endif

				if(preview_buf_fun_show)
				{
					INT16U x_offset = 0, y_offset = 0;

					if((_preview_mode == PREVIEW_MODE_FRONT) || (_preview_mode == PREVIEW_MODE_PIP_REAR))
					{
						x_offset = preview_out_off_x[PREVIEW_SRC_FRONT];
						y_offset = preview_out_off_y[PREVIEW_SRC_FRONT];
					}
					else if((_preview_mode == PREVIEW_MODE_REAR) || (_preview_mode == PREVIEW_MODE_PIP_FRONT) || (_preview_mode == PREVIEW_MODE_BACK_KEEP_RATIO))
					{
						x_offset = preview_out_off_x[PREVIEW_SRC_REAR];
						y_offset = preview_out_off_y[PREVIEW_SRC_REAR];
					}
					else if(_preview_mode == PREVIEW_MODE_BACK)
					{
						x_offset = preview_out_back_off_x;
						y_offset = preview_out_back_off_y;
					}

					#if C_ASYN_DBLE_TFT
					ap_Handle_switch_screen_status(scaler,preview_src_width[src],preview_src_height[src]);

					if(!tft_start_switch_screen_get())
					{
						preview_buf_fun_show(pend_msg, display_offset_byte_len, preview_car_baking,
						y_offset, y_offset, x_offset, x_offset);
					}
					else
					{
						if(preview_buf_fun_free)
							preview_buf_fun_free(pend_msg);	/* free unused display buffer */
					}
					#else

					preview_buf_fun_show(pend_msg, display_offset_byte_len, preview_car_baking,
					y_offset, y_offset, x_offset, x_offset);

					#endif


				}
			}
		}
	}//	if(preview_need_display)

#if DBG_FPS_ENABLE
	if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_BUF_A_DONE)
		++fps_ps_bypass_fifo[scaler];

	if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_BUF_B_DONE)
		++fps_ps_bypass_fifo[scaler];

	if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_FRAME_DONE)
		++fps_ps_bypass_fend[scaler];
#endif

	/*== HOST USB MJPEG CAM ==*/
	#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
	if(pscaler_mode[scaler] == PSCALER_MODE_DRAM)
	{
		if(PScaler_Event & PIPELINE_SCALER_STATUS_AHB_DONE)
		{
			INT32U in_buf;

			pscaler_busy[scaler] = 0;

			in_buf = drv_l1_pscaler_input_buffer_get(scaler);
			if(in_buf != DUMMY_BUFFER_ADDRS)
			{
				if(lib_interp2K_scaler_preview_done_notify(src,in_buf) == STATUS_FAIL)
				{
					// Release pscaler bypass output buffer
					pPscalerStreamCtrl->ps_free_func(in_buf);
				}
			}
		}

		/*
			Task sends message to trigger video recording in pscaler's dram mode
			Don't do the below code
		*/
		return;
	}
	#endif


	if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_FRAME_DONE)
	{
		if (_preview_mode==PREVIEW_ACT_SWITCH)
		{
			pend_msg = preview_process[_preview_mode][src].post_action(scaler, 0);  // Return pend_msg from post_action and display it
		}
	}
   	/*=== Record ===*/
	if(pEncStramInfo->stream_enable == VIDEO_RECORD_NOTHING)
	{
		return;
	}

	pEncStramCtrl = pPscalerStreamCtrl->enc_stream_ctrl;
	if(!pEncStramCtrl)
	{
		return; //no video encoder registered
	}

	/*== Video Record ==*/
	if(pEncStramInfo->stream_enable & (VIDEO_RECORD_START | VIDEO_RECORD_RECING | VIDEO_RECORD_STOP))
	{
		if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_BUF_A_DONE)
		{
			fifo_buf_done = drv_l1_pscaler_bypass_output_A_buffer_get(scaler);
			if(pEncStramInfo->stream_enable & VIDEO_RECORD_STOP)
			{
				fifo_buf_next = DUMMY_BUFFER_ADDRS;
			}
			else
			{
				fifo_buf_next = pPscalerStreamCtrl->ps_get_func();
			}

			++pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num;

			if(pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num < pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_max_count)
			{
				if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_A_buffer_set(scaler, fifo_buf_next))
				{
					drv_l1_pscaler_bypass_output_A_buffer_set(scaler, fifo_buf_done);
					fifo_buf_done = DUMMY_BUFFER_ADDRS;
					if(fifo_buf_next != DUMMY_BUFFER_ADDRS)
					{
						pPscalerStreamCtrl->ps_free_func(fifo_buf_next);
					}
				}
			}
			else
			{
				if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_FRAME_DONE)
				{
					if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_A_buffer_set(scaler, fifo_buf_next))
					{
						drv_l1_pscaler_bypass_output_A_buffer_set(scaler, fifo_buf_done);
						fifo_buf_done = DUMMY_BUFFER_ADDRS;
						if(fifo_buf_next != DUMMY_BUFFER_ADDRS)
						{
							pPscalerStreamCtrl->ps_free_func(fifo_buf_next);
						}
					}
				}
				else
				{
					if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_A_buffer_set(scaler, fifo_buf_next))
					{
						drv_l1_pscaler_bypass_output_A_buffer_set(scaler, fifo_buf_done);
						fifo_buf_done = DUMMY_BUFFER_ADDRS;
						if(fifo_buf_next != DUMMY_BUFFER_ADDRS)
						{
							pPscalerStreamCtrl->ps_free_func(fifo_buf_next);
						}
					}

					pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num = 0;
				}
			}

			if(pEncStramInfo->stream_enable & (VIDEO_RECORD_RECING | VIDEO_RECORD_STOP))
			{
				if(pPscalerStreamCtrl->ps_notify_func)
				{
					pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,fifo_buf_done);
				}
				else
				{
					if(fifo_buf_done != DUMMY_BUFFER_ADDRS)
					{
						pPscalerStreamCtrl->ps_free_func(fifo_buf_done);
					}
				}
			}
			else
			{
				if(fifo_buf_done != DUMMY_BUFFER_ADDRS)
				{
					pPscalerStreamCtrl->ps_free_func(fifo_buf_done);
				}
			}
		}

		if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_BUF_B_DONE)
		{
			fifo_buf_done = drv_l1_pscaler_bypass_output_B_buffer_get(scaler);

			if(pEncStramInfo->stream_enable & VIDEO_RECORD_STOP)
			{
				fifo_buf_next = DUMMY_BUFFER_ADDRS;
			}
			else
			{
				fifo_buf_next = pPscalerStreamCtrl->ps_get_func();
			}

			++pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num;

			if(pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num < pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_max_count)
			{
				if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_B_buffer_set(scaler, fifo_buf_next))
				{
					drv_l1_pscaler_bypass_output_B_buffer_set(scaler, fifo_buf_done);
					fifo_buf_done = DUMMY_BUFFER_ADDRS;
					if(fifo_buf_next != DUMMY_BUFFER_ADDRS)
					{
						pPscalerStreamCtrl->ps_free_func(fifo_buf_next);
					}
				}
			}
			else
			{
				if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_FRAME_DONE)
				{
					if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_B_buffer_set(scaler, fifo_buf_next))
					{
						drv_l1_pscaler_bypass_output_B_buffer_set(scaler, fifo_buf_done);
						fifo_buf_done = DUMMY_BUFFER_ADDRS;
						if(fifo_buf_next != DUMMY_BUFFER_ADDRS)
						{
							pPscalerStreamCtrl->ps_free_func(fifo_buf_next);
						}
					}
				}
				else
				{
					if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_B_buffer_set(scaler, fifo_buf_next))
					{
						drv_l1_pscaler_bypass_output_B_buffer_set(scaler, fifo_buf_done);
						fifo_buf_done = DUMMY_BUFFER_ADDRS;
						if(fifo_buf_next != DUMMY_BUFFER_ADDRS)
						{
							pPscalerStreamCtrl->ps_free_func(fifo_buf_next);
						}
					}

					pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num = 0;
				}
			}

			if(pEncStramInfo->stream_enable & (VIDEO_RECORD_RECING | VIDEO_RECORD_STOP))
			{
				if(pPscalerStreamCtrl->ps_notify_func)
				{
					pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,fifo_buf_done);
				}
				else
				{
					if(fifo_buf_done != DUMMY_BUFFER_ADDRS)
					{
						pPscalerStreamCtrl->ps_free_func(fifo_buf_done);
					}
				}
			}
			else
			{
				if(fifo_buf_done != DUMMY_BUFFER_ADDRS)
				{
					pPscalerStreamCtrl->ps_free_func(fifo_buf_done);
				}
			}
		}

		if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_FRAME_DONE)
		{
			// Record
			if(pEncStramInfo->stream_enable == VIDEO_RECORD_START)
			{
				pEncStramInfo->stream_enable = VIDEO_RECORD_RECING;
			}
			else
			if(pEncStramInfo->stream_enable == VIDEO_RECORD_RECING)
			{
				if(pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num < pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_max_count)
				{
				if(pPscalerStreamCtrl->ps_notify_func)
					{
						pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,DUMMY_BUFFER_ADDRS);
					}
				}
			}
			else
			if((pEncStramInfo->stream_enable == VIDEO_RECORD_STOP)
				&& (drv_l1_pscaler_bypass_output_A_buffer_get(scaler) == DUMMY_BUFFER_ADDRS)
				&& (drv_l1_pscaler_bypass_output_B_buffer_get(scaler) == DUMMY_BUFFER_ADDRS)
			)
			{
				pEncStramInfo->stream_enable = VIDEO_RECORD_NOTHING;
				pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num = DUMMY_BUFFER_ADDRS;
				if(pPscalerStreamCtrl->ps_notify_func)
				{
					pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,DUMMY_BUFFER_ADDRS);
				}

				//DBG_PRINT("REC[%d]Pscaler Stop\r\n",scaler);
			}

			pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num = 0;

			if(((src == PREVIEW_SRC_FRONT) && ((_preview_mode == PREVIEW_MODE_FRONT) || (_preview_mode == PREVIEW_MODE_PIP_REAR)))
				|| ((src == PREVIEW_SRC_REAR) &&  ((_preview_mode == PREVIEW_MODE_REAR) || (_preview_mode == PREVIEW_MODE_PIP_FRONT)
#if !C_DISP_SLIDE_SAVE
				|| (_preview_mode == PREVIEW_MODE_BACK_KEEP_RATIO)
#endif
				)))
			{
				INT32U outW = preview_output_width_get(scaler, _preview_mode);
				INT32U outH = preview_output_height_get(scaler, _preview_mode);

				if(preview_out_h_start[src] != preview_out_h_start_next[src])
				{
					preview_out_h_start[src] = preview_out_h_start_next[src];
					drv_l1_pscaler_outcrop_set(scaler, 1, preview_out_w_start[src], preview_out_h_start[src], outW, outH);
					drv_l1_pscaler_register_refresh(scaler, REG_REFRESH_NOW);
				}
				else
				if(preview_out_w_start[src] != preview_out_w_start_next[src])
				{
					preview_out_w_start[src] = preview_out_w_start_next[src];
					drv_l1_pscaler_outcrop_set(scaler, 1, preview_out_w_start[src], preview_out_h_start[src], outW, outH);
					drv_l1_pscaler_register_refresh(scaler, REG_REFRESH_NOW);
				}
			}
#if C_DISP_SLIDE_SAVE
			else if((src == PREVIEW_SRC_REAR) && (_preview_mode == PREVIEW_MODE_BACK_KEEP_RATIO))
			{
				INT32U outW = preview_output_width_get(scaler, _preview_mode);
				INT32U outH = preview_output_height_get(scaler, _preview_mode);

				if(preview_out_h_start_reversing != preview_out_h_start_reversing_next)
				{
					preview_out_h_start_reversing = preview_out_h_start_reversing_next;
					drv_l1_pscaler_outcrop_set(scaler, 1, preview_out_w_start_reversing, preview_out_h_start_reversing, outW, outH);
					drv_l1_pscaler_register_refresh(scaler, REG_REFRESH_NOW);
				}
				else
				if(preview_out_w_start_reversing != preview_out_w_start_reversing_next)
				{
					preview_out_w_start_reversing = preview_out_w_start_reversing_next;
					drv_l1_pscaler_outcrop_set(scaler, 1, preview_out_w_start_reversing,preview_out_h_start_reversing, outW, outH);
					drv_l1_pscaler_register_refresh(scaler, REG_REFRESH_NOW);
				}
			}
#endif
		}

	}// End of video record

	/*== Photo Capture ==*/
	if(pEncStramInfo->stream_enable & (VIDEO_RECORD_PHOTO_START | VIDEO_RECORD_PHOTO))
	{
		if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_BUF_A_DONE)
		{
			fifo_buf_done = drv_l1_pscaler_bypass_output_A_buffer_get(scaler);

			if(fifo_buf_done != DUMMY_BUFFER_ADDRS)
			{
				if(pPscalerStreamCtrl->ps_notify_func)
				{
					pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,fifo_buf_done);
				}
			}

			fifo_buf_next = pPscalerStreamCtrl->ps_get_func();

			if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_A_buffer_set(scaler, fifo_buf_next))
			{
				drv_l1_pscaler_bypass_output_A_buffer_set(scaler, DUMMY_BUFFER_ADDRS);
			}
		}

		if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_BUF_B_DONE)
		{
			fifo_buf_done = drv_l1_pscaler_bypass_output_B_buffer_get(scaler);

			if(fifo_buf_done != DUMMY_BUFFER_ADDRS)
			{
				if(pPscalerStreamCtrl->ps_notify_func)
				{
					pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,fifo_buf_done);
				}
			}

			fifo_buf_next = pPscalerStreamCtrl->ps_get_func();

			if(PSCALER_RET_SUCCESS != drv_l1_pscaler_bypass_output_B_buffer_set(scaler, fifo_buf_next))
			{
				drv_l1_pscaler_bypass_output_B_buffer_set(scaler, DUMMY_BUFFER_ADDRS);
			}
		}

		if(PScaler_Event & PIPELINE_SCALER_STATUS_BYPASS_FRAME_DONE)
		{
			if(pEncStramInfo->stream_enable == VIDEO_RECORD_PHOTO_START)
			{
				pEncStramInfo->stream_enable = VIDEO_RECORD_PHOTO;
			}
			else
			if((pEncStramInfo->stream_enable == VIDEO_RECORD_PHOTO)
				&& (drv_l1_pscaler_bypass_output_A_buffer_get(scaler) == DUMMY_BUFFER_ADDRS)
				&& (drv_l1_pscaler_bypass_output_B_buffer_get(scaler) == DUMMY_BUFFER_ADDRS)
			)
			{
				//DBG_PRINT("PHO[%d]Pscaler Stop\r\n",scaler);
				pEncStramInfo->stream_enable = VIDEO_RECORD_NOTHING;
				if(msgQSend(pPscalerStreamCtrl->ps_notify_q, (MSG_VID_PSCALER_NOTIFY|src), NULL, 0, MSG_PRI_NORMAL) < 0)
				{
					ERR_PRINT("fail to msgQSend [%d]\r\n",src);
				}
			}
		}
	} // End of photo capture
}

#if defined(C_Y_ONLY_FUNCTION) && C_Y_ONLY_FUNCTION == CUSTOM_ON
void
sensor_y_only_src_path_set(
	Y_ONLY_IN_PATH path
)
{
	y_only_src_path = path;
}
static Y_ONLY_IN_PATH
sensor_y_only_src_path_get(
	void
)
{
	return y_only_src_path;
}
void
sensor_y_only_src_format_set(
	INT32U format
)
{
	y_only_src_format = format;
}


static INT32U
sensor_y_only_src_format_get(
	void
)
{
	return y_only_src_format;
}

void
sensor_y_only_in_width_set(
	INT16U width
)
{
	y_only_in_width= width;
}
static INT16U
sensor_y_only_in_width_get(
	void
)
{
	return y_only_in_width;
}
void
sensor_y_only_in_height_set(
	INT16U height
)
{
	y_only_in_height= height;
}
static INT16U
sensor_y_only_in_height_get(
	void
)
{
	return y_only_in_height;
}

void
sensor_y_only_clip_width_set(
	INT16U width
)
{
	y_only_clip_width= width;
}
static INT16U
sensor_y_only_clip_width_get(
	void
)
{
	return y_only_clip_width;
}
void
sensor_y_only_clip_height_set(
	INT16U height
)
{
	y_only_clip_height= height;
}
static INT16U
sensor_y_only_clip_height_get(
	void
)
{
	return y_only_clip_height;
}
void
sensor_y_only_out_width_set(
	INT16U width
)
{
	y_only_out_width = width;
}
static INT16U
sensor_y_only_out_width_get(
	void
)
{
	return y_only_out_width;
}
void
sensor_y_only_out_height_set(
	INT16U height
)
{
	y_only_out_height = height;
}
static INT16U
sensor_y_only_out_height_get(
	void
)
{
	return y_only_out_height;
}


void
sensor_y_only_in_fifo_line_set(
	INT8U line
)
{
	y_only_in_fifo_line = line;
}
static PIPELINE_SCALER_IN_FIFO_LINES
sensor_y_only_in_fifo_line_get(
	void
)
{
	return y_only_in_fifo_line;
}

void
sensor_y_only_out_pixel_offset_set(
	INT16U line, INT16U width
)
{
	y_only_pixel_offset = line * width;
}
static INT16U
sensor_y_only_out_pixel_offset_get(
	void
)
{
	return y_only_pixel_offset;
}

INT8U
y_only_fifo_malloc(
	Y_ONLY_HANDLE *handle,
	INT32U frame_size,
	INT8U buff_length,
	INT16U total_index
)
{
	INT8U axis;
	INT32U size;
	size = buff_length * (sizeof(INT32) + sizeof(INT8U) + sizeof(INT32U) + sizeof(INT16U));

	if(handle->pool.address == NULL)
	{
		handle->pool.address = gp_malloc(size);
		if(handle->pool.address == NULL)
		{
			gp_free(handle);
			return 0;
		}
	}

	gp_memset((void *)handle->pool.address, 0, size);
	handle->queue.address = (INT32U *)(handle->pool.address + buff_length);
	handle->queue.data_index = (INT16U *)(handle->queue.address + buff_length);
	handle->pool.hold = (INT8U *)(handle->queue.data_index + buff_length);

	handle->pool.address[0] = (INT32U)gp_malloc(frame_size * buff_length);
	if(handle->pool.address[0] == 0)
	{
		gp_free(handle->pool.address);
		gp_free(handle);
        return 0;
	}
	gp_memset((void *)handle->pool.address[0], 0x00, frame_size * buff_length);
	for(axis = 1; axis < buff_length; axis++)
	{
		handle->pool.address[axis] = handle->pool.address[0] + frame_size * axis;
	}

	handle->pool.max_size = buff_length;
	handle->queue.total_index = total_index;
	return 1;
}

void
y_only_fifo_free(
	Y_ONLY_HANDLE *handle
)
{
	if(handle == NULL)
	{
		return;
	}
	if(handle->pool.address != NULL)
	{
        gp_free((void *)handle->pool.address[0]);
		gp_free(handle->pool.address);
		handle->pool.address = NULL;
		DBG_PRINT("Free Yonly FIFO\r\n");
	}
}

INT32U
y_only_fifo_get_read_address(
	Y_ONLY_HANDLE *handle,
	INT16U *index
)
{
	INT32U buf = DUMMY_BUFFER_ADDRS;
	if(handle == NULL || handle->pool.max_size == 0)
	{
		return buf;
	}
	if(handle->queue.size > 0)
	{
		buf = handle->queue.address[handle->queue.pull];
		handle->queue.address[handle->queue.pull] = 0;
		if(index != 0)
		{
			*index = handle->queue.data_index[handle->queue.pull];
		}
		handle->queue.data_index[handle->queue.pull] = 0;
		if(++handle->queue.pull == handle->pool.max_size)
		{
			handle->queue.pull = 0;
		}
		--handle->queue.size;
	}
	return buf;
}

INT8U
y_only_fifo_read_done(
	Y_ONLY_HANDLE *handle,
	INT32U done_address
)
{
	INT8U axis;
	INT8U status = 0;
	if(handle == NULL || done_address == DUMMY_BUFFER_ADDRS || done_address == 0 || handle->pool.in_use_size == 0)
	{
		return status;
	}
	for(axis = 0; axis < handle->pool.max_size; axis++)
	{
		if(done_address == handle->pool.address[handle->pool.free])
		{
			status = 1;
			if(handle->pool.hold[handle->pool.free])
			{
				handle->pool.hold[handle->pool.free] = 0;
				--handle->pool.in_use_size;
			}
			if(++handle->pool.free == handle->pool.max_size)
			{
				handle->pool.free = 0;
			}
			break;
		}
		else
		{
			if(++handle->pool.free == handle->pool.max_size)
			{
				handle->pool.free = 0;
			}
		}
	}
	return status;
}

static INT32U
y_only_fifo_get_write_address(
	Y_ONLY_HANDLE *handle
)
{
	INT32U buf = DUMMY_BUFFER_ADDRS;
	if(handle == NULL || handle->pool.max_size == 0 || handle->pool.in_use_size == handle->pool.max_size)
	{
		return DUMMY_BUFFER_ADDRS;
	}
	do
	{
		if(!handle->pool.hold[handle->pool.get])
		{
			handle->pool.hold[handle->pool.get] = 1;
			buf =  (INT32U)handle->pool.address[handle->pool.get];
			++handle->pool.in_use_size;
			//DBG_PRINT("Get write address:0x%x, buf:%d\r\n", buf, PSFIFOQueue.write);
		}
		++handle->pool.get;
		if(handle->pool.get == handle->pool.max_size)
		{
			handle->pool.get = 0;
		}
	}while(buf == DUMMY_BUFFER_ADDRS);
	return buf;
}

static INT8U
y_only_fifo_write_done(
	Y_ONLY_HANDLE *handle,
	INT32U done_address,
	INT32U index
)
{
	INT8U status = 0;
	if(handle == NULL || done_address == DUMMY_BUFFER_ADDRS || done_address == 0 || handle->pool.max_size == handle->queue.size)
	{
		return status;
	}
	handle->queue.address[handle->queue.push] = done_address;
	handle->queue.data_index[handle->queue.push] = index;
	++handle->queue.push;
	if(handle->queue.push == handle->pool.max_size)
	{
		handle->queue.push = 0;
	}
	++handle->queue.size;
	return status;
}

void y_only_fifo_clean(
	Y_ONLY_HANDLE *handle
)
{
	if(handle == NULL || handle->pool.max_size == 0)
	{
		return;
	}
	gp_memset((INT8S *)handle->pool.hold, 0, handle->pool.max_size);
	handle->pool.in_use_size = 0;
	handle->pool.free = 0;
	handle->pool.get = 0;
	gp_memset((INT8S *)handle->queue.address, 0, handle->pool.max_size << 2);
	gp_memset((INT8S *)handle->queue.data_index, 0, handle->pool.max_size << 1);
	handle->queue.push= 0;
	handle->queue.pull= 0;
	handle->queue.size= 0;
}

static void
y_only_pscaler_parm_set(
	INT32U input_address,
	INT32U out_a_address,
	INT32U out_b_address,
	void (*ps_isr) (PS_ID, PS_ST)
);

static void
y_only_ps_isr(
	PS_ID scaler,
	PS_ST PScaler_Event
)
{
	INT32U out_fifo_done, out_fifo_next;
	INT32U in_fifo_done, in_fifo_next;
	static INT16U in_index = 0, out_index = 0;
	static INT8U ahb_done = 0, frame_done = 0;
	const INT32U mask_event = PIPELINE_SCALER_STATUS_OVERFLOW_OCCUR |
		PIPELINE_SCALER_STATUS_FRAME_DONE |
		PIPELINE_SCALER_STATUS_BUF_A_DONE |
		PIPELINE_SCALER_STATUS_BUF_B_DONE |
		PIPELINE_SCALER_STATUS_INPUT_EMPTY |
		PIPELINE_SCALER_STATUS_AHB_DONE |
		PIPELINE_SCALER_STATUS_REG_REFRESH;

	if(!(PScaler_Event & mask_event))
	{
		DBG_PRINT("[DEBUG]PS[%d]:other=0x%X\r\n", scaler, PScaler_Event);
	}

	if(PScaler_Event & PIPELINE_SCALER_STATUS_OVERFLOW_OCCUR)
	{
		ERR_PRINT("PS[%d] overflow occur\r\n", scaler);
		y_only_fifo_clean(&y_only_out_handle);
		y_only_fifo_clean(&y_only_in_handle);
		drv_l1_pscaler_stop(scaler);
		pscaler_busy[scaler] = 0;
		pscaler_mode[scaler] = PSCALER_MODE_STOP;
		y_only_pscaler_parm_set(DUMMY_BUFFER_ADDRS, DUMMY_BUFFER_ADDRS, DUMMY_BUFFER_ADDRS, y_only_ps_isr);
	}

	//Only for dram mode
	if(PScaler_Event & PIPELINE_SCALER_STATUS_INPUT_EMPTY)
	{
		in_fifo_done = drv_l1_pscaler_input_buffer_get(scaler);
		if(in_fifo_done != DUMMY_BUFFER_ADDRS)
		{
			y_only_fifo_read_done(&y_only_in_handle, in_fifo_done);
		}
		if(sensor_y_only_in_fifo_line_get() != 0)
		{
			in_fifo_next = y_only_fifo_get_read_address(&y_only_in_handle, &in_index);
			if(in_fifo_next != DUMMY_BUFFER_ADDRS)
			{
				drv_l1_pscaler_input_buffer_set(scaler, in_fifo_next);
				drv_l1_pscaler_input_fifo_restart(scaler);
			}
		}
	}

	if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE)
	{
		out_fifo_done = drv_l1_pscaler_output_A_buffer_get(scaler);
		if(out_fifo_done != DUMMY_BUFFER_ADDRS)
		{
			out_fifo_done -= sensor_y_only_out_pixel_offset_get();
			y_only_fifo_write_done(&y_only_out_handle, out_fifo_done, ++out_index);
		}

		if(pscaler_mode[scaler] != PSCALER_MODE_DRAM)
		{
			out_fifo_next = y_only_fifo_get_write_address(&y_only_out_handle);
			if(out_fifo_next != DUMMY_BUFFER_ADDRS)
			{
				out_fifo_next += sensor_y_only_out_pixel_offset_get();
			}
			drv_l1_pscaler_output_A_buffer_set(scaler, out_fifo_next);
		}
	}
	else
	if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE)
	{
		out_fifo_done = drv_l1_pscaler_output_B_buffer_get(scaler);
		if(out_fifo_done != DUMMY_BUFFER_ADDRS)
		{
			out_fifo_done -= sensor_y_only_out_pixel_offset_get();
			y_only_fifo_write_done(&y_only_out_handle, out_fifo_done, ++out_index);
		}

		if(pscaler_mode[scaler] != PSCALER_MODE_DRAM)
		{
			out_fifo_next = y_only_fifo_get_write_address(&y_only_out_handle);
			if(out_fifo_next != DUMMY_BUFFER_ADDRS)
			{
				out_fifo_next += sensor_y_only_out_pixel_offset_get();
			}
			drv_l1_pscaler_output_B_buffer_set(scaler, out_fifo_next);
		}
	}

	//Only for dram mode
	if(PScaler_Event & PIPELINE_SCALER_STATUS_AHB_DONE)
	{
		ahb_done = 1;
		if(sensor_y_only_in_fifo_line_get() != 0)
		{
			if(y_only_in_handle.queue.total_index != in_index)
			{
				y_only_fifo_clean(&y_only_in_handle);
			}
			else
			{
				in_fifo_done = drv_l1_pscaler_input_buffer_get(scaler);
				if(in_fifo_done != DUMMY_BUFFER_ADDRS)
				{
					y_only_fifo_read_done(&y_only_in_handle, in_fifo_done);
				}
			}
		}
	}

	if(PScaler_Event & PIPELINE_SCALER_STATUS_FRAME_DONE)
	{
		frame_done = 1;

		y_only_img_args.address = y_only_fifo_get_read_address(&y_only_out_handle, &out_index);
		out_index = 0;
	}

	if(ahb_done && frame_done)
	{
		pscaler_busy[scaler] = 0;
		pscaler_mode[scaler] = PSCALER_MODE_STOP;
		drv_l1_pscaler_stop(scaler);
	}

}

static void
y_only_pscaler_parm_set(
	INT32U input_address,
	INT32U out_a_address,
	INT32U out_b_address,
	void (*ps_isr) (PS_ID, PS_ST)
)
{
	INT32U	in_width, in_height;
	INT32U	clip_width, clip_height;
	INT32U	clip_done_width = 0, clip_done_height = 0;
	INT32U	out_fill_width, out_fill_height;
	INT32U	wFactor, hFactor;
	INT32U	wIntegration, hIntegration;
	INT8U	h_first;
	INT32U	out_pixel_offset;
	INT32U  scale_width, scale_height;
	INT32U  mul1, mul2;
	in_width = sensor_y_only_in_width_get();
	in_height = sensor_y_only_in_height_get();
	clip_width = sensor_y_only_clip_width_get();
	clip_height = sensor_y_only_clip_height_get();

	if(clip_width > in_width)
	{
		clip_width = in_width;
		sensor_y_only_clip_width_set(clip_width);
	}

	if(clip_height > in_height)
	{
		clip_height = in_height;
		sensor_y_only_clip_height_set(clip_height);
	}


	out_fill_width = sensor_y_only_out_width_get();
	out_fill_height = sensor_y_only_out_height_get();

	mul1 = clip_width * out_fill_height;
	mul2 = clip_height * out_fill_width;
	if(mul1 >= mul2)
	{
		if(out_fill_width > clip_width)
		{
			out_fill_height = out_fill_width = clip_width;
			sensor_y_only_out_width_set(out_fill_width);
			sensor_y_only_out_height_set(out_fill_height);
		}
		scale_height = in_height * out_fill_width / clip_width;
		scale_width = in_width * out_fill_width / clip_width;
	}
	else
	{
		if(out_fill_height > clip_height)
		{
			out_fill_width = out_fill_height = clip_height;
			sensor_y_only_out_width_set(out_fill_width);
			sensor_y_only_out_height_set(out_fill_height);
		}
		scale_height = in_height * out_fill_height / clip_height;
		scale_width = in_width * out_fill_height / clip_height;
	}

	if(scale_width >= (in_width >> 1))
	{
		wIntegration = 0;
		wFactor = ((in_width) << 16) / (scale_width);
		if((in_width << 16) % scale_width) ++wFactor;
	}
	else
	{
		wIntegration = 1;
		wFactor = (scale_width << 16) / (in_width - 1) + 1;
		if((scale_width << 16) % in_width) ++wFactor;
	}
	hFactor = wFactor;
	hIntegration = wIntegration;
	if(mul1 >= mul2)
	{
		clip_done_height = clip_height * out_fill_width / clip_width;
		sensor_y_only_out_pixel_offset_set((out_fill_height - clip_done_height) >> 1, out_fill_width);
	}
	else
	{
		clip_done_width = clip_width* out_fill_height/ clip_height;
		sensor_y_only_out_pixel_offset_set(0, 0);
	}
	out_pixel_offset = sensor_y_only_out_pixel_offset_get();
	if(!pscaler_busy[PSCALER_Y])
	{
		drv_l1_pscaler_init(PSCALER_Y);

		drv_l1_pscaler_outedge_set(
			PSCALER_Y
			, OUTEDGE_TYPE_1
			, OUTEDGE_CLP_64
			, OUTEDGE_COR_2
			, 8
		);
		switch(sensor_y_only_src_path_get())
		{
			case Y_ONLY_IN_PATH_CDSP:
				drv_l1_CdspSetYuvPscalePath(1);
				pscaler_mode[PSCALER_Y] = PSCALER_MODE_PIPE;
				drv_l1_pscaler_input_source_set(PSCALER_Y, PIPELINE_SCALER_INPUT_SOURCE_CDSP);
				drv_l1_pscaler_output_A_buffer_set(PSCALER_Y, DUMMY_BUFFER_ADDRS);
				drv_l1_pscaler_output_B_buffer_set(PSCALER_Y, DUMMY_BUFFER_ADDRS);
			break;
			case Y_ONLY_IN_PATH_CSI0:
				drv_l1_csi_input_pscaler_set(CSI_0,1);
				pscaler_mode[PSCALER_Y] = PSCALER_MODE_PIPE;
				drv_l1_pscaler_input_source_set(PSCALER_Y, PIPELINE_SCALER_INPUT_SOURCE_CSI);
				drv_l1_pscaler_output_A_buffer_set(PSCALER_Y, DUMMY_BUFFER_ADDRS);
				drv_l1_pscaler_output_B_buffer_set(PSCALER_Y, DUMMY_BUFFER_ADDRS);
			break;
			case Y_ONLY_IN_PATH_DRAM:
				pscaler_mode[PSCALER_Y] = PSCALER_MODE_DRAM;
				drv_l1_pscaler_input_source_set(PSCALER_Y, PIPELINE_SCALER_INPUT_SOURCE_DRAM);
				drv_l1_pscaler_input_format_set(PSCALER_Y, sensor_y_only_src_format_get());
				if(sensor_y_only_src_format_get() == PIPELINE_SCALER_INPUT_FORMAT_GP420)
				{
					cache_invalid_range(input_address, in_width * in_height * 3 / 2);
				}
				else if(sensor_y_only_src_format_get() == PIPELINE_SCALER_INPUT_FORMAT_Y)
				{
					cache_invalid_range(input_address, in_width * in_height);
				}
				drv_l1_pscaler_input_buffer_set(PSCALER_Y, input_address);
				drv_l1_pscaler_output_A_buffer_set(PSCALER_Y, (out_a_address == DUMMY_BUFFER_ADDRS)?DUMMY_BUFFER_ADDRS:out_a_address + out_pixel_offset);
				drv_l1_pscaler_output_B_buffer_set(PSCALER_Y, (out_b_address == DUMMY_BUFFER_ADDRS)?DUMMY_BUFFER_ADDRS:out_b_address + out_pixel_offset);
			break;
			default:
				pscaler_mode[PSCALER_Y] = PSCALER_MODE_STOP;
				drv_l1_pscaler_input_buffer_set(PSCALER_Y, DUMMY_BUFFER_ADDRS);
				drv_l1_pscaler_output_A_buffer_set(PSCALER_Y, DUMMY_BUFFER_ADDRS);
				drv_l1_pscaler_output_B_buffer_set(PSCALER_Y, DUMMY_BUFFER_ADDRS);
			break;
		}
		drv_l1_pscaler_callback_register2(PSCALER_Y, ps_isr);
	}

	drv_l1_pscaler_input_X_start_set(PSCALER_Y, 0);
	drv_l1_pscaler_input_Y_start_set(PSCALER_Y, 0);

	drv_l1_pscaler_pixels_set(PSCALER_Y, in_width, in_height, scale_width, scale_height, wFactor, hFactor);
	drv_l1_pscaler_integration_mode_set(PSCALER_Y, hIntegration, wIntegration);
	if(mul1 >= mul2)
	{
		drv_l1_pscaler_outcrop_set(PSCALER_Y, 1, (scale_width - out_fill_width) >> 1, (scale_height - clip_done_height) >> 1, out_fill_width, clip_done_height);
		y_only_img_args.width = out_fill_width;
		y_only_img_args.height = clip_done_height;
	}
	else
	{
		drv_l1_pscaler_outcrop_set(PSCALER_Y, 1, (scale_width - clip_done_width) >> 1, (scale_height - out_fill_height) >> 1, clip_done_width, out_fill_height);
		drv_l1_pscaler_outfill_set(PSCALER_Y, 1, (out_fill_width - clip_done_width) >> 1, 0, out_fill_width, out_fill_height);
		y_only_img_args.width = out_fill_width;
		y_only_img_args.height = out_fill_height;
	}

	drv_l1_pscaler_outboundary_color_set(PSCALER_Y, 1, 0);

	drv_l1_pscaler_input_fifo_line_set(PSCALER_Y, 0, sensor_y_only_in_fifo_line_get());

	drv_l1_pscaler_output_fifo_line_set(PSCALER_Y, out_fill_height, 0);


	if(in_width > 2688 && out_fill_width <= 2688)
	{
		h_first = 1;
	}
	else
	{
		h_first = 0;
	}
	drv_l1_pscaler_H_first_enable(PSCALER_Y, h_first);

	drv_l1_pscaler_drop_frame_set(PSCALER_Y, 0, 0);

	if(!pscaler_busy[PSCALER_Y])
	{
		if(pscaler_mode[PSCALER_Y] == PSCALER_MODE_DRAM)
		{
			//drv_l1_pscaler_HSYNC_latch_delay(PSCALER_Y, 0x20);
			drv_l1_pscaler_register_refresh_last_eof_config(PSCALER_Y, REG_REFRESH_LAST_EOF_SCALE | REG_REFRESH_LAST_EOF_READ);
		}
		drv_l1_pscaler_interrupt_enable(PSCALER_Y, 1);
		drv_l1_pscaler_register_refresh(PSCALER_Y, REG_REFRESH_NOW);
		pscaler_busy[PSCALER_Y] = 1;
		drv_l1_pscaler_start(PSCALER_Y);
	}
	else
	{
		if(pscaler_mode[PSCALER_Y] == PSCALER_MODE_PIPE)
		{
			drv_l1_pscaler_register_refresh(PSCALER_Y, REG_REFRESH_EOF_LAST);
		}
	}
}
#endif

#if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
#include "ap_blind_spot_detection.h"
extern INT32U bsd_gray_buf;
BSD_y_only_pscaler_parm_set(
	INT32U input_address,
	INT32U out_a_address,
	INT32U out_b_address,
	void (*ps_isr) (PS_ID, PS_ST)
)
{
	INT32U	in_width, in_height;
	INT32U	out_width, out_height;
	INT32U	clip_done_width = 0, clip_done_height = 0;
	INT32U	out_fill_width, out_fill_height;
	INT32U	wFactor, hFactor;
	INT32U	wIntegration, hIntegration;
	INT8U	h_first;
	INT32U	out_pixel_offset;
	INT32U  scale_width, scale_height;
	INT32U  mul1, mul2;
	in_width = preview_src_width_get(preview_src_to_ps(PREVIEW_SRC_FRONT));
	in_height = preview_src_height_get(preview_src_to_ps(PREVIEW_SRC_FRONT));	
	out_width = BSD_SCALE_TAR_WIDTH;
	out_height = BSD_SCALE_TAR_HEIGHT;

	if(!pscaler_busy[PSCALER_Y])
	{
		drv_l1_pscaler_init(PSCALER_Y);

		drv_l1_pscaler_outedge_set(
			PSCALER_Y
			, OUTEDGE_TYPE_1
			, OUTEDGE_CLP_64
			, OUTEDGE_COR_2
			, 8
		);

		drv_l1_CdspSetYuvPscalePath(1);
		pscaler_mode[PSCALER_Y] = PSCALER_MODE_PIPE;
		drv_l1_pscaler_input_source_set(PSCALER_Y, PIPELINE_SCALER_INPUT_SOURCE_CDSP);
		drv_l1_pscaler_output_A_buffer_set(PSCALER_Y, out_a_address);
		drv_l1_pscaler_output_B_buffer_set(PSCALER_Y, out_b_address);

		drv_l1_pscaler_callback_register2(PSCALER_Y, ps_isr);
	}

	drv_l1_pscaler_input_X_start_set(PSCALER_Y, 0);
	drv_l1_pscaler_input_Y_start_set(PSCALER_Y, 0);

    wIntegration = 1;
	wFactor = (out_width << 16) / (in_width - 1) + 1;
	if((out_width << 16) % in_width) ++wFactor;

	hFactor = (out_height << 16) / (in_height - 1) + 1;
	if((out_height << 16) % in_height) ++hFactor;
	hIntegration = wIntegration;
	drv_l1_pscaler_pixels_set(PSCALER_Y, in_width, in_height, out_width, out_height, wFactor, hFactor);
	drv_l1_pscaler_integration_mode_set(PSCALER_Y, hIntegration, wIntegration);

	drv_l1_pscaler_outcrop_set(PSCALER_Y, 0, 0, 0, 0, 0);
	drv_l1_pscaler_outfill_set(PSCALER_Y, 0, 0, 0, 0, 0);

	drv_l1_pscaler_outboundary_color_set(PSCALER_Y, 1, 0);

	drv_l1_pscaler_input_fifo_line_set(PSCALER_Y, 0, PSCALER_IN_FIFO_FRAME_MODE);

	drv_l1_pscaler_output_fifo_line_set(PSCALER_Y, out_height, 0);

	if(in_width >= 1280)
        drv_l1_pscaler_H_first_enable(PSCALER_Y, 1);
    else
        drv_l1_pscaler_H_first_enable(PSCALER_Y, 0);

	drv_l1_pscaler_drop_frame_set(PSCALER_Y, 0, 0);

	if(!pscaler_busy[PSCALER_Y])
	{
		drv_l1_pscaler_interrupt_enable(PSCALER_Y, 1);
		drv_l1_pscaler_register_refresh(PSCALER_Y, REG_REFRESH_NOW);
		pscaler_busy[PSCALER_Y] = 1;
		drv_l1_pscaler_start(PSCALER_Y);
	}
	else
	{
		DBG_PRINT("BSD y only pscale initial fail!\r\n");
	}
}
#endif

void PScaler_BSD_Gray_Callback(PS_ID scaler, PS_ST PScaler_Event);
INT32S
sensor_preview_start(
	void
)
{
	#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
	VID_INFO*  pVid_info_front = &pStreamArgs->vid_info_front;
	#endif

	#if defined(C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION == CUSTOM_ON) && (_SENSOR_USB_MJPEG_CAM == CUSTOM_OFF)
	INT32U temp_bufe;
	#endif
	INT32U	i;

	/* param check */
	if(!preview_display_width)	preview_display_width	= 8;
	if(!preview_display_height)	preview_display_height	= 2;

	for(i = PREVIEW_SRC_FRONT; i < PREVIEW_SRC_TOTAL; ++i)
	{
		if(!preview_zoom_width[i])	preview_zoom_width[i]	= 8;
		if(!preview_zoom_height[i])	preview_zoom_height[i]	= 2;
	}

	if(preview_need_display)
	{
		if(_preview_mode != PREVIEW_MODE_STOP)
		{
			return sensor_preview_mode_set(PREVIEW_MODE_NONE, 1);
		}

		for(i = PREVIEW_SRC_FRONT; i < PREVIEW_SRC_TOTAL; ++i)
		{
            #if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
			if(i != PREVIEW_SRC_WIFI)
			#endif
			{
                lib_pscaler_stream_init(i);
            }
		}

		_preview_mode = PREVIEW_MODE_NONE;

		for(i = 0; i < preview_q_size; i++)
		{
			preview_q[i].buf_preview = 0;
			preview_q[i].st_main = ACCESS_NONE;
			preview_q[i].st_ext = ACCESS_NONE;
		}

		preview_q_tail = 0;
		preview_q_head = 0;
	}

	/*== HOST USB MJPEG CAM / INTERP2K ==*/
	#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
	if(pVid_info_front->run_2K_flow_src) // This value is Greater than 0, that means front do scale-up flow
	{// Front sensor links Pscaler_A and Pscaler_B is for interpolating 2K
		preview_pscaler_parm_set(PSCALER_A, _preview_mode, 0, preview_pscaler_isr);
		if(preview_src_path[PREVIEW_SRC_REAR] != PREVIEW_IN_PATH_NONE)
		{// Rear sensor links Scaler_0
			preview_scaler_parm_set(SCALER_0, _preview_mode, 0, preview_scaler_isr);
		}
	}
	else
	#endif
	{// Front sensor links Pscaler_A
		preview_pscaler_parm_set(PSCALER_A, _preview_mode, 0, preview_ps_isr);
		if(preview_src_path[PREVIEW_SRC_REAR] != PREVIEW_IN_PATH_NONE)
		{// Rear sensor links Pscaler_B
			preview_pscaler_parm_set(PSCALER_B, _preview_mode, 0, preview_ps_isr);
		}
	}

	#if defined(C_Y_ONLY_FUNCTION) && C_Y_ONLY_FUNCTION == CUSTOM_ON
	if(y_only_src_path != Y_ONLY_IN_PATH_NONE)
	{
		INT32U out_fifo_size = sensor_y_only_out_width_get() * sensor_y_only_out_height_get();
		INT32U fifo_count = Y_ONLY_BUF_COUNT;
		INT16U total_index = 1;

		if(y_only_fifo_malloc(&y_only_out_handle, out_fifo_size, fifo_count, total_index))
		{
			if(sensor_y_only_src_path_get() == Y_ONLY_IN_PATH_DRAM)
			{
				INT32U in_fifo_size = sensor_y_only_in_width_get() * sensor_y_only_in_height_get();
				if(sensor_y_only_src_format_get() == PIPELINE_SCALER_INPUT_FORMAT_GP420)
				{
					in_fifo_size = in_fifo_size * 3 / 2;
				}
				if(!y_only_fifo_malloc(&y_only_in_handle, in_fifo_size, fifo_count, total_index))
				{
					y_only_fifo_free(&y_only_out_handle);
				}
				else
				{
					y_only_pscaler_parm_set(
						y_only_fifo_get_read_address(&y_only_in_handle, NULL),
						y_only_fifo_get_write_address(&y_only_out_handle),
						DUMMY_BUFFER_ADDRS,
						y_only_ps_isr
					);
				}
			}
			else
			{
				//DBG_PRINT("Y only out fifo size:%d, fifo count:%d, total size:%d\r\n", fifo_size, fifo_count, fifo_size * fifo_count);
				y_only_pscaler_parm_set(
					DUMMY_BUFFER_ADDRS,
					DUMMY_BUFFER_ADDRS,
					DUMMY_BUFFER_ADDRS,
					y_only_ps_isr
				);
			}
		}
	}
	#endif

	#if defined(C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION == CUSTOM_ON)
	bsd_q_reset();
	BSD_y_only_pscaler_parm_set(
	    DUMMY_BUFFER_ADDRS,
		DUMMY_BUFFER_ADDRS,
		DUMMY_BUFFER_ADDRS,
		PScaler_BSD_Gray_Callback
	);
	#endif

	if(preview_need_display)
	{
		return sensor_preview_mode_set(PREVIEW_MODE_NONE, 1);
	}
	else /* keep "_preview_mode = PREVIEW_MODE_STOP" if skip display function */
	{
		return STATUS_OK;
	}
}

INT32S
sensor_preview_stop(
	void
)
{
	INT32U time_out_cnt = 0;
	INT8U	src;
	INT32S	rc = STATUS_OK;
	INT32U	ps;

	switch(_preview_mode)
	{
		case PREVIEW_MODE_STOP_WAIT:
		case PREVIEW_MODE_STOP:
			break;
		default:

			sensor_preview_mode_set(PREVIEW_MODE_STOP_WAIT, 1);

			for(ps = PSCALER_A ; ps < PSCALER_NUM; ++ps)
			{
				if(pscaler_mode[ps] == PSCALER_MODE_PIPE_BYPASS)
				{
					PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl;
					ENC_STREAM_INFO_T* pEncStramInfo;

					src = preview_ps_to_src(ps);
                    pPscalerStreamCtrl = &pscaler_stream[src];
                    pEncStramInfo = &pPscalerStreamCtrl->enc_stream_info;

					switch(pEncStramInfo->stream_enable)
					{
						case VIDEO_RECORD_START:
						case VIDEO_RECORD_RECING:
							pEncStramInfo->stream_enable = VIDEO_RECORD_STOP;
						break;
					}

					while(pEncStramInfo->stream_enable != VIDEO_RECORD_NOTHING)
					{
						//DBG_PRINT("[TRACE]W3(%d)..\r\n", ps);
						++time_out_cnt;
						if(time_out_cnt > 100)
						{
							pEncStramInfo->stream_enable = VIDEO_RECORD_NOTHING;
							break;
						}
						osDelay(10);
					}
				}
			}

			for(ps = PSCALER_A; ps < PSCALER_NUM; ++ps)
			{
				preview_switch_init[ps] = 0;
				pscaler_mode[ps] = PSCALER_MODE_STOP;
				drv_l1_pscaler_stop(ps);

				#if defined(C_Y_ONLY_FUNCTION) && C_Y_ONLY_FUNCTION == CUSTOM_ON
				if(ps == PSCALER_Y)
				{
					y_only_fifo_clean(&y_only_in_handle);
					y_only_fifo_clean(&y_only_out_handle);
				}
				#endif

				pscaler_busy[ps] = 0;
			}
			break;

	}

	switch(_preview_mode)
	{
		case PREVIEW_MODE_STOP_WAIT:
			_preview_mode = PREVIEW_MODE_STOP;
		break;
		default:
		break;
	}
	//DBG_PRINT("[DEBUG]sensor_preview_stop(%d)\r\n", _preview_mode);
	return rc;
}

INT32S
sensor_preview_wait_stop(
	void
)
{
	sensor_preview_stop();

	#if (_SENSOR_USB_MJPEG_CAM)
	return (lib_host_2K_decode_JPEG_finish_check());
	#else
	return STATUS_OK;
	#endif
}

PREVIEW_MODE
sensor_preview_status_get(
	void
)
{
	return _preview_mode;
}

INT32S
sensor_preview_uninit(
	void
)
{
	preview_q_size = 0;

	if(preview_q)
	{
		gp_free(preview_q);
	}

	preview_q = NULL;

	#if defined(C_Y_ONLY_FUNCTION) && C_Y_ONLY_FUNCTION == CUSTOM_ON
	y_only_fifo_free(&y_only_in_handle);
	y_only_fifo_free(&y_only_out_handle);
	#endif
	return STATUS_OK;
}

INT32S
preview_q_malloc(
	INT16U buf_cnt
)
{
	if(preview_q_size || preview_q)
	{
		sensor_preview_uninit();
	}

	if(!buf_cnt)
	{
		return STATUS_FAIL;
	}

	preview_q = gp_malloc(buf_cnt * sizeof(*preview_q));

	if(!preview_q)
	{
		return STATUS_FAIL;
	}

	preview_q_size = buf_cnt;

	return STATUS_OK;
}

/**************************************************************************
 *                           V I D E O	E N C O D E                       *
 **************************************************************************/
INT8U
get_idle_jpeg(
	INT8S user_assign
)
{
/*
user_assign: default -1, assign engine to improving the performance when do scale-up
JPEG HW line buffer:
jpeg A  2048
jpeg B  1280
*/
	INT8U i;
	INT8U jpeg_assign_cnt[JPEG_IDX_MAX];


	osSuspend();

	if(user_assign > -1)
	{
		i = user_assign;
		jpeg_engine_assign[i] += 1;
	}
	else
	{
		for(i=0; i<JPEG_IDX_MAX; i++)
		{
			jpeg_assign_cnt[i] = jpeg_engine_assign[i];

			// Look for an idle engine
			if(jpeg_assign_cnt[i] == 0)
			{
				jpeg_engine_assign[i] += 1;
				break;
			}
		}

		// Not found, return less count
		if(i == JPEG_IDX_MAX)
		{
			if(jpeg_assign_cnt[JPEGA] > jpeg_assign_cnt[JPEGB])
			{
				i = JPEGB;
			}
			else
			{
				i = JPEGA;
			}

			jpeg_engine_assign[i] += 1;
		}
	}

	osResume();

	//DBG_PRINT("==Run:%s==jpeg id:%d \r\n",__FUNCTION__,i);

	return i;
}

void
release_used_jpeg(
	JPEG_IDX jpeg_idx
)
{
	//DBG_PRINT("==Run:%s==jpeg id:%d\r\n",__FUNCTION__,jpeg_idx);

	if(jpeg_idx >= JPEG_IDX_MAX)
	{
		DBG_PRINT("%d over JPEG_IDX_MAX \r\n",jpeg_idx);
		return;
	}

	osSuspend();

	if(jpeg_engine_assign[jpeg_idx] > 0)
	{
		jpeg_engine_assign[jpeg_idx] -= 1;
	}

	osResume();
}

static unsigned int
lib_pscaler_get_bypass(
	INT32U src
)
{
	int nRet;
	unsigned int data = DUMMY_BUFFER_ADDRS;

	nRet = circular_buf_get(cbuf_pscaler_bypass_out[src], &data);
	if(nRet < 0)
	{
		return DUMMY_BUFFER_ADDRS;
	}


	//DBG_PRINT("get buf:0x%x\r\n",data);

	return data;
}

static void
lib_pscaler_free_bypass(
	INT32U src,
	unsigned int buf
)
{
	int nRet;

	//DBG_PRINT("[%d]free buf:0x%x\r\n",src,buf);
	nRet = circular_buf_try_put(cbuf_pscaler_bypass_out[src],buf);
	if(nRet < 0)
	{
		ERR_PRINT("fail to run circular_buf_try_put[%d]\r\n",src);
	}
}

INT32S
lib_pscaler_stop_check(
	INT32U src,
	INT32U* pDevCnt
)
{
	PSCALER_STREAM_CTRL_T* pPsaclerFront = &pscaler_stream[PREVIEW_SRC_FRONT];
	ENC_STREAM_INFO_T *enc_stream_info_front = &pPsaclerFront->enc_stream_info;
	//VID_INFO* pVid_info_front = pPsaclerFront->pVid_info;

	PSCALER_STREAM_CTRL_T* pPsaclerRear = &pscaler_stream[PREVIEW_SRC_REAR];
	ENC_STREAM_INFO_T *enc_stream_info_rear = &pPsaclerRear->enc_stream_info;
	VID_INFO* pVid_info_rear = pPsaclerRear->pVid_info;

	switch(src)
	{
		case PREVIEW_SRC_FRONT:
			ENCODE_STATUS_SET_BUSY(enc_stream_info_front->stream_status,VIDEO_ENCODE_STATUS_ENCODE_STOP_OFFSET);
		break;

		case PREVIEW_SRC_REAR:
			ENCODE_STATUS_SET_BUSY(enc_stream_info_rear->stream_status,VIDEO_ENCODE_STATUS_ENCODE_STOP_OFFSET);
		break;
	}

	/*
		USB_HOST: Skip VIDEO_RECORD_STOP if it does not start recording
	*/
	if(back_sensor_plug_status_get() &&
	   pVid_info_rear
		#if (_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K)
	   && (enc_stream_info_rear->stream_enable & VIDEO_RECORD_START)
	   #endif/*== HOST USB MJPEG CAM ==*/
	   )
	{
		if(ENCODE_STATUS_IS_BUSY(enc_stream_info_front->stream_status,VIDEO_ENCODE_STATUS_ENCODE_STOP_OFFSET) &&
			ENCODE_STATUS_IS_BUSY(enc_stream_info_rear->stream_status,VIDEO_ENCODE_STATUS_ENCODE_STOP_OFFSET))
		{
			if(pDevCnt)
			{
				*pDevCnt = 2;
			}
			return STATUS_OK;
		}
	}
	else
	{
		if(ENCODE_STATUS_IS_BUSY(enc_stream_info_front->stream_status,VIDEO_ENCODE_STATUS_ENCODE_STOP_OFFSET))
		{
			if(pDevCnt)
			{
				*pDevCnt = 1;
			}
			return STATUS_OK;
		}
	}

	return STATUS_FAIL;
}

static void
lib_pscaler_notify_bypass(
	unsigned char src,
	void* p,
	INT32U data
)
{
	PSCALER_STREAM_CTRL_T *stream_ctrl = (PSCALER_STREAM_CTRL_T*) p;
	FB_CFG* pPscalerOutCfg = stream_ctrl->ps_fb_cfg;
	FB_CFG* pMsgNotifyCfg = stream_ctrl->msg_fb_cfg;
	VID_INFO* pVid_info = stream_ctrl->pVid_info;
	FB_INFO* pfb_info = ((FB_INFO*)(pMsgNotifyCfg->fb_cfg_head_addr))+pMsgNotifyCfg->fb_cfg_count_num;

	pfb_info->buf = data;
	pfb_info->idx = pPscalerOutCfg->fb_cfg_count_num;
	pfb_info->ps_buf_time = xTaskGetTickCountFromISR();

	if(pVid_info->vid_rec_line)
	{// FIFO mode
		if(pVid_info->vid_fifo_drop)
		{// Clear vid_fifo_drop at the start of next frame or video stop
			if((pfb_info->idx == 1) || (pfb_info->idx == DUMMY_BUFFER_ADDRS))
			{
				pVid_info->vid_fifo_drop = 0;
			}
			else
			{
				goto Return;
			}
		}

		if((pfb_info->buf == DUMMY_BUFFER_ADDRS) && (pfb_info->idx != DUMMY_BUFFER_ADDRS)) // buf=DUMMY + idx=DUMMY is for stopping video record
		{// Pass this dummy data to stop H264 encode, drop the rest of fifo buffer
			pVid_info->vid_fifo_drop = 1;
			if(pfb_info->idx == 1)
			{// The dummy data at the first index, drop fifo buffer until the start of next frame
				goto Return;
			}
		}
	}

	if(circular_buf_try_put(cbuf_notify_enc[src],((INT32U)pfb_info)) < 0)
	{
		ERR_PRINT("fail to put cbuf_notify_enc[%d]\r\n",src);
		goto    Return;
	}

	pMsgNotifyCfg->fb_cfg_count_num++;
	if(pMsgNotifyCfg->fb_cfg_count_num >= pMsgNotifyCfg->fb_cfg_blocks)
	{
		pMsgNotifyCfg->fb_cfg_count_num = 0;
	}

	/*
		STATE_VIDEO_RECORD: Send MSG_VID_PSCALER_NOTIFY at once!
		STATE_PHOTO_CAPTURE: Wait for pscaler done to send MSG_VID_PSCALER_NOTIFY
	*/
	if((pVid_info->run_ap_mode == STATE_VIDEO_RECORD) ||
		(pVid_info->run_ap_mode == STATE_WIFI_TRANSMISSION) ||
		(pVid_info->run_ap_mode == STATE_CONNECT_TO_PC)
	)
	{
		if(msgQSend(stream_ctrl->ps_notify_q, (MSG_VID_PSCALER_NOTIFY|src), NULL, 0, MSG_PRI_NORMAL) < 0)
		{
			ERR_PRINT("fail to msgQSendFromISR [%d]\r\n",src);
			goto Return;
		}
	}

	return;

Return:
	// Put pscaler bypass output frame/fifo buffer(Record mode) or display buffer(WIFI mode) to circular buf
	if(data != DUMMY_BUFFER_ADDRS)
	{
		#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
		if((pVid_info->run_2K_flow_src & HOST_2K_IS_FRONT) == 0) // This value is Greater than 0, that means front do scale-up flow
		#endif
		{
			stream_ctrl->ps_free_func(data);
		}
    }

	return;
}

INT32S
lib_pscaler_stream_init(
	INT32U src
)
{
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];

	//DBG_PRINT("==Run:%s==[%d]\r\n",__FUNCTION__,src);

	/*
		Don't clear "enc_stream_info" and "pVid_info" because it was assigned in lib_preview_buffer_set()
		Always need it in USB_MJPEG_CAM flow
	*/
	#if (!_SENSOR_USB_MJPEG_CAM) && (!C_VID_INTERPOLATING_2K)
	pPscalerStreamCtrl->pVid_info = 0;
	#endif

	memset(&pPscalerStreamCtrl->enc_stream_info,0, sizeof(ENC_STREAM_INFO_T));
	pPscalerStreamCtrl->enc_stream_ctrl = 0;
	pPscalerStreamCtrl->ps_fb_cfg = 0;
	pPscalerStreamCtrl->msg_fb_cfg = 0;
	pPscalerStreamCtrl->ps_notify_q = 0;

	#if (C_VID_INTERPOLATING_2K) // CSI's output buffer puts into system fifo
	if((pPscalerStreamCtrl->pVid_info->run_2K_flow_src & HOST_2K_IS_REAR) == 0)
	#endif
	{
		pPscalerStreamCtrl->flag_sys_fifo = 0;
	}

	return STATUS_OK;
}

#if C_VID_REC_USE_SYS_FIFO
INT32S
lib_encode_system_fifo_register(
	INT32U src // It's PREVIEW_SRC_XXX
)
{
	INT32U scaler = src;
	INT32U reg_src = SYS_FIFO_MAX;

	if(src <= PREVIEW_SRC_REAR)
	{
		scaler = preview_src_to_ps(src);
	}

	switch(scaler)
	{
		case PSCALER_A:
			reg_src = SYS_FIFO_2;
		break;
		case PSCALER_B:
			reg_src = SYS_FIFO_1;
		break;
		default:
			reg_src = SYS_FIFO_3;
		break;
	}
	return drv_l1_system_fifo_register(reg_src);
}

INT32S
lib_encode_system_fifo_release(
	INT32U src // It's PREVIEW_SRC_XXX
)
{
	INT32U scaler = src;
	INT32U reg_src = SYS_FIFO_MAX;

	if(src <= PREVIEW_SRC_REAR)
	{
		scaler = preview_src_to_ps(src);
	}

	switch(scaler)
	{
		case PSCALER_A:
			reg_src = SYS_FIFO_2;
		break;
		case PSCALER_B:
			reg_src = SYS_FIFO_1;
		break;
		default:
			reg_src = SYS_FIFO_3;
		break;
	}
	return drv_l1_system_fifo_release(reg_src);
}

INT32U
lib_encode_system_fifo_mapping(
	INT32U src, // It's PREVIEW_SRC_XXX
	INT32U buffer
)
{
	INT32U scaler = src;
	INT32U reg_src = SYS_FIFO_MAX;

	if(src <= PREVIEW_SRC_REAR)
	{
		scaler = preview_src_to_ps(src);
	}

	switch(scaler)
	{
		case PSCALER_A:
			reg_src = SYS_FIFO_2;
		break;
		case PSCALER_B:
			reg_src = SYS_FIFO_1;
		break;
		default:
			reg_src = SYS_FIFO_3;
		break;
	}
	return drv_l1_system_fifo_mapping(reg_src,buffer);
}

INT32U
lib_encode_system_fifo_unmapping(
	INT32U src, // It's PREVIEW_SRC_XXX
	INT32U buffer
)
{
	INT32U scaler = src;
	INT32U reg_src = SYS_FIFO_MAX;

	if(src <= PREVIEW_SRC_REAR)
	{
		scaler = preview_src_to_ps(src);
	}

	switch(scaler)
	{
		case PSCALER_A:
			reg_src = SYS_FIFO_2;
		break;
		case PSCALER_B:
			reg_src = SYS_FIFO_1;
		break;
		default:
			reg_src = SYS_FIFO_3;
		break;
	}
	return drv_l1_system_fifo_unmapping(reg_src,buffer);
}
#endif

INT32S
lib_encode_buf_release(
	INT32U src
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	INT8U data_source = VID_SRC_NONE;

	if(pVid_info)
	{
		data_source = pVid_info->data_source;
	}

	//DBG_PRINT("data_source:%d\r\n",data_source);

	if((data_source != VID_SRC_USB)
		#if (C_VID_INTERPOLATING_2K)
		&& !(pVid_info->run_2K_flow_src & HOST_2K_IS_REAR)
		#endif
	)
	{
		if(cbuf_pscaler_bypass_out[src])
		{
			circular_buf_free(cbuf_pscaler_bypass_out[src]);
			cbuf_pscaler_bypass_out[src] = 0;
		}
	}

	if(cbuf_encoder_out[src][ENC_H264I_JPEG_IDX])
	{
		circular_buf_free(cbuf_encoder_out[src][ENC_H264I_JPEG_IDX]);
		cbuf_encoder_out[src][ENC_H264I_JPEG_IDX] = 0;
		no_encode_out_cnt[src][ENC_H264I_JPEG_IDX] = 0;
	}

	if(cbuf_encoder_out[src][ENC_H264P_IDX])
	{
		circular_buf_free(cbuf_encoder_out[src][ENC_H264P_IDX]);
		cbuf_encoder_out[src][ENC_H264P_IDX] = 0;
		no_encode_out_cnt[src][ENC_H264P_IDX] = 0;
	}

	if(cbuf_encoder_out[src][ENC_H264SKIP_IDX])
	{
		circular_buf_free(cbuf_encoder_out[src][ENC_H264SKIP_IDX]);
		cbuf_encoder_out[src][ENC_H264SKIP_IDX] = 0;
		no_encode_out_cnt[src][ENC_H264SKIP_IDX] = 0;
	}

	if(cbuf_notify_enc[src])
	{
		circular_buf_free(cbuf_notify_enc[src]);
		cbuf_notify_enc[src] = 0;
	}

	#if C_VID_REC_USE_SYS_FIFO
	if((data_source != VID_SRC_USB) && (pPscalerStreamCtrl->flag_sys_fifo)
		#if (C_VID_INTERPOLATING_2K)
		&& !(pVid_info->run_2K_flow_src & HOST_2K_IS_REAR)
		&& !(pVid_info->run_2K_flow_src & HOST_2K_IS_FRONT)
		#endif
	)
    {
        lib_encode_system_fifo_release(src);
        pPscalerStreamCtrl->flag_sys_fifo = 0;
    }
	#endif

	#if (C_VID_INTERPOLATING_2K)
		#if C_VID_REC_USE_SYS_FIFO
		if(pVid_info->run_2K_flow_src & HOST_2K_IS_FRONT)
		{
        	lib_encode_system_fifo_release(src);
		}
		#endif

		if(cbuf_interp2K_bypass_out[src])
		{
			circular_buf_free(cbuf_interp2K_bypass_out[src]);
			cbuf_interp2K_bypass_out[src] = 0;
		}

		if(cbuf_notify_interp2K[src])
		{
			circular_buf_free(cbuf_notify_interp2K[src]);
			cbuf_notify_interp2K[src] = 0;
		}
	#endif

	/*
	 Reset to default
	*/
	lib_pscaler_stream_init(src);

	return nRet;
}

INT32S
lib_encode_buf_set(
	INT32U src,// It's PREVIEW_SRC_XXX
	MSG_Q_ID notifyQ,
	VID_INFO* args
)
{
	INT32S nRet = STATUS_OK;
	FB_CFG* pPscalerOutCfg;
	FB_CFG* pEncOutCfg = NULL;
	FB_CFG* pH264OutCfg_I = NULL;
	FB_CFG* pH264OutCfg_P = NULL;
	FB_CFG* pH264OutCfg_SKIP = NULL;
	FB_CFG* pMsgNotifyCfg;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	int i,j;
	/*
	16 alignment for hw constrain
	*/
	INT32U  h264_height_16_alignment = 0;
	INT32U  h264_extra_height = 0;
	INT32U  h264_extra_size = 0;
	INT32U  h264_original_size = 0;

	#if (C_VID_INTERPOLATING_2K)
	FB_CFG* pInterp2KPscalerOutCfg = NULL;
	FB_CFG* pInterp2KMsgNotifyCfg = NULL;
	#endif

	//DBG_PRINT("==Run:%s==[%d]\r\n",__FUNCTION__,src);

	#if C_VID_REC_USE_SYS_FIFO
	if((args->data_source != VID_SRC_USB)
		#if (C_VID_INTERPOLATING_2K)
		&& !(args->run_2K_flow_src & HOST_2K_IS_REAR)
		&& !(args->run_2K_flow_src & HOST_2K_IS_FRONT)
		#endif
	)
	{
		if(lib_encode_system_fifo_register(src) == STATUS_OK)
		{
			pPscalerStreamCtrl->flag_sys_fifo = 1;
		}
	}
	#endif

	switch(args->vid_rec_type)
	{
		case VID_REC_H264:
			#if (C_VID_INTERPOLATING_2K)
			pInterp2KPscalerOutCfg = &args->h264_rec_args.fb_interp2K_ps_out_cfg;
			pInterp2KMsgNotifyCfg = &args->h264_rec_args.fb_interp2K_msg_notify_cfg;
			#endif

			pPscalerOutCfg = &args->h264_rec_args.fb_pscaler_out_cfg;
			pH264OutCfg_I = &args->h264_rec_args.fb_h264_out_I_cfg;
			pH264OutCfg_P = &args->h264_rec_args.fb_h264_out_P_cfg;
			pH264OutCfg_SKIP = &args->h264_rec_args.fb_h264_out_SKIP_cfg;
			pMsgNotifyCfg = &args->h264_rec_args.fb_msg_notify_cfg;
			pPscalerStreamCtrl->enc_stream_ctrl = &enc_stream[ENC_STREAM_RECORD_H264];
			/*
			16 alignment for hw constrain
			*/
			h264_height_16_alignment = (args->target_height + 0xF) >> 4 << 4;
			h264_extra_height = (h264_height_16_alignment-args->target_height);
			h264_extra_size = ((args->target_width*h264_extra_height*3) >> 1);

			if(args->vid_rec_line)
			{// FIFO MODE
				h264_original_size = ((args->target_width*(args->vid_rec_line-h264_extra_height)*3) >> 1);
			}
			else
			{// FRAME MOCDE
				h264_original_size = ((args->target_width*args->target_height*3) >> 1);
			}
		break;
		case VID_REC_JPEG:
			pPscalerOutCfg = &args->jpeg_rec_args.fb_pscaler_out_cfg;
			pEncOutCfg = &args->jpeg_rec_args.fb_jpeg_out_cfg;
			pMsgNotifyCfg = &args->jpeg_rec_args.fb_msg_notify_cfg;
			if((args->run_ap_mode == STATE_VIDEO_RECORD) ||
			   (args->run_ap_mode == STATE_WIFI_TRANSMISSION) ||
			   (args->run_ap_mode == STATE_CONNECT_TO_PC)
			)
			{
				pPscalerStreamCtrl->enc_stream_ctrl = &enc_stream[ENC_STREAM_RECORD_JPEG];
			}
			else
			{
				pPscalerStreamCtrl->enc_stream_ctrl = &enc_stream[ENC_STREAM_PHOTO_JPEG];
			}
		break;
		default:
            RETURN(STATUS_FAIL);
		break;
	}

	// Init parameters
	pPscalerStreamCtrl->enc_stream_info.stream_status = 0;
	pPscalerStreamCtrl->enc_stream_info.stream_enable = 0;

	// Set parameters
	pPscalerStreamCtrl->pVid_info = args;
	pPscalerStreamCtrl->ps_notify_q = notifyQ;
	pPscalerStreamCtrl->ps_fb_cfg = pPscalerOutCfg;
	pPscalerStreamCtrl->msg_fb_cfg = pMsgNotifyCfg;

	#if (C_VID_INTERPOLATING_2K)
	pPscalerStreamCtrl->interp2K_ps_fb_cfg = pInterp2KPscalerOutCfg;
	pPscalerStreamCtrl->interp2K_msg_fb_cfg = pInterp2KMsgNotifyCfg;
	#endif

	// Init pscaler bypass out
	if((args->data_source != VID_SRC_USB)
		#if (C_VID_INTERPOLATING_2K)
		&& !(args->run_2K_flow_src & HOST_2K_IS_REAR)
		#endif
	)
	{
		if(pPscalerOutCfg->fb_cfg_blocks)
		{
			cbuf_pscaler_bypass_out[src] = circular_buf_init(pPscalerOutCfg->fb_cfg_cb_addr,pPscalerOutCfg->fb_cfg_blocks);
			if(cbuf_pscaler_bypass_out[src] == 0)
			{
				ERR_PRINT("fail to cbuf_pscaler_bypass_out[%d]\r\n",src);
				RETURN(STATUS_FAIL);
			}

			#if C_VID_REC_USE_SYS_FIFO
			if(pPscalerStreamCtrl->flag_sys_fifo)
			{
				pPscalerOutCfg->fb_cfg_head_addr = (void *)lib_encode_system_fifo_mapping(src, (INT32U)pPscalerOutCfg->fb_cfg_head_addr);
			}
			//DBG_PRINT("Pscaler bypass sys fifo:0x%x\r\n",pPscalerOutCfg->fb_cfg_head_addr);
			#endif

			for(i = 0; i < pPscalerOutCfg->fb_cfg_blocks; i++)
			{
				#if 0
				DBG_PRINT("[pscaler:%d]i:%d put 0x%x\r\n",src,i,pPscalerOutCfg->fb_cfg_head_addr+(pPscalerOutCfg->fb_cfg_size*i));
				#endif

				/*
				16 alignment for hw constrain
				*/
				if(h264_extra_height)
				{
					#if 0
	                DBG_PRINT("[extera:%d]i:%d put 0x%x\r\n",src,i,pPscalerOutCfg->fb_cfg_head_addr+h264_original_size+(pPscalerOutCfg->fb_cfg_size*i));
					#endif

	                #if C_VID_REC_USE_SYS_FIFO
					gp_memset((INT8S *)lib_encode_system_fifo_unmapping(src, (INT32U)pPscalerOutCfg->fb_cfg_head_addr+h264_original_size+(pPscalerOutCfg->fb_cfg_size*i)),0,h264_extra_size);
					cache_drain_range(lib_encode_system_fifo_unmapping(src, (INT32U)pPscalerOutCfg->fb_cfg_head_addr+h264_original_size+(pPscalerOutCfg->fb_cfg_size*i)),h264_extra_size);
					#else
					gp_memset((INT8S *)(INT32U)pPscalerOutCfg->fb_cfg_head_addr+h264_original_size+(pPscalerOutCfg->fb_cfg_size*i),0,h264_extra_size);
					cache_drain_range((INT8S *)(INT32U)pPscalerOutCfg->fb_cfg_head_addr+h264_original_size+(pPscalerOutCfg->fb_cfg_size*i),h264_extra_size);
					#endif
				}

				if(circular_buf_try_put(cbuf_pscaler_bypass_out[src], (unsigned int)(pPscalerOutCfg->fb_cfg_head_addr+(pPscalerOutCfg->fb_cfg_size*i))) < 0)
				{
					ERR_PRINT("fail to put cbuf_pscaler_bypass_out[%d]\r\n",src);
					RETURN(STATUS_FAIL);
				}
			}
		}
	}

	if(args->vid_rec_type == VID_REC_H264)
	{
		for(i = 0; i < 2; i++)
		{
			if((i % 2) == 0)
			{
				pEncOutCfg = pH264OutCfg_I;
			}
			else
			{
				pEncOutCfg = pH264OutCfg_P;
			}

			// Init h264 I/P encode out
			cbuf_encoder_out[src][i] = circular_buf_init(pEncOutCfg->fb_cfg_cb_addr,pEncOutCfg->fb_cfg_blocks);
			if(cbuf_encoder_out[src][i] == 0)
			{
				ERR_PRINT("fail to cbuf_encoder_out[%d]\r\n",src);
				RETURN(STATUS_FAIL);
			}
			for(j = 0; j < pEncOutCfg->fb_cfg_blocks; j++)
			{
				#if 0
				DBG_PRINT("[h264:%d]i:%d put 0x%x\r\n",src,j,pEncOutCfg->fb_cfg_head_addr+(pEncOutCfg->fb_cfg_size*j));
				#endif

				if(circular_buf_try_put(cbuf_encoder_out[src][i], (unsigned int)(pEncOutCfg->fb_cfg_head_addr+(pEncOutCfg->fb_cfg_size*j))) < 0)
				{
					ERR_PRINT("fail to put cbuf_encoder_out[%d:%d]\r\n",src,i);
					RETURN(STATUS_FAIL);
				}
			}
		}

		// skip frame
		{
		  pEncOutCfg = pH264OutCfg_SKIP;
		  cbuf_encoder_out[src][ENC_H264SKIP_IDX] = circular_buf_init(pEncOutCfg->fb_cfg_cb_addr,pEncOutCfg->fb_cfg_blocks);
		  if(cbuf_encoder_out[src][ENC_H264SKIP_IDX] == 0)
          {
				ERR_PRINT("fail to cbuf_encoder_out[%d]\r\n",src);
				RETURN(STATUS_FAIL);
          }
          for(j = 0; j < pEncOutCfg->fb_cfg_blocks; j++)
          {
            if(circular_buf_try_put(cbuf_encoder_out[src][ENC_H264SKIP_IDX], (unsigned int)(pEncOutCfg->fb_cfg_head_addr+(pEncOutCfg->fb_cfg_size*j))) < 0)
            {
					ERR_PRINT("fail to put cbuf_encoder_out[%d:%d]\r\n",src,i);
					RETURN(STATUS_FAIL);
            }

           // DBG_PRINT("[0x%x]",(pEncOutCfg->fb_cfg_head_addr+(pEncOutCfg->fb_cfg_size*j)));

          }
		}
	}

	if(args->vid_rec_type == VID_REC_JPEG)
	{
		cbuf_encoder_out[src][ENC_H264P_IDX] = 0;
		cbuf_encoder_out[src][ENC_H264SKIP_IDX] = 0;

		// Init jpeg encode out
		cbuf_encoder_out[src][ENC_H264I_JPEG_IDX] = circular_buf_init(pEncOutCfg->fb_cfg_cb_addr,pEncOutCfg->fb_cfg_blocks);
		if(cbuf_encoder_out[src][ENC_H264I_JPEG_IDX] == 0)
		{
			ERR_PRINT("fail to cbuf_encoder_out[%d]\r\n",src);
			RETURN(STATUS_FAIL);
		}
		for(i = 0; i < pEncOutCfg->fb_cfg_blocks; i++)
		{
			#if 0
			DBG_PRINT("[jpeg:%d]i:%d put 0x%x\r\n",src,i,pEncOutCfg->fb_cfg_head_addr+(pEncOutCfg->fb_cfg_size*i));
			#endif

			if(circular_buf_try_put(cbuf_encoder_out[src][ENC_H264I_JPEG_IDX], (unsigned int)(pEncOutCfg->fb_cfg_head_addr+(pEncOutCfg->fb_cfg_size*i))) < 0)
			{
				ERR_PRINT("fail to put cbuf_encoder_out[%d]\r\n",src);
				RETURN(STATUS_FAIL);
			}
		}
	}

	// Init notify queue
	cbuf_notify_enc[src] = circular_buf_init(pMsgNotifyCfg->fb_cfg_cb_addr,pMsgNotifyCfg->fb_cfg_blocks);
	if(cbuf_notify_enc[src] == 0)
	{
		ERR_PRINT("fail to cbuf_notify_enc[%d]\r\n",src);
		RETURN(STATUS_FAIL);
	}

	#if 0
	DBG_PRINT("0x%x\r\n",src,cbuf_pscaler_bypass_out[src]);
	DBG_PRINT("0x%x\r\n",cbuf_encoder_out[src][ENC_H264I_JPEG_IDX]);
	DBG_PRINT("0x%x\r\n",cbuf_encoder_out[src][ENC_H264P_IDX]);
	#endif

	#if (C_VID_INTERPOLATING_2K)
	if(pInterp2KPscalerOutCfg && pInterp2KPscalerOutCfg->fb_cfg_blocks)
	{// It's for front sensor (Pscaler_A bypass)

		#if C_VID_REC_USE_SYS_FIFO
		if(lib_encode_system_fifo_register(src) == STATUS_OK)
		{
			pInterp2KPscalerOutCfg->fb_cfg_head_addr = (void *)lib_encode_system_fifo_mapping(src, (INT32U)pInterp2KPscalerOutCfg->fb_cfg_head_addr);
		}

		//DBG_PRINT("Inter2K bypass :0x%x\r\n",pInterp2KPscalerOutCfg->fb_cfg_head_addr);
		#endif

		cbuf_interp2K_bypass_out[src] = circular_buf_init(pInterp2KPscalerOutCfg->fb_cfg_cb_addr,pInterp2KPscalerOutCfg->fb_cfg_blocks);
		if(cbuf_interp2K_bypass_out[src] == 0)
		{
			ERR_PRINT("fail to cbuf_interp2K_bypass_out[%d]\r\n",src);
			RETURN(STATUS_FAIL);
		}

		for(i = 0; i < pInterp2KPscalerOutCfg->fb_cfg_blocks; i++)
		{
			#if 0
			DBG_PRINT("[interp2K:%d]i:%d put 0x%x\r\n",src,i,pInterp2KPscalerOutCfg->fb_cfg_head_addr+(pInterp2KPscalerOutCfg->fb_cfg_size*i));
			#endif

			/*
			16 alignment for hw constrain
			*/
			if(h264_extra_height)
			{
				#if 0
                DBG_PRINT("[extera:%d]i:%d put 0x%x\r\n",src,i,pPscalerOutCfg->fb_cfg_head_addr+h264_original_size+(pPscalerOutCfg->fb_cfg_size*i));
				#endif

                #if C_VID_REC_USE_SYS_FIFO
				gp_memset((INT8S *)lib_encode_system_fifo_unmapping(src, (INT32U)pInterp2KPscalerOutCfg->fb_cfg_head_addr+h264_original_size+(pInterp2KPscalerOutCfg->fb_cfg_size*i)),0,h264_extra_size);
				#else
				gp_memset((INT8S *)(INT32U)pInterp2KPscalerOutCfg->fb_cfg_head_addr+h264_original_size+(pInterp2KPscalerOutCfg->fb_cfg_size*i), 0,h264_extra_size);
				#endif
			}

			if(circular_buf_try_put(cbuf_interp2K_bypass_out[src], (unsigned int)(pInterp2KPscalerOutCfg->fb_cfg_head_addr+(pInterp2KPscalerOutCfg->fb_cfg_size*i))) < 0)
			{
				ERR_PRINT("fail to put cbuf_interp2K_bypass_out[%d]\r\n",src);
				RETURN(STATUS_FAIL);
			}
		}
	}

	// Init notify queue
	if(pInterp2KMsgNotifyCfg)
	{
		cbuf_notify_interp2K[src] = circular_buf_init(pInterp2KMsgNotifyCfg->fb_cfg_cb_addr,pInterp2KMsgNotifyCfg->fb_cfg_blocks);
		if(cbuf_notify_interp2K[src] == 0)
		{
			ERR_PRINT("fail to cbuf_notify_interp2K[%d]\r\n",src);
			RETURN(STATUS_FAIL);
		}
	}
	#endif

	return STATUS_OK;

Return:

	lib_encode_buf_release(src);

	return nRet;
}

INT32S
lib_encode_start(
	INT32U src,
	VID_INFO* args
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];

	if(pPscalerStreamCtrl->enc_stream_info.stream_enable == VIDEO_RECORD_NOTHING)
	{
		if((args->run_ap_mode == STATE_VIDEO_RECORD) ||
			(args->run_ap_mode == STATE_WIFI_TRANSMISSION) ||
			(args->run_ap_mode == STATE_CONNECT_TO_PC)
		)
		{
			pPscalerStreamCtrl->enc_stream_info.stream_enable = VIDEO_RECORD_START;
		}
		else if(args->run_ap_mode == STATE_PHOTO_CAPTURE)
		{
			pPscalerStreamCtrl->enc_stream_info.stream_enable = VIDEO_RECORD_PHOTO_START;
		}
		#if C_SENSOR_SAVE_RAW_DATA
		else if(args->run_ap_mode == STATE_SENSOR_SAVE_RAW)
		{
			if(msgQSend(pPscalerStreamCtrl->ps_notify_q, MSG_SENSOR_SAVE_RAW_DATA, NULL, 0, MSG_PRI_URGENT) < 0)
			{
				ERR_PRINT("fail to send MSG_SENSOR_SAVE_RAW_DATA \r\n");
			}
		}
		#endif
	}

	//DBG_PRINT("==Run:%s==[%d]=0x%x\r\n",__FUNCTION__,src,pPscalerStreamCtrl->enc_stream_info.stream_enable);
	return nRet;
}

INT32S
lib_encode_stop(
	INT32U src
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;

	//DBG_PRINT("==Run:%s==[%d]=0x%x\r\n",__FUNCTION__,src,pVid_info);

	if((pVid_info->run_ap_mode == STATE_VIDEO_RECORD) ||
		(pVid_info->run_ap_mode == STATE_WIFI_TRANSMISSION) ||
		(pVid_info->run_ap_mode == STATE_CONNECT_TO_PC)
	)
	{
		/*
			Skip VIDEO_RECORD_STOP if it does not start recording
		*/
		if(pPscalerStreamCtrl->enc_stream_info.stream_enable & (VIDEO_RECORD_START|VIDEO_RECORD_RECING))
		{
			pPscalerStreamCtrl->enc_stream_info.stream_enable = VIDEO_RECORD_STOP;
		}
	}

	//DBG_PRINT("==Run:%s==[%d]=0x%x\r\n",__FUNCTION__,src,pPscalerStreamCtrl->enc_stream_info.stream_enable);

	return nRet;
}

static INT32S
lib_encode_engine_path_ctrl(
	INT8U src,
	VIDEO_REC_TYPE		rec_type,
	ENC_ENGINE_ACTION	act_ctrl
)
{
	PSCALER_STREAM_CTRL_T* pPsaclerStream = NULL;
	VID_INFO* pVid_info;
	ENC_STREAM_INFO_T *enc_stream_info = NULL;
	JpegEnc_t* pJpegEnc;
	INT32U cb_content_size[PREVIEW_SRC_TOTAL];
	INT32U max_size, size_tmp, max_cnt;
	INT32S src_idx = -1;
	INT32S JPEG_id = -1;
	INT8U i;
	INT8U H264_path_scores = 0;
	INT8U JPEGA_path_scores = 0;
	INT8U JPEGB_path_scores = 0;
	#if (_SENSOR_USB_MJPEG_CAM)
	JpegEnc_t* pMjpegDec;
	#endif
	#if (C_SUPORT_GET_JPG_BUF)
	JpegEnc_t* pGetJpegEnc;
	#endif
	/*
		Chip has one H264 engine and two JPEG engine
	*/
	for(i=PREVIEW_SRC_FRONT; i<PREVIEW_SRC_TOTAL; i++)
	{
		pPsaclerStream = &pscaler_stream[i];
		pVid_info = pPsaclerStream->pVid_info;
		enc_stream_info =  &pPsaclerStream->enc_stream_info;

		cb_content_size[i] = 0;

		//DBG_PRINT("[%d]pVid_info:0x%x\r\n",i,pVid_info);

		if(!pVid_info) // Source path does not exists
		{
			continue;
		}

		//DBG_PRINT("[%d]0x%x,0x%x,0x%x\r\n",i,rec_type,pVid_info->vid_rec_type,enc_stream_info->stream_status);

		// Encode H264
		if((VID_REC_H264 == rec_type) && (pVid_info->vid_rec_type == rec_type))
		{// The function caller wants to encode H264 and this channel match it
			if(act_ctrl == ENC_ENGINE_ACT_CHECK)
			{// Check how many source path encoded H264
				if(ENCODE_STATUS_IS_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_ENCODE_H264_OFFSET))
				{// Engise is busy
					++H264_path_scores;
				}
				else
				{// Engise is idle
					if(pVid_info->vid_rec_line)
					{// FIFO MODE
						if(pVid_info->vid_fifo_start)
						{// Encoding fifo buffer on this channel
							if(src == i)
							{// Input fifo buffer is this channel, Yes, return [engine is idle]
								H264_path_scores = 0;
								break;
							}
							else
							{// No, return [engine is busy]
								++H264_path_scores;
								break;
							}
						}
					}
				}
			}
			else if(act_ctrl == ENC_ENGINE_ACT_SET)
			{// FRAME/FIFO MODE set
				ENCODE_STATUS_SET_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_ENCODE_H264_OFFSET);
			}
			else if(act_ctrl == ENC_ENGINE_ACT_CLEAR)
			{// FRAME/FIFO MODE clear all channel
				ENCODE_STATUS_CLR_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_ENCODE_H264_OFFSET);

				if(pVid_info->vid_rec_line)
				{// FIFO MODE
					if(pVid_info->vid_fifo_start)
					{ // Encoding fifo buffer on this channel but not yet finished, encode the next fifo buffer now
						src_idx = i;
					}
				}

				osENTER_CRITICAL();
				if(pVid_info->vid_rec_line)
				{// FIFO MODE
					max_cnt = pPsaclerStream->ps_fb_cfg->fb_cfg_max_count;
					size_tmp = circular_buf_size(cbuf_notify_enc[i]);

					cb_content_size[i] = size_tmp/max_cnt;
					if(size_tmp != (cb_content_size[i]*max_cnt)) cb_content_size[i]++;
				}
				else
				{
					cb_content_size[i] = circular_buf_size(cbuf_notify_enc[i]);
				}
				osEXIT_CRITICAL();
			}
		}

		// Encode/Decode/Get JPEG
		if((VID_REC_JPEG == rec_type)  // Encode JPEG
			|| (VID_DEC_JPEG == rec_type) // Decode JPEG for USB HOST
			|| (VID_GET_JPEG == rec_type) // Get JPEG
		)
		{// The function caller wants to encode JPEG and this channel match it
			if(src != i)
			{
				continue;
			}

			if(VID_REC_JPEG == rec_type)
			{
				if(pVid_info->vid_rec_type != rec_type)
				{
					continue;
				}

				pJpegEnc = &pVid_info->jpeg_rec_args.jpeg_cfg;
				JPEG_id = pJpegEnc->hwidx;
			}
			else if(VID_DEC_JPEG == rec_type)
			{
				if(pVid_info->data_source != VID_SRC_USB)
				{
					continue;
				}

				#if (_SENSOR_USB_MJPEG_CAM)
				pMjpegDec = &pVid_info->host_usb_mjpeg_dec_args.jpeg_cfg;
				JPEG_id = pMjpegDec->hwidx;
				#endif
			}
			else if(VID_GET_JPEG == rec_type)
			{
				#if (C_SUPORT_GET_JPG_BUF)
				pGetJpegEnc = &pVid_info->get_jpeg_args.jpeg_cfg;
				JPEG_id = pGetJpegEnc->hwidx;
				#endif
			}

			if(JPEG_id == JPEGA)
			{
				if(act_ctrl == ENC_ENGINE_ACT_CHECK)
				{// // Check how many source path encoded JPEG by JPEGA
					if(ENCODE_STATUS_IS_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_ENCODE_JPEGA_OFFSET))
					{// Engise is busy
						++JPEGA_path_scores;
					}
					else
					{// Engise is idle
						if((VID_DEC_JPEG == rec_type) || (VID_GET_JPEG == rec_type))
						{
							JPEGA_path_scores = 0;
							break;
						}
						else
						{
							if(pVid_info->vid_rec_line)
							{// FIFO MODE
								if(pVid_info->vid_fifo_start)
								{// Encoding fifo buffer on this channel
									if(src == i)
									{// Input fifo buffer is this channel, Yes, return [engine is idle]
										JPEGA_path_scores = 0;
										break;
									}
									else
									{// No, return [engine is busy]
										++JPEGA_path_scores;
										break;
									}
								}
							}
						}
					}
				}
				else if(act_ctrl == ENC_ENGINE_ACT_SET)
				{// FRAME/FIFO MODE set
					ENCODE_STATUS_SET_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_ENCODE_JPEGA_OFFSET);
				}
				else if(act_ctrl == ENC_ENGINE_ACT_CLEAR)
				{// FRAME/FIFO MODE clear all channel
					ENCODE_STATUS_CLR_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_ENCODE_JPEGA_OFFSET);

					if(VID_REC_JPEG == rec_type)
					{
						if(pVid_info->vid_rec_line)
						{// FIFO MODE
							if(pVid_info->vid_fifo_start)
							{ // Encoding fifo buffer on this channel but not yet finished, encode the next fifo buffer now
								src_idx = i;
							}
						}

						osENTER_CRITICAL();
						cb_content_size[i] = circular_buf_size(cbuf_notify_enc[i]);
						osEXIT_CRITICAL();
					}
				}
			}

			if(JPEG_id == JPEGB)
			{
				if(act_ctrl == ENC_ENGINE_ACT_CHECK)
				{// // Check how many source path encoded JPEG by JPEGB
					if(ENCODE_STATUS_IS_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_ENCODE_JPEGB_OFFSET))
					{// Engise is busy
						++JPEGB_path_scores;
					}
					else
					{// Engise is idle
						if((VID_DEC_JPEG == rec_type) || (VID_GET_JPEG == rec_type))
						{
							JPEGB_path_scores = 0;
							break;
						}
						else
						{
							if(pVid_info->vid_rec_line)
							{// FIFO MODE
								if(pVid_info->vid_fifo_start)
								{// Encoding fifo buffer on this channel
									if(src == i)
									{// Input fifo buffer is this channel, Yes, return [engine is idle]
										JPEGB_path_scores = 0;
										break;
									}
									else
									{// No, return [engine is busy]
										++JPEGB_path_scores;
										break;
									}
								}
							}
						}
					}
				}
				else if(act_ctrl == ENC_ENGINE_ACT_SET)
				{// FRAME/FIFO MODE set
					ENCODE_STATUS_SET_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_ENCODE_JPEGB_OFFSET);
				}
				else if(act_ctrl == ENC_ENGINE_ACT_CLEAR)
				{// FRAME/FIFO MODE clear all channel
					ENCODE_STATUS_CLR_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_ENCODE_JPEGB_OFFSET);

					if(VID_REC_JPEG == rec_type)
					{
						if(pVid_info->vid_rec_line)
						{// FIFO MODE
							if(pVid_info->vid_fifo_start)
							{ // Encoding fifo buffer on this channel but not yet finished, encode the next fifo buffer now
								src_idx = i;
							}
						}

						osENTER_CRITICAL();
						cb_content_size[i] = circular_buf_size(cbuf_notify_enc[i]);
						osEXIT_CRITICAL();
					}
				}
			}
		}
	}

	if(act_ctrl == ENC_ENGINE_ACT_CHECK)
	{
		//DBG_PRINT("Path(H264,JPGA,JPGB)=(%d,%d,%d)\r\n",H264_path_scores,JPEGA_path_scores,JPEGB_path_scores);

		if((H264_path_scores > 0) || (JPEGA_path_scores > 0) || (JPEGB_path_scores > 0)) // Uses H264 / JPEGA /JPEGB engine now so it is busy
		{
			return STATUS_FAIL; // engine is busy
		}
	}

	if(act_ctrl == ENC_ENGINE_ACT_CLEAR)
	{
		if((VID_DEC_JPEG == rec_type) || (VID_GET_JPEG == rec_type))
		{
			return STATUS_OK;
		}

		//DBG_PRINT("src_idx:%d\r\n",src_idx);
		if(src_idx == -1)
		{
			max_size = cb_content_size[PREVIEW_SRC_FRONT];
			src_idx = PREVIEW_SRC_FRONT;
			/*
				Check the size of each content item
				Find the max size and encode it next
			*/
			for(i=1; i<PREVIEW_SRC_TOTAL; i++)
			{
				if(max_size < cb_content_size[i])
				{
					max_size = cb_content_size[i];
					src_idx = i;
				}
			}
		}
		else
		{
			max_size = cb_content_size[src_idx];
		}

		//DBG_PRINT("=>src_idx:%d max_size:%d \r\n",src_idx,max_size);
		if(max_size > 0)
		{
			pPsaclerStream = &pscaler_stream[src_idx];
			/*
				1. Encode another pscaler bypass buffer or display buffer after finished encoded H264 or JPEG
				2. USB_HOST: Don't send MSG_VID_PSCALER_NOTIFY on STATE_PHOTO_CAPTURE mode
			*/
			{
				if(msgQSend(pPsaclerStream->ps_notify_q, (MSG_VID_PSCALER_NOTIFY|src_idx), NULL, 0, MSG_PRI_NORMAL) < 0)
				{
					ERR_PRINT("fail to msgQSendFromISR [%d]\r\n",src_idx);
				}
			}
		}

		if(src_idx == -1)
		{// [src_idx == -1] is start of frame then do adjust_file_packer_prio()
	        pPsaclerStream = &pscaler_stream[PREVIEW_SRC_REAR];
	        pVid_info = pPsaclerStream->pVid_info;

	        if(	pVid_info && (pVid_info->run_ap_mode == STATE_VIDEO_RECORD)
			) // rear path exists
			{
	            adjust_file_packer_prio();
	        }
		}
	}

	return STATUS_OK;
}

INT32S
lib_encode_engine_status_check(
	INT8U src,
	VID_INFO* pVid_info
)
{
	if(!pVid_info)
	{
		/*
			Return STATUS_OK to get data and release data
			because some message residual in encoderTaskQ
		*/
		return STATUS_OK;
	}

	return lib_encode_engine_path_ctrl(src,pVid_info->vid_rec_type,ENC_ENGINE_ACT_CHECK);
}

INT32S
lib_encode_engine_status_set(
	INT8U src,
	VID_INFO* pVid_info
)
{
	if(pVid_info)
	{
		lib_encode_engine_path_ctrl(src,pVid_info->vid_rec_type,ENC_ENGINE_ACT_SET);
	}
	return STATUS_OK;
}

INT32S
lib_encode_engine_status_clear(
	INT8U src,
	VID_INFO* pVid_info
)
{
	if(pVid_info)
	{
		lib_encode_engine_path_ctrl(src,pVid_info->vid_rec_type,ENC_ENGINE_ACT_CLEAR);
	}
	return STATUS_OK;
}

void
lib_encode_release_cbuf_notify_enc(
	INT8U src,
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl
)
{
	INT32U data;
	INT32U remaining_count;
	INT32U i;
	FB_INFO* ps_fb_info=NULL;


	#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
	if(pPscalerStreamCtrl->pVid_info->run_2K_flow_src & HOST_2K_IS_FRONT) // This value is Greater than 0, that means front do scale-up flow
	{
		circular_buf_size(cbuf_notify_enc[src]);
		return;
	}
	#endif

	remaining_count = circular_buf_size(cbuf_notify_enc[src]);
	//DBG_PRINT("remaining_count:%d\r\n",remaining_count);

	for(i=0; i<remaining_count; i++)
	{
		if(circular_buf_get(cbuf_notify_enc[src],((INT32U*)&data)) == 0)
		{
			//DBG_PRINT("data:%x\r\n",data);

			ps_fb_info = (FB_INFO*)data;

			//DBG_PRINT("buf:%x\r\n",ps_fb_info->buf);

			if(ps_fb_info->buf != DUMMY_BUFFER_ADDRS)
			{
				pPscalerStreamCtrl->ps_free_func(ps_fb_info->buf);
				ps_fb_info->buf = DUMMY_BUFFER_ADDRS;
			}
		}
	}
}



#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
static volatile INT8U wifi_encode_stop_req = 0;
void wifi_encode_stop_req_set(void)
{
	wifi_encode_stop_req = 1;
}
void wifi_encode_stop_req_clear(void)
{
	wifi_encode_stop_req = 0;
}
INT8U wifi_encode_stop_req_get(void)
{
	return wifi_encode_stop_req;
}
#endif


INT32S
lib_encode_process(
	INT8U src,
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl
)
{
	INT32S nRet = STATUS_OK;
	INT32U data;
	FB_INFO* ps_fb_info = NULL;
	INT32U pscaler_out_buf_size;
	INT32U pscaler_out_max_count = 1; // Frame mode max count
	INT32U unmask_addr;
    INT32S ts;
	INT16S fifo_mode_pos_y;

	ENC_STREAM_CTRL_T *enc_stream_ctrl = pPscalerStreamCtrl->enc_stream_ctrl;
	ENC_STREAM_INFO_T *enc_stream_info = &pPscalerStreamCtrl->enc_stream_info;
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;

	if(lib_encode_engine_status_check(src,pVid_info) == STATUS_FAIL)
	{
		//DBG_PRINT("=>(%d)Busy\r\n",src);
		return STATUS_OK;
	}

	/*
		Video Encode: Get pscaler's bypass output buffer
		WIFI Stream: Get pscaler's display output buffer
	*/
	if(circular_buf_get(cbuf_notify_enc[src],((INT32U*)&data)) < 0)
	{
		//ERR_PRINT("fail to get FB_INFO[%d]\r\n",src);
		return (STATUS_FAIL);
	}

	ps_fb_info = (FB_INFO*)data;

	// Reached end of circular buffer
	if(ps_fb_info->buf == DUMMY_BUFFER_ADDRS)
	{
		if(pVid_info->run_ap_mode == STATE_VIDEO_RECORD)
		{
			if(pVid_info->vid_rec_line)
			{// fifo mode
				//DBG_PRINT("idx:%d\r\n",ps_fb_info->idx);
				if(ps_fb_info->idx == DUMMY_BUFFER_ADDRS)
				{
					// When FRONT pscaler and REAR pscaler both is stopped, send message to task
					lib_encode_release_cbuf_notify_enc(src,pPscalerStreamCtrl);

					if(lib_pscaler_stop_check(src, NULL) == STATUS_OK)
					{
						//DBG_PRINT("=>MSG_VID_RECORD_STOP\r\n");
						if(msgQSend(pPscalerStreamCtrl->ps_notify_q, MSG_VID_RECORD_STOP, NULL, 0, MSG_PRI_NORMAL) < 0)
						{
							ERR_PRINT("fail to msgQSendFromISR [%d]\r\n",src);
							goto Return;
						}
					}
					else
					{
						lib_encode_engine_status_clear(src,pVid_info); //trigger to check cbuf_notify_enc[]
					}

					return STATUS_OK;
				}
			}
			else
			{// frame mode
				if(pPscalerStreamCtrl->enc_stream_info.stream_enable == VIDEO_RECORD_NOTHING)
				{	/*
						Pscaler sends DUMMY_BUFFER_ADDRS to force the ending of video encoding in VIDEO_RECORD_NOTHING mode
						Don't encode DUMMY_BUFFER_ADDRS in VIDEO_RECORD_RECING mode
					*/

					lib_encode_release_cbuf_notify_enc(src,pPscalerStreamCtrl);

					// When FRONT pscaler and REAR pscaler both is stopped, send message to task
					if(lib_pscaler_stop_check(src, NULL) == STATUS_OK)
					{
						//DBG_PRINT("=>MSG_VID_RECORD_STOP\r\n");
						if(msgQSend(pPscalerStreamCtrl->ps_notify_q, MSG_VID_RECORD_STOP, NULL, 0, MSG_PRI_NORMAL) < 0)
						{
							ERR_PRINT("fail to msgQSendFromISR [%d]\r\n",src);
							goto Return;
						}
					}
					else
					{
						lib_encode_engine_status_clear(src,pVid_info); //trigger to check cbuf_notify_enc[]
					}
				}
				return STATUS_OK;
			}
		}
		else if(pVid_info->run_ap_mode == STATE_WIFI_TRANSMISSION)
		{ // WIFI is frame mode
			//DBG_PRINT("=>MSG_WIFI_BUF_RELEASE\r\n");
			lib_encode_release_cbuf_notify_enc(src,pPscalerStreamCtrl);
			if(msgQSend(pPscalerStreamCtrl->ps_notify_q, MSG_WIFI_BUF_RELEASE, NULL, 0, MSG_PRI_NORMAL) < 0)
			{
				ERR_PRINT("fail to MSG_WIFI_BUF_RELEASE\r\n");
			}
			return STATUS_OK;
		}
	}

	/*
		Handle residual message in encoderTaskQ to release encode buffer or display buffer
	*/
	if(!pVid_info)
	{
		RETURN(STATUS_FAIL);
	}


#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
	
	//check the wifi is enable and , wifi_encode_stop_req_get()==1 -> the wifi is disconnected.
	if((src == PREVIEW_SRC_WIFI) && wifi_encode_stop_req_get())
	{
		//check ps_fb_info->buf is real buffer.
		if(ps_fb_info->buf != DUMMY_BUFFER_ADDRS)
		{
			#if 0
			DBG_PRINT("[WIFI_STOP_REQ][ENC_DROP] src:%d fb:0x%x buf:0x%x stream:%d\r\n",
				src,
				ps_fb_info,
				ps_fb_info->buf,
				enc_stream_info->stream_enable);
			#endif
			
				//in PREVIEW_SRC_WIFI, the ps_free_func() should be wifi_pscaler_free_bypass()
				if(pPscalerStreamCtrl->ps_free_func)
				{
					//free wifi(display) buffer, or drop display buffer
					pPscalerStreamCtrl->ps_free_func(ps_fb_info->buf);
				}
			
			//after free buffer , set  ps_fb_info->buf as DUMMY_BUFFER_ADDRS.
			ps_fb_info->buf = DUMMY_BUFFER_ADDRS;
			//clear h264 encode busy status
			lib_encode_engine_status_clear(src, pVid_info);
	
			//retrun , because the buffer was already free.
			return STATUS_OK;
		}
		else
		{
			//in this place ,the buffer is dummy.
			DBG_PRINT("[WIFI_STOP_REQ][ENC_DUMMY] src:%d fb:0x%x stream:%d\r\n",src,ps_fb_info,enc_stream_info->stream_enable);
		}
	}
#endif


	// Add date/time stamps
	if(pVid_info->add_time_stamp)
	{
		switch(pVid_info->vid_rec_type)
		{
			case VID_REC_H264:
				pscaler_out_buf_size = pVid_info->h264_rec_args.fb_pscaler_out_cfg.fb_cfg_size;
				pscaler_out_max_count = pVid_info->h264_rec_args.fb_pscaler_out_cfg.fb_cfg_max_count;
				break;
			case VID_REC_JPEG:
				pscaler_out_buf_size = pVid_info->jpeg_rec_args.fb_pscaler_out_cfg.fb_cfg_size;
				pscaler_out_max_count = pVid_info->jpeg_rec_args.fb_pscaler_out_cfg.fb_cfg_max_count;
				break;
			default:
				pscaler_out_buf_size = 0;
				pscaler_out_max_count = 1; // Frame mode's max count
				break;
		}

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
		if(pVid_info->run_ap_mode == STATE_WIFI_TRANSMISSION)
		{
			cache_invalid_range(ps_fb_info->buf,pscaler_out_buf_size);
			cpu_draw_time_osd(((DISP_P)ps_fb_info->buf)->base, pVid_info->target_width, pVid_info->target_height, STATE_VIDEO_RECORD, 0, YUV420_DRAW, 0);
		}
#endif

#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
		if(pVid_info->run_ap_mode == STATE_CONNECT_TO_PC)
		{
			cache_invalid_range(ps_fb_info->buf,pscaler_out_buf_size);
			cpu_draw_time_osd(((DISP_P)ps_fb_info->buf)->base, pVid_info->target_width, pVid_info->target_height, STATE_VIDEO_RECORD, 0, YUV420_DRAW, 0);
		}
#endif

		if(pVid_info->run_ap_mode == STATE_VIDEO_RECORD)
		{
			#if C_VID_REC_USE_SYS_FIFO
			if(pPscalerStreamCtrl->flag_sys_fifo)
			{
				if(ps_fb_info->buf == DUMMY_BUFFER_ADDRS)
				{
					// FIFI mode sometimes get DUMMY_BUFFER_ADDRS buffer from queue
					unmask_addr = ps_fb_info->buf;
				}
				else
				{
					unmask_addr = lib_encode_system_fifo_unmapping(src,ps_fb_info->buf);
				}
			}
			else
			{
				unmask_addr = ps_fb_info->buf;
			}
			#else
			unmask_addr = ps_fb_info->buf;
			#endif

#if C_TIME_LAPSE

			if((ps_fb_info->idx == pscaler_out_max_count) && (pVid_info->vid_rec_line == 0)) // Frame done
			{
				nRet = time_lapse_check(src);
				if(nRet != STATUS_OK)
				{
					RETURN(STATUS_OK);
				}
			}
#endif

			if(unmask_addr != DUMMY_BUFFER_ADDRS) cache_invalid_range(unmask_addr,pscaler_out_buf_size);

			if(pVid_info->vid_rec_line)
			{// FIFO mode
				if(ps_fb_info->idx == pscaler_out_max_count)
				{// Draw date-time on the last fifo buffer
                    if(unmask_addr != DUMMY_BUFFER_ADDRS)
                    {
                    	#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
						if(pVid_info->run_2K_flow_src & HOST_2K_IS_FRONT) // This value is Greater than 0, that means front do scale-up flow
						{// Use target_width after doing scale-up
							fifo_mode_pos_y = -64;
							fifo_mode_pos_y += (pVid_info->target_height-(pscaler_out_max_count*pVid_info->vid_rec_line));

							cpu_draw_time_osd(unmask_addr, pVid_info->target_width, pVid_info->vid_rec_line, STATE_VIDEO_RECORD, 0, YUV420_DRAW, 0);
						}
						else
						#endif
						{
							fifo_mode_pos_y = -64;
							fifo_mode_pos_y += (pVid_info->sensor_height-(pscaler_out_max_count*pVid_info->vid_rec_line));

							ap_state_resource_time_stamp_position_y_set(fifo_mode_pos_y);

							cpu_draw_time_osd(unmask_addr, pVid_info->sensor_width, pVid_info->vid_rec_line, STATE_VIDEO_RECORD, 0, YUV420_DRAW, 0);
						}
                    }
				}
			}
			else
			{// Frame mode

				ap_state_resource_time_stamp_position_y_set(-64);

				if(pVid_info->data_format == VID_DATA_422)
				{
					cpu_draw_time_osd(unmask_addr, pVid_info->sensor_width, pVid_info->sensor_height, STATE_VIDEO_RECORD, 0, YUYV_DRAW, 0);
				}
				else
				{
					cpu_draw_time_osd(unmask_addr, pVid_info->sensor_width, pVid_info->sensor_height, STATE_VIDEO_RECORD, 0, YUV420_DRAW, 0);
				}
			}
		}
	}

	if(ps_fb_info->idx == pscaler_out_max_count) // Frame done
	{
		enc_stream_info->yuv_update_time = ps_fb_info->ps_buf_time;
	}

	if(enc_stream_ctrl->stream_encode_func)
	{
		nRet = enc_stream_ctrl->stream_encode_func(src,pPscalerStreamCtrl, ps_fb_info);
		if(nRet == STATUS_OK)
		{
			lib_encode_engine_status_set(src,pVid_info);
		}
		else
		{
			lib_encode_engine_status_clear(src,pVid_info);      //trigger to check cbuf_notify_enc[]
			RETURN(STATUS_FAIL);
		}
	}
	return STATUS_OK;
Return:
	/*
	Video Record Mode: Put pscaler bypass output buffer to circular buf
	WIFI Mode: Put display buffer to display_free_queue
	*/
	if(ps_fb_info->buf != DUMMY_BUFFER_ADDRS)
	{
		#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
		if((pVid_info->run_2K_flow_src & HOST_2K_IS_FRONT) == 0) // This value is Greater than 0, that means front do scale-up flow
		#endif
		{
			pPscalerStreamCtrl->ps_free_func(ps_fb_info->buf);
		}
		ps_fb_info->buf = DUMMY_BUFFER_ADDRS;
	}
	return nRet;
}

INT32S
lib_pscaler_process(
	INT32U src
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;

	//DBG_PRINT("==Run:%s==[%d]==0x%x\r\n",__FUNCTION__,src,pVid_info);

	if(pVid_info->run_ap_mode == STATE_PHOTO_CAPTURE)
	{
		//DBG_PRINT("=>Pho:%d\r\n",src);
		nRet = lib_photo_process(src);
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("fail to lib_photo_process\r\n");
			RETURN(STATUS_FAIL);
		}
	}
	else
	{
		//DBG_PRINT("=>Enc:%d\r\n",src);

		#if 0 // for wifi + uvc debug print
		if(wifi_encode_stop_req_get() &&
		   ((src == PREVIEW_SRC_REAR) || (src == PREVIEW_SRC_WIFI)))
		{

			DBG_PRINT("=>Enc:%d mode:%d stream:%d wifi_state:%d rec_sts:0x%x wifi_sts:0x%x\r\n",
				src,
				pVid_info->run_ap_mode,
				pPscalerStreamCtrl->enc_stream_info.stream_enable,
				video_encode_state_get(VIDEO_ENCODE_STATUS_WIFI_TRANSMISSION_OFFSET),
				ap_video_record_sts_get(),
				state_wifi_status_get());

		}
		#endif

		nRet = lib_encode_process(src,pPscalerStreamCtrl);
		if(nRet == STATUS_FAIL)
		{
			//ERR_PRINT("fail to lib_encode_process\r\n");
			RETURN(STATUS_FAIL);
		}
	}

	return STATUS_OK;

Return:

	return nRet;
}

INT32S
front_stream_init(
	void
)
{
	return(lib_pscaler_stream_init(PREVIEW_SRC_FRONT));
}

INT32S
front_encode_buf_set(
	MSG_Q_ID notifyQ,
	VID_INFO* args
)
{
	return(lib_encode_buf_set(PREVIEW_SRC_FRONT,notifyQ,args));
}

INT32S
front_encode_start(
	VID_INFO* args
)
{
	return(lib_encode_start(PREVIEW_SRC_FRONT,args));
}

INT32S
front_encode_stop(
	void
)
{
	return(lib_encode_stop(PREVIEW_SRC_FRONT));
}

INT32S
front_encode_buf_release(
	void
)
{
	return(lib_encode_buf_release(PREVIEW_SRC_FRONT));
}

INT32S
front_encode_engine_status_clear(
	VID_INFO* pVid_info
)
{
	if(pVid_info)
	{
		lib_encode_engine_path_ctrl(PREVIEW_SRC_FRONT,pVid_info->vid_rec_type,ENC_ENGINE_ACT_CLEAR);
	}
	return STATUS_OK;
}

static unsigned int
front_pscaler_get_bypass(
	void
)
{
	return(lib_pscaler_get_bypass(PREVIEW_SRC_FRONT));
}

static void
front_pscaler_free_bypass(
	unsigned int data
)
{
	return(lib_pscaler_free_bypass(PREVIEW_SRC_FRONT,data));
}

static void
front_pscaler_notify_bypass(
	unsigned char src,
	void* p,
	INT32U data
)
{
	return(lib_pscaler_notify_bypass(src,p,data));
}

/*== R E A R ==*/
INT32S
rear_stream_init(
	void
)
{
	return(lib_pscaler_stream_init(PREVIEW_SRC_REAR));
}

INT32S
rear_encode_buf_set(
	MSG_Q_ID notifyQ,
	VID_INFO* args
)
{
	return(lib_encode_buf_set(PREVIEW_SRC_REAR,notifyQ,args));
}

INT32S
rear_encode_start(
	VID_INFO* args
)
{
	return(lib_encode_start(PREVIEW_SRC_REAR,args));
}

INT32S
rear_encode_stop(
	void
)
{
	return(lib_encode_stop(PREVIEW_SRC_REAR));
}

INT32S
rear_encode_buf_release(
	void
)
{
	return(lib_encode_buf_release(PREVIEW_SRC_REAR));
}

INT32S
rear_encode_engine_status_clear(
	VID_INFO* pVid_info
)
{
	if(pVid_info)
	{
		lib_encode_engine_path_ctrl(PREVIEW_SRC_REAR,pVid_info->vid_rec_type,ENC_ENGINE_ACT_CLEAR);
	}
	return STATUS_OK;
}

static unsigned int
rear_pscaler_get_bypass(
	void
)
{
	return(lib_pscaler_get_bypass(PREVIEW_SRC_REAR));
}

static void
rear_pscaler_free_bypass(
	unsigned int data
)
{

	//DBG_PRINT("free buf:0x%x\r\n",data);
	return(lib_pscaler_free_bypass(PREVIEW_SRC_REAR,data));
}

static void
rear_pscaler_notify_bypass(
	unsigned char src,
	void* p,
	INT32U data
)
{
	return(lib_pscaler_notify_bypass(src,p,data));
}

/*== J P E G ==*/
INT32S
lib_jpeg_header_create(
	JpegEnc_t *pJpegEnc
)
{
	INT32S nRet = STATUS_OK;
	INT32U final_width,final_height;

	if(pJpegEnc->scale_mode == JPEG_SCALE_GP420_X3D2)
	{
		final_width = ((pJpegEnc->width*3) >> 1);
		final_height = ((pJpegEnc->height*3) >> 1);
	}
	else if(pJpegEnc->scale_mode == JPEG_SCALE_X2)
	{
        final_width = (pJpegEnc->width)<<1;
		final_height = (pJpegEnc->height)<<1;
	}
	else
	{
		final_width = pJpegEnc->width;
		final_height = pJpegEnc->height;
	}

	pJpegEnc->header = jpeg_header_generate(pJpegEnc->yuv_sample_fmt, pJpegEnc->quality_value, final_width, final_height, 0);
	if(pJpegEnc->header == 0)
	{
		ERR_PRINT("fail to jpeg_header_generate\r\n");
		RETURN(STATUS_FAIL);
	}

	return STATUS_OK;

Return:
	return nRet;
}

INT32S
lib_jpeg_setting(
	JpegEnc_t *pJpegEnc
)
{
	INT32S nRet = STATUS_OK;

	jpeg_encode_init(pJpegEnc->hwidx);

	if (pJpegEnc->quality_value)
	{
		gplib_jpeg_default_quantization_table_load(pJpegEnc->hwidx, pJpegEnc->quality_value);	// Load default qunatization table(quality)
	}

	gplib_jpeg_default_huffman_table_load(pJpegEnc->hwidx);								// Load default huffman table

	return nRet;
}

static void
lib_encode_JPEG_Callback(
	JPEG_IDX idx,
	INT32S event,
	INT32U count
)
{
	PSCALER_STREAM_CTRL_T* pPsaclerStream;
	VID_INFO* pVid_info;
	JPEG_REC_ARGS* pJpegRecArgs;
	JpegEnc_t*pJpegEnc;
	INT32U jpegHeaderSize = jpeg_header_get_size();
	INT8U i;

	// Check the handle
	for(i=PREVIEW_SRC_FRONT; i<PREVIEW_SRC_TOTAL; i++)
	{
		pPsaclerStream = &pscaler_stream[i];
		pVid_info = pPsaclerStream->pVid_info;

		if(!pVid_info || (pVid_info->vid_rec_type != VID_REC_JPEG))
		{ // Source path does not exists or vid_rec_type is not VID_REC_JPEG on this channel
			continue;
		}

		pJpegRecArgs = &pVid_info->jpeg_rec_args;

		if(pJpegRecArgs->flag_jpeg_trigger)
		{
			pJpegRecArgs->flag_jpeg_trigger = 0;

			pJpegEnc = &pJpegRecArgs->jpeg_cfg;

			/*
				Video Record Mode: release pscaler bypass buffer
				WIFI / PC Mode: release display buffer
			*/
			if(pJpegEnc->input_buffer_y != DUMMY_BUFFER_ADDRS)
			{
				pPsaclerStream->ps_free_func(pJpegEnc->input_buffer_y);
		    }

			pJpegEnc->encode_size = 0;

			if(event & C_JPG_STATUS_ENCODE_DONE)
			{
				pJpegEnc->encode_size = count+jpegHeaderSize;
				if((pJpegEnc->encode_size) & 0x3F)
				{
                    pJpegEnc->encode_size = (pJpegEnc->encode_size + 0x3F) >> 6 << 6; //64 byte alignment for hw constrain
                }
			}

			pJpegEnc->jpeg_status = event;

			msgQSend(pPsaclerStream->ps_notify_q, (MSG_VID_JPEG_NOTIFY|i), NULL, 0, MSG_PRI_NORMAL);
			break;
		}
	}

}

INT32S
lib_free_JPEG(
	INT8U src,
	INT32U data,
	INT32U frameType
)
{
	INT32S nRet = STATUS_OK;

 	// Put H264/JPEG encode output buffer back to circular buf
	if(data)
	{
		if(circular_buf_try_put(cbuf_encoder_out[src][ENC_H264I_JPEG_IDX],data) < 0)
		{
			DBG_PRINT("lib_free_JPEG() fail to put cbuf_encoder_out==[%d]\r\n",src);
			nRet = STATUS_FAIL;
		}
	}
	return nRet;
}

static INT32S
lib_encode_JPEG(
	unsigned char src,
	void* p,
	FB_INFO* fb_info
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = (PSCALER_STREAM_CTRL_T*)p;
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	JPEG_REC_ARGS* pJpegRecArgs = &pVid_info->jpeg_rec_args;
	JpegEnc_t*pJpegEnc =  &pJpegRecArgs->jpeg_cfg;
	INT32U jpeg_outb_header_size = sizeof(JPEG_OUTB_HEADER);
	INT32U jpegHeaderSize = jpeg_header_get_size();
	JPEG_OUTB_HEADER* pJpeg_outb_header;
	INT32U unmask_addr;
    FB_CFG* pPscalerOutCfg = &pVid_info->jpeg_rec_args.fb_pscaler_out_cfg;
	if(!pJpegEnc->header)
	{
		//ERR_PRINT("[ERR] pJpegEnc->handle is null\r\n");
		return STATUS_OK;
	}

	pJpegEnc->input_buffer_y = fb_info->buf;
	//DBG_PRINT("=>JPEG(%d,%d)\r\n",src,fb_info->idx);

	if(pVid_info->vid_fifo_start)
	{// FIFO MODE
		#if C_VID_REC_USE_SYS_FIFO
		if(pPscalerStreamCtrl->flag_sys_fifo)
		{
			if(pJpegEnc->input_buffer_y == DUMMY_BUFFER_ADDRS)
			{
				unmask_addr = pJpegEnc->input_buffer_y;
			}
			else
			{
				unmask_addr = lib_encode_system_fifo_unmapping(src,pJpegEnc->input_buffer_y);
			}
		}
		else
		{
			unmask_addr = pJpegEnc->input_buffer_y;
		}
		#else
		unmask_addr = pJpegEnc->input_buffer_y;
		#endif

		//DBG_PRINT("gp_h264enc_restart:%x\r\n",in.RawFrameAddr);
		if(unmask_addr == DUMMY_BUFFER_ADDRS)
		{// Stop video record and reset JPEG engine
			jpeg_encode_stop(pJpegEnc->hwidx);
			pVid_info->vid_fifo_start = 0;
			pJpegRecArgs->flag_jpeg_trigger = 0;
			RETURN(STATUS_FAIL);
		}
		else
		{
            //DBG_PRINT("%d",fb_info->idx);
			pJpegRecArgs->flag_jpeg_trigger = 1;
			jpeg_encode_on_the_fly_start(pJpegEnc->hwidx,unmask_addr,0,0,pJpegEnc->input_y_len);
		}
		return STATUS_OK;
	}

	pJpegEnc->output_buffer = 0;

	if(pVid_info->vid_rec_line)
	{ // FIFO mode
		if(fb_info->idx != 1)
		{// Encode the first fifo buffer
			RETURN(STATUS_FAIL);
		}
        #if C_TIME_LAPSE		//pscaler fifo mode
        if(fb_info->idx <= pPscalerOutCfg->fb_cfg_max_count)
        {
            nRet = time_lapse_check(src);
            if(nRet != STATUS_OK)
            {
                RETURN(STATUS_FAIL);
            }
        }
        #endif
	}
 	//DBG_PRINT("\r\n %d",fb_info->idx);
	// Get jpeg encode output buffer
	if(circular_buf_get(cbuf_encoder_out[src][ENC_H264I_JPEG_IDX],((INT32U*)&pJpegEnc->output_buffer)) < 0)
	{
	  #if 1 // Debug
		no_encode_out_cnt[src][ENC_H264I_JPEG_IDX]++;
		if (no_encode_out_cnt[src][ENC_H264I_JPEG_IDX]<16 || (no_encode_out_cnt[src][ENC_H264I_JPEG_IDX]&0xFF)==0) {
			DBG_PRINT("lib_encode_JPEG circular_buf_get(%d:%d)==NULL, cnt=%d\r\n", src, ENC_H264I_JPEG_IDX, no_encode_out_cnt[src][ENC_H264I_JPEG_IDX]);
		}
	  #endif
		RETURN(STATUS_FAIL);
	}

	if(pJpegEnc->output_buffer == 0)
	{
		ERR_PRINT("pJpegEnc->output_buffer is null ==[%d]\r\n",src);
		RETURN(STATUS_FAIL);
	}

	if((pVid_info->run_ap_mode == STATE_WIFI_TRANSMISSION) || (pVid_info->run_ap_mode == STATE_CONNECT_TO_PC))
	{
		unmask_addr = ((DISP_P)pJpegEnc->input_buffer_y)->base;
	}
	else
	{
		#if C_VID_REC_USE_SYS_FIFO
		if(pPscalerStreamCtrl->flag_sys_fifo)
		{
			unmask_addr = lib_encode_system_fifo_unmapping(src,pJpegEnc->input_buffer_y);
		}
		else
		{
			unmask_addr = pJpegEnc->input_buffer_y;
		}
		#else
		unmask_addr = pJpegEnc->input_buffer_y;
		#endif

		if(pVid_info->vid_rec_line)
		{ // FIFO mode
			pVid_info->vid_fifo_start = 1;
		}
	}

	// Check jpeg's q value if it needs to update header
	pJpeg_outb_header = (JPEG_OUTB_HEADER*)pJpegEnc->output_buffer;
	//DBG_PRINT("pJpegEnc->output_buffer:0x%x \r\n",pJpegEnc->output_buffer);
	//DBG_PRINT("[Q](%d)  ",pJpegEnc->quality_value);
	if(pJpegEnc->quality_value != pJpeg_outb_header->jpeg_q.Y_q_table)
	{
		pJpeg_outb_header->jpeg_q.Y_q_table = pJpegEnc->quality_value;
		cache_invalid_range((INT32U)pJpeg_outb_header->jpeg_header, jpegHeaderSize);
		jpeg_header_quality_set(pJpegEnc->hwidx,pJpegEnc->header,pJpeg_outb_header->jpeg_q.Y_q_table,pJpeg_outb_header->jpeg_q.Y_q_table);
		gp_memcpy((INT8S *)pJpeg_outb_header->jpeg_header,(INT8S *)pJpegEnc->header,jpegHeaderSize);
		cache_drain_range((INT32U)pJpeg_outb_header->jpeg_header, jpegHeaderSize);
	}

	jpeg_encode_init(pJpegEnc->hwidx);
	nRet = jpeg_encode_input_size_set(pJpegEnc->hwidx, pJpegEnc->width, pJpegEnc->height);
	if(nRet < 0)
	{
		ERR_PRINT("fail to jpeg_encode_input_size_set\r\n");
		RETURN(STATUS_FAIL);
	}

	jpeg_encode_input_format_set(pJpegEnc->hwidx, pJpegEnc->yuv_fmt);

	switch(pJpegEnc->yuv_sample_fmt)
	{
	case JPEG_YUV422:
		nRet = jpeg_encode_yuv_sampling_mode_set(pJpegEnc->hwidx, C_JPG_CTRL_YUV422);
		break;

	case JPEG_YUV420:
		nRet = drv_l1_jpeg_yuv_sampling_mode_set(pJpegEnc->hwidx, C_JPG_CTRL_GP420 | C_JPG_CTRL_YUV420);
		break;

	default:
		nRet = STATUS_FAIL;
	}
	if(nRet < 0)
	{
		ERR_PRINT("fail to drv_l1_jpeg_yuv_sampling_mode_set\r\n");
		RETURN(STATUS_FAIL);
	}

	switch(pJpegEnc->scale_mode)
	{
	case JPEG_SCALE_VX2:
		drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_VX2);
		break;
	case JPEG_SCALE_X2:
		drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_X2);
		break;
	case JPEG_SCALE_YUYV_X4:
		drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_X4);
		break;
	case JPEG_SCALE_GP420_X3D2:
		drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_X1_5);
		break;
	case JPEG_SCALE_YUYV_X9D8:
		drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_X9D8);
		break;
	default:
		break;
	}

	nRet = jpeg_encode_output_addr_set(pJpegEnc->hwidx, pJpegEnc->output_buffer+jpeg_outb_header_size);
	if(nRet < 0) {
		ERR_PRINT("fail to jpeg_encode_output_addr_set\r\n");
		RETURN(STATUS_FAIL);
	}

	jpeg_encode_maximum_length_set(pJpegEnc->hwidx, pJpegEnc->output_buffer_len-jpeg_outb_header_size);

	jpeg_encode_isr_callback_set(pJpegEnc->hwidx,lib_encode_JPEG_Callback);

	pJpegRecArgs->flag_jpeg_trigger = 1;

	if(pVid_info->vid_rec_line)
	{// FIFO MODE
		//DBG_PRINT("=>JPEG FIFO\r\n");
		jpeg_encode_on_the_fly_start(pJpegEnc->hwidx,unmask_addr,0,0,pJpegEnc->input_y_len);
	}
	else
	{// FRAME MODE
		//DBG_PRINT("=>JPEG FRAME\r\n");
		jpeg_encode_once_start(pJpegEnc->hwidx,unmask_addr,0,0);
	}

	return STATUS_OK;
Return:

	// Put jpeg encode output buffer back to circular buf if an error occurred
	if(pJpegEnc->output_buffer)
	{
		if(circular_buf_try_put(cbuf_encoder_out[src][ENC_H264I_JPEG_IDX],pJpegEnc->output_buffer) < 0)
		{
			ERR_PRINT("fail to lib_encode_JPEG put cbuf_encoder_out==[%d]\r\n",src);
		}
	}
	return nRet;
}

static INT32S
lib_enc_write_JPEG(
	INT8U src,
	void* p
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T *stream_ctrl = (PSCALER_STREAM_CTRL_T*) p;
	ENC_STREAM_INFO_T *enc_stream_info = &stream_ctrl->enc_stream_info;
	VID_INFO* pVid_info = stream_ctrl->pVid_info;
	JpegEnc_t*pJpegEnc = &pVid_info->jpeg_rec_args.jpeg_cfg;
	JengDynaQ_t* pJpeg_dyna_q = &pVid_info->jpeg_rec_args.jpeg_q_cfg;
	JPEG_OUTB_HEADER* pJpeg_outb_header;
	#if C_SUPORT_GET_JPG_BUF
	GET_JPEG_ARGS* pGet_jpeg_args = &pVid_info->get_jpeg_args;
	#endif
	FILE_PACKER_W_ARGS File_packer_args;

	if(!pJpegEnc->header)
	{
		//ERR_PRINT("[ERR] pJpegEnc->handle is null\r\n");
		return STATUS_OK;
	}

	if(pJpegEnc->jpeg_status & C_JPG_STATUS_INPUT_EMPTY)
	{
		//DBG_PRINT("=>C_JPG_STATUS_INPUT_EMPTY\r\n");
		return STATUS_OK;
	}

	// Clear vid_fifo_start after return C_JPG_STATUS_ENCODE_DONE or C_JPG_STATUS_OUTPUT_FULL
	pVid_info->vid_fifo_start = 0;

	// Dynamic tune jpeg's quality
	vid_dyna_tune_Q_cal(pJpeg_dyna_q,pJpegEnc->encode_size,&pJpegEnc->quality_value);

	// an error occurred when processing the jpeg: C_JPG_STATUS_OUTPUT_FULL
	if((pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODE_DONE) != C_JPG_STATUS_ENCODE_DONE)
	{
		if(pJpegEnc->jpeg_status & C_JPG_STATUS_OUTPUT_FULL){
			// Restart jpeg engine
            jpeg_encode_stop(pJpegEnc->hwidx);
			pVid_info->vid_fifo_start = 0;
		} else {
            ERR_PRINT("[%d] jpeg_status:0x%x\r\n",src,pJpegEnc->jpeg_status);
        }
		lib_free_JPEG(src, pJpegEnc->output_buffer, 0);
		pJpegEnc->output_buffer = 0;
		//ERR_PRINT("[%d] jpeg_status:0x%x\r\n",src,pJpegEnc->jpeg_status);
		return STATUS_FAIL;
	}

	if(pVid_info->run_ap_mode == STATE_VIDEO_RECORD)
	{
		INT32U pts, drop_pts = 0;
		INT8U time_lapse_flag = 0;
		INT8U insert_skip_flag = 0;
		INT8U flag_get_jpeg = 0;

	  #if C_SUPORT_GET_JPG_BUF
		if (pGet_jpeg_args->flag_start_buf_get) {
			flag_get_jpeg = 1;
		}
	  #endif

      #if C_TIME_LAPSE
        if (ap_state_config_time_lapse_recording_get()) {
            pts = 1000 / ap_state_config_time_lapse_fps_get();
            time_lapse_flag = 1;
        } else
      #endif
        {
			if (enc_stream_info->frame_rate != pVid_info->frame_rate) {
        		enc_stream_info->frame_rate = pVid_info->frame_rate;
        		enc_stream_info->frame_pts = 1000 / enc_stream_info->frame_rate;
        	}
        	pts = enc_stream_info->frame_pts;
        }

		// First frame must be I-Frame
		if (enc_stream_info->stream_update_time) {
			if (enc_stream_info->stream_update_time > enc_stream_info->yuv_update_time + (pts>>1) + (pts>>2)) {
				// Drop frames that come too early
				if (src==0) {
                    DBG_PRINT("D");
                } else if (src==1) {
                    DBG_PRINT("d");
                }

				lib_free_JPEG(src, pJpegEnc->output_buffer, 0);
				pJpegEnc->output_buffer = 0;

				return STATUS_FAIL;

			} else if (enc_stream_info->stream_update_time + (pts>>1) < enc_stream_info->yuv_update_time) {
				// Don't insert skip frame if we are recording a time lapse video
				if (!time_lapse_flag) {
					// Insert skip frame
					insert_skip_flag = 1;
				}
			}
		}

		File_packer_args.src = src;
		File_packer_args.snapshot = 0;
		File_packer_args.type = VID_REC_JPEG;
		File_packer_args.drop_time = &drop_pts;						// Not used

		if (insert_skip_flag) {
			// Inser skip frame here
			INT32U packer_skip_buf;
			INT32S pts_skip;
            INT32U skip_frame_cnt = 0;

			File_packer_args.track = MOV_TRACK_VID_DUMMYRECORD;
			File_packer_args.Dummy_frame_cnt = 1;
			File_packer_args.frameType = 2; 	// 0=H264_FRAME_TYPE_I, 1=H264_FRAME_TYPE_P, 2=Skip frame(not used by H264 engine, only for packer and cbuf_encoder_out[] management)
			File_packer_args.pts = pts_skip = pts;
            File_packer_args.data = (INT8U *) pJpeg_outb_header->jpeg_header;
            File_packer_args.size = pJpegEnc->encode_size;

			while (enc_stream_info->stream_update_time + (pts>>1) < enc_stream_info->yuv_update_time) {
				if (enc_stream_info->yuv_update_time - enc_stream_info->stream_update_time < pts) {
					File_packer_args.pts = pts_skip = enc_stream_info->yuv_update_time - enc_stream_info->stream_update_time;
				}

				if (file_packer_write_notify(&File_packer_args) == STATUS_OK) {
					enc_stream_info->pack_frame_cnt++;
					enc_stream_info->stream_update_time += pts_skip;
                    skip_frame_cnt++;

                    #if STATISTICS_DUMP
                    if(src == 0){
                        skip_front_wr_cnt++;
                    }else if(src == 1){
                        skip_rear_wr_cnt++;
                    }
                    #endif

					if (pts_skip != pts) {
						pts += (pts-pts_skip);
						break;
					}

                    #if 1		// Break here if we don't allow too many consecutive skip frame
					if (skip_frame_cnt >= 10) {
                    	DBG_PRINT("too many consecutive skip frame\r\n");
						break;
					}
                    #endif
				} else {
					DBG_PRINT("Failed to send dummy frame to MOV packer\r\n");

					break;
				}
			}
		}

		if (enc_stream_info->stream_start_time && (!time_lapse_flag)) {
			INT32U ideal_time, current_time;
			INT8U max_cnt = 5;		// Add 5ms at most

			ideal_time = 1000*(enc_stream_info->pack_frame_cnt+1);
			current_time = (enc_stream_info->stream_update_time-enc_stream_info->stream_start_time+pts) * enc_stream_info->frame_rate;
			// If video stream is latter than ideal frame time, add 1ms to pts. For example, fps=30, frame display time=1000ms/30=33.3333ms. Set pts to 33ms or (33+1)ms
			while ((current_time < ideal_time) && max_cnt) {
				current_time += enc_stream_info->frame_rate;
				pts++;
				max_cnt--;
			}
		}

		// Change to JPEG_OUTB_HEADER type
		pJpeg_outb_header = (JPEG_OUTB_HEADER *) pJpegEnc->output_buffer;

		File_packer_args.track = MOV_TRACK_VID_RECORD;
		File_packer_args.snapshot = flag_get_jpeg;
		File_packer_args.Dummy_frame_cnt = 0;
		File_packer_args.frameType = H264_FRAME_TYPE_I;			// 0=H264_FRAME_TYPE_I or MJPEG, 1=H264_FRAME_TYPE_P, 2=Skip frame(not used by H264 engine, only for packer and cbuf_encoder_out[] management)
		File_packer_args.ret_data = pJpegEnc->output_buffer;
		File_packer_args.data = (INT8U *) pJpeg_outb_header->jpeg_header;
		File_packer_args.size = pJpegEnc->encode_size;
		File_packer_args.pts = pts;									// Unit is ms

		nRet = file_packer_write_notify(&File_packer_args);
		if (nRet == STATUS_OK) {

            #if STATISTICS_DUMP
            if(src==0){
                front_wr_cnt++;
            }else if(src==1){
                rear_wr_cnt++;
            }
            #endif

			enc_stream_info->pack_frame_cnt++;

			if (!enc_stream_info->stream_update_time) {
				enc_stream_info->stream_start_time = enc_stream_info->yuv_update_time;
				enc_stream_info->stream_update_time = enc_stream_info->stream_start_time + pts;
			} else {
				enc_stream_info->stream_update_time += pts;
			}

		} else {		// STATUS_FAIL
			DBG_PRINT("Fail to pack MJPEG(0x%X, %d)\r\n", pJpegEnc->output_buffer, pJpegEnc->encode_size);
		}
	}
  #if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	else if (pVid_info->run_ap_mode == STATE_WIFI_TRANSMISSION)
	{
		nRet = STATUS_FAIL;
	}
  #endif
  #if (defined C_USB_DEVICE) && (C_USB_DEVICE)
	else if (pVid_info->run_ap_mode == STATE_CONNECT_TO_PC)
	{
		pJpeg_outb_header = (JPEG_OUTB_HEADER*)pJpegEnc->output_buffer;
		nRet = usb_jpeg_send((INT32U)pJpeg_outb_header->jpeg_header, pJpegEnc->encode_size, pJpegEnc->output_buffer);
	}
  #endif
	else
	{
		nRet = STATUS_FAIL;
	}

	if (nRet == STATUS_OK)
	{
		if(pVid_info->run_ap_mode == STATE_VIDEO_RECORD)
		{
			#if C_SUPORT_GET_JPG_BUF
			if(pGet_jpeg_args->flag_start_buf_get)
			{
				pGet_jpeg_args->flag_start_buf_get = 0;
				lib_get_jpeg_enc_process(src);
			}
			#endif
		}
	}
	else
	{
		//DBG_PRINT("jpeg-w src:%d, output_buffer:0x%x, encode_size:%d PictureType:%d\r\n",src,pJpegEnc->output_buffer,pJpegEnc->encode_size,pJpegEnc->PictureType);
		//DBG_PRINT(" putBuf:%x,size:%d\r\n",pJpegEnc->output_buffer,pJpegEnc->encode_size);
		lib_free_JPEG(src, pJpegEnc->output_buffer, 0);
		pJpegEnc->output_buffer = 0;
	}

	return nRet;
}

INT32S
lib_JPEG_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	ENC_STREAM_CTRL_T *enc_stream_ctrl = pPscalerStreamCtrl->enc_stream_ctrl;

	if(enc_stream_ctrl && enc_stream_ctrl->stream_write_func)
	{
		nRet = enc_stream_ctrl->stream_write_func(src,(void*)pPscalerStreamCtrl);
	}

	// Clear encode status to do the next
	lib_encode_engine_status_clear(src,pVid_info);

	return nRet;
}

/*== H 2 6 4 ==*/
INT32S
lib_h264_setting(
	H264Enc_t *pH264Enc
)
{
	INT32S nRet = STATUS_OK;

	pH264Enc->handle = gp_h264enc_init(&pH264Enc->cfg);

	DBG_PRINT("pH264Enc->handle:0x%x \r\n",pH264Enc->handle);

	if (!pH264Enc->handle)
	{
		ERR_PRINT("fail to gp_h264enc_init\r\n");
		RETURN(STATUS_FAIL);
	}

	gp_h264enc_set_rc(pH264Enc->handle, &pH264Enc->rccfg);

	return STATUS_OK;
Return:

	return nRet;
}

static void
lib_encode_H264_Callback(
	void* handle
)
{
	PSCALER_STREAM_CTRL_T* ps_stram_ctrl = NULL;
	VID_INFO* pVid_info = NULL;
	H264Enc_t*p264Enc = NULL;
	INT8U i;

	//DBG_PRINT("handle:0x%x \r\n",handle);
	if(!handle)
	{
		return;
	}

	// Check the handle is PREVIEW_SRC_FRONT or PREVIEW_SRC_REAR
	for(i=PREVIEW_SRC_FRONT; i<PREVIEW_SRC_TOTAL; i++)
	{
		ps_stram_ctrl = &pscaler_stream[i];
		pVid_info = ps_stram_ctrl->pVid_info;

		p264Enc = &pVid_info->h264_rec_args.h264_cfg;
		if(handle == p264Enc->handle)
		{
			/*
				Video Record Mode: release pscaler bypass frame/fifo buffer
				WIFI / PC Mode: release display frame buffer
			*/
			if(p264Enc->input_buffer != DUMMY_BUFFER_ADDRS)
			{
				if(p264Enc->input_buffer_hold == 0)
				{
					#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
					if((pVid_info->run_2K_flow_src & HOST_2K_IS_FRONT) == 0) // This value is Greater than 0, that means front do scale-up flow
					#endif
					{
						ps_stram_ctrl->ps_free_func(p264Enc->input_buffer);
					}
					p264Enc->input_buffer = DUMMY_BUFFER_ADDRS;
				}
		    }
			msgQSend(ps_stram_ctrl->ps_notify_q, (MSG_VID_H264_NOTIFY|i), NULL, 0, MSG_PRI_NORMAL);
			break;
		}
	}
}

INT32S
lib_free_H264(
	INT8U src,
	INT32U data,
	INT32U frameType
)
{
	INT32S nRet = STATUS_OK;

 	// Put h264 encode output buffer back to circular buf if an error occurred
	if(data)
	{
		if(circular_buf_try_put(cbuf_encoder_out[src][frameType],data) < 0)
		{
			//ERR_PRINT("fail to lib_write_H264 put cbuf_encoder_out==[%d]\r\n",src);
			nRet = STATUS_FAIL;
		}
	}
	return nRet;
}

static INT32S
lib_encode_H264(
	INT8U src,
	void* p,
	FB_INFO* fb_info
)
{
	INT32S nRet = STATUS_OK;
	FB_CFG* fb_h264_out_cfg;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = (PSCALER_STREAM_CTRL_T*)p;
	ENC_STREAM_INFO_T *enc_stream_info = &pPscalerStreamCtrl->enc_stream_info;
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	H264Enc_t*pH264Enc =  &pVid_info->h264_rec_args.h264_cfg;
	gp_h264enc_in in;
	INT32U unmask_addr;
#if C_SUPORT_GET_JPG_BUF
	GET_JPEG_ARGS* pGet_jpeg_args = &pVid_info->get_jpeg_args;
	FB_CFG* pPscalerOutCfg = &pVid_info->h264_rec_args.fb_pscaler_out_cfg;
#endif

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);
	if(!pH264Enc->handle)
	{
		//ERR_PRINT("[ERR] pH264Enc->handle is null\r\n");
		return STATUS_OK;
	}

	pH264Enc->input_buffer = fb_info->buf;
	pH264Enc->input_buffer_idx = fb_info->idx;
	pH264Enc->input_buffer_hold = 0;
	//DBG_PRINT("=>H264(%d,%d)\r\n",src,fb_info->idx);

	if(pVid_info->vid_fifo_start)
	{// FIFO MODE
#if C_VID_REC_USE_SYS_FIFO
		if(pPscalerStreamCtrl->flag_sys_fifo)
		{
			if(pH264Enc->input_buffer == DUMMY_BUFFER_ADDRS)
			{
				unmask_addr = pH264Enc->input_buffer;
				/*if(src==0){
				    DBG_PRINT("(F~)");
				}else{
	                DBG_PRINT("(R~)");
            	}*/
			}
			else
			{
				unmask_addr = lib_encode_system_fifo_unmapping(src,pH264Enc->input_buffer);
			}
		}
		else
		{
			unmask_addr = pH264Enc->input_buffer;
		}
#else
		unmask_addr = pH264Enc->input_buffer;
#endif

		in.RawFrameAddr = unmask_addr;

		//DBG_PRINT("gp_h264enc_restart:%x\r\n",in.RawFrameAddr);
		if(in.RawFrameAddr == DUMMY_BUFFER_ADDRS)
		{// Stop video record and reset H264 engine
			gp_h264enc_reset(pH264Enc->handle);
			pVid_info->vid_fifo_start = 0;

#if C_SUPORT_GET_JPG_BUF
			if(pGet_jpeg_args->flag_start_buf_get && pGet_jpeg_args->flag_start_fifo_buf_get)
			{// Release pscaler fifo buffer and try again
				lib_get_jpeg_pscaler_buf_release(src,pPscalerStreamCtrl);
				pGet_jpeg_args->flag_start_fifo_buf_get = 0;
			}
#endif

			if ((INT32U) gp_h264_get_hierP_Type(pH264Enc->handle) != SLICE_TYPE_HP) {			// 0=SLICE_TYPE_P, 1=SLICE_TYPE_HP, 2=SLICE_TYPE_I, 3=SLICE_TYPE_SKIP
				// This frame is referenced by the following frames. Failed to write into MOV packer will induce error in decoding the following P-frames. Set next frame type to I-frame to avoid this error.
				enc_stream_info->h264_frame_num = 0;
				enc_stream_info->h264_pic_order_cnt = 0;
			}

			RETURN(STATUS_FAIL);
		}
		else
		{

#if C_SUPORT_GET_JPG_BUF
			if(pGet_jpeg_args->flag_start_buf_get && pGet_jpeg_args->flag_start_fifo_buf_get)
			{// Start getting pscaler's fifo bypass buffer

				#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
				if(pVid_info->run_2K_flow_src & HOST_2K_IS_FRONT)
				{
					lib_get_jpeg_buf_notify(src,pPscalerStreamCtrl,fb_info,pGet_jpeg_args->flag_start_buf_get);
					if(fb_info->idx == pPscalerOutCfg->fb_cfg_max_count)
					{
						pGet_jpeg_args->flag_start_buf_get = 0; // Finish getting fifo buffer
						pGet_jpeg_args->flag_start_fifo_buf_get = 0;
					}
				}
				else
				#endif
				{
					pH264Enc->input_buffer_hold++;
					if(fb_info->idx == pPscalerOutCfg->fb_cfg_max_count)
					{
						pGet_jpeg_args->flag_start_buf_get = 0; // Finish getting fifo buffer
						pGet_jpeg_args->flag_start_fifo_buf_get = 0;
					}
					lib_get_jpeg_buf_notify(src,pPscalerStreamCtrl,fb_info,!(pGet_jpeg_args->flag_start_buf_get));
				}
			}
#endif
			if(gp_h264enc_restart(pH264Enc->handle, STATUS_H264_FIFO_IN_END, in.RawFrameAddr, 0, 0) < 0)
			{
				ERR_PRINT("fail to gp_h264enc_restart\r\n");
			}
		}
		return STATUS_OK;
	}

	pH264Enc->output_buffer = 0;

	//Encode One Frame
	if (!enc_stream_info->h264_pic_order_cnt || (enc_stream_info->h264_pic_order_cnt >= pH264Enc->rccfg.gopLen)) {
		// I-frame
		gp_h264_set_idr(pH264Enc->handle, enc_stream_info->h264_idr_pic_id);		// Set idr of I-frame

		in.PictureType = H264_FRAME_TYPE_I;			// 0=H264_FRAME_TYPE_I, 1=H264_FRAME_TYPE_P
		in.InsertHdr = 1; //insert h264 header (only I frame)
		fb_h264_out_cfg = &pVid_info->h264_rec_args.fb_h264_out_I_cfg;
	}  else {
		// P-frame
		gp_h264_set_frameNum(pH264Enc->handle, enc_stream_info->h264_frame_num);
		gp_h264_set_picOrderCnt(pH264Enc->handle, enc_stream_info->h264_pic_order_cnt);

		in.PictureType = H264_FRAME_TYPE_P;			// 0=H264_FRAME_TYPE_I, 1=H264_FRAME_TYPE_P
		in.InsertHdr = 0;
		fb_h264_out_cfg = &pVid_info->h264_rec_args.fb_h264_out_P_cfg;
	}

	in.ae_night = 0;
	in.input_fifo = pVid_info->vid_rec_line; //input fifo line: 0: disable or 16 align number

	switch(pVid_info->data_format)
	{
		case VID_DATA_422:
			in.format = H264_FMT_YUYV; //input format: YUYV/YUYU/UYVY/VYUY/GP420
			break;
		case VID_DATA_420:
			in.format = H264_FMT_GP420; //input format: YUYV/YUYU/UYVY/VYUY/GP420
			break;
	}

	/*
	  besides run_ap_mode == STATE_VIDEO_RECORD , input_buffer couldn't be 0xFFFFFFFF
	  becasue put 0xFFFFFFFF to the Q condtion only run_ap_mode == STATE_VIDEO_RECORD
	*/
	if(fb_info->buf == 0xFFFFFFFF)   // skip frame condition
	{
		RETURN(STATUS_FAIL);
	}
	else  // Get h264 encode output buffer ( I / P )
	{
		if(pVid_info->vid_rec_line)
		{ // FIFO mode
			if(fb_info->idx != 1)
			{// Encode the first fifo buffer
				RETURN(STATUS_FAIL);
			}
			#if C_TIME_LAPSE		//pscaler fifo mode
				if(fb_info->idx <= pPscalerOutCfg->fb_cfg_max_count)
				{
					nRet = time_lapse_check(src);
					if(nRet != STATUS_OK)
					{
						RETURN(STATUS_FAIL);
					}
				}
			#endif
		}

		if(circular_buf_get(cbuf_encoder_out[src][in.PictureType],((INT32U*)&pH264Enc->output_buffer)) < 0)
		{
		  #if 1 // Debug
			no_encode_out_cnt[src][in.PictureType]++;
			if (no_encode_out_cnt[src][in.PictureType]<16 || (no_encode_out_cnt[src][in.PictureType]&0xFF)==0)
			{
				DBG_PRINT("lib_encode_H264 circular_buf_get(%d:%d)==NULL, cnt=%d\r\n", src, in.PictureType, no_encode_out_cnt[src][in.PictureType]);
			}
		  #endif

			RETURN(STATUS_FAIL);
		} else {
            no_encode_out_cnt[src][in.PictureType]=0;
        }

		if(pH264Enc->output_buffer == 0)
		{
			ERR_PRINT("p264Enc->output_buffer is null ==[%d]\r\n",src);
			RETURN(STATUS_FAIL);
		}

		pH264Enc->PictureType = in.PictureType;			// 0=H264_FRAME_TYPE_I, 1=H264_FRAME_TYPE_P

		//DBG_PRINT("encode out buf:0x%x size:0x%x type:%d\r\n",pH264Enc->output_buffer,fb_h264_out_cfg->fb_cfg_size,pH264Enc->PictureType);
		if((pVid_info->run_ap_mode == STATE_WIFI_TRANSMISSION) || (pVid_info->run_ap_mode == STATE_CONNECT_TO_PC))
		{
			in.RawFrameAddr = ((DISP_P)pH264Enc->input_buffer)->base;
		}
		else
		{
#if C_VID_REC_USE_SYS_FIFO
			if(pPscalerStreamCtrl->flag_sys_fifo)
			{
				unmask_addr = lib_encode_system_fifo_unmapping(src,pH264Enc->input_buffer);
			}
			else
			{
				unmask_addr = pH264Enc->input_buffer;
			}
#else
			unmask_addr = pH264Enc->input_buffer;
#endif

			in.RawFrameAddr = unmask_addr;

			if(pVid_info->vid_rec_line)
			{ // FIFO mode
				pVid_info->vid_fifo_start = 1;
			}

#if C_SUPORT_GET_JPG_BUF
			if(pGet_jpeg_args->flag_start_buf_get)
			{// Start getting pscaler's frame/fifo bypass buffer
				pGet_jpeg_args->flag_start_fifo_buf_get = 1;

				#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
				if(pVid_info->run_2K_flow_src & HOST_2K_IS_FRONT)
				{
					lib_get_jpeg_buf_notify(src,pPscalerStreamCtrl,fb_info,pGet_jpeg_args->flag_start_buf_get);
				}
				else
				#endif
				{
					pH264Enc->input_buffer_hold++;
					if(fb_info->idx == pPscalerOutCfg->fb_cfg_max_count)
					{// FRAME MODE: get one frame
						pGet_jpeg_args->flag_start_buf_get = 0; // Finish getting frame/fifo buffer
						pGet_jpeg_args->flag_start_fifo_buf_get = 0;
					}
					lib_get_jpeg_buf_notify(src,pPscalerStreamCtrl,fb_info,!(pGet_jpeg_args->flag_start_buf_get));
				}
			}
#endif
		}

		cache_invalid_range(pH264Enc->output_buffer, fb_h264_out_cfg->fb_cfg_size);
	}
	in.BitstreamBaseAddr = pH264Enc->output_buffer;
	in.BitstreamOutAddr = pH264Enc->output_buffer;
	in.BitstreamEndAddr = pH264Enc->output_buffer + fb_h264_out_cfg->fb_cfg_size;

	gp_h264enc_set_callback(lib_encode_H264_Callback);

	pH264Enc->flag_h264_trigger = 1;
	nRet = gp_h264enc_encode(pH264Enc->handle, &in);
	if(nRet < 0)
	{
		pH264Enc->flag_h264_trigger = 0;

		ERR_PRINT("fail to gp_h264enc_encode\r\n");
#if C_SUPORT_GET_JPG_BUF
		if(pGet_jpeg_args->flag_start_buf_get && pGet_jpeg_args->flag_start_fifo_buf_get)
		{// Release pscaler frame/fifo buffer and try again
			lib_get_jpeg_pscaler_buf_release(src,pPscalerStreamCtrl);
			pGet_jpeg_args->flag_start_fifo_buf_get = 0;

			/*
				The fb_info is putted into lib_get_jpeg_buf_notify()
				Don't release fb_info->buf again after returning STATUS_FAIL
			*/
			fb_info->buf = DUMMY_BUFFER_ADDRS;
		}
#endif
		RETURN(STATUS_FAIL);
	}

	return STATUS_OK;

Return:
	lib_free_H264(src,pH264Enc->output_buffer,pH264Enc->PictureType);
	pH264Enc->output_buffer = 0;

	#if (C_VID_INTERPOLATING_2K)
	interp2K_scale_up_err_handle(src,pH264Enc->input_buffer_idx,pVid_info);
	#endif

	return nRet;
}

INT32S lib_enc_dummywrite(INT16U src)
{
    INT32S nRet = STATUS_OK;
    FILE_PACKER_W_ARGS File_packer_args;

    File_packer_args.src = src&0xf;
    File_packer_args.snapshot = 0;
	File_packer_args.track = MOV_TRACK_VID_DUMMYRECORD;
	File_packer_args.type = VID_REC_JPEG;

	File_packer_args.pts = (src&0xff0)>>4;      //pts (ms)
    File_packer_args.frameType = 2;				// 0=H264_FRAME_TYPE_I or MJPEG, 1=H264_FRAME_TYPE_P, 2=Skip frame(not used by H264 engine, only for packer and cbuf_encoder_out[] management)

    File_packer_args.Dummy_frame_cnt = (src&0xf000)>>12;
	nRet = file_packer_write_notify(&File_packer_args);

	return nRet;
}

void h264_set_reverse_number(INT8U *ptr, unsigned int value)
{
	INT16S i, k;

	k = 0;
	for (i=7; i>=0; i--) {
		if ((value >> i) & 0x1) {
			*ptr |= (0x1 << k);
		} else {
			*ptr &= ~(0x1 << k);
		}

		if (k) {
			k--;
		} else {
			k = 7;
			ptr++;
		}
	}
}

INT8U h264_skip_frame[]={		// Up to 2560x2048
	0x00, 0x00, 0x00, 0x01, 0x01, 0x9A, 0x02, 0x03, 0xBF, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
	0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
	0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
	0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0xE3, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static INT32S
lib_enc_write_H264(
	INT8U src,
	void* p
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T *stream_ctrl = (PSCALER_STREAM_CTRL_T*) p;
	ENC_STREAM_INFO_T *enc_stream_info = &stream_ctrl->enc_stream_info;
	VID_INFO* pVid_info = stream_ctrl->pVid_info;
	H264Enc_t*pH264Enc = &pVid_info->h264_rec_args.h264_cfg;
	FB_CFG* pH264OutCfg_SKIP = &pVid_info->h264_rec_args.fb_h264_out_SKIP_cfg;
	INT32S status;
	FILE_PACKER_W_ARGS File_packer_args;
	FB_CFG* fb_h264_out_cfg;
	INT32S pts;
	INT32U drop_pts;

	#if (C_VID_INTERPOLATING_2K) && (C_SUPORT_GET_JPG_BUF)
	GET_JPEG_ARGS* pGet_jpeg_args = &pVid_info->get_jpeg_args;
	#endif

	//DBG_PRINT("==Run:%s==[%d]==\r\n",__FUNCTION__,src);

	if(!pH264Enc->handle)
	{
		//ERR_PRINT("[ERR] pH264Enc->handle is null\r\n");
		return STATUS_OK;
	}

	if(!pH264Enc->flag_h264_trigger)
	{// Don't clear encode status
		return STATUS_SKIP;
	}

	if (pH264Enc->PictureType == H264_FRAME_TYPE_I) {					// 0=H264_FRAME_TYPE_I, 1=H264_FRAME_TYPE_P
		// I-frame
        fb_h264_out_cfg = &pVid_info->h264_rec_args.fb_h264_out_I_cfg;
         //DBG_PRINT("\r\n I");
    } else {
    	// P-frame
        fb_h264_out_cfg = &pVid_info->h264_rec_args.fb_h264_out_P_cfg;
    }

	pH264Enc->encode_size = 0;
	status = gp_h264enc_wait(pH264Enc->handle, 500, (INT32S *)&pH264Enc->encode_size);
	if(status < 0) {
		gp_h264enc_ratecontrol_set(pH264Enc->handle, fb_h264_out_cfg->fb_cfg_size);
		gp_h264enc_reset(pH264Enc->handle);	
		ERR_PRINT("[ERR] H264 Status: %d\r\n",status);
		RETURN(STATUS_FAIL);
	} else if (status & STATUS_H264_FIFO_OUT_END) {
		// Workaround: Missing H264 data due to hardware Interrupt lost
		gp_h264enc_ratecontrol_set(pH264Enc->handle, fb_h264_out_cfg->fb_cfg_size);
		gp_h264enc_reset(pH264Enc->handle);
		DBG_PRINT("=>STATUS_H264_FIFO_OUT_END\r\n");

		#if (C_VID_INTERPOLATING_2K)
		interp2K_scale_up_err_handle(src,pH264Enc->input_buffer_idx,pVid_info);
		#endif

		RETURN(STATUS_FAIL);
	} else if (status & STATUS_H264_FIFO_IN_END) {
		//DBG_PRINT("=>STATUS_H264_FIFO_IN_END\r\n");
		#if (C_VID_INTERPOLATING_2K)
		// Notify the pscaler to restart an output buffer
		if(pVid_info->pscaler_up_start) // Only front sensor enables this variable
		{
			#if C_SUPORT_GET_JPG_BUF
			if(pGet_jpeg_args->flag_start_buf_get && pGet_jpeg_args->flag_start_fifo_buf_get)
			{
				pGet_jpeg_args->flag_fifo_done |= GET_JPEG_DONE_H264;
				if(pGet_jpeg_args->flag_fifo_done != (GET_JPEG_DONE_H264|GET_JPEG_DONE_JPEG))
				{
					return STATUS_OK;
				}
				else
				{
					pGet_jpeg_args->flag_fifo_done = 0;
				}
			}
			#endif

			interp2K_scale_up_restart(src,pH264Enc->input_buffer_idx);
		}
		#endif
		return STATUS_OK;
	}

	#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
	lib_interp2K_frame_check(src);
	#endif

#if STATISTICS_DUMP
	if(src==0){
        front_wr_cnt++;
	}else if(src==1){
        rear_wr_cnt++;
	}else{
		wifi_wr_cnt++;
	}
#endif
	// Clear vid_fifo_start after return STATUS_H264_FRAME_END
	pVid_info->vid_fifo_start = 0;
	pH264Enc->flag_h264_trigger = 0;


	if((pH264Enc->encode_size == 0) || (pH264Enc->encode_size > fb_h264_out_cfg->fb_cfg_size))
	{
		DBG_PRINT("H264 encode over size s=%d f=%d p=%d type=%d src=%d\r\n", pH264Enc->encode_size, enc_stream_info->h264_frame_num, enc_stream_info->h264_pic_order_cnt, pH264Enc->PictureType, src);

		if ((INT32U) gp_h264_get_hierP_Type(pH264Enc->handle) != SLICE_TYPE_HP) {			// 0=SLICE_TYPE_P, 1=SLICE_TYPE_HP, 2=SLICE_TYPE_I, 3=SLICE_TYPE_SKIP
			// This frame is referenced by the following frames. Failed to write into MOV packer will induce error in decoding the following P-frames. Set next frame type to I-frame to avoid this error.
			enc_stream_info->h264_frame_num = 0;
			enc_stream_info->h264_pic_order_cnt = 0;

		}
		lib_free_H264(src, pH264Enc->output_buffer, pH264Enc->PictureType);
		pH264Enc->output_buffer = 0;
		return STATUS_FAIL;
	}

	if (pVid_info->run_ap_mode == STATE_VIDEO_RECORD)
	{
		INT8U time_lapse_flag = 0;
		INT8U insert_skip_flag = 0;
		INT16U adjust_skip_time = 0;
		INT16U skip_frame_width = pH264Enc->cfg.width;

      #if C_TIME_LAPSE
        if (ap_state_config_time_lapse_recording_get()) {
            pts = 1000 / ap_state_config_time_lapse_fps_get();
            time_lapse_flag = 1;
        } else
      #endif
        {
			if (enc_stream_info->frame_rate != pVid_info->frame_rate) {
        		enc_stream_info->frame_rate = pVid_info->frame_rate;
        		enc_stream_info->frame_pts = 1000 / enc_stream_info->frame_rate;
        	}
        	pts = enc_stream_info->frame_pts;
        }

		// First frame must be I-Frame
		if (!enc_stream_info->stream_update_time) {
			if (pH264Enc->PictureType != H264_FRAME_TYPE_I) {			// 0=H264_FRAME_TYPE_I, 1=H264_FRAME_TYPE_P
				RETURN(STATUS_FAIL);
			}
		} else {
			if (enc_stream_info->stream_update_time > enc_stream_info->yuv_update_time + (pts>>1) + (pts>>2)) {
				// Drop frames that come too early
				if(src==0)
                    DBG_PRINT("D");
                else if(src==1)
                    DBG_PRINT("d");

				if ((INT32U) gp_h264_get_hierP_Type(pH264Enc->handle) != SLICE_TYPE_HP) {			// 0=SLICE_TYPE_P, 1=SLICE_TYPE_HP, 2=SLICE_TYPE_I, 3=SLICE_TYPE_SKIP
					// This frame is referenced by the following frames. Failed to write into MOV packer will induce error in decoding the following P-frames. Set next frame type to I-frame to avoid this error.
					enc_stream_info->h264_frame_num = 0;
					enc_stream_info->h264_pic_order_cnt = 0;
				}
				lib_free_H264(src, pH264Enc->output_buffer, pH264Enc->PictureType);
				pH264Enc->output_buffer = 0;

				return STATUS_FAIL;

			} else if (enc_stream_info->stream_update_time + (pts>>1) < enc_stream_info->yuv_update_time) {
				// Don't insert skip frame if we are recording a time lapse video
				if (!time_lapse_flag && (enc_stream_info->h264_previous_frame_type != SLICE_TYPE_HP) && enc_stream_info->h264_pic_order_cnt) {
					// Insert skip frame
					insert_skip_flag = 1;
				  #if 0		// Enable this if you don't want to insert skip frame in front of I-frame
					if (pH264Enc->PictureType == H264_FRAME_TYPE_I) {	// 0=H264_FRAME_TYPE_I, 1=H264_FRAME_TYPE_P
						insert_skip_flag = 0;
					}
				  #endif
				}
			}
		}

		File_packer_args.src = src;
		File_packer_args.snapshot = 0;
		File_packer_args.track = MOV_TRACK_VID_RECORD;
		File_packer_args.type = VID_REC_H264;
		File_packer_args.drop_time = &drop_pts;						// Not used

		if (insert_skip_flag) {
			// Inser skip frame here
			INT32U packer_skip_buf;
			INT8U update_pic_order_cnt = 0;
			INT32S pts_skip;

			File_packer_args.Dummy_frame_cnt = 1;
			File_packer_args.frameType = 2; 	// 0=H264_FRAME_TYPE_I, 1=H264_FRAME_TYPE_P, 2=Skip frame(not used by H264 engine, only for packer and cbuf_encoder_out[] management)
			File_packer_args.size = pH264OutCfg_SKIP->fb_cfg_size;
			File_packer_args.pts = pts_skip = pts;

			while (enc_stream_info->stream_update_time + (pts>>1) < enc_stream_info->yuv_update_time) {
			  #if 0			// Enable this if you don't want to insert skip frame when (I+P+Skip) is larger than GOP(I+P)
				if (enc_stream_info->h264_pic_order_cnt+1 >= pH264Enc->rccfg.gopLen) {
					break;
				}
			  #endif
				if (enc_stream_info->h264_pic_order_cnt > 250) {
					break;
				}

				if (circular_buf_get(cbuf_encoder_out[src][ENC_H264SKIP_IDX], (INT32U *) &packer_skip_buf) < 0) {
					no_encode_out_cnt[src][ENC_H264SKIP_IDX]++;
					if (no_encode_out_cnt[src][ENC_H264SKIP_IDX]<16 || (no_encode_out_cnt[src][ENC_H264SKIP_IDX]&0xFF)==0) {
						DBG_PRINT("Failed to get skip circular buffer(%d, %d)\r\n", src, no_encode_out_cnt[src][ENC_H264SKIP_IDX]);
					}
					break;
				}
				cache_invalid_range((INT32U) packer_skip_buf, File_packer_args.size);			// 160 bytes
				if (sizeof(h264_skip_frame) >= File_packer_args.size) {
					gp_memcpy((void *) packer_skip_buf, (void *) &h264_skip_frame[0], File_packer_args.size);
				} else {
					gp_memcpy((void *) packer_skip_buf, (void *) &h264_skip_frame[0], sizeof(h264_skip_frame));
				}
				// Insert end of frame here so that iOS can playback this skip frame correctly
				if (skip_frame_width <= 1280) {				// 1280x720
					*((INT8U *) packer_skip_buf+0x22) = 0x01;	
					*((INT8U *) packer_skip_buf+0x23) = 0xFE;
				} else if (skip_frame_width <= 1440) {		// 1440x1080
					*((INT8U *) packer_skip_buf+0x34) = 0x0C;
					*((INT8U *) packer_skip_buf+0x35) = 0xF8;
				} else if (skip_frame_width <= 1920) {		// 1920x1080
					*((INT8U *) packer_skip_buf+0x42) = 0x01;
					*((INT8U *) packer_skip_buf+0x43) = 0x17;
				} else if (skip_frame_width <= 2560) {		// 2560x1440
					*((INT8U *) packer_skip_buf+0x6D) = 0x01;
					*((INT8U *) packer_skip_buf+0x6E) = 0xFE;
				}

				File_packer_args.ret_data = packer_skip_buf;
				File_packer_args.data = (INT8U *) packer_skip_buf;
				h264_set_reverse_number((INT8U *) packer_skip_buf+5, enc_stream_info->h264_frame_num);
				h264_set_reverse_number((INT8U *) packer_skip_buf+6, enc_stream_info->h264_pic_order_cnt);
				if (enc_stream_info->yuv_update_time - enc_stream_info->stream_update_time < pts) {
					File_packer_args.pts = pts_skip = enc_stream_info->yuv_update_time - enc_stream_info->stream_update_time;
				}
				if (pts_skip > 8) {
					File_packer_args.pts = 8;				// Reduce skip frame time to 8ms
					adjust_skip_time += pts_skip - 8;		// Record reduced time so that it can be added to real H264 frame
				}
				if (file_packer_write_notify(&File_packer_args) == STATUS_OK) {
					enc_stream_info->pack_frame_cnt++;
					enc_stream_info->stream_update_time += pts_skip;
					enc_stream_info->h264_previous_frame_type = SLICE_TYPE_SKIP;
					enc_stream_info->h264_pic_order_cnt++;
					update_pic_order_cnt++;

					#if STATISTICS_DUMP
                    if(src == 0){
                        skip_front_wr_cnt++;
                    }else if(src == 1){
                        skip_rear_wr_cnt++;
                    }
                    #endif

					if (pts_skip != pts) {
						pts += (pts-pts_skip);
						break;
					}

				  #if 1		// Break here if we don't allow too many consecutive skip frame
					if (update_pic_order_cnt >= 5) {
						break;
					}
				  #endif
				} else {
					DBG_PRINT("Failed to send skip frame to MOV packer\r\n");

					circular_buf_try_put(cbuf_encoder_out[src][ENC_H264SKIP_IDX], packer_skip_buf);
					break;
				}
			}

			if (update_pic_order_cnt && (pH264Enc->PictureType != H264_FRAME_TYPE_I)) {			// 0=H264_FRAME_TYPE_I, 1=H264_FRAME_TYPE_P
				h264_set_reverse_number((INT8U *) pH264Enc->output_buffer + 6, enc_stream_info->h264_pic_order_cnt);
			}
		}

		if (enc_stream_info->stream_start_time && (!time_lapse_flag)) {
			INT32U ideal_time, current_time;
			INT8U max_cnt = 5;		// Add 5ms at most

			ideal_time = 1000*(enc_stream_info->pack_frame_cnt+1);
			current_time = (enc_stream_info->stream_update_time-enc_stream_info->stream_start_time+pts) * enc_stream_info->frame_rate;
			// If video stream is latter than ideal frame time, add 1ms to pts. For example, fps=30, frame display time=1000ms/30=33.3333ms. Set pts to 33ms or (33+1)ms
			while ((current_time < ideal_time) && max_cnt) {
				current_time += enc_stream_info->frame_rate;
				pts++;
				max_cnt--;
			}
		}
		File_packer_args.Dummy_frame_cnt = 0;
		File_packer_args.frameType = pH264Enc->PictureType;			// 0=H264_FRAME_TYPE_I, 1=H264_FRAME_TYPE_P, 2=Skip frame(not used by H264 engine, only for packer and cbuf_encoder_out[] management)
		File_packer_args.ret_data = pH264Enc->output_buffer;
		File_packer_args.data = (INT8U *) pH264Enc->output_buffer;
		File_packer_args.size = pH264Enc->encode_size;
		File_packer_args.pts = pts;									// Unit is ms
		File_packer_args.pts += adjust_skip_time;					// Add reduced skip frame time to real H264 frame here

		nRet = file_packer_write_notify(&File_packer_args);
		if (nRet == STATUS_OK) {
			enc_stream_info->pack_frame_cnt++;

			if (!enc_stream_info->stream_update_time) {
				enc_stream_info->stream_start_time = enc_stream_info->yuv_update_time;
				enc_stream_info->stream_update_time = enc_stream_info->stream_start_time + pts;
			} else {
				enc_stream_info->stream_update_time += pts;
			}

			enc_stream_info->h264_previous_frame_type = gp_h264_get_hierP_Type(pH264Enc->handle);	// 0=SLICE_TYPE_P, 1=SLICE_TYPE_HP, 2=SLICE_TYPE_I, 3=SLICE_TYPE_SKIP
			if (pH264Enc->PictureType == H264_FRAME_TYPE_I) {		// 0=H264_FRAME_TYPE_I, 1=H264_FRAME_TYPE_P
				enc_stream_info->h264_idr_pic_id ^= 0x1;			// Set idr_pic_id of next I-frame
				enc_stream_info->h264_frame_num = 1;
				enc_stream_info->h264_pic_order_cnt = 1;

			} else {
				if (enc_stream_info->h264_previous_frame_type != SLICE_TYPE_HP) {
					enc_stream_info->h264_frame_num++;
				}
				enc_stream_info->h264_pic_order_cnt++;
			}

		} else {		// STATUS_FAIL
			if ((INT32U) gp_h264_get_hierP_Type(pH264Enc->handle) != SLICE_TYPE_HP) {				// 0=SLICE_TYPE_P, 1=SLICE_TYPE_HP, 2=SLICE_TYPE_I, 3=SLICE_TYPE_SKIP
				// This frame is referenced by the following frames. Failed to write into MOV packer will induce error in decoding the following P-frames. Set next frame type to I-frame to avoid this error.
				DBG_PRINT("Fail to pack(f=%d p=%d type=%d), set f & p to 0\r\n", enc_stream_info->h264_frame_num, enc_stream_info->h264_pic_order_cnt, pH264Enc->PictureType);
				enc_stream_info->h264_frame_num = 0;
				enc_stream_info->h264_pic_order_cnt = 0;
			}
		}
	}
  #if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	else if (pVid_info->run_ap_mode == STATE_WIFI_TRANSMISSION)
	{
		pVid_info->frame_rate = WIFI_REC_FRAME_RATE;
		// First frame must be I-Frame
		if (!enc_stream_info->stream_update_time && (pH264Enc->PictureType != H264_FRAME_TYPE_I)) {	// 0=H264_FRAME_TYPE_I, 1=H264_FRAME_TYPE_P
			DBG_PRINT("<T=%d> \r\n", enc_stream_info->stream_update_time );
			RETURN(STATUS_FAIL);
		} else {
			if (!enc_stream_info->stream_update_time || (enc_stream_info->yuv_update_time <= enc_stream_info->stream_update_time)) {
		        if (enc_stream_info->frame_rate != pVid_info->frame_rate) {
	        		enc_stream_info->frame_rate = pVid_info->frame_rate;
	        		enc_stream_info->frame_pts = 1000 / enc_stream_info->frame_rate;
	        	}
	        	pts = enc_stream_info->frame_pts;
				DBG_PRINT("<pts=%d> {fps:%d,%d}\r\n",pts,enc_stream_info->frame_rate, pVid_info->frame_rate);
			} else {
				pts = enc_stream_info->yuv_update_time - enc_stream_info->stream_update_time;
			}

			nRet = rtsp_h264_send(pH264Enc->output_buffer, pH264Enc->encode_size, pts, pH264Enc->PictureType, wifi_stream_buf_release);
			if (nRet == STATUS_OK) {
				enc_stream_info->stream_update_time = enc_stream_info->yuv_update_time;
				if (pH264Enc->PictureType == H264_FRAME_TYPE_I) {		// 0=H264_FRAME_TYPE_I, 1=H264_FRAME_TYPE_P
					enc_stream_info->h264_idr_pic_id ^= 0x1;			// Set idr_pic_id of next I-frame
					enc_stream_info->h264_frame_num = 1;
					enc_stream_info->h264_pic_order_cnt = 1;
				} else {
					if ((INT32U) gp_h264_get_hierP_Type(pH264Enc->handle) != SLICE_TYPE_HP) {			// 0=SLICE_TYPE_P, 1=SLICE_TYPE_HP, 2=SLICE_TYPE_I, 3=SLICE_TYPE_SKIP
						enc_stream_info->h264_frame_num++;
					}
					enc_stream_info->h264_pic_order_cnt++;
				}
			} else {		// STATUS_FAIL
				if ((INT32U) gp_h264_get_hierP_Type(pH264Enc->handle) != SLICE_TYPE_HP) {				// 0=SLICE_TYPE_P, 1=SLICE_TYPE_HP, 2=SLICE_TYPE_I, 3=SLICE_TYPE_SKIP
					enc_stream_info->h264_frame_num = 0;
					enc_stream_info->h264_pic_order_cnt = 0;
				}
			}
		}
	}
  #endif
  #if (defined C_USB_DEVICE) && (C_USB_DEVICE)
	else if (pVid_info->run_ap_mode == STATE_CONNECT_TO_PC)
	{
		// Send H264 bit-stream to UVC instead of setting fail here
		nRet = STATUS_FAIL;
	}
  #endif
	else
	{
		nRet = STATUS_FAIL;
	}

	if (nRet != STATUS_OK)
	{
		//DBG_PRINT("h264-w src:%d, output_buffer:0x%x, encode_size:%d PictureType:%d\r\n",src,pH264Enc->output_buffer,pH264Enc->encode_size,pH264Enc->PictureType);
		lib_free_H264(src,pH264Enc->output_buffer,pH264Enc->PictureType);
		pH264Enc->output_buffer = 0;
	}

	return STATUS_OK;

Return:
	lib_free_H264(src,pH264Enc->output_buffer,pH264Enc->PictureType);
	pH264Enc->output_buffer = 0;
	pH264Enc->flag_h264_trigger = 0;

	enc_stream_info->h264_frame_num = 0;			// Restarting encoding I-Frame
	enc_stream_info->h264_pic_order_cnt = 0;		// Restarting encoding I-Frame

	pVid_info->vid_fifo_start = 0;

	return nRet;
}

INT32S
lib_H264_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	ENC_STREAM_CTRL_T *enc_stream_ctrl = pPscalerStreamCtrl->enc_stream_ctrl;

	if(enc_stream_ctrl && enc_stream_ctrl->stream_write_func)
	{
		nRet = enc_stream_ctrl->stream_write_func(src,(void*)pPscalerStreamCtrl);
		if(nRet == STATUS_SKIP)
		{
			return STATUS_OK;
		}
	}

	// Clear encode status to do the next
	lib_encode_engine_status_clear(src,pVid_info);

	return nRet;
}

/*=== Photo Capture ====*/
INT32S
lib_photo_ready_check(
	INT32U src,
	INT32U* pDevCnt
)
{
	PSCALER_STREAM_CTRL_T* pPsaclerFront = &pscaler_stream[PREVIEW_SRC_FRONT];
	ENC_STREAM_INFO_T *enc_stream_info_front = &pPsaclerFront->enc_stream_info;

	PSCALER_STREAM_CTRL_T* pPsaclerRear = &pscaler_stream[PREVIEW_SRC_REAR];
	ENC_STREAM_INFO_T *enc_stream_info_rear = &pPsaclerRear->enc_stream_info;
	VID_INFO* pVid_info_rear = pPsaclerRear->pVid_info;

	switch(src)
	{
		case PREVIEW_SRC_FRONT:
			ENCODE_STATUS_SET_BUSY(enc_stream_info_front->stream_status,VIDEO_ENCODE_STATUS_PHOTO_FRAME_READY_OFFSET);
		break;

		case PREVIEW_SRC_REAR:
			ENCODE_STATUS_SET_BUSY(enc_stream_info_rear->stream_status,VIDEO_ENCODE_STATUS_PHOTO_FRAME_READY_OFFSET);
		break;
	}

	if(back_sensor_plug_status_get() &&
	   pVid_info_rear
	 )
	{
		if(ENCODE_STATUS_IS_BUSY(enc_stream_info_front->stream_status,VIDEO_ENCODE_STATUS_PHOTO_FRAME_READY_OFFSET) &&
			ENCODE_STATUS_IS_BUSY(enc_stream_info_rear->stream_status,VIDEO_ENCODE_STATUS_PHOTO_FRAME_READY_OFFSET))
		{
			if(pDevCnt)
			{
				*pDevCnt = 2;
			}
			return STATUS_OK;
		}
	}
	else
	{
		if(ENCODE_STATUS_IS_BUSY(enc_stream_info_front->stream_status,VIDEO_ENCODE_STATUS_PHOTO_FRAME_READY_OFFSET))
		{
			if(pDevCnt)
			{
				*pDevCnt = 1;
			}
			return STATUS_OK;
		}
	}

	return STATUS_FAIL;
}

void
lib_photo_ready_clear(
	void
)
{
	PSCALER_STREAM_CTRL_T* pPsaclerFront = &pscaler_stream[PREVIEW_SRC_FRONT];
	ENC_STREAM_INFO_T *enc_stream_info_front = &pPsaclerFront->enc_stream_info;
	//VID_INFO* pVid_info_front = pPsaclerFront->pVid_info;

	PSCALER_STREAM_CTRL_T* pPsaclerRear = &pscaler_stream[PREVIEW_SRC_REAR];
	ENC_STREAM_INFO_T *enc_stream_info_rear = &pPsaclerRear->enc_stream_info;
	VID_INFO* pVid_info_rear = pPsaclerRear->pVid_info;


	if(pVid_info_rear)
	{
		ENCODE_STATUS_CLR_BUSY(enc_stream_info_front->stream_status,VIDEO_ENCODE_STATUS_PHOTO_FRAME_READY_OFFSET);
		ENCODE_STATUS_CLR_BUSY(enc_stream_info_rear->stream_status,VIDEO_ENCODE_STATUS_PHOTO_FRAME_READY_OFFSET);
	}
	else
	{
		ENCODE_STATUS_CLR_BUSY(enc_stream_info_front->stream_status,VIDEO_ENCODE_STATUS_PHOTO_FRAME_READY_OFFSET);
	}

}

static void
photo_capture_mem_free(
	unsigned char src
)
{
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	JpegEnc_t* pJpegEnc = &pVid_info->jpeg_rec_args.jpeg_cfg;
	FB_CFG* pPscalerOutCfg = &pVid_info->jpeg_rec_args.fb_pscaler_out_cfg;
	FB_CFG* pJpegOutCfg = &pVid_info->jpeg_rec_args.fb_jpeg_out_cfg;
	FB_CFG* pMsgNotifyCfg = &pVid_info->jpeg_rec_args.fb_msg_notify_cfg;
	INT8U data_source = VID_SRC_NONE;

	if(pVid_info)
	{
		data_source = pVid_info->data_source;
	}

	//DBG_PRINT("[%s]src:%d data_source:%d\r\n",__FUNCTION__,src,data_source);

	if(data_source == VID_SRC_USB)
	{
		//DBG_PRINT("input_buffer_v:0x%x\r\n",pJpegEnc->input_buffer_v);
		if(pJpegEnc->input_buffer_v)
		{
			lib_pscaler_free_bypass(src,pJpegEnc->input_buffer_v);
		}
	}
	else
	{
		// Pscaler bypass
		if(pPscalerOutCfg->fb_cfg_head_addr)
		{
			#if C_VID_REC_USE_SYS_FIFO
			pPscalerOutCfg->fb_cfg_head_addr = (void *)lib_encode_system_fifo_unmapping(src, (INT32U)pPscalerOutCfg->fb_cfg_head_addr);
			#endif
			gp_free(pPscalerOutCfg->fb_cfg_head_addr);
			pPscalerOutCfg->fb_cfg_head_addr = 0;
		}
		if(pPscalerOutCfg->fb_cfg_cb_addr)
		{
			gp_free(pPscalerOutCfg->fb_cfg_cb_addr);
			pPscalerOutCfg->fb_cfg_cb_addr = 0;
		}
	}

	// Jpeg
	if(pJpegOutCfg->fb_cfg_head_addr)
	{
		gp_free(pJpegOutCfg->fb_cfg_head_addr);
		pJpegOutCfg->fb_cfg_head_addr = 0;
	}
	if(pJpegOutCfg->fb_cfg_cb_addr)
	{
		gp_free(pJpegOutCfg->fb_cfg_cb_addr);
		pJpegOutCfg->fb_cfg_cb_addr = 0;
	}

	// Msg
	if(pMsgNotifyCfg->fb_cfg_head_addr)
	{
		gp_free(pMsgNotifyCfg->fb_cfg_head_addr);
		pMsgNotifyCfg->fb_cfg_head_addr = 0;
	}

	if(pMsgNotifyCfg->fb_cfg_cb_addr)
	{
		gp_free(pMsgNotifyCfg->fb_cfg_cb_addr);
		pMsgNotifyCfg->fb_cfg_cb_addr = 0;
	}

	// Jpeg driver
	if(pJpegEnc->header)
	{
		gp_free(pJpegEnc->header);
		pJpegEnc->header = 0;
	}

	// Scale up
	if(photo_pscaler_scale_up_buf[src][0])
	{
		gp_free((void *)photo_pscaler_scale_up_buf[src][0]);
		photo_pscaler_scale_up_buf[src][0] = 0;
	}

	jpeg_encode_stop(pJpegEnc->hwidx);

	if(data_source != VID_SRC_USB)
	{
		release_used_jpeg(pJpegEnc->hwidx);
	}

}

static void
ps_scale_up_restart(
	unsigned char src,
	INT32U buf_idx
)
{
	INT32U scaler = preview_src_to_ps(src);

	//DBG_PRINT("[%d] buf:%d\r\n", scaler,buf_idx);

	osENTER_CRITICAL();

	if(scalerOutputStatus[src] == (PIPELINE_SCALER_STATUS_BUF_A_DONE | PIPELINE_SCALER_STATUS_BUF_B_DONE))
	{
		drv_l1_pscaler_output_fifo_restart(scaler);
	}

	if(buf_idx & 0x01)
	{
		scalerOutputStatus[src] &= ~PIPELINE_SCALER_STATUS_BUF_B_DONE;
	}
	else
	{
		scalerOutputStatus[src] &= ~PIPELINE_SCALER_STATUS_BUF_A_DONE;
	}

	osEXIT_CRITICAL();
}

static void
ps_scale_up_isr(
	PS_ID scaler,
	PS_ST PScaler_Event
)
{
	INT8U src = preview_ps_to_src(scaler);
	INT32U fifo_buf_done;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl;

	const INT32U mask_event = PIPELINE_SCALER_STATUS_OVERFLOW_OCCUR |
		PIPELINE_SCALER_STATUS_FRAME_DONE |
		PIPELINE_SCALER_STATUS_BUF_A_DONE |
		PIPELINE_SCALER_STATUS_BUF_B_DONE |
		PIPELINE_SCALER_STATUS_AHB_DONE;


	pPscalerStreamCtrl = &pscaler_stream[src];

	if(!(PScaler_Event & mask_event))
	{
		// debug
		DBG_PRINT("[DEBUG]PS[%d]:other=0x%X\r\n", scaler, PScaler_Event);

		/*
		over_cnt2++;
		if(over_cnt2 >= 30){
			DBG_PRINT("[DEBUG]PS[%d]:other=0x%X\r\n", scaler, PScaler_Event);
			over_cnt2=0;
		} */
	}

	if(PScaler_Event & PIPELINE_SCALER_STATUS_OVERFLOW_OCCUR)
	{
		DBG_PRINT("[TRACE]PS[%d]:overflow\r\n", scaler);
	}

	if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE || PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE)
	{
		if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE)
		{
			scalerOutputStatus[src] |= PIPELINE_SCALER_STATUS_BUF_A_DONE;
			fifo_buf_done = drv_l1_pscaler_output_A_buffer_get(scaler);
			if(pPscalerStreamCtrl->ps_notify_scaleup_func)
			{
				pPscalerStreamCtrl->ps_notify_scaleup_func(src,pPscalerStreamCtrl,fifo_buf_done);
			}
		}

		if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE)
		{
			scalerOutputStatus[src] |= PIPELINE_SCALER_STATUS_BUF_B_DONE;

			fifo_buf_done = drv_l1_pscaler_output_B_buffer_get(scaler);
			if(pPscalerStreamCtrl->ps_notify_scaleup_func)
			{
				pPscalerStreamCtrl->ps_notify_scaleup_func(src,pPscalerStreamCtrl,fifo_buf_done);
			}
		}

		if(scalerOutputStatus[src] != (PIPELINE_SCALER_STATUS_BUF_A_DONE | PIPELINE_SCALER_STATUS_BUF_B_DONE))
		{
			drv_l1_pscaler_output_fifo_restart(scaler);
		}
	}


	if(PScaler_Event & PIPELINE_SCALER_STATUS_AHB_DONE)
	{
		scalerInputDone[src] = 1;
	}

	if(PScaler_Event & PIPELINE_SCALER_STATUS_FRAME_DONE)
	{
		scalerOutputDone[src] = 1;
	}

	if(scalerInputDone[src] && scalerOutputDone[src])
	{
		//DBG_PRINT("=>Done%d\r\n",scaler);
		drv_l1_pscaler_stop(scaler);
	}

}

INT32S
lib_photo_scale_up(
	INT32U src,
	INT32U src_buf
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	FB_CFG* pMsgNotifyCfg = pPscalerStreamCtrl->msg_fb_cfg;
	JpegEnc_t*pJpegEnc = &pVid_info->jpeg_rec_args.jpeg_cfg;
	INT32U	inW, inH;
	INT32U	outW, outH;
	INT32U	wFactor, hFactor;
	INT32U scaler = preview_src_to_ps(src);
	INT32U pscaler_scale_up_size;
	INT32U pscaler_out_buf_size;

	pJpegEnc->input_buffer_v = 0;

	/*
		Put pscaler's buffer into input_buffer_v in USB_HOST_CAM mode
		Release pscaler's buffer after JPEG done
	*/
	if(pVid_info->data_source == VID_SRC_USB)
	{
		pJpegEnc->input_buffer_v = src_buf;
	}

	#if C_VID_REC_USE_SYS_FIFO
	if(pPscalerStreamCtrl->flag_sys_fifo)
	{
		src_buf = lib_encode_system_fifo_unmapping(src,src_buf);
	}
	#endif

	// Add date/time stamps
	if(pVid_info->add_time_stamp)
	{
		ap_state_resource_time_stamp_position_y_set(-64);

		pscaler_out_buf_size = pVid_info->jpeg_rec_args.fb_pscaler_out_cfg.fb_cfg_size;
		cache_invalid_range(src_buf,pscaler_out_buf_size);
		cpu_draw_time_osd(src_buf, pVid_info->sensor_width, pVid_info->sensor_height, STATE_PHOTO_CAPTURE, 0, YUV420_DRAW, 0);
	}

	/*== E X I F ==*/
	#if (defined C_PHOTO_JPEG_ADD_EXIF) && (C_PHOTO_JPEG_ADD_EXIF)
	lib_photo_exif_add(src,src_buf,pVid_info);
	#endif	

	// Set to 0 befor encoding jpeg
	pMsgNotifyCfg->fb_cfg_count_num = 0;
	scalerOutputDone[src] = 0;
	scalerInputDone[src] = 0;
	scalerOutputStatus[src] = 0;

	// Set param
	inW = pVid_info->sensor_width;
	inH = pVid_info->sensor_height;
	outW = pVid_info->target_width;
	outH = pVid_info->target_height;
    DBG_PRINT("PHOTO:inW=%d,inH=%d,outW=%d,outH=%d\r\n",inW,inH,outW,outH);
	drv_l1_pscaler_init(scaler);
	drv_l1_pscaler_input_source_set(scaler, PIPELINE_SCALER_INPUT_SOURCE_DRAM);
	drv_l1_pscaler_input_buffer_set(scaler, src_buf);

	pscaler_scale_up_size = outW*PHOTO_PSCALER_SCALE_UP_LINE_CNT;

	switch(pVid_info->data_format)
	{
		case VID_DATA_422:
			drv_l1_pscaler_input_format_set(scaler, PIPELINE_SCALER_INPUT_FORMAT_YUYV);
			pscaler_scale_up_size+= (outW*PHOTO_PSCALER_SCALE_UP_LINE_CNT);
		break;
		case VID_DATA_420:
			drv_l1_pscaler_input_format_set(scaler, PIPELINE_SCALER_INPUT_FORMAT_GP420);
			pscaler_scale_up_size+= (outW*PHOTO_PSCALER_SCALE_UP_LINE_CNT)>>1;
		break;
	}

	// Set jpeg encode input line size
	pJpegEnc->input_y_len = pscaler_scale_up_size;

	photo_pscaler_scale_up_buf[src][0] = (INT32U)gp_malloc_align(pscaler_scale_up_size << 1, 64); // For Buffer A/B
	if(photo_pscaler_scale_up_buf[src][0] == 0)
	{
		ERR_PRINT("[ERROR]photo_pscaler_scale_up_buf[%d] is null \r\n",src);
		RETURN(STATUS_FAIL);
	}

	photo_pscaler_scale_up_buf[src][1] = photo_pscaler_scale_up_buf[src][0]+pscaler_scale_up_size;

	drv_l1_pscaler_output_format_set(scaler, PIPELINE_SCALER_OUTPUT_FORMAT_GP420);

	wFactor = (((inW - 1) << 16) / (outW - 1));
	hFactor = (((inH - 1) << 16) / (outH - 1));

	drv_l1_pscaler_pixels_set(scaler, inW, inH, outW, outH, wFactor, hFactor);

	drv_l1_pscaler_output_fifo_line_set(scaler, PHOTO_PSCALER_SCALE_UP_LINE_CNT, 1);

//	DBG_PRINT("[ScaleUp]Pscaler BufA: 0x%x\r\n",photo_pscaler_scale_up_buf[src][0]);
//	DBG_PRINT("[ScaleUp]Pscaler BufB: 0x%x\r\n",photo_pscaler_scale_up_buf[src][1]);

	drv_l1_pscaler_output_A_buffer_set(scaler, photo_pscaler_scale_up_buf[src][0]);
	drv_l1_pscaler_output_B_buffer_set(scaler, photo_pscaler_scale_up_buf[src][1]);

	drv_l1_pscaler_callback_register2(scaler, ps_scale_up_isr);

	drv_l1_pscaler_interrupt_enable(scaler, 1);
	drv_l1_pscaler_start(scaler);

	return STATUS_OK;

Return:

	return nRet;
}

INT32S
front_photo_buf_set(
	MSG_Q_ID notifyQ,
	VID_INFO* args
)
{
	return(lib_encode_buf_set(PREVIEW_SRC_FRONT,notifyQ,args));
}

INT32S
front_photo_start(
	VID_INFO* args
)
{
	return(lib_encode_start(PREVIEW_SRC_FRONT,args));
}

INT32S
rear_photo_buf_set(
	MSG_Q_ID notifyQ,
	VID_INFO* args
)
{
	return(lib_encode_buf_set(PREVIEW_SRC_REAR,notifyQ,args));
}

INT32S
rear_photo_start(
	VID_INFO* args
)
{
	return(lib_encode_start(PREVIEW_SRC_REAR,args));
}

static void
lib_photo_JPEG_Callback(
	JPEG_IDX idx,
	INT32S event,
	INT32U count
)
{
	PSCALER_STREAM_CTRL_T* pPsaclerStream = NULL;
	VID_INFO* pVid_info  = NULL;
	JpegEnc_t*pJpegEnc  = NULL;
	INT8U i;

	//DBG_PRINT("idx:evt=%d:0x%x\r\n",idx,event);

	// Check the handle
	for(i=PREVIEW_SRC_FRONT; i<=PREVIEW_SRC_REAR; i++)
	{
		pPsaclerStream = &pscaler_stream[i];
		pVid_info = pPsaclerStream->pVid_info;

		if(!pVid_info) // Source path does not exists
		{
			continue;
		}

		pJpegEnc = &pVid_info->jpeg_rec_args.jpeg_cfg;

		if(pJpegEnc->hwidx == idx)
		{
			pJpegEnc->jpeg_status |= event;

			if(event & C_JPG_STATUS_ENCODE_DONE)
			{
				pJpegEnc->encode_size = count;
			}

			if(msgQSend(pPsaclerStream->ps_notify_q, (MSG_VID_PHOTO_JPEG_NOTIFY|i), NULL, 0, MSG_PRI_NORMAL) < 0)
			{
				ERR_PRINT("fail to msgQSend [%d]\r\n",i);
			}
		}
	}
}

static INT32S
lib_photo_JPEG_outbuf_get(
	unsigned char src,
	void* p
)
{
	VID_INFO* pVid_info = ((PSCALER_STREAM_CTRL_T*)p)->pVid_info;
	JPEG_REC_ARGS*	pJpeg_rec_args = &pVid_info->jpeg_rec_args;
	JpegEnc_t*pJpegEnc =  &pJpeg_rec_args->jpeg_cfg;

	/*
		do photo_jpeg_outbuf_change when first time or jpeg output full
	*/
	if(pJpeg_rec_args->photo_jpeg_outbuf_change)
	{
		// Get jpeg encode output buffer
		if(circular_buf_get(cbuf_encoder_out[src][ENC_H264I_JPEG_IDX],((INT32U*)&pJpegEnc->output_buffer)) < 0)
		{
			ERR_PRINT("fail to get cbuf_encoder_out==[%d]\r\n",src);
			return STATUS_FAIL;
		}

		if(pJpegEnc->output_buffer == 0)
		{
			ERR_PRINT("pJpegEnc->output_buffer is null ==[%d]\r\n",src);
			return STATUS_FAIL;
		}

		//DBG_PRINT("pJpegEnc->output_buffer:0x%x \r\n",pJpegEnc->output_buffer);

		pJpeg_rec_args->photo_jpeg_outbuf_change = 0;

		// Debug: dump memory to check jpeg
		#if 0
		gp_memcpy(pJpegEnc->output_buffer,pJpegEnc->header,jpeg_header_get_size());
		pJpegEnc->output_buffer+= jpeg_header_get_size();
		#endif
	}

	return STATUS_OK;
}

static INT32S
lib_photo_JPEG_output_restart(
	unsigned char src,
	void* p
)
{
	VID_INFO* pVid_info = ((PSCALER_STREAM_CTRL_T*)p)->pVid_info;
	JPEG_REC_ARGS*	pJpeg_rec_args = &pVid_info->jpeg_rec_args;
	JpegEnc_t*pJpegEnc =  &pJpeg_rec_args->jpeg_cfg;

	drv_l1_jpeg_vlc_output_restart(pJpegEnc->hwidx,pJpegEnc->output_buffer,pJpegEnc->output_buffer_len);

	return STATUS_OK;
}

static INT32S
lib_photo_JPEG(
	unsigned char src,
	void* p,
	FB_INFO* fb_info
)
{
	INT32S nRet = STATUS_OK;
	VID_INFO* pVid_info = ((PSCALER_STREAM_CTRL_T*)p)->pVid_info;
	JPEG_REC_ARGS*	pJpeg_rec_args = &pVid_info->jpeg_rec_args;
	FB_CFG* pfb_jpeg_out_cfg = &pJpeg_rec_args->fb_jpeg_out_cfg;
	JpegEnc_t*pJpegEnc =  &pJpeg_rec_args->jpeg_cfg;

	pJpegEnc->input_buffer_y = fb_info->buf;
	pfb_jpeg_out_cfg->fb_cfg_count_num =  fb_info->idx;

	// Get jpeg encode output buffer
	if(lib_photo_JPEG_outbuf_get(src,p) == STATUS_FAIL)
	{
		RETURN(STATUS_FAIL);
	}

	if(!jpeg_encode_start_get(pJpegEnc->hwidx))
	{
		lib_jpeg_setting(pJpegEnc);
		DBG_PRINT("[%d]JPEG encode:[%d,%d]\r\n",pJpegEnc->hwidx,pJpegEnc->width,pJpegEnc->height);
		nRet = jpeg_encode_input_size_set(pJpegEnc->hwidx, pJpegEnc->width, pJpegEnc->height);
		if(nRet < 0)
		{
			ERR_PRINT("fail to jpeg_encode_input_size_set\r\n");
			RETURN(STATUS_FAIL);
		}

		jpeg_encode_input_format_set(pJpegEnc->hwidx, pJpegEnc->yuv_fmt);

		switch(pJpegEnc->yuv_sample_fmt)
		{
		case JPEG_YUV422:
			nRet = jpeg_encode_yuv_sampling_mode_set(pJpegEnc->hwidx, C_JPG_CTRL_YUV422);
			break;

		case JPEG_YUV420:
			nRet = drv_l1_jpeg_yuv_sampling_mode_set(pJpegEnc->hwidx, C_JPG_CTRL_GP420 | C_JPG_CTRL_YUV420);
			break;

		default:
			nRet = STATUS_FAIL;
		}
		if(nRet < 0)
		{
			ERR_PRINT("fail to drv_l1_jpeg_yuv_sampling_mode_set\r\n");
			RETURN(STATUS_FAIL);
		}
        DBG_PRINT("JPEG encode scaler:[%d]\r\n",pJpegEnc->scale_mode);
		switch(pJpegEnc->scale_mode)
		{
		case JPEG_SCALE_VX2:
			drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_VX2);
			break;
		case JPEG_SCALE_X2:
			drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_X2);
			break;
		case JPEG_SCALE_YUYV_X4:
			drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_X4);
			break;
		case JPEG_SCALE_GP420_X3D2:
			drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_X1_5);
			break;
		case JPEG_SCALE_YUYV_X9D8:
			drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_X9D8);
			break;
		default:
			break;
		}
        DBG_PRINT("JPEG encode output:[0x%x]\r\n",pJpegEnc->output_buffer);
		nRet = jpeg_encode_output_addr_set(pJpegEnc->hwidx, pJpegEnc->output_buffer);
		if(nRet < 0) {
			ERR_PRINT("fail to jpeg_encode_output_addr_set\r\n");
			RETURN(STATUS_FAIL);
		}

		jpeg_encode_maximum_length_set(pJpegEnc->hwidx, pJpegEnc->output_buffer_len);

		jpeg_encode_isr_callback_set(pJpegEnc->hwidx,lib_photo_JPEG_Callback);
	}

	// Start the jpeg engine
	jpeg_encode_on_the_fly_start(pJpegEnc->hwidx,pJpegEnc->input_buffer_y,0,0,pJpegEnc->input_y_len);

	//DBG_PRINT("S");

	return STATUS_OK;
Return:

	// Put jpeg encode output buffer back to circular buf if an error occurred
	if(pJpegEnc->output_buffer)
	{
		if(circular_buf_try_put(cbuf_encoder_out[src][ENC_H264I_JPEG_IDX],pJpegEnc->output_buffer) < 0)
		{
			ERR_PRINT("fail to lib_encode_JPEG put cbuf_encoder_out==[%d]\r\n",src);
		}
	}
	return nRet;
}

static INT32S
lib_photo_process(
	INT32U src
)
{
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	INT32S nRet = STATUS_OK;
	INT32U data;
	FB_INFO* ps_fb_info;
	INT8U i;

	// Wait for pscaler's bypass output buffer to put into cbuf_notify_enc
	if(lib_photo_ready_check(src,&photo_devCnt) ==  STATUS_FAIL)
	{
		return STATUS_OK;
	}

	if(sensor_preview_wait_stop() != STATUS_OK)
	{
		//ERR_PRINT("[ERROR] preview stop fail \r\n");
		if(msgQSend(pPscalerStreamCtrl->ps_notify_q, (MSG_VID_PSCALER_NOTIFY|src), NULL, 0, MSG_PRI_NORMAL) < 0)
		{
			ERR_PRINT("fail to msgQSend [%d]\r\n",src);
		}
		return STATUS_OK;
	}

	lib_photo_ready_clear();

	for(i=0; i<photo_devCnt; i++)
	{
		/*
			Photo Capture: Get pscaler output buffer for scaling up
		*/
		if(circular_buf_get(cbuf_notify_enc[i],((INT32U*)&data)) < 0)
		{
			ERR_PRINT("fail to get FB_INFO[%d]\r\n",src);
			RETURN(STATUS_FAIL);
		}

		ps_fb_info = (FB_INFO*)data;

		//DBG_PRINT("[process] pscaler buf:0x%x,idx:%d\r\n",ps_fb_info->buf,ps_fb_info->idx);
		if(ps_fb_info->buf == DUMMY_BUFFER_ADDRS)
		{
			RETURN(STATUS_FAIL);
		}

		// Init pscaler an output frame for scaling up
		nRet = lib_photo_scale_up(i,ps_fb_info->buf);
		if(nRet == STATUS_FAIL)
		{
			RETURN(STATUS_FAIL);
		}
	}

	return STATUS_OK;

Return:
	for(i=0; i<photo_devCnt; i++)
	{
		photo_capture_mem_free(i);
		lib_encode_buf_release(i);
	}
	return nRet;
}

static void
lib_pscaler_notify_scaleup(
	unsigned char src,
	void* p,
	INT32U data
)
{
	PSCALER_STREAM_CTRL_T *stream_ctrl = (PSCALER_STREAM_CTRL_T*) p;
	FB_CFG* pMsgNotifyCfg = stream_ctrl->msg_fb_cfg;
	FB_INFO* pfb_info = ((FB_INFO*)(pMsgNotifyCfg->fb_cfg_head_addr))+pMsgNotifyCfg->fb_cfg_count_num;

	pfb_info->buf = data;
	pfb_info->idx = pMsgNotifyCfg->fb_cfg_count_num;

	if(circular_buf_try_put(cbuf_notify_enc[src],((INT32U)pfb_info)) < 0)
	{
		ERR_PRINT("fail to put cbuf_notify_enc[%d]\r\n",src);
		goto ERR_PS_NOTIFY_SCALEUP;
	}

	pMsgNotifyCfg->fb_cfg_count_num++;
	if(pMsgNotifyCfg->fb_cfg_count_num >= pMsgNotifyCfg->fb_cfg_blocks)
	{
		pMsgNotifyCfg->fb_cfg_count_num = 0;
	}

	if(msgQSend(stream_ctrl->ps_notify_q, (MSG_VID_PSCALER_SCALEUP_NOTIFY|src), NULL, 0, MSG_PRI_NORMAL) < 0)
	{
		ERR_PRINT("fail to msgQSendFromISR [%d]\r\n",src);
		goto ERR_PS_NOTIFY_SCALEUP;
	}

	return;

ERR_PS_NOTIFY_SCALEUP:
	// Put pscaler bypass output buffer to circular buf
	if(data != DUMMY_BUFFER_ADDRS)
	{
        lib_pscaler_free_bypass(src,data);
    }
}

static void
front_pscaler_notify_scaleup(
	unsigned char src,
	void* p,
	INT32U data
)
{
	return(lib_pscaler_notify_scaleup(src,p,data));
}

static void
rear_pscaler_notify_scaleup(
	unsigned char src,
	void* p,
	INT32U data
)
{
	return(lib_pscaler_notify_scaleup(src,p,data));
}

INT32S
lib_pscaler_scaleup_process(
	INT32U src
)
{
	INT32S nRet = STATUS_OK;
	INT32U data;
	FB_INFO* ps_fb_info;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	ENC_STREAM_CTRL_T *enc_stream_ctrl = pPscalerStreamCtrl->enc_stream_ctrl;
	ENC_STREAM_INFO_T *enc_stream_info = &pPscalerStreamCtrl->enc_stream_info;

	/*
		Get pscaler's an output buffer for scaling up (sensorW x sensorH)
		Check jpeg engine status before encoding
	*/
	if(enc_stream_ctrl && enc_stream_ctrl->stream_encode_func && !ENCODE_STATUS_IS_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_PHOTO_OFFSET))
	{
		// Get pscaler output buffer A/B
		if(circular_buf_get(cbuf_notify_enc[src],((INT32U*)&data)) < 0)
		{
			//ERR_PRINT("fail to get FB_INFO[%d]\r\n",src);
			return STATUS_FAIL;
		}

		ps_fb_info = (FB_INFO*)data;

		//DBG_PRINT("[process] pscaler buf:0x%x,idx:%d\r\n",ps_fb_info->buf,ps_fb_info->idx);
		if(ps_fb_info->buf == DUMMY_BUFFER_ADDRS)
		{
			return STATUS_OK;
		}

		// Encode jpeg fifo mode
		nRet = enc_stream_ctrl->stream_encode_func(src,pPscalerStreamCtrl,ps_fb_info);
		if(nRet == STATUS_OK)
		{
			// Set jpeg engine to busy
			ENCODE_STATUS_SET_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_PHOTO_OFFSET);
		}
	}

	return nRet;
}

INT32S
lib_photo_write_process(
	INT32U src
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	ENC_STREAM_CTRL_T *enc_stream_ctrl = pPscalerStreamCtrl->enc_stream_ctrl;

	if(enc_stream_ctrl && enc_stream_ctrl->stream_write_func)
	{
		enc_stream_ctrl->stream_write_func(src,(void*)pPscalerStreamCtrl);
	}

	return nRet;
}

static INT32S
lib_photo_write_JPEG(
	INT8U src,
	void* p
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl  = (PSCALER_STREAM_CTRL_T*)p;
	ENC_STREAM_INFO_T *enc_stream_info = &pPscalerStreamCtrl->enc_stream_info;
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	JPEG_REC_ARGS*	pJpeg_rec_args = &pVid_info->jpeg_rec_args;
	FB_CFG* pfb_jpeg_out_cfg = &pJpeg_rec_args->fb_jpeg_out_cfg;
	JpegEnc_t*pJpegEnc =  &pJpeg_rec_args->jpeg_cfg;
	INT32U output_buffer = pJpegEnc->output_buffer;
	INT32U output_buffer_len = pJpegEnc->output_buffer_len;
	INT32U encode_size;
	INT32U jpeg_status;
	INT32U content_size = 0;
	INT8U i;


	osENTER_CRITICAL();
	encode_size = pJpegEnc->encode_size;
	jpeg_status = pJpegEnc->jpeg_status;
	pJpegEnc->jpeg_status = 0;
	//DBG_PRINT("=>0x%x\r\n",jpeg_status);
	osEXIT_CRITICAL();

	if(jpeg_status & C_JPG_STATUS_ENCODE_DONE)
	{
		DBG_PRINT("=>ED%d\r\n",src);
		if(encode_size >= pJpeg_rec_args->photo_jpeg_write_out_size)
		{
			encode_size -=  pJpeg_rec_args->photo_jpeg_write_out_size;
		}

		if (encode_size & 0x3F)
		{
			encode_size = (encode_size + 0x3F) >> 6 << 6; //64 byte alignment for hw constrain
			if(encode_size > output_buffer_len)
			{
				encode_size = output_buffer_len;
			}
		}

		if(encode_size)
		{
		    cache_invalid_range(output_buffer, encode_size);
			ap_storage_service_write_file(pVid_info->rec_file_handle,output_buffer, encode_size);
		}
		ap_storage_service_close_file(pVid_info->rec_file_handle);
		photo_capture_mem_free(src);

		// Wait for jpeg encoding is done
		if(lib_photo_ready_check(src,NULL) ==  STATUS_FAIL)
		{
			return STATUS_OK;
		}

		/*
			Because "lib_pscaler_stream_init" reset value to defalut
			Put "lib_encode_buf_release" here until all source finish their work
		*/
		for(i=0; i<photo_devCnt; i++)
		{
			lib_encode_buf_release(i);
		}

		// Restart preview
		app_msg_send(MSG_STORAGE_SERVICE_PIC_DONE, &nRet, sizeof(INT32S), MSG_PRI_NORMAL);
		sensor_preview_start();

		#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
		if(uvc_q_ready_is_empty() == 0)
		{
			host_2K_mjpeg_ready_notify();
		}
		#endif
		return STATUS_OK;
	}

	if((jpeg_status & C_JPG_STATUS_INPUT_EMPTY) || (jpeg_status & C_JPG_STATUS_OUTPUT_FULL))
	{
		if(jpeg_status & C_JPG_STATUS_INPUT_EMPTY)
		{
			// Clear encode status to do the next
			ENCODE_STATUS_CLR_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_PHOTO_OFFSET);

			// Notify the pscaler to restart an output buffer
			ps_scale_up_restart(src,pfb_jpeg_out_cfg->fb_cfg_count_num);

			osENTER_CRITICAL();
			content_size = circular_buf_size(cbuf_notify_enc[src]);
			osEXIT_CRITICAL();

			//DBG_PRINT("[%d]=>content_size:%d\r\n",src,content_size);

			if(content_size)
			{
				// Send message to encode the next jpeg
				if(msgQSend(pPscalerStreamCtrl->ps_notify_q, (MSG_VID_PSCALER_SCALEUP_NOTIFY|src), NULL, 0, MSG_PRI_NORMAL) < 0)
				{
					ERR_PRINT("fail to MSG_VID_PSCALER_SCALEUP_NOTIFY[%d]\r\n",src);
					RETURN(STATUS_FAIL);
				}
			}
		}

		if(jpeg_status & C_JPG_STATUS_OUTPUT_FULL)
		{
			//DBG_PRINT("=>OUTPUT FULL\r\n");

			pJpeg_rec_args->photo_jpeg_write_out_size += output_buffer_len;

			//C_JPG_STATUS_OUTPUT_FULL: change a new jpeg output buffer
			pJpeg_rec_args->photo_jpeg_outbuf_change = 1;
			// Get jpeg encode output buffer
			if(lib_photo_JPEG_outbuf_get(src,p) == STATUS_FAIL)
			{
				ERR_PRINT("fail to lib_photo_JPEG_outbuf_get\r\n");
				RETURN(STATUS_FAIL);
			}

			// Restart jpeg engine
			lib_photo_JPEG_output_restart(src,p);

		    cache_invalid_range(output_buffer, output_buffer_len);
			ap_storage_service_write_file(pVid_info->rec_file_handle,output_buffer, output_buffer_len);

			if(lib_free_JPEG(src,output_buffer,0)  == STATUS_FAIL)
			{
				ERR_PRINT("fail to lib_free_JPEG\r\n");
				RETURN(STATUS_FAIL);
			}
		}

	}

	return STATUS_OK;

Return:

	ap_storage_service_close_file(pVid_info->rec_file_handle);
	photo_capture_mem_free(src);
	// Set jpeg to done
	if(lib_photo_ready_check(src,NULL) ==  STATUS_FAIL)
	{
		return STATUS_OK;
	}

	/*
		Because "lib_pscaler_stream_init" reset value to defalut
		Put "lib_encode_buf_release" here until all source finish their work
	*/
	for(i=0; i<photo_devCnt; i++)
	{
		lib_encode_buf_release(i);
	}

	// Restart preview
	app_msg_send(MSG_STORAGE_SERVICE_PIC_DONE, &nRet, sizeof(INT32S), MSG_PRI_NORMAL);
	sensor_preview_start();

	#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
	if(uvc_q_ready_is_empty() == 0)
	{
		host_2K_mjpeg_ready_notify();
	}
	#endif
	return nRet;
}


INT32S preview_slide_up(INT32U pixel, INT8U wait)
{
	INT32U	outW, outH;
	INT32U	zoomW, zoomH;
	INT32U  scaler;

	if((_preview_mode == PREVIEW_MODE_FRONT)
		|| (_preview_mode == PREVIEW_MODE_PIP_REAR))
	{
		scaler  = preview_src_to_ps(PREVIEW_SRC_FRONT);
		zoomW	= preview_zoom_width_get(scaler, _preview_mode);
		zoomH	= preview_zoom_height_get(scaler, _preview_mode);
		outW	= preview_output_width_get(scaler, _preview_mode);
		outH	= preview_output_height_get(scaler, _preview_mode);

		if(zoomH > outH)
		{
			if(zoomH > (preview_out_h_start_next[PREVIEW_SRC_FRONT] + pixel + outH))
			{
                osENTER_CRITICAL();
				preview_out_h_start_next[PREVIEW_SRC_FRONT] += pixel;
				osEXIT_CRITICAL();
				DBG_PRINT("front out_h_start = %d\r\n", preview_out_h_start_next[PREVIEW_SRC_FRONT]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}
			else
			{
                osENTER_CRITICAL();
				preview_out_h_start_next[PREVIEW_SRC_FRONT] = zoomH - outH;
				osEXIT_CRITICAL();
				DBG_PRINT("front out_h_start = %d\r\n", preview_out_h_start_next[PREVIEW_SRC_FRONT]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}

			if(wait)
			{
                INT32U time_out = 500;
                while(preview_out_h_start_next[PREVIEW_SRC_FRONT] != preview_out_h_start[PREVIEW_SRC_FRONT])
                {
                    osDelay(1);
                    if(--time_out == 0) break;
                }
			}
		}
		else if(zoomW > outW)
		{
			if(zoomW + (preview_out_w_start_next[PREVIEW_SRC_FRONT] + pixel + outW))
			{
                osENTER_CRITICAL();
				preview_out_w_start_next[PREVIEW_SRC_FRONT] += pixel;
				osEXIT_CRITICAL();
				DBG_PRINT("front out_w_start = %d\r\n", preview_out_w_start_next[PREVIEW_SRC_FRONT]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}
			else
			{
                osENTER_CRITICAL();
				preview_out_w_start_next[PREVIEW_SRC_FRONT] = zoomW - outW;
				osEXIT_CRITICAL();
				DBG_PRINT("front out_w_start = %d\r\n", preview_out_w_start_next[PREVIEW_SRC_FRONT]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}

			if(wait)
			{
                INT32U time_out = 500;
                while(preview_out_w_start_next[PREVIEW_SRC_FRONT] != preview_out_w_start[PREVIEW_SRC_FRONT])
                {
                    osDelay(1);
                    if(--time_out == 0) break;
                }
			}
		}
	}
	else if((_preview_mode == PREVIEW_MODE_REAR)
#if !C_DISP_SLIDE_SAVE
		|| (_preview_mode == PREVIEW_MODE_BACK_KEEP_RATIO)
#endif
		|| (_preview_mode == PREVIEW_MODE_PIP_FRONT))
	{
		scaler  = preview_src_to_ps(PREVIEW_SRC_REAR);
		zoomW	= preview_zoom_width_get(scaler, _preview_mode);
		zoomH	= preview_zoom_height_get(scaler, _preview_mode);
		outW	= preview_output_width_get(scaler, _preview_mode);
		outH	= preview_output_height_get(scaler, _preview_mode);

		if(zoomH > outH)
		{
			if(zoomH > (preview_out_h_start_next[PREVIEW_SRC_REAR] + pixel + outH))
			{
                osENTER_CRITICAL();
				preview_out_h_start_next[PREVIEW_SRC_REAR] += pixel;
				osEXIT_CRITICAL();
				DBG_PRINT("rear out_h_start = %d\r\n", preview_out_h_start_next[PREVIEW_SRC_REAR]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}
			else
			{
				osENTER_CRITICAL();
				preview_out_h_start_next[PREVIEW_SRC_REAR] = zoomH - outH;
				osEXIT_CRITICAL();
				DBG_PRINT("rear out_h_start = %d\r\n", preview_out_h_start_next[PREVIEW_SRC_REAR]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}

			if(wait)
			{
                INT32U time_out = 500;
                while(preview_out_h_start_next[PREVIEW_SRC_REAR] != preview_out_h_start[PREVIEW_SRC_REAR])
                {
                    osDelay(1);
                    if(--time_out == 0) break;
                }
			}
		}
		else if(zoomW > outW)
		{
			if(zoomW > (preview_out_w_start_next[PREVIEW_SRC_REAR] + pixel + outW))
			{
                osENTER_CRITICAL();
				preview_out_w_start_next[PREVIEW_SRC_REAR] += pixel;
				osEXIT_CRITICAL();
				DBG_PRINT("rear out_w_start = %d\r\n", preview_out_w_start_next[PREVIEW_SRC_REAR]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}
			else
			{
                osENTER_CRITICAL();
				preview_out_w_start_next[PREVIEW_SRC_REAR] = zoomW - outW;
				osEXIT_CRITICAL();
				DBG_PRINT("rear out_w_start = %d\r\n", preview_out_w_start_next[PREVIEW_SRC_REAR]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}

			if(wait)
			{
                INT32U time_out = 500;
                while(preview_out_w_start_next[PREVIEW_SRC_REAR] != preview_out_w_start[PREVIEW_SRC_REAR])
                {
                    osDelay(1);
                    if(--time_out == 0) break;
                }
			}
		}
	}
#if C_DISP_SLIDE_SAVE
	else if(_preview_mode == PREVIEW_MODE_BACK_KEEP_RATIO)
	{
		scaler  = preview_src_to_ps(PREVIEW_SRC_REAR);
		zoomW	= preview_zoom_width_get(scaler, _preview_mode);
		zoomH	= preview_zoom_height_get(scaler, _preview_mode);
		outW	= preview_output_width_get(scaler, _preview_mode);
		outH	= preview_output_height_get(scaler, _preview_mode);

		if(zoomH > outH)
		{
			if(zoomH > (preview_out_h_start_reversing_next + pixel + outH))
			{
				osENTER_CRITICAL();
				preview_out_h_start_reversing_next += pixel;
				osEXIT_CRITICAL();
				DBG_PRINT("reversing h start = %d\r\n", preview_out_h_start_reversing_next);
				sensor_preview_out_crop_reversing_flag_set(1);
			}
			else
			{
				osENTER_CRITICAL();
				preview_out_h_start_reversing_next = zoomH - outH;
				osEXIT_CRITICAL();
				DBG_PRINT("reversing h start = %d\r\n", preview_out_h_start_reversing_next);
				sensor_preview_out_crop_reversing_flag_set(1);
			}

			if(wait)
			{
                INT32U time_out = 500;
                while(preview_out_h_start_reversing_next != preview_out_h_start_reversing)
                {
                    osDelay(1);
                    if(--time_out == 0) break;
                }
			}
		}
		else if(zoomW > outW)
		{
			if(zoomW > (preview_out_w_start_reversing_next + pixel + outW))
			{
				osENTER_CRITICAL();
				preview_out_w_start_reversing_next += pixel;
				osEXIT_CRITICAL();
				DBG_PRINT("reversing w start = %d\r\n", preview_out_w_start_reversing_next);
				sensor_preview_out_crop_reversing_flag_set(1);
			}
			else
			{
				osENTER_CRITICAL();
				preview_out_w_start_reversing_next = zoomW - outW;
				osEXIT_CRITICAL();
				DBG_PRINT("reversing w start = %d\r\n", preview_out_w_start_reversing_next);
				sensor_preview_out_crop_reversing_flag_set(1);
			}

			if(wait)
			{
                INT32U time_out = 500;
                while(preview_out_w_start_reversing_next != preview_out_w_start_reversing)
                {
                    osDelay(1);
                    if(--time_out == 0) break;
                }
			}
		}
	}
#endif
	return 0;
}


INT32S preview_slide_down(INT32U pixel, INT8U wait)
{
	INT32U	outW, outH;
	INT32U	zoomW, zoomH;
	INT32U  scaler;

	if((_preview_mode == PREVIEW_MODE_FRONT)
		|| (_preview_mode == PREVIEW_MODE_PIP_REAR))
	{
		scaler  = preview_src_to_ps(PREVIEW_SRC_FRONT);
		zoomW	= preview_zoom_width_get(scaler, _preview_mode);
		zoomH	= preview_zoom_height_get(scaler, _preview_mode);
		outW	= preview_output_width_get(scaler, _preview_mode);
		outH	= preview_output_height_get(scaler, _preview_mode);

		if(zoomH > outH)
		{
			if(preview_out_h_start_next[PREVIEW_SRC_FRONT] > pixel)
			{
                osENTER_CRITICAL();
				preview_out_h_start_next[PREVIEW_SRC_FRONT] -= pixel;
				osEXIT_CRITICAL();
				DBG_PRINT("front out_h_start = %d\r\n", preview_out_h_start_next[PREVIEW_SRC_FRONT]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}
			else
			{
                osENTER_CRITICAL();
				preview_out_h_start_next[PREVIEW_SRC_FRONT] = 0;
				osEXIT_CRITICAL();
				DBG_PRINT("front out_h_start = %d\r\n", preview_out_h_start_next[PREVIEW_SRC_FRONT]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}

			if(wait)
			{
                INT32U time_out = 500;
                while(preview_out_h_start_next[PREVIEW_SRC_FRONT] != preview_out_h_start[PREVIEW_SRC_FRONT])
                {
                    osDelay(1);
                    if(--time_out == 0) break;
                }
			}
		}
		else if(zoomW > outW)
		{
			if(preview_out_w_start_next[PREVIEW_SRC_FRONT] > pixel)
			{
                osENTER_CRITICAL();
				preview_out_w_start_next[PREVIEW_SRC_FRONT] -= pixel;
				osEXIT_CRITICAL();
				DBG_PRINT("front out_w_start = %d\r\n", preview_out_w_start_next[PREVIEW_SRC_FRONT]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}
			else
			{
                osENTER_CRITICAL();
				preview_out_w_start_next[PREVIEW_SRC_FRONT] = 0;
				osEXIT_CRITICAL();
				DBG_PRINT("front out_w_start = %d\r\n", preview_out_w_start_next[PREVIEW_SRC_FRONT]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}

			if(wait)
			{
                INT32U time_out = 500;
                while(preview_out_w_start_next[PREVIEW_SRC_FRONT] != preview_out_w_start[PREVIEW_SRC_FRONT])
                {
                    osDelay(1);
                    if(--time_out == 0) break;
                }
			}
		}
	}
	else if((_preview_mode == PREVIEW_MODE_REAR)
#if !C_DISP_SLIDE_SAVE
		|| (_preview_mode == PREVIEW_MODE_BACK_KEEP_RATIO)
#endif
		|| (_preview_mode == PREVIEW_MODE_PIP_FRONT))
	{
		scaler  = preview_src_to_ps(PREVIEW_SRC_REAR);
		zoomW	= preview_zoom_width_get(scaler, _preview_mode);
		zoomH	= preview_zoom_height_get(scaler, _preview_mode);
		outW	= preview_output_width_get(scaler, _preview_mode);
		outH	= preview_output_height_get(scaler, _preview_mode);

		if(zoomH > outH)
		{
			if(preview_out_h_start_next[PREVIEW_SRC_REAR] > pixel)
			{
                osENTER_CRITICAL();
				preview_out_h_start_next[PREVIEW_SRC_REAR] -= pixel;
				osEXIT_CRITICAL();
				DBG_PRINT("rear out_h_start = %d\r\n", preview_out_h_start_next[PREVIEW_SRC_REAR]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}
			else
			{
                osENTER_CRITICAL();
				preview_out_h_start_next[PREVIEW_SRC_REAR] = 0;
				osEXIT_CRITICAL();
				DBG_PRINT("rear out_h_start = %d\r\n", preview_out_h_start_next[PREVIEW_SRC_REAR]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}

			if(wait)
			{
                INT32U time_out = 500;
                while(preview_out_h_start_next[PREVIEW_SRC_REAR] != preview_out_h_start[PREVIEW_SRC_REAR])
                {
                    osDelay(1);
                    if(--time_out == 0) break;
                }
			}
        }
		else if(zoomW > outW)
		{
			if(preview_out_w_start_next[PREVIEW_SRC_REAR] > pixel)
			{
                osENTER_CRITICAL();
				preview_out_w_start_next[PREVIEW_SRC_REAR] -= pixel;
				osEXIT_CRITICAL();
				DBG_PRINT("rear out_w_start = %d\r\n", preview_out_w_start_next[PREVIEW_SRC_REAR]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}
			else
			{
                osENTER_CRITICAL();
				preview_out_w_start_next[PREVIEW_SRC_REAR] = 0;
				osEXIT_CRITICAL();
				DBG_PRINT("rear out_w_start = %d\r\n", preview_out_w_start_next[PREVIEW_SRC_REAR]);
#if C_DISP_SLIDE_SAVE
				sensor_preview_out_crop_flag_set(1);
#endif
			}

			if(wait)
			{
                INT32U time_out = 500;
                while(preview_out_w_start_next[PREVIEW_SRC_REAR] != preview_out_w_start[PREVIEW_SRC_REAR])
                {
                    osDelay(1);
                    if(--time_out == 0) break;
                }
			}
		}
	}
#if C_DISP_SLIDE_SAVE
	else if(_preview_mode == PREVIEW_MODE_BACK_KEEP_RATIO)
	{
		scaler  = preview_src_to_ps(PREVIEW_SRC_REAR);
		zoomW	= preview_zoom_width_get(scaler, _preview_mode);
		zoomH	= preview_zoom_height_get(scaler, _preview_mode);
		outW	= preview_output_width_get(scaler, _preview_mode);
		outH	= preview_output_height_get(scaler, _preview_mode);

		if(zoomH > outH)
		{
			if(preview_out_h_start_reversing_next > pixel)
			{
                osENTER_CRITICAL();
				preview_out_h_start_reversing_next -= pixel;
				osEXIT_CRITICAL();
				DBG_PRINT("reversing h start = %d\r\n", preview_out_h_start_reversing_next);
				sensor_preview_out_crop_reversing_flag_set(1);
			}
			else
			{
                osENTER_CRITICAL();
				preview_out_h_start_reversing_next = 0;
				osEXIT_CRITICAL();
				DBG_PRINT("reversing h start = %d\r\n", preview_out_h_start_reversing_next);
				sensor_preview_out_crop_reversing_flag_set(1);
			}

			if(wait)
			{
                INT32U time_out = 500;
                while(preview_out_h_start_reversing_next != preview_out_h_start_reversing)
                {
                    osDelay(1);
                    if(--time_out == 0) break;
                }
			}
		}
		else if(zoomW > outW)
		{
			if(preview_out_w_start_reversing_next > pixel)
			{
                osENTER_CRITICAL();
				preview_out_w_start_reversing_next -= pixel;
				osEXIT_CRITICAL();
				DBG_PRINT("reversing w start = %d\r\n", preview_out_w_start_reversing_next);
				sensor_preview_out_crop_reversing_flag_set(1);
			}
			else
			{
                osENTER_CRITICAL();
				preview_out_w_start_reversing_next = 0;
				osEXIT_CRITICAL();
				DBG_PRINT("reversing w start = %d\r\n", preview_out_w_start_reversing_next);
				sensor_preview_out_crop_reversing_flag_set(1);
			}

			if(wait)
			{
                INT32U time_out = 500;
                while(preview_out_w_start_reversing_next != preview_out_w_start_reversing)
                {
                    osDelay(1);
                    if(--time_out == 0) break;
                }
			}
		}
	}
#endif
	return 0;
}


/*== W I F I ==*/
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
INT32S
wifi_stream_init(
	void
)
{
	return(lib_pscaler_stream_init(PREVIEW_SRC_WIFI));
}

INT32S
wifi_encode_buf_set(
	MSG_Q_ID notifyQ,
	VID_INFO* args
)
{
	return(lib_encode_buf_set(PREVIEW_SRC_WIFI,notifyQ,args));
}

INT32S
wifi_encode_start(
	VID_INFO* args
)
{
	return(lib_encode_start(PREVIEW_SRC_WIFI,args));
}

INT32S
wifi_encode_stop(
	void
)
	{
		INT32S ret;
		PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[PREVIEW_SRC_WIFI];
	
		
		ret = lib_encode_stop(PREVIEW_SRC_WIFI);
	
		//check the stream_enable is VIDEO_RECORD_STOP? if yes , dump dummy buffer address for bypass & set stram_enable = VIDEO_RECORD_NOTHING
		if(pPscalerStreamCtrl->enc_stream_info.stream_enable == VIDEO_RECORD_STOP)
		{
			//DBG_PRINT("[WIFI_STOP_REQ] force wifi dummy\r\n");
			//dump dummy to bypass,for  lib_encode_process() checking the DUMMY_BUFFER_ADDRS.
			lib_pscaler_notify_bypass(PREVIEW_SRC_WIFI,pPscalerStreamCtrl,DUMMY_BUFFER_ADDRS);
	
			//set the stream_enable as VIDEO_RECORD_NOTHING
			pPscalerStreamCtrl->enc_stream_info.stream_enable = VIDEO_RECORD_NOTHING;
		}
		return ret; 
	}

INT32S
wifi_encode_buf_release(
	void
)
{
	return(lib_encode_buf_release(PREVIEW_SRC_WIFI));
}

static unsigned int
wifi_pscaler_get_bypass(
	void
)
{
	return STATUS_OK;
}

static void
wifi_pscaler_free_bypass(
	unsigned int data
)
{
	if(ap_display_queue_put(display_free_queue, (DISP_P)data) != STATUS_OK)
	{
		DBG_PRINT("[ERROR]display_free_queue full\r\n");
	}
}

static void
wifi_pscaler_notify_bypass(
	unsigned char src,
	void* p,
	INT32U data
)
{
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = (PSCALER_STREAM_CTRL_T*)p;

	// Assign 1 to fb_cfg_count_num because wifi encodes frame buffer
	pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num = 1;

	if(pPscalerStreamCtrl->enc_stream_info.stream_enable == VIDEO_RECORD_START)
	{
		return(lib_pscaler_notify_bypass(src,p,data));
	}
	else
	{
		if(pPscalerStreamCtrl->enc_stream_info.stream_enable == VIDEO_RECORD_STOP)
		{
			// Sends DUMMY_BUFFER_ADDRS to force the ending of wifi encoding
			lib_pscaler_notify_bypass(src,p,DUMMY_BUFFER_ADDRS);

			pPscalerStreamCtrl->enc_stream_info.stream_enable = VIDEO_RECORD_NOTHING;
		}

		return(pPscalerStreamCtrl->ps_free_func(data));
	}
}

void
wifi_encode_buf_notify(
	INT32U data
)
{
	void* p =  &pscaler_stream[PREVIEW_SRC_WIFI];

	return wifi_pscaler_notify_bypass(PREVIEW_SRC_WIFI,p,data);
}

static void
wifi_stream_buf_release(
	INT32U data,
	INT32U frameType
)
{
	lib_free_H264(PREVIEW_SRC_WIFI,data,frameType);
}
#endif

#if C_SUPORT_GET_JPG_BUF
static void
lib_get_jpeg_encode_Callback(
	JPEG_IDX idx,
	INT32S event,
	INT32U count
)
{
	PSCALER_STREAM_CTRL_T* pPsaclerStream;
	VID_INFO* pVid_info;
	GET_JPEG_ARGS* pGet_jpeg_args;
	JpegEnc_t*pGetJpeg;
	INT32U jpegHeaderSize = jpeg_header_get_size();
	INT8U i;

	//DBG_PRINT("idx:%d, event:0x%x, count:%d \r\n",idx,event,count);

	// Check the handle
	for(i=PREVIEW_SRC_FRONT; i<=PREVIEW_SRC_REAR; i++)
	{
		pPsaclerStream = &pscaler_stream[i];
		pVid_info = pPsaclerStream->pVid_info;

		if(!pVid_info) // Source path does not exists
		{
			continue;
		}

		pGet_jpeg_args = &pVid_info->get_jpeg_args;

		if(pGet_jpeg_args->flag_jpeg_trigger)
		{
			pGetJpeg = &pGet_jpeg_args->jpeg_cfg;
			if(pGetJpeg->hwidx != idx)
			{
				continue;
			}

			pGet_jpeg_args->flag_jpeg_trigger = 0;
			// Put pscaler bypass output buffer to circular buf
			if(pGetJpeg->input_buffer_y != DUMMY_BUFFER_ADDRS)
			{
				#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
				if ((pVid_info->run_2K_flow_src & HOST_2K_IS_FRONT) == 0)
				#endif
				{
		        	lib_pscaler_free_bypass(i,pGetJpeg->input_buffer_y);
				}
				pGetJpeg->input_buffer_y = DUMMY_BUFFER_ADDRS;
		    }

			pGetJpeg->jpeg_status |= event;

			if(event & C_JPG_STATUS_ENCODE_DONE)
			{
				pGet_jpeg_args->flag_fifo_start = 0;
				pGetJpeg->encode_size = count+jpegHeaderSize;
				if((pGetJpeg->encode_size) & 0x3F)
				{
                    pGetJpeg->encode_size = (pGetJpeg->encode_size + 0x3F) >> 6 << 6; //64 byte alignment for hw constrain
                }
				msgQSend(pPsaclerStream->ps_notify_q, (MSG_VID_GET_JPEG_DONE|i), NULL, 0, MSG_PRI_NORMAL);
				break;
			}

			if(event & C_JPG_STATUS_INPUT_EMPTY)
			{
				#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
				if ((pVid_info->run_2K_flow_src & HOST_2K_IS_FRONT) == 0)
				#endif
				{
					msgQSend(pPsaclerStream->ps_notify_q, (MSG_VID_GET_JPEG_NOTIFY|i), NULL, 0, MSG_PRI_NORMAL);
				}
			}
		}
	}

}

static INT32S
lib_get_jpeg_encode(
	INT8U src,
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl
)
{
	INT32S nRet = STATUS_OK;
	INT32U data;
	INT32U jpegHeaderSize = jpeg_header_get_size();
	INT32U unmask_addr;
	FB_INFO* ps_fb_info = NULL;
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	GET_JPEG_ARGS* pGet_jpeg_args = &pVid_info->get_jpeg_args;
	JpegEnc_t*pGetJpegEnc = &pGet_jpeg_args->jpeg_cfg;
	INT32U jpeg_status = 0;
    #if (C_VID_INTERPOLATING_2K)
	H264Enc_t*pH264Enc = &pVid_info->h264_rec_args.h264_cfg;
    #endif

	osENTER_CRITICAL();
	jpeg_status = pGetJpegEnc->jpeg_status;
	pGetJpegEnc->jpeg_status = 0;
	//DBG_PRINT("=>0x%x\r\n",jpeg_status);
	osEXIT_CRITICAL();

	if(jpeg_status & C_JPG_STATUS_INPUT_EMPTY)
	{
		// Clear jpeg engine status to idle
		lib_encode_engine_path_ctrl(src,VID_GET_JPEG,ENC_ENGINE_ACT_CLEAR);

		#if (C_VID_INTERPOLATING_2K)
		// Notify the pscaler to restart an output buffer
		if(pVid_info->pscaler_up_start) // Only front sensor enables this variable
		{
			#if C_SUPORT_GET_JPG_BUF
			if(pGet_jpeg_args->flag_start_buf_get && pGet_jpeg_args->flag_start_fifo_buf_get)
			{
				pGet_jpeg_args->flag_fifo_done |= GET_JPEG_DONE_JPEG;
				if(pGet_jpeg_args->flag_fifo_done == (GET_JPEG_DONE_H264|GET_JPEG_DONE_JPEG))
				{
					interp2K_scale_up_restart(src,pH264Enc->input_buffer_idx);
					pGet_jpeg_args->flag_fifo_done = 0;
				}
			}
			#endif
		}
		#endif
	}

	// Check jpeg engine status
	if(lib_encode_engine_path_ctrl(src,VID_GET_JPEG,ENC_ENGINE_ACT_CHECK) == STATUS_FAIL)
	{
		//DBG_PRINT("=>GJ_Busy(%d)\r\n",src);

		if(pVid_info->vid_rec_line == 0)
		{// FRAME MODE
			msgQSend(pPscalerStreamCtrl->ps_notify_q, (MSG_VID_GET_JPEG_NOTIFY|src), NULL, 0, MSG_PRI_NORMAL);
		}
		return STATUS_OK;
	}

	// Get pscaler's bypass frame/fifo buffer
	if(circular_buf_get(cbuf_notify_get_jpeg[src],((INT32U*)&data)) < 0)
	{
		//ERR_PRINT("fail to get FB_INFO[%d]\r\n",src);
		return (STATUS_FAIL);
	}

	ps_fb_info = (FB_INFO*)data;

	pGetJpegEnc->input_buffer_y = ps_fb_info->buf;

	if(pGet_jpeg_args->flag_fifo_start)
	{// FIFO MODE
		#if C_VID_REC_USE_SYS_FIFO
		if(pPscalerStreamCtrl->flag_sys_fifo)
		{
			if(pGetJpegEnc->input_buffer_y == DUMMY_BUFFER_ADDRS)
			{
				unmask_addr = pGetJpegEnc->input_buffer_y;
				DBG_PRINT("=>IN DUMMY\r\n");
			}
			else
			{
				unmask_addr = lib_encode_system_fifo_unmapping(src,pGetJpegEnc->input_buffer_y);
			}
		}
		else
		{
			unmask_addr = pGetJpegEnc->input_buffer_y;
		}
		#else
		unmask_addr = pGetJpegEnc->input_buffer_y;
		#endif

		//DBG_PRINT("[JR:%d:%x]",src,unmask_addr);
		if(unmask_addr == DUMMY_BUFFER_ADDRS)
		{// Stop video record and reset JPEG engine
			jpeg_encode_stop(pGetJpegEnc->hwidx);
			pGet_jpeg_args->flag_fifo_start = 0;
			pGet_jpeg_args->flag_jpeg_trigger = 0;
			RETURN(STATUS_FAIL);
		}
		else
		{
			pGet_jpeg_args->flag_jpeg_trigger = 1;
			if(ps_fb_info->idx == pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_max_count)
			{// Prevent jpeg engine from sending C_JPG_STATUS_INPUT_EMPTY
				jpeg_encode_on_the_fly_start(pGetJpegEnc->hwidx,unmask_addr,0,0,(pGetJpegEnc->input_y_len << 1));
			}
			else
			{
				jpeg_encode_on_the_fly_start(pGetJpegEnc->hwidx,unmask_addr,0,0,pGetJpegEnc->input_y_len);
			}
		}
		return STATUS_OK;
	}

	//DBG_PRINT("pJpegEnc->output_buffer:0x%x \r\n",pJpegEnc->output_buffer);

	lib_jpeg_setting(pGetJpegEnc);

	nRet = jpeg_encode_input_size_set(pGetJpegEnc->hwidx, pGetJpegEnc->width, pGetJpegEnc->height);
	if(nRet < 0)
	{
		ERR_PRINT("fail to jpeg_encode_input_size_set\r\n");
		RETURN(STATUS_FAIL);
	}

	jpeg_encode_input_format_set(pGetJpegEnc->hwidx, pGetJpegEnc->yuv_fmt);

	switch(pGetJpegEnc->yuv_sample_fmt)
	{
	case JPEG_YUV422:
		nRet = jpeg_encode_yuv_sampling_mode_set(pGetJpegEnc->hwidx, C_JPG_CTRL_YUV422);
		break;

	case JPEG_YUV420:
		nRet = drv_l1_jpeg_yuv_sampling_mode_set(pGetJpegEnc->hwidx, C_JPG_CTRL_GP420 | C_JPG_CTRL_YUV420);
		break;

	default:
		nRet = STATUS_FAIL;
	}
	if(nRet < 0)
	{
		ERR_PRINT("fail to drv_l1_jpeg_yuv_sampling_mode_set\r\n");
		RETURN(STATUS_FAIL);
	}

	switch(pGetJpegEnc->scale_mode)
	{
	case JPEG_SCALE_VX2:
		drv_l1_jpeg_encode_scale_up_set(pGetJpegEnc->hwidx, ENUM_JPG_SCALE_VX2);
		break;
	case JPEG_SCALE_X2:
		drv_l1_jpeg_encode_scale_up_set(pGetJpegEnc->hwidx, ENUM_JPG_SCALE_X2);
		break;
	case JPEG_SCALE_YUYV_X4:
		drv_l1_jpeg_encode_scale_up_set(pGetJpegEnc->hwidx, ENUM_JPG_SCALE_X4);
		break;
	case JPEG_SCALE_GP420_X3D2:
		drv_l1_jpeg_encode_scale_up_set(pGetJpegEnc->hwidx, ENUM_JPG_SCALE_X1_5);
		break;
	case JPEG_SCALE_YUYV_X9D8:
		drv_l1_jpeg_encode_scale_up_set(pGetJpegEnc->hwidx, ENUM_JPG_SCALE_X9D8);
		break;
	default:
		break;
	}

	nRet = jpeg_encode_output_addr_set(pGetJpegEnc->hwidx, pGetJpegEnc->output_buffer+jpegHeaderSize);
	if(nRet < 0) {
		ERR_PRINT("fail to jpeg_encode_output_addr_set\r\n");
		RETURN(STATUS_FAIL);
	}

	jpeg_encode_maximum_length_set(pGetJpegEnc->hwidx, pGetJpegEnc->output_buffer_len-jpegHeaderSize);

	jpeg_encode_isr_callback_set(pGetJpegEnc->hwidx,lib_get_jpeg_encode_Callback);

	#if C_VID_REC_USE_SYS_FIFO
	if(pPscalerStreamCtrl->flag_sys_fifo)
	{
		unmask_addr = lib_encode_system_fifo_unmapping(src,pGetJpegEnc->input_buffer_y);
	}
	else
	{
		unmask_addr = pGetJpegEnc->input_buffer_y;
	}
	#else
		unmask_addr = pGetJpegEnc->input_buffer_y;
	#endif


	pGet_jpeg_args->flag_jpeg_trigger = 1;

	// Set jpeg engine status to busy
	lib_encode_engine_path_ctrl(src,VID_GET_JPEG,ENC_ENGINE_ACT_SET);

	if(pVid_info->vid_rec_line)
	{// FIFO MODE
		//DBG_PRINT("=>JPEG FIFO\r\n");
		pGet_jpeg_args->flag_fifo_start = 1;
		jpeg_encode_on_the_fly_start(pGetJpegEnc->hwidx,unmask_addr,0,0,pGetJpegEnc->input_y_len);
	}
	else
	{// FRAME MODE
		//DBG_PRINT("=>JPEG FRAME\r\n");
		jpeg_encode_once_start(pGetJpegEnc->hwidx,unmask_addr,0,0);
	}

	return STATUS_OK;

Return:


	return STATUS_FAIL;
}

INT32S
lib_get_jpeg_enc_process(
	INT32U src
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	GET_JPEG_ARGS* pGet_jpeg_args = &pVid_info->get_jpeg_args;
	JpegEnc_t*pGetJpegEnc = &pGet_jpeg_args->jpeg_cfg;
	JpegEnc_t*pVidJpegRec = NULL;

	//DBG_PRINT("==Run:%s==src:%d==\r\n",__FUNCTION__,src);

	if(pGet_jpeg_args->flag_need_enc)
	{
		// Encode pscaler's bypass frame/fifo buffer
		lib_get_jpeg_encode(src,pPscalerStreamCtrl);
	}
	else
	{
		pVidJpegRec = &pVid_info->jpeg_rec_args.jpeg_cfg;
		pGetJpegEnc->output_buffer = pVidJpegRec->output_buffer;
		pGetJpegEnc->encode_size = pVidJpegRec->encode_size;
		msgQSend(pPscalerStreamCtrl->ps_notify_q, (MSG_VID_GET_JPEG_DONE|src), NULL, 0, MSG_PRI_NORMAL);
	}

	return nRet;
}

INT32S
lib_get_jpeg_done_process(
	INT32U src
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	GET_JPEG_ARGS* pGet_jpeg_args = &pVid_info->get_jpeg_args;
	JpegEnc_t*pJpeg_Snapshot = &pGet_jpeg_args->jpeg_cfg;
	INT32U msg_id;
	SNAPSHOT_INFO* pSnapshot_info;
	JPEG_OUTB_HEADER* pJpeg_outb_header;

	//DBG_PRINT("==Run:%s==src:%d==\r\n",__FUNCTION__,src);

	// Clear jpeg engine status to idle
	lib_encode_engine_path_ctrl(src,VID_GET_JPEG,ENC_ENGINE_ACT_CLEAR);

	switch(src)
	{
		case PREVIEW_SRC_FRONT:
			msg_id = MSG_SNAPSHOT_JPEG_DONE;
			pSnapshot_info = &gSnapshotInfo_Front;
		break;
		case PREVIEW_SRC_REAR:
			msg_id = MSG_SNAPSHOT_JPEG_DONE_REAR;
			pSnapshot_info = &gSnapshotInfo_Rear;
		break;
		default:
            RETURN(STATUS_FAIL);
		break;
	}

	pSnapshot_info->file_handle = pGet_jpeg_args->w_file_handle;
	pSnapshot_info->file_name = pGet_jpeg_args->w_file_name;

	if(pGet_jpeg_args->flag_need_enc)
	{
		pSnapshot_info->file_w_buf = pJpeg_Snapshot->output_buffer;
	}
	else
	{
		pJpeg_outb_header = (JPEG_OUTB_HEADER*)pJpeg_Snapshot->output_buffer;
		pSnapshot_info->file_w_buf = (INT32U)pJpeg_outb_header->jpeg_header;
	}
	pSnapshot_info->file_w_size = pJpeg_Snapshot->encode_size;

	app_msg_send(msg_id, pSnapshot_info, sizeof(SNAPSHOT_INFO), MSG_PRI_NORMAL);
Return:
	return nRet;
}

INT32S
lib_get_jpeg_buf_set(
	INT8U 	src,
	INT32U	blocks,
	FB_CFG* pMsgNotifyCfg
)
{
	// Init message notify
	pMsgNotifyCfg->fb_cfg_blocks = blocks;
	pMsgNotifyCfg->fb_cfg_head_addr = gp_malloc_align(pMsgNotifyCfg->fb_cfg_blocks*sizeof(FB_INFO),64);
	if(pMsgNotifyCfg->fb_cfg_head_addr == 0)
	{
		ERR_PRINT("fail to pMsgNotifyCfg's fb_cfg_head_addr\r\n");
		return STATUS_FAIL;
	}

	// Message's circular_buffer
	pMsgNotifyCfg->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pMsgNotifyCfg->fb_cfg_blocks,64);
	if(pMsgNotifyCfg->fb_cfg_cb_addr == 0)
	{
		ERR_PRINT("fail to pMsgNotifyCfg's fb_cfg_cb_addr\r\n");
		return STATUS_FAIL;
	}

	// Init notify queue
	cbuf_notify_get_jpeg[src] = circular_buf_init(pMsgNotifyCfg->fb_cfg_cb_addr,pMsgNotifyCfg->fb_cfg_blocks);
	if(cbuf_notify_get_jpeg[src] == 0)
	{
		ERR_PRINT("fail to cbuf_notify_get_jpeg[%d]\r\n",src);
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

INT32S
lib_get_jpeg_buf_release(
	INT8U src,
	FB_CFG* pMsgNotifyCfg
)
{
	// Msg
	if(pMsgNotifyCfg->fb_cfg_head_addr)
	{
		gp_free(pMsgNotifyCfg->fb_cfg_head_addr);
		pMsgNotifyCfg->fb_cfg_head_addr = 0;
	}
	if(pMsgNotifyCfg->fb_cfg_cb_addr)
	{
		gp_free(pMsgNotifyCfg->fb_cfg_cb_addr);
		pMsgNotifyCfg->fb_cfg_cb_addr = 0;
	}

	if(cbuf_notify_get_jpeg[src])
	{
		circular_buf_free(cbuf_notify_get_jpeg[src]);
		cbuf_notify_get_jpeg[src] = 0;
	}

	return STATUS_OK;
}

void
lib_get_jpeg_buf_notify(
	unsigned char src,
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl,
	FB_INFO* fb_info,
	INT8U	msg_send
)
{
	GET_JPEG_ARGS* pGet_jpeg_args = &pPscalerStreamCtrl->pVid_info->get_jpeg_args;
	FB_CFG* pMsgNotifyCfg = &pGet_jpeg_args->fb_msg_notify_cfg;
	FB_INFO* pfb_info = ((FB_INFO*)(pMsgNotifyCfg->fb_cfg_head_addr))+pMsgNotifyCfg->fb_cfg_count_num;

	pfb_info = fb_info;

	if(circular_buf_try_put(cbuf_notify_get_jpeg[src],((INT32U)pfb_info)) < 0)
	{
		ERR_PRINT("fail to put cbuf_notify_enc[%d]\r\n",src);
        return;
	}

	pMsgNotifyCfg->fb_cfg_count_num++;
	if(pMsgNotifyCfg->fb_cfg_count_num >= pMsgNotifyCfg->fb_cfg_blocks)
	{
		pMsgNotifyCfg->fb_cfg_count_num = 0;
	}

	if(msg_send)
	{
		if(msgQSend(pPscalerStreamCtrl->ps_notify_q, (MSG_VID_GET_JPEG_NOTIFY|src), NULL, 0, MSG_PRI_URGENT) < 0)
		{
			ERR_PRINT("fail to msgQSendFromISR [%d]\r\n",src);
	        return;
		}
	}

	return;
}

void
lib_get_jpeg_pscaler_buf_release(
	unsigned char src,
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl
)
{
	INT32U data;
	INT32U remaining_count;
	INT32U i;
	FB_INFO* ps_fb_info=NULL;

	remaining_count = circular_buf_size(cbuf_notify_get_jpeg[src]);
	//DBG_PRINT("remaining_count:%d\r\n",remaining_count);

	for(i=0; i<remaining_count; i++)
	{
		if(circular_buf_get(cbuf_notify_get_jpeg[src],((INT32U*)&data)) == 0)
		{
			//DBG_PRINT("data:%x\r\n",data);
			ps_fb_info = (FB_INFO*)data;

			//DBG_PRINT("buf:%x\r\n",ps_fb_info->buf);

			if(ps_fb_info->buf != DUMMY_BUFFER_ADDRS)
			{
				pPscalerStreamCtrl->ps_free_func(ps_fb_info->buf);
				ps_fb_info->buf = DUMMY_BUFFER_ADDRS;
			}
		}
	}
}

#endif

/*== U V C ==*/
#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
INT32S
uvc_stream_init(
	void
)
{
	return(lib_pscaler_stream_init(PREVIEW_SRC_UVC));
}

INT32S
uvc_encode_buf_set(
	MSG_Q_ID notifyQ,
	VID_INFO* args
)
{
	return(lib_encode_buf_set(PREVIEW_SRC_UVC,notifyQ,args));
}

INT32S
uvc_encode_start(
	VID_INFO* args
)
{
	return(lib_encode_start(PREVIEW_SRC_UVC,args));
}

INT32S
uvc_encode_stop(
	void
)
{
	return(lib_encode_stop(PREVIEW_SRC_UVC));
}

INT32S
uvc_encode_buf_release(
	void
)
{
	return(lib_encode_buf_release(PREVIEW_SRC_UVC));
}

static unsigned int
uvc_pscaler_get_bypass(
	void
)
{
	return STATUS_OK;
}

static void
uvc_pscaler_free_bypass(
	unsigned int data
)
{
	if(ap_display_queue_put(display_free_queue, (DISP_P)data) != STATUS_OK)
	{
		DBG_PRINT("[ERROR]display_free_queue full\r\n");
	}
}

static void
uvc_pscaler_notify_bypass(
	unsigned char src,
	void* p,
	INT32U data
)
{
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = (PSCALER_STREAM_CTRL_T*)p;

	// Assign 1 to fb_cfg_count_num because uvc encodes frame buffer
	pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num = 1;

	if(pPscalerStreamCtrl->enc_stream_info.stream_enable == VIDEO_RECORD_START)
	{
		return(lib_pscaler_notify_bypass(src,p,data));
	}
	else
	{
		return(pPscalerStreamCtrl->ps_free_func(data));
	}
}

void
uvc_encode_buf_notify(
	INT32U data
)
{
	void* p =  &pscaler_stream[PREVIEW_SRC_UVC];

	return uvc_pscaler_notify_bypass(PREVIEW_SRC_UVC,p,data);
}

/*
static void
uvc_stream_buf_release(
	INT32U data,
	INT32U frameType
)
{
	lib_free_H264(PREVIEW_SRC_UVC,data,frameType);
} */
#endif

/*== Save Raw ==*/
#if C_SENSOR_SAVE_RAW_DATA
INT32S
lib_sensor_write_raw_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;
	INT8U  save_done = 0;

	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	FB_CFG* pPscalerOutCfg = &pVid_info->jpeg_rec_args.fb_pscaler_out_cfg;

	CdspRawPara_t	CaptureArg;
	CdspRawPara_t	*pCaptureArg = &CaptureArg;

	pCaptureArg->InWidth = pVid_info->sensor_width;
	pCaptureArg->InHeight = pVid_info->sensor_height;
	pCaptureArg->raw_bit = 10;
	pCaptureArg->capture_flag = 0;
	pCaptureArg->save_flag = 0;
	pCaptureArg->CaptureMode = MODE_PURERAW; // MODE_PURERAW??MODE_LENSCMP??MODE_WBGAIN??MODE_LSC_WBGAIN

	pCaptureArg->ImageSize = ((pCaptureArg->InWidth * (pCaptureArg->InHeight + 16)) * pCaptureArg->raw_bit / 8);

	DBG_PRINT("=>raw %d,%d, size=%d\r\n",pCaptureArg->InWidth,pCaptureArg->InHeight,pCaptureArg->ImageSize);

	if (pCaptureArg->ImageSize & 0x3F)
	{
		pCaptureArg->ImageSize = (pCaptureArg->ImageSize + 0x3F) >> 6 << 6; //64 byte alignment for hw constrain
		if(pCaptureArg->ImageSize > pPscalerOutCfg->fb_cfg_size)
		{
			pCaptureArg->ImageSize = pPscalerOutCfg->fb_cfg_size;
		}
	}
	//DBG_PRINT("=>fb_cfg_size=%d\r\n",pPscalerOutCfg->fb_cfg_size);
	#if C_VID_REC_USE_SYS_FIFO
	if(pPscalerStreamCtrl->flag_sys_fifo)
	{
		pCaptureArg->Rawbuff = lib_encode_system_fifo_unmapping(src, (INT32U)pPscalerOutCfg->fb_cfg_head_addr);
	}
	else
	#endif
	{
		pCaptureArg->Rawbuff = (INT32U)pPscalerOutCfg->fb_cfg_head_addr;
	}

	if(sensor_preview_wait_stop() != STATUS_OK)
	{
		ERR_PRINT("[ERROR] preview stop fail \r\n");
	}

#if 1
	drv_l2_capture_raw_set_cfg(pCaptureArg);

	do
	{
		save_done = drv_l2_capture_raw_save_get();
		osDelay(10);
	} while(!save_done);

	drv_l2_capture_raw_save_set(0);
#endif

	cache_invalid_range(pCaptureArg->Rawbuff, pCaptureArg->ImageSize);
	ap_storage_service_write_file(pVid_info->rec_file_handle,pCaptureArg->Rawbuff, pCaptureArg->ImageSize);

	DBG_PRINT("=>write file done \r\n");


	ap_storage_service_close_file(pVid_info->rec_file_handle);
	photo_capture_mem_free(src);

	/*
		Because "lib_pscaler_stream_init" reset value to defalut
		Put "lib_encode_buf_release" here until all source finish their work
	*/
	lib_encode_buf_release(src);

	// Restart preview
	app_msg_send(MSG_STORAGE_SERVICE_PIC_DONE, &nRet, sizeof(INT32S), MSG_PRI_NORMAL);
	sensor_preview_start();

	return STATUS_OK;
}
#endif

/*== HOST USB MJPEG CAM ==*/
#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
INT32S
lib_host_2K_decode_JPEG_finish_check(
	void
)
{
	#if (_SENSOR_USB_MJPEG_CAM_FRONT)
	VID_INFO*  pVid_info_front = &pStreamArgs->vid_info_front;
	#endif

	#if (_SENSOR_USB_MJPEG_CAM_REAR)
	VID_INFO*  pVid_info_rear = &pStreamArgs->vid_info_rear;
	#endif

	HOST_USB_MJPEG_DEC_ARGS* pMjpeg_dec_args;
	JpegEnc_t*pJpegDec = NULL;

	#if (_SENSOR_USB_MJPEG_CAM_FRONT)
	if(pVid_info_front->data_source == VID_SRC_USB)
	{
		pMjpeg_dec_args = &pVid_info_front->host_usb_mjpeg_dec_args;
		pJpegDec = &pMjpeg_dec_args->jpeg_cfg;
		if(jpeg_decode_status_get(pJpegDec->hwidx))
		{// Not finish doing it yet
			return STATUS_FAIL;
		}
	}
	#endif

	#if (_SENSOR_USB_MJPEG_CAM_REAR)
	if(pVid_info_rear->data_source == VID_SRC_USB)
	{
		pMjpeg_dec_args = &pVid_info_rear->host_usb_mjpeg_dec_args;
		pJpegDec = &pMjpeg_dec_args->jpeg_cfg;
		if(jpeg_decode_status_get(pJpegDec->hwidx))
		{// Not finish doing it yet
			return STATUS_FAIL;
		}
	}
	#endif

	return STATUS_OK;
}

static INT32U
uvc_id_to_src(
	INT32U host_id
)
{
	#if (_SENSOR_USB_MJPEG_CAM_FRONT)
	if(host_id == HOST_UVC_PORT_FRONT)
	{
		return PREVIEW_SRC_FRONT;
	}
	#endif

	#if (_SENSOR_USB_MJPEG_CAM_REAR)
	if(host_id == HOST_UVC_PORT_REAR)
	{
		return PREVIEW_SRC_REAR;
	}
	#endif
}

static INT8U
uvc_src_to_id(
	INT32U src
)
{
	#if (_SENSOR_USB_MJPEG_CAM_FRONT)
	if(src == PREVIEW_SRC_FRONT)
	{
		return HOST_UVC_PORT_FRONT;
	}
	#endif

	#if (_SENSOR_USB_MJPEG_CAM_REAR)
	if(src == PREVIEW_SRC_REAR)
	{
		return HOST_UVC_PORT_REAR;
	}
	#endif
}

void
lib_uvc_q_init(
	INT32U src,
	ENCODE_PARAMS* pEnc_params
)
{
	INT8U 	i;
	INT32U	uvc_buf_addr;
	INT32U 	cb_buf_cnt = pEnc_params->h264_I_out_cnt;
	INT32U 	cb_buf_size = pEnc_params->h264_I_out_size;

	uvc_ready_q_idx[src] = 0;

	for(i=HOST_UVC_Q_IDLE; i<HOST_UVC_Q_MAX; i++)
	{
		#if (_SENSOR_USB_MJPEG_CAM_FRONT)
		if(src == PREVIEW_SRC_FRONT)
		{
			cbuf_host_uvc_q_handle[src][i] = circular_buf_init((INT32U*)host_uvc_q_buf_FRONT[i],cb_buf_cnt);
		}
		#endif

		#if (_SENSOR_USB_MJPEG_CAM_REAR)
		if(src == PREVIEW_SRC_REAR)
		{
			cbuf_host_uvc_q_handle[src][i] = circular_buf_init((INT32U*)host_uvc_q_buf_REAR[i],cb_buf_cnt);
		}
		#endif

		if(cbuf_host_uvc_q_handle[src][i] == 0)
		{
			ERR_PRINT("fail to cbuf_host_uvc_q_handle[%d][%d]\r\n",src,i);
		}
	}

	if((cbuf_host_uvc_q_handle[src][HOST_UVC_Q_IDLE] == 0) ||
		(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_READY] == 0) ||
		(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_DEC_DONE] == 0)
	)
	{
		if(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_IDLE])
		{
			circular_buf_free(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_IDLE]);
			cbuf_host_uvc_q_handle[src][HOST_UVC_Q_IDLE] = 0;
		}

		if(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_READY])
		{
			circular_buf_free(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_READY]);
			cbuf_host_uvc_q_handle[src][HOST_UVC_Q_READY] = 0;
		}

		if(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_DEC_DONE])
		{
			circular_buf_free(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_DEC_DONE]);
			cbuf_host_uvc_q_handle[src][HOST_UVC_Q_DEC_DONE] = 0;
		}

		return;
	}

	uvc_buf_addr = (INT32U)gp_malloc_align(cb_buf_cnt*cb_buf_size,64);
	if(!uvc_buf_addr)
	{
		return;
	}

	for(i = 0; i < cb_buf_cnt; i++)
	{
		#if 1
		DBG_PRINT("[HOST_UVC_Q_IDLE]i:%d put 0x%x\r\n",i,(uvc_buf_addr+(cb_buf_size*i)));
		#endif

		if(circular_buf_try_put(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_IDLE], (unsigned int)(uvc_buf_addr+(cb_buf_size*i))) < 0)
		{
			ERR_PRINT("fail to put cbuf_host_uvc_q_handle[%d][HOST_UVC_Q_IDLE]\r\n",src);
			return;
		}
	}
}

#if (_SENSOR_USB_MJPEG_CAM_FRONT)
void
front_uvc_q_init(
	void
)
{
	ENCODE_PARAMS uvc_param;
	gp_memset((INT8S *)&uvc_param,0,sizeof(ENCODE_PARAMS));

	uvc_param.h264_I_out_cnt = HOST_USB_MJPEG_Q_CNT_FRONT;
	uvc_param.h264_I_out_size = HOST_USB_MJPEG_BUF_SIZE_FRONT;

	lib_uvc_q_init(PREVIEW_SRC_FRONT,&uvc_param);
}
#endif

#if (_SENSOR_USB_MJPEG_CAM_REAR)
void
rear_uvc_q_init(
	void
)
{
	ENCODE_PARAMS uvc_param;
	gp_memset((INT8S *)&uvc_param,0,sizeof(ENCODE_PARAMS));

	uvc_param.h264_I_out_cnt = HOST_USB_MJPEG_Q_CNT_REAR;
	uvc_param.h264_I_out_size = HOST_USB_MJPEG_BUF_SIZE_REAR;

	lib_uvc_q_init(PREVIEW_SRC_REAR,&uvc_param);
}
#endif

void
uvc_q_init(
	void
)
{
	#if (_SENSOR_USB_MJPEG_CAM_FRONT)
	front_uvc_q_init();
	#endif

	#if (_SENSOR_USB_MJPEG_CAM_REAR)
	rear_uvc_q_init();
	#endif
}

INT32S
uvc_q_put_idle(
	INT32U host_id,
	INT32U buf
)
{
	INT32S nRet = STATUS_OK;
	INT8U src = uvc_id_to_src(host_id);

	//DBG_PRINT("put_idle buf:0x%x\r\n",buf);

	if(circular_buf_try_put(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_IDLE], (unsigned int)buf) < 0)
	{
		ERR_PRINT("fail to put cbuf_host_uvc_q_handle[%d][HOST_UVC_Q_IDLE]\r\n",src);
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}

INT32S
uvc_q_get_idle(
	INT32U host_id,
	INT32U *buf
)
{
	INT32S nRet;
	unsigned int data;
	INT8U src = uvc_id_to_src(host_id);

	nRet = circular_buf_get(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_IDLE], &data);
	if(nRet < 0)
	{
		*buf = DUMMY_BUFFER_ADDRS;
		nRet = STATUS_FAIL;
	}
	else
	{
		*buf = data;
		nRet = STATUS_OK;
	}

	//DBG_PRINT("[idle]getbuf:0x%x\r\n",*buf);

	return nRet;
}

#if (_SENSOR_USB_MJPEG_CAM_FRONT)
INT32S
front_uvc_q_put_ready(
	INT32U buf,
	INT32U size
)
{
	INT32S nRet = STATUS_OK;
	INT32U src = PREVIEW_SRC_FRONT;

	//DBG_PRINT("[rdy]putbuf:0x%x\r\n",buf);

	if(buf == 0)
	{
		return STATUS_OK;
	}

	uvc_put_ready_FRONT[uvc_ready_q_idx[src]].buf = buf;
	uvc_put_ready_FRONT[uvc_ready_q_idx[src]].size = size;

	//DBG_PRINT("put 0x%x\r\n",(unsigned int)&uvc_put_ready[uvc_ready_q_idx]);

	if(circular_buf_try_put(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_READY], (unsigned int)&uvc_put_ready_FRONT[uvc_ready_q_idx[src]]) < 0)
	{
		ERR_PRINT("fail to put cbuf_host_uvc_q_handle[%d][HOST_UVC_Q_READY]\r\n",src);
		RETURN(STATUS_FAIL);
	}

	if(++uvc_ready_q_idx[src] >= HOST_USB_MJPEG_Q_CNT_FRONT)
	{
		uvc_ready_q_idx[src] = 0;
	}

Return:
	return nRet;
}
#endif

#if (_SENSOR_USB_MJPEG_CAM_REAR)
INT32S
rear_uvc_q_put_ready(
	INT32U buf,
	INT32U size
)
{
	INT32S nRet = STATUS_OK;
	INT32U src = PREVIEW_SRC_REAR;

	//DBG_PRINT("[rdy]putbuf:0x%x\r\n",buf);

	if(buf == 0)
	{
		return STATUS_OK;
	}

	uvc_put_ready_REAR[uvc_ready_q_idx[src]].buf = buf;
	uvc_put_ready_REAR[uvc_ready_q_idx[src]].size = size;

	//DBG_PRINT("put 0x%x\r\n",(unsigned int)&uvc_put_ready[uvc_ready_q_idx]);

	if(circular_buf_try_put(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_READY], (unsigned int)&uvc_put_ready_REAR[uvc_ready_q_idx[src]]) < 0)
	{
		ERR_PRINT("fail to put cbuf_host_uvc_q_handle[%d][HOST_UVC_Q_READY]\r\n",src);
		RETURN(STATUS_FAIL);
	}

	if(++uvc_ready_q_idx[src] >= HOST_USB_MJPEG_Q_CNT_REAR)
	{
		uvc_ready_q_idx[src] = 0;
	}

Return:
	return nRet;
}
#endif

INT32S
uvc_q_put_ready(
	INT32U host_id,
	INT32U buf,
	INT32U size
)
{
	INT32S nRet = STATUS_OK;
	INT8U src = uvc_id_to_src(host_id);

	#if (_SENSOR_USB_MJPEG_CAM_FRONT)
	if(src == PREVIEW_SRC_FRONT)
	{
 		nRet = front_uvc_q_put_ready(buf,size);
	}
	#endif

	#if (_SENSOR_USB_MJPEG_CAM_REAR)
	if(src == PREVIEW_SRC_REAR)
	{
		nRet = rear_uvc_q_put_ready(buf,size);
	}
	#endif

	host_2K_mjpeg_ready_notify();

	return nRet;
}

INT32S
uvc_q_get_ready(
	INT32U src,
	INT32U *buf,
	INT32U *size
)
{
	INT32S nRet;
	unsigned int data;
	struct uvc_ready_q* uvc_get_ready;

	nRet = circular_buf_get(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_READY], &data);
	if(nRet < 0)
	{
		*buf = 0;
		*size = 0;
		nRet = STATUS_FAIL;
	}
	else
	{
		uvc_get_ready = (struct uvc_ready_q*)data;
		*buf = uvc_get_ready->buf;
		*size = uvc_get_ready->size;
		nRet = STATUS_OK;
	}

	//DBG_PRINT("[get]buf:0x%x size:%d nRet:%d\r\n",*buf,*size,nRet);

	return nRet;
}

#if (_SENSOR_USB_MJPEG_CAM_FRONT)
INT8U
front_uvc_q_ready_is_empty(
	void
)
{
	INT8U queue_empty;
	INT32U src = PREVIEW_SRC_FRONT;

	osENTER_CRITICAL();
	queue_empty = circular_buf_size(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_READY]);
	osEXIT_CRITICAL();

	return queue_empty;
}
#endif

#if (_SENSOR_USB_MJPEG_CAM_REAR)
INT8U
rear_uvc_q_ready_is_empty(
	void
)
{
	INT8U queue_empty;
	INT32U src = PREVIEW_SRC_REAR;

	osENTER_CRITICAL();
	queue_empty = circular_buf_size(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_READY]);
	osEXIT_CRITICAL();

	return queue_empty;
}
#endif

INT8U
uvc_q_ready_is_empty(
	void
)
{
	INT32U queue_empty = 0;

	#if (_SENSOR_USB_MJPEG_CAM_FRONT)
	queue_empty += front_uvc_q_ready_is_empty();
	#endif

	#if (_SENSOR_USB_MJPEG_CAM_REAR)
	queue_empty += rear_uvc_q_ready_is_empty();
	#endif

	if(queue_empty)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

void
uvc_release_keep_buf(
	INT32U host_id
)
{
	INT8U src = uvc_id_to_src(host_id);
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	INT32U 	keep_jpeg_buf = 0;

	if(pVid_info)
	{
		keep_jpeg_buf = pVid_info->host_usb_mjpeg_dec_args.keep_jpeg_buf;
		//DBG_PRINT("rls keep:%x\r\n",keep_jpeg_buf);

		if(keep_jpeg_buf && (keep_jpeg_buf != black_jpg[src]))
		{
			uvc_q_put_idle(host_id,keep_jpeg_buf);
			pVid_info->host_usb_mjpeg_dec_args.keep_jpeg_buf = 0;
		}
	}
}

INT32S
uvc_q_put_dec_done(
	INT32U src,
	INT32U buf
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("put_idle buf:0x%x\r\n",buf);

	if(circular_buf_try_put(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_DEC_DONE], (unsigned int)buf) < 0)
	{
		ERR_PRINT("fail to put cbuf_host_uvc_q_handle[HOST_UVC_Q_DEC_DONE]\r\n");
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}

INT32U
uvc_q_get_dec_done(
	INT32U src
)
{
	int nRet;
	unsigned int data = DUMMY_BUFFER_ADDRS;

	nRet = circular_buf_get(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_DEC_DONE], &data);
	if(nRet < 0)
	{
        //DBG_PRINT(" x");
		return DUMMY_BUFFER_ADDRS;
	}

	//DBG_PRINT("get buf:0x%x\r\n",data);

	return data;
}

INT32U
uvc_q_get_dec_done_content_size_get(
	INT32U src
)
{
	return circular_buf_size(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_DEC_DONE]);
}

static void
lib_host_usb_mjpeg_decode_Callback(
	JPEG_IDX idx,
	INT32S event,
	INT32U count
)
{
	PSCALER_STREAM_CTRL_T* pPsaclerStream;
	VID_INFO* pVid_info;
	HOST_USB_MJPEG_DEC_ARGS* pMjpegDecArgs;
	JpegEnc_t*pJpegDec;
	INT8U i;

	//DBG_PRINT("[Host]( %d,%d,%d)",idx,event,count);

	// Check the handle
	for(i=PREVIEW_SRC_FRONT; i<=PREVIEW_SRC_REAR; i++)
	{
		pPsaclerStream = &pscaler_stream[i];
		pVid_info = pPsaclerStream->pVid_info;

		if(!pVid_info) // Source path does not exists
		{
			continue;
		}

		pMjpegDecArgs = &pVid_info->host_usb_mjpeg_dec_args;

		if(pMjpegDecArgs->flag_jpeg_trigger)
		{
			pJpegDec = &pMjpegDecArgs->jpeg_cfg;
			if(pJpegDec->hwidx != idx)
			{
				continue;
			}

			pMjpegDecArgs->flag_jpeg_trigger = 0;

			pJpegDec->jpeg_status = event;

			host_2K_frame_ready_notify(i);
		}
	}

}

INT32S
lib_host_usb_mjpeg_check_header(
	INT32U hwidx,
	INT32U raw_data_addr,
	INT32U raw_data_size
)
{
	INT32S nRet = STATUS_OK;

	jpeg_decode_init(hwidx);
	nRet = jpeg_decode_parse_header(hwidx, (INT8U *)raw_data_addr, raw_data_size);
	jpeg_decode_stop(hwidx);
	if(nRet != JPEG_PARSE_OK)
	{
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

INT32S
lib_host_usb_mjpeg_decode_output(
	JpegDec_t *pJpegDec
)
{
	INT32S nRet = STATUS_OK;
	INT8U *p_vlc;
	INT32U fly_len, header_len;

	jpeg_decode_init(pJpegDec->hwidx);
	nRet = jpeg_decode_parse_header(pJpegDec->hwidx, (INT8U *) pJpegDec->raw_data_addr, pJpegDec->raw_data_size);
	if(nRet != JPEG_PARSE_OK) {
		RETURN(STATUS_FAIL);
	}

	nRet = jpeg_decode_output_set(pJpegDec->hwidx, pJpegDec->output_addr, DUMMY_BUFFER_ADDRS, DUMMY_BUFFER_ADDRS, C_JPG_FIFO_DISABLE);
	if(nRet < 0) {
		DBG_PRINT("jpeg_decode_output_set() failed\r\n");
		RETURN(STATUS_FAIL);
	}


	p_vlc = jpeg_decode_image_vlc_addr_get(pJpegDec->hwidx);
	header_len = ((INT32U) p_vlc) - pJpegDec->raw_data_addr;
	fly_len = pJpegDec->raw_data_size - header_len;

	pJpegDec->jpeg_yuv_mode = jpeg_decode_image_yuv_mode_get(pJpegDec->hwidx);
	pJpegDec->jpeg_valid_w = jpeg_decode_image_width_get(pJpegDec->hwidx);
	pJpegDec->jpeg_valid_h = jpeg_decode_image_height_get(pJpegDec->hwidx);
	pJpegDec->jpeg_extend_w = jpeg_decode_image_extended_width_get(pJpegDec->hwidx);
	pJpegDec->jpeg_extend_h = jpeg_decode_image_extended_height_get(pJpegDec->hwidx);

	#if 0
	DBG_PRINT("jpeg valid = %dx%d\r\n", pJpegDec->jpeg_valid_w, pJpegDec->jpeg_valid_h);
	DBG_PRINT("jpeg extend = %dx%d\r\n", pJpegDec->jpeg_extend_w, pJpegDec->jpeg_extend_h);
	#endif

	// use union mode
	drv_l1_jpeg_using_union_mode_enable(pJpegDec->hwidx);
	drv_l1_jpeg_decode_scale_down_set(pJpegDec->hwidx, ENUM_JPG_NO_SCALE_DOWN);
	drv_l1_jpeg_out_format_set(pJpegDec->hwidx, C_JPG_DECOUT_FMT_GP420);

	jpeg_decode_isr_callback_set(pJpegDec->hwidx,lib_host_usb_mjpeg_decode_Callback);

	nRet = jpeg_decode_once_start(pJpegDec->hwidx, p_vlc, (fly_len << 1));
	if(nRet < 0)
	{
		RETURN(STATUS_FAIL);
	}

	return STATUS_OK;

Return:
	jpeg_decode_stop(pJpegDec->hwidx);
	return nRet;
}

#if (defined REVERSING_CHECK_JPEG_HEADER_REAR) && (REVERSING_CHECK_JPEG_HEADER_REAR == CUSTOM_ON)
static INT32U pre_parking_status = 0;
INT32U back_car_detect = 0;
#endif

INT32S
lib_host_usb_mjpeg_decode_JPEG_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	HOST_USB_MJPEG_DEC_ARGS* pMjpeg_dec_args = NULL;
	JpegDec_t jpegDecArgs;
	INT32U mjpeg_buf = 0;
	INT32U mjpeg_size = 0;
	INT32U host_id = uvc_src_to_id(src);
	INT32U usb_host_plug = 0;

    #if (defined REVERSING_CHECK_JPEG_HEADER_REAR) && (REVERSING_CHECK_JPEG_HEADER_REAR == CUSTOM_ON)
    char* jpg_buf = NULL;
    #endif

	//DBG_PRINT("==Run:%s==0x%x\r\n",__FUNCTION__,pVid_info);

	#if (_SENSOR_USB_MJPEG_CAM_FRONT)
	if(host_id == HOST_UVC_PORT_FRONT)
	{
		usb_host_plug = 1;
	}
	#endif
	#if (_SENSOR_USB_MJPEG_CAM_REAR)
	if(host_id == HOST_UVC_PORT_REAR)
	{
		usb_host_plug = back_sensor_plug_status_get();
	}
	#endif

	if(	!pVid_info ||
		!usb_host_plug ||
		(sensor_preview_status_get() == PREVIEW_MODE_STOP_WAIT)||
		(sensor_preview_status_get() == PREVIEW_MODE_STOP)
	)
	{
		if(uvc_q_get_ready(src,&mjpeg_buf, &mjpeg_size) != STATUS_FAIL && mjpeg_buf && mjpeg_size)
		{
			uvc_q_put_idle(host_id,mjpeg_buf);
		}

		uvc_release_keep_buf(host_id);

		return STATUS_OK;
	}

	// Check jpeg engine status
	if(lib_encode_engine_path_ctrl(src,VID_DEC_JPEG,ENC_ENGINE_ACT_CHECK) == STATUS_FAIL)
	{
		//DBG_PRINT("=>DJ_Busy(%d)\r\n",src);
		return STATUS_OK;
	}

	memset((void *)&jpegDecArgs, 0, sizeof(jpegDecArgs));

	// Get pscaler's display buffer
	jpegDecArgs.output_addr = lib_pscaler_get_bypass(src);
	if(jpegDecArgs.output_addr == DUMMY_BUFFER_ADDRS)
	{
		// no space to store jpeg output data
		DBG_PRINT("=>no space\r\n");
		
		return STATUS_FAIL;
	}

 	pMjpeg_dec_args= &pVid_info->host_usb_mjpeg_dec_args;
	pMjpeg_dec_args->flag_black_jpeg = 0;

	// Get a jpeg from host usb
	nRet = uvc_q_get_ready(src,&mjpeg_buf,&mjpeg_size);
	if(nRet == STATUS_FAIL)
	{
		if(pMjpeg_dec_args->keep_jpeg_buf)
		{
			// decode a previous jpeg
			//DBG_PRINT("dec keep:%x\r\n",(INT32U)pMjpeg_dec_args->keep_jpeg_buf);
			mjpeg_buf = pMjpeg_dec_args->keep_jpeg_buf;
			mjpeg_size = pMjpeg_dec_args->keep_jpeg_buf_size;
		}
		else
		{
			// decode a black jpeg
			if(pVid_info->sensor_width == 1280)
			{
				black_jpg[src] = (INT32U)black_jpg_720P;
				black_jpg_len[src] = black_jpg_len_720P;
			}
			else
			{
				black_jpg[src] = (INT32U)black_jpg_VGA;
				black_jpg_len[src] = black_jpg_len_VGA;
			}

			mjpeg_buf = black_jpg[src];
			mjpeg_size = black_jpg_len[src];
			pMjpeg_dec_args->flag_black_jpeg = 1;
		}
	}

	// Check jpeg header to avoid illegal jpeg
	nRet = lib_host_usb_mjpeg_check_header(pMjpeg_dec_args->jpeg_cfg.hwidx,mjpeg_buf,mjpeg_size);
	if(nRet == STATUS_FAIL)
	{
		//DBG_PRINT("=>check header fail\r\n");
		// Release host usb mjpeg buffer, but pass black jpeg and keep jpeg
		if(mjpeg_buf && mjpeg_size && !pMjpeg_dec_args->flag_black_jpeg && (mjpeg_buf != pMjpeg_dec_args->keep_jpeg_buf))
		{
			uvc_q_put_idle(host_id,mjpeg_buf);
		}

		if(pMjpeg_dec_args->keep_jpeg_buf)
		{
			// decode a previous jpeg
			//DBG_PRINT("dec keep:%x\r\n",(INT32U)pMjpeg_dec_args->keep_jpeg_buf);
			mjpeg_buf = pMjpeg_dec_args->keep_jpeg_buf;
			mjpeg_size = pMjpeg_dec_args->keep_jpeg_buf_size;
		}
		else
		{
			// decode a black jpeg
			mjpeg_buf = black_jpg[src];
			mjpeg_size = black_jpg_len[src];
			pMjpeg_dec_args->flag_black_jpeg = 1;
		}

	}

	//DBG_PRINT("=>out:0x%x\r\n",jpegDecArgs.output_addr);

	pMjpeg_dec_args->jpeg_cfg.input_buffer_y = mjpeg_buf;
	pMjpeg_dec_args->jpeg_cfg.input_y_len = mjpeg_size;
	pMjpeg_dec_args->jpeg_cfg.output_buffer = jpegDecArgs.output_addr;
	pMjpeg_dec_args->flag_jpeg_trigger = 1;

	jpegDecArgs.hwidx = pMjpeg_dec_args->jpeg_cfg.hwidx;
	jpegDecArgs.raw_data_addr = mjpeg_buf;
	jpegDecArgs.raw_data_size = mjpeg_size;
	jpegDecArgs.output_format = C_SCALER_CTRL_OUT_GP420;

	lib_encode_engine_path_ctrl(src,VID_DEC_JPEG,ENC_ENGINE_ACT_SET);

	nRet = lib_host_usb_mjpeg_decode_output(&jpegDecArgs);
	if(nRet == STATUS_FAIL)
	{
		pMjpeg_dec_args->flag_jpeg_trigger = 0;
		lib_encode_engine_path_ctrl(src,VID_DEC_JPEG,ENC_ENGINE_ACT_CLEAR);
		RETURN(STATUS_FAIL);
	}

#if (defined REVERSING_CHECK_JPEG_HEADER_REAR) && (REVERSING_CHECK_JPEG_HEADER_REAR == CUSTOM_ON)
	if((pMjpeg_dec_args->flag_black_jpeg) || (mjpeg_buf == pMjpeg_dec_args->keep_jpeg_buf))
	{
		back_car_detect = 0;
		return STATUS_OK;
	}

    jpg_buf = (char*)mjpeg_buf;

    if(back_car_detect > 0)
    {
        //DBG_PRINT("BACK CAR[%d,0x%x]\r\n",jpg_buf[0x20]&0x1,jpg_buf[0x20]);
        if((jpg_buf[0x20]&0x1) &&(pre_parking_status == 0))
        {
            back_car_detect++;
            if(back_car_detect == 5)
            {
                DBG_PRINT("BACK CAR ON!\r\n");
                back_car_detect = 0;
                pre_parking_status = 1;
                msgQSend(ApQ, MSG_APQ_BACK_A_CAR_ON, NULL, 0, MSG_PRI_NORMAL);
            }
        }else if(((jpg_buf[0x20]&0x1) == 0) &&(pre_parking_status == 1))
        {
            back_car_detect++;
            if(back_car_detect == 5)
            {
                back_car_detect = 0;
                DBG_PRINT("BACK CAR OFF!\r\n");
                pre_parking_status = 0;
                msgQSend(ApQ, MSG_APQ_BACK_A_CAR_OFF, NULL, 0, MSG_PRI_NORMAL);
            }
        }
        else
        back_car_detect = 0;
    }
#endif

	return STATUS_OK;

Return:

	// Release host usb mjpeg buffer, but pass black jpeg and keep jpeg
	if(mjpeg_buf && mjpeg_size && !pMjpeg_dec_args->flag_black_jpeg && (mjpeg_buf != pMjpeg_dec_args->keep_jpeg_buf))
	{
		uvc_q_put_idle(host_id,mjpeg_buf);
	}

	if(jpegDecArgs.output_addr != DUMMY_BUFFER_ADDRS)
	{
		// Release jpeg decode output buffer (pscaler bypass buffer)
		lib_pscaler_free_bypass(src,jpegDecArgs.output_addr);
	}

	return nRet;
}

static INT32S
preview_image_put(
	INT32U src,
	INT32U buf
)
{
	INT32U scaler = preview_src_to_ps(src);

	//DBG_PRINT("==Run:%s==mode:%d==\r\n",__FUNCTION__,_preview_mode);

	if(_preview_mode == PREVIEW_MODE_STOP_WAIT)
	{
		return STATUS_FAIL;
	}

	if(_preview_mode == PREVIEW_MODE_STOP)
	{
		return STATUS_FAIL;
	}

	if(pscaler_mode[scaler] != PSCALER_MODE_DRAM)
	{
		return STATUS_FAIL;
	}

	if(pscaler_busy[scaler])
	{
		return STATUS_FAIL;
	}

	/*
		Reinit pscaler's output width/height after _preview_mode changes
		from PREVIEW_ACT_SWITCH to target mode
	*/

	if(!preview_switch_init[scaler] && (_preview_mode != PREVIEW_ACT_SWITCH))
	{
		preview_pscaler_parm_set(scaler, _preview_mode, 1, NULL);
	}

	drv_l1_pscaler_input_buffer_set(scaler, buf);
	pscaler_busy[scaler] = 1;

	drv_l1_pscaler_disable(scaler);
	drv_l1_pscaler_start(scaler);

	return STATUS_OK;
}

INT32S
lib_host_usb_mjpeg_frame_ready_process(
	INT32U src,
	INT32U do_scale_up
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	HOST_USB_MJPEG_DEC_ARGS* pMjpeg_dec_args = NULL;
	JpegEnc_t*pJpegDec = NULL;
	INT32U host_usb_frame_buf;
	INT32U host_usb_frame_content_size;
	INT32U host_id = uvc_src_to_id(src);

	//DBG_PRINT("==Run:%s==0x%x\r\n",__FUNCTION__,pVid_info);

 	pMjpeg_dec_args = &pVid_info->host_usb_mjpeg_dec_args;
	pJpegDec = &pMjpeg_dec_args->jpeg_cfg;

	// Stop jpeg engine
	jpeg_decode_stop(pJpegDec->hwidx);

	//DBG_PRINT("[%d] jpeg_status:0x%x\r\n",src,pJpegDec->jpeg_status);

	// The queue is not empty, so send message to decode JPEG
	if(uvc_q_ready_is_empty() == 0)
	{
		host_2K_mjpeg_ready_notify();
	}

	if((pJpegDec->jpeg_status & C_JPG_STATUS_DECODE_DONE) != C_JPG_STATUS_DECODE_DONE)
	{
		ERR_PRINT("[%d] jpeg_status:0x%x\r\n",src,pJpegDec->jpeg_status);

		if(!pMjpeg_dec_args->flag_black_jpeg && pJpegDec->input_buffer_y)
		{
			uvc_q_put_idle(host_id,pJpegDec->input_buffer_y);
	    }

		// Release jpeg decode output buffer (pscaler bypass buffer)
		lib_pscaler_free_bypass(src,pJpegDec->output_buffer);
		// Clear jpeg engine status to idle
		lib_encode_engine_path_ctrl(src,VID_DEC_JPEG,ENC_ENGINE_ACT_CLEAR);

		return STATUS_FAIL;
	}

	// Release host usb mjpeg buffer, but pass black jpeg
	if(!pMjpeg_dec_args->flag_black_jpeg && pJpegDec->input_buffer_y)
	{
		// release the old and keep the new
		if((pMjpeg_dec_args->keep_jpeg_buf) && (pJpegDec->input_buffer_y != pMjpeg_dec_args->keep_jpeg_buf))
		{
			uvc_q_put_idle(host_id,pMjpeg_dec_args->keep_jpeg_buf);
		}

		pMjpeg_dec_args->keep_jpeg_buf = pJpegDec->input_buffer_y;
		pMjpeg_dec_args->keep_jpeg_buf_size = pJpegDec->input_y_len;
		//DBG_PRINT("=>keep:%x\r\n",pMjpeg_dec_args->keep_jpeg_buf);		
	}

	// Clear jpeg engine status to idle
	lib_encode_engine_path_ctrl(src,VID_DEC_JPEG,ENC_ENGINE_ACT_CLEAR);

	//DBG_PRINT("[%d] outbuf:0x%x outlen:%d \r\n",src,pJpegDec->output_buffer,pJpegDec->output_buffer_len);
	if(uvc_q_put_dec_done(src,pJpegDec->output_buffer) == STATUS_FAIL)
	{
		// Release jpeg decode output buffer (pscaler bypass buffer)
		lib_pscaler_free_bypass(src,pJpegDec->output_buffer);
	}

	host_usb_frame_buf = uvc_q_get_dec_done(src);
	if(host_usb_frame_buf == DUMMY_BUFFER_ADDRS)
	{
		return STATUS_OK;
	}

	// Check the content size of DEC_DONE frame buffer
	host_usb_frame_content_size = uvc_q_get_dec_done_content_size_get(src);
	if(host_usb_frame_content_size)
	{
		do
		{
			if(lib_interp2K_scaler_preview_done_notify(src,host_usb_frame_buf) == STATUS_FAIL)
			{
				// Release pscaler bypass output buffer
				lib_pscaler_free_bypass(src,host_usb_frame_buf);
			}

			// Get the next DEC_DONE frame buffer and check the content size of DEC_DONE frame buffer
			host_usb_frame_buf = uvc_q_get_dec_done(src);
			host_usb_frame_content_size = uvc_q_get_dec_done_content_size_get(src);
		}while(host_usb_frame_content_size);
	}

	// Preview
	#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
	if(do_scale_up)
	{// Use Scaler_0
		nRet = interp2k_preview_image_put(src,host_usb_frame_buf);
	}
	else
	#endif
	{
		nRet = preview_image_put(src,host_usb_frame_buf);
	}

	if(nRet == STATUS_FAIL) // pscaler or scaler is busy, then pass output buffer to encode
	{
		// Release pscaler bypass buffer
		lib_pscaler_free_bypass(src,host_usb_frame_buf);
	}

	return nRet;
}


#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
void uvc_record_stop_req_set(INT8U src)
{
	if(src < PREVIEW_SRC_TOTAL)
	{
		uvc_record_stop_req[src] = 1;
	}
}

void uvc_record_stop_req_clear(INT8U src)
{
	if(src < PREVIEW_SRC_TOTAL)
	{
		uvc_record_stop_req[src] = 0;
	}
}

#endif

INT32S
lib_host_usb_mjpeg_preview_done_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	INT32U frame;

	//DBG_PRINT("==Run:%s==0x%x\r\n",__FUNCTION__,pVid_info);
	frame = lib_interp2K_scaler_preview_frame_get(src);
	if(frame == DUMMY_BUFFER_ADDRS)
	{
		//DBG_PRINT("=>DUMMY\r\n");
		return STATUS_OK;
	}

	//prevent recording desyncize when the wifi app crash or disconnected. 
	//do free frame buffer
	#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
		if(uvc_record_stop_req[src])
		{
			//DBG_PRINT("[UVC_REC_STOP_REQ] skip h264 src:%d frame:0x%x stream_enable:%d\r\n",src,frame,pPscalerStreamCtrl->enc_stream_info.stream_enable);
			lib_pscaler_free_bypass(src, frame);
			return STATUS_OK;
		}
	#endif

	// Start video recording
	if(pPscalerStreamCtrl->enc_stream_info.stream_enable == VIDEO_RECORD_START)
	{
		//DBG_PRINT("[HOST USB] VIDEO_RECORD_START:0x%x\r\n",frame);
		if(pPscalerStreamCtrl->ps_notify_func)
		{
			pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,frame);
			return STATUS_OK;
		}
	}
	else if(pPscalerStreamCtrl->enc_stream_info.stream_enable == VIDEO_RECORD_STOP)
	{
		//DBG_PRINT("[HOST USB] VIDEO_RECORD_STOP \r\n");
		pPscalerStreamCtrl->enc_stream_info.stream_enable = VIDEO_RECORD_NOTHING;

		if(pPscalerStreamCtrl->ps_notify_func)
		{
			pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,DUMMY_BUFFER_ADDRS);
		}
	}
	else if(pPscalerStreamCtrl->enc_stream_info.stream_enable == VIDEO_RECORD_PHOTO_START)
	{
		//DBG_PRINT("[HOST USB] VIDEO_RECORD_PHOTO_START:0x%x\r\n",frame);
		pPscalerStreamCtrl->enc_stream_info.stream_enable = VIDEO_RECORD_NOTHING;

		if(pPscalerStreamCtrl->ps_notify_func)
		{
			pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,frame);
		}
		if(msgQSend(pPscalerStreamCtrl->ps_notify_q, (MSG_VID_PSCALER_NOTIFY|src), NULL, 0, MSG_PRI_NORMAL) < 0)
		{
			ERR_PRINT("fail to msgQSend [%d]\r\n",src);
		}
		return STATUS_OK;
	}

	// Release jpeg decode output buffer (pscaler bypass buffer)
	lib_pscaler_free_bypass(src,frame);

	return nRet;
}
#endif

/*== HOST USB MJPEG CAM / INTERP2K IN COMMON ==*/
#if (_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K)
INT32S
lib_preview_buffer_release(
	INT32U src,
	VID_INFO* args
)
{
	INT32S nRet = STATUS_OK;
	FB_CFG* pPscalerOutCfg;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];

    #if (_SENSOR_USB_MJPEG_CAM)
	HOST_USB_MJPEG_DEC_ARGS* pMjpeg_dec_args = &args->host_usb_mjpeg_dec_args;
	INT32U host_id = uvc_src_to_id(src);
	#endif

	//DBG_PRINT("==Run:%s==[%d]\r\n",__FUNCTION__,src);

	switch(args->vid_rec_type)
	{
		case VID_REC_H264:
			pPscalerOutCfg = &args->h264_rec_args.fb_pscaler_out_cfg;
		break;
		case VID_REC_JPEG:
			pPscalerOutCfg = &args->jpeg_rec_args.fb_pscaler_out_cfg;
		break;
		default:
            RETURN(STATUS_FAIL);
		break;
	}

	// Pscaler bypass
	if(pPscalerOutCfg->fb_cfg_head_addr)
	{
		#if C_VID_REC_USE_SYS_FIFO
		pPscalerOutCfg->fb_cfg_head_addr = (void *)lib_encode_system_fifo_unmapping(src, (INT32U)pPscalerOutCfg->fb_cfg_head_addr);
		#endif

		#if (C_VID_INTERPOLATING_2K) && (C_VID_REC_USE_SYS_FIFO)
		if((args->run_2K_flow_src & HOST_2K_IS_REAR) && pPscalerStreamCtrl->flag_sys_fifo)
		{// It's for CIS output
        	lib_encode_system_fifo_release(src);
	        pPscalerStreamCtrl->flag_sys_fifo = 0;
		}
		#endif

		gp_free(pPscalerOutCfg->fb_cfg_head_addr);

		pPscalerOutCfg->fb_cfg_head_addr = 0;
	}
	if(pPscalerOutCfg->fb_cfg_cb_addr)
	{
		gp_free(pPscalerOutCfg->fb_cfg_cb_addr);
		pPscalerOutCfg->fb_cfg_cb_addr = 0;
	}

	if(cbuf_pscaler_bypass_out[src])
	{
		circular_buf_free(cbuf_pscaler_bypass_out[src]);
		cbuf_pscaler_bypass_out[src] = 0;
	}

	//DBG_PRINT("=>Release USBH_JEG_ID:%d\r\n",pMjpeg_dec_args->jpeg_cfg.hwidx);
    #if (_SENSOR_USB_MJPEG_CAM)
	release_used_jpeg(pMjpeg_dec_args->jpeg_cfg.hwidx);
	uvc_release_keep_buf(host_id);
	memset(pMjpeg_dec_args,0, sizeof(HOST_USB_MJPEG_DEC_ARGS));
	if(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_DEC_DONE])
	{
		circular_buf_reset(cbuf_host_uvc_q_handle[src][HOST_UVC_Q_DEC_DONE]);
	}
    #endif

	memset(&pPscalerStreamCtrl->enc_stream_info,0, sizeof(ENC_STREAM_INFO_T));
	pPscalerStreamCtrl->pVid_info = 0;

	#if (C_VID_INTERPOLATING_2K)
	if(args->run_2K_flow_src & HOST_2K_IS_REAR)
	#endif
	{
		if(cbuf_interp2K_handle[src][INTERP_2K_CSI_READY])
		{
			circular_buf_reset(cbuf_interp2K_handle[src][INTERP_2K_CSI_READY]);
		}
		if(cbuf_interp2K_handle[INTERP_2K_SCALER_READY])
		{
			circular_buf_reset(cbuf_interp2K_handle[src][INTERP_2K_SCALER_READY]);
		}
	}

	return STATUS_OK;

Return:

	return nRet;
}

INT32S
lib_preview_buffer_set(
	INT32U src,
	VID_INFO* args
)
{
	INT32S nRet = STATUS_OK;
	FB_CFG* pPscalerOutCfg;
	int i;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
    #if (_SENSOR_USB_MJPEG_CAM)
	HOST_USB_MJPEG_DEC_ARGS* pMjpeg_dec_args = &args->host_usb_mjpeg_dec_args;
	#endif

	/*
	16 alignment for hw constrain
	*/
	INT32U  h264_height_16_alignment = 0;
	INT32U  h264_extra_height = 0;
	INT32U  h264_extra_size = 0;
	INT32U  h264_original_size = 0;

	//DBG_PRINT("==Run:%s==[%d]\r\n",__FUNCTION__,src);

	switch(args->vid_rec_type)
	{
		case VID_REC_H264:
			pPscalerOutCfg = &args->h264_rec_args.fb_pscaler_out_cfg;
			/*
			16 alignment for hw constrain
			*/
			h264_height_16_alignment = (args->target_height + 0xF) >> 4 << 4;
			h264_extra_height = (h264_height_16_alignment-args->target_height);
			h264_original_size = ((args->target_width*args->target_height*3) >> 1);
			h264_extra_size = ((args->target_width*h264_extra_height*3) >> 1);
		break;
		case VID_REC_JPEG:
			pPscalerOutCfg = &args->jpeg_rec_args.fb_pscaler_out_cfg;
		break;
		default:
            RETURN(STATUS_FAIL);
		break;
	}

	pPscalerOutCfg->fb_cfg_count_num = 1;

	// Init pscaler bypass out
	if(pPscalerOutCfg->fb_cfg_blocks)
	{
		#if (C_VID_INTERPOLATING_2K) && (C_VID_REC_USE_SYS_FIFO)
		if(args->run_2K_flow_src & HOST_2K_IS_REAR)
		{ // It's for CSI output
			if(lib_encode_system_fifo_register(src) == STATUS_OK)
			{
				pPscalerStreamCtrl->flag_sys_fifo = 1;
			}

			if(pPscalerStreamCtrl->flag_sys_fifo)
			{
				pPscalerOutCfg->fb_cfg_head_addr = (void *)lib_encode_system_fifo_mapping(src, (INT32U)pPscalerOutCfg->fb_cfg_head_addr);
			}

			//DBG_PRINT("Pscaler:%d bypass :0x%x\r\n",src,pPscalerOutCfg->fb_cfg_head_addr);
		}
		#endif

		cbuf_pscaler_bypass_out[src] = circular_buf_init(pPscalerOutCfg->fb_cfg_cb_addr,pPscalerOutCfg->fb_cfg_blocks);
		if(cbuf_pscaler_bypass_out[src] == 0)
		{
			ERR_PRINT("fail to cbuf_pscaler_bypass_out[%d]\r\n",src);
			RETURN(STATUS_FAIL);
		}

		for(i = 0; i < pPscalerOutCfg->fb_cfg_blocks; i++)
		{
			#if 0
			DBG_PRINT("[pscaler:%d]i:%d put 0x%x\r\n",src,i,pPscalerOutCfg->fb_cfg_head_addr+(pPscalerOutCfg->fb_cfg_size*i));
			#endif

			/*
			16 alignment for hw constrain
			*/
			if(h264_extra_height)
			{
				#if 0
                DBG_PRINT("[extera:%d]i:%d put 0x%x\r\n",src,i,pPscalerOutCfg->fb_cfg_head_addr+h264_original_size+(pPscalerOutCfg->fb_cfg_size*i));
				#endif
				gp_memset((INT8S *)lib_encode_system_fifo_unmapping(src, (INT32U)pPscalerOutCfg->fb_cfg_head_addr+h264_original_size+(pPscalerOutCfg->fb_cfg_size*i)),0,h264_extra_size);
			}

			if(circular_buf_try_put(cbuf_pscaler_bypass_out[src], (unsigned int)(pPscalerOutCfg->fb_cfg_head_addr+(pPscalerOutCfg->fb_cfg_size*i))) < 0)
			{
				ERR_PRINT("fail to put cbuf_pscaler_bypass_out[%d]\r\n",src);
				RETURN(STATUS_FAIL);
			}
		}
	}

	// Set parameters
    #if (_SENSOR_USB_MJPEG_CAM)
	pMjpeg_dec_args->jpeg_cfg.hwidx = get_idle_jpeg(-1);
	pMjpeg_dec_args->jpeg_cfg.output_buffer_len = pPscalerOutCfg->fb_cfg_size;

	//DBG_PRINT("[%d]pMjpeg hwidx :%d \r\n",src,pMjpeg_dec_args->jpeg_cfg.hwidx);
	#endif

	pPscalerStreamCtrl->pVid_info = args;
	memset(&pPscalerStreamCtrl->enc_stream_info,0, sizeof(ENC_STREAM_INFO_T));

	return STATUS_OK;

Return:

	return nRet;
}

INT32S
front_preview_buffer_set(
	VID_INFO* args
)
{
	return(lib_preview_buffer_set(PREVIEW_SRC_FRONT,args));
}

INT32S
rear_preview_buffer_set(
	VID_INFO* args
)
{
	return(lib_preview_buffer_set(PREVIEW_SRC_REAR,args));
}

INT32S
lib_host_2K_decode_JPEG_process(
	void
)
{
	INT32S nRet = STATUS_OK;

	#if (_SENSOR_USB_MJPEG_CAM_FRONT)
	lib_host_usb_mjpeg_decode_JPEG_process(PREVIEW_SRC_FRONT);
	#endif

	#if (_SENSOR_USB_MJPEG_CAM_REAR)
	lib_host_usb_mjpeg_decode_JPEG_process(PREVIEW_SRC_REAR);
	#endif

	return nRet;
}

INT32S
lib_host_2K_frame_ready_process(
	INT8U src
)
{
	#if (C_VID_INTERPOLATING_2K)
	VID_INFO*  pVid_info_front = &pStreamArgs->vid_info_front;
	#endif
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	INT32U do_scale_up = 0;

	if(!pVid_info)
	{
		return STATUS_OK;
	}

	#if (C_VID_INTERPOLATING_2K)
	do_scale_up = pVid_info_front->run_2K_flow_src; // This value is Greater than 0, that means front do scale-up flow
	#endif

	#if (_SENSOR_USB_MJPEG_CAM)
	if(pVid_info->data_source == VID_SRC_USB)
	{
		lib_host_usb_mjpeg_frame_ready_process(src,do_scale_up);
	}
	#endif

	#if (C_VID_INTERPOLATING_2K) // Rear sensor is CSI
	if(pVid_info->run_2K_flow_src & HOST_2K_IS_REAR)
	{
		lib_interp2K_frame_ready_process(src,do_scale_up);
	}
	#endif

	return nRet;
}

INT32S
lib_host_2K_preview_done_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;

	if(!pVid_info)
	{
		return STATUS_OK;
	}

	#if (_SENSOR_USB_MJPEG_CAM)
	if(pVid_info->data_source == VID_SRC_USB)
	{
		lib_host_usb_mjpeg_preview_done_process(src);
	}
	#endif

	#if (C_VID_INTERPOLATING_2K) // Rear sensor is CSI
	if(pVid_info->run_2K_flow_src & HOST_2K_IS_REAR)
	{
		lib_interp2K_preview_done_process(src);
	}
	#endif

	return nRet;
}

void
lib_interp2K_q_init(
	INT32U src,
	ENCODE_PARAMS* pEnc_params
)
{
	INT8U i;
	INT32U 	cb_buf_cnt = pEnc_params->h264_I_out_cnt;

	for(i=INTERP_2K_CSI_READY; i<INTERP_2K_Q_MAX; i++)
	{
		#if (_SENSOR_USB_MJPEG_CAM_FRONT)
		if(src == PREVIEW_SRC_FRONT)
		{
			cbuf_interp2K_handle[src][i] = circular_buf_init(interp2K_q_buf_FRONT[i],cb_buf_cnt);
		}
		#endif

		#if (_SENSOR_USB_MJPEG_CAM_REAR) || (C_VID_INTERPOLATING_2K)
		if(src == PREVIEW_SRC_REAR)
		{
			cbuf_interp2K_handle[src][i] = circular_buf_init(interp2K_q_buf_REAR[i],cb_buf_cnt);
		}
		#endif

		if(cbuf_interp2K_handle[src][i] == 0)
		{
			ERR_PRINT("fail to cbuf_interp2K_handle[%d]\r\n",i);
		}
	}

	if((cbuf_interp2K_handle[src][INTERP_2K_CSI_READY] == 0) || (cbuf_interp2K_handle[src][INTERP_2K_SCALER_READY] == 0))
	{
		if(cbuf_interp2K_handle[src][INTERP_2K_CSI_READY])
		{
			circular_buf_free(cbuf_interp2K_handle[src][INTERP_2K_CSI_READY]);
			cbuf_interp2K_handle[src][INTERP_2K_CSI_READY] = 0;
		}

		if(cbuf_interp2K_handle[src][INTERP_2K_SCALER_READY])
		{
			circular_buf_free(cbuf_interp2K_handle[src][INTERP_2K_SCALER_READY]);
			cbuf_interp2K_handle[src][INTERP_2K_SCALER_READY] = 0;
		}
	}
}


#if (_SENSOR_USB_MJPEG_CAM_FRONT)
void
front_interp2K_q_init(
	void
)
{
	ENCODE_PARAMS interp2K_param;
	gp_memset((INT8S *)&interp2K_param,0,sizeof(ENCODE_PARAMS));

	interp2K_param.h264_I_out_cnt = VID_PSCALER_OUT_CNT_FRONT;

	lib_interp2K_q_init(PREVIEW_SRC_FRONT,&interp2K_param);
}
#endif

#if (_SENSOR_USB_MJPEG_CAM_REAR) || (C_VID_INTERPOLATING_2K)
void
rear_interp2K_q_init(
	void
)
{
	ENCODE_PARAMS interp2K_param;
	gp_memset((INT8S *)&interp2K_param,0,sizeof(ENCODE_PARAMS));

	interp2K_param.h264_I_out_cnt = VID_PSCALER_OUT_CNT_REAR;

	lib_interp2K_q_init(PREVIEW_SRC_REAR,&interp2K_param);
}
#endif

void
interp2K_q_init(
	void
)
{
	#if (_SENSOR_USB_MJPEG_CAM_FRONT)
	front_interp2K_q_init();
	#endif

	#if (_SENSOR_USB_MJPEG_CAM_REAR) || (C_VID_INTERPOLATING_2K)
	rear_interp2K_q_init();
	#endif
}

INT32S
lib_interp2K_scaler_preview_done_notify(
	INT32U src,
	INT32U buf
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("put_idle buf:0x%x\r\n",buf);

	if(circular_buf_try_put(cbuf_interp2K_handle[src][INTERP_2K_SCALER_READY], buf) < 0)
	{
		ERR_PRINT("fail to put cbuf_interp2K_handle[%d]\r\n",INTERP_2K_SCALER_READY);
		RETURN(STATUS_FAIL);
	}

	if(host_2K_preview_done_notify(src) == STATUS_FAIL)
	{
		ERR_PRINT("fail to host_2K_preview_done_notify \r\n");
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}

INT32U
lib_interp2K_scaler_preview_frame_get(
	INT32U src
)
{
	int nRet;
	unsigned int data = DUMMY_BUFFER_ADDRS;

	nRet = circular_buf_get(cbuf_interp2K_handle[src][INTERP_2K_SCALER_READY], &data);
	if(nRet < 0)
	{
		return DUMMY_BUFFER_ADDRS;
	}

	//DBG_PRINT("get buf:0x%x\r\n",data);

	return data;
}
#endif

/*== INTERPOLATING 2K ==*/
#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
static void
interp2K_scale_up_err_handle(
	INT32U src,
	INT32U buf_idx,
	VID_INFO* pVid_info
)
{
	INT8U scale_up_done = 0;
	PSCALER_STREAM_CTRL_T* pPsaclerStream = NULL;
	#if (C_SUPORT_GET_JPG_BUF)
	GET_JPEG_ARGS* pGet_jpeg_args = &pVid_info->get_jpeg_args;
	#endif

	if((pVid_info->run_2K_flow_src & HOST_2K_IS_FRONT) == 0)
	{
		//DBG_PRINT("Not FRONT\r\n");
		return;
	}

	pPsaclerStream = &pscaler_stream[src];

	osENTER_CRITICAL();
	if(scalerInputDone[src] && scalerOutputDone[src])
	{ // Pscaler_B finish doing scale-up in ISR
		scale_up_done = 1;
	}
	osEXIT_CRITICAL();

	if(scale_up_done)
	{
		if(buf_idx >= pPsaclerStream->ps_fb_cfg->fb_cfg_max_count)
		{// if two index object are equal, then release buffer
			lib_interp2K_frame_check(src);
		}
	}
	else
	{
		if(pVid_info->pscaler_up_drop == 0)
		{
			#if C_SUPORT_GET_JPG_BUF
			if(pGet_jpeg_args->flag_start_buf_get && pGet_jpeg_args->flag_start_fifo_buf_get)
			{
				pGet_jpeg_args->flag_get_fail = 1;
			}
			#endif

			pVid_info->pscaler_up_drop = 1; // Restart pscaler_B to finish a frame

			drv_l1_pscaler_output_fifo_restart(PSCALER_B);
		}
	}

}

static void
interp2K_scale_up_restart(
	unsigned char src,
	INT32U buf_idx
)
{
	INT32U scaler = PSCALER_B;

	osENTER_CRITICAL();

	if(scalerOutputStatus[src] == (PIPELINE_SCALER_STATUS_BUF_A_DONE | PIPELINE_SCALER_STATUS_BUF_B_DONE))
	{
		drv_l1_pscaler_output_fifo_restart(scaler);
	}

	if(buf_idx & 0x01)
	{
		scalerOutputStatus[src] &= ~PIPELINE_SCALER_STATUS_BUF_A_DONE;
	}
	else
	{
		scalerOutputStatus[src] &= ~PIPELINE_SCALER_STATUS_BUF_B_DONE;
	}

	osEXIT_CRITICAL();
}

static void
interp2K_scale_up_isr(
	PS_ID scaler, // It's PSCALER_B
	PS_ST PScaler_Event
)
{
	INT8U src = PREVIEW_SRC_FRONT;
	INT32U fifo_buf_done;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl;
	ENC_STREAM_INFO_T* pEncStramInfo;
	VID_INFO* pVid_info;
	#if (C_SUPORT_GET_JPG_BUF)
	JpegEnc_t*pGetJpeg;
	GET_JPEG_ARGS* pGet_jpeg_args;
	#endif

	const INT32U mask_event = PIPELINE_SCALER_STATUS_OVERFLOW_OCCUR |
		PIPELINE_SCALER_STATUS_FRAME_DONE |
		PIPELINE_SCALER_STATUS_BUF_A_DONE |
		PIPELINE_SCALER_STATUS_BUF_B_DONE |
		PIPELINE_SCALER_STATUS_AHB_DONE;

	if(!(PScaler_Event & mask_event))
	{
		// debug
		DBG_PRINT("[DEBUG]PS[%d]:other=0x%X\r\n", scaler, PScaler_Event);
	}

	if(PScaler_Event & PIPELINE_SCALER_STATUS_OVERFLOW_OCCUR)
	{
		DBG_PRINT("[TRACE]PS[%d]:overflow\r\n", scaler);
	}

   	/*=== Record ===*/
	pPscalerStreamCtrl = &pscaler_stream[src];
	if(!pPscalerStreamCtrl)
	{
		return;
	}

	pEncStramInfo = &pPscalerStreamCtrl->enc_stream_info;
	if(pEncStramInfo->stream_enable == VIDEO_RECORD_NOTHING)
	{
		return;
	}

	pVid_info = pPscalerStreamCtrl->pVid_info;

	if(pEncStramInfo->stream_enable & (VIDEO_RECORD_RECING | VIDEO_RECORD_STOP))
	{
		if((PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE) || (PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE))
		{
			if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE)
			{
				++pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num;

				scalerOutputStatus[src] |= PIPELINE_SCALER_STATUS_BUF_A_DONE;
				fifo_buf_done = drv_l1_pscaler_output_A_buffer_get(scaler);

				if(pPscalerStreamCtrl->ps_notify_func)
				{
					pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,fifo_buf_done);
				}

				//DBG_PRINT("=>A:%d\r\n",pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num);
			}

			if(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE)
			{
				++pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num;

				scalerOutputStatus[src] |= PIPELINE_SCALER_STATUS_BUF_B_DONE;

				fifo_buf_done = drv_l1_pscaler_output_B_buffer_get(scaler);

				if(pPscalerStreamCtrl->ps_notify_func)
				{
					pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,fifo_buf_done);
				}

				//DBG_PRINT("=>B:%d\r\n",pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num);
			}

			if(pVid_info->pscaler_up_drop ||
				(scalerOutputStatus[src] != (PIPELINE_SCALER_STATUS_BUF_A_DONE | PIPELINE_SCALER_STATUS_BUF_B_DONE))
			  )
			{
				drv_l1_pscaler_output_fifo_restart(scaler);
			}
		}

		if(PScaler_Event & PIPELINE_SCALER_STATUS_AHB_DONE)
		{
			scalerInputDone[src] = 1;
		}

		if(PScaler_Event & PIPELINE_SCALER_STATUS_FRAME_DONE)
		{
			scalerOutputDone[src] = 1;
		}

		if(scalerInputDone[src] && scalerOutputDone[src])
		{
			//DBG_PRINT("=>PscalerB Done\r\n");
			drv_l1_pscaler_stop(scaler);

			if(pVid_info->pscaler_up_drop)
			{
				pVid_info->pscaler_up_drop = 0;

				#if (C_SUPORT_GET_JPG_BUF)
				pGet_jpeg_args = &pVid_info->get_jpeg_args;
				if(pGet_jpeg_args->flag_start_buf_get &&
				   pGet_jpeg_args->flag_start_fifo_buf_get &&
				   pGet_jpeg_args->flag_get_fail
				)
				{
					pGetJpeg = &pGet_jpeg_args->jpeg_cfg;
					pGet_jpeg_args->flag_get_fail = 0;
					pGet_jpeg_args->flag_start_buf_get = 0;
					pGet_jpeg_args->flag_start_fifo_buf_get = 0;
					pGetJpeg->encode_size = 0;

					msgQSend(pPscalerStreamCtrl->ps_notify_q, (MSG_VID_GET_JPEG_DONE|src), NULL, 0, MSG_PRI_NORMAL);
				}
				#endif
			}

			if((pEncStramInfo->stream_enable == VIDEO_RECORD_STOP)
			    && (drv_l1_pscaler_bypass_output_A_buffer_get(PSCALER_A) == DUMMY_BUFFER_ADDRS)
			    && (drv_l1_pscaler_bypass_output_B_buffer_get(PSCALER_A) == DUMMY_BUFFER_ADDRS)
			)
			{
				//DBG_PRINT("=>PscalerA Done\r\n");

				pEncStramInfo->stream_enable = VIDEO_RECORD_NOTHING;
				pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num = DUMMY_BUFFER_ADDRS;
				if(pPscalerStreamCtrl->ps_notify_func)
				{
					pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,DUMMY_BUFFER_ADDRS);
				}
			}

			pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num = 0;
		}
	}
}

INT32S
lib_interp2K_scale_up(
	INT32U src, // It's PREVIEW_SRC_FRONT
	INT32U src_buf
)
{
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	INT32U	inW, inH;
	INT32U	outW, outH;
	INT32U	wFactor, hFactor;
	INT32U pscaler = PSCALER_B;
	INT32U fifo_buf;
	FB_CFG* pPscalerOutCfg;

	#if C_VID_REC_USE_SYS_FIFO
	src_buf = lib_encode_system_fifo_unmapping(src,src_buf);
	#endif

	// Set to 0 befor encoding
	pVid_info->pscaler_up_drop = 0;
	pPscalerStreamCtrl->ps_fb_cfg->fb_cfg_count_num = 0;
	pPscalerStreamCtrl->msg_fb_cfg->fb_cfg_count_num = 0;
	scalerOutputDone[src] = 0;
	scalerInputDone[src] = 0;
	scalerOutputStatus[src] = 0;

	// Set param
	inW = pVid_info->sensor_width;
	inH = pVid_info->sensor_height;
	outW = pVid_info->target_width;
	outH = pVid_info->target_height;
    //DBG_PRINT("[INTERP2K]inW=%d,inH=%d,outW=%d,outH=%d\r\n",inW,inH,outW,outH);
	drv_l1_pscaler_init(pscaler);
	drv_l1_pscaler_input_source_set(pscaler, PIPELINE_SCALER_INPUT_SOURCE_DRAM);
	drv_l1_pscaler_input_buffer_set(pscaler, src_buf);

	switch(pVid_info->data_format)
	{
		case VID_DATA_422:
			drv_l1_pscaler_input_format_set(pscaler, PIPELINE_SCALER_INPUT_FORMAT_YUYV);
		break;
		case VID_DATA_420:
			drv_l1_pscaler_input_format_set(pscaler, PIPELINE_SCALER_INPUT_FORMAT_GP420);
		break;
	}

	drv_l1_pscaler_output_format_set(pscaler, PIPELINE_SCALER_OUTPUT_FORMAT_GP420);

	wFactor = (((inW - 1) << 16) / (outW - 1));
	hFactor = (((inH - 1) << 16) / (outH - 1));

	drv_l1_pscaler_pixels_set(pscaler, inW, inH, outW, outH, wFactor, hFactor);

	drv_l1_pscaler_output_fifo_line_set(pscaler, VID_REC_LINE_FRONT, 1);

	pPscalerOutCfg = pPscalerStreamCtrl->ps_fb_cfg;

	fifo_buf = (INT32U)pPscalerOutCfg->fb_cfg_head_addr;
	//DBG_PRINT("=>fifoA:%x\r\n",fifo_buf);
	drv_l1_pscaler_output_A_buffer_set(pscaler, fifo_buf);

	fifo_buf += pPscalerOutCfg->fb_cfg_size;
	//DBG_PRINT("=>fifoB:%x\r\n",fifo_buf);
	drv_l1_pscaler_output_B_buffer_set(pscaler, fifo_buf);

	drv_l1_pscaler_callback_register2(pscaler, interp2K_scale_up_isr);

	drv_l1_pscaler_interrupt_enable(pscaler, 1);

	pVid_info->pscaler_up_start = 1;
	drv_l1_pscaler_start(pscaler);

	return STATUS_OK;
}

void
lib_interp2K_scaleup_state_set(
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl
)
{
	ENC_STREAM_INFO_T *enc_stream_info = NULL;
	enc_stream_info =  &pPscalerStreamCtrl->enc_stream_info;

	ENCODE_STATUS_SET_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_INTERP2K_SCALEUP_OFFSET);
}

void
lib_interp2K_scaleup_state_clear(
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl
)
{
	ENC_STREAM_INFO_T *enc_stream_info = NULL;
	enc_stream_info =  &pPscalerStreamCtrl->enc_stream_info;

	osENTER_CRITICAL();
	ENCODE_STATUS_CLR_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_INTERP2K_SCALEUP_OFFSET);
	osEXIT_CRITICAL();
}

INT32S
lib_interp2K_scaleup_state_get(
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl
)
{
	ENC_STREAM_INFO_T *enc_stream_info = NULL;
	enc_stream_info =  &pPscalerStreamCtrl->enc_stream_info;

	return (ENCODE_STATUS_IS_BUSY(enc_stream_info->stream_status,VIDEO_ENCODE_STATUS_INTERP2K_SCALEUP_OFFSET));
}

INT32S
lib_interp2K_scaleup_process(
	INT32U src // It's PREVIEW_SRC_FRONT
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	ENC_STREAM_INFO_T* pEncStramInfo = &pPscalerStreamCtrl->enc_stream_info;
	FB_INFO* ps_fb_info = NULL;
	INT32U data;

	//DBG_PRINT("==Run:%s==[%d]==\r\n",__FUNCTION__,src);
	if(pEncStramInfo->stream_enable == VIDEO_RECORD_NOTHING)
	{
		//DBG_PRINT("=>scale(%d)nothing\r\n",src);
		return STATUS_OK;
	}

	if(lib_interp2K_scaleup_state_get(pPscalerStreamCtrl))
	{
		//DBG_PRINT("=>scale(%d)Busy\r\n", src);
		return STATUS_OK;
	}

	if(circular_buf_get(cbuf_notify_interp2K[src],((INT32U*)&data)) < 0)
	{
		//ERR_PRINT("fail to get FB_INFO[%d]\r\n",src);
		return (STATUS_FAIL);
	}

	ps_fb_info = (FB_INFO*)data;

	//DBG_PRINT("scale in:%x\r\n",ps_fb_info->buf);

	// Set busy
	lib_interp2K_scaleup_state_set(pPscalerStreamCtrl);

	nRet = lib_interp2K_scale_up(src,ps_fb_info->buf);
	if(nRet == STATUS_FAIL)
	{
		// Clear busy
		lib_interp2K_scaleup_state_clear(pPscalerStreamCtrl);

		RETURN(STATUS_FAIL);
	}

	return STATUS_OK;

Return:

	if(ps_fb_info->buf != DUMMY_BUFFER_ADDRS)
	{
		pPscalerStreamCtrl->interp2K_free_func(ps_fb_info->buf);
		ps_fb_info->buf = DUMMY_BUFFER_ADDRS;
	}

    return nRet;
}

INT32S
lib_interp2K_frame_check(
	INT32U src // It's PREVIEW_SRC_FRONT
)
{
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	VID_INFO* pVid_info = pPscalerStreamCtrl->pVid_info;
	INT32U ret_addr;
	INT32U content_size = 0;

	if((pVid_info->run_2K_flow_src & HOST_2K_IS_FRONT) == 0)
	{
		//DBG_PRINT("Not FRONT\r\n");
		return STATUS_OK;
	}

	if(pVid_info->pscaler_up_start)
	{
		pVid_info->pscaler_up_start = 0;

		lib_interp2K_scaleup_state_clear(pPscalerStreamCtrl);

		// Release frame buffer to pscaler's bypass output buffer that is for preview_pscaler_isr
		ret_addr = drv_l1_pscaler_input_buffer_get(PSCALER_B);
		// Set dummy buffer after returning
		drv_l1_pscaler_input_buffer_set(PSCALER_B,DUMMY_BUFFER_ADDRS);
		//DBG_PRINT("scale dn:%x\r\n",ret_addr);
		if(ret_addr != DUMMY_BUFFER_ADDRS)
		{
			pPscalerStreamCtrl->interp2K_free_func(ret_addr);
	    }

		osENTER_CRITICAL();
		content_size = circular_buf_size(cbuf_notify_interp2K[src]);
		osEXIT_CRITICAL();

		if(content_size > 0)
		{// Do the next frame
			lib_interp2K_scaleup_process(src);
		}

	}
	return STATUS_OK;
}
static unsigned int
interp2K_get_bypass(
	void
)
{
	INT32U src = PREVIEW_SRC_FRONT;
	int nRet;
	unsigned int data = DUMMY_BUFFER_ADDRS;

	nRet = circular_buf_get(cbuf_interp2K_bypass_out[src], &data);
	if(nRet < 0)
	{
		#if STATISTICS_DUMP
		//DBG_PRINT(" X ");
		dummy_2k_cnt++;
		#endif
		return DUMMY_BUFFER_ADDRS;
	}

	//DBG_PRINT("get buf:0x%x\r\n",data);

	return data;
}

static void
interp2K_free_bypass(
	unsigned int buf
)
{
	INT32U src = PREVIEW_SRC_FRONT;
	int nRet;

	//DBG_PRINT("[%d]free buf:0x%x\r\n",src,buf);
	nRet = circular_buf_try_put(cbuf_interp2K_bypass_out[src],buf);
	if(nRet < 0)
	{
		ERR_PRINT("fail to run circular_buf_try_put[%d]\r\n",src);
	}
}

static void
interp2K_notify_bypass(
	unsigned char src,// It's PREVIEW_SRC_FRONT
	void* p,
	INT32U data
)
{
	PSCALER_STREAM_CTRL_T *stream_ctrl = (PSCALER_STREAM_CTRL_T*) p;
	FB_CFG* pInterp2KPscalerOutCfg = stream_ctrl->interp2K_ps_fb_cfg;
	FB_CFG* pInterp2KMsgNotifyCfg = stream_ctrl->interp2K_msg_fb_cfg;
	VID_INFO* pVid_info = stream_ctrl->pVid_info;
	FB_INFO* pfb_info = ((FB_INFO*)(pInterp2KMsgNotifyCfg->fb_cfg_head_addr))+pInterp2KMsgNotifyCfg->fb_cfg_count_num;

	if(data == DUMMY_BUFFER_ADDRS)
	{// Don't pass dummy buffer to do scale-up
		return;
	}

	pfb_info->buf = data;
	pfb_info->idx = pInterp2KPscalerOutCfg->fb_cfg_count_num;
	pfb_info->ps_buf_time = xTaskGetTickCountFromISR();

	if(circular_buf_try_put(cbuf_notify_interp2K[src],((INT32U)pfb_info)) < 0)
	{
		ERR_PRINT("fail to put cbuf_notify_interp2K[%d]\r\n",src);
		goto    Return;
	}

	pInterp2KMsgNotifyCfg->fb_cfg_count_num++;
	if(pInterp2KMsgNotifyCfg->fb_cfg_count_num >= pInterp2KMsgNotifyCfg->fb_cfg_blocks)
	{
		pInterp2KMsgNotifyCfg->fb_cfg_count_num = 0;
	}

	if(pVid_info->run_ap_mode == STATE_VIDEO_RECORD)
	{
		if(msgQSend(stream_ctrl->ps_notify_q, (MSG_INTERP2K_BYPASS_FRAME_DONE_NOTIFY|src), NULL, 0, MSG_PRI_NORMAL) < 0)
		{
			ERR_PRINT("fail to MSG_INTERP2K_BYPASS_FRAME_DONE_NOTIFY [%d]\r\n",src);
			goto Return;
		}
	}

	return;

Return:

	if(data != DUMMY_BUFFER_ADDRS)
	{
		stream_ctrl->interp2K_free_func(data);
    }

	return;
}


INT32S
interp2k_preview_image_put(
	INT32U src,// It's PREVIEW_SRC_REAR
	INT32U buf
)
{
	INT32U scaler = PSCALER_B; // fake parameter
	INT32U unmask_addr;

	//DBG_PRINT("==Run:%s==mode:%d==\r\n",__FUNCTION__,_preview_mode);

	if(pscaler_mode[scaler] != PSCALER_MODE_DRAM)
	{
		return STATUS_FAIL;
	}

	if(pscaler_busy[scaler])
	{
		return STATUS_FAIL;
	}

	#if C_VID_REC_USE_SYS_FIFO
	unmask_addr = lib_encode_system_fifo_unmapping(scaler,buf);
	#else
	unmask_addr = buf;
	#endif

	drv_l1_scaler_input_A_addr_set(SCALER_0,unmask_addr,DUMMY_BUFFER_ADDRS,DUMMY_BUFFER_ADDRS);
	pscaler_busy[scaler] = 1;

	drv_l1_scaler_stop(SCALER_0);
	drv_l1_scaler_start(SCALER_0, 0);

	return STATUS_OK;
}

INT32S
lib_interp2K_csi_frame_ready_notify(
	INT32U buf
)
{
	INT32S nRet = STATUS_OK;
	INT8U src = PREVIEW_SRC_REAR;

	//DBG_PRINT("put_idle buf:0x%x\r\n",buf);

	if(circular_buf_try_put(cbuf_interp2K_handle[src][INTERP_2K_CSI_READY], buf) < 0)
	{
		ERR_PRINT("fail to put cbuf_interp2K_handle[%d]\r\n",INTERP_2K_CSI_READY);
		RETURN(STATUS_FAIL);
	}

	if(host_2K_frame_ready_notify(src) == STATUS_FAIL)
	{
		ERR_PRINT("fail to host_2K_frame_ready_notify \r\n");
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}

void
lib_interp2K_csi_callback_isr(
	INT32U CSI_Num,
	INT32U event
)
{
	INT32U csi_buf_done = DUMMY_BUFFER_ADDRS;
	INT32U csi_buf_next;

	//DBG_PRINT("=>(%d,%x)\r\n",CSI_Num,event);

	if(event & CSI_SENSOR_FRAME_END_EVENT)
	{
		csi_buf_next = rear_pscaler_get_bypass();
		csi_buf_done = drv_l1_csi_get_bufA(CSI_Num);
		//DBG_PRINT("csi dn:%x\r\n",csi_buf_done);
		if(csi_buf_done != DUMMY_BUFFER_ADDRS)
		{
			if(event & CSI_SENSOR_FIFO_OVERFLOW_EVENT)
			{
				//DBG_PRINT("CSI overflow\r\n");
				rear_pscaler_free_bypass(csi_buf_done);
			}
			else
			{
				if(lib_interp2K_csi_frame_ready_notify(csi_buf_done) == STATUS_FAIL)
				{// Release pscaler bypass output buffer
					rear_pscaler_free_bypass(csi_buf_done);
				}
			}
		}

		drv_l1_csi_set_bufA(CSI_Num, csi_buf_next);
	}
}

INT32U
lib_interp2K_csi_frame_ready_get(
	INT32U src
)
{
	int nRet;
	unsigned int data = DUMMY_BUFFER_ADDRS;

	nRet = circular_buf_get(cbuf_interp2K_handle[src][INTERP_2K_CSI_READY], &data);
	if(nRet < 0)
	{
        //DBG_PRINT(" x");
		return DUMMY_BUFFER_ADDRS;
	}

	//DBG_PRINT("get buf:0x%x\r\n",data);

	return data;
}

INT32U
lib_interp2K_csi_frame_content_size_get(
	INT32U src
)
{
	return circular_buf_size(cbuf_interp2K_handle[src][INTERP_2K_CSI_READY]);
}

INT32S
lib_interp2K_frame_ready_process(
	INT32U src,
	INT32U do_scale_up
)
{
	INT32S nRet = STATUS_OK;
	INT32U csi_frame_buf;
	INT32U csi_frame_content_size;

	//DBG_PRINT("==Run:%s==0x%x\r\n",__FUNCTION__,pVid_info);

	csi_frame_buf = lib_interp2K_csi_frame_ready_get(src);
	if(csi_frame_buf == DUMMY_BUFFER_ADDRS)
	{
		return STATUS_OK;
	}

	if(	!back_sensor_plug_status_get() ||
		(sensor_preview_status_get() == PREVIEW_MODE_STOP_WAIT)||
		(sensor_preview_status_get() == PREVIEW_MODE_STOP)
	)
	{
		rear_pscaler_free_bypass(csi_frame_buf);

		return STATUS_OK;
	}

	// Check the content size of CIS frame buffer
	csi_frame_content_size = lib_interp2K_csi_frame_content_size_get(src);
	if(csi_frame_content_size)
	{
		do
		{
			if(lib_interp2K_scaler_preview_done_notify(src,csi_frame_buf) == STATUS_FAIL)
			{
				// Release pscaler bypass output buffer
				rear_pscaler_free_bypass(csi_frame_buf);
			}

			// Get the next CIS frame buffer and check the content size of CIS frame buffer
			csi_frame_buf = lib_interp2K_csi_frame_ready_get(src);
			csi_frame_content_size = lib_interp2K_csi_frame_content_size_get(src);
		}while(csi_frame_content_size);
	}

	nRet = interp2k_preview_image_put(src,csi_frame_buf);
	if(nRet == STATUS_FAIL)// pscaler or scaler is busy, then pass output buffer to encode
	{
		// Release pscaler bypass buffer
		rear_pscaler_free_bypass(csi_frame_buf);
	}

	return nRet;
}

INT32S
lib_interp2K_preview_done_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;
	PSCALER_STREAM_CTRL_T* pPscalerStreamCtrl = &pscaler_stream[src];
	INT32U frame;

	//DBG_PRINT("==Run:%s==0x%x\r\n",__FUNCTION__,pVid_info);

	frame = lib_interp2K_scaler_preview_frame_get(src);
	if(frame == DUMMY_BUFFER_ADDRS)
	{
		return STATUS_OK;
	}

	// Start video recording
	if(pPscalerStreamCtrl->enc_stream_info.stream_enable == VIDEO_RECORD_START)
	{
		//DBG_PRINT("[HOST USB] VIDEO_RECORD_START:0x%x\r\n",pMjpeg_dec_args->jpeg_cfg.output_buffer);
		if(pPscalerStreamCtrl->ps_notify_func)
		{
			pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,frame);
			return STATUS_OK;
		}
	}
	else if(pPscalerStreamCtrl->enc_stream_info.stream_enable == VIDEO_RECORD_STOP)
	{
		//DBG_PRINT("[HOST USB] VIDEO_RECORD_STOP \r\n");
		pPscalerStreamCtrl->enc_stream_info.stream_enable = VIDEO_RECORD_NOTHING;

		if(pPscalerStreamCtrl->ps_notify_func)
		{
			pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,DUMMY_BUFFER_ADDRS);
		}
	}
	else if(pPscalerStreamCtrl->enc_stream_info.stream_enable == VIDEO_RECORD_PHOTO_START)
	{
		//DBG_PRINT("VIDEO_RECORD_PHOTO_START:0x%x\r\n",frame);
		pPscalerStreamCtrl->enc_stream_info.stream_enable = VIDEO_RECORD_NOTHING;

		if(pPscalerStreamCtrl->ps_notify_func)
		{
			pPscalerStreamCtrl->ps_notify_func(src,pPscalerStreamCtrl,frame);
		}
		if(msgQSend(pPscalerStreamCtrl->ps_notify_q, (MSG_VID_PSCALER_NOTIFY|src), NULL, 0, MSG_PRI_NORMAL) < 0)
		{
			ERR_PRINT("fail to msgQSend [%d]\r\n",src);
		}
		return STATUS_OK;
	}

	// Release pscaler bypass buffer
	rear_pscaler_free_bypass(frame);

	return nRet;
}
#endif

/*== E X I F ==*/
#if (defined C_PHOTO_JPEG_ADD_EXIF) && (C_PHOTO_JPEG_ADD_EXIF)
INT32S
lib_photo_exif_add(
	INT32U src,
	INT32U src_buf,
	VID_INFO* pVid_info
)
{
	#define THUMBNAIL_ENC_BUF_SIZE (50*1024)
	#define EXIF_HEADER_LIMIT_SIZE (64*1024)

	INT32S nRet = STATUS_OK;
	INT32U	inW, inH;
	INT32U	outW, outH;
	INT32U	wFactor, hFactor;
	INT32U scaler = preview_src_to_ps(src);
	INT32U pscaler_scale_up_size;
	INT32U pscaler_out_buf_size;
	INT32U pscaler_out_buf = 0;
	INT32U pscaler_status;
	JPEG_REC_ARGS*	pJpeg_rec_args = &pVid_info->jpeg_rec_args;
	JpegEnc_t*pJpegEnc =  &pJpeg_rec_args->jpeg_cfg;
	JpegEnc_t JpegEnc = {0};
	INT32U jpeg_enc_out_buf = 0;
	INT8U*  ret_exif_data = NULL;
	INT32U 	ret_exif_data_len;
	INT32S quality_value = 50;

#if ENABLE_WITH_THUMBNAIL_IMAGE

	// Set param
	inW = pVid_info->sensor_width;
	inH = pVid_info->sensor_height;
	outW = EXIT_THUMBNAIL_WIDTH;
	outH = EXIT_THUMBNAIL_HEIGHT;

    DBG_PRINT("thumbnail:inW=%d,inH=%d,outW=%d,outH=%d\r\n",inW,inH,outW,outH);
	drv_l1_pscaler_init(scaler);
	drv_l1_pscaler_input_source_set(scaler, PIPELINE_SCALER_INPUT_SOURCE_DRAM);
	drv_l1_pscaler_input_buffer_set(scaler, src_buf);

	pscaler_scale_up_size = outW*outH;

	switch(pVid_info->data_format)
	{
		case VID_DATA_422:
			drv_l1_pscaler_input_format_set(scaler, PIPELINE_SCALER_INPUT_FORMAT_YUYV);
			pscaler_scale_up_size+= (outW*outH);
		break;
		case VID_DATA_420:
			drv_l1_pscaler_input_format_set(scaler, PIPELINE_SCALER_INPUT_FORMAT_GP420);
			pscaler_scale_up_size+= (outW*outH)>>1;
		break;
	}
	pscaler_out_buf = (INT32U)gp_malloc_align(pscaler_scale_up_size, 64);
	if(pscaler_out_buf == 0)
	{
		ERR_PRINT("[ERROR]pscaler_out_buf[%d] is null \r\n",src);
		RETURN(STATUS_FAIL);
	}

	drv_l1_pscaler_output_format_set(scaler, PIPELINE_SCALER_OUTPUT_FORMAT_GP420);

    wFactor = (((inW - 1) << 16) / (outW - 1));
    hFactor = (((inH - 1) << 16) / (outH - 1));

	drv_l1_pscaler_pixels_set(scaler, inW, inH, outW, outH, wFactor, hFactor);
	drv_l1_pscaler_output_fifo_line_set(scaler, outH, 0);
	drv_l1_pscaler_output_A_buffer_set(scaler, pscaler_out_buf);
	drv_l1_pscaler_output_B_buffer_set(scaler, DUMMY_BUFFER_ADDRS);
	drv_l1_pscaler_callback_register2(scaler, NULL);
	drv_l1_pscaler_interrupt_enable(scaler, 1);
	drv_l1_pscaler_bypass_mode_set(scaler, 0);
	drv_l1_pscaler_start(scaler);

	do
	{
		pscaler_status = drv_l1_pscaler_status_get(scaler);
		if(pscaler_status & PIPELINE_SCALER_STATUS_FRAME_DONE)
		{
			break;
		}

	}while(1);

	drv_l1_pscaler_stop(scaler);

	jpeg_enc_out_buf = (INT32U)gp_malloc_align(THUMBNAIL_ENC_BUF_SIZE, 64);
	if(!jpeg_enc_out_buf)
	{
		RETURN(STATUS_FAIL);
	}
	
#endif // ENABLE_WITH_THUMBNAIL_IMAGE

	do
	{
#if ENABLE_WITH_THUMBNAIL_IMAGE
		JpegEnc.hwidx = pJpegEnc->hwidx;
		JpegEnc.quality_value = quality_value;
		JpegEnc.yuv_fmt = VID_DATA_420;
		JpegEnc.yuv_sample_fmt = JPEG_YUV420;
		JpegEnc.width = EXIT_THUMBNAIL_WIDTH;
		JpegEnc.height = EXIT_THUMBNAIL_HEIGHT;
		JpegEnc.input_buffer_y = pscaler_out_buf;
		JpegEnc.output_buffer = jpeg_enc_out_buf;
		JpegEnc.output_buffer_len = THUMBNAIL_ENC_BUF_SIZE;

		nRet = jpeg_buffer_encode(&JpegEnc);
		if(nRet == STATUS_FAIL)
		{
			RETURN(STATUS_FAIL);
		}

		JpegEnc.encode_size = ((JpegEnc.encode_size+3) & ~0x03);

		// Init EXIF
		nRet = exif_info_init(scaler, 1);
#else
		JpegEnc.encode_size = 0;
		// Init EXIF
		nRet = exif_info_init(scaler, 0);
#endif //ENABLE_WITH_THUMBNAIL_IMAGE
		
		if(nRet == STATUS_FAIL)
		{
			RETURN(STATUS_FAIL);
		}

		// Demo: dynamically update values 
		#if 0
		{
			ExifArgs argMake = {"SampleDemo"};
			exif_tag_set(src,EXIF_IFD_0,EXIF_TAG_MAKE,&argMake);

			ExifArgs argExp = {"1","500"};
			exif_tag_set(src,EXIF_IFD_EXIF,EXIF_TAG_EXPOSURE_TIME,&argExp);

			ExifArgs argGPS = {"M"};
			exif_tag_set(src,EXIF_IFD_GPS,EXIF_TAG_GPS_SPEED_REF,&argGPS);			
		}
		#endif

		nRet = exif_data_content_save(src,JpegEnc.encode_size,&ret_exif_data,&ret_exif_data_len);
		if(nRet == STATUS_FAIL)
		{
			RETURN(STATUS_FAIL);
		}

		if(ret_exif_data_len > EXIF_HEADER_LIMIT_SIZE)
		{
			// Free EXIF
			exif_info_uninit(src);
			if(ret_exif_data)
			{
				gp_free(ret_exif_data);
			}

			quality_value -= 10;
			if(quality_value <= 0)
			{
				ERR_PRINT("EXIF size over EXIF_HEADER_LIMIT_SIZE \r\n");
				RETURN(STATUS_FAIL);
			}
		}
		else
		{
			break;
		}

	}while(1);

	// Write EXIF header
	//DBG_PRINT("Write EXIF header len:%d\r\n",ret_exif_data_len);
	ap_storage_service_write_file(pVid_info->rec_file_handle,ret_exif_data, ret_exif_data_len);
#if ENABLE_WITH_THUMBNAIL_IMAGE
	//DBG_PRINT("Write thumbnail image len:%d\r\n",JpegEnc.encode_size);
	ap_storage_service_write_file(pVid_info->rec_file_handle,jpeg_enc_out_buf, JpegEnc.encode_size);
#else
    // main image skip 0xFF 0xDB, EXIF header included
    nRet = STATUS_OK;
#endif

Return:

	// Free EXIF
	exif_info_uninit(src);
	if(ret_exif_data)
	{
		gp_free(ret_exif_data);
	}

	if(pscaler_out_buf)
	{
		gp_free(pscaler_out_buf);
	}

	if(jpeg_enc_out_buf)
	{
		gp_free(jpeg_enc_out_buf);
	}

	if(nRet == STATUS_OK)
	{
		INT32U jpeg_header_total_size = jpeg_header_get_size()-2; //638 Bytes
		INT32U jpeg_header_size_4Alignment;
		INT32S text_comment_remove_byte;
		INT8U* text_comment_table = (pJpegEnc->header+2); // total 29 byte
		INT8U* jpeg_hader_4Alignment = NULL;
		INT8U* jpeg_hader_quality_table = (pJpegEnc->header+31);// total 609 byte
		INT32S copy_text_comment_size;

		jpeg_header_size_4Alignment = (jpeg_header_total_size & ~0x03);
		text_comment_remove_byte = (jpeg_header_total_size-jpeg_header_size_4Alignment);
		if(text_comment_remove_byte <= 0)
		{
			ERR_PRINT("text_comment_remove_byte:%d\r\n",text_comment_remove_byte);
			RETURN(STATUS_FAIL);
		}

		jpeg_hader_4Alignment = (INT32U)gp_malloc_align(jpeg_header_size_4Alignment, 64);
		if(!jpeg_hader_4Alignment)
		{
			RETURN(STATUS_FAIL);
		}

		copy_text_comment_size = (29-text_comment_remove_byte);
		if(copy_text_comment_size <= 0)
		{
			ERR_PRINT("copy_text_comment_size:%d\r\n",copy_text_comment_size);
			gp_free(jpeg_hader_4Alignment);
			RETURN(STATUS_FAIL);
		}
		
		// Change text comment size
		text_comment_table[3] -= text_comment_remove_byte;

		memcpy(jpeg_hader_4Alignment,text_comment_table,copy_text_comment_size);
		memcpy(jpeg_hader_4Alignment+copy_text_comment_size,jpeg_hader_quality_table,609);

		ap_storage_service_write_file(pVid_info->rec_file_handle, (INT32U)jpeg_hader_4Alignment, jpeg_header_size_4Alignment);
		gp_free(jpeg_hader_4Alignment);
	}
	else
	{
		// Fail to create EXIF , so write main image header
		ap_storage_service_write_file(pVid_info->rec_file_handle, (INT32U)pJpegEnc->header, jpeg_header_get_size());
	}

	return nRet;
}

#endif


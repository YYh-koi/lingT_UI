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
#ifndef __TASK_VIDEO_DECODER_H__
#define __TASK_VIDEO_DECODER_H__

#include "MultiMediaParser.h"
#include "task_video_decoder_cfg.h"
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
	SCALE_FULL_SCREEN = 0x10,
	SCALE_FIT_BUF,
	NO_SCALE,
	NO_SCALE_AT_CENTER,
	SCALE_MAX
} SCALER_TYTE;

typedef struct
{
    INT32U addr;
    INT8U used;
} pscale_decode_para_struct;

typedef struct
{
	INT32U  status;
    INT32S  file_handle;	//file handle
    INT32S  FileType;
    INT64U  FileSize;
    void  	*media_handle;

   	// resume play
   	INT32U bResumePlayFlag;

    // video & audio sync
    INT64S tick_time;
    INT64S tick_time2;
    INT64S time_range;
    INT32S n;
    INT64S ta, tv, Tv;
   	INT32U fail_cnt;
   	INT32U pend_cnt, post_cnt;

    // video
    INT16U video_flag;
    INT16U user_define_flag;
    INT32U video_format;
    INT32U VidTickRate;
    INT32U video_total_sample;
    INT32U video_total_time;
    INT32U video_frame_rate;
    INT32U video_cur_time;
    INT32U video_cur_frame;
	INT32U video_seek_time;
	INT32U video_seek_frame;
    INT32U video_decode_addr;
    INT32U video_decode_cb;
    INT32U video_decode_cb_handle;
    INT8U video_total_track_number;
    INT8U video_select_track;
    INT8U video_current_track;

    // deblock
    INT32U deblock_flag;
    INT32U deblock_addr[DEBLOCK_FRAME_NO];

    // speed control
    INT32U play_speed;
    INT8U  reverse_play_set;
    INT8U  reverse_play_flag;
    INT8U  ff_audio_flag;

    // audio
    INT8U  audio_flag;
    INT32U aud_frame_size;
    INT32U audio_decode_addr[AUDIO_FRAME_NO];

	// audio decoder
	INT8U  *work_mem_addr;
	INT32U work_mem_size;
	INT8U  *ring_buffer_addr;
	INT32U ring_buffer_size;
	INT32U WI;
	INT32U RI;

	// audio special effect
#if VOICE_CHANGER_EN == 1
	void   *vc_handle;
#endif
#if UP_SAMPLE_EN == 1
	void   *us_handle;
#endif

	// audio up sample
	INT8U  upsample_flag;
	INT8U  aud_dec_ch_no;
	INT16U aud_dec_sr;

	// scaler
	INT16U scaler_flag;
	INT16U user_scaler_flag;
	INT32U scaler_output_addr;
	INT32U scaler_output_cb;
	INT32U scaler_output_cb_handle;
	INT16U xoffset, yoffset;
	INT16U image_output_width, image_output_height;
	INT16U buffer_output_width, buffer_output_height;
	INT32U mpeg4_decode_out_format;
    INT32U video_decode_in_format;
    INT32U video_decode_out_format;

    INT16U patch_offset;
	
    INT8U	H264_MB2SCAN_patch;
    INT16U	H264_MB2SCAN_patch_offset;
} video_decode_para_struct;

typedef struct
{
	INT16U	current_index;
	INT16U	total_number;
} vid_dec_buf_struct;

typedef struct
{
	INT32U	scaler_mode;
	INT32U	input_addr;
	INT32U	input_format;
	INT32U	input_x;
	INT32U	input_y;
	INT32U	input_visible_x;
	INT32U	input_visible_y;

	INT32U	output_addr;
	INT32U	output_format;
	INT32U	output_x;
	INT32U	output_y;
	INT32U	output_buffer_x;
	INT32U	output_buffer_y;
	INT32U	boundary_color;
} scaler_info;

typedef struct
{
	INT32U	scaler_mode;
	INT32U	raw_data_addr;
	INT32U	raw_data_size;

	INT32U	fifo_line;
	INT32U	jpeg_yuv_mode;
	INT32U	jpeg_valid_w;
	INT32U	jpeg_valid_h;
	INT32U	jpeg_extend_w;
	INT32U	jpeg_extend_h;

	INT32U	output_addr;
	INT32U	output_format;
	INT32U	jpeg_output_w;
	INT32U	jpeg_output_h;
	INT32U	jpeg_output_buffer_w;
	INT32U	jpeg_output_buffer_h;
	INT32U	boundary_color;
} mjpeg_info;

/**************************************************************************
 *                              E X T E R N A L                           *
 **************************************************************************/
extern video_decode_para_struct *p_vid_dec_para;
extern const GP_BITMAPINFOHEADER *p_bitmap_info;
extern const GP_WAVEFORMATEX *p_wave_info;
extern long  g_bitmap_info_len, g_wave_info_len;


extern INT32S video_decode_task_one_frame(void);
extern INT32U browse_dec_get_buf(void);
extern INT32S browse_parser_header_start(void);

extern INT32S vid_dec_set_scaler(
    INT32U scaler_flag,
    INT32U out_fmt,
    INT16U out_w,
    INT16U out_h,
    INT16U out_buf_w,
    INT16U out_buf_h
);

extern INT32S
browse_vid_dec_start(
	INT8U 	get_first,
	INT32U  get_first_out_buf,
	INT16U*  width,
	INT16U*  height
);

extern INT32S browse_vid_dec_stop(void);
extern void vid_dec_parser_end(void);
extern INT32U jpeg_header_get_size(void);
extern void aud_dec_dac_start(INT8U channel_no, INT32U sample_rate);
extern void aud_dec_dac_stop(void);
extern void vid_dec_aud_dec_memory_free(void);
extern INT32S vid_dec_set_aud_dec_work_mem(INT32U work_mem_size);
extern INT32S vid_dec_set_aud_dec_ring_buffer(void);
extern INT32U vid_dec_get_next_aud_buffer(void);

extern INT32S vid_dec_stop(void);

extern INT32S vid_dec_get_status(void);
extern void vid_dec_clear_status(INT32S flag);
extern void vid_dec_set_status(INT32S flag);

extern void vid_dec_set_video_flag(INT8S video_flag);
extern INT8S vid_dec_get_video_flag(void);
extern void vid_dec_set_audio_flag(INT8S audio_flag);
extern INT8S vid_dec_get_audio_flag(void);

extern void vid_dec_start_timer(void);
extern void vid_dec_stop_timer(void);

extern void aud_dec_ramp_down_handle(INT8U channel_no);
extern INT32S vid_dec_end_callback(INT32U bflag);

extern INT32S vid_dec_get_video_format(INT32U biCompression);
extern INT32U vid_dec_get_next_scaler_buffer(void);
extern INT32U vid_dec_get_next_vid_buffer(void);

extern void vid_dec_memory_free(void);
extern INT32S vid_dec_memory_alloc(void);

extern INT32S video_decode_task_start(void);
extern INT32S video_decode_task_stop(void);
extern INT32S video_decode_task_restart(void);
extern INT32S video_decode_task_nth_frame(void);

extern INT32S audio_decode_task_start(void);
extern INT32S audio_decode_task_stop(void);
extern INT32S audio_decode_task_pause(void);
extern INT32S audio_decode_task_resume(void);

extern INT32S vid_dec_h264_pscaler_start(INT32U in_width, INT32U in_height, INT32U buf_width, INT32U buf_height, void(*callback)(INT32U), INT32U out_buf);
extern INT32S vid_dec_h264_pscaler_stop(void);
extern INT32S vid_dec_h264_pscaler_status_check(void);
extern INT32U vid_dec_h264_pscaler_buf_get(INT8U a_or_b);
extern INT32S Jpeg_Encode_For_Thumbnail(JpegEnc_t *pJpegEnc);
extern INT32S jpeg_buffer_encode(JpegEnc_t *pJpegEnc);
extern INT32S vid_dec_parser_h264_bit_stream(INT8U *pdata, INT32U bs_size, INT16U *width, INT16U *height);

extern INT32S jpeg_buffer_decode_and_scale(IMAGE_DECODE_STRUCT *img_decode_struct, INT32U clip_xy, INT32U clip_size);
extern INT32S h264_buffer_decode_and_scale(IMAGE_DECODE_STRUCT *img_decode_struct, INT32U clip_xy, INT32U clip_size);

extern INT32S vid_dec_parser_start(INT16S fd, INT32S FileTpye, INT32U FileSize);
extern INT32S vid_dec_parser_stop(void);

extern INT32S vid_dec_entry(void);
extern INT32S vid_dec_pause(void);
extern INT32S vid_dec_start(void);
extern INT32S vid_dec_resume(void);
extern INT32S vid_dec_exit(void);
extern INT32S ap_browse_H264_resume(void);

extern INT32S vid_dec_set_reverse_play(INT32U reverse_play_flag);
extern INT32S vid_dec_set_play_speed(INT32U speed);

extern INT32U Speed_reverse_flag ;
extern INT32U raw_data_addr;

extern INT16U video_decode_display_h_offset(void);
extern INT32U video_decode_display_offset(void);

#endif //__TASK_VIDEO_DECODER_H__

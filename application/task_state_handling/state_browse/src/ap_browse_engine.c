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
#include "ap_browse_engine.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
INT32S ap_browse_show(DISP_P disp_frame);

/*== JPEG ==*/
INT32S ap_browse_JPEG_get_first(IMAGE_DECODE_STRUCT *img_ptr);
INT32S ap_browse_JPEG_play(IMAGE_DECODE_STRUCT* img_ptr);
INT32S ap_browse_JPEG_stop(STOR_SERV_PLAYINFO *info_ptr);

/*== H264 ==*/
INT32S ap_browse_H264_get_first(IMAGE_DECODE_STRUCT *img_ptr);
INT32S ap_browse_H264_play(IMAGE_DECODE_STRUCT* img_ptr);
INT32S ap_browse_H264_stop(STOR_SERV_PLAYINFO *info_ptr);
INT32S ap_browse_H264_pause(void);
INT32S ap_browse_H264_resume(void);

extern void browse_dec_disp_where_set(INT8U disp_where);
extern INT32S browse_dec_JPEG_info_get(INT32U raw_data,INT32U raw_size,INT16U* ret_width,INT16U* ret_height);

#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
INT32S ap_browse_wav_get_first(IMAGE_DECODE_STRUCT *img_ptr);
extern void ap_browse_show_file_name(STOR_SERV_PLAYINFO *info_ptr,INT8U enable,INT32U frame);
#endif
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static BROWSE_STREAM_CTRL_T browse_stream[TK_IMAGE_TYPE_MAX] =
{
//TK_IMAGE_TYPE_JPEG
	{
		ap_browse_show,
		ap_browse_JPEG_get_first,
		ap_browse_JPEG_play,
		ap_browse_JPEG_stop
	}
	,
//TK_IMAGE_TYPE_H264
	{
		ap_browse_show,
		ap_browse_H264_get_first,
		ap_browse_H264_play,
		ap_browse_H264_stop,
		ap_browse_H264_pause,
		ap_browse_H264_resume
	}
    #if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
    ,
//TK_IMAGE_TYPE_WAV
	{
		ap_browse_show,
		ap_browse_wav_get_first,
		ap_browse_wav_play,
		ap_browse_wav_stop,
		audio_wav_pause,
		audio_wav_resume
	}
    #endif
};


/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
INT32S
ap_browse_show(
	DISP_P disp_frame
)
{
	if(display_task_msg_send(MSG_DISPLAY_TASK_MJPEG_DRAW, (void *)&disp_frame, sizeof(void *), MSG_PRI_NORMAL) != osOK)
	{
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

INT32S
video_decode_read_file_reload(
	INT16S fd,
	INT32U buf_addr,
	INT32U buf_size
)
{
	return read(fd, buf_addr, buf_size);
}

INT32S
ap_browse_JPEG_get_first(
	IMAGE_DECODE_STRUCT *img_ptr
)
{
	#define JPEG_DEC_PIECE_SIZE 32768
	INT32S nRet;
	INT32U	file_buf_size = img_ptr->source_size;
	INT32U	file_buf_addr;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	#if JPEG_DEC_PIECE_SIZE
	if(JPEG_DEC_PIECE_SIZE < img_ptr->source_size)
	{
		file_buf_size = JPEG_DEC_PIECE_SIZE;
	}
	#endif

	// allocate buffer to decode
	file_buf_addr = (INT32U) gp_malloc_align(file_buf_size, 64);

	if(!file_buf_addr)
	{
		ERR_PRINT("allocate fial\r\n");
		return STATUS_FAIL;
	}

	img_ptr->image_source = (INT32S) file_buf_addr;
	img_ptr->data_reload = video_decode_read_file_reload;
	img_ptr->reload_size = file_buf_size;

	if(video_decode_read_file_reload(img_ptr->file_handle,file_buf_addr,file_buf_size) <= 0)
	{
		if(file_buf_addr)
		{
			gp_free((void *)file_buf_addr);
		}
		return STATUS_FAIL;
	}

	nRet = jpeg_buffer_decode_and_scale(img_ptr, 0 ,0); // Use scaler

	if(file_buf_addr)
	{
		gp_free((void *)file_buf_addr);
	}

	return nRet;
}

INT32S
ap_browse_JPEG_play(
	IMAGE_DECODE_STRUCT *img_ptr
)
{
	#define JPEG_DEC_HEADER_SIZE 2048
	#define JPEG_LIMIT_W 640
	#define JPEG_LIMIT_H 480
	#define JPEG_RE_ENC_BUF_SIZE (150*1024)

	INT32S nRet = STATUS_OK;
	INT16U img_width,img_height;
	INT32U jpeg_header_buf = 0;
	INT32U send_buf = 0;
	INT32U jpeg_dec_out_buf = 0;
	INT32U jpeg_enc_out_buf = 0;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	browse_dec_disp_where_set(img_ptr->disp_where);

	// allocate buffer to decode
	jpeg_header_buf = (INT32U) gp_malloc_align(JPEG_DEC_HEADER_SIZE, 64);
	if(!jpeg_header_buf)
	{
		RETURN(STATUS_FAIL);
	}

	cache_invalid_range(jpeg_header_buf,JPEG_DEC_HEADER_SIZE);
	if(read(img_ptr->file_handle, jpeg_header_buf, JPEG_DEC_HEADER_SIZE) < 0)
	{
		RETURN(STATUS_FAIL);
	}

	nRet = browse_dec_JPEG_info_get(jpeg_header_buf,JPEG_DEC_HEADER_SIZE,&img_width,&img_height);
	if(nRet == STATUS_FAIL)
	{
		RETURN(STATUS_FAIL);
	}

	if(img_width && img_height)
	{
		// Seek to the beginning fo file after getting first frame
		lseek(img_ptr->file_handle, 0, SEEK_SET);

		if((img_width <= JPEG_LIMIT_W) && (img_height <= JPEG_LIMIT_H))
		{
			send_buf = (INT32U)gp_malloc_align(img_ptr->source_size, 64);
			if(!send_buf)
			{
				RETURN(STATUS_FAIL);
			}

			cache_invalid_range(send_buf,img_ptr->source_size);
			if(read(img_ptr->file_handle, send_buf, img_ptr->source_size) > 0)
			{
				#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
				rtsp_jpeg_send(send_buf, img_ptr->source_size, xTaskGetTickCount(), img_width, img_height, NULL);
				#endif
			}

			if(send_buf)
			{
				gp_free((void *)send_buf);
			}

			RETURN(STATUS_OK);
		}
		else
		{
			jpeg_dec_out_buf = (INT32U)gp_malloc_align(((JPEG_LIMIT_W*JPEG_LIMIT_H*3)>>1), 64);
			if(!jpeg_dec_out_buf)
			{
				RETURN(STATUS_FAIL);
			}

			jpeg_enc_out_buf = (INT32U)gp_malloc_align(JPEG_RE_ENC_BUF_SIZE, 64);
			if(!jpeg_enc_out_buf)
			{
				RETURN(STATUS_FAIL);
			}

			cache_invalid_range(jpeg_dec_out_buf, ((JPEG_LIMIT_W*JPEG_LIMIT_H*3)>>1));

			img_ptr->output_buffer_pointer = jpeg_dec_out_buf;
			img_ptr->output_buffer_width = JPEG_LIMIT_W;
			img_ptr->output_buffer_height = JPEG_LIMIT_H;
			img_ptr->output_image_width = JPEG_LIMIT_W;
			img_ptr->output_image_height = JPEG_LIMIT_H;
			img_ptr->output_format = C_SCALER_CTRL_OUT_GP420;

			if(img_width * 9 == img_height * 16)
			{
                img_ptr->output_buffer_height = JPEG_LIMIT_W * 9 / 16;
                img_ptr->output_image_height = JPEG_LIMIT_W * 9 / 16;
			}

			nRet = ap_browse_JPEG_get_first(img_ptr);
			if(nRet == STATUS_FAIL)
			{
				RETURN(STATUS_FAIL);
			}

			JpegEnc_t JpegEnc = {0};

			JpegEnc.hwidx = JPEGA;
			JpegEnc.quality_value = 50;
			JpegEnc.yuv_fmt = VID_DATA_420;
			JpegEnc.yuv_sample_fmt = JPEG_YUV420;
			JpegEnc.width = img_ptr->output_image_width;
			JpegEnc.height = img_ptr->output_image_height;
			JpegEnc.input_buffer_y = jpeg_dec_out_buf;
			JpegEnc.output_buffer = jpeg_enc_out_buf;
			JpegEnc.output_buffer_len = JPEG_RE_ENC_BUF_SIZE;

			cache_invalid_range(jpeg_enc_out_buf, JPEG_RE_ENC_BUF_SIZE);

			nRet = jpeg_buffer_encode(&JpegEnc);
			if(nRet == STATUS_FAIL)
			{
				RETURN(STATUS_FAIL);
			}

			#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
			rtsp_jpeg_send(jpeg_enc_out_buf, JpegEnc.encode_size, xTaskGetTickCount(), JpegEnc.width, JpegEnc.height, NULL);
			#endif

			RETURN(STATUS_OK);
		}
	}
	else
	{
		ERR_PRINT("Can't find image width/height\r\n");
		RETURN(STATUS_FAIL);
	}

Return:
	if(jpeg_header_buf)
	{
		gp_free((void *)jpeg_header_buf);
	}

	if(send_buf)
	{
		gp_free((void *)send_buf);
	}

	if(jpeg_dec_out_buf)
	{
		gp_free((void *)jpeg_dec_out_buf);
	}

	if(jpeg_enc_out_buf)
	{
		gp_free((void *)jpeg_enc_out_buf);
	}

	if(img_ptr->file_handle)
	{
		close(img_ptr->file_handle);
	}


	app_msg_send(MSG_APQ_MJPEG_DECODE_END, NULL, 0, MSG_PRI_NORMAL);

	return nRet;
}

INT32S
ap_browse_JPEG_stop(
	STOR_SERV_PLAYINFO *info_ptr
)
{
	return STATUS_OK;
}

/* == H264 ==*/
INT32S
ap_browse_H264_get_first(
	IMAGE_DECODE_STRUCT *img_ptr
)
{
	INT32S nRet;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = h264_buffer_decode_and_scale(img_ptr, 0 ,0); // Use pscaler

	return nRet;
}

void
ap_H264_stop(
	INT16S	file_handle
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	vid_dec_stop();
	vid_dec_parser_stop();
	close(file_handle);
	file_handle = -1;
}

INT32S
ap_browse_H264_play(
	IMAGE_DECODE_STRUCT *img_ptr
)
{
	INT32S nRet = STATUS_OK;
	#if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
    static INT16U jpeg_output_image_x_offset;
	#endif
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	browse_dec_disp_where_set(img_ptr->disp_where);

	// Seek to the beginning fo file after getting first frame
	lseek(img_ptr->file_handle, 0, SEEK_SET);

	nRet = vid_dec_parser_start(img_ptr->file_handle, FILE_TYPE_MOV,img_ptr->source_size);
	if(nRet == STATUS_FAIL)
	{
		RETURN(STATUS_FAIL);
	}

	if (audio_playing_state_get() == STATE_PLAY)
	{
		osDelay(2*MAX_DAC_BUFFERS);
	}
	#if C_BROWSE_FULL_SCREEN == CUSTOM_OFF

	INT32U video_output_width = img_ptr->output_image_width;
	DBG_PRINT("video_valid_width = %d\r\n",p_bitmap_info->biWidth);
	DBG_PRINT("video_valid_height = %d\r\n",p_bitmap_info->biHeight);
	DBG_PRINT("video_output_image_width = %d\r\n",img_ptr->output_image_width);
	DBG_PRINT("video_output_image_height = %d\r\n",img_ptr->output_image_height);
	jpeg_output_image_x_offset = 0;
    if(p_bitmap_info->biWidth*img_ptr->output_image_height != p_bitmap_info->biHeight*img_ptr->output_image_width)
    {
        img_ptr->output_image_width = (((p_bitmap_info->biWidth*img_ptr->output_image_height)/p_bitmap_info->biHeight)/16)<<4;
        jpeg_output_image_x_offset = ((video_output_width - img_ptr->output_image_width)/4)<<1;
        DBG_PRINT("video_output_image_width = %d\r\n",img_ptr->output_image_width);
        DBG_PRINT("video_output_image_x_offset = %d\r\n",jpeg_output_image_x_offset);
        if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
        {
            img_ptr->output_buffer_pointer += jpeg_output_image_x_offset;
            DBG_PRINT("GP420\r\n");
        }
        else
        {
            img_ptr->output_buffer_pointer += jpeg_output_image_x_offset*2;
            DBG_PRINT("GP565/YUYV\r\n");
        }
    }
    video_decode_display_x_offset_set(jpeg_output_image_x_offset);
	#endif
	vid_dec_set_scaler(SCALE_FIT_BUF,
		img_ptr->output_format,
		img_ptr->output_image_width,
		img_ptr->output_image_height,
		img_ptr->output_buffer_width,
		img_ptr->output_image_height
	);

	nRet = vid_dec_start();
	if(nRet == STATUS_FAIL)
	{
		RETURN(STATUS_FAIL);
	}

	return STATUS_OK;
Return:

	ap_H264_stop(img_ptr->file_handle);

	return nRet;
}

INT32S
ap_browse_H264_stop(
	STOR_SERV_PLAYINFO *info_ptr
)
{
	ap_H264_stop(info_ptr->file_handle);
	return STATUS_OK;
}

INT32S
ap_browse_H264_pause(
	void
)
{
	return vid_dec_pause();
}

INT32S
ap_browse_H264_resume(
	void
)
{
	return vid_dec_resume();
}

BROWSE_STREAM_CTRL_T*
ap_browse_ctrl_get(
	INT8U ctrl_idx
)
{
	#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
	if(ctrl_idx > TK_IMAGE_TYPE_WAV)
	{
		return NULL;
	}
	#else
	if(ctrl_idx > TK_IMAGE_TYPE_H264)
	{
		return NULL;
	}
	#endif

	return &browse_stream[ctrl_idx];
}

#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
INT32S
ap_browse_wav_get_first(
	IMAGE_DECODE_STRUCT *img_ptr
)
{
    ap_browse_show_file_name(0, 0, img_ptr->output_buffer_pointer);
    return 0;
}
#endif

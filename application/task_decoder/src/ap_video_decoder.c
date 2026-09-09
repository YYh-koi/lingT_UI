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
#include "application.h"
#include "circular_buffer.h"
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
// scaler mode
#define C_SCALER_FULL_SCREEN				1
#define C_SCALER_BY_RATIO					2
#define C_SCALER_FULL_SCREEN_BY_RATIO		3
#define C_SCALER_FULL_SCREEN_BY_DIGI_ZOOM	4
#define C_SCALER_RATIO_USER					5
#define C_SCALER_RATIO_FD					6

#define BROWSE_PSCALER_USE_NUM				PSCALER_A

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
INT8U h264_Mb2scan_triger=1;
extern INT8U Get_1st_decode_frm;
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static vid_dec_buf_struct  g_audio;
static vid_dec_buf_struct  g_video;
static vid_dec_buf_struct  g_scaler;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/
extern INT8U browse_dec_disp_where_get(void);
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
vid_dec_timer_isr(
	void
)
{
	if(p_vid_dec_para->audio_flag) {
		if(p_vid_dec_para->tv - p_vid_dec_para->ta < p_vid_dec_para->time_range) {
			p_vid_dec_para->tv += p_vid_dec_para->tick_time2;
		}

		if(p_vid_dec_para->ta - p_vid_dec_para->tv > p_vid_dec_para->time_range) {
			// Make video faster if ta is greater tv more than 32ms
			// This branch will only occur when ICE stop on debug point
			p_vid_dec_para->tv += p_vid_dec_para->tick_time2;
		}
	} else {
		p_vid_dec_para->tv += p_vid_dec_para->tick_time2;
	}

	if(p_vid_dec_para->tv - p_vid_dec_para->Tv >= 0) {
		if(p_vid_dec_para->post_cnt == p_vid_dec_para->pend_cnt) {
			if(video_decode_task_one_frame() == STATUS_OK) {
				p_vid_dec_para->post_cnt++;
			}
		}
	}

}

void
vid_dec_stop_timer(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	timer_stop(VIDEO_DECODE_TIMER);
}

void
vid_dec_start_timer(
	void
)
{
	INT32U	temp, freq_hz;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(p_vid_dec_para->audio_flag)
	{
		temp = (0x10000 - (R_TIMERE_PRELOAD & 0xFFFF)) * p_vid_dec_para->n;
		freq_hz = SystemCoreClock / 2 / temp;
		if(SystemCoreClock % (2 * temp))
			freq_hz++;
	}
	else
	{
		freq_hz = TIME_BASE_TICK_RATE;
	}
	timer_freq_setup(VIDEO_DECODE_TIMER, freq_hz, 0, vid_dec_timer_isr);
}

/**************************************************************************
 *                                Memory                                  *
 **************************************************************************/
static void
viddec_buffer_number_init(
	void
)
{
	gp_memset((INT8S*)&g_audio, 0x00, sizeof(vid_dec_buf_struct));
	gp_memset((INT8S*)&g_video, 0x00, sizeof(vid_dec_buf_struct));
	gp_memset((INT8S*)&g_scaler, 0x00, sizeof(vid_dec_buf_struct));

	//audio
	g_audio.total_number = AUDIO_FRAME_NO;

	//video
    if(((p_vid_dec_para->video_format == C_MJPG_FORMAT) || (p_vid_dec_para->video_format == C_H264_FORMAT)) && (browse_dec_disp_where_get() == DISP_ON_WIFI))
    {
        g_video.total_number = VIDEO_FRAME_NO; // For re-encode
        g_scaler.total_number = SCALER_FRAME_NO; // For scale-down
    }

}

static INT32S
viddec_audio_memory_alloc(
	void
)
{
	INT32S	i, nRet = STATUS_OK;

	if(p_vid_dec_para->audio_flag == 0)
		RETURN(STATUS_OK);

	for(i = 0; i < g_audio.total_number; i++)
	{
		if(p_vid_dec_para->audio_decode_addr[i] == 0)
		{
			p_vid_dec_para->audio_decode_addr[i] = (INT32U) gp_malloc_align(p_vid_dec_para->aud_frame_size, 32);
			if(p_vid_dec_para->audio_decode_addr[i] == 0)
				RETURN(STATUS_FAIL);
			gp_memset((INT8S *) p_vid_dec_para->audio_decode_addr[i], 0x80, p_vid_dec_para->aud_frame_size);
		}
	}

	nRet = STATUS_OK;
Return:
	for(i = 0; i < g_audio.total_number; i++)
	{
		DBG_PRINT("AudioFrame = 0x%x\r\n", p_vid_dec_para->audio_decode_addr[i]);
	}

	return nRet;
}

static void
viddec_audio_memory_free(
	void
)
{
	INT32S	i;

	for(i = 0; i < g_audio.total_number; i++)
	{
		if(p_vid_dec_para->audio_decode_addr[i])
		{
			gp_free((void *) p_vid_dec_para->audio_decode_addr[i]);
			p_vid_dec_para->audio_decode_addr[i] = 0;
		}
	}
}
static void
video_dec_memory_free(
	void
)
{
	circular_buf_free((cbuf_handle_t)p_vid_dec_para->video_decode_cb_handle);
	p_vid_dec_para->video_decode_cb_handle = 0;
	gp_free((void* )p_vid_dec_para->video_decode_cb);
	p_vid_dec_para->video_decode_cb = 0;
	gp_free((void* )p_vid_dec_para->video_decode_addr);
	p_vid_dec_para->video_decode_addr = 0;
	DBG_PRINT("%s\r\n", __FUNCTION__);
}

static void
scaler_output_memory_free(
	void
)
{
	circular_buf_free((cbuf_handle_t)p_vid_dec_para->scaler_output_cb_handle);
	p_vid_dec_para->scaler_output_cb_handle = 0;
	gp_free((void* )p_vid_dec_para->scaler_output_cb);
	p_vid_dec_para->scaler_output_cb = 0;
	gp_free((void* )p_vid_dec_para->scaler_output_addr);
	p_vid_dec_para->scaler_output_addr = 0;
	DBG_PRINT("%s\r\n", __FUNCTION__);
}

INT32S
viddec_video_memory_alloc(
	void
)
{
	INT32S i, block = 0, nRet;

	if(p_vid_dec_para->video_flag == 0 || browse_dec_disp_where_get() == DISP_ON_TFT) {
		RETURN(STATUS_OK);
	}

	if((p_vid_dec_para->video_format == C_MJPG_FORMAT) || (p_vid_dec_para->video_format == C_H264_FORMAT))
	{

		block = p_vid_dec_para->buffer_output_width * p_vid_dec_para->buffer_output_height * 2;
		if(!p_vid_dec_para->video_decode_addr){
			p_vid_dec_para->video_decode_addr = (INT32U)gp_malloc_align(block * g_video.total_number, 64);
			if(!p_vid_dec_para->video_decode_addr){
				ERR_PRINT("\r\n");
				RETURN(STATUS_FAIL);
			}
		}

		if(!p_vid_dec_para->video_decode_cb){
			p_vid_dec_para->video_decode_cb = (INT32U)gp_malloc_align((g_video.total_number * sizeof(p_vid_dec_para->video_decode_addr)), 64);
			if(!p_vid_dec_para->video_decode_cb)
			{
				video_dec_memory_free();
				ERR_PRINT("\r\n");
				RETURN(STATUS_FAIL);
			}
		}

		if(!p_vid_dec_para->video_decode_cb_handle){
			p_vid_dec_para->video_decode_cb_handle = (INT32U)circular_buf_init((unsigned int *)p_vid_dec_para->video_decode_cb, g_video.total_number);
			if(!p_vid_dec_para->video_decode_cb_handle)
			{
				video_dec_memory_free();
				ERR_PRINT("\r\n");
				RETURN(STATUS_FAIL);
			}
		}

		for (i=0; i < g_video.total_number; ++i)
		{
			if(circular_buf_try_put((cbuf_handle_t)p_vid_dec_para->video_decode_cb_handle, (INT32U)(p_vid_dec_para->video_decode_addr + (block * i))) < 0)
			{
				video_dec_memory_free();
				ERR_PRINT("\r\n");
				RETURN(STATUS_FAIL);
			}
			DBG_PRINT("video_decode_addr[%d]=0x%x\r\n", i, (p_vid_dec_para->video_decode_addr + (block * i)));
		}

		//scaler
		block = p_vid_dec_para->buffer_output_width * p_vid_dec_para->buffer_output_height * 2;
		if(!p_vid_dec_para->scaler_output_addr){
			p_vid_dec_para->scaler_output_addr = (INT32U)gp_malloc_align(block * g_scaler.total_number, 64);
			if(!p_vid_dec_para->scaler_output_addr){
				ERR_PRINT("\r\n");
				RETURN(STATUS_FAIL);
			}
		}

		if(!p_vid_dec_para->scaler_output_cb){
			p_vid_dec_para->scaler_output_cb = (INT32U)gp_malloc_align((g_video.total_number * sizeof(p_vid_dec_para->scaler_output_addr)), 64);
			if(!p_vid_dec_para->scaler_output_cb)
			{
				scaler_output_memory_free();
				ERR_PRINT("\r\n");
				RETURN(STATUS_FAIL);
			}
		}

		if(!p_vid_dec_para->scaler_output_cb_handle){
			p_vid_dec_para->scaler_output_cb_handle = (INT32U)circular_buf_init((unsigned int *)p_vid_dec_para->scaler_output_cb, g_video.total_number);
			if(!p_vid_dec_para->scaler_output_cb_handle)
			{
				scaler_output_memory_free();
				ERR_PRINT("\r\n");
				RETURN(STATUS_FAIL);
			}
		}

		for (i=0; i < g_video.total_number; ++i)
		{
			if(circular_buf_try_put((cbuf_handle_t)p_vid_dec_para->scaler_output_cb_handle, (INT32U)(p_vid_dec_para->scaler_output_addr + (block * i))) < 0)
			{
				scaler_output_memory_free();
				ERR_PRINT("\r\n");
				RETURN(STATUS_FAIL);
			}
			DBG_PRINT("scaler_output_addr[%d]=0x%x\r\n", i, (p_vid_dec_para->scaler_output_addr + (block * i)));
		}

	}

	nRet = STATUS_OK;
Return:

	#if 1 // Debug
	for(i=0; i<g_video.total_number; i++) {
		DBG_PRINT("VideoFrame = 0x%x\r\n", p_vid_dec_para->video_decode_addr + (block * i));
	}

	for(i=0; i<g_scaler.total_number; i++) {
		DBG_PRINT("ScalerFrame = 0x%x\r\n", p_vid_dec_para->scaler_output_addr + (block * i));
	}
	#endif

	return nRet;

}

INT32S
vid_dec_memory_alloc(
	void
)
{
	//init index
	viddec_buffer_number_init();
	if(viddec_audio_memory_alloc() < 0) {
		return STATUS_FAIL;
	}

	if(viddec_video_memory_alloc() < 0) {
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

void
vid_dec_memory_free(
	void
)
{
	viddec_audio_memory_free();

	video_dec_memory_free();

	scaler_output_memory_free();
}

/**************************************************************************
 *                                Buffer                                  *
 **************************************************************************/
INT32U
vid_dec_get_next_aud_buffer(
	void
)
{
	INT32U addr;
	do {
		addr = p_vid_dec_para->audio_decode_addr[g_audio.current_index++];
		if(g_audio.current_index >= g_audio.total_number)
			g_audio.current_index = 0;
	} while(0);
	return addr;
}

INT32U
vid_dec_get_next_vid_buffer(
	void
)
{
	INT32U addr;
	if(circular_buf_get((cbuf_handle_t)p_vid_dec_para->video_decode_cb_handle, &addr) < 0){
		addr = DUMMY_BUFFER_ADDRS;
	}
	return addr;
}

INT32U
vid_dec_get_next_scaler_buffer(
	void
)
{
	INT32U addr;
	if(circular_buf_get((cbuf_handle_t)p_vid_dec_para->scaler_output_cb_handle, &addr) < 0){
		addr = DUMMY_BUFFER_ADDRS;
	}
	return addr;
}

/**************************************************************************
 *                                Status                                  *
 **************************************************************************/
void
vid_dec_set_status(
	INT32S flag
)
{
	p_vid_dec_para->status |= flag;
}

void
vid_dec_clear_status(
	INT32S flag
)
{
	p_vid_dec_para->status &= ~flag;
}

INT32S
vid_dec_get_status(
	void
)
{
	return p_vid_dec_para->status;
}

/**************************************************************************
 *                            Video Info                                  *
 **************************************************************************/
void
vid_dec_set_video_flag(
	INT8S video_flag
)
{
	if(video_flag)
		p_vid_dec_para->video_flag = TRUE;
	else
		p_vid_dec_para->video_flag = FALSE;
}

INT8S
vid_dec_get_video_flag(
	void
)
{
	return p_vid_dec_para->video_flag;
}

INT32S
vid_dec_get_video_format(
	INT32U biCompression
)
{
	INT8U	data[4];

	data[0] = (biCompression >> 0) & 0xFF;	//X
	data[1] = (biCompression >> 8) & 0xFF;	//X
	data[2] = (biCompression >> 16) & 0xFF; //X
	data[3] = (biCompression >> 24) & 0xFF; //X
	if((data[0] == 'X' || data[0] == 'x') && (data[1] == 'V' || data[1] == 'v') && (data[2] == 'I' || data[2] == 'i') && (data[3] == 'D' || data[3] == 'd'))
	{
		DBG_PRINT("VidFormat = C_XVID_FORMAT\r\n");
		return C_XVID_FORMAT;
	}
	else
	if((data[0] == 'M' || data[0] == 'm') && (data[1] == '4') && (data[2] == 'S' || data[2] == 's') && (data[3] == '2'))
	{
		DBG_PRINT("VidFormat = C_M4S2_FORMAT\r\n");
		return C_M4S2_FORMAT;
	}
	else
	if((data[0] == 'H' || data[0] == 'h') && (data[1] == '2') && (data[2] == '6') && (data[3] == '3'))
	{
		DBG_PRINT("VidFormat = C_H263_FORMAT\r\n");
		return C_H263_FORMAT;
	}
	else
	if((data[0] == 'M' || data[0] == 'm') && (data[1] == 'J' || data[1] == 'j') && (data[2] == 'P' || data[2] == 'p') && (data[3] == 'G' || data[3] == 'g'))
	{
		DBG_PRINT("VidFormat = C_MJPG_FORMAT\r\n");
		return C_MJPG_FORMAT;
	}
	else if((data[0] == 'H' || data[0] == 'h') &&
			(data[1] == '2') &&
			(data[2] == '6') &&
			(data[3] == '4')) {
		DBG_PRINT("VidFormat = C_H264_FORMAT\r\n");
		return C_H264_FORMAT;
	}
	DBG_PRINT("NotSupportVidFormat = 0x%x\r\n", biCompression);
	return STATUS_FAIL;
}

INT32S
vid_dec_get_file_format(
	INT8S *pdata
)
{
	if((*(pdata + 0) == 'A' || *(pdata + 0) == 'a') && (*(pdata + 1) == 'V' || *(pdata + 1) == 'v') && (*(pdata + 2) == 'I' || *(pdata + 2) == 'i'))
	{
		return FILE_TYPE_AVI;
	}
	else
	if((*(pdata + 0) == 'M' || *(pdata + 0) == 'm') && (*(pdata + 1) == 'O' || *(pdata + 1) == 'o') && (*(pdata + 2) == 'V' || *(pdata + 2) == 'v'))
	{
		return FILE_TYPE_MOV;
	}
	else
	if((*(pdata + 0) == 'M' || *(pdata + 0) == 'm') && (*(pdata + 1) == 'P' || *(pdata + 1) == 'p') && (*(pdata + 2) == '4' || *(pdata + 2) == '4'))
	{
		return FILE_TYPE_MOV;
	}
	else
	if((*(pdata + 0) == '3' || *(pdata + 0) == '3') && (*(pdata + 1) == 'G' || *(pdata + 1) == 'g') && (*(pdata + 2) == 'P' || *(pdata + 2) == 'p'))
	{
		return FILE_TYPE_MOV;
	}
	else
	if((*(pdata + 0) == 'M' || *(pdata + 0) == 'm') && (*(pdata + 1) == '4' || *(pdata + 1) == '4') && (*(pdata + 2) == 'A' || *(pdata + 2) == 'a'))
	{
		return FILE_TYPE_MOV;
	}

	return STATUS_FAIL;
}

void
vid_dec_get_size(
	INT16U *width,
	INT16U *height
)
{
	*width = p_bitmap_info->biWidth;
	*height = p_bitmap_info->biHeight;
}

/**************************************************************************
 *                            Audio Info                                  *
 **************************************************************************/
void
vid_dec_set_audio_flag(
	INT8S audio_flag
)
{
	if(audio_flag)
		p_vid_dec_para->audio_flag = TRUE;
	else
		p_vid_dec_para->audio_flag = FALSE;
}

INT8S
vid_dec_get_audio_flag(
	void
)
{
	return p_vid_dec_para->audio_flag;
}

/**************************************************************************
 *                            Audio Decoder                               *
 **************************************************************************/
INT32S
vid_dec_set_aud_dec_work_mem(
	INT32U work_mem_size
)
{
	p_vid_dec_para->work_mem_size = work_mem_size;
	p_vid_dec_para->work_mem_addr = (INT8U *) gp_malloc_align(work_mem_size, 4);
	if(!p_vid_dec_para->work_mem_addr)
		return STATUS_FAIL;

	gp_memset((INT8S *) p_vid_dec_para->work_mem_addr, 0, work_mem_size);
	return STATUS_OK;
}

INT32S
vid_dec_set_aud_dec_ring_buffer(
	void
)
{
	p_vid_dec_para->ring_buffer_size = MultiMediaParser_GetAudRingLen(p_vid_dec_para->media_handle);
	p_vid_dec_para->ring_buffer_addr = (INT8U *) MultiMediaParser_GetAudRing(p_vid_dec_para->media_handle);
	if(!p_vid_dec_para->ring_buffer_addr)
		return STATUS_FAIL;

	return STATUS_OK;
}

void vid_dec_aud_dec_memory_free(void)
{
	if(p_vid_dec_para->work_mem_addr)
	{
		gp_free((void *) p_vid_dec_para->work_mem_addr);
	}

	p_vid_dec_para->work_mem_size = 0;
	p_vid_dec_para->work_mem_addr = 0;
	p_vid_dec_para->ring_buffer_size = 0;
	p_vid_dec_para->ring_buffer_addr = 0;
}

/**************************************************************************
 *                                 DAC                                    *
 **************************************************************************/
//dac init
void
aud_dec_dac_start(
	INT8U channel_no,
	INT32U sample_rate
)
{
	if(p_wave_info->wFormatTag == WAVE_FORMAT_MPEGLAYER3)	// 2 channel
		drv_l1_dac_stereo_set();
	else
	if(channel_no == 1)
		drv_l1_dac_mono_set();
	else
		drv_l1_dac_stereo_set();

	drv_l1_dac_sample_rate_set(sample_rate);
}

void
aud_dec_dac_stop(
	void
)
{
	drv_l1_dac_timer_stop();
}

void
aud_dec_ramp_down_handle(
	INT8U channel_no
)
{
	INT16S	last_ldata, last_rdata;
	INT16S	i, temp;

	temp = 0 - DAC_RAMP_DOWN_STEP;
	last_ldata = R_DAC_CHA_DATA;
	last_rdata = R_DAC_CHB_DATA;

	//unsigned to signed
	last_ldata ^= 0x8000;
	last_rdata ^= 0x8000;

	return;

	//change timer to 44100
	drv_l1_dac_sample_rate_set(44100);
	while(1)
	{
		if(last_ldata > 0x0)
		{
			last_ldata -= DAC_RAMP_DOWN_STEP;
		}
		else
		if(last_ldata < 0x0)
		{
			last_ldata += DAC_RAMP_DOWN_STEP;
		}

		if((last_ldata < DAC_RAMP_DOWN_STEP) && (last_ldata > temp))
		{
			last_ldata = 0;
		}

		if(channel_no == 2)
		{
			if(last_rdata > 0x0)
			{
				last_rdata -= DAC_RAMP_DOWN_STEP;
			}
			else
			if(last_rdata < 0x0)
			{
				last_rdata += DAC_RAMP_DOWN_STEP;
			}

			if((last_rdata < DAC_RAMP_DOWN_STEP) && (last_rdata > temp))
			{
				last_rdata = 0;
			}
		}

		for(i = 0; i < DAC_RAMP_DOWN_STEP_HOLD; i++)
		{
			if(channel_no == 2)
			{
				while(R_DAC_CHA_FIFO & 0x8000);
				R_DAC_CHA_DATA = last_ldata;
				while(R_DAC_CHB_FIFO & 0x8000);
				R_DAC_CHB_DATA = last_rdata;
			}
			else
			{
				while(R_DAC_CHA_FIFO & 0x8000);
				R_DAC_CHA_DATA = last_ldata;
			}
		}

		if(channel_no == 2)
		{
			if((last_ldata == 0x0) && (last_rdata == 0x0))
				break;
		}
		else
		{
			if(last_ldata == 0x0)
				break;
		}
	}

	drv_l1_dac_timer_stop();
}

/**************************************************************************
 *                                SCALER                                  *
 **************************************************************************/
INT32S
vid_dec_set_scaler(
	INT32U scaler_flag,
	INT32U out_fmt,
	INT16U out_w,
	INT16U out_h,
	INT16U out_buf_w,
	INT16U out_buf_h
)
{
	INT8U  enable = 0;
	INT32S nRet;

	if(p_vid_dec_para->video_format == C_MJPG_FORMAT) {
		switch(out_fmt)
		{
		case C_SCALER_CTRL_OUT_RGB565:
			enable = 1;
			p_vid_dec_para->video_decode_out_format = C_SCALER_CTRL_OUT_RGB565;
			break;

		case C_SCALER_CTRL_OUT_YUYV:
		case C_SCALER_CTRL_OUT_UYVY:
		case C_SCALER_CTRL_OUT_GP420:
			enable = 0;
			p_vid_dec_para->video_decode_out_format = out_fmt;
			break;

		default:
			DBG_PRINT("OutputFormatNoSupport!!!\r\n");
			RETURN(STATUS_FAIL);
		}
	} else {
		//mpeg4
		switch(out_fmt)
		{
		case C_SCALER_CTRL_OUT_RGB565:
			enable = 1;
			p_vid_dec_para->mpeg4_decode_out_format = C_MP4_OUTPUT_Y1UY0V;
			p_vid_dec_para->video_decode_in_format = C_SCALER_CTRL_IN_YUYV;
			p_vid_dec_para->video_decode_out_format = out_fmt;
			break;

		case C_SCALER_CTRL_OUT_YUYV:
			enable = 0;
			p_vid_dec_para->mpeg4_decode_out_format = C_MP4_OUTPUT_Y1UY0V;
			p_vid_dec_para->video_decode_in_format = C_SCALER_CTRL_IN_YUYV;
			p_vid_dec_para->video_decode_out_format = C_SCALER_CTRL_OUT_YUYV;
			break;

		case C_SCALER_CTRL_OUT_UYVY:
			enable = 0;
			p_vid_dec_para->mpeg4_decode_out_format = C_MP4_OUTPUT_UY1VY0;
			p_vid_dec_para->video_decode_in_format = C_SCALER_CTRL_IN_UYVY;
			p_vid_dec_para->video_decode_out_format = C_SCALER_CTRL_OUT_UYVY;
			break;

		case C_SCALER_CTRL_OUT_GP420:
			p_vid_dec_para->video_decode_out_format = out_fmt;
			break;

		default:
			DBG_PRINT("OutputFormatNoSupport!!!\r\n");
			RETURN(STATUS_FAIL);
		}
	}

	if((out_w != out_buf_w) || (out_h != out_buf_h)) {
		enable = 1;
	}

	if((p_bitmap_info->biWidth != out_w) || (p_bitmap_info->biHeight != out_h)) {
		enable = 1;
	}

	if(enable == 0) {
		// not need use scaler
		scaler_flag = SCALE_MAX;
	}

	switch(scaler_flag)
	{
	case SCALE_FULL_SCREEN:
	case SCALE_FIT_BUF:
		if(scaler_flag == SCALE_FULL_SCREEN) {
			p_vid_dec_para->scaler_flag = C_SCALER_FULL_SCREEN;
		} else {
			p_vid_dec_para->scaler_flag = C_SCALER_BY_RATIO;
		}
		p_vid_dec_para->image_output_width = out_w;
		p_vid_dec_para->image_output_height = out_h;
		p_vid_dec_para->buffer_output_width = out_buf_w;
		p_vid_dec_para->buffer_output_height = out_buf_h;
		p_vid_dec_para->xoffset = p_vid_dec_para->yoffset = 0;
		break;

	case NO_SCALE:
	case NO_SCALE_AT_CENTER:
		p_vid_dec_para->scaler_flag = C_SCALER_BY_RATIO;
		p_vid_dec_para->image_output_width = p_bitmap_info->biWidth;
		p_vid_dec_para->image_output_height = p_bitmap_info->biHeight;
		p_vid_dec_para->buffer_output_width = out_buf_w;
		p_vid_dec_para->buffer_output_height = out_buf_h;

		if(scaler_flag == NO_SCALE) {
			p_vid_dec_para->xoffset = p_vid_dec_para->yoffset = 0;
		} else {
			p_vid_dec_para->xoffset = ((out_buf_w - p_bitmap_info->biWidth) / 2) & (~0x0F);
			p_vid_dec_para->yoffset = ((out_buf_h - p_bitmap_info->biHeight) / 2) & (~0x0F);
		}
		break;

	case SCALE_MAX:
		p_vid_dec_para->scaler_flag = 0;
		p_vid_dec_para->image_output_width = p_bitmap_info->biWidth;
		p_vid_dec_para->image_output_height = p_bitmap_info->biHeight;
		p_vid_dec_para->buffer_output_width = p_bitmap_info->biWidth;
		p_vid_dec_para->buffer_output_height = p_bitmap_info->biHeight;
		p_vid_dec_para->xoffset = p_vid_dec_para->yoffset = 0;
		break;

	default:
		p_vid_dec_para->scaler_flag = C_SCALER_FULL_SCREEN;
		p_vid_dec_para->image_output_width = out_w;
		p_vid_dec_para->image_output_height = out_h;
		p_vid_dec_para->buffer_output_width = out_buf_w;
		p_vid_dec_para->buffer_output_height = out_buf_h;
		p_vid_dec_para->xoffset = p_vid_dec_para->yoffset = 0;
		break;
	}

	nRet = STATUS_OK;
Return:
	return nRet;
}

INT32S
vid_dec_h264_pscaler_start(
	INT32U in_width,
	INT32U in_height,
	INT32U buf_width,
	INT32U buf_height,
	void(*callback)(INT32U),
	INT32U out_buf
)
{
	INT32U xFactor, yFactor;
	INT32U video_output_format;
	INT32U x_offset = 0;
	INT32U output_w, output_h;

	drv_l1_pscaler_init(BROWSE_PSCALER_USE_NUM);

	drv_l1_pscaler_callback_register(BROWSE_PSCALER_USE_NUM, callback);

	switch(p_vid_dec_para->video_decode_out_format)
	{
	case C_SCALER_CTRL_OUT_RGB565:  video_output_format = PIPELINE_SCALER_OUTPUT_FORMAT_RGB565; break;
	case C_SCALER_CTRL_OUT_YUYV:	video_output_format = PIPELINE_SCALER_OUTPUT_FORMAT_YUYV; break;
	case C_SCALER_CTRL_OUT_GP420:	video_output_format = PIPELINE_SCALER_OUTPUT_FORMAT_GP420; break;
	default: DBG_PRINT("invalid pscaler output format %x\r\n", p_vid_dec_para->video_decode_out_format);
		return STATUS_FAIL;
	}

	output_w = p_vid_dec_para->image_output_width;
	output_h = p_vid_dec_para->image_output_height;

	p_vid_dec_para->H264_MB2SCAN_patch = 0;

	if (in_width >= 2048) { //patch mb2scan bug
		extern void PScaler_dec_callback_2(INT32U PScaler_Event);
		in_width = in_width >> 1;
		buf_width = buf_width >> 1;
		output_w = output_w >> 1;
		x_offset = output_w;
		drv_l1_pscaler_callback_register(0, PScaler_dec_callback_2);
		p_vid_dec_para->H264_MB2SCAN_patch = 1;
		if (video_output_format == PIPELINE_SCALER_OUTPUT_FORMAT_GP420){
			p_vid_dec_para->H264_MB2SCAN_patch_offset = output_w;
		} else {
			p_vid_dec_para->H264_MB2SCAN_patch_offset = output_w*2;
		}
		//DBG_PRINT("\r\n MB2SCAN_patch_offset=%d, OUT=0x%x ",p_vid_dec_para->H264_MB2SCAN_patch_offset , out_buf);

		h264_Mb2scan_triger = 1;
		Get_1st_decode_frm = 1;
	}

    xFactor = (in_width*0x10000)/output_w;
    yFactor = (in_height*0x10000)/output_h;
	#if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
    x_offset += (p_vid_dec_para->buffer_output_width - output_w);
	#endif
    if ((xFactor < 0x10000) && (xFactor*output_w  < in_width*0x10000))
		xFactor ++;
	if ((yFactor < 0x10000) && (yFactor*output_h  < in_height*0x10000))
		yFactor ++;

	drv_l1_pscaler_input_pixels_set(BROWSE_PSCALER_USE_NUM, buf_width, buf_height);
	drv_l1_pscaler_output_pixels_set(BROWSE_PSCALER_USE_NUM, xFactor, output_w, yFactor, output_h);
	drv_l1_pscaler_input_X_start_set(BROWSE_PSCALER_USE_NUM, 0);
	drv_l1_pscaler_input_Y_start_set(BROWSE_PSCALER_USE_NUM, 0);
	drv_l1_pscaler_output_pixels_X_offset_set(BROWSE_PSCALER_USE_NUM, x_offset);
	drv_l1_pscaler_output_fifo_line_set(BROWSE_PSCALER_USE_NUM, output_h, 0);
	drv_l1_pscaler_interrupt_set(BROWSE_PSCALER_USE_NUM, PIPELINE_SCALER_INT_ENABLE_FRAME_END|PIPELINE_SCALER_INT_ENABLE_BUF_A|PIPELINE_SCALER_INT_ENABLE_BUF_B);
	drv_l1_pscaler_input_format_set(BROWSE_PSCALER_USE_NUM, PIPELINE_SCALER_INPUT_FORMAT_YUYV);
	drv_l1_pscaler_output_format_set(BROWSE_PSCALER_USE_NUM, video_output_format);
	drv_l1_pscaler_input_source_set(BROWSE_PSCALER_USE_NUM, PIPELINE_SCALER_INPUT_SOURCE_MB420);

	if(out_buf) // Get first frame
	{
		drv_l1_pscaler_output_A_buffer_set(BROWSE_PSCALER_USE_NUM, out_buf);
		if (p_vid_dec_para->H264_MB2SCAN_patch==1) {
            drv_l1_pscaler_output_B_buffer_set(BROWSE_PSCALER_USE_NUM, (out_buf+p_vid_dec_para->H264_MB2SCAN_patch_offset));
		}else{
		   drv_l1_pscaler_output_B_buffer_set(BROWSE_PSCALER_USE_NUM, out_buf);
		}
	}
	else
	{
        out_buf = browse_dec_get_buf();
		drv_l1_pscaler_output_A_buffer_set(BROWSE_PSCALER_USE_NUM, out_buf);
		if (p_vid_dec_para->H264_MB2SCAN_patch==1) {
			Get_1st_decode_frm = 0;
            drv_l1_pscaler_output_B_buffer_set(BROWSE_PSCALER_USE_NUM, (out_buf+p_vid_dec_para->H264_MB2SCAN_patch_offset));
        }else{
            drv_l1_pscaler_output_B_buffer_set(BROWSE_PSCALER_USE_NUM, browse_dec_get_buf());
        }
	}

	/*
	due to 720p scale to 1920x480 got a problem , enable H first is a easy way to fixed it
	so  add it
*/
	if(p_vid_dec_para->H264_MB2SCAN_patch ==1){
		drv_l1_pscaler_H_first_enable(BROWSE_PSCALER_USE_NUM,0);
	}else{
		drv_l1_pscaler_H_first_enable(BROWSE_PSCALER_USE_NUM,1);
	}

	drv_l1_pscaler_start(BROWSE_PSCALER_USE_NUM);

	return STATUS_OK;
}

INT32S
vid_dec_h264_pscaler_stop(
	void
)
{
	drv_l1_pscaler_stop(BROWSE_PSCALER_USE_NUM);

	return STATUS_OK;
}

/*
	0: Pscaler A buffer
	1: Pscaler B buffer
*/
INT32U
vid_dec_h264_pscaler_buf_get(
	INT8U a_or_b
)
{
	INT32U ret;

	if(a_or_b)
	{
   		ret =  drv_l1_pscaler_output_B_buffer_get(BROWSE_PSCALER_USE_NUM);
		drv_l1_pscaler_output_B_buffer_set(BROWSE_PSCALER_USE_NUM, DUMMY_BUFFER_ADDRS);
	}
	else
	{
		ret =  drv_l1_pscaler_output_A_buffer_get(BROWSE_PSCALER_USE_NUM);
		drv_l1_pscaler_output_A_buffer_set(BROWSE_PSCALER_USE_NUM, DUMMY_BUFFER_ADDRS);
	}

	//DBG_PRINT("Buf:0x%x\r\n",ret);
	return ret;
}

INT32S
vid_dec_h264_pscaler_status_check(
	void
)
{
	INT32S ret;
	INT8U  time_out_cnt = 100;

	do
	{
		ret = drv_l1_pscaler_status_get(BROWSE_PSCALER_USE_NUM);
		if(ret & PIPELINE_SCALER_STATUS_FRAME_DONE)
		{
			return STATUS_OK;
		}

		osDelay(1);

		if(time_out_cnt > 0)
		{
			--time_out_cnt;
			if(time_out_cnt == 0)
			{
				return STATUS_FAIL;
			}
		}
	}
	while(1);
}


/**************************************************************************
 *						 H264 Header Parser							 *
 **************************************************************************/
const unsigned char * m_pStart;
unsigned short m_nLength;
int m_nCurrentBit;

/**
 * @brief   tool functions to read h264 stream
 */
static unsigned int ReadBit()
{
	//assert(m_nCurrentBit <= m_nLength * 8);
	int nIndex = m_nCurrentBit / 8;
	int nOffset = m_nCurrentBit % 8 + 1;

	m_nCurrentBit ++;
	return (m_pStart[nIndex] >> (8-nOffset)) & 0x01;
}

static unsigned int ReadBits(int n)
{
	int r = 0;
	int i;
	for (i = 0; i < n; i++)
	{
		r |= ( ReadBit() << ( n - i - 1 ) );
	}
	return r;
}

static unsigned int ReadExponentialGolombCode()
{
	int r = 0;
	int i = 0;

	while( (ReadBit() == 0) && (i < 32) )
	{
		i++;
	}

	r = ReadBits(i);
	r += (1 << i) - 1;
	return r;
}

static unsigned int ReadSE()
{
	int r = ReadExponentialGolombCode();
	if (r & 0x01)
	{
		r = (r+1)/2;
	}
	else
	{
		r = -(r/2);
	}
	return r;
}

/**
 * @brief   h264 header parse function
 */
static void Parse(const unsigned char * pStart, unsigned short nLen, INT16U* w, INT16U* h)
{
	m_pStart = pStart;
	m_nLength = nLen;
	m_nCurrentBit = 0;

	int frame_crop_left_offset=0;
	int frame_crop_right_offset=0;
	int frame_crop_top_offset=0;
	int frame_crop_bottom_offset=0;

	int profile_idc = ReadBits(8);
	/*int constraint_set0_flag = */ReadBit();
	/*int constraint_set1_flag = */ReadBit();
	/*int constraint_set2_flag = */ReadBit();
	/*int constraint_set3_flag = */ReadBit();
	/*int constraint_set4_flag = */ReadBit();
	/*int constraint_set5_flag = */ReadBit();
	/*int reserved_zero_2bits  = */ReadBits(2);
	/*int level_idc = */ReadBits(8);
	/*int seq_parameter_set_id = */ReadExponentialGolombCode();


	if( profile_idc == 100 || profile_idc == 110 ||
		profile_idc == 122 || profile_idc == 244 ||
		profile_idc == 44 || profile_idc == 83 ||
		profile_idc == 86 || profile_idc == 118 )
	{
		int chroma_format_idc = ReadExponentialGolombCode();

		if( chroma_format_idc == 3 )
		{
			/*int residual_colour_transform_flag = */ReadBit();
		}
		/*int bit_depth_luma_minus8 = */ReadExponentialGolombCode();
		/*int bit_depth_chroma_minus8 = */ReadExponentialGolombCode();
		/*int qpprime_y_zero_transform_bypass_flag = */ReadBit();
		int seq_scaling_matrix_present_flag = ReadBit();

		if (seq_scaling_matrix_present_flag)
		{
			int i=0;
			for ( i = 0; i < 8; i++)
			{
				int seq_scaling_list_present_flag = ReadBit();
				if (seq_scaling_list_present_flag)
				{
					int sizeOfScalingList = (i < 6) ? 16 : 64;
					int lastScale = 8;
					int nextScale = 8;
					int j=0;
					for ( j = 0; j < sizeOfScalingList; j++)
					{
						if (nextScale != 0)
						{
							int delta_scale = ReadSE();
							nextScale = (lastScale + delta_scale + 256) % 256;
						}
						lastScale = (nextScale == 0) ? lastScale : nextScale;
					}
				}
			}
		}
	}

	/*int log2_max_frame_num_minus4 = */ReadExponentialGolombCode();
	int pic_order_cnt_type = ReadExponentialGolombCode();
	if( pic_order_cnt_type == 0 )
	{
		/*int log2_max_pic_order_cnt_lsb_minus4 = */ReadExponentialGolombCode();
	}
	else if( pic_order_cnt_type == 1 )
	{
		/*int delta_pic_order_always_zero_flag = */ReadBit();
		/*int offset_for_non_ref_pic = */ReadSE();
		/*int offset_for_top_to_bottom_field = */ReadSE();
		int num_ref_frames_in_pic_order_cnt_cycle = ReadExponentialGolombCode();
		int i;
		for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
		{
			ReadSE();
			//sps->offset_for_ref_frame[ i ] = ReadSE();
		}
	}
	/*int max_num_ref_frames = */ReadExponentialGolombCode();
	/*int gaps_in_frame_num_value_allowed_flag = */ReadBit();
	int pic_width_in_mbs_minus1 = ReadExponentialGolombCode();
	int pic_height_in_map_units_minus1 = ReadExponentialGolombCode();
	int frame_mbs_only_flag = ReadBit();
	if( !frame_mbs_only_flag )
	{
		/*int mb_adaptive_frame_field_flag = */ReadBit();
	}
	/*int direct_8x8_inference_flag = */ReadBit();
	int frame_cropping_flag = ReadBit();
	if( frame_cropping_flag )
	{
		frame_crop_left_offset = ReadExponentialGolombCode();
		frame_crop_right_offset = ReadExponentialGolombCode();
		frame_crop_top_offset = ReadExponentialGolombCode();
		frame_crop_bottom_offset = ReadExponentialGolombCode();
	}
	/*int vui_parameters_present_flag = */ReadBit();
	pStart++;

	int Width = ((pic_width_in_mbs_minus1 +1)*16) - frame_crop_left_offset*2 - frame_crop_right_offset*2;
	int Height = ((2 - frame_mbs_only_flag)* (pic_height_in_map_units_minus1 +1) * 16) - (frame_crop_top_offset * 2) - (frame_crop_bottom_offset * 2);

	DBG_PRINT("Parsed H.264 stream for width and height: WxH = %dx%d\r\n",Width,Height);

	*w = Width;
	*h = Height;
}

/**
 * @brief   h264 bit stream header parser to get width and height information
 * @param	pdata:		bit stream source address
 * @param	bs_size:	bit stream size
 * @param	width:		pointer to store video width value
 * @param	height:		pointer to store video height value
 * @return  0 SUCCESS/-1 FAIL
 */
INT32S vid_dec_parser_h264_bit_stream(INT8U *pdata, INT32U bs_size, INT16U *width, INT16U *height)
{
	INT8U* p = pdata;
	INT8U* end = p+bs_size;
	while(p < end)
	{
		if (*p == 0 && *(p+1) == 0 && *(p+2) == 0 && *(p+3) == 1)
		{
			p+= 3;

			if (((*(p+1)) & 0x1f) == 7) // sps
			{
				p+=2;
				break;
			}
		}

		p++;
	}

	if (p >= end)
		return -1;
	 Speed_reverse_flag = 0;
	Parse(p, (INT32U)end - (INT32U)p, width, height);

	return 0;
}

INT16U video_decode_display_h_offset(void)
{
	if(state_wifi_status_get() & FLAG_CONNECT)
		return 0;
	else
		return drv_l2_display_off_lines_video_get();
}


INT32U video_decode_display_offset(void)
{
	INT32U output_buffer_offset;
	INT16U tft_w = drv_l2_display_tft_width_get();
	if(drv_l2_display_tft_fmt_get() == DISP_FMT_RGB565)
	{
		output_buffer_offset = tft_w*video_decode_display_h_offset()*2;
	}
	else if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
	{
		output_buffer_offset = tft_w*video_decode_display_h_offset()*3/2;
	}
	else
	{
		output_buffer_offset = tft_w*video_decode_display_h_offset()*2;
	}
	return output_buffer_offset;
}

#if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
INT32U c_video_decode_x_offset = 0;
INT32U video_decode_display_x_offset_set(INT32U offset)
{
    c_video_decode_x_offset = offset;
}

INT32U video_decode_display_x_offset_get(void)
{
    if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
    return c_video_decode_x_offset;
    else
    return c_video_decode_x_offset*2;
}

void video_decode_display_do_padding(INT32U frame_addr,INT32U buffer_width,INT32U buffer_height,INT32U x_offset) //×óÓÒÌîºÚÉ«
{
    INT32U i,j,cnt;
    INT16U *ptr;
    if(x_offset == 0)
    return;
    switch(drv_l2_display_tft_fmt_get())
	{
        case DISP_FMT_GP420:

            cnt = buffer_height*3/2;
            for(i = 0; i < cnt; i++)
            {
                if((i % 3) == 1)
                {
                    gp_memset((INT8S *)(frame_addr + i * buffer_width), 0x80, x_offset);
                    gp_memset((INT8S *)(frame_addr + (i + 1) * buffer_width - x_offset), 0x80, x_offset);
                }
                else
                {
                    gp_memset((INT8S *)(frame_addr + i * buffer_width), 0, x_offset);
                    gp_memset((INT8S *)(frame_addr + (i + 1) * buffer_width - x_offset), 0, x_offset);
                }
            }
            break;
        case DISP_FMT_YUYV:
            for(i = 0; i < buffer_height; i++)
            {

                ptr = (INT16U *)(frame_addr + i * buffer_width * 2);
                for(j = 0; j < x_offset; j++)
                {
                    *ptr++ = 0x0080;
                }

                ptr = (INT16U *)(frame_addr + (i + 1) * buffer_width * 2 - x_offset * 2);
                for(j = 0; j < x_offset; j++)
                {
                    *ptr++ = 0x0080;
                }
            }
        break;

        default: //RGB565
            for(i = 0; i < buffer_height; i++)
            {
                ptr = (INT16U *)(frame_addr + i * buffer_width * 2);
                for(j = 0; j < x_offset; j++)
                {
                    *ptr++ = 0;
                }

                ptr = (INT16U *)(frame_addr + (i + 1) * buffer_width * 2 - x_offset * 2);
                for(j = 0; j < x_offset; j++)
                {
                    *ptr++ = 0;
                }
            }
        break;
    }
}
#endif

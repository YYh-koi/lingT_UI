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
#include "AviParser.h"
#include "QtffParser.h"
#include "circular_buffer.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define BROWSE_DEC_STATE_TASK_STACK_SIZE	4096
#define BROWSE_DEC_STATE_TASK_Q_MAX			5
#define BROWSE_DEC_STATE_TASK_ACK_MAX		1

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
MSG_Q_ID	vid_dec_state_q;
MSG_Q_ID	vid_dec_state_ack_m;

video_decode_para_struct	vid_dec_para;
video_decode_para_struct	*p_vid_dec_para = &vid_dec_para;

long g_bitmap_info_len, g_wave_info_len;
const GP_BITMAPINFOHEADER	*p_bitmap_info;
const GP_WAVEFORMATEX		*p_wave_info;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/
extern INT32S video_decode_task_create(void);
extern INT32S audio_decode_task_create(void);
extern INT32S video_decode_task_del(void);
extern INT32S audio_decode_task_del(void);

#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
extern INT32S get_audio_curr_time(void);
#endif
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
INT32S video_decode_state_task_create(void);
INT32S video_decode_state_task_del(void);

// 這個變數是為了當快進快退處於reverse 情況時,要確保每次讀VID BUF 都會去加HEADER
INT32U Speed_reverse_flag = 0; // 1: reverse 0: normal

static unsigned int playback_ms_unit=0;

INT32S
vid_dec_entry(
	void
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = video_decode_state_task_create();
	if(nRet == STATUS_FAIL)
	{
		RETURN(STATUS_FAIL);
	}

	nRet = video_decode_task_create();
	if(nRet == STATUS_FAIL)
	{
		RETURN(STATUS_FAIL);
	}

	nRet = audio_decode_task_create();
	if(nRet == STATUS_FAIL)
	{
		RETURN(STATUS_FAIL);
	}

	return STATUS_OK;

Return:
	return nRet;
}

INT32S
vid_dec_exit(
	void
)
{
	INT32S	nRet = STATUS_OK;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = video_decode_state_task_del();
	if(nRet == STATUS_FAIL)
	{
		RETURN(STATUS_FAIL);
	}

	nRet = video_decode_task_del();
	if(nRet == STATUS_FAIL)
	{
		RETURN(STATUS_FAIL);
	}

	nRet = audio_decode_task_del();
	if(nRet == STATUS_FAIL)
	{
		RETURN(STATUS_FAIL);
	}

	return STATUS_OK;

Return:
	return nRet;
}

INT32S
vid_dec_parser_start(
	INT16S fd,
	INT32S FileTpye,
	INT32U FileSize
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(fd < 0)
	{
		RETURN(STATUS_FAIL);
	}

	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR)
	{
		RETURN(STATUS_FAIL);
	}

	p_vid_dec_para->file_handle = fd;
	p_vid_dec_para->FileType = FileTpye;
	p_vid_dec_para->FileSize = FileSize;

	if(!(vid_dec_get_status() & C_VIDEO_DECODE_PARSER))
	{
		//wait 10 sec
		POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PARSER_HEADER_START, vid_dec_state_ack_m, 10000, MSG_PRI_NORMAL);
		vid_dec_clear_status(C_VIDEO_DECODE_ALL);
		vid_dec_set_status(C_VIDEO_DECODE_PARSER);
	}

Return:
	return nRet;
}

INT32S
vid_dec_parser_stop(
	void
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR)
	{
		RETURN(STATUS_FAIL);
	}

	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER_NTH)
	{
		RETURN(STATUS_FAIL);
	}

	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER)
	{
		vid_dec_clear_status(C_VIDEO_DECODE_PARSER);
		POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PARSER_HEADER_STOP, vid_dec_state_ack_m, 10000, MSG_PRI_NORMAL);
	}

Return:
	return nRet;
}

INT32S
vid_dec_start(
	void
)
{
	INT32S	nRet =STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR)
	{
		RETURN(STATUS_FAIL);
	}

	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER_NTH)
	{
		RETURN(STATUS_FAIL);
	}

	if(!(vid_dec_get_status() & C_VIDEO_DECODE_PLAY))
	{
		POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_START, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);
		vid_dec_set_status(C_VIDEO_DECODE_PLAY);
	}
	display_task_msg_send(MSG_DISPLAY_TASK_PLAY_TIME_DRAW, NULL, 0, MSG_PRI_NORMAL);

Return:
	return nRet;
}

INT32S
vid_dec_stop(
	void
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR)
	{
		RETURN(STATUS_FAIL);
	}

	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAY)
	{
		vid_dec_clear_status(C_VIDEO_DECODE_PLAY);
		POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_STOP, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);
	}
	display_task_msg_send(MSG_DISPLAY_TASK_PLAY_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
Return:
	return nRet;
}

INT32S
vid_dec_pause(
	void
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR)
		RETURN(STATUS_FAIL);

	if((vid_dec_get_status() & C_VIDEO_DECODE_PLAY) == 0)
		RETURN(STATUS_FAIL);

	if(!(vid_dec_get_status() & C_VIDEO_DECODE_PAUSE))
	{
		POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PAUSE, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);
		vid_dec_set_status(C_VIDEO_DECODE_PAUSE);
	}

Return:
	return nRet;
}

INT32S
vid_dec_resume(
	void
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR)
		RETURN(STATUS_FAIL);

	if((vid_dec_get_status() & C_VIDEO_DECODE_PLAY) == 0)
		RETURN(STATUS_FAIL);

	if(vid_dec_get_status() & C_VIDEO_DECODE_PAUSE)
	{
		vid_dec_clear_status(C_VIDEO_DECODE_PAUSE);
		POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_RESUME, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);
	}

Return:
	return nRet;
}

INT32S
vid_dec_end_callback(
	INT32U bflag
)
{
	osStatus err = osOK;

	if(bflag == C_VIDEO_DECODE_PLAYING) {
		//DBG_PRINT("%s>>VIDEO.\r\n", __FUNCTION__);
		if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING) {
			vid_dec_set_video_flag(FALSE);
			vid_dec_clear_status(C_VIDEO_DECODE_PLAYING);
		}
		vid_dec_stop_timer();
		err = msgQSend(vid_dec_state_q, MSG_VIDEO_DECODE_VID_END, NULL, 0, MSG_PRI_NORMAL);
	}

	if(bflag == C_AUDIO_DECODE_PLAYING) {
		//DBG_PRINT("%s>>AUDIO.\r\n", __FUNCTION__);
		if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) {
			vid_dec_set_audio_flag(FALSE);
			vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
		}
		err = msgQSend(vid_dec_state_q, MSG_VIDEO_DECODE_AUD_END, NULL, 0, MSG_PRI_NORMAL);
	}

	if(err != osOK) {
		ERR_PRINT("PostFail\r\n");
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

INT32S
vid_dec_get_total_samples(
	void
)
{
	return p_vid_dec_para->video_total_sample;
}

INT32S
vid_dec_get_total_time(
	void
)
{
	return p_vid_dec_para->video_total_time;
}

INT32S
vid_dec_get_current_time(
	void
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);
#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
    if(ap_browse_get_curr_file_type() == TK_IMAGE_TYPE_WAV)
    {
        nRet = get_audio_curr_time();
        return nRet;
    }
#endif
	if(p_vid_dec_para->video_cur_time+1 >= p_vid_dec_para->video_total_time)
	{
		RETURN(STATUS_FAIL);
	}

	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER)
	{
		POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_GET_CUR_TIME, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);
	}

Return:

    nRet =  p_vid_dec_para->video_cur_time;
    return nRet;
}

INT32S
vid_dec_set_play_time(
	INT32U second
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR)
	{
		RETURN(STATUS_FAIL);
	}

	if((vid_dec_get_status() & C_VIDEO_DECODE_PARSER) == 0)
	{
		RETURN(STATUS_FAIL);
	}

	if(second > p_vid_dec_para->video_total_time)
	{
		RETURN(STATUS_FAIL);
	}

	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAY)
	{
		// stop play
		vid_dec_clear_status(C_VIDEO_DECODE_PLAY);
		POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_STOP, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);

		// set seek
		p_vid_dec_para->video_cur_time = second;
		POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_SET_SEEK, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);

		// start play
		if(vid_dec_get_audio_flag() || vid_dec_get_video_flag())
		{
			POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_SET_SEEK_PLAY, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);
			vid_dec_set_status(C_VIDEO_DECODE_PLAY);
		}
		else
		if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER)
		{
			vid_dec_clear_status(C_VIDEO_DECODE_PARSER);
			POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PARSER_HEADER_STOP, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);
		}
	}
	else if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER)
	{
		POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_SET_SEEK, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);
	}
	else
	{
		RETURN(STATUS_FAIL);
	}

	nRet = p_vid_dec_para->video_cur_time;
Return:

	return nRet;
}

// play speed = speed / 0x10000, normal speed = 0x10000
INT32S
vid_dec_set_play_speed(
	INT32U speed
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR)
	{
		RETURN(STATUS_FAIL);
	}

	if((vid_dec_get_status() & C_VIDEO_DECODE_PLAY) == 0)
	{
		RETURN(STATUS_FAIL);
	}

	if(speed == 0)
	{
		RETURN(STATUS_FAIL);
	}

	p_vid_dec_para->play_speed = speed;
	POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_SET_PLAY_SPEED, vid_dec_state_ack_m, 300, MSG_PRI_NORMAL);

Return:
	return nRet;
}

INT32S
vid_dec_set_reverse_play(
	INT32U reverse_play_flag
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR)
	{
		RETURN(STATUS_FAIL);
	}

	if((vid_dec_get_status() & C_VIDEO_DECODE_PLAY) == 0)
	{
		RETURN(STATUS_FAIL);
	}

	if(vid_dec_get_video_flag() == 0)
	{
		RETURN(STATUS_FAIL);
	}

	p_vid_dec_para->reverse_play_set = reverse_play_flag;
	POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_SET_REVERSE, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);

Return:
	return nRet;
}

INT32S
vid_dec_nth_frame(
	INT32U nth
)
{
	INT32S	nRet =STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR)
	{
		RETURN(STATUS_FAIL);
	}

	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAY)
	{
		RETURN(STATUS_FAIL);
	}

	if(vid_dec_get_video_flag() == 0)
	{
		RETURN(STATUS_FAIL);
	}

	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER)
	{
		vid_dec_set_status(C_VIDEO_DECODE_PARSER_NTH);
		POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_NTH_FRAME, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);
	}

Return:
	vid_dec_clear_status(C_VIDEO_DECODE_PARSER_NTH);
	return nRet;
}

static INT32S
video_decode_state_task_init(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	vid_dec_state_q = msgQCreate(BROWSE_DEC_STATE_TASK_Q_MAX,BROWSE_DEC_STATE_TASK_Q_MAX,0);
	if(vid_dec_state_q == NULL)
	{
		RETURN(STATUS_FAIL);
	}

	vid_dec_state_ack_m = msgQCreate(BROWSE_DEC_STATE_TASK_ACK_MAX,BROWSE_DEC_STATE_TASK_ACK_MAX,0);
	if(vid_dec_state_ack_m == NULL)
	{
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}

static INT32S
video_decode_state_task_uninit(
	void
)
{
	if(vid_dec_state_q)
	{
		msgQDelete(vid_dec_state_q);
		vid_dec_state_q = NULL;
	}

	if(vid_dec_state_ack_m)
	{
		msgQDelete(vid_dec_state_ack_m);
		vid_dec_state_ack_m = NULL;
	}

	if(p_vid_dec_para->file_handle > -1)
	{
		close(p_vid_dec_para->file_handle);
		p_vid_dec_para->file_handle = -1;
	}

	p_bitmap_info = NULL;
	p_wave_info = NULL;

	return STATUS_OK;
}

static void
return_ack(
	INT32S nRet
)
{
	INT32U ret_ack;

	if(nRet == STATUS_OK)
	{
		ret_ack = C_ACK_SUCCESS;
	}
	else
	{
		ret_ack = C_ACK_FAIL;
	}

	//DBG_PRINT("==Run:%s==ret_ack:%d==\r\n",__FUNCTION__,ret_ack);
	msgQSend(vid_dec_state_ack_m, ret_ack, NULL, 0, MSG_PRI_NORMAL);
}

void
vid_dec_parser_end(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(p_vid_dec_para->media_handle)
	{
		MultiMediaParser_Delete(p_vid_dec_para->media_handle);
		p_vid_dec_para->media_handle = NULL;
	}

	p_bitmap_info = NULL;
	p_wave_info = NULL;
}

INT32S
video_parser_error_handle(
	INT32S error_id
)
{

	DBG_PRINT("ParserErrID =0x%x\r\n", error_id);
	vid_dec_set_status(C_VIDEO_DECODE_ERR);

	/* error happen close video parser automatic */
	// stop video
	/*if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING)
	{
		vid_dec_clear_status(C_VIDEO_DECODE_PLAYING);
		vid_dec_stop_timer();

		video_decode_task_stop();
	}*/

	// wait audio stop
	if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING)
	{
		while(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING)
		{
			vTaskDelay(10);
		}
	}

	// stop parser and free memory
	msgQSend(vid_dec_state_q, MSG_VIDEO_DECODE_ERROR, NULL, 0, MSG_PRI_NORMAL);

	return STATUS_OK;
}

INT32S
browse_parser_header_start(
	void
)
{
	INT32S error_id;
	INT8U i;

	//clear count
	p_vid_dec_para->n = 0;
	p_vid_dec_para->ta = 0;
	p_vid_dec_para->tv = 0;
	p_vid_dec_para->Tv = 0;
	p_vid_dec_para->fail_cnt = 0;
	p_vid_dec_para->pend_cnt = 0;
	p_vid_dec_para->post_cnt = 0;
	p_vid_dec_para->video_flag = 0;
	p_vid_dec_para->user_define_flag = 0;
	p_vid_dec_para->video_format = 0;
	p_vid_dec_para->deblock_flag = 0;
	p_vid_dec_para->play_speed = 0x10000;
	p_vid_dec_para->reverse_play_set = 0;
	p_vid_dec_para->reverse_play_flag = 0;
	p_vid_dec_para->ff_audio_flag = 0;
	p_vid_dec_para->audio_flag = 0;
	p_vid_dec_para->scaler_flag = 0;
	p_vid_dec_para->image_output_width = 0;
	p_vid_dec_para->image_output_height = 0;
	p_vid_dec_para->buffer_output_width = 0;
	p_vid_dec_para->buffer_output_height = 0;
	p_vid_dec_para->video_decode_addr = 0;
	p_vid_dec_para->scaler_output_addr = 0;

   	for(i=0; i<AUDIO_FRAME_NO; i++) {
		p_vid_dec_para->audio_decode_addr[i] = 0;
	}
	for(i=0; i<DEBLOCK_FRAME_NO; i++) {
		p_vid_dec_para->deblock_addr[i] = 0;
	}

	p_vid_dec_para->media_handle = MultiMediaParser_Create(
			p_vid_dec_para->file_handle,
			p_vid_dec_para->FileSize,
			PARSER_AUD_BITSTREAM_SIZE,
			PARSER_VID_BITSTREAM_SIZE,
			/*
				vid buf 太多 ,對快進快退會有影響 , 影響如下
				每個vidbuf 都會記錄這個frame的Duration,當從快進,切回normal時
				過多的Vid buf(都是I FRAME) ,會導致快速PLAY
			*/
			3,
			PRIORITY_VIDEO_PASER,
			p_vid_dec_para->FileType==FILE_TYPE_AVI ? AviParser_GetFcnTab() : QtffParser_GetFcnTab(),
			video_parser_error_handle,
			&error_id
		);

	if((error_id & 0x80000000) || !p_vid_dec_para->media_handle)
	{
		ERR_PRINT("fail to MultiMediaParser_Create=>error_id:0x%x\r\n",error_id);
		return STATUS_FAIL;
	}

	p_vid_dec_para->VidTickRate = MultiMediaParser_GetVidTickRate(p_vid_dec_para->media_handle);
	p_vid_dec_para->video_total_sample = MultiMediaParser_GetVidTotalSamples(p_vid_dec_para->media_handle);
	p_vid_dec_para->video_total_time = MultiMediaParser_GetVidTrackDur(p_vid_dec_para->media_handle) >> 8;
	p_vid_dec_para->video_cur_time = 0;

	p_bitmap_info = MultiMediaParser_GetVidInfo(p_vid_dec_para->media_handle, &g_bitmap_info_len);
	p_wave_info = MultiMediaParser_GetAudInfo(p_vid_dec_para->media_handle, &g_wave_info_len);

	DBG_PRINT("AudFormat = 0x%x\r\n", p_wave_info->wFormatTag);
	switch(p_wave_info->wFormatTag)
	{
		case WAVE_FORMAT_PCM:
		case WAVE_FORMAT_MULAW:
		case WAVE_FORMAT_ALAW:
		case WAVE_FORMAT_ADPCM:
		case WAVE_FORMAT_DVI_ADPCM:
			vid_dec_set_audio_flag(TRUE);
			p_vid_dec_para->aud_frame_size = AUDIO_PCM_BUFFER_SIZE;

			#if UP_SAMPLE_EN
			p_vid_dec_para->aud_frame_size <<= 1;
			#endif

			break;

		default:
			DBG_PRINT("NotSupportAudFormat!!!\r\n");
			vid_dec_set_audio_flag(FALSE);
			p_wave_info = NULL;
			break;
	}

	#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	#if !C_APP_PLAYBACK_WITH_SOUND
	if((state_wifi_status_get() & FLAG_CONNECT) &&
		(state_wifi_status_get() & FLAG_OFF_DISPLAY)
	)
	{
		vid_dec_set_audio_flag(FALSE);
		p_wave_info = NULL;
	}
	#endif
	#endif

	p_vid_dec_para->video_format = vid_dec_get_video_format(p_bitmap_info->biCompression);
	DBG_PRINT("VidFormat = 0x%x\r\n", p_vid_dec_para->video_format);
	switch(p_vid_dec_para->video_format)
	{
		case C_MJPG_FORMAT:
		case C_H264_FORMAT:
			vid_dec_set_video_flag(TRUE);
			break;

		default:
			vid_dec_set_video_flag(FALSE);
			p_bitmap_info = NULL;
			break;
	}

	// no audio and no video
	if(vid_dec_get_audio_flag() == FALSE && vid_dec_get_video_flag() == FALSE)
	{
		vid_dec_parser_end();
		vid_dec_clear_status(C_VIDEO_DECODE_ALL);
		return STATUS_FAIL;
	}

	if(p_wave_info)
	{
		p_vid_dec_para->n = p_wave_info->nSamplesPerSec * 2 / TIME_BASE_TICK_RATE; /* 此變數是用來SERVICE , 每秒產生幾次vid_dec_timer_isr */
		p_vid_dec_para->tick_time = (INT64S) p_vid_dec_para->VidTickRate * p_vid_dec_para->n;
		p_vid_dec_para->time_range = p_vid_dec_para->tick_time * TIME_BASE_TICK_RATE >> 2;	// about 256ms
	}
	else
	{
		p_vid_dec_para->n = 1;
		p_vid_dec_para->tick_time = (INT64S) p_vid_dec_para->VidTickRate * p_vid_dec_para->n;
		p_vid_dec_para->time_range = p_vid_dec_para->tick_time * TIME_BASE_TICK_RATE >> 2;	// about 256ms
	}
	#if C_APP_PLAYBACK_WITH_SOUND
	playback_ms_unit = (p_vid_dec_para->tick_time*TIME_BASE_TICK_RATE)/1000;
	#endif
	p_vid_dec_para->tick_time2 = p_vid_dec_para->tick_time;

	return STATUS_OK;
}

INT32S
browse_parser_header_stop(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	vid_dec_parser_end();
	vid_dec_memory_free();

   	vid_dec_clear_status(C_VIDEO_DECODE_PARSER);
   	vid_dec_clear_status(C_VIDEO_DECODE_ALL);

	if(vid_dec_state_q)
	{
		msgQFlush(vid_dec_state_q);
	}

	return STATUS_OK;
}

INT32S
browse_decode_start(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_memory_alloc() < 0)
	{
		RETURN(STATUS_FAIL);
	}

	if(vid_dec_get_video_flag())
	{
		if(video_decode_task_start() < 0)
		{
			DBG_PRINT("video_decode_task_start() fail\r\n");
			RETURN(STATUS_FAIL);
		}

		vid_dec_set_status(C_VIDEO_DECODE_PLAYING);
	}

	if(vid_dec_get_audio_flag())
	{
		if(audio_decode_task_start() < 0)
		{
			DBG_PRINT("audio_decode_task_start() fail\r\n");
			RETURN(STATUS_FAIL);
		}

		vid_dec_set_status(C_AUDIO_DECODE_PLAYING);
	}

	if(vid_dec_get_video_flag())
	{
		vid_dec_start_timer();
	}

	return STATUS_OK;

Return:

	vid_dec_memory_free();

	return nRet;
}

INT32S
browse_decode_stop(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING)
	{
		vid_dec_clear_status(C_VIDEO_DECODE_PLAYING);
		vid_dec_stop_timer();

		if(video_decode_task_stop() < 0)
		{
			ERR_PRINT("fail to video_decode_task_stop \r\n");
		}
	}

	if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING)
	{
		vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
		if(audio_decode_task_stop() < 0)
		{
			ERR_PRINT("fail to audio_decode_task_stop \r\n");
		}
	}

	return STATUS_OK;
}

INT32S
browse_decode_pause(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING)
	{
		vid_dec_stop_timer();
	}

	if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING)
	{
		audio_decode_task_pause();
	}

   	vid_dec_set_status(C_VIDEO_DECODE_PAUSE);

	return STATUS_OK;
}

INT32S
browse_decode_resume(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING)
	{
		vid_dec_start_timer();
	}

	if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING)
	{
		audio_decode_task_resume();
	}

	vid_dec_clear_status(C_VIDEO_DECODE_PAUSE);

	return STATUS_OK;
}

INT32S
browse_curr_time_get(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(p_vid_dec_para->media_handle == 0)
	{
		return STATUS_FAIL;
	}

	p_vid_dec_para->video_cur_time = MultiMediaParser_GetVidCurTime(p_vid_dec_para->media_handle) >> 8;

	return STATUS_OK;
}

INT32S
browse_seek_sec_set(
	void
)
{
	long seek_sec = p_vid_dec_para->video_cur_time;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(p_vid_dec_para->media_handle == 0)
	{
		return STATUS_FAIL;
	}

	seek_sec <<= 8;
	if(MultiMediaParser_SeekTo(p_vid_dec_para->media_handle, seek_sec) < 0)
	{
		return STATUS_FAIL;
	}

	if(MultiMediaParser_IsEOA(p_vid_dec_para->media_handle))
	{
		vid_dec_set_audio_flag(FALSE);
		vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
	}

	if(MultiMediaParser_IsEOV(p_vid_dec_para->media_handle))
	{
		vid_dec_set_video_flag(FALSE);
		vid_dec_clear_status(C_VIDEO_DECODE_PLAYING);
	}

	vid_dec_stop_timer();
	vid_dec_start_timer();

	return STATUS_OK;
}

INT32S
browse_seek_play_set(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_video_flag())
	{
		if(video_decode_task_restart() < 0)
		{
			DBG_PRINT("video_decode_task_restart() fail\r\n");
			return STATUS_FAIL;
		}

		vid_dec_set_status(C_VIDEO_DECODE_PLAYING);
	}

	if(vid_dec_get_audio_flag())
	{
		if(audio_decode_task_start() < 0)
		{
			DBG_PRINT("audio_decode_task_start() fail\r\n");
			return STATUS_FAIL;
		}

		vid_dec_set_status(C_AUDIO_DECODE_PLAYING);
	}

	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING)
	{
		vid_dec_start_timer();
	}

	return STATUS_OK;
}

static INT32S
play_speed_set(
	void
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("Speed=0x%x\r\n",p_vid_dec_para->play_speed);

	if(p_vid_dec_para->play_speed == 0x10000 && p_vid_dec_para->reverse_play_flag == 0)
	{
		//--快進切到正常速度 , 有很少機率 , 畫面會解錯 , 原因unknown
		//--因此捨棄掉快進的最後一禎 , 看起來可解
		raw_data_addr = 0;
		if(p_vid_dec_para->ff_audio_flag)
		{
			p_vid_dec_para->ff_audio_flag = FALSE;
			vid_dec_set_audio_flag(TRUE);
			MultiMediaParser_EnableAudStreaming(p_vid_dec_para->media_handle, 1);
			if(audio_decode_task_start() < 0)
			{
				RETURN(STATUS_FAIL);
			}

			vid_dec_set_status(C_AUDIO_DECODE_PLAYING);
		}
	}
	else
	{
		/* no video, only audio, so skip set play speed */
		if(vid_dec_get_video_flag() == 0)
		{
			DBG_PRINT("NoVidData.\r\n");
			RETURN(STATUS_FAIL);
		}

		if(vid_dec_get_audio_flag())
		{
			p_vid_dec_para->ff_audio_flag = TRUE;
			vid_dec_set_audio_flag(FALSE);
			if(audio_decode_task_stop() < 0)
			{
				RETURN(STATUS_FAIL);
			}

			vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
			MultiMediaParser_EnableAudStreaming(p_vid_dec_para->media_handle, 0);
		}
	}

	osENTER_CRITICAL();
	if( (p_vid_dec_para->play_speed>>16) == 1)
	{
		MultiMediaParser_SetFrameDropLevel(p_vid_dec_para->media_handle, 0);
	}
	else
	{
		MultiMediaParser_SetFrameDropLevel(p_vid_dec_para->media_handle, ((p_vid_dec_para->play_speed>>16)-1) );

	}
	p_vid_dec_para->tick_time2 = (p_vid_dec_para->tick_time * p_vid_dec_para->play_speed) >> 16;
	p_vid_dec_para->fail_cnt = 0;
	p_vid_dec_para->ta = 0;
	p_vid_dec_para->tv = 0;
	p_vid_dec_para->Tv = 0;
	osEXIT_CRITICAL();

	vid_dec_stop_timer();
	vid_dec_start_timer();
Return:
	return nRet;
}

INT32S
browse_play_speed_set(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(p_vid_dec_para->media_handle == 0)
	{
		return STATUS_FAIL;
	}

	if(play_speed_set() < 0)
	{
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

static INT32S
reserved_play_set(
	void
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(p_vid_dec_para->reverse_play_flag != p_vid_dec_para->reverse_play_set)
	{
		DBG_PRINT("Speed reverse\r\n");
		Speed_reverse_flag = 1;
		vid_dec_stop_timer();
	//---當影片是mjpg時,在快退 停止錄影時, 會發生 tv>TV的情況(可能是2k解析度,解比較慢)
	//---再做一次deocde , 並post_cnt++,當timer restart,因為 並post_cnt!=pend_cnt
	//---=而無法decode,因此將 raw_data_addr =0 , 避再次decode
		if(p_vid_dec_para->video_format==C_MJPG_FORMAT)
			raw_data_addr = 0;
		if(video_decode_task_stop() < 0)
		{
			RETURN(STATUS_FAIL);
		}
		//--快進切到快退速度 , 很容易畫面會解錯 , 原因unknown,而且之後的I禎 , h264 都解不出來 , stream size =0
		//--因此捨棄掉正常速度的最後一禎(避免快退解到p禎) , 看起來可解
		raw_data_addr = 0;
		p_vid_dec_para->reverse_play_flag = p_vid_dec_para->reverse_play_set;
		if(MultiMediaParser_SetReversePlay(p_vid_dec_para->media_handle, p_vid_dec_para->reverse_play_set) < 0)
		{
			RETURN(STATUS_FAIL);
		}

		if(p_vid_dec_para->play_speed == 0x10000 && p_vid_dec_para->reverse_play_flag == 0)
		{
			osENTER_CRITICAL();
			MultiMediaParser_SetFrameDropLevel(p_vid_dec_para->media_handle, 0);
			p_vid_dec_para->fail_cnt = 0;
			p_vid_dec_para->ta = 0;
			p_vid_dec_para->tv = 0;
			p_vid_dec_para->Tv = 0;
			osEXIT_CRITICAL();
			if(p_vid_dec_para->ff_audio_flag)
			{
				p_vid_dec_para->ff_audio_flag = FALSE;
				vid_dec_set_audio_flag(TRUE);
				MultiMediaParser_EnableAudStreaming(p_vid_dec_para->media_handle, 1);
				if(video_decode_task_restart() < 0)
				{
					RETURN(STATUS_FAIL);
				}

				vid_dec_start_timer();
				if(audio_decode_task_start() < 0)
				{
					RETURN(STATUS_FAIL);
				}

				vid_dec_set_status(C_AUDIO_DECODE_PLAYING);
			}
			else
			{
				if(video_decode_task_restart() < 0)
				{
					RETURN(STATUS_FAIL);
				}

				vid_dec_start_timer();
			}
		}
		else
		{
			if(vid_dec_get_audio_flag())
			{
				p_vid_dec_para->ff_audio_flag = TRUE;
				vid_dec_set_audio_flag(FALSE);
				if(audio_decode_task_stop() < 0)
				{
					RETURN(STATUS_FAIL);
				}

				vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
				MultiMediaParser_EnableAudStreaming(p_vid_dec_para->media_handle, 0);
			}

			osENTER_CRITICAL();
			MultiMediaParser_SetFrameDropLevel(p_vid_dec_para->media_handle, 0);
			p_vid_dec_para->fail_cnt = 0;
			p_vid_dec_para->ta = 0;
			p_vid_dec_para->tv = 0;
			p_vid_dec_para->Tv = 0;
			osEXIT_CRITICAL();
			if(video_decode_task_restart() < 0)
			{
				RETURN(STATUS_FAIL);
			}

			vid_dec_start_timer();
		}
	}

Return:
	return nRet;
}

INT32S
browse_reverse_set(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(p_vid_dec_para->media_handle == 0)
	{
		return STATUS_FAIL;
	}

	if(reserved_play_set() < 0)
	{
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

INT32S
browse_Nth_frame_set(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_memory_alloc() < 0)
	{
		vid_dec_memory_free();
		return STATUS_FAIL;
	}

	if(video_decode_task_nth_frame() < 0)
	{
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

void
video_decode_error_callback(
	void
)
{
	DBG_PRINT("video_decode_error_callback\r\n");
	storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
}

INT32S
browse_force_stop(
	INT32U msg_id
)
{
	INT32S error_id = 0;
	INT16U wait = 0;

	//DBG_PRINT("==Run:%s==msg_id:0x%x\r\n",__FUNCTION__,msg_id);

	if(msg_id == MSG_VIDEO_DECODE_FORCE_STOP)
	{
		error_id = 0;
	}
	else
	if(msg_id == MSG_VIDEO_DECODE_ERROR)
	{
		error_id = 0x80000000;
	}

	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING)
	{
		video_decode_task_stop();
	}

	if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING)
	{
		audio_decode_task_stop();
	}

	vid_dec_clear_status(C_VIDEO_DECODE_ALL);

	vid_dec_parser_end();

    while(1){
        if(!p_vid_dec_para->video_decode_cb_handle || circular_buf_full((cbuf_handle_t)p_vid_dec_para->video_decode_cb_handle)){
            break;
        }
        osDelay(1);
        if(++wait > 2000){
            ERR_PRINT("\r\n");
            break;
        }
    }

	vid_dec_memory_free();
	if(vid_dec_state_q)
	{
		msgQFlush(vid_dec_state_q);
	}

	if(error_id)
	{
		video_decode_error_callback();
	}
	else
	{
		app_msg_send(MSG_APQ_MJPEG_DECODE_END, NULL, 0, MSG_PRI_NORMAL);
	}

	return STATUS_OK;
}

INT32S
browse_state_task_exit(
	INT8U* pExit_flag
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	*pExit_flag = 0;

	return STATUS_OK;
}

INT32S
browse_video_end(
	void
)
{
	INT32S nRet = STATUS_OK;
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(((vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) == 0) &&
		(vid_dec_get_audio_flag() == 0)) {

		browse_force_stop(MSG_VIDEO_DECODE_FORCE_STOP);
	}

//Return:
	return nRet;
}

INT32S
browse_audio_end(
	void
)
{
	INT32S nRet = STATUS_OK;
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(((vid_dec_get_status() & C_VIDEO_DECODE_PLAYING) == 0) &&
		(vid_dec_get_video_flag() == 0)) {
		browse_force_stop(MSG_VIDEO_DECODE_FORCE_STOP);
	}

//Return:
	return nRet;
}

static void
browse_dec_state_task_entry(
	void const *args
)
{
	INT32U msg_id;
	INT8U  flag_task_exit = 1;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	video_decode_state_task_init();

	while(flag_task_exit)
	{
		if(msgQReceive(vid_dec_state_q, &msg_id, NULL, 0) == STATUS_FAIL)
		{
			continue;
		}

		//DBG_PRINT("==Run:%s==msg_id:0x%x==\r\n",__FUNCTION__,msg_id);

		switch(msg_id)
		{
			case MSG_VIDEO_DECODE_PARSER_HEADER_START:
				return_ack(browse_parser_header_start());
			break;
			case MSG_VIDEO_DECODE_PARSER_HEADER_STOP:
				return_ack(browse_parser_header_stop());
			break;
			case MSG_VIDEO_DECODE_START:
				return_ack(browse_decode_start());
			break;
			case MSG_VIDEO_DECODE_STOP:
				return_ack(browse_decode_stop());
			break;
			case MSG_VIDEO_DECODE_PAUSE:
				return_ack(browse_decode_pause());
			break;
			case MSG_VIDEO_DECODE_RESUME:
				return_ack(browse_decode_resume());
			break;
			case MSG_VIDEO_DECODE_GET_CUR_TIME:
				return_ack(browse_curr_time_get());
			break;
			case MSG_VIDEO_DECODE_SET_SEEK:
				return_ack(browse_seek_sec_set());
			break;
			case MSG_VIDEO_DECODE_SET_SEEK_PLAY:
				return_ack(browse_seek_play_set());
			break;
			case MSG_VIDEO_DECODE_SET_PLAY_SPEED:
				return_ack(browse_play_speed_set());
			break;
			case MSG_VIDEO_DECODE_SET_REVERSE:
				return_ack(browse_reverse_set());
			break;
			case MSG_VIDEO_DECODE_NTH_FRAME:
				return_ack(browse_Nth_frame_set());
			break;
			case MSG_VIDEO_DECODE_ERROR:
			case MSG_VIDEO_DECODE_FORCE_STOP:
				return_ack(browse_force_stop(msg_id));
			break;
			case MSG_VIDEO_DECODE_EXIT:
				return_ack(browse_state_task_exit(&flag_task_exit));
			break;
			case MSG_VIDEO_DECODE_VID_END:
				browse_video_end();
			break;
			case MSG_VIDEO_DECODE_AUD_END:
				browse_audio_end();
			break;

		}
	}

	//DBG_PRINT("Exit %s\r\n",__FUNCTION__);

	video_decode_state_task_uninit();

	osThreadTerminate(NULL);
}

INT32S
video_decode_state_task_create(
	void
)
{
	INT32S nRet = STATUS_OK;
	osThreadId id;
	osThreadDef_t browse_dec_state_task = {"browse_dec_state_task", browse_dec_state_task_entry, PRIORITY_BROWSE_DEC_STATE, 1, BROWSE_DEC_STATE_TASK_STACK_SIZE};

	gp_memset((INT8S *) p_vid_dec_para, 0, sizeof(video_decode_para_struct));

	id = osThreadCreate(&browse_dec_state_task, NULL);
	if(id == 0)
	{
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}

INT32S
video_decode_state_task_del(
	void
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAY)
	{
		vid_dec_clear_status(C_VIDEO_DECODE_PLAY);
		POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_STOP, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);
	}

	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER)
	{
		vid_dec_clear_status(C_VIDEO_DECODE_PARSER);
		POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PARSER_HEADER_STOP, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);
	}

	POST_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_EXIT, vid_dec_state_ack_m, 5000, MSG_PRI_NORMAL);

Return:
	return nRet;
}

INT32U wifi_playback_first_tick_get(int src_id)
{
	INT32U tick = 0;

	if (vid_dec_get_status()&C_VIDEO_DECODE_PLAYING)
	{
		if (p_wave_info)
		{
			tick = (INT32U)(p_wave_info->nSamplesPerSec * 2)/playback_ms_unit + 500;
		}
		else
		{
			tick = (INT32U)TIME_BASE_TICK_RATE/playback_ms_unit + 500;
		}
	}

	//DBG_PRINT("first tick = %d\r\n",tick);
	if (src_id == 0)	// video
	{
		tick = tick * 90;
		DBG_PRINT("V:%d\r\n",tick);
	}
	else			// audio
	{
		tick = tick * 8;
		DBG_PRINT("A:%d\r\n",tick);
	}

	return tick;
}

extern INT16U present_state;
INT32U wifi_playback_tick_get(INT32S src_id)
{
	static INT32U tick_bak = 0;
	static INT32U tick_inc = 0;
	INT32U tick = 0;
	//unsigned int status = vid_dec_get_status();

	if(present_state == STATE_BROWSE)
	{
		tick = (INT32U)(p_vid_dec_para->Tv/playback_ms_unit + 500);			// unit: ms
		// DBG_PRINT("tick1=%d\r\n",tick);
		if (src_id==0) // video
		{
			//tick = tick * 90;
			//DBG_PRINT("v:%d\r\n",tick);
		}
		else		// audio
		{
			tick = tick * 8;
			//DBG_PRINT("a:%d\r\n",tick);
		}
	}
	else
	{
		 tick = xTaskGetTickCount()* 90;										// unit:1ms
		 if ( tick == tick_bak )
		 {
			tick_inc++;
		 	tick += tick_inc;
			DBG_PRINT("collision\r\n");
		 }
		 else
		 {
		 	tick_inc = 0;
		 }
		 tick_bak = tick;
		 //DBG_PRINT("tick=%d\r\n",tick);
	}

	return tick;
}

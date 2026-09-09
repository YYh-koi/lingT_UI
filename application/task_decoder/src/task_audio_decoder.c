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

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define BROWSE_AUD_DEC_STACK_SIZE		2048
#define BROWSE_AUD_DEC_TASK_Q_MAX		5
#define BROWSE_AUD_DEC_TASK_ACK_MAX		1

enum
{
	AUD_DATA_OK = STATUS_OK,
	AUD_READ_END,
	AUD_DEC_END
};

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
#if C_APP_PLAYBACK_WITH_SOUND
extern INT32S audio_send(INT32U addr, INT32U size);
#endif
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
/* os task queue */
MSG_Q_ID	aud_dec_q;
MSG_Q_ID	aud_dec_ack_m;

INT32S	buf_RI, buf_WI;
INT32S (*pfun_decode_one_frame)(INT16S*) = NULL;
INT64S	delta_ta[AUDIO_FRAME_NO];
INT32U 	pcm_addr;
INT32S	cwlen;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
INT32S
audio_decode_task_start(
	void
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if((aud_dec_q == NULL) && (aud_dec_ack_m == NULL))
	{
		RETURN(STATUS_FAIL);
	}

	POST_MESSAGE(aud_dec_q, MSG_AUD_DEC_START, aud_dec_ack_m, 5000, MSG_PRI_NORMAL);

	// audio start play
	if(p_vid_dec_para->upsample_flag)
	{
		aud_dec_dac_start(p_vid_dec_para->aud_dec_ch_no, p_vid_dec_para->aud_dec_sr);
	}
	else
	{
		aud_dec_dac_start(p_wave_info->nChannels, p_wave_info->nSamplesPerSec);
	}
Return:
	return nRet;
}

INT32S
audio_decode_task_stop(
	void
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if((aud_dec_q == NULL) && (aud_dec_ack_m == NULL))
	{
		RETURN(STATUS_FAIL);
	}

	POST_MESSAGE(aud_dec_q, MSG_AUD_DEC_STOP, aud_dec_ack_m, 5000, MSG_PRI_NORMAL);

Return:
	return nRet;
}

INT32S
audio_decode_task_pause(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	aud_dec_dac_stop();

	return STATUS_OK;
}

INT32S
audio_decode_task_resume(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(p_vid_dec_para->upsample_flag)
	{
		aud_dec_dac_start(p_vid_dec_para->aud_dec_ch_no, p_vid_dec_para->aud_dec_sr);
	}
	else
	{
		aud_dec_dac_start(p_wave_info->nChannels, p_wave_info->nSamplesPerSec);
	}

	return STATUS_OK;
}

static INT32S
audio_decode_task_init(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	aud_dec_q = msgQCreate(BROWSE_AUD_DEC_TASK_Q_MAX,BROWSE_AUD_DEC_TASK_Q_MAX,0);
	if(aud_dec_q == NULL)
	{
		RETURN(STATUS_FAIL);
	}

	aud_dec_ack_m = msgQCreate(BROWSE_AUD_DEC_TASK_ACK_MAX,BROWSE_AUD_DEC_TASK_ACK_MAX,0);
	if(aud_dec_ack_m == NULL)
	{
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}

static INT32S
audio_decode_task_uninit(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(aud_dec_q)
	{
		msgQDelete(aud_dec_q);
		aud_dec_q = NULL;
	}

	if(aud_dec_ack_m)
	{
		msgQDelete(aud_dec_ack_m);
		aud_dec_ack_m = NULL;
	}

	return STATUS_OK;
}

void
audio_decode_task_end(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	aud_dec_dac_stop();
	drv_l1_dac_dbf_free();
	vid_dec_aud_dec_memory_free();
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
    msgQSend(aud_dec_ack_m, ret_ack, NULL, 0, MSG_PRI_NORMAL);
}

static INT32S
aud_dec_wave_init(
	void
)
{
	INT32S	nRet = STATUS_OK;
	INT32U	WI;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = vid_dec_set_aud_dec_work_mem(WAV_DEC_MEMORY_SIZE);
	if(nRet < 0)
	{
		RETURN(STATUS_FAIL);
	}

	nRet = vid_dec_set_aud_dec_ring_buffer();
	if(nRet < 0)
	{
		RETURN(STATUS_FAIL);
	}

	nRet = wav_dec_init((INT8U *) p_vid_dec_para->work_mem_addr, p_vid_dec_para->ring_buffer_addr);
	if(nRet < 0)
	{
		RETURN(STATUS_FAIL);
	}

	wav_dec_set_ring_buf_size((INT8U *)p_vid_dec_para->work_mem_addr, PARSER_AUD_BITSTREAM_SIZE);
	nRet = wav_dec_set_param((INT8U *) p_vid_dec_para->work_mem_addr, (INT8U *) p_wave_info);
	if(nRet < 0)
	{
		RETURN(STATUS_FAIL);
	}

	WI = MultiMediaParser_GetAudRingWI(p_vid_dec_para->media_handle);
	if(!WI)
	{
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}

static INT32S
aud_dec_wave_one_output(
	void *work_mem,
	INT16S *pcm_buffer_addr,
	INT32S maxlen
)
{
	INT32S pcm_point;
	INT32U RI, WI;
	INT32S nRet, size;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	WI = MultiMediaParser_GetAudRingWI(p_vid_dec_para->media_handle);
	RI = wav_dec_get_ri((INT8U *)p_vid_dec_para->work_mem_addr);

	size = WI - RI;
	if(size == 0) {
		RETURN(0);
	}

	// when pcm format, wavelib need to limite output pcm size
	if(p_wave_info->wFormatTag == WAVE_FORMAT_PCM) {
		if(size < 0) {
			size += PARSER_AUD_BITSTREAM_SIZE;
		}

		if(size > (AUDIO_PCM_BUFFER_SIZE >> 1)) {
			WI = RI + (AUDIO_PCM_BUFFER_SIZE >> 1);
			if(WI >= PARSER_AUD_BITSTREAM_SIZE) {
				WI -= PARSER_AUD_BITSTREAM_SIZE;
			}
		}
	}

	pcm_point = wav_dec_run((INT8U *)p_vid_dec_para->work_mem_addr, (INT16S*)pcm_buffer_addr, WI);
	if(pcm_point < 0) {
		RETURN(pcm_point);
	}

	nRet = pcm_point * wav_dec_get_nChannels((INT8U *)p_vid_dec_para->work_mem_addr);
#if UP_SAMPLE_EN == 1
	if(p_vid_dec_para->upsample_flag) {
		nRet = pcm_point * UpSample_GetChannel(p_vid_dec_para->us_handle);
	}
#endif

Return:
	return nRet;
}

static INT32S
aud_dec_wave_one_frame(
	INT16S* pcm_buffer_addr
)
{
	INT32S nRet;
	INT32U RI;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = aud_dec_wave_one_output(p_vid_dec_para->work_mem_addr, pcm_buffer_addr, WAV_DEC_FRAMESIZE);
	if(nRet <= 0) {
		RETURN(nRet);
	}

Return:
	RI = wav_dec_get_ri((INT8U *)p_vid_dec_para->work_mem_addr);
	MultiMediaParser_RefillAudBuf(p_vid_dec_para->media_handle, RI);
	return nRet;
}

INT32S
browse_aud_data_get(
	void
)
{
	INT32S nRet = STATUS_FAIL;

	pcm_addr = vid_dec_get_next_aud_buffer();
	while(1) {
		cwlen = pfun_decode_one_frame((INT16S*) pcm_addr);
			if(cwlen > 0) {
			// decode success
			break;
		} else if(cwlen < 0) {
			// decode fail
			goto ERR_AUD_DATA_GET;
		} else {
			if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) {
				goto ERR_AUD_DATA_GET;
			}

			if(MultiMediaParser_IsEOA(p_vid_dec_para->media_handle) == 1) {
				DBG_PRINT("AudDataDecEnd\r\n");
				nRet = AUD_DEC_END;
				goto ERR_AUD_DATA_GET;
			}

			if(MultiMediaParser_IsEOA(p_vid_dec_para->media_handle) == 2) {
				DBG_PRINT("AudDataReadEnd\r\n");
				nRet = AUD_READ_END;
				goto ERR_AUD_DATA_GET;
			}

			osDelay(3);
		}
	}

	/*
		x2 because DAC output rate would be twice of sample rate if mono audio
		mp3 is hardware decode, force to 2 channel
	*/
	if(p_wave_info->wFormatTag == WAVE_FORMAT_MPEGLAYER3) {
		delta_ta[buf_WI] = (INT64S)p_vid_dec_para->VidTickRate * cwlen;
	} else if(p_wave_info->nChannels == 1) {
		delta_ta[buf_WI] = (INT64S) p_vid_dec_para->VidTickRate * (cwlen << 1);
	} else {
		delta_ta[buf_WI] = (INT64S) p_vid_dec_para->VidTickRate * cwlen;
	}

	if(p_vid_dec_para->upsample_flag) {
		delta_ta[buf_WI] >>= 1;
	}

	buf_WI++;
	if(buf_WI >= AUDIO_FRAME_NO) {
		buf_WI -= AUDIO_FRAME_NO;
	}

	return STATUS_OK;

ERR_AUD_DATA_GET:

	return nRet;
}

INT32S
browse_aud_dma_done(
	void
)
{
	INT32S nRet;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	//set buffer
	drv_l1_dac_cha_dbf_set((INT16S *)pcm_addr, cwlen);

    #if C_APP_PLAYBACK_WITH_SOUND
    if((state_wifi_status_get() & FLAG_CONNECT) &&
		(state_wifi_status_get() & FLAG_OFF_DISPLAY)
	)
    {
        audio_send(pcm_addr, cwlen);
    }
    #endif
	// increase audio time
	osENTER_CRITICAL();
	p_vid_dec_para->ta += delta_ta[buf_RI];
	osEXIT_CRITICAL();

	buf_RI++;
	if(buf_RI >= AUDIO_FRAME_NO)
	{
		buf_RI -= AUDIO_FRAME_NO;
	}

	nRet = browse_aud_data_get();
	if(nRet == STATUS_OK)
	{
		return STATUS_OK;
	}

	/*
	 Return: STATUS_FAIL,AUD_DEC_END,AUD_READ_END
	 Fail to get audio data or detect end of file, code wait dma done and stop playing
	*/
	while((drv_l1_dac_dbf_status_get() == 1) || (drv_l1_dac_dma_status_get() == 1))
	{
		osDelay(1);
		/*
			audio timer is stopped in pause mode,
			break out of while loops after checking C_VIDEO_DECODE_PAUSE is true
		*/
		if(vid_dec_get_status() & C_VIDEO_DECODE_PAUSE)
		{
			break;
		}
	}

	if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING)
	{
		audio_decode_task_end();
		aud_dec_ramp_down_handle(p_wave_info->nChannels);
		msgQFlush(aud_dec_q);
		vid_dec_end_callback(C_AUDIO_DECODE_PLAYING);

		// Turn off speaker
		ap_peripheral_speaker_ctrl(0);
	}
	return STATUS_OK;
}

void
browse_aud_dma_done_isr(
	INT8U channel,
	INT8U msg
)
{
	INT32S aud_channel;
	osStatus err;

	aud_channel = drv_l1_dac_dbf_dma_channel_get();
	//DBG_PRINT("aud_channel:%d ",aud_channel);

	if(aud_channel < 0)
	{
		return;
	}

	if(channel == aud_channel)
	{
		//DBG_PRINT("done ");

		err = msgQSend(aud_dec_q, MSG_AUD_DEC_DMA_DONE, NULL, 0, MSG_PRI_NORMAL);
		if(err != osOK) {
			ERR_PRINT("fail to msgQSend with aud_dec_q\r\n");
			return;
		}
	}
}

INT32S
browse_aud_dec_start(
	void
)
{
	INT8U 	i;
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	buf_RI = buf_WI = 0;
	p_vid_dec_para->upsample_flag = FALSE;

	// Turn on speaker
	ap_peripheral_speaker_ctrl(1);

	switch(p_wave_info->wFormatTag)
	{
		case WAVE_FORMAT_PCM:
		case WAVE_FORMAT_MULAW:
		case WAVE_FORMAT_ALAW:
		case WAVE_FORMAT_ADPCM:
		case WAVE_FORMAT_DVI_ADPCM:
			if(aud_dec_wave_init() == STATUS_FAIL)
			{
				RETURN(STATUS_FAIL);
			}
			pfun_decode_one_frame = aud_dec_wave_one_frame;
			break;

		default:
			do
			{
				ERR_PRINT("Wrong wave format\r\n");
			}
			while(1);
	}

	for(i = 0; i < AUDIO_FRAME_NO; i++)
	{
		nRet = browse_aud_data_get();
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("Fail to browse_aud_data_get \r\n");
			RETURN(STATUS_FAIL);
		}
		else if(nRet == STATUS_OK)
		{
			if(i == 0)
			{
				drv_l1_dac_cha_dbf_put_callback((INT16S *) pcm_addr, cwlen, browse_aud_dma_done_isr);
			}
			else
			if(i == 1)
			{
				drv_l1_dac_cha_dbf_set((INT16S *) pcm_addr, cwlen);
			}
		}
	}

	return STATUS_OK;

Return:

	// Turn off speaker
	ap_peripheral_speaker_ctrl(0);
	audio_decode_task_end();
	return nRet;
}

INT32S
browse_aud_dec_stop(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	audio_decode_task_end();
	aud_dec_ramp_down_handle(p_wave_info->nChannels);

	if(aud_dec_q)
	{
		msgQFlush(aud_dec_q);
	}

	return STATUS_OK;
}

INT32S
browse_aud_dec_exit(
	INT8U* pExit_flag
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	*pExit_flag = 0;

	return STATUS_OK;
}

static void
browse_aud_dec_task_entry(
	void const *args
)
{
	INT32U msg_id;
	INT8U  flag_task_exit = 1;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	audio_decode_task_init();

	while(flag_task_exit)
	{
		if(msgQReceive(aud_dec_q, &msg_id, NULL, 0) == STATUS_FAIL)
		{
			continue;
		}

		//DBG_PRINT("==Run:%s==msg_id:0x%x==\r\n",__FUNCTION__,msg_id);

		switch(msg_id)
		{
			case MSG_AUD_DEC_DMA_DONE:
				browse_aud_dma_done();
			break;
			case MSG_AUD_DEC_START:
				return_ack(browse_aud_dec_start());
			break;
			case MSG_AUD_DEC_STOP:
				return_ack(browse_aud_dec_stop());
			break;
			case MSG_AUD_DEC_EXIT:
				return_ack(browse_aud_dec_exit(&flag_task_exit));
			break;
		}
	}

	//DBG_PRINT("Exit %s\r\n",__FUNCTION__);

	audio_decode_task_uninit();

	osThreadTerminate(NULL);
}

INT32S
audio_decode_task_create(
	void
)
{
	INT32S nRet = STATUS_OK;
	osThreadId id;
	osThreadDef_t browse_aud_dec_task = {"browse_aud_dec_task", browse_aud_dec_task_entry, PRIORITY_BROWSE_AUD_DEC, 1, BROWSE_AUD_DEC_STACK_SIZE};

	id = osThreadCreate(&browse_aud_dec_task, NULL);
	if(id == 0)
	{
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}

INT32S
audio_decode_task_del(
	void
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if((aud_dec_q == NULL) && (aud_dec_ack_m == NULL))
	{
		RETURN(STATUS_FAIL);
	}

	POST_MESSAGE(aud_dec_q, MSG_AUD_DEC_EXIT, aud_dec_ack_m, 5000, MSG_PRI_NORMAL);

Return:
	return nRet;

	return STATUS_OK;
}

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

typedef struct
{
	INT32S	curr_play_time;
	INT32S	total_play_time;
} ST_AUDIO_PLAY_TIME;

#define AUDIO_DAC_QUEUE_MAX		10
#define AUDIO_DAC_SENDQ_SIZE	MAX_DAC_BUFFERS

// Variables defined in this file
/* Task Q declare */
QueueHandle_t	hAudioDacTaskQ;
QueueHandle_t	aud_send_q;
INT32U last_send_idx;

ST_AUDIO_PLAY_TIME	aud_time;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void audio_dac_task_init(void)
{
	/* Create MsgQueue/MsgBox for TASK */
	hAudioDacTaskQ = xQueueCreate(AUDIO_DAC_QUEUE_MAX, sizeof(INT32S));
	aud_send_q = xQueueCreate(AUDIO_DAC_SENDQ_SIZE, sizeof(INT32U));
}

void audio_dac_task_entry(void const *para)
{
	//INT8U	err = 0;
	INT32S	audio_dac_msg;
	INT8U	pause = 0;
	INT32U	r_idx = 0;
	INT32U	w_idx = 0;
	INT8U	reverse = 0;
	INT32U	total_samples = 0;
	INT32U	pcmlen;

	double	times;

#if AUDIO_PRIOR_DYNAMIC_SW == 1
	INT32U	hiprio_en = 0;
	INT8U	audio_playing = 0;
#endif
	audio_dac_task_init();

	DBG_PRINT("%s ... \r\n", __func__);

	/* Task Star */
	while(1)
	{
		/* Pend task message */
		xQueueReceive(hAudioDacTaskQ, (void*)&audio_dac_msg, portMAX_DELAY);

		switch(audio_dac_msg)
		{
		case C_DMA_STATUS_DONE:
			if(pause != 1)
			{
#if AUDIO_PRIOR_DYNAMIC_SW == 1
				if((uxQueueMessagesWaiting(aud_send_q) < 3) && !hiprio_en && audio_playing)
				{
					osThreadSetPriority(osThreadGetId(), AUD_DEC_HIGH_PRIORITY);
					hiprio_en = 1;

					//DBG_PRINT("hi prio\r\n");
				}
				else
				if(hiprio_en && (uxQueueMessagesWaiting(aud_send_q) > 4))
				{
					osThreadSetPriority(osThreadGetId(), AUD_DEC_PRIORITY);
					hiprio_en = 0;

					//DBG_PRINT("low prio\r\n");
				}

#endif
				if (xQueueReceive(aud_send_q, (void*)&r_idx, 0) == pdTRUE)
				{
					drv_l1_dac_cha_dbf_set(pcm_out[r_idx], pcm_len[r_idx]);
					last_send_idx = r_idx;
				}
				else
				{
					// Turn off speaker
					ap_peripheral_speaker_ctrl(0);
				}
			}

			if(music_para.audio_format == AUDIO_TYPE_WAV)
			{
				pcmlen = pcm_len[w_idx];
				if(channel == 2)
				{
					pcmlen >>= 1;
				}

				if(reverse == 0)
				{
					total_samples += pcmlen;
				}
				else
				{
					if(total_samples >= pcmlen)
					{
						total_samples -= pcmlen;
					}
					else
					{
						total_samples = 0;
					}
				}

				times = (double) total_samples / (double) g_audio_sample_rate;

				if(((INT32U) times > aud_time.curr_play_time) || ((INT32U) times < aud_time.curr_play_time))
				{
					aud_time.curr_play_time = (INT32U) times;

					//					if (start_time_send && aud_time.curr_play_time <=  aud_time.total_play_time) {
					//						msgQSend(ApQ, MSG_AUD_PLAY_TIME_RES, (void *)&aud_time, sizeof(STAudioConfirm), MSG_PRI_NORMAL);
					//					}
				}
			}

			xQueueSend(audio_wq, (void*)&w_idx, 0);
			if(++w_idx == dac_buf_nums)
			{
				w_idx = 0;
			}

			break;

#if AUDIO_PRIOR_DYNAMIC_SW == 1

		case MSG_AUD_LOW_PRIORITY_SET:
			if(hiprio_en)
			{
				osThreadSetPriority(osThreadGetId(), AUD_DEC_PRIORITY);
				hiprio_en = 0;

				//DBG_PRINT("recovery to low prio\r\n");
			}

			break;

		case MSG_AUD_DAC_STOP:
			audio_playing = 0;
			break;
#endif

		case MSG_AUD_DMA_DBF_START:
			xQueueReceive(aud_send_q, (void*)&r_idx, 0);
			drv_l1_dac_cha_dbf_put(pcm_out[r_idx], pcm_len[r_idx], hAudioDacTaskQ);
			xQueueReceive(aud_send_q, (void*)&r_idx, 0);
			drv_l1_dac_cha_dbf_set(pcm_out[r_idx], pcm_len[r_idx]);
			pause = 0;
			drv_l1_dac_audop_speaker_en(1);

#if AUDIO_PRIOR_DYNAMIC_SW == 1
			audio_playing = 1;
#endif

			//DBG_PRINT("dma start\r\n");
			break;

		case MSG_AUD_DMA_DBF_RESTART:
			if(xQueueReceive(aud_send_q, (void*)&r_idx, 0) == pdTRUE)
			{
				drv_l1_dac_cha_dbf_set(pcm_out[r_idx], pcm_len[r_idx]);
				drv_l1_dac_audop_speaker_en(1);
			}

			DBG_PRINT("D..\r\n");
			break;

		case MSG_AUD_DMA_WIDX_CLEAR:
			w_idx = 0;
			break;

		case MSG_AUD_DMA_PAUSE:
			pause = 1;

			//DBG_PRINT("dma pause\r\n");
			break;

		case MSG_AUD_START_SAMPLES_COUNT:
			total_samples = 0;
			aud_time.total_play_time = total_play_time;
			aud_time.curr_play_time = 0;
			if(start_time_send)
			{
				app_msg_send(MSG_AUD_PLAY_TIME_RES, (void *) &aud_time, sizeof(STAudioConfirm), MSG_PRI_NORMAL);
			}

			break;

		case MSG_AUD_DAC_REVRESE:
			reverse = 1;
			break;

		case MSG_AUD_DAC_REVRESE_CLEAR:
			reverse = 0;
			break;

		default:
			break;
		}
	}
}
#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)

INT32S get_audio_curr_time(void)
{
    return aud_time.curr_play_time;
}
#endif
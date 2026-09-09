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
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
STAudioTaskPara music_para;
audio_status_st aud_status;

INT16U  voice_id_fd[VOICE_ID_MAX_NUM];

#if AUDIO_PLAY_SRC_FROM_SDRAM ==1
DRAM_AUDIO_INFO     DRAM_AUDIO[DRAM_AUDIO_NUM];
static INT32U	    Dram_audio_buffer=0;
#define  Dram_audio_buffer_size  128*1024
#endif
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

void ap_music_init(void)
{
	aud_status.aud_state = STATE_IDLE;
	aud_status.volume = ap_state_config_volume_get();
	audio_vol_set(aud_status.volume);
}

void ap_music_effect_resource_init(void)
{
    voice_id_fd[VOICE_ID_CLICK]     = nv_open((INT8U *) "CLICK.WAV");
    voice_id_fd[VOICE_ID_CAMERA]    = nv_open((INT8U *) "CAMERA.WAV");
    voice_id_fd[VOICE_ID_BEEP]      = nv_open((INT8U *) "BEEP.WAV");
    voice_id_fd[VOICE_ID_POWER_ON]  = nv_open((INT8U *) "POWERON_AUDIO.WAV");
    voice_id_fd[VOICE_ID_POWER_OFF] = voice_id_fd[VOICE_ID_POWER_ON];
  //  voice_id_fd[VOICE_ID_CAPTURE]   = nv_open((INT8U *) "VOICE_CAMERA.WAV");
  //  voice_id_fd[VOICE_ID_FILE_LOCK] = nv_open((INT8U *) "VOICE_LOCK.WAV");
  //  voice_id_fd[VOICE_ID_MIC_ON] = nv_open((INT8U *) "VOICE_MIC_ON.WAV");
  //  voice_id_fd[VOICE_ID_MIC_OFF]   = nv_open((INT8U *) "VOICE_MIC_OFF.WAV");
     voice_id_fd[VOICE_ID_FCW_ALARM] = nv_open((INT8U *) "PENGCHEYUJING.WAV");
  	 voice_id_fd[VOICE_ID_LDW_LEFT] = nv_open((INT8U *) "ZUOCEYAXIANYUJING.WAV");
     voice_id_fd[VOICE_ID_LDW_RIGHT] = nv_open((INT8U *) "YOUCEYAXIANYUJING.WAV");  
     voice_id_fd[VOICE_ID_SnG] = nv_open((INT8U *) "SNG.WAV");  
     voice_id_fd[VOICE_ID_PEDESTRIAN] = nv_open((INT8U *) "PEDESTRIAN.WAV");  
     voice_id_fd[VOICE_ID_VIRTUAL_BUMPER] = nv_open((INT8U *) "BUMPER.WAV");  	 
}

void ap_music_reset(void)
{
	aud_status.aud_state = STATE_IDLE;
	audio_dec_task_msg_send(MSG_AUD_STOP, NULL, 0, MSG_PRI_NORMAL);
	aud_status.volume = ap_state_config_volume_get();
	audio_vol_set(aud_status.volume);

}

void audio_mute_ctrl_set(BOOLEAN status)
{
	if(status == TRUE)
	{
		music_para.mute = TRUE;
	}
	else
	{
		if(aud_status.volume == 0)
		{
			music_para.volume = 0;
		}
		else
		{
			music_para.volume = aud_status.volume * VOLUME_STEP - 1;
		}

		DBG_PRINT("volume = %d\r\n", music_para.volume);
		music_para.mute = FALSE;
	}

	audio_dec_task_msg_send(MSG_AUD_SET_MUTE, (void *) &music_para, sizeof(STAudioTaskPara), MSG_PRI_NORMAL);
}

void audio_vol_set(INT8U vol)
{
	if(vol > MAX_VOLUME_LEVEL)
	{
		return;
	}

	if(vol == 0)
	{
		music_para.volume = 0;
	}
	else
	{
		music_para.volume = vol * VOLUME_STEP - 1;
	}

	aud_status.volume = vol;
	ap_state_config_volume_set(aud_status.volume);

	DBG_PRINT("volume = %d\r\n", music_para.volume);
	audio_dec_task_msg_send(MSG_AUD_VOLUME_SET, (void *) &music_para, sizeof(STAudioTaskPara), MSG_PRI_NORMAL);
}

void audio_vol_inc_set(void)
{
	aud_status.volume = ap_state_config_volume_get();
	if(aud_status.volume < MAX_VOLUME_LEVEL)
	{
		aud_status.volume++;
		music_para.volume = aud_status.volume * VOLUME_STEP - 1;

		DBG_PRINT("volume = %d\r\n", music_para.volume);
		ap_state_config_volume_set(aud_status.volume);
		audio_dec_task_msg_send(MSG_AUD_VOLUME_SET, (void *) &music_para, sizeof(STAudioTaskPara), MSG_PRI_NORMAL);
	}
}

void audio_vol_dec_set(void)
{
	aud_status.volume = ap_state_config_volume_get();

	if(aud_status.volume > 0)
	{
		aud_status.volume--;
		if(aud_status.volume == 0)
		{
			music_para.volume = 0;
		}
		else
		{
			music_para.volume = aud_status.volume * VOLUME_STEP - 1;
		}

		DBG_PRINT("volume = %d\r\n", music_para.volume);
		ap_state_config_volume_set(aud_status.volume);
		audio_dec_task_msg_send(MSG_AUD_VOLUME_SET, (void *) &music_para, sizeof(STAudioTaskPara), MSG_PRI_NORMAL);
	}
}

INT32S audio_effect_play(INT32U effect_type)
{
	if(ap_state_config_beep_sound_get() == 0)
	{
		return -1;
	}

	if(effect_type < VOICE_ID_MAX_NUM)
	{
        music_para.fd = voice_id_fd[effect_type];
	}
	else
	{
		return -1;
	}

	if(music_para.fd < 0)
	{
		//DBG_PRINT("invalid effect file handle\r\n");
		return -1;
	}

	music_para.src_type = AUDIO_SRC_TYPE_APP_RS;
	music_para.audio_format = AUDIO_TYPE_WAV;
	music_para.file_len = nv_rs_size_get(music_para.fd);
	DBG_PRINT("effect length = %d\r\n", music_para.file_len);
	aud_status.aud_state = STATE_PLAY;

	#if AUDIO_PLAY_SRC_FROM_SDRAM ==1
    if(Dram_audio_buffer)
	{
        DRAM_AUDIO[0].length = nv_rs_size_get(music_para.fd);
        DRAM_AUDIO[0].start_addr = Dram_audio_buffer;
        DRAM_AUDIO[0].cur_pos = 0;
        nv_read(music_para.fd, DRAM_AUDIO[0].start_addr,DRAM_AUDIO[0].length);
        music_para.fd = 0;
        music_para.src_type = AUDIO_SRC_TYPE_SDRAM;
	}
	#endif

	audio_dec_task_msg_send(MSG_AUD_PLAY, (void *) &music_para, sizeof(STAudioTaskPara), MSG_PRI_NORMAL);

	return 0;
}

void audio_effect_wait(void)
{
#define DAC_WAIT_RUN	5
#define DAC_WAIT_CNT	10000
	INT32U	cnt, run;
	for(cnt = 0, run = 0; (cnt < DAC_WAIT_CNT) && (run < DAC_WAIT_RUN); ++cnt, ++run)
	{
		while((drv_l1_dac_dbf_status_get() == 1) || (drv_l1_dac_dma_status_get() == 1))
		{
			run = 0;
		}
		vTaskDelay(10);
	}
}

INT8U audio_playing_state_get(void)
{
	return aud_status.aud_state;
}

void audio_res_play_process(INT32S result)
{
	switch(result)
	{
	case AUDIO_ERR_NONE:
		DBG_PRINT("state task - start play\r\n");
		return;

	case AUDIO_ERR_INVALID_FORMAT:
		DBG_PRINT("state task - invalid file format\r\n");
		break;

	case AUDIO_ERR_DEC_FINISH:
            aud_status.aud_state = STATE_IDLE;
		DBG_PRINT("state task - decode finsish\r\n");
		break;

	case AUDIO_ERR_DEC_FAIL:
            aud_status.aud_state = STATE_IDLE;
		DBG_PRINT("state task - decode fail\r\n");
			break;
		default:
			break;
	}
}

void audio_res_resume_process(INT32S result)
{
	DBG_PRINT("umi task - resume success\r\n");
}

void audio_res_pause_process(INT32S result)
{
	DBG_PRINT("umi task - pause success\r\n");
}

void audio_res_stop_process(INT32S result)
{
	DBG_PRINT("stop success\r\n");
}

void audio_confirm_handler(STAudioConfirm *aud_con)
{
	switch(aud_con->result_type)
	{
	case MSG_AUD_PLAY_RES:
		audio_res_play_process(aud_con->result);
		break;

	case MSG_AUD_BG_PLAY_RES:
		break;

	case MSG_AUD_STOP_RES:
		audio_res_stop_process(aud_con->result);
		break;

	case MSG_AUD_PAUSE_RES:
		audio_res_pause_process(aud_con->result);
		break;

	case MSG_AUD_RESUME_RES:
		audio_res_resume_process(aud_con->result);
		break;

	default:
		break;
	}
}

void audio_wav_play(INT16S fd)
{
	music_para.fd = fd;
	music_para.src_type = AUDIO_SRC_TYPE_FS;
	aud_status.aud_state = STATE_PLAY;
	audio_dec_task_msg_send(MSG_AUD_PLAY, (void *) &music_para, sizeof(STAudioTaskPara), MSG_PRI_NORMAL);
}

INT32S audio_wav_pause(void)
{
	if(aud_status.aud_state != STATE_PLAY)
	{
		return 0;
	}

	aud_status.aud_state = STATE_PAUSED;
	storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
	audio_dec_task_msg_send(MSG_AUD_PAUSE, NULL, 0, MSG_PRI_NORMAL);
	return 0;
}

INT32S audio_wav_resume(void)
{
	if(aud_status.aud_state != STATE_PAUSED)
	{
		return 0;
	}

	aud_status.aud_state = STATE_PLAY;
	storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_STOP, NULL, 0, MSG_PRI_NORMAL);
	audio_dec_task_msg_send(MSG_AUD_RESUME, NULL, 0, MSG_PRI_NORMAL);
	return 0;
}

void audio_wav_stop(void)
{
	aud_status.aud_state = STATE_IDLE;
	audio_dec_task_msg_send(MSG_AUD_STOP, NULL, 0, MSG_PRI_NORMAL);
}

void ap_state_audio_reverse_set(INT8U reverse)
{
	music_para.reverse = reverse;
	audio_dec_task_msg_send(MSG_AUD_REVERSE_SET, (void *) &music_para, sizeof(STAudioTaskPara), MSG_PRI_NORMAL);
}

void ap_state_audio_play_speed_set(INT8U speed)
{
	music_para.play_speed = speed;
	audio_dec_task_msg_send(MSG_AUD_PLAY_SPEED_SET, (void *) &music_para, sizeof(STAudioTaskPara), MSG_PRI_NORMAL);
}

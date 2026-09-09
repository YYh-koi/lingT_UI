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

// Constant definitions used in this file only go here
#define AUDIO_FS_Q_SIZE			1
#define AUDIO_WRITE_Q_SIZE		MAX_DAC_BUFFERS
#define AUDIO_PARA_MAX_LEN		sizeof(STAudioTaskPara)
#define RAMP_DOWN_STEP			4
#define RAMP_DOWN_STEP_HOLD		4
#define RAMP_DOWN_STEP_LOW_SR	4 * 16
#define USE_RAMP_DOWN_RAPID		0
#define SKIP_ID3_TAG			1

#define READING_BY_FILE_SRV		1

#define PERFORMANCE_MEASURE		0
#if PERFORMANCE_MEASURE == 1
#define R_TIMERC_CTRL		(*((volatile INT32U *) 0xC0020040))
#define R_TIMERC_CCP_CTRL	(*((volatile INT32U *) 0xC0020044))
#define R_TIMERC_PRELOAD	(*((volatile INT32U *) 0xC0020048))
#define R_TIMERC_CCP_REG	(*((volatile INT32U *) 0xC002004C))
#define R_TIMERC_UPCOUNT	(*((volatile INT32U *) 0xC0020050))

INT32S					cnt_max[3] = { 0 };
INT32U					cnt_max_fn[3] = { 0 };
INT32U					point[3] = { 0 };
INT32U					timer_cnt1 = 0;
INT32U					timer_cnt2 = 0;
INT32U					total_cnt = 0;
INT32U					total_frame = 0;
INT32U					total_point = 0;
INT32S					times;
INT32S					times0;
#endif // PERFORMANCE_MEASURE == 1

/* Task Q declare */
QueueHandle_t			audio_wq;
QueueHandle_t			audio_fsq;

INT16S					*pcm_out[MAX_DAC_BUFFERS] = { NULL };
INT32U					pcm_len[MAX_DAC_BUFFERS];

AUDIO_CONTEXT			audio_context;
AUDIO_CONTEXT_P			audio_context_p = &audio_context;
AUDIO_CTRL				audio_ctrl;
STAudioConfirm			aud_con;
INT32U					pcm_buf;

static struct sfn_info	aud_sfn_file;
INT8U					stopped;
INT32U					dac_buf_nums;
INT8U					FB_seek, MP3_Speed;
INT32U					FrameSize_ave;

void (*decode_end) (INT32U audio_decoder_num);
INT8U			channel;
INT32U			g_audio_sample_rate;

INT32U			total_play_time;
INT8U			start_time_send;

static INT8U	reverse_play;
static INT8U	reverse_play_start;
static INT8U	bit_per_samp;
static INT32U	reverse_points;
static INT32U	dac_volume;

#define GUARD_BUFFER	(5 * 1024)	// 修正從 SDRAM 播聲音 FileSrvLDWAudioAppRead 可能產生的錯誤
static ALIGN64 INT32U	audio_dec_buf[(RING_BUF_SIZE + GUARD_BUFFER + WAV_PCM_BUF_SIZE * MAX_DAC_BUFFERS + WAV_DEC_MEMORY_SIZE) / 4 + 2];


/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/



#if USE_RAMP_DOWN_RAPID == 1
static void audio_ramp_down_rapid(void)
{
	INT16S	last_ldata, last_rdata;
	INT16S	i, temp;

	xQueueReset(aud_send_q);
	while(dac_dma_status_get() == 1)
	{
		vTaskDelay(1);
	}

	//free and reset DMA channel
	dac_dbf_free();

	temp = 0 - RAMP_DOWN_STEP;
	last_ldata = R_DAC_CHA_DATA;
	last_rdata = R_DAC_CHB_DATA;

	//unsigned to signed
	last_ldata ^= 0x8000;
	last_rdata ^= 0x8000;

	//change timer to 44100
	drv_l1_dac_sample_rate_set(44100);

	while(1)
	{
		if(last_ldata > 0x0)
		{
			last_ldata -= RAMP_DOWN_STEP;
		}
		else
		if(last_ldata < 0x0)
		{
			last_ldata += RAMP_DOWN_STEP;
		}

		if((last_ldata < RAMP_DOWN_STEP) && (last_ldata > temp))
		{
			last_ldata = 0;
		}

		if(channel == 2)
		{
			if(last_rdata > 0x0)
			{
				last_rdata -= RAMP_DOWN_STEP;
			}
			else
			if(last_rdata < 0x0)
			{
				last_rdata += RAMP_DOWN_STEP;
			}

			if((last_rdata < RAMP_DOWN_STEP) && (last_rdata > temp))
			{
				last_rdata = 0;
			}
		}

		for(i = 0; i < RAMP_DOWN_STEP_HOLD; i++)
		{
			if(channel == 2)
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

		if((last_ldata == 0x0) && (last_rdata == 0x0))
		{
			break;
		}
	}
}
#endif // USE_RAMP_DOWN_RAPID

static void audio_file_read(P_TK_FILE_SERVICE_STRUCT para)
{
	INT32S	read_cnt;

	if(para->rev_seek)
	{
		if(para->FB_seek)
		{
			lseek(para->fd, -para->rev_seek, SEEK_CUR);
			DBG_PRINT("reverse seek- %d\r\n", para->rev_seek);
		}
		else
		{
			lseek(para->fd, para->rev_seek, SEEK_CUR);
			DBG_PRINT("reverse seek+ %d\r\n", para->rev_seek);
		}
	}

	read_cnt = read(para->fd, para->buf_addr, para->buf_size);
	if(para->result_queue)
	{
		xQueueSend(para->result_queue, (void*)&read_cnt, 0);
	}
}

static void audio_nvram_read(P_TK_FILE_SERVICE_STRUCT para)
{
	INT32S	read_cnt = 0;
	INT32U	ret;

	ret = nv_read(para->fd, para->buf_addr, para->buf_size);
	if(ret == 0)
	{
		read_cnt = para->buf_size;
	}

	if(para->result_queue)
	{
		xQueueSend(para->result_queue, (void*)&read_cnt, 0);
	}
}

#if AUDIO_PLAY_SRC_FROM_SDRAM ==1
extern DRAM_AUDIO_INFO  DRAM_AUDIO[];
static void audio_sdram_read(P_TK_FILE_SERVICE_STRUCT para)
{
	INT32S	read_cnt = 0;

	if ( (DRAM_AUDIO[para->fd].cur_pos+para->buf_size)<DRAM_AUDIO[para->fd].length) {
		read_cnt = para->buf_size;
	}
	else {
		read_cnt = DRAM_AUDIO[para->fd].length - DRAM_AUDIO[para->fd].cur_pos;
	}

	memcpy((void*)para->buf_addr, (void *)(DRAM_AUDIO[para->fd].start_addr+DRAM_AUDIO[para->fd].cur_pos), read_cnt);

	DRAM_AUDIO[para->fd].cur_pos += read_cnt;
	if (DRAM_AUDIO[para->fd].cur_pos>=DRAM_AUDIO[para->fd].length) {
		DRAM_AUDIO[para->fd].cur_pos = 0;
	}

	//DBG_PRINT("read_cnt= %d\r\n", read_cnt);

	if(para->result_queue)
	{
		xQueueSend(para->result_queue, (void*)&read_cnt, 0);
	}
}
#endif

static void audio_wav_read(INT8U src_type, P_TK_FILE_SERVICE_STRUCT para)
{

	switch(src_type)
	{
	case AUDIO_SRC_TYPE_FS:
		audio_file_read(para);
	break;

	case AUDIO_SRC_TYPE_APP_RS:
		audio_nvram_read(para);
	break;
#if AUDIO_PLAY_SRC_FROM_SDRAM ==1
	case AUDIO_SRC_TYPE_SDRAM:
		audio_sdram_read(para);
	break;
#endif
	default:
	break;
	}
}

#if (defined AUDIO_FORMAT_JUDGE_AUTO) && (AUDIO_FORMAT_JUDGE_AUTO == 1)
static INT8S parse_mp3_file_head(INT8S *p_audio_file_head_ptr)
{
	INT8S			mpeg_version, layer, sample_rate;
	INT8S			j = 0;
	INT32U			ID3V2_length = 0;
	INT32U			cnt = AUDIO_PASER_BUFFER_SIZE;
	MP3_FILE_HEAD	mp3_file_head[2];
	INT8S			*p_audio_file_head;

	p_audio_file_head = p_audio_file_head_ptr;

	if(*(p_audio_file_head) == (INT8S) 'I' && *(p_audio_file_head + 1) == (INT8S) 'D' && *(p_audio_file_head + 2) == (INT8S) '3')
	{
		ID3V2_length = 10 + ((*(p_audio_file_head + 9) & 0x7f) | ((*(p_audio_file_head + 8) & 0x7f) << 7) | ((*(p_audio_file_head + 7) & 0x7f) << 14) | ((*(p_audio_file_head + 6) & 0x7f) << 21));

		//ID3V2_length	+= 10 + ((*(p_audio_file_head + 6) & 0x7f)<<21);
		//ID3V2_length	+= ((*(p_audio_file_head + 7) & 0x7f)<<14);
		//ID3V2_length	+= ((*(p_audio_file_head + 8) & 0x7f)<<7);
		//ID3V2_length	+=  (*(p_audio_file_head + 9)& 0x7f);
		/*
		if(ID3V2_length > 1024)
		{
			DBG_PRINT("audio file ID3V2 too long \r\n");
			return AUDIO_PARSE_SUCCS ;
		}
		else
		{
			p_audio_file_head +=  ID3V2_length;
			cnt -= ID3V2_length;
		}
		*/
		DBG_PRINT("audio file parse succes \r\n");

		return AUDIO_PARSE_SUCCS;
	}

	while(cnt > 4)
	{
		if((*(p_audio_file_head) == 0xFF) && ((*(p_audio_file_head + 1) & 0xE0) == 0xE0) && ((*(p_audio_file_head + 2) & 0xF0) != 0xF0))	// first 11 bits should be 1
		{
			mpeg_version = (*(p_audio_file_head + 1) & 0x18) >> 3;
			layer = (*(p_audio_file_head + 1) & 0x06) >> 1;
			sample_rate = (*(p_audio_file_head + 2) & 0x0c) >> 2;

			if((mpeg_version != 0x01) && (layer != 0x00) && (layer != 0x03) && (sample_rate != 0x03))	// != RESERVERD
			{
				if(j < 2)
				{
					mp3_file_head[j].mpeg_version = mpeg_version;
					mp3_file_head[j].layer = layer;
					mp3_file_head[j].sample_rate = sample_rate;

					j++;
				}
				else
				if
				(
					(mp3_file_head[0].mpeg_version == mp3_file_head[1].mpeg_version)
				&&	(mp3_file_head[0].layer == mp3_file_head[1].layer)
				&&	(mp3_file_head[0].sample_rate == mp3_file_head[1].sample_rate)
				)
				{
					DBG_PRINT("audio file parse succes \r\n");
					return AUDIO_PARSE_SUCCS;
				}
			}
			else
			{
				p_audio_file_head += 4;
				cnt -= 4;
			}
		}
		else
		{
			p_audio_file_head += 4;
			cnt -= 4;
		}
	}

	return AUDIO_PARSE_FAIL;
}

static INT8S parse_wma_file_head(INT8S *p_audio_file_head)
{
	if(*(p_audio_file_head) != 0x30 || *(p_audio_file_head + 1) != 0x26 || *(p_audio_file_head + 2) != 0xB2 || *(p_audio_file_head + 3) != 0x75)
	{
		return AUDIO_PARSE_FAIL;
	}
	else
	if(*(p_audio_file_head + 4) != 0x8E || *(p_audio_file_head + 5) != 0x66 || *(p_audio_file_head + 6) != 0xCF || *(p_audio_file_head + 7) != 0x11)
	{
		return AUDIO_PARSE_FAIL;
	}
	else
	if(*(p_audio_file_head + 8) != 0xA6 || *(p_audio_file_head + 9) != 0xD9 || *(p_audio_file_head + 10) != 0x00 || *(p_audio_file_head + 11) != 0xAA)
	{
		return AUDIO_PARSE_FAIL;
	}
	else
	if(*(p_audio_file_head + 12) != 0x00 || *(p_audio_file_head + 13) != 0x62 || *(p_audio_file_head + 14) != 0xCE || *(p_audio_file_head + 15) != 0x6C)
	{
		return AUDIO_PARSE_FAIL;
	}
	else
	{
		return AUDIO_PARSE_SUCCS;
	}
}

static INT8S parse_wav_file_head(INT8S *p_audio_file_head)
{
	INT16S	wav_format_type;

	if(*(p_audio_file_head) != (INT8S) 'R' || *(p_audio_file_head + 1) != (INT8S) 'I' || *(p_audio_file_head + 2) != (INT8S) 'F' || *(p_audio_file_head + 3) != (INT8S) 'F')
	{
		return AUDIO_PARSE_FAIL;
	}
	else
	if(*(p_audio_file_head + 8) != (INT8S) 'W' || *(p_audio_file_head + 9) != (INT8S) 'A' || *(p_audio_file_head + 10) != (INT8S) 'V' || *(p_audio_file_head + 11) != (INT8S) 'E')
	{
		return AUDIO_PARSE_FAIL;
	}
	else
	if(*(p_audio_file_head + 12) != (INT8S) 'f' || *(p_audio_file_head + 13) != (INT8S) 'm' || *(p_audio_file_head + 14) != (INT8S) 't' || *(p_audio_file_head + 15) != 0x20)
	{
		return AUDIO_PARSE_FAIL;
	}
	else
	{						//add for mantis #9116  090612 by Lion
		wav_format_type = (INT16S) ((*(p_audio_file_head + 21)) << 8);
		wav_format_type += *(p_audio_file_head + 20);

		if
		(
			wav_format_type == WAVE_FORMAT_PCM
		||	wav_format_type == WAVE_FORMAT_ADPCM
		||	wav_format_type == WAVE_FORMAT_ALAW
		||	wav_format_type == WAVE_FORMAT_MULAW
		||	wav_format_type == WAVE_FORMAT_IMA_ADPCM
		)
		{
			return AUDIO_PARSE_SUCCS;
		}
		else				//for example  WAVE_FORMAT_MPEGLAYER3
		{
			return AUDIO_PARSE_FAIL;
		}
	}

	/*
	INT8S	temp[9] = {0};
	INT16U	i;
	gp_memcpy(temp,p_audio_file_head,8);

	for (i=0;i<gp_strlen(temp);i++) {
		temp[i] = gp_toupper(temp[i]);
	}

	if((gp_strcmp(temp, (INT8S *)"RIFFWAVE")==0)&&(gp_strcmp((p_audio_file_head + 12), (INT8S *)"fmt ")==0))
	{
		return AUDIO_PARSE_SUCCS;
	}
	else
	{
		return AUDIO_PARSE_FAIL ;
	}
	*/
}

static INT32S audio_real_type_get(INT16S fd, INT8S type_index)
{
	INT16U					i;
	INT32S					len;
	AUDIO_TYPE				audio_real_type;
	INT32U					audio_file_head;
	TK_FILE_SERVICE_STRUCT	audio_file_head_para;
	AUDIO_FILE_PARSE		audio_file_parse_head[AUDIO_MAX_TYPE];

	if((type_index != AUDIO_TYPE_WAV) && (type_index != AUDIO_TYPE_MP3) && (type_index != AUDIO_TYPE_WMA))
	{
		return type_index;
	}

	audio_file_parse_head[AUDIO_TYPE_MP3].audio_file_real_type = AUDIO_TYPE_MP3;
	audio_file_parse_head[AUDIO_TYPE_MP3].parse_audio_file_head = parse_mp3_file_head;

	audio_file_parse_head[AUDIO_TYPE_WMA].audio_file_real_type = AUDIO_TYPE_WMA;
	audio_file_parse_head[AUDIO_TYPE_WMA].parse_audio_file_head = parse_wma_file_head;

	audio_file_parse_head[AUDIO_TYPE_WAV].audio_file_real_type = AUDIO_TYPE_WAV;
	audio_file_parse_head[AUDIO_TYPE_WAV].parse_audio_file_head = parse_wav_file_head;

	audio_file_head_para.fd = fd;
	audio_file_head_para.result_queue = audio_fsq;
	audio_file_head_para.buf_size = AUDIO_PASER_BUFFER_SIZE;

	audio_file_head = (INT32U) gp_malloc((audio_file_head_para.buf_size));
	if(audio_file_head == NULL)
	{
		DBG_PRINT("audio file parse failed to allocate memory\r\n");

		return type_index;	//what i can do if malloc error?
	}

	audio_file_head_para.buf_addr = (INT32U) audio_file_head;

	lseek((INT16S) audio_file_head_para.fd, 0, SEEK_SET);
	audio_wav_read(AUDIO_SRC_TYPE_FS, &audio_file_head_para);

	if((xQueueReceive(audio_fsq, (void *) &len, 200) != pdTRUE) || (len < 0))
	{
		gp_free((void *) audio_file_head);
		return type_index;	// AUDIO_READ_FAIL;	//how to handle it  if read error?
	}

	lseek((INT16S) audio_file_head_para.fd, 0, SEEK_SET);

	// 1. check it is real the type_index,according the file extension to judge the file real type.
	if((audio_file_parse_head[type_index].parse_audio_file_head((INT8S *) audio_file_head)) == AUDIO_PARSE_SUCCS)
	{
		gp_free((void *) audio_file_head);
		return audio_file_parse_head[type_index].audio_file_real_type;
	}

	// else then enum all support file type except current extension type
	for(i = 1; i < AUDIO_MAX_TYPE; i++)
	{
		if((i == type_index) && (i < AUDIO_MAX_TYPE - 1))
		{
			i++;
		}
		else
		if((i == type_index) && (i >= AUDIO_MAX_TYPE - 1))
		{
			audio_real_type = AUDIO_TYPE_NONE;
			break;			// if type_index = AUDIO_MAX_TYPE & i the same as type_index ,so should be break.
		}

		if((audio_file_parse_head[i].parse_audio_file_head((INT8S *) audio_file_head)) == AUDIO_PARSE_SUCCS)
		{
			audio_real_type = audio_file_parse_head[i].audio_file_real_type;
			break;
		}
		else
		{
			audio_real_type = AUDIO_TYPE_NONE;
		}
	}

	gp_free((void *) audio_file_head);

	return audio_real_type;
}

#endif // (defined AUDIO_FORMAT_JUDGE_AUTO) && (AUDIO_FORMAT_JUDGE_AUTO == 1)
#if READING_BY_FILE_SRV == 0
static INT32S audio_write_buffer(INT8U *ring_buf, INT32U wi, INT32U ri)
{
	INT32S	t;
	INT32S	len;

	if(wi == 0 && ri == 0)
	{
		len = read(audio_ctrl.file_handle, (INT32U) ring_buf, audio_ctrl.ring_size / 2);
		wi += len;
		return wi;
	}

	len = ri - wi;
	if(len < 0)
	{
		len += audio_ctrl.ring_size;
	}

	if(len < audio_ctrl.ring_size / 2)
	{
		//if(len < (audio_ctrl.ring_size - 2048)) {
		return wi;
	}

	t = wi;
	wi += audio_ctrl.ring_size / 2;
	if(wi == audio_ctrl.ring_size)
	{
		wi = 0;
	}

	if(wi == ri)
	{
		return t;
	}

	len = read(audio_ctrl.file_handle, (INT32U) ring_buf + t, audio_ctrl.ring_size / 2);
	if(len < 0)
	{
		DBG_PRINT("error read file \r\n");
	}

	wi = t + len;
	if(wi == audio_ctrl.ring_size)
	{
		wi = 0;
	}

	return wi;
}

#else // READING_BY_FILE_SRV == 0

static INT32S audio_write_with_file_srv(INT8U *ring_buf, INT32U wi, INT32U ri)
{
	TK_FILE_SERVICE_STRUCT audio_fs_para;
	INT32S	t;
	INT32S	len;

	if(reverse_play || MP3_Speed)
	{
		return wi;
	}

	audio_fs_para.fd = audio_ctrl.file_handle;
	audio_fs_para.result_queue = audio_fsq;

	len = wi - ri;
	if(len < 0)
	{
		len += audio_ctrl.ring_size;
	}

	if((len < 2048) && audio_ctrl.reading)
	{
		return AUDIO_READ_WAIT;
	}

	if(ri == wi)
	{
		audio_fs_para.buf_addr = (INT32U) ring_buf;
		audio_fs_para.buf_size = audio_ctrl.ring_size / 2;
		audio_fs_para.spi_para.sec_cnt = audio_ctrl.ring_size / 1024;
		if(reverse_play_start)
		{
			INT32S audio_dac_msg;

			audio_dac_msg = MSG_AUD_DAC_REVRESE_CLEAR;

			if(xQueueSend(hAudioDacTaskQ, (void*)&audio_dac_msg, 0) != pdTRUE)
			{
				DBG_PRINT("[ERROR] hAudioDacTaskQ post fail, Q=0x%X\r\n", hAudioDacTaskQ);
			}


			vTaskDelay(1);
			if(audio_context_p->audio_format == AUDIO_TYPE_WAV)
			{
				audio_fs_para.rev_seek = reverse_points * (bit_per_samp / 8) * channel;
			}
			else
			{
				audio_fs_para.rev_seek = reverse_points * FrameSize_ave;
			}

			audio_fs_para.FB_seek = FB_seek;
			reverse_points = 0;
			reverse_play_start = 0;
			audio_mute_ctrl_set(FALSE);
			audio_ctrl.wi = 0;
			wi = 0;
		}
		else
		{
			audio_fs_para.rev_seek = 0;
		}

		audio_wav_read(audio_context_p->source_type, &audio_fs_para);

		if((xQueueReceive(audio_fsq, (void*)&len, portMAX_DELAY) != pdTRUE) || (len < 0))
		{
			return AUDIO_READ_FAIL;
		}

		audio_ctrl.read_secs += (len >> 9);
		wi += len;
		if(wi == audio_ctrl.ring_size)
		{
			wi = 0;
		}

		return wi;
	}

	len = ri - wi;
	if(len < 0)
	{
		len += audio_ctrl.ring_size;
	}

	if(len < audio_ctrl.ring_size / 2)
	{
		return wi;
	}

	t = wi;
	wi += audio_ctrl.ring_size / 2;
	if(wi == audio_ctrl.ring_size)
	{
		wi = 0;
	}

	if(wi == ri)
	{
		return t;
	}

	if(audio_ctrl.reading)
	{
		return AUDIO_READ_PEND;
	}

	audio_ctrl.reading = 1;

	audio_fs_para.buf_addr = (INT32U) ring_buf + t;
	audio_fs_para.buf_size = audio_ctrl.ring_size / 2;
	audio_fs_para.spi_para.sec_cnt = audio_ctrl.ring_size / 1024;
	audio_fs_para.rev_seek = 0;
	audio_wav_read(audio_context_p->source_type, &audio_fs_para);
	return AUDIO_READ_PEND;
}

void audio_work_ringbuffer_clear()
{
	audio_ctrl.wi = 0;
}

#endif // READING_BY_FILE_SRV == 0
#if READING_BY_FILE_SRV == 1
static INT32S audio_check_wi(INT32S wi_in, INT32U *wi_out, INT8U wait)
{
	INT32S		len;
	BaseType_t	err;
	INT32S	t;

	if(wi_in >= 0)
	{
		*wi_out = wi_in;
		return AUDIO_ERR_NONE;
	}

	if(wi_in == AUDIO_READ_FAIL)
	{
		return AUDIO_READ_FAIL;
	}

	/* AUDIO_READ_PEND */
	t = audio_ctrl.wi;

	if(wait)
	{
		//DBG_PRINT("need pend\r\n");
		err = xQueueReceive(audio_fsq, (void*)&len, portMAX_DELAY);
	}
	else
	{
		err = xQueueReceive(audio_fsq, (void*)&len, 0);
		if(err == pdFALSE)
		{
			//*wi_out = *wi_out;
			return AUDIO_ERR_NONE;
		}
	}

	audio_ctrl.reading = 0;

	if((err != pdTRUE) || len < 0)
	{
		return AUDIO_READ_FAIL;
	}

	if(len != (audio_ctrl.ring_size >> 1))
	{
		audio_ctrl.file_end = 1;
	}

	audio_ctrl.read_secs += (len >> 9);
	t += len;
	if(t == audio_ctrl.ring_size)
	{
		t = 0;
	}

	*wi_out = t;
	return AUDIO_ERR_NONE;
}

#endif // READING_BY_FILE_SRV == 1
#if (defined SKIP_ID3_TAG) && (SKIP_ID3_TAG == 1)
static INT8U audio_get_id3_type(INT8U *data, INT32U length)
{
	if(length >= 3 && data[0] == 'T' && data[1] == 'A' && data[2] == 'G')
	{
		return ID3_TAG_V1;
	}

	if(length >= 10 && ((data[0] == 'I' && data[1] == 'D' && data[2] == '3') || (data[0] == '3' && data[1] == 'D' && data[2] == 'I')) && (data[3] < 0xff) && (data[4] < 0xff))
	{
		if(data[0] == 'I')
			return ID3_TAG_V2;
		else
			return ID3_TAG_V2_FOOTER;
	}

	return ID3_TAG_NONE;
}

static INT32U audio_id3_get_size(INT8U *ptr)
{
	INT32U	value = 0;

	value = (value << 7) | (*ptr++ &0x7f);
	value = (value << 7) | (*ptr++ &0x7f);
	value = (value << 7) | (*ptr++ &0x7f);
	value = (value << 7) | (*ptr++ &0x7f);

	return value;
}

static void audio_parse_id3_header(INT8U *header, INT32U *version, INT32S *flags, INT32U *size)
{
	INT8U	*ptr;
	INT32U	ver = 0;

	ptr = header;
	ptr += 3;

	ver = *ptr++ << 8;
	*version = ver | *ptr++;
	*flags = *ptr++;
	*size = audio_id3_get_size(ptr);
}

INT32S audio_id3_get_tag_len(INT8U *data, INT32U length)
{
	INT32U	version;
	INT32S	flags;
	INT32U	len;

	switch(audio_get_id3_type(data, length))
	{
	case ID3_TAG_V1:
		return 128;

	case ID3_TAG_V2:
		audio_parse_id3_header(data, &version, &flags, &len);
		if(flags & ID3_TAG_FLAG_FOOTER)
		{
			len += 10;
		}

		return 10 + len;

	case ID3_TAG_NONE:
		break;
	}

	return 0;
}

#endif // (defined SKIP_ID3_TAG) && (SKIP_ID3_TAG == 1)
static INT32S audio_mem_alloc(INT32U audio_format, AUDIO_CTRL *aud_ctrl, INT16S *pcm[])
{
	INT32S	i;
	INT32U	pcm_size;
	//INT32U	wm_size;

	switch(audio_format)
	{
	case AUDIO_TYPE_WAV:
		//wm_size = WAV_DEC_MEMORY_SIZE;
		pcm_size = WAV_PCM_BUF_SIZE;
		break;

	case AUDIO_TYPE_AVI:
		//wm_size = WAV_DEC_MEMORY_SIZE;
		pcm_size = WAV_PCM_BUF_SIZE;
		break;

	default:
		//return AUDIO_ERR_FAILED;
		return AUDIO_ERR_NONE;
	}

	for(i = 0; i < dac_buf_nums; i++)
	{
		pcm[i] = (INT16S *) (pcm_buf + (i * pcm_size));

		//DBG_PRINT("pcm buffer[%d] = 0x%x (%d)\r\n",i,pcm[i],pcm_size*2);
	}

	return AUDIO_ERR_NONE;
}

static INT32S audio_get_type(INT16S fd, INT8S *file_name)
{
	INT8S	temp[5] = { 0 };
	INT16U	i;

	gp_strcpy(temp, file_name);

	for(i = 0; i < gp_strlen(temp); i++)
	{
		temp[i] = gp_toupper(temp[i]);
	}

#if (defined GP_FILE_FORMAT_SUPPORT) && (GP_FILE_FORMAT_SUPPORT == GP_FILE_FORMAT_SET_1)
	if(gp_strcmp(temp, (INT8S *) "GA") == 0)
	{
		return AUDIO_TYPE_WMA;
	}
	else
	{
		return AUDIO_TYPE_NONE;
	}

#endif
	if(gp_strcmp(temp, (INT8S *) "WAV") == 0)
	{
#if (defined AUDIO_FORMAT_JUDGE_AUTO) && (AUDIO_FORMAT_JUDGE_AUTO == 1)
		return audio_real_type_get(fd, AUDIO_TYPE_WAV);
#else
		return AUDIO_TYPE_WAV;
#endif
	}

	if(gp_strcmp(temp, (INT8S *) "IDI") == 0)
		return AUDIO_TYPE_MIDI;

	if(gp_strcmp(temp, (INT8S *) "GMD") == 0)
		return AUDIO_TYPE_MIDI;

	return AUDIO_TYPE_NONE;
}

INT32S audio_play_file_set(INT16S fd, AUDIO_CONTEXT *aud_context, AUDIO_CTRL *aud_ctrl)
{
#if (defined SKIP_ID3_TAG) && (SKIP_ID3_TAG == 1)
	INT8U	id3[10];
	INT32U	len;
#endif
	sfn_stat(fd, &aud_sfn_file);
	aud_context->audio_format = audio_get_type(fd, aud_sfn_file.f_extname);

	if(aud_context->audio_format == AUDIO_TYPE_NONE)
	{
		DBG_PRINT("audio_play_file_set find not support audio.\r\n");
		return AUDIO_ERR_INVALID_FORMAT;
	}

	aud_ctrl->file_handle = fd;

	/* skip id3 */
#if (defined SKIP_ID3_TAG) && (SKIP_ID3_TAG == 1)
	if(aud_context->audio_format == AUDIO_TYPE_MP3)
	{
		read(fd, (INT32U) id3, 10);
		len = audio_id3_get_tag_len(id3, 10);
		aud_ctrl->file_len = aud_sfn_file.f_size - (len &~3);
		lseek(fd, 0, SEEK_SET);
		lseek(fd, (len &~3), SEEK_SET); /* 4 byte alignment */
	}
	else
	{
		aud_ctrl->file_len = aud_sfn_file.f_size;
	}

#else
	aud_ctrl->file_len = aud_sfn_file.f_size;
#endif
	return AUDIO_ERR_NONE;
}

static void audio_queue_clear(void)
{
	INT32S audio_dac_msg;

	xQueueReset(aud_send_q);
	xQueueReset(audio_wq);
	xQueueReset(audio_fsq);

	audio_dac_msg = MSG_AUD_DMA_WIDX_CLEAR;
	if(xQueueSend(hAudioDacTaskQ, (void*)&audio_dac_msg, 0) != pdTRUE)
	{
		DBG_PRINT("[ERROR] hAudioDacTaskQ post fail, Q=0x%X\r\n", hAudioDacTaskQ);
	}

}

static void audio_mute_set(STAudioTaskPara *pAudioTaskPara)
{
	if(pAudioTaskPara->mute == TRUE)
	{
		drv_l1_dac_pga_set(0);

		//dac_vref_set(FALSE);
	}
	else
	{
		drv_l1_dac_pga_set(pAudioTaskPara->volume);

		//dac_vref_set(TRUE);
	}

	audio_send_result(MSG_AUD_MUTE_SET_RES, AUDIO_ERR_NONE);
}

static void audio_reverse_set(STAudioTaskPara *pAudioTaskPara)
{
	INT32S	len;

	if(audio_context_p->state != AUDIO_PLAYING)
	{
		return;
	}

	if(pAudioTaskPara->reverse == 1)
	{
		reverse_play = 1;
	}
	else
	{
		reverse_play = 0;
	}

	if(pAudioTaskPara->audio_format == AUDIO_TYPE_MP3)
	{
		if(audio_ctrl.wi != audio_ctrl.ri)
		{
			len = audio_ctrl.wi - audio_ctrl.ri;
			if(len < 0)
			{
				len += audio_ctrl.ring_size;
			}

			lseek(audio_ctrl.file_handle, -len, SEEK_CUR);
			audio_work_ringbuffer_clear();
		}
	}
}

static void audio_play_speed_set(STAudioTaskPara *pAudioTaskPara)
{
	INT32U	speed;

	speed = pAudioTaskPara->play_speed;

	// only support 1,2,4 times
	if(pAudioTaskPara->audio_format == AUDIO_TYPE_WAV)
	{
		if(speed == 2 || speed == 4)
		{
			drv_l1_dac_pga_set(0);
		}
		else
		if(speed == 1)
		{
			drv_l1_dac_pga_set(dac_volume);
		}
		else
		{
			return;
		}

		drv_l1_dac_sample_rate_set(g_audio_sample_rate * speed);
	}
	else
	if(pAudioTaskPara->audio_format == AUDIO_TYPE_MP3)
	{
		if(speed == 1)
		{
			MP3_Speed = 0;
		}
		else
		if(speed == 2)
		{
			MP3_Speed = 1;
		}
		else
		if(speed == 4)
		{
			MP3_Speed = 2;
		}
		else
		{
			MP3_Speed = 0;
		}
	}
}

static void audio_volume_set(STAudioTaskPara *pAudioTaskPara)
{
	if(pAudioTaskPara->volume < 64)
	{
		drv_l1_dac_pga_set(pAudioTaskPara->volume);
		dac_volume = pAudioTaskPara->volume;
	}

	audio_send_result(MSG_AUD_VOLUME_SET_RES, AUDIO_ERR_NONE);
}

static INT32S audio_send_to_dma(void)
{
	INT32S audio_dac_msg;

	if(stopped && (uxQueueMessagesWaiting(aud_send_q) >= dac_buf_nums))
	{
		/* queuq full */
		stopped = 0;

		// Turn on speaker
		ap_peripheral_speaker_ctrl(1);

		audio_dac_msg = MSG_AUD_DMA_DBF_START;
		if(xQueueSend(hAudioDacTaskQ, (void*)&audio_dac_msg, 0) != pdTRUE)
		{
			DBG_PRINT("[ERROR] hAudioDacTaskQ post fail, Q=0x%X\r\n", hAudioDacTaskQ);
		}

	}
	else
	if(drv_l1_dac_dma_status_get() == 0)
	{
		if(!stopped)
		{
			audio_dac_msg = MSG_AUD_DMA_DBF_RESTART;
			if(xQueueSend(hAudioDacTaskQ, (void*)&audio_dac_msg, 0) != pdTRUE)
			{
				DBG_PRINT("[ERROR] hAudioDacTaskQ post fail, Q=0x%X\r\n", hAudioDacTaskQ);
			}
		}
	}

	return STATUS_OK;
}

static void audio_short_file_handle(void)
{
	INT32S	audio_dac_msg;
	INT32U	wb_idx;
	//INT8U	err;

	if (uxQueueMessagesWaiting(aud_send_q) >= 2)
	{
		// Turn on speaker
		ap_peripheral_speaker_ctrl(1);

		audio_dac_msg = MSG_AUD_DMA_DBF_START;
		if(xQueueSend(hAudioDacTaskQ, (void*)&audio_dac_msg, 0) != pdTRUE)
		{
			DBG_PRINT("[ERROR] hAudioDacTaskQ post fail, Q=0x%X\r\n", hAudioDacTaskQ);
		}

		return;
	}

	xQueueReceive(audio_wq, (void*)&wb_idx, portMAX_DELAY);
	gp_memset((INT8S *) pcm_out[wb_idx], 0, 32 * 2);
	pcm_len[wb_idx] = 32;
	xQueueSend(aud_send_q, (void*)&wb_idx, 0);

	// Turn on speaker
	ap_peripheral_speaker_ctrl(1);

	audio_dac_msg = MSG_AUD_DMA_DBF_START;
	if(xQueueSend(hAudioDacTaskQ, (void*)&audio_dac_msg, 0) != pdTRUE)
	{
		DBG_PRINT("[ERROR] hAudioDacTaskQ post fail, Q=0x%X\r\n", hAudioDacTaskQ);
	}

}

INT32S audio_wav_dec_play_init()
{
	INT32U	in_length;
	INT8U	ch;
	INT32S	count;
	INT32S	t_wi;

	audio_ctrl.file_cnt = 0;
	audio_ctrl.try_cnt = 10;
	audio_ctrl.read_secs = 0;
	audio_ctrl.file_end = 0;

	gp_memset((INT8S *) audio_ctrl.ring_buf, 0, RING_BUF_SIZE);
	wav_dec_init((INT8U *) audio_ctrl.work_mem, audio_ctrl.ring_buf);
	wav_dec_set_ring_buf_size((INT8U *)audio_ctrl.work_mem, audio_ctrl.ring_size);
	audio_ctrl.wi = wav_dec_get_ri((INT8U *) audio_ctrl.work_mem);
	audio_ctrl.ri = wav_dec_get_ri((INT8U *) audio_ctrl.work_mem);
	in_length = audio_ctrl.ri;

#if READING_BY_FILE_SRV == 1
	t_wi = audio_write_with_file_srv(audio_ctrl.ring_buf, audio_ctrl.wi, audio_ctrl.ri);

	/* check reading data */
	if(audio_check_wi(t_wi, &audio_ctrl.wi, 1) != AUDIO_ERR_NONE)
	{
		return AUDIO_ERR_READ_FAIL;
	}

#else
	audio_ctrl.wi = audio_write_buffer(audio_ctrl.ring_buf, audio_ctrl.wi, audio_ctrl.ri);
#endif // READING_BY_FILE_SRV == 1
	audio_ctrl.file_len = wav_dec_parsing((INT8U *) audio_ctrl.work_mem, audio_ctrl.wi);
	if((int) (audio_ctrl.file_len) <= 0)
	{
		return AUDIO_ERR_DEC_FAIL;
	}

	audio_ctrl.ri = wav_dec_get_ri((INT8U *) audio_ctrl.work_mem);
	audio_ctrl.file_cnt += (audio_ctrl.ri - in_length);

	if(in_length > audio_ctrl.ri)
	{
		audio_ctrl.file_cnt += WAV_DEC_BITSTREAM_BUFFER_SIZE;
	}

	in_length = wav_dec_get_SampleRate((INT8U *) audio_ctrl.work_mem);
	ch = wav_dec_get_nChannels((INT8U *) audio_ctrl.work_mem);
	bit_per_samp = wav_dec_get_wBitsPerSample((INT8U *) audio_ctrl.work_mem);

	drv_l1_dac_sample_rate_set(in_length);
	channel = ch;
	g_audio_sample_rate = in_length;
	if(ch == 1)
	{
		drv_l1_dac_mono_set();
	}
	else
	{
		drv_l1_dac_stereo_set();
	}

	total_play_time = audio_ctrl.file_len * 8 / bit_per_samp / g_audio_sample_rate / channel;

	//DBG_PRINT("channel: %d\r\n",ch);
	//DBG_PRINT("sample rate: %d\r\n",wav_dec_get_SampleRate((INT8U*)audio_ctrl.work_mem));
	//DBG_PRINT("file len = %d\r\n",audio_ctrl.file_len);
	for(count = 0; count < dac_buf_nums; count++)
	{
		xQueueSend(audio_wq, (void*)&count, 0);
	}

	return AUDIO_ERR_NONE;
}

INT32S audio_wav_dec_process()
{
	INT32S	pcm_point;
	INT32U	in_length;
	INT32S	tmp_len;
	INT8U	ch;
	INT32U	wb_idx;
	//INT8U	err;

#if READING_BY_FILE_SRV == 1
	INT8U	wait;
	INT32S	t_wi = 0;
#endif
	audio_ctrl.ri = wav_dec_get_ri((INT8U *) audio_ctrl.work_mem);

#if READING_BY_FILE_SRV == 1
	if(audio_ctrl.file_end == 0)
	{
		t_wi = audio_write_with_file_srv(audio_ctrl.ring_buf, audio_ctrl.wi, audio_ctrl.ri);
	}

#else
	audio_ctrl.wi = audio_write_buffer(audio_ctrl.ring_buf, audio_ctrl.wi, audio_ctrl.ri);
#endif

	xQueueReceive(audio_wq, (void*)&wb_idx, portMAX_DELAY);

#if READING_BY_FILE_SRV == 1
	if(audio_ctrl.file_end == 0)
	{
		wait = 0;
		if(t_wi == AUDIO_READ_WAIT)
		{
			wait = 1;	/* pend until read finish */
		}

		/* check reading data */
		if(audio_check_wi(t_wi, &audio_ctrl.wi, wait) != AUDIO_ERR_NONE)
		{
			return AUDIO_ERR_READ_FAIL;
		}
	}

#endif
	if(audio_ctrl.file_cnt >= audio_ctrl.file_len)
	{
		if(stopped)
		{
			xQueueSendToFront(audio_wq, (void*)&wb_idx, 0);
			audio_short_file_handle();
		}

		return AUDIO_ERR_DEC_FINISH;
	}
	else
	{
		in_length = audio_ctrl.wi - audio_ctrl.ri;

		if(audio_ctrl.wi < audio_ctrl.ri)
		{
			in_length += WAV_DEC_BITSTREAM_BUFFER_SIZE;
		}

		if(in_length > audio_ctrl.file_len - audio_ctrl.file_cnt)
		{
			tmp_len = audio_ctrl.wi;
			tmp_len -= (in_length - audio_ctrl.file_len + audio_ctrl.file_cnt);
			if(tmp_len < 0)
			{
				tmp_len += WAV_DEC_BITSTREAM_BUFFER_SIZE;
			}

			audio_ctrl.wi = tmp_len;
		}
	}

	if(reverse_play && (audio_ctrl.wi == audio_ctrl.ri))
	{
		pcm_point = 128;
		if(reverse_play_start == 0)
		{
			INT32S	audio_dac_msg;

			audio_dac_msg = MSG_AUD_DAC_REVRESE;
			if(xQueueSend(hAudioDacTaskQ, (void*)&audio_dac_msg, 0) != pdTRUE)
			{
				DBG_PRINT("[ERROR] hAudioDacTaskQ post fail, Q=0x%X\r\n", hAudioDacTaskQ);
			}

			reverse_play_start = 1;
		}

		reverse_points += pcm_point;
		tmp_len = pcm_point * (bit_per_samp / 8) * channel;
		if(audio_ctrl.file_cnt >= tmp_len)
		{
			audio_ctrl.file_cnt -= tmp_len;
		}
		else
		{
			return AUDIO_ERR_DEC_FINISH;
		}

		goto WAV_OK;
	}

	in_length = audio_ctrl.ri;
	pcm_point = wav_dec_run((INT8U *) audio_ctrl.work_mem, pcm_out[wb_idx], audio_ctrl.wi);

	if(pcm_point == -1)
	{
		return AUDIO_ERR_DEC_FAIL;
	}

	audio_ctrl.ri = wav_dec_get_ri((INT8U *) audio_ctrl.work_mem);
	audio_ctrl.file_cnt += (audio_ctrl.ri - in_length);

	if(in_length > audio_ctrl.ri)
	{
		audio_ctrl.file_cnt += WAV_DEC_BITSTREAM_BUFFER_SIZE;
	}

	if(pcm_point <= 0)
	{
		if(--audio_ctrl.try_cnt == 0)
		{
			return AUDIO_ERR_DEC_FAIL;
		}

		xQueueSendToFront(audio_wq, (void*)&wb_idx, 0);
		msgQSend(AudioTaskQ, MSG_AUD_DECODE_NEXT_FRAME, NULL, 0, MSG_PRI_NORMAL);
		return 0;
	}

WAV_OK:
	ch = wav_dec_get_nChannels((INT8U *) audio_ctrl.work_mem);
	pcm_len[wb_idx] = pcm_point * ch;

	xQueueSend(aud_send_q, (void*)&wb_idx, 0);
	audio_send_to_dma();
	msgQSend(AudioTaskQ, MSG_AUD_DECODE_NEXT_FRAME, NULL, 0, MSG_PRI_NORMAL);
	return 0;
}

static void audio_stop_unfinished(void)
{
#if AUDIO_PRIOR_DYNAMIC_SW == 1
	INT32S audio_dac_msg;

	audio_dac_msg = MSG_AUD_DAC_STOP;
	if(xQueueSend(hAudioDacTaskQ, (void*)&audio_dac_msg, 0) != pdTRUE)
	{
		DBG_PRINT("[ERROR] hAudioDacTaskQ post fail, Q=0x%X\r\n", hAudioDacTaskQ);
	}

#endif

	/* wait until dma finish */
	while(drv_l1_dac_dma_status_get() == 1)
	{
		vTaskDelay(1);
	}

#if AUDIO_PRIOR_DYNAMIC_SW == 1
	audio_dac_msg = MSG_AUD_LOW_PRIORITY_SET;
	if(xQueueSend(hAudioDacTaskQ, (void*)&audio_dac_msg, 0) != pdTRUE)
	{
		DBG_PRINT("[ERROR] hAudioDacTaskQ post fail, Q=0x%X\r\n", hAudioDacTaskQ);
	}

#endif
	if(audio_context_p->source_type == AUDIO_SRC_TYPE_FS && audio_ctrl.file_handle >= 0)
	{
		close(audio_ctrl.file_handle);
	}
	else
	if(audio_context_p->source_type == AUDIO_SRC_TYPE_APP_RS)
	{
		nv_lseek(audio_ctrl.file_handle, 0, SEEK_SET);
	}

	drv_l1_dac_timer_stop();

	/* release dma channel */
	drv_l1_dac_dbf_free();

	++fps_audio_dac_dbf_end;

	//dac_enable_set(FALSE);
	audio_context_p->state = AUDIO_PLAY_STOP;
}

static void audio_decode_next_frame(STAudioTaskPara *pAudioTaskPara)
{
	INT32S	ret;

	if(audio_context_p->state != AUDIO_PLAYING)
	{
		return;
	}

	ret = audio_context_p->fp_deocde();
	if(ret != 0)
	{
		audio_stop_unfinished();
		audio_send_result(MSG_AUD_PLAY_RES, ret);

#if PERFORMANCE_MEASURE == 1
		DBG_PRINT("\r\n");
		DBG_PRINT("MAX 1 = %d, frame index = %d, point = %d\r\n", cnt_max[0], cnt_max_fn[0], point[0]);
		DBG_PRINT("MAX 2 = %d, frame index = %d, point = %d\r\n", cnt_max[1], cnt_max_fn[1], point[1]);
		DBG_PRINT("MAX 3 = %d, frame index = %d, point = %d\r\n", cnt_max[2], cnt_max_fn[2], point[2]);
		DBG_PRINT("total count = %d\r\n", total_cnt);
		DBG_PRINT("total frame = %d\r\n", total_frame);
		DBG_PRINT("ave = %d\r\n", total_cnt / total_frame);
#endif
		if(decode_end != NULL)
		{
			decode_end(1);
		}
	}
}

void audio_send_result(INT32S res_type, INT32S result)
{
	aud_con.result_type = res_type;
	aud_con.result = result;
	aud_con.source_type = audio_context_p->source_type;

	//DBG_PRINT("audio_send_result :res_type =  %x,result = %d\r\n",res_type,result);
	app_msg_send(EVENT_APQ_ERR_MSG, (void *) &aud_con, sizeof(STAudioConfirm), MSG_PRI_NORMAL);
}

static void audio_start(STAudioTaskPara *pAudioTaskPara)
{
	INT32S	ret;
	INT32S audio_dac_msg;

	if(audio_context_p->state != AUDIO_PLAY_STOP)
	{
		audio_stop_unfinished();
	}

	stopped = 1;
	audio_queue_clear();

	audio_context_p->source_type = pAudioTaskPara->src_type;

	if(audio_context_p->source_type == AUDIO_SRC_TYPE_FS)
	{
		ret = audio_play_file_set(pAudioTaskPara->fd, audio_context_p, &audio_ctrl);
	}
	else
	{
		ret = AUDIO_ERR_NONE;
		audio_context_p->audio_format = pAudioTaskPara->audio_format;
		audio_ctrl.file_len = pAudioTaskPara->file_len;
		audio_ctrl.file_handle = pAudioTaskPara->fd;
	}

	if(ret == AUDIO_ERR_NONE)
	{
		audio_context_p->state = AUDIO_PLAYING;
		audio_ctrl.ring_size = RING_BUF_SIZE;
		audio_ctrl.reading = 0;
		dac_buf_nums = MAX_DAC_BUFFERS;

		switch(audio_context_p->audio_format)
		{
		case AUDIO_TYPE_WAV:
			audio_context_p->fp_deocde_init = audio_wav_dec_play_init;
			audio_context_p->fp_deocde = audio_wav_dec_process;
			audio_ctrl.ring_size = WAV_DEC_BITSTREAM_BUFFER_SIZE;
			audio_ctrl.frame_size = WAV_DEC_FRAMESIZE;
			dac_buf_nums = 4;
			reverse_play = 0;
			reverse_play_start = 0;
			reverse_points = 0;

			audio_dac_msg = MSG_AUD_DAC_REVRESE_CLEAR;
			if(xQueueSend(hAudioDacTaskQ, (void*)&audio_dac_msg, 0) != pdTRUE)
			{
				DBG_PRINT("[ERROR] hAudioDacTaskQ post fail, Q=0x%X\r\n", hAudioDacTaskQ);
			}

			break;

		default:
			audio_send_result(MSG_AUD_BG_PLAY_RES, AUDIO_ERR_INVALID_FORMAT);
			return;
		}

		if(audio_mem_alloc(audio_context_p->audio_format, &audio_ctrl, pcm_out) != AUDIO_ERR_NONE)
		{
			DBG_PRINT("audio memory allocate fail\r\n");
			audio_send_result(MSG_AUD_PLAY_RES, AUDIO_ERR_MEM_ALLOC_FAIL);
			return;
		}

		ret = audio_context_p->fp_deocde_init();
		if(ret != AUDIO_ERR_NONE)
		{
			audio_stop_unfinished();
			DBG_PRINT("audio play init failed\r\n");
		}
		else
		{
			audio_dac_msg = MSG_AUD_START_SAMPLES_COUNT;
			if(xQueueSend(hAudioDacTaskQ, (void*)&audio_dac_msg, 0) != pdTRUE)
			{
				DBG_PRINT("[ERROR] hAudioDacTaskQ post fail, Q=0x%X\r\n", hAudioDacTaskQ);
			}

			msgQSend(AudioTaskQ, MSG_AUD_DECODE_NEXT_FRAME, NULL, 0, MSG_PRI_NORMAL);
		}
	}
	else
	{
		ret = AUDIO_ERR_INVALID_FORMAT;
	}

	audio_send_result(MSG_AUD_PLAY_RES, ret);
}

static void audio_stop(STAudioTaskPara *pAudioTaskPara)
{
#if AUDIO_PRIOR_DYNAMIC_SW == 1
	INT32S audio_dac_msg;
#endif

	if((audio_context_p->state == AUDIO_PLAY_STOP) || (audio_context_p->state == AUDIO_IDLE))
	{
		audio_send_result(MSG_AUD_STOP_RES, AUDIO_ERR_NONE);
		return;
	}

#if AUDIO_PRIOR_DYNAMIC_SW == 1
	audio_dac_msg = MSG_AUD_DAC_STOP;
	xQueueSend(hAudioDacTaskQ, (void *) &audio_dac_msg, 0);
#endif
	if(audio_context_p->state == AUDIO_PLAYING)
	{
#if USE_RAMP_DOWN_RAPID == 0
		//audio_ramp_down();//ramp down speed normal
#else
		if(audio_context_p->audio_format == AUDIO_TYPE_AVI)
			audio_ramp_down();	//ramp down speed normal
		else
			audio_ramp_down_rapid();
#endif
	}

	audio_stop_unfinished();
	audio_send_result(MSG_AUD_STOP_RES, AUDIO_ERR_NONE);
}

/*== AVI audio ctrl Setting explain ==*/
// audio_ctrl.f_last => 1:it mean buffer read file over. 0: it mean buffer read file continue
// when audio play over ,the audio_ctrl.ri == audio_ctrl.wi.

/*== ==*/
static void audio_pause(STAudioTaskPara *pAudioTaskPara)
{
	INT32S audio_dac_msg;

	if(audio_context_p->state != AUDIO_PLAYING)
	{
		return;
	}

	audio_dac_msg = MSG_AUD_DMA_PAUSE;
	if(xQueueSend(hAudioDacTaskQ, (void*)&audio_dac_msg, 0) != pdTRUE)
	{
		DBG_PRINT("[ERROR] hAudioDacTaskQ post fail, Q=0x%X\r\n", hAudioDacTaskQ);
	}


	while(drv_l1_dac_dma_status_get() == 1)
	{
		vTaskDelay(1);
	}

	stopped = 1;
	drv_l1_dac_dbf_free();

	//dac_enable_set(FALSE);
#if AUDIO_PRIOR_DYNAMIC_SW == 1
	audio_dac_msg = MSG_AUD_LOW_PRIORITY_SET;
	if(xQueueSend(hAudioDacTaskQ, (void*)&audio_dac_msg, 0) != pdTRUE)
	{
		DBG_PRINT("[ERROR] hAudioDacTaskQ post fail, Q=0x%X\r\n", hAudioDacTaskQ);
	}
#endif
	audio_context_p->state = AUDIO_PLAY_PAUSE;
	audio_send_result(MSG_AUD_PAUSE_RES, AUDIO_ERR_NONE);
}

static void audio_resume(STAudioTaskPara *pAudioTaskPara)
{
	if(audio_context_p->state != AUDIO_PLAY_PAUSE)
	{
		return;
	}

	if(channel == 1)
	{
		drv_l1_dac_mono_set();
	}
	else
	{
		drv_l1_dac_stereo_set();
	}

	//dac_enable_set(TRUE);
	audio_context_p->state = AUDIO_PLAYING;

#if READING_BY_FILE_SRV == 1
	if(audio_ctrl.reading)
	{
		audio_check_wi(AUDIO_READ_WAIT, &audio_ctrl.wi, 1);
	}

#endif
	msgQSend(AudioTaskQ, MSG_AUD_DECODE_NEXT_FRAME, NULL, 0, MSG_PRI_NORMAL);
	audio_send_result(MSG_AUD_RESUME_RES, AUDIO_ERR_NONE);
}

static void audio_init(void)
{
	INT32U	ptr;

	ptr = (INT32U) & audio_dec_buf[0];
	audio_ctrl.ring_buf = (INT8U *) ptr;

	ptr += (RING_BUF_SIZE + GUARD_BUFFER + 3) &~0x3;
	pcm_buf = ptr;

	ptr += (WAV_PCM_BUF_SIZE * MAX_DAC_BUFFERS + 3) &~0x3;
	audio_ctrl.work_mem = (INT8S *) ptr;

	DBG_PRINT("audio memory = %d\r\n", sizeof(audio_dec_buf));
	gp_memset(audio_ctrl.work_mem, 0, WAV_DEC_MEMORY_SIZE);
	DBG_PRINT("decode memory -- 0x%x\r\n", audio_ctrl.work_mem);

	audio_ctrl.ring_size = RING_BUF_SIZE;
	audio_ctrl.wi = 0;
	audio_ctrl.ri = 0;
	audio_context_p->state = AUDIO_PLAY_STOP;
	stopped = 1;

	decode_end = NULL;
}

void audio_task_init(void)
{
	DBG_PRINT("%s ... \r\n", __func__);

	/* Create MsgQueue/MsgBox for TASK */
	audio_wq = xQueueCreate(AUDIO_WRITE_Q_SIZE, sizeof(void*));
	audio_fsq = xQueueCreate(AUDIO_FS_Q_SIZE, sizeof(void*));
}

void
audio_dec_task_init(
	void
)
{
	TASK_APP_ARGUMENT* pAudioDecTaskArgs = &pTurnkeyArgs->taskAudioDec;
	pAudioDecTaskArgs->taskRecvBufLen = AUDIO_PARA_MAX_LEN;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pAudioDecTaskArgs->taskAppQ == 0) {
		pAudioDecTaskArgs->taskAppQ = msgQCreate(pAudioDecTaskArgs->taskMsgLen, pAudioDecTaskArgs->taskMsgLen, pAudioDecTaskArgs->taskRecvBufLen);
		if(pAudioDecTaskArgs->taskAppQ == 0) {
			ERR_PRINT("fail to create pAudioDecTaskArgs->taskAppQ \r\n");
			goto ERR_AUDIO_DEC_TASK_INIT;
		}

		if(pAudioDecTaskArgs->taskRecvBufLen)
		{
			pAudioDecTaskArgs->taskRecvBuf = (void*)malloc(pAudioDecTaskArgs->taskRecvBufLen);
			if(pAudioDecTaskArgs->taskRecvBuf == 0) {
				ERR_PRINT("fail to malloc pAudioDecTaskArgs->taskRecvBuf \r\n");
				goto ERR_AUDIO_DEC_TASK_INIT;
			}
		}
	}
	return;

ERR_AUDIO_DEC_TASK_INIT:
	if(pAudioDecTaskArgs->taskAppQ != 0)
	{
		msgQDelete(pAudioDecTaskArgs->taskAppQ);
		pAudioDecTaskArgs->taskAppQ = 0;
	}

	if(pAudioDecTaskArgs->taskRecvBuf)
	{
		free(pAudioDecTaskArgs->taskRecvBuf);
		pAudioDecTaskArgs->taskRecvBuf = 0;
	}
	return;
}

void
audio_dec_task_entry(
	void const *arg
)
{
	TASK_APP_ARGUMENT* pAudioDecTaskArgs;
	STAudioTaskPara* pstAudioTaskPara;
	INT32U AudioTaskPara_len;
	INT32U	msg_id;

 	pAudioDecTaskArgs = &pTurnkeyArgs->taskAudioDec;
	pstAudioTaskPara = (STAudioTaskPara*)pAudioDecTaskArgs->taskRecvBuf;
	AudioTaskPara_len = pAudioDecTaskArgs->taskRecvBufLen;

	audio_task_init();
	audio_init();

	DBG_PRINT("%s ... \r\n", __func__);
	while(1)
	{
		/* Pend task message */
		if(msgQReceive(AudioTaskQ, &msg_id, pstAudioTaskPara, AudioTaskPara_len) == STATUS_FAIL)
		{
			continue;
		}

		switch(msg_id)
		{
		case MSG_AUD_PLAY:	/* by file handle */
		case MSG_AUD_PLAY_BY_SPI:
			audio_start(pstAudioTaskPara);
			break;

		case MSG_AUD_STOP:
			audio_stop(pstAudioTaskPara);
			break;

		case MSG_AUD_PAUSE:
			audio_pause(pstAudioTaskPara);
			break;

		case MSG_AUD_RESUME:
			audio_resume(pstAudioTaskPara);
			break;

		case MSG_AUD_SET_MUTE:
			audio_mute_set(pstAudioTaskPara);
			break;

		case MSG_AUD_REVERSE_SET:
			audio_reverse_set(pstAudioTaskPara);
			break;

		case MSG_AUD_PLAY_SPEED_SET:
			audio_play_speed_set(pstAudioTaskPara);
			break;

		case MSG_AUD_VOLUME_SET:
			audio_volume_set(pstAudioTaskPara);
			break;

		case MSG_AUD_DECODE_NEXT_FRAME:
			audio_decode_next_frame(pstAudioTaskPara);
			break;

		case MSG_AUD_PLAY_TIME_GET_START:
			start_time_send = 1;
			break;

		case MSG_AUD_PLAY_TIME_GET_END:
			start_time_send = 0;
			break;
		default:
			break;
		}
	}
}

INT32S
audio_dec_task_msg_send(
    INT32U msg, void *para, INT32U bytes, INT32U prio
)
{
    INT32S nRet;

    if(AudioTaskQ == NULL)
    {
        nRet = -1;
        ERR_PRINT("send msg fail\r\n");
   }
    else
    {
        nRet = msgQSend(AudioTaskQ, msg, para, bytes, prio);
    }
    return nRet;
}


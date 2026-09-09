#ifndef __TASK_AUDIO_WAV_DECODER_H__
#define __TASK_AUDIO_WAV_DECODER_H__

#define RING_BUF_SIZE			WAV_DEC_BITSTREAM_BUFFER_SIZE
#define WAV_PCM_BUF_SIZE		WAV_DEC_FRAMESIZE * 2

#define AUDIO_IDLE				0
#define AUDIO_PLAYING			1
#define AUDIO_PLAY_PAUSE		2
#define AUDIO_PLAY_STOP			3

#define AUDIO_READ_FAIL			- 2
#define AUDIO_READ_PEND			- 3
#define AUDIO_READ_WAIT			- 4

#define ID3_TAG_NONE			0
#define ID3_TAG_V1				1
#define ID3_TAG_V2				2
#define ID3_TAG_V2_FOOTER		3
#define ID3_TAG_FLAG_FOOTER		0x10

#define AUDIO_PARSE_FAIL		- 1
#define AUDIO_PARSE_SUCCS		0
#define AUDIO_MAX_TYPE			4
#define AUDIO_PASER_BUFFER_SIZE (2048 * 10)

typedef struct
{
	AUDIO_TYPE	audio_file_real_type;
	INT8S (*parse_audio_file_head) (INT8S * p_audio_file_head);
}

AUDIO_FILE_PARSE;

typedef struct
{
	INT8S	mpeg_version;
	INT8S	layer;
	INT8S	sample_rate;
} MP3_FILE_HEAD;

typedef struct
{
	INT8U	state;
	INT8U	source_type;
	INT8U	curr_vol;
	INT32U	audio_format;
	INT32S (*fp_deocde_init) ();
	INT32S (*fp_deocde) ();
}

AUDIO_CONTEXT, *AUDIO_CONTEXT_P;

extern QueueHandle_t	audio_wq;
extern INT16S *pcm_out[MAX_DAC_BUFFERS];
extern INT32U pcm_len[MAX_DAC_BUFFERS];
extern INT8U channel;
extern INT32U g_audio_sample_rate;
extern INT32U total_play_time;
extern INT8U start_time_send;
extern INT32U dac_buf_nums;

extern void (*decode_end) (INT32U audio_decoder_num);

extern void		audio_send_result(INT32S res_type, INT32S result);
extern INT32S	audio_play_file_set(INT16S fd, AUDIO_CONTEXT *aud_context, AUDIO_CTRL *aud_ctrl);
extern void		audio_dec_task_entry(void const *arg);

extern void     audio_dec_task_init(void);
extern INT32S   audio_dec_task_msg_send(INT32U msg, void *para, INT32U bytes, INT32U prio);
#endif // __TASK_AUDIO_WAV_DECODER_H__

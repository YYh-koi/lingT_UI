#ifndef __TASK_AUDIO_WAV_DAC_H__
#define __TASK_AUDIO_WAV_DAC_H__

extern QueueHandle_t	hAudioDacTaskQ;
extern QueueHandle_t	aud_send_q;
extern QueueHandle_t	aud_avi_q;

extern void audio_dac_task_entry(void const *para);
#endif // __TASK_AUDIO_WAV_DAC_H__

#ifndef __AP_MUSIC_H__
#define __AP_MUSIC_H__

#define MAX_VOLUME_LEVEL	8		/* 0~16 */
#define VOLUME_STEP			4

extern STAudioTaskPara	music_para;

extern void				ap_music_init(void);
extern void				ap_music_effect_resource_init(void);	//wwj add
extern void				audio_res_play_process(INT32S result);
extern void				audio_res_resume_process(INT32S result);
extern void				audio_res_pause_process(INT32S result);
extern void				audio_res_stop_process(INT32S result);
extern void				audio_mute_ctrl_set(BOOLEAN status);
extern void				audio_vol_inc_set(void);
extern void				audio_vol_dec_set(void);
extern INT8U			audio_fg_vol_get(void);
extern void				audio_vol_set(INT8U vol);
extern INT8U			audio_playing_state_get(void);
extern void				audio_confirm_handler(STAudioConfirm *aud_con);
extern void				ap_music_reset(void);
extern INT32S			audio_effect_play(INT32U effect_type);
extern void				audio_effect_wait(void);
extern void				audio_wav_play(INT16S fd);
extern INT32S			audio_wav_pause(void);
extern INT32S			audio_wav_resume(void);
extern void				audio_wav_stop(void);

extern void				ap_state_audio_play_speed_set(INT8U speed);
extern void				ap_state_audio_reverse_set(INT8U reverse);
extern void				audio_send_resume(void);
extern void				audio_send_pause(void);
#endif // __AP_MUSIC_H__

#include "state_audio_record.h"
#include "audio_record.h"

#define AUDIO_RECORD_DISPLAY_TIMER_INTERVAL	32//16 //wwj modify

#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
extern INT8S ap_audio_record_init(INT32U prev_state);
extern void ap_audio_record_exit(INT32U next_state_msg);
extern INT8U ap_audio_record_sts_get(void);
extern void ap_audio_record_func_key_active(void);
extern INT8S ap_audio_record_reply_action(STOR_SERV_FILEINFO *file_info_ptr);
extern void ap_audio_record_sts_set(INT8S sts);
extern void ap_audio_record_timer_start(void);
extern void ap_audio_record_timer_stop(void);
extern void ap_audio_record_timer_draw(void);

#endif

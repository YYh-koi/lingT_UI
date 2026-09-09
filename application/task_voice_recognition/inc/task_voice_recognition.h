#ifndef __TASK_VOICE_RECOGNITION_H__
#define __TASK_VOICE_RECOGNITION_H__

extern void
voice_recg_state_task_init(
	void
);

extern void 
voice_recg_task_entry(
	void const *para
);

extern INT32S
voice_recg_task_process(
	INT32U p_pcm
);

#endif // __TASK_VOICE_RECOGNITION_H__

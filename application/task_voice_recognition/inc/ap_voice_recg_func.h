#ifndef __TASK_VOICE_RECG_FUNC_H__
#define __TASK_VOICE_RECG_FUNC_H__

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern INT32S
ap_voice_recg_start(
	void
);

extern INT32S
ap_voice_recg_stop(
	void
);

extern INT32S
ap_voice_recg_process(
	short* samples, 
	int n
);

#endif // __TASK_VOICE_RECG_FUNC_H__

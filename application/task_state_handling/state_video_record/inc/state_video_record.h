#ifndef __STATE_VIDEO_RECORD_H__
#define __STATE_VIDEO_RECORD_H__

extern INT8S state_video_record_init(void);
extern void state_video_record_exit(INT8U exit_flag);
extern void state_video_record_entry(void *para);
extern INT8U state_record_MD_freeze_status_get(void);
extern void state_record_MD_freeze_release(void);

extern PREVIEW_MODE preview_mode;


#endif // __STATE_VIDEO_RECORD_H__

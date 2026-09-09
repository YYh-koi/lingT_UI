#ifndef __AP_VIDEO_RECORD_H__
#define __AP_VIDEO_RECORD_H__

// Motion detection
#define MOTION_DETECT_CHECK_TIME_INTERVAL	(128)

// when MOTION_DETECT_STOP_TIME seconds detect no motion then stop video record
#define MOTION_DETECT_STOP_TIME						(10)

extern INT8S g_lock_current_file_flag;
extern INT8U g_cycle_record_continuing_flag;

extern INT8S	ap_video_record_init(void);
extern INT8S	ap_video_record_init_TP(void);
extern void		ap_video_record_exit(void);
extern INT8S	ap_video_record_func_key_active(void);
extern void		ap_video_record_start(void);
extern void		ap_video_record_sts_set(INT8S sts);
extern INT8U	ap_video_record_sts_get(void);
extern void		ap_video_record_reply_action(STOR_SERV_FILEINFO *file_info_ptr);

extern void		ap_video_record_lock_current_file(void);

extern void		ap_video_record_show_left_recording_time(void);
extern void		ap_video_record_clear_left_recording_time(void);

extern void		ap_video_record_clear_park_mode(void);
extern void		ap_video_record_show_park_mode(void);
extern void		ap_video_record_clear_resolution(void);
extern void		ap_video_record_show_resolution(void);

#if C_MOTION_DETECTION
extern INT8U    motion_detect_timerid;
extern void		ap_video_record_md_icon_clear_all(void);

extern void		ap_video_record_md_tick(INT8U *md_tick);
extern INT8S	ap_video_record_md_active(INT8U *md_tick);
extern void		ap_video_record_md_disable(void);
#endif // #if C_MOTION_DETECTION

#if C_CYCLIC_VIDEO_RECORD
extern void		ap_video_record_cycle_reply_action(STOR_SERV_FILEINFO *file_info_ptr);
#endif // #if C_CYCLIC_VIDEO_RECORD

extern DISPLAY_ICONSHOW *allocate_and_draw_ascii_string_icon(const CHAR *string_in, INT16U color);

extern void ap_video_record_get_jpeg(STOR_SERV_FILEINFO *file_info_ptr);
extern void ap_video_record_get_jpeg_write_file(INT8U src,	SNAPSHOT_INFO *snapshot_info_ptr);

#endif // __AP_VIDEO_RECORD_H__

#ifndef __AP_PHOTO_CAPTURE_H__
#define __AP_PHOTO_CAPTURE_H__

#define AVI_FRAME_RATE		25	// Frame per second
#define AVI_FRAME_RATE_REAR	25	// Frame per second

extern INT32U	ap_capture_left_num_get(void);

extern void		ap_capture_init(void);
extern void		ap_capture_init_TP(void);
extern void		ap_capture_exit(void);
extern void		ap_capture_exit_TP(void);

extern INT32S	ap_capture_func_key_active(void);
extern INT32S	ap_capture_reply_action(STOR_SERV_FILEINFO *file_info_ptr);
extern void		ap_capture_reply_done(INT8U ret, INT32U file_path_addr);
extern void     ap_capture_cdsp_config(void);

extern void		ap_capture_show_left_num(INT32U left_num);
extern INT32U	ap_capture_left_num_calculate(void);
extern void		ap_capture_clear_left_num(void);
extern void		ap_capture_show_resolution(void);
extern void		ap_capture_clear_resolution(void);

#endif // __AP_PHOTO_CAPTURE_H__

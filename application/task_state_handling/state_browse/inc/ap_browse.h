/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
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
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park,                      *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#ifndef __AP_BROWSE_H__
#define __AP_BROWSE_H__

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define BROWSE_DISPLAY_TIMER_INTERVAL	16

enum browse_state
{
	BROWSE_IDLE				= 0,
	BROWSE_SD_NOT_EXIST		= (1 << 0),
	BROWSE_DECODE_BUSY		= (1 << 1),
	BROWSE_PLAYBACK_BUSY	= (1 << 2),
	BROWSE_PLAYBACK_PAUSE	= (1 << 3),
	BROWSE_OPEN_FILE_NOW	= (1 << 4)
};

typedef enum{
    PLAY_NONE,
    PLAY_NOW,
    PLAY_PAUSE,
    PLAY_RESUME
}BROWSE_PLAY_STATE;

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void ap_browse_display_update_command(void);
extern void ap_browse_fast_play_icon_show(INT8S play_speed);
extern INT32S ap_browse_wav_play(IMAGE_DECODE_STRUCT *img_ptr);
extern INT32S ap_browse_wav_stop(STOR_SERV_PLAYINFO *info_ptr);

extern void ap_browse_init(INT32U prev_state, INT16U play_index);
extern void ap_browse_exit(INT32U next_state_msg);

extern INT32S ap_browse_sts_get(void);
extern void ap_browse_sts_set(INT32S sts);
extern void ap_browse_func_key_active(void);
extern INT32S ap_browse_open_file_req(void* fileIdx);
extern INT8S ap_browse_stop_handle(void);
extern void ap_browse_volume_icon_clear_all(void);
extern void ap_browse_no_media_show(INT16U str_type);
extern void ap_browse_mjpeg_stop(void);
extern void ap_browse_mjpeg_play_end(void);
extern void ap_browse_reply_action(STOR_SERV_PLAYINFO *info_ptr);
extern void ap_browse_next_key_active(INT8U err_flag);
extern INT8U ap_browse_get_curr_file_type(void);
extern void ap_browse_prev_key_active(INT8U err_flag);
extern void ap_browse_display_timer_start(void);
extern void ap_browse_display_timer_stop(void);

extern INT32S ap_browse_reply_get_first_frame(STOR_SERV_PLAYINFO *info_ptr, INT8U en_get_first, INT32U out_buf, INT16U out_w, INT16U out_h);
extern void ap_browse_decode_file_date_time_1(INT32U dos_date_time,INT16U dos_time_ms, TIME_T *time_info);




extern void ap_browse_is_file_exist(INT32U *picCnt, INT32U *vidCnt);		/* customer API for searching file */


#endif // #ifndef __AP_BROWSE_H__

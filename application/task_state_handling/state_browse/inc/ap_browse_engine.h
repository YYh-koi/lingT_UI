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
#ifndef __AP_BROWSE_ENGINE_H__
#define __AP_BROWSE_ENGINE_H__

#include "application.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
enum{
	DISP_ON_TFT = 0,
	DISP_ON_WIFI,
	DISP_ON_MAX
};

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct BROWSE_STREAM_CTRL_S{
	INT32S	(*stream_show_func)(DISP_P disp_frame);
	INT32S	(*stream_get_first_func)(IMAGE_DECODE_STRUCT *img_ptr);
	INT32S	(*stream_play_func)(IMAGE_DECODE_STRUCT *img_ptr);
	INT32S	(*stream_stop_func)(STOR_SERV_PLAYINFO *info_ptr);
	INT32S	(*stream_pause_func)();
	INT32S	(*stream_resume_func)();
}BROWSE_STREAM_CTRL_T;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern BROWSE_STREAM_CTRL_T*
ap_browse_ctrl_get(
	INT8U ctrl_idx
);



#endif // __AP_BROWSE_ENGINE_H__

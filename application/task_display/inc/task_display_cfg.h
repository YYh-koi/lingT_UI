/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2022 by Generalplus Inc.                         *
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
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#ifndef __TASK_DISPLAY_CFG_H__
#define __TASK_DISPLAY_CFG_H__
#include "customer.h"
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#if (H264ENC_FBC_ENABLE && C_VID_INTERPOLATING_2K)
#define TFT_BUF_NUM				3
#else
#define TFT_BUF_NUM				4
#endif
#define ROTATE_BUF_NUM			1
#define WIFI_SYMBOL_BUF_NUM     1
#define DISPLAY_BUF_NUM			(TFT_BUF_NUM + ROTATE_BUF_NUM + WIFI_SYMBOL_BUF_NUM)

#endif //__TASK_DISPLAY_CFG_H__

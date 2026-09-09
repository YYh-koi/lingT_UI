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
#ifndef __DRV_L2_ROTATOR_H__
#define __DRV_L2_ROTATOR_H__

#include "define.h"

// rotator error
#define C_ROTATOR_DEVICE_SUPPORT_ERR					(-1)
#define C_ROTATOR_COLOR_SUPPORT_ERR				        (-2)
#define C_ROTATOR_SIZE_SUPPORT_ERR			            (-3)
#define C_ROTATOR_MODE_SUPPORT_ERR				        (-4)
#define C_ROTATOR_BUFFER_ERR				            (-5)

typedef struct RotatorFormat_s
{
	/* rotator image para */
	INT32U rotator_color_format;			/* rotator color format*/
	INT16U rotator_width;				    /* 1~0xFFF, rotator image x size */
	INT16U rotator_height;			        /* 1~0xFFF, rotator image y size */

	INT32U rotator_src_addr;			    /* rotator source address, must be 4-align */
    INT32U rotator_tar_addr;			    /* rotator target address, must be 4-align */
    INT32U rotator_mode_selection;			/* rotator function mode */
    INT32U rotator_theta_burst;			    /* rotator theta burst */
    void (*isr_callback)(INT8U rotator_number, INT32U rotator_event);   /* scaler callback function */
} RotatorFormat_t;

extern void drv_l2_rotator_init(INT8U rotator_dev);
extern INT32S drv_l2_rotator_once(INT8U rotator_dev, INT8U wait_done, RotatorFormat_t *rot_info);
extern INT32S drv_l2_rotator_fd_angle(INT8U rotator_dev, INT8U wait_done, RotatorFormat_t *rot_info, INT16S angle, INT32U clockwise);
extern void drv_l2_rotator_uninit(INT8U rotator_dev);
extern INT32S drv_l2_rotator_wait_done(INT8U rotator_dev);

#endif //__DRV_L2_ROTATOR_H__

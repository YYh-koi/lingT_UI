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
#ifndef __GP_PAINT_H__
#define __GP_PAINT_H__

#include "typedef.h"
#include "drv_l2.h"

void gp_paint_dot(INT16U *buf, INT16U buf_w, INT16U buf_h, DISP_FMT fmt, INT16U pos_x, INT16U pos_y, INT16U color);
INT8U gp_paint_line(INT16U *buf, INT16U buf_w, INT16U buf_h, DISP_FMT fmt, INT32S start_x, INT32S start_y, INT32S end_x, INT32S end_y, INT16U color);
void gp_paint_circle(INT16U *buf, INT16U buf_w, INT16U buf_h, DISP_FMT fmt, INT32S pos_x, INT32S pos_y, INT16U radius, INT8U solid, INT16U color);
void gp_paint_rect(INT16U *buf, INT16U buf_w, INT16U buf_h, DISP_FMT fmt, INT32S start_x, INT32S start_y, INT32S width, INT32S height, INT16U thickness, INT8U solid, INT16U color);

#endif

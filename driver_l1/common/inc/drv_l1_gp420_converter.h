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

#ifndef __drv_l1_GP420_CONV_H__
#define __drv_l1_GP420_CONV_H__

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/
#define GP420_CONV_ADDR(x)	(0x60000000)
#define GP420_CONV_SRC		(0x60000000)
#define GP420_CONV_DST		(0x68000000)

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern INT16S drv_l1_gp420_conv_clock_enable(INT8U enable);
extern void drv_l1_gp420_conv_src_addr_set(INT32U addr);
extern void drv_l1_gp420_conv_dst_addr_set(INT32U addr);
extern int 	drv_l1_gp420_conv_width_set(INT32U width);
extern void drv_l1_gp420_conv_rgb_format_enable(INT32U enable);
extern void drv_l1_gp420_conv_yuv_format_set(INT32U mode);

#endif		// __drv_l1_GP420_CONV_H__

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
#ifndef __BUS_PRIORITY_CFG_H__
#define __BUS_PRIORITY_CFG_H__

#include "drv_l2.h"

/**************************************************************************
 *             B U S	P R I O R I T Y		C O N F I G                   *
 **************************************************************************/
BUS_PRY_ARGS Bus_Pry_Cfg[] =
{
	// Number, Waiting Count(Bit6~0), Enable Preempt Mode(Bit7)

    /* 3DNR Enable */
    #if C_VID_INTERPOLATING_2K
    {BUS_PRY_M56,	0x70, 	1},
    #else
    {BUS_PRY_M17, 	0x30, 	1},
	{BUS_PRY_M54,	0x30, 	1},
	{BUS_PRY_M56,	0x60, 	1},
    #endif

	#if LCD_DRV_EN_ST7703_QX550H4001
	{BUS_PRY_M51,	0x01, 	0},
	#endif

	#if LCD_DRV_EN_WTL096602G02
	{BUS_PRY_M51,	0x01, 	0},
	#endif

    #if LCD_DRV_EN_CK69502S0_FL7703N
	{BUS_PRY_M51,	0x01, 	0},
	#endif

    #if C_VID_INTERPOLATING_2K && LCD_DRV_EN_WTL096602G02
    {BUS_PRY_M8,	0x30, 	1},
	{BUS_PRY_M9,	0x30, 	1},
	{BUS_PRY_M32,	0x30, 	1},
	#endif

};

#endif //__BUS_PRIORITY_CFG_H__

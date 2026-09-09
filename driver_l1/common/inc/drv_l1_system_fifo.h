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
#ifndef __drv_l1_SYS_FIFO_H__
#define __drv_l1_SYS_FIFO_H__

#include "drv_l1.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
typedef enum
{
	SYS_FIFO_0, // Map 0x0XXX_XXXX to 0x3XXX_XXXX
	SYS_FIFO_1, // Map 0x0XXX_XXXX to 0x4XXX_XXXX
	SYS_FIFO_2, // Map 0x0XXX_XXXX to 0x5XXX_XXXX
	SYS_FIFO_3, // Map 0x0XXX_XXXX to 0x6XXX_XXXX
	SYS_FIFO_4, // Map 0xF8XX_XXXX to 0xF9XX_XXXX
	SYS_FIFO_5, // Map 0xFAXX_XXXX to 0xFBXX_XXXX
	SYS_FIFO_MAX
}SYS_FIFO_ENUM;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern INT32S
drv_l1_system_fifo_register(
	SYS_FIFO_ENUM fifo_num
);

extern INT32S
drv_l1_system_fifo_release(
	SYS_FIFO_ENUM fifo_num
);
 
extern INT32U 
drv_l1_system_fifo_mapping(
	SYS_FIFO_ENUM fifo_num,
	INT32U buffer
);

extern INT32U 
drv_l1_system_fifo_unmapping(
	SYS_FIFO_ENUM fifo_num,
	INT32U buffer
);

extern void 
drv_l1_system_fifo_used_get(
	void
);

#endif		// __drv_l1_SYS_FIFO_H__

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
#ifndef __DRV_L1_BUS_PRIORITY_H__
#define __DRV_L1_BUS_PRIORITY_H__

#include "drv_l1.h"

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
	BUS_PRY_M1,
	BUS_PRY_M2,
	BUS_PRY_M3,
	BUS_PRY_M4,
	BUS_PRY_M5,
	BUS_PRY_M6,
	BUS_PRY_M7,
	BUS_PRY_M8,
	BUS_PRY_M9,
	BUS_PRY_M10,
	BUS_PRY_M11,
	BUS_PRY_M12,
	BUS_PRY_M13,
	BUS_PRY_M14,
	BUS_PRY_M15,
	BUS_PRY_M16,
	BUS_PRY_M17,
	BUS_PRY_M18,
	BUS_PRY_M19,
	BUS_PRY_M20,
	BUS_PRY_M21,
	BUS_PRY_M22,
	BUS_PRY_M23,
	BUS_PRY_M24,
	BUS_PRY_M25,
	BUS_PRY_M26,
	BUS_PRY_M27,
	BUS_PRY_M28,
	BUS_PRY_M29,
	BUS_PRY_M30,
	BUS_PRY_M31,
	BUS_PRY_M32,
	BUS_PRY_M33,
	BUS_PRY_M34,
	BUS_PRY_M35,
	BUS_PRY_M36,
	BUS_PRY_M37,
	BUS_PRY_M38,
	BUS_PRY_M39,
	BUS_PRY_M40,
	BUS_PRY_M41,
	BUS_PRY_M42,
	BUS_PRY_M43,
	BUS_PRY_M44,
	BUS_PRY_M45,
	BUS_PRY_M46,
	BUS_PRY_M47,
	BUS_PRY_M48,
	BUS_PRY_M49,
	BUS_PRY_M50,
	BUS_PRY_M51,
	BUS_PRY_M52,
	BUS_PRY_M53,
	BUS_PRY_M54,
	BUS_PRY_M55,
	BUS_PRY_M56,
	BUS_PRY_M57,
	BUS_PRY_M58,
	BUS_PRY_M59,
	BUS_PRY_M60,
	BUS_PRY_M61,
	BUS_PRY_M62,
	BUS_PRY_M63
}BUS_PRIORITY_ENUM;

typedef struct BUS_PRY_ARGS_S
{
	BUS_PRIORITY_ENUM BP_num;
	INT8U waiting_count; // Must be set 0x1 ~ 0x7F
	INT8U enable_preempt;
}BUS_PRY_ARGS;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void
drv_l1_bus_priority_set(
	BUS_PRY_ARGS* pBusPryArgs
);

#endif //__DRV_L1_BUS_PRIORITY_H__

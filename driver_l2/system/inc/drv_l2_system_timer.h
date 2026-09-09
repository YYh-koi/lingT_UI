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
#ifndef __DRV_L2_SYSTEM_TIMER_H__
#define __DRV_L2_SYSTEM_TIMER_H__

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define MAX_SYSTEM_TIMER		7
#define MAX_SYSTEM_TIMER_ISR	5

#define TIMER0_ENABLE		0x1
#define TIMER1_ENABLE		0x2
#define TIMER2_ENABLE		0x4
#define TIMER3_ENABLE		0x8
#define TIMER4_ENABLE		0x10
#define TIMER5_ENABLE		0x20
#define TIMER6_ENABLE		0x40

#define USER_TIMER0_ENABLE	0x080
#define USER_TIMER1_ENABLE	0x100
#define USER_TIMER2_ENABLE	0x200
#define USER_TIMER3_ENABLE	0x400

#define TIMER_ID_INVALID 	-1

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef INT32S (*fp_msg_qsend)(void*, INT32U, void*, INT32U, INT32U);

typedef struct
{
	fp_msg_qsend	msg_qsend;
	void			*msg_q_id;
	INT32U			msg_id;
	INT32U			Interval;
	INT32U			CurrentTick;
} STSYSTIMER;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void
drv_l2_sys_timer_init(
	void
);

extern INT32S
drv_l2_sys_timer_set(
	void* msg_qsend,
	void* msg_q_id,
	INT32U msg_id,
	INT32U timerid,
	INT32U interval
);

extern INT32S
drv_l2_sys_timer_kill(
	INT32U timerid
);

extern INT32S
drv_l2_sys_timer_all_kill(
	void
);

extern INT32S
drv_l2_sys_timer_128Hz_init(
	void
);

extern INT32S
drv_l2_sys_timer_isr_register(
	void (*TimerIsrFunction)(void)
);

extern INT32S
drv_l2_sys_timer_isr_release(
	void (*TimerIsrFunction)(void)
);

#endif		// __DRV_L2_SYSTEM_TIMER_H__

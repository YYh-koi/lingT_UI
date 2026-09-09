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
#include "application.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define SYS_SOFTWARE_TIMER_STACK_SIZE		1024
#define SYS_SOFTWARE_TIMER_TASK_QUEUE_MAX	64

typedef enum
{
	TIMER0_END		= 0x00010000,
	TIMER1_END		= 0x00020000,
	TIMER2_END		= 0x00030000,
	TIMER3_END		= 0x00040000,
	TIMER4_END		= 0x00050000,
	TIMER5_END		= 0x00060000,
	TIMER6_END		= 0x00070000,
	TIMER7_END		= 0x00080000,
	TIMER8_END		= 0x00090000,
	TIMER9_END		= 0x000A0000,
	TIMER10_END		= 0x000B0000
}ENUM_SYS_SOFT_TIMER;

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
static void (*gTimeBaseC_ISR[MAX_SYSTEM_TIMER_ISR])(void);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static STSYSTIMER	gstSysTimer[MAX_SYSTEM_TIMER];
static INT32U		gstSysTimerEnable;
static QueueHandle_t SysSoftTmrTaskQ = NULL;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static void
TimeBaseC_ISR(
	void
)
{
#if MAX_SYSTEM_TIMER > 0
	BaseType_t taskWoken = pdFALSE;
#endif
	INT32S	i;
	// execute use registe isr
	for(i = 0; i < MAX_SYSTEM_TIMER_ISR; i++)
	{
		if(gTimeBaseC_ISR[i] != NULL)
			gTimeBaseC_ISR[i]();
	}

	if(gstSysTimerEnable == 0)
		return;

#if MAX_SYSTEM_TIMER > 0
	for(i = 0; i < MAX_SYSTEM_TIMER; i++)
	{
		BaseType_t tmp_taskWoken = pdFALSE;

		if(gstSysTimerEnable & (TIMER0_ENABLE<<i) )
		{
			gstSysTimer[i].CurrentTick--;
			if(gstSysTimer[i].CurrentTick == 0)
			{
				void* tmp = 0;
				switch(i)
				{
					case 0:		tmp = (void*)TIMER0_END;	break;
					case 1:		tmp = (void*)TIMER1_END;	break;
					case 2:		tmp = (void*)TIMER2_END;	break;
					case 3:		tmp = (void*)TIMER3_END;	break;
					case 4:		tmp = (void*)TIMER4_END;	break;
					case 5:		tmp = (void*)TIMER5_END;	break;
					case 6:		tmp = (void*)TIMER6_END;	break;
					default:	return;						break;
				}

				xQueueSendFromISR(SysSoftTmrTaskQ, &tmp, &tmp_taskWoken);
				if(tmp_taskWoken != pdFALSE)
					taskWoken = pdTRUE;

				gstSysTimer[i].CurrentTick = gstSysTimer[i].Interval;
			}
		}
	}
	portEND_SWITCHING_ISR(taskWoken);
#endif
}

static void
task_system_software_timer(
	void const *param
)
{
	INT32U msg_id;

	while(1)
	{
		BaseType_t ret = pdFALSE;
		INT32S timer_id = -1;

		msg_id = 0;
		ret = xQueueReceive(SysSoftTmrTaskQ, &msg_id, portMAX_DELAY);
		if((ret != pdTRUE) || (!msg_id))
		{
			continue;
		}
#if MAX_SYSTEM_TIMER > 0
		switch (msg_id)
		{
			case TIMER0_END:	timer_id = 0;	break;
			case TIMER1_END:	timer_id = 1;	break;
			case TIMER2_END:	timer_id = 2;	break;
			case TIMER3_END:	timer_id = 3;	break;
			case TIMER4_END:	timer_id = 4;	break;
			case TIMER5_END:	timer_id = 5;	break;
			case TIMER6_END:	timer_id = 6;	break;
			default:			timer_id = -1;	break;
		}

		if((timer_id > 0) && (timer_id < MAX_SYSTEM_TIMER))
		{
			if(!gstSysTimer[timer_id].msg_qsend)
			{
				continue;
			}
			else if(gstSysTimer[timer_id].msg_q_id)
			{
				gstSysTimer[timer_id].msg_qsend(gstSysTimer[timer_id].msg_q_id, gstSysTimer[timer_id].msg_id, NULL, 0, 0);
			}
			else
			{
				void (*t_runtine)(void) = (void (*)(void)) gstSysTimer[timer_id].msg_qsend;
				t_runtine();
			}
		}
#endif
	}
}

void
drv_l2_sys_timer_init(
	void
)
{
    osThreadDef_t sw_timer_task = { "sw_timer_task", task_system_software_timer, PRIORITY_SYS_SOFTWARE_TIMER_TASK, 1, SYS_SOFTWARE_TIMER_STACK_SIZE};
	int i = 0;

	memset(gstSysTimer, 0, sizeof(gstSysTimer));

	for(i = 0; i < MAX_SYSTEM_TIMER_ISR; i++)
	{
		gTimeBaseC_ISR[i] = NULL;
	}

	gstSysTimerEnable = 0;


	if(SysSoftTmrTaskQ == 0) {
        osMessageQDef_t App_q = {SYS_SOFTWARE_TIMER_TASK_QUEUE_MAX, sizeof(INT32U), 0};

		SysSoftTmrTaskQ = osMessageCreate(&App_q, NULL);
		if(SysSoftTmrTaskQ == 0) {
			ERR_PRINT("fail to create SysSoftTmrTaskQ \r\n");
			return;
		}
	}

	if(osThreadCreate(&sw_timer_task, NULL) == 0)
	{
		ERR_PRINT("fail to create software timer task \r\n");
	}

}

/*
setup interval time, the time clock is 128Hz
every interval time, it will send a message to dispatchQ
timerid is 0~4
interval time, equal to: interval * (1 / 128) (second)
*/

INT32S
drv_l2_sys_timer_set(
	void* msg_qsend,
	void* msg_q_id,
	INT32U msg_id,
	INT32U timerid,
	INT32U interval
)
{
	if(timerid > MAX_SYSTEM_TIMER  - 1)
		return TIMER_ID_INVALID;

	gstSysTimer[timerid].msg_qsend = (fp_msg_qsend)msg_qsend;
	gstSysTimer[timerid].msg_q_id = msg_q_id;
	gstSysTimer[timerid].msg_id = msg_id;

	gstSysTimer[timerid].CurrentTick = interval;
	gstSysTimer[timerid].Interval = interval;
	if(gstSysTimerEnable == 0)
	{
		/* setup timebase C 128Hz */
		time_base_setup(TIME_BASE_C, TMBCS_128HZ, 0, TimeBaseC_ISR);
	}

#if MAX_SYSTEM_TIMER > 0
	if(timerid == 0)
		gstSysTimerEnable |= TIMER0_ENABLE;
#endif

#if MAX_SYSTEM_TIMER > 1
	if(timerid == 1)
        gstSysTimerEnable |= TIMER1_ENABLE;
#endif

#if MAX_SYSTEM_TIMER > 2
	if(timerid == 2)
		gstSysTimerEnable |= TIMER2_ENABLE;
#endif

#if MAX_SYSTEM_TIMER > 3
	if(timerid == 3)
		gstSysTimerEnable |= TIMER3_ENABLE;
#endif

#if MAX_SYSTEM_TIMER > 4
	if(timerid == 4)
		gstSysTimerEnable |= TIMER4_ENABLE;
#endif

#if MAX_SYSTEM_TIMER > 5
	if(timerid == 5)
		gstSysTimerEnable |= TIMER5_ENABLE;
#endif

#if MAX_SYSTEM_TIMER > 6
	if(timerid == 6)
		gstSysTimerEnable |= TIMER6_ENABLE;
#endif
	//SysDisableWaitMode(WAITMODE_TIMER);
	return 0;
}

INT32S
drv_l2_sys_timer_128Hz_init(
	void
)
{
	time_base_setup(TIME_BASE_C, TMBCS_128HZ, 0, TimeBaseC_ISR);
	return STATUS_OK;
}

INT32S
drv_l2_sys_timer_kill(
	INT32U timerid
)
{
	INT32U itemp;
	if(timerid > MAX_SYSTEM_TIMER  - 1)
		return TIMER_ID_INVALID;

#if MAX_SYSTEM_TIMER > 0
	if(timerid == 0)
		itemp = TIMER0_ENABLE;
#endif

#if MAX_SYSTEM_TIMER > 1
	if(timerid == 1)
		itemp = TIMER1_ENABLE;
#endif

#if MAX_SYSTEM_TIMER > 2
	if(timerid == 2)
		itemp = TIMER2_ENABLE;
#endif

#if MAX_SYSTEM_TIMER > 3
	if(timerid == 3)
		itemp = TIMER3_ENABLE;
#endif

#if MAX_SYSTEM_TIMER > 4
	if(timerid == 4)
		itemp = TIMER4_ENABLE;
#endif

#if MAX_SYSTEM_TIMER > 5
	if(timerid == 5)
		itemp = TIMER5_ENABLE;
#endif

#if MAX_SYSTEM_TIMER > 6
	if(timerid == 6)
		itemp = TIMER6_ENABLE;
#endif

	gstSysTimerEnable &= ~itemp;

	if(gstSysTimerEnable == 0)
	{
		/* stop timebase C 128Hz */
		time_base_stop(TIME_BASE_C);
	}
	return 0;
}

INT32S
drv_l2_sys_timer_all_kill(
	void
)
{
	gstSysTimerEnable &= ~(TIMER0_ENABLE | TIMER1_ENABLE | TIMER2_ENABLE | TIMER3_ENABLE | TIMER4_ENABLE | TIMER5_ENABLE | TIMER6_ENABLE);
	if(gstSysTimerEnable == 0)
	{
		/* stop timebase C 128Hz */
		time_base_stop(TIME_BASE_C);
	}
	return 0;
}

/*
registe a user isr to 128Hz interrupt
max can register 5 function
*/

INT32S
drv_l2_sys_timer_isr_register(
	void (*TimerIsrFunction)(void)
)
{
	INT32S	i;

	for(i = 0; i < MAX_SYSTEM_TIMER_ISR; i++)
	{
		if(gTimeBaseC_ISR[i] == TimerIsrFunction)
			return 0;
	}

	if(gstSysTimerEnable == 0)
	{
		/* setup timebase C 128Hz */
		time_base_setup(TIME_BASE_C, TMBCS_128HZ, 0, TimeBaseC_ISR);
	}

	for(i = 0; i < MAX_SYSTEM_TIMER_ISR; i++)
	{
		if(gTimeBaseC_ISR[i] == NULL)
		{
			gTimeBaseC_ISR[i] = TimerIsrFunction;
			gstSysTimerEnable |= (USER_TIMER0_ENABLE << i);
			return 0;
		}
	}
	return -1;
}

INT32S
drv_l2_sys_timer_isr_release(
	void (*TimerIsrFunction)(void)
)
{
	INT32S	i;

	for(i = 0; i < MAX_SYSTEM_TIMER_ISR; i++)
	{
		if(gTimeBaseC_ISR[i] == TimerIsrFunction)
		{
			gTimeBaseC_ISR[i] = NULL;
			gstSysTimerEnable &= ~(USER_TIMER0_ENABLE << i);
			if(gstSysTimerEnable == 0)
			{
				// stop timebase C 128Hz
				time_base_stop(TIME_BASE_C);
			}
			return 0;
		}
	}
	return -1;
}

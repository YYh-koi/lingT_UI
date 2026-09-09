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
#include "drv_l1.h"

// Interrupt Global mask register
#define C_VIC_GLOBAL_MASK_CLEAR		0x00000000
#define C_VIC_GLOBAL_MASK_SET		0x00000001

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
void (*irq_isr[VIC_MAX_IRQ])(void);
void (*irq_arg_isr[VIC_MAX_IRQ])(void*);
void* irq_arg[VIC_MAX_IRQ];

/**
 * @brief   irq service function register
 * @param   irc_num: irq vector number
 * @param   isr: service function
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_irq_register(INT32U irc_num, void (*isr)(void))
{
	if (irc_num && (irc_num<VIC_MAX_IRQ)) {
		irc_num--;

	  #if _OPERATING_SYSTEM == _OS_UCOS2			// Soft Protect for critical section
		OSSchedLock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
		vTaskSuspendAll();
	  #endif

		irq_isr[irc_num] = isr;
		irq_arg_isr[irc_num] = NULL;
		irq_arg[irc_num] = NULL;

	  #if _OPERATING_SYSTEM == _OS_UCOS2
		OSSchedUnlock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
                xTaskResumeAll();
	  #endif

		return STATUS_OK;
	}

	return STATUS_FAIL;
}

INT32S vic_irq_arg_register(INT32U irc_num, void (*isr)(void*), void* arg)
{
	if (irc_num && (irc_num<VIC_MAX_IRQ)) {
		irc_num--;

	  #if _OPERATING_SYSTEM == _OS_UCOS2			// Soft Protect for critical section
		OSSchedLock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
		vTaskSuspendAll();
	  #endif

		irq_isr[irc_num] = NULL;
		irq_arg_isr[irc_num] = isr;
		irq_arg[irc_num] = arg;

	  #if _OPERATING_SYSTEM == _OS_UCOS2
		OSSchedUnlock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
                xTaskResumeAll();
	  #endif

		return STATUS_OK;
	}

	return STATUS_FAIL;
}

/**
 * @brief   irq service function unregister
 * @param   irc_num: irq vector number
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_irq_unregister(INT32U irc_num)
{
	if (irc_num && (irc_num<VIC_MAX_IRQ)) {
		irc_num--;

	  #if _OPERATING_SYSTEM == _OS_UCOS2			// Soft Protect for critical section
		OSSchedLock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
		vTaskSuspendAll();
	  #endif

		irq_isr[irc_num] = NULL;
		irq_arg_isr[irc_num] = NULL;
		irq_arg[irc_num] = NULL;

	  #if _OPERATING_SYSTEM == _OS_UCOS2
		OSSchedUnlock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
                xTaskResumeAll();
	  #endif

		return STATUS_OK;
	}

	return STATUS_FAIL;
}

INT32S vic_irq_priority(INT32U irc_num, INT32U priority)
{
	if (irc_num && (irc_num<VIC_MAX_IRQ)) {
		//irc_num = irc_num + 31;

	  #if _OPERATING_SYSTEM == _OS_UCOS2			// Soft Protect for critical section
		OSSchedLock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
		vTaskSuspendAll();
	  #endif

        GIC_SetPriority(irc_num, priority);

	  #if _OPERATING_SYSTEM == _OS_UCOS2
		OSSchedUnlock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
                xTaskResumeAll();
	  #endif

		return STATUS_OK;
	}

	return STATUS_FAIL;
}

/**
 * @brief   irq service vector enable
 * @param   irc_num: irq vector number
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_irq_enable(INT32U irc_num)
{
	if (irc_num && (irc_num<VIC_MAX_IRQ)) {
		//irc_num = irc_num + 31;

	  #if _OPERATING_SYSTEM == _OS_UCOS2			// Soft Protect for critical section
		OSSchedLock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
		vTaskSuspendAll();
	  #endif

		GIC_EnableIRQ(irc_num);

	  #if _OPERATING_SYSTEM == _OS_UCOS2
		OSSchedUnlock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
                xTaskResumeAll();
	  #endif

		return STATUS_OK;
	}

	return STATUS_FAIL;
}

/**
 * @brief   irq service vector disable
 * @param   irc_num: irq vector number
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_irq_disable(INT32U irc_num)
{
	if (irc_num && (irc_num<VIC_MAX_IRQ)) {

	  #if _OPERATING_SYSTEM == _OS_UCOS2			// Soft Protect for critical section
		OSSchedLock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
		vTaskSuspendAll();
	  #endif

		GIC_DisableIRQ(irc_num);

	  #if _OPERATING_SYSTEM == _OS_UCOS2
		OSSchedUnlock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
                xTaskResumeAll();
	  #endif

		return STATUS_OK;
	}

	return STATUS_FAIL;
}

/**
 * @brief   set specific vector number as fiq
 * @param   irc_num: the vector number to be set as fiq
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_fiq_enable(INT32U irc_num)
{
	if (irc_num && (irc_num<VIC_MAX_IRQ)) {

	  #if _OPERATING_SYSTEM == _OS_UCOS2			// Soft Protect for critical section
		OSSchedLock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
		vTaskSuspendAll();
	  #endif

		GIC_SetGroup(irc_num, 0);

	  #if _OPERATING_SYSTEM == _OS_UCOS2
		OSSchedUnlock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
                xTaskResumeAll();
	  #endif

		return STATUS_OK;
	}

	return STATUS_FAIL;
}

/**
 * @brief   disable specific vector number as fiq
 * @param   irc_num: the vector number to be remove from fiq list.
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_fiq_disable(INT32U irc_num)
{
	if (irc_num && (irc_num<VIC_MAX_IRQ)) {

	  #if _OPERATING_SYSTEM == _OS_UCOS2			// Soft Protect for critical section
		OSSchedLock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
		vTaskSuspendAll();
	  #endif

		GIC_SetGroup(irc_num, 1);

	  #if _OPERATING_SYSTEM == _OS_UCOS2
		OSSchedUnlock();
          #elif _OPERATING_SYSTEM == _OS_FREERTOS
                xTaskResumeAll();
	  #endif

		return STATUS_OK;
	}

	return STATUS_FAIL;
}

/**
 * @brief   irq service routine
 * @param   none
 * @return 	none
 */
void irq_dispatcher(INT32U giccIar)
{
	INT32U vector;

	vector = (giccIar & 0x3FF)-1;

	if (vector < (VIC_MAX_IRQ-1) && (irq_isr[vector] || irq_arg_isr[vector]))
	{
		if (irq_isr[vector])
			(*irq_isr[vector])();
		else
			(*irq_arg_isr[vector])(irq_arg[vector]);
	}
}

/**
 * @brief   interrupt vector init
 * @param   none
 * @return 	none
 */
void vic_init(void)
{
	INT32U i;

	portENTER_CRITICAL();
	for (i=0; i< (VIC_MAX_IRQ-1); i++) {
		irq_isr[i] = NULL;
		irq_arg_isr[i] = NULL;
		irq_arg[i] = NULL;
		GIC_DisableIRQ(i+32);
	}
	portEXIT_CRITICAL();
}

INT32U vic_global_mask_set(void)
{
	INT32S old_mask;

  #if _OPERATING_SYSTEM == _OS_UCOS2				// Soft Protect for critical section
	OSSchedLock();
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
	vTaskSuspendAll();
  #endif

	old_mask = R_INT_FIQ_GMASK & C_VIC_GLOBAL_MASK_SET;
	R_INT_FIQ_GMASK = C_VIC_GLOBAL_MASK_SET;

  #if _OPERATING_SYSTEM == _OS_UCOS2
	OSSchedUnlock();
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
	xTaskResumeAll();
  #endif

	return old_mask;
}

/**
 * @brief   set global interrupt enable
 * @param   old_mask: make value
 * @return 	none
 */
void vic_global_mask_restore(INT32U old_mask)
{
  #if _OPERATING_SYSTEM == _OS_UCOS2				// Soft Protect for critical section
	OSSchedLock();
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
	vTaskSuspendAll();
  #endif

	if (old_mask & C_VIC_GLOBAL_MASK_SET) {
		R_INT_FIQ_GMASK = C_VIC_GLOBAL_MASK_SET;
	} else {
		R_INT_FIQ_GMASK = C_VIC_GLOBAL_MASK_CLEAR;
	}

  #if _OPERATING_SYSTEM == _OS_UCOS2
	OSSchedUnlock();
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
	xTaskResumeAll();
  #endif
}


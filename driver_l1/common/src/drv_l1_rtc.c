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

//Include files
#include "drv_l1.h"

/**************************************************************************
 *                            E X T E R N A L                             *
 **************************************************************************/

extern void print_string(CHAR *fmt, ...);

// Constant definitions used in this file only go here

// Type definitions used in this file only go here

// Global inline functions(Marcos) used in this file only go here

// Declaration of global variables and functions defined in other files

// Variables defined in this file

// Functions defined in this file

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#define CHECK_RETURN(x) \
{\
	ret = x;\
	if(ret < 0) {\
		goto __exit;\
	}\
}\

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

//#if _OPERATING_SYSTEM == 1
#if _OPERATING_SYSTEM != _OS_NONE
#if _OPERATING_SYSTEM == _OS_UCOS2
static OS_EVENT		*sw_rtc_sem = NULL;
#elif _OPERATING_SYSTEM == _OS_FREERTOS
static xSemaphoreHandle sw_rtc_sem;
#endif
#endif

INT8U ext_rtc_pwr_on_flag = 0;
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

void (*rtc_user_isr[RTC_INT_MAX])(void);
void (*rtc_handler_isr)(void);
void (*kc_handler_isr)(void);

extern INT8U g_kc_inten_flag;
static INT8U g_rtc_inten_flag = 0;
static INT8U flag_rtc_fiq_en = 0, flag_ertc_fiq_en = 0;

INT32S ext_rtc_read(INT8U addr, INT8U *data);
//INT32S ext_rtc_write(INT8U addr, INT8U data);
/**
 * @brief   RTC IRQ enable or disable
 * @param   enable - 1:enable
 * @return 	none
 */
static void rtc_irq_set(INT8U enable)
{
	if (enable) {
		vic_irq_enable(VIC_KC);
		vic_irq_enable(VIC_RTCNEW);
		if (flag_rtc_fiq_en) {
			vic_fiq_enable(VIC_KC);
			vic_fiq_enable(VIC_RTCNEW);
		}
	}
	else {
		vic_irq_disable(VIC_KC);
		vic_irq_disable(VIC_RTCNEW);
		if (flag_rtc_fiq_en) {
			vic_fiq_disable(VIC_KC);
			vic_fiq_disable(VIC_RTCNEW);
		}
	}
}

/**
 * @brief   RTC_KC_IRQHandler, keychange and RTC share IRQ number,
            so use this handler to handle their callback function.
 * @param   none
 * @return 	none
 */
void RTC_KC_IRQHandler(void)
{
	if( (g_kc_inten_flag == 1)&&(kc_handler_isr) )
		(*kc_handler_isr)();
	if( (g_rtc_inten_flag == 1)&&(rtc_handler_isr) )
		(*rtc_handler_isr)();
}

/**
 * @brief   ALM_IRQHandler
 * @param   none
 * @return 	none
 */
void ALM_IRQHandler(void)
{
	INT32U int_status = 0;
	INT32U int_ctrl = 0;

	int_status = R_RTC_INT_STATUS;
	int_ctrl = R_RTC_INT_CTRL;

	if (int_status & RTC_ALM_IEN) {
		R_RTC_INT_STATUS = RTC_ALM_IEN;
		if (rtc_user_isr[RTC_ALM_INT_INDEX]&& (int_ctrl & RTC_ALM_IEN)) {
			(*rtc_user_isr[RTC_ALM_INT_INDEX])();
		}
	}
}

/**
 * @brief   SCH_IRQHandler
 * @param   none
 * @return 	none
 */
void SCH_IRQHandler(void)
{
	INT32U int_status = 0;
	INT32U int_ctrl = 0;

	int_status = R_RTC_INT_STATUS;
	int_ctrl = R_RTC_INT_CTRL;

	if (int_status & RTC_SCH_IEN) {
		R_RTC_INT_STATUS = RTC_SCH_IEN;
		if (rtc_user_isr[RTC_SCH_INT_INDEX]&& (int_ctrl & RTC_SCH_IEN)) {
			(*rtc_user_isr[RTC_SCH_INT_INDEX])();
		}
	}
}

/**
 * @brief   HMS_ALM_SCH_IRQHandler
 * @param   none
 * @return 	none
 */
void HMS_ALM_SCH_IRQHandler(void)
{
	INT32U int_status = 0;
	INT32U int_ctrl = 0;

	int_status = R_RTC_INT_STATUS;
	int_ctrl = R_RTC_INT_CTRL;

	if (int_status & RTC_HALF_SEC_IEN) {
		R_RTC_INT_STATUS = RTC_HALF_SEC_IEN;
		if (rtc_user_isr[RTC_HSEC_INT_INDEX] && (int_ctrl & RTC_HALF_SEC_IEN)) {
			(*rtc_user_isr[RTC_HSEC_INT_INDEX])();
		}
	}

	if (int_status & RTC_SEC_IEN) {
		R_RTC_INT_STATUS = RTC_SEC_IEN;
		if (rtc_user_isr[RTC_SEC_INT_INDEX]&& (int_ctrl & RTC_SEC_IEN)) {
			(*rtc_user_isr[RTC_SEC_INT_INDEX])();
		}
	}

	if (int_status & RTC_MIN_IEN) {
		R_RTC_INT_STATUS = RTC_MIN_IEN;
		if (rtc_user_isr[RTC_MIN_INT_INDEX]&& (int_ctrl & RTC_MIN_IEN)) {
			(*rtc_user_isr[RTC_MIN_INT_INDEX])();
		}
	}

	if (int_status & RTC_HR_IEN) {
		R_RTC_INT_STATUS = RTC_HR_IEN;
		if (rtc_user_isr[RTC_HR_INT_INDEX]&& (int_ctrl & RTC_HR_IEN)) {
			(*rtc_user_isr[RTC_HR_INT_INDEX])();
		}
	}

	if (int_status & RTC_DAY_IEN) {
		R_RTC_INT_STATUS = RTC_DAY_IEN;
		if (rtc_user_isr[RTC_DAY_INT_INDEX]&& (int_ctrl & RTC_DAY_IEN)) {
			(*rtc_user_isr[RTC_DAY_INT_INDEX])();
		}
	}
	if (int_status & RTC_ALM_IEN) {
		R_RTC_INT_STATUS = RTC_ALM_IEN;
		if (rtc_user_isr[RTC_ALM_INT_INDEX]&& (int_ctrl & RTC_ALM_IEN)) {
			(*rtc_user_isr[RTC_ALM_INT_INDEX])();
		}
	}
	if (int_status & RTC_SCH_IEN) {
		R_RTC_INT_STATUS = RTC_SCH_IEN;
		if (rtc_user_isr[RTC_SCH_INT_INDEX]&& (int_ctrl & RTC_SCH_IEN)) {
			(*rtc_user_isr[RTC_SCH_INT_INDEX])();
		}
	}
}
/**
 * @brief   RTC software lock
 * @param   none
 * @return 	none
 */
#if _OPERATING_SYSTEM != _OS_NONE
static void sw_rtc_lock(void)
{
#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSemPend(sw_rtc_sem, 0, &err);
#elif _OPERATING_SYSTEM == _OS_FREERTOS
    xSemaphoreTake(sw_rtc_sem, portMAX_DELAY);
#endif
}
/**
 * @brief   RTC software unlock
 * @param   none
 * @return 	none
 */
static void sw_rtc_unlock(void)
{
#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSemPost(sw_rtc_sem);
#elif _OPERATING_SYSTEM == _OS_FREERTOS
	xSemaphoreGive(sw_rtc_sem);
#endif
}
#endif

/**
 * @brief   RTC initialize.
 * @param   vic_rtc / vic_ertc - 0:IRQ, 1~4:FIQ number
 * @return 	none
 */
void drv_l1_rtc_init(INT8U vic_rtc, INT8U vic_ertc)
{
	INT32U dwTimeOut ;
	t_rtc rtc_time = {0};
    //INT8U t_data=0; //,i;
    //INT32S ret;

    drv_l1_clock_set_system_clk_en(CLK_EN0_GPIO_RTC,1);
    flag_rtc_fiq_en = vic_rtc;

	R_RTC_CTRL= 0; /* disable all RTC function */
	R_RTC_CTRL |= RTC_RTCEN; /* enable RTC */
	R_RTC_CTRL |= RTC_HMSEN;

#if _OPERATING_SYSTEM != _OS_NONE
	if(sw_rtc_sem == NULL)
	{
	#if _OPERATING_SYSTEM == _OS_UCOS2
		sw_rtc_sem = OSSemCreate(1);
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
        sw_rtc_sem = xSemaphoreCreateMutex();
    #endif
	}
#endif
	ext_rtc_init(vic_ertc);
/*
    ret = ext_rtc_read(0, &t_data);//switch to external 32K
    t_data |= 0x80;
    ret = ext_rtc_write(0, t_data);

    ret = ext_rtc_read(0x60, &t_data);//IOSC off
    t_data &= ~0x01;
    ret = ext_rtc_write(0x60, t_data);
    if(ret<0) DBG_PRINT("rtc wrtie err\r\n");
*/
	R_RTC_MIN = (INT32U)rtc_time.rtc_min;
	dwTimeOut=0;
	while(R_RTC_BUSY & RTC_MIN_BUSY) {  // wait until not busy
		if (dwTimeOut++ > 100000) break ;
	}

	R_RTC_HOUR = (INT32U)rtc_time.rtc_hour;
	dwTimeOut = 0;
	while(R_RTC_BUSY & RTC_HR_BUSY) {
		if (dwTimeOut++ > 100000) break ;
	}

	R_RTC_SEC = (INT32U)rtc_time.rtc_sec;
	dwTimeOut = 0 ;
	while(R_RTC_BUSY & RTC_SEC_BUSY) {
		if (dwTimeOut++ > 100000) break ;
	}

	rtc_handler_isr = HMS_ALM_SCH_IRQHandler; /* register ext rtc isr */

	g_rtc_inten_flag = 1;
	vic_irq_register(VIC_HMS, RTC_KC_IRQHandler);
	vic_irq_enable(VIC_HMS);
#if FIQ
	vic_fiq_enable(VIC_HMS);
#endif

	vic_irq_register(VIC_ALM, ALM_IRQHandler);
	vic_irq_enable(VIC_ALM);
#if FIQ
	vic_fiq_enable(VIC_ALM);
#endif

	vic_irq_register(VIC_SCH, SCH_IRQHandler);
	vic_irq_enable(VIC_SCH);
#if FIQ
	vic_fiq_enable(VIC_SCH);
#endif
}
/**
 * @brief   RTC re-initialize.
 * @param   vic_rtc / vic_ertc - 0:IRQ, 1~4:FIQ number
 * @return 	none
 */
void drv_l1_rtc_re_init(INT8U vic_rtc, INT8U vic_ertc)
{
	INT32U dwTimeOut ;
	t_rtc rtc_time = {0};

    drv_l1_clock_set_system_clk_en(CLK_EN0_GPIO_RTC,1);
    flag_rtc_fiq_en = vic_rtc;

	R_RTC_CTRL= 0; /* disable all RTC function */
	R_RTC_CTRL |= RTC_RTCEN; /* enable RTC */
	R_RTC_CTRL |= RTC_HMSEN;

#if _OPERATING_SYSTEM != _OS_NONE
	if(sw_rtc_sem == NULL)
	{
	#if _OPERATING_SYSTEM == _OS_UCOS2
		sw_rtc_sem = OSSemCreate(1);
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
        sw_rtc_sem = xSemaphoreCreateMutex();
    #endif
	}
#endif
	ext_rtc_re_init(vic_ertc);

	// update time to RTC
	R_RTC_MIN = (INT32U)rtc_time.rtc_min;
	dwTimeOut=0;
	while(R_RTC_BUSY & RTC_MIN_BUSY) {  // wait until not busy
		if (dwTimeOut++ > 100000) break ;
	}

	R_RTC_HOUR = (INT32U)rtc_time.rtc_hour;
	dwTimeOut = 0;
	while(R_RTC_BUSY & RTC_HR_BUSY) {
		if (dwTimeOut++ > 100000) break ;
	}

	R_RTC_SEC = (INT32U)rtc_time.rtc_sec;
	dwTimeOut = 0 ;
	while(R_RTC_BUSY & RTC_SEC_BUSY) {
		if (dwTimeOut++ > 100000) break ;
	}

	rtc_handler_isr = HMS_ALM_SCH_IRQHandler; /* register ext rtc isr */

	g_rtc_inten_flag = 1;
	vic_irq_register(VIC_HMS, RTC_KC_IRQHandler);
	vic_irq_enable(VIC_HMS);
#if FIQ
	vic_fiq_enable(VIC_HMS);
#endif

	vic_irq_register(VIC_ALM, ALM_IRQHandler);
	vic_irq_enable(VIC_ALM);
#if FIQ
	vic_fiq_enable(VIC_ALM);
#endif

	vic_irq_register(VIC_SCH, SCH_IRQHandler);
	vic_irq_enable(VIC_SCH);
#if FIQ
	vic_fiq_enable(VIC_SCH);
#endif
}
/**
 * @brief   To set hour, minute and second to RTC Hour, Minute and Second registers
 * @param   rtc_time : the pointer of the rtc data structure
 * @return 	none
 */
void rtc_time_set(t_rtc *rtc_time)
{
	INT32U dwTimeOut ;
	//vic_irq_disable(VIC_ALM_SCH_HMS);
#if _OPERATING_SYSTEM == 1				// Soft Protect for critical section
//	sw_rtc_lock();
#endif
	//gpx_rtc_write(0x0,rtc_time->rtc_sec); // sec
	//gpx_rtc_write(0x1,rtc_time->rtc_min); // min
	//gpx_rtc_write(0x2,rtc_time->rtc_hour); // hour
	ext_rtc_time_set(rtc_time) ;
#if _OPERATING_SYSTEM == 1
//	sw_rtc_unlock();
#endif
	//vic_irq_enable(VIC_ALM_SCH_HMS);

	R_RTC_MIN = (INT32U)rtc_time->rtc_min;
	dwTimeOut = 0 ;
	while(R_RTC_BUSY & RTC_MIN_BUSY) {
		if (dwTimeOut++ > 100000) break ;
	}

	R_RTC_HOUR = (INT32U)rtc_time->rtc_hour;
	dwTimeOut = 0 ;
	while(R_RTC_BUSY & RTC_HR_BUSY) {
		if (dwTimeOut++ > 100000) break ;
	}

	R_RTC_SEC = (INT32U)rtc_time->rtc_sec;
	/* wait until not busy */
	dwTimeOut = 0 ;
	while(R_RTC_BUSY & RTC_SEC_BUSY) {
		if (dwTimeOut++ > 100000) break ;
	}

}
/**
 * @brief   To get hour, minute and second from RTC Hour, Minute and Second registers
 * @param   rtc_time : the pointer of the rtc data structure
 * @return 	none
 */
void rtc_time_get(t_rtc *rtc_time)
{
#if _OPERATING_SYSTEM == 1				// Soft Protect for critical section
	//sw_rtc_lock();
#endif
	do {
		rtc_time->rtc_sec = R_RTC_SEC;
		rtc_time->rtc_min = R_RTC_MIN;
		rtc_time->rtc_hour = R_RTC_HOUR;
	} while (rtc_time->rtc_sec != R_RTC_SEC);
#if _OPERATING_SYSTEM == 1
	//sw_rtc_unlock();
#endif
}
/**
 * @brief   To get day
 * @param   rtc_time : the pointer of the rtc data structure
 * @return 	none
 */
void rtc_day_get(t_rtc *rtc_time)
{
	//vic_irq_disable(VIC_ALM_SCH_HMS);
	t_rtc ext_rtc_time ;
#if _OPERATING_SYSTEM == 1				// Soft Protect for critical section
	sw_rtc_lock();
#endif
	//rtc_time->rtc_day = gpx_rtc_read(0x3);
	//rtc_time->rtc_day |= (gpx_rtc_read(0x4) & 0xf) << 8;
	ext_rtc_time_get(&ext_rtc_time);
#if _OPERATING_SYSTEM == 1
	sw_rtc_unlock();
#endif
	rtc_time->rtc_day = ext_rtc_time.rtc_day ;
	//vic_irq_enable(VIC_ALM_SCH_HMS);
}
/**
 * @brief   To set alarm hour, alarm minute and alarm second to rtc Alarm Hour, Alarm Minute and Alarm Second registers
 * @param   rtc_time : the pointer of the rtc data structure
 * @return 	none
 */
void rtc_alarm_set(t_rtc *rtc_time)
{
	R_RTC_ALARM_SEC = (INT32U)rtc_time->rtc_sec;
	R_RTC_ALARM_MIN = (INT32U)rtc_time->rtc_min;
	R_RTC_ALARM_HOUR = (INT32U)rtc_time->rtc_hour;
}
/**
 * @brief   To get alarm hour, alarm minute and alarm second from rtc Alarm Hour, Alarm Minute and Alarm Second registers
 * @param   rtc_time : the pointer of the rtc data structure
 * @return 	none
 */
void rtc_alarm_get(t_rtc *rtc_time)
{
    rtc_time->rtc_sec = R_RTC_ALARM_SEC;
	rtc_time->rtc_min = R_RTC_ALARM_MIN;
	rtc_time->rtc_hour = R_RTC_ALARM_HOUR;
}
/**
 * @brief   To enable RTC alarm interrupt
 * @param   none
 * @return 	none
 */
void rtc_alarm_enable(void)
{
    R_RTC_CTRL |= RTC_ALMEN;
    //vic_irq_enable(VIC_RTC_KC);//set at initial, if set here cause irq and fiq confict when used fiq

}
/**
 * @brief   To enable RTC scheduler interrupt
 * @param   none
 * @return 	none
 */
void rtc_scheduler_enable(void)
{
    R_RTC_CTRL |= RTC_SCHEN;
    //vic_irq_enable(VIC_RTC_KC);
}
/**
 * @brief   To set(enable or disable) rtc interrupt
 * @param   mask: rtc interrupt source. Please refer to P_RTC_INT_CTRL in Programming Guide to know the corresponding bit for the variety of the rtc interrupt source.
 * @param   value: value to set to P_RTC_INT_CTRL.
 * @return 	none
 */
void rtc_int_set(INT16U mask, INT16U value)
{
    INT32U _mask = 0;
    INT32U _value = 0;

    _mask = mask & 0xFFF;
    _value = value & 0xFFF;

    if (mask & GPX_RTC_ALM_IEN) {
        _mask |= RTC_ALM_IEN;
    }

    if (mask & GPX_RTC_DAY_IEN) {
        _mask |= RTC_DAY_IEN;
    }

    if (value & GPX_RTC_ALM_IEN) {
        _value |= RTC_ALM_IEN;
        R_RTC_CTRL |= RTC_ALMEN;
    }
    else {
        R_RTC_CTRL &= ~RTC_ALMEN;
    }

    if (value & GPX_RTC_DAY_IEN) {
        _value |= RTC_DAY_IEN;
    }

    R_RTC_INT_CTRL &= ~_mask;
	R_RTC_INT_CTRL |= (_mask & _value);

}
/**
 * @brief   To enable rtc scheduler, including the scheduler interrupt.
 * @param   freq: scheduler frequency.
            Please refer to RTC_SCH_PERIOD to know how to set this value properly.
 * @return 	none
 */
void rtc_schedule_enable(INT8U freq)
{
	R_RTC_CTRL |= RTC_RTCEN;
	R_RTC_CTRL &= ~RTC_SCHSEL;
	R_RTC_CTRL |= freq;
	R_RTC_CTRL |= RTC_SCHEN;
	R_RTC_INT_CTRL |= RTC_SCH_IEN;
}
/**
 * @brief   To disable rtc scheduler, including the scheduler interrupt.
 * @param   none
 * @return 	none
 */
void rtc_schedule_disable(void)
{
	R_RTC_CTRL &= ~RTC_SCHEN;
	R_RTC_INT_CTRL &= ~RTC_SCH_IEN;
}
/**
 * @brief   To set RTC callback function
 * @param   Int_idx: Interrupt source index,
            please refer to RTC_INT_INDEX to set the correct index.
 * @param   user_isr: the callback function pointer of the specific interrupt source.
 * @return 	0=Success, -1=Fail
 */
INT32S rtc_callback_set(INT8U int_idx, void (*user_isr)(void))
{
	INT32S status = STATUS_OK;

	if (int_idx > RTC_INT_MAX) {
		return STATUS_FAIL;
	}

#if _OPERATING_SYSTEM == 1				// Soft Protect for critical section
	sw_rtc_lock();
#endif
	if (rtc_user_isr[int_idx] != 0) {
		status = STATUS_FAIL;
	}
	else {
		rtc_user_isr[int_idx] = user_isr;
	}
#if _OPERATING_SYSTEM == 1
	sw_rtc_unlock();
#endif

	return status;
}
/**
 * @brief   To set RTC callback function
 * @param   Int_idx: Interrupt source index,
            please refer to RTC_INT_INDEX to set the correct index.
 * @return 	0=Success, -1=Fail
 */
INT32S rtc_callback_clear(INT8U int_idx)
{
	if (int_idx > RTC_INT_MAX) {
		return STATUS_FAIL;
	}
#if _OPERATING_SYSTEM == 1				// Soft Protect for critical section
	sw_rtc_lock();
#endif
	rtc_user_isr[int_idx] = 0;
#if _OPERATING_SYSTEM == 1
	sw_rtc_unlock();
#endif
	return STATUS_OK;
}
/**
 * @brief   To read IRTC hour, minute and second value to internal rtc registers.
 * @param   none
 * @return 	none
 */
void rtc_ext_to_int_set(void)
{

	INT32U dwTimeOut ;
	t_rtc rtc_time={0} ;
	ext_rtc_time_get(&rtc_time) ;

	R_RTC_MIN = (INT32U)rtc_time.rtc_min;
	dwTimeOut = 0 ;
	while(R_RTC_BUSY & RTC_MIN_BUSY) {
		if (dwTimeOut++ > 100000) break ;
	}

	R_RTC_HOUR = (INT32U)rtc_time.rtc_hour;
	dwTimeOut = 0 ;
	while(R_RTC_BUSY & RTC_HR_BUSY) {
		if (dwTimeOut++ > 100000) break ;
	}

	R_RTC_SEC = (INT32U)rtc_time.rtc_sec;
	/* wait until not busy */
	dwTimeOut = 0 ;
	while(R_RTC_BUSY & RTC_SEC_BUSY) {
		if (dwTimeOut++ > 100000) break ;
	}

}
/**
 * @brief   To read IRTC data
 * @param   addr: target address
 * @param   data: output data pointer
 * @return 	none
 */
INT32S ext_rtc_read(INT8U addr, INT8U *data)
{
	INT32S N = 1000;

	R_EXT_RTC_ADDR = addr;
	R_EXT_RTC_REQ = 0x02;	// triger read

	//wait ready
	do {
		if(R_EXT_RTC_READY & 0x01) {
			*data = R_EXT_RTC_RD_DATA;
			return 0;
		} else {
			//drv_msec_wait(1);
		}
	} while(--N > 0);

	return -1;
}
/**
 * @brief   To read IRTC data
 * @param   addr: target address
 * @param   data: write data
 * @return 	none
 */
INT32S ext_rtc_write(INT8U addr, INT8U data)
{
	INT32S N = 1000;

	R_EXT_RTC_ADDR = addr;
	R_EXT_RTC_WR_DATA = data;
	R_EXT_RTC_REQ |= 0x01; //triger write

	//wait ready
	do {
		if(R_EXT_RTC_READY & 0x01) { // check if RTC controller is free
			break;
		}
		else
		{
			//drv_msec_wait(1);
		}
	} while(--N > 0);
	
	if(N == 0)
	{
		return -1;
	}
	switch(addr)
	{
	case 0x00:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x40:
	case 0x50:
	case 0x58:
	case 0x59:
	case 0x60:
	case 0x68:
	case 0x69:
			N = 1000;
			
			do{
				
				ext_rtc_read(R_EXTRTC_CTRL, &data);
				
				if((data & 0x10) == 0)
				{ // check if RTC macro is free
					break;
				}
				else
				{
					//drv_msec_wait(1);
				}
		} while(--N > 0);

			if(N == 0)
			{
				return -1;
			}
		break;

	default:
		break;
	}

	return 0;
}


INT32S ext_rtc_set_io_wakeup(INT8U enable)
{
    INT8U t_data=0; //,i;
    INT32S ret;

    ret = ext_rtc_read(R_EXTRTC_IO_CTRL0, &t_data);//read io wakeup register
    t_data &= ~(1<<5);
    t_data |= (enable<<5);
    ret = ext_rtc_write(R_EXTRTC_IO_CTRL0, t_data);
    if(ret<0){
        DBG_PRINT("rtc wrtie err\r\n");
        return -1;
    }
    return 0;
}


INT32S ext_rtc_set_usb_wakeup(INT8U io_conf)
{
    INT8U t_data=0; //,i;
    INT32S ret;

    ret = ext_rtc_read(R_EXTRTC_IO_CTRL0, &t_data);//read usb wakeup register
    t_data |= 0x1;
    ret = ext_rtc_write(R_EXTRTC_IO_CTRL0, t_data);
    if(ret<0){
        DBG_PRINT("rtc wrtie err\r\n");
        return -1;
    }

    //set usb wakeup
    ret = ext_rtc_read(R_EXTRTC_IO_MODE1, &t_data);//set USB WKU mode
    t_data |= io_conf<<6;
    ret = ext_rtc_write(R_EXTRTC_IO_MODE1, t_data);
    if(ret<0){
        DBG_PRINT("rtc wrtie err\r\n");
        return -1;
    }

    //set usb wakeup interrut
    ret = ext_rtc_read(R_EXTRTC_WKU_IO_INT_EN, &t_data);
    t_data |= (1<<7);//0x80;
    ret = ext_rtc_write(R_EXTRTC_WKU_IO_INT_EN, t_data);
    if(ret<0){
        DBG_PRINT("rtc wrtie err\r\n");
        return -1;
    }

    //read back setup register to doublecheck. Comment it out on formal platform code
    //ret = ext_rtc_read(0x48, &t_data);
    //DBG_PRINT("USB_WAKUP register = %x \r\n",t_data);
    //ret = ext_rtc_read(0x58, &t_data);
    //DBG_PRINT("USB_WAKUP INT register = %x \r\n",t_data);
    return 0;
}

INT32S ext_rtc_set_exio_wakeup(INT8U port, INT8U mode)
{
    INT8U t_data=0; //,i;
    INT32S ret;

    port &= 0x7;
    if(port>5){
        DBG_PRINT("wrong IO configuration \r\n");
        return -1;
    }
    if(port<3){
        ret = ext_rtc_read(R_EXTRTC_IO_MODE0, &t_data);//read wakeup IO register
        t_data &= ~((0x3)<<(port*2));
        t_data |= mode<<(port*2);
        ret = ext_rtc_write(R_EXTRTC_IO_MODE0, t_data);
        if(ret<0){
            DBG_PRINT("rtc wrtie err\r\n");
            return -1;
        }
        ret = ext_rtc_read(R_EXTRTC_WKU_IO_INT_EN, &t_data);//enable wakeup IO interrupt
        t_data |= 1<<port;
        ret = ext_rtc_write(R_EXTRTC_WKU_IO_INT_EN, t_data);
        if(ret<0){
            DBG_PRINT("rtc wrtie err\r\n");
            return -1;
        }
    }else{
        port -=3;
        ret = ext_rtc_read(R_EXTRTC_IO_MODE1, &t_data);//read usb wakeup register
        t_data &= ~((0x3)<<(port*2));
        t_data |= mode<<(port*2);
        ret = ext_rtc_write(R_EXTRTC_IO_MODE1, t_data);
        if(ret<0){
            DBG_PRINT("rtc wrtie err\r\n");
            return -1;
        }

        port += 3;
        ret = ext_rtc_read(R_EXTRTC_WKU_IO_INT_EN, &t_data);
        t_data |= 1<<port;
        ret = ext_rtc_write(R_EXTRTC_WKU_IO_INT_EN, t_data);
        if(ret<0){
            DBG_PRINT("rtc wrtie err\r\n");
            return -1;
        }

    }

    return 0;


}

INT32S ext_rtc_polling_extio(void)
{
    INT8U t_data; //,i;
    INT32S ret;

    CHECK_RETURN(ext_rtc_read(0x48, &t_data));
    //ret = ext_rtc_read(0x48, &t_data);//read ext io wakeup register
    DBG_PRINT("0x48 = 0x%x\r\n",t_data);
    CHECK_RETURN(ext_rtc_read(0x4A, &t_data));
    //ret = ext_rtc_read(0x4A, &t_data);//read ext io wakeup register
    DBG_PRINT("0x4A = 0x%x\r\n",t_data);
    CHECK_RETURN(ext_rtc_read(0x4B, &t_data));
    //ret = ext_rtc_read(0x4B, &t_data);//read ext io wakeup register
    DBG_PRINT("0x4B = 0x%x\r\n",t_data);

    CHECK_RETURN(ext_rtc_read(0x59, &t_data));
    //ret = ext_rtc_read(0x59, &t_data);//read ext io wakeup register
    DBG_PRINT("0x59 = 0x%x\r\n",t_data);

    ext_rtc_write(0x59, t_data);
    CHECK_RETURN(ext_rtc_read(0x59, &t_data));
    //ext_rtc_read(0x59, &t_data);//read ext io wakeup register
    DBG_PRINT("after clear 0x59 = 0x%x\r\n",t_data);

    CHECK_RETURN(ext_rtc_read(0x58, &t_data));
    ret = ext_rtc_read(0x58, &t_data);//read ext io INT en register
    DBG_PRINT("0x58 = 0x%x\r\n",t_data);

__exit:
    return ret ;
}



/**
 * @brief   RTC_IRQHandler
 * @param   none
 * @return 	none
 */
void RTC_IRQHandler(void)
{
	INT8U status,io_status;
	INT8U enable,io_enable;
	INT32S ret;

	ret = ext_rtc_read(R_EXTRTC_INT_STATUS, &status);
	if(ret < 0) {
		goto __fail;
	}

	ret = ext_rtc_read(R_EXTRTC_INT_ENABLE, &enable);
	if(ret < 0) {
		goto __fail;
	}
//io wake irq status check
		ret = ext_rtc_read(0x59, &io_status);
	if(ret < 0) {
		goto __fail;
	}
//io wake irq enable check
	ret = ext_rtc_read(0x58, &io_enable);
	if(ret < 0) {
		goto __fail;
	}

//DBG_PRINT("Ext RTC ISR: status=%02X, enable=%02X\r\n", status, enable);

	//clear flag
	status &= enable;
	ext_rtc_write(R_EXTRTC_INT_STATUS, 0x00);

	if (status & EXT_RTC_SEC_IEN) {
		if (rtc_user_isr[EXT_RTC_SEC_INT_INDEX]) {
			(*rtc_user_isr[EXT_RTC_SEC_INT_INDEX])();
		}
	}

	if (status & EXT_RTC_ALARM_IEN) {
		if (rtc_user_isr[EXT_RTC_ALARM_INT_INDEX]) {
			(*rtc_user_isr[EXT_RTC_ALARM_INT_INDEX])();
		}
	}

	if (status & EXT_RTC_WAKEUP_IEN) {
		if (rtc_user_isr[EXT_RTC_WAKEUP_INT_INDEX]) {
			(*rtc_user_isr[EXT_RTC_WAKEUP_INT_INDEX])();
		}
	}

	//clear flag
	io_status &= io_enable;
	ext_rtc_write(0x59, io_status);
	if (io_status & 0x3F) {
		if (rtc_user_isr[EXT_RTC_IO_INT_INDEX]) {
			(*rtc_user_isr[EXT_RTC_IO_INT_INDEX])();
		}
	}

	io_status &= io_enable;
	ext_rtc_write(0x59, io_status);
	if (io_status & 0x80) {
		if (rtc_user_isr[EXT_RTC_USB_INT_INDEX]) {
			(*rtc_user_isr[EXT_RTC_USB_INT_INDEX])();
		}
	}

	return;

__fail:
	ext_rtc_write(R_EXTRTC_INT_STATUS, 0x00);
	return;
}
/**
 * @brief   To check IRTC wakeup status and clear it if IRTC wakeup flag is
 * @param   none
 * @return 	0: the IRTC wakeup is not occured
            1: the IRTC wakeup is occurred and then the status bit is cleared.
 */
INT32S ext_rtc_chk_wakeup_status(void)
{
    INT8U temp = 0 ;
    INT32S ret;

	//vic_irq_disable(VIC_RTC_KC);
	rtc_irq_set(DISABLE);
	//NVIC_DisableIRQ(RTC_IRQn);
#if _OPERATING_SYSTEM != _OS_NONE				// Soft Protect for critical section
#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSchedLock();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
    osSuspend();
#endif
#endif

    // check if Ext RTC wakeup function trigger
	CHECK_RETURN(ext_rtc_read(R_EXTRTC_INT_STATUS, &temp));
	//DBG_PRINT("Ext RTC Int Value=0x%02X, Ret=%d\r\n",temp, ret);
    if (temp &0x04) // ext rtc wakeup
    {
        ext_rtc_write(R_EXTRTC_INT_STATUS, 0); // clear it
        ret = 1;
    }
__exit:

#if _OPERATING_SYSTEM != _OS_NONE
#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSchedUnlock();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
    osResume();
#endif
#endif
	//vic_irq_enable(VIC_RTC_KC);
	rtc_irq_set(ENABLE);
	return ret;
}
/**
 * @brief   To initialize IRTC macro
 * @param   vic_num - 0:IRQ, 1~4:FIQ number
 * @return 	none
 */
void ext_rtc_init(INT8U vic_num)
{
	INT8U byReliable=0;
	INT32S ret=0;
	INT8U t_data=0;

    flag_ertc_fiq_en = vic_num;
	R_EXT_RTC_CTRL = 0x01;					// enable ext rtc clock
	/*
	if (INIT_MHZ==144) {
		R_EXT_RTC_DIV = 0x20;
	} else if (INIT_MHZ>144) {
	    R_EXT_RTC_DIV = 0x30;
	} else if (INIT_MHZ>96) {
	    R_EXT_RTC_DIV = 0x10;
	}
	else // < 96MHz
        R_EXT_RTC_DIV = 0x08;
    */

    //Serial clock = SYSCLK/(SDIV+1)
    R_EXT_RTC_DIV = 0x20;

    ext_rtc_write(0x07, 0x5A);//key code

	if (ext_rtc_chk_wakeup_status() > 0) {
		ext_rtc_pwr_on_flag = 1;
	} else {
		ext_rtc_pwr_on_flag = 0;
	}

    ext_rtc_write(0xEF, 0x00);              //clear RTC RAM 0xEF to prevent from entering iROM resume mode accidentally

    ext_rtc_get_reliable(&byReliable);
    DBG_PRINT("reliable  = 0x%x \r\n",byReliable);

	if (ext_rtc_get_reliable(&byReliable)<0) {
 		DBG_PRINT("Failed to get Ext RTC reliable\r\n");
		ext_rtc_set_reliable(REALIBABLE);
	}

	if (byReliable!=REALIBABLE) {
		DBG_PRINT("Ext RTC reliable is wrong=0x%02X, set again\r\n", byReliable);
		ext_rtc_set_reliable(REALIBABLE);
		ext_rtc_get_reliable(&byReliable);
		if (byReliable!=REALIBABLE) {
			DBG_PRINT("Failed to set/get Ext RTC reliable!!\r\n");
		}
		ret = ext_rtc_read(0, &t_data);//switch to external 32K
        t_data |= 0x80;
        ret = ext_rtc_write(0, t_data);

        ret = ext_rtc_read(0x60, &t_data);//IOSC off
        t_data &= ~0x01;
        t_data |= 0x02;
        ret = ext_rtc_write(0x60, t_data);
    if(ret<0) DBG_PRINT("rtc wrtie err\r\n");
	} else {
		DBG_PRINT("Ext RTC reliable is OK\r\n");
	}
	//disable EXT RTC interrupt at initial
	//interrupt source shared with RTC/KC,
	//therefore it would also check EXT RTC when others source interrupt trigger
    ext_rtc_alarm_enable(NULL, 0, NULL);
    //ext_rtc_wakeup_int_enable(0, NULL);
    vic_irq_register(VIC_RTCNEW, RTC_IRQHandler);
	vic_irq_enable(VIC_RTCNEW);
	if (vic_num) {
        vic_fiq_enable(VIC_RTCNEW);
    }
	else{
		vic_fiq_disable(VIC_RTCNEW);
    }
}
/**
 * @brief   To re-initialize IRTC macro
 * @param   vic_num - 0:IRQ, 1~4:FIQ number
 * @return 	none
 */
void ext_rtc_re_init(INT8U vic_num)
{
	INT8U byReliable=0;

    flag_ertc_fiq_en = vic_num;
	R_EXT_RTC_CTRL = 0x01;					// enable ext rtc clock

    R_EXT_RTC_DIV = 0x20;

    ext_rtc_write(0x07, 0x5A);//key code

	if (ext_rtc_chk_wakeup_status() > 0) {
		ext_rtc_pwr_on_flag = 1;
	} else {
		ext_rtc_pwr_on_flag = 0;
	}

	//ext_rtc_write(R_EXTRTC_CTRL, 0x11);     //RTC clk enable
    //ext_rtc_write(0xEF, 0x00);              //clear RTC RAM 0xEF to prevent from entering iROM resume mode accidentally

    ext_rtc_get_reliable(&byReliable);
    //DBG_PRINT("reliable  = 0x%x \r\n",byReliable);

	if (ext_rtc_get_reliable(&byReliable)<0) {
 		DBG_PRINT("Failed to get Ext RTC reliable\r\n");
		ext_rtc_set_reliable(REALIBABLE);
	}

	if (byReliable!=REALIBABLE) {
		DBG_PRINT("Ext RTC reliable is wrong=0x%02X, set again\r\n", byReliable);
		ext_rtc_set_reliable(REALIBABLE);
		ext_rtc_get_reliable(&byReliable);
		if (byReliable!=REALIBABLE) {
			DBG_PRINT("Failed to set/get Ext RTC reliable!!\r\n");
		}
	} //else {
		//DBG_PRINT("Ext RTC reliable is OK\r\n");
	//}

    ext_rtc_alarm_enable(NULL, 0, NULL);


    vic_irq_register(VIC_RTCNEW, RTC_IRQHandler);
	vic_irq_enable(VIC_RTCNEW);
	if (vic_num) {
        vic_fiq_enable(VIC_RTCNEW);
    }
	else{
		vic_fiq_disable(VIC_RTCNEW);
    }
}
/**
 * @brief   To set IRTC reliable
 * @param   byReliable: the reliable value to be set to IRTC macro
 * @return 	none
 */
INT32S ext_rtc_set_reliable(INT8U byReliable)
{
    INT32S ret ;

    CHECK_RETURN(ext_rtc_write(R_EXTRTC_RELIABLE, byReliable));
__exit:
    return ret ;
}
/**
 * @brief   To get IRTC reliable
 * @param   *pbyReliable: the reliable value read from IRTC macro
 * @return 	none
 */
INT32S ext_rtc_get_reliable(INT8U *pbyReliable)
{
    INT32S ret ;
    CHECK_RETURN(ext_rtc_read(R_EXTRTC_RELIABLE, pbyReliable));
__exit:
    return ret ;
}
/**
 * @brief   To set time to IRTC macro
 * @param   *rtc_time: the pointer of the rtc data structure
 * @return 	0: Success, -1: Fail
 */
INT32S ext_rtc_time_set(t_rtc *rtc_time)
{
	INT32S ret;
	INT64U sec;

	//vic_irq_disable(VIC_RTC_KC);
	rtc_irq_set(DISABLE);
#if _OPERATING_SYSTEM != _OS_NONE				// Soft Protect for critical section
	sw_rtc_lock();
#endif

	sec = (rtc_time->rtc_day * 60 * 60 * 24) +
			(rtc_time->rtc_hour * 60 * 60) +
			(rtc_time->rtc_min * 60) +
			rtc_time->rtc_sec;

	sec <<= 15;
	CHECK_RETURN(ext_rtc_write(R_EXTRTC_LOAD_TIME0, (sec >> 0) & 0xFF));
	CHECK_RETURN(ext_rtc_write(R_EXTRTC_LOAD_TIME1, (sec >> 8) & 0xFF));
	CHECK_RETURN(ext_rtc_write(R_EXTRTC_LOAD_TIME2, (sec >> 16) & 0xFF));
	CHECK_RETURN(ext_rtc_write(R_EXTRTC_LOAD_TIME3, (sec >> 24) & 0xFF));
	CHECK_RETURN(ext_rtc_write(R_EXTRTC_LOAD_TIME4, (sec >> 32) & 0xFF));
	CHECK_RETURN(ext_rtc_write(R_EXTRTC_LOAD_TIME5, (sec >> 40) & 0xFF));

__exit:
#if _OPERATING_SYSTEM != _OS_NONE
	sw_rtc_unlock();
#endif
	//vic_irq_enable(VIC_RTC_KC);
	rtc_irq_set(ENABLE);
	return ret;
}
/**
 * @brief   To get time from IRTC macro
 * @param   *rtc_time: the pointer of the rtc data structure
 * @return 	0: Success, -1: Fail
 */
INT32S ext_rtc_time_get(t_rtc *rtc_time)
{
        INT8U temp;
        INT32S ret;
        INT64U sec;

        sec = 0;
        CHECK_RETURN(ext_rtc_read(R_EXTRTC_TIME0, &temp));
//DBG_PRINT("\r\nExt_RTC 0x10=0x%08X\r\n", temp);
        sec += (INT64U)temp;

        CHECK_RETURN(ext_rtc_read(R_EXTRTC_TIME1, &temp));
//DBG_PRINT("Ext_RTC 0x11=0x%08X\r\n", temp);
        sec += (INT64U)temp << 8;

        CHECK_RETURN(ext_rtc_read(R_EXTRTC_TIME2, &temp));
//DBG_PRINT("Ext_RTC 0x12=0x%08X\r\n", temp);
        sec += (INT64U)temp << 16;

        CHECK_RETURN(ext_rtc_read(R_EXTRTC_TIME3, &temp));
//DBG_PRINT("Ext_RTC 0x13=0x%08X\r\n", temp);
        sec += (INT64U)temp << 24;

        CHECK_RETURN(ext_rtc_read(R_EXTRTC_TIME4, &temp));
//DBG_PRINT("Ext_RTC 0x14=0x%08X\r\n", temp);
        sec += (INT64U)temp << 32;

        CHECK_RETURN(ext_rtc_read(R_EXTRTC_TIME5, &temp));
//DBG_PRINT("Ext_RTC 0x15=0x%08X\r\n", temp);
        sec += (INT64U)temp << 40;
//DBG_PRINT("sec %d\r\n",sec);
        sec >>= 15;
        rtc_time->rtc_day = (INT64U)sec/(60*60*24);

        sec -= (INT64U)(rtc_time->rtc_day * 60 * 60 * 24);
        rtc_time->rtc_hour = (INT64U)sec / (60*60);

        sec -= (INT64U)(rtc_time->rtc_hour * 60 * 60);
        rtc_time->rtc_min = (INT64U)sec / 60;

        sec -= (INT64U)(rtc_time->rtc_min * 60);
        rtc_time->rtc_sec = sec;

__exit:
        return ret;
}
/**
 * @brief   To get second from IRTC macro
 * @param   none
 * @return 	second
 */
INT64U ext_rtc_time_get_value(void)
{
    INT32S ret;
	INT8U temp;
	INT64U sec;

        sec = 0;
        CHECK_RETURN(ext_rtc_read(R_EXTRTC_TIME0, &temp));
//DBG_PRINT("\r\nExt_RTC 0x10=0x%08X\r\n", temp);
        sec += (INT64U)temp;

        CHECK_RETURN(ext_rtc_read(R_EXTRTC_TIME1, &temp));
//DBG_PRINT("Ext_RTC 0x11=0x%08X\r\n", temp);
        sec += (INT64U)temp << 8;

        CHECK_RETURN(ext_rtc_read(R_EXTRTC_TIME2, &temp));
//DBG_PRINT("Ext_RTC 0x12=0x%08X\r\n", temp);
        sec += (INT64U)temp << 16;

        CHECK_RETURN(ext_rtc_read(R_EXTRTC_TIME3, &temp));
//DBG_PRINT("Ext_RTC 0x13=0x%08X\r\n", temp);
        sec += (INT64U)temp << 24;

        CHECK_RETURN(ext_rtc_read(R_EXTRTC_TIME4, &temp));
//DBG_PRINT("Ext_RTC 0x14=0x%08X\r\n", temp);
        sec += (INT64U)temp << 32;

        CHECK_RETURN(ext_rtc_read(R_EXTRTC_TIME5, &temp));
//DBG_PRINT("Ext_RTC 0x15=0x%08X\r\n", temp);
        sec += (INT64U)temp << 40;

        sec >>= 15;

__exit:
	return sec;
}
/**
 * @brief   convert rtc_time struct to second
 * @param   rtc_time structure
 * @return 	second
 */
INT64U ext_rtc_time_t_rtc_to_sec(t_rtc *rtc_time)
{
	INT64U sec;
	sec = (rtc_time->rtc_day * 60 * 60 * 24) +
			(rtc_time->rtc_hour * 60 * 60) +
			(rtc_time->rtc_min * 60) +
			rtc_time->rtc_sec;
	return sec;
}
/**
 * @brief   Add rtc_time struct time to current IRTC time
 * @param   rtc_time structure
 * @return 	none
 */
INT32S ext_rtc_time_add_now_and_set(t_rtc *rtc_time) // rtc_time -> added
{
        INT32S ret;
        INT64U sec;
        INT64U now_sec;

        INT64U sec_1;
        INT64U now_sec_1;

        t_rtc now_rtc_time ;

        DBG_PRINT("ext_rtc_time_set()~~~~~~~~~~~~~~~\r\n");

        //vic_irq_disable(VIC_RTC_KC);
		rtc_irq_set(DISABLE);
#if _OPERATING_SYSTEM != _OS_NONE                                // Soft Protect for critical section
        sw_rtc_lock();
#endif

        sec = (rtc_time->rtc_day * 60 * 60 * 24) +
                        (rtc_time->rtc_hour * 60 * 60) +
                        (rtc_time->rtc_min * 60) +
                        rtc_time->rtc_sec;
        //
        sec_1 = sec<<15 ;
        DBG_PRINT("SPI time:\r\n");
        DBG_PRINT("Ext RTC 0x10 = 0x%08X\r\n", (sec_1 >> 0) & 0xFF );
        DBG_PRINT("Ext RTC 0x11 = 0x%08X\r\n", (sec_1 >> 8) & 0xFF );
        DBG_PRINT("Ext RTC 0x12 = 0x%08X\r\n", (sec_1 >> 16) & 0xFF );
        DBG_PRINT("Ext RTC 0x13 = 0x%08X\r\n", (sec_1 >> 24) & 0xFF );
        DBG_PRINT("Ext RTC 0x14 = 0x%08X\r\n", (sec_1 >> 32) & 0xFF );
        DBG_PRINT("Ext RTC 0x15 = 0x%08X\r\n", (sec_1 >> 40) & 0xFF );
        DBG_PRINT("========================\r\n");

        // get current time
        ext_rtc_time_get(&now_rtc_time) ;
        now_sec = (now_rtc_time.rtc_day * 60 * 60 * 24) +
                        (now_rtc_time.rtc_hour * 60 * 60) +
                        (now_rtc_time.rtc_min * 60) +
                        now_rtc_time.rtc_sec;

        DBG_PRINT("Current time:\r\n");
        now_sec_1 = now_sec<<15 ;
        DBG_PRINT("Ext RTC 0x10 = 0x%08X\r\n", (now_sec_1 >> 0) & 0xFF );
        DBG_PRINT("Ext RTC 0x11 = 0x%08X\r\n", (now_sec_1 >> 8) & 0xFF );
        DBG_PRINT("Ext RTC 0x12 = 0x%08X\r\n", (now_sec_1 >> 16) & 0xFF );
        DBG_PRINT("Ext RTC 0x13 = 0x%08X\r\n", (now_sec_1 >> 24) & 0xFF );
        DBG_PRINT("Ext RTC 0x14 = 0x%08X\r\n", (now_sec_1 >> 32) & 0xFF );
        DBG_PRINT("Ext RTC 0x15 = 0x%08X\r\n", (now_sec_1 >> 40) & 0xFF );
        DBG_PRINT("========================\r\n");

        sec = now_sec + sec ;

        DBG_PRINT("Final time:\r\n");
        sec <<= 15;
        CHECK_RETURN(ext_rtc_write(R_EXTRTC_LOAD_TIME0, (sec >> 0) & 0xFF));
        DBG_PRINT("Ext RTC 0x10 = 0x%08X\r\n", (sec >> 0) & 0xFF );
        CHECK_RETURN(ext_rtc_write(R_EXTRTC_LOAD_TIME1, (sec >> 8) & 0xFF));
        DBG_PRINT("Ext RTC 0x11 = 0x%08X\r\n", (sec >> 8) & 0xFF );
        CHECK_RETURN(ext_rtc_write(R_EXTRTC_LOAD_TIME2, (sec >> 16) & 0xFF));
        DBG_PRINT("Ext RTC 0x12 = 0x%08X\r\n", (sec >> 16) & 0xFF );
        CHECK_RETURN(ext_rtc_write(R_EXTRTC_LOAD_TIME3, (sec >> 24) & 0xFF));
        DBG_PRINT("Ext RTC 0x13 = 0x%08X\r\n", (sec >> 24) & 0xFF );
        CHECK_RETURN(ext_rtc_write(R_EXTRTC_LOAD_TIME4, (sec >> 32) & 0xFF));
        DBG_PRINT("Ext RTC 0x14 = 0x%08X\r\n", (sec >> 32) & 0xFF );
        CHECK_RETURN(ext_rtc_write(R_EXTRTC_LOAD_TIME5, (sec >> 40) & 0xFF));
        DBG_PRINT("Ext RTC 0x15 = 0x%08X\r\n", (sec >> 40) & 0xFF );
        DBG_PRINT("========================\r\n");

__exit:
#if _OPERATING_SYSTEM != _OS_NONE
        sw_rtc_unlock();
#endif
        //vic_irq_enable(VIC_RTC_KC);
		rtc_irq_set(ENABLE);
        return ret;
}
/**
 * @brief   To enable or disable IRTC alarm
 * @param   *rtc_time: the pointer of the rtc data structure
            byEnable: Enable or disable IRTC alarm
            1: Enable
            0:disable
            ext_rtc_alarm_isr: IRTC alarm interrupt callback function
 * @return 	none
 */
INT32S ext_rtc_alarm_enable(t_rtc *rtc_time, INT8U byEnable, void (*ext_rtc_alarm_isr)(void))
{
	INT32S ret;
	INT64U sec;
	INT8U  byValue;

	//vic_irq_disable(VIC_RTC_KC);
	rtc_irq_set(DISABLE);
#if _OPERATING_SYSTEM != _OS_NONE				// Soft Protect for critical section
#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSchedLock();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
    osSuspend();
#endif
#endif

    if (byEnable) {
    	sec = (rtc_time->rtc_day * 60 * 60 * 24) +
    			(rtc_time->rtc_hour * 60 * 60) +
    			(rtc_time->rtc_min * 60) +
    			rtc_time->rtc_sec;

    	sec <<= 15;
    	CHECK_RETURN(ext_rtc_write(R_EXTRTC_ALARM0, (sec >> 8) & 0xFF));
    	CHECK_RETURN(ext_rtc_write(R_EXTRTC_ALARM1, (sec >> 16) & 0xFF));
    	CHECK_RETURN(ext_rtc_write(R_EXTRTC_ALARM2, (sec >> 24) & 0xFF));
    	CHECK_RETURN(ext_rtc_write(R_EXTRTC_ALARM3, (sec >> 32) & 0xFF));
    	CHECK_RETURN(ext_rtc_write(R_EXTRTC_ALARM4, (sec >> 40) & 0xFF));
    }
    	//while(1);

	CHECK_RETURN(ext_rtc_read(R_EXTRTC_INT_ENABLE, &byValue));
	
	//DBG_PRINT("ext_rtc_alarm_enable()_1: byValue=0x%08X \r\n", byValue);
	if (byEnable)
    	byValue |= 0x02 ;
    else
        byValue &= ~0x02 ;

    CHECK_RETURN(ext_rtc_write(R_EXTRTC_INT_ENABLE, byValue));
    //DBG_PRINT("ext_rtc_alarm_enable()_2: byValue=0x%08X \r\n", byValue);
    

__exit:
#if _OPERATING_SYSTEM != _OS_NONE
#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSchedUnlock();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
    osResume();
#endif
#endif
	rtc_callback_set(EXT_RTC_ALARM_INT_INDEX, ext_rtc_alarm_isr);

	
	if (byEnable)
    	//vic_irq_enable(VIC_RTC_KC);
		rtc_irq_set(ENABLE);
	else
	    //vic_irq_disable(VIC_RTC_KC);
		rtc_irq_set(DISABLE);

	return ret;
}
/**
 * @brief   To enable or disable software IRTC wakeup
 * @param   byEnable: Enable or disable software IRTC wakeup
            1: Enable
            0:disable
 * @return 	none
 */
//INT32S ext_rtc_wakeup_int_enable(INT8U byEnable)
INT32S ext_rtc_wakeup_int_enable(INT8U byEnable,void (*ext_rtc_wakeup_isr)(void))
{
    INT32S ret;
    INT8U  byValue ;
	//vic_irq_disable(VIC_RTC_KC);
	rtc_irq_set(DISABLE);

#if _OPERATING_SYSTEM != _OS_NONE				// Soft Protect for critical section
	sw_rtc_lock();
#endif

	CHECK_RETURN(ext_rtc_read(R_EXTRTC_INT_ENABLE, &byValue));
	if (byEnable)
    	byValue |= 0x04 ;
    else
        byValue &= ~0x04 ;
    CHECK_RETURN(ext_rtc_write(R_EXTRTC_INT_ENABLE, byValue));

__exit:
#if _OPERATING_SYSTEM != _OS_NONE
	sw_rtc_unlock();
#endif
	rtc_callback_set(EXT_RTC_WAKEUP_INT_INDEX, ext_rtc_wakeup_isr);
	if (byEnable)
    	//vic_irq_enable(VIC_RTC_KC);
		rtc_irq_set(ENABLE);
	else
	    //vic_irq_disable(VIC_RTC_KC);
		rtc_irq_set(DISABLE);
	return ret;
}
/**
 * @brief   IRTC second interrupt enable
 * @param   byEnable: Enable or disable IRTC alarm
            1: Enable
            0:disable
            ext_rtc_sec_isr: IRTC second interrupt callback function
 * @return 	none
 */
INT32S ext_rtc_sec_int_enable(INT8U byEnable, void (*ext_rtc_sec_isr)(void))
{
    INT32S ret;
    INT8U  byValue ;
    //vic_irq_disable(VIC_RTC_KC);
	rtc_irq_set(DISABLE);
#if _OPERATING_SYSTEM != _OS_NONE				// Soft Protect for critical section
	sw_rtc_lock();
#endif

	CHECK_RETURN(ext_rtc_read(R_EXTRTC_INT_ENABLE, &byValue));
	if (byEnable)
    	byValue |= 0x01 ;
    else
        byValue &= ~0x01 ;
    CHECK_RETURN(ext_rtc_write(R_EXTRTC_INT_ENABLE, byValue));

__exit:
#if _OPERATING_SYSTEM != _OS_NONE
	sw_rtc_unlock();
#endif
	rtc_callback_set(EXT_RTC_SEC_INT_INDEX, ext_rtc_sec_isr);
	if (byEnable)
    	//vic_irq_enable(VIC_RTC_KC);
		rtc_irq_set(ENABLE);
	else
	    //vic_irq_disable(VIC_RTC_KC);
		rtc_irq_set(DISABLE);
	return ret;
}
/**
 * @brief   IRTC external io(IOA10~IOA15) interrupt enable
 * @param   byEnable: Enable or disable IRTC alarm
            1: Enable
            0:disable
            ext_rtc_io_isr: IRTC second interrupt callback function
 * @return 	none
 */
INT32S ext_rtc_io_int_enable(INT8U byEnable, void (*ext_rtc_io_isr)(void))
{
    INT32S ret=0;
    //INT8U  byValue ;
    //vic_irq_disable(VIC_RTC_KC);
	rtc_irq_set(DISABLE);
#if _OPERATING_SYSTEM != _OS_NONE				// Soft Protect for critical section
	sw_rtc_lock();
#endif
/*
	CHECK_RETURN(ext_rtc_read(R_EXTRTC_INT_ENABLE, &byValue));
	if (byEnable)
    	byValue |= 1<<2 ;
    else
        byValue &= ~(1<<2) ;
    CHECK_RETURN(ext_rtc_write(R_EXTRTC_INT_ENABLE, byValue));

__exit:
*/
#if _OPERATING_SYSTEM != _OS_NONE
	sw_rtc_unlock();
#endif
	ext_rtc_set_io_wakeup(byEnable);

	rtc_callback_set(EXT_RTC_IO_INT_INDEX, ext_rtc_io_isr);
	if (byEnable)
    	//vic_irq_enable(VIC_RTC_KC);
		rtc_irq_set(ENABLE);
	else
	    //vic_irq_disable(VIC_RTC_KC);
		rtc_irq_set(DISABLE);
	return ret;
}

/**
 * @brief   EXTRTC USB io interrupt enable
 * @param   byEnable: Enable or disable EXTRTC USB I/O interrupt
            1: Enable
            0:disable
            ext_rtc_usb_isr: EXTRTC USB IO interrupt callback function
 * @return 	none
 */
INT32S ext_rtc_usb_int_enable(INT8U byEnable, void (*ext_rtc_usb_isr)(void))
{
    INT32S ret;
    INT8U  byValue ;
    //vic_irq_disable(VIC_RTC_KC);
	rtc_irq_set(DISABLE);
#if _OPERATING_SYSTEM != _OS_NONE				// Soft Protect for critical section
	sw_rtc_lock();
#endif

	CHECK_RETURN(ext_rtc_read(0x48, &byValue));
	if (byEnable)
    	byValue |= 1<<0 ;
    else
        byValue &= ~(1<<0) ;
    CHECK_RETURN(ext_rtc_write(0x48, byValue));

__exit:
#if _OPERATING_SYSTEM != _OS_NONE
	sw_rtc_unlock();
#endif
	ext_rtc_set_io_wakeup(byEnable);

	rtc_callback_set(EXT_RTC_USB_INT_INDEX, ext_rtc_usb_isr);
	if (byEnable)
    	//vic_irq_enable(VIC_RTC_KC);
		rtc_irq_set(ENABLE);
	else
	    //vic_irq_disable(VIC_RTC_KC);
		rtc_irq_set(DISABLE);
	return ret;
}


void drv_l1_alarm_wakeup(INT32U alm_sec)
{
    t_rtc rtc_time, alm_time;
	t_rtc *p_rtc_time;
	t_rtc *p_alm_time;
	//INT8U temp = 0 ;
    p_rtc_time = &rtc_time;
    p_alm_time = &alm_time;

	ext_rtc_time_get(p_rtc_time);
	p_rtc_time->rtc_sec += alm_sec;
	if(p_rtc_time->rtc_sec >= 60)
	{
		p_rtc_time->rtc_sec -= 60;
		p_rtc_time->rtc_min += 1;
		if(p_rtc_time->rtc_min >= 60)
		{
			p_rtc_time->rtc_min -= 60;
			p_rtc_time->rtc_hour += 1;
			if(p_rtc_time->rtc_hour >= 24)
			{
				p_rtc_time->rtc_hour -= 24;
				p_rtc_time->rtc_day += 1;
			}
		}
	}
	p_alm_time->rtc_sec = p_rtc_time->rtc_sec;
	p_alm_time->rtc_min = p_rtc_time->rtc_min;
	p_alm_time->rtc_hour = p_rtc_time->rtc_hour;
	p_alm_time->rtc_day = p_rtc_time->rtc_day;

	ext_rtc_alarm_enable(p_alm_time, 1, NULL);
	ext_rtc_wakeup_int_enable(1, NULL);

	R_SYSTEM_POWER_CTRL0 &= ~0x1;       //power down
}

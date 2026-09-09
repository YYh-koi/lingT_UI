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

void (*fft_cbk_func)(void);


osMessageQId	fft_ack_q	= NULL;
osSemaphoreId 	fft_obj_hw_sem = NULL;


void FFT_IRQHandler(void);

/**
 * @brief   Lock FFT semaphore
 * @param   none
 * @return  none
 */
void drv_l1_fft_obj_lock(void)
{
    if(fft_obj_hw_sem)
        osSemaphoreWait(fft_obj_hw_sem, osWaitForever);
}

/**
 * @brief   Unlock FFT semaphore
 * @param   none
 * @return  none
 */
void drv_l1_fft_obj_unlock(void)
{
	if(fft_obj_hw_sem)
        osSemaphoreRelease(fft_obj_hw_sem);
}

/**
 * @brief   Get FFT interrupt status
 * @param   none
 * @return  FFT interrupt status
 */
static INT32U drv_l1_fft_intr_status_get(void)
{
	return (R_FFT_INTSTS&0x01);
}

/**
 * @brief   Clear FFT interrupt status
 * @param   none
 * @return  none
 */
static void drv_l1_fft_intr_status_clear(void)
{
	R_FFT_INTSTS |= 0x01;		//write 1 clear
}

/**
 * @brief   FFT isr
 * @param   none
 * @return  none
 */
static void fft_isr(void)
{
	if(fft_ack_q){
		INT32U	event;

		event = C_FFT_ACK;
		osMessagePut(fft_ack_q, (uint32_t)&event, osWaitForever);
	}

    if(fft_cbk_func!=0)
        (*fft_cbk_func)();

	drv_l1_fft_intr_status_clear();
}

/**
 * @brief   FFT IRQHandler
 * @param   none
 * @return  none
 */
void FFT_IRQHandler(void)
{
	fft_isr();
}

/**
 * @brief   FFT hardware enable
 * @param   none
 * @return  none
 */
void drv_l1_fft_hw_en(void)
{
    R_FFT_CTRL |= SET_FFT_HW_ENA;
}

/**
 * @brief   FFT wait done
 * @param   timeout: 0: osWaitForever; others: timeout in ms
 * @return  none
 */
INT32S drv_l1_fft_wait_done(INT32U timeout)
{
	osEvent	result;

	if(!fft_ack_q)
		return -1;

	if(timeout == 0)
		result = osMessageGet(fft_ack_q, osWaitForever);
	else
		result = osMessageGet(fft_ack_q, timeout);

	 if((result.status != osEventMessage) || (result.value.v == 0)) {
        DBG_PRINT("FFT Wait timeout (%d ms)\r\n", timeout);
        return -1;
    }
    return 0;
}


/**
 * @brief   FFT isr hook setting
 * @param   fft_cbk: FFT isr call back function
 * @return  none
 */
void drv_l1_fft_isr_hook_set( void(*fft_cbk)(void))
{
    fft_cbk_func = fft_cbk;
}

/**
 * @brief   FFT isr hook clear
 * @param   none
 * @return  none
 */
void drv_l1_fft_isr_hook_clear(void)
{
    if(fft_cbk_func != 0)
        fft_cbk_func = 0;
}


/**
 * @brief   FFT hardware disable
 * @param   none
 * @return  none
 */
void drv_l1_fft_hw_dis(void)
{
    R_FFT_CTRL &= ~SET_FFT_HW_ENA;
}

/**
 * @brief   FFT interrupt enable
 * @param   none
 * @return  none
 */
void drv_l1_fft_ena_intr(void)
{
    R_FFT_CTRL |= SET_FFT_INT_ENA;
}

/**
 * @brief   FFT interrupt disable
 * @param   none
 * @return  none
 */
void drv_l1_fft_dis_intr(void)
{
    R_FFT_CTRL &= ~SET_FFT_INT_ENA;
}

/**
 * @brief   FFT clock set
 * @param   enable: 1: enable FFT clock; 0: disable FFT clock
 * @return  none
 */
static void drv_l1_fft_clock_set(INT16U enable)
{
	drv_l1_clock_set_system_clk_en(CLK_EN6_FFT, enable);
	drv_l1_clock_set_system_clk_en(CLK_EN7_FFT_SRAM, enable);
}

/**
 * @brief   FFT init
 * @param   vic_fft: 0: use IRQ; other value: use FIQ
 * @return  none
 */
void drv_l1_fft_init(INT32U vic_fft)
{
	drv_l1_fft_clock_set(ENABLE);

    if(drv_l1_fft_intr_status_get() == 1 || (drv_l1_get_fft_ovf_sts()))
		drv_l1_fft_intr_status_clear();

    if(fft_ack_q == NULL) {
        osMessageQDef_t fft_a_q = { C_FFT_ISR_QUEUE_SIZE, sizeof(INT32U), 0 };

        fft_ack_q = osMessageCreate(&fft_a_q, NULL);
    }
    else{
		xQueueReset(fft_ack_q);
    }

	if(fft_obj_hw_sem == NULL) {
        osSemaphoreDef_t fft_obj_sem = { 0 };
        fft_obj_hw_sem = osSemaphoreCreate(&fft_obj_sem, 1);
    }

    R_FFT_START = 0;
    R_FFT_SCALE = 0;
    R_FFT_RADDR = (INT32S)NULL;
    R_FFT_WADDR = (INT32S)NULL;
    fft_cbk_func = 0;

    drv_l1_fft_hw_en();
    drv_l1_fft_ena_intr();

    vic_irq_register(VIC_FFT, FFT_IRQHandler);
    vic_irq_enable(VIC_FFT);
    if(vic_fft)
        vic_fiq_enable(VIC_FFT);


}

/**
 * @brief   FFT uninit
 * @param   none
 * @return  none
 */
void drv_l1_fft_uninit(void)
{
	vic_irq_disable(VIC_FFT);
    vic_fiq_disable(VIC_FFT);
	drv_l1_fft_isr_hook_clear();
	drv_l1_fft_dis_intr();
	drv_l1_fft_hw_dis();
	drv_l1_fft_clock_set(DISABLE);
}

/**
 * @brief   FFT set points
 * @param   FFT_N: FFT points: 32/64/128/256/512/1024/2048
 * @return  0 is success, -1 is fail.
 */
INT32S drv_l1_fft_set_points(INT32U FFT_N)
{
    INT32S ret = 0;
    INT32S value = 0;

    if(FFT_N==32)
        value |= SET_FFT_32P;
    else if(FFT_N==64)
        value |= SET_FFT_64P;
    else if(FFT_N==128)
        value |= SET_FFT_128P;
    else if(FFT_N==256)
        value|= SET_FFT_256P;
    else if(FFT_N==512)
        value |= SET_FFT_512P;
    else if(FFT_N==1024)
        value |= SET_FFT_1024P;
    else if(FFT_N==2048)
        value |= SET_FFT_2048P;
    else
        ret = -1;

    R_FFT_CTRL &= ~0x07;
    R_FFT_CTRL |= value;

    return ret;
}

/**
 * @brief   FFT set direction
 * @param   inv: 0: FFT mode; 1: IFFT mode
 * @return  0 is success, -1 is fail
 */
INT32S drv_l1_set_fft_dir(INT32U inv)
{
    INT32S ret=0;
    if(inv == C_DIR_IFFT)
        R_FFT_CTRL |= SET_DIR_IFFT;
    else if(inv == C_DIR_FFT)
        R_FFT_CTRL &= (~SET_DIR_IFFT);
    else
        ret = -1;

    return ret;
}

/**
 * @brief   FFT set scale
 * @param   stage: 0~5 to select which stage to scale
 * @param   div: 0: no scale; 1: scale to 1/2; 2: scale to 1/4
 * @return  0 is success, -1 is fail
 */
INT32S drv_l1_fft_set_scl(INT32U stage, INT32U div)
{
    INT32S ret = 0;

    if( (stage>5) ||( div!=2 && div!=4) )
        return -1;
    else{
        if(stage == 0){
            if(div == 2)
                R_FFT_SCALE = SET_SCL_STG_0_12;
            else
                R_FFT_SCALE = SET_SCL_STG_0_14;

        }else if(stage == 1){
            if(div == 2)
                R_FFT_SCALE = SET_SCL_STG_1_12;
            else
                R_FFT_SCALE = SET_SCL_STG_1_14;
        }else if(stage == 2){
            if(div == 2)
                R_FFT_SCALE = SET_SCL_STG_2_12;
            else
                R_FFT_SCALE = SET_SCL_STG_2_14;
        }else if(stage == 3){
            if(div == 2)
                R_FFT_SCALE = SET_SCL_STG_3_12;
            else
                R_FFT_SCALE = SET_SCL_STG_3_14;
        }else if(stage == 4){
            if(div == 2)
                R_FFT_SCALE = SET_SCL_STG_4_12;
            else
                R_FFT_SCALE = SET_SCL_STG_4_14;
        }else if(stage == 5){
            if(div == 2)
                R_FFT_SCALE = SET_SCL_STG_5_12;
            else
                R_FFT_SCALE = SET_SCL_STG_5_14;
        }
    }
    return ret;
}

/**
 * @brief   FFT input buffer address
 * @param   addr: input buffer address
 * @return  none
 */
void drv_l1_fft_in_addr(INT32U *addr)
{
    R_FFT_RADDR = (INT32S)addr;
}

/**
 * @brief   FFT output buffer address
 * @param   addr: output buffer address
 * @return  none
 */
void drv_l1_fft_out_addr(INT32U *addr)
{
    R_FFT_WADDR = (INT32S)addr;
}

/**
 * @brief   FFT set input format
 * @param   in_fmt: 0: 16b real + imagine input; 1: 16b real only input; 2: 24b real + imagine input; 3: 24b real only input
 * @return  0 is success, -1 is fail
 */
INT32S drv_l1_fft_set_in_fmt(FFT_IN_FMT_ENUM in_fmt)
{
    INT32S ret=0;

	R_FFT_CTRL &= ~0x30;

    if(in_fmt == C_IN_R_16b)
        R_FFT_CTRL |= SET_16b_FFTIN_R;
    else if(in_fmt == C_IN_R_I_16b)
        R_FFT_CTRL |= SET_16b_FFTIN_R_AND_I;
    else if(in_fmt == C_IN_R_24b)
        R_FFT_CTRL |= SET_24b_FFTIN_R;
    else if(in_fmt == C_IN_R_I_24b)
        R_FFT_CTRL |= SET_24b_FFTIN_R_AND_I;
    else
        ret = -1;

    return ret;

}

/**
 * @brief   FFT set output format
 * @param   out_fmt: 0: 16b real + imagine input; 1: 16b real only input; 2: 24b real + imagine input; 3: 24b real only input
 * @return  0 is success, -1 is fail
 */
INT32S drv_l1_fft_set_out_fmt(FFT_OUT_FMT_ENUM out_fmt)
{
    INT32S ret=0;

	R_FFT_CTRL &= ~0xC0;

    if(out_fmt == C_OUT_R_16b)
        R_FFT_CTRL |= SET_16b_FFTOUT_R;
    else if(out_fmt == C_OUT_R_I_16b)
        R_FFT_CTRL |= SET_16b_FFTOUT_R_AND_I;
    else if(out_fmt == C_OUT_R_24b)
        R_FFT_CTRL |= SET_24b_FFTOUT_R;
    else if(out_fmt == C_OUT_R_I_24b)
        R_FFT_CTRL |= SET_24b_FFTOUT_R_AND_I;
    else
        ret = -1;

    return ret;

}

/**
 * @brief   FFT get overflow status
 * @param   none
 * @return  overflow status: bit5~0 corresponding to stage5~0, bit6 is output stage. 1 means overflow is occured.
 */
INT32U drv_l1_get_fft_ovf_sts(void)
{
	INT32U ovf_sts = R_FFT_OVFSTS&0x3F;
	return ovf_sts;
}

/**
 * @brief   FFT start
 * @param   none
 * @return  none
 */
void drv_l1_fft_start(void)
{
	R_FFT_START |= SET_FFT_TRIG;
}



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

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
extern uint32_t SystemCoreClock;

/*
 * @brief: Initial the Ture Random Number Generator
 * @param: None
 * @return: None
*/
void drv_l1_trng_init(void)
{
    drv_l1_clock_set_system_clk_en(CLK_EN5_TRNG, 1);

    R_TRNG_CTRL1 = (RING_LEN_9_CELLS<<LEN_SEL0)|(RING_LEN_13_CELLS<<LEN_SEL1)|(RING_LEN_15_CELLS<<LEN_SEL2)|(RING_LEN_21_CELLS<<LEN_SEL3)|0x1;
    R_TRNG_CTRL1 |= RING_4_MODE; //4 Xored ring enable
    R_TRNG_CTRL1 |= PULSE_EN; //pulse enable
}

/*
 * @brief: Uninitial the Ture Random Number Generator
 * @param: None
 * @return: None
*/
void drv_l1_trng_uninit(void)
{
    R_TRNG_CTRL1 = 0;
    drv_l1_clock_set_system_clk_en(CLK_EN5_TRNG, 0);
}
/*
 * @brief: Use CPU mode to get Ture Random Number data,
 * if you need long length random data (ex:more than 32 byte), please use the DMA mode(drv_l1_trng_get_dma).
 * @param: buffer: pointer of output buffer
 * @param: len: TRNG data length in byte
 * @return: 0: success, -1: fail
*/
INT32S drv_l1_trng_get(INT8U *buffer, INT32U len)
{
    INT32U llen;
    INT32U t1,t2;
    INT32U timeout;
    INT32U *ptr = (INT32U*) buffer;
    INT8U *ptr1;

    R_TRNG_CTRL1 &= ~DMA_EN;

    llen = len >> 2;

    while (llen > 0) {
        /* It needs at least 32 bit*40 = 1280 system clock, if system clock = 1MHz (1us) and needs 1280us timeout
        1 tiny count = 2.6us */
        timeout = 1000;
        t1 = tiny_counter_get();
        while (timeout) {
            if ((R_TRNG_STATUS & 0x2) == 0) {
                break;
            }
            t2 = tiny_counter_get();
            if (t1 != t2) {
                t1 = t2;
                if (timeout-- == 0) return -1;
            }
        }
        *ptr++ = R_TRNG_DATA;
        llen--;
    }

    llen = len % 0x4;
    ptr1 = (INT8U *) ptr;

    while (llen > 0) {
        timeout = 1000;
        t1 = tiny_counter_get();
        while (timeout) {
            if ((R_TRNG_STATUS & 0x2) == 0) {
                break;
            }
            t2 = tiny_counter_get();
            if (t1 != t2) {
                t1 = t2;
                if (timeout-- == 0) return -1;
            }
        }
        *ptr1++ = (INT8U) R_TRNG_DATA;
        llen--;
    }

    return 0;
}

/*
 * @brief: Use DMA mode to get Ture Random Number data
 * @param: buffer: pointer of output buffer, please align 64 byte address
 * @param: len: TRNG data length in byte
 * @return: 0: success, -1: fail
*/
INT32S drv_l1_trng_get_dma(INT8U *buffer, INT32U len)
{
    DMA_STRUCT dma_struct={0};
	volatile INT8S done;
	INT32U timeout = 5, timeout_tick, t1, t2;

    R_TRNG_CTRL1 |= DMA_EN;//TRNG DMA mode enable
	dma_struct.s_addr = (INT32U) P_TRNG_DMA_DATA;
	dma_struct.t_addr = (INT32U) buffer;
	dma_struct.width = DMA_DATA_WIDTH_1BYTE;
	dma_struct.count = len;
	dma_struct.timeout = 0; // no DMA timeout, use S/W timeout
    done = C_DMA_STATUS_WAITING;
    dma_struct.notify = &done;

    timeout += 40*len*8/(SystemCoreClock/1000); //in ms, 1 random bit need wait for 40 system clock
    timeout_tick = pdMS_TO_TICKS(timeout);

    if (drv_l1_dma_transfer(&dma_struct)) {
        return -1;
    }

    t1 = xTaskGetTickCount();
    while(timeout_tick) {
        if (done == C_DMA_STATUS_DONE) {
            return 0;
        }
        t2 = xTaskGetTickCount();
        if (t1 != t2) {
            t1 = t2;
            timeout_tick--;
        }
    }

    drv_l1_dma_enforce_timeout((INT32U)dma_struct.channel);
    return -1;
}

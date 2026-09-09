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

/**
 * @brief   initialize bottom level required and common use hardware, resources
 * @param   none
 * @return 	none
 */
void drv_l1_init(void)
{
    SystemCoreClock = drv_l1_clock_get_system_clock_freq();

    MCLK = SystemCoreClock;
    MHZ = MCLK /1000000;

    exception_table_init();

    vic_init();							// Initiate Interrupt controller

    timer_init();						// Initiate Timer
    timerD_counter_init();              // Tiny Counter Initial (1 tiny count == 1.15 us)

    #if UART0_ENABLE == 1
    drv_l1_uart0_init();
    drv_l1_uart0_buad_rate_set(UART_BAUD_RATE);
    drv_l1_uart0_tx_enable();
    drv_l1_uart0_rx_enable();
    #endif

    #if UART1_ENABLE == 1
    drv_l1_uart1_init();
    drv_l1_uart1_buad_rate_set(UART_BAUD_RATE);
    drv_l1_uart1_tx_enable();
    drv_l1_uart1_rx_enable();
    #endif

    #if UART2_ENABLE == 1
    drv_l1_uart2_init();
    drv_l1_uart2_buad_rate_set(UART_BAUD_RATE);
    drv_l1_uart2_tx_enable();
    drv_l1_uart2_rx_enable();
    #endif

    drv_l1_rtc_init(0,0);

    drv_l1_dma_init();					// Initiate DMA controller

	drv_l1_dac_init(0,0);

#if defined(C_RUN_ON_SPIFC) && (C_RUN_ON_SPIFC == 1)
#else
    R_SPIFC_CTRL &= (~(1<<0)); // Note: this will pull CS(IO_D0) to low, SCK(IO_D1) to high

    R_DSI_OP_CTRL &= ~(0x1<<31); // MIPI TX PHY reset, reduce power consumption
#endif
}


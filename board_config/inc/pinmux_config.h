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

#ifndef __PINMUX_CONFIG__
#define __PINMUX_CONFIG__

#include "project.h"

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/
/* mux selection define */
#define MUX0        0
#define MUX1        1
#define MUX2        2
#define MUX3        3
#define MUX4        4
#define MUX5        5
#define MUX6        6
#define MUX7        7

/************************************************************/
/*						Pinmux	setting						*/
/************************************************************/
/* UART0 PINMUX */
#define UART0_MUX0								MUX0//UART0_TX_RX__IOB5_IOB4
#define UART0_MUX1								MUX1//UART0_TX_RX__IOC12_IOC13
#define UART0_MUX2								MUX2//UART0_TX_RX__IOD5_IOD4
#define UART0_MUX3								MUX3//UART0_TX_RX__IOA3_IOA2

/* UART1 PINMUX */
#define UART1_MUX0								MUX0//UART1_TX_RX__IOB7_IOB6
#define UART1_MUX1								MUX1//UART1_TX_RX__IOA5_IOA4
#define UART1_MUX2								MUX2//UART1_TX_RX__IOD9_IOD8
#define UART1_MUX3								MUX3//UART1_TX_RX__IOA11_IOA10

/* UART2 PINMUX */
#define UART2_MUX0								MUX0//UART2_TX_RX__IOB12_IOB11
#define UART2_MUX1								MUX1//UART2_TX_RX__IOA7_IOA6
#define UART2_MUX2								MUX2//UART2_TX_RX__IOC3_IOC2
#define UART2_MUX3								MUX3//UART2_TX_RX__IOC5_IOC4

/* RS485 PINMUX */
#define RS485_IOB5_MUX0                         MUX0//IOB5_GPIO
#define RS485_IOB5_MUX1                         MUX1//IOB5_UART0_RS485_RTS
#define RS485_IOB5_MUX2                         MUX2//IOB5_UART1_RS485_RTS
#define RS485_IOB5_MUX3                         MUX3//IOB5_UART2_RS485_RTS

#define RS485_IOB6_MUX0                         MUX0//IOB6_GPIO
#define RS485_IOB6_MUX1                         MUX1//IOB6_UART0_RS485_RTS
#define RS485_IOB6_MUX2                         MUX2//IOB6_UART1_RS485_RTS
#define RS485_IOB6_MUX3                         MUX3//IOB6_UART2_RS485_RTS

#define RS485_IOC14_MUX0                        MUX0//IOC14_GPIO
#define RS485_IOC14_MUX1                        MUX1//IOC14_UART0_RS485_RTS
#define RS485_IOC14_MUX2                        MUX2//IOC14_UART1_RS485_RTS
#define RS485_IOC14_MUX3                        MUX3//IOC14_UART2_RS485_RTS

#define RS485_IOD12_MUX0                        MUX0//IOD12_GPIO
#define RS485_IOD12_MUX1                        MUX1//IOD12_UART0_RS485_RTS
#define RS485_IOD12_MUX2                        MUX2//IOD12_UART1_RS485_RTS
#define RS485_IOD12_MUX3                        MUX3//IOD12_UART2_RS485_RTS

#define RS485_IOE5_MUX0                         MUX0//IOE5_GPIO
#define RS485_IOE5_MUX1                         MUX1//IOE5_UART0_RS485_RTS
#define RS485_IOE5_MUX2                         MUX2//IOE5_UART1_RS485_RTS
#define RS485_IOE5_MUX3                         MUX3//IOE5_UART2_RS485_RTS

#define RS485_IOE6_MUX0                         MUX0//IOE6_GPIO
#define RS485_IOE6_MUX1                         MUX1//IOE6_UART0_RS485_RTS
#define RS485_IOE6_MUX2                         MUX2//IOE6_UART1_RS485_RTS
#define RS485_IOE6_MUX3                         MUX3//IOE6_UART2_RS485_RTS

/* EXTA IRQ PINMUX */
#define EXTA_IRQ_MUX0							MUX0//EXTA_IRQ__IOA10
#define EXTA_IRQ_MUX1							MUX1//EXTA_IRQ__IOC12
#define EXTA_IRQ_MUX2							MUX2//EXTA_IRQ__IOD7
#define EXTA_IRQ_MUX3							MUX3//EXTA_IRQ__IOD12

/* EXTB IRQ PINMUX */
#define EXTB_IRQ_MUX0							MUX0//EXTB_IRQ__IOA11
#define EXTB_IRQ_MUX1							MUX1//EXTB_IRQ__IOC13
#define EXTB_IRQ_MUX2							MUX2//EXTB_IRQ__IOD8
#define EXTB_IRQ_MUX3							MUX3//EXTB_IRQ__IOD14

/* EXTC IRQ PINMUX */
#define EXTC_IRQ_MUX0							MUX0//EXTC_IRQ__IOA12
#define EXTC_IRQ_MUX1							MUX1//EXTC_IRQ__IOC14
#define EXTC_IRQ_MUX2							MUX2//EXTC_IRQ__IOD9
#define EXTC_IRQ_MUX3							MUX3//EXTC_IRQ__IOD11

/* EXTD IRQ PINMUX */
#define EXTD_IRQ_MUX0							MUX0//EXTD_IRQ__IOA13
#define EXTD_IRQ_MUX1							MUX1//EXTD_IRQ__IOF2
#define EXTD_IRQ_MUX2							MUX2//EXTD_IRQ__IOF3
#define EXTD_IRQ_MUX3							MUX3//EXTD_IRQ__IOE10

/* EXTE IRQ PINMUX */
#define EXTE_IRQ_MUX0							MUX0//EXTE_IRQ__IOB6
#define EXTE_IRQ_MUX1							MUX1//EXTE_IRQ__IOF1
#define EXTE_IRQ_MUX2							MUX2//EXTE_IRQ__IOF4
#define EXTE_IRQ_MUX3							MUX3//EXTE_IRQ__IOE11

/* EXTF IRQ PINMUX */
#define EXTF_IRQ_MUX0							MUX0//EXTF_IRQ__IOB7
#define EXTF_IRQ_MUX1							MUX1//EXTF_IRQ__IOF0
#define EXTF_IRQ_MUX2							MUX2//EXTF_IRQ__IOF5
#define EXTF_IRQ_MUX3							MUX3//EXTF_IRQ__IOE12

/* EXTG IRQ PINMUX */
#define EXTG_IRQ_MUX0							MUX0//EXTG_IRQ__IOB8
#define EXTG_IRQ_MUX1							MUX1//EXTG_IRQ__IOE6
#define EXTG_IRQ_MUX2							MUX2//EXTG_IRQ__IOA14
#define EXTG_IRQ_MUX3							MUX3//EXTG_IRQ__IOA2

/* EXTH IRQ PINMUX */
#define EXTH_IRQ_MUX0							MUX0//EXTH_IRQ__IOC2
#define EXTH_IRQ_MUX1							MUX1//EXTH_IRQ__IOE7
#define EXTH_IRQ_MUX2							MUX2//EXTH_IRQ__IOA15
#define EXTH_IRQ_MUX3							MUX3//EXTH_IRQ__IOA4

/* EXTI IRQ PINMUX */
#define EXTI_IRQ_MUX0							MUX0//EXTI_IRQ__IOC3
#define EXTI_IRQ_MUX1							MUX1//EXTI_IRQ__IOE8
#define EXTI_IRQ_MUX2							MUX2//EXTI_IRQ__IOB0
#define EXTI_IRQ_MUX3							MUX3//EXTI_IRQ__IOA5

/* EXTJ IRQ PINMUX */
#define EXTJ_IRQ_MUX0							MUX0//EXTJ_IRQ__IOB2
#define EXTJ_IRQ_MUX1							MUX1//EXTJ_IRQ__IOE9
#define EXTJ_IRQ_MUX2							MUX2//EXTJ_IRQ__IOB1
#define EXTJ_IRQ_MUX3							MUX3//EXTJ_IRQ__IOD4

/* EXTK IRQ PINMUX */
#define EXTK_IRQ_MUX0							MUX0//EXTK_IRQ__IOB3
#define EXTK_IRQ_MUX1							MUX1//EXTK_IRQ__IOE2
#define EXTK_IRQ_MUX2							MUX2//EXTK_IRQ__IOB11
#define EXTK_IRQ_MUX3							MUX3//EXTK_IRQ__IOD5

/* EXTL IRQ PINMUX */
#define EXTL_IRQ_MUX0							MUX0//EXTL_IRQ__IOD15
#define EXTL_IRQ_MUX1							MUX1//EXTL_IRQ__IOE3
#define EXTL_IRQ_MUX2							MUX2//EXTL_IRQ__IOB12
#define EXTL_IRQ_MUX3							MUX3//EXTL_IRQ__IOD6

/* Timer A CCP PINMUX */
#define TIMERA_CCP_MUX0						    MUX0//TIMER_A__IOB8
#define TIMERA_CCP_MUX1						    MUX1//TIMER_A__IOD7
#define TIMERA_CCP_MUX2						    MUX2//TIMER_A__IOA8
#define TIMERA_CCP_MUX3						    MUX3//TIMER_A__IOE0

/* Timer B CCP PINMUX */
#define TIMERB_CCP_MUX0						    MUX0//TIMER_B__IOB9
#define TIMERB_CCP_MUX1						    MUX1//TIMER_B__IOD8
#define TIMERB_CCP_MUX2						    MUX2//TIMER_B__IOA9
#define TIMERB_CCP_MUX3						    MUX3//TIMER_B__IOE1

/* Timer C CCP PINMUX */
#define TIMERC_CCP_MUX0						    MUX0//TIMER_C__IOB10
#define TIMERC_CCP_MUX1						    MUX1//TIMER_C__IOD9
#define TIMERC_CCP_MUX2						    MUX2//TIMER_C__IOA10
#define TIMERC_CCP_MUX3						    MUX3//TIMER_C__IOE2

/* Timer D CCP PINMUX */
#define TIMERD_CCP_MUX0						    MUX0//TIMER_D__IOC12
#define TIMERD_CCP_MUX1						    MUX1//TIMER_D__IOD12
#define TIMERD_CCP_MUX2						    MUX2//TIMER_D__IOA11
#define TIMERD_CCP_MUX3						    MUX3//TIMER_D__IOE3

/* Timer E CCP PINMUX */
#define TIMERE_CCP_MUX0						    MUX0//TIMER_E__IOC13
#define TIMERE_CCP_MUX1						    MUX1//TIMER_E__IOD14
#define TIMERE_CCP_MUX2						    MUX2//TIMER_E__IOA12
#define TIMERE_CCP_MUX3						    MUX3//TIMER_E__IOE4

/* Timer F CCP PINMUX */
#define TIMERF_CCP_MUX0						    MUX0//TIMER_F__IOD4
#define TIMERF_CCP_MUX1						    MUX1//TIMER_F__IOD15
#define TIMERF_CCP_MUX2						    MUX2//TIMER_F__IOA13
#define TIMERF_CCP_MUX3						    MUX3//TIMER_F__IOE5

/* Timer G CCP PINMUX */
#define TIMERG_CCP_MUX0						    MUX0//TIMER_G__IOD5
#define TIMERG_CCP_MUX1						    MUX1//TIMER_G__IOB6
#define TIMERG_CCP_MUX2						    MUX2//TIMER_G__IOA14
#define TIMERG_CCP_MUX3						    MUX3//TIMER_G__IOE6

/* Timer H CCP PINMUX */
#define TIMERH_CCP_MUX0						    MUX0//TIMER_H__IOD6
#define TIMERH_CCP_MUX1						    MUX1//TIMER_H__IOB7
#define TIMERH_CCP_MUX2						    MUX2//TIMER_H__IOA15
#define TIMERH_CCP_MUX3						    MUX3//TIMER_H__IOE7

/* I2C0 PINMUX */
#define I2C0_MUX0								MUX0//I2C0_SCL_SDA__IOB4_IOB5
#define I2C0_MUX1								MUX1//I2C0_SCL_SDA__IOC12_IOC13
#define I2C0_MUX2								MUX2//I2C0_SCL_SDA__IOA8_IOA9
#define I2C0_MUX3								MUX3//I2C0_SCL_SDA__IOB14_IOB15

/* I2C1 PINMUX */
#define I2C1_MUX0								MUX0//I2C1_SCL_SDA__IOC0_IOC1
#define I2C1_MUX1								MUX1//I2C1_SCL_SDA__IOD4_IOD5
#define I2C1_MUX2								MUX2//I2C1_SCL_SDA__IOC6_IOC7

/* I2C2 PINMUX */
#define I2C2_MUX0								MUX0//I2C2_SCL_SDA__IOD14_IOD15
#define I2C2_MUX1								MUX1//I2C1_SCL_SDA__IOD12_IOD10
#define I2C2_MUX2								MUX2//I2C1_SCL_SDA__IOA10_IOA11
#define I2C2_MUX3								MUX3//I2C1_SCL_SDA__IOB10_IOB13

/* SPI0 PINMUX */
#define SPI0_MUX0								MUX0//SPI0_CS_CLK_TX_RX__IOD6_IOD7_IOD8_IOD9_IOC0_IOC1
#define SPI0_MUX1								MUX1//SPI0_CS_CLK_TX_RX__IOD0_IOD1_IOD2_IOD3_IOD4_IOD5

/* SPI1 PINMUX */
#define SPI1_MUX0								MUX0//SPI1_CS_CLK_TX_RX__IOC8_IOC9_IOC10_IOC11_IOC2_IOC3
#define SPI1_MUX1								MUX1//SPI1_CS_CLK_TX_RX__IOD10_IOD11_IOD12_IOD13_IOD14_IOD15

/* SPIFC PINMUX */
#define SPIFC_MUX0								MUX0//SPIFC_CS_CLK_RX0_3__IOD0_IOD1_IOD2_IOD3_IOD4_IOD5
#define SPIFC_MUX1								MUX1//SPIFC_CS_CLK_RX0_3__IOD7_IOA14_IOA12_IOD8_IOD9_IOA15

/* SDC0 PINMUX */
#define SDC0_MUX0								MUX0//SDC0_CMD_CLK_DATA0_3__IOA2_IOA3_IOA5_IOA6_IOA7_IOA4
#define SDC0_MUX1								MUX1//SDC0_CMD_CLK_DATA0_3__IOB14_IOB15_IOB11_IOB12_IOB13_IOB10
#define SDC0_MUX2								MUX2//SDC0_CMD_CLK_DATA0_3__IOC6_IOC7_IOC9_IOC10_IOC11_IOC8
#define SDC0_MUX3                               MUX3//SDC0_CMD_CLK_DATA0_3__IOE0_IOE1_IOE3_IOE4_IOE5_IOE2
#define SDC0_MUX4                               MUX4//SDC0_CMD_CLK_DATA0_3__IOF0_IOF1_IOF3_IOF4_IOF5_IOF2
#define SDC0_MUX5                               MUX5//SDC0_CMD_CLK_DATA0_3__IODB0_IOB1_IOB3_IOB4_IOB5_IOB2
#define SDC0_MUX6                               MUX6//SDC0_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOC14_IOC15_IOC13, conflict with JTAG
#define SDC0_MUX7                               MUX7//SDC0_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOD14_IOD15_IOD12

/* SDC1 PINMUX */
#define SDC1_MUX0								MUX0//SDC1_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOD14_IOD15_IOD12
#define SDC1_MUX1								MUX1//SDC1_CMD_CLK_DATA0_3__IOB0_IOB1_IOB3_IOB4_IOB5_IOB2
#define SDC1_MUX2								MUX2//SDC1_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOC14_IOC15_IOC13
#define SDC1_MUX3								MUX3//SDC1_CMD_CLK_DATA0_3__IOF0_IOF1_IOF3_IOF4_IOF5_IOF2
#define SDC1_MUX4								MUX4//SDC1_CMD_CLK_DATA0_3__IOB14_IOB15_IOB11_IOB12_IOB13_IOB10

/* I2S0 PINMUX */
#define I2S0_MUX0_RX							MUX0//I2S_MCLK_LR_BCK_TX__IOB0_IOB1_IOB2_IOB3
#define I2S0_MUX1_TX							MUX1//I2S_MCLK_LR_BCK_TX__IOB4_IOB5_IOB6_IOB7

/* I2S1 PINMUX */
#define I2S1_MUX0_RX							MUX0//I2S_MCLK_LR_BCK_TX__IOB8_IOB9_IOB10_IOB11
#define I2S1_MUX1_TX							MUX1//I2S_MCLK_LR_BCK_TX__IOB12_IOB13_IOB14_IOB15

/* I2S2 PINMUX */
#define I2S2_MUX0_RX							MUX0//I2S_MCLK_LR_BCK_TX__IOC4_IOC5_IOC6_IOC7
#define I2S2_MUX1_TX							MUX1//I2S_MCLK_LR_BCK_TX__IOC12_IOC13_IOD14_IOD15

/* I2S3 PINMUX */
#define I2S3_MUX0_RX							MUX0//I2S_MCLK_LR_BCK_TX__IOD6_IOD7_IOD8_IOD9
#define I2S3_MUX1_TX							MUX1//I2S_MCLK_LR_BCK_TX__IOD10_IOD11_IOD12_IOD13

/* NAND DATA PINMUX */
#define NADN_DATA_MUX0						    MUX0// NAND_IO_IOD_IOB,  NAND_CTRL__IOD0_IOD5_DATA__IOB8_IOB15
#define NADN_DATA_MUX1						    MUX1// NAND_IO_IOD_IOC,  NAND_CTRL__IOD0_IOD5_DATA__IOC4_IOC11
#define NADN_DATA_MUX2						    MUX2// NAND_IO_IOD_IOCB, NAND_CTRL__IOD0_IOD5_DATA__IOC0_IOC5_IOB8_IOB9
#define NADN_DATA_MUX3						    MUX3// NAND_IO_IOD_IOF,  NAND_CTRL__IOD0_IOD5_DATA__IOF0_IOF7
#define NADN_DATA_MUX4						    MUX4// NAND_IO_IOA_IOC,  NAND_CTRL__IOA0_IOA2_IOA5_IOA7__DATA__IOC4_IOC11
#define NADN_DATA_MUX5						    MUX5// NAND_IO_IOA_IOB,  NAND_CTRL__IOA0_IOA2_IOA5_IOA7__DATA__IOB8_IOB15
#define NADN_DATA_MUX6						    MUX6// NAND_IO_IOA_IOCB, NAND_CTRL__IOA0_IOA2_IOA5_IOA7__DATA__IOC0_IOC5_IOB8_IOB9
#define NADN_DATA_MUX7						    MUX7// NAND_IO_IOA_IOF,  NAND_CTRL__IOA0_IOA2_IOA5_IOA7__DATA__IOF0_IOF7

/* TFT PIN MUX	*/
#define TFT_DATA0_7_MUX0					    MUX0//TFT_DATA_0_7__IOA0_IOA7
#define TFT_DATA0_7_MUX1					    MUX1//TFT_DATA_0_7__IOE0_IOE7

#define TFT_DATA8_15_MUX0					    MUX0//TFT_DATA_8_15__IOA8_IOA15
#define TFT_DATA8_15_MUX1					    MUX1//TFT_DATA_8_15__IOE0_E7
#define TFT_DATA8_15_MUX2					    MUX2//TFT_DATA_8_15__IOC0_IOC5_IOD5_IOD7
#define TFT_DATA8_15_MUX3					    MUX3//TFT_DATA_8_15__IOB14_IOB15_IOA10_IOA15

#define TFT_DATA16_23_MUX0					    MUX0//TFT_DATA_16_23__IOF0_IOF7
#define TFT_DATA16_23_MUX1					    MUX1//TFT_DATA_16_23__IOB6_13

#define TFT_TE_MUX0					            MUX0//TFT_TE_IOF2
#define TFT_TE_MUX1					            MUX1//TFT_TE_IOB4

#define TFT_CTRL_MUX0							MUX0//TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOB0_IOB1_IOB2_IOB3
#define TFT_CTRL_MUX1							MUX1//TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOC8_IOC9_IOC10_IOC11
#define TFT_CTRL_MUX2							MUX2//TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOF4_IOF5_IOF6_IOF7

/* TFT2 PIN MUX	*/
#define TFT2_DATA0_7_MUX0					    MUX0//TFT_DATA_0_7__IOA0_IOA7
#define TFT2_DATA0_7_MUX1					    MUX1//TFT_DATA_0_7__IOE0_IOE7

#define TFT2_DATA8_15_MUX0					    MUX0//TFT_DATA_8_15__IOA8_IOA15
#define TFT2_DATA8_15_MUX1					    MUX1//TFT_DATA_8_15__IOE0_E7
#define TFT2_DATA8_15_MUX2					    MUX2//TFT_DATA_8_15__IOD7_IOD5_IOC5_IOC0
#define TFT2_DATA8_15_MUX3                      MUX3//TFT2_DATA_8_15__IOB14_IOB15_IOA10_IOA15

#define TFT2_DATA16_23_MUX0					    MUX0//TFT_DATA_16_23__IOF0_IOF7
#define TFT2_DATA16_23_MUX1					    MUX1//TFT_DATA_16_23__IOB6_13

#define TFT2_TE_MUX0					        MUX0//TFT_TE_IOF2
#define TFT2_TE_MUX1					        MUX1//TFT_TE_IOB4

#define TFT2_CTRL_MUX0							MUX0//TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOB0_IOB1_IOB2_IOB3
#define TFT2_CTRL_MUX1							MUX1//TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOC8_IOC9_IOC10_IOC11
#define TFT2_CTRL_MUX2							MUX2//TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOF4_IOF5_IOF6_IOF7

/* TFT1 SPI PIN MUX	*/
#define TFT1_SPI_DATA0_3_MUX0					MUX0//TFT1_SPI_DATA_0_3__IOB0_IOB3
#define TFT1_SPI_DATA0_3_MUX1					MUX1//TFT1_SPI_DATA_0_3__IOC8_IOC11
#define TFT1_SPI_DATA0_3_MUX2					MUX2//TFT1_SPI_DATA_0_3__IOF4_IOF7

#define TFT1_SPI_TE_MUX0					    MUX0//TFT1_SPI_TE_IOB4
#define TFT1_SPI_TE_MUX1					    MUX1//TFT1_SPI_TE_IOF2

/* TFT2 SPI PIN MUX	*/
#define TFT2_SPI_DATA0_3_MUX0					MUX0//TFT2_SPI_DATA_0_3__IOB0_IOB3
#define TFT2_SPI_DATA0_3_MUX1					MUX1//TFT2_SPI_DATA_0_3__IOC8_IOC11
#define TFT2_SPI_DATA0_3_MUX2					MUX2//TFT2_SPI_DATA_0_3__IOF4_IOF7

#define TFT2_SPI_TE_MUX0					    MUX0//TFT2_SPI_TE_IOB4
#define TFT2_SPI_TE_MUX1					    MUX1//TFT2_SPI_TE_IOF2

/*	CSI0 PIN MUX	*/
#define CSI_DATA2_9_MUX0					    MUX0//CSI_DATA_2_9__IOC0_IOC7
#define CSI_DATA2_9_MUX1					    MUX1//CSI_DATA_2_9__IOB8_IOB15
#define CSI_DATA2_9_MUX2					    MUX2//CSI_DATA_2_9__IOE0_IOE7
#define CSI_DATA2_9_MUX3                        MUX3//CSI_DATA_2_9__IOF0_IOF7

#define CSI_CTRL_MUX0							MUX0//CSI_CTRL_CLKI_HSYNC_VSYNC__IOC8_IOC10_IOC11
#define CSI_CTRL_MUX1							MUX1//CSI_CTRL_CLKI_HSYNC_VSYNC__IOD6_IOD8_IOD9

/*	CDSP PIN MUX	*/
#define CDSP_DATA2_9_MUX0					    MUX0//CSI_DATA_2_9__IOC0_IOC7
#define CDSP_DATA2_9_MUX1					    MUX1//CSI_DATA_2_9__IOB8_IOB15
#define CDSP_DATA2_9_MUX2					    MUX2//CSI_DATA_2_9__IOE0_IOE7
#define CDSP_DATA2_9_MUX3					    MUX2//CSI_DATA_2_9__IOF0_IOF7

#define CDSP_DATA0_1_MUX0					    MUX0//CSI_DATA_0_1__IOB6_IOB7
#define CDSP_DATA0_1_MUX1					    MUX1//CSI_DATA_0_1__IOB4_IOB5
#define CDSP_DATA0_1_MUX2					    MUX2//CSI_DATA_0_1__IOC12_IOC13
#define CDSP_DATA0_1_NONE					    MUX3//NONE

#define CDSP_CTRL_MUX0							MUX0//CSI_CTRL_CLKI_HSYNC_VSYNC__IOC8_IOC10_IOC11
#define CDSP_CTRL_MUX1							MUX1//CSI_CTRL_CLKI_HSYNC_VSYNC__IOD6_IOD8_IOD9

/*	CSI2 PIN MUX	*/
#define CSI2_DATA2_9_MUX0					    MUX0//CSI_DATA_2_9__IOC0_IOC7
#define CSI2_DATA2_9_MUX1					    MUX1//CSI_DATA_2_9__IOB8_IOB15
#define CSI2_DATA2_9_MUX2					    MUX2//CSI_DATA_2_9__IOE0_IOE7
#define CSI2_DATA2_9_MUX3                       MUX3//CSI_DATA_2_9__IOF0_IOF7

#define CSI2_CTRL_MUX0							MUX0//CSI_CTRL_CLKI_HSYNC_VSYNC__IOC8_IOC10_IOC11
#define CSI2_CTRL_MUX1							MUX1//CSI_CTRL_CLKI_HSYNC_VSYNC__IOD6_IOD8_IOD9

/*CLKO SOURCE*/
#define CLKO_FROM_CDSP                          MUX0
#define CLKO_FROM_CSI                           MUX1

/*CDSP CLKO PIN MUX*/
#define CDSP_CLKO_POS_MUX0                      MUX0 //CLKO_IOD7
#define CDSP_CLKO_POS_MUX1                      MUX1 //CLKO_IOD12
#define CDSP_CLKO_POS_MUX2                      MUX2 //CLKO_IOC9
#define CDSP_CLKO_POS_MUX3                      MUX3 //CLKO_IOA9

/*CSI CLKO PIN MUX*/
#define CSI_CLKO_POS_MUX0                       MUX0 //CLKO_IOD7
#define CSI_CLKO_POS_MUX1                       MUX1 //CLKO_IOD12
#define CSI_CLKO_POS_MUX2                       MUX2 //CLKO_IOC9
#define CSI_CLKO_POS_MUX3                       MUX3 //CLKO_IOA9

/*	PDM PIN	MUX	*/
#define PDM_MUX0					            MUX0//PDM_CLK_RX__IOB0_IOB1
#define PDM_MUX1					            MUX1//PDM_CLK_RX__IOB8_IOB9
#define PDM_MUX2					            MUX2//PDM_CLK_RX__IOC4_IOC5
#define PDM_MUX3					            MUX3//PDM_CLK_RX__IOD6_IOD7
#define PDM_MUX4					            MUX4//PDM_CLK_RX__IOF0_IOF1

/* CAN PINMUX */
#define CAN_MUX0							    MUX0//CAN_TX_RX__IOC2_IOC3
#define CAN_MUX1							    MUX1//CAN_TX_RX__IOC0_IOC1

/* SMCARD RST Pin PINMUX */
#define SM_RST_MUX0							    MUX0//SMCARD_RST__IOB5
#define SM_RST_MUX1							    MUX1//SMCARD_RST__IOB6

/* ETHERNET MAC PINMUX */
#define ETH_MAC_MUX0							MUX0//ETH_MAC__IOF0_IOF7_IOC12_IOC13
#define ETH_MAC_MUX1							MUX1//ETH_MAC__IOA4_IOA9_IOC0_IOC3

#endif //__PINMUX_CONFIG__

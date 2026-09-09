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
#ifndef __drv_l1_TRNG_H__
#define __drv_l1_TRNG_H__


/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/
#define RING_LEN_9_CELLS  0
#define RING_LEN_13_CELLS 1
#define RING_LEN_15_CELLS 2
#define RING_LEN_21_CELLS 3

#define LEN_SEL0 8
#define LEN_SEL1 10
#define LEN_SEL2 12
#define LEN_SEL3 14

#define RING_4_MODE (1 << 31)
#define PULSE_EN    (1 << 5)

#define POST_VonNeumann (0 << 4)
#define POST_XOR    (1 << 4)

#define DIS_POST    (1 << 1)
#define EN_POST     (0 << 1)

//#define INT_EN      (1 << 3)
#define DMA_EN      (1 << 2)

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void drv_l1_trng_init(void);
extern INT32S drv_l1_trng_get(INT8U *buffer, INT32U len);
extern INT32S drv_l1_trng_get_dma(INT8U *buffer, INT32U len);
extern void drv_l1_trng_uninit(void);

#endif  //__drv_l1_TRNG_H__

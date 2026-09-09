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
#ifndef __USBD_UAC_H__
#define __USBD_UAC_H__

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/* Definition of UAC speaker & microphone sample rates */
#define USBD_UAC_SPEAKER_SAMPLE_RATE		48000
#define USBD_UAC_MICROPHONE_SAMPLE_RATE		16000
#define USBD_UAC_SPK_SUBFRAME_BYTE			0x02
#define USBD_UAC_SPK_SUBFRAME_VALID_BIT		0x10

#if UAC_HI_SPEED == 1
#define EPC_MAX_PKT_SIZE					1024
#else
#define EPC_MAX_PKT_SIZE					200
#endif



/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
#endif

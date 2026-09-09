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
#ifndef DRV_L2_USBD_UAC_H
#define DRV_L2_USBD_UAC_H

#include "drv_l2_usbd.h"
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/* Definition of UAC speaker & microphone sample rates */
#define ISO_OUT_MONO	0
#define ISO_OUT_STEREO	1
#define ISO_OUT_CH		ISO_OUT_STEREO//ISO_OUT_MONO

#define USBD_UAC_SPEAKER_SAMPLE_RATE		48000
#define USBD_UAC_MICROPHONE_SAMPLE_RATE		16000
#define USBD_UAC_SPK_SUBFRAME_BYTE			0x02
#define USBD_UAC_SPK_SUBFRAME_VALID_BIT		0x10

/*
 * @brief: USBD UAC initial
 * @param: usb_id: 0 for USB_A; 1 for USB_B
 * @return: 0: SUCCESS
*/
extern INT32S drv_l2_usbd_ctl_uac_init(INT32U usb_id);

/*
 * @brief: USBD UAC uninitial
 * @param: usb_id: 0 for USB_A; 1 for USB_B
 * @return: 0: SUCCESS
*/
extern INT32S drv_l2_usbd_uac_uninit(INT32U usb_id);

/*
 * @brief: USBD UAC callback function to set gain
 * @param: usb_id: 0 for USB_A; 1 for USB_B
 * @param: fun: Set gain callback function
 * @return: 0: SUCCESS
*/
extern void drv_l2_usbd_register_uac_gain_cbk(USBD_L2_UAC_SET_GAIN_CALLBACK_FUN fun);

/*
 * @brief: USBD UAC HID set idle call back function
 * @param: fun: Set idle callback function
 * @return: None
*/
extern void drv_l2_usbd_register_hid_set_idle_id_cbk(USBD_L2_HID_SET_IDLE_CALLBACK_FUN fun);

#endif

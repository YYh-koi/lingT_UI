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
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park,                      *
 *  Hsinchu City 30077, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/

#ifndef __drv_l2_USBH_UVC_H__
#define __drv_l2_USBH_UVC_H__

/**************************************************************************
*                           C O N S T A N T S                             *
**************************************************************************/
/* Video Class-Specific VideoControl Interface Descriptor Subtypes, (USB_Video_Class_1.1.pdf, A.5 Video Class-Specific VC Interface Descriptor Subtypes) */
#define VC_DESCRIPTOR_UNDEFINED                    0x00
#define VC_HEADER                                  0x01
#define VC_INPUT_TERMINAL                          0x02
#define VC_OUTPUT_TERMINAL                         0x03
#define VC_SELECTOR_UNIT                           0x04
#define VC_PROCESSING_UNIT                         0x05
#define VC_EXTENSION_UNIT                          0x06

/* Input Terminal Types, (USB_Video_Class_1.1.pdf, B.2 Input Terminal Types) */
#define ITT_VENDOR_SPECIFIC                      0x0200
#define ITT_CAMERA                               0x0201
#define ITT_MEDIA_TRANSPORT_INPUT                0x0202

#define HC_UVC_ITD_FRAME_NUM_DELAY		0
#define TD_NO_INTERRUPT_DELAY			0x07

typedef struct
{
	INT32U	abuf_addr;
	INT32U	bbuf_addr;
	INT32U	abuf_size;
	INT32U	bbuf_size;
} UVC_FRAME_BUF_INFO;


/**
* @brief 		Start EHCI isochronous in data transferring.
* @param 		hostid: host id.
* @return		TRUE: success, FALSE: fail.
*/
extern INT32U drv_l2_usbh_ehci_iso_in_trans(INT32U hostid);
/**
* @brief 		Start OHCI isochronous in data transferring.
* @param 		hostid: host id.
* @return		TRUE: success, FALSE: fail.
*/
extern INT32U drv_l2_usbh_ohci_iso_in_trans(INT32U hostid);
extern void drv_l2_usbh_return_itd(INT32U hostid);
extern void drv_l2_usbh_uvc_reset_itd(INT32U hostid);
extern INT8U drv_l2_usbh_ehci_chk_reverse_parking(INT32U hostid);
extern void drv_l2_usbh_ehci_release_reverse_parking(INT32U hostid);
extern INT8U drv_l2_usbh_ohci_chk_reverse_parking(INT32U hostid);
extern void drv_l2_usbh_ohci_release_reverse_parking(INT32U hostid);

#endif

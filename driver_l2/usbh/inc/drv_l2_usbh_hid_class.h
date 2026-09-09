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

#ifndef __drv_l2_USBH_HID_CLASS_H__
#define __drv_l2_USBH_HID_CLASS_H__


#define HID_CLASS           0x03
#define BOOT_IF             0x01
#define KEYBOARD_PROTO      0x01
#define MOUSE_PROTO         0x02
#define REPORT_DESCRIPTOR   0x22

#define REPORT_TYPE_IN          1
#define REPORT_TYPE_OUT         2
#define REPORT_TYPE_FEATURE     3

#define NumLockbit          0x01
#define CapLockbit          0x02
#define ScrlLockbit         0x04

#define TD_MAX_XFER_LENGTH  0x2000

/**************************************************************************
 *                         M A C R O S                                    *
**************************************************************************/
/* The D0/D1 toggle bits */
#define usb_gettoggle(dev, ep, out) (((dev)->toggle[out] >> ep) & 1)
#define usb_dotoggle(dev, ep, out)  ((dev)->toggle[out] ^= (1 << ep))
#define usb_settoggle(dev, ep, out, bit) ((dev)->toggle[out] = ((dev)->toggle[out] & ~(1 << ep)) | ((bit) << ep))
/**************************************************************************
 *                         H E A D E R   F I L E S                        *
**************************************************************************/
#include "drv_l2_usbh.h"


extern USBH_CLASS_OPERATION	g_usbh_hid_operation;

/**************************************************************************
*                          D A T A    T Y P E S                           *
**************************************************************************/

typedef struct
{
    INT8U                   hid_desc_size;
	INT8U 					int_in_ep;
	INT8U					int_in_toggle;
	INT16U					int_in_maxpktsz:11;
	INT16U					int_in_multi:2;
	INT16U					int_in_reserved:3;
	INT8U					int_in_interval;
	INT8U*                  int_in_buffer;
}USBH_HID_INFO;


extern void drv_l2_usbh_int_in_send(INT32U hostid);
extern void drv_l2_usbh_int_in_done_callback(INT32U hostid);
extern void drv_l2_usbh_ohci_intin_register_buffer(INT8U* buf);
extern void drv_l2_usbh_hid_class_protocol_reg(INT8U protocol_type);

#endif/* __drv_l2_USBH_HID_CLASS_H__ */

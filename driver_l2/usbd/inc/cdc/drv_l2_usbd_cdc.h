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
#ifndef DRV_L2_USBD_CDC_H
#define DRV_L2_USBD_CDC_H

#include "drv_l1_usbd.h"
#include "drv_l2_usbd.h"
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define CDCACM_MSDC_SUPPORT 		0

/* USB Sub class and protocol codes */
#define USB_CDC_ACM_SUBCLASS		0x02
#define USB_CDC_AT_COMMAND_PROTOCOL	0x01

/* CDC interface descriptor type */
#define USB_DESCTYPE_CS_INTERFACE	0x24

#if CDCACM_MSDC_SUPPORT == 1
#define USB_MSDC_INTF_NUM			0
#define USB_CDC_INTF_NUM			1
#define USB_CDC_DATA_INTF_NUM		2
#define USB_MSDC_EP_PLD_SZ_OFFSET   22
#define USB_CDC_EP_PLD_SZ_OFFSET    88
#else
#define USB_CDC_INTF_NUM			0
#define USB_CDC_DATA_INTF_NUM		1
#define USB_MSDC_INTF_NUM           0xFF
#define USB_CDC_EP_PLD_SZ_OFFSET    57
#endif

/* Endpoint Addresses for CDC device */
#define CDC_NOTIFICATION_EP_NUM		3
#if CDC_EP == BULK_EP89
#define CDC_DATA_RX_EP				9
#define CDC_DATA_TX_EP				8
#elif CDC_EP == BULK_EP12
#define CDC_DATA_RX_EP				2
#define CDC_DATA_TX_EP				1
#endif

#if MSDC_EP == BULK_EP12
#define MSDC_BULK_IN_EP				1
#define MSDC_BULK_OUT_EP			2
#elif MSDC_EP == BULK_EP89
#define MSDC_BULK_IN_EP				8
#define MSDC_BULK_OUT_EP			9
#endif


/* Data endpoint packet size */
#define MAX_EP_SIZE 512
/* CDC Communications Class requests */
#define CDC_SET_LINE_CODING 		0x20
#define CDC_GET_LINE_CODING 		0x21
#define CDC_SET_CONTROL_LINE_STATE	0x22
#define CDC_SEND_BREAK				0x23


/* USB Descriptor Types */
#define USB_DEVICE_DESCRIPTOR_TYPE                 1
#define USB_CONFIGURATION_DESCRIPTOR_TYPE          2
#define USB_STRING_DESCRIPTOR_TYPE                 3
#define USB_INTERFACE_DESCRIPTOR_TYPE              4
#define USB_ENDPOINT_DESCRIPTOR_TYPE               5
#define USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE       6
#define USB_OTHER_SPEED_CONFIG_DESCRIPTOR_TYPE     7
#define USB_INTERFACE_POWER_DESCRIPTOR_TYPE        8
#define USB_OTG_DESCRIPTOR_TYPE                    9
#define USB_DEBUG_DESCRIPTOR_TYPE                 10
#define USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE 11

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
 #define WBVAL(x) ((x) & 0xFF),(((x) >> 8) & 0xFF)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct CDC_CTLBLK_S{
	INT8U	*rxBufPtr;
	INT8U	*txBufPtr;
	INT8U	*dataPtr;
	INT32U	dataCnt;
	INT32U  resData;

}CDC_CTLBLK_T;

typedef void (*CDC_TASK_INIT)(INT32U usb_id);

typedef struct LINECODING_S {
	INT32U baudRate;
	INT8U charFormat;	/*0->1 Stop bit; 1->1.5 Stop bit; 2: 2Stop bit*/
	INT8U parityType;	/*0->None; 1->Odd; 2-> Even; 3-> Mark; 4-> Space*/
	INT8U dataBits;		/*Data bits(5,6,7,8 or 16)*/
}LINECODING_t;

typedef struct LINSTATE_S {
	INT8U dtr;
	INT8U rts;
}LINSTATE_t;
/**************************************************************************
 *                           F U N C T I O N S                            *
 **************************************************************************/
 /*
 * @brief: USBD CDC initial
 * @param: usb_id: 0 for USB_A; 1 for USB_B
 * @return: None
*/
extern INT32S drv_l2_usbd_cdc_init(INT32U usb_id);

/*
 * @brief: USBD CDC function when EP0 device get out token
 * @param: usb_id: 0 for USB_A; 1 for USB_B
 * @param: event: EP0 event for current  out token
 * @return: None
*/
extern void drv_l2_usbd_state_ctl_cdc_ep0_out(INT32U usb_id, INT32U event);

/*
 * @brief: USBD CDC function when EP0 device get setup token.
 * @param: usb_id: 0 for USB_A; 1 for USB_B
 * @param: event: EP0 event for current setup token
 * @return: None
*/
extern void drv_l2_usbd_state_ctl_cdc_ep0_setup(INT32U usb_id, INT32U event);

/*
 * @brief: USBD CDC function to register cdc task init function.
 * @param: fn: cdc task init function
 * @return: None
*/
extern void drv_l2_usbd_cdc_register_task_init(CDC_TASK_INIT fn);
#endif

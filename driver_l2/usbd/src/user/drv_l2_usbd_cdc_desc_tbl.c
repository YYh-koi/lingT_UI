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
#include "project.h"
#include "drv_l2_usbd.h"
#include "drv_l2_usbd_cdc.h"



/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define CDC_DESC_LEN			WBVAL(9+9+5+5+4+5+7+9+7+7)
#define CDC_W_MSDC_DESC_LEN		WBVAL(9+9+7+7+8+9+5+5+4+5+7+9+7+7)

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
/* Device descriptor */
#if CDCACM_MSDC_SUPPORT== 1
ALIGN4 INT8U cdc_device_desc_tbl[] =
{

	0x12,			//bLength: 0x12 byte
	0x01,			//bDescriptorType	: Device
	0x00, 0x02,		//bcdUSB			: version 2.00
	0xEF,			//Miscellaneous
	0x02,			//bDeviceSubclass = 0x02, bDeviceProtocol = 0x01: IAD
	0x01,			//


	0x40,			//bMaxPacketSize0	:
	0x3F, 0x1B,		//idVendor			: Vendor ID
	0x79, 0x06,		//idProduct			: Product ID
	0x00, 0x01,		//bcdDevice			: 1.00
	0x01,			//iManufacturer		: 1
	0x02,			//iProduct
	0x00,			//iSerialNumber
	0x01			//bNumConfigurations
};

/* Configuration Descriptor */
ALIGN4 INT8U cdc_config_desc_tbl[] =
{
	0x09, 										/* 0 bLength */
	0x02,										/* 1 bDescriptortype - Configuration*/
	CDC_W_MSDC_DESC_LEN,
	0x03,										/* 4 bNumInterfaces */

	0x01,										/* 5 bConfigurationValue */
	0x04,										/* 6 iConfiguration - index of string */
	0x80,										/* 7 bmAttributes - Bus powered */
	0xFA, 										/* 8 bMaxPower - 500mA */



	/*MSDC interface descriptor*/
	0x09,					//bLength: 0x09 byte
	0x04,					//bDescriptorType: INTERFACE
	USB_MSDC_INTF_NUM,		//bInterfaceNumber: interface 2
	0x00,					//bAlternateSetting: alternate setting 0
	0x02,					//bNumEndpoints: 2 endpoints
	0x08,					//bInterfaceClass : mass storage class
	0x06,					//bInterfaceSubClass: SCSI transparent commend set
	0x50,					//Bulk Only
	0x00,					//iInterface: index of string

	//Endpoint 1 (0x07 byte)
	0x07,					//bLength: 0x07 byte
	0x05,					//bDescriptorType: ENDPOINT
	(MSDC_BULK_IN_EP| 0x80),//bEndpointAddress: IN endpoint 8 --
	0x02,					//bmAttributes: Bulk
	0x40, 0x00,				//wMaxPacketSize: 64 byte
	0x00,					//bInterval: ignored

	//Endpoint 2 (0x07 byte)
	0x07,					//bLength: 0x07 byte
	0x05,					//bDescriptorType: ENDPOINT
	MSDC_BULK_OUT_EP,       //bEndpointAddress: OUT endpoint 9 --
	0x02,					//bmAttributes: Bulk
	0x40, 0x00,				//wMaxPacketSize: 64 byte
	0x00,					//bInterval: ignored

	/*IAD descriptor*/
	0x08,					//bLength: 0x08 byte
	0x0B,					//bDescriptorType: 0x08
	0x01,					//bFirstInterface: 0x01
	0x02,					//bInterfaceCount: 0x02
	0x02,					//bFunctionClass: 0x02: Communications and CDC Control
	0x02,					//bFunctionSubClass: 0x02: Abstract Control Model
	0x01,					//bFunctionProtocol: 0x01: AT Command V.250 protocol
	0x00,					//iFunction

	/* CDC Communication interface */
	0x09, 										/* 0 bLength */
	0x04,					 					/* 1 bDescriptorType - Interface */
	USB_CDC_INTF_NUM, 										/* 2 bInterfaceNumber - Interface 0 */
	0x00, 										/* 3 bAlternateSetting */
	0x01, 										/* 4 bNumEndpoints */
	0x02, 										/* 5 bInterfaceClass - Communication */
	0x02, 						 				/* 6 bInterfaceSubClass - Abstract Control Model */
	0x01, 										/* 7 bInterfaceProtocol - AT Command V.250 protocol */
	0x00, 										/* 8 iInterface - No string descriptor */

	/* Header Functional descriptor */
	0x05, 										/* 0 bLength */
	USB_DESCTYPE_CS_INTERFACE, 					/* 1 bDescriptortype, CS_INTERFACE */
	0x00, 										/* 2 bDescriptorsubtype, HEADER */
	0x10, 0x01, 								/* 3 bcdCDC */

	/* ACM Functional descriptor */
	0x04, 										/* 0 bLength */
	USB_DESCTYPE_CS_INTERFACE, 					/* 1 bDescriptortype, CS_INTERFACE */
	0x02, 										/* 2 bDescriptorsubtype, ABSTRACT CONTROL MANAGEMENT */
	0x02, 										/* 3 bmCapabilities: Supports subset of ACM commands */

	/* Union Functional descriptor */
	0x05, 										/* 0 bLength */
	USB_DESCTYPE_CS_INTERFACE,					/* 1 bDescriptortype, CS_INTERFACE */
	0x06, 										/* 2 bDescriptorsubtype, UNION */
	USB_CDC_INTF_NUM, 							/* 3 bControlInterface - Interface 0 */
	USB_CDC_DATA_INTF_NUM, 						/* 4 bSubordinateInterface0 - Interface 1 */

	/* Call Management Functional descriptor */
	0x05, 										/* 0 bLength */
	USB_DESCTYPE_CS_INTERFACE,					/* 1 bDescriptortype, CS_INTERFACE */
	0x01, 										/* 2 bDescriptorsubtype, CALL MANAGEMENT */
	//0x03, 										/* 3 bmCapabilities, DIY */
	0x00, 										/* 3 bmCapabilities, */
	USB_CDC_DATA_INTF_NUM, 						/* 4 bDataInterface */

	/* Notification Endpoint descriptor */
	0x07, 										/* 0 bLength */
	0x05, 										/* 1 bDescriptorType */
	(CDC_NOTIFICATION_EP_NUM | 0x80),			/* 2 bEndpointAddress */
	0x03, 										/* 3 bmAttributes - 3: Interrupt */
	0x40, 										/* 4 wMaxPacketSize - Low */
	0x00, 										/* 5 wMaxPacketSize - High */
	0xFF, 										/* 6 bInterval */

	/* CDC Data interface */
	0x09, 										/* 0 bLength */
	0x04,					 					/* 1 bDescriptorType */
	USB_CDC_DATA_INTF_NUM, 						/* 2 bInterfacecNumber */
	0x00, 										/* 3 bAlternateSetting */
	0x02, 										/* 4 bNumEndpoints */
	0x0A, 										/* 5 bInterfaceClass: CDC-Data */
	0x00, 										/* 6 bInterfaceSubClass */
	0x00, 										/* 7 bInterfaceProtocol*/
	0x00, 										/* 8 iInterface - No string descriptor*/

	/* Data OUT Endpoint descriptor */
	0x07, 										/* 0 bLength */
	0x05, 										/* 1 bDescriptorType */
	CDC_DATA_RX_EP,		 						/* 2 bEndpointAddress */
	0x02, 										/* 3 bmAttributes - 2: Bulk */
	0x00, 										/* 4 wMaxPacketSize - Low */
	0x02, 										/* 5 wMaxPacketSize - High */
	0x00, 										/* 6 bInterval */

	/* Data IN Endpoint descriptor */
	0x07, 										/* 0 bLength */
	0x05, 										/* 1 bDescriptorType */
	(CDC_DATA_TX_EP | 0x80),					/* 2 bEndpointAddress */
	0x02, 										/* 3 bmAttributes - 2: Bulk*/
	0x00, 										/* 4 wMaxPacketSize - Low byte */
	0x02, 										/* 5 wMaxPacketSize - High byte */
	0x01, 										/* 6 bInterval */
};
#else
ALIGN4 INT8U cdc_device_desc_tbl[] =
{

	0x12,			//bLength: 0x12 byte
	0x01,			//bDescriptorType	: Device
	0x00, 0x02,		//bcdUSB			: version 2.00
	0x02,			//bDeviceClass		: Communication Device Class
	0x00,			//bDeviceSubClass	:
	0x00,			//bDeviceProtocol	:

	0x40,			//bMaxPacketSize0	:
	0x3F, 0x1B,		//idVendor			: Vendor ID
	0x78, 0x06,		//idProduct			: Product ID
	0x00, 0x01,		//bcdDevice			: 1.00
	0x01,			//iManufacturer		: 1
	0x02,			//iProduct
	0x00,			//iSerialNumber
	0x01			//bNumConfigurations
};

/* Configuration Descriptor */
ALIGN4 INT8U cdc_config_desc_tbl[] =
{
	0x09, 										/* 0 bLength */
	0x02,										/* 1 bDescriptortype - Configuration*/
	CDC_DESC_LEN,
	0x02,										/* 4 bNumInterfaces */

	0x01,										/* 5 bConfigurationValue */
	0x04,										/* 6 iConfiguration - index of string */
	0x80,										/* 7 bmAttributes - Bus powered */
	0xC8, 										/* 8 bMaxPower - 400mA */


	/* CDC Communication interface */
	0x09, 										/* 0 bLength */
	0x04,					 					/* 1 bDescriptorType - Interface */
	USB_CDC_INTF_NUM, 										/* 2 bInterfaceNumber - Interface 0 */
	0x00, 										/* 3 bAlternateSetting */
	0x01, 										/* 4 bNumEndpoints */
	0x02, 										/* 5 bInterfaceClass - Communication */
	0x02, 						 				/* 6 bInterfaceSubClass - Abstract Control Model */
	0x01, 										/* 7 bInterfaceProtocol - AT Command V.250 protocol */
	0x00, 										/* 8 iInterface - No string descriptor */

	/* Header Functional descriptor */
	0x05, 										/* 0 bLength */
	USB_DESCTYPE_CS_INTERFACE, 					/* 1 bDescriptortype, CS_INTERFACE */
	0x00, 										/* 2 bDescriptorsubtype, HEADER */
	0x10, 0x01, 								/* 3 bcdCDC */

	/* ACM Functional descriptor */
	0x04, 										/* 0 bLength */
	USB_DESCTYPE_CS_INTERFACE, 					/* 1 bDescriptortype, CS_INTERFACE */
	0x02, 										/* 2 bDescriptorsubtype, ABSTRACT CONTROL MANAGEMENT */
	0x02, 										/* 3 bmCapabilities: Supports subset of ACM commands */

	/* Union Functional descriptor */
	0x05, 										/* 0 bLength */
	USB_DESCTYPE_CS_INTERFACE,					/* 1 bDescriptortype, CS_INTERFACE */
	0x06, 										/* 2 bDescriptorsubtype, UNION */
	USB_CDC_INTF_NUM, 							/* 3 bControlInterface - Interface 0 */
	USB_CDC_DATA_INTF_NUM, 						/* 4 bSubordinateInterface0 - Interface 1 */

	/* Call Management Functional descriptor */
	0x05, 										/* 0 bLength */
	USB_DESCTYPE_CS_INTERFACE,					/* 1 bDescriptortype, CS_INTERFACE */
	0x01, 										/* 2 bDescriptorsubtype, CALL MANAGEMENT */
	0x00, 										/* 3 bmCapabilities, */
	USB_CDC_DATA_INTF_NUM, 						/* 4 bDataInterface */

	/* Notification Endpoint descriptor */
	0x07, 										/* 0 bLength */
	0x05, 										/* 1 bDescriptorType */
	(CDC_NOTIFICATION_EP_NUM | 0x80),			/* 2 bEndpointAddress */
	0x03, 										/* 3 bmAttributes - 3: Interrupt */
	0x40, 										/* 4 wMaxPacketSize - Low */
	0x00, 										/* 5 wMaxPacketSize - High */
	0xFF, 										/* 6 bInterval */

	/* CDC Data interface */
	0x09, 										/* 0 bLength */
	0x04,					 					/* 1 bDescriptorType */
	USB_CDC_DATA_INTF_NUM, 						/* 2 bInterfacecNumber */
	0x00, 										/* 3 bAlternateSetting */
	0x02, 										/* 4 bNumEndpoints */
	0x0A, 										/* 5 bInterfaceClass: CDC-Data */
	0x00, 										/* 6 bInterfaceSubClass */
	0x00, 										/* 7 bInterfaceProtocol*/
	0x00, 										/* 8 iInterface - No string descriptor*/

	/* Data OUT Endpoint descriptor */
	0x07, 										/* 0 bLength */
	0x05, 										/* 1 bDescriptorType */
	CDC_DATA_RX_EP,		 						/* 2 bEndpointAddress */
	0x02, 										/* 3 bmAttributes - 2: Bulk */
	0x00, 										/* 4 wMaxPacketSize - Low */
	0x02, 										/* 5 wMaxPacketSize - High */
	0x00, 										/* 6 bInterval */

	/* Data IN Endpoint descriptor */
	0x07, 										/* 0 bLength */
	0x05, 										/* 1 bDescriptorType */
	(CDC_DATA_TX_EP | 0x80),					/* 2 bEndpointAddress */
	0x02, 										/* 3 bmAttributes - 2: Bulk*/
	0x00, 										/* 4 wMaxPacketSize - Low byte */
	0x02, 										/* 5 wMaxPacketSize - High byte */
	0x01, 										/* 6 bInterval */
};
#endif
/* Qualifier Descriptor */
ALIGN4 INT8U cdc_qualifier_desc_tbl[]=
{
	0x0A,                   //bLength: 0x0A byte
	0x06,                   //bDescriptorType: DEVICE_QUALIFIER
	0x00, 0x02,             //bcdUSB: version 200 // 0x00,0x02
	0x00,                   //bDeviceClass:
	0x00,                   //bDeviceSubClass:
	0x00,                   //bDeviceProtocol:
	0x40,                   //bMaxPacketSize0: maximum packet size for endpoint zero
	0x01,                   //bNumConfigurations: 1 configuration
	0x00					//bReserved
};

ALIGN4 INT8U cdc_string0_desc[] =
{
    /* Index 0x00: LANGID Codes */
    0x04,                                   // bLength                  4
    USB_STRING_DESCRIPTOR_TYPE,             // bDescriptorType          3 (STRING)
    0x09,                      	 // wLANGID             0x0409 US English
    0x04,
};

ALIGN4 INT8U cdc_string1_desc[] =
{
    /* Index 0x01: Manufacturer */
     0x10,                                   // bLength                 28
    USB_STRING_DESCRIPTOR_TYPE,             // bDescriptorType          3 (STRING)
    'G',0,
    'E',0,
    'N',0,
    'E',0,
    'R',0,
    'A',0,
    'L',0
};

ALIGN4 INT8U cdc_string2_desc[] =
{
  /* Index 0x02: Product */
	0x1C,                                   // bLength                  28
	USB_STRING_DESCRIPTOR_TYPE,             // bDescriptorType          3 (STRING)
    'G',0,
    'E',0,
    'N',0,
    'E',0,
    'R',0,
    'A',0,
    'L',0,
    ' ',0,
    '-',0,
    ' ',0,
    'C',0,
    'D',0,
    'C',0
};

ALIGN4 INT8U cdc_string3_desc[] =
{
    /* Index 0x03: Serial Number */
    0x14,                                   // bLength                  20
    USB_STRING_DESCRIPTOR_TYPE,             // bDescriptorType          3 (STRING)
    'D',0,
    'e',0,
    'm',0,
    'o',0,
    ' ',0,
    '1',0,
    '.',0,
    '0',0,
    '0',0
};

ALIGN4 INT8U cdc_string4_desc[] =
 {
    /* Index 0x04:  */
    0x1C,                                   // bLength
    USB_STRING_DESCRIPTOR_TYPE,             // bDescriptorType          3 (STRING)
    'G',0,
    'E',0,
    'N',0,
    'E',0,
    'R',0,
    'A',0,
    'L',0,
    ' ',0,
    '-',0,
    ' ',0,
    'C',0,
    'D',0,
    'C',0
};

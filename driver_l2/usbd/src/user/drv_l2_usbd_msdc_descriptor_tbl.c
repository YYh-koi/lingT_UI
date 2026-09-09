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
ALIGN4 INT8U Default_Device_Descriptor_TBL[] =
{
     0x12,                      //bLength: 0x12 byte
 	 0x01,				        //bDescriptorType	: Device
	 0x00, 0x02,				//bcdUSB			: version 2.00
	 0x00, 						//bDeviceClass
	 0x00, 						//bDeviceSubClass
	 0x00,						//bDeviceProtocol
     0x40,                      //bMaxPacketSize0
	 0x3F, 0x1B,				//idVendor
	 0x07,0x80,//0x01, 0x83,				//idProduct
	 0x00, 0x01,				//bcdDevice
	 0x01,						//iManufacturer, string index 1
	 0x02,						//iProduct, string index 2
	 0x00,//0x03,						//iSerialNumber, string index 3, no string now
	 0x01,						//bNumConfigurations
};

const INT8U Default_Qualifier_Descriptor_TBL[] =
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

ALIGN4 INT8U Default_Config_Descriptor_TBL[] =
{
    0x09,                   //bLength: 0x09 byte
    0x02,                   //bDescriptorType: CONFIGURATION
    0x20,                   //wTotalLength:
    0x00,
    0x01,                   //bNumInterfaces: 1 interfaces
    0x01,                   //bConfigurationValue: configuration 1
    0x00,                   //iConfiguration: index of string
    0xC0,                   //bmAttributes: bus powered, Not Support Remote-Wakeup
    0x32,                   //MaxPower: 100 mA
/* Interface_Descriptor */
    0x09,                   //bLength: 0x09 byte
    0x04,                   //bDescriptorType: INTERFACE
    0x00,                   //bInterfaceNumber: interface 0
    0x00,                   //bAlternateSetting: alternate setting 0
    0x02,                   //bNumEndpoints: 2 endpoints(EP1,EP2)
    0x08,                   //bInterfaceClass: Mass Storage Devices Class
    0x06,                   //bInterfaceSubClass:
    0x50,                   //bInterfaceProtocol
    0x00,                   //iInterface: index of string
/* Endpoint1 */
    0x07,                   //bLength: 0x07 byte
    0x05,                   //bDescriptorType: ENDPOINT
#if (MSDC_EP == BULK_EP12)
    0x81,                   //bEndpointAddress: IN endpoint 1 --
#elif (MSDC_EP == BULK_EP89)
	0x88,                   //bEndpointAddress: IN endpoint 8 --
#endif
    0x02,                   //bmAttributes: Bulk
    0x40, 0x00,             //wMaxPacketSize: 64 byte
    0x00,                   //bInterval: ignored
/* Endpoint2 */
    0x07,                   //bLength: 0x07 byte
    0x05,                   //bDescriptorType: ENDPOINT
#if (MSDC_EP == BULK_EP12)
    0x02,                   //bEndpointAddress: OUT endpoint 2 --
#elif (MSDC_EP == BULK_EP89)
	0x09,                   //bEndpointAddress: OUT endpoint 9 --
#endif
    0x02,                   //bmAttributes: Bulk
    0x40, 0x00,             //wMaxPacketSize: 64 byte
    0x00,                   //bInterval: ignored
};

const INT8U Default_String0_Descriptor[] =
{
	 0x04,		//bLength
	 0x03,		//bDescriptorType
	 0x09, 0x04,//bString
};

const INT8U Default_String1_Descriptor[] =
{
     0x40,      //bLength
	 0x03,		//bDescriptorType
	 'G', 0x00,	//bString
	 'e', 0x00,
	 'n', 0x00,
	 'e', 0x00,
 	 'r', 0x00,
 	 'i', 0x00,
 	 'c', 0x00,
  	 ' ', 0x00,
   	 'U', 0x00,
  	 'S', 0x00,
  	 'B', 0x00,
  	 ' ', 0x00,
  	 'M', 0x00,
  	 'a', 0x00,
  	 's', 0x00,
  	 's', 0x00,
  	 ' ', 0x00,
  	 'S', 0x00,
  	 't', 0x00,
  	 'o', 0x00,
  	 'r', 0x00,
  	 'a', 0x00,
  	 'g', 0x00,
   	 'e', 0x00,
   	 ' ', 0x00,
  	 'D', 0x00,
  	 'e', 0x00,
  	 'v', 0x00,
  	 'i', 0x00,
  	 'c', 0x00,
  	 'e', 0x00,
};

const INT8U Default_String2_Descriptor[]={
     0x22,//bLength
	 0x03,		//bDescriptorType
	 'G', 0x00,	//bString
	 'E', 0x00,
	 'N', 0x00,
	 'E', 0x00,
	 'R', 0x00,
	 'A', 0x00,
	 'L', 0x00,
	 'P', 0x00,
	 'L', 0x00,
	 'U', 0x00,
	 'S', 0x00,
	 '-', 0x00,
	 'M', 0x00,
	 'S', 0x00,
	 'D', 0x00,
	 'C', 0x00,
};

const INT8U Default_String3_Descriptor[]={
     0x1C,      //bLength
	 0x03,		//bDescriptorType
	 '2', 0x00,	//bString
	 '0', 0x00,
	 '1', 0x00,
	 '6', 0x00,
	 '0', 0x00,
	 '0', 0x00,
	 '0', 0x00,
	 '0', 0x00,
	 '0', 0x00,
	 '0', 0x00,
	 '0', 0x00,
	 '0', 0x00,
	 '1', 0x00,

};

const INT8U Default_scsi_inquirydata[] =
{
	0x00,
	0x80,
	0x00,
	0x00,
	0x1F,
	0x00,
	0x00,
	0x00,
	'G','E','N','P','L','U','S',
	0x20,
	'U','S','B','-','M','S','D','C',' ','D','I','S','K',
	' ',
	'C',
	0x20,
	'1','.','0','0','G','P','-','P','R','O','D','.',
};

const INT8U Default_scsi_inquirydata2[] =
{
	0x00,
	0x80,
	0x00,
	0x00,
	0x1F,
	0x00,
	0x00,
	0x00,
	'G','E','N','P','L','U','S',
	0x20,
	'U','S','B','-','M','S','D','C',' ','D','I','S','K',
	' ',
	'B',
	0x20,
	'1','.','0','0','G','P','-','P','R','O','D','.',
};
const INT8U Default_scsi_inquirydata_CDROM[] =
{
	0x05,
	0x02,
	0x01,
	0x00,
	0x1F,
	0x00,
	0x00,
	0x00,
	'G','E','N','P','L','U','S',
	0x20,
	'U','S','B','-','C','D','R','M',' ','D','I','S','K',
	' ',
	'A',
	0x20,
	'1','.','0','0','G','P','-','P','R','O','D','.',
};
const INT8U Default_TOC_PMA_ATIP_Format[] =
{
 //    0     1     2     3
    0x00, 0x0A, 0x01, 0x01,

//    4     5     6     7    8     9    10    11
	0x01, 0x14, 0x01, 0xA0, 0x00, 0x00, 0x02, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x03, 0x24, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x35, 0x00, 0x35, 0x12, 0x00,
};

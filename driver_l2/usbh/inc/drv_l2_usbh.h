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
#ifndef __drv_l2_USBH_H__
#define __drv_l2_USBH_H__

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
**************************************************************************/
#include "project.h"

/**************************************************************************
*                           C O N S T A N T S                             *
**************************************************************************/
#define ENDPOINT_DIR_MASK            0x80
#define ENDPOINT_DIR_OUT             0x00
#define ENDPOINT_DIR_IN              0x80
#define EP_TYPE_MASK                 0x03
#define EP_TYPE_CONTROL              0x00
#define EP_TYPE_ISOCHRONOUS          0x01
#define EP_TYPE_BULK                 0x02
#define EP_TYPE_INTERRUPT            0x03

//*-------------------------------------------------------------------------*//
#define BMREQDIR_HOST2DEVICE         (0 << 7)
#define BMREQDIR_DEVICE2HOST         (1 << 7)
#define BMREQTYPE_STANDARD           (0 << 5)
#define BMREQTYPE_CLASS              (1 << 5)
#define BMREQTYPE_VENDOR             (2 << 5)
#define BMREQREC_DEVICE              (0 << 0)
#define BMREQREC_INTERFACE           (1 << 0)
#define BMREQREC_ENDPOINT            (2 << 0)
#define BMREQREC_OTHER               (3 << 0)
//*-------------------------------------------------------------------------*//

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/
extern const INT32U g_usbh_id[2];
extern INT8U g_enumRetryNum;
/**************************************************************************
*                          D A T A    T Y P E S                           *
**************************************************************************/

/* USB class code */
typedef enum
{
	UC_DEVICE 	= 0x00,
	UC_AUDIO	= 0x01,
	UC_CDC_CTRL	= 0x02,
	UC_HID		= 0x03,
	UC_PHYSICAL	= 0x05,
	UC_IMAGE	= 0x06,
	UC_PRINTER	= 0x07,
	UC_MSC		= 0x08,
	UC_HUB		= 0x09,
	UC_CDC		= 0x0A,
	UC_CCID		= 0x0B,
	UC_CS		= 0x0D,
	UC_VIDEO	= 0x0E,
	UC_AUDIO_VIDEO = 0x10,
	UC_MAX		= 0x11
}USB_CLASS_CODES;

/* USB descriptor types */
typedef enum
{
	DT_DEVICE			= 0x01,		/* Device descriptor */
	DT_CONFIG			= 0x02,		/* Configuration descriptor */
	DT_STRING			= 0x03,		/* String descriptor */
	DT_IF				= 0x04,		/* Interface descriptor */
	DT_ENDP				= 0x05,		/* Endpoint descriptor */
	DT_DEVICE_QUAL		= 0x06,		/* Device qualifier descriptor */
	DT_OTHER_SPEED		= 0x07,		/* Other speed configuration descriptor*/
	DT_IF_POW			= 0x08,		/* Interface power descriptor */
	DT_IF_ASSOCIATION	= 0x0B,		/* Interface association */
	DT_HID              = 0x21,     /* HID descriptor */

	/* Video Class-Specific Descriptor Types */
	DT_VCS_UNDEFINE		= 0x20,		/* Undefined descriptor */
	DT_VCS_DEVICE		= 0x21,		/* Device descriptor */
	DT_VCS_CONFIG		= 0x22,		/* Configuration descriptor */
	DT_VCS_STRING		= 0x23,		/* String descriptor */
	DT_VCS_IF			= 0x24,		/* Interface descriptor */
	DT_VCS_ENDP			= 0x25,		/* Endpoint descriptor */

	/* Audio Class-Specific Descriptor Types */
	DT_ACS_IF			= 0x24,		/* Interface descriptor */
	DT_ACS_ENDP			= 0x25		/* CS Endpoint descriptor */
}USB_DESC_TYPE;
typedef enum
{
	REQ_GetStatus           = 0,
	REQ_ClearFeature        = 1,
	REQ_SetFeature          = 3,
	REQ_SetAddress          = 5,
	REQ_GetDescriptor       = 6,
	REQ_SetDescriptor       = 7,
	REQ_GetConfiguration    = 8,
	REQ_SetConfiguration    = 9,
	REQ_GetInterface        = 10,
	REQ_SetInterface        = 11,
	REQ_SynchFrame          = 12,
}USB_CONTROL_REQ;

typedef struct
{
	INT8U			ucbmRequestType;
	INT8U			ucbRequest;
	INT16U			uswValue;
	INT16U			uswIndex;
	INT16U			uswLength;
}ST_USBH_SetupCmd;

typedef struct
{
	INT8U			Class;
	INT8U			Sub_class;
	INT8U			Protocol;
	void*			priv;
}USBH_INTF_INFO;

typedef struct USBH_DEV_INFO_T
{
    INT8U	        		type;
	INT8U					addr;
	INT8U                   deviceclass;
	INT8U					ep0_max_packet;
	INT8U					interface_number;
	INT32U                  toggle[2];
	INT8U 					speed;				/* 1 for high speed, 0 for full speed */
	INT16U 					VID;
	INT16U					PID;
	INT16U                  string_langid;
	INT8U                   manufacturer_string_idx;
	INT8U                   product_string_idx;
	INT8U                   serialnum_string_idx;
	struct USBH_DEV_INFO_T*	parent;
	USBH_INTF_INFO* 	    interface;
	void* 	                para[3];
}USBH_DEV_INFO;

typedef struct
{
	INT32S (*init) (USBH_DEV_INFO *dev, INT8U* desc, INT32U ln);
	void (*release)(USBH_INTF_INFO *interface);
}USBH_CLASS_OPERATION;


typedef struct
{
	uint16_t Connected : 1;
	uint16_t Enabled : 1;
	uint16_t Suspended : 1;
	uint16_t OverCurrent : 1;
	uint16_t Reset : 1;
	uint16_t _reserved5_7 : 3;
	uint16_t Power : 1;
	uint16_t LowSpeedAttatched : 1;
	uint16_t HighSpeedAttatched : 1;
	uint16_t TestMode : 1;
	uint16_t IndicatorControl : 1;
	uint16_t _reserved13_15 : 3;
} PACKED HubPortStatus_T;


typedef struct
{
	uint16_t ConnectedChanged : 1;
	uint16_t EnabledChanged : 1;
	uint16_t SuspendedChanged : 1;
	uint16_t OverCurrentChanged : 1;
	uint16_t ResetChanged : 1;
	uint16_t _reserved5_15 : 11;
} PACKED HubPortStatusChange_T;
/**************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S                *
**************************************************************************/

extern void print_string(CHAR *fmt, ...);

void drv_l2_usbh_init(INT32U hostid);
void drv_l2_usbh_uninit(INT32U hostid);

INT32U drv_l2_usbh_detect(INT32U hostid);
void drv_l2_usbh_register_host_operation_table(INT32U hostid, INT8U HostType);

/**
* @brief 		Control SETUP transaction.
* @param        hostid[in]: host port number
* @param 		addr[in]: Device address.
* @param 		max_pkt[in]: Device ep0 max packet size.
* @param 		cmd[in]: Standard request.
* @param 		buf[in]: Data buffer.
* @param 		pErrorCode[in]: Error code.
* @return		TRUE: success, FALSE: fail.
*/
extern INT32U drv_l2_usbh_ctrl_trans(
    INT32U hostid,
	INT8U addr,
	INT8U max_pkt,
	ST_USBH_SetupCmd cmd,
	void* buf,
	INT8U* pErrorCode );

/**
* @brief 		USB host enumeration.
* @return		TRUE: success, FALSE: false.
*/
extern INT32U drv_l2_usbh_enumeration(INT32U hostid);

/**
* @brief 		USB host release all resouce.
* @return		None.
*/
extern void drv_l2_usbh_release(INT32U hostid);

/**
* @brief 		Set max USB device number.
* @param 		devno[in]: Device number.
* @return		None.
*/
extern void drv_l2_usbh_max_devno_set(INT32U hostid,
	INT32U devno);

/**
* @brief 		Get how many device init success.
* @return		Device number.
*/
extern INT32U drv_l2_usbh_devno_get(INT32U hostid);

/**
* @brief 		Get device information.
* @param 		devno[in]: Device number.
* @return		Device information pointer, NULL means no device.
*/
extern USBH_DEV_INFO* drv_l2_usbh_dev_get(
	INT32U devno);

/**
* @brief 		Register class operation function.
* @param 		class[in]: Class number.
* @param 		ops[in]: Operation function.
* @return		None.
*/
extern void drv_l2_usbh_register_class_operation(
	INT8U class,
	USBH_CLASS_OPERATION *ops);

/**
* @brief 		Change the Enumeration retry times.
* @param 		enumRetryNum[in]: Enumeration retry times, default:5.
* @return		The old enumeration retry times.
*/
extern INT8U change_enumRetryNum(INT8U enumRetryNum);

//extern void drv_l2_usbh_reinit(INT32U hostid);
extern INT32U drv_l2_usbh_chk_port_connect_status(INT32U hostid);
extern void drv_l2_usbh_free_uvc_ctrl_units(INT32U hostid);
extern void drv_l2_usbh_set_type(uint8_t HostType);
extern INT8U drv_l2_usbh_hosttype(INT32U hostid);
extern INT32U drv_l2_wait_hci_halt(INT32U hostid);

/**
* @brief 		Check uvc reverse parking status.
* @param 		hostid[in]: host port number
* @return		reverse parking status.
*/
extern INT32U drv_l2_usbh_chk_reverse_parking(INT32U hostid);

extern void drv_l2_usbh_release_reverse_parking(INT32U hostid);

#endif /*__drv_l2_USBH_H__*/


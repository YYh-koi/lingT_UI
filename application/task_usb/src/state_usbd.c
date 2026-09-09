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
#include "application.h"

#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
enum
{
	USB_IDLE= 0,
	USB_MSDC,
	USB_UVC,
	USB_CDC,
	USB_EXIT,
	USB_ERR
};


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
QueueHandle_t	USBCamApQ;
QueueHandle_t	USBAudioApQ;

static INT32U usb_status = USB_IDLE;
static SemaphoreHandle_t	usb_phy_sem;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
usbd_state_task_init(
	void
)
{
	TASK_APP_ARGUMENT* pUSBDevTaskArgs = &pTurnkeyArgs->taskUSBDevice;
	INT32U usbDevMsgLen = pUSBDevTaskArgs->taskMsgLen;
	INT32U usbDevRecvBufLen = pUSBDevTaskArgs->taskRecvBufLen;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pUSBDevTaskArgs->taskAppQ == 0) {
		pUSBDevTaskArgs->taskAppQ = msgQCreate(usbDevMsgLen,usbDevMsgLen,usbDevRecvBufLen);
		if(pUSBDevTaskArgs->taskAppQ == 0) {
			ERR_PRINT("fail to create pUSBDevTaskQ \r\n");
			goto ERR_USB_DEV_STATE_TASK_INIT;
		}

		if(usbDevRecvBufLen)
		{
			pUSBDevTaskArgs->taskRecvBuf = (void*)malloc(usbDevRecvBufLen);
			if(pUSBDevTaskArgs->taskRecvBuf == 0) {
				ERR_PRINT("fail to malloc pUSBDevRecvBuf \r\n");
				goto ERR_USB_DEV_STATE_TASK_INIT;
			}
		}
	}
	return;

ERR_USB_DEV_STATE_TASK_INIT:
	if(pUSBDevTaskArgs->taskAppQ != 0)
	{
		msgQDelete(pUSBDevTaskArgs->taskAppQ);
		pUSBDevTaskArgs->taskAppQ = 0;
	}

	if(pUSBDevTaskArgs->taskRecvBuf)
	{
		free(pUSBDevTaskArgs->taskRecvBuf);
		pUSBDevTaskArgs->taskRecvBuf = 0;
	}
	return;
}


void usb_phy_lock(void)
{
	//INT8U err;

	if(!usb_phy_sem)
	{
		usb_phy_sem = xSemaphoreCreateMutex();
	}

	if(usb_phy_sem)
	{
		xSemaphoreTake(usb_phy_sem, portMAX_DELAY);
	}
}

void usb_phy_unlock(void)
{
	if(usb_phy_sem)
	{
		xSemaphoreGive(usb_phy_sem);
	}
}

void
state_usb_device_entry(
	void const *arg
)
{

	TASK_APP_ARGUMENT* pUSBDevTaskArgs;
	INT32U usbDevRecvBufLen;
	void* usbDevRecvBuf;
	INT32U  msg_id;
	TickType_t	QReceiveTimeout = portMAX_DELAY;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	pUSBDevTaskArgs = &pTurnkeyArgs->taskUSBDevice;
	usbDevRecvBufLen = pUSBDevTaskArgs->taskRecvBufLen;
	usbDevRecvBuf = pUSBDevTaskArgs->taskRecvBuf;

	while(1)
	{
		msgQReceive_timeout(USBTaskQ, &msg_id, usbDevRecvBuf, usbDevRecvBufLen, QReceiveTimeout);

		usb_phy_lock();

		switch(msg_id)
		{
		case MSG_USBD_INITIAL:
		case MSG_USBCAM_INITIAL:
			usb_mode_detect();
			usb_status = USB_IDLE;
			QReceiveTimeout = portMAX_DELAY;
			break;

		case MSG_USBD_PLUG_IN:
			if(usb_msdc_init() == 0)
			{
				usb_status = USB_MSDC;
				QReceiveTimeout = 1000;	// 1s
			}
			break;

		case MSG_USBCAM_PLUG_IN:
			if(usb_uvc_init() == 0)
			{
				usb_status = USB_UVC;
				QReceiveTimeout = portMAX_DELAY;
			}
			break;

		case MSG_USBCAM_PLUG_OUT:
			usb_status = USB_EXIT;
			usb_uvc_exit();
			QReceiveTimeout = portMAX_DELAY;
			break;

		case MSG_USBD_PLUG_OUT:
			usb_status = USB_EXIT;
			usb_msdc_exit();
			QReceiveTimeout = portMAX_DELAY;
			break;

		case MSG_CHARGE_PLUG_OUT:
			break;

		case MSG_CDC_PLUG_IN:
			usbd_cdc_init();
			usb_status = USB_CDC;
			QReceiveTimeout = 1000;	// 1s
			break;

		case MSG_CDC_PLUG_OUT:
			usb_status = USB_EXIT;
			usbd_cdc_exit();
			QReceiveTimeout = portMAX_DELAY;
			break;

		default:
			if(usb_status == USB_MSDC)
			{
				drv_l2_usbd_msdc_process_insertion(USBD_USE_USB_HW_IDX);
			}

			break;
		}

		usb_phy_unlock();
	}
}

INT32S state_usbd_task_msg_send(INT32U msg, void *para, INT32U bytes, INT32U prio)
{
    INT32S nRet;

	if(USBTaskQ == NULL)
	{
	    nRet = -1;
	    ERR_PRINT("send msg fail\r\n");
	}
    else
    {
        nRet = msgQSend(USBTaskQ, msg, para, bytes, prio);
    }
    return nRet;
}

#endif //#if (defined C_USB_DEVICE) && (C_USB_DEVICE)


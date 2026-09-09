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
#ifndef __USBD_CDC_H__
#define __USBD_CDC_H__

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define MAX_RXBUF_NUM			2

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum{
	MSG_CDC_RX_DATA = 1,
	MSG_CDC_TX_DATA,
	MSG_CDC_RX_NEW_DATA,
	MSG_CDC_FILL_APP_BUF
} CDC_TASK_EVENT_E;

typedef enum{
	MSG_APP_CDC_DATA_GET = 2,
	MSG_APP_CDC_DATA_PUT,
} AP_CDC_TASK_EVENT_E;

typedef enum{
	GET_PKT = 1,
	GET_CHAR
}CDC_GET_TYPE;

typedef struct _cdc_param_s{
	INT16U msg;
	INT32U len;
	INT8U* str;
}cdc_param_t;


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void usbd_cdc_task_init(INT32U usb_id);
extern void usbd_cdc_task_uninit(INT32U usb_id);
extern INT32S cdc_data_put(INT32U usb_id, INT8U* str, INT32U len);
extern INT32S cdc_data_get(INT32U usb_id, INT8U* str, INT32U get_type, INT32U wait);
#endif

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
#include "drv_l2.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define SDC_DET_IO              IO_A14
#define C_USBD_TASK_QHDL_MSDC   1
#define C_USBD_TASK_QHDL_UAC    2
#define C_USBD_TASK_QHDL_UVC    3
#define C_USBD_TASK_QHDL_CDC    4

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
void drv_l2_usbd_user_tmrisr_A(void);
void drv_l2_usbd_user_tmrisr_B(void);
/**************************************************************************
 *                            E X T E R N A L                             *
 **************************************************************************/
extern USBD_USER_PARA_T usbd_userpara[2];
extern xQueueHandle hUSBD_MSDC_TaskQ[2];
extern xQueueHandle hUSBD_UAC_TaskQ[2];
extern xQueueHandle hUSBD_UVC_TaskQ[2];
extern xQueueHandle hUSBD_CDC_TaskQ[2];
extern USBD_L2_DECTECT_CALLBACK_FUN usbl2_sdc_detect_cbk;

extern void drv_l2_usbd_sdc_detio_init(INT32U sd_det_io);
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
INT8U _usbd_detpin[2] = {0, 0}, usbd_deb_flag[2] = {0, 0};
INT32U usbd_det_debcnt[2] = {0, 0};
INT8U connect_2_host[2] = {0, 0};
INT32U usbd_flag_period[2] = {0, 0};
INT8U _sdc_detpin = 1, sdc_deb_flag = 0, sdcd_prev_state = 0;
INT32U sdc_det_debcnt = 0;
ALIGN4 const INT8U USBD_USER_PARA[2][5] =
{
    {   /*Port A*/
         IO_A12,		            //detect pin
         0x05,		                //debounce count(unit 5ms)
         C_USBD_TASK_QHDL_MSDC,     //USBD Q handle Type
         100,                       //timer flag time delay(unit 5ms)
         0                          //timer flag enable/disable initialization
    },
    {   /*Port B*/
         IO_A12,		            //detect pin
         0x05,		                //debounce count(unit 5ms)
         C_USBD_TASK_QHDL_MSDC,     //USBD Q handle Type
         100,                       //timer flag time delay(unit 5ms)
         0
    }
};

ALIGN4 const INT8U UAC_USER_PARA[2][5] =
{
    {   /*Port A*/
         IO_A12,		            //detect pin
         0x05,		                //debounce count(unit 5ms)
         C_USBD_TASK_QHDL_UAC,     //USBD Q handle Type
         100,                       //timer flag time delay(unit 5ms)
         0                          //timer flag enable/disable initialization
    },
    {   /*Port B*/
         IO_A12,		            //detect pin
         0x05,		                //debounce count(unit 5ms)
         C_USBD_TASK_QHDL_UAC,     //USBD Q handle Type
         100,                       //timer flag time delay(unit 5ms)
         0                          //timer flag enable/disable initialization
	 }
};

ALIGN4 const INT8U UVC_USER_PARA[2][5] =
{
    {
         IO_A12,		                //detect pin
         0x05,		                //debounce count(unit 5ms)
         C_USBD_TASK_QHDL_UVC,           //USBD Q handle Type
         100,                       //timer flag time delay(unit 5ms)
         0                          //timer flag enable/disable initialization
	 },
    {
         IO_A12,		                //detect pin
         0x05,		                //debounce count(unit 5ms)
         C_USBD_TASK_QHDL_UVC,           //USBD Q handle Type
         100,                       //timer flag time delay(unit 5ms)
         0                          //timer flag enable/disable initialization
	 }
};

ALIGN4 const INT8U CDC_USER_PARA[2][5] =
{
    {
         IO_A12,		            //detect pin
         0x05,		                //debounce count(unit 5ms)
         C_USBD_TASK_QHDL_CDC,		//USBD Q handle Type
         100,                       //timer flag time delay(unit 5ms)
         0                          //timer flag enable/disable initialization
    },
    {
         IO_A12,		            //detect pin
         0x05,		                //debounce count(unit 5ms)
         C_USBD_TASK_QHDL_CDC,		//USBD Q handle Type
         100,                       //timer flag time delay(unit 5ms)
         0                          //timer flag enable/disable initialization
    }
};
/**************************************************************************
 *                           F U N C T I O N S                            *
 **************************************************************************/
void (*drv_l2_usbd_user_tmrisr[])(void)=
{
    drv_l2_usbd_user_tmrisr_A, drv_l2_usbd_user_tmrisr_B
};

void drv_l2_usbd_user_tmrisr_A(void)
{
    static INT8U usbd_detpin = 0;
    INT32U msg_idx;
    INT32U usb_id = 0;
    //detect host connecting
    usbd_detpin  = gpio_read_io(usbd_userpara[usb_id].detpin);
    if(_usbd_detpin[usb_id]^usbd_detpin)
    {
        _usbd_detpin[usb_id] = usbd_detpin ;
        usbd_det_debcnt[usb_id] = 0;
		usbd_deb_flag[usb_id]  = 1;
    }
    else if(usbd_deb_flag[usb_id])
    {
        if(++usbd_det_debcnt[usb_id]  > usbd_userpara[usb_id].debcnt)
        {
            _usbd_detpin[usb_id]  = usbd_detpin;
			usbd_det_debcnt[usb_id] = 0;
			usbd_deb_flag[usb_id]  = 0;
            if(usbd_detpin && connect_2_host[usb_id] == 0)
            {
                msg_idx = MSG_USB_DEVICE_INSERT;
            }
            else if(usbd_detpin == 0 && connect_2_host[usb_id])
            {
                msg_idx = MSG_USB_DEVICE_REMOVE;
            }
            else
            {
                return;
            }
            #if 0
            cache_invalid_range(usbd_userpara[usb_id].qhandletype, 1);
            #else
            drv_l1_cp15_clean_dcache_range(usbd_userpara[usb_id].qhandletype, 1);
            drv_l1_cp15_invalidate_dcache_range(usbd_userpara[usb_id].qhandletype, 1);
            #endif
            if(usbd_userpara[usb_id].qhandletype == 1)
            {
                if(hUSBD_MSDC_TaskQ[usb_id])
                    xQueueSendFromISR(hUSBD_MSDC_TaskQ[usb_id], &msg_idx, 0);
            }
            else if(usbd_userpara[usb_id].qhandletype == 2)
            {
                if(hUSBD_UAC_TaskQ[usb_id])
                    xQueueSendFromISR(hUSBD_UAC_TaskQ[usb_id], &msg_idx, 0);
            }
            else if(usbd_userpara[usb_id].qhandletype == 3)
            {
                if(hUSBD_UVC_TaskQ[0])
                    xQueueSendFromISR(hUSBD_UVC_TaskQ[0], &msg_idx, 0);
                if(hUSBD_UVC_TaskQ[1])
                    xQueueSendFromISR(hUSBD_UVC_TaskQ[1], &msg_idx, 0);
            }
            else if(usbd_userpara[usb_id].qhandletype == 4)
            {
                if(hUSBD_CDC_TaskQ[usb_id])
                {
                    xQueueSendFromISR(hUSBD_CDC_TaskQ[usb_id], &msg_idx, 0);
                }
            }
        }
    }
    if(++usbd_flag_period[usb_id] > usbd_userpara[usb_id].tmrflag_period)
    {
        usbd_flag_period[usb_id] = 0;
        if(usbd_userpara[usb_id].tmrflagOnOff)
        {
            msg_idx = MSG_USB_DEVICE_TMR_TIMEOUT;
            if(usbd_userpara[usb_id].qhandletype == 1)
            {
                if(hUSBD_MSDC_TaskQ[usb_id])
                {
                    xQueueSendFromISR(hUSBD_MSDC_TaskQ[usb_id], &msg_idx, 0);
                }
            }
            else if(usbd_userpara[usb_id].qhandletype == 2)
            {
                if(hUSBD_UAC_TaskQ[usb_id])
                {
                    xQueueSendFromISR(hUSBD_UAC_TaskQ[usb_id], &msg_idx, 0);
                }
            }
            else if(usbd_userpara[usb_id].qhandletype == 3)
            {
                if(hUSBD_UVC_TaskQ[usb_id])
                {
                    xQueueSendFromISR(hUSBD_UVC_TaskQ[usb_id], &msg_idx, 0);
                }
            }
            else if(usbd_userpara[usb_id].qhandletype == 4)
            {
                if(hUSBD_CDC_TaskQ[usb_id])
                {
                    xQueueSendFromISR(hUSBD_CDC_TaskQ[usb_id], &msg_idx, 0);
                }
            }
        }
    }
    if(usbl2_sdc_detect_cbk)
    {
        usbl2_sdc_detect_cbk(usb_id);
    }
}

void drv_l2_usbd_user_tmrisr_B(void)
{
    static INT8U usbd_detpin = 0;
    INT32U msg_idx;
    INT32U usb_id = 1;

    //detect host connecting
    usbd_detpin = gpio_read_io(usbd_userpara[usb_id].detpin);
    if(_usbd_detpin[usb_id] ^usbd_detpin )
    {
        _usbd_detpin[usb_id]  = usbd_detpin;
        usbd_det_debcnt[usb_id] = 0;
		usbd_deb_flag[usb_id]  = 1;
    }
    else if(usbd_deb_flag[usb_id] )
    {
        if(++usbd_det_debcnt[usb_id] > usbd_userpara[usb_id].debcnt)
        {
            _usbd_detpin[usb_id]  = usbd_detpin ;
			usbd_det_debcnt[usb_id] = 0;
			usbd_deb_flag[usb_id]  = 0;
            if(usbd_detpin && connect_2_host[usb_id] == 0)
            {
                msg_idx = MSG_USB_DEVICE_INSERT;
            }
            else if(usbd_detpin == 0 && connect_2_host[usb_id])
            {
                msg_idx = MSG_USB_DEVICE_REMOVE;
            }
            else
            {
                return;
            }

            drv_l1_cp15_clean_dcache_range(usbd_userpara[usb_id].qhandletype, 1);
            drv_l1_cp15_invalidate_dcache_range(usbd_userpara[usb_id].qhandletype, 1);

            if(usbd_userpara[usb_id].qhandletype == 1)
            {
                if(hUSBD_MSDC_TaskQ[usb_id])
                    xQueueSendFromISR(hUSBD_MSDC_TaskQ[usb_id], &msg_idx, 0);
            }
            else if(usbd_userpara[usb_id].qhandletype == 2)
            {
                if(hUSBD_UAC_TaskQ[usb_id])
                    xQueueSendFromISR(hUSBD_UAC_TaskQ[usb_id], &msg_idx, 0);
            }
            else if(usbd_userpara[usb_id].qhandletype == 3)
            {
                if(hUSBD_UVC_TaskQ[usb_id])
                    xQueueSendFromISR(hUSBD_UVC_TaskQ[usb_id], &msg_idx, 0);
            }
            else if(usbd_userpara[usb_id].qhandletype == 4)
            {
                if(hUSBD_CDC_TaskQ[usb_id])
                {
                    xQueueSendFromISR(hUSBD_CDC_TaskQ[usb_id], &msg_idx, 0);
                }
            }
        }
    }
    if(++usbd_flag_period[usb_id] > usbd_userpara[usb_id].tmrflag_period)
    {
        usbd_flag_period[usb_id] = 0;
        if(usbd_userpara[usb_id].tmrflagOnOff)
        {
            msg_idx = MSG_USB_DEVICE_TMR_TIMEOUT;
            if(usbd_userpara[usb_id].qhandletype == 1)
            {
                if(hUSBD_MSDC_TaskQ[usb_id])
                {
                    xQueueSendFromISR(hUSBD_MSDC_TaskQ[usb_id], &msg_idx, 0);
                }
            }
            else if(usbd_userpara[usb_id].qhandletype == 2)
            {
                if(hUSBD_UAC_TaskQ[usb_id])
                {
                    xQueueSendFromISR(hUSBD_UAC_TaskQ[usb_id], &msg_idx, 0);
                }
            }
            else if(usbd_userpara[usb_id].qhandletype == 3)
            {
                if(hUSBD_UVC_TaskQ[usb_id])
                {
                    xQueueSendFromISR(hUSBD_UVC_TaskQ[usb_id], &msg_idx, 0);
                }
            }
            else if(usbd_userpara[usb_id].qhandletype == 4)
            {
                if(hUSBD_CDC_TaskQ[usb_id])
                {
                    xQueueSendFromISR(hUSBD_CDC_TaskQ[usb_id], &msg_idx, 0);
                }
            }
        }
    }
    if(usbl2_sdc_detect_cbk)
    {
        usbl2_sdc_detect_cbk(usb_id);
    }
}


void _usbd_sdc_cd(INT32U usb_id)
{
    static INT8U sdc_detpin = 0;
    INT32U msg_idx;

    sdc_detpin = gpio_read_io(SDC_DET_IO);
    if(_sdc_detpin^sdc_detpin)
    {
        _sdc_detpin = sdc_detpin;
        sdc_det_debcnt = 0;
		sdc_deb_flag = 1;
    }
    else if(sdc_deb_flag)
    {
        if(++sdc_det_debcnt > 30)
        {
            _sdc_detpin = sdc_detpin;
			sdc_det_debcnt = 0;
			sdc_deb_flag = 0;
            if(sdc_detpin && sdcd_prev_state)
            {
                msg_idx = MSG_USB_SDC_CARDOUT;
                sdcd_prev_state = 0;
            }
            else if(sdc_detpin == 0 && sdcd_prev_state == 0)
            {
                msg_idx = MSG_USB_SDC_CARDIN;
                sdcd_prev_state = 1;
            }
            else
            {
                return;
            }

            if(hUSBD_MSDC_TaskQ[usb_id])
                xQueueSendFromISR(hUSBD_MSDC_TaskQ[usb_id], &msg_idx, 0);

            if(hUSBD_CDC_TaskQ[usb_id])
                xQueueSendFromISR(hUSBD_CDC_TaskQ[usb_id], &msg_idx, 0);
        }
    }
}

void drv_l2_usbd_usr_sdc_det_io_init(void)
{
    drv_l2_usbd_sdc_detio_init(SDC_DET_IO);
}

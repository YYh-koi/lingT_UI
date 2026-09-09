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
#ifndef __DRV_L2_SCCB_H__
#define __DRV_L2_SCCB_H__

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct
{
    GPIO_ENUM       scl_port;
    IO_DRV_LEVEL    scl_drv;
    GPIO_ENUM       sda_port;
    IO_DRV_LEVEL    sda_drv;
    GPIO_ENUM       pwdn_port;
    IO_DRV_LEVEL    pwdn_drv;   // not yet implemented
    INT8U           have_pwdn;
    INT8U           RegBits;
    INT8U           DataBits;
    INT8U           slaveAddr;
    INT32U          timeout;    // in ms unit, not yet implemented
    INT32U          clock_rate; // in kHz unit, not yet implemented
}sccb_config_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void *drv_l2_sccb_open(INT8U ID, INT8U RegBits, INT8U DataBits);
extern void *drv_l2_sccb_open_ext(sccb_config_t *pSccbConfig);
extern void drv_l2_sccb_close(void *handle);
extern INT32S drv_l2_sccb_write(void *handle, INT32U addr, INT32U data);
extern INT32S drv_l2_sccb_read(void *handle, INT32U addr, INT16U *data);
extern INT32S drv_l2_sccb_continue_write(void *handle, INT32U addr, INT8U* data, INT32U len);
extern INT32S drv_l2_sccb_continue_read(void *handle, INT32U addr, INT8U *data, INT32U len);
extern INT32S drv_l2_sccb_seg_p_edid(void *handle_seg, void *handle, INT16U addr_seg, INT16U addr, INT8U *data, INT32U len);

#endif  //DRV_L2_SCCB_H

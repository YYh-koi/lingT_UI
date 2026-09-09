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
#ifndef __DRV_L2_HDMI_H__
#define __DRV_L2_HDMI_H__

extern int hdmi_read_EDID(INT8U *p);
extern int hdmi_read_block(INT8U index, INT8U *p, INT16U length);
extern INT32S ddc_sccb_open(void);
extern INT32S drv_l2_hdmi_hdcp_init(INT32U cnt);
extern INT32S drv_l2_hdmi_hdcp_stop(void);
extern INT8U drv_l2_hdmi_get_supported_resolution(void);
extern INT32U drv_l2_check_EDID(void);
extern void drv_l2_hdmi_set_video(INT8U resolution);
extern INT8U drv_l2_hdmi_get_video(void);
extern INT8U drv_l2_hdmi_get_audio(void);
#ifdef __cplusplus
extern "C" {
#endif

int drvl2_hdmi_init(unsigned int DISPLAY_MODE, unsigned int AUD_FREQ);


#ifdef __cplusplus
}
#endif

#endif		// __DRV_L2_HDMI_H__


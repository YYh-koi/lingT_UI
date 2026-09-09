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

void drv_l2_cdsp_enable(gpUserFav_t *pFavTable, INT16U sensor_w, INT16U sensor_h, INT16U target_w, INT16U target_h, INT32U sensor_id)
{
    INT32U x = 32;
    INT32U y = 32;
    INT32U w = (sensor_w - (x << 1)) >> 3;
    INT32U h = (sensor_h - (y << 1)) >> 3;

	#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
    INT32U dn3d_en = DISABLE;
	#else
	#if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
    INT32U dn3d_en = DISABLE;
	#else
	INT32U dn3d_en = ENABLE;
	#endif
	#endif

	drv_l2_isp_set_sensor_id(sensor_id);

    drv_l2_cdsp_enable_pre(pFavTable, sensor_id);

    drv_l2_CdspUserAeTableRegister(pFavTable);

    drv_l2_cdsp_set_badpixel_ob();
    drv_l2_cdsp_set_hybrid(sensor_w, sensor_h);
    drv_l2_cdsp_set_edge(0);

	drv_l2_cdsp_set_ae_target(pFavTable->ae_target);

	drv_l2_cdsp_set_ae_window(sensor_w, sensor_h, pFavTable->ae_meter, pFavTable->lpAE_weight_table_f);

	if(pFavTable->awb_mode == AWB_AUTO_DC2)
        drv_l2_cdsp_set_awb_RdG_BdG(ENABLE, sensor_w, sensor_h);
    else if(pFavTable->awb_mode == AWB_AUTO_DC)
    {
        x = (sensor_w / 30) & 0xfffe;
        w = ((sensor_w - 2*x) & 0xfffe);
        y = (sensor_h / 8) & 0xfffe;
        h = ((sensor_h - 2*y) & 0xfffe);
        drv_l2_cdsp_set_new_wb_window(ENABLE, x, y, w, h);
	}

	drv_l2_cdsp_set_wb_gain(ENABLE);

	drv_l2_isp_set_af_window(DISABLE, sensor_w, sensor_h,pFavTable);


    if(drv_l1_CdspGetCropEn()!=0)
    {
        sensor_w = target_w = R_CDSP_IMCROP_HSIZE;
        sensor_h = target_h = R_CDSP_IMCROP_VSIZE;
    }

    drv_l2_isp_set_mesh_lsc(DISABLE);

    #if ((H264ENC_FBC_ENABLE == 1) && (C_WIFI_FUNC ==0))
    if(sensor_w <= 2592 && dn3d_en == ENABLE)   //If 3DNR on and sensor width more than 2048, suggestion sensor frame rate set to 25~27fps
    #else
    if(sensor_w <= 2048 && dn3d_en == ENABLE)
    #endif
    {
        drv_l2_isp_set_dn3d(dn3d_en, (sensor_w & 0x1FFF), (sensor_h & 0x1FFF));
	}


   drv_l2_isp_set_nlm(ENABLE, sensor_w, sensor_h);
   //rv_l2_isp_set_nlm(DISABLE, sensor_w, sensor_h);

	drv_l2_cdsp_set_intp(900, 20);
    drv_l2_cdsp_set_wdr(ENABLE);
    drv_l2_cdsp_set_histgm(1, 32, sensor_w - 32, 32, sensor_h - 32);
	drv_l2_cdsp_set_colormatrix(ENABLE);
    drv_l2_cdsp_set_rgb_gamma(ENABLE);

    drv_l2_cdsp_set_sharpen(ENABLE, sensor_w, pFavTable->day_edge);
  // drv_l2_cdsp_set_sharpen(DISABLE, sensor_w, pFavTable->day_edge);

	drv_l2_isp_set_denoise(ENABLE, ENABLE);
	//drv_l2_isp_set_denoise(DISABLE, DISABLE);
}




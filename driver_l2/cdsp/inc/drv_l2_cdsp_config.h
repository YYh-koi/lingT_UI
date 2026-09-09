#ifndef __drv_l2_CDSP_CONFIG_H__
#define __drv_l2_CDSP_CONFIG_H__

#include "drv_l2_user_preference.h"

extern INT32S drv_l2_cdsp_set_badpixel_ob(void);
extern INT32S drv_l2_cdsp_set_wb_gain(INT8U enbit);
extern INT32S drv_l2_cdsp_set_ae_target(INT32S ae_target);
extern INT32S drv_l2_cdsp_set_ae_window(INT16U width, INT16U height, INT8U ae_meter, INT32U *lpAE_weight_table);
extern INT32S drv_l2_cdsp_set_new_wb_window(INT8U en, INT16U x, INT16U y, INT16U w, INT16U h);
extern INT32S drv_l2_cdsp_set_gamma(void);
extern INT32S drv_l2_cdsp_set_wb_offset(INT32S wb_offset_day, INT32S wb_offset_night);
extern INT32S drv_l2_cdsp_set_intp(INT32U hi_thr, INT32U low_thr);
extern INT32S drv_l2_cdsp_set_colormatrix(INT8U en);
extern INT32S drv_l2_cdsp_set_histgm(INT8U hisEn, INT16U HStart, INT16U HEnd, INT16U VStart, INT16U VEnd);

extern INT32S drv_l2_cdsp_set_edge(INT32S sharpness);
extern INT32S drv_l2_cdsp_set_new_denoise(INT32S sharpness);
extern INT32S drv_l2_cdsp_set_saturation_day(INT32S u_offset, INT32S v_offset, INT32S u_scale, INT32S v_scale);
extern INT32S drv_l2_cdsp_set_saturation_night(INT32S u_offset, INT32S v_offset, INT32S u_scale, INT32S v_scale);
extern INT32S drv_l2_cdsp_set_backlight_dt(INT32U en);
extern INT32S drv_l2_cdsp_set_awb_mode(INT32U mode);
extern INT32S drv_l2_cdsp_set_ev_val(INT32S ev);
extern INT32S drv_l2_cdsp_raw_special_mode(INT32U mode);
extern INT32S drv_l2_cdsp_set_iso(INT32U iso);
extern INT32S drv_l2_cdsp_set_yuv_dma_scale(INT16U dst_w, INT16U dst_h, INT8U dma_hscale_en);
extern INT32S drv_l2_cdsp_set_yuv_scale(INT16U dst_w, INT16U dst_h);
extern INT32S drv_l2_cdsp_set_cv_para(INT8U *lpCV_para);
extern INT32S drv_l2_cdsp_set_lens_compensation(INT32U en, INT16U r_ratio, INT16U g_ratio, INT16U b_ratio);

extern void drv_l2_cdsp_enable(gpUserFav_t *pFavTable, INT16U sensor_w, INT16U sensor_h, INT16U target_w, INT16U target_h, INT32U sensor_id);
extern void drv_l2_cdsp_enable_pre(gpUserFav_t *FavTable, INT32U sensor_id);

extern INT32S drv_l2_cdsp_set_awb_RdG_BdG(INT8U awben, INT32U sensor_w, INT32U sensor_h);
extern INT32S drv_l2_cdsp_set_hybrid(INT16U sensor_w, INT16U sensor_h);
extern INT32S drv_l2_cdsp_set_rgb_gamma(INT8U en);
extern INT32S drv_l2_cdsp_set_sharpen(INT8U hisen, INT16U width, INT8U sharpen_s);
extern INT32S drv_l2_cdsp_set_wdr(INT8U en);

extern INT32S drv_l2_isp_set_af_window(INT8U afen, INT32U img_w, INT32U img_h,gpUserFav_t *FavTable);
extern INT32S drv_l2_isp_set_denoise(INT8U NRen, INT8U AJen);
extern INT32S drv_l2_isp_set_dn3d(INT8U en, INT32U width, INT32U height);
extern INT32S drv_l2_isp_set_ext_dn3d_buffer(INT32U addr);
extern INT32S drv_l2_isp_set_mesh_lsc(INT32U en);
extern INT32S drv_l2_isp_set_nlm(INT8U en, INT32U width, INT32U height);
extern INT32S drv_l2_isp_set_sensor_id(INT32S sensor_id);

extern void drv_l2_gp_cdsp_set_awb_by_config(INT32U awb_value);
extern void drv_l2_3dnr_flag_set(INT8U onoff);
extern INT8U drv_l2_3dnr_flag_get(void);

extern void drv_l2_cdsp_ReDoConfig( INT32U target_w, INT32U target_h);

extern void drv_l2_cdsp_select_max_lum(int selection);



#endif	/*__drv_l2_CDSP_H__*/

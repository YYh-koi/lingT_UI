#ifndef __GP_LENSDEF_H__
#define __GP_LENSDEF_H__

#include "gp_3a.h"

typedef struct gpUserFav_s
{
	unsigned char ae_target;			/* target luminace */

    unsigned short iso;

    unsigned char ae_meter;
    unsigned int  *lpAE_weight_table_f;//when ae_meter = CDSP_AE_METER_USER_WEIGHT, use this table! table size = 64; table type = unsigned int
	unsigned char awb_mode;
	unsigned char color_mode;

	signed char   ev;

	signed char	day_hue_u_offset;	/* -128 ~ +127,   +: more blue,  -: more yellow/green	*/
	signed char	day_hue_v_offset;	/* -128 ~ +127,   +: more red,  -: more blue/green */

	unsigned char	day_sat_u_scale;
	unsigned char	day_sat_v_scale;

	unsigned char	day_edge;			/*	0:soft, 1:smooth, 2:normal, 3:strong */

	signed char	night_hue_u_offset;	/* -128 ~ +127,   +: more blue,  -: more yellow/green	*/
	signed char	night_hue_v_offset;	/* -128 ~ +127,   +: more red,  -: more blue/green */

	unsigned char	night_sat_u_scale;
	unsigned char	night_sat_v_scale;

	unsigned char	night_edge;			/*	0:soft, 1:smooth, 2:normal, 3:strong */
	signed int      wb_factor;


    int awb_speed; // 1 (slow) ~ 256 (fast), default: 128
    unsigned char max_lum;
	unsigned char max_lum2;
	unsigned char   (*UserAeTable)[17];


	unsigned char af_mode;//default AF_STEPINC//AF_FULLSEARCH
	unsigned char af_step;//default 2
	unsigned char af_max_index;//default 40
	unsigned char af_min_index;//default 16



} gpUserFav_t;

typedef struct gpCsiCali_s
{
	signed short    *ob;
	/*mash lsc*/
	unsigned int	*meshlsc_grid;
	unsigned int	*meshlsc_gain_2800k;
	unsigned int	*meshlsc_gain_4000k;
	unsigned int	*meshlsc_gain_6500k;

	unsigned int   *rgb_gamma;
	unsigned int   *rgb_gamma2;

	signed short   *awb_thr;
	unsigned short awb_thr_cnt;

	signed short   *awb_thr_excpt_2800k;
	unsigned short awb_thr_excpt_2800k_cnt;
	signed short   *awb_thr_excpt_4000k;
	unsigned short awb_thr_excpt_4000k_cnt;
	signed short   *awb_thr_excpt_5000k;
	unsigned short awb_thr_excpt_5000k_cnt;
	signed short   *awb_thr_excpt_6500k;
	unsigned short awb_thr_excpt_6500k_cnt;

	signed short   *awb_new_thr;
	unsigned short awb_new_thr_cnt;

	signed short   *color_matrix;
	signed short   *color_matrix2;
	signed short   *color_matrix3;

	/*YUV Spec mode constrat*/
	unsigned int	*y_lut;
	unsigned int	*y_lut2;

    unsigned short  *wb_adj;

    unsigned short  *lsc_ref_wb_gain;
    unsigned short  *lsc_rgb_cmp_ratio;

	unsigned short  (*NRthr)[5];
	unsigned short  (*DPCthr)[4];

	/*NLM*/
	unsigned short   (*nlm_thr_wt)[84];

	/*DN3D*/
	unsigned char	(*dn3d_lum_thr)[32];

	/*YUVNR*/
	unsigned char	(*yuvnr)[8];

} gpCisCali_t;



#endif


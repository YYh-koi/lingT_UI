#ifndef __GP_PREFERENCE_H__
#define __GP_PREFERENCE_H__

#include "..\gp_LensDef.h"




static const INT8U g_UserAeTable[17] =
{

0,7,14,21,27,33,38,42,42,42,42,42,42,42,42,42,42

};

static const gpUserFav_t g_FavTable =
{
	.ae_target = 95,

	.ae_meter = CDSP_AE_METER_AVG_T2,//CDSP_AE_METER_CENTER_WEIGHTED_AVG_CVR,
	.awb_mode = AWB_AUTO_DC,
	.color_mode = 0,
	.iso = 0,

	.ev = 0,

	.day_hue_u_offset = 2,	//-128 ~ +127,   +: more blue,  -: more yellow/green
	.day_hue_v_offset = 2,	//-128 ~ +127,   +: more red,  -: more blue/green

	.day_sat_u_scale = 0x40, //Default:0x20 	// blud
	.day_sat_v_scale = 0x40, //Default:0x20		// red

	.day_edge = 32, 		/*	0(smooth) ~ 127(sharp)*/

	.night_hue_u_offset = 2, //-128 ~ +127,   +: more blue,  -: more yellow/green
	.night_hue_v_offset = 2, //-128 ~ +127,   +: more red,  -: more blue/green

	.night_sat_u_scale = 0x30,//Default:0x20, blue
	.night_sat_v_scale = 0x30,//Default:0x20, red

	.night_edge = 26,//30,

	.wb_factor = 64,
	 .awb_speed= 128,

	.max_lum = (128 - 20),//(128 - 40)
 	.max_lum2 = (128 - 1),
	.UserAeTable = (unsigned char (*)[17] )g_UserAeTable,


	.af_mode = 0,//default AF_STEPINC//AF_FULLSEARCH
	.af_step = 2,//default 2
	.af_max_index = 40,//default 40
	.af_min_index = 16//default 16

};


#endif


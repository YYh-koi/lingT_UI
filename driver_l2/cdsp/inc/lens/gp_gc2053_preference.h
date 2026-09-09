#ifndef __GP_GC2053_PREFERENCE_H__
#define __GP_GC2053_PREFERENCE_H__

#include "..\gp_LensDef.h"


static const INT8U g_UserAeTable[17] =
{
#if 0
 0,
 3, 6, 9,12,15,18,21,24,
 27,30,33,36,36,36,36,36,
#else
   0,
   7,14,21,27,33,38,42,42,
  42,42,42,42,42,42,42,42
#endif
};


static const gpUserFav_t g_FavTable =
{
	.ae_target = 92,

	.ae_meter = CDSP_AE_METER_CENTER_WEIGHTED_AVG_CVR,//CDSP_AE_METER_AVG_T2,
	.awb_mode = AWB_AUTO_DC,
	.color_mode = 0,
	.iso = 0,

	.ev = 0,

	.day_hue_u_offset = 0,	//-128 ~ +127,   +: more blue,  -: more yellow/green
	.day_hue_v_offset = 0,	//-128 ~ +127,   +: more red,  -: more blue/green

	.day_sat_u_scale = 0x30, //Default:0x20 	// blud
	.day_sat_v_scale = 0x30, //Default:0x20		// red

	.day_edge = 30, 		/*	0(smooth) ~ 127(sharp)*/

	.night_hue_u_offset = 0, //-128 ~ +127,   +: more blue,  -: more yellow/green
	.night_hue_v_offset = 0, //-128 ~ +127,   +: more red,  -: more blue/green

	.night_sat_u_scale = 0x26,//Default:0x20, blue
	.night_sat_v_scale = 0x26,//Default:0x20, red

	.night_edge = 26,//30,

	.wb_factor = 64,

	.max_lum = (128 - 20),//(128 - 40)
	.UserAeTable = (unsigned char (*)[17] )g_UserAeTable
};


#endif


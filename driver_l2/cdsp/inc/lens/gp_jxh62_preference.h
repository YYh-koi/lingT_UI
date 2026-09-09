#ifndef __GP_JXH62_PREFERENCE_H__
#define __GP_JXH62_PREFERENCE_H__

#include "..\gp_LensDef.h"


static const INT8U g_UserAeTable[17] =
{
    0,3,6,9,12,15,18,21,24,27,30,33,36,36,36,36,36
};

static const gpUserFav_t g_FavTable =
{
	.ae_target = 95,
	//.ae_target_night = 94,

	.ae_meter = CDSP_AE_METER_AVG_T2,
	.awb_mode = AWB_AUTO_DC,
	.color_mode = 0,
	.iso = 0,

	.ev = 0,

	.day_hue_u_offset = 0, //-128 ~ +127,   +: more blue,  -: more yellow/green
	.day_hue_v_offset = 0,	//-128 ~ +127,   +: more red,  -: more blue/green

	.day_sat_u_scale = 0x2a, //Default:0x20 	// blud
	.day_sat_v_scale = 0x2a, //Default:0x20		// red

	.day_edge = 23, /*	0(smooth) ~ 127(sharp)*/

	.night_hue_u_offset = 0, //chen //-128 ~ +127,   +: more blue,  -: more yellow/green
	.night_hue_v_offset = 0, //-128 ~ +127,   +: more red,  -: more blue/green

	.night_sat_u_scale = 0x20,//Default:0x20, blue
	.night_sat_v_scale = 0x20,//Default:0x20, red

	.night_edge = 20,

	.wb_factor = 64,

	.max_lum = (128 - 4),//(128 - 40)
	.UserAeTable = (unsigned char (*)[17] )g_UserAeTable
};


#endif


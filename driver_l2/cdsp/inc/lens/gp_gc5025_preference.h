#ifndef __GP_GC5025_PREFERENCE_H__
#define __GP_GC5025_PREFERENCE_H__

#include "..\gp_LensDef.h"

static const INT8U g_UserAeTable[17] =
{
    0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,30
};

static const gpUserFav_t g_FavTable =
{
	.ae_target = 102,//102    //130 for 4P, //115 for 3P

	.ae_meter = CDSP_AE_METER_AVG_T2,
	.awb_mode = AWB_AUTO_DC,
	.color_mode = 0,
	.iso = 0,

	.ev = 0,

	.day_hue_u_offset = 0,	//-128 ~ +127,   +: more blue,  -: more yellow/green
	.day_hue_v_offset = 0,	//-128 ~ +127,   +: more red,  -: more blue/green

	.day_sat_u_scale = 0x2a,//0x27, //0x20, //Default:0x20 // blue
	.day_sat_v_scale = 0x28, //0x1E, //Default:0x20	// red

	.day_edge = 30,							 /*	0(smooth) ~ 127(sharp)*/

	.night_hue_u_offset = 0, //-128 ~ +127,   +: more blue,  -: more yellow/green
	.night_hue_v_offset = 0, //-128 ~ +127,   +: more red,  -: more blue/green

	.night_sat_u_scale = 0x21,	//Default:0x20, blue
	.night_sat_v_scale = 0x21,	//Default:0x20, red

	.night_edge = 20,

	.wb_factor = 64,

	.max_lum = (128 - 10),

	.UserAeTable = (unsigned char (*)[17] )g_UserAeTable

};


#endif


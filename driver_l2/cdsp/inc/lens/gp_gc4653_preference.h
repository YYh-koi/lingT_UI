static const INT8U g_UserAeTable[17] =
{
    //0,4,8,12,16,20,24,28,  32,36,40,   40,40,40,40,40,40//42,45,46,50
         //--------------------------------789-------10 11 12------13 14 15
          //  0,5,10,15,20,25,30,35,    40,45,52,    55,58,58,    58,58,58
//最后一档的亮度最好大于38，否则很暗的时候ae不稳定
       // 0,5,10,15,20,25,30,35,     40,45,50,    50,50,50,     50,50,52
        //0,6,12,18,24,30,36,42,     46,50,50,    50,50,50,     50,52,52//0314

         0,4,8,12,16,20,24,28,     32,36,40,    44,48,52,     52,52,52

       // 0,3,6,9,12,15,18,21,     24,27,30,    33,33,33,     33,33,33
};


static const gpUserFav_t g_FavTable =
{
 .ae_target = 92,      //130 for 4P, //115 for 3P

 .ae_meter = CDSP_AE_METER_CENTER_WEIGHTED_AVG_CVR,
 .awb_mode = AWB_AUTO_DC2,
 .color_mode = 0,
 .iso = 0,

 .ev = 0,


 .day_hue_u_offset = 0,  //-128 ~ +127,   +: more blue,  -: more yellow/green
 .day_hue_v_offset = 0,  //-128 ~ +127,   +: more red,   -: more blue/green

 .day_sat_u_scale = 0x2d, //Default:0x20 // blue
 .day_sat_v_scale = 0x2d,  //Default:0x20 // red

 .day_edge = 40,     /* 0(smooth) ~ 127(sharp)*/

 .night_hue_u_offset = 0,  //-128 ~ +127,   +: more blue,  -: more yellow/green
 .night_hue_v_offset = 0,  //-128 ~ +127,   +: more red,  -: more blue/green

 .night_sat_u_scale = 0x18, //Default:0x20, blue
 .night_sat_v_scale = 0x18, //Default:0x20, red

 .night_edge = 35,

 .wb_factor = 65,


.awb_speed= 64,


.max_lum = (128 - 8),//(128 - 40)
.max_lum2 = (128 - 1),


 .UserAeTable = (unsigned char (*)[17] )g_UserAeTable

};

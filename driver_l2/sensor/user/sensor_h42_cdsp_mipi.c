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

#if (defined _SENSOR_H42_CDSP_MIPI) && (_SENSOR_H42_CDSP_MIPI == 1)

#include "drv_l2_user_calibration.h"
#include "drv_l2_user_preference.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define MIPI_ISR_TEST                       0
#define READ_TABLE_EN                       0
#define MIPI_LANE_NO			            MIPI_1_LANE     // 1 or 2 lane
#define MIPI_DEV_NO                         MIPI_1          //0:MIPI_0(IOC8~11) or 1:MIPI_1(IOE0~9)

#define SENSOR_FLIP			                0
#define	H42_ID							    0x60//0x64,0x68,0x6C
#define Group_Write_En                      0

#define H42_WIDTH						    1280
#define H42_HEIGHT						    720
#define H42_OUT_WIDTH					    1280
#define H42_OUT_HEIGHT					    720

#define H42_I2C_MODE	                    SENSOR_I2C_MODE_HW

#if H42_I2C_MODE == SENSOR_I2C_MODE_SW
    #define H42_GPIO_SCL		            IO_B4
    #define H42_GPIO_SDA		            IO_B5
    #define H42_GPIO_DRIVING	            IOB_DRV_30mA
#else
    #define H42_I2C_DEVICE		            I2C_0
#endif

#define H42_30FPS_50HZ_DAY_EV_IDX 			140
#define H42_30FPS_50HZ_NIGHT_EV_IDX			190
#define H42_30FPS_50HZ_EXP_TIME_TOTAL		209
#define H42_30FPS_50HZ_INIT_EV_IDX 			((H42_30FPS_50HZ_DAY_EV_IDX + H42_30FPS_50HZ_NIGHT_EV_IDX) >> 1)
#define H42_30FPS_50HZ_MAX_EV_IDX			(H42_30FPS_50HZ_EXP_TIME_TOTAL - 10)

#define H42_30FPS_60HZ_DAY_EV_IDX 			137
#define H42_30FPS_60HZ_NIGHT_EV_IDX			190
#define H42_30FPS_60HZ_EXP_TIME_TOTAL		215
#define H42_30FPS_60HZ_INIT_EV_IDX 			((H42_30FPS_60HZ_DAY_EV_IDX + H42_30FPS_60HZ_NIGHT_EV_IDX) >> 1)
#define H42_30FPS_60HZ_MAX_EV_IDX			 (H42_30FPS_60HZ_EXP_TIME_TOTAL - 10)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct regval8_s
{
	INT8U reg_num;
	INT8U value;
} regval8_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static int *p_expTime_table;
static sensor_exposure_t 	seInfo;
static int pre_sensor_a_gain, pre_sensor_time;

#if H42_I2C_MODE == SENSOR_I2C_MODE_SW
sccb_config_t h42_sccb_config =
{
    H42_GPIO_SCL,				//scl_port
    H42_GPIO_DRIVING,			//scl_drv
    H42_GPIO_SDA,				//sda_port
    H42_GPIO_DRIVING,			//sda_drv
    IO_F15,						//pwdn_port
    0,	                        //pwdn_drv
    0,							//have_pwdn
    8,							//RegBits
    8,							//DataBits
    H42_ID,					//slaveAddr
    0x20000,					//timeout
    200							//clock_rate
};
static void *h42_handle;
#elif H42_I2C_MODE == SENSOR_I2C_MODE_HW
static drv_l1_i2c_bus_handle_t h42_handle;
#endif

static mipi_config_t h42_mipi_cfg =
{
	DISABLE, 			/* low power, 0:disable, 1:enable */

#if MIPI_LANE_NO == MIPI_1_LANE
	D_PHY_SAMPLE_POS, 	/* byte clock edge, 0:posedge, 1:negedge */
	MIPI_1_LANE,		/* lane */
	D_PHY_BYTE_CLK,	    /* byte clock source */
#elif MIPI_LANE_NO == MIPI_2_LANE
	D_PHY_SAMPLE_POS,
	MIPI_2_LANE,		/* lane */
	GENERATE_PIXEL_CLK,	/* byte clock source */
#endif

	MIPI_AUTO_DETECT,	/* data mode, 0:auto detect, 1:user define */
	MIPI_RAW10,			/* data type, valid when data mode is 1*/
	MIPI_DATA_TO_CDSP,	/* data type, 1:data[7:0]+2':00 to cdsp, 0: 2'00+data[7:0] to csi */
	0,  				/* RSD 2 */

	H42_WIDTH,	    	/* width, 0~0xFFFF */
	H42_HEIGHT, 		/* height, 0~0xFFFF */
	4, 					/* back porch, 0~0xF */ //If Set 4 have show paper color at left side.u2.20160119
	4,					/* front porch, 0~0xF */
	ENABLE,				/* blanking_line_en, 0:disable, 1:enable */
	0,  				/* RSD 3 */

	ENABLE,				/* ecc, 0:disable, 1:enable */
	MIPI_ECC_ORDER3,	/* ecc order */
	200,
	MIPI_CHECK_LP_00    /* check hs sequence or LP00 for clock lane */
};

static const regval8_t h42_reset_table[] =
{
	{ 0x12, 0x80 },
	{ 0xFF, 0xFF },
};

static const regval8_t h42_init_table[] =
{
	//INI Start
	{0x12,0x40},
	//DVP Setting
	{0x0D,0x40},
	{0x1F,0x04},
	//PLL Setting
	#if 1
	{0x0E,0x1d},
	{0x0F,0x09},
    ///{0x10,0x1E},
	{0x10,0x1C},
	#else 	//Pclk = Mclk =>12M 影像效果不好
	{0x0E,0x1E},	//bypass mode
	{0x0F,0x09},
	{0x10,0x12},
	#endif
	{0x11,0x80},
	//Frame/Window
	#if 0 	//30fps
	{0x20,0x40},
	{0x21,0x06},
	{0x22,0xEE},
	{0x23,0x02},
	#else 	// 26 fps
	{0x20,0xDB},
	{0x21,0x06},
	{0x22,0x23},//23//13
	{0x23,0x03},
	#endif
	{0x24,0x04}, //w
	{0x25,0xD4}, //h
	{0x26,0x25},
	{0x27,0x45},//49}
	{0x28,0x0D},
	{0x29,0x01},
	{0x2A,0x24},
	{0x2B,0x29},
	{0x2C,0x00},
	{0x2D,0x00},
	{0x2E,0xB9},
	{0x2F,0x00},
	//Sensor Timing
	{0x30,0x92},
	{0x31,0x0A},
	{0x32,0xAA},
	{0x33,0x14},
	{0x34,0x38},
	{0x35,0x54},
	{0x42,0x41},
	{0x43,0x50},
	//Interface
	{0x1D,0x00},
	{0x1E,0x00},
	{0x6C,0x50},
	{0x73,0x33},
	{0x70,0x68},
	{0x76,0x40},
	{0x77,0x06},
	{0x78,0x18},
	//Array/AnADC/PWC
	{0x48,0x40},
	{0x60,0xA4},
	{0x61,0xFF},
	{0x62,0x40},
	{0x65,0x00},
	{0x66,0x20},
	{0x67,0x30},
	{0x68,0x04},
	{0x69,0x74},
	{0x6F,0x04},
	// for black sun
	{0x63,0x19},
	{0x6a,0x09},
	//AE/AG/ABLC
	{0x13,0x87},//87 auto ae
	{0x14,0x80},
	{0x16,0xC0},
	{0x17,0x40},
	{0x18,0xBB},
	{0x19,0x20},		//Gain delay
	{0x38,0x35},
	{0x39,0x98},
	{0x4a,0x03},
	{0x49,0x10},
#if TEST_PATTERN == 1
	{0x0C,0x41}, //Test Pattern
#else
    {0x0C,0x40},
#endif
	//INI End
	#if SENSOR_FLIP
	{0x12,0x30},
	#else
	{0x12,0x00},
	#endif
	//PWDN Setting
    //write Done
	{0xff,0xff}
};


static const int jxh42_30fps_exp_time_gain_50Hz[H42_30FPS_50HZ_EXP_TIME_TOTAL][3] =
{ // {time, analog gain, digital gain}
{8	,(int)(1.00*16), (int)(1.00*64)},
{8	,(int)(1.00*16), (int)(1.00*64)},
{9	,(int)(1.00*16), (int)(1.00*64)},
{9	,(int)(1.00*16), (int)(1.00*64)},
{9	,(int)(1.00*16), (int)(1.00*64)},
{10	,(int)(1.00*16), (int)(1.00*64)},
{10	,(int)(1.00*16), (int)(1.00*64)},
{10	,(int)(1.00*16), (int)(1.00*64)},
{11	,(int)(1.00*16), (int)(1.00*64)},
{11	,(int)(1.00*16), (int)(1.00*64)},//10
{11	,(int)(1.00*16), (int)(1.00*64)},
{12	,(int)(1.00*16), (int)(1.00*64)},
{12	,(int)(1.00*16), (int)(1.00*64)},
{13	,(int)(1.00*16), (int)(1.00*64)},
{13	,(int)(1.00*16), (int)(1.00*64)},
{14	,(int)(1.00*16), (int)(1.00*64)},
{14	,(int)(1.00*16), (int)(1.00*64)},
{15	,(int)(1.00*16), (int)(1.00*64)},
{15	,(int)(1.00*16), (int)(1.00*64)},
{16	,(int)(1.00*16), (int)(1.00*64)},//20
{16	,(int)(1.00*16), (int)(1.00*64)},
{17	,(int)(1.00*16), (int)(1.00*64)},
{17	,(int)(1.00*16), (int)(1.00*64)},
{18	,(int)(1.00*16), (int)(1.00*64)},
{19	,(int)(1.00*16), (int)(1.00*64)},
{19	,(int)(1.00*16), (int)(1.00*64)},
{20	,(int)(1.00*16), (int)(1.00*64)},
{21	,(int)(1.00*16), (int)(1.00*64)},
{21	,(int)(1.00*16), (int)(1.00*64)},
{22	,(int)(1.00*16), (int)(1.00*64)},//30
{23	,(int)(1.00*16), (int)(1.00*64)},
{24	,(int)(1.00*16), (int)(1.00*64)},
{24	,(int)(1.00*16), (int)(1.00*64)},
{25	,(int)(1.00*16), (int)(1.00*64)},
{26	,(int)(1.00*16), (int)(1.00*64)},
{27	,(int)(1.00*16), (int)(1.00*64)},
{28	,(int)(1.00*16), (int)(1.00*64)},
{29	,(int)(1.00*16), (int)(1.00*64)},
{30	,(int)(1.00*16), (int)(1.00*64)},
{31	,(int)(1.00*16), (int)(1.00*64)},//40
{32	,(int)(1.00*16), (int)(1.00*64)},
{33	,(int)(1.00*16), (int)(1.00*64)},
{35	,(int)(1.00*16), (int)(1.00*64)},
{36	,(int)(1.00*16), (int)(1.00*64)},
{37	,(int)(1.00*16), (int)(1.00*64)},
{38	,(int)(1.00*16), (int)(1.00*64)},
{40	,(int)(1.00*16), (int)(1.00*64)},
{41	,(int)(1.00*16), (int)(1.00*64)},
{43	,(int)(1.00*16), (int)(1.00*64)},
{44	,(int)(1.00*16), (int)(1.00*64)},//50
{46	,(int)(1.00*16), (int)(1.00*64)},
{47	,(int)(1.00*16), (int)(1.00*64)},
{49	,(int)(1.00*16), (int)(1.00*64)},
{51	,(int)(1.00*16), (int)(1.00*64)},
{52	,(int)(1.00*16), (int)(1.00*64)},
{54	,(int)(1.00*16), (int)(1.00*64)},
{56	,(int)(1.00*16), (int)(1.00*64)},
{58	,(int)(1.00*16), (int)(1.00*64)},
{60	,(int)(1.00*16), (int)(1.00*64)},
{62	,(int)(1.00*16), (int)(1.00*64)},//60
{65	,(int)(1.00*16), (int)(1.00*64)},
{67	,(int)(1.00*16), (int)(1.00*64)},
{69	,(int)(1.00*16), (int)(1.00*64)},
{72	,(int)(1.00*16), (int)(1.00*64)},
{74	,(int)(1.00*16), (int)(1.00*64)},
{77	,(int)(1.00*16), (int)(1.00*64)},
{80	,(int)(1.00*16), (int)(1.00*64)},
{82	,(int)(1.00*16), (int)(1.00*64)},
{85	,(int)(1.00*16), (int)(1.00*64)},
{88	,(int)(1.00*16), (int)(1.00*64)},//70
{91	,(int)(1.00*16), (int)(1.00*64)},
{95	,(int)(1.00*16), (int)(1.00*64)},
{98	,(int)(1.00*16), (int)(1.00*64)},
{101,(int)(1.00*16), (int)(1.00*64)},
{105,(int)(1.00*16), (int)(1.00*64)},
{109,(int)(1.00*16), (int)(1.00*64)},
{112,(int)(1.00*16), (int)(1.00*64)},
{116,(int)(1.00*16), (int)(1.00*64)},
{121,(int)(1.00*16), (int)(1.00*64)},
{125,(int)(1.00*16), (int)(1.00*64)},//80
{129,(int)(1.00*16), (int)(1.00*64)},
{134,(int)(1.00*16), (int)(1.00*64)},
{139,(int)(1.00*16), (int)(1.00*64)},
{143,(int)(1.00*16), (int)(1.00*64)},
{148,(int)(1.00*16), (int)(1.00*64)},
{154,(int)(1.00*16), (int)(1.00*64)},
{159,(int)(1.00*16), (int)(1.00*64)},
{165,(int)(1.00*16), (int)(1.00*64)},
{171,(int)(1.00*16), (int)(1.00*64)},
{177,(int)(1.00*16), (int)(1.00*64)},//90
{183,(int)(1.00*16), (int)(1.00*64)},
{189,(int)(1.00*16), (int)(1.00*64)},
{196,(int)(1.00*16), (int)(1.00*64)},
{203,(int)(1.00*16), (int)(1.00*64)},
{210,(int)(1.00*16), (int)(1.00*64)},
{217,(int)(1.00*16), (int)(1.00*64)},
{225,(int)(1.00*16), (int)(1.00*64)},
{225,(int)(1.04*16), (int)(1.00*64)},
{225,(int)(1.06*16), (int)(1.00*64)},
{225,(int)(1.11*16), (int)(1.00*64)},//100
{225,(int)(1.13*16), (int)(1.00*64)},
{225,(int)(1.19*16), (int)(1.00*64)},
{225,(int)(1.25*16), (int)(1.00*64)},
{225,(int)(1.27*16), (int)(1.00*64)},
{225,(int)(1.31*16), (int)(1.00*64)},
{225,(int)(1.38*16), (int)(1.00*64)},
{225,(int)(1.41*16), (int)(1.00*64)},
{225,(int)(1.44*16), (int)(1.00*64)},
{225,(int)(1.50*16), (int)(1.00*64)},
{225,(int)(1.56*16), (int)(1.00*64)},//110
{225,(int)(1.63*16), (int)(1.00*64)},
{225,(int)(1.69*16), (int)(1.00*64)},
{225,(int)(1.75*16), (int)(1.00*64)},
{225,(int)(1.81*16), (int)(1.00*64)},
{225,(int)(1.88*16), (int)(1.00*64)},
{225,(int)(1.94*16), (int)(1.00*64)},
{450,(int)(1.00*16), (int)(1.00*64)},
{450,(int)(1.04*16), (int)(1.00*64)},
{450,(int)(1.06*16), (int)(1.00*64)},
{450,(int)(1.11*16), (int)(1.00*64)},//120
{450,(int)(1.13*16), (int)(1.00*64)},
{450,(int)(1.19*16), (int)(1.00*64)},
{450,(int)(1.25*16), (int)(1.00*64)},
{450,(int)(1.27*16), (int)(1.00*64)},
{450,(int)(1.31*16), (int)(1.00*64)},
{450,(int)(1.38*16), (int)(1.00*64)},
{450,(int)(1.41*16), (int)(1.00*64)},
{450,(int)(1.44*16), (int)(1.00*64)},
{450,(int)(1.50*16), (int)(1.00*64)},
{450,(int)(1.56*16), (int)(1.00*64)},//130
{450,(int)(1.63*16), (int)(1.00*64)},
{450,(int)(1.69*16), (int)(1.00*64)},
{450,(int)(1.75*16), (int)(1.00*64)},
{450,(int)(1.81*16), (int)(1.00*64)},
{450,(int)(1.88*16), (int)(1.00*64)},
{450,(int)(1.94*16), (int)(1.00*64)},
{675,(int)(1.31*16), (int)(1.00*64)},
{675,(int)(1.38*16), (int)(1.00*64)},
{675,(int)(1.41*16), (int)(1.00*64)},
{675,(int)(1.44*16), (int)(1.00*64)},//140
{675,(int)(1.50*16), (int)(1.00*64)},
{675,(int)(1.56*16), (int)(1.00*64)},
{675,(int)(1.63*16), (int)(1.00*64)},
{675,(int)(1.69*16), (int)(1.00*64)},
{675,(int)(1.75*16), (int)(1.00*64)},
{675,(int)(1.81*16), (int)(1.00*64)},
{675,(int)(1.88*16), (int)(1.00*64)},
{675,(int)(1.94*16), (int)(1.00*64)},
{675,(int)(2.00*16), (int)(1.00*64)},
{675,(int)(2.07*16), (int)(1.00*64)},//150
{675,(int)(2.14*16), (int)(1.00*64)},
{675,(int)(2.22*16), (int)(1.00*64)},
{675,(int)(2.30*16), (int)(1.00*64)},
{675,(int)(2.38*16), (int)(1.00*64)},
{675,(int)(2.46*16), (int)(1.00*64)},
{675,(int)(2.55*16), (int)(1.00*64)},
{675,(int)(2.64*16), (int)(1.00*64)},
{675,(int)(2.73*16), (int)(1.00*64)},
{675,(int)(2.83*16), (int)(1.00*64)},
{675,(int)(2.93*16), (int)(1.00*64)},//160
{675,(int)(3.03*16), (int)(1.00*64)},
{675,(int)(3.14*16), (int)(1.00*64)},
{675,(int)(3.25*16), (int)(1.00*64)},
{675,(int)(3.36*16), (int)(1.00*64)},
{675,(int)(3.48*16), (int)(1.00*64)},
{675,(int)(3.61*16), (int)(1.00*64)},
{675,(int)(3.73*16), (int)(1.00*64)},
{675,(int)(3.86*16), (int)(1.00*64)},
{675,(int)(4.00*16), (int)(1.00*64)},
{675,(int)(4.14*16), (int)(1.00*64)},//170
{675,(int)(4.29*16), (int)(1.00*64)},
{675,(int)(4.44*16), (int)(1.00*64)},
{675,(int)(4.59*16), (int)(1.00*64)},
{675,(int)(4.76*16), (int)(1.00*64)},
{675,(int)(4.92*16), (int)(1.00*64)},
{675,(int)(5.10*16), (int)(1.00*64)},
{675,(int)(5.28*16), (int)(1.00*64)},
{675,(int)(5.46*16), (int)(1.00*64)},
{675,(int)(5.66*16), (int)(1.00*64)},
{675,(int)(5.86*16), (int)(1.00*64)},//180
{675,(int)(6.06*16), (int)(1.00*64)},
{675,(int)(6.28*16), (int)(1.00*64)},
{675,(int)(6.50*16), (int)(1.00*64)},
{675,(int)(6.73*16), (int)(1.00*64)},
{675,(int)(6.96*16), (int)(1.00*64)},
{675,(int)(7.21*16), (int)(1.00*64)},
{675,(int)(7.46*16), (int)(1.00*64)},
{675,(int)(7.73*16), (int)(1.00*64)},
{675,(int)(8.00*16), (int)(1.00*64)}, //30fps*8x
{675,(int)(8.28*16), (int)(1.00*64)},//190
{675,(int)(8.57*16), (int)(1.00*64)},
{675,(int)(8.88*16), (int)(1.00*64)},
{675,(int)(9.19*16), (int)(1.00*64)},
{675,(int)(9.51*16), (int)(1.00*64)},
{675,(int)(9.85*16), (int)(1.00*64)},
{675,(int)(10.20*16), (int)(1.00*64)},
{675,(int)(10.56*16), (int)(1.00*64)},
{675,(int)(10.93*16), (int)(1.00*64)},
{675,(int)(11.31*16), (int)(1.00*64)},
{675,(int)(11.71*16), (int)(1.00*64)},//200
{675,(int)(12.13*16), (int)(1.00*64)},
{675,(int)(12.55*16), (int)(1.00*64)},
{675,(int)(13.00*16), (int)(1.00*64)},
{675,(int)(13.45*16), (int)(1.00*64)},
{675,(int)(13.93*16), (int)(1.00*64)},
{675,(int)(14.42*16), (int)(1.00*64)},
{675,(int)(14.93*16), (int)(1.00*64)},
{675,(int)(15.45*16), (int)(1.00*64)},
{675,(int)(16.00*16), (int)(1.00*64)} //209,30fps*16x
};

static const  int jxh42_30fps_exp_time_gain_60Hz[H42_30FPS_60HZ_EXP_TIME_TOTAL][3] =
{ // {time, analog gain, digital gain}
{8	,(int)(1.00*16), (int)(1.00*64)},
{8	,(int)(1.00*16), (int)(1.00*64)},
{8	,(int)(1.00*16), (int)(1.00*64)},
{9	,(int)(1.00*16), (int)(1.00*64)},
{9	,(int)(1.00*16), (int)(1.00*64)},
{9	,(int)(1.00*16), (int)(1.00*64)},
{10	,(int)(1.00*16), (int)(1.00*64)},
{10	,(int)(1.00*16), (int)(1.00*64)},
{10	,(int)(1.00*16), (int)(1.00*64)},
{11	,(int)(1.00*16), (int)(1.00*64)}, //10
{11	,(int)(1.00*16), (int)(1.00*64)},
{11	,(int)(1.00*16), (int)(1.00*64)},
{12	,(int)(1.00*16), (int)(1.00*64)},
{12	,(int)(1.00*16), (int)(1.00*64)},
{13	,(int)(1.00*16), (int)(1.00*64)},
{13	,(int)(1.00*16), (int)(1.00*64)},
{13	,(int)(1.00*16), (int)(1.00*64)},
{14	,(int)(1.00*16), (int)(1.00*64)},
{14	,(int)(1.00*16), (int)(1.00*64)},
{15	,(int)(1.00*16), (int)(1.00*64)},//20
{16	,(int)(1.00*16), (int)(1.00*64)},
{16	,(int)(1.00*16), (int)(1.00*64)},
{17	,(int)(1.00*16), (int)(1.00*64)},
{17	,(int)(1.00*16), (int)(1.00*64)},
{18	,(int)(1.00*16), (int)(1.00*64)},
{18	,(int)(1.00*16), (int)(1.00*64)},
{19	,(int)(1.00*16), (int)(1.00*64)},
{20	,(int)(1.00*16), (int)(1.00*64)},
{20	,(int)(1.00*16), (int)(1.00*64)},
{21	,(int)(1.00*16), (int)(1.00*64)},//30
{22	,(int)(1.00*16), (int)(1.00*64)},
{23	,(int)(1.00*16), (int)(1.00*64)},
{24	,(int)(1.00*16), (int)(1.00*64)},
{24	,(int)(1.00*16), (int)(1.00*64)},
{25	,(int)(1.00*16), (int)(1.00*64)},
{26	,(int)(1.00*16), (int)(1.00*64)},
{27	,(int)(1.00*16), (int)(1.00*64)},
{28	,(int)(1.00*16), (int)(1.00*64)},
{29	,(int)(1.00*16), (int)(1.00*64)},
{30	,(int)(1.00*16), (int)(1.00*64)},//40
{31	,(int)(1.00*16), (int)(1.00*64)},
{32	,(int)(1.00*16), (int)(1.00*64)},
{33	,(int)(1.00*16), (int)(1.00*64)},
{34	,(int)(1.00*16), (int)(1.00*64)},
{36	,(int)(1.00*16), (int)(1.00*64)},
{37	,(int)(1.00*16), (int)(1.00*64)},
{38	,(int)(1.00*16), (int)(1.00*64)},
{40	,(int)(1.00*16), (int)(1.00*64)},
{41	,(int)(1.00*16), (int)(1.00*64)},
{42	,(int)(1.00*16), (int)(1.00*64)},//50
{44	,(int)(1.00*16), (int)(1.00*64)},
{45	,(int)(1.00*16), (int)(1.00*64)},
{47	,(int)(1.00*16), (int)(1.00*64)},
{49	,(int)(1.00*16), (int)(1.00*64)},
{50	,(int)(1.00*16), (int)(1.00*64)},
{52	,(int)(1.00*16), (int)(1.00*64)},
{54	,(int)(1.00*16), (int)(1.00*64)},
{56	,(int)(1.00*16), (int)(1.00*64)},
{58	,(int)(1.00*16), (int)(1.00*64)},
{60	,(int)(1.00*16), (int)(1.00*64)},//60
{62	,(int)(1.00*16), (int)(1.00*64)},
{64	,(int)(1.00*16), (int)(1.00*64)},
{66	,(int)(1.00*16), (int)(1.00*64)},
{69	,(int)(1.00*16), (int)(1.00*64)},
{71	,(int)(1.00*16), (int)(1.00*64)},
{74	,(int)(1.00*16), (int)(1.00*64)},
{76	,(int)(1.00*16), (int)(1.00*64)},
{79	,(int)(1.00*16), (int)(1.00*64)},
{82	,(int)(1.00*16), (int)(1.00*64)},
{85	,(int)(1.00*16), (int)(1.00*64)},//70
{88	,(int)(1.00*16), (int)(1.00*64)},
{91	,(int)(1.00*16), (int)(1.00*64)},
{94	,(int)(1.00*16), (int)(1.00*64)},
{97	,(int)(1.00*16), (int)(1.00*64)},
{101,(int)(1.00*16), (int)(1.00*64)},
{104,(int)(1.00*16), (int)(1.00*64)},
{108,(int)(1.00*16), (int)(1.00*64)},
{112,(int)(1.00*16), (int)(1.00*64)},
{116,(int)(1.00*16), (int)(1.00*64)},
{120,(int)(1.00*16), (int)(1.00*64)}, //80
{124,(int)(1.00*16), (int)(1.00*64)},
{128,(int)(1.00*16), (int)(1.00*64)},
{133,(int)(1.00*16), (int)(1.00*64)},
{138,(int)(1.00*16), (int)(1.00*64)},
{142,(int)(1.00*16), (int)(1.00*64)},
{148,(int)(1.00*16), (int)(1.00*64)},
{153,(int)(1.00*16), (int)(1.00*64)},
{158,(int)(1.00*16), (int)(1.00*64)},
{164,(int)(1.00*16), (int)(1.00*64)},
{169,(int)(1.00*16), (int)(1.00*64)}, //90
{175,(int)(1.00*16), (int)(1.00*64)},
{182,(int)(1.00*16), (int)(1.00*64)},
{188,(int)(1.00*16), (int)(1.00*64)},
{188,(int)(1.04*16), (int)(1.00*64)},
{188,(int)(1.06*16), (int)(1.00*64)},
{188,(int)(1.11*16), (int)(1.00*64)},
{188,(int)(1.13*16), (int)(1.00*64)},
{188,(int)(1.19*16), (int)(1.00*64)},
{188,(int)(1.25*16), (int)(1.00*64)},
{188,(int)(1.27*16), (int)(1.00*64)}, //100
{188,(int)(1.31*16), (int)(1.00*64)},
{188,(int)(1.38*16), (int)(1.00*64)},
{188,(int)(1.41*16), (int)(1.00*64)},
{188,(int)(1.44*16), (int)(1.00*64)},
{188,(int)(1.50*16), (int)(1.00*64)},
{188,(int)(1.56*16), (int)(1.00*64)},
{188,(int)(1.63*16), (int)(1.00*64)},
{188,(int)(1.69*16), (int)(1.00*64)},
{188,(int)(1.75*16), (int)(1.00*64)},
{188,(int)(1.81*16), (int)(1.00*64)}, //110
{188,(int)(1.88*16), (int)(1.00*64)},
{188,(int)(1.94*16), (int)(1.00*64)},
{375,(int)(1.00*16), (int)(1.00*64)},
{375,(int)(1.04*16), (int)(1.00*64)},
{375,(int)(1.06*16), (int)(1.00*64)},
{375,(int)(1.11*16), (int)(1.00*64)},
{375,(int)(1.13*16), (int)(1.00*64)},
{375,(int)(1.19*16), (int)(1.00*64)},
{375,(int)(1.25*16), (int)(1.00*64)},
{375,(int)(1.27*16), (int)(1.00*64)}, //120
{375,(int)(1.31*16), (int)(1.00*64)},
{375,(int)(1.38*16), (int)(1.00*64)},
{375,(int)(1.41*16), (int)(1.00*64)},
{375,(int)(1.44*16), (int)(1.00*64)},
{375,(int)(1.50*16), (int)(1.00*64)},
{375,(int)(1.56*16), (int)(1.00*64)},
{375,(int)(1.63*16), (int)(1.00*64)},
{375,(int)(1.69*16), (int)(1.00*64)},
{375,(int)(1.75*16), (int)(1.00*64)},
{375,(int)(1.81*16), (int)(1.00*64)}, //130
{375,(int)(1.88*16), (int)(1.00*64)},
{375,(int)(1.94*16), (int)(1.00*64)},
{563,(int)(1.31*16), (int)(1.00*64)},
{563,(int)(1.38*16), (int)(1.00*64)},
{563,(int)(1.41*16), (int)(1.00*64)},
{563,(int)(1.44*16), (int)(1.00*64)},
{563,(int)(1.50*16), (int)(1.00*64)},
{563,(int)(1.56*16), (int)(1.00*64)},
{563,(int)(1.63*16), (int)(1.00*64)},
{563,(int)(1.69*16), (int)(1.00*64)}, //140
{563,(int)(1.75*16), (int)(1.00*64)},
{750,(int)(1.31*16), (int)(1.00*64)},
{750,(int)(1.38*16), (int)(1.00*64)},
{750,(int)(1.41*16), (int)(1.00*64)},
{750,(int)(1.44*16), (int)(1.00*64)},
{750,(int)(1.50*16), (int)(1.00*64)},
{750,(int)(1.56*16), (int)(1.00*64)},
{750,(int)(1.63*16), (int)(1.00*64)},
{750,(int)(1.69*16), (int)(1.00*64)},
{750,(int)(1.75*16), (int)(1.00*64)}, //150
{750,(int)(1.81*16), (int)(1.00*64)},
{750,(int)(1.88*16), (int)(1.00*64)},
{750,(int)(1.94*16), (int)(1.00*64)},
{750,(int)(2.00*16), (int)(1.00*64)},
{750,(int)(2.07*16), (int)(1.00*64)},
{750,(int)(2.14*16), (int)(1.00*64)},
{750,(int)(2.22*16), (int)(1.00*64)},
{750,(int)(2.30*16), (int)(1.00*64)},
{750,(int)(2.38*16), (int)(1.00*64)},
{750,(int)(2.46*16), (int)(1.00*64)},//160
{750,(int)(2.55*16), (int)(1.00*64)},
{750,(int)(2.64*16), (int)(1.00*64)},
{750,(int)(2.73*16), (int)(1.00*64)},
{750,(int)(2.83*16), (int)(1.00*64)},
{750,(int)(2.93*16), (int)(1.00*64)},
{750,(int)(3.03*16), (int)(1.00*64)},
{750,(int)(3.14*16), (int)(1.00*64)},
{750,(int)(3.25*16), (int)(1.00*64)},
{750,(int)(3.36*16), (int)(1.00*64)},
{750,(int)(3.48*16), (int)(1.00*64)},//170
{750,(int)(3.61*16), (int)(1.00*64)},
{750,(int)(3.73*16), (int)(1.00*64)},
{750,(int)(3.86*16), (int)(1.00*64)},
{750,(int)(4.00*16), (int)(1.00*64)},
{750,(int)(4.14*16), (int)(1.00*64)},
{750,(int)(4.29*16), (int)(1.00*64)},
{750,(int)(4.44*16), (int)(1.00*64)},
{750,(int)(4.59*16), (int)(1.00*64)},
{750,(int)(4.76*16), (int)(1.00*64)},
{750,(int)(4.92*16), (int)(1.00*64)},//180
{750,(int)(5.10*16), (int)(1.00*64)},
{750,(int)(5.28*16), (int)(1.00*64)},
{750,(int)(5.46*16), (int)(1.00*64)},
{750,(int)(5.66*16), (int)(1.00*64)},
{750,(int)(5.86*16), (int)(1.00*64)},
{750,(int)(6.06*16), (int)(1.00*64)},
{750,(int)(6.28*16), (int)(1.00*64)},
{750,(int)(6.50*16), (int)(1.00*64)},
{750,(int)(6.73*16), (int)(1.00*64)},
{750,(int)(6.96*16), (int)(1.00*64)},//190
{750,(int)(7.21*16), (int)(1.00*64)},//50hz,30fps,8x
{750,(int)(7.46*16), (int)(1.00*64)},
{750,(int)(7.73*16), (int)(1.00*64)},
{750,(int)(8.00*16), (int)(1.00*64)},
{750,(int)(8.28*16), (int)(1.00*64)},
{750,(int)(8.57*16), (int)(1.00*64)},
{750,(int)(8.88*16), (int)(1.00*64)},
{750,(int)(9.19*16), (int)(1.00*64)},
{750,(int)(9.51*16), (int)(1.00*64)},
{750,(int)(9.85*16), (int)(1.00*64)},//200
{750,(int)(10.20*16), (int)(1.00*64)},
{750,(int)(10.56*16), (int)(1.00*64)},
{750,(int)(10.93*16), (int)(1.00*64)},
{750,(int)(11.31*16), (int)(1.00*64)},
{750,(int)(11.71*16), (int)(1.00*64)},
{750,(int)(12.13*16), (int)(1.00*64)},
{750,(int)(12.55*16), (int)(1.00*64)},
{750,(int)(13.00*16), (int)(1.00*64)},
{750,(int)(13.45*16), (int)(1.00*64)},
{750,(int)(13.93*16), (int)(1.00*64)},//210
{750,(int)(14.42*16), (int)(1.00*64)},//50hz,30fps,16x
{750,(int)(14.93*16), (int)(1.00*64)},
{750,(int)(15.45*16), (int)(1.00*64)},
{750,(int)(16.00*16), (int)(1.00*64)},
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTION IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////////////////////////////

static INT32S h42_sccb_open(void)
{
#if H42_I2C_MODE == SENSOR_I2C_MODE_SW
	h42_handle = drv_l2_sccb_open_ext(&h42_sccb_config);
	if(h42_handle == 0)
	{
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#elif H42_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_init(H42_I2C_DEVICE);
	h42_handle.devNumber = H42_I2C_DEVICE;
	h42_handle.slaveAddr = H42_ID;
	h42_handle.clkRate = 100;
#endif
	return STATUS_OK;
}

static void h42_sccb_close(void)
{
#if H42_I2C_MODE == SENSOR_I2C_MODE_SW
	if(h42_handle) {
		drv_l2_sccb_close(h42_handle);
		h42_handle = NULL;
	}
#elif H42_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_uninit(H42_I2C_DEVICE);
	h42_handle.slaveAddr = 0;
	h42_handle.clkRate = 0;
#endif
}

static INT32S h42_sccb_write(INT16U reg, INT8U value)
{
#if H42_I2C_MODE == SENSOR_I2C_MODE_SW
	return drv_l2_sccb_write(h42_handle, reg, value);
#elif H42_I2C_MODE == SENSOR_I2C_MODE_HW
    return drv_l1_reg_1byte_data_1byte_write(&h42_handle, reg, value);
#endif
}

#if READ_TABLE_EN == 1
static INT32S h42_sccb_read(INT16U reg, INT8U *value)
{
#if H42_I2C_MODE == SENSOR_I2C_MODE_SW
	INT16U data;

	if(drv_l2_sccb_read(h42_handle, reg, &data) >= 0) {
		*value = (INT8U)data;
		return STATUS_OK;
	} else {
		*value = 0xFF;
		return STATUS_FAIL;
	}
#elif H42_I2C_MODE == SENSOR_I2C_MODE_HW
	INT8U	value = 0;

	if(drv_l1_reg_1byte_data_1byte_read(&h42_handle, reg, &value) >=0)
	{
		DBG_PRINT("0x%02x, 0x%02x\r\n", reg, value);
		return STATUS_OK;
	}
	else
	{
		DBG_PRINT("h42_sccb_read fail...\r\n");
		return STATUS_FAIL;
	}
#endif
	return STATUS_OK;
}
#endif

static INT32S h42_sccb_write_table(regval8_t *pTable)
{
    INT8U retry_cnt=0;
	while(1) {
		if(pTable->reg_num == 0xFF && pTable->value == 0xFF) {
			break;
		}
		//DBG_PRINT("0x%02x, 0x%02x\r\n", pTable->reg_num, pTable->value);
		if(h42_sccb_write(pTable->reg_num, pTable->value) < 0) {
			DBG_PRINT("sccb write fail.\r\n");
            retry_cnt += 1;
			if(retry_cnt >= 10){
                retry_cnt = 0;
				return STATUS_FAIL;
			}
			continue;
		}
		pTable++;
	}
	return STATUS_OK;
}

#if READ_TABLE_EN == 1
static INT32S h42_sccb_read_table(regval8_t *pTable)
{
    INT8U *Rdata;
	while(1) {
		if(pTable->reg_num == 0xFF && pTable->value == 0xFF) {
			break;
		}
		if(h42_sccb_read(pTable->reg_num, Rdata) < 0) {
			DBG_PRINT("sccb read fail.\r\n");
			continue;
		}
		DBG_PRINT("0x%02x, 0x%02x\r\n", pTable->reg_num, *Rdata);
		pTable++;
	}
	return STATUS_OK;
}
#endif

/*****************************************************************************************
+++++++++++++++++AEC/AGC
*****************************************************************************************/
static INT32U GainThrTable[6] =
{
    2.00*16, 4.00*16,  8.00*16, 12.00*16, 14.00*16, 15.00*16
};

static int h42_cvt_agc_gain(int agc_gain)
{
	INT32U h42_agc_gain, i;
    INT32U pow2;

    pow2 = 0;
    i = 5;
    do {
	    if(agc_gain <= 0x1f)
           break;

	    agc_gain >>= 1;
	    pow2++;

	    i--;
    } while(i != 0);

    agc_gain -= 0x10;

    if(agc_gain < 0) agc_gain = 0;
    h42_agc_gain = (pow2 << 4) + agc_gain;

	return h42_agc_gain;
}

static int h42_set_xfps_exposure_time(sensor_exposure_t *si)
{
	unsigned char lsb_time, msb_time;
	int idx, temp, s_gain;
	INT8U 			iso_x = 1;

	//DBG_PRINT("%s:%d\n", __FUNCTION__, __LINE__);
    iso_x = (drv_l2_CdspGetIso() & 0xF);
	if (iso_x <= 0)
		iso_x = 1;

	si->sensor_ev_idx += si->ae_ev_step;
	if(si->sensor_ev_idx >= si->max_ev_idx)
        si->sensor_ev_idx = si->max_ev_idx;
	if(si->sensor_ev_idx < 0)
        si->sensor_ev_idx = 0;

	idx = si->sensor_ev_idx * 3;

	si ->time = p_expTime_table[idx];
	si ->analog_gain = (p_expTime_table[idx+1] * iso_x);
	si ->digital_gain = p_expTime_table[idx+2];
    // exposure time
	if(si ->time != pre_sensor_time)
	{
		pre_sensor_time = si->time;
		temp = si->time;

		lsb_time = (temp & 0xff);
		msb_time = (temp >> 8) & 0x00ff;
        #if Group_Write_En ==0
        h42_sccb_write(0x01, lsb_time);
		h42_sccb_write(0x02, msb_time);
		#else//group write
		h42_sccb_write(0xC0, 0x01);
		h42_sccb_write(0xC1, lsb_time);
		h42_sccb_write(0xC2, 0x02);
		h42_sccb_write(0xC3, msb_time);
		#endif
	}

    //gain
    if(si ->analog_gain != pre_sensor_a_gain)
	{
		// gain
		pre_sensor_a_gain = si->analog_gain;

		temp = h42_cvt_agc_gain(si->analog_gain);
		s_gain = temp & 0x007f;
        #if Group_Write_En ==0
		h42_sccb_write(0x00, s_gain);
		#else   //group write
		h42_sccb_write(0xC4, 0x00);
		h42_sccb_write(0xC5, s_gain);
		#endif
	}
	 #if Group_Write_En ==1 //group write
        #if SENSOR_FLIP
        h42_sccb_write(0x12, 0x38);   //group write enable
        #else
        h42_sccb_write(0x12, 0x08);   //group write enable
        #endif
    #endif

    return 0;
}


static void h42_set_ae(int ev_step)
{
    seInfo.ae_ev_step = ev_step;
    h42_set_xfps_exposure_time(&seInfo);
}

void sensor_register_ae_ctrl(INT32U *handle)
{
    *handle = (INT32U)h42_set_ae;
}

void sensor_get_ae_info(sensor_exposure_t *si)
{
    memcpy(si, &seInfo, sizeof(sensor_exposure_t));
}
void sensor_set_max_lum(int max_lum)
{
    seInfo.max_ev_idx = seInfo.total_ev_idx - (128 - max_lum);
}

/*****************************************************************************************
-------------------AEC/AGC
*****************************************************************************************/

#if MIPI_ISR_TEST == 1
static void mipi_h42_handle(INT32U mipi_num, INT32U event)
{
	if(event & MIPI_LANE0_SOT_SYNC_ERR) {
		DBG_PRINT("LANE0_SOT_SYNC_ERR\r\n");
	}

	if(event & MIPI_HD_1BIT_ERR) {
		DBG_PRINT("HD_1BIT_ERR\r\n");
	}

	if(event & MIPI_HD_NBIT_ERR) {
		DBG_PRINT("HD_NBIT_ERR\r\n");
	}

	if(event & MIPI_DATA_CRC_ERR) {
		DBG_PRINT("DATA_CRC_ERR\r\n");
	}

	if(event & MIPI_LANE1_SOT_SYNC_ERR) {
		DBG_PRINT("LANE1_SOT_SYNC_ERR\r\n");
	}

	if(event & MIPI_CCIR_SOF) {
		DBG_PRINT("CCIR_SOF\r\n");
	}

    if(event & MIPI_SOF) {
        drv_l2_cdsp_handle_vd();
		//DBG_PRINT("SOF 1\r\n");
	}

	if(event & MIPI_EOF) {
		DBG_PRINT("EOF\r\n");
	}
}
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++

void h42_mipi_set_exp_freq(INT8U freq)
{
    DBG_PRINT("\r\n ==== %s == freq:%d == \r\n", __FUNCTION__, freq);
	if(freq == 50)
	{
        seInfo.sensor_ev_idx = H42_30FPS_50HZ_INIT_EV_IDX;
        seInfo.ae_ev_step = 0;
        seInfo.daylight_ev_idx= H42_30FPS_50HZ_DAY_EV_IDX;
        seInfo.night_ev_idx= H42_30FPS_50HZ_NIGHT_EV_IDX;
        seInfo.max_ev_idx = H42_30FPS_50HZ_MAX_EV_IDX;
        seInfo.total_ev_idx = H42_30FPS_50HZ_EXP_TIME_TOTAL;

        p_expTime_table = (int *)jxh42_30fps_exp_time_gain_50Hz;
        DBG_PRINT(">>Set frequence 30fps 50Hz.\r\n");

	}
	else if(freq == 60)
	{
        seInfo.sensor_ev_idx = H42_30FPS_60HZ_INIT_EV_IDX;
        seInfo.ae_ev_step = 0;
        seInfo.daylight_ev_idx= H42_30FPS_60HZ_DAY_EV_IDX;
        seInfo.night_ev_idx= H42_30FPS_60HZ_NIGHT_EV_IDX;
        seInfo.max_ev_idx = H42_30FPS_60HZ_MAX_EV_IDX;
        seInfo.total_ev_idx = H42_30FPS_60HZ_EXP_TIME_TOTAL;

        p_expTime_table = (int *)jxh42_30fps_exp_time_gain_60Hz;

        DBG_PRINT(">>Set frequence 30fps 60Hz.\r\n");
	}

	sensor_set_max_lum(g_FavTable.max_lum);
}

void jxh42_seinfo_init(void)
{
    seInfo.sensor_ev_idx = H42_30FPS_50HZ_INIT_EV_IDX;
	seInfo.ae_ev_step = 0;
	seInfo.daylight_ev_idx= H42_30FPS_50HZ_DAY_EV_IDX;
	seInfo.night_ev_idx= H42_30FPS_50HZ_NIGHT_EV_IDX;
	seInfo.max_ev_idx = H42_30FPS_50HZ_MAX_EV_IDX;
	seInfo.total_ev_idx = H42_30FPS_50HZ_EXP_TIME_TOTAL;

	p_expTime_table = (int *)jxh42_30fps_exp_time_gain_50Hz;

	pre_sensor_time = 1;
	pre_sensor_a_gain = 0xff;

	seInfo.time = 1;
	seInfo.analog_gain = 0x00;
	seInfo.digital_gain = 0x40;
}

static void h42_cdsp_mipi_init(void)
{
    INT8S ret = 0;

	//reguest sccb
	h42_sccb_open();
    //ae init
	jxh42_seinfo_init();
	h42_mipi_set_exp_freq(50);
	// cdsp init
	ret = drv_l2_cdsp_open();
	if(ret < 0)
		DBG_PRINT("H42 CDSP init fail !\r\n");

	// Reset
#ifdef FRONT_SENSOR_RESET
	gpio_set_port_attribute(FRONT_SENSOR_RESET, ATTRIBUTE_HIGH);
	gpio_init_io(FRONT_SENSOR_RESET, GPIO_OUTPUT);
	gpio_write_io(FRONT_SENSOR_RESET, DATA_HIGH);
	osDelay(10);
	gpio_write_io(FRONT_SENSOR_RESET, DATA_LOW);
	osDelay(20);
	gpio_write_io(FRONT_SENSOR_RESET, DATA_HIGH);
	osDelay(10);
#endif

	// mipi enable
	drv_l1_mipi_init(MIPI_DEV_NO, ENABLE, 0);

    DBG_PRINT("Sensor H42 sensor mipi init completed\r\n");
}

/**
 * @brief   un-initialization function
 * @param   sensor format parameters
 * @return 	none
 */
static void h42_cdsp_mipi_uninit(void)
{
	DBG_PRINT("%s\r\n", __func__);
	// cdsp disable
	drv_l2_cdsp_close();

	// mipi disable
	drv_l1_mipi_init(MIPI_DEV_NO, DISABLE, 0);

	// release sccb
	h42_sccb_close();

	// disable mclk
	drv_l2_sensor_set_mclkout(MCLK_NONE, 0);
}

/**
 * @brief   stream start function
 * @param   info index
 *
 * @return 	none
 */
void h42_cdsp_mipi_stream_on(INT32U index, INT32U bufA, INT32U bufB)
{
	INT16U target_w, target_h, sensor_w, sensor_h;
	gpCdspFmt_t format;

	INT8U docropflag = 0;

	// set sensor size
	DBG_PRINT("%s = %d\r\n", __func__, index);

    //Enabel mipi clk, set mipi clk
       if(h42_mipi_cfg.pixel_clk_sel == GENERATE_PIXEL_CLK)
    {
        drv_l2_mipi_ctrl_set_clk(ENABLE, 4);
        osDelay(10);
    }

    // set cdsp format
    if(MIPI_DEV_NO == MIPI_0)
    {
		format.image_source = C_CDSP_MIPI;
    }
	else
	{
		format.image_source = C_CDSP_MIPI1;
	}
	format.input_format =  h42_cdsp_mipi_ops.info[index].input_format;
	format.output_format = h42_cdsp_mipi_ops.info[index].output_format;
	target_w = h42_cdsp_mipi_ops.info[index].target_w;
	target_h = h42_cdsp_mipi_ops.info[index].target_h;
	format.hpixel = sensor_w = h42_cdsp_mipi_ops.info[index].sensor_w;
	format.vline = sensor_h = h42_cdsp_mipi_ops.info[index].sensor_h;
	format.hoffset = h42_cdsp_mipi_ops.info[index].hoffset;
	format.voffset = h42_cdsp_mipi_ops.info[index].voffset;
	format.sensor_timing_mode = h42_cdsp_mipi_ops.info[index].interface_mode;
	format.sensor_hsync_mode = h42_cdsp_mipi_ops.info[index].hsync_active;
	format.sensor_vsync_mode = h42_cdsp_mipi_ops.info[index].vsync_active;

    if(drv_l2_cdsp_set_fmt(&format, 0) < 0)
    {
		DBG_PRINT("cdsp set fmt err!!!\r\n");
	}
    drv_l1_CdspSetDataSource(SDRAM_INPUT);
	// set scale down
	if(((sensor_w > target_w) || (sensor_h > target_h)) && (docropflag == 0))
	{
		drv_l2_cdsp_set_yuv_scale(target_w, target_h);
	}
	else if(((sensor_w > target_w) || (sensor_h > target_h)) && (docropflag == 1))
	{
        INT32U point_x = 16, point_y = 16;
		drv_l1_CdspSetCrop(point_x, point_y, target_w, target_h);
		drv_l1_CdspSetCropEn(docropflag);	//1:Enable, 0:Disable
	}

    // cdsp start
	drv_l2_CdspTableRegister((gpCisCali_t*)&g_cali, 0);
    drv_l2_cdsp_stream_on(ENABLE, bufA, bufB, 0);
	drv_l2_cdsp_enable((gpUserFav_t *)&g_FavTable, sensor_w, sensor_h, target_w, target_h, 0);
	drv_l2_cdsp_analog_gain_thr_init(GainThrTable);

	//mipi start
    #if (MIPI_DEV_NO == MIPI_1_LANE)
      if (drv_l1_mipi_set_parameter(MIPI_1, &h42_mipi_cfg) < 0)
      {
        DBG_PRINT("MIPI1 init fail !!!\r\n");
      }
      #if MIPI_ISR_TEST == 1
      drv_l1_mipi_isr_register(mipi_h42_handle);
      drv_l1_mipi_set_irq_enable(MIPI_1, ENABLE, MIPI_SOF);
      #endif
    #else
      if (drv_l1_mipi_set_parameter(MIPI_0, &h42_mipi_cfg) < 0)
      {
        DBG_PRINT("MIPI0 init fail !!!\r\n");
      }
      else
      {
        DBG_PRINT("MIPI0 init completed.\r\n");
          #if MIPI_ISR_TEST == 1
          drv_l1_mipi_isr_register(mipi_h42_handle);
          drv_l1_mipi_set_irq_enable(MIPI_0, ENABLE, MIPI_SOF);
          #endif
      }
    #endif

	// init sensor
    h42_sccb_write_table((regval8_t *)h42_init_table);

    // reset sensor ev idx
    seInfo.ae_ev_step = 0;
    h42_set_xfps_exposure_time(&seInfo);
	drv_l1_CdspSetDataSource(MIPI_INPUT);

	DBG_PRINT("H42 cdsp mipi init done!!\r\n");
}

/**
 * @brief   stream stop function
 * @param   none
 * @return 	none
 */
static void h42_cdsp_mipi_stream_off(void)
{
	//drv_l2_cdsp_stream_off();
}

/**
 * @brief   get info function
 * @param   none
 * @return 	pointer to sensor information data
 */
static drv_l2_sensor_info_t* h42_cdsp_mipi_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1)) {
		return NULL;
	} else {
		return (drv_l2_sensor_info_t*)&h42_cdsp_mipi_ops.info[index];
	}
}

/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t h42_cdsp_mipi_ops =
{
	SOURCE_CDSP_MIPI,		    /* sensor name */
	h42_cdsp_mipi_init,
	h42_cdsp_mipi_uninit,
	h42_cdsp_mipi_stream_on,
	h42_cdsp_mipi_stream_off,
	h42_cdsp_mipi_get_info,
	h42_set_ae,
	sensor_get_ae_info,
    sensor_set_max_lum,
    h42_mipi_set_exp_freq,
	{
		/* 1st info */
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SBGGR10,		/* input format */
			V4L2_PIX_FMT_YUYV,			/* output format */
			30,							/* FPS in sensor */
			H42_OUT_WIDTH,				/* target width */
			H42_OUT_HEIGHT,				/* target height */
			H42_WIDTH,				    /* sensor width */
			H42_HEIGHT,  				/* sensor height */
			4,							/* sensor h offset */
			0,							/* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
		/* 2nd info */
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SBGGR10,       /* input format */
			V4L2_PIX_FMT_YUYV,			/* output format */
			30,							/* FPS in sensor */
			H42_OUT_WIDTH,              /* target width */
			H42_OUT_HEIGHT,             /* target height */
			H42_WIDTH,				    /* sensor width */
			H42_HEIGHT, 				/* sensor height */
			2,							/* sensor h offset */
			0,							/* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
	}
};
#endif //(defined _SENSOR_H42_CDSP_MIPI) && (_SENSOR_H42_CDSP_MIPI == 1)

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

#if (defined _SENSOR_H62_CDSP_MIPI) && (_SENSOR_H62_CDSP_MIPI == 1)

#include "drv_l2_user_calibration.h"
#include "drv_l2_user_preference.h"
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define SOI_GROUP_WRITE					1			//0:non group write,1:group write, 2:test
#define MIPI_ISR_TEST                   0
#define MIPI_LANE_NO					1           // 1 or 2 lane
#define MIPI_DEV_NO                     0   //0:MIPI_0(IOC8~11) or 1:MIPI_1(IOE0~9)

#define SENSOR_FLIP						0
#define SENSOR_MIRROR					0

#define	H62_ID							0x60		//0x64,0x68,0x6C

#define H62_WIDTH						1280
#define H62_HEIGHT						720
#define H62_OUT_WIDTH					1280
#define H62_OUT_HEIGHT					720

#define H62_I2C_MODE				SENSOR_I2C_MODE_HW

#if H62_I2C_MODE == SENSOR_I2C_MODE_SW
#define H62_GPIO_SCL			IO_B4
#define H62_GPIO_SDA			IO_B5
#define H62_GPIO_DRIVING		IOB_DRV_30mA
#else
#define H62_I2C_DEVICE			I2C_0
#endif

#define H62_30FPS_50HZ_DAY_EV_IDX 			103//107
#define H62_30FPS_50HZ_NIGHT_EV_IDX			162//166
#define H62_30FPS_50HZ_EXP_TIME_TOTAL		(227+20+10)//224
#define H62_30FPS_50HZ_INIT_EV_IDX 			((H62_30FPS_50HZ_DAY_EV_IDX + H62_30FPS_50HZ_NIGHT_EV_IDX) >> 1)
#define H62_30FPS_50HZ_MAX_EV_IDX			(H62_30FPS_50HZ_EXP_TIME_TOTAL - 10)

#define H62_30FPS_60HZ_DAY_EV_IDX 			100//107
#define H62_30FPS_60HZ_NIGHT_EV_IDX			160//157
#define H62_30FPS_60HZ_EXP_TIME_TOTAL		(225+40)//215
#define H62_30FPS_60HZ_INIT_EV_IDX 			((H62_30FPS_60HZ_DAY_EV_IDX + H62_30FPS_60HZ_NIGHT_EV_IDX) >> 1)
#define H62_30FPS_60HZ_MAX_EV_IDX			 (H62_30FPS_60HZ_EXP_TIME_TOTAL - 8)

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

#if H62_I2C_MODE == SENSOR_I2C_MODE_SW
sccb_config_t h62_sccb_config =
{
	H62_GPIO_SCL,				//scl_port
	H62_GPIO_DRIVING,			//scl_drv
	H62_GPIO_SDA,				//sda_port
	H62_GPIO_DRIVING,			//sda_drv
	IO_F15,						//pwdn_port
	0,							//pwdn_drv
	0,							//have_pwdn
	8,							//RegBits
	8,							//DataBits
	H62_ID,						//slaveAddr
	0x20000,					//timeout
	100							//clock_rate
};
static void *h62_handle;
#elif H62_I2C_MODE == SENSOR_I2C_MODE_HW
static drv_l1_i2c_bus_handle_t h62_handle;
#endif

static mipi_config_t h62_mipi_cfg =
{
	DISABLE, 			/* low power, 0:disable, 1:enable */
	D_PHY_SAMPLE_POS,	/* byte clock edge, 0:posedge, 1:negedge */
#if MIPI_LANE_NO == 1
	MIPI_1_LANE,		/* lane */
	D_PHY_BYTE_CLK,		/* byte clock source */
#elif MIPI_LANE_NO == 2
	MIPI_2_LANE,		/* lane */
	GENERATE_PIXEL_CLK,	/* pixel clock source */
#endif
	MIPI_AUTO_DETECT,	/* data mode, 0:auto detect, 1:user define */
	MIPI_RAW10,			/* data type, valid when data mode is 1*/
	MIPI_DATA_TO_CDSP,	/* data type, 1:data[7:0]+2':00 to cdsp, 0: 2'00+data[7:0] to csi */
	0,//NULL,				/* RSD 2 */

	H62_WIDTH,		/* width, 0~0xFFFF */
	H62_HEIGHT,		/* height, 0~0xFFFF */
#if SENSOR_FLIP
	0,
	0,
#else
	2,//4,//2, 					/* back porch, 0~0xF */ //If Set 4 have show paper color at left side.u2.20160119
	0,//2,					/* front porch, 0~0xF */
#endif
	ENABLE,				/* blanking_line_en, 0:disable, 1:enable */
	0,//NULL,				/* RSD 3 */

	ENABLE,				/* ecc, 0:disable, 1:enable */
	MIPI_ECC_ORDER3,	/* ecc order */
	120,//95,//range:20~220,				/* data mask, unit is ns */
	MIPI_CHECK_HS_SEQ//MIPI_CHECK_LP_00//	/* check hs sequence or LP00 for clock lane */
};

static const regval8_t h62_reset_table[] =
{
	{0x12, 0x80},
	{0xFF, 0xFF},
};

static const regval8_t h62_init_table[] =
{
 //2248, D_PHY_SAMPLE_POS
	{0x12, 0x40},
	{0x0E, 0x11},
	{0x0F, 0x09},

	{0x10, 0x24},
	{0x11, 0x80},
	{0x19, 0x68},

	//25fps
	{0x20, 0x80},
	{0x21, 0x07},
	{0x22, 0x84},
	{0x23, 0x03},

	//win out size
	{0x24, 0x04},
	{0x25, 0xD4},
	{0x26, 0x25},

	 //start offset
#if SENSOR_FLIP
#if SENSOR_MIRROR
	{0x27, 0x9E},
	{0x28, 0x16},
#else
	{0x27, 0x9F},
	{0x28, 0x16},
#endif
#else
#if SENSOR_MIRROR
	{0x27, 0x9F},
	{0x28, 0x15},
#else
	{0x27, 0x94},
	{0x28, 0x15},
#endif
#endif

	{0x29, 0x02},
	{0x2A, 0x43},
	{0x2B, 0x21},
	{0x2C, 0x08},
	{0x2D, 0x01},
	{0x2E, 0xBB},
	{0x2F, 0xC0},
	{0x41, 0x88},
	{0x42, 0x12},
	{0x39, 0x90},
	{0x1D, 0x00},

	{0x1E, 0x04},

	{0x7A, 0x4C},

	{0x70, 0x89},

	{0x71, 0x4A},
	{0x72, 0x48},
	{0x73, 0x43},
	{0x74, 0x52},
	{0x75, 0x2B},
	{0x76, 0x4a},
	{0x77, 0x06},
	{0x78, 0x20},
	{0x66, 0x38},
	{0x30, 0x98},
	{0x31, 0x0E},
	{0x32, 0xF0},
	{0x33, 0x0E},
	{0x34, 0x2C},
	{0x35, 0xA3},
	{0x38, 0x40},
	{0x3A, 0x08},
	{0x56, 0x02},
	{0x60, 0x01},
	{0x0D, 0x50},
	{0x57, 0x80},
	{0x58, 0x33},
	{0x5A, 0x04},
	{0x5B, 0xB6},
	{0x5C, 0x08},
	{0x5D, 0x67},
	{0x5E, 0x04},
	{0x5F, 0x08},
	{0x66, 0x28},
	{0x67, 0xF8},
	{0x68, 0x00},
	{0x69, 0x74},
	{0x6A, 0x1F},
	{0x63, 0x80},
	{0x6C, 0xC0},
	{0x6E, 0x5C},
	{0x82, 0x01},
	{0x0C, 0x00},
	{0x46, 0xC2},
	{0x48, 0x7E},
	{0x62, 0x40},
	{0x7D, 0x57},
	{0x7E, 0x28},
	{0x80, 0x00},
	{0x4A, 0x05},
	//{0x49,0x10},
	{0x49, 0x08},
	{0x13, 0x81},
	{0x59, 0x97},
	{0x1f, 0x0},

#if SENSOR_FLIP
#if SENSOR_MIRROR
	{0x12, 0x10},
#else
	{0x12, 0x30},
#endif
#else
#if SENSOR_MIRROR
	{0x12, 0x20},
#else
	{0x12, 0x00},
#endif
#endif

	{0x47, 0x47},
	 //sleep 500
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},

	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},

	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},
	{0x0B, 0x62},

	{0x47, 0x44},
	{0x1F, 0x01},

	{0xFF, 0xFF}
};



static const int jxh62_mipi_30fps_exp_time_gain_50Hz[H62_30FPS_50HZ_EXP_TIME_TOTAL][3] =
{ // {time, analog gain, digital gain}

	{8, (int)(1.00 * 16), (int)(1.00 * 64)},
	{8, (int)(1.00 * 16), (int)(1.00 * 64)},
	{9, (int)(1.00 * 16), (int)(1.00 * 64)},
	{9, (int)(1.00 * 16), (int)(1.00 * 64)},
	{9, (int)(1.00 * 16), (int)(1.00 * 64)},
	{10, (int)(1.00 * 16), (int)(1.00 * 64)},
	{10, (int)(1.00 * 16), (int)(1.00 * 64)},
	{10, (int)(1.00 * 16), (int)(1.00 * 64)},
	{11, (int)(1.00 * 16), (int)(1.00 * 64)},
	{11, (int)(1.00 * 16), (int)(1.00 * 64)},
	{11, (int)(1.00 * 16), (int)(1.00 * 64)},
	{12, (int)(1.00 * 16), (int)(1.00 * 64)},
	{12, (int)(1.00 * 16), (int)(1.00 * 64)},
	{13, (int)(1.00 * 16), (int)(1.00 * 64)},
	{13, (int)(1.00 * 16), (int)(1.00 * 64)},
	{14, (int)(1.00 * 16), (int)(1.00 * 64)},
	{14, (int)(1.00 * 16), (int)(1.00 * 64)},
	{15, (int)(1.00 * 16), (int)(1.00 * 64)},
	{15, (int)(1.00 * 16), (int)(1.00 * 64)},
	{16, (int)(1.00 * 16), (int)(1.00 * 64)},
	{16, (int)(1.00 * 16), (int)(1.00 * 64)},
	{17, (int)(1.00 * 16), (int)(1.00 * 64)},
	{17, (int)(1.00 * 16), (int)(1.00 * 64)},
	{18, (int)(1.00 * 16), (int)(1.00 * 64)},
	{19, (int)(1.00 * 16), (int)(1.00 * 64)},
	{19, (int)(1.00 * 16), (int)(1.00 * 64)},
	{20, (int)(1.00 * 16), (int)(1.00 * 64)},
	{21, (int)(1.00 * 16), (int)(1.00 * 64)},
	{21, (int)(1.00 * 16), (int)(1.00 * 64)},
	{22, (int)(1.00 * 16), (int)(1.00 * 64)},
	{23, (int)(1.00 * 16), (int)(1.00 * 64)},
	{24, (int)(1.00 * 16), (int)(1.00 * 64)},
	{24, (int)(1.00 * 16), (int)(1.00 * 64)},
	{25, (int)(1.00 * 16), (int)(1.00 * 64)},
	{26, (int)(1.00 * 16), (int)(1.00 * 64)},
	{27, (int)(1.00 * 16), (int)(1.00 * 64)},
	{28, (int)(1.00 * 16), (int)(1.00 * 64)},
	{29, (int)(1.00 * 16), (int)(1.00 * 64)},
	{30, (int)(1.00 * 16), (int)(1.00 * 64)},
	{31, (int)(1.00 * 16), (int)(1.00 * 64)},
	{32, (int)(1.00 * 16), (int)(1.00 * 64)},
	{33, (int)(1.00 * 16), (int)(1.00 * 64)},
	{35, (int)(1.00 * 16), (int)(1.00 * 64)},
	{36, (int)(1.00 * 16), (int)(1.00 * 64)},
	{37, (int)(1.00 * 16), (int)(1.00 * 64)},
	{38, (int)(1.00 * 16), (int)(1.00 * 64)},
	{40, (int)(1.00 * 16), (int)(1.00 * 64)},
	{41, (int)(1.00 * 16), (int)(1.00 * 64)},
	{43, (int)(1.00 * 16), (int)(1.00 * 64)},
	{44, (int)(1.00 * 16), (int)(1.00 * 64)},
	{46, (int)(1.00 * 16), (int)(1.00 * 64)},
	{47, (int)(1.00 * 16), (int)(1.00 * 64)},
	{49, (int)(1.00 * 16), (int)(1.00 * 64)},
	{51, (int)(1.00 * 16), (int)(1.00 * 64)},
	{52, (int)(1.00 * 16), (int)(1.00 * 64)},
	{54, (int)(1.00 * 16), (int)(1.00 * 64)},
	{56, (int)(1.00 * 16), (int)(1.00 * 64)},
	{58, (int)(1.00 * 16), (int)(1.00 * 64)},
	{60, (int)(1.00 * 16), (int)(1.00 * 64)},
	{62, (int)(1.00 * 16), (int)(1.00 * 64)},
	{65, (int)(1.00 * 16), (int)(1.00 * 64)},
	{67, (int)(1.00 * 16), (int)(1.00 * 64)},
	{69, (int)(1.00 * 16), (int)(1.00 * 64)},
	{72, (int)(1.00 * 16), (int)(1.00 * 64)},
	{74, (int)(1.00 * 16), (int)(1.00 * 64)},
	{77, (int)(1.00 * 16), (int)(1.00 * 64)},
	{80, (int)(1.00 * 16), (int)(1.00 * 64)},
	{82, (int)(1.00 * 16), (int)(1.00 * 64)},
	{85, (int)(1.00 * 16), (int)(1.00 * 64)},
	{88, (int)(1.00 * 16), (int)(1.00 * 64)},
	{91, (int)(1.00 * 16), (int)(1.00 * 64)},
	{95, (int)(1.00 * 16), (int)(1.00 * 64)},
	{98, (int)(1.00 * 16), (int)(1.00 * 64)},
	{101, (int)(1.00 * 16), (int)(1.00 * 64)},
	{105, (int)(1.00 * 16), (int)(1.00 * 64)},
	{109, (int)(1.00 * 16), (int)(1.00 * 64)},
	{112, (int)(1.00 * 16), (int)(1.00 * 64)},
	{116, (int)(1.00 * 16), (int)(1.00 * 64)},
	{121, (int)(1.00 * 16), (int)(1.00 * 64)},
	{125, (int)(1.00 * 16), (int)(1.00 * 64)},
	{129, (int)(1.00 * 16), (int)(1.00 * 64)},
	{134, (int)(1.00 * 16), (int)(1.00 * 64)},
	{139, (int)(1.00 * 16), (int)(1.00 * 64)},
	{143, (int)(1.00 * 16), (int)(1.00 * 64)},
	{148, (int)(1.00 * 16), (int)(1.00 * 64)},
	{154, (int)(1.00 * 16), (int)(1.00 * 64)},
	{159, (int)(1.00 * 16), (int)(1.00 * 64)},
	{165, (int)(1.00 * 16), (int)(1.00 * 64)},
	{171, (int)(1.00 * 16), (int)(1.00 * 64)},
	{177, (int)(1.00 * 16), (int)(1.00 * 64)},
	{183, (int)(1.00 * 16), (int)(1.00 * 64)},
	{189, (int)(1.00 * 16), (int)(1.00 * 64)},
	{196, (int)(1.00 * 16), (int)(1.00 * 64)},
	{203, (int)(1.00 * 16), (int)(1.00 * 64)},
	{210, (int)(1.00 * 16), (int)(1.00 * 64)},
	{217, (int)(1.00 * 16), (int)(1.00 * 64)},
	{225, (int)(1.00 * 16), (int)(1.00 * 64)},
	{225, (int)(1.04 * 16), (int)(1.00 * 64)},
	{225, (int)(1.07 * 16), (int)(1.00 * 64)},
	{225, (int)(1.11 * 16), (int)(1.00 * 64)},
	{225, (int)(1.15 * 16), (int)(1.00 * 64)},
	{225, (int)(1.19 * 16), (int)(1.00 * 64)},
	{225, (int)(1.23 * 16), (int)(1.00 * 64)},
	{225, (int)(1.27 * 16), (int)(1.00 * 64)},
	{225, (int)(1.32 * 16), (int)(1.00 * 64)},
	{225, (int)(1.37 * 16), (int)(1.00 * 64)},
	{225, (int)(1.41 * 16), (int)(1.00 * 64)},
	{225, (int)(1.46 * 16), (int)(1.00 * 64)},
	{225, (int)(1.52 * 16), (int)(1.00 * 64)},
	{225, (int)(1.57 * 16), (int)(1.00 * 64)},
	{225, (int)(1.62 * 16), (int)(1.00 * 64)},
	{225, (int)(1.68 * 16), (int)(1.00 * 64)},
	{225, (int)(1.74 * 16), (int)(1.00 * 64)},
	{225, (int)(1.80 * 16), (int)(1.00 * 64)},
	{225, (int)(1.87 * 16), (int)(1.00 * 64)},
	{225, (int)(1.93 * 16), (int)(1.00 * 64)},
	{450, (int)(1.00 * 16), (int)(1.00 * 64)},
	{450, (int)(1.04 * 16), (int)(1.00 * 64)},
	{450, (int)(1.07 * 16), (int)(1.00 * 64)},
	{450, (int)(1.11 * 16), (int)(1.00 * 64)},
	{450, (int)(1.15 * 16), (int)(1.00 * 64)},
	{450, (int)(1.19 * 16), (int)(1.00 * 64)},
	{450, (int)(1.23 * 16), (int)(1.00 * 64)},
	{450, (int)(1.27 * 16), (int)(1.00 * 64)},
	{450, (int)(1.32 * 16), (int)(1.00 * 64)},
	{450, (int)(1.37 * 16), (int)(1.00 * 64)},
	{450, (int)(1.41 * 16), (int)(1.00 * 64)},
	{450, (int)(1.46 * 16), (int)(1.00 * 64)},
	{450, (int)(1.52 * 16), (int)(1.00 * 64)},
	{450, (int)(1.57 * 16), (int)(1.00 * 64)},
	{450, (int)(1.62 * 16), (int)(1.00 * 64)},
	{450, (int)(1.68 * 16), (int)(1.00 * 64)},
	{450, (int)(1.74 * 16), (int)(1.00 * 64)},
	{450, (int)(1.80 * 16), (int)(1.00 * 64)},
	{450, (int)(1.87 * 16), (int)(1.00 * 64)},
	{450, (int)(1.93 * 16), (int)(1.00 * 64)},
	{675, (int)(1.32 * 16), (int)(1.00 * 64)},
	{675, (int)(1.37 * 16), (int)(1.00 * 64)},
	{675, (int)(1.41 * 16), (int)(1.00 * 64)},
	{675, (int)(1.46 * 16), (int)(1.00 * 64)},
	{675, (int)(1.52 * 16), (int)(1.00 * 64)},
	{675, (int)(1.57 * 16), (int)(1.00 * 64)},
	{675, (int)(1.62 * 16), (int)(1.00 * 64)},
	{675, (int)(1.68 * 16), (int)(1.00 * 64)},
	{675, (int)(1.74 * 16), (int)(1.00 * 64)},
	{675, (int)(1.80 * 16), (int)(1.00 * 64)},
	{675, (int)(1.87 * 16), (int)(1.00 * 64)},
	{675, (int)(1.93 * 16), (int)(1.00 * 64)},
	{900, (int)(1.52 * 16), (int)(1.00 * 64)},
	{900, (int)(1.57 * 16), (int)(1.00 * 64)},
	{900, (int)(1.62 * 16), (int)(1.00 * 64)},
	{900, (int)(1.68 * 16), (int)(1.00 * 64)},
	{900, (int)(1.74 * 16), (int)(1.00 * 64)},
	{900, (int)(1.80 * 16), (int)(1.00 * 64)},
	{900, (int)(1.87 * 16), (int)(1.00 * 64)},
	{900, (int)(1.93 * 16), (int)(1.00 * 64)},
	{900, (int)(2.00 * 16), (int)(1.00 * 64)},
	{900, (int)(2.07 * 16), (int)(1.00 * 64)},
	{900, (int)(2.14 * 16), (int)(1.00 * 64)},
	{900, (int)(2.22 * 16), (int)(1.00 * 64)},
	{900, (int)(2.30 * 16), (int)(1.00 * 64)},
	{900, (int)(2.38 * 16), (int)(1.00 * 64)},
	{900, (int)(2.46 * 16), (int)(1.00 * 64)},
	{900, (int)(2.55 * 16), (int)(1.00 * 64)},
	{900, (int)(2.64 * 16), (int)(1.00 * 64)},
	{900, (int)(2.73 * 16), (int)(1.00 * 64)},
	{900, (int)(2.83 * 16), (int)(1.00 * 64)},
	{900, (int)(2.93 * 16), (int)(1.00 * 64)},
	{900, (int)(3.03 * 16), (int)(1.00 * 64)},
	{900, (int)(3.14 * 16), (int)(1.00 * 64)},
	{900, (int)(3.25 * 16), (int)(1.00 * 64)},
	{900, (int)(3.36 * 16), (int)(1.00 * 64)},
	{900, (int)(3.48 * 16), (int)(1.00 * 64)},
	{900, (int)(3.61 * 16), (int)(1.00 * 64)},
	{900, (int)(3.73 * 16), (int)(1.00 * 64)},
	{900, (int)(3.86 * 16), (int)(1.00 * 64)},
	{900, (int)(4.00 * 16), (int)(1.00 * 64)},
	{900, (int)(4.16 * 16), (int)(1.00 * 64)},
	{900, (int)(4.28 * 16), (int)(1.00 * 64)},
	{900, (int)(4.44 * 16), (int)(1.00 * 64)},
	{900, (int)(4.60 * 16), (int)(1.00 * 64)},
	{900, (int)(4.76 * 16), (int)(1.00 * 64)},
	{900, (int)(4.92 * 16), (int)(1.00 * 64)},
	{900, (int)(5.12 * 16), (int)(1.00 * 64)},
	{900, (int)(5.28 * 16), (int)(1.00 * 64)},
	{900, (int)(5.48 * 16), (int)(1.00 * 64)},
	{900, (int)(5.68 * 16), (int)(1.00 * 64)},
	{900, (int)(5.88 * 16), (int)(1.00 * 64)},
	{900, (int)(6.08 * 16), (int)(1.00 * 64)},
	{900, (int)(6.28 * 16), (int)(1.00 * 64)},
	{900, (int)(6.52 * 16), (int)(1.00 * 64)},
	{900, (int)(6.72 * 16), (int)(1.00 * 64)},
	{900, (int)(6.96 * 16), (int)(1.00 * 64)},
	{900, (int)(7.20 * 16), (int)(1.00 * 64)},
	{900, (int)(7.48 * 16), (int)(1.00 * 64)},
	{900, (int)(7.72 * 16), (int)(1.00 * 64)},
	{900, (int)(8.00 * 16), (int)(1.00 * 64)},
	{900, (int)(8.28 * 16), (int)(1.00 * 64)},
	{900, (int)(8.56 * 16), (int)(1.00 * 64)},
	{900, (int)(8.88 * 16), (int)(1.00 * 64)},
	{900, (int)(9.20 * 16), (int)(1.00 * 64)},
	{900, (int)(9.52 * 16), (int)(1.00 * 64)},
	{900, (int)(9.84 * 16), (int)(1.00 * 64)},
	{900, (int)(10.2 * 16), (int)(1.00 * 64)},
	{900, (int)(10.56 * 16), (int)(1.00 * 64)},
	{900, (int)(10.92 * 16), (int)(1.00 * 64)},
	{900, (int)(11.32 * 16), (int)(1.00 * 64)},
	{900, (int)(11.72 * 16), (int)(1.00 * 64)},
	{900, (int)(12.12 * 16), (int)(1.00 * 64)},
	{900, (int)(12.56 * 16), (int)(1.00 * 64)},
	{900, (int)(13.00 * 16), (int)(1.00 * 64)},
	{900, (int)(13.44 * 16), (int)(1.00 * 64)},
	{900, (int)(13.92 * 16), (int)(1.00 * 64)},
	{900, (int)(14.44 * 16), (int)(1.00 * 64)},
	{900, (int)(14.92 * 16), (int)(1.00 * 64)},
	{900, (int)(15.44 * 16), (int)(1.00 * 64)},
	{900, (int)(16.00 * 16), (int)(1.00 * 64)},//218

	{900, (int)(16.56 * 16), (int)(1.00 * 64)},
	{900, (int)(17.15 * 16), (int)(1.00 * 64)},
	{900, (int)(17.75 * 16), (int)(1.00 * 64)},
	{900, (int)(18.83 * 16), (int)(1.00 * 64)},
	{900, (int)(19.03 * 16), (int)(1.00 * 64)},
	{900, (int)(19.70 * 16), (int)(1.00 * 64)},
	{900, (int)(20.39 * 16), (int)(1.00 * 64)},
	{900, (int)(21.11 * 16), (int)(1.00 * 64)},
	{900, (int)(21.86 * 16), (int)(1.00 * 64)},
	{900, (int)(22.63 * 16), (int)(1.00 * 64)},
	{900, (int)(23.43 * 16), (int)(1.00 * 64)},
	{900, (int)(24.25 * 16), (int)(1.00 * 64)},
	{900, (int)(25.11 * 16), (int)(1.00 * 64)},
	{900, (int)(25.99 * 16), (int)(1.00 * 64)},
	{900, (int)(26.91 * 16), (int)(1.00 * 64)},
	{900, (int)(27.86 * 16), (int)(1.00 * 64)},
	{900, (int)(28.84 * 16), (int)(1.00 * 64)},
	{900, (int)(29.86 * 16), (int)(1.00 * 64)},
	{900, (int)(30.91 * 16), (int)(1.00 * 64)},
	{900, (int)(32.00 * 16), (int)(1.00 * 64)},//+20=238

	{900, (int)(33.128 * 16), (int)(1.00 * 64)},
	{900, (int)(34.296 * 16), (int)(1.00 * 64)},
	{900, (int)(35.506 * 16), (int)(1.00 * 64)},
	{900, (int)(36.758 * 16), (int)(1.00 * 64)},
	{900, (int)(38.054 * 16), (int)(1.00 * 64)},
	{900, (int)(39.396 * 16), (int)(1.00 * 64)},
	{900, (int)(40.785 * 16), (int)(1.00 * 64)},
	{900, (int)(42.224 * 16), (int)(1.00 * 64)},
	{900, (int)(43.713 * 16), (int)(1.00 * 64)},
	{900, (int)(45.254 * 16), (int)(1.00 * 64)},//+10
	{900, (int)(46.850 * 16), (int)(1.00 * 64)},
	{900, (int)(48.502 * 16), (int)(1.00 * 64)},
	{900, (int)(50.213 * 16), (int)(1.00 * 64)},
	{900, (int)(51.984 * 16), (int)(1.00 * 64)},
	{900, (int)(53.817 * 16), (int)(1.00 * 64)},
	{900, (int)(55.715 * 16), (int)(1.00 * 64)},
	{900, (int)(57.680 * 16), (int)(1.00 * 64)},
	{900, (int)(59.714 * 16), (int)(1.00 * 64)},
	{900, (int)(61.819 * 16), (int)(1.00 * 64)},
	{900, (int)(64.000 * 16), (int)(1.00 * 64)},//+20=258

};

static const  int jxh62_mipi_30fps_exp_time_gain_60Hz[H62_30FPS_60HZ_EXP_TIME_TOTAL][3] =
{ // {time, analog gain, digital gain}
	{7, (int)(1.00 * 16), (int)(1.00 * 64)},
	{7, (int)(1.00 * 16), (int)(1.00 * 64)},
	{7, (int)(1.00 * 16), (int)(1.00 * 64)},
	{7, (int)(1.00 * 16), (int)(1.00 * 64)},
	{8, (int)(1.00 * 16), (int)(1.00 * 64)},
	{8, (int)(1.00 * 16), (int)(1.00 * 64)},
	{8, (int)(1.00 * 16), (int)(1.00 * 64)},
	{9, (int)(1.00 * 16), (int)(1.00 * 64)},
	{9, (int)(1.00 * 16), (int)(1.00 * 64)},
	{9, (int)(1.00 * 16), (int)(1.00 * 64)},
	{10, (int)(1.00 * 16), (int)(1.00 * 64)},
	{10, (int)(1.00 * 16), (int)(1.00 * 64)},
	{10, (int)(1.00 * 16), (int)(1.00 * 64)},
	{11, (int)(1.00 * 16), (int)(1.00 * 64)},
	{11, (int)(1.00 * 16), (int)(1.00 * 64)},
	{11, (int)(1.00 * 16), (int)(1.00 * 64)},
	{12, (int)(1.00 * 16), (int)(1.00 * 64)},
	{12, (int)(1.00 * 16), (int)(1.00 * 64)},
	{13, (int)(1.00 * 16), (int)(1.00 * 64)},
	{13, (int)(1.00 * 16), (int)(1.00 * 64)},
	{13, (int)(1.00 * 16), (int)(1.00 * 64)},
	{14, (int)(1.00 * 16), (int)(1.00 * 64)},
	{14, (int)(1.00 * 16), (int)(1.00 * 64)},
	{15, (int)(1.00 * 16), (int)(1.00 * 64)},
	{16, (int)(1.00 * 16), (int)(1.00 * 64)},
	{16, (int)(1.00 * 16), (int)(1.00 * 64)},
	{17, (int)(1.00 * 16), (int)(1.00 * 64)},
	{17, (int)(1.00 * 16), (int)(1.00 * 64)},
	{18, (int)(1.00 * 16), (int)(1.00 * 64)},
	{18, (int)(1.00 * 16), (int)(1.00 * 64)},
	{19, (int)(1.00 * 16), (int)(1.00 * 64)},
	{20, (int)(1.00 * 16), (int)(1.00 * 64)},
	{20, (int)(1.00 * 16), (int)(1.00 * 64)},
	{21, (int)(1.00 * 16), (int)(1.00 * 64)},
	{22, (int)(1.00 * 16), (int)(1.00 * 64)},
	{23, (int)(1.00 * 16), (int)(1.00 * 64)},
	{24, (int)(1.00 * 16), (int)(1.00 * 64)},
	{24, (int)(1.00 * 16), (int)(1.00 * 64)},
	{25, (int)(1.00 * 16), (int)(1.00 * 64)},
	{26, (int)(1.00 * 16), (int)(1.00 * 64)},
	{27, (int)(1.00 * 16), (int)(1.00 * 64)},
	{28, (int)(1.00 * 16), (int)(1.00 * 64)},
	{29, (int)(1.00 * 16), (int)(1.00 * 64)},
	{30, (int)(1.00 * 16), (int)(1.00 * 64)},
	{31, (int)(1.00 * 16), (int)(1.00 * 64)},
	{32, (int)(1.00 * 16), (int)(1.00 * 64)},
	{33, (int)(1.00 * 16), (int)(1.00 * 64)},
	{34, (int)(1.00 * 16), (int)(1.00 * 64)},
	{36, (int)(1.00 * 16), (int)(1.00 * 64)},
	{37, (int)(1.00 * 16), (int)(1.00 * 64)},
	{38, (int)(1.00 * 16), (int)(1.00 * 64)},
	{40, (int)(1.00 * 16), (int)(1.00 * 64)},
	{41, (int)(1.00 * 16), (int)(1.00 * 64)},
	{42, (int)(1.00 * 16), (int)(1.00 * 64)},
	{44, (int)(1.00 * 16), (int)(1.00 * 64)},
	{45, (int)(1.00 * 16), (int)(1.00 * 64)},
	{47, (int)(1.00 * 16), (int)(1.00 * 64)},
	{49, (int)(1.00 * 16), (int)(1.00 * 64)},
	{50, (int)(1.00 * 16), (int)(1.00 * 64)},
	{52, (int)(1.00 * 16), (int)(1.00 * 64)},
	{54, (int)(1.00 * 16), (int)(1.00 * 64)},
	{56, (int)(1.00 * 16), (int)(1.00 * 64)},
	{58, (int)(1.00 * 16), (int)(1.00 * 64)},
	{60, (int)(1.00 * 16), (int)(1.00 * 64)},
	{62, (int)(1.00 * 16), (int)(1.00 * 64)},
	{64, (int)(1.00 * 16), (int)(1.00 * 64)},
	{66, (int)(1.00 * 16), (int)(1.00 * 64)},
	{69, (int)(1.00 * 16), (int)(1.00 * 64)},
	{71, (int)(1.00 * 16), (int)(1.00 * 64)},
	{74, (int)(1.00 * 16), (int)(1.00 * 64)},
	{76, (int)(1.00 * 16), (int)(1.00 * 64)},
	{79, (int)(1.00 * 16), (int)(1.00 * 64)},
	{82, (int)(1.00 * 16), (int)(1.00 * 64)},
	{85, (int)(1.00 * 16), (int)(1.00 * 64)},
	{88, (int)(1.00 * 16), (int)(1.00 * 64)},
	{91, (int)(1.00 * 16), (int)(1.00 * 64)},
	{94, (int)(1.00 * 16), (int)(1.00 * 64)},
	{97, (int)(1.00 * 16), (int)(1.00 * 64)},
	{101, (int)(1.00 * 16), (int)(1.00 * 64)},
	{104, (int)(1.00 * 16), (int)(1.00 * 64)},
	{108, (int)(1.00 * 16), (int)(1.00 * 64)},
	{112, (int)(1.00 * 16), (int)(1.00 * 64)},
	{116, (int)(1.00 * 16), (int)(1.00 * 64)},
	{120, (int)(1.00 * 16), (int)(1.00 * 64)},
	{124, (int)(1.00 * 16), (int)(1.00 * 64)},
	{128, (int)(1.00 * 16), (int)(1.00 * 64)},
	{133, (int)(1.00 * 16), (int)(1.00 * 64)},
	{138, (int)(1.00 * 16), (int)(1.00 * 64)},
	{142, (int)(1.00 * 16), (int)(1.00 * 64)},
	{148, (int)(1.00 * 16), (int)(1.00 * 64)},
	{153, (int)(1.00 * 16), (int)(1.00 * 64)},
	{158, (int)(1.00 * 16), (int)(1.00 * 64)},
	{164, (int)(1.00 * 16), (int)(1.00 * 64)},
	{169, (int)(1.00 * 16), (int)(1.00 * 64)},
	{175, (int)(1.00 * 16), (int)(1.00 * 64)},
	{182, (int)(1.00 * 16), (int)(1.00 * 64)},
	{188, (int)(1.00 * 16), (int)(1.00 * 64)},
	{188, (int)(1.04 * 16), (int)(1.00 * 64)},
	{188, (int)(1.06 * 16), (int)(1.00 * 64)},
	{188, (int)(1.11 * 16), (int)(1.00 * 64)},
	{188, (int)(1.13 * 16), (int)(1.00 * 64)},
	{188, (int)(1.19 * 16), (int)(1.00 * 64)},
	{188, (int)(1.25 * 16), (int)(1.00 * 64)},
	{188, (int)(1.27 * 16), (int)(1.00 * 64)},
	{188, (int)(1.31 * 16), (int)(1.00 * 64)},
	{188, (int)(1.38 * 16), (int)(1.00 * 64)},
	{188, (int)(1.41 * 16), (int)(1.00 * 64)},
	{188, (int)(1.44 * 16), (int)(1.00 * 64)},
	{188, (int)(1.50 * 16), (int)(1.00 * 64)},
	{188, (int)(1.56 * 16), (int)(1.00 * 64)},
	{188, (int)(1.63 * 16), (int)(1.00 * 64)},
	{188, (int)(1.69 * 16), (int)(1.00 * 64)},
	{188, (int)(1.75 * 16), (int)(1.00 * 64)},
	{188, (int)(1.81 * 16), (int)(1.00 * 64)},
	{188, (int)(1.88 * 16), (int)(1.00 * 64)},
	{188, (int)(1.94 * 16), (int)(1.00 * 64)},
	{376, (int)(1.00 * 16), (int)(1.00 * 64)},
	{376, (int)(1.04 * 16), (int)(1.00 * 64)},
	{376, (int)(1.06 * 16), (int)(1.00 * 64)},
	{376, (int)(1.11 * 16), (int)(1.00 * 64)},
	{376, (int)(1.13 * 16), (int)(1.00 * 64)},
	{376, (int)(1.19 * 16), (int)(1.00 * 64)},
	{376, (int)(1.25 * 16), (int)(1.00 * 64)},
	{376, (int)(1.27 * 16), (int)(1.00 * 64)},
	{376, (int)(1.31 * 16), (int)(1.00 * 64)},
	{376, (int)(1.38 * 16), (int)(1.00 * 64)},
	{376, (int)(1.41 * 16), (int)(1.00 * 64)},
	{376, (int)(1.44 * 16), (int)(1.00 * 64)},
	{376, (int)(1.50 * 16), (int)(1.00 * 64)},
	{376, (int)(1.56 * 16), (int)(1.00 * 64)},
	{376, (int)(1.63 * 16), (int)(1.00 * 64)},
	{376, (int)(1.69 * 16), (int)(1.00 * 64)},
	{376, (int)(1.75 * 16), (int)(1.00 * 64)},
	{376, (int)(1.81 * 16), (int)(1.00 * 64)},
	{376, (int)(1.88 * 16), (int)(1.00 * 64)},
	{376, (int)(1.94 * 16), (int)(1.00 * 64)},
	{564, (int)(1.31 * 16), (int)(1.00 * 64)},
	{564, (int)(1.38 * 16), (int)(1.00 * 64)},
	{564, (int)(1.41 * 16), (int)(1.00 * 64)},
	{564, (int)(1.44 * 16), (int)(1.00 * 64)},
	{564, (int)(1.50 * 16), (int)(1.00 * 64)},
	{564, (int)(1.56 * 16), (int)(1.00 * 64)},
	{564, (int)(1.63 * 16), (int)(1.00 * 64)},
	{564, (int)(1.69 * 16), (int)(1.00 * 64)},
	{564, (int)(1.75 * 16), (int)(1.00 * 64)},
	{564, (int)(1.81 * 16), (int)(1.00 * 64)},
	{564, (int)(1.88 * 16), (int)(1.00 * 64)},
	{564, (int)(1.94 * 16), (int)(1.00 * 64)},
	{752, (int)(1.44 * 16), (int)(1.00 * 64)},
	{752, (int)(1.50 * 16), (int)(1.00 * 64)},
	{752, (int)(1.56 * 16), (int)(1.00 * 64)},
	{752, (int)(1.63 * 16), (int)(1.00 * 64)},
	{752, (int)(1.69 * 16), (int)(1.00 * 64)},
	{752, (int)(1.75 * 16), (int)(1.00 * 64)},
	{752, (int)(1.81 * 16), (int)(1.00 * 64)},
	{752, (int)(1.88 * 16), (int)(1.00 * 64)},
	{752, (int)(1.94 * 16), (int)(1.00 * 64)},
	{940, (int)(1.56 * 16), (int)(1.00 * 64)},
	{940, (int)(1.63 * 16), (int)(1.00 * 64)},
	{940, (int)(1.69 * 16), (int)(1.00 * 64)},
	{940, (int)(1.75 * 16), (int)(1.00 * 64)},
	{940, (int)(1.81 * 16), (int)(1.00 * 64)},
	{940, (int)(1.88 * 16), (int)(1.00 * 64)},
	{940, (int)(1.94 * 16), (int)(1.00 * 64)},
	{940, (int)(2.00 * 16), (int)(1.00 * 64)},
	{940, (int)(2.04 * 16), (int)(1.00 * 64)},
	{940, (int)(2.13 * 16), (int)(1.00 * 64)},
	{940, (int)(2.25 * 16), (int)(1.00 * 64)},
	{940, (int)(2.25 * 16), (int)(1.00 * 64)},
	{940, (int)(2.38 * 16), (int)(1.00 * 64)},
	{940, (int)(2.50 * 16), (int)(1.00 * 64)},
	{940, (int)(2.50 * 16), (int)(1.00 * 64)},
	{940, (int)(2.63 * 16), (int)(1.00 * 64)},
	{940, (int)(2.75 * 16), (int)(1.00 * 64)},
	{940, (int)(2.88 * 16), (int)(1.00 * 64)},
	{940, (int)(2.88 * 16), (int)(1.00 * 64)},
	{940, (int)(3.00 * 16), (int)(1.00 * 64)},
	{940, (int)(3.13 * 16), (int)(1.00 * 64)},
	{940, (int)(3.25 * 16), (int)(1.00 * 64)},
	{940, (int)(3.38 * 16), (int)(1.00 * 64)},
	{940, (int)(3.50 * 16), (int)(1.00 * 64)},
	{940, (int)(3.63 * 16), (int)(1.00 * 64)},
	{940, (int)(3.75 * 16), (int)(1.00 * 64)},
	{940, (int)(3.88 * 16), (int)(1.00 * 64)},
	{940, (int)(4.00 * 16), (int)(1.00 * 64)},
	{940, (int)(4.16 * 16), (int)(1.00 * 64)},
	{940, (int)(4.28 * 16), (int)(1.00 * 64)},
	{940, (int)(4.44 * 16), (int)(1.00 * 64)},
	{940, (int)(4.60 * 16), (int)(1.00 * 64)},
	{940, (int)(4.76 * 16), (int)(1.00 * 64)},
	{940, (int)(4.92 * 16), (int)(1.00 * 64)},
	{940, (int)(5.12 * 16), (int)(1.00 * 64)},
	{940, (int)(5.28 * 16), (int)(1.00 * 64)},
	{940, (int)(5.48 * 16), (int)(1.00 * 64)},
	{940, (int)(5.68 * 16), (int)(1.00 * 64)},
	{940, (int)(5.88 * 16), (int)(1.00 * 64)},
	{940, (int)(6.08 * 16), (int)(1.00 * 64)},
	{940, (int)(6.28 * 16), (int)(1.00 * 64)},
	{940, (int)(6.52 * 16), (int)(1.00 * 64)},
	{940, (int)(6.72 * 16), (int)(1.00 * 64)},
	{940, (int)(6.96 * 16), (int)(1.00 * 64)},
	{940, (int)(7.20 * 16), (int)(1.00 * 64)},
	{940, (int)(7.48 * 16), (int)(1.00 * 64)},
	{940, (int)(7.72 * 16), (int)(1.00 * 64)},
	{940, (int)(8.00 * 16), (int)(1.00 * 64)},
	{940, (int)(8.28 * 16), (int)(1.00 * 64)},
	{940, (int)(8.56 * 16), (int)(1.00 * 64)},
	{940, (int)(8.88 * 16), (int)(1.00 * 64)},
	{940, (int)(9.20 * 16), (int)(1.00 * 64)},
	{940, (int)(9.52 * 16), (int)(1.00 * 64)},
	{940, (int)(9.84 * 16), (int)(1.00 * 64)},
	{940, (int)(10.2 * 16), (int)(1.00 * 64)},
	{940, (int)(10.56 * 16), (int)(1.00 * 64)},
	{940, (int)(10.92 * 16), (int)(1.00 * 64)},
	{940, (int)(11.32 * 16), (int)(1.00 * 64)},
	{940, (int)(11.72 * 16), (int)(1.00 * 64)},
	{940, (int)(12.12 * 16), (int)(1.00 * 64)},
	{940, (int)(12.56 * 16), (int)(1.00 * 64)},
	{940, (int)(13.00 * 16), (int)(1.00 * 64)},
	{940, (int)(13.44 * 16), (int)(1.00 * 64)},
	{940, (int)(13.92 * 16), (int)(1.00 * 64)},
	{940, (int)(14.44 * 16), (int)(1.00 * 64)},
	{940, (int)(14.92 * 16), (int)(1.00 * 64)},
	{940, (int)(15.44 * 16), (int)(1.00 * 64)},
	{940, (int)(16.00 * 16), (int)(1.00 * 64)},

	{940, (int)(16.56 * 16), (int)(1.00 * 64)},
	{940, (int)(17.15 * 16), (int)(1.00 * 64)},
	{940, (int)(17.75 * 16), (int)(1.00 * 64)},
	{940, (int)(18.83 * 16), (int)(1.00 * 64)},
	{940, (int)(19.03 * 16), (int)(1.00 * 64)},
	{940, (int)(19.70 * 16), (int)(1.00 * 64)},
	{940, (int)(20.39 * 16), (int)(1.00 * 64)},
	{940, (int)(21.11 * 16), (int)(1.00 * 64)},
	{940, (int)(21.86 * 16), (int)(1.00 * 64)},
	{940, (int)(22.63 * 16), (int)(1.00 * 64)},
	{940, (int)(23.43 * 16), (int)(1.00 * 64)},
	{940, (int)(24.25 * 16), (int)(1.00 * 64)},
	{940, (int)(25.11 * 16), (int)(1.00 * 64)},
	{940, (int)(25.99 * 16), (int)(1.00 * 64)},
	{940, (int)(26.91 * 16), (int)(1.00 * 64)},
	{940, (int)(27.86 * 16), (int)(1.00 * 64)},
	{940, (int)(28.84 * 16), (int)(1.00 * 64)},
	{940, (int)(29.86 * 16), (int)(1.00 * 64)},
	{940, (int)(30.91 * 16), (int)(1.00 * 64)},
	{940, (int)(32.00 * 16), (int)(1.00 * 64)},//+20=238

	{940, (int)(33.128 * 16), (int)(1.00 * 64)},
	{940, (int)(34.296 * 16), (int)(1.00 * 64)},
	{940, (int)(35.506 * 16), (int)(1.00 * 64)},
	{940, (int)(36.758 * 16), (int)(1.00 * 64)},
	{940, (int)(38.054 * 16), (int)(1.00 * 64)},
	{940, (int)(39.396 * 16), (int)(1.00 * 64)},
	{940, (int)(40.785 * 16), (int)(1.00 * 64)},
	{940, (int)(42.224 * 16), (int)(1.00 * 64)},
	{940, (int)(43.713 * 16), (int)(1.00 * 64)},
	{940, (int)(45.254 * 16), (int)(1.00 * 64)},//+10
	{940, (int)(46.850 * 16), (int)(1.00 * 64)},
	{940, (int)(48.502 * 16), (int)(1.00 * 64)},
	{940, (int)(50.213 * 16), (int)(1.00 * 64)},
	{940, (int)(51.984 * 16), (int)(1.00 * 64)},
	{940, (int)(53.817 * 16), (int)(1.00 * 64)},
	{940, (int)(55.715 * 16), (int)(1.00 * 64)},
	{940, (int)(57.680 * 16), (int)(1.00 * 64)},
	{940, (int)(59.714 * 16), (int)(1.00 * 64)},
	{940, (int)(61.819 * 16), (int)(1.00 * 64)},
	{940, (int)(64.000 * 16), (int)(1.00 * 64)},//+20=258

};

////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTION IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////////////////////////////

static INT32S h62_sccb_open(void)
{
#if H62_I2C_MODE == SENSOR_I2C_MODE_SW
	h62_handle = drv_l2_sccb_open_ext(&h62_sccb_config);
	if(h62_handle == 0)
	{
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#elif H62_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_init(H62_I2C_DEVICE);
	h62_handle.devNumber = H62_I2C_DEVICE;
	h62_handle.slaveAddr = H62_ID;
	h62_handle.clkRate = 150;
#endif
	return STATUS_OK;
}

static void h62_sccb_close(void)
{
#if H62_I2C_MODE == SENSOR_I2C_MODE_SW
	if(h62_handle)
	{
		drv_l2_sccb_close(h62_handle);
		h62_handle = NULL;
	}
#elif H62_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_uninit(H62_I2C_DEVICE);
	h62_handle.slaveAddr = 0;
	h62_handle.clkRate = 0;
#endif
}

static INT32S h62_sccb_write(INT16U reg, INT8U value)
{
#if H62_I2C_MODE == SENSOR_I2C_MODE_SW
	return drv_l2_sccb_write(h62_handle, reg, value);

#elif H62_I2C_MODE == SENSOR_I2C_MODE_HW

	return drv_l1_reg_1byte_data_1byte_write(&h62_handle, reg, value);
#endif
}


static INT32S h62_sccb_read(INT16U reg, INT8U *value)
{
#if H62_I2C_MODE == SENSOR_I2C_MODE_SW
	INT16U data;

	if(drv_l2_sccb_read(h62_handle, reg, &data) >= 0)
	{
		*value = (INT8U)data;
		return STATUS_OK;
	}
	else
	{
		*value = 0xFF;
		DBG_PRINT("i2C read fail!\n");
		return STATUS_FAIL;
	}
#elif H62_I2C_MODE == SENSOR_I2C_MODE_HW

	return drv_l1_reg_1byte_data_1byte_read(&h62_handle, reg, value);
#endif
}

static INT32S h62_sccb_write_table(regval8_t *pTable)
{
	INT32U retry_cnt = 0;

	while(1)
	{
		if(pTable->reg_num == 0xFF && pTable->value == 0xFF)
		{
		  //  DBG_PRINT("sccb\r\n");
			break;
		}

		//DBG_PRINT("0x%02x, 0x%02x\r\n", pTable->reg_num, pTable->value);
		if(h62_sccb_write(pTable->reg_num, pTable->value) < 0)
		{
			DBG_PRINT("sccb write fail.\r\n");
			retry_cnt += 1;
			if(retry_cnt >= 10)
				return STATUS_FAIL;
			continue;
		}
		else
		{
			retry_cnt = 0;
		}
		/*
		if(h62_sccb_read(pTable->reg_num, &Rdata) < 0)
		{
			DBG_PRINT("sccb read fail.\r\n");
			continue;
		}*/
		pTable++;
	}
	return STATUS_OK;
}

static INT32S h62_sccb_read_table(regval8_t *pTable)
{
	INT8U Rdata;
	while(1)
	{
		if(pTable->reg_num == 0xFF && pTable->value == 0xFF)
		{
			break;
		}

		if(h62_sccb_read(pTable->reg_num, &Rdata) < 0)
		{
			DBG_PRINT("sccb read fail.\r\n");
			continue;
		}
		pTable++;
	}
	DBG_PRINT("\r\n\r\n");
	return STATUS_OK;
}

/*****************************************************************************************
+++++++++++++++++AEC/AGC
*****************************************************************************************/
static INT32U GainThrTable[6] =
{
	2.00 * 100, 4.00 * 100, 8.00 * 100, 12.00 * 100, 16.00 * 100, 20.00 * 100
};
static int h62_cvt_agc_gain(int agc_gain)
{
	int h62_agc_gain, i;
	int pow2;

	pow2 = 0;
	i = 5;
	do
	{
		if(agc_gain <= 0x1f)
			break;

		agc_gain >>= 1;
		pow2++;

		i--;
	} while(i != 0);

	agc_gain -= 0x10;
	if(agc_gain < 0) agc_gain = 0;
	h62_agc_gain = (pow2 << 4) + agc_gain;

	return h62_agc_gain;
}

static int h62_set_xfps_exposure_time(sensor_exposure_t *si)
{
	unsigned char t1, t2;
	int idx, temp, ret;
	INT8U 			iso_x = 1;

	//DBG_PRINT("%s:%d\n", __FUNCTION__, __LINE__);
	iso_x = (drv_l2_CdspGetIso() & 0xF);
	if(iso_x <= 0)
		iso_x = 1;

	si->sensor_ev_idx += si->ae_ev_step;
	if(si->sensor_ev_idx >= si->max_ev_idx)
		si->sensor_ev_idx = si->max_ev_idx;
	if(si->sensor_ev_idx < 0)
		si->sensor_ev_idx = 0;

	idx = si->sensor_ev_idx * 3;

	si->time = p_expTime_table[idx];
	si->analog_gain = (p_expTime_table[idx + 1] * iso_x);
	si->digital_gain = p_expTime_table[idx + 2];
	// exposure time
	if(si->time != pre_sensor_time)
	{
		pre_sensor_time = si->time;
		temp = si->time;

		t1 = (temp & 0xff);
		t2 = (temp >> 8) & 0x00ff;

#if (SOI_GROUP_WRITE == 0)
		ret = h62_sccb_write(0x01, t1);
		if(ret < 0)
		{
			DBG_PRINT("\r\n ERR:C0 ");
			return ret;
		}

		ret = h62_sccb_write(0x02, t2);
		if(ret < 0)
		{
			DBG_PRINT("\r\n ERR:C0 ");
			return ret;
		}

#else//soi group write
		ret = h62_sccb_write(0xC0, 0x01);
		if(ret < 0)
		{
			DBG_PRINT("\r\n ERR:C0 ");
			return ret;
		}

		ret = h62_sccb_write(0xC1, t1);
		if(ret < 0)
		{
			DBG_PRINT("\r\n ERR:C1 ");
			return ret;
		}

		ret = h62_sccb_write(0xC2, 0x02);
		if(ret < 0)
		{
			DBG_PRINT("\r\n ERR:C2 ");
			return ret;
		}

		ret = h62_sccb_write(0xC3, t2);
		if(ret < 0)
		{
			DBG_PRINT("\r\n ERR:C3 ");
			return ret;
		}
#endif

	}

	//gain
	if(si->analog_gain != pre_sensor_a_gain)
	{
		// gain
		pre_sensor_a_gain = si->analog_gain;

		temp = si->analog_gain;
		temp = h62_cvt_agc_gain(temp);
		t1 = temp & 0x007f;

#if (SOI_GROUP_WRITE == 0)
		ret = h62_sccb_write(0x00, t1);
		if(ret < 0)
		{
			DBG_PRINT("\r\n ERR:C0 ");
			return ret;
		}

#else   //group write
		ret = h62_sccb_write(0xC4, 0x00);
		if(ret < 0)
		{
			DBG_PRINT("\r\n ERR:C4 ");
			return ret;
		}

		ret = h62_sccb_write(0xC5, t1);
		if(ret < 0)
		{
			DBG_PRINT("\r\n ERR:C5 ");
			return ret;
		}
#endif

	}
#if (SOI_GROUP_WRITE == 1)	//soi group write
	ret = h62_sccb_write(0x1F, 0x80);   //group write enable
	if(ret < 0)
	{
		DBG_PRINT("ERR: triger group write FAIL!!!\r\n");
		return ret;
	}
#endif

	return 0;
}

static void h62_set_ae(int ev_step)
{
	seInfo.ae_ev_step = ev_step;
	h62_set_xfps_exposure_time(&seInfo);
}

void sensor_register_ae_ctrl(INT32U *handle)
{
	*handle = (INT32U)h62_set_ae;
}
void sensor_get_ae_info(sensor_exposure_t *si)
{
	memcpy(si, &seInfo, sizeof(sensor_exposure_t));
}

void sensor_set_max_lum(int max_lum)
{
	seInfo.max_ev_idx = seInfo.total_ev_idx - (128 - max_lum);
	DBG_PRINT("MaxLum:%d, MaxEvIdx:%d, TotalEvIdx:%d\r\n", max_lum, seInfo.max_ev_idx, seInfo.total_ev_idx);
}


/*****************************************************************************************
-------------------AEC/AGC
*****************************************************************************************/

#if MIPI_ISR_TEST
static void mipi_h62_handle(INT32U mipi, INT32U event)
{
	if(event & MIPI_LANE0_SOT_SYNC_ERR)
	{
		DBG_PRINT("LANE0_SOT_SYNC_ERR\r\n");
	}

	if(event & MIPI_HD_1BIT_ERR)
	{
		DBG_PRINT("HD_1BIT_ERR\r\n");
	}

	if(event & MIPI_HD_NBIT_ERR)
	{
		DBG_PRINT("HD_NBIT_ERR\r\n");
	}

	if(event & MIPI_DATA_CRC_ERR)
	{
		DBG_PRINT("DATA_CRC_ERR\r\n");
	}

	if(event & MIPI_LANE1_SOT_SYNC_ERR)
	{
		DBG_PRINT("LANE1_SOT_SYNC_ERR\r\n");
	}

	if(event & MIPI_CCIR_SOF)
	{
		DBG_PRINT("CCIR_SOF\r\n");
	}
}
#endif


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void h62_mipi_set_exp_freq(INT8U freq)
{
	if(freq == 50)
	{
		seInfo.sensor_ev_idx = H62_30FPS_50HZ_INIT_EV_IDX;
		seInfo.ae_ev_step = 0;
		seInfo.daylight_ev_idx = H62_30FPS_50HZ_DAY_EV_IDX;
		seInfo.night_ev_idx = H62_30FPS_50HZ_NIGHT_EV_IDX;
		seInfo.max_ev_idx = H62_30FPS_50HZ_MAX_EV_IDX;
		seInfo.total_ev_idx = H62_30FPS_50HZ_EXP_TIME_TOTAL;

		p_expTime_table = (int *)jxh62_mipi_30fps_exp_time_gain_50Hz;

		DBG_PRINT(">>Set frequence 50Hz.\r\n");
	}
	else if(freq == 60)
	{
		seInfo.sensor_ev_idx = H62_30FPS_60HZ_INIT_EV_IDX;
		seInfo.ae_ev_step = 0;
		seInfo.daylight_ev_idx = H62_30FPS_60HZ_DAY_EV_IDX;
		seInfo.night_ev_idx = H62_30FPS_60HZ_NIGHT_EV_IDX;
		seInfo.max_ev_idx = H62_30FPS_60HZ_MAX_EV_IDX;
		seInfo.total_ev_idx = H62_30FPS_60HZ_EXP_TIME_TOTAL;

		p_expTime_table = (int *)jxh62_mipi_30fps_exp_time_gain_60Hz;

		DBG_PRINT(">>Set frequence 60Hz.\r\n");
	}

	//if(seInfo.max_ev_idx > seInfo.total_ev_idx) seInfo.max_ev_idx = seInfo.total_ev_idx;
	sensor_set_max_lum(g_FavTable.max_lum);
}

void h62_mipi_seinfo_init(void)
{
	seInfo.sensor_ev_idx = H62_30FPS_50HZ_INIT_EV_IDX;
	seInfo.ae_ev_step = 0;
	seInfo.daylight_ev_idx = H62_30FPS_50HZ_DAY_EV_IDX;
	seInfo.night_ev_idx = H62_30FPS_50HZ_NIGHT_EV_IDX;
	seInfo.max_ev_idx = H62_30FPS_50HZ_MAX_EV_IDX;
	seInfo.total_ev_idx = H62_30FPS_50HZ_EXP_TIME_TOTAL;

	p_expTime_table = (int *)jxh62_mipi_30fps_exp_time_gain_50Hz;

	pre_sensor_time = 1;
	pre_sensor_a_gain = 0x00;

	seInfo.time = 1;
	seInfo.analog_gain = 0x00;
	seInfo.digital_gain = 0x40;

}

/**
 * @brief   initialization function
 * @param   sensor format parameters
 * @return 	none
 */

static void h62_cdsp_mipi_init(void)
{
	INT8S ret = 0;
	DBG_PRINT("%s\r\n", __func__);

	// reguest sccb
	h62_sccb_open();
	//ae init
	h62_mipi_seinfo_init();
	h62_mipi_set_exp_freq(60);
	// cdsp init
	ret = drv_l2_cdsp_open();
	if(ret < 0)
		DBG_PRINT(" CDSP init fail !\r\n");

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

	DBG_PRINT("Sensor H62 cdsp mipi[%d] init completed\r\n", MIPI_DEV_NO);
}

/**
 * @brief   un-initialization function
 * @param   sensor format parameters
 * @return 	none
 */
static void h62_cdsp_mipi_uninit(void)
{
	DBG_PRINT("%s\r\n", __func__);
	// cdsp disable
	drv_l2_cdsp_close();

	// mipi disable
	drv_l1_mipi_init(MIPI_DEV_NO, DISABLE, 0);

	// release sccb
	h62_sccb_close();

	// disable mclk
	drv_l2_sensor_set_mclkout(MCLK_NONE, 0);
}

/**
 * @brief   stream start function
 * @param   info index
 *
 * @return 	none
 */
void h62_cdsp_mipi_stream_on(INT32U index, INT32U bufA, INT32U bufB)
{
	INT16U target_w, target_h, sensor_w, sensor_h;
	gpCdspFmt_t format;

	INT8U docropflag = 0;

	// set sensor size
	DBG_PRINT("%s = %d\r\n", __func__, index);

	//Enabel mipi clk, set mipi clk
	if(h62_mipi_cfg.pixel_clk_sel == GENERATE_PIXEL_CLK)
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
	format.input_format = h62_cdsp_mipi_ops.info[index].input_format;
	format.output_format = h62_cdsp_mipi_ops.info[index].output_format;
	target_w = h62_cdsp_mipi_ops.info[index].target_w;
	target_h = h62_cdsp_mipi_ops.info[index].target_h;
	format.hpixel = sensor_w = h62_cdsp_mipi_ops.info[index].sensor_w;
	format.vline = sensor_h = h62_cdsp_mipi_ops.info[index].sensor_h;
	format.hoffset = h62_cdsp_mipi_ops.info[index].hoffset;
	format.voffset = h62_cdsp_mipi_ops.info[index].voffset;
	format.sensor_timing_mode = h62_cdsp_mipi_ops.info[index].interface_mode;
	format.sensor_hsync_mode = h62_cdsp_mipi_ops.info[index].hsync_active;
	format.sensor_vsync_mode = h62_cdsp_mipi_ops.info[index].vsync_active;

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
	drv_l2_CdspTableRegister((gpCisCali_t *)&g_cali, 0);
	drv_l2_cdsp_stream_on(ENABLE, bufA, bufB, 0);
	drv_l2_cdsp_enable((gpUserFav_t *)&g_FavTable, sensor_w, sensor_h, target_w, target_h, 0);
	drv_l2_cdsp_analog_gain_thr_init(GainThrTable);

	//mipi start
	if(drv_l1_mipi_set_parameter(MIPI_DEV_NO, &h62_mipi_cfg) < 0)
	{
		DBG_PRINT("MIPI0 init fail !!!\r\n");
	}

#if MIPI_ISR_TEST == 1
	if(MIPI_DEV_NO == MIPI_1)
	{
		drv_l1_mipi_B_isr_register(&mipi_h62_handle);
		drv_l1_mipi_set_irq_enable(MIPI_1, ENABLE, MIPI_SOF);
	}
	else
	{
		drv_l1_mipi_isr_register(&mipi_h62_handle);
		drv_l1_mipi_set_irq_enable(MIPI_0, ENABLE, MIPI_SOF);
	}

#endif

// reset sensor
	h62_sccb_write_table((regval8_t *)h62_reset_table);
	osDelay(2);		//wait 2ms

	// init sensor
	h62_sccb_write_table((regval8_t *)h62_init_table);

	// reset sensor ev idx
	seInfo.ae_ev_step = 0;
	h62_set_xfps_exposure_time(&seInfo);
	DBG_PRINT("H62 mipi Exposure Time set!!\r\n");

	drv_l1_CdspSetDataSource(MIPI_INPUT);

	DBG_PRINT("H62 cdsp mipi init done!!\r\n");
}
/*
 * @brief   stream stop function
 * @param   none
 * @return 	none
 */
static void h62_cdsp_mipi_stream_off(void)
{
	//drv_l2_cdsp_stream_off();
}

/**
 * @brief   get info function
 * @param   none
 * @return 	pointer to sensor information data
 */
static drv_l2_sensor_info_t *h62_cdsp_mipi_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1))
	{
		return NULL;
	}
	else
	{
		return (drv_l2_sensor_info_t *)&h62_cdsp_mipi_ops.info[index];
	}
}

/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t h62_cdsp_mipi_ops =
{
	SOURCE_CDSP_MIPI,		/* sensor name */
	h62_cdsp_mipi_init,
	h62_cdsp_mipi_uninit,
	h62_cdsp_mipi_stream_on,
	h62_cdsp_mipi_stream_off,
	h62_cdsp_mipi_get_info,
	h62_set_ae,
	sensor_get_ae_info,
	sensor_set_max_lum,
	h62_mipi_set_exp_freq,
	{
		/* 1st info */
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SBGGR10,		/* input format */
			V4L2_PIX_FMT_YUYV,			/* output format */
			30,							/* FPS in sensor */
			H62_OUT_WIDTH,				/* target width */
			H62_OUT_HEIGHT, 			/* target height */
			H62_WIDTH,					/* sensor width */
			H62_HEIGHT, 				/* sensor height */
#if SENSOR_FLIP
			0,
			0,
#else
			2,//1,						/* sensor h offset */
			0,
#endif						/* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
		/* 2st info */
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SBGGR10,		/* input format */
			V4L2_PIX_FMT_YUYV,			/* output format */
			30,							/* FPS in sensor */
			H62_OUT_WIDTH,				/* target width */
			H62_OUT_HEIGHT, 			/* target height */
			H62_WIDTH,					/* sensor width */
			H62_HEIGHT, 				/* sensor height */
#if SENSOR_FLIP
			0,
			0,
#else
			2,//1,						/* sensor h offset */
			0,
#endif						/* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
}
};
#endif //(defined _SENSOR_H62_CDSP_MIPI) && (_SENSOR_H62_CDSP_MIPI == 1)

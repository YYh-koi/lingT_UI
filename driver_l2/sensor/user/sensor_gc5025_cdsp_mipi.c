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


#if (defined _SENSOR_GC5025_CDSP_MIPI) && (_SENSOR_GC5025_CDSP_MIPI == 1)

#include "drv_l2_user_calibration.h"
#include "drv_l2_user_preference.h"
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define vu(x) (*(volatile unsigned *)(x))

#define CONFIG_FPGA_TEST                		0
#define COLOR_BAR_EN                    		0
#define MIPI_LANE_NO			        		2       // 1 or 2 lane
#define MIPI_DEV_NO                     		0   	//0:MIPI_0(IOC8~11) or 1:MIPI_1(IOE0~9)

/* Dual Sensor Setting */
#define BINMODE_OUT								0
#define NUM_SENSOR								SINGLE_SENSOR
#define MIPI_ISR_TEST							0//(BINMODE_OUT && (NUM_SENSOR != SINGLE_SENSOR)) //0: one sensor, 1: for two sensors
#define CDSP_IDX								((NUM_SENSOR == REAR_SENSOR) ? 1:0)
#define INTERFACE_FLAG		(S_MIPI_(MIPI_DEV_NO) |	S_CDSP | (BINMODE_OUT*S_BINMODE_(MIPI_DEV_NO)) | S_NUMSENSOR(NUM_SENSOR))

#define	GC5025_ID								(0x6E)
#define GC5025_WIDTH							 2560
#define GC5025_HEIGHT					    	(1944)
#define GC5025_OUT_WIDTH						 2560
#define GC5025_OUT_HEIGHT						(1944)

#define GC5025_I2C_MODE	            			SENSOR_I2C_MODE_HW

#if GC5025_I2C_MODE == SENSOR_I2C_MODE_SW
#define	SENSOR_I2C_SCL							IO_B4
#define	SENSOR_I2C_SDA							IO_B5
#define	SENSOR_I2C_DRIVING						IOB_DRV_20mA
#else
#define I2C_DEVICE_NUM							I2C_0
#endif


//Exposure Time Setting
#define GC5025_20FPS_50HZ_DAY_EV_IDX 			143
#define GC5025_20FPS_50HZ_NIGHT_EV_IDX			194
#define GC5025_20FPS_50HZ_EXP_TIME_TOTAL		254
#define GC5025_20FPS_50HZ_INIT_EV_IDX 			((GC5025_20FPS_50HZ_DAY_EV_IDX + GC5025_20FPS_50HZ_NIGHT_EV_IDX) >> 1)
#define GC5025_20FPS_50HZ_MAX_EV_IDX			(GC5025_20FPS_50HZ_EXP_TIME_TOTAL - 4)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
extern const ALIGN4 INT16U g_lsc_gain_A[];
//static INT16U *p_lsc_table_gain_start;

#define DBG_MSG(...)
#define ERR_MSG	DBG_PRINT

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
//static int ob_cnt = 0;
static int *p_expTime_table;
static sensor_exposure_t  seInfo;
static int pre_sensor_a_gain, pre_sensor_time;
static INT32U GainThrTable[6] =
{
    1.90*64, 4.00*64, 6.00*64, 8.00*64, 12.00*64, 16.00*64
};

//static INT32U gc5025_analog_gain;
static int num_sensors = NUM_SENSOR;


#if GC5025_I2C_MODE == SENSOR_I2C_MODE_SW
sccb_config_t GC5025_sccb_config =
{
	SENSOR_I2C_SCL,				//scl_port
	SENSOR_I2C_DRIVING,			//scl_drv
	SENSOR_I2C_SDA,				//sda_port
	SENSOR_I2C_DRIVING,			//sda_drv
	IO_F15,						//pwdn_port
	0,				            //pwdn_drv
	0,							//have_pwdn
	8,							//RegBits
	8,							//DataBits
	GC5025_ID,					//slaveAddr
	0x20000,					//timeout
	200							//clock_rate
};

static void *gc5025_handle;
#elif GC5025_I2C_MODE == SENSOR_I2C_MODE_HW
	static drv_l1_i2c_bus_handle_t gc5025_handle;
#endif

static mipi_config_t gc5025_mipi_cfg =
{
	DISABLE, 			/* low power, 0:disable, 1:enable */

#if MIPI_LANE_NO == 1
	D_PHY_SAMPLE_NEG, 	/* byte clock edge, 0:posedge, 1:negedge */
	MIPI_1_LANE,		/* lane */
	D_PHY_BYTE_CLK,		/* byte clock source */
#elif MIPI_LANE_NO == 2
    D_PHY_SAMPLE_POS,
	MIPI_2_LANE,		/* lane */
	D_PHY_BYTE_CLK,		/* byte clock source */
#endif

	MIPI_AUTO_DETECT,	/* data mode, 0:auto detect, 1:user define */
	MIPI_RAW10,			/* data type, valid when data mode is 1*/
	MIPI_DATA_TO_CDSP,	/* data type, 1: data[7:0]+2':00 to cdsp,  0: 2'00+data[7:0] to csi */
	0,					/* RSD 2 */

	GC5025_WIDTH,		/* width, 0~0xFFFF */
	GC5025_HEIGHT,		/* height, 0~0xFFFF */
	4,
	4,

	ENABLE,				/* blanking_line_en, 0:disable, 1:enable */
	0,//NULL,			/* RSD 3 */

	ENABLE,				/* ecc, 0:disable, 1:enable */
	MIPI_ECC_ORDER3,	/* ecc order */
	200,				/* data mask, unit is ns */
	MIPI_CHECK_LP_00//MIPI_CHECK_HS_SEQ	//MIPI_CHECK_LP_00///* check hs sequence or LP00 for clock lane */
};

static const regval8_t gc5025_reset_table[] =
{// SYS
	{0xFE, 0x80},
	{0xFE, 0x80},
	{0xFE, 0x80},
	{0xFF, 0xFF}
};

static const regval8_t gc5025_15fps_624M_2560x1944[] =
{
    //MCLK=24MHz,MIPI_Clock=624MHz,frame_rate=15 fps
    //Actual_window_size=2560*1944
    //pixel_line=3713,frame_line=2800,row_time=23.8us
    {0xfe,0x80},
    {0xfe,0x80},
    {0xfe,0x80},
    {0xf7,0x01},
    #if FPGA==1    //MCLK need 12MHz
    {0xf8,0x01},
    #else
    {0xf8,0x0C},
    #endif
    {0xf9,0x00},
    {0xfa,0xa0},
    {0xfc,0x2a},
    {0xfe,0x03},
    {0x01,0x07},
    {0xfc,0x2e},
    {0xfe,0x00},
    {0x88,0x03},
    {0xe7,0xcc},
    {0x03,0x07},
    {0x04,0x08},
    {0x05,0x02},
    {0x06,0x6b},
    {0x07,0x03},
    {0x08,0x3c},
    {0x09,0x00},
    {0x0a,0x1c},
    {0x0b,0x00},
    {0x0c,0x04},
    {0x0d,0x07},
    {0x0e,0x9c},  //1948=1944+4
    {0x0f,0x0a},
    {0x10,0x30},  //2608=2592+16
    {0x17,0xc0},
    {0x18,0x02},
    {0x19,0x17},
    {0x1a,0x1a},
    {0x1c,0x1c},//28
    {0x1d,0x13},
    {0x1e,0x90},
    {0x1f,0xb0},
    {0x20,0x2b},
    {0x21,0x2b},
    {0x26,0x2b},
    {0x25,0xc1},
    {0x27,0x64},
    {0x28,0x00},
    {0x29,0x3f},
    {0x2b,0x80},
    {0x2f,0x4a},//4d
    {0x30,0x11},//44
    {0x31,0x20},//40
    {0x32,0xa0},//c0
    {0x33,0x00},
    {0x34,0x55},
    {0x38,0x02},//08
    {0x39,0x00},//08
    {0x3a,0x00},//0a
    {0x3b,0x00},//09
    {0x3c,0x02},//0d
    {0x3d,0x02},
    {0x81,0x60},
    {0xcb,0x02},
    {0xcd,0xad},//af
    {0xcf,0x50},
    {0xd0,0xb3},//b2
    {0xd1,0x18},//00
    {0xd3,0xcc},//cb
    {0xd9,0xaa},
    {0xdc,0x03},
    {0xdd,0xaa},
    {0xe0,0x00},
    {0xe1,0x0a},
    {0xe3,0x2a},
    {0xe4,0xa0},//c0
    {0xe5,0x06},//09
    {0xe6,0x10},
    {0xe7,0xc2},//cc
    {0x80,0x10},
    {0x89,0x03},
    {0xfe,0x01},
    {0x88,0xf7},
    {0x8a,0x03},
    {0x8e,0xc7},
    {0xfe,0x00},
    {0x40,0x22},
    {0x43,0x03},
    {0xae,0x40},
    {0x60,0x00},
    {0x61,0x80},
    {0xb0,0x58},
    {0xb1,0x01},
    {0xb2,0x00},
    {0xb6,0x00},
    {0x90,0x01},
    {0x91,0x00},
    {0x92,0x03},
    {0x93,0x00},
    {0x94,0x04},
    {0x95,0x07},
    {0x96,0x98}, // 1944
    {0x97,0x0a},
    {0x98,0x00}, // 2560
    {0xfe,0x00},
    {0x3f,0x91},
    {0xfe,0x03},
    {0x02,0x07},
    {0x03,0x8e},
    {0x06,0x80},

    {0x12,0x80},
    {0x13,0x0c},

    {0x15,0x02},
    {0x16,0x09},
    {0x18,0x0a},
    {0x21,0x10},
    {0x22,0x05},
    {0x23,0x20},
    {0x24,0x02},
    {0x25,0x20},
    {0x26,0x08},
    {0x29,0x06},
    {0x2a,0x0a},
    {0x2b,0x08},
    {0xfe,0x00},

    {0xFF, 0xFF}
};

static const regval8_t gc5025_20fps_624M_2560x1944[] =
{
    //MCLK=24MHz,MIPI_Clock=624MHz,frame_rate=20 fps
    //Actual_window_size=2560*1944
    //pixel_line=3713,frame_line=2100,row_time=23.8us
    //FPGA=1,  MCLK need 12MHz

    {0xfe,0x80},
    {0xfe,0x80},
    {0xfe,0x80},
    {0xf7,0x01},
#if FPGA==1         //MCLK need 12MHz
    {0xf8,0x02},    //0xc, org
#else
    {0xf8,0x0C},
#endif
    {0xf9,0x00},
    {0xfa,0xa0},
    {0xfc,0x2a},
    {0xfe,0x03},
    {0x01,0x07},
    {0xfc,0x2e},
    {0xfe,0x00},
    {0x88,0x03},
    {0xe7,0xcc},
    {0x03,0x07},
    {0x04,0x08},
    {0x05,0x02},
    {0x06,0x6b},
    {0x07,0x00},
    {0x08,0x80},
    {0x09,0x00},
    {0x0a,0x1c}, //org 0x1c
    {0x0b,0x00},
    {0x0c,0x04}, // org 0x04
    {0x0d,0x07},
    {0x0e,0x9c},  //1948=1944+4
    {0x0f,0x0a},
    {0x10,0x30},  //2608=2592+16
    {0x17,0xc0},
    {0x18,0x02},
    {0x19,0x17},
    {0x1a,0x1a},
    {0x1c,0x1c},//28
    {0x1d,0x13},
    {0x1e,0x90},
    {0x1f,0xb0},
    {0x20,0x2b},
    {0x21,0x2b},
    {0x26,0x2b},
    {0x25,0xc1},
    {0x27,0x64},
    {0x28,0x00},
    {0x29,0x3f},
    {0x2b,0x80},
    {0x2f,0x4a},//4d
    {0x30,0x11},//44
    {0x31,0x20},//40
    {0x32,0xa0},//c0
    {0x33,0x00},
    {0x34,0x55},
    {0x38,0x02},//08
    {0x39,0x00},//08
    {0x3a,0x00},//0a
    {0x3b,0x00},//09
    {0x3c,0x02},//0d
    {0x3d,0x02},
    {0x81,0x60},
    {0xcb,0x02},
    {0xcd,0xad},//af
    {0xcf,0x50},
    {0xd0,0xb3},//b2
    {0xd1,0x18},//00
    {0xd3,0xcc},//cb
    {0xd9,0xaa},
    {0xdc,0x03},
    {0xdd,0xaa},
    {0xe0,0x00},
    {0xe1,0x0a},
    {0xe3,0x2a},
    {0xe4,0xa0},//c0
    {0xe5,0x06},//09
    {0xe6,0x10},
    {0xe7,0xc2},//cc
    {0x80,0x10},
    {0x89,0x03},
    {0xfe,0x01},
    {0x88,0xf7},
    {0x8a,0x03},
    {0x8e,0xc7},
    {0xfe,0x00},
    {0x40,0x22},
    {0x43,0x03},
    {0xae,0x40},
    {0x60,0x00},
    {0x61,0x80},
    {0xb0,0x58},
    {0xb1,0x01},
    {0xb2,0x00},
    {0xb6,0x00},
    {0x90,0x01},
    {0x91,0x00},
    {0x92,0x03},
    {0x93,0x00},
    {0x94,0x04},
    {0x95,0x07},
    {0x96,0x98}, // 1944
    {0x97,0x0a},
    {0x98,0x00}, // 2560
    {0xfe,0x00},
    {0x3f,0x91},
    {0xfe,0x03},
    {0x02,0x07},
    {0x03,0x8e},
    {0x06,0x80},

    {0x12,0x80},
    {0x13,0x0c},

    {0x15,0x02},
    {0x16,0x09},
    {0x18,0x0a},
    {0x21,0x10},
    {0x22,0x05},
    {0x23,0x20},
    {0x24,0x02},
    {0x25,0x20},
    {0x26,0x08},
    {0x29,0x06},
    {0x2a,0x0a},
    {0x2b,0x08},
    {0xfe,0x00},

    {0xFF, 0xFF}
};

static const regval8_t gc5025_25fps_672M_2560x1944[] =
{
    //MCLK=24MHz,MIPI_Clock=672MHz,frame_rate=25 fps
    //Actual_window_size=2560*1944
    //pixel_line=3456,frame_line=1980,row_time=20.57us
    {0xfe,0x80},
    {0xfe,0x80},
    {0xfe,0x80},
    {0xf7,0x01},
    {0xf8,0x0d},
    {0xf9,0x00},
    {0xfa,0xa0},
    {0xfc,0x2a},
    {0xfe,0x03},
    {0x01,0x07},
    {0xfc,0x2e},
    {0xfe,0x00},
    {0x88,0x03},
    {0xe7,0xcc},
    {0x03,0x07},
    {0x04,0x08},
    {0x05,0x02},
    {0x06,0x40},
    {0x07,0x00},
    {0x08,0x08},
    {0x09,0x00},
    {0x0a,0x1c},
    {0x0b,0x00},
    {0x0c,0x04},
    {0x0d,0x07},
    {0x0e,0x9c},  //1948=1944+4
    {0x0f,0x0a},
    {0x10,0x30},  //2608=2592+16
    {0x17,0xc0},
    {0x18,0x02},
    {0x19,0x17},
    {0x1a,0x1a},
    {0x1c,0x1c},//28
    {0x1d,0x13},
    {0x1e,0x90},
    {0x1f,0xb0},
    {0x20,0x2b},
    {0x21,0x2b},
    {0x26,0x2b},
    {0x25,0xc1},
    {0x27,0x64},
    {0x28,0x00},
    {0x29,0x3f},
    {0x2b,0x80},
    {0x2f,0x4a},//4d
    {0x30,0x11},//44
    {0x31,0x20},//40
    {0x32,0xa0},//c0
    {0x33,0x00},
    {0x34,0x55},
    {0x38,0x02},//08
    {0x39,0x00},//08
    {0x3a,0x00},//0a
    {0x3b,0x00},//09
    {0x3c,0x02},//0d
    {0x3d,0x02},
    {0x81,0x60},
    {0xcb,0x02},
    {0xcd,0xad},//af
    {0xcf,0x50},
    {0xd0,0xb3},//b2
    {0xd1,0x18},//00
    {0xd3,0xcc},//cb
    {0xd9,0xaa},
    {0xdc,0x03},
    {0xdd,0xaa},
    {0xe0,0x00},
    {0xe1,0x0a},
    {0xe3,0x2a},
    {0xe4,0xa0},//c0
    {0xe5,0x06},//09
    {0xe6,0x10},
    {0xe7,0xc2},//cc
    {0x80,0x10},
    {0x89,0x03},
    {0xfe,0x01},
    {0x88,0xf7},
    {0x8a,0x03},
    {0x8e,0xc7},
    {0xfe,0x00},
    {0x40,0x22},
    {0x43,0x03},
    {0xae,0x40},
    {0x60,0x00},
    {0x61,0x80},
    {0xb0,0x58},
    {0xb1,0x01},
    {0xb2,0x00},
    {0xb6,0x00},
    {0x90,0x01},
    {0x91,0x00},
    {0x92,0x03},
    {0x93,0x00},
    {0x94,0x04},
    {0x95,0x07},
    {0x96,0x98}, // 1944
    {0x97,0x0a},
    {0x98,0x00}, // 2560
    {0xfe,0x00},
    {0x3f,0x91},
    {0xfe,0x03},
    {0x02,0x07},
    {0x03,0x8e},
    {0x06,0x80},

    {0x12,0x80},
    {0x13,0x0c},

    {0x15,0x02},
    {0x16,0x09},
    {0x18,0x0a},
    {0x21,0x10},
    {0x22,0x05},
    {0x23,0x20},
    {0x24,0x02},
    {0x25,0x20},
    {0x26,0x08},
    {0x29,0x06},
    {0x2a,0x0a},
    {0x2b,0x08},
    {0xfe,0x00},
    {0xFF, 0xFF}
};

static const int gc5025_20fps_exp_time_gain_50Hz[GC5025_20FPS_50HZ_EXP_TIME_TOTAL][3] =
{ //AGain max 23, DGain max
    { 4	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 4	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 4	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 5	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 5	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 5	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 5	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 5	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 6	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 6	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 6	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 6	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 6	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 7	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 7	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 7	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 7	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 8	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 8	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 8	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 8	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 9	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 9	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 9	  ,     (int)(1.00*64),   (int)(1.00*64) },
    { 10  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 10  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 10  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 11  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 11  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 11  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 12  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 12  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 13  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 13  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 14  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 14  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 15  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 15  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 16  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 16  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 17  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 17  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 18  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 19  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 19  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 20  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 21  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 21  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 22  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 23  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 24  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 24  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 25  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 26  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 27  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 28  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 29  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 30  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 31  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 32  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 33  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 35  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 36  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 37  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 38  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 40  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 41  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 43  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 44  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 46  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 47  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 49  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 51  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 53  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 54  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 56  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 58  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 60  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 62  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 65  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 67  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 69  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 72  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 74  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 77  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 80  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 82  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 85  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 88  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 91  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 95  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 98  ,	    (int)(1.00*64),   (int)(1.00*64) },
    { 101  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 105  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 109  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 113  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 117  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 121  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 125  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 129  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 134  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 139  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 143  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 148  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 154  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 159  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 165  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 171  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 177  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 183  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 189  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 196  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 203  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 210  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 217  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 225  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 233  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 241  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 250  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 259  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 268  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 277  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 287  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 297  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 307  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 318  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 330  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 341  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 353  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 366  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 379  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 392  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 406  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 420  ,	(int)(1.00*64),   (int)(1.00*64) },
    { 420  ,	(int)(1.0625*64), (int)(1.00*64) },
    { 420  ,	(int)(1.0625*64), (int)(1.00*64) },
    { 420  ,	(int)(1.125*64),  (int)(1.00*64) },
    { 420  ,	(int)(1.125*64),  (int)(1.00*64) },
    { 420  ,	(int)(1.1875*64), (int)(1.00*64) },
    { 420  ,	(int)(1.25*64),   (int)(1.00*64) },
    { 420  ,	(int)(1.25*64),   (int)(1.00*64) },
    { 420  ,	(int)(1.3125*64), (int)(1.00*64) },
    { 420  ,	(int)(1.375*64),  (int)(1.00*64) },
    { 420  ,	(int)(1.4375*64), (int)(1.00*64) },
    { 420  ,	(int)(1.4375*64), (int)(1.00*64) },
    { 420  ,	(int)(1.5*64),    (int)(1.00*64) },
    { 420  ,	(int)(1.5625*64), (int)(1.00*64) },
    { 420  ,	(int)(1.625*64),  (int)(1.00*64) },
    { 420  ,	(int)(1.6875*64), (int)(1.00*64) },
    { 420  ,	(int)(1.75*64), (int)(1.00*64) },
    { 420  ,	(int)(1.8125*64), (int)(1.00*64) },
    { 420  ,	(int)(1.875*64), (int)(1.00*64) },
    { 420  ,	(int)(1.9375*64), (int)(1.00*64) },
    { 840  ,	(int)(1.00*64), (int)(1.00*64) },
    { 840  ,	(int)(1.0625*64), (int)(1.00*64) },
    { 840  ,	(int)(1.0625*64), (int)(1.00*64) },
    { 840  ,	(int)(1.125*64), (int)(1.00*64) },
    { 840  ,	(int)(1.125*64), (int)(1.00*64) },
    { 840  ,	(int)(1.1875*64), (int)(1.00*64) },
    { 840  ,	(int)(1.25*64), (int)(1.00*64) },
    { 840  ,	(int)(1.25*64), (int)(1.00*64) },
    { 840  ,	(int)(1.3125*64), (int)(1.00*64) },
    { 840  ,	(int)(1.375*64), (int)(1.00*64) },
    { 840  ,	(int)(1.4375*64), (int)(1.00*64) },
    { 840  ,	(int)(1.4375*64), (int)(1.00*64) },
    { 840  ,	(int)(1.5*64), (int)(1.00*64) },
    { 840  ,	(int)(1.5625*64), (int)(1.00*64) },
    { 840  ,	(int)(1.625*64), (int)(1.00*64) },
    { 840  ,	(int)(1.6875*64), (int)(1.00*64) },
    { 840  ,	(int)(1.75*64), (int)(1.00*64) },
    { 840  ,	(int)(1.8125*64), (int)(1.00*64) },
    { 840  ,	(int)(1.875*64), (int)(1.00*64) },
    { 840  ,	(int)(1.9375*64), (int)(1.00*64) },
    { 1260 ,	(int)(1.3125*64), (int)(1.00*64) },
    { 1260 ,	(int)(1.375*64), (int)(1.00*64) },
    { 1260 ,	(int)(1.4375*64), (int)(1.00*64) },
    { 1260 ,	(int)(1.4375*64), (int)(1.00*64) },
    { 1260 ,	(int)(1.5*64), (int)(1.00*64) },
    { 1260 ,	(int)(1.5625*64), (int)(1.00*64) },
    { 1260 ,	(int)(1.625*64), (int)(1.00*64) },
    { 1260 ,	(int)(1.6875*64), (int)(1.00*64) },
    { 1260 ,	(int)(1.75*64), (int)(1.00*64) },
    { 1260 ,	(int)(1.8125*64), (int)(1.00*64) },
    { 1260 ,	(int)(1.875*64), (int)(1.00*64) },
    { 1260 ,	(int)(1.9375*64), (int)(1.00*64) },
    { 1680 ,	(int)(1.5*64), (int)(1.00*64) },
    { 1680 ,	(int)(1.5625*64), (int)(1.00*64) },
    { 1680 ,	(int)(1.625*64), (int)(1.00*64) },
    { 1680 ,	(int)(1.6875*64), (int)(1.00*64) },
    { 1680 ,	(int)(1.75*64), (int)(1.00*64) },
    { 1680 ,	(int)(1.8125*64), (int)(1.00*64) },
    { 1680 ,	(int)(1.875*64), (int)(1.00*64) },
    { 1680 ,	(int)(1.9375*64), (int)(1.00*64) },
    { 1680 ,	(int)(2.0*64), (int)(1.00*64) },
    { 1680 ,	(int)(2.07*64), (int)(1.00*64) },
    { 1680 ,	(int)(2.14*64), (int)(1.00*64) },
    { 1680 ,	(int)(2.22*64), (int)(1.00*64) },
    { 1680 ,	(int)(2.30*64), (int)(1.00*64) },
    { 1680 ,	(int)(2.38*64), (int)(1.00*64) },
    { 1680 ,	(int)(2.46*64), (int)(1.00*64) },
    { 1680 ,	(int)(2.55*64), (int)(1.00*64) },
    { 1680 ,	(int)(2.64*64), (int)(1.00*64) },
    { 1680 ,	(int)(2.73*64), (int)(1.00*64) },
    { 1680 ,	(int)(2.83*64), (int)(1.00*64) },
    { 1680 ,	(int)(2.93*64), (int)(1.00*64) },
    { 1680 ,	(int)(3.03*64), (int)(1.00*64) },
    { 1680 ,	(int)(3.14*64), (int)(1.00*64) },
    { 1680 ,	(int)(3.25*64), (int)(1.00*64) },
    { 1680 ,	(int)(3.36*64), (int)(1.00*64) },
    { 1680 ,	(int)(3.48*64), (int)(1.00*64) },
    { 1680 ,	(int)(3.61*64), (int)(1.00*64) },
    { 1680 ,	(int)(3.73*64), (int)(1.00*64) },
    { 1680 ,	(int)(3.86*64), (int)(1.00*64) },
    { 1680 ,	(int)(4.00*64), (int)(1.00*64) },
    { 1680 ,	(int)(4.14*64), (int)(1.00*64) },
    { 1680 ,	(int)(4.29*64), (int)(1.00*64) },
    { 1680 ,	(int)(4.44*64), (int)(1.00*64) },
    { 1680 ,	(int)(4.59*64), (int)(1.00*64) },
    { 1680 ,	(int)(4.76*64), (int)(1.00*64) },
    { 1680 ,	(int)(4.92*64), (int)(1.00*64) },
    { 1680 ,	(int)(5.10*64), (int)(1.00*64) },
    { 1680 ,	(int)(5.28*64), (int)(1.00*64) },
    { 1680 ,	(int)(5.46*64), (int)(1.00*64) },
    { 1680 ,	(int)(5.66*64), (int)(1.00*64) },
    { 1680 ,	(int)(5.86*64), (int)(1.00*64) },
    { 1680 ,	(int)(6.06*64), (int)(1.00*64) },
    { 1680 ,	(int)(6.28*64), (int)(1.00*64) },
    { 1680 ,	(int)(6.50*64), (int)(1.00*64) },
    { 1680 ,	(int)(6.73*64), (int)(1.00*64) },
    { 1680 ,	(int)(6.96*64), (int)(1.00*64) },
    { 1680 ,	(int)(7.21*64), (int)(1.00*64) },
    { 1680 ,	(int)(7.46*64), (int)(1.00*64) },
    { 1680 ,	(int)(7.73*64), (int)(1.00*64) },
    { 1680 ,	(int)(8.00*64), (int)(1.00*64) },
    { 1680 ,	(int)(8.28*64), (int)(1.00*64) },
    { 1680 ,	(int)(8.57*64), (int)(1.00*64) },
    { 1680 ,	(int)(8.88*64), (int)(1.00*64) },
    { 1680 ,	(int)(9.19*64), (int)(1.00*64) },
    { 1680 ,	(int)(9.51*64), (int)(1.00*64) },
    { 1680 ,	(int)(9.85*64), (int)(1.00*64) },
    { 1680 ,	(int)(10.20*64), (int)(1.00*64) },
    { 1680 ,	(int)(10.56*64), (int)(1.00*64) },
    { 1680 ,	(int)(10.93*64), (int)(1.00*64) },
    { 1680 ,	(int)(11.31*64), (int)(1.00*64) },
    { 1680 ,	(int)(11.71*64), (int)(1.00*64) },
    { 1680 ,	(int)(12.13*64), (int)(1.00*64) },
    { 1680 ,	(int)(12.55*64), (int)(1.00*64) },
    { 1680 ,	(int)(13.00*64), (int)(1.00*64) },
    { 1680 ,	(int)(13.45*64), (int)(1.00*64) },
    { 1680 ,	(int)(13.93*64), (int)(1.00*64) },
    { 1680 ,	(int)(14.42*64), (int)(1.00*64) },
    { 1680 ,	(int)(14.93*64), (int)(1.00*64) },
    { 1680 ,	(int)(15.45*64), (int)(1.00*64) },
    { 1680 ,	(int)(16.00*64), (int)(1.00*64)  }
};//50Hz

////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTION IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////////////////////////////

static INT32S gc5025_sccb_open(void)
{
#if GC5025_I2C_MODE == SENSOR_I2C_MODE_SW
	gc5025_handle = drv_l2_sccb_open_ext(&GC5025_sccb_config);

	if(gc5025_handle == 0) {
		ERR_MSG("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#elif  GC5025_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_init(I2C_DEVICE_NUM);
	gc5025_handle.devNumber = I2C_DEVICE_NUM;
	gc5025_handle.slaveAddr = GC5025_ID;
	gc5025_handle.clkRate = 100;
#endif
	return STATUS_OK;
}

static void gc5025_sccb_close(void)
{
#if GC5025_I2C_MODE == SENSOR_I2C_MODE_SW
	if(gc5025_handle) {
		drv_l2_sccb_close(gc5025_handle);
		gc5025_handle = NULL;
	}
#elif GC5025_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_uninit(I2C_DEVICE_NUM);
	gc5025_handle.slaveAddr = 0;
	gc5025_handle.clkRate = 0;
#endif
}

static INT32S gc5025_sccb_write(INT16U reg, INT8U value)
{
#if GC5025_I2C_MODE == SENSOR_I2C_MODE_SW
	return drv_l2_sccb_write(gc5025_handle, reg, value);

#elif GC5025_I2C_MODE == SENSOR_I2C_MODE_HW
	return drv_l1_reg_1byte_data_1byte_write(&gc5025_handle, reg, value);
	/*
	INT8U data[2];

	data[0] = reg & 0xFF;
	data[1] = value;
	return drv_l1_i2c_bus_write(&gc5025_handle, data, 2);
	*/
#endif
}

/*
static INT32S gc5025_sccb_read(INT16U reg, INT8U *value)
{
#if SCCB_MODE == SCCB_GPIO
	INT16U data;

	if(drv_l2_sccb_read(gc5025_handle, reg, &data) >= 0) {
		*value = (INT8U)data;
		return STATUS_OK;
	} else {
		*value = 0xFF;
		return STATUS_FAIL;
	}
#elif SCCB_MODE == SCCB_HW_I2C
	INT8U data[2];

	data[0] = (reg >> 8) & 0xFF;
	data[1] = reg & 0xFF;
	if(drv_l1_i2c_bus_write(&gc5025_handle, data, 2) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}

	if(drv_l1_i2c_bus_read(&gc5025_handle, data, 1) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}

	*value = data[0];
#endif
	return STATUS_OK;
}*/

static INT32S gc5025_sccb_write_table(regval8_t *pTable)
{
    INT8U retry_cnt=0;
	while(1) {
		if(pTable->reg_num == 0xFF && pTable->value == 0xFF) {
			break;
		}

		DBG_MSG("0x%02x, 0x%02x\r\n", pTable->reg_num, pTable->value);
		if(gc5025_sccb_write(pTable->reg_num, pTable->value) < 0) {
			ERR_MSG("sccb write fail.\r\n");
            retry_cnt += 1;
			if(retry_cnt >= 10){
                retry_cnt = 0;
				return STATUS_FAIL;
			}
			continue;
		}

		pTable++;
	}

	DBG_MSG("\r\n\r\n");

	return STATUS_OK;
}
/*
static INT32S gc5025_sccb_read_table(regval8_t *pTable)
{
    INT8U *Rdata;
	while(1) {
		if(pTable->reg_num == 0xFF && pTable->value == 0xFF) {
			break;
		}

		if(gc5025_sccb_read(pTable->reg_num, Rdata) < 0) {
			DBG_PRINT("sccb read fail.\r\n");
			continue;
		}
		DBG_PRINT("0x%02x, 0x%02x = 0x%02x\r\n", pTable->reg_num, pTable->value, *Rdata);
		pTable++;
	}
	DBG_PRINT("\r\n\r\n");

	return STATUS_OK;
}
*/
/*****************************************************************************************
+++++++++++++++++AEC/AGC
*****************************************************************************************/
static void gc5025_set_shutter(INT16U shutter)
{
	// Update Shutter
	if(shutter > 8191) shutter = 8191;
	if(shutter <1) shutter = 1;
	shutter=shutter/2;
	shutter=shutter*2;
	//Update Shutter
	gc5025_sccb_write(0xfe,0x00);
	gc5025_sccb_write(0x03,(shutter>>8) & 0x3F);
	gc5025_sccb_write(0x04,shutter & 0xFF);
}

#define ANALOG_GAIN_1 64   // 1.00x
#define ANALOG_GAIN_2 92   // 1.445x

static INT16U gc5025_set_gain(INT16U gain)
{
	INT16U iReg,temp;

	iReg = gain;

	if(iReg < 0x40)
		iReg = 0x40;

	if((ANALOG_GAIN_1<= iReg)&&(iReg < ANALOG_GAIN_2))
	//if((ANALOG_GAIN_1<= iReg)&&(iReg < ANALOG_GAIN_1*2))
	{
		gc5025_sccb_write(0xfe,  0x00);
		gc5025_sccb_write(0xb6,  0x00);
		temp = iReg;
		gc5025_sccb_write(0xb1, temp>>6);
		gc5025_sccb_write(0xb2, (temp<<2)&0xfc);
	}
	else
	{
		gc5025_sccb_write(0xfe,  0x00);
		gc5025_sccb_write(0xb6,  0x01);
		temp = 64*iReg/ANALOG_GAIN_2;
		gc5025_sccb_write(0xb1, temp>>6);
		gc5025_sccb_write(0xb2, (temp<<2)&0xfc);
	}
	return gain;
}

extern INT32U g_flag_update;
extern int ev_step_index;

static int gc5025_set_xfps_exposure_time(sensor_exposure_t *si)
{
    //unsigned char t1, t2;
    int idx,iso_x;

    //DBG_PRINT("%s:%d\n", __FUNCTION__, __LINE__);
   iso_x = (drv_l2_CdspGetIso() & 0xF);
	if (iso_x <= 0)
		iso_x = 1;

    si->sensor_ev_idx += si->ae_ev_step;
    if(si->sensor_ev_idx >= si->max_ev_idx)
    	si->sensor_ev_idx = si->max_ev_idx;
    if(si->sensor_ev_idx < 0)
    	si->sensor_ev_idx = 0;

    #if RAW_MODE
    idx = (g_raw_mode_idx < 0) ? si->sensor_ev_idx * 3 : g_raw_mode_idx * 3;
    DBG_PRINT("%s:%d %d\n", __FUNCTION__, __LINE__, g_raw_mode_idx);
    #else
    idx = si->sensor_ev_idx * 3;
    #endif

    si ->time = p_expTime_table[idx];
    si ->analog_gain = (int)(p_expTime_table[idx+1]*iso_x);
    si ->digital_gain = 64;//(int)(p_expTime_table[idx+2]);
    //DBG_PRINT("[EV=%d, EvStep=%d]\r\n", si->sensor_ev_idx, si->ae_ev_step);
    // exposure time
    if(si ->time != pre_sensor_time)
    {
        pre_sensor_time = si->time;// = 840*2;
        gc5025_set_shutter(pre_sensor_time);
        //DBG_PRINT("gc5025_set_xfps_exposure_time: time = %d!!!\r\n", si->time);

    }

    //gain
    if(si ->analog_gain != pre_sensor_a_gain)
    {
        // gain
        pre_sensor_a_gain = si->analog_gain;// = 64*1.36*1.3;//92;
        gc5025_set_gain(pre_sensor_a_gain);
        //DBG_PRINT("gc5025_set_xfps_exposure_time: analog_gain = %d!!!\r\n", si->analog_gain);
    }

    return 0;
}

static void gc5025_set_ae(int ev_step)
{
    seInfo.ae_ev_step = ev_step;
    gc5025_set_xfps_exposure_time(&seInfo);
}

//__attribute__((unused))
static void sensor_register_ae_ctrl(INT32U *handle)
{
    *handle = (INT32U)gc5025_set_ae;
}

static void sensor_get_ae_info(sensor_exposure_t *si)
{
    memcpy(si, &seInfo, sizeof(sensor_exposure_t));
}

void sensor_set_max_lum(int max_lum)
{
    seInfo.max_ev_idx = seInfo.total_ev_idx - (128 - max_lum);
	DBG_PRINT("max_ev_idx:%d\r\n", seInfo.max_ev_idx);
}

/*****************************************************************************************
-------------------AEC/AGC
*****************************************************************************************/

#if MIPI_ISR_TEST
static void mipi_gc5025_handle(INT32U mipi_num, INT32U event)
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
}
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++

void gc5025_mipi_set_exp_freq(INT8U freq)
{
    DBG_PRINT("\r\n ==== %s == freq:%d == \r\n", __FUNCTION__, freq);
	if(freq == 50)
	{
        seInfo.sensor_ev_idx = GC5025_20FPS_50HZ_DAY_EV_IDX;
        seInfo.ae_ev_step = 0;
        seInfo.daylight_ev_idx= GC5025_20FPS_50HZ_DAY_EV_IDX;
        seInfo.night_ev_idx= GC5025_20FPS_50HZ_NIGHT_EV_IDX;
        seInfo.max_ev_idx = GC5025_20FPS_50HZ_MAX_EV_IDX;
        seInfo.total_ev_idx = GC5025_20FPS_50HZ_EXP_TIME_TOTAL;

        p_expTime_table = (int *)gc5025_20fps_exp_time_gain_50Hz;
        //DBG_PRINT(">>Set frequence 30fps 50Hz.\r\n");

	}
	else
	{
        DBG_PRINT(">>Not Support Set frequence 20fps 60Hz.\r\n");
	}
	/*
	else if(freq == 60)
	{
        seInfo.sensor_ev_idx = GC5025_20FPS_60HZ_INIT_EV_IDX;
        seInfo.ae_ev_step = 0;
        seInfo.daylight_ev_idx= GC5025_20FPS_60HZ_DAY_EV_IDX;
        seInfo.night_ev_idx= GC5025_20FPS_60HZ_NIGHT_EV_IDX;
        seInfo.max_ev_idx = GC5025_20FPS_60HZ_MAX_EV_IDX;
        seInfo.total_ev_idx = GC5025_20FPS_60HZ_EXP_TIME_TOTAL;

        p_expTime_table = (int *)gc5025_20fps_exp_time_gain_50Hz; //must be modify

        //DBG_PRINT(">>Set frequence 30fps 60Hz.\r\n");
	}
    */
	sensor_set_max_lum(g_FavTable.max_lum);
}

void GC5025_seinfo_init(void)
{
    seInfo.sensor_ev_idx = GC5025_20FPS_50HZ_INIT_EV_IDX-30;
	seInfo.ae_ev_step = 0;
	seInfo.daylight_ev_idx= GC5025_20FPS_50HZ_DAY_EV_IDX;
	seInfo.night_ev_idx= GC5025_20FPS_50HZ_NIGHT_EV_IDX;
	seInfo.max_ev_idx = GC5025_20FPS_50HZ_MAX_EV_IDX;
	seInfo.total_ev_idx = GC5025_20FPS_50HZ_EXP_TIME_TOTAL;

	p_expTime_table = (int *)gc5025_20fps_exp_time_gain_50Hz;

	pre_sensor_time = 10;
	pre_sensor_a_gain = 0x10;

	seInfo.time = 1;
	seInfo.analog_gain = 0x40;
	seInfo.digital_gain = 0x40;
}

static void gc5025_mipi_sensor_init(void)
{
	// reguest sccb
	gc5025_sccb_open();

	// reset sensor
	gc5025_sccb_write_table((regval8_t *)gc5025_reset_table);
    osDelay(10);

	// init sensor
    //gc5025_sccb_write_table((regval8_t *)gc5025_15fps_624M_2560x1944);
     gc5025_sccb_write_table((regval8_t *)gc5025_20fps_624M_2560x1944);
    //gc5025_sccb_write_table((regval8_t *)gc5025_25fps_672M_2560x1944);

//    osDelay(500);

    DBG_PRINT("Sensor gc5025 mipi init completed\r\n");
}


/**
 * @brief   initialization function
 * @param   sensor format parameters
 * @return 	none
 */
static void gc5025_cdsp_mipi_init(void)
{
	//ae init
	GC5025_seinfo_init();
	gc5025_mipi_set_exp_freq(50);
    //cdsp init
	drv_l2_cdsp_open();

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
	DBG_PRINT("Sensor gc5025 cdsp mipi[%d] init completed\r\n", MIPI_DEV_NO);
}

/**
 * @brief   un-initialization function
 * @param   sensor format parameters
 * @return 	none
 */
static void gc5025_cdsp_mipi_uninit(void)
{
	// cdsp disable
	drv_l2_cdsp_close();

	// mipi disable
	drv_l1_mipi_init(MIPI_DEV_NO, DISABLE, 0);

	// release sccb
	gc5025_sccb_close();
}

/**
 * @brief   stream start function
 * @param   info index
 *
 * @return 	none
 */
void gc5025_cdsp_mipi_stream_on(INT32U index, INT32U bufA, INT32U bufB)
{
	INT16U target_w, target_h, sensor_w, sensor_h;
	gpCdspFmt_t format;
    INT8U docropflag = 0;

	// set sensor size
	DBG_PRINT("%s = %d\r\n", __func__, index);

    //Enabel mipi clk, set mipi clk
    if(gc5025_mipi_cfg.pixel_clk_sel == GENERATE_PIXEL_CLK)
    {
        drv_l1_clock_set_peripheral_pre_dvi2(1);
        drv_l2_mipi_ctrl_set_clk(ENABLE, 2);
        osDelay(100);
    }

    // set cdsp format
	#if BINMODE_OUT
    format.image_source = C_CDSP_BINMODE + MIPI_DEV_NO;
    #elif MIPI_DEV_NO == 0
	format.image_source = C_CDSP_MIPI;
	#else
	format.image_source = C_CDSP_MIPI1;
	#endif

	//format.image_source = C_CDSP_BINMODE;//binmode path

	format.input_format =  gc5025_cdsp_mipi_ops.info[index].input_format;
	format.output_format = gc5025_cdsp_mipi_ops.info[index].output_format;
	target_w = gc5025_cdsp_mipi_ops.info[index].target_w;
	target_h = gc5025_cdsp_mipi_ops.info[index].target_h;
	if (format.image_source == C_CDSP_BINMODE || format.image_source == C_CDSP_BINMODE1)
	{
		//use binmode input, cdsp use target size, sensor size would larger than target size,
        //binmode use by2, by4, crop, to scale down input, so dont need cdsp scale size here
		format.hpixel = sensor_w = gc5025_cdsp_mipi_ops.info[index].target_w;
		format.vline = sensor_h = gc5025_cdsp_mipi_ops.info[index].target_h;
	}
	else
	{
		format.hpixel = sensor_w = gc5025_cdsp_mipi_ops.info[index].sensor_w;
		format.vline = sensor_h = gc5025_cdsp_mipi_ops.info[index].sensor_h;
	}
	format.hoffset = gc5025_cdsp_mipi_ops.info[index].hoffset;
	format.voffset = gc5025_cdsp_mipi_ops.info[index].voffset;
	format.sensor_timing_mode = gc5025_cdsp_mipi_ops.info[index].interface_mode;
	format.sensor_hsync_mode = gc5025_cdsp_mipi_ops.info[index].hsync_active;
	format.sensor_vsync_mode = gc5025_cdsp_mipi_ops.info[index].vsync_active;


    if(drv_l2_cdsp_set_fmt(&format, 0) < 0)
    {
		ERR_MSG("cdsp set fmt err!!!\r\n");
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
	drv_l2_cdsp_analog_gain_thr_init(GainThrTable);
    drv_l2_cdsp_enable((gpUserFav_t *)&g_FavTable, sensor_w, sensor_h, target_w, target_h, 0);
	drv_l2_cdsp_sat_contrast_reinit_thr();

	//mipi start
	drv_l1_mipi_set_parameter(MIPI_DEV_NO, &gc5025_mipi_cfg);
	#if MIPI_ISR_TEST
	drv_l1_mipi_isr_register(mipi_gc5025_handle);
	drv_l1_mipi_set_irq_enable(MIPI_DEV_NO, ENABLE, MIPI_INT_ALL);
	#endif

	//Enable MCLK & Init Sensor
    gc5025_mipi_sensor_init();

    // reset sensor ev idx
    seInfo.ae_ev_step = 0;
    gc5025_set_xfps_exposure_time(&seInfo);
//drv_l2_cdsp_save_raw();
  // drv_l2_isp_dram_yuv_read(0x1000000, 0x3000000, sensor_w, sensor_h, target_w, target_h);
   //drv_l2_isp_dram_raw_read(0x1000000, 0x2000000, sensor_w, sensor_h, target_w, target_h);
   // drv_l1_CdspSetRedoTriger(1);
	drv_l1_CdspSetDataSource(format.image_source);
	DBG_PRINT("gc5025 cdsp mipi stream on done.\r\n");
}

/**
 * @brief   stream stop function
 * @param   none
 * @return 	none
 */
static void gc5025_cdsp_mipi_stream_off(void)
{
	//drv_l2_cdsp_stream_off();
}

/**
 * @brief   get info function
 * @param   none
 * @return 	pointer to sensor information data
 */
static drv_l2_sensor_info_t* gc5025_cdsp_mipi_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1)) {
		return NULL;
	} else {
		return (drv_l2_sensor_info_t*)&gc5025_cdsp_mipi_ops.info[index];
	}
}

/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t gc5025_cdsp_mipi_ops =
{
	SOURCE_CDSP_MIPI,					/* sensor name */
	gc5025_cdsp_mipi_init,
	gc5025_cdsp_mipi_uninit,
	gc5025_cdsp_mipi_stream_on,
	gc5025_cdsp_mipi_stream_off,
	gc5025_cdsp_mipi_get_info,
	gc5025_set_ae,
	sensor_get_ae_info,
    sensor_set_max_lum,
    gc5025_mipi_set_exp_freq,
	{
		/* 1st info */
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SBGGR10,		/* input format */
			V4L2_PIX_FMT_YUYV,			/* output format */
			20,							/* FPS in sensor */
			GC5025_OUT_WIDTH,			/* target width */
			GC5025_OUT_HEIGHT,  		/* target height */
			GC5025_WIDTH,				/* sensor width */
			GC5025_HEIGHT,  			/* sensor height */
			0,							/* sensor h offset */
			0,							/* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
		/* 2nd info */
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SBGGR10,		/* input format */
			V4L2_PIX_FMT_YUYV,			/* output format */
			20,							/* FPS in sensor */
			GC5025_OUT_WIDTH,			/* target width */
			GC5025_OUT_HEIGHT,			/* target height */
			GC5025_WIDTH,				/* sensor width */
			GC5025_HEIGHT, 				/* sensor height */
			0,							/* sensor h offset */
			0,							/* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
	}
};

#endif //(defined _SENSOR_GC5025_CDSP_MIPI) && (_SENSOR_GC5025_CDSP_MIPI == 1)

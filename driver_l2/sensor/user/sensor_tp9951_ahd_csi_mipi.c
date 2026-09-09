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
/*******************************************************
	Include file
*******************************************************/
#include "drv_l2.h"

#if (defined _SENSOR_TP9951_AHD_CSI_MIPI) && (_SENSOR_TP9951_AHD_CSI_MIPI == 1)


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define MIPI_LANE_NO		MIPI_2_LANE // 1 or 2 lane
#define MIPI_DEV_NO         0			//0:MIPI_0(IOC8~11) or 1:MIPI_1(IOE0~9)

#define TP9951_MIPI_ID			0x88

#if 0
/*ISP Input*/
#define TP9951_MIPI_WIDTH		1280
#define TP9951_MIPI_HEIGHT		720

/*ISP Output*/
#define TP9951_MIPI_OUT_WIDTH	1280
#define TP9951_MIPI_OUT_HEIGHT	720
#else
/*ISP Input*/
#define TP9951_MIPI_WIDTH		1920
#define TP9951_MIPI_HEIGHT		1080

/*ISP Output*/
#define TP9951_MIPI_OUT_WIDTH	1920
#define TP9951_MIPI_OUT_HEIGHT	1080
#endif


#define TP9951_MIPI_CSI_DEV		CSI_0_PATH

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define TP9951_MIPI_I2C_MODE	            SENSOR_I2C_MODE_SW

#if TP9951_MIPI_I2C_MODE == SENSOR_I2C_MODE_SW
    #define TP9951_MIPI_GPIO_SCL		    IO_B4
    #define TP9951_MIPI_GPIO_SDA		    IO_B5
    #define TP9951_MIPI_GPIO_DRIVING	    IOB_DRV_30mA
#else
    #define TP9951_MIPI_I2C_DEVICE		I2C_0
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct regval8_s
{
	INT8U	reg_num;
	INT8U	value;
} regval8_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
#if TP9951_MIPI_I2C_MODE == SENSOR_I2C_MODE_SW
sccb_config_t tp9951_mipi_sccb_config =
{
	TP9951_MIPI_GPIO_SCL,			//scl_port
	TP9951_MIPI_GPIO_DRIVING,		//scl_drv
	TP9951_MIPI_GPIO_SDA,			//sda_port
	TP9951_MIPI_GPIO_DRIVING,		//sda_drv
	IO_F15,						//pwdn_port
	0,	                        //pwdn_drv
	0,			                //have_pwdn
	8,			                //RegBits
	8,			                //DataBits
	TP9951_MIPI_ID,	            //slaveAddr
	0x2000,	                    //timeout
	100			                //clock_rate
};

static void	 *tp9951_mipi_handle;

#elif TP9951_MIPI_I2C_MODE == SENSOR_I2C_MODE_HW
static drv_l1_i2c_bus_handle_t	tp9951_mipi_handle;
#endif

static mipi_config_t  tp9951_mipi_cfg =
{
	DISABLE,			/* low power, 0:disable, 1:enable */
	D_PHY_SAMPLE_POS,	/* byte clock edge, 0:posedge, 1:negedge */
#if MIPI_LANE_NO == MIPI_1_LANE
	MIPI_1_LANE,		/* lane */
#elif MIPI_LANE_NO == MIPI_2_LANE
	MIPI_2_LANE,		/* lane */
#endif
	D_PHY_BYTE_CLK,		/* byte clock source */
	MIPI_USER_DEFINE,	/* data mode, 0:auto detect, 1:user define */
	MIPI_YUV422,			/* data type, valid when data mode is 1*/
	MIPI_DATA_TO_CSI,	/* data type, 1:data[7:0]+2':00 to cdsp, 0: 2'00+data[7:0] to csi */
	0,					//NULL,			/* RSD 2 */
	TP9951_MIPI_WIDTH,			/* width, 0~0xFFFF */
	TP9951_MIPI_HEIGHT,			/* height, 0~0xFFFF */
	0,					//2, 				/* back porch, 0~0xF */ //If Set 4 have show paper color at left side.u2.20160119
	0,					//2,				/* front porch, 0~0xF */
	ENABLE,				/* blanking_line_en, 0:disable, 1:enable */
	0,					//NULL,			/* RSD 3 */
	ENABLE,				/* ecc, 0:disable, 1:enable */
	MIPI_ECC_ORDER3,	/* ecc order */
	350,				//400,//250,			/* data mask, unit is ns */
	MIPI_CHECK_HS_SEQ	//MIPI_CHECK_LP_00	/* check hs sequence or LP00 for clock lane */
};

static regval8_t tp9951_mipi_1080P_PAL_25fps[]=
{
{0x40, 0x00},
{0x06, 0x12},
{0x42, 0x00},
{0x4e, 0x00},
{0x54, 0x00},
{0x41, 0x00},	//video MUX select
////////////

{0x02, 0x40},
{0x07, 0xc0},
{0x0b, 0xc0},
{0x0c, 0x03},
{0x0d, 0x50},

{0x15, 0x03},
{0x16, 0xd2},
{0x17, 0x80},
{0x18, 0x29},
{0x19, 0x38},
{0x1a, 0x47},

{0x1c, 0x0a},
{0x1d, 0x50},

{0x20, 0x30},
{0x21, 0x84},
{0x22, 0x36},
{0x23, 0x3c},

{0x2b, 0x60},
{0x2c, 0x2a},
{0x2d, 0x30},
{0x2e, 0x70},

{0x30, 0x48},
{0x31, 0xbb},
{0x32, 0x2e},
{0x33, 0x90},

{0x35, 0x05},
{0x38, 0x00},
{0x39, 0x1C},

////////////
{0x02, 0x44},

{0x0d, 0x73},

{0x15, 0x01},
{0x16, 0xf0},
{0x18, 0x2a},

{0x20, 0x3c},
{0x21, 0x46},

{0x25, 0xfe},
{0x26, 0x0d},

{0x2c, 0x3a},
{0x2d, 0x54},
{0x2e, 0x40},

{0x30, 0xa5},
{0x31, 0x86},
{0x32, 0xfb},
{0x33, 0x60},
/////////////

{0x40, 0x08},
{0x02, 0x79},
{0x03, 0x71},
{0x04, 0x71},

{0x20, 0x00},
#if MIPI_LANE_NO	==	MIPI_1_LANE
{0x21, 0x11},		//1lan
#else
{0x21, 0x12},  	//2lan
#endif

{0x23, 0x9e},
////////////
{0x12, 0x54},
{0x13, 0xef},
#if MIPI_LANE_NO	==	MIPI_1_LANE
{0x14, 0x00},		// 1lan
#else
{0x14, 0x41},		//2lan
#endif
{0x15, 0x02},
{0x2a, 0x08},
{0x2b, 0x04},
{0x2c, 0x0c},
{0x2e, 0x02},

///////////
{0x10, 0xa0},
{0x10, 0x20},
{0x28, 0x02},
{0x28, 0x00},
{0x40, 0x00},
{0xff, 0xff}

};

static regval8_t tp9951_mipi_1080P_NTSC_30fps[]=
{
{0x40, 0x00},
{0x06, 0x12},
{0x42, 0x00},
{0x4e, 0x00},
{0x54, 0x00},
{0x41, 0x00},	//video MUX select
////////////
{0x02, 0x40},
{0x07, 0xc0},
{0x0b, 0xc0},
{0x0c, 0x03},
{0x0d, 0x50},

{0x15, 0x03},
{0x16, 0xd2},
{0x17, 0x80},
{0x18, 0x29},
{0x19, 0x38},
{0x1a, 0x47},
{0x1c, 0x08},
{0x1d, 0x98},

{0x20, 0x30},
{0x21, 0x84},
{0x22, 0x36},
{0x23, 0x3c},

{0x2b, 0x60},
{0x2c, 0x2a},
{0x2d, 0x30},
{0x2e, 0x70},

{0x30, 0x48},
{0x31, 0xbb},
{0x32, 0x2e},
{0x33, 0x90},

{0x35, 0x05},
{0x38, 0x00},
{0x39, 0x1C},
///////////
{0x02, 0x44},

{0x0d, 0x72},

{0x15, 0x01},
{0x16, 0xf0},
{0x18, 0x2a},

{0x20, 0x38},
{0x21, 0x46},

{0x25, 0xfe},
{0x26, 0x0d},

{0x2c, 0x3a},
{0x2d, 0x54},
{0x2e, 0x40},

{0x30, 0xa5},
{0x31, 0x95},
{0x32, 0xe0},
{0x33, 0x60},

/////////////

{0x40, 0x08},
{0x02, 0x79},
{0x03, 0x71},
{0x04, 0x71},

{0x20, 0x00},
#if MIPI_LANE_NO	==	MIPI_1_LANE
{0x21, 0x11},		//1lan
#else
{0x21, 0x12},  	//2lan
#endif

{0x23, 0x9e},
////////////
{0x12, 0x54},
{0x13, 0xef},
#if MIPI_LANE_NO	==	MIPI_1_LANE
{0x14, 0x00},		// 1lan
#else
{0x14, 0x41},		//2lan
#endif
{0x15, 0x02},
{0x2a, 0x08},
{0x2b, 0x04},
{0x2c, 0x0c},
{0x2e, 0x02},

///////////
{0x10, 0xa0},
{0x10, 0x20},
{0x28, 0x02},
{0x28, 0x00},
{0x40, 0x00},
{0xff, 0xff}
};

static regval8_t tp9950_mipi_720p_PAL_25fps[]=
{
	{ 0x40, 0x08 }, /* MIPI page */
	{ 0x01, 0xf8 },
	{ 0x02, 0x01 },
	{ 0x08, 0x03 },
	{ 0x20, 0x12 },
	{ 0x39, 0x00 },
	{ 0x40, 0x00 }, /* decoder page */
	{ 0x4c, 0x40 },
	{ 0x4e, 0x00 },
	{ 0x27, 0x2d },
	{ 0xfd, 0x80 },
	{ 0x02, 0x42 },
	{ 0x07, 0xc0 },
	{ 0x0b, 0xc0 },
	{ 0x0c, 0x13 },
	{ 0x0d, 0x50 },
	{ 0x15, 0x13 },
	{ 0x16, 0x15 },
	{ 0x17, 0x00 },
	{ 0x18, 0x19 },
	{ 0x19, 0xd0 },
	{ 0x1a, 0x25 },
	{ 0x1c, 0x07 }, /* 1280*720, 25fps */
	{ 0x1d, 0xbc }, /* 1280*720, 25fps */
	{ 0x20, 0x30 },
	{ 0x21, 0x84 },
	{ 0x22, 0x36 },
	{ 0x23, 0x3c },
	{ 0x2b, 0x60 },
	{ 0x2c, 0x0a },
	{ 0x2d, 0x30 },
	{ 0x2e, 0x70 },
	{ 0x30, 0x48 },
	{ 0x31, 0xbb },
	{ 0x32, 0x2e },
	{ 0x33, 0x90 },
	{ 0x35, 0x25 },
	{ 0x38, 0x00 },
	{ 0x39, 0x18 },
	/* STD_HDA */
	{ 0x02, 0x46 },
	{ 0x0d, 0x71 },
	{ 0x20, 0x40 },
	{ 0x21, 0x46 },
	{ 0x25, 0xfe },
	{ 0x26, 0x01 },
	{ 0x2c, 0x3a },
	{ 0x2d, 0x5a },
	{ 0x2e, 0x40 },
	{ 0x30, 0x9e },
	{ 0x31, 0x20 },
	{ 0x32, 0x10 },
	{ 0x33, 0x90 },
	/* STD_HDA */

	{ 0x40, 0x08 },
	{ 0x23, 0x02 },
	{ 0x13, 0x24 },
	{ 0x14, 0x46 },
	{ 0x15, 0x09 },
	{ 0x25, 0x08 },
	{ 0x26, 0x01 },
	{ 0x27, 0x0e },
	{ 0x10, 0x88 },
	{ 0x10, 0x08 },

 	{ 0x23, 0x00 },
	{ 0x40, 0x00 },

	{0xff, 0xff}
};
//tp9950 720p30 NTSC init code
static regval8_t tp9950_mipi_720p_NTSC_30fps[]=
{
	{ 0x40, 0x08 }, /* MIPI page */
	{ 0x01, 0xf8 },
	{ 0x02, 0x01 },
	{ 0x08, 0x03 },
	{ 0x20, 0x12 },
	{ 0x39, 0x00 },
	{ 0x40, 0x00 }, /* decoder page */
	{ 0x4c, 0x40 },
	{ 0x4e, 0x00 },
	{ 0x27, 0x2d },
	{ 0xfd, 0x80 },
	{ 0x02, 0x42 },
	{ 0x07, 0xc0 },
	{ 0x0b, 0xc0 },
	{ 0x0c, 0x13 },
	{ 0x0d, 0x50 },
	{ 0x15, 0x13 },
	{ 0x16, 0x15 },
	{ 0x17, 0x00 },
	{ 0x18, 0x19 },
	{ 0x19, 0xd0 },
	{ 0x1a, 0x25 },
	{ 0x1c, 0x06 }, /* 1280*720, 30fps */
	{ 0x1d, 0x72 }, /* 1280*720, 30fps */
	{ 0x20, 0x30 },
	{ 0x21, 0x84 },
	{ 0x22, 0x36 },
	{ 0x23, 0x3c },
	{ 0x2b, 0x60 },
	{ 0x2c, 0x0a },
	{ 0x2d, 0x30 },
	{ 0x2e, 0x70 },
	{ 0x30, 0x48 },
	{ 0x31, 0xbb },
	{ 0x32, 0x2e },
	{ 0x33, 0x90 },
	{ 0x35, 0x25 },
	{ 0x38, 0x00 },
	{ 0x39, 0x18 },

	/* STD_HDA */
	{ 0x02, 0x46 },
	{ 0x0d, 0x70 },
	{ 0x20, 0x40 },
	{ 0x21, 0x46 },
	{ 0x25, 0xfe },
	{ 0x26, 0x01 },
	{ 0x2c, 0x3a },
	{ 0x2d, 0x5a },
	{ 0x2e, 0x40 },
	{ 0x30, 0x9d },
	{ 0x31, 0xca },
	{ 0x32, 0x01 },
	{ 0x33, 0xd0 },
	/* STD_HDA */

	{ 0x40, 0x08 },
	{ 0x23, 0x02 },
	{ 0x13, 0x24 },
	{ 0x14, 0x46 },
	{ 0x15, 0x09 },
	{ 0x25, 0x08 },
	{ 0x26, 0x01 },
	{ 0x27, 0x0e },
	{ 0x10, 0x88 },
	{ 0x10, 0x08 },

	{ 0x23, 0x00 },
	{ 0x40, 0x00 },

	{0xff, 0xff}
};

//tp9950 1080P25 PAL  init code
static regval8_t tp9950_mipi_1080p_PAL_25fps[]=
{
{ 0x40, 0x08 },/* MIPI page */
{ 0x01, 0xf8 },
{ 0x02, 0x01 },
{ 0x08, 0x03 },
{ 0x20, 0x12 },
{ 0x39, 0x00 },
{ 0x40, 0x00 },/* decoder page */
{ 0x4c, 0x40 },
{ 0x4e, 0x00 },
{ 0x27, 0x2d },
{ 0xfd, 0x80 },
{ 0x02, 0x40 },
{ 0x07, 0xc0 },
{ 0x0b, 0xc0 },
{ 0x0c, 0x03 },
{ 0x0d, 0x50 },
{ 0x15, 0x03 },
{ 0x16, 0xCE },///{ 0x16, 0xd2 }
{ 0x17, 0x80 },
{ 0x18, 0x29 },
{ 0x19, 0x38 },
{ 0x1a, 0x47 },
{ 0x1c, 0x0a }, /* 1920*1080, 25fps */
{ 0x1d, 0x50 },
{ 0x20, 0x30 },
{ 0x21, 0x84 },
{ 0x22, 0x36 },
{ 0x23, 0x3c },
{ 0x2b, 0x60 },
{ 0x2c, 0x0a },
{ 0x2d, 0x30 },
{ 0x2e, 0x70 },
{ 0x30, 0x48 },
{ 0x31, 0xbb },
{ 0x32, 0x2e },
{ 0x33, 0x90 },
{ 0x35, 0x05 },
{ 0x38, 0x00 },
{ 0x39, 0x1C },
{ 0x02, 0x44 },
{ 0x0d, 0x73 },
{ 0x15, 0x01 },
{ 0x16, 0xf0 },
{ 0x18, 0x2a },
{ 0x20, 0x3c },
{ 0x21, 0x46 },
{ 0x25, 0xfe },
{ 0x26, 0x0d },
{ 0x2c, 0x3a },
{ 0x2d, 0x54 },
{ 0x2e, 0x40 },
{ 0x30, 0xa5 },
{ 0x31, 0x86 },
{ 0x32, 0xfb },
{ 0x33, 0x60 },
{ 0x40, 0x08 },
{ 0x23, 0x02 },
{ 0x13, 0x04 },
{ 0x14, 0x46 },
{ 0x15, 0x09 },
#if 1
{ 0x25, 0x10 },///{ 0x25, 0x10 }
{ 0x26, 0x02 },///{ 0x26, 0x02 }
{ 0x27, 0x16 },///{ 0x27, 0x16 }
#else
{ 0x25, 0x0A },///{ 0x25, 0x10 }
{ 0x26, 0x06 },///{ 0x26, 0x02 }
{ 0x27, 0x10 },///{ 0x27, 0x16 }
#endif
{ 0x10, 0x88 },
{ 0x10, 0x08 },

{ 0x23, 0x00 },
{ 0x40, 0x00 },
{0xff, 0xff}
};

//tp9950 1080P30 NTSC  init code
static regval8_t tp9950_mipi_1080p_NTSC_30fps[]=
{
	{ 0x40, 0x08 }, /* MIPI page */
	{ 0x01, 0xf8 },
	{ 0x02, 0x01 },
	{ 0x08, 0x03 },
	{ 0x20, 0x12 },
	{ 0x39, 0x00 },
	{ 0x40, 0x00 }, /* decoder page */
	{ 0x4c, 0x40 },
	{ 0x4e, 0x00 },
	{ 0x27, 0x2d },
	{ 0xfd, 0x80 },
	{ 0x02, 0x40 },
	{ 0x07, 0xc0 },
	{ 0x0b, 0xc0 },
	{ 0x0c, 0x03 },
	{ 0x0d, 0x50 },
	{ 0x15, 0x03 },
	{ 0x16, 0xd2 },
	{ 0x17, 0x80 },
	{ 0x18, 0x29 },
	{ 0x19, 0x38 },
	{ 0x1a, 0x47 },
	{ 0x1c, 0x08 },  /* 1920*1080, 30fps */
	{ 0x1d, 0x98 },
	{ 0x20, 0x30 },
	{ 0x21, 0x84 },
	{ 0x22, 0x36 },
	{ 0x23, 0x3c },
	///{ 0x2a, 0x3c }, /* blue screen */
	{ 0x2b, 0x60 },
	{ 0x2c, 0x0a },
	{ 0x2d, 0x30 },
	{ 0x2e, 0x70 },
	{ 0x30, 0x48 },
	{ 0x31, 0xbb },
	{ 0x32, 0x2e },
	{ 0x33, 0x90 },
	{ 0x35, 0x05 },
	{ 0x38, 0x00 },
	{ 0x39, 0x1C },
	{ 0x02, 0x44 },
	{ 0x0d, 0x72 },
	{ 0x15, 0x01 },
	{ 0x16, 0xf0 },
	{ 0x18, 0x2a },
	{ 0x20, 0x38 },
	{ 0x21, 0x46 },
	{ 0x25, 0xfe },
	{ 0x26, 0x0d },
	{ 0x2c, 0x3a },
	{ 0x2d, 0x54 },
	{ 0x2e, 0x40 },
	{ 0x30, 0xa5 },
	{ 0x31, 0x95 },
	{ 0x32, 0xe0 },
	{ 0x33, 0x60 },
	{ 0x40, 0x08 },
	{ 0x23, 0x02 },
	{ 0x13, 0x04 },
	{ 0x14, 0x46 },
	{ 0x15, 0x09 },
	{ 0x25, 0x10 },
	{ 0x26, 0x02 },
	{ 0x27, 0x16 },
	{ 0x10, 0x88 },
	{ 0x10, 0x08 },
	{ 0x23, 0x00 },
	{ 0x40, 0x00 },
	 {0xff, 0xff}
};
static INT32S tp9951_mipi_sccb_open(void)
{
#if TP9951_MIPI_I2C_MODE == SENSOR_I2C_MODE_SW
	tp9951_mipi_handle = drv_l2_sccb_open_ext(&tp9951_mipi_sccb_config);
	if(tp9951_mipi_handle == 0)
	{
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#elif TP9951_MIPI_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_init(TP9951_MIPI_I2C_DEVICE);
	tp9951_mipi_handle.devNumber = TP9951_MIPI_I2C_DEVICE;
	tp9951_mipi_handle.slaveAddr = TP9951_MIPI_ID;
	tp9951_mipi_handle.clkRate = 100;
#endif
	return STATUS_OK;
}

static void tp9951_mipi_sccb_close(void)
{
#if TP9951_MIPI_I2C_MODE == SENSOR_I2C_MODE_SW
	if(tp9951_mipi_handle)
	{
		drv_l2_sccb_close(tp9951_mipi_handle);
		tp9951_mipi_handle = NULL;
	}

#elif TP9951_MIPI_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_uninit(TP9951_I2C_DEVICE);
	tp9951_mipi_handle.slaveAddr = 0;
	tp9951_mipi_handle.clkRate = 0;
#endif
}

static INT32S tp9951_mipi_sccb_write(INT16U reg, INT8U value)
{
#if TP9951_MIPI_I2C_MODE == SENSOR_I2C_MODE_SW
	return drv_l2_sccb_write(tp9951_mipi_handle, reg, value);

#elif TP9951_MIPI_I2C_MODE == SENSOR_I2C_MODE_HW
	return drv_l1_reg_1byte_data_1byte_write(&tp9951_mipi_handle, reg, value);
#endif
}


static INT32S tp9951_mipi_sccb_read(INT16U reg,INT16U *value)
{
#if TP9951_MIPI_I2C_MODE == SENSOR_I2C_MODE_SW
	INT16U	data;

	if(drv_l2_sccb_read(tp9951_mipi_handle, reg, &data) >= 0)
	{
		*value = (INT16U) data;
		//DBG_PRINT("0x%02x, 0x%02x\r\n", reg, *value);
		return STATUS_OK;
	}
	else
	{
		*value = 0xFF;
		//DBG_PRINT("0x%02x, 0x%02x\r\n", reg, *value);
		return STATUS_FAIL;
	}

#elif TP9951_MIPI_I2C_MODE == SENSOR_I2C_MODE_HW
	INT8U	value = 0;
	if(drv_l1_reg_1byte_data_1byte_read(&tp9951_mipi_handle, reg, &value) >= 0)
	{
		DBG_PRINT("0x%02x, 0x%02x\r\n", reg, value);
		return STATUS_OK;
	}
	else
	{
		DBG_PRINT("sccb_read fail...\r\n");
		return STATUS_FAIL;
	}

#endif
}


static INT32S tp9951_mipi_sccb_write_table(regval8_t *pTable)
{
	//unsigned short tt;
	while(1)
	{
		if(pTable->reg_num == 0xFF && pTable->value == 0xFF)
		{
			break;
		}
        //DBG_PRINT("reg = 0x%x, val = 0x%x\r\n", pTable->reg_num, pTable->value);
		if(tp9951_mipi_sccb_write(pTable->reg_num, pTable->value) < 0)
		{
			DBG_PRINT("%s: sccb write fail: reg = 0x%x, val = 0x%x\r\n", __FUNCTION__, pTable->reg_num, pTable->value);
			continue;
		}

        //  osDelay(1);
        //  tp9951_sccb_read(pTable->reg_num,&tt);
		//  if(tt!= pTable->value)
		//	DBG_PRINT("i2c reg[0x%04x] = 0x%04x, read = 0x%04x\r\n", pTable->reg_num, pTable->value, tt);

		pTable++;
	}

	return STATUS_OK;
}


void tp9951_csi_mipi_init(void)
{
	DBG_PRINT("%s\r\n", __func__);
	// reguest sccb
	tp9951_mipi_sccb_open();

    drv_l1_csi_init(TP9951_MIPI_CSI_DEV);

	#ifdef REAR_SENSOR_RESET
	gpio_set_port_attribute(REAR_SENSOR_RESET, ATTRIBUTE_HIGH);
	gpio_init_io(REAR_SENSOR_RESET, GPIO_OUTPUT);
	gpio_write_io(REAR_SENSOR_RESET, DATA_HIGH);
	osDelay(10);
	gpio_write_io(REAR_SENSOR_RESET, DATA_LOW);
	osDelay(20);
	gpio_write_io(REAR_SENSOR_RESET, DATA_HIGH);
	osDelay(10);
	#endif
    drv_l1_mipi_init(MIPI_DEV_NO, ENABLE, 0);
}

static void tp9951_mipi_csi_sensor_init(void)
{
	// reset sensor
	//tp9951_sccb_write_table((regval16_t *) tp9951_reset_table);

	// init sensor
	tp9951_mipi_sccb_write_table((regval8_t *) tp9951_mipi_1080P_PAL_25fps);
	//tp9951_sccb_write_table((regval16_t *)tp9951_mipi_init);

	DBG_PRINT("Sensor tp9951 sensor mipi init completed\r\n");
}

/**
 * @brief   un-initialization function
 * @param   sensor format parameters
 * @return 	none
 */
void tp9951_csi_mipi_uninit(void)
{
	// csi disable
    drv_l1_csi_uninit(TP9951_MIPI_CSI_DEV);
	// mipi disable
    drv_l1_mipi_init(MIPI_DEV_NO, DISABLE, 0);
	// release sccb
	tp9951_mipi_sccb_close();
	// disable mclk
	drv_l2_sensor_set_mclkout(MCLK_NONE, 0);
}

/**
 * @brief   stream start function
 * @param   info index
 *
 * @return 	none
 */
void tp9951_csi_mipi_stream_on(INT32U index, INT32U bufA, INT32U bufB)
{
//	DBG_PRINT("%s = %d\r\n", __func__, index);
	gpCSIPara_t csi_Para;

	gp_memset((INT8S *)&csi_Para, 0, sizeof(gpCSIPara_t));
	//Enabel mipi clk, set mipi clk
    if(tp9951_mipi_cfg.pixel_clk_sel == GENERATE_PIXEL_CLK)
    {
        drv_l2_mipi_ctrl_set_clk(ENABLE, 4);
        osDelay(10);
    }
	/* set input & output format */
	csi_Para.csi_fmt.input_format = tp9951_csi_mipi_ops.info[index].input_format;
	csi_Para.csi_fmt.output_format = tp9951_csi_mipi_ops.info[index].output_format;

	csi_Para.csi_tim.clki_inv = 1;
	csi_Para.csi_fmt.fifo_size = 0;
    csi_Para.csi_fmt.interface_mode = tp9951_csi_mipi_ops.info[index].interface_mode;
    csi_Para.csi_fmt.preview_mode = CSI_CAPTURE_MODE;
	csi_Para.csi_fmt.clk_stopb = ENABLE;
	csi_Para.csi_tim.fget_mode = ENUM_CSI_RISING_EDGE;
    csi_Para.csi_tim.hrst_mode = ENUM_CSI_RISING_EDGE;
    csi_Para.csi_tim.vadd_mode = ENUM_CSI_RISING_EDGE;
    csi_Para.csi_tim.vrst_mode = ENUM_CSI_RISING_EDGE;
    csi_Para.csi_tim.d_type = ENUM_CSI_LATCH_DELAY_1_CLOCK;
    csi_Para.h_start = csi_Para.v_start = 0x0;
    csi_Para.h_end = csi_Para.v_end = 0xfff;

	csi_Para.target_w = tp9951_csi_mipi_ops.info[index].target_w;
	csi_Para.target_h = tp9951_csi_mipi_ops.info[index].target_h;
	csi_Para.hratio = 0;		// No Scale
	csi_Para.vratio = 0;		// No Scale

    if(drv_l2_csi_set_fmt(TP9951_MIPI_CSI_DEV,&csi_Para.csi_fmt) < 0)
    {
		DBG_PRINT("csi set fmt err!!!\r\n");
	}

	if(bufA) {
		drv_l2_csi_stream_on(TP9951_MIPI_CSI_DEV,(gpCSIPara_t *)&csi_Para, bufA, bufB);
	} else {
		DBG_PRINT("Input frame buffer address error, fail to start %s\r\n", tp9951_csi_mipi_ops.name);
		tp9951_csi_mipi_ops.stream_stop();
	}
		//mipi start
	if(drv_l1_mipi_set_parameter(MIPI_DEV_NO, &tp9951_mipi_cfg) < 0)
	{
		DBG_PRINT("MIPI0 init fail !!!\r\n");
	}
    drv_l2_sensor_set_csi_source(SENSOR_INTERFACE_MIPI, TP9951_MIPI_CSI_DEV, MIPI_DEV_NO);
	/*Init Sensor*/
	tp9951_mipi_csi_sensor_init();

}

/**
 * @brief   stream stop function
 * @param   none
 * @return 	none
 */
void tp9951_csi_mipi_stream_off(void)
{
	/* Disable interrupt */
	drv_l2_csi_stream_off(TP9951_MIPI_CSI_DEV);
	DBG_PRINT("%s:no function \r\n", __func__);
}

/**
 * @brief   get info function
 * @param   none
 * @return 	pointer to sensor information data
 */
drv_l2_sensor_info_t *tp9951_csi_mipi_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1))
	{
		return NULL;
	}
	else
	{
		return (drv_l2_sensor_info_t *) &tp9951_csi_mipi_ops.info[index];
	}
}

/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t	tp9951_csi_mipi_ops =
{
    #if(TP9951_CSI_DEV == CSI_0_PATH)
    SOURCE_CSI0_MIPI,               /* sensor name */
    #else
    SOURCE_CSI1_MIPI,               /* sensor name */
    #endif
	tp9951_csi_mipi_init,
	tp9951_csi_mipi_uninit,
	tp9951_csi_mipi_stream_on,
	tp9951_csi_mipi_stream_off,
	tp9951_csi_mipi_get_info,
	NULL,
	NULL,
	NULL,
	NULL,
	{
		/* 1nd info */
		{
		    MCLK_24M,		/* CSI clock */
			CSI_IN_UYVY,			/* input format */
			CSI_OUT_YUYV,			/* output format */
			25,						/* FPS in sensor */
			TP9951_MIPI_OUT_WIDTH,		/* target width */
			TP9951_MIPI_OUT_HEIGHT,		/* target height */
			TP9951_MIPI_WIDTH,			/* sensor width */
			TP9951_MIPI_HEIGHT,			/* sensor height */
			0, 		                /* sensor h offset */
			0,   				    /* sensor v offset */
			CSI_HREF,		        /* input interface */
			CSI_NON_INTERLACE,		/* interlace mode */
			ENUM_CSI_RISING_EDGE,	/* hsync pin active level */
			ENUM_CSI_RISING_EDGE	/* vsync pin active level */
		},

		/* 2st info */
		{
			MCLK_24M,//MASTER_CLOCK_24MHZ,		/* CSI clock */
			CSI_IN_UYVY,			/* input format */
			CSI_OUT_YUYV,			/* output format */
			25,						/* FPS in sensor */
			TP9951_MIPI_OUT_WIDTH,		/* target width */
			TP9951_MIPI_OUT_HEIGHT,		/* target height */
			TP9951_MIPI_WIDTH,			/* sensor width */
			TP9951_MIPI_HEIGHT,			/* sensor height */
			0,						/* sensor h offset */
			0,						/* sensor v offset */
			CSI_HREF,				/* input interface */
			CSI_NON_INTERLACE,		/* interlace mode */
			ENUM_CSI_RISING_EDGE,	/* hsync pin active level */
			ENUM_CSI_RISING_EDGE	/* vsync pin active level */
		},

	 }
};
#endif //(defined _SENSOR_TP9951_AHD_CSI_MIPI) && (_SENSOR_TP9951_AHD_CSI_MIPI == 1)

/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2014 by Generalplus Inc.                         *
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

#if (defined _SENSOR_RN6752_AHD_CSI_DVP) && (_SENSOR_RN6752_AHD_CSI_DVP == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#if 1

#define RN6752_ID					0x5A//0x58

/*ISP Input*/
#define RN6752_WIDTH				1280
#define RN6752_HEIGHT				720

/*ISP Output*/
#define RN6752_OUT_WIDTH			1280
#define RN6752_OUT_HEIGHT			720

#else

#define RN6752_ID					0x58 //RM6752

/*ISP Input*/
#define RN6752_WIDTH				1920
#define RN6752_HEIGHT				1080

/*ISP Output*/
#define RN6752_OUT_WIDTH			1920
#define RN6752_OUT_HEIGHT			1080

#endif

#define RN6752_CSI_DEV			    CSI_0_PATH

#define RN6752_I2C_MODE	            SENSOR_I2C_MODE_SW

#if RN6752_I2C_MODE==SENSOR_I2C_MODE_SW
    #define RN6752_GPIO_SCL		    IO_B4
    #define RN6752_GPIO_SDA		    IO_B5
    #define RN6752_GPIO_DRIVING		IOB_DRV_30mA
#else
    #define RN6752_I2C_DEVICE		I2C_0
#endif

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

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
#if RN6752_I2C_MODE==SENSOR_I2C_MODE_SW
sccb_config_t rn6752_sccb_config =
{
	RN6752_GPIO_SCL,		//scl_port
	RN6752_GPIO_DRIVING,	//scl_drv
	RN6752_GPIO_SDA,		//sda_port
	RN6752_GPIO_DRIVING,	//sda_drv
	IO_F15,			        //pwdn_port
	RN6752_GPIO_DRIVING,   //pwdn_drv
	0,			//have_pwdn
	8,			//RegBits
	8,			//DataBits
	RN6752_ID,	//slaveAddr
	0x20000,	//timeout
	100			//clock_rate
};

static void	 *rn6752_handle;

#elif RN6752_I2C_MODE==SENSOR_I2C_MODE_HW
static drv_l1_i2c_bus_handle_t	rn6752_handle;
#endif


static regval8_t rn6752_dvp_init[]=
{

#if 0//RM6752

#if RN6752_OUT_WIDTH == 1280

// 720P@25 with mipi 2 data lanes + 1 clock lane out
// pin24/23 floating, pin18/17 floating
// pin16/15 data lane1, pin12/11 data lane0
// pin14/13 clock lane
// Slave address is 0x58
// Register, data

// if clock source(Xin) of RN6752 is 26MHz, please add these proce
//0xD2, 0x85, // disable auto clock detect
//0xD6, 0x37, // 27MHz default
//0xD8, 0x18, // switch to 26MHz clock
//delay(100), // delay 100ms

{0x81, 0x01}, // turn on video decoder
{0xA3, 0x04},
{0xDF, 0xFE}, // enable HD format

// ch0
{0xFF, 0x00}, // switch to ch0 (default; optional)
{0x00, 0x20}, // internal use*
{0x06, 0x08}, // internal use*
{0x07, 0x63}, // HD format
{0x2A, 0x01}, // filter control
{0x3A, 0x00}, // No Insert Channel ID in SAV/EAV code
{0x3F, 0x10}, // channel ID
{0x4C, 0x37}, // equalizer
{0x4F, 0x03}, // sync control
{0x50, 0x02}, // 720p resolution
{0x56, 0x05}, // BT 72M mode and BT601 mode
{0x5F, 0x40}, // blank level
{0x63, 0xF5}, // filter control
{0x59, 0x00}, // extended register access
{0x5A, 0x42}, // data for extended register
{0x58, 0x01}, // enable extended register write
{0x59, 0x33}, // extended register access
{0x5A, 0x23}, // data for extended register
{0x58, 0x01}, // enable extended register write
{0x51, 0xE1}, // scale factor1
{0x52, 0x88}, // scale factor2
{0x53, 0x12}, // scale factor3
{0x5B, 0x07}, // H-scaling control
{0x5E, 0x0B}, // enable H-scaling control
{0x6A, 0x82}, // H-scaling control
{0x28, 0x92}, // cropping
{0x03, 0x80}, // saturation
{0x04, 0x80}, // hue
{0x05, 0x00}, // sharpness
{0x57, 0x23}, // black/white stretch
{0x68, 0x32}, // coring
{0x3A, 0x04},
{0x3E, 0x32},
{0x40, 0x04},
{0x46, 0x23},

{0x8E, 0x00}, // single channel output for VP
{0x8F, 0x80}, // 720p mode for VP
{0x8D, 0x31}, // enable VP out
{0x89, 0x09}, // select 72MHz for SCLK
{0x88, 0x41}, // enable SCLK out

{0x96, 0x00}, // select AVID & VBLK as status indicator
{0x97, 0x0B}, // enable status indicator out on AVID,VBLK & VSYNC
{0x98, 0x00}, // video timing pin status
{0x9A, 0x40}, // select AVID & VBLK as status indicator
{0x9B, 0xE1}, // enable status indicator out on HSYNC
{0x9C, 0x00}, // video timing pin status

#else

// 1080P@25 with mipi 2 data lanes + 1 clock lane out
// pin24/23 floating, pin18/17 floating
// pin16/15 data lane1, pin12/11 data lane0
// pin14/13 clock lane
// Slave address is 0x58
// Register, data

// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
//0xD2, 0x85, // disable auto clock detect
//0xD6, 0x37, // 27MHz default
//0xD8, 0x18, // switch to 26MHz clock
//delay(100), // delay 100ms

{0x81, 0x01}, // turn on video decoder
{0xA3, 0x04},
{0xDF, 0xFE}, // enable HD format
{0xF0, 0xC0}, // 144MHz output

// ch0
{0xFF, 0x00}, // switch to ch0 (default; optional)
{0x00, 0x20}, // internal use*
{0x06, 0x08}, // internal use*
{0x07, 0x63}, // HD format
{0x2A, 0x01}, // filter control
{0x3A, 0x20}, // Insert Channel ID in SAV/EAV code
{0x3F, 0x10}, // channel ID
{0x4C, 0x37}, // equalizer
{0x4F, 0x03}, // sync control
{0x50, 0x03}, // 1080p resolution
{0x56, 0x02}, // BT 144M mode
{0x5F, 0x44}, // blank level
{0x63, 0xF8}, // filter control
{0x59, 0x00}, // extended register access
{0x5A, 0x48}, // data for extended register
{0x58, 0x01}, // enable extended register write
{0x59, 0x33}, // extended register access
{0x5A, 0x23}, // data for extended register
{0x58, 0x01}, // enable extended register write
{0x51, 0xF4}, // scale factor1
{0x52, 0x29}, // scale factor2
{0x53, 0x15}, // scale factor3
{0x5B, 0x01}, // H-scaling control
{0x5E, 0x0F}, // enable H-scaling control
{0x6A, 0x87}, // H-scaling control
{0x28, 0x92}, // cropping
{0x03, 0x80}, // saturation
{0x04, 0x80}, // hue
{0x05, 0x04}, // sharpness
{0x57, 0x23}, // black/white stretch
{0x68, 0x00}, // coring
{0x3A, 0x04},
{0x3E, 0x32},
{0x40, 0x04},
{0x46, 0x23},

{0x8E, 0x00}, // single channel output for VP
{0x8F, 0x80}, // 1080p mode for VP
{0x8D, 0x31}, // enable VP out
{0x89, 0x0A}, // select 144MHz for SCLK
{0x88, /*0x41*/0x43}, // enable SCLK out

{0x96, 0x00}, // select AVID & VBLK as status indicator
{0x97, 0x0B}, // enable status indicator out on AVID,VBLK & VSYNC
{0x98, 0x00}, // video timing pin status
{0x9A, 0x40}, // select AVID & VBLK as status indicator
{0x9B, 0xE1}, // enable status indicator out on HSYNC
{0x9C, 0x00}, // video timing pin status

#endif


#else  //RN6752

	// If clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	///{0xD2, 0x85},		 disable auto clock detect
	///{0xD6, 0x37},		// 27MHz default
	///{0xD8, 0x18},		// switch to 26MHz clock
	// delay 100ms

	{0x81, 0x01},		// Turn on video decoder

	{0xA3, 0x04},		// Enable 72MHz sampling
	{0xDB, 0x8F},		// Internal use*
	{0xFF, 0x00},		// Switch to ch0 (default; optional)
	{0x2C, 0x30},		// Select sync slice points
	{0x50, 0x02},		// 720p resolution select for BT.601
//	{0x56, 0x00},		// Disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	{0x56, 0x04},		// Bit[2]=Enable BT601
	{0x63, 0xBD},		// Filter control
	{0x59, 0x00},		// Address for extended register access

  #if 0		// NTSC
	{0x5A, 0x04},		// Data for extended register
  #else		// PAL
	{0x5A, 0x02},		// Data for extended register
  #endif

	{0x58, 0x01},		// Enable extended register write
	{0x07, 0x23},		// 720p format
	{0x2F, 0x04},		// Internal use*
	{0x5E, 0x0B},		// Enable H-scaling control
	{0x51, 0x88},		// Scale factor1
	{0x52, 0x0C},		// Scale factor2
	{0x53, 0x25},		// Scale factor3
	{0x5B, 0x02},		// Disable down-scaler
	{0x3A, 0x04},		// No channel information insertion; invert VBLK for frame valid
	{0x3E, 0x32},		// AVID & VBLK out for BT.601
	{0x40, 0x04},		// No channel information insertion; invert VBLK for frame valid
	{0x46, 0x23},		// AVID & VBLK out for BT.601
	{0x28, 0x92},		// Cropping
	#if 0
	{0x00, 0xE0},		// Open color bar
	#else
	{0x00, 0x20},		// Internal use*
	#endif

	{0x2D, 0xF2},		// Cagc adjust
	{0x0D, 0x20},		// Cagc initial value
	{0x05, 0x00},		// Sharpness

  #if 0		// NTSC
	{0x04, 0x60},		// Hue
  #else		// PAL
	{0x04, 0x80},		// Hue
  #endif

	{0xDF, 0xFE},		// Enable 720p format
	{0x8E, 0x00},		// Single channel output for VP
	{0x8F, 0x80},		// 720p mode for VP
	{0x8D, 0x31},		// Enable VP out
	{0x89, 0x09},		// Select 72MHz for SCLK
	{0x88, 0xC1},		// Enable SCLK out
	{0x81, 0x01},		// Turn on video decoder
	{0x96, 0x00},		// Select AVID & VBLK as status indicator
	{0x97, 0x0B},		// Enable status indicator out on AVID,VBLK & VSYNC
	{0x98, 0x00},		// Video timing pin status
	{0x9A, 0x40},		// Select AVID & VBLK as status indicator
	{0x9B, 0xE1},		// Enable status indicator out on HSYNC
    {0x9C, 0x00},	    // Video timing pin status

#endif

    {0xff, 0xff}

};

static INT32S rn6752_sccb_open(void)
{

#if RN6752_I2C_MODE==SENSOR_I2C_MODE_SW
	rn6752_handle = drv_l2_sccb_open_ext(&rn6752_sccb_config);
	if(rn6752_handle == 0)
	{
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}

#elif RN6752_I2C_MODE==SENSOR_I2C_MODE_HW
	drv_l1_i2c_init(RN6752_I2C_DEVICE);
	rn6752_handle.devNumber = RN6752_I2C_DEVICE;
	rn6752_handle.slaveAddr = RN6752_ID;
	rn6752_handle.clkRate = 100;
#endif
	return STATUS_OK;
}

static void rn6752_sccb_close(void)
{
#if RN6752_I2C_MODE==SENSOR_I2C_MODE_SW
	if(rn6752_handle)
	{
		drv_l2_sccb_close(rn6752_handle);
		rn6752_handle = NULL;
	}
#elif RN6752_I2C_MODE==SENSOR_I2C_MODE_HW
	drv_l1_i2c_uninit(RN6752_I2C_DEVICE);
	rn6752_handle.devNumber = 0;
	rn6752_handle.slaveAddr = 0;
	rn6752_handle.clkRate = 0;
#endif
}

static INT32S rn6752_sccb_write(INT8U reg, INT8U value)
{
#if RN6752_I2C_MODE==SENSOR_I2C_MODE_SW
	return drv_l2_sccb_write(rn6752_handle, reg, value);

#elif RN6752_I2C_MODE==SENSOR_I2C_MODE_HW
	return drv_l1_reg_1byte_data_1byte_write(&rn6752_handle, reg, value);
#endif
}

#if 0
static INT32S rn6752_sccb_read(INT16U reg,INT16U *value)
{
#if RN6752_I2C_MODE==SENSOR_I2C_MODE_SW
	INT16U	data;

	if(drv_l2_sccb_read(rn6752_handle, reg, &data) >= 0)
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

#elif RN6752_I2C_MODE==SENSOR_I2C_MODE_HW
	if(drv_l1_reg_1byte_data_1byte_read(&rn6752_handle, reg, value) >= 0)
	{
		DBG_PRINT("0x%02x, 0x%02x\r\n", reg, *value);
		return STATUS_OK;
	}
	else
	{
		DBG_PRINT("sccb_read fail...\r\n");
		return STATUS_FAIL;
	}

#endif
}
#endif


static INT32S rn6752_sccb_write_table(regval8_t *pTable)
{
    INT32U retry_cnt = 0;

	while(1)
	{
		if(pTable->reg_num == 0xff && pTable->value == 0xff)
		{
			break;
		}
        //DBG_PRINT("reg = 0x%x, val = 0x%x\r\n", pTable->reg_num, pTable->value);
		if(rn6752_sccb_write(pTable->reg_num, pTable->value) < 0)
		{
			DBG_PRINT("%s: sccb write fail: reg = 0x%x, val = 0x%x\r\n", __FUNCTION__, pTable->reg_num, pTable->value);
			retry_cnt += 1;
			if(retry_cnt >= 10)
				return STATUS_FAIL;
			continue;
		}
#if 0
		//if((pTable->reg_num == 0x0090) || (pTable->reg_num == 0x0033) ||(pTable->reg_num == 0x0032))
		{
			unsigned short tt;

		    osDelay(10);
		    rn6752_sccb_read(pTable->reg_num,&tt);
		//    if(tt!= pTable->value)
			DBG_PRINT("i2c reg[0x%04x] = 0x%04x, read = 0x%04x\r\n", pTable->reg_num, pTable->value, tt);
		}
#endif
		pTable++;
	}

	return STATUS_OK;
}

void rn6752_csi_dvp_init(void)
{
	DBG_PRINT("%s\r\n", __func__);

    drv_l1_csi_init(RN6752_CSI_DEV);

	// reguest sccb
	rn6752_sccb_open();

	#ifdef REAR_SENSOR_RESET
	gpio_set_port_attribute(REAR_SENSOR_RESET, ATTRIBUTE_HIGH);
	gpio_init_io(REAR_SENSOR_RESET, GPIO_OUTPUT);
	gpio_write_io(REAR_SENSOR_RESET, DATA_HIGH);
	osDelay(100);
	gpio_write_io(REAR_SENSOR_RESET, DATA_LOW);
	osDelay(200);
	gpio_write_io(REAR_SENSOR_RESET, DATA_HIGH);
	osDelay(100);
	#endif
}

/**
 * @brief   un-initialization function
 * @param   sensor format parameters
 * @return 	none
 */
void rn6752_csi_dvp_uninit(void)
{
	// disable mclk
	drv_l2_sensor_set_mclkout(MCLK_NONE,RN6752_CSI_DEV);

    //drv_l1_csi_stop();
    drv_l2_csi_stop(RN6752_CSI_DEV);

   // release sccb
	rn6752_sccb_close();

	drv_l1_csi_uninit(RN6752_CSI_DEV);
}

/**
 * @brief   stream start function
 * @param   info index
 *
 * @return 	none
 */
void rn6752_csi_dvp_stream_start(INT32U index, INT32U bufA, INT32U bufB)
{
	gpCSIPara_t csi_Para;

	gp_memset((INT8S *)&csi_Para, 0, sizeof(gpCSIPara_t));

	/* set input & output format */
	csi_Para.csi_fmt.input_format = rn6752_csi_dvp_ops.info[index].input_format;
	csi_Para.csi_fmt.output_format = rn6752_csi_dvp_ops.info[index].output_format;

	csi_Para.csi_tim.clki_inv = 1;
    csi_Para.csi_fmt.interface_mode = rn6752_csi_dvp_ops.info[index].interface_mode;
    csi_Para.csi_fmt.preview_mode = CSI_CAPTURE_MODE;
	csi_Para.csi_fmt.clk_stopb = ENABLE;
	csi_Para.csi_tim.fget_mode = ENUM_CSI_RISING_EDGE;
    csi_Para.csi_tim.hrst_mode = ENUM_CSI_RISING_EDGE;
    csi_Para.csi_tim.vadd_mode = ENUM_CSI_RISING_EDGE;
    csi_Para.csi_tim.vrst_mode = ENUM_CSI_RISING_EDGE;
    csi_Para.csi_tim.d_type = ENUM_CSI_LATCH_DELAY_3_CLOCK;
    csi_Para.h_start = csi_Para.v_start = 0x0;
    csi_Para.h_end = csi_Para.v_end = 0xfff;

	csi_Para.target_w = rn6752_csi_dvp_ops.info[index].target_w;
	csi_Para.target_h = rn6752_csi_dvp_ops.info[index].target_h;
	csi_Para.hratio = 0;		// No Scale
	csi_Para.vratio = 0;		// No Scale

	/* Set sensor's registers via SCCB */
	if(rn6752_sccb_write_table((regval8_t *)rn6752_dvp_init) < 0) {
		DBG_PRINT("rn6752_sccb_write_table fail!!!\r\n");
	}

	DBG_PRINT("%s = %d\r\n", __func__, index);
    if(drv_l2_csi_set_fmt(RN6752_CSI_DEV,&csi_Para.csi_fmt) < 0)
    {
		DBG_PRINT("csi set fmt err!!!\r\n");
	}

	if(bufA) {
		drv_l2_csi_stream_on(RN6752_CSI_DEV,(gpCSIPara_t *)&csi_Para, bufA, bufB);
		DBG_PRINT("Sensor rn6752 sensor dvp init completed\r\n");
	} else {
		DBG_PRINT("Input frame buffer address error, fail to start %s\r\n", rn6752_csi_dvp_ops.name);
		rn6752_csi_dvp_ops.stream_stop();
	}
	

  // while(1);
}

/**
 * @brief   stream stop function
 * @param   none
 * @return 	none
 */
void rn6752_csi_dvp_stream_stop(void)
{
	drv_l2_csi_stream_off(RN6752_CSI_DEV);
}

/**
 * @brief   get info function
 * @param   none
 * @return 	pointer to sensor information data
 */
drv_l2_sensor_info_t *rn6752_csi_dvp_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1))
	{
		return NULL;
	}
	else
	{
		return (drv_l2_sensor_info_t *) &rn6752_csi_dvp_ops.info[index];
	}
}

/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t	rn6752_csi_dvp_ops =
{
	#if RN6752_I2C_DEVICE == CSI_0_PATH
	SOURCE_CSI0_DVP, 				/* sensor name */
	#else
	SOURCE_CSI1_DVP, 				/* sensor name */
	#endif
	rn6752_csi_dvp_init,
	rn6752_csi_dvp_uninit,
	rn6752_csi_dvp_stream_start,
	rn6752_csi_dvp_stream_stop,
	rn6752_csi_dvp_get_info,
	NULL,
	NULL,
	NULL,
	NULL,
	{
		/* 1nd info */
		{
		    MCLK_24M,		        /* CSI clock */
			CSI_IN_YUYV,			/* input format */
			#if C_AHDCSI_GP420_EN
			CSI_OUT_GP420,			/* output format */
			#else
			CSI_OUT_YUYV,			/* output format */
			#endif
			25,	                	/* FPS in sensor */
			RN6752_OUT_WIDTH,		/* target width */
			RN6752_OUT_HEIGHT,		/* target height */
			RN6752_WIDTH,			/* sensor width */
			RN6752_HEIGHT,			/* sensor height */
			0,						/* sensor h offset */
			0,						/* sensor v offset */
			CSI_HSYNC_CCIR_601,		/* input interface */
			CSI_NON_INTERLACE,		/* interlace mode */
			ENUM_CSI_RISING_EDGE,	/* hsync pin active level */
			ENUM_CSI_RISING_EDGE	/* vsync pin active level */
		},

		/* 2st info */
		{
			MCLK_24M,       		/* CSI clock */
			CSI_IN_YUYV,			/* input format */
			CSI_OUT_YUYV,			/* output format */
			25,						/* FPS in sensor */
			RN6752_OUT_WIDTH,		/* target width */
			RN6752_OUT_HEIGHT,		/* target height */
			RN6752_WIDTH,			/* sensor width */
			RN6752_HEIGHT,			/* sensor height */
			0,						/* sensor h offset */
			0,						/* sensor v offset */
			CSI_HSYNC_CCIR_601,		/* input interface */
			CSI_NON_INTERLACE,		/* interlace mode */
			ENUM_CSI_RISING_EDGE,	/* hsync pin active level */
			ENUM_CSI_RISING_EDGE	/* vsync pin active level */
		},

	 }
};
#endif //(defined _SENSOR_RN6752_AHD_CSI_DVP) && (_SENSOR_RN6752_AHD_CSI_DVP == 1)

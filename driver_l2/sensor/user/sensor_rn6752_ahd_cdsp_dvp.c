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

#if (defined _SENSOR_RN6752_AHD_CDSP_DVP) && (_SENSOR_RN6752_AHD_CDSP_DVP == 1)


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/


#define RN6752_ID			0x5a

/*ISP Input*/
#define RN6752_WIDTH		1280
#define RN6752_HEIGHT		720

/*ISP Output*/
#define RN6752_OUT_WIDTH	1280
#define RN6752_OUT_HEIGHT	720

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
	// If clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//{0xD2, 0x85},		// disable auto clock detect
	//{0xD6, 0x37},		// 27MHz default
	//{0xD8, 0x18},		// switch to 26MHz clock
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
	{0x00, 0x20},		// Internal use*
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
	INT8U	value = 0;
	if(drv_l1_reg_1byte_data_1byte_read(&rn6752_handle, reg, &value) >= 0)
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

void rn6752_cdsp_dvp_init(void)
{
    INT8S ret = 0;
	DBG_PRINT("%s\r\n", __func__);

    // reguest sccb
	rn6752_sccb_open();
    //Change CDSP DVP Data[9:2] Pins to IOE0~IOE7, Ctrl[9:2] Pins to IOD6/IOD8/IOD9, Enable 2nd Sensor.
    R_FUNPOS1 &= ~(0x00000F80<<0);
    R_FUNPOS1 |= ((2<<8)|(1<<7)|(1<<0));

    // cdsp init
	ret = drv_l2_cdsp_open();
	if(ret < 0)
		DBG_PRINT("rn6752 CDSP init fail !\r\n");

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
void rn6752_cdsp_dvp_uninit(void)
{
	// disable mclk
	drv_l2_sensor_set_mclkout(MCLK_NONE, 0);

   // release sccb
	rn6752_sccb_close();
}

/**
 * @brief   stream start function
 * @param   info index
 *
 * @return 	none
 */
void rn6752_cdsp_dvp_stream_start(INT32U index, INT32U bufA, INT32U bufB)
{
	INT16U	target_w, target_h, sensor_w, sensor_h;
	gpCdspFmt_t format;

	DBG_PRINT("%s = %d\r\n", __func__, index);

    /* cdsp clock */
    drv_l1_CdspSetClockTree(ENABLE, FRONT_INPUT, 1);

	// set cdsp format
	format.image_source = C_CDSP_DVP;
	format.input_format = rn6752_cdsp_dvp_ops.info[index].input_format;
	format.output_format = rn6752_cdsp_dvp_ops.info[index].output_format;
	target_w = rn6752_cdsp_dvp_ops.info[index].target_w;
	target_h = rn6752_cdsp_dvp_ops.info[index].target_h;
	format.hpixel = sensor_w = rn6752_cdsp_dvp_ops.info[index].sensor_w;
	format.vline = sensor_h = rn6752_cdsp_dvp_ops.info[index].sensor_h;
	format.hoffset = rn6752_cdsp_dvp_ops.info[index].hoffset;
	format.voffset = rn6752_cdsp_dvp_ops.info[index].voffset;
	format.sensor_timing_mode = rn6752_cdsp_dvp_ops.info[index].interface_mode;
	format.sensor_hsync_mode = rn6752_cdsp_dvp_ops.info[index].hsync_active;
	format.sensor_vsync_mode = rn6752_cdsp_dvp_ops.info[index].vsync_active;

	if(drv_l2_cdsp_set_fmt(&format,0) < 0)
	{
		DBG_PRINT("cdsp set fmt err!!!\r\n");
	}

    drv_l1_CdspSetDataSource(SDRAM_INPUT);
	// set scale down
	#if 0
	if((format.hpixel > target_w) || (format.vline > target_h))
	{
		drv_l2_cdsp_set_yuv_scale(target_w, target_h);
	}
	#endif

	// cdsp start
	drv_l2_cdsp_stream_on(DISABLE, bufA, bufB,0);

	//Init Sensor
	rn6752_sccb_write_table((regval8_t *)rn6752_dvp_init);

	drv_l1_CdspSetDataSource(FRONT_INPUT);

	DBG_PRINT("Sensor rn6752 sensor_cdsp_dvp_yuv init completed\r\n");

  // while(1);
}

/**
 * @brief   stream stop function
 * @param   none
 * @return 	none
 */
void rn6752_cdsp_dvp_stream_stop(void)
{
	DBG_PRINT("%s:no function \r\n", __func__);

	//drv_l2_cdsp_stream_off();
}

/**
 * @brief   get info function
 * @param   none
 * @return 	pointer to sensor information data
 */
drv_l2_sensor_info_t *rn6752_cdsp_dvp_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1))
	{
		return NULL;
	}
	else
	{
		return (drv_l2_sensor_info_t *) &rn6752_cdsp_dvp_ops.info[index];
	}
}

/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t	rn6752_cdsp_dvp_ops =
{
	SOURCE_CDSP_DVP,	/* sensor name */
	rn6752_cdsp_dvp_init,
	rn6752_cdsp_dvp_uninit,
	rn6752_cdsp_dvp_stream_start,
	rn6752_cdsp_dvp_stream_stop,
	rn6752_cdsp_dvp_get_info,
	NULL,
	NULL,
	NULL,
	NULL,
	{
		/* 1nd info */
		{
		    MCLK_24M,		/* CSI clock */
			V4L2_PIX_FMT_UYVY,		/* input format */
			V4L2_PIX_FMT_YUYV,		/* output format */
			25,	                	/* FPS in sensor */
			RN6752_OUT_WIDTH,		/* target width */
			RN6752_OUT_HEIGHT,		/* target height */
			RN6752_WIDTH,			/* sensor width */
			RN6752_HEIGHT,			/* sensor height */
			0,						/* sensor h offset */
			0,						/* sensor v offset */
			MODE_CCIR_601,			/* input interface */
			MODE_NONE_INTERLACE,	/* interlace mode */
			MODE_ACTIVE_HIGH,		/* hsync pin active level */
			MODE_ACTIVE_HIGH		/* vsync pin active level */
		},

		/* 2st info */
		{
			MCLK_24M,//MASTER_CLOCK_24MHZ,		/* CSI clock */
			V4L2_PIX_FMT_YUYV,		/* input format */
			V4L2_PIX_FMT_YUYV,		/* output format */
			25,						/* FPS in sensor */
			RN6752_OUT_WIDTH,		/* target width */
			RN6752_OUT_HEIGHT,		/* target height */
			RN6752_WIDTH,			/* sensor width */
			RN6752_HEIGHT,			/* sensor height */
			0,						/* sensor h offset */
			0,						/* sensor v offset */
			MODE_CCIR_601,			/* input interface */
			MODE_NONE_INTERLACE,	/* interlace mode */
			MODE_ACTIVE_HIGH,		/* hsync pin active level */
			MODE_ACTIVE_HIGH		/* vsync pin active level */
		},

	 }
};
#endif //(defined _SENSOR_RN6752_AHD_CDSP_DVP_YUV) && (_SENSOR_RN6752_AHD_CDSP_DVP_YUV == 1)

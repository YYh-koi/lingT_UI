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
#if (defined _SENSOR_TP9951_AHD_CSI_DVP) && (_SENSOR_TP9951_AHD_CSI_DVP == 1)

#define AHD_720P		0
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define TP9951_DVP_CSI_DEV	        CSI_0_PATH

#define TP9951_DVP_ID			    0x88

#if AHD_720P
/*ISP Input*/
#define TP9951_DVP_WIDTH		    1280
#define TP9951_DVP_HEIGHT		    720

/*ISP Output*/
#define TP9951_DVP_OUT_WIDTH	    1280
#define TP9951_DVP_OUT_HEIGHT	    720
#else
/*ISP Input*/
#define TP9951_DVP_WIDTH		    1920
#define TP9951_DVP_HEIGHT		    1080

/*ISP Output*/
#define TP9951_DVP_OUT_WIDTH	    1920
#define TP9951_DVP_OUT_HEIGHT	    1080
#endif

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define TP9951_DVP_I2C_MODE	            SENSOR_I2C_MODE_SW

#if TP9951_DVP_I2C_MODE == SENSOR_I2C_MODE_SW
    #define TP9951_DVP_GPIO_SCL		    IO_B4
    #define TP9951_DVP_GPIO_SDA		    IO_B5
    #define TP9951_DVP_GPIO_DRIVING	    IOC_DRV_10mA
#else
    #define TP9951_DVP_I2C_DEVICE		I2C_0
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
#if TP9951_DVP_I2C_MODE == SENSOR_I2C_MODE_SW
sccb_config_t tp9951_dvp_sccb_config =
{
	TP9951_DVP_GPIO_SCL,		//scl_port
	TP9951_DVP_GPIO_DRIVING,	//scl_drv
	TP9951_DVP_GPIO_SDA,		//sda_port
	TP9951_DVP_GPIO_DRIVING,	//sda_drv
	IO_F15,			    //pwdn_port
	TP9951_DVP_GPIO_SDA, //pwdn_drv
	0,			//have_pwdn
	8,			//RegBits
	8,			//DataBits
	TP9951_DVP_ID,	//slaveAddr
	0x20000,	//timeout
	100			//clock_rate
};
static void	 *tp9951_dvp_handle;

#elif TP9951_DVP_I2C_MODE == SENSOR_I2C_MODE_HW
static drv_l1_i2c_bus_handle_t	tp9951_dvp_handle;
#endif

static regval8_t tp9951_1080p_dvp_init[]=
{

////褉褌芯斜薪鈺檌nit
{0x40, 0x00}, //select decoder page
{0x42, 0x00}, //common setting for all format
{0x4c, 0x43}, //common setting for all format
{0x4e, 0x1d}, //common setting for dvp output
{0x54, 0x04}, //common setting for dvp output

{0xf6, 0x00}, //common setting for all format
{0xf7, 0x44}, //common setting for dvp output
{0xfa, 0x00}, //common setting for dvp output
{0x1b, 0x01}, //common setting for dvp output

{0x41, 0x00},//(0x41, ch); //video MUX select

{0x40, 0x08}, //common setting for all format
{0x02, 0x80},
{0x03, 0x80},
{0x04, 0x80},
{0x12, 0x54},
{0x13, 0xef},
{0x14, 0x41},
{0x15, 0x02},

{0x40, 0x00}, //back to decoder page
{0x02, 0xc8},
{0x07, 0xc0},
{0x0b, 0xc0},
{0x0c, 0x03},
{0x0d, 0x50},

{0x15, 0x01},
{0x16, 0xd2},
{0x17, 0x88},//7-0,1928
{0x18, 0x24},
{0x19, 0x40},
{0x1a, 0x47},//11-8,1928

{0x1c, 0x0a}, //1920*1080, 25fps
{0x1d, 0x50}, //

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

//AHD1080p25 extra
{0x02, 0xcc},

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

{0xff, 0xff}

};

static regval8_t tp9951_720p_dvp_init[]=
{
		{0x40, 0x00},		//select decoder page
		{0x42, 0x00},		//common setting for all format
		{0x4c, 0x43},		//common setting for all format
		{0x4e, 0x1d},		//common setting for dvp output
		{0x54, 0x04},		//common setting for dvp output
		{0xf6, 0x00},		//common setting for all format
		{0xf7, 0x44},		//common setting for dvp output
		{0xfa, 0x00},		//common setting for dvp output
		{0x1b, 0x01},		//common setting for dvp output

		{0x41, 0x00},//tp9951_write_reg(0x41, ch);		//video MUX select

		{0x40, 0x08},		//common setting for all format
		{0x13, 0xef},		//common setting for dvp output
		{0x14, 0x41},		//common setting for dvp output
		{0x15, 0x02},		//common setting for dvp output
		{0x40, 0x00},		//select decoder page
	//TP9951_dvp_out
		//mipi setting
		{0x40, 0x08},		//select mipi page
		{0x12, 0x54},
		{0x13, 0xef},
		{0x14, 0x41},
		{0x15, 0x12},
		{0x40, 0x00},
		{0x40, 0x00},		//back to decoder page
		{0x02, 0xca},
		{0x07, 0xc0},
		{0x0b, 0xc0},
		{0x0c, 0x13},
		{0x0d, 0x50},

		{0x15, 0x13},
		{0x16, 0x15},
		{0x17, 0x08},
		{0x18, 0x19},
		{0x19, 0xd0},
		{0x1a, 0x25},
		{0x1c, 0x07},
		{0x1d, 0xbc},

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

		{0x35, 0x25},
		{0x38, 0x00},
		{0x39, 0x18},
	//AHD720p25 extra
		{0x02, 0xce},

		{0x0d, 0x71},

		{0x18, 0x1b},

		{0x20, 0x40},
		{0x21, 0x46},

		{0x25, 0xfe},
		{0x26, 0x01},

		{0x2c, 0x3a},
		{0x2d, 0x5a},
		{0x2e, 0x40},

		{0x30, 0x9e},
		{0x31, 0x20},
		{0x32, 0x10},
		{0x33, 0x90},

	{0xff, 0xff}
};

static INT32S tp9951_dvp_sccb_open(void)
{
#if TP9951_DVP_I2C_MODE == SENSOR_I2C_MODE_SW
	tp9951_dvp_handle = drv_l2_sccb_open_ext(&tp9951_dvp_sccb_config);
	if(tp9951_dvp_handle == 0)
	{
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}

#elif TP9951_DVP_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_init(TP9951_DVP_I2C_DEVICE);
	tp9951_dvp_handle.devNumber = TP9951_DVP_I2C_DEVICE;
	tp9951_dvp_handle.slaveAddr = TP9951_DVP_ID;
	tp9951_dvp_handle.clkRate = 100;
#endif
	return STATUS_OK;
}

static void tp9951_dvp_sccb_close(void)
{

#if TP9951_DVP_I2C_MODE == SENSOR_I2C_MODE_SW
	if(tp9951_dvp_handle)
	{
		drv_l2_sccb_close(tp9951_dvp_handle);
		tp9951_dvp_handle = NULL;
	}

#elif TP9951_DVP_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_uninit(TP9951_DVP_I2C_DEVICE);
	tp9951_dvp_handle.devNumber = 0;
	tp9951_dvp_handle.slaveAddr = 0;
	tp9951_dvp_handle.clkRate = 0;
#endif
}

static INT32S tp9951_dvp_sccb_write(INT8U reg, INT8U value)
{
	INT32S ret;
#if TP9951_DVP_I2C_MODE == SENSOR_I2C_MODE_SW
	ret =  drv_l2_sccb_write(tp9951_dvp_handle, reg, value);
	if (ret < 0)
		DBG_PRINT("tp9951 sccb write fail\r\n");
	return ret;

#elif TP9951_DVP_I2C_MODE == SENSOR_I2C_MODE_HW
	return drv_l1_reg_1byte_data_1byte_write(&tp9951_dvp_handle, reg, value);
#endif
}

static INT32S tp9951_dvp_sccb_read(INT16U reg,INT16U *value)
{
#if TP9951_DVP_I2C_MODE == SENSOR_I2C_MODE_SW
	INT16U	data;

	if(drv_l2_sccb_read(tp9951_dvp_handle, reg, &data) >= 0)
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

#elif TP9951_DVP_I2C_MODE == SENSOR_I2C_MODE_HW
	if(drv_l1_reg_1byte_data_1byte_read(&tp9951_dvp_handle, reg, value) >= 0)
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

static INT32S tp9951_dvp_sccb_write_table(regval8_t *pTable)
{
    INT32U retry_cnt = 0;

	while(1)
	{
		if(pTable->reg_num == 0xff && pTable->value == 0xff)
		{
			break;
		}
        //DBG_PRINT("reg = 0x%x, val = 0x%x\r\n", pTable->reg_num, pTable->value);
		if(tp9951_dvp_sccb_write(pTable->reg_num, pTable->value) < 0)
		{
			DBG_PRINT("%s: sccb write fail: reg = 0x%x, val = 0x%x\r\n", __FUNCTION__, pTable->reg_num, pTable->value);
			retry_cnt += 1;
			if(retry_cnt >= 10)
				return STATUS_FAIL;
			continue;
		}
#if 0
		{
			unsigned short tt;

		    osDelay(10);
		    tp9951_dvp_sccb_read(pTable->reg_num,&tt);
		//    if(tt!= pTable->value)
			DBG_PRINT("i2c reg[0x%04x] = 0x%04x, read = 0x%04x\r\n", pTable->reg_num, pTable->value, tt);
		}
#endif
		pTable++;
	}

	return STATUS_OK;
}


void tp9951_dvp_csi_init(void)
{
	DBG_PRINT("%s\r\n", __func__);

	// reguest sccb
	tp9951_dvp_sccb_open();

    drv_l1_csi_init(TP9951_DVP_CSI_DEV);

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
void tp9951_dvp_csi_uninit(void)
{
    //csi_stop;
    drv_l2_csi_stop(TP9951_DVP_CSI_DEV);

   // release sccb
	tp9951_dvp_sccb_close();

	drv_l1_csi_uninit(TP9951_DVP_CSI_DEV);

    // disable mclk
	drv_l2_sensor_set_mclkout(MCLK_NONE, TP9951_DVP_CSI_DEV);
}

/**
 * @brief   stream start function
 * @param   info index
 *
 * @return 	none
 */
void tp9951_dvp_csi_stream_start(INT32U index, INT32U bufA, INT32U bufB)
{
	gpCSIPara_t csi_Para;

	gp_memset((INT8S *)&csi_Para, 0, sizeof(gpCSIPara_t));

	/* set input & output format */
	csi_Para.csi_fmt.input_format = tp9951_csi_dvp_ops.info[index].input_format;
	csi_Para.csi_fmt.output_format = tp9951_csi_dvp_ops.info[index].output_format;

    //csi_Para.csi_tim.clki_inv = 1;
    csi_Para.csi_fmt.interface_mode = tp9951_csi_dvp_ops.info[index].interface_mode;
	csi_Para.csi_fmt.interlace_mode = (INT32U)tp9951_csi_dvp_ops.info[index].interlace_mode;
    csi_Para.csi_fmt.preview_mode = CSI_CAPTURE_MODE;
	csi_Para.csi_fmt.fieldinv = DISABLE;
	csi_Para.csi_fmt.clk_stopb = ENABLE;

  	//latch_timing1
	csi_Para.csi_tim.hl_start = (INT32U)tp9951_csi_dvp_ops.info[index].hoffset;
	csi_Para.csi_tim.vl0_start = (INT32U)tp9951_csi_dvp_ops.info[index].voffset;
	csi_Para.csi_tim.vl1_start = 0x02;
	csi_Para.csi_tim.d_type = ENUM_CSI_LATCH_DELAY_3_CLOCK;
	//latch_timing2
	csi_Para.csi_tim.fget_mode = (INT32U)tp9951_csi_dvp_ops.info[index].hsync_active;
	csi_Para.csi_tim.vrst_mode = (INT32U)tp9951_csi_dvp_ops.info[index].vsync_active;
	csi_Para.csi_tim.vadd_mode = (INT32U)tp9951_csi_dvp_ops.info[index].vsync_active;
	csi_Para.csi_tim.hrst_mode = (INT32U)tp9951_csi_dvp_ops.info[index].hsync_active;
	//
	csi_Para.csi_tim.clko_inv = DISABLE;
	csi_Para.h_start = csi_Para.v_start = 0x00;
	csi_Para.h_end = csi_Para.v_end = 0xfff;

	csi_Para.target_w = tp9951_csi_dvp_ops.info[index].target_w;
	csi_Para.target_h = tp9951_csi_dvp_ops.info[index].target_h;
	csi_Para.hratio = 0;		// No Scale
	csi_Para.vratio = 0;		// No Scale

	/* Set sensor's registers via SCCB */
	DBG_PRINT("tp9951[%d]_target_w[%d]\r\n",index,tp9951_csi_dvp_ops.info[index].target_w);
	if(tp9951_csi_dvp_ops.info[index].target_w ==1280)
	{
        if(tp9951_dvp_sccb_write_table((regval8_t *)tp9951_720p_dvp_init) < 0) {
            DBG_PRINT("tp9951_sccb_write_table fail!!!\r\n");
        }
        DBG_PRINT("tp9951_720p_dvp_init ok\r\n");
	}
	else
	{
        if(tp9951_dvp_sccb_write_table((regval8_t *)tp9951_1080p_dvp_init) < 0) {
            DBG_PRINT("tp9951_sccb_write_table fail!!!\r\n");
        }
        DBG_PRINT("tp9951_1080p_dvp_init ok\r\n");
	}

	DBG_PRINT("%s = %d\r\n", __func__, index);
    if(drv_l2_csi_set_fmt(TP9951_DVP_CSI_DEV,&csi_Para.csi_fmt) < 0)
    {
		DBG_PRINT("csi set fmt err!!!\r\n");
	}

	if(bufA) {
		drv_l2_csi_stream_on(TP9951_DVP_CSI_DEV,(gpCSIPara_t *)&csi_Para, bufA, 0);
	} else {
		DBG_PRINT("Input frame buffer address error, fail to start %s\r\n", tp9951_csi_dvp_ops.name);
		tp9951_csi_dvp_ops.stream_stop();
	}

	DBG_PRINT("Sensor tp9951 sensor dvp init completed\r\n");

  // while(1);
}

/**
 * @brief   stream stop function
 * @param   none
 * @return 	none
 */
void tp9951_dvp_csi_stream_stop(void)
{
	drv_l2_csi_stream_off(TP9951_DVP_CSI_DEV);
}

/**
 * @brief   get info function
 * @param   none
 * @return 	pointer to sensor information data
 */
drv_l2_sensor_info_t *tp9951_dvp_csi_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1))
	{
		return NULL;
	}
	else
	{
		return (drv_l2_sensor_info_t *) &tp9951_csi_dvp_ops.info[index];
	}
}

/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t	tp9951_csi_dvp_ops =
{

  	#if(TP9951_DVP_CSI_DEV == CSI_0_PATH)
    SOURCE_CSI0_DVP,               /* sensor name */
    #else
    SOURCE_CSI1_DVP,               /* sensor name */
    #endif
	tp9951_dvp_csi_init,
	tp9951_dvp_csi_uninit,
	tp9951_dvp_csi_stream_start,
	tp9951_dvp_csi_stream_stop,
	tp9951_dvp_csi_get_info,
	NULL,
	NULL,
	NULL,
	NULL,
	{
		/* 1nd info */
		{
		    MCLK_24M,		/* CSI clock */
            CSI_IN_YUYV,			/* input format */
			CSI_OUT_YUYV,			/* output format */
			25,	                	/* FPS in sensor */
			TP9951_DVP_OUT_WIDTH,		/* target width */
			TP9951_DVP_OUT_HEIGHT,		/* target height */
			TP9951_DVP_WIDTH,			/* sensor width */
			TP9951_DVP_HEIGHT,			/* sensor height */
            #if AHD_720P
			4,						/* sensor h offset */
			#else
			0,						/* sensor h offset */
			#endif
			0,						/* sensor v offset */
			CSI_HSYNC_CCIR_656,     /* input interface */
			CSI_NON_INTERLACE,		/* interlace mode */
			ENUM_CSI_FALLING_EDGE,	/* hsync pin active level */
			ENUM_CSI_FALLING_EDGE	/* vsync pin active level */
		},

		/* 2st info */
		{
			MCLK_24M,//MASTER_CLOCK_24MHZ,		/* CSI clock */
			CSI_IN_YUYV,			/* input format */
			CSI_OUT_GP420,			/* output format */
			25,						/* FPS in sensor */
			TP9951_DVP_OUT_WIDTH,		/* target width */
			TP9951_DVP_OUT_HEIGHT,		/* target height */
			TP9951_DVP_WIDTH,			/* sensor width */
			TP9951_DVP_HEIGHT,			/* sensor height */
			0,						/* sensor h offset */
			0,						/* sensor v offset */
			CSI_HSYNC_CCIR_656,		/* input interface */
			CSI_NON_INTERLACE,		/* interlace mode */
			ENUM_CSI_RISING_EDGE,	/* hsync pin active level */
			ENUM_CSI_RISING_EDGE	/* vsync pin active level */
		},

	 }
};
#endif //(defined _SENSOR_TP9951_AHD_CSI_DVP) && (_SENSOR_TP9951_AHD_CSI_DVP == 1)

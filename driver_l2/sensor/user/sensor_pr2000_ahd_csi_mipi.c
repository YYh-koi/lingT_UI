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

#if (defined _SENSOR_PR2000_AHD_CSI_MIPI) && (_SENSOR_PR2000_AHD_CSI_MIPI == 1)


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define MIPI_ISR_TEST		0
#define MIPI_LANE_NO		MIPI_2_LANE // 1 or 2 lane
#define MIPI_DEV_NO         0			//0:MIPI_0(IOC8~11) or 1:MIPI_1(IOE0~9)

#define PR2000_ID			0xb8//0x78//0xb8//

/*ISP Input*/
#define PR2000_WIDTH		1280
#define PR2000_HEIGHT		720

/*ISP Output*/
#define PR2000_OUT_WIDTH	1280
#define PR2000_OUT_HEIGHT	720

#define PR2000_CSI_DEV		CSI_0_PATH

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define PR2000_I2C_MODE	            SENSOR_I2C_MODE_SW

#if PR2000_I2C_MODE == SENSOR_I2C_MODE_SW
    #define PR2000_GPIO_SCL		    IO_B4
    #define PR2000_GPIO_SDA		    IO_B5
    #define PR2000_GPIO_DRIVING	    IOB_DRV_30mA
#else
    #define PR2000_I2C_DEVICE		I2C_0
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
#if PR2000_I2C_MODE == SENSOR_I2C_MODE_SW
sccb_config_t pr2000_sccb_config =
{
	PR2000_GPIO_SCL,			//scl_port
	PR2000_GPIO_DRIVING,		//scl_drv
	PR2000_GPIO_SDA,			//sda_port
	PR2000_GPIO_DRIVING,		//sda_drv
	IO_F15,						//pwdn_port
	0,	                        //pwdn_drv
	0,			                //have_pwdn
	8,			                //RegBits
	8,			                //DataBits
	PR2000_ID,	                //slaveAddr
	0x20000,	                //timeout
	100			                //clock_rate
};

static void	 *pr2000_handle;

#elif PR2000_I2C_MODE == SENSOR_I2C_MODE_HW
static drv_l1_i2c_bus_handle_t	pr2000_handle;
#endif

static mipi_config_t  pr2000_mipi_cfg =
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
	PR2000_WIDTH,			/* width, 0~0xFFFF */
	PR2000_HEIGHT,			/* height, 0~0xFFFF */
	0,					//2, 				/* back porch, 0~0xF */ //If Set 4 have show paper color at left side.u2.20160119
	0,					//2,				/* front porch, 0~0xF */
	ENABLE,				/* blanking_line_en, 0:disable, 1:enable */
	0,					//NULL,			/* RSD 3 */
	ENABLE,				/* ecc, 0:disable, 1:enable */
	MIPI_ECC_ORDER3,	/* ecc order */
	350,				//400,//250,			/* data mask, unit is ns */
	MIPI_CHECK_HS_SEQ	//MIPI_CHECK_LP_00	/* check hs sequence or LP00 for clock lane */
};

static regval8_t pr2000_mipi_yuv_720P[]=
{
{0xff, 0x00},
{0xa1, 0xff},
{0xff, 0x00},
{0xa1, 0xff},
{0xff, 0x00},
{0xa1, 0xff},
{0xff, 0x00},
{0xa1, 0xff},
{0xff, 0x80},
{0xff, 0x80},
{0xff, 0x80},
{0xff, 0x80},
{0xff, 0x00},
{0x10, 0x82},
{0x11, 0x07},
{0x12, 0x00},
{0x13, 0x00},
{0x14, 0x21},
{0x15, 0x44},
{0x16, 0x0d},
{0x40, 0x00},
{0x41, 0x08},
{0x42, 0x00},
{0x43, 0x00},
{0x47, 0x9a},
{0x48, 0x00},
{0x49, 0x0a},
{0x4e, 0x4c},
{0x80, 0x56},
{0x81, 0x0e},
{0x82, 0x0d},
{0x84, 0x30},
{0x86, 0x20},
{0x87, 0x00},
{0x8a, 0x00},
{0x90, 0x00},
{0x91, 0x00},
{0x92, 0x00},
{0x94, 0xff},
{0x95, 0xff},
{0x96, 0xff},
{0xa0, 0x00},
{0xa1, 0x20},
{0xa4, 0x01},
{0xa5, 0xe3},
{0xa6, 0x00},
{0xa7, 0x12},
{0xa8, 0x00},
{0xd0, 0x30},
{0xd1, 0x08},
{0xd2, 0x21},
{0xd3, 0x00},
{0xd8, 0x37},
{0xd9, 0x08},
{0xda, 0x21},
{0xe0, 0x19},
{0xe1, 0x10},
{0xe2, 0x18},
{0xe3, 0x00},
{0xe4, 0x00},
{0xe5, 0x0c},
{0xe6, 0x00},
{0xea, 0x00},
{0xeb, 0x00},
{0xf1, 0x44},
{0xf2, 0x01},

{0xff, 0x01},
{0x00, 0xe4},
{0x01, 0x61},
{0x02, 0x00},
{0x03, 0x57},
{0x04, 0x0c},
{0x05, 0x88},
{0x06, 0x04},
{0x07, 0xb2},
{0x08, 0x44},
{0x09, 0x34},
{0x0a, 0x02},
{0x0b, 0x14},
{0x0c, 0x04},
{0x0d, 0x08},
{0x0e, 0x5e},
{0x0f, 0x5e},
{0x10, 0x26},
{0x11, 0x01},
{0x12, 0x45},
{0x13, 0x0c},
{0x14, 0x00},
{0x15, 0x1b},
{0x16, 0xd0},
{0x17, 0x00},
{0x18, 0x41},
{0x19, 0x46},
{0x1a, 0x22},
{0x1b, 0x05},
{0x1c, 0xea},
{0x1d, 0x45},
{0x1e, 0x4c},
{0x1f, 0x00},
{0x20, 0x80},
{0x21, 0x80},
{0x22, 0x90},
{0x23, 0x80},
{0x24, 0x80},
{0x25, 0x80},
{0x26, 0x84},
{0x27, 0x82},
{0x28, 0x00},
{0x29, 0x7d},
{0x2a, 0x00},
{0x2b, 0x00},
{0x2c, 0x00},
{0x2d, 0x00},
{0x2e, 0x00},
{0x2f, 0x00},
{0x30, 0x00},
{0x31, 0x00},
{0x32, 0xc0},
{0x33, 0x14},
{0x34, 0x14},
{0x35, 0x80},
{0x36, 0x80},
{0x37, 0xaa},
{0x38, 0x48},
{0x39, 0x08},
{0x3a, 0x27},
{0x3b, 0x02},
{0x3c, 0x01},
{0x3d, 0x23},
{0x3e, 0x02},
{0x3f, 0xc4},
{0x40, 0x05},
{0x41, 0x55},
{0x42, 0x01},
{0x43, 0x33},
{0x44, 0x6a},
{0x45, 0x00},
{0x46, 0x09},
{0x47, 0xe2},
{0x48, 0x01},
{0x49, 0x00},
{0x4a, 0x7b},
{0x4b, 0x60},
{0x4c, 0x00},
{0x4d, 0x4a},
{0x4e, 0x00},
{0x4f, 0x04},
{0x50, 0x01},
{0x51, 0x28},
{0x52, 0x40},
{0x53, 0x0c},
{0x54, 0x0f},
{0x55, 0x8d},
{0x70, 0x06},
{0x71, 0x08},
{0x72, 0x0a},
{0x73, 0x0c},
{0x74, 0x0e},
{0x75, 0x10},
{0x76, 0x12},
{0x77, 0x14},
{0x78, 0x06},
{0x79, 0x08},
{0x7a, 0x0a},
{0x7b, 0x0c},
{0x7c, 0x0e},
{0x7d, 0x10},
{0x7e, 0x12},
{0x7f, 0x14},
{0x80, 0x00},
{0x81, 0x09},
{0x82, 0x00},
{0x83, 0x07},
{0x84, 0x00},
{0x85, 0x17},
{0x86, 0x03},
{0x87, 0xe5},
{0x88, 0x0a},
{0x89, 0x48},
{0x8a, 0x0a},
{0x8b, 0x48},
{0x8c, 0x0b},
{0x8d, 0xe0},
{0x8e, 0x05},
{0x8f, 0x47},
{0x90, 0x05},
{0x91, 0x69},
{0x92, 0x73},
{0x93, 0xe8},
{0x94, 0x0f},
{0x95, 0x5e},
{0x96, 0x07},
{0x97, 0x90},
{0x98, 0x17},
{0x99, 0x34},
{0x9a, 0x13},
{0x9b, 0x56},
{0x9c, 0x0b},
{0x9d, 0x9a},
{0x9e, 0x09},
{0x9f, 0xab},
{0xa0, 0x01},
{0xa1, 0x74},
{0xa2, 0x01},
{0xa3, 0x6b},
{0xa4, 0x00},
{0xa5, 0xba},
{0xa6, 0x00},
{0xa7, 0xa3},
{0xa8, 0x01},
{0xa9, 0x39},
{0xaa, 0x01},
{0xab, 0x39},
{0xac, 0x00},
{0xad, 0xc1},
{0xae, 0x00},
{0xaf, 0xc1},
{0xb0, 0x0b},
{0xb1, 0x99},
{0xb2, 0x12},
{0xb3, 0xca},
{0xb4, 0x00},
{0xb5, 0x17},
{0xb6, 0x08},
{0xb7, 0xe8},
{0xb8, 0xb0},
{0xb9, 0xce},
{0xba, 0x90},
{0xbb, 0x03},
{0xbc, 0x00},
{0xbd, 0x04},
{0xbe, 0x05},
{0xbf, 0x00},
{0xc0, 0x00},
{0xc1, 0x1b},//12
{0xc2, 0x02},
{0xc3, 0xd0},

{0xff, 0x01},
{0x54, 0x0e},
{0xff, 0x01},
{0x54, 0x0f},

{0xff, 0x00},
{0x47, 0x1a},
{0xff, 0x00},
{0x40, 0x00},
{0xff, 0x00},
{0x4e, 0x7f},

{0xff, 0x00},
{0x40, 0x00},
{0xff, 0x00},
{0x4e, 0x4c},
{0xff, 0x00},
{0x40, 0x40},
{0xff, 0x00},
{0x4e, 0x6c},
{0xff, 0x00},
{0x4e, 0x4c},
{0xff, 0x00},
{0x47, 0x9a},

{0xff, 0xff}
};

static INT32S pr2000_sccb_open(void)
{

#if PR2000_I2C_MODE == SENSOR_I2C_MODE_SW
	pr2000_handle = drv_l2_sccb_open_ext(&pr2000_sccb_config);
	if(pr2000_handle == 0)
	{
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#elif PR2000_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_init(PR2000_I2C_DEVICE);
	pr2000_handle.devNumber = PR2000_I2C_DEVICE;
	pr2000_handle.slaveAddr = PR2000_ID;
	pr2000_handle.clkRate = 100;
#endif
	return STATUS_OK;
}

static void pr2000_sccb_close(void)
{
#if PR2000_I2C_MODE == SENSOR_I2C_MODE_SW
	if(pr2000_handle)
	{
		drv_l2_sccb_close(pr2000_handle);
		pr2000_handle = NULL;
	}

#elif PR2000_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_uninit(PR2000_I2C_DEVICE);
	pr2000_handle.slaveAddr = 0;
	pr2000_handle.clkRate = 0;
#endif
}

static INT32S pr2000_sccb_write(INT16U reg, INT8U value)
{
#if PR2000_I2C_MODE == SENSOR_I2C_MODE_SW
	return drv_l2_sccb_write(pr2000_handle, reg, value);

#elif PR2000_I2C_MODE == SENSOR_I2C_MODE_HW
	return drv_l1_reg_1byte_data_1byte_write(&pr2000_handle, reg, value);
#endif
}


static INT32S pr2000_sccb_read(INT16U reg,INT16U *value)
{
#if PR2000_I2C_MODE == SENSOR_I2C_MODE_SW
	INT16U	data;

	if(drv_l2_sccb_read(pr2000_handle, reg, &data) >= 0)
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

#elif PR2000_I2C_MODE == SENSOR_I2C_MODE_HW
	INT8U	value = 0;
	if(drv_l1_reg_1byte_data_1byte_read(&pr2000_handle, reg, &value) >= 0)
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


static INT32S pr2000_sccb_write_table(regval8_t *pTable)
{
	//unsigned short tt;
	while(1)
	{
		if(pTable->reg_num == 0xFF && pTable->value == 0xFF)
		{
			break;
		}
        //DBG_PRINT("reg = 0x%x, val = 0x%x\r\n", pTable->reg_num, pTable->value);
		if(pr2000_sccb_write(pTable->reg_num, pTable->value) < 0)
		{
			DBG_PRINT("%s: sccb write fail: reg = 0x%x, val = 0x%x\r\n", __FUNCTION__, pTable->reg_num, pTable->value);
			continue;
		}

        //  osDelay(1);
        //  pr2000_sccb_read(pTable->reg_num,&tt);
		//  if(tt!= pTable->value)
		//	DBG_PRINT("i2c reg[0x%04x] = 0x%04x, read = 0x%04x\r\n", pTable->reg_num, pTable->value, tt);

		pTable++;
	}

	return STATUS_OK;
}


void pr2000_csi_mipi_init(void)
{
	DBG_PRINT("%s\r\n", __func__);
	// reguest sccb
	pr2000_sccb_open();

    drv_l1_csi_init(PR2000_CSI_DEV);

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

static void pr2000_mipi_yuv_sensor_init(void)
{
	// reset sensor
	//pr2000_sccb_write_table((regval16_t *) pr2000_reset_table);

	// init sensor
	pr2000_sccb_write_table((regval8_t *) pr2000_mipi_yuv_720P);
	//pr2000_sccb_write_table((regval16_t *)pr2000_mipi_init);

	DBG_PRINT("Sensor pr2000 sensor mipi init completed\r\n");
}

/**
 * @brief   ov7670 un-initialization function
 * @param   sensor format parameters
 * @return 	none
 */
void pr2000_csi_mipi_uninit(void)
{
	// csi disable
    drv_l1_csi_uninit(PR2000_CSI_DEV);
	// mipi disable
    drv_l1_mipi_init(MIPI_DEV_NO, DISABLE, 0);
	// release sccb
	pr2000_sccb_close();
	// disable mclk
	drv_l2_sensor_set_mclkout(MCLK_NONE, 0);
}

/**
 * @brief   stream start function
 * @param   info index
 *
 * @return 	none
 */
void pr2000_csi_mipi_stream_on(INT32U index, INT32U bufA, INT32U bufB)
{
//	DBG_PRINT("%s = %d\r\n", __func__, index);
	gpCSIPara_t csi_Para;

	gp_memset((INT8S *)&csi_Para, 0, sizeof(gpCSIPara_t));
	//Enabel mipi clk, set mipi clk
    if(pr2000_mipi_cfg.pixel_clk_sel == GENERATE_PIXEL_CLK)
    {
        drv_l2_mipi_ctrl_set_clk(ENABLE, 4);
        osDelay(10);
    }
	/* set input & output format */
	csi_Para.csi_fmt.input_format = pr2000_csi_mipi_ops.info[index].input_format;
	csi_Para.csi_fmt.output_format = pr2000_csi_mipi_ops.info[index].output_format;

	csi_Para.csi_tim.clki_inv = 1;
	csi_Para.csi_fmt.fifo_size = 0;
    csi_Para.csi_fmt.interface_mode = pr2000_csi_mipi_ops.info[index].interface_mode;
    csi_Para.csi_fmt.preview_mode = CSI_CAPTURE_MODE;
	csi_Para.csi_fmt.clk_stopb = ENABLE;
	csi_Para.csi_tim.fget_mode = ENUM_CSI_RISING_EDGE;
    csi_Para.csi_tim.hrst_mode = ENUM_CSI_RISING_EDGE;
    csi_Para.csi_tim.vadd_mode = ENUM_CSI_RISING_EDGE;
    csi_Para.csi_tim.vrst_mode = ENUM_CSI_RISING_EDGE;
    csi_Para.csi_tim.d_type = ENUM_CSI_LATCH_DELAY_1_CLOCK;
    csi_Para.h_start = csi_Para.v_start = 0x0;
    csi_Para.h_end = csi_Para.v_end = 0xfff;

	csi_Para.target_w = pr2000_csi_mipi_ops.info[index].target_w;
	csi_Para.target_h = pr2000_csi_mipi_ops.info[index].target_h;
	csi_Para.hratio = 0;		// No Scale
	csi_Para.vratio = 0;		// No Scale

    if(drv_l2_csi_set_fmt(PR2000_CSI_DEV,&csi_Para.csi_fmt) < 0)
    {
		DBG_PRINT("csi set fmt err!!!\r\n");
	}

	if(bufA) {
		drv_l2_csi_stream_on(PR2000_CSI_DEV,(gpCSIPara_t *)&csi_Para, bufA, bufB);
	} else {
		DBG_PRINT("Input frame buffer address error, fail to start %s\r\n", pr2000_csi_mipi_ops.name);
		pr2000_csi_mipi_ops.stream_stop();
	}
		//mipi start
	if(drv_l1_mipi_set_parameter(MIPI_DEV_NO, &pr2000_mipi_cfg) < 0)
	{
		DBG_PRINT("MIPI0 init fail !!!\r\n");
	}
    drv_l2_sensor_set_csi_source(SENSOR_INTERFACE_MIPI, PR2000_CSI_DEV, MIPI_DEV_NO);
	/*Init Sensor*/
	pr2000_mipi_yuv_sensor_init();

}

/**
 * @brief   stream stop function
 * @param   none
 * @return 	none
 */
void pr2000_csi_mipi_stream_off(void)
{
	/* Disable interrupt */
	drv_l2_csi_stream_off(PR2000_CSI_DEV);
	DBG_PRINT("%s:no function \r\n", __func__);
}

/**
 * @brief   get info function
 * @param   none
 * @return 	pointer to sensor information data
 */
drv_l2_sensor_info_t *pr2000_csi_mipi_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1))
	{
		return NULL;
	}
	else
	{
		return (drv_l2_sensor_info_t *) &pr2000_csi_mipi_ops.info[index];
	}
}

/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t	pr2000_csi_mipi_ops =
{
    #if(PR2000_CSI_DEV == CSI_0_PATH)
    SOURCE_CSI0_MIPI,               /* sensor name */
    #else
    SOURCE_CSI1_MIPI,               /* sensor name */
    #endif
	pr2000_csi_mipi_init,
	pr2000_csi_mipi_uninit,
	pr2000_csi_mipi_stream_on,
	pr2000_csi_mipi_stream_off,
	pr2000_csi_mipi_get_info,
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
			PR2000_OUT_WIDTH,		/* target width */
			PR2000_OUT_HEIGHT,		/* target height */
			PR2000_WIDTH,			/* sensor width */
			PR2000_HEIGHT,			/* sensor height */
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
			PR2000_OUT_WIDTH,		/* target width */
			PR2000_OUT_HEIGHT,		/* target height */
			PR2000_WIDTH,			/* sensor width */
			PR2000_HEIGHT,			/* sensor height */
			0,						/* sensor h offset */
			0,						/* sensor v offset */
			CSI_HREF,				/* input interface */
			CSI_NON_INTERLACE,		/* interlace mode */
			ENUM_CSI_RISING_EDGE,	/* hsync pin active level */
			ENUM_CSI_RISING_EDGE	/* vsync pin active level */
		},

	 }
};
#endif //(defined _SENSOR_PR2000_AHD_CSI_MIPI_YUV) && (_SENSOR_PR2000_AHD_CSI_MIPI_YUV == 1)

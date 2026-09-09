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
 /*************************************************************************
	Include file
*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drv_l2.h"

#if (defined _SENSOR_GC0308_CSI_DVP) && (_SENSOR_GC0308_CSI_DVP == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define GC0308_ID	    					0x42
#define GC0308_RESET    				IO_F15

#define GC0308_DVP_CSI_DEV	    		CSI_0_PATH
#define CSI_SENSOR_FPS					30

/*ISP Input*/
#define GC0308_DVP_WIDTH		    		640
#define GC0308_DVP_HEIGHT		    	480

/*ISP Output*/
#define GC0308_DVP_OUT_WIDTH	    		640
#define GC0308_DVP_OUT_HEIGHT	    	480
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define GC0308_DVP_I2C_MODE	            	SENSOR_I2C_MODE_SW

#if GC0308_DVP_I2C_MODE == SENSOR_I2C_MODE_SW
	#define GC0308_DVP_GPIO_SCL		    	IO_B4
	#define GC0308_DVP_GPIO_SDA		    	IO_B5
	#define GC0308_DVP_GPIO_DRIVING	    	IOC_DRV_10mA
#else
	#define GC0308_DVP_I2C_DEVICE		I2C_0
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
#if GC0308_DVP_I2C_MODE == SENSOR_I2C_MODE_SW
sccb_config_t gc0308_dvp_sccb_config =
{
	GC0308_DVP_GPIO_SCL,			//scl_port
	GC0308_DVP_GPIO_DRIVING,		//scl_drv
	GC0308_DVP_GPIO_SDA,			//sda_port
	GC0308_DVP_GPIO_DRIVING,		//sda_drv
	IO_F15,			   			 //pwdn_port
	IO_F15, 						//pwdn_drv
	0,							//have_pwdn
	8,							//RegBits
	8,							//DataBits
	GC0308_ID,				//slaveAddr
	0x20000,						//timeout
	100							//clock_rate
};

static void *gc0308_handle;
#elif PR2000_DVP_I2C_MODE == SENSOR_I2C_MODE_HW
static drv_l1_i2c_bus_handle_t gc0308_handle;
#endif


static INT32S gc0308_sccb_open(void)
{
#if GC0308_DVP_I2C_MODE == SENSOR_I2C_MODE_SW
	#if 0
	gc0308_handle = drv_l2_sccb_open(GC0308_ID, 8, 8);
	if(gc0308_handle == 0) {
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
	#else
	gc0308_handle = drv_l2_sccb_open_ext(&gc0308_dvp_sccb_config);
	if(gc0308_handle == 0) {
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
	#endif
#elif PR2000_DVP_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_init(GC0308_DVP_I2C_DEVICE);
	gc0308_dvp_handle.devNumber = GC0308_DVP_I2C_DEVICE;
	gc0308_dvp_handle.slaveAddr = GC0308_ID;
	gc0308_dvp_handle.clkRate = 100;
#endif
	return STATUS_OK;
}

static void gc0308_sccb_close(void)
{
#if GC0308_DVP_I2C_MODE == SENSOR_I2C_MODE_SW
	if(gc0308_handle) {
		drv_l2_sccb_close(gc0308_handle);
		gc0308_handle = NULL;
	}
#elif GC0308_DVP_I2C_MODE == SENSOR_I2C_MODE_HW
	drv_l1_i2c_uninit(GC0308_DVP_I2C_DEVICE);
	gc0308_dvp_handle.devNumber = 0;
	gc0308_dvp_handle.slaveAddr = 0;
	gc0308_dvp_handle.clkRate = 100;
#endif
}

static INT32S gc0308_sccb_write(INT8U reg,  INT8U value)
{
	INT32S ret;
#if GC0308_DVP_I2C_MODE == SENSOR_I2C_MODE_SW
	ret = drv_l2_sccb_write(gc0308_handle, reg, value);
	if (ret < 0)
		DBG_PRINT("gc0308 sccb write fail\r\n");
	return ret;

#elif GC0308_DVP_I2C_MODE == SENSOR_I2C_MODE_HW
	#if 1
	return drv_l1_reg_1byte_data_1byte_write(&gc0308_handle,  reg,  value);
	#else
	INT8U data[2];

	data[0] = reg;
	data[1] = value;
	return drv_l1_i2c_bus_write(&gc0308_handle, data, 2);
	#endif
#endif
}

static INT32S gc0308_dvp_sccb_write_table(regval8_t *pTable)
{
	INT32U retry_cnt = 0;

	while(1)
	{
		if(pTable->reg_num == 0xff && pTable->value == 0xff)
		{
			break;
		}
        //DBG_PRINT("reg = 0x%x, val = 0x%x\r\n", pTable->reg_num, pTable->value);
		if(gc0308_sccb_write(pTable->reg_num, pTable->value) < 0)
		{
			DBG_PRINT("sccb write fail: reg = 0x%x, val = 0x%x\r\n",  pTable->reg_num, pTable->value);
			retry_cnt += 1;
			if(retry_cnt >= 10)
				return STATUS_FAIL;
			continue;
		}
#if 0
		{
			INT8U tt;

		    osDelay(10);
		    gc0308_dvp_sccb_read(pTable->reg_num,&tt);
		//    if(tt!= pTable->value)
			DBG_PRINT("i2c reg[0x%04x] = 0x%04x, read = 0x%04x\r\n", pTable->reg_num, pTable->value, tt);
		}
#endif
		pTable++;
	}

	return STATUS_OK;
}


#if 1
static INT32S gc0308_sccb_read(INT8U reg, INT8U *value)
{
#if GC0308_DVP_I2C_MODE == SENSOR_I2C_MODE_SW
	INT16U data;

	if(drv_l2_sccb_read(gc0308_handle,  reg,  &data) >= 0) {
		*value = (INT8U)data;
		//DBG_PRINT("0x%02x, 0x%02x\r\n", reg, *value);
		return STATUS_OK;
	} else {
		*value = 0xFF;
		//DBG_PRINT("0x%02x, 0x%02x\r\n", reg, *value);
		return STATUS_FAIL;
	}
#elif GC0308_DVP_I2C_MODE == SENSOR_I2C_MODE_HW
	#if 1
		if(drv_l1_reg_1byte_data_1byte_read(&gc0308_handle,  reg,  value) >= 0)
	{
		DBG_PRINT("0x%02x, 0x%02x\r\n", reg,  *value);
		return STATUS_OK;
	}
	else
	{
		DBG_PRINT("sccb_read fail...\r\n");
		return STATUS_FAIL;
	}
	#else

	INT8U data[1];

	data[0] = reg;
	if(drv_l1_i2c_bus_write(&gc0308_handle, data, 1) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}

	if(drv_l1_i2c_bus_read(&gc0308_handle, data, 1) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}

	*value = data[0];
	#endif
#endif
	return STATUS_OK;
}
#endif

INT8U GC0308_ReadY(void)
{
	INT8U value;

	gc0308_sccb_read(0xd4, &value);
	return value;
}

INT8U GC0308_SetY(INT8U Value)
{
	return gc0308_sccb_write(0xd3, Value); //0x48
}

static regval8_t  gc0308_dvp_init[] =
{
	//640x480 init registers code.
	{0xfe, 0x80},

	{0xfe, 0x00},  // set page0

	{0xd2, 0x10},  // close AEC
	{0x22, 0x55},  // close AWB

	{0x5a, 0x56},
	{0x5b, 0x40},
	{0x5c, 0x4a},

	{0x22, 0x57}, // Open AWB

	{0x01, 0x6a},
	{0x02, 0x70},
	{0x0f, 0x00},
	{0xe2, 0x00},
	{0xe3, 0x96},
	{0xe4, 0x02},
	{0xe5, 0x58},
	{0xe6, 0x02},
	{0xe7, 0x58},
	{0xe8, 0x02},
	{0xe9, 0x58},
	{0xea, 0x04},
	{0xeb, 0xb0},
	{0xec, 0x20},


	{0x05, 0x00},
	{0x06, 0x00},
	{0x07, 0x00},
	{0x08, 0x00},
	{0x09, 0x01},
	{0x0a, 0xe8},
	{0x0b, 0x02},
	{0x0c, 0x88},
	{0x0d, 0x02},
	{0x0e, 0x02},
	{0x10, 0x22},
	{0x11, 0xfd},
	{0x12, 0x0a},
	{0x13, 0x00},

	{0x14, 0x13},// change direction  10:normal , 11:H SWITCH,12: V SWITCH, 13:H&V SWITCH
	//{0x14, 0x11},// change direction  10:normal , 11:H SWITCH,12: V SWITCH, 13:H&V SWITCH

	{0x15, 0x0a},
	{0x16, 0x05},
	{0x17, 0x01},
	{0x18, 0x44},
	{0x19, 0x44},
	{0x1a, 0x1e},
	{0x1b, 0x00},
	{0x1c, 0xc1},
	{0x1d, 0x08},
	{0x1e, 0x60},
	{0x1f, 0x16}, //pad drv ,00 03 13 1f 3f james remarked

	{0x20, 0xff},
	{0x21, 0xf8},
	{0x22, 0x57},
	{0x24, 0xa2}, 	// YUV
	//{0x24, 0xB7}, //RAW10
	//{0x24, 0xa6},  // RGB
	{0x25, 0x0f},

	//output sync_mode
	{0x26, 0x02},//vsync  maybe need changed, value is 0x02
	{0x2f, 0x01},
	{0x30, 0xf7},
	{0x31, 0x50},
	{0x32, 0x00},
	{0x39, 0x04},
	{0x3a, 0x18},
	{0x3b, 0x20},
	{0x3c, 0x00},
	{0x3d, 0x00},
	{0x3e, 0x00},
	{0x3f, 0x00},
	{0x50, 0x10},
	{0x53, 0x82},
	{0x54, 0x80},
	{0x55, 0x80},
	{0x56, 0x82},

	{0x57, 0x80},  // R
	{0x58, 0x80},  // G
	{0x59, 0x80},  // B

	{0x8b, 0x40},
	{0x8c, 0x40},
	{0x8d, 0x40},
	{0x8e, 0x2e},
	{0x8f, 0x2e},
	{0x90, 0x2e},
	{0x91, 0x3c},
	{0x92, 0x50},
	{0x5d, 0x12},
	{0x5e, 0x1a},
	{0x5f, 0x24},
	{0x60, 0x07},
	{0x61, 0x15},
	{0x62, 0x08},
	{0x64, 0x03},
	{0x66, 0xe8},
	{0x67, 0x86},
	{0x68, 0xa2},
	{0x69, 0x18},
	{0x6a, 0x0f},
	{0x6b, 0x00},
	{0x6c, 0x5f},
	{0x6d, 0x8f},
	{0x6e, 0x55},
	{0x6f, 0x38},
	{0x70, 0x15},
	{0x71, 0x33},
	{0x72, 0xdc},
	{0x73, 0x80},
	{0x74, 0x02},
	{0x75, 0x3f},
	{0x76, 0x02},
	{0x77, 0x36},
	{0x78, 0x88},
	{0x79, 0x81},
	{0x7a, 0x81},
	{0x7b, 0x22},
	{0x7c, 0xff},
	{0x93, 0x48},
	{0x94, 0x00},
	{0x95, 0x05},
	{0x96, 0xe8},
	{0x97, 0x40},
	{0x98, 0xf0},
	{0xb1, 0x38},
	{0xb2, 0x38},
	{0xbd, 0x38},
	{0xbe, 0x36},
	{0xd0, 0xc9},
	{0xd1, 0x10},
	//{0xd2, 0x90},
	//{0xd3, 0x80},
	{0xd3, 0xA0},		 // ?תצ
	{0xd5, 0xf2},
	{0xd6, 0x16},
	{0xdb, 0x92},
	{0xdc, 0xa5},
	{0xdf, 0x23},
	{0xd9, 0x00},
	{0xda, 0x00},
	{0xe0, 0x09},
	{0xec, 0x20},
	{0xed, 0x04},
	{0xee, 0xa0},
	{0xef, 0x40},
	{0x80, 0x03},
	{0x80, 0x03},

	//smallest gamma curve
	{0x9F, 0x0B},
	{0xA0, 0x16},
	{0xA1, 0x29},
	{0xA2, 0x3C},
	{0xA3, 0x4F},
	{0xA4, 0x5F},
	{0xA5, 0x6F},
	{0xA6, 0x8A},
	{0xA7, 0x9F},
	{0xA8, 0xB4},
	{0xA9, 0xC6},
	{0xAA, 0xD3},
	{0xAB, 0xDD},
	{0xAC, 0xE5},
	{0xAD, 0xF1},
	{0xAE, 0xFA},
	{0xAF, 0xFF},

	{0xc0, 0x00},
	{0xc1, 0x10},
	{0xc2, 0x1C},
	{0xc3, 0x30},
	{0xc4, 0x43},
	{0xc5, 0x54},
	{0xc6, 0x65},
	{0xc7, 0x75},
	{0xc8, 0x93},
	{0xc9, 0xB0},
	{0xca, 0xCB},
	{0xcb, 0xE6},
	{0xcc, 0xFF},
	{0xf0, 0x02},
	{0xf1, 0x01},
	{0xf2, 0x01},
	{0xf3, 0x30},
	{0xf9, 0x9f},
	{0xfa, 0x78},

	//---------------------------------------------------------------
	{0xfe, 0x01},  //set page 1

	{0x00, 0xf5},
	{0x02, 0x1a},
	{0x0a, 0xa0},
	{0x0b, 0x60},
	{0x0c, 0x08},
	{0x0e, 0x4c},
	{0x0f, 0x39},

	{0x11, 0x3f},
	//{0x11 ,0x37},  // bit3 = 0

	{0x12, 0x72},
	{0x13, 0x13},
	{0x14, 0x42},
	{0x15, 0x43},
	{0x16, 0xc2},
	{0x17, 0xa8},
	{0x18, 0x18},
	{0x19, 0x40},
	{0x1a, 0xd0},
	{0x1b, 0xf5},
	{0x70, 0x40},
	{0x71, 0x58},
	{0x72, 0x30},
	{0x73, 0x48},
	{0x74, 0x20},
	{0x75, 0x60},
	{0x77, 0x20},
	{0x78, 0x32},
	{0x30, 0x03},
	{0x31, 0x40},
	{0x32, 0xe0},
	{0x33, 0xe0},
	{0x34, 0xe0},
	{0x35, 0xb0},
	{0x36, 0xc0},
	{0x37, 0xc0},
	{0x38, 0x04},
	{0x39, 0x09},
	{0x3a, 0x12},
	{0x3b, 0x1C},
	{0x3c, 0x28},
	{0x3d, 0x31},
	{0x3e, 0x44},
	{0x3f, 0x57},
	{0x40, 0x6C},
	{0x41, 0x81},
	{0x42, 0x94},
	{0x43, 0xA7},
	{0x44, 0xB8},
	{0x45, 0xD6},
	{0x46, 0xEE},
	{0x47, 0x0d},

	{0xfe, 0x00},

	{0xd2, 0x90}, // Open AEC at last.

	/////////////////////////////////////////////////////////
	//-----------Update the registers -------------//
	///////////////////////////////////////////////////////////

	{0xfe, 0x00},//set Page0

	{0x10, 0x26},
	{0x11, 0x0d},// fd,modified by mormo 2010/07/06
	{0x1a, 0x2a},// 1e,modified by mormo 2010/07/06

	{0x1c, 0x49}, // c1,modified by mormo 2010/07/06
	{0x1d, 0x9a}, // 08,modified by mormo 2010/07/06
	{0x1e, 0x61}, // 60,modified by mormo 2010/07/06
	//{0x1f, 0x16}, // io driver current

	{0x3a, 0x20},

	{0x50, 0x14},// 10,modified by mormo 2010/07/06
	{0x53, 0x80},
	{0x56, 0x80},

	{0x8b, 0x20}, //LSC
	{0x8c, 0x20},
	{0x8d, 0x20},
	{0x8e, 0x14},
	{0x8f, 0x10},
	{0x90, 0x14},

	{0x94, 0x02},
	{0x95, 0x07},
	{0x96, 0xe0},

	{0xb1, 0x40}, // YCPT
	{0xb2, 0x40},
	{0xb3, 0x40},
	{0xb6, 0xe0},

	{0xd0, 0xcb}, // AECT  c9,modifed by mormo 2010/07/06
	{0xd3, 0x48}, // 80,modified by mormor 2010/07/06

	{0xf2, 0x02},
	{0xf7, 0x12},
	{0xf8, 0x0a},

	{0xfe, 0x01},//set  Page1

	{0x02, 0x20},
	{0x04, 0x10},
	{0x05, 0x08},
	{0x06, 0x20},
	{0x08, 0x0a},

	{0x0e, 0x44},
	{0x0f, 0x32},
	{0x10, 0x41},
	{0x11, 0x37},
	{0x12, 0x22},
	{0x13, 0x19},
	{0x14, 0x44},
	{0x15, 0x44},

	{0x19, 0x50},
	{0x1a, 0xd8},

	{0x32, 0x10},

	{0x35, 0x00},
	{0x36, 0x80},
	{0x37, 0x00},

	//-----------Update the registers end---------//
	{0xfe, 0x00},// set back for page1

	{0xff, 0xff} //End
};

/**
 * @brief   gc0308 initialization function
 * @param   sensor format parameters
 * @return 	none
 */
void gc0308_csi_init(void)
{
	#ifdef GC0308_RESET
	gpio_set_port_attribute(GC0308_RESET, ATTRIBUTE_HIGH);
	gpio_init_io(GC0308_RESET,GPIO_OUTPUT);
	gpio_write_io(GC0308_RESET,DATA_HIGH);
	osDelay(100);
	gpio_write_io(GC0308_RESET,DATA_LOW);
	osDelay(100);
	gpio_write_io(GC0308_RESET,DATA_HIGH);
	#endif

	// request sccb
	gc0308_sccb_open();
	// CSI init
	 drv_l1_csi_init(GC0308_DVP_CSI_DEV);

	DBG_PRINT("Sensor GC0308 csi interface init completed\r\n");
}

/**
 * @brief   gc0308 un-initialization function
 * @param   sensor format parameters
 * @return 	none
 */
void gc0308_csi_uninit(void)
{
    	// csi disable
    	drv_l2_csi_stop(GC0308_DVP_CSI_DEV);
	// release sccb
	gc0308_sccb_close();
	// CSI uninit
    	drv_l1_csi_uninit(GC0308_DVP_CSI_DEV);

	#ifdef GC0308_RESET
	gpio_write_io(GC0308_RESET, DATA_LOW);
	#endif
	// disable mclk
	drv_l2_sensor_set_mclkout(MCLK_NONE,  GC0308_DVP_CSI_DEV);
}

/**
 * @brief   gc0308 stream start function
 * @param   info index
 *
 * @return 	none
 */
void gc0308_csi_stream_start(INT32U index, INT32U bufA, INT32U bufB)
{
	gpCSIPara_t csi_Para;

	gp_memset((INT8S *)&csi_Para, 0, sizeof(gpCSIPara_t));
	/* set input & output format */
	csi_Para.csi_fmt.input_format = gc0308_sensor_csi_ops.info[index].input_format;
	csi_Para.csi_fmt.output_format = gc0308_sensor_csi_ops.info[index].output_format;

	csi_Para.csi_fmt.interface_mode = CSI_HREF;
	csi_Para.csi_fmt.preview_mode = CSI_CAPTURE_MODE;
	csi_Para.csi_fmt.clk_stopb = ENABLE;
	csi_Para.csi_tim.hrst_mode = ENUM_CSI_RISING_EDGE;
	csi_Para.csi_tim.vadd_mode = ENUM_CSI_RISING_EDGE;
	csi_Para.csi_tim.vrst_mode = ENUM_CSI_RISING_EDGE;
	csi_Para.csi_tim.d_type = DELAY_1CLOCK;
	csi_Para.h_start = csi_Para.v_start = 0x0;
	csi_Para.h_end = csi_Para.v_end = 0xfff;

	switch(index)
	{
	case 0:
		csi_Para.target_w = gc0308_sensor_csi_ops.info[index].target_w;
		csi_Para.target_h = gc0308_sensor_csi_ops.info[index].target_h;
		csi_Para.hratio = 0;		// No Scale
		csi_Para.vratio = 0;		// No Scale
		break;

	case 1:
		csi_Para.target_w = gc0308_sensor_csi_ops.info[index].target_w;
		csi_Para.target_h = gc0308_sensor_csi_ops.info[index].target_h * 2;
		csi_Para.hratio = 0x0102;	// Scale to 1/2
		csi_Para.vratio = 0x0102;	// Scale to 1/2
		break;

	case 2:
		csi_Para.target_w = gc0308_sensor_csi_ops.info[index].target_w;
		csi_Para.target_h = gc0308_sensor_csi_ops.info[index].target_h * 2;
		csi_Para.hratio = 0x0104;	// Scale to 1/4
		csi_Para.vratio = 0x0104;	// Scale to 1/4
		break;
	case 3:
		csi_Para.target_w = gc0308_sensor_csi_ops.info[index].target_w;
		csi_Para.target_h = gc0308_sensor_csi_ops.info[index].target_h;
		csi_Para.hratio = 0;		// No Scale
		csi_Para.vratio = 0;		// No Scale
		break;
	default:
		while(1);
	}

	/* Set sensor's registers via SCCB */
#if 0
	if(gc0308_init() < 0) {
		DBG_PRINT("gc0308 init fail!!!\r\n");
	}
#else
	if(gc0308_dvp_sccb_write_table((regval8_t *)gc0308_dvp_init) < 0) {
		DBG_PRINT("gc0308_sccb_write_table fail!!!\r\n");
    }
#endif
	DBG_PRINT("%s = %d\r\n", __func__, index);

	if(drv_l2_csi_set_fmt(GC0308_DVP_CSI_DEV, &csi_Para.csi_fmt) < 0)
	{
		DBG_PRINT("csi set fmt err!!!\r\n");
	}
//bufA = 0x41000000;
	if(bufA) {
		drv_l2_csi_stream_on(GC0308_DVP_CSI_DEV ,(gpCSIPara_t *)&csi_Para, bufA, 0);
	} else {
		DBG_PRINT("Input frame buffer address error, fail to start %s\r\n", gc0308_sensor_csi_ops.name);
		gc0308_sensor_csi_ops.stream_stop();
	}
	DBG_PRINT("gc0308_csi_dvp_init done!!!\r\n");
}

/**
 * @brief   gc0308 stream stop function
 * @param   none
 * @return 	none
 */
void gc0308_csi_stream_stop(void)
{
	 drv_l2_csi_stream_off(GC0308_DVP_CSI_DEV);
}

/**
 * @brief   gc0308 get info function
 * @param   none
 * @return 	pointer to sensor information data
 */
drv_l2_sensor_info_t* gc0308_csi_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1)){
		return NULL;
	} else {
		return (drv_l2_sensor_info_t*)&gc0308_sensor_csi_ops.info[index];
	}
}

void sensor_register_ae_ctrl(INT32U *handle) __attribute__((weak));
void sensor_register_ae_ctrl(INT32U *handle)
{
	*handle = (INT32U) NULL;
}
/*
void sensor_get_ae_info(sensor_exposure_t *si) __attribute__((weak));
void sensor_get_ae_info(sensor_exposure_t *si)
{
    //memcpy(si, &seInfo, sizeof(sensor_exposure_t));
}
*/
/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t	gc0308_sensor_csi_ops =
{
	#if(GC0308_DVP_CSI_DEV == CSI_0_PATH)
	SOURCE_CSI0_DVP,               /* sensor name */
	#else
	SOURCE_CSI1_DVP,               /* sensor name */
	#endif
	gc0308_csi_init,
	gc0308_csi_uninit,
	gc0308_csi_stream_start,
	gc0308_csi_stream_stop,
	gc0308_csi_get_info,
	NULL,
	NULL,
	NULL,
	NULL,
	{
		/* 1rd info */
		{
			MCLK_24M,					/* CSI clock */
			CSI_IN_YUYV,			/* input format */
			#if C_AHDCSI_GP420_EN
			CSI_OUT_GP420,			/* output format */
			#else
			CSI_OUT_YUYV,			/* output format */
			#endif
			CSI_SENSOR_FPS,				/* FPS in sensor */
			GC0308_DVP_OUT_WIDTH,		/* target width */
			GC0308_DVP_OUT_HEIGHT,		/* target height */
			GC0308_DVP_WIDTH,			/* sensor width */
			GC0308_DVP_HEIGHT,			/* sensor height */
			0,							/* sensor h offset */
			0,							/* sensor v offset */
			MODE_CCIR_HREF,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_LOW,			/* vsync pin active level */
		},
		/* 2nd info */
		{
			MCLK_24M,					/* CSI clock */
			CSI_IN_YUYV,			/* input format */
			CSI_OUT_YUYV,			/* output format */
			CSI_SENSOR_FPS,				/* FPS in sensor */
			GC0308_DVP_OUT_WIDTH,		/* target width */
			GC0308_DVP_OUT_HEIGHT,		/* target height */
			GC0308_DVP_WIDTH,			/* sensor width */
			GC0308_DVP_HEIGHT,			/* sensor height */
			0,							/* sensor h offset */
			0,							/* sensor v offset */
			MODE_CCIR_HREF,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_LOW,			/* vsync pin active level */
		},
	}
};
#endif //(defined _SENSOR_GC03080_CSI) && (_SENSOR_GC03080_CSI == 1)

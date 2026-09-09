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

#if (defined _DW9714P) && (_DW9714P == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
//For DW9714P
#define	DW9714P_ID		    (0x18)//0X18
#define	DW9714P_XSD_PIN		IO_B15//ShutDown mode(active low)


#define VCM_MIN_CURRENT         10  //30
#define VCM_MAX_CURRENT         110 //100

#define DW9714P_MIN_STEP        (float)(120.0 / 1023.0)
#define DW9714P_MIN_CURRENT     (VCM_MIN_CURRENT / DW9714P_MIN_STEP)
#define DW9714P_MAX_CURRENT     (VCM_MAX_CURRENT / DW9714P_MIN_STEP)

#define TOTAL_STEPS_ACTIVE      64
#define DW9714P_STEP            ((DW9714P_MAX_CURRENT - DW9714P_MIN_CURRENT) / TOTAL_STEPS_ACTIVE)

#define DW9714P_I2C_MODE	            			ACTUATOR_I2C_MODE_SW

#if DW9714P_I2C_MODE == ACTUATOR_I2C_MODE_SW
#define	ACTUATOR_I2C_SCL							IO_B10
#define	ACTUATOR_I2C_SDA							IO_B13
#define	ACTUATOR_I2C_DRIVING						IOA_DRV_30mA
#else
#define	DW9714P_I2C_DEVICE						I2C_0
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct actuator_t {
	INT32S pos;
	INT32S cur_pos;
	INT32S dir;
	INT32U step;
	INT32U total_step;
	INT32U max, min;
	INT16U table[TOTAL_STEPS_ACTIVE];
} actuator_s;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
actuator_s  dw9714p_act;
static void *dw9714p_handle;

static INT32S dw9714p_sccb_open(void)
{
#if DW9714P_I2C_MODE == ACTUATOR_I2C_MODE_SW
	sccb_config_t cfg;
	cfg.scl_port = ACTUATOR_I2C_SCL;
	cfg.scl_drv = IO_DRIVING_lvl0;
	cfg.sda_port = ACTUATOR_I2C_SDA;
	cfg.sda_drv = IO_DRIVING_lvl0;
	cfg.pwdn_port = 0;
	cfg.pwdn_drv = 0;
	cfg.have_pwdn = 0;
	cfg.RegBits = 8;
	cfg.DataBits = 8;
	cfg.slaveAddr = DW9714P_ID;
	cfg.timeout = 0x20000;
	cfg.clock_rate = 100;

	dw9714p_handle = drv_l2_sccb_open_ext(&cfg);

	if(dw9714p_handle == 0)
	{
		DBG_PRINT("dw9714p Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#endif
	return STATUS_OK;
}

static void dw9714p_sccb_close(void)
{
#if DW9714P_I2C_MODE == ACTUATOR_I2C_MODE_SW
	if(dw9714p_handle)
	{
		drv_l2_sccb_close(dw9714p_handle);
		dw9714p_handle = NULL;
	}
#endif
}


static INT32S dw9714p_normal_sccb_write(INT16U value)
{
#if DW9714P_I2C_MODE == ACTUATOR_I2C_MODE_SW
	//return drv_l2_sccb_write2(dw9714p_handle, value);
	return drv_l2_sccb_write(dw9714p_handle,value&0xFF, ((value>>8)&0xFF));
#endif
}


static INT32S dw9714p_sccb_write(INT8U reg, INT16U value)
{
#if DW9714P_I2C_MODE == ACTUATOR_I2C_MODE_SW
	return drv_l2_sccb_write(dw9714p_handle, reg, value);
#endif
}
/*
static INT32S dw9714p_normal_sccb_read(INT16U *value)
{
#if DW9714P_I2C_MODE == ACTUATOR_I2C_MODE_SW
	INT16U data;

	if(drv_l2_dw9714p_sccb_read(dw9714p_handle, &data) >= 0)
	{
		*value = ((data>>8)&0xFF)|((data<<8)&0xFF00);
		return STATUS_OK;
	}
	else
	{
		*value = 0xFFFF;
		return STATUS_FAIL;
	}
#endif
}
*/
static INT32S dw9714p_advanced_sccb_write(INT8U reg, INT8U value)
{
#if DW9714P_I2C_MODE == ACTUATOR_I2C_MODE_SW
	return drv_l2_sccb_write(dw9714p_handle, reg, value);
#endif
}

static INT32S dw9714p_advanced_sccb_read(INT8U reg, INT8U *value)
{
#if DW9714P_I2C_MODE == ACTUATOR_I2C_MODE_SW
	INT16U data;

	if(drv_l2_sccb_read(dw9714p_handle, reg, &data) >= 0)
	{
		*value = (INT8U)data;
		return STATUS_OK;
	}
	else
	{
		*value = 0xFF;
		return STATUS_FAIL;
	}
#endif
	return STATUS_OK;
}

INT32S dw9714p_init(void)
{//IC Manufacturer ID, IC Model
	INT8U value;
	INT32S ret = 0;
	INT16U  sensor_id;
	int test = 0;
	int test_inc = 100;
	int data = 0;
	int i;


	DBG_PRINT("***************************************\r\n");
	DBG_PRINT("%s.\r\n", __FUNCTION__);
	DBG_PRINT("***************************************\r\n");

	//DW9714P_XSD_PIN:IO_D10//ShutDown mode(active low)
	gpio_init_io(DW9714P_XSD_PIN, GPIO_OUTPUT);
	gpio_set_port_attribute(DW9714P_XSD_PIN, ATTRIBUTE_HIGH);
	gpio_write_io(DW9714P_XSD_PIN, DATA_HIGH);
	osDelay(10);
	//Normal Mode

	// reguest sccb
	dw9714p_sccb_open();
	osDelay(10);
#if 1
	/*//PD --> '1' //Byte1[7] Byte2
	if(dw9714p_normal_sccb_write(0x0080)<0)
		return STATUS_FAIL;
	osDelay(1);
	//PD --> '0' //Byte1[7] Byte2
	if(dw9714p_normal_sccb_write(0x0)<0)
		return STATUS_FAIL;
	osDelay(1);
*/
#if 0
	//Enter Advanced Mode
	if(dw9714p_normal_sccb_write(0xABED)<0)
		return STATUS_FAIL;
	osDelay(1);

	//Get IC Manufacturer ID, IC Model
	if(dw9714p_advanced_sccb_read(0x00, &value)<0)
		return STATUS_FAIL;

	//dw9714p_sccb_close();
	DBG_PRINT("IC Manufacturer ID[7:4], IC Model[3:0] = 0x%X\r\n",value);
	if(0xFE != value)
	return STATUS_FAIL;
#else
	if(dw9714p_normal_sccb_write(0xA3EC)<0) return STATUS_FAIL;
		osDelay(1);

	if(dw9714p_normal_sccb_write(0x05A1)<0) return STATUS_FAIL;
		osDelay(1);

	//if(dw9714p_normal_sccb_write(0x00F2)<0) return STATUS_FAIL;
	//	osDelay(1);

	if(dw9714p_normal_sccb_write(0x51DC)<0) return STATUS_FAIL;
		osDelay(1);

#endif
    /* if(dw9714p_sccb_write(0xEC,0xA3)<0) return STATUS_FAIL;
		osDelay(1);
	 if(dw9714p_sccb_write(0xA16,0x0D)<0) return STATUS_FAIL;
		osDelay(1);
	 if(dw9714p_sccb_write(0xF2,0x00)<0) return STATUS_FAIL;
		osDelay(1);
    if(dw9714p_sccb_write(0xDC,0x51)<0) return STATUS_FAIL;
		osDelay(1);
    if(dw9714p_sccb_write(0xDF,0x5B)<0) return STATUS_FAIL;
		osDelay(1);*/
	dw9714p_act.cur_pos = 0;
	dw9714p_act.dir = 0;
	dw9714p_act.pos = 0;
	dw9714p_act.step = 4;
	dw9714p_act.max = DW9714P_MAX_CURRENT;
	dw9714p_act.min = DW9714P_MIN_CURRENT;
	dw9714p_act.total_step = TOTAL_STEPS_ACTIVE;

	dw9714p_act.table[0] = dw9714p_act.min;
	for(i = 1 ; i < dw9714p_act.total_step - 1; i++)
	{
		dw9714p_act.table[i] = dw9714p_act.min + i * DW9714P_STEP;
	}
	dw9714p_act.table[dw9714p_act.total_step - 1] = dw9714p_act.max;


	data = dw9714p_act.min;
	test_inc = 1;
	test = 0;
	while(1)
	{
		int ret;

		test += test_inc;
		if(test < 0)
			break;
		else if(test >= dw9714p_act.total_step)
		{
			test_inc = -1;
			test = dw9714p_act.total_step - 1;
		}

		data = dw9714p_act.table[test];
		//DBG_PRINT("data = %d\r\n", data);

		//DBG_PRINT("data = %d\r\n", data);
		ret = dw9714p_advanced_sccb_write(0x03, data >> 8);
		if(ret != 0)
			DBG_PRINT("sccb err\r\n");
		ret = dw9714p_advanced_sccb_write(0x04, data & 0xff);
		if(ret != 0)
			DBG_PRINT("sccb err\r\n");

		osDelay(10);
	}


	return STATUS_OK;
#else
	//For GC5035 Read ID Test.
	//drv_l2_sensor_set_mclkout(MCLK_24M);
	osDelay(20);
	if(dw9714p_advanced_sccb_read(0x00, &value)>=0)
	{
		sensor_id = value;
		sensor_id <<= 8;
	}
	else
	{
		return STATUS_FAIL;
	}
	if(dw9714p_advanced_sccb_read(0x00, &value)>=0)
	{
		sensor_id += value;
	}
	else
	{
		return STATUS_FAIL;
	}
	dw9714p_sccb_close();
	DBG_PRINT("sensor_id = 0x%x\r\n",sensor_id);

	if(0x5035 != sensor_id)
	return STATUS_FAIL;

	return STATUS_OK;
#endif
}


static INT32S dw9714p_set_val(unsigned short val)
{
	int ret;
	unsigned short data = dw9714p_act.table[val];

	DBG_PRINT("data = %d\r\n", data);
	ret = dw9714p_advanced_sccb_write(0x03, data >> 8);
	if(ret != 0)
		DBG_PRINT("sccb err\r\n");
	ret = dw9714p_advanced_sccb_write(0x04, data & 0xff);
	if(ret != 0)
		DBG_PRINT("sccb err\r\n");

	return ret;
}

void dw9714p_manul_mov_pos(unsigned short data){
	int ret;
	DBG_PRINT("data = %x %x %x \r\n", data >> 8,data & 0xff,data);
	ret = dw9714p_advanced_sccb_write(0x03, data >> 8);
	if(ret != 0)
		DBG_PRINT("sccb err\r\n");
	ret = dw9714p_advanced_sccb_write(0x04, data & 0xff);
	if(ret != 0)
		DBG_PRINT("sccb err\r\n");

 }


void dw9714p_mov_pos(INT32S dest_pos)
{
	INT32S target_pos;
	#if 1
	DBG_PRINT("dw9714p_mov_pos %d %d\r\n",dw9714p_act.cur_pos,dest_pos);
	target_pos = dw9714p_act.cur_pos;
	while (target_pos != dest_pos)
	{
		int ret;
		int step;

		step = (dest_pos - target_pos) >> 1;

		if(step == 0)
		{
			if(target_pos < dest_pos)
			step = 1;
			else
			step = -1;
		}

		target_pos+=step;

		ret = dw9714p_set_val(target_pos);
		if (ret == 0)
		{
			dw9714p_act.cur_pos = target_pos;
			osDelay(1);
		}
		else
			break;
	}
	#endif
}


INT32S dw9714p_uninit(void)
{
	//DW9714P_XSD_PIN:IO_D10//ShutDown mode(active low)
	gpio_init_io(DW9714P_XSD_PIN, GPIO_OUTPUT);
	gpio_set_port_attribute(DW9714P_XSD_PIN, ATTRIBUTE_HIGH);
	gpio_write_io(DW9714P_XSD_PIN, DATA_LOW);
	//osDelay(50);
}

const drv_l2_actuator_ops_t dw9714p_act_ops =
{
	DW9714P_ACT_NAME,
	dw9714p_init,
	dw9714p_uninit,
	dw9714p_mov_pos
};

#endif

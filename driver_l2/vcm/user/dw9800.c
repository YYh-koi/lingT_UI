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
#include "application.h"
#if (defined _DW9800) && (_DW9800 == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
//For DW9800
#define	DW9800_ID		    (0x18)
#define	DW9800_XSD_PIN		IO_B15//ShutDown mode(active low)

 
#define VCM_MIN_CURRENT        10  //30//50
#define VCM_MAX_CURRENT        110 //110 //100//80

#define DW9800_MIN_STEP        (float)(120.0 / 1023.0)//120.0//120/1023
#define DW9800_MIN_CURRENT     (VCM_MIN_CURRENT / DW9800_MIN_STEP)
#define DW9800_MAX_CURRENT     (VCM_MAX_CURRENT / DW9800_MIN_STEP)

#define TOTAL_STEPS_ACTIVE      64//64
#define DW9800_STEP            ((DW9800_MAX_CURRENT - DW9800_MIN_CURRENT) / TOTAL_STEPS_ACTIVE)

#define DW9800_I2C_MODE	            			ACTUATOR_I2C_MODE_SW

#if DW9800_I2C_MODE == ACTUATOR_I2C_MODE_SW
#define	ACTUATOR_I2C_SCL							IO_B10
#define	ACTUATOR_I2C_SDA							IO_B13
#define	ACTUATOR_I2C_DRIVING						IOA_DRV_30mA
#else 
#define	DW9800_I2C_DEVICE						I2C_0
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
actuator_s  dw9800_act;
static void *dw9800_handle;
 
static INT32S dw9800_sccb_open(void)
{
#if DW9800_I2C_MODE == ACTUATOR_I2C_MODE_SW
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
	cfg.slaveAddr = DW9800_ID;
	cfg.timeout = 0x20000;
	cfg.clock_rate = 100;

	dw9800_handle = drv_l2_sccb_open_ext(&cfg);

	if(dw9800_handle == 0)
	{
		DBG_PRINT("dw9800 Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#endif
	return STATUS_OK;
}

static void dw9800_sccb_close(void)
{
#if DW9800_I2C_MODE == ACTUATOR_I2C_MODE_SW
	if(dw9800_handle)
	{
		drv_l2_sccb_close(dw9800_handle);
		dw9800_handle = NULL;
	}
#endif
}

/*
static INT32S dw9800_normal_sccb_write(INT16U value)
{
#if DW9800_I2C_MODE == ACTUATOR_I2C_MODE_SW
	//return drv_l2_sccb_write2(dw9800_handle, value);
	return drv_l2_sccb_write(dw9800_handle, reg, value);
#endif
}

*/

static INT32S dw9800_sccb_write(INT8U reg, INT16U value)
{
#if DW9800_I2C_MODE == ACTUATOR_I2C_MODE_SW
	return drv_l2_sccb_write(dw9800_handle, reg, value);
#endif
}
/*
static INT32S dw9800_normal_sccb_read(INT16U *value)
{
#if DW9800_I2C_MODE == ACTUATOR_I2C_MODE_SW
	INT16U data;

	//if(drv_l2_dw9800_sccb_read(dw9800_handle, &data) >= 0)
	if(drv_l2_sccb_read(dw9800_handle, &data) >= 0)
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
static INT32S dw9800_advanced_sccb_write(INT8U reg, INT8U value)
{
#if DW9800_I2C_MODE == ACTUATOR_I2C_MODE_SW
	return drv_l2_sccb_write(dw9800_handle, reg, value);
#endif
}

static INT32S dw9800_advanced_sccb_read(INT8U reg, INT8U *value)
{
#if DW9800_I2C_MODE == ACTUATOR_I2C_MODE_SW
	INT16U data;

	if(drv_l2_sccb_read(dw9800_handle, reg, &data) >= 0)
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

INT32S dw9800_init(void)
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

	//DW9800_XSD_PIN:IO_D10//ShutDown mode(active low)
	gpio_init_io(DW9800_XSD_PIN, GPIO_OUTPUT);
	gpio_set_port_attribute(DW9800_XSD_PIN, ATTRIBUTE_HIGH);
	gpio_write_io(DW9800_XSD_PIN, DATA_HIGH);
	osDelay(10);
	//Normal Mode

	// reguest sccb
	dw9800_sccb_open();
	osDelay(10);
#if 1
	//PD --> '1' //Byte1[7] Byte2
	/*if(dw9800_normal_sccb_write(0x0080)<0)
		return STATUS_FAIL;
	osDelay(1);
	//PD --> '0' //Byte1[7] Byte2
	if(dw9800_normal_sccb_write(0x0)<0)
		return STATUS_FAIL;
	osDelay(1);*/

#if 0
	//Enter Advanced Mode
	if(dw9800_normal_sccb_write(0xABED)<0)
		return STATUS_FAIL;
	osDelay(1);

	//Get IC Manufacturer ID, IC Model
	if(dw9800_advanced_sccb_read(0x00, &value)<0)
		return STATUS_FAIL;

	//dw9800_sccb_close();
	DBG_PRINT("IC Manufacturer ID[7:4], IC Model[3:0] = 0x%X\r\n",value);
	if(0xFE != value)
	return STATUS_FAIL;
#else
	/*if(dw9800_normal_sccb_write(0xA3EC)<0) return STATUS_FAIL;
		osDelay(1);

	if(dw9800_normal_sccb_write(0x05A1)<0) return STATUS_FAIL;
		osDelay(1);

	//if(dw9800_normal_sccb_write(0x00F2)<0) return STATUS_FAIL;
	//	osDelay(1);

	if(dw9800_normal_sccb_write(0x51DC)<0) return STATUS_FAIL;
		osDelay(1);
*/
#endif

	  if(dw9800_sccb_write(0x02,0x02)<0) return STATUS_FAIL;
		osDelay(1);
	 if(dw9800_sccb_write(0x06,0x40)<0) return STATUS_FAIL;//40
		osDelay(1);
	 if(dw9800_sccb_write(0x07,0x7a)<0) return STATUS_FAIL;//7a
		osDelay(1);
	dw9800_sccb_write(0x03,0x01);
	dw9800_sccb_write(0x04,0x50);


	dw9800_act.cur_pos = 20;
	dw9800_act.dir = 0;
	dw9800_act.pos = 0;
	dw9800_act.step = 4;
	dw9800_act.max = DW9800_MAX_CURRENT;
	dw9800_act.min = DW9800_MIN_CURRENT;
	dw9800_act.total_step = TOTAL_STEPS_ACTIVE;

	dw9800_act.table[0] = dw9800_act.min;
	for(i = 1 ; i < dw9800_act.total_step - 1; i++)
	{
		dw9800_act.table[i] = dw9800_act.min + i * DW9800_STEP;
		DBG_PRINT("dw9800_act.table[%d] = %d\r\n", i,dw9800_act.table[i]);
	}
	dw9800_act.table[dw9800_act.total_step - 1] = dw9800_act.max;


	data = dw9800_act.min;
	test_inc = 1;
	test = 0;
	/*while(1)
	{
		int ret;

		test += test_inc;
		if(test < 0)
			break;
		else if(test >= dw9800_act.total_step)
		{
			test_inc = -1;
			test = dw9800_act.total_step - 1;
		}

		data = dw9800_act.table[test];
		//DBG_PRINT("data = %d\r\n", data);

		//DBG_PRINT("data = %d\r\n", data);
		ret = dw9800_advanced_sccb_write(0x03, data >> 8);
		if(ret != 0)
			DBG_PRINT("sccb err\r\n");
		ret = dw9800_advanced_sccb_write(0x04, data & 0xff);
		if(ret != 0)
			DBG_PRINT("sccb err\r\n");

		osDelay(10);
	}*/

	dw9800_init_pos();
	return STATUS_OK;
#else
	//For GC5035 Read ID Test.
	drv_l2_sensor_set_mclkout(MCLK_24M);
	osDelay(20);
	if(dw9800_advanced_sccb_read(0xf0, &value)>=0)
	{
		sensor_id = value;
		sensor_id <<= 8;
	}
	else
	{
		return STATUS_FAIL;
	}
	if(dw9800_advanced_sccb_read(0xf1, &value)>=0)
	{
		sensor_id += value;
	}
	else
	{
		return STATUS_FAIL;
	}
	dw9800_sccb_close();
	DBG_PRINT("sensor_id = 0x%x\r\n",sensor_id);

	if(0x5035 != sensor_id)
	return STATUS_FAIL;

	return STATUS_OK;
#endif
}


static INT32S dw9800_set_val(unsigned short val)
{
	int ret;
	unsigned short data = dw9800_act.table[val];

	//DBG_PRINT("data = %d\r\n", data);
	ret = dw9800_advanced_sccb_write(0x03, data >> 8);
	if(ret != 0)
		DBG_PRINT("sccb err\r\n");
	ret = dw9800_advanced_sccb_write(0x04, data & 0xff);
	if(ret != 0)
		DBG_PRINT("sccb err\r\n");

	return ret;
}


void dw9800_manul_mov_pos(unsigned short data){
	int ret;
	DBG_PRINT("data = %d\r\n", data);
	ret = dw9800_advanced_sccb_write(0x03, data >> 8);
	if(ret != 0)
		DBG_PRINT("sccb err\r\n");
	ret = dw9800_advanced_sccb_write(0x04, data & 0xff);
	if(ret != 0)
		DBG_PRINT("sccb err\r\n");

 }


 void dw9800_init_pos(void){
	int ret;
	ret = dw9800_advanced_sccb_write(0x03, 0x01);
	if(ret != 0)
		DBG_PRINT("sccb err\r\n");
	ret = dw9800_advanced_sccb_write(0x04, 0x50);
	if(ret != 0)
		DBG_PRINT("sccb err\r\n");

 }
extern INT8U tag_end;

void dw9800_mov_pos(INT32S dest_pos)
{
	INT32S target_pos;
#if 1
	//DBG_PRINT("dw9800_mov_pos %d %d\r\n",dw9800_act.cur_pos,dest_pos);
	target_pos = dw9800_act.cur_pos;

	while (target_pos != dest_pos)
	{
		int ret;
		int step;


		step = (dest_pos - target_pos) ;//>> 2;

		if(step == 0)
		{
			if(target_pos < dest_pos)
			step = 1;
			else
			step = -1;
		}

		target_pos+=step;

		ret = dw9800_set_val(target_pos);
		if (ret == 0)
		{
			dw9800_act.cur_pos = target_pos;
			osDelay(1);
		}
		else
			break;
	}


	#endif

}


INT32S dw9800_uninit(void)
{
	//DW9800_XSD_PIN:IO_D10//ShutDown mode(active low)
	gpio_init_io(DW9800_XSD_PIN, GPIO_OUTPUT);
	gpio_set_port_attribute(DW9800_XSD_PIN, ATTRIBUTE_HIGH);
	gpio_write_io(DW9800_XSD_PIN, DATA_LOW);
	//osDelay(50);
}

const drv_l2_actuator_ops_t dw9800_act_ops =
{
	DW9800_ACT_NAME,
	dw9800_init,
	dw9800_uninit,
	dw9800_mov_pos
};

#endif

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

#if (defined _AD5823) && (_AD5823 == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
//For AD5823
#define	AD5823_ID               (0x18)

#define VCM_MIN_CURRENT         10  //30
#define VCM_MAX_CURRENT         110 //100

#define AD5823_MIN_STEP        (float)(120.0 / 1023.0)
#define AD5823_MIN_CURRENT     (VCM_MIN_CURRENT / AD5823_MIN_STEP)
#define AD5823_MAX_CURRENT     (VCM_MAX_CURRENT / AD5823_MIN_STEP)

#define TOTAL_STEPS_ACTIVE      64
#define AD5823_STEP            ((AD5823_MAX_CURRENT - AD5823_MIN_CURRENT) / TOTAL_STEPS_ACTIVE)

#define AD5823_INIT_FOCUS_POS   350

#define AD5823_MAX_FOCUS_POS	1023

#define AD5823_RESONANCE_PERIOD		100000
#define AD5823_RESONANCE_COEF		512
#define AD5823_HIGH_FREQ_RANGE		0x80

#define AD5823_I2C_MODE	            			ACTUATOR_I2C_MODE_SW

#if AD5823_I2C_MODE == ACTUATOR_I2C_MODE_SW
#define	ACTUATOR_I2C_SCL							IO_A12
#define	ACTUATOR_I2C_SDA							IO_A13
#define	ACTUATOR_I2C_DRIVING						IOA_DRV_30mA
#else
#define	AD5823_I2C_DEVICE						I2C_0
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

enum ad5823_vcm_mode {
	AD5823_ARC_RES0 = 0x0,	/* Actuator response control RES1 */
	AD5823_ARC_RES1 = 0x1,	/* Actuator response control RES0.5 */
	AD5823_ARC_RES2 = 0x2,	/* Actuator response control RES2 */
	AD5823_ESRC     = 0x3,	/* Enhanced slew rate control */
	AD5823_DIRECT   = 0x4,	/* Direct control */
};

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
actuator_s  ad5823_act;
static void *ad5823_handle;

static INT32S ad5823_sccb_open(void)
{
#if AD5823_I2C_MODE == ACTUATOR_I2C_MODE_SW
	sccb_config_t cfg;
	cfg.scl_port = IO_B4;
	cfg.scl_drv = IO_DRIVING_lvl0;
	cfg.sda_port = IO_B5;
	cfg.sda_drv = IO_DRIVING_lvl0;
	cfg.pwdn_port = 0;
	cfg.pwdn_drv = 0;
	cfg.have_pwdn = 0;
	cfg.RegBits = 8;
	cfg.DataBits = 8;
	cfg.slaveAddr = AD5823_ID;
	cfg.timeout = 0x20000;
	cfg.clock_rate = 100;

	ad5823_handle = drv_l2_sccb_open_ext(&cfg);

	if(ad5823_handle == 0)
	{
		DBG_PRINT("ad5823 Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#endif
	return STATUS_OK;
}

static void ad5823_sccb_close(void)
{
#if AD5823_I2C_MODE == ACTUATOR_I2C_MODE_SW
	if(ad5823_handle)
	{
		drv_l2_sccb_close(ad5823_handle);
		ad5823_handle = NULL;
	}
#endif
}

static INT32S ad5823_sccb_write(INT8U reg, INT16U value)
{
#if AD5823_I2C_MODE == ACTUATOR_I2C_MODE_SW
	return drv_l2_sccb_write(ad5823_handle, reg, value);
#endif
}

INT32S ad5823_init(void)
{
	INT32S ret = 0;
	int i, test = 0, test_inc = 100, data = 0;
	INT16U value;

	DBG_PRINT("***************************************\r\n");
	DBG_PRINT("%s.\r\n", __FUNCTION__);
	DBG_PRINT("***************************************\r\n");

	// reguest sccb
	ad5823_sccb_open();
	osDelay(10);

	//VCM init
	ret = ad5823_sccb_write(0x01, 0x01);
	if(ret != 0)
		DBG_PRINT("sccb err, addr 0x01 val 0x01\r\n");

	//change the mode
	ret = ad5823_sccb_write(0x04, 0x04); //ring control enable
	if(ret != 0)
		DBG_PRINT("sccb err, addr 0x04 val 0x04\r\n");
	ret = ad5823_sccb_write(0x02, AD5823_ARC_RES1); //set ARC RES0.5 mode
	if(ret != 0)
		DBG_PRINT("sccb err, addr 0x02 val 0x%x\r\n", AD5823_ARC_RES1);

	/* set reg VCM_CODE_MSB Bit[1:0] */
	value = (AD5823_INIT_FOCUS_POS >> 8);
	ret = ad5823_sccb_write(0x04, value);
	if(ret != 0) DBG_PRINT("sccb err, addr 0x04 val 0x%x\r\n", value);

	/* set reg VCM_CODE_LSB Bit[7:0] */
	value = (AD5823_INIT_FOCUS_POS & 0xff);
	ret = ad5823_sccb_write(0x05, value);
	if(ret != 0)
		DBG_PRINT("sccb err, addr 0x05 val 0x%x\r\n", value);

	/* set required vcm move time */
	value = AD5823_RESONANCE_PERIOD / AD5823_RESONANCE_COEF - AD5823_HIGH_FREQ_RANGE;
	ret = ad5823_sccb_write(0x03, value);
	if(ret != 0)
		DBG_PRINT("sccb err, addr 0x03 val 0x%x\r\n", value);

	ad5823_act.cur_pos = 0;
	ad5823_act.dir = 0;
	ad5823_act.pos = 0;
	ad5823_act.step = 0;
	ad5823_act.max = AD5823_MAX_CURRENT;
	ad5823_act.min = AD5823_MIN_CURRENT;
	ad5823_act.total_step = TOTAL_STEPS_ACTIVE;

	ad5823_act.table[0] = ad5823_act.min;
	for(i = 1 ; i < ad5823_act.total_step - 1; i++)
	{
		ad5823_act.table[i] = ad5823_act.min + i * AD5823_STEP;
	}
	ad5823_act.table[ad5823_act.total_step - 1] = ad5823_act.max;

	data = ad5823_act.min;
	test_inc = 1;
	test = 0;
	while(1)
	{
		test += test_inc;
		if(test < 0)
			break;
		else if(test >= ad5823_act.total_step)
		{
			test_inc = -1;
			test = ad5823_act.total_step - 1;
		}

		data = ad5823_act.table[test];
		//DBG_PRINT("data = %d\r\n", data);

		ret = ad5823_sccb_write(0x04, (data >> 8));
		if(ret != 0)
			DBG_PRINT("sccb err, addr 0x04 val 0x%x\r\n", (data >> 8));
		ret = ad5823_sccb_write(0x05, (data & 0xff));
		if(ret != 0)
			DBG_PRINT("sccb err, addr 0x05 val 0x%x\r\n", (data & 0xff));

		osDelay(10);
	}

	return STATUS_OK;
}


static INT32S ad5823_set_val(unsigned short val)
{
	int ret;
	unsigned short data = ad5823_act.table[val];

	//DBG_PRINT("set data = %d\r\n", data);
	ret = ad5823_sccb_write(0x04, (data >> 8));
	if(ret != 0)
		DBG_PRINT("sccb err, addr 0x04 val 0x%x\r\n", (data >> 8));
	ret = ad5823_sccb_write(0x05, (data & 0xff));
	if(ret != 0)
		DBG_PRINT("sccb err, addr 0x05 val 0x%x\r\n", (data & 0xff));

	return ret;
}

void ad5823_mov_pos(INT32S dest_pos)
{
	INT32S target_pos;

	target_pos = ad5823_act.cur_pos;
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

		ret = ad5823_set_val(target_pos);
		if (ret == 0)
		{
			ad5823_act.cur_pos = target_pos;
			osDelay(1);
		}
		else
			break;
	}
}

void ad5823_uninit(void)
{
	ad5823_sccb_close();
	//osDelay(50);
}

const drv_l2_actuator_ops_t ad5823_act_ops =
{
	AD5823_ACT_NAME,
	ad5823_init,
	ad5823_uninit,
	ad5823_mov_pos
};
#endif

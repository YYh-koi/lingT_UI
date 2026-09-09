#include "drv_l2.h"

#define DA380_SLAVE_ID    		0x4E
#define SC7A30E_SLAVE_ID  		0x32//0x3A

static INT8U _gsensor_dev;
static INT8U _gsensor_active;

static INT8U g_sensor_id;
static INT8U g_sensor_st;

static INT32U _i2c_gpio_scl;
static INT32U _i2c_gpio_sda;

static void I2C_delay (INT16U i)
{
#if 1
	INT32U cnt = i * 200;

	drv_tiny_wait(cnt);
	//drv_tiny_wait(i * 23);

#elif 0
	INT16U j;

	for (j=0;j<(i<<4);j++)
		i=i;
#endif
}

static void I2C_start (void)
{
	gpio_write_io(_i2c_gpio_scl, DATA_HIGH);					//SCL1
	I2C_delay(1);
	gpio_write_io(_i2c_gpio_sda, DATA_HIGH);					//SDA1
	I2C_delay(1);
	gpio_write_io(_i2c_gpio_sda, DATA_LOW);						//SDA0
	I2C_delay(1);
}

static void I2C_stop (void)
{
	I2C_delay(1);
	gpio_write_io(_i2c_gpio_sda, DATA_LOW);						//SDA0
	I2C_delay(1);
	gpio_write_io(_i2c_gpio_scl, DATA_HIGH);					//SCL1
	I2C_delay(1);
	gpio_write_io(_i2c_gpio_sda, DATA_HIGH);					//SDA1
	I2C_delay(1);
}

static void I2C_w_phase (INT16U value)
{
	INT16U i;

	for (i=0;i<8;i++)
	{
		gpio_write_io(_i2c_gpio_scl, DATA_LOW);					//SCL0
		I2C_delay (1);
		if (value & 0x80)
		{
			gpio_write_io(_i2c_gpio_sda, DATA_HIGH);			//SDA1
		}
		else
		{
			gpio_write_io(_i2c_gpio_sda, DATA_LOW);				//SDA0
		}
		//sccb_delay (1);
		gpio_write_io(_i2c_gpio_scl, DATA_HIGH);				//SCL1
		I2C_delay(1);
		value <<= 1;
	}

	// The 9th bit transmission
	gpio_write_io(_i2c_gpio_scl, DATA_LOW);						//SCL0
	gpio_init_io(_i2c_gpio_sda, GPIO_INPUT);					//SDA is Hi-Z mode
	I2C_delay(1);
	gpio_write_io(_i2c_gpio_scl, DATA_HIGH);					//SCL1
	I2C_delay(1);
	gpio_write_io(_i2c_gpio_scl, DATA_LOW);						//SCL0
	gpio_init_io(_i2c_gpio_sda, GPIO_OUTPUT);					//SDA is Hi-Z mode
}

INT16U I2C_r_phase(void)
{
	INT16U i;
	INT16U data;

	gpio_init_io(_i2c_gpio_sda, GPIO_INPUT);					//SDA is Hi-Z mode
	data = 0x00;
	for (i=0;i<8;i++)
	{
		gpio_write_io(_i2c_gpio_scl, DATA_LOW);					//SCL0
		I2C_delay(1);
		gpio_write_io(_i2c_gpio_scl, DATA_HIGH);				//SCL1
		data <<= 1;
		data |=( gpio_read_io(_i2c_gpio_sda));
		I2C_delay(1);
	}

	// The 9th bit transmission
	gpio_write_io(_i2c_gpio_scl, DATA_LOW);						//SCL0
	gpio_init_io(_i2c_gpio_sda, GPIO_OUTPUT);					//SDA is output mode
	gpio_write_io(_i2c_gpio_sda, DATA_HIGH);					//SDA0, the nighth bit is NA must be 1
	I2C_delay(1);
	gpio_write_io(_i2c_gpio_scl, DATA_HIGH);					//SCL1
	I2C_delay(1);
	gpio_write_io(_i2c_gpio_scl, DATA_LOW);						//SCL0

	return data;
}

static void I2C_gpio_init(void)
{
	//init IO
	gpio_set_port_attribute(_i2c_gpio_scl, ATTRIBUTE_HIGH);
	gpio_set_port_attribute(_i2c_gpio_sda, ATTRIBUTE_HIGH);
	gpio_init_io(_i2c_gpio_scl, GPIO_OUTPUT);				//set dir
	gpio_init_io(_i2c_gpio_sda, GPIO_OUTPUT);				//set dir
	gpio_write_io(_i2c_gpio_scl, DATA_HIGH);				//SCL1
	gpio_write_io(_i2c_gpio_sda, DATA_HIGH);				//SDA0
}

static void gsensor_write(INT8U id, INT8U addr, INT8U data)
{
	I2C_gpio_init();
	// 3-Phase write transmission cycle is starting now ...
	I2C_start();						// Transmission start
	I2C_w_phase(id);					// Phase 1: Device ID
	I2C_w_phase(addr);					// Phase 2: Register address. High pass filter enable
	I2C_w_phase(data);					// Phase 3: Data value
	I2C_stop();							// Transmission stop
}

static INT16U gsensor_read(INT8U id, INT8U addr)
{
	INT16U redata;

	I2C_gpio_init();
	I2C_start();						// Transmission start
	I2C_w_phase(id);					// Phase 1: Device ID
	I2C_w_phase(addr);					// Phase 2: Register address. Transient source
	I2C_start();						// Transmission start
	I2C_w_phase(id | 0x01);				// Phase 1 (read)
	redata = (INT8U) I2C_r_phase();		// Phase 2
	I2C_stop();							// Transmission stop

	return redata;
}

static void G_Sensor_DA380_Init(INT8U level)
{

	INT16U temp1,temp2;
	DBG_PRINT(">>> %s ... \r\n", __func__);

	temp1 = gsensor_read(g_sensor_id, CHIPID);
	DBG_PRINT("chip_id = %x\r\n", temp1);

	temp1 = gsensor_read(g_sensor_id, MOTION_FLAG);
	temp2 = gsensor_read(g_sensor_id, INT_MAP1);
	if((temp1 != 0xff) && (temp1 & 0x04) && (temp2 !=0))	//active int flag
	{
		g_sensor_st = 1;
		DBG_PRINT("==========gsensor power on=========\r\n");
	}
	else
	{
		g_sensor_st = 0;
		DBG_PRINT("==========key power on=============\r\n");
	}

	gsensor_write(g_sensor_id, SOFT_RESET, 0x24);
	drv_msec_wait(4);
	gsensor_write(g_sensor_id, RESOLUTION_RANGE, 0x02);
	gsensor_write(g_sensor_id, MODE_BW, 0x1e);
	gsensor_write(g_sensor_id, ODR_AXIS, 0x07);

	if(_gsensor_active)
	{
		gsensor_write(g_sensor_id, INT_CONFIG, 0x05);	//selects active level high for pin INT
		DBG_PRINT("high active\r\n");
	}
	else
	{
		gsensor_write(g_sensor_id, INT_CONFIG, 0x04);	//selects active level low for pin INT
		DBG_PRINT("low active\r\n");
	}

	gsensor_write(g_sensor_id, INT_LATCH, 0x8f);

	gsensor_write(g_sensor_id, INT_SET1, 0x47);		//avtive x,y,z interrupt
	gsensor_write(g_sensor_id, ACTIVE_DUR, 0x03);
	gsensor_write(g_sensor_id, INT_MAP1, 0x00);		//doesn't map to INT
	gsensor_write(g_sensor_id, INT_LATCH, 0x87);	//latch forever

	switch(level)
	{
		case 1:	// low
			gsensor_write(g_sensor_id, ACTIVE_THS, DMT_SENSITIVE_LOW);
		break;
		case 2:	// mid
			gsensor_write(g_sensor_id, ACTIVE_THS, DMT_SENSITIVE_MID);
		break;
		case 3:	// high
			gsensor_write(g_sensor_id, ACTIVE_THS, DMT_SENSITIVE_HIGH);
		break;
	}
}

static void DA380_Enter_Interrupt_WakeUp_Mode(INT8U level)
{
	gsensor_write(g_sensor_id, SOFT_RESET, 0x24);
	drv_msec_wait(4);
	gsensor_write(g_sensor_id, RESOLUTION_RANGE, 0x02);
	gsensor_write(g_sensor_id, ODR_AXIS, 0x07);

	if(_gsensor_active)
	{
		gsensor_write(g_sensor_id, INT_CONFIG, 0x05);	//selects active level high for pin INT
	}
	else
	{
		gsensor_write(g_sensor_id, INT_CONFIG, 0x04);	//selects active level low for pin INT
	}

	gsensor_write(g_sensor_id, INT_LATCH, 0x8f);

	gsensor_write(g_sensor_id, INT_SET1, 0x07);
	gsensor_write(g_sensor_id, ACTIVE_DUR, 0x03);

	switch(level)
	{
		case 1://hig
			gsensor_write(g_sensor_id, ACTIVE_THS, P_SENSITIVE_HIGH);
		break;
		case 2://mid
			gsensor_write(g_sensor_id, ACTIVE_THS, P_SENSITIVE_MID);
		break;
		case 3://low
			gsensor_write(g_sensor_id, ACTIVE_THS, P_SENSITIVE_LOW);
		break;
		default:
		break;
	}

	if(level)
	{
		// map to INT pin
		gsensor_write(g_sensor_id, INT_MAP1, 0x04);
	}
	else
	{
		// doesn't map to INT
		gsensor_write(g_sensor_id, INT_MAP1, 0x00);
	}

	//gsensor_write(g_sensor_id, MODE_BW, 0x1e);		 //normal mode
	gsensor_write(g_sensor_id, MODE_BW, 0x5e);		//low power mode
	gsensor_write(g_sensor_id, INT_LATCH, 0x87);	//clear gsensor interrupt flag
}

static void DA380_gps_data_get(INT8U *pDATA)
{
	*pDATA++ = gsensor_read(g_sensor_id,  ACC_X_LSB);
	*pDATA++ = gsensor_read(g_sensor_id,  ACC_X_MSB);
	*pDATA++ = gsensor_read(g_sensor_id,  ACC_Y_LSB);
	*pDATA++ = gsensor_read(g_sensor_id,  ACC_Y_MSB);
	*pDATA++ = gsensor_read(g_sensor_id,  ACC_Z_LSB);
	*pDATA++ = gsensor_read(g_sensor_id,  ACC_Z_MSB);
	*pDATA++ = gsensor_read(g_sensor_id,  ORIENT_STATUS);
	*pDATA++ = gsensor_read(g_sensor_id,  RESOLUTION_RANGE);
}

static void DA380_gps_data_set(void *pDST, void *pSRC)
{
	INT16U *s = (INT16U *)pSRC;
	GSENSOR_DATA *d = (GSENSOR_DATA *)pDST;

	d->Axis.Xacc = (INT32U )(*s++)>>2;
	d->Axis.Yacc = (INT32U)(*s++)>>2;
	d->Axis.Zacc = (INT32U)(*s++)>>2;
	d->Ori = (INT32U)( ((*s)>>4)&(0x3) );
	d->SH = (INT32U)( (*s)>>6 );
}

static void G_Sensor_SC7A30E_Init(INT8U level)
{
	INT16U temp1,temp2;
	INT16U ttmp = 0;

	DBG_PRINT(">>> %s ... \r\n", __func__);

	temp1 = gsensor_read(g_sensor_id, SC7A30E_CHIPID);

	if (temp1 != 0x11)
	{
		DBG_PRINT("[WARING]Get SC7A30E ID:0x%x Error!!\r\n",temp1);
	}
	else
	{
		DBG_PRINT("sc7a30e_chip_id = 0x%x\r\n", temp1);
	}

	temp2 = gsensor_read(g_sensor_id, SC7A30E_CLICK_SRC);	//Read Isr is 0x40
	if((temp2 != 0xff) && (temp2 & 0x40)) //active int flag
	{
		g_sensor_st = 1;
		DBG_PRINT("==========gsensor power on=========\r\n");
	} else {
		g_sensor_st = 0;
		DBG_PRINT("==========key power on=============\r\n");
	}

	gsensor_write(g_sensor_id, SC7A30E_CTRL_REG_3, 0x00);		//disable isr
	gsensor_write(g_sensor_id, SC7A30E_CLICK_CFG, 0x00);		//disable isr
	gsensor_write(g_sensor_id, SC7A30E_CTRL_REG_5, 0x80);		//BOOT
	drv_msec_wait(4);

	gsensor_write(g_sensor_id, SC7A30E_CTRL_REG_1, 0x37);		//ODR:25Hz
	gsensor_read(g_sensor_id, SC7A30E_REFERANCE);
	gsensor_write(g_sensor_id, SC7A30E_CTRL_REG_2, 0x0d);		//Enable High Pass Filter
	gsensor_write(g_sensor_id, SC7A30E_CTRL_REG_4, 0x80);		//BDU,+-2G

	switch(level)
	{
		//range:0x01~0x7e
		case 0:
			gsensor_write(g_sensor_id, SC7A30E_CLICK_THS, SC7A30E_SENSITIVE_OFF);
			DBG_PRINT("[MSG] G SENSOR Off\r\n");
		break;
		case 1://hig
			gsensor_write(g_sensor_id, SC7A30E_CLICK_THS, SC7A30E_SENSITIVE_HIGH);
		break;
		case 2://mid
			gsensor_write(g_sensor_id, SC7A30E_CLICK_THS, SC7A30E_SENSITIVE_MID);
		break;
		case 3://low
			gsensor_write(g_sensor_id, SC7A30E_CLICK_THS, SC7A30E_SENSITIVE_LOW);
		break;
		default:
		break;
	}

	ttmp = gsensor_read(g_sensor_id, SC7A30E_CTRL_REG_6);
	DBG_PRINT("==>>SC7A30E_CTRL_REG_6:0x%x\r\n",ttmp);

	if(_gsensor_active)
	{
		gsensor_write(g_sensor_id, SC7A30E_CTRL_REG_6, 0x00);	//selects active level high for pin INT
		DBG_PRINT("==>>SC7A30E_High_Active\r\n");
	}
	else
	{
		gsensor_write(g_sensor_id, SC7A30E_CTRL_REG_6, 0x02);	//selects active level low for pin INT
		DBG_PRINT("==>>SC7A30E_Low_Active\r\n");
	}

#if 1
	ttmp = gsensor_read(g_sensor_id, SC7A30E_CLICK_THS);
	DBG_PRINT("==>>SC7A30E_CLICK_THS:0x%x\r\n",ttmp);
	ttmp = gsensor_read(g_sensor_id, SC7A30E_CTRL_REG_3);
	DBG_PRINT("==>>SC7A30E_CTRL_REG_3:0x%x\r\n",ttmp);
	ttmp = gsensor_read(g_sensor_id, SC7A30E_CLICK_CFG);
	DBG_PRINT("==>>SC7A30E_CLICK_CFG:0x%x\r\n",ttmp);
	ttmp = gsensor_read(g_sensor_id, SC7A30E_CTRL_REG_1);
	DBG_PRINT("==>>SC7A30E_CTRL_REG_1:0x%x\r\n",ttmp);
	ttmp = gsensor_read(g_sensor_id, SC7A30E_INT1_SOURCE);
	DBG_PRINT("==>>SC7A30E_INT1_SOURCE:0x%x\r\n",ttmp);
	ttmp = gsensor_read(g_sensor_id, SC7A30E_INT1_CFG);
	DBG_PRINT("==>>SC7A30E_INT1_CFG:0x%x\r\n",ttmp);
	ttmp = gsensor_read(g_sensor_id, SC7A30E_CTRL_REG_6);
	DBG_PRINT("==>>SC7A30E_CTRL_REG_6:0x%x\r\n",ttmp);

	DBG_PRINT("\r\n");
#endif

}

static void SC7A30E_Enter_Interrupt_WakeUp_Mode(INT8U level)
{
	DBG_PRINT(">>> %s ... \r\n", __func__);

	gsensor_write(g_sensor_id, SC7A30E_CTRL_REG_6, 0x00);	//中斷高電平有效

	switch(level)
	{
		//range:0x01~0x7e
		case 0:
			gsensor_write(g_sensor_id, SC7A30E_CLICK_THS, SC7A30E_SENSITIVE_OFF);
		case 1://hig
			gsensor_write(g_sensor_id, SC7A30E_CLICK_THS, SC7A30E_SENSITIVE_HIGH);
		break;
		case 2://mid
			gsensor_write(g_sensor_id, SC7A30E_CLICK_THS, SC7A30E_SENSITIVE_MID);
		break;
		case 3://low
			gsensor_write(g_sensor_id, SC7A30E_CLICK_THS, SC7A30E_SENSITIVE_LOW);
		break;
		default:
		break;
	}

	if(_gsensor_active)
	{
		gsensor_write(g_sensor_id, SC7A30E_CTRL_REG_6, 0x00);	//selects active level high for pin INT
	}
	else
	{
		gsensor_write(g_sensor_id, SC7A30E_CTRL_REG_6, 0x02);	//selects active level low for pin INT
	}

	if(level)
	{
		gsensor_write(g_sensor_id,SC7A30E_TIME_LIMIT, 0x7f);
		gsensor_write(g_sensor_id,SC7A30E_TIME_LATENCY, 0x6c);	//ISR Status hold 40ms/LSB
		gsensor_write(g_sensor_id,SC7A30E_CLICK_CFG, 0x15);		//XYZ
		gsensor_write(g_sensor_id,SC7A30E_CTRL_REG_3, 0x80);	//Enable ISR
	}
	/*
	else
	{
		gsensor_write(g_sensor_id,SC7A30E_CLICK_THS,SC7A30E_SENSITIVE_OFF);
		DBG_PRINT("[MSG] G SENSOR SENSITIVE Off\r\n");
		//gsensor_write(g_sensor_id,SC7A30E_CTRL_REG_1,0x00);	//disable g sensor
	}
	*/
}

static void G_Sensor_SC7A30E_power_off(void)
{
	DBG_PRINT(">>> %s ... \r\n", __func__);

	gsensor_write(g_sensor_id,SC7A30E_CLICK_THS,SC7A30E_SENSITIVE_OFF);
	gsensor_write(g_sensor_id,SC7A30E_CTRL_REG_1,0x00);	//disable g sensor
}

void drv_l2_gsensor_sensitive_set(INT8U gsensor)
{
	switch(_gsensor_dev)
	{
		case G_SENSOR_NONE:
		break;
		case G_SENSOR_DA380:
		{
			gsensor_write(g_sensor_id,RESOLUTION_RANGE,0x00);

			switch(gsensor)
			{
				// doesn't map to INT
				case 0:
					gsensor_write(g_sensor_id,INT_MAP1,0x00);
				break;
				// 2g
				case 1:
					gsensor_write(g_sensor_id,ACTIVE_THS,DMT_SENSITIVE_HIGH);
				break;
				// 4g
				case 2:
					gsensor_write(g_sensor_id,ACTIVE_THS,DMT_SENSITIVE_MID);
				break;
				 // 8g
				case 3:
					gsensor_write(g_sensor_id,ACTIVE_THS,DMT_SENSITIVE_LOW);
				break;
				default:
				break;
			}

			gsensor_write(g_sensor_id,INT_LATCH, 0x87);	//clear gsensor interrupt flag
		}
		break;

		case G_SENSOR_SC7A30E:
		{
			switch(gsensor)
			{
				//range:0x01~0x7e
				case 0:
					gsensor_write(g_sensor_id,SC7A30E_CLICK_THS,SC7A30E_SENSITIVE_OFF);
					DBG_PRINT("[MSG] G SENSOR Off\r\n");
				break;
				// high
				case 1:
					gsensor_write(g_sensor_id,SC7A30E_CLICK_THS,SC7A30E_SENSITIVE_HIGH);
					DBG_PRINT("[MSG] G SENSOR HIGH\r\n");
				break;
				//mid
				case 2:
					gsensor_write(g_sensor_id,SC7A30E_CLICK_THS,SC7A30E_SENSITIVE_MID);
					DBG_PRINT("[MSG] G SENSOR MID\r\n");
				break;
				//low
				case 3:
					gsensor_write(g_sensor_id,SC7A30E_CLICK_THS,SC7A30E_SENSITIVE_LOW);
					DBG_PRINT("[MSG] G SENSOR LOW\r\n");
				break;
				default:
				break;
			}

			if(gsensor)
			{
				gsensor_write(g_sensor_id,SC7A30E_TIME_LIMIT,0x7f);
				gsensor_write(g_sensor_id,SC7A30E_TIME_LATENCY, 0x6c);	//ISR Status hold 40ms/LSB
				gsensor_write(g_sensor_id,SC7A30E_CLICK_CFG, 0x15);		//XYZ
				gsensor_write(g_sensor_id,SC7A30E_CTRL_REG_3, 0x80);	//Enable ISR
			}
			else
			{
				gsensor_write(g_sensor_id,SC7A30E_CTRL_REG_3, 0x00);	//Disable ISR
				gsensor_write(g_sensor_id,SC7A30E_CLICK_CFG, 0x00);		//disable isr
				DBG_PRINT("G_sensor_Disable ISR = 0x%02x\r\n", gsensor);
			}

		}
		break;
		default:
			DBG_PRINT("The function isn't implement yet\r\n");
		break;
	}
}

INT16U drv_l2_gsensor_int_active_get(void)
{
	INT16U temp = 0xff;

	switch(_gsensor_dev)
	{
		case G_SENSOR_NONE:
		break;
		case G_SENSOR_DA380:
			temp = gsensor_read(g_sensor_id, MOTION_FLAG);
		break;
		case G_SENSOR_SC7A30E:
			temp = gsensor_read(g_sensor_id, SC7A30E_CLICK_SRC);	//Read Isr is 0x40
			if(temp != 0xff)
			{
				temp = temp >> 4;
			}
		break;
		default:
			DBG_PRINT("The function isn't implement yet\r\n");
		break;
	}
	//DBG_PRINT("drv_l2_gsensor_int_active_get = 0x%02x\r\n", temp);
	return temp;
}

void drv_l2_gsensor_int_flag_clear(void)
{
	switch(_gsensor_dev)
	{
		case G_SENSOR_NONE:
		break;
		case G_SENSOR_DA380:
			gsensor_write(g_sensor_id,INT_LATCH, 0x87); //clear gsensor interrupt flag
		break;
		case G_SENSOR_SC7A30E:
			gsensor_read(g_sensor_id,SC7A30E_INT1_SOURCE);
		break;
		default:
		break;
	}
}

INT8U drv_l2_gsensor_power_on_status_get(void)
{
	return g_sensor_st;
}

void drv_l2_gsensor_park_mode_init(INT8U level)
{
	DBG_PRINT(">>> %s ... ,Level:%d\r\n", __func__,level);

	switch(_gsensor_dev)
	{
		case G_SENSOR_NONE:
		break;
		case G_SENSOR_DA380:
			DA380_Enter_Interrupt_WakeUp_Mode(level);
		break;
		case G_SENSOR_SC7A30E:
			SC7A30E_Enter_Interrupt_WakeUp_Mode(level);
		break;
		default:
			DBG_PRINT("The function isn't implement yet\r\n");
		break;
	}
}

void drv_l2_gsensor_gps_data_get(INT8U *pDATA)
{
	switch(_gsensor_dev)
	{
		case G_SENSOR_NONE:
		break;
		case G_SENSOR_DA380:
			DA380_gps_data_get(pDATA);
		break;
		default:
			DBG_PRINT("The function isn't implement yet\r\n");
		break;
	}
}

void drv_l2_gsensor_gps_data_set(void *pDST, void *pSRC)
{
	switch(_gsensor_dev)
	{
		case G_SENSOR_NONE:
		break;
		case G_SENSOR_DA380:
			DA380_gps_data_set(pDST, pSRC);
		break;
		default:
			DBG_PRINT("The function isn't implement yet\r\n");
		break;
	}
}

void drv_l2_gsensor_pw_off(void)
{
	DBG_PRINT(">>> %s ... \r\n", __func__);
	g_sensor_st = 0;

	switch(_gsensor_dev)
	{
		case G_SENSOR_NONE:
		break;
		case G_SENSOR_SC7A30E:
			G_Sensor_SC7A30E_power_off();
		break;
		default:
			DBG_PRINT("The function isn't implement yet\r\n");
		break;
	}
}

void drv_l2_gsensor_init(INT8U dev, INT8U act, GPIO_ENUM gpio_scl, GPIO_ENUM gpio_sda, INT8U level)
{
	DBG_PRINT(">>> %s Level:%d... \r\n", __func__, level);

	_gsensor_dev	= dev;
	_gsensor_active	= act;
	_i2c_gpio_scl	= gpio_scl;
	_i2c_gpio_sda	= gpio_sda;

	g_sensor_st = 0;

	switch(_gsensor_dev)
	{
		case G_SENSOR_NONE:
		break;
		case G_SENSOR_DA380:
			g_sensor_id = DA380_SLAVE_ID;
			G_Sensor_DA380_Init(level);
		break;
		case G_SENSOR_SC7A30E:
			g_sensor_id = SC7A30E_SLAVE_ID;
			G_Sensor_SC7A30E_Init(level);
		break;
	}
}



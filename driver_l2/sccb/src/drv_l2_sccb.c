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

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define SCCB_MAX_SEM_ID         16

#define SCCB_SCL     	        IO_B4
#define SCCB_SDA		        IO_B5
#define CSI_PWDN		        IO_D8

#define SCCB_SCL_DRV	        IOB_DRV_30mA
#define SCCB_SDA_DRV	        IOB_DRV_30mA
#define SCCB_PWDN_DRV           IOD_DRV_10mA

#define SCCB_TIMEOUT	0x20000 // in ms unit
#define SCCB_CLOCK_RATE 100 // 100K, in kHz

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct sccb_tran_s
{
	INT8U id; // slave address
	INT8U addr_bits; // 8 multiple
	INT16U data_bits; // 8 multiple
	INT8U ack;
	INT32U addr;
	INT32U data;

    osSemaphoreId sccb_sem_id;

    // williamyeo added
    GPIO_ENUM       scl_port;
    IO_DRV_LEVEL    scl_drv;
    GPIO_ENUM       sda_port;
    IO_DRV_LEVEL    sda_drv;
    GPIO_ENUM       pwdn_port;
    IO_DRV_LEVEL    pwdn_drv;
    INT8U           have_pwdn;
    INT32U          timeout;
    INT32U          clock_rate; // no function yet

    INT32U *paddr;
    INT32U *pdat;
} sccb_tran_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static sccb_tran_t sccb_sem_id_max[SCCB_MAX_SEM_ID];

static void sccb_delay(INT32U i, INT32 clock)
{
	volatile INT32U j;
	//INT32U cnt = i * (SystemCoreClock >> 22);
	INT32U cnt = i * (SystemCoreClock/(clock*8000));
	if (cnt==0)
        cnt = 1;

	for(j=0; j<cnt; j++) {
		j = j;
	}
}

static void sccb_lock(sccb_tran_t *pSccb)
{
    osSemaphoreWait(pSccb->sccb_sem_id, osWaitForever);
}

static void sccb_unlock(sccb_tran_t *pSccb)
{
    osSemaphoreRelease(pSccb->sccb_sem_id);
}

static void sccb_start(sccb_tran_t *pSccb)
{
	gpio_write_io(pSccb->scl_port, DATA_HIGH);
	gpio_write_io(pSccb->sda_port, DATA_HIGH);
	sccb_delay (1,pSccb->clock_rate);
	gpio_write_io(pSccb->sda_port, DATA_LOW);
	sccb_delay (1,pSccb->clock_rate);
	gpio_write_io(pSccb->scl_port, DATA_LOW);
	sccb_delay (1,pSccb->clock_rate);
}
static void sccb_repeat_start(sccb_tran_t *pSccb)
{
    gpio_write_io(pSccb->sda_port, DATA_HIGH);
    sccb_delay (1,pSccb->clock_rate);
	gpio_write_io(pSccb->scl_port, DATA_HIGH);
	sccb_delay (1,pSccb->clock_rate);
	gpio_write_io(pSccb->sda_port, DATA_LOW);
	sccb_delay (1,pSccb->clock_rate);
	gpio_write_io(pSccb->scl_port, DATA_LOW);
	sccb_delay (1,pSccb->clock_rate);
}

static void sccb_stop(sccb_tran_t *pSccb)
{
	gpio_write_io(pSccb->scl_port, DATA_LOW);
	gpio_write_io(pSccb->sda_port, DATA_LOW);
	sccb_delay (1,pSccb->clock_rate);
	gpio_write_io(pSccb->scl_port, DATA_HIGH);
	sccb_delay (1,pSccb->clock_rate);
	gpio_write_io(pSccb->sda_port, DATA_HIGH);
	sccb_delay (1,pSccb->clock_rate);
}

static INT32S sccb_w_phase(sccb_tran_t *pSccb, INT16U value, INT8U ack)
{
	INT32U i;
	INT32S ret = STATUS_OK;

	for(i=0;i<8;i++){
		gpio_write_io(pSccb->scl_port, DATA_LOW);
		while(gpio_read_io(pSccb->scl_port));       //ming modify
		if(value & 0x80) {
			gpio_write_io(pSccb->sda_port, DATA_HIGH);
		} else {
			gpio_write_io(pSccb->sda_port, DATA_LOW);
		}

		sccb_delay (1,pSccb->clock_rate);
		gpio_write_io(pSccb->scl_port, DATA_HIGH);
		sccb_delay(1,pSccb->clock_rate);
		value <<= 1;
	}

	// The 9th bit transmission
	gpio_write_io(pSccb->scl_port, DATA_LOW);
	while(gpio_read_io(pSccb->scl_port));       //ming modify
	gpio_init_io(pSccb->sda_port, GPIO_INPUT);	//SDA is Hi-Z mode
	sccb_delay(1,pSccb->clock_rate);
	gpio_write_io(pSccb->scl_port, DATA_HIGH);

	// check ack = low
	if(ack) {
		for(i=0; i<pSccb->timeout; i++) {
			if(gpio_read_io(pSccb->sda_port) == 0) {
				break;
			}
		}
	}

	if(i == pSccb->timeout) {
		ret = STATUS_FAIL;
	}

	sccb_delay(1,pSccb->clock_rate);
	gpio_write_io(pSccb->scl_port, DATA_LOW);
	gpio_init_io(pSccb->sda_port, GPIO_OUTPUT);	//SDA is Hi-Z mode
	return ret;
}

static INT16U sccb_r_phase(sccb_tran_t *pSccb, INT8U phase)
{
	INT16U i;
	INT16U data = 0x00;

	gpio_init_io(pSccb->sda_port, GPIO_INPUT);	//SDA is Hi-Z mode
	for(i=0;i<8;i++) {
		gpio_write_io(pSccb->scl_port, DATA_LOW);
		sccb_delay(1,pSccb->clock_rate);
		gpio_write_io(pSccb->scl_port, DATA_HIGH);
		data <<= 1;
		while(!gpio_read_io(pSccb->scl_port));     //ming modify
		data |=( gpio_read_io(pSccb->sda_port));
		sccb_delay(1,pSccb->clock_rate);
	}

	// The 9th bit transmission
	gpio_write_io(pSccb->scl_port, DATA_LOW);
	while(gpio_read_io(pSccb->scl_port));      //ming modify
	gpio_init_io(pSccb->sda_port, GPIO_OUTPUT);	//SDA is output mode
	if(phase == 2) {
		gpio_write_io(pSccb->sda_port, DATA_LOW);       //SDA0, the nineth bit is ACK must be 1
	} else {
		gpio_write_io(pSccb->sda_port, DATA_HIGH);	//SDA0, the nineth bit is NAK must be 1
	}

	sccb_delay(1,pSccb->clock_rate);
	gpio_write_io(pSccb->scl_port, DATA_HIGH);
	sccb_delay(1,pSccb->clock_rate);
	gpio_write_io(pSccb->scl_port, DATA_LOW);
	return data;
}

static INT32S sccb_write(sccb_tran_t *pData)
{
	INT8U temp;
	INT8U id = pData->id;
	INT8U addr_bits = pData->addr_bits;
	INT8U data_bits = pData->data_bits;
	INT8U ack = 1;
	INT16U addr = pData->addr;
	INT16U data = pData->data;
	INT32S ret;

	// 3-Phase write transmission
	// Transmission start
	sccb_start(pData);

	// Phase 1, SLAVE_ID
	ret = sccb_w_phase(pData, id, 1);
	if(ret < 0) {
		goto __exit;
	}

	// Phase 2, Register address
	while(addr_bits >= 8) {
		addr_bits -= 8;
		temp = addr >> addr_bits;
		ret = sccb_w_phase(pData, temp, ack);
		if(ret < 0) {
			goto __exit;
		}
	}

	// Phase 3, Register Data
	while(data_bits >= 8) {
		data_bits -= 8;
		if(data_bits) {
			ack = 1; //ACK
		} else {
			ack = 0; //NACK
		}

		temp = data >> data_bits;
		ret = sccb_w_phase(pData, temp, ack);
		if(ret < 0) {
			goto __exit;
		}
	}

__exit:
	// Transmission stop
	sccb_stop(pData);
	return ret;
}

static INT32S sccb_write_1(sccb_tran_t *pData)
{
	INT8U temp;
	INT8U id = pData->id;
	INT8U addr_bits = pData->addr_bits;
	INT8U data_bits = pData->data_bits;
	INT8U ack = 1;
	INT32U addr = pData->addr;
	INT32U data = pData->data;
	INT32S ret;

	// 3-Phase write transmission
	// Transmission start
	sccb_start(pData);

	// Phase 1, SLAVE_ID
	ret = sccb_w_phase(pData, id|0x10, 1);
	if(ret < 0) {
		goto __exit;
	}

	// Phase 2, Register address
	while(addr_bits >= 8) {
		addr_bits -= 8;
		temp = addr >> addr_bits;
		ret = sccb_w_phase(pData, temp, ack);
		if(ret < 0) {
			goto __exit;
		}
	}

	// Phase 3, Register Data
	while(data_bits >= 8) {
		data_bits -= 8;

		if(data_bits) {
			ack = 1; //ACK
		} else {
			ack = 0; //NACK
		}


		//temp = data >> data_bits;
		temp = (INT8U)data&0xFF;
		ret = sccb_w_phase(pData, temp, ack);
		if(ret < 0) {
			goto __exit;
		}
		data >>= 8;
	}

__exit:
	// Transmission stop
	sccb_stop(pData);
	return ret;
}

static INT32S sccb_write_2(sccb_tran_t *pData)
{
	INT8U temp;
	INT8U id = pData->id;
	INT8U addr_bits = pData->addr_bits;
	INT16U data_bits = pData->data_bits;
	INT8U ack = 1;
	INT8U *paddr = (INT8U *)pData->paddr;
	INT8U *pdat = (INT8U *)pData->pdat;
	INT32S ret;
	//INT16U offset;

	// 3-Phase write transmission
	// Transmission start
	sccb_start(pData);

	// Phase 1, SLAVE_ID
	ret = sccb_w_phase(pData, id|0x10, 1);
	if(ret < 0) {
		goto __exit;
	}

	// Phase 2, command
	while(addr_bits >= 8) {
		addr_bits -= 8;
		//offset = addr_bits>>3;
		temp = *paddr++;
		ret = sccb_w_phase(pData, temp, ack);
		if(ret < 0) {
			DBG_PRINT("B\r\n");
			goto __exit;
		}
	}

	// Phase 3, data
	while(data_bits >= 8) {
		data_bits -= 8;
		//offset = data_bit>>3;
		if(data_bits) {
			ack = 1; //ACK
		} else {
			ack = 0; //NACK
		}


		//temp = data >> data_bits;
		temp = *pdat++;
		ret = sccb_w_phase(pData, temp, ack);
		if(ret < 0) {
			goto __exit;
		}
	}

__exit:
	// Transmission stop
	sccb_stop(pData);
	return ret;
}

static INT32S sccb_read(sccb_tran_t *pData)
{
	INT8U temp;
	INT8U id = pData->id;
	INT8U addr_bits = pData->addr_bits;
	INT8U data_bits = pData->data_bits;
	INT8U ack = 1;
	INT16U addr = pData->addr;
	INT32U read_data;
	INT32S ret;

	// 2-Phase write transmission cycle
	// Transmission start
	sccb_start(pData);

	// Phase 1, SLAVE_ID
	ret = sccb_w_phase(pData, id, 1);
	if(ret < 0) {
		goto __exit;
	}

	// Phase 2, Register Address
	while(addr_bits >= 8) {
		addr_bits -= 8;
		temp = addr >> addr_bits;
		ret = sccb_w_phase(pData, temp, ack);
		if(ret < 0) {
			goto __exit;
		}
	}

	// Transmission stop
	sccb_stop(pData);

	// 2-Phase read transmission cycle
	// Transmission start
	sccb_start(pData);

	// Phase 1 (read)
	ret = sccb_w_phase(pData, id | 0x01, 1);
	if(ret < 0) {
		goto __exit;
	}

	// Phase 2 (read)
	read_data = 0;
	while(data_bits >= 8) {
		data_bits -= 8;
		if(data_bits) {
			ack = 1; //ACK
		} else {
			ack = 0; //NACK
		}

		temp = sccb_r_phase(pData, ack);
		read_data <<= 8;
		read_data |= temp;
	}
	pData->data = read_data;

__exit:
	// Transmission stop
	sccb_stop(pData);
	return ret;
}

static INT32S sccb_read_1(sccb_tran_t *pData)
{
	INT8U temp;
	INT8U id = pData->id;
	INT8U addr_bits = pData->addr_bits;
	INT8U data_bits = pData->data_bits;
	INT8U ack = 1;
	INT32U addr = pData->addr;
	INT32U read_data;
	INT8U *pread_data = (INT8U *)(&read_data);
	INT32S ret;

	// 2-Phase write transmission cycle
	// Transmission start
	sccb_start(pData);

	// Phase 1, SLAVE_ID
	ret = sccb_w_phase(pData, id|0x10, 1);
	if(ret < 0) {
		DBG_PRINT("A\r\n");
		goto __exit;
	}

	// Phase 2, Register Address
	while(addr_bits >= 8) {
		addr_bits -= 8;
		temp = addr >> addr_bits;
		ret = sccb_w_phase(pData, temp, ack);
		if(ret < 0) {
			DBG_PRINT("B\r\n");
			goto __exit;
		}
	}

	// Transmission stop
	sccb_stop(pData);

	// 2-Phase read transmission cycle
	// Transmission start
	sccb_start(pData);

	// Phase 1 (read)
	ret = sccb_w_phase(pData, id | 0x11, 1);
	if(ret < 0) {
		DBG_PRINT("C1\r\n");
		goto __exit;
	}

	// Phase 2 (read)
	read_data = 0;
	while(data_bits >= 8) {
		data_bits -= 8;
		if(data_bits) {
			ack = 2; //ACK
		} else {
			ack = 0; //NACK
		}

		temp = sccb_r_phase(pData, ack);
		//read_data <<= 8;
		//read_data |= temp;
		*pread_data++ = temp;
	}
	pData->data = read_data;

__exit:
	// Transmission stop
	sccb_stop(pData);
	return ret;
}


static INT32S sccb_read_2(sccb_tran_t *pData)
{
	INT8U temp;
	INT8U id = pData->id;
	INT8U addr_bits = pData->addr_bits;
	INT16U data_bits = pData->data_bits;
	INT8U ack = 1;
	INT8U *paddr = (INT8U *)pData->paddr;
	INT8U *pdat = (INT8U *)pData->pdat;
	INT32S ret;

	// 2-Phase write transmission cycle
	// Transmission start
	sccb_start(pData);

	// Phase 1, SLAVE_ID
	ret = sccb_w_phase(pData, id|0x10, 1);
	if(ret < 0) {
		DBG_PRINT("A\r\n");
		goto __exit;
	}

	// Phase 2, Register Address
	while(addr_bits >= 8) {
		addr_bits -= 8;
		//offset = addr_bits>>3;
		temp = *paddr++;
		ret = sccb_w_phase(pData, temp, ack);
		if(ret < 0) {
			DBG_PRINT("B\r\n");
			goto __exit;
		}
	}

	// Transmission stop
	sccb_stop(pData);

	// 2-Phase read transmission cycle
	// Transmission start
	sccb_start(pData);

	// Phase 1 (read)
	ret = sccb_w_phase(pData, id | 0x11, 1);
	if(ret < 0) {
		DBG_PRINT("C2\r\n");
		goto __exit;
	}

	// Phase 2 (read)
	while(data_bits >= 8) {
		data_bits -= 8;
		//offset = data_bits>>3;
		if(data_bits) {
			ack = 2; //ACK
		} else {
			ack = 0; //NACK
		}

		temp = sccb_r_phase(pData, ack);
		*pdat++ = temp;
	}

__exit:
	// Transmission stop
	sccb_stop(pData);
	return ret;
}

static void sccb_init(sccb_tran_t *pSccb)
{
	gpio_set_port_attribute(pSccb->scl_port, ATTRIBUTE_HIGH);
	gpio_set_port_attribute(pSccb->sda_port, ATTRIBUTE_HIGH);
	gpio_init_io(pSccb->scl_port, GPIO_OUTPUT);		//set dir
	gpio_init_io(pSccb->sda_port, GPIO_OUTPUT);		//set dir
	gpio_write_io(pSccb->scl_port, DATA_HIGH);		//SCL
	gpio_write_io(pSccb->sda_port, DATA_HIGH);		//SDA

    if (pSccb->have_pwdn)
    {
        gpio_set_port_attribute(pSccb->pwdn_port, ATTRIBUTE_HIGH);
        gpio_init_io(pSccb->pwdn_port, GPIO_OUTPUT);	//set dir
        gpio_write_io(pSccb->pwdn_port, DATA_LOW);		//SDA0
    }

	gpio_drving_init_io(pSccb->scl_port, pSccb->scl_drv);
	gpio_drving_init_io(pSccb->sda_port, pSccb->sda_drv);

	// wait io stable
	sccb_delay(200,pSccb->clock_rate);
}

void *drv_l2_sccb_open_inside(INT32U scl_port, INT32U sda_port)
{
	INT8U i;
	osSemaphoreId same_port_sem = 0;
	sccb_tran_t *pSccb = NULL;
	sccb_tran_t *pSccb_inside;
    osSemaphoreId sem_id;
    osSemaphoreDef_t sccb_sem = { 0 };

	osSuspend();

	for(i=0; i<SCCB_MAX_SEM_ID; i++)
	{
		pSccb_inside = &sccb_sem_id_max[i];
		if(!pSccb && (pSccb_inside->id == 0)) // Content is null
		{
		    memset((void *)pSccb_inside, 0, sizeof(sccb_tran_t));
			pSccb = pSccb_inside;
		}
		else if((pSccb_inside->scl_port == scl_port) && (pSccb_inside->sda_port == sda_port))
		{
			same_port_sem = pSccb_inside->sccb_sem_id;
		}
	}

	osResume();

	if(pSccb == NULL)
	{
      	DBG_PRINT("sccb_sem_id_max array is full\r\n");
		return 0;
	}

	if(same_port_sem == 0)
	{
		sem_id = osSemaphoreCreate(&sccb_sem, 1);
		if(!sem_id)
		{
		  DBG_PRINT("sem_id error\r\n");
		  return NULL;
		}

		pSccb->sccb_sem_id = sem_id;
	}
	else
	{
		pSccb->sccb_sem_id = same_port_sem;
	}

	return pSccb;
}

/**
 * @brief   open a sccb request
 * @param   ID[in]: slave address
 * @param   RegBits[in]: register bit number
 * @param   DataBits[in]: data bit number
 * @return  sccb handle
 */
void *drv_l2_sccb_open(INT8U ID, INT8U RegBits, INT8U DataBits)
{
	sccb_tran_t *pSccb;

	pSccb = drv_l2_sccb_open_inside(SCCB_SCL,SCCB_SDA);
	if(pSccb == 0)
	{
            return 0;
		}

	pSccb->id = ID;
	pSccb->addr_bits = RegBits >> 3 << 3;
	pSccb->data_bits = DataBits >> 3 << 3;
	pSccb->ack = 1;

	//init IO
    pSccb->scl_port = SCCB_SCL;
    pSccb->sda_port = SCCB_SDA;

    pSccb->scl_drv = SCCB_SCL_DRV;
    pSccb->sda_drv = SCCB_SDA_DRV;

    pSccb->have_pwdn = 0;
    #if defined(CSI_PWDN)
    pSccb->have_pwdn = 1;
    pSccb->pwdn_port = CSI_PWDN;
    pSccb->pwdn_drv = SCCB_PWDN_DRV;
    #endif

    pSccb->timeout = SCCB_TIMEOUT;
    pSccb->clock_rate = SCCB_CLOCK_RATE;

    sccb_init(pSccb);

	return ((void *)pSccb);
}

/**
 * @brief   open a sccb request
 * @param   ID[in]: slave address
 * @param   RegBits[in]: register bit number
 * @param   DataBits[in]: data bit number
 * @param   pSccbConfig[in]: more scc config parameters
 * @return  sccb handle
 */
void *drv_l2_sccb_open_ext(sccb_config_t *pSccbConfig)
{
	sccb_tran_t *pSccb;

	pSccb = drv_l2_sccb_open_inside(pSccbConfig->scl_port,pSccbConfig->sda_port);
	if(pSccb == 0)
	{
            return 0;
		}

	pSccb->id = pSccbConfig->slaveAddr;
	pSccb->addr_bits = pSccbConfig->RegBits >> 3 << 3;
	pSccb->data_bits = pSccbConfig->DataBits >> 3 << 3;
	pSccb->ack = 1;

	//init IO
    pSccb->scl_port = pSccbConfig->scl_port;
    pSccb->scl_drv = pSccbConfig->scl_drv;

    pSccb->sda_port = pSccbConfig->sda_port;
    pSccb->sda_drv = pSccbConfig->sda_drv;

    pSccb->pwdn_port = pSccbConfig->pwdn_port;
    pSccb->pwdn_drv = pSccbConfig->pwdn_drv;
    pSccb->have_pwdn = pSccbConfig->have_pwdn;

    pSccb->timeout = pSccbConfig->timeout;
    pSccb->clock_rate = pSccbConfig->clock_rate;

    sccb_init(pSccb);

	return ((void *)pSccb);
}

/**
 * @brief   close a sccb request
 * @param   handle[in]: sccb handle
 * @return 	none
 */
void drv_l2_sccb_close(void *handle)
{
	INT8U i;
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;
	sccb_tran_t *pSccb_inside;
	INT8U sem_cnt = 0;

	if(pSccb)
	{
		osSuspend();

        // find use sem_id
            for(i=0;i<SCCB_MAX_SEM_ID;i++)
            {
			pSccb_inside = &sccb_sem_id_max[i];

            if(pSccb->sccb_sem_id == pSccb_inside->sccb_sem_id)
                {
				sem_cnt++;
                }
            }

		/*
			Only one uses this semaphore
		*/
		if(sem_cnt == 1)
        {
			vSemaphoreDelete(pSccb->sccb_sem_id);
			pSccb->sccb_sem_id = NULL;
        }

		osResume();

	    memset((void *)pSccb, 0, sizeof(sccb_tran_t));
		pSccb = 0;
	}
}

/**
 * @brief   sccb write data function
 * @param   handle[in]: sccb handle
 * @param   addr[in] : register address
 * @param   data[in] : write data
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_sccb_write(void *handle, INT32U addr, INT32U data)
{
	INT32S ret;
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	sccb_lock(pSccb);
	pSccb->addr = addr;
	pSccb->data = data;
	ret = sccb_write(pSccb);
	sccb_unlock(pSccb);
	return ret;
}

INT32S drv_l2_sccb_write_1(void *handle, INT32U addr, INT32U data)
{
	INT32S ret;
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	sccb_lock(pSccb);
	pSccb->addr = addr;
	pSccb->data = data;
	ret = sccb_write_1(pSccb);
	sccb_unlock(pSccb);
	return ret;
}

INT32S drv_l2_sccb_write_2(void *handle, INT32U *addr, INT32U *data)
{
	INT32S ret;
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	sccb_lock(pSccb);
	pSccb->paddr = addr;
	pSccb->pdat = data;
	ret = sccb_write_2(pSccb);
	sccb_unlock(pSccb);
	return ret;
}

/**
 * @brief   sccb continue write data function
 * @param   handle[in]: sccb handle
 * @param   addr[in] : register address
 * @param   data[in] : write data
 * @param   len[in] : write data length
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_sccb_continue_write(void *handle, INT32U addr, INT8U* data, INT32U len)
{
	INT32S ret;
	INT8U ack = 1;
	INT8U temp;
    INT8U addr_bits;
	INT8U data_bits;
	INT32U data_len = len;
	INT8U *data_out;
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	sccb_lock(pSccb);
	pSccb->addr = addr;
	data_out = data;
    addr_bits = pSccb->addr_bits;

	sccb_start(pSccb);

	// Phase 1, SLAVE_ID
	ret = sccb_w_phase(pSccb, pSccb->id, 1);
	if(ret < 0) {
		goto __exit;
	}

	// Phase 2, Register address
	while(addr_bits >= 8) {
		addr_bits -= 8;
		temp = pSccb->addr >> addr_bits;
		ret = sccb_w_phase(pSccb, temp, ack);
		if(ret < 0) {
			goto __exit;
		}
	}

	// Phase 3, Register Data
	while(data_len){
        data_bits = pSccb->data_bits;
        while(data_bits >= 8) {
            data_bits -= 8;
            if(data_len>1) {
                ack = 1; //ACK
            } else {
                if((data_bits >= 8))
                    ack = 1; //ACK
                else
                    ack = 0; //NACK
            }
            if(data_bits >= 8)
                temp = (*data_out) >> pSccb->data_bits;
            else
                temp = (*data_out);
            ret = sccb_w_phase(pSccb, temp, ack);
            data_out++;
            if(ret < 0) {
                goto __exit;
            }
		}
		data_len--;

	}

__exit:
	// Transmission stop
	sccb_stop(pSccb);

	sccb_unlock(pSccb);
	return ret;
}

/**
 * @brief   sccb read for HDMI E-DDC read function
 * @param   handle_seg[in]: sccb handle for HDMI E-DDC segment pointer
 * @param   handle[in]: sccb handle for HDMI DDC
 * @param   addr_seg[in] : Segment address
 * @param   addr[in] : DDC address
 * @param   data[out] : read data
 * @param   len[in] : read data length
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_sccb_seg_p_edid(void *handle_seg, void *handle, INT16U addr_seg, INT16U addr, INT8U *data, INT32U len)
{
    INT32S ret;
	INT8U ack = 1;
	INT8U temp;
    INT8U addr_bits;
	INT8U data_bits;
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;
    sccb_tran_t *pSccb_seg = (sccb_tran_t *)handle_seg;
	INT32U data_len = len;
    INT8U *data_read;

	if(handle_seg == 0) {
		return STATUS_FAIL;
	}

	if(handle == 0) {
		return STATUS_FAIL;
	}

	sccb_lock(pSccb_seg);


	pSccb_seg->addr = addr_seg;
    addr_bits = pSccb_seg->addr_bits;

	sccb_start(pSccb_seg);

	// Phase 1, SLAVE_ID
	ret = sccb_w_phase(pSccb_seg, pSccb_seg->id, 0);
	if(ret < 0) {
		goto __exit;
	}

	// Phase 2, Register address
	while(addr_bits >= 8) {
		addr_bits -= 8;
		temp = pSccb_seg->addr >> addr_bits;
		ret = sccb_w_phase(pSccb_seg, temp, 0);
		if(ret < 0) {
			goto __exit;
		}
	}

    sccb_unlock(pSccb_seg);
    sccb_lock(pSccb);

	pSccb->addr = addr;
    addr_bits = pSccb->addr_bits;

    data_read = data;
	// 2-Phase write transmission cycle
	// Transmission start
	sccb_repeat_start(pSccb);

	// Phase 1, SLAVE_ID
	ret = sccb_w_phase(pSccb, pSccb->id, 1);
	if(ret < 0) {
		goto __exit;
	}

	// Phase 2, Register Address
	while(addr_bits >= 8) {
		addr_bits -= 8;
		temp = pSccb->addr >> addr_bits;
		ret = sccb_w_phase(pSccb, temp, ack);
		if(ret < 0) {
			goto __exit;
		}
	}

	// 2-Phase read transmission cycle
	// Transmission start
	sccb_repeat_start(pSccb);

	// Phase 1 (read)
	ret = sccb_w_phase(pSccb, pSccb->id | 0x01, 1);
	if(ret < 0) {
		goto __exit;
	}

	// Phase 2 (read)
	while(data_len) {
        data_bits = pSccb->data_bits;
        while(data_bits >= 8) {
            data_bits -= 8;
            if(data_len>1)
            {
                ack = 2; //ACK
            }
            else
            {
                if(data_bits) {
                    ack = 2; //ACK
                } else {
                    ack = 0; //NACK
                }
            }

            temp = sccb_r_phase(pSccb, ack);
            *data_read = temp;
            data_read++;
        }
        data_len--;
    }
__exit:
	// Transmission stop
	sccb_stop(pSccb);

	sccb_unlock(pSccb);

	return ret;
}

/**
 * @brief   sccb continue read data function
 * @param   handle[in]: sccb handle
 * @param   addr[in] : register address
 * @param   data[out] : read data
 * @param   len[in] : read data length
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_sccb_continue_read(void *handle, INT32U addr, INT8U *data, INT32U len)
{
	INT32S ret;
	INT32U data_len = len;
	INT8U temp;
	INT8U addr_bits;
	INT8U data_bits;
	INT8U ack = 1;
    INT8U *data_read;
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	sccb_lock(pSccb);
	pSccb->addr = addr;
    addr_bits = pSccb->addr_bits;

    data_read = data;
	// 2-Phase write transmission cycle
	// Transmission start
	sccb_start(pSccb);

	// Phase 1, SLAVE_ID
	ret = sccb_w_phase(pSccb, pSccb->id, 1);
	if(ret < 0) {
		goto __exit;
	}

	// Phase 2, Register Address
	while(addr_bits >= 8) {
		addr_bits -= 8;
		temp = pSccb->addr >> addr_bits;
		ret = sccb_w_phase(pSccb, temp, ack);
		if(ret < 0) {
			goto __exit;
		}
	}

	// 2-Phase read transmission cycle
	// Transmission start
	sccb_repeat_start(pSccb);

	// Phase 1 (read)
	ret = sccb_w_phase(pSccb, pSccb->id | 0x01, 1);
	if(ret < 0) {
		goto __exit;
	}

	// Phase 2 (read)
	while(data_len) {
        data_bits = pSccb->data_bits;
        while(data_bits >= 8) {
            data_bits -= 8;
            if(data_len>1)
            {
                ack = 2; //ACK
            }
            else
            {
                if(data_bits) {
                    ack = 2; //ACK
                } else {
                    ack = 0; //NACK
                }
            }
            temp = sccb_r_phase(pSccb, ack);
            *data_read = temp;
            data_read++;
        }
        data_len--;
    }
__exit:
	// Transmission stop
	sccb_stop(pSccb);

	sccb_unlock(pSccb);
	return ret;
}
/**
 * @brief   sccb read data function
 * @param   addr[in] : register address
 * @param   data[out] : read data
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_sccb_read(void *handle, INT32U addr, INT16U *data)
{
	INT32S ret;
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	sccb_lock(pSccb);
	pSccb->addr = addr;
	ret = sccb_read(pSccb);
	if(ret >= 0) {
		*data = pSccb->data;
	} else {
		*data = 0xFF;
	}

	sccb_unlock(pSccb);
	return ret;
}

INT32S drv_l2_sccb_read_1(void *handle, INT32U addr, INT32U *data)
{
	INT32S ret;
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	sccb_lock(pSccb);
	pSccb->addr = addr;
	ret = sccb_read_1(pSccb);
	if(ret >= 0) {
		*data = pSccb->data;
	} else {
		*data = 0xFF;
		DBG_PRINT("F2\r\n");
	}

	sccb_unlock(pSccb);
	return ret;
}

INT32S drv_l2_sccb_read_2(void *handle, INT32U *addr, INT32U *data)
{
	INT32S ret;
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	sccb_lock(pSccb);
	pSccb->paddr = addr;
	pSccb->pdat = data;
	ret = sccb_read_2(pSccb);
	if(ret >= 0) {
		//*data = pSccb->data;
	} else {
		*data = 0xFF;
		DBG_PRINT("F3\r\n");
	}

	sccb_unlock(pSccb);
	return ret;
}

INT32S drv_l2_sccb_get_addr_bits(void *handle)
{
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	return pSccb->addr_bits;
}

INT32S drv_l2_sccb_set_addr_bits(void *handle, INT32U bits)
{
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	//if(bits !=8 || bits != 16 || bits != 32){
	//	return STATUS_FAIL;
	//}
	if(bits & 0x3){
		DBG_PRINT("addr bits is not 8 multiple\r\n ");
	}

	pSccb->addr_bits = bits;
	return STATUS_OK;

}


INT32S drv_l2_sccb_get_data_bits(void *handle)
{
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	return pSccb->data_bits;
}

INT32S drv_l2_sccb_set_data_bits(void *handle, INT32U bits)
{
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	//if(bits !=8 || bits != 16 || bits != 32){
	//	return STATUS_FAIL;
	//}
	if(bits & 0x3){
		DBG_PRINT("data bits is not 8 multiple\r\n ");
	}

	pSccb->data_bits = bits;
	return STATUS_OK;

}

void drv_l2_sccb_stop(void *handle)
{
    sccb_tran_t *pSccb = (sccb_tran_t *)handle;
    sccb_stop(pSccb);
}

INT8U drv_l2_sccb_get_slave_id(void *handle)
{
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	return pSccb->id;
}

INT32S drv_l2_sccb_set_slave_id(void *handle, INT32U slave_id)
{
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;

	if(handle == 0) {
		return STATUS_FAIL;
	}

	pSccb->id = (INT8U)slave_id;
	return STATUS_OK;
}

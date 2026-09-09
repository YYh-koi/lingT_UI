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
#include "drv_l1.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define I2C_RESTART_WITHOUT_STOP     0x01
#define DBG_I2C_ENABLE		         0

#define DBG_I2C_PIN_EN               0
#define I2C_CLOCK_SOURCE             8
#define I2C_CLOCK_ENABLE             1
#define I2C_CLOCK_DISABLE            0

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if DBG_I2C_ENABLE == 1
#define DRV_I2C_DBG		DBG_PRINT
#else
#define DRV_I2C_DBG(...)
#endif

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static i2cReg_t* _i2c_select_sfr_base(INT32S DevNo);
static void _i2c_msec_wait(INT32U msec);
static INT32S _i2c_busy_waiting(i2cReg_t *pI2CDev, INT32S ms);
static void _i2cSetClkRate(i2cReg_t *pI2CDev, INT16U clkRate);
static INT32S _i2cStartTran(i2cReg_t *pI2CDev, INT16U slaveAddr, INT16U clkRate, INT8U cmd, INT8U aAck);
static void _i2cStopTran(i2cReg_t *pI2CDev, INT8U cmd);
static INT32S _i2cMiddleTran(i2cReg_t *pI2CDev, INT8U *data, INT8U cmd, INT8U aAck);
static INT32S _i2c_bus_xfer(drv_l1_i2c_bus_handle_t *hd, INT8U* data, INT16U len, INT8U cmd);
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static osMessageQId i2c_rx_q = 0;
static osMessageQDef_t rx_q_def = {1, sizeof(INT32U), 0};

 /**
 * @brief   I2C set clock
 * @param   enable: enable clock (enable/disable)
 * @return 	0(successful) or -1(failed)
 */
INT16S i2c_clock_set(INT32U dev,INT16U enable)
{
    INT16U module=0;
    if(dev == I2C_0){
        module = CLK_EN2_I2C0;
    }
    else if(dev == I2C_1) {
        module = CLK_EN2_I2C1;
    }
    else if(dev == I2C_2) {
        module = CLK_EN2_I2C2;
    }
    return drv_l1_clock_set_system_clk_en(module, enable);
}

 /**
 * @brief   I2C select base address
 * @param   DevNo: I2C_0/I2C_1/I2C_2
 * @return 	address or 0(failed)
 */
static i2cReg_t* _i2c_select_sfr_base(INT32S DevNo)
{
    if(DevNo == I2C_0) {
        return (i2cReg_t *)I2C0_BASE;
    }
    else if(DevNo == I2C_1) {
        return (i2cReg_t *)I2C1_BASE;
    }
    else if(DevNo == I2C_2) {
        return (i2cReg_t *)I2C2_BASE;
    }

	return 0;
}

 /**
 * @brief   I2C select DMA base address
 * @param   channel: C_DMA_CH0~C_DMA_CH7
 * @return 	address or 0(failed)
 */
i2c_dmaReg_t* _i2c_select_dma_base(INT32U channel)
{
    if(channel == C_DMA_CH0) {
        return (i2c_dmaReg_t *)DMA0_BASE;
    }
    else if(channel == C_DMA_CH1) {
        return (i2c_dmaReg_t *)DMA1_BASE;
    }
    else if(channel == C_DMA_CH2) {
        return (i2c_dmaReg_t *)DMA2_BASE;
    }
    else if(channel == C_DMA_CH3) {
        return (i2c_dmaReg_t *)DMA3_BASE;
    }
    else if(channel == C_DMA_CH4) {
        return (i2c_dmaReg_t *)DMA4_BASE;
    }
    else if(channel == C_DMA_CH5) {
        return (i2c_dmaReg_t *)DMA5_BASE;
    }
    else if(channel == C_DMA_CH6) {
        return (i2c_dmaReg_t *)DMA6_BASE;
    }
    else if(channel == C_DMA_CH7) {
        return (i2c_dmaReg_t *)DMA7_BASE;
    }

	return 0;
}

 /**
 * @brief   I2C wait msec
 * @param   msec: msec
 * @return 	None
 */
static void _i2c_msec_wait(INT32U msec)
{
	drv_msec_wait(msec);
}

 /**
 * @brief   Check I2C busy status
 * @param   pI2CDev: I2C base address
 * @param   ms: wait time
 * @return 	0(successful) or -1(failed)
 */
static INT32S _i2c_busy_waiting(i2cReg_t *pI2CDev, INT32S ms)
{
	INT32S timeout = ms * 75;
	while((pI2CDev->ICSR & MASK_I2C_SR_BUSYSTA))
	{
		timeout--;
		if(timeout < 0) {
			return -1;
		}
		_i2c_msec_wait(1);
	}

	return 0;
}

 /**
 * @brief   Check I2C-bus TX/RX interrupt pending flag
 * @param   pI2CDev: I2C base address
 * @param   ms: wait time
 * @return 	0(successful) or -1(failed)
 */
static INT32S _i2c_intpend_waiting(i2cReg_t *pI2CDev, INT32S ms)
{
	INT32S timeout = ms * 75;
	//print_string("ICCR 0x%x \r\n",pI2CDev->ICCR);
	while((pI2CDev->ICCR & MASK_I2C_CR_INTRPEND) == 0)
	{
		timeout--;
		if(timeout < 0) {
			return -1;
		}
		_i2c_msec_wait(1);
	}

	return 0;
}

 /**
 * @brief   Check I2C-bus TX/RX interrupt pending flag in DMA mode
 * @param   pI2CDev: I2C base address
 * @param   ms: wait time
 * @return 	0(successful) or -1(failed)
 */
INT32S _i2c_intpend_waiting_dma(i2cReg_t *pI2CDev, INT32S ms)
{
	INT32S timeout = ms * 75;

	while((pI2CDev->ICCR & MASK_I2C_CR_DMA_INT) == 0)
	{
		timeout--;
		if(timeout < 0) {
			return -1;
		}
		_i2c_msec_wait(1);
	}

	return 0;
}

 /**
 * @brief   Set I2C clock rate
 * @param   pI2CDev: I2C base address
 * @param   clkRate: clock rate
 * @return 	None
 */
static void _i2cSetClkRate(i2cReg_t *pI2CDev, INT16U clkRate)
{
	//==================================================================================
	// I2C_Clock = SYSCLK /( { ( TXCLKPRE[3:0], TXCLKLSB[7:0] ) + 1 } * 2)
	// ( TXCLKPRE[3:0], TXCLKLSB[7:0] ) means [4 + 8 = 12] bits combined as following
	//
	// -------------------------------------------------
	// | TXCLKPRE[3:0] |         TXCLKLSB[7:0]         |
	// -------------------------------------------------
	// | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
	// -------------------------------------------------
	//
	// ( TXCLKPRE[3:0], TXCLKLSB[7:0] ) = ( SYSCLK / 2 / I2C_Clock ) - 1
	//==================================================================================
	INT32U tmp = 0;

	tmp = SystemCoreClock / (2*clkRate*1000);
	if (tmp > 0xFFF) {
		tmp = 0xFFF;
	} else if (tmp > 0) {
		tmp--;
	}

	// Setup TXCLKLSB[7:0]
	pI2CDev->TXCLKLSB = (tmp & 0xFF);
	// Setup TXCLKPRE[3:0] in ICCR
	pI2CDev->ICCR &= ~0xF;
	pI2CDev->ICCR |= ((tmp & 0xF00) >> 8);
#if DBG_I2C_PIN_EN == 1
	pI2CDev->IDEBCLK = ((SystemCoreClock / (5*clkRate*1000)) & 0xFF);
#endif

}

 /**
 * @brief   I2C start transmit
 * @param   pI2CDev: I2C base address
 * @param   slaveAddr: slave address
 * @param   clkRate: clock rate
 * @param   cmd: I2C_BUS_WRITE/I2C_BUS_READ
 * @param   aAck: Wait ack or not
 * @return 	0(successful) or -1(failed)
 */
static INT32S _i2cStartTran(i2cReg_t *pI2CDev, INT16U slaveAddr, INT16U clkRate, INT8U cmd, INT8U aAck)
{
	INT32S ret = 1;
	INT32U iccr = 0, ctrl = 0;
	INT32S timeout;

	/* check i2c bus is idle or not */
	ret = _i2c_busy_waiting(pI2CDev, I2C_TIME_OUT);
	if (ret < 0) {
		DRV_I2C_DBG("[%s]-- I2C bus is busy\r\n", __func__);
		return -1;
	}
	_i2cSetClkRate(pI2CDev, clkRate);

	iccr = pI2CDev->ICCR;
	//iccr = (iccr & I2C_ICCR_TXCLKMSB_MASK) | MASK_I2C_CR_INTREN;
	// [TODO]
	// TXCLKMSB_MASK was set in _i2cSetClkRate() and I2C does not use interrupt.
	// The following line should be un-necessary.
	//												by Craig 2012.6.28

	iccr &= I2C_ICCR_TXCLKMSB_MASK | MASK_I2C_CR_INTREN;
	switch (cmd)
	{
		case I2C_BUS_WRITE:
			iccr |= I2C_ICCR_INIT;
			ctrl = I2C_ICSR_MASTER_TX | MASK_I2C_SR_DOEN;
			break;

		case I2C_BUS_READ:
			iccr |= MASK_I2C_CR_ACKEN;
			ctrl = I2C_ICSR_MASTER_RX | MASK_I2C_SR_DOEN;
			break;

		default:
			return -1;
			break;
	}
	pI2CDev->ICCR = iccr;
	pI2CDev->ICSR = ctrl;
	if (cmd == I2C_BUS_READ) {
		pI2CDev->IDSR = (slaveAddr & 0xFF) | 0x01;
		pI2CDev->IDSR = (slaveAddr & 0xFF) | 0x01; 	//Delay 1T for fixed write fail
	}
	else {
		pI2CDev->IDSR = slaveAddr & 0xFE;
		pI2CDev->IDSR = slaveAddr & 0xFE; 			//Delay 1T for fixed write fail
	}
    //print_string("pI2CDev->IDSR %x\r\n",pI2CDev->IDSR);
    // I2C Start to Send Start Signal
	pI2CDev->ICSR |= MASK_I2C_SR_BUSYSTA;

	ret = _i2c_intpend_waiting(pI2CDev, I2C_TIME_OUT);

	if (ret < 0) {
		DRV_I2C_DBG("[%s]-- Interrupt is not received\r\n", __func__);
		return -1;
	}


    if(pI2CDev->ICSR & MASK_I2C_SR_APBIT)
	{
		// data tx fail
		DRV_I2C_DBG("[%s]--data bus arbiter is failed.\r\n", __func__);

        return -1;
	}

	timeout = NO_ACK_TIMEOUT * 75;
	while ((pI2CDev->ICSR & MASK_I2C_SR_LRBIT) && aAck)
	{
		// Waiting for ACK
		timeout--;
		if(timeout < 0) {
			DRV_I2C_DBG("[%s]-- Waiting ACK timeout\r\n", __func__);
			ret = -1;
			break;
		}
		_i2c_msec_wait(1);
	}

	return ret;
}

 /**
 * @brief   I2C Restart transmit
 * @param   pI2CDev: I2C base address
 * @param   slaveAddr: slave address
 * @return 	0(successful) or -1(failed)
 */
static INT32S _i2cBusRestartTran(i2cReg_t *pI2CDev, INT16U slaveAddr)
{
	INT32U ctrl = 0;
	INT32U iccr = 0;
	INT32S ret = 0;

	iccr = pI2CDev->ICCR;
	iccr &= I2C_ICCR_TXCLKMSB_MASK | MASK_I2C_CR_INTREN;

    pI2CDev->IDSR = (slaveAddr & 0xFF) | 0x01;
    pI2CDev->IDSR = (slaveAddr & 0xFF) | 0x01;  //Delay 1T for fixed write fail

    iccr |= MASK_I2C_CR_ACKEN;
    ctrl = MASK_I2C_SR_BUSYSTA | I2C_ICSR_MASTER_RX | MASK_I2C_SR_DOEN;

	pI2CDev->ICCR = iccr;
	pI2CDev->ICSR = ctrl;

	ret = _i2c_intpend_waiting(pI2CDev, I2C_TIME_OUT);
	if (ret < 0) {
		DRV_I2C_DBG("[%s]-- Interrupt is not received\r\n", __func__);
		return -1;
	}

	return ret;
}

 /**
 * @brief   I2C Stop transmit
 * @param   pI2CDev: I2C base address
 * @param   cmd: I2C_BUS_WRITE/I2C_BUS_READ
 * @return 	None
 */
static void _i2cStopTran(i2cReg_t *pI2CDev, INT8U cmd)
{
	INT32S ret=0;
	INT32U ctrl = 0;

	if (cmd == I2C_BUS_WRITE) {
		ctrl = I2C_ICSR_MASTER_TX | MASK_I2C_SR_DOEN;
	} else {
		ctrl = I2C_ICSR_MASTER_RX | MASK_I2C_SR_DOEN;
	}

	pI2CDev->ICSR = ctrl;
	pI2CDev->MISC |= (1 << 2);
	// Stop transmition
	ret = _i2c_busy_waiting(pI2CDev, I2C_TIME_OUT);
	if (ret < 0) {
		DRV_I2C_DBG("[%s]-- I2C bus is busy\r\n", __func__);
		return;
	}
}

 /**
 * @brief   I2C transmit data
 * @param   pI2CDev: I2C base address
 * @param   data: Write/Read data
 * @param   cmd: I2C_BUS_WRITE/I2C_BUS_READ
 * @param   aAck: wait ack or not
 * @return 	0(successful) or -1(failed)
 */
static INT32S _i2cMiddleTran(i2cReg_t *pI2CDev, INT8U *data, INT8U cmd, INT8U aAck)
{
	INT32S ret = 1;
	INT32U iccr = 0;
	INT32S timeout;

	iccr = pI2CDev->ICCR;
	iccr &= I2C_ICCR_TXCLKMSB_MASK;

	switch (cmd)
	{
		case I2C_BUS_WRITE:
			pI2CDev->IDSR = *data & 0xFF;
			pI2CDev->IDSR = *data & 0xFF; //Delay 1T for fixed write fail
			iccr |= MASK_I2C_CR_INTRPEND;
			break;

		case I2C_BUS_READ:
            if (aAck == 1) {
				iccr |= MASK_I2C_CR_ACKEN | MASK_I2C_CR_INTRPEND;
            }
            else {
				iccr |= MASK_I2C_CR_INTRPEND;
            }
            break;

		default:
			return -1;
			break;
	}

	/* clear irq */
	pI2CDev->ICCR = iccr;
	// Interrupt Pending
	ret = _i2c_intpend_waiting(pI2CDev, I2C_TIME_OUT);
	if (ret < 0) 
	{
		DRV_I2C_DBG("[%s]-- Interrupt is not received\r\n", __func__);
		return -1;
	}

    if(pI2CDev->ICSR & MASK_I2C_SR_APBIT)
	{
		// data tx fail
		DRV_I2C_DBG("[%s]--data bus arbiter is failed.\r\n", __func__);

        return -1;
	}

	timeout = NO_ACK_TIMEOUT * 75;
	// Ack Received ?
	while ((pI2CDev->ICSR & MASK_I2C_SR_LRBIT) && aAck)
	{
		// Waiting for ACK
		timeout--;
		if(timeout < 0) {
			DRV_I2C_DBG("[%s]-- Waiting ACK timeout\r\n", __func__);
			ret = -1;
			break;
		}
		_i2c_msec_wait(1);
	}

	*data = (pI2CDev->IDSR & 0xFF);
	return ret;
}

 /**
 * @brief   I2C bus transfer
 * @param   hd: i2c bus handle
 * @param   data: Write/Read data
 * @param   len: data length
 * @param   cmd: I2C_BUS_WRITE/I2C_BUS_READ
 * @return 	0(successful) or -1(failed)
 */
static INT32S _i2c_bus_xfer(drv_l1_i2c_bus_handle_t *hd, INT8U* data, INT16U len, INT8U cmd)
{
	INT32S i=0, ret = -1;
    i2cReg_t *pI2CDev = _i2c_select_sfr_base(hd->devNumber);
	pI2CDev->MISC |= 0x1;
	if (hd->clkRate == 0) {
		DRV_I2C_DBG("[%s]-- Error: i2c clock rate must to be more than zero\r\n", __func__);
		goto __exit;
	}

	if (cmd == I2C_BUS_WRITE) {
		ret = _i2cStartTran(pI2CDev, hd->slaveAddr, hd->clkRate, I2C_BUS_WRITE, 0);
		if (ret < 0) {
			DRV_I2C_DBG("WRITE-Error: write slave device address fail\r\n", __func__);
			goto __exit;
		}

		for (i = 0; i < len; i++)
		{
			if (i==(len-1)) {
				ret = _i2cMiddleTran(pI2CDev, (data+i), I2C_BUS_WRITE, 0);
			}
            else {
				ret = _i2cMiddleTran(pI2CDev, (data+i), I2C_BUS_WRITE, 1);
			}
			if (ret < 0) {
				DRV_I2C_DBG("WRITE-Error: write data fail\r\n", __func__);
				goto __exit;
			}
		}

		_i2cStopTran(pI2CDev, I2C_BUS_WRITE);
	}
	else if (cmd == I2C_BUS_READ) {
		ret = _i2cStartTran(pI2CDev, hd->slaveAddr, hd->clkRate, I2C_BUS_READ, 1);
		if (ret < 0) {
			DRV_I2C_DBG("READ-Error: write slave device address fail\r\n", __func__);
			goto __exit;
		}

		for (i = 0; i < len; i++)
		{
			if (i == (len-1)) {
				ret = _i2cMiddleTran(pI2CDev, (data+i), I2C_BUS_READ, 0);
			}
            else {
				ret = _i2cMiddleTran(pI2CDev, (data+i), I2C_BUS_READ, 1);
			}

			if (ret < 0) {
				DRV_I2C_DBG("READ-Error: read data fail\r\n", __func__);
				goto __exit;
			}
		}

		_i2cStopTran(pI2CDev, I2C_BUS_READ);
	}

	ret = i;
__exit:
    pI2CDev->MISC &= ~0x1;

	return ret;
}

 /**
 * @brief   I2C bus transfer in DMA mode
 * @param   hd: i2c bus handle
 * @param   wdata: Write data buffer
 * @param   rdata: Read data buffer
 * @param   len: Total data length
 * @param   addr_len: address length
 * @param   data_len: data length
 * @param   cmd: I2C_BUS_WRITE/I2C_BUS_READ
 * @return 	0(successful) or -1(failed)
 */
static INT32S _i2c_bus_xfer_dma(drv_l1_i2c_bus_handle_t *hd, INT8U* wdata, INT8U* rdata, INT16U len, INT8U addr_len, INT16U data_len, INT8U cmd)
{
    int ret;
    INT32U flag = 0, int_flag=0; //DMA timeout and interrupt flag
    i2cReg_t *pI2CDev = _i2c_select_sfr_base(hd->devNumber);
    DMA_STRUCT dma_rx={0}, dma_tx={0};
    volatile INT8S done_rx, done_tx;
    INT32S status;

    ret = _i2c_busy_waiting(pI2CDev, I2C_TIME_OUT);
	if (ret < 0) {
		DRV_I2C_DBG("[%s]-- I2C bus is busy\r\n", __func__);
		return -1;
	}

    hd->slaveAddr &= ~(0x1);
	_i2cSetClkRate(pI2CDev,hd->clkRate);
    if(cmd == I2C_BUS_WRITE){
        drv_l1_cp15_clean_dcache_range((INT32U) wdata, len);
        pI2CDev->ICSR = MASK_I2C_SR_DOEN; //Enable TX
        done_tx = C_DMA_STATUS_WAITING;
        dma_tx.s_addr = (INT32U) wdata;
        dma_tx.t_addr = (INT32U) &pI2CDev->TXDMA_IDSR;
        dma_tx.count = (INT32U) len;
        dma_tx.width = DMA_DATA_WIDTH_1BYTE;
        dma_tx.notify = &done_tx;
        dma_tx.timeout = 0;//255;
        //dma_tx.aes = 0;
        dma_tx.trigger = 0;
        status = drv_l1_dma_transfer(&dma_tx);
        if(status != 0) {
            print_string("fail!\r\n");
            return status;
        }

        pI2CDev->DMA_DID = hd->slaveAddr;
        pI2CDev->ADDR_CR = addr_len;
        pI2CDev->DATA_CR = data_len;
        pI2CDev->STOP_CR = len;
        pI2CDev->MISC = MASK_I2C_MISC_GPIOEN | MASK_I2C_MISC_DMA_MODE | MASK_I2C_MISC_MASTER_TX;
        pI2CDev->ICCR |= MASK_I2C_CR_INTREN | MASK_I2C_CR_ACKEN;
        pI2CDev->ICSR = MASK_I2C_SR_DOEN | MASK_I2C_SR_BUSYSTA;
        _i2c_msec_wait(1);
    }
    else if(cmd == I2C_BUS_READ){
        drv_l1_cp15_clean_dcache_range((INT32U) rdata, len);
        drv_l1_cp15_invalidate_dcache_range((INT32U) rdata, len);
        pI2CDev->ICSR = MASK_I2C_SR_DOEN; //Enable RX
        done_tx = C_DMA_STATUS_WAITING;
        dma_tx.s_addr = (INT32U) wdata;
        dma_tx.t_addr = (INT32U) &pI2CDev->TXDMA_IDSR;
        dma_tx.count = addr_len;

        dma_tx.width = DMA_DATA_WIDTH_1BYTE;
        dma_tx.notify = &done_tx;
        dma_tx.timeout = 0;//255;
        //dma_tx.aes = 0;
        dma_tx.trigger = 0;
        status = drv_l1_dma_transfer(&dma_tx);
        if(status != 0) {
            print_string("fail!\r\n");
            return status;
        }

        dma_rx.s_addr = (INT32U) &pI2CDev->RXDMA_IDSR;
		dma_rx.t_addr = (INT32U) rdata;
        dma_rx.count = data_len;
        dma_rx.width = DMA_DATA_WIDTH_1BYTE;
        dma_rx.notify = &done_rx;
        dma_rx.timeout = 0;//255;
        //dma_rx.aes = 0;
        dma_rx.trigger = 0;
        status = drv_l1_dma_transfer(&dma_rx);
        if(status != 0) {
            print_string("fail!\r\n");
            return status;
        }

        pI2CDev->DMA_DID = hd->slaveAddr;
        pI2CDev->ADDR_CR = addr_len;
        pI2CDev->DATA_CR = data_len;
        pI2CDev->STOP_CR = len;
        pI2CDev->MISC = MASK_I2C_MISC_GPIOEN | MASK_I2C_MISC_DMA_MODE | MASK_I2C_MISC_MASTER_RX;
        pI2CDev->ICCR |= MASK_I2C_CR_INTREN | MASK_I2C_CR_ACKEN;
        pI2CDev->ICSR = MASK_I2C_SR_DOEN | MASK_I2C_SR_BUSYSTA;
        _i2c_msec_wait(1);
    }
    //---------- check interrupt pending ----------//
    ret = _i2c_intpend_waiting_dma(pI2CDev, I2C_TIME_OUT);
    if (ret < 0) {
		DRV_I2C_DBG("[%s]-- Interrupt is not received\r\n", __func__);
		return -1;
	}
    //---------- clear interrupt ----------//
    pI2CDev->ICCR |= MASK_I2C_CR_DMA_INT;

    if(cmd == I2C_BUS_WRITE){
        pI2CDev->ICSR = MASK_I2C_SR_DOEN;
        pI2CDev->MISC &= ~(0x11);
        if (R_DMA_INT & flag) {
			return -1;
        }
        R_DMA_INT = int_flag;
    }
    else if(cmd == I2C_BUS_READ){
        pI2CDev->ICSR =  MASK_I2C_SR_DOEN;
        pI2CDev->MISC &= ~(0x91);
    }
    return STATUS_OK;
}

/**
 * @brief   I2C bus data transfer function
 * @param   handle[in]: i2c bus handle
 * @param   subaddr[in]: sub-address buffer
 * @param   length[in]: sub-address length
 * @param   data[in]: data buffer
 * @param	data_len[in]: data length
 * @param   cmd[in]: I2C_BUS_WRITE/I2C_BUS_READ
 * @param	mode[in]:i2c restart without stop or not: 1、I2C_RESTART_WITHOUT_STOP 0、I2C_RESTART_WITH_STOP
 * @return  data length/ERROR_ID
 */
static int i2c_multi_xfer(drv_l1_i2c_bus_handle_t *hd,  INT8U* subaddr, INT32U length, INT8U* data, INT32U data_len, INT8U cmd, INT32U mode)
{
    #define I2C_MULTI_WRITE_MODE        1
    #define I2C_MULTI_READ_MODE         1
	INT32S i=0, ret = -1;
    i2cReg_t *pI2CDev = _i2c_select_sfr_base(hd->devNumber);

	pI2CDev->MISC |= 0x1;
	if (hd->clkRate == 0) {
		DRV_I2C_DBG("[%s]-- Error: i2c clock rate must to be more than zero\r\n", __func__);
		goto __exit;
	}

	if (cmd == I2C_BUS_WRITE) {
#if I2C_MULTI_WRITE_MODE == 1
        ret = _i2cStartTran(pI2CDev, hd->slaveAddr, hd->clkRate, I2C_BUS_WRITE, 1);
#else
		ret = _i2cStartTran(pI2CDev, hd->slaveAddr, hd->clkRate, I2C_BUS_WRITE, 0);
#endif
		if (ret < 0) {
			DRV_I2C_DBG("WRITE-Error: write slave device address fail\r\n", __func__);
			goto __exit;
		}

		for (i = 0; i < length; i++)
		{
#if I2C_MULTI_WRITE_MODE == 1
			if (i==(length-1)) {
				ret = _i2cMiddleTran(pI2CDev, (subaddr+i), I2C_BUS_WRITE, 0);
			}
            else {
				ret = _i2cMiddleTran(pI2CDev, (subaddr+i), I2C_BUS_WRITE, 1);
			}
#else
            ret = _i2cMiddleTran(pI2CDev, (subaddr+i), I2C_BUS_WRITE, 0);
#endif

			if (ret < 0) {
				DRV_I2C_DBG("WRITE-Error: write subaddr fail\r\n", __func__);
				goto __exit;
			}
		}

		for (i = 0; i < data_len; i++)
		{
#if I2C_MULTI_WRITE_MODE == 1
			if (i==(data_len-1)) {
				ret = _i2cMiddleTran(pI2CDev, (data+i), I2C_BUS_WRITE, 0);
			}
            else {
				ret = _i2cMiddleTran(pI2CDev, (data+i), I2C_BUS_WRITE, 1);
			}
#else
            ret = _i2cMiddleTran(pI2CDev, (data+i), I2C_BUS_WRITE, 0);
#endif
			if (ret < 0) {
				DRV_I2C_DBG("WRITE-Error: write data fail\r\n", __func__);
				goto __exit;
			}
		}

		_i2cStopTran(pI2CDev, I2C_BUS_WRITE);
	}
	else if (cmd == I2C_BUS_READ) {
		ret = _i2cStartTran(pI2CDev, hd->slaveAddr, hd->clkRate, I2C_BUS_WRITE, 1);
		if (ret < 0) {
			DRV_I2C_DBG("READ-Error: write slave device address fail\r\n", __func__);
			goto __exit;
		}

		for (i = 0; i < length; i++)
		{
#if I2C_MULTI_READ_MODE == 1
			if (i == (length-1)) {
				ret = _i2cMiddleTran(pI2CDev, (subaddr+i), I2C_BUS_WRITE, 0);
			}
            else {
				ret = _i2cMiddleTran(pI2CDev, (subaddr+i), I2C_BUS_WRITE, 1);
			}
#else
            ret = _i2cMiddleTran(pI2CDev, (subaddr+i), I2C_BUS_WRITE, 0);
#endif
			if (ret < 0) {
				DRV_I2C_DBG("READ-Error: read subaddr fail\r\n", __func__);
				goto __exit;
			}
		}

		if(mode == I2C_RESTART_WITHOUT_STOP) {
			ret = _i2cBusRestartTran(pI2CDev, hd->slaveAddr);
			if (ret < 0) {
				DRV_I2C_DBG("ERROR: restart signal generate fail\n");
				goto __exit;
			}
		} else {
			_i2cStopTran(pI2CDev, I2C_BUS_WRITE);

			ret = _i2cStartTran(pI2CDev, hd->slaveAddr, hd->clkRate, I2C_BUS_READ, 1);
			if (ret < 0) {
				DRV_I2C_DBG("ERROR: read slave device address fail\n");
				goto __exit;
			}
		}

		for (i = 0; i < data_len; i++)
		{
#if I2C_MULTI_READ_MODE == 1
			if (i == (data_len-1)) {
				ret = _i2cMiddleTran(pI2CDev, (data+i), I2C_BUS_READ, 0);
			}
            else {
				ret = _i2cMiddleTran(pI2CDev, (data+i), I2C_BUS_READ, 1);
			}
#else
            ret = _i2cMiddleTran(pI2CDev, (data+i), I2C_BUS_READ, 0);
#endif
			if (ret < 0) {
				DRV_I2C_DBG("READ-Error: read data fail\r\n", __func__);
				goto __exit;
			}
		}

		_i2cStopTran(pI2CDev, I2C_BUS_READ);
	}

	ret = i;
__exit:
    pI2CDev->MISC &= ~0x1;

	return ret;
}

/*****************************************
*			I2C APIs    				 *
*****************************************/
/**
 * @brief   driver layer 1 I2C initilization
 * @param   DevNo: I2C_0/I2C_1/I2C_2
 * @return 	none
 */
void drv_l1_i2c_init(INT32U DevNo)
{
    i2cReg_t *pI2CDev = _i2c_select_sfr_base(DevNo);
	i2c_clock_set(DevNo,I2C_CLOCK_ENABLE);
	pI2CDev->MISC = MASK_I2C_MISC_GPIOEN;
	pI2CDev->ICCR = I2C_ICCR_INIT;
	pI2CDev->IDEBCLK = I2C_IDEBCLK_INIT;
}

/**
 * @brief   driver layer 1 I2C un-initilization
 * @param   DevNo: I2C_0/I2C_1/I2C_2
 * @return 	none
 */
void drv_l1_i2c_uninit(INT32U DevNo)
{
    i2cReg_t *pI2CDev = _i2c_select_sfr_base(DevNo);

	pI2CDev->MISC &= ~MASK_I2C_MISC_GPIOEN;
	i2c_clock_set(DevNo,I2C_CLOCK_DISABLE);
}

/**
 * @brief   I2C bus write function
 * @param   handle: hanle for I2C clock and slave address
 * @param   data: data want to write
 * @param   len: data length
 * @return 	0(successful) or -1(failed)
 */
INT32S drv_l1_i2c_bus_write(drv_l1_i2c_bus_handle_t *handle, INT8U* data, INT8U len)
{
	return _i2c_bus_xfer(handle, data, len, (INT8U)I2C_BUS_WRITE);
}
/**
 * @brief   I2C bus read function
 * @param   handle: hanle for I2C clock and slave address
 * @param   data: the buffer where I2C write the read data into
 * @param   len: data length
 * @return 	0(successful) or -1(failed)
 */
INT32S drv_l1_i2c_bus_read(drv_l1_i2c_bus_handle_t *handle, INT8U* data, INT16U len)
{
	return _i2c_bus_xfer(handle, data, len, (INT8U)I2C_BUS_READ);
}

/**
 * @brief   I2C bus write function with DMA
 * @param   handle: hanle for I2C clock and slave address
 * @param   wdata: the buffer where I2C data want to write
 * @param   rdata: the buffer where I2C write the data into
 * @param   len: total data length
 * @param   addr_len: address length
 * @param   data_len: data length
 * @return 	0(successful) or -1(failed)
 */
INT32S drv_l1_i2c_bus_write_dma(drv_l1_i2c_bus_handle_t *handle, INT8U* wdata, INT8U* rdata, INT8U len, INT8U addr_len, INT8U data_len)
{

	return _i2c_bus_xfer_dma(handle, wdata, rdata, len, addr_len, data_len, (INT8U)I2C_BUS_WRITE);
}

/**
 * @brief   I2C bus read function with DMA
 * @param   handle: hanle for I2C clock and slave address
 * @param   wdata: the buffer where I2C data want to write
 * @param   rdata: the buffer where I2C write the data into
 * @param   len: total data length
 * @param   addr_len: address length
 * @param   data_len: data length
 * @return 	0(successful) or -1(failed)
 */
INT32S drv_l1_i2c_bus_read_dma(drv_l1_i2c_bus_handle_t *handle, INT8U* wdata,INT8U* rdata, INT16U len, INT8U addr_len, INT16U data_len)
{
	return _i2c_bus_xfer_dma(handle, wdata, rdata, len, addr_len, data_len, (INT8U)I2C_BUS_READ);
}
/**
 * @brief   I2C bus write 1byte then write 1 bytes
 * @param   handle: hanle for I2C clock and slave address
 * @param   reg: the first data want to write
 * @param   value: the second data wnat to write
 * @return 	0(successful) or -1(failed)
 */

INT32S drv_l1_reg_1byte_data_1byte_write(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT8U value)
{
	INT8U data[2]={0};

	data[0] = reg & 0xFF;
	data[1] = value & 0xFF;
	DRV_I2C_DBG("[%s]-- addr=0x%02X, data=0x%02X\r\n", __func__, reg, value);

	return drv_l1_i2c_bus_write(handle, data, 2);
}

/**
 * @brief   I2C bus write 1byte then read 1 bytes
 * @param   handle: hanle for I2C clock and slave address
 * @param   reg: the data want to write
 * @param   value: the buffer where I2C write the read data into
 * @return 	0(successful) or -1(failed)
 */
INT32S drv_l1_reg_1byte_data_1byte_read(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT8U *value)
{
	INT32S ret=0;
	INT8U addr[1]={0}, data[1]={0};

	addr[0] = reg & 0xFF;

	ret = drv_l1_i2c_bus_write(handle, addr, 1);
	ret = drv_l1_i2c_bus_read(handle, data, 1);
	*value = data[0];
	DRV_I2C_DBG("[%s]-- addr=0x%02X, data=0x%02X\r\n", __func__, reg, data[0]);

	return ret;
}

/**
 * @brief   I2C bus write 1byte then write 2 bytes
 * @param   handle: hanle for I2C clock and slave address
 * @param   reg: the first data want to write
 * @param   value: contains 2 bytes data want to write
 * @return 	0(successful) or -1(failed)
 */
INT32S drv_l1_reg_1byte_data_2byte_write(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT16U value)
{
	INT8U data[3]={0};

	data[0] = reg & 0xFF;
	data[1] = (value >> 8) & 0xFF;
	data[2] = value & 0xFF;
	DRV_I2C_DBG("[%s]-- addr=0x%02X, data=0x%04X\r\n", __func__, reg, value);

	return drv_l1_i2c_bus_write(handle, data, 3);
}

/**
 * @brief   I2C bus write 1byte then read 2 bytes
 * @param   handle: hanle for I2C clock and slave address
 * @param   reg: the first data want to write
 * @param   value: the buffer where I2C write the read 2 bytes data into
 * @return 	0(successful) or -1(failed)
 */
INT32S drv_l1_reg_1byte_data_2byte_read(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT16U *value)
{
	INT32S ret=0;
	INT8U addr[1]={0}, data[2]={0};

	addr[0] = reg & 0xFF;

	ret = drv_l1_i2c_bus_write(handle, addr, 1);
	ret = drv_l1_i2c_bus_read(handle, data, 2);
	*value = (((INT16U)data[0]) << 8) | (data[1]);
	DRV_I2C_DBG("[%s]-- addr=0x%02X, data=0x%04X\r\n", __func__, reg, *value);

	return ret;
}

/**
 * @brief   I2C bus write 2byte then write 1 bytes
 * @param   handle: hanle for I2C clock and slave address
 * @param   reg: contains 2 bytes data want to write
 * @param   value: the third byte want to write
 * @return 	0(successful) or -1(failed)
 */
INT32S drv_l1_reg_2byte_data_1byte_write(drv_l1_i2c_bus_handle_t *handle, INT16U reg, INT8U value)
{
	INT8U data[3]={0};

	data[0] = (reg>>8) & 0xFF;
	data[1] = reg & 0xFF;
	data[2] = value & 0xFF;
	DRV_I2C_DBG("[%s]-- addr=0x%04X, data=0x%02X\r\n", __func__, reg, value);

	return drv_l1_i2c_bus_write(handle, data, 3);
}

/**
 * @brief   I2C bus write 2byte then read 1 bytes
 * @param   handle: hanle for I2C clock and slave address
 * @param   reg: contains 2 bytes data want to write
 * @param   value: the buffer where I2C write the read data into
 * @return 	0(successful) or -1(failed)
 */
INT32S drv_l1_reg_2byte_data_1byte_read(drv_l1_i2c_bus_handle_t *handle, INT16U reg, INT8U *value)
{
	INT32S ret=0;
	INT8U addr[2]={0}, data[1]={0};

	addr[0] = (reg>>8) & 0xFF;
	addr[1] = reg & 0xFF;

	ret = drv_l1_i2c_bus_write(handle, addr, 2);
	ret = drv_l1_i2c_bus_read(handle, data, 1);
	*value = data[0];
	DRV_I2C_DBG("[%s]-- addr=0x%04X, data=0x%02X\r\n", __func__, reg, *value);

	return ret;
}

/**
 * @brief   I2C bus write function
 * @param   handle[in]: i2c bus handle
 * @param   subaddr[in]: sub-address buffer
 * @param   length[in]: sub-address length
 * @param   data[in]: data buffer
 * @param	data_len[in]: data length
 * @param	mode[in]:i2c restart without stop or not: 1、I2C_RESTART_WITHOUT_STOP 0、I2C_RESTART_WITH_STOP
 * @return  data length/ERROR_ID
 */
INT32S drv_l1_i2c_multi_write(drv_l1_i2c_bus_handle_t *handle, INT8U *subaddr, INT32U length, INT8U *data, INT32U data_len, INT32U mode)
{
	return	i2c_multi_xfer(handle, subaddr, length, data, data_len, I2C_BUS_WRITE, mode);
}

/**
 * @brief   I2C bus read function
 * @param   handle[in]: i2c bus handle
 * @param   subaddr[in]: sub-address buffer
 * @param   length[in]: sub-address length
 * @param   data[in]: data buffer
 * @param	data_len[in]: data length
 * @param	mode[in]:i2c restart without stop or not: 1、I2C_RESTART_WITHOUT_STOP 0、I2C_RESTART_WITH_STOP
 * @return  data length/ERROR_ID
 */
INT32S drv_l1_i2c_multi_read(drv_l1_i2c_bus_handle_t *handle, INT8U *subaddr, INT32U length, INT8U *data, INT32U data_len, INT32U mode)
{
	return	i2c_multi_xfer(handle, subaddr, length, data, data_len, I2C_BUS_READ, mode);
}

INT32S drv_l1_i2c_slave_init(void)
{

    if(i2c_rx_q == 0) {
		i2c_rx_q = osMessageCreate(&rx_q_def, NULL);
		if(i2c_rx_q == 0) {
			return STATUS_FAIL;
		}
	}

	return STATUS_OK;
}

INT32S drv_l1_i2c_slave_rx(drv_l1_i2c_bus_handle_t *handle, INT8U* rdata, INT32U length)
{
	i2cReg_t *pI2CDev = _i2c_select_sfr_base(handle->devNumber);
    DMA_STRUCT dma_rx={0};
    volatile INT8S done_rx;
    INT32U ctrl=0;
    INT32S status;
    osEvent result;

    done_rx = C_DMA_STATUS_WAITING;
    dma_rx.s_addr = (INT32U) &pI2CDev->RXDMA_IDSR;
    dma_rx.t_addr = (INT32U) rdata;
    dma_rx.count = length;
    dma_rx.width = DMA_DATA_WIDTH_1BYTE;
    dma_rx.notify = &done_rx;
    dma_rx.timeout = 0;
    //dma_rx.aes = 0;
    dma_rx.trigger = 0;

    status = drv_l1_dma_transfer_with_queue(&dma_rx,i2c_rx_q);
    if(status != 0) {
        print_string("fail!\r\n");
        return status;
    }
   while(1){
        if((pI2CDev->ICCR & (MASK_I2C_CR_DMA_INT)) >> 6 ){
            pI2CDev->ICCR |= MASK_I2C_CR_DMA_INT;
            ctrl |= I2C_ICSR_SLAVE_RX | MASK_I2C_CR_INTRPEND;
            pI2CDev->ICSR = ctrl;
            break;
        }
   }
    result = osMessageGet(i2c_rx_q, osWaitForever);
    if((result.status != osEventMessage) || (result.value.v != C_DMA_STATUS_DONE)) {
        print_string("DMA fail!!\r\n");
        return STATUS_FAIL;
    }

    return 0;
}

INT32S drv_l1_i2c_slave_tx(drv_l1_i2c_bus_handle_t *handle, INT8U* wdata, INT32U length)
{
	i2cReg_t *pI2CDev = _i2c_select_sfr_base(handle->devNumber);
    DMA_STRUCT dma_tx={0};
    volatile INT8S  done_tx;
    INT32S status;

    done_tx = C_DMA_STATUS_WAITING;
    dma_tx.s_addr = (INT32U) wdata;
    dma_tx.t_addr = (INT32U) &pI2CDev->TXDMA_IDSR;
    dma_tx.count = length;

    dma_tx.width = DMA_DATA_WIDTH_1BYTE;
    dma_tx.notify = &done_tx;
    dma_tx.timeout = 0;;
    //dma_tx.aes = 0;
    dma_tx.trigger = 0;

    status = drv_l1_dma_transfer(&dma_tx);
    if(status != 0) {
        print_string("fail!\r\n");
        return status;
    }

    print_string("drv_l1_i2c_slave_tx finish\r\n");
    return 0;
}

void gp_i2c_dump_reg(void)
{
	int				i;
	volatile INT32U *p_sram_hw;

	DBG_PRINT("\r\n");
	DBG_PRINT("%s\r\n", __FUNCTION__);
    //i2cReg_t *pI2CDev = _i2c_select_sfr_base(hd->devNumber);
	p_sram_hw = (INT32U *) I2C1_BASE;

	for(i = 0; i < 16; i++)
	{
		DBG_PRINT("R0x%8x: 0x%8x\r\n", p_sram_hw, *p_sram_hw);
		p_sram_hw++;
	}
	vTaskDelay(100);
}
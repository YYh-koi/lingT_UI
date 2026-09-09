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
#ifndef __drv_l1_I2C_H__
#define __drv_l1_I2C_H__

/****************************************************
*		Register bitwise definition 				*
****************************************************/
/********************* Define R_I2C_ICCR bit mask (0xC00B0000, I2C control register) *****************/
#define MASK_I2C_CR_ACKEN		(1 << 7)		/* I2C-bus acknowledge enable */
#define MASK_I2C_CR_DMA_INT 	(1 << 6)		/* I2C-bus TX/RX interrupt flag for dma mode*/
#define MASK_I2C_CR_INTREN		(1 << 5)		/* I2C-bus Tx/Rx interrupt enable */
#define MASK_I2C_CR_INTRPEND	(1 << 4)		/* I2C-bus Tx/Rx interrupt pending flag */

/********************* Define R_I2C_ICSR bit mask (0xC00B0004, I2C control/status register) *****************/
#define MASK_I2C_SR_BUSYSTA		(1 << 5)		/* I2C-bus busy signal status bit, when write 1: START signal generation, 0: STOP signal generation */
#define MASK_I2C_SR_DOEN		(1 << 4)		/* I2C-bus data output enable/disable bit */
#define MASK_I2C_SR_APBIT		(1 << 3)		/* I2C-bus arbitration procedure status flag bit */
#define MASK_I2C_SR_ASBIT		(1 << 2)		/* I2C-bus address as slave status flag bit */
#define MASK_I2C_SR_AZBIT		(1 << 1)		/* I2C-bus address zero status flag bit */
#define MASK_I2C_SR_LRBIT		(1 << 0)		/* I2C-bus last received bit status flag bit. 0: ACK received, 1: ACK not received */

/********************* Define R_I2C_MISC bit mask (0xC00B0018, I2C miscellaneous control register) *****************/
#define MASK_I2C_MISC_MASTER_RX (1 << 7)		/* For DMA mode auto transmission, I2c operate as Master Read*/
#define MASK_I2C_MISC_MASTER_TX (1 << 6)		/* For DMA mode auto transmission, I2c operate as Master Write*/
#define MASK_I2C_MISC_DMA_MODE  (1 << 4)		/* For DMA mode auto transmission,I2c DMA MODE Enable */
#define MASK_I2C_MISC_Stop   	(1 << 2)		/* I2C-bus interface STOP signal generation (when write) */
#define MASK_I2C_MISC_IGNOREACK	(1 << 1)		/* I2C-bus ignore ACK control bit */
#define MASK_I2C_MISC_GPIOEN	(1 << 0)		/* I2C-bus output via GPIO enable */

/****************************************************
*	Definition	 									*
****************************************************/
#define I2C_TIME_OUT		10	//ms
#define	NO_ACK_TIMEOUT		10	//ms

#define I2C_BUS_WRITE	0
#define I2C_BUS_READ	1

#define I2C_ICCR_INIT				0x50
#define I2C_IDEBCLK_INIT			0x04
#define I2C_ICCR_TXCLKMSB_MASK		0x0F

#define I2C_ICSR_MASTER_TX			0xC0
#define I2C_ICSR_MASTER_RX			0x80
#define I2C_ICSR_SLAVE_TX			0x40
#define I2C_ICSR_SLAVE_RX			0x00

#define I2C_MISC_PINMUX_EN			0x01
#define I2C_MISC_ACK_DONTCARE		0x02

typedef enum
{
    I2C_0,
	I2C_1,
	I2C_2,
    I2C_MAX_NUMS
} I2C_DEVICE;

/****************************************************
*		Data structure 								*
*****************************************************/
typedef struct drv_l1_i2c_bus_handle_s
{
	INT32U		devNumber;
	INT16U      slaveAddr;			/* slave device address */
	INT16U      clkRate;			/* i2c bus clock rate */
} drv_l1_i2c_bus_handle_t;

typedef struct i2cReg_s
{
	volatile INT32U	ICCR;		//0xC00B0000
	volatile INT32U	ICSR;	    //0xC00B0004
	volatile INT32U	IAR;	    //0xC00B0008
	volatile INT32U	IDSR;	    //0xC00B000C
	volatile INT32U	IDEBCLK;	//0xC00B0010
	volatile INT32U	TXCLKLSB;	//0xC00B0014
    volatile INT32U	MISC;       //0xC00B0018
    volatile INT32U	TXDMA_IDSR; //0xC00B001C
    volatile INT32U	RXDMA_IDSR; //0xC00B0020
    volatile INT32U	DMA_DID;    //0xC00B0024
    volatile INT32U	STOP_CR;    //0xC00B0028
    volatile INT32U	WAIT_CR;    //0xC00B002C
    volatile INT32U	ADDR_CR;    //0xC00B0030
    volatile INT32U	DATA_CR;    //0xC00B0034
    volatile INT32U	RXDATA_CR;  //0xC00B0038
    volatile INT32U	ID_STATUS;  //0xC00C003C
} i2cReg_t;

typedef struct i2c_dmaReg_s
{
	volatile INT32U	CTRL;		//0xD0300000
	volatile INT32U	SRC_ADDR;	//0xD0300004
	volatile INT32U	TAR_ADDR;	//0xD0300008
	volatile INT32U	TX_CNT;	    //0xD030000C
} i2c_dmaReg_t;

/****************************************************
*		external function declarations				*
****************************************************/
extern void drv_l1_i2c_init(INT32U DevNo);
extern INT32S drv_l1_i2c_slave_init(void);
extern void drv_l1_i2c_uninit(INT32U DevNo);
extern INT16S i2c_clock_set(INT32U dev,INT16U enable);
extern INT32S drv_l1_i2c_bus_write(drv_l1_i2c_bus_handle_t *handle, INT8U* data, INT8U len);
extern INT32S drv_l1_i2c_bus_read(drv_l1_i2c_bus_handle_t *handle, INT8U* data, INT16U len);
extern INT32S drv_l1_i2c_bus_write_dma(drv_l1_i2c_bus_handle_t *handle, INT8U* wdata, INT8U* rdata, INT8U len, INT8U addr_len, INT8U data_len);
extern INT32S drv_l1_i2c_bus_read_dma(drv_l1_i2c_bus_handle_t *handle, INT8U* wdata,INT8U* rdata, INT16U len, INT8U addr_len, INT16U data_len);
extern INT32S drv_l1_reg_1byte_data_1byte_write(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT8U value);
extern INT32S drv_l1_reg_1byte_data_1byte_read(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT8U *value);
extern INT32S drv_l1_reg_1byte_data_2byte_write(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT16U value);
extern INT32S drv_l1_reg_1byte_data_2byte_read(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT16U *value);
extern INT32S drv_l1_i2c_multi_write(drv_l1_i2c_bus_handle_t *handle, INT8U *subaddr, INT32U length, INT8U *data, INT32U data_len, INT32U mode);
extern INT32S drv_l1_i2c_multi_read(drv_l1_i2c_bus_handle_t *handle, INT8U *subaddr, INT32U length, INT8U *data, INT32U data_len, INT32U mode);
extern i2c_dmaReg_t* _i2c_select_dma_base(INT32U channel);
extern INT32S _i2c_intpend_waiting_dma(i2cReg_t *pI2CDev, INT32S ms);
extern INT32S drv_l1_i2c_slave_rx(drv_l1_i2c_bus_handle_t *handle, INT8U* rdata, INT32U length);
extern INT32S drv_l1_i2c_slave_tx(drv_l1_i2c_bus_handle_t *handle, INT8U* wdata, INT32U length);
#endif	/*__drv_l1_I2C_H__*/

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
#include "drv_l1.h"

#define C_SPI_DMA_DUMMY_ADDR    0x40000000 // 0x02000000, winworm suggest 0x40000000

#define K_SPI_USE_CMSIS_OS      1
#define K_SPI_SUPPORT_SPINAND   1

#if (K_SPI_USE_CMSIS_OS == 1)
#include "cmsis_os.h"
#endif

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define SPI_CTRL_ENA		(1 << 15)	/* SPI enable */
#define SPI_CTRL_RX_ONLY_EN	(1 << 14)	/* dma RX only, no need to do TX when doing RX, GCV770 start support */
#define SPI_CTRL_LBM		(1 << 13)	/* loop back mode selection: SPIRX = SPITX */
#define SPI_CTRL_QPI		(1 << 12)   /* QPI mode enable. 1: 4 bit mode enable */
#define SPI_CTRL_RST		(1 << 11)	/* SPI software reset */
#define SPI_CTRL_QPIXD_OEB  (1 << 10)   /* QPI Data output enable.  0: TX, 1:RX */
#define SPI_CTRL_2IO_OEB	(1 <<  9)   /* 2 bit mode data output enable. 0: TX, 1:RX */
#define SPI_CTRL_SLVE		(1 <<  8)	/* SPI Slave Mode */

#define SPI_CTRL_CPHA		(1 <<  5)	/* SPI Clock Phase */
#define SPI_CTRL_CPOL		(1 <<  4)	/* SPI Clock Polarity */
#define SPI_CTRL_2IO        (1 <<  3)   /* 2 bit mode enable. 1: 2 bit mode enable */
#define SPI_CTRL_CLK		(7 <<  0)	/* SPI master mode clock selection */

#define SPI_TX_STATUS_IF	(1 << 15)	/* SPI transmit interrupt flag */
#define SPI_RX_STATUS_IF	(1 << 15)	/* SPI receive interrupt flag */
#define SPI_TX_STATUS_IEN	(1 << 14)	/* SPI transmit interrupt enable */
#define SPI_RX_STATUS_IEN	(1 << 14)	/* SPI receive interrupt enable */

#define SPI_TX_STATUS_LEVEL	(0xF << 4)  /* SPI transmit FIFO interrupt level */
#define SPI_RX_STATUS_LEVEL	(0xF << 4)  /* SPI receiver FIFO interrupt level */
#define SPI_RX_STATUS_RXFLAG (7 << 0)

#define SPI_MISC_SMART		(1 << 8)  	/* SPI FIFO smart mode */
#define SPI_MISC_RFF		(1 <<  3)  	/* Rcecive FIFO is not empty */
#define SPI_MISC_RNE		(1 <<  2)  	/* Rcecive FIFO is not empty */
#define SPI_MISC_TNF		(1 <<  1)  	/* Transmit FIFO is not full */
#define SPI_MISC_TFE		(1 <<  0)  	/* Transmit FIFO Empty Flag */

#define SPI_RD_LATCH_DLY_0  (1 <<  6)
#define SPI_RD_LATCH_DLY_1  (1 <<  7)


#define SPI_TIMEOUT_VALUE	10000
#define SPI_TRANS_INT		0
#define SPI_QUEUE_MAX		1

#define SPI_CLOCK_SOURCE            24
#define SPI_CLOCK_ENABLE            1
#define SPI_CLOCK_DISABLE           0

#define SPI_INTERRUPT               0
#define SPI_FIQ		                0
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define CLEAR(ptr, cblen)		memset((void *)ptr, 0x00, cblen)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
    SPI_DISABLED,
    SPI_ENABLED
} SPI_FLAG;

typedef struct
{
	INT8U 	spi_usage;
	INT8U   cs_by_external;
	INT8U 	cs_gpio_pin;
	INT8U   cs_active;

	INT8U   tx_int_level;
	INT8U   rx_int_level;
	INT8S	slave_dma_en;
	INT8S	reserved;

	INT8U 	*tx_buf;
	INT8U 	*rx_buf;

	INT32U  len;
	INT32U  rcv_len;
	INT32U  tx_count;
	INT32U  rx_count;

#if (_OPERATING_SYSTEM != _OS_NONE)
#if (K_SPI_USE_CMSIS_OS == 1)
	osMessageQId spi_dma_txq;
	osMessageQId spi_dma_rxq;
#else
    xQueueHandle spi_dma_txq;
    xQueueHandle spi_dma_rxq;
#endif
#endif

} SPI_MASTER;

typedef struct spiReg_s
{
	volatile INT32U	CTRL;		//0xC0090000
	volatile INT32U	TX_STATUS;	//0xC0090004
	volatile INT32U	TX_DATA;	//0xC0090008
	volatile INT32U	RX_STATUS;	//0xC009000C
	volatile INT32U	RX_DATA;	//0xC0090010
	volatile INT32U	MISC;		//0xC0090014
	volatile INT32U RESERVE;    //0xC0090018
	volatile INT32U DMA_RX_CYC; //0xC009001C
} spiReg_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static spiReg_t* spi_select(INT8U spi_num);
static SPI_MASTER* spi_workmem_get(INT8U spi_num);
static INT32S spi_usage_get(SPI_MASTER *pSpi);
static void spi_usage_free(SPI_MASTER *pSpi);
static void spi_reg_clear(spiReg_t *pSpiReg);
static void spi_tx_int_set(spiReg_t *pSpiReg, INT8S tx_int);
static void spi_rx_int_set(spiReg_t *pSpiReg, INT8S tx_int);
static INT32S spi_transceive_dma(INT8U spi_num);
static INT32S spi_transceive_dma_2(INT8U spi_num); 		// use by spi nand
static INT32S spi_transceive_polling(INT8U spi_num);
static INT32S spi_transceive_polling_2(INT8U spi_num); 	// use by spi nand

static void spi_allocate_dma_sema(INT8U spi_num);
static void spi_0_dma_tx_user_isr(INT8U dma_channel, INT8U dma_result);
static void spi_1_dma_tx_user_isr(INT8U dma_channel, INT8U dma_result);
static void spi_0_dma_rx_user_isr(INT8U dma_channel, INT8U dma_result);
static void spi_1_dma_rx_user_isr(INT8U dma_channel, INT8U dma_result);
void spi_fasp_apb_set(INT8U spi_num);
void spi_fasp_apb_clear(INT8U spi_num);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
SPI_MASTER spi_master[SPI_MAX_NUMS];
static void (*spi_dma_tx_user_isr[SPI_MAX_NUMS])(INT8U, INT8U) = {spi_0_dma_tx_user_isr, spi_1_dma_tx_user_isr};
static void (*spi_dma_rx_user_isr[SPI_MAX_NUMS])(INT8U, INT8U) = {spi_0_dma_rx_user_isr, spi_1_dma_rx_user_isr};
static void *spi_dma_tx_done_sema[SPI_MAX_NUMS] = {NULL, NULL};
static void *spi_dma_rx_done_sema[SPI_MAX_NUMS] = {NULL, NULL};
static volatile INT8U spi_dma_tx_result[SPI_MAX_NUMS];
static volatile INT8U spi_dma_rx_result[SPI_MAX_NUMS];
static INT32U spi_tx_clock_rate[SPI_MAX_NUMS] = {12000000, 12000000};
static INT32U spi_rx_clock_rate[SPI_MAX_NUMS] = {12000000, 12000000};
static INT8U spi_rx_edge_polar[SPI_MAX_NUMS] = {SPI_RX_EDGE_FALLING, SPI_RX_EDGE_FALLING};

/**
 * @brief   Start spi OS mode
 * @param   none
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
static INT8U spi_is_os_started(void)
{
#if _OPERATING_SYSTEM != _OS_NONE
    #if _OPERATING_SYSTEM == _OS_UCOS2
    if (OSRunning)
        return 1;
    else
        return 0;
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
    if (xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)
        return 1;
    else
        return 0;
    #endif
#else
    return 0;
#endif
}

/**
 * @brief   Create spi dma Semaphore
 * @param   spi_num: spi device number
 * @return 	none
 */
static void spi_allocate_dma_sema(INT8U spi_num)
{
	if (spi_is_os_started())
	{
	#if _OPERATING_SYSTEM != _OS_NONE
		#if _OPERATING_SYSTEM == _OS_UCOS2
		#elif _OPERATING_SYSTEM == _OS_FREERTOS
		if (spi_dma_tx_done_sema[spi_num] == NULL)
			spi_dma_tx_done_sema[spi_num] = xSemaphoreCreateBinary();
		if (spi_dma_rx_done_sema[spi_num] == NULL)
			spi_dma_rx_done_sema[spi_num] = xSemaphoreCreateBinary();
		#endif
	#endif
	}
}

/**
 * @brief   Release spi dma Semaphore
 * @param   spi_dma_done_sema: Semaphore
 * @return 	none
 */
static void spi_dma_done(void *spi_dma_done_sema)
{
	if ((spi_dma_done_sema != NULL)) {
		#if _OPERATING_SYSTEM != _OS_NONE
			#if _OPERATING_SYSTEM == _OS_UCOS2
			#elif _OPERATING_SYSTEM == _OS_FREERTOS
			BaseType_t xResult;
			BaseType_t xHigherPriorityTaskWoken = 0;

			xResult = xSemaphoreGiveFromISR(spi_dma_done_sema, &xHigherPriorityTaskWoken);
			if (xResult != pdFALSE) {
				portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			}
			else {
				DBG_PRINT("xSemaphoreGiveFromISR is pdFALSE. %u\r\n", xHigherPriorityTaskWoken);
			}
			#endif
		#endif
	}
}

/**
 * @brief   spi0 dma tx isr
 * @param   dma_channel: dma channel
 * @param   dma_result: result
 * @return 	none
 */
static void spi_0_dma_tx_user_isr(INT8U dma_channel, INT8U dma_result)
{
	spi_dma_tx_result[SPI_0] = dma_result;
	spi_dma_done(spi_dma_tx_done_sema[SPI_0]);
}

/**
 * @brief   spi1 dma tx isr
 * @param   dma_channel: dma channel
 * @param   dma_result: result
 * @return 	none
 */
static void spi_1_dma_tx_user_isr(INT8U dma_channel, INT8U dma_result)
{
	spi_dma_tx_result[SPI_1] = dma_result;
	spi_dma_done(spi_dma_tx_done_sema[SPI_1]);
}

/**
 * @brief   spi0 dma rx isr
 * @param   dma_channel: dma channel
 * @param   dma_result: result
 * @return 	none
 */
static void spi_0_dma_rx_user_isr(INT8U dma_channel, INT8U dma_result)
{
	spi_dma_rx_result[SPI_0] = dma_result;
	spi_dma_done(spi_dma_rx_done_sema[SPI_0]);
}

/**
 * @brief   spi1 dma rx isr
 * @param   dma_channel: dma channel
 * @param   dma_result: result
 * @return 	none
 */
static void spi_1_dma_rx_user_isr(INT8U dma_channel, INT8U dma_result)
{
	spi_dma_rx_result[SPI_1] = dma_result;
	spi_dma_done(spi_dma_rx_done_sema[SPI_1]);
}

/**
 * @brief   set spi clock
 * @param   enable: enable / disable
 * @return 	STATUS_OK
 */
static INT16S drv_l1_spi_clock_set(INT8U spi_num,INT16U enable)
{
    if(spi_num == SPI_0)
        return drv_l1_clock_set_system_clk_en(CLK_EN2_SPI0, enable);
    else if(spi_num == SPI_1)
        return drv_l1_clock_set_system_clk_en(CLK_EN2_SPI1, enable);
    else
        return STATUS_FAIL;
}

/**
 * @brief   select spi CTRL register
 * @param   spi_num: SPI_0/SPI_1
 * @return 	P_SPI0_CTRL/P_SPI1_CTRL
 */
static spiReg_t* spi_select(INT8U spi_num)
{
	if(spi_num == SPI_0) {
		return (spiReg_t*)P_SPI0_CTRL;
	}

#if SPI_NUM > 1
	if(spi_num == SPI_1) {
	 	return (spiReg_t*)P_SPI1_CTRL;
	}
#endif
	return 0;
}

/**
 * @brief   get spi varible
 * @param   spi_num: SPI_0/SPI_1
 * @return 	spi_master[SPI_0]/spi_master[SPI_1]
 */
static SPI_MASTER* spi_workmem_get(INT8U spi_num)
{
	if(spi_num == SPI_0) {
		return (SPI_MASTER*)&spi_master[SPI_0];
	}

#if SPI_NUM > 1
	if(spi_num == SPI_1) {
		return (SPI_MASTER*)&spi_master[SPI_1];
	}
#endif
	return 0;
}

/**
 * @brief   spi delay
 * @param   msec: delay time
 * @return 	none
 */
static void spi_msec_wait(INT32U msec)
{
#if 0
	INT32U cnt = msec * (SystemCoreClock >> 13); //0.64ms
	INT32U i;

	for(i=0; i<cnt; i++) {
		i = i;
	}
#else
	#if (_OPERATING_SYSTEM != _OS_NONE)
        #if (K_SPI_USE_CMSIS_OS == 1)
        osDelay(msec);
        #else
        vTaskDelay(msec);
        #endif
	#else
        drv_msec_wait(msec);
	#endif
#endif
}

/**
 * @brief   get spi usage
 * @param   pSpi: master varible
 * @return 	result:SPI_BUSY/STATUS_OK
 */
static INT32S spi_usage_get(SPI_MASTER *pSpi)
{
	INT32S ret;

#if (_OPERATING_SYSTEM != _OS_NONE)
    #if (K_SPI_USE_CMSIS_OS == 1)
        osSuspend();
    #else
        vTaskSuspendAll();
    #endif
#endif

	if(pSpi->spi_usage == 1) {
		ret = SPI_BUSY;
	} else {
		pSpi->spi_usage = 1;
		ret = STATUS_OK;
	}

#if (_OPERATING_SYSTEM != _OS_NONE)
    #if (K_SPI_USE_CMSIS_OS == 1)
        osResume();
    #else
        xTaskResumeAll();
    #endif
#endif

	return ret;
}

/**
 * @brief   free spi usage
 * @param   pSpi: master varible
 * @return 	none
 */
static void spi_usage_free(SPI_MASTER *pSpi)
{
#if (_OPERATING_SYSTEM != _OS_NONE)
    #if (K_SPI_USE_CMSIS_OS == 1)
        osSuspend();
    #else
        vTaskSuspendAll();
    #endif
#endif

	pSpi->spi_usage = 0;

#if (_OPERATING_SYSTEM != _OS_NONE)
    #if (K_SPI_USE_CMSIS_OS == 1)
        osResume();
    #else
        xTaskResumeAll();
    #endif
#endif
}

/**
 * @brief   clear spi register
 * @param   pSpiReg: spi register
 * @return 	none
 */
static void spi_reg_clear(spiReg_t *pSpiReg)
{
	pSpiReg->CTRL &= 0xC0; // keep the sample delay bit 6, 7
	pSpiReg->TX_STATUS = 0;
	pSpiReg->RX_STATUS = 0;
	pSpiReg->MISC = 0;

	/* reset the SPI */
	pSpiReg->CTRL |= SPI_CTRL_RST;

	/* clear interrupt flag */
	pSpiReg->TX_STATUS |= SPI_TX_STATUS_IF;
	pSpiReg->RX_STATUS |= SPI_RX_STATUS_IF;
}

/**
 * @brief   set SPI transmit interrupt enable
 * @param   pSpiReg: spi register
 * @param   tx_int: enable/disable
 * @return 	none
 */
static void spi_tx_int_set(spiReg_t *pSpiReg, INT8S tx_int)
{
	if(tx_int == SPI_ENABLED) {
		pSpiReg->TX_STATUS |= SPI_TX_STATUS_IEN;
	} else {
		pSpiReg->TX_STATUS &= ~SPI_TX_STATUS_IEN;
	}
}

/**
 * @brief   set SPI receive interrupt enable
 * @param   pSpiReg: spi register
 * @param   rx_int: enable/disable
 * @return 	none
 */
static void spi_rx_int_set(spiReg_t *pSpiReg, INT8S rx_int)
{
	if(rx_int == SPI_ENABLED) {
		pSpiReg->RX_STATUS |= SPI_RX_STATUS_IEN;
	} else {
		pSpiReg->RX_STATUS &= ~SPI_RX_STATUS_IEN;
	}
}

#if DEBUG == 1
static INT32U spi_rx_interrupt_count = 0;
static INT32U spi_tx_interrupt_count = 0;
#endif

/**
 * @brief   spi isr
 * @param   pSpiReg: spi register
 * @param   pSpi: master varible
 * @return 	STATUS_OK
 */
static INT32S handle_spi_isr(spiReg_t *pSpiReg, SPI_MASTER *pSpi)
{
	INT8U  dummy;
    INT32U tx_status = 0;
    INT32U rx_status = 0;
    INT32U i = 0;

 	tx_status = pSpiReg->TX_STATUS;
    rx_status = pSpiReg->RX_STATUS;

	if (rx_status & SPI_RX_STATUS_IF) {
		pSpiReg->RX_STATUS |= SPI_RX_STATUS_IF; /* clear interrupt flag */
		while(pSpiReg->MISC & SPI_MISC_RNE){
			/* receive */
			if (pSpi->rx_count < pSpi->len) {
				if (pSpi->rx_count < pSpi->rcv_len) {
					pSpi->rx_buf[pSpi->rx_count++] = (INT8U)pSpiReg->RX_DATA;
				} else {
					dummy = (INT8U)pSpiReg->RX_DATA;
					if(dummy){};	//remove compiler warning
					pSpi->rx_count++;
				}
			} else {
				dummy = (INT8U)pSpiReg->RX_DATA;
			}
		}

		#if DEBUG == 1
		spi_rx_interrupt_count++;
		if (spi_rx_interrupt_count <= 100)
            DBG_PRINT("spi_rx_interrupt_count %u, RX_STATUS 0x%x\r\n", spi_rx_interrupt_count, pSpiReg->RX_STATUS);
        #endif
	}

    if (tx_status & SPI_TX_STATUS_IF) {
    	pSpiReg->TX_STATUS |= SPI_TX_STATUS_IF; /* clear interrupt flag */

    	/* start transmition */
    	for (i=0;(i<8-pSpi->tx_int_level)&&(pSpi->len > pSpi->tx_count);i++) {
			if ((pSpiReg->MISC & SPI_MISC_TNF)) {
				pSpiReg->TX_DATA = (INT32U)pSpi->tx_buf[pSpi->tx_count++];
			}
		}

		if (pSpi->tx_count >= pSpi->len) {
			spi_tx_int_set(pSpiReg, SPI_DISABLED);
		}

		#if DEBUG == 1
		spi_tx_interrupt_count++;
		if (spi_tx_interrupt_count <= 100)
            DBG_PRINT("spi_tx_interrupt_count %u, TX_STATUS 0x%x\r\n", spi_tx_interrupt_count, pSpiReg->TX_STATUS);
        #endif
	}

	return 0;
}

/**
 * @brief   spi0 IRQHandler
 * @param   none
 * @return 	none
 */
void SPI0_IRQHandler(void)
{
	SPI_MASTER *pSpi = spi_workmem_get(SPI_0);
	spiReg_t *pSpiReg = spi_select(SPI_0);

	handle_spi_isr(pSpiReg, pSpi);
}

#if SPI_NUM > 1
/**
 * @brief   spi1 IRQHandler
 * @param   none
 * @return 	none
 */
void SPI1_IRQHandler(void)
{
	SPI_MASTER *pSpi = spi_workmem_get(SPI_1);
	spiReg_t *pSpiReg = spi_select(SPI_1);

	handle_spi_isr(pSpiReg, pSpi);
}
#endif

/**
 * @brief   spi transmit data by dma
 * @param   spi_num: spi device number
 * @return 	ret: STATUS_FAIL/STATUS_OK
 */
static INT32S spi_transceive_dma(INT8U spi_num)
{
	volatile INT8S done_rx;
	volatile INT8S done_tx;
	INT32S status;
	INT8U clk_divisor;

#if (_OPERATING_SYSTEM != _OS_NONE)
    osEvent result;
#endif
	DMA_STRUCT dma_rx={0}, dma_tx={0};
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num);
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}

	/* reset spi control */
	pSpiReg->CTRL |= SPI_CTRL_RST;
	spi_tx_int_set(pSpiReg, SPI_DISABLED);
	spi_rx_int_set(pSpiReg, SPI_DISABLED);
	drv_l1_spi_txrx_level_set(spi_num, 0x7, 0);
	drv_l1_spi_clk_divisor_get(spi_num, &clk_divisor);
	if(clk_divisor == 0)
        spi_fasp_apb_set(spi_num);
    else
        spi_fasp_apb_clear(spi_num);
	//RX setting
    if(pSpi->rcv_len){

    if (spi_rx_edge_polar[spi_num] == SPI_RX_EDGE_RISING)
        //read data from SPI device pol = 0, pha = 1. RisingEdge Master Get Data
        drv_l1_spi_pha_pol_set(spi_num, PHA0_POL0);
    else
        //read data from SPI device pol = 0, pha = 1. FallingEdge Master Get Data
		drv_l1_spi_pha_pol_set(spi_num, PHA1_POL0);

	done_rx = C_DMA_STATUS_WAITING;
	dma_rx.s_addr = (INT32U) &pSpiReg->RX_DATA;
	if(pSpi->rcv_len == 0) {
		dma_rx.t_addr = C_SPI_DMA_DUMMY_ADDR; /* reserved memory */
	} else {
		dma_rx.t_addr = (INT32U) pSpi->rx_buf;
		//DBG_PRINT("spi rx buf=0x%08x len=%u\r\n", (INT32U) pSpi->rx_buf, pSpi->rcv_len);
	}

    dma_rx.count = pSpi->rcv_len;
	dma_rx.width = DMA_DATA_WIDTH_1BYTE;
	dma_rx.notify = &done_rx;
	dma_rx.timeout = 0;//255;
	//dma_rx.aes = 0;
	dma_rx.trigger = 0;
	if((pSpiReg->MISC & 0x400)>>10)
        dma_rx.fast_apb = 1;
#if SPI_RX_DMA_ONLY == 1
		pSpiReg->DMA_RX_CYC = pSpi->rcv_len;
		pSpiReg->CTRL |= SPI_CTRL_RX_ONLY_EN;
#endif

#if (_OPERATING_SYSTEM != _OS_NONE)
        OSQFlush(pSpi->spi_dma_rxq);
        status = drv_l1_dma_transfer_with_queue(&dma_rx, pSpi->spi_dma_rxq);
#else
        status = drv_l1_dma_transfer(&dma_rx);
#endif
        if(status != 0) {
            return status;
        }
    }
    // TX setting
    if(pSpi->len){
        drv_l1_spi_txrx_level_set(spi_num, 0x0, 0);
    //write data to SPI device: pol = 0, pha = 0. RisingEdge Master Send Data
		drv_l1_spi_pha_pol_set(spi_num, PHA0_POL0);

        done_tx = C_DMA_STATUS_WAITING;
        dma_tx.s_addr = (INT32U) pSpi->tx_buf;
        dma_tx.t_addr = (INT32U) &pSpiReg->TX_DATA;
        dma_tx.count = pSpi->len;
        dma_tx.width = DMA_DATA_WIDTH_1BYTE;
        dma_tx.notify = &done_tx;
        dma_tx.timeout = 0;//255;
        //dma_tx.aes = 0;
        dma_tx.trigger = 0;
        //DBG_PRINT("spi tx buf=0x%08x len=%u\r\n", (INT32U) pSpi->tx_buf, pSpi->len);
#if (_OPERATING_SYSTEM != _OS_NONE)
        OSQFlush(pSpi->spi_dma_txq);
        status = drv_l1_dma_transfer_with_queue(&dma_tx, pSpi->spi_dma_txq);
#else
        status = drv_l1_dma_transfer(&dma_tx);
#endif
        if (status != 0) {
            return status;
        }
    }
#if (_OPERATING_SYSTEM != _OS_NONE)
    if(pSpi->rcv_len){
        result = osMessageGet(pSpi->spi_dma_rxq, 1000); // osWaitForever
        if((result.status != osEventMessage) || (result.value.v != C_DMA_STATUS_DONE)) {
            return STATUS_FAIL;
        }
    }
    if(pSpi->len){
        result = osMessageGet(pSpi->spi_dma_txq, 1000); // osWaitForever
        if((result.status != osEventMessage) || (result.value.v != C_DMA_STATUS_DONE)) {
            return STATUS_FAIL;
        }
    }
#else
     if(pSpi->rcv_len){
        INT64U timeoutcnt = ((pSpi->rcv_len+255)>>8) * SPI_TIMEOUT_VALUE;

        //DBG_PRINT("pSpi->rcv_len=%u, %u\r\n", pSpi->rcv_len, (INT32U)timeoutcnt);

        while (timeoutcnt != 0) {
        DMA_IRQHandler();
            if (done_rx == C_DMA_STATUS_DONE)
                break;
            timeoutcnt--;
        }
        if (done_rx != C_DMA_STATUS_DONE) {
            DBG_PRINT("spi rx dma timeout. %u, %u\r\n", pSpi->rcv_len, done_rx);
            return SPI_TIMEOUT;
        }
	}

    if(pSpi->len){
        INT32U timeoutcnt = SPI_TIMEOUT_VALUE;

        while (timeoutcnt != 0) {
            DMA_IRQHandler();
            if (done_tx == C_DMA_STATUS_DONE)
                break;
            timeoutcnt--;
        }
        if (done_tx != C_DMA_STATUS_DONE) {
            DBG_PRINT("spi tx dma timeout\r\n");
		return SPI_TIMEOUT;
	}
    }
#endif

	return STATUS_OK;
}

#if (K_SPI_SUPPORT_SPINAND == 1)

#define K_SPI_TX_APPLY_DMA_WIDTH_4		0 // IC limitation, TX couldn't work for 4 byte dma width, so make 0
#define K_SPI_RX_APPLY_DMA_WIDTH_4		0 // IC limitation, RX couldn't work for 4 byte dma width, so make 0

extern void DMA_IRQHandler(void);

static INT32S spi_dma_rx_wait_complete(INT8U spi_num, volatile INT8S *pdone_rx)
{
    #if (_OPERATING_SYSTEM != _OS_NONE)
	if (spi_dma_rx_done_sema[spi_num] != NULL) {
		INT32U tick_elapsed = 0;
		INT32U tick_step = 100;
		BaseType_t xResult;
		INT32U tick_500ms = pdMS_TO_TICKS(500);// 500ms

		do {
			xResult = xSemaphoreTake(spi_dma_rx_done_sema[spi_num], tick_step);
			tick_elapsed += tick_step;
		} while (xResult == pdFALSE);
		if (tick_elapsed > tick_500ms)
			DBG_PRINT("rx wait more than 500ms. %u, %u\r\n", tick_elapsed, tick_500ms);
	}
	else
	#endif
	{
        SPI_MASTER *pSpi = spi_workmem_get(spi_num);
        INT64U timeoutcnt = ((pSpi->rcv_len+255)>>8) * SPI_TIMEOUT_VALUE;

        while (timeoutcnt != 0) {
            DMA_IRQHandler();
            if (*pdone_rx == C_DMA_STATUS_DONE)
                break;
            timeoutcnt--;
        }
        if (*pdone_rx != C_DMA_STATUS_DONE) {
            DBG_PRINT("spi nand rx dma timeout\r\n");
            return SPI_TIMEOUT;
        }
    }

	return STATUS_OK;
}

static INT32S spi_dma_tx_wait_complete(INT8U spi_num, volatile INT8S *pdone_tx)
{
    #if (_OPERATING_SYSTEM != _OS_NONE)
	if (spi_dma_tx_done_sema[spi_num] != NULL) {
		INT32U tick_elapsed = 0;
		INT32U tick_step = 100;
		BaseType_t xResult;
		INT32U tick_500ms = pdMS_TO_TICKS(500);// 500ms

		do {
			xResult = xSemaphoreTake(spi_dma_tx_done_sema[spi_num], tick_step);
			tick_elapsed += tick_step;
		} while (xResult == pdFALSE);
		if (tick_elapsed > tick_500ms)
			DBG_PRINT("tx wait more than 500ms. %u, %u\r\n", tick_elapsed, tick_500ms);
	}
	else
	#endif
	{
        INT32U timeoutcnt = SPI_TIMEOUT_VALUE;
        INT32U cur_clk_rate;

        drv_l1_spi_clk_rate_get(spi_num, &cur_clk_rate);
        if (cur_clk_rate > 0 && cur_clk_rate <= 24000000)
            timeoutcnt = timeoutcnt * ((24000000 / cur_clk_rate)*2);
        while (timeoutcnt != 0) {
            DMA_IRQHandler();
            if (*pdone_tx == C_DMA_STATUS_DONE)
                break;
            timeoutcnt--;
        }
        if (*pdone_tx != C_DMA_STATUS_DONE) {
            DBG_PRINT("spi nand tx dma timeout. %u\r\n", timeoutcnt);
            return SPI_TIMEOUT;
        }
    }

	return STATUS_OK;
}

static INT32S spi_transceive_dma_2(INT8U spi_num)
{
	volatile INT8S done_rx, done_tx; // must volatile to avoid compiler optimization issue
	#if SPI_RX_DMA_ONLY == 1
	INT8U rx_dma_only = 0;
	#endif
	INT32S status;
	DMA_STRUCT dma_rx={0}, dma_tx={0};
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num);
	INT8U clk_divisor;

	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}

	/* reset spi control */
	pSpiReg->CTRL |= SPI_CTRL_RST;
	spi_tx_int_set(pSpiReg, SPI_DISABLED);
	spi_rx_int_set(pSpiReg, SPI_DISABLED);
	drv_l1_spi_txrx_level_set(spi_num, 0x7, 0);
    drv_l1_spi_clk_divisor_get(spi_num, &clk_divisor);
    if(clk_divisor == 0)
        spi_fasp_apb_set(spi_num);
    else
        spi_fasp_apb_clear(spi_num);

	// rx setting
	dma_rx.width = DMA_DATA_WIDTH_1BYTE;
	done_rx = C_DMA_STATUS_WAITING;
	if (pSpi->rcv_len != 0) {
        if (spi_rx_edge_polar[spi_num] == SPI_RX_EDGE_RISING)
            //read data from SPI device pol = 0, pha = 1. RisingEdge Master Get Data
            drv_l1_spi_pha_pol_set(spi_num, PHA0_POL0);
        else
		//read data from SPI device pol = 0, pha = 1. FallingEdge Master Get Data
		drv_l1_spi_pha_pol_set(spi_num, PHA1_POL0);

		dma_rx.s_addr = (INT32U) &pSpiReg->RX_DATA;
		if(pSpi->rcv_len == 0) {
			dma_rx.t_addr = C_SPI_DMA_DUMMY_ADDR;	/* reserved memory */
			dma_rx.count = pSpi->len;
		} else {
			dma_rx.t_addr = (INT32U) pSpi->rx_buf;
			dma_rx.count = pSpi->rcv_len;
			#if (K_SPI_RX_APPLY_DMA_WIDTH_4 == 1)
			if (((dma_rx.count & 0x3) == 0) && ((dma_rx.t_addr & 0x3) == 0)) {
				dma_rx.width = DMA_DATA_WIDTH_4BYTE;
				dma_rx.count >>= 2;
			}
			#endif
		}

		dma_rx.notify = &done_rx;
		dma_rx.timeout = 128;
		//dma_rx.aes = 0;
		dma_rx.trigger = 0;
        if((pSpiReg->MISC & 0x400)>>10)
            dma_rx.fast_apb = 1;
        #if SPI_RX_DMA_ONLY == 1 // apply RX dma only here
        if (pSpi->len == 0) {
            // since just RX dma, use GVC770 spi rx dma only mode
            rx_dma_only = 1;
            pSpiReg->DMA_RX_CYC = dma_rx.count;
            pSpiReg->CTRL |= SPI_CTRL_RX_ONLY_EN;
        }
        #endif

        spi_allocate_dma_sema(spi_num);
		if (spi_dma_rx_done_sema[spi_num] != NULL) {
            spi_dma_rx_result[spi_num] = C_DMA_STATUS_WAITING;
            status = drv_l1_dma_transfer_with_callback(&dma_rx, spi_dma_rx_user_isr[spi_num]);
		}
		else {
            status = drv_l1_dma_transfer(&dma_rx);
		}
		if(status != 0) {
			DBG_PRINT("spi nand rx dma fail %d\r\n", status);
			return status;
		}
		#if SPI_RX_DMA_ONLY == 1
		if (rx_dma_only) {
            // when spi RX dma dont required TX dma to trigger, no need to trigger TX dma
            //DBG_PRINT("spi RX dma only. count=%u, DMA_RX_CYC=%u\r\n", dma_rx.count, pSpiReg->DMA_RX_CYC);
            status = spi_dma_rx_wait_complete(spi_num, &done_rx);
            if (status != STATUS_OK) {
                DBG_PRINT("spi nand rx dma complete fail %d, rcv_len=%u\r\n", status, pSpi->rcv_len);
                return status;
            }
            return STATUS_OK;
		}
		#endif
	}

	// tx setting
	dma_tx.width = DMA_DATA_WIDTH_1BYTE;
	done_tx = C_DMA_STATUS_WAITING;
	if (pSpi->len == 0) {
		dma_tx.s_addr = (INT32U) C_SPI_DMA_DUMMY_ADDR;  /* reserved memory */
		dma_tx.count = pSpi->rcv_len;
		#if (K_SPI_RX_APPLY_DMA_WIDTH_4 == 1)
		if (dma_rx.width == DMA_DATA_WIDTH_4BYTE)
		{
            dma_tx.width = DMA_DATA_WIDTH_4BYTE;
            dma_tx.count >>= 2;
		}
		#endif
	} else {
        drv_l1_spi_txrx_level_set(spi_num, 0x0, 0);
		//write data to SPI device: pol = 0, pha = 0. RisingEdge Master Send Data
		drv_l1_spi_pha_pol_set(spi_num, PHA0_POL0);

		dma_tx.s_addr = (INT32U) pSpi->tx_buf;
		dma_tx.count = pSpi->len;
		#if (K_SPI_TX_APPLY_DMA_WIDTH_4 == 1)
		if (((dma_tx.count & 0x3) == 0) && ((dma_tx.t_addr & 0x3) == 0)) {
            dma_tx.width = DMA_DATA_WIDTH_4BYTE;
            dma_tx.count >>= 2;

            dma_rx.width = DMA_DATA_WIDTH_4BYTE;
            dma_rx.count >>= 2;
        }
        #endif
	}

	dma_tx.t_addr = (INT32U) &pSpiReg->TX_DATA;
	dma_tx.notify = &done_tx;
	dma_tx.timeout = 128;
	//dma_tx.aes = 0;
	dma_tx.trigger = 0;
	if (pSpi->len != 0) {
        spi_allocate_dma_sema(spi_num);
		if (spi_dma_tx_done_sema[spi_num] != NULL) {
            spi_dma_tx_result[spi_num] = C_DMA_STATUS_WAITING;
            status = drv_l1_dma_transfer_with_callback(&dma_tx, spi_dma_tx_user_isr[spi_num]);
		}
		else {
            status = drv_l1_dma_transfer(&dma_tx);
		}
	}
	else
             status = drv_l1_dma_transfer(&dma_tx);
	if (status != 0) {
		DBG_PRINT("spi nand tx dma fail %d\r\n", status);
		return status;
	}

	if (pSpi->rcv_len != 0) {
		status = spi_dma_rx_wait_complete(spi_num, &done_rx);
		if (status != STATUS_OK) {
            DBG_PRINT("spi nand rx dma complete fail %d, rcv_len=%u\r\n", status, pSpi->rcv_len);
			return status;
        }
	}

	if (pSpi->len != 0) {
		status = spi_dma_tx_wait_complete(spi_num, &done_tx);
		if (status != STATUS_OK) {
            DBG_PRINT("spi nand tx dma complete fail %d, len=%u\r\n", status, pSpi->len);
			return status;
        }
	}

	return STATUS_OK;
}
#endif
/**
 * @brief   spi transmit data by dma in slave mode
 * @param   spi_num: spi device number
 * @return 	ret: STATUS_FAIL/STATUS_OK
 */
#if 0
static INT32S spi_slave_transceive_dma(INT8U spi_num)
{
	INT8S done_rx;
	INT32S status;

	DMA_STRUCT dma_rx={0};
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num);
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}

	/* set DMA 0 for receiving data */
	R_DMA0_SRC_ADDR = (INT32U) &pSpiReg->RX_DATA;
    R_DMA0_TAR_ADDR = (INT32U) pSpi->rx_buf;
	R_DMA0_TX_COUNT = pSpi->rcv_len;
	R_DMA_DEVICE   &= ~((C_DMA_IO_MASK) << C_DMA0_IO_SHIFT);
	if(spi_num == 0) {
		R_DMA_DEVICE |= C_DMA_IO_SPI0_RX << C_DMA0_IO_SHIFT;
	} else {
		R_DMA_DEVICE |= C_DMA_IO_SPI1_RX << C_DMA0_IO_SHIFT;
	}

	R_DMA0_CTRL     = C_DMA_CTRL_8BIT|C_DMA_CTRL_INT|C_DMA_CTRL_NORMAL_INT|C_DMA_CTRL_DEMAND_TRANS
	       		      |C_DMA_CTRL_IO2M|C_DMA_CTRL_SRC_FIX|C_DMA_CTRL_EXTERNAL|C_DMA_CTRL_ENABLE;

	while((R_DMA_INT & C_DMA0_INT_PEND) == 0);

	if (R_DMA_INT & C_DMA0_TIMEOUT) {
		return C_DMA0_TIMEOUT;
	}
	R_DMA_INT = C_DMA0_INT_PEND;

	// Invalid target memory from cache
	drv_l1_cp15_invalidate_dcache_range((INT32U) pSpi->rx_buf, pSpi->rcv_len); //cache_invalid_range((INT32U) pSpi->rx_buf, pSpi->rcv_len);
	return STATUS_OK;
}
#else
static INT32S spi_slave_transceive_dma(INT8U spi_num)
{
	INT8S done_rx;
	INT32S status;
#if (_OPERATING_SYSTEM != _OS_NONE)
    osEvent result;
#endif
	DMA_STRUCT dma_rx={0};
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num);
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}

	done_rx = C_DMA_STATUS_WAITING;
	dma_rx.s_addr = (INT32U) &pSpiReg->RX_DATA;
	dma_rx.t_addr = (INT32U) pSpi->rx_buf;
	dma_rx.count = pSpi->rcv_len;
	dma_rx.width = DMA_DATA_WIDTH_1BYTE;
	dma_rx.notify = &done_rx;
	dma_rx.timeout = 0;
	//dma_rx.aes = 0;
	dma_rx.trigger = 0;

#if (_OPERATING_SYSTEM != _OS_NONE)
	OSQFlush(pSpi->spi_dma_rxq);
	status = drv_l1_dma_transfer_with_queue(&dma_rx, pSpi->spi_dma_rxq);
#else
	status = drv_l1_dma_transfer(&dma_rx);
#endif
	if(status != 0) {
		return status;
	}

#if (_OPERATING_SYSTEM != _OS_NONE)
    result = osMessageGet(pSpi->spi_dma_rxq, 1000); // osWaitForever
    if (result.status != osEventMessage) {
        return STATUS_FAIL;
    }

    if (result.value.v == C_DMA_STATUS_DONE) {
		return STATUS_FAIL;
	}

#else
	while (done_rx == C_DMA_STATUS_WAITING);

	if (done_rx != C_DMA_STATUS_DONE) {
		return SPI_TIMEOUT;
	}
#endif
	// Invalid target memory from cache
	drv_l1_cp15_invalidate_dcache_range((INT32U) pSpi->rx_buf, pSpi->rcv_len); // cache_invalid_range((INT32U) pSpi->rx_buf, pSpi->rcv_len);
	return STATUS_OK;
}
#endif
/**
 * @brief   spi polling status
 * @param   spi_num: spi device number
 * @return 	ret: STATUS_FAIL/STATUS_OK
 */
static INT32S spi_transceive_polling(INT8U spi_num)
{
    INT8U start_receive_stage = 0;
	INT8U dummy;
	INT32U timeout;
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num);
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}

	/* reset spi control */
	pSpiReg->CTRL |= SPI_CTRL_RST;
	spi_tx_int_set(pSpiReg, SPI_DISABLED);
	spi_rx_int_set(pSpiReg, SPI_DISABLED);
	drv_l1_spi_txrx_level_set(spi_num, 0, 0);

	drv_tiny_wait(2); // 3us

	/* polling status */
	while (pSpi->len > pSpi->tx_count) {
		drv_l1_spi_pha_pol_set(spi_num, PHA0_POL0);
		pSpiReg->TX_DATA = (INT32U)pSpi->tx_buf[pSpi->tx_count];
		timeout = 0;
		 while((pSpiReg->RX_STATUS & SPI_RX_STATUS_RXFLAG) == 0) {
			if(++timeout == SPI_TIMEOUT_VALUE) {
	        	return SPI_TIMEOUT;
	    	}
	    }

		if(pSpi->tx_count < pSpi->rcv_len) {
            if (start_receive_stage == 0) {
                if (spi_rx_edge_polar[spi_num] == SPI_RX_EDGE_RISING)
                    //read data from SPI device pol = 0, pha = 1. RisingEdge Master Get Data
            drv_l1_spi_pha_pol_set(spi_num, PHA0_POL0);
                else
                    //read data from SPI device pol = 0, pha = 1. FallingEdge Master Get Data
			drv_l1_spi_pha_pol_set(spi_num, PHA1_POL0);
                start_receive_stage = 1;
            }
			pSpi->rx_buf[pSpi->tx_count] = (INT8U)pSpiReg->RX_DATA;
		} else {
			dummy = (INT8U)pSpiReg->RX_DATA;
			if(dummy){};	//for remove compiler warning
		}
		pSpi->tx_count++;
	}

	return STATUS_OK;
}

// use by spi nand
static INT32S spi_transceive_polling_2(INT8U spi_num)
{
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num);
	INT8U* tx_buffer;
	INT8U* rx_buffer;
	volatile INT8U dummy; // must volatile to avoid compiler optimization issue
	volatile INT32U timeout;
	INT32U i;
	INT32U tx_clock_rate;
	INT32U rx_clock_rate=12000000;

	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}

	tx_buffer = pSpi->tx_buf;
	rx_buffer = pSpi->rx_buf;

	/* reset spi control */
	pSpiReg->CTRL |= SPI_CTRL_RST;
	spi_tx_int_set(pSpiReg, SPI_DISABLED);
	spi_rx_int_set(pSpiReg, SPI_DISABLED);
	drv_l1_spi_txrx_level_set(spi_num, 0, 0);

	//write data to SPI device
	drv_l1_spi_tx_clk_get(spi_num, &tx_clock_rate);
	drv_l1_spi_clk_set(spi_num, tx_clock_rate);
	drv_l1_spi_pha_pol_set(spi_num, PHA0_POL0);

	drv_tiny_wait(2); // 3us

	for(i=0; i<pSpi->len; i++){
		pSpiReg->TX_DATA = (INT32U)tx_buffer[i];

		timeout = 0;
		while(0 == (pSpiReg->MISC & SPI_MISC_RNE)){//rx empty then waiting
			if (++timeout == SPI_TIMEOUT_VALUE) {
				DBG_PRINT("spi nand tx cpu timeout\r\n");
		        return SPI_TIMEOUT;
		    }
		}
		dummy = pSpiReg->RX_DATA; // this is must
		dummy = dummy;// resolve compile warning use
	}

	if(pSpi->rcv_len != 0){
		//read data from SPI device

		drv_l1_spi_rx_clk_get(spi_num, &rx_clock_rate);
		drv_l1_spi_clk_set(spi_num, rx_clock_rate);

        if (spi_rx_edge_polar[spi_num] == SPI_RX_EDGE_RISING)
            //read data from SPI device pol = 0, pha = 1. RisingEdge Master Get Data
        drv_l1_spi_pha_pol_set(spi_num, PHA0_POL0);
        else
            //read data from SPI device pol = 0, pha = 1. FallingEdge Master Get Data
		drv_l1_spi_pha_pol_set(spi_num, PHA1_POL0);

		for(i=0; i<pSpi->rcv_len; i++){
			pSpiReg->TX_DATA = (INT32U)0x00; // this is must to trigger RX to read

			timeout = 0;
			while(0 == (pSpiReg->MISC & SPI_MISC_RNE)){//rx empty then waiting
				if (++timeout == SPI_TIMEOUT_VALUE) {
                    DBG_PRINT("spi nand rx cpu timeout\r\n");
		        	return SPI_TIMEOUT;
		    	}
			}

			if(pSpi->rcv_len == 0xFFFFFFFFUL){ //Wait OIP finished
				rx_buffer[0] = (INT8U)(pSpiReg->RX_DATA);
				if( (rx_buffer[0] & 0x1) == 0 ){
					break;
				}
			}
			else{
				rx_buffer[i] = (INT8U)(pSpiReg->RX_DATA);
			}
		}
	}

	return STATUS_OK;
}

/**
 * @brief   set spi rx data latched edge polar
            set once and before receiving data
 * @param   spi_num: spi device number
 * @param   spi_num: SPI_RX_EDGE_FALLING, SPI_RX_EDGE_RISING
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_rx_edge_polar_set(INT8U spi_num, INT8U polar)
{
    spi_rx_edge_polar[spi_num] = polar;

    return STATUS_OK;
}

/**
 * @brief   Initialize SPI interface
 * @param   spi_num: spi device number
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_init(INT8U spi_num)
{
#if (_OPERATING_SYSTEM != _OS_NONE)
#if (K_SPI_USE_CMSIS_OS == 1)
	osMessageQDef_t tx_q = { SPI_QUEUE_MAX, sizeof(INT32U), 0 };
	osMessageQDef_t rx_q = { SPI_QUEUE_MAX, sizeof(INT32U), 0 };
#endif
#endif
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num);
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}

	/* clear spi */
    //drv_l1_spi_clock_set(spi_num, SPI_CLOCK_ENABLE);
	drv_l1_spi_uninit(spi_num);

#if (_OPERATING_SYSTEM != _OS_NONE)
	/* create queue */

#if (K_SPI_USE_CMSIS_OS == 1)
	if(pSpi->spi_dma_txq == 0) {
		pSpi->spi_dma_txq = osMessageCreate(&tx_q, NULL);
		if(pSpi->spi_dma_txq == 0) {
			return STATUS_FAIL;
		}
	}

	if(pSpi->spi_dma_rxq == 0) {
		pSpi->spi_dma_rxq = osMessageCreate(&rx_q, NULL);
		if(pSpi->spi_dma_rxq == 0) {
			return STATUS_FAIL;
		}
	}
#else
	if(pSpi->spi_dma_txq == 0) {
		pSpi->spi_dma_txq = xQueueCreate(SPI_QUEUE_MAX, sizeof(INT32U));
		if(pSpi->spi_dma_txq == 0) {
			return STATUS_FAIL;
		}
	}

	if(pSpi->spi_dma_rxq == 0) {
		pSpi->spi_dma_rxq = xQueueCreate(SPI_QUEUE_MAX, sizeof(INT32U));
		if(pSpi->spi_dma_rxq == 0) {
			return STATUS_FAIL;
		}
	}
#endif
#endif
    drv_l1_spi_clock_set(spi_num,SPI_CLOCK_ENABLE);
    drv_l1_spi_mio_set(spi_num, IO_1BIT, SPI_TX);
	/* enable the SPI */
	pSpiReg->CTRL |= SPI_CTRL_ENA;

	/* enable the SPI smart mode */
	pSpiReg->MISC |= SPI_MISC_SMART;
#if SPI_INTERRUPT == 1
    pSpiReg->CTRL |= SPI_CTRL_RST;
	spi_tx_int_set(pSpiReg, SPI_ENABLED);
	spi_rx_int_set(pSpiReg, SPI_ENABLED);
#endif
	drv_l1_spi_txrx_level_set(spi_num, 4, 4); /* set interrupt trigger level */
	if(spi_num == SPI_0) {
        vic_irq_register(VIC_SPI0, SPI0_IRQHandler);
        vic_irq_enable(VIC_SPI0);
        #if SPI_FIQ == 1
        vic_fiq_enable(VIC_SPI0);
        #endif
	}

#if SPI_NUM > 1
    if(spi_num == SPI_1) {
        vic_irq_register(VIC_SPI1, SPI1_IRQHandler);
        vic_irq_enable(VIC_SPI1);
        #if SPI_FIQ == 1
        vic_fiq_enable(VIC_SPI1);
        #endif
	}
#endif

	return STATUS_OK;
}

/**
 * @brief   Un-initialize SPI interface
 * @param   spi_num: spi device number
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_uninit(INT8U spi_num)
{
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num);
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}

	/* clear register */
	spi_reg_clear(pSpiReg);

	/* clear memory */
	CLEAR(pSpi, sizeof(SPI_MASTER));

	if(spi_num == SPI_0) {
        vic_irq_disable(VIC_SPI0);
        #if SPI_FIQ == 1
        vic_fiq_disable(VIC_SPI0);
        #endif
	}

#if SPI_NUM > 1
	if(spi_num == SPI_1) {
        vic_irq_disable(VIC_SPI1);
        #if SPI_FIQ == 1
        vic_fiq_disable(VIC_SPI1);
        #endif
	}
#endif
    drv_l1_spi_clock_set(spi_num,SPI_CLOCK_DISABLE);

	return STATUS_OK;
}

/**
 * @brief   disable spi interface
 * @param   spi_num: spi device number
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_disable(INT8U spi_num)
{
	spiReg_t *pSpiReg = spi_select(spi_num);
	if(pSpiReg == 0) {
		return STATUS_FAIL;
	}

	pSpiReg->CTRL &= ~SPI_CTRL_ENA;
	return STATUS_OK;
}

/**
 * @brief   set spi fifo threshold
 * @param   spi_num: spi device number
 * @param   tx_level: tx fifo threshold, 0 ~ 7
 * @param   rx_level: rx fifo threshold, 0 ~ 7
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_txrx_level_set(INT8U spi_num, INT8U tx_level, INT8U rx_level)
{
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num);
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}

	if(rx_level > 7) {
		rx_level = 7;
	}

	if(tx_level > 7) {
		tx_level = 7;
	}

	pSpiReg->TX_STATUS &= ~SPI_TX_STATUS_LEVEL;
	pSpiReg->TX_STATUS |= tx_level << 4;

	pSpiReg->RX_STATUS &= ~SPI_RX_STATUS_LEVEL;
	pSpiReg->RX_STATUS |= rx_level << 4;

	pSpi->tx_int_level = tx_level;
	pSpi->rx_int_level = rx_level;

	return STATUS_OK;
}


/**
 * @brief   set spi divisor for master mode.
 * @param   spi_num: spi device number
 * @param   spi_clk: spi divisor
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_spi_divisor_set(INT8U spi_num, INT32U div)
{
	spiReg_t *pSpiReg = spi_select(spi_num);
	if(pSpiReg == 0) {
		return STATUS_FAIL;
	}

	/* set the SPI clock */
	pSpiReg->CTRL &= ~SPI_CTRL_CLK;
	pSpiReg->CTRL |= div;

    return STATUS_OK;
}

/**
 * @brief   get spi divisor for master mode
 * @param   spi_num: spi device number
 * @param   p_spi_clk_divisor: spi divisor
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_spi_clk_divisor_get(INT8U spi_num, INT8U *p_spi_clk_divisor)
{
	spiReg_t *pSpiReg = spi_select(spi_num);

	if (spi_num >= SPI_MAX_NUMS) {
		return STATUS_FAIL;
	}
	*p_spi_clk_divisor = pSpiReg->CTRL & SPI_CTRL_CLK;

	return STATUS_OK;
}

/**
 * @brief   set spi clock speed for master mode.
 * @param   spi_num: spi device number
 * @param   spi_clk: spi clock speed
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_spi_clk_set(INT8U spi_num, INT32U spi_clk)
{
	INT8U div, N;
	spiReg_t *pSpiReg = spi_select(spi_num);
	if(pSpiReg == 0) {
		return STATUS_FAIL;
	}

	div = SystemCoreClock / spi_clk;
	for(N=0; N<7; N++) {
		div >>= 1;
		if(div == 1) {
			break;
		}
	}

	if(N == 7) {
		N--;
	}

    drv_l1_spi_divisor_set(spi_num, N);

	return STATUS_OK;
}

/**
 * @brief   get spi clock rate for master mode.
 * @param   spi_num: spi device number
 * @param   p_clk_rate: spi clock rate
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_spi_clk_rate_get(INT8U spi_num, INT32U *p_clk_rate)
{
	INT8U clk_divisor;
	INT32U i, div;

	if (spi_num >= SPI_MAX_NUMS) {
		return STATUS_FAIL;
	}
	drv_l1_spi_clk_divisor_get(spi_num, &clk_divisor);
	clk_divisor++;
	for (i=0, div = 1; i < clk_divisor; i++)
		div <<= 1;

	*p_clk_rate = MCLK/div;

	return STATUS_OK;
}

/**
 * @brief   set spi tx clock
 * @param   spi_num: spi device number
 * @param   spi_clk: spi clock
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_tx_clk_set(INT8U spi_num, INT32U spi_clk)
{
	INT32S ret_val;

	ret_val = drv_l1_spi_clk_set(spi_num, spi_clk);
	if (ret_val == STATUS_OK) {
		spi_tx_clock_rate[spi_num] = spi_clk;
	}
	return ret_val;
}

/**
 * @brief   get spi tx clock
 * @param   spi_num: spi device number
 * @param   p_spi_clk: spi clock
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_tx_clk_get(INT8U spi_num, INT32U *p_spi_clk)
{
	if (spi_num < SPI_MAX_NUMS) {
		*p_spi_clk = spi_tx_clock_rate[spi_num];
		return STATUS_OK;
	}
	else
		return STATUS_FAIL;
}

/**
 * @brief   set spi rx clock
 * @param   spi_num: spi device number
 * @param   spi_clk: spi clock
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_rx_clk_set(INT8U spi_num, INT32U spi_clk)
{
	INT32S ret_val;

	ret_val = drv_l1_spi_clk_set(spi_num, spi_clk);
	if (ret_val == STATUS_OK) {
		spi_rx_clock_rate[spi_num] = spi_clk;
	}
	return ret_val;
}

/**
 * @brief   get spi rx clock
 * @param   spi_num: spi device number
 * @param   p_spi_clk: spi clock
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_rx_clk_get(INT8U spi_num, INT32U *p_spi_clk)
{
	if (spi_num < SPI_MAX_NUMS) {
		*p_spi_clk = spi_rx_clock_rate[spi_num];
		return STATUS_OK;
	}
	else
		return STATUS_FAIL;
}

/**
 * @brief   Set the spi clock phase and polarity for master mode.
 * @param   spi_num: spi device number
 * @param   pha_pol: PHA0_POL0/PHA0_POL1/PHA1_POL0/PHA1_POL1
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_pha_pol_set(INT8U spi_num, INT8U pha_pol)
{
	INT32U reg;
	spiReg_t *pSpiReg = spi_select(spi_num);
	if(pSpiReg == 0) {
		return STATUS_FAIL;
	}

	// spi disable first
	//pSpiReg->CTRL &= ~SPI_CTRL_ENA; // remark this line, it will cause extra triangle clock pulse

	reg = pSpiReg->CTRL;
	switch(pha_pol)
	{
		case PHA0_POL0:
			reg &= ~(SPI_CTRL_CPHA | SPI_CTRL_CPOL);
			break;

		case PHA0_POL1:
			reg &= ~SPI_CTRL_CPHA;
			reg |= SPI_CTRL_CPOL;
			break;

		case PHA1_POL0:
			reg |= SPI_CTRL_CPHA;
			reg &= ~SPI_CTRL_CPOL;
			break;

		case PHA1_POL1:
			reg |= SPI_CTRL_CPHA;
			reg |= SPI_CTRL_CPOL;
			break;

		default:
			break;
	}

	reg |= SPI_CTRL_ENA;
	pSpiReg->CTRL = reg;

    if (pSpiReg->CTRL != reg)
        DBG_PRINT("CTRL != reg, 0x%08x, should 0x%08x\r\n", pSpiReg->CTRL, reg);

	__asm volatile("nop"); // dont remove this

	return STATUS_OK;
}

INT32S drv_l1_spi_delay_set(INT8U spi_num, INT8U delay)
{
    INT32U reg;
    spiReg_t *pSpiReg = spi_select(spi_num);
    if(pSpiReg == 0) {
		return STATUS_FAIL;
	}

	reg = pSpiReg->CTRL;
	if(delay == 0x1){
        reg |= SPI_RD_LATCH_DLY_0;
        reg &= ~SPI_RD_LATCH_DLY_1;
	}
	else if(delay == 0x2){
        reg &= ~SPI_RD_LATCH_DLY_0;
        reg |= SPI_RD_LATCH_DLY_1;
	}
	else if(delay == 0x3){
        reg |= (SPI_RD_LATCH_DLY_0 | SPI_RD_LATCH_DLY_1);
	}
	else{
        reg &= ~(SPI_RD_LATCH_DLY_0 | SPI_RD_LATCH_DLY_1);
	}
	pSpiReg->CTRL = reg;

	return STATUS_OK;
}

INT32S drv_l1_spi_mio_set(INT8U spi_num, SPI_IO_NUMBER mio, SPI_IO_DIR io_dir)
{
	INT32U reg;
	spiReg_t *pSpiReg = spi_select(spi_num);
	if(pSpiReg == 0) {
		return STATUS_FAIL;
	}

	reg = pSpiReg->CTRL;
	switch(mio)
	{
		case IO_2BIT:
			reg &= ~(SPI_CTRL_QPI);
			reg |= SPI_CTRL_2IO;
			if (io_dir == SPI_RX)
				reg |= SPI_CTRL_2IO_OEB;
			else
				reg &= (~SPI_CTRL_2IO_OEB);
			break;

		case IO_4BIT:
			reg &= ~(SPI_CTRL_2IO);
			reg |= SPI_CTRL_QPI;
			if (io_dir == SPI_RX)
				reg |= SPI_CTRL_QPIXD_OEB;
			else
				reg &= (~SPI_CTRL_QPIXD_OEB);
			break;

		case IO_1BIT:
		default:
			reg &= ~(SPI_CTRL_2IO | SPI_CTRL_QPI);
			reg &= ~(SPI_CTRL_2IO_OEB | SPI_CTRL_QPIXD_OEB); 	// set to default according prog guide
																// actually this 2 bit not effect when
																// SPI_CTRL_2IO, SPI_CTRL_QPI not 1
			break;
	}

	pSpiReg->CTRL = reg;

	return STATUS_OK;
}

/**
 * @brief   Set the spi loop back mode
 * @param   spi_num: spi device number
 * @param   status: SPI_LBM_NORMAL / SPI_LBM_LOOP_BACK
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_lbm_set(INT8U spi_num, INT8S status)
{
	spiReg_t *pSpiReg = spi_select(spi_num);
	if(pSpiReg == 0) {
		return STATUS_FAIL;
	}

	if(status == SPI_LBM_LOOP_BACK) {
		pSpiReg->CTRL |= SPI_CTRL_LBM;
	} else {
		pSpiReg->CTRL &= ~SPI_CTRL_LBM;
	}

	return STATUS_OK;
}

/**
 * @brief   Set the spi cs pin by external gpio
 * @param   spi_num: spi device number
 * @param   enable: enable / disable
 * @param   pin_num: gpio pin mumber
 * @param   active: active level
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_cs_by_gpio_set(INT8U spi_num, INT8U enable, INT8U pin_num, INT8U active)
{
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	if(pSpi == 0) {
		return STATUS_FAIL;
	}

	if(enable) {
		pSpi->cs_by_external = TRUE;
		pSpi->cs_gpio_pin = pin_num;
		pSpi->cs_active = active;

		gpio_init_io(pin_num, GPIO_OUTPUT);
		gpio_set_port_attribute(pin_num, 1);
		gpio_write_io(pSpi->cs_gpio_pin, (!pSpi->cs_active)&0x1);
	} else {
		pSpi->cs_by_external = FALSE;
	}

	return STATUS_OK;
}

/**
 * @brief   master mode transmit and receive data by dma
 * @param   spi_num: spi device number
 * @param   tx_data: tx data buffer
 * @param   tx_len: tx data size
 * @param   rx_data: rx data buffer
 * @param   rx_len: rx data size
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_transceive(INT8U spi_num, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len)
{
	INT32S result;
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	if(pSpi == 0) {
		return STATUS_FAIL;
	}

	result = spi_usage_get(pSpi);
	if (result == SPI_BUSY) {
		return SPI_BUSY;
	}

	pSpi->tx_buf = tx_data;
	pSpi->rx_buf = rx_data;
	pSpi->len = tx_len;
	pSpi->rcv_len = rx_len;
	pSpi->tx_count = 0;
	pSpi->rx_count = 0;

	if (pSpi->cs_by_external == TRUE) { /* chip select by internal */
		#if (_DRV_L1_SPI_SW_CS == 0)
			gpio_write_io(pSpi->cs_gpio_pin, pSpi->cs_active);
        #endif
	}

	result = spi_transceive_dma(spi_num);
	if (pSpi->cs_by_external == TRUE) { /* chip select by internal */
		#if (_DRV_L1_SPI_SW_CS == 0)
            gpio_write_io(pSpi->cs_gpio_pin, (!pSpi->cs_active)&0x1);
        #endif
	}
	drv_l1_spi_pha_pol_set(spi_num, PHA0_POL0); // restore back
	spi_usage_free(pSpi);

	return result;
}

#if (K_SPI_SUPPORT_SPINAND == 1)
/**
 * @brief   master mode transmit and receive data by dma
 * @param   spi_num: spi device number
 * @param   tx_data: tx data buffer
 * @param   tx_len: tx data size
 * @param   rx_data: rx data buffer
 * @param   rx_len: rx data size
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
 // use by spi nand
INT32S drv_l1_spi_transceive_dma_v2(INT8U spi_num, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len)
{
    INT32U tx_clock_rate=12000000;
	INT32U rx_clock_rate=12000000;
	INT32S result;
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	if(pSpi == 0) {
		return STATUS_FAIL;
	}

	result = spi_usage_get(pSpi);
	if (result == SPI_BUSY) {
		return SPI_BUSY;
	}

	pSpi->tx_buf = tx_data;
	pSpi->rx_buf = rx_data;
	pSpi->len = tx_len;
	pSpi->rcv_len = (tx_len == 0) ? rx_len : 0;
	pSpi->tx_count = 0;
	pSpi->rx_count = 0;

	if (pSpi->cs_by_external == TRUE) { /* chip select by internal */
		#if (_DRV_L1_SPI_SW_CS == 0)
			gpio_write_io(pSpi->cs_gpio_pin, pSpi->cs_active);
        #endif
	}

	if (tx_len == 0 && rx_len != 0) {
		drv_l1_spi_rx_clk_get(spi_num, &rx_clock_rate);
		drv_l1_spi_clk_set(spi_num, rx_clock_rate);
	}
	else {
		drv_l1_spi_tx_clk_get(spi_num, &tx_clock_rate);
		drv_l1_spi_clk_set(spi_num, tx_clock_rate);
	}

	// when both tx_len and rx_len not zero, doing tx here
	// when only tx_len not zero, doing tx here
	// when only rx_len not zero, doing rx here
	result = spi_transceive_dma_2(spi_num);

	if (result == STATUS_OK && tx_len != 0 && rx_len != 0)
	{
		drv_l1_spi_rx_clk_get(spi_num, &rx_clock_rate);
		drv_l1_spi_clk_set(spi_num, rx_clock_rate);

		// when both tx_len and rx_len not zero, doing rx here
		pSpi->tx_buf = tx_data;
		pSpi->rx_buf = rx_data;
		pSpi->len = 0;
		pSpi->rcv_len = rx_len;
		pSpi->tx_count = 0;
		pSpi->rx_count = 0;
		result = spi_transceive_dma_2(spi_num);
	}

	if (pSpi->cs_by_external == TRUE) { /* chip select by internal */
		#if (_DRV_L1_SPI_SW_CS == 0)
		gpio_write_io(pSpi->cs_gpio_pin, (!pSpi->cs_active)&0x1);
        #endif
	}

    drv_l1_spi_pha_pol_set(spi_num, PHA0_POL0); // restore back

	spi_usage_free(pSpi);

	if (result != STATUS_OK)
	{
		DBG_PRINT("drv_l1_spi_transceive_dma_v2 fail %d\r\n", result);
	}

	return result;
}
#endif

/**
 * @brief   master mode transmit and receive data by cpu
 * @param   spi_num: spi device number
 * @param   tx_data: tx data buffer
 * @param   tx_len: tx data size
 * @param   rx_data: rx data buffer
 * @param   rx_len: rx data size
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_transceive_cpu(INT8U spi_num, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len)
{
	INT32S result;
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	if(pSpi == 0) {
		return STATUS_FAIL;
	}

	result = spi_usage_get(pSpi);
	if (result == SPI_BUSY) {
		return SPI_BUSY;
	}

	pSpi->tx_buf = tx_data;
	pSpi->rx_buf = rx_data;
	pSpi->len = tx_len;
	pSpi->rcv_len = rx_len;
	pSpi->tx_count = 0;
	pSpi->rx_count = 0;
	if (pSpi->cs_by_external == TRUE) { /* chip select by internal */
		gpio_write_io(pSpi->cs_gpio_pin, pSpi->cs_active);
	}
	result = spi_transceive_polling(spi_num);
	if (pSpi->cs_by_external == TRUE) { /* chip select by internal */
		gpio_write_io(pSpi->cs_gpio_pin, (!pSpi->cs_active)&0x1);
	}
	drv_l1_spi_pha_pol_set(spi_num, PHA0_POL0); // restore back
	spi_usage_free(pSpi);
	return result;
}

/**
 * @brief   master mode transmit and receive data by cpu
 * @param   spi_num: spi device number
 * @param   tx_data: tx data buffer
 * @param   tx_len: tx data size
 * @param   rx_data: rx data buffer
 * @param   rx_len: rx data size
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
 // use by spi_nand
INT32S drv_l1_spi_transceive_cpu_v2(INT8U spi_num, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len)
{
	INT32S result;
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	if(pSpi == 0) {
		return STATUS_FAIL;
	}

	result = spi_usage_get(pSpi);
	if (result == SPI_BUSY) {
		return SPI_BUSY;
	}

	pSpi->tx_buf = tx_data;
	pSpi->rx_buf = rx_data;
	pSpi->len = tx_len;
	pSpi->rcv_len = rx_len;
	pSpi->tx_count = 0;
	pSpi->rx_count = 0;

    #if (_DRV_L1_SPI_SW_CS == 0)
	if (pSpi->cs_by_external == TRUE) { /* chip select by internal */
		gpio_write_io(pSpi->cs_gpio_pin, pSpi->cs_active);
	}
	#endif

	result = spi_transceive_polling_2(spi_num);

	#if (_DRV_L1_SPI_SW_CS == 0)
	if (pSpi->cs_by_external == TRUE) { /* chip select by internal */
		gpio_write_io(pSpi->cs_gpio_pin, (!pSpi->cs_active)&0x1);
	}
    #endif

    drv_l1_spi_pha_pol_set(spi_num, PHA0_POL0); // restore back

	spi_usage_free(pSpi);

	if (result != STATUS_OK)
	{
		DBG_PRINT("drv_l1_spi_transceive_cpu_v2 fail %d\r\n", result);
	}

	return result;
}

/**
 * @brief   spi slave mode start receive data
 * @param   spi_num: spi device number
 * @param   dma_en: dma mode use.
 * @param   rx_data: receive data buffer.
 * @param   rx_len: receive data size.
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_slave_mode_trans(INT8U spi_num, INT8U dma_en, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len)
{
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num);
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}

	pSpi->tx_buf = tx_data;
	pSpi->rx_buf = rx_data;
	pSpi->len = tx_len;
	pSpi->rcv_len = rx_len;
	pSpi->tx_count = 0;
	pSpi->rx_count = 0;
	pSpi->slave_dma_en = dma_en;

	/* slave mode enable */
	pSpiReg->CTRL |= SPI_CTRL_SLVE;
	pSpiReg->CTRL |= SPI_CTRL_RST;

	if(dma_en) {
		/* fifo size = 0 in dma mode */
		drv_l1_spi_txrx_level_set(spi_num, 0, 0);

		if((pSpi->tx_buf == 0) || (pSpi->len == 0)) {
			// only slave rx mode
			if(spi_slave_transceive_dma(spi_num) < 0) {
				return STATUS_FAIL;
			}
		} else {
			// slave rx and tx mode
			if(spi_transceive_dma(spi_num) < 0) {
				return STATUS_FAIL;
			}
		}
	} else {
	#if SPI_TRANS_INT == 1
		if((rx_len % 7) == 0) {
			drv_l1_spi_txrx_level_set(spi_num, 0, 6);
		} else if((rx_len % 6) == 0) {
			drv_l1_spi_txrx_level_set(spi_num, 0, 5);
		} else if((rx_len % 5) == 0) {
			drv_l1_spi_txrx_level_set(spi_num, 0, 4);
		} else if((rx_len % 4) == 0) {
			drv_l1_spi_txrx_level_set(spi_num, 0, 3);
		} else if((rx_len % 3) == 0) {
			drv_l1_spi_txrx_level_set(spi_num, 0, 2);
		} else if((rx_len % 2) == 0) {
			drv_l1_spi_txrx_level_set(spi_num, 0, 1);
		} else {
			drv_l1_spi_txrx_level_set(spi_num, 0, 0);
		}

		/* Enable SPI receive interrupt */
		spi_rx_int_set(pSpiReg, SPI_ENABLED);

		/* wait receive done */
		while(pSpi->rx_count < pSpi->rcv_len) {
			spi_msec_wait(1);
		}

		/* Disable SPI receive interrupt */
		spi_rx_int_set(pSpiReg, SPI_DISABLED);

	#else
		drv_l1_spi_txrx_level_set(spi_num, 0, 0);

		/* polling receice fifo */
		if(pSpi->tx_buf && pSpi->len) {
			pSpiReg->TX_DATA = pSpi->tx_buf[pSpi->tx_count++];
		}

		while(pSpi->rx_count < pSpi->rcv_len) {
			if(pSpiReg->RX_STATUS & 0x0F) {
				pSpi->rx_buf[pSpi->rx_count++] = (INT8U)pSpiReg->RX_DATA;

				if(pSpi->tx_buf && pSpi->len && (pSpi->tx_count < pSpi->len)) {
					pSpiReg->TX_DATA = (INT8U)pSpi->tx_buf[pSpi->tx_count++];
				}
			} else {
				spi_msec_wait(1);
			}
		}
	#endif
	}

	/* slave mode disable */
	//pSpiReg->CTRL &= ~SPI_CTRL_SLVE;

	return STATUS_OK;
}

void spi_sw_reset(INT8U spi_num)
{
    spiReg_t *pSpiReg = spi_select(spi_num);

	/* reset the SPI */
	pSpiReg->CTRL |= SPI_CTRL_RST;
}

void spi_tx_byte(INT8U spi_num, INT8U txbyte)
{
    spiReg_t *pSpiReg = spi_select(spi_num);

	pSpiReg->TX_DATA = (INT32U)txbyte;
	while(!(pSpiReg->MISC&0x04));//(JC)wait for data recv'ed in recv-fifo
	pSpiReg->CTRL |= SPI_CTRL_RST;
}

INT8U spi_rx_byte(INT8U spi_num)
{
    spiReg_t *pSpiReg = spi_select(spi_num);

	pSpiReg->TX_DATA = 0x00;//(JC)dummy write to clock the recv'ing action
	while(!(pSpiReg->MISC&0x04));//(JC)wait for data recv'ed in recv-fifo
	return (pSpiReg->RX_DATA);
}

void spi_fasp_apb_set(INT8U spi_num)
{
    INT32U reg;
    spiReg_t *pSpiReg = spi_select(spi_num);

    reg = pSpiReg->CTRL;
    if((reg & SPI_CTRL_QPI)>>12)
        pSpiReg->MISC |= (1<<10);
}

void spi_fasp_apb_clear(INT8U spi_num)
{
    spiReg_t *pSpiReg = spi_select(spi_num);

    pSpiReg->MISC &= ~(1<<10);
}

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

extern uint32_t SystemCoreClock;
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define UART_FIQ 1

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct fifo
{
	INT8U *p_start;
	INT8U *p_end;
	INT8U *p_write;
	INT8U *p_read;
} UART_FIFO;

typedef struct uartReg_s
{
	volatile INT32U uart_data;			// 0xC0060000
	volatile INT32U uart_rx_status;		// 0xC0060004
	volatile INT32U uart_ctrl;			// 0xC0060008
	volatile INT32U uart_baud_rate;		// 0xC006000C
	volatile INT32U uart_irq_status;	// 0xC0060010
	volatile INT32U uart_fifo;			// 0xC0060014
	volatile INT32U uart_tx_delay;		// 0xC0060018
	volatile INT32U irda_buad_rate;		// 0xC006001C
	volatile INT32U irda_ctrl;			// 0xC0060020
	volatile INT32U irda_low_power;		// 0xC0060024
	volatile INT32U reserved1[4];		// 0xC0060028
	volatile INT32U uart_ext_baud_rate; // 0xC0060038
	volatile INT32U reserved2;			// 0xC006003C
	volatile INT32U rs485_ctrl;			// 0xC0060040
	volatile INT32U rs485_addr;			// 0xC0060044
	volatile INT32U rs485_rtscr;		// 0xC0060048
	volatile INT32U reserved[2];		// 0xC006004C
	volatile INT32U uart_rxto_cr;		// 0xC0060054
} uartReg_t;

SIR_IRQ_HANDLE sir_irq_handle = NULL;
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
INT32S drv_l1_uart_sw_fifo_init(INT8U dev_idx);
INT32S drv_l1_uart_sw_fifo_get(INT8U dev_idx, INT8U *data);
INT32S drv_l1_uart_sw_fifo_put(INT8U dev_idx, INT8U data);
void drv_l1_uart_set_ctrl(INT8U dev_idx, INT32U val);
INT32U drv_l1_uart_get_ctrl(INT8U dev_idx);
void drv_l1_uart_buad_rate_set(INT8U dev_idx, INT32U bps);
void drv_l1_uart_fifo_ctrl(INT8U dev_idx, INT8U tx_level, INT8U rx_level);
void drv_l1_uart_init(INT8U dev_idx);
void drv_l1_uart_uninit(INT8U dev_idx);
void drv_l1_uart_rx_tx_en(INT8U dev_idx, INT32U dir, INT32U enable);
void drv_l1_uart_fifo_en(INT8U dev_idx, INT32U enable);
void drv_l1_uart_data_send(INT8U dev_idx, INT8U data, INT8U wait);
INT32S drv_l1_uart_data_get(INT8U dev_idx, INT8U *data, INT8U wait);
INT32S drv_l1_uart_word_len_set(INT8U dev_idx, INT8U word_len);
INT32S drv_l1_uart_stop_bit_size_set(INT8U dev_idx, INT8U stop_size);
INT32S drv_l1_uart_parity_chk_set(INT8U dev_idx, INT8U status, INT8U parity);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
UART_FIFO uart_sw_fifo_head[UART_DEV_NO];
INT8U uart_sw_fifo_buf[UART_DEV_NO][UART_FIFO_SIZE];
INT8U uart_tx_on[UART_DEV_NO];
INT8U uart_sir_mode[UART_DEV_NO] = {UART_MODE, UART_MODE, UART_MODE};

static void (*uart_rx_user_isr[UART_DEV_NO])(INT8U, INT32U, INT8U);

static void drv_l1_uart_rx_isr_set(INT32U dev_idx, void (*user_isr)(INT8U, INT32U, INT8U))
{
	if (dev_idx < UART_DEV_NO)
	{
		uart_rx_user_isr[dev_idx] = user_isr;
	}
}

void drv_l1_uart0_rx_isr_set(void (*user_isr)(INT8U, INT32U, INT8U))
{
	drv_l1_uart_rx_isr_set(UART0_DEV, user_isr);
}

void drv_l1_uart1_rx_isr_set(void (*user_isr)(INT8U, INT32U, INT8U))
{
	drv_l1_uart_rx_isr_set(UART1_DEV, user_isr);
}

void drv_l1_uart2_rx_isr_set(void (*user_isr)(INT8U, INT32U, INT8U))
{
	drv_l1_uart_rx_isr_set(UART2_DEV, user_isr);
}

/**
 * @brief   UART select base address
 * @param   dev_idx: uart device
 * @return 	address or 0(failed)
 */
static uartReg_t *uart_select(INT8U dev_idx)
{
	if (dev_idx == UART0_DEV)
	{
		return (uartReg_t *)P_UART0_BASE;
	}
	else if (dev_idx == UART1_DEV)
	{
		return (uartReg_t *)P_UART1_BASE;
	}
	else if (dev_idx == UART2_DEV)
	{
		return (uartReg_t *)P_UART2_BASE;
	}

	return 0;
}

/**
 * @brief   UART set mode
 * @param   dev_idx: uart device
 * @param   mode: UART_MODE/SIR_MODE/FIR_MIR/RS485_MODE
 * @param   handle: IRQ handle
 * @return 	None
 */
void drv_l1_uart_set_mode(INT32U dev_idx, INT32U mode, INT32U handle)
{
	if (dev_idx < UART_DEV_NO)
	{
		uart_sir_mode[dev_idx] = mode;
		sir_irq_handle = (SIR_IRQ_HANDLE)handle;
	}
}

/**
 * @brief   uart software fifo init
 * @param   dev_idx: uart device
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_uart_sw_fifo_init(INT8U dev_idx)
{
	INT32U i;
	UART_FIFO *pFifoHead;

	for (i = 0; i < UART_FIFO_SIZE; i++)
	{
		uart_sw_fifo_buf[dev_idx][i] = 0;
	}

	pFifoHead = &uart_sw_fifo_head[dev_idx];
	pFifoHead->p_start = &uart_sw_fifo_buf[dev_idx][0];
	pFifoHead->p_end = &uart_sw_fifo_buf[dev_idx][UART_FIFO_SIZE - 1];
	pFifoHead->p_write = &uart_sw_fifo_buf[dev_idx][0];
	pFifoHead->p_read = &uart_sw_fifo_buf[dev_idx][0];
	return 0;
}

/**
 * @brief   get data by DMA
 * @param   dev_idx: uart device
 * @param   buf: data buffer
 * @param   size: DMA TX count
 * @return 	result: >=0 is success, <0 is fail
 */
INT32S drv_l1_uart_rx_dma(INT8U dev_idx, INT8U *buf, INT32U size)
{
	INT32S status;
	volatile INT8S done_rx;
	DMA_STRUCT dma_rx = {0};
	uartReg_t *pUart;
	pUart = uart_select(dev_idx);

	drv_l1_cp15_clean_dcache_range((INT32U)buf, size);
	drv_l1_cp15_invalidate_dcache_range((INT32U)buf, size);

	done_rx = C_DMA_STATUS_WAITING;
	dma_rx.s_addr = (INT32U)&pUart->uart_data;
	dma_rx.t_addr = (INT32U)buf;
	dma_rx.count = size;
	dma_rx.width = DMA_DATA_WIDTH_1BYTE;
	dma_rx.notify = &done_rx;
	dma_rx.timeout = 0;
	// dma_rx.aes = 0;
	dma_rx.trigger = 0;

	status = drv_l1_dma_transfer(&dma_rx);

	if (status != 0)
	{
		print_string("%s fail!\r\n", __FUNCTION__);
		return status;
	}
	return dma_rx.channel;
}

/**
 * @brief   get data from software fifo
 * @param   dev_idx: uart device
 * @param   data: return data value
 * @return 	result: >=0 is success, <0 is fail
 */
INT32S drv_l1_uart_sw_fifo_get(INT8U dev_idx, INT8U *data)
{
	INT8U *read_ptr;
	INT32S ret;
	UART_FIFO *pFifoHead;

	if (data == 0)
	{
		return -1;
	}
#if _OPERATING_SYSTEM == _OS_UCOS2
	osSuspend();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
	vTaskSuspendAll();
#endif
	ret = 0;
	pFifoHead = &uart_sw_fifo_head[dev_idx];

	// Check FIFO is empty
	if (pFifoHead->p_write == pFifoHead->p_read)
	{
		ret = -2;
		goto __exit;
	}

	// Only read p_read once
	read_ptr = pFifoHead->p_read;
	*data = *read_ptr++;
	if (read_ptr > pFifoHead->p_end)
	{
		read_ptr = pFifoHead->p_start;
	}

	pFifoHead->p_read = read_ptr;
__exit:
#if _OPERATING_SYSTEM == _OS_UCOS2
	osResume();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
	xTaskResumeAll();
#endif
	return ret;
}

/**
 * @brief   RS485 get data
 * @param   dev_idx: uart device
 * @param   data: return data value
 * @return 	none
 */
INT32S drv_l1_uart_sw_fifo_get_rs485(INT8U dev_idx, INT32U *data)
{
	INT8U addr;
	INT32U temp, dummy;
	INT32S ret = 0;
	uartReg_t *pUart;

	pUart = uart_select(dev_idx);
	addr = pUart->rs485_addr;
	temp = 0x200 | addr;
	// Check RxFIFO if Empty
	while ((pUart->uart_irq_status & 0x10) == 0x10)
		;
	while ((pUart->uart_irq_status & 0x8) == 0x8)
		;

	if ((pUart->rs485_ctrl & 0x3) == 1)
	{
		dummy = pUart->uart_data;
		print_string("dummy=0x%x\r\n", dummy);
		*data = 0xFF;
		return ret;
	}
	*data = pUart->uart_data;
	if ((*data >> 9) == 1 && (*data != temp))
	{
		pUart->rs485_ctrl &= ~(0x40);
	}
	print_string("R_UART_STATUS: %x, RS485 CR: %x\r\n", pUart->uart_irq_status, pUart->rs485_ctrl);
	print_string("UART RX DATA: data %x\r\n", *data);
	return ret;
}

/**
 * @brief   put data to software fifo
 * @param   dev_idx: uart device
 * @param   data: data value
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_uart_sw_fifo_put(INT8U dev_idx, INT8U data)
{
	INT8U *read_ptr, *write_ptr;
	UART_FIFO *pFifoHead;

	pFifoHead = &uart_sw_fifo_head[dev_idx];

	read_ptr = pFifoHead->p_read - 1;
	if (read_ptr < pFifoHead->p_start)
	{
		read_ptr = pFifoHead->p_end;
	}

	// check FIFO is full
	write_ptr = pFifoHead->p_write;
	if (write_ptr == read_ptr)
	{
		return -2;
	}

	// only write p_write once
	*write_ptr++ = data;
	if (write_ptr > pFifoHead->p_end)
	{
		write_ptr = pFifoHead->p_start;
	}

	pFifoHead->p_write = write_ptr;
	return 0;
}

/**
 * @brief   uart interrupt interrupt service
 * @param   dev_idx: uart device
 * @return 	none
 */
void UART_IRQHandler(INT8U dev_idx)
{
	INT32U temp;
	uartReg_t *pUart;
	pUart = uart_select(dev_idx);

	if (uart_sir_mode[dev_idx] == UART_MODE)
	{
		if (pUart->uart_irq_status & C_UART_STATUS_RX_INT)
		{
			while (!(pUart->uart_irq_status & C_UART_STATUS_RX_EMPTY))
			{
				INT8U data = pUart->uart_data;
				drv_l1_uart_sw_fifo_put(dev_idx, data);
				if (uart_rx_user_isr[dev_idx] != NULL)
					(*uart_rx_user_isr[dev_idx])(dev_idx, pUart->uart_irq_status, data);
			}
		}
		else if (pUart->uart_irq_status & C_UART_STATUS_RXTO_INT)
		{
			if (uart_rx_user_isr[dev_idx] != NULL)
				(*uart_rx_user_isr[dev_idx])(dev_idx, C_UART_STATUS_RXTO_INT, 0);
		}
		else
		{
			DBG_PRINT("%d\r\n", pUart->uart_irq_status);
		}
	}
	else if (uart_sir_mode[dev_idx] == RS485_MODE)
	{
		pUart->uart_irq_status |= (1 << 11); // clear interrupt
		if ((pUart->rs485_ctrl & 0x3) == 1)
		{
			temp = pUart->uart_data;
			print_string("value %x\r\n", temp);
		}
	}
}

void UART0_IRQHandler(void)
{
	if (uart_sir_mode[UART0_DEV] == UART_MODE)
		UART_IRQHandler(UART0_DEV);
	else if (uart_sir_mode[UART0_DEV] == SIR_MODE)
	{
		if (sir_irq_handle)
			sir_irq_handle(UART0_DEV);
	}
	else if (uart_sir_mode[UART0_DEV] == RS485_MODE)
		UART_IRQHandler(UART0_DEV);

	if (uart_select(UART0_DEV)->uart_irq_status & C_UART_STATUS_RXTO_INT)
	{
		drv_l1_uart0_rx_to_restart();
	}
}

void UART1_IRQHandler(void)
{
	if (uart_sir_mode[UART1_DEV] == UART_MODE)
		UART_IRQHandler(UART1_DEV);
	else if (uart_sir_mode[UART1_DEV] == SIR_MODE)
	{
		if (sir_irq_handle)
			sir_irq_handle(UART1_DEV);
	}
	else if (uart_sir_mode[UART1_DEV] == RS485_MODE)
		UART_IRQHandler(UART1_DEV);

	if (uart_select(UART1_DEV)->uart_irq_status & C_UART_STATUS_RXTO_INT)
	{
		drv_l1_uart1_rx_to_restart();
	}
}

void UART2_IRQHandler(void)
{
	if (uart_sir_mode[UART2_DEV] == UART_MODE)
		UART_IRQHandler(UART2_DEV);
	else if (uart_sir_mode[UART2_DEV] == SIR_MODE)
	{
		if (sir_irq_handle)
		{
			sir_irq_handle(UART2_DEV);
		}
	}
	else if (uart_sir_mode[UART2_DEV] == RS485_MODE)
		UART_IRQHandler(UART2_DEV);

	if (uart_select(UART2_DEV)->uart_irq_status & C_UART_STATUS_RXTO_INT)
	{
		drv_l1_uart2_rx_to_restart();
	}
}

/**
 * @brief   set uart control register
 * @param   dev_idx: uart device
 * @param   val: register value
 * @return 	none
 */
void drv_l1_uart_set_ctrl(INT8U dev_idx, INT32U val)
{
	// Bit15: Receive interrupt enable
	// Bit14: Transmit interrupt enable
	// Bit13: Receive timeout interrupt enable
	// Bit12: UART enable
	// Bit11: Modem status interrupt enable
	// Bit10: Self loop test enable(used in IrDA mode only)
	// Bit9-7: Reservede
	// Bit6-5: Word length, 00=5 bits, 01=6 bits, 10=7 bits, 11=8 bits
	// Bit4: FIFO enable
	// Bit3: Stop bit size, 0=1 bit, 1=2 bits
	// Bit2: Parity, 0=odd parity, 1=even parity
	// Bit1: Parity enable, 0=disable, 1=enable
	// Bit0: Send break, 0=normal, 1=send break signal
	uartReg_t *pUart = uart_select(dev_idx);
	if (pUart == 0)
	{
		return;
	}

	pUart->uart_ctrl = val & 0xFC7F;
}

/**
 * @brief   get uart control register value
 * @param   dev_idx: uart device
 * @return 	register value
 */
INT32U drv_l1_uart_get_ctrl(INT8U dev_idx)
{
	uartReg_t *pUart = uart_select(dev_idx);
	if (pUart == 0)
	{
		return 0;
	}

	return pUart->uart_ctrl;
}

/**
 * @brief   set uart baud rate
 * @param   dev_idx: uart device
 * @param   bps: baut rate speed
 * @return 	none
 */
void drv_l1_uart_buad_rate_set(INT8U dev_idx, INT32U bps)
{
	INT32U tmp;
	uartReg_t *pUart = uart_select(dev_idx);
	if (pUart == 0)
	{
		return;
	}
	tmp = SystemCoreClock / bps;
	pUart->uart_baud_rate = tmp & 0xFFFF;
	pUart->uart_ext_baud_rate = (tmp & 0x70000) >> 16;
}

/**
 * @brief   get uart baud rate
 * @param   dev_idx: uart device
 * @return  bps: buad rate speed
 */
INT32U drv_l1_uart_buad_rate_get(INT8U dev_idx)
{
	INT32U tmp;
	uartReg_t *pUart = uart_select(dev_idx);
	if (pUart == 0)
	{
		return STATUS_FAIL;
	}
	tmp = (pUart->uart_ext_baud_rate & 0x7) << 16 | (pUart->uart_baud_rate & 0xFFFF);
	return tmp;
}

/**
 * @brief   set uart fifo threshold
 * @param   dev_idx: uart device
 * @param   tx_level: tx fifo threshold
 * @param   rx_level: rx fifo threshold
 * @return 	none
 */
void drv_l1_uart_fifo_ctrl(INT8U dev_idx, INT8U tx_level, INT8U rx_level)
{
	INT32U reg;
	uartReg_t *pUart = uart_select(dev_idx);
	if (pUart == 0)
	{
		return;
	}

	reg = ((INT32U)tx_level & 0x7) << 12;
	reg |= ((INT32U)rx_level & 0x7) << 4;
	pUart->uart_fifo = reg;
}

/**
 * @brief   set uart initialize.
 * @param   dev_idx: uart device
 * @return 	none
 */
void drv_l1_uart_init(INT8U dev_idx)
{
	if (dev_idx >= UART_DEV_NO)
	{
		return;
	}

	// disable uart
	drv_l1_uart_set_ctrl(dev_idx, 0);

	uart_tx_on[dev_idx] = 0;
	drv_l1_uart_sw_fifo_init(dev_idx);

	// 8 bits, 1 stop bit, no parity check, RX disabled
	drv_l1_uart_set_ctrl(dev_idx, C_UART_CTRL_UART_ENABLE | C_UART_CTRL_WORD_8BIT | C_UART_CTRL_RX_INT | C_UART_CTRL_RTIF_INT);

	if (dev_idx == 0)
	{
		vic_irq_register(VIC_UART0, UART0_IRQHandler); // Non vector interrupt register
		vic_irq_enable(VIC_UART0);

#if UART_FIQ == 1
		vic_fiq_enable(VIC_UART0);
#endif
	}
	else if (dev_idx == 1)
	{

		vic_irq_register(VIC_UART1, UART1_IRQHandler); // Non vector interrupt register
		vic_irq_enable(VIC_UART1);
#if UART_FIQ == 1
		vic_fiq_enable(VIC_UART1);
#endif
	}
	else if (dev_idx == 2)
	{

		vic_irq_register(VIC_UART2, UART2_IRQHandler); // Non vector interrupt register
		vic_irq_enable(VIC_UART2);
#if UART_FIQ == 1
		vic_fiq_enable(VIC_UART2);
#endif
	}
}

/**
 * @brief   set uart uninitialize.
 * @param   dev_idx: uart device
 * @return 	none
 */
void drv_l1_uart_uninit(INT8U dev_idx)
{
	drv_l1_uart_set_ctrl(dev_idx, 0);
	if (dev_idx == 0)
	{
		vic_irq_disable(VIC_UART0);
#if UART_FIQ == 1
		vic_fiq_disable(VIC_UART0);
#endif
	}
	else if (dev_idx == 1)
	{
		vic_irq_disable(VIC_UART1);
#if UART_FIQ == 1
		vic_fiq_disable(VIC_UART1);
#endif
	}
	else if (dev_idx == 2)
	{
		vic_irq_disable(VIC_UART2);
#if UART_FIQ == 1
		vic_fiq_disable(VIC_UART2);
#endif
	}
}

/**
 * @brief   set uart direction enable
 * @param   dev_idx: uart device
 * @param   dir: direction, UART_RX or UART_TX
 * @param   enable: enable bit
 * @return 	none
 */
void drv_l1_uart_rx_tx_en(INT8U dev_idx, INT32U dir, INT32U enable)
{
	INT32U reg;
	uartReg_t *pUart = uart_select(dev_idx);
	if (pUart == 0)
	{
		return;
	}
#if _OPERATING_SYSTEM == _OS_UCOS2
	osSuspend();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
	vTaskSuspendAll();
#endif
	if (dir == UART_RX)
	{ // RX
		reg = pUart->uart_ctrl;
		if (enable == UART_ENABLE)
		{
			reg |= C_UART_CTRL_RX_INT;
		}
		else
		{
			reg &= ~C_UART_CTRL_RX_INT;
		}

		pUart->uart_ctrl = reg;
	}
	else
	{ // TX
		if (enable == UART_ENABLE)
		{
			uart_tx_on[dev_idx] = 1;
		}
		else
		{
			uart_tx_on[dev_idx] = 0;
		}
	}
#if _OPERATING_SYSTEM == _OS_UCOS2
	osResume();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
	xTaskResumeAll();
#endif
}

/**
 * @brief   set uart fifo enable
 * @param   dev_idx: uart device
 * @param   enable: enable bit
 * @return 	none
 */
void drv_l1_uart_fifo_en(INT8U dev_idx, INT32U enable)
{
	INT32U reg;
	uartReg_t *pUart = uart_select(dev_idx);
	if (pUart == 0)
	{
		return;
	}
#if _OPERATING_SYSTEM == _OS_UCOS2
	osSuspend();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
	vTaskSuspendAll();
#endif
	reg = pUart->uart_ctrl;
	if (enable == UART_ENABLE)
	{
		reg |= C_UART_CTRL_FIFO_ENABLE;
	}
	else
	{
		reg &= ~C_UART_CTRL_FIFO_ENABLE;
	}

	pUart->uart_ctrl = reg;
#if _OPERATING_SYSTEM == _OS_UCOS2
	osResume();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
	xTaskResumeAll();
#endif
}

/**
 * @brief   send uart data
 * @param   dev_idx: uart device
 * @param   data: data value
 * @param   wait: wait data transfer
 * @return 	none
 */
void drv_l1_uart_data_send(INT8U dev_idx, INT8U data, INT8U wait)
{
	uartReg_t *pUart = uart_select(dev_idx);
	if (pUart == 0)
	{
		return;
	}

	if (uart_tx_on[dev_idx] == 0)
	{
		return;
	}

	// Wait until FIFO is not full
	if (wait)
	{
		while (pUart->uart_irq_status & C_UART_STATUS_TX_FULL)
			;
	}

	// Send our data now
	pUart->uart_data = data;
}

/**
 * @brief   get uart data
 * @param   dev_idx: uart device
 * @param   data: get data value
 * @param   wait: wait data get
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart_data_get(INT8U dev_idx, INT8U *data, INT8U wait)
{
	if (dev_idx >= UART_DEV_NO)
	{
		return STATUS_FAIL;
	}

	while (drv_l1_uart_sw_fifo_get(dev_idx, data))
	{
		// Queue is empty and the caller doesn't want to wait
		if (!wait)
		{
			return STATUS_FAIL;
		}
	}

	return STATUS_OK;
}

/**
 * @brief   get RS485 data
 * @param   dev_idx: uart device
 * @param   data: get data value
 * @param   wait: wait data get
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart_data_get_rs485(INT8U dev_idx, INT32U *data, INT8U wait)
{
	if (dev_idx >= UART_DEV_NO)
	{
		return STATUS_FAIL;
	}

	while (drv_l1_uart_sw_fifo_get_rs485(dev_idx, data))
	{
		if (!wait)
		{
			return STATUS_FAIL;
		}
	}

	return STATUS_OK;
}

/**
 * @brief   set uart word length
 * @param   dev_idx: uart device
 * @param   data: C_UART_CTRL_WORD_5BIT ~ C_UART_CTRL_WORD_8BIT
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart_word_len_set(INT8U dev_idx, INT8U word_len)
{
	INT32U reg;
	uartReg_t *pUart = uart_select(dev_idx);
	if (pUart == 0)
	{
		return STATUS_FAIL;
	}

	reg = pUart->uart_ctrl;
	reg &= ~(3 << 5);
	switch (word_len)
	{
	case WORD_LEN_5:
		reg |= C_UART_CTRL_WORD_5BIT;
		break;
	case WORD_LEN_6:
		reg |= C_UART_CTRL_WORD_6BIT;
		break;
	case WORD_LEN_7:
		reg |= C_UART_CTRL_WORD_7BIT;
		break;
	case WORD_LEN_8:
		reg |= C_UART_CTRL_WORD_8BIT;
		break;
	default:
		return STATUS_FAIL;
	}

	pUart->uart_ctrl = reg;
	return STATUS_OK;
}

/**
 * @brief   set uart stop bit
 * @param   dev_idx: uart device
 * @param   stop_size: STOP_SIZE_1 or STOP_SIZE_2
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart_stop_bit_size_set(INT8U dev_idx, INT8U stop_size)
{
	INT32U reg;
	uartReg_t *pUart = uart_select(dev_idx);
	if (pUart == 0)
	{
		return STATUS_FAIL;
	}

	reg = pUart->uart_ctrl;
	reg &= ~(1 << 3);
	if (stop_size == STOP_SIZE_2)
	{
		reg |= C_UART_CTRL_2STOP_BIT;
	}

	pUart->uart_ctrl = reg;
	return STATUS_OK;
}

/**
 * @brief   set uart parity
 * @param   dev_idx: uart device
 * @param   status: UART_DISABLE or UART_ENABLE
 * @param   parity: PARITY_EVEN or PARITY_ODD
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart_parity_chk_set(INT8U dev_idx, INT8U status, INT8U parity)
{
	INT32U reg;
	uartReg_t *pUart = uart_select(dev_idx);
	if (pUart == 0)
	{
		return STATUS_FAIL;
	}

	reg = pUart->uart_ctrl;
	if (status == UART_DISABLE)
	{
		reg &= ~(1 << 1);
	}
	else
	{
		reg |= C_UART_CTRL_PARITY_EN;
		reg &= ~(1 << 2);
		if (parity == PARITY_EVEN)
		{
			reg |= C_UART_CTRL_EVEN_PARITY;
		}
	}

	pUart->uart_ctrl = reg;
	return STATUS_OK;
}

/**
 * @brief   set uart0 initialize.
 * @param   None
 * @return 	none
 */
void drv_l1_uart0_init(void)
{
	drv_l1_clock_set_system_clk_en(CLK_EN2_UART0, 1);
	drv_l1_clock_set_system_clk_en(CLK_EN2_UART0_REG_BANK, 1);
	drv_l1_uart_init(UART0_DEV);
}

/**
 * @brief   set RS485 initialize.
 * @param   mode: Normal Mode/ Address Byte Detection mode/ Address Detection mode
 * @param   automode: AUTO_MODE/MANUAL_TX/MANUAL_RX
 * @param   add:RS485 address
 * @return 	none
 */
void drv_l1_uart0_rs485_init(INT8U mode, INT8U automode, INT32U add)
{
	uart_sir_mode[UART0_DEV] = RS485_MODE;
	R_UART0_RS485_CR |= (C_UART_RS485_ENABLE | C_UART_RS485_ADDR_INTEN); // enable RS485
	if (mode == MODE0)
	{
		R_UART0_RS485_CR &= ~(0x3);
	}
	else if (mode == MODE1)
	{
		R_UART0_RS485_CR |= (0x1);
	}
	else
	{
		R_UART0_RS485_CR |= (0x2);
	}
	if (automode == AUTO_MODE)
	{
		R_UART0_RS485_RTSCR |= (C_UART_RS485_AUTOMODE);
	}
	else if (automode == MANUAL_TX)
	{
		R_UART0_RS485_RTSCR &= ~(C_UART_RS485_AUTOMODE);
		R_UART0_RS485_RTSCR |= C_UART_RS485_RTS_TX;
	}
	else
	{ // RX
		R_UART0_RS485_RTSCR &= ~(C_UART_RS485_AUTOMODE);
		R_UART0_RS485_RTSCR &= ~(C_UART_RS485_RTS_TX);
	}
	R_UART0_RS485_CR &= ~(C_UART_RS485_RXEN);
	R_UART0_CTRL = (0x10E2); // parity & fifo enable
	R_UART0_RS485_ADDR = add;
}

/**
 * @brief   set uart0 baud rate.
 * @param   bps: buad rate speed
 * @return 	none
 */
void drv_l1_uart0_buad_rate_set(INT32U bps)
{
	drv_l1_uart_buad_rate_set(UART0_DEV, bps);
}

/**
 * @brief   set uart0 enable RX.
 * @param   none
 * @return 	none
 */
void drv_l1_uart0_rx_enable(void)
{
	drv_l1_uart_rx_tx_en(UART0_DEV, UART_RX, UART_ENABLE);
}

/**
 * @brief   set uart0 disable RX.
 * @param   none
 * @return 	none
 */
void drv_l1_uart0_rx_disable(void)
{
	drv_l1_uart_rx_tx_en(UART0_DEV, UART_RX, UART_DISABLE);
}

/**
 * @brief   set uart0 enable TX.
 * @param   none
 * @return 	none
 */
void drv_l1_uart0_tx_enable(void)
{
	drv_l1_uart_rx_tx_en(UART0_DEV, UART_TX, UART_ENABLE);
}

/**
 * @brief   set uart0 disable TX.
 * @param   none
 * @return 	none
 */
void drv_l1_uart0_tx_disable(void)
{
	drv_l1_uart_rx_tx_en(UART0_DEV, UART_TX, UART_DISABLE);
}

/**
 * @brief   set uart0 enable fifo.
 * @param   none
 * @return 	none
 */
void drv_l1_uart0_fifo_enable(void)
{
	drv_l1_uart_fifo_en(UART0_DEV, UART_ENABLE);
}

/**
 * @brief   set uart0 disable fifo.
 * @param   none
 * @return 	none
 */
void drv_l1_uart0_fifo_disable(void)
{
	drv_l1_uart_fifo_en(UART0_DEV, UART_DISABLE);
}

/**
 * @brief   send uart0 data
 * @param   data: data value
 * @param   wait: wait data transfer
 * @return 	none
 */
void drv_l1_uart0_data_send(INT8U data, INT8U wait)
{
	drv_l1_uart_data_send(UART0_DEV, data, wait);
}

/**
 * @brief   get uart0 data
 * @param   data: get data value
 * @param   wait: wait data get
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart0_data_get(INT8U *data, INT8U wait)
{
	return drv_l1_uart_data_get(UART0_DEV, data, wait);
}

/**
 * @brief   get RS485 data
 * @param   data: get data value
 * @param   wait: wait data get
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart0_data_get_rs485(INT32U *data, INT8U wait)
{
	return drv_l1_uart_data_get_rs485(UART0_DEV, data, wait);
}

/**
 * @brief   set uart0 word length
 * @param   data: C_UART_CTRL_WORD_5BIT ~ C_UART_CTRL_WORD_8BIT
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart0_word_len_set(INT8U word_len)
{
	return drv_l1_uart_word_len_set(UART0_DEV, word_len);
}

/**
 * @brief   set uart0 stop bit
 * @param   stop_size: STOP_SIZE_1 or STOP_SIZE_2
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart0_stop_bit_size_set(INT8U stop_size)
{
	return drv_l1_uart_stop_bit_size_set(UART0_DEV, stop_size);
}

/**
 * @brief   set uart0 parity
 * @param   status: UART_DISABLE or UART_ENABLE
 * @param   parity: PARITY_EVEN or PARITY_ODD
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart0_parity_chk_set(INT8U status, INT8U parity)
{
	return drv_l1_uart_parity_chk_set(UART0_DEV, status, parity);
}

/**
 * @brief   Initial uart0 rx timeout
 * @param   lsb: timeout counter lsb
 * @param   msb: timeout counter msb
 * @return 	None
 */
void drv_l1_uart0_rx_to_init(INT8U idle_level, INT32U lsb, INT32U msb)
{
	R_UART0_RXTO_LSB = lsb;
	R_UART0_RXTO_MSB = msb;
	if (idle_level)
	{
		R_UART0_RXTO_CR |= (0x1 | (1 << 3));
	}
	else
	{
		R_UART0_RXTO_CR |= 0x1;
		R_UART0_RXTO_CR &= ~(1 << 3);
	}
}

/**
 * @brief   Restart uart0 rx timeout
 * @param   None
 * @return 	None
 */
void drv_l1_uart0_rx_to_restart(void)
{
	R_UART0_STATUS |= (1 << 10);
	R_UART0_RXTO_CR &= ~(0x1);
	R_UART0_RXTO_CR |= 0x1;
}

/**
 * @brief   set uart1 initialize.
 * @param   None
 * @return 	none
 */
void drv_l1_uart1_init(void)
{
	drv_l1_clock_set_system_clk_en(CLK_EN2_UART1, 1);
	drv_l1_clock_set_system_clk_en(CLK_EN2_UART1_REG_BANK, 1);
	drv_l1_uart_init(UART1_DEV);
}

/**
 * @brief   set RS485 initialize.
 * @param   mode: Normal Mode/ Address Byte Detection mode/ Address Detection mode
 * @param   automode: AUTO_MODE/MANUAL_TX/MANUAL_RX
 * @param   add:RS485 address
 * @return 	none
 */
void drv_l1_uart1_rs485_init(INT8U mode, INT8U automode, INT32U add)
{
	uart_sir_mode[UART1_DEV] = RS485_MODE;
	R_UART1_RS485_CR |= (C_UART_RS485_ENABLE | C_UART_RS485_ADDR_INTEN); // enable RS485
	if (mode == MODE0)
	{
		R_UART1_RS485_CR &= ~(0x3);
	}
	else if (mode == MODE1)
	{
		R_UART1_RS485_CR |= (0x1);
	}
	else
	{
		R_UART1_RS485_CR |= (0x2);
	}
	if (automode == AUTO_MODE)
	{
		R_UART1_RS485_RTSCR |= (C_UART_RS485_AUTOMODE);
	}
	else if (automode == MANUAL_TX)
	{
		R_UART1_RS485_RTSCR &= ~(C_UART_RS485_AUTOMODE);
		R_UART1_RS485_RTSCR |= C_UART_RS485_RTS_TX;
	}
	else
	{ // RX
		R_UART1_RS485_RTSCR &= ~(C_UART_RS485_AUTOMODE);
		R_UART1_RS485_RTSCR &= ~(C_UART_RS485_RTS_TX);
	}
	R_UART1_RS485_CR &= ~(C_UART_RS485_RXEN);
	R_UART1_CTRL = (0x10E2); // parity & fifo enable
	R_UART1_RS485_ADDR = add;
}

/**
 * @brief   set uart1 baud rate.
 * @param   bps: buad rate speed
 * @return 	none
 */
void drv_l1_uart1_buad_rate_set(INT32U bps)
{
	drv_l1_uart_buad_rate_set(UART1_DEV, bps);
}

/**
 * @brief   set uart1 enable RX.
 * @param   none
 * @return 	none
 */
void drv_l1_uart1_rx_enable(void)
{
	drv_l1_uart_rx_tx_en(UART1_DEV, UART_RX, UART_ENABLE);
}

/**
 * @brief   set uart1 disable RX.
 * @param   none
 * @return 	none
 */
void drv_l1_uart1_rx_disable(void)
{
	drv_l1_uart_rx_tx_en(UART1_DEV, UART_RX, UART_DISABLE);
}

/**
 * @brief   set uart1 enable TX.
 * @param   none
 * @return 	none
 */
void drv_l1_uart1_tx_enable(void)
{
	drv_l1_uart_rx_tx_en(UART1_DEV, UART_TX, UART_ENABLE);
}

/**
 * @brief   set uart1 disable TX.
 * @param   none
 * @return 	none
 */
void drv_l1_uart1_tx_disable(void)
{
	drv_l1_uart_rx_tx_en(UART1_DEV, UART_TX, UART_DISABLE);
}

/**
 * @brief   set uart1 enable fifo.
 * @param   none
 * @return 	none
 */
void drv_l1_uart1_fifo_enable(void)
{
	drv_l1_uart_fifo_en(UART1_DEV, UART_ENABLE);
}

/**
 * @brief   set uart1 disable fifo.
 * @param   none
 * @return 	none
 */
void drv_l1_uart1_fifo_disable(void)
{
	drv_l1_uart_fifo_en(UART1_DEV, UART_DISABLE);
}

/**
 * @brief   send uart1 data
 * @param   data: data value
 * @param   wait: wait data transfer
 * @return 	none
 */
void drv_l1_uart1_data_send(INT8U data, INT8U wait)
{
	drv_l1_uart_data_send(UART1_DEV, data, wait);
}

/**
 * @brief   get uart1 data
 * @param   data: get data value
 * @param   wait: wait data get
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart1_data_get(INT8U *data, INT8U wait)
{
	return drv_l1_uart_data_get(UART1_DEV, data, wait);
}

/**
 * @brief   get RS485 data
 * @param   data: get data value
 * @param   wait: wait data get
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart1_data_get_rs485(INT32U *data, INT8U wait)
{
	return drv_l1_uart_data_get_rs485(UART1_DEV, data, wait);
}

/**
 * @brief   set uart1 word length
 * @param   data: C_UART_CTRL_WORD_5BIT ~ C_UART_CTRL_WORD_8BIT
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart1_word_len_set(INT8U word_len)
{
	return drv_l1_uart_word_len_set(UART1_DEV, word_len);
}

/**
 * @brief   set uart1 stop bit
 * @param   stop_size: STOP_SIZE_1 or STOP_SIZE_2
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart1_stop_bit_size_set(INT8U stop_size)
{
	return drv_l1_uart_stop_bit_size_set(UART1_DEV, stop_size);
}

/**
 * @brief   set uart1 parity
 * @param   status: UART_DISABLE or UART_ENABLE
 * @param   parity: PARITY_EVEN or PARITY_ODD
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart1_parity_chk_set(INT8U status, INT8U parity)
{
	return drv_l1_uart_parity_chk_set(UART1_DEV, status, parity);
}

/**
 * @brief   Initial uart1 rx timeout
 * @param   lsb: timeout counter lsb
 * @param   msb: timeout counter msb
 * @return 	None
 */
void drv_l1_uart1_rx_to_init(INT8U idle_level, INT32U lsb, INT32U msb)
{
	R_UART1_RXTO_LSB = lsb;
	R_UART1_RXTO_MSB = msb;
	if (idle_level)
	{
		R_UART1_RXTO_CR |= (0x1 | (1 << 3));
	}
	else
	{
		R_UART1_RXTO_CR |= 0x1;
		R_UART1_RXTO_CR &= ~(1 << 3);
	}
}

/**
 * @brief   Restart uart1 rx timeout
 * @param   None
 * @return 	None
 */
void drv_l1_uart1_rx_to_restart(void)
{
	R_UART1_STATUS |= (1 << 10);
	R_UART1_RXTO_CR &= ~(0x1);
	R_UART1_RXTO_CR |= 0x1;
}

/**
 * @brief   set uart2 initialize.
 * @param   None
 * @return 	none
 */
void drv_l1_uart2_init(void)
{
	drv_l1_clock_set_system_clk_en(CLK_EN2_UART2, 1);
	drv_l1_clock_set_system_clk_en(CLK_EN2_UART2_REG_BANK, 1);
	drv_l1_uart_init(UART2_DEV);
}

/**
 * @brief   set RS485 initialize.
 * @param   mode: Normal Mode/ Address Byte Detection mode/ Address Detection mode
 * @param   automode: AUTO_MODE/MANUAL_TX/MANUAL_RX
 * @param   add:RS485 address
 * @return 	none
 */
void drv_l1_uart2_rs485_init(INT8U mode, INT8U automode, INT32U add)
{
	uart_sir_mode[UART2_DEV] = RS485_MODE;
	R_UART2_RS485_CR |= (C_UART_RS485_ENABLE | C_UART_RS485_ADDR_INTEN); // enable RS485
	if (mode == MODE0)
	{
		R_UART2_RS485_CR &= ~(0x3);
	}
	else if (mode == MODE1)
	{
		R_UART2_RS485_CR |= (0x1);
	}
	else
	{
		R_UART2_RS485_CR |= (0x2);
	}
	if (automode == AUTO_MODE)
	{
		R_UART2_RS485_RTSCR |= (C_UART_RS485_AUTOMODE);
	}
	else if (automode == MANUAL_TX)
	{
		R_UART2_RS485_RTSCR &= ~(C_UART_RS485_AUTOMODE);
		R_UART2_RS485_RTSCR |= C_UART_RS485_RTS_TX;
	}
	else
	{ // RX
		R_UART2_RS485_RTSCR &= ~(C_UART_RS485_AUTOMODE);
		R_UART2_RS485_RTSCR &= ~(C_UART_RS485_RTS_TX);
	}
	R_UART2_RS485_CR &= ~(C_UART_RS485_RXEN);
	R_UART2_CTRL = (0x10E2); // parity & fifo enable
	R_UART2_RS485_ADDR = add;
}

/**
 * @brief   set uart2 baud rate.
 * @param   bps: buad rate speed
 * @return 	none
 */
void drv_l1_uart2_buad_rate_set(INT32U bps)
{
	drv_l1_uart_buad_rate_set(UART2_DEV, bps);
}

/**
 * @brief   set uart2 enable RX.
 * @param   none
 * @return 	none
 */
void drv_l1_uart2_rx_enable(void)
{
	drv_l1_uart_rx_tx_en(UART2_DEV, UART_RX, UART_ENABLE);
}

/**
 * @brief   set uart2 disable RX.
 * @param   none
 * @return 	none
 */
void drv_l1_uart2_rx_disable(void)
{
	drv_l1_uart_rx_tx_en(UART2_DEV, UART_RX, UART_DISABLE);
}

/**
 * @brief   set uart2 enable TX.
 * @param   none
 * @return 	none
 */
void drv_l1_uart2_tx_enable(void)
{
	drv_l1_uart_rx_tx_en(UART2_DEV, UART_TX, UART_ENABLE);
}

/**
 * @brief   set uart2 disable TX.
 * @param   none
 * @return 	none
 */
void drv_l1_uart2_tx_disable(void)
{
	drv_l1_uart_rx_tx_en(UART2_DEV, UART_TX, UART_DISABLE);
}

/**
 * @brief   set uart2 enable fifo.
 * @param   none
 * @return 	none
 */
void drv_l1_uart2_fifo_enable(void)
{
	drv_l1_uart_fifo_en(UART2_DEV, UART_ENABLE);
}

/**
 * @brief   set uart2 disable fifo.
 * @param   none
 * @return 	none
 */
void drv_l1_uart2_fifo_disable(void)
{
	drv_l1_uart_fifo_en(UART2_DEV, UART_DISABLE);
}

/**
 * @brief   send uart2 data
 * @param   data: data value
 * @param   wait: wait data transfer
 * @return 	none
 */
void drv_l1_uart2_data_send(INT8U data, INT8U wait)
{
	drv_l1_uart_data_send(UART2_DEV, data, wait);
}

/**
 * @brief   get uart2 data
 * @param   data: get data value
 * @param   wait: wait data get
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart2_data_get(INT8U *data, INT8U wait)
{
	return drv_l1_uart_data_get(UART2_DEV, data, wait);
}

/**
 * @brief   get RS485 data
 * @param   data: get data value
 * @param   wait: wait data get
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart2_data_get_rs485(INT32U *data, INT8U wait)
{
	return drv_l1_uart_data_get_rs485(UART2_DEV, data, wait);
}

/**
 * @brief   set uart2 word length
 * @param   data: C_UART_CTRL_WORD_5BIT ~ C_UART_CTRL_WORD_8BIT
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart2_word_len_set(INT8U word_len)
{
	return drv_l1_uart_word_len_set(UART2_DEV, word_len);
}

/**
 * @brief   set uart2 stop bit
 * @param   stop_size: STOP_SIZE_1 or STOP_SIZE_2
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart2_stop_bit_size_set(INT8U stop_size)
{
	return drv_l1_uart_stop_bit_size_set(UART2_DEV, stop_size);
}

/**
 * @brief   set uart2 parity
 * @param   status: UART_DISABLE or UART_ENABLE
 * @param   parity: PARITY_EVEN or PARITY_ODD
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart2_parity_chk_set(INT8U status, INT8U parity)
{
	return drv_l1_uart_parity_chk_set(UART2_DEV, status, parity);
}

/**
 * @brief   Initial uart2 rx timeout
 * @param   lsb: timeout counter lsb
 * @param   msb: timeout counter msb
 * @return 	None
 */
void drv_l1_uart2_rx_to_init(INT8U idle_level, INT32U lsb, INT32U msb)
{
	R_UART2_RXTO_LSB = lsb;
	R_UART2_RXTO_MSB = msb;
	if (idle_level)
	{
		R_UART2_RXTO_CR |= (0x1 | (1 << 3));
	}
	else
	{
		R_UART2_RXTO_CR |= 0x1;
		R_UART2_RXTO_CR &= ~(1 << 3);
	}
}

/**
 * @brief   Restart uart0 rx timeout
 * @param   None
 * @return 	None
 */
void drv_l1_uart2_rx_to_restart(void)
{
	R_UART2_STATUS |= (1 << 10);
	R_UART2_RXTO_CR &= ~(0x1);
	R_UART2_RXTO_CR |= 0x1;
}

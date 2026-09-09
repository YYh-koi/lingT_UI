#ifndef __drv_l1_UART_H__
#define __drv_l1_UART_H__
#include "typedef.h"

// UART RX error status register
#define	C_UART_ERR_FRAME		0x00000001
#define	C_UART_ERR_PARITY		0x00000002
#define	C_UART_ERR_BREAK		0x00000004
#define	C_UART_ERR_OVERRUN		0x00000008

// UART control register
#define	C_UART_CTRL_RX_INT		0x00008000
#define	C_UART_CTRL_TX_INT		0x00004000
#define	C_UART_CTRL_RTIF_INT		0x00002000
#define	C_UART_CTRL_UART_ENABLE		0x00001000
#define	C_UART_CTRL_MODEM_INT		0x00000800
#define	C_UART_CTRL_LOOPBACK		0x00000400
#define	C_UART_CTRL_WORD_8BIT		0x00000060
#define	C_UART_CTRL_WORD_7BIT		0x00000040
#define	C_UART_CTRL_WORD_6BIT		0x00000020
#define	C_UART_CTRL_WORD_5BIT		0x00000000
#define	C_UART_CTRL_FIFO_ENABLE		0x00000010
#define	C_UART_CTRL_2STOP_BIT		0x00000008
#define	C_UART_CTRL_EVEN_PARITY		0x00000004
#define	C_UART_CTRL_PARITY_EN		0x00000002
#define	C_UART_CTRL_SEND_BREAK		0x00000001

// UART interrupt pending bit and status register
#define	C_UART_STATUS_RX_INT		0x00008000
#define	C_UART_STATUS_TX_INT		0x00004000
#define	C_UART_STATUS_RTIF_INT		0x00002000
#define	C_UART_STATUS_RXTO_INT		0x00000400
#define	C_UART_STATUS_TX_EMPTY		0x00000080
#define	C_UART_STATUS_RX_FULL		0x00000040
#define	C_UART_STATUS_TX_FULL		0x00000020
#define	C_UART_STATUS_RX_EMPTY		0x00000010
#define	C_UART_STATUS_BUSY		0x00000008

// UART FIFO register
#define	C_UART_FIFO_TX_LEVEL_0		0x00000000
#define	C_UART_FIFO_TX_LEVEL_1		0x00001000
#define	C_UART_FIFO_TX_LEVEL_2		0x00002000
#define	C_UART_FIFO_TX_LEVEL_3		0x00003000
#define	C_UART_FIFO_TX_LEVEL_4		0x00004000
#define	C_UART_FIFO_TX_LEVEL_5		0x00005000
#define	C_UART_FIFO_TX_LEVEL_6		0x00006000
#define	C_UART_FIFO_TX_LEVEL_7		0x00007000
#define	C_UART_FIFO_RX_LEVEL_0		0x00000000
#define	C_UART_FIFO_RX_LEVEL_1		0x00000010
#define	C_UART_FIFO_RX_LEVEL_2		0x00000020
#define	C_UART_FIFO_RX_LEVEL_3		0x00000030
#define	C_UART_FIFO_RX_LEVEL_4		0x00000040
#define	C_UART_FIFO_RX_LEVEL_5		0x00000050
#define	C_UART_FIFO_RX_LEVEL_6		0x00000060
#define	C_UART_FIFO_RX_LEVEL_7		0x00000070

// UART RX Timeout control register
#define	C_UART_RX_TO_LEVEL_INT		(1 << 3)
#define	C_UART_RX_TO_EN_INT		(1 << 0)


// RS485 control register
#define	C_UART_RS485_ENABLE 		0x00000080
#define	C_UART_RS485_RXEN    		0x00000040
#define	C_UART_RS485_ADDR_INTEN     0x00000008

//RS485 RTS control
#define	C_UART_RS485_AUTOMODE       0x00000001
#define	C_UART_RS485_MANUALMODE     0x00000000
#define	C_UART_RS485_RTS_TX         0x00000010
#define	C_UART_RS485_RTS_RX         0x00000000

#define UART_RX			        0
#define UART_TX			        1
#define UART_DISABLE	                0
#define UART_ENABLE		        1

#define UART_FIFO_SIZE                  32

#define UART0_DEV		        0
#define UART1_DEV		        1
#define UART2_DEV		        2
#define UART_DEV_NO		        3

typedef enum
{
	WORD_LEN_5,
	WORD_LEN_6,
	WORD_LEN_7,
	WORD_LEN_8
} WORD_LEN;

typedef enum
{
	STOP_SIZE_1,
	STOP_SIZE_2
} STOP_BIT_SIZE;

typedef enum
{
	PARITY_DISABLE,
	PARITY_ODD,
	PARITY_EVEN
} PARITY_SEL;

typedef enum
{
    UART_MODE,
    SIR_MODE,
    FIR_MIR,
    RS485_MODE
} UART_SIR_MODE;

typedef enum {
	ENUM_GPIO_INPUT_PULL_LOW = 0,				// Dir_Attr_Data = 0b000
	ENUM_GPIO_INPUT_FLOATING,					// Dir_Attr_Data = 0b01x
	ENUM_GPIO_INPUT_PULL_HIGH,					// Dir_Attr_Data = 0b001
	ENUM_GPIO_OUTPUT_LOW,						// Dir_Attr_Data = 0b110
	ENUM_GPIO_OUTPUT_HIGH						// Dir_Attr_Data = 0b111
} GPIO_FUNCTION_ENUM;

typedef enum
{
    MODE0, //Normal Mode
    MODE1, //Address Byte Detection mode
    MODE2 // Address Detection mode
} UART_RS485_RXMODE;

typedef enum
{
    AUTO_MODE,
    MANUAL_TX,
    MANUAL_RX
} UART_RS485_AUTOMODE;

typedef void(*SIR_IRQ_HANDLE)(INT32U);

// UART0 extern API
extern void drv_l1_uart0_init(void);
extern void drv_l1_uart0_buad_rate_set(INT32U bps);
extern void drv_l1_uart0_rx_enable(void);
extern void drv_l1_uart0_rx_disable(void);
extern void drv_l1_uart0_tx_enable(void);
extern void drv_l1_uart0_tx_disable(void);
extern void drv_l1_uart0_data_send(INT8U data, INT8U wait);
extern INT32S drv_l1_uart0_data_get(INT8U *data, INT8U wait);
extern INT32S drv_l1_uart0_word_len_set(INT8U word_len);
extern INT32S drv_l1_uart0_stop_bit_size_set(INT8U stop_size);
extern INT32S drv_l1_uart0_parity_chk_set(INT8U status, INT8U parity);
extern void drv_l1_uart0_rx_isr_set(void (*user_isr)(INT8U, INT32U, INT8U));
extern void drv_l1_uart0_rx_to_restart(void);
extern void drv_l1_uart0_rx_to_init(INT8U idle_level, INT32U lsb, INT32U msb);

// UART1 extern API
extern void drv_l1_uart1_init(void);
extern void drv_l1_uart1_buad_rate_set(INT32U bps);
extern void drv_l1_uart1_rx_enable(void);
extern void drv_l1_uart1_rx_disable(void);
extern void drv_l1_uart1_tx_enable(void);
extern void drv_l1_uart1_tx_disable(void);
extern void drv_l1_uart1_data_send(INT8U data, INT8U wait);
extern INT32S drv_l1_uart1_data_get(INT8U *data, INT8U wait);
extern INT32S drv_l1_uart1_word_len_set(INT8U word_len);
extern INT32S drv_l1_uart1_stop_bit_size_set(INT8U stop_size);
extern INT32S drv_l1_uart1_parity_chk_set(INT8U status, INT8U parity);
extern void drv_l1_uart1_rx_isr_set(void (*user_isr)(INT8U, INT32U, INT8U));
extern void drv_l1_uart1_rx_to_restart(void);
extern void drv_l1_uart1_rx_to_init(INT8U idle_level, INT32U lsb, INT32U msb);

// UART2 extern API
extern void drv_l1_uart2_init(void);
extern void drv_l1_uart2_buad_rate_set(INT32U bps);
extern void drv_l1_uart2_rx_enable(void);
extern void drv_l1_uart2_rx_disable(void);
extern void drv_l1_uart2_tx_enable(void);
extern void drv_l1_uart2_tx_disable(void);
extern void drv_l1_uart2_data_send(INT8U data, INT8U wait);
extern INT32S drv_l1_uart2_data_get(INT8U *data, INT8U wait);
extern INT32S drv_l1_uart2_word_len_set(INT8U word_len);
extern INT32S drv_l1_uart2_stop_bit_size_set(INT8U stop_size);
extern INT32S drv_l1_uart2_parity_chk_set(INT8U status, INT8U parity);
extern void drv_l1_uart2_rx_isr_set(void (*user_isr)(INT8U, INT32U, INT8U));
extern void drv_l1_uart2_rx_to_restart(void);
extern void drv_l1_uart2_rx_to_init(INT8U idle_level, INT32U lsb, INT32U msb);

extern void UART0_IRQHandler(void);
extern void UART1_IRQHandler(void);
extern void UART2_IRQHandler(void);
extern void drv_l1_uart_set_mode(INT32U dev_idx, INT32U mode, INT32U handle);
extern INT32S drv_l1_uart_rx_dma(INT8U dev_idx,INT8U *buf,INT32U size);
#endif		// __drv_l1_UART_H__

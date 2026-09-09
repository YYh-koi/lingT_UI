#ifndef __drv_l1_SPI_H__
#define __drv_l1_SPI_H__

#define SPI_NUM				2

/* SPI error message */
#define SPI_TIMEOUT        (-1)
#define SPI_BUSY           (-2)
#define SPI_RX_DMA_ONLY     1    // when 1, maximal read bulk size is 64K, dma use just one channel
                                 // when 0, maximal read size is 2^24, dma use two channel
                                 // performance of one big size is better than split into few bulk size,
                                 // and bootloader require performance, and using two channel is ok
                                 // so set it to 0
typedef enum
{
	SPI_IOD_6_9_ENUM = 0x0, // SPI0#0, IO_C0, IO_C1
	SPI_IOD_0_3_ENUM = 0x1, // SPI0#1, IO_D4, IO_D5
	SPI_IOC_8_11_ENUM = 0x2, // SPI1#0, IO_C2, IO_C3
	SPI_IOD_10_13_ENUM = 0x3, // SPI1#1, IO_D14, IO_D15
	SPI_MAX_PINMUX_ENUM = 0x4,
	SPI_MANUAL_PINMUX = SPI_IOD_0_3_ENUM, //SPI_IOD_10_13_ENUM,
	SPI_AUTO_ENUM = 0xFF
}SPI_IO_ENUM;

typedef enum
{
    SPI_0,
#if SPI_NUM > 1
	SPI_1,
#endif
    SPI_MAX_NUMS
} SPI_CHANNEL;

typedef enum
{
    SPI_LBM_NORMAL, /* no loop back */
    SPI_LBM_LOOP_BACK
} SPI_LOOP_BACK_MODE;

typedef enum
{
    PHA0_POL0 = 0,
    PHA0_POL1,
    PHA1_POL0,
    PHA1_POL1,
    PHA_MAX
} SPI_CLK_PHA_POL;

#ifndef _ENUM_SPI_IO_NUMBER
#define _ENUM_SPI_IO_NUMBER
typedef enum
{
	IO_1BIT,
	IO_2BIT,
	IO_4BIT,
	IO_MAX
} SPI_IO_NUMBER;
#endif

typedef enum
{
	SPI_TX,
	SPI_RX
} SPI_IO_DIR;

// spi rx data edge polar
#define SPI_RX_EDGE_FALLING        0   // RX latching data on falling edge, this is the default
#define SPI_RX_EDGE_RISING         1   // RX latching data on rising edge

extern INT32S drv_l1_spi_rx_edge_polar_set(INT8U spi_num, INT8U polar);
extern INT32S drv_l1_spi_init(INT8U spi_num);
extern INT32S drv_l1_spi_uninit(INT8U spi_num);
extern INT32S drv_l1_spi_disable(INT8U spi_num);
extern INT32S drv_l1_spi_txrx_level_set(INT8U spi_num, INT8U tx_level, INT8U rx_level);
extern INT32S drv_l1_spi_divisor_set(INT8U spi_num, INT32U div);
extern INT32S drv_l1_spi_clk_divisor_get(INT8U spi_num, INT8U *p_spi_clk_divisor);
extern INT32S drv_l1_spi_clk_set(INT8U spi_num, INT32U spi_clk);
extern INT32S drv_l1_spi_clk_rate_get(INT8U spi_num, INT32U *p_clk_rate);
extern INT32S drv_l1_spi_tx_clk_set(INT8U spi_num, INT32U spi_clk);
extern INT32S drv_l1_spi_tx_clk_get(INT8U spi_num, INT32U *p_spi_clk);
extern INT32S drv_l1_spi_rx_clk_set(INT8U spi_num, INT32U spi_clk);
extern INT32S drv_l1_spi_rx_clk_get(INT8U spi_num, INT32U *p_spi_clk);
extern INT32S drv_l1_spi_pha_pol_set(INT8U spi_num, INT8U pha_pol);
extern INT32S drv_l1_spi_mio_set(INT8U spi_num, SPI_IO_NUMBER mio, SPI_IO_DIR io_dir);
extern INT32S drv_l1_spi_lbm_set(INT8U spi_num, INT8S status);
extern INT32S drv_l1_spi_cs_by_gpio_set(INT8U spi_num, INT8U enable, INT8U pin_num, INT8U active);
extern INT32S drv_l1_spi_transceive(INT8U spi_num, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len);
extern INT32S drv_l1_spi_transceive_cpu(INT8U spi_num, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len);
extern INT32S drv_l1_spi_transceive_cpu_v2(INT8U spi_num, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len);
extern INT32S drv_l1_spi_slave_mode_trans(INT8U spi_num, INT8U dma_en, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len);
extern void spi_sw_reset(INT8U spi_num);
extern void spi_tx_byte(INT8U spi_num, INT8U txbyte);
extern INT8U spi_rx_byte(INT8U spi_num);
extern void spi_fasp_apb_set(INT8U spi_num);
extern void spi_fasp_apb_clear(INT8U spi_num);
extern INT32S drv_l1_spi_delay_set(INT8U spi_num, INT8U delay);
extern INT32S drv_l1_spi_transceive_dma_v2(INT8U spi_num, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len);
#endif 		// __drv_l1_SPI_H__

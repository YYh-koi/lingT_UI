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
#ifndef __drv_l1_TFT_SPI_H__
#define __drv_l1_TFT_SPI_H__

typedef enum
{
	TFT_SPI0 = 0,
	TFT_SPI1,
	TFT_SPI_NUM
} TFT_SPI_DEV_ID;

// tft spi state
#define B_TFT_SPI_TX_DONE                       1
#define C_TFT_SPI_TX_DONE                       (1<<B_TFT_SPI_TX_DONE)

// tft spi tx data
#define C_MASK_TFT_SPI_TX_DATA                  0xFFFFFFF

// TFT_SPI data_cnt
#define C_MASK_TFT_SPI_DATA_CNT                 0x3FFFFF

// TFT_SPI H_disp
#define C_MASK_TFT_SPI_H_DISP                   0x1FFF

// tft spi clock
#define TFT_SPI_CLK_DIVIDE_2                    0x0
#define TFT_SPI_CLK_DIVIDE_4                    0x1
#define TFT_SPI_CLK_DIVIDE_6                    0x2
#define TFT_SPI_CLK_DIVIDE_8                    0x3
#define TFT_SPI_CLK_DIVIDE_10                   0x4
#define TFT_SPI_CLK_DIVIDE_16                   0x5
#define TFT_SPI_CLK_DIVIDE_32                   0x6
#define TFT_SPI_CLK_DIVIDE_64                   0x7

// tft spi rx_data_sel
#define TFT_SPI_RX_DATA_DELAY3                  0x0
#define TFT_SPI_RX_DATA_DELAY2                  0x1
#define TFT_SPI_RX_DATA_DELAY1                  0x2
#define TFT_SPI_RX_DATA_DELAY0                  0x3

// TFT_SPI_CTRL
#define B_TFT_SPI_CLOCK_PHASE                   1
#define C_TFT_SPI_CLOCK_PHASE_2DATA             (1<<B_TFT_SPI_CLOCK_PHASE)
#define B_TFT_SPI_CLOCK_POLARITY                2
#define C_TFT_SPI_CLOCK_PHASE_HIGH              (1<<B_TFT_SPI_CLOCK_POLARITY)
#define B_TFT_SPI_TX_INTE                       8
#define C_TFT_SPI_TX_INTE_ENABLE                (1<<B_TFT_SPI_TX_INTE)
#define B_TFT_SPI_MAUT_RW_E                     18
#define C_TFT_SPI_MEM_MODE                      (1<<B_TFT_SPI_MAUT_RW_E)
#define B_TFT_SPI_CLOCK_OUT                     20
#define C_MASK_TFT_SPI_CLOCK_OUT                (7<<B_TFT_SPI_CLOCK_OUT)
#define B_TFT_SPI_RX_DATA_SEL                   28
#define C_MASK_TFT_SPI_RX_DATA_SEL              (3<<B_TFT_SPI_RX_DATA_SEL)
#define B_TFT_SPI_TX_DISABLE                    30
#define C_TFT_SPI_TX_DISABLE                    (1<<B_TFT_SPI_TX_DISABLE)
#define B_TFT_SPI_MASTER_RX                     31
#define C_TFT_SPI_MASTER_RX_MODE                (1<<B_TFT_SPI_MASTER_RX)

// TFT_SPI_CTRL2
#define B_TFT_SPI_ENABLE                        0
#define C_TFT_SPI_ENABLE                        (1<<B_TFT_SPI_ENABLE)
#define B_TFT_SPI_3BYTE                         1
#define B_TFT_SPI_4BYTE                         2
#define B_TFT_SPI_IO_KEEP_ENABLE                3
#define C_TFT_SPI_IO_KEEP_ENABLE                (1<<B_TFT_SPI_IO_KEEP_ENABLE)
#define B_TFT_SPI_DATA_INV                      7
#define C_TFT_SPI_BGR_ENABLE                    (1<<B_TFT_SPI_DATA_INV)
#define B_TFT_SPI_TE_MODE                       10
#define C_TFT_SPI_TE_VSYNC_HSYNC_ENABLE         (1<<B_TFT_SPI_TE_MODE)
#define B_TFT_SPI_TE_ENABLE                     11
#define C_TFT_SPI_TE_ENABLE                     (1<<B_TFT_SPI_TE_ENABLE)
#define B_TFT_SPI_MEM_CONT                      12
#define C_TFT_SPI_CONTINUE_FRAME_ENABLE         (1<<B_TFT_SPI_MEM_CONT)
#define B_TFT_SPI_DC_SEL                        13
#define C_TFT_SPI_DATA_ENABLE                   (1<<B_TFT_SPI_DC_SEL)
#define B_TFT_SPI_WIRE_MODE                     14
#define C_TFT_SPI_4WIRE_MODE                    (1<<B_TFT_SPI_WIRE_MODE)
#define B_TFT_SPI_MEM_START                     15
#define C_TFT_SPI_MEM_PPU_ENABLE                (1<<B_TFT_SPI_MEM_START)

// TFT_SPI_CTRL3
#define B_TFT_SPI_READ_CNT                      0
#define C_MASK_TFT_SPI_READ_CNT                 (0x1F<<B_TFT_SPI_READ_CNT)
#define B_TFT_SPI_DUMMY_CNT                     8
#define C_MASK_TFT_SPI_DUMMY_CNT                (0x3F<<B_TFT_SPI_DUMMY_CNT)

// TFT_SPI_VSYNC
#define B_TFT_SPI_VSYNC_WIDTH                   0
#define C_MASK_TFT_SPI_VSYNC_WIDTH              (0x3F<<B_TFT_SPI_VSYNC_WIDTH)
#define B_TFT_SPI_VSYNC_BACK_PORCH              8
#define C_MASK_TFT_SPI_VSYNC_BACK_PORCH         (0x3F<<B_TFT_SPI_VSYNC_BACK_PORCH)
#define B_TFT_SPI_VSYNC_FRONT_PORCH             16
#define C_MASK_TFT_SPI_VSYNC_FRONT_PORCH        (0x3F<<B_TFT_SPI_VSYNC_FRONT_PORCH)

// TFT_SPI_HSYNC
#define B_TFT_SPI_HSYNC_WIDTH                   0
#define C_MASK_TFT_SPI_HSYNC_WIDTH              (0x3F<<B_TFT_SPI_HSYNC_WIDTH)
#define B_TFT_SPI_HSYNC_BACK_PORCH              8
#define C_MASK_TFT_SPI_HSYNC_BACK_PORCH         (0x3F<<B_TFT_SPI_HSYNC_BACK_PORCH)
#define B_TFT_SPI_HSYNC_FRONT_PORCH             16
#define C_MASK_TFT_SPI_HSYNC_FRONT_PORCH        (0x3F<<B_TFT_SPI_HSYNC_FRONT_PORCH)

// TFT_SPI_TE_CNT
#define B_TFT_SPI_TE_LINE_CNT                   24
#define C_MASK_TFT_TE_LINE_CNT                  (0xFF<<B_TFT_SPI_TE_LINE_CNT)
#define B_TFT_SPI_TE_HSYNC_CNT                  0
#define C_MASK_TFT_SPI_TE_HSYNC_CNT             (0x3FFFF<<B_TFT_SPI_TE_HSYNC_CNT)

extern void drv_l1_tft_spi_lock(INT8U tft_spi_dev);
extern void drv_l1_tft_spi_unlock(INT8U tft_spi_dev);

extern INT32U drv_l1_tft_spi_inte_get(TFT_SPI_DEV_ID tft_spi_dev);
extern INT32U drv_l1_tft_spi_sts_get(TFT_SPI_DEV_ID tft_spi_dev);
extern INT32S drv_l1_tft_spi_sts_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U state);

extern void drv_l1_tft_spi_write_command_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U set_4wire, INT32U command);
extern void drv_l1_tft_spi_write_data_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U set_4wire, INT32U data);

extern void drv_l1_tft_spi_init(TFT_SPI_DEV_ID tft_spi_dev);
extern void drv_l1_tft_spi_uninit(TFT_SPI_DEV_ID tft_spi_dev);

extern INT32S drv_l1_tft_spi_ck_pha(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable);
extern INT32S drv_l1_tft_spi_ck_pol(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable);
extern INT32S drv_l1_tft_spi_manual_mode_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable);
extern INT32S drv_l1_tft_spi_tx_inte_enable(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable);
extern INT32S drv_l1_tft_spi_colok_out_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U clock_out);
extern INT32S drv_l1_tft_spi_rx_data_sel_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U rx_data_sel);
extern INT32S drv_l1_tft_spi_tx_disable_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U disable);
extern INT32S drv_l1_tft_spi_master_rx_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable);

extern INT32U drv_l1_tft_spi_enable_status(TFT_SPI_DEV_ID tft_spi_dev);
extern INT32S drv_l1_tft_spi_enable(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable);
extern INT32S drv_l1_tft_spi_rgb_order_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable);
extern INT32S drv_l1_tft_spi_manual_frame_mode_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U data_en);
extern INT32S drv_l1_tft_spi_ppu_mem_continue_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable);
extern INT32S drv_l1_tft_spi_4wire_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable);
extern INT32S drv_l1_tft_spi_ppu_mem_start(TFT_SPI_DEV_ID tft_spi_dev, INT32U start);

extern INT32S drv_l1_tft_spi_te_line_cnt_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U line_cnt, INT32U hsync_cnt);
extern INT32S drv_l1_tft_spi_te_mode_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U mode);
extern INT32S drv_l1_tft_spi_te_enable(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable);
extern INT32S drv_l1_tft_spi_io_keep_enable(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable);

extern INT32S drv_l1_tft_spi_read_dummy_cnt_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U read_cnt, INT32U dummy_cnt);

extern INT32S drv_l1_tft_spi_vsync_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U vsync_width, INT32U vsync_back_porch, INT32U vsync_front_porch);
extern INT32S drv_l1_tft_spi_v_disp_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U v_disp_width);
extern INT32S drv_l1_tft_spi_hsync_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U hsync_width, INT32U hsync_back_porch, INT32U hsync_front_porch);
extern INT32S drv_l1_tft_spi_h_disp_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U h_disp_width, INT32U wire_3_en);
extern INT32S drv_l1_tft_spi_data_cnt_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U h_disp_width, INT32U v_disp_width);

extern INT32S drv_l1_tft_spi_update_frame_wait(TFT_SPI_DEV_ID tft_spi_dev, INT32U wait);
extern INT32S drv_l1_tft_spi_update_frame_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U frame, INT32U wait);

extern void drv_l1_tft_spi_isr_callback_set(void (*callback)(INT8U tft_spi_dev, INT32U tft_spi_event));
extern void drv_l1_tft_spi_display_callback_set(void (*callback)(INT8U tft_spi_dev, INT32U tft_spi_event));
extern  void TFT_SPI1_IRQHandler(void);
#endif 		/* __drv_l1_TFT_H__ */

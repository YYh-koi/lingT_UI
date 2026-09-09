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

#ifndef __DRV_L1_I2S_TX_H__
#define __DRV_L1_I2S_TX_H__

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/
#define K_I2S_TX_STS_EMPTY                  0  // Read only
#define K_I2S_TX_STS_FULL                   1  // Read only
#define K_I2S_TX_STS_OVERWRITE              2  // Read only

#define K_I2S_TX_CTL_BIT_IRT_Polarity       14 // RW, 0 int low active, 1 int high active
#define K_I2S_TX_CTL_BIT_EN_OVWR_n          15 // RW, 0 enable, 1 disable
#define K_I2S_TX_CTL_BIT_EN_IRT             16 // RW, 1 enable, 0 disable
#define K_I2S_TX_CTL_BIT_IRT_FLAG           17 // RWC, write 1 to clear, raise when tx fifo is half empty
#define K_I2S_TX_CTL_BIT_CLRFIFO            18 // RW
#define K_I2S_TX_CTL_BIT_UNDERFLOW          19 // RWC, write 1 to clear, raised when tx fifo is empty

 /**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct i2sTxReg_s
{
    volatile INT32U     TX_CTRL;
    volatile INT32U     TX_DATA;
    volatile INT32U     TX_STATUS;
    volatile INT32U     TX_DUMMY;
    volatile INT32U     RX_CTRL;
    volatile INT32U     RX_DATA;
    volatile INT32U     RX_STATUS;
    volatile INT32U     RX_DUMMY;
} i2sTxReg_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern i2sTxReg_t *drv_l1_i2s_tx_get_register_base_addr(INT32U channel);
extern void drv_l1_i2s_tx_clear_fifo(INT32U channel);
extern INT32S drv_l1_i2s_tx_disable(INT32U channel, INT8U wait);
extern void drv_l1_i2s_tx_enable(INT32U channel);
extern void drv_l1_i2s_tx_disable_interrupt(INT32U channel);
extern void drv_l1_i2s_tx_enable_interrupt(INT32U channel);
extern INT32S drv_l1_i2s_tx_dma_transfer(INT32U channel, INT8U* srcBuf, INT32U BufLen, volatile INT8S *notify, INT8U *pchannel);
extern void drv_l1_i2s_tx_init(INT32U channel);
extern INT8U drv_l1_i2s_tx_check_is_all_idle(void);
extern INT32S drv_l1_i2s_tx_uninit(INT32U channel);
extern INT32U drv_l1_i2s_tx_get_fifo_count(INT32U channel);
extern void drv_l1_i2s_tx_set_bit_send_mode(INT32U channel, INT8U mode);
extern INT32S drv_l1_i2s_tx_set_data_bit_length(INT32U channel, INT32U data_size);
extern void drv_l1_i2s_tx_set_edge_mode(INT32U channel, INT8U mode);
extern void drv_l1_i2s_tx_set_first_frame_left_right(INT32U channel, INT8U polar);
extern void drv_l1_i2s_tx_set_frame_polar(INT32U channel, INT8U polar);
extern INT32S drv_l1_i2s_tx_set_frame_size(INT32U channel, INT32U frame_size, INT8U slave);
extern void drv_l1_i2s_tx_set_framing_mode(INT32U channel, INT8U mode);
extern void drv_l1_i2s_tx_set_mclk_divider(INT32U channel, INT32U mclk_div);
extern void drv_l1_i2s_tx_set_merge_mode(INT32U channel, INT32U merge);
extern void drv_l1_i2s_tx_set_mono(INT32U channel, INT32U mono);
extern void drv_l1_i2s_tx_set_normal_mode_bit_align(INT32U channel, INT8U mode);
extern void drv_l1_i2s_tx_set_right_channel_at_lsb_byte(INT32U channel, INT32U r_lsb);
extern INT32S drv_l1_i2s_tx_set_spu_fs(INT32U channel, INT32U spu_fs);
extern INT32S drv_l1_i2s_tx_resgiter_isr(INT32U channel, void (*isr_func)(INT32U channel, INT32U isr_global), INT32U isr_global);
extern void drv_l1_i2s_tx_dbf_free(INT32U channel);
extern INT32S drv_l1_i2s_tx_dbf_status_get(INT32U channel);
extern INT32S drv_l1_i2s_tx_dma_status_get(INT32U channel);
extern INT32S drv_l1_i2s_tx_dbf_set(INT32U channel, INT16S *data, INT32U cwlen);
extern INT32S drv_l1_i2s_tx_dbf_put(INT32U channel, INT16S *data, INT32U cwlen, osMessageQId os_q, volatile INT8S *notify, void (*isr_func)(INT8U, INT8U));

#endif		// __DRV_L1_I2S_TX_H__

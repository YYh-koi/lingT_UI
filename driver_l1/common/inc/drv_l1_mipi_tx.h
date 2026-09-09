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
#ifndef __DRV_L1_MIPI_TX_H__
#define __DRV_L1_MIPI_TX_H__

/****************************************************
*		constant 									*
****************************************************/
#define ROT2DISP2_WAIT_DONE             0x12345678

#define CMD_DELAY	     			    0xFA

#define MIPI_GENERIC_SHORT_WRITE_0P	    0x03
#define MIPI_GENERIC_SHORT_WRITE_1P	    0x13
#define MIPI_GENERIC_SHORT_WRITE_2P	    0x23
#define MIPI_DCS_SHORT_WRITE_0P		    0x05
#define MIPI_DCS_SHORT_WRITE_1P		    0x15
#define MIPI_GENERIC_LONG_WRITE		    0x29
#define MIPI_DCS_LONG_WRITE			    0x39
#define MIPI_GENERIC_SHORT_READ_0P	    0x04
#define MIPI_GENERIC_SHORT_READ_1P	    0x14
#define MIPI_GENERIC_SHORT_READ_2P	    0x24
#define MIPI_DCS_READ_0P			    0x06

#if 1
#define MIPI_DSI_10MS_DELAY(x)	        vTaskDelay(10*x)
#else
#define MIPI_DSI_10MS_DELAY(...)
#endif

#define COMMAND_MODE                    1
#define VIDEO_MODE                      0

#define HAS_BLK_MODE                    (0x1 << 0)
#define HBP_BLK_MODE                    (0x1 << 4)
#define HFP_BLK_MODE                    (0x1 << 8)  //HFP Blank Mode
#define BLLP_BLK_MODE                   (0x1 << 12) //BLLP Blank Mode

//For video input control
#define NON_BURST_SYNC_PULSE            0x0
#define NON_BURST_SYNC_EVENT            0x1
#define BURST_MODE                      0x2

typedef enum
{
	F_93p75M,
	F_125M,
	F_187p5M,
	F_250M,
	F_375M,
	F_500M,
	F_750M,
	F_1000M,
} PHY_FREQ_SEL;
/****************************************************
*		data type 									*
*****************************************************/
typedef struct mipi_tx_config_s
{

} mipi_tx_config_t;

typedef struct MIPI_Command_Table_s
{
    char data_cnt;
    char data[78];		/* command + data */
    char packet_type;	/* packet type */
} MIPI_Command_Table_t;

typedef struct MIPI_Short_Command_Table_s
{
    char data_cnt;		/* only 1 or 2 bytes */
    char data[2];		/* command + data */
    char packet_type;	/* packet type */
} MIPI_Short_Command_Table_t;

typedef enum
{
	PACKET_SHORT_WRITE = 0,
	PACKET_LONG_WRITE
} PACKET_WRITE_METHOD;
/****************************************************
*		function
*****************************************************/
void mipi_write_lcd_command(void* table, INT32U cnt, INT32U type);
void mipi_lcd_software_reset(INT16U delay);
void drv_l1_mipi_tx_clock_en(INT8U enable);
void drv_l1_mipi_tx_phy_reset(INT8U en);
void drv_l1_mipi_tx_lane_swap_set(INT8U PN_swap, INT32U data_lane);
void drv_l1_mipi_tx_lane_set(INT32U lane);
void drv_l1_mipi_tx_enable(INT8U enable);
INT32U drv_l1_mipi_tx_get_tmds_clock();
INT32U drv_l1_mipi_tx_get_dsi_clock();
void drv_l1_mipi_tx_set_LPCK(INT8U div);
void drv_l1_mipi_tx_phy_frq_sel(INT8U frq);
void drv_l1_mipi_tx_mode_set(INT8U mode);
void drv_l1_mipi_tx_low_power_mode(INT8U enable);
void drv_l1_mipi_tx_h_timing_ctrl(INT32U DSI_HSA, INT32U DSI_HFP, INT32U DSI_HBP);
void drv_l1_mipi_tx_v_timing_ctrl(INT32U TFT_VSA, INT32U TFT_VFP, INT32U TFT_VBP, INT32U TFT_VACT);
void drv_l1_mipi_tx_lane_timing_ctrl(INT32U phy_hs_exit, INT32U phy_lpx);
void drv_l1_mipi_tx_clock_lane_timing_ctrl0(INT32U phy_clk_prepare, INT32U phy_clk_zero);
void drv_l1_mipi_tx_clock_lane_timing_ctrl1(INT32U phy_clk_trail, INT32U phy_clk_pre, INT32U phy_clk_post);
void drv_l1_mipi_tx_data_lane_timing_ctrl(INT32U phy_hs_trail, INT32U phy_hs_prpr, INT32U phy_hs_zero);
void drv_l1_mipi_tx_wakeup_delay(INT16U delay);
void drv_l1_mipi_tx_RGB_word_count(INT16U word_count);
void drv_l1_mipi_tx_video_input_mode(INT8U mode);
void drv_l1_mipi_tx_blank_mode(INT32U mode);
void drv_l1_mipi_tx_clock_HS_set(INT8U en);
void drv_l1_mipi_tx_Eotp_set(INT8U enable);
INT32U drv_l1_mipi_tx_get_status();
void drv_l1_mipi_tx_software_module_reset(void);
#endif

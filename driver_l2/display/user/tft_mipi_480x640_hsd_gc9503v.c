
#include "drv_l2.h"

#if (defined LCD_DRV_EN_HSD_GC9503V) && (LCD_DRV_EN_HSD_GC9503V == 1)

//***********************
// Program Configuration
//***********************

#define LANE_NUM 			2

#define IMG_WIDTH     		480
#define IMG_HEIGHT      	640
#define TFT_HACT			IMG_WIDTH
#define TFT_VACT			IMG_HEIGHT
#define TFT_HS_WIDTH		20							//HPW
#define TFT_H_START			(TFT_HS_WIDTH + 30)			//HPW+HBP
#define TFT_H_END			(TFT_H_START + IMG_WIDTH)	//HPW+HBP+HDE
#define TFT_H_PERIOD		(TFT_H_END + 30)			//HPW+HBP+HDE+HFP

#define TFT_VS_WIDTH    	(5)		                	//VPW
#define TFT_V_START    	 	(TFT_VS_WIDTH + 8)         //VPW+VBP  //Please note: + value must be either 32 or greater than 32.
#define TFT_V_END      		(TFT_V_START + IMG_HEIGHT)  //VPW+VBP+VDE
#define TFT_V_PERIOD    	(TFT_V_END + 15)            //VPW+VBP+VDE+VFP

#define TFT_VSA             TFT_VS_WIDTH
#define TFT_VBP             TFT_V_START - TFT_VS_WIDTH
#define TFT_VFP             TFT_V_PERIOD - TFT_V_END

#define TFT_HSA             TFT_HS_WIDTH
#define TFT_HBP             TFT_H_START - TFT_HS_WIDTH
#define TFT_HFP             TFT_H_PERIOD - TFT_H_END

//*********************************
//DSI Timing Spec
//**********************************
#define SPEC_HS_Exit		200							//unit: ns
#define SPEC_LPX			65							//unit: ns
#define	SPEC_CLK_Prepare	((38+95)/2)					//unit: ns
#define	SPEC_CLK_Zero		(500 - SPEC_CLK_Prepare)	//unit: ns
#define SPEC_clk_post		52							//unit  UI
#define	SPEC_clk_pre		8							//unit: UI
#define SPEC_clk_trail		60 							//unit: ns
#define SPEC_hs_prepare_ns	65							//Unit: ns
#define SPEC_hs_prepare_UI	5							//Unit: UI
#define SPEC_hs_zero		300							//Unit: ns
#define SPEC_hs_trail		100							//Unit: UI


static MIPI_Command_Table_t gc9503v_setting[] =
{
	{ 6,{0xF0,0x55, 0xAA, 0x52, 0x08, 0x00}, MIPI_GENERIC_LONG_WRITE},

	{ 3,{0xF6,0x5A, 0x87}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0xC1,0x3F}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0xC2,0x0E}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0xC6,0xF8}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0xC9,0x10}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0xCD,0x25}, MIPI_GENERIC_LONG_WRITE},

	{5, {0xfa,0x08,0x08,0x00,0x04}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0xF8,0x8A}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0x71,0x48}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0x72,0x48}, MIPI_GENERIC_LONG_WRITE},

	{ 3,{0x73,0x00, 0x44}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0x97,0xEE}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0x83,0x93}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0xA3,0x22}, MIPI_GENERIC_LONG_WRITE},

	{ 4,{0xFD,0x28, 0x3C, 0x00}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0xAC,0x45}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0xA7,0x47}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0xA0,0xDD}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0x9A,0xd0}, MIPI_GENERIC_LONG_WRITE},//c0

	{ 2,{0x9B,0x30}, MIPI_GENERIC_LONG_WRITE},//40

	{ 3,{0x82,0x3e, 0x3e}, MIPI_GENERIC_LONG_WRITE},//3e

	///{ 2,{0x80,0x57}, MIPI_GENERIC_LONG_WRITE},

	{ 2, {0xB1,0x10}, MIPI_GENERIC_LONG_WRITE},//10

	{ 3, {0x7A,0x13, 0x1A}, MIPI_GENERIC_LONG_WRITE},

	{ 3, {0x7B,0x13, 0x1A}, MIPI_GENERIC_LONG_WRITE},

	{ 33,{0x6D,0x00, 0x1F, 0x19, 0x1A, 0x10, 0x0E, 0x0C, 0x0A, 0x02, 0x08, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
	      0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x07, 0x01, 0x09, 0x0B, 0x0D, 0x0F, 0x1A, 0x19, 0x1F, 0x00}, MIPI_GENERIC_LONG_WRITE},

	{ 17,{0x64,0x28, 0x04, 0x02, 0x7C, 0x03, 0x03, 0x28, 0x03, 0x02, 0x7D, 0x03, 0x03, 0x7A, 0x7A, 0x7A, 0x7A}, MIPI_GENERIC_LONG_WRITE},

	{ 17,{0x65,0x28, 0x02, 0x02, 0x7E, 0x03, 0x03, 0x28, 0x01, 0x02, 0x7F, 0x03, 0x03, 0x7A, 0x7A, 0x7A, 0x7A}, MIPI_GENERIC_LONG_WRITE},

	{ 17,{0x66,0x28, 0x00, 0x02, 0x80, 0x03, 0x03, 0x20, 0x01, 0x02, 0x81, 0x03, 0x03, 0x7A, 0x7A, 0x7A, 0x7A}, MIPI_GENERIC_LONG_WRITE},

	{ 17,{0x67,0x20, 0x02, 0x02, 0x82, 0x03, 0x03, 0x20, 0x03, 0x02, 0x83, 0x03, 0x03, 0x7A, 0x7A, 0x7A, 0x7A}, MIPI_GENERIC_LONG_WRITE},

	{ 14,{0x68,0x77, 0x08, 0x06, 0x08, 0x05, 0x7A, 0x7A, 0x08, 0x06, 0x08, 0x05, 0x7A, 0x7A}, MIPI_GENERIC_LONG_WRITE},

	{ 9, {0x60,0x28, 0x06, 0x7A, 0x7A, 0x28, 0x05, 0x7A, 0x7A}, MIPI_GENERIC_LONG_WRITE},

	{ 9, {0x63,0x22, 0x7E, 0x7A, 0x7A, 0x22, 0x7F, 0x7A, 0x7A}, MIPI_GENERIC_LONG_WRITE},

	{ 8, {0x69,0x14, 0x22, 0x14, 0x22, 0x14, 0x22, 0x08}, MIPI_GENERIC_LONG_WRITE},

	{ 2, {0x6B,0x07}, MIPI_GENERIC_LONG_WRITE},

	{ 53,{0xD1,0x00,0x00,0x00,0x7F,0x00,0xD8,0x00,0xF8,0x01,0x14,0x01,0x3D,0x01,0x61,0x01,0x98,0x01,0xC4,0x02,0x07,
	      0x02,0x3E,0x02,0x94,0x02,0xDC,0x02,0xDE,0x03,0x27,0x03,0x80,0x03,0xA8,0x03,0xC3,0x03,0xD3,0x03,0xE4,0x03,
	      0xED,0x03,0xF6,0x03,0xF8,0x03,0xF9,0x03,0xFB,0x03,0XFF}, MIPI_GENERIC_LONG_WRITE},

	{ 53,{0xD2,0x00,0x00,0x00,0x7F,0x00,0xD8,0x00,0xF8,0x01,0x14,0x01,0x3D,0x01,0x61,0x01,0x98,0x01,0xC4,0x02,0x07,
	      0x02,0x3E,0x02,0x94,0x02,0xDC,0x02,0xDE,0x03,0x27,0x03,0x80,0x03,0xA8,0x03,0xC3,0x03,0xD3,0x03,0xE4,0x03,
	      0xED,0x03,0xF6,0x03,0xF8,0x03,0xF9,0x03,0xFB,0x03,0XFF}, MIPI_GENERIC_LONG_WRITE},

	{ 53,{0xD3,0x00,0x00,0x00,0x7F,0x00,0xD8,0x00,0xF8,0x01,0x14,0x01,0x3D,0x01,0x61,0x01,0x98,0x01,0xC4,0x02,0x07,
	      0x02,0x3E,0x02,0x94,0x02,0xDC,0x02,0xDE,0x03,0x27,0x03,0x80,0x03,0xA8,0x03,0xC3,0x03,0xD3,0x03,0xE4,0x03,
	      0xED,0x03,0xF6,0x03,0xF8,0x03,0xF9,0x03,0xFB,0x03,0XFF}, MIPI_GENERIC_LONG_WRITE},

	{ 53,{0xD4,0x00,0x00,0x00,0x7F,0x00,0xD8,0x00,0xF8,0x01,0x14,0x01,0x3D,0x01,0x61,0x01,0x98,0x01,0xC4,0x02,0x07,
	      0x02,0x3E,0x02,0x94,0x02,0xDC,0x02,0xDE,0x03,0x27,0x03,0x80,0x03,0xA8,0x03,0xC3,0x03,0xD3,0x03,0xE4,0x03,
	      0xED,0x03,0xF6,0x03,0xF8,0x03,0xF9,0x03,0xFB,0x03,0XFF}, MIPI_GENERIC_LONG_WRITE},

	{ 53,{0xD5,0x00,0x00,0x00,0x7F,0x00,0xD8,0x00,0xF8,0x01,0x14,0x01,0x3D,0x01,0x61,0x01,0x98,0x01,0xC4,0x02,0x07,
          0x02,0x3E,0x02,0x94,0x02,0xDC,0x02,0xDE,0x03,0x27,0x03,0x80,0x03,0xA8,0x03,0xC3,0x03,0xD3,0x03,0xE4,0x03,
          0xED,0x03,0xF6,0x03,0xF8,0x03,0xF9,0x03,0xFB,0x03,0XFF}, MIPI_GENERIC_LONG_WRITE},

	{ 53,{0xD6,0x00,0x00,0x00,0x7F,0x00,0xD8,0x00,0xF8,0x01,0x14,0x01,0x3D,0x01,0x61,0x01,0x98,0x01,0xC4,0x02,0x07,
	      0x02,0x3E,0x02,0x94,0x02,0xDC,0x02,0xDE,0x03,0x27,0x03,0x80,0x03,0xA8,0x03,0xC3,0x03,0xD3,0x03,0xE4,0x03,
	      0xED,0x03,0xF6,0x03,0xF8,0x03,0xF9,0x03,0xFB,0x03,0XFF}, MIPI_GENERIC_LONG_WRITE},

	{ 2,{0x11,0x00}, MIPI_GENERIC_LONG_WRITE},
	{12, {CMD_DELAY}, MIPI_GENERIC_LONG_WRITE},			///* Delay 100ms

	{ 2,{0x29,0x00}, MIPI_GENERIC_LONG_WRITE},

	{10, {CMD_DELAY}, MIPI_GENERIC_LONG_WRITE},			///* Delay 100ms
};


static void Pannel_gc9503v_Setting(void)
{
    DBG_PRINT("->%s,%d:%s %s\r\n",__func__,__LINE__,__DATE__,__TIME__);

	/* Do reset LCD pannel */
    gpio_write_io(PANEL_IO_RESET, 1);
	osDelay(30);
	gpio_write_io(PANEL_IO_RESET, 0);
	osDelay(120);
    gpio_write_io(PANEL_IO_RESET, 1);
	osDelay(70);

	/* Set lane swap */
    drv_l1_mipi_tx_lane_swap_set(0, 0x3210);    //PN_swap is useless and can only be set to zero, commment by wwj

    mipi_lcd_software_reset(15);

	//R_DSI_SPKT_HD_CTRL = 0x00000006;

	mipi_write_lcd_command((void*)gc9503v_setting, sizeof(gc9503v_setting)/sizeof(MIPI_Command_Table_t), PACKET_LONG_WRITE);
}

//
INT32S tft_hsd_gc9503v_init(INT32U tft_index)
{
    INT32U	reg_data;
    INT32U  PCLK, DSICLK, DSICLK_period, bit_period, PtoD_Ratio;;
	INT32U	DSI_HFP, DSI_HSA, DSI_HBP;
	INT32U	phy_hs_exit, phy_lpx;
	INT32U	phy_clk_prepare, phy_clk_zero;
	INT32U	phy_clk_post, phy_clk_pre, phy_clk_trail;
	INT32U	phy_hs_prpr, phy_hs_zero, phy_hs_trail;

	if(tft_index != TFT_2)  return STATUS_FAIL;

	DBG_PRINT("%s\r\n", __func__);

    drv_l1_mipi_tx_clock_en(ENABLE);
    MIPI_DSI_10MS_DELAY(1);

	//========================================
    //enable LPRX_SCLK
    // LPRX_SCLK = PERICLK/(1+LPRX_SCLK_DIV)
    //b15: LPRX_SCLKEN
    //b13~8: LPRX_SCLK_DIV
    //lprx_sclk >= Low-Power clock*4

    reg_data = R_SYSTEM_DSI_CTRL;
    reg_data &= 0x00FF;
#if (LANE_NUM == 4)
    reg_data |= 0x8700;         //  = 330MHz / 8
#elif(LANE_NUM == 2)
    reg_data |= 0x8400;         //  = 330MHz / 5
#endif
    R_SYSTEM_DSI_CTRL = reg_data;

	//========================================

    ////////////////////////////////////////////////////////////////////////////////////
    //!!!  Need to be modified if you want to change the PLL freq. or panel fps    !!!//
    //!!!  720p: 14.5us/line =>  1280-line *1.1 blanking *14.5us = 20.4ms => 49fps !!!//
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    R_SYSTEM_PERI_CLK_CTRL |= 0x6;                                      //!!! [1]=1:select PLLEXT, [2]=ENABLE(default)
    R_SYSTEM_EXTCLK_CTRL    = 0x6300;                                   //!!! [14]=enable,  [13:8]= 0x23 => PLLEXT  = 387MHz
    R_DSI_MISC_CTRL         = 0x1266;                                   //!!! DSICLK from PLLEXT: DSICLKX8 = (PLLEXT*2)/([11:9]+1) = 774M, DSICLK=774M/8= 96.75MHz
    R_SYSTEM_TMDS_CTRL      = ((R_SYSTEM_TMDS_CTRL & 0x00FF) | 0x9100); //!!! [15]=enable,  [13:8] => PCLK = (PLLSYS/2)/([13:8]+1) = 333M/([13:8]+1)= 23.8MHz
    drv_l1_mipi_tx_set_LPCK(14);                                         //!!! LP mode freq = DSICLK/(14+1) = 96.75M/15 = 6.43M < LP SPEC: 10MHz

    DSICLK = 45;                                                        //!!! MHz;                     RULE: DSICLK > 3/2-lane * PCLK * (1.14 for blanking margin)
	PCLK   = 24;                                                        //!!! MHz;                                96.76M > 1.5 * 23.8.5M * 1.14 = 40.698M
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//========================================
	//Setting PHY BIT Width : DSI_CLK * 8 Range
#if (LANE_NUM == 4)
    drv_l1_mipi_tx_phy_frq_sel(F_500M);
#elif(LANE_NUM == 2)
    drv_l1_mipi_tx_phy_frq_sel(F_500M);                                 //!!! check DSICLKX8 freq.
#endif
    //========================================

    MIPI_DSI_10MS_DELAY(1);

    //***********************************************************//
    //Switch IOA15~10 to be floating for MIPI_DSI ports
    //***********************************************************//
    //note that TFT2 funpos doesn't need to occupy IOA
	//R_IOF_ATT |= 0x0FFF;	//IOF_ATT for HDMI only

    //=========================================
    //Analog PHY Enable and DSI Enable
    drv_l1_mipi_tx_enable(ENABLE);
    //=========================================

    //=========================================
    //DSI Setting for LPDT
    drv_l1_mipi_tx_mode_set(COMMAND_MODE);
    drv_l1_mipi_tx_low_power_mode(ENABLE);
    //=========================================

    drv_l1_mipi_tx_phy_reset(ENABLE);
    MIPI_DSI_10MS_DELAY(1);
    drv_l1_mipi_tx_phy_reset(DISABLE);

    Pannel_gc9503v_Setting();

    //**********************************************************
    //*************  DSI Parameters Setting     ****************
    //**********************************************************
    //-----|    |--------|-----------------------|    |----
    //     |    |                                |    |
    //     |____|                                |____|
    //     |HS_W|
    //     |<- H_START ->|
    //     |<---------  HEND  -------------->|
    //	   |<------------  H_PERUOD ------------>|
	//     |HSA |<-HBP-->|<------ HACT ----->|HFP|


	//****************************************
	//  TFT Timing map to DSI Function
	//****************************************
    DSICLK_period = (1000 / DSICLK);
    bit_period = (1000 / (DSICLK * 8));

    PtoD_Ratio =  DSICLK/PCLK;
    if(DSICLK % PCLK)    PtoD_Ratio += 1;

    DBG_PRINT("PtoD_Ratio %d\r\n", PtoD_Ratio);

    DSI_HSA = TFT_HSA * PtoD_Ratio;
    DSI_HBP = TFT_HBP * PtoD_Ratio;
    DSI_HFP = TFT_HFP * PtoD_Ratio;

    //DSI_HBP = 0x80;		/* Fix wave waveform on TFT */

    if(DSI_HSA < LANE_NUM)  DSI_HSA = LANE_NUM;
    if(DSI_HBP < LANE_NUM)  DSI_HBP = LANE_NUM;

    drv_l1_mipi_tx_h_timing_ctrl(DSI_HSA, DSI_HFP, DSI_HBP);
    drv_l1_mipi_tx_v_timing_ctrl(TFT_VSA, TFT_VFP, TFT_VBP, TFT_VACT);

    //************************************************//
    //  DSI Timing Setting
    //************************************************//

    //========================================
    //spec_hs_exit = 200;
    //spec_lpx = 65;
    //phy_hs_exit = (200 / DSICLK_period) +1;
    //phy_lpx = (65 / DSICLK_period) + 1;
    phy_hs_exit = (SPEC_HS_Exit/ DSICLK_period) +1;
    phy_lpx = ( SPEC_LPX/ DSICLK_period) + 1;
    drv_l1_mipi_tx_lane_timing_ctrl(phy_hs_exit, phy_lpx);
    //========================================

    //========================================
    //phy_clk_prepare = ((38 + 95) / 2) / DSICLK_period + 1; //(max+min)/2
    //phy_clk_zero = (500 - phy_clk_prepare) / DSICLK_period + 1;	//??unit not right ?? 500 ns, phy_clk_prepare unit is dsi clk
    phy_clk_prepare = (SPEC_CLK_Prepare / DSICLK_period) +1;
    phy_clk_zero =	(SPEC_CLK_Zero / DSICLK_period)+1;
    drv_l1_mipi_tx_clock_lane_timing_ctrl0(phy_clk_prepare, phy_clk_zero);
    //========================================

    //========================================
    //phy_clk_post = ((60 + 52 * bit_period) * 2) / DSICLK_period;
    //phy_clk_pre = (8 * bit_period * 2) / DSICLK_period + 1;
    //phy_clk_trail = (60 * 2) / DSICLK_period + 1;
    phy_clk_post = 	(SPEC_clk_post * 2);	//unit UI
    phy_clk_pre = 	(SPEC_clk_pre * 2) ;	//unit UI
    phy_clk_trail = (SPEC_clk_trail * 2) / DSICLK_period + 1;		//unit : ns
    drv_l1_mipi_tx_clock_lane_timing_ctrl1(phy_clk_trail, phy_clk_pre, phy_clk_post);
    //========================================

    //========================================
    phy_hs_zero = (SPEC_hs_zero/DSICLK_period) +1;
    phy_hs_prpr = (SPEC_hs_prepare_ns + (SPEC_hs_prepare_UI*bit_period)) / DSICLK_period +1;
    phy_hs_trail = SPEC_hs_trail;
    drv_l1_mipi_tx_data_lane_timing_ctrl(phy_hs_trail, phy_hs_prpr, phy_hs_zero);

    drv_l1_mipi_tx_wakeup_delay(0xC350);//set counter value for 1ms.
    drv_l1_mipi_tx_RGB_word_count(TFT_HACT * 3);//pixel per line
    drv_l1_mipi_tx_video_input_mode(NON_BURST_SYNC_EVENT);
    drv_l1_mipi_tx_blank_mode(BLLP_BLK_MODE|HFP_BLK_MODE);

    drv_l1_mipi_tx_clock_HS_set(ENABLE);//CLK lan HS enable
    drv_l1_mipi_tx_Eotp_set(ENABLE);
    drv_l1_mipi_tx_lane_set(LANE_NUM);
    drv_l1_mipi_tx_mode_set(VIDEO_MODE);

	//****************************************
	//  TFT Controller Setting
	//****************************************
    //b[0]: TFT2_EN
    //b[16]: ASYN_EN,  only parallel RGB is supported, this mode is for HDMI and MIPI DSI
    //b[17]: FAST_REQ, this is very helpful for enhancing the access speed in-between TFT2 and MIPI DSI/HDMI interpolation module
    // other bits are reserved
    R_TFT2_CTRL = 0x00030000;

    //b[25:16]: HDMI_VSYNC_START2, HDMI VSYNC pulse start line count register for interlace mode odd field
    //b[10:0]: HDMI_V_PERIOD, HDMI Vertical period register for progressive mode or interlace mode field 1 (odd field)
    R_TFT2_V_PERIOD = TFT_V_PERIOD - 1;

    //b[31:20]: HDMI_V_VALID_END2
    //b[19:8]: HDMI_V_VALID_START2
    //b[7:0]: reserved
    //R_TFT2_VS_WIDTH = TFT_VS_WIDTH - 1;

    //b[9:0]: HDMI_V_VAILD_START
    //b[25:16]: HDMI_VSYNC_START
    //other bits: reserved
    R_TFT2_V_START = ((0 << 16) | TFT_V_START);

    //b[10:0]: HDMI_V_VAILD_END
    //b[25:16]: HDMI_VSYNC_END
    //other bits: reserved
    R_TFT2_V_END = (((TFT_VS_WIDTH - 1) << 16) | (TFT_V_END - 1));

    //b[26:16]: HDMI_VSYNC_END2
    //b[12:0]: HDMI_H_PERIOD
    //other bits: reserved
    R_TFT2_H_PERIOD = TFT_H_PERIOD - 1;

    //b[28:16]: HDMI_VSYNC_HSHIFT
    //other bits: reserved
    //R_TFT2_HS_WIDTH = TFT_HS_WIDTH - 1;

    //b[28:16]: HDMI_HSYNC_START
    //b[12:0]: HDMI_H_VALID_START
    //other bits: reserved
    R_TFT2_H_START = ((1 << 16) | (TFT_H_START + 1));

    //b[28:16]: HDMI_HSYNC_END
    //b[12:0]: HDMI_H_VALID_END
    //other bits: reserved
    //R_TFT2_H_END = ((4 << 16) | TFT_H_END);
    R_TFT2_H_END = ((TFT_HS_WIDTH << 16) | TFT_H_END);

    R_TFT2_CTRL |= (1 << 31);       //switching to bank1
    R_TFT2_LINE_RGB_ORDER = 0x0;	//RGB order
    R_TFT2_CTRL &= ~(1 << 31);      //switching to bank0

    reg_data = drv_l1_mipi_tx_get_status();
    while(!(reg_data & 0x1))			//wait TXPHY stop
    {
        reg_data = drv_l1_mipi_tx_get_status();
    }

    //[b0]: TFT2_EN
    R_TFT2_CTRL |= 0x1;
	return STATUS_OK;
}

static INT32S tft_hsd_gc9503v_uninit(INT32U tft_index)
{
	DBG_PRINT("Do mipi_display_uninit, ctl = 0x%x\r\n", R_DSI_CTRL);

    R_DSI_LPKT_HD_CTRL = 0x229;
    R_DSI_LPKT_PLD_CTRL = 0x0028;
    MIPI_DSI_10MS_DELAY(1);
    R_DSI_LPKT_HD_CTRL = 0x229;
    R_DSI_LPKT_PLD_CTRL = 0x0010;
    MIPI_DSI_10MS_DELAY(12);

    /* Stop MIPI DSI */
    drv_l1_mipi_tx_enable(0);
    osDelay(200);

	/* Let reste be low */
    gpio_write_io(PANEL_IO_RESET, 0);
	return 0;
}

// tft table
const DispCtrl_t TFT_Param_HSD_GC9503V =
{
	/* lcd size */
	TFT_HACT,
	TFT_VACT,

    0,  //sync_pin_enable
	0,  //de_pin_enable
	0,  //te_pin_enable
	0,  //tft2_io_enable

	60,  //h_off_lines_video
	60,  //h_off_lines_cap

	0,  //en_crop_video
	0,  //en_crop_cap
	0,  //pbp_crop_video
	0,  //pbp_crop_cap
	0,  //back_ratio_video
	0,  //back_ratio_cap

	/* pip size */
	320,//256,
	180,//160,

	/* pip position */
	-320,   //x, must be multiple of 8
	0,      //y

	/* lcd init & uninit */
	tft_hsd_gc9503v_init,
	tft_hsd_gc9503v_uninit,

	/* lcd backlight control */
	LCD_BL_CTRL_CONSTANT,
	20 * 1000,	// in HZ for LCD_BL_CTRL_PWM
	55,			// in duty cycle for LCD_BL_CTRL_PWM

	/* tft format */
    #if RESOURCE_FORMAT == RESOURCE_GP420
	DISP_FMT_GP420,
	#elif RESOURCE_FORMAT == RESOURCE_RGB565
	DISP_FMT_RGB565,
	#elif RESOURCE_FORMAT == RESOURCE_YUYV
	DISP_FMT_YUYV,
	#endif

	/* tft IFCE     */
	LCD_IFCE_RGB,

	/* tft rotate   */
    TFT_ROT_LEFT,

    /*  tft width & height  */
    TFT_HACT,
    TFT_VACT
};

#endif//#if (defined LCD_DRV_EN_HSD_GC9503V) && (LCD_DRV_EN_HSD_GC9503V == 1)


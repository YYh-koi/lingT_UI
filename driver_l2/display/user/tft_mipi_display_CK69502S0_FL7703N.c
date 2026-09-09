#include "drv_l2.h"
/*#include "drv_l1_sfr.h"
#include "drv_l1_tft.h"
#include "drv_l1_gpio.h"
#include "drv_l2_display.h"
#include "drv_l1_mipi_tx.h"
#include "drv_l1_clock.h"
#include "drv_l1_system.h"
#include "gplib_print_string.h"*/

#if (defined LCD_DRV_EN_CK69502S0_FL7703N) && (LCD_DRV_EN_CK69502S0_FL7703N == 1)
#define PANEL_IO_RESET  IO_A8

#define LANE_NUM        2
#define FRAME_RATE      40///55
#define DSI_CLK_SRC     PLLEXTx2

#define IMG_WIDTH       608
#define IMG_HEIGHT      1024
#define TFT_HACT        IMG_WIDTH
#define TFT_VACT        IMG_HEIGHT

#define TFT_HS_WIDTH    60                           //*4
#define TFT_H_START     (TFT_HS_WIDTH + 140)         //*4+10 = 14
#define TFT_H_END       (TFT_H_START + IMG_WIDTH)   //*14+800 = 814
#define TFT_H_PERIOD    (TFT_H_END + 140)            //*814+20 = 834

#define TFT_VS_WIDTH    (4)                         //*2
#define TFT_V_START     (TFT_VS_WIDTH + 14)         //*2+14 = 16
#define TFT_V_END       (TFT_V_START + IMG_HEIGHT)  //*16+1280 = 1296
#define TFT_V_PERIOD    (TFT_V_END + 16)             //*1058

#define TFT_VSA         TFT_VS_WIDTH
#define TFT_VBP         (TFT_V_START - TFT_VS_WIDTH)
#define TFT_VFP         (TFT_V_PERIOD - TFT_V_END)

#define TFT_HSA         TFT_HS_WIDTH
#define TFT_HBP         (TFT_H_START - TFT_HS_WIDTH)
#define TFT_HFP         (TFT_H_PERIOD - TFT_H_END)
//*********************************
//DSI Timing Spec
//**********************************
#define SPEC_HS_Exit        200                     //unit: ns
#define SPEC_LPX			65						//unit: ns
#define SPEC_CLK_Prepare	((38+95)/2)				//unit: ns
#define SPEC_CLK_Zero		(500 - SPEC_CLK_Prepare)//unit: ns
#define SPEC_clk_post		52						//unit UI
#define SPEC_clk_pre		8						//unit: UI
#define SPEC_clk_trail		60 						//unit: ns
#define SPEC_hs_prepare_ns	65                      //Unit:ns
#define SPEC_hs_prepare_UI	5						//Unit: UI
#define SPEC_hs_zero		300                     //Unit:ns
#define SPEC_hs_trail		100                     //Unit: UI

extern void Panel_CK69502S0_FL7703N_Init(void);
extern void Panel_CK69502S0_FL7703N_uninit(void);

typedef struct MIPI_Panel_Driver_s
{
	void(*panel_init)(void);
	void(*panel_uninit)(void);
} MIPI_Panel_Driver_t;

static MIPI_Panel_Driver_t mipi_panel_driver =
{
	Panel_CK69502S0_FL7703N_Init,
	Panel_CK69502S0_FL7703N_uninit
};

static MIPI_Command_Table_t CK69502S0_FL7703N_initialization_setting[] =
{

    {4, {0xB9, 0xF1, 0x12, 0x87}, MIPI_GENERIC_LONG_WRITE},

    {4, {0xB2, 0x00, 0x05, 0x78}, MIPI_GENERIC_LONG_WRITE},

    {11, {0xB3, 0x10, 0x10, 0x28, 0x28, 0x03, 0xFF, 0x00, 0x00, 0x00, 0x00}, MIPI_GENERIC_LONG_WRITE},

    {2, {0xB4, 0xA0}, MIPI_GENERIC_LONG_WRITE},

    {3, {0xB5, 0x09, 0x09}, MIPI_GENERIC_LONG_WRITE},

    {3, {0xB6, 0x78, 0x78}, MIPI_GENERIC_LONG_WRITE},

    {5, {0xB8, 0x26, 0x22, 0xF0, 0x13}, MIPI_GENERIC_LONG_WRITE},

    {28, {0xBA, 0x31, 0x81, 0x05, 0xF9, 0x0E, 0x0E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x25, 0x00, 0x91, 0x0A, 0x00, 0x00, 0x01, 0x4F, 0x01, 0x00, 0x00, 0x37}, MIPI_GENERIC_LONG_WRITE},

    {2, {0xBC, 0x47}, MIPI_GENERIC_LONG_WRITE},

    {6, {0xBF, 0x02, 0x10, 0x00, 0x80, 0x04}, MIPI_GENERIC_LONG_WRITE},

    {10, {0xC0, 0x73, 0x73, 0x50, 0x50, 0x00, 0x00, 0x12, 0x73, 0x00}, MIPI_GENERIC_LONG_WRITE},

    {18, {0xC1, 0x54, 0x00, 0x32, 0x32, 0x77, 0xE4, 0x7C, 0x7C, 0xAE, 0xAE, 0xEF, 0xEF, 0x11, 0x11, 0x00, 0x00, 0x32}, MIPI_GENERIC_LONG_WRITE},

    {13, {0xC7, 0x10, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0xED, 0xC5, 0x00, 0xA5}, MIPI_GENERIC_LONG_WRITE},

    {5, {0xC8, 0x10, 0x40, 0x1E, 0x03}, MIPI_GENERIC_LONG_WRITE},

    {2, {0xCC, 0x0B}, MIPI_GENERIC_LONG_WRITE},

    {35, {0xE0, 0x00, 0x02, 0x03, 0x23, 0x38, 0x3F, 0x2D, 0x27, 0x04, 0x08, 0x0A, 0x0F, 0x11, 0x0E, 0x11, 0x12, 0x18, 0x00, 0x02, 0x03, 0x23, 0x38, 0x3F, 0x2D, 0x27, 0x04, 0x08, 0x0A, 0x0F, 0x11, 0x0E, 0x11, 0x12, 0x18}, MIPI_GENERIC_LONG_WRITE},

    {8, {0xE1, 0x11, 0x11, 0x91, 0x00, 0x00, 0x00, 0x00}, MIPI_GENERIC_LONG_WRITE},

    {15, {0xE3, 0x07, 0x07, 0x0B, 0x0B, 0x0B, 0x0B, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x04, 0xC0, 0x10}, MIPI_GENERIC_LONG_WRITE},

    {64, {0xE9, 0xC8, 0x10, 0x03, 0x04, 0x0A, 0x80, 0x81, 0x12, 0x31, 0x23, 0x4F, 0x81, 0x80, 0x81, 0x37, 0x08, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x88, 0x94, 0xF8, 0x13, 0x57, 0x18, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x94, 0xF8, 0x02, 0x46, 0x08, 0x88, 0x88, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x81, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00}, MIPI_GENERIC_LONG_WRITE},

    {64, {0xEA, 0x01, 0x10, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x23, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x0D, 0x12, 0x30, 0x30, 0x80, 0x81, 0x50, 0x80, 0x81, 0x00, 0x00, 0x00}, MIPI_GENERIC_LONG_WRITE},

    {4, {0xEF, 0xFF, 0xFF, 0x01}, MIPI_GENERIC_LONG_WRITE},

    {2, {0x11, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {25, {CMD_DELAY}, MIPI_GENERIC_LONG_WRITE}, /* Delay 250ms */

    {2, {0x29, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {5, {CMD_DELAY}, MIPI_GENERIC_LONG_WRITE}, /* Delay 50ms */
};

static MIPI_Command_Table_t CK69502S0_FL7703N_uninit_table[] =
{
    {2, {0x28, 0x00}, MIPI_DCS_LONG_WRITE},
    {1, {CMD_DELAY}, MIPI_DCS_LONG_WRITE}, /* Delay 10ms */
    {2, {0x10, 0x00}, MIPI_DCS_LONG_WRITE},
};

void Panel_CK69502S0_FL7703N_Init(void)
{
	/* Do reset LCD pannel */
	gpio_write_io(PANEL_IO_RESET, 0);
	MIPI_DSI_10MS_DELAY(3);
	gpio_write_io(PANEL_IO_RESET, 1);

	/* Set lane swap */
    drv_l1_mipi_tx_lane_swap_set(0, 0x3210);

    mipi_lcd_software_reset(15);

    mipi_write_lcd_command((void*)CK69502S0_FL7703N_initialization_setting, sizeof(CK69502S0_FL7703N_initialization_setting)/sizeof(MIPI_Command_Table_t), PACKET_LONG_WRITE);
}

void Panel_CK69502S0_FL7703N_uninit(void)
{
	//INT32U regdata;

    mipi_write_lcd_command((void*)CK69502S0_FL7703N_uninit_table, sizeof(CK69502S0_FL7703N_uninit_table)/sizeof(MIPI_Command_Table_t), PACKET_LONG_WRITE);

	MIPI_DSI_10MS_DELAY(12);

	/* Stop MIPI DSI */
    drv_l1_mipi_tx_enable(DISABLE);

	MIPI_DSI_10MS_DELAY(15);

	/* Let reset to low */
	gpio_write_io(PANEL_IO_RESET, 0);

	drv_l1_ldo28_sel(LDO_V18, DISABLE);
}

static INT32S mipi_display_CK69502S0_FL7703N_init(INT32U tft_index)
{
	INT32U	i;
	INT32U	regdata;
	INT32U	phy_hs_exit;
	INT32U	phy_lpx;
	INT32U	phy_clk_prepare, phy_clk_zero;
	INT32U	phy_clk_post, phy_clk_pre, phy_clk_trail;
	INT32U	phy_hs_prpr, phy_hs_zero, phy_hs_trail;
	INT32U	DSI_HFP,DSI_HSA,DSI_HBP;
    //INT32U  PCLK_period;
    INT32U  DSICLK_period;
    INT32U  bit_period;
    INT32U  PtoD_Ratio;
    INT32U  DSICLK;
    INT32U  PCLK,BCLK;

    drv_l1_mipi_tx_clock_en(ENABLE);
    drv_l1_ldo28_sel(LDO_V18, ENABLE);
    MIPI_DSI_10MS_DELAY(10);
#if 0  //by ming modify
    //enable MIPI TX clock
#if (LANE_NUM == 4)
    R_SYSTEM_DSI_CTRL = 0x8A8A;
#elif(LANE_NUM == 2)
    R_SYSTEM_DSI_CTRL = 0x8585;
#endif
    i = R_SYSTEM_TMDS_CTRL;
    R_SYSTEM_TMDS_CTRL = ((i & 0xC0FF) | 0x8000 | 0x0600);

    ////////////////////////////////////////////////////////////////////////////////////
    //!!!  Need to be modified if you want to change the PLL freq. or panel fps    !!!//
    //!!!  720p: 14.5us/line =>  1280-line *1.1 blanging *14.5us = 20.4ms => 49fps !!!//
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    R_SYSTEM_EXTCLK_CTRL    = 0x6300;                                   //!!! [14]=enable,  [13:8]= 0x23 => PLLEXT  = 387MHz
    R_DSI_MISC_CTRL         = 0x1066;                                   //!!! DSICLK from PLLEXT: DSICLKX8 = (PLLEXT*2)/([11:9]+1) = 774M, DSICLK=774M/8= 96.75MHz
    R_SYSTEM_TMDS_CTRL      = ((R_SYSTEM_TMDS_CTRL & 0x00FF) | 0x8500); //!!! [15]=enable,  [13:8] => PCLK = (PLLSYS/2)/([13:8]+1) = 333M/([13:8]+1)= 55.5MHz
    DSICLK = 96;///96.75;                                                     //!!! MHz;                     RULE: DSICLK > 3/2-lane * PCLK * (1.14 for blanking margin)
	PCLK   = 55;///55.5;                                                      //!!! MHz;                                96.76M > 1.5 * 55.5M * 1.14 = 94.9M
    drv_l1_mipi_tx_set_LPCK(9);                                         //!!! LP mode freq = DSICLK/(9+1) = 96.75M/10 = 9.675M < LP SPEC: 10MHz
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//========================================
	//Setting PHY BIT Width : DSI_CLK * 8 Range
#if (LANE_NUM == 4)
    drv_l1_mipi_tx_phy_frq_sel(F_375M);
#elif(LANE_NUM == 2)
    //(*((volatile INT32U *) 0xD00000CC)) = 0x0066;
    drv_l1_mipi_tx_phy_frq_sel(F_750M);                                 //!!! check DSICLKX8 freq.
#endif
	//Adjust VOUT
    //drv_l1_mipi_tx_phy_vout(0x3);
    //========================================
#else  //by ming modify
    i=drv_l1_clock_get_peripheral_clock_source_freq();
    PCLK = i/(TFT_H_PERIOD*TFT_V_PERIOD)/FRAME_RATE;
    if(PCLK>0x40)
        PCLK = 0x40;
    PCLK -=1;
    i = R_SYSTEM_TMDS_CTRL;
    //R_SYSTEM_TMDS_CTRL = ((i & 0xC0FF) | 0x8000 | 0x0800);
    R_SYSTEM_TMDS_CTRL = ((i & 0xC0FF) | 0x8000 | (PCLK<<8)); //clock set from frame rate

    MIPI_DSI_10MS_DELAY(1);
    i =PCLK;
    PCLK = drv_l1_clock_get_peripheral_clock_source_freq()/(i+1);
    i = (PCLK/10)*3*12/LANE_NUM; //DSI clk min.
    BCLK=i*8;

    if(DSI_CLK_SRC==PLLEXTx2){
        drv_l1_clock_set_ext_pll_freq((BCLK/2+9000000), 1);
        i = drv_l1_clock_get_ext_pll_freq()*2;

    }else{
        i = drv_l1_clock_get_system_pll_freq()*2;
    }
    regdata = i/BCLK;
    if((regdata>1)&&(regdata%2))
        regdata--;
    if(regdata == 6)
        regdata = 4;
    if(regdata>8)
        regdata =8;
    drv_l1_gpu_clk_setting(0,DSI_CLK_SRC,regdata);  //set BCLK/DSICLK
    BCLK = i/regdata;

    i =(BCLK/8/20000000); //
    if(BCLK%160000000)
        i++;
    if (i>0x3F)
        i=0x40;
    drv_l1_mipi_tx_set_LPCK(--i); //LP base clk is DSI clock (LP clk<20MHz)
    DSICLK = BCLK/8;
    DBG_PRINT("PCLK = %d; BCLK = %d; DSICK = %d; LPCLK dvi = %d \r\n",PCLK,BCLK,DSICLK,i);
    DSICLK/=1000000;
    PCLK /=1000000;


#endif

    MIPI_DSI_10MS_DELAY(1);




	//************************************************************//
	//Switch IO TYPE
	//***********************************************************//
	//note that TFT2 funpos need not occupied IOF
	//R_IOF_ATT |= 0x0FFF;	//IOA_ATT

	//=========================================
	//Analog PHY Enable and DSI Enable
    drv_l1_mipi_tx_enable(ENABLE);
    drv_l1_mipi_tx_phy_reset(ENABLE);
    //=========================================

	//=========================================
	//DSI Setting for LPDT Go to LP CMD Mode
	drv_l1_mipi_tx_mode_set(COMMAND_MODE);
	//=========================================

	//=========================================
	//Bus turn around ,timing Setting
	//drv_l1_mipi_tx_BTA_set(2,2,4);//TA_GET = 5, TA_SURE = 2, TA_GO = 4
	//=========================================

    drv_l1_mipi_tx_low_power_mode(ENABLE);
    drv_l1_mipi_tx_phy_reset(DISABLE);

	mipi_panel_driver.panel_init();/* Initialize panel setting */

	//****************************************
	//*****  DSI Parameters Setting     *****
	//****************************************
	//-----|____|--------|---------------------|___|----
	//     |HS_W|
	//     |  H_START    |
	//     |            HEND			|
	//	   |                 H_PERUOD          |
	//     |HSA |  HBP   |--   HACT   --|  HFP |
	//Unit : T
	//****************************************
	//TFT Timing map to DSI Function
	//****************************************



	//PCLK_period = (1000 / PCLK);
	DSICLK_period = (1000 / DSICLK);

    bit_period = (1000 / (DSICLK * 8));
    PtoD_Ratio =  DSICLK/PCLK;
    if(PtoD_Ratio == 0)
        PtoD_Ratio = 1;
    DBG_PRINT("PtoD_Ratio %d\r\n", PtoD_Ratio);

	DSI_HSA = TFT_HSA * PtoD_Ratio;
	DSI_HBP = TFT_HBP * PtoD_Ratio;
	DSI_HFP = TFT_HFP * PtoD_Ratio;

	DSI_HBP = 0x80;		/* Fix wave waveform on TFT */

	if(DSI_HSA<LANE_NUM)
		DSI_HSA = LANE_NUM;

	if(DSI_HBP<LANE_NUM)
		DSI_HBP = LANE_NUM;

    drv_l1_mipi_tx_h_timing_ctrl(DSI_HSA, DSI_HFP, DSI_HBP);
    drv_l1_mipi_tx_v_timing_ctrl(TFT_VSA, TFT_VFP, TFT_VBP, TFT_VACT);

    //************************************************//
    //*DSI Timing Setting
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
	phy_clk_trail = (SPEC_clk_trail* 2) / DSICLK_period + 1;		//unit : ns
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
    drv_l1_mipi_tx_mode_set(VIDEO_MODE);

	drv_l1_mipi_tx_lane_set(LANE_NUM);

	//**************************
	//**TFT Controller Setting**
	//**************************
	//tft2_mipi_en
	R_TFT2_CTRL = 0x0003833e;		//MODE = 3 for Parallel Mode, RGB888, TFT CLK Divider[20,21,3:1] = (1+2)=3
#if 1
	R_TFT2_V_PERIOD = TFT_V_PERIOD - 1;
	R_TFT2_VS_WIDTH = TFT_VS_WIDTH - 1;
	R_TFT2_V_START = ((0 << 16) | TFT_V_START);
	R_TFT2_V_END = ((2 << 16) | (TFT_V_END - 1));
	R_TFT2_H_PERIOD = TFT_H_PERIOD - 1;
	R_TFT2_HS_WIDTH = TFT_HS_WIDTH - 1;
	R_TFT2_H_START = ((1 << 16) | (TFT_H_START + 1));
	R_TFT2_H_END = ((4 << 16) | TFT_H_END);
#else
	R_TFT2_V_PERIOD = TFT_V_PERIOD - 1;
	R_TFT2_VS_WIDTH = TFT_VS_WIDTH - 1;
	R_TFT2_V_START = (((0 << 16) | TFT_V_START) << 16);
	R_TFT2_V_END = (((2 << 16) | (TFT_V_END - 1)) << 16);
	R_TFT2_H_PERIOD = TFT_H_PERIOD - 1;
	R_TFT2_HS_WIDTH = TFT_HS_WIDTH - 1;
	R_TFT2_H_START = (((1 << 16) | TFT_H_START + 1) << 16);
	R_TFT2_H_END = (((4 << 16) | TFT_H_END) << 16);
#endif

    R_TFT2_CTRL |= (1<<31);//switching to bank1
	R_TFT2_LINE_RGB_ORDER = 0x0;		//RGB order
	R_TFT2_CTRL &= ~(1<<31);//switching to bank0

	//R_TFT2_INTP_OUT_RESO = (((IMG_HEIGHT) << 16) + IMG_WIDTH);
	R_TFT2_SCA_OUT_RES = (((IMG_HEIGHT) << 16) + IMG_WIDTH);

	//P_TFT_FBI_ADDR = 0x100000; //image source for TFT Controller input.
	//R_FREE_SIZE = 0x14000F0; //320x240,,,,for 480x800

	R_PPU_ENABLE = 0x80;

	////////////////////////////////////////////////////////////////////////////////////

	regdata = drv_l1_mipi_tx_get_status();
	while(!(regdata & 0x1))			//wait TXPHY stop
	{
		regdata = drv_l1_mipi_tx_get_status();
	}

	regdata = R_TFT2_CTRL;
	R_TFT2_CTRL = regdata | 0x1;

	return STATUS_OK;
}

const DispCtrl_t TFT_Param_CK69502S0_FL7703N =
{
    /* lcd size */
	TFT_HACT,
	TFT_VACT,

    0,  //sync_pin_enable
	0,  //de_pin_enable
	0,  //te_pin_enable
	0,  //tft2_io_enable

	0,  //h_off_lines_video
	0,  //h_off_lines_cap

	0,  //en_crop_video
	0,  //en_crop_cap
	0,  //pbp_crop_video
	0,  //pbp_crop_cap
	1,  //back_ratio_video
	1,  //back_ratio_cap

	/* pip size */
#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
    320,
	160,
#else
	480,
	240,
#endif
	/* pip position */
#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
    -320,   //x, must be multiple of 8
	0,      //y
#else
	-480,   //x, must be multiple of 8
	0,      //y
#endif

	/* lcd init & uninit */
	mipi_display_CK69502S0_FL7703N_init,
	Panel_CK69502S0_FL7703N_uninit,

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
    TFT_ROT_RIGHT,

    /*  tft width & height  */
#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
    480,
    800
#else
    TFT_HACT,
    TFT_VACT
#endif
};
#endif

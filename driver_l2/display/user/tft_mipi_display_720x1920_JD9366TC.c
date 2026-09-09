#include "drv_l2.h"

#if (defined LCD_DRV_EN_JD9366TC) && (LCD_DRV_EN_JD9366TC == 1)

#define PANEL_IO_RESET				IO_A8

#define LANE_NUM	    2

#define IMG_WIDTH		720
#define IMG_HEIGHT		1920
#define TFT_HACT		IMG_WIDTH
#define TFT_VACT		IMG_HEIGHT

#define TFT_HS_WIDTH		8//	20							//*20
#define TFT_H_START		(TFT_HS_WIDTH + 35)			//*20+30 =50
#define TFT_H_END		(TFT_H_START + IMG_WIDTH)	//*50+720 = 770
#define TFT_H_PERIOD	(TFT_H_END + 30)			//*770+30 = 800

#define TFT_VS_WIDTH	8//(17 + 1)						//*4
#define TFT_V_START		(TFT_VS_WIDTH + 7 + 1)		//*4+7+1 = 12
#define TFT_V_END		(TFT_V_START + IMG_HEIGHT) 	//*12+1280 = 1292
#define TFT_V_PERIOD	(TFT_V_END + 16 + 1)		//*1292+16+1 = 1309

#define TFT_VSA         TFT_VS_WIDTH
#define TFT_VBP         TFT_V_START - TFT_VS_WIDTH
#define TFT_VFP         TFT_V_PERIOD - TFT_V_END

#define TFT_HSA         TFT_HS_WIDTH
#define TFT_HBP         TFT_H_START - TFT_HS_WIDTH
#define TFT_HFP         TFT_H_PERIOD - TFT_H_END
//*********************************
//DSI Timing Spec
//**********************************
#define SPEC_HS_Exit		200						//unit: ns
#define SPEC_LPX			65						//unit: ns
#define	SPEC_CLK_Prepare	((38+95)/2)				//unit: ns
#define	SPEC_CLK_Zero		(500 - SPEC_CLK_Prepare)//unit: ns
#define SPEC_clk_post		52						//unit UI
#define	SPEC_clk_pre		8						//unit: UI
#define SPEC_clk_trail		60 						//unit: ns
#define SPEC_hs_prepare_ns	65						//Unit:ns
#define SPEC_hs_prepare_UI	5						//Unit: UI
#define SPEC_hs_zero		300						//Unit:ns
#define SPEC_hs_trail		100						//Unit: UI

typedef struct MIPI_Panel_Driver_s
{
	void(*panel_init)(void);
	void(*panel_uninit)(void);
} MIPI_Panel_Driver_t;

extern void Panel_JD9366TC_Init(void);

static MIPI_Panel_Driver_t mipi_720p_panel_driver =
{
	Panel_JD9366TC_Init,
	NULL
};

static MIPI_Command_Table_t jd9366tc_initialization_setting[] =
{
	{2,{0x30,0x01}, MIPI_GENERIC_LONG_WRITE},					//Page 1
	{5,{0x78,0x49,0x61,0x02,0x00}, MIPI_GENERIC_LONG_WRITE},	//Inhouse register enbale
	{2,{0x30,0x02}, MIPI_GENERIC_LONG_WRITE},					//Page 2
	{2,{0x31,0x22}, MIPI_GENERIC_LONG_WRITE},					//ckv_width, stv_width	//2 4-line
	{2,{0x32,0x08}, MIPI_GENERIC_LONG_WRITE},					//ckv_phase	//Phase=8
	{2,{0x33,0x31}, MIPI_GENERIC_LONG_WRITE},					//rst_width, rst_dly_sft	//2: 4-line, dly=0 line  
	{2,{0x3d,0x42}, MIPI_GENERIC_LONG_WRITE},					//flc toggle by N-frmare	//66-frames
	{2,{0x3E,0x67}, MIPI_GENERIC_LONG_WRITE},					//flc non-overlap time
	{2,{0x3f,0x60}, MIPI_GENERIC_LONG_WRITE},					//CKV_dummy number		//R3F[0]=1, Add one more 12-phase ckv number
	{2,{0x42,0x83}, MIPI_GENERIC_LONG_WRITE},					//FLC stage en=1
	{2,{0x43,0xF0}, MIPI_GENERIC_LONG_WRITE},					//STV_ckv_dis extern setting
	{2,{0x44,0x21}, MIPI_GENERIC_LONG_WRITE},					//STV_ckv_dis 	//shift distance between STV1 rising and CKV1 rising, LSB=1, STV1 rising - CKV rising = 2H
	{2,{0x49,0xA1}, MIPI_GENERIC_LONG_WRITE},					//grst_adv_sft 	//RST will shit more 2H forward DE side. 
	{2,{0x40,0x38}, MIPI_GENERIC_LONG_WRITE},					//GIP EQ
	{13,{0x41,0x50,0x50,0x08,0x08,0x50,0x50,0x08,0x08,0x50,0x50,0x08,0x08}, MIPI_GENERIC_LONG_WRITE},		//STV/ CKV/ RST rise/ fall time setting, modify @ 20220601
	{12,{0x5A,0x23,0x23,0x23,0x23,0x23,0x22,0x23,0x18,0x00,0x21,0x00}, MIPI_GENERIC_LONG_WRITE},			//GS=0, gout_sel for left side   	//left for slave ( slave: even gate line )          
	{12,{0x5B,0x08,0x0A,0x0C,0x0E,0x04,0x23,0x23,0x23,0x23,0x23,0x23}, MIPI_GENERIC_LONG_WRITE},			//GS=0, gout_sel for left side   	//left for slave ( slave: even gate line ) 
	{12,{0x5C,0x23,0x23,0x23,0x23,0x23,0x22,0x23,0x19,0x01,0x21,0x20}, MIPI_GENERIC_LONG_WRITE},	//GS=0, gout_sel for right side   //right for master ( Master: odd gate line )  
	{12,{0x5D,0x09,0x0B,0x0D,0x0F,0x05,0x23,0x23,0x23,0x23,0x23,0x23}, MIPI_GENERIC_LONG_WRITE},			//GS=0, gout_sel for right side    //right for master ( Master: odd gate line )    
	{12,{0x5E,0x23,0x23,0x23,0x23,0x23,0x23,0x22,0x05,0x01,0x21,0x20}, MIPI_GENERIC_LONG_WRITE},		//GS=1, gout_sel for left side 		//left for slave ( slave: odd gate line ) 
	{12,{0x5F,0x0F,0x0D,0x0B,0x09,0x19,0x23,0x23,0x23,0x23,0x23,0x23}, MIPI_GENERIC_LONG_WRITE},	//GS=1, gout_sel for left side         	//left for slave ( slave: odd gate line )
	{12,{0x60,0x23,0x23,0x23,0x23,0x23,0x23,0x22,0x04,0x00,0x21,0x20}, MIPI_GENERIC_LONG_WRITE},		//GS=1, gout_sel for right side 	//right for Master ( Master: even gate line )
	{12,{0x61,0x0E,0x0C,0x0A,0x08,0x18,0x23,0x23,0x23,0x23,0x23,0x23}, MIPI_GENERIC_LONG_WRITE},		//GS=1, gout_sel for right side    	//right for Master ( Master: even gate line )
	{2,{0x68,0x80}, MIPI_GENERIC_LONG_WRITE},				//FLC_keep_en=1
	{4,{0x64,0xff,0xff,0x7f}, MIPI_GENERIC_LONG_WRITE},		//APO mapping
	{4,{0x65,0xff,0xff,0x7f}, MIPI_GENERIC_LONG_WRITE},	
	{4,{0x6f,0x00,0x04,0x00}, MIPI_GENERIC_LONG_WRITE},		//Power on mapping
	{4,{0x70,0x00,0x04,0x00}, MIPI_GENERIC_LONG_WRITE},
	{4,{0x71,0xff,0xff,0x3f}, MIPI_GENERIC_LONG_WRITE},		//Power off mapping
	{4,{0x72,0xff,0xff,0x03}, MIPI_GENERIC_LONG_WRITE},	
	{2,{0x30,0x07}, MIPI_GENERIC_LONG_WRITE},					//Page 7
	{2,{0x31,0xC0}, MIPI_GENERIC_LONG_WRITE},					//r_lp11_check_enb: low power mode check enb. //0x4D from hsinchu, HS fail

	//Long V setting	
	{2,{0x30,0x08}, MIPI_GENERIC_LONG_WRITE},					//Page 8
	{2,{0x33,0x05}, MIPI_GENERIC_LONG_WRITE},					//SLPIN sync vsync
	{2,{0x40,0x50}, MIPI_GENERIC_LONG_WRITE},					//hbp
	{2,{0x41,0x80}, MIPI_GENERIC_LONG_WRITE}, 				//hfp
	{2,{0x42,0x1A}, MIPI_GENERIC_LONG_WRITE},					//vbp, like line delay used
	{2,{0x47,0x0A}, MIPI_GENERIC_LONG_WRITE},					//hole time, 100us
	{2,{0x48,0x0C}, MIPI_GENERIC_LONG_WRITE},		  			//hole number
	{2,{0x50,0x14}, MIPI_GENERIC_LONG_WRITE},					//Long V used for Hole start after DE 
	{2,{0x5A,0xD0}, MIPI_GENERIC_LONG_WRITE},					//RSO_X[7:0] setting, 
	{2,{0x5B,0x80}, MIPI_GENERIC_LONG_WRITE},					//RSO_Y[7:0] setting, 
	{2,{0x5C,0x72}, MIPI_GENERIC_LONG_WRITE},					//RSO_Y[11:8], RSO_X[11:8]
	{2,{0x62,0x04}, MIPI_GENERIC_LONG_WRITE},	 				//R62h[2]:cascade_slave_sync_en=1, R62h[0]:Long H enable=1 
	{2,{0x65,0x5F}, MIPI_GENERIC_LONG_WRITE},					//OCSD setting to 40MHZ

	{2,{0x5e,0x00}, MIPI_GENERIC_LONG_WRITE},					//DSI PN, DSWAP setting		//0x00: not enable function.

	#if LANE_NUM == 2
	{2,{0x5D,0x0D}, MIPI_GENERIC_LONG_WRITE},					// Add @20231024 for 2 lane applicaiton ; 	Internal lane selection enable  
	{2,{0x5F,0x01}, MIPI_GENERIC_LONG_WRITE},			
	#endif

	{2,{0x73,0x01}, MIPI_GENERIC_LONG_WRITE},					//CRC_MUX enable	//R73h[2:1]: CRC_MUX_SEL: 00->SRAM, 01->Coleng, 10->dgamma	//R73h[0]: CRC_EN
	{2,{0x30,0x0a}, MIPI_GENERIC_LONG_WRITE},					//Page A
	{2,{0x33,0x30}, MIPI_GENERIC_LONG_WRITE},					//SDT
	{2,{0x3f,0x50}, MIPI_GENERIC_LONG_WRITE},					//SD overdrive/ OP driving/ POL switching time 	//R3F[7:6]: SD_POLT[1:0]: POL switching time 	//R3F[5:4]: SD_DRVT[1:0]: SD OP driving nonoverlap time //R3F[2:0]: SD_OVDT[2:0]: SD overdrive time
	{2,{0x47,0x20}, MIPI_GENERIC_LONG_WRITE},
	{2,{0x48,0x80}, MIPI_GENERIC_LONG_WRITE},						
	{2,{0x49,0x03}, MIPI_GENERIC_LONG_WRITE},					//SD inversion type & Ziazag type selection   //R49h[5:4]: inverson type: 00->col, 01->1 dot, 02-> 1+2dot, 03->4 dot, 04-> 2dot, 05: ziazag //R49h[2:0]: ZZ_2pl, zz_lr, zz_eo
	{2,{0x30,0x0b}, MIPI_GENERIC_LONG_WRITE},					// Page B
	{3,{0x33,0x00,0x75}, MIPI_GENERIC_LONG_WRITE},			//VCOM_SEL: 10mv/step, 0~-3V
	{3,{0x3c,0x00,0x7D}, MIPI_GENERIC_LONG_WRITE},			//VCOM_R_SEL: 10mv/step, 0~-3V
	{2,{0x43,0x9F}, MIPI_GENERIC_LONG_WRITE},					//VGMP & VGM_EN_M & VGM_EN_S	//R43h[7]:  Master VGM enable //R43h[6]:  Slave VGM enable //R43h[5:0]:VGMP_S: 0x00->3.05V, 50mv/step,

	{2,{0x44,0x1F}, MIPI_GENERIC_LONG_WRITE},					//VGMN	//R44h[5:0]:VGMN_S: 0x00-> -3.05V, 50mv/step,  
	{6,{0x3E,0x00,0x03,0x09,0x0F,0x13}, MIPI_GENERIC_LONG_WRITE},							//Gamma setting for P-pol     
	{15,{0x3f,0x1B,0x2C,0x31,0x36,0x33,0x4C,0x52,0x57,0x66,0x62,0x69,0x72,0x86,0x84}, MIPI_GENERIC_LONG_WRITE},
	{10,{0x40,0x43,0x4E,0x51,0x65,0x00,0x03,0x09,0x0F,0x13}, MIPI_GENERIC_LONG_WRITE},
	{15,{0x41,0x1B,0x2C,0x31,0x36,0x33,0x4C,0x52,0x57,0x66,0x62,0x69,0x72,0x86,0x84}, MIPI_GENERIC_LONG_WRITE},		//Gamma setting for N-pol   
	{5,{0x42,0x43,0x4E,0x7E,0x73}, MIPI_GENERIC_LONG_WRITE},
	{2,{0x45,0x66}, MIPI_GENERIC_LONG_WRITE},					//VGHO=17.5V	//R45h[6:0]: 0x00->7.3V, 0.1v/step,
	{2,{0x46,0x2F}, MIPI_GENERIC_LONG_WRITE},					//VGLO = -10V	//R45h[6:0]: 0x00-> -5.3V, -0.1v/step,
	{2,{0x48,0x38}, MIPI_GENERIC_LONG_WRITE},					//VGH=18V		//R48h[6:0]: 0x00->7V, 0.2v/step
	{2,{0x49,0x15}, MIPI_GENERIC_LONG_WRITE},					//VGL=-11		//R48h[5:0]: 0x00-> -6.8V, 0.2v/step,
	{2,{0x4A,0x2A}, MIPI_GENERIC_LONG_WRITE},					//VGH_RT & VGL_RT		//R4Ah[5:3]: VGH_RT, 00:1.5X ~ 07:5x   //R4Ah[2:0]: VGL_RT, 00->1.5x (VCL-AVDD), 01->2x(AVEE-AVDD), 10->2.5x(AVEE+VCL-AVDD), 11->3x(2AVEE-AVDD)
	{2,{0x30,0x0C}, MIPI_GENERIC_LONG_WRITE},					//Page C
	{2,{0x32,0x62}, MIPI_GENERIC_LONG_WRITE},					//DPHY_HSRX_ALL_ISEL, DPHY_HSRX_STL		//DPHY_HSRX_ALL_ISEL[2:0]=001 -> 110
	{2,{0x71,0x44}, MIPI_GENERIC_LONG_WRITE},
	{2,{0x30,0x0D}, MIPI_GENERIC_LONG_WRITE},					//Page D
	{2,{0x4C,0x74}, MIPI_GENERIC_LONG_WRITE},					//R4Ch[6]: VGM_MC_EN		 //0x34(default), VGM_MC_EN=1, VGMP/N not output to pad.
	{2,{0x30,0x08}, MIPI_GENERIC_LONG_WRITE},					//Page 8
	{2,{0x52,0xC3}, MIPI_GENERIC_LONG_WRITE},					//Enable TE1	  //R52h[6]: TE1 enable
	{2,{0x54,0x20}, MIPI_GENERIC_LONG_WRITE},					//Enable TE1 pulse width
	{2,{0x30,0x00}, MIPI_GENERIC_LONG_WRITE},					//Page 0
	
	{2,{0x36,0x02}, MIPI_GENERIC_LONG_WRITE},				//Display Flip Horizontal
		
	{1,{0x11}, MIPI_DCS_SHORT_WRITE_0P},					//SLP OUT		// SLPOUT
	{12, {CMD_DELAY}, MIPI_GENERIC_LONG_WRITE},				/* Delay 120 ms */
	{1,{0x29}, MIPI_DCS_SHORT_WRITE_0P},					//DISP ON		// DSPON	
	{5, {CMD_DELAY}, MIPI_GENERIC_LONG_WRITE},				/* Delay 50 ms */
	{2,{0x35,0x00}, MIPI_GENERIC_LONG_WRITE},					//TE ON

};


void Panel_JD9366TC_Init(void)
{
	/* Do reset LCD pannel */
	gpio_write_io(PANEL_IO_RESET, 0);
	MIPI_DSI_10MS_DELAY(3);
	gpio_write_io(PANEL_IO_RESET, 1);

	/* Set lane swap */
	drv_l1_mipi_tx_lane_swap_set(0, 0x3210);

	mipi_lcd_software_reset(15);

	/* Set panel command */
	mipi_write_lcd_command((void*)jd9366tc_initialization_setting, sizeof(jd9366tc_initialization_setting)/sizeof(MIPI_Command_Table_t), PACKET_LONG_WRITE);
}


static INT32S mipi_display_JD9366TC_720p_init(INT32U tft_index)
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
    INT32U  BCLK_period;
    INT32U  bit_period;
    INT32U  PtoD_Ratio;
    INT32U  BCLK;
    INT32U  PCLK;
    INT32U  DSICLK, DSICLK_period;
	
    drv_l1_mipi_tx_clock_en(ENABLE);
    drv_l1_ldo28_sel(LDO_V18, ENABLE);
    MIPI_DSI_10MS_DELAY(10);

    //enable MIPI TX clock
    regdata = R_SYSTEM_DSI_CTRL;
    regdata &= 0x00FF;
#if (LANE_NUM == 4)
    R_SYSTEM_DSI_CTRL = 0x8A8A;
#elif(LANE_NUM == 2)
    ///R_SYSTEM_DSI_CTRL = 0x8500;
    regdata |= 0x8500;
#endif
    R_SYSTEM_DSI_CTRL = regdata;

    ///i = R_SYSTEM_TMDS_CTRL;
    ///R_SYSTEM_TMDS_CTRL = ((i & 0xC0FF) | 0x8000 | 0x0A00);
    R_SYSTEM_EXTCLK_CTRL    = 0x6300;                                   //!!! [14]=enable,  [13:8]= 0x23 => PLLEXT  = 387MHz
    R_DSI_MISC_CTRL         = 0x1066;                                   //!!! DSICLK from PLLEXT: DSICLKX8 = (PLLEXT*2)/([11:9]+1) = 774M, DSICLK=774M/8= 96.75MHz
    R_SYSTEM_TMDS_CTRL      = ((R_SYSTEM_TMDS_CTRL & 0x00FF) | 0x8600); //!!! [15]=enable,  [13:8] => PCLK = (PLLSYS/2)/([13:8]+1) = 333M/([13:8]+1)= 55.5MHz
    drv_l1_mipi_tx_set_LPCK(9);                                         //!!! LP mode freq = DSICLK/(9+1) = 96.75M/10 = 9.675M < LP SPEC: 10MHz

    MIPI_DSI_10MS_DELAY(1);

    ///drv_l1_mipi_tx_set_LPCK(3);
#if 0
    DSICLK = 96;                                                        //!!! MHz;                     RULE: DSICLK > 3/2-lane * PCLK * (1.14 for blanking margin)
	PCLK   = 55;                                                        //!!! MHz;                                96.76M > 1.5 * 55.5M * 1.14 = 94.9M
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//========================================
	//Setting PHY BIT Width : DSI_CLK * 8 Range
#if (LANE_NUM == 4)
    drv_l1_mipi_tx_phy_frq_sel(F_500M);
#elif(LANE_NUM == 2)
    drv_l1_mipi_tx_phy_frq_sel(F_1000M);                                 //!!! check DSICLKX8 freq.
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

    mipi_720p_panel_driver.panel_init();/* Initialize panel setting */

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

    regdata = drv_l1_mipi_tx_get_status();
    while(!(regdata & 0x1))			//wait TXPHY stop
    {
        regdata = drv_l1_mipi_tx_get_status();
    }

    //[b0]: TFT2_EN
    R_TFT2_CTRL |= 0x1;
	return STATUS_OK;
#else
	//========================================
	//Setting PHY BIT Width : DSI_CLK * 8 Range
#if (LANE_NUM == 4)
    drv_l1_mipi_tx_phy_frq_sel(F_375M);
#elif(LANE_NUM == 2)
    drv_l1_mipi_tx_phy_frq_sel(F_750M);
#endif
	//Adjust VOUT
    //drv_l1_mipi_tx_phy_vout(0x3);
    //========================================

	//************************************************************//
	//Switch IO TYPE
	//***********************************************************//
	//note that TFT2 funpos need not occupied IOF
	R_IOF_ATT |= 0x0FFF;	//IOA_ATT

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

	mipi_720p_panel_driver.panel_init();/* Initialize panel setting */

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

	BCLK = 50;
	PCLK = 50;

	//PCLK_period = (1000 / PCLK);
	BCLK_period = (1000 / BCLK);

    bit_period = (1000 / (BCLK * 8));
    PtoD_Ratio =  BCLK/PCLK;
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
   	//phy_hs_exit = (200 / BCLK_period) +1;
	//phy_lpx = (65 / BCLK_period) + 1;
	phy_hs_exit = (SPEC_HS_Exit/ BCLK_period) +1;
	phy_lpx = ( SPEC_LPX/ BCLK_period) + 1;
    drv_l1_mipi_tx_lane_timing_ctrl(phy_hs_exit, phy_lpx);
   	//========================================

   	//========================================
    //phy_clk_prepare = ((38 + 95) / 2) / BCLK_period + 1; //(max+min)/2
	//phy_clk_zero = (500 - phy_clk_prepare) / BCLK_period + 1;	//??unit not right ?? 500 ns, phy_clk_prepare unit is dsi clk
	phy_clk_prepare = (SPEC_CLK_Prepare / BCLK_period) +1;
	phy_clk_zero =	(SPEC_CLK_Zero / BCLK_period)+1;
    drv_l1_mipi_tx_clock_lane_timing_ctrl0(phy_clk_prepare, phy_clk_zero);
    //========================================

    //========================================
	//phy_clk_post = ((60 + 52 * bit_period) * 2) / BCLK_period;
	//phy_clk_pre = (8 * bit_period * 2) / BCLK_period + 1;
	//phy_clk_trail = (60 * 2) / BCLK_period + 1;
	phy_clk_post = 	(SPEC_clk_post * 2);	//unit UI
	phy_clk_pre = 	(SPEC_clk_pre * 2) ;	//unit UI
	phy_clk_trail = (SPEC_clk_trail* 2) / BCLK_period + 1;		//unit : ns
	drv_l1_mipi_tx_clock_lane_timing_ctrl1(phy_clk_trail, phy_clk_pre, phy_clk_post);
	//========================================

 	//========================================
    phy_hs_zero = (SPEC_hs_zero/BCLK_period) +1;
    phy_hs_prpr = (SPEC_hs_prepare_ns + (SPEC_hs_prepare_UI*bit_period)) / BCLK_period +1;
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
	R_TFT2_CTRL = 0x00038334;		//MODE = 3 for Parallel Mode, RGB888, TFT CLK Divider = 2`
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
#endif
}

// tft table
const DispCtrl_t TFT_Param_JD9366TC =
{
	/* lcd size */
		TFT_HACT,
		TFT_VACT,
	
		0,	//sync_pin_enable
		0,	//de_pin_enable
		0,	//te_pin_enable
		0,	//tft2_io_enable
	
		0,	//h_off_lines_video
		0,	//h_off_lines_cap
	
		0,	//en_crop_video
		0,	//en_crop_cap
		0,	//pbp_crop_video
		0,	//pbp_crop_cap
		0,	//back_ratio_video
		0,	//back_ratio_cap
	
		/* pip size */
		480,
		300,
	
		/* pip position */
		-480,	//x, must be multiple of 8
		0,		//y
	
		/* lcd init & uninit */
		mipi_display_JD9366TC_720p_init,
		NULL,
	
		/* lcd backlight control */
		LCD_BL_CTRL_CONSTANT,
		20 * 1000,	// in HZ for LCD_BL_CTRL_PWM
		55, 		// in duty cycle for LCD_BL_CTRL_PWM
	
		/* tft format */
    #if RESOURCE_FORMAT == RESOURCE_GP420
		DISP_FMT_GP420,
	#elif RESOURCE_FORMAT == RESOURCE_RGB565
		DISP_FMT_RGB565,
	#elif RESOURCE_FORMAT == RESOURCE_YUYV
		DISP_FMT_YUYV,
	#endif
	
		/* tft IFCE 	*/
		LCD_IFCE_RGB,
	
		/* tft rotate	*/
		TFT_ROT_RIGHT/*TFT_ROT_LEFT*/,
	
		/*	tft width & height	*/
		TFT_HACT,
		TFT_VACT
};
#endif

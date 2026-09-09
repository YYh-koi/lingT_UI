#include "drv_l2.h"

#if (defined LCD_DRV_EN_KD055HD_IL9881C) && (LCD_DRV_EN_KD055HD_IL9881C == 1)

#define PANEL_IO_RESET				IO_A8


#define LANE_NUM	    2

#define IMG_WIDTH		720
#define IMG_HEIGHT		1280
#define TFT_HACT		IMG_WIDTH
#define TFT_VACT		IMG_HEIGHT

#define TFT_HS_WIDTH	20							//*20
#define TFT_H_START		(TFT_HS_WIDTH + 35)			//*20+30 =50
#define TFT_H_END		(TFT_H_START + IMG_WIDTH)	//*50+720 = 770
#define TFT_H_PERIOD	(TFT_H_END + 30)			//*770+30 = 800

#define TFT_VS_WIDTH	(17 + 1)						//*4
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

extern void Panel_KD055HD_IL9881C_Init(void);
extern void Panel_KD055HD_IL9881C_uninit(void);

static MIPI_Panel_Driver_t mipi_720p_panel_driver =
{
	Panel_KD055HD_IL9881C_Init,
	Panel_KD055HD_IL9881C_uninit
};

static MIPI_Command_Table_t KD055HD_IL9881C_initialization_setting[] =
{
	{4, 	{0xFF, 0x98, 0x81, 0x03}, MIPI_GENERIC_LONG_WRITE},//PAGE 3

    {2, 	{0x01, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x02, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x03, 0x72}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x04, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x05, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x06, 0x09}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x07, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x08, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x09, 0x01}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x0A, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x0B, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x0C, 0x01}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x0D, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x0E, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x0F, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x10, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x11, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x12, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x13, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x14, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x15, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x16, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x17, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x18, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x19, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x1A, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x1B, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x1C, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x1D, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x1E, 0x40}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x1F, 0x80}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x20, 0x05}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x21, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x22, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x23, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x24, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x25, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x26, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x27, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x28, 0x33}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x29, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x2A, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x2B, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x2C, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x2D, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x2E, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x2F, 0x00}, MIPI_GENERIC_LONG_WRITE},

	{2, 	{0x30, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x31, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x32, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x33, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x34, 0x04}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x35, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x36, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x37, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x38, 0x3C}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x39, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x3A, 0x40}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x3B, 0x40}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x3C, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x3D, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x3E, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x3F, 0x00}, MIPI_GENERIC_LONG_WRITE},

	{2, 	{0x40, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x41, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x42, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x43, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x44, 0x00}, MIPI_GENERIC_LONG_WRITE},


	{2, 	{0x50, 0x01}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x51, 0x23}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x52, 0x45}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x53, 0x67}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x54, 0x89}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x55, 0xAB}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x56, 0x01}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x57, 0x23}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x58, 0x45}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x59, 0x67}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x5A, 0x89}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x5B, 0xAB}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x5C, 0xCD}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x5D, 0xEF}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x5E, 0x11}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x5F, 0x01}, MIPI_GENERIC_LONG_WRITE},

	{2, 	{0x60, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x61, 0x15}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x62, 0x14}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x63, 0x0E}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x64, 0x0F}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x65, 0x0C}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x66, 0x0D}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x67, 0x06}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x68, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x69, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x6A, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x6B, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x6C, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x6D, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x6E, 0x07}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x6F, 0x02}, MIPI_GENERIC_LONG_WRITE},

	{2, 	{0x70, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x71, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x72, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x73, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x74, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x75, 0x01}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x76, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x77, 0x14}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x78, 0x15}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x79, 0x0E}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x7A, 0x0F}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x7B, 0x0C}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x7C, 0x0D}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x7D, 0x06}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x7E, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x7F, 0x07}, MIPI_GENERIC_LONG_WRITE},

	{2, 	{0x80, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x81, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x82, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x83, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x84, 0x07}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x85, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x86, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x87, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x88, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x89, 0x02}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x8A, 0x02}, MIPI_GENERIC_LONG_WRITE},

	{4, 	{0xFF, 0x98, 0x81, 0x04}, MIPI_GENERIC_LONG_WRITE},//PAGE4

	{2, 	{0x6C, 0x15}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x6E, 0x2A}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x6F, 0x33}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x3A, 0x94}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x8D, 0x1A}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x87, 0xBA}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0x26, 0x76}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xB2, 0xD1}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xB5, 0x06}, MIPI_GENERIC_LONG_WRITE},

	{4, 	{0xFF, 0x98, 0x81, 0x01}, MIPI_GENERIC_LONG_WRITE},//PAGE1

	{2, 	{0x22, 0x0A}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0x31, 0x00}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0x53, 0x8A}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0x55, 0x8A}, MIPI_GENERIC_LONG_WRITE},

	{2, 	{0x50, 0xAE}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0x51, 0xAE}, MIPI_GENERIC_LONG_WRITE},

	{2, 	{0x60, 0x28}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0x61, 0x00}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0x62, 0x19}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0x63, 0x10}, MIPI_GENERIC_LONG_WRITE},

	{2, 	{0xA0, 0x0F}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xA1, 0x1B}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xA2, 0x28}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xA3, 0x12}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xA4, 0x15}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xA5, 0x28}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xA6, 0x1B}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xA7, 0x1E}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xA8, 0x79}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xA9, 0x1B}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xAA, 0x27}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xAB, 0x69}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xAC, 0x19}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xAD, 0x18}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xAE, 0x4C}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xAF, 0x21}, MIPI_GENERIC_LONG_WRITE},

	{2, 	{0xB0, 0x28}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xB1, 0x52}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xB2, 0x65}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xB3, 0x3F}, MIPI_GENERIC_LONG_WRITE},
#if LANE_NUM == 2
    {2, 	{0xB6, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {2, 	{0xB7, 0x03}, MIPI_GENERIC_LONG_WRITE},
#endif
	{2, 	{0xC0, 0x04}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xC1, 0x1B}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xC2, 0x27}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xC3, 0x13}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xC4, 0x15}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xC5, 0x28}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xC6, 0x1C}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xC7, 0x1E}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xC8, 0x79}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xC9, 0x1A}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xCA, 0x27}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xCB, 0x69}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xCC, 0x1A}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xCD, 0x18}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xCE, 0x4C}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xCF, 0x21}, MIPI_GENERIC_LONG_WRITE},

	{2, 	{0xD0, 0x27}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xD1, 0x52}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xD2, 0x65}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0xD3, 0x3F}, MIPI_GENERIC_LONG_WRITE},

	{4, 	{0xFF, 0x98, 0x81, 0x00}, MIPI_GENERIC_LONG_WRITE},//PAGE0

	{2, 	{0x35, 0x00}, MIPI_GENERIC_LONG_WRITE},
	{2, 	{0x3A, 0x70}, MIPI_GENERIC_LONG_WRITE},

	{2,  	{0x11, 0x00}, MIPI_GENERIC_LONG_WRITE},			/* Sleep out */
	{15, 	{CMD_DELAY}, MIPI_GENERIC_LONG_WRITE},			/* Delay 150ms */
	{2, 	{0x29, 0x00}, MIPI_GENERIC_LONG_WRITE},			/* Display on */
	{3, 	{CMD_DELAY}, MIPI_GENERIC_LONG_WRITE},			/* Delay 150ms */
};

static MIPI_Command_Table_t KD055HD_IL9881C_uninit_table[] =
{
    {2, 	{0x28, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {1, 	{CMD_DELAY}, MIPI_GENERIC_LONG_WRITE},			/* Delay 10ms */
    {2, 	{0x10, 0x00}, MIPI_GENERIC_LONG_WRITE},
};


#if C_ASYN_DBLE_TFT
void set_KD055HD_IL9881C_display_for_double_TFT(void);
void clr_KD055HD_IL9881C_display_for_double_TFT(void);
INT32U get_KD055HD_IL9881C_display_for_double_TFT(void);

INT32U KD055HD_IL9881C_display_enable_flag=0;					/* if this flag is  "1" -> KD055HD_IL9881C tft enable ; if this flag is "0" -> KD055HD_IL9881C tft disable */

void set_KD055HD_IL9881C_display_for_double_TFT(void){
	KD055HD_IL9881C_display_enable_flag=1;
}
void clr_KD055HD_IL9881C_display_for_double_TFT(void){
	KD055HD_IL9881C_display_enable_flag=0;
	gpio_write_io(PANEL_IO_BL_ON,DATA_LOW);
}
INT32U get_KD055HD_IL9881C_display_for_double_TFT(void){
	return KD055HD_IL9881C_display_enable_flag;
}
#endif


void Panel_KD055HD_IL9881C_Init(void)
{
	DBG_PRINT("Enter Panel_KD055HD_IL9881C_Init\r\n");
	/* Do reset LCD pannel */
	gpio_write_io(PANEL_IO_RESET, 0);
	MIPI_DSI_10MS_DELAY(3);
	gpio_write_io(PANEL_IO_RESET, 1);

	/* Set lane swap */
    drv_l1_mipi_tx_lane_swap_set(0, 0x3210);

    mipi_lcd_software_reset(15);

    mipi_write_lcd_command((void*)KD055HD_IL9881C_initialization_setting, sizeof(KD055HD_IL9881C_initialization_setting)/sizeof(MIPI_Command_Table_t), PACKET_LONG_WRITE);
}

void Panel_KD055HD_IL9881C_uninit(void)
{
	#if 0	/* do not use this code that will cause inevitable error */
    	mipi_write_lcd_command((void*)KD055HD_IL9881C_uninit_table, sizeof(KD055HD_IL9881C_uninit_table)/sizeof(MIPI_Command_Table_t), PACKET_LONG_WRITE);
	MIPI_DSI_10MS_DELAY(12);

	/* Stop MIPI DSI */
	drv_l1_mipi_tx_enable(DISABLE);

	MIPI_DSI_10MS_DELAY(15);

	/* Let reset to low */
	gpio_write_io(PANEL_IO_RESET, 0);

	drv_l1_ldo28_sel(LDO_V18, DISABLE);
	#endif

	/* Let reset to low */
	gpio_write_io(PANEL_IO_RESET, 0);
}



INT32S mipi_display_KD055HD_IL9881C_720p_init(INT32U tft_index)
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
	
	DBG_PRINT("Enter mipi_display_KD055HD_IL9881C_720p_init\r\n");	
	
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


#if C_ASYN_DBLE_TFT
		set_KD055HD_IL9881C_display_for_double_TFT();
#endif


	return STATUS_OK;
#endif
}

// tft table
const DispCtrl_t TFT_Param_KD055HD_IL9881C =
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
		mipi_display_KD055HD_IL9881C_720p_init,
		Panel_KD055HD_IL9881C_uninit,

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
		TFT_ROT_RIGHT,

		/*	tft width & height	*/
		TFT_HACT,
		TFT_VACT
};
#endif

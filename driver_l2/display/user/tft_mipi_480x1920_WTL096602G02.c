
#include "drv_l2.h"

#if defined(LCD_DRV_EN_WTL096602G02) && LCD_DRV_EN_WTL096602G02

#define LANE_NUM	    2

#define IMG_WIDTH       480
#define IMG_HEIGHT      1920
#define TFT_HACT        IMG_WIDTH
#define TFT_VACT        IMG_HEIGHT

#define TFT_HS_WIDTH	2							//(HSA) HSW
#define TFT_H_START		(TFT_HS_WIDTH + 2)			//(HBP)
#define TFT_H_END		(TFT_H_START + IMG_WIDTH)
#define TFT_H_PERIOD	(TFT_H_END + 2)			    //(HFP)

#define TFT_VS_WIDTH	20						//(VSA)
#define TFT_V_START		(TFT_VS_WIDTH + 25)		//(VBP)
#define TFT_V_END		(TFT_V_START + IMG_HEIGHT)
#define TFT_V_PERIOD	(TFT_V_END + 35)		//(VFP)

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

static MIPI_Short_Command_Table_t WTL096602G02_initialization_setting[] =
{
////2lane
	{2, {0xB0,	0x5A}, MIPI_DCS_SHORT_WRITE_1P},
	{5, {CMD_DELAY}, MIPI_DCS_SHORT_WRITE_1P}, 	// Delay 50ms

	{2, {0xB0,	0x5A}, MIPI_DCS_SHORT_WRITE_1P},

	{2,{0xB1,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x89,0x01},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x91,0x17},MIPI_DCS_SHORT_WRITE_1P}, // 0x16 ²âÊÔÄ£Ê½
	{2,{0xB1,0x03},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x2C,0x28},MIPI_DCS_SHORT_WRITE_1P},

	{2,{0x00,0xF1},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x01,0x18},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x02,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x03,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x04,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x05,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x06,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x07,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x08,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x09,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x0A,0x01},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x0B,0x01},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x0C,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x0D,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x0E,0x24},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x0F,0x1C},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x10,0xC9},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x11,0x60},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x12,0x70},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x13,0x01},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x14,0xE5},MIPI_DCS_SHORT_WRITE_1P},// 0xE5 2lane / 0xE7 4lane
	{2,{0x15,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x16,0x3D},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x17,0x0E},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x18,0x01},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x19,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x1A,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x1B,0xFC},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x1C,0x0B},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x1D,0xA0},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x1E,0x03},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x1F,0x04},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x20,0x0C},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x21,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x22,0x04},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x23,0x81},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x24,0x1F},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x25,0x10},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x26,0x9B},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x2D,0x01},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x2E,0x84},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x2F,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x30,0x02},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x31,0x08},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x32,0x01},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x33,0x1C},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x34,0x40},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x35,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x36,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x37,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x38,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x39,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x3A,0x05},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x3B,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x3C,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x3D,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x3E,0x0F},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x3F,0x8C},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x40,0x2A},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x41,0xFC},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x42,0x01},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x43,0x40},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x44,0x05},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x45,0xE8},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x46,0x16},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x47,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x48,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x49,0x88},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x4A,0x08},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x4B,0x05},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x4C,0x03},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x4D,0xD0},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x4E,0x13},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x4F,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x50,0x0A},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x51,0x53},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x52,0x26},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x53,0x22},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x54,0x09},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x55,0x22},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x56,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x57,0x1C},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x58,0x03},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x59,0x3F},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x5A,0x28},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x5B,0x01},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x5C,0xCC},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x5D,0x21},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x5E,0x84},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x5F,0x84},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x60,0x8E},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x61,0x89},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x62,0xF0},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x63,0xB9},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x64,0xC6},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x65,0x96},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x66,0x0A},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x67,0x62},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x68,0x90},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x69,0x12},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x6A,0x42},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x6B,0x48},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x6C,0xE8},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x6D,0x98},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x6E,0x08},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x6F,0x9F},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x70,0x6B},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x71,0x6C},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x72,0xA9},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x73,0x20},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x74,0x06},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x75,0x29},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x76,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x77,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x78,0x0F},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x79,0xE0},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x7A,0x01},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x7B,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x7C,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x7D,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x7E,0x4F},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x7F,0xFE},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0xB1,0x02},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x00,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x01,0x05},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x02,0xF0},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x03,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x04,0x9E},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x05,0x57},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x06,0xC8},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x07,0x0F},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x08,0xC0},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x09,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x0A,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x0B,0x14},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x0C,0xE6},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x0D,0x0D},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x0F,0x00},MIPI_DCS_SHORT_WRITE_1P},

//============= #Gamma 1.6
	/*{2,{0x10,0x39},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x11,0xFC},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x12,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x13,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x14,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x15,0x55},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x16,0xF5},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x17,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x18,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x19,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x1A,0x78},MIPI_DCS_SHORT_WRITE_1P},*/

//============= #Gamma 2.0
	{2,{0x10,0xB9},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x11,0xCA},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x12,0xA5},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x13,0x15},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x14,0x7B},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x15,0x80},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x16,0x47},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x17,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x18,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x19,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x1A,0x74},MIPI_DCS_SHORT_WRITE_1P},

	{2,{0x1B,0x0E},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x1C,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x1D,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x1E,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x1F,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x20,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x21,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x22,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x23,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x24,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x25,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x26,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x27,0x1F},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x28,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x29,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x2A,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x2B,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x2C,0xFF},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x2D,0x07},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x33,0x06},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x35,0x7E},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x36,0x06},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x38,0x7E},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x3A,0x80},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x3B,0x01},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x3C,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x3D,0x2F},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x3E,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x3F,0xE0},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x40,0x05},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x41,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x42,0xBC},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x43,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x44,0x80},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x45,0x07},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x46,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x47,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x48,0x9B},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x49,0xD2},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x4A,0xC1},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x4B,0x83},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x4C,0x17},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x4D,0xC0},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x4E,0x0F},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x4F,0x61},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x50,0x78},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x51,0x7A},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x52,0x34},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x53,0x99},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x54,0xA2},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x55,0x02},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x56,0x14},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x57,0xB8},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x58,0xDC},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x59,0x34},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x5A,0x1E},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x5B,0x8F},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x5C,0xC7},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x5D,0xE3},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x5E,0xF1},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x5F,0x78},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x60,0x3C},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x61,0x36},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x62,0x1E},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x63,0x1B},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x64,0x8F},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x65,0xC7},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x66,0xE3},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x67,0x31},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x68,0x0C},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x69,0x89},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x6A,0x30},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x6B,0x8C},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x6C,0x8D},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x6D,0x8D},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x6E,0x8D},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x6F,0x8D},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x70,0xC7},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x71,0xE3},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x72,0x31},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x73,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x74,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x75,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x76,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x77,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x78,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x79,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x7A,0xC6},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x7B,0xC6},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x7C,0xC6},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x7D,0xC6},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x7E,0xC6},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x7F,0xE3},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x0B,0x04},MIPI_DCS_SHORT_WRITE_1P},

	{2,{0xB1,0x03}, MIPI_DCS_SHORT_WRITE_1P},
	//{2,{0x14,0xE1}, MIPI_DCS_SHORT_WRITE_1P},//2LANE
	{2,{0x2C,0x2C}, MIPI_DCS_SHORT_WRITE_1P},

	{2,{0xB1,0x00},MIPI_DCS_SHORT_WRITE_1P},
	{2,{0x89,0x03},MIPI_DCS_SHORT_WRITE_1P},

	{1, {0x11, 0x00}, MIPI_DCS_SHORT_WRITE_0P}, 	/* Exit sleep mode */
	{12, {CMD_DELAY}, MIPI_DCS_SHORT_WRITE_1P}, 	/* Delay 120ms */
	{1, {0x29, 0x00}, MIPI_DCS_SHORT_WRITE_0P}, 	/* Display on */
	{2, {CMD_DELAY}, MIPI_DCS_SHORT_WRITE_1P}, 		/* Delay 20ms */
};

static MIPI_Command_Table_t WTL096602G02_uninit_table[] =
{
    {2, 	{0x28, 0x00}, MIPI_GENERIC_LONG_WRITE},
    {1, 	{CMD_DELAY}, MIPI_GENERIC_LONG_WRITE},			/* Delay 10ms */
    {2, 	{0x10, 0x00}, MIPI_GENERIC_LONG_WRITE},
};

void Panel_WTL096602G02_Init(void)
{
	/* Do reset LCD pannel */
	gpio_write_io(PANEL_IO_RESET, 0);
	MIPI_DSI_10MS_DELAY(3);
	gpio_write_io(PANEL_IO_RESET, 1);

	/* Set lane swap */
	drv_l1_mipi_tx_lane_swap_set(0, 0x3210);

	mipi_lcd_software_reset(15);
	mipi_write_lcd_command((void*)WTL096602G02_initialization_setting, sizeof(WTL096602G02_initialization_setting)/sizeof(MIPI_Short_Command_Table_t), PACKET_SHORT_WRITE);
}

static INT32S tft_WTL096602G02_uninit(INT32U tft_index)
{
    mipi_write_lcd_command((void*)WTL096602G02_uninit_table, sizeof(WTL096602G02_uninit_table)/sizeof(MIPI_Command_Table_t), PACKET_LONG_WRITE);

	MIPI_DSI_10MS_DELAY(12);

	/* Stop MIPI DSI */
    drv_l1_mipi_tx_enable(DISABLE);

	MIPI_DSI_10MS_DELAY(15);

	/* Let reset to low */
	gpio_write_io(PANEL_IO_RESET, 0);
    return 0;
}

static INT32S tft_WTL096602G02_init(INT32U tft_index)
{
	INT32U	reg_data;
	INT32U	phy_hs_exit;
	INT32U	phy_lpx;
	INT32U	phy_clk_prepare, phy_clk_zero;
	INT32U	phy_clk_post, phy_clk_pre, phy_clk_trail;
	INT32U	phy_hs_prpr, phy_hs_zero, phy_hs_trail;
	INT32U	DSI_HFP,DSI_HSA,DSI_HBP;
	INT32U  DSICLK_period;
	INT32U  bit_period;
	INT32U  PtoD_Ratio;
	INT32U  DSICLK;
	INT32U  PCLK;

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
    reg_data |= 0x8300;         //  = 330MHz / 4
#endif
    R_SYSTEM_DSI_CTRL = reg_data;


    /////////////////////////////////////////////////////////////////////////////////////////
    //!!!  Need to be modified if you want to change the PLL freq. or panel fps         !!!//
    //!!!  480x1920: 9.66us/line =>  1920-line *1.1 blanking *9.66us = 20.4ms => 49fps  !!!//
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    R_SYSTEM_PERI_CLK_CTRL |= 0x6;                                      //!!! [1]=1:select PLLEXT, [2]=ENABLE(default)
    R_SYSTEM_EXTCLK_CTRL    = 0x6300;                                   //!!! [14]=enable,  [13:8]= 0x23 => PLLEXT  = 387MHz
    R_DSI_MISC_CTRL         = 0x1066;                                   //!!! DSICLK from PLLEXT: DSICLKX8 = (PLLEXT*2)/([11:9]+1) = 774M, DSICLK=774M/8= 96.75MHz
    R_SYSTEM_TMDS_CTRL      = ((R_SYSTEM_TMDS_CTRL & 0x00FF) | 0x8900); //!!! [15]=enable,  [13:8] => PCLK = PLLEXT/([13:8]+1) = 387/([13:8]+1)= 48.38MHz
    drv_l1_mipi_tx_set_LPCK(9);                                         //!!! LP mode freq = DSICLK/(9+1) = 96.75M/10 = 9.675M < LP SPEC: 10MHz

    DSICLK = 96;                                                        //!!! MHz;                     RULE: DSICLK > 3/2-lane * PCLK * (1.14 for blanking margin)
	PCLK   = 50;                                                        //!!! MHz;                                96.76M > 1.5 * 55.5M * 1.14 = 94.9M
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
	//R_IOF_ATT |= 0x0FFF;	//IOF_ATT for HDMI

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

	Panel_WTL096602G02_Init();

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

// tft table
const DispCtrl_t TFT_Param_WTL096602G02 =
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
	480,
	240,

	/* pip position */
	-480,   //x, must be multiple of 8
	0,      //y

	/* lcd init & uninit */
	tft_WTL096602G02_init,
	tft_WTL096602G02_uninit,

	/* lcd backlight control */
	LCD_BL_CTRL_PWM,
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
#endif

#include "drv_l2.h"
#if (defined LCD_DRV_EN_TP2915) && (LCD_DRV_EN_TP2915 == 1)

/**************************************************************************
 *  							M A C R O S								  *
 **************************************************************************/
#define TFT_WIDTH 1024
#define TFT_HEIGHT 600

/**************************************************************************
 *						  F U N C T I O N								  *
 **************************************************************************/
INT32S tft_tp2915_init(INT32U tft_index)
{
	INT32U TFT_HS_WIDTH;
	INT32U TFT_H_START;
	INT32U TFT_H_END;
	INT32U TFT_H_PERIOD;

	INT32U TFT_VS_WIDTH;
	INT32U TFT_V_START;
	INT32U TFT_V_END;
	INT32U TFT_V_PERIOD;

	/* XSJ0702IH40RI-127C: 1024x600 RGB24bit panel, direct connection to GPCV2178 TFT1.
	   Timing derived from proven in-project panel LY101BOE, HFP 30->100 用于治右缘竖黑条毛刺:
	   HPW=5,HBP=113,HDE=1024,HFP=100 => H_PERIOD=1243 ; VPW=5,VBP=116,VDE=600,VFP=10 => V_PERIOD=726 */
	TFT_HS_WIDTH = 5;
	TFT_H_START = 6 + 113;                    // = HPW+HBP = 119
	TFT_H_END = 5 + 113 + TFT_WIDTH;          // = HPW+HBP+HDE = 1142
	TFT_H_PERIOD = 6 + 113 + TFT_WIDTH + 100; // = 1243 (HFP 30->100，加大行尾留白)
	TFT_VS_WIDTH = 5;                         // = VPW
	TFT_V_START = 116;                        // = VPW+VBP = 116
	TFT_V_END = 116 + TFT_HEIGHT;             // = 716
	TFT_V_PERIOD = 116 + TFT_HEIGHT + 10;     // = 726

	/* DEBUG 关背光对照实验（2026-08-28）：注释背光强拉探针（配合 drv_l2_display.c 的 BL SKIPPED），
	   验证背光升压电路是否干扰 SDIO DAT1。实验后恢复 */
	//gpio_init_io(PANEL_IO_BL_ON, GPIO_OUTPUT);   /* DEBUG 强拉：确保背光脚为 GPIO 输出 */
	//gpio_write_io(PANEL_IO_BL_ON, 1);
	DBG_PRINT("\n==TP2915-INIT-ENTER==\r\n");

	DBG_PRINT("Run %s\r\n", __FUNCTION__);

	if(tft_index == TFT_2)
	{
		DBG_PRINT("[TP2915] bank write (TFT_2)\r\n");
		R_TFT2_CTRL &= ~(1 << 31); // switching to bank0
		R_TFT2_HS_WIDTH = TFT_HS_WIDTH;
		R_TFT2_H_START = TFT_H_START;
		R_TFT2_H_END = TFT_H_END;
		R_TFT2_H_PERIOD = TFT_H_PERIOD;
		R_TFT2_VS_WIDTH = TFT_VS_WIDTH;
		R_TFT2_V_START = TFT_V_START;
		R_TFT2_V_END = TFT_V_END;
		R_TFT2_V_PERIOD = TFT_V_PERIOD;

		R_TFT2_CTRL |= (1 << 31);	 // switching to bank1
		R_TFT2_LINE_RGB_ORDER = 0x0; // RGB order
		R_TFT2_CTRL &= ~(1 << 31);	 // switching to bank0
	}
	else
	{
		DBG_PRINT("[TP2915] bank write (TFT_1)\r\n");
		R_TFT_HS_WIDTH = TFT_HS_WIDTH;
		R_TFT_H_START = TFT_H_START;
		R_TFT_H_END = TFT_H_END;
		R_TFT_H_PERIOD = TFT_H_PERIOD;
		R_TFT_VS_WIDTH = TFT_VS_WIDTH;
		R_TFT_V_START = TFT_V_START;
		R_TFT_V_END = TFT_V_END;
		R_TFT_V_PERIOD = TFT_V_PERIOD;
		R_TFT_LINE_RGB_ORDER = 0x00;
	}
	drv_l1_disp_dev_rb_switch_set(tft_index, TRUE);
	DBG_PRINT("[TP2915] rb_switch ok\r\n");
	drv_l1_disp_dev_signal_inv_set(tft_index, TFT_VSYNC_INV|TFT_HSYNC_INV, 0);
	DBG_PRINT("[TP2915] signal_inv ok\r\n");
	drv_l1_disp_dev_mode_set(tft_index, TFT_MODE_PARALLEL);
	DBG_PRINT("[TP2915] mode ok\r\n");
	/* 结论：888 数据模式会在 en_set 死锁（2026-08-27 实测复现），SoC 显示管线仅支持 RGB565，
	   故本屏固定用 565 驱动。右缘毛刺另查（走线/时序/旋转）。 */
	drv_l1_disp_dev_data_mode_set(tft_index, TFT_DATA_MODE_565);
	DBG_PRINT("[TP2915] data_mode ok\r\n");

	/* EXPERIMENT: DIVIDE_6 selects 37 MHz DCLK, approximately 41.0 Hz with
	   the current 1243 x 726 timing.  Validate panel image quality and
	   Wi-Fi/SDIO stability before treating this as a production setting. */
	drv_l1_disp_dev_clk_set(tft_index, TFT_CLK_DIVIDE_10);
	DBG_PRINT("[TP2915] clk ok (DIVIDE_6 37MHz, experimental)\r\n");
	drv_l1_disp_dev_hdmi_en_set(tft_index, 0);
	DBG_PRINT("[TP2915] pre-en ctrl=0x%08x\r\n", R_TFT_CTRL);
	DBG_PRINT("[TP2915] output enable deferred until after Wi-Fi/BT startup\r\n");

	DBG_PRINT("%s OK\r\n", __FUNCTION__);
	return STATUS_OK;
}

static INT32S tft_tp2915_uninit(INT32U tft_index)
{
	DBG_PRINT("Run %s function\r\n", __FUNCTION__);

	/* Let rest be low */
	gpio_write_io(PANEL_IO_RESET, 0);

	return 0;
}

// tft table
const DispCtrl_t TFT_Param_tp2915 =
{
	/* lcd size */
	TFT_WIDTH,
	TFT_HEIGHT,

	1, // sync_pin_enable
	1, // de_pin_enable
	0, // te_pin_enable
	1, // tft2_io_enable

	0, // h_off_lines_video
	0, // h_off_lines_cap

	0, // en_crop_video
	0, // en_crop_cap
	0, // pbp_crop_video
	0, // pbp_crop_cap
	0, // back_ratio_video
	0, // back_ratio_cap

	/* pip size */
	128,
	96,

	/* pip position */
	-128, // x, must be multiple of 8
	0,	  // y

	/* lcd init & uninit */
	tft_tp2915_init,
	tft_tp2915_uninit,

	/* lcd backlight control */
	LCD_BL_CTRL_CONSTANT,
	20 * 1000, // in HZ for LCD_BL_CTRL_PWM
	55,			// in duty cycle for LCD_BL_CTRL_PWM

	/* DEBUG: 强制走 RGB565 帧缓冲，绕过 GP420/YUV 管线，直接显示 FBI 四象限纯色 */
	DISP_FMT_RGB565,

	/* tft IFCE	 */
	LCD_IFCE_RGB,

	/* tft rotate   */
	TFT_ROT_NONE,

	/*  tft width & height  */
	TFT_WIDTH,
	TFT_HEIGHT
};

#endif // #if (defined LCD_DRV_EN_TP2915) && (LCD_DRV_EN_TP2915 == 1)

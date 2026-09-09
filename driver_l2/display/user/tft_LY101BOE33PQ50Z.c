
#include "drv_l2.h"

#if defined(LCD_DRV_EN_LY101BOE) && LCD_DRV_EN_LY101BOE

//#define TFT_TAR_CLK      18000000

static INT32S tft_LY101BOE_init(INT32U tft_index)
{
    DBG_PRINT("%s ...\r\n", __func__);

    if(tft_index == TFT_2)
    {
        R_TFT2_CTRL &= ~(1<<31);//switching to bank0
        R_TFT2_HS_WIDTH			= 5;                //  6   =HPW
        R_TFT2_H_START			= 6+113;            //      =HPW+HBP
        R_TFT2_H_END			= 5+113+1024;       //      =HPW+HBP+HDE
        R_TFT2_H_PERIOD			= 6+113+1024+30;    //      =HPW+HBP+HDE+HFP

        R_TFT2_VS_WIDTH			= 5;				//  6   =VPW				(LINE)
        R_TFT2_V_START			= 116;				//		=VPW+VBP			(LINE)
        R_TFT2_V_END			= 116+600;			//		=VPW+VBP+VDE		(LINE)
        R_TFT2_V_PERIOD			= 116+600+10;		//      =VPW+VBP+VDE+VFP	(LINE)

        R_TFT2_CTRL |= (1<<31);     //switching to bank1
        R_TFT2_LINE_RGB_ORDER = 0x00;   //RGB order
        R_TFT2_CTRL &= ~(1<<31);    //switching to bank0
    }
    else
    {
        R_TFT_HS_WIDTH			= 5;                //  2   =HPW
        R_TFT_H_START			= 6+113;            //      =HPW+HBP
        R_TFT_H_END			    = 5+113+1024;       //      =HPW+HBP+HDE
        R_TFT_H_PERIOD			= 6+113+1024+30;    //      =HPW+HBP+HDE+HFP

        R_TFT_VS_WIDTH			= 5;				//  6   =VPW				(LINE)
        R_TFT_V_START			= 116;				//		=VPW+VBP			(LINE)
        R_TFT_V_END			    = 116+600;			//		=VPW+VBP+VDE		(LINE)
        R_TFT_V_PERIOD			= 116+600+10;		//      =VPW+VBP+VDE+VFP	(LINE)

        R_TFT_LINE_RGB_ORDER = 0x00;		//RGB order
    }

    drv_l1_disp_dev_rb_switch_set(tft_index, 1);
	drv_l1_disp_dev_signal_inv_set(tft_index, TFT_VSYNC_INV|TFT_HSYNC_INV, 0/*(TFT_ENABLE & TFT_VSYNC_INV)|(TFT_ENABLE & TFT_HSYNC_INV)*/);
	drv_l1_disp_dev_mode_set(tft_index, TFT_MODE_PARALLEL);
    drv_l1_disp_dev_data_mode_set(tft_index, TFT_DATA_MODE_565);
    //drv_l1_disp_dev_target_clk_set(tft_index, TFT_TAR_CLK);
    drv_l1_disp_dev_clk_set(tft_index, TFT_CLK_DIVIDE_6);
    drv_l1_disp_dev_hdmi_en_set(tft_index, 0);
    //drv_l1_disp_dev_vsync_unit_set(tft_index, 1);
    drv_l1_disp_dev_en_set(tft_index, TRUE);
	return STATUS_OK;

}

// tft table
const DispCtrl_t TFT_Param_LY101BOE =
{
	/* lcd size */
	1024,
	600,

    1,  //sync_pin_enable
	1,  //de_pin_enable
	0,  //te_pin_enable
	1,  //tft2_io_enable

	0,  //h_off_lines_video
	0,  //h_off_lines_cap

	0,  //en_crop_video
	0,  //en_crop_cap
	0,  //pbp_crop_video
	0,  //pbp_crop_cap
	0,  //back_ratio_video
	0,  //back_ratio_cap

	/* pip size */
	320,
	180,

	/* pip position */
	-320,   //x, must be multiple of 8
	0,      //y

	/* lcd init & uninit */
	tft_LY101BOE_init,
	NULL,

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
    TFT_ROT_NONE,

    /*  tft width & height  */
    1024,
    600
};
#endif

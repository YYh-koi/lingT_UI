#include "drv_l2.h"

#if defined(LCD_DRV_EN_TXDT240C_OTA5182A) && LCD_DRV_EN_TXDT240C_OTA5182A

#define TFT_TAR_CLK      18000000

static void cmd_delay(INT32U i)
{
	drv_msec_wait(i);
}

static void serial_cmd_1(INT32U cmd)
{
	INT32S	i;

	gpio_write_io(PANEL_IO_CSN, 1); //CS=1
	gpio_write_io(PANEL_IO_SCL, 0); //SCL=0
	gpio_write_io(PANEL_IO_SDA, 0); //SDA

	// set csn low
	gpio_write_io(PANEL_IO_CSN, 0); //CS=0
	cmd_delay(1);
	for(i = 0; i < 16; i++)
	{
		// shift data
		gpio_write_io(PANEL_IO_SDA, ((cmd & 0x8000) >> 15));	/* SDA */
		cmd = (cmd << 1);
		cmd_delay(1);

		// toggle clock
		gpio_write_io(PANEL_IO_SCL, 1);		//SCL=0
		cmd_delay(1);
		gpio_write_io(PANEL_IO_SCL, 0);		//SCL=0
		cmd_delay(1);
	}

	// set csn high
	gpio_write_io(PANEL_IO_CSN, 1);			//CS=1
	cmd_delay(1);
}

static INT32S tft_TXDT240C_OTA5182A_init(INT32U tft_index)
{

	DBG_PRINT("%s\r\n", __func__);

	serial_cmd_1(0x000D);
	cmd_delay(5);
	serial_cmd_1(0x0005);
	cmd_delay(5);
	serial_cmd_1(0x0005);
	cmd_delay(5);
	serial_cmd_1(0x000D);
	cmd_delay(5);
	serial_cmd_1(0x6001);
	cmd_delay(5);
	serial_cmd_1(0x4003);
	serial_cmd_1(0xA008);
	serial_cmd_1(0x5068);

    if(tft_index == TFT_2)
    {
        R_TFT2_CTRL &= ~(1<<31);                        //switching to bank0
        R_TFT2_HS_WIDTH			= 1;				    //	2	=HPW
        R_TFT2_H_START			=  1 + 252;			    //		=HPW+HBP
        R_TFT2_H_END			= 1 + 252 + 1280;		//		=HPW+HBP+HDE
        R_TFT2_H_PERIOD			= 1 + 252 + 1280 + 28;	//		=HPW+HBP+HDE+HFP
        R_TFT2_VS_WIDTH			= 0;				    //	1	=VPW				(LINE)
        R_TFT2_V_START			= 1 + 21;				//		=VPW+VBP			(LINE)
        R_TFT2_V_END			= 1 + 21 + 240;			//		=VPW+VBP+VDE		(LINE)
        R_TFT2_V_PERIOD			= 1 + 21 + 240 + 9;		//		=VPW+VBP+VDE+VFP	(LINE)

        R_TFT2_CTRL |= (1<<31);                         //switching to bank1
        R_TFT2_LINE_RGB_ORDER = 0x00;		            //RGB order
        R_TFT2_CTRL &= ~(1<<31);                        //switching to bank0
    }
    else
    {
        R_TFT_HS_WIDTH =        1;					    //	1		=HPW
        R_TFT_H_START =         1 + 252;				//	240		=HPW+HBP
        R_TFT_H_END =           1 + 252 + 1280;			//	1520	=HPW+HBP+HDE
        R_TFT_H_PERIOD =        1 + 252 + 1280 + 28;	//	1560	=HPW+HBP+HDE+HFP
        R_TFT_VS_WIDTH =        1;					    //	1		=VPW				(DCLK)
        R_TFT_V_START =         1 + 21;					//	21		=VPW+VBP			(LINE)
        R_TFT_V_END =       1 + 21 + 240;				//	261		=VPW+VBP+VDE		(LINE)
        R_TFT_V_PERIOD =        1 + 21 + 240 + 9;		//	262		=VPW+VBP+VDE+VFP	(LINE)
        R_TFT_LINE_RGB_ORDER = 0x00;
    }
	drv_l1_disp_dev_signal_inv_set(tft_index, TFT_VSYNC_INV|TFT_HSYNC_INV, (TFT_ENABLE & TFT_VSYNC_INV)|(TFT_ENABLE & TFT_HSYNC_INV));
	drv_l1_disp_dev_mode_set(tft_index, TFT_MODE_UPS052);
    drv_l1_disp_dev_data_mode_set(tft_index, TFT_DATA_MODE_8);
    drv_l1_disp_dev_target_clk_set(tft_index, TFT_TAR_CLK);
    drv_l1_disp_dev_hdmi_en_set(tft_index, 0);
    //drv_l1_disp_dev_vsync_unit_set(tft_index, 1);
    drv_l1_disp_dev_en_set(tft_index, TRUE);
	return STATUS_OK;
}

// tft table
const DispCtrl_t TFT_Param_TXDT240C_OTA5182A =
{
	/* lcd size */
	320,
	240,

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
	128,
	96,

    /* pip position */
	-128,   //x, must be multiple of 8
	0,      //y

	/* lcd init & uninit */
	tft_TXDT240C_OTA5182A_init,
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

	/* lcd rotate */
	TFT_ROT_NONE,
	/* TFT  width*/
	320,
	/* TFT  heigh*/
	240,
};

#endif // #if defined(LCD_DRV_EN_TXDT240C_OTA5182A) && LCD_DRV_EN_TXDT240C_OTA5182A

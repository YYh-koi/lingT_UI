#include "drv_l2.h"

#if defined(LCD_DRV_EN_AUO_A027DTN019) && (LCD_DRV_EN_AUO_A027DTN019 == 1)

static INT8U CSN_n;
static INT8U SCL_n;
static INT8U SDA_n;

static void cmd_delay(INT32U i)
{
	INT32U j, cnt;

	cnt = i*8;
	for (j = 0; j < cnt; j++);
}

static void serial_cmd_1(INT32U cmd)
{
	INT32S i;

	gpio_write_io(CSN_n, 1);//CS=1
	gpio_write_io(SCL_n, 0);//SCL=0
	gpio_write_io(SDA_n, 0);//SDA

	// set csn low
	gpio_write_io(CSN_n, 0);//CS=0
	cmd_delay(1);
	for (i=0;i<16;i++) {
		// shift data
		gpio_write_io(SDA_n, ((cmd&0x8000)>>15)); /* SDA */
		cmd = (cmd<<1);
		cmd_delay(1);
		// toggle clock
		gpio_write_io(SCL_n, 1);//SCL=0
		cmd_delay(1);
		gpio_write_io(SCL_n, 0);//SCL=0
		cmd_delay(1);
	}

	// set csn high
	gpio_write_io(CSN_n,1);//CS=1
	cmd_delay(1);

}

const INT8U AUO_REGTable[]={


	0x05,0x44,
	0x05,0x04,
	0x05,0x44,

	0x01,0xb4,//ups 052 mode
    0x00,0x23,
	0x04,0x1b,

	0x05,0x45,
//	0x05,0x5d,
//	0x85,0x80,
	0XFF,0XFF
};
static INT32S tft_auo_a027dtn019_init(INT32U tft_index)
{
	INT8U i = 0;

	CSN_n = PANEL_IO_CSN;
	SDA_n = PANEL_IO_SDA;
	SCL_n = PANEL_IO_SCL;


	DBG_PRINT("%s-%d\r\n", __func__,tft_index);

	gpio_init_io (CSN_n, GPIO_OUTPUT);
	gpio_init_io (SCL_n, GPIO_OUTPUT);
	gpio_init_io (SDA_n, GPIO_OUTPUT);

	serial_cmd_1((0x02 << (2+8)) | 0x00);	//enable
#if 0
    serial_cmd_1((0x05 << (2+8)) | 0x10);   //rotate
#endif

    while(AUO_REGTable[i] !=0xff)
    {
	    serial_cmd_1( (AUO_REGTable[i]<<8) |(AUO_REGTable[i+1]) );
    	i+=2;
    }

    if(tft_index == TFT_2)
    {
        R_TFT2_CTRL &= ~(1<<31);//switching to bank0
        R_TFT2_HS_WIDTH			= 1;				//	1		=HPW
        R_TFT2_H_START			= 1+240;			//	240		=HPW+HBP
        R_TFT2_H_END			= 1+240+1280;		//	1520	=HPW+HBP+HDE
        R_TFT2_H_PERIOD			= 1+240+1280+40;	//	1560	=HPW+HBP+HDE+HFP
        R_TFT2_VS_WIDTH			= 0;				//	1		=VPW				(LINE)
        R_TFT2_V_START			= 21;				//	21		=VPW+VBP			(LINE)
        R_TFT2_V_END			= 21+240;			//	261		=VPW+VBP+VDE		(LINE)
        R_TFT2_V_PERIOD			= 21+240+1;		//	262		=VPW+VBP+VDE+VFP	(LINE)

        R_TFT2_CTRL |= (1<<31);//switching to bank1
        R_TFT2_LINE_RGB_ORDER = 0x0;		//RGB order
        R_TFT2_CTRL &= ~(1<<31);//switching to bank0
    }
    else
    {
		R_TFT_HS_WIDTH			= 1;				//	1		=HPW
		R_TFT_H_START			= 1+240;			//	240		=HPW+HBP
		R_TFT_H_END				= 1+240+1280;		//	1520	=HPW+HBP+HDE
		R_TFT_H_PERIOD			= 1560;            	//	1560	=HPW+HBP+HDE+HFP
		R_TFT_VS_WIDTH			= 1;				//	1		=VPW				(DCLK)
		R_TFT_V_START			= 21;				//	21		=VPW+VBP			(LINE)
		R_TFT_V_END				= 21+240;			//	261		=VPW+VBP+VDE		(LINE)
		R_TFT_V_PERIOD			= 1+21+240;			//	262		=VPW+VBP+VDE+VFP	(LINE)
		R_TFT_LINE_RGB_ORDER    = 0x00;
	}

	drv_l1_disp_dev_signal_inv_set(tft_index, TFT_VSYNC_INV|TFT_HSYNC_INV, (TFT_ENABLE & TFT_VSYNC_INV)|(TFT_ENABLE & TFT_HSYNC_INV));
	drv_l1_disp_dev_mode_set(tft_index, TFT_MODE_UPS052);
	drv_l1_disp_dev_data_mode_set(tft_index, TFT_DATA_MODE_8);
	//drv_l1_tft_clk_set(TFT_CLK_DIVIDE_8);	/* FS=66 */
	drv_l1_disp_dev_target_clk_set(tft_index, 24000000);
	drv_l1_disp_dev_hdmi_en_set(tft_index, 0);
    drv_l1_disp_dev_vsync_unit_set(tft_index, 1);
    drv_l1_disp_dev_en_set(tft_index, TRUE);
	//drv_l1_tft_en_set(TRUE);

	return STATUS_OK;
}

// tft table
const DispCtrl_t TFT_Param_auo_a027dtn019 =
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
	tft_auo_a027dtn019_init,
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
    320,
    240
};

#endif // #if defined(LCD_DRV_EN_AUO_A027DTN019) && LCD_DRV_EN_AUO_A027DTN019

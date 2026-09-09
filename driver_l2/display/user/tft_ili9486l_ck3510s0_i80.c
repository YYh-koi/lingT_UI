
#include "drv_l2.h"

#if defined(LCD_DRV_EN_ILI9486L_CK3510S0) && LCD_DRV_EN_ILI9486L_CK3510S0

#define USE_GPIO	0
#define LCD_RD		IO_B0
#define LCD_RS		IO_B1
#define LCD_CS		IO_B2
#define LCD_WR		IO_B3

#define WriteComm(x)    Wirte_Com_LC8bit(tft_index, x)
#define WriteData(x)    Wirte_Data_LC8bit(tft_index, x)
#define Delay           drv_msec_wait


void Wirte_Com_LC8bit(INT32U tft_index, INT32U index)
{
#if USE_GPIO
	INT16U  temp;

	gpio_write_io(LCD_CS, DATA_HIGH);
	gpio_write_io(LCD_WR, DATA_HIGH);
	gpio_write_io(LCD_RS, DATA_LOW);
	gpio_write_io(LCD_RD, DATA_HIGH);

	gpio_write_io(LCD_CS, DATA_LOW);
	gpio_write_io(LCD_WR, DATA_LOW);

	temp = R_IOA_BUFFER;
	temp &= 0xff00;
	temp |= index & 0x00ff;
	R_IOA_BUFFER = temp;

	gpio_write_io(LCD_WR, DATA_HIGH);
	gpio_write_io(LCD_CS, DATA_HIGH);
#else
    drv_l1_disp_dev_i80_wr_cmd(tft_index, index);
#endif
}



void Wirte_Data_LC8bit(INT32U tft_index, INT32U data)
{
#if USE_GPIO
	INT16U  temp;

	gpio_write_io(LCD_CS, DATA_HIGH);
	gpio_write_io(LCD_WR, DATA_HIGH);
	gpio_write_io(LCD_RS, DATA_HIGH);
	gpio_write_io(LCD_RD, DATA_HIGH);

	gpio_write_io(LCD_CS, DATA_LOW);
	gpio_write_io(LCD_WR, DATA_LOW);

	temp = R_IOA_BUFFER;
	temp &= 0xff00;
	temp |= data & 0x00ff;
	R_IOA_BUFFER = temp;

	gpio_write_io(LCD_WR, DATA_HIGH);
	gpio_write_io(LCD_CS, DATA_HIGH);
#else
    drv_l1_disp_dev_i80_wr_data(tft_index, data);
#endif
}


void TFT_Clear(INT32U tft_index, unsigned int Color)
{
    INT16U  width, height;

	for(height = 0; height < 480; height++)
	{
		for(width = 0; width < 320; width++)
		{
			Wirte_Data_LC8bit(tft_index, Color >> 8);
			Wirte_Data_LC8bit(tft_index, Color);
		}
	}
}


static INT32S tft_ili9486l_ck3510s0_init(INT32U tft_index)
{
	DBG_PRINT("%s\r\n", __func__);

#if USE_GPIO
	R_IOA_ATTRIB |= 0x00FF;
	R_IOA_DIR    |= 0x00FF;
	R_IOA_BUFFER &= 0xFF00;

	gpio_init_io(LCD_CS, GPIO_OUTPUT);
	gpio_init_io(LCD_RS, GPIO_OUTPUT);
	gpio_init_io(LCD_WR, GPIO_OUTPUT);
	gpio_init_io(LCD_RD, GPIO_OUTPUT);
#endif

	gpio_init_io(PANEL_IO_RESET, GPIO_OUTPUT);
	gpio_write_io (PANEL_IO_RESET, 1);
	drv_msec_wait(100);
	gpio_write_io (PANEL_IO_RESET, 0);
	drv_msec_wait(130);
	gpio_write_io (PANEL_IO_RESET, 1);
	drv_msec_wait(130);

    drv_l1_disp_dev_data_mode_set(tft_index, TFT_DATA_MODE_8);
    drv_l1_disp_dev_mem_unit_set(tft_index, 8);
    drv_l1_disp_dev_target_clk_set(tft_index, 48000000);    //real clock = 222MHz / 4 = 55.5MHz

	if(tft_index == TFT_1)
	{
        //b[7]: TFT_MEM_CSB_MODE, When this bit is set as 1, the low pulse width of CS signal will be the same as WE/OE signals
        //      which means the settings in P_TFT_H_START and P_TFT_HS_WIDTH will be invalid.
        R_TFT_LINE_RGB_ORDER = 0;//0x0080;    //memory mode

        //b[11:0]: TFT_V_PERIOD = Vertical line count - 1
        R_TFT_V_PERIOD = 480 - 1;

        //b[5:0]: TFT_VS_WIDTH, Unit: TFT Clock Cycle???
        R_TFT_VS_WIDTH = 1;

        //b[9]: TFT_MSA_EN     0: Disabled (fixed 3 TFT clock)  1: Enabled (depend on TFT_MS_CYC)
        //b[7:0]: TFT_MS_CYC,  TFT Memory Mode Setup Cycle    Unit: TFT Clock Cycle???
        R_TFT_V_START = 0;

        //b[10]: TFT_MEM_TE_WAIT_EN
        //b[7:0]: TFT_MEM_TE_WAIT_CYC, TFT memory mode tearing effect wait time, Unit: TFT Clock Cycle
        R_TFT_V_END = 0;

        //b[12:0]: TFT_H_PERIOD = Horizontal pixel count - 1
        R_TFT_H_PERIOD = 320 - 1;

        //b[7:0]: TFT_HS_WIDTH, R/W pulse low period = (TFT_HS_WIDTH[7:0] + 1), Unit: TFT Clock Cycle
        R_TFT_HS_WIDTH = 1;

        //b[7:0]: TFT_H_START, R/W pulse high period = (TFT_H_START[7:0] + 1), Unit: TFT Clock Cycle
        R_TFT_H_START = 1;
	}
	else //TFT_2
	{
        drv_l1_disp_dev_hdmi_en_set(tft_index, 0);

        drv_l1_disp_bank_sel_en_set(tft_index, 1);
        //b[7]: TFT2_MEM_CSB_MODE, When this bit is set as 1, the low pulse width of CS signal will be the same as WE/OE signals.
        //      That means the settings in P_TFT2_H_START and P_TFT2_HS_WIDTH will be invalid
        R_TFT2_LINE_RGB_ORDER = 0;//0x0080;    //memory mode
        drv_l1_disp_bank_sel_en_set(tft_index, 0);

        //b[10:0]: TFT2_V_PERIOD = Vertical line count - 1
        R_TFT2_V_PERIOD = 480 - 1;

        //b[5:0]: TFT2_VS_WIDTH, Unit: TFT2 Clock Cycle???
        R_TFT2_VS_WIDTH = 1;

        //b[9]: TFT2_MSA_EN     0: Disabled (fixed 3 TFT2 clock)  1: Enabled (depend on TFT2_MS_CYC)
        //b[7:0]: TFT2_MS_CYC,  TFT2 Memory Mode Setup Cycle    Unit: TFT2 Clock Cycle???
        R_TFT2_V_START = 0;

        //b[10]: TFT2_MEM_TE_WAIT_EN
        //b[7:0]: TFT2_MEM_TE_WAIT_CYC, TFT2 memory mode tearing effect wait time, Unit: TFT2 Clock Cycle
        R_TFT2_V_END = 0;

        //b[12:0]: TFT2_H_PERIOD = Horizontal pixel count - 1
        R_TFT2_H_PERIOD = 320 - 1;

        //b[7:0]: TFT2_HS_WIDTH, R/W pulse low period = (TFT2_HS_WIDTH[7:0] + 1), Unit: TFT2 Clock Cycle
        R_TFT2_HS_WIDTH = 1;

        //b[7:0]: TFT2_H_START, R/W pulse high period = (TFT2_H_START[7:0] + 1), Unit: TFT2 Clock Cycle
        R_TFT2_H_START = 1;
   }

    //WriteComm(0x01);//software reset
    //drv_msec_wait(430);

    WriteComm(0xC0);
    WriteData(0x18);//VREG1OUT POSITIVE
    WriteData(0x1a);//VREG2OUT NEGATIVE

    WriteComm(0xC1);
    WriteData(0x45);//VGH,VGL    VGH>=14V.
    WriteData(0x00);

    WriteComm(0xC2);
    WriteData(0x33);

    WriteComm(0xC5);
    WriteData(0x00);
    WriteData(0x36);//VCM_REG[7:0]. <=0x80.

    WriteComm(0xB1);
    WriteData(0xA0);//0xB0 =70HZ, <=0xB0.
    WriteData(0x11);

    WriteComm(0xB4);
    WriteData(0x00); //2 DOT FRAME MODE,F<=70HZ.

    WriteComm(0xB6);
    WriteData(0x00);
    WriteData(0x42);//0 GS SS SM ISC[3:0];
    WriteData(0x3B);
    /////////////////
    WriteComm(0x21);
    WriteComm(0x13);
    ////////////////
    WriteComm(0xB7);
    WriteData(0x07);

    WriteComm(0xE0);
    WriteData(0x1F);
    WriteData(0x25);
    WriteData(0x22);
    WriteData(0x0B);
    WriteData(0x06);
    WriteData(0x0A);
    WriteData(0x4E);
    WriteData(0xC6);
    WriteData(0x39);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x00);

    WriteComm(0xE1);
    WriteData(0x1F);
    WriteData(0x3F);
    WriteData(0x3F);
    WriteData(0x0F);
    WriteData(0x1F);
    WriteData(0x0F);
    WriteData(0x46);
    WriteData(0x49);
    WriteData(0x31);
    WriteData(0x05);
    WriteData(0x09);
    WriteData(0x03);
    WriteData(0x1C);
    WriteData(0x1A);
    WriteData(0x00);

    WriteComm(0xF1);
    WriteData(0x36);
    WriteData(0x04);
    WriteData(0x00);
    WriteData(0x3C);
    WriteData(0x0F);
    WriteData(0x0F);
    WriteData(0xA4);
    WriteData(0x02);

    WriteComm(0xF2);
    WriteData(0x18);
    WriteData(0xA3);
    WriteData(0x12);
    WriteData(0x02);
    WriteData(0x32);
    WriteData(0x12);
    WriteData(0xFF);
    WriteData(0x32);
    WriteData(0x00);

    WriteComm(0xF4);
    WriteData(0x40);
    WriteData(0x00);
    WriteData(0x08);
    WriteData(0x91);
    WriteData(0x04);

    WriteComm(0xF8);
    WriteData(0x21);
    WriteData(0x04);
    WriteComm(0xF9);
    WriteData(0x00);
    WriteData(0x08);

    WriteComm(0x36);
    WriteData(0x08);    //480x320 , BIT[3] => (0 = RGB color filter panel, 1 = BGR color filter panel)

    WriteComm(0x3A);
    WriteData(0x55);

    WriteComm(0x2A);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x01);
    WriteData(0x3F);

    WriteComm(0x2B);            // Row address set "V"
    WriteData(0x00);            // High byte of Y start
    WriteData(0x00);            // Low byte of Y start
    WriteData(0x01);            // High byte of Y end
    WriteData(0xDF);            // Low byte of Y end

    WriteComm(0x35);    //TE ON
    //WriteData(0x08);

    Delay(10);
    WriteComm(0x11);
    Delay(120);
    WriteComm(0x29);    //display ON
    Delay(10);
    WriteComm(0x2c);    //memory write

    /*
    while(1)
    {
        TFT_Clear(tft_index, 0x001f);
        osDelay(1000);
        TFT_Clear(tft_index, 0x03e0);
        osDelay(1000);
        TFT_Clear(tft_index, 0xf800);
        osDelay(1000);
    } */

	drv_l1_disp_dev_mode_set(tft_index, TFT_MODE_MEM_ONCE);

    //panel frame rate = 62Hz
    //According to the above setting:
    //Actual blanking time = (1/TFT2_CLK) * (R_TFT2_H_PERIOD + 1)*( R_TFT2_VS_WIDTH + 1) = 320 * 2 * 1/48000000
    //Actual setup time = (1/TFT2_CLK) * (R_TFT2_H_PERIOD + 1)*( R_TFT2_V_START bit[7:0] + 1) = 320 * 3 * 1/48000000
    //Frame data write time = (1/TFT2_CLK)* ((R_TFT2_H_START + 1)+(R_TFT2_HS_WIDTH + 1)) *( R_TFT2_V_PERIOD + 1)*( R_TFT2_H_PERIOD + 1) * 2
    //                      = (2 + 2) * 320 * 480 * 2 * 1/48000000
    // frame rate = 55500000 / (640 + 960 + 1228800) = 45Hz < panel frame rate(62Hz), use slow mode

    if(TFT_Param_ILI9486L_CK3510S0.te_pin_enable)
    {
        if(tft_index == TFT_1)
        {
            R_TFT_TE_HS_WIDTH = 8325;
            R_TFT_TE_CTRL = (0 << 24) + (1 << 16) + (0 << 8) + 3;
        }
        else //TFT_2
        {
            drv_l1_disp_bank_sel_en_set(tft_index, 1);

            //b[17:0]: TFT2_TE_HSYN_CNT
            //TE high pulse = 800us, so set delay = 1.2ms, Delay count = 0.0012/(8*(1/system clock)) = 0.0012/(8*(1/55.5M)) = 8325
            R_TFT2_TE_HS_WIDTH = 8325;

            //b[31:24]: TFT2_TE_LINE_SEL, Tearing Effect Horizontal Line Numbers
            //b[16]: TFT2_TE_MODE, 1: Tearing effect input signal only contains V-blanking information.
            //                     0: Tearing effect input signal consists of V-blanking and H-blanking information.
            //b[11:10]: GP420_UR_THRES
            //b[9]: GP420_EXT_RAM
            //b[8]: TFT2_TE_888_EN, 0: Disabled, RGB565 (16 bit RGB data will send twice on 8 bit data pin)
            //                      1: Enabled, RGB888 (24 bit RGB data will send three times on 8 bit data pin)
            //b[6:5]: TFT2FB_BUF_THRES
            //b[4]: TFT2FB_BUF_MODE_EN
            //b[2]: TFT2_GAMMA_FIX, 0: i80 mode can¡¯t use gamma table.  1: i80 mode can use gamma table.
            //b[1]: TFT2_TE_FIX, 0: i80 write once mode can¡¯t use TE.   1: i80 write once mode can use TE.
            //b[0]: TFT2_TE_EN, 0: Disabled 1: Enabled
            R_TFT2_TE_CTRL = (0 << 24) + (1 << 16) + (0 << 8) + 3;

            drv_l1_disp_bank_sel_en_set(tft_index, 0);
        }
    }

	return STATUS_OK;
}

// tft table
const DispCtrl_t TFT_Param_ILI9486L_CK3510S0 =
{
	/* lcd size */
	320,
	480,

    1,  //sync_pin_enable
	1,  //de_pin_enable
	1,  //te_pin_enable
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
	160,
	128,

	/* pip position */
	-160,   //x, must be multiple of 8
	0,      //y

	/* lcd init & uninit */
	tft_ili9486l_ck3510s0_init,
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
	LCD_IFCE_I80,

	/* tft rotate   */
    TFT_ROT_RIGHT,

    /*  tft width & height  */
    320,
    480
};

#endif // #if defined(LCD_DRV_EN_ILI9486_I80) && LCD_DRV_EN_ILI9486_I80

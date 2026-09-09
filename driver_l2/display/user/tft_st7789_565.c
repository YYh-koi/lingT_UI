#include "drv_l2.h"

#if defined(LCD_DRV_EN_st7789_565) && LCD_DRV_EN_st7789_565

#define TFT_TAR_CLK      16000000

static void cmd_delay(INT32U i)
{
	drv_msec_wait(i);
}

static void serial_cmd_1(INT32U cmd)
{
	INT32S	i;

	gpio_write_io(PANEL_IO_CSN, 1); //CS=1
	gpio_write_io(PANEL_IO_SCL, 1); //SCL=0
	gpio_write_io(PANEL_IO_SDA, 1); //SDA

	// set csn low
	gpio_write_io(PANEL_IO_CSN, 0); //CS=0
	cmd_delay(1);
	for (i = 0; i < 9; i++) {
		cmd_delay(1);
		gpio_write_io(PANEL_IO_SCL, 0); //SCL=0
		if (cmd & 0x100) {
			gpio_write_io(PANEL_IO_SDA, 1);    /* SDA = 1 */
		} else {
			gpio_write_io(PANEL_IO_SDA, 0);    /* SDA = 0 */
		}
		cmd <<= 1;
		cmd_delay(1);
		gpio_write_io(PANEL_IO_SCL, 1); //SCL=1
	}
	cmd_delay(1);
	//cmd_delay(5);
	gpio_write_io(PANEL_IO_SDA, 1);  /* SDA = 1 */
	gpio_write_io(PANEL_IO_SCL, 1);  //SCL=1
}

static void Write_COMMAND16i(INT32U data)
{
	serial_cmd_1(data);
}

static void Write_DATA16i(INT32U data)
{
	serial_cmd_1(0x100 | data);
}

static INT32S tft_st7789_565_init(INT32U tft_index)
{

	DBG_PRINT("%s\r\n", __func__);
    #if 1

	#if 0
	Write_COMMAND16i(0x11); //Sleep out
	drv_msec_wait(12); //Delay 120ms
	
	Write_COMMAND16i(0x36); //MX, MY, RGB mode	  Memory Data Access Control
	Write_DATA16i(0x00);
	
	Write_COMMAND16i(0x3A); //65k mode RAM control
	Write_DATA16i(0x55);
	Write_COMMAND16i(0x21);
	//Write_COMMAND16i(0xB0); 	//Set MCU inerface
	//Write_DATA16i(0xE0);
	
//	Write_COMMAND16i(0xE4);  //Gate Line Control
//	Write_DATA16i(0x20);
//	Write_DATA16i(0x05);
//	Write_DATA16i(0x10);


	Write_COMMAND16i(0xB0);
	Write_DATA16i(0x11);
	Write_DATA16i(0x00);	 //00 18w  04 6w
	
	Write_COMMAND16i( 0xB1);
Write_DATA16i( 0xe4);  //0xC0
Write_DATA16i( 0x0B);  //0x02
Write_DATA16i( 0x10);  //0x14

	Write_COMMAND16i(0xB2);   //Porch setting
	Write_DATA16i(0x0C);
	Write_DATA16i(0x0C);
	Write_DATA16i(0x00);
	Write_DATA16i(0x33);
	Write_DATA16i(0x33);
	
	Write_COMMAND16i(0xB7); 	//Gate control
	Write_DATA16i(0x75);
	
	Write_COMMAND16i(0xB8);  //Gate On Timing Adjustment
	Write_DATA16i(0x2A);
	Write_DATA16i(0x2B);
	Write_DATA16i(0x09);

 	Write_COMMAND16i(0xBB);   //VCOMS Setting
	Write_DATA16i(0x0C);

 	Write_COMMAND16i(0xC0);   //LCM Control
	Write_DATA16i(0x2C);
	
	Write_COMMAND16i(0xC2); 	//VDV and VRH register value comes from CMD or NVM
	Write_DATA16i(0x01);
	
	Write_COMMAND16i(0xC3);   //Set VAP=+4V/-4V
	Write_DATA16i(0x09);
	
	Write_COMMAND16i(0xC6);  	//Frame Rate Control in Normal mode
	Write_DATA16i(0x0F);
	
	Write_COMMAND16i(0xD0); 	//Power Control 1
	Write_DATA16i(0xA4);
	Write_DATA16i(0x31);	


	//Gamma Setting
	Write_COMMAND16i(0xe0);  //Positive gamma setting
	Write_DATA16i(0xf0);
	Write_DATA16i(0x04);
	Write_DATA16i(0x09);
	Write_COMMAND16i(0x0B);
	Write_DATA16i(0x0B);
	Write_COMMAND16i(0x06);
	Write_DATA16i(0x30);
	Write_DATA16i(0x44);
	Write_COMMAND16i(0x43);
	Write_DATA16i(0x3A);
	Write_DATA16i(0x15);
	Write_COMMAND16i(0x11);
	Write_DATA16i(0x18);
	Write_DATA16i(0x27);
	
	Write_COMMAND16i(0xe1);   //Nagetive gamma setting
	Write_DATA16i(0xf0);
	Write_DATA16i(0x04);
	Write_DATA16i(0x09);
	Write_COMMAND16i(0x0B);
	Write_DATA16i(0x0B);
	Write_COMMAND16i(0x06);
	Write_DATA16i(0x30);
	Write_DATA16i(0x44);
	Write_COMMAND16i(0x43);
	Write_DATA16i(0x3A);
	Write_DATA16i(0x15);
	Write_COMMAND16i(0x11);
	Write_DATA16i(0x18);
	Write_DATA16i(0x27);
	//----------------End ST7735S Gamma Sequence-------------//

	
	Write_COMMAND16i(0xD2); //Enable VAP/VAN signal output
	Write_DATA16i(0x4C);


	Write_COMMAND16i(0x2A); //Set Column Address
	Write_DATA16i(0x00);
	Write_DATA16i(0x00);
	Write_DATA16i(0x01);
	Write_DATA16i(0x3F);
	
	Write_COMMAND16i(0x2B); //Set Page Address
	Write_DATA16i(0x00);
	Write_DATA16i(0x00);
	Write_DATA16i(0x00);
	Write_DATA16i(0xeF);
	
		Write_COMMAND16i(0x21); 

	Write_COMMAND16i(0x29); 
	drv_msec_wait(5);
	Write_COMMAND16i(0x2c);  //Transfer data from MCU to frame memory.
	drv_msec_wait(2);
#else
	Write_COMMAND16i(0x11); //Sleep out
	drv_msec_wait (120); //Delay 120ms
	
	Write_COMMAND16i(0x36); //MX, MY, RGB mode	  Memory Data Access Control
	Write_DATA16i(0x00);
	
	Write_COMMAND16i(0x3A); //65k mode RAM control
	Write_DATA16i(0x05);
	
	//Write_COMMAND16i(0xB0); 	//Set MCU inerface
	//Write_DATA16i(0xE0);
	
//	Write_COMMAND16i(0xE4);  //Gate Line Control
//	Write_DATA16i(0x20);
//	Write_DATA16i(0x05);
//	Write_DATA16i(0x10);


	Write_COMMAND16i(0xB0);
	Write_DATA16i(0x11);
	Write_DATA16i(0x00);	 //00 18w  04 6w
	
	Write_COMMAND16i(0xB1);
	Write_DATA16i(0xA4);//0x40
	Write_DATA16i(0x0B);	
	Write_DATA16i(0x10);


	Write_COMMAND16i(0xB2);   //Porch setting
	Write_DATA16i(0x0C);
	Write_DATA16i(0x0C);
	Write_DATA16i(0x00);
	Write_DATA16i(0x33);
	Write_DATA16i(0x33);

	
	Write_COMMAND16i(0xB7); 	//Gate control
	Write_DATA16i(0x75);
	
	Write_COMMAND16i(0xB8);  //Gate On Timing Adjustment
	Write_DATA16i(0x2A);
	Write_DATA16i(0x2B);
	Write_DATA16i(0x09);

 	Write_COMMAND16i(0xBB);   //VCOMS Setting
	Write_DATA16i(0x0C);

 	Write_COMMAND16i(0xC0);   //LCM Control
	Write_DATA16i(0x2C);
	
	Write_COMMAND16i(0xC2); 	//VDV and VRH register value comes from CMD or NVM
	Write_DATA16i(0x01);
	
	Write_COMMAND16i(0xC3);   //Set VAP=+4V/-4V
	Write_DATA16i(0x09);
	
	Write_COMMAND16i(0xC6);  	//Frame Rate Control in Normal mode
	Write_DATA16i(0x1F);
	
	Write_COMMAND16i(0xD0); 	//Power Control 1
	Write_DATA16i(0xA4);
	Write_DATA16i(0x31);	


	//Gamma Setting
	Write_COMMAND16i(0xe0);  //Positive gamma setting
	Write_DATA16i(0xf0);
	Write_DATA16i(0x04);
	Write_DATA16i(0x09);
	Write_COMMAND16i(0x0B);
	Write_DATA16i(0x0B);
	Write_COMMAND16i(0x06);
	Write_DATA16i(0x30);
	Write_DATA16i(0x44);
	Write_COMMAND16i(0x43);
	Write_DATA16i(0x3A);
	Write_DATA16i(0x15);
	Write_COMMAND16i(0x11);
	Write_DATA16i(0x18);
	Write_DATA16i(0x27);
	
	Write_COMMAND16i(0xe1);   //Nagetive gamma setting
	Write_DATA16i(0xf0);
	Write_DATA16i(0x04);
	Write_DATA16i(0x09);
	Write_COMMAND16i(0x0B);
	Write_DATA16i(0x0B);
	Write_COMMAND16i(0x06);
	Write_DATA16i(0x30);
	Write_DATA16i(0x44);
	Write_COMMAND16i(0x43);
	Write_DATA16i(0x3A);
	Write_DATA16i(0x15);
	Write_COMMAND16i(0x11);
	Write_DATA16i(0x18);
	Write_DATA16i(0x27);
	//----------------End ST7735S Gamma Sequence-------------//

	
	Write_COMMAND16i(0xD2); //Enable VAP/VAN signal output
	Write_DATA16i(0x4C);


	Write_COMMAND16i(0x2A); //Set Column Address
	Write_DATA16i(0x00);
	Write_DATA16i(0x00);
	Write_DATA16i(0x00);
	Write_DATA16i(0xEF);
	
	Write_COMMAND16i(0x2B); //Set Page Address
	Write_DATA16i(0x00);
	Write_DATA16i(0x00);
	Write_DATA16i(0x01);
	Write_DATA16i(0x3F);
	
		Write_COMMAND16i(0x21); 

	Write_COMMAND16i(0x29); 
	drv_msec_wait(50);
	Write_COMMAND16i(0x2c);  //Transfer data from MCU to frame memory.
	drv_msec_wait(20);
#endif
	
     gpio_write_io(PANEL_IO_CSN, 1); //CS=1
    if(tft_index == TFT_2)
    {
        
        R_TFT2_CTRL &= ~(1<<31);//switching to bank0
        R_TFT2_HS_WIDTH			= 30;				//	2	=HPW
        R_TFT2_H_START			= 30+10;			    //		=HPW+HBP
        R_TFT2_H_END			= 30+10+240-1;		//		=HPW+HBP+HDE
        R_TFT2_H_PERIOD			= 30+10+240-1+20;	//		=HPW+HBP+HDE+HFP
        R_TFT2_VS_WIDTH			= 2;				//	1	=VPW				(LINE)
        R_TFT2_V_START			= 2+4;				//		=VPW+VBP			(LINE)
        R_TFT2_V_END			= 2+4+320-1;			//		=VPW+VBP+VDE		(LINE)
        R_TFT2_V_PERIOD			= 2+4+320-1;		    //		=VPW+VBP+VDE+VFP	(LINE)

        R_TFT2_CTRL |= (1<<31);//switching to bank1
        R_TFT2_LINE_RGB_ORDER = 0x00;		//RGB order
        R_TFT2_CTRL &= ~(1<<31);//switching to bank0
       //R_TFT2_LINE_RGB_ORDER = 0x00;
    }
    else
    { 
        R_TFT_HS_WIDTH =        1;				 	    //	1		=HPW
        R_TFT_H_START =         1 + 17;				//	240		=HPW+HBP
        R_TFT_H_END =           1 + 17 + 960;			//	1520	=HPW+HBP+HDE
        R_TFT_H_PERIOD =        1 + 17 + 960 + 10;	//	1560	=HPW+HBP+HDE+HFP
        R_TFT_VS_WIDTH =        1;					    //	1		=VPW				(DCLK)
        R_TFT_V_START =         3 + 0;					//	21		=VPW+VBP			(LINE)
        R_TFT_V_END =       3 + 0 + 240;				//	261		=VPW+VBP+VDE		(LINE)
        R_TFT_V_PERIOD =        3 + 0 + 240 + 4;		//	262		=VPW+VBP+VDE+VFP	(LINE)
        R_TFT_LINE_RGB_ORDER = 0x04 | (0x4 << 4);
    }
   

       drv_l1_disp_dev_rb_switch_set(tft_index,TRUE);
	drv_l1_disp_dev_data_mode_set(tft_index, TFT_DATA_MODE_565);
	drv_l1_disp_dev_signal_inv_set(tft_index,TFT_VSYNC_INV|TFT_HSYNC_INV,(TFT_ENABLE & TFT_VSYNC_INV)|(TFT_ENABLE & TFT_HSYNC_INV));
	drv_l1_disp_dev_mode_set(tft_index,TFT_MODE_PARALLEL);

	//drv_l1_disp_dev_target_clk_set(tft_index, 2000000);
	 drv_l1_disp_dev_clk_set(tft_index, TFT_CLK_DIVIDE_32);
	drv_l1_disp_dev_hdmi_en_set(tft_index, 0);
    drv_l1_disp_dev_vsync_unit_set(tft_index, 1);
    drv_l1_disp_dev_en_set(tft_index, TRUE);
  
    #endif
	return STATUS_OK;
}

// tft table
const DispCtrl_t TFT_Param_st7789_565=
{
	/* lcd size */
	240,
	320,

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
	tft_st7789_565_init,
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
	TFT_ROT_RIGHT,
	/* TFT  width*/
	240,
	/* TFT  heigh*/
	320,
};


#endif



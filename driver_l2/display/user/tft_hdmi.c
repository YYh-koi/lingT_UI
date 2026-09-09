#include "drv_l2.h"

#if (defined LCD_DRV_EN_HDMI) && (LCD_DRV_EN_HDMI == 1)

//this timing define order need follow drv_l1_hdmi.h HDMI_VID_
const HDMI_TFT_TIMIG_CFG hdmi_tft_cfg[4] = {
    {//HDMI_VID_480P
        .mode = TFT2_HDMI_EN,
        .hs_width = 0,
        .h_period = 858-1,
        .vs_width = 0,
        .v_period = 524,
        .v_start = 0x0009002D,
        .v_end = 0x000F020D-1,
        .h_start = 0x000F0089,
        .h_end = 0x004C0358,
        .hs_start = 0,
        .hs_end = 0,
        .vs_start = 0,
        .vs_end = 0,
        .hdmi_out_width = 720,
        .hdmi_out_height = 480,
    },
    {//HDMI_VID_720P
        .mode = TFT2_HDMI_EN,
        .hs_width = 0,
        .h_period = 1650-1,
        .vs_width = 2,
        .v_period = 750-1,
        .v_start = 0x0005001E,
        .v_end = 0x000A02ED,
        .h_start = 0x006D0171,
        .h_end = 0x00940670,
        .hs_start = 0,
        .hs_end = 0,
        .vs_start = 0,
        .vs_end = 0,
        .hdmi_out_width = 1280,
        .hdmi_out_height = 720,
    },
    {//HDMI_VID_1080P
        .mode = TFT2_HDMI_EN | TFT2_HDMI_FASTREQ,
        .hs_width = 0,
        .h_period = 2200-1,
        .vs_width = 0,
        .v_period = 1125-1,
        .v_start = 0x0004002D,
        .v_end = 0x00090465,
        .h_start = 0x00570117,
        .h_end = 0x00820896,
        .hs_start = 0,
        .hs_end = 0,
        .vs_start = 0,
        .vs_end = 0,
        .hdmi_out_width = 1920,
        .hdmi_out_height = 1080,
    },
    {//HDMI_VID_1080I
        .mode = TFT2_HDMI_EN | TFT2_HDMI_FASTREQ,
        .hs_width = 0x044C0003,
        .h_period = 0x00050897, //1080i only
        .vs_width = 0x23001502,
        .v_period = 0x00000231,
        .v_start = 0x00000014,
        .v_end = 0x0005022F,
        .h_start = 0x00570117,
        .h_end = 0x00820896,
        .hs_start = 0,
        .hs_end = 1,
        .vs_start = 0x2320000,
        .vs_end = 0x80008000,
        .hdmi_out_width = 1920,
        .hdmi_out_height = 1080,
    },
};

const HDMI_TFT_TIMIG_CFG hdmi_tft_cfg_50hz[4] = {
    {//HDMI_VID_480P_50HZ
        .mode = TFT2_HDMI_EN,
        .hs_width = 0,
        .h_period = 864-1,
        .vs_width = 0,
        .v_period = 625-1,
        .v_start = 0x00050031,//V_SYNC_START = (0x50000|V_VALID_START=5+44=49=0x31)
        .v_end = 0x000A0271,//V_SYNC_END = (5+5=10=0x0A|V_VALID_END=49+576=625=0x271)
        .h_start = 0x000B008F,//HSYNC_START = ((12-1)=11=0x0B/H_VALID_START=144-1=143=0x8F)
        .h_end = 0x004A035E,//HSYNC_END = (11+64-1)=74=0x4A|H_VALID_END=143+720-1=862=0x35E)
        .hs_start = 0,
        .hs_end = 0,
        .vs_start = 0,
        .vs_end = 0,
        .hdmi_out_width = 720,
        .hdmi_out_height = 480,
    },
    {//HDMI_VID_720P_50HZ
        .mode = TFT2_HDMI_EN,
        .hs_width = 0,
        .h_period = 1980-1,    //H_PERIOD=(440+40+220)+1280:
        .vs_width = 2,
        .v_period = 750-1,
        .v_start = 0x0005001E,
        .v_end = 0x000A02ED,
        .h_start = 0x01B702BB, //699(0x2BB)=(440+40+220)-1@H_START: Hfront=440(0x1B8)@H_SYNC_start
        .h_end = 0x01DE07BA,   //1979-1@H_END: Hfront+HSync-1=479(0x1DF)=440+40-1@H_SYNC_end
        .hs_start = 0,
        .hs_end = 0,
        .vs_start = 0,
        .vs_end = 0,
        .hdmi_out_width = 1280,
        .hdmi_out_height = 720,
    },
    {//HDMI_VID_1080P_50HZ
        .mode = TFT2_HDMI_EN | TFT2_HDMI_FASTREQ,
        .hs_width = 0,
        .h_period = 2640-1,    //H_PERIOD=(528+44+148)+1920:
        .vs_width = 0,
        .v_period = 1125-1,
        .v_start = 0x0004002D,
        .v_end = 0x00090465,
        .h_start = 0x020F02CF, //719(0x2CF)=(528+44+148)-1@H_START: Hfront=528(0x210)@H_SYNC_start:
        .h_end = 0x023A0A4E,  //2639-1@H_END: Hfront+HSync-1=571(0x23B)=528+44-1@H_SYNC_end
        .hs_start = 0,
        .hs_end = 0,
        .vs_start = 0,
        .vs_end = 0,
        .hdmi_out_width = 1920,
        .hdmi_out_height = 1080,
    },
    {//HDMI_VID_1080I_50HZ
        .mode = TFT2_HDMI_EN | TFT2_HDMI_FASTREQ,
        .hs_width = 0x05280000,
        .h_period = 0x00050A4F,//0x000504AF,//H_PERIOD=(528+44+148)+1920:
        .vs_width = 0x23001500,
        .v_period = 0x00000231,
        .v_start = 0x00000014,
        .v_end = 0x0005022F,
        .h_start = 0x020F02CF, //719(0x2CF)=(528+44+148)-1@H_START:-1 Hfront=528(0x210)@H_SYNC_start: -1
        .h_end = 0x023A0A4E,   //2639-1@H_END:Hfront+HSync-1=595(0x253)=528+44-1@H_SYNC_end
        .hs_start = 0,
        .hs_end = 1,
        .vs_start = 0x2320000,
        .vs_end = 0x80008000,
        .hdmi_out_width = 1920,
        .hdmi_out_height = 1080,
    },
};

#if C_ASYN_DBLE_TFT
INT32U do_once_flag=1;	
#endif

static INT32U hdmi_scaler_flag = 0;
static INT32U hdmi_scaler_hsize = 0;
static INT32U hdmi_scaler_vsize = 0;

INT32S drv_l2_display_tft2_hdmi_scaler_set(INT32U display_mode, INT32U h_size, INT32U v_size)
{
    switch(display_mode)
    {
        case DISDEV_TFT2_HDMI1080P:
        case DISDEV_TFT2_HDMI1080P_50HZ:
        case DISDEV_TFT2_DVI1080P:
        case DISDEV_TFT2_HDMI1080I:
        case DISDEV_TFT2_HDMI1080I_50HZ:
        case DISDEV_TFT2_DVI1080I:
            if((h_size != 1920) || (v_size != 1080))
            {
                hdmi_scaler_hsize = h_size;
                hdmi_scaler_vsize = v_size;
                hdmi_scaler_flag = 1;
            }
            else
                hdmi_scaler_flag = 0;
            break;

        case DISDEV_TFT2_HDMI720P:
        case DISDEV_TFT2_HDMI720P_50HZ:
        case DISDEV_TFT2_DVI720P:
            if((h_size != 1280) || (v_size != 720))
            {
                hdmi_scaler_hsize = h_size;
                hdmi_scaler_vsize = v_size;
                hdmi_scaler_flag = 1;
            }
            else
                hdmi_scaler_flag = 0;
            break;
        default:
            if((h_size != 720) || (v_size != 480))
            {
                hdmi_scaler_hsize = h_size;
                hdmi_scaler_vsize = v_size;
                hdmi_scaler_flag = 1;
            }
            else
                hdmi_scaler_flag = 0;
            break;
    }

    return 0;
}


INT32S tft2_hdmi_init(INT32U tft_index)
{
	#define HDMI_FAST_REQ_MODE              0  // 1:display color GP420 use system fifo(4), 2:display color YUYV use
	INT32U vid_mode,aud_mode = HDMI_AUD_48K;//HDMI_AUD_44K;//HDMI_AUD_OFF;
	INT32U dvi_flag = 0,hdmi50hz_flag = 0;
#if HDMI_FAST_REQ_MODE == 0
	INT32U color_mode,fast_mode_enable = 0;
#endif
	HDMI_TFT_TIMIG_CFG *hdmi_tft_cfg_ptr;
	INT32U hdmi_disp_resolution = HDMI_DISP_RESOLUTION;

	

#if	C_ASYN_DBLE_TFT
	#if LCD_DRV_EN_st7789_I80
	clr_st7789_display_for_double_TFT();
	#endif
	
	#if  LCD_DRV_EN_st7789_I80_240x240
	clr_st7789_240x240_display_for_double_TFT();
	#endif

	#if LCD_DRV_EN_KD055HD_IL9881C
	clr_KD055HD_IL9881C_display_for_double_TFT();
	#endif
	
#endif
	
    switch(hdmi_disp_resolution)
    {
        case DISDEV_TFT2_HDMI480P:
        case DISDEV_TFT2_DVI480P:
        case DISDEV_TFT2_HDMI480P_50HZ:
            vid_mode = HDMI_VID_480P;
            if(hdmi_disp_resolution == DISDEV_TFT2_DVI480P)
                dvi_flag = 1;
            if(hdmi_disp_resolution == DISDEV_TFT2_HDMI480P_50HZ)
                hdmi50hz_flag = 1;
            break;

        case DISDEV_TFT2_HDMI720P:
        case DISDEV_TFT2_DVI720P:
        case DISDEV_TFT2_HDMI720P_50HZ:
            vid_mode = HDMI_VID_720P;
            if(hdmi_disp_resolution == DISDEV_TFT2_DVI720P)
                dvi_flag = 1;
            if(hdmi_disp_resolution == DISDEV_TFT2_HDMI720P_50HZ)
                hdmi50hz_flag = 1;
        #if HDMI_FAST_REQ_MODE == 0
            fast_mode_enable = 1;
        #endif
            break;

        case DISDEV_TFT2_HDMI1080P:
        case DISDEV_TFT2_DVI1080P:
        case DISDEV_TFT2_HDMI1080P_50HZ:
            vid_mode = HDMI_VID_1080P;
            if(hdmi_disp_resolution == DISDEV_TFT2_DVI1080P)
                dvi_flag = 1;
            if(hdmi_disp_resolution == DISDEV_TFT2_HDMI1080P_50HZ)
                hdmi50hz_flag = 1;
        #if HDMI_FAST_REQ_MODE == 0
            fast_mode_enable = 1;
        #endif
            break;

        case DISDEV_TFT2_HDMI1080I:
        case DISDEV_TFT2_DVI1080I:
        case DISDEV_TFT2_HDMI1080I_50HZ:
            vid_mode = HDMI_VID_1080I;
            if(hdmi_disp_resolution == DISDEV_TFT2_DVI1080I)
                dvi_flag = 1;
            if(hdmi_disp_resolution == DISDEV_TFT2_HDMI1080I_50HZ)
                hdmi50hz_flag = 1;
            break;

       default:
            vid_mode = HDMI_VID_720P;
            break;
    }

    R_TFT2_CTRL &= ~BIT0;// TFT2 Disable
	
    if(dvi_flag)
        aud_mode = HDMI_AUD_OFF;

    if(hdmi50hz_flag)
        hdmi_tft_cfg_ptr = (HDMI_TFT_TIMIG_CFG *)&hdmi_tft_cfg_50hz[0];
    else
        hdmi_tft_cfg_ptr = (HDMI_TFT_TIMIG_CFG *)&hdmi_tft_cfg[0];

	
#if C_ASYN_DBLE_TFT
	if(do_once_flag)
#endif	
	{
#if HDMI_FAST_REQ_MODE == 1
	    // display color GP420
	    drv_l1_disp_dev_ppu_gp420_line_set(TFT_2, 1, 1, 3);
#elif HDMI_FAST_REQ_MODE == 2
	    // display color YUYV
	    drv_l1_disp_dev_ppu_gp420_line_set(TFT_2, 0, 1, 3);
#else
	    if(fast_mode_enable)
		{
			color_mode = drv_l2_display_tft_fmt_get();
			if(color_mode == DISP_FMT_GP420)
				drv_l1_disp_dev_ppu_gp420_line_set(TFT_2, 1, 1, 3);
			else
				drv_l1_disp_dev_ppu_gp420_line_set(TFT_2, 0, 1, 3);
	    }
#endif
		#if C_ASYN_DBLE_TFT
		do_once_flag=0;
		#endif
	}


    if(hdmi_scaler_flag)
    {
        drv_l1_hdmi_scaling_enable(hdmi_scaler_hsize, hdmi_scaler_vsize, 0);
        drv_l1_hdmi_scaling_up_size_set(hdmi_tft_cfg_ptr[vid_mode].hdmi_out_width, hdmi_tft_cfg_ptr[vid_mode].hdmi_out_height);
    }
    else
    {
    #if HDMI_SCALER == 1
        drv_l1_hdmi_scaling_enable(HDMI_SCALER_IN_WIDTH, HDMI_SCALER_IN_HEIGHT, 0);
        drv_l1_hdmi_scaling_up_size_set(hdmi_tft_cfg_ptr[vid_mode].hdmi_out_width, hdmi_tft_cfg_ptr[vid_mode].hdmi_out_height);
    #endif
    }
	
    //setup HDMI clock
    drv_l1_hdmi_set_ext_pll_clock(vid_mode);
    drv_l1_hdmi_set_audio_pll_clock(aud_mode);

    //the TFT2 by tft_index
    if(tft_index != TFT_2)
    {
        ERR_PRINT("HDMI must use TFT2\r\n");
        return STATUS_FAIL;
    }
	
    drv_l1_disp_dev_signal_inv_set(tft_index, TFT_VSYNC_INV | TFT_HSYNC_INV, TFT_VSYNC_INV | TFT_HSYNC_INV);
    drv_l1_disp_dev_mode_set(tft_index, TFT_MODE_PARALLEL); //  HDMI must  set "PARALLEL" mode
    drv_l1_disp_dev_data_mode_set(tft_index, TFT_DATA_MODE_8);
    drv_l1_disp_dev_clk_set(tft_index, TFT_CLK_DIVIDE_1); /* FS=66 */

    if(vid_mode == HDMI_VID_1080I) {
		/*
	    #if DISPLAY_DUAL_MODE_ENABLE == 1
	        if((TFT1_Param.lcd_width == 1920)&&(TFT1_Param.lcd_height == 540))
	        {
	    #else
	        if((TFT_Param.lcd_width == 1920)&&(TFT_Param.lcd_height == 540))
	        {
	    #endif
	            R_FREE_SIZE |= 0x40000000; //set HDMI interlace mode
	        }
	        else
	    */
			{
           	 R_FREE_SIZE |= 0xC0000000; //set ppu free size/HDM Iinterlace mode
			}
    #if HDMI_SCALER == 0
        R_TFT2_SCA_OUT_RES = (1080<<16) | 1920;
    #endif
    }

    R_TFT2_CTRL &= ~0xF0000;//clear mode
    R_TFT2_CTRL	|= hdmi_tft_cfg_ptr[vid_mode].mode;

    R_TFT2_HS_WIDTH = hdmi_tft_cfg_ptr[vid_mode].hs_width;
    R_TFT2_H_PERIOD	= hdmi_tft_cfg_ptr[vid_mode].h_period;
    R_TFT2_VS_WIDTH = hdmi_tft_cfg_ptr[vid_mode].vs_width;
    R_TFT2_V_PERIOD	= hdmi_tft_cfg_ptr[vid_mode].v_period;

    R_TFT2_V_START 	= hdmi_tft_cfg_ptr[vid_mode].v_start;
    R_TFT2_V_END	= hdmi_tft_cfg_ptr[vid_mode].v_end;
    R_TFT2_H_START 	= hdmi_tft_cfg_ptr[vid_mode].h_start;
    R_TFT2_H_END	= hdmi_tft_cfg_ptr[vid_mode].h_end;

    R_TFT2_VS_START	= hdmi_tft_cfg_ptr[vid_mode].vs_start;
    R_TFT2_VS_END   = hdmi_tft_cfg_ptr[vid_mode].vs_end;
    R_TFT2_HS_START = hdmi_tft_cfg_ptr[vid_mode].hs_start;
    R_TFT2_HS_END	= hdmi_tft_cfg_ptr[vid_mode].hs_end;

    if(hdmi50hz_flag)
        drv_l1_hdmi_init((HDMI_50HZ_EN|vid_mode), aud_mode);
    else
    drv_l1_hdmi_init(vid_mode, aud_mode);

	R_TFT2_CTRL |= BIT0;	/* TFT ON, PPU enable => HDMI is enabled now. */

	return STATUS_OK;
}

INT32S tft2_hdmi_uninit(INT32U tft_index)
{	
	/* must do nothing ! */
	return STATUS_OK;
}

// tft table
const DispCtrl_t TFT_Param_hdmi =
{
#if  (HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI480P)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI480P)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_DVI480P)
   720,
   480,
#elif (HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI720P)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI720P_50HZ)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_DVI720P)
   1280,
   720,
#elif (HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI1080P)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI1080I)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI1080P_50HZ)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI1080I_50HZ)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_DVI1080P)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_DVI1080I)
   1920,
   1080,
#else
   1920,
   1080,
#endif
	1,  //sync_pin_enable
	1,  //de_pin_enable
	0,  //te_pin_enable
	0,  //tft2_io_enable

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
	tft2_hdmi_init,
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
#if (HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI480P)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI480P)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_DVI480P)
    720,
    480
#elif (HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI720P)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI720P_50HZ)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_DVI720P)
    1280,
    720
#elif (HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI1080P)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI1080I)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI1080P_50HZ)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_HDMI1080I_50HZ)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_DVI1080P)||(HDMI_DISP_RESOLUTION == DISDEV_TFT2_DVI1080I)
    1920,
    1080
#else
    1920,
    1080
#endif
};

#endif//#if (defined LCD_DRV_EN_HDMI) && (LCD_DRV_EN_HDMI == 1)

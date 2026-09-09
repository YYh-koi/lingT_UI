/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2022 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
/********************************************************************
* Purpose:  COMS censor interface layer 1 driver
* Author:
* Date: 	2014/09/09
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
* Version : 1.00
* History :
*********************************************************************/
#include "drv_l1.h"

//================================================================//
/****************************************************
*		Definition 									*
****************************************************/
typedef struct
{									  //Offset
    volatile INT32U	CSI_GP420_WIDTH;  //022C
    volatile INT32U	CSI_GP420_HEIGHT; //0230
    volatile INT32U RSV1;             //0234
    volatile INT32U CSI_IRQ_STATUS;   //0238
    volatile INT32U CSI_IRQ_EN;       //023C
    volatile INT32U	CSI_TG_CTRL0;     //0240
    volatile INT32U	CSI_TG_CTRL1;     //0244
    volatile INT32U	CSI_TG_HLSTART;   //0248
    volatile INT32U	CSI_TG_HEND;      //024C
    volatile INT32U	CSI_TG_VL0START;  //0250
    volatile INT32U	CSI_MD_FBADDR;    //0254
    volatile INT32U	CSI_TG_VEND;      //0258
    volatile INT32U	CSI_TG_HSTART;    //025C
    volatile INT32U	CSI_MD_RGBL;      //0260
    volatile INT32U	CSI_SEN_CTRL;     //0264
    volatile INT32U	CSI_TG_BSUPPER;   //0268
    volatile INT32U	CSI_TG_BSLOWER;   //026C
    //volatile INT32U	CSI_MD_RGBH;      //0270
    volatile INT32U	CSI_MD_CTRL2;      //0270
    volatile INT32U	CSI_MD_CTRL;      //0274
    volatile INT32U	CSI_TG_FBSADDR;   //0278
    volatile INT32U RSV2;             //027C
    volatile INT32U	CSI_TG_VL1START;  //0280
    volatile INT32U	CSI_TG_HWIDTH;    //0284
    volatile INT32U	CSI_TG_VHEIGHT;   //0288
    volatile INT32U	CSI_TG_CUTSTART;  //028C
    volatile INT32U	CSI_TG_CUTSIZE;   //0290
    volatile INT32U	CSI_TG_VSTART;    //0294
    volatile INT32U RSV3;             //0298
    volatile INT32U	CSI_TG_HRATIO;    //029C
    volatile INT32U	CSI_TG_VRATIO;    //02A0
    volatile INT32U	CSI_MD_HPOS;      //02A4
    volatile INT32U	CSI_MD_VPOS;      //02A8
    volatile INT32U	CSI_TG_FBSADDR_B;  //02AC
    volatile INT32U RSV4;             //02B0
    volatile INT32U	CSI_MD_DIFF_CNT;  //02B4
}*CSI_SFR;
/****************************************************
*		varaible and data declaration				*
****************************************************/
static	CSI_CALLBACK_FUN csi_callback[CSI_NUM] = {NULL};

#define CSI_INT_FIQ 0
#if CSI_INT_FIQ == 1
#define CSI_INT_FIQ_NUM    VIC_FIQ_2
#endif

#define CSI_CLOCK_NUM      CLK_EN1_STN//CLK_EN1_STN_SEN_AHBMUX

static __inline CSI_SFR get_CSI_Reg_Base(CSI_ID CSI_Num)
{
	if(CSI_Num == CSI_0)
	{
        //set bank 0
        R_CSI2_BANK_SEL &= ~(0x1<<0);
		return (CSI_SFR)CSI_BASE;
	}
	else if(CSI_Num == CSI_1)
	{
        //set bank 1
        R_CSI2_BANK_SEL |= (0x1<<0);
        return (CSI_SFR)CSI_BASE;
	}
	else {
        DBG_PRINT("CSI ID err %d\r\n", CSI_Num);
        return 0;
    }

}

/*****************************************
*			CSI functions				 *
*****************************************/
/**
 * @brief   Set CSI input format function
 * @param   Interface : CSI_INPUT_INTERFACE_DEF
 * @param   Interlace : CSI_INTERLACE_MODE_DEF
 * @param   invert_clk : 0: no invert clock, 1: invert clock
 * @param   data_format : CSI_INPUT_DATA_DEF
 * @return 	none
 */
void drv_l1_csi_input_set(CSI_ID CSI_Num, CSI_INPUT_INTERFACE_DEF interface, CSI_INTERLACE_MODE_DEF interlace, INT32U invert_clk, CSI_INPUT_DATA_DEF data_format)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if (interface == CSI_HREF) {
		pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_HREF;
		pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_CCIR656;
	} else if (interface == CSI_HSYNC_CCIR_656) {
		pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_HREF;
		pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_CCIR656;
	} else {		// ENUM_CSI_HSYNC_CCIR_601
		pCSI_Reg->CSI_TG_CTRL0 &= ~(MASK_CSI_CTRL0_CCIR656 | MASK_CSI_CTRL0_HREF);
	}

	if (interlace == CSI_NON_INTERLACE) {
		pCSI_Reg->CSI_TG_CTRL0 &= ~(MASK_CSI_CTRL0_INTL | MASK_CSI_CTRL0_FIELDINV);
	} else if (interlace == CSI_INTERLACE) {
		pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_FIELDINV;
		pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_INTL;
	} else {	// ENUM_CSI_INTERLACE_INVERT_FIELD
		pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_INTL | MASK_CSI_CTRL0_FIELDINV;
	}

	if (invert_clk) {
		pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_CLKIINV;
	} else {
		pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_CLKIINV;
	}

	if (data_format < CSI_IN_UYVY) {
		// RGB
		pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_YUVIN;
		if (data_format == CSI_IN_RGB888_BGRG || data_format == CSI_IN_RGB888_GBGR) {
			pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_CCIR656;
		} else {
			pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_CCIR656;
		}

		if (data_format == CSI_IN_RGB888_BGRG || data_format == CSI_IN_RGB888_GBGR) {
			pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_YUVTYPE;
		} else {
			pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_YUVTYPE;
		}
	} else {
        // YUV
		pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_YUVIN;
		switch(data_format)
		{
            case CSI_IN_UYVY:
                pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_YUVTYPE;
                pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_INVYUVI;
                break;
            case CSI_IN_UYVY_INVERT_UV7:
                pCSI_Reg->CSI_TG_CTRL0 &= MASK_CSI_CTRL0_YUVTYPE;
                pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_INVYUVI;
                break;

            case CSI_IN_YUYV:
                pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_YUVTYPE;
                pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_INVYUVI;
                break;

            case CSI_IN_YUYV_INVERT_UV7:
                pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_YUVTYPE;
                pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_INVYUVI;
                break;
            default:break;
		}
	}
}
/**
 * @brief   CSI output data format set
 * @param   data_format : CSI_OUTPUT_DATA_DEF
 * @return 	none
 */
INT32S drv_l1_csi_output_data_format_set(CSI_ID CSI_Num, CSI_OUTPUT_DATA_DEF data_format)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(data_format < CSI_OUT_VYUY)
	{
        // RGB
		pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_YUVOUT;
		if (data_format == CSI_OUT_RGB565) {
			pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_RGB1555;
		} else {
			pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_RGB1555;
		}
	} else {		// YUV
		pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_YUVOUT;
		pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_YONLY;
		switch(data_format)
		{
            case CSI_OUT_VYUY:
                pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_INVYUVO;
                pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_RGB1555;
                break;
            case CSI_OUT_VYUY_INVERT_UV7:
                pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_INVYUVO;
                pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_RGB1555;
                break;

            case CSI_OUT_YUYV:
                pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_INVYUVO;
                pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_RGB1555;
                break;

            case CSI_OUT_YUYV_INVERT_UV7:
                pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_INVYUVO;
                pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_RGB1555;
                break;

            case CSI_OUT_Y_ONLY:
                pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_YONLY;
                break;
            case CSI_OUT_GP420:
                pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_INVYUVO;
                pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_RGB1555;
                pCSI_Reg->CSI_SEN_CTRL |= (0x1 << 16);
                break;
            default:break;
		}
	}

	return 0;
}
/**
 * @brief   CSI scale down set
 * @param   ho, vo : ratio
 * @return 	none
 */
INT32S drv_l1_csi_scale_down_set(CSI_ID CSI_Num, INT32U ho, INT32U vo)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

    pCSI_Reg->CSI_TG_HRATIO = ho;
    pCSI_Reg->CSI_TG_VRATIO = vo;

	return 0;
}
/**
 * @brief   CSI cubic output set
 * @param   enable - 1:enable
 * @param   size 0,1
 * @return 	none
 */
INT32S drv_l1_csi_cubic_output_set(CSI_ID CSI_Num, INT32U enable, INT32U size)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(enable)
	{
        pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_CUNICEN;
        if(size)
            pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_CUNIC_32_32;
        else
            pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CUNIC_32_32;
	}
    else
    {
        pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CUNICEN;
        pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CUNIC_32_32;
    }

	return 0;
}
/**
 * @brief   CSI cut screen set
 * @param   enable !V 1:enable
 * @param   cut_s !V cut start
 * @param   cut_e !V cut size
 * @return 	none
 */
INT32S drv_l1_csi_cut_screen_set(CSI_ID CSI_Num, INT32U enable, INT32U cut_s, INT32U cut_e)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(enable)
	{
        pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_CUTEN;
        pCSI_Reg->CSI_TG_CUTSTART = cut_s;
        pCSI_Reg->CSI_TG_CUTSIZE = cut_e;
	}
    else
    {
        pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CUTEN;
        pCSI_Reg->CSI_TG_CUTSTART = 0;
        pCSI_Reg->CSI_TG_CUTSIZE = 0;
    }
    if(pCSI_Reg->CSI_SEN_CTRL & 0x00010000) {//BIT16 GP420 enable
    //GP420 convert module is after cut effect, so set output as cut size
        pCSI_Reg->CSI_GP420_WIDTH = (cut_e&0xFF)*16;
        pCSI_Reg->CSI_GP420_HEIGHT = ((cut_e>>8)&0xFFF);
    }
	return 0;
}
/**
 * @brief   CSI black screen set
 * @param   Hs !V horizontal start
 * @param   He !V horizontal end
 * @param   Vs !V vertical start
 * @param   Ve !V vertical end
 * @return 	none
 */
INT32S drv_l1_csi_black_screen_set(CSI_ID CSI_Num, INT32U hs, INT32U he, INT32U vs, INT32U ve)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

    pCSI_Reg->CSI_TG_HSTART = hs;
    pCSI_Reg->CSI_TG_HEND = he;
    pCSI_Reg->CSI_TG_VSTART = vs;
    pCSI_Reg->CSI_TG_VEND = ve;

	return 0;
}
/**
 * @brief   CSI blue screen set
 * @param   enable !V 1:enable
 * @param   bs_upper !V upper
 * @param   bs_lower !V lower
 * @return 	none
 */
INT32S drv_l1_csi_blue_screen_set(CSI_ID CSI_Num, INT32U enable, INT32U bs_upper, INT32U bs_lower)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(enable)
	{
        pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_BSEN;
        pCSI_Reg->CSI_TG_BSUPPER = bs_upper;
        pCSI_Reg->CSI_TG_BSLOWER = bs_lower;
	}
    else
    {
        pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_BSEN;
        pCSI_Reg->CSI_TG_BSUPPER = 0;
        pCSI_Reg->CSI_TG_BSLOWER = 0;
    }

	return 0;
}
/**
 * @brief   CSI enable
 * @param   enable !V 1:enable
 * @return 	none
 */
INT32S drv_l1_csi_enable(CSI_ID CSI_Num, INT32U enable)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(enable)
        pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_CSIEN;
    else {
        pCSI_Reg->CSI_TG_CTRL0 = 0;//&= ~MASK_CSI_CTRL0_CSIEN;
        pCSI_Reg->CSI_SEN_CTRL = 0;
    }

	return 0;
}
/**
 * @brief   CSI clock out enable
 * @param   enable !V 1:enable
 * @return 	none
 */
INT32S drv_l1_csi_clockout_enable(CSI_ID CSI_Num, INT32U enable)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(enable)
        pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_CLKOEN;
    else
        pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CLKOEN;

	return 0;
}
/**
 * @brief   CSI sensor clock select
 * @param   sel
 * @return 	none
 */
void drv_l1_csi_sensor_clock_select(CSI_ID CSI_Num, INT32U sel)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

    if(sel)
        pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_CLK_SEL;
    else
        pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CLK_SEL;
}
/**
 * @brief   CSI field invert set
 * @param   enable !V 1:enable
 * @return 	none
 */
INT32S drv_l1_csi_field_invert_set(CSI_ID CSI_Num, INT32U enable)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(enable)
        pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_FIELDINV;
    else
        pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_FIELDINV;

	return 0;
}
/**
 * @brief   CSI clock stop set
 * @param   enable !V 1:enable
 * @return 	none
 */
INT32S drv_l1_csi_clock_stop_set(CSI_ID CSI_Num, INT32U enable)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(enable)
        pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_CLK_STOPB;
    else
        pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CLK_STOPB;

	return 0;
}
/**
 * @brief   CSI clock priority set
 * @param   enable !V 1:enable
 * @return 	none
 */
INT32S drv_l1_csi_clock_priority_set(CSI_ID CSI_Num, INT32U enable)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(enable)
        pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_CLK_PRI;
    else
        pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CLK_PRI;

	return 0;
}
/**
 * @brief   CSI preview mode set
 * @param   enable !V 1:enable
 * @return 	none
 */
INT32S drv_l1_csi_preview_mode_set(CSI_ID CSI_Num, INT32U enable)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(enable)
        pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_CAP;
    else
        pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_CAP;

	return 0;
}
/**
 * @brief   CSI long burst set
 * @param   enable !V 1:enable
 * @return 	none
 */
INT32S drv_l1_csi_long_burst_set(CSI_ID CSI_Num, INT32U enable)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(enable)
        pCSI_Reg->CSI_SEN_CTRL |= MASK_CSI_SEN_LONG_BURST;
    else
        pCSI_Reg->CSI_SEN_CTRL &= ~MASK_CSI_SEN_LONG_BURST;

	return 0;
}
/**
 * @brief   CSI jpeg mode set
 * @param   enable !V 1:enable
 * @return 	none
 */
/*
INT32S drv_l1_csi_jpeg_mode_set(CSI_ID CSI_Num, INT32U enable)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(enable)
        pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_JPEG;
    else
        pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_JPEG;

	return 0;
}
*/
/**
 * @brief   CSI clock out invert set
 * @param   enable !V 1:enable
 * @return 	none
 */
INT32S drv_l1_csi_clko_invert_set(CSI_ID CSI_Num, INT32U enable)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(enable)
        pCSI_Reg->CSI_TG_CTRL1 |= MASK_CSI_CTRL1_CLKOINV;
    else
        pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CLKOINV;

	return 0;
}
/**
 * @brief   CSI fifo mode set
 * @param   mode !V 0: no fifo mode
                   1:  8 lines fifo mode
                   2: 16 lines fifo mode
                   3: 32 lines fifo mode
 * @return 	none
 */
INT32S drv_l1_csi_fifo_mode_set(CSI_ID CSI_Num, INT32U mode)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

    pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_FIFO;
    pCSI_Reg->CSI_TG_CTRL0 |= (mode&0x3)<<20;

	return 0;
}
/**
 * @brief   CSI input resolution set
 * @param   width and height < 4095
 * @return 	none
 */
INT32S drv_l1_csi_input_resolution_set(CSI_ID CSI_Num, INT32U width, INT32U height)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if (width>4095 || height>4095) {
		return -1;
	}

	pCSI_Reg->CSI_TG_HWIDTH = width;
	pCSI_Reg->CSI_TG_VHEIGHT = height;
    pCSI_Reg->CSI_GP420_WIDTH = width;
    pCSI_Reg->CSI_GP420_HEIGHT = height;
	return 0;
}
/**
 * @brief   CSI input to pscaler set
 * @param   enable !V 1:enable
 * @return 	none
 */
INT32S drv_l1_csi_input_pscaler_set(CSI_ID CSI_Num, INT32U enable)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	if(enable)
        pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_INPUT_TO_PSCALER;
    else
        pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_INPUT_TO_PSCALER;

	return 0;
}
/**
 * @brief   CSI input latch timing1 set
 * @param   h_start !V 0~4095
 * @param   field0_vstart !V 0~4095
 * @param   field1_vstart !V 0~4095
 * @param   delay - CSI_LATCH_DELAY_ENUM
 * @return 	none
 */
// Latch control registers are used in HSYNC mode only. It is not used in HREF mode
void drv_l1_csi_input_latch_timing1_set(CSI_ID CSI_Num, INT32U h_start, INT32U field0_vstart, INT32U field1_vstart, CSI_LATCH_DELAY_ENUM delay)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	pCSI_Reg->CSI_TG_HLSTART = h_start & 0x0FFF;  				// Horizontal latch start
	pCSI_Reg->CSI_TG_VL0START = field0_vstart & 0x0FFF;  	// Field 0 vertical latch start
	pCSI_Reg->CSI_TG_VL1START = field1_vstart & 0x0FFF;		// Field 1 vertical latch start when interlace mode is used

	pCSI_Reg->CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_LATCH_DELAY_MASK;
	pCSI_Reg->CSI_TG_CTRL1 |= delay;
}
/**
 * @brief   CSI input latch timing2 set
 * @param   field !V CSI_LATCH_EDGE_ENUM
 * @param   v_reset !V CSI_LATCH_EDGE_ENUM
 * @param   v_increase !V CSI_LATCH_EDGE_ENUM
 * @param   h_reset - CSI_LATCH_EDGE_ENUM
 * @return 	none
 */
void drv_l1_csi_input_latch_timing2_set(CSI_ID CSI_Num, CSI_LATCH_EDGE_ENUM field, CSI_LATCH_EDGE_ENUM v_reset, CSI_LATCH_EDGE_ENUM v_increase, CSI_LATCH_EDGE_ENUM h_reset)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	// Latch field at falling(0) or rising(1) edge of VSYNC
	if (field == ENUM_CSI_FALLING_EDGE) {
		pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_FGET;
	} else {
		pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_FGET;
	}

	// Vertical counter increase at falling(0) or rising(1) edge of HSYNC
	if (v_increase == ENUM_CSI_FALLING_EDGE) {
		pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_VADD;
	} else {
		pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_VADD;
	}

	// Vertical counter reset at falling(0) or rising(1) edge of VSYNC
	if (v_reset == ENUM_CSI_FALLING_EDGE) {
		pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_VRST;;
	} else {
		pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_VRST;
	}

	// Horizontal counter reset at falling(0) or rising(1) edge of HSYNC
	if (h_reset == ENUM_CSI_FALLING_EDGE) {
		pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_HRST;
	} else {
		pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_HRST;
	}
}
/**
 * @brief   CSI register callback function set
 * @param   fun - CSI_CALLBACK_FUN
 * @return 	none
 */
void drv_l1_register_csi_cbk(CSI_ID CSI_Num, CSI_CALLBACK_FUN fun)
{
	csi_callback[CSI_Num] = fun;
}
/**
 * @brief   CSI state clear
 * @param   fun - CSI_CALLBACK_FUN
 * @return 	none
 */
void drv_l1_csi_state_clear(CSI_ID CSI_Num, INT32U flag)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

    pCSI_Reg->CSI_IRQ_STATUS = flag;
}
/**
 * @brief   CSI_IRQHandler
 * @param   none
 * @return 	none
 */
void CSI_IRQHandler()
{
	INT32U flag = R_TGR_IRQ_STATUS;
	INT32U enable = R_TGR_IRQ_EN;
	INT32U event = CSI_NONE_EVENT;

	//set to CSI bank
    R_CSI2_BANK_SEL &= ~(0x1<<0);
	flag &= enable;
	//DBG_PRINT("(%x)", R_TGR_IRQ_STATUS);
    if(flag & MASK_CSI_FRAME_END_FLAG)
	{
		/* Sensor frame end occurred */
		event |= CSI_SENSOR_FRAME_END_EVENT;
	}

	if(flag & MASK_CSI_OUTPUT_FIFO_FLAG)
	{
		/* Sensor FIFO mode occurred */
		event |= CSI_SENSOR_OUTPUT_FIFO_EVENT;
	}

	if(flag & MASK_CSI_FIFO_OVERFLOW_FLAG)
	{
		/* Sensor under run */
		event |= CSI_SENSOR_FIFO_OVERFLOW_EVENT;
	}

	if(flag & MASK_CSI_MOTION_DET_FLAG)
    {
		/* Sensor frame end occurred */
		event |= CSI_MOTION_DET_EVENT;
    }
	
	R_TGR_IRQ_STATUS = flag;

	if(event != CSI_NONE_EVENT && csi_callback[CSI_0] != NULL)
	{
		/* Call callback function come from sensor */
		(*csi_callback[CSI_0])(CSI_0, event);
	}
}
/**
 * @brief   CSI2_IRQHandler
 * @param   none
 * @return 	none
 */
void CSI2_IRQHandler()
{
	INT32U flag = R_TGR_IRQ_STATUS;
	INT32U enable = R_TGR_IRQ_EN;
	INT32U event = CSI_NONE_EVENT;

    //set to CSI2 bank
    R_CSI2_BANK_SEL |= (0x1<<0);
	flag &= enable;
	if(flag & MASK_CSI2_FRAME_END_FLAG)
	{
        if(flag & MASK_CSI2_OUTPUT_FIFO_FLAG)
            R_TGR_IRQ_STATUS = MASK_CSI2_OUTPUT_FIFO_FLAG;
		/* Sensor frame end occurred */
		R_TGR_IRQ_STATUS = MASK_CSI2_FRAME_END_FLAG;
		event = CSI_SENSOR_FRAME_END_EVENT;
	}
	else if(flag & MASK_CSI2_OUTPUT_FIFO_FLAG)
	{
		/* Sensor FIFO mode occurred */
		R_TGR_IRQ_STATUS = MASK_CSI2_OUTPUT_FIFO_FLAG;
		event = CSI_SENSOR_OUTPUT_FIFO_EVENT;
	}
	else if(flag & MASK_CSI2_FIFO_OVERFLOW_FLAG)
	{
		/* Sensor under run */
		R_TGR_IRQ_STATUS = MASK_CSI2_FIFO_OVERFLOW_FLAG;
		event = CSI_SENSOR_FIFO_OVERFLOW_EVENT;
	}
	else if(flag & MASK_CSI2_MOTION_DET_FLAG)
    {
		/* Sensor frame end occurred */
		R_TGR_IRQ_STATUS = MASK_CSI2_MOTION_DET_FLAG;
		event = CSI_MOTION_DET_EVENT;
    }
	else {
		R_TGR_IRQ_STATUS = flag;
	}

	if(event != CSI_NONE_EVENT && csi_callback[CSI_1] != NULL)
	{
		/* Call callback function come from sensor */
		(*csi_callback[CSI_1])(CSI_1, event);
	}
}
/**
 * @brief   CSI set bufferA
 * @param   buffer
 * @return 	none
 */
void drv_l1_csi_set_bufA(CSI_ID CSI_Num, INT32U buf)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	pCSI_Reg->CSI_TG_FBSADDR = buf;
}
/**
 * @brief   CSI set bufferB
 * @param   buffer
 * @return 	none
 */
void drv_l1_csi_set_bufB(CSI_ID CSI_Num, INT32U buf)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	pCSI_Reg->CSI_TG_FBSADDR_B = buf;
}
/**
 * @brief   CSI get bufferA
 * @param   none
 * @return 	buffer
 */
INT32U drv_l1_csi_get_bufA(CSI_ID CSI_Num)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	return (INT32U)pCSI_Reg->CSI_TG_FBSADDR;
}
/**
 * @brief   CSI get bufferB
 * @param   none
 * @return 	buffer
 */
INT32U drv_l1_csi_get_bufB(CSI_ID CSI_Num)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

	return (INT32U)pCSI_Reg->CSI_TG_FBSADDR_B;
}
/**
 * @brief   CSI set IRQ
 * @param   enable - 1:enable
 * @param   vic_num - 0:IRQ, 1~4: FIQ
 * @return 	buffer
 */
void drv_l1_csi_set_irq(CSI_ID CSI_Num, INT32U enable, INT8U vic_num)
{
    if(CSI_Num == CSI_0) {
        if(enable) {
			vic_irq_register(VIC_CSI, CSI_IRQHandler);
			vic_irq_enable(VIC_CSI);
			if (vic_num)
				vic_fiq_enable(VIC_CSI);
			else
				vic_fiq_disable(VIC_CSI);
        } else {
			vic_irq_disable(VIC_CSI);
			vic_fiq_disable(VIC_CSI);
        }
    } else {
		if(enable) {
			vic_irq_register(VIC_CSI2, CSI2_IRQHandler);
			vic_irq_enable(VIC_CSI2);
			if (vic_num)
				vic_fiq_enable(VIC_CSI2);
			else
				vic_fiq_disable(VIC_CSI2);
        } else {
			vic_irq_disable(VIC_CSI2);
			vic_fiq_disable(VIC_CSI2);
        }
    }
}
/**
 * @brief   CSI IRQ enable
 * @param   enable - 1:enable
 * @param   flag - enable/disable interrupt flag
 * @return 	buffer
 */
void drv_l1_csi_irq_enable(CSI_ID CSI_Num, INT32U enable, INT32U flag)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

    if(enable)
        pCSI_Reg->CSI_TG_CTRL0 |= MASK_CSI_CTRL0_OWN_IRQ;
    else
        pCSI_Reg->CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_OWN_IRQ;

    //enable use same base
    //set to CSI bank
    R_CSI2_BANK_SEL &= ~(0x1<<0);
    if(enable){
        if(CSI_Num == CSI_0) {
            pCSI_Reg->CSI_IRQ_EN |= flag;
        } else {
            pCSI_Reg->CSI_IRQ_EN |= (flag<<16);
        }
    } else {
        if(CSI_Num == CSI_0) {
            pCSI_Reg->CSI_IRQ_EN &= flag;
        } else {
            pCSI_Reg->CSI_IRQ_EN &= ~(flag<<16);
        }
    }
}
/**
 * @brief   CSI motion detect set threshold
 * @param   value - 0~0x7F
 * @return 	buffer
 */
void drv_l1_csi_md_set_threshold(CSI_ID CSI_Num, INT32U value)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

    pCSI_Reg->CSI_MD_CTRL |= ((value << 9) & (0x7F << 9));
}
/**
 * @brief   CSI motion detect set mode
 * @param   mode - 0: Average mode
                   1: Single mode
                   2: Average IIR mode
                   3: Single IIR mode
 * @return 	buffer
 */
void drv_l1_csi_md_set_mode(CSI_ID CSI_Num, INT32U value)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

    pCSI_Reg->CSI_MD_CTRL |= ((value << 6) & (0x3 << 6));
}
/**
 * @brief   CSI motion detect set frame
 * @param   Frame - 0: Detects every frame
                   1: Detects every 2 frames
                   2: Detects every 4 frames
                   3: Detects every 8 frames
 * @return 	buffer
 */
void drv_l1_csi_md_set_frame(CSI_ID CSI_Num, INT32U value)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

    pCSI_Reg->CSI_MD_CTRL |= ((value << 2) & (0x3 << 6));
}

void drv_l1_csi_md_set_new_parameter(CSI_ID CSI_Num, INT32U enable, INT32U img_h, INT32U img_v, INT32 char_mode)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);
    INT32U value = 0;
    INT32U size,temp;
    INT32U md2_en = 0;

    if(enable)
    {
        if((char_mode == 0) && ((img_h == 320) && (img_v == 240)))
        {
            size = 8;
            md2_en = 1;
}
        else if(char_mode == 2)
        {
            size = 32;
            md2_en = 1;
        }
        else if(char_mode == 3)
        {
            size = 64;
            md2_en = 1;
        }
        else
        {
            size = 16;
            md2_en = 1;
        }

        if(md2_en && enable)
        {
            value = 0;
            if(size == 32)
                value |= (1 << 2);
            else if(size == 64)
                value |= (2 << 2);
            else if(size == 8)
                value |= (3 << 2);
            value |= (1 << 0);
        }
        pCSI_Reg->CSI_MD_CTRL2 = value;

        value = 0;
        if(md2_en)
        {
            // block h number
            temp = (img_h / size);
            value |= ((temp-1) & 0x3F) << 16;
            // block v number
            temp = (img_v / size);
            value |= ((temp-1) & 0x3F) << 24;
        }
        else if((img_h == 640) && (img_v == 480))
            value |= (1 << 4);
        else if((img_h == 320) && (img_v == 240))
            value &= ~(1 << 4);
        else
        {
            // block h number
            temp = (img_h / size);
            value |= ((temp-1) & 0x3F) << 16;
            // block v number
            temp = (img_v / size);
            value |= ((temp-1) & 0x3F) << 24;
        }
        value |= (1 << 1);
    }

    pCSI_Reg->CSI_MD_CTRL = value;
}

/**
 * @brief   CSI motion detect set parameter
 * @param   enable !V 1:enale
 * @param   img_h, img_y !V size
 * @param   char_mode !V 0: 8x8 (only support QVGA), 1: 16x16
 * @return 	buffer
 */
void drv_l1_csi_md_set_parameter(CSI_ID CSI_Num, INT32U enable, INT32U img_h, INT32U img_v, INT32 char_mode)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);
    INT32U value = 0;
    INT32U size,temp,temp1;

    if(enable)
    {
        if((char_mode == 0) && ((img_h == 320) && (img_v == 240)))
            size = 8;
        else
            size = 16;

        if((img_h == 640) && (img_v == 480))
            value |= (1 << 4);
        else if((img_h == 320) && (img_v == 240))
            value &= ~(1 << 4);
        else
        {
            // block h number
            temp = (img_h / size);
            // h multiple of 4
            temp1 = (temp % 4);
            if(temp1 == 1)
                temp+=3;
            else if(temp1 == 2)
                temp+=2;
            else if(temp1)
                temp+=1;
            value |= ((temp-1) & 0x7F) << 16;
            // block v number
            temp = (img_v / size);
            value |= ((temp-1) & 0x7F) << 24;
        }
        value |= (1 << 1);
    }

    pCSI_Reg->CSI_MD_CTRL = value;
}
/**
 * @brief   CSI motion detect set buffer
 * @param   buf - buffer
 * @return 	none
 */
void drv_l1_csi_md_set_buf(CSI_ID CSI_Num, INT32U buf)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

    if(buf)
        pCSI_Reg->CSI_MD_FBADDR = buf;
}
/**
 * @brief   CSI motion detect get buffer
 * @param   none
 * @return 	buf - buffer
 */
INT32U drv_l1_csi_md_get_buf(CSI_ID CSI_Num)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

    return  (INT32U)pCSI_Reg->CSI_MD_FBADDR;
}

INT32U drv_l1_csi_md_get_diff_cnt(CSI_ID CSI_Num)
{
    CSI_SFR pCSI_Reg = get_CSI_Reg_Base(CSI_Num);

    return  (INT32U)pCSI_Reg->CSI_MD_DIFF_CNT;
}

/**
 * @brief   CSI motion detect get buffer
 * @param   img_h, img_y !V size
 * @param   char_mode !V 0: 8x8 (only support QVGA), 1: 16x16
 * @return 	buf - buffer
 */
INT32U drv_l1_csi_md_get_block_size(CSI_ID CSI_Num, INT32U img_h, INT32U img_v, INT32 char_mode)
{
    INT32U h,v,size,temp;

    if((char_mode == 0) && ((img_h == 320) && (img_v == 240)))
        size = 8;
    else if(char_mode == 2)
        size = 32;
    else if(char_mode == 3)
        size = 64;
    else
        size = 16;

    // block h number
    temp = (img_h / size);
    v = (temp % 4);
    if(v == 1)
        temp+=3;
    else if(v == 2)
        temp+=2;
    else if(v)
        temp+=1;
    h = temp;

    // block v number
    v = (img_v / size);
    temp = (img_v % size);
    if(temp)
        v++;

    return  (INT32U)(h*v);
}
/**
 * @brief   CSI init
 * @param   none
 * @return 	none
 */
INT32U drv_l1_csi_init(CSI_ID CSI_Num)
{
    drv_l1_clock_set_system_clk_en(CSI_CLOCK_NUM, 1);

    return 0;
}
/**
 * @brief   CSI uninit
 * @param   none
 * @return 	none
 */
INT32U drv_l1_csi_uninit(CSI_ID CSI_Num)
{
    if((R_TFT_CTRL & 0x1) == 0)
        drv_l1_clock_set_system_clk_en(CSI_CLOCK_NUM, 0);

    return 0;
}

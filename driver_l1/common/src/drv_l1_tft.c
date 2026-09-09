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
#include "drv_l1.h"

#define TFT_CLOCK_SOURCE				CLK_EN1_TFT  // CLK_EN0_TFT
#define TFT_CLOCK_SOURCE2				CLK_EN1_STN // CLK_EN1_TV_SPI0
#define PPU_CLOCK_SOURCE				CLK_EN1_PPU_FB_AHBMUX
#define FRAME_BUFFER_CLOCK_SOURCE		CLK_EN1_PPU_FRAME_BUF // CLK_EN1_DISPLAY_FRAME_BUFFER
#define DISPLAY_REG_CLOCK_SOURCE		CLK_EN1_PPU_REG_BANK	// CLK_EN1_DISPAY_OUT_REG
#define TFT_CLOCK_ENABLE				1
#define TFT_CLOCK_DISABLE				0
static INT32U tft_clock_init[TFT_NUM] = {0};

// extern INT32U MCLK;

static INT16S drv_l1_tft_clock_set(TFT_DEV_ID disp_dev, INT16U enable)
{
	INT16S state, ret, value = 0;

#if 1
	if(disp_dev == TFT_1)
	{
		/*
		ret = drv_l1_clock_set_system_clk_en(TFT_CLOCK_SOURCE, enable);
		if(ret)
			value = ret;
		*/

		// if(enable && (ret == 0))
		if(enable)
			tft_clock_init[TFT_1] = 1;
		else
			tft_clock_init[TFT_1] = 0;
	}
	else
	{
		ret = drv_l1_clock_set_system_clk_en(TFT_CLOCK_SOURCE2, enable);
		if(ret)
			value = ret;

		if(enable && (ret == 0))
			tft_clock_init[TFT_2] = 1;
		else
			tft_clock_init[TFT_2] = 0;
	}

	if(enable)
	{
		state = 1;
	}
	else
	{
		if((tft_clock_init[TFT_1] == 0) && (tft_clock_init[TFT_2] == 0))
			state = 0;
		else
			state = 1;
	}

	ret = drv_l1_clock_set_system_clk_en(TFT_CLOCK_SOURCE, state);
	if(ret)
		value = ret;

	ret = drv_l1_clock_set_system_clk_en(DISPLAY_REG_CLOCK_SOURCE, state);
	if(ret)
		value = ret;

	ret = drv_l1_clock_set_system_clk_en(FRAME_BUFFER_CLOCK_SOURCE, state);
	if(ret)
		value = ret;

	ret = drv_l1_clock_set_system_clk_en(PPU_CLOCK_SOURCE, state);
	if(ret)
		value = ret;
#endif
	return value;
}

static void drv_l1_tft_msec_wait(INT32U msec)
{
#if 1
	INT32U cnt = (INT32U)(msec * (SystemCoreClock >> 13)); // 0.64ms
	INT32U i;

	for(i = 0; i < cnt; i++)
	{
		i = cnt;
	}
#else
#if 1
	osDelay(msec);
#else
	drv_msec_wait(msec);
#endif
#endif
}

/**
 * @brief   Initialize TFT controller
 * @param   none
 * @return 	none
 */
void drv_l1_tft_init(void)
{
	drv_l1_tft_clock_set(TFT_1, TFT_CLOCK_ENABLE);
	R_TFT_CTRL = 0;
	drv_l1_tft_slide_en_set(FALSE);
}

/**
 * @brief   Uninitialize TFT controller
 * @param   none
 * @return 	none
 */
void drv_l1_tft_uninit(void)
{
	R_TFT_CTRL = 0;
	drv_l1_tft_clock_set(TFT_1, TFT_CLOCK_DISABLE);
}

/**
 * @brief   TFT controller enable
 * @param   enable[in]: 1: enable, 0: disable
 * @return 	none
 */
void drv_l1_tft_en_set(INT32U enable)
{
	if(enable)
	{
		R_TFT_CTRL |= TFT_EN;
	}
	else
	{
		R_TFT_CTRL &= ~TFT_EN;
	}
}

/**
 * @brief   TFT controller set clock
 * @param   clk[in]: TFT_CLK_DIVIDE_1 ~ TFT_CLK_DIVIDE_32
 * @return 	none
 */
void drv_l1_tft_clk_set(INT32U clk)
{
	INT32U clock, clock_hi;

	clock = R_TFT_CTRL;
	clock_hi = R_TFT_TS_MISC;

	clock &= ~TFT_CLK_SEL;
	clock_hi &= ~0xC0;

	if(clk < TFT_CLK_DIVIDE_9)
	{
		clock |= clk;
		R_TFT_CTRL = clock;
	}
	else
	{
		clock |= (clk & 0xF);
		clock_hi |= ((clk & 0x20) << 1);
		clock_hi |= ((clk & 0x10) << 3);
		R_TFT_CTRL = clock;
		R_TFT_TS_MISC = clock_hi;
	}
}
void drv_l1_tft2_clk_set(INT32U clk)
{
	INT32U clock, clock_hi;

	clock = R_TFT2_CTRL;
	clock_hi = R_TFT_TS_MISC;

	clock &= ~TFT_CLK_SEL;
	clock_hi &= ~0xC0;

	if(clk < TFT_CLK_DIVIDE_9)
	{
		clock |= clk;
		R_TFT2_CTRL = clock;
	}
	else
	{
		clock |= (clk & 0xF);
		clock_hi |= ((clk & 0x20) << 1);
		clock_hi |= ((clk & 0x10) << 3);
		R_TFT2_CTRL = clock;
		R_TFT_TS_MISC = clock_hi;
	}
}


/**
 * @brief   TFT set control mode
 * @param   mode[in]: TFT_MODE_UPS051 ~ TFT_MODE_MEM_ONCE
 * @return 	none
 */
void drv_l1_tft_mode_set(INT32U mode)
{
	INT32U reg = R_TFT_CTRL;

	reg &= ~TFT_MODE;
	reg |= mode;
	R_TFT_CTRL = reg;
}
void drv_l1_tft2_mode_set(INT32U mode)
{
	INT32U reg = R_TFT2_CTRL;

	reg &= ~TFT_MODE;
	reg |= mode;
	R_TFT2_CTRL = reg;
}

/**
 * @brief   set vsync, hsync, dclk and DE inverse
 * @param   mask[in]: mask bit
 * @param   value[in]: P_TFT_CTRL signal control bit
 * @return 	none
 */
void drv_l1_tft_signal_inv_set(INT32U mask, INT32U value)
{
	/*set vsync,hsync,dclk and DE inv */
	INT32U reg = R_TFT_CTRL;

	reg &= ~mask;
	reg |= (mask & value);
	R_TFT_CTRL = reg;
}

/**
 * @brief   set horizontal compression
 * @param   status[in]: enable or disable
 * @return 	none
 */
void drv_l1_tft_h_compress_set(INT32U enable)
{
	if(enable)
	{
		R_TFT_CTRL |= TFT_H_COMPRESS;
	}
	else
	{
		R_TFT_CTRL &= ~TFT_H_COMPRESS;
	}
}

/**
 * @brief   set i80 data width 8 or 16bit
 * @param   data_width[in]: 8 or 16
 * @return 	none
 */
void drv_l1_tft_mem_unit_set(INT8U data_width)
{
	if(data_width == 16)
	{
		R_TFT_CTRL &= ~TFT_MEM_BYTE_EN; /* word */
	}
	else
	{
		R_TFT_CTRL |= TFT_MEM_BYTE_EN; /* byte */
	}
}
void drv_l1_tft2_mem_unit_set(INT8U data_width)
{
	if(data_width == 16)
	{
		R_TFT2_CTRL &= ~TFT_MEM_BYTE_EN; /* word */
	}
	else
	{
		R_TFT2_CTRL |= TFT_MEM_BYTE_EN; /* byte */
	}
}

/**
 * @brief   set tft is interlace or non-interlace
 * @param   interlace[in]: 0 is non interlace, 1 is interlace
 * @return 	none
 */
void drv_l1_tft_interlace_set(INT32U interlace)
{
	if(interlace)
	{
		R_TFT_CTRL |= TFT_INTERLACE_MOD;
	}
	else
	{
		R_TFT_CTRL &= ~TFT_INTERLACE_MOD;
	}
}

/**
 * @brief   set tft unit base dclk or hsync
 * @param   base_dclk[in]: 0: base unit is dclk, 1: base unit is hsync
 * @return 	none
 */
void drv_l1_tft_vsync_unit_set(INT32U base_dclk)
{
	if(base_dclk)
	{
		R_TFT_CTRL |= TFT_VSYNC_UNIT;
	}
	else
	{
		R_TFT_CTRL &= ~TFT_VSYNC_UNIT;
	}
}

/**
 * @brief   set tft data mode
 * @param   mode[in]: TFT_DATA_MODE_8/TFT_DATA_MODE_565/TFT_DATA_MODE_666/TFT_DATA_MODE_888
 * @return 	none
 */
void drv_l1_tft_data_mode_set(INT32U mode)
{
	INT32U reg = R_TFT_TS_MISC;

	reg &= ~TFT_DATA_MODE;
	reg |= mode;
	R_TFT_TS_MISC = reg;
}

/**
 * @brief   set R[7:3] switch with B[7:3]
 * @param   enable[in]: enable or disable
 * @return 	none
 */
void drv_l1_tft_rb_switch_set(INT32U enable)
{
	if(enable)
	{
		R_TFT_TS_MISC |= TFT_SWITCH_EN;
	}
	else
	{
		R_TFT_TS_MISC &= ~TFT_SWITCH_EN;
	}
}

/**
 * @brief   set slide window enable
 * @param   enable[in]: enable or disable
 * @return 	none
 */
void drv_l1_tft_slide_en_set(INT32U enable)
{
	if(enable)
	{
		R_TFT_TS_MISC |= TFT_SLIDE_EN;
	}
	else
	{
		R_TFT_TS_MISC &= ~TFT_SLIDE_EN;
	}
}

/**
 * @brief   set TFT DCLK shift selection based on system clock
 * @param   sel[in]: TFT_DCLK_SEL_0 ~ TFT_DCLK_SEL_270
 * @return 	none
 */
void drv_l1_tft_dclk_sel_set(INT32U sel)
{
	INT32U reg = R_TFT_TS_MISC;

	reg &= ~TFT_DCLK_SEL;
	reg |= sel;
	R_TFT_TS_MISC = reg;
}

/**
 * @brief   set TFT DCLK shift selection based on DELC cell
 * @param   delay[in]: TFT_DCLK_DELAY_0 ~ TFT_DCLK_DELAY_7
 * @return 	none
 */
void drv_l1_tft_dclk_delay_set(INT32U delay)
{
	INT32U reg = R_TFT_TS_MISC;

	reg &= ~TFT_DCLK_DELAY;
	reg |= delay;
	R_TFT_TS_MISC = reg;
}

/**
 * @brief   set TFT display buffer address
 * @param   addr[in]: buffer address
 * @return 	none
 */
void drv_l1_tft_display_buffer_set(INT32U addr)
{
	if(addr)
	{
		R_TFT_FBI_ADDR = addr;
	}
}

void drv_l1_tft_reg_pol_set(INT8U pol)
{
	if(pol)
	{
		R_TFT_LINE_RGB_ORDER |= TFT_NEW_POL_EN;
	}
	else
	{
		R_TFT_LINE_RGB_ORDER &= ~TFT_NEW_POL_EN;
	}
}

/**
 * @brief   set TFT i-80 mode, write command
 * @param   command[in]: command
 * @return 	none
 */
void drv_l1_tft_i80_wr_cmd(INT16U command)
{
	INT32U reg = R_TFT_CTRL;

	R_TFT_MEM_BUFF_WR = command;
	reg &= ~(0xF << 4);
	reg |= TFT_MODE_MEM_CMD_WR | TFT_EN;
	R_TFT_CTRL = reg;
}

/**
 * @brief   set TFT i-80 mode, write data
 * @param   data[in]: data
 * @return 	none
 */
void drv_l1_tft_i80_wr_data(INT16U data)
{
	INT32U reg = R_TFT_CTRL;

	R_TFT_MEM_BUFF_WR = data;
	reg &= ~(0xF << 4);
	reg |= TFT_MODE_MEM_DATA_WR | TFT_EN;
	R_TFT_CTRL = reg;
}

/**
 * @brief   set TFT i-80 mode, read data
 * @param   command[in]: command
 * @return 	data
 */
INT16U drv_l1_tft_i80_read_data(INT16U command)
{
	INT32U reg, data;

	drv_l1_tft_i80_wr_cmd(command);

	drv_l1_tft_msec_wait(1);

	reg = R_TFT_CTRL;
	reg &= ~(0xF << 4);
	reg |= TFT_MODE_MEM_DATA_RD | TFT_EN;
	R_TFT_CTRL = reg;

	data = R_TFT_MEM_BUFF_RD;
	data = R_TFT_MEM_BUFF_RD;
	data = R_TFT_MEM_BUFF_RD;
	return data;
}

/**
 * @brief   set TFT target clock, read data
 * @param   tar_clk[in]: target clock, unit:Hz
 * @return 	none
 */
INT16U drv_l1_tft_target_clk_set(INT32U tar_clk)
{
	INT32U clock_div;

	clock_div = MCLK / tar_clk;
	clock_div = ((clock_div - 1) << 1);
	if(clock_div > TFT_CLK_DIVIDE_32)
		clock_div = TFT_CLK_DIVIDE_32;
	drv_l1_tft_clk_set(clock_div);

	return 0;
}

//******************************************************************************//
// Display Device
static __inline INT32U get_Disp_Dev_Reg_Base(TFT_DEV_ID disp_dev)
{
	if(disp_dev == TFT_1)
	{
		return (INT32U)0xD0500140;
	}
	else
	{
		return (INT32U)0xD05002C0;
	}
}

/**
 * @brief   Initialize TFT2 banl selection
 * @param   dev_num[in]: device id
 * @param   enable[in]: 1: enable, 0: disable
 * @return 	none
 */
void drv_l1_disp_bank_sel_en_set(TFT_DEV_ID dev_num, INT32U enable)
{
	TFT2_DEVICE_SFR TFT2_Reg = (TFT2_DEVICE_SFR)get_Disp_Dev_Reg_Base(dev_num);

	if(dev_num == TFT_2)
	{
		if(enable)
			TFT2_Reg->TFT_CTRL |= TFT2_BANK_SEL_EN;
		else
			TFT2_Reg->TFT_CTRL &= ~TFT2_BANK_SEL_EN;
	}
}

/**
 * @brief   set slide window enable
 * @param   dev_num[in]: device id
 * @param   enable[in]: enable or disable
 * @return 	none
 */
void drv_l1_disp_dev_slide_en_set(TFT_DEV_ID dev_num, INT32U enable)
{
	if(dev_num == TFT_1)
	{
		if(enable)
		{
			R_TFT_TS_MISC |= TFT_SLIDE_EN;
		}
		else
		{
			R_TFT_TS_MISC &= ~TFT_SLIDE_EN;
		}
	}
	else
	{
		if(enable)
		{
			R_TFT_TS_MISC |= TFT2_SLIDE_EN;
		}
		else
		{
			R_TFT_TS_MISC &= ~TFT2_SLIDE_EN;
		}
	}
}

/**
 * @brief   Initialize TFT controller
 * @param   dev_num[in]: device id
 * @return 	none
 */
void drv_l1_disp_dev_init(TFT_DEV_ID dev_num)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(dev_num);

	drv_l1_tft_clock_set(dev_num, TFT_CLOCK_ENABLE);
	if(dev_num == TFT_2)
	{
		R_SYSTEM_PERI_CLK_CTRL |= (0x1 << 2);
		R_SYSTEM_TMDS_CTRL |= (0x1 << 15);
	}
	TFT_Reg->TFT_CTRL = 0;
	drv_l1_disp_dev_slide_en_set(dev_num, FALSE);
}

/**
 * @brief   Uninitialize TFT controller
 * @param   dev_num[in]: device id
 * @return 	none
 */
void drv_l1_disp_dev_uninit(TFT_DEV_ID dev_num)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(dev_num);

	TFT_Reg->TFT_CTRL = 0;
	drv_l1_tft_clock_set(dev_num, TFT_CLOCK_DISABLE);
}

/**
 * @brief   TFT controller enable
 * @param   dev_num[in]: device id
 * @param   enable[in]: 1: enable, 0: disable
 * @return 	none
 */
void drv_l1_disp_dev_en_set(TFT_DEV_ID dev_num, INT32U enable)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(dev_num);

	if(enable)
	{
		TFT_Reg->TFT_CTRL |= TFT_EN;
	}
	else
	{
		TFT_Reg->TFT_CTRL &= ~TFT_EN;
	}
}

/**
 * @brief   TFT controller set clock
 * @param   dev_num[in]: device id
 * @param   clk[in]: TFT_CLK_DIVIDE_1 ~ TFT_CLK_DIVIDE_32
 * @return 	none
 */
void drv_l1_disp_dev_clk_set(TFT_DEV_ID dev_num, INT32U clk)
{
	INT32U clock, clock_hi;
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(dev_num);

	clock = TFT_Reg->TFT_CTRL;
	clock_hi = R_TFT_TS_MISC;

	clock &= ~TFT_CLK_SEL;
	clock_hi &= ~0xC0;

	if(dev_num == TFT_1)
	{
		if(clk < TFT_CLK_DIVIDE_9)
		{
			clock |= clk;
			TFT_Reg->TFT_CTRL = clock;
		}
		else
		{
			clock |= clk & 0xF;
			clock_hi |= (clk & 0x20) << 1;
			clock_hi |= (clk & 0x10) << 3;
			TFT_Reg->TFT_CTRL = clock;
			R_TFT_TS_MISC = clock_hi;
		}
	}
	else
	{
		clock &= ~(3 << 20); // clear hi select bits

		if(clk < TFT_CLK_DIVIDE_9)
		{
			clock |= clk;
		}
		else
		{
			clock |= clk & 0xF;
			clock |= (clk & 0x20) << (20 - 5);
			clock |= (clk & 0x10) << (21 - 4);
		}
		TFT_Reg->TFT_CTRL = clock;
	}
}

/**
 * @brief   TFT set control mode
 * @param   dev_num[in]: device id
 * @param   mode[in]: TFT_MODE_UPS051 ~ TFT_MODE_MEM_ONCE
 * @return 	none
 */
void drv_l1_disp_dev_mode_set(TFT_DEV_ID dev_num, INT32U mode)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(dev_num);
	INT32U reg = TFT_Reg->TFT_CTRL;

	reg &= ~TFT_MODE;
	reg |= mode;
	TFT_Reg->TFT_CTRL = reg;
}

/**
 * @brief   set vsync, hsync, dclk and DE inverse
 * @param   dev_num[in]: device id
 * @param   mask[in]: mask bit
 * @param   value[in]: P_TFT_CTRL signal control bit
 * @return 	none
 */
void drv_l1_disp_dev_signal_inv_set(TFT_DEV_ID dev_num, INT32U mask, INT32U value)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(dev_num);
	INT32U reg = TFT_Reg->TFT_CTRL;

	/*set vsync,hsync,dclk and DE inv */
	reg &= ~mask;
	reg |= (mask & value);
	TFT_Reg->TFT_CTRL = reg;
}

/**
 * @brief   set horizontal compression
 * @param   dev_num[in]: device id
 * @param   status[in]: enable or disable
 * @return 	none
 */
void drv_l1_disp_dev_h_compress_set(TFT_DEV_ID dev_num, INT32U enable)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(dev_num);

	if(enable)
	{
		TFT_Reg->TFT_CTRL |= TFT_H_COMPRESS;
	}
	else
	{
		TFT_Reg->TFT_CTRL &= ~TFT_H_COMPRESS;
	}
}

/**
 * @brief   set i80 data width 8 or 16bit
 * @param   dev_num[in]: device id
 * @param   data_width[in]: 8 or 16
 * @return 	none
 */
void drv_l1_disp_dev_mem_unit_set(TFT_DEV_ID dev_num, INT8U data_width)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(dev_num);

	if(data_width == 16)
	{
		TFT_Reg->TFT_CTRL &= ~TFT_MEM_BYTE_EN; /* word */
	}
	else
	{
		TFT_Reg->TFT_CTRL |= TFT_MEM_BYTE_EN; /* byte */
	}
}

/**
 * @brief   set tft is interlace or non-interlace
 * @param   dev_num[in]: device id
 * @param   interlace[in]: 0 is non interlace, 1 is interlace
 * @return 	none
 */
void drv_l1_disp_dev_interlace_set(TFT_DEV_ID dev_num, INT32U interlace)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(dev_num);

	if(interlace)
	{
		TFT_Reg->TFT_CTRL |= TFT_INTERLACE_MOD;
	}
	else
	{
		TFT_Reg->TFT_CTRL &= ~TFT_INTERLACE_MOD;
	}
}

/**
 * @brief   set tft unit base dclk or hsync
 * @param   dev_num[in]: device id
 * @param   base_dclk[in]: 0: base unit is dclk, 1: base unit is hsync
 * @return 	none
 */
void drv_l1_disp_dev_vsync_unit_set(TFT_DEV_ID dev_num, INT32U base_dclk)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(dev_num);

	if(base_dclk)
	{
		TFT_Reg->TFT_CTRL |= TFT_VSYNC_UNIT;
	}
	else
	{
		TFT_Reg->TFT_CTRL &= ~TFT_VSYNC_UNIT;
	}
}

/**
 * @brief   set tft data mode
 * @param   dev_num[in]: device id
 * @param   mode[in]: TFT_DATA_MODE_8/TFT_DATA_MODE_565/TFT_DATA_MODE_666/TFT_DATA_MODE_888
 * @return 	none
 */
void drv_l1_disp_dev_data_mode_set(TFT_DEV_ID dev_num, INT32U mode)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(dev_num);
	INT32U reg;

	if(dev_num == TFT_1)
	{
		reg = R_TFT_TS_MISC;
		reg &= ~TFT_DATA_MODE;
		reg |= (mode << B_TFT_DATA_MODE);
		R_TFT_TS_MISC = reg;
	}
	else
	{
		reg = TFT_Reg->TFT_CTRL;
		reg &= ~TFT2_DATA_MODE;
		reg |= (mode << B_TFT2_DATA_MODE);
		TFT_Reg->TFT_CTRL = reg;
	}
}

/**
 * @brief   set R[7:3] switch with B[7:3]
 * @param   dev_num[in]: device id
 * @param   enable[in]: enable or disable
 * @return 	none
 */
void drv_l1_disp_dev_rb_switch_set(TFT_DEV_ID dev_num, INT32U enable)
{
	TFT2_DEVICE_SFR TFT2_Reg = (TFT2_DEVICE_SFR)get_Disp_Dev_Reg_Base(TFT_2);

#if 1
	if(dev_num == TFT_1)
	{
		if(enable)
		{
			R_TFT_TS_MISC |= TFT_SWITCH_EN;
		}
		else
		{
			R_TFT_TS_MISC &= ~TFT_SWITCH_EN;
		}
	}
	else
	{
		if(enable)
		{
			TFT2_Reg->TFT_CTRL |= TFT2_SWITCH_EN;
		}
		else
		{
			TFT2_Reg->TFT_CTRL &= ~TFT2_SWITCH_EN;
		}
	}
#else
	if(enable)
	{

		R_TFT_TS_MISC |= TFT_SWITCH_EN;
	}
	else
	{
		R_TFT_TS_MISC &= ~TFT_SWITCH_EN;
	}
#endif
}

/**
 * @brief   set TFT DCLK shift selection based on system clock
 * @param   dev_num[in]: device id
 * @param   sel[in]: TFT_DCLK_SEL_0 ~ TFT_DCLK_SEL_270
 * @return 	none
 */
void drv_l1_disp_dev_dclk_sel_set(TFT_DEV_ID dev_num, INT32U sel)
{
	// TFT_DEVICE_SFR TFT_Reg = get_Disp_Dev_Reg_Base(dev_num);

	R_TFT_TS_MISC &= ~TFT_DCLK_SEL;
	R_TFT_TS_MISC |= sel;
}

/**
 * @brief   set TFT DCLK shift selection based on DELC cell
 * @param   dev_num[in]: device id
 * @param   delay[in]: TFT_DCLK_DELAY_0 ~ TFT_DCLK_DELAY_7
 * @return 	none
 */
void drv_l1_disp_dev_dclk_delay_set(TFT_DEV_ID dev_num, INT32U delay)
{
	// TFT_DEVICE_SFR TFT_Reg = get_Disp_Dev_Reg_Base(dev_num);

	R_TFT_TS_MISC &= ~TFT_DCLK_DELAY;
	R_TFT_TS_MISC |= delay;
}

/**
 * @brief   set TFT display buffer address
 * @param   dev_num[in]: device id
 * @param   addr[in]: buffer address
 * @return 	none
 */
void drv_l1_disp_dev_display_buffer_set(TFT_DEV_ID dev_num, INT32U addr)
{
	if(dev_num == TFT_2)
	{
		if(addr)
		{
			R_TV_FBI_ADDR = addr;
		}
	}
	else
	{
		if(addr)
		{
			R_TFT_FBI_ADDR = addr;
		}
	}
}

/**
 * @brief   set TFT2 HDMI enable
 * @param   dev_num[in]: device id
 * @param   enable[in]: 0:disable, 1:enable
 * @return 	none
 */
void drv_l1_disp_dev_hdmi_en_set(TFT_DEV_ID dev_num, INT32U enable)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(dev_num);

	if(dev_num == TFT_2)
	{
		if(enable)
			TFT_Reg->TFT_CTRL |= TFT2_HDMI_EN;
		else
			TFT_Reg->TFT_CTRL &= ~TFT2_HDMI_EN;
	}
}

/**
 * @brief   set TFT display pol enable
 * @param   dev_num[in]: device id
 * @param   pol[in]: enable or disable
 * @return 	none
 */
void drv_l1_disp_dev_reg_pol_set(TFT_DEV_ID dev_num, INT8U pol)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(TFT_1);
	TFT2_DEVICE_SFR TFT2_Reg = (TFT2_DEVICE_SFR)get_Disp_Dev_Reg_Base(TFT_2);

	if(dev_num == TFT_1)
	{
		if(pol)
		{
			TFT_Reg->TFT_LINE_RGB_ORDER |= TFT_NEW_POL_EN;
		}
		else
		{
			TFT_Reg->TFT_LINE_RGB_ORDER &= ~TFT_NEW_POL_EN;
		}
	}
	else
	{
		drv_l1_disp_bank_sel_en_set(TFT_2, 1);
		if(pol)
		{
			TFT2_Reg->TFT_V_END_2_LINE_RGB_ORDER |= TFT_NEW_POL_EN;
		}
		else
		{
			TFT2_Reg->TFT_V_END_2_LINE_RGB_ORDER &= ~TFT_NEW_POL_EN;
		}
		drv_l1_disp_bank_sel_en_set(TFT_2, 0);
	}
}

/**
 * @brief   set TFT display pol enable
 * @param   dev_num[in]: device id
 * @param   pol[in]: enable or disable
 * @return 	none
 */
INT32S drv_l1_disp_dev_ppu_gp420_line_set(TFT_DEV_ID dev_num, INT32U text_gp420_en, INT32U tvfb_yuv420_en, INT32U tvfb_buffer_yuv420_en)
{
	TFT2_DEVICE_SFR TFT_Reg = (TFT2_DEVICE_SFR)get_Disp_Dev_Reg_Base(TFT_2);
	INT32U reg;

	if(dev_num == TFT_1)
		return -1;

	drv_l1_disp_bank_sel_en_set(dev_num, 1);
	reg = TFT_Reg->TFT_VS_END_2_TEARING_EFF;
	if(text_gp420_en)
	{
		if(tvfb_yuv420_en)
		{
			reg &= ~TVFB_BUFFER_EN;
			reg |= YUV420_EXT_RAM;
		}
		else
		{
			reg &= ~YUV420_EXT_RAM;
		}

		if(tvfb_buffer_yuv420_en)
		{
			reg &= ~MASK_TVFB_BUFFER_THRES;
			reg &= ~MASK_YUV420_THRES;
			reg |= (tvfb_buffer_yuv420_en << B_YUV420_UNDERRUN);
		}
		else
		{
			reg &= ~MASK_YUV420_THRES;
		}
	}
	else
	{
		if(tvfb_yuv420_en)
		{
			reg |= TVFB_BUFFER_EN;
			reg &= ~YUV420_EXT_RAM;
		}
		else
		{
			reg &= ~TVFB_BUFFER_EN;
		}

		if(tvfb_buffer_yuv420_en)
		{
			reg &= ~MASK_YUV420_THRES;
			reg &= ~MASK_TVFB_BUFFER_THRES;
			reg |= (tvfb_buffer_yuv420_en << B_TVFB_BUFFER);
		}
		else
		{
			reg &= ~MASK_TVFB_BUFFER_THRES;
		}
	}
	TFT_Reg->TFT_VS_END_2_TEARING_EFF = reg;
	drv_l1_disp_bank_sel_en_set(dev_num, 0);

	return 0;
}

/**
 * @brief   set TFT display pol enable
 * @param   dev_num[in]: device id
 * @param   pol[in]: enable or disable
 * @return 	none
 */
INT32S drv_l1_disp_dev_hdmi_2048_set(TFT_DEV_ID dev_num, INT32U hdmi_2048_en)
{
#if 1
	return -1;
#else
	TFT2_DEVICE_SFR TFT_Reg = (TFT2_DEVICE_SFR)get_Disp_Dev_Reg_Base(TFT_2);

	if(dev_num == TFT_1)
		return -1;

	drv_l1_disp_bank_sel_en_set(dev_num, 1);
	if(hdmi_2048_en)
	{
		TFT_Reg->TFT_VS_END_2_TEARING_EFF |= HDMI_2048_EN;
	}
	else
	{
		TFT_Reg->TFT_VS_END_2_TEARING_EFF &= ~HDMI_2048_EN;
	}
	drv_l1_disp_bank_sel_en_set(dev_num, 0);

	return 0;
#endif
}

/**
 * @brief   set TFT i-80 mode, write command
 * @param   dev_num[in]: device id
 * @param   command[in]: command
 * @return 	none
 */
void drv_l1_disp_dev_i80_wr_cmd(TFT_DEV_ID dev_num, INT16U command)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(TFT_1);
	TFT2_DEVICE_SFR TFT2_Reg = (TFT2_DEVICE_SFR)get_Disp_Dev_Reg_Base(TFT_2);
	INT32U reg = TFT_Reg->TFT_CTRL;

	if(dev_num == TFT_2)
	{
		drv_l1_disp_bank_sel_en_set(TFT_2, 1);
#if 1
		TFT2_Reg->TFT_H_PERIOD_2_MEM_BUFF_WR = command;
#else
		R_TFT2_MEM_BUFF_WR = command;
#endif
		drv_l1_disp_bank_sel_en_set(TFT_2, 0);

		reg = TFT2_Reg->TFT_CTRL;
		reg &= ~(0xF << 4);
		reg |= TFT_MODE_MEM_CMD_WR | TFT_EN;
		TFT2_Reg->TFT_CTRL = reg;
	}
	else
	{
		TFT_Reg->TFT_MEM_BUFF_WR = command;

		reg = TFT_Reg->TFT_CTRL;
		reg &= ~(0xF << 4);
		reg |= TFT_MODE_MEM_CMD_WR | TFT_EN;
		TFT_Reg->TFT_CTRL = reg;
	}
}

/**
 * @brief   set TFT i-80 mode, write data
 * @param   dev_num[in]: device id
 * @param   data[in]: data
 * @return 	none
 */
void drv_l1_disp_dev_i80_wr_data(TFT_DEV_ID dev_num, INT16U data)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(TFT_1);
	TFT2_DEVICE_SFR TFT2_Reg = (TFT2_DEVICE_SFR)get_Disp_Dev_Reg_Base(TFT_2);
	INT32U reg;

	if(dev_num == TFT_2)
	{
		drv_l1_disp_bank_sel_en_set(TFT_2, 1);
#if 1
		TFT2_Reg->TFT_H_PERIOD_2_MEM_BUFF_WR = data;
#else
		R_TFT2_MEM_BUFF_WR = data;
#endif
		drv_l1_disp_bank_sel_en_set(TFT_2, 0);

		reg = TFT2_Reg->TFT_CTRL;
		reg &= ~(0xF << 4);
		reg |= TFT_MODE_MEM_DATA_WR | TFT_EN;
		TFT2_Reg->TFT_CTRL = reg;
	}
	else
	{
		TFT_Reg->TFT_MEM_BUFF_WR = data;

		reg = TFT_Reg->TFT_CTRL;
		reg &= ~(0xF << 4);
		reg |= TFT_MODE_MEM_DATA_WR | TFT_EN;
		TFT_Reg->TFT_CTRL = reg;
	}
}

/**
 * @brief   set TFT i-80 mode, read data
 * @param   dev_num[in]: device id
 * @param   command[in]: command
 * @return 	data
 */
INT16U drv_l1_disp_dev_i80_read_data(TFT_DEV_ID dev_num, INT16U command)
{
	TFT_DEVICE_SFR TFT_Reg = (TFT_DEVICE_SFR)get_Disp_Dev_Reg_Base(TFT_1);
	TFT2_DEVICE_SFR TFT2_Reg = (TFT2_DEVICE_SFR)get_Disp_Dev_Reg_Base(TFT_2);

	INT32U reg, data;

	drv_l1_disp_dev_i80_wr_cmd(dev_num, command);

	drv_l1_tft_msec_wait(1);

	reg = TFT_Reg->TFT_CTRL;
	reg &= ~(0xF << 4);
	reg |= TFT_MODE_MEM_DATA_RD | TFT_EN;
	TFT_Reg->TFT_CTRL = reg;

	if(dev_num == TFT_2)
	{
		drv_l1_disp_bank_sel_en_set(TFT_2, 1);
#if 1
		data = TFT2_Reg->TFT_H_PERIOD_2_MEM_BUFF_WR >> 16;
#else
		data = R_TFT2_MEM_BUFF_RD >> 16;
#endif
		drv_l1_disp_bank_sel_en_set(TFT_2, 0);
	}
	else
		data = TFT_Reg->TFT_MEM_BUFF_RD;

	return data;
}

/**
 * @brief   set TFT target clock, read data
 * @param   dev_num[in]: device id
 * @param   tar_clk[in]: target clock, unit:Hz
 * @return 	none
 */
INT16U drv_l1_disp_dev_target_clk_set(TFT_DEV_ID dev_num, INT32U tar_clk)
{
	INT32U clock_div;

	clock_div = MCLK / tar_clk;
	if((MCLK % tar_clk) >= (tar_clk >> 1))
	{
		++clock_div;
	}
	DBG_PRINT("================MCLK:%d, tar_clk:%d, clock_div:%d\r\n", MCLK, tar_clk, clock_div);
	if(clock_div == 0)
	{
		if(dev_num == TFT_2)
			clock_div = TFT_CLK_DIVIDE_2;
		else
			clock_div = TFT_CLK_DIVIDE_1;
	}
	clock_div = ((clock_div - 1) << 1);
	if(clock_div > TFT_CLK_DIVIDE_32)
		clock_div = TFT_CLK_DIVIDE_32;
	if(dev_num == TFT_2)
	{
		if(clock_div == TFT_CLK_DIVIDE_1)
			clock_div = TFT_CLK_DIVIDE_2;
	}

	drv_l1_disp_dev_clk_set(dev_num, clock_div);

	return 0;
}

/**
 * @brief   set TFT sync/de/te pin enable
 * @param   dev_num[in]: device id
 * @param   sync_en [in]:syne pin enable or disable
 * @param   de_en [in]: de pin enable or disable
 * @param   te_en [in]: te pin enable or disable
 * @return 	none
 */
void drv_l1_disp_dev_sync_de_te_set(TFT_DEV_ID dev_num, INT32U sync_en, INT32U de_en, INT32U te_en)
{
	INT32U funpos0 = R_TFTIOCTRL;
	INT32U temp = 0;

	if(dev_num == TFT_2)
	{
		temp = ((1 << 26) | (1 << 25) | (1 << 17));
		funpos0 &= ~temp;

		// sync_en
		if(sync_en)
			funpos0 |= (1 << 26);
		// de_en
		if(de_en)
			funpos0 |= (1 << 25);
		// te_en
		if(te_en)
			funpos0 |= (1 << 17);
	}
	else
	{
		temp = ((1 << 10) | (1 << 9) | (1 << 1));
		funpos0 &= ~temp;

		// sync_en
		if(sync_en)
			funpos0 |= (1 << 10);
		// de_en
		if(de_en)
			funpos0 |= (1 << 9);
		// te_en
		if(te_en)
			funpos0 |= (1 << 1);
	}

	R_TFTIOCTRL = funpos0;
}

void drv_l1_tft2_scaling_up_size_set(INT32U in_width, INT32U in_height, INT32U width_offset, INT32U height_offset, INT32U out_width, INT32U out_height)
{
#define LINE_BUFFER_EN 0
	// setup TFT2 scaler, scale up to hdmi_widthxhdmi_height
	INT32U W_factor, H_factor;

	W_factor = 65536 * in_width / out_width;

	if(W_factor >= 65536)
	{
#if LINE_BUFFER_EN == 1
		W_factor = 65535;
#else
		if(W_factor == 65536)
			W_factor = 0;
		else
			W_factor = 65535;
#endif
	}

	H_factor = 65536 * in_height / out_height;
	if(H_factor >= 65536)
	{
#if LINE_BUFFER_EN == 1
		H_factor = 65535;
#else
		if(H_factor == 65536)
			H_factor = 0;
		else
			H_factor = 65535;
#endif
	}

	R_TFT2_SCA_OFFSET = (height_offset << 16) | width_offset;
	R_TFT2_SCA_OUT_RES = (out_height << 16) | out_width;
	R_TFT2_SCA_FACTOR = (H_factor << 16) | W_factor;
}



/**
 * @brief   disable TE pin
 * @param   dev_num[in]: device id
 * @return 	none
 */
void drv_l1_tft_tepin_disable(TFT_DEV_ID dev_num)
{
	if(dev_num==TFT_2)
	{
		drv_l1_disp_bank_sel_en_set(TFT_2, 1);
		R_TFT2_TE_HS_WIDTH =0;
		R_TFT2_TE_CTRL =0;
		drv_l1_disp_bank_sel_en_set(TFT_2, 0);

	}
	else if (dev_num==TFT_1)
	{
		drv_l1_disp_bank_sel_en_set(TFT_1, 1);
		R_TFT_TE_HS_WIDTH =0;
		R_TFT_TE_CTRL =0;
		drv_l1_disp_bank_sel_en_set(TFT_1, 0);
	}
}
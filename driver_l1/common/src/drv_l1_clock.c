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

INT32U MCLK;
INT32U MHZ;

INT32U drv_l1_clock_cal_system_pll_freq(INT32U r_system_pllen)
{
    INT32U syspll = r_system_pllen;
    INT32U r_SYSTEM_CLK_CTRL = R_SYSTEM_CLK_CTRL;

    if ((r_SYSTEM_CLK_CTRL & (1<<14)) != 0) { // SW32K
        syspll = 32768; // 32K
        return syspll;
    }
    else if ((r_SYSTEM_CLK_CTRL & (1<<15)) == 0) { // SLOW mode instead of FAST mode
        syspll = 24000000; // 24M
        return syspll;
    }

    syspll &= 0x7F;
    syspll = ((syspll*18) + C_PLLN_MIN);

    syspll =  (syspll* XTAL_CLK_M)/XT24M;

    syspll = syspll * 1000000;

    return syspll;
}

INT32U drv_l1_clock_cal_system_clock_freq(INT32U r_system_pllen, INT32U r_system_ext_clk_ctrl)
{
    INT32U sysPll;
    INT8U clk_ratio,clk_div;

    // each step is 18MHz, plus basis 144MHz, then by clock divider and clock ration
    sysPll = drv_l1_clock_cal_system_pll_freq(r_system_pllen);

    clk_ratio = (r_system_ext_clk_ctrl>>8) & 0x3;
    //post divider
    if((r_system_ext_clk_ctrl&0xC000) == 0xC000) //bit14 post divider mux, bit15 post divider enable
        clk_div = (r_system_ext_clk_ctrl>>2) & 0x1F;
    else
        clk_div = 0;

    // GPA7XXXA system freq is divide by (clk_ratio + 1)
    sysPll = sysPll/(1+clk_div)/(1+clk_ratio);

    return sysPll;
}

INT32U drv_l1_clock_get_system_clock_freq(void)
{
    INT32U system_clock_freq;

    system_clock_freq = drv_l1_clock_cal_system_clock_freq(R_SYSTEM_PLLEN, R_SYSTEM_EXT_CLK_CTRL);

    return system_clock_freq;
}

INT32U drv_l1_clock_get_system_pll_freq(void)
{
    INT32U syspll;

    syspll = drv_l1_clock_cal_system_pll_freq(R_SYSTEM_PLLEN);

    return syspll;
}

INT32U drv_l1_clock_get_cpu_clock_freq(void)
{
    INT32U sysPll;
    INT8U clk_div;

    // each step is 18MHz, plus basis 144MHz, then by clock divider and clock ration
    sysPll = drv_l1_clock_get_system_pll_freq();

    //post divider
    if((R_SYSTEM_EXT_CLK_CTRL&0xC000) == 0xC000) //bit14 post divider mux, bit15 post divider enable
        clk_div = (R_SYSTEM_EXT_CLK_CTRL>>2) & 0x1F;
    else
        clk_div = 0;

    // GPA7XXXA cpu rate always equal PLL frequency
    sysPll = sysPll/(1+clk_div);

    return sysPll;
}

INT32U drv_l1_clock_set_ext_pll_freq(INT32U ext_pll, INT8U enable)
{
    INT32U temp;
    // each step is 9MHz, plus basis 72MHz, then by clock divider and clock ration
    if(ext_pll<72000000)
        ext_pll = 72000000;
    else if (ext_pll>639000000)
        ext_pll = 639000000;

    ext_pll = (ext_pll-72000000)/9000000;
    temp = R_SYSTEM_EXTCLK_CTRL;
    temp = (temp & (~0x4000)) | ((enable&0x1)<<14);
    temp = (temp & ((~(INT32U)0x3F)<<8)) | (ext_pll<<8);
    R_SYSTEM_EXTCLK_CTRL = temp;

    return (ext_pll+8)*9000000;
}

INT32U drv_l1_clock_get_ext_pll_freq(void)
{
    INT32U extPll;

    // each step is 9MHz, plus basis 72MHz, then by clock divider and clock ration
    extPll = (((R_SYSTEM_EXTCLK_CTRL>>8) & 0x3f) * 9UL) + 72;
    extPll = (extPll * XTAL_CLK_M)/XT24M;
    return extPll * 1000000;
}

INT32U drv_l1_clock_get_ddr_pll_freq(void)
{
    INT32U ddrPll;

    // each step is 24MHz, plus basis 36MHz
    ddrPll = (((R_SYSTEM_DDR_PLLN & 0xFF)>>2) * 24UL) + 36;

    ddrPll =  (ddrPll* XTAL_CLK_M)/XT24M;
    return (ddrPll/2) * 1000000;
}

INT32U drv_l1_clock_cal_ddr_pll_freq_idx(INT32U freq)
{
    INT32U ddrPll;
    INT32U ddrIdx;

    ddrPll = (freq * 2) / 1000000;
    if (ddrPll < 36)
        ddrPll = 36;
    else if (ddrPll > 1548)
        ddrPll = 1548;
    ddrIdx = (ddrPll - 36) / 24;

    return ddrIdx;
}

void drv_l1_clock_set_ddr_pll_freq(INT32U freq)
{
    INT32U ddrIdx;
    INT32U temp;

    ddrIdx = drv_l1_clock_cal_ddr_pll_freq_idx(freq);

    temp = R_SYSTEM_DDR_PLLN;
    temp = (temp & ~0xFC) | ((ddrIdx << 2) & 0xFC);
    R_SYSTEM_DDR_PLLN = temp;

    return;
}

void drv_l1_clock_set_ddr_pll_src(INT8U src)
{
    if(src)
        R_SYSTEM_DDR_CTRL |= BIT0;//from EXTPLL
    else
        R_SYSTEM_DDR_CTRL &= ~BIT0;//from DDRPLL
}

void drv_l1_clock_set_audio_pll_freq(INT8U base_freq, INT8U enable)
{
    if(base_freq)//0xD000015C [0] -> 0: 67.7376 Mhz  1: 73.728 Mhz
        R_SYSTEM_APLL_SEL |= BIT0;
    else
        R_SYSTEM_APLL_SEL &=~BIT0;

    if(enable)//0xD0000058[4] audio pll en
        R_SYSTEM_CKGEN_CTRL |= BIT4;
    else
        R_SYSTEM_CKGEN_CTRL &= ~BIT4;
}

INT32U drv_l1_clock_get_audio_pll_freq(void)
{
    INT32U apll;
    if(R_SYSTEM_APLL_SEL&BIT0)
        apll = 73728000;
    else
        apll = 67737600;
    apll = (apll*XTAL_CLK_M)/XT24M;
    return apll;
}

void drv_l1_clock_set_peripheral_pre_dvi2(INT8U enable)
{
    INT32U peri_clk;
    peri_clk = R_SYSTEM_PERI_CLK_CTRL;
    if(enable)
        peri_clk = (peri_clk & (~0x1));
    else
        peri_clk |= 0x1;

    R_SYSTEM_PERI_CLK_CTRL = peri_clk;
}

void drv_l1_clock_set_peripheral_src_extpll(INT8U enable)
{
    INT32U peri_clk;
    peri_clk = R_SYSTEM_PERI_CLK_CTRL;
    peri_clk = (peri_clk & (~0x2)) | ((enable & 0x1)<<1);
    R_SYSTEM_PERI_CLK_CTRL = peri_clk;
}

INT32U drv_l1_clock_get_peripheral_clock_source_freq(void)
{
    INT32U peri_clk;

    if((R_SYSTEM_PERI_CLK_CTRL&0x2) == 0x2) {//bit1 mux of select syspll or extpll
        peri_clk = drv_l1_clock_get_ext_pll_freq();
    } else {//syspll
        if((R_SYSTEM_PERI_CLK_CTRL&0x1) == 0x1)//bit0 whether need div2
            //peri_clk = drv_l1_clock_get_system_clock_freq();
            peri_clk = drv_l1_clock_get_system_pll_freq();  //ming for GPA7
        else
            //peri_clk = drv_l1_clock_get_system_clock_freq()/2;
            peri_clk = drv_l1_clock_get_system_pll_freq()/2;  //ming for GPA7
    }
    DBG_PRINT("periperal clock %d\r\n", peri_clk);
    return peri_clk;
}

void drv_l1_clock_set_peripheral_clock_freq(PERI_CLOCK_SRC_ENUM src, INT32U target_clock)
{
    INT32U div, reg, clk;

    if(src == SRC_AUDIO_MCLK)
        clk = drv_l1_clock_get_audio_pll_freq();
    else if(src == SRC_HDMI_AUDIO)
        clk = drv_l1_clock_get_audio_pll_freq()/3;//divide 3 is HW fix divider
    else
        clk = drv_l1_clock_get_peripheral_clock_source_freq();

    div = (clk + (target_clock/2))/target_clock;

    if(div>0)
        div -=1;

    if (div < 1)
        DBG_PRINT("WARNNING:peripheral_clock divider < 1, div %u, src %u, target_clock %u\r\n", div, src, target_clock);

    switch(src)
    {
        case SRC_SPU27M:
            reg = R_SYSTEM_SPU27M_DIV;
            reg &= ~(0x7F);
            reg |= div&0x7F;
            R_SYSTEM_SPU27M_DIV = reg;
        break;
        case SRC_C27M:
            reg = R_SYSTEM_TMDS_CTRL;
            reg &= ~(0x3F);
            reg |= div&0x3F;
            R_SYSTEM_TMDS_CTRL = reg;
        break;
        case SRC_TMDS:
            reg = R_SYSTEM_TMDS_CTRL;
            reg &= ~(0x3F00);
            reg |= (div&0x3F)<<8;
            R_SYSTEM_TMDS_CTRL = reg;
        break;
        case SRC_MIPI:
            reg = R_SYSTEM_MIPI_CTRL;
            reg &= ~(0x3F);
            reg |= div&0x3F;
            R_SYSTEM_MIPI_CTRL = reg;
        break;
        case SRC_DSI:
            reg = R_SYSTEM_DSI_CTRL;
            reg &= ~(0x3F);
            reg |= div&0x3F;
            R_SYSTEM_DSI_CTRL = reg;
        break;
        case SRC_LPRX_SCLK:
            reg = R_SYSTEM_DSI_CTRL;
            reg &= ~(0x3F00);
            reg |= (div&0x3F)<<8;
            R_SYSTEM_DSI_CTRL = reg;
        break;
        case SRC_APCK:
        /* no programming guide yet
            reg = R_SYSTEM_APCK_CTRL;
            reg &= ~(0x7F);
            reg |= div&0x7F;
            R_SYSTEM_APCK_CTRL = reg;
        */
        break;
        case SRC_AUDIO_MCLK:
            //for MIC and I2S
            reg = R_SYSTEM_CODEC_CTRL2;
            reg &= ~(0x3FF);
            reg |= div&0x3FF;
            R_SYSTEM_CODEC_CTRL2 = reg;
        break;
        case SRC_HDMI_AUDIO:
            reg = R_SYSTEM_CODEC_CTRL1;
            reg &= ~(0x3FF);
            reg |= div&0x3FF;
            R_SYSTEM_CODEC_CTRL1 = reg;
        break;
        default:
            DBG_PRINT("no this peripheral clock divider\r\n");
        break;
    }

}

static INT8U peri_clk_src_user_count[PERI_CLOCK_SRC_TOTAL] = {0};

void drv_l1_clock_set_peripheral_clock_enable(PERI_CLOCK_SRC_ENUM src, INT8U enable)
{
    switch(src)
    {
        //case SPU27M_DIV: //it depend on SPU module clock enable
        //break;
        case SRC_C27M:
            if(enable)
                R_SYSTEM_TMDS_CTRL |= 0x1<<7;
            else
                R_SYSTEM_TMDS_CTRL &= ~(0x1<<7);
        break;
        case SRC_TMDS:
            if(enable)
                R_SYSTEM_TMDS_CTRL |= 0x1<<15;
            else
                R_SYSTEM_TMDS_CTRL &= ~(0x1<<15);
        break;
        case SRC_MIPI: //MIPI_A & MIPI_B PLL CLK same register
            if(enable){
                R_SYSTEM_MIPI_CTRL |= 0x1<<7;
            }
            else{
                R_SYSTEM_MIPI_CTRL &= ~(0x1<<7);
            }
        break;
        case SRC_DSI:
            if(enable)
                R_SYSTEM_DSI_CTRL |= 0x1<<7;
            else
                R_SYSTEM_DSI_CTRL &= ~(0x1<<7);
        break;
        case SRC_LPRX_SCLK:
            if(enable)
                R_SYSTEM_DSI_CTRL |= 0x1<<15;
            else
                R_SYSTEM_DSI_CTRL &= ~(0x1<<15);
        break;
        case SRC_APCK:
        /*
        */
        break;
        //RD: R_SYSTEM_CODEC_CTRL1 no enable bit10~12
        case SRC_AUDIO_MCLK:
            if(enable) {
                peri_clk_src_user_count[SRC_AUDIO_MCLK]++;
                R_SYSTEM_CODEC_CTRL1 |= 0x1<<12;
            }
            else {
                if (peri_clk_src_user_count[SRC_AUDIO_MCLK] > 0)
                    peri_clk_src_user_count[SRC_AUDIO_MCLK]--;
                if (peri_clk_src_user_count[SRC_AUDIO_MCLK] == 0)
                    R_SYSTEM_CODEC_CTRL1 &= ~(0x1<<12);
            }
        break;
        case SRC_I2S_RX:
            if(enable) {
                peri_clk_src_user_count[SRC_I2S_RX]++;
                R_SYSTEM_CODEC_CTRL1 |= 0x1<<10;
            }
            else {
                if (peri_clk_src_user_count[SRC_I2S_RX] > 0)
                    peri_clk_src_user_count[SRC_I2S_RX]--;
                if (peri_clk_src_user_count[SRC_I2S_RX] == 0)
                    R_SYSTEM_CODEC_CTRL1 &= ~(0x1<<10);
            }
        break;
        case SRC_I2S:
            if(enable) {
                peri_clk_src_user_count[SRC_I2S]++;
                R_SYSTEM_CODEC_CTRL1 |= 0x1<<11;
            }
            else {
                if (peri_clk_src_user_count[SRC_I2S] > 0)
                    peri_clk_src_user_count[SRC_I2S]--;
                if (peri_clk_src_user_count[SRC_I2S] == 0)
                    R_SYSTEM_CODEC_CTRL1 &= ~(0x1<<11);
            }
        break;
        default:
            DBG_PRINT("no this peripheral clock\r\n");
        break;
    }
}

INT8U drv_l1_clock_get_peripheral_clock_enable_state(PERI_CLOCK_SRC_ENUM src)
{
    INT8U state = 0;

    switch(src)
    {
        //case SPU27M_DIV: //it depend on SPU module clock enable
        //break;
        case SRC_C27M:
            state = ((R_SYSTEM_TMDS_CTRL & (0x1<<7)) == 0) ? 0 : 1;
        break;
        case SRC_TMDS:
            state = ((R_SYSTEM_TMDS_CTRL & (0x1<<15)) == 0) ? 0 : 1;
        break;
        case SRC_MIPI:
            state = ((R_SYSTEM_MIPI_CTRL & (0x1<<7)) == 0) ? 0 : 1;
        break;
        case SRC_DSI:
            state = ((R_SYSTEM_DSI_CTRL & (0x1<<7)) == 0) ? 0 : 1;
        break;
        case SRC_LPRX_SCLK:
            state = ((R_SYSTEM_DSI_CTRL & (0x1<<7)) == 0) ? 0 : 1;
        break;
        case SRC_APCK:
        /*
        */
        break;
        case SRC_AUDIO_MCLK:
            state = ((R_SYSTEM_CODEC_CTRL1 & (0x1<<12)) == 0) ? 0 : 1;
        break;
        case SRC_I2S_RX:
            state = ((R_SYSTEM_CODEC_CTRL1 & (0x1<<10)) == 0) ? 0 : 1;
        break;
        case SRC_I2S:
            state = ((R_SYSTEM_CODEC_CTRL1 & (0x1<<11)) == 0) ? 0 : 1;
        break;
        default:
            DBG_PRINT("no this peripheral clock\r\n");
        break;
    }
    return state;
}

static I2S_MAIN_FREQ_E i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_12288MHZ;

INT32U drv_l1_clock_get_i2s_main_mclk_freq(void)
{
    INT32U freq;

    switch (i2s_main_mclk_freq_sel)
    {
        case I2S_MAIN_FREQ_73728MHZ:
            freq = 73728000;
            break;
        case I2S_MAIN_FREQ_36864MHZ: // div 2
            freq = 36864000;
            break;
        case I2S_MAIN_FREQ_24576MHZ: // div 3
            freq = 24576000;
            break;
        case I2S_MAIN_FREQ_18432MHZ: // div 4
            freq = 18432000;
            break;
        case I2S_MAIN_FREQ_14745MHZ: // div 5
            freq = 14745600;
            break;
        case I2S_MAIN_FREQ_12288MHZ: // div 6
            freq = 12288000;
            break;

        case I2S_MAIN_FREQ_67738MHZ: // div 1
            freq = 67737600;
            break;
        case I2S_MAIN_FREQ_33869MHZ: // div 2
            freq = 33868800;
            break;
        case I2S_MAIN_FREQ_22579MHZ: // div 3
            freq = 22579200;
            break;
        case I2S_MAIN_FREQ_16934MHZ: // div 4
            freq = 16934400;
            break;
        case I2S_MAIN_FREQ_13547MHZ: // div 5
            freq = 13547520;
            break;
        case I2S_MAIN_FREQ_11289MHZ: // div 6
            freq = 11289600;
            break;


        case I2S_MAIN_FREQ_16385MHZ:  // 73.728MHz div 4.5, 16.385MHz
            freq = 16384000;
            break;
        case I2S_MAIN_FREQ_8192MHz:   // 73.728MHz div 9, 8.192MHz
            freq = 8192000;
            break;
        case I2S_MAIN_FREQ_6144MHZ:   // 73.728 div12, 6.144 MHz
            freq = 6144000;
            break;
        case I2S_MAIN_FREQ_5644MHZ:   // 63.7376 div 12, 5.644 MHz
            freq = 5644800;
            break;
        case I2S_MAIN_FREQ_4096MHZ:   // 73.728 div 18, 4.096 MHz
            freq = 4096000;
            break;
        case I2S_MAIN_FREQ_3072MHZ:   // 73.728 div 24, 3.072 MHz
            freq = 3072000;
            break;
        case I2S_MAIN_FREQ_28224MHZ:  // 63.7376 div 24, 2.8224 MHz
            freq = 2822400;
            break;
        case I2S_MAIN_FREQ_2048MHZ:    // 73.728 div 36, 2.048 MHz
            freq = 2048000;
            break;
        default:
            freq = 12288000;
            break;
    }

    return (freq/1000);
}

INT32S drv_l1_clock_set_i2s_main_mclk_freq(I2S_MAIN_FREQ_E req_freq)
{
    INT32S ret = 0;

    switch (req_freq)
    {
        case I2S_MAIN_FREQ_73728MHZ:
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 73728000);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_73728MHZ;
            break;
        case I2S_MAIN_FREQ_36864MHZ: // div 2
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 36864000);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_36864MHZ;
            break;
        case I2S_MAIN_FREQ_24576MHZ: // div 3
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 24576000);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_24576MHZ;
            break;
        case I2S_MAIN_FREQ_18432MHZ: // div 4
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 18432000);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_18432MHZ;
            break;
        case I2S_MAIN_FREQ_14745MHZ: // div 5
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 14745600);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_14745MHZ;
            break;
        case I2S_MAIN_FREQ_12288MHZ: // div 6
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 12288000);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_12288MHZ;
            break;

        case I2S_MAIN_FREQ_67738MHZ: // div 1
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 67737600);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_67738MHZ;
            break;
        case I2S_MAIN_FREQ_33869MHZ: // div 2
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 33868800);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_33869MHZ;
            break;
        case I2S_MAIN_FREQ_22579MHZ: // div 3
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 22579200);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_22579MHZ;
            break;
        case I2S_MAIN_FREQ_16934MHZ: // div 4
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 16934400);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_16934MHZ;
            break;
        case I2S_MAIN_FREQ_13547MHZ: // div 5
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 13547520);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_13547MHZ;
            break;
        case I2S_MAIN_FREQ_11289MHZ: // div 6
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 11289600);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_11289MHZ;
            break;


        case I2S_MAIN_FREQ_16385MHZ:  // 73.728MHz div 4.5, 16.385MHz
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 16384000);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_16385MHZ;
            break;
        case I2S_MAIN_FREQ_8192MHz:   // 73.728MHz div 9, 8.192MHz
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 8192000);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_8192MHz;
            break;
        case I2S_MAIN_FREQ_6144MHZ:   // 73.728 div12, 6.144 MHz
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 6144000);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_6144MHZ;
            break;
        case I2S_MAIN_FREQ_5644MHZ:   // 63.7376 div 12, 5.644 MHz
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 5644800);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_5644MHZ;
            break;
        case I2S_MAIN_FREQ_4096MHZ:   // 73.728 div 18, 4.096 MHz
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 4096000);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_4096MHZ;
            break;
        case I2S_MAIN_FREQ_3072MHZ:   // 73.728 div 24, 3.072 MHz
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 3072000);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_3072MHZ;
            break;
        case I2S_MAIN_FREQ_28224MHZ:  // 63.7376 div 24, 2.8224 MHz
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 2822400);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_28224MHZ;
            break;
        case I2S_MAIN_FREQ_2048MHZ:    // 73.728 div 36, 2.048 MHz
            drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, 2048000);
            i2s_main_mclk_freq_sel = I2S_MAIN_FREQ_2048MHZ;
             break;
        default:
            ret = -1;
            break;
    }
    return ret;
}

INT16S drv_l1_clock_set_system_clk_en(INT16U module, INT16U enable)
{
    INT16S ret = 0;
    INT16U val;
    //INT32U inISR;
    INT8U bank;
    INT32U *ptr;

    if (module > CLK_EN_MAX)
        return ret = -1;

    bank = module/16;
    module = module%16;

    if(bank < 2){
        ptr = ((INT32U *)P_SYSTEM_CLK_EN0)+bank;
        val = *ptr;
        val &= ~ (1<< module);
        val |= (enable<<module);
        *ptr = val;
    }
    else if(bank<6){
        ptr = ((INT32U *)P_SYSTEM_CLK_EN2)+ (bank-2);
        val = *ptr;
        val &= ~ (1<< module);
        val |= (enable<<module);
        *ptr = val;
    }
    else{
        ptr = ((INT32U *)P_SYSTEM_CLK_EN6)+ (bank-6);
        val = *ptr;
        val &= ~ (1<< module);
        val |= (enable<<module);
        *ptr = val;
    }

    return ret;
}

INT16S drv_l1_clock_set_system_clk_en_isr(INT16U module, INT16U enable)
{
    INT16S ret = 0;
    INT16U val;
    //INT32U inISR;
    INT8U bank;
    INT32U *ptr;

    if (module > CLK_EN_MAX)
        return ret = -1;

	taskENTER_CRITICAL();

    bank = module/16;
    module = module%16;

    if(bank < 2){
        ptr = ((INT32U *)P_SYSTEM_CLK_EN0)+bank;
        val = *ptr;
        val &= ~ (1<< module);
        val |= (enable<<module);
        *ptr = val;
    }
    else if(bank < 6){
        ptr = ((INT32U *)P_SYSTEM_CLK_EN2)+ (bank-2);
        val = *ptr;
        val &= ~ (1<< module);
        val |= (enable<<module);
        *ptr = val;
    }
    else{
        ptr = ((INT32U *)P_SYSTEM_CLK_EN6)+ (bank-6);
        val = *ptr;
        val &= ~ (1<< module);
        val |= (enable<<module);
        *ptr = val;
    }

	taskEXIT_CRITICAL();

    return ret;
}

void drv_l1_clock_print_major_clock_info(void)
{
    INT32U temp1;
    INT32U temp2;
    INT32U temp3;
    INT32U temp4;

    temp1 = drv_l1_clock_get_system_clock_freq();
    temp2 = drv_l1_clock_get_cpu_clock_freq();
    temp3 = drv_l1_clock_get_system_pll_freq();
    temp4 = drv_l1_clock_get_ddr_pll_freq();

    DBG_PRINT("\r\nclock freq: sys=%u, cpu=%u, pll=%u, ddr=%u\r\n", temp1, temp2, temp3, temp4);

    return;
}

void drv_l1_gpu_clk_enable(INT8U enable)
{
    INT32U value;
    value = R_SYSTEM_EXT_CLK_CTRL & (~(1<<7));
    if(enable)
        value |= (1<<7);

    R_SYSTEM_EXT_CLK_CTRL = value;

}

void drv_l1_DSICLKX8_src_select(INT8U extpll)
{
    INT32U value;
    value = R_DSI_MISC_CTRL & (~(1<<12));
    if(extpll)
        value |= 1<<12;

    R_DSI_MISC_CTRL = value;
}

INT32S drv_l1_gpu_clk_setting(INT8U enable,GPU_CLK_SRC set, INT8U div)
{
    INT32U value;

    drv_l1_gpu_clk_enable(0);
    value = R_SYSTEM_EXT_CLK_CTRL & (~3);
    R_SYSTEM_EXT_CLK_CTRL= value |(set&0x3);
    if((set==0x4)||(set == 0x5)||(set == 0x7))
        return -1;

    /*if((set == PLLSYSx2)&&((R_SYSTEM_PLLEN&(0x3<<4))==0)){
        div=div<<1; //internal PLL/4
        if(div>4)
            div=8;
    }*/

    if((set&0x3)==0x2){
        drv_l1_DSICLKX8_src_select((set>>2) & 1);
        if (div){
            div--;
        }
        div &= 0x7;
        value = R_DSI_MISC_CTRL & (~(0x7<<9));
        value |= div<<9;
        R_DSI_MISC_CTRL = value;
    }
    drv_l1_gpu_clk_enable(enable);
    return 0;
}

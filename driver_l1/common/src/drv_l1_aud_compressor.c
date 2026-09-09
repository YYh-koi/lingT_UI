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

/**
 * @brief   set audio compressor attack and release
 * @param   attack :
 * @param   release :
 * @return 	none
 */
void drv_l1_audio_compressor_attack_release(INT16U attack, INT16U release)
{
    R_AUD_COMPRESSOR_ATT_REL = ((release&0x7FFF)<<16)|(attack&0x7FFF);
}
/**
 * @brief   set audio compressor enable
 * @param   1:enable, 0:disable
 * @return 	none
 */
void drv_l1_audio_compressor_enable(INT8U enable)
{
    if(enable)
        R_AUD_COMPRESSOR_THRES_EN |= 0x1<<31;
    else
        R_AUD_COMPRESSOR_THRES_EN &= ~(0x1<<31);
}
/**
 * @brief   set audio compressor threshold
 * @param   threshold
 * @return 	none
 */
void drv_l1_audio_compressor_threshold(INT16U threshold)
{
    INT32U reg = R_AUD_COMPRESSOR_THRES_EN;
    reg &= ~0x7FFF;
    reg |= threshold&0x7FFF;
    R_AUD_COMPRESSOR_THRES_EN = reg;
}
/**
 * @brief   set audio compressor up bound
 * @param   upbound
 * @return 	none
 */
void drv_l1_audio_compressor_up_bound(INT16U upbound)
{
    INT32U reg = R_AUD_COMPRESSOR_THRES_EN;
    reg &= ~(0x7FFF<<16);
    reg |= ((upbound&0x7FFF)<<16);
    R_AUD_COMPRESSOR_THRES_EN = reg;
}
/**
 * @brief   set audio compressor config
 * @param   mono :
 * @param   LR_ch :
 * @param   I2S_32bLR :
 * @param   sign : for DAC path
 * @param   src_sel :
 * @return 	none
 */
void drv_l1_audio_compressor_config(INT8U mono, INT8U LR_ch, INT8U I2S_32bLR, INT8U sign, INT8U src_sel)
{
    INT32U reg = R_AUD_COMPRESSOR_CFG1;
    reg &= ~((0x1<<30)|(0x1<<29)|(0x1<<28)|(0x3<<8));
    reg |= (((mono&0x1)<<30)|((LR_ch&0x1)<<29)|((I2S_32bLR&0x1)<<28)|((sign&0x1)<<27)|((src_sel&0x3)<<8));
    R_AUD_COMPRESSOR_CFG1 = reg;
}
/**
 * @brief   set audio compressor set gain dB
 * @param   GDB2D_idx :  0: -12dB
                        12: 0dB
                        24: 12dB
                        25: mute
 * @return 	none
 */
void drv_l1_audio_compressor_set_gain_dB(INT8U GDB2D_idx)
{
    INT32U reg = R_AUD_COMPRESSOR_CFG1;
    reg &= ~(0x1F);
    if(GDB2D_idx>25)
        reg |= 25;
    else
        reg |= (GDB2D_idx&0x1F);
    R_AUD_COMPRESSOR_CFG1 = reg;
}
/**
 * @brief   set audio source mux select
 * @param 	com_in
0000: original MIC record data
0001: original MIC record data(with delay fifo write timing)
0010: {agc_acc, original MIC record data}
0011: {PDM_R,PDM_L}
0100: {PDM_L,PDM_R}
0101: {agc_acc,PDM_L}
0110: {agc_acc,PDM_R}
0111: { original MIC record data,PDM_L}
1000: { original MIC record data,PDM_R}
1001: {det_noise,10’h0,PGA[4:0], original MIC record data}
1010: { det_noise,10’h0,PGA[4:0],PDM_L}
1011: { det_noise,10’h0,PGA[4:0],PDM_R}
1100: {DAC_in, original MIC record data}
1101: {DAC_in, PDM_L}
1110: {DAC_in, PDM_R}
1111: { det_noise,10’h0,PGA[4:0],1’b0,
 * @return 	none
 */
void drv_l1_adc_audio_mux_input_select(INT8U com_in)
{
    INT32U reg = R_PDM_RDS;
    reg &= ~((0xF<<0));
    reg |= (com_in&0xF)<<0;// compressor input selection
    R_PDM_RDS = reg;
}
/**
 * @brief   set audio output mux select
 * @param 	com_out
00000: compressor data out: CDO[15:0]
00001: {agc_acc,CDO[15:0]}
00010: {PDM_L,CDO[15:0]}
00011: {PDM_R,CDO[15:0]}
00100: {denoise_L, CDO[15:0]}
00101: {denoise_R, CDO[15:0]}
00110: {original MIC record data[15:0], CDO[15:0]}
00111: {original MIC record data[31:16], CDO[15:0]}
01000: { det_noise,10’h0,PGA[4:0],CDO[15:0]}
01001: {DAC_in,CDO[15:0]}
01010: { det_noise,10’h0,PGA[4:0],1’b0,agc_acc}
0xxxx: compressor data out: CDO[15:0]

10000: compressor data out: CDO[31:0]
10001: {agc_acc,CDO[31:16]}
10010: {PDM_L,CDO[31:16]}
10011: {PDM_R,CDO[31:16]}
10100: {denoise_L, CDO[31:16]}
10101: {denoise_R, CDO[31:16]}
10110: {original MIC record data[15:0], CDO[31:16]}
10111: {original MIC record data[31:16], CDO[31:16]}
11000: { det_noise,10’h0,PGA[4:0],CDO[31:16]}
11001: {DAC_in,CDO[31:16]}
11010: { det_noise,10’h0,PGA[4:0],1’b0,agc_acc}
1xxxx: compressor data out: CDO[31:0]
 * @return 	none
 */
void drv_l1_adc_audio_mux_output_select(INT16U com_out)
{
    INT32U reg = R_PDM_RDS;
    reg &= ~((0x1F<<4));
    reg |= (com_out&0x1F)<<4;//compressor output select
    R_PDM_RDS = reg;
}
/**
 * @brief   get audio compressor enable status
 * @param   none
 * @return 	ENABLE or DISABLE
 */
INT8U drv_l1_audio_compressor_get_enable_status(void)
{
    return (R_AUD_COMPRESSOR_THRES_EN&0x8000)>>31;
}
/**
 * @brief   init audio compressor
 * @param   1:enable, 0:disable
 * @return 	none
 */
void drv_l1_audio_compressor_init(INT8U enable)
{
    drv_l1_clock_set_system_clk_en(CLK_EN2_SAR_ADC, enable);//for R_PDM_RDS register
    drv_l1_clock_set_system_clk_en(CLK_EN7_COMPRESSOR, enable);
    drv_l1_clock_set_system_clk_en(CLK_EN1_SPU, enable);
}

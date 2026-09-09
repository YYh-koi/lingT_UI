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
 * @brief   pdm h/w enable
 * @param   1:enable, 0:disable
 * @return 	none
 */
void drv_l1_pdm_enable(INT8U enable)
{
    if(enable) {
        drv_l1_clock_set_system_clk_en(CLK_EN2_SAR_ADC,1);
        R_MIC_MACRO &= ~(0xF);
        R_PDM_RDS |= 0x1<<9;
        R_MIC_MACRO |= 0x1;
    } else {
        R_PDM_RDS &= ~(0x1<<9);
    }
}

/**
 * @brief   pdm input data L/R channel select
 * @param   1:Right channel, 0:Left channel
 * @return 	none
 */
void drv_l1_pdm_AGC_data_LR_select(INT8U sel)
{
    if(sel)
        R_PDM_RDS |= 0x1<<14;
    else
        R_PDM_RDS &= ~(0x1<<14);
}

/**
 * @brief   pdm sample edge select
 * @param
 *   00: LCH = data sample by clock rise edge
 *       RCH = data sample by clock fall edge
 *   01: LCH = data sample by clock fall edge
 *       RCH = data sample by clock rise edge
 *   10: LCH = data sample by clock rise edge
 *       RCH = data sample by clock rise edge
 *   11: LCH = data sample by clock fall edge
 *       RCH = data sample by clock fall edge
 * @return 	none
 */
void drv_l1_pdm_sample_edge(INT8U edge)
{
    R_PDM_RDS &= ~(0x3<<12);
    R_PDM_RDS |= (edge&0x3)<<12;
}

/**
 * @brief   pdm LRC data channel select
 * @param   1:LRC rise as Right channel, 0:LRC rise as Left channel
 * @return 	none
 */
void drv_l1_pdm_LRC_data_select(INT8U sel)
{
    if(sel)
        R_PDM_RDS |= 0x1<<11;
    else
        R_PDM_RDS &= ~(0x1<<11);
}

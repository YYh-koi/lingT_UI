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

/*
 * @brief: enable gp420 conv clock
 * @param: enable	enable or disable gp420 clock
 * @return: 0 for success, -1 for fail
*/
INT16S drv_l1_gp420_conv_clock_enable(INT8U enable)
{
	return drv_l1_clock_set_system_clk_en(CLK_EN0_YUV_REMAP,enable);
}
/*
 * @brief: This function sets the gp420 source buffer address.
 * @param: addr		gp420 buffer address
 * @return: none
*/
void drv_l1_gp420_conv_src_addr_set(INT32U addr)
{
    R_GP420_START_ADDRL = addr & 0xFFFF;
    R_GP420_START_ADDRH = (addr >> 16);
}

/*
 * @brief: This function sets the gp420 destination buffer address.
 * @param: addr		gp420 buffer address
 * @return: none
*/
void drv_l1_gp420_conv_dst_addr_set(INT32U addr)
{
    R_GP420_TARGET_ADDRL = addr & 0xFFFF;
    R_GP420_TARGET_ADDRH = (addr >> 16);
}

/*
 * @brief: This function set the gp420 image width
 * @param: width	image witdh with 16~8176,must be a multiple of 16
 * @return: none
*/
int drv_l1_gp420_conv_width_set(INT32U width)
{
    if ((width & 0xF) || (width > 8176)) {
        DBG_PRINT("invalid gp420 converter image width\r\n");
        return -1;
    }

    R_GP420_WIDTH &= ~0xFFF8;
    R_GP420_WIDTH |= (width << 3);

    return 0;
}

/*
 * @brief: This function sets the converter format to the RGB888 or the YUV422
 * @param: enable	1 is RGB888, 0 is YUV422
 * @return: none
*/
void drv_l1_gp420_conv_rgb_format_enable(INT32U enable)
{
    if (enable) {
        R_SYSTEM_MISC_CTRL7 |= 0x40; //RGB888
    } else {
        R_SYSTEM_MISC_CTRL7 &= ~0x40;
    }
}

/*
 * @brief: This function sets the yuv sequence in the ram when
 *        use the YUV422 format
 * @param: mode		indicate yuv order:
 *  0: V Y1 U Y0
 *  1: Y1 V Y0 U
 *  2: U Y1 V Y0
 *  3: Y1 U Y0 V
 * @return: none
*/
void drv_l1_gp420_conv_yuv_format_set(INT32U mode)
{
    R_SYSTEM_MISC_CTRL7 &= ~0x30;
    R_SYSTEM_MISC_CTRL7 |= (mode << 4);
}

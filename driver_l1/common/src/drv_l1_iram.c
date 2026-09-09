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
 * @brief   initialize iram control registers and set default iram overall size
 * @param   none
 * @return 	none
 */
void drv_l1_iram_ctrl_init(void)
{
	drv_l1_iram_clr_set(1);
	drv_l1_iram_set_overall_size(1, C_IRAM_SIZE_KB);

    drv_l1_iram_clr_set(2);
    drv_l1_iram_set_overall_size(2, C_IRAM_SIZE_KB);

}

/**
 * @brief   Set Internal Ram Overall Size
 * @param   iram_num: 1: iram1; 2: iram2
 *          size: overall size in KBytes
 * @return 	0: STATUS_OK
 *         -1: STATUS_FAIL
 */
INT32S drv_l1_iram_set_overall_size(INT32U iram_num, INT32U size)
{
	INT32U reg_value;

	if( size > MAX_IRAM_SIZE ){
		size = MAX_IRAM_SIZE;
	}

    if(iram_num == 1){
        reg_value = R_IRAM1_OVERALL_SIZE;
        reg_value &= (~0x3FF);
        reg_value |= size;
        R_IRAM1_OVERALL_SIZE = reg_value;
    }else if(iram_num == 2){
        reg_value = R_IRAM2_OVERALL_SIZE;
        reg_value &= (~0x3FF);
        reg_value |= size;
        R_IRAM2_OVERALL_SIZE = reg_value;
    }else
        return STATUS_FAIL;
    return STATUS_OK;
}

/**
 * @brief   Get Internal Ram Overall Size
 * @param   iram_num: 1: iram1; 2: iram2
 * @return 	Current internal ram overall size
 *          -1: STATUS_FAIL
 */
INT32S drv_l1_iram_get_overall_size(INT32U iram_num)
{
    if(iram_num == 1)
        return (R_IRAM1_OVERALL_SIZE & 0x3FF);
    else if(iram_num == 2)
        return (R_IRAM2_OVERALL_SIZE & 0x3FF);
    else
        return STATUS_FAIL;
}

/**
 * @brief   Clear Internal Ram Setting(except the overall size)
 * @param   iram_num: 1: iram 1, 2: iram 2
 * @return 	0: STATUS_OK;
 *         -1: STATUS_FAIL;
 */
INT32S drv_l1_iram_clr_set(INT32U iram_num)
{
    if(iram_num == 1)
        R_IRAM1_OVERALL_SIZE |= CLR_IRAM_SET;
    else if(iram_num == 2)
        R_IRAM2_OVERALL_SIZE |= CLR_IRAM_SET;
    else
        return STATUS_FAIL;

    return STATUS_OK;
}

/**
 * @brief   Set the starting offset of the specific module as shared internal ram from 0xF800_0000/0xFA00_0000 in kBytes.
 * @param   iram_num: 1: iram1; 2: iram2
 * @param   module: module to be shared as part of internal ram.
 * @param   offset_kB: Starting offset of specific module as shared internal ram. This value is an offset from 0xF800_0000/0xFA00_0000 in kByte
 *                     ex: if the actual address is 0xF801_0000, this value should be 64.
 * @return 	0: STATUS_OK;
 *         -1: STATUS_FAIL;
 */
INT32S drv_l1_iram_set_start_offset(INT32U iram_num, I_RAM_SHARED_MODULES module, INT32U offset_kB)
{
	INT8U mod = module;
	volatile INT32U *reg_base;

	if(iram_num == 1)
        reg_base = P_IRAM1_ROT_STR_ADDR;
    else if(iram_num == 2)
        reg_base = P_IRAM2_ROT_STR_ADDR;
    else
        return STATUS_FAIL;

	if(mod < IRAM_SHARED_ROT || mod > IRAM_SHARED_MODULE_MAX || mod == DUMMY1 || mod == DUMMY2)
		return STATUS_FAIL;

	if(offset_kB > MAX_IRAM_SIZE)
		return STATUS_FAIL;

	mod -= 1;
	*(reg_base + mod) = offset_kB;

	return STATUS_OK;
}

/**
 * @brief   Get starting offset of the specific module as shared internal ram from 0xF800_0000 in kBytes.
 * @param   iram_num: 1:iram1; 2: iram2
 * @param   module: module to be shared as part of internal ram.
 * @return 	-1: STATUS_FAIL.
 *          others: Starting offset of the specific module as shared internal ram from 0xF800_0000 in kBytes.
 */
INT32S drv_l1_iram_get_start_offset(INT32U iram_num, I_RAM_SHARED_MODULES module)
{
	INT8U mod = module;
	INT32U offset_kB;

	volatile INT32U *reg_base;

	if(iram_num == 1)
        reg_base = P_IRAM1_ROT_STR_ADDR;
    else if(iram_num == 2)
        reg_base = P_IRAM2_ROT_STR_ADDR;
    else
        return STATUS_FAIL;

	if(mod < IRAM_SHARED_ROT || mod > IRAM_SHARED_MODULE_MAX || mod == DUMMY1 || mod == DUMMY2)
		return STATUS_FAIL;

	mod -= 1;
	offset_kB = *(reg_base + mod);

	return offset_kB;
}

/**
 * @brief   Iram shared enable, the selected internal ram cannot be shared to another internal ram.
 * @param   iram_num: 1: iram1; 2: iram2
 * @return 	0: STATUS_OK, -1: STATUS_FAIL
 */
INT32S drv_l1_iram_shared_enable(INT32U iram_num)
{
	if(iram_num == 1){
        R_SYSTEM_SRAM_SEL &= (~BIT14);  /*disable share sram1 to sram2*/
	}else if(iram_num == 2){
        R_SYSTEM_SRAM_SEL &= (~BIT13);  /*disable share sram2 to sram1*/
	}else
        return STATUS_FAIL;

    return STATUS_OK;
}

/**
 * @brief   Enable specific shared ram as a part of internal ram
 * @param   iram_num: 1:iram1; 2: iram2
 * @param   module: specific shared ram module
 * @return 	0: STATUS_OK, -1: STATUS_FAIL
 */
INT32S drv_l1_iram_shared_module_en(INT32U iram_num, I_RAM_SHARED_MODULES module)
{
	INT32S ret = STATUS_OK;

	if(!iram_num || iram_num >2)
        return STATUS_FAIL;
	switch(module)
	{
		case IRAM_SHARED_ROT:
            if(iram_num == 1)
                R_SYSTEM_SRAM_SEL|=BIT0;
            else
                R_SYSTEM_SRAM_SEL&=(~BIT0);
            R_SYSTEM_MISC_CTRL8 |= BIT0;
			break;

		case IRAM_SHARED_ETHER:
            if(iram_num == 1)
                R_SYSTEM_SRAM_SEL|=BIT8;
            else
                R_SYSTEM_SRAM_SEL&=(~BIT8);
            R_SYSTEM_MISC_CTRL4 |= BIT7;
			break;

		case IRAM_SHARED_PPU:
            R_PPU_ENABLE &= (~0x01);		//PPU_EN = 0

            if(iram_num == 1)
                R_SYSTEM_SRAM_SEL|=BIT7;
            else
                R_SYSTEM_SRAM_SEL&=(~BIT7);

			break;


		case IRAM_SHARED_GPDLA:
            R_SYSTEM_MISC_CTRL8 |= (BIT1|BIT8);

            if(iram_num == 1)
                R_SYSTEM_SRAM_SEL|=BIT1;
            else
                R_SYSTEM_SRAM_SEL&=(~BIT1);
			break;

		case IRAM_SHARED_USBD:
            R_SYSTEM_MISC_CTRL8 |= BIT2;
            if(iram_num == 1)
                R_SYSTEM_SRAM_SEL|=BIT4;
            else
                R_SYSTEM_SRAM_SEL&=(~BIT4);
			break;

		case IRAM_SHARED_PSCA:
            *(PIPELINE_SCALERA_BASE+0) &= (~0x0001);
            if(iram_num == 1)
                R_SYSTEM_SRAM_SEL|=BIT2;
            else
                R_SYSTEM_SRAM_SEL&=(~BIT2);

			break;

		case IRAM_SHARED_PSCA_B:
			*(PIPELINE_SCALERB_BASE+0) &= (~0x0001);
            if(iram_num == 1)
                R_SYSTEM_SRAM_SEL|=BIT3;
            else
                R_SYSTEM_SRAM_SEL&=(~BIT3);
			break;

		case IRAM_SHARED_SCA:
            R_SYSTEM_MISC_CTRL8 |= BIT6;
            if(iram_num == 1)
                R_SYSTEM_SRAM_SEL|=BIT12;
            else
                R_SYSTEM_SRAM_SEL&=(~BIT12);
			break;

        case IRAM_SHARED_FFT:
            (*(volatile INT32U *)0xD0E00100) &= (~0x0001);

            if(iram_num == 1)
                R_SYSTEM_SRAM_SEL|=BIT6;
            else
                R_SYSTEM_SRAM_SEL&=(~BIT6);
            break;

        case IRAM_SHARED_H264:
            R_SYSTEM_MISC_CTRL8 |= BIT4;

            if(iram_num == 1)
                R_SYSTEM_SRAM_SEL|=BIT11;
            else
                R_SYSTEM_SRAM_SEL&=(~BIT11);
            break;

        case IRAM_SHARED_EXT:

            if(iram_num == 1){
                R_SYSTEM_SRAM_SEL &= (~BIT14);
                 R_SYSTEM_SRAM_SEL |= BIT13;
            }else{
                R_SYSTEM_SRAM_SEL &= (~BIT13);
                R_SYSTEM_SRAM_SEL |= BIT14;
            }
            break;

        case IRAM_SHARED_USBD_B:
            R_SYSTEM_MISC_CTRL8 |= BIT3;

            if(iram_num == 1)
                R_SYSTEM_SRAM_SEL|=BIT5;
            else
                R_SYSTEM_SRAM_SEL&=(~BIT5);

            break;

		default:
			ret = STATUS_FAIL;
			break;
	}

	return ret;
}

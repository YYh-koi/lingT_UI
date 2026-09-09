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
#include "drv_l2.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
static INT32U INT_FLAG;

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define __FUNCTION__		__func__

#ifndef DEBUG
#define DEBUG				DBG_PRINT
#else
#undef DEBUG
#define DEBUG(...)
#endif

#define MEMCPY				gp_memcpy
#define CLEAR(ptr, cblen)	gp_memset((INT8S *)ptr, 0x00, cblen)
//#define RETURN(x)			ret = x; goto __exit;
#define ABS(x)				((x) >= 0 ? (x) : -(x))
//#define gp_malloc(size)                 pvPortMalloc(size)
//#define gp_malloc_align(size, align)    gp_malloc(size)
//#define gp_free(ptr)                    vPortFree((void *)ptr);

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N                                          *
 **************************************************************************/
/**
 * @brief	csi set input parameter
 * @param	pFmt[in]: input parameter
 * @return	0: Success, -1: Fail
 */
INT32S drv_l2_csi_set_fmt(CSI_ID csi_num, gpCsiFmt_t *pFmt)
{
    INT32S ret = STATUS_OK;

    if(pFmt == 0)
        return -1;

    drv_l1_csi_fifo_mode_set(csi_num, pFmt->fifo_size);
    drv_l1_csi_preview_mode_set(csi_num, pFmt->preview_mode);
    drv_l1_csi_clock_priority_set(csi_num, pFmt->clk_pri);
    drv_l1_csi_clock_stop_set(csi_num, pFmt->clk_stopb);
    drv_l1_csi_cubic_output_set(csi_num, pFmt->cubicen, pFmt->cubicen_size);
	return ret;
}

/**
 * @brief	CSI stream on
 * @param   csi_info: gpCSIPara_t
 * @param	bufA[in]: buffer a
 * @param	bufB[in]: buffer b
 * @return	0: Success, -1: Fail
 */
INT32S drv_l2_csi_stream_on(CSI_ID csi_num, gpCSIPara_t *csi_info, INT32U bufA, INT32U bufB)
{
	INT32S ret = STATUS_OK;

    drv_l2_sensor_para_t *SensorPara;

    if(bufA == 0 || csi_info == 0)
        return -1;

    INT_FLAG = 0;
    drv_l1_csi_set_bufA(csi_num, bufA);
    drv_l1_csi_set_bufB(csi_num, bufA);
	drv_l1_csi_input_set(csi_num, csi_info->csi_fmt.interface_mode, csi_info->csi_fmt.interlace_mode, csi_info->csi_tim.clki_inv, csi_info->csi_fmt.input_format);
    drv_l1_csi_field_invert_set(csi_num, csi_info->csi_fmt.fieldinv);
    drv_l1_csi_input_latch_timing1_set(csi_num, csi_info->csi_tim.hl_start, csi_info->csi_tim.vl0_start, csi_info->csi_tim.vl1_start, csi_info->csi_tim.d_type);
    drv_l1_csi_input_latch_timing2_set(csi_num, csi_info->csi_tim.fget_mode, csi_info->csi_tim.vrst_mode, csi_info->csi_tim.vadd_mode, csi_info->csi_tim.hrst_mode);
    ret = drv_l1_csi_clko_invert_set(csi_num, csi_info->csi_tim.clko_inv);

    drv_l1_csi_blue_screen_set(csi_num, csi_info->csi_fmt.bs_mode, csi_info->bs_upper, csi_info->bs_lower);
    drv_l1_csi_black_screen_set(csi_num, csi_info->h_start, csi_info->h_end, csi_info->v_start, csi_info->v_end);
    drv_l1_csi_cut_screen_set(csi_num, csi_info->csi_fmt.cuten, csi_info->cut_start, csi_info->cut_size);
    drv_l1_csi_input_resolution_set(csi_num, csi_info->target_w, csi_info->target_h);
    if(csi_info->target_w & 0x1F)
        drv_l1_csi_long_burst_set(csi_num, 0);
    else
        drv_l1_csi_long_burst_set(csi_num, 1);

    drv_l1_csi_scale_down_set(csi_num, csi_info->hratio, csi_info->vratio);
    drv_l1_csi_output_data_format_set(csi_num, csi_info->csi_fmt.output_format);

    // Open CDSP data path
    SensorPara = drv_l2_sensor_get_para();
    if(SensorPara->pscaler_src_mode == MODE_PSCALER_SRC_CSI)
        drv_l1_csi_input_pscaler_set(csi_num, 1);

    if(SensorPara->md_mode)
    {
        if(SensorPara->md_mode & 0x80000000)
        {
        if(SensorPara->md_block_size == MD_8_SIZE)
            drv_l1_csi_md_set_parameter(csi_num, 1, csi_info->target_w, csi_info->target_h,0);
        else
            drv_l1_csi_md_set_parameter(csi_num, 1, csi_info->target_w, csi_info->target_h,1);
        }
        else
        {
            if(SensorPara->md_block_size == MD_8_SIZE)
                drv_l1_csi_md_set_new_parameter(csi_num, 1, csi_info->target_w, csi_info->target_h, 0);
            else if(SensorPara->md_block_size == MD_32_SIZE)
                drv_l1_csi_md_set_new_parameter(csi_num, 1, csi_info->target_w, csi_info->target_h, 2);
            else if(SensorPara->md_block_size == MD_64_SIZE)
                drv_l1_csi_md_set_new_parameter(csi_num, 1, csi_info->target_w, csi_info->target_h, 3);
            else
                drv_l1_csi_md_set_new_parameter(csi_num, 1, csi_info->target_w, csi_info->target_h, 1);
        }
        drv_l1_csi_md_set_buf(csi_num, SensorPara->md_buf);
        drv_l1_csi_md_set_threshold(csi_num, SensorPara->md_threshold);
        drv_l1_register_csi_cbk(csi_num, (CSI_CALLBACK_FUN)SensorPara->md_csi_callback);
        INT_FLAG |= MASK_CSI_MOTION_DET_FLAG;
    }

    drv_l1_csi_set_irq(csi_num, ENABLE, 0);
    drv_l1_csi_state_clear(csi_num, 0x3F);
    INT_FLAG |= MASK_CSI_FRAME_END_FLAG | 0x3F;
    drv_l1_csi_irq_enable(csi_num, ENABLE, INT_FLAG);
    drv_l1_csi_enable(csi_num, ENABLE);

	return ret;
}

/**
 * @brief	csi stream off
 * @param	none
 * @return	0: Success, -1: Fail
 */
INT32S drv_l2_csi_stream_off(CSI_ID csi_num)
{
    drv_l1_csi_irq_enable(csi_num, DISABLE, INT_FLAG);
    drv_l1_csi_set_irq(csi_num, DISABLE, 0);
    drv_l1_csi_state_clear(csi_num, 0x3F);
    drv_l1_csi_enable(csi_num, DISABLE);

	return 0;
}

/**
 * @brief	csi control off
 * @param	none
 * @return	0: Success, -1: Fail
 */
INT32S drv_l2_csi_stop(CSI_ID csi_num)
{
    drv_l1_csi_clockout_enable(csi_num, DISABLE);
    drv_l1_csi_enable(csi_num, DISABLE);

	return 0;
}


//debug
void gp_CSIDumpRegTable(CSI_ID CSI_Num)
{
	int	i;
	volatile INT32U *p_sram_hw;

	DBG_PRINT("\r\n");
	DBG_PRINT("%s\r\n", __FUNCTION__);

	if(CSI_Num == CSI_0)
	{
        //set bank 0
        R_CSI2_BANK_SEL &= ~(0x1<<0);
		p_sram_hw = (INT32U *) CSI_BASE;
	}
	else if(CSI_Num == CSI_1)
	{
        //set bank 1
        R_CSI2_BANK_SEL |= (0x1<<0);
        p_sram_hw = (INT32U *) CSI_BASE;
	}

	for(i = 0; i < 40; i++)
	{
		//DBG_PRINT("R0x%8x: 0x%8x\r\n", p_sram_hw, *p_sram_hw);
		DBG_PRINT("vu(0x%8x) = 0x%08x;\r\n",p_sram_hw, *p_sram_hw);
		p_sram_hw++;
	}
	vTaskDelay(100);
}


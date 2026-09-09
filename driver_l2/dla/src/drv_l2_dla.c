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

/*******************************************************
    Include file
*******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <arm_neon.h>
#include "drv_l1_sfr.h"
#include "drv_l1_dla.h"
#include "drv_l1_arm_cp15.h"
#include "drv_l1_scaler.h"
#include "drv_l2_scaler.h"
#include "drv_l2_dla.h"
#include "gpdla_cfg.h"
#include "gpdla_wt.h"
//#include "gplib_object_detect_callback.h"
#include "gplib_print_string.h"
#include "gplib_mm_gplus.h"
#include "gp_stdlib.h"

#define GPDLA_TO_PSCALER_ENABLE             1
#if GPDLA_TO_PSCALER_ENABLE == 1
#include "drv_l1_pscaler.h"
#endif
/******************************************************
    Definitions and variable declaration
*******************************************************/
#define MAX_DLA_MODULE_EXIT                 0xFFFFFFFF

/**********************************************
*	Extern variables and functions
**********************************************/

/*********************************************
*	Variables declaration
*********************************************/
static const INT32U* DLA_MODULE_POOL[]={
#if _GPLIB_FD_EN_ == 1
 	(INT32U *)GP_NN_Model_NEW(MV2_FD_DETECTION),
#endif

#if _GPLIB_HAND_EN_ == 1
 	(INT32U *)GP_NN_Model_NEW(MV2_HAND_DETECTION),
#endif

#if _GPLIB_FL_EN_ == 1
 	(INT32U *)GP_NN_Model_NEW(FL),
#endif

#if _GPLIB_BODY_EN_ == 1
 	(INT32U *)GP_NN_Model_NEW(YOLO_PERSON),
#endif

#if _GPLIB_CF_EN_ == 1
 	(INT32U *)GP_NN_Model_NEW(CF),
#endif

#if _GPLIB_SG_FULL_EN_ == 1
    (INT32U *)GP_NN_Model_NEW(SG_FULL),
#endif

#if _GPLIB_SG_HALF_EN_ == 1
    (INT32U *)GP_NN_Model_NEW(SG_HALF),
#endif

#if _GPLIB_POSE_EN_ == 1
    (INT32U *)GP_NN_Model_NEW(POSE),
#endif
#if _GPLIB_YOLO_LITE_EN_ == 1
    (INT32U *)GP_NN_Model_NEW(YOLO_LITE),
#endif
#if _GPLIB_MNIST_EN_ == 1
    (INT32U *)GP_NN_Model_NEW(MNIST),
#endif

#if _GPLIB_AUTOSYS_BSD_EN_ == 1
    (INT32U *)GP_NN_Model_NEW(BSD),
#endif

#if _GPLIB_GP_BSD_EN_ == 1
	(INT32U *)GP_NN_Model_NEW(BSD),
#endif

#if _GPLIB_LDW_EN_ == 1
		(INT32U *)GP_NN_Model_NEW(LDW),
#endif

 	(INT32U *)MAX_DLA_MODULE_EXIT
};

static void *bypass_module = 0;
static INT32U drvl2_box_cfg_ptr = 0;
static INT32U drvl2_box_wt_ptr = 0;
static INT32U gpdla_saturation_flag;

#if BYPASS_LAYER == 1
Bypass_Info *bypass_lib_info = 0;
BYPASS_AUTO_INFO dla_bypass_info = {0};
#endif

#if GPDLA_TO_PSCALER_ENABLE == 1
#define GPDLA_TO_PSCALER_FLOW_ENABLE             1
#define GPDLA_TO_PSCALER_FLOW_END                0
typedef struct
{
    INT32U pscaler_number[PSCALER_NUM];
    INT32U pscaler_feame_end[PSCALER_NUM];
} PSCALER_AUTO_INFO;
PSCALER_AUTO_INFO pscaler_info = {{0}};
#endif

/*****************************************************
    Utility functions
******************************************************/
static INT32S drv_l2_dla_bypass_info_get(void)
{
#if BYPASS_LAYER == 1
    INT32U i;

    if(bypass_module == 0)
        return 0;
    bypass_lib_info = (Bypass_Info *)bypass_module;

    dla_bypass_info.bypass_layers = bypass_lib_info->bypass_num;
    for(i=0;i<dla_bypass_info.bypass_layers;i++)
    {
        dla_bypass_info.bypass_info[i].layer_index = bypass_lib_info->info[i].layer_index;
        dla_bypass_info.bypass_info[i].next_input = bypass_lib_info->info[i].next_input;
        dla_bypass_info.bypass_info[i].offset = bypass_lib_info->info[i].offset;
    }

    return (INT32S)&dla_bypass_info;
#else
    return 0;
#endif
}

#if GPDLA_TO_PSCALER_ENABLE == 1
#define PSCALER_BUSY_START              0x80000000
#define PSCALER_DONE                    0x19921215
static xQueueHandle process_task_state_queue = NULL;

/**************************************************************************
 *                              Queue Get/Post                            *
 **************************************************************************/
static INT32U process_queue_buf_get(xQueueHandle queue, INT32U wait)
{
    osEvent result;

    if(wait == 1)
        result = osMessageGet(queue, osWaitForever);
    else if(wait)
        result = osMessageGet(queue, wait);
    else
        result = osMessageGet(queue, 1);

    if(result.status != osEventMessage)
    {
    #if 0
        return PRCESS_DUMMY_ADDR;
    #else
        return 0;
    #endif
    }
    else
        return result.value.v;
}

static INT32S process_queue_buf_put(xQueueHandle queue, INT32U buf, INT32U wait)
{
    if(wait == 1)
        osMessagePut(queue, (INT32U)&buf, osWaitForever);
    else if(wait)
        osMessagePut(queue, (INT32U)&buf, wait);
    else
        osMessagePut(queue, (INT32U)&buf, 1);

    return 0;
}

static void PScaler_Callback(INT32U PScaler_Event)
{
    if((PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE)
    ||(PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE)
    ||(PScaler_Event & PIPELINE_SCALER_STATUS_FRAME_DONE))
    {
        INT32U i;
        PSCALER_AUTO_INFO *pscaler_info_ptr = (PSCALER_AUTO_INFO *)&pscaler_info;

        for(i=0;i<PSCALER_NUM;i++)
        {
            if(pscaler_info_ptr->pscaler_number[i])
            {
                if(process_task_state_queue)
                    process_queue_buf_put(process_task_state_queue, PSCALER_DONE, 0);
                pscaler_info_ptr->pscaler_feame_end[i] = GPDLA_TO_PSCALER_FLOW_END;
                break;
            }
        }
    }
}
#endif

static INT32S gpdla_to_pscaler_start(PS_ID pScalerNum, INT32U in_hszie, INT32U in_vsize, INT32U in_color, INT32U out_hszie, INT32U out_vszie, INT32U out_color, INT32U out_buffer)
{
#if GPDLA_TO_PSCALER_ENABLE == 1
	INT32U widthFactor, heightFactor;
    PSCALER_AUTO_INFO *pscaler_info_ptr = (PSCALER_AUTO_INFO *)&pscaler_info;

    if(pscaler_info_ptr->pscaler_feame_end[pScalerNum] != GPDLA_TO_PSCALER_FLOW_END)
        return -1;

	drv_l1_pscaler_clk_ctrl(pScalerNum, 1);
	drv_l1_pscaler_init(pScalerNum);

	widthFactor = ((in_hszie*65536)/out_hszie);
	heightFactor = ((in_vsize*65536)/out_vszie);

	drv_l1_pscaler_integration_mode_set(pScalerNum, 0, 0);
	drv_l1_pscaler_drop_frame_set(pScalerNum, 0, 0);

	drv_l1_pscaler_input_pixels_set(pScalerNum, in_hszie, in_vsize);
	if(pScalerNum == PSCALER_Y)
        drv_l1_pscaler_input_source_set(pScalerNum, PIPELINE_SCALER_Y_INPUT_SOURCE_GPDLA);
	else
        drv_l1_pscaler_input_source_set(pScalerNum, PIPELINE_SCALER_AB_INPUT_SOURCE_GPDLA);
	drv_l1_pscaler_input_buffer_set(pScalerNum, 0);

	drv_l1_pscaler_input_format_set(pScalerNum, in_color);

	//+++ Frame Mode: FIFO Line = Output Height
	drv_l1_pscaler_output_fifo_line_set(pScalerNum, out_vszie, 0);
	drv_l1_pscaler_output_pixels_set(pScalerNum, widthFactor, out_hszie, heightFactor, out_vszie);

    drv_l1_pscaler_output_A_buffer_set(pScalerNum, out_buffer);
    drv_l1_pscaler_output_B_buffer_set(pScalerNum, out_buffer);

    drv_l1_pscaler_output_format_set(pScalerNum, out_color);

    drv_l1_pscaler_gpdla_if_enable(pScalerNum, 1);

	drv_l1_pscaler_H_first_enable(pScalerNum, 0);
	drv_l1_pscaler_PIP_window_set(pScalerNum, 0, 0);
	drv_l1_pscaler_outedge_set(pScalerNum, OUTEDGE_TYPE_DISABLE, OUTEDGE_CLP_64, OUTEDGE_COR_2, 8);
	drv_l1_pscaler_outboundary_color_set(pScalerNum, 0, 0);
	drv_l1_pscaler_outfill_set(pScalerNum, 0, 0, 0, 0, 0);

	drv_l1_pscaler_interrupt_set(pScalerNum, PIPELINE_SCALER_INT_ENABLE_ALL);
	drv_l1_pscaler_callback_register(pScalerNum, PScaler_Callback);
	pscaler_info_ptr->pscaler_feame_end[pScalerNum] = PSCALER_BUSY_START;

	drv_l1_pscaler_start(pScalerNum);

	return 0;
#else
    return -1;
#endif
}

static INT32S gpdla_to_pscaler_stop(PS_ID pScalerNum, INT32U wait)
{
#if GPDLA_TO_PSCALER_ENABLE == 1
    #define QUEUE_MAX_CNT       100
    INT32U i,error;
    PSCALER_AUTO_INFO *pscaler_info_ptr = (PSCALER_AUTO_INFO *)&pscaler_info;

    error = 1;
    if(process_task_state_queue)
    {
        INT32U time;

        if(wait > QUEUE_MAX_CNT)
            time = QUEUE_MAX_CNT;
        else
            time = wait;
        error = process_queue_buf_get(process_task_state_queue, time);
        pscaler_info_ptr->pscaler_number[pScalerNum] = GPDLA_TO_PSCALER_FLOW_END;
        pscaler_info_ptr->pscaler_feame_end[pScalerNum] = GPDLA_TO_PSCALER_FLOW_END;
        if(error == PSCALER_DONE)
            error = 0;
        else
            error = 1;
    }
    else
    {
        for(i=0;i<wait;i++)
        {
            if(pscaler_info_ptr->pscaler_feame_end[pScalerNum] == GPDLA_TO_PSCALER_FLOW_END)
            {
                pscaler_info_ptr->pscaler_number[pScalerNum] = GPDLA_TO_PSCALER_FLOW_END;
                error = 0;
                break;
            }
        }
    }

    drv_l1_pscaler_stop(pScalerNum);

    if(error)
    {
        pscaler_info_ptr->pscaler_feame_end[pScalerNum] = GPDLA_TO_PSCALER_FLOW_END;
        pscaler_info_ptr->pscaler_number[pScalerNum] = GPDLA_TO_PSCALER_FLOW_END;
        DBG_PRINT("gpdla_to_pscaler[%d]_stop error !!! \r\n", pScalerNum);

        return -1;
    }
    else
        return 0;
#else
    return -1;
#endif
}

INT32U drv_l2_dla_bypass_module_set(void *module)
{
    bypass_module = module;

    return (INT32U)bypass_module;
}

INT32U drv_l2_dla_module_max_input_size_get(void)
{
    INT32U name,max_size,state,src_max_size;
    const GP_NN_Model_NEW *object_model;

    name = 0;
    max_size = 0;
    drv_l1_dla_bypass_info_set(drv_l2_dla_bypass_info_get);

    while(1)
    {
        state = (INT32U)DLA_MODULE_POOL[name];
        if((state == MAX_DLA_MODULE_EXIT) || (state == 0))
            break;

        object_model = (GP_NN_Model_NEW *)DLA_MODULE_POOL[name];

        if(max_size)
        {
            src_max_size = (object_model->buffer_size*object_model->buffer_blocks);
            if(src_max_size > max_size)
                max_size = src_max_size;
        }
        else
            max_size = (object_model->buffer_size*object_model->buffer_blocks);
        name++;
    }

    return max_size;
}

INT32U drv_l2_dla_module_max_output_size_get(void)
{
    INT32U i,name,max_size,state,src_max_size;
    const GP_NN_Model_NEW *object_model;
    INT32U c_align;
    INT32U output_layer = 0;
    INT32U cf_enable = 0;
#if _GPLIB_CF_EN_ == 1
    const GP_NN_Model_NEW *cartoon_face_model;
    cartoon_face_model = GP_NN_Model_NEW(CF);
#endif

    name = 0;
    max_size = 0;
    while(1)
    {
        state = (INT32U)DLA_MODULE_POOL[name];
        if((state == MAX_DLA_MODULE_EXIT) || (state == 0))
            break;

        object_model = (GP_NN_Model_NEW *)DLA_MODULE_POOL[name];
        output_layer = object_model->output_layers;
        src_max_size = 0;
        for( i=0;i<output_layer;i++)
        {
            c_align = object_model->output_info[i].out_c;
            c_align = (c_align % 4 == 0) ? c_align : (c_align + 4 - (c_align % 4));
            src_max_size += object_model->output_info[i].out_w*object_model->output_info[i].out_h*c_align;
        }

        if(max_size)
        {
            if(src_max_size > max_size)
            {
                max_size = src_max_size;
            #if _GPLIB_CF_EN_ == 1
                if(cf_enable)
                {
                    if(cartoon_face_model != object_model)
                        cf_enable = 0;
                }
                else
                {
                    if(cartoon_face_model == object_model)
                        cf_enable = 1;
                }
            #endif
            }
        }
        else
        {
            max_size = src_max_size;
        #if _GPLIB_CF_EN_ == 1
            if(cartoon_face_model == object_model)
                cf_enable = 1;
        #endif
        }
        name++;
    }

    if(cf_enable)
        return max_size;
    else
    return (max_size*2);
}

static INT32U dla_boxfilter_max_output_size_get(INT32U img_max_hsize, INT32U img_max_vsize)
{
    return (img_max_hsize*img_max_vsize);
}

INT32U drv_l2_dla_module_and_boxfilter_max_output_size_get(INT32U img_max_hsize, INT32U img_max_vsize)
{
    INT32U size,size1,size2;

    size = drv_l2_dla_module_max_output_size_get();
    size1 = size*sizeof(float);
    size = dla_boxfilter_max_output_size_get(img_max_hsize, img_max_vsize);
    size2 = size*sizeof(INT32U);

    if(size1 > size2)
        size = (size1/sizeof(float));
    else
        size = (size2/sizeof(INT32U));

    return size;
}

static void insert_data_32_in_8(unsigned char *ptr_in, unsigned int dat)
{
    unsigned char dat_8;
    unsigned char *ptr;
    int i;

    ptr = (unsigned char *)ptr_in;
    for(i=0;i<4;i++)
    {
        dat_8 = dat;
        *ptr = dat_8;
        dat = dat >> 8;
        ptr++;
    }
}

static INT32S QuantizeMultiplier(float scale, INT32S *int_scale)
{
    #define NEW_MODE_ENABLE     1
    float M_value;
    INT32S shift;
    INT32S q_fixed;
#if NEW_MODE_ENABLE == 1
    double round_temp = 2147483648.0f;
#endif

    M_value = (float)frexp(scale, &shift);

#if NEW_MODE_ENABLE == 1
    round_temp = (double)(M_value*round_temp);
    q_fixed = (INT32S)round_temp;
#else
    q_fixed = round(M_value*(1<<31));
#endif

    if(q_fixed == (1 << 31))
    {
        q_fixed /= 2;
        shift+=1;
    }
    else if((q_fixed & (1 << 31)))
    {
        q_fixed &= ~(1 << 31);
    }

    if(shift < -31)
    {
        shift = 0;
        q_fixed = 0;
    }
    *int_scale = q_fixed;

    return shift;
}

static void xor_nbyte_neon(char *in, unsigned int len)
{
    int32_t i,loop;
    int32x4_t v_in, v_xval, v_xres;
    int32_t xval[] = {0x80808080,0x80808080,0x80808080,0x80808080};

    v_xval = vld1q_s32(xval);

    loop = (len >> 4);
    for (i=0;i< loop ;i++) {
        v_in = vld1q_s32((int32_t*)in);
        v_xres = veorq_s32(v_in, v_xval);
        vst1q_s32((int32_t*)in, v_xres);
        in += 16;
    }
}

static INT32U sobel_cfg_init(BOX_FILTER_CFG *box_info)
{
    INT8U *cfg_ptr;
    int8_t dat_8_s;
    INT32U i,dat,buffer_size;
    INT32S int_scale,shift;
    float scale = 0.5;

    if(drvl2_box_wt_ptr == 0)
    {
        buffer_size = sizeof(drv_l2_gpdla_A7_box_wt);
        drvl2_box_wt_ptr = (INT32U)gp_malloc_align(buffer_size, 64);
        if(drvl2_box_wt_ptr == 0)
        {
            DBG_PRINT("box_filter_cfg_init malloc error !!! \r\n");

            return 0;
        }
        gp_memcpy((INT8S *)drvl2_box_wt_ptr, (INT8S *)drv_l2_gpdla_A7_box_wt, buffer_size);
    }

    // Modify weighting
    cfg_ptr = (INT8U *)drvl2_box_wt_ptr;
    cfg_ptr += 40;
    for(i=0; i<4; i++){ // x0y0
        *cfg_ptr = box_info->x0y0;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x1y0
        *cfg_ptr = box_info->x1y0;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x2y0
        *cfg_ptr = box_info->x2y0;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x0y1
        *cfg_ptr = box_info->x0y1;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x1y1
        *cfg_ptr = box_info->x1y1;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x2y1
        *cfg_ptr = box_info->x2y1;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x0y2
        *cfg_ptr = box_info->x0y2;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x1y2
        *cfg_ptr = box_info->x1y2;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x2y2
        *cfg_ptr = box_info->x2y2;
        cfg_ptr++;
    }

    shift = QuantizeMultiplier(scale,(INT32S *)&int_scale);

    // Modify Scale
    for(i=0; i<4; i++){ // Bias
        dat = 0x00000000;
        insert_data_32_in_8(cfg_ptr, dat);
        cfg_ptr += 4;
    }
    for(i=0; i<4; i++){ // Scale
        dat = int_scale;
        insert_data_32_in_8(cfg_ptr, dat);
        cfg_ptr += 4;
    }
    for(i=0; i<4; i++){ // Scale
        dat_8_s = shift;
        *cfg_ptr = (unsigned char)dat_8_s;
        cfg_ptr += 1;
    }

    cache_invalid_range((INT32U)drvl2_box_wt_ptr, sizeof(drv_l2_gpdla_A7_box_wt));

    return drvl2_box_wt_ptr;
}


static INT32U box_filter_cfg_init(BOX_FILTER_CFG *box_info)
{
    INT8U *cfg_ptr;
    int8_t dat_8_s;
    INT32U i,dat,buffer_size;
    INT32S int_scale,shift;
    float scale,filter_sum;

    if(drvl2_box_wt_ptr == 0)
    {
        buffer_size = sizeof(drv_l2_gpdla_A7_box_wt);
        drvl2_box_wt_ptr = (INT32U)gp_malloc_align(buffer_size, 64);
        if(drvl2_box_wt_ptr == 0)
        {
            DBG_PRINT("box_filter_cfg_init malloc error !!! \r\n");

            return 0;
        }
        gp_memcpy((INT8S *)drvl2_box_wt_ptr, (INT8S *)drv_l2_gpdla_A7_box_wt, buffer_size);
    }

    // Modify weighting
    cfg_ptr = (INT8U *)drvl2_box_wt_ptr;
    cfg_ptr += 40;
    for(i=0; i<4; i++){ // x0y0
        *cfg_ptr = box_info->x0y0;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x1y0
        *cfg_ptr = box_info->x1y0;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x2y0
        *cfg_ptr = box_info->x2y0;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x0y1
        *cfg_ptr = box_info->x0y1;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x1y1
        *cfg_ptr = box_info->x1y1;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x2y1
        *cfg_ptr = box_info->x2y1;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x0y2
        *cfg_ptr = box_info->x0y2;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x1y2
        *cfg_ptr = box_info->x1y2;
        cfg_ptr++;
    }
    for(i=0; i<4; i++){ // x2y2
        *cfg_ptr = box_info->x2y2;
        cfg_ptr++;
    }

    filter_sum = (box_info->x0y0+box_info->x1y0+box_info->x2y0+box_info->x0y1+box_info->x1y1+box_info->x2y1+box_info->x0y2+box_info->x1y2+box_info->x2y2);
    if(filter_sum == 0)
    {
        DBG_PRINT("filter_sum error !!! \r\n");

        return -1;
    }

    scale = (1/filter_sum);
    shift = QuantizeMultiplier(scale,(INT32S *)&int_scale);

    // Modify Scale
    for(i=0; i<4; i++){ // Bias
        dat = 0x00000000;
        insert_data_32_in_8(cfg_ptr, dat);
        cfg_ptr += 4;
    }
    for(i=0; i<4; i++){ // Scale
        dat = int_scale;
        insert_data_32_in_8(cfg_ptr, dat);
        cfg_ptr += 4;
    }
    for(i=0; i<4; i++){ // Scale
        dat_8_s = shift;
        *cfg_ptr = (unsigned char)dat_8_s;
        cfg_ptr += 1;
    }

    cache_invalid_range((INT32U)drvl2_box_wt_ptr, sizeof(drv_l2_gpdla_A7_box_wt));

    return drvl2_box_wt_ptr;
}

INT32S drv_l2_dla_box_filter_start(BOX_FILTER_INFO *box_info)
{
    #define WAIT_CNT       500
    INT8U *cfg_ptr,*gpdla_cfg;
    INT32S ret;
    INT32U *src_ptr,dat,buffer_size,pscaler_set;
    INT32U width,height,error,box_wt_ptr,xor_result_flag;
    INT32U gpdla_output_ptr,gpdla_output_mode = 0;
    PSCALER_AUTO_INFO *pscaler_info_ptr = (PSCALER_AUTO_INFO *)&pscaler_info;

    if(box_info->image_output_ptr == 0)
    {
        DBG_PRINT("image_output_ptr error !!! \r\n");

        return -1;
    }

    if(box_info->pscaler_gpdla_num > PSCALER_Y)
        pscaler_set = PSCALER_Y;
    else
        pscaler_set = box_info->pscaler_gpdla_num;

    if(process_task_state_queue == NULL)
	{
        osMessageQDef_t disp_q = {9, sizeof(INT32U), 0};

        process_task_state_queue = osMessageCreate(&disp_q, NULL);
		if(!process_task_state_queue)
		{
            DBG_PRINT("process_task_state_queue error\r\n");
            while(1);
		}
		else
            DBG_PRINT("process_task_state_queue = 0x%x\r\n", process_task_state_queue);
	}

    gpdla_saturation_flag = 1;
    width = box_info->image_width;
    height = box_info->image_height;

    if(gpdla_output_mode)
    {
        buffer_size = (width*height*4);
        gpdla_output_ptr = (INT32U)gp_malloc_align(buffer_size, 64);
        if(gpdla_output_ptr == 0)
        {
            DBG_PRINT("gpdla_output_ptr malloc error !!! \r\n");
        }
    }
    else
        gpdla_output_ptr = 0;

    if(drvl2_box_cfg_ptr == 0)
    {
        buffer_size = sizeof(drv_l2_gpdla_A7_box_cfg);
        drvl2_box_cfg_ptr = (INT32U)gp_malloc_align(buffer_size, 64);
        if(drvl2_box_cfg_ptr == 0)
        {
            DBG_PRINT("drvl2_box_cfg_ptr malloc error !!! \r\n");

            return -1;
        }
        gp_memcpy((INT8S *)drvl2_box_cfg_ptr, (INT8S *)drv_l2_gpdla_A7_box_cfg, buffer_size);
    }

    if(box_info->sobel_cfg_enable)
    {
        xor_result_flag = 0;
        gpdla_saturation_flag = 0;
    }
    else
        xor_result_flag = 1;

    if(box_info->filter_cfg_enable)
    {
        box_wt_ptr = box_filter_cfg_init((BOX_FILTER_CFG *)&box_info->box_info);
        if(box_wt_ptr == 0)
        {
            DBG_PRINT("box_filter_cfg_init error !!! \r\n");

            return -1;
        }
    }
    else if(box_info->sobel_cfg_enable)
    {
        box_wt_ptr = sobel_cfg_init((BOX_FILTER_CFG *)&box_info->box_info);
        if(box_wt_ptr == 0)
        {
            DBG_PRINT("sobel_cfg_init error !!! \r\n");

            return -1;
        }
        box_info->get_sobel_scale = 2;
    }
    else
        box_wt_ptr = drvl2_box_wt_ptr;

    if(gpdla_output_ptr)
        gpdla_saturation_flag = 0;

    gpdla_cfg = (INT8U *)drvl2_box_cfg_ptr;

    // 0x000
    cfg_ptr = gpdla_cfg + 0x000;
    dat = (height<<16) + width;
    insert_data_32_in_8(cfg_ptr, dat);

    // 0x00c
    cfg_ptr = gpdla_cfg + 0x00c;
    dat = (height<<16) + width;
    insert_data_32_in_8(cfg_ptr, dat);

    // 0x010
    cfg_ptr = gpdla_cfg + 0x010;
    dat = (height<<16) + width;
    insert_data_32_in_8(cfg_ptr, dat);

    // 0x030
    cfg_ptr = (INT8U *)(gpdla_cfg + 0x030);
    src_ptr = (INT32U *)(gpdla_cfg + 0x030);
    dat = *src_ptr;
    dat |= DLA_IDMA_SN_ENABLE;
    insert_data_32_in_8(cfg_ptr, dat);

    // 0x058
    cfg_ptr = gpdla_cfg + 0x058;
    dat = (height<<16) + width;
    insert_data_32_in_8(cfg_ptr, dat);

    // 0x068
    cfg_ptr = gpdla_cfg + 0x068;
    dat = (height<<16) + width;
    insert_data_32_in_8(cfg_ptr, dat);

    if(gpdla_saturation_flag)
    {
    // 0x088
    cfg_ptr = (INT8U *)(gpdla_cfg + 0x088);
    src_ptr = (INT32U *)(gpdla_cfg + 0x088);
    dat = *src_ptr;
    #if 1
        dat &= ~MASK_SATURATION_MODE2;
        dat |= (0x80<<B_SATURATION_OFFSET2);
        dat |= (0x80<<B_SATURATION_OFFSET);
    #else
    dat &= ~MASK_SATURATION_MODE;
    dat |= (0x80<<B_SATURATION_OFFSET);
    #endif
    insert_data_32_in_8(cfg_ptr, dat);
    }
    else
    {
        // 0x088
        cfg_ptr = (INT8U *)(gpdla_cfg + 0x088);
        src_ptr = (INT32U *)(gpdla_cfg + 0x088);
        dat = *src_ptr;
        dat &= ~MASK_SATURATION_MODE2;
        insert_data_32_in_8(cfg_ptr, dat);
    }

    // 0x0bc
    cfg_ptr = (INT8U *)(gpdla_cfg + 0x0bc);
    src_ptr = (INT32U *)(gpdla_cfg + 0x0bc);
    dat = *src_ptr;
    dat |= DLA_DODMA_SN_ENABLE;
    if(gpdla_output_ptr)
    {
       dat &= ~0x7;
       dat |= 0x1;
    }
    insert_data_32_in_8(cfg_ptr, dat);

    if(gpdla_output_ptr)
    {
        // 0x0C4
        cfg_ptr = (INT8U *)(gpdla_cfg + 0x0C4);
        dat = (width * 4);
        insert_data_32_in_8(cfg_ptr, dat);

        // 0x0C8
        cfg_ptr = (INT8U *)(gpdla_cfg + 0x0C8);
        dat = (width * height * 4);
        insert_data_32_in_8(cfg_ptr, dat);
    }

    // 0x0cc
    cfg_ptr = gpdla_cfg + 0x0cc;
    dat = (height<<16) + width;
    insert_data_32_in_8(cfg_ptr, dat);

    // 0x0d0
    cfg_ptr = gpdla_cfg + 0x0d0;
    dat = width * height * 4;
    insert_data_32_in_8(cfg_ptr, dat);

    // 0x0e4
    cfg_ptr = gpdla_cfg + 0x0e4;
    dat = width * height * 1;
    insert_data_32_in_8(cfg_ptr, dat);

    // 0x0f4
    cfg_ptr = gpdla_cfg + 0x0f4;
    dat = (1<<16) + width;
    insert_data_32_in_8(cfg_ptr, dat);

    // 0x0f8
    cfg_ptr = gpdla_cfg + 0x0f8;
    dat = (1<<16) + width;
    insert_data_32_in_8(cfg_ptr, dat);

    // 0x0fc
    cfg_ptr = gpdla_cfg + 0x0fc;
    dat = (width<<16) + width*3;
    insert_data_32_in_8(cfg_ptr, dat);

    // 0x104
    cfg_ptr = gpdla_cfg + 0x104;
    dat = width * height * 4;
    insert_data_32_in_8(cfg_ptr, dat);

    // 0x108
    cfg_ptr = gpdla_cfg + 0x108;
    dat = (height<<16) + width;
    insert_data_32_in_8(cfg_ptr, dat);

    // 0x114
    cfg_ptr = gpdla_cfg + 0x114;
    dat = width * height;
    insert_data_32_in_8(cfg_ptr, dat);

    cache_invalid_range((INT32U)drvl2_box_cfg_ptr, sizeof(drv_l2_gpdla_A7_box_cfg));

    if((box_wt_ptr == 0)||(drvl2_box_cfg_ptr == 0))
    {
        DBG_PRINT("box_wt_ptr or drvl2_box_cfg_ptr error !!! \r\n");

        return -1;
    }

    error = 0;
    if(gpdla_output_ptr)
    {
        ret = drv_l1_dla_box_filter_start_dcache_flow_enable((INT32U)box_info->image_input_ptr, (INT32U)gpdla_output_ptr, (INT32U)drvl2_box_cfg_ptr, (INT32U)box_wt_ptr, (width*height), gpdla_saturation_flag);
        if(ret < 0)
            error++;
        gplib_frame_scaler_start_crop((INT32U)gpdla_output_ptr, IMG_FMT_UYVY, (INT32U)box_info->image_output_ptr, IMG_FMT_GRAY,
        (width*2), height, width, height, 0, 11);
    }
    else
    {
        gpdla_saturation_flag = 0;
        pscaler_info_ptr->pscaler_number[pscaler_set] = GPDLA_TO_PSCALER_FLOW_ENABLE;
        ret = gpdla_to_pscaler_start(pscaler_set, width, height, PIPELINE_SCALER_INPUT_FORMAT_Y, width, height, PIPELINE_SCALER_OUTPUT_FORMAT_YONLY, box_info->image_output_ptr);
        if(ret < 0)
            error++;
        ret = drv_l1_dla_box_filter_start_dcache_flow_enable((INT32U)box_info->image_input_ptr, (INT32U)box_info->image_output_ptr, (INT32U)drvl2_box_cfg_ptr, (INT32U)box_wt_ptr, (width*height), gpdla_saturation_flag);
        if(ret < 0)
            error++;
        ret = gpdla_to_pscaler_stop(pscaler_set, WAIT_CNT);
        if(ret < 0)
            error++;

        if((gpdla_saturation_flag == 0)&&(error == 0)&&(xor_result_flag))
        {
            INT32U i;

            if((width%16) == 0)
                xor_nbyte_neon((char *)box_info->image_output_ptr, (width*height));
            else if((width%4) == 0)
            {
                INT32S *src_32 = (INT32S *)box_info->image_output_ptr;

                for(i=0;i<((width*height)/4);i++)
                {
                    src_32[i] ^= 0x80808080;
                }
            }
            else
            {
                INT8S *src_8 = (INT8S *)box_info->image_output_ptr;

                for(i=0;i<(width*height);i++)
                {
                            src_8[i] ^= 0x80;
                }
            }
            cache_invalid_range((INT32U)box_info->image_output_ptr, (width*height));
        }
    }

    if(gpdla_output_ptr)
    {
        gp_free((void *)gpdla_output_ptr);
        gpdla_output_ptr = 0;
    }

    if(error)
    {
        DBG_PRINT("drv_l2_dla_box_filter_start error !!! \r\n");

        return -1;
    }
    else
        return 0;
}

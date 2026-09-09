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

#ifndef __drv_l1_BOXFILTER_H__
#define __drv_l1_BOXFILTER_H__

#include "typedef.h"
#include "cmsis_os.h"
#include "drv_l1_lbp.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

// control register
#define C_BOXFILTER_START			            0x00000001
#define C_BOXFILTER_INT_END			            C_BOXFILTER_START
#define C_BOXFILTER_INT_EN			            C_BOXFILTER_START
#define	C_MASK_IMG_RANGE			            0xFFF

#define C_FILTER_OUT_MASK                      (0x3<<6)

#define C_MASK_COLOR_MODE                       0x3


#define C_BOX_CH_MASK                           0x3

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct
{
    INT32U boxfilter_bufi_addr;         //0x0000
    INT32U boxfilter_bufoa_addr;        //0x0004
    INT32U boxfilter_bufob_addr;        //0x0008
    INT32U boxfilter_img_width;         //0x000C
    INT32U boxfilter_img_height;        //0x0010
    INT32U boxfilter_coef1;             //0x0014
    INT32U boxfilter_coef2;             //0x0018
    INT32U boxfilter_coef3;             //0x001C
    INT32U boxfilter_ctrl;              //0x0020
    INT32U boxfilter_start;             //0x0024
    INT32U boxfilter_irq_status;        //0x0028
    INT32U boxfilter_irq_en;            //0x002C
}*BOXFILTER_SFR;

typedef enum
{
    C_IMG_SCAN_LINE0,
    C_IMG_SCAN_LINE1,
    C_IMG_SCAN_LINE2,
    C_IMG_DDR
}BOXFTR_CH;

typedef struct
{
    INT8S   coef_a;
    INT8S   coef_b;
    INT8S   coef_c;
    INT8S   coef_d;
    INT8S   coef_e;
    INT8S   coef_f;
    INT8S   coef_g;
    INT8S   coef_h;
    INT8S   coef_i;
    INT8S   coef_j;
}BOXFILTER_COEF;

typedef enum
{
    C_BOXFTR_OUT_ABS = 0,
    C_BOXFTR_OUT,
    C_BOXFTR_BYPASS
}BOXFILTER_OUT_MODE;


/**************************************************************************
 *                            E X T E R N A L                             *
 **************************************************************************/

extern osSemaphoreId boxfilter_hw_sem;

/**
 * @brief   gray format transfer to other color depend on sor_img->format
 * @param   sor_img: output image information
 * @param   g_img: gray image information
 * @return 	none
 */
extern void drv_l1_yuv_generator(gpImage* sor_img, gpImage* g_img);

/**
 * @brief   tranfer image to gray image
 * @param   img: source image information
 * @param   tag_out: output buffer
 * @return 	SUCCESS
 */
extern INT32S drv_l1_get_y_only(gpImage* img, INT8U* tag_out);

/*****************************************************
*	Deiver Level 1 BOXFILTER INIT:
*	@brief box filter initial
*   @param  fiq_en = 0: IRQ; 1:FIQ
*   @return 0: success; -1: vic_num fail
*****************************************************/
extern INT32S drv_l1_boxfilter_init(INT32U vic_num);

/*****************************************************
*	Deiver Level 1 BOXFILTER UNINIT:
*	@brief box filter uninitial
*   @param  fiq_en = 0: IRQ; 1:FIQ
*   @return 0: success; -1: vic_num fail
*****************************************************/
extern INT32S drv_l1_boxfilter_uninit(INT32U vic_num);

/*****************************************************
*	Deiver Level 1 BOXFILTER Source Image Info:
*	@brief Set box filter input source information
*   @param  color: input color format;w: input x width;h: input y width;in_buf: input buffer address
*   @return 0:success; -1: fail
*****************************************************/
extern INT32S drv_l1_boxfilter_src_img_info(INT8S color, INT16S w, INT16S h, INT8U* in_buf, BOXFILTER_OUT_MODE out_mode);

/*****************************************************
*	Deiver Level 1 BOXFILTER Output B buffer Image Info:
*   and Set continue mode
*	@brief Set continue mode and B buffer address
*   @param  outb_buf: DRAM address; enable = 1==>enable continue mode
*   @return 0:success; -1: fail
*****************************************************/
extern INT32S drv_l1_boxftr_continue_set(INT32U outb_buf, INT8U enable);

//extern INT32S drv_l1_boxfilter_tar_img_addr(INT8U* out_buf);
/*****************************************************
*	Deiver Level 1 BOXFILTER Coef set:
*	@brief Set box filter coefficient
*   @param  coef: coefficient address point
*   @return 0:success; -1:fail
*****************************************************/
extern INT32S drv_l1_boxfilter_coef_set(INT8S* coef);

/*****************************************************
*	Deiver Level 1 BOXFILTER Input CH:
*	@brief Set box filter input channel
*   @param  ch: reference BOXFTR_CH
*   @return NONE
*****************************************************/
extern void drv_l1_boxfilter_input_ch(BOXFTR_CH ch);

/*****************************************************
*	Deiver Level 1 BOXFILTER OUTPUT FORMAT:
*	@brief Set box filter output format
*   @param  out_mode: reference BOXFILTER_OUT_MODE
*   @return NONE
*****************************************************/
extern void drv_l1_boxfilter_output_mode(BOXFILTER_OUT_MODE mode);

/*****************************************************
*	Deiver Level 1 BOXFILTER SET:
*	@brief  box filter register initial
*   @param  img: input source information; coef: coefficient; out_mode: Output mode selsction
*   @return NONE
*****************************************************/
extern void drv_l1_boxftr_reg_init(gpImage* img, INT8U* out_buf, INT8S* coef, BOXFILTER_OUT_MODE mode);

/*****************************************************
*	Deiver Level 1 BOXFILTER Start:
*	@brief  box filter start work
*   @param  NONE
*   @return NONE
*****************************************************/
extern void drv_l1_boxfilter_start(void);

/*****************************************************
*	Deiver Level 1 BOXFILTER END wait:
*	@brief wait box filter done
*   @param 1: wait; 0: no wait
*   @return NONE
*****************************************************/
extern INT32S drv_l1_boxfilter_end_wait(INT32U wait);

#endif // __drv_l1_BOXFILTER_H__

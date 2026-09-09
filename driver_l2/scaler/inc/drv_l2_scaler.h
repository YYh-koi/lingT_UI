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
#ifndef __drv_l2_SCALER_H__
#define __drv_l2_SCALER_H__

#include "define.h"

// pscaler y number
#define PSCALER_Y_FD_EN                     1
typedef enum
{
    PSCALER_FD_Y = 0x10,
} FD_PSCALER_NUM;

// scaler mode
#define C_SCALER_FULL_SCREEN				1
#define C_SCALER_BY_RATIO					2
#define C_SCALER_FULL_SCREEN_BY_RATIO		3
#define C_SCALER_FULL_SCREEN_BY_DIGI_ZOOM	4
#define C_SCALER_RATIO_USER					5
#define C_SCALER_RATIO_FD					6
#define C_SCALER_RATIO_CV					7

// scale error
#define C_SCALER_START_ERR					(-1)
#define C_SCALER_INPUT_SIZE_ERR				(-2)
#define C_SCALER_INPUT_OFFSET_ERR			(-3)
#define C_SCALER_INPUT_BUF_ERR				(-4)
#define C_SCALER_INPUT_FMT_ERR				(-5)
#define C_SCALER_INPUT_FIFO_ERR				(-6)
#define C_SCALER_OUTPUT_SIZE_ERR			(-7)
#define C_SCALER_OUTPUT_OFFSET_ERR			(-8)
#define C_SCALER_OUTPUT_BUF_ERR				(-9)
#define C_SCALER_OUTPUT_FMT_ERR				(-10)
#define C_SCALER_OUTPUT_FIFO_ERR			(-11)
#define C_SCALER_EXT_BUF_ERR				(-12)
#define C_SCALER_BND_COR_ERR				(-13)
#define C_SCALER_INTEGRATION_ERR			(-14)

// cv mode
#define CV_IN_360P_TYPE                     0
#define CV_IN_720P_TYPE                     1

typedef struct ScalerFormat_s
{
	/* img input para */
	INT32U input_format;			/* input format*/
	INT16U input_width;				/* 1~0x1FFF, input image x size */
	INT16U input_height;			/* 1~0x1FFF, input image y size */
	INT16U input_visible_width;		/* 0~0x1FFF, 0 is disable, clip x size*/
	INT16U input_visible_height;	/* 0~0x1FFF, 0 is disable, clip y size */
	INT32U input_x_offset;			/* 0~0x1FFF, x start offset in effective area */
	INT32U input_y_offset;			/* 0~0x1FFF, y start offset in effective area */

	INT32U input_y_addr;			/* input y addr, must be 4-align */
	INT32U input_u_addr;			/* input u addr, must be 4-align */
	INT32U input_v_addr;			/* input v addr, must be 4-align */

	INT32U input_b_y_addr;			/* input b y addr, must be 4-align */
	INT32U input_b_u_addr;			/* input b u addr, must be 4-align */
	INT32U input_b_v_addr;			/* input b v addr, must be 4-align */

	INT32U output_format; 			/* output format*/
	INT16U output_width; 			/* 1~0x1FFF, must be 8-align, but YUV444/YUV422/YUV420 is 16-align, YUV411 is 32-align */
	INT16U output_height;			/* 1~0x1FFF */
	INT16U output_buf_width;		/* 1~0x1FFF, must be 8-align, but YUV444/YUV422/YUV420 is 16-align, YUV411 is 32-align */
	INT16U output_buf_height;		/* 1~0x1FFF */
	INT16U output_x_offset;			/* 0~0x1FFF, must be 8-align, skip x size after every line output */
	INT16U reserved0;

	INT32U fd_stepout_width;        /* prcess output real width for fd ratio */
	INT32U output_y_addr;			/* output y addr, must be 4-align */
	INT32U output_u_addr;			/* output u addr, must be 4-align */
	INT32U output_v_addr;			/* output v addr, must be 4-align */

	/* scale para */
	INT32U fifo_mode;				/* FIFO in or FIFO out mode */
	INT32U force_intp_en;           /* 0:Disable, 1:Enable */
	INT8U  scale_mode;				/* C_SCALER_FULL_SCREEN / C_SCALER_FIT_RATIO.... */
	INT8U  digizoom_m;				/* digital zoom, ratio =  m/n */
	INT8U  digizoom_n;
	INT8U  scale_status;			/* scale status */
	void (*callback)(INT32U scaler0_event, INT32U scaler1_event);   /* scaler callback function */
	INT32U (*new_int_callback)(void);                               /* scaler intergration callback function */
} ScalerFormat_t;

typedef struct ScalerPara_s
{
	INT8U boundary_mode;		/* 0:Use boundary data, 1:Use register define data */
	INT8U gamma_en;				/* 0:Disable, 1:Enable */
	INT8U color_matrix_en;		/* 0:Disable, 1:Enable */
    INT8U scaler_intmode;       /* 0:Integration Mode0, 1:Integration Mode1, 3:Integration Mode3 */

	INT32U cv_mode;                 /* Algorithm CV Process Use */
	INT32U cv_csi_size[2];          /* Algorithm CV Process Use */
	INT32U yuv_type;				/* C_SCALE_CTRL_TYPE_YUV / C_SCALE_CTRL_TYPE_YCBCR */
	INT32U boundary_color;			/* format is YcbCr, 0x008080:Block */
	INT8U gamma_table[256];
	COLOR_MATRIX_CFG matrix;		/* 0x000:0, 0x080:0.5, 0x380:-0.5, 0x100:1,0, 0x300:-1.0 */
} ScalerPara_t;

extern void drv_l2_scaler_init(void);
extern INT32S drv_l2_FD_scaler_clip(INT8U scale_dev, INT8U wait_done, gpImage *src, gpImage *dst, gpRect *clip);
extern INT32S drv_l2_CV_scaler_clip(INT8U scale_dev, INT8U wait_done, gpImage *src, gpImage *dst, gpRect *clip, INT32U csi_h, INT32U csi_v, INT32U cv_mode);
extern INT32S drv_l2_scaler_clip(INT8U scale_dev, INT8U wait_done, gpImage *src, gpImage *dst, gpRect *clip);
extern INT32S drv_l2_scaler_once(INT8U scale_dev, INT8U wait_done, gpImage *src, gpImage *dst);
extern INT32S drv_l2_scaler_full_screen(INT8U scale_dev, INT8U wait_done, gpImage *src, gpImage *dst);
extern INT32S drv_l2_scaler_wait_done(INT8U scale_dev, ScalerFormat_t *pScale);
extern void drv_l2_scaler_stop(INT8U scale_dev);
extern INT32S drv_l2_scaler_trigger(INT8U scale_dev, INT8U wait_done, ScalerFormat_t *pScale_dev, ScalerPara_t *pPara);
extern INT32S drv_l2_scaler_retrigger(INT8U scale_dev, ScalerFormat_t *pScale);
#endif

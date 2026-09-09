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
#include "drv_l2_nn_model.h"

#ifndef _GPLIB_FD_EN_
#define _GPLIB_FD_EN_                   0
#endif
#if _GPLIB_FD_EN_ == 1
#include "face_nn_kernel.h"
#endif


#ifndef _GPLIB_HAND_EN_
#define _GPLIB_HAND_EN_                   0
#endif
#if _GPLIB_HAND_EN_ == 1
#include "MV2_HAND_DETECTION_nn_kernel.h"
#endif


#ifndef _GPLIB_FL_EN_
#define _GPLIB_FL_EN_                   0
#endif
#if _GPLIB_FL_EN_ == 1
#include "FL_nn_kernel.h"
#endif

#ifndef _GPLIB_BODY_EN_
#define _GPLIB_BODY_EN_                 0
#endif
#if _GPLIB_BODY_EN_ == 1
#include "body_nn_kernel.h"
#endif

#ifndef _GPLIB_CF_EN_
#define _GPLIB_CF_EN_                 0
#endif
#if _GPLIB_CF_EN_ == 1
#include "CF_nn_kernel.h"
#endif

#ifndef _GPLIB_SG_FULL_EN_
#define _GPLIB_SG_FULL_EN_                 0
#endif
#if _GPLIB_SG_FULL_EN_ == 1
#include "SG_FULL_nn_kernel.h"
#endif

#ifndef _GPLIB_SG_HALF_EN_
#define _GPLIB_SG_HALF_EN_                 0
#endif
#if _GPLIB_SG_HALF_EN_ == 1
#include "SG_HALF_nn_kernel.h"
#endif

#ifndef _GPLIB_POSE_EN_
#define _GPLIB_POSE_EN_                 0
#endif
#if _GPLIB_POSE_EN_ == 1
#include "POSE_nn_kernel.h"
#endif

#if _GPLIB_YOLO_LITE_EN_ == 1
#include "YOLO_LITE_nn_kernel.h"
#endif

#if _GPLIB_MNIST_EN_ == 1
#include "MNIST_nn_kernel.h"
#endif

#ifndef _GPLIB_AUTOSYS_BSD_EN_
#define _GPLIB_AUTOSYS_BSD_EN_                0
#endif
#if _GPLIB_AUTOSYS_BSD_EN_ == 1
#include "BSD_nn_kernel.h"
#endif

#ifndef _GPLIB_GP_BSD_EN_
#define _GPLIB_GP_BSD_EN_                0
#endif
#if _GPLIB_GP_BSD_EN_ == 1
#include "BSD_nn_kernel.h"
#endif

#ifndef _GPLIB_LDW_EN_
#define _GPLIB_LDW_EN_                0
#endif
#if _GPLIB_LDW_EN_ == 1
#include "LDW_nn_kernel.h"
#endif


typedef struct {
	INT8S x0y0;
    INT8S x1y0;
    INT8S x2y0;
    INT8S x0y1;
    INT8S x1y1;
    INT8S x2y1;
    INT8S x0y2;
    INT8S x1y2;
    INT8S x2y2;
} BOX_FILTER_CFG;

typedef struct {
    INT32U image_width;
    INT32U image_height;
    INT32U image_input_ptr;
    INT32U image_output_ptr;
    INT32U pscaler_gpdla_num;
    INT32U filter_cfg_enable;
    INT32U sobel_cfg_enable;
    float get_sobel_scale;
    BOX_FILTER_CFG box_info;
} BOX_FILTER_INFO;

// GPDLA memory size mode
extern INT32U drv_l2_dla_module_max_input_size_get(void);
extern INT32U drv_l2_dla_module_max_output_size_get(void);
extern INT32U drv_l2_dla_module_and_boxfilter_max_output_size_get(INT32U img_max_hsize, INT32U img_max_vsize);

// GPDLA bypass mode
extern INT32U drv_l2_dla_bypass_module_set(void *module);

// GPDLA box_filter mode
extern INT32S drv_l2_dla_box_filter_start(BOX_FILTER_INFO *box_info);

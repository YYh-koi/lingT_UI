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
#ifndef __DEFINE_H__
#define __DEFINE_H__

#include "project.h"

#define IMG_FMT_GRAY	        0
#define IMG_FMT_YUYV	        1
#define IMG_FMT_YCbYCr	        2
#define IMG_FMT_RGB		        3
#define IMG_FMT_UYVY	        4
#define IMG_FMT_CbYCrY	        5
#define IMG_FMT_INTEGRAL_Y      6
#define IMG_FMT_YUV444          7
#define IMG_FMT_GP420           8
#define IMG_FMT_RGB565		    9
#define IMG_FMT_ARGB		    10

#if !defined(MAX)
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#endif
#if !defined(MIN)
#define MIN(x, y) ((x) <  (y) ? (x) : (y))
#endif
#define ABS(x)	  ((x) >= 0 ? (x) : -(x))
// Clamp Macro
#define Clamp(x,min,max) (((x) > (max)) ? (max) : (((x) < (min)) ? (min) : (x)))

typedef signed char q7_t;
typedef float float32_t;

typedef struct
{
    INT16S x;
    INT16S y;
    INT16S width;
    INT16S height;
    float MPD_conf;
    float classify_dist;
    int classify_label;
} nnResult;

typedef struct
{
    INT16S x_start;
    INT16S y_start;
    INT16S x_center;
    INT16S y_center;
    INT16S width;
    INT16S height;
} pipelinePose;

typedef struct
{
	INT16S width;
	INT16S height;
    INT16S widthStep;
	INT8S ch;
	INT8S format;
	INT8U *ptr;
	INT8U *ptr_u;
	INT8U *ptr_v;
} gpImage;

typedef struct
{
    INT16S x;
    INT16S y;
    INT16S width;
    INT16S height;
} gpRect;

typedef struct
{
    INT16S width;
    INT16S height;
} gpSize;

typedef struct
{
	INT16S x;
	INT16S y;
} gpPoint;

typedef struct
{
	INT16S x_start;
	INT16S y_start;
	INT16S x_end;
	INT16S y_end;
} gpLine;

typedef struct {
	gpPoint	left_pt;
	gpPoint right_pt;
	gpPoint top_pt;
	gpPoint bottom_pt;
} gpFeaturePoint;

typedef struct {
	gpPoint	left_pt;
	gpPoint right_pt;
	gpPoint rtop_pt;
	gpPoint ltop_pt;
	gpPoint rbottom_pt;
	gpPoint lbottom_pt;
} gpFeaturePoint2;

typedef struct DataArr
{

    unsigned short width;
    unsigned short height;
    unsigned short step;
    unsigned int *i;

} DataArr;

#endif // __DEFINE_H__

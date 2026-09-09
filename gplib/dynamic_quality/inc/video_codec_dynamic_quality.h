#ifndef __VIDEO_CODEC_DYNAMIC_QUALITY_H__
#define __VIDEO_CODEC_DYNAMIC_QUALITY_H__
/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
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
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park,                      *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#include "drv_l2.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define Q_STEP	1

// 4K
#define Q_MAX_4K		50
#define Q_MIN_4K		35
#define Q_EMER_4K 		20

// 2K
#define Q_MAX_2K4K		70
#define Q_MIN_2K4K		40
#define Q_EMER_2K4K 	45

// 1080P/1080FHD
#define Q_MAX_1080P		70
#define Q_MIN_1080P		40
#define Q_EMER_1080P	45

// 720P/WVGA/VGA
#define Q_MAX_720P		80
#define Q_MIN_720P		40
#define Q_EMER_720P		45

// REAR
#define Q_MAX_REAR		70
#define Q_MIN_REAR		40
#define Q_EMER_REAR		45


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct JPEG_QUANTIZATION_TABLE_s // keep 16 multiples
{
	INT8U Y_q_table;
	INT8U UV_q_table;
	INT8U reserve[14];
}JPEG_Q_TABLE;

typedef struct JPEG_DYNAMIC_TUNE_Q_s // Dynamic tune Jpeg's quantization table
{
	JPEG_Q_TABLE max_q;		// max a table
	JPEG_Q_TABLE min_q;		// min q table
	JPEG_Q_TABLE emer_q;	// emergency q table
	INT32U enc_size_max;	// max encode size
	INT32U enc_size_mid;	// middle encode size
	INT32U enc_size_min;	// middle encode size
	INT8U  step_q;
} JengDynaQ_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void
vid_dyna_tune_Q_init(
	JengDynaQ_t* pJpeg_dyna_q,
	INT32U encSizMax,
	INT32U encSizMid,
	INT32U encSizMin,
	INT8U  emerY,
	INT8U  maxY,
	INT8U  minY,
	INT8U  step
);

extern void
vid_dyna_tune_Q_cal(
	JengDynaQ_t* args,
	INT32U encSize,
	INT32U* pRetYq
);

#endif //__VIDEO_CODEC_DYNAMIC_QUALITY_H__

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

#ifndef __GPLIB_JPEG_H__
#define __GPLIB_JPEG_H__

#define GPLIB_JPEG_ENCODE_EN            1
#define GPLIB_JPEG_DECODE_EN            1

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern INT32U gplib_jpeg_version_get(void);
extern void gplib_jpeg_default_quantization_table_load(JPEG_IDX idx, INT32U quality);
extern void default_huffman_dc_lum_table_load(JPEG_IDX idx);

// JPEG load default huffman table.
extern void gplib_jpeg_default_huffman_table_load(JPEG_IDX idx);
extern const INT8U zigzag_scan[64] ;

extern INT16U quant20_luminance_table[];
extern INT16U quant20_chrominance_table[];

extern INT16U quant25_luminance_table[];
extern INT16U quant25_chrominance_table[];

extern INT16U quant30_luminance_table[];
extern INT16U quant30_chrominance_table[];

extern INT16U quant40_luminance_table[];
extern INT16U quant40_chrominance_table[];

extern INT16U quant50_luminance_table[];
extern INT16U quant50_chrominance_table[];

extern INT16U quant65_luminance_table[];
extern INT16U quant65_chrominance_table[];

extern INT16U quant70_luminance_table[];
extern INT16U quant70_chrominance_table[];

extern INT16U quant80_luminance_table[];
extern INT16U quant80_chrominance_table[];

extern INT16U quant85_luminance_table[];
extern INT16U quant85_chrominance_table[];

extern INT16U quant90_luminance_table[];
extern INT16U quant90_chrominance_table[];

extern INT16U quant93_luminance_table[];
extern INT16U quant93_chrominance_table[];

extern INT16U quant95_luminance_table[];
extern INT16U quant95_chrominance_table[];

extern INT16U quant98_luminance_table[];
extern INT16U quant98_chrominance_table[];
#endif 		// __GPLIB_JPEG_H__

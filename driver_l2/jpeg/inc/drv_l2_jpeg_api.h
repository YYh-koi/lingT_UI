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

#ifndef __JPEG_API_H__
#define __JPEG_API_H__

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/
// yuv format
#define JPEG_YUV422				0
#define JPEG_YUV420				1

#define JPEG_SCALE_NONE			0
#define JPEG_SCALE_VX2			1
#define JPEG_SCALE_X2			2
#define JPEG_SCALE_GP420_X3D2	3
#define JPEG_SCALE_YUYV_X4		4
#define JPEG_SCALE_YUYV_X9D8	5

#define JPEG_IMG_FORMAT_422		0x21
#define JPEG_IMG_FORMAT_420		0x22

#define JPEG_HEADER_SIZE		624

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct JpegEnc_s
{
	INT32U hwidx;
	INT32U quality_value;
	struct {
		INT8U qY;
		INT8U qUV;
	} q_update;
	INT8U* header;
	INT32U yuv_fmt;
	INT16U yuv_sample_fmt;
	INT32U width;
	INT32U height;
	INT32U input_buffer_y;
	INT32U input_buffer_u;
	INT32U input_buffer_v;
	INT32U input_y_len;
	INT32U input_uv_len;
	INT32U output_buffer;
	INT32U output_buffer_len;
	INT32U scale_mode;

	INT32U jpeg_status;
	INT32U encode_size;
} JpegEnc_t;

typedef struct JpegDec_s
{
	INT32U hwidx;
	INT32U raw_data_addr;
	INT32U raw_data_size;

	INT32U scaler_mode;
	INT32U jpeg_fifo;
	INT32U scaler_fifo;
	INT32U fifo_line;

	INT32U output_format;
	INT32U output_w;
	INT32U output_h;
	INT32U output_buffer_w;
	INT32U output_buffer_h;
	INT32U output_addr;
	INT32U output_boundary_color;
	INT32U input_alpha_addr;

	// jpeg file read back infomation
	INT32U jpeg_yuv_mode;
	INT32U jpeg_valid_w;
	INT32U jpeg_valid_h;
	INT32U jpeg_extend_w;
	INT32U jpeg_extend_h;
} JpegDec_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern INT32S jpeg_api_decode_get_size(JPEG_IDX idx, INT32U raw_data_addr, INT32U size, INT16U *width, INT16U *height, INT16U *img_yuv_mode);
extern INT32S jpeg_api_decode_output(JpegDec_t *pJpegDec);

extern INT32S jpeg_api_encode_one_frame(JpegEnc_t *pJpegEnc);
extern INT32S jpeg_api_encode_fifo_start(JpegEnc_t *pJpegEnc);
extern INT32S jpeg_api_encode_fifo_once(JpegEnc_t *pJpegEnc);
extern INT32S jpeg_api_encode_fifo_stop(JpegEnc_t *pJpegEnc);

extern INT8U* jpeg_header_generate(INT32U yuv_fmt, INT32U quality, INT16U width, INT16U height, INT32U fifo_line);
extern void   jpeg_header_quality_set(JPEG_IDX idx, INT8U* header, INT8U Y_quality_value,INT8U UV_quality_value);

#endif 		// __JPEG_API_H__

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

#ifndef __DRV_L1_JPEG_H__
#define __DRV_L1_JPEG_H__

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/
// JFIF releate register
#define C_JPG_STANDARD_FORMAT			0x00000001

// JPEG image width and height registers
#define C_JPG_HSIZE_MAX					0x00001FFF		// Maximum 8191 pixels
#define C_JPG_VSIZE_MAX					0x00001FFF		// Maximum 8191 pixels

// JPEG read control register
#define C_JPG_READ_CTRL_EN				0x80000000
#define C_JPG_READ_ADDR_UPDATE			0x40000000
#define C_JPG_READ_ON_FLY_LEN_MAX		0x00FFFFFF

// VLC control register
#define C_JPG_VLC_TOTAL_LEN_MAX			0x03FFFFFF
#define C_JPG_ENCODE_ADDR_UPDATE		0x40000000

// Control register
#define C_JPG_CTRL_COMPRESS				0x00000001
#define C_JPG_CTRL_DECOMPRESS			0x00000002
#define C_JPG_CTRL_CLIPPING_EN			0x00000008
#define C_JPG_CTRL_YUV444				0x00000000
#define C_JPG_CTRL_YUV422				0x00000010
#define C_JPG_CTRL_YUV420				0x00000020
#define C_JPG_CTRL_YUV411				0x00000030
#define C_JPG_CTRL_GRAYSCALE			0x00000040
#define C_JPG_CTRL_YUV422V				0x00000050
#define C_JPG_CTRL_GP420				0x00000080
#define C_JPG_CTRL_MODE_MASK			0x00000070
#define C_JPG_CTRL_YCBCR_FORMAT			0x00001000
#define C_JPG_CTRL_SCALEN				0x00010000
#define C_JPG_CTRL_UNION_MODE_EN		0x04000000
#define C_JPG_CTRL_YUYV_DECODE_EN		0x00002000
#define C_JPG_CTRL_YUYV_ENCODE_EN		0x00004000
#define C_JPG_CTRL_UNION_DIV2_EN		0x08000000
#define C_JPG_CTRL_UNION_DIV4_EN		0x10000000
#define C_JPG_CTRL_YUV411V				0x00000060
#define C_JPG_CTRL_YUV420H2				0x00000120
#define C_JPG_CTRL_YUV420V2				0x00000220
#define C_JPG_CTRL_YUV411H2				0x00000130
#define C_JPG_CTRL_YUV411V2				0x00000260

// GP420 control register
#define C_JPG_GP420_WIDTH_MASK			0x0000FFFF		// When compressing, JPEG width after 3/2 scale-up. When decompressing, JPEG width after 2/3X scale-down
#define C_JPG_GP420_H_23X_SCALE_DOWN	0x00010000		// When decompressing to GP420 format, scale down output width to 2/3X
#define C_JPG_GP420_V_23X_SCALE_DOWN	0x00020000		// When decompressing to GP420 format, scale down output height to 2/3X

#define C_JPG_DEC_UNION_MASK			0x00300000
#define C_JPG_DEC_UNION_32_EN			0x00100000		// decomp union 32 bit enable: enable ARGB8888/AYUV8888 mode
#define C_JPG_DEC_UNION_AYUV_EN			0x00200000		// decomp_union_ayuv      : 1: AYUV8888, 0: ARGB8888
#define C_JPG_DEC_UNION_FIX_ALPHA_EN	0x00400000		// decomp_union_fix_alpha : 1: use [31:24] alpha value , 0: alpha from dram
#define C_JPG_DEC_UNION_ALPHA_MASK		0xFF000000		// decomp_union_alpha[7:0] : fix alpha value

#define C_JPG_DECOUT_FMT_YUYV			0x00000000
#define C_JPG_DECOUT_FMT_GP420			C_JPG_CTRL_GP420
#define C_JPG_DECOUT_FMT_ARGB8888		C_JPG_DEC_UNION_32_EN
#define C_JPG_DECOUT_FMT_AYUV8888		C_JPG_DEC_UNION_32_EN | C_JPG_DEC_UNION_AYUV_EN

// Reset register
#define C_JPG_RESET						0x00000003
#define C_JPG_STOP						0x00000010

// Ring FIFO control register
#define C_JPG_FIFO_DISABLE				0x00000000
#define C_JPG_FIFO_ENABLE				0x00000001
#define C_JPG_FIFO_16LINE				0x00000009
#define C_JPG_FIFO_32LINE				0x00000001
#define C_JPG_FIFO_64LINE				0x00000003
#define C_JPG_FIFO_128LINE				0x00000005
#define C_JPG_FIFO_256LINE				0x00000007
#define C_JPG_FIFO_LINE_MASK			0x0000000F

// Image size register
#define C_JPG_IMAGE_SIZE_MAX			0x0FFFFFFF

// Interrupt control register
#define C_JPG_INT_ENCODE_DONE			0x00000001
#define C_JPG_INT_DECODE_DONE			0x00000002
#define C_JPG_INT_DECODE_OUTPUT_FULL	0x00000004
#define C_JPG_INT_INPUT_EMPTY			0x00000008
#define C_JPG_INT_ENCODE_OUTPUT_FULL	0x00000040
#define C_JPG_INT_AUTO_OUTPUT_FULL		0x00000080

// Interrupt pending register
#define C_JPG_FLAG_ENCODE_DONE			0x00000001
#define C_JPG_FLAG_DECODE_DONE			0x00000002
#define C_JPG_FLAG_DECODE_OUTPUT_FULL	0x00000004
#define C_JPG_FLAG_INPUT_EMPTY			0x00000008
#define C_JPG_FLAG_RST_VLC_DONE			0x00000010
#define C_JPG_FLAG_RST_ERROR			0x00000020
#define C_JPG_FLAG_ENCODE_OUTPUT_FULL	0x00000040
#define C_JPG_FLAG_AUTO_OUPUT_FULL		0x00000080
#define C_JPG_FLAG_STOP					0x00000100

// Define JPEG engine status
#define C_JPG_STATUS_DECODING			0x00000001
#define C_JPG_STATUS_ENCODING			0x00000002
#define C_JPG_STATUS_INPUT_EMPTY		0x00000004
#define C_JPG_STATUS_OUTPUT_FULL		0x00000008
#define C_JPG_STATUS_DECODE_DONE		0x00000010
#define C_JPG_STATUS_ENCODE_DONE		0x00000020
#define C_JPG_STATUS_STOP				0x00000040
#define C_JPG_STATUS_TIMEOUT			0x00000080
#define C_JPG_STATUS_INIT_ERR			0x00000100
#define C_JPG_STATUS_RST_VLC_DONE		0x00000200
#define C_JPG_STATUS_RST_MARKER_ERR		0x00000400
#define C_JPG_STATUS_SCALER_DONE		0x00008000
#define C_JPG_STATUS_AUTO_OUTPUT_FULL	0x00010000

#define C_JPG_AUTO_FIFO_NUM_MASK		0x00FF0000
#define C_JPG_AUTO_FIFO_NUM_SHIFT		16
#define C_JPG_AUTO_RESTART_EN			0x00008000
#define C_JPG_AUTO_RESTART_EN_SHIFT		15
#define C_JPG_AUTO_RESTART_ADD_MASK		0x00003000
#define C_JPG_AUTO_RESTART_ADD_SHIFT	12
#define C_JPG_AUTO_RESTART_IDX_MASK		0x00000700
#define C_JPG_AUTO_RESTART_IDX_SHIFT	8

#define C_JPG_AUTO_ALIGN_EN				0x00000080
#define C_JPG_AUTO_ALIGN_SEL_MASK		0x00000060
#define C_JPG_AUTO_ALIGN_SEL_SHIFT		5
#define C_JPG_AUTO_ALIGN_BYTE_MASK		0x0000001F
#define C_JPG_AUTO_ALIGN_BYTE_SHIFT		0

#define C_JPG_AUTO_OUT_SEL_MASK			0x40000000
#define C_JPG_AUTO_OUT_SEL_SHIFT		30
#define C_JPG_AUTO_ABFIFO_EN			0x20000000
#define C_JPG_AUTO_CONT_MASK			0x18000000
#define C_JPG_AUTO_CONT_BW_OUT			0x10000000
#define C_JPG_AUTO_CONT_FRAME_OUT		0x18000000
#define C_JPG_AUTO_2USB_START1			0x04000000
#define C_JPG_AUTO_2USB_EN				0x02000000
#define C_JPG_AUTO_OUT_OFFSET_MASK		0x000003FF
#define C_JPG_AUTO_OUT_OFFSET_SHIFT		0
#define C_JPG_AUTO_OUT_NUM_MASK			0x01FF0000
#define C_JPG_AUTO_OUT_NUM_SHIFT		16
#define C_JPG_AUTO_INPUT_SEL_MASK		0x00000F00
#define C_JPG_AUTO_INPUT_SEL_SHIFT		8
#define C_JPG_AUTO_OUT_SW_EN			0x00000008
#define C_JPG_AUTO_DEC_EN				0x00000004
#define C_JPG_AUTO_OUT_EN				0x00000002
#define C_JPG_AUTO_INPUT_EN				0x00000001

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
	ENUM_JPG_NO_SCALE_UP = 0x0,				// No scale-up
	ENUM_JPG_SCALE_VX2,						// Vertical x2
	ENUM_JPG_SCALE_X2,						// X2
	ENUM_JPG_SCALE_X1_5,					// X1.5 (3/2 GP420)
	ENUM_JPG_SCALE_X4,						// X4 (YUYV)
	ENUM_JPG_SCALE_X9D8						// X9/8 (YUYV)
} JPEG_SCALE_UP_MODE_ENUM;

typedef enum
{
	ENUM_JPG_NO_SCALE_DOWN = 0x0,			// No scale-down
	ENUM_JPG_DIV2,							// Sampling mode=YUV422/YUV422V/YUV420/YUV444/YUV411. Output format=YUYV
	ENUM_JPG_DIV4,							// Sampling mode=YUV422/YUV422V/YUV420/YUV444/YUV411. Output format=YUYV
	ENUM_JPG_H_DIV23,						// Sampling mode=YUV420. Output format=GP420
	ENUM_JPG_V_DIV23,						// Sampling mode=YUV420. Output format=GP420
	ENUM_JPG_HV_DIV23						// Sampling mode=YUV420. Output format=GP420
} JPEG_SCALE_DOWN_MODE_ENUM;

typedef enum
{
	JPEGA = 0x0,
	JPEGB,
	JPEG_IDX_MAX
} JPEG_IDX;

typedef enum
{
	ENUM_PSA_OUTPUT0 = 0x0,
	ENUM_PSA_OUTPUT1,
	ENUM_PSA_BYPASS_OUTPUT0,
	ENUM_PSA_BYPASS_OUTPUT1,
	ENUM_PSB_OUTPUT0,
	ENUM_PSB_OUTPUT1,
	ENUM_PSB_BYPASS_OUTPUT0,
	ENUM_PSB_BYPASS_OUTPUT1,
	ENUM_ISP_OUTPUT0,
	ENUM_ISP_OUTPUT1,
	ENUM_JPGA_DECOUT,
	ENUM_JPGB_DECOUT,

} JPEG_AUTO_INPUT_SRC;

typedef enum
{
	ENUM_AUTO_OUT_USBA = 0x0,
	ENUM_AUTO_OUT_USBB,
	ENUM_AUTO_OUT_2_USB,
	ENUM_AUTO_OUT_SW
} JPEG_AUTO_OUTPUT_DST;

typedef enum
{
	ENUM_AUTO_MODE_SINGLE = 0x0,
	ENUM_AUTO_MODE_DUAL,
	ENUM_AUTO_MODE_DUAL_2ND
} JPEG_AUTO_MODE;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
// JPEG init API
extern void drv_l1_jpeg_init(JPEG_IDX idx, INT8U fiq_en);

// JPEG APIs for setting Huffman table
extern INT32S drv_l1_jpeg_huffman_dc_lum_mincode_set(JPEG_IDX idx, INT32U offset, INT32U len, INT16U *val);
extern INT32S drv_l1_jpeg_huffman_dc_lum_valptr_set(JPEG_IDX idx, INT32U offset, INT32U len, INT8U *val);
extern INT32S drv_l1_jpeg_huffman_dc_lum_huffval_set(JPEG_IDX idx, INT32U offset, INT32U len, INT8U *val);
extern INT32S drv_l1_jpeg_huffman_dc_chrom_mincode_set(JPEG_IDX idx, INT32U offset, INT32U len, INT16U *val);
extern INT32S drv_l1_jpeg_huffman_dc_chrom_valptr_set(JPEG_IDX idx, INT32U offset, INT32U len, INT8U *val);
extern INT32S drv_l1_jpeg_huffman_dc_chrom_huffval_set(JPEG_IDX idx, INT32U offset, INT32U len, INT8U *val);
extern INT32S drv_l1_jpeg_huffman_ac_lum_mincode_set(JPEG_IDX idx, INT32U offset, INT32U len, INT16U *val);
extern INT32S drv_l1_jpeg_huffman_ac_lum_valptr_set(JPEG_IDX idx, INT32U offset, INT32U len, INT8U *val);
extern INT32S drv_l1_jpeg_huffman_ac_lum_huffval_set(JPEG_IDX idx, INT32U offset, INT32U len, INT8U *val);
extern INT32S drv_l1_jpeg_huffman_ac_chrom_mincode_set(JPEG_IDX idx, INT32U offset, INT32U len, INT16U *val);
extern INT32S drv_l1_jpeg_huffman_ac_chrom_valptr_set(JPEG_IDX idx, INT32U offset, INT32U len, INT8U *val);
extern INT32S drv_11_jpeg_huffman_ac_chrom_huffval_set(JPEG_IDX idx, INT32U offset, INT32U len, INT8U *val);

// JPEG APIs for setting Quantization table
extern INT32S drv_l1_jpeg_quant_luminance_set(JPEG_IDX idx, INT32U offset, INT32U len, INT16U *val);
extern INT32S drv_l1_jpeg_quant_chrominance_set(JPEG_IDX idx, INT32U offset, INT32U len, INT16U *val);

// JPEG APIs for setting image relative parameters
extern INT32S drv_l1_jpeg_restart_interval_set(JPEG_IDX idx, INT16U interval);
extern INT32S drv_l1_jpeg_image_size_set(JPEG_IDX idx, INT16U hsize, INT16U vsize, INT32U mode);		// hsize<=8000, vsize<=8000, Decompression: hsize must be multiple of 16, vsize must be at least multiple of 8
extern INT32S drv_l1_jpeg_yuv_sampling_mode_set(JPEG_IDX idx, INT32U mode);				// mode=C_JPG_CTRL_YUV422/C_JPG_CTRL_YUV420/C_JPG_CTRL_YUV444/C_JPG_CTRL_YUV411/C_JPG_CTRL_GRAYSCALE/C_JPG_CTRL_YUV422V/C_JPG_CTRL_YUV411V/C_JPG_CTRL_YUV420H2/C_JPG_CTRL_YUV420V2/C_JPG_CTRL_YUV411H2/C_JPG_CTRL_YUV411V2
extern INT32S drv_l1_jpeg_clipping_mode_enable(JPEG_IDX idx);						// Decompression: When clipping mode is enabled, JPEG will output image data according to jpeg_image_clipping_range_set()
extern INT32S drv_l1_jpeg_clipping_mode_disable(JPEG_IDX idx);
extern INT32S drv_l1_jpeg_image_clipping_range_set(JPEG_IDX idx, INT16U start_x, INT16U start_y, INT16U width, INT16U height);	// x, y, width, height must be at lease 8-byte alignment
extern INT32S drv_l1_jpeg_decode_scale_down_set(JPEG_IDX idx, JPEG_SCALE_DOWN_MODE_ENUM mode);
extern INT32S drv_l1_jpeg_out_format_set(JPEG_IDX idx, INT32U format);

extern INT32S drv_l1_jpeg_using_union_mode_enable(JPEG_IDX idx);
extern INT32S drv_l1_jpeg_using_union_mode_disable(JPEG_IDX idx);

extern INT32S drv_l1_jpeg_progressive_mode_enable(JPEG_IDX idx);
extern INT32S drv_l1_jpeg_progressive_mode_disable(JPEG_IDX idx);

// JPEG APIs for setting Y/Cb/Cr or YUYV data
extern INT32S drv_l1_jpeg_yuv_addr_set(JPEG_IDX idx, INT32U y_addr, INT32U u_addr, INT32U v_addr);	// Compression: input addresses(8-byte alignment for normal compression, 16-bytes alignment for YUYV data compression); Decompression: output addresses(8-byte alignment)
extern INT32S drv_l1_jpeg_multi_yuv_input_init(JPEG_IDX idx, INT32U len);								// Compression: When YUYV mode is used, it should be the length of first YUYV data that will be compressed. It should be sum of Y,U and V data when YUV separate mode is used

// JPEG APIs for setting entropy encoded data address and length
extern INT32S drv_l1_jpeg_vlc_addr_set(JPEG_IDX idx, INT32U addr);						// Compression: output VLC address, addr must be 16-byte alignment. Decompression: input VLC address
extern INT32S drv_l1_jpeg_vlc_addr_B_set(JPEG_IDX idx, INT32U addr);
extern INT32S drv_l1_jpeg_vlc_maximum_length_set(JPEG_IDX idx, INT32U len);				// Decompression: JPEG engine stops when maximum VLC bytes are read(Maximum=0x03FFFFFF)
extern INT32S drv_l1_jpeg_multi_vlc_input_init(JPEG_IDX idx, INT32U len);					// Decompression: length(Maximum=0x000FFFFF) of first VLC buffer

// JPEG APIs for setting output FIFO line
extern INT32S drv_l1_jpeg_fifo_line_set(JPEG_IDX idx, INT32U line);						// Decompression: FIFO line number(C_JPG_FIFO_DISABLE/C_JPG_FIFO_ENABLE/C_JPG_FIFO_16LINE/C_JPG_FIFO_32LINE/C_JPG_FIFO_64LINE/C_JPG_FIFO_128LINE/C_JPG_FIFO_256LINE)

// JPEG start, restart and stop function APIs
extern INT32S drv_l1_jpeg_compression_start(JPEG_IDX idx, void (*callback)(JPEG_IDX idx, INT32S event, INT32U count));
extern INT32S drv_l1_jpeg_multi_yuv_input_restart(JPEG_IDX idx, INT32U y_addr, INT32U u_addr, INT32U v_addr, INT32U len);	// Compression: Second, third, ... YUV addresses(8-byte alignment for normal compression, 16-bytes alignment for YUYV data compression) and lengths(Maximum=0x000FFFFF)
extern INT32S drv_l1_jpeg_vlc_output_restart(JPEG_IDX idx, INT32U addr, INT32U len);

extern INT32S drv_l1_jpeg_decompression_start(JPEG_IDX idx, void (*callback)(JPEG_IDX idx, INT32S event, INT32U count));
extern INT32S drv_l1_jpeg_multi_vlc_input_restart(JPEG_IDX idx, INT32U addr, INT32U len);	// Decompression: Second, third, ... VLC addresses(16-byte alignment) and lengths(Maximum=0x000FFFFF)
extern INT32S drv_l1_jpeg_yuv_output_restart(JPEG_IDX idx);								// Decompression: Restart JPEG engine when FIFO is full
extern void drv_l1_jpeg_stop(JPEG_IDX idx);

// JPEG status polling API
extern void drv_l1_jpeg_device_protect(JPEG_IDX idx);
extern void drv_l1_jpeg_device_unprotect(JPEG_IDX idx);
extern INT32S drv_l1_jpeg_status_wait_idle(JPEG_IDX idx, INT32U wait);
extern INT32S drv_l1_jpeg_status_polling(JPEG_IDX idx, INT32U wait);						// If wait is 0, this function returns immediately. Return value:C_JPG_STATUS_INPUT_EMPTY/C_JPG_STATUS_OUTPUT_FULL/C_JPG_STATUS_DECODE_DONE/C_JPG_STATUS_ENCODE_DONE/C_JPG_STATUS_STOP/C_JPG_STATUS_TIMEOUT/C_JPG_STATUS_INIT_ERR
extern INT32U drv_l1_jpeg_compress_vlc_cnt_get(JPEG_IDX idx);						// This API returns the total bytes of VLC data stream that JPEG compression has been generated

extern INT32S drv_l1_jpeg_encode_scale_up_set(JPEG_IDX idx, JPEG_SCALE_UP_MODE_ENUM mode);
extern INT32S drv_l1_jpeg_level2_scaledown_mode_enable(JPEG_IDX idx);
extern INT32S drv_l1_jpeg_level2_scaledown_mode_disable(JPEG_IDX idx);

extern INT32S drv_l1_jpeg_decode_alpha_set(JPEG_IDX idx, INT8U fix_alpha_en, INT8U alpha_value);

extern INT32S drv_l1_jpeg_auto_restart_config(JPEG_IDX idx, INT8U enable, INT8U fifo_num, INT8U start_idx, INT8U step);
extern INT32S drv_l1_jpeg_auto_align_config(JPEG_IDX idx, INT8U enable, INT8U align, INT8U last_align_byte_mask);
extern INT32S drv_l1_jpeg_auto_fifo_input_config(JPEG_IDX idx, INT8U enable, JPEG_AUTO_INPUT_SRC src, JPEG_AUTO_MODE mode);
extern INT32S drv_l1_jpeg_auto_fifo_output_config(JPEG_IDX idx, INT8U enable, JPEG_AUTO_OUTPUT_DST dst, INT16U byte_offset, INT8U fifo_num, INT8U cont_frame);
extern INT32U drv_l1_jpeg_auto_get_sw_vlc_size(JPEG_IDX idx, INT16U buf_idx);
extern INT32S drv_l1_jpeg_auto_clear_sw_vlc_size(JPEG_IDX idx, INT16U buf_idx);

//+++ Add
extern void drv_l1_jpeg_isr_callback_set(JPEG_IDX idx, void (*callback)(JPEG_IDX idx, INT32S event, INT32U count));

#endif		// __DRV_L1_JPEG_H__

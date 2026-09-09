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
#ifndef __GPLIB_JPEG_ENCODE_H__
#define __GPLIB_JPEG_ENCODE_H__

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/
#define C_JPEG_FORMAT_YUV_SEPARATE		0x0
#define C_JPEG_FORMAT_YUYV				0x1

#define C_JPEG_HEADER_SIZE				640 //(624+16)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
//JPEG
typedef enum
{
    ENUM_JPEG_LUMINANCE_QTABLE = 0,
    ENUM_JPEG_CHROMINANCE_QTABLE
} JPEG_QTABLE_ENUM;

typedef struct JpegAutoModeCfg
{
	INT8U auto_mode_enable;			//enable auto mode: 0:disable 1:enable
	JPEG_AUTO_INPUT_SRC source;		//auto mode source
	JPEG_AUTO_OUTPUT_DST output;	//auto mode output
	JPEG_AUTO_MODE dual_mode;		//dual jpeg mode: 0:single 1:dual first 2: dual second
	INT8U auto_restart;				//auto restart enable (for jpeg fifo mode with restart marker)
	INT8U auto_restart_num;			//auto restart number (total fifo number)
	INT16U header_offset;			//jpeg header offset (byte offset of first fifo in auto restart mode)
	INT8U dbg_disable_continue_frame; //disable auto continue next frame for debug.
} JPEG_AUTOMODE_CFG;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void jpeg_header_quantization_table_calculate(JPEG_IDX idx, JPEG_QTABLE_ENUM table, INT32U quality, INT8U *qtable);
// JPEG encoder initiate API
extern void jpeg_encode_init(JPEG_IDX idx);

// JPEG encoder input buffer relative APIs
extern INT32S jpeg_encode_input_size_set(JPEG_IDX idx, INT16U width, INT16U height);			// Width and height of the image that will be compressed
extern INT32S jpeg_encode_input_format_set(JPEG_IDX idx, INT32U format);						// format: C_JPEG_FORMAT_YUV_SEPARATE or C_JPEG_FORMAT_YUYV

// JPEG encoder output buffer relative APIs
extern INT32S jpeg_encode_yuv_sampling_mode_set(JPEG_IDX idx, INT32U encode_mode);			// encode_mode: C_JPG_CTRL_YUV422 or C_JPG_CTRL_YUV420
extern INT32S jpeg_encode_output_addr_set(JPEG_IDX idx, INT32U addr);							// The address that VLC(variable length coded) data will be output
extern INT32S jpeg_encode_maximum_length_set(JPEG_IDX idx, INT32U len);						// Maximum VLC data length (26 bits), jpeg encode will fail if exceed this length.
// JPEG encoder start, restart and stop APIs
extern INT32S jpeg_encode_once_start(JPEG_IDX idx, INT32U y_addr, INT32U u_addr, INT32U v_addr);	// If input format is YUV separate, both y_addr, u_addr and v_addr must be 8-byte alignment. If input format is YUYV, only y_addr(16-byte alignment) is used.
extern INT32S jpeg_encode_on_the_fly_start(JPEG_IDX idx, INT32U y_addr, INT32U u_addr, INT32U v_addr, INT32U len);	// If input format is YUV separate, both y_addr, u_addr and v_addr must be 8-byte alignment. If input format is YUYV, only y_addr(16-byte alignment) is used.
extern void jpeg_encode_stop(JPEG_IDX idx);

// JPEG encoder status query API
extern INT32S jpeg_encode_status_query(JPEG_IDX idx, INT32U wait);
extern INT32U jpeg_encode_vlc_cnt_get(JPEG_IDX idx);

extern INT32S jpeg_encode_scale_up_set(JPEG_IDX idx, JPEG_SCALE_UP_MODE_ENUM mode);

extern INT32S jpeg_encode_automode_set(JPEG_IDX idx, JPEG_AUTOMODE_CFG cfg);
extern INT32U jpeg_encode_automode_vlc_cnt_get(JPEG_IDX idx, INT16U buf_idx);	//output vlc size get for automode with auto restart.
extern INT32S jpeg_encode_output_addr_B_set(JPEG_IDX idx, INT32U addr); 	//second vlc output address for automode.
extern INT32S jpeg_encode_auto_start(JPEG_IDX idx, INT32U input_len); //start automode encode.

/*== Turnkey ==*/
extern void jpeg_encode_isr_callback_set(JPEG_IDX idx, void (*callback)(JPEG_IDX idx, INT32S event, INT32U count));
extern INT32U jpeg_encode_start_get(JPEG_IDX idx);

#endif 		// __GPLIB_JPEG_ENCODE_H__

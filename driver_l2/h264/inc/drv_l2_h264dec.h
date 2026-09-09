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

#ifndef __drv_l2_H264DEC_H__
#define __drv_l2_H264DEC_H__

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/

#define H264Dec_OUT_PSCALER     0
#define H264Dec_OUT_PPU         1

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct {
  INT32U BitstreamAddr;
  INT32U BitstreamBaseAddr;
  INT32U BitstreamMaxAddr;
  INT8U  drop_frame;
} gp_h264dec_in;

/* configuration of H.264 decoder */
typedef struct {
    INT32U width;	/* image width */
    INT32U height;	/* image height */
    INT32U out_div; /* output image division, 2: (w/2)*(h/2) 4: (w/4)*(h/4) 0:orginal size (w*h) */
    INT8U* ext_buf; /* external buffer address to use as wokring buffer, set NULL to use internal driver allocated buffer. */
    INT8U  buf_num;	/* working buffer number, 1:1fb+1mb line, 2:2fb, 3: 3fb, 0: default 2 */
    INT8U  fbc;		/* frame buffer compression enable, default 0, when enabled, buffer number must be 2. */
} gp_h264dec_cfg;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**
 * @brief   get working buffers size of H.264 decoder instance
 * @param	cfg:	configuration of H.264 decoding, see gp_h264dec_cfg define.
 * @return  total size of working buffers, -1 if fail
 * @note	if user wants to use custom external buffer, call this function to get required size.
 */
int gp_h264dec_get_buffer_size(gp_h264dec_cfg* cfg);

/**
 * @brief   initialize H.264 decoder instance
 * @param	cfg:	configuration of H.264 decoding, see gp_h264dec_cfg define.
 * @return  instance handle, NULL for fail.
 */
void* gp_h264dec_init(gp_h264dec_cfg* cfg);

/**
 * @brief   deocde H.264 current frame and output last frame result, without wait finish.
 * @param	handle:	H.264 decoder handle
 * @param	in:		H.264 decode input parameters, see gp_h264dec_in define.
 * @return  0: Success -1: Fail
 * @note	Call this funciton to decode one H.264 frame, but the result image will not
 *			be outputted until decondig next frame. instead previous decoded frame image
 *			will be outputted by MB2SCAN. The "off by one frame" design is for improving
 *			hardware effciency and utilization in video decoding, especially for low speed
 *			system. If immediate result is required, don't use this funciton, call
 *			gp_h264dec_decode_one_frm() instead.
 *			call gp_h264dec_decode_wait() to make sure deocde is finished before decode
 *			next frame.
 */
int gp_h264dec_decode(void* handle, gp_h264dec_in* in);

/**
 * @brief   deocde one H.264 frame, and wait finish
 * @param	handle:	H.264 decoder handle
 * @param	in:		H.264 decode input parameters, see gp_h264dec_in define.
 * @param	frame_type:	return decoded frame type.
 * @param	time_out:	wait time out in millisecond
 * @param	len:		return decoded frame size in bytes;
 * @return  status:		see STATUS_H264_* defines
 * @note	Call this funciton to decode one H.264 frame, and wait MB2SCAN output
 *			result image. If decode one frame only, or when decoding a video stream
 *			in a higher speed system where hardware utilization is not an issue,
 *			it is preffred to call this function instead of calling gp_h264dec_decode().
 *			gp_h264dec_decode_wait() is not required.
 */
int gp_h264dec_decode_one_frm(void* handle, gp_h264dec_in* in, INT32U* frame_type, INT32U time_out, INT32S* len);

/**
 * @brief   wait deocde H.264 frame finish
 * @param	handle:	H.264 decoder handle
 * @param	frame_type:	return decoded frame type.
 * @param	time_out:	wait time out in millisecond
 * @param	len:		return decoded frame size in bytes;
 * @return  status:		see STATUS_H264_* defines
 * @note	call this function to make sure decode is finished after calling
 *			gp_h264dec_decode(). it is not required to call this function with
 *			gp_h264dec_decode_one_frm().
 */
int gp_h264dec_decode_wait(void* handle, INT32U* frame_type, INT32U time_out, INT32S* len);

/**
 * @brief   notify MB2SCAN to process remaining image.
 * @note	MB2SCAN in GPA7XXXA support maximum output width to 2047, for larger
 *			width, MB2SCAN need to run 2 passes to convert left half then right half
 *			image. After Pscaler finishing output left half, call this function to
 *			trigger MB2SCAN to convert other half.
 */
void gp_h264dec_mb2scan_notify();

/**
 * @brief   Release H.264 decoder instance.
 * @param	handle:	H.264 decoder handle.
 */
void gp_h264dec_release(void* handle);

/**
 * @brief   Reset H.264 decoder instance.
 * @param	handle:	H.264 decoder handle.
 * @note	This function reset H.264 engine and re-allocate internal working buffers.
 */
void gp_h264dec_reset(void* handle);

extern void H264_IRQHandler(void);
extern void MB2SCAN_IRQHandler(void);
#endif

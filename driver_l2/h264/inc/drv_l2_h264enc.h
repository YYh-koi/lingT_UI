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

#ifndef __drv_l2_H264ENC_H__
#define __drv_l2_H264ENC_H__

#include "ratecontrol.h"

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/

#define H264_SKIP_FRAME_MAX_SIZE    160

#define STATUS_H264_FRAME_END		0x1
#define STATUS_H264_FIFO_OUT_END	0x2
#define STATUS_H264_FIFO_IN_END		0x4
#define STATUS_H264_FBC_OVERFLOW	-5
#define STATUS_H264_FBC_DEC_ERR		-2
#define STATUS_H264_FIFO_OUT_ERR	-3

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/* rc parameter types allowed to be changed in runtime */
typedef enum
{
    RC_QP           = 1,
    RC_BITRATE      = 1 << 1,
    RC_GOPLEN       = 1 << 2,
    RC_FRAMERATE    = 1 << 3,
    RC_HIERPFRAMES  = 1 << 4,
    RC_RATEFACTOR   = 1 << 5,
} RC_PARAM_TYPES;

/* h264 encoder configuration */
typedef struct {
	int width;			/* encode width, at least 2-alignment */
	int height;			/* encode height, at least 2-alignment */
	int level;			/* h264 level, see H264 document */
	/* 	crop value, when width or height is not 16 align, set crop to make sure actual image is 16 alignment.
		unit of each crop value is 2 pixels. (eg. for crop 8 pixels, should set crop value to 4.
		when leave these valuse as 0, driver would caculat proper crop value for user. */
	int crop_left;
	int crop_right;
	int crop_top;
	int crop_bottom;
	int fifo_mode;
	int buf_num;		/* internal buffer number: 0 or 1: 1 buffer + 1 mb line, 2: 2 buffers */
	int fbc;			/* 0: fbc disable, 1: fbc with default ratio, n: fbc with ratio = n (range 10-100) */
} gp_h264_cfg;

/* h264 rate control configuration */
typedef struct {
	int qpHdr;			/* initial QP. */
	int qpMin;			/* minimum QP. */
	int qpMax;			/* maximum QP. */
	int bitPerSeconds;	/* target Bitrate in ABR mode */
	int gopLen;			/* Group of Picture number */
	int frameRate;		/* estimate frame rate */
	int hierPFrames; 	/* set the number of hierarchical P-frames encoded after a regular P-frame */
	int qualityLevel; 	/* smaller values get better quality, used in H264_RC_CRF mode */
	int ae_night;		/* reduce qpMin when ae_night == 1 */
	int rc_mode;		/* H264_RC_CRF (constant quality) or H264_RC_ABR (average bitrate) */
	int vbv_dbg; 		/* Video Buffer Verifier debug, always set 0 */
} gp_h264enc_rate_ctrl;

/* h264 frame encode parameters */
typedef struct {
	INT32U RawFrameAddr;		/* input raw frame address, make sure raw frame data is 16x16 alignment. see crop in gp_h264_cfg. */
	INT32U BitstreamBaseAddr;	/* output bitstream buffer base address */
	INT32U BitstreamOutAddr;	/* output bitstream buffer current address */
	INT32U BitstreamEndAddr;	/* output bitstream buffer end address */
	INT8U PictureType;			/* picture type: 0 for I-frame, 1 for P-frame */
	INT8U InsertHdr;			/* insert sps/pps header or not (only for I-frame) */
	INT8U ae_night; 			/* TODO: move to gp_h264enc_rate_ctrl()*/
	INT32U input_fifo;			/* input fifo line (16 align) for this frame */
	INT8U format;				/* input format, see H264_FMT_* */
} gp_h264enc_in;


/* H.264 Encoder Width and Height Alignment

The width and height of video image frame should be 2-alignment, but the
actual width and height of input buffer must be 16-alignment.

Internally, H.264 encoder take input buffer contain multiple 16x16 macro
blocks, if encode width or height is not 16-alignment, driver will set
crop parameter in h.264 bit stream to make pixels outside target width or
height invisible/invalid.

Eg. For 1920x1080 video encode, the actual size of input buffer should be
1920x1088. When setting encode width and height as 1920x1080, driver will
crop out bottom 8 lines.

User should always allocate input image with 16-alignment width and height,
and make sure clear image data outside crop region if they are not natural
image. Random data may cause unexpected behavior of H.264 encoder.

*/


#ifndef H264_FMT_GP420
	#define H264_FMT_GP420	0
	#define H264_FMT_YUYV	1
	#define H264_FMT_YVYU	2
	#define H264_FMT_UYVY	3
	#define H264_FMT_VYUY	4
#endif

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/* h264 share functions ***************************************************/

/**
 * @brief   initial h264 driver
 */
void h264_drv_init();

/**
 * @brief   un-initial h264 driver
 */
void h264_drv_uninit();

/**
 * @brief   reset h264 driver wait status
 */
void h264_reset_wait_status();

/**
 * @brief   wait h264 finish
 * @param	time_out: in millisecond.
 */
INT32S h264_wait_finish(INT32U time_out);

/* h264 encode ************************************************************/

/**
 * @brief   get working buffers size of H.264 encoder instance
 * @param	cfg:	configuration of H.264 encoding, see gp_h264dec_cfg define.
 * @return  total size of working buffers, -1 if fail.
 * @note	if user wants to use custom external buffer, call this function to get required size.
 */
int gp_h264enc_get_buffer_size(gp_h264_cfg* cfg);

/**
 * @brief   initialize h264 encoder
 * @param	cfg: configuration of h264 encoder
 * @return	h264 encoder handle: return NULL when fail
 */
void* gp_h264enc_init(gp_h264_cfg* cfg);

/**
 * @brief   trigger h264 encode
 * @param	handle: h264 encoder handel
 * @param	in: 	encoder input setting
 * @return	0: Success -1: Fail
 */
int gp_h264enc_encode(void* handle, gp_h264enc_in* in);

/**
 * @brief   wait h264 encode frame to finish
 * @param	handle: 	h264 encoder handle
 * @param	time_out: 	time out value in millisecond
 * @param	len: 		pointer to return encoded bitstream length
 * @return	0: Success -1: Fail
 */
int gp_h264enc_encode_wait(void* handle, INT32U time_out);
int gp_h264enc_insert_skip(void* handle, gp_h264enc_in* in, INT8U keep_bitrate);
int gp_h264_get_hierP_Type(void* handle);
extern INT8U gp_h264_get_idr(void *handle);
extern void gp_h264_set_idr(void *handle, INT8U num);
extern INT8U gp_h264_get_frameNum(void *handle);
extern void gp_h264_set_frameNum(void *handle, INT8U num);
extern INT8U gp_h264_get_picOrderCnt(void *handle);
extern void gp_h264_set_picOrderCnt(void *handle, INT8U order);
void gp_h264enc_release(void* handle);

/**
 * @brief   set callback function for h264 encoding
 * @param	notify: function to notify h264 encoding done.
 */
void gp_h264enc_set_callback(void (*notify)(void* handle));

/**
 * @brief   re-trigger h264 encoder to continue encoding
 * @param	handle: 	h264 encoder handle
 * @param	status:		status return from h264enc_wait()
 * @param	inAddrs: 	next input address (for continue from scan2mb fifo end)
 * @param	outAddrs: 	next output address (for continue from h264 FIFO output end)
 * @param	outSize: 	next output buffer size (for continue from h264 FIFO output end)
 * @return	0: Success -1: Fail
 */
int gp_h264enc_restart(void* handle, INT32U status, INT32U inAddrs, INT32U outAddrs, INT32U outSize);

/**
 * @brief   reset h264 encoder driver
 */
void gp_h264enc_reset(void* handle);

/* h264 rate control ******************************************************/

/**
 * @brief   reconfigure rate control
 * @param	handle: h264 encoder handle
 * @param	cfg:	rate control configuration
 * @param	types:	RC_PARAM_TYPES, indicate what setting should be reconfigure.
 * @return	STATUS_OK
 *
 * you have to run gp_h264enc_reconfig_rc(..) right after changing rc parameters
 * example: update QP and bitrate at the same time
 *          cfg->qpMin = 30;
 *          cfg->bitPerSeconds = 800000;
 *          gp_h264enc_reconfig_rc(handle, cfg, RC_QP | RC_BITRATE);
 */
int gp_h264enc_reconfig_rc(void *handle, gp_h264enc_rate_ctrl* cfg, RC_PARAM_TYPES param_types);

/**
 * @brief   set rate control
 * @param	handle: h264 encoder handle
 * @param	cfg:	rate control configuration
 * @return	STATUS_OK
 */
int gp_h264enc_set_rc(void *handle, gp_h264enc_rate_ctrl* cfg);

/**
 * @brief   get current rate control qp value
 * @param	handle: h264 encoder handle
 * @return	qp value
 */
int gp_h264enc_get_qp(void *handle);
int gp_h264enc_wait(void* handle, INT32U time_out, INT32S* len);
int gp_h264enc_restart(void* handle, INT32U status, INT32U inAddrs, INT32U outAddrs, INT32U outSize);
void gp_h264enc_reset(void* handle);
extern void H264_IRQHandler(void);
extern void SCAN2MB_IRQHandler(void);
extern void H264_FIFO_IRQHandler(void);

/*== Turnkey ==*/
void gp_h264enc_ratecontrol_set(void* handle,INT32U buf_len);

#endif

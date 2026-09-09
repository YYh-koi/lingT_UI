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
#ifndef __drv_l2_CDSP_H__
#define __drv_l2_CDSP_H__

#include "gp_3a.h"
#include "drv_l2_actuator.h"
#include "gp_LensDef.h"
#include "drv_l1_cdsp.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/* cdsp input source */
#define C_CDSP_DVP		        0x0 //Parallel sensor
#define C_CDSP_SDRAM		    0x1
#define C_CDSP_MIPI			    0x2
#define C_CDSP_MIPI1		    0x3
#define C_CDSP_SKIP_WR		    0x4
#define C_CDSP_BINMODE          0x5
#define C_CDSP_BINMODE1         0x6
#define C_CDSP_BINMODE_MIPI     0x7 //use CDSP(binmode function)
#define C_CDSP_BINMODE_MIPI1    0x8 //use CDSP(binmode function)
#define C_CDSP_MAX			0xFF

/* cdsp interrupt isr index */
#define C_ISR_EOF			0	//eof switch buffer
#define C_ISR_SENSOR		1	//sensor set ae/awb
#define C_ISR_OVERFLOW		2
#define C_ISR_MD		    3   // md isr
#define C_ISR_MAX			4

/* dma mode */
#define C_DMA_CH			1	//0: auto switch, 1:dma a, 2: dma b

//+++
// Motion Detect
#define MOTION_DETECT_STATUS_IDLE	0
#define MOTION_DETECT_STATUS_START	1
#define MOTION_DETECT_STATUS_STOP	2

#define MD_SENS						10
#define MD_NORMAL					35
#define MD_SLOW						80
#define MD_OFF						127//0x7F
#define CDSP_MD_THR					MD_SENS
//---

#define WDR_EN                  	1

#define ISP_CAPTURE_MODE            MODE_PURERAW
#define AEBUFF_W_IRAM				ENABLE
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
	MODE_NORMAL = 0,
	MODE_NEGATIVE,
	MODE_SOLARISE,
	MODE_EMBOSSMENT,
	MODE_BINARIZE,
	MODE_SEPIA,
	MODE_BLACKWHITE
} CDSP_RAW_SPECIAL_MODE;

typedef enum
{
	MSG_CDSP_POSTPROCESS = 0x30000000,
	MSG_CDSP_SCALE_CROP,

	MSG_CDSP_BADPIX_OB,
	MSG_CDSP_LENS_CMP,
	MSG_CDSP_WBGAIN,
	MSG_CDSP_WB_OFFSET_DAY,
	MSG_CDSP_WB_OFFSET_NIGHT,
	MSG_CDSP_RAW_GAMMA,
	MSG_CDSP_RGB_GAMMA,
	MSG_CDSP_INTERPOLATION,
	MSG_CDSP_RAW_SPEF,
	MSG_CDSP_EDGE,
	MSG_CDSP_EDGE_AMPGA_DAY,
	MSG_CDSP_EDGE_AMPGA_NIGHT,
	MSG_CDSP_COLOR_MATRIX,
	MSG_CDSP_POSWB_RGB2YUV,

	MSG_CDSP_YUV_INSERT,
	MSG_CDSP_YUV_HAVG,
	MSG_CDSP_SPEC_MODE,
	MSG_CDSP_SAT_HUE,
	MSG_CDSP_SAT_HUE_DAY,
	MSG_CDSP_SAT_HUE_NIGHT,
	MSG_CDSP_SUPPRESSION,
	MSG_CDSP_NEWDENOISE,

	MSG_CDSP_RAW_WIN,
	MSG_CDSP_AE_WIN,
	MSG_CDSP_AF_WIN,
	MSG_CDSP_AWB_WIN,
	MSG_CDSP_WBGAIN2,
	MSG_CDSP_HISTGM,

	MSG_CDSP_TARGET_AE,
	MSG_CDSP_TARGET_AE_NIGHT,
	MSG_CDSP_WB_MODE,
	MSG_CDSP_EV,
	MSG_CDSP_ISO,
	MSG_CDSP_BACKLIGHT_DETECT,

	MSG_CDSP_3A_STATISTIC,
	MSG_CDSP_LINCORR,
	MSG_CDSP_MAX,

    MSG_CDSP_POSTLUT,
	MSG_CDSP_SHARPEN,
	MSG_CDSP_DENOISE,
	MSG_CDSP_HYBRID,
    MSG_CDSP_WDR,
    MSG_CDSP_NEW_WB,
    MSG_CDSP_CV_PARA,
    MSG_CDSP_WB_FACTOR,
    MSG_CDSP_SENSOR_ID,

	//GPA7XXXA
	MSG_CDSP_NLM,
	MSG_CDSP_MESH_LSC,
	MSG_ISP_DN3D_LUM,
	MSG_CDSP_AWB_DIV_THR,

	 MSG_CDSP_WB_SPEED,
	 MSG_CDSP_SENSOR_MAX_LUM,
	 MSG_CDSP_SENSOR_MAX_LUM2,


} CDSP_MSG_CTRL_ID;

typedef struct gpCdspCtrl_s
{
	INT32U id;
	INT32U value;
} gpCdspCtrl_t;

typedef struct gpCdspSensor_s
{
	INT32S (*sensor_set_ctrl)(gpCdspCtrl_t *ctrl);
	INT32S (*sensor_get_ctrl)(gpCdspCtrl_t *ctrl);
} gpCdspSensor_t;

typedef struct gpCdspFmt_s
{
	INT32U image_source;
	INT32U input_format;
	INT32U output_format;
	INT32U h_input;
	INT32U v_input;
	INT16U hpixel;
	INT16U vline;
	INT16U hoffset;
	INT16U voffset;
	INT32U sensor_timing_mode;
	INT32U sensor_hsync_mode;
	INT32U sensor_vsync_mode;
} gpCdspFmt_t;

typedef struct CdspDmaPara_s
{
	/* format set */
	INT8U fifo_enable;     /* Eanle cdsp fifo mode */
	INT8U fifo_line;       /* Line auto switch time */
	INT8U fifo_index;      /* A B buffer full triger */
	INT8U fifo_flag;
	INT16U InWidth;        /* image/csi h size */
	INT16U InHeight;       /* image/csi v size */
	INT16U OutWidth;       /* output buffer h size */
	INT16U OutHeight;      /* output buffer v size */
	INT32U FifoLoopMaxCnt; /* Fifo Loop Max Count:(OutHeight/fifo_line)*/
	INT32U FifoLoopCnt;    /* Fifo Loop Count*/
	INT32U FifoLoopAddr;
	INT32U fifo_size;
	INT32U FifoTotalSize;
	INT32U buffA;
	INT32U buffB;
} CdspDmaPara_t;

typedef struct gpCdspPostProcess_s
{
	INT32U SetFlag;
	INT32U inFmt;
	INT32U yuvRange;
	INT32U width;
	INT32U height;
	INT32U inAddr;
	INT32U outAddr;
} gpCdspPostProcess_t;

typedef struct gpCdspScalePara_s
{
	/* raw hscale */
	INT8U 	hscale_en;		/* 0:disable, 1:enable */
	INT8U 	hscale_mode;	/* 0:drop, 1:filter */
	INT16U	dst_hsize;		/* 0~0xFFF */

	/* crop function */
	INT32U	crop_en;		/* 0:disable, 1:enable */
	INT16U	crop_hoffset;	/* 1~0xFFF */
	INT16U	crop_voffset;	/* 1~0xFFF */
	INT16U	crop_hsize;		/* 1~0xFFF */
	INT16U	crop_vsize;		/* 1~0xFFF */

	/* yuv h/v scale down */
	INT8U	yuvhscale_en;	/* 0:disable, 1:enable */
	INT8U	yuvvscale_en;	/* 0:disable, 1:enable */
	INT8U	yuvhscale_mode; /* 0:drop, 1:filter */
	INT8U	yuvvscale_mode; /* 0:drop, 1:filter */
	INT16U	yuv_dst_hsize;	/* 0~0xFFF */
	INT16U	yuv_dst_vsize;	/* 0~0xFFF */

	/* read back size */
	INT16U	img_rb_h_size;		/* 0~0xFFF */
	INT16U	img_rb_v_size;		/* 0~0xFFF */
} gpCdspScalePara_t;

typedef struct gpCdspBadPixOB_s
{
	/* bad pixel */
	INT8U   badpixen; 		/* 0:disable, 1:enable */
	INT8U   badpixmirr;        /* badpixmirlen[0]: Enable badpix mirror 2 pixels, badpixmirren[1]: Enable badpix right mirror 2 pixels*/
	INT16U	bprthr;			/* 0~1023 */
	INT16U	bpgthr;			/* 0~1023 */
	INT16U	bpbthr;			/* 0~1023 */

	/* optical black */
	INT8U	manuoben;		/* 0:disable, 1:enable */
	INT8U   reserved1;
	INT16S	manuob;			/* -1024~1023 */

	INT8U   reserved2;
	INT8U   reserved3;
	INT8U	autooben;		/* 0:disable, 1:enable */
	INT8U	obtype;			/* 0:2x256,1:4x256,2:8x256,3:16x256,4:256x2,5:256x4,6:256x8,7:256x16 */

	INT16U	obHOffset;		/* 0~0xFFF */
	INT16U	obVOffset;		/* 0~0xFFF */
	INT16U	Ravg;			/* read back auto ob r avg */
	INT16U	GRavg;			/* read back auto ob gr avg */
	INT16U	Bavg;			/* read back auto ob b avg */
	INT16U	GBavg;			/* read back auto ob gb avg */

	/* white balance */
	INT8U	wboffseten;		/* 0:disable, 1:enable */
	INT8S	roffset;		/* -128 ~ 127 */
	INT8S	groffset;		/* -128 ~ 127 */
	INT8S	boffset;		/* -128 ~ 127 */
	INT8S	gboffset;		/* -128 ~ 127 */
} gpCdspBadPixOB_t;

typedef struct gpCdspLensCmp_s
{
	/* lens compensation */
	INT32U	lcen;				/* 0:disable, 1:enable 			*/
	INT32U	ve_en;				/* 0:disable, 1:enable 			*/
	INT16U	*maxtan8_table;		/* maxtan8 table				*/
	INT16U	*slop4_table;		/* slop4 table				*/
	INT16U	*clpoint_table;		/* clpoint table				*/
	INT16U  *rdiusfile0_table;	/* radiusfile0_table, size 512 byte */
	INT16U	*rdiusfile1_table;	/* radiusfile1_table, size 512 byte */
    INT16S  *veparm;

	INT16U  *rdiusfile0_table2;	/* radiusfile0_table, size 512 byte */
	INT16U	*rdiusfile1_table2;	/* radiusfile1_table, size 512 byte */

	INT16U	seg_R;			/* 0~0xFFF */

	INT16U  cmp_ratio_r; // 0 ~ 256
	INT16U  cmp_ratio_g; // 0 ~ 256
	INT16U  cmp_ratio_b; // 0 ~ 256
} gpCdspLensCmp_t;


typedef struct gpIsp_MeshLsc_s
{
	/* Mash lens compensation */
	INT8U	lscen;				/* 0:disable, 1:enable 		*/
	INT32U	*grid_table;		/* Grid table				*/
	INT32U	*gain_a_table;		/* Gain A table				*/
	INT32U	*gain_b_table;		/* Gain B table				*/
	INT8U	sw;				    /* Change A or B Table		*/

	INT16U  cmp_ratio_r; 		/* 0 ~ 256					*/
	INT16U  cmp_ratio_g; 		/* 0 ~ 256					*/
	INT16U  cmp_ratio_b;  		/* 0 ~ 256					*/

} gpIsp_MeshLsc_t;

typedef struct IspAwbMode_s
{
	INT8U	awb_en;
	INT8U	y_mode;
	INT8U	subsample;

    INT16U	h_start_idx, h_end_idx, v_start_idx, v_end_idx;

	INT8U	thr1, thr2;
	INT8U	thr13;

	INT8U	thr1_2, thr2_2;
	INT8U	thr13_2;

	INT16U	thr3, thr4;
	INT16S	thr5, thr6;
	INT16U	thr7, thr8;
	INT16U	thr9, thr10;
	INT32U	thr11, thr12;

	/*GPA7XXXA*/
    INT16U	thr14, thr15;
	INT16U	thr16, thr17;

	INT16U	thr3_2, thr4_2;
	INT16S	thr5_2, thr6_2;
	INT16U	thr7_2, thr8_2;
	INT16U	thr9_2, thr10_2;
	INT32U	thr11_2, thr12_2;

	/*GPA7XXXA*/
    INT16U	thr14_2, thr15_2;
	INT16U	thr16_2, thr17_2;
} IspAwbMode_t;

typedef struct IspAwbRdGBdG_s
{	/*GPA7XXXA*/
	INT8U	awb_en, awb_win_hold;

	INT8U	awb_Y_thr0, awb_Y_thr1;
	INT8U 	awb_G_diff_thr;

	INT16U	x, y, h, w;

	INT16U 	RdG_thr1, RdG_thr2, RdG_thr3, RdG_thr4;
	INT16U 	RdG_thr5, RdG_thr6, RdG_thr7, RdG_thr8;
	INT16U 	RdG_thr9, RdG_thr10,RdG_thr11,RdG_thr12;
	INT16U 	RdG_thr13,RdG_thr14,RdG_thr15,RdG_thr16;
	INT16U 	BdG_thr1, BdG_thr2, BdG_thr3, BdG_thr4;
	INT16U	 BdG_thr5, BdG_thr6, BdG_thr7, BdG_thr8;
	INT16U	 BdG_thr9, BdG_thr10,BdG_thr11,BdG_thr12;
	INT16U 	BdG_thr13,BdG_thr14,BdG_thr15,BdG_thr16;
}IspAwbRdGBdG_t;

typedef struct gpCdspWhtBal_s
{
	/* white balance */
	INT8U	wboffseten;		/* 0:disable, 1:enable */
	INT8S	roffset;		/* -128 ~ 127 */
	INT8S	groffset;		/* -128 ~ 127 */
	INT8S	boffset;		/* -128 ~ 127 */
	INT8S	gboffset;		/* -128 ~ 127 */

	INT8U	wbgainen;		/* 0:disable, 1:enable */
	INT8U	global_gain;	/* 0~0xFF */
	INT8U 	reserved;

	INT16U	rgain;			/* 3.6 bit, 0/64~511/64 */
	INT16U	grgain;			/* 3.6 bit, 0/64~511/64 */
	INT16U	bgain;			/* 3.6 bit, 0/64~511/64 */
	INT16U	gbgain;			/* 3.6 bit, 0/64~511/64 */
} gpCdspWhtBal_t;

typedef struct gpCdspIntpl_s
{
	/* interpolation */
	INT16U	int_low_thr;	/* 0~0xFF */
	INT16U	int_hi_thr;		/* 0~0xFF */
	/* raw special mode */
	INT8U	rawspecmode;	/* 0~6 */
	INT8U	mode;           /*0: org,  1: Comi*/
} gpCdspIntpl_t;

typedef struct gpCdspEdge_s
{
	/* edge lut table */
	INT8U	eluten;			/* 0:disable, 1:enable */
	INT8U 	*edge_table;	/* edge table, 256byte */

	/* edge enhance */
	INT8U	edgeen;			/* 0:disable, 1:enable */
	INT8U	lhdiv;			/* 1/lhdiv,  1, 2, 4, 8, 16, 32, 64, 128 */
	INT8U	lhtdiv;			/* 1/lhtdiv,  1, 2, 4, 8, 16, 32, 64, 128 */

	INT8U	lhcoring;		/* 0~0xFF */
	INT8U	ampga;			/* 1, 2, 3, 4 */
	INT8U	edgedomain;		/* 0:add y value, 1:add RGB value */
	INT8U	lhmode;			/* 0:USE HPF LF00 matrix, 1:default matrix */

	INT8U	Qthr;			/* edge thr set */
	/* edge filter */
	INT8U	lf00;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	lf01;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	lf02;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */

	INT8U	lf10;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	lf11;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	lf12;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	lf20;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */

	INT8U	lf21;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	lf22;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	reserved0;
	INT8U	reserved1;

	INT32U 	Qcnt;			/* read back edge thr count */
} gpCdspEdge_t;

typedef struct gpCdspCorMatrix_s
{
	/* color matrix */
	INT8U	colcorren;		/* 0:disable, 1:enable */
	INT8U	reserved0;

	INT16S	a11;			/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S	a12;			/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S	a13;			/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S	a21;			/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S	a22;			/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S	a23;			/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S	a31;			/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S	a32;			/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S	a33;			/* 1 sign + 3.7 bit, -1024/128~1023/128 */

	/*GPA7XXXA*/
	INT16U  col_thr0, col_thr1, col_thr2;
	INT16U  col_thr3, col_thr4, col_thr5, col_thr6;

    INT16U col_scale0, col_scale1, col_scale2;
    INT16U col_scale3, col_scale4, col_scale5;
    INT16U col_scale6, col_scale7;

} gpCdspCorMatrix_t;

typedef struct gpCdspRgb2Yuv_s
{
	/* pre rb clamp */
	INT8U  	pre_rbclamp;
	/* rb clamp */
	INT8U  	rbclampen;
	INT16U 	rbclamp;

	/* Uv divider */
	INT8U	uvdiven;		/* 0:disable, 1:enable */
	INT8U 	reserved;
	INT8U	Yvalue_T1;		/* 0~0xFF, when Y=T1, UV divide 1/8 */
	INT8U	Yvalue_T2;		/* 0~0xFF, when Y=T2, UV divide 2/8 */
	INT8U	Yvalue_T3;		/* 0~0xFF, when Y=T3, UV divide 3/8 */
	INT8U	Yvalue_T4;		/* 0~0xFF, when Y=T4, UV divide 4/8 */
	INT8U	Yvalue_T5;		/* 0~0xFF, when Y=T5, UV divide 5/8 */
	INT8U	Yvalue_T6;		/* 0~0xFF, when Y=T6, UV divide 6/8 */
} gpCdspRgb2Yuv_t;

typedef struct gpCdspGamma_s
{
	INT16U	lut_gamma_en;	/* 0:disable, 1:enable */
	INT16U	sensor_id;
	INT32U 	*gamma_table;	/* gamma table, 512 byte */
} gpCdspGamma_t;


typedef struct gpCdspSpecMod_s
{
	/* yuv special mode */
	INT8U	yuvspecmode;	/* 0~7 */
	INT8U	binarthr;		/* 0~0xFF */
	INT8U	reserved0;
	INT8U	reserved1;
} gpCdspSpecMod_t;

typedef struct gpCdspYuvInsert_s
{
	/* yuv 422 to 444 */
	INT8U	yuv444_insert;	/* 0:disable, 1:enable */
	/* yuv coring threshold */
	INT8U	y_coring;		/* 0~0xF */
	INT8U	u_coring;		/* 0~0xF */
	INT8U	v_coring;		/* 0~0xF */
	/* yuv coring subtraction */
	INT8U	y_corval;		/* 0~0xF */
	INT8U	u_corval;		/* 0~0xF */
	INT8U	v_corval;		/* 0~0xF */
	INT8U	reserved;
} gpCdspYuvInsert_t;

typedef struct gpCdspYuvHAvg_s
{
	/* h average */
	INT8U	ytype;			/* 0:disable, 1:3tap, 2:5tap */
	INT8U	utype;			/* 0:disable, 1:3tap, 2:5tap */
	INT8U	vtype;			/* 0:disable, 1:3tap, 2:5tap */
	INT8U   reserved;
} gpCdspYuvHAvg_t;

typedef struct gpCdspSatHue_s
{
	INT8U	YbYcEn;			/* 0:disable, 1:enable */
	/* brightness & constract */

	INT8U	reserved1;

	INT16U	sensor_id;
	INT32U  *y_lut_table;

	INT8S	u_offset;		/* -128 ~ 127 */
	INT8S	v_offset;		/* -128 ~ 127 */

	INT8U	u_scale;		/* 3.5 bit, 0/32~255/32 */
	INT8U	v_scale;		/* 3.5 bit, 0/32~255/32 */

	/* saturation, hue */
	INT8S	u_huesindata;	/* 1 sign + 1.6 bit , -128/64~127/64 */
	INT8S	u_huecosdata;	/* 1 sign + 1.6 bit , -128/64~127/64 */
	INT8S	v_huesindata;	/* 1 sign + 1.6 bit , -128/64~127/64 */
	INT8S	v_huecosdata;	/* 1 sign + 1.6 bit , -128/64~127/64 */
} gpCdspSatHue_t;

typedef struct gpCdspSuppression_s
{
	/* suppression */
	INT8U	suppressen;		/* 0:disable, 1:enable */
	INT8U   suppr_mode;		/* 0:edge, 1:denoise, 2:YLPF */

	/* y denoise */
	INT8U 	denoisen;		/* 0:disable, 1:enable */
	INT8U	denoisethrl;	/* 0~0xFF */
	INT8U	denoisethrwth;	/* 1, 2, 4, 8, 16, 32, 64, 128 */
	INT8U	yhtdiv;			/* 1, 2, 4, 8, 16, 32, 64, 128 */

	/* y lpf */
	INT8U	lowyen;			/* 0:disable, 1:enable */
	INT8U   reserved;
} gpCdspSuppression_t;

typedef struct gpCdspRawWin_s
{
	INT8U	aeawb_src;		/* 0:from poswb, 1:form awb line ctrl */
	INT8U	subsample; 		/* 0:disable, 1:1/2, 2:1/4 subsample */
	INT8U 	AeWinTest;		/* 0:disable, 1:enable */
	INT8U 	AfWinTest;		/* 0:disable, 1:enable */
	/* rgb window set */
	INT16U	hwdoffset;		/* 0~0x3FFF */
	INT16U	vwdoffset;		/* 0~0x3FFF */
	INT16U	hwdsize;		/* 0~0x1FF, h cell size */
	INT16U	vwdsize;		/* 0~0x1FF, v cell size */

	INT16U  bpthr;
	INT16U  wpthr;
} gpCdspRawWin_t;

typedef struct gpIspAF_s
{
	/* auto focuse */
	INT8U	af_win_en;		/* 0:disable, 1:enable */
	INT8U	af_win_hold;	/* 0:disable, 1:enable */
	INT8U	af_gamma_en;	/* 0:disable, 1:enable */
	INT8U	af_win_test;
	//INT8U	reserved01;
	/* af window 1 */
	INT16U	af1_hoffset;	/* 0~0xFFF */
	INT16U 	af1_voffset;	/* 0~0xFFF */
	INT16U 	af1_hsize;		/* 0~0xFFF */
	INT16U	af1_vsize;		/* 0~0xFFF */
	/* af window 2 */
	INT16U	af2_hoffset;	/* 0~0xFFF */
	INT16U 	af2_voffset;	/* 0~0xFFF */
	INT16U 	af2_hsize;		/* 64, 256, 512, 1024, 2048 */
	INT16U	af2_vsize;		/* 64, 256, 512, 1024, 2048 */
	/* af window 3 */
	INT16U	af3_hoffset;	/* 0~0xFFF */
	INT16U 	af3_voffset;	/* 0~0xFFF */
	INT16U 	af3_hsize;		/* 64, 256, 512, 1024, 2048 */
	INT16U	af3_vsize;		/* 64, 256, 512, 1024, 2048 */

    INT16U	af_a0;
	INT16U	af_a1;
	INT16U	af_a2;
	INT16U	af_a3;
	INT16U	af_a4;

	INT16U	af_b0;
	INT16U	af_b1;
	INT16U	af_b2;
	INT16U	af_b3;
	INT16U	af_b4;

	INT16U	af_thr1;
	INT16U	af_thr2;
	INT16U 	af_thr3;
	INT16U	af_thr4;
	AF_SEARCH_MODE mode;
	INT8U   af_step;
	INT8U   af_min_idx;
	INT8U   af_max_idx;
} gpIspAF_t;




typedef struct gpCdspAE_s
{
	/* auto expore */
	INT8U	ae_win_en;		/* 0:disable, 1:enable */
	INT8U	ae_win_hold;	/* 0:disable, 1:enable */
	INT16U  ae_meter;
	//float   ae_user_weight_table[64];//user define ae weight table
	INT32U   *ae_user_weight_table;//user define ae weight table
} gpCdspAE_t;

typedef struct gpCdspAWB_s
{
	/* auto white balance */
	INT8U	awb_win_en;		/* 0:disable, 1:enable */
	INT8U	awb_win_hold;	/* 0:disable, 1:enable */
	INT8U	awbclamp_en;	/* 0:disable, 1:enable */
	INT8U	awbwinthr;		/* 0~0xFF */
	INT8U	sindata;		/* 1 sign + 1.6bits, -128/64~127/64 */
	INT8U	cosdata;		/* 1 sign + 1.6bits, -128/64~127/64 */
	INT8U	reserved0;
	INT8U	reserved1;

	INT8U	Ythr0;			/* 0~0xFF */
	INT8U	Ythr1;			/* 0~0xFF */
	INT8U	Ythr2;			/* 0~0xFF */
	INT8U	Ythr3;			/* 0~0xFF */

	INT16S	UL1N1;			/* -256 ~ 255 */
	INT16S	UL1P1;			/* -256 ~ 255 */
	INT16S	VL1N1;			/* -256 ~ 255 */
	INT16S	VL1P1;			/* -256 ~ 255 */

	INT16S	UL1N2;			/* -256 ~ 255 */
	INT16S	UL1P2;			/* -256 ~ 255 */
	INT16S	VL1N2;			/* -256 ~ 255 */
	INT16S	VL1P2;			/* -256 ~ 255 */

	INT16S	UL1N3;			/* -256 ~ 255 */
	INT16S	UL1P3;			/* -256 ~ 255 */
	INT16S	VL1N3;			/* -256 ~ 255 */
	INT16S	VL1P3;			/* -256 ~ 255 */

	INT16U	GdivBthr1;						/*0~0x3FFF*/
	INT16U	GdivBthr2;						/*0~0x3FFF*/
	INT16U	GdivRthr1;						/*0~0x3FFF*/
	INT16U	GdivRthr2;						/*0~0x3FFF*/

} gpCdspAWB_t;

typedef struct gpCdspWbGain2_s
{
	/* wb gain2 */
	INT8U	wbgain2en;		/* 0:disable, 1:enable */
	INT8U	reserved;
	INT16U	rgain2;			/* 0~0x1FF */
	INT16U	ggain2;			/* 0~0x1FF */
	INT16U	bgain2;			/* 0~0x1FF */
} gpCdspWbGain2_t;

typedef struct gpCdspHistgm_s
{
	/* histgm */
	INT8U	his_en;			/* 0:disable, 1:enable */
	INT8U	his_hold_en;	/* 0:disable, 1:enable */
	INT16U	H_Start_idx;					/* 0~0x0FFF*/
	INT16U	H_End_idx;						/* 0~0x0FFF*/
	INT16U	V_Start_idx;					/* 0~0x0FFF*/
	INT16U	V_End_idx;						/* 0~0x0FFF*/

	INT8U his_rgb_path;
	INT8U shift_bit;
} gpCdspHistgm_t;

typedef struct gpCdsp3aResult_s
{
    INT64U          dummy;

    IspAfResult_t   af_result;

	/* awb */
	AWB_RESULT_t awb_result;
	IspAwbResult_t	new_abw_result;

	/* ae */
	IspAeResult_t    ae_result;

	/* histgm */
	INT32U histogram[64];	/* Y histgm */

	INT32U R_histogram[64];	/* R histgm */
	INT32U G_histogram[64];	/* G histgm */
	INT32U B_histogram[64];	/* B histgm */

} gpCdsp3aResult_t;

typedef struct gpIspHybridRaw_s
{
    INT16U IspHeight;
    INT16U ISPWidth;

    /* new denoise enhance */
	INT8U DefectPixelSel;
	INT8U DefectPixelEnable;
	INT8U DenoiseEnable;
   	INT8U CrosstalkEnable;

   	INT8U DPCth1_1;
   	INT8U DPCth1_2;
   	INT8U DPCth2;
   	INT8U DPCth3;

   	INT8U DPCDefaultMode;
   	INT8U DPCn;

   	INT8U DenoiseT1;
   	INT8U DenoiseT2;
   	INT8U DenoiseT3;
   	INT8U DenoiseT4;
   	INT8U DenoiseW1;
   	INT8U DenoiseW2;
   	INT8U DenoiseW3;
   	INT8U DenoiseW4;
   	INT8U DenoiseW5;

   	INT8U CrosstalkT1;
   	INT8U CrosstalkT2;
   	INT8U CrosstalkT3;
   	INT8U CrosstalkT4;
   	INT8U CrosstalkW1;
   	INT8U CrosstalkW2;
   	INT8U CrosstalkW3;
	INT8U CrosstalkGbGr;
} gpIspHybridRaw_t;

typedef struct gpCdspLinCorr_s
{
	/* color matrix */
	INT8U	lincorren;		/* 0:disable, 1:enable */
	INT8U	reserved0;
	INT8U*	lincorr_table;	/* 1 sign + 3.6 bit, -512/64~511/64 */
} gpCdspLinCorr_t;

typedef enum
{
	MODE_PURERAW = 0x100,
	MODE_LENSCMP,
	MODE_WBGAIN,
	MODE_LSC_WBGAIN,
	MODE_YUV,
	MODE_MAX
} CDSP_RAW_CALIBRATION_MODE;

typedef struct gpCdspWBGain_s
{
	unsigned short max_rgain;
	unsigned short max_bgain;
	unsigned short min_rgain;
	unsigned short min_bgain;
}gpCdspWBGain_t;

typedef struct IspNLM_Para_s
{
	INT8U   nlm_en;
	INT32U  nlm_w, nlm_h;
	INT16U	g_thrsohld;

    INT16U	(*nlm_table)[84];		    /* Nlm table				*/

	INT16U	g0_thr0, g0_thr1, g0_thr2;
	INT16U	g0_thr3, g0_thr4, g0_thr5;
	INT8U	g0_wt0, g0_wt1, g0_wt2;
	INT8U	g0_wt3, g0_wt4, g0_wt5;

	INT16U	g8_thr0, g8_thr1, g8_thr2;
	INT16U	g8_thr3, g8_thr4, g8_thr5;
	INT8U	g8_wt0, g8_wt1, g8_wt2;
	INT8U	g8_wt3, g8_wt4, g8_wt5;


	INT16U	g16_thr0, g16_thr1, g16_thr2;
	INT16U	g16_thr3, g16_thr4, g16_thr5;
	INT8U	g16_wt0, g16_wt1, g16_wt2;
	INT8U	g16_wt3, g16_wt4, g16_wt5;


    INT16U	g32_thr0, g32_thr1, g32_thr2;
    INT16U	g32_thr3, g32_thr4, g32_thr5;
    INT8U	g32_wt0, g32_wt1, g32_wt2;
    INT8U	g32_wt3, g32_wt4, g32_wt5;

    INT16U	g64_thr0, g64_thr1, g64_thr2;
    INT16U	g64_thr3, g64_thr4, g64_thr5;
    INT8U	g64_wt0, g64_wt1, g64_wt2;
    INT8U	g64_wt3, g64_wt4, g64_wt5;

    INT16U	g128_thr0, g128_thr1, g128_thr2;
    INT16U	g128_thr3, g128_thr4, g128_thr5;
    INT8U	g128_wt0, g128_wt1, g128_wt2;
    INT8U	g128_wt3, g128_wt4, g128_wt5;

    INT16U	g255_thr0, g255_thr1, g255_thr2;
    INT16U	g255_thr3, g255_thr4, g255_thr5;
    INT8U	g255_wt0, g255_wt1, g255_wt2;
    INT8U	g255_wt3, g255_wt4, g255_wt5;
} IspNLM_Para_t;

typedef struct gpCdspDev_s
{
	/*Sub device*/
	gpCdspSensor_t *sensor_sd;
	void (*interrupt_handle[C_ISR_MAX])(void);


	/* AE buffer */
	volatile ALIGN64 unsigned int aeAddr[2][512];

	INT8U OpenCnt;	/* open count */
	INT8U RunFlag;	/* Cdsp running flag */
	INT8U SkipCnt;	/* Cdsp skip count */
	INT8U MclkEn;	/* mclk enable flag */

	/* format set */
	INT32U imgSrc; /* image input source, sdram, front, mipi */
	INT32U inFmt; /* input format */
	INT32U rawFmtFlag; /* raw Format Flag */
	INT32U rawFmt; /* raw Format Flag */
	INT16U imgWidth; /* image/csi h size */
	INT16U imgHeight; /* image/csi v size */
	INT16U imgRbWidth; /* output buffer h size */
	INT16U imgRbHeight; /* output buffer v size */

	/* cdsp module set */
	gpCdspScalePara_t scale; /* scale down set */
	gpCdspSuppression_t suppression; /* suppression mode */
	gpCdsp3aResult_t cdsp3aresult, cdsp3aresult2;


	gpIspHybridRaw_t IspHybridRaw;

	/* cdsp 2A */
	INT32U ae_awb_task_en;
	volatile INT32U ae_awb_flag, ae_awb_flag2;
	INT32S color_temp_init_val;
	INT32S sensor_ev_idx;
	INT32S ae_target;
	INT32S ae_target_night;
	INT32S ae_target2;
	INT32S ae_target_night2;
    INT32U *lpAE_User_weight_table;
    INT32U af_win_en;

	INT32S wb_factor_r, wb_factor_b;
	INT32S wb_factor_r2, wb_factor_b2;
	INT32S edge_day, edge_night;
	INT32S edge_day2, edge_night2;

	INT32S sat_yuv_level[6][4], sat_yuv_level2[6][4];
	INT32S lum_thr[16], lum_thr2[16];
	INT32S edge_level[6], edge_level2[6];
	INT32S sat_contr_idx, sat_contr_idx2;
	INT32S pre_sat_contr_idx;
	uv_divide_t UVDivide;

	//ae, awb
	INT16U ae_win_w, ae_win_h;
	INT16U hist_win_w, hist_win_h;
	INT16U ae_fr_thr, ae_fr_cnt, af_fr_thr, af_fr_cnt;
	INT16U awb_fr_thr, awb_fr_cnt;
	INT16U ae_fr_thr2, ae_fr_cnt2;
	INT16U awb_fr_thr2, awb_fr_cnt2;

	INT16U rgain, bgain, ggain, global_gain;
	INT16U rgain2, bgain2, ggain2, global_gain2;



	INT16S high_lum_switch_cnt;
    INT16S low_lum_switch_cnt;
    INT8U int_low_thr, int_hi_thr;
    INT8U lsc_write_buf_idx;  // 0: A buffer,  1: B buffer
    INT8U lsc_en;
    INT16U wdr_status;

    INT8U 	 *af_workmem;
	INT8U 	 *ae_workmem, *ae_workmem2;
	INT8U	 *awb_workmem, *awb_workmem2;
	AWB_MODE awbmode;

    int sensor_id; // 2 sensors mode
    int two_sensor_mode;
    int set_ctrl_sensor_id;
    int sensor_ev_step, sensor_ev_step2;

    sensor_exposure_t si;

	void (*sensor_set_exptime)(int);
	void (*sensor_set_exptime2)(int);
    void (*sensor_get_ae_info)(sensor_exposure_t *si);
    void (*sensor_get_ae_info2)(sensor_exposure_t *si);

    void (*act_set_val)(int);

    INT16S  awb_thr[33];

    INT16S  awb_new_thr[2][17*2];

	gpCisCali_t LensCaliInfo, LensCaliInfo2;

    INT16U  binmode_crop_x;
    INT16U  binmode_crop_y;
    INT16U  binmode_crop_width;
    INT16U  binmode_crop_height;
    INT32U  binmode_cfg;

    INT16U  cdsp_busy_flag;

	//gpCisCali_t sensor_cdsp;
	INT16S		cm[9];
	INT32U      lsc_gain[2 * 23 * 31];
	INT32U      analog_gain_thr[6];
	//INT32U      dummy[4];

	INT8U awb_speed;
	INT8U max_ev_idx,max_ev_idx2;
	INT8U using_max_lum2;

	INT32U ylut[64];
    INT32U cl, lpt, hpt;
} gpCdspDev_t;

typedef struct CdspaRawPara_s
{
	/* format set */
	INT8U capture_flag;
	INT8U raw_bit;
	INT8U save_flag;
	INT16U InWidth;        	/* Sensor h size */
	INT16U InHeight;       	/* Sensor v size */
	INT16U OutWidth;       	/* output buffer h size */
	INT16U OutHeight;      	/* output buffer v size */
	INT32U Rawbuff;		/*Raw data out address*/
	INT32U CaptureMode;
	INT32U ImageSize;
} CdspRawPara_t;

typedef struct CdspAutoMode_s
{
	INT16U	fifo_size;		/* output fifo size */
	INT8U	mode;			/* AUTO_MODE_* */
	INT8U	buf_num;		/* output buffer number, maximum 8 */
	INT32U	buf_addr[8];	/* output buffer address */
	INT8U	fifo_stop;		/* fifo output stop when stream start */
} CdspAutoMode_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

extern gpCdspDev_t *CdspDev;
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
typedef void (*CDSP_ISR_EOF_FUN) (void);

extern void drv_l2_CdspIsrRegister(INT32U isr_index, void (*handle)(void));
extern void drv_l2_CdspSensorCtrlRegister(gpCdspSensor_t *pCtrl);
extern void drv_l2_CdspTableRegister(gpCisCali_t *sensor, int sensor_id);
extern INT32S drv_l2_CdspPostProcess(gpCdspPostProcess_t *PostPress);
extern INT32S drv_l2_cdsp_get_ctrl(gpCdspCtrl_t *ctrl);
extern INT32S drv_l2_cdsp_set_ctrl(gpCdspCtrl_t *ctrl);
extern INT32S drv_l2_cdsp_set_fmt(gpCdspFmt_t *pFmt, INT32U num_sensor);
extern INT32S drv_l2_cdsp_stream_on(INT32U ae_awb_task_en, INT32U bufA, INT32U bufB, INT32U num_sensors);
extern INT32S drv_l2_cdsp_stream_off(void);
extern INT32S drv_l2_cdsp_open(void);
extern INT32S drv_l2_cdsp_close(void);

extern void drv_l2_cdsp_yuyv_restart(INT32U g_frame_addr);
extern void drv_l2_capture_raw_set_cfg(CdspRawPara_t* pIspCaptureArg);
extern void drv_l2_cdsp_raw_preset(void);
extern void drv_l2_capture_raw_save_set(INT8U sarg);
extern INT8U drv_l2_capture_raw_save_get(void);
extern void drv_l2_cdsp_save_raw(void);

extern INT32S drv_l2_cdsp_set_linear_correction(void);
extern INT32S drv_l2_cdsp_ae_is_night(void);
extern void drv_l2_cdsp_clock_enable(INT16U en);
extern void drv_l2_cdsp_AEAWBOnOff(INT32U ctrlValue);
extern INT32S drv_l2_cdsp_isr_once_set(CDSP_ISR_EOF_FUN isr);
extern void drv_l2_cdsp_set_targetLum(INT16U targetLum);
void drv_l2_cdsp_sat_contrast_reinit_thr(void);
void drv_l2_isp_set_actuator(drv_l2_actuator_ops_t* act);
extern void drv_l2_set_2sensor_mode(int enable);
extern INT32S cdsp_isr_callback_set(void (*callback)(INT32U cdsp_event));
extern INT32S drv_l2_cdsp_set_automode(INT8U enable, CdspAutoMode_t* auto_cfg);
extern void drv_l2_cdsp_automode_stop(INT8U enable);
extern void drv_l2_isp_dram_raw_read(INT32U drambuffer, INT32U outbuffer, INT32U wsize, INT32U hsize, INT32U Twsize, INT32U Thsize);
extern void drv_l2_cdsp_analog_gain_thr_init(INT32U *again_thr);

extern void drv_l2_cdsp_edge_set_by_config(INT8U edge);
extern void motion_detect_status_set(INT8U statusVal);
extern void drv_l2_cdsp_ReDoConfig(INT32U target_w, INT32U target_h);

extern void drv_l2_CdspUserAeTableRegister(gpUserFav_t *AeTable);
extern void drv_l2_cdsp_extand_h_blank(INT32U line_cnt);
extern INT32U drv_l2_CdspSetFifoSize(INT16U width);
extern INT8U motion_detect_status_get(void);
extern void sensor_frame_clip_done(INT8U statusVal);
extern INT32U drv_l2_CdspGetIso(void);


extern void sensor_set_max_lum(int max_lum);
extern INT32S drv_l2_cdsp_set_awb_speed(int speed);


extern INT32S drv_l2_cdsp_get_saturation_night(gpCdspSatHue_t *sat_hue);
extern INT32S drv_l2_cdsp_get_saturation_day(gpCdspSatHue_t *sat_hue);
extern INT32U drv_l2_cdsp_get_cdsp_spec_mode(gpCdspSpecMod_t *spec_mode);
extern INT32U drv_l2_cdsp_set_cdsp_spec_mode(INT8U specmode);


#endif	/*__drv_l2_CDSP_H__*/

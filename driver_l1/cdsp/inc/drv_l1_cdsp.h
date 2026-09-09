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
#ifndef __drv_l1_CDSP_H__
#define __drv_l1_CDSP_H__

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define CDSP_DEBUG			1
#define _DRV_L2_CDSP        1
// image source
#define FRONT_INPUT		    0
#define SDRAM_INPUT		    1
#define MIPI_INPUT		    2   //for 2Lane & 1Lane MIPI0
#define MIPI1_INPUT		    3   //for 1Lane MIPI1
#define BINMODE_INPUT       5
#define BINMODE1_INPUT      6
#define MIPI_ISP_BINMODE    7
#define MIPI1_ISP_BINMODE   8

// cdsp int type
#define CDSP_INT_BIT		(1 << 0)
#define FRONT_VD_INT_BIT	(1 << 1)
#define FRONT_INT_BIT		(1 << 2)

// cdsp interrupt bit
#define CDSP_MD_UPDATE	        (1 << 0)
#define CDSP_AWBWIN_UPDATE	    (1 << 1)
#define CDSP_AEWIN_SEND		    (1 << 2)
#define CDSP_OVERFOLW		    (1 << 3)
#define CDSP_EOF			    (1 << 4)
#define CDSP_FACWR			    (1 << 5)
#define CDSP_FIFO			    (1 << 6)
#define CDSP_NEW_AWB		    (1 << 7)
#define CDSP_BINMODE_OVERFLOW   (1 << 8)
#define CDSP_SOF		        (1 << 9)
#define CDSP_TIME_OUT	        (1 << 10)
#define CDSP_SDRAM_FIFO	        (1 << 11)
#define CDSP_AFWIN_SEND	        (1 << 13)

#define CDSP_INT_ALL			0x3FFF//0x37
#define CDSP_INT_ALL_WFIFO		0x27F

// front vd interrupt bit
#define FRONT_VD_RISE		0x1
#define FRONT_VD_FALL		0x2

// front interrupt bit
#define VDR_EQU_VDRINT_NO	(1 << 0)
#define VDF_EQU_VDFINT_NO	(1 << 1)
#define SERIAL_DONE			(1 << 2)
#define SNAP_DONE			(1 << 3)
#define CLR_DO_CDSP			(1 << 4)
#define EHDI_FS_EQU_INTH_NO	(1 << 5)
#define FRONT_VVALID_RISE	(1 << 6)
#define FRONT_VVALID_FALL	(1 << 7)
#define OVERFLOW_RISE		(1 << 8)
#define OVERFLOW_FALL		(1 << 9)

// yuv special mode
#define SP_YUV_NONE			0
#define SP_YUV_NEGATIVE		1
#define SP_YUV_BINARIZE		2
#define SP_YUV_YbYcSatHue	3
#define SP_YUV_EMBOSSMENT	4
#define SP_YUV_EMBOSSMENTx2	5
#define SP_YUV_EMBOSSMENTx4	6
#define SP_YUV_EMBOSSMENTx8	7

// raw spec mode
#define SP_RAW_NONE			0
#define SP_RAW_NEGATIVE		1
#define SP_RAW_SOL_ARISE	2
#define SP_RAW_EMBOSSMENT	3
#define SP_RAW_BINARIZE		4
#define SP_RAW_SEPIA		5
#define SP_RAW_BLACK_WHITE	6

// image input source
#define C_SDRAM_FMT_RAW8	0x10
#define C_SDRAM_FMT_RAW10	0x11
#define C_FRONT_FMT_RAW8	0x12
#define C_FRONT_FMT_RAW10	0x13
#define C_MIPI_FMT_RAW8		0x14
#define C_MIPI_FMT_RAW10	0x15

#define C_SDRAM_FMT_8Y4U4V	0x1F
#define C_SDRAM_FMT_VY1UY0	0x20
#define C_FRONT_FMT_UY1VY0	0x21
#define C_FRONT_FMT_Y1VY0U	0x22
#define C_FRONT_FMT_VY1UY0	0x23
#define C_FRONT_FMT_Y1UY0V	0x24
#define C_MIPI_FMT_Y1VY0U	0x25

// dma set
#define RD_A_WR_A		0
#define RD_A_WR_B		1
#define RD_B_WR_B		2
#define RD_B_WR_A		3
#define AUTO_SWITCH		4

// fifo line set
#define LINE_8			0
#define LINE_16			1
#define LINE_32			2
#define LINE_64			3
#define LINE_NONE		4

// YUV mode
#define FMT_8Y4U4V		0
#define FMT_YUYV		1

// rgb path
#define RGB_NONE		0
#define RGB_PATH1		1	//From Lenscmp
#define RGB_PATH2		3	//From WBGain
#define RGB_PATH3		5	//From Lutgamma

// AF window size
#define C_AF256			0
#define C_AF512			1
#define C_AF1024		2
#define C_AF64			3
#define C_AF2048		4

// AE windows size
#define C_AE4			0
#define C_AE8			1
#define C_AE16			2
#define C_AE32			3
#define C_AE64			4
#define C_AE128			5

// Output Format
#define YUYV			0
#define UYVY 			1
#define YVYU 			2
#define	VYUY			3

// Algorithm Select
#define mastumoto		0
#define missing_pixel	1

// DPC RCV Mode
#define default_mode	1
#define second_mode		0

// ISP_Crosstalk removal
#define Gb				1
#define Gr				2
#define GbGr			3

// Input Scal path
#define CDSP_PATH		0
#define	CSI_PATH		1

//front input
#define SENSOR_RAW10	0
#define SENSOR_RAW8		1
#define SENSOR_UYVY		2
#define SENSOR_YVYU		3
#define SENSOR_VYUY		4
#define SENSOR_YUYV		5
//#define MIPI_RAW8		6
//#define MIPI_RAW10	7
#define MIPI_UYVY		8

#define MD_BLK_16X16    0
#define MD_BLK_32X32    1
#define MD_BLK_64X64    2

#define MD_FRAMERATE_1    0
#define MD_FRAMERATE_2    1
#define MD_FRAMERATE_4    2
#define MD_FRAMERATE_8    3

#define BIN_CROP        0x02
#define BIN_EN          0x01
#define BIN_BY2         0x00
#define BIN_BY4         0x04
#define BIN_BYPASS      0x20
#define BIN_AVG         0x00
#define BIN_SKIP        0x10

#define BIN_CLKDIV1     0x00000
#define BIN_CLKDIV2     0x10000
#define BIN_CLKDIV4     0x20000
#define BIN_CLKDIV8     0x30000
#define BIN_CLKDIV16    0x40000

#define AUTO_MODE_SINGLE_NON_STOP	0x0
#define AUTO_MODE_SINGLE_STOP		0x1
#define AUTO_MODE_DUAL_NON_STOP		0x2
#define AUTO_MODE_DUAL_STOP			0x3

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct edge_filter_s
{
	INT8U LPF00;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U LPF01;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U LPF02;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U RSD0;

	INT8U LPF10;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U LPF11;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U LPF12;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U RSD1;

	INT8U LPF20;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U LPF21;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U LPF22;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U RSD2;
} edge_filter_t;

typedef struct color_matrix_s
{
	INT16S CcCof00;		/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S CcCof01;		/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S CcCof02;		/* 1 sign + 3.7 bit, -1024/128~1023/128 */

	INT16S CcCof10;		/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S CcCof11;		/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S CcCof12;		/* 1 sign + 3.7 bit, -1024/128~1023/128 */

	INT16S CcCof20;		/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S CcCof21;		/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16S CcCof22;		/* 1 sign + 3.7 bit, -1024/128~1023/128 */
	INT16U RSD0;
    /*GPA7XXXA*/
	INT16U  col_thr0, col_thr1, col_thr2;
	INT16U  col_thr3, col_thr4, col_thr5, col_thr6;

    INT16U col_scale0, col_scale1, col_scale2;
    INT16U col_scale3, col_scale4, col_scale5;
    INT16U col_scale6, col_scale7;
} color_matrix_t;

typedef struct uv_divide_s
{
	INT8U YT1;			/* 0~0xFF, when Y=T1, UV divide 1/8 */
	INT8U YT2;			/* 0~0xFF, when Y=T2, UV divide 2/8 */
	INT8U YT3;			/* 0~0xFF, when Y=T3, UV divide 3/8 */
	INT8U YT4;			/* 0~0xFF, when Y=T4, UV divide 4/8 */
	INT8U YT5;			/* 0~0xFF, when Y=T5, UV divide 5/8 */
	INT8U YT6;			/* 0~0xFF, when Y=T6, UV divide 6/8 */
	INT8U RSD0;
	INT8U RSD1;
} uv_divide_t;

typedef struct awb_uv_thr_s
{
	INT16S UL1N1;
	INT16S UL1P1;
	INT16S VL1N1;
	INT16S VL1P1;

	INT16S UL1N2;
	INT16S UL1P2;
	INT16S VL1N2;
	INT16S VL1P2;

	INT16S UL1N3;
	INT16S UL1P3;
	INT16S VL1N3;
	INT16S VL1P3;
} awb_uv_thr_t;

typedef struct af_windows_value_s
{
	INT32U h_value_l;
	INT32U h_value_h;
	INT32U v_value_l;
	INT32U v_value_h;
} af_windows_value_t;


typedef struct IspWDRData_s
{
    INT8U wdr_en;
    INT8U win_bg;
    INT8U win_i;
} IspWDRData_t;


typedef struct IspHbNrData_s {
    INT8U NrEn;
    INT8U AjEn;
    INT8U EdgeThr;
    INT8U LumaFilter;
    INT8U ChromaFilter;
    INT8U Clip;
    INT8U AjEdgeThr;
    INT8U reserved;
} IspHbNrData_t;


typedef struct IspHbNrAjData_s
{
    INT8U NrEn; //0x280 b[0]
    INT8U AjEn; //0x284 b[12]

    INT8U LS1; //0x294 b[6:0],LumaStrength1
    INT8U LS2; //0x294 b[14:8]
    INT8U LS3; //0x294 b[22:16]
    INT8U LS4; //0x294 b[30:24]
    INT8U LS5; //0x298 b[6:0],LumaStrength5

    INT8U LT1; //0x284 b[23:16],LumaThreshold 1,2
    INT8U LT2; //0x284 b[31:24]
    INT8U LT3; //0x288 b[23:16],LumaThreshold 3,4
    INT8U LT4; //0x288 b[31:24]

    INT8U LowLightT; //0x298 b[23:16]
    INT8U LowLightS;   //0x298 b[30:24]
    INT8U LowLumaMode; //0x280 b[27:26]
    INT8U EnhanceMode; //0x280 b[29:28]
    INT8U LumaMode; //0x280 b[2:1]
    INT8U CS; //0x298 b[14:8]
    INT8U LumaFilter; //0x280 b[6:4]
    INT8U ChromaFilter; //0x280 b[10:8]
    INT8U Noise_clip; //0x280 b[14:12]
    INT8U AJ_EdgeT; //0x284 b[9:0]

} IspHbNrAjData_t;


typedef struct gpIspRgbGamma_s
{
	INT16U	rgb_gamma_en;	/* 0:disable, 1:enable */
	INT16U	sensor_id;
	INT32U 	*gamma_table;	/* gamma table, 512 byte */
} gpIspRgbGamma_t;


typedef struct IspSharpenData_s
{
    INT8U sharpen_en;
    INT8U sharpen_S;
    INT8U sharpen_th;
    INT8U clip1;
    INT8U clip2;
    INT8U reduce;
    INT8U mask;
    INT8U luma_th;
    INT8U under_luma_reduce;
    INT8U G_th;
    INT8U G_reduce;
    INT8U uvf_en;
    INT16U width;

    INT8U reserved1;
    INT8U reserved2;

} IspSharpenData_t;

/*RGB LUT Table*/
typedef struct ispRGBLucData_s
{
    volatile INT32U RGBLuct[192];
} ispRGBLucData_t;

typedef struct gpIspDN3D_s
{
	INT8U dn_en;
	INT8U dn_bypass;
    INT8U dn_sol;
    INT8U auto_cal_blk;
    INT8U dn_path;

	INT32U dn_w, dn_h;
	INT32U dn_adddr;
	/*Lum Threshold*/
	INT8U	lum_thr0 ;
	INT8U	lum_thr1 ;
	INT8U	lum_thr2 ;
	INT8U	lum_thr3 ;
	INT8U	lum_thr4 ;
	INT8U	lum_thr5 ;
	INT8U	lum_thr6 ;
	INT8U	lum_thr7 ;
	INT8U	lum_thr8 ;
	INT8U	lum_thr9 ;
	INT8U	lum_thr10;
	INT8U	lum_thr11;
	INT8U	lum_thr12;
	INT8U	lum_thr13;
	INT8U	lum_thr14;
	INT8U	lum_thr15;

	/*Max Lum Threshold*/
	INT8U	lum_max_thr0 ;
	INT8U	lum_max_thr1 ;
	INT8U	lum_max_thr2 ;
	INT8U	lum_max_thr3 ;
	INT8U	lum_max_thr4 ;
	INT8U	lum_max_thr5 ;
	INT8U	lum_max_thr6 ;
	INT8U	lum_max_thr7 ;
	INT8U	lum_max_thr8 ;
	INT8U	lum_max_thr9 ;
	INT8U	lum_max_thr10;
	INT8U	lum_max_thr11;
	INT8U	lum_max_thr12;
	INT8U	lum_max_thr13;
	INT8U	lum_max_thr14;
	INT8U	lum_max_thr15;
} gpIspDN3D_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/***************************
    CLOCK path
****************************/
extern INT32S drv_l1_CdspSetClockTree(INT8U enable, INT32U path, INT8U raw_fmt_en);

/***************************
    CDSP ISR & INT Handle
****************************/
extern INT32U drv_l1_CdspGetIntStatus(void);
extern INT32U drv_l1_CdspGetFrontVdIntStatus(void);
extern INT32U drv_l1_CdspGetFrontIntStatus(void);
extern INT32U drv_l1_CdspGetGlbIntStatus(void);

/***************************
       cdsp system
****************************/
extern void drv_l1_CdspReset(void);
extern void drv_l1_CdspSetGatingClk(INT8U enable);
extern void drv_l1_CdspSetRedoTriger(INT8U docdsp);
extern void drv_l1_CdspSetGatingValid(INT8U gating_gamma_vld, INT8U gating_badpixob_vld);
extern void drv_l1_CdspSetDarkSub(INT8U raw_sub_en, INT8U sony_jpg_en);
extern void drv_l1_CdspSetRawDataFormat(INT8U format);
extern void drv_l1_CdspSetYuvRange(INT8U signed_yuv_en);
extern void drv_l1_CdspSetSRAM(INT8U overflowen, INT16U sramthd);
extern void drv_l1_CdspSetSensorFrameMode(int frame);

/***************************
        interrupt
****************************/
extern void drv_l1_CdspSetIntEn(INT8U inten, INT16U bit);
extern void drv_l1_CdspClrIntStatus(INT16U bit);

/***************************
          other
****************************/
extern void drv_l1_CdspSetLineInterval(INT16U line_interval);
extern void drv_l1_CdspSetExtLine(INT16U linesize,INT16U lineblank);

/***************************
         path set
****************************/
extern void drv_l1_CdspSetDataSource(INT8U image_source);
extern void drv_l1_CdspSetRawPath(INT8U raw_mode, INT8U cap_mode, INT8U yuv_mode, INT8U yuv_fmt);
extern void drv_l1_CdspSetRawPath2(INT8U raw_mode, INT8U cap_mode, INT8U yuv_mode);
extern void drv_l1_CdspSetYuvMuxPath(INT8U redoedge);
extern void drv_l1_CdspSetLensCmpPath(INT8U yuvlens);
extern void drv_l1_CdspSetExtLinePath(INT8U extinen, INT8U path);
extern void drv_l1_CdspSetLineCtrl(INT8U ybufen);

/***************************
    cdsp dma buffer set
****************************/
extern void drv_l1_CdspSetYuvBuffA(INT16U width, INT16U height, INT32U buffer_addr);
extern void drv_l1_CdspSetYuvBuffB(INT16U width, INT16U height, INT32U buffer_addr);
extern void drv_l1_CdspSetRawBuffSize(INT16U width, INT16U height, INT16U hoffset, INT8U rawbit);
extern void drv_l1_CdspSetRawBuff(INT32U buffer_addr);
extern void drv_l1_CdspSetReadBackSize(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize);
extern void drv_l1_CdspSetDmaConfig(INT8U buffer_mode);
extern void drv_l1_CdspSetFIFOMode(INT8U frcen, INT8U fifo_en);
extern void drv_l1_CdspSetFifoLine(INT8U line_mode);

/***************************
       clamp set
****************************/
extern void drv_l1_CdspSetClampEn(INT8U clamphsizeen, INT16U clamphsize);
extern void drv_l1_CdspSetPreRBClamp(INT16U pre_rb_clamp);
extern INT32U drv_l1_CdspGetPreRBClamp(void);
extern void drv_l1_CdspSetRBClamp(INT8U rbclampen, INT8U rbclamp);
extern void drv_l1_CdspGetRBClamp(INT8U *rbclampen, INT8U *rbclamp);
extern void drv_l1_CdspSetUvDivideEn(INT8U uvDiven);
extern INT32U gpHalCdspGetUvDivideEn(void);
extern void drv_l1_CdspSetUvDivide(uv_divide_t *pDiv);
extern void drv_l1_CdspGetUvDivide(uv_divide_t *UVDivide);
extern void drv_l1_CdspSetQcntThr(INT16U Qthr, INT16U PreRBclamp);

/***************************
       bad pixel set
****************************/
extern void drv_l1_CdspSetBadPixelEn(INT8U badpixen, INT8U badpixmirrlen);
extern void drv_l1_CdspGetBadPixelEn(INT8U *badpixen, INT8U *badpixmirrlen);
extern void drv_l1_CdspSetBadPixel(INT8U bprthr, INT8U bpgthr, INT8U bpbthr);
extern void drv_l1_CdspGetBadPixel(INT8U *bprthr, INT8U *bpgthr, INT8U *bpbthr);

/***************************
     optical black set
****************************/
extern void drv_l1_CdspSetManuOB(INT8U manuoben, INT16U manuob);
extern void drv_l1_CdspGetManuOB(INT8U *manuoben, INT16U *manuob);
extern void drv_l1_CdspSetAutoOB(INT8U autooben, INT8U obtype, INT8U curfob);
extern void drv_l1_CdspGetAutoOB(INT8U *autooben, INT8U *obtype, INT16U *obHOffset, INT16U *obVOffset);
extern void drv_l1_CdspSetOBHVoffset(INT16U obHOffset, INT16U obVOffset);
extern void drv_l1_CdspGetAutoOBAvg(INT16U *ob_avg_r, INT16U *ob_avg_gr, INT16U *ob_avg_b, INT16U *ob_avg_gb);

/***************************
     lens compensation
****************************/
extern void drv_l1_CdspSetLensCmpTable(INT16U *plensdata);
extern void drv_l1_CdspSetLensCmpEn(INT8U lcen);
extern INT32U drv_l1_CdspGetLensCmpEn(void);
extern void drv_l1_CdspSetLensCmpPos(INT16U centx, INT16U centy, INT16U xmoffset, INT16U ymoffset);
extern void drv_l1_CdspGetLensCmpPos(INT16U *centx, INT16U *centy, INT16U *xmoffset, INT16U *ymoffset);
extern void drv_l1_CdspSetLensCmp(INT8U stepfactor, INT8U xminc, INT8U ymoinc, INT8U ymeinc);
extern void drv_l1_CdspGetLensCmp(INT8U *stepfactor, INT8U *xminc, INT8U *ymoinc, INT8U *ymeinc);

/***************************
    New ISP Macro Table
****************************/
extern void drv_l1_IspSetLiCor(INT8U *plicordata);
extern void drv_l1_IspSetHr0(INT32U value);
extern void drv_l1_IspSetHr1(INT32U value);
extern void drv_l1_IspSetHr2(INT32U value);
extern void drv_l1_IspSetHr3(INT32U value);
extern void drv_l1_IspSetLucMaxTan8SlopCLP(INT16U *pmaxtan8data, INT16U *pslopdata, INT16U *pclpdata);
extern void drv_l1_IspSetRadusF0(INT16U *pradusf0data);
extern void drv_l1_IspSetRadusF1(INT16U *pradusf1data);
extern void drv_l1_IspSetLenCmp(INT32U lcen,INT32U stepfactor);

/***************************
      raw h scaler
****************************/
extern void drv_l1_CdspSetRawHScaleEn(INT8U hscale_en, INT8U hscale_mode);
extern void drv_l1_CdspSetRawHScale(INT32U src_hsize, INT32U dst_hsize);

/***************************
     white balance set
****************************/
extern void drv_l1_CdspSetWbGainEn(INT8U wbgainen);
extern INT8U drv_l1_CdspGetWbGainEn(void);
extern void drv_l1_CdspSetWbGain(INT16U r_gain, INT16U g_gain, INT16U b_gain);
extern void drv_l1_CdspSetWb_RB_Gain(INT16U r_gain, INT16U b_gain);
extern void drv_l1_CdspSetWbOffsetEn(INT8U wboffseten);
extern void drv_l1_CdspSetWbOffset(INT8U roffset, INT8U groffset, INT8U boffset, INT8U gboffset);
extern INT32U drv_l1_CdspGetWbGain(INT16U *prgain, INT16U *pgrgain, INT16U *pbgain, INT16U *pgbgain);
extern void drv_l1_CdspGetWbOffset(INT8U *pwboffseten, INT8S *proffset, INT8S *pgroffset, INT8S *pboffset,	INT8S *pgboffset);

/***************************
     global gain
****************************/
extern void drv_l1_CdspSetGlobalGain(INT8U global_gain);
extern INT32U drv_l1_CdspGetGlobalGain(void);

/***************************
          gain2
****************************/
extern void drv_l1_CdspSetWbGain2En(INT8U wbgain2en);
extern void drv_l1_CdspSetWbGain2(INT16U rgain2, INT16U ggain2, INT16U bgain2);
extern void drv_l1_CdspGetWbGain2(INT8U *pwbgain2en, INT16U *prgain, INT16U *pggain, INT16U *pbgain);
void drv_l1_CdspSetWb_RB_Gain2(INT16U rgain2, INT16U bgain2);

/***************************
         3dnr set
****************************/
extern void drv_l1_IspSet3dnr(gpIspDN3D_t *argp);
extern void  drv_l1_IspSet3dnr_thr(INT8U *d3nr_thr);
extern void drv_l1_IspSet3dnr_ctrl(gpIspDN3D_t *argp);
extern INT8S drv_l1_IspSet3dnr_en(INT8U en);
extern INT8U drv_l1_IspGet3dnr_en(void);
extern void drv_l1_IspSet3dnr_size(INT32U w, INT32U h);


/***************************
         gamma set
****************************/
extern void drv_l1_CdspSetGammaTable(INT32U *pGammaTable);
extern void drv_l1_CdspSetRawGammaEn(INT8U lut_gamma_en);
extern INT32U drv_l1_CdspGetLutGammaEn(void);

/***************************
      interpolation set
****************************/
extern void drv_l1_CdspSetIntplMirEn(INT8U intplmirhlen, INT8U intplmirvsel, INT8U intplcnt2sel);
extern void drv_l1_CdspSetIntplThr(INT16U int_low_thr, INT16U int_hi_thr);
extern void drv_l1_CdspGetIntplThr(INT16U *int_low_thr, INT16U *int_hi_thr);
extern void drv_l1_CdspSetIntplMode(INT8U mode);

/***************************
  edge in intpolation set
****************************/
extern void drv_l1_CdspSetEdgeEn(INT8U edgeen);
extern INT32U drv_l1_CdspGetEdgeEn(void);
extern void drv_l1_CdspSetEdgeFilter(edge_filter_t *pEdgeFilter);
extern void drv_l1_CdspGetEdgeFilter(edge_filter_t *pEdgeFilter);
extern void drv_l1_CdspSetEdgeLCoring(INT8U lhdiv, INT8U lhtdiv, INT8U lhcoring,INT8U lhmode);
extern void drv_l1_CdspGetEdgeLCoring(INT8U *lhdiv, INT8U *lhtdiv, INT8U *lhcoring, INT8U *lhmode);
extern void drv_l1_CdspSetEdgeAmpga(INT8U ampga);
extern INT32U drv_l1_CdspGetEdgeAmpga(void);
extern void drv_l1_CdspSetEdgeDomain(INT8U edgedomain);
extern INT32U drv_l1_CdspGetEdgeDomain(void);
extern void drv_l1_CdspSetEdgeQthr(INT8U Qthr);
extern INT32U drv_l1_CdspGetEdgeQCnt(void);

/***************************
    edge lut table set
****************************/
extern void drv_l1_CdspSetEdgeLutTable(INT8U *pLutEdgeTable);
extern void drv_l1_CdspSetEdgeLutTableEn(INT8U eluten);
extern INT32U drv_l1_CdspGetEdgeLutTableEn(void);

/***************************
    color matrix set
****************************/
extern void drv_l1_CdspSetColorMatrixEn(INT8U colcorren);
extern INT32U drv_l1_CdspGetColorMatrixEn(void);
extern void drv_l1_CdspSetColorMatrix(color_matrix_t *pCM);
void drv_l1_CdspSetColorMatrixArray(INT16S *pCM);
extern void drv_l1_CdspGetColorMatrix(color_matrix_t *pCM);
void drv_l1_CdspSetColorMatrix_Scale(color_matrix_t *pCM);
void drv_l1_CdspSetColorMatrix_Thr(color_matrix_t *pCM);
void drv_l1_CdspSetColorMatrix_a(INT16S *pCM);

/***************************
    raw special mode set
****************************/
extern void drv_l1_CdspSetRawSpecMode(INT8U rawspecmode);
extern INT32U drv_l1_CdspGetRawSpecMode(void);

/***************************
  YUV insert & coring set
****************************/
extern void drv_l1_CdspSetYuv444InsertEn(INT8U yuvinserten);
extern INT32U drv_l1_CdspGetYuv444InsertEn(void);
extern void drv_l1_CdspSetYuvCoring(INT8U y_corval_coring, INT8U u_corval_coring, INT8U v_corval_coring);
extern void drv_l1_CdspGetYuvCoring(INT8U *y_corval_coring, INT8U *u_corval_coring, INT8U *v_corval_coring);

/***************************
       crop set
****************************/
extern void drv_l1_CdspSetCropEn(INT8U hvEnable);
extern void drv_l1_CdspSetCrop(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize);
extern void drv_l1_CdspGetCrop(INT8U *hvEnable, INT32U *hoffset, INT32U *voffset, INT32U *hsize, INT32U *vsize);

/***************************
       YUV havg set
****************************/
extern void drv_l1_CdspSetYuvHAvg(INT8U yuvhavgmiren, INT8U ytype, INT8U utype, INT8U vtype);
extern void drv_l1_CdspGetYuvHAvg(INT8U *yuvhavgmiren, INT8U *ytype, INT8U *utype, INT8U *vtype);

/***************************
    YUV special mode set
****************************/
extern void drv_l1_CdspSetYuvSpecMode(INT8U yuvspecmode);
extern INT32U drv_l1_CdspGetYuvSpecMode(void);
extern void drv_l1_CdspSetBinarizeModeThr(INT8U binarthr);
extern INT32U drv_l1_CdspGetBinarizeModeThr(void);

/***************************
 Y brightness & contrast set
****************************/
extern void drv_l1_CdspSetBriCont(INT8U YbYcEn, INT32U yb,INT32U yc);
extern void drv_l1_CdspSetBriContEn(INT8U YbYcEn);
extern INT32U drv_l1_CdspGetBriContEn(void);
extern void drv_l1_CdspSetYuvSPEffOffset(INT8S u_offset, INT8S v_offset);
extern void drv_l1_CdspGetYuvSPEffOffset(INT8S *u_offset, INT8S *v_offset);
extern void drv_l1_CdspSetYuvSPEffScale(INT8U u_scale, INT8U v_scale);
extern void drv_l1_CdspGetYuvSPEffScale(INT8U *u_scale, INT8U *v_scale);



/***************************
   UV saturation & hue set
****************************/
extern void drv_l1_CdspSetSatHue(INT32U uv_sat_scale, INT32U uoffset, INT32U voffset,
						  		 INT32U u_huesindata, INT32U u_huecosdata, INT32U v_huesindata, INT32U v_huecosdata);
extern void drv_l1_CdspSetYuvSPHue(INT16U u_huesindata, INT16U u_huecosdata, INT16U v_huesindata, INT16U v_huecosdata);
extern void drv_l1_CdspGetYuvSPHue(INT8S *u_huesindata, INT8S *u_huecosdata,	INT8S *v_huesindata, INT8S *v_huecosdata);
void drv_l1_CdspSetContrastBypass(int bypass);


/***************************
      yuv h & v scale
****************************/
extern void drv_l1_CdspSetYuvHScaleEn(INT8U yuvhscale_en, INT8U yuvhscale_mode);
extern void drv_l1_CdspSetYuvVScaleEn(INT8U vscale_en, INT8U vscale_mode);
extern void drv_l1_CdspSetYuvHScale(INT16U hscaleaccinit, INT16U yuvhscalefactor);
extern void drv_l1_CdspSetYuvVScale(INT16U vscaleaccinit, INT16U yuvvscalefactor);

/***************************
      Suppression set
****************************/
extern void drv_l1_CdspSetUvSupprEn(INT8U suppressen);
extern void drv_l1_CdspSetUvSuppr(INT8U yuvsupmirvsel, INT8U fstextsolen, INT8U yuvsupmiren);

/***************************
Y edge in uv suppression set
****************************/
extern void drv_l1_CdspSetEdgeSrc(INT8U posyedgeen);
extern INT32U drv_l1_CdspGetEdgeSrc(void);

/***************************
Y denoise in suppression set
****************************/
extern void drv_l1_CdspSetYDenoiseEn(INT8U denoisen);
extern INT32U drv_l1_CdspGetYDenoiseEn(void);
extern void drv_l1_CdspSetYDenoise(INT8U denoisethrl,	INT8U denoisethrwth, INT8U yhtdiv);
extern void drv_l1_CdspGetYDenoise(INT8U *denoisethrl,	INT8U *denoisethrwth, INT8U *yhtdiv);

/***************************
  Y LPF in suppression set
****************************/
extern void drv_l1_CdspSetYLPFEn(INT8U lowyen);
extern INT32U drv_l1_CdspGetYLPFEn(void);

/***************************
        New ISP
****************************/
extern void drv_l1_IspSetImageSize(INT16U hsize, INT16U vsize);
extern void drv_l1_CdspSetLinCorrTable(INT8U *rgb_tbl);
extern void drv_l1_IspSetLinCorrEn(INT8U lincorr_en);
extern void drv_l1_IspSetDbpcSel(INT8U algorithm_sel);
extern void drv_l1_IspSetDpcEn(INT8U dpc_en);
extern void drv_l1_IspSetDpcRcvModeSelThr(INT8U sel_mode, INT16U DPCth1, INT16U DPCth2, INT16U DPCth3);
extern void drv_l1_IspSetSmoothFactor(INT16U DPCn);
extern void drv_l1_IspSetCrostlkGbGrEn(INT8U ctk_en, INT8U ctk_gbgr);
extern void drv_l1_IspSetCrostlkThold(INT16U ctk_thr1, INT16U ctk_thr2, INT16U ctk_thr3, INT16U ctk_thr4);
extern void drv_l1_IspSetCrostlkWeight(INT16U ctkw1, INT16U ctkw2, INT16U ctkw3);
extern void drv_l1_IspSetDenoiseEn(INT8U denoise_en);
extern void drv_l1_IspSetDenoiseThold(INT16U rdn_thr1,INT16U rdn_thr2,INT16U rdn_thr3,INT16U rdn_thr4);
extern void drv_l1_IspSetDenoiseWeight(INT16U rdnw1, INT16U rdnw2, INT16U rdnw3);
extern void drv_l1_CdspSetNewDenoiseEn(INT8U newdenoiseen);
extern INT32U drv_l1_CdspGetNewDenoiseEn(void);
extern void drv_l1_CdspSetNewDenoise(INT8U ndmirvsel, INT8U ndmiren);
extern void drv_l1_CdspGetNewDenoise(INT8U *ndmirvsel, INT8U *ndmiren);
extern void drv_l1_CdspSetNdEdgeEn(INT8U ndedgeen,INT8U ndeluten);
extern void drv_l1_CdspGetNdEdgeEn(INT8U *ndedgeen, INT8U *ndeluten);
extern void drv_l1_CdspSetNdEdgeFilter(edge_filter_t *pNdEdgeFilter);
extern void drv_l1_CdspGetNdEdgeFilter(edge_filter_t *pNdEdgeFilter);
extern void drv_l1_CdspSetNdEdgeLCoring(INT8U ndlhdiv, INT8U ndlhtdiv, INT8U ndlhcoring);
extern void drv_l1_CdspGetNdEdgeLCoring(INT8U *ndlhdiv, INT8U *ndlhtdiv, INT8U *ndlhcoring);
extern void drv_l1_CdspSetNdEdgeAmpga(INT8U ndampga);
extern INT32U drv_l1_CdspGetNdEdgeAmpga(void);

/***************************
   auto focuse / af set
****************************/
extern void drv_l1_IspSetAFEn(INT8U af_en, INT8U af_win_hold);
extern void drv_l1_IspGetAFEn(INT8U *af_en, INT8U *af_win_hold);
extern void drv_l1_IspSetAfWin1(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize);
extern void drv_l1_IspSetAfWin2(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize);
extern void drv_l1_IspSetAfWin3(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize);
extern INT32S drv_l1_CdspGetAFWinVlaue(INT8U windows_no, af_windows_value_t *af_value);

/***************************
auto white balance / awb set
****************************/
extern void drv_l1_CdspSetAWBEn(INT8U awb_win_en, INT8U awb_win_hold);
extern void drv_l1_CdspSetAWB(INT8U awbclamp_en, INT8U sindata, INT8U cosdata, INT8U awbwinthr);
extern void drv_l1_CdspSetAwbYThr(INT8U Ythr0, INT8U Ythr1, INT8U Ythr2, INT8U Ythr3);
extern void drv_l1_CdspSetAwbUVThr(awb_uv_thr_t *UVthr);

/***************************
   auto expore / ae set
****************************/
extern void drv_l1_CdspSetAeAwbSrc(INT8U raw_en);
extern INT32U drv_l1_CdspGetAeAwbSrc(void);
extern void drv_l1_CdspSetAeAwbSubSample(INT8U subample);
extern INT32U drv_l1_CdspGetAeAwbSubSample(void);
extern void drv_l1_CdspSetAEEn(INT8U ae_win_en, INT8U ae_win_hold);
extern void drv_l1_CdspGetAEEn(INT8U *ae_win_en, INT8U *ae_win_hold);
extern INT32U drv_l1_CdspGetAEActBuff(void);
extern void drv_l1_CdspSetAEWin(INT8U phaccfactor, INT8U pvaccfactor);
extern void drv_l1_CdspSetAEBuffAddr(INT32U winaddra,INT32U winaddrb);
extern void drv_l1_CdspSetRGBWin(INT16U hwdoffset, INT16U vwdoffset, INT16U hwdsize, INT16U vwdsize);
extern void drv_l1_CdspGetRGBWin(INT16U *hwdoffset, INT16U *vwdoffset, INT16U *hwdsize, INT16U *vwdsize);
extern void drv_l1_CdspSet3ATestWinEn(INT8U AeWinTest, INT8U AfWinTest);
extern void drv_l1_CdspSetAeBpWp_TH(INT16U wpthr, INT16U bpthr);
extern void drv_l1_CdspGetAeBpWp_TH(INT16U *wpthr, INT16U *bpthr);
extern void drv_l1_CdspSetAWBWin_GDivR_TH(INT16U thr1, INT16U thr2);
extern void drv_l1_CdspGetAWBWin_GDivR_TH(INT16U *thr1, INT16U *thr2);


/***************************
       histgm set
****************************/
extern void drv_l1_CdspSetHistgmEn(INT8U his_en, INT8U his_hold_en);
extern void drv_l1_CdspGetHistgm(INT32U *table_val,INT32U len);

/***************************
       awb sum set
****************************/
extern INT32S drv_l1_CdspGetAwbSumCnt(INT8U section, INT32U *sumcnt);
extern INT32S drv_l1_IspGetAwbSumR(INT8U section, INT32U *sumcnt);
extern INT32S drv_l1_IspGetAwbSumG(INT8U section, INT32U *sumcnt);
extern INT32S drv_l1_IspGetAwbSumB(INT8U section, INT32U *sumcnt);
extern INT32S drv_l1_IspGetAwbSumRdG(INT8U section, INT32U *sumcnt);
extern INT32S drv_l1_IspGetAwbSumBdG(INT8U section, INT32U *sumcnt);

/***************************
   Motion Detection set
****************************/
extern void drv_l1_CdspSetMDEn(INT8U enable);
extern void drv_l1_CdspSetMD(INT8U enable, INT8U threshold, INT16U width, INT32U working_buf, INT8U block_size, INT8U update_frame);
extern INT32U drv_l1_CdspGetResult(void);
extern void drv_l1_CdspSetMD_IRQ_Trigger(INT16U threshold);



/***************************
       Sharpen
****************************/
void drv_l1_CdspSetSpEn(INT8U sp_en, INT8U uvf_en);
void drv_l1_CdspSetSpCtrl(INT8U mask, INT8U reduce, INT16U width);
void drv_l1_CdspSetSpLevel(INT8U clip1, INT8U clip2, INT8U sharpenS, INT8U sharpen_Th);
void drv_l1_CdspSetSharpenS(INT8U sharpenS);
void drv_l1_CdspSetSharpenS_bypass(INT8U sharpenS);
void drv_l1_CdspSetSp_Table(INT32U *pSPTable);
void drv_l1_IspSetSp_G_Reduce(INT8U Gluma, INT8U reduce);
void drv_l1_IspSetSpLumaReduce(INT8U luma, INT8U reduce);


/***************************
       Denoise
****************************/
void drv_l1_CdspSetHbNrAJ_en_thr(INT8U aj_en, INT16U ajedge_t);
void drv_l1_CdspSetHbNrAJ_en(INT8U aj_en);
void drv_l1_CdspSetHbNrEdge_thr(INT16U edge_t);
void drv_l1_CdspSetHbNrYnoise_clip(INT16U clip_y_val);
void drv_l1_CdspSetHbNrFilter_size(INT8U luma_size, INT8U chroma_size);
void drv_l1_CdspSetHbNrEn(INT8U nr_en);
void drv_l1_CdspSetHbNrEn_bypass(INT8U nr_en);
void drv_l1_CdspSetHbNr_Table(INT32U *pNrTable);

/***************************
       WDR
****************************/
void drv_l1_CdspSetWDREn(INT8U wdr_en);
void drv_l1_CdspSetWDRUVEn(INT8U uv_en);
void drv_l1_CdspSetWDRUVLevl(INT8U chro_fact);
void drv_l1_CdspSetWDRBndMod(INT8U bnd_case);
void drv_l1_CdspSetWDRWinPix_Bckgd(INT8U win_i, INT8U win_bg);
void drv_l1_CdspSetWDR_Table(INT32U *pWDRTable, INT32U table_sel);
void drv_l1_CdspSetWDRShadowEn(INT8U en);



/***************************
       AWB
****************************/
void drv_l1_IspSetAwbEn(INT8U awb_en);
void drv_l1_IspSetAwbHVidx(INT16U h_start_idx, INT16U h_end_idx, INT16U v_start_idx, INT16U v_end_idx);

void drv_l1_CdspSetContrast_Table(INT32U *pYTable);

/***************************
       DPC
****************************/
/**
 * @brief  set bad pixel correction threshold
 * @thr1:  small value: weak correction,  large value: strong correction
 * @thr2:  small value: strong correction,  large value: weak correction
 * @thr3: 0(strong) ~ 4(weak)
 * @thr4: 0(strong) ~ 3(weak)
 */
void drv_l1_CdspSetDpcThr(INT16U DPCth1, INT16U DPCth2, INT16U DPCth3, INT16U DPCth4);



/***************************
       Read table
****************************/
#define DRV_L1_READ_NR_TABLE    0x1D1D
#define DRV_L1_READ_SP_TABLE    0x1E1E
void drv_l1_CdspRead_Table(INT32U *table_val, INT32U len, INT16U macro_ctrl);

extern void drv_l1_CdspSetYuvPscalePath(INT8U onPscale);
extern void drv_l1_Cdsp_aeawb_enable(INT8U enable);

void drv_l1_CdspSetSpImageSize(INT16U width);
extern void drv_l1_IspSetSYSFIFOEn(INT8U sysfifo_en);
extern void drv_l1_CdspSetAutoMode(INT8U auto_en, INT8U auto_mode, INT16U fifo_line, INT8U fifo_num);
extern void drv_l1_CdspSetAutoBufOut(INT8U idx, INT32U address);
extern void drv_l1_CdspGetAutoBufOut(INT8U idx, INT32U* address);
extern void drv_l1_CdspAutoFifoStop(INT8U stop);
extern INT8U drv_l1_CdspGetAutoEnable(void);

// Mesh LSC
void drv_l1_IspSetMeshLSCEn(INT8U lscen);
void drv_l1_ISPSetMeshLSCTblSw(INT8U update);
void drv_l1_IspSetLSC_Grid_Table(INT32U *pNlmTable);
void drv_l1_IspSetLSC_Gain_Table_A(INT32U *pNlmTable);
void drv_l1_IspSetLSC_Gain_Table_B(INT32U *pNlmTable);
INT8U drv_l1_IspGetMeshLSCEn(void);

// NLM
void drv_l1_IspSetNLM_En(INT8U enable);
void drv_l1_IspSetNLM_GValue(INT16U thrshold);
void drv_l1_IspSetNLM_Size(INT32U nlmWidth, INT32U nlmHeight);
INT32U drv_l1_IspGetNLM_HSize(void);
void drv_l1_IspSetNLM_Table(INT8U *pNlmTable);
void drv_l1_CdspGetNLM_GP6_WT345(INT8U *wt3, INT8U *wt4, INT8U *wt5);
void drv_l1_IspSetNLM_GP6_WT345(INT8U wt3, INT8U wt4, INT8U wt5);
void drv_l1_CdspGetNLM_GP6_WT012(INT8U *wt0, INT8U *wt1, INT8U *wt2);
void drv_l1_IspSetNLM_GP6_WT012(INT8U wt0, INT8U wt1, INT8U wt2);
void drv_l1_CdspGetNLM_GP6_TH345(INT16U *thr3, INT16U *thr4, INT16U *thr5);
void drv_l1_IspSetNLM_GP6_TH345(INT16U thr3, INT16U thr4, INT16U thr5);
void drv_l1_CdspGetNLM_GP6_TH012(INT16U *thr0, INT16U *thr1, INT16U *thr2);
void drv_l1_IspSetNLM_GP6_TH012(INT16U thr0, INT16U thr1, INT16U thr2);
void drv_l1_CdspGetNLM_GP5_WT345(INT8U *wt3, INT8U *wt4, INT8U *wt5);
void drv_l1_IspSetNLM_GP5_WT345(INT8U wt3, INT8U wt4, INT8U wt5);
void drv_l1_CdspGetNLM_GP5_WT012(INT8U *wt0, INT8U *wt1, INT8U *wt2);
void drv_l1_IspSetNLM_GP5_WT012(INT8U wt0, INT8U wt1, INT8U wt2);
void drv_l1_CdspGetNLM_GP5_TH345(INT16U *thr3, INT16U *thr4, INT16U *thr5);
void drv_l1_IspSetNLM_GP5_TH345(INT16U thr3, INT16U thr4, INT16U thr5);
void drv_l1_CdspGetNLM_GP5_TH012(INT16U *thr0, INT16U *thr1, INT16U *thr2);
void drv_l1_IspSetNLM_GP5_TH012(INT16U thr0, INT16U thr1, INT16U thr2);

void drv_l1_CdspGetNLM_GP4_WT345(INT8U *wt3, INT8U *wt4, INT8U *wt5);
void drv_l1_IspSetNLM_GP4_WT345(INT8U wt3, INT8U wt4, INT8U wt5);
void drv_l1_CdspGetNLM_GP4_WT012(INT8U *wt0, INT8U *wt1, INT8U *wt2);
void drv_l1_IspSetNLM_GP4_WT012(INT8U wt0, INT8U wt1, INT8U wt2);
void drv_l1_CdspGetNLM_GP4_TH345(INT16U *thr3, INT16U *thr4, INT16U *thr5);
void drv_l1_IspSetNLM_GP4_TH345(INT16U thr3, INT16U thr4, INT16U thr5);
void drv_l1_CdspGetNLM_GP4_TH012(INT16U *thr0, INT16U *thr1, INT16U *thr2);
void drv_l1_IspSetNLM_GP4_TH012(INT16U thr0, INT16U thr1, INT16U thr2);

void drv_l1_CdspGetNLM_GP3_WT345(INT8U *wt3, INT8U *wt4, INT8U *wt5);
void drv_l1_IspSetNLM_GP3_WT345(INT8U wt3, INT8U wt4, INT8U wt5);
void drv_l1_CdspGetNLM_GP3_WT012(INT8U *wt0, INT8U *wt1, INT8U *wt2);
void drv_l1_IspSetNLM_GP3_WT012(INT8U wt0, INT8U wt1, INT8U wt2);
void drv_l1_CdspGetNLM_GP3_TH345(INT16U *thr3, INT16U *thr4, INT16U *thr5);
void drv_l1_IspSetNLM_GP3_TH345(INT16U thr3, INT16U thr4, INT16U thr5);
void drv_l1_CdspGetNLM_GP3_TH012(INT16U *thr0, INT16U *thr1, INT16U *thr2);
void drv_l1_IspSetNLM_GP3_TH012(INT16U thr0, INT16U thr1, INT16U thr2);

void drv_l1_CdspGetNLM_GP2_WT345(INT8U *wt3, INT8U *wt4, INT8U *wt5);
void drv_l1_IspSetNLM_GP2_WT345(INT8U wt3, INT8U wt4, INT8U wt5);
void drv_l1_CdspGetNLM_GP2_WT012(INT8U *wt0, INT8U *wt1, INT8U *wt2);
void drv_l1_IspSetNLM_GP2_WT012(INT8U wt0, INT8U wt1, INT8U wt2);
void drv_l1_CdspGetNLM_GP2_TH345(INT16U *thr3, INT16U *thr4, INT16U *thr5);
void drv_l1_IspSetNLM_GP2_TH345(INT16U thr3, INT16U thr4, INT16U thr5);
void drv_l1_CdspGetNLM_GP2_TH012(INT16U *thr0, INT16U *thr1, INT16U *thr2);
void drv_l1_IspSetNLM_GP2_TH012(INT16U thr0, INT16U thr1, INT16U thr2);

void drv_l1_CdspGetNLM_GP1_WT345(INT8U *wt3, INT8U *wt4, INT8U *wt5);
void drv_l1_IspSetNLM_GP1_WT345(INT8U wt3, INT8U wt4, INT8U wt5);
void drv_l1_CdspGetNLM_GP1_WT012(INT8U *wt0, INT8U *wt1, INT8U *wt2);
void drv_l1_IspSetNLM_GP1_WT012(INT8U wt0, INT8U wt1, INT8U wt2);
void drv_l1_CdspGetNLM_GP1_TH345(INT16U *thr3, INT16U *thr4, INT16U *thr5);
void drv_l1_IspSetNLM_GP1_TH345(INT16U thr3, INT16U thr4, INT16U thr5);
void drv_l1_CdspGetNLM_GP1_TH012(INT16U *thr0, INT16U *thr1, INT16U *thr2);
void drv_l1_IspSetNLM_GP1_TH012(INT16U thr0, INT16U thr1, INT16U thr2);


void drv_l1_CdspGetNLM_GP0_WT345(INT8U *wt3, INT8U *wt4, INT8U *wt5);
void drv_l1_IspSetNLM_GP0_WT345(INT8U wt3, INT8U wt4, INT8U wt5);
void drv_l1_CdspGetNLM_GP0_WT012(INT8U *wt0, INT8U *wt1, INT8U *wt2);
void drv_l1_IspSetNLM_GP0_WT012(INT8U wt0, INT8U wt1, INT8U wt2);
void drv_l1_CdspGetNLM_GP0_TH345(INT16U *thr3, INT16U *thr4, INT16U *thr5);
void drv_l1_IspSetNLM_GP0_TH345(INT16U thr3, INT16U thr4, INT16U thr5);
void drv_l1_CdspGetNLM_GP0_TH012(INT16U *thr0, INT16U *thr1, INT16U *thr2);
void drv_l1_IspSetNLM_GP0_TH012(INT16U thr0, INT16U thr1, INT16U thr2);

// NEW AWB
void drv_l1_IspSetAwbMode(INT8U awb_en, INT8U y_mode, INT8U subsample);
void drv_l1_IspSetAwbThr1617_2(INT16U thr16, INT16U thr17);
void drv_l1_IspSetAwbThr1415_2(INT16U thr14, INT16U thr15);
void drv_l1_IspSetAwbThr1617(INT16U thr16, INT16U thr17);
void drv_l1_IspSetAwbThr1415(INT16U thr14, INT16U thr15);
void drv_l1_IspGetAwbPxLv_2(INT32U *pxlv, INT32U *rglv, INT32U *bglv);
void drv_l1_IspGetAwbPxLv(INT32U *px, INT32U *pxlv, INT32U *rglv, INT32U *bglv);
void drv_l1_IspGetAwbRGBG(INT32U *rg, INT32U *bg);
void drv_l1_IspGetAwbRGB(INT32U *r, INT32U *g, INT32U *b);
void drv_l1_IspSetAwbThr11cd_2(INT32U thr11, INT32U thr12, INT16U thr13);
void drv_l1_IspSetAwbThr789a_2(INT16U thr7, INT16U thr8, INT16U thr9, INT16U thr10);
void drv_l1_IspSetAwbThr3456_2(INT16U thr3, INT16U thr4, INT16S thr5, INT16S thr6);
void drv_l1_IspSetAwbThr12_2(INT8U thr1, INT8U thr2);
void drv_l1_IspSetAwbThr11cd(INT32U thr11, INT32U thr12, INT16U thr13);
void drv_l1_IspSetAwbThr789a(INT16U thr7, INT16U thr8, INT16U thr9, INT16U thr10);
void drv_l1_IspSetAwbThr3456(INT16U thr3, INT16U thr4, INT16S thr5, INT16S thr6);
void drv_l1_IspSetAwbThr12(INT8U thr1, INT8U thr2);
void drv_l1_IspSetAwbMode(INT8U awb_en, INT8U y_mode, INT8U subsample);
void drv_l1_IspSetAwbEn(INT8U awb_en);

// rgb/Y Histogram
void drv_l1_IspGet_Y_Histgm(INT32U *table_val,INT32U len);
void drv_l1_CdspGetHistgmEn(INT8U *his_en, INT8U *his_hold_en);
void drv_l1_IspSetHistgm_RGBpath(INT8U shift_val);
void drv_l1_IspSetHistgm_HWin(INT16U start_idx, INT16U end_idx);
void drv_l1_IspSetHistgm_VWin(INT16U start_idx, INT16U end_idx);
void drv_l1_IspSetHistgm_ShiftBit(INT8U path_val);
void drv_l1_IspSetRGBAWBWin(INT16U hwdoffset, INT16U vwdoffset, INT16U hwdsize, INT16U vwdsize);

// AF
void drv_l1_IspSetAF_GammaEn(INT8U af_gamma_en);
void drv_l1_IspSetAf_Gamma_Th23(INT16U gamma_th2, INT16U gamma_th3);
void drv_l1_IspSetAf_Gamma_Th01(INT16U gamma_th0, INT16U gamma_th1);
void drv_l1_IspSetAf_Gamma_B34(INT16U gamma_b3, INT16U gamma_b4);
void drv_l1_IspSetAf_Gamma_B012(INT16U gamma_b0, INT16U gamma_b1, INT16U gamma_b2);
void drv_l1_IspSetAf_Gamma_A234(INT16U gamma_a2, INT16U gamma_a3, INT16U gamma_a4);
void drv_l1_IspSetAf_Gamma_A01(INT16U gamma_a0, INT16U gamma_a1);

// AWB
void drv_l1_IspSetAwbSumBdGThr1516(INT16U thr15,  INT16U thr16);
void drv_l1_IspSetAwbSumBdGThr121314(INT16U thr12,  INT16U thr13,  INT16U thr14);
void drv_l1_IspSetAwbSumBdGThr91011(INT16U thr9,  INT16U thr10,  INT16U thr11);
void drv_l1_IspSetAwbSumBdGThr678(INT16U thr6,  INT16U thr7,  INT16U thr8);
void drv_l1_IspSetAwbSumBdGThr345(INT16U thr3,  INT16U thr4,  INT16U thr5);
void drv_l1_IspSetAwbSumBdGThr12(INT16U thr1,  INT16U thr2);

void drv_l1_IspSetAwbSumRdGThr13_16(INT16U thr13,  INT16U thr14,  INT16U thr15, INT16U thr16);
void drv_l1_IspSetAwbSumRdGThr101112(INT16U thr10,  INT16U thr11,  INT16U thr12);
void drv_l1_IspSetAwbSumRdGThr789(INT16U thr7,  INT16U thr8,  INT16U thr9);
void drv_l1_IspSetAwbSumRdGThr456(INT16U thr4,  INT16U thr5,  INT16U thr6);
void drv_l1_IspSetAwbSumRdGThr123(INT16U thr1,  INT16U thr2,  INT16U thr3);
void drv_l1_IspSetAwbSumY_Thr(INT8U thr0,  INT8U thr1,  INT8U Gthr);


// RGB gamma
void drv_l2_IspGetRgbGamma(gpIspRgbGamma_t *argp);
void drv_l1_IspSetRgbGammaEn(INT8U rgb_gamma_en);
void drv_l1_IspSetRgbGammaBypass(int bypass);
void drv_l1_IspSetRgbGammaTable(INT32U *pGammaTable);
INT32U drv_l1_IspGetRgbGammaEn(void);



// Hybrid
void drv_l1_IspSetDpcThr(INT8U DPCth1_1, INT8U DPCth1_2, INT8U DPCth2, INT8U DPCth3);
void drv_l1_CdspSetDenoiseWeight(INT16U rdnw1,INT16U rdnw2,INT16U rdnw3,INT16U rdnw4,INT16U rdnw5);
void drv_l1_CdspSetDenoiseEn(INT8U denoise_en);
void drv_l1_CdspSetDenoiseThold(INT16U rdn_thr1,INT16U rdn_thr2,INT16U rdn_thr3,INT16U rdn_thr4);;
void drv_l1_CdspSetCrostlkGbGrEn(INT8U ctk_en, INT8U ctk_gbgr);
void drv_l1_CdspSetCrostlkWeight(INT16U ctkw1, INT16U ctkw2, INT16U ctkw3);
void drv_l1_CdspSetCrostlkThold(INT16U ctk_thr1, INT16U ctk_thr2, INT16U ctk_thr3, INT16U ctk_thr4);
void drv_l1_CdspSetHybridImageSize(INT16U hsize, INT16U vsize);


// HBNR
void drv_l1_IspSetHbNrEnhance(INT16U ehm);
void drv_l1_IspSetHbNrLumaMode(INT8U lmm, INT8U llm);
void drv_l1_IspSetHbNrLowLight_cs(INT8U cs, INT8U llt, INT8U lls);
void drv_l1_IspSetHbNrLumaStrength(INT8U LS1, INT8U LS2, INT8U LS3, INT8U LS4 ,INT8U LS5);
void drv_l1_IspSetHbNrEn(INT8U nr_en, INT8U val);
void drv_l1_IspSetHbNrAJ_en(INT8U aj_en);
void drv_l1_IspSetHbNrAJ_Edge(INT16U ajedge);
void drv_l1_IspSetHbNrYnoise_clip(INT8U Yclip);
void drv_l1_IspSetHbNrFilter(INT8U luma, INT8U chroma);
void drv_l1_IspSetHbNrLumaTH(INT8U LT1, INT8U LT2, INT8U LT3, INT8U LT4);


// undefined function
void drv_l1_CdspSetReadBackSize10(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize);
void drv_l1_FrontClrIntStatus(INT8U bit);
void drv_l1_CdspSetAddr(INT32U work_buffer);
void drv_l1_CdspSetBinmode(INT32U crop_x, INT32U crop_y, INT32U crop_width, INT32U crop_height, INT32U config);
void drv_l1_CdspSetSkipWrite(int skip);
void drv_l1_CdspSetWriteDRAMMode(void);
void drv_l1_CdspSetBurstMode(int burst);
void drv_l1_IspSetAEGain(INT16U Rgain, INT16U Ggain, INT16U Bgain);
INT8U drv_l1_CdspGetCropEn(void);
INT32U drv_l1_CdspGetUvDivideEn(void);
void drv_l1_set_2sensor_mode(int enable);

extern INT8U drv_l1_IspGetNLM_En(void);
extern INT32U drv_l1_CdspGetDataSource(void);
extern void drv_l1_CdspSetContrastUpdate(int update);


#endif //__drv_l1_CDSP_H__

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

#ifndef __drv_l1_PSCALER_H__
#define __drv_l1_PSCALER_H__

#include "project.h"

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/
#define PSCALER_RET_SUCCESS		0
#define PSCALER_RET_FAIL		1

enum
{
	PIPELINE_SCALER_INT_ENABLE_DISABLE					= 0x0UL,
	PIPELINE_SCALER_INT_ENABLE_AHB_READ_DONE			= (0x1<<0),
	PIPELINE_SCALER_INT_ENABLE_FRAME_END				= (0x1<<1),
	PIPELINE_SCALER_INT_ENABLE_EOF_CLEAR				= (0x1<<3),
	PIPELINE_SCALER_INT_ENABLE_CHG_FIFO					= (0x1<<4),
	PIPELINE_SCALER_INT_ENABLE_BUF_A					= (0x1<<5),
	PIPELINE_SCALER_INT_ENABLE_BUF_B					= (0x1<<6),
	PIPELINE_SCALER_INT_ENABLE_OVERFLOW					= (0x1<<7),
	PIPELINE_SCALER_INT_ENABLE_AHB_IN_FIFO_DONE			= (0x1<<8),
	PIPELINE_SCALER_INT_ENABLE_TEXT_STAMP_AHB_RD_DONE	= (0x1<<9),
	PIPELINE_SCALER_INT_ENABLE_TEXT_STAMP_DONE			= (0x1<<10),
	PIPELINE_SCALER_INT_ENABLE_FRAME_END_LAST			= (0x1<<11),
	PIPELINE_SCALER_INT_ENABLE_EDGE_ERROR				= (0x1<<14),
	PIPELINE_SCALER_INT_ENABLE_REG_REFRESH				= (0x1<<15),
	PIPELINE_SCALER_INT_ENABLE_BYPASS_FRAME_END			= (0x1<<17),
	PIPELINE_SCALER_INT_ENABLE_BYPASS_BUF_A				= (0x1<<18),
	PIPELINE_SCALER_INT_ENABLE_BYPASS_BUF_B				= (0x1<<19),
	PIPELINE_SCALER_INT_ENABLE_BYPASS_EOF_CLEAR			= (0x1<<20)
};
// ISO C standard (c99) states that the enumeration constants are of type int
#define PIPELINE_SCALER_INT_ENABLE_ALL					~(0x00010000U)

//+++ Input Format
#define PIPELINE_SCALER_INPUT_FORMAT_YUYV					0x00000000
#define PIPELINE_SCALER_INPUT_FORMAT_YVYU					0x00000001
#define PIPELINE_SCALER_INPUT_FORMAT_UYVY					0x00000002
#define PIPELINE_SCALER_INPUT_FORMAT_VYUY					0x00000003
#define PIPELINE_SCALER_INPUT_FORMAT_GP420					0x00000006
#define PIPELINE_SCALER_INPUT_FORMAT_Y					    0x00000007  // only for PSCALER_Y

//+++ Output Format
#define PIPELINE_SCALER_OUTPUT_FORMAT_YUYV					0x00000000
#define PIPELINE_SCALER_OUTPUT_FORMAT_YVYU					0x00000001
#define PIPELINE_SCALER_OUTPUT_FORMAT_UYVY					0x00000002
#define PIPELINE_SCALER_OUTPUT_FORMAT_VYUY					0x00000003
#define PIPELINE_SCALER_OUTPUT_FORMAT_RGB565				0x00000004
#define PIPELINE_SCALER_OUTPUT_FORMAT_GP420					0x00000006
#define PIPELINE_SCALER_OUTPUT_FORMAT_DRGB888				0x00000008
#define PIPELINE_SCALER_OUTPUT_FORMAT_DYUV888				0x00000009
#define PIPELINE_SCALER_OUTPUT_FORMAT_YONLY					0x0000000C

//+++ Input Source
#define PIPELINE_SCALER_INPUT_SOURCE_CDSP					0x00000000
#define PIPELINE_SCALER_INPUT_SOURCE_CSI					0x00000001
#define PIPELINE_SCALER_INPUT_SOURCE_PPUFB					0x00000002
#define PIPELINE_SCALER_INPUT_SOURCE_MB420					0x00000003
#define PIPELINE_SCALER_Y_INPUT_SOURCE_GPDLA				0x00000003
#define PIPELINE_SCALER_INPUT_SOURCE_DRAM					0x00000004
#define PIPELINE_SCALER_INPUT_SOURCE_CSI2					0x00000005
#define PIPELINE_SCALER_A_INPUT_SOURCE_MIPIB_CH0			0x00000006
#define PIPELINE_SCALER_A_INPUT_SOURCE_MIPIB_CH1			0x00000007
#define PIPELINE_SCALER_B_INPUT_SOURCE_MIPIB_CH2			0x00000006
#define PIPELINE_SCALER_B_INPUT_SOURCE_MIPIB_CH3			0x00000007
#define PIPELINE_SCALER_AB_INPUT_SOURCE_GPDLA			    0x0000000A //Pscaler A/B

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

//+++ Status
typedef enum
{
	PIPELINE_SCALER_STATUS_BUSY					= 0,
	PIPELINE_SCALER_STATUS_REFRESH_DONE			= 1,
	PIPELINE_SCALER_STATUS_FRAME_DONE			= (0x1 << 1),
	PIPELINE_SCALER_STATUS_BUF_A_DONE			= (0x1 << 2),
	PIPELINE_SCALER_STATUS_BUF_B_DONE			= (0x1 << 3),
	PIPELINE_SCALER_STATUS_BYPASS_FRAME_DONE	= (0x1 << 4),
	PIPELINE_SCALER_STATUS_BYPASS_BUF_A_DONE	= (0x1 << 5),
	PIPELINE_SCALER_STATUS_BYPASS_BUF_B_DONE	= (0x1 << 6),
	PIPELINE_SCALER_STATUS_OVERFLOW_OCCUR		= (0x1 << 7),
	PIPELINE_SCALER_STATUS_INPUT_EMPTY			= (0x1 << 8),
	PIPELINE_SCALER_STATUS_AHB_DONE				= (0x1 << 9),
	PIPELINE_SCALER_STATUS_FRAME_DONE_LAST		= (0x1 << 10),
	PIPELINE_SCALER_STATUS_EDGE_ERROR			= (0x1 << 11),
	PIPELINE_SCALER_STATUS_REG_REFRESH			= (0x1 << 12)
} PS_ST;

typedef enum
{
	PSCALER_A,
	PSCALER_B,
	PSCALER_Y,
	PSCALER_NUM
} PS_ID;

typedef enum
{
	TEXT_STAMP_Y_LINES_16,
	TEXT_STAMP_Y_LINES_32,
	TEXT_STAMP_Y_LINES_64,
	TEXT_STAMP_Y_LINES_128
}TEXT_STAMP_Y_LINES;

typedef enum
{
	TEXT_STAMP_DISABLE,
	TEXT_STAMP_ENABLE
}TEXT_STAMP_ACTION;

typedef enum
{
	TEXT_STAMP_1ST_SETTING,
	TEXT_STAMP_2ND_SETTING
}TEXT_STAMP_GROUP;

typedef enum
{
	TEXT_STAMP_1BIT_MODE,
	TEXT_STAMP_2BIT_MODE
}TEXT_STAMP_BITMODE;

typedef struct
{
	INT32U textStartX;			// Can,t be 0
	INT32U textStartY;			// Can,t be 0
	INT32U textWidth;			// 16 alignment
	INT32U textHeight;			// Refter to TEXT_STAMP_Y_LINES
	INT32U textSrcAddrs;
	INT8U  textBitMode; 		// 0:1bit 1:2bit
	INT8U  textStampSet;		// 0:First Setting  1:Second Setting
}TEXT_STAMP_PARAM;

typedef struct
{
	INT16U textColor0;
	INT16U textColor1;
	INT16U textColor2;
	INT16U textColor3;
}TEXT_STAMP_COLOR_PARAM;

typedef enum
{
	PSCALER_IN_FIFO_MODE_RELOAD_ADDRS,
	PSCALER_IN_FIFO_MODE_CONTINUE_ADDRS
}PIPELINE_SCALER_IN_FIFO_MODE;

typedef enum
{
	PSCALER_IN_FIFO_FRAME_MODE,
	PSCALER_IN_FIFO_8_LINES,
	PSCALER_IN_FIFO_16_LINES,
	PSCALER_IN_FIFO_32_LINES,
	PSCALER_IN_FIFO_64_LINES,
	PSCALER_IN_FIFO_128_LINES,
	PSCALER_IN_FIFO_256_LINES
}PIPELINE_SCALER_IN_FIFO_LINES;

typedef struct
{
	INT8U Filter1DEnableY;	// 0:Disable 1:Enable
	INT8U Filter1DEnableX;	// 0:Disable 1:Enable
	INT8S Filter1DXFactorA;	// -15~15
	INT8S Filter1DXFactorB;	// -15~15
	INT8S Filter1DXFactorC;	// -15~15
	INT8U Filter1DXFactorD;	// 0~5  A+B+C=2^D
}FILTER_1D_PARAM;

typedef enum
{
	REG_REFRESH_SOF = 0x1,
	REG_REFRESH_EOF_LAST = 0x2,
	REG_REFRESH_NOW = 0x4,
	REG_REFRESH_EOF = 0x8
}REG_REFRESH_PARAM;

typedef enum
{
	REG_REFRESH_LAST_EOF_NONE	= 0x00,
	REG_REFRESH_LAST_EOF_SCALE	= 0x01,
	REG_REFRESH_LAST_EOF_BYPASS	= 0x02,
	REG_REFRESH_LAST_EOF_MB420	= 0x04,
	REG_REFRESH_LAST_EOF_TXTBLD	= 0x08,
	REG_REFRESH_LAST_EOF_READ	= 0x10
}REG_REFRESH_LAST_PARAM;

typedef struct
{
	INT16U PipStartX; // 16 Alignment
	INT16U PipStartY; // 2 Alignment
	INT16U PipEndX;  // 16 Alignment
	INT16U PipEndY; // 2 Alignment
}PIP_WINDOW_PARAM;

typedef enum
{
	OUTEDGE_CLP_16,
	OUTEDGE_CLP_32,
	OUTEDGE_CLP_64,
	OUTEDGE_CLP_128
}OUTEDGE_CLP;

typedef enum
{
	OUTEDGE_COR_0,
	OUTEDGE_COR_2,
	OUTEDGE_COR_4,
	OUTEDGE_COR_8
}OUTEDGE_COR;

typedef enum
{
	OUTEDGE_TYPE_0,
	OUTEDGE_TYPE_1,
	OUTEDGE_TYPE_2,
	OUTEDGE_TYPE_DISABLE
}OUTEDGE_TYPE;

typedef enum
{
	SCALE_WR_BUF_A	= 0x1,
	SCALE_WR_BUF_B	= 0x2,
	BYPASS_WR_BUF_A	= 0x4,
	BYPASS_WR_BUF_B	= 0x8
}PS_WR_ST;

typedef enum
{
	BYPASS_DISABLE	= 0x0,
	BYPASS_GP420	= 0x1,
	BYPASS_YONLY	= 0x2
} PS_BYPASS_MODE;

typedef enum
{
	AUTO_MODE_NONSTOP = 0,
	AUTO_MODE_DONE_STOP
} PS_AUTO_STOP_MODE;

typedef enum
{
	AUTO_OUT_ONE_JPEG = 0,
	AUTO_OUT_TWO_JPEG
} PS_AUTO_JPEG_MODE;

typedef enum
{
	PS_OUT_A = 0,
	PS_OUT_B,
	PS_OUT_C,
	PS_OUT_D,
	PS_OUT_E,
	PS_OUT_F,
	PS_OUT_G,
	PS_OUT_H
} PS_OUT_ID;

typedef struct
{
    //CTRL0
    INT8U ck_unclear_op;                //Unclear region enable <<17
    INT8U ck_bg_cnt_sel;                // 0:512pixel; 1:256 pixel  <<16
    INT8U ck_avg_upd_mode;              // total 4 mode(2bit) <<14
    INT8U ck_edge_mode;                 // edge process 2bit  <<12
    INT8U ck_hue_use_reg;               //1:fix HUE centroid enable; 0: H/W auto  <<10
    INT8U ck_cb_use_reg;                //1:fix Cb centroid enable; 0:H/W auto   <<9
    INT8U ck_cr_use_reg;                //1:fix Cr centroid enable; 0:H/W auto   <<8
    INT8U ck_ydark_en;                  //1: mask dark color  <<7
    INT8U ck_hue_en;                    //1: Use Hue judgment  <<6
    INT8U ck_cb_rn;                     //1: Use Cb judgment  <<5
    INT8U ck_cr_en;                     //1: Use Cr judgment  <<4
    INT8U ck_filter_en;                 //1: enable 1x5 filter <<3
    INT8U ck_en;                        //1: chroma key enable <<0
    //BG_CTRL
    INT8U ck_use_bg_frame;              //0:fix bg color; 1: bg frame DRAM <<12
    INT8U ck_extra_rd_ahbbus_ck_en;     //  <<8
    INT8U ck_in_conti_mode;             //0: reload addr; 1:conti addr <<7
    INT8U ck_in_fifo_mode;              // set fifo line=> frame mode/ 8/16/32/64/128/256 line <<4
    INT8U ck_inf;                       //input bg format  <<0
}CK_CTRL_EN;

typedef struct
{
   INT32U ck_bg_yuv;                    //when ck_use_bg_frame =0
   INT32U ck_bg_in_addr;                //BG from DRAM pictrue
   INT8U  ck_hue_unclear;               //for 1x5 filter use   <<16
   INT8U  ck_ydark_thr;                 //<<0
   INT8U  ck_hue_thr_upd;                //  <<28
   INT8U  ck_hue_thr_step;               //  <<24
   INT16U  ck_hue_centroid_reg;          //ck_hue_use_reg = 1  <<0
   INT16U ck_hue_thr_h1;                //<<16
   INT16U ck_hue_thr_l1;                //<<0
   INT16U ck_hue_boundh;                // <<16
   INT16U ck_hue_boundl;                // <<0
   INT16U ck_hue_probe_x;               // <<16
   INT16U ck_hue_probe_y;               // <<0
   INT8U  ck_sat_thr;                   // <<16
#if 0
   INT8U  ck_cb_centroid_reg;           //ck_cb_use_reg = 1 <<8
   INT8U  ck_cb_thr_upd;                //  <<28
   INT16U  ck_cb_tolernce;              //   <<16
   INT8U  ck_cr_centroid_reg;           //ck_cr_use_reg = 1  <<0
   INT8U  ck_cr_thr_h1;                 //<<24
   INT8U  ck_cr_thr_l1;                 //<<8
   INT8U  ck_cr_thr_l0;                 //<<0
   INT8U  ck_cb_thr_h1;
   INT8U  ck_cb_thr_h0;
   INT8U  ck_cb_thr_l1;
   INT8U  ck_cb_thr_l0;
#endif
}CK_DATA_SET;

typedef void (*PS_ISR) (PS_ID, PS_ST);

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void drv_l1_pscaler_init(PS_ID PScalerNum);
extern INT32S drv_l1_pscaler_sw_reset(PS_ID PScalerNum);
extern void drv_l1_pscaler_disable(PS_ID PScalerNum);
extern INT32S drv_l1_pscaler_start(PS_ID PScalerNum);
extern INT32S drv_l1_pscaler_stop(PS_ID PScalerNum);
extern INT32S drv_l1_pscaler_stop1(PS_ID PScalerNum);
extern INT32S drv_l1_pscaler_interrupt_enable(PS_ID PScalerNum, INT8U en);
extern INT32S drv_l1_pscaler_interrupt_set(INT8U PScalerNum, INT32U intEnVale);

extern INT32S drv_l1_pscaler_HSYNC_latch_delay(PS_ID PScalerNum, INT32U delayTCount);
extern INT32S drv_l1_pscaler_1D_filter_set(PS_ID PScalerNum, FILTER_1D_PARAM* pFilter1DParam);
extern INT32S drv_l1_pscaler_H_first_enable(PS_ID PScalerNum, INT8U hFirstEnable);
extern INT32S drv_l1_pscaler_integration_mode_set(PS_ID PScalerNum,INT8U verticalEnable,INT8U horizontalEnable);

extern INT32U drv_l1_pscaler_status_get(PS_ID PScalerNum);
extern void drv_l1_pscaler_callback_register(PS_ID PScalerNum, void(*PScaler_Callback)(INT32U));
extern void drv_l1_pscaler_callback_register2(PS_ID PScalerNum, PS_ISR PScaler_Callback);
extern void drv_l1_pscaler_bypass_callback_register(PS_ID PScalerNum, void(*PScaler_Callback)(INT32U));
extern void drv_l1_pscaler_bypass_callback_register2(PS_ID PScalerNum, PS_ISR PScaler_Callback);

extern INT32S drv_l1_pscaler_pixels_set(PS_ID PScalerNum, INT16U inW, INT16U inH, INT16U outW, INT16U outH, INT32U factorW, INT32U factorH);
//backward compatible to GPM4
extern INT32S drv_l1_pscaler_input_pixels_set(INT8U PScalerNum, INT32U inWidth, INT32U inHeight);
extern INT32S drv_l1_pscaler_output_pixels_set(INT8U PScalerNum, INT32U widthFactor, INT32U outWidth, INT32U heightFactor, INT32U outHeight);

extern INT32S drv_l1_pscaler_input_fifo_line_set(PS_ID PScalerNum, PIPELINE_SCALER_IN_FIFO_MODE addrsMode, PIPELINE_SCALER_IN_FIFO_LINES fifoLine);
extern INT32S drv_l1_pscaler_input_fifo_restart(PS_ID PScalerNum);

extern INT32S drv_l1_pscaler_input_X_start_set(PS_ID PScalerNum, INT32U xOffset);
extern INT32S drv_l1_pscaler_input_Y_start_set(PS_ID PScalerNum, INT32U yOffset);
extern INT32S drv_l1_pscaler_input_format_set(PS_ID PScalerNum, INT32U inFormat);
extern INT32S drv_l1_pscaler_input_source_set(PS_ID PScalerNum, INT32U inSource);

extern INT32U drv_l1_pscaler_input_buffer_get(PS_ID PScalerNum);
extern INT32S drv_l1_pscaler_input_buffer_set(PS_ID PScalerNum, INT32U bufAddr);

extern INT32S drv_l1_pscaler_output_fifo_line_set(PS_ID PScalerNum, INT32U fifoLine, INT8U outStopEnable); // Only Support 422/GP420 , Not Support MB420
extern INT32S drv_l1_pscaler_output_format_set(PS_ID PScalerNum, INT32U outFormat);
extern INT32S drv_l1_pscaler_outboundary_color_set(PS_ID PScalerNum, INT8U outBoundEnable, INT32U outBoundColor);
extern INT32S drv_l1_pscaler_output_pixels_X_offset_set(PS_ID PScalerNum, INT32U outOffsetX); // Only Support 422/GP420 , Not Support MB420
extern INT32S drv_l1_pscaler_output_fifo_restart(PS_ID PScalerNum);

extern INT32S drv_l1_pscaler_output_A_buffer_set(PS_ID PScalerNum, INT32U bufAddr); // bufAddr: 4-byte alignment
extern INT32U drv_l1_pscaler_output_A_buffer_get(PS_ID PScalerNum);
extern INT32S drv_l1_pscaler_output_B_buffer_set(PS_ID PScalerNum, INT32U bufAddr); // bufAddr: 4-byte alignment
extern INT32U drv_l1_pscaler_output_B_buffer_get(PS_ID PScalerNum);

extern INT32S drv_l1_pscaler_bypass_mode_set(PS_ID PScalerNum,INT8U bypassEnable);
extern INT32S drv_l1_pscaler_bypass_output_A_buffer_set(PS_ID PScalerNum, INT32U bufAddr);
extern INT32U drv_l1_pscaler_bypass_output_A_buffer_get(PS_ID PScalerNum);
extern INT32S drv_l1_pscaler_bypass_output_B_buffer_set(PS_ID PScalerNum, INT32U bufAddr);
extern INT32U drv_l1_pscaler_bypass_output_B_buffer_get(PS_ID PScalerNum);
extern INT32S drv_l1_pscaler_bypass_output_fifo_line_set(PS_ID PScalerNum, INT32U fifoLine, INT8U outStopEnable);
extern INT32S drv_l1_pscaler_bypass_output_pixels_X_offset_set(PS_ID PScalerNum, INT32U outOffsetX);
extern INT32S drv_l1_pscaler_bypass_output_fifo_restart(PS_ID PScalerNum);

extern INT32S drv_l1_pscaler_PIP_window_set(PS_ID PScalerNum,INT8U pipEnable,PIP_WINDOW_PARAM* pPipWinParam);

extern INT32S drv_l1_pscaler_text_stamp_set(PS_ID PScalerNum, TEXT_STAMP_PARAM* ptextStampParam);
extern INT32S drv_l1_pscaler_text_stamp_color_cet(PS_ID PScalerNum, TEXT_STAMP_COLOR_PARAM* ptextStampColorParam);
extern INT32S drv_l1_pscaler_text_stamp_enable(PS_ID PScalerNum, INT8U settingNum, INT8U enableCtrl);

extern INT32S drv_l1_pscaler_register_refresh(PS_ID PScalerNum, REG_REFRESH_PARAM refreshMode);
extern INT32S drv_l1_pscaler_register_refresh_status_get(PS_ID PScalerNum);
extern INT32S drv_l1_pscaler_register_refresh_last_eof_config(PS_ID PScalerNum, REG_REFRESH_LAST_PARAM mode);

extern INT32S drv_l1_pscaler_drop_frame_set(PS_ID PScalerNum, INT8U fraction,INT8U denominator);
extern INT32S drv_l1_pscaler_outcrop_set(PS_ID PScalerNum, INT8U cropEnable, INT16U xStart, INT16U yStart, INT16U Width, INT16U Height);
extern INT32S drv_l1_pscaler_outedge_set(PS_ID PScalerNum, OUTEDGE_TYPE type, OUTEDGE_CLP clp, OUTEDGE_COR cor, INT8U gain);
extern INT32S drv_l1_pscaler_outfill_set(PS_ID PScalerNum, INT8U en, INT16U xStart, INT16U yStart, INT16U Width, INT16U Height);
extern INT32S drv_l1_pscaler_output_to_boxfilter_set(PS_ID PScalerNum, INT32U boxfilter_set);

extern PS_WR_ST drv_l1_pscaler_write_status_get(PS_ID PScalerNum);

extern void drv_l1_pscaler_clk_ctrl(PS_ID PScalerNum,INT16U clkEnable);

extern void chromakey_ref_data_print(PS_ID PScalerNum);
extern INT32S InitChromkey(PS_ID pnum, INT32U bg_color, INT8U pscale_out_fmt);
extern INT32S chromakey_start(PS_ID pnum, INT8U enable);

extern void drv_l1_pscaler_enable_automode(PS_ID PScalerNum, INT32U enable);
extern INT32S drv_l1_pscaler_set_automode(PS_ID PScalerNum, PS_AUTO_JPEG_MODE jpeg_mode, PS_AUTO_STOP_MODE stop_mode, INT32U fifo_num);
extern void drv_l1_pscaler_enable_bypass_automode(PS_ID PScalerNum, INT32U enable);
extern INT32S drv_l1_pscaler_set_bypass_automode(PS_ID PScalerNum, PS_AUTO_JPEG_MODE jpeg_mode, PS_AUTO_STOP_MODE stop_mode, INT32U fifo_num);
extern INT32S drv_l1_pscaler_output_buffer_set(PS_ID PScalerNum, PS_OUT_ID idx, INT32U bufAddr);
extern INT32U drv_l1_pscaler_output_buffer_get(PS_ID PScalerNum, PS_OUT_ID idx);
extern void drv_l1_pscaler_automode_stop(PS_ID PScalerNum, INT8U stop);

#endif // __drv_l1_PSCALER_H__


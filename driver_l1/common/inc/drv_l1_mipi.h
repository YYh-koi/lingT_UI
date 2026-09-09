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
#ifndef __drv_l1_MIPI_H__
#define __drv_l1_MIPI_H__

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/
/* mipi clock sample edge */
#define D_PHY_SAMPLE_POS		0x0
#define D_PHY_SAMPLE_NEG		0x1

/* mipi ecc order*/
#define MIPI_ECC_ORDER0			0x0
#define MIPI_ECC_ORDER1			0x1
#define MIPI_ECC_ORDER2			0x2
#define MIPI_ECC_ORDER3			0x3

/* mipi check method */
#define MIPI_CHECK_LP_00		0x0
#define MIPI_CHECK_HS_SEQ		0x1

/* detect input format */
#define MIPI_AUTO_DETECT		0
#define MIPI_USER_DEFINE		1

/* mipi data format */
#define MIPI_YUV422				0x0
#define MIPI_RGB888				0x1
#define MIPI_YUV565				0x2
#define MIPI_RAW8				0x3
#define MIPI_RAW10				0x4
#define MIPI_RAW12				0x5
#define MIPI_GENERIC_8_BIT		0x6
#define MIPI_USER_DEFINE_BYTE	0x7

/* mipi data type */
#define MIPI_DATA_TO_CSI		0
#define MIPI_DATA_TO_CDSP		1

/* mipi lane */
#define MIPI_1_LANE				0x1
#define MIPI_2_LANE				0x2
#define MIPI_4_LANE				0x4

/* mipi interface */
#define MIPI_0                  0
#define MIPI_1                  1

/* mipi byte clock source */
#define D_PHY_BYTE_CLK			0x0
#define GENERATE_PIXEL_CLK		0x1

/* interrupt flag */
#define MIPI_LANE0_SOT_SYNC_ERR	0x001
#define MIPI_HD_1BIT_ERR 		0x002
#define MIPI_HD_NBIT_ERR		0x004
#define MIPI_DATA_CRC_ERR		0x008
#define MIPI_LANE1_SOT_SYNC_ERR	0x010
#define MIPI_CCIR_SOF 			0x020
#define MIPI_SOF 			    0x100
#define MIPI_EOF 			    0x200
#define MIPI_INT_ALL			0x33F

#define VC_POS(ch3,ch2,ch1,ch0) (ch3<<6)|(ch2<<4)|(ch1<<2)|(ch0)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
    CH0=0,
	CH1,
    CH2,
    CH3,
    CH_MAX
} MIPI_VC_CH;

typedef struct mipi_config_s
{
	/* global configure */
	INT8U low_power_en;		/* 0:disable, 1:enable */
	INT8U byte_clk_edge;	/* 0:posedge, 1:negedge */
	INT8U mipi_lane_no;		/* MIPI_1Lane or MIPI_2Lane */
	INT8U pixel_clk_sel;	/* D_PHY_BYTE_CLK or GENERATE_PIXEL_CLK */

	/* format */
	INT8U data_from_mmr;	/* 0:auto detect, 1:user define */
	INT8U data_type;		/* MIPI_YUV422 ~ MIPI_USER_DEFINE_BYTE */
	INT8U data_type_to_cdsp;/* cdsp raw10, 1:data[7:0]+2':00 to cdsp, 0: 2'00+data[7:0] to csi */
	INT8U RSD2;

	/* ccir601 timing */
	INT16U h_size;			/* 0~0xFFFF */
	INT16U v_size;			/* 0~0xFFFF */
	INT8U h_back_porch;		/* 0~0xF */
	INT8U h_front_porch;	/* 0~0xF */
	INT8U blanking_line_en;	/* 0:disable, 1:enable */
	INT8U RSD3;

	/* ecc */
	INT8U ecc_check_en;		/* 0:disable, 1:enable */
	INT8U ecc_order;		/* MIPI_ECC_ORDER0 ~ MIPI_ECC_ORDER3 */
	INT16U data_mask_time;	/* unit is ns */
	INT8U check_hs_seq;		/* 0:disable, 1:enable */
	INT8U pixel4;		    /* 0:disable, 1:enable */
} mipi_config_t;

typedef struct mipi_config_vc_s
{
    /* Virtual channel */
    /*ch0 set by mipi_config_t*/
	INT8U virtual_ch_en;    /* vc en */
	INT16U data_ch_position;/* vc data output position */
	INT16U h_size1;			/* vc ch1 h_size 0~0xFFFF */
	INT16U v_size1;			/* vc ch1 v_size 0~0xFFFF */
	INT8U h_back_porch1;	/* vc ch1 back porch 0~0xF */
	INT8U h_front_porch1;	/* vc ch1 front porch 0~0xF */
	INT16U h_size2;			/* vc ch2 h_size 0~0xFFFF */
	INT16U v_size2;			/* vc ch2 v_size 0~0xFFFF */
	INT8U h_back_porch2;	/* vc ch2 back porch 0~0xF */
	INT8U h_front_porch2;	/* vc ch2 front porch 0~0xF */
	INT16U h_size3;			/* vc ch3 h_size 0~0xFFFF */
	INT16U v_size3;			/* vc ch3 v_size 0~0xFFFF */
	INT8U h_back_porch3;	/* vc ch3 back porch 0~0xF */
	INT8U h_front_porch3;	/* vc ch3 front porch 0~0xF */
} mipi_config_vc_t, *p_mipi_config_vc_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void drv_l1_mipi_init(INT32U dev_no, INT8U enable, INT8U vic_num);
extern INT32S drv_l1_mipi_isr_register(void (*user_isr_callback)(INT32U dev_no, INT32U event));
extern INT32S drv_l1_mipi_B_isr_register(void (*user_isr_callback)(INT32U dev_no, INT32U event));
extern void drv_l1_mipi_isr_unregister(void);
extern INT32S drv_l1_mipi_set_irq_enable(INT32U dev_no, INT32U enable, INT32U bit);
extern void drv_l1_mipi_set_global_cfg(INT32U dev_no, INT8U low_power_en, INT8U byte_clk_edge);
extern void drv_l1_mipi_set_lane_number(INT32U dev_no, INT8U lane_no);
extern void drv_l1_mipi_set_pixel_clock(INT32U dev_no, INT8U pixel_clk);
extern void drv_l1_mipi_set_ecc(INT32U dev_no, INT8U ecc_order, INT8U ecc_check_en);
extern void drv_l1_mipi_set_mask_cnt(INT32U dev_no, INT8U da_mask_cnt, INT8U check_hs_seq);
extern void drv_l1_mipi_set_ccir601_if(INT32U dev_no, INT8U h_back_porch, INT8U h_front_porch, INT8U blanking_line_en);
extern void drv_l1_mipi_set_image_size(INT32U dev_no, INT16U h_size, INT16U v_size);
extern void drv_l1_mipi_set_data_fmt(INT32U dev_no, INT8U data_from_mmr, INT8U data_type_mmr, INT8U data_type_cdsp);
extern INT32S drv_l1_mipi_set_parameter(INT32U dev_no, mipi_config_t *argp);
extern INT32S drv_l1_mipi_set_4pixel_mode(INT32U dev_no, INT8U enable);
/* virtual channel */
extern INT32S drv_l1_mipi_set_vc_enable(INT32U dev_no, INT8U enable);
extern INT32S drv_l1_mipi_set_vc_channel_data_set(INT32U dev_no, INT8U ch_data);
extern INT32S drv_l1_mipi_set_vc_config(INT32U dev_no, INT8U channel, INT8U h_back_porch, INT8U h_front_porch);
extern INT32S drv_l1_mipi_set_vc_size(INT32U dev_no, INT8U channel, INT16U h_size, INT16U v_size, INT8U data_type);

#endif

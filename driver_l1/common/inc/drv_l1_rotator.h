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
#ifndef __DRV_L1_ROTATOR_H__
#define __DRV_L1_ROTATOR_H__

// irq state
#define C_ROTATOR_END_STATE			                0x00000001
#define C_ROTATOR_WRITR_BUFFER_READY_STATE	        0x00000002
#define C_ROTATOR_WRITR_FRAME_END_STATE	            0x00000004
#define C_ROTATOR_READ_FRAME_END_STATE	            0x00000008
#define C_ROTATOR_SRC_FRAME_SEL_STATE	            (1 << 12) // 0: A Frame Buffer 1: B Frame Buffer
#define C_ROTATOR_SLAVE_WRITE_PORT_HREADY_STATE	    (1 << 13)
#define C_ROTATOR_SLAVE_WRITE_PORT_HREADY_LNB_STATE	(1 << 14)
#define C_ROTATOR_LINE_BUFFER_END_STATE	            (1 << 16)
#define C_ROTATOR_MASK_STATE	                    0x0001FFFF

// irq interrupt enable
#define C_ROTATOR_END_INT_EN			            C_ROTATOR_END_STATE
#define C_ROTATOR_WRITR_BUFFER_READY_INT_EN	        C_ROTATOR_WRITR_BUFFER_READY_STATE
#define C_ROTATOR_WRITR_FRAME_END_INT_EN	        C_ROTATOR_WRITR_FRAME_END_STATE
#define C_ROTATOR_READ_FRAME_END_INT_EN	            C_ROTATOR_READ_FRAME_END_STATE
#define C_ROTATOR_LINE_BUFFER_END_INT_EN	        C_ROTATOR_LINE_BUFFER_END_STATE

// control register
#define C_ROTATOR_START			                    0x00000001
#define C_ROTATOR_RESET			                    0x00000001
#define	MASK_IMG_RANGE			                    0xFFF

//Rotator0 control bit
#define	B_ROT_MODE				                    0
#define	MASK_ROT_MODE			                    (0xF << B_ROT_MODE)
#define	MASK_ROT0_MODE			                    (0x7 << B_ROT_MODE)
#define B_IMAGE_RGB565                              3
#define	IMAGE_RGB565_ENABLE                         (((INT32U) 0)<<B_IMAGE_RGB565)
#define	IMAGE_YUV422_ENABLE                         (((INT32U) 1)<<B_IMAGE_RGB565)
#define B_IMAGE_YUV422                              4
#define	MASK_YUYV_ENABLE                            (((INT32U) 3)<<B_IMAGE_YUV422)

//Rotator1/2 control bit
#define B_NON_RIGHT_ANGLE_LINE_BUF_NUM              18
#define	MASK_RIGHT_ANGLE_LINE_BUF_NUM_MODE          (((INT32U) 0x3)<<B_NON_RIGHT_ANGLE_LINE_BUF_NUM)
#define B_BURST16_MODE                              16
#define	BURST16_ENABLE                              (((INT32U) 1)<<B_BURST16_MODE)
#define B_AUTO_HIGH_PRIORITY_MODE                   15
#define	AUTO_HIGH_PRIORITY_ENABLE                   (((INT32U) 1)<<B_AUTO_HIGH_PRIORITY_MODE)
#define B_IMAGE_ROT1_Y_ONLY                         14
#define	IMAGE_ROT1_Y_ONLY_ENABLE                    (((INT32U) 1)<<B_IMAGE_ROT1_Y_ONLY)
#define B_IMAGE_ROT1_RGB565                         12
#define	IMAGE_ROT1_GP420_ENABLE                     (((INT32U) 0)<<B_IMAGE_ROT1_RGB565)
#define	IMAGE_ROT1_YUV422_ENABLE                    (((INT32U) 1)<<B_IMAGE_ROT1_RGB565)
#define	IMAGE_ROT1_RGB565_ENABLE                    (((INT32U) 0x2)<<B_IMAGE_ROT1_RGB565)
#define	MASK_IMAGE_ROT1_COLOR_ENABLE                (((INT32U) 0x3)<<B_IMAGE_ROT1_RGB565)
#define B_ROTATOR_ENABLE_MODE                       11
#define	ROTATOR_ENABLE                              (((INT32U) 1)<<B_ROTATOR_ENABLE_MODE)
#define B_AB_FRAME_BUFFER_MODE                      9
#define	AB_FRAME_BUFFER_ENABLE                      (((INT32U) 1)<<B_AB_FRAME_BUFFER_MODE)

//Rotator2 control bit
#define B_ROT_PPU420_MODE                           20
#define	ROT_PPU420_ENABLE                           (((INT32U) 1)<<B_ROT_PPU420_MODE)
#define B_TFT2_FROM_ROT_LB                          11
#define TFT2_FROM_ROT_LB_ENABLE                     (((INT32U) 1)<<B_TFT2_FROM_ROT_LB)
#define B_ROT_IRAM_SYSFIFO_MODE                     10
#define ROT_IRAM_SYSFIFO_ENABLE                     (((INT32U) 1)<<B_ROT_IRAM_SYSFIFO_MODE)
#define B_ROT_DRAM_TFT_MODE                         8
#define	ROT_DRAM_2_TFT_ENABLE                       (((INT32U) 1)<<B_ROT_DRAM_TFT_MODE)
#define B_ROT_TO_TFT2_MODE                          4
#define	ROT_TO_TFT2_MODE_ENABLE                     (((INT32U) 1)<<B_ROT_TO_TFT2_MODE)
#define B_ROT_IRAM_SHARE_MODE                       0
#define	ROT_IRAM_SHARE_ENABLE                       (((INT32U) 1)<<B_ROT_IRAM_SHARE_MODE)

// Rotator1 theta control register
#define B_CACHE_FLUSH_MODE                          6
#define	CACHE_FLUSH_MODE_ENABLE                     (((INT32U) 1)<<B_CACHE_FLUSH_MODE)
#define B_CACHE_MODE                                5
#define	CACHE_MODE_ENABLE                           (((INT32U) 1)<<B_CACHE_MODE)
#define B_BLOCK_MODE                                4
#define	BLOCK_MODE_ENABLE                           (((INT32U) 1)<<B_BLOCK_MODE)
#define B_THETA_BUR_W_SEL                           2
#define	MASK_THETA_BUR_W_SEL_MODE                   (((INT32U) 0x3)<<B_THETA_BUR_W_SEL)
#define B_Y_ONLY_IN_MODE                            1
#define	Y_ONLY_IN_ENABLE                            (((INT32U) 1)<<B_Y_ONLY_IN_MODE)
#define B_COUNTER_CLOCKWISE_MODE                    0
#define	COUNTER_CLOCKWISE_ENABLE                    (((INT32U) 1)<<B_COUNTER_CLOCKWISE_MODE)

// API Function Parameter Input
// rotator 0/1 input color mode
#define IMAGE_YUYV                                  0
#define IMAGE_YVYU                                  1
#define IMAGE_UYVY                                  2
#define IMAGE_VYUY                                  3
#define IMAGE_RGB565                                4
#define IMAGE_GP420                                 5   // rotator1/2
#define IMAGE_Y_ONLY                                6   // rotator1 only

// rotator 0/1/2 mode
#define ROTATOR_90                                  0
#define ROTATOR_180                                 1
#define ROTATOR_270                                 2
#define ROTATOR_HORIZONTAL_MIRROR                   3
#define ROTATOR_VERTICAL_MIRROR                     4
#define ROTATOR_270_VERTICAL_MIRROR                 5
#define ROTATOR_90_HORIZONTAL_MIRROR                6
#define ROTATOR_THETA                               7   // rotator1 only
#define ROTATOR_PST                                 15  // rotator1 only

// rotator1 Burst mode
#define ROTATOR1_BURST8                             0
#define ROTATOR1_BURST16                            1

// rotator1 non-right angle rotation line buffer number
#define ROTATOR1_NON_RIGHT_ANGLE_LINE1              0
#define ROTATOR1_NON_RIGHT_ANGLE_LINE2              1
#define ROTATOR1_NON_RIGHT_ANGLE_LINE4              2
#define ROTATOR1_NON_RIGHT_ANGLE_LINE8              3

// rotator1 theta color mode
#define THETA_IMAGE_GP420                           0
#define THETA_IMAGE_Y_ONLY                          1

// rotator1 theta clockwise mode
#define ROTATOR1_THETA_IMAGE_CLOCKWISE              0
#define ROTATOR1_THETA_IMAGE_COUNTER_CLOCKWISE      1

// rotator1 theta burst mode
#define ROTATOR1_THETA_WRITE_BURST16                0
#define ROTATOR1_THETA_WRITE_BURST8                 1
#define ROTATOR1_THETA_WRITE_BURST4                 2

// HW Rotator Selection
#define ROTATOR0_EN		                            1
#define ROTATOR1_EN		                            1
#define ROTATOR2_EN		                            1

typedef enum
{
#if ROTATOR0_EN == 1
    ROTATOR_0 = 0,
#endif
#if ROTATOR1_EN == 1
    ROTATOR_1 = 1,
#endif
#if ROTATOR2_EN == 1
    ROTATOR_2 = 2,
#endif
    ROTATOR_MAX
} ROTATOR_NUM;

typedef struct {
	INT32U PST_MXT_11;
    INT32U PST_MXT_12;
    INT32U PST_MXT_13;
    INT32U PST_MXT_21;
    INT32U PST_MXT_22;
    INT32U PST_MXT_23;
    INT32U PST_MXT_31;
    INT32U PST_MXT_32;
    INT32U PST_MXT_33;
} PERSPECTIVE_MATRIX_CFG;

typedef enum
{
	HIGH_PRY_LEVEL_0,
	HIGH_PRY_LEVEL_1,
	HIGH_PRY_LEVEL_2,
	HIGH_PRY_LEVEL_3,
	HIGH_PRY_LEVEL_4
} RPTATOR_HIGH_PRIORITY_NUM;

// M4 rotator API
extern void rotator_init(void);
extern void rotator_uninit(void);
extern INT32S rotator_src_img_info(INT8U color, INT16U w, INT16U h, INT32U in_buf);
extern INT32S rotator_tar_img_addr(INT32U out_buf);
extern INT32S drv_l1_rotator_src_img_addr(INT8U rotator_num, INT32U out_buf);
extern INT32S rotator_mode_set(INT32U mode);
extern INT32S rotator_start(void);
extern INT32S rotator_end_wait(INT32U wait);

extern void drv_l1_rotator_init(INT8U rotator_num);
extern void drv_l1_rotator_uninit(INT8U rotator_num);
extern INT32S drv_l1_rotator_src_img_info(INT8U rotator_num, INT8U color, INT16U w, INT16U h, INT32U in_buf);
extern INT32S drv_l1_rotator_tar_img_addr(INT8U rotator_num, INT32U out_buf);
extern INT32S drv_l1_rotator_mode_set(INT8U rotator_num, INT32U mode);
extern INT32S drv_l1_rotator_start(INT8U rotator_num);
extern INT32S drv_l1_rotator_end_wait(INT8U rotator_num, INT32U wait);
extern INT32S drv_l1_rotator_isr_callback_set(INT8U rotator_num, void (*user_isr_callback)(INT8U rotator_number, INT32U rotator_event));

// rotator1/2
extern INT32S drv_l1_rotator_src_img_addr2(INT8U rotator_num, INT32U in_buf2);
extern INT32S drv_l1_rotator_ab_buf_set(INT8U rotator_num, INT32U enable);
extern INT32S drv_l1_rotator_enable_set(INT8U rotator_num, INT32U enable);
extern INT32S drv_l1_rotator_auto_high_priority_set(INT8U rotator_num, INT32U enable);
extern INT32S drv_l1_rotator_burst16_set(INT8U rotator_num, INT32U enable);
extern INT32S drv_l1_rotator_non_right_angle_line_buffer_number_set(INT8U rotator_num, INT32U line_number);
extern INT32S drv_l1_rotator_software_reset_set(INT8U rotator_num);
extern INT32S drv_l1_rotator_theta_info_set(INT8U rotator_num, INT32U burst_mode, INT32U in_color_mode, INT32U clockwise_mode);
extern INT32S drv_l1_rotator_angle_set(INT8U rotator_num, INT16S angle);
extern INT32S drv_l1_rotator_cache_set(INT8U rotator_num, INT32U cache_on, INT32U cache_flush, INT32U block_mode);
extern INT32S drv_l1_rotator_perspective_set(INT8U rotator_num, INT32U in_h, INT32U in_v, PERSPECTIVE_MATRIX_CFG *PST_MXT);

// rotator2 only
extern INT32S drv_l1_rotator_burst16_set(INT8U rotator_num, INT32U enable);
extern INT32S drv_l1_rotator_ppu420_set(INT8U rotator_num, INT32U enable);
extern INT32S drv_l1_rotator_dram2tft_set(INT8U rotator_num, INT32U enable);
extern INT32S drv_l1_rotator_high_priority_set(INT8U rotator_num, INT8U enable, INT8U level);

#endif		// __DRV_L1_PPU_H__

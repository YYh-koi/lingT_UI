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
#ifndef __drv_l1_TFT_H__
#define __drv_l1_TFT_H__

#define TFT_ENABLE    0xFFFFFFFF
#define TFT_DIS       0

typedef struct
{												// Offset
    volatile INT32U	TFT_CTRL;					// 0x0000
    volatile INT32U	TFT_V_PERIOD;			    // 0x0004
	volatile INT32U	TFT_VS_WIDTH;			    // 0x0008
	volatile INT32U	TFT_V_START;			    // 0x000C
	volatile INT32U	TFT_V_END;				    // 0x0010
	volatile INT32U	TFT_H_PERIOD;				// 0x0014
	volatile INT32U	TFT_HS_WIDTH;				// 0x0018
	volatile INT32U	TFT_H_START;				// 0x001C
	volatile INT32U	TFT_H_END;			        // 0x0020
	volatile INT32U	TFT_LINE_RGB_ORDER;		    // 0x0024
	volatile INT32U	TFT_STATUS;				    // 0x0028
	volatile INT32U	TFT_MEM_BUFF_WR;			// 0x002C
	volatile INT32U	TFT_MEM_BUFF_RD;			// 0x0030
    volatile INT32U RESERVED[3]; 				// 0x0034~3C
    volatile INT32U TFT_TE_CTRL;                // 0x0040
    volatile INT32U TFT_TE_HS_WIDTH;            // 0x0044
    volatile INT32U TFT_INT_EN;                 // 0x0048
    volatile INT32U TFT_INT_CLR;                // 0x004C
    volatile INT32U RESERVED2[8];                // 0x0050~6C
    volatile INT32U TFT_VS_START;               // 0x0070
    volatile INT32U TFT_VS_END;                 // 0x0074
    volatile INT32U TFT_HS_START;               // 0x0078
    volatile INT32U TFT_HS_END;                 // 0x007C
}*TFT_DEVICE_SFR;

typedef struct
{												    // Offset
    volatile INT32U	TFT_CTRL;					    // 0x0000
    volatile INT32U	TFT_V_PERIOD;			        // 0x0004
	volatile INT32U	TFT_VS_WIDTH;			        // 0x0008
	volatile INT32U	TFT_V_START;			        // 0x000C
	volatile INT32U	TFT_V_END_2_LINE_RGB_ORDER;     // 0x0010
	volatile INT32U	TFT_H_PERIOD_2_MEM_BUFF_WR;	    // 0x0014
	volatile INT32U	TFT_HS_WIDTH_2_CLIP_V_START;    // 0x0018
	volatile INT32U	TFT_H_START_2_CLIP_V_END;		// 0x001C
	volatile INT32U	TFT_H_END_2_CLIP_H_START;	    // 0x0020
	volatile INT32U	TFT_SCA_OUT_RES;		        // 0x0024
	volatile INT32U	TFT_VS_START_2_CLIP_H_END;		// 0x0028
	volatile INT32U	TFT_VS_END_2_TEARING_EFF;		// 0x002C
	volatile INT32U	TFT_HS_START_2_TEARING_HCNT;	// 0x0030
    volatile INT32U TFT_HS_END;                     // 0x0034
    volatile INT32U TFT_SCA_FACTOR;                 // 0x0038
    volatile INT32U TFT_SCA_OFFSET;                 // 0x003C
}*TFT2_DEVICE_SFR;

typedef enum
{
	TFT_1,
	TFT_2,
	TFT_NUM
} TFT_DEV_ID;

// tft clock divide
#define TFT_CLK_DIVIDE_1        0x0
#define TFT_CLK_DIVIDE_2        0x2
#define TFT_CLK_DIVIDE_3        0x4
#define TFT_CLK_DIVIDE_4        0x6
#define TFT_CLK_DIVIDE_5        0x8
#define TFT_CLK_DIVIDE_6        0xA
#define TFT_CLK_DIVIDE_7        0xC
#define TFT_CLK_DIVIDE_8        0xE
#define TFT_CLK_DIVIDE_9        0x10
#define TFT_CLK_DIVIDE_10       0x12
#define TFT_CLK_DIVIDE_11       0x14
#define TFT_CLK_DIVIDE_12       0x16
#define TFT_CLK_DIVIDE_13       0x18
#define TFT_CLK_DIVIDE_14       0x1A
#define TFT_CLK_DIVIDE_15       0x1C
#define TFT_CLK_DIVIDE_16       0x1E
#define TFT_CLK_DIVIDE_17       0x20
#define TFT_CLK_DIVIDE_18       0x22
#define TFT_CLK_DIVIDE_19       0x24
#define TFT_CLK_DIVIDE_20       0x26
#define TFT_CLK_DIVIDE_21       0x28
#define TFT_CLK_DIVIDE_22       0x2A
#define TFT_CLK_DIVIDE_23       0x2C
#define TFT_CLK_DIVIDE_24       0x2E
#define TFT_CLK_DIVIDE_25       0x30
#define TFT_CLK_DIVIDE_26       0x32
#define TFT_CLK_DIVIDE_27       0x34
#define TFT_CLK_DIVIDE_28       0x36
#define TFT_CLK_DIVIDE_29       0x38
#define TFT_CLK_DIVIDE_30       0x3A
#define TFT_CLK_DIVIDE_31       0x3C
#define TFT_CLK_DIVIDE_32       0x3E

// P_TFT_CTRL
#define TFT_EN                  (1<<0)
#define TFT_CLK_SEL             (7<<1)
#define TFT_MODE                (0xF<<4)
#define TFT_VSYNC_INV           (1<<8)
#define TFT_HSYNC_INV           (1<<9)
#define TFT_DCLK_INV            (1<<10)
#define TFT_DE_INV              (1<<11)
#define TFT_H_COMPRESS          (1<<12)
#define TFT_MEM_BYTE_EN         (1<<13)
#define TFT_INTERLACE_MOD       (1<<14)
#define TFT_VSYNC_UNIT          (1<<15)

// P_TFT2_CTRL
#define TFT2_HDMI_EN            (1<<16)
#define TFT2_HDMI_FASTREQ       (1<<17)
#define B_TFT2_DATA_MODE        24
#define TFT2_DATA_MODE          (3<<B_TFT2_DATA_MODE)
#define TFT2_SWITCH_EN          (1<<26)
#define TFT2_BANK_SEL_EN        (1<<31)

#define B_TFT2_MEM_BUFF_WR       0
#define TFT2_MEM_BUFF_WR_SIZE    (0xFFFF<<B_TFT2_MEM_BUFF_WR)
#define B_TFT2_MEM_BUFF_RD       16
#define TFT2_MEM_BUFF_RD_SIZE    (0xFFFF<<B_TFT2_MEM_BUFF_RD)

// P_TFT_LINE_RGB_ORDER
#define TFT_REG_POL_1LINE       0x0
#define TFT_REG_POL_2LINE       0x1
#define TFT_NEW_POL_EN          0x80

// P_TFT_TS_MISC
#define TFT_REG_POL             (1<<4)
#define TFT_REG_REV             (1<<5)
#define TFT_UD_I                (1<<6)
#define TFT_RL_I                (1<<7)
#define TFT_DITH_EN             (1<<8)
#define TFT_DITH_MODE           (1<<9)
#define B_TFT_DATA_MODE         10
#define TFT_DATA_MODE           (3<<B_TFT_DATA_MODE)
#define TFT_SWITCH_EN           (1<<12)
#define TFT_GAMMA_EN            (1<<13)
#define TFT_DCLK_SEL            (3<<14)
#define TFT_DCLK_DELAY          (7<<18)
#define TFT_SLIDE_EN            (1<<21)

// P_TFT2_TE
#define TFT2_TE_EN              (1<<0)
#define TFT2_TE_WRITE_ONCE      (1<<1)
#define TFT2_GAMMA_EN           (1<<2)
#define HDMI_2048_EN            (1<<3)
#define TVFB_BUFFER_EN          (1<<4)
#define B_TVFB_BUFFER           5
#define MASK_TVFB_BUFFER_THRES  (0x3<<B_TVFB_BUFFER)
#define TFT2_TE888_EN           (1<<8)
#define YUV420_EXT_RAM          (1<<9)
#define B_YUV420_UNDERRUN       10
#define MASK_YUV420_THRES       (0x3<<B_YUV420_UNDERRUN)
#define TFT2_TE_MODE            (1<<16)
#define B_TFT2_TE_LINE_MODE     24
#define MASK_TFT2_TE_LINE_SEL   (0xFF<<B_TFT2_TE_LINE_MODE)

#define TFT2_DCLK_DELAY         (7<<26)
#define TFT2_SLIDE_EN           (1<<29)
#define TFT2_DCLK_SEL           (3<<30)

// tft mode
#define TFT_MODE_UPS051         0x0
#define TFT_MODE_UPS052         0x10
#define TFT_MODE_CCIR656        0x20
#define TFT_MODE_PARALLEL       0x30
#define TFT_MODE_TCON           0x40

#define TFT_MODE_MEM_CMD_WR     0x80
#define TFT_MODE_MEM_CMD_RD     0x90
#define TFT_MODE_MEM_DATA_WR    0xa0
#define TFT_MODE_MEM_DATA_RD    0xb0
#define TFT_MODE_MEM_CONTI      0xc0
#define TFT_MODE_MEM_ONCE       0xd0

// tft data mode
#define TFT_DATA_MODE_8         0x0
#define TFT_DATA_MODE_565	    0x1
#define TFT_DATA_MODE_666       0x2
#define TFT_DATA_MODE_888       0x3

// tft clock phase select
#define TFT_DCLK_SEL_0          0x0000
#define TFT_DCLK_SEL_90         0x4000
#define TFT_DCLK_SEL_180        0x8000
#define TFT_DCLK_SEL_270        0xC000

// tft clock delay select
#define TFT_DCLK_DELAY_0        0x000000
#define TFT_DCLK_DELAY_1        0x040000
#define TFT_DCLK_DELAY_2        0x080000
#define TFT_DCLK_DELAY_3        0x0C0000
#define TFT_DCLK_DELAY_4        0x100000
#define TFT_DCLK_DELAY_5        0x140000
#define TFT_DCLK_DELAY_6        0x180000
#define TFT_DCLK_DELAY_7        0x1C0000

extern void drv_l1_tft_init(void);
extern void drv_l1_tft_uninit(void);
extern void drv_l1_tft_en_set(INT32U enable);
extern void drv_l1_tft_clk_set(INT32U clk);
extern void drv_l1_tft_mode_set(INT32U mode);
extern void drv_l1_tft_signal_inv_set(INT32U mask, INT32U value);
extern void drv_l1_tft_h_compress_set(INT32U enable);
extern void drv_l1_tft_mem_unit_set(INT8U data_width);
extern void drv_l1_tft_interlace_set(INT32U interlace);
extern void drv_l1_tft_vsync_unit_set(INT32U base_dclk);

extern void drv_l1_tft_data_mode_set(INT32U mode);
extern void drv_l1_tft_rb_switch_set(INT32U enable);
extern void drv_l1_tft_slide_en_set(INT32U enable);
extern void drv_l1_tft_dclk_sel_set(INT32U sel);
extern void drv_l1_tft_dclk_delay_set(INT32U delay);
extern void drv_l1_tft_display_buffer_set(INT32U addr);
extern void drv_l1_tft_reg_pol_set(INT8U pol);

extern void drv_l1_tft_i80_wr_cmd(INT16U command);
extern void drv_l1_tft_i80_wr_data(INT16U data);
extern INT16U drv_l1_tft_i80_read_data(INT16U command);
extern INT16U drv_l1_tft_target_clk_set(INT32U tar_clk);

// disp device
extern void drv_l1_disp_dev_init(TFT_DEV_ID dev_num);
extern void drv_l1_disp_dev_uninit(TFT_DEV_ID dev_num);
extern void drv_l1_disp_dev_en_set(TFT_DEV_ID dev_num, INT32U enable);
extern void drv_l1_disp_dev_clk_set(TFT_DEV_ID dev_num, INT32U clk);
extern void drv_l1_disp_dev_mode_set(TFT_DEV_ID dev_num, INT32U mode);
extern void drv_l1_disp_dev_signal_inv_set(TFT_DEV_ID dev_num, INT32U mask, INT32U value);
extern void drv_l1_disp_dev_h_compress_set(TFT_DEV_ID dev_num, INT32U enable);
extern void drv_l1_disp_dev_mem_unit_set(TFT_DEV_ID dev_num, INT8U data_width);
extern void drv_l1_disp_dev_interlace_set(TFT_DEV_ID dev_num, INT32U interlace);
extern void drv_l1_disp_dev_vsync_unit_set(TFT_DEV_ID dev_num, INT32U base_dclk);
extern void drv_l1_disp_dev_hdmi_en_set(TFT_DEV_ID dev_num, INT32U enable);

extern void drv_l1_disp_dev_data_mode_set(TFT_DEV_ID dev_num, INT32U mode);
extern void drv_l1_disp_dev_rb_switch_set(TFT_DEV_ID dev_num, INT32U enable);
extern void drv_l1_disp_dev_slide_en_set(TFT_DEV_ID dev_num, INT32U enable);
extern void drv_l1_disp_dev_dclk_sel_set(TFT_DEV_ID dev_num, INT32U sel);
extern void drv_l1_disp_dev_dclk_delay_set(TFT_DEV_ID dev_num, INT32U delay);
extern void drv_l1_disp_dev_display_buffer_set(TFT_DEV_ID dev_num, INT32U addr);
extern void drv_l1_disp_dev_reg_pol_set(TFT_DEV_ID dev_num, INT8U pol);
extern void drv_l1_disp_bank_sel_en_set(TFT_DEV_ID dev_num, INT32U enable);

extern INT32S drv_l1_disp_dev_ppu_gp420_line_set(TFT_DEV_ID dev_num, INT32U text_gp420_en, INT32U tvfb_yuv420_en, INT32U tvfb_buffer_yuv420_en);
extern INT32S drv_l1_disp_dev_hdmi_2048_set(TFT_DEV_ID dev_num, INT32U hdmi_2048_en);

extern void drv_l1_disp_dev_i80_wr_cmd(TFT_DEV_ID dev_num, INT16U command);
extern void drv_l1_disp_dev_i80_wr_data(TFT_DEV_ID dev_num, INT16U data);
extern INT16U drv_l1_disp_dev_i80_read_data(TFT_DEV_ID dev_num, INT16U command);
extern INT16U drv_l1_disp_dev_target_clk_set(TFT_DEV_ID dev_num, INT32U tar_clk);
extern void drv_l1_disp_dev_sync_de_te_set(TFT_DEV_ID dev_num, INT32U sync_en, INT32U de_en, INT32U te_en);
extern void drv_l1_tft2_scaling_up_size_set(INT32U in_width, INT32U in_height, INT32U width_offset, INT32U height_offset, INT32U out_width, INT32U out_height);

extern void drv_l1_tft_tepin_disable(TFT_DEV_ID dev_num);

#endif 		/* __drv_l1_TFT_H__ */

#ifndef __DRV_L1_DLA_H__
#define __DRV_L1_DLA_H__
#include "drv_l1.h"
#define DLA_LAYER_BUFFER_SIZE				32
#define DLA_LAYER_CHOUT						8

// Interrupt flag register
#define C_DLA_INT_DISABLE_MODE				0x00000000
#define	C_DLA_INT_LAYER_MODE_DONE			0x00000001
#define	C_DLA_INT_AUTO_MODE_DONE			0x00000010
#define	C_DLA_INT_INBUF_RELEASE_DONE		0x00000100
#define	C_DLA_INT_BRK_DONE					0x00001000

// control register
#define C_DLA_LAYER_MODE_START				0x00000001
#define C_DLA_AUTO_MODE_START				0x00000001
#define C_DLA_AUTO_MODE_BRK_ENABLE			0x00000001
#define C_DLA_SOFT_RESET					0x00000001

#define B_BIT24_SIZE						24
#define B_BIT16_SIZE						16
#define	B_BIT1_SIZE							1
#define	B_BIT_SIZE							0
#define	MASK_26BIT_SIZE						(((INT32U) 0x3FFFFFF)<<B_BIT_SIZE)
#define	MASK_15BIT_SIZE						(((INT32U) 0x7FFF)<<B_BIT_SIZE)
#define	MASK_16BIT_SIZE						(((INT32U) 0xFFFF)<<B_BIT_SIZE)
#define	MASK_12BIT_SIZE						(((INT32U) 0xFFF)<<B_BIT_SIZE)
#define	MASK_11BIT_SIZE						(((INT32U) 0x7FF)<<B_BIT_SIZE)
#define	MASK_10BIT_SIZE						(((INT32U) 0x3FF)<<B_BIT_SIZE)
#define	MASK_8BIT_SIZE						(((INT32U) 0xFF)<<B_BIT_SIZE)
#define	MASK_5BIT_SIZE						(((INT32U) 0x1F)<<B_BIT_SIZE)
#define	MASK_4BIT_SIZE						(((INT32U) 0xF)<<B_BIT_SIZE)
#define	MASK_3BIT_SIZE						(((INT32U) 0x7)<<B_BIT_SIZE)

#define C_DLA_MODULE_RESET				  (((INT32U) 0x1)<<B_BIT_SIZE)

// Layer Control Register
#define	B_DIL_EN							20
#define	DLA_DIL_DISABLE						(((INT32U) 0)<<B_DIL_EN)
#define	DLA_DIL_ENABLE						(((INT32U) 1)<<B_DIL_EN)
#define	B_BS_EN								16
#define	DLA_BS_DISABLE						(((INT32U) 0)<<B_BS_EN)
#define	DLA_BS_ENABLE						(((INT32U) 1)<<B_BS_EN)
#define	B_LAYER_END							12
#define	DLA_LAYER_END_DISABLE				(((INT32U) 0)<<B_LAYER_END)
#define	DLA_LAYER_END_ENABLE				(((INT32U) 1)<<B_LAYER_END)
#define	B_PRSV_WT_EN						10
#define	DLA_PRSV_WT_DISABLE					(((INT32U) 0)<<B_PRSV_WT_EN)
#define	DLA_PRSV_WT_ENABLE					(((INT32U) 1)<<B_PRSV_WT_EN)
#define	B_PRSV_FT_EN						9
#define	DLA_PRSV_FT_DISABLE					(((INT32U) 0)<<B_BS_EN)
#define	DLA_PRSV_FT_ENABLE					(((INT32U) 1)<<B_BS_EN)
#define	B_PRSV_BS_EN						8
#define	DLA_PRSV_BS_DISABLE					(((INT32U) 0)<<B_LAYER_END)
#define	DLA_PRSV_BS_ENABLE					(((INT32U) 1)<<B_LAYER_END)
#define	B_WORK_MODE							0
#define	MASK_WORK_MODE						(((INT32U) 0x3)<<B_WORK_MODE)

// IDMA Control Register
#define	B_IDMA_MER_NUM						16
#define	MASK_IDMA_MER_NUM					(((INT32U) 0xFFF)<<B_IDMA_MER_NUM)
#define	B_IDMA_CROP_EN						8
#define	DLA_IDMA_CROP_DISABLE				(((INT32U) 0)<<B_IDMA_CROP_EN)
#define	DLA_IDMA_CROP_ENABLE				(((INT32U) 1)<<B_IDMA_CROP_EN)
#define	B_IDMA_SN_EN						4
#define	DLA_IDMA_SN_DISABLE					(((INT32U) 0)<<B_IDMA_SN_EN)
#define	DLA_IDMA_SN_ENABLE					(((INT32U) 1)<<B_IDMA_SN_EN)
#define	B_IDMA_DAT_TYPE						0
#define	MASK_IDMA_DAT_TYPE					(((INT32U) 0x7)<<B_IDMA_DAT_TYPE)

// CBUF BK Register
#define	B_CBUF_WTAG_BK_NUM					28
#define	B_CBUF_WTAG_BK_START				24
#define	B_CBUF_OUT_BK_NUM					20
#define	B_CBUF_OUT_BK_START					16
#define	B_CBUF_WT_BK_NUM					12
#define	B_CBUF_WT_BK_START					8
#define	B_CBUF_FT_BK_NUM					4
#define	B_CBUF_FT_BK_START					0

// SDP Control Register
#define	B_SDP_SP_NUM						16
#define	MASK_SDP_SP_NUM						(((INT32U) 0xFFF)<<B_SDP_SP_NUM)
#define	B_SDP_ACT_RD_EN						12
#define	DLA_SDP_ACT_RD_DISABLE				(((INT32U) 0)<<B_SDP_ACT_RD_EN)
#define	DLA_SDP_ACT_RD_ENABLE				(((INT32U) 1)<<B_SDP_ACT_RD_EN)
#define	B_SDP_ODMA_MD						8
#define	MASK_SDP_ODMA_MD					(((INT32U) 0x7)<<B_SDP_ODMA_MD)
#define	B_SDP_POOL_MD						4
#define	MASK_SDP_POOL_MD					(((INT32U) 0x3)<<B_SDP_POOL_MD)
#define	B_SDP_ACT_MD						0
#define	MASK_SDP_ACT_MD						(((INT32U) 0x7)<<B_SDP_ACT_MD)

// BRK Control Register
#define	B_BRK_NUM							24
#define	MASK_BRK_NUM						(((INT32U) 0xFFF)<<B_BRK_NUM)

// INF7 Register
#define	B_INF_POOL_PLANE					24
#define	B_INF_PAD_SIZE_T					16
#define	B_INF_POOLW_MUL_STRW				8
#define	B_INF_POOLH_MUL_STRH				0

// DODMA CR Control Register
#define	B_DODMA_SN_EN						8
#define	DLA_DODMA_SN_DISABLE				(((INT32U) 0)<<B_DODMA_SN_EN)
#define	DLA_DODMA_SN_ENABLE					(((INT32U) 1)<<B_DODMA_SN_EN)

// DWC MAC OFS Control Register
#define	B_SATURATION_OFFSET					8
#define	B_SATURATION_OFFSET2				0
#define	MASK_SATURATION_MODE				(((INT32U) 0xFF)<<B_SATURATION_OFFSET)
#define	MASK_SATURATION_MODE2				(((INT32U) 0xFFFF)<<B_SATURATION_OFFSET2)

// Misc Register
#define	B_SATURATION_NUM					0
#define	C_DLA_SATURATION_ENABLE				(((INT32U) 0x1)<<B_SATURATION_NUM)

#define MAX_BYPASS_LAYER					5

typedef struct {
	INT32U layer_index;
	INT32U offset;
	INT32U next_input;
}Bypass_Internal_Info;

typedef struct
{
	INT32U bypass_layers;
	Bypass_Internal_Info bypass_info[MAX_BYPASS_LAYER];
} BYPASS_AUTO_INFO;

extern void drv_l1_dia_init(void);
extern void drv_l1_dla_uninit(void);
extern INT32S drv_l1_dla_inft_size_set(INT32U layer, INT32U w_size, INT32U h_size);
extern INT32S drv_l1_dla_ft_ch_set(INT32U layer, INT32U in_ch, INT32U out_ch);
extern INT32S drv_l1_dla_ker_size_set(INT32U layer, INT32U w_size, INT32U h_size);
extern INT32S drv_l1_dla_out_ftcv_size_set(INT32U layer, INT32U w_size, INT32U h_size);
extern INT32S drv_l1_dla_out_ftpl_size_set(INT32U layer, INT32U w_size, INT32U h_size);
extern INT32S drv_l1_dla_out_str_set(INT32U layer, INT32U w_size, INT32U h_size);
extern INT32S drv_l1_dla_out_pad_set(INT32U layer, INT32U pad_left, INT32U pad_top);
extern INT32S drv_l1_dla_out_pool_set(INT32U layer, INT32U w_size, INT32U h_size);
extern INT32S drv_l1_dla_ft_src_addr(INT32U layer, INT32U addr);
extern INT32S drv_l1_dla_ft_tar_addr(INT32U layer, INT32U addr);
extern INT32S drv_l1_dla_wt_addr(INT32U layer, INT32U addr);
extern INT32S drv_l1_dla_bs_addr(INT32U layer, INT32U addr);
extern INT32S drv_l1_dla_layctrl_dil_set(INT32U layer, INT32U dil_en);
extern INT32S drv_l1_dla_layctrl_bs_set(INT32U layer, INT32U bs_en);
extern INT32S drv_l1_dla_layctrl_layer_end_set(INT32U layer, INT32U layer_end);
extern INT32S drv_l1_dla_layctrl_prsv_set(INT32U layer, INT32U prsv_wt, INT32U prsv_ft, INT32U prsv_bs);
extern INT32S drv_l1_dla_layctrl_work_mode_set(INT32U layer, INT32U work_mode);
extern INT32S drv_l1_dla_idmactrl_dat_type_set(INT32U layer, INT32U dat_type);
extern INT32S drv_l1_dla_idmactrl_mer_num_set(INT32U layer, INT32U mer_num);
extern INT32S drv_l1_dla_idmactrl_crop_sn_set(INT32U layer, INT32U crop_en, INT32U sn_en);
extern INT32S drv_l1_dla_idmactrl_mer_str_addr(INT32U layer, INT32U addr);
extern INT32S drv_l1_dla_idmactrl_mer_ch_set(INT32U layer, INT32U mer_ch, INT32U mer_ch_las);
extern INT32S drv_l1_dla_cbuf_wtag_bk_set(INT32U layer, INT32U wtag_bk_num, INT32U wtag_bk_start);
extern INT32S drv_l1_dla_cbuf_out_bk_set(INT32U layer, INT32U out_bk_num, INT32U out_bk_start);
extern INT32S drv_l1_dla_cbuf_wt_bk_set(INT32U layer, INT32U wt_bk_num, INT32U wt_bk_start);
extern INT32S drv_l1_dla_cbuf_ft_bk_set(INT32U layer, INT32U ft_bk_num, INT32U ft_bk_start);
extern INT32S drv_l1_dla_idmactrl_cbuf_bs_set(INT32U layer, INT32U cbuf_bs_size);
extern INT32S drv_l1_dla_cmac_ofs_set(INT32U layer, INT32U cmac_bs_lsh, INT32U cmac_out_rsh);
extern INT32S drv_l1_dla_sdpctrl_sp_num_set(INT32U layer, INT32U sp_num);
extern INT32S drv_l1_dla_sdpctrl_act_rd_set(INT32U layer, INT32U act_rd_en);
extern INT32S drv_l1_dla_sdpctrl_odma_md_set(INT32U layer, INT32U odma_md);
extern INT32S drv_l1_dla_sdpctrl_pool_md_set(INT32U layer, INT32U pool_md);
extern INT32S drv_l1_dla_sdpctrl_act_md_set(INT32U layer, INT32U act_md);
extern INT32S drv_l1_dla_sdp_sp_ch_set(INT32U layer, INT32U sdp_sp_ch, INT32U sdp_sp_remain_ch);
extern INT32S drv_l1_dla_sdp_act_addr(INT32U layer, INT32U addr);
extern INT32S drv_l1_dla_csc_inft_size_set(INT32U layer, INT32U in_ft_w, INT32U in_ft_h);
extern INT32S drv_l1_dla_csc_ker_size_set(INT32U layer, INT32U ker_w, INT32U ker_h);
extern INT32S drv_l1_dla_crop_width_set(INT32U layer, INT32U crop_width);
extern INT32S drv_l1_dla_crop_stride_set(INT32U layer, INT32U crop_stride);
extern INT32S drv_l1_dla_sdp_relu_threshold_set(INT32U layer, INT32U relu_threshold);
extern INT32S drv_l1_dla_inf0_6_set(INT32U layer, INT32U inf_num, INT32U inf_col_size, INT32U inf_blk_size);
extern INT32S drv_l1_dla_inf7_set(INT32U layer, INT32U pool_plane, INT32U pad_size_T, INT32U PoolW_mul_StrW, INT32U PoolH_mul_StrH);
extern INT32S drv_l1_dla_inf8_12_set(INT32U layer, INT32U inf_num, INT32U inf_sp_cubic_size);
extern INT32S drv_l1_dla_layer_mode_start(void);
extern INT32S drv_l1_dla_soft_reset(void);
extern INT32S drv_l1_dla_auto_mode_src_addr(INT32U max_layer, INT32U addr);
extern INT32S drv_l1_dla_auto_mode_start(void);
extern INT32S drv_l1_dla_end_wait(INT32U wait);
extern INT32S drv_l1_dla_auto_start_flow_set(INT32U max_layer, INT32U inputImg_buf, INT32U out_buffer, INT32U ram_star, INT32U wt_start, INT32U size_start);
extern INT32S drv_l1_dla_auto_start_enable(INT32U max_layer, INT32U inputImg_buf, INT32U out_buffer, INT32U ram_star);
extern INT32S drv_l1_dla_auto_start_flow_with_input_buffer(INT32U max_layer, INT32U inputImg_buf, INT32U out_buffer, INT32U ram_star, INT32U wt_start, INT32U size_start, INT32U in_buf1, INT32U in_buf2);
extern INT32S drv_l1_dla_auto_start_dcache_flow_with_input_buffer(INT32U max_layer, INT32U inputImg_buf, INT32U out_buffer, INT32U ram_star, INT32U wt_start, INT32U size_start, INT32U in_buf1, INT32U in_buf2, INT32U dcache_size);
extern INT32S drv_l1_dla_auto_start_dcache_flow_enable(INT32U max_layer, INT32U inputImg_buf, INT32U out_buffer, INT32U dla_ram_star, INT32U wt_start, INT32U internal_buf, INT32U dcache_size);
extern INT32S drv_l1_dla_auto_start_dcache_flow_debug(INT32U max_layer, INT32U inputImg_buf, INT32U out_buffer, INT32U dla_ram_star, INT32U wt_start, INT32U internal_buf, INT32U dcache_size, INT32U stop_layer, INT32U layer_done);
extern INT32S drv_l1_dla_bypass_info_set(INT32S (*callback)(void));
extern INT32S drv_l1_dla_auto_start_dcache_bypass_info_enable(INT32U max_layer, INT32U inputImg_buf, INT32U out_buffer, INT32U dla_ram_star, INT32U wt_start, INT32U internal_buf, INT32U dcache_size, void *bypass_info);
extern INT32S drv_l1_dla_box_filter_start_dcache_flow_enable(INT32U inputImg_buf, INT32U out_buffer, INT32U dla_ram_star, INT32U wt_start, INT32U dcache_size, INT32U mode);
#endif

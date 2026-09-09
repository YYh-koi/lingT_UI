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
#ifndef __drv_l1_HDMI_H__
#define __drv_l1_HDMI_H__

enum {
	HDMI_OK = 0,
	HDMI_FAIL = -1
};

#define HDMI_50HZ_EN                    0x80000000

#define HDMI_AUD_32K       0
#define HDMI_AUD_44K       1
#define HDMI_AUD_48K       2
#define HDMI_AUD_OFF       3

#define HDMI_VID_480P      0
#define HDMI_VID_720P      1
#define HDMI_VID_1080P     2
#define HDMI_VID_1080I     3

#define HDCP_SUPPORT       0
#define CEC_SUPPORT        0

#define DEVICE_HDMI        1
#define DEVICE_DVI         0

//for test
#define HDMI_RD_SIM        0
#define HDMI_SCALER        0

#if HDMI_SCALER == 1
#define HDMI_SCALER_IN_WIDTH    640
#define HDMI_SCALER_IN_HEIGHT   480
#endif

// hdmi input sync
#define	B_IN_VSYNC_POLARITY		        15
#define	HDMI_IN_VSYNC_POSITIVE			(((INT32U) 1)<<B_IN_VSYNC_POLARITY)
#define	B_IN_HSYN_POLARITY		        14
#define	HDMI_IN_HSYNC_POSITIVE			(((INT32U) 1)<<B_IN_HSYN_POLARITY)
#define	B_HDMI_SYNC_POLARITY_EN		    13
#define	HDMI_SYNC_POLARITY_ENABLE	    (((INT32U) 1)<<B_HDMI_SYNC_POLARITY_EN)

// hdmi output sync
#define	B_OUT_VSYNC_POLARITY		    0
#define	HDMI_OUT_VSYNC_POSITIVE			(((INT32U) 1)<<B_OUT_VSYNC_POLARITY)
#define	B_OUT_HSYNC_POLARITY		    1
#define	HDMI_OUT_HSYNC_POSITIVE			(((INT32U) 1)<<B_OUT_HSYNC_POLARITY)

// hdmi system state
#define	B_HDMI_RX_ACTIVE		        0
#define	HDMI_RX_ACTIVE			        (((INT32U) 1)<<B_HDMI_RX_ACTIVE)
#define	B_HDMI_HOT_PLUG_IN		        1
#define	HDMI_HOT_PLUG_IN_ENABLE	        (((INT32U) 1)<<B_HDMI_HOT_PLUG_IN)

// hdmi intr1 enable
#define	B_HDCP_128FRAME_EN		        0
#define	HDCP_128FRAME_ENABLE			(((INT32U) 1)<<B_HDCP_128FRAME_EN)
#define	B_HDCP_16FRAME_EN		        1
#define	HDCP_16FRAME_ENABLE			    (((INT32U) 1)<<B_HDCP_16FRAME_EN)

// hdmi intr1 status
#define	B_HDCP_128FRAME_ACTIVE		    0
#define	HDCP_128FRAME_ACTIVE			(((INT32U) 1)<<B_HDCP_128FRAME_ACTIVE)
#define	B_HDCP_16FRAME_ACTIVE		    1
#define	HDCP_16FRAME_ACTIVE			    (((INT32U) 1)<<B_HDCP_16FRAME_ACTIVE)

//CEC INT EN
typedef enum
{
	CEC_INT_FREE_TIMEUP		    = BIT0,
	CEC_INT_FREE_LINE_ERR_SED	= BIT1,
	CEC_INT_FREE_BLK_SED	    = BIT2,
	CEC_INT_FREE_FRM_SED	    = BIT3,
	CEC_INT_NO_ACK_SED			= BIT4,
	CEC_INT_RISE_EDGE	        = BIT8,
	CEC_INT_FALL_EDGE	        = BIT9,
	CEC_INT_ERR_RCV	            = BIT10,
	CEC_INT_EOM_RCV		        = BIT11,
	CEC_INT_BLK_RCV			    = BIT12,
	CEC_INT_FRM_RCV				= BIT13,
} CEC_INT;
#define     HDMI_INT_CECF_INT   BIT3

typedef enum
{
	HDMI_INT_EN0= 0,
	HDMI_INT_EN1,
	HDMI_CEC_INT_EN,
} HDMI_INT_CH;

typedef struct
{
    INT32U mode;
    INT32U hs_width;
    INT32U h_period;
    INT32U vs_width;
    INT32U v_period;
    INT32U hs_start;
    INT32U hs_end;
    INT32U vs_start;
    INT32U vs_end;
    INT32U h_start;
    INT32U h_end;
    INT32U v_start;
    INT32U v_end;
    INT32U hdmi_out_width;
    INT32U hdmi_out_height;
} HDMI_TFT_TIMIG_CFG;

extern INT32S drv_l1_hdmi_audio_ctrl(INT32U status);
extern INT32S drv_l1_hdmi_dac_mute(INT32U status);
extern INT32S drv_l1_hdmi_init(INT32U vid_mode, INT32U aud_mode);
extern INT32S drv_l1_hdmi_exit(void);
extern INT32S drv_l1_hdmi_scaling_enable(INT32U img_width, INT32U img_height, INT32U img_offset);
extern void drv_l1_hdmi_scaling_up_size_set(INT32U hdmi_width, INT32U hdmi_height);
extern INT32S drv_l1_hdmi_scaling_disable(void);
extern void drv_l1_hdmi_audio_sample_rate_set(INT32U aud_mode);
extern void drv_l1_hdmi_set_ext_pll_clock(INT32U vid_mode);
extern void drv_l1_hdmi_set_audio_pll_clock(INT32U aud_mode);
extern void drv_l1_hdmi_inten_set(HDMI_INT_CH src, INT32U inten);
extern void drv_l1_hdmi_set_YCbCr_mode(INT32U enable);

//HDCP FUNCTION
extern void drv_l1_hdmi_hdcp_reset(void);
extern void drv_l1_hdmi_hdcp_set_repeater_mode(INT8U en);
extern void drv_l1_hdmi_hdcp_set_An_generate(INT8U en);
extern void drv_l1_hdmi_hdcp_read_An(INT16U *p);
extern void drv_l1_hdmi_hdcp_read_AKsv(INT8U *p);
extern void drv_l1_hdmi_hdcp_write_Bksv(INT16U *p);
extern INT32U drv_l1_hdmi_hdcp_read_Ri(void);
extern INT8U drv_l1_hdmi_hdcp_Ri_ready(void);
extern void drv_l1_hdmi_hdcp_enable(INT8U en, INT8U feature_en_1p1);

//CEC FUNCTION
extern void drv_l1_cec_timing_set(void);
extern void drv_l1_cec_tx_enable(INT8U en);
extern void drv_l1_cec_interrupt_enable_set(INT32U int_bit);
extern void drv_l1_cec_tx_send(void);
extern void drv_l1_cec_tx_para_set(INT32U para);
extern void drv_l1_cec_tx_data(INT8U *data);
extern void drv_l1_cec_rx_set(INT32U para);
extern void drv_l1_cec_rx_enable(INT32U para);
extern void drv_l1_cec_rx_addr_thd(INT32U addr_thd);

extern  INT16S drv_l1_hdmi_clock_set(INT16U enable);
extern void drv_l1_hmdi_disable_output_pin(void);



enum hdmi_packet_type {
	HDMI_PACKET_TYPE_NULL = 0x00,
	HDMI_PACKET_TYPE_AUDIO_CLOCK_REGEN = 0x01,
	HDMI_PACKET_TYPE_AUDIO_SAMPLE = 0x02,
	HDMI_PACKET_TYPE_GENERAL_CONTROL = 0x03,
	HDMI_PACKET_TYPE_ACP = 0x04,
	HDMI_PACKET_TYPE_ISRC1 = 0x05,
	HDMI_PACKET_TYPE_ISRC2 = 0x06,
	HDMI_PACKET_TYPE_ONE_BIT_AUDIO_SAMPLE = 0x07,
	HDMI_PACKET_TYPE_DST_AUDIO = 0x08,
	HDMI_PACKET_TYPE_HBR_AUDIO_STREAM = 0x09,
	HDMI_PACKET_TYPE_GAMUT_METADATA = 0x0a,
	/* + enum hdmi_infoframe_type */
};

enum hdmi_infoframe_type {
	HDMI_INFOFRAME_TYPE_VENDOR = 0x81,
	HDMI_INFOFRAME_TYPE_AVI = 0x82,
	HDMI_INFOFRAME_TYPE_SPD = 0x83,
	HDMI_INFOFRAME_TYPE_AUDIO = 0x84,
	HDMI_INFOFRAME_TYPE_DRM = 0x87,
};

enum hdmi_colorspace {
	HDMI_COLORSPACE_RGB,
	HDMI_COLORSPACE_YUV422,
	HDMI_COLORSPACE_YUV444,
	HDMI_COLORSPACE_YUV420,
	HDMI_COLORSPACE_RESERVED4,
	HDMI_COLORSPACE_RESERVED5,
	HDMI_COLORSPACE_RESERVED6,
	HDMI_COLORSPACE_IDO_DEFINED,
};

enum hdmi_scan_mode {
	HDMI_SCAN_MODE_NONE,
	HDMI_SCAN_MODE_OVERSCAN,
	HDMI_SCAN_MODE_UNDERSCAN,
	HDMI_SCAN_MODE_RESERVED,
};

enum hdmi_colorimetry {
	HDMI_COLORIMETRY_NONE,
	HDMI_COLORIMETRY_ITU_601,
	HDMI_COLORIMETRY_ITU_709,
	HDMI_COLORIMETRY_EXTENDED,
};

enum hdmi_picture_aspect {
	HDMI_PICTURE_ASPECT_NONE,
	HDMI_PICTURE_ASPECT_4_3,
	HDMI_PICTURE_ASPECT_16_9,
	HDMI_PICTURE_ASPECT_64_27,
	HDMI_PICTURE_ASPECT_256_135,
	HDMI_PICTURE_ASPECT_RESERVED,
};

enum hdmi_active_aspect {
	HDMI_ACTIVE_ASPECT_16_9_TOP = 2,
	HDMI_ACTIVE_ASPECT_14_9_TOP = 3,
	HDMI_ACTIVE_ASPECT_16_9_CENTER = 4,
	HDMI_ACTIVE_ASPECT_PICTURE = 8,
	HDMI_ACTIVE_ASPECT_4_3 = 9,
	HDMI_ACTIVE_ASPECT_16_9 = 10,
	HDMI_ACTIVE_ASPECT_14_9 = 11,
	HDMI_ACTIVE_ASPECT_4_3_SP_14_9 = 13,
	HDMI_ACTIVE_ASPECT_16_9_SP_14_9 = 14,
	HDMI_ACTIVE_ASPECT_16_9_SP_4_3 = 15,
};

enum hdmi_extended_colorimetry {
	HDMI_EXTENDED_COLORIMETRY_XV_YCC_601,
	HDMI_EXTENDED_COLORIMETRY_XV_YCC_709,
	HDMI_EXTENDED_COLORIMETRY_S_YCC_601,
	HDMI_EXTENDED_COLORIMETRY_OPYCC_601,
	HDMI_EXTENDED_COLORIMETRY_OPRGB,

	/* The following EC values are only defined in CEA-861-F. */
	HDMI_EXTENDED_COLORIMETRY_BT2020_CONST_LUM,
	HDMI_EXTENDED_COLORIMETRY_BT2020,
	HDMI_EXTENDED_COLORIMETRY_RESERVED,
};

enum hdmi_quantization_range {
	HDMI_QUANTIZATION_RANGE_DEFAULT,
	HDMI_QUANTIZATION_RANGE_LIMITED,
	HDMI_QUANTIZATION_RANGE_FULL,
	HDMI_QUANTIZATION_RANGE_RESERVED,
};

/* non-uniform picture scaling */
enum hdmi_nups {
	HDMI_NUPS_UNKNOWN,
	HDMI_NUPS_HORIZONTAL,
	HDMI_NUPS_VERTICAL,
	HDMI_NUPS_BOTH,
};

enum hdmi_ycc_quantization_range {
	HDMI_YCC_QUANTIZATION_RANGE_LIMITED,
	HDMI_YCC_QUANTIZATION_RANGE_FULL,
};

enum hdmi_content_type {
	HDMI_CONTENT_TYPE_GRAPHICS,
	HDMI_CONTENT_TYPE_PHOTO,
	HDMI_CONTENT_TYPE_CINEMA,
	HDMI_CONTENT_TYPE_GAME,
};

enum hdmi_metadata_type {
	HDMI_STATIC_METADATA_TYPE1 = 1,
};

enum hdmi_eotf {
	HDMI_EOTF_TRADITIONAL_GAMMA_SDR,
	HDMI_EOTF_TRADITIONAL_GAMMA_HDR,
	HDMI_EOTF_SMPTE_ST2084,
	HDMI_EOTF_BT_2100_HLG,
};

enum hdmi_spd_sdi {
	HDMI_SPD_SDI_UNKNOWN,
	HDMI_SPD_SDI_DSTB,
	HDMI_SPD_SDI_DVDP,
	HDMI_SPD_SDI_DVHS,
	HDMI_SPD_SDI_HDDVR,
	HDMI_SPD_SDI_DVC,
	HDMI_SPD_SDI_DSC,
	HDMI_SPD_SDI_VCD,
	HDMI_SPD_SDI_GAME,
	HDMI_SPD_SDI_PC,
	HDMI_SPD_SDI_BD,
	HDMI_SPD_SDI_SACD,
	HDMI_SPD_SDI_HDDVD,
	HDMI_SPD_SDI_PMP,
};

enum hdmi_audio_coding_type {
	HDMI_AUDIO_CODING_TYPE_STREAM,
	HDMI_AUDIO_CODING_TYPE_PCM,
	HDMI_AUDIO_CODING_TYPE_AC3,
	HDMI_AUDIO_CODING_TYPE_MPEG1,
	HDMI_AUDIO_CODING_TYPE_MP3,
	HDMI_AUDIO_CODING_TYPE_MPEG2,
	HDMI_AUDIO_CODING_TYPE_AAC_LC,
	HDMI_AUDIO_CODING_TYPE_DTS,
	HDMI_AUDIO_CODING_TYPE_ATRAC,
	HDMI_AUDIO_CODING_TYPE_DSD,
	HDMI_AUDIO_CODING_TYPE_EAC3,
	HDMI_AUDIO_CODING_TYPE_DTS_HD,
	HDMI_AUDIO_CODING_TYPE_MLP,
	HDMI_AUDIO_CODING_TYPE_DST,
	HDMI_AUDIO_CODING_TYPE_WMA_PRO,
	HDMI_AUDIO_CODING_TYPE_CXT,
};

enum hdmi_audio_channel_count {
	HDMI_AUDIO_CC_STREAM,
	HDMI_AUDIO_CC_CH2,
    HDMI_AUDIO_CC_CH3,
    HDMI_AUDIO_CC_CH4,
    HDMI_AUDIO_CC_CH5,
    HDMI_AUDIO_CC_CH6,
    HDMI_AUDIO_CC_CH7,
    HDMI_AUDIO_CC_CH8,
};

enum hdmi_audio_sample_size {
	HDMI_AUDIO_SAMPLE_SIZE_STREAM,
	HDMI_AUDIO_SAMPLE_SIZE_16,
	HDMI_AUDIO_SAMPLE_SIZE_20,
	HDMI_AUDIO_SAMPLE_SIZE_24,
};

enum hdmi_audio_sample_frequency {
	HDMI_AUDIO_SAMPLE_FREQUENCY_STREAM,
	HDMI_AUDIO_SAMPLE_FREQUENCY_32000,
	HDMI_AUDIO_SAMPLE_FREQUENCY_44100,
	HDMI_AUDIO_SAMPLE_FREQUENCY_48000,
	HDMI_AUDIO_SAMPLE_FREQUENCY_88200,
	HDMI_AUDIO_SAMPLE_FREQUENCY_96000,
	HDMI_AUDIO_SAMPLE_FREQUENCY_176400,
	HDMI_AUDIO_SAMPLE_FREQUENCY_192000,
};

enum hdmi_audio_coding_type_ext {
	/* Refer to Audio Coding Type (CT) field in Data Byte 1 */
	HDMI_AUDIO_CODING_TYPE_EXT_CT,

	/*
	 * The next three CXT values are defined in CEA-861-E only.
	 * They do not exist in older versions, and in CEA-861-F they are
	 * defined as 'Not in use'.
	 */
	HDMI_AUDIO_CODING_TYPE_EXT_HE_AAC,
	HDMI_AUDIO_CODING_TYPE_EXT_HE_AAC_V2,
	HDMI_AUDIO_CODING_TYPE_EXT_MPEG_SURROUND,

	/* The following CXT values are only defined in CEA-861-F. */
	HDMI_AUDIO_CODING_TYPE_EXT_MPEG4_HE_AAC,
	HDMI_AUDIO_CODING_TYPE_EXT_MPEG4_HE_AAC_V2,
	HDMI_AUDIO_CODING_TYPE_EXT_MPEG4_AAC_LC,
	HDMI_AUDIO_CODING_TYPE_EXT_DRA,
	HDMI_AUDIO_CODING_TYPE_EXT_MPEG4_HE_AAC_SURROUND,
	HDMI_AUDIO_CODING_TYPE_EXT_MPEG4_AAC_LC_SURROUND = 10,
};

enum hdmi_3d_structure {
	HDMI_3D_STRUCTURE_INVALID = -1,
	HDMI_3D_STRUCTURE_FRAME_PACKING = 0,
	HDMI_3D_STRUCTURE_FIELD_ALTERNATIVE,
	HDMI_3D_STRUCTURE_LINE_ALTERNATIVE,
	HDMI_3D_STRUCTURE_SIDE_BY_SIDE_FULL,
	HDMI_3D_STRUCTURE_L_DEPTH,
	HDMI_3D_STRUCTURE_L_DEPTH_GFX_GFX_DEPTH,
	HDMI_3D_STRUCTURE_TOP_AND_BOTTOM,
	HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF = 8,
};

#endif 		/* __drv_l1_HDMI_H__ */


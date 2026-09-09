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

//Include files
#include "drv_l1.h"

// for TFT2 scaler is enabled
static INT32U g_hdmi_img_width = 0;		// Image width after scaling down
static INT32U g_hdmi_img_height = 0;		// Image height after scaling down
static INT32U g_hdmi_img_offset = 0;		// If you don't know how to setup, set it to zero.

static void (*cec_user_isr)(INT32U) = NULL;
static void (*hdmi_user_isr0)(INT32U) = NULL;
static void (*hdmi_user_isr1)(INT32U) = NULL;

static INT32S hdmi_hwYCbCr_mode = 0;

#define HDMI_CLOCK_SOURCE                 CLK_EN5_HDMI
#define HDMI_AUDIO_CLOCK_SOURCE           CLK_EN5_HDMI_AUDIO

void drv_l1_hdmi_scaling_up_size_set(INT32U hdmi_width, INT32U hdmi_height)
{
    // setup TFT2 scaler, scale up to hdmi_width x hdmi_height
    INT32U W_factor, H_factor;

    W_factor = 65536 * g_hdmi_img_width / hdmi_width;
    if (W_factor>=65536)
    {
        W_factor = 65535;
    }

    H_factor = 65536 * g_hdmi_img_height / hdmi_height;
    if (H_factor>=65536)
    {
        H_factor = 65535;
    }
    R_TFT2_SCA_OUT_RES = (hdmi_height<<16) | hdmi_width;
    R_TFT2_SCA_OFFSET = g_hdmi_img_offset;
    R_TFT2_SCA_FACTOR =  (H_factor<<16) | W_factor;
}

/////////////////// HDMI Setting ///////////////////
#define HAL_HDMI_PKT_HVLD				0x10000000
#define HAL_HDMI_PKT_VVLD				0x20000000
#define HAL_HDMI_PKT_AUTO				0x40000000
#define HAL_HDMI_PKT_SEND				0x80000000

#define HDMI_ACR_PACKET					0x01
#define HDMI_AUDIOSAMPLE_PACKET			0x02
#define HDMI_GENERALCTRL_PACKET			0x03
#define HDMI_ACP_PACKET					0x04
#define HDMI_ISRC1_PACKET				0x05
#define HDMI_ISRC2_PACKET				0x06
#define HDMI_1BITAUDIOSAMPLE_PACKET		0x07
#define HDMI_DSTAUDIO_PACKET			0x08
#define HDMI_HBRAUDIOSTREAM_PACKET		0x09
#define HDMI_GAMUTMETADATA_PACKET		0x0a
#define HDMI_VENDORINFOFRAME_PACKET		0x81
#define HDMI_AVIINFOFRAME_PACKET		0x82
#define HDMI_SPDINFOFRAME_PACKET		0x83
#define HDMI_AUDIOINFOFRAME_PACKET		0x84
#define HDMI_MPEGINFOFRAME_PACKET		0x85

#define HDMI_VS0	0
#define HDMI_VS1	1
#define HDMI_VS2	2
#define HDMI_VS3	3

enum Sampling
{
    Sampling_22K = 0x4000000,
    Sampling_44K = 0x0000000,
    Sampling_88K = 0x8000000,
    Sampling_176K = 0xC000000,
    Sampling_24K = 0x6000000,
    Sampling_48K = 0x2000000,
    Sampling_96K = 0xA000000,
    Sampling_192K = 0xE000000,
    Sampling_32K = 0x3000000,
    Sampling_none = 0x1000000,
    Sampling_768K = 0x9000000
};

enum Original_sampling
{
    Original_sampling_no_match = 0x00,
    Original_sampling_16K = 0x80,
    Original_sampling_Reserved1 = 0x40,
    Original_sampling_32K = 0xC0,
    Original_sampling_12K = 0x20,
    Original_sampling_11K = 0xA0,
    Original_sampling_8K = 0x60,
    Original_sampling_Reserved2 = 0xE0,
    Original_sampling_192K = 0x10,
    Original_sampling_24K = 0x90,
    Original_sampling_96K = 0x50,
    Original_sampling_48K = 0xD0,
    Original_sampling_176K = 0x30,
    Original_sampling_22K = 0xB0,
    Original_sampling_88K = 0x70,
    Original_sampling_44K = 0xF0
};

typedef volatile struct reg_hdmi_tx_s
{
    INT32U hdmi_vendor_id;                          //0x0000
    INT32U hdmi_device_id;                          //0x0004
    INT32U hdmi_revision;                           //0x0008
    INT32U hdmi_reserved_0003;                      //0x000C
    INT32U hdmi_reserved_0004;                      //0x0010
    INT32U hdmi_pwr_ctrl;                           //0x0014
    INT32U hdmi_sw_reset;                           //0x0018
    INT32U hdmi_system_status;                      //0x001C
    INT32U hdmi_system_ctrl1;                       //0x0020
    INT32U hdmi_reserved_0009;                      //0x0024
    INT32U hdmi_system_ctrl2;                       //0x0028
    INT32U hdmi_system_ctrl3;                       //0x002C
    INT32U hdmi_system_ctrl4;                       //0x0030
    INT32U hdmi_reserved_000d;                      //0x0034
    INT32U hdmi_reserved_000e;                      //0x0038
    INT32U hdmi_system_ctrl5;                       //0x003C
    INT32U hdmi_hdcp_ctrl1;                         //0x0040
    INT32U hdmi_bksv12;                             //0x0044
    INT32U hdmi_bksv34;                             //0x0048
    INT32U hdmi_bksv5;                              //0x004C
    INT32U hdmi_mi12;                               //0x0050
    INT32U hdmi_mi34;                               //0x0054
    INT32U hdmi_mi56;                               //0x0058
    INT32U hdmi_mi78;                               //0x005C
    INT32U hdmi_aksv12;                             //0x0060
    INT32U hdmi_aksv34;                             //0x0064
    INT32U hdmi_aksv5;                              //0x0068
    INT32U hdmi_ri_cmp;                             //0x006C
    INT32U hdmi_rj_cmp;                             //0x0070
    INT32U hdmi_ri_cmp_set;                         //0x0074
    INT32U hdmi_framecnt;                           //0x0078
    INT32U hdmi_reserved_001f;                      //0x007C

    INT32U hdmi_autoricmp;                          //0x0080
    INT32U hdmi_hdcpsts;                            //0x0084
    INT32U hdmi_rxri;                               //0x0088
    INT32U hdmi_rxpj;                               //0x008C
    INT32U hdmi_hdcp_test;                          //0x0090
    INT32U hdmi_hdcp_test_result;                   //0x0094
    INT32U hdmi_apo_spdif_chnl_sts0;                //0x0098
    INT32U hdmi_apo_spdif_chnl_sts1;                //0x009C
    INT32U hdmi_apo_spdif_chnl_sts2;                //0x00A0
    INT32U hdmi_acr_config2;                        //0x00A4
    INT32U hdmi_asp_urg_th;                         //0x00A8
    INT32U hdmi_reserved_010b;                      //0x00AC
    INT32U hdmi_blvctrl;                            //0x00B0
    INT32U hdmi_blvpa0;                             //0x00B4
    INT32U hdmi_blvpa1;                             //0x00B8
    INT32U hdmi_blvpa2;                             //0x00BC
    INT32U hdmi_video_ctrl1;                        //0x00C0
    INT32U hdmi_video_in_h_sts;                     //0x00C4
    INT32U hdmi_video_in_v_sts;                     //0x00C8
    INT32U hdmi_video_dither_ctrl;                  //0x00CC
    INT32U hdmi_video_pat_gen1;                     //0x00D0
    INT32U hdmi_video_pat_gen2;                     //0x00D4
    INT32U hdmi_video_pat_gen3;                     //0x00D8
    INT32U hdmi_video_pat_gen4;                     //0x00DC
    INT32U hdmi_video_pat_gen5;                     //0x00E0
    INT32U hdmi_video_pat_gen6;                     //0x00E4
    INT32U hdmi_video_pat_gen7;                     //0x00E8
    INT32U hdmi_video_pat_gen8;                     //0x00EC
    INT32U hdmi_video_pat_gen9;                     //0x00F0
    INT32U hdmi_csc_coeff1;                         //0x00F4
    INT32U hdmi_csc_coeff2;                         //0x00F8
    INT32U hdmi_csc_coeff3;                         //0x00FC

    INT32U hdmi_csc_coeff4;                         //0x0100
    INT32U hdmi_csc_coeff7;                         //0x0104
    INT32U hdmi_csc_coeff8;                         //0x0108
    INT32U hdmi_csc_coeff9;                         //0x010C
    INT32U hdmi_csc_coeff10;                        //0x0110
    INT32U hdmi_csc_coeff13;                        //0x0114
    INT32U hdmi_csc_coeff14;                        //0x0118
    INT32U hdmi_csc_coeff15;                        //0x011C
    INT32U hdmi_csc_coeff16;                        //0x0120
    INT32U hdmi_csc_coeff19;                        //0x0124
    INT32U hdmi_csc_coeff20;                        //0x0128
    INT32U hdmi_csc_coeff21;                        //0x012C
    INT32U hdmi_video_format;                       //0x0130
    INT32U hdmi_reserved_020d;                      //0x0134
    INT32U hdmi_audio_sw_cts1;                      //0x0138
    INT32U hdmi_audio_sw_cts2;                      //0x013C
    INT32U hdmi_audio_ctrl1;                        //0x0140
    INT32U hdmi_audio_ctrl2;                        //0x0144
    INT32U hdmi_audio_spdif_ctrl;                   //0x0148
    INT32U hdmi_audio_spdif_sw_ui;                  //0x014C
    INT32U hdmi_audio_spdif_sw_3ui;                 //0x0150
    INT32U hdmi_audio_spdif_hw_ui;                  //0x0154
    INT32U hdmi_audio_spdif_hw_3ui;                 //0x0158
    INT32U hdmi_audio_chnl_sts1;                    //0x015C
    INT32U hdmi_audio_chnl_sts2;                    //0x0160
    INT32U hdmi_audio_chnl_sts3;                    //0x0164
    INT32U hdmi_arc_config1;                        //0x0168
    INT32U hdmi_arc_n_value1;                       //0x016C
    INT32U hdmi_arc_n_value2;                       //0x0170
    INT32U hdmi_arc_hw_cts1;                        //0x0174
    INT32U hdmi_arc_hw_cts2;                        //0x0178
    INT32U hdmi_reserved_021f;                      //0x017C

    INT32U hdmi_intr_ctrl;                          //0x0180
    INT32U hdmi_intr0_unmask;                       //0x0184
    INT32U hdmi_intr1_unmask;                       //0x0188
    INT32U hdmi_intr2_unmask;                       //0x018C
    INT32U hdmi_intr0_sts;                          //0x0190
    INT32U hdmi_intr1_sts;                          //0x0194
    INT32U hdmi_intr2_sts;                          //0x0198
    INT32U hdmi_ddc_master_set;                     //0x019C
    INT32U hdmi_ddc_slv_device_addr;                //0x01A0
    INT32U hdmi_ddc_slv_seg_addr;                   //0x01A4
    INT32U hdmi_ddc_slv_reg_offset;                 //0x01A8
    INT32U hdmi_ddc_data_cnt;                       //0x01AC
    INT32U hdmi_ddc_cmd;                            //0x01B0
    INT32U hdmi_ddc_sts;                            //0x01B4
    INT32U hdmi_ddc_data;                           //0x01B8
    INT32U hdmi_ddc_fifodata_cnt;                   //0x01BC
    INT32U hdmi_cec_config1;                        //0x01C0
    INT32U hdmi_cec_gpio;                           //0x01C4
    INT32U hdmi_cec_data_send;                      //0x01C8
    INT32U hdmi_cec_config2;                        //0x01CC
    INT32U hdmi_cec_config3;                        //0x01D0
    INT32U hdmi_cec_cmd;                            //0x01D4
    INT32U hdmi_cec_timer;                          //0x01D8
    INT32U hdmi_cec_timer_sts;                      //0x01DC
    INT32U hdmi_cec_config4;                        //0x01E0
    INT32U hdmi_cec_data_rcv;                       //0x01E4
    INT32U hdmi_cec_sts;                            //0x01E8
    INT32U hdmi_cec_sts2;                           //0x01EC
    INT32U hdmi_cec_sts3;                           //0x01F0
    INT32U hdmi_cec_sts4;                           //0x01F4
    INT32U hdmi_reserved_031e;                      //0x01F8
    INT32U hdmi_reserved_031f;                      //0x01FC

    INT32U hdmi_cec_send_byte12;                    //0x0200
    INT32U hdmi_cec_send_byte34;                    //0x0204
    INT32U hdmi_cec_send_byte56;                    //0x0208
    INT32U hdmi_cec_send_byte78;                    //0x020C
    INT32U hdmi_cec_send_byte910;                   //0x0210
    INT32U hdmi_cec_send_byte1112;                  //0x0214
    INT32U hdmi_cec_send_byte1314;                  //0x0218
    INT32U hdmi_cec_send_byte1516;                  //0x021C
    INT32U hdmi_cec_rcv_byte12;                     //0x0220
    INT32U hdmi_cec_rcv_byte34;                     //0x0224
    INT32U hdmi_cec_rcv_byte56;                     //0x0228
    INT32U hdmi_cec_rcv_byte78;                     //0x022C
    INT32U hdmi_cec_rcv_byte910;                    //0x0230
    INT32U hdmi_cec_rcv_byte1112;                   //0x0234
    INT32U hdmi_cec_rcv_byte1314;                   //0x0238
    INT32U hdmi_cec_rcv_byte1516;                   //0x023C
    INT32U hdmi_universal_infoframe_hb01;           //0x0240
    INT32U hdmi_universal_infoframe_hb2;            //0x0244
    INT32U hdmi_universal_packetbody01;             //0x0248
    INT32U hdmi_universal_packetbody23;             //0x024C
    INT32U hdmi_universal_packetbody45;             //0x0250
    INT32U hdmi_universal_packetbody67;             //0x0254
    INT32U hdmi_universal_packetbody89;             //0x0258
    INT32U hdmi_universal_packetbody1011;           //0x025C
    INT32U hdmi_universal_packetbody1213;           //0x0260
    INT32U hdmi_universal_packetbody1415;           //0x0264
    INT32U hdmi_universal_packetbody1617;           //0x0268
    INT32U hdmi_universal_packetbody1819;           //0x026C
    INT32U hdmi_universal_packetbody2021;           //0x0270
    INT32U hdmi_universal_packetbody2223;           //0x0274
    INT32U hdmi_universal_packetbody2425;           //0x0278
    INT32U hdmi_universal_packetbody2627;           //0x027C

    INT32U hdmi_infoframe_ctrl1;                    //0x0280
    INT32U hdmi_infoframe_ctrl2;                    //0x0284
    INT32U hdmi_infoframe_ctrl3;                    //0x0288
    INT32U hdmi_infoframe_ctrl4;                    //0x028C
    INT32U hdmi_infoframe_ctrl5;                    //0x0290
    INT32U hdmi_infoframe_ctrl6;                    //0x0294
    INT32U hdmi_avi_infoframe01;                    //0x0298
    INT32U hdmi_avi_infoframe23;                    //0x029C
    INT32U hdmi_avi_infoframe45;                    //0x02A0
    INT32U hdmi_avi_infoframe67;                    //0x02A4
    INT32U hdmi_avi_infoframe89;                    //0x02A8
    INT32U hdmi_avi_infoframe1011;                  //0x02AC
    INT32U hdmi_avi_infoframe1213;                  //0x02B0
    INT32U hdmi_audio_infoframe01;                  //0x02B4
    INT32U hdmi_audio_infoframe23;                  //0x02B8
    INT32U hdmi_audio_infoframe45;                  //0x02BC
    INT32U hdmi_audio_infoframe67;                  //0x02C0
    INT32U hdmi_audio_infoframe89;                  //0x02C4
    INT32U hdmi_audio_infoframe1011;                //0x02C8
    INT32U hdmi_general_control_packet;             //0x02CC
    INT32U hdmi_acp_packet_header;                  //0x02D0
    INT32U hdmi_acp_packet_body01;                  //0x02D4
    INT32U hdmi_acp_packet_body23;                  //0x02D8
    INT32U hdmi_acp_packet_body45;                  //0x02DC
    INT32U hdmi_acp_packet_body67;                  //0x02E0
    INT32U hdmi_acp_packet_body89;                  //0x02E4
    INT32U hdmi_acp_packet_body1011;                //0x02E8
    INT32U hdmi_acp_packet_body1213;                //0x02EC
    INT32U hdmi_acp_packet_body1415;                //0x02F0
    INT32U hdmi_acp_packet_body1617;                //0x02F4
    INT32U hdmi_acp_packet_body1819;                //0x02F8
    INT32U hdmi_acp_packet_body2021;                //0x02FC

    INT32U hdmi_acp_packet_body2223;                //0x0300
    INT32U hdmi_acp_packet_body2425;                //0x0304
    INT32U hdmi_acp_packet_body2627;                //0x0308
    INT32U hdmi_vendor_specific_infoframe;          //0x030C
    INT32U hdmi_vendor_specific_length;             //0x0310
    INT32U hdmi_spd_infoframe_pb01;                 //0x0314
    INT32U hdmi_spd_infoframe_pb23;                 //0x0318
    INT32U hdmi_spd_infoframe_pb45;                 //0x031C
    INT32U hdmi_spd_infoframe_pb67;                 //0x0320
    INT32U hdmi_spd_infoframe_pb89;                 //0x0324
    INT32U hdmi_spd_infoframe_pb1011;               //0x0328
    INT32U hdmi_spd_infoframe_pb1213;               //0x032C
    INT32U hdmi_spd_infoframe_pb1415;               //0x0330
    INT32U hdmi_spd_infoframe_pb1617;               //0x0334
    INT32U hdmi_spd_infoframe_pb1819;               //0x0338
    INT32U hdmi_spd_infoframe_pb2021;               //0x033C
    INT32U hdmi_spd_infoframe_pb2223;               //0x0340
    INT32U hdmi_spd_infoframe_pb2425;               //0x0344
    INT32U hdmi_isrc_packet_header;                 //0x0348
    INT32U hdmi_isrc1_pb01;                         //0x034C
    INT32U hdmi_isrc1_pb23;                         //0x0350
    INT32U hdmi_isrc1_pb45;                         //0x0354
    INT32U hdmi_isrc1_pb67;                         //0x0358
    INT32U hdmi_isrc1_pb89;                         //0x035C
    INT32U hdmi_isrc1_pb1011;                       //0x0360
    INT32U hdmi_isrc1_pb1213;                       //0x0364
    INT32U hdmi_isrc1_pb1415;                       //0x0368
    INT32U hdmi_isrc2_pb01;                         //0x036C
    INT32U hdmi_isrc2_pb23;                         //0x0370
    INT32U hdmi_isrc2_pb45;                         //0x0374
    INT32U hdmi_isrc2_pb67;                         //0x0378
    INT32U hdmi_isrc2_pb89;                         //0x037C

    INT32U hdmi_isrc2_pb1011;                       //0x0380
    INT32U hdmi_isrc2_pb1213;                       //0x0384
    INT32U hdmi_isrc2_pb1415;                       //0x0388
    INT32U hdmi_gamut_metadata_packet_header;       //0x038C
    INT32U hdmi_gamut_metadata_packet_pb01;         //0x0390
    INT32U hdmi_gamut_metadata_packet_pb23;         //0x0394
    INT32U hdmi_gamut_metadata_packet_pb45;         //0x0398
    INT32U hdmi_gamut_metadata_packet_pb67;         //0x039C
    INT32U hdmi_gamut_metadata_packet_pb89;         //0x03A0
    INT32U hdmi_gamut_metadata_packet_pb1011;       //0x03A4
    INT32U hdmi_gamut_metadata_packet_pb1213;       //0x03A8
    INT32U hdmi_gamut_metadata_packet_pb1415;       //0x03AC
    INT32U hdmi_gamut_metadata_packet_pb1617;       //0x03B0
    INT32U hdmi_gamut_metadata_packet_pb1819;       //0x03B4
    INT32U hdmi_gamut_metadata_packet_pb2021;       //0x03B8
    INT32U hdmi_gamut_metadata_packet_pb2223;       //0x03BC
    INT32U hdmi_gamut_metadata_packet_pb2425;       //0x03C0
    INT32U hdmi_gamut_metadata_packet_pb2627;       //0x03C4
    INT32U hdmi_mpeg_source_infoframe_pb01;         //0x03C8
    INT32U hdmi_mpeg_source_infoframe_pb23;         //0x03CC
    INT32U hdmi_mpeg_source_infoframe_pb45;         //0x03D0
    INT32U hdmi_mpeg_source_infoframe_pb67;         //0x03D4
    INT32U hdmi_mpeg_source_infoframe_pb89;         //0x03D8
    INT32U hdmi_mpeg_source_infoframe_pb10;         //0x03DC
    INT32U hdmi_phy_ctrl1;                          //0x03E0
    INT32U hdmi_phy_ctrl2;                          //0x03E4
    INT32U hdmi_tmdstx_ctrl1;                       //0x03E8
    INT32U hdmi_tmdstx_ctrl2;                       //0x03EC
    INT32U hdmi_tmdstx_ctrl3;                       //0x03F0
    INT32U hdmi_tmdstx_ctrl4;                       //0x03F4
    INT32U hdmi_tmdstx_ctrl5;                       //0x03F8
    INT32U hdmi_io_ctrl;                            //0x03FC
} reg_hdmi_tx_t;

#define hdmi_tx_regs ((volatile reg_hdmi_tx_t *) HDMI_TX_BASE)

typedef struct AUD_CH_Reg_s
{
    volatile unsigned int status0;
    volatile unsigned int status1;
    volatile unsigned int status2;
    volatile unsigned int status3;
} AUD_CH_Reg_t;

typedef struct
{
    unsigned char type;
    unsigned char version;
    unsigned char length;
    unsigned char flag;
    unsigned char CheckSum;
    unsigned char Body[27];
} HDMI_PACKET;

static void hdmi_packet_checksum(void *data)
{
    HDMI_PACKET *Pkt = (HDMI_PACKET *)(data);
    INT8U *body = (INT8U *)(Pkt->Body);
    int CheckSum = 0, i;

    CheckSum -= Pkt->type + Pkt->version + Pkt->length;
    for(i=0; i<Pkt->length; i++)
        CheckSum -= *body++;
    Pkt->CheckSum = (INT8U)(CheckSum & 0xFF);
}

static void hdmi_avi_info_pkt(HDMI_PACKET *Pkt, INT8U VID, INT8U aspect_ratio)
{
    memset((void*)Pkt, 0, sizeof(HDMI_PACKET));
    Pkt->type = HDMI_AVIINFOFRAME_PACKET;
    Pkt->version = 2;
    Pkt->length = 13;
    if(hdmi_hwYCbCr_mode)
        Pkt->Body[0] = 0x2C;
    else
        Pkt->Body[0] = 0x0C;
    Pkt->Body[1] = (HDMI_COLORIMETRY_ITU_709 << 6) | ((aspect_ratio&0x3)<<4) |  HDMI_ACTIVE_ASPECT_PICTURE;
    Pkt->Body[2] = 0x84;
    Pkt->Body[3] = VID;	    // VideoIdCode
    Pkt->Body[4] = 0x00;	// PixelRep
    Pkt->Body[5] = 0x00;
    Pkt->Body[6] = 0x00;
    Pkt->Body[7] = 0x00;
    Pkt->Body[8] = 0x00;
    Pkt->Body[9] = 0x00;
    Pkt->Body[10] = 0x00;
    Pkt->Body[11] = 0x00;
    Pkt->Body[12] = 0x00;

    hdmi_packet_checksum(Pkt);

    hdmi_tx_regs->hdmi_avi_infoframe01 = (Pkt->Body[0] << 8) | (Pkt->CheckSum);
    hdmi_tx_regs->hdmi_avi_infoframe23 = (Pkt->Body[2] << 8) | (Pkt->Body[1]);
    hdmi_tx_regs->hdmi_avi_infoframe45 = (Pkt->Body[4] << 8) | (Pkt->Body[3]);
    hdmi_tx_regs->hdmi_avi_infoframe67 = (Pkt->Body[6] << 8) | (Pkt->Body[5]);
    hdmi_tx_regs->hdmi_avi_infoframe89 = (Pkt->Body[8] << 8) | (Pkt->Body[7]);
    hdmi_tx_regs->hdmi_avi_infoframe1011 = (Pkt->Body[10] << 8) | (Pkt->Body[9]);
    hdmi_tx_regs->hdmi_avi_infoframe1213 = (Pkt->Body[12] << 8) | (Pkt->Body[11]);
}

static void hdmi_audio_info_frame_pkt(HDMI_PACKET *Pkt)
{
    memset((void*)Pkt, 0, sizeof(HDMI_PACKET));
    Pkt->type = HDMI_AUDIOINFOFRAME_PACKET;
    Pkt->version = 1;
    Pkt->length = 10;

    Pkt->Body[0] = (HDMI_AUDIO_CODING_TYPE_STREAM << 4 ) | HDMI_AUDIO_CC_CH2;
    Pkt->Body[1] = (HDMI_AUDIO_SAMPLE_FREQUENCY_STREAM << 2 ) | HDMI_AUDIO_SAMPLE_SIZE_STREAM;
    Pkt->Body[2] = 0x00;//CA[7:0]
    Pkt->Body[3] = 0x00;
    Pkt->Body[4] = 0x00;
    Pkt->Body[5] = 0x00;
    Pkt->Body[6] = 0x00;
    Pkt->Body[7] = 0x00;
    Pkt->Body[8] = 0x00;
    Pkt->Body[9] = 0x00;

    hdmi_packet_checksum(Pkt);

    hdmi_tx_regs->hdmi_audio_infoframe01 = (Pkt->Body[0] << 8) | (Pkt->CheckSum);
    hdmi_tx_regs->hdmi_audio_infoframe23 = (Pkt->Body[2] << 8) | (Pkt->Body[1]);
    hdmi_tx_regs->hdmi_audio_infoframe45 = (Pkt->Body[4] << 8) | (Pkt->Body[3]);
    hdmi_tx_regs->hdmi_audio_infoframe67 = (Pkt->Body[6] << 8) | (Pkt->Body[5]);
    hdmi_tx_regs->hdmi_audio_infoframe89 = (Pkt->Body[8] << 8) | (Pkt->Body[7]);
    hdmi_tx_regs->hdmi_audio_infoframe1011 = (Pkt->Body[9]);

    //hdmi_tx_regs->hdmi_infoframe_ctrl1 |= 0x300;//can't set here , not avi info
}

static void hdmi_audio_channel_status(INT32U sampling_rate, INT32U org_sampling_frequency)
{
    hdmi_tx_regs->hdmi_audio_chnl_sts1 = 0x8004;
    hdmi_tx_regs->hdmi_audio_chnl_sts2 = 0x10 | (sampling_rate >> 16);
    hdmi_tx_regs->hdmi_audio_chnl_sts3 = 0x2;
}

void drv_l1_hdmi_set_general_ctrl_packet(void *RegBase)
{
}

void drv_l1_hdmi_set_YCbCr_mode(INT32U enable)
{
    if(enable)
        hdmi_hwYCbCr_mode = 1;
    else
        hdmi_hwYCbCr_mode = 0;
}

void drv_l1_hdmi_set_acr_packet(INT32U N, INT32U CTS)
{
    //ACR packets
    // acr_sw_CTS[15:0] -> 0xd0d00138[15:0],
    hdmi_tx_regs->hdmi_audio_sw_cts1 = CTS & 0xFFFF;
    // acr_sw_CTS[19:16] -> 0xd0d0013C[3:0],
    hdmi_tx_regs->hdmi_audio_sw_cts2 = (CTS>>16) & 0xF;
    //acr_N[15:0] -> 0xd0d0016C[15:0]=16'h1800,
    hdmi_tx_regs->hdmi_arc_n_value1 = N & 0xFFFF;
    //acr_N[19:16] -> 0xd0d00170[3:0],
    hdmi_tx_regs->hdmi_arc_n_value2 = (N>>16) & 0xF;
    //0xd0d00168[0], to enable acr_pkt_en
    //0xd0d00168[1], to enable sw_CTS_en
    //0xd0d00168[4:2], ACR MCLK over Fs
    //3・b000: MCLK = 128Fs
    //3・b001: MCLK = 256Fs
    //3・b010: MCLK = 384Fs
    //3・b011: MCLK = 512Fs
    //3・b100: MCLK = 768Fs
    //3・b101: MCLK = 1024Fs
    //3・b110: MCLK = 1152Fs
    //3・b111: MCLK = 192Fs
    //0xd0d00168[5], Fs_over_192k
    //0xd0d00168[6], ACR CTS valid flag, 0:error, 1:valid
    //0xd0d00168[7], APt_acr_CTS_error, ACR CTS value is too small(CTS <1024),
    //When this bit = 1, check the audio configuration
    hdmi_tx_regs->hdmi_arc_config1 = (BIT0 | BIT2); // disable sw_CTS_en + MCLK = 256Fs
}

void drv_l1_hdmi_set_acr_packet_fs(INT32U aud, INT32U N, INT32U CTS)
{
    //ACR packets
    // acr_sw_CTS[15:0] -> 0xd0d00138[15:0],
    hdmi_tx_regs->hdmi_audio_sw_cts1 = CTS & 0xFFFF;
    // acr_sw_CTS[19:16] -> 0xd0d0013C[3:0],
    hdmi_tx_regs->hdmi_audio_sw_cts2 = (CTS>>16) & 0xF;
    //acr_N[15:0] -> 0xd0d0016C[15:0]=16'h1800,
    hdmi_tx_regs->hdmi_arc_n_value1 = N & 0xFFFF;
    //acr_N[19:16] -> 0xd0d00170[3:0],
    hdmi_tx_regs->hdmi_arc_n_value2 = (N>>16) & 0xF;
    //0xd0d00168[0], to enable acr_pkt_en
    //0xd0d00168[1], to enable sw_CTS_en
    //0xd0d00168[4:2], ACR MCLK over Fs
    //3・b000: MCLK = 128Fs
    //3・b001: MCLK = 256Fs
    //3・b010: MCLK = 384Fs
    //3・b011: MCLK = 512Fs
    //3・b100: MCLK = 768Fs
    //3・b101: MCLK = 1024Fs
    //3・b110: MCLK = 1152Fs
    //3・b111: MCLK = 192Fs
    //0xd0d00168[5], Fs_over_192k
    //0xd0d00168[6], ACR CTS valid flag, 0:error, 1:valid
    //0xd0d00168[7], APt_acr_CTS_error, ACR CTS value is too small(CTS <1024),
    //When this bit = 1, check the audio configuration
    if(aud == HDMI_AUD_32K)
        hdmi_tx_regs->hdmi_arc_config1 = (BIT0 | BIT3); // disable sw_CTS_en + MCLK = 384Fs
    else
        hdmi_tx_regs->hdmi_arc_config1 = (BIT0 | BIT2); // disable sw_CTS_en + MCLK = 256Fs
}

void drv_l1_hdmi_set_audio_sample_packet(void *RegBase, INT32U ch)
{

}

void drv_l1_hdmi_set_input_sync_timing(INT32U h_sync, INT32U v_sync, INT32U enable)
{
    hdmi_tx_regs->hdmi_video_ctrl1 &= ~(B_IN_VSYNC_POLARITY|B_IN_HSYN_POLARITY|B_HDMI_SYNC_POLARITY_EN);

    if(enable)
    {
        hdmi_tx_regs->hdmi_video_ctrl1 |= HDMI_SYNC_POLARITY_ENABLE;
        if(h_sync)
            hdmi_tx_regs->hdmi_video_ctrl1 |= HDMI_IN_HSYNC_POSITIVE;

        if(v_sync)
            hdmi_tx_regs->hdmi_video_ctrl1 |= HDMI_IN_VSYNC_POSITIVE;
    }
}

void drv_l1_hdmi_set_output_sync_timing(INT32U h_sync, INT32U v_sync)
{
    hdmi_tx_regs->hdmi_video_format &= ~(B_OUT_VSYNC_POLARITY|B_OUT_HSYNC_POLARITY);
    if(h_sync)
        hdmi_tx_regs->hdmi_video_format |= HDMI_OUT_VSYNC_POSITIVE;

    if(v_sync)
        hdmi_tx_regs->hdmi_video_format |= HDMI_OUT_HSYNC_POSITIVE;
}

void drv_l1_hdmi_set_mode(INT8U mode)
{
    if(mode)
        hdmi_tx_regs->hdmi_system_ctrl1 |= BIT0;//HDMI mode
    else
        hdmi_tx_regs->hdmi_system_ctrl1 &= ~BIT0;//DVI mode
}

void drv_l1_hdmi_config_phy(INT32U phy1, INT32U phy2)
{
    hdmi_tx_regs->hdmi_phy_ctrl1 = phy1;
    hdmi_tx_regs->hdmi_phy_ctrl2 = (BIT13|phy2);
}

/**
 * @brief   The function is called after HDMI initialization. When it is called, both DAC of GPM4xxA and HDMI device will output the same sound.
 * @param   status: on/off HDMI audio
 * @return   HDMI_OK: Function is successful. HDMI_FAIL: Function is fail.
 */
INT32S drv_l1_hdmi_audio_ctrl(INT32U status)
{
    //reset module, to enable audio FIFO
    //BIT[0]:Software Reset
    //BIT[1]:Audio FIFO Reset
    //BIT[2]:HDCP Engine Reset
    //BIT[3]:CEC Reset
    //BIT[4]:Audio Low Power Enable
    //BIT[5]:Video reset enable
    //BIT[6]:Audio reset enable
    //BIT[7]:TMDS reset enable
    hdmi_tx_regs->hdmi_sw_reset |= BIT1;
    //0xd0d00140[4]:to enable i2s_ch0_en
    //0xd0d00140[2]:Audio Mute,1:mute, 0:normal
    //0xd0d00140[0]:Audio Enable,1:Enable, 0:Disable
    //to enable i2s_ch0_en
    hdmi_tx_regs->hdmi_audio_ctrl1 |= BIT4;
    //0xd0d00144[0]:I2S SCK edge select,1:Inverted, 0:Non-inverted
    //0xd0d00144[1]:I2S WS polarity,1:when WS=0 -> Left ch, 0:when WS=1 -> Left ch
    //0xd0d00144[2]:I2S data left justify,1:left justify, 0:right justif
    //0xd0d00144[3]:I2S data direction,1:LSb first, 0:MSb first
    //0xd0d00144[4]:i2s_philips_shift,1:delay one bit of sd to ws, 0:No shift
    //i2s_left_justify
    hdmi_tx_regs->hdmi_audio_ctrl2 |= BIT2;
    //(*((volatile INT32U *) 0xD0D00144)) &= ~(0x1<<2);
    return HDMI_OK;
}

/**
 * @brief   Register hdmi or cec callback function
 * @param   channel  - 0~1:hdmi0~1, 2:cec, others:clear
 * @param   user_isr - callback
 * @return 	none
 */
void drv_l1_hdmi_user_isr_register(INT8U channel, void (*user_isr)(INT32U))
{
    if(channel == 0)
        hdmi_user_isr0 = user_isr;
    else if(channel == 1)
        hdmi_user_isr1 = user_isr;
    else if(channel == 2)
        cec_user_isr = user_isr;
    else {
        hdmi_user_isr0 = NULL;
        hdmi_user_isr1 = NULL;
        cec_user_isr = NULL;
    }
}

/**
 * @brief   enable hdmi or cec interrupt
 * @param   src  - 0~1:hdmi0~1, 2:cec
 * @param   inten - bit of inten
 * @return 	none
 */
void drv_l1_hdmi_inten_set(HDMI_INT_CH src, INT32U inten)
{
    if(src == HDMI_INT_EN0)
        hdmi_tx_regs->hdmi_intr0_unmask = inten;
    else if(src == HDMI_INT_EN1)
        hdmi_tx_regs->hdmi_intr1_unmask = inten;
    else if(src == HDMI_CEC_INT_EN)
        hdmi_tx_regs->hdmi_cec_config3 = inten;
    else {
        DBG_PRINT("HDMI inten set fail\r\n");
    }
}

/**
 * @brief   Disable hdmi or cec interrupt
 * @param   src  - 0~1:hdmi0~1, 2:cec
 * @param   inten - bit of inten
 * @return 	none
 */
#if 0
void drv_l1_hdmi_inten_clr(HDMI_INT_CH src, INT32U inten)
{
    if(src == HDMI_INT_EN0)
        hdmi_tx_regs->hdmi_intr0_unmask = inten;
    else if(src == HDMI_INT_EN1)
        hdmi_tx_regs->hdmi_intr1_unmask = inten;
    else if(src == HDMI_CEC_INT_EN)
        hdmi_tx_regs->hdmi_cec_config3 = inten;
    else {
        DBG_PRINT("HDMI inten clear fail\r\n");
    }
}
#endif

void HDMI_IRQHandler(void)
{
    INT32U int_hdmi_sts0 = hdmi_tx_regs->hdmi_intr0_sts;
    INT32U int_hdmi_sts1 = hdmi_tx_regs->hdmi_intr1_sts;
    INT32U int_hdmi_en0 = hdmi_tx_regs->hdmi_intr0_unmask;
    INT32U int_hdmi_en1 = hdmi_tx_regs->hdmi_intr1_unmask;
    INT32U int_cec_sts;
	INT32U int_cec_en;
	INT32U state;

    state = int_hdmi_sts0;
	hdmi_tx_regs->hdmi_intr0_sts = ~state;
	state = int_hdmi_sts1;
    hdmi_tx_regs->hdmi_intr1_sts = ~state;

	if(int_hdmi_en0 & HDMI_INT_CECF_INT) {
        int_cec_sts = hdmi_tx_regs->hdmi_cec_sts2;
        int_cec_en = hdmi_tx_regs->hdmi_cec_config3;
        state = int_cec_sts;
        hdmi_tx_regs->hdmi_cec_sts2 = ~state;
        if ((int_cec_sts & int_cec_en)) {
            if (cec_user_isr) {
                (*cec_user_isr)(int_cec_sts & int_cec_en);
            }
        }
	}

    if (int_hdmi_sts0 & int_hdmi_en0) {
		if (hdmi_user_isr0) {
			(*hdmi_user_isr0)((int_hdmi_sts0 & int_hdmi_en0));
		}
	}

	if (int_hdmi_sts1 & int_hdmi_en1) {
		if (hdmi_user_isr1) {
			(*hdmi_user_isr1)((int_hdmi_sts1 & int_hdmi_en1));
		}
	}
}

void drv_l1_hdmi_set_ext_pll_clock(INT32U vid_mode)
{
    //peripheral switch to EXTPLL
    drv_l1_clock_set_peripheral_src_extpll(ENABLE);//this may decide at drv_l1_init, because it effect all peripheral
    switch(vid_mode)
    {
        //the divid 2 is due to using XT12M, it's half of design XT24M (shuttle issue)
        case HDMI_VID_480P:
            drv_l1_clock_set_ext_pll_freq(270000000, ENABLE);
            //drv_l1_clock_set_ext_pll_freq(324000000, ENABLE);//16M
            drv_l1_clock_set_peripheral_clock_freq(SRC_TMDS, 27000000);
            drv_l1_clock_set_peripheral_clock_enable(SRC_TMDS, ENABLE);
        break;
        case HDMI_VID_720P:
        case HDMI_VID_1080I:
            drv_l1_clock_set_ext_pll_freq(297000000, ENABLE);
            drv_l1_clock_set_peripheral_clock_freq(SRC_TMDS, 74250000);
            drv_l1_clock_set_peripheral_clock_enable(SRC_TMDS, ENABLE);
        break;
        case HDMI_VID_1080P:
            drv_l1_clock_set_ext_pll_freq(594000000, ENABLE);// divder can't set to 0(0+1), HW auto set to (1+1)
            drv_l1_clock_set_peripheral_clock_freq(SRC_TMDS, 148500000);
            drv_l1_clock_set_peripheral_clock_enable(SRC_TMDS, ENABLE);
        break;
        default:
            DBG_PRINT("HDMI clock set fail\r\n");
            return;
        break;
    }

}

void drv_l1_hdmi_set_audio_pll_clock(INT32U aud_mode)
{
    //APLL
    //0xD0000070 [11:0]    audio divider
    //0xD0000070 = 0x2FF    audio divider 73.7280 Mhz/(767+1)/3(fix) = 32000
    //0xD0000070 = 0x1FF    audio divider 67.7376 Mhz/(511+1)/3(fix) = 44100
    //0xD0000070 = 0x1FF    audio divider 73.7280 Mhz/(511+1)/3(fix) = 48000
    switch(aud_mode)
    {
        case HDMI_AUD_32K:
            drv_l1_clock_set_audio_pll_freq(1, ENABLE);
            drv_l1_clock_set_peripheral_clock_freq(SRC_HDMI_AUDIO, 32000);
        break;
        case HDMI_AUD_44K:
            drv_l1_clock_set_audio_pll_freq(0, ENABLE);
            drv_l1_clock_set_peripheral_clock_freq(SRC_HDMI_AUDIO, 44100);
        break;
        case HDMI_AUD_48K:
            drv_l1_clock_set_audio_pll_freq(1, ENABLE);
            drv_l1_clock_set_peripheral_clock_freq(SRC_HDMI_AUDIO, 48000);
        break;
        default:
        break;
    }
}

/**
 * @brief   hdmi clock set
 * @param   enable[in]: 0:disable/1:enable
 * @return 	none
 */
INT16S drv_l1_hdmi_clock_set(INT16U enable)
{
	INT16S ret;

	ret = drv_l1_clock_set_system_clk_en(HDMI_CLOCK_SOURCE, enable);
	if (ret < 0)
		return ret;

    return drv_l1_clock_set_system_clk_en(HDMI_AUDIO_CLOCK_SOURCE, enable);
}

/**
 * @brief   Initiate video and audio block in HDMI hardware macro.
 * @param   vid_mode: resolution of display
 * @param   aud_mode: resolution of audio
 * @return   HDMI_OK: Function is successful. HDMI_FAIL: Function is fail.
 */
INT32S drv_l1_hdmi_init(INT32U vid_mode, INT32U aud_mode)
{
    HDMI_PACKET	Pkt;
    INT32U phy1=0, phy2=0;
    INT32S ret = HDMI_FAIL;
    INT32U hdmi50hz_flag;

    drv_l1_hdmi_clock_set(1);

    if(vid_mode & HDMI_50HZ_EN)
    {
        vid_mode &= ~HDMI_50HZ_EN;
        hdmi50hz_flag = 1;
    }
    else
        hdmi50hz_flag = 0;

    vic_irq_register(VIC_HDMI, HDMI_IRQHandler);
    vic_irq_enable(VIC_HDMI);
#if 0 //test CEC interrupt
    drv_l1_hdmi_inten_set(HDMI_INT_EN0 ,HDMI_INT_CECF_INT);
    drv_l1_hdmi_inten_set(HDMI_CEC_INT_EN ,CEC_INT_EOM_RCV);
#endif

#if 0 //test HDMI vsync interrupt
    drv_l1_hdmi_inten_set(HDMI_INT_EN0 ,HDMI_INT_VPP_VSYNC);
#endif

/**
 * GPA7 HDMI In/Output Sync Setting Note:
 * Design:TFT2->HDMI->Display Device.
 * 1.TFT2 controller->Output Vsync/Hsync = low active.
 * 2.GPA7 HDMI design: HDCP->input Vsync/Hsync = high active
 * 3.480p HDMI SPEC->Output Vsync/Hsync = low activ
 * 4.720p/1080p HDMI SPEC->Output Vsync/Hsync = high active
*/
    drv_l1_hdmi_set_input_sync_timing(0,0,0);
    drv_l1_hdmi_set_output_sync_timing(0,0);
    if(hdmi_hwYCbCr_mode)
    {
        hdmi_tx_regs->hdmi_video_ctrl1 &= ~(0x1<<6);
        hdmi_tx_regs->hdmi_video_ctrl1 |= ((0x1<<12)|(0x1<<8));
    }

    switch (vid_mode)
    {
        case HDMI_VID_480P:
            drv_l1_hdmi_set_output_sync_timing(1,1);
            if(hdmi50hz_flag)
                hdmi_avi_info_pkt(&Pkt, 0x11, HDMI_PICTURE_ASPECT_4_3);      // 576P@50Hz
            else
            hdmi_avi_info_pkt(&Pkt, 0x02, HDMI_PICTURE_ASPECT_4_3);      // 480P

            phy1 = 0x121D;
            phy2 = 0x6000;

            ret = HDMI_OK;
            break;

        case HDMI_VID_720P:
            if(hdmi50hz_flag)
                hdmi_avi_info_pkt(&Pkt, 0x13, HDMI_PICTURE_ASPECT_16_9);      // 720P
            else
            hdmi_avi_info_pkt(&Pkt, 0x04, HDMI_PICTURE_ASPECT_16_9);      // 720P

            phy1 = 0x131F;
            phy2 = 0x6180;

            ret = HDMI_OK;
            break;
        case HDMI_VID_1080P:
            if(hdmi50hz_flag)
                hdmi_avi_info_pkt(&Pkt, 0x1F, HDMI_PICTURE_ASPECT_16_9);      // 1080P
            else
            hdmi_avi_info_pkt(&Pkt, 0x10, HDMI_PICTURE_ASPECT_16_9);      // 1080P

            phy1 = 0x131F;
            phy2 = 0x6180;

            ret = HDMI_OK;
	     break;
        case HDMI_VID_1080I:
            if(hdmi50hz_flag)
                hdmi_avi_info_pkt(&Pkt, 0x14, HDMI_PICTURE_ASPECT_16_9);      // 1080I
            else
            hdmi_avi_info_pkt(&Pkt, 0x5, HDMI_PICTURE_ASPECT_16_9);      // 1080I

            phy1 = 0x131F;
            phy2 = 0x6180;

            ret = HDMI_OK;
            break;
        default:
            ret = HDMI_FAIL;
        break;
    }

    drv_l1_hdmi_config_phy(phy1, phy2);

    if(aud_mode != HDMI_AUD_OFF) {

    }

	switch(aud_mode)
    {
        case HDMI_AUD_32K:
            // using 73.728M Hz
            switch (vid_mode)
            {
                case HDMI_VID_480P:
                    drv_l1_hdmi_set_acr_packet_fs(HDMI_AUD_32K, 4096, 27000);
                break;
                case HDMI_VID_720P:
                    drv_l1_hdmi_set_acr_packet_fs(HDMI_AUD_32K, 4096, 74250);
                break;
                case HDMI_VID_1080I:
                    drv_l1_hdmi_set_acr_packet_fs(HDMI_AUD_32K, 6144, 27000);
                break;
                case HDMI_VID_1080P:
                    drv_l1_hdmi_set_acr_packet_fs(HDMI_AUD_32K, 4096, 148500);
                break;
                default:break;
            }

            hdmi_audio_channel_status(Sampling_32K, Original_sampling_32K);// "Channel Status Bit"  information
            break;
        case HDMI_AUD_44K:
            // using 67.7376M Hz
            switch (vid_mode)
            {
                case HDMI_VID_480P:
                    drv_l1_hdmi_set_acr_packet(6272, 30000);
                break;
                case HDMI_VID_720P:
                case HDMI_VID_1080I:
                    drv_l1_hdmi_set_acr_packet(6272, 82500);
                break;
                case HDMI_VID_1080P:
                    drv_l1_hdmi_set_acr_packet(6272, 165000);
                break;
                default:break;
            }
            hdmi_audio_channel_status(Sampling_44K, Original_sampling_44K);// "Channel Status Bit"  information
            break;
        case HDMI_AUD_48K:
            // using 73.728M Hz
            switch (vid_mode)
            {
                case HDMI_VID_480P:
                    drv_l1_hdmi_set_acr_packet(6144, 27000);	// auto CTS
                break;
                case HDMI_VID_720P:
                case HDMI_VID_1080I:
                    drv_l1_hdmi_set_acr_packet(6144, 74250);	// auto CTS
                break;
                case HDMI_VID_1080P:
                    drv_l1_hdmi_set_acr_packet(6144, 148500);	// auto CTS
                break;
                default:break;
            }
            hdmi_audio_channel_status(Sampling_48K, Original_sampling_48K);// "Channel Status Bit"  information
        break;
        default:break;
    }

    //switch IOF to HDMI
    hdmi_tx_regs->hdmi_io_ctrl |= BIT0;

    if(aud_mode == HDMI_AUD_OFF)
        drv_l1_hdmi_set_mode(0);
    else
    {
        hdmi_audio_info_frame_pkt(&Pkt);
        drv_l1_hdmi_audio_ctrl(0);
        hdmi_tx_regs->hdmi_infoframe_ctrl1 = 0x1313;//for AVI packet, but need set after ACR
        drv_l1_hdmi_set_mode(1);
    }

    return ret;
}

/**
 * @brief   Disconnect HDMI device and turn off HDMI clock.
 * @param   none
 * @return   HDMI_OK: Function is successful. HDMI_FAIL: Function is fail.
 */
INT32S drv_l1_hdmi_exit(void)
{
	/* disable HDMI */
	vic_irq_disable(VIC_PPU);
	vic_irq_unregister(VIC_PPU);
	vic_irq_disable(VIC_HDMI);
	vic_irq_unregister(VIC_HDMI);
		
	drv_l1_hdmi_clock_set(0);
	drv_l1_hmdi_disable_output_pin();
	drv_l1_hdmi_set_mode(0);
	drv_l1_clock_set_audio_pll_freq(1, 0);	//disable
	drv_l1_clock_set_peripheral_clock_enable(SRC_TMDS, 0);	  //disable
	//drv_l1_clock_set_ext_pll_freq(270000000, 0);		//disable	
	//drv_l1_clock_set_peripheral_src_extpll(0);	//disable
		
    // disable clock source
    // disable audio
    // disable PHY power
    // disable HDMI clock enable
    return HDMI_OK;
}


/**
 * @brief      If video frame size of H264 are smaller than you want to play(480P/720P/1080P). Enable this function, It will scale up H264 video frame to appropriate size.
 * @param   width of H264 frame
 * @param   height of H264 frame
 * @param   image offset (Usually set 0)
 * @return   HDMI_OK: Function is successful. HDMI_FAIL: Function is fail.
 */
INT32S drv_l1_hdmi_scaling_enable(INT32U img_width, INT32U img_height, INT32U img_offset)
{
    g_hdmi_img_width = img_width;
    g_hdmi_img_height = img_height;
    g_hdmi_img_offset = img_offset;

    return HDMI_OK;
}


/**
 * @brief      Disable scaling H264 frame.
 * @param   none
 * @return   HDMI_OK: Function is successful. HDMI_FAIL: Function is fail.
 */
INT32S drv_l1_hdmi_scaling_disable(void)
{
    g_hdmi_img_width = 0;
    g_hdmi_img_height = 0;
    g_hdmi_img_offset = 0;

    return HDMI_OK;
}

//HDCP FUNCTION
#if HDCP_SUPPORT == 1
void drv_l1_hdmi_hdcp_reset(void)
{
    hdmi_tx_regs->hdmi_sw_reset |= 0x1<<2;
}

void drv_l1_hdmi_hdcp_set_repeater_mode(INT8U en)
{
    if(en)
        hdmi_tx_regs->hdmi_hdcp_ctrl1 |= 0x1<<2;
    else
        hdmi_tx_regs->hdmi_hdcp_ctrl1 &= ~(0x1<<2);
}

void drv_l1_hdmi_hdcp_set_An_generate(INT8U en)
{
    if(en)
        hdmi_tx_regs->hdmi_hdcp_ctrl1 |= 0x1<<3;
    else
        hdmi_tx_regs->hdmi_hdcp_ctrl1 &= ~(0x1<<3);
}

void drv_l1_hdmi_hdcp_read_An(INT16U *p)
{
    *p = hdmi_tx_regs->hdmi_mi12;
    *(p+1) = hdmi_tx_regs->hdmi_mi34;
    *(p+2) = hdmi_tx_regs->hdmi_mi56;
    *(p+3) = hdmi_tx_regs->hdmi_mi78;
}

void drv_l1_hdmi_hdcp_read_AKsv(INT8U *p)
{
    *p = ((hdmi_tx_regs->hdmi_aksv12)&0xFF);
    *(p+1) = ((hdmi_tx_regs->hdmi_aksv12 >> 8)&0xFF);
    *(p+2) = ((hdmi_tx_regs->hdmi_aksv34)&0xFF);
    *(p+3) = ((hdmi_tx_regs->hdmi_aksv34 >> 8)&0xFF);
    *(p+4) = ((hdmi_tx_regs->hdmi_aksv5)&0xFF);
}

void drv_l1_hdmi_hdcp_write_Bksv(INT16U *p)
{
    hdmi_tx_regs->hdmi_bksv12 = *p;
    hdmi_tx_regs->hdmi_bksv34 = *(p+1);
    hdmi_tx_regs->hdmi_bksv5 = (*(p+2))&0xFF;
}

INT32U drv_l1_hdmi_hdcp_read_Ri(void)
{
    return hdmi_tx_regs->hdmi_ri_cmp & 0x0000FFFF;
}

INT8U drv_l1_hdmi_hdcp_Ri_ready(void)
{
    if( (hdmi_tx_regs->hdmi_hdcp_ctrl1 & 0x0100) == 0x0100 )
        return 1;
    else
        return 0;
}

void drv_l1_hdmi_hdcp_enable(INT8U en, INT8U feature_en_1p1)
{
    hdmi_tx_regs->hdmi_hdcp_ctrl1 &= ~(0x3<<0);

    if(feature_en_1p1)
        hdmi_tx_regs->hdmi_hdcp_ctrl1 |= 0x1<<1;

    if(en)
        hdmi_tx_regs->hdmi_hdcp_ctrl1 |= 0x1<<0;
}
#endif

//CEC FUNCTION
#if CEC_SUPPORT == 1
void drv_l1_cec_tx_enable(INT8U en)
{
    if(en)
        hdmi_tx_regs->hdmi_cec_config2 |= BIT0;
    else
        hdmi_tx_regs->hdmi_cec_config2 &= ~BIT0;
}

void drv_l1_cec_interrupt_enable_set(INT32U int_bit)
{
    hdmi_tx_regs->hdmi_cec_config3 = int_bit;
}

void drv_l1_cec_tx_send(void)
{
    hdmi_tx_regs->hdmi_cec_cmd |= BIT1;
}

void drv_l1_cec_tx_para_set(INT32U para)
{
    //maybe need document modify naming
    //7.set 0xd0d001D4[7:0]=8'h2300
    hdmi_tx_regs->hdmi_cec_cmd = para;
}

void drv_l1_cec_tx_data(INT8U *data)
{
    //byte2:op code, byte1: addr[initiaior][rcv]
    hdmi_tx_regs->hdmi_cec_send_byte12 = (data[1]<<8)|data[0];
    hdmi_tx_regs->hdmi_cec_send_byte34 = (data[3]<<8)|data[2];
    hdmi_tx_regs->hdmi_cec_send_byte56 = (data[5]<<8)|data[4];
    hdmi_tx_regs->hdmi_cec_send_byte78 = (data[7]<<8)|data[6];
    hdmi_tx_regs->hdmi_cec_send_byte910 = (data[9]<<8)|data[8];
    hdmi_tx_regs->hdmi_cec_send_byte1112 = (data[11]<<8)|data[10];
    hdmi_tx_regs->hdmi_cec_send_byte1314 = (data[13]<<8)|data[12];
    hdmi_tx_regs->hdmi_cec_send_byte1516 = (data[15]<<8)|data[14];
}

void drv_l1_cec_timing_set(void)
{
    INT32U system_freq = drv_l1_clock_get_system_clock_freq();
    //CEC timing, 27M set 0x548, about 50usec
    //[15:0]
    //from system clock only
    hdmi_tx_regs->hdmi_cec_config1 = (system_freq/20000);
}

void drv_l1_cec_rx_data(INT8U *data)
{
    data[0] = hdmi_tx_regs->hdmi_cec_rcv_byte12 & 0xFF;
    data[1] = (hdmi_tx_regs->hdmi_cec_rcv_byte12>>8) & 0xFF;
    data[2] = hdmi_tx_regs->hdmi_cec_rcv_byte34 & 0xFF;
    data[3] = (hdmi_tx_regs->hdmi_cec_rcv_byte34>>8) & 0xFF;
    data[4] = hdmi_tx_regs->hdmi_cec_rcv_byte56 & 0xFF;
    data[5] = (hdmi_tx_regs->hdmi_cec_rcv_byte56>>8) & 0xFF;
    data[6] = hdmi_tx_regs->hdmi_cec_rcv_byte78 & 0xFF;
    data[7] = (hdmi_tx_regs->hdmi_cec_rcv_byte78>>8) & 0xFF;
    data[8] = hdmi_tx_regs->hdmi_cec_rcv_byte910 & 0xFF;
    data[9] = (hdmi_tx_regs->hdmi_cec_rcv_byte910>>8) & 0xFF;
    data[10] = hdmi_tx_regs->hdmi_cec_rcv_byte1112 & 0xFF;
    data[11] = (hdmi_tx_regs->hdmi_cec_rcv_byte1112>>8) & 0xFF;
    data[12] = hdmi_tx_regs->hdmi_cec_rcv_byte1314 & 0xFF;
    data[13] = (hdmi_tx_regs->hdmi_cec_rcv_byte1314>>8) & 0xFF;
    data[14] = hdmi_tx_regs->hdmi_cec_rcv_byte1516 & 0xFF;
    data[15] = (hdmi_tx_regs->hdmi_cec_rcv_byte1516>>8) & 0xFF;
}

void drv_l1_cec_rx_set(INT32U para)
{
    //maybe need document modify naming
    //3.set 0xd0d001E4=16'h0700    //cec follower: [10]cec_in_en[9]cec_header_ack[8]cec_data_ack
    hdmi_tx_regs->hdmi_cec_data_rcv = para;
}

void drv_l1_cec_rx_enable(INT32U para)
{
    //4.set 0xd0d001E8=16'h0300    //[9]sram_en[8]cec_rcv_frm_en
    hdmi_tx_regs->hdmi_cec_sts = para;
}

void drv_l1_cec_rx_addr_thd(INT32U addr_thd)
{
    //5.set 0xd0d001E0=0324 //[11:8]cec_addr [3:0]debounce_thd
    hdmi_tx_regs->hdmi_cec_config4 = addr_thd;
}

#endif


void drv_l1_hmdi_disable_output_pin(void)
{
	hdmi_tx_regs->hdmi_io_ctrl&=~ BIT0;	 //disable switch IOF to HDMI

}


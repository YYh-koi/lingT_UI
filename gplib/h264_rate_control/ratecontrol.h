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

/*********************************************************************************
 * Description :
 *
 * Authors :
 *        Ken Yang <kenyang@generalplus.com>
 *********************************************************************************/

#ifndef _RATE_CONRTROL_
#define _RATE_CONRTROL_

#ifndef int64_t
typedef long long int64_t;
#endif

#define X264_RATECONTROL

#ifdef X264_RATECONTROL

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/

#define H264_RC_CQP     0
#define H264_RC_CRF     1
#define H264_RC_ABR     2
#define H264_RC_DBR     3

#define SLICE_TYPE_P    0
#define SLICE_TYPE_HP   1
#define SLICE_TYPE_I    2
#define SLICE_TYPE_SKIP	3

/**************************************************************************
 *                             M A C R O S                                *
 **************************************************************************/

#define X264_MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define X264_MAX(a,b) ( (a)>(b) ? (a) : (b) )

/**************************************************************************
 *                          D A T A   T Y P E S                           *
 **************************************************************************/

typedef struct
{
	float coeff_min;
	float coeff;
	float count;
	float decay;
	float offset;
} predictor_t;

/* internal parameters */
typedef struct
{
	int b_vbv;
	float fps;
	float bitrate;
	float init_bitrate;
	float rate_tolerance;       /* must > 0.01, assigned by users */
	/* set qcompress can ajust qscale range. Larger qcompress causes smaller qscale and qscale range (better quality)
	 * I think the reasonable qcompress range should be 0.5-0.7
	 * In CRF mode, you can reduce f_rf_constant to shift down the qscale range */
	float qcompress;            /* 0.0 => cbr, 1.0 => constant qp. Assigned by users */
	int nmb;                    /* number of MBs, must be consistent with the encoder's nmb */
	int qp_constant[3];         /* calculated from i_qp_constant, ip_offset, and php_offset */

	/* stats */
	int frame_no;
	int64_t frame_size[3];    /* must update by users outside the ratecontrol loop */
	int qp;                     /* qp for the current frame */
	int qp_prev;                /* qp for the previous frame */
	int qp_prev_for[3];         /* qp for the previous I/P/hp frame */
	float qpm;                  /* qp for current ROI: precise float for AQ */
	float qpa_rc;               /* average of ROIs' qp before aq */
	float qpa_rc_prev;
	int   qpa_aq;               /* average of ROIs' qp after aq */
	int   qpa_aq_prev;
	float qp_novbv;             /* QP for the current frame if 1-pass VBV was disabled. */
	int frame_size_prev;
	int frame_size_prev_for[3];
	int pict_type_prev;
	float last_non_skip_mb_ratio;	/* (Intra_mbs + P_mbs) / (Intra_mbs + P_mbs + SKIP_mbs) */

	/* VBV stuff */
	float buffer_size;
	int buffer_fill_final;
	int buffer_fill_final_min;
	float buffer_fill;         /* planned buffer, if all in-progress frames hit their bit budget */
	float buffer_rate;         /* # of bits added to buffer_fill after each frame */
	float vbv_max_rate;        /* # of bits added to buffer_fill per second */
	predictor_t pred[4];          /* predict frame size from satd. P, HP, I, and HP_from_P */
	int single_frame_vbv;
	float rate_factor_max_increment; /* Don't allow RF above (CRF + this value). */

	/* ABR stuff */
	float last_satd;            /* frame satd after MC of the down-scaled frame in the pre-processing */
	float last_rceq;            /* estimated qscale using (blurred_complexity)^(1-qcompress) */
	float cplxr_sum;            /* sum of bits*qscale/rceq */
	float wanted_bits_window;   /* target bitrate*window */
	float cbr_decay;            /* 1.0 */
	float short_term_cplxsum;
	float short_term_cplxcount;
	float long_term_cplxsum;
	float long_term_cplxcount;
	int cplx_change_count;
	float rate_factor_constant; /* base_cplx^(1-qcompress)/qp2qscale(f_rf_constant) */
	float ip_offset;            /* i_qp = p_qp - ip_offset = 6*lg(f_ip_factor)*/
	float php_offset;           /* hp_qp = p_qp + php_offset = 6*lg(f_php_factor) */

	float last_qscale;
	float qscale_before_tuning;
	float last_qscale_for[3];   /* last qscale for a specific pict type, used for max_diff & ipb factor stuff */
	int last_non_hp_pict_type;
	float accum_p_qp;           /* for determining I-frame quant */
	float accum_p_norm;
	float lmin[3];              /* min qscale by frame type */
	float lmax[3];
	float lstep;                /* max change (multiply) in qscale per frame = 2^(i_qp_step/6) */
	float accum_q_x_nskip_for[3];
	float accum_q_x_nskip_norm_for[3];

	/* ROI RC stuff */
	float frame_size_estimated; /* Access to this variable must be atomic: double is
								 * not atomic on all arches we care about */
	int frame_size_maximum;       /* roughly limit max frame size, assigned by users */
	float frame_size_planned;
	predictor_t *roi_pred;
	predictor_t roi_preds[3];
	predictor_t *pred_hp_from_p; /* predict B-frame size from P-frame satd */
	int cur_hpframe;
	int hpframe_bits;            /* total cost of those frames */

	/* Complexity (satd) estimation */
	float complexity_coeff;
	float accum_complexity_for[3];
	float accum_complexity_norm[3];
	float satd_prev_for[3];     /* satd for P, HP, I */

	//float frame_size_tolerance; /* roughly limit frame size variation, assigned by users */
} gp_rc_t;

/*
 *  x264 use qscale to represent the quality of a frame. qscale is directly proportional to SATD/frame_bits
 *  When the X264_RC_CQP mode is enable, qp is fixed and assigned by qp_constant.
 *  When the X264_RC_CRF mode is enable, rate_factor is fixed (rate_factor = virtual_frame_size / target_frame_size). The result is similar to constant quality if the frame complexity can be correctly predicted.
 *  When the X264_RC_ABR mode is enable, the bitrate variation ratio is confined within rate_tolerance.
 *  VBV working principle: Avoid decoder buffer underflow (i.e., encoder buffer overflow), but ignore decoder buffer overflow.
 */
typedef struct
{
	/* user parameters */
	int b_hpframe;              /* if hierarchical p is used or not */
	int i_hpframes;                /* # consecutive HP-frames before this P-frame */
	int i_pict_type;            /* SLICE_TYPE_I, SLICE_TYPE_P, or SLICE_TYPE_HP. Must update by users outside the ratecontrol loop */

	int i_rc_method;            /* ratecontrol method. Can't dynamically change */

	int i_qp_constant;          /* 0 to (51+6*(x264_bit_depth-8)), q_qp_constant = i_rf_constant + QP_BD_OFFSET. Since there is only 8-bit depth, QP_BD_OFFSET=0 */
	int i_qp_min;               /* min allowed QP */
	int i_qp_max;               /* max allowed QP */
	int i_qp_step;              /* max QP step between frames */

	int i_bitrate;
	float f_rf_constant;        /* 1pass VBR, nominal QP, smaller value produces better quality. ~25: Good; 25~30: Fair; 30~: Bad */
	float f_rf_constant_max;  /* In CRF mode, maximum CRF as caused by VBV */
	float f_rate_tolerance;
	int   i_vbv_max_bitrate;
	int   i_vbv_buffer_size;
	float f_vbv_buffer_init; /* <=1: fraction of buffer_size. >1: bits */
	float f_ip_factor;          /* qscale_I = qscale_P / f_ip_factor */
	float f_php_factor;         /* qscale_HP = qscale_P * f_ip_factor */
	/* set qcompress can ajust qscale range. Larger qcompress causes smaller qscale and qscale range (better quality)
	 * I think the reasonable qcompress range should be 0.5-0.7
	 * In CRF mode, you can reduce f_rf_constant to shift down the qscale range */
	float f_qcompress;    /* 0.0 => cbr, 1.0 => constant qp */

	int i_frame_size_maximum; /* roughly limit max frame size, assigned by users */
	int i_qp_min_night_offset;  /* qp_min_night = qp_min - qp_min_night_offset */
	int i_rc_reset_period;

	int i_mb_count;
	int i_fps;
	int i_gop_size;

	int i_isnight[5];           /* day or night */
	int i_rawY_delay;

	/* output */
	int qp;

	gp_rc_t *rc;

} gp_ratecontrol_param;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
void gp_H264_ratecontrol_new(gp_ratecontrol_param *rc);
void gp_H264_ratecontrol_start(gp_ratecontrol_param *rc, int isnight, int dynamic_bitrate_lv); /* dynamic_bitrate_lv >= 10k means new_bitrate = dynamic_bitrate_lv; 0 <= dynamic_bitrate_lv <= 10 means new_bitrate = old_bitrate * (1 + (dynamic_bitrate_lv-5)*10%); */
void gp_H264_ratecontrol_end(gp_ratecontrol_param *rc, int bits, int cost, float non_skip_mb_ratio);
int gp_H264_ratecontrol_reconfig_qp(gp_ratecontrol_param *rc_param);
int gp_H264_ratecontrol_reconfig_VBV(gp_ratecontrol_param *rc_param);
int gp_H264_ratecontrol_reconfig_bitrate_fps(gp_ratecontrol_param *rc_param);
int gp_H264_ratecontrol_reconfig_rf_hp(gp_ratecontrol_param *rc_param);

#endif
#endif

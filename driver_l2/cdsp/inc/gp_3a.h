#ifndef __GP_3A_H__
#define __GP_3A_H__


#ifndef sensor_exposure_t
typedef struct {
	// ev index
	int sensor_ev_idx;
	int ae_ev_step;

	// real setting for sensor
	int time, analog_gain, digital_gain;

	int daylight_ev_idx;	// daylight time
	int night_ev_idx;		// night time
	int max_ev_idx;
	int total_ev_idx;

    int raw_mode_idx;
} sensor_exposure_t;
#endif


#ifndef ISO_AUTO
#define ISO_AUTO						0x1881
#endif

#ifndef int64_t
typedef long long int64_t;
#endif



// ***********************************************************************************************
// Constant value definition
// ***********************************************************************************************
#define AEGAIN_P05EV				1060    // + 0.05 EV
#define AEGAIN_P01EV				1031    // +0.01 EV
#define AEGAIN_0EV					1024    // 0 EV
#define AEGAIN_N01EV				1017    // -0.01 EV
#define AEGAIN_N05EV				989     // -0.05 EV


#define AE_EV0_LUM					100    // default target luminance
#define AE_EV0_LUM_TOR				0x06    // default tolerance for target luminance


#ifndef ENABLE
#define ENABLE						1
#endif

#ifndef DISABLE
#define DISABLE						0
#endif




// ***********************************************************************************************
// Type definition
// ***********************************************************************************************
typedef enum {
	AE_STABLE = -1,  // AE satble
	AE_CHANGE = 0 	// AE should be modified and to set sensor exposure time
} AE_RESULT;



typedef enum {
	AWB_FAIL = -1,		// AWB fail
	AWB_RET = 0,		// AWB processing but has no result for setting wb gain
	AWB_SUCCESS_DC,	// AWB success in DC mode
	AWB_USERDEF		// AWB success in user define mode
} AWB_RESULT;


typedef enum {
	AWB_ALG_NORMAL = 0x0,
	AWB_ALG_GREY_WORLD = 0x1
} AWB_ALG;


typedef enum {
	AWB_AUTO_DC = 0x0,	// AUTO WB for DC
    AWB_AUTO_DC2,

	AWB_LAMP, 					// Color temperature = 3200k
	AWB_FLUORESCENT, 			// Color temperature = 4000k
	AWB_DAYLIGHT,				// Color temperature = 5200k
	AWB_FLASH, 				// Color temperature = 6000k
	AWB_CLOULDY, 				// Color temperature =  6000k
	AWB_SHADE, 				// Color temperature =  7000k
} AWB_MODE;

typedef enum {
	AF_STEPINC = 0x0,
    AF_FULLSEARCH,
    AF_USER,
} AF_SEARCH_MODE;


typedef enum {
	CDSP_AE_METER_SPOT = 0,						// AE METER: Spot
	CDSP_AE_METER_CENTER_WEIGHTED_AVG,		// AE METER: Center weighted average
	CDSP_AE_METER_CENTER_WEIGHTED_AVG_CVR,	// AE METER: Center weighted average for CVR
	CDSP_AE_METER_CENTER_WEIGHTED_AVG_CVR2,	// AE METER: Center weighted average 2 for CVR
	CDSP_AE_METER_AVG,							// AE METER: Averate
	CDSP_AE_METER_AVG_T2,							// AE METER: Averate Type 2
	CDSP_AE_METER_USER_WEIGHT,					// AE METER: 64 window weight is defined by user
	CDSP_AE_METER_PCAM,                         // AE METER: PCAM mode
} CDSP_AE_METER_MODE;


typedef struct {

	unsigned int sumcnt[4];	// count for each region (different luminance)
	unsigned int sumr[4];	// sum of r vale in different region
	unsigned int sumg[4];	// sum of green vale in different region
	unsigned int sumb[4];	// sum of b vale in different region
	unsigned int sumrg[4];	// sum of r/g vale in different region
	unsigned int sumbg[4];	// sum of b/g vale in different region

} AWB_RESULT_t;


typedef struct IspAfResult_s{
    // af
	unsigned long long	af1_h_value;
	unsigned long long	af1_v_value;
	unsigned long long	af2_h_value;
	unsigned long long	af2_v_value;
	unsigned long long	af3_h_value;
	unsigned long long	af3_v_value;
} IspAfResult_t;

typedef struct IspAwbResult_s
{
	unsigned int	r, g, b;

	unsigned int	rg, bg, rglv, bglv;
	unsigned int	px, pxlv;

	unsigned int	rglv2, bglv2;
	unsigned int	pxlv2;

	unsigned int	alg_mode;
	unsigned short  y_high, y_low;
} IspAwbResult_t;


typedef struct IspAeResult_s
{
	unsigned int	ae_win[64];
	unsigned int	ae_bp_win[64];
	unsigned int	ae_wp_win[64];
	unsigned int	ae_bpwp_win[64];
} IspAeResult_t;


// ***********************************************************************************************
//                                                       Function definition
// ***********************************************************************************************


//=========================================================================
// Function Name :  gp_isp_3a_get_version
// Purpose :  Get library version
// Parameters : void
// Return : version string
//=========================================================================
const char *gp_isp_3a_get_version(void);




//=========================================================================
// Function Name :  gp_isp_3a_init
// Purpose :  for AE & AWB initialization
// Parameters : unsigned char *_ae: working memory for AE
//                    unsigned char *_awb: working memory for AWB
//                    unsigned char *_af: working memory for AF
//                    AWB_MODE mode: AWB mode
// Return : void
//=========================================================================
void gp_isp_3a_init(unsigned char *_ae, unsigned char *_awb, unsigned char *_af, AWB_MODE mode);



//=========================================================================
// Function Name :  gp_isp_3A_reset
// Purpose :  for 3A reset some memory
// Parameters : unsigned char *_ae: working memory for AE
//                    unsigned char *_awb: working memory for AWB
//                    unsigned char *_af: working memory for AF
// Return : void
//=========================================================================
void gp_isp_3a_reset(unsigned char *_ae, unsigned char *_awb, unsigned char *_af);



//=========================================================================
// Function Name :  gp_isp_ae_get_workmem_size
// Purpose :  Get AE working memory size
// Parameters : void
// Return: size of working memory for AE
//=========================================================================
unsigned int gp_isp_ae_get_workmem_size(void);


//=========================================================================
// Function Name :  gp_isp_awb_get_workmem_size
// Purpose :  Get AWB working memory size
// Parameters : void
// Return: size of working memory for AWB
//=========================================================================
unsigned int gp_isp_awb_get_workmem_size(void);





// ***********************************************************************************************
// ****************************************   AE   *************************************************
// ***********************************************************************************************


//=========================================================================
// Function Name: gp_isp_ae_set_fps
// Purpose: to make AE algorithm to know the frame rate
// Parameters: unsigned char *_ae: working memory for AE
//                  int fps: sensor frame rate, ex: 30 or 25 or 20 ...
// Return: none
//=========================================================================
void gp_isp_ae_set_fps(unsigned char *_ae, int fps);



//=========================================================================
// Function Name: gp_isp_ae_calc_exp
// Purpose: to calculate the sensor exposure time
// Parameters: unsigned char *_ae: working memory for AE
//                  unsigned char *ae_win: pointer to AE window (8x8 = 64 values)
//                  sensor_exposure_t *seinfo: information of sensor exposure time/gain
//                  int *hist: pointer to histogram
// Return: AE_STABLE: AE satble
//     	  AE_CHANGE: AE should be modified and to set sensor exposure time
//=========================================================================
int gp_isp_ae_calc_exp(unsigned char *_ae, IspAeResult_t *ae_win, sensor_exposure_t *seinfo, unsigned int *histogram);



//=========================================================================
// Function Name: gp_isp_ae_set_meter
// Purpose : set AE meter
// Parameters : CDSP_AE_METER_MODE meter: type of AE meter
//			   unsigned char *_ae: working memory for AE
// Return: void
//=========================================================================
void gp_isp_ae_set_meter(CDSP_AE_METER_MODE meter, unsigned char *_ae);



//=========================================================================
// Function Name: gp_isp_ae_get_meter
// Purpose : get AE meter
// Parameters :
//			   unsigned char *_ae: working memory for AE
// Return: current AE meter
//=========================================================================
int gp_isp_ae_get_meter(unsigned char *_ae);


//=========================================================================
// Function Name : gp_isp_ae_set_target_lum
// Purpose : gp_isp_ae_set_target_lum
// Parameters : unsigned char *_ae: working memory for AE
//                    int targetLum: target luminance
// Return: void
//=========================================================================
void gp_isp_ae_set_target_lum(unsigned char *_ae, int targetLum);




//=========================================================================
// Function Name : gp_isp_ae_set_target_lum_night
// Purpose : set AE target for night
// Parameters : unsigned char *_ae: working memory for AE
//                    int targetLum_night: target luminance for night
// Return: void
//=========================================================================
void gp_isp_ae_set_target_lum_night(unsigned char *_ae, int targetLum_night);



//=========================================================================
// Function Name: gp_isp_ae_set_wp_ratio_thr
// Purpose : set wp ratio threshold for ae T2 when IR is on
// Parameters: unsigned char *_ae: working memory for AE
//             unsigned char wp_ratio_thr: 0 (0.0) ~ 255 (1.0)
// Return: void
//=========================================================================
void gp_isp_ae_set_wp_ratio_thr(unsigned char *_ae, unsigned char wp_ratio_thr);


//=========================================================================
// Function Name : gp_isp_ae_set_ev
// Purpose : se EV (form -2 ~ 0 ~ +2)
// Parameters : unsigned char *_ae: working memory for AE
//              int ae_target
//              int ev_gain: format Q10
// Return: void
//=========================================================================
int gp_isp_ae_set_ev(unsigned char *_ae, int ae_target, int ev_gain);



//=========================================================================
// Function Name : gp_isp_ae_set_iso
// Purpose : set ISO
// Parameters: unsigned char *_ae: working memory for AE
//                   int iso: ISO value (1: 100, 2: 200, 4: 400, ISO_AUTO)
//                   sensor_exposure_t *seinfo: information of sensor exposure time/gain
// Return: void
//=========================================================================
void gp_isp_ae_set_iso(unsigned char *_ae, int iso, sensor_exposure_t *seinfo);


//=========================================================================
// Function Name: gp_isp_ae_set_lum_bound
// Purpose : set the luminance bound by different size of ae window
// Parameters : unsigned char *_ae: working memory for AE
//                    int win_w: width of ae window
//                    int win_h: height of ae window
// Return: void
//=========================================================================
void gp_isp_ae_set_lum_bound(unsigned char *_ae, int win_w, int win_h);


//=========================================================================
// Function Name: gp_isp_ae_set_lum_bound
// Purpose : set the histogram bound by different size of histogram window
// Parameters : unsigned char *_ae: working memory for AE
//                    int hist_win_w: width of histogram window
//                    int hist_win_h: height of histogram window
//                    int shiftbit: depend on histogram window size
// Return: void
//=========================================================================
void gp_isp_ae_set_hist_bound(unsigned char *_ae, int hist_win_w, int hist_win_h, int shiftbit);

//=========================================================================
// Function Name: gp_isp_ae_set_sensor_exp_time
// Purpose : AE algorithm must save the information of sensor exposure time/gain
// Parameters: unsigned char *_ae: working memory for AE
//                   sensor_exposure_t *seinfo: information of sensor exposure time/gain
// Return: void
//=========================================================================
void gp_isp_ae_set_sensor_exp_time(unsigned char *_ae, sensor_exposure_t *seinfo);




//=========================================================================
// Function Name: gp_isp_ae_is_night
// Purpose : get the night status
// Parameters: unsigned char *_ae: working memory for AE
// Return: 1: night,  0: not night
//=========================================================================
int gp_isp_ae_is_night(void *_ae);
int gp_isp_ae_is_night_mode(void *_ae);


//=========================================================================
// Function Name: gp_isp_ae_get_result_lum
// Purpose : get the current luminance
// Parameters: unsigned char *_ae: working memory for AE
// Return: current luminance
//=========================================================================
int gp_isp_ae_get_result_lum(unsigned char *_ae);




//=========================================================================
// Function Name: gp_isp_ae_get_result_ev
// Purpose : get the EV index, Ex: 0ev = 0,  +0.2ev = +2,  -0.3ev = -3, ...
// Parameters: unsigned char *_ae: working memory for AE
// Return: EV index, step = 0.1ev
//=========================================================================
int gp_isp_ae_get_result_ev(unsigned char *_ae);




//=========================================================================
// Function Name: gp_isp_ae_set_user_center_weight
// Purpose : set ae weight for each window (8x8 = 64)
// Parameters: unsigned int *weight: total 64 weighting values, Ex: 0.4 * 32768, 0.4 * 32768, 0.6 * 32768 ...
//                  unsigned char *_ae: working memory for AE
// Return: void
//=========================================================================
void gp_isp_ae_set_user_weight(unsigned int *weight, unsigned char *_ae);




//=========================================================================
// Function Name: gp_isp_ae_set_user_center_weight
// Purpose : set ae weight for CVR
// Parameters: unsigned int *weight: total 4 weighting values, total sum = 1.0
//             Ex: 0.1 * 1024, 0.2 * 1024, 0.3 * 1024, 0.4 * 1024
//             unsigned char *_ae: working memory for AE
// Return: void
//=========================================================================
void gp_isp_ae_set_cvr_weight(unsigned int *weight, unsigned char *_ae);


//=========================================================================
// Function Name: gp_isp_ae_set_t2_bd_thr
// Purpose : set AE meter T2 bright & dark thr
// Parameters:  unsigned char *_ae: working memory for AE
//              int bright_thr: 0 ~ 31
//              int dark_thr: 0 ~ 31
// Return: void
//=========================================================================
void gp_isp_ae_set_t2_bd_thr(unsigned char *_ae, int bright_thr, int dark_thr);


//=========================================================================
// Function Name: gp_cdse_ae_set_backlight_detect
// Purpose : Enable/Disable backlight detect
// Parameters:  unsigned char *_ae: working memory for AE
//                    int enable: enable = 1, disable = 0
// Return: void
//=========================================================================
void gp_cdse_ae_set_backlight_detect(unsigned char *_ae, int enable);



//=========================================================================
// Function Name: gp_isp_ae_comb_win
// Purpose : combine two ae window for dynamic set window
// Parameters:  unsigned char *_ae: working memory for AE
//              unsigned char *ae_win_org: destination of AE window
//              unsigned char *ae_win_src: source of AE window
//              int flag: 0: 1st,   2: 2nd
// Return: void
//=========================================================================
void gp_isp_ae_comb_win(unsigned char *_ae, unsigned short *ae_win_dst, unsigned char *ae_win_src, int flag);



// ***********************************************************************************************
// ****************************************   AWB   ************************************************
// ***********************************************************************************************

//=========================================================================
// Function Name : gp_isp_awb_get_thr
// Purpose : get awb thresholds to set ISP
// Parameters : short* thr_out: thresholds output
//              short* thr_in: thresholds input
//              unsigned short thr_cnt: count of thresholds
// Return: void
//=========================================================================
void gp_isp_awb_get_thr(short* thr_out, short* thr_in, unsigned short thr_cnt);


//=========================================================================
// Function Name : gp_isp_awb_set_excpt_thr
// Purpose : get awb exception thresholds
// Parameters : short* thr_out: thresholds output
//              short* thr_in1: 2800k exception thresholds
//              short* thr_in2: 4000k exception thresholds
//              short* thr_in3: 5000k exception thresholds
//              short* thr_in4: 6500k exception thresholds
//              unsigned short thr_cnt1: count of 2800k exception thresholds
//              unsigned short thr_cnt2: count of 4000k exception thresholds
//              unsigned short thr_cnt3: count of 5000k exception thresholds
//              unsigned short thr_cnt4: count of 6500k exception thresholds
// Return: void
//=========================================================================
void gp_isp_awb_set_excpt_thr(short* thr_out, short* thr_in1, short* thr_in2, short* thr_in3, short* thr_in4,
	unsigned short thr_cnt1, unsigned short thr_cnt2, unsigned short thr_cnt3, unsigned short thr_cnt4);




//=========================================================================
// Function Name : gp_isp_awb_set_mode
// Purpose : set WB mode
// Parameters : unsigned char *_awb: working memory for AWB
//                    AWB_MODE mode: white balance mode
// Return: void
//=========================================================================
void gp_isp_awb_set_mode(unsigned char *_awb, AWB_MODE mode);




//=========================================================================
// Function Name : gp_isp_awb_set_cnt_thr
// Purpose : set
// Parameters : unsigned char *_awb: working memory for AWB
//                    int win_w: width of awb window
//                    int win_h: height of awb window
//                    int cnt_thr: user set thr, otherwise cnt_thr = 0;
// Return: void
//=========================================================================
void gp_isp_awb_set_cnt_thr(unsigned char *_awb, int win_w, int win_h, int cnt_thr);



//=========================================================================
// Function Name : gp_isp_awb_get_mode
// Purpose : get wb mode
// Parameters : unsigned char *_awb: working memory for AWB
// Return: white balance mode
//=========================================================================
int gp_isp_awb_get_mode(unsigned char *_awb);


//=========================================================================
// Function Name : gp_isp_awb_get_color_temperature
// Purpose : get color temperature
// Parameters : unsigned char *_awb: working memory for AWB
// Return: color temperature
//=========================================================================
int gp_isp_awb_get_color_temperature(unsigned char *_awb);


//=========================================================================
// Function Name : gp_isp_awb_get_calc_color_temperature
// Purpose : get calcucated color temperature
// Parameters : unsigned char *_awb: working memory for AWB
// Return: color temperature
//=========================================================================
int gp_isp_awb_get_calc_color_temperature(unsigned char *_awb);


//=========================================================================
// Function Name : gp_isp_awb_get_r_gain
// Purpose : get Red gain
// Parameters : unsigned char *_awb: working memory for AWB
// Return: red gain for wb gain
//=========================================================================
int gp_isp_awb_get_r_gain(unsigned char *_awb);


//=========================================================================
// Function Name : gp_isp_awb_get_g_gain
// Purpose : get Green gain
// Parameters : unsigned char *_awb: working memory for AWB
// Return: green gain for wb gain
//=========================================================================
int gp_isp_awb_get_g_gain(unsigned char *_awb);

//=========================================================================
// Function Name : gp_isp_awb_get_b_gain
// Purpose :  get blue gain
// Parameters : unsigned char *_awb: working memory for AWB
// Return: blue gain for wb gain
//=========================================================================
int gp_isp_awb_get_b_gain(unsigned char *_awb);


//=========================================================================
// Function Name : gp_isp_awb_get_r_gain2
// Purpose : get Red gain
// Parameters : unsigned char *_awb: working memory for AWB
// Return: red gain for wb gain2
//=========================================================================
int gp_isp_awb_get_r_gain2(unsigned char *_awb);


//=========================================================================
// Function Name : gp_isp_awb_get_g_gain2
// Purpose : get Green gain
// Parameters : unsigned char *_awb: working memory for AWB
// Return: green gain for wb gain2
//=========================================================================
int gp_isp_awb_get_g_gain2(unsigned char *_awb);



//=========================================================================
// Function Name : gp_isp_awb_get_b_gain2
// Purpose :  get blue gain
// Parameters : unsigned char *_awb: working memory for AWB
// Return: blue gain for wb gain2
//=========================================================================
int gp_isp_awb_get_b_gain2(unsigned char *_awb);


//=========================================================================
// Function Name : gp_isp_awb_get_points
// Purpose :  get number of points in AWB thr
// Parameters : unsigned char *_awb: working memory for AWB
// Return: number of points in AWB thr
//=========================================================================
unsigned int gp_isp_awb_get_points(unsigned char *_awb);



//=========================================================================
// Function Name:  gp_isp_awb_calc_gain
// Purpose :  calculate the white balance gain
// Parameters : unsigned char *_awb: working memory for AWB
//                    AWB_RESULT_t *awb_result: result of wb window
// Return: AWB_FAIL: AWB fail
//	         AWB_RET: AWB processing but has no result for setting wb gain
//	         AWB_SUCCESS_CVR: AWB success in CVR mode
//	         AWB_SUCCESS_DC: AWB success in DC mode
//	         AWB_USERDEF: AWB success in user define mode
//=========================================================================
int gp_isp_awb_calc_gain(unsigned char *_awb, AWB_RESULT_t *awb_result);





//=========================================================================
// Function Name:  gp_isp_awb_pre_calc_gain
// Purpose :  calculate the white balance gain
// Parameters : unsigned char *_awb: working memory for AWB
//                    AWB_RESULT_t *awb_result: result of wb window
// Return: AWB_FAIL: AWB fail
//	         AWB_RET: AWB processing but has no result for setting wb gain
//	         AWB_SUCCESS_CVR: AWB success in CVR mode
//	         AWB_SUCCESS_DC: AWB success in DC mode
//	         AWB_USERDEF: AWB success in user define mode
//=========================================================================
int gp_isp_awb_pre_calc_gain(unsigned char *_awb, AWB_RESULT_t *awb_result);



//=========================================================================
// Function Name:  gp_isp_awb_reset_wb_gain
// Purpose :  reset the white balance gain
// Parameters : unsigned char *_awb: working memory for AWB
//                    int color_temperature: Ex: 5000k --> color_temperature = 50, 6500k --> color_temperature = 65, ...
// Return: void
//=========================================================================
void gp_isp_awb_reset_wb_gain(unsigned char *_awb, int color_temperature);


//=========================================================================
// Function Name:  gp_isp_awb_set_r_gain_factor
// Purpose :  adjust the r gain by factor
// Parameters : unsigned char *_awb: working memory for AWB
//                    int factor: the scale factor of r gain // x 256
// Return: void
//=========================================================================
void gp_isp_awb_set_r_gain_factor(unsigned char *_awb, int factor);


//=========================================================================
// Function Name:  gp_isp_awb_set_g_gain_factor
// Purpose :  adjust the g gain by factor
// Parameters : unsigned char *_awb: working memory for AWB
//                    int factor: the scale factor of g gain // x 256
// Return: void
//=========================================================================
void gp_isp_awb_set_g_gain_factor(unsigned char *_awb, int factor);


//=========================================================================
// Function Name:  gp_isp_awb_set_b_gain_factor
// Purpose :  adjust the r gain by factor
// Parameters : unsigned char *_awb: working memory for AWB
//              int  factor: the scale factor of b gain // x 256
// Return: void
//=========================================================================
void gp_isp_awb_set_b_gain_factor(unsigned char *_awb, int factor);



//=========================================================================
// Function Name:  gp_isp_awb_set_ct_offset
// Purpose :  set the white balance offset
// Parameters : unsigned char *_awb: working memory for AWB
//                    int offset: the offset of the color temperature
// Return: void
//=========================================================================
void gp_isp_awb_set_ct_offset(unsigned char *_awb, int offset);



//=========================================================================
// Function Name:  gp_isp_awb_set_calc_ct_offset
// Purpose :  set the white balance offset when calucated
// Parameters : unsigned char *_awb: working memory for AWB
//                    int offset: the offset of the color temperature
// Return: void
//=========================================================================
void gp_isp_awb_set_calc_ct_offset(unsigned char *_awb, int offset);


//=========================================================================
// Function Name:  gp_isp_awb_set_r_b_gain_boundary
// Purpose :  limit the r/b gain for AWB2
// Parameters : unsigned char *_awb: working memory for AWB
//                    int max_rgain: max r gain
//                    int max_bgain: max b gain
//                    int min_rgain: min r gain
//                    int min_bgain: min b gain
// Return: void
//=========================================================================
void gp_isp_awb_set_r_b_gain_boundary(unsigned char *_awb, int max_rgain, int max_bgain, int min_rgain, int min_bgain);


//=========================================================================
// Function Name:  gp_isp_awb_autoset_r_b_gain_boundary
// Purpose :  auto set r/b gain boundary by gain table for AWB2
// Parameters : unsigned char *_awb: working memory for AWB
// Return: void
//=========================================================================
void gp_isp_awb_autoset_r_b_gain_boundary(unsigned char *_awb);




//=========================================================================
// Function Name:  gp_isp_awb_autoset_r_b_gain_boundary2
// Purpose :  auto set r/b gain boundary by gain table for AWB2
// Parameters : unsigned char *_awb: working memory for AWB
//              short *awb_thr
//              int size: sizeof(awb_thr)
// Return: void
//=========================================================================
void gp_isp_awb_set_r_b_gain_boundary2(unsigned char *_awb, short *awb_thr, int size);


//=========================================================================
// Function Name:  gp_isp_awb_autoset_r_b_gain_boundary3
// Purpose :  auto set r/b gain boundary by gain table for AWB
// Parameters : unsigned char *_awb: working memory for AWB
//              short *awb_thr
// Return: void
//=========================================================================
void gp_isp_awb_set_r_b_gain_boundary3(unsigned char *_awb, short *awb_thr);


int gp_isp_get_gamma(unsigned char *_ae, unsigned int *gamma, int is_rgb_gamma);

void gp_isp_ae_set_wb_gain(unsigned char *_ae, unsigned short r_gain, unsigned short b_gain);

void gp_isp_ae_set_global_gain(unsigned char *_ae, unsigned short global_gain);

//=========================================================================
// Function Name:  gp_isp_awb_new_get_thr
// Purpose :  get new awb threshold
// Parameters : short *thr_in: threshold for specific color temperature
//              int thr_in_size: how many kinds of color temperatures
//              short *thr_out: threshold result
//              int lct_ctrl: low CT control: 0(no low CT) ~ 16(all low CT)
//              int is_7500k: user has 7500K calibration result
// Return: void
//=========================================================================
void gp_isp_awb_new_get_thr(unsigned char *_awb, short *thr_in, int thr_in_size, short *thr_out, int lct_ctrl, int is_7500k);



//=========================================================================
// Function Name:  gp_isp_awb_set_wb_gain_adj
// Purpose :  wb gain adjustment
// Parameters : unsigned char *_awb: working memory for AWB
//              unsigned short *wb_gain_adj: to adjust wb gain weighting (1.0x = 1024)
//
// Return: void
//=========================================================================
void gp_isp_awb_set_wb_gain_adj(unsigned char *_awb, unsigned short *wb_gain_adj);


//=========================================================================
// Function Name:  gp_isp_get_lsc_radius
// Purpose : get lsc parameters
// Parameters : unsigned char *_awb: working memory for AWB
//              unsigned short *radius0: lsc radius0
//              unsigned short *radius1: lsc radius1
//              unsigned short rgain: white balance r gain
//              unsigned short bgain: white balance b gain
//
// Return: void
//=========================================================================
void gp_isp_get_lsc_radius(unsigned char *_awb, unsigned short *radius0, unsigned short *radius1, unsigned short rgain, unsigned short bgain);


//=========================================================================
// Function Name:  gp_isp_set_lsc_radius
// Purpose : set lsc parameters for adjustment
// Parameters : unsigned char *_awb: working memory for AWB
//              unsigned short *lsc_ref_wb_gain: lsc reference wb gain
//              unsigned int *gain2800k: lsc gain 2800k
//              unsigned int *gain4000k: lsc gain 4000k
//              unsigned int *gain6500k: lsc gain 6500k
//              int ratio_r: decay ratio for R channel * 256
//              int ratio_g: decay ratio for G channel * 256
//              int ratio_b: decay ratio for B channel * 256
// Return: void
//=========================================================================
void gp_isp_set_lsc_gain(unsigned char *_awb, unsigned short *lsc_ref_wb, unsigned int *gain2800k, unsigned int *gain4000k,
unsigned int *gain6500k, int ratio_r, int ratio_g, int ratio_b);



//=========================================================================
// Function Name:  gp_isp_set_cm
// Purpose : set color matrix for adjustment
// Parameters : unsigned char *_awb: working memory for AWB
//              unsigned short *lsc_ref_wb_gain: lsc reference wb gain
//              unsigned short *cm1: cm 6500k
//              unsigned short *cm2: cm 4000k
//              unsigned short *cm3: cm 2750k
// Return: void
//=========================================================================
void gp_isp_set_cm(unsigned char *_awb, unsigned short *lsc_ref_wb_gain, short *cm1, short *cm2, short *cm3);



//=========================================================================
// Function Name:  gp_isp_get_cm
// Purpose : get color matrix parameters
// Parameters : unsigned char *_awb: working memory for AWB
//              unsigned short *cm: output CM
//              unsigned short r_gain: white balance r gain
//              unsigned short g_gain: white balance g gain
//              unsigned short b_gain: white balance b gain
//
// Return: void
//=========================================================================
void gp_isp_get_cm(unsigned char *_awb, short *cm, unsigned short r_gain, unsigned short g_gain, unsigned short b_gain);


//=========================================================================
// Function Name:  gp_isp_awb_get_ct_gain
// Purpose : get r/g/b gain of color temperature
// Parameters : unsigned char *_awb: working memory for AWB
//              unsigned short ct: color temperature, from 20 ~ 70 (2000k ~ 7000k)
//              unsigned short *rgain: white balance r gain
//              unsigned short *ggain: white balance g gain
//              unsigned short *bgain: white balance b gain
//
// Return: void
//=========================================================================
void gp_isp_awb_get_ct_gain(unsigned char *_awb, unsigned short ct, unsigned short *rgain, unsigned short *ggain, unsigned short *bgain);



//=========================================================================
// Function Name:  gp_isp_cvt_gamma_curve
// Purpose : convert rgb gamma curve to driver (10bit to 8 bit)
// Parameters : unsigned char *gamma: input gamma curve, total 1024 element,
//              unsigned int *gamma_driver: output rgb gamma value for driver
//
// Return: void
//=========================================================================
void gp_isp_cvt_gamma_curve(unsigned char *gamma, unsigned int *gamma_driver);




//=========================================================================
// Function Name:  gp_isp_af_set_win_size
// Purpose : set AF win size
// Parameters : unsigned char *_af: AF working memory
//              int win1_w: width of AF window1
//              int win1_h: height of AF window1
//              int win2_w: width of AF window2
//              int win2_h: height of AF window2
//              int win3_w: width of AF window3
//              int win3_h: height of AF window3
//
// Return: void
//=========================================================================
void gp_isp_af_set_win_size(unsigned char *_af, int win1_w, int win1_h, int win2_w, int win2_h, int win3_w, int win3_h);


//=========================================================================
// Function Name:  gp_isp_af_set_tolerance_ratio
// Purpose : set AF stable tolerance
// Parameters : unsigned char *_af: AF working memory
//              int step: step of VCM
//              int tolerance_up: tolerance upper bound, default = (int)(1.1 * 256)
//              int tolerance_lo: tolerance lower bound, default = (int)(0.9 * 256)
//              int tolerance_decay: the tolerance of lens position would be different
// Return: void
//=========================================================================
void gp_isp_af_set_tolerance(unsigned char *_af, int step, int tolerance_up, int tolerance_lo, int tolerance_decay);


//=========================================================================
// Function Name:  gp_isp_af_set_tolerance_ratio
// Purpose : set AF stable tolerance
// Parameters : unsigned char *_af: AF working memory
//              int max_idx: max index of VCM
//              int min_idx: min index of VCM
// Return: void
//=========================================================================
void gp_isp_af_set_focus_range(unsigned char *_af, int max_idx, int min_idx);

//=========================================================================
// Function Name :  gp_isp_af_get_workmem_size
// Purpose :  Get AF working memory size
// Parameters : void
// Return: size of working memory for AF
//=========================================================================
unsigned int gp_isp_af_get_workmem_size(void);

//=========================================================================
// Function Name : gp_isp_af_set_mode
// Purpose : set AF mode
// Parameters : unsigned char *_af: AF working memory
//              AF mode: af mode
// Return: void
//=========================================================================
int gp_isp_af_set_mode(unsigned char *_af, AF_SEARCH_MODE mode);

//=========================================================================
// Function Name:  gp_isp_af_calc
// Purpose : AF calculation
// Parameters : unsigned char *_af: AF working memory
//              IspAfResult_t *_af_result: af statistics from ISP
//              int idx: index of lens position
//
// Return: new index of lens position
//=========================================================================
int gp_isp_af_calc(unsigned char *_af, IspAfResult_t *_af_result, int idx, int af_force);


//=========================================================================
// Function Name:  gp_isp_af_set_weight
// Purpose : Set AF win weight
// Parameters : unsigned char *_af: AF working memory
//              int win1_wt: af win1 weighting value, from 0 ~ 16
//              int win2_wt: af win2 weighting value, from 0 ~ 16
//              int win3_wt: af win3 weighting value, from 0 ~ 16
//
// Return: none
//=========================================================================
void gp_isp_af_set_weight(unsigned char *_af, int win1_wt, int win2_wt, int win3_wt);


void gp_isp_awb_gen_wb_table(unsigned char *awb, short *thr_in, int is_7500k);
void gp_isp_get_lsc_gain(unsigned char *_awb, unsigned int *gain, unsigned short r_gain, unsigned short g_gain, unsigned short b_gain);
void gp_isp_lsc_set_vignetting(unsigned char *_awb, int vignetting_en, unsigned int *grid, int w, int h, int decay_ratio_r, int decay_ratio_g, int decay_ratio_b);

// speed:  Normal = 128, Max = 256, Min = 1, No change = 0
void gp_isp_awb_set_speed(unsigned char *_awb, int speed);

int gp_isp_awb_new_custom_wb(unsigned char *_awb, IspAwbResult_t *awb_result);


//=========================================================================
// Function Name:  gp_isp_gen_contrast
// Purpose : generate contrast curve
// Parameters : unsigned char *_ae: AE working memory
//              unsigned int cl: contrast value, from 0(disable) ~ 512 (strong)
//              unsigned int ema_w: update rate, from 1(slow) ~ 256(fast)
//              unsigned int lpt: low point, default 80, lpt > 80: more black, otherwise brightness
//              unsigned int hpt: high point, default 200, hpt > 200: supress the high light part
//              unsigned int *y_hist: histogram
//              unsigned int *ylut_in: org ylut
//              unsigned int *ylut_out: output ylut
//
// Return: none
//=========================================================================
void gp_isp_gen_contrast(unsigned char *_ae, unsigned int cl, unsigned int ema_w, unsigned int lpt, unsigned int hpt, unsigned int *y_hist, unsigned int *ylut_in, unsigned int *ylut_out);



//=========================================================================
// Function Name:  gp_isp_gen_contrast2
// Purpose : generate contrast curve
// Parameters : unsigned int bright: bright point, 192(bright) ~ 255 (org),
//              unsigned int dark: dark point, 0(org) ~ 64 (dark)
//              unsigned int *ylut: org ylut, and output ylut is also in this ylut
//
// Return: none
//=========================================================================
void gp_isp_gen_contrast2(unsigned int bright, unsigned int dark, unsigned int *ylut);



//=========================================================================
// Function Name:  gp_isp_gen_contrast3
// Purpose : generate contrast curve
// Parameters : unsigned int *y_hist: histogram
//              unsigned int bright_ratio: from 0(org) ~ 128(strong)
//              unsigned int dark_ratio: from 0(org) ~ 128(strong)
//              unsigned int min_bright: high point, 192(bright) ~ 255 (org)
//              unsigned int max_dark: low point, 0(org) ~ 64 (dark)
//              unsigned int *ylut: org ylut, and output ylut is also in this ylut
//
// Return: none
//=========================================================================
void gp_isp_gen_contrast3(unsigned int *y_hist, unsigned int bright_ratio, unsigned int dark_ratio, unsigned int min_bright, unsigned int max_dark, unsigned char* ylut);


//=========================================================================
// Function Name:  gp_isp_meq_contrast
// Purpose : generate contrast curve
// Parameters : unsigned char *_ae: AE working memory
//              unsigned int ema_w: update rate, from 1(slow) ~ 256(fast)
//              unsigned int cr: contrast value, from 0(strong) ~ 256 (weak)
//              unsigned int *y_hist: histogram
//              unsigned int *ylut_in: org ylut
//              unsigned int *ylut_out: output ylut
//
// Return: none
//=========================================================================
void gp_isp_meq_contrast(unsigned char *_ae, unsigned int ema_w, unsigned int cr, unsigned int *y_hist, unsigned int *ylut_in, unsigned int *ylut_out);

#endif



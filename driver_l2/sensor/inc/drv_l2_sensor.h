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
#ifndef DRV_L2_SENSOR_H
#define DRV_L2_SENSOR_H
#include "project.h"
#include "drv_l2_cdsp.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
// sccb interface
#define SENSOR_I2C_MODE_HW		0
#define SENSOR_I2C_MODE_SW		1

#define MAX_SENSOR_NUM	        2	/* Support 1 sensor in platform */
#define MAX_INFO_NUM	        2
#define MIPI_CLK_DIV            3 			/* notice: mipi clock dont higher than CDSP clock*/

#define SINGLE_SENSOR           0
#define FRONT_SENSOR            1
#define REAR_SENSOR             2

#define	CSI_0_PATH               0
#define CSI_1_PATH               1

/* Define the sensor name we support now */
#define SOURCE_CDSP_DVP		    			"sensor_cdsp_dvp"
#define SOURCE_CDSP_MIPI					"sensor_cdsp_mipi"
#define SOURCE_CSI0_DVP						"sensor_csi0_dvp"
#define SOURCE_CSI0_MIPI		   			"sensor_csi0_mipi"
#define SOURCE_CSI1_DVP						"sensor_csi1_dvp"
#define SOURCE_CSI1_MIPI		   			"sensor_csi1_mipi"
#define SOURCE_USB_CAM		    			"sensor_usb_cam"

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
	MCLK_NONE = 0,
	MCLK_12M,
	MCLK_13_5M,
	MCLK_18M,
	MCLK_24M,
	MCLK_27M,
	MCLK_37M125,
	MCLK_48M,
	MCLK_SYSTEM_DIV2,
	MCLK_SYSTEM_DIV4
} CSI_CLK_DEF;

typedef enum
{
	MODE_CCIR_601 = 0,
	MODE_CCIR_656,
	MODE_CCIR_HREF
} TIMING_MODE;

typedef enum
{
	MODE_NONE_INTERLACE = 0,
	MODE_INTERLACE
} INTERLACE_MODE;

typedef enum
{
	MODE_POSITIVE_EDGE = 0,
	MODE_NEGATIVE_EDGE
} PCLK_MODE;

typedef enum
{
	MODE_ACTIVE_LOW = 0,
	MODE_ACTIVE_HIGH
} HVSYNC_MODE;

typedef enum
{
	MODE_PSCALER_DISABLE = 0,
	MODE_PSCALER_SRC_CDSP,
	MODE_PSCALER_SRC_CSI
} PSCALER_MODE;

// sensor format is LSB to MSB
typedef enum
{
	V4L2_PIX_FMT_RGB565 = 0,
	V4L2_PIX_FMT_YUYV,
	V4L2_PIX_FMT_YVYU,
	V4L2_PIX_FMT_UYVY,
	V4L2_PIX_FMT_VYUY,
	V4L2_PIX_FMT_SBGGR8, //5, raw format order affect binmode raw8/raw10 setting in drv_l2_cdsp_stream_on()
	V4L2_PIX_FMT_SGBRG8,
	V4L2_PIX_FMT_SGRBG8,
	V4L2_PIX_FMT_SRGGB8,
	V4L2_PIX_FMT_SGRBG10,
	V4L2_PIX_FMT_SRGGB10, //10
	V4L2_PIX_FMT_SBGGR10,
	V4L2_PIX_FMT_SGBRG10,
	V4L2_PIX_FMT_GP420,
	V4L2_PIX_FMT_MAX
} SENSOR_FORMAT;

typedef enum
{
	SENSOR_PATH_CDSP = 0,
	SENSOR_PATH_CSI,
	SENSOR_PATH_USB
} SENSOR_PATH;

typedef enum
{
	SENSOR_INTERFACE_DVP = 0,		//parallel
	SENSOR_INTERFACE_MIPI
} SENSOR_SOURCE;

typedef enum
{
	MD_NONE = 0,
	MD_64_SIZE,
	MD_32_SIZE,
	MD_16_SIZE,
	MD_8_SIZE
} SENSOR_MD;

typedef enum {
	SENSOR_SET_H_FLIP = 0,
	SENSOR_SET_V_FLIP,
	SENSOR_SET_DENOISE,
	SENSOR_SET_MAX
}sensor_ctrl;

typedef struct drv_l2_sensor_info_s
{
	INT32U mclk;				/* master clock */
	INT32U input_format;		/* input format */
	INT32U output_format;		/* output format */
	INT32U sensor_fps;			/* FPS in sensor */
	INT16U target_w; 			/* sensor width */
	INT16U target_h;			/* sensor height */
	INT16U sensor_w; 			/* sensor width */
	INT16U sensor_h;			/* sensor height */
	INT16U hoffset; 			/* sensor h offset */
	INT16U voffset;				/* sensor v offset */
	INT8U interface_mode;		/* input interface, HREF CCIR601 and CCIR656 */
	INT8U interlace_mode;		/* interlace mode */
	INT8U hsync_active;			/* hsync pin active level */
	INT8U vsync_active;			/* vsync pin active level */
} drv_l2_sensor_info_t;

typedef struct drv_l2_sensor_ops_s
{
	char*	name;
	void   (*init)(void);
	void   (*uninit)(void);
	void   (*stream_start)(INT32U index, INT32U bufA, INT32U bufB);
	void   (*stream_stop)(void);
	drv_l2_sensor_info_t*   (*get_info)(INT32U index);
    void   (*set_exptime)(int ev_step);
    void   (*get_ae_info)(sensor_exposure_t *si);
    void   (*set_max_lum)(int max_lum);
    void   (*exposure_set_freq) (INT8U num);
	drv_l2_sensor_info_t	info[MAX_INFO_NUM];
} drv_l2_sensor_ops_t;

typedef struct drv_l2_sensor_para_s
{
    INT8U md_threshold;        /* sensor modition threshold */
    INT32U pscaler_src_mode;   /* sensor output selection: PSCALER_MODE */
    INT32U md_mode;            /* sensor modition enable, 0:disable, 1:enable*/
    INT32U md_block_size;      /* sensor modition block size, 0: 16x16, 1: 8x8*/
    INT32U md_buf;             /* sensor modition workmem */
    INT8U  md_update_frame;
    void (*md_csi_callback)(INT32U event);/* sensor modition callback function for CSI */
    void (*md_callback)(void); /* sensor modition callback function for CDSP */
} drv_l2_sensor_para_t;

typedef struct sensor_frame_range_s
{
	INT32U	point_x;
	INT32U	point_y;
	INT32U	frame_w;
	INT32U	frame_h;
	INT32U	clip_w;
	INT32U	clip_h;
	INT32U	scaledown_w;
	INT32U	scaledown_h;
	INT8U	docropflag;
	INT8U	doscaledownflag;
	INT32U	frameAddrs;
} sensor_frame_range_t;

/*********************************************************************
        External ops declaration for sensors we spport now
**********************************************************************/

#if (defined _SENSOR_H42_CDSP_MIPI) && (_SENSOR_H42_CDSP_MIPI == 1)
    extern const drv_l2_sensor_ops_t h42_cdsp_mipi_ops;
#endif

#if (defined _SENSOR_H62_CDSP_MIPI) && (_SENSOR_H62_CDSP_MIPI == 1)
    extern const drv_l2_sensor_ops_t h62_cdsp_mipi_ops;
#endif

#if (defined _SENSOR_GC1054_CDSP_MIPI) && (_SENSOR_GC1054_CDSP_MIPI == 1)
    extern const drv_l2_sensor_ops_t gc1054_cdsp_mipi_ops;
#endif

#if (defined _SENSOR_GC1064_CDSP_MIPI) && (_SENSOR_GC1064_CDSP_MIPI == 1)
    extern const drv_l2_sensor_ops_t GC1064_cdsp_mipi_ops;
#endif

#if (defined _SENSOR_GC2053_CDSP_MIPI) && (_SENSOR_GC2053_CDSP_MIPI == 1)
    extern const drv_l2_sensor_ops_t gc2053_cdsp_mipi_ops;
#endif

#if (defined _SENSOR_GC2083_CDSP_MIPI) && (_SENSOR_GC2083_CDSP_MIPI == 1)
    extern const drv_l2_sensor_ops_t gc2083_cdsp_mipi_ops;
#endif

#if (defined _SENSOR_GC4653_CDSP_MIPI) && (_SENSOR_GC4653_CDSP_MIPI == 1)
	extern const drv_l2_sensor_ops_t GC4653_cdsp_mipi_ops;
#endif

#if (defined _SENSOR_GC5025_CDSP_MIPI) && (_SENSOR_GC5025_CDSP_MIPI == 1)
	extern const drv_l2_sensor_ops_t gc5025_cdsp_mipi_ops;
#endif

#if (defined _SENSOR_IMX230_CDSP_MIPI) && (_SENSOR_IMX230_CDSP_MIPI == 1)
    extern const drv_l2_sensor_ops_t imx230_cdsp_mipi_ops;
#endif

#if (defined _SENSOR_TP9950_AHD_CSI_DVP) && (_SENSOR_TP9950_AHD_CSI_DVP == 1)
	extern const drv_l2_sensor_ops_t    tp9950_csi_dvp_ops;
#endif

#if (defined _SENSOR_TP9951_AHD_CSI_MIPI) && (_SENSOR_TP9951_AHD_CSI_MIPI == 1)
    extern const drv_l2_sensor_ops_t tp9951_csi_mipi_ops;
#endif

#if (defined _SENSOR_TP9951_AHD_CSI_DVP) && (_SENSOR_TP9951_AHD_CSI_DVP == 1)
    extern const drv_l2_sensor_ops_t tp9951_csi_dvp_ops;
#endif

#if (defined _SENSOR_PR2000_AHD_CSI_DVP) && (_SENSOR_PR2000_AHD_CSI_DVP == 1)
    extern const drv_l2_sensor_ops_t pr2000_dvp_csi_ops;
#endif

#if (defined _SENSOR_PR2000_AHD_CSI_MIPI) && (_SENSOR_PR2000_AHD_CSI_MIPI == 1)
    extern const drv_l2_sensor_ops_t pr2000_csi_mipi_ops;
#endif

#if (defined _SENSOR_PR2000_AHD_CDSP_MIPI) && (_SENSOR_PR2000_AHD_CDSP_MIPI == 1)
    extern const drv_l2_sensor_ops_t pr2000_cdsp_mipi_yuv_ops;
#endif

#if (_SENSOR_USB_MJPEG_CAM_FRONT)
    extern const drv_l2_sensor_ops_t usb_mjpeg_cam_ops_front;
#endif

#if (_SENSOR_USB_MJPEG_CAM_REAR)
    extern const drv_l2_sensor_ops_t usb_mjpeg_cam_ops_rear;
#endif

#if (defined _SENSOR_RN6752_AHD_CSI_DVP) && (_SENSOR_RN6752_AHD_CSI_DVP == 1)
    extern const drv_l2_sensor_ops_t	rn6752_csi_dvp_ops;
#endif

#if (defined _SENSOR_RN6752_AHD_CDSP_DVP) && (_SENSOR_RN6752_AHD_CDSP_DVP == 1)
    extern const drv_l2_sensor_ops_t	rn6752_cdsp_dvp_ops;
#endif

#if (defined _SENSOR_PR2100_AHD_MIPI_DVP) && (_SENSOR_PR2100_AHD_MIPI_DVP == 1)
    extern const drv_l2_sensor_ops_t    pr2100_cdsp_mipi_ops;
    extern const drv_l2_sensor_ops_t	pr2100_csi_dvp_ops;
#endif

#if (defined _SENSOR_GC2145_YUV_CSI_MIPI) && (_SENSOR_GC2145_YUV_CSI_MIPI == 1)
	extern const drv_l2_sensor_ops_t    gc2145_csi_mipi_ops;
#endif

#if (defined _SENSOR_PR2100_AHD_MIPI_CDSP_CSI) && (_SENSOR_PR2100_AHD_MIPI_CDSP_CSI == 1)
    extern const drv_l2_sensor_ops_t    pr2100_cdsp_mipi_ops;
    extern const drv_l2_sensor_ops_t	pr2100_csi_mipi_ops;
#endif

#if (defined _SENSOR_GC0308_CSI_DVP) && (_SENSOR_GC0308_CSI_DVP == 1)
	extern const drv_l2_sensor_ops_t 	gc0308_sensor_csi_ops;
#endif

#if (defined _SENSOR_IMX307_CDSP_MIPI) && (_SENSOR_IMX307_CDSP_MIPI == 1)
	extern const drv_l2_sensor_ops_t imx307_cdsp_mipi_ops;
#endif

#if (defined _SENSOR_IMX376_CDSP_MIPI) && (_SENSOR_IMX376_CDSP_MIPI == 1)
	extern const drv_l2_sensor_ops_t IMX376_cdsp_mipi_ops;
#endif

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void drv_l2_sensor_register_ops(drv_l2_sensor_ops_t* sensor, int i);
extern void drv_l2_sensor_init(void);
extern drv_l2_sensor_ops_t* drv_l2_sensor_get_ops(INT32U index);
extern drv_l2_sensor_para_t* drv_l2_sensor_get_para(void);
extern void drv_l2_sensor_set_csi_source(INT32U image_src, INT8U csi_num, INT8U mipi_num);
extern INT32S drv_l2_sensor_set_mclkout(INT32U mclk_sel, INT8U csi_num);
extern void drv_l2_mipi_ctrl_set_clk(INT8U mipiclk_on, INT32U mipiclk_div);
extern void drv_l2_user_sensor_init(INT32U isp_bufferA, INT32U isp_bufferB);
extern INT16U drv_l2_user_sensor_width_get(INT32U index);
extern INT16U drv_l2_user_sensor_height_get(INT32U index);
extern void   drv_l2_user_sensor_frequence(INT32U freq_num);
extern INT32U drv_l2_sensor_get_path(INT32U index);
extern INT32S drv_l2_sensor_frame_range_clip(sensor_frame_range_t *frameRange);
extern INT16U drv_l2_user_csi_sensor_out_format_get(INT32U index);

#endif  //DRV_L2_SENSOR_H

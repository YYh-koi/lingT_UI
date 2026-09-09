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
#ifndef __CUSTOMER_H__
#define __CUSTOMER_H__

/**************************************************************************
 *             		S E N S O R			C O N F I G                       *
 **************************************************************************/
// sensor device enable
/*== Front ==*/
#define _SENSOR_H42_CDSP_MIPI				0
#define _SENSOR_H62_CDSP_MIPI				0
#define _SENSOR_GC1054_CDSP_MIPI			0
#define _SENSOR_GC1064_CDSP_MIPI			0
#define _SENSOR_GC2053_CDSP_MIPI            1
#define _SENSOR_GC2083_CDSP_MIPI			0
#define _SENSOR_GC4653_CDSP_MIPI        	0
#define _SENSOR_GC5025_CDSP_MIPI        	0
#define _SENSOR_IMX307_CDSP_MIPI            0
#define _SENSOR_PR2000_AHD_CSI_MIPI			0
#define _SENSOR_PR2000_AHD_CDSP_MIPI		0
#define _SENSOR_TP9951_AHD_CSI_MIPI			0
#define _SENSOR_IMX230_CDSP_MIPI			0
//YUV
#define _SENSOR_GC2145_YUV_CSI_MIPI			0
#define _SENSOR_GC0308_CSI_DVP              0
#define _SENSOR_USB_MJPEG_CAM_FRONT			0

/*== Rear ==*/
#define _SENSOR_USB_MJPEG_CAM_REAR			0
#define _SENSOR_RN6752_AHD_CSI_DVP			0
#define _SENSOR_RN6752_AHD_CDSP_DVP			0
#define _SENSOR_TP9950_AHD_CSI_DVP			0
#define _SENSOR_PR2000_AHD_CSI_DVP          0
#define _SENSOR_TP9951_AHD_CSI_DVP			0
/*== 2 in 1  ADH  ==*/
#define _SENSOR_PR2100_AHD_MIPI_DVP			0
#define _SENSOR_PR2100_AHD_MIPI_CDSP_CSI	0


#if(_SENSOR_USB_MJPEG_CAM_FRONT || _SENSOR_USB_MJPEG_CAM_REAR)
//Master definition
#define _SENSOR_USB_MJPEG_CAM				1
#endif

/**************************************************************************
 *             			L C D			C O N F I G                       *
 **************************************************************************/
#define LCD_DRV_EN_TPO_TD025THD1	    	0
#define LCD_DRV_EN_T43                      0
#define LCD_DRV_EN_ST7703_QX550H4001    	0
#define LCD_DRV_EN_ILI9486L_CK3510S0    	0
#define LCD_DRV_EN_WTL096601G06         	0
#define LCD_DRV_EN_WTL096602G02         	0
#define LCD_DRV_EN_TXDT240C_OTA5182A   	 	0
#define LCD_DRV_EN_HDMI                 	0
#define LCD_DRV_EN_AUO_A027DTN019			0
#define LCD_DRV_EN_HSD_GC9503V          	0
#define LCD_DRV_EN_DST_GC9106           	0
#define LCD_DRV_EN_TP2915               	1
#define LCD_DRV_EN_LY101BOE                 0
#define LCD_DRV_EN_ST7701S				    0
#define LCD_DRV_EN_KD055HD_IL9881C	        0
#define LCD_DRV_EN_JD9366TC			        0
#define LCD_DRV_EN_st7789_I80				0
#define LCD_DRV_EN_st7789_I80_240x240		0
#define LCD_DRV_EN_CK69502S0_FL7703N        0

#if(LCD_DRV_EN_HDMI)
#define HDMI_DISP_RESOLUTION    DISDEV_TFT2_HDMI1080P
#define C_HDMI_SCALE_UP_EN    	CUSTOM_ON
#endif

/**************************************************************************
 *             			S Y S T E M		F I F O                           *
 **************************************************************************/
// CDSP 3DNR: HW Design			  [0x3XXX_XXXX]
// PscalerA Bypass use SYS_FIFO_2 [0x5XXX_XXXX]
// PscalerB Bypass use SYS_FIFO_1 [0x4XXX_XXXX]

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define CUSTOM_ON	1
#define CUSTOM_OFF	0

#define WIFI_VENDOR_SSV                 1
#define WIFI_VENDOR_RTK                 2
#define WIFI_VENDOR_ATBM                3

#ifndef WIFI_VENDOR
#define WIFI_VENDOR                     WIFI_VENDOR_RTK
#endif

#define STATISTICS_DUMP				    CUSTOM_ON
#define STATISTICS_DUMP_TIME		    (30)

/*== Power on ==*/
#define C_LOGO_POWER_ON					CUSTOM_ON
#define C_AUDIO_POWER_ON				CUSTOM_ON

/*== Power off ==*/
#define C_LOGO_POWER_OFF				CUSTOM_ON
#define C_IDLE_POWER_OFF				CUSTOM_OFF

/*== System ==*/
#define C_WATCHDOG						CUSTOM_OFF
#define C_RTC_REBOOT					CUSTOM_OFF
#define C_FIRMWARE_UPGRADE				CUSTOM_ON
#define C_SDC_UPGRADE_ERASE_USER_CONFIG	CUSTOM_OFF
#define C_DEL_SDC_FIRMWARE_BIN			CUSTOM_ON
#define C_VOICE_RECOGNIZE				CUSTOM_OFF
#define C_WIFI_FUNC						CUSTOM_ON
#define C_WIFI_ON_OFF_KEY				CUSTOM_OFF
#define C_APP_DOUBLE_BUFFER_DOWNLOAD	CUSTOM_ON
#define C_TFT_DISPLAY_FUNC				CUSTOM_ON
#define C_WATER_MARK                    CUSTOM_OFF
#define C_REV_BACK_SENSOR_FULL_SCREEN	CUSTOM_ON
#define C_AUDIO_RECORD_EN               CUSTOM_OFF
#define C_DISP_SLIDE_SAVE				CUSTOM_OFF
#define C_BLIND_SPOT_DETECTION          CUSTOM_OFF

#define C_WIFI_SEND_FIX_H264I		CUSTOM_ON		
#define C_DISABLE_SENSOR_TFT	CUSTOM_ON



/*== Video Record ==*/
#define C_SUPORT_GET_JPG_BUF			CUSTOM_ON
#define C_CYCLIC_VIDEO_RECORD			CUSTOM_ON
#define C_AUTO_DEL_FILE					CUSTOM_OFF
#define C_RENAME_LOCK_FILE				CUSTOM_ON
#define C_MOTION_DETECTION				CUSTOM_OFF
#define C_MOTION_DET_NO_CANCEL          CUSTOM_OFF
#define C_VID_REC_USE_SYS_FIFO          CUSTOM_ON	// Put pscaler bypass buffer into system fifo
#define C_TIME_LAPSE    				CUSTOM_OFF
#define C_APP_PLAYBACK_WITH_SOUND       CUSTOM_OFF
#define C_VID_INTERPOLATING_2K			CUSTOM_OFF
#define C_AHDCSI_GP420_EN               CUSTOM_OFF // ON:output GP420 format OFF:output YUYV format
#define C_ACTUATOR						CUSTOM_OFF
#define C_PACKER_AES_ENCRYPT			CUSTOM_OFF
/*== Photo ==*/
#define C_PHOTO_JPEG_ADD_EXIF			CUSTOM_OFF
       

/*== Y only ==*/
#define C_Y_ONLY_FUNCTION				CUSTOM_OFF

/*== Browse ==*/
#define C_BROWSE_FULL_SCREEN			CUSTOM_ON

/*== Peripheral ==*/
#define C_GSENSOR_FUNCTION				CUSTOM_OFF
#define C_BATTERY_DETECT				CUSTOM_OFF
#define C_BATTERY_LOW_POWER_OFF			CUSTOM_OFF
#define C_SCREEN_SAVER					CUSTOM_ON
#define C_GPIO_SPEEDUP_SD_CHECK			CUSTOM_OFF
#define C_USB_DEVICE					CUSTOM_OFF
#define C_USB_AUDIO						CUSTOM_OFF
#define C_USB_H264						CUSTOM_ON
#define C_STORAGE_DEVICE                CUSTOM_OFF
#define C_CARCAM_UVC_DISPLAY			CUSTOM_OFF // OFF:PCAM mode  ON:In-vehicle Infotainment System mode
#if (LVGL_SUPPORT == 1)
#define C_LVGL_SUPPORT					CUSTOM_ON
#else
#define C_LVGL_SUPPORT  				CUSTOM_OFF
#endif

/*== Storage Type ==*/
#define STORAGE_SD_DEV					1 // 0:SD0, 1:SD1
#if STORAGE_SD_DEV == 0
#define STORAGE_DEVICE					FS_SD0
#else
#define STORAGE_DEVICE					FS_SD1
#endif

/*== Resource ==*/
#define RESOURCE_GP420          		1
#define RESOURCE_RGB565         		2
#define RESOURCE_YUYV           		3
#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT == 1)
#define RESOURCE_FORMAT         		RESOURCE_RGB565
#else
#define RESOURCE_FORMAT         		RESOURCE_GP420
#endif

/*== Debug ==*/
#define C_DEBUG_MESSAGE					CUSTOM_ON
#define C_USER_MESSAGE					CUSTOM_OFF
#define C_SUPPORT_JTAG					CUSTOM_ON
#define C_STATISTICS_DUMP				CUSTOM_OFF
#define C_CREATE_FILE_DEBUG				CUSTOM_OFF
#define C_SENSOR_SAVE_RAW_DATA			CUSTOM_OFF
#define C_AEAWB_INFO_STAMP				CUSTOM_OFF
#define C_WIFI_INFO_STAMP				CUSTOM_OFF

#if C_DEBUG_MESSAGE
#define DBG_PRINT_UART					1
#ifndef DBG_PRINT
#define DBG_PRINT(...)					do{print_string(__VA_ARGS__);}while(0)
#endif
#else
#define DBG_PRINT(...)					do{}while(0)
#endif

#if C_USER_MESSAGE
#define USER_PRINT_UART					1
#ifndef USER_PRINT
#define USER_PRINT(...)					do{user_print_string(__VA_ARGS__);}while(0)
#endif
#else
#define USER_PRINT(...)					do{}while(0)
#endif

/*== EXIF ==*/
#if (defined C_PHOTO_JPEG_ADD_EXIF) && (C_PHOTO_JPEG_ADD_EXIF)

#define ENABLE_WITH_THUMBNAIL_IMAGE		0

/*
	Don't change, limit size of EXIF header is 64KB
*/
#define EXIT_THUMBNAIL_WIDTH			320
#define EXIT_THUMBNAIL_HEIGHT			240
#endif

/*== WIFI ==*/
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
#define WIFI_FW_READ_FROM_FLASH			CUSTOM_ON

#define WIFI_SMARTCONFIG_MODE       	0
#define WIFI_STATION_MODE           	1
#define WIFI_AP_MODE                	2
#define WIFI_RUN_MODE               	WIFI_AP_MODE

/*
Encode Format:
*/
#define WIFI_STREAM_JPEG				0
#define WIFI_STREAM_H264				1
#define WIFI_STREAM_FORMAT				WIFI_STREAM_H264
#endif

#if (WIFI_STREAM_FORMAT == WIFI_STREAM_H264) // H264
#define WIFI_WIDTH						800
#define WIFI_HEIGHT						448
#define WIFI_PIP_WIDTH					304
#define WIFI_PIP_HEIGHT					160
#else // JPEG
#define WIFI_WIDTH						640
#define WIFI_HEIGHT						480
#define WIFI_PIP_WIDTH					256
#define WIFI_PIP_HEIGHT					192
#endif

/*== VOICE RECOGNITION ==*/
#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
#define VOICE_RECG_WANSON				0
#define VOICE_RECG_MODULE				VOICE_RECG_WANSON
#endif

/*== HOST USB MJPG CAM ==*/
#define HOST_USB_OHCI					0   //USB1.1 for VGA
#define HOST_USB_EHCI					1   //USB2.0 for 720P

#if (_SENSOR_USB_MJPEG_CAM_FRONT == 1)
#define HOST_USB_TYPE_FRONT				HOST_USB_EHCI

#if HOST_USB_TYPE_FRONT == HOST_USB_OHCI
#define HOST_USB_MJPEG_Q_CNT_FRONT			4
#define HOST_USB_MJPEG_BUF_SIZE_FRONT			(100 * 1024) // for host usb put jpeg

#define REVERSING_CHECK_USB_COMMAND_FRONT		CUSTOM_ON

#define USB_OHCI_OUT_WIDTH_FRONT		640
#define USB_OHCI_OUT_HEIGHT_FRONT		480
#endif

#if HOST_USB_TYPE_FRONT == HOST_USB_EHCI
#define HOST_USB_MJPEG_Q_CNT_FRONT			4
#define HOST_USB_MJPEG_BUF_SIZE_FRONT			(200 * 1024) // for host usb put jpeg

#define REVERSING_CHECK_JPEG_HEADER_FRONT		CUSTOM_ON

#define USB_EHCI_OUT_WIDTH_FRONT		1280
#define USB_EHCI_OUT_HEIGHT_FRONT		720
#endif
#endif //#if (_SENSOR_USB_MJPEG_CAM_FRONT == 1)

#if (_SENSOR_USB_MJPEG_CAM_REAR == 1)
#define HOST_USB_TYPE_REAR				HOST_USB_EHCI

#if HOST_USB_TYPE_REAR == HOST_USB_OHCI
#define HOST_USB_MJPEG_Q_CNT_REAR			4
#define HOST_USB_MJPEG_BUF_SIZE_REAR			(100 * 1024) // for host usb put jpeg

#define REVERSING_CHECK_USB_COMMAND_REAR		CUSTOM_ON

#define USB_OHCI_OUT_WIDTH_REAR			640
#define USB_OHCI_OUT_HEIGHT_REAR		480
#endif

#if HOST_USB_TYPE_REAR == HOST_USB_EHCI
#define HOST_USB_MJPEG_Q_CNT_REAR			4
#define HOST_USB_MJPEG_BUF_SIZE_REAR			(200 * 1024) // for host usb put jpeg

#define REVERSING_CHECK_JPEG_HEADER_REAR		CUSTOM_ON

#define USB_EHCI_OUT_WIDTH_REAR			1280
#define USB_EHCI_OUT_HEIGHT_REAR		720
#endif
#endif //#if (_SENSOR_USB_MJPEG_CAM_REAR == 1)

/**************************************************************************
 *             A P P L I C A T I O N	C O N F I G                       *
 **************************************************************************/
/*===VIDEO===*/
///+++++++++FRONT++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define VID_REC_FRAME_RATE_FRONT    			25
#define VID_GOP_LEN_FRONT 						10

#define VID_REC_BITRATE_RATE_FRONT     			12000*1000 //Kbps

#if (C_VID_INTERPOLATING_2K == CUSTOM_ON)// It's frame buffer for scale-up
#define INTERP2K_PSCALER_OUT_CNT_FRONT			4 // Save Pscaler_A's bypass output buffer
#else
#define INTERP2K_PSCALER_OUT_CNT_FRONT			0
#endif

#if (_SENSOR_USB_MJPEG_CAM_FRONT == CUSTOM_ON)
#define VID_REC_LINE_FRONT						0 // must be frame mode
#else
#define VID_REC_LINE_FRONT						288 // 0:frame mode  288:fifo mode
#endif

#if (defined VID_REC_LINE_FRONT) && (VID_REC_LINE_FRONT == 0)
// frame mode
#define	VID_PSCALER_OUT_CNT_FRONT				4
#else
// fifo mode
#define	VID_PSCALER_OUT_CNT_FRONT				10
#endif

#define	VID_H264_OUT_I_CNT_FRONT				6//8
#define	VID_H264_OUT_I_SIZE_FRONT				(350*1024)
#define	VID_H264_OUT_P_CNT_FRONT				24//30
#define	VID_H264_OUT_P_SIZE_FRONT				(200*1024)
#define	VID_H264_OUT_SKIP_CNT_FRONT				20
#define	VID_H264_OUT_SKIP_SIZE_FRONT			160

#define H264_QP_START_FRONT						27
#define H264_QP_MAX_FRONT						44
#define H264_QP_MIN_FRONT						22
#define H264_HIERPFRAME_FRONT					5 //0:disable, >0:ENABLE, max same with GOP_LEN

#define	VID_JPEG_OUT_CNT_FRONT					5
#define	VID_JPEG_OUT_SIZE_FRONT					(600*1024)

///+++++++++REAR++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define VID_REC_FRAME_RATE_REAR    			    25
#define VID_GOP_LEN_REAR 						10

#define VID_REC_BITRATE_RATE_REAR     			6000*1000 //Kbps

#if (_SENSOR_USB_MJPEG_CAM_REAR == CUSTOM_ON) || (C_VID_INTERPOLATING_2K == CUSTOM_ON)
#define VID_REC_LINE_REAR						0 // must be frame mode
#else
#define VID_REC_LINE_REAR						288 // 0:frame mode  288:fifo mode
#endif

#if (defined VID_REC_LINE_REAR) && (VID_REC_LINE_REAR == 0)
// frame mode
#define	VID_PSCALER_OUT_CNT_REAR				2
#else
// fifo mode
#define	VID_PSCALER_OUT_CNT_REAR				10
#endif

#define	VID_H264_OUT_I_CNT_REAR					5
#define	VID_H264_OUT_I_SIZE_REAR				(220*1024)
#define	VID_H264_OUT_P_CNT_REAR					20
#define	VID_H264_OUT_P_SIZE_REAR				(120*1024)
#define	VID_H264_OUT_SKIP_CNT_REAR				20
#define	VID_H264_OUT_SKIP_SIZE_REAR				160

#define H264_QP_START_REAR						27
#define H264_QP_MAX_REAR						44
#define H264_QP_MIN_REAR						22
#define H264_HIERPFRAME_REAR					0 //0:disable, >1:ENABLE, max same with GOP_LEN

#define	VID_JPEG_OUT_CNT_REAR					5
#define	VID_JPEG_OUT_SIZE_REAR					(200*1024)

///+++++++++WIFI++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define WIFI_REC_FRAME_RATE		    			25
#define WIFI_GOP_LEN 							25
#define WIFI_REC_BITRATE_RATE        			3200*1000 // 1.5M

#define	WIFI_H264_OUT_I_CNT						8
#define	WIFI_H264_OUT_I_SIZE					(130*1024)
#define	WIFI_H264_OUT_P_CNT						30
#define	WIFI_H264_OUT_P_SIZE					(80*1024)

#define WIFI_H264_QP_START						30
#define WIFI_H264_QP_MAX						35
#define WIFI_H264_QP_MIN						25
#define WIFI_H264_HIERPFRAME					5//0:disable, >0:ENABLE, max same with GOP_LEN

///+++++++++UVC++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define UVC_REC_FRAME_RATE		    			25
#define UVC_GOP_LEN 							30
#define UVC_REC_BITRATE_RATE        			0x180000 // 1.5M

#define	UVC_H264_OUT_I_CNT						3
#define	UVC_H264_OUT_I_SIZE						(130*1024)
#define	UVC_H264_OUT_P_CNT						30
#define	UVC_H264_OUT_P_SIZE					    (70*1024)

#define UVC_H264_QP_START						32
#define UVC_H264_QP_MAX							51
#define UVC_H264_QP_MIN							27
#define UVC_H264_HIERPFRAME						0 //0:disable, >0:ENABLE, max same with GOP_LEN

#define	UVC_JPEG_OUT_CNT					    5
#define	UVC_JPEG_OUT_SIZE					    (200*1024)
///+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/*== H264 ENCODE FBC ==*/
#define H264ENC_FBC_ENABLE                      1       //Reduce h264 encode bus bandwith, but increase ram buffer
#define H264ENC_FBC_RAM_RATIO                   85      //Control ram buffer size

/*== Photo Capture ==*/
#define	PHOTO_PSCALER_OUT_CNT					1


//FRONT
#define	PHOTO_JPEG_OUT_CNT_FRONT				2
#define	PHOTO_JPEG_OUT_SIZE_FRONT				(700*1024)

//REAR
#define	PHOTO_JPEG_OUT_CNT_REAR					2
#define	PHOTO_JPEG_OUT_SIZE_REAR				(700*1024)

/*== Snapshot ==*/
//FRONT
#define	SNAPSHOT_JPEG_OUT_SIZE_FRONT			(700*1024)

//REAR
#define	SNAPSHOT_JPEG_OUT_SIZE_REAR				(700*1024)

#endif //__CUSTOMER_H__

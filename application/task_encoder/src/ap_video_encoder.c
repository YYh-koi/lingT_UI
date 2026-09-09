/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
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
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park,                      *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#include "application.h"

extern INT32S video_encode_state_task_create(void);
extern INT32S audio_encode_state_task_create(void);
extern INT32S video_encode_state_sensor_start(void);
extern INT32S video_encode_state_sensor_stop(void);
extern INT32S video_encode_state_preview_mode_set(INT32U mode);

extern INT32S video_encode_state_encode_start(void);
extern INT32S video_encode_state_encode_stop(void);
extern INT32S video_encode_state_photo_start(void);

extern INT32S audio_encode_state_encode_start(void);
extern INT32S audio_encode_state_encode_stop(void);

#if C_SUPORT_GET_JPG_BUF
extern INT32S video_encode_state_jpeg_on_enc_get(void);
extern INT32S video_encode_state_release_get_jpeg_buf(INT8U src);
#endif

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static INT32S
video_encode_resolution_get(
	INT32U ap_mode
)
{
	VID_INFO* pVid_info_front = &pStreamArgs->vid_info_front;
	VID_INFO* pVid_info_rear = &pStreamArgs->vid_info_rear;
	#if (defined C_Y_ONLY_FUNCTION) && (C_Y_ONLY_FUNCTION)
	Y_ONLY_INFO* pY_only_info = &pStreamArgs->y_only_inf;
	#endif
	DISP_INFO* pDisp_info = &pStreamArgs->disp_info;
	#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
	//VID_INFO* pVid_info_uvc = &pStreamArgs->vid_info_uvc;
	#endif

	INT8U video_resolution;
	INT32U sensor_width,sensor_height;
	INT32U RealWidth, RealHeight;

	memset(pVid_info_front,0,sizeof(VID_INFO));
	memset(pVid_info_rear,0,sizeof(VID_INFO));
	#if (defined C_Y_ONLY_FUNCTION) && (C_Y_ONLY_FUNCTION)
	memset(pY_only_info,0,sizeof(Y_ONLY_INFO));
	#endif
	memset(pDisp_info,0,sizeof(DISP_INFO));

	sensor_width = drv_l2_user_sensor_width_get(PREVIEW_SRC_FRONT);
	RealHeight = sensor_height = drv_l2_user_sensor_height_get(PREVIEW_SRC_FRONT);

	pVid_info_front->data_source = drv_l2_sensor_get_path(PREVIEW_SRC_FRONT);

	if(pVid_info_front->data_source == VID_SRC_USB)
	{
    	RealWidth = sensor_width;
	}
	else
	{
    	RealWidth = RealHeight * 4 / 3;
	}

	if(ap_mode == STATE_PHOTO_CAPTURE)
	{
		pVid_info_front->vid_rec_type = VID_REC_JPEG;

       	#if 0 // Debug
		video_resolution = 1;// R_RANDOM1 & 7;
		ap_state_config_pic_size_set(video_resolution);
		#else
		video_resolution = ap_state_config_pic_size_get();
		#endif

		DBG_PRINT("==STATE_PHOTO_CAPTURE:%d==\r\n",video_resolution);

		switch(video_resolution)
		{
			case 0:		// 4032*3024
			pVid_info_front->sensor_width = RealWidth;
			pVid_info_front->sensor_height = RealHeight;
			pVid_info_front->target_width = 4032;
			pVid_info_front->target_height = 3024;
			break;

			case 1:		// 3648*2736
			pVid_info_front->sensor_width = RealWidth;
			pVid_info_front->sensor_height = RealHeight;
			pVid_info_front->target_width = 3648;
			pVid_info_front->target_height = 2736;
			break;

			case 2:		// 3264*2448
			pVid_info_front->sensor_width = RealWidth;
			pVid_info_front->sensor_height = RealHeight;
			pVid_info_front->target_width = 3264;
			pVid_info_front->target_height = 2448;
			break;

			case 3:		// 2592*1944
			pVid_info_front->sensor_width = RealWidth;
			pVid_info_front->sensor_height = RealHeight;
			pVid_info_front->target_width = 2592;
			pVid_info_front->target_height = 1944;
			break;

			case 4:		// 2048*1536
			pVid_info_front->sensor_width = RealWidth;
			pVid_info_front->sensor_height = RealHeight;
			pVid_info_front->target_width = 2048;
			pVid_info_front->target_height = 1536;
			break;

			case 5:		// 1920*1080
			default:
			pVid_info_front->sensor_width = sensor_width;
			pVid_info_front->sensor_height = sensor_height;
			pVid_info_front->target_width = 1920;
			pVid_info_front->target_height = 1080;
			break;

			case 6:		// 1280*960
			pVid_info_front->sensor_width = RealWidth;
			pVid_info_front->sensor_height = RealHeight;
			pVid_info_front->target_width = 1280;
			pVid_info_front->target_height = 960;
			break;

			case 7:		// 640*480
			pVid_info_front->sensor_width = RealWidth;
			pVid_info_front->sensor_height = RealHeight;
			pVid_info_front->target_width = 640;
			pVid_info_front->target_height = 480;
			break;
		}

		/*
		If need to set a resolution scale above 1x in STATE_PHOTO_CAPTURE
		Remove this section of code	
		*/
		if(pVid_info_front->data_source == VID_SRC_USB)
		{
			pVid_info_front->target_width = pVid_info_front->sensor_width;
			pVid_info_front->target_height = pVid_info_front->sensor_height;
		}

		pVid_info_front->clip_width	= pVid_info_front->sensor_width;
		pVid_info_front->clip_height = pVid_info_front->sensor_height;

	}
	else if(ap_mode == STATE_VIDEO_RECORD)
	{
		pVid_info_front->vid_rec_type = VID_REC_H264;

		pVid_info_front->vid_rec_line = VID_REC_LINE_FRONT;

        #if 0 // Debug
		video_resolution = R_RANDOM0 & 7;
		ap_state_config_video_resolution_set(video_resolution);
		#else
		video_resolution = ap_state_config_video_resolution_get();
        #endif

		if(pVid_info_front->data_source == VID_SRC_USB)
		{
			if(sensor_width == 1280)
			{
				video_resolution = 4; //720P 1280X720
			}
			else
			{
				video_resolution = 6; //VGA 640X480
			}
		}

		DBG_PRINT("==STATE_VIDEO_RECORD:%d==\r\n",video_resolution);

		switch(video_resolution)
		{
			case 0:		// 4K 3840X2160
				pVid_info_front->target_width = VID_REC_WIDTH_4K;
				pVid_info_front->target_height = VID_REC_HEIGHT_4K;
				pVid_info_front->sensor_width = VID_REC_WIDTH_1080FHD;//VID_REC_WIDTH_2K;//
				pVid_info_front->sensor_height = VID_REC_HEIGHT_1080FHD;//VID_REC_HEIGHT_2K;//
				pVid_info_front->clip_width	= sensor_width;
				pVid_info_front->clip_height = sensor_height;
				pVid_info_front->vid_rec_type = VID_REC_JPEG;
			break;

			case 1:		// 2K 2560X1440
				pVid_info_front->target_width = VID_REC_WIDTH_2K;
				pVid_info_front->target_height = VID_REC_HEIGHT_2K;
				if(sensor_width > 1920)
				{
                    pVid_info_front->sensor_width = VID_REC_WIDTH_2K;
                    pVid_info_front->sensor_height = VID_REC_HEIGHT_2K;
                    pVid_info_front->vid_rec_type = VID_REC_H264;
                }
                else
                {
					/*== INTERPOLATING 2K ==*/
					#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
                    pVid_info_front->sensor_width = sensor_width;
                    pVid_info_front->sensor_height = sensor_height;
                    pVid_info_front->vid_rec_type = VID_REC_H264;
					pVid_info_front->run_2K_flow_src = HOST_2K_IS_FRONT;
					#else
                    pVid_info_front->sensor_width = VID_REC_WIDTH_2K>>1;
                    pVid_info_front->sensor_height = VID_REC_HEIGHT_2K>>1;
                    pVid_info_front->vid_rec_type = VID_REC_JPEG;
                	#endif
                }
				pVid_info_front->clip_width	= sensor_width;
				pVid_info_front->clip_height = sensor_height;
			break;

			case 2:		//1080FHD 1920X1080
				pVid_info_front->target_width = VID_REC_WIDTH_1080FHD;
				pVid_info_front->target_height = VID_REC_HEIGHT_1080FHD;
				pVid_info_front->sensor_width = VID_REC_WIDTH_1080FHD;
				pVid_info_front->sensor_height = VID_REC_HEIGHT_1080FHD;
				pVid_info_front->clip_width	= sensor_width;
				pVid_info_front->clip_height = sensor_height;
			break;

			case 3:		//1080P 1440X1080
				pVid_info_front->target_width = VID_REC_WIDTH_1080P;
				pVid_info_front->target_height = VID_REC_HEIGHT_1080P;
				pVid_info_front->sensor_width = VID_REC_WIDTH_1080P;
				pVid_info_front->sensor_height = VID_REC_HEIGHT_1080P;
				pVid_info_front->clip_width	= RealWidth;
				pVid_info_front->clip_height = RealHeight;
			break;

			case 4:		//720P 1280X720
			default:
				pVid_info_front->target_width = VID_REC_WIDTH_720P;
				pVid_info_front->target_height = VID_REC_HEIGHT_720P;
				pVid_info_front->sensor_width = VID_REC_WIDTH_720P;
				pVid_info_front->sensor_height = VID_REC_HEIGHT_720P;
				pVid_info_front->clip_width	= sensor_width;
				pVid_info_front->clip_height = sensor_height;
			break;

			case 5:		//WVGA 800X480
				pVid_info_front->target_width = VID_REC_WIDTH_WVGA;
				pVid_info_front->target_height = VID_REC_HEIGHT_WVGA;
				pVid_info_front->sensor_width = VID_REC_WIDTH_WVGA;
				pVid_info_front->sensor_height = VID_REC_HEIGHT_WVGA;
				pVid_info_front->clip_width	= sensor_width;
				pVid_info_front->clip_height = sensor_height;
			break;

			case 6:		//VGA 640X480
				pVid_info_front->target_width = VID_REC_WIDTH_VGA;
				pVid_info_front->target_height = VID_REC_HEIGHT_VGA;
				pVid_info_front->sensor_width = VID_REC_WIDTH_VGA;
				pVid_info_front->sensor_height = VID_REC_HEIGHT_VGA;
				pVid_info_front->clip_width	= RealWidth;
				pVid_info_front->clip_height = RealHeight;
			break;

			case 7:		//QVGA 320X240
				pVid_info_front->target_width = VID_REC_WIDTH_QVGA;
				pVid_info_front->target_height = VID_REC_HEIGHT_QVGA;
				pVid_info_front->sensor_width = VID_REC_WIDTH_QVGA;
				pVid_info_front->sensor_height = VID_REC_HEIGHT_QVGA;
				pVid_info_front->clip_width	= RealWidth;
				pVid_info_front->clip_height = RealHeight;
			break;

            case 8:		// 5M 2560X1944
				pVid_info_front->target_width = VID_REC_WIDTH_5M;
				pVid_info_front->target_height = VID_REC_HEIGHT_5M;
				if(sensor_width > 1920)
				{
                    pVid_info_front->sensor_width = VID_REC_WIDTH_5M;
                    pVid_info_front->sensor_height = VID_REC_HEIGHT_5M;
                    pVid_info_front->vid_rec_type = VID_REC_H264;
                }
                else
                {
                    pVid_info_front->sensor_width = VID_REC_WIDTH_5M>>1;
                    pVid_info_front->sensor_height = VID_REC_HEIGHT_5M>>1;
                    pVid_info_front->vid_rec_type = VID_REC_JPEG;
                }
				pVid_info_front->clip_width	= sensor_width;
				pVid_info_front->clip_height = sensor_height;
			break;
		}
		if(pVid_info_front->sensor_width > pVid_info_front->clip_width)
		{
			pVid_info_front->sensor_width = pVid_info_front->clip_width;
		}
		if(pVid_info_front->sensor_height > pVid_info_front->clip_height)
		{
			pVid_info_front->sensor_height = pVid_info_front->clip_height;
		}
		
	}
	else if(ap_mode == STATE_CONNECT_TO_PC)
	{
		pVid_info_front->vid_rec_type = VID_REC_JPEG;
		pVid_info_front->target_width = UVC_TAR_WIDTH_MJPEG;
		pVid_info_front->target_height = UVC_TAR_HEIGHT_MJPEG;
		pVid_info_front->sensor_width = VID_REC_WIDTH_720P;
		pVid_info_front->sensor_height = VID_REC_HEIGHT_720P;
		pVid_info_front->clip_width	= sensor_width;
		pVid_info_front->clip_height = sensor_height;
	}
	else if(ap_mode == STATE_SENSOR_SAVE_RAW)
	{
        pVid_info_front->sensor_width = sensor_width;
        pVid_info_front->sensor_height = sensor_height;
        pVid_info_front->target_width = sensor_width;
        pVid_info_front->target_height = sensor_height;
        pVid_info_front->clip_width	= sensor_width;
		pVid_info_front->clip_height = sensor_height;
	}
	else
	{
		ERR_PRINT("wrong app_mode=%d\r\n", ap_mode);

		ap_mode = STATE_VIDEO_RECORD;
		video_encode_resolution_get(ap_mode);
		return STATUS_FAIL;
	}

	/*=== Front Setting ===*/
	#if C_SENSOR_SAVE_RAW_DATA
	pVid_info_front->run_ap_mode = STATE_SENSOR_SAVE_RAW;
	#else
	pVid_info_front->run_ap_mode = ap_mode;
	#endif
	pVid_info_front->frame_rate = VID_REC_FRAME_RATE_FRONT;
	pVid_info_front->data_format = VID_DATA_420;

	/*=== Rear Setting ===*/
	pVid_info_rear->run_ap_mode = ap_mode;
	pVid_info_rear->data_source = drv_l2_sensor_get_path(PREVIEW_SRC_REAR);
	pVid_info_rear->data_format = VID_DATA_420;

	/*== INTERPOLATING 2K ==*/
	#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
	if(pVid_info_front->run_2K_flow_src) // This value is Greater than 0, that means front do scale-up flow
	{
		if(pVid_info_rear->data_source == VID_SRC_USB)
		{
			pVid_info_rear->run_2K_flow_src = HOST_2K_IS_USB;
		}
		else if ((pVid_info_rear->data_source == VID_SRC_CSI0) || (pVid_info_rear->data_source == VID_SRC_CSI1))
		{
			pVid_info_rear->run_2K_flow_src = HOST_2K_IS_REAR;
			pVid_info_rear->data_format = drv_l2_user_csi_sensor_out_format_get(PREVIEW_SRC_REAR);
		}
	}
	#endif

	pVid_info_rear->frame_rate = VID_REC_FRAME_RATE_REAR;
	pVid_info_rear->sensor_width = drv_l2_user_sensor_width_get(PREVIEW_SRC_REAR);
	pVid_info_rear->sensor_height = drv_l2_user_sensor_height_get(PREVIEW_SRC_REAR);

	pVid_info_rear->target_width = pVid_info_rear->sensor_width;
	pVid_info_rear->target_height = pVid_info_rear->sensor_height;

	/*=== Display Setting ===*/
#if C_TFT_DISPLAY_FUNC
    #if C_WIFI_FUNC
	if(state_wifi_status_get() & FLAG_CONNECT)
	{
		pDisp_info->display_width = wifi_display_width_get();
		pDisp_info->display_height = wifi_display_height_get();
		pDisp_info->display_format = wifi_display_format_get();

		pDisp_info->pip_width = wifi_display_pip_width_get();
		pDisp_info->pip_height = wifi_display_pip_height_get();
	}
	else
	#endif
	#if C_USB_DEVICE
	if(state_usbd_status_get())
	{
		pDisp_info->display_width = usbd_uvc_display_width_get();
		pDisp_info->display_height = usbd_uvc_display_height_get();
        pDisp_info->display_format = uvc_display_format_get();//drv_l2_display_tft_fmt_get();

		pDisp_info->pip_width = drv_l2_display_pip_width_get();
		pDisp_info->pip_height = drv_l2_display_pip_height_get();
	}
	else
	#endif
	{
		pDisp_info->display_width = drv_l2_display_tft_width_get();
		pDisp_info->display_height = drv_l2_display_tft_height_get();
		pDisp_info->display_format = drv_l2_display_tft_fmt_get();

		#if (defined C_HDMI_SCALE_UP_EN) && (C_HDMI_SCALE_UP_EN)
		pDisp_info->display_width = min(pDisp_info->display_width,pVid_info_front->sensor_width);
		pDisp_info->display_height = min(pDisp_info->display_width,pVid_info_front->sensor_height);
		
		if(pVid_info_rear->data_source != VID_SRC_NONE)
		{
			pDisp_info->display_width = min(pDisp_info->display_width,pVid_info_rear->sensor_width);
			pDisp_info->display_height = min(pDisp_info->display_height,pVid_info_rear->sensor_height);
		}

		//DBG_PRINT("Front(%d,%d)\r\n",pVid_info_front->sensor_width,pVid_info_front->sensor_height);
		//DBG_PRINT("Rear(%d,%d)\r\n",pVid_info_rear->sensor_width,pVid_info_rear->sensor_height);
		//DBG_PRINT("Disp(%d,%d)\r\n",pDisp_info->display_width,pDisp_info->display_height);

		drv_l2_display_tft_width_reset(pDisp_info->display_width);
		drv_l2_display_tft_height_reset(pDisp_info->display_height);		
		#endif
		
		pDisp_info->pip_width = drv_l2_display_pip_width_get();
		pDisp_info->pip_height = drv_l2_display_pip_height_get();
		pDisp_info->pip_pos_x = drv_l2_display_pip_x_get();
		pDisp_info->pip_pos_y = drv_l2_display_pip_y_get();
	}

#else
	#if C_USB_DEVICE
	if(state_usbd_status_get())
	{
		pDisp_info->display_width = usbd_uvc_display_width_get();
		pDisp_info->display_height = usbd_uvc_display_height_get();
		pDisp_info->display_format = wifi_display_format_get();

		pDisp_info->pip_width = wifi_display_pip_width_get();
		pDisp_info->pip_height = wifi_display_pip_height_get();
	}
	else
	#endif
	{
		pDisp_info->display_width = wifi_display_width_get();
		pDisp_info->display_height = wifi_display_height_get();
		pDisp_info->display_format = wifi_display_format_get();

		pDisp_info->pip_width = wifi_display_pip_width_get();
		pDisp_info->pip_height = wifi_display_pip_height_get();
	}
#endif

	DBG_PRINT("==display:[w:%d,h:%d,f:%d]==\r\n",pDisp_info->display_width, pDisp_info->display_height, pDisp_info->display_format);
	DBG_PRINT("==pip:[w:%d,h:%d]==\r\n",pDisp_info->pip_width, pDisp_info->pip_height);

	/*=== Y only Setting ===*/
	#if (defined C_Y_ONLY_FUNCTION) && (C_Y_ONLY_FUNCTION)
	if(pY_only_info->data_source == Y_ONLY_SRC_DRAM){
		pY_only_info->data_format = Y_ONLY_DATA_420;
		pY_only_info->sensor_width = 1280;
		pY_only_info->sensor_height= 720;
		pY_only_info->in_fifo_line = PSCALER_IN_FIFO_FRAME_MODE;
	}
	else {
		if(pVid_info_front->data_source == VID_SRC_CDSP || pVid_info_front->data_source == VID_SRC_CSI0)
		{
			pY_only_info->data_source = (pVid_info_front->data_source == VID_SRC_CDSP)?Y_ONLY_SRC_CDSP:Y_ONLY_SRC_CSI0;
			pY_only_info->sensor_width = pVid_info_front->sensor_width;
			pY_only_info->sensor_height= pVid_info_front->sensor_height;
			pY_only_info->in_fifo_line = PSCALER_IN_FIFO_FRAME_MODE;
		}
		else if(pVid_info_rear->data_source == VID_SRC_CDSP || pVid_info_rear->data_source == VID_SRC_CSI0)
		{
			pY_only_info->data_source = (pVid_info_rear->data_source == VID_SRC_CDSP)?Y_ONLY_SRC_CDSP:Y_ONLY_SRC_CSI0;
			pY_only_info->sensor_width = pVid_info_rear->sensor_width;
			pY_only_info->sensor_height= pVid_info_rear->sensor_height;
			pY_only_info->in_fifo_line = PSCALER_IN_FIFO_FRAME_MODE;
		}
		else
		{
			pY_only_info->data_source = Y_ONLY_SRC_NONE;
			ERR_PRINT("Y only data source select fail\r\n");
		}
	}

	#if Y_ONLY_CLIP_EN == 1
	pY_only_info->clip_width = (Y_ONLY_CLIP_WIDTH > pY_only_info->sensor_width)?pY_only_info->sensor_width:Y_ONLY_CLIP_WIDTH;
	pY_only_info->clip_height = (Y_ONLY_CLIP_HEIGHT > pY_only_info->sensor_height)?pY_only_info->sensor_height:Y_ONLY_CLIP_HEIGHT;
    #else
	pY_only_info->clip_width = pY_only_info->sensor_width;
	pY_only_info->clip_height = pY_only_info->sensor_height;
    #endif

	pY_only_info->clip_x_start = (pY_only_info->sensor_width - pY_only_info->clip_width) >> 1;
	pY_only_info->clip_y_start = (pY_only_info->sensor_height - pY_only_info->clip_height) >> 1;

	if(pY_only_info->clip_x_start + pY_only_info->clip_width > pY_only_info->sensor_width)
	{
		pY_only_info->clip_x_start = (pY_only_info->sensor_width - pY_only_info->clip_width) >> 1;
	}
	if(pY_only_info->clip_y_start + pY_only_info->clip_height > pY_only_info->sensor_height)
	{
		pY_only_info->clip_y_start = (pY_only_info->sensor_height - pY_only_info->clip_height) >> 1;
	}
	pY_only_info->target_width = Y_ONLY_TARGET_WIDTH;
	pY_only_info->target_height = Y_ONLY_TARGET_HEIGHT;

	//Pscaler not support scale up
	if(pY_only_info->clip_width * pY_only_info->target_height >= pY_only_info->clip_height * pY_only_info->target_width)
	{
		if(pY_only_info->target_width > pY_only_info->clip_width)
		{
			pY_only_info->target_height = pY_only_info->target_width = pY_only_info->clip_width;
		}
	}
	else
	{
		if(pY_only_info->target_height > pY_only_info->clip_height)
		{
			pY_only_info->target_width = pY_only_info->target_height = pY_only_info->clip_height;
		}
	}
	DBG_PRINT("==Y only:[clip_w:%d,clip_h:%d,target_w:%d,target_h:%d]==\r\n"
		, pY_only_info->clip_width
		, pY_only_info->clip_height
		, pY_only_info->target_width
		, pY_only_info->target_height
	);
	#endif

	if(ap_mode == STATE_PHOTO_CAPTURE)
	{
		pVid_info_rear->vid_rec_type = VID_REC_JPEG;

		pDisp_info->display_h_offset = drv_l2_display_off_lines_cap_get();
		pDisp_info->en_crop = drv_l2_display_en_crop_cap_get();
		pDisp_info->pbp_crop = drv_l2_display_pbp_crop_cap_get();
		pDisp_info->back_ratio = drv_l2_display_back_ratio_cap_get();
	}
	else if(ap_mode == STATE_VIDEO_RECORD)
	{
		pVid_info_rear->vid_rec_type = VID_REC_H264;
		pVid_info_rear->vid_rec_line = VID_REC_LINE_REAR;

		pDisp_info->display_h_offset = drv_l2_display_off_lines_video_get();
		pDisp_info->en_crop = drv_l2_display_en_crop_video_get();        //enable crop
		pDisp_info->pbp_crop = drv_l2_display_pbp_crop_video_get();
		pDisp_info->back_ratio = drv_l2_display_back_ratio_video_get();
	}
	if(state_wifi_status_get() & FLAG_CONNECT)
	{
        pDisp_info->display_h_offset = 0;
	}

	return STATUS_OK;
}

void
video_encode_entrance(
	void
)
{
	INT32S nRet;

	nRet = video_encode_state_task_create();
	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("==fail to vid_encode_state_task_create\r\n");
		return;
	}

	nRet = audio_encode_state_task_create();
	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("==fail to audio_encode_task_create\r\n");
		return;
	}

}

INT32S
video_encode_sensor_on(
	INT32U ap_mode
)
{
	INT32S nRet;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	video_encode_resolution_get(ap_mode);

	nRet = video_encode_state_sensor_start();

	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("==fail to video_encode_sensor_on\r\n");
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

INT32S
video_encode_sensor_off(
	void
)
{
	INT32S nRet;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);
	nRet = video_encode_state_sensor_stop();
	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("==fail to video_encode_sensor_off\r\n");
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

INT32S
video_encode_preview_mode_set(
	INT32U mode
)
{
	INT32S nRet;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = video_encode_state_preview_mode_set(mode);

	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("==fail to video_encode_preview_mode_set\r\n");
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

INT32S
video_encode_preview_off(
	void
)
{
	INT32S nRet;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = video_encode_state_preview_mode_set(PREVIEW_MODE_NONE);

	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("==fail to video_encode_preview_off\r\n");
		return STATUS_FAIL;
	}

	return STATUS_OK;
}


INT32S
video_encode_record_start(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	video_encode_state_encode_start();

	audio_encode_state_encode_start();

	return STATUS_OK;
}

INT32S
video_encode_record_stop(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	audio_encode_state_encode_stop();

	video_encode_state_encode_stop();

	return STATUS_OK;
}

INT32S
video_encode_capture_picture(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	video_encode_state_photo_start();

	return STATUS_OK;
}

#if C_SUPORT_GET_JPG_BUF
INT32S
video_encode_get_jpeg(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	return video_encode_state_jpeg_on_enc_get();
}

INT32S
video_encode_release_get_jpeg_buf(
	INT8U src
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	video_encode_state_release_get_jpeg_buf(src);

	return STATUS_OK;
}
#endif


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
#include "vid_mp4_packer.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define VID_ENC_MSG_NOTIFY_CNT					64

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
DISP_QUEUE sensor_preview_queue;

#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
static INT8U flag_uvc_connect = 0;
#endif


#if C_TIME_LAPSE
INT32U  g_time_lapse = 0;
INT8U time_lapse_fps = 25;
INT32U time_lapse_current_time = 0, time_lapse_last_os_time = 0;
INT32S time_lapse_delta_time = 0;
INT32U time_lapse_current_time_rear = 0, time_lapse_last_os_time_rear = 0;
INT32S time_lapse_delta_time_rear = 0;

#endif

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/
extern INT32S rear_packer_start(MSG_Q_ID notifyQ, VID_INFO* args);
extern INT32S front_packer_stop(void);
extern INT32S rear_packer_stop(void);
extern INT32S lib_enc_dummywrite(INT16U src);
#if C_AEAWB_INFO_STAMP
extern ae_info_t ae_set_ifo;
#endif

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
void uvc_connect_status_set(INT8U flag_cnt);
INT8U uvc_connect_status_get(void);
#endif

#if C_TIME_LAPSE
void ap_time_lapse_tick_clean(void);
#endif

/*== HOST USB MJPEG CAM / INTERP2K ==*/
#if (_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K)
INT32S host_2K_preview_buffer_set(INT32U src,VID_INFO* args);
#endif

#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
extern void uvc_record_stop_req_clear(INT8U src);
extern void wifi_encode_stop_req_clear(void);
#endif


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static INT32U
preview_buf_malloc(
	void
)
{
	DISP_P frame = NULL;
	#if (defined C_HDMI_SCALE_UP_EN) && (C_HDMI_SCALE_UP_EN)
	DISP_INFO*  pDisp_info = &pStreamArgs->disp_info;
	#endif
	
	frame = ap_display_queue_get(display_free_queue);
	if(!frame)
	{
		return 0;
	}

	#if (defined C_HDMI_SCALE_UP_EN) && (C_HDMI_SCALE_UP_EN)
	frame->scaleup.src_w = pDisp_info->display_width;
	frame->scaleup.src_h = pDisp_info->display_height;
	#endif

	if(ap_display_queue_put(sensor_preview_queue, frame) != STATUS_OK)
	{
		DBG_PRINT("[ERROR]sensor_preview_queue full\r\n");
	}
	return frame->base;
}

void
preview_buf_free(
	INT32U buf
)
{
	DISP_P frame = NULL;

	frame = ap_display_queue_get(sensor_preview_queue);

	if(!frame)
	{
		DBG_PRINT("[WARN] sensor_preview_queue empty\r\n");
		return;
	}

	if(frame->base != buf)
	{
		DBG_PRINT("[TRACE] %s get 0x%X != 0x%X\r\n", __func__, buf, frame->base);
	}

	if(ap_display_queue_put(display_free_queue, frame) != STATUS_OK)
	{
		DBG_PRINT("[ERROR]display_free_queue full\r\n");
	}
}

static INT32S
preview_buf_show_path_check(
	DISP_P frame
)
{
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
    if((state_wifi_status_get() & FLAG_CONNECT) &&
		(state_wifi_status_get() & FLAG_OFF_DISPLAY)
	)
	{
        extern int rtsp_connection_get(void);

		if(rtsp_connection_get())
		{
			wifi_encode_buf_notify((INT32)frame);
		}
		else
		{
			if(ap_display_queue_put(display_free_queue, frame) != STATUS_OK)
			{
				DBG_PRINT("[ERROR]display_free_queue full\r\n");
			}
		}

		return STATUS_OK;	// Skip Display
	}
#endif

#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
	if(uvc_connect_status_get())
	{
		uvc_encode_buf_notify((INT32)frame);

		return STATUS_OK;	// Skip Display
	}
#endif

	return STATUS_FAIL;	// Run Display
}

static void
preview_buf_show(
	INT32U buf,
	INT32U offset,
	INT8U car_back,
	INT16U top,
	INT16U bot,
	INT16U left,
	INT16U right
)
{
    #if C_TFT_DISPLAY_FUNC
	INT32S nRet;
	#endif

	DISP_P frame = NULL;

	frame = ap_display_queue_get(sensor_preview_queue);

	if(!frame)
	{
		DBG_PRINT("[WARN] sensor_preview_queue empty\r\n");
		return;
	}

	if(frame->base != buf)
	{
		DBG_PRINT("[TRACE] %s get 0x%X != 0x%X\r\n", __func__, buf, frame->base);
	}

	frame->offset = offset;
    frame->padding.top = top;
    frame->padding.bottom = bot;
    frame->padding.left = left;
    frame->padding.right = right;

	if(preview_buf_show_path_check(frame) == STATUS_FAIL)
	{
        #if C_TFT_DISPLAY_FUNC
        if(car_back)
		{
            nRet = display_task_msg_send(MSG_DISPLAY_TASK_BACK_A_CAR_DRAW, &frame, sizeof(INT32U), MSG_PRI_NORMAL);
		}
		else
		{
            nRet = display_task_msg_send(MSG_DISPLAY_TASK_SENSOR_DRAW, &frame, sizeof(INT32U), MSG_PRI_NORMAL);
		}

		if(nRet == STATUS_FAIL)
		{
	        if(ap_display_queue_put(display_free_queue, frame) != STATUS_OK)
	        {
	            DBG_PRINT("[ERROR]display_free_queue full\r\n");
	        }
			else
			{
				DBG_PRINT("=>display_free_queue ok\r\n");
			}
		}

        #else
        if(ap_display_queue_put(display_free_queue, frame) != STATUS_OK)
        {
            DBG_PRINT("[ERROR]display_free_queue full\r\n");
        }
        #endif
	}
}

void
preview_buf_queue_clear(
	void
)
{
	INT32U i;

	osENTER_CRITICAL();

	for(i = 0; i < DISPLAY_BUF_NUM; ++i)
	{
		sensor_preview_queue[i] = 0;
	}

	osEXIT_CRITICAL();
}

INT32S
cb_func_sensor_start(
	void
)
{
	VID_INFO*  pVid_info_select;
	VID_INFO*  pVid_info_front = &pStreamArgs->vid_info_front;
	VID_INFO*  pVid_info_rear = &pStreamArgs->vid_info_rear;
	#if defined(C_Y_ONLY_FUNCTION) && C_Y_ONLY_FUNCTION == CUSTOM_ON
	Y_ONLY_INFO* pY_only_info = &pStreamArgs->y_only_inf;
	#endif
	DISP_INFO*  pDisp_info = &pStreamArgs->disp_info;
	sensor_frame_range_t sensorFrameRange = { 0 };
	INT16U sensor_out_w		= drv_l2_user_sensor_width_get(PREVIEW_SRC_FRONT);
	INT16U sensor_out_h		= drv_l2_user_sensor_height_get(PREVIEW_SRC_FRONT);
	INT16U clip_width		= pVid_info_front->clip_width;
	INT16U clip_height		= pVid_info_front->clip_height;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pVid_info_front->data_source == VID_SRC_CDSP)
	{
        /*=== Sensor Clip ===*/
        if((pVid_info_front->sensor_width == sensor_out_w) && (pVid_info_front->sensor_height == sensor_out_h))
        {
            sensorFrameRange.point_x = 0;
            sensorFrameRange.point_y = 0;
            sensorFrameRange.clip_w = clip_width;
            sensorFrameRange.clip_h = clip_height;
            sensorFrameRange.scaledown_w = clip_width;
            sensorFrameRange.scaledown_h = clip_height;
            sensorFrameRange.frame_w = sensor_out_w;
            sensorFrameRange.frame_h = sensor_out_h;
            sensorFrameRange.docropflag = 0;
            sensorFrameRange.doscaledownflag = 0;
        }
        else if(((pVid_info_front->sensor_width == 1920) && (pVid_info_front->sensor_height == 1440) &&(sensor_out_h == 1440))||
            ((pVid_info_front->sensor_width == 1440) && (pVid_info_front->sensor_height == 1080) &&(sensor_out_h == 1080))||
            ((pVid_info_front->sensor_width == 960) && (pVid_info_front->sensor_height == 720) &&(sensor_out_h == 720)))
        {
            if(sensor_out_h == 1080)
            clip_width =(((clip_width >> 6)+1) << 6) ;		//set 64Alignment for MD
            sensorFrameRange.point_x = (sensor_out_w - clip_width) >> 1;
            sensorFrameRange.point_y = (sensor_out_h - clip_height) >> 1;
            sensorFrameRange.clip_w = clip_width;
            sensorFrameRange.clip_h = clip_height;
            sensorFrameRange.scaledown_w = clip_width;
            sensorFrameRange.scaledown_h = clip_height;
            sensorFrameRange.frame_w = sensor_out_w;
            sensorFrameRange.frame_h = sensor_out_h;
            sensorFrameRange.docropflag = 3;
            sensorFrameRange.doscaledownflag = 0;
        }
        else if(((pVid_info_front->sensor_width == 1920) && (pVid_info_front->sensor_height == 1080)) ||
            ((pVid_info_front->sensor_width == 1280) && (pVid_info_front->sensor_height == 720))||
            ((pVid_info_front->sensor_width == 800) && (pVid_info_front->sensor_height == 480)))
        {
            sensorFrameRange.point_x = 0;
            sensorFrameRange.point_y = 0;
            sensorFrameRange.clip_w = sensor_out_w;
            sensorFrameRange.clip_h = sensor_out_h;
            sensorFrameRange.scaledown_w = pVid_info_front->sensor_width;
            sensorFrameRange.scaledown_h = pVid_info_front->sensor_height;
            sensorFrameRange.frame_w = sensor_out_w;
            sensorFrameRange.frame_h = sensor_out_h;
            sensorFrameRange.docropflag = 0;
            sensorFrameRange.doscaledownflag = 1;
        }
		else if((pVid_info_front->sensor_width >= 2560) && (pVid_info_front->sensor_height >= 1440))
		{
	 		clip_width =(((clip_width >> 6)+1) << 6) ;		//set 64Alignment for MD
	 		sensorFrameRange.point_x = (sensor_out_w - clip_width) >> 1;
			sensorFrameRange.point_y = (sensor_out_h - clip_height) >> 1;
			sensorFrameRange.clip_w = clip_width;
			sensorFrameRange.clip_h = clip_height;
			sensorFrameRange.scaledown_w = pVid_info_front->sensor_width;
			sensorFrameRange.scaledown_h = pVid_info_front->sensor_height;
			sensorFrameRange.frame_w = sensor_out_w;
			sensorFrameRange.frame_h = sensor_out_h;
			sensorFrameRange.docropflag = 0;
			sensorFrameRange.doscaledownflag = 1;
		}
        else
        {
            clip_width =(((clip_width >> 6)+1) << 6) ;		//set 64Alignment for MD
            sensorFrameRange.point_x = (sensor_out_w - clip_width) >> 1;
            sensorFrameRange.point_y = (sensor_out_h - clip_height) >> 1;
            sensorFrameRange.clip_w = clip_width;
            sensorFrameRange.clip_h = clip_height;
            sensorFrameRange.scaledown_w = pVid_info_front->sensor_width;
            sensorFrameRange.scaledown_h = pVid_info_front->sensor_height;
            sensorFrameRange.frame_w = sensor_out_w;
            sensorFrameRange.frame_h = sensor_out_h;
            sensorFrameRange.docropflag = 1;
            sensorFrameRange.doscaledownflag = 1;
        }
        sensorFrameRange.frameAddrs = 0x70000000;

        DBG_PRINT("\r\n[V]x:%d,y:%d, Cw:%d,Ch:%d, Sw:%d,Sh%d, Fw:%d,Fh:%d, Cf:%d,Sf:%d\r\n"
            , sensorFrameRange.point_x
            , sensorFrameRange.point_y
            , sensorFrameRange.clip_w
            , sensorFrameRange.clip_h
            , sensorFrameRange.scaledown_w
            , sensorFrameRange.scaledown_h
            , sensorFrameRange.frame_w
            , sensorFrameRange.frame_h
            , sensorFrameRange.docropflag
            , sensorFrameRange.doscaledownflag
        );

        if (drv_l2_sensor_frame_range_clip(&sensorFrameRange)<0){
            DBG_PRINT("=>[Err]ispClip & ispScaler FAIL!! \r\n");
            return STATUS_FAIL;
        }else{
            DBG_PRINT("=>ispClip & ispScaler Done \r\n");
        }
    }
	/*=== Display ===*/
	INT32U display_width = pDisp_info->display_width;
	INT32U display_height = pDisp_info->display_height;
	INT32U display_format = pDisp_info->display_format;
	INT16U pip_width = pDisp_info->pip_width;
	INT16U pip_height = pDisp_info->pip_height;
	INT16U pip_pos_x = pDisp_info->pip_pos_x;
	INT16U pip_pos_y = pDisp_info->pip_pos_y;
	INT8U  en_crop = pDisp_info->en_crop;
	INT8U  pbp_crop = pDisp_info->pbp_crop;
	INT8U  back_ratio = pDisp_info->back_ratio;
	INT32U display_h_offset = pDisp_info->display_h_offset;
	INT16U zoom_width;
	INT16U zoom_height;
	INT16U zoom_w_start;
	INT16U zoom_h_start;
	INT16U out_w_start;
	INT16U out_h_start;
	INT16U cropW, cropH;
	INT16U out_off_x, out_off_y;

	INT16U sensor_width;
	INT16U sensor_height;
	INT16U target_width;

	INT32U data_source;
	INT8U  i;
#if C_DISP_SLIDE_SAVE
	INT8U  crop_flag;
#endif
	sensor_preview_need_display_set(0);

	if(display_width && display_height) // The product has a screen
	{
		if(preview_q_malloc(DISPLAY_BUF_NUM + 1) == STATUS_FAIL) //reserve 1 more space for q full judging
		{
			ERR_PRINT("preview_q_malloc(%d) fail\r\n", DISPLAY_BUF_NUM);
		}

		sensor_preview_need_display_set(1);
	}

	display_h_offset &= ~0x0001;
	display_height -= display_h_offset*2;
	sensor_preview_display_pixel_set(display_width, display_height);
	sensor_preview_pip_pixel_set(pip_width, pip_height, pip_pos_x, pip_pos_y);
	sensor_preview_pbp_crop_back_ratio_set(pbp_crop, back_ratio);

	if(display_format == DISP_FMT_RGB565)
	{
		sensor_preview_display_format_set(PIPELINE_SCALER_OUTPUT_FORMAT_RGB565);
		sensor_preview_display_offset_set(display_width * display_h_offset << 1);
	}
	else if(display_format == DISP_FMT_GP420)
	{
		sensor_preview_display_format_set(PIPELINE_SCALER_OUTPUT_FORMAT_GP420);
		sensor_preview_display_offset_set(display_width * display_h_offset * 3 >> 1);
	}
	else
	{
		sensor_preview_display_format_set(PIPELINE_SCALER_OUTPUT_FORMAT_YUYV);
		sensor_preview_display_offset_set(display_width * display_h_offset << 1);
	}

#if C_DISP_SLIDE_SAVE

    extern void sensor_preview_out_crop_flag_set(INT8U crop_flag);
    extern void sensor_preview_out_crop_reversing_flag_set(INT8U crop_flag);

	if(((ap_state_config_crop_slide_width_get() != pVid_info_front->sensor_width)
		||(ap_state_config_crop_slide_height_get() != pVid_info_front->sensor_height))
        #if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
		&&(state_wifi_status_get() == FLAG_DISCONNECT)
        #endif
		)
	{
		DBG_PRINT("[AI]display size changed w=%d,h=%d\r\n",pVid_info_front->sensor_width,pVid_info_front->sensor_height);
		ap_state_config_crop_slide_width_set(pVid_info_front->sensor_width);
		ap_state_config_crop_slide_height_set(pVid_info_front->sensor_height);
		crop_flag = 1;
		ap_state_config_crop_slide_load(0);
    }
    #if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	else if(state_wifi_status_get() & FLAG_CONNECT)//if(pVid_info_front->run_ap_mode == STATE_WIFI_TRANSMISSION)
	{
		DBG_PRINT("[AI]wifi mode display size changed w=%d,h=%d\r\n",pVid_info_front->sensor_width,pVid_info_front->sensor_height);
		crop_flag = 1;
	}
    #endif
	else
	{
		crop_flag = 0;
		ap_state_config_crop_slide_load(1);
	}
#endif

	for(i = PREVIEW_SRC_FRONT; i <= PREVIEW_SRC_REAR; i++)
	{
		zoom_width = display_width;
		zoom_height = display_height;
		zoom_w_start = 0;
		zoom_h_start = 0;
		out_w_start = 0;
		out_h_start = 0;
		out_off_x = 0;
		out_off_y = 0;

		if(i == PREVIEW_SRC_FRONT)
		{
			pVid_info_select = pVid_info_front;
		}
		else if(i == PREVIEW_SRC_REAR)
		{
			pVid_info_select = pVid_info_rear;
		}
		sensor_width = pVid_info_select->sensor_width;
		sensor_height = pVid_info_select->sensor_height;

		target_width = pVid_info_select->target_width;
		sensor_preview_src_pixel_set(i, sensor_width, sensor_height);

		if((target_width == VID_REC_WIDTH_4K) && (pVid_info_select->vid_rec_type == VID_REC_JPEG))
		{
			sensor_preview_src_drop_half(i, 1); //half
			DBG_PRINT("Pscaler Drop Half\r\n");
		}
		else
		{
			sensor_preview_src_drop_half(i, 0);
		}

		cropW = sensor_width * display_height / sensor_height;
		cropH = sensor_height * display_width / sensor_width;

		if(en_crop && (cropH > display_height))
		{
			if(en_crop == 1)
			{
				if(sensor_width<display_width)
				{
					zoom_height = (sensor_height >> 1) << 1;
				}
				else
				{
					zoom_height = (cropH >> 1) << 1;
				}
				out_h_start = (zoom_height - display_height) >> 1;
			}
			else //en_crop == 2
			{
				zoom_width = (cropW >> 4) << 4;
				out_off_x = (display_width - zoom_width) >> 1;
				if(out_off_x & 0x0001) out_off_x += 0x0001;
			}

            #if C_DISP_SLIDE_SAVE
			if(!crop_flag)
            {
                do
                {
                    /*------------------------------------------------------------------------------------------*/
                    /*   check if slide parameters retrieved from spi are legal, or reset them to normal value  */
                    /*------------------------------------------------------------------------------------------*/

                    INT8U   en_crop_get;
                    INT16U  off_x_get, off_y_get, out_w_st_get, out_h_st_get, out_w_st_next_get, out_h_st_next_get;

                    sensor_preview_out_crop_get(i, &en_crop_get, &off_x_get, &off_y_get, &out_w_st_get, &out_h_st_get, &out_w_st_next_get, &out_h_st_next_get);
                    if(en_crop_get != en_crop)
                    {
                        crop_flag = 1;      //reset sliding parameters
                        break;
                    }

                    if(en_crop == 1)
                    {
                        if(((out_h_st_get + display_height) > zoom_height) || (out_w_st_get != 0) || (off_x_get != 0) || (off_y_get != 0))
                        {
                           crop_flag = 1;   //reset sliding parameters
                        }
                    }
                    else if(en_crop == 2)
                    {
                        if(((off_x_get + zoom_width) > display_width) || (off_y_get != 0) || (out_w_st_get != 0) || (out_h_st_get != 0))
                        {
                           crop_flag = 1;   //reset sliding parameters
                        }
                    }
                } while(0);

                if(crop_flag)
                {
                   ERR_PRINT("illegal slide parameters retrieved\r\n");
                }
            }

			if(crop_flag)
			{
				sensor_preview_out_crop_set(i, en_crop, out_w_start, out_h_start, out_off_x, out_off_y);
				sensor_preview_out_crop_flag_set(1);
                sensor_preview_out_crop_reversing_flag_set(1);
			}

            #else
			sensor_preview_out_crop_set(i, en_crop, out_w_start, out_h_start, out_off_x, out_off_y);
            #endif
		}
		else if(en_crop && (cropW > display_width))
		{
			if(en_crop == 1)
			{
				zoom_width = (cropW >> 4) << 4;
				out_w_start = (zoom_width - display_width) >> 1;
			}
			else //en_crop == 2
			{
				zoom_height = (cropH >> 1) << 1;
				out_off_y = (display_height - zoom_height) >> 1;
				if(out_off_y & 0x0001) out_off_y += 0x0001;
			}

            #if C_DISP_SLIDE_SAVE

			if(!crop_flag)
            {
                do
                {
                    /*------------------------------------------------------------------------------------------*/
                    /*   check if slide parameters retrieved from spi are legal, or reset them to normal value  */
                    /*------------------------------------------------------------------------------------------*/

                    INT8U   en_crop_get;
                    INT16U  off_x_get, off_y_get, out_w_st_get, out_h_st_get, out_w_st_next_get, out_h_st_next_get;

                    sensor_preview_out_crop_get(i, &en_crop_get, &off_x_get, &off_y_get, &out_w_st_get, &out_h_st_get, &out_w_st_next_get, &out_h_st_next_get);
                    if(en_crop_get != en_crop)
                    {
                        crop_flag = 1;      //reset sliding parameters
                        break;
                    }

                    if(en_crop == 1)
                    {
                        if(((out_w_st_get + display_width) > zoom_width) || (out_h_st_get != 0) || (off_x_get != 0) || (off_y_get != 0))
                        {
                           crop_flag = 1;   //reset sliding parameters
                        }
                    }
                    else if(en_crop == 2)
                    {
                        if(((off_y_get + zoom_height) > display_height) || (off_x_get != 0) || (out_w_st_get != 0) || (out_h_st_get != 0) )
                        {
                           crop_flag = 1;   //reset sliding parameters
                        }
                    }
                } while(0);

                if(crop_flag)
                {
                   ERR_PRINT("illegal slide parameters retrieved\r\n");
                }
            }

			if(crop_flag)
			{
				sensor_preview_out_crop_set(i, en_crop, out_w_start, out_h_start, out_off_x, out_off_y);
				sensor_preview_out_crop_flag_set(1);
                sensor_preview_out_crop_reversing_flag_set(1);
			}
            #else
			sensor_preview_out_crop_set(i, en_crop, out_w_start, out_h_start, out_off_x, out_off_y);
            #endif
		}
		else
		{
            #if C_DISP_SLIDE_SAVE
			if(!crop_flag)
            {
                do
                {
                    /*------------------------------------------------------------------------------------------*/
                    /*   check if slide parameters retrieved from spi are legal, or reset them to normal value  */
                    /*------------------------------------------------------------------------------------------*/

                    INT8U   en_crop_get;
                    INT16U  off_x_get, off_y_get, out_w_st_get, out_h_st_get, out_w_st_next_get, out_h_st_next_get;

                    sensor_preview_out_crop_get(i, &en_crop_get, &off_x_get, &off_y_get, &out_w_st_get, &out_h_st_get, &out_w_st_next_get, &out_h_st_next_get);

                    if((out_w_st_get != 0) || (out_h_st_get != 0) || (off_x_get != 0) || (off_y_get != 0) || (en_crop_get != 0))
                    {
                       crop_flag = 1;   //reset sliding parameters
                    }
                } while(0);

                if(crop_flag)
                {
                   ERR_PRINT("illegal slide parameters retrieved\r\n");
                }
            }

			if(crop_flag)
			{
				sensor_preview_out_crop_set(i, 0, out_w_start, out_h_start, out_off_x, out_off_y);  //don't need cropping
				sensor_preview_out_crop_flag_set(1);
                sensor_preview_out_crop_reversing_flag_set(1);
			}
            #else
            sensor_preview_out_crop_set(i, 0, out_w_start, out_h_start, out_off_x, out_off_y);  //don't need cropping
            #endif
		}

        #if C_DISP_SLIDE_SAVE
		if(crop_flag)
		{
            #if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
			if(state_wifi_status_get() & FLAG_CONNECT)//(pVid_info_front->run_ap_mode != STATE_WIFI_TRANSMISSION)
            #endif
			{
				ap_state_config_crop_slide_get(PREVIEW_SRC_TOTAL);
			}
		}
        #endif

		sensor_preview_zoom_pixel_set(i, zoom_width, zoom_height, zoom_w_start, zoom_h_start);
		sensor_record_line_set(i, pVid_info_select->vid_rec_line);

		data_source = pVid_info_select->data_source;
		if(data_source == VID_SRC_CDSP)
		{
			sensor_preview_src_path_set(i, PREVIEW_IN_PATH_CDSP);
			sensor_preview_src_format_set(i, PIPELINE_SCALER_INPUT_FORMAT_YUYV);
		}
		else if((data_source == VID_SRC_CSI0) || (data_source == VID_SRC_CSI1))
		{
			if(data_source == VID_SRC_CSI0)
			{
				sensor_preview_src_path_set(i, PREVIEW_IN_PATH_CSI0);
				#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
				drv_l1_register_csi_cbk(CSI_0,NULL);
				#endif
			}
			else
			{
				sensor_preview_src_path_set(i, PREVIEW_IN_PATH_CSI1);
				#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
				drv_l1_register_csi_cbk(CSI_1,NULL);
				#endif
			}
			sensor_preview_src_format_set(i, PIPELINE_SCALER_INPUT_FORMAT_YUYV);
		}
		else if(data_source == VID_SRC_USB)
		{
			sensor_preview_src_path_set(i, PREVIEW_IN_PATH_USB);
			sensor_preview_src_format_set(i, PIPELINE_SCALER_INPUT_FORMAT_GP420);
		}
		else
		{
			sensor_preview_src_path_set(i, PREVIEW_IN_PATH_NONE);
		}
	}

	/*== HOST USB MJPEG CAM / INTERP2K ==*/
	#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
	if(pVid_info_front->run_2K_flow_src) // This value is Greater than 0, that means front do scale-up flow
	{	// Change pscaler_A bypass output to frame mode
		sensor_record_line_set(PREVIEW_SRC_FRONT, 0);

		if(pVid_info_rear->run_2K_flow_src)
		{
			if(pVid_info_rear->data_format == VID_DATA_420)
			{
				sensor_preview_src_format_set(PREVIEW_SRC_REAR, PIPELINE_SCALER_INPUT_FORMAT_GP420);
			}
			else
			{
				sensor_preview_src_format_set(PREVIEW_SRC_REAR, PIPELINE_SCALER_INPUT_FORMAT_YUYV);
			}
			// Allocate memory to store data for HOST_USB_MJPEG_CAM/INTERP2K
			host_2K_preview_buffer_set(PREVIEW_SRC_REAR,pVid_info_rear);
		}
	}
	else
	#endif
	{
        #if (_SENSOR_USB_MJPEG_CAM)
		if(pVid_info_front->data_source == VID_SRC_USB)
		{
			sensor_preview_src_format_set(PREVIEW_SRC_FRONT, PIPELINE_SCALER_INPUT_FORMAT_GP420);
			// Allocate memory to store data for HOST_USB_MJPEG_CAM/INTERP2K
			host_2K_preview_buffer_set(PREVIEW_SRC_FRONT,pVid_info_front);
		}

		if(pVid_info_rear->data_source == VID_SRC_USB)
		{
			sensor_preview_src_format_set(PREVIEW_SRC_REAR, PIPELINE_SCALER_INPUT_FORMAT_GP420);
			// Allocate memory to store data for HOST_USB_MJPEG_CAM/INTERP2K
			host_2K_preview_buffer_set(PREVIEW_SRC_REAR,pVid_info_rear);
		}
		#endif
	}

	preview_buf_queue_clear();

	if(display_width && display_height) // The product has a screen
	{
		sensor_preview_buf_func_set(
			preview_buf_malloc,
			preview_buf_free,
			preview_buf_show
		);
	}

	/*=== Y ONLY ===*/
#if defined(C_Y_ONLY_FUNCTION) && C_Y_ONLY_FUNCTION == CUSTOM_ON
	switch(pY_only_info->data_source)
	{
		case Y_ONLY_SRC_CDSP:
			sensor_y_only_src_path_set(Y_ONLY_IN_PATH_CDSP);
			break;
		case Y_ONLY_SRC_CSI0:
			sensor_y_only_src_path_set(Y_ONLY_IN_PATH_CSI0);
			break;
		case Y_ONLY_SRC_DRAM:
			sensor_y_only_src_path_set(Y_ONLY_IN_PATH_DRAM);
			break;
		default:
			sensor_y_only_src_path_set(Y_ONLY_IN_PATH_NONE);
			break;
	}
	if(pY_only_info->data_source != Y_ONLY_SRC_NONE){
		if(pY_only_info->data_source == Y_ONLY_SRC_DRAM)
		{
			switch(pY_only_info->data_format)
			{
				case Y_ONLY_DATA_420:
					sensor_y_only_src_format_set(PIPELINE_SCALER_INPUT_FORMAT_GP420);
					break;
				case Y_ONLY_DATA_Y:
					sensor_y_only_src_format_set(PIPELINE_SCALER_INPUT_FORMAT_Y);
					break;
				default:
					ERR_PRINT("PScaler Y error src format\r\n");
					break;
			}
		}

		sensor_y_only_in_width_set(pY_only_info->sensor_width);
		sensor_y_only_in_height_set(pY_only_info->sensor_height);
		sensor_y_only_clip_width_set(pY_only_info->clip_width);
		sensor_y_only_clip_height_set(pY_only_info->clip_height);
		sensor_y_only_out_width_set(pY_only_info->target_width);
		sensor_y_only_out_height_set(pY_only_info->target_height);
		sensor_y_only_in_fifo_line_set(pY_only_info->in_fifo_line);
	}
#endif

	if(sensor_preview_start() == STATUS_FAIL)
	{
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

INT32S
cb_func_sensor_stop(
	void
)
{
	#if ((_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K))
	VID_INFO*  pVid_info_front = &pStreamArgs->vid_info_front;
	VID_INFO*  pVid_info_rear = &pStreamArgs->vid_info_rear;
	VID_INFO*  pVid_info_select;
	INT8U  i;
	#endif

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);
	#if ((_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K))
	for(i = PREVIEW_SRC_FRONT; i <= PREVIEW_SRC_REAR; i++)
	{
		if(i == PREVIEW_SRC_FRONT)
		{
			pVid_info_select = pVid_info_front;
		}
		else if(i == PREVIEW_SRC_REAR)
		{
			pVid_info_select = pVid_info_rear;
		}

		if((pVid_info_select->data_source == VID_SRC_USB)
			#if (C_VID_INTERPOLATING_2K)
			|| (pVid_info_select->run_2K_flow_src & HOST_2K_IS_REAR)
			#endif
		)
		{
			lib_preview_buffer_release(i,pVid_info_select);
		}
	}
	#endif

	return STATUS_OK;
}

INT32S
cb_func_preview_mode_set(
	PREVIEW_MODE mode
)
{
	sensor_preview_mode_set(mode, 0);

	return STATUS_OK;
}

INT32S
cb_h264_encode_buf_release(
	INT8U src,
	VID_INFO* args
)
{
	INT32S nRet = STATUS_OK;
	H264Enc_t* pH264Enc = &args->h264_rec_args.h264_cfg;
	FB_CFG* pPscalerOutCfg = &args->h264_rec_args.fb_pscaler_out_cfg;
	FB_CFG* pH264OutCfg_I = &args->h264_rec_args.fb_h264_out_I_cfg;
	FB_CFG* pH264OutCfg_P = &args->h264_rec_args.fb_h264_out_P_cfg;
	FB_CFG* pH264OutCfg_SKIP = &args->h264_rec_args.fb_h264_out_SKIP_cfg;
	FB_CFG* pMsgNotifyCfg = &args->h264_rec_args.fb_msg_notify_cfg;

	#if (C_VID_INTERPOLATING_2K)
	FB_CFG* pInterp2KPscalerOutCfg = &args->h264_rec_args.fb_interp2K_ps_out_cfg;
	FB_CFG* pInterp2KMsgNotifyCfg = &args->h264_rec_args.fb_interp2K_msg_notify_cfg;
	#endif

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if((args->data_source != VID_SRC_USB)
		#if (C_VID_INTERPOLATING_2K)
		&& !(args->run_2K_flow_src & HOST_2K_IS_REAR) // free pscaler_B bypass A/B buffer
		#endif
	)
	{
		// Pscaler bypass
		if(pPscalerOutCfg->fb_cfg_head_addr)
		{
			#if C_VID_REC_USE_SYS_FIFO
			pPscalerOutCfg->fb_cfg_head_addr = (void *)lib_encode_system_fifo_unmapping(src, (INT32U)pPscalerOutCfg->fb_cfg_head_addr);
			#endif
			gp_free(pPscalerOutCfg->fb_cfg_head_addr);

			pPscalerOutCfg->fb_cfg_head_addr = 0;
		}
		if(pPscalerOutCfg->fb_cfg_cb_addr)
		{
			gp_free(pPscalerOutCfg->fb_cfg_cb_addr);
			pPscalerOutCfg->fb_cfg_cb_addr = 0;
		}
	}

	// H264-I
	if(pH264OutCfg_I->fb_cfg_head_addr)
	{
		gp_free(pH264OutCfg_I->fb_cfg_head_addr);
		pH264OutCfg_I->fb_cfg_head_addr = 0;
	}
	if(pH264OutCfg_I->fb_cfg_cb_addr)
	{
		gp_free(pH264OutCfg_I->fb_cfg_cb_addr);
		pH264OutCfg_I->fb_cfg_cb_addr = 0;
	}

	// H264-P
	if(pH264OutCfg_P->fb_cfg_head_addr)
	{
		gp_free(pH264OutCfg_P->fb_cfg_head_addr);
		pH264OutCfg_P->fb_cfg_head_addr = 0;
	}
	if(pH264OutCfg_P->fb_cfg_cb_addr)
	{
		gp_free(pH264OutCfg_P->fb_cfg_cb_addr);
		pH264OutCfg_P->fb_cfg_cb_addr = 0;
	}

	// H264-SKIP
	if(pH264OutCfg_SKIP->fb_cfg_head_addr)
	{
	   gp_free(pH264OutCfg_SKIP->fb_cfg_head_addr);
	   pH264OutCfg_SKIP->fb_cfg_head_addr = 0;
	}
	if(pH264OutCfg_SKIP->fb_cfg_cb_addr)
	{
		gp_free(pH264OutCfg_SKIP->fb_cfg_cb_addr);
		pH264OutCfg_SKIP->fb_cfg_cb_addr = 0;
	}

	// Msg
	if(pMsgNotifyCfg->fb_cfg_head_addr)
	{
		gp_free(pMsgNotifyCfg->fb_cfg_head_addr);
		pMsgNotifyCfg->fb_cfg_head_addr = 0;
	}
	if(pMsgNotifyCfg->fb_cfg_cb_addr)
	{
		gp_free(pMsgNotifyCfg->fb_cfg_cb_addr);
		pMsgNotifyCfg->fb_cfg_cb_addr = 0;
	}

	// H264 driver
	if(pH264Enc->handle)
	{
	    gp_h264enc_release(pH264Enc->handle);
		pH264Enc->handle = 0;
	}

	#if (C_VID_INTERPOLATING_2K)
	if(pInterp2KPscalerOutCfg->fb_cfg_head_addr)
	{// Unmapping Pscaler_A bypass buffer
	    #if C_VID_REC_USE_SYS_FIFO
	    pInterp2KPscalerOutCfg->fb_cfg_head_addr = (void *)lib_encode_system_fifo_unmapping(src, (INT32U)pInterp2KPscalerOutCfg->fb_cfg_head_addr);
	    #endif

		gp_free(pInterp2KPscalerOutCfg->fb_cfg_head_addr);
		pInterp2KPscalerOutCfg->fb_cfg_head_addr = 0;
	}
	if(pInterp2KPscalerOutCfg->fb_cfg_cb_addr)
	{
		gp_free(pInterp2KPscalerOutCfg->fb_cfg_cb_addr);
		pInterp2KPscalerOutCfg->fb_cfg_cb_addr = 0;
	}

	if(pInterp2KMsgNotifyCfg->fb_cfg_head_addr)
	{
		gp_free(pInterp2KMsgNotifyCfg->fb_cfg_head_addr);
		pInterp2KMsgNotifyCfg->fb_cfg_head_addr = 0;
	}
	if(pInterp2KMsgNotifyCfg->fb_cfg_cb_addr)
	{
		gp_free(pInterp2KMsgNotifyCfg->fb_cfg_cb_addr);
		pInterp2KMsgNotifyCfg->fb_cfg_cb_addr = 0;
	}
	#endif

	return nRet;
}

INT32S
cb_h264_encode_param_set(
	INT8U src,
	VID_INFO* args,
	ENCODE_PARAMS* pEnc_params
)
{
	INT32S nRet = STATUS_OK;
	H264Enc_t* pH264Enc = &args->h264_rec_args.h264_cfg;
	FB_CFG* pPscalerOutCfg = &args->h264_rec_args.fb_pscaler_out_cfg;
	FB_CFG* pH264OutCfg_I = &args->h264_rec_args.fb_h264_out_I_cfg;
	FB_CFG* pH264OutCfg_P = &args->h264_rec_args.fb_h264_out_P_cfg;
	FB_CFG* pH264OutCfg_SKIP = &args->h264_rec_args.fb_h264_out_SKIP_cfg;
	FB_CFG* pMsgNotifyCfg = &args->h264_rec_args.fb_msg_notify_cfg;
	INT32U  height_16_alignment; //16 alignment for hw constrain

	#if (C_VID_INTERPOLATING_2K)
	FB_CFG* pInterp2KPscalerOutCfg = &args->h264_rec_args.fb_interp2K_ps_out_cfg;
	FB_CFG* pInterp2KMsgNotifyCfg = &args->h264_rec_args.fb_interp2K_msg_notify_cfg;
	#endif

	args->data_format = pEnc_params->in_data_format;

	// VID_SRC_USB/INTERP2K(REAR) do the following code in cb_h264_preview_buffer_set
	if((args->data_source != VID_SRC_USB)
		#if (C_VID_INTERPOLATING_2K)
		&& !(args->run_2K_flow_src & HOST_2K_IS_REAR) // For front sensor or non-scale-up flow
		#endif
	)
	{
		/* 16 alignment */
		height_16_alignment = (args->target_height + 0xF) >> 4 << 4;

		// Init pscaler output buffer
		if(args->vid_rec_line)
		{	// FIFO mode
			pPscalerOutCfg->fb_cfg_size = args->target_width*args->vid_rec_line;
			if(args->data_format == VID_DATA_420)
			{
				pPscalerOutCfg->fb_cfg_size+= (args->target_width*args->vid_rec_line)>>1;
			}
			else
			{
				pPscalerOutCfg->fb_cfg_size+= args->target_width*args->vid_rec_line;
			}

			pPscalerOutCfg->fb_cfg_max_count = height_16_alignment/args->vid_rec_line;
			if(height_16_alignment != (pPscalerOutCfg->fb_cfg_max_count*args->vid_rec_line)) pPscalerOutCfg->fb_cfg_max_count++;
		}
		else // Frame mode
		{
			pPscalerOutCfg->fb_cfg_size = args->target_width*height_16_alignment;
			if(args->data_format == VID_DATA_420)
			{
				pPscalerOutCfg->fb_cfg_size+= (args->target_width*height_16_alignment)>>1;
			}
			else
			{
				pPscalerOutCfg->fb_cfg_size+= args->target_width*height_16_alignment;
			}

			pPscalerOutCfg->fb_cfg_max_count = 1;
		}

		DBG_PRINT("=>[%s] max:%d \r\n",__FUNCTION__,pPscalerOutCfg->fb_cfg_max_count);

		if(pEnc_params->pscaler_bypass_out_cnt)
		{
			pPscalerOutCfg->fb_cfg_blocks = pEnc_params->pscaler_bypass_out_cnt;
			pPscalerOutCfg->fb_cfg_head_addr = gp_malloc_align(pPscalerOutCfg->fb_cfg_blocks*pPscalerOutCfg->fb_cfg_size,64);
			if(pPscalerOutCfg->fb_cfg_head_addr == 0)
			{
				ERR_PRINT("fail to pscaler's fb_cfg_head_addr\r\n");
				RETURN(STATUS_FAIL);
			}

			cache_invalid_range((INT32U)pPscalerOutCfg->fb_cfg_head_addr,pPscalerOutCfg->fb_cfg_blocks*pPscalerOutCfg->fb_cfg_size);

			// Pscaler output's circular_buffer
			pPscalerOutCfg->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pPscalerOutCfg->fb_cfg_blocks,64);
			if(pPscalerOutCfg->fb_cfg_cb_addr == 0)
			{
				ERR_PRINT("fail to pscaler's fb_cfg_cb_addr\r\n");
				RETURN(STATUS_FAIL);
			}
		}
	}

	// Init h264 output buffer
	pH264OutCfg_I->fb_cfg_size = pEnc_params->h264_I_out_size;
	pH264OutCfg_I->fb_cfg_blocks = pEnc_params->h264_I_out_cnt;
	pH264OutCfg_I->fb_cfg_head_addr = gp_malloc_align(pH264OutCfg_I->fb_cfg_blocks*pH264OutCfg_I->fb_cfg_size,64);
	if(pH264OutCfg_I->fb_cfg_head_addr == 0)
	{
		ERR_PRINT("fail to pH264OutCfg_I's fb_cfg_head_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	cache_invalid_range((INT32U)pH264OutCfg_I->fb_cfg_head_addr,pH264OutCfg_I->fb_cfg_blocks*pH264OutCfg_I->fb_cfg_size);

	pH264OutCfg_P->fb_cfg_size = pEnc_params->h264_P_out_size;
	pH264OutCfg_P->fb_cfg_blocks = pEnc_params->h264_P_out_cnt;
	pH264OutCfg_P->fb_cfg_head_addr = gp_malloc_align(pH264OutCfg_P->fb_cfg_blocks*pH264OutCfg_P->fb_cfg_size,64);
	if(pH264OutCfg_P->fb_cfg_head_addr == 0)
	{
		ERR_PRINT("fail to pH264OutCfg_P's fb_cfg_head_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	cache_invalid_range((INT32U)pH264OutCfg_P->fb_cfg_head_addr,pH264OutCfg_P->fb_cfg_blocks*pH264OutCfg_P->fb_cfg_size);

	pH264OutCfg_SKIP->fb_cfg_size   = pEnc_params->h264_SKIP_out_size;
	pH264OutCfg_SKIP->fb_cfg_blocks = pEnc_params->h264_SKIP_out_cnt;
	if(pH264OutCfg_SKIP->fb_cfg_blocks!=0&&pH264OutCfg_SKIP->fb_cfg_size!=0)
	{
        pH264OutCfg_SKIP->fb_cfg_head_addr =  gp_malloc_align(pH264OutCfg_SKIP->fb_cfg_blocks*pH264OutCfg_SKIP->fb_cfg_size,64);
        if(pH264OutCfg_SKIP->fb_cfg_head_addr ==0)
            {
                ERR_PRINT("fail to pH264OutCfg_SKIP's fb_cfg_head_addr\r\n");
                RETURN(STATUS_FAIL);
            }
    }
    if(pH264OutCfg_SKIP->fb_cfg_blocks==0&&pH264OutCfg_SKIP->fb_cfg_size==0) //  run_ap_mode == wifi mode
    {
        pH264OutCfg_SKIP->fb_cfg_head_addr =0;
    }

	gp_memset((INT8S *)pH264Enc, 0, sizeof(H264Enc_t));
	pH264Enc->cfg.width = args->target_width;					    //encode width
	pH264Enc->cfg.height = args->target_height;				        //encode height, should be 16 align
	pH264Enc->cfg.level = (args->target_width > 1920) ? 51:41;	    //h264 profile level
	/*
		Workaround: Missing H264 data due to hardware Interrupt lost
	*/
	pH264Enc->cfg.fifo_mode = 1;					                //enable h264 output fifo mode.
    #if H264ENC_FBC_ENABLE == 1
    pH264Enc->cfg.buf_num = 2;                                      //internal working frame buffer number, default 1. (2 for fbc enable)
    #else
    pH264Enc->cfg.buf_num = 1;
    #endif
	pH264Enc->cfg.fbc = H264ENC_FBC_ENABLE;						    //frame buffer compression to reduce memory access bandwidth if needed. require 2 internal buffers if enabled.

	pH264Enc->rccfg.bitPerSeconds = pEnc_params->bitPerSeconds;		// target bitrate in kbps for ABR mode
	pH264Enc->rccfg.frameRate = pEnc_params->frameRate;				// framerate for accurate bitrate
	pH264Enc->rccfg.gopLen = pEnc_params->gopLen;					// length of GOP (Group of Picutres)
	pH264Enc->rccfg.qpHdr = pEnc_params->qp_start;					// starting qp value
	pH264Enc->rccfg.qpMax = pEnc_params->qp_max;					// maximum qp value allow
	pH264Enc->rccfg.qpMin = pEnc_params->qp_min;					// minimum qp value allow
	pH264Enc->rccfg.hierPFrames = pEnc_params->hierPFrames;			// hierachical P frame
	pH264Enc->rccfg.qualityLevel = 32;								// quality level for CRF mode.
	pH264Enc->rccfg.rc_mode = H264_RC_ABR;							// rate control mode: ABR (Average Bitrate)/CRF(Constant Rate Factor)
	pH264Enc->rccfg.vbv_dbg = 0;									// set 0 to disable VBV debug message.

	if(lib_h264_setting(pH264Enc) == STATUS_FAIL)
	{
		ERR_PRINT("fail to lib_h264_setting\r\n");
		RETURN(STATUS_FAIL);
	}

	// H264 I-frame output's circular_buffer
	pH264OutCfg_I->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pH264OutCfg_I->fb_cfg_blocks,64);
	if(pH264OutCfg_I->fb_cfg_cb_addr == 0)
	{
		ERR_PRINT("fail to pH264OutCfg_I's fb_cfg_cb_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	// H264 P-frame output's circular_buffer
	pH264OutCfg_P->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pH264OutCfg_P->fb_cfg_blocks,64);
	if(pH264OutCfg_P->fb_cfg_cb_addr == 0)
	{
		ERR_PRINT("fail to pH264OutCfg_P's fb_cfg_cb_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	// H264 SKIP-frame output's circular_buffer
	if(pH264OutCfg_SKIP->fb_cfg_blocks!=0)
   {
        pH264OutCfg_SKIP->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pH264OutCfg_SKIP->fb_cfg_blocks,64);
        if(pH264OutCfg_SKIP->fb_cfg_cb_addr == 0)
        {
            ERR_PRINT("fail to pH264OutCfg_SKIP's fb_cfg_cb_addr\r\n");
            RETURN(STATUS_FAIL);
        }
    }
    if(pH264OutCfg_SKIP->fb_cfg_blocks==0&&pH264OutCfg_SKIP->fb_cfg_size==0)  //  run_ap_mode == wifi mode
    {
        pH264OutCfg_SKIP->fb_cfg_cb_addr =0;
    }

	#if 0
	DBG_PRINT("[Pscaler] size:%d,blocks:%d,head_addr:0x%x cb_addr:0x%x\r\n",
	pPscalerOutCfg->fb_cfg_size,pPscalerOutCfg->fb_cfg_blocks,pPscalerOutCfg->fb_cfg_head_addr,pPscalerOutCfg->fb_cfg_cb_addr);

	DBG_PRINT("[H264-I] size:%d,blocks:%d,head_addr:0x%x cb_addr:0x%x\r\n",
	pH264OutCfg_I->fb_cfg_size,pH264OutCfg_I->fb_cfg_blocks,pH264OutCfg_I->fb_cfg_head_addr,pH264OutCfg_I->fb_cfg_cb_addr);

	DBG_PRINT("[H264-P] size:%d,blocks:%d,head_addr:0x%x cb_addr:0x%x\r\n",
	pH264OutCfg_P->fb_cfg_size,pH264OutCfg_P->fb_cfg_blocks,pH264OutCfg_P->fb_cfg_head_addr,pH264OutCfg_P->fb_cfg_cb_addr);
	#endif

	// Init message notify
	pMsgNotifyCfg->fb_cfg_blocks = VID_ENC_MSG_NOTIFY_CNT;
	pMsgNotifyCfg->fb_cfg_head_addr = gp_malloc_align(pMsgNotifyCfg->fb_cfg_blocks*sizeof(FB_INFO),64);
	if(pMsgNotifyCfg->fb_cfg_head_addr == 0)
	{
		ERR_PRINT("fail to pMsgNotifyCfg's fb_cfg_head_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	// Message's circular_buffer
	pMsgNotifyCfg->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pMsgNotifyCfg->fb_cfg_blocks,64);
	if(pMsgNotifyCfg->fb_cfg_cb_addr == 0)
	{
		ERR_PRINT("fail to pMsgNotifyCfg's fb_cfg_cb_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	#if (C_VID_INTERPOLATING_2K)
	if(pEnc_params->interp2K_bypass_out_cnt)
	{// It's for saving pscaler_A bypass output buffer,than scale up via pscaler_B
		// Frame mode
		pInterp2KPscalerOutCfg->fb_cfg_size = args->sensor_width*args->sensor_height;
		if(args->data_format == VID_DATA_420)
		{
			pInterp2KPscalerOutCfg->fb_cfg_size+= (args->sensor_width*args->sensor_height)>>1;
		}
		else
		{
			pInterp2KPscalerOutCfg->fb_cfg_size+= args->sensor_width*args->sensor_height;
		}

		pInterp2KPscalerOutCfg->fb_cfg_max_count = 1;

		pInterp2KPscalerOutCfg->fb_cfg_blocks = pEnc_params->interp2K_bypass_out_cnt;
		pInterp2KPscalerOutCfg->fb_cfg_head_addr = gp_malloc_align(pInterp2KPscalerOutCfg->fb_cfg_blocks*pInterp2KPscalerOutCfg->fb_cfg_size,64);
		if(pInterp2KPscalerOutCfg->fb_cfg_head_addr == 0)
		{
			ERR_PRINT("fail to interp2K's fb_cfg_head_addr\r\n");
			RETURN(STATUS_FAIL);
		}

		// Pscaler output's circular_buffer
		pInterp2KPscalerOutCfg->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pInterp2KPscalerOutCfg->fb_cfg_blocks,64);
		if(pInterp2KPscalerOutCfg->fb_cfg_cb_addr == 0)
		{
			ERR_PRINT("fail to interp2K's fb_cfg_cb_addr\r\n");
			RETURN(STATUS_FAIL);
		}
	}

	// Init message notify
	pInterp2KMsgNotifyCfg->fb_cfg_blocks = VID_ENC_MSG_NOTIFY_CNT;
	pInterp2KMsgNotifyCfg->fb_cfg_head_addr = gp_malloc_align(pInterp2KMsgNotifyCfg->fb_cfg_blocks*sizeof(FB_INFO),64);
	if(pInterp2KMsgNotifyCfg->fb_cfg_head_addr == 0)
	{
		ERR_PRINT("fail to pMsgNotifyCfg's fb_cfg_head_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	// Message's circular_buffer
	pInterp2KMsgNotifyCfg->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pInterp2KMsgNotifyCfg->fb_cfg_blocks,64);
	if(pInterp2KMsgNotifyCfg->fb_cfg_cb_addr == 0)
	{
		ERR_PRINT("fail to pMsgNotifyCfg's fb_cfg_cb_addr\r\n");
		RETURN(STATUS_FAIL);
	}
	#endif

	return STATUS_OK;
Return:

	cb_h264_encode_buf_release(src,args);

	return nRet;
}

INT32S
front_h264_encode_param_set(
	VID_INFO* args
)
{
	INT32U pscaler_bypass_out_cnt = VID_PSCALER_OUT_CNT_FRONT;

	#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
	INT32U interp2K_pscaler_bypass_out_cnt = 0;

	if(args->run_2K_flow_src) // This value is Greater than 0, that means front do scale-up flow
	{
		pscaler_bypass_out_cnt = 2; // Don't change this fixed value, it's for pscaler_B bypass output A/B buffer
		interp2K_pscaler_bypass_out_cnt = INTERP2K_PSCALER_OUT_CNT_FRONT; // Save pscaler_A bypass output buffer
	}
	#endif

	ENCODE_PARAMS h264_param = {
	pscaler_bypass_out_cnt,
	VID_DATA_420,
	VID_H264_OUT_I_CNT_FRONT,
	VID_H264_OUT_I_SIZE_FRONT,
	VID_H264_OUT_P_CNT_FRONT,
	VID_H264_OUT_P_SIZE_FRONT,
	VID_H264_OUT_SKIP_CNT_FRONT,
	VID_H264_OUT_SKIP_SIZE_FRONT,
	VID_REC_BITRATE_RATE_FRONT,
	VID_GOP_LEN_FRONT,
	VID_REC_FRAME_RATE_FRONT,
	H264_QP_START_FRONT,
	H264_QP_MAX_FRONT,
	H264_QP_MIN_FRONT,
	H264_HIERPFRAME_FRONT,
	#if (C_VID_INTERPOLATING_2K)
	interp2K_pscaler_bypass_out_cnt
	#endif
	};

	return cb_h264_encode_param_set(PREVIEW_SRC_FRONT,args,&h264_param);
}

INT32S
front_h264_encode_buf_release(
	VID_INFO* args
)
{
	return cb_h264_encode_buf_release(PREVIEW_SRC_FRONT,args);
}

INT32S
rear_h264_encode_param_set(
	VID_INFO* args
)
{
	ENCODE_PARAMS h264_param = {
	VID_PSCALER_OUT_CNT_REAR,
	args->data_format,
	VID_H264_OUT_I_CNT_REAR,
	VID_H264_OUT_I_SIZE_REAR,
	VID_H264_OUT_P_CNT_REAR,
	VID_H264_OUT_P_SIZE_REAR,
	VID_H264_OUT_SKIP_CNT_REAR,
	VID_H264_OUT_SKIP_SIZE_REAR,
	VID_REC_BITRATE_RATE_REAR,
	VID_GOP_LEN_REAR,
	VID_REC_FRAME_RATE_REAR,
	H264_QP_START_REAR,
	H264_QP_MAX_REAR,
	H264_QP_MIN_REAR,
	H264_HIERPFRAME_REAR,
	#if (C_VID_INTERPOLATING_2K)
	0
	#endif
	};

	return cb_h264_encode_param_set(PREVIEW_SRC_REAR,args,&h264_param);
}

INT32S
rear_h264_encode_buf_release(
	VID_INFO* args
)
{
	return cb_h264_encode_buf_release(PREVIEW_SRC_REAR,args);
}

INT32S
cb_jpeg_encode_buf_release(
	INT8U src,
	VID_INFO* args
)
{
	INT32S nRet = STATUS_OK;
	JpegEnc_t* pJpegEnc = &args->jpeg_rec_args.jpeg_cfg;
	FB_CFG* pPscalerOutCfg = &args->jpeg_rec_args.fb_pscaler_out_cfg;
	FB_CFG* pJpegOutCfg = &args->jpeg_rec_args.fb_jpeg_out_cfg;
	FB_CFG* pMsgNotifyCfg = &args->jpeg_rec_args.fb_msg_notify_cfg;

	if(args->data_source != VID_SRC_USB)
	{
		// Pscaler bypass
		if(pPscalerOutCfg->fb_cfg_head_addr)
		{
			#if C_VID_REC_USE_SYS_FIFO
			pPscalerOutCfg->fb_cfg_head_addr = (void *)lib_encode_system_fifo_unmapping(src, (INT32U)pPscalerOutCfg->fb_cfg_head_addr);
			#endif
			gp_free(pPscalerOutCfg->fb_cfg_head_addr);
			pPscalerOutCfg->fb_cfg_head_addr = 0;
		}
		if(pPscalerOutCfg->fb_cfg_cb_addr)
		{
			gp_free(pPscalerOutCfg->fb_cfg_cb_addr);
			pPscalerOutCfg->fb_cfg_cb_addr = 0;
		}
	}

	// Jpeg
	if(pJpegOutCfg->fb_cfg_head_addr)
	{
		gp_free(pJpegOutCfg->fb_cfg_head_addr);
		pJpegOutCfg->fb_cfg_head_addr = 0;
	}
	if(pJpegOutCfg->fb_cfg_cb_addr)
	{
		gp_free(pJpegOutCfg->fb_cfg_cb_addr);
		pJpegOutCfg->fb_cfg_cb_addr = 0;
	}

	// Msg
	if(pMsgNotifyCfg->fb_cfg_head_addr)
	{
		gp_free(pMsgNotifyCfg->fb_cfg_head_addr);
		pMsgNotifyCfg->fb_cfg_head_addr = 0;
	}

	if(pMsgNotifyCfg->fb_cfg_cb_addr)
	{
		gp_free(pMsgNotifyCfg->fb_cfg_cb_addr);
		pMsgNotifyCfg->fb_cfg_cb_addr = 0;
	}

	// Jpeg driver
	if(pJpegEnc->header)
	{
		gp_free(pJpegEnc->header);
		pJpegEnc->header = 0;
	}

	jpeg_encode_stop(pJpegEnc->hwidx);
	release_used_jpeg(pJpegEnc->hwidx);

	return nRet;
}

INT32S
cb_jpeg_encode_param_set(
	INT8U src,
	VID_INFO* args,
	ENCODE_PARAMS* pEnc_params
)
{
	INT32S nRet = STATUS_OK;
	INT8U i;
	JpegEnc_t* pJpegEnc = &args->jpeg_rec_args.jpeg_cfg;
	FB_CFG* pPscalerOutCfg = &args->jpeg_rec_args.fb_pscaler_out_cfg;
	FB_CFG* pJpegOutCfg = &args->jpeg_rec_args.fb_jpeg_out_cfg;
	FB_CFG* pMsgNotifyCfg = &args->jpeg_rec_args.fb_msg_notify_cfg;
	INT32U final_width=0,final_height=0;
	INT32U jpeg_outb_header_size = sizeof(JPEG_OUTB_HEADER);
	JPEG_OUTB_HEADER* pJpeg_outb_header;
	INT32U jpegHeaderSize = jpeg_header_get_size();

	args->data_format = pEnc_params->in_data_format;

	pJpegEnc->scale_mode = JPEG_SCALE_NONE;
	if(args->sensor_width != args->target_width)
	{
		if(args->data_format == VID_DATA_420)
		{
			// target width / sensor width = 1.5
			if((args->target_width*2) == (args->sensor_width*3))
			{
				pJpegEnc->scale_mode = JPEG_SCALE_GP420_X3D2;
			}
            else if(args->target_width == args->sensor_width*2)
			{
                pJpegEnc->scale_mode = JPEG_SCALE_X2;
			}
            DBG_PRINT("JPEG Encode Scaler Up:[%d,%d]\r\n",args->sensor_width,args->target_width);
		}
	}

	if(pJpegEnc->scale_mode)
	{
		final_width = args->sensor_width;
		final_height =  args->sensor_height;
	}
	else
	{
		final_width = args->target_width;
		final_height = args->target_height;
	}

	// VID_SRC_USB do the following code in cb_jpeg_preview_buffer_set
	if(args->data_source != VID_SRC_USB)
	{
		// Init pscaler output buffer
		if(args->vid_rec_line)
		{	// FIFO mode
			pPscalerOutCfg->fb_cfg_size = final_width*args->vid_rec_line;
			if(args->data_format == VID_DATA_420)
			{
				pPscalerOutCfg->fb_cfg_size+= (final_width*args->vid_rec_line)>>1;
			}
			else
			{
				pPscalerOutCfg->fb_cfg_size+= final_width*args->vid_rec_line;
			}

			pPscalerOutCfg->fb_cfg_max_count = final_height/args->vid_rec_line;
			if(final_height != (pPscalerOutCfg->fb_cfg_max_count*args->vid_rec_line)) pPscalerOutCfg->fb_cfg_max_count++;
		}
		else // Frame mode
		{
			pPscalerOutCfg->fb_cfg_size =final_width*final_height;
			if(args->data_format == VID_DATA_420)
			{
				pPscalerOutCfg->fb_cfg_size+= (final_width*final_height)>>1;
			}
			else
			{
				pPscalerOutCfg->fb_cfg_size+= final_width*final_height;
			}

			pPscalerOutCfg->fb_cfg_max_count = 1;
		}

		//DBG_PRINT("=>[%s] max:%d \r\n",__FUNCTION__,pPscalerOutCfg->fb_cfg_max_count);

		if(pEnc_params->pscaler_bypass_out_cnt)
		{
			pPscalerOutCfg->fb_cfg_blocks = pEnc_params->pscaler_bypass_out_cnt;
			pPscalerOutCfg->fb_cfg_head_addr = gp_malloc_align(pPscalerOutCfg->fb_cfg_blocks*pPscalerOutCfg->fb_cfg_size,64);
			if(pPscalerOutCfg->fb_cfg_head_addr == 0)
			{
				ERR_PRINT("fail to pscaler's fb_cfg_head_addr\r\n");
				RETURN(STATUS_FAIL);
			}

			cache_invalid_range((INT32U)pPscalerOutCfg->fb_cfg_head_addr,pPscalerOutCfg->fb_cfg_blocks*pPscalerOutCfg->fb_cfg_size);

			pPscalerOutCfg->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pPscalerOutCfg->fb_cfg_blocks,64);
			if(pPscalerOutCfg->fb_cfg_cb_addr == 0)
			{
				ERR_PRINT("fail to pscaler's fb_cfg_cb_addr\r\n");
				RETURN(STATUS_FAIL);
			}
		}
	}

	/*
	Init jpeg output buffer
	INT32U h264_I_out_cnt; // JPEG out cnt in this member variable
	INT32U h264_I_out_size; // JPEG out size in this member variable
	*/

	pJpegOutCfg->fb_cfg_size = pEnc_params->h264_I_out_size+jpeg_outb_header_size;
	pJpegOutCfg->fb_cfg_blocks = pEnc_params->h264_I_out_cnt;
	pJpegOutCfg->fb_cfg_head_addr = gp_malloc_align(pJpegOutCfg->fb_cfg_blocks*pJpegOutCfg->fb_cfg_size,64);
	if(pJpegOutCfg->fb_cfg_head_addr == 0)
	{
		ERR_PRINT("fail to pJpegOut's fb_cfg_head_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	cache_invalid_range((INT32U)pJpegOutCfg->fb_cfg_head_addr,pJpegOutCfg->fb_cfg_blocks*pJpegOutCfg->fb_cfg_size);

	if(pJpegEnc->scale_mode && (args->sensor_width > 1280)) // For performance
	{
		pJpegEnc->hwidx = get_idle_jpeg(JPEGA);
	}
	else
	{
		pJpegEnc->hwidx = get_idle_jpeg(-1);
	}

	pJpegEnc->quality_value = 35;
	pJpegEnc->yuv_fmt = args->data_format;
	pJpegEnc->yuv_sample_fmt = args->data_format;
	pJpegEnc->width = final_width;
	pJpegEnc->height = final_height;
	pJpegEnc->output_buffer = (INT32U)pJpegOutCfg->fb_cfg_head_addr;
	pJpegEnc->output_buffer_len = pJpegOutCfg->fb_cfg_size;
	pJpegEnc->input_y_len = pPscalerOutCfg->fb_cfg_size;

	if(lib_jpeg_header_create(pJpegEnc) == STATUS_FAIL)
	{
		ERR_PRINT("fail to lib_jpeg_header_create\r\n");
		RETURN(STATUS_FAIL);
	}

	lib_jpeg_setting(pJpegEnc);

	// Copy jpeg header to jpeg's output buffer
	for(i=0; i<pJpegOutCfg->fb_cfg_blocks; i++)
	{
		pJpeg_outb_header = (JPEG_OUTB_HEADER*)(pJpegOutCfg->fb_cfg_head_addr+(i*pJpegOutCfg->fb_cfg_size));
		//DBG_PRINT("[JPEG] outb:0x%x\r\n",pJpeg_outb_header);
		pJpeg_outb_header->jpeg_q.Y_q_table = pJpegEnc->quality_value;
		gp_memcpy((INT8S *)pJpeg_outb_header->jpeg_header, (INT8S *)pJpegEnc->header, jpegHeaderSize);
	}

	pJpegOutCfg->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pJpegOutCfg->fb_cfg_blocks,64);
	if(pJpegOutCfg->fb_cfg_cb_addr == 0)
	{
		ERR_PRINT("fail to pJpegOut's fb_cfg_cb_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	#if 0
	DBG_PRINT("[Pscaler] size:%d,blocks:%d,head_addr:0x%x cb_addr:0x%x\r\n",
	pPscalerOutCfg->fb_cfg_size,pPscalerOutCfg->fb_cfg_blocks,pPscalerOutCfg->fb_cfg_head_addr,pPscalerOutCfg->fb_cfg_cb_addr);

	DBG_PRINT("[Jpeg] size:%d,blocks:%d,head_addr:0x%x cb_addr:0x%x\r\n",
	pJpegOutCfg->fb_cfg_size,pJpegOutCfg->fb_cfg_blocks,pJpegOutCfg->fb_cfg_head_addr,pJpegOutCfg->fb_cfg_cb_addr);
	#endif

	// Init message notify
	pMsgNotifyCfg->fb_cfg_blocks = VID_ENC_MSG_NOTIFY_CNT;
	pMsgNotifyCfg->fb_cfg_head_addr = gp_malloc_align(pMsgNotifyCfg->fb_cfg_blocks*sizeof(FB_INFO),64);
	if(pMsgNotifyCfg->fb_cfg_head_addr == 0)
	{
		ERR_PRINT("fail to pMsgNotifyCfg's fb_cfg_head_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	pMsgNotifyCfg->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pMsgNotifyCfg->fb_cfg_blocks,64);
	if(pMsgNotifyCfg->fb_cfg_cb_addr == 0)
	{
		ERR_PRINT("fail to pMsgNotifyCfg's fb_cfg_cb_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	return nRet;
Return:

	cb_jpeg_encode_buf_release(src,args);

	return nRet;
}

INT32S
front_jpeg_encode_param_set(
	VID_INFO* args
)
{
	ENCODE_PARAMS jpeg_param = {
	VID_PSCALER_OUT_CNT_FRONT,
	VID_DATA_420,
	VID_JPEG_OUT_CNT_FRONT,
	VID_JPEG_OUT_SIZE_FRONT,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	#if (C_VID_INTERPOLATING_2K)
	0
	#endif
	};

	return cb_jpeg_encode_param_set(PREVIEW_SRC_FRONT,args,&jpeg_param);
}

INT32S
front_jpeg_encode_buf_release(
	VID_INFO* args
)
{
	return cb_jpeg_encode_buf_release(PREVIEW_SRC_FRONT,args);
}

INT32S
front_jpeg_encode_Q_init(
	VID_INFO* args
)
{
	INT32U	target_width = args->target_width;
	JengDynaQ_t* pJpeg_dyna_q = &args->jpeg_rec_args.jpeg_q_cfg;
	INT32U encSizMax;
	INT32U encSizMid;
	INT32U encSizMin;
	INT8U  emerY;
	INT8U  maxY;
	INT8U  minY;
	INT8U  step;

	if((target_width == VID_REC_WIDTH_4K) || (target_width == VID_REC_WIDTH_4K))
	{
		encSizMax = VID_JPEG_OUT_SIZE_FRONT - (5 * 1024);
		encSizMid = (400 * 1024);
		encSizMin = (180 * 1024);
		emerY = Q_EMER_4K;
		maxY = Q_MAX_4K;
		minY = Q_MIN_4K;
	}
	else if((target_width == VID_REC_WIDTH_2K) || (target_width == VID_REC_WIDTH_2K))
	{
		encSizMax = VID_JPEG_OUT_SIZE_FRONT - (5 * 1024);
		encSizMid = (220 * 1024);
		encSizMin = (160 * 1024);
		emerY = Q_EMER_2K4K;
		maxY = Q_MAX_2K4K;
		minY = Q_MIN_2K4K;
	}
	else if((target_width == VID_REC_WIDTH_1080FHD) || (target_width == VID_REC_WIDTH_1080P))
	{
		encSizMax = VID_JPEG_OUT_SIZE_FRONT - (5 * 1024);
		encSizMid = (145 * 1024);
		encSizMin = (90 * 1024);
		emerY = Q_EMER_1080P;
		maxY = Q_MAX_1080P;
		minY = Q_MIN_1080P;
	}
	else
	{
		encSizMax = VID_JPEG_OUT_SIZE_FRONT - (5 * 1024);
		encSizMid = (200 * 1024);
		encSizMin = (120 * 1024);
		emerY = Q_EMER_720P;
		maxY = Q_MAX_720P;
		minY = Q_MIN_720P;
	}

	step = Q_STEP;

	vid_dyna_tune_Q_init(pJpeg_dyna_q,
	encSizMax,
	encSizMid,
	encSizMin,
	emerY,
	maxY,
	minY,
	step);
	return STATUS_OK;
}

INT32S
rear_jpeg_encode_param_set(
	VID_INFO* args
)
{
	ENCODE_PARAMS jpeg_param = {
	VID_PSCALER_OUT_CNT_REAR,
	VID_DATA_420,
	VID_JPEG_OUT_CNT_REAR,
	VID_JPEG_OUT_SIZE_REAR,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	#if (C_VID_INTERPOLATING_2K)
	0
	#endif
	};

	return cb_jpeg_encode_param_set(PREVIEW_SRC_REAR,args,&jpeg_param);
}

INT32S
rear_jpeg_encode_buf_release(
	VID_INFO* args
)
{
	return cb_jpeg_encode_buf_release(PREVIEW_SRC_REAR,args);
}

INT32S
rear_jpeg_encode_Q_init(
	VID_INFO* args
)
{
	INT32U	target_width = args->target_width;
	JengDynaQ_t* pJpeg_dyna_q = &args->jpeg_rec_args.jpeg_q_cfg;
	INT32U encSizMax;
	INT32U encSizMid;
	INT32U encSizMin;
	INT8U  emerY;
	INT8U  maxY;
	INT8U  minY;
	INT8U  step;

	if((target_width == VID_REC_WIDTH_1080FHD) || (target_width == VID_REC_WIDTH_1080P))
	{
		encSizMax = VID_JPEG_OUT_SIZE_REAR- (5 * 1024);
		encSizMid = (145 * 1024);
		encSizMin = (90 * 1024);
		emerY = Q_EMER_REAR;
		maxY = Q_MAX_REAR;
		minY = Q_MIN_REAR;
	}
	else
	{
		encSizMax = VID_JPEG_OUT_SIZE_REAR- (5 * 1024);
		encSizMid = (105 * 1024);
		encSizMin = (70 * 1024);
		emerY = Q_EMER_REAR;
		maxY = Q_MAX_REAR;
		minY = Q_MIN_REAR;
	}

	step = Q_STEP;

	vid_dyna_tune_Q_init(pJpeg_dyna_q,
	encSizMax,
	encSizMid,
	encSizMin,
	emerY,
	maxY,
	minY,
	step);

	return STATUS_OK;
}

INT32S
cb_func_encode_start(
	void
)
{
	INT32S nRet = STATUS_OK;

	VID_INFO*  pVid_info_front = (VID_INFO*)&pStreamArgs->vid_info_front;
	VID_INFO*  pVid_info_rear = (VID_INFO*)&pStreamArgs->vid_info_rear;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);
	front_stream_init();

	#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
	uvc_record_stop_req_clear(PREVIEW_SRC_REAR);
	#endif

	rear_stream_init();

	if(pVid_info_front->data_source != VID_SRC_NONE)
	{
		pVid_info_front->add_time_stamp = ap_state_config_record_date_stamp_get();
		#if C_TIME_LAPSE
        g_time_lapse = ap_state_config_time_lapse_recording_temp_get();
        ap_time_lapse_tick_clean();
        #endif
		nRet = front_packer_start(ApQ, pVid_info_front);
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("fail to front_packer_start\r\n");
			RETURN(STATUS_FAIL);
		}

		switch(pVid_info_front->vid_rec_type)
		{
			case VID_REC_H264:
				front_h264_encode_param_set(pVid_info_front);
			break;
			case VID_REC_JPEG:
				front_jpeg_encode_param_set(pVid_info_front);
				front_jpeg_encode_Q_init(pVid_info_front);
			break;
			default:
				ERR_PRINT("[FRONT] unknown video record type:%d\r\n",pVid_info_front->vid_rec_type);
				RETURN(STATUS_FAIL);
		}

		nRet = front_encode_buf_set(encoderTaskQ,pVid_info_front);
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("fail to front_encode_buf_set\r\n");
			RETURN(STATUS_FAIL);
		}
	}

	if(back_sensor_plug_status_get() && (pVid_info_rear->data_source != VID_SRC_NONE))
	{
		pVid_info_rear->add_time_stamp = ap_state_config_record_date_stamp_get();

		nRet = rear_packer_start(ApQ, pVid_info_rear);
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("fail to rear_packer_start\r\n");
			RETURN(STATUS_FAIL);
		}

		switch(pVid_info_rear->vid_rec_type)
		{
			case VID_REC_H264:
				rear_h264_encode_param_set(pVid_info_rear);
			break;
			case VID_REC_JPEG:
				rear_jpeg_encode_param_set(pVid_info_rear);
				rear_jpeg_encode_Q_init(pVid_info_rear);
			break;
			default:
				ERR_PRINT("[REAR] unknown video record type:%d\r\n",pVid_info_rear->vid_rec_type);
				RETURN(STATUS_FAIL);
		}

		nRet = rear_encode_buf_set(encoderTaskQ,pVid_info_rear);
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("fail to rear_encode_buf_set\r\n");
			RETURN(STATUS_FAIL);
		}
	}


	if(pVid_info_front->data_source != VID_SRC_NONE)
	{
		front_encode_start(pVid_info_front);
	}

	if(back_sensor_plug_status_get() && (pVid_info_rear->data_source != VID_SRC_NONE))
	{
		rear_encode_start(pVid_info_rear);
	}

Return:
	return nRet;
}

INT32S
cb_func_encode_stop(
	void
)
{
	INT32S nRet = STATUS_OK;

	VID_INFO*  pVid_info_front = (VID_INFO*)&pStreamArgs->vid_info_front;
	VID_INFO*  pVid_info_rear = (VID_INFO*)&pStreamArgs->vid_info_rear;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pVid_info_front->data_source != VID_SRC_NONE)
	{
		front_encode_stop();
	}

	if(pVid_info_rear->data_source != VID_SRC_NONE)
	{
		rear_encode_stop();
	}

	return nRet;
}

INT32S
cb_func_packer_stop(
	void
)
{
	INT32S nRet = STATUS_OK;

	VID_INFO*  pVid_info_front = (VID_INFO*)&pStreamArgs->vid_info_front;
	VID_INFO*  pVid_info_rear = (VID_INFO*)&pStreamArgs->vid_info_rear;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pVid_info_front->data_source != VID_SRC_NONE)
	{
		front_encode_engine_status_clear(pVid_info_front);
		nRet |= front_packer_stop();

		switch(pVid_info_front->vid_rec_type)
		{
			case VID_REC_H264:
				front_h264_encode_buf_release(pVid_info_front);
			break;
			case VID_REC_JPEG:
				front_jpeg_encode_buf_release(pVid_info_front);
			break;
			default:
				ERR_PRINT("[REAR] unknown video record type:%d\r\n",pVid_info_front->vid_rec_type);
		}

		front_encode_buf_release();
	}

	if(pVid_info_rear->data_source != VID_SRC_NONE)
	{
		rear_encode_engine_status_clear(pVid_info_rear);
		nRet |= rear_packer_stop();

		switch(pVid_info_rear->vid_rec_type)
		{
			case VID_REC_H264:
				rear_h264_encode_buf_release(pVid_info_rear);
			break;
			case VID_REC_JPEG:
				rear_jpeg_encode_buf_release(pVid_info_rear);
			break;
			default:
				ERR_PRINT("[REAR] unknown video record type:%d\r\n",pVid_info_rear->vid_rec_type);
		}

		rear_encode_buf_release();
	}

	return nRet;
}

INT32S
cb_func_pscaler_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==[%d]==\r\n",__FUNCTION__,src);

	nRet = lib_pscaler_process(src);

	return nRet;
}

INT32S cb_func_insertframe_process(INT16U src)
{
   INT32S nRet = STATUS_OK;
   nRet = lib_enc_dummywrite(src);
   return nRet;
}

INT32S
cb_func_H264_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==[%d]==\r\n",__FUNCTION__,src);

	nRet = lib_H264_process(src);

	return nRet;
}

INT32S
cb_func_JPEG_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = lib_JPEG_process(src);

	return nRet;
}

INT32S
cb_jpeg_capture_param_set(
	INT8U src,
	VID_INFO* args,
	VIDEO_DATA_FROMAT in_data_format,
	INT32U pscaler_bypass_out_cnt,
	INT32U jpeg_out_cnt,
	INT32U jpeg_out_size
)
{
	INT32S nRet = STATUS_OK;
	JpegEnc_t* pJpegEnc = &args->jpeg_rec_args.jpeg_cfg;
	FB_CFG* pPscalerOutCfg = &args->jpeg_rec_args.fb_pscaler_out_cfg;
	FB_CFG* pJpegOutCfg = &args->jpeg_rec_args.fb_jpeg_out_cfg;
	FB_CFG* pMsgNotifyCfg = &args->jpeg_rec_args.fb_msg_notify_cfg;
	INT8U photo_quality_setting;

	#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
	HOST_USB_MJPEG_DEC_ARGS* pMjpeg_dec_args = &args->host_usb_mjpeg_dec_args;
	#endif /*== HOST USB MJPEG CAM ==*/

	args->data_format = in_data_format;
	args->jpeg_rec_args.photo_jpeg_outbuf_change = 1; // default set 1 for getting the first jpeg encode output buffer
	args->jpeg_rec_args.photo_jpeg_write_out_size = 0; // default set 0

	// VID_SRC_USB do the following code in cb_h264_preview_buffer_set
	if(args->data_source != VID_SRC_USB)
	{
		// Init pscaler output buffer
		if(args->vid_rec_line)
		{	// FIFO mode
			pPscalerOutCfg->fb_cfg_size = args->sensor_width*args->vid_rec_line;
			if(args->data_format == VID_DATA_420)
			{
				pPscalerOutCfg->fb_cfg_size+= (args->sensor_width*args->vid_rec_line)>>1;
			}
			else
			{
				pPscalerOutCfg->fb_cfg_size+= args->sensor_width*args->vid_rec_line;
			}

			pPscalerOutCfg->fb_cfg_max_count = args->sensor_height/args->vid_rec_line;
			if(args->sensor_height != (pPscalerOutCfg->fb_cfg_max_count*args->vid_rec_line)) pPscalerOutCfg->fb_cfg_max_count++;
		}
		else // Frame mode
		{
			pPscalerOutCfg->fb_cfg_size = args->sensor_width*args->sensor_height;
			if(args->data_format == VID_DATA_420)
			{
				pPscalerOutCfg->fb_cfg_size+= (args->sensor_width*args->sensor_height)>>1;
			}
			else
			{
				pPscalerOutCfg->fb_cfg_size+= args->sensor_width*args->sensor_height;
			}

			pPscalerOutCfg->fb_cfg_max_count = 1;
		}

		// Init pscaler output buffer
		pPscalerOutCfg->fb_cfg_blocks = pscaler_bypass_out_cnt;
		pPscalerOutCfg->fb_cfg_head_addr = gp_malloc_align(pPscalerOutCfg->fb_cfg_blocks*pPscalerOutCfg->fb_cfg_size,64);
		if(pPscalerOutCfg->fb_cfg_head_addr == 0)
		{
			ERR_PRINT("fail to pscaler's fb_cfg_head_addr\r\n");
			RETURN(STATUS_FAIL);
		}

		// Init circular buffer of pscaler output buffer
		pPscalerOutCfg->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pPscalerOutCfg->fb_cfg_blocks,64);
		if(pPscalerOutCfg->fb_cfg_cb_addr == 0)
		{
			ERR_PRINT("fail to pscaler's fb_cfg_cb_addr\r\n");
			RETURN(STATUS_FAIL);
		}
	}

	// Init jpeg output buffer
	pJpegOutCfg->fb_cfg_size = jpeg_out_size;
	pJpegOutCfg->fb_cfg_blocks = jpeg_out_cnt;
	pJpegOutCfg->fb_cfg_head_addr = gp_malloc_align(pJpegOutCfg->fb_cfg_blocks*pJpegOutCfg->fb_cfg_size,64);
	if(pJpegOutCfg->fb_cfg_head_addr == 0)
	{
		ERR_PRINT("fail to pJpegOut's fb_cfg_head_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	pJpegEnc->scale_mode = JPEG_SCALE_NONE;

	if(args->data_source == VID_SRC_USB)
	{
		/*
			Set USB_HOST_CAM's JPEG HW_ID = GET_JPEG's JPEG HW_ID
		*/
		#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
		pJpegEnc->hwidx = pMjpeg_dec_args->jpeg_cfg.hwidx;
		#endif /*== HOST USB MJPEG CAM ==*/
	}
	else
	{
		pJpegEnc->hwidx = get_idle_jpeg(-1);
	}

	photo_quality_setting = ap_state_config_quality_get();
	switch(photo_quality_setting)
	{
		case PHOTO_QUALITY_BEST:
			pJpegEnc->quality_value = PHOTO_Q_VALUE_BEST;
			break;

		case PHOTO_QUALITY_NORMAL:
		default:
			pJpegEnc->quality_value = PHOTO_Q_VALUE_NORMAL;
			break;

		case PHOTO_QUALITY_LOW:
			pJpegEnc->quality_value = PHOTO_Q_VALUE_LOW;
			break;
	}

	pJpegEnc->yuv_fmt = args->data_format;
	pJpegEnc->yuv_sample_fmt = args->data_format;
	pJpegEnc->width = args->target_width;
	pJpegEnc->height = args->target_height;
	pJpegEnc->output_buffer = (INT32U)pJpegOutCfg->fb_cfg_head_addr;
	pJpegEnc->output_buffer_len = pJpegOutCfg->fb_cfg_size;

	if(lib_jpeg_header_create(pJpegEnc) == STATUS_FAIL)
	{
		ERR_PRINT("fail to lib_jpeg_header_create\r\n");
		RETURN(STATUS_FAIL);
	}

	// Init circular buffer of jpeg output buffer
	pJpegOutCfg->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pJpegOutCfg->fb_cfg_blocks,64);
	if(pJpegOutCfg->fb_cfg_cb_addr == 0)
	{
		ERR_PRINT("fail to pJpegOut's fb_cfg_cb_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	#if 0
	DBG_PRINT("[Pscaler] size:%d,blocks:%d,head_addr:0x%x cb_addr:0x%x\r\n",
	pPscalerOutCfg->fb_cfg_size,pPscalerOutCfg->fb_cfg_blocks,pPscalerOutCfg->fb_cfg_head_addr,pPscalerOutCfg->fb_cfg_cb_addr);

	DBG_PRINT("[Jpeg] size:%d,blocks:%d,head_addr:0x%x cb_addr:0x%x\r\n",
	pJpegOutCfg->fb_cfg_size,pJpegOutCfg->fb_cfg_blocks,pJpegOutCfg->fb_cfg_head_addr,pJpegOutCfg->fb_cfg_cb_addr);
	#endif

	// Init message notify
	pMsgNotifyCfg->fb_cfg_blocks = pJpegOutCfg->fb_cfg_blocks;
	pMsgNotifyCfg->fb_cfg_head_addr = gp_malloc_align(pMsgNotifyCfg->fb_cfg_blocks*sizeof(FB_INFO),64);
	if(pMsgNotifyCfg->fb_cfg_head_addr == 0)
	{
		ERR_PRINT("fail to pMsgNotifyCfg's fb_cfg_head_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	// Init circular buffer of message notify
	pMsgNotifyCfg->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pMsgNotifyCfg->fb_cfg_blocks,64);
	if(pMsgNotifyCfg->fb_cfg_cb_addr == 0)
	{
		ERR_PRINT("fail to pMsgNotifyCfg's fb_cfg_cb_addr\r\n");
		RETURN(STATUS_FAIL);
	}

	// Write jpeg header
	#if ((C_SENSOR_SAVE_RAW_DATA == CUSTOM_OFF) && (C_PHOTO_JPEG_ADD_EXIF == CUSTOM_OFF))
	ap_storage_service_write_file(args->rec_file_handle, (INT32U)pJpegEnc->header, jpeg_header_get_size());
	#endif

	return nRet;
Return:

	cb_jpeg_encode_buf_release(src,args);

	return nRet;
}

INT32S
front_jpeg_capture_param_set(
	VID_INFO* args
)
{
	return cb_jpeg_capture_param_set(
	PREVIEW_SRC_FRONT,
	args,
	VID_DATA_420,
	PHOTO_PSCALER_OUT_CNT,
	PHOTO_JPEG_OUT_CNT_FRONT,
	PHOTO_JPEG_OUT_SIZE_FRONT
	);
}

INT32S
rear_jpeg_capture_param_set(
	VID_INFO* args
)
{
	return cb_jpeg_capture_param_set(
	PREVIEW_SRC_REAR,
	args,
	VID_DATA_420,
	PHOTO_PSCALER_OUT_CNT,
	PHOTO_JPEG_OUT_CNT_REAR,
	PHOTO_JPEG_OUT_SIZE_REAR
	);
}

INT32S
cb_func_photo_start(
	void
)
{
	INT32S nRet = STATUS_OK;

	VID_INFO*  pVid_info_front = (VID_INFO*)&pStreamArgs->vid_info_front;
	VID_INFO*  pVid_info_rear = (VID_INFO*)&pStreamArgs->vid_info_rear;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	#if (_SENSOR_USB_MJPEG_CAM)
	if((pVid_info_front->data_source == VID_SRC_USB) ||
	   (pVid_info_rear->data_source == VID_SRC_USB)
	)
	{
		nRet = lib_host_2K_decode_JPEG_finish_check();
		if(nRet == STATUS_FAIL)
		{
			return STATUS_FAIL;
		}
	}
	#endif

	front_stream_init();
	rear_stream_init();

	if(pVid_info_front->data_source != VID_SRC_NONE)
	{
		pVid_info_front->add_time_stamp = ap_state_config_capture_date_stamp_get();

		nRet = front_jpeg_capture_param_set(pVid_info_front);
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("fail to front_jpeg_capture_param_set\r\n");
			RETURN(STATUS_FAIL);
		}

		nRet = front_photo_buf_set(encoderTaskQ,pVid_info_front);
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("fail to front_photo_buf_set\r\n");
			RETURN(STATUS_FAIL);
		}
	}

	if(back_sensor_plug_status_get() && (pVid_info_rear->data_source != VID_SRC_NONE))
	{
		pVid_info_rear->add_time_stamp = ap_state_config_capture_date_stamp_get();

		nRet = rear_jpeg_capture_param_set(pVid_info_rear);
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("fail to rear_jpeg_capture_param_set\r\n");
			RETURN(STATUS_FAIL);
		}

		nRet = rear_photo_buf_set(encoderTaskQ,pVid_info_rear);
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("fail to rear_photo_buf_set\r\n");
			RETURN(STATUS_FAIL);
		}
	}

	if(pVid_info_front->data_source != VID_SRC_NONE)
	{
		front_photo_start(pVid_info_front);
	}

	if(back_sensor_plug_status_get() && (pVid_info_rear->data_source != VID_SRC_NONE))
	{
		rear_photo_start(pVid_info_rear);
	}

Return:
	return nRet;
}

INT32S
cb_func_pscaler_scaleup_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = lib_pscaler_scaleup_process(src);

	return nRet;
}

INT32S
cb_func_photo_write_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = lib_photo_write_process(src);

	return nRet;
}

/*== SNAPSHOT ==*/
#if C_SUPORT_GET_JPG_BUF
INT32S
cb_func_get_jpeg_buf_release(
	INT8U src,
	GET_JPEG_ARGS* args
)
{
	FB_CFG* pMsgNotifyCfg = &args->fb_msg_notify_cfg;
	JpegEnc_t* pJpegEnc = &args->jpeg_cfg;
	VID_INFO*  pVid_info_front = (VID_INFO*)&pStreamArgs->vid_info_front;
	VID_INFO*  pVid_info_rear = (VID_INFO*)&pStreamArgs->vid_info_rear;
	if(args->flag_need_enc)
	{
		// Jpeg driver
		if(pJpegEnc->output_buffer)
		{
			gp_free((void *)pJpegEnc->output_buffer);
		}

		if(pJpegEnc->header)
		{
			gp_free(pJpegEnc->header);
		}
        if((src == PREVIEW_SRC_REAR)&&(pVid_info_rear->data_source != VID_SRC_USB))
        {
            DBG_PRINT("REAR STOP jpeg[%d]\r\n",pJpegEnc->hwidx);
            jpeg_encode_stop(pJpegEnc->hwidx);
            release_used_jpeg(pJpegEnc->hwidx);
        }
        else if((src == PREVIEW_SRC_FRONT)&&(pVid_info_front->data_source != VID_SRC_USB))
        {
            DBG_PRINT("Front STOP jpeg[%d]\r\n",pJpegEnc->hwidx);
            jpeg_encode_stop(pJpegEnc->hwidx);
            release_used_jpeg(pJpegEnc->hwidx);
        }

		lib_get_jpeg_buf_release(src,pMsgNotifyCfg);
	}

	return STATUS_OK;
}

INT32S
cb_func_get_jpeg_param_set(
	INT8U src,
	VID_INFO* args,
	VIDEO_DATA_FROMAT in_data_format,
	INT32U jpeg_out_size,
	INT8U do_enc
)
{
	INT32S nRet = STATUS_OK;
	GET_JPEG_ARGS* pGet_jpeg_args = &args->get_jpeg_args;
	FB_CFG* pMsgNotifyCfg = &pGet_jpeg_args->fb_msg_notify_cfg;
	JpegEnc_t* pGetJpegEnc = &pGet_jpeg_args->jpeg_cfg;
	INT32U jpegHeaderSize = jpeg_header_get_size();
	INT8U data_source = args->data_source;
	FB_CFG* pPscalerOutCfg;

	#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
	HOST_USB_MJPEG_DEC_ARGS* pMjpeg_dec_args = &args->host_usb_mjpeg_dec_args;
	#endif /*== HOST USB MJPEG CAM ==*/

	pGet_jpeg_args->flag_need_enc = do_enc;
	pGetJpegEnc->output_buffer = 0;

	// Init jpeg output buffer
	if(pGet_jpeg_args->flag_need_enc)
	{
		pPscalerOutCfg = &args->h264_rec_args.fb_pscaler_out_cfg;

		pGetJpegEnc->output_buffer_len = jpeg_out_size+jpegHeaderSize;
		pGetJpegEnc->output_buffer = (INT32U)gp_malloc_align(pGetJpegEnc->output_buffer_len,64);
		if(pGetJpegEnc->output_buffer == 0)
		{
			ERR_PRINT("fail to malloc pJpegEnc->output_buffer\r\n");
			RETURN(STATUS_FAIL);
		}

		pGetJpegEnc->scale_mode = JPEG_SCALE_NONE;
		pGetJpegEnc->quality_value = 50;
		pGetJpegEnc->yuv_fmt = in_data_format;
		pGetJpegEnc->yuv_sample_fmt = in_data_format;
		pGetJpegEnc->width = args->target_width;
		pGetJpegEnc->height = args->target_height;
		pGetJpegEnc->input_y_len = pPscalerOutCfg->fb_cfg_size;

		//DBG_PRINT("=>[%s]%x\r\n",__FUNCTION__,pGetJpegEnc->input_y_len);

		if(lib_jpeg_header_create(pGetJpegEnc) == STATUS_FAIL)
		{
			ERR_PRINT("fail to lib_jpeg_header_create\r\n");
			RETURN(STATUS_FAIL);
		}

		if(data_source == VID_SRC_USB)
		{
			/*
				Set USB_HOST_CAM's JPEG HW_ID = GET_JPEG's JPEG HW_ID
			*/
			#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
			pGetJpegEnc->hwidx = pMjpeg_dec_args->jpeg_cfg.hwidx;
			#endif /*== HOST USB MJPEG CAM ==*/
		}
		else
		{
			pGetJpegEnc->hwidx = get_idle_jpeg(-1);
		}

		//DBG_PRINT("[%d]pGetJpegEnc->hwidx :%d \r\n",src,pGetJpegEnc->hwidx);

		gp_memcpy((INT8S *)pGetJpegEnc->output_buffer, (INT8S *)pGetJpegEnc->header, jpegHeaderSize);

		if(lib_get_jpeg_buf_set(src,VID_ENC_MSG_NOTIFY_CNT,pMsgNotifyCfg) == STATUS_FAIL)
		{
			ERR_PRINT("fail to lib_get_jpeg_buf_set \r\n");
			RETURN(STATUS_FAIL);
		}
	}

	#if (C_VID_INTERPOLATING_2K)
	pGet_jpeg_args->flag_fifo_done = 0;
	#endif

	pGet_jpeg_args->flag_get_fail = 0;
	pGet_jpeg_args->flag_start_fifo_buf_get = 0;
	pGet_jpeg_args->flag_start_buf_get = 1;

	return nRet;
Return:

	cb_func_get_jpeg_buf_release(src,pGet_jpeg_args);
	return nRet;
}

INT32S
front_get_jpeg_param_set(
	VID_INFO* args,
	INT8U do_enc
)
{
	return cb_func_get_jpeg_param_set(
	PREVIEW_SRC_FRONT,
	args,
	VID_DATA_420,
	SNAPSHOT_JPEG_OUT_SIZE_FRONT,
	do_enc
	);
}

INT32S
rear_get_jpeg_param_set(
	VID_INFO* args,
	INT8U do_enc
)
{
	return cb_func_get_jpeg_param_set(
	PREVIEW_SRC_REAR,
	args,
	VID_DATA_420,
	SNAPSHOT_JPEG_OUT_SIZE_REAR,
	do_enc
	);
}

INT32S
cb_func_get_jpeg_on_encode(
	void
)
{
	INT32S nRet = STATUS_OK;

	VID_INFO*  pVid_info_front = (VID_INFO*)&pStreamArgs->vid_info_front;
	VID_INFO*  pVid_info_rear = (VID_INFO*)&pStreamArgs->vid_info_rear;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pVid_info_front->data_source != VID_SRC_NONE)
	{
		if(pVid_info_front->vid_rec_type == VID_REC_H264)
		{
			// Encoding H264 now, so encode pscaler's bypass buffer to a jpeg and write file
			front_get_jpeg_param_set(pVid_info_front,1);
        }
        else if(pVid_info_front->vid_rec_type == VID_REC_JPEG)
        {
            // Encoding JPEG now, so get this JPEG to write file
			front_get_jpeg_param_set(pVid_info_front,0);
		}
	}

	if(back_sensor_plug_status_get() && (pVid_info_rear->data_source != VID_SRC_NONE))
	{
		if(pVid_info_front->vid_rec_type == VID_REC_H264)
		{
			// Encoding H264 now, so encode pscaler's bypass buffer to a jpeg and write file
			rear_get_jpeg_param_set(pVid_info_rear,1);
        }
        else if(pVid_info_front->vid_rec_type == VID_REC_JPEG)
        {
			// Encoding JPEG now, so get this JPEG to write file
			rear_get_jpeg_param_set(pVid_info_rear,0);
		}
	}

	return nRet;
}

INT32S
cb_func_get_jpeg_enc_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = lib_get_jpeg_enc_process(src);

	return nRet;
}

INT32S
cb_func_get_jpeg_done_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = lib_get_jpeg_done_process(src);

	return nRet;
}
#endif

/*
 *  cpu_draw_time_osd
 *  Input:  ap_mode - STATE_VIDEO_RECORD or STATE_PHOTO_CAPTURE
 *          fifo_mode - 0: frame mode, 1: fifo mode
 *          draw_type - YUYV_DRAW or YUV420_DRAW
 *          state - 0: 1st fifo, 1: 2nd fifo, 2: 3rd fifo
 */
void cpu_draw_time_osd(INT32U buf, INT16U buf_w, INT16U buf_h, INT32U ap_mode, INT8U fifo_mode, INT8U draw_type, INT8U state)
{
    static CHAR osd_str[] = "0000/00/00 00:00:00";
    static STRING_ASCII_INFO ascii_str = {0};
    static TIME_T last_osd_time = {0};
    static INT8U last_dt_mode = 0xff;

    INT8U dt_mode, num_tmp0, num_tmp1;
    INT32S  year;
    CHAR    *ptr_y_hi, *ptr_y, *ptr_m, *ptr_d;
    TIME_T  osd_time;
    t_STR_SIZE_STRUCT str_res;

    #if C_WATER_MARK == CUSTOM_ON
    DISPLAY_ICONSHOW    water_mark;
    #endif

    ascii_str.buff_w = buf_w;
    ascii_str.buff_h = buf_h;
    ascii_str.pos_x = ap_state_resource_time_stamp_position_x_get();

	
    if(fifo_mode)
    {
        ascii_str.pos_y = 0;
    }
    else
    {
        ascii_str.pos_y = ap_state_resource_time_stamp_position_y_get();
    }

    if(ascii_str.pos_x < 0)     ascii_str.pos_x += buf_w;
    if(ascii_str.pos_y < 0)     ascii_str.pos_y += buf_h;
    ascii_str.pos_x &= ~0x0001;
    ascii_str.pos_y &= ~0x0001;

    ap_peripheral_osd_time_get(&osd_time);

    dt_mode = ap_state_config_date_time_mode_get();
    if(last_dt_mode != dt_mode)
    {
        last_dt_mode = dt_mode;

        gp_memset((INT8S *)&last_osd_time, 0, sizeof(last_osd_time));
        gp_strcpy((INT8S *)&osd_str[0], (INT8S *)"2000/00/00 00:00:00");
        osd_str[10] = 0;     //attention!!! space character needs to access spi flash, so split osd_str[] to two strings.

        ascii_str.font_color = WHITE_COLOR;
        ascii_str.font_type = REC_FONT_TYPE;
        ascii_str.font_off_h = 0;
        ascii_str.str_ptr = osd_str;
    }

    if(dt_mode == 0) // YY/MM/DD
    {
        ptr_y_hi = &osd_str[0];
        ptr_y = &osd_str[2];
        ptr_m = &osd_str[5];
        ptr_d = &osd_str[8];

        osd_str[4] = '/';
        osd_str[7] = '/';
    }
    else if(dt_mode == 1) // DD/MM/YY
    {
        ptr_d = &osd_str[0];
        ptr_m = &osd_str[3];
        ptr_y_hi = &osd_str[6];
        ptr_y = &osd_str[8];

        osd_str[2] = '/';
        osd_str[5] = '/';
    }
    else // MM/DD/YY
    {
        ptr_m = &osd_str[0];
        ptr_d = &osd_str[3];
        ptr_y_hi = &osd_str[6];
        ptr_y = &osd_str[8];

        osd_str[2] = '/';
        osd_str[5] = '/';
    }

	//YEAR
	#if C_AEAWB_INFO_STAMP
	osd_time.tm_year = 2000 + (ae_set_ifo.analog_gain/16);
    ptr_y_hi[0] = ae_set_ifo.is_night + 0x30;
	#endif

    if(last_osd_time.tm_year != osd_time.tm_year)
    {
        last_osd_time.tm_year = osd_time.tm_year;

        #if !C_AEAWB_INFO_STAMP
        ptr_y_hi[0] = '2';
        #endif

        ptr_y_hi[1] = '0';

        year = (osd_time.tm_year) - 2000;
        if(year > 99)
        {
            while(year > 99)
            {
                year -= 100;
                ptr_y_hi[1] += 1;
            }
        }

        num_tmp0 = (INT8U)year;
        num_tmp1 = num_tmp0 / 10;
        num_tmp0 -= num_tmp1 * 10;
        num_tmp1 += 0x30;
        num_tmp0 += 0x30;

        ptr_y[0] = num_tmp1;
        ptr_y[1] = num_tmp0;
    }

    // MONTH
    #if C_AEAWB_INFO_STAMP
    osd_time.tm_mon = ae_set_ifo.ae_target;
	if(osd_time.tm_mon < 0)
		osd_time.tm_mon = 99;
	#endif

    if(last_osd_time.tm_mon != osd_time.tm_mon)
    {
        last_osd_time.tm_mon = osd_time.tm_mon;

        num_tmp0 = (INT8U)(osd_time.tm_mon);
        num_tmp1 = num_tmp0 / 10;
        num_tmp0 -= num_tmp1 * 10;
        num_tmp1 += 0x30;
        num_tmp0 += 0x30;
        ptr_m[0] = num_tmp1;
        ptr_m[1] = num_tmp0;
    }

    // DAY
    #if C_AEAWB_INFO_STAMP
    osd_time.tm_mday = ae_set_ifo.satcontr_idx;
	if(osd_time.tm_mday < 0)
		osd_time.tm_mday = 99;
	#endif

    if(last_osd_time.tm_mday != osd_time.tm_mday)
    {
        last_osd_time.tm_mday = osd_time.tm_mday;

        num_tmp0 = (INT8U) (osd_time.tm_mday);
        num_tmp1 = num_tmp0 / 10;
        num_tmp0 -= num_tmp1 * 10;
        num_tmp1 += 0x30;
        num_tmp0 += 0x30;

        ptr_d[0] = num_tmp1;
        ptr_d[1] = num_tmp0;
    }

    // HOUR

    #if C_AEAWB_INFO_STAMP
	osd_time.tm_hour = ae_set_ifo.get_lum;// / 2;
	#endif

    if(last_osd_time.tm_hour != osd_time.tm_hour)
    {
        last_osd_time.tm_hour = osd_time.tm_hour;

        num_tmp0 = (INT8U) (osd_time.tm_hour);
        num_tmp1 = num_tmp0 / 10;
        num_tmp0 -= num_tmp1 * 10;
        num_tmp1 += 0x30;
        num_tmp0 += 0x30;

        osd_str[11] = num_tmp1;
        osd_str[12] = num_tmp0;
    }

    // MINUTE
    #if C_AEAWB_INFO_STAMP
	osd_time.tm_min = ae_set_ifo.sensor_ev_idx/100;
	#endif

    if(last_osd_time.tm_min != osd_time.tm_min)
    {
        last_osd_time.tm_min = osd_time.tm_min;

        num_tmp0 = (INT8U) (osd_time.tm_min);
        num_tmp1 = num_tmp0 / 10;
        num_tmp0 -= num_tmp1 * 10;
        num_tmp1 += 0x30;
        num_tmp0 += 0x30;

        osd_str[14] = num_tmp1;
        osd_str[15] = num_tmp0;
    }

	// SECOND
	#if C_AEAWB_INFO_STAMP
	osd_time.tm_sec = ae_set_ifo.sensor_ev_idx - 100 * (int)(ae_set_ifo.sensor_ev_idx/100);
	#endif

    if(last_osd_time.tm_sec != osd_time.tm_sec)
    {
        last_osd_time.tm_sec = osd_time.tm_sec;

        num_tmp0 = (INT8U)(osd_time.tm_sec);
        num_tmp1 = num_tmp0 / 10;
        num_tmp0 -= num_tmp1 * 10;
        num_tmp1 += 0x30;
        num_tmp0 += 0x30;

        osd_str[17] = num_tmp1;
        osd_str[18] = num_tmp0;
    }

    ascii_str.str_ptr = "8";
    ap_state_resource_ascii_string_resolution_get(&ascii_str, &str_res);
    ascii_str.str_ptr = osd_str;

    if(ap_mode == STATE_VIDEO_RECORD)
    {
        INT8U   draw_date_time = 1;
        #if C_WATER_MARK == CUSTOM_ON
        INT8U   draw_water_mark = 1;

        if(draw_type == YUV420_DRAW)
        {
            water_mark = water_mark_gp420;
        }
        else if(draw_type == YUYV_DRAW)
        {
            water_mark = water_mark_yuyv;
        }

        water_mark.pos_x = -ascii_str.pos_x - water_mark.icon_w;
        if(water_mark.pos_x < 0) water_mark.pos_x += buf_w;
        water_mark.pos_x &= ~0x0001;
        water_mark.pos_y = ascii_str.pos_y;
        #endif

        if(fifo_mode)
        {
            if(state == 0) //draw 1st fifo
            {
                ascii_str.font_off_h = 0;

                #if C_WATER_MARK == CUSTOM_ON
                water_mark.pos_y = 0;
                if(water_mark.icon_h > buf_h)
                {
                    water_mark.icon_h = buf_h;
                }
                #endif
            }
            else if(state == 1) //draw 2nd fifo
            {
                if(str_res.string_height > buf_h)
                {
                    ascii_str.font_off_h = buf_h;
                }
                else
                {
                    draw_date_time = 0;
                }

                #if C_WATER_MARK == CUSTOM_ON
                water_mark.pos_y = 0;
                if(water_mark.icon_h > buf_h)
                {
                    water_mark.icon_h -= buf_h;
                    if(water_mark.icon_h > buf_h)
                    {
                        water_mark.icon_h = buf_h;
                    }

                    if(draw_type == YUV420_DRAW)
                    {
                        water_mark.data += water_mark.icon_w * buf_h * 3 >> 2;  //INT16U *
                    }
                    else
                    {
                        water_mark.data += water_mark.icon_w * buf_h;           //INT16U *
                    }
                }
                else
                {
                    draw_water_mark = 0;
                }
                #endif
            }
            else if(state == 2) //draw 3rd fifo
            {
                if(str_res.string_height > (buf_h * 2)) // need 3 fifo
                {
                    ascii_str.font_off_h = buf_h * 2;
                }
                else
                {
                    draw_date_time = 0;
                }

                #if C_WATER_MARK == CUSTOM_ON
                water_mark.pos_y = 0;
                if(water_mark.icon_h > (buf_h * 2))
                {
                    water_mark.icon_h -= (buf_h * 2);
                    if(water_mark.icon_h > buf_h)
                    {
                        water_mark.icon_h = buf_h;
                    }

                    if(draw_type == YUV420_DRAW)
                    {
                        water_mark.data += water_mark.icon_w * buf_h * 3 >> 1;  //INT16U *
                    }
                    else
                    {
                        water_mark.data += water_mark.icon_w * buf_h << 1;      //INT16U *
                    }
                }
                else
                {
                    draw_water_mark = 0;
                }
                #endif
            }
        }

        #if C_WATER_MARK == CUSTOM_ON
        if(ap_state_config_plate_number_onoff_get() && draw_water_mark)
        {
            if(draw_type == YUV420_DRAW)
            {
                ap_display_icon_draw_gp420((INT16U *)buf, buf_w, buf_h, &water_mark, 0);
            }
            else if(draw_type == YUYV_DRAW)
            {
                ap_display_icon_draw_yuyv((INT16U *)buf, buf_w, buf_h, &water_mark, 0);
            }
        }
        #endif

        if(draw_date_time)
        {
            ap_state_resource_string_ascii_draw((INT16U *)buf, &ascii_str, draw_type, DRAW_BLACK_EDGE_OFF, DRAW_UV_OFF);     //date
            ascii_str.pos_x += str_res.string_width;
            ascii_str.str_ptr = &osd_str[11];
            ap_state_resource_string_ascii_draw((INT16U *)buf, &ascii_str, draw_type, DRAW_BLACK_EDGE_OFF, DRAW_UV_OFF);     //time
        }
        cache_drain_range(buf, (draw_type == YUV420_DRAW) ? (buf_w * buf_h * 3 >> 1) : (buf_w * buf_h * 2));
    }
    else if(ap_mode == STATE_PHOTO_CAPTURE) //consider frame mode only
    {
        ap_state_resource_string_ascii_draw((INT16U *)buf, &ascii_str, draw_type, DRAW_BLACK_EDGE_OFF, DRAW_UV_OFF);     //date

        if(ap_state_config_capture_date_stamp_get() == 2) // 1 : DATE_ONLY,   2 : DATE & TIME
        {
            ascii_str.pos_x += str_res.string_width;
            ascii_str.str_ptr = &osd_str[11];
            ap_state_resource_string_ascii_draw((INT16U *)buf, &ascii_str, draw_type, DRAW_BLACK_EDGE_OFF, DRAW_UV_OFF);    //time
        }
        cache_drain_range(buf, (draw_type == YUV420_DRAW) ? (buf_w * buf_h * 3 >> 1) : (buf_w * buf_h * 2));
    }
}

/*== WIFI ==*/
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
INT32S
wifi_h264_encode_param_set(
	VID_INFO* args
)
{
	ENCODE_PARAMS h264_param = {
	0, // Use display buffer ,so set pscaler to 0
	VID_DATA_420,
	WIFI_H264_OUT_I_CNT,
	WIFI_H264_OUT_I_SIZE,
	WIFI_H264_OUT_P_CNT,
	WIFI_H264_OUT_P_SIZE,
	0,  // don't need skip frame , so set 0
	0,  // don't need skip frame , so set 0
	WIFI_REC_BITRATE_RATE,
	WIFI_GOP_LEN,
	WIFI_REC_FRAME_RATE,
	WIFI_H264_QP_START,
	WIFI_H264_QP_MAX,
	WIFI_H264_QP_MIN,
	WIFI_H264_HIERPFRAME,
	#if (C_VID_INTERPOLATING_2K)
	0
	#endif
	};

	return cb_h264_encode_param_set(PREVIEW_SRC_WIFI,args,&h264_param);
}

INT32S
wifi_jpeg_encode_param_set(
	VID_INFO* args
)
{
	ENCODE_PARAMS jpeg_param = {
	VID_PSCALER_OUT_CNT_FRONT,
	VID_DATA_420,
	VID_JPEG_OUT_CNT_FRONT,
	VID_JPEG_OUT_SIZE_FRONT,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	#if (C_VID_INTERPOLATING_2K)
	0
	#endif
	};

	return cb_jpeg_encode_param_set(PREVIEW_SRC_FRONT,args,&jpeg_param);
}

static INT32S
wifi_encode_resolution_set(
	VID_INFO* pVid_info_wifi
)
{

	pVid_info_wifi->vid_rec_type = wifi_encode_type_get();
	pVid_info_wifi->run_ap_mode = STATE_WIFI_TRANSMISSION;
	pVid_info_wifi->vid_rec_line = 0; // Frame mode

	pVid_info_wifi->sensor_width = drv_l2_user_sensor_width_get(PREVIEW_SRC_REAR);
	pVid_info_wifi->sensor_height = drv_l2_user_sensor_height_get(PREVIEW_SRC_REAR);

	pVid_info_wifi->target_width = wifi_display_width_get();
	pVid_info_wifi->target_height = wifi_display_height_get();
	DBG_PRINT("wifi_resolution:%d,%d,%d,%d\r\n", pVid_info_wifi->sensor_width,pVid_info_wifi->sensor_height,pVid_info_wifi->target_width,pVid_info_wifi->target_height);
	return STATUS_OK;
}

INT32S
cb_func_wifi_enc_start(
	void
)
{
    static INT8U buffer_alloc = 1;
	INT32S nRet = STATUS_OK;

	VID_INFO*  pVid_info_wifi = &pStreamArgs->vid_info_wifi;

//	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
		wifi_encode_stop_req_clear();
#endif

    if(buffer_alloc)
    {
        memset(pVid_info_wifi,0,sizeof(VID_INFO));

        wifi_encode_resolution_set(pVid_info_wifi);

        switch(pVid_info_wifi->vid_rec_type)
        {
            case VID_REC_H264:
                wifi_h264_encode_param_set(pVid_info_wifi);
            break;
            case VID_REC_JPEG:
                wifi_jpeg_encode_param_set(pVid_info_wifi);
            break;
            default:
                ERR_PRINT("[FRONT] unknown video record type:%d\r\n",pVid_info_wifi->vid_rec_type);
                RETURN(STATUS_FAIL);
        }
        buffer_alloc = 0;
    }

	wifi_stream_init();

	nRet = wifi_encode_buf_set(encoderTaskQ,pVid_info_wifi);
	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to wifi_encode_start\r\n");
		RETURN(STATUS_FAIL);
	}

	wifi_encode_start(pVid_info_wifi);

Return:
	return nRet;
}

INT32S
wifi_h264_encode_buf_release(
	VID_INFO* args
)
{
	return cb_h264_encode_buf_release(PREVIEW_SRC_WIFI,args);
}

INT32S
wifi_jpeg_encode_buf_release(
	VID_INFO* args
)
{
	return cb_jpeg_encode_buf_release(PREVIEW_SRC_WIFI,args);
}

INT32S
cb_func_wifi_enc_stop(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = wifi_encode_stop();

	return nRet;
}

INT32S
cb_func_wifi_enc_buf_release(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

    #if 0
	VID_INFO*  pVid_info_wifi = &pStreamArgs->vid_info_wifi;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	switch(pVid_info_wifi->vid_rec_type)
	{
		case VID_REC_H264:
			wifi_h264_encode_buf_release(pVid_info_wifi);
		break;
		case VID_REC_JPEG:
			wifi_jpeg_encode_buf_release(pVid_info_wifi);
		break;
		default:
			ERR_PRINT("[REAR] unknown video record type:%d\r\n",pVid_info_wifi->vid_rec_type);
	}
    #endif

	wifi_encode_buf_release();

	return nRet;
}

#endif //#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)

/*== UVC ==*/
#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
void
uvc_connect_status_set(
	INT8U flag_cnt
)
{
	flag_uvc_connect = flag_cnt;
}

INT8U
uvc_connect_status_get(
	void
)
{
	return flag_uvc_connect;
}

INT32S
uvc_h264_encode_param_set(
	VID_INFO* args
)
{
	ENCODE_PARAMS h264_param = {
	0, // Use display buffer ,so set pscaler to 0
	VID_DATA_420,
	UVC_H264_OUT_I_CNT,
	UVC_H264_OUT_I_SIZE,
	UVC_H264_OUT_P_CNT,
	UVC_H264_OUT_P_SIZE,
	0,  // don't need skip frame , so set 0
	0,  // don't need skip frame , so set 0
	UVC_REC_BITRATE_RATE,
	UVC_GOP_LEN,
	UVC_REC_FRAME_RATE,
	UVC_H264_QP_START,
	UVC_H264_QP_MAX,
	UVC_H264_QP_MIN,
	UVC_H264_HIERPFRAME,
	#if (C_VID_INTERPOLATING_2K)
	0
	#endif
	};

	return cb_h264_encode_param_set(PREVIEW_SRC_UVC,args,&h264_param);
}

INT32S
uvc_jpeg_encode_param_set(
	VID_INFO* args
)
{
	ENCODE_PARAMS jpeg_param = {
	0, // Use display buffer ,so set pscaler to 0
	VID_DATA_420,
	UVC_JPEG_OUT_CNT,
	UVC_JPEG_OUT_SIZE,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	#if (C_VID_INTERPOLATING_2K)
	0
	#endif
	};

	return cb_jpeg_encode_param_set(PREVIEW_SRC_UVC,args,&jpeg_param);
}

static INT32S
uvc_encode_resolution_set(
	VID_INFO* pVid_info_uvc
)
{
	pVid_info_uvc->vid_rec_type = uvc_encode_type_get();
	pVid_info_uvc->run_ap_mode = STATE_CONNECT_TO_PC;
	pVid_info_uvc->vid_rec_line = 0; // Frame mode

	pVid_info_uvc->target_width = usbd_uvc_display_width_get();
	pVid_info_uvc->target_height = usbd_uvc_display_height_get();

	return STATUS_OK;
}

INT32S
uvc_jpeg_encode_Q_init(
	VID_INFO* args
)
{
	INT32U	target_width = args->target_width;
	JengDynaQ_t* pJpeg_dyna_q = &args->jpeg_rec_args.jpeg_q_cfg;
	INT32U encSizMax;
	INT32U encSizMid;
	INT32U encSizMin;
	INT8U  emerY;
	INT8U  maxY;
	INT8U  minY;
	INT8U  step;

	if((target_width == UVC_VID_WIDTH_720p) || (target_width == UVC_VID_WIDTH_VGA) || (target_width == UVC_VID_WIDTH_QVGA))
	{
		encSizMax = UVC_JPEG_OUT_SIZE - (5 * 1024);
		encSizMid = (200 * 1024);
		encSizMin = (120 * 1024);
		emerY = Q_EMER_720P;
		maxY = Q_MAX_720P;
		minY = Q_MIN_720P;
	}
	else{
		encSizMax = UVC_JPEG_OUT_SIZE - (5 * 1024);
		encSizMid = (200 * 1024);
		encSizMin = (120 * 1024);
		emerY = Q_EMER_720P;
		maxY = Q_MAX_720P;
		minY = Q_MIN_720P;
        DBG_PRINT("UVC jpeg target_width not match, set Q param to default(720P)!!!!\r\n");
	}

	step = Q_STEP;

	vid_dyna_tune_Q_init(pJpeg_dyna_q,
	encSizMax,
	encSizMid,
	encSizMin,
	emerY,
	maxY,
	minY,
	step);
    DBG_PRINT("%s, max:%d, emer:%d, min:%d, step:%d\r\n", __FUNCTION__, pJpeg_dyna_q->max_q.Y_q_table, pJpeg_dyna_q->emer_q.Y_q_table, pJpeg_dyna_q->min_q.Y_q_table, step);
	return STATUS_OK;
}

INT32S
cb_func_uvc_enc_start(
	void
)
{
	INT32S nRet = STATUS_OK;

	VID_INFO*  pVid_info_uvc = &pStreamArgs->vid_info_uvc;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	memset(pVid_info_uvc,0,sizeof(VID_INFO));

	uvc_stream_init();
	uvc_encode_resolution_set(pVid_info_uvc);

	switch(pVid_info_uvc->vid_rec_type)
	{
		case VID_REC_H264:
			uvc_h264_encode_param_set(pVid_info_uvc);
		break;
		case VID_REC_JPEG:
			uvc_jpeg_encode_param_set(pVid_info_uvc);
			uvc_jpeg_encode_Q_init(pVid_info_uvc);
		break;
		default:
			ERR_PRINT("[FRONT] unknown video record type:%d\r\n",pVid_info_uvc->vid_rec_type);
			RETURN(STATUS_FAIL);
	}

	nRet = uvc_encode_buf_set(encoderTaskQ,pVid_info_uvc);
	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to uvc_encode_buf_set\r\n");
		RETURN(STATUS_FAIL);
	}

	uvc_connect_status_set(1);

	uvc_encode_start(pVid_info_uvc);

Return:
	return nRet;
}

INT32S
uvc_h264_encode_buf_release(
	VID_INFO* args
)
{
	return cb_h264_encode_buf_release(PREVIEW_SRC_UVC,args);
}

INT32S
uvc_jpeg_encode_buf_release(
	VID_INFO* args
)
{
	return cb_jpeg_encode_buf_release(PREVIEW_SRC_UVC,args);
}

INT32S
cb_func_uvc_enc_stop(
	void
)
{
	INT32S nRet = STATUS_OK;

	VID_INFO*  pVid_info_uvc = &pStreamArgs->vid_info_uvc;

	uvc_connect_status_set(0);
	nRet = uvc_encode_stop();

	switch(pVid_info_uvc->vid_rec_type)
	{
		case VID_REC_H264:
			uvc_h264_encode_buf_release(pVid_info_uvc);
		break;
		case VID_REC_JPEG:
			uvc_jpeg_encode_buf_release(pVid_info_uvc);
		break;
		default:
			ERR_PRINT("[REAR] unknown video record type:%d\r\n",pVid_info_uvc->vid_rec_type);
	}

	uvc_encode_buf_release();

	return nRet;
}
#endif //#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)

#if C_TIME_LAPSE
INT32S
time_lapse_check(
	INT8U src
)
{
    if (g_time_lapse)
    {

        if(src == PREVIEW_SRC_FRONT)
        {
            time_lapse_current_time = xTaskGetTickCount();
            time_lapse_delta_time = (INT32S)time_lapse_current_time - (INT32S)time_lapse_last_os_time;
            time_lapse_delta_time /= 1000;
            if(!time_lapse_last_os_time || (time_lapse_delta_time >= g_time_lapse))
            {
                time_lapse_last_os_time += time_lapse_delta_time*1000;
                DBG_PRINT("@front time lapse:%d second\r\n", time_lapse_current_time/1000);
            }
            else
            {
                return STATUS_FAIL;
            }
        }
        else
        {
            time_lapse_current_time_rear = xTaskGetTickCount();
            time_lapse_delta_time_rear = (INT32S)time_lapse_current_time_rear - (INT32S)time_lapse_last_os_time_rear;
            time_lapse_delta_time_rear /= 1000;
            if(!time_lapse_last_os_time_rear || (time_lapse_delta_time_rear >= g_time_lapse))
            {
                time_lapse_last_os_time_rear += time_lapse_delta_time_rear*1000;
                DBG_PRINT("@rear time lapse:%d second\r\n", time_lapse_current_time_rear/1000);
            }
            else
            {
                return STATUS_FAIL;
            }
        }
    }
    return STATUS_OK;
}

void ap_time_lapse_tick_clean(void)
{
    time_lapse_current_time = 0;
    time_lapse_last_os_time = 0;
    time_lapse_delta_time = 0;
    time_lapse_current_time_rear = 0;
    time_lapse_last_os_time_rear = 0;
    time_lapse_delta_time_rear = 0;
}
#endif

/*== Save Raw ==*/
#if C_SENSOR_SAVE_RAW_DATA
INT32S
cb_func_sensor_write_raw_process(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = lib_sensor_write_raw_process(PREVIEW_SRC_FRONT);

	return nRet;
}
#endif

/**************************************************************************/
#if GPS_INFO_PACK
	INT32U frame_count[2] = {0};
	static GPSInfo_t gps_info[2];
#endif
#if GPS_INFO_PACK_2
	INT8U* gps_tmp[2] = {0};
#endif

#if GPS_INFO_PACK
INT32S mov_create_gps_info(INT32U handle, char* Id, INT8U type, INT8U packerID)
{
	MP4DataBoxInfo_t box[2];
    INT32U temp_addr = 0;
	if (gps_info[packerID].InfoBlk)
	{
		temp_addr = (INT32U)gps_info[packerID].InfoBlk;
		//gp_free(gps_info[packerID].InfoBlk);
    }
	gp_memset((INT8S *)&gps_info[packerID], 0, sizeof (GPSInfo_t));

	strcpy(gps_info[packerID].chidentification, Id);
	gps_info[packerID].type = type;
	gps_info[packerID].maxCount = GPS_MAX_COUNT;
	if(temp_addr!=0)
	{
       gps_info[packerID].InfoBlk =  (INT8U*)temp_addr;
	}
	else
	{
        #if NEW_HEADER
        gps_info[packerID].InfoBlk = (INT8U*)gp_malloc(GPS_INFO_BLK_SIZE+8);
        #else
        gps_info[packerID].InfoBlk = (INT8U*)gp_malloc(GPS_INFO_BLK_SIZE);
        #endif
    }
	if (!gps_info[packerID].InfoBlk)
	{
		DBG_PRINT("[%s] out of memory.\r\n", __FUNCTION__);
		return -1;
	}

	gp_memset((INT8S *)&gps_info[packerID].InfoBlk[0], 0, GPS_INFO_BLK_SIZE);
	#if NEW_HEADER
	gp_memcpy((INT8S *)(gps_info[packerID].InfoBlk+4), (INT8S *)"skip", 4);
	gp_memcpy((INT8S *)(gps_info[packerID].InfoBlk+8), (INT8S *)&gps_info[packerID].chidentification[0], 15);
	gps_info[packerID].InfoBlk[15+8] = type;
	cache_drain_range((INT32U)gps_info[packerID].InfoBlk,GPS_INFO_BLK_SIZE+8);
	#else
	gp_memcpy((INT8S *)&gps_info[packerID].InfoBlk[0], (INT8S *)&gps_info[packerID].chidentification[0], 15);
	gps_info[packerID].InfoBlk[15] = type;
	cache_drain_range((INT32U)gps_info[packerID].InfoBlk,GPS_INFO_BLK_SIZE);
	#endif

	gp_memcpy((INT8S *)&box[0].name, (INT8S *)"skip", 4);
	box[0].level = 0;
	box[0].type = Mp4DataAppendMovEnd;
	box[1].name = 0; //mark end of box
	muxMP4SetDataBox(handle, &(box[0]));
	return 0;
}

INT32S mov_add_gps_data(INT8U type, INT32U num, INT8U* data, INT8U packerID)
{
	INT32U idx;

	if(gps_info[packerID].InfoBlk == 0)
	{
        return -1;
	}

	if (gps_info[packerID].nCount >= gps_info[packerID].maxCount)
	{
		DBG_PRINT("[%s][ERR] out of memory.\r\n", __FUNCTION__);
	}

	idx = 20 + gps_info[packerID].nCount*132;
	#if NEW_HEADER
	idx += 8;
	#endif
	if (type == 0)
	{
		*(INT32U *)(&gps_info[packerID].InfoBlk[idx]) = num;
		gp_memcpy((INT8S *)&gps_info[packerID].InfoBlk[idx+4], (INT8S *)data, 128);
	}
	else
	{
		gp_strcpy((INT8S *)&gps_info[packerID].InfoBlk[idx], (INT8S *)"####");
		if (num > 124)
			num = 124;
		*(INT32U *)(&gps_info[packerID].InfoBlk[idx+4]) = num;
		gp_memcpy((INT8S *)&gps_info[packerID].InfoBlk[idx+8], (INT8S *)data, num);
	}
	gps_info[packerID].nCount++;
	#if NEW_HEADER
	*(INT32U*)(&gps_info[packerID].InfoBlk[16+8]) = gps_info[packerID].nCount;
	#else
	*(INT32U*)(&gps_info[packerID].InfoBlk[16]) = gps_info[packerID].nCount;
    #endif
	return 0;
}

INT32S mov_write_gps_info(INT32U handle, gpMux_t* p, INT8U packerID)
{
	gpMuxPkt_t pkt;

	if (!p)
	{
		DBG_PRINT("[%s] invalid parameter.\r\n", __FUNCTION__);
		return -1;
	}

	DBG_PRINT("->%d,%x\r\n",packerID,&gps_info[packerID].InfoBlk[0]);

	if (gps_info[packerID].nCount)
	{
		pkt.data = &gps_info[packerID].InfoBlk[0];
		pkt.size = 20 + 132*gps_info[packerID].nCount;
		#if NEW_HEADER
		pkt.size += 8;
		#endif
		gp_memcpy((INT8S *)&pkt.pts, (INT8S *)"skip", 4);
		p->pack(handle, &pkt, GP_ES_TYPE_DATA);
	}

	//gp_free(gps_info[packerID].InfoBlk);
	//gps_info[packerID].InfoBlk = 0;

	return 0;
}
unsigned char chInput1[128] = "2014/04/24 10:20:34 N:22.587164 E:113.871490 29.47km/h x:+0.000 y:-0.000 z:+0.000";
unsigned char chInput2[128] = "2015/04/24 10:20:34 N:22.587164 E:113.871490 29.47km/h x:+0.000 y:-0.000 z:+0.000";
#endif

#if GPS_INFO_PACK_2
INT32S mov_create_gps_info2(INT32U handle, char* Id)
{
	MP4DataBoxInfo_t box[2];

	gp_memcpy(&box[0].name, Id, 4);
	box[0].level = 0;
	box[0].type = Mp4DataNormal;
	box[1].name = 0; //mark end of box
	muxMP4SetDataBox(handle, &(box[0]));
	return 0;
}

INT32S mov_write_gps_info2(INT32U handle, gpMux_t* p, INT8U* data, INT32U size, INT8U* name)
{
	gpMuxPkt_t pkt;

	if (!p || !size)
	{
		DBG_PRINT("[%s] invalid parameter.\r\n", __FUNCTION__);
		return -1;
	}

	pkt.data = data;
	pkt.size = size;
	gp_memcpy(&pkt.pts, name, 4);
	p->pack(handle, &pkt, GP_ES_TYPE_DATA);

	return 0;
}

#endif

void mov_gps_info_init(INT8U packerID)
{
#if GPS_INFO_PACK
	frame_count[packerID] = 0;
	gps_info[packerID].InfoBlk = 0;
#endif

#if GPS_INFO_PACK_2
	gps_tmp[packerID] = (INT8U*)gp_malloc(GPS_MAX_COUNT);
	if(gps_tmp[packerID] == NULL)
	{
		DBG_PRINT("->[ERR] gps_tmp is null! \r\n");
	}
	gp_memset(gps_tmp[packerID], 0, GPS_MAX_COUNT);
	gp_memcpy(gps_tmp[packerID], "GPS ", 4);
#endif
}

void mov_gps_info_uninit(INT8U packerID)
{
    #if GPS_INFO_PACK
	if(gps_info[packerID].InfoBlk)
	{
        DBG_PRINT("GPS FREE[%d,0x%x]\r\n",packerID,gps_info[packerID].InfoBlk);
        gp_free(gps_info[packerID].InfoBlk);
        gps_info[packerID].InfoBlk = 0;
	}
    #endif
    #if GPS_INFO_PACK_2
	if(gps_tmp[packerID])
	{
        gp_free(gps_tmp[packerID]);
        gps_tmp[packerID] = 0;
	}
    #endif

}

void mov_gps_info_create(INT32U handle, INT8U packerID)
{
#if GPS_INFO_PACK
	frame_count[packerID] = 0;
	mov_create_gps_info(handle, "LIGOGPSINFO", 2,packerID);
#endif
#if GPS_INFO_PACK_2
	mov_create_gps_info2(handle, "free");
#endif
}

void mov_gps_frame_add(INT8U packerID)
{
#if GPS_INFO_PACK
	frame_count[packerID]++;
#endif
}

void mov_gps_data_add(INT8U packerID)
{
#if GPS_INFO_PACK
    if(packerID == 0)
	mov_add_gps_data(0, frame_count[packerID], chInput1, packerID); //type 0
	else
	mov_add_gps_data(0, frame_count[packerID], chInput2, packerID); //type 0
#endif
}

/*
writeTime: 0: end of MOV header 1: write after audio
*/
void mov_gps_data_write(INT32U handle, gpMux_t* pMovPacker, INT8U packerID, INT32U offsetSize, INT8U writeTime)
{
#if GPS_INFO_PACK
	if(writeTime)
	{
		return;
	}
	mov_write_gps_info(handle, pMovPacker, packerID);
#endif

#if GPS_INFO_PACK_2
	if(writeTime == 0)
	{
		return;
	}
	mov_write_gps_info2(handle, pMovPacker, 0, 0x20000 - offsetSize - 8, "free");
	mov_write_gps_info2(handle, pMovPacker, gps_tmp[packerID], GPS_MAX_COUNT, "free");
#endif
}

/*== HOST USB MJPEG CAM / INTERP2K ==*/
#if (_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K)
INT32S
cb_h264_preview_buffer_set(
	INT32U src,
	VID_INFO* args,
	ENCODE_PARAMS* pEnc_params
)
{
	INT32S nRet = STATUS_OK;
	FB_CFG* pPscalerOutCfg = &args->h264_rec_args.fb_pscaler_out_cfg;
	INT32U  height_16_alignment; //16 alignment for hw constrain

	args->data_format = pEnc_params->in_data_format;

	// Init pscaler output buffer
	if(args->vid_rec_line)
	{	// FIFO mode
		pPscalerOutCfg->fb_cfg_size = args->target_width*args->vid_rec_line;
		if(args->data_format == VID_DATA_420)
		{
			pPscalerOutCfg->fb_cfg_size+= (args->target_width*args->vid_rec_line)>>1;
		}
		else
		{
			pPscalerOutCfg->fb_cfg_size+= args->target_width*args->vid_rec_line;
		}

		pPscalerOutCfg->fb_cfg_max_count = args->target_height/args->vid_rec_line;
		if(args->target_height != (pPscalerOutCfg->fb_cfg_max_count*args->vid_rec_line)) pPscalerOutCfg->fb_cfg_max_count++;
	}
	else // Frame mode
	{
		/* 16 alignment */
		height_16_alignment = (args->target_height + 0xF) >> 4 << 4;

		pPscalerOutCfg->fb_cfg_size = args->target_width*height_16_alignment;
		if(args->data_format == VID_DATA_420)
		{
			pPscalerOutCfg->fb_cfg_size+= (args->target_width*height_16_alignment)>>1;
		}
		else
		{
			pPscalerOutCfg->fb_cfg_size+= args->target_width*height_16_alignment;
		}

		pPscalerOutCfg->fb_cfg_max_count = 1;
	}

	//DBG_PRINT("=>[%s]src:%d, max:%d \r\n",__FUNCTION__,src,pPscalerOutCfg->fb_cfg_max_count);

	if(pEnc_params->pscaler_bypass_out_cnt)
	{
		pPscalerOutCfg->fb_cfg_blocks = pEnc_params->pscaler_bypass_out_cnt;
		pPscalerOutCfg->fb_cfg_head_addr = gp_malloc_align(pPscalerOutCfg->fb_cfg_blocks*pPscalerOutCfg->fb_cfg_size,64);
		if(pPscalerOutCfg->fb_cfg_head_addr == 0)
		{
			ERR_PRINT("fail to pscaler's fb_cfg_head_addr\r\n");
			RETURN(STATUS_FAIL);
		}

		#if 0 // For debug
		memset(pPscalerOutCfg->fb_cfg_head_addr,0,(pPscalerOutCfg->fb_cfg_blocks*pPscalerOutCfg->fb_cfg_size));
		#endif

		// Pscaler output's circular_buffer
		pPscalerOutCfg->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pPscalerOutCfg->fb_cfg_blocks,64);
		if(pPscalerOutCfg->fb_cfg_cb_addr == 0)
		{
			ERR_PRINT("fail to pscaler's fb_cfg_cb_addr\r\n");
			RETURN(STATUS_FAIL);
		}
	}

	return STATUS_OK;

Return:
	return nRet;
}

INT32S
front_h264_preview_buffer_set(
	VID_INFO* args
)
{
	ENCODE_PARAMS h264_param = {
	VID_PSCALER_OUT_CNT_FRONT,
	args->data_format,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	#if (C_VID_INTERPOLATING_2K)
	0
	#endif
	};

	return cb_h264_preview_buffer_set(PREVIEW_SRC_FRONT,args,&h264_param);
}

INT32S
rear_h264_preview_buffer_set(
	VID_INFO* args
)
{
	ENCODE_PARAMS h264_param = {
	VID_PSCALER_OUT_CNT_REAR,
	args->data_format,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	#if (C_VID_INTERPOLATING_2K)
	0
	#endif
	};

	return cb_h264_preview_buffer_set(PREVIEW_SRC_REAR,args,&h264_param);
}

INT32S
cb_jpeg_preview_buffer_set(
	INT32U src,
	VID_INFO* args,
	ENCODE_PARAMS* pEnc_params
)
{
	INT32S nRet = STATUS_OK;
	JpegEnc_t* pJpegEnc = &args->jpeg_rec_args.jpeg_cfg;
	FB_CFG* pPscalerOutCfg = &args->jpeg_rec_args.fb_pscaler_out_cfg;
	INT32U final_width,final_height;

	args->data_format = pEnc_params->in_data_format;

	pJpegEnc->scale_mode = JPEG_SCALE_NONE;
	final_width = args->sensor_width;
	final_height =  args->sensor_height;

	if(args->run_ap_mode == STATE_VIDEO_RECORD)
	{
		if(args->sensor_width != args->target_width)
		{
			if(args->data_format == VID_DATA_420)
			{
				// target width / sensor width = 1.5
				if((args->target_width*2) == (args->sensor_width*3))
				{
					pJpegEnc->scale_mode = JPEG_SCALE_GP420_X3D2;
				}
	            else if(args->target_width == args->sensor_width*2)
				{
	                pJpegEnc->scale_mode = JPEG_SCALE_X2;
				}
	            DBG_PRINT("JPEG Encode Scaler Up:[%d,%d]\r\n",args->sensor_width,args->target_width);
			}
		}

		if(pJpegEnc->scale_mode)
		{
			final_width = args->sensor_width;
			final_height =  args->sensor_height;
		}
		else
		{
			final_width = args->target_width;
			final_height = args->target_height;
		}
	}

	// Init pscaler output buffer
	if(args->vid_rec_line)
	{	// FIFO mode
		pPscalerOutCfg->fb_cfg_size = final_width*args->vid_rec_line;
		if(args->data_format == VID_DATA_420)
		{
			pPscalerOutCfg->fb_cfg_size+= (final_width*args->vid_rec_line)>>1;
		}
		else
		{
			pPscalerOutCfg->fb_cfg_size+= final_width*args->vid_rec_line;
		}

		pPscalerOutCfg->fb_cfg_max_count = final_height/args->vid_rec_line;
		if(final_height & (args->vid_rec_line -1)) // find the remainder
		{
			pPscalerOutCfg->fb_cfg_max_count++;
		}
	}
	else // Frame mode
	{
		pPscalerOutCfg->fb_cfg_size =final_width*final_height;
		if(args->data_format == VID_DATA_420)
		{
			pPscalerOutCfg->fb_cfg_size+= (final_width*final_height)>>1;
		}
		else
		{
			pPscalerOutCfg->fb_cfg_size+= final_width*final_height;
		}

		pPscalerOutCfg->fb_cfg_max_count = 1;
	}

	if(pEnc_params->pscaler_bypass_out_cnt)
	{
		pPscalerOutCfg->fb_cfg_blocks = pEnc_params->pscaler_bypass_out_cnt;
		pPscalerOutCfg->fb_cfg_head_addr = gp_malloc_align(pPscalerOutCfg->fb_cfg_blocks*pPscalerOutCfg->fb_cfg_size,64);
		if(pPscalerOutCfg->fb_cfg_head_addr == 0)
		{
			ERR_PRINT("fail to pscaler's fb_cfg_head_addr\r\n");
			RETURN(STATUS_FAIL);
		}

		pPscalerOutCfg->fb_cfg_cb_addr = gp_malloc_align(sizeof(void*)*pPscalerOutCfg->fb_cfg_blocks,64);
		if(pPscalerOutCfg->fb_cfg_cb_addr == 0)
		{
			ERR_PRINT("fail to pscaler's fb_cfg_cb_addr\r\n");
			RETURN(STATUS_FAIL);
		}
	}

	return STATUS_OK;

Return:
	return nRet;
}

INT32S
front_jpeg_preview_buffer_set(
	VID_INFO* args
)
{
	ENCODE_PARAMS jpeg_param = {
	VID_PSCALER_OUT_CNT_FRONT,
	args->data_format,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	#if (C_VID_INTERPOLATING_2K)
	0
	#endif
	};

	return cb_jpeg_preview_buffer_set(PREVIEW_SRC_FRONT,args,&jpeg_param);
}

INT32S
rear_jpeg_preview_buffer_set(
	VID_INFO* args
)
{
	ENCODE_PARAMS jpeg_param = {
	VID_PSCALER_OUT_CNT_REAR,
	VID_DATA_420,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	#if (C_VID_INTERPOLATING_2K)
	0
	#endif
	};

	return cb_jpeg_preview_buffer_set(PREVIEW_SRC_REAR,args,&jpeg_param);
}

INT32S
host_2K_preview_buffer_set(
	INT32U src,
	VID_INFO* args
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==%d==\r\n",__FUNCTION__,src);

	if(src == PREVIEW_SRC_FRONT)
	{
		switch(args->vid_rec_type)
		{
			case VID_REC_H264:
				front_h264_preview_buffer_set(args);
			break;
			case VID_REC_JPEG:
				front_jpeg_preview_buffer_set(args);
			break;
			default:
				ERR_PRINT("[FRONT] unknown video record type:%d\r\n",args->vid_rec_type);
				RETURN(STATUS_FAIL);
		}

		nRet = front_preview_buffer_set(args);
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("fail to front_preview_buffer_set\r\n");
			RETURN(STATUS_FAIL);
		}

	}
	else if (src == PREVIEW_SRC_REAR)
	{
		switch(args->vid_rec_type)
		{
			case VID_REC_H264:
				rear_h264_preview_buffer_set(args);
			break;
			case VID_REC_JPEG:
				rear_jpeg_preview_buffer_set(args);
			break;
			default:
				ERR_PRINT("[REAR] unknown video record type:%d\r\n",args->vid_rec_type);
				RETURN(STATUS_FAIL);
		}

		nRet = rear_preview_buffer_set(args);
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("fail to rear_preview_buffer_set\r\n");
			RETURN(STATUS_FAIL);
		}
	}

Return:
	return nRet;
}

INT32S
cb_func_host_2K_decode_JPEG_process(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = lib_host_2K_decode_JPEG_process();

	return nRet;
}

INT32S
cb_func_host_2K_frame_ready_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = lib_host_2K_frame_ready_process(src);

	return nRet;
}

INT32S
cb_func_host_2K_preview_done_process(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	nRet = lib_host_2K_preview_done_process(src);

	return nRet;
}
#endif

/*== INTERPOLATING 2K ==*/
#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
INT32S
cb_func_interp2K_scaleup_process(
	INT8U src // It's PREVIEW_SRC_FRONT
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==[%d]==\r\n",__FUNCTION__,src);

	nRet = lib_interp2K_scaleup_process(src);

	return nRet;
}
#endif


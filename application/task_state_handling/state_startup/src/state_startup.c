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
#include "application.h"

#if defined(C_AUDIO_POWER_ON) && C_AUDIO_POWER_ON
static INT8U audio_done;
#endif

#if C_GSENSOR_FUNCTION
INT8U gsensor_power_on;
#endif

extern INT8U gWDog_Reset;
extern INT8S video_record_sts;


/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern INT32S jpeg_buffer_decode_and_scale(IMAGE_DECODE_STRUCT *img_decode_struct, INT32U clip_xy, INT32U clip_size);

#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
extern INT32S state_usbh_uvc_do_init(void);
#endif

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/



/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
INT8U Check_PowerOn_Battery_Value(void)
{
	INT32U	battery_value;
	INT8U	dc_in;
	INT8U	power_low = 0;

	drv_l1_adc_init(0,0);
	drv_l1_adc_vref_enable_set(TRUE);
	drv_l1_adc_conv_time_sel(4);
	drv_l1_adc_manual_ch_set(BATTERY_DETECT_PIN);
	osDelay(50);
	R_ADC_MADC_CTRL |= 0x70;					/* start manual sample */
	while(!(R_ADC_MADC_CTRL & 0x8000));

	/* wait ready */
	R_ADC_MADC_CTRL |= 0x8000;

	battery_value = R_ADC_MADC_DATA >> 4;		// 0=0V, 1023=3.3V

	#if !defined ADP_OUT_PIN
		DBG_PRINT("[ERR] Not define ADP_OUT_PIN %s:%d",__func__,__LINE__);

		dc_in = 0;
	#else
		#if (ADP_PIN_TYPE == READ_FROM_GPIO)
			dc_in = gpio_read_io(ADP_OUT_PIN);
		#else
			dc_in = drv_l1_sys_pwr_key_read(ADP_OUT_PIN);
	 	#endif
	#endif

	DBG_PRINT("!!!!battery_value = %d,dc_in = %d\n", battery_value, dc_in);

	//if( ((dc_in==1)&&((battery_value>2250)||(battery_value<1250))) || ((dc_in==0)&&(battery_value>2200)) )
	if((battery_value <= 2319) && (dc_in == 0)) //這裡數值需要根據實際量測到的低電壓值做調整；如果電壓低於3.6就關機
	{
		power_low = 0;
	}
	else
	{
		power_low = 1;
	}

	return power_low;
}

void state_startup_init(void)
{
	#if C_LOGO_POWER_ON
	INT16U tft_w = drv_l2_display_tft_width_get();
	INT16U tft_h = drv_l2_display_tft_height_get();
	IMAGE_DECODE_STRUCT img_info;
	INT32U	size;
	INT16U	logo_fd;
	INT32U	jpg_buf = 0;
	DISP_P	display_frame = NULL;
	INT32U	display_buf = 0;
	#endif

	DBG_PRINT("%s ... \r\n", __func__);

	audio_vol_set(ap_state_config_volume_get());

	#if C_AUDIO_POWER_ON

	#if C_RTC_REBOOT
	if(gWDog_Reset)
	{
		++audio_done;
	}
	#endif // #if C_RTC_REBOOT

	#if C_BATTERY_LOW_POWER_OFF
	if (!Check_PowerOn_Battery_Value())
	{
       	DBG_PRINT("Critical low power off\r\n");
		drv_l2_display_stop();
		#if C_GSENSOR_FUNCTION == CUSTOM_ON
        {
            INT8U gsensor_level;

            gsensor_level = ap_state_config_park_mode_G_sensor_get();
            drv_l2_gsensor_park_mode_init(gsensor_level);
        }
		#endif
		drv_l1_sys_power_handler();
	}
    #endif

	if(!audio_done && audio_effect_play(VOICE_ID_POWER_ON))
	{
		++audio_done;
	}
	#endif // #if defined(C_AUDIO_POWER_ON) && C_AUDIO_POWER_ON == CUSTOM_ON

	#if C_LOGO_POWER_ON
	do
	{

		#if C_RTC_REBOOT
		if(gWDog_Reset)
		{
			break;
		}
		#endif

		logo_fd = nv_open((INT8U *) "POWER_ON_LOGO.JPG");
		if(logo_fd == 0xFFFF)
		{
			DBG_PRINT("open POWER_ON_LOGO.JPG fail.\r\n");
			break;
		}

		size = nv_rs_size_get(logo_fd);
		if(!size)
		{
			DBG_PRINT("State startup get logo size fail.[%d]\r\n", size);
			break;;
		}

		jpg_buf = (INT32U) gp_malloc(size);
		if(!jpg_buf)
		{
			DBG_PRINT("State startup allocate jpeg input buffer fail.[0x%X]\r\n", jpg_buf);
			break;;
		}

		if(nv_read(logo_fd, (INT32U) jpg_buf, size))
		{
			DBG_PRINT("Failed to read resource_header in ap_startup_init\r\n");
			break;
		}

		display_frame = ap_display_queue_get(display_free_queue);
		if(display_frame == NULL)
		{
			DBG_PRINT("State startup allocate display buffer fail.\r\n");
			break;
		}
		else
		{
			display_frame->offset = 0;
			display_buf = display_frame->base + display_frame->offset;
		}

		img_info.image_source = (INT32S) jpg_buf;
		img_info.source_size = size;
		img_info.source_type = TK_IMAGE_SOURCE_TYPE_BUFFER;

		if(drv_l2_display_tft_fmt_get() == DISP_FMT_RGB565)
		{
			img_info.output_format = C_SCALER_CTRL_OUT_RGB565;
		}
		else if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
		{
			img_info.output_format = C_SCALER_CTRL_OUT_GP420;
		}
		else
		{
			img_info.output_format = C_SCALER_CTRL_OUT_YUYV;
		}

		img_info.output_ratio = 0;
		img_info.out_of_boundary_color = 0x008080;

		img_info.output_buffer_width = tft_w;
		img_info.output_buffer_height = tft_h;
		img_info.output_image_width = tft_w;
		img_info.output_image_height = tft_h;

		img_info.output_buffer_pointer = display_buf;

		if(jpeg_buffer_decode_and_scale(&img_info , 0, 0) == STATUS_FAIL)
		{
			DBG_PRINT("State startup decode jpeg file fail.\r\n");
			break;
		}
		display_task_msg_send(MSG_DISPLAY_TASK_JPEG_DRAW, &display_frame, sizeof(void *), MSG_PRI_NORMAL);

		display_frame = NULL;
	} while(0);

	if(jpg_buf)
	{
		gp_free((void *) jpg_buf);
		vTaskDelay(50);
	}

	if(display_frame != NULL)
	{
		ap_display_queue_put(display_free_queue, display_frame);
	}

	gplib_jpeg_default_huffman_table_load(DECODE_USE_JPEG_HW_IDX); // LOGO may change the huffman table, reload it
	#endif // #if C_LOGO_POWER_ON

	#if C_ASYN_DBLE_TFT
	drv_l2_asyn_double_tft_backlight_en_set(TRUE,FALSE);		/* display1 set TRUE ; display2 set FALSE */
	#else
	drv_l2_tft_backlight_en_set(TRUE);
	#endif

}

void state_startup_exit(void)
{
	#if C_GSENSOR_FUNCTION
	INT8U temp;
	#endif

	DBG_PRINT("%s ... \r\n", __func__);

	#if C_GSENSOR_FUNCTION
	temp = ap_state_config_record_G_sensor_get();
	drv_l2_gsensor_sensitive_set(temp);
	temp = drv_l2_gsensor_power_on_status_get();
	if(temp)
	{	// g_sensor power on
		peripheral_task_msg_send(MSG_PERIPHERAL_TASK_G_SENSOR_POWER_ON_START, NULL, 0, MSG_PRI_NORMAL);
		gsensor_power_on = 1;
	}
	else
	{
		gsensor_power_on = 0;
	}
	#endif // #if C_GSENSOR_FUNCTION

	#if C_SENSOR_SAVE_RAW_DATA
	state_handling_msg_send(STATE_PHOTO_CAPTURE, NULL, 0, MSG_PRI_NORMAL);
	#else
	state_handling_msg_send(STATE_VIDEO_RECORD, NULL, 0, MSG_PRI_NORMAL);
	//state_handling_msg_send(STATE_PHOTO_CAPTURE, NULL, 0, MSG_PRI_NORMAL);
	//state_handling_msg_send(STATE_BROWSE, NULL, 0, MSG_PRI_NORMAL);
	#endif

	/*== HOST USB MJPEG CAM ==*/
	#if (_SENSOR_USB_MJPEG_CAM)
    if(
		drv_l2_sensor_get_ops(0) ||
		drv_l2_sensor_get_ops(1) 
	)
    {
		state_usbh_uvc_do_init();
    }
	#else // AHD
    if(drv_l2_sensor_get_ops(1)) // Check rear device exists
    {
    	peripheral_task_msg_send(MSG_PERIPHERAL_BACK_SENSOR_POLLING_START, NULL, 0, MSG_PRI_NORMAL); // For AHD
    }	
	#endif

    #if C_USB_DEVICE
    peripheral_task_msg_send(MSG_PERIPHERAL_TASK_USBD_DETECT_INIT, NULL, 0, MSG_PRI_NORMAL);
    #endif

}

void
state_startup_entry(
	void *para
)
{
	#if defined(C_AUDIO_POWER_ON) && C_AUDIO_POWER_ON
	STAudioConfirm	*audio_confirm;
	EXIT_FLAG_ENUM	exit_flag = EXIT_RESUME;
	INT32U			msg_id;
	//INT32U			pend_msg = 0;
	INT8U			storage_id;
	#endif

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	ap_startup_init();

	ap_music_effect_resource_init();

	state_startup_init();

#if !C_DISABLE_SENSOR_TFT
		drv_l2_user_sensor_init(DUMMY_BUFFER_ADDRS, DUMMY_BUFFER_ADDRS);
#endif


	
	DBG_PRINT("%s ... \r\n", __func__);

	#if defined(C_AUDIO_POWER_ON) && C_AUDIO_POWER_ON

	while(exit_flag == EXIT_RESUME)
	{
		if (msgQReceive(ApQ, &msg_id, ApQ_para, ApQ_para_len) == STATUS_FAIL)
		{
			continue;
		}

		switch(msg_id)
		{

		case EVENT_APQ_ERR_MSG:
			audio_confirm = (STAudioConfirm *) ApQ_para;
			if(audio_confirm->result_type == MSG_AUD_PLAY_RES && audio_confirm->result != AUDIO_ERR_NONE)
			{
				++audio_done;
			}

			break;
		case MSG_STORAGE_SERVICE_MOUNT:
		case MSG_STORAGE_SERVICE_NO_STORAGE:
			//pend_msg	= msg_id;
			storage_id	= ApQ_para[0];
			ap_state_handling_storage_id_set(storage_id);
			break;

		#if C_BATTERY_DETECT
		case MSG_APQ_BATTERY_LVL_SHOW:
			ap_state_handling_battery_icon_show(ApQ_para[0]);
			break;

		case MSG_APQ_BATTERY_CHARGED_SHOW:
			ap_state_handling_charge_icon_show(1);
			break;

		case MSG_APQ_BATTERY_CHARGED_CLEAR:
			ap_state_handling_charge_icon_show(0);
			break;
		#endif // #if C_BATTERY_DETECT

		#if defined(C_FIRMWARE_UPGRADE) && C_FIRMWARE_UPGRADE
		case MSG_STORAGE_FIRMWARE_UPGRADE:
			state_handling_msg_send( STATE_FIRMWARE_UPGRADE, NULL, 0, MSG_PRI_NORMAL);
			break;
		#endif // defined(C_FIRMWARE_UPGRADE) && C_FIRMWARE_UPGRADE

		default:
			break;
		}

		if(audio_done /*&& pend_msg*/)  //wait pend_msg if need to wait card mounting
		{
			exit_flag = EXIT_BREAK;
		}
	}

	#endif // defined(C_AUDIO_POWER_ON) && C_AUDIO_POWER_ON  == CUSTOM_ON

	state_startup_exit();
}

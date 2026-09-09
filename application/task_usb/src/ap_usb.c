
/*********************************************************************
* Include file
**********************************************************************/
#include "application.h"
#include "ap_usb.h"

/**************************************************************************
* CONSTANTS
**************************************************************************/

#define C_USB_CAM_STATE_STACK_SIZE	2048

#define USB_CAM_QUEUE_MAX_LEN	    20

#define USB_MODE_CHARGING	        0
#define USB_MODE_CONNECT_PC         1

#define USB_HOST_DETECTED_CNT	    1000	/* 1000ms */

#define USBD_MSDC_DMA_BUF_ORDER	    4 // 6 // should not set more than 6
#define USBD_MSDC_DMA_BUF_SIZE	    ((1 << USBD_MSDC_DMA_BUF_ORDER)*512)	/* Support 512/1K/2K/4K/8K/16K/32K/64K/128K data length */

#define USBD_STORAGE_NO_WPROTECT	0
#define USBD_STORAGE_WPROTECT		1

#define UVC_ACK_QUEUE_MAX_LEN		1

INT8U *scsi_inquirydata_ptr[MAX_MSDC_LUN_NUM] = {NULL};

enum
{
	USBD_ENGINE_STATUS_IDLE			= 0,
	USBD_ENGINE_STATUS_SEND_VIDEO,
	USBD_ENGINE_STATUS_SEND_AUDIO
};

extern INT32S lib_free_JPEG(INT32U src,INT32U data,INT32U frameType);
void state_usbd_status_set(INT8U status);
INT8U state_usbd_status_get(void);
/**************************************************************************
* MACROS
**************************************************************************/

/**************************************************************************
 * DATA TYPES
 **************************************************************************/
UVC_SEND_BUF_DONE uvc_send_buf_done = NULL;

/**************************************************************************
* GLOBAL DATA
**************************************************************************/
static INT8U msdc_mode_flag = USB_MODE_CHARGING;

static INT32U send_video = 0;
static INT32U send_audio = 0;

INT32U uvc_send_buf;
INT32U uvc_send_buf_size;
INT32U uvc_ret_send_buf;

#if (defined C_USB_AUDIO) && (C_USB_AUDIO)
INT32U uac_send_buf;
INT32U uac_send_buf_size;
INT32U uac_ret_send_buf;
INT8U gstart_audio_uac = 0;
static INT32U uac_send_iso_buf;
static INT32U uac_send_iso_buf_size;
#endif


MSG_Q_ID	UVCTaskQ;
MSG_Q_ID    UVCTaskAckQ=NULL;


osThreadId 		UVCThreadId;

INT8U gstart_video_uvc = 0;
INT8U usbd_status = 0;

/**************************************************************************
* EXTERN DATA
**************************************************************************/
// MSDC
extern INT8U Default_Device_Descriptor_TBL[];
extern INT8U Default_Config_Descriptor_TBL[];
extern INT8U Default_Qualifier_Descriptor_TBL[];
extern INT8U Default_String0_Descriptor[];
extern INT8U Default_String1_Descriptor[];
extern INT8U Default_String2_Descriptor[];
extern INT8U Default_String3_Descriptor[];
extern INT8U Default_scsi_inquirydata[];
extern INT8U Default_scsi_inquirydata_CDROM[];
extern INT8U Default_TOC_PMA_ATIP_Format[];

// SD
extern const MDSC_LUN_STORAGE_DRV gp_msdc_ramdisk;
extern const MDSC_LUN_STORAGE_DRV gp_msdc_sd1;

//UVC
extern INT8U USB_UVC_MJPEG_DeviceDescriptor[];
extern INT8U USB_UVC_MJPEG_Qualifier_Descriptor_TBL[];
extern INT8U USB_UVC_MJPEG_ConfigDescriptor[];
extern INT8U UVC_MJPEG_String0_Descriptor[];
extern INT8U UVC_MJPEG_String1_Descriptor[];
extern INT8U UVC_MJPEG_String2_Descriptor[];
extern INT8U UVC_MJPEG_String3_Descriptor[];
extern INT8U UVC_MJPEG_String4_Descriptor[];
extern INT8U UVC_MJPEG_String5_Descriptor[];


extern INT8U USB_UVC_UC_DeviceDescriptor[];
extern INT8U USB_UVC_UC_Qualifier_Descriptor_TBL[];
extern INT8U USB_UVC_UC_ConfigDescriptor[];
extern INT8U UVC_UC_String0_Descriptor[];
extern INT8U UVC_UC_String1_Descriptor[];
extern INT8U UVC_UC_String2A_Descriptor[];
extern INT8U UVC_UC_String2B_Descriptor[];
extern INT8U UVC_UC_String3_Descriptor[];
extern INT8U UVC_UC_String4_Descriptor[];
extern INT8U UVC_UC_String5_Descriptor[];

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
extern INT8U gwifi_transmission_stop;
#endif
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern INT32S video_encode_state_msg_send(INT32U msg, void *para, INT32U bytes, INT32U prio);
extern INT32S video_encode_state_msg_send_with_ack(INT32U msg, INT32U msc_timeout);
extern INT32S video_encode_state_get(VIDEO_ENCODE_STATUS_OFFSET stateOffset);
extern void video_encode_state_set(VIDEO_ENCODE_STATUS_OFFSET stateOffset);

extern void drv_l2_usbd_misc_handle(INT32U usb_id, INT32U event);

void usbd_uvc_encode_buf_cbk(UVC_SEND_BUF_DONE cbk);
UVC_USER_VID_DESC * user_frm_desc_init(void);

#if (defined C_USB_AUDIO) && (C_USB_AUDIO)
static INT32S usb_audio_iso_send(INT32U new_buf, INT32U new_size);
#endif

/*************************************************************************/

/*
 *	usb_mode_detect_callback
 */
void usb_mode_detect_callback(void)
{
	drv_l1_usbd_soft_disconnect(USBD_USE_USB_HW_IDX);
	msdc_mode_flag = USB_MODE_CONNECT_PC;
}

INT32U usb_device_is_connected_to_pc(void)
{
	return ((msdc_mode_flag == USB_MODE_CONNECT_PC)?1:0);
}

/*************************************************************************/

/*
 *	usb_mode_detect
 */
INT32S usb_mode_detect(void)
{
	INT32S	ret = STATUS_OK;
	INT32U	time_current = 0;

#if 0
		app_msg_send(MSG_APQ_CONNECT_TO_PC, NULL, 0, MSG_PRI_NORMAL);
#else

	msdc_mode_flag = USB_MODE_CHARGING;

	/* Enable USB PHY and switch to device mode */
	drv_l1_usbd_phy_clk_on(USBD_USE_USB_HW_IDX);

	osDelay(10);
#if 0
	/* Init control state handler */
	drv_l2_usbd_register_state_handler(USBD_XFER_CONTROL, drv_l2_usbd_state_ctl_ep0_setup, NULL, NULL);

	drv_l2_usbd_register_state_handler(USBD_XFER_MISC, drv_l2_usbd_misc_handle, NULL, NULL);
#else
	/* Init control state handler */
	drv_l2_usbd_register_state_handler(USBD_XFER_CONTROL, drv_l2_usbd_state_ctl_ep0_setup, drv_l2_usbd_state_ctl_ep0_in, drv_l2_usbd_state_ctl_ep0_out);

	/* reset the control block of control transfer */
	gp_memset((INT8S*)&ctl_ctlblk[USBD_USE_USB_HW_IDX], 0, sizeof(ctl_ctlblk[USBD_USE_USB_HW_IDX]));

	/* Register default descriptor pointer */
	ctl_ctlblk[USBD_USE_USB_HW_IDX].dev_desptr = Default_Device_Descriptor_TBL;
	ctl_ctlblk[USBD_USE_USB_HW_IDX].config_desptr = Default_Config_Descriptor_TBL;
	ctl_ctlblk[USBD_USE_USB_HW_IDX].dev_qualifier_desptr = Default_Qualifier_Descriptor_TBL;
	ctl_ctlblk[USBD_USE_USB_HW_IDX].report_desptr = NULL;

	ctl_ctlblk[USBD_USE_USB_HW_IDX].str0_desptr = Default_String0_Descriptor;
	ctl_ctlblk[USBD_USE_USB_HW_IDX].str1_desptr = Default_String1_Descriptor;
	ctl_ctlblk[USBD_USE_USB_HW_IDX].str2_desptr = Default_String2_Descriptor;
	ctl_ctlblk[USBD_USE_USB_HW_IDX].str3_desptr = Default_String3_Descriptor;

	/* default interface_num is 0 */
	ctl_ctlblk[USBD_USE_USB_HW_IDX].cur_interface_num = 0;
	/* Init miscellaneous function handle*/
	drv_l2_usbd_register_state_handler(USBD_XFER_MISC, drv_l2_usbd_misc_handle, NULL, NULL);
#endif
	drv_l2_usbd_register_detect_cbk((void *) usb_mode_detect_callback);

	drv_l1_usbd_set_l1tol2_msg_type(1);

	/* Init USBD L1 register layer */
	ret = drv_l1_usbd_init(USBD_USE_USB_HW_IDX);
	if(ret == STATUS_FAIL)
	{
		DBG_PRINT("drv_l1_usbd_init failed!\r\n");
		return ret;
	}

	/* Set PHY config */
	drv_l1_usbd_phy_config(USBD_USE_USB_HW_IDX, USB_PHY_SETTING);

	/* register USBD ISR handler */
	drv_l1_usbd_enable_isr(USBD_USE_USB_HW_IDX);

	/* Check  Seup command from pc */
	while(msdc_mode_flag == USB_MODE_CHARGING)
	{
		if(time_current++ > USB_HOST_DETECTED_CNT)
			break;
		osDelay(1);
	}

	drv_l1_usbd_disable_isr(USBD_USE_USB_HW_IDX);

	drv_l2_usbd_register_state_handler(USBD_XFER_CONTROL, NULL, NULL, NULL);

	drv_l2_usbd_register_detect_cbk(NULL);

	drv_l1_usbd_set_l1tol2_msg_type(0);

	if(msdc_mode_flag == USB_MODE_CONNECT_PC)
	{
		app_msg_send(MSG_APQ_CONNECT_TO_PC, NULL, 0, MSG_PRI_NORMAL);
	}
	else
	{
		drv_l1_usbd_uninit(USBD_USE_USB_HW_IDX);
	}
#endif
	return ret;
}

/*************************************************************************/

/*
 *	usb_msdc_init
 */
INT32S usb_msdc_init(void)
{
	INT32S	ret = STATUS_OK;
    INT32U dma_buf_size = USBD_MSDC_DMA_BUF_SIZE;

	/* Enable USB PHY and switch to device mode */
	drv_l1_usbd_phy_clk_on(USBD_USE_USB_HW_IDX);

	/* Init USBD L2 protocol layer first, including control/bulk/ISO/interrupt transfers */
	/******************************* Control transfer ************************************/
	ret = drv_l2_usbd_ctl_init(USBD_USE_USB_HW_IDX);
	if(ret == STATUS_FAIL)
	{
		DBG_PRINT("drv_l2_usbd_ctl_init failed!\r\n");
		return ret;
	}

	/* Register new descriptor table here, this action must be done after drv_l2_usbd_ctl_init() */
	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX,REG_DEVICE_DESCRIPTOR_TYPE, (INT8U *) Default_Device_Descriptor_TBL);
	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX,REG_CONFIG_DESCRIPTOR_TYPE, (INT8U *) Default_Config_Descriptor_TBL);
	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX,REG_DEVICE_QUALIFIER_DESCRIPTOR_TYPE, (INT8U *) Default_Qualifier_Descriptor_TBL);
	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX,REG_STRING0_DESCRIPTOR_TYPE, (INT8U *) Default_String0_Descriptor);
	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX,REG_STRING1_DESCRIPTOR_TYPE, (INT8U *) Default_String1_Descriptor);
	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX,REG_STRING2_DESCRIPTOR_TYPE, (INT8U *) Default_String2_Descriptor);
    drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX,REG_STRING3_DESCRIPTOR_TYPE, (INT8U*)Default_String3_Descriptor);

	if(ap_state_handling_storage_id_get() != STORAGE_DEVICE)
	{
		drv_l2_usbd_msdc_set_lun(USBD_USE_USB_HW_IDX,LUN_RAM_DISK_TYPE, LUN_NUM_0, USBD_STORAGE_NO_WPROTECT, &gp_msdc_ramdisk);
	}
	else
	{
		drv_l2_usbd_msdc_set_lun(USBD_USE_USB_HW_IDX,LUN_SDC_TYPE, LUN_NUM_0, USBD_STORAGE_NO_WPROTECT, &gp_msdc_sd1);
	}

	drv_l2_usbd_msdc_set_dma_buffer_size(dma_buf_size);

	/* Init MSDC driver */
	ret = drv_l2_usbd_msdc_init(USBD_USE_USB_HW_IDX);
	if(ret == STATUS_FAIL)
	{
		/* Init failed, do uninit procedures */
		drv_l2_usbd_msdc_uninit(USBD_USE_USB_HW_IDX);
		DBG_PRINT("drv_l2_usbd_msdc_uninit failed!\r\n");
		return ret;
	}

	/* Register SCSI inquiry data pointer, it must be done after drv_l2_usbd_msdc_init() */
    scsi_inquirydata_ptr[0] = (INT8U *)Default_scsi_inquirydata;
	drv_l2_usbd_msdc_register_scsi_inquiry_data(USBD_USE_USB_HW_IDX, scsi_inquirydata_ptr, (INT8U *) Default_scsi_inquirydata_CDROM);
    drv_l2_usbd_msdc_register_scsi_MMC_TOC_PMA_ATIP_data(USBD_USE_USB_HW_IDX,(INT8U *)Default_TOC_PMA_ATIP_Format);

	/* Init USBD L1 register layer */
	ret = drv_l1_usbd_init(USBD_USE_USB_HW_IDX);
	if(ret == STATUS_FAIL)
	{
		DBG_PRINT("drv_l1_usbd_init failed!\r\n");
		return ret;
	}

	/* Set PHY config */
	#if 0 //???
	drv_l1_usbd_phy_config();
	#endif

	/* register USBD ISR handler */
	drv_l1_usbd_enable_isr(USBD_USE_USB_HW_IDX);
	DBG_PRINT("USB MSDC device init completed!\r\n");

	return ret;
}

/*************************************************************************/

/*
 *	usb_msdc_exit
 */
void usb_msdc_exit(void)
{
	drv_l1_usbd_uninit(USBD_USE_USB_HW_IDX);
	drv_l2_usbd_ctl_uninit(USBD_USE_USB_HW_IDX);
	drv_l2_usbd_msdc_uninit(USBD_USE_USB_HW_IDX);
}

/*************************************************************************/

/*
 *	_usbd_ep5_send_done_cbk
 */
static void _usbd_ep5_send_done_cbk(INT32U usb_id)
{
	INT32U msg = USBD_SEND_VIDEO_DONE_EVENT;

	if(msgQSend(UVCTaskQ, msg, NULL, 0, MSG_PRI_NORMAL) != osOK)
    {
		DBG_PRINT("EP5 done cbk send event to UVCTaskQ failed\r\n");
	}
}

/*************************************************************************/

/*
 *	_usbd_ep7_send_done_cbk
 */
static void _usbd_ep7_send_done_cbk(void)
{
    INT32U msg = USBD_SEND_AUDIO_DONE_EVENT;

	if(msgQSend(UVCTaskQ, msg, NULL, 0, MSG_PRI_NORMAL)!= osOK)
    {
		DBG_PRINT("EP7 done cbk send event to UVCTaskQ failed\r\n");
	}
}

/*************************************************************************/

/*
 *	_usbd_get_set_interface
 */
void _usbd_get_set_interface(INT32U video, INT32U audio)
{
	INT32U	msg = USBD_NO_EVENT;

	/* Check Video */
	if(send_video != video)
	{
		if(video)
			msg = USBD_START_VIDEO_EVENT;
		else
			msg = USBD_STOP_VIDEO_EVENT;

		send_video = video;

		if(msg != USBD_NO_EVENT)
		{
		    if(msgQSend(UVCTaskQ, msg, NULL, 0, MSG_PRI_NORMAL)!= osOK)
			{
				DBG_PRINT("Send event to UVCTaskQ failed\r\n");
			}
		}
	}

	/* Check Audio */
#if defined(C_USB_AUDIO) && (C_USB_AUDIO)
	msg = USBD_NO_EVENT;
	if(send_audio != audio)
	{
		if(audio)
			msg = USBD_START_AUDIO_EVENT;
		else
			msg = USBD_STOP_AUDIO_EVENT;

		send_audio = audio;

		if(msg != USBD_NO_EVENT)
		{
		    if(msgQSend(UVCTaskQ, msg, NULL, 0, MSG_PRI_NORMAL)!= osOK)
			{
				DBG_PRINT("Send event to UVCTaskQ failed\r\n");
			}
		}
	}
#endif

}

/*************************************************************************/

/*
 *	usb_webcam_start
 */
INT32S usb_webcam_start(void)
{
	INT32S	nRet;

	nRet = STATUS_OK;
	if(video_encode_state_get(VIDEO_ENCODE_STATUS_USB_WEBCAM_OFFSET) == 0)
	{
		video_encode_state_msg_send_with_ack(MSG_AVI_START_USB_WEBCAM,5000);

		video_encode_state_set(VIDEO_ENCODE_STATUS_USB_WEBCAM_OFFSET);
	}

//Return:
	return nRet;
}

/*************************************************************************/

/*
 *	usb_webcam_stop
 */
INT32S usb_webcam_stop(void)
{
	INT32S	nRet;

	nRet = STATUS_OK;
	gstart_video_uvc = 0;

	if(uvc_send_buf)
	{
		uvc_send_buf = 0;
		uvc_send_buf_size = 0;
	}

	if(uvc_send_buf_done)
	{
	    uvc_send_buf_done = NULL;
	}

//Return:
	return nRet;
}

/****************************************************************************/

/*
 *	usb_jpeg_send
 */
INT32S usb_jpeg_send(INT32U buf, INT32U size, INT32U ret_buf)
{
	INT32U msg = USBD_SEND_VIDEO_EVENT;

	if((uvc_send_buf == 0) && (gstart_video_uvc == 1))
    {
		uvc_send_buf = buf;
		uvc_send_buf_size = size;
		uvc_ret_send_buf = ret_buf;
        /////DBG_PRINT("t:%x,%x\r\n",uvc_send_buf,uvc_send_buf_size);
		if(UVCTaskQ)
		{
			if(msgQSend(UVCTaskQ, msg, NULL, 0, MSG_PRI_NORMAL)!= osOK)
			{
				DBG_PRINT("Send event to UVCTaskQ failed\r\n");
				return STATUS_FAIL;
			}
		}
    }
	else
	{
	    ///free buffer
	    return STATUS_FAIL;
	}

	return STATUS_OK;
}

/*************************************************************************/

/*
 *	UVCTask
 */
static void UVCTask(void const *param)
{
	INT32U	msg;

	UVCTaskAckQ = msgQCreate(UVC_ACK_QUEUE_MAX_LEN, UVC_ACK_QUEUE_MAX_LEN, 0);
	if(UVCTaskAckQ == 0)
	{
		ERR_PRINT("fail to create UVCTaskAckQ \r\n");
		return;
	}

	while(1)
	{
	    if(msgQReceive(UVCTaskQ, &msg, NULL, 0) == STATUS_FAIL)
		{
			continue;
		}

		switch(msg)
		{
		case USBD_START_VIDEO_EVENT:
#if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_ON))
			video_encode_sensor_off();
			osDelay(100);       //wait sensor frame to stop
			state_usbd_status_set(1);
			video_encode_sensor_on(STATE_VIDEO_RECORD);

			if(drv_l2_sensor_get_ops(1) == NULL) // Only front sensor is defined in customer.h
			{
				video_encode_preview_mode_set(PREVIEW_MODE_FRONT);
			}
			else
			{
				video_encode_preview_mode_set(PREVIEW_MODE_PIP_REAR);
			}

			uvc_transmission_start();
			if((ap_video_record_sts_get() & VIDEO_RECORD_BUSY) == 0)   // start record
			{
				ap_video_record_func_key_active();
			}

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
			gwifi_transmission_stop = 0;
#endif
#endif
			gstart_video_uvc = 1;
			usbd_uvc_encode_buf_cbk((UVC_SEND_BUF_DONE)lib_free_JPEG);
			break;

		case USBD_SEND_VIDEO_DONE_EVENT:
			if(uvc_send_buf_done)
			{
			    uvc_send_buf_done(2,uvc_ret_send_buf,0);
			}
			uvc_send_buf = 0;
			uvc_ret_send_buf = 0;
			uvc_send_buf_size = 0;
			break;

		case USBD_SEND_VIDEO_EVENT:
			cache_invalid_range((INT32U)uvc_send_buf, uvc_send_buf_size);
			drv_l1_usbd_frame_iso_ep5_in(USBD_USE_USB_HW_IDX,(void *) uvc_send_buf, uvc_send_buf_size, 1);
			break;

		case USBD_STOP_VIDEO_EVENT:
			//DBG_PRINT("YUV end\r\n");
			usb_webcam_stop();
#if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_ON))
			state_usbd_status_set(0);
			video_encode_sensor_off();
			osDelay(100);//wait sensor frame to stop
			uvc_transmission_stop();
			video_encode_sensor_on( STATE_VIDEO_RECORD);
			video_encode_preview_mode_set(PREVIEW_MODE_PIP_REAR);
			msgQSend(UVCTaskAckQ, C_ACK_SUCCESS, NULL, 0, MSG_PRI_NORMAL);
#endif
			break;

#if defined(C_USB_AUDIO) && (C_USB_AUDIO)
		case USBD_START_AUDIO_EVENT:
			DBG_PRINT("MIC start\r\n");
			audio_encode_state_encode_start();
			gstart_audio_uac = 1;
			break;

		case USBD_SEND_AUDIO_EVENT:
			//DBG_PRINT("send audio (%x,%d)\r\n",uac_send_buf,uac_send_buf_size);
			cache_invalid_range((INT32U)uac_send_buf, uac_send_buf_size);
			if(send_audio && usb_audio_iso_send(uac_send_buf, uac_send_buf_size) == STATUS_OK)
			{
				break;
			}
			break;

		case USBD_SEND_AUDIO_DONE_EVENT:
			//DBG_PRINT("audio done\r\n");
			if(send_audio && usb_audio_iso_send(0, 0) == STATUS_OK)
			{
				break;
			}
			break;

		case USBD_STOP_AUDIO_EVENT:
			DBG_PRINT("MIC end\r\n");
			audio_encode_state_encode_stop();
			gstart_audio_uac = 0;
			break;
#endif
		default:
			break;
		}
	}
}

/*************************************************************************/

/*
 *	usb_uvc_init
 */
void usbd_uvc_vid_arg_update(INT32U newFormatType, UVC_USER_FRM_TYPE *newFrm);

INT32S usb_uvc_init(void)
{
    INT32S	ret = STATUS_OK;
	osThreadDef_t uvc_task = {"uvc_task", UVCTask, PRIORITY_USB_CAM, 1, C_USB_CAM_STATE_STACK_SIZE};

	#if defined(C_USB_AUDIO) && (C_USB_AUDIO)
    drv_l1_clock_set_system_clk_en(CLK_EN3_I2S_RX_SRAM,1);
	#endif

    /* uphy resume */
	drv_l1_usbd_uphy_suspend(USBD_USE_USB_HW_IDX, 0);

	drv_l1_usbd_phy_switch(USBD_USE_USB_HW_IDX);

	ret = drv_l2_usbd_ctl_init(USBD_USE_USB_HW_IDX);
	if(ret == STATUS_FAIL)
	{
		DBG_PRINT("drv_l2_usbd_ctl_init failed!\r\n");
		return  ret;
	}

	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_DEVICE_DESCRIPTOR_TYPE, (INT8U*) USB_UVC_UC_DeviceDescriptor);
	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_CONFIG_DESCRIPTOR_TYPE, (INT8U*) USB_UVC_UC_ConfigDescriptor);
	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_DEVICE_QUALIFIER_DESCRIPTOR_TYPE, (INT8U*) USB_UVC_UC_Qualifier_Descriptor_TBL);
	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_STRING0_DESCRIPTOR_TYPE, (INT8U*) UVC_UC_String0_Descriptor);

	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_STRING1_DESCRIPTOR_TYPE, (INT8U*) UVC_UC_String1_Descriptor);
	if(USBD_USE_USB_HW_IDX == USB_A)
		drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_STRING2_DESCRIPTOR_TYPE, (INT8U*) UVC_UC_String2A_Descriptor);
	else
		drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_STRING2_DESCRIPTOR_TYPE, (INT8U*) UVC_UC_String2B_Descriptor);

	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_STRING3_DESCRIPTOR_TYPE, (INT8U*) UVC_UC_String3_Descriptor);
	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_STRING4_DESCRIPTOR_TYPE, (INT8U*) UVC_UC_String4_Descriptor);
	drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_STRING5_DESCRIPTOR_TYPE, (INT8U*) UVC_UC_String5_Descriptor);

	ret = drv_l2_usbd_uvc_init();
	if(ret == STATUS_FAIL)
	{
		DBG_PRINT("drv_l2_usbd_uvc_init failed!\r\n");
		return  ret;
	}

	/* Register EP5 sending done call back function */
	drv_l2_usbd_register_uvc_frame_done_cbk(_usbd_ep5_send_done_cbk);

	/* Register EP7 sending done call back function */
	#if defined(C_USB_AUDIO) && (C_USB_AUDIO)
    drv_l2_usbd_register_uac_frame_done_cbk(_usbd_ep7_send_done_cbk);
	#endif

	/* Register set interface call back */
	drv_l2_usbd_register_set_interface_cbk(_usbd_get_set_interface);
	drv_l2_usbd_register_uvc_frame_types(user_frm_desc_init());
	drv_l2_usbd_register_uvc_change_fmt(usbd_uvc_vid_arg_update);

	UVCThreadId = osThreadCreate(&uvc_task, NULL);
	if(UVCThreadId == 0)
	{
		ret = STATUS_FAIL;
		return ret;
	}

	UVCTaskQ = msgQCreate(USB_CAM_QUEUE_MAX_LEN,USB_CAM_QUEUE_MAX_LEN,0);
	if(UVCTaskQ == NULL)
	{
		DBG_PRINT("UVCTaskQ create failed\r\n");
		ret = STATUS_FAIL;
		return ret;
	}

	ret = drv_l1_usbd_uvc_init(USBD_USE_USB_HW_IDX);
	if(ret == STATUS_FAIL)
	{
		DBG_PRINT("drv_l1_usbd_uvc_init failed!\r\n");
		return ret;
	}

	 /* register USBD ISR handler */
	drv_l1_usbd_enable_isr(USBD_USE_USB_HW_IDX);

	return ret;
}

/*************************************************************************/

/*
 *	usb_uvc_exit
 */
void usb_uvc_exit(void)
{
	//INT8U	err;

	usb_webcam_stop();

	drv_l1_usbd_uninit(USBD_USE_USB_HW_IDX);
	drv_l2_usbd_ctl_uninit(USBD_USE_USB_HW_IDX);
	drv_l2_usbd_uvc_uninit(USBD_USE_USB_HW_IDX);

	send_video = 0;
	send_audio = 0;

	//+++ Del Task & Q
	vTaskSuspend(UVCThreadId);
	osDelay(10);
	vTaskDelete(UVCThreadId);

	msgQFlush(UVCTaskQ);
	msgQDelete(UVCTaskQ);
}

void usbd_uvc_encode_buf_cbk(UVC_SEND_BUF_DONE cbk)
{
    uvc_send_buf_done = cbk;
}

void state_usbd_status_set(INT8U status)
{
    usbd_status = status;
}

INT8U state_usbd_status_get(void)
{
    return usbd_status;
}

INT32U usbd_uvc_display_width_get(void)
{
    return UVC_TAR_WIDTH_MJPEG;
}

INT32U usbd_uvc_display_height_get(void)
{
    return UVC_TAR_HEIGHT_MJPEG;
}

INT32U uvc_display_format_get(void)
{
    return DISP_FMT_GP420;
}

INT32U uvc_display_buf_size_get(void)
{
    INT16U uvc_w = usbd_uvc_display_width_get();
	INT16U uvc_h = usbd_uvc_display_height_get();
    INT32U size;

	if(uvc_display_format_get() == DISP_FMT_GP420)
	{
		size=((uvc_w * (1 + uvc_h)) * 3) >> 1;
	}
	else
	{
		size=(uvc_w * (1 + uvc_h)) << 1;
	}
    size += 0x003f;
    size &= ~0x003f;    //64Byte alignment
	return size;
}

INT32U uvc_encode_type_get(void)
{
    return VID_REC_JPEG;
}

/*************************************************************************/

/*
 *	USB CDC device
 */

#define CDCSTACKSIZE	2048
#define CDC_Q_MAX_SIZE	32
#define CDC_DEMO_GET_PKT            0
#define CDC_DEMO_GET_CHAR           1
#define CDC_DEMO_GET_TYPE           CDC_DEMO_GET_PKT
/**************************************************************************
 *							  E X T E R N A L							  *
 **************************************************************************/
extern osMessageQId cdcTaskQ[2];
extern osThreadId cdcTaskID[2];
extern INT32U dmabufsize;
extern INT32U dmadone;
extern INT8U cdc_SetLineCoding[2];
extern INT8U cdc_device_desc_tbl[];
extern INT8U cdc_config_desc_tbl[];
extern INT8U cdc_qualifier_desc_tbl[];
extern INT8U cdc_string0_desc[];
extern INT8U cdc_string1_desc[];
extern INT8U cdc_string2_desc[];
extern INT8U cdc_string3_desc[];
extern INT8U cdc_string4_desc[];

/**************************************************************************
 *						   G L O B A L	  D A T A						  *
 **************************************************************************/
ALIGN4 INT8U cdc_rxBuf[2][MAX_RXBUF_NUM][MAX_EP_SIZE];
ALIGN4 INT8U cdc_dataBuf[2][MAX_EP_SIZE];
ALIGN64 INT8U cdc_wBuf[512];
ALIGN64 INT8U cdc_wBuf_r[512];
INT32U rxWBufIdx[2] = {0, 0};
INT32U rxRBufIdx[2] = {0, 0};
INT32U rxLen[2][MAX_RXBUF_NUM];
INT32U readIdx[2] = {0, 0}; 	/*0~MAX_EP_SIZE-1*/
volatile INT32U ReadyToGet[2] = {0, 0};
volatile CDC_CTLBLK_T cdc_ctlblk[2];
cdc_param_t cdc_param[2][64];
INT8U cdc_param_idx[2] = {0, 0};
SemaphoreHandle_t	UsbCdc_Sem[2] = {NULL, NULL};
osMessageQId cdcTaskQ[2] = {NULL, NULL};
osThreadId cdcTaskID[2]  = {NULL, NULL};
osThreadId cdcProcessDataID[2]  = {NULL, NULL};
INT32U cdcprocessdata[2] = {0, 0};

/**************************************************************************
 *							 F U N C T I O N S							  *
 **************************************************************************/
/**
 * @brief	Lock USBD CDC semaphore
 * @param:	usb_id: 0: USB_A; 1: USB_B
 * @return	none
 */
INT32S usbd_cdc_lock(INT32U usb_id)
{
	INT32S ret = 0;
	if(UsbCdc_Sem[usb_id]){
		ret = osSemaphoreWait(UsbCdc_Sem[usb_id], osWaitForever);
	}
	return ret;

}

/**
 * @brief	Unlock USBD CDC semaphore
 * @param:	usb_id: 0: USB_A; 1: USB_B
 * @return	none
 */
void usbd_cdc_unlock(INT32U usb_id)
{
	if(UsbCdc_Sem[usb_id]){
		osSemaphoreRelease(UsbCdc_Sem[usb_id]);
	}

}

static INT32S cdc_get_pkt(INT32U usb_id, INT8U* str)
{
	INT32S ret = 0;

	if(ReadyToGet[usb_id] == 1){
		gp_memcpy((INT8S *)str, (INT8S *)cdc_ctlblk[usb_id].dataPtr, cdc_ctlblk[usb_id].dataCnt);
		cdc_ctlblk[usb_id].resData = 0;
		ReadyToGet[usb_id] = 0;
		ret = cdc_ctlblk[usb_id].dataCnt;
	}
	return ret;
}

/**
 * @brief	USBD CDC get 1 char
 * @param:	usb_id: 0: USB_A; 1: USB_B
 * @param:	str: the pointer of the string to get to
 * @return: The getting char length
 */
INT32S cdc_get_char(INT32U usb_id, INT8U* str)
{
	INT32S ret = 0;

	if(ReadyToGet[usb_id] == 1){
		if(cdc_ctlblk[usb_id].resData == 0){
			DBG_PRINT("Err: 0 resData!\r\n");
		}
		*str = cdc_ctlblk[usb_id].dataPtr[readIdx[usb_id]];
		readIdx[usb_id]++;
		cdc_ctlblk[usb_id].resData --;

		if(cdc_ctlblk[usb_id].resData == 0){
			ReadyToGet[usb_id] = 0;
		}
		ret = 1;
	}

	return ret;
}

/**
 * @brief	USBD CDC get 1 packet
 * @param:	usb_id: 0: USB_A; 1: USB_B
 * @param:	str: the pointer of the string to get to
 * @param:	get_type: GET_PKT: get packet; GET_CHAR: get 1 character
 * @param:	wait: 0: not wait; others: wait until get data.
 * @return	The getting char length
 */
INT32S cdc_data_get(INT32U usb_id, INT8U* str, INT32U get_type, INT32U wait)
{
	INT32S len = 0;

	while(1){
		if(get_type == GET_PKT){
			len = cdc_get_pkt(usb_id, str);
		}else{
			len = cdc_get_char(usb_id, str);
		}
		if(len || !wait){
			break;
		}
		osDelay(1);
	}


	return len;
}

/**
 * @brief	USBD CDC put 1 packet
 * @param:	usb_id: 0: USB_A; 1: USB_B
 * @param:	str: The pointer of the string to put from
 * @param:	len: Putting length
 * @return	0:Success; -1: Failed
 */
INT32S cdc_data_put(INT32U usb_id, INT8U* str, INT32U len)
{
	ALIGN4 INT8U tBuf[512]; /*cdc_data_put() need a 4-byte alignment input buffer*/
	INT32S ret = 0;
	INT32U res_len, trans_len;
	INT8U *ptr = str;
	INT32U maxPktSize = 512;
	INT32U maxPktSizeMask;

	if(USBD_HI_SPEED == 0){
		maxPktSize = 64;
	}
	maxPktSizeMask = maxPktSize-1;
	if (len == 0){
		DBG_PRINT("err: cdc data put len should not be 0!\r\n");
		return -1;
	}

	if(len & (~((1<<22)-1)) ){	  /* len > 4MB; (1<<22): GPA7 usb DMA max size is 4MB */
		trans_len = (1<<22) - maxPktSize;
		res_len = len - trans_len;
	}else if(len < maxPktSize){   /*len < maxPacketSize*/
		trans_len = len;
		res_len = 0;
	}else{						  /* maxPacketSize < len < 4MB*/
		trans_len = len&(~maxPktSizeMask);
		res_len = len & maxPktSizeMask;
	}
	drv_l1_cp15_clean_dcache_range((INT32)str, len);

	while(1){
		if( (INT32U)str & 0x03){
			if(len < maxPktSize){
				gp_memcpy((INT8S* )tBuf, (INT8S* )str, len );
				if(usbd_cdc_lock(usb_id)){
					DBG_PRINT("cdc lock false\r\n");
					ret = -1;
					return ret;
				}
        #if CDC_EP == BULK_EP12
				drv_l1_usbd_disable_ep2_out_nak(usb_id);
				ret += drv_l1_usbd_send_bulk_in(usb_id, tBuf, len); /*the 1'st argument of drv_l1_usbd_send_bulk_in() MUST be a 4-byte alignment address*/
        #elif CDC_EP == BULK_EP89
				drv_l1_usbd_disable_ep9_out_nak(usb_id);
				ret += drv_l1_usbd_send_bulk89_in(usb_id, tBuf, len);
        #endif
			} else {
				DBG_PRINT("cdc data put input buffer should be 4-byte alignment to use DMA mode\r\n");
			}
		} else {
			if(usbd_cdc_lock(usb_id)){
				DBG_PRINT("cdc lock false\r\n");
				ret = -1;
				return ret;
			}
    #if CDC_EP == BULK_EP12
			drv_l1_usbd_disable_ep2_out_nak(usb_id);
			ret += drv_l1_usbd_send_bulk_in(usb_id, ptr, trans_len);
    #elif CDC_EP == BULK_EP89
			drv_l1_usbd_disable_ep9_out_nak(usb_id);
			ret += drv_l1_usbd_send_bulk89_in(usb_id, ptr, trans_len);
    #endif

			if(res_len && res_len < maxPktSize){
				ptr += trans_len;
				trans_len = res_len;
				res_len = 0;
			}else if (res_len == 0){
				break;
			}else{
				ptr += trans_len;
				trans_len = (res_len & ~((1<<22)-1))? ((1<<22) - maxPktSize):(res_len & ~(maxPktSizeMask));
				res_len =  (res_len & ~((1<<22)-1))? (res_len - trans_len):(res_len & maxPktSizeMask);
			}
		}
		osDelay(1);
	}
	return ret;
}


static void cdc_continue_to_get(INT32U usb_id)
{
#if CDC_EP == BULK_EP12
	drv_l1_usbd_enable_rec_bulk_out_pkt(usb_id);
#elif CDC_EP == BULK_EP89
	drv_l1_usbd_enable_rec_bulk89_out_pkt(usb_id);
#endif

}

/**
 * @brief	USBD CDC interrupt in
 * @param:	usb_id: 0: USB_A; 1: USB_B
 * @event:	USBD CDC interrupt in event
 * @return	None
 */
void usbd_cdc_state_int_in(INT32U usb_id, INT32U event)
{
	DBG_PRINT("int in\r\n");
}

/**
 * @brief	USBD CDC BOT(bulk only transport) function for in token
 * @param:	usb_id: 0: USB_A; 1: USB_B
 * @event:	USBD CDC BOT event for in token
 * @return	None
 */
void usbd_cdc_state_bot_in(INT32U usb_id, INT32U event)
{
	switch(event)
	{
#if (MSDC_EP == BULK_EP12)
		case USBD_BULK_IN_ACK_EVENT:
			/* disable bulk in/out pkt 0x330[1] */
			drv_l1_usbd_disable_bulk_in_out(usb_id);
#elif (MSDC_EP == BULK_EP89)
		case USBD_BULK_89_IN_ACK_EVENT:
			drv_l1_usbd_disable_bulk89_in_out(usb_id);
#endif
			if(msdc_ctlblk[usb_id].prostate == BULK_DATA_STA)
			{
				INT8U dir = msdc_ctlblk[usb_id].cbw.cbw_cb.bmCBWFlags >> 7;
				if(dir == CBW_DATA_DIR_IN)
				{
					if(msdc_ctlblk[usb_id].cbwdatatcnt == 0)
					{
						/* send csw pkt*/
						drv_l2_usbd_msdc_send_csw_data(usb_id);
					}
					else
					{
						/* msdc_ctlblk[usb_id].cbwdatatcnt > 0, stall bulk IN pipe, for USB 2.0 test suite */
						DBG_PRINT("IN ACK, data cnt = 0x%x, stall IN pipe\r\n", msdc_ctlblk[usb_id].cbwdatatcnt);
						drv_l1_usbd_send_bulk_stall_null(usb_id, MSDC_EP, CBW_DATA_DIR_IN);
						msdc_ctlblk[usb_id].csw.bCSWStatus = CSW_CMD_PASS_STA;
						msdc_ctlblk[usb_id].cswdataresidue = 0;
						msdc_ctlblk[usb_id].cbwdatatcnt = 0;
						drv_l2_usbd_msdc_send_csw_data(usb_id);
					}
				}
			}
			else if(msdc_ctlblk[usb_id].prostate == BULK_CSW_STA)
			{
			  /* enable bulk out pkt to receive next CBW and reset BOT state to CBW */
				msdc_ctlblk[usb_id].prostate = BULK_CBW_STA;
#if (MSDC_EP == BULK_EP12)
				drv_l1_usbd_enable_rec_bulk_out_pkt(usb_id);
#elif (MSDC_EP == BULK_EP89)
				drv_l1_usbd_enable_rec_bulk89_out_pkt(usb_id);
#endif
			}
			break;

#if CDC_EP == BULK_EP89
		case USBD_BULK_89_IN_DMA_TRANS_DONE_EVENT:
			drv_l1_usbd_disable_bulk89_in_out(usb_id);
		case USBD_BULK_89_IN_ACK_EVENT:
#elif CDC_EP == BULK_EP12
		case USBD_BULK_IN_DMA_TRANS_DONE_EVENT:
			/* disable bulk in/out pkt 0x330[1] */
			drv_l1_usbd_disable_bulk_in_out(usb_id);
		case USBD_BULK_IN_ACK_EVENT:
#endif
			usbd_cdc_unlock(usb_id);

	#if CDC_EP == BULK_EP12
			drv_l1_usbd_enable_ep2_out_nak(usb_id);
	#elif CDC_EP == BULK_EP89
			drv_l1_usbd_enable_ep9_out_nak(usb_id);
	#endif

			break;
#if (MSDC_EP == BULK_EP12)
		case USBD_BULK_IN_DMA_TRANS_DONE_EVENT:

			/* disable bulk in/out pkt 0x330[1] */
			drv_l1_usbd_disable_bulk_in_out(usb_id);
#elif (MSDC_EP == BULK_EP89)
		case USBD_BULK_89_IN_DMA_TRANS_DONE_EVENT:
			drv_l1_usbd_disable_bulk89_in_out(usb_id);
#endif
			dmadone = DMA_BULK_IN_MODE;

			if(msdc_ctlblk[usb_id].prostate == BULK_DATA_STA)
			{
				drv_l2_usbd_msdc_do_read_scsi_cmd(usb_id);
			}
			break;

		case USBD_BULK_IN_NACK_EVENT:
			DBG_PRINT("EP1 IN NAK\r\n");
			drv_l1_usbd_enable_ep1_in_nak(usb_id);
			break;

		case USBD_BULK_89_IN_NACK_EVENT:
			DBG_PRINT("EP8 IN NAK\r\n");
			drv_l1_usbd_enable_ep8_in_nak(usb_id);
			break;

		default:
			break;
	}
}

/**
 * @brief	USBD CDC BOT(bulk only transport) function for out token
 * @param:	usb_id: 0: USB_A; 1: USB_B
 * @event:	USBD CDC BOT event for out token
 * @return	None
 */
void usbd_cdc_state_bot_out(INT32U usb_id, INT32U event)
	{
		INT32U len;
		INT32U msg;
		osStatus status;

		switch(event)
		{
#if (MSDC_EP == BULK_EP12)
			case USBD_BULK_OUT_READY_EVENT:
				/* diable bulk in/out 0x330[1] */
				drv_l1_usbd_disable_bulk_in_out(usb_id);
#elif (MSDC_EP == BULK_EP89)
			case USBD_BULK_89_OUT_READY_EVENT:
				drv_l1_usbd_disable_bulk89_in_out(usb_id);
#endif
				if(msdc_ctlblk[usb_id].prostate == BULK_CBW_STA)
				{
					if(drv_l2_usbd_msdc_get_cbw_data(usb_id) == USBD_RET_SUCCESS)
					{
						/* process SCSI command */
						drv_l2_usbd_scsi_command_process(usb_id);
					}
					else
					{
						//DBG_PRINT("Get CBW data error, stall BULK IN/OUT...\r\n");
					}
				}
				else if(msdc_ctlblk[usb_id].prostate == BULK_DATA_STA)
				{
					/* continuous to receive data from host */

				}
				break;
#if (MSDC_EP == BULK_EP12)
			case USBD_BULK_OUT_M_DMA_DONE_EVENT:
				/* diable bulk in/out 0x330[1] */
				drv_l1_usbd_disable_bulk_in_out(usb_id);

#elif (MSDC_EP == BULK_EP89)
			case USBD_BULK_OUT_89_M_DMA_DONE_EVENT:
				drv_l1_usbd_disable_bulk89_in_out(usb_id);
#endif
				drv_l1_usbd_reset_bulk_dma_type();

				if(msdc_ctlblk[usb_id].prostate == BULK_DATA_STA)
				{
					MSDC_LOGIC_UNIT* lun_ptr;
					INT32U* toscsiptr;
					INT32U presecnum, transfer_len;
					INT32S ret;

					lun_ptr = &msdc_ctlblk[usb_id].lunptr[msdc_ctlblk[usb_id].cbw.cbw_cb.bCBWLUN];

					toscsiptr = (INT32U*)msdc_ctlblk[usb_id].dmaptr->dataptr;
					presecnum = msdc_ctlblk[usb_id].secnum;
					msdc_ctlblk[usb_id].dmaptr = msdc_ctlblk[usb_id].dmaptr->next;
					if(msdc_ctlblk[usb_id].rambufcnt)
					{
						msdc_ctlblk[usb_id].secnum = (dmabufsize/MSDC_BLOCK_SIZE);
						transfer_len = dmabufsize;	   /* Full RAM buffer size */
						msdc_ctlblk[usb_id].rambufcnt--;
					}
					else if(msdc_ctlblk[usb_id].rambufres)
					{
						msdc_ctlblk[usb_id].secnum = msdc_ctlblk[usb_id].rambufres;
						transfer_len = msdc_ctlblk[usb_id].rambufres * MSDC_BLOCK_SIZE;
						msdc_ctlblk[usb_id].rambufres = 0;
					}
					else
					{
						msdc_ctlblk[usb_id].nodmadata = 1;
						msdc_ctlblk[usb_id].secnum = 0;
						transfer_len = 0;
					}
					if(!msdc_ctlblk[usb_id].nodmadata)
					{
#if (MSDC_EP == BULK_EP12)
						drv_l1_usbd_rec_bulk_out(usb_id, (void*)msdc_ctlblk[usb_id].dmaptr->dataptr, transfer_len);
#elif (MSDC_EP == BULK_EP89)
						drv_l1_usbd_rec_bulk89_out(usb_id,(void*)msdc_ctlblk[usb_id].dmaptr->dataptr, transfer_len);
#endif

						if(lun_ptr->storageaccess.usbd_msdc_write_dma_phase != NULL)
						{
							drv_l1_cp15_clean_dcache_range((INT32U)toscsiptr, (presecnum << 9));
							drv_l1_cp15_invalidate_dcache_range((INT32U)toscsiptr, (presecnum << 9));
							ret = lun_ptr->storageaccess.usbd_msdc_write_dma_phase(lun_ptr->storageaccess.priv, (INT32U*)toscsiptr, 1, presecnum);
							if(ret)
								return;
						}
					}
					else
					{
						if(lun_ptr->storageaccess.usbd_msdc_write_dma_phase != NULL)
						{

							drv_l1_cp15_clean_dcache_range((INT32U)toscsiptr, (presecnum << 9));
							drv_l1_cp15_invalidate_dcache_range((INT32U)toscsiptr, (presecnum << 9));

							ret = lun_ptr->storageaccess.usbd_msdc_write_dma_phase(lun_ptr->storageaccess.priv, (INT32U*)toscsiptr, 1, presecnum);
							if(ret)
								return;
						}

						/* Receive done, enter CSW stage */
						if(lun_ptr->storageaccess.usbd_msdc_write_cmdend_phase != NULL)
						{
							lun_ptr->storageaccess.usbd_msdc_write_cmdend_phase(lun_ptr->storageaccess.priv);
						}

						msdc_ctlblk[usb_id].csw.bCSWStatus = CSW_CMD_PASS_STA;
						msdc_ctlblk[usb_id].csw.dCSWDataResidue = 0;
						drv_l2_usbd_msdc_send_csw_data(usb_id);
					}
				}

				break;
#if CDC_EP == BULK_EP12
			case USBD_BULK_OUT_READY_EVENT:
#elif CDC_EP == BULK_EP89
			case USBD_BULK_89_OUT_READY_EVENT:
#endif
				/* Check buffer */
				while(1){
					if(rxLen[usb_id][rxWBufIdx[usb_id]]){
						DBG_PRINT("cdc no buffer to receive rx data\r\n");
					}else{
						break;
					}
				}

				/* Get length */
#if CDC_EP == BULK_EP12
				len = drv_l1_usbd_get_bulk_out_cnt(usb_id);
				if(len == 0){
					DBG_PRINT("Err: bo len 0!\r\n");
					return;
				}
				/* Received some data */
				drv_l1_usbd_get_bulk_out_pkt_data(usb_id, (void*)(&(cdc_rxBuf[usb_id][rxWBufIdx[usb_id]])), len);
#elif CDC_EP == BULK_EP89
				len = drv_l1_usbd_get_bulk89_out_cnt(usb_id);
				if(len == 0){
					DBG_PRINT("Err: bo len 0!\r\n");
					break;
				}
				/* Received some data */
				drv_l1_usbd_get_bulk89_out_pkt_data(usb_id, (void*)(&(cdc_rxBuf[usb_id][rxWBufIdx[usb_id]])), len);
#endif
				usbd_cdc_unlock(usb_id);
				rxLen[usb_id][rxWBufIdx[usb_id]] = len;
				rxRBufIdx[usb_id] = rxWBufIdx[usb_id];
				rxWBufIdx[usb_id]++;
				if(rxWBufIdx[usb_id] >= MAX_RXBUF_NUM){
					rxWBufIdx[usb_id] = 0;
				}

				/* fill to app buffer */
				msg = rxRBufIdx[usb_id]<<16|MSG_CDC_FILL_APP_BUF;
				status = osMessagePut(cdcTaskQ[usb_id], (INT32U)&msg, osWaitForever);
				if(status != osOK){
					DBG_PRINT("send event to cdcTaskQ[%d] failed\r\n",usb_id);
				}

		#if CDC_EP == BULK_EP12
				drv_l1_usbd_enable_ep2_out_nak(usb_id);
		#elif CDC_EP == BULK_EP89
				drv_l1_usbd_enable_ep9_out_nak(usb_id);
		#endif

				break;
#if CDC_EP == BULK_EP12 || CDC_EP == BULK_EP89
			case USBD_BULK_OUT_NACK_EVENT:
			case USBD_BULK_89_OUT_NACK_EVENT:

				msg = MSG_CDC_RX_NEW_DATA;
				status = osMessagePut(cdcTaskQ[usb_id], (INT32U)&msg, osWaitForever);
				if(status != osOK){
					DBG_PRINT("send event to cdcTaskQ[%d] failed\r\n",usb_id);
				}

				break;
#endif

			default:
				break;
		}
	}


/**
 * @brief	USBD CDC Task
 * @param:	param: A pointer to arguments given to the task
 * @return	None
 */
void usbd_cdc_task(void const *param)
{
	INT32U	msg;
	INT32U	rBufIdx;
	osEvent result;
	osStatus status;

	DBG_PRINT("Enter usbd_cdc_task\r\n");
	while(1)
	{
		result = osMessageGet(cdcTaskQ[USBD_USE_USB_HW_IDX], osWaitForever);
		if(result.status != osEventMessage){
			continue;
		}

		msg = (result.value.v)&0xFFFF;
		rBufIdx = result.value.v>>16;

		switch(msg)
		{
			case MSG_CDC_TX_DATA:
				break;

			case MSG_CDC_RX_NEW_DATA:
				if(rxLen[USBD_USE_USB_HW_IDX][rxWBufIdx[USBD_USE_USB_HW_IDX]] == 0 && ReadyToGet[USBD_USE_USB_HW_IDX] == 0){
					if(usbd_cdc_lock(USBD_USE_USB_HW_IDX)){
						DBG_PRINT("cdc lock false\r\n");
						break;
					}
					cdc_continue_to_get(USBD_USE_USB_HW_IDX);
				}else{
					msg = MSG_CDC_RX_NEW_DATA;
					status = osMessagePut(cdcTaskQ[USBD_USE_USB_HW_IDX], (INT32U)&msg, osWaitForever);
					if(status != osOK){
						DBG_PRINT("send event to cdcTaskQ[%d] failed\r\n",USBD_USE_USB_HW_IDX);
					}
				}
				break;

			case MSG_CDC_FILL_APP_BUF:
				if(rxLen[USBD_USE_USB_HW_IDX][rBufIdx] && ReadyToGet[USBD_USE_USB_HW_IDX] == 0){
					INT32U i;
					INT32U idx = 0;
					ReadyToGet[USBD_USE_USB_HW_IDX] = 0;

					if(rxLen[USBD_USE_USB_HW_IDX][rBufIdx] == 0){
						DBG_PRINT("Err: 0 rxLen!\r\n");
					}

					for(i=0; i<rxLen[USBD_USE_USB_HW_IDX][rBufIdx]; i++){
						cdc_dataBuf[USBD_USE_USB_HW_IDX][idx++] = cdc_rxBuf[USBD_USE_USB_HW_IDX][rBufIdx][i];
					}

					rxLen[USBD_USE_USB_HW_IDX][rBufIdx] = 0;
					if(idx)
						ReadyToGet[USBD_USE_USB_HW_IDX] = 1;
					readIdx[USBD_USE_USB_HW_IDX] = 0;
					cdc_ctlblk[USBD_USE_USB_HW_IDX].dataPtr = cdc_dataBuf[USBD_USE_USB_HW_IDX];
					cdc_ctlblk[USBD_USE_USB_HW_IDX].resData = cdc_ctlblk[USBD_USE_USB_HW_IDX].dataCnt = idx;
				}else{
					DBG_PRINT("No rx buf or app buf to fill\r\n");
				}
				break;

			default:
				break;
		}

		/* If CDC process data task end uninit cdc task */
		if(cdcprocessdata[USBD_USE_USB_HW_IDX] == 0)
		{
			usbd_cdc_task_uninit(USBD_USE_USB_HW_IDX);
		}

	}
}

/**
 * @brief	USBD CDC task initial
 * @param:	usb_id: 0: USB_A; 1: USB_B
 * @return	None
 */
void usbd_cdc_task_init(INT32U usb_id)
{
	INT32U i;
	DBG_PRINT("%s\r\n",__FUNCTION__);

	osThreadDef_t cdc_task = {"cdc_task", usbd_cdc_task, osPriorityNormal, 1, CDCSTACKSIZE};
	osMessageQDef_t cdc_q = {CDC_Q_MAX_SIZE, sizeof(INT32U), 0};

	/* Create CDC task and TaskQ */
	cdcTaskQ[USBD_USE_USB_HW_IDX] = osMessageCreate(&cdc_q, NULL);
	if (cdcTaskQ[USBD_USE_USB_HW_IDX] == NULL) {
		DBG_PRINT("cdcTaskQ create failed\r\n");
	}

	cdcTaskID[USBD_USE_USB_HW_IDX] = osThreadCreate(&cdc_task, NULL);
	if (cdcTaskID[USBD_USE_USB_HW_IDX] == NULL) {
        DBG_PRINT("cdc_task thread create failed\r\n");
    }

	if(cdcTaskQ[usb_id]){
		xQueueReset(cdcTaskQ[usb_id]);
	}
	gp_memset((INT8S *)(&cdc_ctlblk[usb_id]), 0, sizeof(CDC_CTLBLK_T));
	rxWBufIdx[usb_id] = 0;

	for(i=0; i<MAX_RXBUF_NUM; i++){
		rxLen[usb_id][i] = 0;
	}

	ReadyToGet[usb_id] = 0;
	readIdx[usb_id] = 0;
	cdc_SetLineCoding[usb_id] = 0;

	if(UsbCdc_Sem[usb_id] == NULL){
		osSemaphoreDef_t Usb_cdc_Sem = { 0 };
		UsbCdc_Sem[usb_id] = osSemaphoreCreate(&Usb_cdc_Sem, 1);
	}
}

/**
 * @brief	USBD CDC Task uninitial, to delete cdcTaskQ and usbd_cdc_task
 * @param:	None
 * @return	None
 */
void usbd_cdc_task_uninit(INT32U usb_id)
{
	if(UsbCdc_Sem[usb_id]){
		vSemaphoreDelete(UsbCdc_Sem[usb_id]);
		UsbCdc_Sem[usb_id] = NULL;
	}

	if(cdcTaskQ[usb_id]){
		xQueueReset(cdcTaskQ[usb_id]);
		vQueueDelete(cdcTaskQ[usb_id]);
		cdcTaskQ[usb_id] = 0;
	}

	if(cdcTaskID[usb_id])
		osThreadTerminate(cdcTaskID[usb_id]);

	DBG_PRINT("CDC task uninit\r\n");
}

void usbd_cdc_init(void)
{
	INT32S ret;

    /* uphy resume */
    drv_l1_usbd_uphy_suspend(USBD_USE_USB_HW_IDX, 0);
    osDelay(1);
	/* switch to USB device mode  bit8 = 0 */
 	drv_l1_usbd_phy_switch(USBD_USE_USB_HW_IDX);

 	//drv_l2_uvc_isp_mode(DISABLE);
    /* Init USBD L2 protocol layer first, including control/bulk/ISO/interrupt transfers */
    /******************************* Control transfer ************************************/
    ret = drv_l2_usbd_ctl_init(USBD_USE_USB_HW_IDX);
    if (ret == STATUS_FAIL) {
        DBG_PRINT("drv_l2_usbd_ctl_init failed!\r\n");
        return;
    }

    /* Register new descriptor table here, this action must be done after drv_l2_usbd_ctl_init() */
    drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_DEVICE_DESCRIPTOR_TYPE, (INT8U*)cdc_device_desc_tbl);
    drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_CONFIG_DESCRIPTOR_TYPE, (INT8U*)cdc_config_desc_tbl);
    drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_DEVICE_QUALIFIER_DESCRIPTOR_TYPE, (INT8U*)cdc_qualifier_desc_tbl);
    drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_STRING0_DESCRIPTOR_TYPE, (INT8U*)cdc_string0_desc);
    drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_STRING1_DESCRIPTOR_TYPE, (INT8U*)cdc_string1_desc);
    drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_STRING2_DESCRIPTOR_TYPE, (INT8U*)cdc_string2_desc);
    drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_STRING3_DESCRIPTOR_TYPE, (INT8U*)cdc_string3_desc);
    drv_l2_usbd_register_descriptor(USBD_USE_USB_HW_IDX, REG_STRING4_DESCRIPTOR_TYPE, (INT8U*)cdc_string4_desc);

    /* Init USBD L1 register layer */
    ret = drv_l1_usbd_init(USBD_USE_USB_HW_IDX);
    if (ret == STATUS_FAIL) {
        DBG_PRINT("drv_l1_usbd_init failed!\r\n");
        return;
    }

	drv_l2_usbd_cdc_init(USBD_USE_USB_HW_IDX);
	drv_l2_usbd_cdc_register_task_init(usbd_cdc_task_init);

	drv_l2_usbd_register_state_handler(USBD_XFER_CONTROL, drv_l2_usbd_state_ctl_cdc_ep0_setup, drv_l2_usbd_state_ctl_ep0_in, drv_l2_usbd_state_ctl_cdc_ep0_out);
	drv_l2_usbd_register_state_handler(USBD_XFER_BULK, NULL, usbd_cdc_state_bot_in, usbd_cdc_state_bot_out);

	/* register INT IN/OUT state handler */
    drv_l2_usbd_register_state_handler(USBD_XFER_INT, NULL, usbd_cdc_state_int_in, NULL);

	/* register USBD ISR handler */
	drv_l1_usbd_enable_isr(USBD_USE_USB_HW_IDX);
    DBG_PRINT("USB device CDC init completed!\r\n");

	/* Init process CDC driver task */
	usbd_cdc_task_init(USBD_USE_USB_HW_IDX);

	/* Init process CDC data task */
	usbd_cdc_process_data_init(USBD_USE_USB_HW_IDX);
}

void usbd_cdc_exit(void)
{
	INT32S ret;

	usbd_cdc_task_uninit(USBD_USE_USB_HW_IDX);

	usbd_cdc_process_data_uninit(USBD_USE_USB_HW_IDX);

	ret = drv_l1_usbd_uninit(USBD_USE_USB_HW_IDX);
    if (ret == STATUS_FAIL) {
		DBG_PRINT("drv_l1_usbd_uninit failed!\r\n");
		return;
	}
	drv_l2_usbd_ctl_uninit(USBD_USE_USB_HW_IDX);

	//drv_l1_clock_set_system_clk_en(CLK_EN1_USB20DEV, 0);
}

void usbd_cdc_process_data_task(void const *param)
{
	INT32U len, put_len;
	INT32U wait = 1;
	INT32U i, j;

	cdcprocessdata[USBD_USE_USB_HW_IDX] = 1;
	while(cdcprocessdata[USBD_USE_USB_HW_IDX])
	{
#if CDC_DEMO_GET_TYPE == CDC_DEMO_GET_CHAR
		len = cdc_data_get(USBD_USE_USB_HW_IDX, cdc_wBuf_r, GET_CHAR, wait);   //Get 1 char
#else
		len = cdc_data_get(USBD_USE_USB_HW_IDX, cdc_wBuf_r, GET_PKT, wait);  //Get 1 packet
#endif
        /*Padding LF after CR*/
        put_len = len;

        j=0;
        for(i=0; i<len; i++)
		{
            cdc_wBuf[j] = cdc_wBuf_r[i];
            if(cdc_wBuf[j++] == '\r')
			{
                cdc_wBuf[j++] = '\n';
                put_len++;
            }
        }

		if(cdc_data_put(USBD_USE_USB_HW_IDX, cdc_wBuf, put_len)!= put_len)
		{
            DBG_PRINT("put len err!\r\n");
		}
		//DBG_PRINT("CDC len %d put len %d\r\n", len, put_len);
	}
}

void usbd_cdc_process_data_init(INT32U usb_id)
{
	/* Create CDC process data task */
	osThreadDef_t usbdCdcDataTask = {"usbdDataTask", usbd_cdc_process_data_task, osPriorityNormal, 1, CDCSTACKSIZE};
	cdcProcessDataID[usb_id] = osThreadCreate(&usbdCdcDataTask, NULL);
	if (cdcProcessDataID[usb_id] == NULL)
	{
    	DBG_PRINT("usbd_cdc_process_data_task thread create failed\r\n");
    }
}

void usbd_cdc_process_data_uninit(INT32U usb_id)
{
	if(cdcProcessDataID[usb_id])
	{
		osThreadTerminate(cdcProcessDataID[usb_id]);
		cdcProcessDataID[usb_id] = 0;
	}
	DBG_PRINT("CDC process data uninit\r\n");
}

#if (defined C_USB_AUDIO) && (C_USB_AUDIO)
INT32S
usb_audio_send(
	INT32U buf,
	INT32U size,
	INT32U ret_buf
)
{
	INT32U msg = USBD_SEND_AUDIO_EVENT;

	if((uac_send_buf == 0) && (gstart_audio_uac == 1))
    {
		uac_send_buf = buf;
		uac_send_buf_size = size;
		uac_ret_send_buf = ret_buf;
        //DBG_PRINT("t:%x,%x,%x\r\n",uac_send_buf,uac_send_buf_size,uac_ret_send_buf);
		if(UVCTaskQ)
		{
			if(msgQSend(UVCTaskQ, msg, NULL, 0, MSG_PRI_NORMAL)!= osOK)
			{
				DBG_PRINT("Send event to UVCTaskQ failed\r\n");
				return STATUS_FAIL;
			}
		}
    }
	else
	{
	    ///free buffer
	    return STATUS_FAIL;
	}

	return STATUS_OK;
}

static INT32S usb_audio_iso_send(INT32U new_buf, INT32U new_size)
{
	#define USBD_UVC_MIC_PKT_SIZE		128

	if(new_buf && new_size)
	{
		uac_send_iso_buf		= new_buf;
		uac_send_iso_buf_size	= new_size;
	}

	if(!uac_send_iso_buf || !uac_send_iso_buf_size)
	{
		//DBG_PRINT("uac_ret_send_buf:%x\r\n",uac_ret_send_buf);
		ap_audio_free_cap_buffer(uac_ret_send_buf);
		uac_send_buf = 0;
		return STATUS_FAIL;
	}

	if(drv_l2_usbd_uvc_check_ep7_flush())
	{
		//DBG_PRINT("Aud EP7 flush\r\n");
		drv_l1_usbd_iso_ep7_flush(USBD_USE_USB_HW_IDX);
	}


	if(uac_send_iso_buf_size < USBD_UVC_MIC_PKT_SIZE)
	{
		//drv_l1_usbd_dma_iso_ep7_in(USBD_USE_USB_HW_IDX,(void *) uac_send_iso_buf, uac_send_iso_buf_size);
		drv_l1_usbd_iso_ep7_in(USBD_USE_USB_HW_IDX,(void *) uac_send_iso_buf, uac_send_iso_buf_size);
		uac_send_iso_buf		= 0;
		uac_send_iso_buf_size	= 0;
	}
	else
	{
		//drv_l1_usbd_dma_iso_ep7_in(USBD_USE_USB_HW_IDX,(void *) uac_send_iso_buf, USBD_UVC_MIC_PKT_SIZE);
		drv_l1_usbd_iso_ep7_in(USBD_USE_USB_HW_IDX,(void *) uac_send_iso_buf, USBD_UVC_MIC_PKT_SIZE);
		uac_send_iso_buf_size -= USBD_UVC_MIC_PKT_SIZE;
		uac_send_iso_buf += USBD_UVC_MIC_PKT_SIZE;
	}

	return STATUS_OK;
}


#endif


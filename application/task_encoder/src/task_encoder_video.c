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

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define VIDEO_ENCODE_STATE_ACK_QUEUE_MAX_LEN		1

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
INT32S video_encode_state_photo_start(void);

extern INT32S cb_func_sensor_start(void);
extern INT32S cb_func_sensor_stop(void);
extern INT32S cb_func_preview_mode_set(PREVIEW_MODE mode);
extern INT32S cb_func_encode_start(void);
extern INT32S cb_func_encode_stop(void);
extern INT32S cb_func_packer_stop(void);
extern INT32S cb_func_pscaler_process(INT8U src);
extern INT32S cb_func_H264_process(INT8U src);
extern INT32S cb_func_JPEG_process(INT8U src);
extern INT32S cb_func_get_jpeg_on_encode(void);
extern INT32S cb_func_get_jpeg_enc_process(INT8U src);
extern INT32S cb_func_get_jpeg_done_process(INT8U src);
extern INT32S cb_func_photo_start(void);
extern INT32S cb_func_pscaler_scaleup_process(INT8U src);
extern INT32S cb_func_photo_write_process(INT8U src);
extern INT32S cb_func_get_jpeg_buf_release(INT8U src,GET_JPEG_ARGS* args);
extern INT32S lib_free_JPEG(INT32U src,INT32U data,INT32U frameType);
extern INT32S cb_func_insertframe_process(INT16U src);

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
extern INT32S cb_func_wifi_enc_start(void);
extern INT32S cb_func_wifi_enc_stop(void);
extern INT32S cb_func_wifi_enc_buf_release(void);
#endif


#if C_WIFI_SEND_FIX_H264I
extern void rtsp_h264_fix_send_start(void);
extern void rtsp_h264_fix_send_stop(void);
#endif


#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
extern INT32S cb_func_uvc_enc_start(void);
extern INT32S cb_func_uvc_enc_stop(void);
#endif

/*== HOST USB MJPEG CAM / INTERP2K ==*/
#if (_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K)
extern INT32S cb_func_host_2K_decode_JPEG_process(void);
extern INT32S cb_func_host_2K_frame_ready_process(INT8U src);
extern INT32S cb_func_host_2K_preview_done_process(INT8U src);
extern INT32S cb_func_interp2K_scaleup_process(INT8U src);
extern INT32S host_2K_mjpeg_ready_notify(void);
#endif


#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
extern void uvc_record_stop_req_set(INT8U src);
extern void wifi_encode_stop_req_set(void);
#endif

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT32U VidEncStateFlag = 0;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static void
return_ack(
	INT32S nRet
)
{
	INT32U ret_ack;

	if(nRet >= 0) {
		ret_ack = C_ACK_SUCCESS;
	} else {
		ret_ack = C_ACK_FAIL;
	}

	//DBG_PRINT("==Run:%s==ret_ack:%d==\r\n",__FUNCTION__,ret_ack);

    msgQSend(encoderAckQ, ret_ack, NULL, 0, MSG_PRI_NORMAL);
}

void
video_encode_state_task_init(
	void
)
{
	TASK_APP_ARGUMENT* pVideoEncTaskArgs = &pTurnkeyArgs->taskVideoEnc;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pVideoEncTaskArgs->taskAckQ == 0) {
		pVideoEncTaskArgs->taskAckQ = msgQCreate(VIDEO_ENCODE_STATE_ACK_QUEUE_MAX_LEN, VIDEO_ENCODE_STATE_ACK_QUEUE_MAX_LEN, 0);
		if(pVideoEncTaskArgs->taskAckQ == 0) {
			ERR_PRINT("fail to create pVideoEncTaskArgs->taskAckQ\r\n");
			return;
		}
	}

	if(pVideoEncTaskArgs->taskAppQ == 0) {
		pVideoEncTaskArgs->taskAppQ = msgQCreate(pVideoEncTaskArgs->taskMsgLen, pVideoEncTaskArgs->taskMsgLen, pVideoEncTaskArgs->taskRecvBufLen);
		if(pVideoEncTaskArgs->taskAppQ == 0) {
			ERR_PRINT("fail to create pVideoEncTaskArgs->taskAppQ \r\n");
			goto ERR_VIDEO_ENC_STATE_TASK_INIT;
		}

		if(pVideoEncTaskArgs->taskRecvBufLen)
		{
			pVideoEncTaskArgs->taskRecvBuf = (void*)malloc(pVideoEncTaskArgs->taskRecvBufLen);
			if(pVideoEncTaskArgs->taskRecvBuf == 0) {
				ERR_PRINT("fail to malloc pVideoEncTaskArgs->taskRecvBuf\r\n");
				goto ERR_VIDEO_ENC_STATE_TASK_INIT;
			}
		}
	}

	if(pVideoEncTaskArgs->taskSem == 0) {
		osSemaphoreDef (taskVideoEnc_semaphore);    // Declare semaphore
		pVideoEncTaskArgs->taskSem = osSemaphoreCreate(osSemaphore(taskVideoEnc_semaphore), 1);
		if(pVideoEncTaskArgs->taskSem  == 0) {
			ERR_PRINT("fail to create pVideoEncTaskArgs->taskSem\r\n");
			goto ERR_VIDEO_ENC_STATE_TASK_INIT;
		}
	}

	return;

ERR_VIDEO_ENC_STATE_TASK_INIT:
	if(pVideoEncTaskArgs->taskAckQ != 0)
	{
		msgQDelete(pVideoEncTaskArgs->taskAckQ);
		pVideoEncTaskArgs->taskAckQ = 0;
	}

	if(pVideoEncTaskArgs->taskAppQ != 0)
	{
		msgQDelete(pVideoEncTaskArgs->taskAppQ);
		pVideoEncTaskArgs->taskAppQ = 0;
	}

	if(pVideoEncTaskArgs->taskRecvBuf)
	{
		free(pVideoEncTaskArgs->taskRecvBuf);
		pVideoEncTaskArgs->taskRecvBuf = 0;
	}
	return;
}

void
video_encode_preview_stop(
	void
)
{
	INT32S	nRet;

	nRet = sensor_preview_wait_stop();
	if(nRet == STATUS_FAIL)
	{
		if(msgQSend(encoderTaskQ, MSG_VID_STOP_SENSOR, NULL, 0, MSG_PRI_NORMAL) < 0)
		{
			ERR_PRINT("fail to MSG_VID_WAIT_STOP_SENSOR \r\n");
		}
	}
	else
	{
		return_ack(cb_func_sensor_stop());
	}
}

static void
video_encode_state_task_entry(
	void const *arg
)
{
	TASK_APP_ARGUMENT* pVideoEncTaskArgs;
	INT32U encoderRecvBufLen;
	void* pEncoderRecvBuf;
	INT32U  msg_id;

	#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
	INT32U	tick_curr = xTaskGetTickCount();
	INT32U  uvc_tick_get_last = 0;
	INT32S	nRet;
	#endif

 	pVideoEncTaskArgs = &pTurnkeyArgs->taskVideoEnc;
	pEncoderRecvBuf = pVideoEncTaskArgs->taskRecvBuf;
	encoderRecvBufLen = pVideoEncTaskArgs->taskRecvBufLen;

	#if (_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K)
	interp2K_q_init();
	#endif

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	while(1)
	{
		#if  (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
		nRet = msgQReceive_timeout(encoderTaskQ, &msg_id, pEncoderRecvBuf, encoderRecvBufLen, 10);

		tick_curr = xTaskGetTickCount();
		if((msg_id & 0xFFFF0000) == MSG_HOST_2K_MJPEG_READY_NOTIFY)
		{
			uvc_tick_get_last = tick_curr;
		}

		if(tick_curr - uvc_tick_get_last >= 40)
		{
			uvc_tick_get_last = tick_curr;

			if(sensor_preview_status_get() == PREVIEW_MODE_STOP)
			{
				// do nothing
				;
			}
			else
			{
				if(uvc_q_ready_is_empty())
				{
					if(host_2K_mjpeg_ready_notify() == STATUS_FAIL)
					{
						ERR_PRINT("[ERROR] Post Fail \r\n");
					}
				}
			}
		}

		//DBG_PRINT("msg_id:0x%x==\r\n",msg_id);

		if((!msg_id) || (nRet == STATUS_FAIL))
		{
			continue;
		}

		#else
		if (msgQReceive(encoderTaskQ, &msg_id, pEncoderRecvBuf, encoderRecvBufLen) == STATUS_FAIL)
		{
			continue;
		}
		#endif

		//DBG_PRINT("==Run:%s==msg_id:0x%x==\r\n",__FUNCTION__,msg_id);

		switch(msg_id & 0xFFFF0000)
		{
	        case MSG_VID_START_SENSOR:
				return_ack(cb_func_sensor_start());
				break;

			case MSG_VID_STOP_SENSOR:
				video_encode_preview_stop();
				break;

			case MSG_VID_PREVIEW_MODE_SET:
				return_ack(cb_func_preview_mode_set(msg_id & 0x0000FFFF));
				break;

			/*== Video Encode ==*/
			case MSG_VID_START_ENCODE:
				return_ack(cb_func_encode_start());
				break;

			case MSG_VID_STOP_ENCODE:
				cb_func_encode_stop();
				break;

			case MSG_VID_RECORD_STOP:
				return_ack(cb_func_packer_stop());
				break;

			case MSG_VID_PSCALER_NOTIFY:
				cb_func_pscaler_process((INT8U)(msg_id & 0x00FF));
				break;

			case MSG_VID_H264_DUMMY_NOTIFY:
	            cb_func_insertframe_process((INT16U)(msg_id & 0xFFFF));
			    break;
			case MSG_VID_H264_NOTIFY:
				cb_func_H264_process((INT8U)(msg_id & 0x00FF));
				break;

			case MSG_VID_JPEG_NOTIFY:
				cb_func_JPEG_process((INT8U)(msg_id & 0x00FF));
				break;

			/*== Snapshot ==*/
			#if C_SUPORT_GET_JPG_BUF
			case MSG_VID_GET_JPEG_ON_ENC:
				cb_func_get_jpeg_on_encode();
				break;

			case MSG_VID_GET_JPEG_NOTIFY:
				cb_func_get_jpeg_enc_process((INT8U)(msg_id & 0x00FF));
				break;

			case MSG_VID_GET_JPEG_DONE:
				cb_func_get_jpeg_done_process((INT8U)(msg_id & 0x00FF));
				break;
			#endif

			/*== Photo Capture ==*/
			case MSG_VID_CAPTURE_PHOTO:
				if(cb_func_photo_start() == STATUS_FAIL)
				{
					video_encode_state_photo_start();
				}
				break;

			case MSG_VID_PSCALER_SCALEUP_NOTIFY:
				cb_func_pscaler_scaleup_process((INT8U)(msg_id & 0x00FF));
				break;

			case MSG_VID_PHOTO_JPEG_NOTIFY:
				cb_func_photo_write_process((INT8U)(msg_id & 0x00FF));
				break;

			/*== Save Raw ==*/
			#if C_SENSOR_SAVE_RAW_DATA
			case MSG_SENSOR_SAVE_RAW_DATA:
				cb_func_sensor_write_raw_process();
				break;
			#endif

			/*== WIFI ==*/
			#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
			case MSG_WIFI_START_ENCODE:
				return_ack(cb_func_wifi_enc_start());
				break;

	        case MSG_WIFI_STOP_ENCODE:
	           cb_func_wifi_enc_stop();
	            break;

			case MSG_WIFI_BUF_RELEASE:
				return_ack(cb_func_wifi_enc_buf_release());
				break;
			#endif

			/*== UVC ==*/
			#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
			case MSG_UVC_START_ENCODE:
				return_ack(cb_func_uvc_enc_start());
				break;

			case MSG_UVC_STOP_ENCODE:
				return_ack(cb_func_uvc_enc_stop());
				break;
			#endif

			/*== HOST USB MJPEG CAM / INTERP2K IN COMMON ==*/
			#if (_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K)
			case MSG_HOST_2K_MJPEG_READY_NOTIFY:
			// Check queue buffer and decode jpeg
			cb_func_host_2K_decode_JPEG_process();
			break;

			case MSG_HOST_2K_FRAME_READY_NOTIFY:
			// Scale down the data and show it on display
			cb_func_host_2K_frame_ready_process((INT8U)(msg_id & 0x00FF));
			break;

			case MSG_HOST_2K_PREVIEW_DONE_NOTIFY:
			// Handle video record and photo capture
			cb_func_host_2K_preview_done_process((INT8U)(msg_id & 0x00FF));
			break;
			#endif

			/*== INTERPOLATING 2K ==*/
			#if (defined C_VID_INTERPOLATING_2K) && (C_VID_INTERPOLATING_2K)
			case MSG_INTERP2K_BYPASS_FRAME_DONE_NOTIFY:
			// Scale up the data and send it to encode
			cb_func_interp2K_scaleup_process((INT8U)(msg_id & 0x00FF));
			break;
			#endif
		}
	}
}

INT32S
video_encode_state_task_create(
	void
)
{
	INT32S nRet = STATUS_OK;
	osThreadId id;
	osThreadDef_t video_enc_task = {"video_enc_state_task", video_encode_state_task_entry, PRIORITY_VIDEO_ENC_TASK, 1, VIDEO_ENCODE_STATE_STACK_SIZE};

	id = osThreadCreate(&video_enc_task, NULL);
	if(id == 0) {
		RETURN(STATUS_FAIL);
	}

	nRet = STATUS_OK;

Return:
	return nRet;
}

void
video_encode_state_set(
	VIDEO_ENCODE_STATUS_OFFSET stateOffset
)
{
	if(ENCODE_STATUS_IS_BUSY(VidEncStateFlag,stateOffset))
	{
		ERR_PRINT("state flag:0x%x==offset:%d is set\r\n",VidEncStateFlag,stateOffset);
		return;
	}

	osSuspend();
	ENCODE_STATUS_SET_BUSY(VidEncStateFlag,stateOffset);
	osResume();
}

void
video_encode_state_clear(
	VIDEO_ENCODE_STATUS_OFFSET stateOffset
)
{
	if(ENCODE_STATUS_IS_BUSY(VidEncStateFlag,stateOffset) == 0)
	{
		ERR_PRINT("state flag:0x%x==offset:%d is not set\r\n",VidEncStateFlag,stateOffset);
		return;
	}

	osSuspend();
	ENCODE_STATUS_CLR_BUSY(VidEncStateFlag,stateOffset);
	osResume();
}

INT32S
video_encode_state_get(
	VIDEO_ENCODE_STATUS_OFFSET stateOffset
)
{
	return (ENCODE_STATUS_IS_BUSY(VidEncStateFlag,stateOffset));
}

INT32S
video_encode_state_sensor_start(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	osSemaphoreWait(encoderSem,osWaitForever);

	if(video_encode_state_get(VIDEO_ENCODE_STATUS_SENSOR_OFFSET) == 0)
	{
        POST_MESSAGE(encoderTaskQ, MSG_VID_START_SENSOR, encoderAckQ, 5000, MSG_PRI_NORMAL);
        video_encode_state_set(VIDEO_ENCODE_STATUS_SENSOR_OFFSET);
    }
Return:

	osSemaphoreRelease(encoderSem);

	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to preview start\r\n");
	}

	return nRet;
}

INT32S
video_encode_state_sensor_stop(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	osSemaphoreWait(encoderSem,osWaitForever);

	if(video_encode_state_get(VIDEO_ENCODE_STATUS_SENSOR_OFFSET))
	{
		video_encode_state_clear(VIDEO_ENCODE_STATUS_SENSOR_OFFSET);
		POST_MESSAGE(encoderTaskQ, MSG_VID_STOP_SENSOR, encoderAckQ, 5000, MSG_PRI_NORMAL);
	}

Return:

	osSemaphoreRelease(encoderSem);

	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to preview stop\r\n");
	}

	return nRet;
}


INT32S
video_encode_state_preview_mode_set(
	INT32U mode
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(mode >= PREVIEW_MODE_VALID)
	{
		DBG_PRINT("[ERROR] invalid preview mode=%d\r\n, @%s:%d\r\n", mode, __func__, __LINE__);
		mode = PREVIEW_MODE_FRONT;
	}

	osSemaphoreWait(encoderSem,osWaitForever);

	POST_MESSAGE(encoderTaskQ, MSG_VID_PREVIEW_MODE_SET|mode, encoderAckQ, 1000, MSG_PRI_NORMAL);

Return:

	osSemaphoreRelease(encoderSem);

	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to preview set\r\n");
	}

	return nRet;
}

INT32S
video_encode_state_encode_start(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	osSemaphoreWait(encoderSem,osWaitForever);

	if(video_encode_state_get(VIDEO_ENCODE_STATUS_ENCODE_OFFSET) == 0)
	{
		POST_MESSAGE(encoderTaskQ, MSG_VID_START_ENCODE, encoderAckQ, 1000, MSG_PRI_NORMAL);
		video_encode_state_set(VIDEO_ENCODE_STATUS_ENCODE_OFFSET);
	}

Return:

	osSemaphoreRelease(encoderSem);

	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to encode start\r\n");
	}

	return nRet;
}

INT32S
video_encode_state_encode_stop(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	osSemaphoreWait(encoderSem,osWaitForever);

	if(video_encode_state_get(VIDEO_ENCODE_STATUS_ENCODE_OFFSET))
	{
		POST_MESSAGE(encoderTaskQ, MSG_VID_STOP_ENCODE, encoderAckQ, 20000, MSG_PRI_NORMAL);
		video_encode_state_clear(VIDEO_ENCODE_STATUS_ENCODE_OFFSET);
	}

Return:

	osSemaphoreRelease(encoderSem);

	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to encode stop\r\n");
	}

	return nRet;
}

INT32S
video_encode_state_photo_start(
	void
)
{
	INT32S nRet = STATUS_OK;

	nRet = msgQSend(encoderTaskQ, MSG_VID_CAPTURE_PHOTO, NULL, 0, MSG_PRI_NORMAL);

	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to encode start\r\n");
	}

	return nRet;
}

INT32S
video_encode_state_msg_send(
    INT32U msg, void *para, INT32U bytes, INT32U prio
)
{
    INT32S nRet;

    if(encoderTaskQ == NULL)
    {
        nRet = -1;
        ERR_PRINT("send msg fail\r\n");
   }
    else
    {
        nRet = msgQSend(encoderTaskQ, msg, para, bytes, prio);
    }
    return nRet;
}

INT32S
video_encode_state_msg_send_with_ack(
    INT32U msg,
	INT32U msc_timeout
)
{
    INT32S nRet;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

    if(encoderTaskQ == NULL)
    {
        nRet = -1;
        ERR_PRINT("send msg fail\r\n");
   }
    else
    {
		osSemaphoreWait(encoderSem,osWaitForever);
        nRet = _post_message(encoderTaskQ, msg, encoderAckQ, msc_timeout, MSG_PRI_NORMAL);
		osSemaphoreRelease(encoderSem);
    }
    return nRet;
}

/*== WIFI ==*/
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
INT32S
wifi_transmission_start(
	void
)
{
	
#if C_WIFI_SEND_FIX_H264I
		DBG_PRINT("wifi send fix H264 I-frame start\r\n");
		rtsp_h264_fix_send_start();
		return STATUS_OK;
#endif


	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	osSemaphoreWait(encoderSem,osWaitForever);

	video_encode_state_set(VIDEO_ENCODE_STATUS_WIFI_TRANSMISSION_OFFSET);

	POST_MESSAGE(encoderTaskQ, MSG_WIFI_START_ENCODE, encoderAckQ, 1000, MSG_PRI_NORMAL);

Return:

	osSemaphoreRelease(encoderSem);

	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to wifi encode start\r\n");
	}

	return nRet;
}

INT8U gwifi_transmission_stop = 0;
INT32S
wifi_transmission_stop(
	void
)
	{

#if C_WIFI_SEND_FIX_H264I
		DBG_PRINT("wifi send fix H264 I-frame stop\r\n");
		rtsp_h264_fix_send_stop();
		return STATUS_OK;
#endif


		INT32S nRet = STATUS_OK;
		INT32S wifi_tx_active;
	
		DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);
	
	
		wifi_tx_active = video_encode_state_get(VIDEO_ENCODE_STATUS_WIFI_TRANSMISSION_OFFSET);
	
	
	#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
		wifi_encode_stop_req_set();
		//DBG_PRINT("[WIFI_STOP_REQ] set\r\n");
	#endif
	
	
    #if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_ON))
		gwifi_transmission_stop = 1;
	#endif
	


		if(wifi_tx_active)
		{
			if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
			{
			#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
				uvc_record_stop_req_set(PREVIEW_SRC_REAR);
			#endif
				
				ap_video_record_func_key_active();
				DBG_PRINT("FUN_KEY\r\n");
			}
			vTaskDelay(100);//wait sensor frame to stop
		}
		osSemaphoreWait(encoderSem,osWaitForever);
	
		if(wifi_tx_active)
		{
			// Wait for wifi encodeing to stop
			POST_MESSAGE(encoderTaskQ, MSG_WIFI_STOP_ENCODE, encoderAckQ, 1500, MSG_PRI_NORMAL);
	
			video_encode_state_clear(VIDEO_ENCODE_STATUS_WIFI_TRANSMISSION_OFFSET);
		}
		else
		{
			DBG_PRINT("[WIFI_STOP_REQ] wifi_tx already clear\r\n");
		}
	
	Return:
		osSemaphoreRelease(encoderSem);
	
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("fail to wifi encode stop\r\n");
		}
		return nRet;
	}

#endif

#if C_SUPORT_GET_JPG_BUF
INT32S
video_encode_state_jpeg_on_enc_get(
	void
)
{
	INT32S nRet = STATUS_OK;

	if(video_encode_state_get(VIDEO_ENCODE_STATUS_GET_JPEG_OFFSET) == 0)
	{
		nRet = msgQSend(encoderTaskQ, MSG_VID_GET_JPEG_ON_ENC, NULL, 0, MSG_PRI_NORMAL);
		if(nRet == STATUS_FAIL)
		{
			ERR_PRINT("fail to get jpeg on encode\r\n");
			return nRet;
		}

		video_encode_state_set(VIDEO_ENCODE_STATUS_GET_JPEG_OFFSET);
	}
	else
	{
		nRet = STATUS_FAIL;
		ERR_PRINT("The last action has not been completed\r\n");
	}

	return nRet;
}

INT32S
video_encode_state_release_get_jpeg_buf(
	INT8U src
)
{
	VID_INFO*  pVid_info;
	GET_JPEG_ARGS* pGet_jpeg_args;
	JpegEnc_t* pJpeg_Snapshot;

	switch(src)
	{
		case 0: // Front
		pVid_info = &pStreamArgs->vid_info_front;
		break;
		default: // Rear
		pVid_info = &pStreamArgs->vid_info_rear;
		break;
	}

	pGet_jpeg_args = &pVid_info->get_jpeg_args;
 	pJpeg_Snapshot = &pGet_jpeg_args->jpeg_cfg;

	if(pGet_jpeg_args->flag_need_enc)
	{
		// Free jpeg's encode output buffer and header buffer
		cb_func_get_jpeg_buf_release(src,pGet_jpeg_args);
	}
	else
	{
		// Put jpeg encode output buffer back to circular buf
		lib_free_JPEG(src,pJpeg_Snapshot->output_buffer,0);
	}

	if(video_encode_state_get(VIDEO_ENCODE_STATUS_GET_JPEG_OFFSET))
	{
		video_encode_state_clear(VIDEO_ENCODE_STATUS_GET_JPEG_OFFSET);
	}

    return STATUS_OK;
}
#endif

/*== USB CAM ==*/
 #if(defined(C_USB_DEVICE) && C_USB_DEVICE)
INT32S
uvc_transmission_start(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	osSemaphoreWait(encoderSem,osWaitForever);

	video_encode_state_set(VIDEO_ENCODE_STATUS_UVC_TRANSMISSION_OFFSET);

	POST_MESSAGE(encoderTaskQ, MSG_UVC_START_ENCODE, encoderAckQ, 1000, MSG_PRI_NORMAL);

Return:

	osSemaphoreRelease(encoderSem);

	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to uvc encode start\r\n");
	}

	return nRet;
}

INT32S
uvc_transmission_stop(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	osSemaphoreWait(encoderSem,osWaitForever);

	if(video_encode_state_get(VIDEO_ENCODE_STATUS_UVC_TRANSMISSION_OFFSET))
	{
		POST_MESSAGE(encoderTaskQ, MSG_UVC_STOP_ENCODE, encoderAckQ, 1000, MSG_PRI_NORMAL);
		video_encode_state_clear(VIDEO_ENCODE_STATUS_UVC_TRANSMISSION_OFFSET);
	}

Return:

	osSemaphoreRelease(encoderSem);

	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to uvc encode stop\r\n");
	}

	return nRet;
}
#endif

/*== HOST USB MJPEG CAM / INTERP2K ==*/
#if (_SENSOR_USB_MJPEG_CAM) || (C_VID_INTERPOLATING_2K)
INT32S
host_2K_mjpeg_ready_notify(
	void
)
{
	INT32S nRet = STATUS_OK;

	if(!encoderTaskQ)
	{
		ERR_PRINT("encoderTaskQ is null\r\n");
		RETURN(STATUS_FAIL);
	}

	if(msgQSend(encoderTaskQ, MSG_HOST_2K_MJPEG_READY_NOTIFY, NULL, 0, MSG_PRI_NORMAL) < 0)
	{
		ERR_PRINT("fail to MSG_HOST_2K_MJPEG_READY_NOTIFY \r\n");
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}

INT32S
host_2K_frame_ready_notify(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;

	if(!encoderTaskQ)
	{
		ERR_PRINT("encoderTaskQ is null\r\n");
		RETURN(STATUS_FAIL);
	}

	if(msgQSend(encoderTaskQ, MSG_HOST_2K_FRAME_READY_NOTIFY|src, NULL, 0, MSG_PRI_NORMAL) < 0)
	{
		ERR_PRINT("fail to MSG_HOST_2K_FRAME_READY_NOTIFY \r\n");
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}

INT32S
host_2K_preview_done_notify(
	INT8U src
)
{
	INT32S nRet = STATUS_OK;

	if(!encoderTaskQ)
	{
		ERR_PRINT("encoderTaskQ is null\r\n");
		RETURN(STATUS_FAIL);
	}

	if(msgQSend(encoderTaskQ, MSG_HOST_2K_PREVIEW_DONE_NOTIFY|src, NULL, 0, MSG_PRI_NORMAL) < 0)
	{
		ERR_PRINT("fail to MSG_HOST_2K_PREVIEW_DONE_NOTIFY \r\n");
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}
#endif


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

#if (defined C_STORAGE_DEVICE) && (C_STORAGE_DEVICE)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define STORAGE_TASK_ACK_QUEUE_MAX_LEN		1

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern INT8U	device_plug_phase;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
storage_service_init(
	void
)
{
    TASK_APP_ARGUMENT* pStorageTaskArgs = &pTurnkeyArgs->taskStorage;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pStorageTaskArgs->taskAckQ == 0) {
		pStorageTaskArgs->taskAckQ = msgQCreate(STORAGE_TASK_ACK_QUEUE_MAX_LEN, STORAGE_TASK_ACK_QUEUE_MAX_LEN, 0);
		if(pStorageTaskArgs->taskAckQ == 0) {
			ERR_PRINT("fail to create pStorageTaskArgs->taskAckQ \r\n");
			return;
		}
	}

	if(pStorageTaskArgs->taskAppQ == 0) {
		pStorageTaskArgs->taskAppQ = msgQCreate(pStorageTaskArgs->taskMsgLen, pStorageTaskArgs->taskMsgLen, pStorageTaskArgs->taskRecvBufLen);
		if(pStorageTaskArgs->taskAppQ == 0) {
			ERR_PRINT("fail to create pStorageTaskArgs->taskAppQ\r\n");
			return;
		}

		if(pStorageTaskArgs->taskRecvBufLen)
		{
			pStorageTaskArgs->taskRecvBuf = (void*)malloc(pStorageTaskArgs->taskRecvBufLen);
			if(pStorageTaskArgs->taskRecvBuf == 0) {
				ERR_PRINT("fail to malloc pStorageTaskArgs->taskRecvBuf\r\n");
				goto ERR_STORAGE_TASK_INIT;
			}
		}
	}
	return;

ERR_STORAGE_TASK_INIT:
	if(pStorageTaskArgs->taskAppQ != 0)
	{
		msgQDelete(pStorageTaskArgs->taskAppQ);
		pStorageTaskArgs->taskAppQ = 0;
	}

	if(pStorageTaskArgs->taskAckQ)
	{
        msgQDelete(pStorageTaskArgs->taskAckQ);
        pStorageTaskArgs->taskAckQ = NULL;
	}
	return;

}

#if 0
static void
return_ack(
	INT32U ret_ack
)
{
	//DBG_PRINT("==Run:%s==ret_ack:%d==\r\n",__FUNCTION__,ret_ack);

    msgQSend(StorageServiceMsgAck, ret_ack, NULL, 0, MSG_PRI_NORMAL);
}
#endif

void
task_storage_service_init(
	void
)
{
	DBG_PRINT("%s ... \r\n", __func__);
	ap_storage_service_init();

	DBG_PRINT(" task_storage_service_init OK!\r\n");
}

void
task_storage_service_entry(
	void const *arg
)
{
	TASK_APP_ARGUMENT* pStorageTaskArgs;
	INT32U msg_para_len;
	void* msg_para;
	INT32U	msg_id;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

 	pStorageTaskArgs = &pTurnkeyArgs->taskStorage;
	msg_para = pStorageTaskArgs->taskRecvBuf;
	msg_para_len = pStorageTaskArgs->taskRecvBufLen;

	task_storage_service_init();

	DBG_PRINT("%s ... \r\n", __func__);

	while(1)
	{
		if(msgQReceive(StorageServiceQ, &msg_id, msg_para, msg_para_len) == STATUS_FAIL)
		{
			continue;
		}

		switch(msg_id)
		{
		case MSG_STORAGE_SERVICE_STORAGE_CHECK:
			//DBG_PRINT(" s_usbd_pin = 0x%x\r\n",s_usbd_pin);
			#if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_OFF))
			if(!s_usbd_pin)
			#endif
			{
				ap_storage_service_storage_mount();
			}

			break;

#if C_AUTO_DEL_FILE == CUSTOM_ON
		case MSG_STORAGE_SERVICE_FREESIZE_CHECK:
			ap_storage_service_freesize_check_and_del();
			break;

		case MSG_STORAGE_SERVICE_AUTODEL_LOCK:
			ap_storage_service_freesize_check_switch(FALSE);

			return_ack(msg_id);
			break;

		case MSG_STORAGE_SERVICE_AUTODEL_UNLOCK:
			ap_storage_service_freesize_check_switch(TRUE);

			return_ack(msg_id);
			break;
#endif // C_AUTO_DEL_FILE == CUSTOM_ON

		case MSG_STORAGE_SERVICE_VIDEO_FILE_DEL:
			ap_storage_service_file_del();
			break;

		case MSG_STORAGE_SERVICE_FILE_DEL_ALL:
			ap_storage_service_file_delete_all();
			break;

		case MSG_STORAGE_SERVICE_LOCK_ONE:
			ap_storage_service_file_lock_one();
			break;

		case MSG_STORAGE_SERVICE_LOCK_ALL:
			ap_storage_service_file_lock_all();
			break;

		case MSG_STORAGE_SERVICE_UNLOCK_ONE:
			ap_storage_service_file_unlock_one();
			break;

		case MSG_STORAGE_SERVICE_UNLOCK_ALL:
			ap_storage_service_file_unlock_all();
			break;

		case MSG_STORAGE_SERVICE_TIMER_START:
			ap_storage_service_timer_start();
			break;

		case MSG_STORAGE_SERVICE_TIMER_STOP:
			ap_storage_service_timer_stop();
			break;

		case MSG_STORAGE_SERVICE_AUD_REQ:
		case MSG_STORAGE_SERVICE_PIC_REQ:
		case MSG_STORAGE_SERVICE_PIC_REQ2:
		case MSG_STORAGE_SERVICE_VID_H264_REQ:
			ap_storage_service_file_open_handle(msg_id, *(INT16U *) msg_para);
			break;

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
		case MSG_STORAGE_SERVICE_VID_H264_CYCLIC_REQ:
			ap_storage_service_cyclic_record_file_open_handle(msg_id);
			break;
#endif

		case MSG_STORAGE_SERVICE_THUMBNAIL_REQ:
		case MSG_STORAGE_SERVICE_BROWSE_REQ:
			ap_storage_service_play_req(*(INT32U *) msg_para);
			break;

		case MSG_STORAGE_SERVICE_FORMAT_REQ:
			ap_storage_service_format_req();
			break;

		case MSG_STORAGE_USBD_EXIT:
		case MSG_STORAGE_USBD_PCAM_EXIT:
			device_plug_phase = 0;

			//usbd_storage_exit = 1;
			disk_safe_exit(STORAGE_DEVICE);
			break;
		case MSG_STORAGE_SERVICE_WRITE_FILE:
			ap_storage_service_write_file_process();
			break;
		case MSG_STORAGE_SERVICE_CLOSE_FILE:
			ap_storage_service_close_file_process();
			break;

		default:
			break;
		}
	}
}

#endif

INT32S
storage_task_msg_send(
    INT32U msg, void *para, INT32U bytes, INT32U prio
)
{
	INT32S nRet;

	#if (defined C_STORAGE_DEVICE) && (C_STORAGE_DEVICE)
	if(StorageServiceQ == NULL)
	{
		nRet = -1;
		ERR_PRINT("send msg fail\r\n");
	}
	else
	{
		nRet = msgQSend(StorageServiceQ, msg, para, bytes, prio);
	}
	#else
	nRet = -1;
	ERR_PRINT("C_STORAGE_DEVICE Not enable\r\n");
	#endif
    return nRet;
}

INT32S
storage_task_msg_send_with_ack(
    INT32U msg,
	INT32U msc_timeout
)
{
	INT32S nRet;

	#if (defined C_STORAGE_DEVICE) && (C_STORAGE_DEVICE)
	if(StorageServiceQ == NULL)
	{
		nRet = -1;
		ERR_PRINT("send msg fail\r\n");
	}
	else
	{
		nRet = _post_message(StorageServiceQ, msg, StorageServiceMsgAck, msc_timeout, MSG_PRI_NORMAL);
	}
	#else
	nRet = -1;
	ERR_PRINT("C_STORAGE_DEVICE Not enable\r\n");
	#endif
	return nRet;
}


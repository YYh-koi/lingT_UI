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
#include "bt_music_art.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
state_handling_task_init(
	void
)
{
	TASK_APP_ARGUMENT* pStateTaskArgs = &pTurnkeyArgs->taskStateHdlg;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pStateTaskArgs->taskAppQ == 0) {
		pStateTaskArgs->taskAppQ = msgQCreate(pStateTaskArgs->taskMsgLen, pStateTaskArgs->taskMsgLen, pStateTaskArgs->taskRecvBufLen);
		if(pStateTaskArgs->taskAppQ == 0) {
			ERR_PRINT("fail to create pStateHdlgTaskQ \r\n");
			return;
		}

		if(pStateTaskArgs->taskRecvBufLen)
		{
			pStateTaskArgs->taskRecvBuf = (void*)malloc(pStateTaskArgs->taskRecvBufLen);
			if(pStateTaskArgs->taskRecvBuf == 0) {
				ERR_PRINT("fail to malloc pStateHdlgRecvBuf \r\n");
				goto ERR_STATE_HANDLING_TASK_INIT;
			}
		}
	}
	return;

ERR_STATE_HANDLING_TASK_INIT:
	if(pStateTaskArgs->taskAppQ != 0)
	{
		msgQDelete(pStateTaskArgs->taskAppQ);
		pStateTaskArgs->taskAppQ = 0;
	}

	if(pStateTaskArgs->taskRecvBuf)
	{
		free(pStateTaskArgs->taskRecvBuf);
		pStateTaskArgs->taskRecvBuf = 0;
	}
}

void
state_app_init(
	void
)
{
	TASK_APP_ARGUMENT* pStateAppArgs = &pTurnkeyArgs->taskStateApp;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pStateAppArgs->taskAppQ == 0) {
		pStateAppArgs->taskAppQ = msgQCreate(pStateAppArgs->taskMsgLen, pStateAppArgs->taskMsgLen, pStateAppArgs->taskRecvBufLen);
		if(pStateAppArgs->taskAppQ == 0) {
			ERR_PRINT("fail to create pStateAppArgs->taskAppQ \r\n");
			return;
		}

		if(pStateAppArgs->taskRecvBufLen)
		{
			pStateAppArgs->taskRecvBuf = (void*)malloc(pStateAppArgs->taskRecvBufLen);
			if(pStateAppArgs->taskRecvBuf == 0) {
				ERR_PRINT("fail to malloc pStateAppArgs->taskRecvBuf \r\n");
				goto ERR_STATE_APP_INIT;
			}
		}

		pStateAppArgs->taskPrivateData = (void*)malloc(sizeof(STREAM_ARGUMENT));
		if(pStateAppArgs->taskPrivateData == 0) {
			ERR_PRINT("fail to malloc pStateAppArgs->taskPrivateData \r\n");
			goto ERR_STATE_APP_INIT;
		}
	}
	return;

ERR_STATE_APP_INIT:
	if(pStateAppArgs->taskAppQ != 0)
	{
		msgQDelete(pStateAppArgs->taskAppQ);
		pStateAppArgs->taskAppQ = 0;
	}

	if(pStateAppArgs->taskRecvBuf)
	{
		free(pStateAppArgs->taskRecvBuf);
		pStateAppArgs->taskRecvBuf = 0;
	}
}

void
state_handling_init(
	void
)
{
	INT32S config_load_flag;

	DBG_PRINT("%s ... \r\n", __func__);

	overlap_state_setting_function_initial();

	nvmemory_init();
	config_load_flag = ap_state_config_load();
	ap_state_resource_init();
	ap_state_config_initial(config_load_flag);

    // for test
	//ap_state_config_video_resolution_set(4);	//0:4K, 1:2K, 2:1080FHD, 3:1080P, 4:720P

	#if defined(ENABLE_CHECK_RTC) && ENABLE_CHECK_RTC
	if(config_load_flag != STATUS_FAIL)
	{
		ap_state_config_date_time_check();
	}
	#endif

	ap_state_handling_calendar_init();

	//ap_state_handling_storage_id_set(NO_STORAGE);

	DBG_PRINT("state_handling_init OK!\r\n");
}

void
state_handling_entry(
	void const *arg
)
{
	TASK_APP_ARGUMENT* pStateHdlgTaskArgs;
	INT32U stateRecvBufLen;
	void* stateRecvBuf;
	INT32U	msg_id, prev_state;

	pStateHdlgTaskArgs = &pTurnkeyArgs->taskStateHdlg;
	stateRecvBuf = pStateHdlgTaskArgs->taskRecvBuf;
	stateRecvBufLen = pStateHdlgTaskArgs->taskRecvBufLen;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	state_handling_init();

	if(StateHandlingQ)
	{
		msgQSend(StateHandlingQ,STATE_STARTUP, stateRecvBuf, stateRecvBufLen, MSG_PRI_NORMAL);
	}

	present_state = 0;

	DBG_PRINT("%s ... \r\n", __func__);

	while(1)
	{
		prev_state = present_state;
		if (msgQReceive(StateHandlingQ, &msg_id, stateRecvBuf, stateRecvBufLen) == STATUS_FAIL)
		{
			continue;
		}

		present_state = msg_id & 0x1FF;

		switch(msg_id & 0x1FF)
		{
		case STATE_STARTUP:
			state_startup_entry((void *)&prev_state);
			break;

		case STATE_PHOTO_CAPTURE:
			state_photo_capture_entry((void *)&prev_state);
			break;

		case STATE_VIDEO_RECORD:
			state_video_record_entry((void *)&prev_state);
			break;

		case STATE_BROWSE:
			state_browse_entry((void *)&prev_state, (msg_id & 0xFFFF0000) >> 16);

			#if 0 && CRAZY_KEY_TEST // Set random number
			ap_state_config_video_resolution_set(R_RANDOM0 & 7);
			ap_state_config_pic_size_set(R_RANDOM0 & 7);
			#endif

			break;

		case STATE_BT_MUSIC:
			bt_music_art_state_entry((void *)&prev_state);
			break;
        #if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
        case STATE_AUDIO_RECORD:
            state_audio_record_entry((void *)&prev_state);
            break;
        #endif
		#if 0
		case STATE_THUMBNAIL:
			state_thumbnail_entry((void *) &prev_state);
			break;
		#endif

		case STATE_SETTING:
			State_Seting.state_setting_entry_ptr((void *)&prev_state);
			break;

		case STATE_FIRMWARE_UPGRADE:
			ap_state_firmware_upgrade();
			break;

		default:
			break;
		}
	}
}

INT32S
app_msg_send(
    INT32U msg, void *para, INT32U bytes, INT32U prio
)
{
    INT32S nRet;

    if(ApQ == NULL)
    {
        nRet = -1;
        ERR_PRINT("send msg fail\r\n");
   }
    else
    {
        nRet = msgQSend(ApQ, msg, para, bytes, prio);
    }
    return nRet;
}

INT32S
state_handling_msg_send(
    INT32U msg, void *para, INT32U bytes, INT32U prio
)
{
    INT32S nRet;

    if(StateHandlingQ == NULL)
    {
        nRet = -1;
        ERR_PRINT("send msg fail\r\n");
   }
    else
    {
        nRet = msgQSend(StateHandlingQ, msg, para, bytes, prio);
    }
    return nRet;
}

INT32S
__check_file_handle(
	STOR_SERV_FILEINFO* file_info
)
{
	INT32S nRet = STATUS_OK;

	VID_INFO*  pVid_info_front = &pStreamArgs->vid_info_front;
	VID_INFO*  pVid_info_rear = &pStreamArgs->vid_info_rear;

	if(pVid_info_front->data_source != VID_SRC_NONE)
	{
		pVid_info_front->rec_file_handle = -1;

		if(file_info->file_handle < 0)
		{
            osSuspend();
			ERR_PRINT("[Front] file_handle is less than 0 \r\n");
			osResume();
			RETURN(STATUS_FAIL);
		}

		pVid_info_front->rec_file_handle = file_info->file_handle;

        osSuspend();
		DBG_PRINT("file_info->file_handle=%d\r\n",file_info->file_handle);
		osResume();
	}

	if((back_sensor_plug_status_get())&&(pVid_info_rear->data_source != VID_SRC_NONE))
	{
		pVid_info_rear->rec_file_handle = -1;
		if(file_info->file_handle_b < 0)
		{
            osSuspend();
			ERR_PRINT("[Rear] file_handle is less than 0 \r\n");
			osResume();
			RETURN(STATUS_FAIL);
		}

		pVid_info_rear->rec_file_handle = file_info->file_handle_b;

        osSuspend();
		DBG_PRINT("file_info->file_handle_b=%d\r\n",file_info->file_handle_b);
		osResume();
	}

Return:
	if(nRet == STATUS_FAIL)
	{
		if(pVid_info_front->data_source != VID_SRC_NONE)
		{
            if(pVid_info_front->rec_file_handle > -1)
            {
                close(pVid_info_front->rec_file_handle);
            }
			pVid_info_front->rec_file_handle = -1;
		}

		if(pVid_info_rear->data_source != VID_SRC_NONE)
		{
            if(pVid_info_rear->rec_file_handle > -1)
            {
                close(pVid_info_rear->rec_file_handle);
            }
			pVid_info_rear->rec_file_handle = -1;
		}
	}

	return nRet;
}


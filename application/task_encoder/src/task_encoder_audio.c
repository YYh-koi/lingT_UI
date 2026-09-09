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
#define AUDIO_ENCODE_STATE_ACK_QUEUE_MAX_LEN		1


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern INT32S ap_audio_record_device_open(void);
extern INT32S ap_audio_record_device_close(void);
extern INT32U ap_audio_done_buf(void);
extern INT32S ap_audio_record_buf_exchange(void);
extern INT32S ap_audio_capture(INT32U p_pcm);

extern INT32S ap_audio_record_buf_init(void);
extern INT32S ap_audio_record_buf_release(void);

extern INT32U  fps_audrec_dma;


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT32U AudEncStateFlag = 0;

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

    msgQSend(audioEncAckQ, ret_ack, NULL, 0, MSG_PRI_NORMAL);
}

void
audio_encode_state_task_init(
	void
)
{
	TASK_APP_ARGUMENT* pAudioEncTaskArgs = &pTurnkeyArgs->taskAudioEnc;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pAudioEncTaskArgs->taskAckQ == 0) {
		pAudioEncTaskArgs->taskAckQ = msgQCreate(AUDIO_ENCODE_STATE_ACK_QUEUE_MAX_LEN, AUDIO_ENCODE_STATE_ACK_QUEUE_MAX_LEN, 0);
		if(pAudioEncTaskArgs->taskAckQ == 0) {
			ERR_PRINT("fail to create pAudioEncTaskArgs->taskAckQ\r\n");
			return;
		}
	}

	if(pAudioEncTaskArgs->taskAppQ == 0) {
		pAudioEncTaskArgs->taskAppQ = msgQCreate(pAudioEncTaskArgs->taskMsgLen, pAudioEncTaskArgs->taskMsgLen, pAudioEncTaskArgs->taskRecvBufLen);
		if(pAudioEncTaskArgs->taskAppQ == 0) {
			ERR_PRINT("fail to create pAudioEncTaskArgs->taskAppQ\r\n");
			goto ERR_AUDIO_ENC_STATE_TASK_INIT;
		}

		if(pAudioEncTaskArgs->taskRecvBufLen)
		{
			pAudioEncTaskArgs->taskRecvBuf = (void*)gp_malloc(pAudioEncTaskArgs->taskRecvBufLen);
			if(pAudioEncTaskArgs->taskRecvBuf == 0) {
				ERR_PRINT("fail to malloc pAudioEncTaskArgs->taskRecvBuf\r\n");
				goto ERR_AUDIO_ENC_STATE_TASK_INIT;
			}
		}
	}

	if(ap_audio_record_buf_init() == STATUS_FAIL)
	{
		goto ERR_AUDIO_ENC_STATE_TASK_INIT;
	}
	return;

ERR_AUDIO_ENC_STATE_TASK_INIT:
	if(pAudioEncTaskArgs->taskAckQ != 0)
	{
		msgQDelete(pAudioEncTaskArgs->taskAckQ);
		pAudioEncTaskArgs->taskAckQ = 0;
	}

	if(pAudioEncTaskArgs->taskAppQ != 0)
	{
		msgQDelete(pAudioEncTaskArgs->taskAppQ);
		pAudioEncTaskArgs->taskAppQ = 0;
	}

	if(pAudioEncTaskArgs->taskRecvBuf)
	{
		free(pAudioEncTaskArgs->taskRecvBuf);
		pAudioEncTaskArgs->taskRecvBuf = 0;
	}

    ap_audio_record_buf_release();
	return;
}

static void
audio_encode_state_task_entry(
	void const *arg
)
{
	TASK_APP_ARGUMENT* pAudioEncTaskArgs;
	INT32U audioEncRecvBufLen;
	void* pAudioEncRecvBuf;
	INT32U msg_id;
	INT32U pcm_addr;
	INT8U bStop = 0;

 	pAudioEncTaskArgs = &pTurnkeyArgs->taskAudioEnc;
	pAudioEncRecvBuf = pAudioEncTaskArgs->taskRecvBuf;
	audioEncRecvBufLen = pAudioEncTaskArgs->taskRecvBufLen;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	while(1)
	{
		if (msgQReceive(audioEncTaskQ, &msg_id, pAudioEncRecvBuf, audioEncRecvBufLen) == STATUS_FAIL)
		{
			continue;
		}

		//DBG_PRINT("==Run:%s==msg_id:0x%x==\r\n",__FUNCTION__,msg_id);

		switch(msg_id)
		{
		case MSG_AUD_DMA_DONE:
            fps_audrec_dma++;
			if(bStop)
			{
				msgQFlush(audioEncTaskQ);
				return_ack(ap_audio_record_device_close());
				break;
			}

			pcm_addr = ap_audio_done_buf();
			ap_audio_record_buf_exchange();
			ap_audio_capture(pcm_addr);
			break;

		case MSG_AUD_ENCODE_START:
            bStop = 0;
			DBG_PRINT("MSG_AUD_ENCODE_START\r\n");
            return_ack(ap_audio_record_device_open());
			break;

		case MSG_AUD_ENCODE_STOP:
			bStop = 1;
			break;
		}
	}
}

INT32S
audio_encode_state_task_create(
	void
)
{
	INT32S nRet = STATUS_OK;
	osThreadId id;
	osThreadDef_t audio_enc_task = {"audio_enc_state_task", audio_encode_state_task_entry, PRIORITY_AUDIO_ENC_TASK, 1, AUDIO_ENCODE_STATE_STACK_SIZE};

	id = osThreadCreate(&audio_enc_task, NULL);
	if(id == 0) {
		RETURN(STATUS_FAIL);
	}

	nRet = STATUS_OK;

Return:
	return nRet;
}

void
audio_encode_state_set(
	AUDIO_ENCODE_STATUS_OFFSET stateOffset
)
{
	if(ENCODE_STATUS_IS_BUSY(AudEncStateFlag,stateOffset))
	{
		ERR_PRINT("state flag:0x%x==offset:%d is set\r\n",AudEncStateFlag,stateOffset);
		return;
	}

	osSuspend();
	ENCODE_STATUS_SET_BUSY(AudEncStateFlag,stateOffset);
	osResume();
}

void
audio_encode_state_clear(
	AUDIO_ENCODE_STATUS_OFFSET stateOffset
)
{
	if(ENCODE_STATUS_IS_BUSY(AudEncStateFlag,stateOffset) == 0)
	{
		ERR_PRINT("state flag:0x%x==offset:%d is not set\r\n",AudEncStateFlag,stateOffset);
		return;
	}

	osSuspend();
	ENCODE_STATUS_CLR_BUSY(AudEncStateFlag,stateOffset);
	osResume();
}

INT32S
audio_encode_state_get(
	AUDIO_ENCODE_STATUS_OFFSET stateOffset
)
{
	return (ENCODE_STATUS_IS_BUSY(AudEncStateFlag,stateOffset));
}

INT32S
audio_encode_state_encode_start(
	void
)
{
	INT32S nRet = STATUS_OK;
	DBG_PRINT("==%s==\r\n",__FUNCTION__);
	if(audio_encode_state_get(AUDIO_ENCODE_STATUS_ENCODE_OFFSET) == 0)
	{
		POST_MESSAGE(audioEncTaskQ, MSG_AUD_ENCODE_START, audioEncAckQ, 5000, MSG_PRI_NORMAL);
		audio_encode_state_set(AUDIO_ENCODE_STATUS_ENCODE_OFFSET);
	}

Return:
	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to audio encode start\r\n");
	}

	return nRet;
}

INT32S
audio_encode_state_encode_stop(
	void
)
{
	INT32S nRet = STATUS_OK;

	#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
		return nRet;
	#endif

	if(audio_encode_state_get(AUDIO_ENCODE_STATUS_ENCODE_OFFSET))
	{
		POST_MESSAGE(audioEncTaskQ, MSG_AUD_ENCODE_STOP, audioEncAckQ, 5000, MSG_PRI_NORMAL);
		audio_encode_state_clear(AUDIO_ENCODE_STATUS_ENCODE_OFFSET);
	}

Return:
	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to audio encode stop\r\n");
	}

	return nRet;
}


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
#include "ap_voice_recg_func.h"

#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
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
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern void ap_audio_free_cap_buffer(INT32U data);
extern INT32S audio_encode_state_encode_start(void);

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
voice_recg_state_task_init(
	void
)
{
	TASK_APP_ARGUMENT* pVoiceRecgTaskArgs = &pTurnkeyArgs->taskVoiceRecg;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pVoiceRecgTaskArgs->taskAppQ == 0) {
		pVoiceRecgTaskArgs->taskAppQ = msgQCreate(pVoiceRecgTaskArgs->taskMsgLen, pVoiceRecgTaskArgs->taskMsgLen, pVoiceRecgTaskArgs->taskRecvBufLen);
		if(pVoiceRecgTaskArgs->taskAppQ == 0) {
			ERR_PRINT("fail to create pVoiceRecgTaskArgs->taskAppQ \r\n");
			goto ERR_VOICE_RECG_STATE_TASK_INIT;
		}

		if(pVoiceRecgTaskArgs->taskRecvBufLen)
		{
			pVoiceRecgTaskArgs->taskRecvBuf = (void*)malloc(pVoiceRecgTaskArgs->taskRecvBufLen);
			if(pVoiceRecgTaskArgs->taskRecvBuf == 0) {
				ERR_PRINT("fail to malloc pVoiceRecgTaskArgs->taskRecvBuf\r\n");
				goto ERR_VOICE_RECG_STATE_TASK_INIT;
			}
		}
	}
	return;

ERR_VOICE_RECG_STATE_TASK_INIT:
	if(pVoiceRecgTaskArgs->taskAckQ != 0)
	{
		msgQDelete(pVoiceRecgTaskArgs->taskAckQ);
		pVoiceRecgTaskArgs->taskAckQ = 0;
	}

	if(pVoiceRecgTaskArgs->taskAppQ != 0)
	{
		msgQDelete(pVoiceRecgTaskArgs->taskAppQ);
		pVoiceRecgTaskArgs->taskAppQ = 0;
	}

	if(pVoiceRecgTaskArgs->taskRecvBuf)
	{
		free(pVoiceRecgTaskArgs->taskRecvBuf);
		pVoiceRecgTaskArgs->taskRecvBuf = 0;
	}
	return;
}

void
voice_recg_MSG_VOICE_RECG_PROCESS(
	INT32U p
)
{
	short* p_pcm = (short*)(p+sizeof(AUD_CAP_BUF_HEADER));
	ap_voice_recg_process(p_pcm,SND_REC_SAMPLING_FRAME);
	ap_audio_free_cap_buffer((INT32U)p);
}

void
voice_recg_task_entry(
	void const *para
)
{
	TASK_APP_ARGUMENT* pVoiceRecgArgs;
	INT32U voiceRecvBufLen;
	void* voiceRecvBuf;
	INT32U  msg_id;

	pVoiceRecgArgs = &pTurnkeyArgs->taskVoiceRecg;
	voiceRecvBufLen = pVoiceRecgArgs->taskRecvBufLen;
	voiceRecvBuf = pVoiceRecgArgs->taskRecvBuf;

	ap_voice_recg_start();

	audio_encode_state_encode_start();

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	while(1)
	{
		if(msgQReceive(voiceRecgTaskQ, &msg_id, voiceRecvBuf, voiceRecvBufLen) == STATUS_FAIL)
		{
            DBG_PRINT("voice_recg_task_entry() receive err\r\n");
			continue;
		}

		//DBG_PRINT("==Run:%s==msg_id:0x%x==\r\n",__FUNCTION__,msg_id);

		switch(msg_id)
		{
			case MSG_VOICE_RECG_PROCESS:
				voice_recg_MSG_VOICE_RECG_PROCESS(*(INT32U *)voiceRecvBuf);
			break;

	        default:
	            DBG_PRINT("voice_recg_task_entry() unknown msg = 0x%x\r\n", msg_id);
	        break;
		}
	}

}

INT32S
voice_recg_task_msg_send(
    INT32U msg, void *para, INT32U bytes, INT32U prio
)
{
    INT32S nRet;

    if(voiceRecgTaskQ == NULL)
    {
        nRet = -1;
        ERR_PRINT("send msg fail\r\n");
   }
    else
    {
        nRet = msgQSend(voiceRecgTaskQ, msg, para, bytes, prio);
    }
    return nRet;
}

INT32S
voice_recg_task_process(
	INT32U p_pcm
)
{
	AUD_CAP_BUF_HEADER* p_aud_cap_buf_header = (AUD_CAP_BUF_HEADER*)p_pcm;

	osSuspend();
	p_aud_cap_buf_header->copies++;
	osResume();

	if(voice_recg_task_msg_send(MSG_VOICE_RECG_PROCESS, &p_pcm, sizeof(INT32U), MSG_PRI_NORMAL) == STATUS_FAIL)
	{
        ap_audio_free_cap_buffer((INT32U)p_aud_cap_buf_header);
	}

	return STATUS_OK;
}

#endif //#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)

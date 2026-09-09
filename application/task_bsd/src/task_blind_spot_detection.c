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
#include "task_blind_spot_detection.h"
#include "ap_blind_spot_detection.h"

#if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
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

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
INT32U bsd_gray_buf;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
bsd_task_init(
	void
)
{
	TASK_APP_ARGUMENT* pBSDTaskArgs = &pTurnkeyArgs->taskBSD;

	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pBSDTaskArgs->taskAppQ == 0) {
		pBSDTaskArgs->taskAppQ = msgQCreate(pBSDTaskArgs->taskMsgLen, pBSDTaskArgs->taskMsgLen, pBSDTaskArgs->taskRecvBufLen);
		if(pBSDTaskArgs->taskAppQ == 0) {
			ERR_PRINT("fail to create pBSDTaskArgs->taskAppQ \r\n");
			goto ERR_BSD_TASK_INIT;
		}

		if(pBSDTaskArgs->taskRecvBufLen)
		{
			pBSDTaskArgs->taskRecvBuf = (void*)malloc(pBSDTaskArgs->taskRecvBufLen);
			if(pBSDTaskArgs->taskRecvBuf == 0) {
				ERR_PRINT("fail to malloc pBSDTaskArgs->taskRecvBuf\r\n");
				goto ERR_BSD_TASK_INIT;
			}
		}
	}
	return;

ERR_BSD_TASK_INIT:
	if(pBSDTaskArgs->taskAckQ != 0)
	{
		msgQDelete(pBSDTaskArgs->taskAckQ);
		pBSDTaskArgs->taskAckQ = 0;
	}

	if(pBSDTaskArgs->taskAppQ != 0)
	{
		msgQDelete(pBSDTaskArgs->taskAppQ);
		pBSDTaskArgs->taskAppQ = 0;
	}

	if(pBSDTaskArgs->taskRecvBuf)
	{
		free(pBSDTaskArgs->taskRecvBuf);
		pBSDTaskArgs->taskRecvBuf = 0;
	}
	return;
}

void
bsd_task_entry(
	void const *para
)
{
	TASK_APP_ARGUMENT* pBSDArgs;
	INT32U bsdRecvBufLen;
	void* bsdRecvBuf;
	INT32U  msg_id, temp_bufa, temp_bufb;
	INT32S nRet;

	pBSDArgs = &pTurnkeyArgs->taskBSD;
	bsdRecvBufLen = pBSDArgs->taskRecvBufLen;
	bsdRecvBuf = pBSDArgs->taskRecvBuf;

	bsd_q_init();

	CollisionAvoidance();

	// sensitivity
	// arg1: 1500
	// arg2: 4
	GP_Driving_Set(TTC_TIMETHRE, LDW_THRE1, LDW_THRE2, LDW_THRE3, LDW_THRE4, MISS_THRE);
	// pedestrian
	// arg1: 30 ~ 50
	GP_pedestrian_set(PEDESTRIAN_THRE);
	// virtual_bumper
	// arg1: 3
	GP_bumper_set(VIRTUAL_BUMPERTHRE);
	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	osDelay(1000);

	while(1)
	{
		if(msgQReceive(bsdTaskQ, &msg_id, bsdRecvBuf, bsdRecvBufLen) == STATUS_FAIL)
		{
            DBG_PRINT("bsd_task_entry() receive err\r\n");
			continue;
		}

		//DBG_PRINT("==Run:%s==msg_id:0x%x==\r\n",__FUNCTION__,msg_id);

		switch(msg_id)
		{
			case MSG_BSD_PROCESS:
				if((state_wifi_status_get() & FLAG_CONNECT) &&
					(state_wifi_status_get() & FLAG_OFF_DISPLAY)
				)
				{
					GPDLA_Process(0);
				}
				else if(present_state == STATE_VIDEO_RECORD) {
					 GPDLA_Process(1);
				}
				else GPDLA_Process(0);
			break;
	        default:
	            DBG_PRINT("bsd_task_entry() unknown msg = 0x%x\r\n", msg_id);
	        break;
		}
	}

	CollisionAvoidance_Exit();

}

INT32S
bsd_task_msg_send(
    INT32U msg, void *para, INT32U bytes, INT32U prio
)
{
    INT32S nRet;

    if(bsdTaskQ == NULL)
    {
        nRet = -1;
        ERR_PRINT("bsd send msg fail\r\n");
    }
    else
    {
        nRet = msgQSend(bsdTaskQ, msg, para, bytes, prio);
    }
    return nRet;
}

#endif //#if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)

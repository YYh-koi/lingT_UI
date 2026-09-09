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

/*
* Purpose: message q manager
*
* Author: zhangzha
*
* Date: 2008/05/29
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 0.01
* History :
*          1 2008/5/29 zhangzha creat
*/

#include "application.h"

#define MSG_Q_DEBUG			0

#define MAX_Q_SIZE          64

#define MSG_Q_LOCK()			osMutexWait(msgQId->lock, osWaitForever)
#define MSG_Q_UNLOCK()			osMutexRelease(msgQId->lock)

/**
 * @brief   create and initialize a message queue
 * @param   maxQSize:			max queue can be creat
 * @param   maxMsgs:			max messages that can be queued
 * @param   maxMsgLength:		max bytes in a message
 * @return 	NULL if faile
 */
MSG_Q_ID msgQCreate(INT32U maxQSize, INT32U maxMsgs, INT32U maxMsgLength)
{
    MSG_Q_ID msgQId = NULL;
    xQueueHandle queue = NULL;
    osMutexId lock = NULL;

    if(maxMsgs == 0)
    {
        goto __fail;
    }

    if(maxQSize > maxMsgs) {
        maxMsgs = maxQSize;
    }

    if((maxMsgLength + 4) > MAX_Q_SIZE)
    {
        ERR_PRINT("msg too long\r\n");
        goto __fail;
    }

    msgQId = (MSG_Q_ID)gp_malloc_align(sizeof(*msgQId), 4);
	if(msgQId == NULL) {
		goto __fail;
	}

    queue = xQueueCreate(maxMsgs, 4 + maxMsgLength);
	if(queue == NULL) {
        goto __fail;
    }

    lock = osMutexCreate(NULL);
    if (lock == NULL) {
		goto __fail;
    }

	msgQId->maxMsgs = maxMsgs;
	msgQId->maxMsgLength = maxMsgLength;
	msgQId->queue = queue;
    msgQId->lock = lock;

    return msgQId;

__fail:
	if(lock)
	{
        osMutexDelete(lock);
        lock = NULL;
	}

	if(queue)
	{
        vQueueDelete(queue);
        queue = NULL;
	}

	if(msgQId) {
    	gp_free((void *)msgQId);
    	msgQId = NULL;
	}

#if MSG_Q_DEBUG == 1
	DBG_PRINT("msg create err:queue create fail\r\n");
#endif
	return NULL;
}

/**
 * @brief   delete a message queue
 * @param   msgQId:				message queue to delete
 * @return 	none
 */
void msgQDelete (MSG_Q_ID msgQId)
{
    if(msgQId)
    {
        vQueueDelete(msgQId->queue);
        osMutexDelete(msgQId->lock);
        gp_free((void *)msgQId);
    }
}


/**
 * @brief   send a message to a message queue
 * @param   msgQId:			message queue on which to send
 * @param   msg_id:			message id
 * @param   para:			message to send
 * @param   nParaByte:		byte number of para buffer
 * @param   priority:		MSG_PRI_NORMAL or MSG_PRI_URGENT
 * @return 	0 SUCCESS/-1 FAIL
 */
INT32S msgQSend(MSG_Q_ID msgQId, INT32U msg_id, void *para, INT32U nParaByte, INT32U priority)
{
    INT32S  ret;
    ALIGN4  INT8U   ptr[MAX_Q_SIZE];
    INT32U  *ptr32;

    if(nParaByte > msgQId->maxMsgLength) {
		return -1;
	}

	if((nParaByte != 0) && (para == NULL))
	{
		return -1;
	}

    ptr32 = (INT32U *)ptr;
    *ptr32 = msg_id;
    gp_memcpy((INT8S *)ptr+4, (INT8S *)para, nParaByte);

    // put message
	if(priority == MSG_PRI_NORMAL) {
		ret = osMessagePut(msgQId->queue, (uint32_t)ptr, 100);
	}
    else {
		ret = osMessagePutToFront(msgQId->queue, (uint32_t)ptr, 100);
    }

	if(ret != osOK) {
   		goto __fail;
    }

    return 0;

__fail:

#if MSG_Q_DEBUG == 1
	DBG_PRINT("msg send err:message send fail\r\n");
#endif
    return -1;
}

/**
 * @brief   receive a message from a message queue
 * @param   msgQId:			message queue on which to receive
 * @param   msg_id:			message id
 * @param   para:			message and type received
 * @param   maxNByte:		message size
 * @return 	0 SUCCESS/-1 FAIL
 */
INT32S msgQReceive(MSG_Q_ID msgQId, INT32U *msg_id, void *para, INT32U nBytes)
{
    INT32S ret;
    ALIGN4 INT8U ptr[MAX_Q_SIZE];
    INT32U *ptr32;

    if(nBytes != msgQId->maxMsgLength) {
		goto __fail;
	}

	if((nBytes != 0) && (para == NULL))
	{
        goto __fail;
	}

	ret = (INT32S)xQueueReceive(msgQId->queue, (void *)ptr, portMAX_DELAY);
    if(ret != pdPASS) {
        goto __fail;
    }

    ptr32 = (INT32U *)ptr;
    *msg_id = *ptr32;
    gp_memcpy((INT8S *)para, (INT8S *)ptr + 4, nBytes);
    return 0;

__fail:
	*msg_id = 0;

#if MSG_Q_DEBUG == 1
	DBG_PRINT("msg recv err:message recv fail\r\n");
#endif
    return -1;
}

/**
 * @brief   Check whether a message is available from a message queue
 * @param   msgQId:			message queue on which to check
 * @param   msg_id:			message id
 * @param   para:			message and type received
 * @param   maxNByte:		message size
 * @return 	0 SUCCESS/-1 FAIL (queue is empty or fail)
 */
INT32S msgQAccept(MSG_Q_ID msgQId, INT32U *msg_id, void *para, INT32U nBytes)
{
    INT32S ret;
    ALIGN4  INT8U ptr[MAX_Q_SIZE];
    INT32U  *ptr32;

    if(nBytes != msgQId->maxMsgLength) {
		goto __fail;
	}

	if((nBytes != 0) && (para == NULL))
	{
        goto __fail;
	}

	ret = (INT32S)xQueueReceive(msgQId->queue, (void *)ptr, 0);
    if(ret != pdPASS) {
        goto __fail;
    }

    ptr32 = (INT32U *)ptr;
    *msg_id = *ptr32;
    gp_memcpy((INT8S *)para, (INT8S *)ptr + 4, nBytes);
    return 0;

__fail:
    *msg_id = 0;

#if MSG_Q_DEBUG == 1
	DBG_PRINT("msg accept err:message accept fail\r\n");
#endif
    return -1;

}

/**
 * @brief   flush message queue
 * @param   msgQId:			message queue on which to flush
 * @return 	none
 */
void msgQFlush(MSG_Q_ID msgQId)
{
    if(msgQId)
    {
        xQueueReset(msgQId->queue);
    }
}

INT32S msgQReceive_timeout(MSG_Q_ID msgQId, INT32U *msg_id, void *para, INT32U nBytes, INT32U timeOut)
{
    INT32S  ret;
    ALIGN4  INT8U ptr[MAX_Q_SIZE];
    INT32U  *ptr32;

    if(nBytes != msgQId->maxMsgLength) {
		goto __fail;
	}

	if((nBytes != 0) && (para == NULL))
	{
        goto __fail;
	}

	ret = (INT32S)xQueueReceive(msgQId->queue, (void *)ptr, timeOut);
    if(ret != pdPASS) {
        goto __fail;
    }

    ptr32 = (INT32U *)ptr;
    *msg_id = *ptr32;
    gp_memcpy((INT8S *)para, (INT8S *)ptr + 4, nBytes);
    return 0;

__fail:
	*msg_id = 0;
	return -1;
}

INT32S _post_message(
	MSG_Q_ID msg_queue,
	INT32U message,
	MSG_Q_ID ack_mbox,
	INT32U msc_time,
	INT32U prio
)
{
	INT32U msg;
	INT32S err;

	msgQFlush(ack_mbox);
    err = msgQSend(msg_queue, message, NULL, 0, prio);
    if(err != 0)
    {
        DBG_PRINT("OSQPost Fail!!!:0x%x 0x%x\r\n",message,err);
        return STATUS_FAIL;
    }

	err = msgQReceive_timeout(ack_mbox, &msg, NULL, 0, msc_time);
	if(err != 0 || msg == C_ACK_FAIL)
	{
		DBG_PRINT("OSMbox ack Fail!!!:0x%x 0x%x\r\n",message,err);
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

INT32U
msgQSizeGet(
	MSG_Q_ID msgQId
)
{
	UBaseType_t ret;

	if(msgQId && msgQId->queue)
	{
		ret = uxQueueMessagesWaiting(msgQId->queue);
	}
	else
	{
		ret = 0;
	}
    return ret;
}



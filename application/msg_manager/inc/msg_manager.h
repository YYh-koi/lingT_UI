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

#ifndef __TURNKEY_MSG_MANAGER_H__
#define __TURNKEY_MSG_MANAGER_H__

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/
/* turn key message manager */
#define MSG_PRI_NORMAL		0
#define MSG_PRI_URGENT		1

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct
{
	INT32U		maxMsgs;
    INT32U		maxMsgLength;
	xQueueHandle queue;
	osMutexId	lock;
} *MSG_Q_ID;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/**
 * @brief   create and initialize a message queue
 * @param   maxQSize:			max queue can be creat
 * @param   maxMsgs:			max messages that can be queued
 * @param   maxMsgLength:		max bytes in a message
 * @return 	NULL if faile
 */
extern MSG_Q_ID msgQCreate(INT32U maxQSize, INT32U maxMsgs, INT32U maxMsgLength);

/**
 * @brief   delete a message queue
 * @param   msgQId:				message queue to delete
 * @return 	none
 */
extern void msgQDelete (MSG_Q_ID msgQId);

/**
 * @brief   send a message to a message queue
 * @param   msgQId:			message queue on which to send
 * @param   msg_id:			message id
 * @param   para:			message to send
 * @param   nParaByte:		byte number of para buffer
 * @param   priority:		MSG_PRI_NORMAL or MSG_PRI_URGENT
 * @return 	0 SUCCESS/-1 FAIL
 */
extern INT32S msgQSend(MSG_Q_ID msgQId, INT32U msg_id, void *para, INT32U nParaByte, INT32U priority);

/**
 * @brief   receive a message from a message queue
 * @param   msgQId:			message queue on which to receive
 * @param   msg_id:			message id
 * @param   para:			message and type received
 * @param   maxNByte:		message size
 * @return 	0 SUCCESS/-1 FAIL
 */
extern INT32S msgQReceive(MSG_Q_ID msgQId, INT32U *msg_id, void *para, INT32U maxParaNByte);
extern INT32S msgQReceive_timeout(MSG_Q_ID msgQId, INT32U *msg_id, void *para, INT32U nBytes, INT32U timeOut);

/**
 * @brief   Check whether a message is available from a message queue
 * @param   msgQId:			message queue on which to check
 * @param   msg_id:			message id
 * @param   para:			message and type received
 * @param   maxNByte:		message size
 * @return 	0 SUCCESS/-1 FAIL (queue is empty or fail)
 */
extern INT32S msgQAccept(MSG_Q_ID msgQId, INT32U *msg_id, void *para, INT32U maxParaNByte);

/**
 * @brief   flush message queue
 * @param   msgQId:			message queue on which to flush
 * @return 	none
 */
extern void msgQFlush(MSG_Q_ID msgQId);

/**
 * @brief   query message queue
 * @param   msgQId:			message queue on which to query
 * @return 	none
 */
extern INT32S msgQQuery(MSG_Q_ID msgQId, INT32U *msg_id, void *para, INT32U maxParaNByte);

/**
 * @brief   get message queue size
 * @param   msgQId:			size of message queue on which to get
 * @return 	none
 */
extern INT32U msgQSizeGet(MSG_Q_ID msgQId);


extern INT32S _post_message(MSG_Q_ID msg_queue,	INT32U message, MSG_Q_ID ack_mbox, INT32U msc_time, INT32U prio);

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define C_ACK_SUCCESS	0x00000001
#define C_ACK_FAIL		0x80000000

#define RETURN(x) \
	do \
	{ \
		nRet = x; \
		goto Return; \
	} while(0)


#define POST_MESSAGE(msg_queue, message, ack_mbox, msc_time, prio) \
	do \
	{ \
		if(_post_message(msg_queue, message, ack_mbox, msc_time, prio) != STATUS_OK) \
		{ \
			RETURN(STATUS_FAIL); \
		} \
	} while(0)


#endif /* __TURNKEY_MSG_MANAGER_H__ */

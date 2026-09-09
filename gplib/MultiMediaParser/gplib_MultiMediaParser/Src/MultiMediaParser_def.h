#ifndef __QTFFPARSERAPI_H__
#define __QTFFPARSERAPI_H__

#include "MediaParser.h"
#include "MultiMediaParser.h"

#ifndef LINUX_SYSTEM
#ifdef GP_FREERTOS
#include "cmsis_os.h"
#else
#include "application.h"
#endif

#define DEFAULT_AUD_BUF_SIZE		32768
#define DEFAULT_VID_BUF_SIZE		65536
#ifdef GP_FREERTOS
#define NUM_VID_BUF					32
#else
#define NUM_VID_BUF					3
#endif
#define EXDATA_LEN					1024

#else
#include "./../libMcpThread/mcp_thread.h"
#include "./../libMcpThread/mcp_queue.h"

#define DEFAULT_AUD_BUF_SIZE		8192
#define DEFAULT_VID_BUF_SIZE		65536
#define NUM_VID_BUF					6
#define EXDATA_LEN					1024
#endif

// Messages from InterTask function to Task
#define MMP_MSG_REFILL_VID				1
#define MMP_MSG_REFILL_AUD				2
#define MMP_MSG_STOP					3
#define MMP_MSG_SEEK					4
#define MMP_MSG_ENABLE_FOLLOW_MODE		5
#define MMP_MSG_DISABLE_FOLLOW_MODE		6
#define MMP_MSG_ENABLE_REVERSE_PLAY		7
#define MMP_MSG_DISABLE_REVERSE_PLAY	8
#define MMP_MSG_SELECT_TRACK			9

#define MSGQ_SIZE				8
#define MMP_STACK_SIZE	1024

typedef struct
{
	int workmem_size;
	char *kernel;
	const MEDIA_PARSER_FCN_TAB *Parser;
	FILEHANDLE	fid;
	long BaseOffset;
#ifdef SEEK64_EN
	long long FileLength;
#else
	long FileLength;
#endif
	int flag;


	int AudRingRI, AudRingWI;
	int AudRingLen, AudRest;
#ifdef SEEK64_EN
	long long AudFileOffset;
#else
	int AudFileOffset;
#endif
	char *AudRing;

	int NumVidBuf;
	char ADTSHeader[4];
	char ExData[EXDATA_LEN];
	char **VidBuf;
	char **VidData;
	long *VidDur;
	long *VidTime;
	int *VidBufMax;
	int *VidBufLen;
	int *VidIsKey;
	short VidBufRI, VidBufWI;

#ifndef LINUX_SYSTEM
	void *MsgQ[MSGQ_SIZE];
#ifdef GP_FREERTOS
	osMessageQId Msg, Ack;
#else
	OS_EVENT *Msg, *Ack;
#endif
	char AudReqRx, AudReqTx;
	char VidReqRx, VidReqTx;

	long GAP0;
  #ifndef GP_FREERTOS
	OS_STK stack[MMP_STACK_SIZE];
  #endif
	long GAP1;
#else
	mcp_thread_t thread;
	mcp_mbox_t	msg_queue;
	mcp_mbox_t	ack_queue;
	mcp_mbox_t	*Msg, *Ack;

	char AudReqRx, AudReqTx;
	char VidReqRx, VidReqTx;
#endif

	int (*ErrHandler)(int ErrID);

	int TotalSkipFrame;
	int TotalNullFrame;
	int FrameDropLevel;
	int ReadVidCnt;


	long vCnt;
	long aCnt;
	long VidMaxSample;
	// long AudMaxSample;

	MEDIA_PARSER_SAMPLE_INFO aSample;
	MEDIA_PARSER_SAMPLE_INFO vSample;

	__int64 vUserTime;
	long SeekSecQ8;
	long SeekSample;
	int SelectVTrk;
	int SelectATrk;
} PARSERAPI;


#endif // __QTFFPARSERAPI_H__

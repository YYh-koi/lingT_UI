/******************************************************
* rtsp_h264_gp_server.c
*
* Purpose: RTP H264 payload over RTSP server
*
* Author: Eugene Hsu
*
* Date: 2018/07/24
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version :
* History :
*
*******************************************************/
#include "lwip/sockets.h"
#include "lwip/api.h"
#include "application.h"

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)

#if C_WIFI_SEND_FIX_H264I
#include "iframes_dragon.h"
#endif


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/
extern INT32S wifi_transmission_start(void);
extern INT32S wifi_transmission_stop(void);
extern INT32S video_encode_state_get(VIDEO_ENCODE_STATUS_OFFSET stateOffset);
extern int socket_cmd_connection_status(void);

#if C_APP_PLAYBACK_WITH_SOUND
extern INT32U wifi_playback_first_tick_get(int src_id);
extern INT32U wifi_playback_tick_get(INT32S src_id);
#endif

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define RTSP_INTERLEAVED_DEBUG	0

#define TCP_CLIENT_KEEPALIVE_IDLE	    2//3		/* 10000ms */
#define TCP_CLIENT_KEEPALIVE_INTVL	    1//3//1	/* 1000ms */
#define TCP_CLIENT_KEEPALIVE_CNT	    7//10//9	/* 1 time */

//#define RTSP_RTP_H264_PKT_SIZE			1400
#define RTSP_RTP_H264_PKT_SIZE			(TCP_TX_BUF_LEN - 100)
#define RTSP_RTP_JPEG_PKT_SIZE			(TCP_TX_BUF_LEN - 100)

typedef enum
{
	MSG_H264_FRAME_DONE = 0x9000,
	MSG_RTSP_SEND_REPLY,
	MSG_JPEG_FRAME_DONE,
	MSG_AUD_FRAME_DONE
}TASK_H264_SERVER_ENUM;

static TCP_SRV_SOCKET_CTL_T rtsp_tcp_ctl_blk =
{
	-1,					/* TCP server sokcet */
	-1,					/* TCP client sokcet */
    NULL,               /* TCP server tx message Q handler */
	NULL,				/* RX buffer data pointer */
	NULL,				/* TX buffer data pointer */
	H264_SERVER_PORT	/* Server port number */
};

UDP_SRV_SOCKET_CTL_T rtp_server_ctl_blk =
{
	0,					/* Server socket */
//	NULL,				/* TX message Q handler */
//	NULL,				/* RX buffer pointer */
//	NULL,				/* TX buffer pointer */
	0,					/* Receive broadcast packet */
	RTP_SERVER_PORT,	/* Server port */
};

RTP_CTL_BLK_T rtsp_rtp_ctl_blk =
{
	0,		/* Time Stamp in RTP header */
	0,	    /* Width of picture in RTP JPEG header */
	0,	    /* Height of picture in RTP JPEG header */
	0,	    /* Audio streaming sample rate */
	0,	    /* Audio ptime */
	0x9fa33d59,		/* SSRC in RTP header */
	0, 		/* Transfer size for each RTP data transfer */
	0,		/* RTP packet send flag */
	0,		/* Transfer data size */
	0,		/* Remain transfer size */
	0,		/* Sequence number in RTP header */
	{0},	/* Remote client IP address */
	NULL,	/* RTP packet buffer pointer */
	NULL,	/* RTP data buffer */
	RTP_PT_H264,	/* RTP payload type */
};

typedef struct
{
    INT32U type;
    INT16U width;
    INT16U height;
    INT32U stream_addr;
    INT32U stream_size;
    INT32U stream_type;
    INT64U timestamp;
    osPoolId framePool;
	void (*func_done) (INT32U stream_addr, INT32U stream_type);
} MESSAGE_INFO_STRUCT;

static volatile char* tcp_tx_write_ptr = NULL;
//static void *tcp_q_stack[TCP_QUEUE_LEN];
//static INT32U tcp_task_stack[TCP_TASK_STACKSIZE];
static osThreadId rtsp_rx_task  = NULL;
static osThreadId rtsp_tx_task  = NULL;
static xSemaphoreHandle rtsp_h264_sema = NULL;
static RTSP_INSTANCE_CTL_BLK_T rtsp_instance = {{0}};
static RTP_HDR_T rtphdr;
#if C_APP_PLAYBACK_WITH_SOUND
static RTP_HDR_T rtpaud_hdr;
#endif
#if 0
//timestamp convert
static unsigned int g_sum_ts = 0;
static unsigned int g_previous_ts = 0;
#endif

static int g_keep_idle = TCP_CLIENT_KEEPALIVE_IDLE;
static int g_keep_intvl = TCP_CLIENT_KEEPALIVE_INTVL;
static int g_keep_cnt = TCP_CLIENT_KEEPALIVE_CNT;

INT32U g_rtsp_cmd_wait = 100;

extern INT16U browse_play_idx;
extern INT32U socket_cmd_status;
extern int force_close;
RTSP_METHOD_E rtsp_st;

INT8U gConnect_status = 0;


#if C_WIFI_SEND_FIX_H264I
#define RTSP_H264_FIX_SEND_FRAME_COUNT     	(9)		/* send the maximun number */
#define RTSP_H264_FIX_SEND_SWITCH_TICKS    	(4)		/* (16)->1S,(8)->0.5S,(4)->0.25S */
#define RTSP_H264_FIX_SEND_TS_STEP_MS      	(63)	

INT32U rtsp_h264_fix_send_index=0;
INT32U rtsp_h264_fix_send_ts=0;
INT32U rtsp_h264_fix_send_tick_div=0;
INT32U h264_fix_send_change_cnt=0;		/* for change different H254 I frame */
INT8U rtsp_h264_fix_send_en=0;

static INT32U rtsp_h264_fix_send_get_size(INT32U index)
{
	switch(index)
	{
		case 0:/*  1 */
			return (INT32U)dragon_dib1_len;
	
		case 1:/*  2 */
			return (INT32U)dragon_dib2_len;
			
		case 2:/*  3 */
			return (INT32U)dragon_dib3_len;
			
		case 3:/*  4 */
			return (INT32U)dragon_dib4_len;
			
		case 4:/*  5 */
			return (INT32U)dragon_dib5_len;
			
		case 5:/*  6 */
			return (INT32U)dragon_dib6_len;
			
		case 6:/*  7 */
			return (INT32U)dragon_dib7_len;
			
		case 7:/*  8 */
			return (INT32U)dragon_dib8_len;
			
		case 8:/* 9 */
			return (INT32U)dragon_dib9_len;
		default:
			
			return 0;
			
	}
}

/* initial buffer for fix send I frame */
void rtsp_h264_fix_send_init(void)
{
	DBG_PRINT("==rtsp_h264_fix_send_init=\r\n");
	rtsp_h264_fix_send_en=0;
	rtsp_h264_fix_send_index=0;
	rtsp_h264_fix_send_ts=0;
	rtsp_h264_fix_send_tick_div=0;
	h264_fix_send_change_cnt=0;
}

/* start send fix h264 I frame */
void rtsp_h264_fix_send_start(void)
{
	DBG_PRINT("==rtsp_h264_fix_send_start=\r\n");
	rtsp_h264_fix_send_init();
	rtsp_h264_fix_send_en=1;
}

/* stop send fix h264 I frame */
void rtsp_h264_fix_send_stop(void)
{
	DBG_PRINT("==rtsp_h264_fix_send_stop==\r\n");
	rtsp_h264_fix_send_en=0;
	rtsp_h264_fix_send_index=0;
	rtsp_h264_fix_send_ts=0;
	rtsp_h264_fix_send_tick_div=0;
	h264_fix_send_change_cnt=0;
}

/* Send I frame to rtsp_h264_tx_task() */
void rtsp_h264_fix_send(void)
{
		const INT8U *buf;
		INT32U size;
	
		if(rtsp_h264_fix_send_en)		/* check the send flag is enable */
		{
			if(rtsp_tcp_ctl_blk.tcpsrv_tx_q == NULL)
			{
				return;
			}
	
			rtsp_h264_fix_send_tick_div++;
			if(rtsp_h264_fix_send_tick_div < 2)
			{
				return;
			}

			
			rtsp_h264_fix_send_tick_div = 0;


			//rtsp_h264_fix_send_index=0;
			buf=rtsp_h264_fix_send_buf_tbl[rtsp_h264_fix_send_index];		/* point to I frame array */
			size=rtsp_h264_fix_send_get_size(rtsp_h264_fix_send_index);	/* get the current I frame size */
			if((buf ==NULL) ||(size ==0))
			{
				DBG_PRINT("rtsp_h264_fix_send() I frame resource fail\r\n");
				return;
			}
	
			/* send I frame to rtsp task without callback */
			if(rtsp_h264_send((INT32U)buf,size,rtsp_h264_fix_send_ts,H264_FRAME_TYPE_I,NULL) != STATUS_OK)
			{
				DBG_PRINT("Send rtsp_h264_fix_send() fail\r\n");
				return;
			}
	
			rtsp_h264_fix_send_ts+=RTSP_H264_FIX_SEND_TS_STEP_MS;	/* increase ts time */ 
			h264_fix_send_change_cnt++;
			
			if(h264_fix_send_change_cnt >=RTSP_H264_FIX_SEND_SWITCH_TICKS)
			{
				h264_fix_send_change_cnt=0;
				rtsp_h264_fix_send_index++; 	/* rtsp_h264_fix_send_get_size() get the specific I frame szie */
				
				if(rtsp_h264_fix_send_index>=RTSP_H264_FIX_SEND_FRAME_COUNT)
				{
					rtsp_h264_fix_send_index=0;
				}
			}
	
		}
	
	
}
#endif


INT32S rtsp_h264_send(INT32U buf, INT32U size, INT32U ts, INT32U type, void (*callback_done) (INT32U stream_addr, INT32U stream_type))
{
    //INT32S ret = STATUS_OK;
	//INT32U msg;
    //osStatus status;

    MESSAGE_INFO_STRUCT msg_info;
    msg_info.type        = MSG_H264_FRAME_DONE;
    msg_info.stream_addr = buf;
    msg_info.stream_size = size;
	msg_info.stream_type = type;
    msg_info.timestamp   = ts;
    msg_info.framePool   = 0;
	msg_info.func_done   = callback_done;

	#if 0
	DBG_PRINT("%s(0x%X,%d,%d)\r\n", __FUNCTION__, buf, size, ts);
	#endif

    if( 0 != msgQSend(rtsp_tcp_ctl_blk.tcpsrv_tx_q, MSG_H264_FRAME_DONE, (void *)&msg_info, sizeof(MESSAGE_INFO_STRUCT), MSG_PRI_NORMAL))
    {
        //print_string("[WAR] H264enc_Callback msgQ full\r\n");
        DBG_PRINT("F");

		if(callback_done)
		{
			callback_done(buf,type);
		}

		return STATUS_FAIL;
	}
	else
	{
		#if 1
		DBG_PRINT("*");
		#endif

		return STATUS_OK;
	}
}

INT32S rtsp_jpeg_send(INT32U buf, INT32U size, INT32U ts, INT16U width, INT16U height, void (*callback_done) (INT32U, INT32U))
{
    //INT32S ret = STATUS_OK;
	//INT32U msg;
    //osStatus status;

    MESSAGE_INFO_STRUCT msg_info;
    msg_info.type        = RTP_PT_JPEG;
	msg_info.width       = width;
	msg_info.height      = height;
    msg_info.stream_addr = buf;
    msg_info.stream_size = size;
    msg_info.timestamp   = ts;
    msg_info.framePool   = 0;
	msg_info.func_done   = callback_done;

	// DBG_PRINT("%s(0x%X,%d,%d,%d,%d)\r\n", __FUNCTION__, buf, size, ts, width, height);

    if( 0 != msgQSend(rtsp_tcp_ctl_blk.tcpsrv_tx_q, MSG_JPEG_FRAME_DONE, (void *)&msg_info, sizeof(MESSAGE_INFO_STRUCT), MSG_PRI_NORMAL))
    {
        DBG_PRINT("F");

		if(callback_done)
		{
			callback_done(msg_info.stream_addr, msg_info.stream_type);
		}

		return STATUS_FAIL;
	}
	else
	{
		#if 0 // Debug
		print_string(",");
		#endif

		return STATUS_OK;
	}
}

static void rtsp_tcp_h264_lock(void)
{
	//INT8U err;

	/*err = */xSemaphoreTake(rtsp_h264_sema, portMAX_DELAY);
}

static void rtsp_tcp_h264_unlock(void)
{
	xSemaphoreGive(rtsp_h264_sema);
}

static int g_rtsp_method_type = 0;
void rtsp_method_type_set(int val)
{
	g_rtsp_method_type = val;
}

int rtsp_method_type_get(void)
{
	return g_rtsp_method_type;
}

static void rtsp_method_reply(INT32U method, INT32U code, char* reason)
{
	#if C_APP_PLAYBACK_WITH_SOUND
	static int state;
	#endif
	int len;//, cnt;
	char* temp_buf = NULL;
	char* tx_buf = NULL;
	//INT32U msg;
	MESSAGE_INFO_STRUCT msg_info;

	tx_buf = (char*)gp_malloc(RTSP_TX_BUF_LEN);
	if(tx_buf == NULL)
	{
		DBG_PRINT("RTSP tx_buf failed\r\n");
		while(1);
	}
	memset(tx_buf, 0, RTSP_TX_BUF_LEN);
	switch(method)
	{
		case RTSP_DESCRIBE_METHOD:
			temp_buf = (char*)gp_malloc(1024);
			if(temp_buf == NULL)
			{
				DBG_PRINT("RTSP alloc failed\r\n");
				while(1);
			}
			memset(temp_buf, 0, 1024);
			#if C_APP_PLAYBACK_WITH_SOUND
            state = 0;
            #endif
			switch (g_rtsp_method_type)
			{
				case 1:		// JPEG FLOW
            #if C_APP_PLAYBACK_WITH_SOUND
					sprintf(temp_buf, "v=0\r\no=- %d 0 IN IP4 %d.%d.%d.%d\r\n" RTSP_SDP_JPEG_REPLY1 "c=IN IP4 %d.%d.%d.%d\r\n" RTSP_SDP_JPEG_REPLY2 RTSP_SDP_JPEG_REPLY3 RTSP_SDP_VID_AUD_REPLY1 RTSP_SDP_VID_AUD_REPLY2 RTSP_SDP_VID_AUD_REPLY3,
						rtsp_instance.session_id,
						rtsp_instance.local_ip[0] & 0xff, rtsp_instance.local_ip[1] & 0xff, rtsp_instance.local_ip[2] & 0xff, rtsp_instance.local_ip[3] & 0xff,
						rtsp_instance.connect_ip[0] & 0xff, rtsp_instance.connect_ip[1] & 0xff, rtsp_instance.connect_ip[2] & 0xff, rtsp_instance.connect_ip[3] & 0xff);
            #else
					sprintf(temp_buf, "v=0\r\no=- %d 0 IN IP4 %d.%d.%d.%d\r\n" RTSP_SDP_JPEG_REPLY1 "c=IN IP4 %d.%d.%d.%d\r\n" RTSP_SDP_JPEG_REPLY2 RTSP_SDP_JPEG_REPLY3,
						rtsp_instance.session_id,
						rtsp_instance.local_ip[0] & 0xff, rtsp_instance.local_ip[1] & 0xff, rtsp_instance.local_ip[2] & 0xff, rtsp_instance.local_ip[3] & 0xff,
						rtsp_instance.connect_ip[0] & 0xff, rtsp_instance.connect_ip[1] & 0xff, rtsp_instance.connect_ip[2] & 0xff, rtsp_instance.connect_ip[3] & 0xff);
            #endif
					break;
				case 0:
				default:	// H264 FLOW
					sprintf(temp_buf, "v=0\r\no=- %d 0 IN IP4 %d.%d.%d.%d\r\n" RTSP_SDP_H264_REPLY1 "c=IN IP4 %d.%d.%d.%d\r\n" RTSP_SDP_H264_REPLY2 RTSP_SDP_H264_REPLY3,
						rtsp_instance.session_id,
						rtsp_instance.local_ip[0] & 0xff, rtsp_instance.local_ip[1] & 0xff, rtsp_instance.local_ip[2] & 0xff, rtsp_instance.local_ip[3] & 0xff,
						rtsp_instance.connect_ip[0] & 0xff, rtsp_instance.connect_ip[1] & 0xff, rtsp_instance.connect_ip[2] & 0xff, rtsp_instance.connect_ip[3] & 0xff);
			}

			len = strlen(temp_buf);
			strcat(tx_buf, temp_buf);
			sprintf(tx_buf, "RTSP/1.0 %d %s\r\nCseq: " "%d\r\n" RTSP_DESCRIBE_REPLY2 "%d.%d.%d.%d:%d/%s\r\nContent-length: %d\r\n\r\n", code, reason, rtsp_instance.cseqnum,
					rtsp_instance.local_ip[0] & 0xff, rtsp_instance.local_ip[1] & 0xff, rtsp_instance.local_ip[2] & 0xff, rtsp_instance.local_ip[3] & 0xff, rtsp_tcp_ctl_blk.server_port, RTSP_FILE_PATCH, len);
			strcat(tx_buf, temp_buf);
			gp_free(temp_buf);
			break;

		case RTSP_OPTION_METHOD:
			sprintf(tx_buf, "RTSP/1.0 %d %s\r\nCseq: " "%d\r\n" RTSP_OPTION_REPLY2 LINE_END, code, reason, rtsp_instance.cseqnum);
			break;

		case RTSP_SETUP_METHOD:
			#if C_APP_PLAYBACK_WITH_SOUND
            state = 0;
            #endif
			if(code == 200)
			{
				#if C_APP_PLAYBACK_WITH_SOUND
				if (rtsp_instance.setup_id==0)
				{
				#endif
				switch (g_rtsp_method_type)
				{
					case 1: //JPEG UDP flow
					sprintf(tx_buf, "RTSP/1.0 %d %s\r\nCseq: " "%d\r\n" "Session: %d\r\n" RTSP_SETUP_UDP_REPLY2 "client_port=%d-%d;server_port=%d-%d\r\n\r\n",
						code, reason,  rtsp_instance.cseqnum, rtsp_instance.session_id,rtsp_instance.client_rtp_port1, rtsp_instance.client_rtp_port2,
						rtsp_instance.server_rtp_port1, rtsp_instance.server_rtp_port2);
					break;
					case 0:
					default:	// H264 TCP flow
					sprintf(tx_buf, "RTSP/1.0 %d %s\r\nCseq: " "%d\r\n" "Session: %d\r\n" RTSP_SETUP_TCP_REPLY2 "destination=%d.%d.%d.%d;source=%d.%d.%d:%d;interleaved=0-1\r\n\r\n",
						code, reason,  rtsp_instance.cseqnum, rtsp_instance.session_id,
						rtsp_instance.connect_ip[0] & 0xff, rtsp_instance.connect_ip[1] & 0xff, rtsp_instance.connect_ip[2] & 0xff, rtsp_instance.connect_ip[3] & 0xff,
						rtsp_instance.local_ip[0] & 0xff, rtsp_instance.local_ip[1] & 0xff, rtsp_instance.local_ip[2] & 0xff, rtsp_instance.local_ip[3] & 0xff);
				}
                #if C_APP_PLAYBACK_WITH_SOUND
				}
				else
				{
					sprintf(tx_buf, "RTSP/1,0 %d %s\r\nCseq: " "%d\r\n" "Session: %d\r\n" RTSP_SETUP_REPLY2 "client_port=%d-%d;server_port=%d-%d\r\n\r\n",
							code, reason,  rtsp_instance.cseqnum, rtsp_instance.session_id, rtsp_instance.client_rtp_aud_port1, rtsp_instance.client_rtp_aud_port2,
							rtsp_instance.server_rtp_aud_port1, rtsp_instance.server_rtp_aud_port2);
				}
				#endif
			}
			else
			{
				#if C_APP_PLAYBACK_WITH_SOUND
				if (rtsp_instance.setup_id==0)
				{
                #endif
				sprintf(tx_buf, "RTSP/1.0 %d %s\r\nCseq: " "%d\r\n" "Session: %d\r\n\r\n", code, reason, rtsp_instance.cseqnum, rtsp_instance.session_id);
				#if C_APP_PLAYBACK_WITH_SOUND
				}
				else
				{
					sprintf(tx_buf, "RTSP/1,0 %d %s\r\nCseq: " "%d\r\n" "Session: %d\r\n" RTSP_SETUP_REPLY2 "client_port=%d-%d;server_port=%d-%d\r\n\r\n",
							code, reason,  rtsp_instance.cseqnum, rtsp_instance.session_id, rtsp_instance.client_rtp_aud_port1, rtsp_instance.client_rtp_aud_port2,
							rtsp_instance.server_rtp_aud_port1, rtsp_instance.server_rtp_aud_port2);
				}
				#endif
			}
			break;

		case RTSP_PLAY_METHOD:
            #if C_APP_PLAYBACK_WITH_SOUND
			DBG_PRINT("state=%d\r\n",state);
			if (state==0)	// first time
			{
			#endif
			sprintf(tx_buf, "RTSP/1.0 %d %s\r\nCseq: " "%d\r\n" "Session: %d\r\n\r\n", code, reason, rtsp_instance.cseqnum, rtsp_instance.session_id);
			#if C_APP_PLAYBACK_WITH_SOUND
			}
			else
			{
				sprintf(tx_buf, "RTSP/1,0 %d %s\r\nCseq: " "%d\r\n" "Session: %d\r\n" "RTP-Info: url=rtsp://" "%d.%d.%d.%d:%d/?action=stream/streamid=0;" "rtptime=%d, " "url=rtsp://" "%d.%d.%d.%d:%d/?action=stream/streamid=1;" "rtptime=%d\r\n\r\n",
					code, reason, rtsp_instance.cseqnum, rtsp_instance.session_id,
					rtsp_instance.local_ip[0] & 0xff, rtsp_instance.local_ip[1] & 0xff, rtsp_instance.local_ip[2] & 0xff, rtsp_instance.local_ip[3] & 0xff, rtsp_tcp_ctl_blk.server_port, wifi_playback_first_tick_get(0),
					rtsp_instance.local_ip[0] & 0xff, rtsp_instance.local_ip[1] & 0xff, rtsp_instance.local_ip[2] & 0xff, rtsp_instance.local_ip[3] & 0xff, rtsp_tcp_ctl_blk.server_port, wifi_playback_first_tick_get(1));
				//DBG_PRINT("%s\r\n",tx_buf);
			}
			state++;
			#endif
	  		break;

		case RTSP_TEARDOWN_METHOD:
			sprintf(tx_buf, "RTSP/1.0 %d %s\r\nCseq: " "%d\r\n" "Session: %d\r\n\r\n", code, reason, rtsp_instance.cseqnum, rtsp_instance.session_id);
			break;

		case RTSP_PAUSE_METHOD:
		  	sprintf(tx_buf, "RTSP/1.0 %d %s\r\nCseq: " "%d\r\n" "Session: %d\r\n\r\n", code, reason, rtsp_instance.cseqnum, rtsp_instance.session_id);
		  	#if C_APP_PLAYBACK_WITH_SOUND
			state++;
			#endif
	  		break;

		default:
			sprintf(tx_buf, "RTSP/1.0 %d %s\r\nCseq: " "%d\r\n" "Session: %d\r\n\r\n", code, reason, rtsp_instance.cseqnum, rtsp_instance.session_id);
			break;
	}

	len = strlen(tx_buf);
	/* Send to TX task to reply */
    msg_info.type        = MSG_RTSP_SEND_REPLY;
    msg_info.stream_addr = (INT32U)tx_buf;
    msg_info.stream_size = len;
    msg_info.timestamp   = 0;
    msg_info.framePool   = 0;

    if( 0 != msgQSend(rtsp_tcp_ctl_blk.tcpsrv_tx_q, MSG_RTSP_SEND_REPLY, (void *)&msg_info, sizeof(MESSAGE_INFO_STRUCT), MSG_PRI_NORMAL))
    {
        DBG_PRINT("Send RTSP TXQ failed\r\n");
	}
}

static int rtsp_find_url_in_method(char*input, char*desbuf, INT32U maxlen)
{
  	char* start = NULL;
	char* end = NULL;
	INT32U len;

	/* Check URL => rtsp://192.168.0.1:554/test.sdp RTSP/1.0\r\n*/
	start = strstr(input, RTSP_METHOD_URL);

	if(start != NULL)
	{
		end = strstr(start, RTSP_SPACE);

		if(end)
		{
			memset(desbuf, 0, maxlen);
			len = end - start;
			if(len > maxlen)
				len = maxlen;

			strncpy(desbuf, start, len);
			return 0;
		}
	}
	return -1;
}

static void rtsp_find_string_by_header(char* input, char* desbuf, char* header, INT32U maxlen)
{
	char* start = NULL;
	char* end = NULL;
	INT32U len;

	start = strstr(input, header);
	if(start != NULL)
	{
		end = strstr(start, LINE_END);

		if(end)
		{
			start += strlen(header);
			len = end - start;
			if(len > maxlen)
				len = maxlen;
			strncpy(desbuf, start, len);
		}
	}
	else
	{
		/* The specified header not found */
	}
}

static void _handle_option_method(void)
{
	char cseq[RTSP_CSEQ_LEN] = {0};
	//INT32U len = 0;

	/* Check command sequence => CSeq: 2\r\n */
	rtsp_find_string_by_header(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, cseq, RTSP_CSEQ_HEAD, RTSP_CSEQ_LEN);
	rtsp_instance.cseqnum = atoi(cseq);
	DBG_PRINT("OPTION CSeq=%d\r\n", rtsp_instance.cseqnum);

	rtsp_method_reply(RTSP_OPTION_METHOD, 200, "OK");
}

static void _handle_describe_method(void)
{
	char cseq[RTSP_CSEQ_LEN] = {0};
	//INT32U len = 0;

	rtsp_find_url_in_method(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, rtsp_instance.rtsp_url, RTSP_URL_LEN);

	/* Check command sequence => CSeq: 2\r\n */
	rtsp_find_string_by_header(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, cseq, RTSP_CSEQ_HEAD, RTSP_CSEQ_LEN);
	rtsp_instance.cseqnum = atoi(cseq);
	DBG_PRINT("DESCRIBE CSeq=%d\r\n", rtsp_instance.cseqnum);

	rtsp_method_reply(RTSP_DESCRIBE_METHOD, 200, "OK");
	#if C_APP_PLAYBACK_WITH_SOUND
	rtsp_instance.setup_id = 0;
	#endif
}

int rtsp_atoi(const char *str)
{
		const char *s = str;
		int acc;
		char c;

		c = *s++;
		acc = 0;
    while(c >= '0' && c <= '9')
    {
        c -= '0';

        acc *= 10;
        acc += c;

        c = *s++;
    }

    return acc;
}

static void _handle_setup_method(void)
{
	char* start = NULL;
	char* middle = NULL;
	char* end = NULL;
	char cseq[RTSP_CSEQ_LEN] = {0};
	char tranport[64] = {0};
	char temp[12] = {0};
	INT32U len = 0;
	//int port1, port2;

	rtsp_find_string_by_header(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, cseq, RTSP_CSEQ_HEAD, RTSP_CSEQ_LEN);
	rtsp_instance.cseqnum = atoi(cseq);
	DBG_PRINT("SETUP CSeq=%d\r\n", rtsp_instance.cseqnum);

	switch (g_rtsp_method_type)
	{
		case 1:
		/* Find client port in transport header for UDP */
		rtsp_find_string_by_header(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, tranport, RTSP_TRANSPORT_HEAD, 64);
		start = strstr(tranport, RTSP_CLIENT_PORT_FIELD);

		if(start)
		{
			start += strlen(RTSP_CLIENT_PORT_FIELD);
			/* search '-' */
			middle = strchr(start, '-');
			/* Find end */
			end = tranport + strlen(tranport);

			if(end && middle)
			{
				len = middle - start;
				if(len > 12)
					len = 12;
				strncpy(temp, start, len);
				#if C_APP_PLAYBACK_WITH_SOUND
				if (rtsp_instance.setup_id==0)
				{	// video
				#endif
				rtsp_instance.client_rtp_port1 = atoi(temp);
				#if C_APP_PLAYBACK_WITH_SOUND
				}
				else
				{
					rtsp_instance.client_rtp_aud_port1 = rtsp_atoi(temp);
				}
				#endif
				DBG_PRINT("Port1 %d\r\n", rtsp_instance.client_rtp_port1);

				len = end - middle;
				if(len > 12)
					len = 12;
				strncpy(temp, (middle+1), len);
				#if C_APP_PLAYBACK_WITH_SOUND
				if (rtsp_instance.setup_id==0)
				{	// video
				#endif
				rtsp_instance.client_rtp_port2 = atoi(temp);
                #if C_APP_PLAYBACK_WITH_SOUND
				}
				else
				{
					rtsp_instance.client_rtp_aud_port2 = rtsp_atoi(temp);
				}
				#endif
				DBG_PRINT("Port2 %d\r\n", rtsp_instance.client_rtp_port2);
			}
			else
			{
				DBG_PRINT("Client port not found\r\n");
			}
		}
		rtsp_method_reply(RTSP_SETUP_METHOD, 200, "OK");
		#if C_APP_PLAYBACK_WITH_SOUND
		rtsp_instance.setup_id++;
		#endif
		break;

		case 0:
		default:	//for payload via TCP
		/* Find Transport is TCP */
		rtsp_find_string_by_header(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, tranport, RTSP_TRANSPORT_HEAD, 128);

		start = strstr(tranport, RTSP_TRANSPORT_TCP);
		if(start)
		{
			//DBG_PRINT("Setup TCP found\r\n");
			/* Find TCP in tranport header */
			rtsp_method_reply(RTSP_SETUP_METHOD, 200, "OK");
		}
		else
		{
			//DBG_PRINT("Setup TCP not found\r\n");
			/* TCP string not found in tranport header */
			rtsp_method_reply(RTSP_SETUP_METHOD, 461, "Unsupported Transport");
		}
	}
}

static void _handle_play_method(void)
{
	char cseq[RTSP_CSEQ_LEN] = {0};

	rtsp_find_string_by_header(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, cseq, RTSP_CSEQ_HEAD, RTSP_CSEQ_LEN);
	rtsp_instance.cseqnum = atoi(cseq);
	DBG_PRINT("PLAY CSeq=%d\r\n", rtsp_instance.cseqnum);

	rtsp_method_reply(RTSP_PLAY_METHOD, 200, "OK");

    gConnect_status = 1;///var = 1;

	/* Start to send RTP H264 payload */
	if(present_state == STATE_VIDEO_RECORD || present_state == STATE_PHOTO_CAPTURE)
	{
		wifi_transmission_start();
    }
	else
	{
		#if 0 // Socket cmd has sent MSG_APQ_BROWSE_PLAY_FILE
		app_msg_send(MSG_APQ_BROWSE_PLAY_FILE, &browse_play_idx, sizeof(browse_play_idx), MSG_PRI_NORMAL);
		#endif
	}
}

static void _handle_pause_method(void)
{
	char cseq[RTSP_CSEQ_LEN] = {0};

	rtsp_find_string_by_header(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, cseq, RTSP_CSEQ_HEAD, RTSP_CSEQ_LEN);
	rtsp_instance.cseqnum = atoi(cseq);
	DBG_PRINT("PAUSE CSeq=%d\r\n", rtsp_instance.cseqnum);

	rtsp_method_reply(RTSP_PAUSE_METHOD, 200, "OK");

	/* Stop H264 encode */
	wifi_transmission_stop();

}

static void _handle_teardown_method(void)
{
	char cseq[RTSP_CSEQ_LEN] = {0};

	rtsp_find_string_by_header(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, cseq, RTSP_CSEQ_HEAD, RTSP_CSEQ_LEN);
	rtsp_instance.cseqnum = atoi(cseq);
	DBG_PRINT("TEARDOWN CSeq=%d\r\n", rtsp_instance.cseqnum);

	rtsp_method_reply(RTSP_TEARDOWN_METHOD, 200, "OK");
}

static void _handle_unknown(void)
{
	char cseq[RTSP_CSEQ_LEN] = {0};

	rtsp_find_string_by_header(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, cseq, RTSP_CSEQ_HEAD, RTSP_CSEQ_LEN);
	rtsp_instance.cseqnum = atoi(cseq);
	DBG_PRINT("Handle unknown CSeq=%d\r\n", rtsp_instance.cseqnum);

	rtsp_method_reply(RTSP_ERROR_METHOD, 501, "Not Implemented");
}

static int rtsp_process_method(INT32U len)
{
	//DBG_PRINT("Enter rtsp_process_method\r\n");
  	if(!strncmp(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, DESCRIBE_METHOD, strlen(DESCRIBE_METHOD)))
  	{
  		_handle_describe_method();
	}
	else if(!strncmp(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, OPTION_METHOD, strlen(OPTION_METHOD)))
	{
		_handle_option_method();
	}
	else if(!strncmp(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, SETUP_METHOD, strlen(SETUP_METHOD)))
	{
		_handle_setup_method();
	}
	else if(!strncmp(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, PLAY_METHOD, strlen(PLAY_METHOD)))
	{
		_handle_play_method();
	}
	else if(!strncmp(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, PAUSE_METHOD, strlen(PAUSE_METHOD)))
	{
		_handle_pause_method();
	}
	else if(!strncmp(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, TEARDOWN_METHOD, strlen(TEARDOWN_METHOD)))
	{
		_handle_teardown_method();
	}
 	else
	{
		/* Find interleaved frame */
		char* start = NULL;
		//char* data;
		static INT32U inter_data_cnt;
		//INT32U data_cnt, i;
		RTSP_INTERLEAVED_FRAME_T* interleaved;

		start = strstr(rtsp_tcp_ctl_blk.tcpsrv_rx_buf, RTSP_INTERLEAVED_TOKEN);

		if(start)
		{
			interleaved = (RTSP_INTERLEAVED_FRAME_T*)start;
			inter_data_cnt = interleaved->length[1] | (interleaved->length[0] << 8);
#if RTSP_INTERLEAVED_DEBUG
			data_cnt = len - sizeof(RTSP_INTERLEAVED_FRAME_T);
			DBG_PRINT("Interleaved frame found, channel %d, total length %d, data_cnt %d\r\n", interleaved->channel, inter_data_cnt, data_cnt);
			if(data_cnt)
			{
				data = (char*)(start+sizeof(RTSP_INTERLEAVED_FRAME_T));
				for(i=0; i<data_cnt; i++)
				{
					DBG_PRINT("0x%x ", *(char*)(data+i));
				}
				DBG_PRINT("\r\n");
			}
#endif
			inter_data_cnt -= len;
		}
		else if(inter_data_cnt)
		{
#if RTSP_INTERLEAVED_DEBUG
			data = (char*)rtsp_tcp_ctl_blk.tcpsrv_rx_buf;
			DBG_PRINT("Interleaved frame found, remain length %d, rxlen %d\r\n", inter_data_cnt, len);
			for(i=0; i<len; i++)
			{
				DBG_PRINT("0x%x ", *(char*)(data+i));
			}
			DBG_PRINT("\r\n");
#endif
			inter_data_cnt -= len;
		}
		else
		{
			/* Unknown method */
			_handle_unknown();
		}
	}

	return 0;
}

static INT32U _find_next_nal_start_code(int buf, int len)
{
	INT32U i;
	char* cur_buf = (char*)buf;
	char* temp_buf;
	INT32U nextheader = 0;

	for(i=0; i<len; i++)
	{
		temp_buf = cur_buf + i;
		nextheader = (temp_buf[0] << 24)| (temp_buf[1] << 16) | (temp_buf[2] << 8) | temp_buf[3];
		if(nextheader == 0x00000001)
		{
			return (INT32U)temp_buf;
		}
	}

	/* Start code not found */
	return 0;
}

static void _find_h264_nal_info(NAL_INFO_T* nal_info, int buf_addr, int stream_len)
{
	char *start_pos = NULL;
	char *next_start_pos = NULL;
	int len = 0;

	/* Clear nal info */
	memset(nal_info, 0, sizeof(NAL_INFO_T));
	start_pos = (char*)_find_next_nal_start_code(buf_addr, stream_len);

	if(start_pos)
	{
		/* Find next start code position to calculate NAL length */
		next_start_pos = (char*)_find_next_nal_start_code((int)(start_pos + 4), stream_len - (int)(start_pos + 4 - buf_addr));
		if(next_start_pos)
		{
			len = next_start_pos - start_pos - 4;
			nal_info->islast = 0;
		}
		else
		{
			len = stream_len - 4;
			nal_info->islast = 1;
		}

		nal_info->start_addr = (INT32U)start_pos;	/* include 0x00000001 */
		nal_info->length = len;
	}
}

int rtsp_rtp_send_h264_nal(int addr, int len, INT32U ts, INT32U islast)
{
	char *pkt_ptr = (char*)rtsp_tcp_ctl_blk.tcpsrv_tx_buf;
	char *data_ptr = (char*)addr;
	int remain_len = len;
	int total_len, tx_len, ret;
	char nal_byte;

#if 0
	INT32U tick_start = xTaskGetTickCount();
	INT32U tick_end;
#endif
	/* Check maximum RTP H264 packet size for RTP H264 paylaod */
	tx_len = (RTSP_RTP_H264_PKT_SIZE - sizeof(RTSP_INTERLEAVED_FRAME_T)) - sizeof(RTP_HDR_T);

	if(len <= tx_len)
	{
		/* Send NAL via 1 packet */
		/* RTSP interleaved header */
		total_len = sizeof(rtphdr) + remain_len;
		pkt_ptr[0] = '$';
		pkt_ptr[1] = 0x00;							/* This is a data frame */
		pkt_ptr[2] = (total_len & 0xFF00) >> 8;		/* MSB of length of RTP + H264 payload */
		pkt_ptr[3] = total_len & 0xFF;				/* LSB of length of RTP + H264 payload */

		pkt_ptr += 4;

		/* Initialize RTP header */
		rtsp_rtp_ctl_blk.rtp_ts = ts;

	    rtphdr.version = 2;
	    rtphdr.p  = 0;
	    rtphdr.x  = 0;
	    rtphdr.cc = 0;
	    if(islast)
            rtphdr.m  = 1;
        else
            rtphdr.m  = 0;
	    rtphdr.pt = RTP_PT_H264;
	    rtphdr.seq = ((rtsp_rtp_ctl_blk.rtp_sequence & 0xff) << 8) | ((rtsp_rtp_ctl_blk.rtp_sequence & 0xff00) >> 8);
	    rtphdr.ts = ((rtsp_rtp_ctl_blk.rtp_ts & 0xff) << 24) | ((rtsp_rtp_ctl_blk.rtp_ts & 0xff00) << 8) | ((rtsp_rtp_ctl_blk.rtp_ts & 0xff0000) >> 8) | ((rtsp_rtp_ctl_blk.rtp_ts & 0xff000000) >> 24);
	    rtphdr.ssrc = rtsp_rtp_ctl_blk.rtp_ssrc;

	    memcpy(pkt_ptr, &rtphdr, sizeof(rtphdr));

	    pkt_ptr += sizeof(rtphdr);

	    /* Fill NAL data */
		#if  0
	    memcpy(pkt_ptr, (void*)addr, len);
		#else
		if(dma_data_memcpy((INT8U *)pkt_ptr, (const INT8U *)addr, len))
		{
			gp_memcpy((INT8S *)pkt_ptr, (INT8S *)addr, len);
		}
		#endif

		#if 0
		tick_start = xTaskGetTickCount();
		#endif
	    /* Send packet */
		ret = send(rtsp_tcp_ctl_blk.tcp_client, rtsp_tcp_ctl_blk.tcpsrv_tx_buf, total_len + 4, 0);

		#if 0
		tick_end = xTaskGetTickCount();
		DBG_PRINT("\r\nsend1(%d)"
			, tick_end - tick_start
		);
		#endif

		if(ret < 0)
        {
            DBG_PRINT("Send H264 payload error, ret = %d\r\n", ret);
        }
        rtsp_rtp_ctl_blk.rtp_sequence++;

	}
	else
	{
		/* Fragment this NAL via type 28 FU indicator(1 byte) + FU header(1 byte) */
		nal_byte = *(char*)addr;
		remain_len -= 1;	/* NALU byte */
		data_ptr = (char*)(addr + 1);

		/* FU indicator + FU header = 2 bytes */
		tx_len -= 2;

		/* Packetize NAL into RTSP interleaved frame + RTP header + RTP H264 payload, size = RTSP_RTP_H264_PKT_SIZE bytes */
		while(remain_len)
		{
			pkt_ptr = (char*)rtsp_tcp_ctl_blk.tcpsrv_tx_buf;

			if(remain_len > tx_len)
			{
				/* Interleave frame */
				total_len = RTSP_RTP_H264_PKT_SIZE - sizeof(RTSP_INTERLEAVED_FRAME_T);  /* total len = RTP header + H264 payload */
				pkt_ptr[0] = '$';
				pkt_ptr[1] = 0x00;							/* This is a data frame */
				pkt_ptr[2] = (total_len & 0xFF00) >> 8;		/* MSB of length of RTP + H264 payload */
				pkt_ptr[3] = total_len & 0xFF;				/* LSB of length of RTP + H264 payload */

	            pkt_ptr += 4;

	            /* Initialize RTP header */
				rtsp_rtp_ctl_blk.rtp_ts = ts;

			    rtphdr.version = 2;
			    rtphdr.p  = 0;
			    rtphdr.x  = 0;
			    rtphdr.cc = 0;
	            rtphdr.m  = 0;
			    rtphdr.pt = RTP_PT_H264;
			    rtphdr.seq = ((rtsp_rtp_ctl_blk.rtp_sequence & 0xff) << 8) | ((rtsp_rtp_ctl_blk.rtp_sequence & 0xff00) >> 8);
			    rtphdr.ts = ((rtsp_rtp_ctl_blk.rtp_ts & 0xff) << 24) | ((rtsp_rtp_ctl_blk.rtp_ts & 0xff00) << 8) | ((rtsp_rtp_ctl_blk.rtp_ts & 0xff0000) >> 8) | ((rtsp_rtp_ctl_blk.rtp_ts & 0xff000000) >> 24);
			    rtphdr.ssrc = rtsp_rtp_ctl_blk.rtp_ssrc;

			    memcpy(pkt_ptr, &rtphdr, sizeof(rtphdr));

				pkt_ptr += sizeof(rtphdr);
			    /* Fill FU indicator */
				pkt_ptr[0] = (nal_byte & 0xE0) | NAL_FU_TYPE;

				/* Fill FU header */
				pkt_ptr[1] = (nal_byte & 0x1F);

				if(remain_len == (len - 1))
				{
					/* Start of FU NAL frame */
					pkt_ptr[1] |= (1 << 7);
					//DBG_PRINT("NAL 0x%x FU[%d] start 0x%x 0x%x\r\n", nal_byte, i, pkt_ptr[0], pkt_ptr[1]);
				}
				//else
                	//DBG_PRINT("NAL 0x%x FU[%d] 0x%x 0x%x\r\n", nal_byte, i, pkt_ptr[0], pkt_ptr[1]);
				pkt_ptr += 2;
				/* Copy H264 payload to packet buffer */
				#if 0
				memcpy(pkt_ptr, data_ptr, tx_len);
				#else
				if(dma_data_memcpy((INT8U *)pkt_ptr, (const INT8U *)data_ptr, tx_len))
				{
					gp_memcpy((INT8S *)pkt_ptr, (INT8S *)data_ptr, tx_len);
				}
				#endif

				remain_len -= tx_len;
				data_ptr += tx_len;

				#if 0
				tick_start = xTaskGetTickCount();
				#endif

				ret = send(rtsp_tcp_ctl_blk.tcp_client, rtsp_tcp_ctl_blk.tcpsrv_tx_buf, RTSP_RTP_H264_PKT_SIZE, MSG_MORE);

				#if 0
				tick_end = xTaskGetTickCount();
				DBG_PRINT("\r\nsend2(%d)"
					, tick_end - tick_start
				);
				#endif

		        if(ret < 0)
		        {
		            DBG_PRINT("Send H264 payload error, ret = %d, len %d\r\n", ret, RTSP_RTP_H264_PKT_SIZE);
					break;
		        }
			}
			else
			{
				/* Interleaved frame */
				total_len = sizeof(rtphdr) + 2 + remain_len;  /* RTP header + FU indicator + FU header + H264 payload */
				pkt_ptr[0] = '$';
				pkt_ptr[1] = 0x00;							/* This is a data frame */
				pkt_ptr[2] = (total_len & 0xFF00) >> 8;		/* MSB of length of RTP + H264 payload */
				pkt_ptr[3] = total_len & 0xFF;				/* LSB of length of RTP + H264 payload */

	            pkt_ptr += 4;

	            /* Initialize RTP header */
				rtsp_rtp_ctl_blk.rtp_ts = ts;

			    rtphdr.version = 2;
			    rtphdr.p  = 0;
			    rtphdr.x  = 0;
			    rtphdr.cc = 0;
	            rtphdr.m  = 0;
			    rtphdr.pt = RTP_PT_H264;
			    rtphdr.seq = ((rtsp_rtp_ctl_blk.rtp_sequence & 0xff) << 8) | ((rtsp_rtp_ctl_blk.rtp_sequence & 0xff00) >> 8);
			    rtphdr.ts = ((rtsp_rtp_ctl_blk.rtp_ts & 0xff) << 24) | ((rtsp_rtp_ctl_blk.rtp_ts & 0xff00) << 8) | ((rtsp_rtp_ctl_blk.rtp_ts & 0xff0000) >> 8) | ((rtsp_rtp_ctl_blk.rtp_ts & 0xff000000) >> 24);
			    rtphdr.ssrc = rtsp_rtp_ctl_blk.rtp_ssrc;

			    memcpy(pkt_ptr, &rtphdr, sizeof(rtphdr));

				pkt_ptr += sizeof(rtphdr);

			    /* Fill FU indicator */
				pkt_ptr[0] = (nal_byte & 0xE0) | NAL_FU_TYPE;

			    /* End of FU NAL frame */
				pkt_ptr[1] = (nal_byte & 0x1F) | (1 << 6);
				if(islast)
				{
            		rtphdr.m  = 1;
            	}
                //DBG_PRINT("NAL 0x%x FU[%d] last, 0x%x 0x%x\r\n", nal_byte, i, pkt_ptr[0], pkt_ptr[1]);
            	pkt_ptr += 2;
				#if 0
				memcpy(pkt_ptr, data_ptr, remain_len);
				#else
				if(dma_data_memcpy((INT8U *)pkt_ptr, (const INT8U *)data_ptr, remain_len))
				{
					gp_memcpy((INT8S *)pkt_ptr, (INT8S *)data_ptr, remain_len);
				}
				#endif
				remain_len = 0;

				#if 0
				tick_start = xTaskGetTickCount();
				#endif

				ret = send(rtsp_tcp_ctl_blk.tcp_client, rtsp_tcp_ctl_blk.tcpsrv_tx_buf, total_len + 4, 0);

				#if 0
				tick_end = xTaskGetTickCount();
				DBG_PRINT("\r\nsend3(%d)"
					, tick_end - tick_start
				);
				#endif

		        if(ret < 0)
		        {
		            DBG_PRINT("Send H264 payload error, ret %d,len %d\r\n", ret, total_len+4);
					break;
		        }
			}
			//i++;
			rtsp_rtp_ctl_blk.rtp_sequence++;
		}
	}

	return ret;
}

int rtsp_rtp_send_h264_frame(int stream_addr, int stream_len, INT32U ts)
{
	NAL_INFO_T nal_info;
	int addr = stream_addr;
	int len = stream_len;
	int ret = 0;

	#if 0
	INT32U tick_start = xTaskGetTickCount();
	INT32U tick_end;
	#endif

	while(1)
	{
		/* Find NAL for RTP streaming */
		_find_h264_nal_info(&nal_info, addr, len);
		if(nal_info.start_addr)
		{
			//DBG_PRINT("NAL found, addr 0x%x, len = 0x%x, ts 0x%x, islast %d\r\n", nal_info.start_addr, nal_info.length, ts, nal_info.islast);
			/* Send interleaved frame for RTP H264 payload */
			ret = rtsp_rtp_send_h264_nal((nal_info.start_addr + 4), nal_info.length, ts, nal_info.islast);

			if(nal_info.islast || ret <= 0)
			{
				/* last nal */
				break;
			}
			addr += nal_info.length + 4;
			len -=  nal_info.length + 4;
		}
		else
		{
			/* escape loop */
			break;
		}
	}

	#if 0
	tick_end = xTaskGetTickCount();
	DBG_PRINT("\r\nrtsp_rtp_send_h264_frame(0x%X,%d,%d)\t=[%d,%d, %d]"
		, stream_addr
		, stream_len
		, ts
		, tick_start
		, tick_end
		, tick_end - tick_start
	);
	#endif

	return ret;
}

#define DBG_LEN  0x20
static int rtp_seq_flag = 0;
static JPEG_HDR_T jpeghdr;
RTP_CTL_BLK_T rtp_ctl_blk =
{
	0,		/* Time Stamp in RTP header */
	640,	/* Width of picture in RTP JPEG header */
	480,	/* Height of picture in RTP JPEG header */
	8000,	/* Audio streaming sample rate */
	100,	/* Audio ptime */
	0,		/* SSRC in RTP header */
	0, 		/* Transfer size for each RTP data transfer */
	0,		/* RTP packet send flag */
	0,		/* Transfer data size */
	0,		/* Remain transfer size */
	0,		/* Sequence number in RTP header */
	{0},	/* Remote client IP address */
	NULL,	/* RTP packet buffer pointer */
	NULL,	/* RTP data buffer */
	RTP_PT_JPEG,	/* RTP payload type */
};
#if C_APP_PLAYBACK_WITH_SOUND
RTP_CTL_BLK_T rtp_aud_ctl_blk =
{
	0,		/* Time Stamp in RTP header */
	640,	/* Width of picture in RTP JPEG header */
	480,	/* Height of picture in RTP JPEG header */
	8000,	/* Audio streaming sample rate */
	100,	/* Audio ptime */
	0,		/* SSRC in RTP header */
	0, 		/* Transfer size for each RTP data transfer */
	0,		/* RTP packet send flag */
	0,		/* Transfer data size */
	0,		/* Remain transfer size */
	0,		/* Sequence number in RTP header */
	{0},	/* Remote client IP address */
	NULL,	/* RTP packet buffer pointer */
	NULL,	/* RTP data buffer */
	RTP_PT_PCMA,	/* RTP payload type */
};
#endif
int rtp_send_packet(char* buf, INT32U size)
{
	struct sockaddr_in to_addr;
	int err;
	INT32U retrycnt=0;

	/* Copy remote client IP address */
    sprintf(rtp_ctl_blk.rtp_remote_ip, "%d.%d.%d.%d", rtsp_instance.connect_ip[0] & 0xFF, rtsp_instance.connect_ip[1] & 0xFF, rtsp_instance.connect_ip[2] & 0xFF, rtsp_instance.connect_ip[3] & 0xFF);
    to_addr.sin_len = sizeof(to_addr);
    to_addr.sin_port = htons(rtsp_instance.client_rtp_port1);
	to_addr.sin_family = AF_INET;
	to_addr.sin_addr.s_addr = inet_addr(rtp_ctl_blk.rtp_remote_ip);
	///DBG_PRINT("port1:%x\r\n", rtsp_instance.client_rtp_port1);
	///DBG_PRINT("rtp_remote_ip:%s\r\n", rtp_ctl_blk.rtp_remote_ip);
	///DBG_PRINT("size:%d\r\n", size);
	/* Check WLAN buffer is enough or not */
//check_skb:

send_again:
	err = sendto(rtp_server_ctl_blk.udp_server, buf, size, 0, (struct sockaddr *)&to_addr, sizeof(to_addr));

	if(err <= 0)
	{
		DBG_PRINT("r");
	  	vTaskDelay(5);
		retrycnt++;
		if (retrycnt>100)
		{
			return size;
		}
		goto send_again;
		//return -1;
	}
	else
	{
		return err;
	}
}
#if C_APP_PLAYBACK_WITH_SOUND
int rtp_aud_send_packet(char* buf, INT32U size)
{
	struct sockaddr_in to_addr;
	int err;
	INT32U retrycnt = 0;

	/* Copy remote client IP address */
    sprintf(rtp_ctl_blk.rtp_remote_ip, "%d.%d.%d.%d", rtsp_instance.connect_ip[0] & 0xFF, rtsp_instance.connect_ip[1] & 0xFF, rtsp_instance.connect_ip[2] & 0xFF, rtsp_instance.connect_ip[3] & 0xFF);
    to_addr.sin_len = sizeof(to_addr);
    to_addr.sin_port = htons(rtsp_instance.client_rtp_aud_port1);
	to_addr.sin_family = AF_INET;
	to_addr.sin_addr.s_addr = inet_addr(rtp_ctl_blk.rtp_remote_ip);

send_again:
	err = sendto(rtp_server_ctl_blk.udp_server, buf, size, 0, (struct sockaddr *)&to_addr, sizeof(to_addr));

	if(err <= 0)
	{
		DBG_PRINT("R");
		retrycnt++;
		if (retrycnt>100)
		{
			return size;
		}
		vTaskDelay(50);
		goto send_again;
		//return -1;
	}
	else
	{
		retrycnt = 0;
		return err;
	}
}
#endif
int rtsp_rtp_send_jpeg_frame(INT32U buf_addr, INT32U size, INT32U ts)
{
	char   *buf =(char *)buf_addr;
	//INT32U t1, t2;
	INT32U reamin_size = size;
	INT32U tx_len, copy_len;
	char* pkt_ptr;
	char* jpeg_ptr;
	//char* temp;
	int /*err, */ret;
	//DBG_PRINT("RTP SEND size %d, ts %d\r\n", size, ts);

	/* Reset JPEG offset */
	rtp_ctl_blk.rtp_transfer_size = 0;
	rtp_ctl_blk.rtp_ts = ts;

	/* Initialize RTP header */
    rtphdr.version = 2;
    rtphdr.p = 0;
    rtphdr.x = 0;
    rtphdr.cc = 0;
    rtphdr.m = 0;
    rtphdr.pt = RTP_PT_JPEG;
    rtphdr.seq = ((rtp_ctl_blk.rtp_sequence & 0xff) << 8) | ((rtp_ctl_blk.rtp_sequence & 0xff00) >> 8);
    rtphdr.ts = ((rtp_ctl_blk.rtp_ts & 0xff) << 24) | ((rtp_ctl_blk.rtp_ts & 0xff00) << 8) | ((rtp_ctl_blk.rtp_ts & 0xff0000) >> 8) | ((rtp_ctl_blk.rtp_ts & 0xff000000) >> 24);
    rtphdr.ssrc = rtp_ctl_blk.rtp_ssrc;

	/* Initialize JPEG header */
    jpeghdr.tspec = 0;
    jpeghdr.type = 0;
    jpeghdr.q = 1;
    jpeghdr.width = rtp_ctl_blk.rtp_width >> 3;
    jpeghdr.height = rtp_ctl_blk.rtp_height >> 3;
    jpeghdr.off = ((rtp_ctl_blk.rtp_transfer_size & 0xff) << 16) | (rtp_ctl_blk.rtp_transfer_size & 0x00ff00) | ((rtp_ctl_blk.rtp_transfer_size & 0xff0000) >> 16);


	/* Add Debug Information */
	if (rtp_seq_flag)
	//if (1)
	{
		INT8U  *pGPEncoder;
		INT32U addr, *p;
		INT32U data;

		pGPEncoder = (INT8U*)(buf + 0x1E);
		data = (INT32U)(*pGPEncoder++)<<24;
		data += (INT32U)(*pGPEncoder++)<<16;
		data += (INT32U)(*pGPEncoder++)<<8;
		data += (INT32U)(*pGPEncoder++);

		addr = (INT32U)buf + size;
		reamin_size = size;
		addr = (addr + 4) & (~0x3);
		p = (INT32U *)addr;
		*p++ = 0x47424447;		// symbol 'GDBG'
		*p++ = rtp_ctl_blk.rtp_ts;
		*p++ = data;
		//*p++ = lwip_stats.memp[MEMP_PBUF_POOL].avail;
		//*p++ = lwip_stats.memp[MEMP_PBUF_POOL].used;
		//*p++ = lwip_stats.memp[MEMP_PBUF_POOL].max;
		//DBG_PRINT("data=0x%x\r\n",data);
		reamin_size += DBG_LEN;		// dbg message
		rtp_seq_flag = 1;
	}

	/* Assign JPEG data buffer */
	jpeg_ptr = buf;

	//t1 = xTaskGetTickCount();
	while(reamin_size)
	{
		if(reamin_size > RTP_PAYLOAD_SIZE)
		{
			tx_len = RTP_PAYLOAD_SIZE;
		}
		else
		{
			/* This is end of frame */
			tx_len = reamin_size;
			rtphdr.m = 1;
		}

		//DBG_PRINT("In loop remain %d txlen %d, tx size %d\r\n", reamin_size, tx_len, rtp_ctl_blk.rtp_transfer_size);
		pkt_ptr = (char*)rtsp_tcp_ctl_blk.tcpsrv_tx_buf;

		/* Copy RTP header */
		copy_len = sizeof(rtphdr);		// 12 bytes
		memcpy(pkt_ptr, (char*)&rtphdr, copy_len);
		pkt_ptr += copy_len;

		/* Copy RTP JPEG header */
		copy_len = sizeof(jpeghdr);		// 8 bytes
		memcpy(pkt_ptr, (char*)&jpeghdr, copy_len);
		pkt_ptr += copy_len;

		//DBG_PRINT("JDATA:0x%x 0x%x 0x%x 0x%x\r\n", jpeg_ptr[0], jpeg_ptr[1], jpeg_ptr[2], jpeg_ptr[3]);
		memcpy(pkt_ptr, jpeg_ptr, tx_len);
		ret = rtp_send_packet(rtsp_tcp_ctl_blk.tcpsrv_tx_buf, (tx_len+sizeof(rtphdr)+sizeof(jpeghdr)));
		tx_len = ret - (sizeof(rtphdr)+sizeof(jpeghdr));
		jpeg_ptr+= tx_len;
		rtp_ctl_blk.rtp_transfer_size += tx_len;
		reamin_size -= tx_len;
		rtp_ctl_blk.rtp_sequence++;

		jpeghdr.off = ((rtp_ctl_blk.rtp_transfer_size & 0xff) << 16) | (rtp_ctl_blk.rtp_transfer_size & 0x00ff00) | ((rtp_ctl_blk.rtp_transfer_size & 0xff0000) >> 16);
		rtphdr.seq = ((rtp_ctl_blk.rtp_sequence & 0xff) << 8) | ((rtp_ctl_blk.rtp_sequence & 0xff00) >> 8);
	}
	//t2 = xTaskGetTickCount();
	//DBG_PRINT("t:%d\r\n",t2-t1);

	if (rtp_seq_flag)
	{
		INT32U addr = (INT32U)buf + size;
		gp_memset((INT8S*)addr, (INT8U)0, (INT32U)DBG_LEN);
	}

	return 0;
}
#if C_APP_PLAYBACK_WITH_SOUND
#define RTP_AUD_PAYLOAD_SIZE 1024

int rtp_send_pcm(char* buf, unsigned int size, unsigned int ts)
{
	unsigned int tx_len, copy_len;
	INT32U reamin_size = size;
	char* pkt_ptr;
	int ret;
	char* aud_ptr;

	rtp_aud_ctl_blk.rtp_ts = ts;

	/* Initialize RTP header */
    rtpaud_hdr.version = 2;
    rtpaud_hdr.p = 0;
    rtpaud_hdr.x = 0;
    rtpaud_hdr.cc = 0;
    rtpaud_hdr.m = 0;
    rtpaud_hdr.pt = rtp_aud_ctl_blk.rtp_payload_type&0x7f;
    rtpaud_hdr.seq = ((rtp_aud_ctl_blk.rtp_sequence & 0xff) << 8) | ((rtp_aud_ctl_blk.rtp_sequence & 0xff00) >> 8);
    rtpaud_hdr.ts = ((rtp_aud_ctl_blk.rtp_ts & 0xff) << 24) | ((rtp_aud_ctl_blk.rtp_ts & 0xff00) << 8) | ((rtp_aud_ctl_blk.rtp_ts & 0xff0000) >> 8) | ((rtp_aud_ctl_blk.rtp_ts & 0xff000000) >> 24);
    rtpaud_hdr.ssrc = rtp_aud_ctl_blk.rtp_ssrc;


	aud_ptr = buf;
	while(reamin_size)
	{
		//DBG_PRINT("s_:%d\r\n",reamin_size);
		if(reamin_size > RTP_AUD_PAYLOAD_SIZE)
		{
			tx_len = RTP_AUD_PAYLOAD_SIZE;
		}
		else
		{
			/* This is end of frame */
			tx_len = reamin_size;
		}
		rtpaud_hdr.m = 1;

		pkt_ptr = (char*)rtp_aud_ctl_blk.rtp_packet_buf;

		/* Copy RTP header */
		copy_len = sizeof(rtpaud_hdr);
		memcpy(pkt_ptr, (char*)&rtpaud_hdr, copy_len);
		pkt_ptr += copy_len;

		memcpy(pkt_ptr, aud_ptr, tx_len);
		ret=rtp_aud_send_packet(rtp_aud_ctl_blk.rtp_packet_buf, (tx_len+sizeof(rtpaud_hdr)));
		if (ret <=0)
		{
			DBG_PRINT("drop aud frame\r\n");
			break;
		}

		tx_len=ret - sizeof(rtpaud_hdr);
		aud_ptr+= tx_len;
		rtp_aud_ctl_blk.rtp_transfer_size += tx_len;
		reamin_size -= tx_len;

		/* Update sequence number and timestamp for per frame */
		rtp_aud_ctl_blk.rtp_sequence++;
		rtp_aud_ctl_blk.rtp_ts += tx_len;
		rtpaud_hdr.seq = ((rtp_aud_ctl_blk.rtp_sequence & 0xff) << 8) | ((rtp_aud_ctl_blk.rtp_sequence & 0xff00) >> 8);
		rtpaud_hdr.ts = ((rtp_aud_ctl_blk.rtp_ts & 0xff) << 24) | ((rtp_aud_ctl_blk.rtp_ts & 0xff00) << 8) | ((rtp_aud_ctl_blk.rtp_ts & 0xff0000) >> 8) | ((rtp_aud_ctl_blk.rtp_ts & 0xff000000) >> 24);
	}

	return 0;
}



#define SAMPLE_RATE_8K	8000
#define SAMPLE_BLOCK 8
static MESSAGE_INFO_STRUCT rtp_aud_info;
static char rtp_aud_buf[SAMPLE_BLOCK][SAMPLE_RATE_8K];
static int rtp_aud_buf_idx[SAMPLE_BLOCK] = {0};

enum {
	SIGN_BIT = 0x80,  /* Sign bit for a A-law byte. */
	QUANT_MASK = 0xF, /* Quantization field mask. */
	NSEGS = 8, 	   /* Number of A-law U-law segments. */
	SEG_SHIFT = 4,    /* Left shift for segment number. */
	SEG_MASK = 0x70,  /* Segment field mask. */
	BIAS = 0x84	   /* Bias for linear code. */
 };

static int search(int val)
{
	int i;
	short seg_end[NSEGS] = {0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

	for (i = 0; i < NSEGS; ++i) {
		if (val <= seg_end[i])
			return i;
	}
	return NSEGS;
}

static unsigned char linear2alaw(int pcm_val)	 /* 2's complement (16-bit range) */
{
	int		 mask;
	int		 seg;
	unsigned char aval;

	if (pcm_val >= 0) {
		mask = 0xD5;		 /* sign (7th) bit = 1 */
	}
	else {
		mask = 0x55;		 /* sign bit = 0 */
		pcm_val = -pcm_val - 8;
	}
	if (pcm_val < 0) pcm_val = 0;

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val);

	/* Combine the sign, segment, and quantization bits. */
	if (seg >= 8)	 /* out of range, return maximum value. */
		return (0x7F ^ mask);
	else {
		aval = seg << SEG_SHIFT;
		if (seg < 2)
			aval |= (pcm_val >> 4) & QUANT_MASK;
		else
			aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
		return (aval ^ mask);
	}
}

int audio_send_idx(void)
{
	int i, ret = -1;
	for (i=0; i<SAMPLE_BLOCK; ++i)
	{
		if (rtp_aud_buf_idx[i]==0)
		{
			ret = i;
			break;
		}
	}

	return ret;
}

// only support 8K/16K Hz
INT32S audio_send(INT32U addr, INT32U size)	// unit: words
{
	short *ptr = (short *)addr;
	int pcm_size = size;
	int i, idx, ret = 0;

	idx =audio_send_idx();
    if(idx!=-1)
   	{
		//DBG_PRINT("m:%d\r\n",size);
		for (i=0; i<pcm_size; ++i)
		{
			rtp_aud_buf[idx][i] = linear2alaw(ptr[2*i]);		// 16K Hz -> 8KHz
		}
		rtp_aud_info.stream_addr = (INT32U)(rtp_aud_buf[idx]);
		rtp_aud_info.stream_size = (INT32U)(pcm_size>>1);		// pcm_size/2 for 8KHz

		msgQSend(rtsp_tcp_ctl_blk.tcpsrv_tx_q, MSG_AUD_FRAME_DONE, &rtp_aud_info, sizeof(MESSAGE_INFO_STRUCT), MSG_PRI_NORMAL);
   	}
	else
	{
		ret = 1;
	}

	return ret;
}

INT32S wifi_aud_finish(INT32U addr)
{
	INT32U i, aud_buf;
	int ret = 0;

	for (i=0; i<SAMPLE_BLOCK; ++i)
	{
		aud_buf = (INT32U)(rtp_aud_buf[i]);
		if (addr == aud_buf)
		{
			rtp_aud_buf_idx[i] = 0;
			break;
		}
	}

	if (i>=SAMPLE_BLOCK)
	{
		DBG_PRINT("RTP audio buffer err\r\n");
		ret = -1;
	}

	return ret;
}
#endif


int rtsp_connection_get(void)
{
	return gConnect_status;///rtsp_tcp_ctl_blk.tcp_client;
}

int rtsp_h264_connection_status(void)
{
	return rtsp_tcp_ctl_blk.tcp_client;
}

#if ((WIFI_RUN_MODE == WIFI_SMARTCONFIG_MODE)||(WIFI_RUN_MODE == WIFI_STATION_MODE))
extern void socket_connection_timer_stop(void);
#endif
#if 1
volatile static INT8U rtsp_connect_status=1;

void
rtsp_h264_rx_task(
	void const *arg
)
{
	struct sockaddr_in server_addr, cli_addr, temp_addr;
	struct sockaddr_in udp_server_addr;
	socklen_t client, temp_len;
	int status;
	int ret, len;//, msg, cnt;
	int	enable = 1;
	//unsigned int remain;
	//err_t err;
	int keepalive = 1;
	int keep_idle = g_keep_idle;
	int keep_intvl = g_keep_intvl;
	int keep_cnt = g_keep_cnt;
	int nodelay = 1;
    //MESSAGE_INFO_STRUCT *info;
    INT32U softap_ip = 0;
    struct timeval tv;
	fd_set readfds;
	int max_fd;
	int addr_reuse = 1;

	/* Create TCP server and wait for connect */
	rtsp_tcp_ctl_blk.tcp_server = socket(AF_INET, SOCK_STREAM, 0);
	if(rtsp_tcp_ctl_blk.tcp_server < 0)
	{
		DBG_PRINT("RTSP socket failed\r\n");
		goto exit_rx_task;
	}

	status = setsockopt(rtsp_tcp_ctl_blk.tcp_server, SOL_SOCKET, SO_KEEPALIVE, (const char*)&enable, sizeof(enable));
	if(status < 0)
	{
		DBG_PRINT("RTSP SO_KEEPALIVE error\r\n");
		goto exit_rx_task;
	}
#if (WIFI_VENDOR == 1)
	/* Set socket address reusable */
	status = setsockopt(rtsp_tcp_ctl_blk.tcp_server, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable));
	if(status < 0)
	{
		DBG_PRINT("RTSP SO_REUSEADDR error %d\r\n", status);
		goto exit_rx_task;
	}
#endif
	/* Bind the TCP server socket address */
	memset((char *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_len         = sizeof(server_addr);
	server_addr.sin_port        = htons(rtsp_tcp_ctl_blk.server_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	status = bind(rtsp_tcp_ctl_blk.tcp_server, (struct sockaddr *)&server_addr,  sizeof(server_addr));
	if(status < 0)
	{
		DBG_PRINT("RTSP bind error\r\n");
		goto exit_rx_task;
	}

	/* Listen */
	if(listen(rtsp_tcp_ctl_blk.tcp_server , 1) < 0)
	{
		DBG_PRINT("RTSP listen error\r\n");
		goto exit_rx_task;
	}

	rtsp_instance.server_rtp_port1 = RTP_SERVER_PORT;
	rtsp_instance.server_rtp_port2 = RTCP_SERVER_PORT;
	#if C_APP_PLAYBACK_WITH_SOUND
	rtsp_instance.server_rtp_aud_port1 = RTP_SERVER_AUD_PORT;
	rtsp_instance.server_rtp_aud_port2 = RTCP_SERVER_AUD_PORT;
	#endif
	rtsp_instance.session_id = RTP_SESSION_ID;
	/* Get local IP */
#if (WIFI_VENDOR == 1)
	softap_ip = ssv_wifi_get_local_ip();
#elif (WIFI_VENDOR == 2)
    /* To do, Eugene 2019/04/29 */
    memset((char *)&temp_addr, 0, sizeof(temp_addr));
    temp_len = sizeof(temp_addr);
    getsockname(rtsp_tcp_ctl_blk.tcp_server, (struct sockaddr*)&temp_addr, &temp_len);

	#if 0// To fix it
	softap_ip = htonl(inet_addr("192.168.25.1"));
	#endif
#endif
	rtsp_instance.local_ip[0] = (softap_ip & 0xFF000000) >> 24;
	rtsp_instance.local_ip[1] = (softap_ip & 0x00FF0000) >> 16;
	rtsp_instance.local_ip[2] = (softap_ip & 0x0000FF00) >> 8;
	rtsp_instance.local_ip[3] = (softap_ip & 0xFF);
	//DBG_PRINT("Local IP address %d.%d.%d.%d, session id %d\r\n", rtsp_instance.local_ip[0]&0xff, rtsp_instance.local_ip[1]&0xff, rtsp_instance.local_ip[2], rtsp_instance.local_ip[3], rtsp_instance.session_id);

	//DBG_PRINT("Enter rtsp_task port %d\r\n", rtsp_tcp_ctl_blk.server_port);
	client = sizeof(cli_addr);

       // Create a UDP server (for JPEG streamming)
	rtp_server_ctl_blk.udp_server = socket(AF_INET, SOCK_DGRAM, 0);
	if(rtp_server_ctl_blk.udp_server < 0)
	{
		DBG_PRINT("RP socket failed\r\n");
		goto exit_rx_task;
	}

	status = setsockopt(rtp_server_ctl_blk.udp_server, SOL_SOCKET, SO_REUSEADDR, (const char *) &addr_reuse, sizeof(addr_reuse));
	if(status < 0)
	{
		DBG_PRINT("RP REUSEADDR error 0x%x\r\n", status);
	}

	/* Bind server to local port and IP address */
	FD_ZERO(&udp_server_addr);
	udp_server_addr.sin_family = AF_INET;
	udp_server_addr.sin_addr.s_addr = INADDR_ANY;
	udp_server_addr.sin_port = htons(rtp_server_ctl_blk.server_port);

	if (bind(rtp_server_ctl_blk.udp_server, (struct sockaddr *)&udp_server_addr, sizeof(udp_server_addr)) < 0)
	{
		DBG_PRINT("UDP RTP bind error\r\n");
	}

	/* Accept TCP client connection */
 	while((rtsp_tcp_ctl_blk.tcp_client = accept(rtsp_tcp_ctl_blk.tcp_server, (struct sockaddr *)&cli_addr, &client)) >= 0)
 	{
		DBG_PRINT("RTSP: remote IP %d.%d.%d.%d connected\r\n", cli_addr.sin_addr.s_addr&0x000000FF, (cli_addr.sin_addr.s_addr&0x0000FF00) >> 8
 																, (cli_addr.sin_addr.s_addr&0x00FF0000) >> 16, (cli_addr.sin_addr.s_addr&0xFF000000) >> 24);
		/* Record remote connected IP address */
		rtsp_instance.connect_ip[0] = cli_addr.sin_addr.s_addr&0x000000FF;
		rtsp_instance.connect_ip[1] = (cli_addr.sin_addr.s_addr&0x0000FF00) >> 8;
		rtsp_instance.connect_ip[2] = (cli_addr.sin_addr.s_addr&0x00FF0000) >> 16;
		rtsp_instance.connect_ip[3] = (cli_addr.sin_addr.s_addr&0xFF000000) >> 24;
		rtsp_connect_status = 0;

        #if ((WIFI_RUN_MODE == WIFI_SMARTCONFIG_MODE)||(WIFI_RUN_MODE == WIFI_STATION_MODE))
        socket_connection_timer_stop();
        #endif

		//keep alive usage
		if(setsockopt(rtsp_tcp_ctl_blk.tcp_client, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) != 0)
		{
			DBG_PRINT("ERR: SO_KEEPALIVE\r\n");
			goto exit_rx_task;
		}

		//set socket attribute
		if(setsockopt(rtsp_tcp_ctl_blk.tcp_client, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle)) < 0 )
		{
			DBG_PRINT("ERR: TCP_KEEPIDLE\r\n");
			goto exit_rx_task;
		}

		if(setsockopt(rtsp_tcp_ctl_blk.tcp_client, IPPROTO_TCP, TCP_KEEPINTVL, &keep_intvl, sizeof(keep_intvl)) < 0 )
		{
			DBG_PRINT("ERR: TCP_KEEPINTVL\r\n");
			goto exit_rx_task;
		}

		if(setsockopt(rtsp_tcp_ctl_blk.tcp_client, IPPROTO_TCP, TCP_KEEPCNT, &keep_cnt, sizeof(keep_cnt)) < 0 )
		{
			DBG_PRINT("TC set TCP_KEEPCNT err! ");
			goto exit_rx_task;
		}
		DBG_PRINT("tcp client Keep alive\r\n");

		#if 1
		if(setsockopt(rtsp_tcp_ctl_blk.tcp_client, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) < 0 )
		{
			DBG_PRINT("ERR: TCP_NODELAY\r\n");
		}
		#endif

		#if 1
		enable = 100;		// timeout ticks
		if(setsockopt(rtsp_tcp_ctl_blk.tcp_client, SOL_SOCKET, SO_RCVTIMEO, &enable, sizeof(enable)) < 0 )
		{
			DBG_PRINT("SO_RCVTIMEO error!\r\n");
		}
		#endif

		while(1)
		{
			FD_ZERO(&readfds);
		    max_fd = -1;

			//tv.tv_sec = 1;		/* 1 second timeout */
			tv.tv_sec = 0;
			tv.tv_usec = 0;

			FD_SET(rtsp_tcp_ctl_blk.tcp_client, &readfds);
			max_fd = rtsp_tcp_ctl_blk.tcp_client;

			ret = select((max_fd + 1), &readfds, NULL, NULL, &tv);

			if(ret > 0)
			{
				rtsp_tcp_h264_lock();
				len = recv(rtsp_tcp_ctl_blk.tcp_client, (void *)rtsp_tcp_ctl_blk.tcpsrv_rx_buf, RTSP_RX_BUF_LEN, 0);
				if(len > 0)
				{
					//DBG_PRINT("RX:len %d\r\n", len);
					ret = rtsp_process_method(len);
				}
				#if (WIFI_VENDOR == 2)
				else if(errno == ETIMEDOUT)	//timeout
				{
					#if 0
					DBG_PRINT("RTSP read socket timeout\r\n");
					#endif
				}
				#endif
				else
				{
					/* read socket error, exit loop */
					#if (WIFI_VENDOR == 2)
					DBG_PRINT("RTSP read socket error=%d\r\n", errno);
					#endif
					rtsp_connect_status = 1;
					rtsp_tcp_h264_unlock();
					break;
				}
				rtsp_tcp_h264_unlock();
			}
			else
			{
				vTaskDelay(g_rtsp_cmd_wait);

				if (rtsp_connect_status)
				{
					DBG_PRINT("rtsp break\r\n");
					break;
				}
				extern int GPSOCK_Playback_GetThumbnail_BusyGet(void);
				if ( GPSOCK_Playback_GetThumbnail_BusyGet() )
				{
					DBG_PRINT("Thumbnail busy, break\r\n");
				}
			}
		}

		#if ((WIFI_RUN_MODE == WIFI_SMARTCONFIG_MODE)||(WIFI_RUN_MODE == WIFI_STATION_MODE))
		socket_connection_timer_stop();
		#endif
		DBG_PRINT("rtsp_connect_status = %d\r\n",rtsp_connect_status);
		rtsp_tcp_h264_lock();
		/* Close connected client socket then wait for new client connection */
		DBG_PRINT("rtsp_tcp_ctl_blk.tcp_client = %d\r\n",rtsp_tcp_ctl_blk.tcp_client);
		if(rtsp_tcp_ctl_blk.tcp_client >= 0)
		{

			if(lwip_close(rtsp_tcp_ctl_blk.tcp_client) != 0)
			{
				DBG_PRINT("close tcp client error\r\n");
			}

			wifi_transmission_stop();
			rtsp_tcp_ctl_blk.tcp_client = -1;

        	DBG_PRINT("Exit RX H264 client socket1.\r\n");
        }
		rtsp_tcp_h264_unlock();
		DBG_PRINT("Exit RX H264 client socket2.\r\n");
		gConnect_status = 0;///var = 0;
	}

exit_rx_task:
	DBG_PRINT("exit rx task\r\n");
	while(1)
	{
		osDelay(1000);
	}
}
#else
void rtsp_h264_rx_task(void const *arg)
{
	struct sockaddr_in server_addr, cli_addr;
	socklen_t client;
	int status;
	int ret, len, msg, cnt;
	int	enable = 1;
	unsigned int remain;
	err_t err;
	int keepalive = 1;
	int keep_idle = g_keep_idle;
	int keep_intvl = g_keep_intvl;
	int keep_cnt = g_keep_cnt;
    MESSAGE_INFO_STRUCT *info;
    INT32U softap_ip = 0;

	/* Create TCP server and wait for connect */
	rtsp_tcp_ctl_blk.tcp_server = socket(AF_INET, SOCK_STREAM, 0);
	if(rtsp_tcp_ctl_blk.tcp_server < 0)
	{
		DBG_PRINT("RTSP socket failed\r\n");
		goto exit_rx_task;
	}

	status = setsockopt(rtsp_tcp_ctl_blk.tcp_server, SOL_SOCKET, SO_KEEPALIVE, (const char*)&enable, sizeof(enable));
	if(status < 0)
	{
		DBG_PRINT("RTSP SO_KEEPALIVE error\r\n");
		goto exit_rx_task;
	}
#if (WIFI_VENDOR == 1)
	/* Set socket address reusable */
	status = setsockopt(rtsp_tcp_ctl_blk.tcp_server, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable));
	if(status < 0)
	{
		DBG_PRINT("RTSP SO_REUSEADDR error %d\r\n", status);
		goto exit_rx_task;
	}
#endif
	/* Bind the TCP server socket address */
	memset((char *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_len         = sizeof(server_addr);
	server_addr.sin_port        = htons(rtsp_tcp_ctl_blk.server_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	status = bind(rtsp_tcp_ctl_blk.tcp_server, (struct sockaddr *)&server_addr,  sizeof(server_addr));
	if(status < 0)
	{
		DBG_PRINT("RTSP bind error\r\n");
		goto exit_rx_task;
	}

	/* Listen */
	if(listen(rtsp_tcp_ctl_blk.tcp_server , 1) < 0)
	{
		DBG_PRINT("RTSP listen error\r\n");
		goto exit_rx_task;
	}

	rtsp_instance.server_rtp_port1 = RTP_SERVER_PORT;
	rtsp_instance.server_rtp_port2 = RTCP_SERVER_PORT;
	rtsp_instance.session_id = RTP_SESSION_ID;
	/* Get local IP */
	softap_ip = ssv_wifi_get_local_ip();
	rtsp_instance.local_ip[0] = (softap_ip & 0xFF000000) >> 24;
	rtsp_instance.local_ip[1] = (softap_ip & 0x00FF0000) >> 16;
	rtsp_instance.local_ip[2] = (softap_ip & 0x0000FF00) >> 8;
	rtsp_instance.local_ip[3] = (softap_ip & 0xFF);
	//DBG_PRINT("Local IP address %d.%d.%d.%d, session id %d\r\n", rtsp_instance.local_ip[0]&0xff, rtsp_instance.local_ip[1]&0xff, rtsp_instance.local_ip[2], rtsp_instance.local_ip[3], rtsp_instance.session_id);

	//DBG_PRINT("Enter rtsp_task port %d\r\n", rtsp_tcp_ctl_blk.server_port);
	client = sizeof(cli_addr);
	/* Accept TCP client connection */
 	while((rtsp_tcp_ctl_blk.tcp_client = accept(rtsp_tcp_ctl_blk.tcp_server, (struct sockaddr *)&cli_addr, &client)) >= 0)
 	{
		DBG_PRINT("RTSP: remote IP %d.%d.%d.%d connected\r\n", cli_addr.sin_addr.s_addr&0x000000FF, (cli_addr.sin_addr.s_addr&0x0000FF00) >> 8
 																, (cli_addr.sin_addr.s_addr&0x00FF0000) >> 16, (cli_addr.sin_addr.s_addr&0xFF000000) >> 24);
		/* Record remote connected IP address */
		rtsp_instance.connect_ip[0] = cli_addr.sin_addr.s_addr&0x000000FF;
		rtsp_instance.connect_ip[1] = (cli_addr.sin_addr.s_addr&0x0000FF00) >> 8;
		rtsp_instance.connect_ip[2] = (cli_addr.sin_addr.s_addr&0x00FF0000) >> 16;
		rtsp_instance.connect_ip[3] = (cli_addr.sin_addr.s_addr&0xFF000000) >> 24;

		//keep alive usage
		if(setsockopt(rtsp_tcp_ctl_blk.tcp_client, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) != 0)
		{
			DBG_PRINT("ERR: SO_KEEPALIVE\r\n");
			goto exit_rx_task;
		}

		//set socket attribute
		if(setsockopt(rtsp_tcp_ctl_blk.tcp_client, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle)) < 0 )
		{
			DBG_PRINT("ERR: TCP_KEEPIDLE\r\n");
			goto exit_rx_task;
		}

		if(setsockopt(rtsp_tcp_ctl_blk.tcp_client, IPPROTO_TCP, TCP_KEEPINTVL, &keep_intvl, sizeof(keep_intvl)) < 0 )
		{
			DBG_PRINT("ERR: TCP_KEEPINTVL\r\n");
			goto exit_rx_task;
		}

		if(setsockopt(rtsp_tcp_ctl_blk.tcp_client, IPPROTO_TCP, TCP_KEEPCNT, &keep_cnt, sizeof(keep_cnt)) < 0 )
		{
			DBG_PRINT("TC set TCP_KEEPCNT err! ");
			goto exit_rx_task;
		}
		DBG_PRINT("tcp client Keep alive\r\n");

		while(1)
		{
			len = recv(rtsp_tcp_ctl_blk.tcp_client, (void *)rtsp_tcp_ctl_blk.tcpsrv_rx_buf, RTSP_RX_BUF_LEN, 0);
			if(len > 0)
			{
                //DBG_PRINT("RX:len %d\r\n", len);
				ret = rtsp_process_method(len);
			}
			else
			{
				/* read socket error, exit loop */
				DBG_PRINT("RTSP read socket error\r\n");
				break;
			}
		}

		rtsp_tcp_h264_lock();
		/* Close connected client socket then wait for new client connection */
		if(rtsp_tcp_ctl_blk.tcp_client >= 0)
		{
			if(lwip_close(rtsp_tcp_ctl_blk.tcp_client) != 0)
			{
				DBG_PRINT("close tcp client error\r\n");
			}
			rtsp_tcp_ctl_blk.tcp_client = -1;
        	DBG_PRINT("Exit RX H264 client socket.\r\n");
		}
		rtsp_tcp_h264_unlock();
	}

exit_rx_task:
	DBG_PRINT("exit rx task\r\n");
	while(1)
	{
		osDelay(1000);
	}
}
#endif
INT32U RTSP_run_flag = 0;
void rtsp_h264_tx_task(void const *pvParameters)
{
	int ret = 0;
	int ts = 0;
    MESSAGE_INFO_STRUCT info;
    INT32U msg_type;

	while(1)
	{
        if(msgQReceive(rtsp_tcp_ctl_blk.tcpsrv_tx_q, &msg_type, &info, sizeof(MESSAGE_INFO_STRUCT)) == STATUS_FAIL)
        {
            continue;
        }

		rtsp_tcp_h264_lock();
		RTSP_run_flag = 1;
	    switch (msg_type)
	    {
			case MSG_H264_FRAME_DONE:

				#if 0
				if(g_previous_ts == 0)
				{
		            ts = 0;
		            g_sum_ts = 0;
		            g_previous_ts = (INT32U)(info.timestamp&0xffffffff);
	            }
	            else
	            {
					#if 1
					unsigned int cur_ts = (INT32U)(info.timestamp&0xffffffff);
					#else
					unsigned int cur_ts = g_previous_ts + 40;
					#endif

					//ts = ((cur_ts - g_previous_ts)/1000)*90;
					ts = ((cur_ts - g_previous_ts)*90)/1000;
					if(ts)
					{
						g_sum_ts += ts;
						g_previous_ts = cur_ts;
						//DBG_PRINT("g_previous_ts=%d,g_sum_ts=%d\r\n", g_previous_ts, g_sum_ts);
					}
				}
				#else
				ts = (INT32U)(info.timestamp&0xffffffff) * 90;
				#endif

				#if 0
				//DBG_PRINT("H264 frame addr 0x%x size 0x%x, ts 0x%x, timestamp 0x%x\r\n", info.stream_addr, info.stream_size, g_sum_ts, info.timestamp);
				ret = rtsp_rtp_send_h264_frame(info.stream_addr, info.stream_size, g_sum_ts);
				#else
				if ( (rtsp_tcp_ctl_blk.tcp_client>=0)&&(rtsp_connect_status==0) )
				{
                    ret = rtsp_rtp_send_h264_frame(info.stream_addr, info.stream_size, ts);
				}
				else
				{
                    ret = -1;
				}
				#endif

				if(info.func_done)
				{
					info.func_done(info.stream_addr,info.stream_type);
				}

				#if 0
				if(ret <= 0)
				{
					/* Stop H264 encode */
					usb_webcam_stop();
					//Rtsp_Pscaler_Sensor_Stop(PSCALER_B);

					#if 0
					g_previous_ts = 0;
					#endif

					//flush msg queue
		        	msgQFlush(rtsp_tcp_ctl_blk.tcpsrv_tx_q);
				}
				else
				{
					#if 0
					vTaskDelay(1);
					#endif
				}
				#endif
				break;

			case MSG_RTSP_SEND_REPLY:
				if(rtsp_tcp_ctl_blk.tcp_client >= 0)
				{
					ret = send(rtsp_tcp_ctl_blk.tcp_client, (char*)info.stream_addr, info.stream_size, 0);
					if(ret < 0)
					{
						DBG_PRINT("Send RTSP error %d, len 0x%x\r\n", ret, info.stream_size);
					}
					//DBG_PRINT("MSG_RTSP_SEND: len %d, cnt %d\r\n", info.stream_size, ret);
				}
				if(info.stream_addr)
					gp_free((void*)info.stream_addr);
				break;

			case MSG_JPEG_FRAME_DONE:
				ts = (INT32U)(info.timestamp&0xffffffff) * 90;

				// RTP header doesn't allw 0 ((2048/8)&0xFF)
				// Max JPEG widht is  2040.
				if ( (info.width == 2048)&&(info.height == 1536) )
				{
					info.width = 2040;
					//char *addr = (char*)(info.stream_addr);
					//addr[9] = 0x07; addr[10] = 0xF8;		// 2040x1536
				}

				rtp_ctl_blk.rtp_width = info.width;
				rtp_ctl_blk.rtp_height = info.height;
				//DBG_PRINT("MSG_JPEG_FRAME_DONE (%d, %d)\r\n",rtp_ctl_blk.rtp_width, rtp_ctl_blk.rtp_height);
				ret = rtsp_rtp_send_jpeg_frame(info.stream_addr, info.stream_size, ts);
				//g_browser_reply_action_flag = 2;
				if(info.func_done)
				{
					info.func_done(info.stream_addr,info.stream_type);
				}
				break;
#if C_APP_PLAYBACK_WITH_SOUND
			case MSG_AUD_FRAME_DONE:
				{
					INT32U ts1;
					INT32U addr = info.stream_addr;
					INT32U size = info.stream_size;

					ts1= wifi_playback_tick_get(1);
					//DBG_PRINT("AUD=%d\r\n",ts1);
					rtp_send_pcm((char*)(addr), size, ts1);
					wifi_aud_finish(addr);
				}
				break;
#endif
			default:
				break;
		}

		RTSP_run_flag = 0;
		rtsp_tcp_h264_unlock();
	}

//exit_tx_task:
	DBG_PRINT("exit tx task\r\n");
	while(1)
	{
		osDelay(1000);
	}
}

void
rtsp_h264_start_service(
	void
)
{
	//INT32S err;
  	//INT32U i;

    /* create msg queue */
    if (rtsp_tcp_ctl_blk.tcpsrv_tx_q == NULL)
    {
        rtsp_tcp_ctl_blk.tcpsrv_tx_q = msgQCreate(TCP_QUEUE_LEN, TCP_QUEUE_LEN, sizeof(MESSAGE_INFO_STRUCT));
    }

	if(rtsp_tcp_ctl_blk.tcpsrv_rx_buf == NULL)
	{
		rtsp_tcp_ctl_blk.tcpsrv_rx_buf = (char*)gp_malloc(TCP_RX_BUF_LEN);

		if(!rtsp_tcp_ctl_blk.tcpsrv_rx_buf)
		{
			DBG_PRINT("H264 rx_buf failed\r\n");
			while(1);
		}
	}

	if(rtsp_tcp_ctl_blk.tcpsrv_tx_buf == NULL)
	{
		rtsp_tcp_ctl_blk.tcpsrv_tx_buf = (char*)gp_malloc(TCP_TX_BUF_LEN);

		if(!rtsp_tcp_ctl_blk.tcpsrv_tx_buf)
		{
			DBG_PRINT("H264 tx_buf failed\r\n");
			while(1);
		}
		tcp_tx_write_ptr = rtsp_tcp_ctl_blk.tcpsrv_tx_buf;
	}
    #if C_APP_PLAYBACK_WITH_SOUND
	if(rtp_aud_ctl_blk.rtp_packet_buf == NULL)
	{
		rtp_aud_ctl_blk.rtp_packet_buf = (char*)gp_malloc(RTP_PACKET_SIZE);

		if(!rtp_aud_ctl_blk.rtp_packet_buf)
		{
			DBG_PRINT("RTP Aud pkt_buf failed\r\n");
			while(1);
		}
	}
    #endif
	/* Crete RTSP H264 semaphore */
	if(rtsp_h264_sema == NULL)
	{
		vSemaphoreCreateBinary(rtsp_h264_sema);
	}

	/* Crete RTSP RX server for getting RTSP command from WiFi */
    osThreadDef_t H264_rx_server_def = {"rtsp_h264_rx_task", rtsp_h264_rx_task, PRIORITY_RTSP_H264, 1, TCP_TASK_STACKSIZE};
    rtsp_rx_task = osThreadCreate(&H264_rx_server_def, NULL);
	if(rtsp_rx_task == NULL)
	{
		DBG_PRINT("rtsp_rx_task create failed\r\n");
	}

	/* Crete RTSP RX server for getting RTSP command from WiFi */
    osThreadDef_t H264_tx_server_def = {"rtsp_h264_tx_task", rtsp_h264_tx_task, PRIORITY_RTSP_H264, 1, TCP_TASK_STACKSIZE};
    rtsp_tx_task = osThreadCreate(&H264_tx_server_def, NULL);
	if(rtsp_tx_task == NULL)
	{
		DBG_PRINT("rtsp_tx_task create failed\r\n");
	}
}
int rtsp_h264_service_get(void)
{
	return rtsp_connect_status;
}
void rtsp_h264_stop_service(void)
{
	rtsp_connect_status = 1;

#if 0
	INT32S err;

	//DBG_PRINT("Stop H264\r\n");
	if(rtsp_tcp_ctl_blk.tcp_server >= 0)
	{
		if(close(rtsp_tcp_ctl_blk.tcp_server) != 0)
		{
			DBG_PRINT("close rtsp server error\r\n");
		}
		rtsp_tcp_ctl_blk.tcp_server = -1;
		DBG_PRINT("Close H264 server socket\r\n");
	}

	if(rtsp_tcp_ctl_blk.tcp_client >= 0)
	{
		close(rtsp_tcp_ctl_blk.tcp_client);
		rtsp_tcp_ctl_blk.tcp_client = -1;
		DBG_PRINT("Close RTSP client socket\r\n");
	}

	if(rtsp_tcp_ctl_blk.tcpsrv_rx_buf)
	{
		gp_free((void*)rtsp_tcp_ctl_blk.tcpsrv_rx_buf);
		rtsp_tcp_ctl_blk.tcpsrv_rx_buf = NULL;
	}

	if(rtsp_tcp_ctl_blk.tcpsrv_tx_buf)
	{
		gp_free((void*)rtsp_tcp_ctl_blk.tcpsrv_tx_buf);
		rtsp_tcp_ctl_blk.tcpsrv_tx_buf = NULL;
	}
#endif
}
#endif //	#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)



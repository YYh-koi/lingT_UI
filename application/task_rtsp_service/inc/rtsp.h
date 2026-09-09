/******************************************************
* rtsp.h
*
* Purpose: RTSP header file
*
* Author: Eugene Hsu
*
* Date: 2016/03/21
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version :
* History :
*
*******************************************************/
#ifndef __RTSP_H__
#define __RTSP_H__

#define LINE_END		"\r\n"
#define DESCRIBE_METHOD "DESCRIBE"
#define OPTION_METHOD	"OPTIONS"
#define SETUP_METHOD	"SETUP"
#define PLAY_METHOD		"PLAY"
#define PAUSE_METHOD	"PAUSE"
#define TEARDOWN_METHOD	"TEARDOWN"

#define RTSP_FILE_PATCH			"?action=stream"
#define RTSP_METHOD_URL			"rtsp:"
#define RTSP_CSEQ_HEAD			"CSeq: "
#define RTSP_TRANSPORT_HEAD 	"Transport: "
#define RTSP_CLIENT_PORT_FIELD	"client_port="
#define RTSP_TRANSPORT_TCP		"TCP"
#define RTSP_TRANSPORT_UDP		"UDP"
#define RTSP_SPACE				" "
#define RTSP_INTERLEAVED_TOKEN	"$"

#define RTSP_URL_LEN	64
#define RTSP_CSEQ_LEN	12
#define RTSP_IP_LEN		4

#define RTSP_OPTION_REPLY2		"Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n"

#define RTSP_DESCRIBE_REPLY2	"Content-type: application/sdp\r\nContent-Base: rtsp://"

#define RTSP_SDP_H264_REPLY1	"i=goplus\r\n"
#define RTSP_SDP_H264_REPLY2	"t=0 0\r\nm=video 0 RTP/AVP 96\r\n"
#define RTSP_SDP_H264_REPLY3	"a=rtpmap:96 H264/90000\r\na=control:streamid=0\r\na=fmtp:96 packetization-mode=1\r\n"

#define RTSP_SDP_JPEG_REPLY1	"i=goplus\r\n"
#define RTSP_SDP_JPEG_REPLY2	"t=0 0\r\nm=video 0 RTP/AVP 26\r\n"
#define RTSP_SDP_JPEG_REPLY3	"a=rtpmap:26 JPEG/90000\r\na=control:streamid=0\r\na=framerate:25\r\n"

#define RTSP_SETUP_TCP_REPLY2		"Transport: RTP/AVP/TCP;unicast;"
#define RTSP_SETUP_UDP_REPLY2		"Transport: RTP/AVP/UDP;unicast;"

#define RTSP_SDP_VID_AUD_REPLY1 "m=audio 1 RTP/AVP 8\r\n"
#define RTSP_SDP_VID_AUD_REPLY2 "a=rtpmap:0 PCMA/8000\r\n"
#define RTSP_SDP_VID_AUD_REPLY3 "a=control:streamid=1\r\n"

#define RTSP_SETUP_REPLY2		"Transport: RTP/AVP/UDP;unicast;"

#define RTSP_PLAY_REPLY1		"RTSP/1,0 200 OK\r\nCseq: "

#define RTSP_RX_BUF_LEN	(1600)
#define RTSP_TX_BUF_LEN	(1500)
#define RTSP_QUEUE_LEN	16

#if 0
#define RTSP_H264_TASK_PRIORITY	osPriorityNormal
#endif

/* keepalive timeout = TCP_KEEPIDLE_DEFAULT + TCP_KEEPINTVL_DEFAULT * TCP_KEEPCNT_DEFAULT, RFC.1122 */
#define RTSP_CLIENT_KEEPALIVE_IDLE	    3		/* 3000ms */
#define RTSP_CLIENT_KEEPALIVE_INTVL	    1		/* 1000ms */
#define RTSP_CLIENT_KEEPALIVE_CNT	    9		/* 9 time */

typedef struct RTSP_INSTANCE_CTL_BLK_S
{
	char local_ip[RTSP_IP_LEN];		/* Local IP address */
	char connect_ip[RTSP_IP_LEN];	/* Connection IP address */
	char rtsp_url[RTSP_URL_LEN];
	int cseqnum;
	#if C_APP_PLAYBACK_WITH_SOUND
	unsigned int session_id;
	int setup_id;
	#else
	INT32U session_id;
	#endif
	int server_rtp_port1;
	int server_rtp_port2;
	#if C_APP_PLAYBACK_WITH_SOUND
    int server_rtp_aud_port1;
	int server_rtp_aud_port2;
	#endif
	int client_rtp_port1;
	int client_rtp_port2;
	#if C_APP_PLAYBACK_WITH_SOUND
    int client_rtp_aud_port1;
	int client_rtp_aud_port2;
	#endif
} PACKED RTSP_INSTANCE_CTL_BLK_T;

typedef struct RTSP_INTERLEAVED_FRAME_S
{
	char dollar_sign;	/* $ */
	char channel;		/* channel number, even = data, odd = control */
	char length[2];
} PACKED RTSP_INTERLEAVED_FRAME_T;

typedef struct NAL_INFO_S
{
	INT32U start_addr;
	INT32U length;
	INT8U islast;
} PACKED NAL_INFO_T;

typedef enum
{
	RTSP_METHOD_START = 0,
	RTSP_DESCRIBE_METHOD,
	RTSP_OPTION_METHOD,
	RTSP_SETUP_METHOD,
	RTSP_PLAY_METHOD,
	RTSP_PAUSE_METHOD,
	RTSP_TEARDOWN_METHOD,
	RTSP_ERROR_METHOD
} RTSP_METHOD_E;

#endif	//__RTSP_H__

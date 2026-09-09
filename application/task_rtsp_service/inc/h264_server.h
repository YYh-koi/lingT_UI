/******************************************************
* h264_server.h
*
* Purpose: H264 server header file
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
#ifndef __H264_SERVER_H__
#define __H264_SERVER_H__

#define H264_TASK_PRIORITY	7

#define H264_SERVER_PORT	8080
#define TCP_RX_BUF_LEN	   (1500)
//#define TCP_TX_BUF_LEN	   (1500)
#define TCP_TX_BUF_LEN	   (32*1024)
#define TCP_QUEUE_LEN		32
#define TCP_TASK_STACKSIZE	2048

/* TCP server control block */
typedef struct TCP_SRV_SOCKET_CTL_S
{
	int tcp_server;
	int tcp_client;
	MSG_Q_ID 	 tcpsrv_tx_q;
	char* tcpsrv_rx_buf;
	char* tcpsrv_tx_buf;
	unsigned short	server_port;
} TCP_SRV_SOCKET_CTL_T;

/* GP RTSP H264 server */
void rtsp_h264_start_service(void);
void rtsp_h264_stop_service(void);
INT32S rtsp_h264_send(INT32U buf, INT32U size, INT32U ts, INT32U type, void (*callback_done) (INT32U stream_addr, INT32U stream_type));
INT32S rtsp_jpeg_send(INT32U buf, INT32U size, INT32U ts, INT16U width, INT16U height, void (*callback_done) (INT32U, INT32U));

#endif	//__H264_SERVER_H__

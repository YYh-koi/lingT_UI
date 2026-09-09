/******************************************************
 * socket_cmd_wifi.c
 *
 * Purpose: Accept commands via a TCP socket --- wifi part
 *
 * Author: Craig Yang
 *
 * Date: 2015/10/20
 *
 * Copyright Generalplus Corp. ALL RIGHTS RESERVED.
 *
 * Version :
 * History :
 *
 *******************************************************/
#include "application.h"
#include "lwip/tcp.h"
#include "lwip/sockets.h"
#include "lwip/api.h"
#include "socket_cmd_wifi.h"


#if 1
#define printf  DBG_PRINT
#else
#define printf(...)
#endif

// =============================================================================
// Define
// =============================================================================

// =============================================================================
// Static variables
// =============================================================================

// =============================================================================
// External variables
// =============================================================================

// =============================================================================
// Note: Create network connection and listen
// =============================================================================
INT32S socket_cmd_wifi_start_server(socket_cmd_ctl_t *ctl_blk)
{
	err_t lwip_err;
	INT32S ret = SC_RET_FAIL;

	printf("Enter %s port %d\r\n", __func__, SOCKET_CMD_PORT);

	/* Create a new tcp connection handle */
	ctl_blk->socket_cmd_server = netconn_new(NETCONN_TCP);
	if (ctl_blk->socket_cmd_server != NULL)
	{
		printf("socket_cmd_task: socket cmd server connection created(%x)\r\n", ctl_blk->socket_cmd_server);
	}
	else
	{
		printf("socket_cmd_task: socket cmd server connection creation fail\r\n");
		goto START_SERVER_EXIT;
	}

	/* Bind the tcp connection to specific port */
	lwip_err = netconn_bind(ctl_blk->socket_cmd_server, IP_ADDR_ANY, SOCKET_CMD_PORT);
	if (lwip_err == ERR_OK)
	{
		printf("socket_cmd_task: socket cmd server bind to port %d\r\n", SOCKET_CMD_PORT);
	}
	else
	{
		printf("socket_cmd_task: socket cmd server bind fail (err=%d)\r\n", lwip_err);
		goto START_SERVER_EXIT;
	}

	/* Listen for incoming connection */
	lwip_err = netconn_listen(ctl_blk->socket_cmd_server);
	if (lwip_err == ERR_OK)
	{
		ret = SC_RET_SUCCESS;
		printf("socket_cmd_task: socket cmd server listen ok\r\n");
	}
	else
	{
		printf("socket_cmd_task: socket cmd server listen fail (err=%d)\r\n", lwip_err);
		goto START_SERVER_EXIT;
	}

	//netconn_set_recvtimeout(ctl_blk->socket_cmd_server, 1000);         //  time out for accpet

START_SERVER_EXIT:
	if (ret != SC_RET_SUCCESS)
		socket_cmd_wifi_del_server_conn(ctl_blk);

	return ret;
}

// =============================================================================
// Note: Wait for a new connection
// =============================================================================
INT32S socket_cmd_wifi_wait_client_conn(socket_cmd_ctl_t *ctl_blk)
{
	err_t lwip_err = ERR_OK;
	INT32S ret;

	ctl_blk->socket_cmd_client = NULL;

	//lwip_err = netconn_accept(ctl_blk->socket_cmd_server, &ctl_blk->socket_cmd_client);
	#if (WIFI_VENDOR == 1)
    lwip_err = netconn_accept(ctl_blk->socket_cmd_server, &ctl_blk->socket_cmd_client);
    #elif (WIFI_VENDOR == 2)
    lwip_err = netconn_accept(ctl_blk->socket_cmd_server, &ctl_blk->socket_cmd_client);
    #endif
	if (ctl_blk->socket_cmd_server == 0 || (lwip_err != ERR_OK && lwip_err != ERR_TIMEOUT))
	{
		// If no client is connected to the server, netconn_accept will timeout.
		// This is not an error and we just need to keep waiting.
		if (lwip_err == ERR_TIMEOUT) {
			// josephhsieh netconn_err(ctl_blk->socket_cmd_server) = ERR_OK;
		}	
		else
			printf("socket cmd netconn_accept error (%d)\r\n", lwip_err);
	}
	else
	{
		//netconn_set_recvtimeout(ctl_blk->socket_cmd_client, 1000);	//  time out for client
	}

	#if ( (WIFI_VENDOR == 1)&&(GP_SOCK_TCP_BEHAVIOUR == 1) )
	netconn_set_recvtimeout(ctl_blk->socket_cmd_client, 1000);	//	blocking & non-blocking
	#endif

	if (ctl_blk->socket_cmd_server && lwip_err == ERR_OK)
	{
		#if 0
			Wifi_symbol_activate();
		#endif
		ret = SC_RET_SUCCESS;
	}
	else
	{
		ret = SC_RET_FAIL;
	}

	return ret;
}

// =============================================================================
// Note: Wait until new data from client is received. The received data will
//       be placed in 'buf'.
// =============================================================================
INT32S socket_cmd_wifi_recv_data(socket_cmd_ctl_t *ctl_blk, INT8U *buf, INT32U buf_size)
{
	struct netbuf *pxRxBuffer = NULL;
	INT32S ret = SC_RET_SUCCESS;
	INT16U len = 0;
	INT8S *rxstring;
	err_t err = ERR_OK;

	/* Wait until new data is received */
	//err = netconn_recv(ctl_blk->socket_cmd_client, &pxRxBuffer);
    #if (WIFI_VENDOR == 1)
    while ((err = netconn_recv(ctl_blk->socket_cmd_client, &pxRxBuffer)) == ERR_OK)
    #elif (WIFI_VENDOR == 2)
    while ((err = netconn_recv(ctl_blk->socket_cmd_client, &pxRxBuffer)) == ERR_OK)
    #endif
	if(err == ERR_OK)
	{
		/* Where is the data? */
		netbuf_data(pxRxBuffer, (void*)&rxstring, &len);
		if(len)
		{
			if(len > buf_size)
				len = buf_size;

			gp_memcpy((INT8S*)buf, rxstring, (INT32U)len);
			// printf("rxstring[%d]\r\n", len);

			netbuf_delete(pxRxBuffer);
		}
		else
		{
			ret = SC_RET_NO_DATA;
		}
	}
    else if (err == ERR_TIMEOUT)
    {
        ret = SC_RET_RECV_TIMEOUT;
    }
	else
	{
		printf("socket cmd netconn_recv err(%d)\r\n", err);
		ret = SC_RET_FAIL;
	}

	//netbuf_delete(pxRxBuffer);

	return ret;
}

// =============================================================================
// Note: Check if there is any new data to handle
// =============================================================================
extern err_t   netconn_recv_try(struct netconn *conn, struct netbuf **new_buf);
INT32S socket_cmd_wifi_check_new_data(socket_cmd_ctl_t *ctl_blk)
{
	INT32S ret;
	err_t err;
	struct netbuf *pxRxBuffer = NULL;

	// netconn_recv_try checks if there is any new data pending to process.
	// It's not a standard LWIP API but is provided by vendor so that we can
	// cancel massive data downloading.
	err = netconn_recv_try(ctl_blk->socket_cmd_client, &pxRxBuffer);
	ret = (err == ERR_OK) ? SC_RET_SUCCESS : SC_RET_FAIL;

	if(pxRxBuffer)
	{
		netbuf_delete(pxRxBuffer);
	}

	return ret;
}

// =============================================================================
// Note: It's a streaming request if STREAM_REQ_HEADER can be detected
// =============================================================================
INT32S socket_cmd_wifi_send_data(socket_cmd_ctl_t *ctl_blk, INT8U *tx_buf, INT16U len)
{
	INT32S ret = SC_RET_SUCCESS;
	err_t err;

	err = netconn_write(ctl_blk->socket_cmd_client, tx_buf, len, NETCONN_NOFLAG);
	if (err != ERR_OK)
	{
		ret = SC_RET_FAIL;
		printf("socket_cmd_wifi_send_data: netconn_write fail(err=%d)\r\n", err);
	}

	return ret;
}

// =============================================================================
// Note: Return the size of client's available tx buffer so that we know how much data to send
// =============================================================================
INT32U socket_cmd_wifi_get_client_avail_tx_buf_size(socket_cmd_ctl_t *ctl_blk)
{
	return ctl_blk->socket_cmd_client->pcb.tcp->snd_buf;
}

// =============================================================================
// Note: Close the client connection
// =============================================================================
void socket_cmd_wifi_close_client_conn(socket_cmd_ctl_t *ctl_blk)
{
	netconn_close(ctl_blk->socket_cmd_client);
}

// =============================================================================
// Note: Delete a client connection when the client is disconnected
// =============================================================================
INT32S socket_cmd_wifi_del_client_conn(socket_cmd_ctl_t *ctl_blk)
{
	err_t lwip_err;
	INT32S ret;

	lwip_err = netconn_delete(ctl_blk->socket_cmd_client);
	ret = (lwip_err == ERR_OK) ? SC_RET_SUCCESS : SC_RET_FAIL;
	#if 0
		Wifi_symbol_inactivate();
	#endif

	return ret;
}

// =============================================================================
// Note: Close the server connection
// =============================================================================
void socket_cmd_wifi_close_server_conn(socket_cmd_ctl_t *ctl_blk)
{
	// What is this for ??
	sys_mbox_trypost( &(ctl_blk->socket_cmd_server->acceptmbox), (void *)(NULL));
}

// =============================================================================
// Note: Delete a server connection
// =============================================================================
INT32S socket_cmd_wifi_del_server_conn(socket_cmd_ctl_t *ctl_blk)
{
	err_t lwip_err;
	INT32S ret;

	lwip_err = netconn_delete(ctl_blk->socket_cmd_server);
	ret = (lwip_err == ERR_OK) ? SC_RET_SUCCESS : SC_RET_FAIL;

	ctl_blk->socket_cmd_server = NULL;

	return ret;
}

#if ((WIFI_RUN_MODE == WIFI_SMARTCONFIG_MODE)||(WIFI_RUN_MODE == WIFI_STATION_MODE))
INT8U sta_ip[4];
INT8U sta_broadcast_ip_flag=0;
void fill_broadcast_buffer(char* buffer)
{
    *buffer = 'G';
    *(buffer+1) = 'P';
    *(buffer+2) = 'L';
    *(buffer+3) = 'U';
    *(buffer+4) = 'S';
    *(buffer+5) = sta_ip[0];
    *(buffer+6) = sta_ip[1];
    *(buffer+7) = sta_ip[2];
    *(buffer+8) = sta_ip[3];

    print_string("IP  => %d.%d.%d.%d\r\n", *(buffer+5), *(buffer+6), *(buffer+7), *(buffer+8));
}

void socket_broadcast_ip(void)
{
    int sock,status, broadcast = 1;//,retry = 5;
    char buffer[15] = {0};
    struct sockaddr_in  dest;

    //initialize value in dest
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_len         = sizeof(dest);
    dest.sin_port = htons(8080);
    dest.sin_addr.s_addr = 0xffffffffUL;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock < 0)
    {
        print_string("US socket failed\r\n");
    }
    status = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *) &broadcast, sizeof(broadcast));
    if(status < 0)
    {
        print_string("US SO_BROADCAST error %d\r\n", status);
    }
    fill_broadcast_buffer(buffer);
    /*int bytesSent = */sendto(sock, (char*)buffer, 9, 0,(struct sockaddr*)&dest, sizeof dest);
    //print_string("sock %x, status %d, bytesSent %d\r\n",sock,status,bytesSent);
    if(sock >= 0)
        closesocket(sock);
}

void socket_connection_timer_start(void)
{
	sta_broadcast_ip_flag = 1;
}

void socket_connection_timer_stop(void)
{
	sta_broadcast_ip_flag = 0;
}

INT8U socket_connection_timer_get(void)
{
    return sta_broadcast_ip_flag;
}
#endif

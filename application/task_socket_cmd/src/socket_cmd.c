#include "application.h"
#include "lwip/sockets.h"
#include "lwip/api.h"
#include "lwip/tcp.h"
#include "wifi_menu.h"


#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/
extern INT32S peripheral_task_msg_send(INT32U msg, void *para, INT32U bytes, INT32U prio);
extern void ap_peripheral_ok_key_exe(INT16U *tick_cnt_ptr);
extern int Playback_Type_Get(INT32U req_msg);
extern int rtsp_connection_get(void);
extern void rtsp_method_type_set(int val);
extern INT32S wifi_transmission_stop(void);
/**************************************************************************
*                              M A C R O S                               *
**************************************************************************/
//#define GPTCPCMD_THREAD_PRIO  ( tskIDLE_PRIORITY + 8 )


/* network control*/
#define TCP_RX_BUF_MAX_LEN                  2048
#define TCP_TX_BUF_MAX_LEN                  2048

#define PLAYFILE_INDEX_TABLE_SIZE          (20000/512+1)         // Each 512 files creates a record
#define CLIENT_BUF_SIZE                     1460

#define CAPTURE_PERIOD                      150

#define THUMBNAIL_WIDTH						320
#define THUMBNAIL_HEIGHT					240

/**************************************************************************
*                          D A T A    T Y P E S                          *
**************************************************************************/


/**************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S               *
**************************************************************************/
INT32S gp_resp_set(INT32U type, INT16U payload_attr, INT8U *payload_data, INT32U payload_len, INT8U *tx_buf);
static void gp_cmd_process(void *void_ctl_blk, INT8U *rx_buf, INT16U rxlen, INT8U *tx_buf, void *len);

extern INT32S Rtsp_Flow_Cdsp_Capture_Start();
extern INT32S Rtsp_Flow_Cdsp_Capture_Stop();
extern void pscaler_rectangle_set(INT8U enable);
//extern void Send_Dron_Ctrl_Uart_Data( GOPLUS_ALOG_CTRL_DATA_T *Ctrl_ptr );
//extern void Send_Ctrl_I2C_Data( GOPLUS_ALOG_CTRL_DATA_T *Ctrl_ptr, int init_reset );
extern void setupSocketFlag( INT8U );
extern signed int* GetTrackingFlag( void );
extern void resetROI( void );
extern void terminateTracking( void );
extern void setTimer( INT8U flag );
extern void udp_socket_send( unsigned char *data,int len);
extern void SetOperationMode( unsigned int input );
extern unsigned int GetOperationMode();
extern void SetSystemFlow( unsigned int input );
extern unsigned int GetSystemFlow();
extern void ButtonCancel();
extern INT32S prcess_state_get(void);
extern INT8U GetFaceFlyCountEnable();
extern INT8U SetFaceFlyCountEnable( INT8U input );
//extern void udpsend_Init_JPG(RTSP_SELFIE_TASK_STRUCT *pvParameters);
extern void RX_Jpg_ACK_Report_set();
extern void set_JPG_First_in(int Val);
extern void update_GI_STATUS(uint8_t type,uint8_t value);
//extern INT8U FaceDetectToFly(void);
/**************************************************************************
*                         G L O B A L    D A T A                         *
**************************************************************************/
extern INT8S video_record_sts;                                    // status (record mode)
INT32U g_wifi_notify_pic_done = 0;
INT32U g_wifi_sd_full_flag = 0;
static int wifi_mode_bak = RECORD_MODE;
static int wifi_mode_last = RECORD_MODE;
static int wifi_mode_new = RECORD_MODE;
static INT8U wifi_in_menu;
static INT16U playfile_index_table[PLAYFILE_INDEX_TABLE_SIZE];    // Each 512 files creates a record.
static unsigned char tx_buf[TCP_TX_BUF_MAX_LEN] = {0};
static unsigned char rx_buf[TCP_RX_BUF_MAX_LEN] = {0};
INT16U browse_play_idx;
INT32U socket_cmd_status = 0;

//firmware_upgrade
static INT32U firmware_download_size = 0;                         // frimware total size
static INT32U firmware_download_checksum = 0;
static INT32U firmware_upgrade_flag = 0;
static INT8U *pfirmware = NULL;
static INT8U *pfirmware_cur = NULL;
static INT32U firmware_cur_size = 0;
static INT8U *prx_firmware_buf;

static socket_api_ctl_t socket_socket_ctl_blk =
{
	-1,				/* Server connection */
	-1,				/* Client connection */
};
volatile INT32U thumbnail_decode_busy = 0;
void GPSOCK_Playback_GetThumbnail_BusySet(int flag)
{
	thumbnail_decode_busy = flag;
}

int GPSOCK_Playback_GetThumbnail_BusyGet(void)
{
	return thumbnail_decode_busy;
}

void gp_clear_modebak(void)
{
	wifi_mode_bak = -1;
}

static int gp_mode_map(wifi_state)
{
	int local_state;

	switch (wifi_state)
	{
		case RECORD_MODE:
			local_state = STATE_VIDEO_RECORD;
			break;
		case CAPTURE_MODE:
			local_state = STATE_PHOTO_CAPTURE;
			break;
		case BROWSE_MODE:
			local_state = STATE_BROWSE;
			break;
		default:
			local_state = STATE_SETTING;
			break;
	}
	// DBG_PRINT("present_state = 0x%x, wifi_state = 0x%x, wifi_mode_bak = 0x%x\r\n",present_state, wifi_state, wifi_mode_bak);
	return local_state;
}

static INT32U wifi_ref_state = 0;
int wifi_reference_state_set(int state)
{
	int ret = 0;
	wifi_ref_state = state;
	return ret;
}
// 手機和小機模式定義不同
int gp_mode_remap(void)
{
	return wifi_ref_state;
}

// mode：手機端下的模式
static int gp_mode_set(int wifi_mode)
{
	INT32S ret = 0;
	//INT32S local_mode_cur;
	INT32U mode_set;
	//INT16U cnt = 0;
	//INT8U  data;
	volatile INT16U wait_max = 500;

	if(wifi_mode_bak == wifi_mode)
	{
		goto GP_MODE_SET_END;
	}
	wifi_mode_last = wifi_mode_new;
	wifi_mode_new = wifi_mode;
	if(FALSE_MENU_MODE == wifi_mode)
	{
		wifi_in_menu = 1;
	}
	else
	{
		if(wifi_in_menu)
		{
			wifi_in_menu = 0;
			ap_state_config_store();
		}

		mode_set = gp_mode_map(wifi_mode);
		app_msg_send(MSG_APQ_WIFI_MODE_CHANGE, &mode_set, sizeof(mode_set), MSG_PRI_NORMAL);

		while(--wait_max && gp_mode_remap() != wifi_mode)
		{
			vTaskDelay(10);
		}
	}


	wifi_mode_bak = wifi_mode;

GP_MODE_SET_END:
	return ret;
}
static int gp_mode_get(void)
{
	return wifi_mode_bak;
}
INT32U wifi_transmission_stop_depen_on_mode(void)
{
	DBG_PRINT("[AI]%s:wifi_mode_last = %d, wifi_mode_new = %d\r\n", __func__,wifi_mode_last,wifi_mode_new);
	if(((wifi_mode_last == RECORD_MODE)&&(wifi_mode_new == BROWSE_MODE))// record mode to browse mode
		||((wifi_mode_last == BROWSE_MODE)&&(wifi_mode_new == RECORD_MODE))// browse mode to record mode
		||((wifi_mode_last == CAPTURE_MODE)&&(wifi_mode_new == BROWSE_MODE))// capture mode to browse mode
		||((wifi_mode_last == BROWSE_MODE)&&(wifi_mode_new == CAPTURE_MODE))// mode browse mode to capture mode
		//||((wifi_mode_last == RECORD_MODE)&&(wifi_mode_new == RECORD_MODE))
		)//first change mode record mode to browse mode
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
static INT32S GPSOCK_General_SetMode_Decode(INT8U* rxstring)
{
	INT32S ret = NAK_OK;
	INT32S mode = 0;

	// Byte 0 of SetMode's payload is 'mode'
	mode = (INT32S)rxstring[GP_SOCK_CMD_PAYLOAD_OFFSET];
	gp_mode_set(mode);

	return ret;
}

static int GPSOCK_Record_Audio_Decode(unsigned short gp_sock_cmd, socket_cmd_ctl_t *ctl_blk, INT8U* rxstring)
{
	INT32S ret = NAK_OK;
	INT8U flag = 0;
	if (present_state != STATE_VIDEO_RECORD)
	{
		ret = NAK_Mode_error;
		goto GPSOCK_RECORD_AUDIO_DECODE_END;
	}

	flag = ap_state_config_voice_record_switch_get();
	if(flag)
	{
		ap_state_config_voice_record_switch_set(0);
	}
	else
	{
		ap_state_config_voice_record_switch_set(1);
	}

	if(ap_state_config_voice_record_switch_get() != 0)
	{
		ap_state_handling_icon_clear_cmd(ICON_RECORD_MIC_OFF, 0, 0, 0);
	}
	else
	{
		ap_state_handling_icon_show_cmd(ICON_RECORD_MIC_OFF, 0, 0, 0);
	}

GPSOCK_RECORD_AUDIO_DECODE_END:
	return ret;
}

INT32S GPSOCK_CapturePicture_Capture_Decode(unsigned short gp_sock_cmd, socket_cmd_ctl_t *ctl_blk, INT8U* rxstring)
{
	INT32S ret = STATUS_OK;
	INT32S wifi_mode;
	INT32U i = 0;
	INT16U cnt = 0;

	wifi_mode = gp_mode_remap();
	if (wifi_mode!=CAPTURE_MODE)
	{
		ret = NAK_Mode_error;
		goto CAPTUREPICTURE_CAPTURE_DECODE_END;
	}

	if (ap_state_handling_storage_id_get() != STORAGE_DEVICE)
	{
		ret = NAK_No_storage;
		goto CAPTUREPICTURE_CAPTURE_DECODE_END;
	}

	if  (ap_state_config_capture_burst_get())
	{
		if (g_wifi_notify_pic_done==0)
		{	// sequence
			ap_peripheral_ok_key_exe(&cnt);
			vTaskDelay(1000);
		}
		else
		{
			DBG_PRINT("sequence is busy, skip cmd\r\n");
		}
	}
	else
	{
		g_wifi_notify_pic_done = 0;	// trigger
		g_wifi_sd_full_flag = 0;
		ap_peripheral_ok_key_exe(&cnt);
		DBG_PRINT("Capture_Start\r\n");

		for (i = 0; i < CAPTURE_PERIOD; ++i)
		{
			if  ( g_wifi_notify_pic_done )
			{
				DBG_PRINT("Capture Done !!\r\n");
				g_wifi_notify_pic_done = 0;
				break;
			}
			vTaskDelay(20);		// one picture per 3 seconds
		}
		if (g_wifi_sd_full_flag==1)
		{
			ret = NAK_Full_storage;
			g_wifi_sd_full_flag = 0;
			DBG_PRINT("[Capture] SD Card Full\r\n");
		}
	}

	if ( (i == CAPTURE_PERIOD)&&(ret==STATUS_OK) )
	{
		DBG_PRINT("Capture Time Out\r\n");
		g_wifi_notify_pic_done = 0;
		ret = NAK_Request_timeout;
	}

CAPTUREPICTURE_CAPTURE_DECODE_END:
	return ret;
}

#define PLAYBACK_START_TIME_OUT 2000
static int GPSOCK_Playback_Start_Decode(unsigned short gp_sock_cmd, socket_cmd_ctl_t *ctl_blk, INT8U* rxstring)
{
	//INT32U i;
	INT32S ret = NAK_OK;
	INT32S wifi_mode;
	//INT16U cnt = 0;


	wifi_mode = gp_mode_remap();
	if (wifi_mode!=BROWSE_MODE)
	{
		ret = NAK_Mode_error;
		goto GPSOCK_PLAYBACK_START_DECODE_END;
	}

	if (ap_browse_sts_get() & BROWSE_SD_NOT_EXIST)
	{
		ret = NAK_No_storage;
	}

	browse_play_idx = rxstring[13]<<8 | rxstring[12];
	Playback_Type_Get(browse_play_idx);

	do
	{
		if (rtsp_connection_get())
		{
			break;
		}
		osDelay(1);
	}while(1);

	if(app_msg_send(MSG_APQ_BROWSE_PLAY_FILE, &browse_play_idx, sizeof(browse_play_idx), MSG_PRI_NORMAL) < 0)
	{
		ret = NAK_Request_timeout;
		goto GPSOCK_PLAYBACK_START_DECODE_END;
	}

	return ret;

GPSOCK_PLAYBACK_START_DECODE_END:
	rtsp_method_type_set(0);		// reset
	DBG_PRINT("Playback_Start ..... (%d)\r\n",ret);

	return ret;
}
extern int rtsp_h264_service_get(void);
INT32S GPSOCK_Playback_Pause_Decode(unsigned short gp_sock_cmd, socket_cmd_ctl_t *ctl_blk, INT8U* rxstring)
{
	INT32S ret = STATUS_OK;
	INT16U cnt = 0;

	if ( rtsp_h264_service_get()==1 )
	{
		DBG_PRINT("Skip puase/resume because of no connection\r\n");
		return ret;
	}

	if (rxstring[12] == 'J')
	{
		// JPEG AE
		ap_peripheral_ok_key_exe(&cnt);
		DBG_PRINT("GPSOCK_JPG_Pause_Resume\r\n");
	}
	else
	{
		// AVI AE
		app_msg_send(MSG_APQ_MJPEG_DECODE_PAUSE, NULL, 0, MSG_PRI_NORMAL);
		DBG_PRINT("GPSOCK_AVI_Pause_Resume\r\n");
	}

	return ret;
}

INT32S General_GetInformation_Decode(unsigned char *status)
{
	DBG_PRINT("General_GetInformation_Decode\r\n");
	status[0] = (curr_storage_id != STORAGE_DEVICE)?0x00:0x01;
	status[1] = 0x00 ; //no flight Control
	status[2] = 0x01 ; //H264

	return 0;
}

static int socket_sd_file_num = 0;
int GPSOCK_SD_Total_File_Get(void)
{
	return socket_sd_file_num;
}
INT32S GPSOCK_Playback_GetFileCount_Decode(unsigned short gp_sock_cmd, INT8U *tx_buf) //need to check
{
	INT32S ret = NAK_OK;
	INT32S i;
	INT8U packet_const[12] = {0x47,0x50,0x53,0x4F,0x43,0x4B,0x45,0x54,0x02,0x00,0x03,0x02};

	gp_memcpy((INT8S *)tx_buf,(INT8S*)packet_const,12);

	// Initial file index talbe
	for (i=0;i<PLAYFILE_INDEX_TABLE_SIZE;++i)
	{
		playfile_index_table[i] = 0xFFFF;
	}
	playfile_index_table[0] = 0xFFFE;	// 1~512 -O file ￠DN index = 0 ?}cl!AA?T
	socket_sd_file_num = 0;

	if (ap_browse_sts_get() == BROWSE_SD_NOT_EXIST)
	{
		DBG_PRINT(" No_storage\r\n");
		return NAK_No_storage;
	}

	for (i = 0; i < PLAYBACK_START_TIME_OUT; ++i)
	{
		if (ap_browse_sts_get() == BROWSE_IDLE)
			break;
		osDelay(1);
	}

	ret = ap_storage_service_playfile_totalcount(playfile_index_table);
	socket_sd_file_num = ret;
	DBG_PRINT("total  %d  files\r\n",ret);

	if (ret == 0)
	{
		ret = NAK_No_storage;
	}

	return ret;
}

INT32S GPSOCK_Playback_GetFileList_Decode(void *void_ctl_blk, INT8U* rxstring,INT8U *tx_buf)
{
	INT32S size = 0;
	//INT32S ret = STATUS_OK;
	INT32U dcim_a_b = 0;
	INT16U play_file_index;
	INT8U packet_const[12] = {0x47,0x50,0x53,0x4F,0x43,0x4B,0x45,0x54,0x02,0x00,0x03,0x03};
	INT32U i;

	gp_memcpy((INT8S*)tx_buf,(INT8S*)packet_const,12);
	// rxstring[12]=0: Get list by file index
	// rxstring[12]=1: Get list from 1st file
	if(*(rxstring+12) == 1)
	{
		play_file_index = 0xffff;
	}
	else
	{
		play_file_index = *(rxstring+14);
		play_file_index = play_file_index<<8;
		play_file_index |= *(rxstring+13);
		if(play_file_index >= 19999)
		{
			//ret = NAK_Invalid_command;
			goto EXIT_GET_FILE_LIST;
		}
		if (play_file_index <= 9999)
		{
			dcim_a_b = 0;	// DCIMA
			if(play_file_index == 9999)
			{
                play_file_index = 0xffff;
                dcim_a_b = 1;
			}
		}
		else
		{
			play_file_index -= (9999+1);
			dcim_a_b = 1;	// DCIMB
		}
	}

	for (i = 0; i < PLAYBACK_START_TIME_OUT; ++i)
	{
		if (ap_browse_sts_get() == BROWSE_IDLE)
			break;
		osDelay(1);
	}

	size = (INT32S)ap_storage_service_playfilelist_req(tx_buf+12,play_file_index,dcim_a_b);
	if ( (size == 1)&&(dcim_a_b==0) )
	{	// search DCIMA ended but not find any file. Start search DCIMB
		play_file_index = 0xffff;
		dcim_a_b = 1;
		size = (INT32S)ap_storage_service_playfilelist_req(tx_buf+12,play_file_index, dcim_a_b);
	}

	if (size != -1)
	{
		size = (size + GP_SOCK_RESP_PAYLOAD_OFFSET);	// GP_SOCK_Tag(8) + GP_SOCKType(2) + GP_SOCK_MODE_ID_(1) +GP_SOCK_CMD_ID(1) + paylad size filed
	}
	else
	{
		//ret = NAK_Get_file_list_fail;
		DBG_PRINT("GetFileList Error!!\r\n");
	}

EXIT_GET_FILE_LIST:
	return size;
}

INT32S GPSOCK_Playback_GetThumbnail_Decode(unsigned short gp_sock_cmd, socket_api_ctl_t* ctl_blk, INT8U* rxstring)
{
	const INT8U packet_const[12] = {0x47,0x50,0x53,0x4F,0x43,0x4B,0x45,0x54,0x02,0x00,0x03,0x04};
	INT32S ret = STATUS_OK;//, sc_ret = STATUS_OK;
	INT32U size;
	INT32U wifi_mode;
	INT8U *buf = 0;
	INT16U play_file_index = 0;
	INT16U *p;
	INT8U *p_sock;

	if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
	{
		ret = NAK_Server_is_busy;
		goto GPSOCK_PLAYBACK_GETTHUMBNAIL_DECODE_END;
	}
	wifi_mode = gp_mode_remap();
	if (wifi_mode!=BROWSE_MODE)
	{
		ret = NAK_Server_is_busy;
		goto GPSOCK_PLAYBACK_GETTHUMBNAIL_DECODE_END;
	}

	play_file_index = *(rxstring+13);
	play_file_index = play_file_index<<8;
	play_file_index |= *(rxstring+12);

    if ( rtsp_h264_service_get()==0 )
	{
		DBG_PRINT("Thnail[%d]:RTSP is active\r\n",play_file_index);
		ret = NAK_Server_is_busy;
		goto GPSOCK_PLAYBACK_GETTHUMBNAIL_DECODE_END;
	}
    GPSOCK_Playback_GetThumbnail_BusySet(1);

	buf = (INT8U*)ap_storage_service_thumbnail_req(play_file_index, &size, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT);
	if (buf == NULL)
	{
		// maybe remove SD card
		ret = NAK_Invalid_command;
		goto GPSOCK_PLAYBACK_GETTHUMBNAIL_DECODE_END;
	}

	p_sock = (INT8U*)(buf+64-GP_SOCK_RESP_PAYLOAD_OFFSET);
	gp_memcpy((INT8S*)p_sock, (INT8S*)packet_const, 12);
	p = (INT16U*)(buf+64-2);
	*p = (INT16U)size;

	ret = send(ctl_blk->socket_cmd_client, (char const*)p_sock, size+GP_SOCK_RESP_PAYLOAD_OFFSET, 0); //not sure

	//if(ret)
		ret = STATUS_OK;
	//else
		//STATUS_FAIL;
	if(ret != STATUS_OK)
	{
		ret = NAK_Get_thumbnail_fail;
		DBG_PRINT("thumbnail get error\r\n");
	}
	if (buf)
	{
		gp_free((void*)buf);
	}

GPSOCK_PLAYBACK_GETTHUMBNAIL_DECODE_END:
	DBG_PRINT("Thnail:%d(%d)\r\n",play_file_index,ret);
	GPSOCK_Playback_GetThumbnail_BusySet(0);
	return ret;
}
#if C_APP_DOUBLE_BUFFER_DOWNLOAD

#define DOWNLOAD_RETRY 1000
#define DOWNLOAD_PACKET_SIZE (60*1024)
#define DOWNLOAD_SD_BLOCK_SIZE (35*DOWNLOAD_PACKET_SIZE)

static INT32U netwr_t;
INT32U wifi_dl_cur_size=0;
INT32U curr_wifi=0,curr_buf=0;
INT32U buf_dl_flag[2]={0};
INT32U buf_dl_size[2]={0};
INT32U wifi_dl_state=WIFI_DOWNLOAD_IDLE;
INT32U wifi_dl_sd_get_size=0;
INT8U *wifi_dl_buf_A=NULL;
INT8U *wifi_dl_buf_B=NULL;
INT16U wifi_dl_play_file_index=0;

STOR_SERV_DOWNLOAD_FILEINFO download_file_info;

INT8U wifi_download_task_flag = 0;

static void
wifi_file_download_state_task_entry(
	void const *arg
)
{
	INT32U wifi_dl_cnt = 0;
	INT32S ret;
	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);
    wifi_download_task_flag = 1;
	while(1)
	{
        if (wifi_dl_state==WIFI_DOWNLOAD_ACTIVE)
		{
            //DBG_PRINT("S[%d,%d,%d,%d,%d]",wifi_dl_cur_size,download_file_info.file_size,curr_buf,buf_dl_flag[0],buf_dl_flag[1]);
			if ((download_file_info.file_size > wifi_dl_cur_size)||(wifi_dl_cnt==0))  // wifi_dl_cnt=0 first time
			{
				wifi_dl_cnt++;
				if ( (curr_buf==1)&&(buf_dl_flag[1]==0) )
				{
					// Download
					//DBG_PRINT("b ");
					ret = ap_storage_service_download_file_req(wifi_dl_play_file_index,&download_file_info,wifi_dl_buf_B+64,&(buf_dl_size[1])); // +64(memory align) for speed up
					if (ret!=0)
					{
						buf_dl_size[1] = 0xFFFFFFFF;
						DBG_PRINT("download buffer B fail\r\n");
					}
					//else { DBG_PRINT("b_ "); }
					// Switch index
					buf_dl_flag[1] = 1;
					curr_buf = 0;
				}
				else if ( (curr_buf==0)&&(buf_dl_flag[0]==0) )
				{
					// Download
					//DBG_PRINT("a ");
					ret = ap_storage_service_download_file_req(wifi_dl_play_file_index,&download_file_info,wifi_dl_buf_A+64,&(buf_dl_size[0])); // +64(memory align) for speed up
					if (ret!=0)
					{
						buf_dl_size[0] = 0xFFFFFFFF;
						DBG_PRINT("download buffer A fail\r\n");
					}
					//else { DBG_PRINT("a_ "); }
					// Switch index
					buf_dl_flag[0] = 1;
					curr_buf = 1;
				}
				else
				{
					DBG_PRINT("L%d%d%d ",curr_buf,buf_dl_flag[0],buf_dl_flag[1]);
				}
			}
			else
			{
				wifi_dl_cnt = 0;
				buf_dl_flag[curr_buf] = 0xFF;
				if(download_file_info.file_handle>=0)
				{
                    DBG_PRINT("close file\r\n");
                    close(download_file_info.file_handle);
                    download_file_info.file_handle = -1;
                }
				DBG_PRINT("F\r\n");
			}
		}
		else if(wifi_dl_state == WIFI_DOWNLOAD_FINISH)
		{
			wifi_dl_cnt = 0;
			wifi_dl_state = WIFI_DOWNLOAD_IDLE;
			DBG_PRINT("wifi download finish,exit!!!\r\n");
			break;
		}
		else
		{
            wifi_dl_cnt = 0;
		}
		osDelay(200);
    }
    DBG_PRINT("kill download task!!!\r\n");
    wifi_download_task_flag = 0;
    vTaskDelete(NULL);
    while(1)
    {
        osDelay(10);
        DBG_PRINT("wait task delete\r\n");
    }
}


INT32S
wifi_file_download_task_create(
	void
)
{
	INT32S nRet = STATUS_OK;
	osThreadId id;
	osThreadDef_t wifi_download_task = {"wifi_file_download_task", wifi_file_download_state_task_entry, PRIORITY_WIFI_DOWNLOAD_TASK, 1, WIFI_DOWNLOAD_STATE_STACK_SIZE};

	id = osThreadCreate(&wifi_download_task, NULL);
	if(id == 0) {
		RETURN(STATUS_FAIL);
	}

	nRet = STATUS_OK;

Return:
	return nRet;
}

static INT32S Playback_GetRawData(void *void_ctl_blk, INT8U *buf, INT32U sd_get_size)
{
	INT32U sd_get_idx = 0;
	INT8U *pPacketbuf;
	INT8U *pBuf;	// download current buffer
	INT16U *pPacksize;		// packet size
	//INT16U available;
	INT32S ret;
	INT32S err = NAK_OK;
	INT8U packet_const[12] = {0x47,0x50,0x53,0x4F,0x43,0x4B,0x45,0x54,0x02,0x00,0x03,0x05};
	socket_api_ctl_t *ctl_blk = (socket_api_ctl_t *)void_ctl_blk;
    //INT32U retry_cnt = DOWNLOAD_RETRY;
    INT32U wlen, tlen, size;
	sd_get_idx = 0;
	// each DOWNLOAD_PACKET_SIZE
	while (sd_get_size != 0)
	{


		if(sd_get_idx==0)
		{
			pPacketbuf = (INT8U*)(buf + 64-GP_SOCK_RESP_PAYLOAD_OFFSET);
			sd_get_idx++;
			pBuf = (INT8U*)pPacketbuf;
			gp_memcpy((INT8S*)pBuf,(INT8S*)packet_const,sizeof(packet_const) );
			if (sd_get_size > (DOWNLOAD_PACKET_SIZE-64+GP_SOCK_RESP_PAYLOAD_OFFSET))
			{
				size = DOWNLOAD_PACKET_SIZE-64+GP_SOCK_RESP_PAYLOAD_OFFSET;
				sd_get_size -= size;
			}
			else
			{
				size = sd_get_size;
				sd_get_size = 0;
			}
			pPacksize = (INT16U*)(pBuf+GP_SOCK_RESP_PAYLOAD_SIZE_OFFSET);
			*pPacksize = size;
			tlen = size+GP_SOCK_RESP_PAYLOAD_OFFSET;
		}
		else
		{
			pPacketbuf = (INT8U*)(buf + sd_get_idx*DOWNLOAD_PACKET_SIZE);
			sd_get_idx++;
			pBuf = (INT8U*)pPacketbuf;
			gp_memcpy((INT8S*)pBuf,(INT8S*)packet_const,sizeof(packet_const) );
			if (sd_get_size > DOWNLOAD_PACKET_SIZE)
			{
				size = DOWNLOAD_PACKET_SIZE;
				sd_get_size -= DOWNLOAD_PACKET_SIZE;
			}
			else
			{
				size = sd_get_size;
				sd_get_size = 0;
			}
			pPacksize = (INT16U*)(pBuf+GP_SOCK_RESP_PAYLOAD_SIZE_OFFSET);
			*pPacksize = size;
			tlen = size+GP_SOCK_RESP_PAYLOAD_OFFSET;
		}
		// Wi-Fi driver divide into few packets
		while(tlen)
		{
			//available = socket_cmd_wifi_get_client_avail_tx_buf_size(ctl_blk);
			//if (available)
			{
				//if(tlen > available)
				//	wlen = available;
				//else
				wlen = tlen;

				// printf("available=%d,wlen=%d,tlen=%d,pBuf=%p\r\n",available,wlen,tlen,pBuf);
				//str_t = xTaskGetTickCount();
				ret = send(ctl_blk->socket_cmd_client, pBuf, wlen, 0);
				//cur_t = xTaskGetTickCount();
				//netwr_t += (cur_t-str_t);
				if(ret < 0)
				{
					err = NAK_Write_Fail;
					printf("raw data get error: send data fail(wlen=%d)\r\n", wlen);
					goto Playback_GetRawData_END;
				}

				tlen -= wlen;
				pBuf += wlen;
				//retry_cnt = DOWNLOAD_RETRY;
			}
			#if 0
			else
			{
				OSTimeDly(1);
				retry_cnt--;
				if(!retry_cnt)
				{
					printf("wifi send data timeout,%d\r\n",retry_cnt);
					err = NAK_Request_timeout;
					goto Playback_GetRawData_END;
				}
			}
			#endif
		}
		wifi_dl_cur_size += size;
		// printf("wifi send data OK,%d\r\n",retry_cnt);
	}

Playback_GetRawData_END:
	return err;
}


static int GPSOCK_Playback_GetRawData_Decode(unsigned short gp_sock_cmd, void *void_ctl_blk, INT8U* rxstring)
{
	//INT32S ret;
	INT32S err = NAK_OK;
	INT32U cmd_st=0, cmd_et;
	socket_api_ctl_t *ctl_blk = (socket_api_ctl_t *)void_ctl_blk;

	#if GP_SOCK_TCP_BEHAVIOUR
	{
		int enable = 1;		// timeout ticks
		if(setsockopt(ctl_blk->socket_cmd_client, SOL_SOCKET, SO_RCVTIMEO, &enable, sizeof(enable)) < 0 )
		{
			DBG_PRINT("SO_RCVTIMEO error!\r\n");
		}
	}
	#endif

	// mm_dump();
	wifi_dl_cur_size = 0;
	wifi_dl_buf_A = NULL;
	wifi_dl_buf_B = NULL;
	wifi_dl_buf_A = (INT8U*)gp_malloc_align(DOWNLOAD_SD_BLOCK_SIZE+64, 8);
	if (wifi_dl_buf_A == NULL)
	{
		printf("GPSOCK_Playback_GetRawData memory A alloc fail\r\n");
		err = NAK_Invalid_command;
		goto GPSOCK_PLAYBACK_GETRAWDATA_DECODE_END;
	}
	wifi_dl_buf_B = (INT8U*)gp_malloc_align(DOWNLOAD_SD_BLOCK_SIZE+64, 8);
	if (wifi_dl_buf_B == NULL)
	{
		printf("GPSOCK_Playback_GetRawData memory B alloc fail\r\n");
		err = NAK_Invalid_command;
		goto GPSOCK_PLAYBACK_GETRAWDATA_DECODE_END;
	}

	gp_memset((INT8S*)(&download_file_info), 0, sizeof(download_file_info) );
    download_file_info.file_handle = -1;
	ap_storage_service_download_file_block_size(DOWNLOAD_SD_BLOCK_SIZE);
	wifi_dl_play_file_index = *(rxstring+13);
	wifi_dl_play_file_index = wifi_dl_play_file_index<<8;
	wifi_dl_play_file_index |= *(rxstring+12);		// file index
	curr_wifi = 0; curr_buf = 1;		// A-B buffer current index
	buf_dl_flag[0] = buf_dl_flag[1] = 0;
	netwr_t = 0;
	wifi_dl_state = WIFI_DOWNLOAD_ACTIVE;

	DBG_PRINT("start download\r\n");
	wifi_file_download_task_create();
	cmd_st = xTaskGetTickCount();
	while (1)
	{
		// New data detected (i.e. cancel download) during download.
		// Leave the while loop to handle it.
		int rxlen = recv(ctl_blk->socket_cmd_client, (void *)rx_buf, TCP_RX_BUF_MAX_LEN, 0);
		if(rxlen > 0)
		{
			DBG_PRINT("Cancel download\r\n");
			err = NAK_Invalid_command;
			break;
		}

		if ( (curr_wifi==0)&&(buf_dl_flag[1]==1) )
		{
			// Wi-Fi will be transmitted through B buffer
			//DBG_PRINT("B_");
			err = Playback_GetRawData(void_ctl_blk, wifi_dl_buf_B, buf_dl_size[1]);
			//DBG_PRINT("B ");
			if (err!=NAK_OK)
			{
				//DBG_PRINT("B buffer send error\r\n");
				err = NAK_Write_Fail;
				goto GPSOCK_PLAYBACK_GETRAWDATA_DECODE_END;
			}
			///////////
			curr_wifi = 1;
			buf_dl_flag[1] = 0;
		}
		else if ( (curr_wifi==1)&&(buf_dl_flag[0]==1) )
		{
			// Wi-Fi will be transmitted through A buffer
			//DBG_PRINT("A_");
			err = Playback_GetRawData(void_ctl_blk, wifi_dl_buf_A, buf_dl_size[0]);
			//DBG_PRINT("A ");
			if (err!=NAK_OK)
			{
				err = NAK_Write_Fail;
				//DBG_PRINT("A buffer send error\r\n");
				goto GPSOCK_PLAYBACK_GETRAWDATA_DECODE_END;
			}
			///////////
			curr_wifi = 0;
			buf_dl_flag[0] = 0;
		}
		else
		{
			//DBG_PRINT("Load[%d,%d,%d]\r\n",curr_wifi,buf_dl_flag[0],buf_dl_flag[1]);
			vTaskDelay(1);	// loading data
		}

		if ( (curr_wifi==0)&&(buf_dl_flag[1]==0xFF) ) {
			DBG_PRINT("B buffer end\r\n");
			break;
		}

		if ( (curr_wifi==1)&&(buf_dl_flag[0]==0xFF) ) {
			DBG_PRINT("A buffer end\r\n");
			break;
		}

		if ( (buf_dl_flag[0]==0xFFFFFFFF)||(buf_dl_flag[1]==0xFFFFFFFF) )
		{
			printf("SD file load error!!\r\n");
			err = NAK_Server_is_busy;
			break;
		}

	}


GPSOCK_PLAYBACK_GETRAWDATA_DECODE_END:
	cmd_et = xTaskGetTickCount();
	{
		int enable = SO_RCVTIMEO_TIMEOUT;		// timeout ticks
		if(setsockopt(ctl_blk->socket_cmd_client, SOL_SOCKET, SO_RCVTIMEO, &enable, sizeof(enable)) < 0 )
		{
			DBG_PRINT("SO_RCVTIMEO error!\r\n");
		}
	}

	if (wifi_dl_buf_A)
	{
		gp_free((void*)wifi_dl_buf_A);
	}
	if (wifi_dl_buf_B)
	{
		gp_free((void*)wifi_dl_buf_B);
	}
	wifi_dl_state = WIFI_DOWNLOAD_FINISH;
	if(download_file_info.file_handle >= 0)
	{
        DBG_PRINT("Close file\r\n");
        close(download_file_info.file_handle);
        download_file_info.file_handle = -1;
	}
	DBG_PRINT("download finish time(%d) size(%d)\r\n",cmd_et-cmd_st,download_file_info.file_size);
	storage_task_msg_send(MSG_STORAGE_SERVICE_STORAGE_CHECK, NULL, 0, MSG_PRI_NORMAL);
	while(1)
	{
        if(wifi_download_task_flag == 0)
        {
            break;
        }
        DBG_PRINT("wait download task exit\r\n");
        osDelay(10);
	}
	return err;
}


#else

#define DOWNLOAD_RETRY 1000
#define DOWNLOAD_SD_BLOCK_SIZE (70*60*1024)
#define DOWNLOAD_PACKET_SIZE (60*1024)

STOR_SERV_DOWNLOAD_FILEINFO download_file_info;
// 目前 CMD 的 payload size 最大支持 64KB
// SD 一次讀取 150KB（為了提速），然後每50KB 為一個 packet command，而每個 packet，wi-fi drvier 每次約送 1.5KB
static int GPSOCK_Playback_GetRawData_Decode(unsigned short gp_sock_cmd, socket_cmd_ctl_t *ctl_blk, INT8U* rxstring)
{
	INT8U *buf = 0;
	INT16U play_file_index;
	INT8U packet_const[12] = {0x47,0x50,0x53,0x4F,0x43,0x4B,0x45,0x54,0x02,0x00,0x03,0x05}; //response : G,P,S,O,C,K,E,T,ACK(2 bytes),PLAYBACK,GetRawData
	INT32U sd_get_size;		//
	INT32U sd_get_idx = 0;
	INT32U sd_download_size;
	INT32S ret;
	INT32U cur_size = 0,cur_t,str_t,readsd_t = 0, netwr_t = 0;
	INT8U *pPacketbuf;
	INT16U *pPacksize;		// packet size
	INT8U *pBuf;	// download current buffer
	INT32S err = NAK_OK;
	//INT16U available;

	play_file_index = *(rxstring+13);
	play_file_index = play_file_index<<8;
	play_file_index |= *(rxstring+12);		// file index
	sd_download_size = DOWNLOAD_SD_BLOCK_SIZE;
	buf = (INT8U*)gp_malloc_align(sd_download_size+64, 8);
	if (buf == NULL)
	{
		printf("GPSOCK_Playback_GetRawData memory alloc fail\r\n");
		err = NAK_Invalid_command;
		goto GPSOCK_PLAYBACK_GETRAWDATA_DECODE_END;
	}
	gp_memset((INT8S*)(&download_file_info), 0, sizeof(download_file_info) );
	ap_storage_service_download_file_block_size(sd_download_size);

	DBG_PRINT("change non-blocking time for cancel download\r\n");
	#if GP_SOCK_TCP_BEHAVIOUR
	{
		int enable = 10;		// timeout ticks
		if(setsockopt((int)(ctl_blk->socket_cmd_client), SOL_SOCKET, SO_RCVTIMEO, &enable, sizeof(enable)) < 0 )
		{
			DBG_PRINT("SO_RCVTIMEO error!\r\n");
		}
	}
	#endif

	//////////// TODO ///////////
	DBG_PRINT("start download\r\n");
	do
	{
		// New data detected (i.e. cancel download) during download.
		// Leave the while loop to handle it.
		int rxlen = recv((int)(ctl_blk->socket_cmd_client), (void *)rx_buf, TCP_RX_BUF_MAX_LEN, 0);
		if(rxlen > 0)
		{
			DBG_PRINT("Cancel download\r\n");
			err = NAK_Invalid_command;
			break;
		}

		str_t = xTaskGetTickCount();
		// each     DOWNLOAD_SD_BLOCK_SIZE
		ret = ap_storage_service_download_file_req(play_file_index,&download_file_info,buf+64,&sd_get_size);
		cur_t = xTaskGetTickCount();
		if(ret == 0)
		{
			sd_get_idx = 0;
			// each DOWNLOAD_PACKET_SIZE
			while (sd_get_size != 0)
			{
				//INT32U retry_cnt = DOWNLOAD_RETRY;
				INT32U wlen, tlen, size;

				readsd_t += (cur_t-str_t);
				if(sd_get_idx==0)
				{
					pPacketbuf = (INT8U*)(buf + 64-GP_SOCK_RESP_PAYLOAD_OFFSET);
					sd_get_idx++;
					pBuf = (INT8U*)pPacketbuf;
					gp_memcpy((INT8S*)pBuf,(INT8S*)packet_const,sizeof(packet_const) );
					if (sd_get_size > (DOWNLOAD_PACKET_SIZE-64+GP_SOCK_RESP_PAYLOAD_OFFSET))
					{
						size = DOWNLOAD_PACKET_SIZE-64+GP_SOCK_RESP_PAYLOAD_OFFSET;
						sd_get_size -= size;
					}
					else
					{
						size = sd_get_size;
						sd_get_size = 0;
					}
					pPacksize = (INT16U*)(pBuf+GP_SOCK_RESP_PAYLOAD_SIZE_OFFSET);
					*pPacksize = size;
					tlen = size+GP_SOCK_RESP_PAYLOAD_OFFSET;
				}
				else
				{
					pPacketbuf = (INT8U*)(buf + sd_get_idx*DOWNLOAD_PACKET_SIZE);
					sd_get_idx++;
					pBuf = (INT8U*)pPacketbuf;
					gp_memcpy((INT8S*)pBuf,(INT8S*)packet_const,sizeof(packet_const) );
					if (sd_get_size > DOWNLOAD_PACKET_SIZE)
					{
						size = DOWNLOAD_PACKET_SIZE;
						sd_get_size -= DOWNLOAD_PACKET_SIZE;
					}
					else
					{
						size = sd_get_size;
						sd_get_size = 0;
					}
					pPacksize = (INT16U*)(pBuf+GP_SOCK_RESP_PAYLOAD_SIZE_OFFSET);
					*pPacksize = size;
					tlen = size+GP_SOCK_RESP_PAYLOAD_OFFSET;
				}
				// Wi-Fi driver divide into few packets
				while(tlen)
				{
					#if 0
					if(tlen > CLIENT_BUF_SIZE)
						wlen = CLIENT_BUF_SIZE;
					else
						wlen = tlen;

					#else
					if(tlen > DOWNLOAD_PACKET_SIZE)
					{
						wlen = DOWNLOAD_PACKET_SIZE;
					}
					else
					{
						wlen = tlen;
					}
					#endif

					{
						#if 0
						socket_api_ctl_t *ctl_blk = (socket_api_ctl_t *)void_ctl_blk;
						#endif

						ret = send((int)(ctl_blk->socket_cmd_client), (char const*)pBuf, wlen, 0);
					}

					//if(ret)
						ret = STATUS_OK;
					//else
						//STATUS_FAIL;
					netwr_t += (cur_t-str_t);

					if(ret != STATUS_OK)
					{
						err = NAK_Write_Fail;
						DBG_PRINT("raw data get error: send data fail(wlen=%d)\r\n", wlen);
						goto GPSOCK_PLAYBACK_GETRAWDATA_DECODE_END;
					}
					tlen -= wlen;
					pBuf += wlen;

					/*available = socket_cmd_wifi_get_client_avail_tx_buf_size(ctl_blk);
					if (available)
					{
						if(tlen > available)
							wlen = available;
						else
							wlen = tlen;

						// printf("available=%d,wlen=%d,tlen=%d,pBuf=%p\r\n",available,wlen,tlen,pBuf);
						str_t = OSTimeGet();
						ret = socket_cmd_wifi_send_data(ctl_blk, pBuf, wlen);
						cur_t = OSTimeGet();
						netwr_t += (cur_t-str_t);
						if(ret != SC_RET_SUCCESS)
						{
							err = NAK_Write_Fail;
							printf("raw data get error: send data fail(wlen=%d)\r\n", wlen);
							goto GPSOCK_PLAYBACK_GETRAWDATA_DECODE_END;
						}

						tlen -= wlen;
						pBuf += wlen;
						retry_cnt = DOWNLOAD_RETRY;
					}
					else
					{
						OSTimeDly(1);
						retry_cnt--;
						if(!retry_cnt)
						{
							printf("wifi send data timeout,%d\r\n",retry_cnt);
							err = NAK_Request_timeout;
							goto GPSOCK_PLAYBACK_GETRAWDATA_DECODE_END;
						}
					}*/
				}
				cur_size += size;
				// DBG_PRINT("wifi send data OK,%d\r\n",retry_cnt);
			}
		}
		else
		{
			DBG_PRINT("file down load error!!\r\n");
			err = NAK_Server_is_busy;
			break;
		}
	}
	while(download_file_info.file_size > cur_size);

GPSOCK_PLAYBACK_GETRAWDATA_DECODE_END:
	#if GP_SOCK_TCP_BEHAVIOUR
	{
		int enable = 3000;		// timeout ticks
		if(setsockopt((int)(ctl_blk->socket_cmd_client), SOL_SOCKET, SO_RCVTIMEO, &enable, sizeof(enable)) < 0 )
		{
			DBG_PRINT("SO_RCVTIMEO error!\r\n");
		}
	}
	#endif
	if (buf)
	{
		gp_free((void*)buf);
		DBG_PRINT("download finish  %d,%d\r\n",readsd_t,netwr_t);
	}

	return err;
}
#endif
INT32S GPSOCK_Playback_Stop_Decode(unsigned short gp_sock_cmd, socket_cmd_ctl_t *ctl_blk, INT8U* rxstring)
{
	//INT32U i;
	INT32S ret = NAK_OK;

	if (ap_browse_sts_get() & BROWSE_PLAYBACK_BUSY)
	{	// stop playing file
		app_msg_send(MSG_APQ_MJPEG_DECODE_END, NULL, 0, MSG_PRI_NORMAL);
		osDelay(100);
		DBG_PRINT("GPSOCK_Playback_Stop_Decode_1\r\n");
	}
	DBG_PRINT("GPSOCK_Playback_Stop_Decode_2\r\n");

	rtsp_h264_stop_service();

	// Init H264 for Preview
	rtsp_method_type_set(0);		// h264

	ap_storage_service_timer_start();
	return ret;
}

INT32S GPSOCK_Playback_GetSpecificName_Decode(INT8U* rxstring, unsigned short *p_status)
{
	INT16U play_file_order;
	INT16S play_file_index;
	INT32S ret = STATUS_OK;
	INT32U i;

	play_file_order = *(rxstring+13);
	play_file_order = play_file_order<<8;
	play_file_order |= *(rxstring+12);

	for (i = 0; i < PLAYBACK_START_TIME_OUT; ++i)
	{
		if (ap_browse_sts_get() == BROWSE_IDLE)
			break;
		osDelay(1);
	}

	play_file_index = ap_storage_service_indexfile_req(play_file_order, playfile_index_table);
	if  (play_file_index == -1)
	{
		ret = NAK_Get_file_list_fail;
	}
	else
	{
		*p_status =(unsigned short)play_file_index;
	}

	return ret;
}

INT32S GPSOCK_Playback_DeleteFile_Decode(unsigned short gp_sock_cmd, socket_cmd_ctl_t *ctl_blk, INT8U* rxstring)
{
	INT32U i;
	INT32S ret = NAK_OK;
	INT32S wifi_mode;
    INT16U browse_delete_idx;
	browse_delete_idx = *(rxstring+13);
	browse_delete_idx = browse_delete_idx<<8;
	browse_delete_idx |= *(rxstring+12);

	// delete  file
//	browse_delete_idx = rxstring[13]<<8 | rxstring[12];
	DBG_PRINT("delidx:%d ",browse_delete_idx);

	wifi_mode = gp_mode_remap();
	if (wifi_mode !=BROWSE_MODE)
	{
		ret = NAK_Mode_error;
		goto GPSOCK_PLAYBACK_DELETEFILE_END;
	}

	for (i = 0; i < PLAYBACK_START_TIME_OUT; ++i)
	{
		if (ap_browse_sts_get() == BROWSE_IDLE)
			break;
		osDelay(1);
	}

	ret = ap_storage_service_index_delfile(browse_delete_idx);

GPSOCK_PLAYBACK_DELETEFILE_END:
	DBG_PRINT("(%d)\r\n",ret);
//	osDelay(20);
	return ret;
}

INT32S General_GetParameterFile_Decode(void *void_ctl_blk, INT16S gp_sock_cmd, INT8U* rxstring, INT8U* tx_buf)
{
	INT32S  ret = NAK_OK;
	INT16S *pGP_Cmd = (INT16S *) (tx_buf + GP_SOCK_CMD_TYPE_OFFSET);
	INT8U  *pGP_ID = (INT8U *)(tx_buf+GP_SOCK_CMD_MODE_ID_OFFSET);
	INT8U  *p = (INT8U*)(&gp_sock_cmd);
	INT8U  *xml_ptr = (INT8U*)(&wifi_menu);
	INT32S xml_len, xml_offset = 0, txlen;

	xml_len = (INT32S)sizeof(wifi_menu);
	*pGP_Cmd = GP_SOCK_TYPE_ACK;
	*pGP_ID++ = p[1];		// MODE
	*pGP_ID++ = p[0];		// CMD_ID
	pGP_Cmd += 2;	// point to payload
	DBG_PRINT("xml_len = %d\r\n", xml_len);
	while (xml_len > 0)
	{
		INT32S err = 0;
		//int flag = 0;

		xml_len -= (TCP_TX_BUF_MAX_LEN - GP_SOCK_RESP_PAYLOAD_OFFSET);
		if (xml_len > 0)
		{
			*pGP_Cmd = (INT16S)(TCP_TX_BUF_MAX_LEN - GP_SOCK_RESP_PAYLOAD_OFFSET);		           // continuous frame
			//flag = nv_read(xml_fd,  (INT32U)(tx_buf+GP_SOCK_RESP_PAYLOAD_OFFSET), TCP_TX_BUF_MAX_LEN-GP_SOCK_RESP_PAYLOAD_OFFSET);
			gp_memcpy((INT8S *)(tx_buf+GP_SOCK_RESP_PAYLOAD_OFFSET), (INT8S *)(xml_ptr + xml_offset),(TCP_TX_BUF_MAX_LEN-GP_SOCK_RESP_PAYLOAD_OFFSET));
			xml_offset += (TCP_TX_BUF_MAX_LEN-GP_SOCK_RESP_PAYLOAD_OFFSET);
		}
		else
		{
			*pGP_Cmd = (INT16S)(xml_len + TCP_TX_BUF_MAX_LEN - GP_SOCK_RESP_PAYLOAD_OFFSET);		// last frame
			//flag = nv_read(xml_fd,  (INT32U)(tx_buf+GP_SOCK_RESP_PAYLOAD_OFFSET), (xml_len + TCP_TX_BUF_MAX_LEN-GP_SOCK_RESP_PAYLOAD_OFFSET));
			gp_memcpy((INT8S *)(tx_buf+GP_SOCK_RESP_PAYLOAD_OFFSET), (INT8S *)(xml_ptr + xml_offset),(xml_len + TCP_TX_BUF_MAX_LEN - GP_SOCK_RESP_PAYLOAD_OFFSET));
			xml_offset += (xml_len + TCP_TX_BUF_MAX_LEN - GP_SOCK_RESP_PAYLOAD_OFFSET);
		}
		// printf("flag = %d\r\n",flag);
		INT32U retry = 10;

send_again:
		txlen =  (*pGP_Cmd + GP_SOCK_RESP_PAYLOAD_OFFSET);

		{
			socket_api_ctl_t *ctl_blk = (socket_api_ctl_t *)void_ctl_blk;
			err = send(ctl_blk->socket_cmd_client, (void*)tx_buf, txlen, 0);
			if (err <0)
				DBG_PRINT("gp cmd: netconn_write fail, err %d\r\n", err);
		}

		if (err <0)
		{
			DBG_PRINT("General_GetParameterFile_Decode:retry %d\r\n",retry);
			if(retry){
				retry--;
				DBG_PRINT("xml send again\r\n");
				goto send_again;
			}
			else{
				DBG_PRINT("xml get error\r\n");
			}
		}
	}
	DBG_PRINT("xml finish\r\n");

//CMD_GETPARAMETERFILE_DECODE_END:
	return ret;
}

INT32S Menu_GetParameter_Decode(INT8U* rxstring,  unsigned char *buf)
{
	INT32S  ret = 1;
	INT32S *pID = (INT32S*)(&rxstring[GP_SOCK_CMD_PAYLOAD_OFFSET]);
	//INT8U ssid_length;
	buf[0] = 0;

	switch (*pID)
	{
		// ===============================================
		// RECORD
		// ===============================================
		case Record_Resolution:
			buf[0] = ap_state_config_video_resolution_get();
			break;
		case Record_Exposure:
			buf[0] = ap_state_config_record_ev_get();
			break;
		case Record_MotionDetection:
			buf[0] = ap_state_config_md_get();
			break;
		case Record_Loop_Recording:
			buf[0] = ap_state_config_record_time_get();
			break;
		case Record_WDR:
			break;
		case Record_RecordAudio:
			buf[0] = ap_state_config_voice_record_switch_get();
			break;
		case Record_DateStamp:
			buf[0] = ap_state_config_record_date_stamp_get();
			break;
		// ===============================================
		// CAPTURE
		// ===============================================
		case Capture_Resolution:
			buf[0] = ap_state_config_pic_size_get();
			break;
		case Capture_Exposure:
			buf[0] = ap_state_config_capture_ev_get();
			break;
		case Capture_Quality:
			buf[0] = ap_state_config_quality_get();
			break;
		case Capture_Sequence:
			buf[0] = ap_state_config_capture_burst_get();
			break;
		case Capture_Sharpness:
			buf[0] = ap_state_config_sharpness_get();
			break;
		case Capture_ISO:
			buf[0] = ap_state_config_iso_get();
			break;
		case Capture_AntiShaking:
			buf[0] = ap_state_config_anti_shaking_get();
			break;
		case Capture_DateTime:
			buf[0] = ap_state_config_capture_date_stamp_get();
			break;
		case Capture_WhiteBalance:
			buf[0] = ap_state_config_white_balance_get();
			break;
		// ===============================================
		// SYSTEM
		// ===============================================
		case System_Frequency:
			buf[0] = ap_state_config_light_freq_get();
			break;
		case System_ScreenSaver:
			buf[0] = ap_state_config_auto_off_TFT_BL_get();
			break;
		case System_AutoPowerOff:
			buf[0] = ap_state_config_auto_off_get();
			break;
		case System_Language:
			// The stored value is actual menu value + 1.
			// We have to minus 1 to get the original menu value.
			buf[0] = ap_state_config_language_get();
			break;
		case System_BeepSound:
			buf[0] = ap_state_config_beep_sound_get();
			break;
		case System_DataTime:
			buf[0] = ap_state_config_date_time_mode_get();
			break;
		case WiFi_Name:
			{
				INT8U wifi_ssid[GPSOCK_WiFi_Name_Length];
				INT8U wifi_ssid_len = 0;

				memset(&wifi_ssid, 0 , sizeof(wifi_ssid));
				ap_state_wifi_ssid_get((INT8U*)&wifi_ssid, (INT8U*)&wifi_ssid_len);
				gp_memcpy( (INT8S*)buf, (INT8S *)&wifi_ssid, (INT32U)wifi_ssid_len);
				ret = (wifi_ssid_len == 0) ? NAK_Invalid_command : (wifi_ssid_len + 1);
				buf[wifi_ssid_len] = 0;
				DBG_PRINT("wifi_ssid %s len %d\r\n", wifi_ssid, wifi_ssid_len);
			}
			break;
		case WiFi_Passwd:
			{
				INT8U wifi_pw[GPSOCK_WiFi_Passwd_Length];
				INT8U wifi_pw_len = 0;

				memset(&wifi_pw, 0 , sizeof(wifi_pw));
				ap_state_wifi_password_get((INT8U*)&wifi_pw, (INT8U*)&wifi_pw_len);
				gp_memcpy( (INT8S*)buf, (INT8S *)&wifi_pw, (INT32U)wifi_pw_len);
				ret = (wifi_pw_len == 0) ? NAK_Invalid_command : (wifi_pw_len + 1);
				buf[wifi_pw_len] = 0;
				DBG_PRINT("wifi_pw %s len %d\r\n", wifi_pw, wifi_pw_len);
			}
			break;
		case System_ClearBuffer:
		case System_Format:
		case System_DefaultSetting:
		case System_Version:
		default:
			buf[0] = 0;
	}
	DBG_PRINT("ID = 0x%x  Data:0x%x\r\n", *pID, buf[0]);

	return ret;
}

INT32S Menu_SetParameter_Decode(INT8U* rxstring)
{
	INT32S ret = NAK_OK;
	INT32S id = *((INT32S*)(&rxstring[GP_SOCK_CMD_PAYLOAD_OFFSET]));
	INT8U value = rxstring[GP_SOCK_CMD_PAYLOAD_OFFSET+SetParameterDataOffset];
	//STRING_INFO str;

	DBG_PRINT("set:0x%x, 0x%x\r\n", id, value);
	//str.font_color = (INT16U)value;
	switch (id)
	{
		// ===============================================
		// RECORD
		// ===============================================
		case Record_Resolution:
			//str.str_idx = STR_RESOLUTION;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_video_resolution_set(value);
			break;
		case Record_Exposure:
			//str.str_idx = STR_EV;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_record_ev_set(value);
			break;
		case Record_MotionDetection:
			//str.str_idx = STR_MOTIONDETECT;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_md_set(value);
			break;
		case Record_Loop_Recording:
			//str.str_idx = STR_LOOPRECORDING;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_record_time_set(value);
			break;
		case Record_WDR:
			break;
		case Record_RecordAudio:
			//str.str_idx = STR_RECORD_AUDIO;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_voice_record_switch_set(value);
			break;
		case Record_DateStamp:
			//str.str_idx = STR_DATE_STAMP;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_record_date_stamp_set(value);
			break;
		// ===============================================
		// CAPTURE
		// ===============================================
		case Capture_Resolution:
			//str.str_idx = STR_RESOLUTION2;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_pic_size_set(value);
			break;
		case Capture_Exposure:
			//str.str_idx = STR_EV;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_capture_ev_set(value);
			break;
		case Capture_Quality:
			//str.str_idx = STR_QUALITY;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_quality_set(value);
			break;
		case Capture_Sequence:
			//str.str_idx = STR_SEQUENCE;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_capture_burst_set(value);
			break;
		case Capture_Sharpness:
			//str.str_idx = STR_SHARPNESS;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_sharpness_set(value);
			break;
		case Capture_ISO:
			//str.str_idx = STR_ISO;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_iso_set(value);
			break;
		case Capture_AntiShaking:
			//str.str_idx = STR_ANTI_SHAKING;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_anti_shaking_set(value);
			break;
		case Capture_DateTime:
			//str.str_idx = STR_DATE_STAMP;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_capture_date_stamp_set(value);
			break;
		case Capture_WhiteBalance:
			//str.str_idx = STR_WHITE_BALANCE;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_white_balance_set(value);
			break;
		// ===============================================
		// SYSTEM
		// ===============================================
		case System_Frequency:
			//str.str_idx = STR_FREQUENCY;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_light_freq_set(value);
			break;
		case System_ScreenSaver:
			//str.str_idx = STR_SCREEN_SAVER;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_auto_off_TFT_BL_set(value);
			break;
		case System_AutoPowerOff:
			//str.str_idx = STR_AUTO_POWER_OFF;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_auto_off_set(value);
			break;
		case System_Language:
			//str.str_idx = STR_LANGUAGE;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_language_set(value);
			break;
		case System_BeepSound:
			//str.str_idx = STR_BEEP_SOUND;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_beep_sound_set(value);
			break;
		case System_DataTime:
			ap_state_config_date_time_mode_set(value);
			break;
		case System_ClearBuffer:
			break;
		case System_Format:
			//osThreadSetPriority(socket_cmd_task, osPriorityLow);	// 太高的優先級，會把其它線程打掛
			ap_storage_service_format_req();
			//osThreadSetPriority(socket_cmd_task, osPriorityNormal);
			break;
		case System_DefaultSetting:
			// White balance (mantis 87166)
			//str.str_idx = STR_WHITE_BALANCE;
			//str.font_color = (INT16U)0;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			drv_l2_gp_cdsp_set_awb_by_config(0);
			ap_state_config_white_balance_set(0);
			// Default
			//str.str_idx = STR_DEFAULT_SETTING;
			//str.font_color = (INT16U)1;
			//ap_setting_right_menu_active(&str, MENU_FUNCTION_ACTIVE, (INT8U *)NULL);
			ap_state_config_default_set();
			break;
		case System_Version:
			break;
		case WiFi_Name:
			{
				INT8U wifi_ssid[GPSOCK_WiFi_Name_Length];
				INT8U wifi_ssid_len = 0;
				INT8U *p = (INT8U *)(rxstring+GP_SOCK_CMD_PAYLOAD_OFFSET + SetParameterDataOffset);
				INT8U i = 0;
				memset(&wifi_ssid, 0, GPSOCK_WiFi_Name_Length);
				//count ssid length
				for(i = 0; i < GPSOCK_WiFi_Name_Length; i++)
				{
					if(*(p+i) == 0)
						break;
					DBG_PRINT("%x,",*(p+i));
				}
				DBG_PRINT("\r\n");
				wifi_ssid_len = i;
				DBG_PRINT("wifi_ssid_len %d\r\n",wifi_ssid_len);
				gp_memcpy((INT8S*)&wifi_ssid, (INT8S*)p, wifi_ssid_len);

				ap_state_wifi_ssid_set((INT8U*)&wifi_ssid, wifi_ssid_len);
			}
			break;
		case WiFi_Passwd:
			{
				INT8U wifi_pw[GPSOCK_WiFi_Passwd_Length];
				INT8U wifi_pw_len = 0;
				INT8U *p = (INT8U *)(rxstring+GP_SOCK_CMD_PAYLOAD_OFFSET + SetParameterDataOffset);
				INT8U i = 0;
				memset(&wifi_pw, 0, GPSOCK_WiFi_Passwd_Length);
				//count pw length
				for(i = 0; i < GPSOCK_WiFi_Passwd_Length; i++)
				{
					if(*(p+i) == 0)
						break;
					DBG_PRINT("%x,",*(p+i));
				}
				DBG_PRINT("\r\n");
				wifi_pw_len = i;
				DBG_PRINT("wifi_pw_len %d\r\n",wifi_pw_len);
				gp_memcpy((INT8S*)&wifi_pw, (INT8S*)p, wifi_pw_len);

				ap_state_wifi_password_set((INT8U*)&wifi_pw, wifi_pw_len);
			}
			break;
		default:
			ret = NAK_OK;
			break;
	}

	return ret;
}

extern INT8U g_LFSRseed[4];
extern INT8U g_LFSR_Key[6];
extern INT8U Gen_LFSR_32Bit_Key(void);
INT32S GPSOCK_General_AuthDevice_Decode(INT8U* rxstring, unsigned char *key)
{
	INT8U *seed = (INT8U*)(&rxstring[GP_SOCK_CMD_PAYLOAD_OFFSET]);
	DBG_PRINT("AuthDevice_Decode seed %x %x %x %x\r\n",seed[0],seed[1],seed[2],seed[3]);
	gp_memcpy((INT8S*)&g_LFSRseed, (INT8S *)seed, sizeof(g_LFSRseed));
	Gen_LFSR_32Bit_Key();
	gp_memcpy((INT8S *)key,(INT8S*)&g_LFSR_Key, sizeof(g_LFSR_Key));
	return 0;
}

extern void ap_display_timer(void);
extern INT32U ap_display_timer_rec_time_get(void);
INT32S General_GetDeviceStatus_Decode(unsigned char *status)
{
	//int ret = STATUS_OK;
	int val;
	char *p_val = (char *)(&val);
	char adp = 0;
	unsigned char flag = 0;
	char device_status = 0;
	unsigned char wifi_mode;

	if (present_state == STATE_VIDEO_RECORD)  // if record & capture mode is enable, device status =1 ,
	{
		if (video_record_sts & VIDEO_RECORD_BUSY)
			device_status = 1;
	}
	else if (present_state == STATE_PHOTO_CAPTURE)
	{
		if (g_wifi_notify_pic_done != 0)
			device_status = 1;
	}
	// buf[1]:bit1
	flag = ap_state_config_voice_record_switch_get();
	if (flag == 1)
	{
		device_status |= 0x2;
	}

	if (ap_peripheral_get_usbd_connection_state()== USBD_CONNECTED_TO_CHARGER)
			adp = 1;
	else	adp = 0;
	adp     |= 0x80;
	wifi_mode = (char)gp_mode_remap();
	status[0] = wifi_mode;
	status[1] = device_status;//0x03;
	status[2] = ap_peripheral_battery_level_get() | (1<<7);		// battery level , bit[7] for tell GoplusCam http(0) or rtsp(1)
	status[3] = adp;

	/////////// Add recording status
	ap_display_timer();
	val = ap_display_timer_rec_time_get();

	status[4] =  ap_state_config_video_resolution_get();
	status[5] = p_val[0];
	status[6] = p_val[1];
	status[7] = p_val[2];
	status[8] = p_val[3];

	/////////// Add capture status
	val = ap_capture_left_num_get();
	status[9] = ap_state_config_pic_size_get();
	status[10] = p_val[0];
	status[11] = p_val[1];
	status[12] = p_val[2];
	status[13] = p_val[3];

	//status[14] = 0; //frame rate

	return 0;
}


INT32S gp_wifi_upgrade_check(INT8U **p_Firmware , INT32U *p_Size)
{
	INT32S ret = 0;

	if ( (pfirmware!=NULL)&&(firmware_download_size!=0) )
	{
		ret = 1;
		*p_Firmware = pfirmware;
		*p_Size = firmware_download_size;
	}

	return ret;
}

static int GPSOCK_Firmware_Download_Decode(unsigned short gp_sock_cmd, void *void_ctl_blk, INT8U* rxstring)
{
	INT32S ret = NAK_OK;
	INT32U *p;

#if 0
	if ( (bat_lvl_cal_bak<3)&&(adp_status == 0) )
	{
		ret = NAK_Battery_low;
		goto GPSOCK_Firmware_Download_Decode_END;
	}
#endif
	// malloc memory
	pfirmware = gp_malloc_align(SOCKET_CMD_FIRMWARE_BUF_SIZE, 64);
	prx_firmware_buf = gp_malloc_align(SOCKET_CMD_DOWNLOAD_BUF_SIZE, 64);
	if ( (pfirmware==NULL)||(prx_firmware_buf==NULL) )
	{
		ret = NAK_Mem_malloc_error;
		DBG_PRINT("[ERR] Firmware Buffer is null\r\n");
		goto GPSOCK_Firmware_Download_Decode_END;
	}
	else
	{
		pfirmware_cur = pfirmware;
	}

	p = (INT32U*)(rxstring+12);
	firmware_download_size = *p;
	p++;
	firmware_download_checksum = *p;
	firmware_cur_size = 0;
	DBG_PRINT("download_size=%d, download_checksum=0x%x\r\n",firmware_download_size,firmware_download_checksum);

GPSOCK_Firmware_Download_Decode_END:
	return ret;
}

static int GPSOCK_Firmware_SendRawData_Decode(unsigned short gp_sock_cmd, void *void_ctl_blk, INT8U* rxstring)
{
	INT32S ret = NAK_OK;
	INT32U size;
	INT8U *p;

	p = (INT8U*)(rxstring + 12);
	size  = (INT32U)(*p++);
	size += ((*p++) << 8);

	//DBG_PRINT("cur_size=%d, size = %d\r\n", firmware_cur_size, size);
	firmware_upgrade_flag = 0;

	if (size!=0)
	{	// downloading
		gp_memcpy((INT8S*)pfirmware_cur, (INT8S*)p, size);
		firmware_cur_size += size;
		pfirmware_cur += (INT32U)size;
	}
	else
	{	// checksum
		INT32U i;
		INT32U chksum = 0;

		DBG_PRINT("download finish\r\n");
		if (firmware_cur_size!=firmware_download_size)
		{
			gp_free(pfirmware);	pfirmware = NULL;
			gp_free(prx_firmware_buf); prx_firmware_buf = NULL;
			DBG_PRINT("download size mismatch !! %d %d\r\n",firmware_cur_size,firmware_download_size);
			ret = NAK_Checksum_error;
			goto GPSOCK_Firmware_SendRawData_Decode_END;
		}
		for (i=0; i<firmware_download_size;++i)
		{
			chksum += pfirmware[i];
		}
		if (chksum!=firmware_download_checksum)
		{
			gp_free(pfirmware); pfirmware = NULL;
			gp_free(prx_firmware_buf); prx_firmware_buf = NULL;
			DBG_PRINT("firmware checksum error !! 0x%x 0x%x\r\n",chksum,firmware_download_checksum);
			ret = NAK_Checksum_error;
			goto GPSOCK_Firmware_SendRawData_Decode_END;
		}
		firmware_upgrade_flag = 1;		// firmware verify pass

	}

GPSOCK_Firmware_SendRawData_Decode_END:
	return ret;
}

static int GPSOCK_Firmware_Upgrade_Decode(unsigned short gp_sock_cmd, void *void_ctl_blk, INT8U* rxstring)
{
	INT32S ret = NAK_OK;

	DBG_PRINT("GPSOCK_Firmware_Upgrade_Decode\r\n");

	if (firmware_upgrade_flag != 1)
	{
		gp_free(pfirmware); pfirmware = NULL;
		gp_free(prx_firmware_buf); prx_firmware_buf = NULL;
		DBG_PRINT("NAK_Checksum_error\r\n");
		ret = NAK_Checksum_error;
		goto GPSOCK_Firmware_Upgrade_Decode_END;
	}

	//if ( ap_state_firmware_upgrade() == -1 )
	{

		DBG_PRINT("ap_state_firmware_upgrade malloc buffer error\r\n");
		gp_free(pfirmware); pfirmware = NULL;
		gp_free(prx_firmware_buf); prx_firmware_buf = NULL;
		ret = NAK_Mem_malloc_error;

		goto GPSOCK_Firmware_Upgrade_Decode_END;
	}
	firmware_upgrade_flag = 2;

GPSOCK_Firmware_Upgrade_Decode_END:
	return ret;
}

static int GPSOCK_General_SetGetPIP_Decode(unsigned short gp_sock_cmd, INT8U* rxstring)
{
	INT8U temp;

	temp = rxstring[GP_SOCK_CMD_PAYLOAD_OFFSET];
	if(temp)
	{
		msgQSend(ApQ, MSG_APQ_WIFI_PIP_SWITCH, NULL, 0, MSG_PRI_NORMAL);
	}
	return 0;
}

INT32S Vendor_Command_Decode(unsigned short gp_sock_cmd, void *void_ctl_blk, INT8U* rxstring,  unsigned char *buf)
{
	int i;
	unsigned char *p = (unsigned char *)(&rxstring[GP_SOCK_CMD_PAYLOAD_OFFSET+2]);
	short size = *((short*)(&rxstring[GP_SOCK_CMD_PAYLOAD_OFFSET]));
	TIME_T	g_time;

	if (size > GPSOCK_Vendor_Command_Buf_Length)
	{
		size = GPSOCK_Vendor_Command_Buf_Length;
	}

	for (i = 0; i < size; ++i)
	{
		buf[i] = p[i];
	}

	if((*p == 'G')&&(*(p+1) == 'P')&&(*(p+2) == 'V')&&(*(p+3) == 'E')&&(*(p+4) == 'N')&&(*(p+5) == 'D')
	   &&(*(p+6) == 'O')&&(*(p+7) == 'R'))
	{
		g_time.tm_year = p[11];
		g_time.tm_year = g_time.tm_year<<8;
		g_time.tm_year += p[10];
		g_time.tm_mon  = p[12];
		g_time.tm_mday = p[13];
		g_time.tm_hour = p[14];
		g_time.tm_min = p[15];
		g_time.tm_sec = p[16];
		cal_time_set(g_time);
		DBG_PRINT("Phone set time:%d-%02d-%02d %02d:%02d:%02d\r\n",g_time.tm_year,g_time.tm_mon,g_time.tm_mday,
		                                                           g_time.tm_hour,g_time.tm_min,g_time.tm_sec);
		/*#if ENABLE_CHECK_RTC == 1
		ap_state_config_data_time_save_set();
		#endif*/
	}
	return (int)size;
}

/*************************************************/
/*
* input
* ctl_blk: server/client netconn
* rx_buf: received buffer
* rxlen: received buffer length
*
* ouput
* txlen: response buffer length
* tx_buf: response buffer
*/
static void gp_cmd_process(void *void_ctl_blk, INT8U *rx_buf, INT16U rxlen, INT8U *tx_buf, void *txlen)
{
	static INT16S GP_SOCK_MODE = 0, GP_SOCK_ID = 0;
	void *len = txlen;
	INT16U GP_SOCK_Cmd;
	//INT32U status;
	//INT32S SD_status;
	GP_SOCK_MODE = (INT16S)(rx_buf[GP_SOCK_CMD_MODE_ID_OFFSET]);
	GP_SOCK_ID   = (INT16S)(rx_buf[GP_SOCK_CMD_CMD_ID_OFFSET]);
	GP_SOCK_Cmd  = (GP_SOCK_MODE<<8)|GP_SOCK_ID;
	memset(tx_buf, 0, TCP_TX_BUF_MAX_LEN);
	strncpy((char *)tx_buf, (const char*)"GPSOCKET", 8);
	/*if(!(GP_SOCK_Cmd == GPSOCK_General_GetDeviceStatus || GP_SOCK_Cmd == GPSOCK_Firmware_SendRawData))
	DBG_PRINT("gpcmd received cmd:0x%x\r\n",GP_SOCK_Cmd);*/

	#if 0
	DBG_PRINT("[AI]>>>>>>>>>>>>>>>>>>>>>>>CMD = 0x%x\r\n",GP_SOCK_Cmd);
	#endif

	switch (GP_SOCK_Cmd)
	{
		case GPSOCK_General_SetMode:
			{
				GPSOCK_General_SetMode_Decode(rx_buf);
				*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
			}
			break;
		case GPSOCK_General_RestarStreaming:
			{
				*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
			}
			break;
		case GPSOCK_General_PowerOff:
			{
				//msgQSend(ApQ, MSG_APQ_POWER_KEY_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
				*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
			}
			break;
		case GPSOCK_Record_Start:
			{
				int ret = STATUS_OK;
				int timeout = 2000; //20s
				INT16U tmp=0;

                if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
                {
                    ap_peripheral_ok_key_exe(&tmp);
                    while(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
                    {
                        osDelay(10);
                        if(--timeout == 0)  break;
                    }
                }
                else
                {
                    ap_peripheral_ok_key_exe(&tmp);
                }

				if (ret == STATUS_OK)
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
				}
				else
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (short)ret, 0, 0, tx_buf);
				}
			}
			break;
            case GPSOCK_Record_Audio:
            {
				int ret = STATUS_OK;
                ret = GPSOCK_Record_Audio_Decode(GP_SOCK_Cmd, void_ctl_blk, tx_buf);
                if (ret == NAK_OK)
                {
                    *(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
                }
                else
                {
                    *(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (short)ret, 0, 0, tx_buf);
                }
            }
            break;
			case GPSOCK_CapturePicture_Capture:
			{
				int ret = STATUS_OK;
				ret = GPSOCK_CapturePicture_Capture_Decode(GP_SOCK_Cmd, void_ctl_blk, rx_buf);

				if (ret == STATUS_OK)
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
				}
				else
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (short)ret, 0, 0, tx_buf);
				}
			}
			break;
		case GPSOCK_Playback_Start:
			{
				GPSOCK_Playback_Start_Decode(GP_SOCK_Cmd, void_ctl_blk, rx_buf);
				*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
			}
			break;
		case GPSOCK_Playback_Pause:
			{
				GPSOCK_Playback_Pause_Decode(GP_SOCK_Cmd, void_ctl_blk, rx_buf);
				*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
			}
			break;
		case GPSOCK_Playback_GetFileCount:
			{
				int ret = STATUS_OK;
				ret =   GPSOCK_Playback_GetFileCount_Decode(GP_SOCK_Cmd, tx_buf);
				if (ret >= 0)
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0002, (unsigned char *)(&ret), 2, tx_buf);
				}
				else
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (short)ret, 0, 0, tx_buf);
				}
			}
			break;
		case GPSOCK_Playback_GetFileList:
			{
				int ret = STATUS_OK;
				ret = GPSOCK_Playback_GetFileList_Decode(void_ctl_blk, rx_buf, tx_buf);
				if (ret < 0)
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (short)ret, 0, 0, tx_buf);
				}
				else
					*(INT16U*)len = ret;
			}
			break;
		case GPSOCK_Playback_GetThumbnail:
			{
				int ret = STATUS_OK;
				ret = GPSOCK_Playback_GetThumbnail_Decode(GP_SOCK_Cmd, void_ctl_blk, rx_buf);

				if (ret == STATUS_OK)
				{
					*(INT16U*)len = gp_resp_set(GP_SOCK_TYPE_ACK<<16|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
				}
				else
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (short)ret, 0, 0, tx_buf);
				}
			}
			break;
		case GPSOCK_Playback_GetRawData:
			{
				int ret = STATUS_OK;
				ret = GPSOCK_Playback_GetRawData_Decode(GP_SOCK_Cmd, void_ctl_blk, rx_buf);
				if (ret == STATUS_OK)
				{
					*(INT16U*)len = gp_resp_set(GP_SOCK_TYPE_ACK<<16|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
				}
				else
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (short)ret, 0, 0, tx_buf);
				}
			}
			break;
		case GPSOCK_Playback_Stop:
			{
				int ret = STATUS_OK;
				ret = GPSOCK_Playback_Stop_Decode(GP_SOCK_Cmd, void_ctl_blk, rx_buf);
				if (ret == STATUS_OK)
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
				}
				else
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (short)ret, 0, 0, tx_buf);
				}
			}
			break;
		case GPSOCK_Playback_GetSpecificName:
			{
				unsigned short status;
				int ret = STATUS_OK;
				ret = GPSOCK_Playback_GetSpecificName_Decode(rx_buf, &status);
				if (ret == STATUS_OK)
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0002, (unsigned char *)(&status), 2, tx_buf);
				}
				else
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (short)ret, 0, 0, tx_buf);
				}
			}
			break;
        case GPSOCK_Playback_DeleteFile:
		{
			int ret = STATUS_OK;
			ret = GPSOCK_Playback_DeleteFile_Decode(GP_SOCK_Cmd, void_ctl_blk, rx_buf);
			if (ret == STATUS_OK)
			{
                *(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
			}
			else
			{
				*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (short)ret, 0, 0, tx_buf);
			}
		}
		break;
		case GPSOCK_General_GetInformation:
			{
				unsigned char value[GPSOCK_General_GetInfo_Length];
				General_GetInformation_Decode((INT8U*)&value);
				DBG_PRINT("[GETINFO]SD card 0x%x, flight control 0x%x, H264 0x%x \r\n",value[0],value[1],value[2]);
				*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), GPSOCK_General_GetInfo_Length, value, GPSOCK_General_GetInfo_Length, tx_buf);
			}
			break;
		case GPSOCK_General_GetParameterFile:
			{
				General_GetParameterFile_Decode(void_ctl_blk, GP_SOCK_Cmd, rx_buf, tx_buf);
				*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
			}
			break;
		case GPSOCK_General_GetDeviceStatus:
			{
				unsigned char status[GPSOCK_General_DeviceStatus_Length];
				General_GetDeviceStatus_Decode((INT8U*)&status);
				*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), GPSOCK_General_DeviceStatus_Length, status, GPSOCK_General_DeviceStatus_Length, tx_buf);
			}
			break;
		case GPSOCK_General_AuthDevice:
			{
				unsigned char key[GPSOCK_General_Key_Length];
				memset((INT8U*)&key, 0,GPSOCK_General_Key_Length);
				GPSOCK_General_AuthDevice_Decode(rx_buf, (INT8U*)&key);
				*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), GPSOCK_General_Key_Length, key, GPSOCK_General_Key_Length, tx_buf);
			}
			break;
		case GPSOCK_Firmware_Download:
			{
				int ret = STATUS_OK;

#if 0
				Rtsp_H264enc_Stop(RTSP_STREAM_WIFI);
				Rtsp_H264enc_Callback_Register(RTSP_STREAM_WIFI,0);
				Rtsp_H264enc_DeInit(RTSP_STREAM_WIFI, NULL);

				Rtsp_Pscaler_Sensor_Stop(PSCALER_B);
				Rtsp_Pscaler_Callback_Register(PSCALER_B, 0);
				Rtsp_Pscaler_Sensor_GP420_DeInit (PSCALER_B);
#endif

				ret = GPSOCK_Firmware_Download_Decode(GP_SOCK_Cmd, void_ctl_blk, rx_buf);
				if (ret==NAK_OK)
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
				}
				else
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (short)ret, 0, 0, tx_buf);
				}
			}
			break;
		case GPSOCK_Firmware_SendRawData:
			{
				int ret = STATUS_OK;
				ret = GPSOCK_Firmware_SendRawData_Decode(GP_SOCK_Cmd, void_ctl_blk, rx_buf);
				if (ret==NAK_OK)
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
				}
				else if (ret == NAK_Checksum_error)
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (short)ret, 0, 0, tx_buf);
				}
			}
			break;
		case GPSOCK_Firmware_Upgrade:
			{
				int ret = STATUS_OK;
				ret = GPSOCK_Firmware_Upgrade_Decode(GP_SOCK_Cmd, void_ctl_blk, rx_buf);
				if (ret==NAK_OK)
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
				}
				else
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (short)ret, 0, 0, tx_buf);
				}
			}
			break;
		case GPSOCK_Menu_GetParameter:
			{
				unsigned char value[GPSOCK_WiFi_Name_Length];
				int value_len = 0;
				value_len = Menu_GetParameter_Decode(rx_buf, (unsigned char *)(&value));
				if (value_len >= NAK_OK)
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), value_len, (unsigned char *)(&value), value_len, tx_buf);
				}
				else
				{
					*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), (INT16U)value_len, 0, 0, tx_buf);
				}
			}
			break;
		case GPSOCK_Menu_SetParameter:
			{
				Menu_SetParameter_Decode(rx_buf);
				*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), (INT16U)NAK_OK, 0, 0, tx_buf);
			}
			break;
		case GPSOCK_Vendor_Command:
			{
				unsigned char vendor_buf[GPSOCK_Vendor_Command_Buf_Length];
				unsigned int vendor_len;
				vendor_len = Vendor_Command_Decode(GP_SOCK_Cmd, void_ctl_blk, rx_buf, vendor_buf);
				*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), vendor_len, vendor_buf, vendor_len, tx_buf);
			}
			break;

		case GPSOCK_General_SetGetPIP:
			GPSOCK_General_SetGetPIP_Decode(GP_SOCK_Cmd, rx_buf);
		    *(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_ACK<<16)|(GP_SOCK_Cmd), 0x0000, 0, 0, tx_buf);
			break;

		default:
			*(INT16U*)len = gp_resp_set((GP_SOCK_TYPE_NAK<<16)|(GP_SOCK_Cmd), 0xFFFF, 0, 0, tx_buf);
	}
	//DBG_PRINT("gpcmd response: len %d\r\nACK:%x CMD:%x\r\n",*(INT16U*)len,tx_buf[8],tx_buf[11]);
}

INT32S gp_resp_set(INT32U type, INT16U payload_attr, INT8U *payload_data, INT32U payload_len, INT8U *tx_buf)
{
	INT8U  *resp_type         = (INT8U *) (tx_buf + GP_SOCK_RESP_TYPE_OFFSET);
	INT16U *resp_payload_attr = (INT16U *)(tx_buf + GP_SOCK_RESP_PAYLOAD_SIZE_OFFSET);
	CHAR   *resp_payload_data = (CHAR *)  (tx_buf + GP_SOCK_RESP_PAYLOAD_OFFSET);
	CHAR   *p = (CHAR *)(&type);

	// Write "GP_SOCK_Type + GP_SOCK_Mode_ID + GP_SOCK_Cmd_ID"
	// endian convert
	resp_type[0] = p[2];
	resp_type[1] = p[3];
	resp_type[2] = p[1];
	resp_type[3] = p[0];

	// Write "GP_SOCK_Payload"
	*resp_payload_attr = payload_attr;
	gp_memcpy((INT8S*)resp_payload_data,(INT8S*)payload_data, (INT32U)payload_len);

	return (payload_len+GP_SOCK_RESP_PAYLOAD_OFFSET);
}

#define SOCKET_CMD_KEEPALIVE_IDLE	    3	/* 10000ms */
#define SOCKET_CMD_KEEPALIVE_INTVL	    1	/* 1000ms */
#define SOCKET_CMD_KEEPALIVE_CNT	    20	/* 1 time */

int socket_cmd_connection_status(void)
{
	return socket_socket_ctl_blk.socket_cmd_client;
}
extern INT32S rtk_wifi_module_conn_status();


int force_close=0;

extern INT8U gwifi_transmission_stop;
void socket_cmd_task(void *pvParameters)
{
	//static INT16S GP_SOCK_MODE = 0, GP_SOCK_ID = 0, GP_SOCK_Cmd = 0;
	struct sockaddr_in server_addr, cli_addr;
	socklen_t client;
	socket_api_ctl_t *ctl_blk = &socket_socket_ctl_blk;
	int	enable = 1;
	int status;
	int keepalive = 1;
	int keep_idle = SOCKET_CMD_KEEPALIVE_IDLE;
	int keep_intvl = SOCKET_CMD_KEEPALIVE_INTVL;
	int keep_cnt = SOCKET_CMD_KEEPALIVE_CNT;
	int nodelay = 1;
	int state = 0, cnt = 0;

	ctl_blk->socket_cmd_server = socket(AF_INET, SOCK_STREAM, 0);
	if(ctl_blk->socket_cmd_server < 0)
	{
		DBG_PRINT("SCKET_CMD failed\r\n");
		goto SOCKET_CMD_TASK_END;
	}

	status = setsockopt(ctl_blk->socket_cmd_server, SOL_SOCKET, SO_KEEPALIVE, (const char*)&enable, sizeof(enable));
	if(status < 0)
	{
		DBG_PRINT("SOCKET_CMD SO_KEEPALIVE error\r\n");
		goto SOCKET_CMD_TASK_END;
	}

#if (WIFI_VENDOR == 1)
	/* Set socket address reusable */
	status = setsockopt(ctl_blk->socket_cmd_server, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable));
	if(status < 0)
	{
		DBG_PRINT("SOCKET_CMD REUSEADDR error %d\r\n", status);
		goto SOCKET_CMD_TASK_END;
	}
#endif

	/* Bind the TCP server socket address */
	gp_memset((INT8S *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_len         = sizeof(server_addr);
	server_addr.sin_port        = htons(SOCKET_CMD_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	status = bind(ctl_blk->socket_cmd_server, (struct sockaddr *)&server_addr,  sizeof(server_addr));
	if(status < 0)
	{
		DBG_PRINT("SOCKET_CMD bind error\r\n");
		goto SOCKET_CMD_TASK_END;
	}

	/* Listen */
	if(listen(ctl_blk->socket_cmd_server , 1) < 0)
	{
		DBG_PRINT("SOCKET_CMD listen error\r\n");
		goto SOCKET_CMD_TASK_END;
	}

	DBG_PRINT("gp cmd: server socket 0x%x\r\n", ctl_blk->socket_cmd_server);

	client = sizeof(cli_addr);
 	while((ctl_blk->socket_cmd_client = accept(ctl_blk->socket_cmd_server, (struct sockaddr *)&cli_addr, &client)) >= 0)
	{
		#if 0 // To fix it
		cloud_run_stop();	// stop sos file
		#endif

		#if 0 // Option
		// 連上 GoPlusCam 停止錄影
		if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
		{
			ap_video_record_func_key_active();
		}
		#endif

		/*
		// 刪除 ramdisk 時，SD 卡狀態已經錯了
		_deviceunmount(STORAGE_DEVICE);
		ap_video_record_sts_set(VIDEO_RECORD_UNMOUNT);
		// 讓 storage task 自動 mount SD card
		*/
		#if C_WIFI_ON_OFF_KEY == 0
		{
			INT8U type = KEY_MODE_DISABLE;
			peripheral_task_msg_send(MSG_PERIPHERAL_TASK_KEY_REGISTER, &type, sizeof(INT8U), MSG_PRI_NORMAL);
		}
		#endif

		if(setsockopt(ctl_blk->socket_cmd_client, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) != 0)
			DBG_PRINT("ERR: TC SO_KEEPALIVE \r\n");

		//set socket attribute
		if(setsockopt(ctl_blk->socket_cmd_client, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle)) < 0 )
		{
			DBG_PRINT("TC set TCP_KEEPIDLE err! \r\n");
			//goto exit_task;
		}

		if(setsockopt(ctl_blk->socket_cmd_client, IPPROTO_TCP, TCP_KEEPINTVL, &keep_intvl, sizeof(keep_intvl)) < 0 )
		{
			DBG_PRINT("TC set TCP_KEEPINTVL err! \r\n");
			//goto exit_task;
		}

		if(setsockopt(ctl_blk->socket_cmd_client, IPPROTO_TCP, TCP_KEEPCNT, &keep_cnt, sizeof(keep_cnt)) < 0 )
		{
			DBG_PRINT("TC set TCP_KEEPCNT err! ");
			//goto exit_task;
		}
		DBG_PRINT("socket_cmd TCP Keep alive\r\n");
        while(rtsp_connection_get())
        {
            DBG_PRINT("Wait RTSP exit!\r\n");
            osDelay(10);
        }
        socket_cmd_status = 1;
		#if 1
		if(setsockopt(ctl_blk->socket_cmd_server, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) < 0 )
		{
			DBG_PRINT("TCP_NODELAY fail\r\n");
		}
		#endif

		#if GP_SOCK_TCP_BEHAVIOUR
		enable = 3000;		// timeout ticks
		if(setsockopt(ctl_blk->socket_cmd_client, SOL_SOCKET, SO_RCVTIMEO, &enable, sizeof(enable)) < 0 )
		{
			DBG_PRINT("SO_RCVTIMEO error!\r\n");
		}
		#endif

        force_close = state = cnt = 0;
		/* Process the new connection. */
		if (ctl_blk->socket_cmd_client>=0)
		{
			//struct netbuf *buf;
			//void *data;
			INT16S rxlen;
			INT16U txlen;

			#if C_WIFI_ON_OFF_KEY == 0
			#if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_ON))
			while(gwifi_transmission_stop == 1)
			{
			    if(gwifi_transmission_stop == 1)
				    osDelay(5);
				else
					break;
			}
			#endif

			state_wifi_connect();
			#endif

			while (1)
			{
				rxlen = recv(ctl_blk->socket_cmd_client, (void *)rx_buf, TCP_RX_BUF_MAX_LEN, 0);
				if(rxlen > 0)
				{
					INT32U ret;
					do
					{
						//DBG_PRINT("gp cmd: rxlen %d\r\n", rxlen);

						if(strncmp((const char*)rx_buf, "GPSOCKET", 8) != 0)
						{
							DBG_PRINT("gp cmd(%d): skip cmd 0x%x 0x%x 0x%x 0x%x\r\n",rxlen,rx_buf[0],rx_buf[1],rx_buf[2],rx_buf[3]);
							break;
						}

						//GP_SOCK_MODE = (INT16S)(rx_buf[GP_SOCK_CMD_MODE_ID_OFFSET]);
						//GP_SOCK_ID   = (INT16S)(rx_buf[GP_SOCK_CMD_CMD_ID_OFFSET]);
						//GP_SOCK_Cmd  = (GP_SOCK_MODE<<8)|GP_SOCK_ID;
						txlen = 0;

						gp_cmd_process((void *)(&socket_socket_ctl_blk), (INT8U *)&rx_buf, rxlen,(INT8U *)&tx_buf, &txlen);
						if (force_close==0)
						{
                            //DBG_PRINT("gpcmd response: txlen %d\r\nACK:%x CMD:%x\r\n",txlen,tx_buf[8],tx_buf[11]);
                            ret = send(ctl_blk->socket_cmd_client, (void*)tx_buf, txlen, 0);
                            gp_memset((INT8S *)rx_buf, 0, rxlen);
                            if(ret < 0) {
                                DBG_PRINT("gp cmd: send fail, err %d\r\n", ret);
                            }
						}
						else
                        {
                            DBG_PRINT("froce close0\r\n");
                        }

					} while(0);
				}
				#if (WIFI_VENDOR == 1) // For ssv
				#else
				else if ( errno==EAGAIN )
				{
					DBG_PRINT("Try Again\r\n");
				}
				#endif
				#if (WIFI_VENDOR == 1) // For ssv
				else if ( (rxlen == -1) /*&&(errno==EAGAIN)*/ )	//timeout
				#else // For rtk
				else if(errno == ETIMEDOUT)	//timeout
				#endif
				{
					//#if C_WIFI_ON_OFF_KEY
					#if (WIFI_VENDOR == 2)
					DBG_PRINT("@@state_wifi_status_get %d ,%d\r\n",state_wifi_status_get(),rtk_wifi_module_conn_status());
					#elif (WIFI_VENDOR == 1)
					DBG_PRINT("@@state_wifi_status_get %d ,%d\r\n",state_wifi_status_get(),ssv_wifi_module_conn_status());
					#endif

					//if((state_wifi_status_get() & FLAG_CONNECT) == 0)
					#if (WIFI_VENDOR == 2)
					if(rtk_wifi_module_conn_status()!=2)
					#elif (WIFI_VENDOR == 1)
					if(ssv_wifi_module_conn_status()!=2)
					#endif

					break;
					//#endif

					#if 0
					DBG_PRINT("gp cmd: socket_cmd_recv timeout\r\n");
					#endif
				}
				else  // rxlen = 0 => close
				{
					#if (WIFI_VENDOR == 1) // For ssv
					DBG_PRINT("gp cmd: socket_cmd_recv len=%d\r\n", rxlen);
					#else // For rtk
					DBG_PRINT("gp cmd: socket_cmd_recv len=%d errno=%d\r\n", rxlen, errno);
					#endif
					break;
				}

				{	// In RECORD_MODE or CAPTURE_MODE, if TCP (PORT8080) is disconnected, GoplusCam will return to the initial state.
					switch(state)
					{
						case 1:
							if ( (wifi_ref_state==BROWSE_MODE)||(wifi_ref_state==FALSE_MENU_MODE) ) {
								state = 0; cnt = 0;
							}
							else if ( (wifi_ref_state==RECORD_MODE)||((wifi_ref_state==CAPTURE_MODE)) )
							{
								if (wifi_in_menu==1){
									state = 0; cnt = 0;
								}
								else if ( (rtsp_connection_get()==0)&&(wifi_in_menu==0) )
								{
									cnt++;  // for menu mode exit
									if (cnt > 8)  // 0.5 sec once
									{
										force_close = 1;
										state = 0; cnt = 0;
										DBG_PRINT("PORT8080 abnoraml\r\n");
									}
									else {
										DBG_PRINT("cnt=%d\r\n",cnt);
									}
								}
							}
							break;
						case 0:
							state = rtsp_connection_get();
							cnt = 0;
							break;
					}
				}

				if (force_close>0) {
					DBG_PRINT("froce close\r\n");
					break;
				}
			}

			DBG_PRINT("gp cmd: 0x%x client netconn close.\r\n",ctl_blk->socket_cmd_client);
			#if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_ON))
			gp_mode_set(RECORD_MODE);
			#else
			if(gp_mode_get() == BROWSE_MODE)
			{
				GPSOCK_Playback_Stop_Decode(0, 0, 0);
				gp_mode_set(wifi_mode_last);
			}
			#endif

			/* Close connected client socket then wait for new client connection */
			if(lwip_close(ctl_blk->socket_cmd_client) != 0)
			{
				DBG_PRINT("close socket_cmd client error\r\n");
			}

			#if 0 // Option
			// Stop GoPlusCam recording, when GoPlusCam disconnt port 8081
			if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)
			{
				ap_video_record_func_key_active();
			}
			#endif

			#if C_WIFI_ON_OFF_KEY == 0
			{
				INT8U type = KEY_MODE_NORMAL;
				peripheral_task_msg_send(MSG_PERIPHERAL_TASK_KEY_REGISTER, &type, sizeof(INT8U), MSG_PRI_NORMAL);
			}
			//wifi_transmission_stop();
            //rtsp_h264_stop_service();

			//while(1)
			//{
                //if(rtsp_connection_get() == 0)
                {
                    state_wifi_disconnect();
                    //break;
                }
                //osDelay(10);
           // }
			#endif

			ctl_blk->socket_cmd_client = -1;
			socket_cmd_status = 0;
			//wifi_transmission_stop();
			rtsp_h264_stop_service();

			DBG_PRINT("Exit socket_cmd client socket.\r\n");
		}
		else
		{
			DBG_PRINT("gp cmd: socket accept fail, netconn 0x%x 0x%x\r\n",ctl_blk->socket_cmd_server, ctl_blk->socket_cmd_client);
		}

	}

SOCKET_CMD_TASK_END:
	DBG_PRINT("\x1b[;32msocket_cmd_task Thread EXIT\x1b[0m\r\n");
	osThreadTerminate(NULL);
}

void socket_cmd_service_init(INT32U taskPriority, INT32U stackSize)
{
	sys_thread_new("socket_cmd_service", socket_cmd_task, NULL, stackSize, taskPriority);
}

#endif //#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)

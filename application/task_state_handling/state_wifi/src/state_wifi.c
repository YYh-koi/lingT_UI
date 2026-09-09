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
#include "ap_usb.h"

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/
extern MSG_Q_ID UVCTaskQ;
extern MSG_Q_ID UVCTaskAckQ;
extern INT32U   usb_device_is_connected_to_pc(void);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT8S wifi_status = FLAG_DISCONNECT;
QueueHandle_t wifiMsgAck;



/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
sw_wifi_ack_init(
	void
)
{
	wifiMsgAck = xQueueCreate(1, sizeof(INT32S));
	if(!wifiMsgAck)
	{
		DBG_PRINT("[ERR] wifiMsgAck is null!\r\n");
	}
}

void
sw_wifi_ack_send(
	INT8U retResult
)
{
	INT32U  retAck;

	retAck = retResult;
	if(wifiMsgAck)
	{
		xQueueSend(wifiMsgAck, (void*)&retAck, 0);
	}
}

INT16U
wifi_display_height_get(
	void
)
{
	return WIFI_HEIGHT;
}

INT16U
wifi_display_width_get(
	void
)
{
	return WIFI_WIDTH;
}

INT16U
wifi_display_pip_height_get(
	void
)
{
	return WIFI_PIP_HEIGHT;
}

INT16U
wifi_display_pip_width_get(
	void
)
{
	return WIFI_PIP_WIDTH;
}

DISP_FMT
wifi_display_format_get(
	void
)
{
	return DISP_FMT_GP420;
}

INT32U
wifi_encode_type_get(
	void
)
{
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	return WIFI_STREAM_FORMAT;
#else
    return STATUS_OK;
#endif
}

INT32U wifi_display_buf_size_get(void)
{
	INT16U wifi_w = wifi_display_width_get();
	INT16U wifi_h = wifi_display_height_get();
    INT32U size;

	if(wifi_display_format_get() == DISP_FMT_GP420)
	{
		size =((wifi_w * (1 + wifi_h)) * 3) >> 1;
	}
	else
	{
		size =(wifi_w * (1 + wifi_h)) << 1;
	}

    size += 0x003f;
    size &= ~0x003f;    //64Byte alignment
	return size;

}

void
state_wifi_status_set(
	INT8S state_flag
)
{
	if(state_flag > 0)
	{
		wifi_status |= state_flag;
	}
	else if(state_flag == 0)
	{
		wifi_status = 0;
	}
	else
	{
		wifi_status &= state_flag;
	}
}

INT8U
state_wifi_status_get(
	void
)
{
	return wifi_status;
}

INT32S
state_wifi_connect(void)
{
	INT32U msg_ack;
	#if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_ON))
	INT32U usbd_connected_status=0;
	INT32U usbd_io_status=0;
	#endif
	INT32S	nRet = STATUS_OK;



	state_wifi_status_set(FLAG_CONNECT);

    xQueueReset(wifiMsgAck);
	DBG_PRINT("333\r\n");
	app_msg_send(MSG_APQ_WIFI_CONNECT, NULL, 0, MSG_PRI_NORMAL);
	xQueueReceive(wifiMsgAck, (void*)&msg_ack, portMAX_DELAY);

	state_wifi_status_set(FLAG_OFF_DISPLAY);

    #if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_ON))
	usbd_connected_status =usb_device_is_connected_to_pc();
    usbd_io_status =ap_peripheral_get_usbd_io_status();
    DBG_PRINT("usbd_connected_status=%d\r\n",usbd_connected_status);
	if((state_usbd_status_get() == 1)&& usbd_connected_status && usbd_io_status)
	{
	    msgQFlush(UVCTaskAckQ);
	    POST_MESSAGE(UVCTaskQ, USBD_STOP_VIDEO_EVENT, UVCTaskAckQ, 1000, MSG_PRI_NORMAL);
	}
    #endif
#if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_ON))
Return:
#endif
	if(nRet == STATUS_FAIL)
	{
		ERR_PRINT("fail to stop uvc\r\n");
	}
	return nRet;

}

INT32S
state_wifi_disconnect(
	void
)
{
	INT32U msg_ack;
	#if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_ON))
    INT32U usbd_connected_status=0;
	INT32U usbd_io_status=0;
    #endif
    xQueueReset(wifiMsgAck);
	app_msg_send(MSG_APQ_WIFI_DISCONNECT, NULL, 0, MSG_PRI_NORMAL);
	xQueueReceive(wifiMsgAck, (void*)&msg_ack, portMAX_DELAY);
	#if(defined(C_CARCAM_UVC_DISPLAY) && (C_CARCAM_UVC_DISPLAY == CUSTOM_ON))
	usbd_connected_status =usb_device_is_connected_to_pc();
    usbd_io_status =ap_peripheral_get_usbd_io_status();
    DBG_PRINT("usbd_connected_status=%d\r\n",usbd_connected_status);
	if((state_usbd_status_get() == 0)&& usbd_connected_status && usbd_io_status)
	{
	    msgQSend(UVCTaskQ, USBD_START_VIDEO_EVENT, NULL, 0, MSG_PRI_NORMAL);
	}
	#endif
    return 0;
}

INT32S
state_wifi_symbol_show(
	INT8U EnDisable
)
{
#if C_DISABLE_SENSOR_TFT
	return STATUS_OK;
#endif
    #if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
    STRING_ASCII_INFO ascii_str = {0};
    t_STR_SIZE_STRUCT str_res;
	INT16U	tft_w = drv_l2_display_tft_width_get();
	INT16U	tft_h = drv_l2_display_tft_height_get();
    DISP_P  display_frame;
    INT32U  display_buf;
    static INT8U  wifi_ssid[GPSOCK_WiFi_Name_Length];
    static INT8U  wifi_ssid_len = 0;
    static INT8U  wifi_pw[GPSOCK_WiFi_Passwd_Length];
    static INT8U  wifi_pw_len	 = 0;
    if(EnDisable)
    {
        ap_state_handling_warn_str_icon_exit();     //disable warning icon display

        display_frame = ap_display_queue_get(display_free_queue);
        while(display_frame == NULL)
        {
            osDelay(1);
            display_frame = ap_display_queue_get(display_free_queue);
        }

        display_frame->offset = 0;
        display_buf = display_frame->base + display_frame->offset;

        ap_display_clear_screen(display_buf);

  		ap_state_wifi_ssid_get    ((INT8U *)&wifi_ssid, &wifi_ssid_len);
		ap_state_wifi_password_get((INT8U *)&wifi_pw  , &wifi_pw_len);

        ascii_str.font_color = WHITE_COLOR;
        ascii_str.font_type = 0;
        ascii_str.str_ptr = (CHAR *)wifi_ssid;//"WIFI CONNECTED";
        ap_state_resource_ascii_string_resolution_get(&ascii_str, &str_res);

        ascii_str.pos_x = (tft_w - str_res.string_width) >> 1;
        ascii_str.pos_y = (tft_h - str_res.string_height)/3;
        ascii_str.pos_x &= ~0x0001;
        ascii_str.pos_y &= ~0x0001;
        ascii_str.buff_w = tft_w;
        ascii_str.buff_h = tft_h;
        ap_state_resource_string_ascii_draw((INT16U *)display_buf, &ascii_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);


        ascii_str.font_color = WHITE_COLOR;
        ascii_str.font_type = 0;
        ascii_str.str_ptr = (CHAR *)wifi_pw;//"WIFI CONNECTED";
        ap_state_resource_ascii_string_resolution_get(&ascii_str, &str_res);

        ascii_str.pos_x = (tft_w - str_res.string_width) >> 1;
        ascii_str.pos_y = ((tft_h - str_res.string_height)/3)*2;
        ascii_str.pos_x &= ~0x0001;
        ascii_str.pos_y &= ~0x0001;
        ascii_str.buff_w = tft_w;
        ascii_str.buff_h = tft_h;
        ap_state_resource_string_ascii_draw((INT16U *)display_buf, &ascii_str, 0, DRAW_BLACK_EDGE_ON, DRAW_UV_ON);

        cache_invalid_range((INT32U)display_buf, getDispDevBufSize());

        if(display_task_msg_send(MSG_DISPLAY_TASK_JPEG_DRAW, &display_frame, sizeof(void *), MSG_PRI_NORMAL) == STATUS_FAIL)
        {
		    if(ap_display_queue_put(display_free_queue, display_frame) != STATUS_OK)
		    {
		        DBG_PRINT("[ERROR]display_free_queue full\r\n");
		    }
        }
    }
    #endif
    return  STATUS_OK;
}


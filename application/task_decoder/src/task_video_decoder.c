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
#include "application.h"
#include "circular_buffer.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define BROWSE_VID_DEC_STACK_SIZE		2048
#define BROWSE_VID_DEC_TASK_Q_MAX		32
#define BROWSE_VID_DEC_TASK_ACK_MAX		1

#define BROWSE_VID_BUFFER_ALIGN_MASK	0x3F

enum{
	VID_DEC_MJPG = 0,
	VID_DEC_H264,
	VID_DEC_MAX
};

#define USE_JPEG_NUM		JPEGA

#define PARSER_DROP					0x00
#define DECODE_DROP					0x01
#define DROP_MENTHOD				PARSER_DROP


#define C_H264ENC_BITRATE_WIFI        0x180000
#define C_H264ENC_FRAMERATE_WIFI      25
#define C_H264ENC_GOP_LEN			  20

#define CIRCULAR_BUFFER_SIZE 			10

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct BROWSE_VID_DEC_CTRL_S{
	INT32S	(*vid_dec_get_info_func)(INT32U raw_data, INT32U raw_size, INT16U* ret_width, INT16U* ret_height);
	INT32S	(*vid_dec_decode_and_scale_func)(void* vid_dec_ctrl,INT32U raw_data, INT32U raw_size, INT16U raw_width, INT16U raw_height);
	INT32S	(*vid_dec_decode_one_frame_func)(INT32U raw_data,INT32U raw_size, INT16U raw_width, INT16U raw_height);
	INT32U	get_first_out_buf;
	INT8U   flag_get_first;
}BROWSE_VID_DEC_CTRL_T;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

extern INT32U wifi_playback_tick_get(INT32S src_id);

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
INT32S browse_dec_JPEG_info_get(INT32U raw_data,INT32U raw_size,INT16U* ret_width,INT16U* ret_height);
INT32S browse_dec_JPEG_and_scale(void* p,INT32U raw_data,INT32U raw_size,INT16U raw_width,INT16U raw_height);
INT32S browse_dec_JPEG_one_frame(INT32U raw_data,INT32U raw_size,INT16U raw_width,INT16U raw_height);


INT32S browse_dec_H264_info_get(INT32U raw_data,INT32U raw_size,INT16U* ret_width,INT16U* ret_height);
INT32S browse_dec_H264_and_scale(void* p,INT32U raw_data,INT32U raw_size,INT16U raw_width,INT16U raw_height);
INT32S browse_dec_H264_one_frame(INT32U raw_data,INT32U raw_size,INT16U raw_width,INT16U raw_height);


#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
INT32S browse_reencode_JPEG(INT32U disp_buf);
INT32S browse_reencode_H264(INT32U disp_buf);
#endif

void browse_dec_buf_release(void);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
/* os task queue */
MSG_Q_ID	vid_dec_q;
MSG_Q_ID	vid_dec_ack_m;

INT32U 	raw_data_addr = 0;
INT8U	nth_flag = 0;
long 	size,Duration;
INT64S 	delta_Tv = 0, old_delta_Tv=0;
INT16U  raw_width = 0, raw_height = 0;

static BROWSE_VID_DEC_CTRL_T Browse_Vid_Dec_Ctrl[VID_DEC_MAX] =
{
//VID_DEC_MJPG
	{
		browse_dec_JPEG_info_get,
		browse_dec_JPEG_and_scale,
		browse_dec_JPEG_one_frame
	}
	,
//VID_DEC_H264
	{
		browse_dec_H264_info_get,
		browse_dec_H264_and_scale,
		browse_dec_H264_one_frame
	}
};

BROWSE_VID_DEC_CTRL_T* pVid_Dec_Ctrl = NULL;

static void* h264_dec = NULL;
static INT8U flag_disp_where = DISP_ON_TFT;
static void* h264_enc = NULL;
INT32S FrmCnt = 0;

static INT32U* circular_buffer_storage = NULL;
static cbuf_handle_t cb_handle = NULL;
static INT16U jpeg_yuv_mode;
extern INT8U h264_Mb2scan_triger;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
INT32S
video_decode_task_one_frame(
	void
)
{
	osStatus err;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	err = msgQSend(vid_dec_q, MSG_VID_DEC_ONE_FRAME, NULL, 0, MSG_PRI_NORMAL);
	if(err != osOK) {
		ERR_PRINT("fail to msgQSend with vid_dec_q\r\n");
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

INT32S
video_decode_task_start(
	void
)
{
	INT32S	nRet = STATUS_OK;

	if((vid_dec_q == NULL) && (vid_dec_ack_m == NULL))
	{
		RETURN(STATUS_FAIL);
	}

	POST_MESSAGE(vid_dec_q, MSG_VID_DEC_START, vid_dec_ack_m, 5000, MSG_PRI_NORMAL);
Return:
	return nRet;
}

INT32S
video_decode_task_restart(
	void
)
{
	INT32S	nRet = STATUS_OK;

	if((vid_dec_q == NULL) && (vid_dec_ack_m == NULL))
	{
		RETURN(STATUS_FAIL);
	}

	POST_MESSAGE(vid_dec_q, MSG_VID_DEC_RESTART, vid_dec_ack_m, 5000, MSG_PRI_NORMAL);
Return:
	return nRet;
}

INT32S
video_decode_task_stop(
	void
)
{
	INT32S	nRet = STATUS_OK;

	if((vid_dec_q == NULL) && (vid_dec_ack_m == NULL))
	{
		RETURN(STATUS_FAIL);
	}

	POST_MESSAGE(vid_dec_q, MSG_VID_DEC_STOP, vid_dec_ack_m, 5000, MSG_PRI_NORMAL);
Return:
	return nRet;
}

INT32S
video_decode_task_nth_frame(
	void
)
{
	INT32S	nRet = STATUS_OK;

	if((vid_dec_q == NULL) && (vid_dec_ack_m == NULL))
	{
		RETURN(STATUS_FAIL);
	}

	POST_MESSAGE(vid_dec_q, MSG_VID_DEC_NTH, vid_dec_ack_m, 5000, MSG_PRI_NORMAL);

Return:
	return nRet;
}

INT32S
video_decode_task_display_notify(
	INT32U disp_addr
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if((vid_dec_q == NULL) && (vid_dec_ack_m == NULL))
	{
		RETURN(STATUS_FAIL);
	}

	if(cb_handle)
	{
		if(circular_buf_try_put(cb_handle, disp_addr) < 0)
		{
			ERR_PRINT("addr:%d\r\n", disp_addr);
			RETURN(STATUS_FAIL);
		}
	}

    msgQSend(vid_dec_q, MSG_VID_DEC_DISP, NULL, 0, MSG_PRI_NORMAL);
Return:
	return nRet;
}

static INT32S
video_decode_task_init(
	void
)
{
	INT32S nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	vid_dec_q = msgQCreate(BROWSE_VID_DEC_TASK_Q_MAX,BROWSE_VID_DEC_TASK_Q_MAX,0);
	if(vid_dec_q == NULL)
	{
		RETURN(STATUS_FAIL);
	}

	vid_dec_ack_m = msgQCreate(BROWSE_VID_DEC_TASK_ACK_MAX,BROWSE_VID_DEC_TASK_ACK_MAX,0);
	if(vid_dec_ack_m == NULL)
	{
		RETURN(STATUS_FAIL);
	}

	circular_buffer_storage = gp_malloc_align((CIRCULAR_BUFFER_SIZE*sizeof(INT32U)),64);
	if(circular_buffer_storage == NULL)
	{
		RETURN(STATUS_FAIL);
	}

	cb_handle = circular_buf_init(circular_buffer_storage, CIRCULAR_BUFFER_SIZE);
	if(cb_handle == NULL)
	{
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}

static INT32S
video_decode_task_uninit(
	void
)
{
	if(vid_dec_q)
	{
		msgQDelete(vid_dec_q);
		vid_dec_q = NULL;
	}

	if(vid_dec_ack_m)
	{
		msgQDelete(vid_dec_ack_m);
		vid_dec_ack_m = NULL;
	}

	if(circular_buffer_storage)
	{
		gp_free(circular_buffer_storage);
		circular_buffer_storage = NULL;
	}

	if(cb_handle)
	{
		circular_buf_free(cb_handle);
		cb_handle = NULL;
	}

	return STATUS_OK;
}

static void
return_ack(
	INT32S nRet
)
{
	INT32U ret_ack;

	if(nRet == STATUS_OK)
	{
		ret_ack = C_ACK_SUCCESS;
	}
	else
	{
		ret_ack = C_ACK_FAIL;
	}

	//DBG_PRINT("==Run:%s==ret_ack:%d==\r\n",__FUNCTION__,ret_ack);
    msgQSend(vid_dec_ack_m, ret_ack, NULL, 0, MSG_PRI_NORMAL);
}

INT32S
browse_vid_data_get(
	void
)
{
	INT32S 	error_id;
	INT32S	loop_cnt = 150;

	//fre-latch video raw data
	while(1)
	{
		raw_data_addr = (INT32U)MultiMediaParser_GetVidBuf(p_vid_dec_para->media_handle, &size, &Duration, &error_id);
		if (error_id < 0)
		{
			ERR_PRINT("error_id:%d \r\n",error_id);
			return STATUS_FAIL;
		}

		size += raw_data_addr & BROWSE_VID_BUFFER_ALIGN_MASK;	//align and size increase
		raw_data_addr -= raw_data_addr & BROWSE_VID_BUFFER_ALIGN_MASK;	//align addr
		if ((raw_data_addr + size) & BROWSE_VID_BUFFER_ALIGN_MASK) {	//align end address
			size += (BROWSE_VID_BUFFER_ALIGN_MASK + 1);
			size -= (raw_data_addr + size) & BROWSE_VID_BUFFER_ALIGN_MASK;
		}

		if(nth_flag)
		{
			if(raw_data_addr && (size >= 10))
			{
				break;
			}
		} else {
			if(raw_data_addr)
			{
				break;
			}
		}

		//check end or error
		if(vid_dec_get_status() & C_VIDEO_DECODE_ERR)
		{
			return STATUS_FAIL;
		}
		else if(MultiMediaParser_IsEOV(p_vid_dec_para->media_handle) == 1)
		{
			raw_data_addr = 0;
			break;
		}
		else
		{
			--loop_cnt;
			if(loop_cnt <= 0)
			{
				return STATUS_FAIL;
			}

			osDelay(1);
		}
	}

	return STATUS_OK;
}

INT32S
browse_vid_dec_start(
	INT8U 	get_first,
	INT32U  get_first_out_buf,
	INT16U*  width,
	INT16U*  height
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	delta_Tv = old_delta_Tv = 0;
	p_vid_dec_para->fail_cnt = 0;
	p_vid_dec_para->pend_cnt = p_vid_dec_para->post_cnt = 0;

	nRet = browse_vid_data_get();
	if(nRet == STATUS_FAIL)
	{
		return STATUS_FAIL;
	}

	if(p_wave_info)
	{
		delta_Tv = (INT64S)Duration * p_wave_info->nSamplesPerSec*2;
	} else {
		delta_Tv = (INT64S)Duration * TIME_BASE_TICK_RATE;
	}

	switch(p_vid_dec_para->video_format)
	{
		case C_MJPG_FORMAT:
			pVid_Dec_Ctrl = &Browse_Vid_Dec_Ctrl[VID_DEC_MJPG];
		break;
        case C_H264_FORMAT:
			pVid_Dec_Ctrl = &Browse_Vid_Dec_Ctrl[VID_DEC_H264];
		break;
		default:
			ERR_PRINT("The format[0x%x] is not supported \r\n",p_vid_dec_para->video_format);
		return STATUS_FAIL;
	}

	pVid_Dec_Ctrl->flag_get_first = get_first;
	pVid_Dec_Ctrl->get_first_out_buf = get_first_out_buf;

	pVid_Dec_Ctrl->vid_dec_get_info_func(raw_data_addr, size, &raw_width, &raw_height);
    if(width != NULL)
    {
        *width = raw_width;
        *height = raw_height;
    }
	if (raw_width && raw_height)
	{
		pVid_Dec_Ctrl->vid_dec_decode_and_scale_func(pVid_Dec_Ctrl,raw_data_addr, size, raw_width, raw_height);
	}
	else
	{
		ERR_PRINT("Can't get raw width/height \r\n");
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

INT32S
browse_vid_dec_stop(
	void
)
{
	//INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(C_H264_FORMAT == p_vid_dec_para->video_format)
	{
		if(h264_dec)
		{
			gp_h264dec_release(h264_dec);
			h264_dec = NULL;
		}

		vid_dec_h264_pscaler_stop();
	}
	if(C_MJPG_FORMAT == p_vid_dec_para->video_format)
	{
	    drv_l2_scaler_stop(SCALER_0); // mjpg 是搭配一般scaler 使用
	}

	browse_dec_buf_release();

	msgQFlush(vid_dec_q);

	return STATUS_OK;
}

INT32S
vid_dec_reencode_func(
	INT32U disp_buf
)
{
	#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
    extern int rtsp_method_type_get(void);

	if(rtsp_method_type_get())
	{
		browse_reencode_JPEG(disp_buf);
	}
	else
	{
		browse_reencode_H264(disp_buf);
	}
    #endif
	return STATUS_OK;
}

void
browse_vid_dec_display_process(
	void
)
{
	INT32U disp_buf;

	if(circular_buf_get(cb_handle,((INT32U*)&disp_buf)) < 0)
	{
		ERR_PRINT("fail to get data from cb_handle \r\n");
		return ;
	}

	if(flag_disp_where == DISP_ON_TFT)
	{
		DISP_P frame = displayaddr_to_displayframe(disp_buf);
		if(!frame)
		{
			return;
		}

		if(display_task_msg_send(MSG_DISPLAY_TASK_MJPEG_DRAW, &frame, sizeof(DISP_P), MSG_PRI_NORMAL) != osOK)
		{
			ERR_PRINT("fail to display_task_msg_send\r\n");
		}
	}
	else // DISP_ON_WIFI
	{
		if(pVid_Dec_Ctrl == NULL)
		{
			ERR_PRINT("pVid_Dec_Ctrl is null\r\n");
			return;
		}

		vid_dec_reencode_func(disp_buf);
	}

}

INT32S
browse_vid_dec_one_frame(
	void
)
{
	INT32S	nRet = STATUS_OK;
	INT8U 	end_flag,success_flag = 0;
	INT64S	temp;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(pVid_Dec_Ctrl == NULL)
	{
		return STATUS_FAIL;
	}

	//check video data ready
	if(!raw_data_addr || size <= 0) {
		//check end or error
		if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) {
			end_flag = 1;
		} else if(MultiMediaParser_IsEOV(p_vid_dec_para->media_handle) == 1) {
			end_flag = 1;
		} else {
			end_flag = 0;
		}

		if(end_flag) {
			DBG_PRINT("VidDecEnd.\r\n");
			//---快進切到快退 或 快退 時 , 在影片結束後 , display_ready_queue 有一禎 畫面有問題的禎 , 原因 unknown
			//--- work around 方式 , return to display_free_queue
			if(Speed_reverse_flag)
				ap_display_queue_put(display_free_queue, ap_display_queue_get(display_ready_queue));
			browse_vid_dec_stop();
			msgQFlush(vid_dec_q);
			vid_dec_end_callback(C_VIDEO_DECODE_PLAYING);
			return STATUS_OK;
		} else {
			goto __VidDecOneFrameEnd;
		}
	}

	//check sync
	taskENTER_CRITICAL();
	temp = (INT64S)(p_vid_dec_para->tv - p_vid_dec_para->Tv);
	taskEXIT_CRITICAL();
	if(temp > p_vid_dec_para->time_range) {
		if (p_vid_dec_para->video_format == C_H264_FORMAT)
			p_vid_dec_para->fail_cnt = 1;
		else
	#if DROP_MENTHOD == PARSER_DROP
		if(p_vid_dec_para->fail_cnt < 2) {
			p_vid_dec_para->fail_cnt++;
			MultiMediaParser_SetFrameDropLevel(p_vid_dec_para->media_handle, p_vid_dec_para->fail_cnt);
			DBG_PRINT("FailCnt = %d\r\n", p_vid_dec_para->fail_cnt);
		}
	#elif DROP_MENTHOD == DECODE_DROP
		p_vid_dec_para->fail_cnt = 1;
		DBG_PRINT("FailCnt = 1\r\n");
	#endif
	} else {
		if (p_vid_dec_para->video_format == C_H264_FORMAT)
			p_vid_dec_para->fail_cnt = 0;
		else
	#if DROP_MENTHOD == PARSER_DROP
		if(p_vid_dec_para->fail_cnt > 0) {
			p_vid_dec_para->fail_cnt--;
			MultiMediaParser_SetFrameDropLevel(p_vid_dec_para->media_handle, p_vid_dec_para->fail_cnt);
			DBG_PRINT("FailCnt = %d\r\n", p_vid_dec_para->fail_cnt);
		}
	#elif DROP_MENTHOD == DECODE_DROP
		p_vid_dec_para->fail_cnt = 0;
	#endif
	}

	if (p_vid_dec_para->video_format == C_MJPG_FORMAT)
	{
		#if DROP_MENTHOD == DECODE_DROP
			if(p_vid_dec_para->fail_cnt) {
				goto __VidDecOneFrameEnd;
			}
		#endif
	}

    if(size>192)
	  pVid_Dec_Ctrl->vid_dec_decode_one_frame_func(raw_data_addr,size,raw_width,raw_height);

__VidDecOneFrameEnd:
	success_flag = 1;
	if(success_flag) {
		//add video decode time
		if(p_vid_dec_para->scaler_flag && (p_vid_dec_para->video_format != C_MJPG_FORMAT)) {
			taskENTER_CRITICAL();
			p_vid_dec_para->Tv += old_delta_Tv;
			taskEXIT_CRITICAL();
			old_delta_Tv = delta_Tv;
		} else {
			taskENTER_CRITICAL();
			p_vid_dec_para->Tv += delta_Tv;
			taskEXIT_CRITICAL();
		}
	}

	//allow next frame input
	p_vid_dec_para->pend_cnt++;
	taskENTER_CRITICAL();
	temp = p_vid_dec_para->tv - p_vid_dec_para->Tv;
	if((temp >= 0) && (p_vid_dec_para->post_cnt == p_vid_dec_para->pend_cnt) && (raw_data_addr)) {
		if (video_decode_task_one_frame() == STATUS_OK) {
			p_vid_dec_para->post_cnt++;
		}
	}
	taskEXIT_CRITICAL();

	// get video raw data
	nRet = browse_vid_data_get();
	if(nRet == STATUS_FAIL)
	{
		return STATUS_FAIL;
	}

	//update current time & frame
	p_vid_dec_para->video_cur_time = MultiMediaParser_GetVidCurTime(p_vid_dec_para->media_handle) >> 8;
	if (p_vid_dec_para->FileType == FILE_TYPE_AVI) {
		p_vid_dec_para->video_cur_frame = MultiMediaParser_GetVidCurNumberOfVideo(p_vid_dec_para->media_handle);
	}

	//caculate video frame delta time
	if(p_wave_info) {
			delta_Tv = (INT64S)Duration * p_wave_info->nSamplesPerSec*2;
	} else {
		p_vid_dec_para->n = 1;
		delta_Tv = (INT64S)Duration * TIME_BASE_TICK_RATE;
	}

	#if 0 // Debug
	if (delta_Tv)
	{
    	DBG_PRINT("parse one frame [%lld] (tv = %lld, TV = %lld)\r\n", delta_Tv, p_vid_dec_para->tv, p_vid_dec_para->Tv);
	}
	#endif

	return STATUS_OK;
}

INT32S
browse_vid_dec_task_exit(
	INT8U* pExit_flag
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	*pExit_flag = 0;

	return STATUS_OK;
}

static void
browse_vid_dec_task_entry(
	void const *args
)
{
	INT32U msg_id;
	INT8U  flag_task_exit = 1;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	video_decode_task_init();

	while(flag_task_exit)
	{
		if(msgQReceive(vid_dec_q, &msg_id, NULL, 0) == STATUS_FAIL)
		{
			continue;
		}

		//DBG_PRINT("==Run:%s==msg_id:0x%x==\r\n",__FUNCTION__,msg_id);

		switch(msg_id)
		{
			case MSG_VID_DEC_ONE_FRAME:
				browse_vid_dec_one_frame();
			break;
			case MSG_VID_DEC_START:
				return_ack(browse_vid_dec_start(0,0,NULL,NULL));
			break;
			case MSG_VID_DEC_RESTART:
				return_ack(STATUS_OK);
			break;
			case MSG_VID_DEC_STOP:
				return_ack(browse_vid_dec_stop());
			break;
			case MSG_VID_DEC_NTH:
				return_ack(STATUS_OK);
			break;
			case MSG_VID_DEC_DISP:
				browse_vid_dec_display_process();
			break;
			case MSG_VID_DEC_EXIT:
				return_ack(browse_vid_dec_task_exit(&flag_task_exit));
			break;
		}
	}

	//DBG_PRINT("Exit %s\r\n",__FUNCTION__);

	video_decode_task_uninit();

	osThreadTerminate(NULL);
}

INT32S
video_decode_task_create(
	void
)
{
	INT32S nRet = STATUS_OK;
	osThreadId id;
	osThreadDef_t browse_vid_dec_task = {"browse_vid_dec_task", browse_vid_dec_task_entry, PRIORITY_BROWSE_VID_DEC, 1, BROWSE_VID_DEC_STACK_SIZE};

	id = osThreadCreate(&browse_vid_dec_task, NULL);
	if(id == 0)
	{
		RETURN(STATUS_FAIL);
	}

Return:
	return nRet;
}

INT32S
video_decode_task_del(
	void
)
{
	INT32S	nRet = STATUS_OK;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if((vid_dec_q == NULL) && (vid_dec_ack_m == NULL))
	{
		RETURN(STATUS_FAIL);
	}

	POST_MESSAGE(vid_dec_q, MSG_VID_DEC_EXIT, vid_dec_ack_m, 5000, MSG_PRI_NORMAL);

Return:
	return nRet;
}

/*=== Browse_Vid_Dec_Ctrl ===*/
INT32U
browse_dec_get_buf(
	void
)
{
	DISP_P frame = NULL;
	INT32U addr;

	if(flag_disp_where == DISP_ON_TFT)
	{
		frame = ap_display_queue_get(display_free_queue);
		if(!frame)
		{
			return DUMMY_BUFFER_ADDRS;
		}

		//DBG_PRINT("frame:0x%x base:0x%x\r\n",frame,frame->base);
		#if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
		return (INT32U)frame->base+video_decode_display_offset()+video_decode_display_x_offset_get();
		#else
		return (INT32U)frame->base+video_decode_display_offset();
		#endif
	}
	else
	{
		addr = vid_dec_get_next_scaler_buffer();
		//DBG_PRINT("addr:0x%x\r\n",addr);
		return addr;
	}
}

void
browse_dec_buf_release(
	void
)
{
	/*
		WIFI mode buffer use malloc, only release TFT mode buffer
	*/

	INT32U residue;
	INT32U disp_buf;
	DISP_P frame;
	INT8U i;

	if(flag_disp_where == DISP_ON_TFT)
	{
		residue = circular_buf_size(cb_handle);
		if(residue > 0)
		{
			for(i=0; i<residue; i++)
			{
				if(circular_buf_get(cb_handle,((INT32U*)&disp_buf)) < 0)
				{
					ERR_PRINT("fail to get data from cb_handle \r\n");
					continue;
				}

				frame = displayaddr_to_displayframe(disp_buf);
				if(!frame)
				{
					continue;
				}

				if(ap_display_queue_put(display_free_queue, frame) != STATUS_OK)
				{
					DBG_PRINT("[ERROR]display_free_queue full\r\n");
				}
			}
		}

		/*
			0: Pscaler A buffer
			1: Pscaler B buffer

			Check buffer in Pscaler and put it back to display_free_queue
			In get first mode, code send pscaler buffer to display task then it will put it back to display_free_queue
		*/
		if(pVid_Dec_Ctrl->flag_get_first == 0)
		{
		//---dec buf release時 , 只有在h264時 , 才需要將pscaler 的buf 丟回display free Q
		//---如果是mjpg , 上面那個動作就全都還回去display free Q
	      if(p_vid_dec_para->video_format==C_H264_FORMAT)
	      {
			for(i=0; i<2; i++)
			{
				#if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
				frame = displayaddr_to_displayframe(vid_dec_h264_pscaler_buf_get(i)-video_decode_display_offset()-video_decode_display_x_offset_get());
				#else
				frame = displayaddr_to_displayframe(vid_dec_h264_pscaler_buf_get(i)-video_decode_display_offset());
				#endif
				if(frame)
				{
					if(ap_display_queue_put(display_free_queue, frame) != STATUS_OK)
					{
						DBG_PRINT("[ERROR]display_free_queue full\r\n");
					}
				}
			}
          }

		}
	}
	else
	{
 		if(cb_handle)
 		{
            circular_buf_reset(cb_handle);
        }
	}
}

/*
 Decoding a JPEG Image using JPEG_A
 Re-encoding a JPEG Image using JPEG_B
*/
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
void browse_rtsp_jpeg_send_callback_done (INT32U stream_addr, INT32U stream_type){
	if(circular_buf_try_put((cbuf_handle_t)p_vid_dec_para->video_decode_cb_handle, stream_addr) < 0){
		ERR_PRINT("addr:0x%x\r\n", stream_addr);
	}
}

INT32S
browse_reencode_JPEG(
	INT32U disp_buf
)
{
	INT32S nRet;
	JpegEnc_t JpegEnc = {0};
	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	INT32U jpeg_enc_out_buf = vid_dec_get_next_vid_buffer();
	INT32U jpeg_re_enc_buf_size = p_vid_dec_para->buffer_output_width * p_vid_dec_para->buffer_output_height * 2;
	if(jpeg_enc_out_buf == DUMMY_BUFFER_ADDRS){
        if(disp_buf)
        {
            if(circular_buf_try_put((cbuf_handle_t)p_vid_dec_para->scaler_output_cb_handle, disp_buf) < 0){
                nRet = STATUS_FAIL;
                ERR_PRINT("addr:0x%x\r\n", disp_buf);
            }
        }
		return STATUS_FAIL;
	}
	JpegEnc.hwidx = JPEGB;
	JpegEnc.quality_value = 50;
	JpegEnc.yuv_fmt = VID_DATA_420;
	JpegEnc.yuv_sample_fmt = JPEG_YUV420;
	JpegEnc.width = p_vid_dec_para->buffer_output_width;
	JpegEnc.height = p_vid_dec_para->buffer_output_height;
	JpegEnc.input_buffer_y = disp_buf;
	JpegEnc.output_buffer = jpeg_enc_out_buf;
	JpegEnc.output_buffer_len = jpeg_re_enc_buf_size;

	cache_invalid_range(jpeg_enc_out_buf, jpeg_re_enc_buf_size);

	nRet = jpeg_buffer_encode(&JpegEnc);

	if(circular_buf_try_put((cbuf_handle_t)p_vid_dec_para->scaler_output_cb_handle, disp_buf) < 0){
		nRet = STATUS_FAIL;
		ERR_PRINT("addr:0x%x\r\n", disp_buf);
	}

	if(nRet == STATUS_FAIL)
	{
		if(circular_buf_try_put((cbuf_handle_t)p_vid_dec_para->video_decode_cb_handle, jpeg_enc_out_buf) < 0){
			ERR_PRINT("addr:0x%x\r\n", jpeg_enc_out_buf);
		}
		return STATUS_FAIL;
	}

	return rtsp_jpeg_send(jpeg_enc_out_buf, JpegEnc.encode_size, wifi_playback_tick_get(0), JpegEnc.width, JpegEnc.height, browse_rtsp_jpeg_send_callback_done);
}

static void*
OpenH264(
	int width,
	int height
)
{
	gp_h264_cfg cfg;
	gp_h264enc_rate_ctrl rccfg;
	void* handle;

	FrmCnt = 0;

	cfg.width = width;
	cfg.height = height;
	cfg.level = 31;
	cfg.fbc = 0;

	handle = gp_h264enc_init(&cfg);

	if (!handle)
	return NULL;

	rccfg.bitPerSeconds = C_H264ENC_BITRATE_WIFI;
	rccfg.rc_mode = H264_RC_ABR;
	rccfg.frameRate = C_H264ENC_FRAMERATE_WIFI;
	rccfg.gopLen = C_H264ENC_GOP_LEN;
	rccfg.qpHdr = 32;
	rccfg.qpMax = 51;
	rccfg.qpMin = 20;
	rccfg.hierPFrames = 1;
	rccfg.qualityLevel = 34;

	gp_h264enc_set_rc(handle, &rccfg);

  return handle;
}
#endif

INT32S
EncodeH264Frame(
	INT32U frameAddr,
	INT32U outAddr,
	INT32U max_out_size,
	INT32U PictureType,
	INT32S insertHdr,
	INT32U NullFrame,
	INT32S* pStreamSize
)
{
    gp_h264enc_in in;
    INT32S status;
    INT32U buffer_size;
    void* h264_handle = h264_enc;

    if (!h264_handle || !pStreamSize)
        return STATUS_FAIL;

    in.RawFrameAddr = frameAddr;
    in.BitstreamBaseAddr = outAddr;
    in.BitstreamOutAddr = outAddr;
    in.BitstreamEndAddr = outAddr + max_out_size;
    in.PictureType = PictureType;
    in.InsertHdr = insertHdr;
    in.input_fifo = 0;
    if (p_vid_dec_para->video_decode_out_format == C_SCALER_CTRL_OUT_GP420) {
		in.format = H264_FMT_GP420;
		buffer_size = ((p_vid_dec_para->buffer_output_width*p_vid_dec_para->buffer_output_height*3) >> 1);
	}
	else {
		in.format = H264_FMT_YUYV;
		buffer_size = ((p_vid_dec_para->buffer_output_width*p_vid_dec_para->buffer_output_height) << 1);
	}

    cache_drain_range(frameAddr, buffer_size);
    cache_drain_range(outAddr, max_out_size);

    if (NullFrame)
        gp_h264enc_insert_skip(h264_handle, &in, 0);
    else
		gp_h264enc_encode(h264_handle, &in);
	status = gp_h264enc_wait(h264_handle, 0, pStreamSize);

    if (status < 0) {
		return status;
    }

    if (status == STATUS_H264_FRAME_END && *pStreamSize) {
		if (*pStreamSize > max_out_size)
			cache_invalid_range(outAddr, max_out_size);
		else
			cache_invalid_range(outAddr, *pStreamSize);
	}
	else if (status == STATUS_H264_FIFO_OUT_END)
		cache_invalid_range(outAddr, max_out_size);

	return status;
}

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
void browse_rtsp_h264_send_callback_done (INT32U stream_addr, INT32U stream_type){
	if(circular_buf_try_put((cbuf_handle_t)p_vid_dec_para->video_decode_cb_handle, stream_addr) < 0){
		ERR_PRINT("addr:%d\r\n", stream_addr);
	}
}

INT32S
browse_reencode_H264(
	INT32U disp_buf
)
{
	INT32S nRet;
	INT32U h264_enc_out_buf = vid_dec_get_next_vid_buffer();
	INT32U h264_re_enc_buf_size = p_vid_dec_para->buffer_output_width * p_vid_dec_para->buffer_output_height * 2;
	INT32S encode_size;
	INT32U type = 0;
	INT32S insertHdr = 0;

	if(h264_enc_out_buf == DUMMY_BUFFER_ADDRS){
		return STATUS_FAIL;
	}

	if(!h264_enc)
	{
		h264_enc = OpenH264(p_vid_dec_para->buffer_output_width, p_vid_dec_para->buffer_output_height);
		if(!h264_enc)
		{
			return STATUS_FAIL;
		}
	}

	if (FrmCnt > 0)
	{
		/*
			0: I-frame,1:P-frame
		*/
		type = 1;
	}
	else
	{
		insertHdr = 1;
	}

	nRet = EncodeH264Frame(
			disp_buf,
			h264_enc_out_buf,
			h264_re_enc_buf_size,
			type,
			insertHdr,
			0,
			&encode_size
			);

	if(circular_buf_try_put((cbuf_handle_t)p_vid_dec_para->scaler_output_cb_handle, disp_buf) < 0){
		nRet = -1;
		ERR_PRINT("addr:%d\r\n", disp_buf);
	}

	if(nRet < 0)
	{
		if(circular_buf_try_put((cbuf_handle_t)p_vid_dec_para->video_decode_cb_handle, h264_enc_out_buf) < 0){
			ERR_PRINT("addr:%d\r\n", h264_enc_out_buf);
		}
		return STATUS_FAIL;
	}


	if (nRet == STATUS_H264_FRAME_END)
	{
		if (FrmCnt >= C_H264ENC_GOP_LEN)
		{
			FrmCnt = 0;
		}
		rtsp_h264_send(h264_enc_out_buf,encode_size,xTaskGetTickCount(),type, browse_rtsp_h264_send_callback_done);
	}
	else{
		if(circular_buf_try_put((cbuf_handle_t)p_vid_dec_para->video_decode_cb_handle, h264_enc_out_buf) < 0){
			ERR_PRINT("addr:%d\r\n", h264_enc_out_buf);
		}
	}

	return STATUS_OK;
}
#endif


/* == JPEG == */
INT32S
browse_dec_JPEG_info_get(
	INT32U raw_data,
	INT32U raw_size,
	INT16U* ret_width,
	INT16U* ret_height
)
{
	return (jpeg_api_decode_get_size(USE_JPEG_NUM, raw_data, raw_size, ret_width, ret_height, &jpeg_yuv_mode));
}

INT32S
browse_dec_JPEG(
	INT32U raw_data,
	INT32U raw_size,
	INT16U raw_width,
	INT16U raw_height,
	INT32U dec_out_buf
)
{
	INT32S nRet;
	JpegDec_t mjpeg;

	memset(&mjpeg, 0x0, sizeof(JpegDec_t));
	mjpeg.hwidx = USE_JPEG_NUM;
	mjpeg.scaler_mode = 1;
	mjpeg.raw_data_addr = raw_data;
	mjpeg.raw_data_size = raw_size;
	mjpeg.jpeg_yuv_mode = jpeg_yuv_mode;
	mjpeg.jpeg_valid_w = raw_width;
	mjpeg.jpeg_valid_h = raw_height;
	mjpeg.output_addr = dec_out_buf;
	mjpeg.output_format = p_vid_dec_para->video_decode_out_format;
	mjpeg.output_w = p_vid_dec_para->image_output_width;
	mjpeg.output_h = p_vid_dec_para->image_output_height;
	mjpeg.output_buffer_w = p_vid_dec_para->buffer_output_width;
	mjpeg.output_buffer_h = p_vid_dec_para->buffer_output_height;
	mjpeg.output_boundary_color = 0x008080;

	nRet = jpeg_api_decode_output(&mjpeg);

	return nRet;
}


INT32S
browse_dec_JPEG_and_scale(
	void* p,
	INT32U raw_data,
	INT32U raw_size,
	INT16U raw_width,
	INT16U raw_height
)
{
	INT32S nRet = STATUS_OK;
	BROWSE_VID_DEC_CTRL_T *vid_dec_ctrl = (BROWSE_VID_DEC_CTRL_T*) p;


	if(vid_dec_ctrl->flag_get_first)
	{
		nRet = browse_dec_JPEG(raw_data,raw_size,raw_width,raw_height,vid_dec_ctrl->get_first_out_buf);
	}

	return nRet;
}

INT32S
browse_dec_JPEG_one_frame(
	INT32U raw_data,
	INT32U raw_size,
	INT16U raw_width,
	INT16U raw_height
)
{
	INT32S nRet;
	INT32U out_buf;

	out_buf = browse_dec_get_buf();

	nRet = browse_dec_JPEG(raw_data,raw_size,raw_width,raw_height,out_buf);

	if (out_buf == DUMMY_BUFFER_ADDRS)
	{
		out_buf = 0;
	}
	else
	{
		out_buf -= video_decode_display_offset();
		if(video_decode_task_display_notify(out_buf) == STATUS_FAIL){
			if(circular_buf_try_put((cbuf_handle_t)p_vid_dec_para->scaler_output_cb_handle, out_buf) < 0){
				ERR_PRINT("addr:%d\r\n", out_buf);
			}
		}
	}

	return nRet;
}

/* == H264 == */
INT32S browse_dec_H264_info_get(INT32U raw_data,INT32U raw_size,INT16U* ret_width,INT16U* ret_height)
{
	return (vid_dec_parser_h264_bit_stream((INT8U *)raw_data, raw_size, ret_width, ret_height));
}

static void PScaler_dec_callback(INT32U PScaler_Event)
{
    INT32U display_addr = 0;
	INT32U disp_addr_next;

    if (PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE)
    {
        display_addr = drv_l1_pscaler_output_A_buffer_get(PSCALER_A);

		disp_addr_next = browse_dec_get_buf();
		drv_l1_pscaler_output_A_buffer_set(PSCALER_A, disp_addr_next);
    }
    else if (PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE)
    {
        display_addr = drv_l1_pscaler_output_B_buffer_get(PSCALER_A);
		disp_addr_next = browse_dec_get_buf();
		drv_l1_pscaler_output_B_buffer_set(PSCALER_A, disp_addr_next);
    }

	if (display_addr == DUMMY_BUFFER_ADDRS)
	{
		display_addr = 0;
	}

	if (display_addr)
	{
        #if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
        if(video_decode_display_x_offset_get())
        {
            display_addr -= video_decode_display_x_offset_get();
            if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
            video_decode_display_do_padding(display_addr,p_vid_dec_para->buffer_output_width,p_vid_dec_para->image_output_height,video_decode_display_x_offset_get());
            else
            video_decode_display_do_padding(display_addr,p_vid_dec_para->buffer_output_width,p_vid_dec_para->image_output_height,video_decode_display_x_offset_get()/2);
            /*INT32U i,cnt;
            cnt = p_vid_dec_para->image_output_height*3/2;
            for(i = 0; i < cnt; i++)
            {
                if((i % 3) == 1)
                {
                    gp_memset((INT8S *)(display_addr + i * p_vid_dec_para->buffer_output_width), 0x80, video_decode_display_x_offset_get());
                    gp_memset((INT8S *)(display_addr + (i + 1) * p_vid_dec_para->buffer_output_width - video_decode_display_x_offset_get()), 0x80, video_decode_display_x_offset_get());
                }
                else
                {
                    gp_memset((INT8S *)(display_addr + i * p_vid_dec_para->buffer_output_width), 0, video_decode_display_x_offset_get());
                    gp_memset((INT8S *)(display_addr + (i + 1) * p_vid_dec_para->buffer_output_width - video_decode_display_x_offset_get()), 0, video_decode_display_x_offset_get());
                }
            }*/
        }
        #endif
		//DBG_PRINT("=>0x%x ",display_addr);
		display_addr -= video_decode_display_offset();
		if(video_decode_task_display_notify(display_addr) == STATUS_FAIL){
			if(circular_buf_try_put((cbuf_handle_t)p_vid_dec_para->scaler_output_cb_handle, display_addr) < 0){
				ERR_PRINT("addr:%d\r\n", display_addr);
			}
		}
	}

}


void PScaler_dec_callback_2(INT32U PScaler_Event)
{
    INT32U display_addr = 0, display_addr_0 = 0;
    INT32U disp_addr_next = 0;


    if (PScaler_Event & PIPELINE_SCALER_STATUS_BUF_A_DONE)
    {
        if(h264_Mb2scan_triger == 1){
            disp_addr_next = drv_l1_pscaler_output_A_buffer_get(PSCALER_A);
            h264_Mb2scan_triger = 0;
        }else{
            disp_addr_next = browse_dec_get_buf();
        }
            drv_l1_pscaler_output_A_buffer_set(PSCALER_A, disp_addr_next);
            gp_h264dec_mb2scan_notify();
        }
        else if (PScaler_Event & PIPELINE_SCALER_STATUS_BUF_B_DONE)
        {
            display_addr = drv_l1_pscaler_output_B_buffer_get(PSCALER_A);
            display_addr_0 = drv_l1_pscaler_output_A_buffer_get(PSCALER_A);
            if (display_addr_0 != DUMMY_BUFFER_ADDRS){
                drv_l1_pscaler_output_B_buffer_set(PSCALER_A, display_addr_0 + p_vid_dec_para->H264_MB2SCAN_patch_offset);
            }

            if (display_addr != DUMMY_BUFFER_ADDRS){
                display_addr = display_addr - p_vid_dec_para->H264_MB2SCAN_patch_offset;
            }
        }

        if (display_addr == DUMMY_BUFFER_ADDRS)
            display_addr = 0;

        if (display_addr)
        {
            #if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
            if(video_decode_display_x_offset_get())
            {
                display_addr -= video_decode_display_x_offset_get();
                if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
                video_decode_display_do_padding(display_addr,p_vid_dec_para->buffer_output_width,p_vid_dec_para->image_output_height,video_decode_display_x_offset_get());
                else
                video_decode_display_do_padding(display_addr,p_vid_dec_para->buffer_output_width,p_vid_dec_para->image_output_height,video_decode_display_x_offset_get()/2);
                /*INT32U i,cnt;
                cnt = p_vid_dec_para->image_output_height*3/2;
                for(i = 0; i < cnt; i++)
                {
                    if((i % 3) == 1)
                    {
                        gp_memset((INT8S *)(display_addr + i * p_vid_dec_para->buffer_output_width), 0x80, video_decode_display_x_offset_get());
                        gp_memset((INT8S *)(display_addr + (i + 1) * p_vid_dec_para->buffer_output_width - video_decode_display_x_offset_get()), 0x80, video_decode_display_x_offset_get());
                    }
                    else
                    {
                        gp_memset((INT8S *)(display_addr + i * p_vid_dec_para->buffer_output_width), 0, video_decode_display_x_offset_get());
                        gp_memset((INT8S *)(display_addr + (i + 1) * p_vid_dec_para->buffer_output_width - video_decode_display_x_offset_get()), 0, video_decode_display_x_offset_get());
                    }
                }*/
            }
            #endif
            display_addr -= video_decode_display_offset();
        	//DBG_PRINT(" <dis0x%x> \r\n",display_addr);
            if(video_decode_task_display_notify(display_addr) == STATUS_FAIL){
                if(circular_buf_try_put((cbuf_handle_t)p_vid_dec_para->scaler_output_cb_handle, display_addr) < 0){
                    ERR_PRINT("addr:%d)\r\n", display_addr);
            }
        }
    }

}

INT32S browse_dec_H264_one_frame(INT32U raw_data,INT32U raw_size,INT16U raw_width,INT16U raw_height)
{
    gp_h264dec_in param;
    INT8U pad_size;
    INT32S Len ;
	INT32U coding_type=0;

 //--- for fast reverse playback add----
 //----fast reverse playback will stop decode first , then handel will reset---
    INT16U disp_width,disp_height;
    INT32U out_div = 0, out_width, out_height, crop_width, crop_height;

  if(h264_dec == NULL)
  {
    //DISP_P	display_frame = NULL;
    BROWSE_VID_DEC_CTRL_T *vid_dec_ctrl = pVid_Dec_Ctrl;
    gp_h264dec_cfg cfg = {0};


    cfg.width = raw_width;
    cfg.height = raw_height;
    cfg.ext_buf = NULL;
    disp_width = p_vid_dec_para->image_output_width;
	disp_height =  p_vid_dec_para->image_output_height;
	out_width = (raw_width + 0xf) & ~0xf;
    out_height = (raw_height + 0xf) & ~0xf;
    out_div = out_width/disp_width;
    if (out_div > out_height/disp_height)
    {
        out_div = out_height/disp_height;
    }

    if (out_div >= 4)
    {
        out_div = 4;
    }
    else if (out_div >= 2)
    {
        out_div = 2;
    }

    else
    {
        out_div = 2;
    }
	cfg.out_div = out_div;

	#if H264ENC_FBC_ENABLE == 1
    cfg.buf_num = 2;            //internal working frame buffer number, default 1. (2 for fbc enable)
    #else
    cfg.buf_num = 1;
    #endif

	cfg.fbc = H264ENC_FBC_ENABLE;

    h264_dec = gp_h264dec_init(&cfg);
	if(h264_dec == NULL)
	{
		ERR_PRINT("h264_dec is null \r\n");
		return STATUS_FAIL;
	}
	if (out_div)
	{
        out_width = out_width/out_div;
        out_height = out_height/out_div;
        out_width = (out_width + 0xf) & ~0xf;
		out_height = (out_height + 0xf) & ~0xf;
		crop_width = raw_width/out_div;
		crop_height = raw_height/out_div;
    }
    else
    {
        crop_width = raw_width;
        crop_height = raw_height;
    }

	if(vid_dec_ctrl->flag_get_first)
	{
		vid_dec_h264_pscaler_start(crop_width, crop_height, out_width, out_height, NULL, vid_dec_ctrl->get_first_out_buf);

		if(browse_dec_H264_one_frame(raw_data,raw_size,raw_width,raw_height) == STATUS_FAIL)
		{
			return STATUS_FAIL;
		}

		if(vid_dec_h264_pscaler_status_check() == STATUS_FAIL)
		{
			ERR_PRINT("decode time out\r\n");
			return STATUS_FAIL;
		}
	}
	else
	{
		if (p_vid_dec_para->H264_MB2SCAN_patch==1){
			vid_dec_h264_pscaler_start(crop_width, crop_height, out_width, out_height, PScaler_dec_callback_2, 0);
		}else{
			vid_dec_h264_pscaler_start(crop_width, crop_height, out_width, out_height, PScaler_dec_callback, 0);
		}
	}
  }


    param.BitstreamAddr = raw_data;
    param.BitstreamBaseAddr = raw_data;
    param.BitstreamMaxAddr = raw_data + raw_size;
    param.BitstreamMaxAddr = (param.BitstreamMaxAddr + 0x3F) >> 6 << 6; //64 byte alignment for H.264
    pad_size = param.BitstreamMaxAddr - (raw_data + raw_size);
    if (pad_size)
    { //clear padding bytes to avoid possible decoding error
        gp_memset((void*)(raw_data + raw_size), 0, pad_size);
    }
    cache_drain_range(raw_data, raw_size + pad_size);

    if(p_vid_dec_para->fail_cnt)
        param.drop_frame = TRUE;
    else
        param.drop_frame = FALSE;

    if (0 >gp_h264dec_decode_one_frm(h264_dec, &param, &coding_type, 50, &Len))
    {
		ERR_PRINT("decode time out\r\n");
		return STATUS_FAIL;
    }
    if (Len<0xA0)
    {
      ERR_PRINT("Abnormal size(%d)\r\n",Len); // gp 影片 , 沒有skip frame , so 不應該有如此小的size
      return STATUS_FAIL;
    }


	return STATUS_OK;
}

INT32S
browse_dec_H264_and_scale(
	void* p,
	INT32U raw_data,
	INT32U raw_size,
	INT16U raw_width,
	INT16U raw_height
)
{
    INT32U out_div = 0, out_width, out_height, crop_width, crop_height;
	INT16U disp_width,disp_height;
	BROWSE_VID_DEC_CTRL_T *vid_dec_ctrl = (BROWSE_VID_DEC_CTRL_T*) p;

    gp_h264dec_cfg cfg = {0};
    cfg.width = raw_width;
    cfg.height = raw_height;
    cfg.ext_buf = NULL;

	disp_width = p_vid_dec_para->image_output_width;
	disp_height =  p_vid_dec_para->image_output_height;

    out_width = (raw_width + 0xf) & ~0xf;
    out_height = (raw_height + 0xf) & ~0xf;
    out_div = out_width/disp_width;
    if (out_div > out_height/disp_height)
    {
        out_div = out_height/disp_height;
    }

    if (out_div >= 4)
    {
        out_div = 4;
    }
    else if (out_div >= 2)
    {
        out_div = 2;
    }
    else
    {
        out_div = 2;
    }

    cfg.out_div = out_div;


	#if H264ENC_FBC_ENABLE == 1
    cfg.buf_num = 2;            //internal working frame buffer number, default 1. (2 for fbc enable)
    #else
    cfg.buf_num = 1;
    #endif

	cfg.fbc = H264ENC_FBC_ENABLE;


    h264_dec = gp_h264dec_init(&cfg);
	if(h264_dec == NULL)
	{
		ERR_PRINT("h264_dec is null \r\n");
		return STATUS_FAIL;
	}

    if (out_div)
	{
        out_width = out_width/out_div;
        out_height = out_height/out_div;
        out_width = (out_width + 0xf) & ~0xf;
		out_height = (out_height + 0xf) & ~0xf;
		crop_width = raw_width/out_div;
		crop_height = raw_height/out_div;
    }
    else
    {
        crop_width = raw_width;
        crop_height = raw_height;
    }

	DBG_PRINT("out[W,H]=[%d,%d] crop[W,H]=[%d,%d] {%d} \r\n",out_width,out_height,crop_width,crop_height,vid_dec_ctrl->flag_get_first);
	if(vid_dec_ctrl->flag_get_first)
	{
		vid_dec_h264_pscaler_start(crop_width, crop_height, out_width, out_height, NULL, vid_dec_ctrl->get_first_out_buf);

		if(browse_dec_H264_one_frame(raw_data,raw_size,raw_width,raw_height) == STATUS_FAIL)
		{
			return STATUS_FAIL;
		}

		if(vid_dec_h264_pscaler_status_check() == STATUS_FAIL)
		{
			ERR_PRINT("decode time out\r\n");
			return STATUS_FAIL;
		}
	}
	else
	{
		if (p_vid_dec_para->H264_MB2SCAN_patch==1){
			vid_dec_h264_pscaler_start(crop_width, crop_height, out_width, out_height, PScaler_dec_callback_2, 0);
		}else{
			vid_dec_h264_pscaler_start(crop_width, crop_height, out_width, out_height, PScaler_dec_callback, 0);
		}
	}

	return STATUS_OK;
}

void
browse_dec_disp_where_set(
	INT8U disp_where
)
{
	flag_disp_where = disp_where;
}

INT8U
browse_dec_disp_where_get(
	void
)
{
	return flag_disp_where;
}


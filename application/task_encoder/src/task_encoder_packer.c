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
#include "gp_mux.h"
#include "vid_mp4_packer.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define FILE_PACKER_QUEUE_MAX			256
#define FILE_PACKER_ACK_MAX				1

typedef enum{
	FILE_STREAM_ACT_OPEN,
	FILE_STREAM_ACT_WRITE,
	FILE_STREAM_ACT_CLOSE,
	FILE_STREAM_ACT_REOPEN_SOFT,
	FILE_STREAM_ACT_REOPEN_HARD
} FILE_STREAM_ACT;

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct MOV_TRACK_CTRL_S{
	gpEsType_t	stream_type;
	INT32U		stream_id;
	void		(*frame_free_func)(INT8U src,INT8U to_snapshot,INT8U type,INT8U* data,INT32U frame_type);
	INT16S		last_aud_value;
	INT32U		track_pad_ms;
	INT32U		track_total_ms; // accumulate recording time
	INT32U		track_total_frame;
}MOV_TRACK_CTRL_T;

typedef struct MOV_STREAM_CTRL_S{
	INT32U		file_stream_max_ms; // maximum recording time
	INT32U		file_stream_next_ms; // for opening a new file
	MSG_Q_ID	file_stream_queue;
	MSG_Q_ID	file_stream_ack;
	osThreadId	file_stream_thread;
	int			file_packer_handle;
	INT8U		file_stream_enable;
	INT8U		file_stream_cyclic;
	FILE_STREAM_ACT file_stream_act;
	VID_INFO*  	pVid_info;
	MSG_Q_ID 	app_notify_q;
	INT8U		vid_src;
	INT8U		file_stream_open_file;
	INT32S      current_rec_file_handle;
} FILE_STREAM_CTRL_T;

typedef struct gpTrackMuxPkt_s
{
	INT8U vid_src;
	INT8U vid_type;
	INT8U track_id;
	INT8U flag_to_snapshot;
	INT32U ret_buf;
	gpMuxPkt_t pkt;
}gpTrackMuxPkt_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void stream_video_free(INT8U src,INT8U to_snapshot,INT8U type,INT8U* data,INT32U frame_type);
static void stream_audio_free(INT8U src,INT8U to_snapshot,INT8U type,INT8U* data,INT32U frame_type);


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static struct gpMux_s* file_packer = &MOV_packer;
static FILE_STREAM_CTRL_T file_stream_ctrl[PREVIEW_SRC_TOTAL] = {{0}};

MOV_TRACK_CTRL_T mov_track[PREVIEW_SRC_TOTAL][MOV_TRACK_NUM] =
{
//PREVIEW_SRC_FRONT
	{
		{
			GP_ES_TYPE_VIDEO,
			MOV_STREAM_VID_RECORD,
			stream_video_free,
		}
		,
		{
			GP_ES_TYPE_AUDIO,
			MOV_STREAM_AUD_RECORD,
			stream_audio_free,
		}
    /*
      below is  for insert dummy frame use
    */
		,
		{
		   GP_ES_TYPE_DUMMYVIDEO,
		   MOV_STREAM_VID_RECORD,
           stream_video_free,
		}
	},

//PREVIEW_SRC_REAR
	{
		{
			GP_ES_TYPE_VIDEO,
			MOV_STREAM_VID_RECORD,
			stream_video_free,
		}
		,
		{
			GP_ES_TYPE_AUDIO,
			MOV_STREAM_AUD_RECORD,
			stream_audio_free,
		}
     /*
      below is  for insert dummy frame use
    */
		,
		{
		   GP_ES_TYPE_DUMMYVIDEO,
		   MOV_STREAM_VID_RECORD,
           stream_video_free,
		}
	}

};

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
INT32U file_packer_timer = 0;
INT32U file_packer_timer_max = 0;
volatile INT32U file_packer_timer_flag = 0;

volatile INT32U file_packer_err = 0;

static INT32U prio_flag = 0;

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/
extern INT32S lib_free_H264(INT32U src,INT32U data,INT32U frameType);
extern INT32S lib_free_JPEG(INT32U src,INT32U data,INT32U frameType);
extern void ap_audio_free_cap_buffer(INT32U data);

extern void mov_gps_info_init(INT8U packerID);
extern void mov_gps_info_uninit(INT8U packerID);
extern void mov_gps_data_write(INT32U handle, gpMux_t* pMovPacker, INT8U packerID, INT32U offsetSize, INT8U writeTime);
extern void mov_gps_info_create(INT32U handle, INT8U packerID);
extern void mov_gps_frame_add(INT8U packerID);
extern void mov_gps_data_add(INT8U packerID);
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

#if (C_PACKER_AES_ENCRYPT) && (C_PACKER_AES_ENCRYPT)
INT32S AesEncryptFunc(INT8U* buf, INT32U size)
{
	AES_PARAM_t aes_info={0};
	static ALIGN64 const unsigned char key1[]=
	{
		0x98, 0x07, 0x62, 0x45, 0x38, 0x96, 0x57, 0x02, 0x87, 0x09, 0x21, 0x75, 0x96, 0x28, 0x97, 0x53,
		0x53, 0x97, 0x28, 0x96, 0x75, 0x21, 0x09, 0x87, 0x02, 0x57, 0x96, 0x38, 0x45, 0x62, 0x07, 0x98
	};

	static ALIGN64 unsigned char iv1[]=
	{
		0x23, 0x96, 0x57, 0x02, 0x89, 0x57, 0x14, 0x02, 0x52, 0x89, 0x14, 0x20, 0x63, 0x89, 0x57, 0x02
	};
	INT8U* srcbuf;
	INT8U* dstbuf;
	INT32U length;

	length = size >> 5 << 5;
	if (((INT32U)buf) & 0x1F) {
		srcbuf = gp_malloc(length*2);
		if (!srcbuf)
			return -1;
		dstbuf = srcbuf + length;
	}
	else {
		srcbuf = buf;
		dstbuf = gp_malloc(length);
		if (!dstbuf)
			return -1;
	}


	gp_memcpy((INT8S*)srcbuf, (INT8S*)buf, length);
	cache_drain_range((INT32U)srcbuf, length);

	aes_info.src_addr = srcbuf;
	aes_info.dst_addr = dstbuf;
    aes_info.data_len = length;
	aes_info.iv_addr = iv1;
	aes_info.key_addr = (INT8U *)key1;
	aes_info.key_len = 128;
	aes_info.AesMode = 0; //EcB
	aes_info.AesDir = AesENCRYPT;
	aes_info.uid_key_gen_en = 0;

	drv_l2_aes_dma(&aes_info);

	cache_invalid_range((INT32U)dstbuf, length);

	gp_memcpy((INT8S*)buf, (INT8S*)dstbuf, length);

	if (buf != srcbuf)
		gp_free(srcbuf);
	else
		gp_free(dstbuf);

	return 0;
}
#endif
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void file_packer_timer_TickHook(void)
{
    if(file_packer_timer_max)
    {

        if(++file_packer_timer >= file_packer_timer_max)
        {
			//DBG_PRINT("<%d,%d>", file_packer_timer,file_packer_timer_max );
            file_packer_timer = 0;
            file_packer_timer_flag = 1;
        }
    }
}

void adjust_file_packer_prio(void)
{
    FILE_STREAM_CTRL_T *stream_ctrl_front = &file_stream_ctrl[PREVIEW_SRC_FRONT];
    FILE_STREAM_CTRL_T *stream_ctrl_rear = &file_stream_ctrl[PREVIEW_SRC_REAR];

    if(stream_ctrl_front->file_stream_enable && stream_ctrl_front->file_stream_queue && stream_ctrl_front->file_stream_thread)
    {
        if(stream_ctrl_rear->file_stream_enable && stream_ctrl_rear->file_stream_queue && stream_ctrl_rear->file_stream_thread)
        {
            INT32U front_q_size, rear_q_size;

            front_q_size = msgQSizeGet(stream_ctrl_front->file_stream_queue);
            rear_q_size = msgQSizeGet(stream_ctrl_rear->file_stream_queue);

            if(front_q_size && rear_q_size)
            {
                if(front_q_size > (rear_q_size + 2))
                {
                    if(prio_flag != 1)
                    {
                        osThreadSetPriority(stream_ctrl_rear->file_stream_thread, PRIORITY_FILE_PACKER);
                        osThreadSetPriority(stream_ctrl_front->file_stream_thread, PRIORITY_FILE_PACKER_EMER);
                        prio_flag = 1;
                    }
                }
                else if((front_q_size + 2) < rear_q_size)
                {
                    if(prio_flag != 2)
                    {
                        osThreadSetPriority(stream_ctrl_front->file_stream_thread, PRIORITY_FILE_PACKER);
                        osThreadSetPriority(stream_ctrl_rear->file_stream_thread, PRIORITY_FILE_PACKER_EMER);
                        prio_flag = 2;
                    }
                }
            }
        }
    }
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


static void
stream_video_free(
	INT8U src,
	INT8U to_snapshot,
	INT8U type,
	INT8U* data,
	INT32U frame_type
)
{
	// Put H264/JPEG encode output buffer back to circular buf
	if(type == VID_REC_JPEG)
	{
		/*
			Don't put jpeg into circular buf if to_snapshot=1,
			Pass this jpeg to write a file
		*/
		if(to_snapshot)
		{
			return;
		}

		lib_free_JPEG(src, (INT32U)data, frame_type);
	}
	else
	{
		lib_free_H264(src, (INT32U)data, frame_type);
	}
}

static void
stream_audio_free(
	INT8U src,
	INT8U to_snapshot,
	INT8U type,
	INT8U* data,
	INT32U frame_type
)
{
	ap_audio_free_cap_buffer((INT32U)data);
}

static void
file_packer_free(
	gpTrackMuxPkt_t *trackPkt
)
{
	gpMuxPkt_t *pkt;
	MOV_TRACK_CTRL_T *track_ctrl;

	if(!trackPkt || trackPkt->track_id >= MOV_TRACK_NUM)
	{
		return;
	}

	track_ctrl = &mov_track[trackPkt->vid_src][trackPkt->track_id];

	pkt = &trackPkt->pkt;

	if(!pkt || !pkt->data)
	{
		return;
	}

	if(track_ctrl->frame_free_func)
	{
		track_ctrl->frame_free_func(trackPkt->vid_src,trackPkt->flag_to_snapshot,trackPkt->vid_type,(INT8U *)trackPkt->ret_buf,pkt->frameType);
	}

	pkt->data = NULL;
}

long long
file_packer_request_drop(
	INT32U src,
	INT8U track
)
{
	long long	drop_time = 0;

	return drop_time;
}

INT32S
file_packer_write_notify(
	FILE_PACKER_W_ARGS* pFile_packer_args
)
{
	gpTrackMuxPkt_t pkt_info = {pFile_packer_args->src,
								pFile_packer_args->type,
								pFile_packer_args->track,
								pFile_packer_args->snapshot,
								pFile_packer_args->ret_data,
								{pFile_packer_args->data, pFile_packer_args->size, pFile_packer_args->pts, pFile_packer_args->frameType,pFile_packer_args->Dummy_frame_cnt}};

	INT8U src = pFile_packer_args->src;
	INT8U track = pFile_packer_args->track;
	MOV_TRACK_CTRL_T *track_ctrl;
	FILE_STREAM_CTRL_T *stream_ctrl;

	stream_ctrl = &file_stream_ctrl[src];

	#if 0
	switch(track)
	{
		case MOV_TRACK_VID_RECORD:	DBG_PRINT("."); break;
		case MOV_TRACK_VID_THUMB:	DBG_PRINT(","); break;
		case MOV_TRACK_AUD_RECORD:	DBG_PRINT("A"); break;
		default:					DBG_PRINT("?"); break;
	}
	#endif

	if(track >= MOV_TRACK_NUM)
	{
		return STATUS_FAIL;
	}

	track_ctrl = &mov_track[src][track];
	switch(track_ctrl->stream_type)
	{
    case GP_ES_TYPE_VIDEO:
        if(track_ctrl->stream_id >= MOV_STREAM_VID_NUM)
            return STATUS_FAIL;
        break;
    case GP_ES_TYPE_AUDIO:
        if(track_ctrl->stream_id >= MOV_STREAM_AUD_NUM)
            return STATUS_FAIL;
        break;
    case GP_ES_TYPE_DUMMYVIDEO:
    //   DBG_PRINT(" 2K_Insert");

    break;
    default:
        break;
	}

	if(stream_ctrl->file_stream_enable)
	{
		if(stream_ctrl->file_stream_queue)
		{
			return msgQSend(stream_ctrl->file_stream_queue, MSG_FILE_PACKER_WRITE, &pkt_info, sizeof(pkt_info), MSG_PRI_NORMAL);
		}
	}
	return STATUS_FAIL;
}

static void
file_packer_write(
	FILE_STREAM_CTRL_T *stream_ctrl,
	gpTrackMuxPkt_t *trackPkt
)
{
	int		ret_err,i;
	gpMuxPkt_t *pkt;
	MOV_TRACK_CTRL_T *track_ctrl;

	if(!trackPkt || trackPkt->track_id >= MOV_TRACK_NUM)
	{
		goto FAIL_FILE_PACKER_WRITE;
	}

	track_ctrl = &mov_track[trackPkt->vid_src][trackPkt->track_id];

	pkt = &trackPkt->pkt;

	//DBG_PRINT("[%s]->msg_id:0x%x\n",__func__,msg_id);
	if(!pkt || !pkt->data)
	{
		goto FAIL_FILE_PACKER_WRITE;
	}

    /*
	if(track_ctrl->track_total_ms >= stream_ctrl->file_stream_max_ms)
	{
		DBG_PRINT("d%c%d", track_ctrl->stream_type == GP_ES_TYPE_VIDEO ? 'v' : 'a', track_ctrl->stream_id);
		goto FAIL_FILE_PACKER_WRITE;
	} */

	//DBG_PRINT("Addr,Size,Pts,Type = 0x%x,%d,%d,%d\n", pkt.data, pkt.size, pkt.pts, pkt.frameType);
	if(stream_ctrl->file_stream_act != FILE_STREAM_ACT_WRITE || !stream_ctrl->file_packer_handle)
	{
		DBG_PRINT("close filing:pkt->data:0x%x\r\n",pkt->data);
		goto FAIL_FILE_PACKER_WRITE;
	}

	/*
		The first frame must be a I-Frame
	*/
	if(track_ctrl->stream_type == GP_ES_TYPE_VIDEO &&
		mov_track[trackPkt->vid_src][MOV_TRACK_VID_RECORD].track_total_ms == 0 &&
		pkt->frameType != H264_FRAME_TYPE_I
	)
	{
		//DBG_PRINT("p%c%d", track_ctrl->stream_type == GP_ES_TYPE_VIDEO ? 'v' : 'a', track_ctrl->stream_id);
		goto FAIL_FILE_PACKER_WRITE;
	}

    if(file_packer_err)
    {
		goto FAIL_FILE_PACKER_WRITE;
    }

	ret_err = file_packer->pack(stream_ctrl->file_packer_handle, pkt, track_ctrl->stream_type);
	if (ret_err != 0)
	{
		ERR_PRINT("pack fail (%d)\r\n", ret_err);
		if (ret_err == MUX_FRAME_INVALID)
		{
			ERR_PRINT("0x%08x 0x%08x 0x%08x 0x%08x\r\n", *(INT32U*)pkt->data, *(INT32U*)(pkt->data+4), *(INT32U*)(pkt->data+8), *(INT32U*)(pkt->data+12));
		}
		else
		{
            osSuspend();
            if(file_packer_err == 0)
            {
                file_packer_err = 1;
                osResume();

                //DBG_PRINT("Snd-PACKER_ERROR\r\n");
                while(0 != msgQSend(stream_ctrl->app_notify_q, MSG_APQ_PACKER_ERROR, NULL, 0, MSG_PRI_NORMAL))
                {
                    osDelay(10);
                }
                DBG_PRINT("abnormal Close file!\r\n");
            }
            else
            {
                osResume();
            }

			goto FAIL_FILE_PACKER_WRITE;
		}
	}
	else
	{
		#if 1
		if(trackPkt->vid_src)
		{
			switch(trackPkt->track_id)
			{
				case MOV_TRACK_VID_RECORD:
				if(pkt->frameType==2)   // skip frame of h264
				{
				    DBG_PRINT("n"); // debug use , prevent lots of log
                }
				else
                    DBG_PRINT(",");
                break;
				case MOV_TRACK_AUD_RECORD:	DBG_PRINT("B"); break;
				case MOV_TRACK_VID_DUMMYRECORD : // dummy frame for  mjpg condition
			//	DBG_PRINT("n");      // debug use , prevent lots of log
				break;
				default:					DBG_PRINT("@"); break;
			}
		}
		else
		{
			switch(trackPkt->track_id)
			{
				case MOV_TRACK_VID_RECORD:
				if(pkt->frameType==2)
				{
				  for(i=0;i<pkt->Dummy_frame_cnt;i++)
				  {
					DBG_PRINT("N"); // debug use , prevent lots of log
				  }
				}
				else
					DBG_PRINT(".");

                break;
				case MOV_TRACK_AUD_RECORD:	DBG_PRINT("A"); break;
				case MOV_TRACK_VID_DUMMYRECORD :
                for(i=0;i<pkt->Dummy_frame_cnt;i++)
                {
                    DBG_PRINT("N"); // debug use , prevent lots of log
                }
				//pkt->pts = 0;    // do not need to count time , when insert dummy frame
				break;
				default:					DBG_PRINT("?"); break;
			}
		}
		#endif
	}

	if ((track_ctrl->stream_type == GP_ES_TYPE_VIDEO) || (track_ctrl->stream_type == GP_ES_TYPE_DUMMYVIDEO)) {
		// video
		if (ret_err) {
			mov_track[trackPkt->vid_src][MOV_TRACK_VID_RECORD].track_pad_ms += pkt->pts;
		} else {
			mov_track[trackPkt->vid_src][MOV_TRACK_VID_RECORD].track_total_ms += pkt->pts + mov_track[trackPkt->vid_src][MOV_TRACK_VID_RECORD].track_pad_ms;
			mov_track[trackPkt->vid_src][MOV_TRACK_VID_RECORD].track_pad_ms = 0;
		}
		mov_track[trackPkt->vid_src][MOV_TRACK_VID_RECORD].track_total_frame++;

	} else if (ret_err == 0) {		// Audio
		track_ctrl->track_total_ms += pkt->pts;
		++track_ctrl->track_total_frame;
	}

	file_packer_free(trackPkt);

	if(stream_ctrl->file_stream_act == FILE_STREAM_ACT_WRITE)
	{
		if(mov_track[trackPkt->vid_src][MOV_TRACK_VID_RECORD].track_total_ms >= stream_ctrl->file_stream_max_ms)
		{
			if(stream_ctrl->file_stream_cyclic)
			{
				//stream_ctrl->file_stream_act = FILE_STREAM_ACT_REOPEN_SOFT;
			}
			else
			{
				stream_ctrl->file_stream_act = FILE_STREAM_ACT_CLOSE;
			}
		}

		/*
			Send MSG_APQ_CYCLIC_VIDEO_RECORD if need loop recording
		*/
		if(!stream_ctrl->file_stream_open_file &&
			stream_ctrl->file_stream_cyclic &&
			(mov_track[trackPkt->vid_src][MOV_TRACK_VID_RECORD].track_total_ms >= stream_ctrl->file_stream_next_ms)
		)
		{
			//DBG_PRINT("\r\nOpen the next file(%d, %d)\r\n", mov_track[trackPkt->vid_src][MOV_TRACK_VID_RECORD].track_total_ms, stream_ctrl->file_stream_next_ms);

            /************************************************************/
            file_packer_timer_flag = 0;     //wait next time to set this flag
            /************************************************************/

			stream_ctrl->file_stream_open_file = 1;
			//msgQSend(stream_ctrl->app_notify_q, MSG_APQ_CYCLIC_VIDEO_RECORD, NULL, 0, MSG_PRI_NORMAL);    //moved to file_packer_task_entry()
		}
	}

	if(stream_ctrl->file_stream_act != FILE_STREAM_ACT_WRITE)
	{
		int i;

		DBG_PRINT("\r\n");
		for(i = 0 ; i < MOV_TRACK_NUM ; ++i)
		{
			DBG_PRINT("Close file!(track[%d]: %d frames in %d ms)\r\n", i, mov_track[trackPkt->vid_src][i].track_total_frame, mov_track[trackPkt->vid_src][i].track_total_ms);
		}
	}

	return;

FAIL_FILE_PACKER_WRITE:
	if(trackPkt->flag_to_snapshot)
	{
		trackPkt->flag_to_snapshot = 0;
	}
	file_packer_free(trackPkt);
}

void
file_packer_set_max_time(
	INT8U 	src,
	INT8U	max_minute_time
)
{
	FILE_STREAM_CTRL_T *stream_ctrl;

	stream_ctrl = &file_stream_ctrl[src];
	stream_ctrl->file_stream_max_ms = max_minute_time * 60 * 1000;
	stream_ctrl->file_stream_next_ms = stream_ctrl->file_stream_max_ms >> 1;
}

void
file_packer_set_cyclic(
	INT8U 	src,
	INT8U	cyclic
)
{
	FILE_STREAM_CTRL_T *stream_ctrl;

	stream_ctrl = &file_stream_ctrl[src];
	stream_ctrl->file_stream_cyclic = cyclic;
}

INT32S
file_packer_open(
	INT8U src,
 	VID_INFO* pVideo_info,
 	INT16S rec_file_handle
)
{
	INT32S file_packer_handle;
	int i;

	file_packer_handle = file_packer->open(rec_file_handle,src);
	if(file_packer_handle == 0)
	{
		ERR_PRINT("fail to packer open\r\n");
		return STATUS_FAIL;
	}

    file_packer->set(file_packer_handle, MUX_MAX_SIZE, FILE_PACKER_MAX_RECORD_SIZE);

    if (pVideo_info->vid_rec_type == VID_REC_JPEG)
    {
        file_packer->set(file_packer_handle, MUX_VID_TYPE, VIDEO_TYPE_MJPEG);
    }
    else if (pVideo_info->vid_rec_type == VID_REC_H264)
    {
        file_packer->set(file_packer_handle, MUX_VID_TYPE, VIDEO_TYPE_H264_HP);
    }

    file_packer->set(file_packer_handle, MUX_WIDTH, pVideo_info->target_width);
    file_packer->set(file_packer_handle, MUX_HEIGHT, pVideo_info->target_height);
    file_packer->set(file_packer_handle, MUX_FRMRATE, pVideo_info->frame_rate);

	if(MOV_STREAM_AUD_NUM)
	{
		file_packer->set(file_packer_handle, MUX_AUD_TYPE, AUD_FORMAT_PCM);
		file_packer->set(file_packer_handle, MUX_AUD_CHANNEL, 1);
		file_packer->set(file_packer_handle, MUX_AUDSR, AUD_RECORD_SAMPLE_RATE);
		file_packer->set(file_packer_handle, MUX_AUD_BIT, 16);
	}
	else
	{
		file_packer->set(file_packer_handle, MUX_AUD_TYPE, 0);
	}

	// Init mov track
	for(i = 0 ; i < MOV_TRACK_NUM ; ++i)
	{
		MOV_TRACK_CTRL_T *track_ctrl = &mov_track[src][i];
		track_ctrl->track_total_frame = 0;
		track_ctrl->track_total_ms = 0;
		track_ctrl->track_pad_ms = 0;
	}

	#if (C_PACKER_AES_ENCRYPT) && (C_PACKER_AES_ENCRYPT)
	muxMP4AddMetaData(file_packer_handle, "VE", "1"); // 1: video encrypted
	muxMP4AddMetaData(file_packer_handle, "AE", "1"); // 1: audio encrypted
	muxMP4AddMetaData(file_packer_handle, "ENMODE", "0"); // 0: 128 ECB, 1:128 CBC, 2: 256 ECB, 3:256 CBC	
	muxMP4SetEncryptFunc(file_packer_handle, AesEncryptFunc);
	#endif

	return file_packer_handle;
}


static INT32S
file_packer_task_init(
	FILE_STREAM_CTRL_T *stream_ctrl
)
{
	INT32S nRet = STATUS_OK;

	stream_ctrl->file_stream_queue = msgQCreate(FILE_PACKER_QUEUE_MAX, FILE_PACKER_QUEUE_MAX, sizeof(gpTrackMuxPkt_t));
	if(stream_ctrl->file_stream_queue == NULL)
	{
		ERR_PRINT("file_stream_queue is null!\r\n");
		RETURN(STATUS_FAIL);
	}

	stream_ctrl->file_stream_ack = msgQCreate(FILE_PACKER_ACK_MAX,FILE_PACKER_ACK_MAX,0);
	if(stream_ctrl->file_stream_ack == NULL)
	{
		ERR_PRINT("file_stream_ack is null!\r\n");
		RETURN(STATUS_FAIL);
	}
Return:
	return nRet;
}

static void
file_packer_task_uninit(
	FILE_STREAM_CTRL_T *stream_ctrl
)
{
	if(stream_ctrl->file_stream_queue)
	{
#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE == CUSTOM_ON)
		INT32U msg_id;
		gpTrackMuxPkt_t trackPkt;

		while(msgQAccept(stream_ctrl->file_stream_queue, &msg_id, &trackPkt, sizeof(trackPkt)) != STATUS_FAIL)
		{
			if(trackPkt.track_id == MOV_TRACK_AUD_RECORD)
			{
				file_packer_free(&trackPkt);
			}
			//DBG_PRINT("trackPkt.track_id=0x%x\r\n", trackPkt.track_id);
		}
#endif

		msgQDelete(stream_ctrl->file_stream_queue);
		stream_ctrl->file_stream_queue = 0;
	}

	if(stream_ctrl->file_stream_ack)
	{
		msgQDelete(stream_ctrl->file_stream_ack);
		stream_ctrl->file_stream_ack = 0;
	}
}

INT32S
file_packer_uninit(
	INT8U src,
	FILE_STREAM_CTRL_T *stream_ctrl
)
{
	INT32S nRet = STATUS_OK;

	if(stream_ctrl->file_packer_handle)
	{
		file_packer->uninit(stream_ctrl->file_packer_handle,src);
	}

	return nRet;
}

static void
file_packer_task_entry(
	void const *args
)
{
	INT32U msg_id;
	gpTrackMuxPkt_t trackPkt;
	FILE_STREAM_CTRL_T *stream_ctrl = (FILE_STREAM_CTRL_T*) args;
	VID_INFO* vid_info = stream_ctrl->pVid_info;
	INT8U vid_src = stream_ctrl->vid_src;

	stream_ctrl->current_rec_file_handle = vid_info->rec_file_handle;
	vid_info->rec_file_handle = -1;
	#if GPS_INFO_PACK || GPS_INFO_PACK_2
	MOV_TRACK_CTRL_T *track_ctrl;
	INT8U mov_gps_info_create_flag = 0;
	INT32U	last_time = 0;
    #endif
	//DBG_PRINT("file_packer_task_entry(%d) start\r\n",vid_src);

	stream_ctrl->file_stream_act = FILE_STREAM_ACT_OPEN;
	stream_ctrl->file_packer_handle = file_packer_open(vid_src,vid_info, stream_ctrl->current_rec_file_handle);
	//DBG_PRINT("file_packer_handle->0x%x\r\n", stream_ctrl->file_packer_handle);
	if(stream_ctrl->file_packer_handle == STATUS_FAIL)
	{
		ERR_PRINT("file_packer_task_entry() stop: file_packer_handle == NULL\r\n");

        if(stream_ctrl->current_rec_file_handle > -1)
        {
            close(stream_ctrl->current_rec_file_handle);
            stream_ctrl->current_rec_file_handle = -1;
        }
		stream_ctrl->file_stream_enable = 0;
		goto FAIL_FILE_PACKER_TASK;
	}

	stream_ctrl->file_stream_act = FILE_STREAM_ACT_WRITE;
	#if GPS_INFO_PACK || GPS_INFO_PACK_2
    mov_gps_info_init(vid_src);
    #endif
	while(stream_ctrl->file_stream_enable)
	{
		if(stream_ctrl->file_stream_act != FILE_STREAM_ACT_WRITE)
		{
			if(stream_ctrl->file_packer_handle)
			{
                #if GPS_INFO_PACK || GPS_INFO_PACK_2
                if(mov_gps_info_create_flag)
                {
                    //DBG_PRINT("GPS write[%d]\r\n",vid_src);
                    mov_gps_data_write(stream_ctrl->file_packer_handle,file_packer,vid_src, 0, 0);
                    mov_gps_info_create_flag = 0;
                    last_time = 0;
                }
                #endif
				DBG_PRINT("close file_packer_handle->0x%x\r\n", stream_ctrl->file_packer_handle);
				file_packer->close(stream_ctrl->file_packer_handle);
				if(stream_ctrl->file_stream_act != FILE_STREAM_ACT_CLOSE)
					stream_ctrl->file_packer_handle = 0;
			}

			if(stream_ctrl->current_rec_file_handle > -1)
			{
				DBG_PRINT("close current_rec_file_handle->0x%x\r\n", stream_ctrl->current_rec_file_handle);
				close(stream_ctrl->current_rec_file_handle);
				stream_ctrl->current_rec_file_handle = -1;
				UsrFlushBuffers(STORAGE_DEVICE);
			}

			//DBG_PRINT("act:%d\r\n",stream_ctrl->file_stream_act);

			switch(stream_ctrl->file_stream_act)
			{
				case FILE_STREAM_ACT_CLOSE:
				{
                    if(stream_ctrl->file_stream_ack)
                    {
                        msgQSend(stream_ctrl->file_stream_ack, C_ACK_SUCCESS, NULL, 0, MSG_PRI_NORMAL);
                    }

					stream_ctrl->file_stream_enable = 0;
					continue;
				}
				break;

				case FILE_STREAM_ACT_REOPEN_SOFT:
				{
                    //stream_ctrl->current_rec_file_handle = vid_info->rec_file_handle;
					//vid_info->rec_file_handle = -1;

					stream_ctrl->file_packer_handle = file_packer_open(vid_src, vid_info, stream_ctrl->current_rec_file_handle);     //stream_ctrl->current_rec_file_handle == -1
					if(stream_ctrl->file_packer_handle == STATUS_FAIL)
					{
						ERR_PRINT("fail to file_packer_open\r\n");
						stream_ctrl->file_stream_enable = 0;
						continue;
					}

					stream_ctrl->file_stream_open_file = 0;
					stream_ctrl->file_stream_act = FILE_STREAM_ACT_WRITE;

                    msgQSend(stream_ctrl->app_notify_q, MSG_APQ_CYCLIC_VIDEO_RECORD, NULL, 0, MSG_PRI_NORMAL);
				}
				break;

				default:
				break;
			}
		} // if(stream_ctrl->file_stream_act != FILE_STREAM_ACT_WRITE)

		if(msgQReceive(stream_ctrl->file_stream_queue, &msg_id, &trackPkt, sizeof(trackPkt)) == STATUS_FAIL)
		{
			continue;
		}

		//DBG_PRINT("msg_id:0x%x\r\n",msg_id);
		switch(msg_id)
		{
        case MSG_FILE_PACKER_WRITE:
            if(stream_ctrl->file_stream_cyclic && stream_ctrl->file_stream_open_file && file_packer_timer_flag)
            {
                if(trackPkt.track_id == MOV_TRACK_VID_RECORD)
                {
                    gpMuxPkt_t *pkt;

                    pkt = &trackPkt.pkt;
                    if(pkt->frameType == H264_FRAME_TYPE_I)
                    {
                        msgQSend(stream_ctrl->file_stream_queue, MSG_FILE_PACKER_WRITE, &trackPkt, sizeof(trackPkt), MSG_PRI_URGENT);   //send this frame back
                        stream_ctrl->file_stream_act = FILE_STREAM_ACT_REOPEN_SOFT;
                        break;
                    }
                }
            }

            if(stream_ctrl->file_packer_handle)
            {
                #if 0
                if(stream_ctrl->current_rec_file_handle < 0)
                {
                    if(vid_info->rec_file_handle > -1)
                    {
                        stream_ctrl->current_rec_file_handle = vid_info->rec_file_handle;
                        vid_info->rec_file_handle = -1;
                        file_packer->set(stream_ctrl->file_packer_handle, MUX_FILE_HANDLE, stream_ctrl->current_rec_file_handle);

                        osSuspend();
                        DBG_PRINT("new file handle = %d\r\n", stream_ctrl->current_rec_file_handle);
                        osResume();
                    }
                }
                #endif
            }
            else
            {
                ERR_PRINT("file packer error\r\n");
            }

            file_packer_write(stream_ctrl, &trackPkt);
            #if GPS_INFO_PACK || GPS_INFO_PACK_2
            if(trackPkt.track_id != MOV_TRACK_AUD_RECORD)
            {
                if(mov_gps_info_create_flag == 0)
                {
                    mov_gps_info_create_flag = 1;
                    mov_gps_info_create(stream_ctrl->file_packer_handle,vid_src);
                }

                if(mov_gps_info_create_flag)
                {
                    mov_gps_frame_add(vid_src);
                }
                track_ctrl = &mov_track[trackPkt.vid_src][trackPkt.track_id];
                if (last_time)
                {
                    if ((track_ctrl->track_total_ms - last_time) >= 1000)
                    {
                        last_time = track_ctrl->track_total_ms;
                        if(mov_gps_info_create_flag)
                        {
                            mov_gps_data_add(vid_src);
                            //DBG_PRINT("V[%d,%d]",vid_src,last_time/1000);
                        }
                    }
                }
                else
                {
                    last_time = track_ctrl->track_total_ms;
                }
            }
            else
            {
                if(mov_gps_info_create_flag)
                {
                    mov_gps_data_write(stream_ctrl->file_packer_handle,file_packer,vid_src,trackPkt.pkt.size, 1);
                }
            }
            #endif
            break;
        case MSG_FILE_PACKER_CLOSE:
            stream_ctrl->file_stream_act = FILE_STREAM_ACT_CLOSE;
            break;
		}
	} // endof while(stream_ctrl->file_stream_enable)

FAIL_FILE_PACKER_TASK:
	stream_ctrl->file_stream_thread = 0;

	file_packer_uninit(vid_src,stream_ctrl);
	stream_ctrl->file_packer_handle = 0;
	mov_gps_info_uninit(vid_src);
	//DBG_PRINT("Exit packer task\r\n");
	osThreadTerminate(NULL);
}

osThreadId
file_packer_task_create(
	void *arg
)
{
	osThreadDef_t file_packer_task = {"file_packer_task", file_packer_task_entry, PRIORITY_FILE_PACKER, 1, FILE_PACKER_TASK_STACK_SIZE};

	return osThreadCreate(&file_packer_task, arg);
}

INT32S
file_packer_start(
	INT32U src,
	MSG_Q_ID notifyQ,
	VID_INFO* args
)
{
	INT32S nRet = STATUS_OK;
	FILE_STREAM_CTRL_T *stream_ctrl;
	//INT8U rec_time_cfg;
	INT8U rec_min_time;
	INT8U cyclic_rec_cfg;
#if C_TIME_LAPSE
	INT8U lapse,fps;
#endif
	DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	stream_ctrl = &file_stream_ctrl[src];
	memset(stream_ctrl,0,sizeof(FILE_STREAM_CTRL_T));

	stream_ctrl->vid_src = src;
	stream_ctrl->pVid_info = args;
	stream_ctrl->app_notify_q = notifyQ;

	file_packer_err = 0;

    //Init max recording time
    #if 0 // Always turn on cyclic recording
    rec_time_cfg = ap_state_config_record_time_get();
    if(rec_time_cfg)
        cyclic_rec_cfg = 1;
    else
        cyclic_rec_cfg = 0;
    #else
        cyclic_rec_cfg = 1;
    #endif

    rec_min_time = ap_state_config_record_minute_get();

    DBG_PRINT("rec time:%d min\r\n",rec_min_time);
    DBG_PRINT("cyclic rec:%d\r\n",cyclic_rec_cfg);

    file_packer_set_max_time(src,rec_min_time);
    //Init recording mode
    file_packer_set_cyclic(src,cyclic_rec_cfg);

	/************************************************************/
	file_packer_timer_max = stream_ctrl->file_stream_max_ms;
#if C_TIME_LAPSE
	lapse = ap_state_config_time_lapse_recording_temp_get();
	if(lapse > 0)
	{
		fps = ap_state_config_time_lapse_fps_get();
		file_packer_timer_max -= rec_min_time*2*1000;//correct lapse record time by antion@20230419
		file_packer_timer_max = file_packer_timer_max * lapse * fps;
	}
#endif
	file_packer_timer = 0;
	file_packer_timer_flag = 0;

    prio_flag = 0;
    /************************************************************/

    stream_ctrl->file_stream_enable = 1;

    if(file_packer_task_init(stream_ctrl) == STATUS_FAIL)
    {
        RETURN(STATUS_FAIL);
    }

    stream_ctrl->file_stream_thread	= file_packer_task_create((void*)stream_ctrl);
    if(stream_ctrl->file_stream_thread == NULL)
    {
        ERR_PRINT("fail to file_packer_task_create()\n");
        RETURN(STATUS_FAIL);
    }

	return STATUS_OK;

Return:
	file_packer_task_uninit(stream_ctrl);
	return nRet;
}

INT32S
file_packer_stop(
	INT32U src
)
{
	INT32S nRet = STATUS_OK;
	FILE_STREAM_CTRL_T *stream_ctrl;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

    /************************************************************/
    file_packer_timer_max = 0;
    file_packer_timer = 0;
    file_packer_timer_flag = 0;
    /************************************************************/

	stream_ctrl = &file_stream_ctrl[src];
	if(stream_ctrl->file_stream_enable)
	{
		if(stream_ctrl->file_stream_queue && stream_ctrl->file_stream_ack)
		{
			//DBG_PRINT("[%d]MSG_FILE_PACKER_CLOSE\r\n", src);
			POST_MESSAGE(stream_ctrl->file_stream_queue, MSG_FILE_PACKER_CLOSE, stream_ctrl->file_stream_ack, 20000, MSG_PRI_URGENT);
		}
	}
Return:
    file_packer_task_uninit(stream_ctrl);
    if(nRet != STATUS_OK)
    {
        ERR_PRINT("[%d]fail to file_packer_stop\r\n",src);
    }
	return nRet;
}

INT32S
front_packer_start(
	MSG_Q_ID notifyQ,
	VID_INFO* args
)
{
	return(file_packer_start(PREVIEW_SRC_FRONT,notifyQ,args));
}

INT32S
front_packer_stop(
	void
)
{
	return(file_packer_stop(PREVIEW_SRC_FRONT));
}

INT32S
rear_packer_start(
	MSG_Q_ID notifyQ,
	VID_INFO* args
)
{
	return(file_packer_start(PREVIEW_SRC_REAR,notifyQ,args));
}

INT32S
rear_packer_stop(
	void
)
{
	return(file_packer_stop(PREVIEW_SRC_REAR));
}

INT32S file_packer_set_file_handle(INT32U src)
{
	FILE_STREAM_CTRL_T *stream_ctrl = &file_stream_ctrl[src];
	VID_INFO* vid_info = stream_ctrl->pVid_info;

    osSuspend();
    if((stream_ctrl->file_packer_handle) && (stream_ctrl->file_stream_act == FILE_STREAM_ACT_WRITE))
    {
        if(stream_ctrl->current_rec_file_handle < 0)
        {
            if(vid_info->rec_file_handle > -1)
            {
                MP4MuxInfo_t *gpPackBitsInfo = (MP4MuxInfo_t *)stream_ctrl->file_packer_handle;

                stream_ctrl->current_rec_file_handle = vid_info->rec_file_handle;
                gpPackBitsInfo->gPackFd = vid_info->rec_file_handle;
                vid_info->rec_file_handle = -1;
            }
        }
    }
    osResume();
    return  STATUS_OK;
}


INT32U MOOV_HEADER_SIZE(void)
{
    return 320*1024;
}


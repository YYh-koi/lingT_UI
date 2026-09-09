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
#include "circular_buffer.h"
#include "application.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
enum {
	MAX_AUD_CAPTURE_NUM = (7 * AUD_RECORD_SAMPLE_RATE) / SND_REC_SAMPLING_FRAME + 1,	/* audio capture number, keep 3s max */
	//  CAPTURE STATUS
	CAPTURE_IDLE = 0,
	CAPTURE_INIT = 1,
	CAPTURE_RUN = 2
};


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
struct _hwparams {
	char capture_mute;
	char capture_ramp_up;
	char capture_ramp_down;
	void* capture_head_addr;
	void* capture_cb_addr; // For circular buffer
	cbuf_handle_t cbuf_capture_free;
};

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern INT32S
file_packer_write_notify(
	FILE_PACKER_W_ARGS* pFile_packer_args
);

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static struct _hwparams  aud_rec_hwparams = {0};
static INT32U ready_addr, next_addr; //Content: AUD_CAP_BUF_HEADER + pcm data
static DMA_STRUCT adc_dma_dbf;
static INT8U aud_rec_dev_num = 0;

/*== D E B U G ==*/
#if 1
static INT32U no_sampling_buf_cnt = 0;
#endif
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
ap_audio_record_dev_num_find(
	void
)
{
	VID_INFO*  pVid_info_front = (VID_INFO*)&pStreamArgs->vid_info_front;
	VID_INFO*  pVid_info_rear = (VID_INFO*)&pStreamArgs->vid_info_rear;

	//DBG_PRINT("==Run:%s==%d\r\n",__FUNCTION__,aud_rec_dev_num);

	aud_rec_dev_num = 0;

	if(pVid_info_front->data_source != VID_SRC_NONE)
	{
		aud_rec_dev_num++;
	}

	if(back_sensor_plug_status_get() && (pVid_info_rear->data_source != VID_SRC_NONE))
	{
		aud_rec_dev_num++;
	}
	
}

INT32S
ap_audio_double_buffer_put(
	INT32U data,
	INT32U cwlen,
	void (*dma_user_isr)(INT8U, INT8U)
)
{
	INT32S ret_err;

	if(data != DUMMY_BUFFER_ADDRS)
	{
		data += sizeof(AUD_CAP_BUF_HEADER);
	}

	adc_dma_dbf.s_addr = (INT32U) P_I2SRX_DATA;
	adc_dma_dbf.t_addr = (INT32U) data;
	adc_dma_dbf.width = DMA_DATA_WIDTH_4BYTE;
	adc_dma_dbf.count = (INT32U) cwlen / 2;
	adc_dma_dbf.notify = NULL;
	adc_dma_dbf.timeout = 0;

	ret_err = drv_l1_dma_transfer_double_buf_with_callback(&adc_dma_dbf, NULL, dma_user_isr);
	if(ret_err < 0)
	{
		ERR_PRINT("==fail to put dma double buf==\r\n");
	}

	return ret_err;
}

INT32S
ap_audio_double_buffer_set(
	INT32U data,
	INT32U cwlen
)
{
	INT32S ret_err;
	//DBG_PRINT("==%s==\r\n",__FUNCTION__);
	if(data != DUMMY_BUFFER_ADDRS)
	{
		data += sizeof(AUD_CAP_BUF_HEADER);
	}

	adc_dma_dbf.s_addr = (INT32U) P_I2SRX_DATA;
	adc_dma_dbf.t_addr = (INT32U) data;
	adc_dma_dbf.width = DMA_DATA_WIDTH_4BYTE;
	adc_dma_dbf.count = (INT32U) cwlen / 2;
	adc_dma_dbf.notify = NULL;
	adc_dma_dbf.timeout = 0;

	ret_err = drv_l1_dma_transfer_double_buf_set(&adc_dma_dbf);
	if(ret_err < 0)
	{
		ERR_PRINT("==fail to set dma double buf==\r\n");
	}

	return ret_err;
}

void ap_audio_double_buffer_free(void)
{
	drv_l1_dma_transfer_double_buf_free(&adc_dma_dbf);
	adc_dma_dbf.channel = 0xff;
}

INT32U
ap_audio_get_next_buffer(
	void
)
{
	int nRet;
	unsigned int data = DUMMY_BUFFER_ADDRS;

	nRet = circular_buf_get(aud_rec_hwparams.cbuf_capture_free, &data);
	if(nRet < 0)
	{
		#if 0 // Debug
		no_sampling_buf_cnt++;
		if (no_sampling_buf_cnt<16 || (no_sampling_buf_cnt&0xFF)==0) {
			DBG_PRINT("ap_audio_get_next_buffer get DUMMY_BUFFER_ADDRS, cnt=%d\r\n", no_sampling_buf_cnt);
		}
		#endif
        DBG_PRINT("&");
		return DUMMY_BUFFER_ADDRS;
	}

	//DBG_PRINT("get buf:0x%x\r\n",data);
	return data;
}

void
ap_audio_free_cap_buffer(
	INT32U data //Content: AUD_CAP_BUF_HEADER + pcm data
)
{
	int nRet;
	AUD_CAP_BUF_HEADER* p_aud_cap_buf_header = (AUD_CAP_BUF_HEADER*)data;

	//DBG_PRINT("free buf:0x%x header:0x%x\r\n",data,p_aud_cap_buf_header);

	osSuspend();

	if(p_aud_cap_buf_header->copies != 0)
	{
		p_aud_cap_buf_header->copies--;
	}

	if(p_aud_cap_buf_header->copies == 0)
	{
		nRet = circular_buf_try_put(aud_rec_hwparams.cbuf_capture_free,(INT32U)p_aud_cap_buf_header);
		if(nRet < 0)
		{
			ERR_PRINT("fail to run circular_buf_try_put\r\n");
		}
	}

	osResume();
}

void
ap_audio_adc_hw_start(
	INT16U sampling_rate
)
{
	//DBG_PRINT("==%s==\r\n",__FUNCTION__);
	drv_l1_adc_mic_enable();
	drv_l1_adc_mic_boost_pga(1,0);
	drv_l1_i2s_rx_adc_set_input_path(MIC_INPUT);
	drv_l1_i2s_rx_adc_init();
	drv_l1_i2s_rx_adc_sample_rate_set(sampling_rate);
	drv_l1_i2s_rx_adc_mono_ch_set();
	drv_l1_i2s_rx_adc_start();
}

void
ap_audio_adc_hw_stop(
	void
)
{
	drv_l1_adc_mic_disable();
	drv_l1_i2s_rx_adc_stop();
	drv_l1_i2s_rx_adc_exit();
}

INT32S
ap_audio_record_buf_exchange(
	void
)
{
	int ret_err;
	//INT32U ret_next;

	ready_addr = next_addr;
	next_addr = ap_audio_get_next_buffer();
	ret_err = ap_audio_double_buffer_set(next_addr,SND_REC_SAMPLING_FRAME);

	return ret_err;
}

INT32S
ap_audio_record_buf_init(
	void
)
{
	INT32U i;
	INT32U block_size = (sizeof(AUD_CAP_BUF_HEADER) + SND_REC_SAMPLING_FRAME * 2);

	block_size += (64 - (block_size & 0x3f)) & 0x3f;

	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	// Init audio capture buffer
	aud_rec_hwparams.capture_head_addr = (char*)gp_malloc_align(MAX_AUD_CAPTURE_NUM*block_size,64);
	if (aud_rec_hwparams.capture_head_addr == NULL)
	{
		ERR_PRINT("fail to aud_rec_hwparams's capture_head_addr\r\n");
		return STATUS_FAIL;
	}

	// audio's circular_buffer
	aud_rec_hwparams.capture_cb_addr = gp_malloc_align(sizeof(void*)*MAX_AUD_CAPTURE_NUM,64);
	if(aud_rec_hwparams.capture_cb_addr == 0)
	{
		ERR_PRINT("fail to aud_rec_hwparams's capture_cb_addr\r\n");
		return STATUS_FAIL;
	}

	aud_rec_hwparams.cbuf_capture_free = circular_buf_init(aud_rec_hwparams.capture_cb_addr,MAX_AUD_CAPTURE_NUM);
	if(aud_rec_hwparams.cbuf_capture_free == 0)
	{
		ERR_PRINT("fail to aud_rec_hwparams's cbuf_capture_free\r\n");
		return STATUS_FAIL;
	}

	#if 0
	DBG_PRINT("[Audio] head:0x%x cb_addr:0x%x free:0x%x\r\n",
	aud_rec_hwparams.capture_head_addr,aud_rec_hwparams.capture_cb_addr,aud_rec_hwparams.cbuf_capture_free);
	#endif

	no_sampling_buf_cnt = 0;
	for (i=0; i<MAX_AUD_CAPTURE_NUM; ++i)
	{
		if(circular_buf_try_put(aud_rec_hwparams.cbuf_capture_free,(INT32U)(aud_rec_hwparams.capture_head_addr+(block_size*i))) < 0)
		{
			ERR_PRINT("fail to put cbuf_capture_free\r\n");
			return STATUS_FAIL;
		}
		DBG_PRINT("Aud[%d]=0x%x\r\n",i,(aud_rec_hwparams.capture_head_addr+(block_size*i)));
	}

	return STATUS_OK;
}

INT32S ap_audio_record_buf_reset(void)
{
    INT32U i;
	INT32U block_size = (sizeof(AUD_CAP_BUF_HEADER) + SND_REC_SAMPLING_FRAME * 2);
	
	//DBG_PRINT("==%s==\r\n",__FUNCTION__);
	block_size += (64 - (block_size & 0x3f)) & 0x3f;

    circular_buf_reset(aud_rec_hwparams.cbuf_capture_free);

	no_sampling_buf_cnt = 0;
	for (i=0; i<MAX_AUD_CAPTURE_NUM; ++i)
	{
		if(circular_buf_try_put(aud_rec_hwparams.cbuf_capture_free,(INT32U)(aud_rec_hwparams.capture_head_addr+(block_size*i))) < 0)
		{
			ERR_PRINT("fail to put cbuf_capture_free\r\n");
			return STATUS_FAIL;
		}
		//DBG_PRINT("Aud[%d]=0x%x\r\n",i,(aud_rec_hwparams.capture_head_addr+(block_size*i)));
	}
	return STATUS_OK;
}


INT32S
ap_audio_record_buf_release(
	void
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(aud_rec_hwparams.capture_head_addr)
	{
		gp_free(aud_rec_hwparams.capture_head_addr);
		aud_rec_hwparams.capture_head_addr = 0;
	}

	if(aud_rec_hwparams.capture_cb_addr)
	{
		gp_free(aud_rec_hwparams.capture_cb_addr);
		aud_rec_hwparams.capture_cb_addr = 0;
	}

	if(aud_rec_hwparams.cbuf_capture_free)
	{
		gp_free(aud_rec_hwparams.cbuf_capture_free);
		aud_rec_hwparams.cbuf_capture_free = 0;
	}

	return STATUS_OK;
}

void
ap_audio_dma_done_callback(
	INT8U channel,
	INT8U n
)
{
	if(adc_dma_dbf.channel == channel)
	{
        if(n == C_DMA_STATUS_DONE)
        {
            msgQSend(audioEncTaskQ, MSG_AUD_DMA_DONE, NULL, 0, MSG_PRI_NORMAL);
        }
        else
        {
            ERR_PRINT("error msg = 0x%x\r\n", n);
        }
	}
}

INT32S
ap_audio_record_device_open(
	void
)
{
	int ret_err;
	//INT32U ret_next;
	//DBG_PRINT("==%s==\r\n",__FUNCTION__);

	/*
	if(ap_audio_record_buf_init() == STATUS_FAIL)
	{
		goto ERR_AP_AUDIO_RECORD_DEVICE_OPEN;
	} */

    ap_audio_record_buf_reset();

	ready_addr = ap_audio_get_next_buffer();
	ret_err = ap_audio_double_buffer_put(ready_addr,SND_REC_SAMPLING_FRAME,ap_audio_dma_done_callback);
	if(ret_err < 0)
	{
		goto ERR_AP_AUDIO_RECORD_DEVICE_OPEN;
	}

	next_addr = ap_audio_get_next_buffer();
	ret_err = ap_audio_double_buffer_set(next_addr,SND_REC_SAMPLING_FRAME);
	if(ret_err < 0)
	{
		goto ERR_AP_AUDIO_RECORD_DEVICE_OPEN;
	}

	ap_audio_adc_hw_start(SND_REC_SAMPLING_RATE);

	return STATUS_OK;

ERR_AP_AUDIO_RECORD_DEVICE_OPEN:

	//ap_audio_record_buf_release();

	return STATUS_FAIL;
}

INT32S
ap_audio_record_device_close(
	void
)
{
	ap_audio_adc_hw_stop();
	//ap_audio_record_buf_release();
	ap_audio_double_buffer_free();
	return STATUS_OK;
}

INT32S
ap_audio_capture(
	INT32U p_pcm
)
{
	INT32S nRet = STATUS_OK;
	INT32U i;
	AUD_CAP_BUF_HEADER* p_aud_cap_buf_header = (AUD_CAP_BUF_HEADER*)p_pcm;
	FILE_PACKER_W_ARGS File_packer_args;

	if(p_pcm == DUMMY_BUFFER_ADDRS)
	{
		return STATUS_FAIL;
	}

#if (defined C_USB_AUDIO) && (C_USB_AUDIO)
	if(state_usbd_status_get()) // Connect to PC
	{
		p_aud_cap_buf_header->copies = 1;
		p_pcm += sizeof(AUD_CAP_BUF_HEADER);
		nRet = usb_audio_send(p_pcm,(SND_REC_SAMPLING_FRAME<<1),(INT32U)p_aud_cap_buf_header);
		if(nRet == STATUS_FAIL)
		{
			p_aud_cap_buf_header->copies = 0;
			ap_audio_free_cap_buffer((INT32U)p_aud_cap_buf_header);
		}		

		return STATUS_OK;		
	}
#endif

	ap_audio_record_dev_num_find();
	
	p_aud_cap_buf_header->copies = aud_rec_dev_num;

#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
#if C_TIME_LAPSE
	if(ap_state_config_time_lapse_recording_get() != 0)
		p_aud_cap_buf_header->copies = 0;
#endif
	voice_recg_task_process(p_pcm);
	#endif

	p_pcm += sizeof(AUD_CAP_BUF_HEADER);
#if 0	//靜音改用0xD2F00000 當聲音source
    if(ap_state_config_voice_record_switch_get() == 0)
    {


        INT16U	*ptr = (INT16U *)p_pcm;
        for(i = 0; i < SND_REC_SAMPLING_FRAME; i++)
        {
            *ptr++ = 0;
        }
    }
#endif
#if C_TIME_LAPSE
	if(ap_state_config_time_lapse_recording_get() != 0)
	{
#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
		// Need a pcm buffer for voice recognition
#else
		p_aud_cap_buf_header->copies = 0;
		ap_audio_free_cap_buffer((INT32U)p_aud_cap_buf_header);
#endif
		return STATUS_OK;
	}
#endif

	for(i=0; i<aud_rec_dev_num; i++)
	{
		File_packer_args.src = i;
		File_packer_args.snapshot = 0;
		File_packer_args.track = MOV_TRACK_AUD_RECORD;
		File_packer_args.type = 0;
		File_packer_args.ret_data = (INT32U)p_aud_cap_buf_header;
		if(ap_state_config_voice_record_switch_get() == 0)
		{	//靜音改用0xD2F00000 當聲音source
			File_packer_args.data = (INT8U*)0xD2F00000;
		}
		else
		{
			File_packer_args.data = (unsigned char *)p_pcm;//packet data
		}
		File_packer_args.size = SND_REC_SAMPLING_FRAME*2; // packet size
		File_packer_args.pts = SND_REC_SAMPLING_MS; // ms
		File_packer_args.frameType = 0; // 0 = I, 1 = P,  2 = B for video, 1 = key frame for audio
		File_packer_args.drop_time = NULL;

		nRet = file_packer_write_notify(&File_packer_args);
		if(nRet == STATUS_FAIL)
		{
			ap_audio_free_cap_buffer((INT32U)p_aud_cap_buf_header);
		}

	}
	return STATUS_OK;
}

INT32U
ap_audio_done_buf(
	void
)
{
	return ready_addr;
}


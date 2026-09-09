/**************************************************************************************************
audio record to storage:
The encode format can be
1. A1800 (GP)
2. WAVE  (PCM)
3. WAVE  (microsoft ADPCM)
4. WAVE  (IMA ADPCM)
5. MP3	 sample rate, 48k, 44.1K, 32k, 24K, 22.05k, 16K, 12K, 11.025K, 8k
		 bit rate, 32,40,48,56,64,80,96,112,128,160,192,224,256,320 kbps
		  			 8,16,24,32,40,48,56, 64, 80, 96,112,128,144,160 kbps
***************************************************************************************************/
#include "audio_record.h"
#include "audio_codec_callback.h"
#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
#define APP_AUDIO_STREAM_EN 1

/*  define */
#define ADC_RECORD_STACK_SIZE			1024
#define AUD_ENC_QACCEPT_MAX				5
#define AUD_ENC_ACK_QUEUE_MAX			1
#define RECORD_CLUSTER_SHIFT			16
#define RECORD_CLUSTER_SIZE			    (1<<RECORD_CLUSTER_SHIFT)       //64K


#define fs_open         open
#define fs_close        close
#define fs_read         read
#define fs_write        write

/* os stack */
#if _OPERATING_SYSTEM != _OS_NONE
  #if _OPERATING_SYSTEM == _OS_UCOS2
  INT32U  AdcRecordStack[ADC_RECORD_STACK_SIZE];
  INT32U  AdcRecordWriteStack[ADC_RECORD_STACK_SIZE];
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
  #define AUD_TASK_STACKSIZE 1024
  #endif
#endif
/* os global varaible */
#if _OPERATING_SYSTEM != _OS_NONE
#if _OPERATING_SYSTEM == _OS_UCOS2
  void	 	*aud_enc_reqQ_area[AUD_ENC_QACCEPT_MAX];
  void	 	*aud_enc_reqQ_area2[AUD_ENC_QACCEPT_MAX];
  void	 	*aud_enc_writeQ_area[AUD_ENC_QACCEPT_MAX];
  OS_EVENT 	*aud_enc_reqQ;
  OS_EVENT 	*aud_enc_reqQ2;
  OS_EVENT 	*aud_enc_writeQ;
  OS_EVENT    *aud_write_task_ack_m;
  #if RECORD_WITH_DAC_OUT_EN == 1
  void	 	*aud_enc_reqQ_area3[AUD_ENC_QACCEPT_MAX];
  OS_EVENT 	*aud_enc_reqQ3;
  #endif
#elif _OPERATING_SYSTEM == _OS_FREERTOS
  xQueueHandle 	aud_enc_reqQ;
  xQueueHandle 	aud_enc_reqQ2;
  xQueueHandle 	aud_enc_writeQ;
  xQueueHandle  aud_write_task_ack_m;
  #if RECORD_WITH_DAC_OUT_EN == 1
  xQueueHandle 	aud_enc_reqQ3;
  #endif
#endif
#endif

/* global varaible */
static Audio_Encode_Para *pAudio_Encode_Para;

/* function */
static void adc_record_entry(void *param);
static void adc_record_write_entry(void *param);
//dma
#if _OPERATING_SYSTEM != _OS_NONE
#if _OPERATING_SYSTEM == _OS_UCOS2
  static INT32S aud_adc_double_buffer_put(INT16U *data,INT32U cwlen, INT8U aud_in, OS_EVENT *os_q);
#elif _OPERATING_SYSTEM == _OS_FREERTOS
  static INT32S aud_adc_double_buffer_put(INT16U *data,INT32U cwlen, INT8U aud_in, xQueueHandle os_q);
#endif
#endif
static INT32U aud_adc_double_buffer_set(INT16U *data, INT32U cwlen, INT8U aud_in);
static INT32S aud_adc_dma_status_get(INT8U aud_in);
static void aud_adc_double_buffer_free(INT8U aud_in);
static INT32S adc_hardware_start(INT8U InputDevice, INT8U channel, INT32U SampleRate);
static INT32S adc_hardware_stop(INT8U InputDevice, INT8U channel);

//other
static INT32S adc_memory_allocate(INT32U CbLen, INT8U adc_in);
static INT32S adc_memory_free(void);
static void adc_work_mem_free(void);

//wave
static AUD_ENC_WAVE_HEADER	WaveHeadPara;
static void aud_enc_RIFF_init(INT32U samplerate);
static INT32S wave_encode_start(void);
static INT32S wave_encode_stop(void);
static int wave_encode_once(void *workmem, const short* buffer_addr, int cwlen);

#if BUILD_IN_MIC_EN == 1
extern INT32S mic_timer_stop(INT8U timer_id);
extern void mic_fifo_clear(void);
extern void mic_fifo_level_set(INT8U level);
extern INT32S mic_auto_sample_start(void);
extern INT32S mic_sample_rate_set(INT8U timer_id, INT32U hz);
#endif

int max_audio_record_file_size = 0;
extern xSemaphoreHandle lwip_socket_sem;
extern void ap_audio_adc_hw_start(INT16U sampling_rate);
extern void ap_audio_adc_hw_stop(void);
//===============================================================================================
// 	audio mic/linein initial
//	parameter:	mic/linein.
//	return:		none.
//===============================================================================================
void aud_adc_input_init(INT32U input_sel, INT32U sr_hz)
{

    if(input_sel == BUILD_IN_MIC)
        drv_l1_i2s_rx_adc_set_input_path(0);
//        drv_l1_adc_mic_enable();
    else
        drv_l1_i2s_rx_adc_set_input_path(1);
//        drv_l1_adc_linein_enable();

    //drv_msec_wait(200);
	drv_l1_i2s_rx_adc_init();

    drv_l1_i2s_rx_adc_sample_rate_set(sr_hz);

	if(input_sel == BUILD_IN_MIC) {
		drv_l1_i2s_rx_set_mono(0,1);
	} else {
		drv_l1_i2s_rx_set_mono(0,0);
	}

	drv_l1_i2s_rx_adc_start();

}
//===============================================================================================
// 	audio mic/linein stop
//	parameter:	mic/linein.
//	return:		none.
//===============================================================================================
void aud_adc_input_stop(void)
{
    drv_l1_i2s_rx_adc_stop();
    drv_l1_i2s_rx_adc_exit();
}

//===============================================================================================
// 	audio task create
//	parameter:	priority.
//	return:		none.
//===============================================================================================
INT32S adc_record_task_create(INT8U priority)
{
	INT8U err;

#if _OPERATING_SYSTEM != _OS_NONE
    #if _OPERATING_SYSTEM == _OS_UCOS2
        aud_enc_reqQ = OSQCreate(aud_enc_reqQ_area, AUD_ENC_QACCEPT_MAX);
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
        aud_enc_reqQ = xQueueCreate(AUD_ENC_QACCEPT_MAX, sizeof(INT32U));
    #endif
#endif

	if (!aud_enc_reqQ) {
		return STATUS_FAIL;
	}

#if _OPERATING_SYSTEM != _OS_NONE
    #if _OPERATING_SYSTEM == _OS_UCOS2
        aud_enc_reqQ2 = OSQCreate(aud_enc_reqQ_area2, AUD_ENC_QACCEPT_MAX);
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
        aud_enc_reqQ2 = xQueueCreate(AUD_ENC_QACCEPT_MAX, sizeof(INT32U));
    #endif
#endif
	if (!aud_enc_reqQ2) {
		return STATUS_FAIL;
	}

#if _OPERATING_SYSTEM != _OS_NONE
    #if _OPERATING_SYSTEM == _OS_UCOS2
        aud_enc_writeQ = OSQCreate(aud_enc_writeQ_area, AUD_ENC_QACCEPT_MAX);
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
        aud_enc_writeQ = xQueueCreate(AUD_ENC_QACCEPT_MAX, sizeof(INT32U));
    #endif
#endif
	if (!aud_enc_writeQ) {
		return STATUS_FAIL;
	}

#if _OPERATING_SYSTEM != _OS_NONE
    #if _OPERATING_SYSTEM == _OS_UCOS2
        aud_write_task_ack_m = OSMboxCreate(NULL);
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
        aud_write_task_ack_m = xQueueCreate(AUD_ENC_ACK_QUEUE_MAX, sizeof(INT32U));
    #endif
#endif
	if (!aud_write_task_ack_m) {
		return STATUS_FAIL;
	}



#if RECORD_WITH_DAC_OUT_EN == 1

#if _OPERATING_SYSTEM != _OS_NONE
    #if _OPERATING_SYSTEM == _OS_UCOS2
        aud_enc_reqQ3 = OSQCreate(aud_enc_reqQ_area3, AUD_ENC_QACCEPT_MAX);
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
        aud_enc_reqQ3 = xQueueCreate(AUD_ENC_QACCEPT_MAX, sizeof(INT32U));
    #endif
#endif
	if (!aud_enc_reqQ3) {
		return STATUS_FAIL;
	}
#endif

	if(pAudio_Encode_Para == 0) {
		pAudio_Encode_Para = (Audio_Encode_Para *)gp_malloc_align(sizeof(Audio_Encode_Para), 4);
		if(pAudio_Encode_Para == 0) {
			return STATUS_FAIL;
		}
	}

	gp_memset((INT8S *)pAudio_Encode_Para, 0, sizeof(Audio_Encode_Para));
#if ((defined MCU_VERSION) && (MCU_VERSION < GPL326XX))
	pAudio_Encode_Para->InputDevice = ADC_LINE_IN;
#else
	pAudio_Encode_Para->InputDevice = BUILD_IN_MIC;
#endif
	pAudio_Encode_Para->Channel = C_MONO_RECORD;

#if _OPERATING_SYSTEM != _OS_NONE
    #if _OPERATING_SYSTEM == _OS_UCOS2
	err = OSTaskCreate(adc_record_entry, (void *)pAudio_Encode_Para, (void *)&AdcRecordStack[ADC_RECORD_STACK_SIZE - 1], priority);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
    #elif _OPERATING_SYSTEM == _OS_FREERTOS

        err = xTaskCreate(adc_record_entry, "aud_rec", AUD_TASK_STACKSIZE, pAudio_Encode_Para, priority, NULL);

        if(err != pdPASS) {
		return STATUS_FAIL;
	}

    #endif

    #if _OPERATING_SYSTEM == _OS_UCOS2
	err = OSTaskCreate(adc_record_write_entry, (void *)pAudio_Encode_Para, (void *)&AdcRecordWriteStack[ADC_RECORD_STACK_SIZE - 1], priority + 1);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
    #elif _OPERATING_SYSTEM == _OS_FREERTOS

        err = xTaskCreate(adc_record_write_entry, "aud_write", AUD_TASK_STACKSIZE, pAudio_Encode_Para, priority, NULL);

        if(err != pdPASS) {
		return STATUS_FAIL;
	}

    #endif

#endif

	DBG_PRINT("AudioEncodeTaskCreate[%d]\r\n", priority);
	return STATUS_OK;
}

//===============================================================================================
// 	audio task detete
//	parameter:	priority.
//	return:		none.
//===============================================================================================
INT32S adc_record_task_del(INT8U priority)
{
	INT8U err = 0;

#if _OPERATING_SYSTEM != _OS_NONE
    #if _OPERATING_SYSTEM == _OS_UCOS2
	if(aud_enc_reqQ) {
		OSQFlush(aud_enc_reqQ);
	}

	if(aud_enc_reqQ2) {
		OSQFlush(aud_enc_reqQ2);
	}

#if RECORD_WITH_DAC_OUT_EN == 1
	if(aud_enc_reqQ3) {
		OSQFlush(aud_enc_reqQ3);
	}
#endif

    err = OSTaskDel(priority);
	DBG_PRINT("AudioEncodeTaskDel[%d]\r\n", priority);


	if(aud_enc_reqQ) {
		OSQDel(aud_enc_reqQ, OS_DEL_ALWAYS, &err);
		aud_enc_reqQ = 0;
	}

	if(aud_enc_reqQ2) {
		OSQDel(aud_enc_reqQ2, OS_DEL_ALWAYS, &err);
		aud_enc_reqQ2 = 0;
	}

	if(aud_enc_writeQ) {
		OSQDel(aud_enc_writeQ, OS_DEL_ALWAYS, &err);
		aud_enc_writeQ = 0;
	}

	if(aud_write_task_ack_m) {
		OSMboxDel(aud_write_task_ack_m, OS_DEL_ALWAYS, &err);
		aud_write_task_ack_m = 0;
	}

#if RECORD_WITH_DAC_OUT_EN == 1
	if(aud_enc_reqQ3) {
		OSQDel(aud_enc_reqQ3, OS_DEL_ALWAYS, &err);
		aud_enc_reqQ3 = 0;
	}
#endif

    #elif _OPERATING_SYSTEM == _OS_FREERTOS

    {
        INT32U msg;
        osStatus status;

        msg = MSG_AUDIO_ENCODE_EXIT;
        status = osMessagePut(aud_enc_reqQ, (INT32U)&msg, osWaitForever);
        if(status != osOK) {
            DBG_PRINT("MSG_AUDIO_ENCODE_EXIT Error\r\n");
        }

        status = osMessagePut(aud_enc_writeQ, (INT32U)&msg, osWaitForever);
        if(status != osOK) {
            DBG_PRINT("MSG_AUDIO_ENCODE_EXIT Error\r\n");
        }
	}
    osDelay(5);

	if(aud_enc_reqQ) {
		xQueueReset(aud_enc_reqQ);
	}

	if(aud_enc_reqQ2) {
		xQueueReset(aud_enc_reqQ2);
	}

	if(aud_enc_writeQ) {
		xQueueReset(aud_enc_writeQ);
	}

	if(aud_write_task_ack_m) {
		xQueueReset(aud_write_task_ack_m);
	}

#if RECORD_WITH_DAC_OUT_EN == 1
	if(aud_enc_reqQ3) {
		xQueueReset(aud_enc_reqQ3);
	}
#endif

	if(aud_enc_reqQ) {
                vQueueDelete(aud_enc_reqQ);
		aud_enc_reqQ = 0;
	}

	if(aud_enc_reqQ2) {
                vQueueDelete(aud_enc_reqQ2);
		aud_enc_reqQ2 = 0;
	}

    if(aud_enc_writeQ) {
        vQueueDelete(aud_enc_writeQ);
		aud_enc_writeQ = 0;
	}

	if(aud_write_task_ack_m) {
        vQueueDelete(aud_write_task_ack_m);
		aud_write_task_ack_m = 0;
	}
#if RECORD_WITH_DAC_OUT_EN == 1
	if(aud_enc_reqQ3) {
                vQueueDelete(aud_enc_reqQ3);
		aud_enc_reqQ3 = 0;
	}
#endif


    #endif
#endif

	if(pAudio_Encode_Para) {
		gp_free((void *)pAudio_Encode_Para);
		pAudio_Encode_Para = 0;
	}
	return err;
}

//===============================================================================================
// 	adc_envelop_detect_start
//	parameter:	none.
//	return:		status.
//===============================================================================================
INT32S adc_envelop_detect_start(void)
{
	INT8U err;
	INT32S i, status;
	INT32U message;

#if _OPERATING_SYSTEM != _OS_NONE
    #if _OPERATING_SYSTEM == _OS_UCOS2
	err = OSQPost(aud_enc_reqQ, (void *) MSG_AUDIO_ENVELOP_DET_START);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}

    #elif _OPERATING_SYSTEM == _OS_FREERTOS
	message = MSG_AUDIO_ENVELOP_DET_START;
        err = xQueueSend( (xQueueHandle) aud_enc_reqQ, (void *) &message, portMAX_DELAY);
	if(err != pdPASS) {
		return STATUS_FAIL;
	}

    #endif
#endif

	//wait 10 second
	for(i=0; i<1000; i++) {
		status = audio_record_get_status();
		if(status == C_START_RECORD) {
			return STATUS_OK;
		}

		if(status == C_START_FAIL) {
			return STATUS_FAIL;
		}

        #if _OPERATING_SYSTEM != _OS_NONE
            #if _OPERATING_SYSTEM == _OS_UCOS2
		OSTimeDly(1);
            #elif _OPERATING_SYSTEM == _OS_FREERTOS
                vTaskDelay(1);
            #endif
        #endif
	}

	if(i == 1000) {
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

//===============================================================================================
// 	audio record task start
//	parameter:	none.
//	return:		status.
//===============================================================================================
INT32S adc_record_task_start(void)
{
	INT8U err;
	INT32S i, status;
	INT32U message;

#if _OPERATING_SYSTEM != _OS_NONE
    #if _OPERATING_SYSTEM == _OS_UCOS2
	err = OSQPost(aud_enc_reqQ, (void *) MSG_AUDIO_ENCODE_START);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}

    #elif _OPERATING_SYSTEM == _OS_FREERTOS
    DBG_PRINT("%s MSG_AUDIO_ENCODE_START\r\n",__func__);
	message = MSG_AUDIO_ENCODE_START;
        err = xQueueSend( (xQueueHandle) aud_enc_reqQ, (void *) &message, portMAX_DELAY);
	if(err != pdPASS) {
		return STATUS_FAIL;
	}

    #endif
#endif

	//wait 10 second
	for(i=0; i<1000; i++) {
		status = audio_record_get_status();
		if(status == C_START_RECORD) {
			return STATUS_OK;
		}

		if(status == C_START_FAIL) {
			return STATUS_FAIL;
		}

        #if _OPERATING_SYSTEM != _OS_NONE
            #if _OPERATING_SYSTEM == _OS_UCOS2
		OSTimeDly(1);
            #elif _OPERATING_SYSTEM == _OS_FREERTOS
                vTaskDelay(1);
            #endif
        #endif
	}

	if(i == 1000) {
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

//===============================================================================================
// 	audio record task start
//	parameter:	none.
//	return:		status.
//===============================================================================================
INT32S adc_record_task_stop(void)
{
	INT8U err;
	INT32S i, status;
	INT32U message;

#if _OPERATING_SYSTEM != _OS_NONE
    #if _OPERATING_SYSTEM == _OS_UCOS2
	err = OSQPost(aud_enc_reqQ, (void *)MSG_AUDIO_ENCODE_STOPING);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
	message = MSG_AUDIO_ENCODE_STOPING;
    DBG_PRINT("%s MSG_AUDIO_ENCODE_STOPING\r\n",__func__);
        err = xQueueSend( (xQueueHandle) aud_enc_reqQ, (void *) &message, portMAX_DELAY);
	if(err != pdPASS) {
		return STATUS_FAIL;
	}

    #endif
#endif

	//wait 10 second
	//m-for(i=0; i<1000; i++) {
	while(1) {
		status = audio_record_get_status();
		if(status == C_STOP_RECORD) {
			return STATUS_OK;
		}

        #if _OPERATING_SYSTEM != _OS_NONE
            #if _OPERATING_SYSTEM == _OS_UCOS2
		OSTimeDly(1);
            #elif _OPERATING_SYSTEM == _OS_FREERTOS
                vTaskDelay(1);
            #endif
        #endif
	}

	if(i == 1000) {
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

//===============================================================================================
// 	audio_record_set_status
//	parameter:	status
//	return:
//===============================================================================================
void audio_record_set_status(INT32U status)
{
	pAudio_Encode_Para->Status = status;
}

//===============================================================================================
// 	audio_record_get_status
//	parameter:
//	return:		status.
//===============================================================================================
INT32U audio_record_get_status(void)
{
	return pAudio_Encode_Para->Status;
}

//===============================================================================================
// 	audio_record_set_source_type
//	parameter:	type = GP_FS/user_define
//	return:
//===============================================================================================
void audio_record_set_source_type(INT32U type)
{
	pAudio_Encode_Para->SourceType = type;
}

//===============================================================================================
// 	audio_record_get_source_type
//	parameter:
//	return:		type.
//===============================================================================================
INT32U audio_record_get_source_type(void)
{
	return pAudio_Encode_Para->SourceType;
}

//===============================================================================================
// 	audio_record_set_file_handle_free_size
//	parameter:	file_handle = wave file handle
//	return:		status.
//===============================================================================================
INT64U audio_record_set_file_handle_free_size(INT16S file_handle)
{
	INT8U  fs_disk;
	INT64U disk_free = 0;

	if(file_handle >= 0) {
#if 1 //m-
		fs_disk = GetDiskOfFile(file_handle);
		disk_free = vfsFreeSpace(fs_disk);
		pAudio_Encode_Para->FileHandle = file_handle;
		pAudio_Encode_Para->disk_free_size = disk_free - 100*1024; //reserved 100K
#else
		disk_free = 100*1024*1024;
		pAudio_Encode_Para->FileHandle = file_handle;
		pAudio_Encode_Para->disk_free_size = disk_free - 10*1024; //reserved 10K
#endif
	} else {
		pAudio_Encode_Para->FileHandle = -1;
		pAudio_Encode_Para->disk_free_size = 0;
	}
	return disk_free;
}

//===============================================================================================
// 	audio_record_set_info
//	parameter:	audio_format = format
//				sample_rate =
//				bit_rate =
//				channel =
//	return:		status.
//===============================================================================================
void audio_record_set_info(INT32U audio_format, INT32U sample_rate, INT32U bit_rate)
{
	pAudio_Encode_Para->AudioFormat = audio_format;
	pAudio_Encode_Para->SampleRate = sample_rate;
	pAudio_Encode_Para->BitRate = bit_rate;
}

void audio_record_set_channel(INT8U device, INT8U channel)
{
	pAudio_Encode_Para->InputDevice = device;
	pAudio_Encode_Para->Channel = channel; //1, mono, 2, stereo
}

INT8U audio_record_set_CutLastFrame(INT8U flag)
{
    INT8U old_flag = pAudio_Encode_Para->CutLastFrame;
	pAudio_Encode_Para->CutLastFrame = flag;
	return old_flag;
}

//===============================================================================================
// 	audio_record_set_down_sample
//	parameter:	b_down_sample = enable/disable
//				ratio = 2~4
//	return:		status.
//===============================================================================================
INT32S audio_record_set_down_sample(BOOLEAN b_down_sample, INT8U ratio)
{
#if	APP_DOWN_SAMPLE_EN
	pAudio_Encode_Para->bEnableDownSample = b_down_sample;
	pAudio_Encode_Para->DownsampleFactor = ratio;
	return AUD_RECORD_STATUS_OK;
#else
	return AUD_RECORD_STATUS_ERR;
#endif
}

//===============================================================================================
// 	pcm_energy_detect
//	parameter:	none.
//	return:		status.
//===============================================================================================
INT32S pcm_energy_detect(INT16S* buffer_addr, INT32U pcm_size)
{
	INT16S temp;
	INT16U avg_value;
	INT64U temp_total;
	INT32S i, cnt, local_cnt;

	temp_total = 0;
	cnt = 0;
	for (i = 0; i < pcm_size; i++) {
		temp = abs(*(buffer_addr + i));
		temp_total += (INT64U)temp;
		if (temp > PCM_LOCAL_THR) {
			cnt += 1;
		}
	}

	// average
	avg_value = (INT64U)temp_total / pcm_size;
	DBG_PRINT("temp_total = 0x%x\r\n", temp_total);
	DBG_PRINT("avg_value = 0x%x, cnt = %d\r\n", avg_value, cnt);

	//samples above Local_Thr to judge as active
	local_cnt = pcm_size / 6;

	if((avg_value > PCM_GLOBAL_THR)||(cnt>local_cnt)) {
		return 0;	//active
	} else {
		return 1; 	//not active
	}
}

//===============================================================================================
// 	audio task write entry
//===============================================================================================
static void adc_record_write_entry(void *param)

{
	INT8U err;
	INT32U msg;
	INT32S ret;
	INT32U RI, WI;
	volatile INT8U *addr;
	Audio_Encode_Para *pAudRec = (Audio_Encode_Para *)param;
//	DBG_PRINT("record_write_entry\r\n");

	while(1)
	{
#if _OPERATING_SYSTEM != _OS_NONE
     #if _OPERATING_SYSTEM == _OS_UCOS2
		msg = (AUDIO_RECORD_ENUM)(int)OSQPend(aud_enc_writeQ, 0, &err);
		if(err != OS_NO_ERR) {
			continue;
		}
     #elif _OPERATING_SYSTEM == _OS_FREERTOS
        if(xQueueReceive(aud_enc_writeQ, &msg, portMAX_DELAY) != pdPASS)
        {
			continue;
		}
    #endif
#endif
		if(msg==MSG_AUDIO_ENCODE_WRITE)
		{
            WI = (pAudRec->write_index >> RECORD_CLUSTER_SHIFT) << RECORD_CLUSTER_SHIFT; // 64KB multiple
	        RI = pAudRec->read_index;
	        err = TRUE;
			while(RI!=WI)
        	{
        	    addr = pAudRec->Bit_Stream_Buffer + RI;
        		if(pAudio_Encode_Para->SourceType == C_GP_FS) {
				    ret = fs_write(pAudRec->FileHandle, (INT32U)addr, RECORD_CLUSTER_SIZE);
    			} else {
    				ret = audio_encode_data_write(0, (INT32U)addr, RECORD_CLUSTER_SIZE);
    			}
        		if(ret != RECORD_CLUSTER_SIZE)
        		{
        		   err = FALSE;
        		   break;
        		}
        		RI += RECORD_CLUSTER_SIZE;
        		if(RI>= C_BS_BUFFER_SIZE ){
					RI -= C_BS_BUFFER_SIZE;
					//DBG_PRINT("\r\n >>WI:%x, RI:%x ", WI, RI);
        		}

        		vTaskSuspendAll();
        		pAudRec->read_index = RI;
        		xTaskResumeAll();
        	}
        	if(FALSE == err)
        	{
        	    continue;
        	}
            pAudRec->MsgRx ++;
          #if 0
		    {
				DBG_PRINT("\r\nRING:W%x,R%x, MsgTx:%d,Rx:%d\r\n",pAudRec->write_index, pAudRec->read_index, pAudRec->MsgTx, pAudRec->MsgRx);
			};
	      #endif
		}
		else if(msg==MSG_AUDIO_ENCODE_WRITE_STOP)
		{
		    addr = pAudRec->Bit_Stream_Buffer + pAudRec->read_index;
            if(pAudRec->read_index > pAudRec->write_index)
            {
                WI = C_BS_BUFFER_SIZE - pAudRec->read_index + pAudRec->write_index;
            }
            else
            {
                WI = pAudRec->write_index - pAudRec->read_index;
            }
            RI = pAudRec->read_index;
            RI += WI;
    		if(RI>= C_BS_BUFFER_SIZE ) RI -= C_BS_BUFFER_SIZE;


    		vTaskSuspendAll();
    		pAudRec->read_index = RI;
    		xTaskResumeAll();
    		if(pAudio_Encode_Para->SourceType == C_GP_FS) {
			    ret = fs_write(pAudRec->FileHandle, (INT32U)addr, WI);
			} else {
				ret = audio_encode_data_write(0, (INT32U)addr, WI);
			}
            if(ret != WI)
            {
                msg = AUD_WRITE_ACK_FAIL;
            }
            else
            {
                msg = AUD_WRITE_ACK_OK;
            }
        #if _OPERATING_SYSTEM != _OS_NONE
            #if _OPERATING_SYSTEM == _OS_UCOS2
            OSMboxPost(aud_write_task_ack_m, (void*)&msg);
            #elif _OPERATING_SYSTEM == _OS_FREERTOS
             xQueueSend( (xQueueHandle) aud_write_task_ack_m, (void *) &msg, portMAX_DELAY);
            #endif
        #endif

		}
		else if(MSG_AUDIO_ENCODE_EXIT == msg)
        {
            osThreadId id;
            id = osThreadGetId();
    		osThreadTerminate(id);
        }

	}
}


//===============================================================================================
// 	audio task main entry
//===============================================================================================
void adc_record_entry(void *param)
{
	INT8U   adc_bufidx=0;
	INT8U   mic_bufidx=0;
	INT8U   buffer_index=0;
	INT8U   err, device=0, channel=0;
	INT16U  t, t2;
	INT16U  SampleRate;
	INT32U  msg_id;
	INT32S  i, nRet;
	INT32U  ready_addr=0, free_addr=0, pcm_addr;
	INT16U  *ptr, *ptr1, *ptr2, *temp;
	INT32U  adc_ready_addr=0, adc_free_addr=0, adc_pcm_addr=0;
	INT32U  mic_ready_addr=0, mic_free_addr=0, mic_pcm_addr=0;
	INT32U  encode_addr=0, aud_dst_buffer=0;
	void    *hProc;
	INT32S  (*pfn_audio_encode_stop)(void);
	INT32S  (*pfn_audio_encode_once)(void *workmem, const short* buffer_addr, int cwlen);
	Audio_Encode_Para *pAudRec = (Audio_Encode_Para *)param;
    INT32U message;
    DBG_PRINT("%s\r\n",__func__);
#if 0
	INT32U  L_pcm_ptr, R_pcm_ptr;
#endif

	while(1)
	{
#if _OPERATING_SYSTEM != _OS_NONE
            #if _OPERATING_SYSTEM == _OS_UCOS2
		msg_id = (INT32U) OSQPend(aud_enc_reqQ, 0, &err);
		if(err != OS_NO_ERR) {
			continue;
		}
            #elif _OPERATING_SYSTEM == _OS_FREERTOS
                err = (INT32S) xQueueReceive(aud_enc_reqQ, &msg_id, portMAX_DELAY);
                if(err != pdPASS) {
			continue;
		}
            #endif
#endif

		switch(msg_id)
		{
		case MSG_ADC_DMA_DONE:
			switch(device)
			{
			case ADC_LINE_IN:
			case BUILD_IN_MIC:
			case BUILD_LINEIN:
//                DBG_PRINT("PGA = %x \r\n",drv_l1_adc_dagc_current_pga_gain());
				pcm_addr = ready_addr;
				ready_addr = free_addr;
				buffer_index++;
				if(buffer_index >= C_AUD_PCM_BUFFER_NO) {
					buffer_index = 0;
				}

				if(device == ADC_LINE_IN) {
					free_addr = pAudRec->aud_pcm_buffer[buffer_index];
				} else {
					free_addr =  pAudRec->mic_pcm_buffer[buffer_index];
				}

				if(pAudRec->Status == C_STOP_RECORDING) {
					// check dma is done and stop
					if(aud_adc_dma_status_get(device) == 0) {
                                #if _OPERATING_SYSTEM != _OS_NONE
                                    #if _OPERATING_SYSTEM == _OS_UCOS2
						OSQPost(aud_enc_reqQ, (void *) MSG_AUDIO_ENCODE_STOP);
                                    #elif _OPERATING_SYSTEM == _OS_FREERTOS
					    message = MSG_AUDIO_ENCODE_STOP;
                                                xQueueSend( (xQueueHandle) aud_enc_reqQ, (void *) &message, portMAX_DELAY);
                                    #endif
                                #endif
					}
					if(pAudio_Encode_Para->CutLastFrame)
					{
                        continue;
					}
				} else {
					// set dma buffer
					aud_adc_double_buffer_set((INT16U *)free_addr, pAudRec->PCMInFrameSize, device);
				}

				// invalid cache
				cache_invalid_range(pcm_addr, pAudRec->PCMInFrameSize << 1);

				// unsigned to signed
				ptr = (INT16U*)pcm_addr;
				for(i=0; i<pAudRec->PCMInFrameSize; i++)
				{
					t = *ptr;
				#if SOFTWARE_UNSIGNED_TO_SIGNED == 1
                    t ^= mask;
                #endif
				#if APP_LPF_ENABLE == 1
					t = (INT16U)LPF_process(t);
				#endif
					*ptr++ = t;
				}

				encode_addr = pcm_addr;

				break;

			case DOUBLE_LINEIN:
                        #if _OPERATING_SYSTEM != _OS_NONE
                            #if _OPERATING_SYSTEM == _OS_UCOS2
				msg_id = (INT32U) OSQPend(aud_enc_reqQ2, 0, &err);
                            #elif _OPERATING_SYSTEM == _OS_FREERTOS
                                err = (INT32S) xQueueReceive(aud_enc_reqQ2, &msg_id, portMAX_DELAY);
                            #endif
                        #endif

				if (msg_id != MSG_ADC_DMA_DONE) {
					DBG_PRINT("MIC DMA error !\r\n");
					continue;
				}

				adc_pcm_addr = adc_ready_addr;
				adc_ready_addr = adc_free_addr;
				mic_pcm_addr = mic_ready_addr;
				mic_ready_addr = mic_free_addr;
				adc_bufidx++;
				mic_bufidx++;
				if(adc_bufidx >= C_AUD_PCM_BUFFER_NO) {
					adc_bufidx = 0;
				}

				if(mic_bufidx >= C_AUD_PCM_BUFFER_NO) {
					mic_bufidx = 0;
				}

				adc_free_addr = pAudRec->aud_pcm_buffer[adc_bufidx];
				mic_free_addr = pAudRec->mic_pcm_buffer[mic_bufidx];
				if(pAudRec->Status == C_STOP_RECORDING) {
					// check dma is done and stop
					if ((aud_adc_dma_status_get(ADC_LINE_IN) == 0) && (aud_adc_dma_status_get(BUILD_IN_MIC) == 0)) {
                                        #if _OPERATING_SYSTEM != _OS_NONE
                                            #if _OPERATING_SYSTEM == _OS_UCOS2
						OSQPost(aud_enc_reqQ, (void *) MSG_AUDIO_ENCODE_STOP);
                                            #elif _OPERATING_SYSTEM == _OS_FREERTOS
					    message = MSG_AUDIO_ENCODE_STOP;
                                                xQueueSend( (xQueueHandle) aud_enc_reqQ, (void *) &message, portMAX_DELAY);
                                            #endif
                                        #endif
					}
				} else {
					// set dma buffer
					aud_adc_double_buffer_set((INT16U *)adc_free_addr, pAudRec->PCMInFrameSize>>1, ADC_LINE_IN);
					aud_adc_double_buffer_set((INT16U *)mic_free_addr, pAudRec->PCMInFrameSize>>1, BUILD_IN_MIC);
				}

				// invalid cache
				cache_invalid_range(adc_pcm_addr, pAudRec->PCMInFrameSize);
				cache_invalid_range(mic_pcm_addr, pAudRec->PCMInFrameSize);

				// unsigned to signed
				ptr1 = (INT16U*)adc_pcm_addr;
				ptr2 = (INT16U*)mic_pcm_addr;
				temp = (INT16U*)aud_dst_buffer;
				for (i=0; i<(pAudRec->PCMInFrameSize>>1); i++)
				{
					t = *ptr1++;
					t2 = *ptr2++;
				#if ((defined MCU_VERSION) && (MCU_VERSION < GPL327XX))
					t ^= mask;
					t2 ^= mask;
				#endif
				#if APP_LPF_ENABLE == 1
					t = (INT16U)LPF_process(t);
					t2 = (INT16U)LPF_process(t2);
				#endif
					*temp++ = t;
					*temp++ = t2;
				}

				encode_addr = aud_dst_buffer;
				break;
			}
			// encode pcm wave

			nRet = pfn_audio_encode_once(hProc, (const short*)encode_addr, pAudRec->PCMInFrameSize);
			if(nRet < 0) {
                        #if _OPERATING_SYSTEM != _OS_NONE
                            #if _OPERATING_SYSTEM == _OS_UCOS2
				OSQPost(aud_enc_reqQ, (void *) MSG_AUDIO_ENCODE_ERR);
                            #elif _OPERATING_SYSTEM == _OS_FREERTOS
			     message = MSG_AUDIO_ENCODE_ERR;
                                xQueueSend( (xQueueHandle) aud_enc_reqQ, (void *) &message, portMAX_DELAY);
                            #endif
                        #endif
				continue;
			}
            DBG_PRINT("*"); //wav done
			// check storage is full
			if(pAudRec->SourceType == C_GP_FS) {
//                DBG_PRINT("FileLenth =0x%x disk_free_size = 0x%x\r\n",pAudRec->FileLenth/1024,pAudRec->disk_free_size/1024);
				if(pAudRec->FileLenth >= pAudRec->disk_free_size) {
                                #if _OPERATING_SYSTEM != _OS_NONE
                                    #if _OPERATING_SYSTEM == _OS_UCOS2
					OSQPost(aud_enc_reqQ, (void *) MSG_AUDIO_ENCODE_STOPING);
                                    #elif _OPERATING_SYSTEM == _OS_FREERTOS
				    message = MSG_AUDIO_ENCODE_STOPING;
                                        xQueueSend( (xQueueHandle) aud_enc_reqQ, (void *) &message, portMAX_DELAY);
                                    #endif
                                #endif
				}
				if(pAudRec->FileLenth >= max_audio_record_file_size) {
                                #if _OPERATING_SYSTEM != _OS_NONE
                                    #if _OPERATING_SYSTEM == _OS_UCOS2
					OSQPost(aud_enc_reqQ, (void *) MSG_AUDIO_ENCODE_STOPING);
                                    #elif _OPERATING_SYSTEM == _OS_FREERTOS
				    message = MSG_AUDIO_ENCODE_STOPING;
                                        xQueueSend( (xQueueHandle) aud_enc_reqQ, (void *) &message, portMAX_DELAY);
                                    #endif
                                #endif
				}
			}
			break;

		case MSG_AUDIO_ENCODE_START:

			DBG_PRINT("[MSG_AUDIO_ENCODE_START]\r\n");
			hProc = 0;
			pAudRec->ring_buffer = 0;
			pAudio_Encode_Para->CutLastFrame = 0;
			pAudRec->read_index = 0;
			pAudRec->write_index = 0;
			pAudRec->MsgTx = 0;
			pAudRec->MsgRx = 0;
		#if 0
			L_pcm_ptr = open("L_pcm.pcm", O_WRONLY|O_CREAT|O_TRUNC);
			R_pcm_ptr = open("R_pcm.pcm", O_WRONLY|O_CREAT|O_TRUNC);
		#endif
			switch(pAudRec->AudioFormat)
			{
			case WAVE_FORMAT_PCM:
				nRet = wave_encode_start();
				pfn_audio_encode_once = wave_encode_once;
				pfn_audio_encode_stop = wave_encode_stop;
				break;

			default:
				DBG_PRINT("encode fmt err.\r\n");
				goto __START_FAIL;
			}

			if(nRet < 0) {
				DBG_PRINT("encode start fail.\r\n");
				goto __START_FAIL;
			}

			device = pAudRec->InputDevice;
			channel = pAudRec->Channel;
			SampleRate = pAudRec->SampleRate;
			DBG_PRINT("Dev=0x%x, Ch=%d, SR=%d\r\n", device, channel, SampleRate);
			DBG_PRINT("PCMInFrameSize=%d\r\n", pAudRec->PCMInFrameSize);

			// start dma and adc
			switch(device)
			{
			case ADC_LINE_IN:
				if(channel != 1) {
					DBG_PRINT("ChErr.\r\n");
					goto __START_FAIL;
				}

				nRet = adc_memory_allocate(pAudRec->PCMInFrameSize<<1, ADC_LINE_IN);
				if(nRet < 0) {
					DBG_PRINT("alocate memory fail.\r\n");
					goto __START_FAIL;
				}

				ready_addr = pAudRec->aud_pcm_buffer[0];
				free_addr = pAudRec->aud_pcm_buffer[1];
				buffer_index = 1;
				aud_adc_double_buffer_put((INT16U*)ready_addr, pAudRec->PCMInFrameSize, ADC_LINE_IN, aud_enc_reqQ);
				aud_adc_double_buffer_set((INT16U*)free_addr, pAudRec->PCMInFrameSize, ADC_LINE_IN);
				adc_hardware_start(ADC_LINE_IN, 1, SampleRate);
				break;

			case BUILD_IN_MIC:
				if(channel != 1) {
					DBG_PRINT("ChErr.\r\n");
					goto __START_FAIL;
				}

				nRet = adc_memory_allocate(pAudRec->PCMInFrameSize<<1, BUILD_IN_MIC);
				if(nRet < 0) {
					DBG_PRINT("alocate memory fail.\r\n");
					goto __START_FAIL;
				}

				ready_addr = pAudRec->mic_pcm_buffer[0];
				free_addr = pAudRec->mic_pcm_buffer[1];
				buffer_index = 1;
				aud_adc_double_buffer_put((INT16U*)ready_addr, pAudRec->PCMInFrameSize, BUILD_IN_MIC, aud_enc_reqQ);
				aud_adc_double_buffer_set((INT16U*)free_addr, pAudRec->PCMInFrameSize, BUILD_IN_MIC);
				adc_hardware_start(BUILD_IN_MIC, 1, SampleRate);
				break;

			case DOUBLE_LINEIN:
				if(channel != 2) {
					DBG_PRINT("ChErr.\r\n");
					goto __START_FAIL;
				}

				nRet = adc_memory_allocate(pAudRec->PCMInFrameSize, ADC_LINE_IN|BUILD_IN_MIC);
				if(nRet < 0) {
					DBG_PRINT("alocate memory fail.\r\n");
					goto __START_FAIL;
				}

				aud_dst_buffer = (INT32U)gp_malloc_align(pAudRec->PCMInFrameSize<<1, 32);
				if(aud_dst_buffer == 0) {
					DBG_PRINT("aud_dst_buffer fail.\r\n");
					goto __START_FAIL;
				}

				adc_ready_addr = pAudRec->aud_pcm_buffer[0];
				adc_free_addr = pAudRec->aud_pcm_buffer[1];
				mic_ready_addr = pAudRec->mic_pcm_buffer[0];
				mic_free_addr = pAudRec->mic_pcm_buffer[1];
				adc_bufidx = 1;
				mic_bufidx = 1;
				aud_adc_double_buffer_put((INT16U*)adc_ready_addr, pAudRec->PCMInFrameSize>>1, ADC_LINE_IN, aud_enc_reqQ);
				aud_adc_double_buffer_set((INT16U*)adc_free_addr, pAudRec->PCMInFrameSize>>1, ADC_LINE_IN);
				aud_adc_double_buffer_put((INT16U*)mic_ready_addr, pAudRec->PCMInFrameSize>>1, BUILD_IN_MIC, aud_enc_reqQ2);
				aud_adc_double_buffer_set((INT16U*)mic_free_addr, pAudRec->PCMInFrameSize>>1, BUILD_IN_MIC);
				adc_hardware_start(DOUBLE_LINEIN, 2, SampleRate);
				break;
			case BUILD_LINEIN:
				if(channel != 2) {
					DBG_PRINT("ChErr.\r\n");
					goto __START_FAIL;
				}

				nRet = adc_memory_allocate(pAudRec->PCMInFrameSize<<1, BUILD_LINEIN);
				if(nRet < 0) {
					DBG_PRINT("alocate memory fail.\r\n");
					goto __START_FAIL;
				}

				ready_addr = pAudRec->mic_pcm_buffer[0];
				free_addr = pAudRec->mic_pcm_buffer[1];
				buffer_index = 1;
				DBG_PRINT("LINEIN length %d \r\n", pAudRec->PCMInFrameSize);
				aud_adc_double_buffer_put((INT16U*)ready_addr, pAudRec->PCMInFrameSize, BUILD_LINEIN, aud_enc_reqQ);
				aud_adc_double_buffer_set((INT16U*)free_addr, pAudRec->PCMInFrameSize, BUILD_LINEIN);
				adc_hardware_start(BUILD_LINEIN, 2, SampleRate);
				break;


			default:
				DBG_PRINT("InputDeviceErr.\r\n");
				goto __START_FAIL;
			}
			pAudRec->Status = C_START_RECORD;
			break;

__START_FAIL:
			DBG_PRINT("AudioEncodeStartFail!!!\r\n");
			adc_work_mem_free();
			adc_memory_free();
			pAudRec->Status = C_START_FAIL;
			break;

		case MSG_AUDIO_ENCODE_STOPING:
			DBG_PRINT("[MSG_AUDIO_ENCODE_STOPING]\r\n");
			pAudRec->Status = C_STOP_RECORDING;
			break;

		case MSG_AUDIO_ENCODE_STOP:
			DBG_PRINT("[MSG_AUDIO_ENCODE_STOP]\r\n");
			adc_hardware_stop(device, channel);
			pfn_audio_encode_stop();
			adc_memory_free();

			switch(device)
			{
			case ADC_LINE_IN:
				aud_adc_double_buffer_free(ADC_LINE_IN);
				break;

			case BUILD_IN_MIC:
				aud_adc_double_buffer_free(BUILD_IN_MIC);
				break;

			case DOUBLE_LINEIN:
				aud_adc_double_buffer_free(ADC_LINE_IN);
				aud_adc_double_buffer_free(BUILD_IN_MIC);
				if (aud_dst_buffer) {
					gp_free((void *)aud_dst_buffer);
					aud_dst_buffer = 0;
				}
					break;
				case BUILD_LINEIN:
					aud_adc_double_buffer_free(BUILD_LINEIN);
				break;
			}
                #if _OPERATING_SYSTEM != _OS_NONE
                    #if _OPERATING_SYSTEM == _OS_UCOS2
                        OSQFlush(aud_enc_reqQ);
			OSQFlush(aud_enc_reqQ2);
                    #elif _OPERATING_SYSTEM == _OS_FREERTOS
                        xQueueReset(aud_enc_reqQ);
                        xQueueReset(aud_enc_reqQ2);
                    #endif
                #endif


			pAudRec->Status = C_STOP_RECORD;
            UsrFlushBuffers(STORAGE_DEVICE);
            ap_peripheral_auto_off_force_disable_set(0);
			break;

		case MSG_AUDIO_ENCODE_ERR:
			DBG_PRINT("[MSG_AUDIO_ENCODE_ERR]\r\n");
                #if _OPERATING_SYSTEM != _OS_NONE
                    #if _OPERATING_SYSTEM == _OS_UCOS2
			OSQPost(aud_enc_reqQ, (void *) MSG_AUDIO_ENCODE_STOPING);
                    #elif _OPERATING_SYSTEM == _OS_FREERTOS
		      message = MSG_AUDIO_ENCODE_STOPING;
                        xQueueSend( (xQueueHandle) aud_enc_reqQ, (void *) &message, portMAX_DELAY);
                    #endif
                #endif
			break;
        case MSG_AUDIO_ENCODE_EXIT:
        {
            osThreadId id;
            id = osThreadGetId();
    		osThreadTerminate(id);
        }
        break;
		}
	}
}

//===============================================================================================
// 	aud_adc_double_buffer_put
//	parameter:	data = buffer_addr.
//				len  = size in word
//				os_q = OS _EVENT
//	return:		status.
//===============================================================================================
#if _OPERATING_SYSTEM == _OS_UCOS2
static INT32S aud_adc_double_buffer_put(INT16U *data,INT32U cwlen, INT8U aud_in, OS_EVENT *os_q)
#elif _OPERATING_SYSTEM == _OS_FREERTOS
static INT32S aud_adc_double_buffer_put(INT16U *data,INT32U cwlen, INT8U aud_in, xQueueHandle os_q)
#endif
{
	INT32S status;
	DMA_STRUCT *pDma;

	if (aud_in == ADC_LINE_IN) {
		pDma = &pAudio_Encode_Para->adc_dma_dbf;
		pDma->s_addr = (INT32U) P_ADC_ASADC_DATA;
		pDma->t_addr = (INT32U) data;
		pDma->width = DMA_DATA_WIDTH_2BYTE;
		pDma->count = (INT32U) cwlen;
		pDma->notify = NULL;
		pDma->timeout = 0;
	} else if ((aud_in == BUILD_IN_MIC)||(aud_in == BUILD_LINEIN)) {
		pDma = &pAudio_Encode_Para->mic_dma_dbf;
		pDma->s_addr = (INT32U) P_I2SRX_DATA;
		pDma->t_addr = (INT32U) data;
		pDma->width = DMA_DATA_WIDTH_4BYTE;
		pDma->count = (INT32U) cwlen/2;
		pDma->notify = NULL;
		pDma->timeout = 0;
	} else {
		return STATUS_FAIL;
	}

	status = drv_l1_dma_transfer_with_double_buf(pDma, os_q);
	if (status != 0) {
		return status;
	}

	return STATUS_OK;
}

//===============================================================================================
// 	aud_adc_double_buffer_set
//	parameter:	data = buffer_addr.
//				len  = size in word
//	return:		status.
//===============================================================================================
static INT32U aud_adc_double_buffer_set(INT16U *data, INT32U cwlen, INT8U aud_in)
{
	INT32S status;
	DMA_STRUCT *pDma;

	if (aud_in == ADC_LINE_IN) {
		pDma = &pAudio_Encode_Para->adc_dma_dbf;
		pDma->s_addr = (INT32U) P_ADC_ASADC_DATA;
		pDma->t_addr = (INT32U) data;
		pDma->width = DMA_DATA_WIDTH_2BYTE;
		pDma->count = (INT32U) cwlen;
		pDma->notify = NULL;
		pDma->timeout = 0;

	} else if ((aud_in == BUILD_IN_MIC)||(aud_in == BUILD_LINEIN)) {
		pDma = &pAudio_Encode_Para->mic_dma_dbf;
		pDma->s_addr = (INT32U) P_I2SRX_DATA;
		pDma->t_addr = (INT32U) data;
		pDma->width = DMA_DATA_WIDTH_4BYTE;
		pDma->count = (INT32U) cwlen/2;
		pDma->notify = NULL;
		pDma->timeout = 0;
	} else {
		return STATUS_FAIL;
	}

	status = drv_l1_dma_transfer_double_buf_set(pDma);
	if(status != 0) {
		return status;
	}

	return STATUS_OK;
}

//===============================================================================================
// 	aud_adc_dma_status_get
//	parameter:	none
//	return:		status.
//===============================================================================================
static INT32S aud_adc_dma_status_get(INT8U aud_in)
{
	INT32S dbf = 0;
	INT32S status = 0;
	DMA_STRUCT *pDma;

	if (aud_in == ADC_LINE_IN) {
		pDma = &pAudio_Encode_Para->adc_dma_dbf;
	} else if ((aud_in == BUILD_IN_MIC)||(aud_in == BUILD_LINEIN)) {
		pDma = &pAudio_Encode_Para->mic_dma_dbf;
	} else {
		return STATUS_FAIL;
	}

	if (pDma->channel == 0xff) {
		return -1;
	}

	dbf = drv_l1_dma_dbf_status_get(pDma->channel);
	status = drv_l1_dma_status_get(pDma->channel);
	if(dbf || status) {
		return 1;
	}

	return 0;
}

//===============================================================================================
// 	aud_adc_double_buffer_free
//	parameter:	none
//	return:		free double buffer dma channel.
//===============================================================================================
static void aud_adc_double_buffer_free(INT8U aud_in)
{
	DMA_STRUCT *pDma;

	if (aud_in == ADC_LINE_IN) {
		pDma = &pAudio_Encode_Para->adc_dma_dbf;
	} else if ((aud_in == BUILD_IN_MIC)||(aud_in == BUILD_LINEIN)) {
		pDma = &pAudio_Encode_Para->mic_dma_dbf;
	} else {
		return;
	}

	drv_l1_dma_transfer_double_buf_free(pDma);
	pDma->channel = 0xff;
}
//===============================================================================================
// 	adc_hardware_start
//	parameter:	SampleRate = adc sample rate set.
//	return:		none.
//===============================================================================================
static INT32S adc_hardware_start(INT8U InputDevice, INT8U channel, INT32U SampleRate)
{
	switch(InputDevice)
	{
	#if ADC_LINE_IN_EN == 1
	case ADC_LINE_IN:
		if(channel != 1) {
			DBG_PRINT("ChERR.\r\n");
			return STATUS_FAIL;
		}

		drv_l1_adc_fifo_clear();
		drv_l1_adc_auto_ch_set(C_ADC_USE_CHANNEL);
		drv_l1_adc_fifo_level_set(4);
		drv_l1_adc_auto_sample_start();
		vTaskDelay(50); //wait bais stable
		drv_l1_adc_sample_rate_set(C_ADC_USE_TIMER, SampleRate);
		break;
	#endif

	#if BUILD_IN_MIC_EN == 1
	case BUILD_IN_MIC:
		if(channel != 1) {
			DBG_PRINT("ChERR.\r\n");
			return STATUS_FAIL;
		}
        ap_audio_adc_hw_start(SampleRate);

        vTaskDelay(50);
		break;
	#endif

	case DOUBLE_LINEIN:
		if(channel != 2) {
			DBG_PRINT("ChERR.\r\n");
			return STATUS_FAIL;
		}

	#if ADC_LINE_IN_EN == 1

		drv_l1_adc_fifo_clear();
		drv_l1_adc_fifo_level_set(4);
		drv_l1_adc_auto_ch_set(C_ADC_USE_CHANNEL);
		drv_l1_adc_auto_sample_start();
	#endif

	#if BUILD_IN_MIC_EN == 1
        //dagc init
		drv_l1_adc_dagc_mode(0,0);
        drv_l1_adc_dagc_scale(0,0);
        drv_l1_adc_dagc_att_rel_time(1,0xf8);
        drv_l1_adc_dagc_threshold(0x20,0);
        drv_l1_adc_dagc_enable(1);
		aud_adc_input_init(BUILD_IN_MIC, SampleRate);
	#endif

		// wait bais stable
        #if _OPERATING_SYSTEM != _OS_NONE
            #if _OPERATING_SYSTEM == _OS_UCOS2
		OSTimeDly(70);
            #elif _OPERATING_SYSTEM == _OS_FREERTOS
                vTaskDelay(70);
            #endif
        #endif

        #if _OPERATING_SYSTEM != _OS_NONE
            #if _OPERATING_SYSTEM == _OS_UCOS2
		OS_ENTER_CRITICAL();
            #elif _OPERATING_SYSTEM == _OS_FREERTOS
                vPortEnterCritical();
            #endif
        #endif

	#if ADC_LINE_IN_EN == 1 && BUILD_IN_MIC_EN == 1
		drv_l1_adc_sample_rate_set(C_ADC_USE_TIMER, SampleRate);
	#endif

        #if _OPERATING_SYSTEM != _OS_NONE
            #if _OPERATING_SYSTEM == _OS_UCOS2
		OS_EXIT_CRITICAL();
            #elif _OPERATING_SYSTEM == _OS_FREERTOS
                vPortExitCritical();
            #endif
        #endif
		break;

    #if BUILD_LINEIN_EN == 1
	case BUILD_LINEIN:
		if(channel != 2) {
			DBG_PRINT("ChERR.\r\n");
			return STATUS_FAIL;
		}
        aud_adc_input_init(BUILD_LINEIN, SampleRate);

        vTaskDelay(50);
		break;
	#endif

	default:
		DBG_PRINT("InputDeviceERR.\r\n");
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

//===============================================================================================
// 	adc_hardware_start
//	parameter:	SampleRate = adc sample rate set.
//	return:		none.
//===============================================================================================
static INT32S adc_hardware_stop(INT8U InputDevice, INT8U channel)
{
	switch(InputDevice)
	{
	#if ADC_LINE_IN_EN == 1
	case ADC_LINE_IN:
		if(channel != 1) {
			DBG_PRINT("ChERR.\r\n");
			return STATUS_FAIL;
		}
		drv_l1_adc_timer_stop(C_ADC_USE_TIMER);
		break;
	#endif

	#if BUILD_IN_MIC_EN == 1
	case BUILD_IN_MIC:
		if(channel != 1) {
			return STATUS_FAIL;
		}

		ap_audio_adc_hw_stop();
		break;
	#endif

	case DOUBLE_LINEIN:
		if(channel != 2) {
			DBG_PRINT("ChERR.\r\n");
			return STATUS_FAIL;
		}
	#if ADC_LINE_IN_EN == 1
		drv_l1_adc_timer_stop(C_ADC_USE_TIMER);
	#endif
	#if BUILD_IN_MIC_EN == 1
		aud_adc_input_stop();
	#endif
		break;

	case BUILD_LINEIN:
		if(channel != 2) {
			DBG_PRINT("ChERR.\r\n");
			return STATUS_FAIL;
		}
    #if BUILD_LINEIN_EN == 1
        aud_adc_input_stop();
    #endif
		break;

	default:
		return STATUS_FAIL;
	}

	return STATUS_OK;
}
//===============================================================================================
// 	save_buffer_to_storage
//	parameter:
//	return:		status
//===============================================================================================
static INT32S save_buffer_to_storage(void)
{
    INT32U message;

    if(pAudio_Encode_Para->MsgTx == pAudio_Encode_Para->MsgRx)
	{
#if _OPERATING_SYSTEM != _OS_NONE
    #if _OPERATING_SYSTEM == _OS_UCOS2
        if(OSQPost(aud_enc_writeQ, (void*)MSG_AUDIO_ENCODE_WRITE)!=OS_NO_ERR) // notify file write
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
        message = MSG_AUDIO_ENCODE_WRITE;
        if(xQueueSend( (xQueueHandle) aud_enc_writeQ, (void *) &message, portMAX_DELAY) != pdPASS)
    #endif
#endif
		{
			return (AUD_RECORD_FILE_WRITE_ERR);
		}
		pAudio_Encode_Para->MsgTx++;
	}
	return AUD_RECORD_STATUS_OK;
}
//===============================================================================================
// 	save_final_data_to_storage
//	parameter:
//	return:		status
//===============================================================================================
static INT32S save_final_data_to_storage(void)
{
	INT32U message;

    xQueueReceive(aud_write_task_ack_m, (void *) &message, 0);
	message = MSG_AUDIO_ENCODE_WRITE_STOP;
    if(xQueueSend(aud_enc_writeQ, (void *) &message, portMAX_DELAY) != pdPASS)
	{
		DBG_PRINT("xQueueSend Fail!!!\r\n");
		return (AUD_RECORD_FILE_WRITE_ERR);
	}
	if(xQueueReceive(aud_write_task_ack_m, &message, 5000/10) != pdPASS)
	{
		DBG_PRINT("xQueueReceive ack Fail!!!\r\n");
		return (AUD_RECORD_FILE_WRITE_ERR);
	}
	if(message != AUD_WRITE_ACK_OK)
	{
		DBG_PRINT("xQueueReceive ack Fail!!!\r\n");
		return (AUD_RECORD_FILE_WRITE_ERR);
	}

    DBG_PRINT("read = %d,write=%d\r\n", pAudio_Encode_Para->read_index, pAudio_Encode_Para->write_index);
	return AUD_RECORD_STATUS_OK;
}

//===============================================================================================
// 	adc_memory_allocate
//	parameter:	cbLen = adc buffer size in byte.
//	return:		status.
//===============================================================================================
static INT32S adc_memory_allocate(INT32U CbLen, INT8U adc_in)
{
	INT16U *ptr;
	INT32S i, j;

	if(adc_in & ADC_LINE_IN) {
		pAudio_Encode_Para->aud_pcm_buffer[0] =(INT32U) gp_malloc_align(CbLen*C_AUD_PCM_BUFFER_NO, 4);
		if(pAudio_Encode_Para->aud_pcm_buffer[0] == 0) {
			return AUD_RECORD_MEMORY_ALLOC_ERR;
		}

		for(i=0; i<C_AUD_PCM_BUFFER_NO; i++) {
			pAudio_Encode_Para->aud_pcm_buffer[i] = pAudio_Encode_Para->aud_pcm_buffer[0] + (CbLen * i);
			DBG_PRINT("aud_pcm_buffer[%d] = 0x%x\r\n", i, pAudio_Encode_Para->aud_pcm_buffer[i]);
			ptr = (INT16U*)pAudio_Encode_Para->aud_pcm_buffer[i];
			for(j=0; j<CbLen/2; j++) {
				*ptr++ = 0x8000;
			}
		}
	}

	if((adc_in & BUILD_IN_MIC)||(adc_in & BUILD_LINEIN)) {
		pAudio_Encode_Para->mic_pcm_buffer[0] =(INT32U) gp_malloc_align(CbLen*C_AUD_PCM_BUFFER_NO, 4);
		if(pAudio_Encode_Para->mic_pcm_buffer[0] == 0) {
			return AUD_RECORD_MEMORY_ALLOC_ERR;
		}

		for(i=0; i<C_AUD_PCM_BUFFER_NO; i++) {
			pAudio_Encode_Para->mic_pcm_buffer[i] = pAudio_Encode_Para->mic_pcm_buffer[0] + (CbLen * i);
			DBG_PRINT("mic_pcm_buffer[%d] = 0x%x\r\n", i, pAudio_Encode_Para->mic_pcm_buffer[i]);
			ptr = (INT16U*)pAudio_Encode_Para->mic_pcm_buffer[i];
			for(j=0; j<CbLen/2; j++) {
				*ptr++ = 0x8000;
			}
		}
	}

	return AUD_RECORD_STATUS_OK;
}

//===============================================================================================
// 	adc_memory_free
//	parameter:	none.
//	return:		status.
//===============================================================================================
static INT32S adc_memory_free(void)
{
	INT32S i;

	if(pAudio_Encode_Para->aud_pcm_buffer[0]) {
		gp_free((void *)pAudio_Encode_Para->aud_pcm_buffer[0]);
	}

	if(pAudio_Encode_Para->mic_pcm_buffer[0]) {
		gp_free((void *)pAudio_Encode_Para->mic_pcm_buffer[0]);
	}

	if(pAudio_Encode_Para->Bit_Stream_Buffer) {
		gp_free((void *)pAudio_Encode_Para->Bit_Stream_Buffer);
		pAudio_Encode_Para->Bit_Stream_Buffer = 0;
	}

	for(i=0; i<C_AUD_PCM_BUFFER_NO; i++) {
		pAudio_Encode_Para->aud_pcm_buffer[i] = 0;
		pAudio_Encode_Para->mic_pcm_buffer[i] = 0;
	}

	return AUD_RECORD_STATUS_OK;
}

//===============================================================================================
// 	adc_work_mem_free
//	parameter:
//	return:
//===============================================================================================
static void adc_work_mem_free(void)
{
	if(pAudio_Encode_Para->EncodeWorkMem) {
		gp_free((void *)pAudio_Encode_Para->EncodeWorkMem);
		pAudio_Encode_Para->EncodeWorkMem = 0;
	}

	if(pAudio_Encode_Para->Bit_Stream_Buffer) {
		gp_free((void *)pAudio_Encode_Para->Bit_Stream_Buffer);
		pAudio_Encode_Para->Bit_Stream_Buffer = 0;
	}

	if(pAudio_Encode_Para->pack_buffer) {
		gp_free((void *)pAudio_Encode_Para->pack_buffer);
		pAudio_Encode_Para->pack_buffer = 0;
	}
}

#if 1
//===============================================================================================
// 	aud_enc_RIFF_init
//	parameter:	samplerate:  wave file sample rate.
//	return:		none.
//===============================================================================================
static void aud_enc_RIFF_init(INT32U samplerate)
{
	WaveHeadPara.RIFF_ID[0] ='R';
	WaveHeadPara.RIFF_ID[1] ='I';
	WaveHeadPara.RIFF_ID[2] ='F';
	WaveHeadPara.RIFF_ID[3] ='F';
	WaveHeadPara.RIFF_len = 0;

	WaveHeadPara.type_ID[0] = 'W';
	WaveHeadPara.type_ID[1] = 'A';
	WaveHeadPara.type_ID[2] = 'V';
	WaveHeadPara.type_ID[3] = 'E';

	WaveHeadPara.fmt_ID[0] = 'f';
	WaveHeadPara.fmt_ID[1] = 'm';
	WaveHeadPara.fmt_ID[2] = 't';
	WaveHeadPara.fmt_ID[3] = ' ';

	WaveHeadPara.fmt_len = 16;
	WaveHeadPara.format = 1;
	WaveHeadPara.channel = 1;
	WaveHeadPara.sample_rate = samplerate;

	//8, 16, 24 or 32
	WaveHeadPara.Sign_bit_per_sample = 16;
	//BlockAlign = SignificantBitsPerSample / 8 * NumChannels
	WaveHeadPara.Block_align = 16/8*1;
	//AvgBytesPerSec = SampleRate * BlockAlign
	WaveHeadPara.avg_byte_per_sec = samplerate*2;

	WaveHeadPara.data_ID[0] = 'd';
	WaveHeadPara.data_ID[1] = 'a';
	WaveHeadPara.data_ID[2] = 't';
	WaveHeadPara.data_ID[3] = 'a';

	WaveHeadPara.data_len = 0;
}

//===============================================================================================
// 	wave_encode_start
//	parameter:	none.
//	return:		status.
//===============================================================================================
static INT32S wave_encode_start(void)
{
	INT32S cbLen;
	AUD_ENC_WAVE_HEADER *pWaveHeadPara;

	pAudio_Encode_Para->EncodeWorkMem = NULL;
	pAudio_Encode_Para->Bit_Stream_Buffer = (INT8U *)gp_malloc_align(C_BS_BUFFER_SIZE, 4);
	if(!pAudio_Encode_Para->Bit_Stream_Buffer)
		return AUD_RECORD_MEMORY_ALLOC_ERR;

	//copy header
	pWaveHeadPara = &WaveHeadPara;
	cbLen = sizeof(AUD_ENC_WAVE_HEADER);
	aud_enc_RIFF_init(pAudio_Encode_Para->SampleRate);
	gp_memcpy((INT8S*)pAudio_Encode_Para->Bit_Stream_Buffer, (INT8S*)pWaveHeadPara, cbLen);

	pAudio_Encode_Para->write_index = cbLen;
	pAudio_Encode_Para->FileLenth = cbLen;
	pAudio_Encode_Para->PackSize = C_WAVE_RECORD_BUFFER_SIZE<<1;
	pAudio_Encode_Para->pack_buffer = NULL;
	pAudio_Encode_Para->PCMInFrameSize = C_WAVE_RECORD_BUFFER_SIZE;
	pAudio_Encode_Para->OnePCMFrameSize = C_WAVE_RECORD_BUFFER_SIZE;

	return AUD_RECORD_STATUS_OK;
}

//===============================================================================================
// 	wave_encode_stop
//	parameter:	none.
//	return:		status.
//===============================================================================================
static INT32S wave_encode_stop(void)
{
	INT32S nRet;
	AUD_ENC_WAVE_HEADER *pWaveHeadPara;

	nRet = save_final_data_to_storage();
	if(nRet < 0)
	{
		adc_work_mem_free();
		return AUD_RECORD_FILE_WRITE_ERR;
	}

	//write header
	pWaveHeadPara = &WaveHeadPara;
	pAudio_Encode_Para->FileLenth -= sizeof(AUD_ENC_WAVE_HEADER);
	pWaveHeadPara->RIFF_len = sizeof(AUD_ENC_WAVE_HEADER) + pAudio_Encode_Para->FileLenth - 8;//file size -8
	pWaveHeadPara->data_len = pAudio_Encode_Para->FileLenth;
	if(pAudio_Encode_Para->SourceType == C_GP_FS)
	{
		lseek(pAudio_Encode_Para->FileHandle, 0, SEEK_SET);
		fs_write(pAudio_Encode_Para->FileHandle, (INT32U)pWaveHeadPara, sizeof(AUD_ENC_WAVE_HEADER));
		fs_close(pAudio_Encode_Para->FileHandle);
	}
	else if(pAudio_Encode_Para->SourceType == C_USER_DEFINE)
	{
		audio_encode_data_write(1, (INT32U)pWaveHeadPara, sizeof(AUD_ENC_WAVE_HEADER));
	}

	//free memory
	adc_work_mem_free();
	return AUD_RECORD_STATUS_OK;
}

//===============================================================================================
// 	wave_encode_once
//	parameter:	workmem = work memory
//				buffer_addr = buffer address
//				cwlen = buffer size in word
//	return:		length in word
//===============================================================================================
static int wave_encode_once(void *workmem, const short* buffer_addr, int cwlen)
{
	INT8U  *dest_addr;
	INT32S nRet, temp;
	INT32U cblen=0, size;
	INT32U  free_size = 0;

	if(pAudio_Encode_Para->read_index < pAudio_Encode_Para->write_index)
    {
        free_size = C_BS_BUFFER_SIZE + pAudio_Encode_Para->read_index - pAudio_Encode_Para->write_index;
    }
    else
    {
        free_size = pAudio_Encode_Para->read_index - pAudio_Encode_Para->write_index;
    }

	cblen = cwlen<<1;
//    DBG_PRINT("cblen[%d] free_size[%d]\r\n",cblen,free_size);
	if(cblen >= free_size)
	{
	    return AUD_RECORD_FILE_WRITE_ERR;
	}

	temp = pAudio_Encode_Para->write_index + cblen;
	if(temp > C_BS_BUFFER_SIZE)
	{
		size = C_BS_BUFFER_SIZE - pAudio_Encode_Para->write_index;
		dest_addr = pAudio_Encode_Para->Bit_Stream_Buffer + pAudio_Encode_Para->write_index;
		gp_memcpy((INT8S*)dest_addr, (INT8S*)buffer_addr, size);

		temp = cblen - size;		//remain size
		if(temp > C_BS_BUFFER_SIZE/2)
		{
		    DBG_PRINT("C_BS_BUFFER_SIZE\r\n");
		}
		dest_addr = pAudio_Encode_Para->Bit_Stream_Buffer;
		buffer_addr += (size>>1); 	//word address
		gp_memcpy((INT8S*)dest_addr, (INT8S*)buffer_addr, temp);
		vTaskSuspendAll();
		pAudio_Encode_Para->write_index = temp;
		xTaskResumeAll();
	}
	else
	{
		dest_addr = pAudio_Encode_Para->Bit_Stream_Buffer + pAudio_Encode_Para->write_index;
		gp_memcpy((INT8S*)dest_addr, (INT8S*)buffer_addr, cblen);
		vTaskSuspendAll();
		pAudio_Encode_Para->write_index += cblen;
		xTaskResumeAll();
	}

	pAudio_Encode_Para->FileLenth += cblen;
	nRet = save_buffer_to_storage();
	if(nRet < 0)
	{
        DBG_PRINT("AUD_RECORD_FILE_WRITE_ERR\r\n");
		return AUD_RECORD_FILE_WRITE_ERR;
	}
//    DBG_PRINT("cwlen = %d\r\n",cwlen);
	return cwlen;
}
#endif
#endif

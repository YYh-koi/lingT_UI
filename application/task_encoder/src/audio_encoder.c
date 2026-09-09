#include "audio_encoder.h"
#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
//===================================================================================
// audio encode entry
//===================================================================================
void audio_encode_entrance(void)
{
	INT32S nRet = adc_record_task_create(osPriorityAboveNormal);
	if(nRet < 0)
		DBG_PRINT("adc_record_task_create fail !!!");
	else
		DBG_PRINT("adc_record_task_create ok !!!");
}

void audio_encode_exit(void)
{
    INT32S nRet = adc_record_task_del(osPriorityAboveNormal);
	if(nRet < 0)
		DBG_PRINT("adc_record_task_del fail !!!");
	else
		DBG_PRINT("adc_record_task_del ok !!!");
}
//===================================================================================
// start record
//===================================================================================
CODEC_START_STATUS audio_encode_start(INT32S handle,INT32U input_type, INT16U SampleRate, INT32U BitRate)
{
	INT64U temp;

	if(audio_record_get_status() == C_START_RECORD) {
		return REENTRY_ERROR;
	}

    if(handle < 0) {
        return RESOURCE_NO_FOUND_ERROR;
    }

    audio_record_set_status(C_STOP_RECORD);
    audio_record_set_source_type(C_GP_FS);
    temp = audio_record_set_file_handle_free_size(handle);
    if(temp <= (1024 * 10)) {
        return 	RESOURCE_WRITE_ERROR;
    }

	switch(input_type)
	{
        #if ADC_LINE_IN_EN == 1
        case ADC_LINE_IN:
            audio_record_set_channel(ADC_LINE_IN,1);
        break;
        #endif
        #if BUILD_IN_MIC_EN == 1
        case BUILD_IN_MIC:
            audio_record_set_channel(BUILD_IN_MIC,1);
        break;
        #endif
        #if (ADC_LINE_IN_EN == 1) && (BUILD_IN_MIC_EN == 1)
        case DOUBLE_LINEIN:
            audio_record_set_channel(DOUBLE_LINEIN,2);
        break;
        #endif
        #if BUILD_LINEIN_EN == 1
        case BUILD_LINEIN:
            audio_record_set_channel(BUILD_LINEIN,2);
        break;
        #endif
        default:
            DBG_PRINT("No input device support\r\n");
            audio_record_set_channel(BUILD_IN_MIC,1);
        break;
	}

    audio_record_set_info(WAVE_FORMAT_PCM, SampleRate, BitRate);

	if(adc_record_task_start() < 0) {
		return CODEC_START_STATUS_ERROR_MAX;
	}

	return START_OK;
}

//===================================================================================
// stop record
//===================================================================================
void audio_encode_stop(void)
{
	if(audio_record_get_status() == C_START_RECORD) {
		adc_record_task_stop();
	}
}

//====================================================================================
//	status
//====================================================================================
AUDIO_CODEC_STATUS audio_encode_status(void)
{
	if(audio_record_get_status() == C_START_RECORD)
		return AUDIO_CODEC_PROCESSING;
	else if(audio_record_get_status() == C_STOP_RECORD)
		return AUDIO_CODEC_PROCESS_END;
	else if(audio_record_get_status() == C_STOP_RECORDING)
		return AUDIO_CODEC_PROCESS_END;
	else if(audio_record_get_status() == C_START_FAIL)
		return AUDIO_CODEC_BREAK_OFF;
	else
		return AUDIO_CODEC_STATUS_MAX;
}

//===================================================================================
// audio_encode_set_CutLastFrame
//===================================================================================
INT8U audio_encode_set_CutLastFrame(INT8U flag)
{
	return audio_record_set_CutLastFrame(flag);
}
//====================================================================================
//	Set DownSample when audio record
//  bEnable: enable or disable
//  DownSampleFactor: 2, 3 or 4
//====================================================================================
CODEC_START_STATUS audio_encode_set_downsample(INT8U bEnable, INT8U DownSampleFactor)
{
	INT32S nRet;

	if(DownSampleFactor > 4)
		DownSampleFactor = 4;

	if(DownSampleFactor < 2)
		DownSampleFactor = 2;

	nRet = audio_record_set_down_sample(bEnable, DownSampleFactor);
	if(nRet >= 0)
		return START_OK;
	else
		return  RESOURCE_NO_FOUND_ERROR;
}
#endif

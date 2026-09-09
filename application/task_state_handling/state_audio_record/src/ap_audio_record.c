#include "application.h"
#include "ap_state_config.h"
#include "audio_encoder.h"
#include "../inc/ap_audio_record.h"
#include "ap_state_handling.h"
#include "task_storage_service.h"

#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)

static INT8U audio_record_sts;
static INT8U audio_record_display_timerid = 0xFF;
static INT8U audio_record_time_interval;
/* static */ INT8U audio_record_proc = 0;

static STOR_SERV_FILEINFO curr_audio_file_info;
void audio_calculate_left_recording_time_enable(void);
void audio_calculate_left_recording_time_disable(void);
INT32U audio_background_image_ptr=0;
INT32U audio_background_image_size=0;
INT8S ap_audio_record_init(INT32U prev_state)
{
	IMAGE_DECODE_STRUCT img_info;
	INT32U background_image_ptr=0;
	INT32U size;
	INT16U logo_fd;
	DISP_P	display_frame = NULL;
	INT32U	display_buf = 0;
	INT16U tft_w = drv_l2_display_tft_width_get();
	INT16U tft_h =  drv_l2_display_tft_height_get();

	//load time interval setting
	audio_record_time_interval = ap_state_config_record_time_get();
	DBG_PRINT("%s ... T:%d\r\n",__func__,audio_record_time_interval);
	ap_state_config_record_time_set(3); //mean: 5min, 0~4 <--> 0, 1, 3, 5, 10
	audio_record_proc = 0;

	// Clear all icons
    display_task_msg_send(MSG_DISPLAY_TASK_EFFECT_INIT, NULL, 0, MSG_PRI_NORMAL);


	if (ap_state_handling_storage_id_get() != STORAGE_DEVICE) {
		ap_audio_record_sts_set(AUDIO_RECORD_UNMOUNT);
		//ap_state_handling_icon_show_cmd(ICON_AUDIO_RECORD, ICON_NO_SD_CARD, NULL);
		audio_calculate_left_recording_time_disable();
		DBG_PRINT("[WARING]No SD Card\r\n");
	} else {
		ap_audio_record_sts_set(~AUDIO_RECORD_UNMOUNT);
		//ap_state_handling_icon_show_cmd(ICON_AUDIO_RECORD, NULL, NULL);
		//ap_state_handling_icon_clear_cmd(ICON_NO_SD_CARD, NULL, NULL);
		audio_calculate_left_recording_time_enable();
		DBG_PRINT("Audio Recodeing\r\n");
	}

#if C_BATTERY_DETECT
	ap_state_handling_current_bat_lvl_show();
	ap_state_handling_current_charge_icon_show();
#endif

#if 1
    do
    {
		logo_fd = nv_open((INT8U *) "POWER_ON_LOGO.JPG");
		if(logo_fd == 0xFFFF)
		{
			DBG_PRINT("open POWER_ON_LOGO.JPG fail.\r\n");
			break;
		}

		size = nv_rs_size_get(logo_fd);
		if(!size)
		{
			DBG_PRINT("State startup get logo size fail.[%d]\r\n", size);
			break;
		}

		background_image_ptr = (INT32U) gp_malloc(size);
		if(!background_image_ptr)
		{
			DBG_PRINT("State startup allocate jpeg input buffer fail.[0x%X]\r\n", background_image_ptr);
			break;
		}

		if(nv_read(logo_fd, (INT32U) background_image_ptr, size))
		{
			DBG_PRINT("Failed to read resource_header in ap_startup_init\r\n");
			break;
		}

		display_frame = ap_display_queue_get(display_free_queue);
		if(display_frame == NULL)
		{
			DBG_PRINT("State startup allocate display buffer fail.\r\n");
			break;
		}
		else
		{
			display_frame->offset = 0;
			display_buf = display_frame->base + display_frame->offset;
		}

		img_info.image_source = (INT32S) background_image_ptr;
		img_info.source_size = size;
		img_info.source_type = TK_IMAGE_SOURCE_TYPE_BUFFER;

		if(drv_l2_display_tft_fmt_get() == DISP_FMT_RGB565)
		{
			img_info.output_format = C_SCALER_CTRL_OUT_RGB565;
			audio_background_image_ptr =    (INT32U) gp_malloc(tft_w*tft_h<<2);
			audio_background_image_size = tft_w*tft_h<<2;
		}
		else if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
		{
			img_info.output_format = C_SCALER_CTRL_OUT_GP420;
			audio_background_image_ptr =    (INT32U) gp_malloc(tft_w*tft_h*3>>1);
			audio_background_image_size = tft_w*tft_h*3>>1;
		}
		else
		{
			img_info.output_format = C_SCALER_CTRL_OUT_YUYV;
			audio_background_image_ptr =    (INT32U) gp_malloc(tft_w*tft_h<<2);
			audio_background_image_size = tft_w*tft_h<<2;
		}

        /* if(drv_l2_display_tft_offset_get(1))
		{
			tft_h	=  (drv_l2_display_tft_height_get() - (drv_l2_display_tft_offset_get(1)*2));
			display_buf  += (tft_w*drv_l2_display_tft_offset_get(0));
		}
		else*/
		{
			tft_h	=  drv_l2_display_tft_height_get();
		}

		img_info.output_ratio = 0;
		img_info.out_of_boundary_color = 0x008080;

		img_info.output_buffer_width = tft_w;
		img_info.output_buffer_height = tft_h;
		img_info.output_image_width = tft_w;
		img_info.output_image_height = tft_h;

		img_info.output_buffer_pointer = display_buf;

		if(jpeg_buffer_decode_and_scale(&img_info,0,0) == STATUS_FAIL)
		{
			DBG_PRINT("State startup decode jpeg file fail.\r\n");
			break;
		}
		gp_memcpy((INT8S *)audio_background_image_ptr,(INT8S *)display_buf,audio_background_image_size);
		display_task_msg_send(MSG_DISPLAY_TASK_JPEG_DRAW, &display_frame, sizeof(DISP_P), MSG_PRI_NORMAL);

		display_frame = NULL;

	} while(0);

	if(background_image_ptr)
	{
		gp_free((void *) background_image_ptr);
		vTaskDelay(50);
	}

	if(display_frame != NULL)
	{
		ap_display_queue_put(display_free_queue, display_frame);
	}

//	ap_audio_record_timer_draw();
	ap_audio_record_timer_start();
#endif

	return STATUS_OK;
}

void ap_audio_ui_draw(void)
{
   	DISP_P	display_frame = NULL;
	INT32U	display_buf = 0;
    if(audio_background_image_ptr != 0)
    {
 		display_frame = ap_display_queue_get(display_free_queue);
		if(display_frame == NULL)
		{
			DBG_PRINT("State startup allocate display buffer fail.\r\n");
			return;
		}
		else
		{
			display_frame->offset = 0;
			display_buf = display_frame->base + display_frame->offset;
		}
		gp_memcpy((INT8S *)display_buf,(INT8S *)audio_background_image_ptr,audio_background_image_size);
		display_task_msg_send(MSG_DISPLAY_TASK_SENSOR_DRAW, &display_frame, sizeof(DISP_P), MSG_PRI_NORMAL);

		display_frame = NULL;

    }
}

void ap_audio_record_exit(INT32U next_state_msg)
{
	//store time interval setting
	ap_audio_record_timer_stop();
	ap_state_config_record_time_set(audio_record_time_interval);
	DBG_PRINT("%s ... \r\n",__func__);

	if(next_state_msg == MSG_APQ_MENU_KEY_ACTIVE) {
		DBG_PRINT("[E1]\r\n");
        app_msg_send(MSG_APQ_MENU_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);
	}

	if (audio_record_sts & AUDIO_RECORD_BUSY) {
		struct stat_t file_stat;
        DBG_PRINT("[E2]\r\n");
		ap_audio_record_sts_set(~AUDIO_RECORD_BUSY);
		ap_state_handling_icon_clear_cmd(ICON_RECORD_REC, 0, 0, 0);
		//timer_counter_force_display(0);

		audio_encode_stop();

		close(curr_audio_file_info.file_handle);
		curr_audio_file_info.file_handle = open((CHAR *) curr_audio_file_info.file_path_addr, O_RDONLY);
		fstat(curr_audio_file_info.file_handle, &file_stat);
		close(curr_audio_file_info.file_handle);
		if(file_stat.st_size == 0) {
			unlink((CHAR *) curr_audio_file_info.file_path_addr);
		}
		UsrFlushBuffers(STORAGE_DEVICE);
		osDelay(10);
		DBG_PRINT("D1\r\n");
		storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
	}
	audio_record_sts = 0;

	audio_calculate_left_recording_time_disable();
	ap_peripheral_auto_off_force_disable_set(0);	//wwj add, disable auto off
	if(audio_background_image_ptr!=0)
	{
        gp_free((void *)audio_background_image_ptr);
        audio_background_image_ptr = 0;
	}
	DBG_PRINT("E3\r\n");
}

void ap_audio_record_sts_set(INT8S sts)
{
	if (sts > 0) {
		audio_record_sts |= sts;
	} else {
		audio_record_sts &= sts;
	}
}

INT8U ap_audio_record_sts_get(void)
{
	return audio_record_sts;
}

void ap_audio_record_timer_start(void)
{
	if (audio_record_display_timerid == 0xFF) {
		audio_record_display_timerid = HOME_SELECT_TIMER_ID;
		drv_l2_sys_timer_set((void*)msgQSend, (void*)ApQ, MSG_APQ_DISPLAY_DRAW_TIMER, audio_record_display_timerid, AUDIO_RECORD_DISPLAY_TIMER_INTERVAL);
	}
}

void ap_audio_record_timer_stop(void)
{
	if (audio_record_display_timerid != 0xFF) {
		drv_l2_sys_timer_kill(audio_record_display_timerid);
		audio_record_display_timerid = 0xFF;
	}
}

void audio_calculate_left_recording_time_enable(void)
{
	INT32U freespace, left_recording_time;
    DISPLAY_REC_TIME    disp_time;

	freespace = vfsFreeSpace(STORAGE_DEVICE);

	if(freespace > (CARD_FULL_MB_SIZE << 20)) freespace -= CARD_FULL_MB_SIZE << 20;
	else freespace = 0;

	left_recording_time = freespace / (16000*2);

    disp_time.s = left_recording_time % 60;
    left_recording_time /= 60;
    disp_time.m = left_recording_time % 60;
    left_recording_time /= 60;
    disp_time.h = left_recording_time % 100;

    display_task_msg_send(MSG_DISPLAY_TASK_LEFT_REC_TIME_DRAW, &disp_time, sizeof(DISPLAY_REC_TIME), MSG_PRI_NORMAL);
}

void audio_calculate_left_recording_time_disable(void)
{
    display_task_msg_send(MSG_DISPLAY_TASK_LEFT_REC_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
}

extern int max_audio_record_file_size;
void ap_audio_record_func_key_active(void)
{

    if(audio_encode_status() == AUDIO_CODEC_PROCESSING) {
        DBG_PRINT("Stop\r\n");
		ap_audio_record_sts_set(~AUDIO_RECORD_BUSY);
		ap_state_handling_icon_clear_cmd(ICON_RECORD_REC, 0, 0, 0);
        audio_encode_stop();
		ap_peripheral_auto_off_force_disable_set(0);
		display_task_msg_send(MSG_DISPLAY_TASK_LEFT_REC_TIME_DRAW, NULL, 0, MSG_PRI_NORMAL);
    }else
    {
		ap_audio_record_sts_set(AUDIO_RECORD_BUSY);
        max_audio_record_file_size = 0x80000000;//2G
		ap_peripheral_auto_off_force_disable_set(1);
		ap_state_handling_icon_show_cmd(ICON_RECORD_REC, 0, 0, 0);
		display_task_msg_send(MSG_DISPLAY_TASK_REC_TIME_DRAW, NULL, 0, MSG_PRI_NORMAL);
		DBG_PRINT("@@AUDIO START\r\n");
		storage_task_msg_send(MSG_STORAGE_SERVICE_AUD_REQ, NULL, 0, MSG_PRI_NORMAL);
    }
}

INT8S ap_audio_record_reply_action(STOR_SERV_FILEINFO *file_info_ptr)
{
	CODEC_START_STATUS audio_status;

    if(file_info_ptr->file_handle < 0) {
        DBG_PRINT("file open failed\r\n");
        ap_peripheral_auto_off_force_disable_set(0);
        return STATUS_FAIL;
    }

    audio_status = audio_encode_start(file_info_ptr->file_handle, BUILD_IN_MIC , 44100, 0);
    if(audio_status != START_OK) {
        DBG_PRINT("start failed\r\n");
        return STATUS_FAIL;
    }
    return STATUS_OK;
}
// TBD: Need a timer to check audio encode status

#endif

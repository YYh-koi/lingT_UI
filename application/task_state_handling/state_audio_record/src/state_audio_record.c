#include "application.h"
#include "../inc/state_audio_record.h"
#include "audio_encoder.h"
#include "../inc/ap_audio_record.h"
#include "ap_state_handling.h"
#include "ap_state_config.h"
#include "drv_l2_sensor.h"
#include "audio_record.h"
#include "task_storage_service.h"

#if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
//	prototypes
INT8S state_audio_record_init(INT32U prev_state);
void state_audio_record_exit(INT32U next_state_msg);
void state_audio_record_exit_entry_PC_CAM(INT32U next_state_msg);

extern INT8U ap_audio_record_sts_get(void);
extern INT8U audio_record_proc;
extern void video_encode_preview_restart(void);
extern void audio_encode_exit(void);
extern void ap_audio_ui_draw(void);
INT8S state_audio_record_init(INT32U prev_state)
{
	DBG_PRINT("audio_record state init enter\r\n");

	audio_encode_entrance();    // create audio task

	if (ap_audio_record_init(prev_state) == STATUS_OK) {
			return STATUS_OK;
		} else {
			return STATUS_FAIL;
		}
	return STATUS_OK;
}

void state_audio_record_entry(void *para)
{
	EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
	INT32U msg_id, prev_state;
	prev_state = *((INT32U *) para);
	state_audio_record_init(prev_state);
	DBG_PRINT("audio_record state init enter OK!\r\n");
	while (exit_flag == EXIT_RESUME) {
		if (msgQReceive(ApQ, &msg_id, ApQ_para, ApQ_para_len) == STATUS_FAIL)
		{
			continue;
		}
		switch (msg_id) {
			case EVENT_APQ_ERR_MSG:
				audio_confirm_handler((STAudioConfirm *)ApQ_para);
				break;
			case MSG_STORAGE_SERVICE_MOUNT:
				ap_state_handling_storage_id_set(ApQ_para[0]);
			   	ap_audio_record_sts_set(~AUDIO_RECORD_UNMOUNT);
				audio_calculate_left_recording_time_enable();
        		//ap_audio_record_timer_draw();
        		DBG_PRINT("[Audio Record Mount OK]\r\n");
        		break;
        	case MSG_STORAGE_SERVICE_NO_STORAGE:
        		ap_state_handling_storage_id_set(ApQ_para[0]);
        		ap_audio_record_sts_set(AUDIO_RECORD_UNMOUNT);
				audio_calculate_left_recording_time_disable();
				//ap_audio_record_timer_draw();
        		DBG_PRINT("[Audio Record Mount FAIL]\r\n");
        		break;

        	case MSG_APQ_MENU_KEY_ACTIVE:
        	case MSG_APQ_NEXT_KEY_ACTIVE:
        	case MSG_APQ_PREV_KEY_ACTIVE:
        		break;

        	case MSG_APQ_MODE:
				//msg_id = STATE_PHOTO_CAPTURE;
				//xQueueSend(StateHandlingQ, (void*)&msg_id, 0);
                state_handling_msg_send(STATE_BROWSE, NULL, 0, MSG_PRI_NORMAL);
	            DBG_PRINT("audio state Mode key OK!\r\n");
	        	exit_flag = EXIT_BREAK;

        		break;

        	case MSG_APQ_FUNCTION_KEY_ACTIVE:		// OK key pressed
                #if C_SENSOR_PW_SAVING
                #if (defined _SENSOR_sc2143_CDSP_MIPI) && (_SENSOR_sc2143_CDSP_MIPI == 1)
                sc2143_mipi_sleep();
                #endif
                #if (defined _SENSOR_SC2363_CDSP_MIPI) && (_SENSOR_SC2363_CDSP_MIPI == 1)
                sc2363_mipi_sleep();
                #endif
                #endif
				if (ap_state_handling_storage_id_get() != STORAGE_DEVICE) {
					DBG_PRINT(" [Err]audio state NO_STORAGE!\r\n");
				} else {
					ap_audio_record_func_key_active();
				}

        		break;

        	case MSG_STORAGE_SERVICE_AUD_REPLY:
        		ap_audio_record_reply_action((STOR_SERV_FILEINFO *) ApQ_para);
        		break;

        	case MSG_APQ_POWER_KEY_ACTIVE:
        		ap_audio_record_exit(msg_id);
        		ap_state_handling_power_off(0);
        		break;
            #if(defined(C_USB_DEVICE) && C_USB_DEVICE)
        	case MSG_APQ_CONNECT_TO_PC:
        		if (ap_audio_record_sts_get() == AUDIO_RECORD_BUSY) {
        			ap_audio_record_func_key_active();	//Stop audio recording
        		}
				ap_audio_record_timer_stop();
        		audio_calculate_left_recording_time_disable();
				//ap_state_handling_str_draw_exit();
				//ap_audio_record_timer_draw();
				osDelay(10);
				msgQFlush(ApQ);
        		if (ap_state_config_usb_mode_get() == 0) {
        			state_audio_record_exit_entry_PC_CAM(msg_id);
        		}
				ap_state_handling_connect_to_pc(STATE_AUDIO_RECORD);
        		break;

        	case MSG_APQ_DISCONNECT_TO_PC:
				ap_state_handling_disconnect_to_pc();
				vTaskDelay(100);
				if (ap_state_config_usb_mode_get() == 0)
				{
					;//vid_enc_disable_sensor_clock();
				}
				else
				{
					if(ap_state_handling_storage_id_get() == STORAGE_DEVICE)
					{
		        		audio_calculate_left_recording_time_enable();
					}
					//ap_audio_record_timer_draw();
					ap_audio_record_timer_start();
        		}
        		//ap_music_update_icon_status();		// Restore MP3 icons
				break;
            #endif
			case MSG_APQ_USER_CONFIG_STORE:
				ap_state_config_store();
				break;

			case MSG_APQ_DISPLAY_DRAW_TIMER:
                //DBG_PRINT("u");
                ap_audio_ui_draw();
				break;

			case MSG_APQ_AUDIO_ENCODE_ERR:
				if(ap_audio_record_sts_get() & AUDIO_RECORD_BUSY) {
					ap_audio_record_func_key_active();
                    storage_task_msg_send(MSG_STORAGE_SERVICE_TIMER_START, NULL, 0, MSG_PRI_NORMAL);
                    storage_task_msg_send(MSG_STORAGE_SERVICE_STORAGE_CHECK, NULL, 0, MSG_PRI_NORMAL);
					//msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
					//msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_STORAGE_CHECK, NULL, 0, MSG_PRI_NORMAL);
				}
				break;

			case MSG_APQ_AUDIO_EFFECT_MENU:
				if(ap_audio_record_sts_get() & AUDIO_RECORD_BUSY) {
					break;
				}
			case MSG_APQ_AUDIO_EFFECT_OK:
			case MSG_APQ_AUDIO_EFFECT_MODE:
				ap_state_common_handling(msg_id);
				break;

			default:
				break;
		}
	}

	if (exit_flag == EXIT_BREAK) {
		state_audio_record_exit(msg_id);
	}
}

void state_audio_record_exit(INT32U next_state_msg)
{
	ap_audio_record_exit(next_state_msg);
	audio_encode_exit();  // delete audio record task
	DBG_PRINT("Exit audio_record state\r\n");
}

void state_audio_record_exit_entry_PC_CAM(INT32U next_state_msg)
{
	state_audio_record_exit(next_state_msg);

}

#endif

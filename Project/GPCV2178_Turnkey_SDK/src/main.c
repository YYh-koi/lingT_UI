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
#include "project.h"
#include "turnkey_version.h"
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
#include "socket_cmd.h"
#endif
#include "bt_xstack_demo.h"
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/
extern void video_encode_state_task_init(void);
extern void audio_encode_state_task_init(void);
#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
#if (WIFI_VENDOR == WIFI_VENDOR_ATBM)
extern INT32S atbm_wifi_init(void);
extern INT32S atbm_wifi_p2p_mode_enable(void);
#else
extern INT32S wal_ap_mode_enable();
#endif
#endif
/* == Blind Spot Detection == */
#if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
extern void bsd_task_entry(void const *para);
#endif

/*== HOST USB MJPEG CAM ==*/
#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
extern void uvc_q_init();
#endif

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
uint32_t SystemCoreClock;
static HeapRegion_t xHeapRegions[2];
INT32U SDRAM_END_ADDR;


INT32U	fps_tft_vblank=0;
INT32U	fps_tft_update=0;
INT32U  fps_isp_eof=0;
INT32U  fps_isp_vd=0;
INT32U  fps_audrec_dma=0;
INT32U  rear_wr_cnt = 0;
INT32U  front_wr_cnt = 0;
INT32U  fps_wifi_cnt = 0;
INT32U  wifi_wr_cnt = 0;
INT32U 	dummy_2k_cnt = 0;
INT32U  jpeg_done_cnt = 0;
INT32U  skip_rear_wr_cnt = 0;
INT32U  skip_front_wr_cnt = 0;
INT32U  skip_wifi_wr_cnt = 0;

TURNKEY_ARGUMENT    TurnkeyArgs = { {0} };
TURNKEY_ARGUMENT*   pTurnkeyArgs = &TurnkeyArgs;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
#if configRECORD_STACK_HIGH_ADDRESS
void
CheckTaskStack(
	void
)
{
	float stack_usage = 0;
	UBaseType_t T;
	UBaseType_t task_stack_size;
	UBaseType_t task_remain_size;

	UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
	TaskStatus_t *pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );
	if(pxTaskStatusArray==0) return;

	uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, NULL );

	DBG_PRINT("\r\n------------------------------------\r\n");

	for(T=0; T<uxArraySize; T++)
	{
		TaskStatus_t *Task = pxTaskStatusArray+T;

		task_stack_size = uxTaskGetStackSize(Task->xHandle);

		task_remain_size = uxTaskGetStackHighWaterMark(Task->xHandle);

		stack_usage = 100.0f*((float)task_stack_size - (float)task_remain_size )/ (float)task_stack_size;

		DBG_PRINT(">name=%24s, stack usage=%5.2f%%, free/all=%4d/%4d.\r\n",
			Task->pcTaskName, stack_usage,
			task_remain_size, task_stack_size);
	}

	DBG_PRINT("------------------------------------\r\n\r\n");

	vPortFree( pxTaskStatusArray );
}
#endif

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
void wifi_ap_mode_enable(void)
{
	#if (WIFI_VENDOR == WIFI_VENDOR_ATBM)
	static INT8U wifi_first_run_flag = 0;
	INT32S wifi_init_status = atbm_wifi_init();
	if(wifi_init_status != STATUS_OK)
	{
		ERR_PRINT("ATBM WiFi init failed: %d\r\n", wifi_init_status);
		return;
	}
	if(atbm_wifi_p2p_mode_enable() == STATUS_OK)
	{
		#if (defined C_BT_XSTACK_DEMO_ENABLE) && (C_BT_XSTACK_DEMO_ENABLE)
		INT32S bt_status = bt_xstack_demo_start();
		if(bt_status != 0)
		{
			ERR_PRINT("xstack demo start failed: %d\r\n", bt_status);
		}
		#endif
		if(wifi_first_run_flag == 0)
		{
			wifi_first_run_flag = 1;
			socket_cmd_service_init(PRIORITY_SOCKET_CMD, SOCKET_CMD_TASK_STACK_SIZE);
			rtsp_h264_start_service();
		}
	}
	#elif (WIFI_VENDOR == WIFI_VENDOR_RTK)
	static INT8U wifi_first_run_flag = 0;
	wal_ap_mode_enable();
	if(wifi_first_run_flag == 0)
	{
        wifi_first_run_flag = 1;
        socket_cmd_service_init(PRIORITY_SOCKET_CMD, SOCKET_CMD_TASK_STACK_SIZE);
        rtsp_h264_start_service();
	}
	#elif (WIFI_VENDOR == WIFI_VENDOR_SSV)
	Wifi_Init();
	#endif


}
#endif

void
InitTurnkeyTaskCfg(
	void
)
{
	TASK_APP_ARGUMENT* pDispTaskCfg = &pTurnkeyArgs->taskDisplay;

	#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
	TASK_APP_ARGUMENT* pUSBDevTaskCfg = &pTurnkeyArgs->taskUSBDevice;
	#endif

	TASK_APP_ARGUMENT* pPeriTaskCfg = &pTurnkeyArgs->taskPeripheral;
	TASK_APP_ARGUMENT* pStateHdlgTaskCfg = &pTurnkeyArgs->taskStateHdlg;
	TASK_APP_ARGUMENT* pStateAppTaskCfg = &pTurnkeyArgs->taskStateApp;
	TASK_APP_ARGUMENT* pVideoEncTaskCfg = &pTurnkeyArgs->taskVideoEnc;
	TASK_APP_ARGUMENT* pAudioEncTaskCfg = &pTurnkeyArgs->taskAudioEnc;
	#if (defined C_STORAGE_DEVICE) && (C_STORAGE_DEVICE)
	TASK_APP_ARGUMENT* pStorageTaskCfg = &pTurnkeyArgs->taskStorage;
	#endif
	TASK_APP_ARGUMENT* pAudioDecTaskCfg = &pTurnkeyArgs->taskAudioDec;

	#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
	TASK_APP_ARGUMENT* pVoiceRecgTaskCfg = &pTurnkeyArgs->taskVoiceRecg;
	#endif

    #if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
	TASK_APP_ARGUMENT* pBSDTaskCfg = &pTurnkeyArgs->taskBSD;
	#endif

    #if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
	TASK_APP_ARGUMENT* pLVGLTaskCfg = &pTurnkeyArgs->taskLVGL;
	#endif

	/* == Display == */

	#if C_DISABLE_SENSOR_TFT
	pDispTaskCfg->taskMsgLen = DISPLAY_TASK_QUEUE_MAX_LEN;
	pDispTaskCfg->taskRecvBufLen = sizeof(void *);
	display_task_init();
	#endif

	#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
	/* ==USB Device == */
	pUSBDevTaskCfg->taskMsgLen = USB_TASK_QUEUE_MAX;
	pUSBDevTaskCfg->taskRecvBufLen = 0;
	usbd_state_task_init();
	#endif

	/* == Peripheral == */
	pPeriTaskCfg->taskMsgLen = PERIPHERAL_TASK_QUEUE_MAX_LEN;
	pPeriTaskCfg->taskRecvBufLen = sizeof(INT32U);
	peripheral_handling_task_init();

	/* == State Handling == */
	pStateHdlgTaskCfg->taskMsgLen = STATE_HANDLING_TASK_QUEUE_MAX_LEN;
	pStateHdlgTaskCfg->taskRecvBufLen = 0;
	state_handling_task_init();

	/* == State App == */
	pStateAppTaskCfg->taskMsgLen = STATE_APP_TASK_QUEUE_MAX_LEN;
	pStateAppTaskCfg->taskRecvBufLen = max(max(sizeof(STOR_SERV_FILEINFO), sizeof(STAudioConfirm)), sizeof(STOR_SERV_PLAYINFO));
	state_app_init();

	/* == Video Encoder State == */
	pVideoEncTaskCfg->taskMsgLen = VIDEO_ENC_TASK_QUEUE_MAX_LEN;
	pVideoEncTaskCfg->taskRecvBufLen = 0;
	video_encode_state_task_init();

	/* == Audio Encoder State == */
	pAudioEncTaskCfg->taskMsgLen = AUDIO_ENC_TASK_QUEUE_MAX_LEN;
	pAudioEncTaskCfg->taskRecvBufLen = 0;
	audio_encode_state_task_init();

	/* == Storage Service == */
	#if (defined C_STORAGE_DEVICE) && (C_STORAGE_DEVICE)
	pStorageTaskCfg->taskMsgLen = STORAGE_SERVICE_TASK_QUEUE_MAX_LEN;
	pStorageTaskCfg->taskRecvBufLen = sizeof(INT32U);
	storage_service_init();
	#endif

    /* == Audio Decode == */
	pAudioDecTaskCfg->taskMsgLen = AUDIO_DEC_QUEUE_MAX;
	pAudioDecTaskCfg->taskRecvBufLen = sizeof(STAudioTaskPara);
	audio_dec_task_init();

    /* == Voice Recognition == */
	#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
	pVoiceRecgTaskCfg->taskMsgLen = VOICE_RECG_TASK_QUEUE_MAX_LEN;
	pVoiceRecgTaskCfg->taskRecvBufLen = sizeof(INT32U);
	voice_recg_state_task_init();
	#endif

    /* == Blind Spot Detection == */
	#if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
	pBSDTaskCfg->taskMsgLen = BSD_TASK_QUEUE_MAX_LEN;
	pBSDTaskCfg->taskRecvBufLen = sizeof(INT32U);
	bsd_task_init();
	#endif

	#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
	/* ==LVGL == */
	pLVGLTaskCfg->taskMsgLen = LVGL_TASK_QUEUE_MAX_LEN;
	pLVGLTaskCfg->taskRecvBufLen = 0;
	lvgl_state_task_init();
	#endif

    /* == H264 == */
	// Set it here becase h264ack_q is not released
	h264_drv_init();

	#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	sw_wifi_ack_init();
	#endif

	/*== HOST USB MJPEG CAM ==*/
	#if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
	uvc_q_init();
	#endif

}

/**
 * @brief   Initial task
 * @param   none
 * @return 	none
 */
void InitTask()
{
	pTurnkeyArgs = &TurnkeyArgs;

	#if C_DISABLE_SENSOR_TFT
	osThreadDef_t disp_task = { "disp_task", display_task_entry, PRIORITY_DISPLAY_TASK, 1, DISPLAY_TASK_STACK_SIZE};
	#endif

	#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
	osThreadDef_t usb_task = { "usb_dev_task", state_usb_device_entry, PRIORITY_USB_DEVICE, 1, USB_TASK_STACK_SIZE};
	#endif

	osThreadDef_t peripheral_task = { "peripheral_task", task_peripheral_handling_entry, PRIORITY_PERIPHERAL_HANDLING, 1, PERIPHERAL_HANDLING_STACK_SIZE};
	osThreadDef_t state_task = { "state_handling_task", state_handling_entry, PRIORITY_STATE_HANDLING, 1, STATE_HANDLING_STACK_SIZE};
	#if (defined C_STORAGE_DEVICE) && (C_STORAGE_DEVICE)
	osThreadDef_t storage_task = { "storage_task", task_storage_service_entry, PRIORITY_STORAGE_SERVICE, 1, STORAGE_SERVICE_TASK_STACK_SIZE};
	#endif
	osThreadDef_t audio_dec_task = { "audio_dec_task", audio_dec_task_entry, PRIORITY_AUD_DEC_TASK, 1, AUDIO_DEC_TASK_STACK_SIZE};
	osThreadDef_t audio_dac_task = { "audio_dac_task", audio_dac_task_entry, PRIORITY_AUD_DAC_TASK, 1, AUDIO_DAC_TASK_STACK_SIZE};

	#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
	osThreadDef_t lvgl_task = { "lvgl_task", state_lvgl_entry, PRIORITY_LVGL, 1, LVGL_TASK_STACK_SIZE};
	#endif
	#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
	osThreadDef_t voice_recg_task = { "voice_recg_task", voice_recg_task_entry, PRIORITY_VOICE_RECG_TASK, 1, VOICE_RECG_TASK_STACK_SIZE};
	#endif

    #if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
	osThreadDef_t bsd_task = { "bsd_task", bsd_task_entry, PRIORITY_BSD_TASK, 1, BSD_TASK_STACK_SIZE};
	#endif

	/* Config Task */
	InitTurnkeyTaskCfg();

    #if C_DISABLE_SENSOR_TFT
	/* Create Task */
	if(osThreadCreate(&disp_task, NULL) == 0)
	{
		ERR_PRINT("fail to create display task \r\n");
	}
    #endif

#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
	if(osThreadCreate(&lvgl_task, NULL) == 0)
	{
		ERR_PRINT("fail to create lvgl task \r\n");
	}
#endif
	if(osThreadCreate(&audio_dac_task, NULL) == 0)
	{
		ERR_PRINT("fail to create audio dac task \r\n");
	}

	if(osThreadCreate(&audio_dec_task, NULL) == 0)
	{
		ERR_PRINT("fail to create audio decode task \r\n");
	}

	if(osThreadCreate(&peripheral_task, NULL) == 0)
	{
		ERR_PRINT("fail to create peripheral task \r\n");
	}

	if(osThreadCreate(&state_task, NULL) == 0)
	{
		ERR_PRINT("fail to create state handling task \r\n");
	}

	#if (defined C_STORAGE_DEVICE) && (C_STORAGE_DEVICE)
	if(osThreadCreate(&storage_task, NULL) == 0)
	{
		ERR_PRINT("fail to create storage task \r\n");
	}
	#endif

	#if (defined C_VOICE_RECOGNIZE) && (C_VOICE_RECOGNIZE)
	if(osThreadCreate(&voice_recg_task, NULL) == 0)
	{
		ERR_PRINT("fail to create voice recognition task \r\n");
	}
	#endif

	#if (defined C_USB_DEVICE) && (C_USB_DEVICE)
	if(osThreadCreate(&usb_task, NULL) == 0)
	{
		ERR_PRINT("fail to create usb device task \r\n");
	}
	#endif

    #if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
	if(osThreadCreate(&bsd_task, NULL) == 0)
	{
		ERR_PRINT("fail to create blind spot detection task \r\n");
	}
	#endif

    #if (defined _SENSOR_USB_MJPEG_CAM) && (_SENSOR_USB_MJPEG_CAM == 1)
	state_usbh_uvc_entry(NULL);
	#endif

	#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	#if (C_WIFI_ON_OFF_KEY==0)
	/* Start Wi-Fi/BT before enabling the panel output and backlight. */
	wifi_ap_mode_enable();
	/* The display task defers backlight-on requests until this flag is set. */
	extern INT32U g_bl_delay_enable;
	g_bl_delay_enable = 1;
	drv_l2_tft_backlight_en_set(TRUE);
	#endif
	#endif

	/* TP2915 output is enabled after Wi-Fi/BT startup. */
	drv_l1_disp_dev_en_set(TFT_1, TRUE);
	DBG_PRINT("[MAIN] TFT output enabled after-wifi\r\n");

	#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
	/* Release LVGL after Wi-Fi/BT and panel startup. */
	extern volatile INT32U g_lvgl_start_ok;
	g_lvgl_start_ok = 1;
	DBG_PRINT("[MAIN] LVGL start released after-wifi\r\n");
	#endif


#if 0
	do
	{
		osDelay(3 * 1000);
		DBG_PRINT("free heap size: 0x%x\r\n",xPortGetFreeHeapSize());

	} while(1);
#endif

#if STATISTICS_DUMP
	do
	{
		osDelay(STATISTICS_DUMP_TIME * 1000);
        DBG_PRINT("\r\n");
		#if 0
        DBG_PRINT("FPS_tft(vblank,update,ispEof,ispVD, audDMA)=(%d, %d, %d, %d, %d)\r\n"
            , (fps_tft_vblank	/ STATISTICS_DUMP_TIME)
            , (fps_tft_update	/ STATISTICS_DUMP_TIME)
            , (fps_isp_eof      / STATISTICS_DUMP_TIME)
            , (fps_isp_vd       / STATISTICS_DUMP_TIME)
            , (fps_audrec_dma   / STATISTICS_DUMP_TIME)
        );
        fps_tft_vblank	= 0;
        fps_tft_update	= 0;
        fps_isp_eof     = 0;
        fps_isp_vd		= 0;
        fps_audrec_dma	= 0;
		#else
        DBG_PRINT("FPS_tft(vblank,update,ispEof,front,rear,Wifi,jpeg,fdumy)=(%d, %d, %d, %d, %d, %d, %d, %d)\r\n"
            , (fps_tft_vblank	/ STATISTICS_DUMP_TIME)
            , (fps_tft_update	/ STATISTICS_DUMP_TIME)
            , (fps_isp_eof      / STATISTICS_DUMP_TIME)
            , (front_wr_cnt		/ STATISTICS_DUMP_TIME)
            , (rear_wr_cnt		/ STATISTICS_DUMP_TIME)
            , (wifi_wr_cnt      / STATISTICS_DUMP_TIME)
            , (jpeg_done_cnt    / STATISTICS_DUMP_TIME)
            , (dummy_2k_cnt)
        );
        /*
        DBG_PRINT("SKIP(front,rear,Wifi)=(%d, %d, %d)\r\n"
            , (skip_front_wr_cnt	/ STATISTICS_DUMP_TIME)
            , (skip_rear_wr_cnt		/ STATISTICS_DUMP_TIME)
            , (skip_wifi_wr_cnt     / STATISTICS_DUMP_TIME)
        );*/
         DBG_PRINT("free heap size: 0x%x\r\n",xPortGetFreeHeapSize());

        fps_tft_vblank	= 0;
        fps_tft_update	= 0;
        fps_isp_eof     = 0;
        front_wr_cnt	= 0;
        rear_wr_cnt	    = 0;
        wifi_wr_cnt     = 0;
        dummy_2k_cnt    = 0;
        jpeg_done_cnt   = 0;
        skip_front_wr_cnt	= 0;
        skip_rear_wr_cnt	    = 0;
        skip_wifi_wr_cnt     = 0;
		#endif
	} while(1);
#endif

	while(1)
	{
		osDelay(20000);

		#if configRECORD_STACK_HIGH_ADDRESS
		CheckTaskStack();
		#endif

    }
}

/*********************************************************************
 *
 *  heap_init()
 *
 *********************************************************************/
#define FIX_NUM_64   (8*12)
#define FIX_NUM_256  (8*28)
#define FIX_NUM_1024 (8*8)
#define FIX_NUM_2048 (8*4)
#define FIX_NUM_4096 (8*4)
ALIGN4 unsigned char fix_64_flag[FIX_NUM_64/8] = {0};
ALIGN4 unsigned int fix_64_addr[FIX_NUM_64] = {0};
ALIGN4 unsigned char fix_256_flag[FIX_NUM_256/8] = {0};
ALIGN4 unsigned int fix_256_addr[FIX_NUM_256] = {0};
ALIGN4 unsigned char fix_1024_flag[FIX_NUM_1024/8] = {0};
ALIGN4 unsigned int fix_1024_addr[FIX_NUM_1024] = {0};
ALIGN4 unsigned char fix_2048_flag[FIX_NUM_2048/8] = {0};
ALIGN4 unsigned int fix_2048_addr[FIX_NUM_2048] = {0};
ALIGN4 unsigned char fix_4096_flag[FIX_NUM_4096/8] = {0};
ALIGN4 unsigned int fix_4096_addr[FIX_NUM_4096] = {0};
mm_type CHUNK_MEM = {0};
INT32U GP_MALLOC_END_ADDR=0;
void heap_init(void *mem_base,void *mem_end)
{
    INT32U chunk_start_addr;
	INT32U chunk_end_addr;
    INT32U size;
	/* GP Chunk */
	chunk_start_addr = ((INT32U)mem_base + 0x3F) & (~0x3F);
	size = 64*FIX_NUM_64 + 256*FIX_NUM_256 + 1024*FIX_NUM_1024 + 2048*FIX_NUM_2048 + 4096*FIX_NUM_4096 + 1024;
	GP_MALLOC_END_ADDR = chunk_end_addr = chunk_start_addr + size;
    sp_fix_MM_BYTE_NUM_set(&CHUNK_MEM, 64,  FIX_NUM_64, fix_64_flag, fix_64_addr);
    sp_fix_MM_BYTE_NUM_set(&CHUNK_MEM, 256,  FIX_NUM_256, fix_256_flag, fix_256_addr);
    sp_fix_MM_BYTE_NUM_set(&CHUNK_MEM, 1024, FIX_NUM_1024, fix_1024_flag, fix_1024_addr);
    sp_fix_MM_BYTE_NUM_set(&CHUNK_MEM, 2048, FIX_NUM_2048, fix_2048_flag, fix_2048_addr);
    sp_fix_MM_BYTE_NUM_set(&CHUNK_MEM, 4096, FIX_NUM_4096, fix_4096_flag, fix_4096_addr);
	sp_mm_init(&CHUNK_MEM, chunk_start_addr, chunk_end_addr);

	/*=== Init Free MEM ===*/
	SDRAM_END_ADDR = (INT32U)mem_end -1;
	xHeapRegions[0].pucStartAddress = (uint8_t *)chunk_end_addr;
	xHeapRegions[0].xSizeInBytes = SDRAM_END_ADDR - (INT32U)chunk_end_addr;

	/* Terminates the array. */
	xHeapRegions[1].pucStartAddress = NULL;
	xHeapRegions[1].xSizeInBytes = 0;

	vPortDefineHeapRegions( xHeapRegions );
}

/*********************************************************************
 *
 *  main()
 *
 *********************************************************************/
int main()
 {
	/*=== Init MMU ===*/
	MMU_PTCreate();
	drv_l1_cp15_mmu_enable();

	drv_l1_init();

	board_init();

	// init watchdog
	#if C_WATCHDOG
	drv_l1_sys_wdt_period_set(SYS_WDT_4T0_SEC);
	drv_l1_sys_wdt_enable_set(SYS_WDT_EN_ON);
	drv_l1_sys_wdt_clear();
	#else
	drv_l1_sys_wdt_enable_set(SYS_WDT_EN_OFF);
	#endif

	drv_l1_clock_print_major_clock_info();

	drv_l2_init();

	fs_init();

	DBG_PRINT("==SYS_CLK=%d==\r\n", MCLK);
	DBG_PRINT("==SVN:%s==Build Time:%s==\r\n",TURNKEY_SVN_VERSION,TURNKEY_BUILD_TIME);
	DBG_PRINT("==MEM Start:0x%x==Free:0x%x==\r\n",xHeapRegions[0].pucStartAddress,xHeapRegions[0].xSizeInBytes);

	#if C_RTC_REBOOT
	ap_peripheral_rtc_reboot_init();
	#endif

	osThreadDef(InitTask, osPriorityNormal, 0, INIT_TASK_STACKSIZE);
	osKernelStart(&os_thread_def_InitTask, NULL);

	return 0;
}

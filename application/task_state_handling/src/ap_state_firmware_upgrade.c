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

#define C_UPGRADE_FILE_SLICE		0
#define C_UPGRADE_BUFFER_SIZE		0x10000		// Must be multiple of 64K

#define C_UPGRADE_SPI_BLOCK_SIZE	0x10000	// 64K
#define C_UPGRADE_SPI_WRITE_SIZE	0x100	// 256 bytes
#define C_UPGRADE_FAIL_RETRY		20		// Must larger than 0

#define FW_UPDATE_TIME_LIMIT		0	// in seconds
#define SPIFC_RESOURCE_OFFSET		0

extern INT32S	SPIFC_Flash_erase_block(INT32U addr);
extern INT32S   SPIFC_Flash_read_page(INT32U addr, INT8U *buf, INT32U byte_len);
extern INT32S	SPIFC_Flash_write_page(INT32U addr, INT8U *buf);

enum
{
	DRAW_PERCENT,
	DRAW_FINISH,
	DRAW_FAIL
};

DISPLAY_ICONSHOW    *str_fail_icon = NULL;
DISPLAY_ICONSHOW    *str_prefix_icon = NULL;
DISPLAY_ICONSHOW    *str_percent_icon = NULL;
DISPLAY_ICONSHOW    *str_icon_0 = NULL;
DISPLAY_ICONSHOW    *str_icon_1 = NULL;
DISPLAY_ICONSHOW    *str_icon_2 = NULL;
DISPLAY_ICONSHOW    *str_icon_3 = NULL;
DISPLAY_ICONSHOW    *str_icon_4 = NULL;
DISPLAY_ICONSHOW    *str_icon_5 = NULL;
DISPLAY_ICONSHOW    *str_icon_6 = NULL;
DISPLAY_ICONSHOW    *str_icon_7 = NULL;
DISPLAY_ICONSHOW    *str_icon_8 = NULL;
DISPLAY_ICONSHOW    *str_icon_9 = NULL;


/*
  當Resource有新增或減少檔案時,應該會讓字庫位置改變,因此在sd card 升級時 , 應該會去改到字庫的offset , 導致先找不到字庫
  當然也就發生花屏 , 解決方式 =>
  第一次去讀resource字串時, 就把它keep 在ram中
*/
#if C_TFT_DISPLAY_FUNC
static void show_burn_state(INT32U type, INT8U val)
{
    INT16U	tft_w = drv_l2_display_tft_width_get();
    INT16U	tft_h = drv_l2_display_tft_height_get();
    static DISPLAY_ICONSHOW *ptr_digit[10];
    INT8U digit1, digit2;

	DISP_P display_frame;
	INT32U display_buf;

	char str_tmp[64];

    if(str_prefix_icon == NULL)
    {
        str_prefix_icon = allocate_and_draw_ascii_string_icon("Upgrade...", WHITE_COLOR);
        if(!str_prefix_icon)
        {
            DBG_PRINT("[%s:%d]: allocate ascii string fail\r\n", __FUNCTION__, __LINE__);
        }

        str_fail_icon = allocate_and_draw_ascii_string_icon("Fail", RED_COLOR);
        if(!str_fail_icon)
        {
            DBG_PRINT("[%s:%d]: allocate ascii string fail\r\n", __FUNCTION__, __LINE__);
        }

        str_percent_icon = allocate_and_draw_ascii_string_icon("%", WHITE_COLOR);
        if(!str_percent_icon)
        {
            DBG_PRINT("[%s:%d]: allocate ascii string fail\r\n", __FUNCTION__, __LINE__);
        }

        str_icon_0 = allocate_and_draw_ascii_string_icon("0", WHITE_COLOR);
        if(!str_icon_0)
        {
            DBG_PRINT("[%s:%d]: allocate ascii string fail\r\n", __FUNCTION__, __LINE__);
        }

        str_icon_1 = allocate_and_draw_ascii_string_icon("1", WHITE_COLOR);
        if(!str_icon_1)
        {
            DBG_PRINT("[%s:%d]: allocate ascii string fail\r\n", __FUNCTION__, __LINE__);
        }

        str_icon_2 = allocate_and_draw_ascii_string_icon("2", WHITE_COLOR);
        if(!str_icon_2)
        {
            DBG_PRINT("[%s:%d]: allocate ascii string fail\r\n", __FUNCTION__, __LINE__);
        }

        str_icon_3 = allocate_and_draw_ascii_string_icon("3", WHITE_COLOR);
        if(!str_icon_3)
        {
            DBG_PRINT("[%s:%d]: allocate ascii string fail\r\n", __FUNCTION__, __LINE__);
        }

        str_icon_4 = allocate_and_draw_ascii_string_icon("4", WHITE_COLOR);
        if(!str_icon_4)
        {
            DBG_PRINT("[%s:%d]: allocate ascii string fail\r\n", __FUNCTION__, __LINE__);
        }

        str_icon_5 = allocate_and_draw_ascii_string_icon("5", WHITE_COLOR);
        if(!str_icon_5)
        {
            DBG_PRINT("[%s:%d]: allocate ascii string fail\r\n", __FUNCTION__, __LINE__);
        }

        str_icon_6 = allocate_and_draw_ascii_string_icon("6", WHITE_COLOR);
        if(!str_icon_6)
        {
            DBG_PRINT("[%s:%d]: allocate ascii string fail\r\n", __FUNCTION__, __LINE__);
        }

        str_icon_7 = allocate_and_draw_ascii_string_icon("7", WHITE_COLOR);
        if(!str_icon_7)
        {
            DBG_PRINT("[%s:%d]: allocate ascii string fail\r\n", __FUNCTION__, __LINE__);
        }

        str_icon_8 = allocate_and_draw_ascii_string_icon("8", WHITE_COLOR);
        if(!str_icon_8)
        {
            DBG_PRINT("[%s:%d]: allocate ascii string fail\r\n", __FUNCTION__, __LINE__);
        }

        str_icon_9 = allocate_and_draw_ascii_string_icon("9", WHITE_COLOR);
        if(!str_icon_9)
        {
            DBG_PRINT("[%s:%d]: allocate ascii string fail\r\n", __FUNCTION__, __LINE__);
        }

        // display position set for str_prefix_icon
        if(tft_w > str_prefix_icon->icon_w)
        {
            str_prefix_icon->pos_x = (tft_w - str_prefix_icon->icon_w) >> 1;
        }
        else
        {
            str_prefix_icon->pos_x = 0;
        }

        str_prefix_icon->pos_y = (tft_h - str_prefix_icon->icon_h * 2) >> 1;
        str_prefix_icon->pos_x &= ~0x0001;
        str_prefix_icon->pos_y &= ~0x0001;

        // display position set for str_fail_icon
        str_fail_icon->pos_x = (tft_w - str_fail_icon->icon_w) >> 1;
        str_fail_icon->pos_y = str_prefix_icon->pos_y + str_prefix_icon->icon_h;
        str_fail_icon->pos_x &= ~0x0001;
        str_fail_icon->pos_y &= ~0x0001;

        ptr_digit[0] = str_icon_0;
        ptr_digit[1] = str_icon_1;
        ptr_digit[2] = str_icon_2;
        ptr_digit[3] = str_icon_3;
        ptr_digit[4] = str_icon_4;
        ptr_digit[5] = str_icon_5;
        ptr_digit[6] = str_icon_6;
        ptr_digit[7] = str_icon_7;
        ptr_digit[8] = str_icon_8;
        ptr_digit[9] = str_icon_9;
    }

	display_frame = ap_display_queue_get(display_free_queue);
	while(display_frame == NULL)
	{
		osDelay(1);
		display_frame = ap_display_queue_get(display_free_queue);
	}

	display_frame->offset = 0;
	display_buf = display_frame->base + display_frame->offset;

	ap_display_clear_screen(display_buf);

	switch(type)
	{
    case DRAW_PERCENT:
        sprintf(str_tmp, "Upgrade %3d%%", val);
        DBG_PRINT("%s\r\n", str_tmp);

        ap_display_icon_draw((INT16U *)display_buf, tft_w, tft_h, str_prefix_icon, 0);

        if(val > 99)
        {
            val = 100;
            goto    show_burn_finish;
        }
        digit1 = val / 10;
        digit2 = val % 10;

        ptr_digit[digit1]->pos_x = (tft_w - str_icon_0->icon_w * 3) >> 1;
        ptr_digit[digit1]->pos_y = str_prefix_icon->pos_y + str_prefix_icon->icon_h;
        ap_display_icon_draw((INT16U *)display_buf, tft_w, tft_h, ptr_digit[digit1], 0);

        ptr_digit[digit2]->pos_x = ptr_digit[digit1]->pos_x + ptr_digit[digit1]->icon_w;
        ptr_digit[digit2]->pos_y = ptr_digit[digit1]->pos_y;
        ap_display_icon_draw((INT16U *)display_buf, tft_w, tft_h, ptr_digit[digit2], 0);

        str_percent_icon->pos_x = ptr_digit[digit2]->pos_x + ptr_digit[digit2]->icon_w;
        str_percent_icon->pos_y = ptr_digit[digit2]->pos_y;
        ap_display_icon_draw((INT16U *)display_buf, tft_w, tft_h, str_percent_icon, 0);
        break;

show_burn_finish:
    case DRAW_FINISH:
        sprintf(str_tmp, "Upgrade %3d%%", 100);
        DBG_PRINT("%s\r\n", str_tmp);
        DBG_PRINT("Upgrade OK\r\n");
        DBG_PRINT("Remove SD card and restart now\r\n");

        ap_display_icon_draw((INT16U *)display_buf, tft_w, tft_h, str_prefix_icon, 0);

        str_icon_1->pos_x = (tft_w - str_icon_0->icon_w * 4) >> 1;
        str_icon_1->pos_y = str_prefix_icon->pos_y + str_prefix_icon->icon_h;
        ap_display_icon_draw((INT16U *)display_buf, tft_w, tft_h, str_icon_1, 0);

        str_icon_0->pos_x = str_icon_1->pos_x + str_icon_1->icon_w;
        str_icon_0->pos_y = str_icon_1->pos_y;
        ap_display_icon_draw((INT16U *)display_buf, tft_w, tft_h, str_icon_0, 0);

        str_icon_0->pos_x += str_icon_0->icon_w;
        ap_display_icon_draw((INT16U *)display_buf, tft_w, tft_h, str_icon_0, 0);

        str_percent_icon->pos_x = str_icon_0->pos_x + str_icon_0->icon_w;
        str_percent_icon->pos_y = str_icon_0->pos_y;
        ap_display_icon_draw((INT16U *)display_buf, tft_w, tft_h, str_percent_icon, 0);
        break;

    case DRAW_FAIL:
        DBG_PRINT("Upgrade Fail\r\n");

        ap_display_icon_draw((INT16U *)display_buf, tft_w, tft_h, str_prefix_icon, 0);
        ap_display_icon_draw((INT16U *)display_buf, tft_w, tft_h, str_fail_icon, 0);
        break;
	}

    if(display_task_msg_send(MSG_DISPLAY_TASK_JPEG_DRAW, &display_frame, sizeof(DISP_P), MSG_PRI_NORMAL) == STATUS_FAIL)
    {
	    if(ap_display_queue_put(display_free_queue, display_frame) != STATUS_OK)
	    {
	        DBG_PRINT("[ERROR]display_free_queue full\r\n");
	    }
    }
}
#else
static void show_burn_state(INT32U type, INT8U val)
{
	char str_tmp[64];
	switch(type)
	{
    case DRAW_PERCENT:
        sprintf(str_tmp, "Upgrade %3d%%", val);
        DBG_PRINT("%s\r\n", str_tmp);
        break;

    case DRAW_FINISH:
        sprintf(str_tmp, "Upgrade %3d%%", 100);
        DBG_PRINT("%s\r\n", str_tmp);
        DBG_PRINT("Upgrade OK\r\n");
        DBG_PRINT("Remove SD card and restart now\r\n");
        break;

    case DRAW_FAIL:
        DBG_PRINT("Upgrade Fail\r\n");
        break;
	}
}
#endif


#if 1 // Use SPIFC
static INT32U spifc_rewrite(int idx, INT8U *frame_buf, INT8U *verify_buf)
{
	INT32U	i = 0;
	INT32U	ret = -1;
	INT32U	time = 5;

	do
	{
		for(i = 0; i < C_UPGRADE_SPI_WRITE_SIZE; ++i)
		{
			if(verify_buf[i] != 0xFF)
			{
				DBG_PRINT("0x%x page is not empty\r\n", idx);
				return -1;							// Erase 64KB，整個 PAGE 重新寫過
			}
		}

		if(SPIFC_Flash_write_page(idx, frame_buf) != 0)
		{
			DBG_PRINT("Rewrite 0x%x Error!!\r\n");
			return -1;
		}

		if(SPIFC_Flash_read_page(idx, verify_buf,256) != 0)
		{
			DBG_PRINT("Reread 0x%x Error!!\r\n");
		}

		if(gp_memcmp((INT8S*)verify_buf, (INT8S*)frame_buf, C_UPGRADE_SPI_WRITE_SIZE) == 0)
		{
			DBG_PRINT("0x%x rewrite OK\r\n", idx);
			ret = 0;
			break;
		}
	} while(--time != 0);

	return ret;
}

void ap_state_firmware_upgrade(void)
{
	struct stat_t statetest;
	INT32U	i, j = 0, total_size, complete_size;
	INT32U	*firmware_buffer, *verify_buffer;

	INT32S	verify_size;
	INT16S	fd;
	INT8U	retry = 0;

	volatile UBaseType_t	uxArraySize, x;
	TaskStatus_t 			*pxTaskStatusArray;
	TaskHandle_t 			currentTaskHandle;
	char*					currentTaskName;

	drv_l1_sys_wdt_enable_set(SYS_WDT_EN_OFF);

    #if C_TFT_DISPLAY_FUNC
	// clear all icons
	display_task_msg_send(MSG_DISPLAY_TASK_EFFECT_INIT, NULL, 0, MSG_PRI_NORMAL);
    display_task_msg_send(MSG_DISPLAY_RTC_DATE_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
    display_task_msg_send(MSG_DISPLAY_TASK_REC_TIME_CLEAR, NULL, 0, MSG_PRI_NORMAL);
    #endif

	#if STORAGE_DEVICE == FS_SD0
	fd = open("G:\\gp_cardvr_upgrade.bin", O_RDONLY);
	#else
	fd = open("C:\\gp_cardvr_upgrade.bin", O_RDONLY);
	#endif
	if(fd < 0)
	{
        #if STORAGE_DEVICE == FS_SD0
        fd = open("G:\\gp_cardvr_upgrade_no_del.bin", O_RDONLY);
        #else
		fd = open("C:\\gp_cardvr_upgrade_no_del.bin", O_RDONLY);
		#endif
        if(fd < 0)
        {
            DBG_PRINT("firmware upgrade open file fail\r\n");
            drv_l1_sys_power_handler();
            return;
        }
	}

	if(fstat(fd, &statetest))
	{
		DBG_PRINT("firmware upgrade stat file fail\r\n");
		close(fd);
		drv_l1_sys_power_handler();
		return;
	}

	total_size = statetest.st_size;
	verify_size = (INT32S) total_size;

	#if C_UPGRADE_FILE_SLICE
	firmware_buffer	= (INT32U*)gp_malloc(C_UPGRADE_BUFFER_SIZE);
	#else
	firmware_buffer	= (INT32U*)gp_malloc(total_size);
	#endif

	verify_buffer	= (INT32U*)gp_malloc(C_UPGRADE_SPI_WRITE_SIZE);
	if(!firmware_buffer || !verify_buffer)
	{
		DBG_PRINT("firmware upgrade allocate fail\r\n");
		close(fd);
		drv_l1_sys_power_handler();
		return;
	}

	#if !C_UPGRADE_FILE_SLICE
	if(read(fd, (INT32U) firmware_buffer, total_size) <= 0)
	{
		DBG_PRINT("firmware upgrade read fail\r\n");
		close(fd);
		drv_l1_sys_power_handler();
		return;
	}
	#endif

	currentTaskHandle = xTaskGetCurrentTaskHandle();
	currentTaskName = pcTaskGetTaskName(currentTaskHandle);

	uxArraySize = uxTaskGetNumberOfTasks();

	pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof( TaskStatus_t));

 	if(pxTaskStatusArray != NULL)
   	{
      /* Generate raw status information about each task. */
     	uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);

		for(x = 0 ; x < uxArraySize; x++)
		{
			// for debug
			#if 0
			DBG_PRINT("Task Name: %s\r\n",pxTaskStatusArray[x].pcTaskName);
			#endif

			if(strcmp(pxTaskStatusArray[x].pcTaskName, "Tmr Svc") == 0)
			{
				continue;
			}

			if(strcmp(pxTaskStatusArray[x].pcTaskName, "IDLE") == 0)
			{
				continue;
			}

			if(strcmp(pxTaskStatusArray[x].pcTaskName, "disp_task") == 0)   //don't kill display task
			{
				continue;
			}

			if(strcmp(pxTaskStatusArray[x].pcTaskName, currentTaskName) == 0)
			{
				continue;
			}
			else
			{
				vTaskDelete(pxTaskStatusArray[x].xHandle);
			}
		}

		vPortFree(pxTaskStatusArray);
 	}

	for(i = 0 ; i < VIC_MAX_IRQ ; ++i)
	{
		switch(i)
		{
			case SecurePhyTimer_IRQn:
			//case VIC_TIMER8:
			case VIC_UART0:
			case VIC_UART1:
			case VIC_DMA:
			case VIC_PPU:           //for display task
			case VIC_ROTATOR_LB:    //for display task
			case VIC_SD2:
			case VIC_SD3:
			#if TFT1_SPI_ENABLE || TFT2_SPI_ENABLE
			case VIC_TFT1_SPI:
			case VIC_TFT2_SPI:
			#endif
			break;
			default:
				vic_irq_disable(i);
			break;
		}
	}

	show_burn_state(DRAW_PERCENT, 0);

	// 把 SPI clock 降低，讀寫比較穩
	drv_l1_spifc_init(USE_SPIFC_SPEED);

	complete_size = 0;

	R_GPIO_CTRL |= 0x0208; // SPIFC CS pin output is generated by SPIFC controller.

	// for debug
	#if 0
	{
		INT8U ID[3];
		drv_l1_spifcflash_read_id(ID);

		// SPI ID = 0xc2 0x20 0x16
		DBG_PRINT("SPI ID = 0x%x 0x%x 0x%x\r\n",ID[0],ID[1],ID[2]);
		osDelay(100);
	}
	#endif

	while(complete_size < total_size)
	{
		INT32U	buffer_left;

		#if C_UPGRADE_FILE_SLICE
		if(read(fd, (INT32U) firmware_buffer, C_UPGRADE_BUFFER_SIZE) <= 0)
		{
			break;
		}
		#endif

		// DBG_PRINT("S:%d\r\n",block_size);
		buffer_left = (total_size - complete_size + (C_UPGRADE_SPI_BLOCK_SIZE - 1)) & ~(C_UPGRADE_SPI_BLOCK_SIZE - 1);
		if(buffer_left > C_UPGRADE_BUFFER_SIZE)
		{
			buffer_left = C_UPGRADE_BUFFER_SIZE;
		}

		retry = 0;

		// 每個 64KB 有 20 次機會
		while(buffer_left && retry < C_UPGRADE_FAIL_RETRY)
		{
			INT32U	complete_size_bak;

			complete_size &= ~(C_UPGRADE_SPI_BLOCK_SIZE - 1);
			complete_size_bak = complete_size;

			if(SPIFC_Flash_erase_block(SPIFC_RESOURCE_OFFSET + complete_size_bak))
			{
				++retry;
				continue;
			}

			for(j = C_UPGRADE_SPI_BLOCK_SIZE; j; j -= C_UPGRADE_SPI_WRITE_SIZE)
			{
				if(SPIFC_Flash_write_page(SPIFC_RESOURCE_OFFSET + complete_size_bak, (INT8U *) (firmware_buffer + ((complete_size_bak & (C_UPGRADE_BUFFER_SIZE - 1)) >> 2))))
				{
					break;
				}

				complete_size_bak += C_UPGRADE_SPI_WRITE_SIZE;
				if(complete_size_bak >= total_size)
				{
					j = 0;
					break;
				}
			}

			// verify stage
			if((j == 0) && (verify_size > 0))
			{
				complete_size_bak = complete_size;

				for(j = C_UPGRADE_SPI_BLOCK_SIZE; j; j -= C_UPGRADE_SPI_WRITE_SIZE)
				{
					INT32U	i;
					INT32U	flag = 0;

					for(i = 0; i < 5; ++i)
					{
						if((SPIFC_Flash_read_page(SPIFC_RESOURCE_OFFSET + complete_size_bak, (INT8U *) verify_buffer,256)) == 0)
                        {
							break;
                        }
					}

					flag = 0;
					if(gp_memcmp((INT8S*) verify_buffer, (INT8S*) (firmware_buffer + ((complete_size_bak & (C_UPGRADE_BUFFER_SIZE - 1)) >> 2)), C_UPGRADE_SPI_WRITE_SIZE) != 0)
					{
						DBG_PRINT("Verify 0x%x (0x%x) Error !!\r\n", complete_size_bak, C_UPGRADE_SPI_WRITE_SIZE);
						flag = 1;
					}
					//else DBG_PRINT("Verify 0x%x (0x%x) OK !!\r\n",complete_size_bak,verify_size);

					if(flag == 1)
					{
						if
						(
							spifc_rewrite
								(
									SPIFC_RESOURCE_OFFSET + complete_size_bak,
									(INT8U *) (firmware_buffer + ((complete_size_bak & (C_UPGRADE_BUFFER_SIZE - 1)) >> 2)),
									(INT8U *) verify_buffer
								) != 0
						)
						{
							break;
						}
					}

					complete_size_bak += C_UPGRADE_SPI_WRITE_SIZE;
					if(complete_size_bak >= total_size)
					{
						j = 0;
						break;
					}

					verify_size -= C_UPGRADE_SPI_WRITE_SIZE;
					if(verify_size <= 0)			// 檔案結束，沒必要再比下去
					{
						j = 0;
						break;
					}
				}
			}

			complete_size = complete_size_bak;
			if(j == 0)
			{
				buffer_left -= C_UPGRADE_SPI_BLOCK_SIZE;

				if(complete_size < total_size)
				{
					j = complete_size * 100 / total_size;
				}
				else
				{
					j = 99;							//100;
				}

				show_burn_state(DRAW_PERCENT, j);
			}
			else
			{
				++retry;
			}
		}

		if(retry >= C_UPGRADE_FAIL_RETRY)
		{
			break;
		}

		#if !C_UPGRADE_FILE_SLICE
		firmware_buffer += C_UPGRADE_BUFFER_SIZE >> 2;
		#endif
	}

	#if defined(C_SDC_UPGRADE_ERASE_USER_CONFIG) && (C_SDC_UPGRADE_ERASE_USER_CONFIG == 1)
	if(complete_size%C_UPGRADE_SPI_BLOCK_SIZE != 0)
		complete_size += C_UPGRADE_SPI_BLOCK_SIZE;
	SPIFC_Flash_erase_block(SPIFC_RESOURCE_OFFSET + complete_size);
	#endif

	osDelay(50);

	if(retry < C_UPGRADE_FAIL_RETRY)
	{
		show_burn_state(DRAW_FINISH, j);
	}
	else
	{
		show_burn_state(DRAW_FAIL, j);
	}

	#if defined(C_DEL_SDC_FIRMWARE_BIN) && (C_DEL_SDC_FIRMWARE_BIN == 1)
	if((INT32U)close < SDRAM_END_ADDR
		&& (INT32U)unlink < SDRAM_END_ADDR
		&& (INT32U)UsrFlushBuffers < SDRAM_END_ADDR
	)
	{
		DBG_PRINT("Delete gp_cardvr_upgrade.bin\r\n");
		close(fd);
		#if STORAGE_DEVICE == FS_SD0
		unlink((CHAR *) "G:\\gp_cardvr_upgrade.bin");
		#else
		unlink((CHAR *) "C:\\gp_cardvr_upgrade.bin");
		#endif
		UsrFlushBuffers(STORAGE_DEVICE);
	}
	#endif

	#if C_RTC_REBOOT
	ext_rtc_alarm_enable(NULL, 0, NULL);
	ext_rtc_wakeup_int_enable(0, NULL);
	ext_rtc_write(0x80, 0);   // clear rtc reset count
	ext_rtc_write(0x81, 0);   // clear rtc reset index
	ext_rtc_write(0x82, 0);   // clear rtc reset index
	#endif

	i = 0;
	while(1)
	{
		INT32U	status;

		if(PW_KEY_TYPE == READ_FROM_GPIO)
		{
			if(PW_KEY_ACTIVE)
				status = gpio_read_io(PW_KEY);
			else
				status = !gpio_read_io(PW_KEY);
		}
		else
		{
			status = drv_l1_sys_pwr_key_read(PW_KEY);
		}

		if(status)
			i++;
		else
			i = 0;

		if(i >= 3)
		{
			DBG_PRINT("UPGRADE POWER KEY START1\r\n");
			if(PW_KEY_TYPE == READ_FROM_GPIO)
			{
				if(PW_KEY_ACTIVE)
					while(gpio_read_io(PW_KEY));
				else
					while(!gpio_read_io(PW_KEY));
			}
			else
			{
				while(drv_l1_sys_pwr_key_read(PW_KEY));
			}
            DBG_PRINT("UPGRADE POWER KEY START2\r\n");
			drv_l1_sys_power_handler(); // turn off LDO 1.2v
		}

		osDelay(50);
	}

}
#else // Use SPI-Interface

static INT32U spifc_rewrite(int idx, INT8U *frame_buf, INT8U *verify_buf)
{
	INT32U	i = 0;
	INT32U	ret = -1;
	INT32U	time = 5;

	do
	{
		for(i = 0; i < C_UPGRADE_SPI_WRITE_SIZE; ++i)
		{
			if(verify_buf[i] != 0xFF)
			{
				DBG_PRINT("0x%x page is not empty\r\n", idx);
				return -1;							// Erase 64KB，整個 PAGE 重新寫過
			}
		}

		if(spi_write(idx, frame_buf,C_UPGRADE_SPI_WRITE_SIZE) != 0)
		{
			DBG_PRINT("Rewrite 0x%x Error!!\r\n");
			return -1;
		}

		if(spi_read(idx, verify_buf,C_UPGRADE_SPI_WRITE_SIZE) != 0)
		{
			DBG_PRINT("Reread 0x%x Error!!\r\n");
		}

		if(gp_memcmp((INT8S*)verify_buf, (INT8S*)frame_buf, C_UPGRADE_SPI_WRITE_SIZE) == 0)
		{
			DBG_PRINT("0x%x rewrite OK\r\n", idx);
			ret = 0;
			break;
		}
	} while(--time != 0);

	return ret;
}

INT32S spi_earse(INT32U spi_addr, INT32S size)
{
	INT32S ret;

	while(size > 0) {
		DBG_PRINT("Earse = 0x%x\r\n", spi_addr);
		ret = drv_l2_spiflash_sector_erase(spi_addr);
		if(ret < 0) {
			DBG_PRINT("earse Fail\r\n");
			goto __exit;
		}

		spi_addr += (1 << 12); //4K
		size -= (1 << 12);
	}

__exit:
	return ret;
}

INT32S spi_write(INT32U spi_addr, INT8U *buf, INT32S size)
{
	INT32S ret;

	while(size > 0) {
		DBG_PRINT("Write = 0x%x\r\n", spi_addr);
#if 1 || (SPI_MODE ==  MODE_1BIT) || (SPI_MODE ==  MODE_2BIT)
		ret = drv_l2_spiflash_write_page(IO_1BIT, spi_addr, buf);
#else
		ret = drv_l2_spiflash_write_page(IO_4BIT, spi_addr, buf);
#endif
		if(ret < 0) {
			DBG_PRINT("read Fail\r\n");
			goto __exit;
		}

		spi_addr += (1 << 8);
		buf += (1 << 8);
		size -= (1 << 8);
	}

__exit:
	return ret;
}

INT32S spi_read(INT32U spi_addr, INT8U *buf, INT32S size)
{
	INT32S ret;

	while(size > 0) {
		DBG_PRINT("Read = 0x%x\r\n", spi_addr);
#if 1 || (SPI_MODE ==  MODE_1BIT)
		ret = drv_l2_spiflash_read_page(IO_1BIT,spi_addr, buf);
#elif (SPI_MODE ==  MODE_2BIT)
		ret = drv_l2_spiflash_read_page(IO_2BIT,spi_addr, buf);
#else
        ret = drv_l2_spiflash_read_page(IO_4BIT,spi_addr, buf);
#endif
		if(ret < 0) {
			DBG_PRINT("read Fail\r\n");
			goto __exit;
		}

		spi_addr += (1 << 8);
		buf += (1 << 8);
		size -= (1 << 8);
	}

__exit:
	return ret;
}

void ap_state_firmware_upgrade(void)
{
	struct stat_t statetest;
	INT32U	i, j, total_size, complete_size;
	INT32U	*firmware_buffer, *verify_buffer;

	INT32S	verify_size;
	INT16S	fd;
	INT8U	retry;

	volatile UBaseType_t	uxArraySize,x;
	TaskStatus_t 			*pxTaskStatusArray;
	TaskHandle_t 			currentTaskHandle;
	char*					currentTaskName;

	drv_l1_sys_wdt_enable_set(SYS_WDT_EN_OFF);

	fd = open("C:\\gp_cardvr_upgrade.bin", O_RDONLY);
	if(fd < 0)
	{
		DBG_PRINT("firmware upgrade open file fail\r\n");
		drv_l1_sys_power_handler();
		return;
	}

	if(fstat(fd, &statetest))
	{
		DBG_PRINT("firmware upgrade stat file fail\r\n");
		close(fd);
		drv_l1_sys_power_handler();
		return;
	}

	total_size = statetest.st_size;
	verify_size = (INT32S) total_size;

	#if C_UPGRADE_FILE_SLICE
	firmware_buffer	= (INT32U*)gp_malloc(C_UPGRADE_BUFFER_SIZE);
	#else
	firmware_buffer	= (INT32U*)gp_malloc(total_size);
	#endif

	verify_buffer	= (INT32U*)gp_malloc(C_UPGRADE_SPI_WRITE_SIZE);
	if(!firmware_buffer || !verify_buffer)
	{
		DBG_PRINT("firmware upgrade allocate fail\r\n");
		close(fd);
		drv_l1_sys_power_handler();
		return;
	}

	#if !C_UPGRADE_FILE_SLICE
	if(read(fd, (INT32U) firmware_buffer, total_size) <= 0)
	{
		DBG_PRINT("firmware upgrade read fail\r\n");
		close(fd);
		drv_l1_sys_power_handler();
		return;
	}
	#endif


	currentTaskHandle = xTaskGetCurrentTaskHandle();
	currentTaskName = pcTaskGetTaskName(currentTaskHandle);

	uxArraySize = uxTaskGetNumberOfTasks();

	pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof( TaskStatus_t));


 	if(pxTaskStatusArray != NULL)
   	{
      /* Generate raw status information about each task. */
     	uxArraySize = uxTaskGetSystemState(pxTaskStatusArray,uxArraySize,NULL);

		for(x = 0 ; x < uxArraySize; x++)
		{
			// for debug
			#if 0
			DBG_PRINT("Task Name: %s\r\n",pxTaskStatusArray[x].pcTaskName);
			#endif

			if(strcmp(pxTaskStatusArray[x].pcTaskName,"Tmr Svc") == 0)
			{
				continue;
			}

			if(strcmp(pxTaskStatusArray[x].pcTaskName,"IDLE") == 0)
			{
				continue;
			}

			if(strcmp(pxTaskStatusArray[x].pcTaskName, "disp_task") == 0)   //don't kill display task
			{
				continue;
			}

			if(strcmp(pxTaskStatusArray[x].pcTaskName,currentTaskName) == 0)
			{
				continue;
			}
			else
			{
				vTaskDelete(pxTaskStatusArray[x].xHandle);
			}
		}

	 vPortFree(pxTaskStatusArray);

 	}

	for(i = 0 ; i < VIC_MAX_IRQ ; ++i)
	{
		switch(i)
		{
			case SecurePhyTimer_IRQn:
			//case VIC_TIMER8:
			case VIC_UART0:
			case VIC_UART1:
			case VIC_DMA:
			case VIC_PPU:           //for display task
			case VIC_ROTATOR_LB:    //for display task
				break;
			default:
				vic_irq_disable(i);
			break;
		}
	}

	show_burn_state(DRAW_PERCENT, 0);

	// 把 SPI clock 降低，讀寫比較穩
	drv_l1_spifc_uninit();
	drv_l2_spiflash_init(12000000);

	complete_size = 0;


	// for debug
	#if 0
	{
		INT8U ID[3];
		drv_l2_spiflash_read_id(ID);

		// SPI ID = 0xc2 0x20 0x16
		DBG_PRINT("SPI ID = 0x%x 0x%x 0x%x\r\n",ID[0],ID[1],ID[2]);
		osDelay(100);
	}
	#endif

	while(complete_size < total_size)
	{
		INT32U	buffer_left;


		#if C_UPGRADE_FILE_SLICE
		if(read(fd, (INT32U) firmware_buffer, C_UPGRADE_BUFFER_SIZE) <= 0)
		{
			break;
		}
		#endif

		buffer_left = (total_size - complete_size + (C_UPGRADE_SPI_BLOCK_SIZE - 1)) &~(C_UPGRADE_SPI_BLOCK_SIZE - 1);
		if(buffer_left > C_UPGRADE_BUFFER_SIZE)
		{
			buffer_left = C_UPGRADE_BUFFER_SIZE;
		}

		retry = 0;

		// 每個 64KB 有 20 次機會
		while(buffer_left && retry < C_UPGRADE_FAIL_RETRY)
		{
			INT32U	complete_size_bak;

			complete_size &= ~(C_UPGRADE_SPI_BLOCK_SIZE - 1);
			complete_size_bak = complete_size;


			if(spi_earse((SPIFC_RESOURCE_OFFSET + complete_size_bak),C_UPGRADE_SPI_BLOCK_SIZE))
			{
				++retry;
				continue;
			}

			for(j = C_UPGRADE_SPI_BLOCK_SIZE; j; j -= C_UPGRADE_SPI_WRITE_SIZE)
			{
				if(spi_write(SPIFC_RESOURCE_OFFSET + complete_size_bak, (INT8U *) (firmware_buffer + ((complete_size_bak & (C_UPGRADE_BUFFER_SIZE - 1)) >> 2)),C_UPGRADE_SPI_WRITE_SIZE))
				{
					break;
				}

				complete_size_bak += C_UPGRADE_SPI_WRITE_SIZE;
				if(complete_size_bak >= total_size)
				{
					j = 0;
					break;
				}
			}

			// verify stage
			if((j == 0) && (verify_size > 0))
			{
				complete_size_bak = complete_size;

				for(j = C_UPGRADE_SPI_BLOCK_SIZE; j; j -= C_UPGRADE_SPI_WRITE_SIZE)
				{
					INT32U	i;
					INT32U	flag = 0;

					for(i = 0; i < 5; ++i)
					{
						if((spi_read(SPIFC_RESOURCE_OFFSET + complete_size_bak, (INT8U *) verify_buffer,C_UPGRADE_SPI_WRITE_SIZE)) == 0)
							break;
					}

					flag = 0;
					if(gp_memcmp((INT8S*) verify_buffer, (INT8S*) (firmware_buffer + ((complete_size_bak & (C_UPGRADE_BUFFER_SIZE - 1)) >> 2)), C_UPGRADE_SPI_WRITE_SIZE) != 0)
					{
						DBG_PRINT("Verify 0x%x (0x%x) Error !!\r\n", complete_size_bak, C_UPGRADE_SPI_WRITE_SIZE);
						flag = 1;
					}
					//else DBG_PRINT("Verify 0x%x (0x%x) OK !!\r\n",complete_size_bak,verify_size);

					if(flag == 1)
					{
						if
						(
							spifc_rewrite
								(
									SPIFC_RESOURCE_OFFSET + complete_size_bak,
									(INT8U *) (firmware_buffer + ((complete_size_bak & (C_UPGRADE_BUFFER_SIZE - 1)) >> 2)),
									(INT8U *) verify_buffer
								) != 0
						)
						{
							break;
						}
					}

					complete_size_bak += C_UPGRADE_SPI_WRITE_SIZE;
					if(complete_size_bak >= total_size)
					{
						j = 0;
						break;
					}

					verify_size -= C_UPGRADE_SPI_WRITE_SIZE;
					if(verify_size <= 0)			// 檔案結束，沒必要再比下去
					{
						j = 0;
						break;
					}
				}
			}

			complete_size = complete_size_bak;
			if(j == 0)
			{
				buffer_left -= C_UPGRADE_SPI_BLOCK_SIZE;

				if(complete_size < total_size)
				{
					j = complete_size * 100 / total_size;
				}
				else
				{
					j = 99;							//100;
				}

				show_burn_state(DRAW_PERCENT, j);
			}
			else
			{
				++retry;
			}
		}

		if(retry >= C_UPGRADE_FAIL_RETRY)
		{
			break;
		}

		#if !C_UPGRADE_FILE_SLICE
		firmware_buffer += C_UPGRADE_BUFFER_SIZE >> 2;
		#endif
	}

	#if defined(C_SDC_UPGRADE_ERASE_USER_CONFIG) && (C_SDC_UPGRADE_ERASE_USER_CONFIG == 1)
	spi_earse(SPIFC_RESOURCE_OFFSET + complete_size + C_UPGRADE_SPI_BLOCK_SIZE,C_UPGRADE_SPI_BLOCK_SIZE);
	#endif

	osDelay(50);

	if(retry < C_UPGRADE_FAIL_RETRY)
	{
		show_burn_state(DRAW_FINISH, j);
	}
	else
	{
		show_burn_state(DRAW_FAIL, j);
	}

	#if defined(C_DEL_SDC_FIRMWARE_BIN) && (C_DEL_SDC_FIRMWARE_BIN == 1)
	if((INT32U)close < SDRAM_END_ADDR
		&& (INT32U)unlink < SDRAM_END_ADDR
		&& (INT32U)UsrFlushBuffers < SDRAM_END_ADDR
	)
	{
		DBG_PRINT("Delete C:\\gp_cardvr_upgrade.bin\r\n");
		close(fd);
		unlink((CHAR *) "C:\\gp_cardvr_upgrade.bin");
		UsrFlushBuffers(STORAGE_DEVICE);
	}
	#endif

	#if C_RTC_REBOOT
	ext_rtc_alarm_enable_a(NULL, 0, NULL);
	ext_rtc_wakeup_int_enable_a(0, NULL);
	ext_rtc_write(0x80, 0);   // clear rtc reset count
	ext_rtc_write(0x81, 0);   // clear rtc reset index
	ext_rtc_write(0x82, 0);   // clear rtc reset index
	gPower_Off_Set_Alarm = 1;
	#endif

	i = 0;
	while(1)
	{
		INT32U	status;

		if(PW_KEY_TYPE == READ_FROM_GPIO)
		{
			if(PW_KEY_ACTIVE)
				status = gpio_read_io(PW_KEY);
			else
				status = !gpio_read_io(PW_KEY);
		}
		else
		{
			status = drv_l1_sys_pwr_key_read(PW_KEY);
		}

		if(status)
			i++;
		else
			i = 0;

		if(i >= 3)
		{
			if(PW_KEY_TYPE == READ_FROM_GPIO)
			{
				if(PW_KEY_ACTIVE)
					while(gpio_read_io(PW_KEY));
				else
					while(!gpio_read_io(PW_KEY));
			}
			else
			{
				while(drv_l1_sys_pwr_key_read(PW_KEY));
			}

			drv_l1_sys_power_handler(); // turn off LDO 1.2v
		}

		osDelay(50);
	}

}

#endif


INT8U ap_state_firmware_upgrade_check(void)
{
	INT16S fd;

	#if FW_UPDATE_TIME_LIMIT
	if(xTaskGetTickCount() > (FW_UPDATE_TIME_LIMIT * 1000))
	{
		return 0;
	}
	#endif

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\");
	fd = open((CHAR *) "G:\\gp_cardvr_upgrade.bin", O_RDONLY);
	#else
	chdir("C:\\");
	fd = open((CHAR *) "C:\\gp_cardvr_upgrade.bin", O_RDONLY);
	#endif
	if(fd > -1)
	{
		close(fd);
		return 1;
	}
	#if STORAGE_DEVICE == FS_SD0
	fd = open((CHAR *) "G:\\gp_cardvr_upgrade_no_del.bin", O_RDONLY);
	#else
	fd = open((CHAR *) "C:\\gp_cardvr_upgrade_no_del.bin", O_RDONLY);
	#endif
	if(fd > -1)
	{
		close(fd);
		return 1;
	}
	return 0;
}

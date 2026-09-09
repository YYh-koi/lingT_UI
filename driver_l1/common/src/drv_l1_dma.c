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
#define K_DMA_USE_CMSIS_OS      1

#include "drv_l1.h"

#define K_DMA_TIMEOUT_PATCH_APPLY			1 // patch that fix timeout then next dma may have garbage bytes

#define K_SD_DMA_TEST_CODE					0 // must off when release code

#define DMA_CLOCK_DISABLE                   0
#define DMA_CLOCK_ENABLE                    1

extern void drv_usec_wait(INT32U u_sec);

#if (K_SD_DMA_TEST_CODE != 0)
static volatile int sd_dma_test = 0;

void dma_test_set(INT32U value)
{
    sd_dma_test = value;
}
#endif

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static INT32U dma_get_channel(INT8U usage);
///static INT32S dma_set_channel(INT8U usage, INT8U channel);
static void dma_free_channel(INT32U channel);

static INT32U dma_get_queue(void);
static void dma_free_queue(INT32U q_index);

void dma_set_notify(INT32U channel, INT32U notify, INT32U os_q);
static INT32S dma_device_protect(void);
static void dma_device_unprotect(INT32S mask);

static void dma_set_control(INT32U channel, INT32U ctrl);
static void dma_set_source(INT32U channel, INT32U addr);
static void dma_set_target(INT32U channel, INT32U addr);
static INT32S dma_set_tx_count(INT32U channel, INT32U count);
static INT32S dma_set_device(INT32U channel, INT16U device);
static INT32S dma_set_timeout(INT32U channel, INT32U timeout);
static INT32S dma_set_line_length(INT32U length);
static INT32S dma_set_sprite_size(INT32U channel, INT32U size);

#ifdef DMA_TRANSPANRENT
static INT32S dma_set_transparent_enable(INT32U channel);
static INT32S dma_set_transparent_disable(INT32U channel);
static INT32S dma_set_transparent_pattern(INT32U channel, INT16U pattern);
#endif

static INT32S dma_transfer_extend(DMA_STRUCT *dma_struct, INT8U usage, INT32U os_q, void (*dma_user_isr)(INT8U, INT8U));

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT8U dma_init_done = FALSE;
static volatile INT8U dma_usage[C_DMA_CHANNEL_NUM];
static volatile INT8U dma_clock[C_DMA_CHANNEL_NUM];
static volatile INT32U dma_notify_variable[C_DMA_CHANNEL_NUM];
#if (K_DMA_TIMEOUT_PATCH_APPLY == 1)
static volatile INT8S dma_previous_done_state[C_DMA_CHANNEL_NUM] = {0};
#endif
static volatile INT32U dma_checksum[C_DMA_CHANNEL_NUM];
INT32U *check_sum[C_DMA_CHANNEL_NUM] = {NULL};

#if (K_DMA_USE_CMSIS_OS == 1)
static volatile osMessageQId dma_notify_queue[C_DMA_CHANNEL_NUM];
static osMessageQId dma_driver_queue[C_DMA_Q_NUM] = {NULL};
#else
static volatile xQueueHandle dma_notify_queue[C_DMA_CHANNEL_NUM];
static xQueueHandle dma_driver_queue[C_DMA_Q_NUM] = {NULL};
#endif
static INT8U dma_q_usage[C_DMA_Q_NUM];
static INT8U dma_is_sd[C_DMA_CHANNEL_NUM];

/**
 * @brief   set dma2/3 clock
 * @param   0: clock disable/ 1: clock enable
 * @return 	none
 * @note
 */
static void dma23_clock_set(INT8U enable)
{
    INT8U i;

    for(i=2;i<4;i++)
        dma_clock[i] = enable;

    drv_l1_clock_set_system_clk_en(CLK_EN0_DMA_CH2_CH3,enable);
}

/**
 * @brief   set dma clock (ch0/1/4/5/6/7)
 * @param   0: clock disable/ 1: clock enable
 * @return 	none
 */
static void dma_clock_set(INT8U enable)
{
    INT8U i;
    for(i=0;i<2;i++)
        dma_clock[i] = enable;
    drv_l1_clock_set_system_clk_en(CLK_EN0_DMA_CH0_CH1,enable);

    dma23_clock_set(enable);

    for(i=4;i<8;i++)
        dma_clock[i] = enable;
    drv_l1_clock_set_system_clk_en(CLK_EN0_DMA_CH4_CH5,enable);
    drv_l1_clock_set_system_clk_en(CLK_EN0_DMA_CH6_CH7,enable);
}

/**
 * @brief   get dma idle channel
 * @param   usage: use method
 * @return 	channel
 */
static INT32U dma_get_channel(INT8U usage)
{
	INT32U i;

	for (i=0; i<C_DMA_CHANNEL_NUM; i++) {
        taskENTER_CRITICAL();
		if (dma_usage[i] == C_DMA_NOT_UESED) {
			dma_usage[i] = usage;

            taskEXIT_CRITICAL();

			// Reset DMA controller
			dma_set_control(i, C_DMA_CTRL_RESET);
			return i;
		}
		taskEXIT_CRITICAL();
	}

	return i;
}

/**
 * @brief   get dma assign channel
 * @param   usage: use method
 * @param   channel: assign channel
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S dma_set_channel(INT8U usage, INT8U channel)
{
	INT32S ret;

	if(channel >= C_DMA_CHANNEL_NUM) {
		return STATUS_FAIL;
	}

    #if _OPERATING_SYSTEM != _OS_NONE				// Soft Protect for critical section
        #if _OPERATING_SYSTEM == _OS_UCOS2
        OSSchedLock();
        #elif _OPERATING_SYSTEM == _OS_FREERTOS
        taskENTER_CRITICAL();
        #endif
    #endif

	if (dma_usage[channel] == C_DMA_NOT_UESED) {
		dma_usage[channel] = usage;
		ret = STATUS_OK;
	} else {
		ret = STATUS_FAIL;
	}

    #if _OPERATING_SYSTEM != _OS_NONE
        #if _OPERATING_SYSTEM == _OS_UCOS2
        OSSchedUnlock();
        #elif _OPERATING_SYSTEM == _OS_FREERTOS
        taskEXIT_CRITICAL();
        #endif
    #endif

    if (ret == STATUS_OK) {
        dma_set_control(channel, C_DMA_CTRL_RESET); 	// Reset DMA controller
    }

	return ret;
}

static void (*dma_user_isr[C_DMA_CHANNEL_NUM])(INT8U, INT8U) = {NULL};

/**
 * @brief   dma isr callback setupt
 * @param   channel: assign channel
 * @param   user_isr: registered isr callback
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S drv_l1_dma_callback_set(INT32U channel, void (*user_isr)(INT8U, INT8U))
{
    INT32S ret = STATUS_OK;
    if(channel >7)
        ret = STATUS_FAIL;

    dma_user_isr[channel] = user_isr;

    return ret;
}

/**
 * @brief   dma isr callback clear
 * @param   channel: assign channel
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S drv_l1_dma_callback_clear(INT32U channel)
{
    INT32S ret = STATUS_OK;
    if(channel >7)
        ret = STATUS_FAIL;

    dma_user_isr[channel] = 0;

    return ret;
}

/**
 * @brief   free used dma channel
 * @param   channel: assign channel
 * @return none
 */
void drv_l1_dma_free_channel(INT32U channel)
{
	dma_free_channel(channel);
	dma_set_target(channel, 0);
	dma_set_source(channel, 0);
}

/**
 * @brief   get dma channel target address
 * @param   channel: assign channel
 * @return  target address
 */
INT32U drv_l1_dma_get_channel_target_address(INT32U channel)
{
	INT32U addr = 0;

	// Clear pending bit
	if (channel == C_DMA_CH0) {
		addr = R_DMA0_TAR_ADDR;
	} else if (channel == C_DMA_CH1) {
		addr = R_DMA1_TAR_ADDR;
	} else if (channel == C_DMA_CH2) {
		addr = R_DMA2_TAR_ADDR;
	} else if (channel == C_DMA_CH3) {
		addr = R_DMA3_TAR_ADDR;
	} else if (channel == C_DMA_CH4) {
		addr = R_DMA4_TAR_ADDR;
	} else if (channel == C_DMA_CH5) {
		addr = R_DMA5_TAR_ADDR;
	} else if (channel == C_DMA_CH6) {
		addr = R_DMA6_TAR_ADDR;
	} else if (channel == C_DMA_CH7) {
		addr = R_DMA7_TAR_ADDR;
	}

	return addr;
}

/**
 * @brief   get dma timeout flag
 * @param   channel: assign channel
 * @return  timeout flag
 */
static INT32U dma_get_timeout_flag(INT32U channel)
{
	INT32U flag = 0;

	// Clear pending bit
	if (channel == C_DMA_CH0) {
		flag = C_DMA0_TIMEOUT;
	} else if (channel == C_DMA_CH1) {
		flag = C_DMA1_TIMEOUT;
	} else if (channel == C_DMA_CH2) {
		flag = C_DMA2_TIMEOUT;
	} else if (channel == C_DMA_CH3) {
		flag = C_DMA3_TIMEOUT;
	} else if (channel == C_DMA_CH4) {
		flag = C_DMA4_TIMEOUT;
	} else if (channel == C_DMA_CH5) {
		flag = C_DMA5_TIMEOUT;
	} else if (channel == C_DMA_CH6) {
		flag = C_DMA6_TIMEOUT;
	} else if (channel == C_DMA_CH7) {
		flag = C_DMA7_TIMEOUT;
	}
	return flag;
}

/**
 * @brief   get dma interrupt flag
 * @param   channel: assign channel
 * @return  interrupt flag
 */
static INT32U dma_get_int_flag(INT32U channel)
{
	INT32U flag = 0;

	// Clear pending bit
	if (channel == C_DMA_CH0) {
		flag = C_DMA0_INT_PEND;
	} else if (channel == C_DMA_CH1) {
		flag = C_DMA1_INT_PEND;
	} else if (channel == C_DMA_CH2) {
		flag = C_DMA2_INT_PEND;
	} else if (channel == C_DMA_CH3) {
		flag = C_DMA3_INT_PEND;
	} else if (channel == C_DMA_CH4) {
		flag = C_DMA4_INT_PEND;
	} else if (channel == C_DMA_CH5) {
		flag = C_DMA5_INT_PEND;
	} else if (channel == C_DMA_CH6) {
		flag = C_DMA6_INT_PEND;
	} else if (channel == C_DMA_CH7) {
		flag = C_DMA7_INT_PEND;
	}
	return flag;
}

/**
 * @brief   reset dma channel
 * @param   channel: assign channel
 * @return  none
 */
static void dma_reset_channel(INT32U channel)
{
	if (channel >= C_DMA_CHANNEL_NUM) {
		return;
	}

	R_DMA_INT = dma_get_int_flag(channel) | dma_get_timeout_flag(channel);

	// Reset DMA controller
	dma_set_control(channel, C_DMA_CTRL_RESET);
}

/**
 * @brief   free dma unsued channel
 * @param   channel: channel
 * @return 	none
 */
static void dma_free_channel(INT32U channel)
{
	if (channel >= C_DMA_CHANNEL_NUM) {
		return;
	}

	dma_reset_channel(channel);

	dma_user_isr[channel] = 0;
	dma_notify_variable[channel] = (INT32U) NULL;
#if (K_DMA_USE_CMSIS_OS == 1)
  	dma_notify_queue[channel] = (osMessageQId) NULL;
#else
        dma_notify_queue[channel] = (xQueueHandle) NULL;
#endif
	dma_usage[channel] = (INT8U) C_DMA_NOT_UESED;
}

/**
 * @brief   get and set dma channel queue
 * @param   none
 * @return 	queue number
 */
static INT32U dma_get_queue(void)
{
	INT32U i;

	for (i=0; i<C_DMA_Q_NUM; i++) {
        taskENTER_CRITICAL();
		if (!dma_q_usage[i]) {
			dma_q_usage[i] = (INT8U) TRUE;
            taskEXIT_CRITICAL();
            break;
		}
		taskEXIT_CRITICAL();
	}

	if (i < C_DMA_Q_NUM)
    {
        if (dma_driver_queue[i]) {
            OSQFlush(dma_driver_queue[i]);
        } else {
#if (K_DMA_USE_CMSIS_OS == 1)
            osMessageQDef_t dma_q = {C_DMA_Q_BUF_SIZE, sizeof(INT32U), 0};

            dma_driver_queue[i] = osMessageCreate(&dma_q, NULL);
#else
            dma_driver_queue[i] = xQueueCreate(C_DMA_Q_BUF_SIZE, sizeof(INT32U));
#endif
        }
    }

	return i;
}

/**
 * @brief   free dma channel queue
 * @param   q_index: queue number
 * @return 	none
 */
static void dma_free_queue(INT32U q_index)
{
	if (q_index >= C_DMA_Q_NUM) {
		return;
	}

	dma_q_usage[q_index] = (INT8U) FALSE;
}

/**
 * @brief   free all dma driver queue
 * @param   none
 * @return 	none
 */
void dma_free_driver_queue(void)
{
	INT32U i;

	for (i=0; i<C_DMA_Q_NUM; i++) {
        if(dma_driver_queue[i]){
            xQueueReset(dma_driver_queue[i]);
            vQueueDelete(dma_driver_queue[i]);
            dma_driver_queue[i] = 0;
        }
	}
}

/**
 * @brief   set dma done notify
 * @param   channel: channel number
 * @param   notify: notify point
 * @param   os_q: notify queue
 * @return 	none
 */
void dma_set_notify(INT32U channel, INT32U notify, INT32U os_q)
{
	dma_notify_variable[channel] = notify;
#if (K_DMA_USE_CMSIS_OS == 1)
  	dma_notify_queue[channel] = (osMessageQId)os_q;
#else
        dma_notify_queue[channel] = (xQueueHandle)os_q;
#endif

}

/**
 * @brief   disable dma irq
 * @param   none
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S dma_device_protect(void)
{
    vic_irq_disable(VIC_DMA);
#if FIQ
    vic_fiq_disable(VIC_DMA);
#endif
	return 0;
}

/**
 * @brief   enable/disable dma irq
 * @param   mask: mask value
 * @return 	none
 */
static void dma_device_unprotect(INT32S mask)
{
    vic_irq_enable(VIC_DMA);
#if FIQ
    vic_fiq_enable(VIC_DMA);
#endif
}
/**
 * @brief   get dma checksum
 * @param   channel: assign channel
 * @return  checksum
 */
INT32U dma_checksum_get(INT32S channel)
{
	INT32U checksum=0;

	if (channel == C_DMA_CH0) {
		checksum = R_DMA0_CHECKSUM;
	} else if (channel == C_DMA_CH1) {
		checksum = R_DMA1_CHECKSUM;
	} else if (channel == C_DMA_CH2) {
		checksum = R_DMA2_CHECKSUM;
	} else if (channel == C_DMA_CH3) {
		checksum = R_DMA3_CHECKSUM;
	} else if (channel == C_DMA_CH4) {
		checksum = R_DMA4_CHECKSUM;
	} else if (channel == C_DMA_CH5) {
		checksum = R_DMA5_CHECKSUM;
	} else if (channel == C_DMA_CH6) {
		checksum = R_DMA6_CHECKSUM;
	} else if (channel == C_DMA_CH7) {
		checksum = R_DMA7_CHECKSUM;
	}

	return checksum;

}
/**
 * @brief   user reset dma timeout flag
 * @param   channel: assign channel
 * @return  none
 */
void user_dma_reset_channel(INT32S channel)
{
    dma_reset_channel(channel);
}

static void handle_dma_done(INT32S channel)
{
	#if (K_DMA_TIMEOUT_PATCH_APPLY == 1)
	dma_previous_done_state[channel] = C_DMA_STATUS_DONE;
	#endif
    dma_checksum[channel] = dma_checksum_get(channel);
    if(check_sum[channel]) {
        *check_sum[channel] = dma_checksum[channel];
    }

	if (dma_user_isr[channel])
	{
        (*dma_user_isr[channel])((INT8U)channel, C_DMA_STATUS_DONE);
	}

	if (dma_notify_variable[channel]) {
		*((INT8S *) dma_notify_variable[channel]) = (INT8S) C_DMA_STATUS_DONE;
	}

	if (dma_notify_queue[channel]) {
        INT32U msg = C_DMA_STATUS_DONE;
#if (K_DMA_USE_CMSIS_OS == 1)
		osMessagePut(dma_notify_queue[channel], (uint32_t)&msg, osWaitForever);
#else
          xQueueSendFromISR(dma_notify_queue[channel], (const void *)&msg, NULL);
#endif
	}
}

#if (K_DMA_TIMEOUT_PATCH_APPLY == 1)

#define K_DMA_DBG_TIMEOUT_DUMMY_BYTE	0 // Just for DEBUG, must off when release code

// A dummy copy 64 bytes memory to memory to patch dma when dma abnormal complete
// it should be called when dma timeout by hardware or software.
// If the original dma involve IO device, should first stop the IO device in registered
// timeout isr, then call this patch function.
// If didnt doing this way, it will have chance that the next dma will fill with 16 garbage
// bytes. This patch working by just doing a dummy copy so that the garbage byte be flushed.
static ALIGN64 INT32U dma_dummy_src_buf[16];
static ALIGN64 INT32U dma_dummy_dst_buf[16];

void dma_patch_dummy_copy(INT32U channel)
{
#if (K_DMA_DBG_TIMEOUT_DUMMY_BYTE == 1)
INT32U len;
static INT8U dma_dummy_src_inited = 0;
#endif
	INT32U ctrl;
	//INT16U src_type, target_type;
	INT32U count = 16;
	INT32U s_addr = (INT32U)(&dma_dummy_src_buf[0]);
	INT32U t_addr = (INT32U)(&dma_dummy_dst_buf[0]);

	ctrl = C_DMA_CTRL_32BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
#if (K_DMA_DBG_TIMEOUT_DUMMY_BYTE == 1)
	len = count << 2;
#endif
	//src_type = C_DMA_MEMORY;
	//target_type = C_DMA_MEMORY;
	dma_set_source(channel, s_addr);
	dma_set_target(channel, t_addr);
	dma_set_tx_count(channel, count);
	dma_set_timeout(channel, 16);
	dma_set_sprite_size(channel, 0);
	ctrl |= C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_SRC_INCREASE | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_SOFTWARE;
	#if (K_DMA_DBG_TIMEOUT_DUMMY_BYTE == 1)
	if (!dma_dummy_src_inited)
	{
		INT8U *pbyte = (INT8U *)(&dma_dummy_src_buf[0]);
		INT8U k;

		for (k=0; k< 64; k++)
			pbyte[k] = k;
		// Drain source memory and invalid target memory from cache
		drv_l1_cp15_clean_dcache_range(s_addr, len); //cache_drain_range(s_addr, len);
		drv_l1_cp15_clean_dcache_range(t_addr, len);
		drv_l1_cp15_invalidate_dcache_range(t_addr, len); //cache_invalid_range(t_addr, len);

		dma_dummy_src_inited = 1;
	}
	#endif
	ctrl |= C_DMA_CTRL_BURST8_ACCESS;
	dma_set_control(channel, ctrl);
}

static void handle_dma_timout_first_round(INT32S channel)
{
	dma_previous_done_state[channel] = C_DMA_STATUS_TIMEOUT;
	if (dma_user_isr[channel])
	{
        (*dma_user_isr[channel])((INT8U)channel, C_DMA_STATUS_TIMEOUT);
	}
	dma_reset_channel(channel);
	dma_patch_dummy_copy(channel);
}

static void handle_dma_timout_second_round(INT32S channel)
{
	dma_previous_done_state[channel] = C_DMA_STATUS_DONE;

	if (dma_notify_variable[channel]) {
		*((INT8S *) dma_notify_variable[channel]) = (INT8S) C_DMA_STATUS_TIMEOUT;
	}

	if (dma_notify_queue[channel]) {
        INT32U msg = C_DMA_STATUS_TIMEOUT;
#if (K_DMA_USE_CMSIS_OS == 1)
		osMessagePut(dma_notify_queue[channel], (uint32_t)&msg, osWaitForever);
#else
          //xQueueSend(dma_notify_queue[channel], &msg, portMAX_DELAY);
          xQueueSendFromISR(dma_notify_queue[channel], (const void *)&msg, NULL);
#endif
	}

	#if (K_DMA_DBG_TIMEOUT_DUMMY_BYTE == 1)
	{
		INT8U *pbyte = (INT8U *)(&dma_dummy_dst_buf[0]);
		//INT8U match = 1;
        INT8U k;

		for (k=0; k< 64; k++)
		{
			if (pbyte[k] != k)
			{
				//match = 0;
				DBG_PRINT("\r\ndma dummy buffer become dirty! k[%u]=0x%02x\r\n", k, pbyte[k]);
				//break;
			}
		}
		drv_l1_cp15_clean_dcache_range((INT32U)pbyte, 64);
		drv_l1_cp15_invalidate_dcache_range((INT32U)pbyte, 64); // cache_invalid_range((INT32U)pbyte, 64);
	}
	#endif
}

static void dma_irq_handler(INT32U pending)							// Device ISR
{
	// DMA0
	if (pending & C_DMA0_INT_PEND) {		// DMA0 interrupt is pending
		if (dma_usage[C_DMA_CH0] != C_DMA_NOT_UESED) {
			R_DMA_INT = C_DMA0_INT_PEND;		// Clear pending bit
			if (pending & C_DMA0_TIMEOUT) {
				R_DMA_INT = C_DMA0_TIMEOUT;		// Clear timeout bit
				handle_dma_timout_first_round(C_DMA_CH0);
			} else {
				if (dma_previous_done_state[C_DMA_CH0] == C_DMA_STATUS_TIMEOUT)
					handle_dma_timout_second_round(C_DMA_CH0);
				else
					handle_dma_done(C_DMA_CH0);
				if (dma_usage[C_DMA_CH0] != C_DMA_OCCUPIED) {
					dma_free_channel(C_DMA_CH0);
				}
			}
		}
	}

	// DMA1
	if (pending & C_DMA1_INT_PEND) {		// DMA1 interrupt is pending
		if (dma_usage[C_DMA_CH1] != C_DMA_NOT_UESED) {
			R_DMA_INT = C_DMA1_INT_PEND;		// Clear pending bit
			if (pending & C_DMA1_TIMEOUT) {
				R_DMA_INT = C_DMA1_TIMEOUT;		// Clear timeout bit
				handle_dma_timout_first_round(C_DMA_CH1);
			} else {
				if (dma_previous_done_state[C_DMA_CH1] == C_DMA_STATUS_TIMEOUT)
					handle_dma_timout_second_round(C_DMA_CH1);
				else
					handle_dma_done(C_DMA_CH1);

				if (dma_usage[C_DMA_CH1] != C_DMA_OCCUPIED) {
					dma_free_channel(C_DMA_CH1);
				}
			}
		}
	}

	// DMA2
	if (pending & C_DMA2_INT_PEND) {		// DMA2 interrupt is pending
		if (dma_usage[C_DMA_CH2] != C_DMA_NOT_UESED) {
			R_DMA_INT = C_DMA2_INT_PEND;		// Clear pending bit
			if (pending & C_DMA2_TIMEOUT) {
				R_DMA_INT = C_DMA2_TIMEOUT;		// Clear timeout bit
				handle_dma_timout_first_round(C_DMA_CH2);
			} else {
				if (dma_previous_done_state[C_DMA_CH2] == C_DMA_STATUS_TIMEOUT)
					handle_dma_timout_second_round(C_DMA_CH2);
				else
					handle_dma_done(C_DMA_CH2);
				if (dma_usage[C_DMA_CH2] != C_DMA_OCCUPIED) {
					dma_free_channel(C_DMA_CH2);
				}
			}
		}
	}

	// DMA3
	if (pending & C_DMA3_INT_PEND) {		// DMA3 interrupt is pending
		if (dma_usage[C_DMA_CH3] != C_DMA_NOT_UESED) {
			R_DMA_INT = C_DMA3_INT_PEND;		// Clear pending bit
			if (pending & C_DMA3_TIMEOUT) {
				R_DMA_INT = C_DMA3_TIMEOUT;		// Clear timeout bit
				handle_dma_timout_first_round(C_DMA_CH3);
			} else {
				if (dma_previous_done_state[C_DMA_CH3] == C_DMA_STATUS_TIMEOUT)
					handle_dma_timout_second_round(C_DMA_CH3);
				else
					handle_dma_done(C_DMA_CH3);

				if (dma_usage[C_DMA_CH3] != C_DMA_OCCUPIED) {
					dma_free_channel(C_DMA_CH3);
				}
			}
		}
	}

	// DMA4
	if (pending & C_DMA4_INT_PEND) {		// DMA4 interrupt is pending
		if (dma_usage[C_DMA_CH4] != C_DMA_NOT_UESED) {
			R_DMA_INT = C_DMA4_INT_PEND;		// Clear pending bit
			if (pending & C_DMA4_TIMEOUT) {
				R_DMA_INT = C_DMA4_TIMEOUT;		// Clear timeout bit
				handle_dma_timout_first_round(C_DMA_CH4);
			} else {
				if (dma_previous_done_state[C_DMA_CH4] == C_DMA_STATUS_TIMEOUT)
					handle_dma_timout_second_round(C_DMA_CH4);
				else
					handle_dma_done(C_DMA_CH4);

				if (dma_usage[C_DMA_CH4] != C_DMA_OCCUPIED) {
					dma_free_channel(C_DMA_CH4);
				}
			}
		}
	}

	// DMA5
	if (pending & C_DMA5_INT_PEND) {		// DMA5 interrupt is pending
		if (dma_usage[C_DMA_CH5] != C_DMA_NOT_UESED) {
			R_DMA_INT = C_DMA5_INT_PEND;		// Clear pending bit
			if (pending & C_DMA5_TIMEOUT) {
				R_DMA_INT = C_DMA5_TIMEOUT;		// Clear timeout bit
				handle_dma_timout_first_round(C_DMA_CH5);
			} else {
				if (dma_previous_done_state[C_DMA_CH5] == C_DMA_STATUS_TIMEOUT)
					handle_dma_timout_second_round(C_DMA_CH5);
				else
					handle_dma_done(C_DMA_CH5);

				if (dma_usage[C_DMA_CH5] != C_DMA_OCCUPIED) {
					dma_free_channel(C_DMA_CH5);
				}
			}
		}
	}

	// DMA6
	if (pending & C_DMA6_INT_PEND) {		// DMA6 interrupt is pending
		if (dma_usage[C_DMA_CH6] != C_DMA_NOT_UESED) {
			R_DMA_INT = C_DMA6_INT_PEND;		// Clear pending bit
			if (pending & C_DMA6_TIMEOUT) {
				R_DMA_INT = C_DMA6_TIMEOUT;		// Clear timeout bit
				handle_dma_timout_first_round(C_DMA_CH6);
			} else {
				if (dma_previous_done_state[C_DMA_CH6] == C_DMA_STATUS_TIMEOUT)
					handle_dma_timout_second_round(C_DMA_CH6);
				else
					handle_dma_done(C_DMA_CH6);

				if (dma_usage[C_DMA_CH6] != C_DMA_OCCUPIED) {
					dma_free_channel(C_DMA_CH6);
				}
			}
		}
	}

	// DMA7
	if (pending & C_DMA7_INT_PEND) {		// DMA7 interrupt is pending
		if (dma_usage[C_DMA_CH7] != C_DMA_NOT_UESED) {
			R_DMA_INT = C_DMA7_INT_PEND;		// Clear pending bit
			if (pending & C_DMA7_TIMEOUT) {
				R_DMA_INT = C_DMA7_TIMEOUT;		// Clear timeout bit
				handle_dma_timout_first_round(C_DMA_CH7);
			} else {
				if (dma_previous_done_state[C_DMA_CH7] == C_DMA_STATUS_TIMEOUT)
					handle_dma_timout_second_round(C_DMA_CH7);
				else
					handle_dma_done(C_DMA_CH7);

				if (dma_usage[C_DMA_CH7] != C_DMA_OCCUPIED) {
					dma_free_channel(C_DMA_CH7);
				}
			}
		}
	}
}

#else

static void handle_dma_timout(INT32S channel)
{
	if (dma_user_isr[channel])
	{
        (*dma_user_isr[channel])((INT8U)channel, C_DMA_STATUS_TIMEOUT);
	}

	//dma_reset_channel(channel);

	if (dma_notify_variable[channel]) {
		*((INT8S *) dma_notify_variable[channel]) = (INT8S) C_DMA_STATUS_TIMEOUT;
	}

	if (dma_notify_queue[channel]) {
        INT32U msg = C_DMA_STATUS_TIMEOUT;
        #if (K_DMA_USE_CMSIS_OS == 1)
		osMessagePut(dma_notify_queue[channel], (uint32_t)&msg, osWaitForever);
        #else
          //xQueueSend(dma_notify_queue[channel], &msg, portMAX_DELAY);
          xQueueSendFromISR(dma_notify_queue[channel], (const void *)&msg, NULL);
        #endif
	}
}

static void dma_irq_handler(INT32U pending)							// Device ISR
{
	// DMA0
	if (pending & C_DMA0_INT_PEND) {		// DMA0 interrupt is pending
		if (dma_usage[C_DMA_CH0] != C_DMA_NOT_UESED) {
			if (pending & C_DMA0_TIMEOUT) {
				handle_dma_timout(C_DMA_CH0);
			} else {
				handle_dma_done(C_DMA_CH0);
			}

			if (dma_usage[C_DMA_CH0] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH0);
			}
		}
		R_DMA_INT = C_DMA0_INT_PEND;		// Clear pending bit
	}

	// DMA1
	if (pending & C_DMA1_INT_PEND) {		// DMA1 interrupt is pending
		if (dma_usage[C_DMA_CH1] != C_DMA_NOT_UESED) {
			if (pending & C_DMA1_TIMEOUT) {
				handle_dma_timout(C_DMA_CH1);
			} else {
				handle_dma_done(C_DMA_CH1);
			}

			if (dma_usage[C_DMA_CH1] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH1);
			}
		}
		R_DMA_INT = C_DMA1_INT_PEND;		// Clear pending bit
	}

	// DMA2
	if (pending & C_DMA2_INT_PEND) {		// DMA2 interrupt is pending
		if (dma_usage[C_DMA_CH2] != C_DMA_NOT_UESED) {
			if (pending & C_DMA2_TIMEOUT) {
				handle_dma_timout(C_DMA_CH2);
			} else {
				handle_dma_done(C_DMA_CH2);
			}

			if (dma_usage[C_DMA_CH2] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH2);
			}
		}
		R_DMA_INT = C_DMA2_INT_PEND;		// Clear pending bit
	}

	// DMA3
	if (pending & C_DMA3_INT_PEND) {		// DMA3 interrupt is pending
		if (dma_usage[C_DMA_CH3] != C_DMA_NOT_UESED) {
			if (pending & C_DMA3_TIMEOUT) {
				handle_dma_timout(C_DMA_CH3);
			} else {
				handle_dma_done(C_DMA_CH3);
			}

			if (dma_usage[C_DMA_CH3] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH3);
			}
		}
		R_DMA_INT = C_DMA3_INT_PEND;		// Clear pending bit
	}

	// DMA4
	if (pending & C_DMA4_INT_PEND) {		// DMA4 interrupt is pending
		if (dma_usage[C_DMA_CH4] != C_DMA_NOT_UESED) {
			if (pending & C_DMA4_TIMEOUT) {
				handle_dma_timout(C_DMA_CH4);
			} else {
				handle_dma_done(C_DMA_CH4);
			}

			if (dma_usage[C_DMA_CH4] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH4);
			}
		}
		R_DMA_INT = C_DMA4_INT_PEND;		// Clear pending bit
	}

	// DMA5
	if (pending & C_DMA5_INT_PEND) {		// DMA5 interrupt is pending
		if (dma_usage[C_DMA_CH5] != C_DMA_NOT_UESED) {
			if (pending & C_DMA5_TIMEOUT) {
				handle_dma_timout(C_DMA_CH5);
			} else {
				handle_dma_done(C_DMA_CH5);
			}

			if (dma_usage[C_DMA_CH5] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH5);
			}
		}
		R_DMA_INT = C_DMA5_INT_PEND;		// Clear pending bit
	}

	// DMA6
	if (pending & C_DMA6_INT_PEND) {		// DMA6 interrupt is pending
		if (dma_usage[C_DMA_CH6] != C_DMA_NOT_UESED) {
			if (pending & C_DMA6_TIMEOUT) {
				handle_dma_timout(C_DMA_CH6);
			} else {
				handle_dma_done(C_DMA_CH6);
			}

			if (dma_usage[C_DMA_CH6] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH6);
			}
		}
		R_DMA_INT = C_DMA6_INT_PEND;		// Clear pending bit
	}

	// DMA7
	if (pending & C_DMA7_INT_PEND) {		// DMA7 interrupt is pending
		if (dma_usage[C_DMA_CH7] != C_DMA_NOT_UESED) {
			if (pending & C_DMA7_TIMEOUT) {
				handle_dma_timout(C_DMA_CH7);
			} else {
				handle_dma_done(C_DMA_CH7);
			}

			if (dma_usage[C_DMA_CH7] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH7);
			}
		}
		R_DMA_INT = C_DMA7_INT_PEND;		// Clear pending bit
	}
}
#endif

void DMA_IRQHandler(void)							// Device ISR
{
	dma_irq_handler(R_DMA_INT);
}

/**
 * @brief   dma initialize
 * @param   none
 * @return 	none
 */
void drv_l1_dma_init(void)
{
	INT32U i;

	dma_clock_set(DMA_CLOCK_ENABLE);

	R_DMA0_CTRL	= C_DMA_CTRL_RESET;			// Software reset, this bit will auto clear after reset complete
	R_DMA0_TX_COUNT	= 0x0;
	R_DMA0_SPRITE_SIZE = 0x0;
	R_DMA0_TRANSPARENT = 0x0;
	R_DMA0_MISC = 0x0;

	R_DMA1_CTRL	= C_DMA_CTRL_RESET;
	R_DMA1_TX_COUNT	= 0x0;
	R_DMA1_SPRITE_SIZE = 0x0;
	R_DMA1_TRANSPARENT = 0x0;
	R_DMA1_MISC = 0x0;

	R_DMA2_CTRL	= C_DMA_CTRL_RESET;
	R_DMA2_TX_COUNT	= 0x0;
	R_DMA2_SPRITE_SIZE = 0x0;
	R_DMA2_TRANSPARENT = 0x0;
	R_DMA2_MISC = 0x0;

	R_DMA3_CTRL	= C_DMA_CTRL_RESET;
	R_DMA3_TX_COUNT	= 0x0;
	R_DMA3_SPRITE_SIZE = 0x0;
	R_DMA3_TRANSPARENT = 0x0;
	R_DMA3_MISC = 0x0;

	R_DMA4_CTRL	= C_DMA_CTRL_RESET;
	R_DMA4_TX_COUNT	= 0x0;
	R_DMA4_SPRITE_SIZE = 0x0;
	R_DMA4_TRANSPARENT = 0x0;
	R_DMA4_MISC = 0x0;

	R_DMA5_CTRL	= C_DMA_CTRL_RESET;
	R_DMA5_TX_COUNT	= 0x0;
	R_DMA5_SPRITE_SIZE = 0x0;
	R_DMA5_TRANSPARENT = 0x0;
	R_DMA5_MISC = 0x0;

	R_DMA6_CTRL	= C_DMA_CTRL_RESET;
	R_DMA6_TX_COUNT	= 0x0;
	R_DMA6_SPRITE_SIZE = 0x0;
	R_DMA6_TRANSPARENT = 0x0;
	R_DMA6_MISC = 0x0;

	R_DMA7_CTRL	= C_DMA_CTRL_RESET;
	R_DMA7_TX_COUNT	= 0x0;
	R_DMA7_SPRITE_SIZE = 0x0;
	R_DMA7_TRANSPARENT = 0x0;
	R_DMA7_MISC = 0x0;

	R_DMA_LINE_LEN = 0x0;

	R_DMA_DEVICE = 0x76543210;

	#if 1 // williamyeo_gcv770 modify
	//R_DMA_CEMODE = C_DMA_CE_DONT_RESET;  // when R_DMA_CEMODE = C_DMA_CE_DONT_RESET, this will cause spi have extra byte
	R_DMA_CEMODE = 0; // when R_DMA_CEMODE = 0, spi flash become normal
	#endif

	R_DMA_INT = 0xFFFFFFFF;						// Clear all pending bits

	for (i=0; i<C_DMA_CHANNEL_NUM; i++) {
		dma_usage[i] = (INT8U) C_DMA_NOT_UESED;
		dma_notify_variable[i] = (INT32U) NULL;
#if (K_DMA_USE_CMSIS_OS == 1)
		dma_notify_queue[i] = (osMessageQId) NULL;
#else
        dma_notify_queue[i] = (xQueueHandle) NULL;
#endif
		dma_checksum[i] = (INT32U) NULL;
	}

	for (i=0; i<C_DMA_Q_NUM; i++) {
		dma_q_usage[i] = (INT8U) FALSE;
		if (dma_driver_queue[i]) {
            OSQFlush(dma_driver_queue[i]);
		} else {
#if (K_DMA_USE_CMSIS_OS == 1)
			osMessageQDef_t dma_q = {C_DMA_Q_BUF_SIZE, sizeof(INT32U), 0};

			dma_driver_queue[i] = osMessageCreate(&dma_q, NULL);
#else
                  dma_driver_queue[i] = xQueueCreate(C_DMA_Q_BUF_SIZE, sizeof(INT32U));
#endif
		}
	}

    //NVIC_SetPriority(DMA_IRQn, 5);
    //NVIC_EnableIRQ(DMA_IRQn);
    vic_irq_register(VIC_DMA, DMA_IRQHandler);
    vic_irq_enable(VIC_DMA);
#if FIQ
    vic_fiq_enable(VIC_DMA);
#endif
    dma_init_done = TRUE;
}

/**
 * @brief   set dma direction
 * @param   channel: dma channel number
 * @param   dir: direction, frame buffer to sprite or sprite to frame buffer
 * @return 	none
 */
static void dma_set_direction(INT32U channel, INT32U dir)
{
	switch (channel) {
	case C_DMA_CH0:
		if(dir)
			R_DMA0_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA0_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH1:
		if(dir)
			R_DMA1_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA1_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH2:
		if(dir)
			R_DMA2_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA2_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH3:
		if(dir)
			R_DMA3_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA3_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH4:
		if(dir)
			R_DMA4_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA4_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH5:
		if(dir)
			R_DMA5_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA5_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH6:
		if(dir)
			R_DMA6_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA6_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH7:
		if(dir)
			R_DMA7_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA7_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	default:
		break;
	}
}

/**
 * @brief   set dma control register
 * @param   channel: dma channel number
 * @param   ctrl: dma control register value
 * @return 	none
 */
static void dma_set_control(INT32U channel, INT32U ctrl)
{
	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_CTRL = ctrl;
		break;
	case C_DMA_CH1:
		R_DMA1_CTRL = ctrl;
		break;
	case C_DMA_CH2:
		R_DMA2_CTRL = ctrl;
		break;
	case C_DMA_CH3:
		R_DMA3_CTRL = ctrl;
		break;
	case C_DMA_CH4:
		R_DMA4_CTRL = ctrl;
		break;
	case C_DMA_CH5:
		R_DMA5_CTRL = ctrl;
		break;
	case C_DMA_CH6:
		R_DMA6_CTRL = ctrl;
		break;
	case C_DMA_CH7:
		R_DMA7_CTRL = ctrl;
		break;
	default:
		break;
	}
}

/**
 * @brief   set dma source address
 * @param   channel: dma channel number
 * @param   addr: dma source address
 * @return 	none
 */
static void dma_set_source(INT32U channel, INT32U addr)
{
    if ((addr & 0xff000000) == 0x80000000) {
        addr ^= 0x80000000;
    }

	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_SRC_ADDR = addr;
		break;
	case C_DMA_CH1:
		R_DMA1_SRC_ADDR = addr;
		break;
	case C_DMA_CH2:
		R_DMA2_SRC_ADDR = addr;
		break;
	case C_DMA_CH3:
		R_DMA3_SRC_ADDR = addr;
		break;
	case C_DMA_CH4:
		R_DMA4_SRC_ADDR = addr;
		break;
	case C_DMA_CH5:
		R_DMA5_SRC_ADDR = addr;
		break;
	case C_DMA_CH6:
		R_DMA6_SRC_ADDR = addr;
		break;
	case C_DMA_CH7:
		R_DMA7_SRC_ADDR = addr;
		break;
	default:
		break;
	}
}

/**
 * @brief   set dma destination address
 * @param   channel: dma channel number
 * @param   addr: dma destination address
 * @return 	none
 */
static void dma_set_target(INT32U channel, INT32U addr)
{
    if ((addr & 0xff000000) == 0x80000000) {
        addr ^= 0x80000000;
    }

	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_TAR_ADDR = addr;
		break;
	case C_DMA_CH1:
		R_DMA1_TAR_ADDR = addr;
		break;
	case C_DMA_CH2:
		R_DMA2_TAR_ADDR = addr;
		break;
	case C_DMA_CH3:
		R_DMA3_TAR_ADDR = addr;
		break;
	case C_DMA_CH4:
		R_DMA4_TAR_ADDR = addr;
		break;
	case C_DMA_CH5:
		R_DMA5_TAR_ADDR = addr;
		break;
	case C_DMA_CH6:
		R_DMA6_TAR_ADDR = addr;
		break;
	case C_DMA_CH7:
		R_DMA7_TAR_ADDR = addr;
		break;
	default:
		break;
	}
}

/**
 * @brief   set dma transfer length
 * @param   channel: dma channel number
 * @param   count: transfer length
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S dma_set_tx_count(INT32U channel, INT32U count)
{
	if (count > C_DMA_COUNT_MAX) {
		return -1;
	}

	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_TX_COUNT = count;
		break;
	case C_DMA_CH1:
		R_DMA1_TX_COUNT = count;
		break;
	case C_DMA_CH2:
		R_DMA2_TX_COUNT = count;
		break;
	case C_DMA_CH3:
		R_DMA3_TX_COUNT = count;
		break;
	case C_DMA_CH4:
		R_DMA4_TX_COUNT = count;
		break;
	case C_DMA_CH5:
		R_DMA5_TX_COUNT = count;
		break;
	case C_DMA_CH6:
		R_DMA6_TX_COUNT = count;
		break;
	case C_DMA_CH7:
		R_DMA7_TX_COUNT = count;
		break;
	default:
		return -1;
	}
	return 0;
}

/**
 * @brief   get dma transfer length
 * @param   channel: dma channel number
 * @param   pcount: return transfer length
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S dma_get_tx_count(INT32U channel, INT32U *pcount)
{
    *pcount = 0;
	switch (channel) {
	case C_DMA_CH0:
		*pcount = R_DMA0_TX_COUNT & 0x00FFFFFF;
		break;
	case C_DMA_CH1:
		*pcount = R_DMA1_TX_COUNT & 0x00FFFFFF;
		break;
	case C_DMA_CH2:
		*pcount = R_DMA2_TX_COUNT & 0x00FFFFFF;
		break;
	case C_DMA_CH3:
		*pcount = R_DMA3_TX_COUNT & 0x00FFFFFF;
		break;
	case C_DMA_CH4:
		*pcount = R_DMA4_TX_COUNT & 0x00FFFFFF;
		break;
	case C_DMA_CH5:
		*pcount = R_DMA5_TX_COUNT & 0x00FFFFFF;
		break;
	case C_DMA_CH6:
		*pcount = R_DMA6_TX_COUNT & 0x00FFFFFF;
		break;
	case C_DMA_CH7:
		*pcount = R_DMA7_TX_COUNT & 0x00FFFFFF;
		break;
	default:
		return -1;
	}
	return 0;
}

static INT32S dma_set_toe_len(INT32U channel, INT32U len)
{
    INT32U value;

	switch (channel) {
	case C_DMA_CH0:
        value = R_DMA0_SPRITE_SIZE;
        value &= ~0xFFFF0000;
        value |= len << 16;
		R_DMA0_SPRITE_SIZE = value;
		value = R_DMA0_TRANSPARENT;
		value &= ~0x3FF0000;
		value |= (len >> 16) << 16;
		R_DMA0_TRANSPARENT = value;
		break;
	case C_DMA_CH1:
		value = R_DMA1_SPRITE_SIZE;
        value &= ~0xFFFF0000;
        value |= len << 16;
		R_DMA1_SPRITE_SIZE = value;
		value = R_DMA1_TRANSPARENT;
		value &= ~0x3FF0000;
		value |= (len >> 16) << 16;
		R_DMA1_TRANSPARENT = value;
		break;
	case C_DMA_CH2:
		value = R_DMA2_SPRITE_SIZE;
        value &= ~0xFFFF0000;
        value |= len << 16;
		R_DMA2_SPRITE_SIZE = value;
		value = R_DMA2_TRANSPARENT;
		value &= ~0x3FF0000;
		value |= (len >> 16) << 16;
		R_DMA2_TRANSPARENT = value;
		break;
	case C_DMA_CH3:
		value = R_DMA3_SPRITE_SIZE;
        value &= ~0xFFFF0000;
        value |= len << 16;
		R_DMA3_SPRITE_SIZE = value;
		value = R_DMA3_TRANSPARENT;
		value &= ~0x3FF0000;
		value |= (len >> 16) << 16;
		R_DMA3_TRANSPARENT = value;
		break;
	case C_DMA_CH4:
		value = R_DMA4_SPRITE_SIZE;
        value &= ~0xFFFF0000;
        value |= len << 16;
		R_DMA4_SPRITE_SIZE = value;
		value = R_DMA4_TRANSPARENT;
		value &= ~0x3FF0000;
		value |= (len >> 16) << 16;
		R_DMA4_TRANSPARENT = value;
		break;
	case C_DMA_CH5:
		value = R_DMA5_SPRITE_SIZE;
        value &= ~0xFFFF0000;
        value |= len << 16;
		R_DMA5_SPRITE_SIZE = value;
		value = R_DMA5_TRANSPARENT;
		value &= ~0x3FF0000;
		value |= (len >> 16) << 16;
		R_DMA5_TRANSPARENT = value;
		break;
	case C_DMA_CH6:
		value = R_DMA6_SPRITE_SIZE;
        value &= ~0xFFFF0000;
        value |= len << 16;
		R_DMA6_SPRITE_SIZE = value;
		value = R_DMA6_TRANSPARENT;
		value &= ~0x3FF0000;
		value |= (len >> 16) << 16;
		R_DMA6_TRANSPARENT = value;
		break;
	case C_DMA_CH7:
		value = R_DMA7_SPRITE_SIZE;
        value &= ~0xFFFF0000;
        value |= len << 16;
		R_DMA7_SPRITE_SIZE = value;
		value = R_DMA7_TRANSPARENT;
		value &= ~0x3FF0000;
		value |= (len >> 16) << 16;
		R_DMA7_TRANSPARENT = value;
		break;
	default:
		return -1;
	}
	return 0;
}
/**
 * @brief   set dma transfer device number
 * @param   channel: dma channel number
 * @param   count: device number
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S dma_set_device(INT32U channel, INT16U device)
{
	INT32U shift = 0, bshift = 0;
	INT32U bank = device >> 4;

	if ((channel>=C_DMA_CHANNEL_NUM) || (device>C_DMA_IO_MAX)) {
		return -1;
	}

	if (channel == C_DMA_CH0) {
		shift = C_DMA0_IO_SHIFT;
		bshift = C_DMA0_BANK_SHIFT;
	} else if (channel == C_DMA_CH1) {
		shift = C_DMA1_IO_SHIFT;
		bshift = C_DMA1_BANK_SHIFT;
	} else if (channel == C_DMA_CH2) {
		shift = C_DMA2_IO_SHIFT;
		bshift = C_DMA2_BANK_SHIFT;
	} else if (channel == C_DMA_CH3) {
		shift = C_DMA3_IO_SHIFT;
		bshift = C_DMA3_BANK_SHIFT;
	} else if (channel == C_DMA_CH4) {
		shift = C_DMA4_IO_SHIFT;
		bshift = C_DMA4_BANK_SHIFT;
	} else if (channel == C_DMA_CH5) {
		shift = C_DMA5_IO_SHIFT;
		bshift = C_DMA5_BANK_SHIFT;
	} else if (channel == C_DMA_CH6) {
		shift = C_DMA6_IO_SHIFT;
		bshift = C_DMA6_BANK_SHIFT;
	} else if (channel == C_DMA_CH7) {
		shift = C_DMA7_IO_SHIFT;
		bshift = C_DMA7_BANK_SHIFT;
	}

    taskENTER_CRITICAL();
	R_DMA_CEMODE &= ~(0x03 << bshift);
	R_DMA_CEMODE |= (bank << bshift);


	R_DMA_DEVICE &= ~((C_DMA_IO_MASK) << shift);
	R_DMA_DEVICE |= ((device & C_DMA_IO_MASK) << shift);
    taskEXIT_CRITICAL();

	return 0;
}

/**
 * @brief   set dma timeout value
 * @param   channel: dma channel number
 * @param   timeout: timeout value
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S dma_set_timeout(INT32U channel, INT32U timeout)
{
	if((channel>=C_DMA_CHANNEL_NUM) || (timeout>C_DMA_MISC_TIMEOUT_MAX)) {
		return -1;
	}

	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA0_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH1:
		R_DMA1_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA1_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH2:
		R_DMA2_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA2_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH3:
		R_DMA3_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA3_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH4:
		R_DMA4_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA4_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH5:
		R_DMA5_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA5_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH6:
		R_DMA6_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA6_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH7:
		R_DMA7_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA7_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	default:
		break;
	}

	return 0;
}

/**
 * @brief   set dma line length
 * @param   length: line length for LCD frame buffer
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S dma_set_line_length(INT32U length)
{
	if  (length > C_DMA_LINE_MAX) {
		return -1;
	}

	R_DMA_LINE_LEN = length;
	return 0;
}

/**
 * @brief   set dma sprite size
 * @param   channel: dma channel
 * @param   size: sprite size
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S dma_set_sprite_size(INT32U channel, INT32U size)
{
	INT32U value;
    INT32U value1;

	if  (size > C_DMA_SPRITE_MAX) {
		return -1;
	}

	switch (channel) {
	case C_DMA_CH0:
        value = R_DMA0_SPRITE_SIZE;
        value &= ~0x3FFF;
        value |= size;
		R_DMA0_SPRITE_SIZE = value;
		break;
	case C_DMA_CH1:
        value = R_DMA1_SPRITE_SIZE;
        value &= ~0x3FFF;
        value |= size;
		R_DMA1_SPRITE_SIZE = value;
		break;
	case C_DMA_CH2:
        value = R_DMA2_SPRITE_SIZE;
        value &= ~0x3FFF;
        value |= size;
		R_DMA2_SPRITE_SIZE = value;
		break;
	case C_DMA_CH3:
        value = R_DMA3_SPRITE_SIZE;
        value1 = R_DMA3_TRANSPARENT;
        value &= ~0x3FFF;
        value |= size;
		R_DMA3_SPRITE_SIZE = value;
		R_DMA3_TRANSPARENT = value1;
		break;
	case C_DMA_CH4:
        value = R_DMA4_SPRITE_SIZE;
        value &= ~0x3FFF;
        value |= size;
		R_DMA4_SPRITE_SIZE = value;
		break;
	case C_DMA_CH5:
        value = R_DMA5_SPRITE_SIZE;
        value &= ~0x3FFF;
        value |= size;
		R_DMA5_SPRITE_SIZE = value;
		break;
	case C_DMA_CH6:
        value = R_DMA6_SPRITE_SIZE;
        value &= ~0x3FFF;
        value |= size;
		R_DMA6_SPRITE_SIZE = value;
		break;
	case C_DMA_CH7:
        value = R_DMA7_SPRITE_SIZE;
        value &= ~0x3FFF;
        value |= size;
		R_DMA7_SPRITE_SIZE = value;
		break;
	default:
		return -1;
	}

	return 0;
}

#ifdef DMA_TRANSPANRENT
static INT32S dma_set_transparent_enable(INT32U channel)
{
	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH1:
		R_DMA1_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH2:
		R_DMA2_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH3:
		R_DMA3_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH4:
		R_DMA4_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH5:
		R_DMA5_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH6:
		R_DMA6_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH7:
		R_DMA7_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	default:
		return -1;
	}

	return 0;
}

static INT32S dma_set_transparent_disable(INT32U channel)
{
	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH1:
		R_DMA1_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH2:
		R_DMA2_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH3:
		R_DMA3_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH4:
		R_DMA4_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH5:
		R_DMA5_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH6:
		R_DMA6_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH7:
		R_DMA7_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	default:
		return -1;
	}

	return 0;
}

static INT32S dma_set_transparent_pattern(INT32U channel, INT16U pattern)
{
    INT32U value;

	if (pattern > C_DMA_TRANSPARENT_MAX) {
		return -1;
	}

	switch (channel) {
	case C_DMA_CH0:
        value = R_DMA0_TRANSPARENT;
        value &= ~0xFFFF;
        value |= pattern;
		R_DMA0_TRANSPARENT = value;
		break;
	case C_DMA_CH1:
		value = R_DMA1_TRANSPARENT;
        value &= ~0xFFFF;
        value |= pattern;
		R_DMA1_TRANSPARENT = value;
		break;
	case C_DMA_CH2:
		value = R_DMA2_TRANSPARENT;
        value &= ~0xFFFF;
        value |= pattern;
		R_DMA2_TRANSPARENT = value;
		break;
	case C_DMA_CH3:
		value = R_DMA3_TRANSPARENT;
        value &= ~0xFFFF;
        value |= pattern;
		R_DMA3_TRANSPARENT = value;
		break;
	case C_DMA_CH4:
		value = R_DMA4_TRANSPARENT;
        value &= ~0xFFFF;
        value |= pattern;
		R_DMA4_TRANSPARENT = value;
		break;
	case C_DMA_CH5:
		value = R_DMA5_TRANSPARENT;
        value &= ~0xFFFF;
        value |= pattern;
		R_DMA5_TRANSPARENT = value;
		break;
	case C_DMA_CH6:
		value = R_DMA6_TRANSPARENT;
        value &= ~0xFFFF;
        value |= pattern;
		R_DMA6_TRANSPARENT = value;
		break;
	case C_DMA_CH7:
		value = R_DMA7_TRANSPARENT;
        value &= ~0xFFFF;
        value |= pattern;
		R_DMA7_TRANSPARENT = value;
		break;
	default:
		return -1;
	}

	return 0;
}
#endif


static INT32S dma_transfer_extend(DMA_STRUCT *dma_struct, INT8U usage, INT32U os_q, void (*dma_user_isr)(INT8U, INT8U))
{
	INT32U s_addr = dma_struct->s_addr;
	INT32U t_addr = dma_struct->t_addr;
	INT32U count = dma_struct->count;
	INT32U channel, len, ctrl;
	INT16U src_type, target_type;
	INT8U is_sd_dma = 0;

	if (!count) {
		if (dma_struct->notify) {
			*(dma_struct->notify) = C_DMA_STATUS_DONE;
		}
		return 0;
	}

	if (dma_struct->width == DMA_DATA_WIDTH_1BYTE) {
		ctrl = C_DMA_CTRL_8BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = count;
	} else if (dma_struct->width == DMA_DATA_WIDTH_2BYTE) {
		if ((s_addr&0x1) || (t_addr&0x1)) {
			return -1;						// Both source and target address must be 2-byte alignment
		}

		ctrl = C_DMA_CTRL_16BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = count << 1;
	} else if (dma_struct->width == DMA_DATA_WIDTH_4BYTE) {
		if ((s_addr&0x3) || (t_addr&0x3)) {
			return -1;						// Both source and target address must be 4-byte alignment
		}

		ctrl = C_DMA_CTRL_32BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = count << 2;
	} else {
		return -1;
	}

	if ((s_addr<C_DMA_IO_ADDR_START) || (s_addr>C_DMA_IO_ADDR_END)) {
		src_type = C_DMA_MEMORY;
	} else {
		if (s_addr == (INT32U)P_UART0_BASE) {
			src_type = C_DMA_IO_UART0_RX;
		} else if (s_addr == (INT32U)P_UART1_BASE) {
			src_type = C_DMA_IO_UART1_RX;
		} else if (s_addr == (INT32U)P_UART2_BASE) {
			src_type = C_DMA_IO_UART2_RX;
		} else if (s_addr == (INT32U) P_SPI0_RX_DATA) {
			src_type = C_DMA_IO_SPI0_RX;
		} else if (s_addr == (INT32U) P_SPI1_RX_DATA) {
			src_type = C_DMA_IO_SPI1_RX;
		} else if (s_addr == (INT32U) P_SDC0_DATA_RX) {
			src_type = C_DMA_IO_SDC0;
			is_sd_dma = 0x81; // bit 7 mean sd0
		} else if (s_addr == (INT32U) P_SDC1_DATA_RX) {
			src_type = C_DMA_IO_SDC1;
			is_sd_dma = 0x01;
		} else if (s_addr == (INT32U) P_ADC_ASADC_DATA) {
			src_type = C_DMA_IO_ADC;
		} else if (s_addr == (INT32U) P_I2SRX_DATA) {
			src_type = C_DMA_IO_I2S_RX;
		} else if (s_addr == (INT32U) P_I2S1RX_DATA) {
			src_type = C_DMA_IO_I2S1_RX;
		} else if (s_addr == (INT32U) P_I2S2RX_DATA) {
			src_type = C_DMA_IO_I2S2_RX;
		} else if (s_addr == (INT32U) P_I2S3RX_DATA) {
			src_type = C_DMA_IO_I2S3_RX;
		} else if (s_addr == (INT32U) P_FIR_RXFIFO_BASE) {
			src_type = C_DMA_IO_FIR_RX;
		} else if (s_addr == (INT32U) P_FIR_TXFIFO_BASE) {
			src_type = C_DMA_IO_FIR_RX2;
		} else if (s_addr == (INT32U) R_I2C0_RXDMA) {
			src_type = C_DMA_IO_I2C0_RX;
		} else if (s_addr == (INT32U) R_I2C1_RXDMA) {
			src_type = C_DMA_IO_I2C1_RX;
		} else if (s_addr == (INT32U) R_I2C2_RXDMA) {
			src_type = C_DMA_IO_I2C2_RX;
		} else if (s_addr == (INT32U) P_TRNG_DMA_DATA) {
 			src_type = C_DMA_IO_TRNG;
		}
		else {
			return -4;						// Unknow IO
		}
	}
	if ((t_addr<C_DMA_IO_ADDR_START) || (t_addr>C_DMA_IO_ADDR_END)) {
		target_type = C_DMA_MEMORY;
	} else {
		if (src_type != C_DMA_MEMORY) {
			return -1;						// IO to IO is not supported
		}

		if (t_addr == (INT32U) P_UART0_BASE) {
			target_type = C_DMA_IO_UART0_TX;
		} else if (t_addr == (INT32U) P_UART1_BASE) {
			target_type = C_DMA_IO_UART1_TX;
		} else if (t_addr == (INT32U) P_UART2_BASE) {
			target_type = C_DMA_IO_UART2_TX;
		} else if (t_addr == (INT32U) P_SPI0_TX_DATA) {
			target_type = C_DMA_IO_SPI0_TX;
		} else if (t_addr == (INT32U) P_SPI1_TX_DATA) {
			target_type = C_DMA_IO_SPI1_TX;
		} else if (t_addr == (INT32U) P_DAC_CHA_DATA) {
			target_type = C_DMA_IO_DAC_CHA;
		} else if (t_addr == (INT32U) P_DAC_CHB_DATA) {
			target_type = C_DMA_IO_DAC_CHB;
		} else if (t_addr == (INT32U) P_SDC0_DATA_TX) {
			target_type = C_DMA_IO_SDC0;
			is_sd_dma = 0x82; // bit 7 mean sd0
        } else if (t_addr == (INT32U) P_SDC1_DATA_TX) {
			target_type = C_DMA_IO_SDC1;
			is_sd_dma = 0x02;
		} else if (t_addr == (INT32U) P_I2STX_DATA) {
			target_type = C_DMA_IO_I2S_TX;
		} else if (t_addr == (INT32U) P_I2S1TX_DATA) {
			target_type = C_DMA_IO_I2S1_TX;
		} else if (t_addr == (INT32U) P_I2S2TX_DATA) {
			target_type = C_DMA_IO_I2S2_TX;
		} else if (t_addr == (INT32U) P_I2S3TX_DATA) {
			target_type = C_DMA_IO_I2S3_TX;
		} else if (t_addr == (INT32U) P_FIR_TXFIFO_BASE) {
			target_type = C_DMA_IO_FIR_TX;
		} else if (t_addr == (INT32U) R_I2C0_TXDMA) {
			target_type = C_DMA_IO_I2C0_TX;
		} else if (t_addr == (INT32U) R_I2C1_TXDMA) {
			target_type = C_DMA_IO_I2C1_TX;
		} else if (t_addr == (INT32U) R_I2C2_TXDMA) {
			target_type = C_DMA_IO_I2C2_TX;
		}
		else {
			return -5;						// Unknow IO
		}
	}

	if (!dma_init_done) {
		drv_l1_dma_init();
	}

    channel = dma_get_channel(usage);
    dma_struct->channel = channel;
    if  (channel >= C_DMA_CHANNEL_NUM) {
        return -1;							// No free DMA channel is available
    }

    if(dma_struct->toe && dma_struct->toe->toe_mode == C_DMA_TOE_EN) {
        // toe checksum setting
        if (dma_struct->toe->assign_ch & 0x80000000) { // use fix channel
            dma_free_channel(channel);
            channel = dma_struct->toe->assign_ch & 0xFF;
            dma_struct->channel = channel;
            if(dma_set_channel(usage, channel) < 0) {
                return -1;							// DMA channel is not available
            }
        }
        ctrl |= C_DMA_CTRL_TOE_CHKSUM_MODE;
        if (dma_struct->toe->toe_swap == 1) {
            ctrl |= C_DMA_CTRL_TOE_CHKSUM_SWAP;
        }
        dma_set_toe_len(channel,dma_struct->toe->toe_len);
        check_sum[channel] = &dma_struct->toe->checksum;
    }

	// Prepare control register
	if (src_type != C_DMA_MEMORY) {			// IO to memory
		ctrl |= C_DMA_CTRL_DEMAND_TRANS | C_DMA_CTRL_IO2M | C_DMA_CTRL_SRC_FIX | C_DMA_CTRL_EXTERNAL;

		if (dma_set_device(channel, src_type)) {
			dma_free_channel(channel);
			return -1;
		}

		// Invalid target memory from cache
        if (t_addr == 0x40000000) { // spi C_SPI_DMA_DUMMY_ADDR, could not do clean_dcache, invalidate
            //DBG_PRINT("t_addr == 0x40000000\r\n ");
        }
        else {
            drv_l1_cp15_clean_dcache_range(t_addr, len);
            drv_l1_cp15_invalidate_dcache_range(t_addr, len); //cache_invalid_range(t_addr, len);
        }
	} else if (target_type != C_DMA_MEMORY) {	// Memory to IO
		ctrl |= C_DMA_CTRL_DEMAND_TRANS | C_DMA_CTRL_M2IO | C_DMA_CTRL_DEST_FIX | C_DMA_CTRL_EXTERNAL;

		if (dma_set_device(channel, target_type)) {
			dma_free_channel(channel);
			return -1;
		}

        // Drain source memory from cache
        if (s_addr == 0x40000000) { // spi C_SPI_DMA_DUMMY_ADDR, could not do clean_dcache
            //DBG_PRINT("s_addr == 0x40000000\r\n ");
        }
        else
            drv_l1_cp15_clean_dcache_range(s_addr, len); //cache_drain_range(s_addr, len);
	} else {								// Memory to memory
		ctrl |= C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_SRC_INCREASE | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_SOFTWARE;

		// Drain source memory and invalid target memory from cache
		drv_l1_cp15_clean_dcache_range(s_addr, len); //cache_drain_range(s_addr, len);
		drv_l1_cp15_clean_dcache_range(t_addr, len);
		drv_l1_cp15_invalidate_dcache_range(t_addr, len); //cache_invalid_range(t_addr, len);
	}

    // Check whether burst mode can be used
    if (usage == C_DMA_OCCUPIED || src_type == C_DMA_IO_FIR_RX || src_type == C_DMA_IO_FIR_RX2 || src_type == C_DMA_IO_FIR_TX || target_type == C_DMA_IO_FIR_TX \
        /* || src_type == C_DMA_IO_SPI0_RX || src_type == C_DMA_IO_SPI1_RX || target_type == C_DMA_IO_SPI0_TX || target_type == C_DMA_IO_SPI1_TX*/ ) {
        ctrl |= C_DMA_CTRL_SINGLE_ACCESS;
    } else if (!(count & 0x7)) {
        ctrl |= C_DMA_CTRL_BURST8_ACCESS;
    } else if (!(count & 0x3)) {
        ctrl |= C_DMA_CTRL_BURST4_ACCESS;
    } else {
        ctrl |= C_DMA_CTRL_SINGLE_ACCESS;
    }

	if(dma_user_isr != NULL)
    {
        drv_l1_dma_callback_set(dma_struct->channel, dma_user_isr);
    }
    else
        drv_l1_dma_callback_clear(dma_struct->channel);

    dma_is_sd[channel] = is_sd_dma;

	dma_set_notify(channel, (INT32U) dma_struct->notify, os_q);

	// Set source address
	dma_set_source(channel, s_addr);

	// Set target address
	dma_set_target(channel, t_addr);

	// Set transmit counter
	if (dma_set_tx_count(channel, count)) {
		dma_free_channel(channel);
		return -1;
	}

	{
		INT32U timeout = (INT32U)(dma_struct->timeout);

//		if (timeout == 0) // williamyeo, when timeout is 0(disable timeout), set it to 255 so that have 1 second timeout
//			timeout = 255;
		if (dma_set_timeout(channel, timeout)) {
			dma_free_channel(channel);
			return -1;
		}
	}

	if (dma_set_sprite_size(channel, 0)) {
		dma_free_channel(channel);
		return -1;
	}

	// Start DMA now

	#if (K_SD_DMA_TEST_CODE == 2) // TEST CODE
	if (sd_dma_test)
	{
		// stop here and plug out card to sim dma timeout
        sd_dma_test = 0;
	}
	#endif

    if (dma_struct->fast_apb == 1) {
        ctrl |= C_DMA_CTRL_FAST_APB;
        //print_string("ctrl: 0x%x, fast_apb\r\n",ctrl);
    }

	#if (K_DMA_TIMEOUT_PATCH_APPLY == 1)
	dma_previous_done_state[channel] = C_DMA_STATUS_WAITING;
	#endif

	dma_set_control(channel, ctrl);

	#if (K_SD_DMA_TEST_CODE == 1) // TEST CODE
	if (sd_dma_test)
	{
        sd_dma_test = 0;
		// purpoesely reset dma to sim dma become dead case so that get software osEventTimeout
        //drv_usec_wait(2);
        dma_reset_channel(channel);
    }
	#endif

	return 0;
}

/**
 * @brief   transfer data by dma
 * @param   dma_struct: dma struct
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S drv_l1_dma_transfer(DMA_STRUCT *dma_struct)
{
	if (!dma_struct) {
		return -1;
	}
	return dma_transfer_extend(dma_struct, C_DMA_NORMAL_USED, (INT32U) NULL, NULL);
}

void drv_l1_dma_enforce_timeout(INT32U channel)
{
	dma_irq_handler(dma_get_int_flag(channel) | dma_get_timeout_flag(channel));
}

#define K_SD_READ_TIMEOUT_PER_SECTOR	105 // ms unit, should be 100, add more 5 for safe.
											// although sd read spec timeout per sector is 100ms, use 105ms, because 326XXB hardware is 104.5 ms
#define K_SD_WRITE_TIMEOUT_PER_SECTOR   250

/**
 * @brief   dma transfer data and wait transfer finish
 * @param   dma_struct: dma struct
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S drv_l1_dma_transfer_wait_ready(DMA_STRUCT *dma_struct)
{
	volatile INT8S notify;
	INT32U q_index;
//	INT32S mask;
#if (K_DMA_USE_CMSIS_OS == 1)
	osEvent result;
#else
	INT32S result;
	INT32S msg;
#endif
	if (!dma_struct) {
		return -1;
	}
	if (!(dma_struct->notify)) {
		dma_struct->notify = &notify;
	}

	*(dma_struct->notify) = C_DMA_STATUS_WAITING;
	q_index = dma_get_queue();
	if (q_index >= C_DMA_Q_NUM) {
		return -1;
	}
  	if (dma_transfer_extend(dma_struct, C_DMA_NORMAL_USED, (INT32U) dma_driver_queue[q_index], NULL)) {
  		if (dma_struct->notify == &notify) {		// Restore dma_struct->notify
  			dma_struct->notify = NULL;
  		}
  		dma_free_queue(q_index);

		return -1;
	}

	INT32U msecwait = 10000;

	// williamyeo, to avoid loop forever, set timeout when using osMessageGet
  	{
		static INT16U sd_read_over_spec_sector_timeout[2] = {0, 0};
		INT8U sd_access_type = dma_is_sd[dma_struct->channel] & 0x7f; // 1 is sd read, 2 is sd write
		INT8U sd_device_id;
		INT32U sd_spec_sector_timeout; // in ms unit
        INT32U dma_first_count;
        INT32U sd_nr_sector=0;

		if (sd_access_type)
		{
            dma_get_tx_count(dma_struct->channel, &dma_first_count);
            if (dma_struct->width == DMA_DATA_WIDTH_2BYTE)
                dma_first_count = (dma_first_count << 1);
            else if (dma_struct->width == DMA_DATA_WIDTH_4BYTE)
                dma_first_count = (dma_first_count << 2);

			sd_device_id = ((dma_is_sd[dma_struct->channel] & 0x80) == 0x80) ? 0 : 1;
			sd_spec_sector_timeout = (sd_access_type == 1) ? K_SD_READ_TIMEOUT_PER_SECTOR : K_SD_WRITE_TIMEOUT_PER_SECTOR;
			if (sd_access_type == 1) // read
			{
				if (sd_read_over_spec_sector_timeout[sd_device_id] != 0)
					sd_spec_sector_timeout = sd_read_over_spec_sector_timeout[sd_device_id];
			}
		}

        if (dma_struct->timeout == 0)
		{
			if (sd_access_type)
			{
				// set dma timeout for sd read and sd write

				if (dma_struct->width == DMA_DATA_WIDTH_1BYTE)
					sd_nr_sector = (dma_struct->count + 511) >> 9;
				else if (dma_struct->width == DMA_DATA_WIDTH_2BYTE)
					sd_nr_sector = ((dma_struct->count << 1) + 511) >> 9;
				else
					sd_nr_sector = ((dma_struct->count << 2) + 511) >> 9;

				if (sd_access_type == 1)
				{
					// double the timeout, because discover burn out sd card read time will up to 168ms per sector
					if (sd_read_over_spec_sector_timeout[sd_device_id] != 0)
						msecwait = sd_nr_sector * sd_spec_sector_timeout;
					else
						msecwait = sd_nr_sector*K_SD_READ_TIMEOUT_PER_SECTOR*2;
				}
				else
					msecwait = (sd_nr_sector * sd_spec_sector_timeout) + K_SD_WRITE_TIMEOUT_PER_SECTOR + 615;

				// this is to adjust precision of msec cause by os tick
				msecwait = (msecwait * 1016) / 1000;
				//DBG_PRINT("sd dma timeout %u, count %u, w %u, dma_is_sd[%u]=%u\r\n", msecwait, dma_struct->count, dma_struct->width, dma_struct->channel, dma_is_sd[dma_struct->channel]);
			}
			else
			{
            #if (K_DMA_USE_CMSIS_OS == 1)
				msecwait = osWaitForever;
            #else
				msecwait = portMAX_DELAY;//osWaitForever;
            #endif
			}
		}
		else
            msecwait = ((((INT32U)dma_struct->timeout) << 2)*1016)/1000;

        if (sd_access_type)
        {
			// check sd read timeout 9 bit early
			INT32U sd_status = 0;
            INT32U slicemswait;
            INT32U mswait;
			INT32U accmswait = 0;
            INT32U previous_accmswait = 0;
            INT32U dmamswait;
            INT32U sdc_timeout_ms = (((INT64U)10485760000)/MCLK)+ 3;
            INT16U slice_idx = 0;
            INT32U sd_req_bytes = sd_nr_sector * 512UL;

            dmamswait = (sd_access_type == 1) ? (K_SD_READ_TIMEOUT_PER_SECTOR*2) : 615;
            mswait = msecwait;
            slicemswait = sdc_timeout_ms;
            if (slicemswait > 200)
                slicemswait = 200;

            while (mswait != 0)
            {
                // check sd read bit 9 status timeout
                sd_status = (sd_device_id == 0) ? R_SDC0_STATUS : R_SDC1_STATUS;

                if ((sd_status & (1<<9)) != 0)
                {
					// trigger dummy clock to allow sd complete it action
                    drvl1_sdc_dummyclk_en(sd_device_id, 1);
                    drv_l1_dma_enforce_timeout((INT32U)dma_struct->channel);
                    #if (K_DMA_USE_CMSIS_OS == 1)
                    result = osMessageGet(dma_driver_queue[q_index], 100);
                    #else
                    result = (INT32S)xQueueReceive(dma_driver_queue[q_index], &msg, 100);
                    #endif
					if (sd_spec_sector_timeout > accmswait)
					{
                        if (drvl1_sdc_check_card_present(sd_device_id))
                            drv_msec_wait(sd_spec_sector_timeout - accmswait);
                    }
					drvl1_sdc_dummyclk_en(sd_device_id, 0); // disable sd clock

					if (MCLK >= 81000000)
                        sd_read_over_spec_sector_timeout[sd_device_id] = 135;
					else
						sd_read_over_spec_sector_timeout[sd_device_id] = 0;
					//DBG_PRINT("SD STATUS BIT9 ON!!! %u, %u\r\n", sd_spec_sector_timeout, sd_read_over_spec_sector_timeout[sd_device_id]);
                    if (sd_spec_sector_timeout == K_SD_READ_TIMEOUT_PER_SECTOR)
                        DBG_PRINT("9");
                    else
                        DBG_PRINT("6");
                    break;
                }
                else
                    sd_read_over_spec_sector_timeout[sd_device_id] = 0;

                if (slicemswait > mswait)
                    slicemswait = mswait;
            #if (K_DMA_USE_CMSIS_OS == 1)
                result = osMessageGet(dma_driver_queue[q_index], slicemswait);
            #else
                result = (INT32S)xQueueReceive(dma_driver_queue[q_index], &msg, slicemswait);
            #endif
				accmswait += slicemswait;
                mswait -= slicemswait;
                if (mswait == 0)
                {
                    break;
                }
            #if (K_DMA_USE_CMSIS_OS == 1)
                if (result.status != osEventTimeout)
            #else
                if (result == pdPASS)
            #endif
                    break;

                if ((accmswait - previous_accmswait) >= dmamswait)
                {
                    INT32U dma_second_count;

                    dma_get_tx_count(dma_struct->channel, &dma_second_count);
                     if (dma_struct->width == DMA_DATA_WIDTH_2BYTE)
                        dma_second_count = (dma_second_count << 1);
                    else if (dma_struct->width == DMA_DATA_WIDTH_4BYTE)
                        dma_second_count = (dma_second_count << 2);
                    if ((dma_second_count != 0) && (dma_second_count == dma_first_count) && (dma_second_count != sd_req_bytes))
                    {
                        drv_l1_dma_enforce_timeout((INT32U)dma_struct->channel);
                        DBG_PRINT("1 sd-%u dma left count is same after a while. %u, %u, %u, %u, 0x%08x, %u\r\n", sd_device_id, dma_second_count, dmamswait, sd_access_type, accmswait, sd_status, sd_nr_sector);
                    #if (K_DMA_USE_CMSIS_OS == 1)
                        result = osMessageGet(dma_driver_queue[q_index], 100);
                    #else
                        result = (INT32S)xQueueReceive(dma_driver_queue[q_index], &msg, 100);
                    #endif
                        drvl1_sdc_clear_timeout_bit(sd_device_id, 10); // this is not timeout bit bug, so clear it
                        break;
                    }
                    dma_first_count = dma_second_count;
                    previous_accmswait = accmswait;
                }

                if (drvl1_sdc_check_card_present(sd_device_id) == 0)
                {
                    drv_l1_dma_enforce_timeout((INT32U)dma_struct->channel);
                #if (K_DMA_USE_CMSIS_OS == 1)
                    result = osMessageGet(dma_driver_queue[q_index], 100);
                #else
                    result = (INT32S)xQueueReceive(dma_driver_queue[q_index], &msg, 100);
                #endif
                    break;
                }

                slice_idx++;
                if (slice_idx == 1)
                    slicemswait = 2;
                else if (slice_idx == 2)
                    slicemswait = 2;
                else
                    slicemswait = 100;
            } // end while
        }
        else
    	#if (K_DMA_USE_CMSIS_OS == 1)
        	result = osMessageGet(dma_driver_queue[q_index], msecwait);
    	#else
        	result = (INT32S)xQueueReceive(dma_driver_queue[q_index], &msg, msecwait);
    	#endif
    }

#if (K_DMA_USE_CMSIS_OS == 1)
    if (result.status != osEventMessage) // osEventTimeout
    {
        DBG_PRINT("dma result.status = 0x%x timeout=%u msecwait=%u count=%u w=%u\r\n", result.status, dma_struct->timeout, msecwait, dma_struct->count, dma_struct->width);
    }
#else
    if(result != pdPASS)
    {
        DBG_PRINT("dma result = 0x%x timeout=%u msecwait=%u count=%u w=%u\r\n", result, dma_struct->timeout, msecwait, dma_struct->count, dma_struct->width);
    }
#endif

#if (K_DMA_USE_CMSIS_OS == 1)
	if (result.status == osEventMessage) {
#else
    if(result == pdPASS) {
#endif
  		if (dma_struct->notify == &notify) {		// Restore dma_struct->notify
  			dma_struct->notify = NULL;
  		}
#if (K_DMA_USE_CMSIS_OS == 1)
		if (result.value.v == C_DMA_STATUS_DONE) {
#else
        if (msg == C_DMA_STATUS_DONE) {
#endif
			dma_free_queue(q_index);
			return 0;
		}
#if (K_DMA_USE_CMSIS_OS == 1)
		if (result.value.v == C_DMA_STATUS_TIMEOUT) {
#else
        if (msg == C_DMA_STATUS_TIMEOUT) {
#endif
			dma_free_queue(q_index);
			return -1;
		}
		//return -1;		// DMA timeout // williamyeo remark this line, let it go below code to reset dma when error
  	}

	#if (K_DMA_TIMEOUT_PATCH_APPLY == 1)
       drv_l1_dma_enforce_timeout((INT32U)dma_struct->channel);
       #if (K_DMA_USE_CMSIS_OS == 1)
       result = osMessageGet(dma_driver_queue[q_index], msecwait);
       #else
        result = (INT32S)xQueueReceive(dma_driver_queue[q_index], &msg, msecwait);
       #endif
    #else
	// If we don't receive response from DMA, we have to reset DMA controller and free the channel by ourselves
	mask = dma_device_protect();
	dma_free_channel((INT32U) dma_struct->channel);
	dma_device_unprotect(mask);
	#endif

	dma_free_queue(q_index);

	if (*(dma_struct->notify) == C_DMA_STATUS_DONE) {
  		if (dma_struct->notify == &notify) {		// Restore dma_struct->notify
  			dma_struct->notify = NULL;
  		}

		return 0;
	}

	if (dma_struct->notify == &notify) {		// Restore dma_struct->notify
		dma_struct->notify = NULL;
	}

	return -1;								// DMA timeout
}

#if 1
/**
 * @brief   dma transfer data with queue
 * @param   dma_struct: dma struct
 * @param   os_q: notify queue
 * @return 	result: >=0 is success, <0 is fail.
 */
#if (K_DMA_USE_CMSIS_OS == 1)
INT32S drv_l1_dma_transfer_with_queue(DMA_STRUCT *dma_struct, osMessageQId os_q)
#else
INT32S drv_l1_dma_transfer_with_queue(DMA_STRUCT *dma_struct, xQueueHandle os_q)
#endif
{
	if (!dma_struct || !os_q) {
		return -1;
	}
	return dma_transfer_extend(dma_struct, C_DMA_NORMAL_USED, (INT32U) os_q, NULL);
}

/**
 * @brief   dma transfer data with callback
 * @param   dma_struct: dma struct
 * @param   *dma_user_isr: function pointer of dma user callback isr
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S drv_l1_dma_transfer_with_callback(DMA_STRUCT *dma_struct, void (*dma_user_isr)(INT8U, INT8U))
{
	if (!dma_struct /*|| !os_q*/) {
		return -1;
	}
	return dma_transfer_extend(dma_struct, C_DMA_NORMAL_USED, (INT32U) NULL, dma_user_isr);
}

/**
 * @brief   dma transfer data with callback
 * @param   dma_struct: dma struct
 * @param   *dma_user_isr: function pointer of dma user callback isr
 * @return 	result: >=0 is success, <0 is fail.
 */
#if (K_DMA_USE_CMSIS_OS == 1)
INT32S drv_l1_dma_transfer_with_queue_n_callback(DMA_STRUCT *dma_struct, osMessageQId os_q, void (*dma_user_isr)(INT8U, INT8U))
#else
INT32S drv_l1_dma_transfer_with_queue_n_callback(DMA_STRUCT *dma_struct, xQueueHandle os_q, void (*dma_user_isr)(INT8U, INT8U))
#endif
{
	if (!dma_struct || !os_q) {
		return -1;
	}
	return dma_transfer_extend(dma_struct, C_DMA_NORMAL_USED, (INT32U) os_q, dma_user_isr);
}

/**
 * @brief   dma transfer data with queue in double buffer mode.
 * @param   dma_struct: dma struct
 * @param   os_q: notify queue
 * @return 	result: >=0 is success, <0 is fail.
 */
#if (K_DMA_USE_CMSIS_OS == 1)
INT32S drv_l1_dma_transfer_with_double_buf(DMA_STRUCT *dma_struct, osMessageQId os_q)
#else
INT32S drv_l1_dma_transfer_with_double_buf(DMA_STRUCT *dma_struct, xQueueHandle os_q)
#endif
{
	if (!dma_struct /*|| !os_q*/) {
		return -1;
	}
	return dma_transfer_extend(dma_struct, C_DMA_OCCUPIED, (INT32U) os_q, NULL);
}
/**
 * @brief   dma transfer data with queue and callback in double buffer mode.
 * @param   dma_struct: dma struct
 * @param   os_q: notify queue
 * @param   *dma_user_isr: function pointer of dma user callback isr
 * @return 	result: >=0 is success, <0 is fail.
 */
#if (K_DMA_USE_CMSIS_OS == 1)
INT32S drv_l1_dma_transfer_double_buf_with_callback(DMA_STRUCT *dma_struct, osMessageQId os_q, void (*dma_user_isr)(INT8U, INT8U))
#else
INT32S drv_l1_dma_transfer_double_buf_with_callback(DMA_STRUCT *dma_struct, xQueueHandle os_q, void (*dma_user_isr)(INT8U, INT8U))
#endif
{
	if (!dma_struct /*|| !os_q*/) {
		return -1;
	}
	return dma_transfer_extend(dma_struct, C_DMA_OCCUPIED, (INT32U) os_q, dma_user_isr);
}
#else
/**
 * @brief   dma transfer data in double buffer mode.
 * @param   dma_struct: dma struct
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S drv_l1_dma_transfer_with_double_buf(DMA_STRUCT *dma_struct)
{
	if (!dma_struct) {
		return -1;
	}
	return dma_transfer_extend(dma_struct, C_DMA_OCCUPIED, NULL);
}
#endif

/**
 * @brief   dma transfer next data in double buffer mode.
 * @param   dma_struct: dma struct
 * @param   os_q: notify queue
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S drv_l1_dma_transfer_double_buf_set(DMA_STRUCT *dma_struct)
{
  	INT32S mask;
    INT32U s_addr = dma_struct->s_addr;
	INT32U t_addr = dma_struct->t_addr;
	INT32U count = dma_struct->count;
    INT32U len;
    INT16U src_type, target_type;

	if (!dma_struct || dma_struct->channel>=C_DMA_Q_NUM) {
		return -1;
	}
	if (dma_usage[dma_struct->channel] != C_DMA_OCCUPIED) {
		return -1;
	}

    if (dma_struct->width == DMA_DATA_WIDTH_1BYTE)
        len = count;
    else if (dma_struct->width == DMA_DATA_WIDTH_2BYTE)
        len = count << 1;
    else if (dma_struct->width == DMA_DATA_WIDTH_4BYTE)
        len = count << 2;
    else
        return -1;
    if ((s_addr<C_DMA_IO_ADDR_START) || (s_addr>C_DMA_IO_ADDR_END))
        src_type = C_DMA_MEMORY;
    else {
        src_type = C_DMA_IO_I2S_RX; // here, purpose is find out memort or IO, so just set any IO is ok
    }
	if ((t_addr<C_DMA_IO_ADDR_START) || (t_addr>C_DMA_IO_ADDR_END))
		target_type = C_DMA_MEMORY;
    else
        target_type = C_DMA_IO_I2S_TX; // here, purpose is find out memort or IO, so just set any IO is ok

    if (src_type != C_DMA_MEMORY) {	// IO to memory
        drv_l1_cp15_clean_dcache_range(t_addr, len);
        drv_l1_cp15_invalidate_dcache_range(t_addr, len);//cache_invalid_range(t_addr, len);
    }
    else if (target_type != C_DMA_MEMORY)
        drv_l1_cp15_clean_dcache_range(s_addr, len); //cache_drain_range(s_addr, len);
    else {
        drv_l1_cp15_clean_dcache_range(s_addr, len); //cache_drain_range(s_addr, len);
        drv_l1_cp15_clean_dcache_range(t_addr, len);
        drv_l1_cp15_invalidate_dcache_range(t_addr, len); //cache_invalid_range(t_addr, len);
    }

	dma_set_source((INT32U) dma_struct->channel, dma_struct->s_addr);
	dma_set_target((INT32U) dma_struct->channel, dma_struct->t_addr);

	if (dma_set_tx_count((INT32U) dma_struct->channel, dma_struct->count)) {
		mask = dma_device_protect();

		dma_free_channel((INT32U) dma_struct->channel);

		dma_device_unprotect(mask);

		return -1;
	}

	return 0;
}

/**
 * @brief   dma free channel in double buffer mode.
 * @param   dma_struct: dma struct
 * @param   os_q: notify queue
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S drv_l1_dma_transfer_double_buf_free(DMA_STRUCT *dma_struct)
{
  	INT32S mask;

	if (!dma_struct) {
		return -1;
	}
	if (dma_usage[dma_struct->channel] != C_DMA_OCCUPIED) {
		return -1;
	}

	mask = dma_device_protect();

	dma_free_channel((INT32U) dma_struct->channel);

	dma_device_unprotect(mask);

	return 0;
}

/**
 * @brief   Check double buffer full flag.
 * @param   channel: channel number, C_DMA_CH0 ~ C_DMA_CH7
 * @return 	1: Full, 0: Not full
 */
INT32S drv_l1_dma_dbf_status_get(INT8U channel)
{
	INT32U ctrl;

	if (channel == C_DMA_CH0) {
		ctrl = R_DMA0_CTRL;
	} else if (channel == C_DMA_CH1) {
		ctrl = R_DMA1_CTRL;
	} else if (channel == C_DMA_CH2) {
		ctrl = R_DMA2_CTRL;
	} else if (channel == C_DMA_CH3) {
		ctrl = R_DMA3_CTRL;
	} else if (channel == C_DMA_CH4) {
		ctrl = R_DMA4_CTRL;
	} else if (channel == C_DMA_CH5) {
		ctrl = R_DMA5_CTRL;
	} else if (channel == C_DMA_CH6) {
		ctrl = R_DMA6_CTRL;
	} else if (channel == C_DMA_CH7) {
		ctrl = R_DMA7_CTRL;
	} else {
		ctrl = 0;
	}

	if (ctrl & C_DMA_CTRL_DBF) {
		return 1;
	}
	return 0;
}

/**
 * @brief   Check DMA channel status
 * @param   channel: channel number, C_DMA_CH0 ~ C_DMA_CH7
 * @return 	1: Busy, 0: Idle
 */
INT32S drv_l1_dma_status_get(INT8U channel)
{
	INT32U ctrl;

	if (channel == C_DMA_CH0) {
		ctrl = R_DMA0_CTRL;
	} else if (channel == C_DMA_CH1) {
		ctrl = R_DMA1_CTRL;
	} else if (channel == C_DMA_CH2) {
		ctrl = R_DMA2_CTRL;
	} else if (channel == C_DMA_CH3) {
		ctrl = R_DMA3_CTRL;
	} else if (channel == C_DMA_CH4) {
		ctrl = R_DMA4_CTRL;
	} else if (channel == C_DMA_CH5) {
		ctrl = R_DMA5_CTRL;
	} else if (channel == C_DMA_CH6) {
		ctrl = R_DMA6_CTRL;
	} else if (channel == C_DMA_CH7) {
		ctrl = R_DMA7_CTRL;
	} else {
		ctrl = 0;
	}

	if (ctrl & C_DMA_CTRL_BUSY) {
		return 1;
	}
	return 0;
}

/**
 * @brief   Fill memory by DMA
 * @param   t_addr: memory address
 * @param   value: filled value
 * @param   byte_count: data length in byte
 * @return 	0: Success, -1: Fail
 */
INT32S drv_l1_dma_memory_fill(INT32U t_addr, INT8U value, INT32U byte_count)
{
	INT8U *p8;
	INT32U *p32;
	INT32U ctrl, channel;
	INT32U src_value;
	INT32S ret;
  	INT32U q_index;
      #if (K_DMA_USE_CMSIS_OS == 1)
	osEvent result;
      #else
        INT32S result;
        INT32S msg;
      #endif
//  	INT32S mask;
  	volatile INT8S notify;

	if (!dma_init_done) {
		drv_l1_dma_init();
	}

	// If length is less than 16 bytes, uses CPU to set the memory directly
	if (byte_count < 16) {
		p8 = (INT8U *) t_addr;
		while (byte_count--) {
			*p8++ = value;
		}

		return 0;
	}

	// Make sure start address is 4-byte alignment
	while (t_addr & 0x3) {
		*((INT8U *) t_addr++) = value;
		byte_count--;
	}
	// Make sure end address is on 4-byte boundry
	while (byte_count & 0x3) {
		*((INT8U *) t_addr+byte_count-1) = value;
		byte_count--;
	}

	src_value = (value<<24) | (value<<16) | (value<<8) | value;

	// If left length is less than 128 bytes, uses CPU to set the memory directly
	if (byte_count < 128) {
		p32 = (INT32U *) t_addr;
		while (byte_count) {
			*p32++ = src_value;
			byte_count -= 4;
		}
		return 0;
	}

	// Create a queue to receive DMA result
	q_index = dma_get_queue();
	if (q_index >= C_DMA_Q_NUM) {
		return -1;
	}

	// Get a free DMA channel
	channel = dma_get_channel(C_DMA_NORMAL_USED);
	if  (channel >= C_DMA_CHANNEL_NUM) {
	  	dma_free_queue(q_index);
		return -1;							// No free DMA channel is available
	}

	// Set transmit counter
	ret = dma_set_tx_count(channel, byte_count>>2);

	// Disable timeout function
	if (dma_set_timeout(channel, 0)) {
		ret = -1;
	}

	// Disable skip function
	if (dma_set_sprite_size(channel, 0)) {
		ret = -1;
	}

	// Return if any error occurs
	if (ret) {
	  	dma_free_queue(q_index);
		dma_free_channel(channel);
		return -1;
	}

	// Set DMA status to waiting transmit
	notify = C_DMA_STATUS_WAITING;
	dma_set_notify(channel, (INT32U) &notify, (INT32U) dma_driver_queue[q_index]);

	// Set source address
	dma_set_source(channel, (INT32U) &src_value);

	// Set target address
	dma_set_target(channel, t_addr);

	// Drain source memory and invalid target memory from cache
	drv_l1_cp15_clean_dcache_range((INT32U) &src_value, 4); // cache_drain_range((INT32U) &src_value, 4);
	drv_l1_cp15_clean_dcache_range(t_addr, byte_count);
	drv_l1_cp15_invalidate_dcache_range(t_addr, byte_count); // cache_invalid_range(t_addr, byte_count);

	// Prepare control register
	if (!((byte_count>>2) & 0x7)) {
		ctrl = C_DMA_CTRL_BURST8_ACCESS | C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_32BIT | C_DMA_CTRL_M2M | C_DMA_CTRL_INT | C_DMA_CTRL_SRC_FIX | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_SOFTWARE | C_DMA_CTRL_ENABLE;
	} else if (!((byte_count>>2) & 0x3)) {
		ctrl = C_DMA_CTRL_BURST4_ACCESS | C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_32BIT | C_DMA_CTRL_M2M | C_DMA_CTRL_INT | C_DMA_CTRL_SRC_FIX | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_SOFTWARE | C_DMA_CTRL_ENABLE;
	} else {
		ctrl = C_DMA_CTRL_SINGLE_ACCESS | C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_32BIT | C_DMA_CTRL_M2M | C_DMA_CTRL_INT | C_DMA_CTRL_SRC_FIX | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_SOFTWARE | C_DMA_CTRL_ENABLE;
	}
	// Start DMA now
	#if (K_DMA_TIMEOUT_PATCH_APPLY == 1)
	dma_previous_done_state[channel] = C_DMA_STATUS_WAITING;
	#endif
	dma_set_control(channel, ctrl);

	// Wait until DMA finish transmitting or timeout
      #if (K_DMA_USE_CMSIS_OS == 1)
	result = osMessageGet(dma_driver_queue[q_index], osWaitForever);
    if((result.status != osEventMessage) || (result.value.v != C_DMA_STATUS_DONE)) {
        return -1;
    }
      #else
        result = (INT32S)xQueueReceive(dma_driver_queue[q_index], &msg, portMAX_DELAY);
        if(result != pdPASS || msg != C_DMA_STATUS_DONE){
          return -1;
        }
      #endif
	dma_free_queue(q_index);

	// If we don't receive response from DMA, we have to reset DMA controller and free the channel by ourselves
	//mask = dma_device_protect();
	//dma_free_channel(channel);
	//dma_device_unprotect(mask);

	if (notify == C_DMA_STATUS_DONE) {
		return 0;
	}
	return -1;

	//while (notify == C_DMA_STATUS_WAITING) ;
	//return 0;					// DMA timeout function is not enabled, DMA transfer must be ok to reach here
}

/**
 * @brief   Copy a section of data
 * @param   s_addr: Source address
 * @param   t_addr: Destination address
 * @param   byte_count: Data number needed to be copied
 * @param   s_width: Buffer width of source buffer
 * @param   t_width: Buffer width of destination buffer
 * @param   dir:dma direction
 * @param   mode:Prepare control mod
 * @return 	0: Success, -1: Fail
 */
INT32S dma_buffer_copy_extend(INT32U s_addr, INT32U t_addr, INT32U byte_count, INT32U s_width, INT32U t_width, INT32U dir, INT32U mode)
{
	INT32U ctrl, channel;
	INT32S ret;
	INT32U q_index;
      #if (K_DMA_USE_CMSIS_OS == 1)
	osEvent result;
      #else
        INT32S result;
        INT32S msg;
      #endif
//	INT32S mask;
  	volatile INT8S notify;

	if (!dma_init_done) {
		drv_l1_dma_init();
	}

	// Make sure address and size are 2-byte alignment
	if ((s_addr & 0x1) || (t_addr & 0x1) || (byte_count & 0x1) || (s_width & 0x1) || (t_width & 0x1) || (s_width>t_width)) {
		return -1;
	}

  	// Create a queue to receive DMA result
	q_index = dma_get_queue();
	if (q_index >= C_DMA_Q_NUM) {
		return -1;
	}

	// Get a free DMA channel
	channel = dma_get_channel(C_DMA_NORMAL_USED);
	if  (channel >= C_DMA_CHANNEL_NUM) {
	  	dma_free_queue(q_index);
		return -1;							// No free DMA channel is available
	}

	// Set transmit counter
	ret = dma_set_tx_count(channel, byte_count>>1);

	// Disable timeout function
	if (dma_set_timeout(channel, 0)) {
		ret = -1;
	}

	dma_set_direction(channel, dir);

	// Set destination buffer width (1 means 2 bytes)
	if (dma_set_line_length(t_width>>1)) {
		ret = -1;
	}

	// Set source buffer width (1 means 2 bytes)
	if (dma_set_sprite_size(channel, s_width>>1)) {
		ret = -1;
	}

	// Return if any error occurs
	if (ret) {
	  	dma_free_queue(q_index);
		dma_free_channel(channel);

		return -1;
	}

	// Set DMA status to waiting transmit
	notify = C_DMA_STATUS_WAITING;
	dma_set_notify(channel, (INT32U) &notify, (INT32U) dma_driver_queue[q_index]);

	// Set source address
	dma_set_source(channel, s_addr);

	// Set target address
	dma_set_target(channel, t_addr);

	// Drain source memory and invalid target memory from cache
	drv_l1_cp15_clean_dcache_range(s_addr, byte_count); //cache_drain_range(s_addr, byte_count);
	drv_l1_cp15_clean_dcache_range(t_addr, (byte_count/s_width)*t_width);
	drv_l1_cp15_invalidate_dcache_range(t_addr, (byte_count/s_width)*t_width); //cache_invalid_range(t_addr, (byte_count/s_width)*t_width);

	// Prepare control register, burst is not supported in sprite copy mode
	ctrl = mode | C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_16BIT | C_DMA_CTRL_M2M | C_DMA_CTRL_INT | C_DMA_CTRL_SRC_INCREASE | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_SOFTWARE | C_DMA_CTRL_ENABLE;
	// Start DMA now
	#if (K_DMA_TIMEOUT_PATCH_APPLY == 1)
	dma_previous_done_state[channel] = C_DMA_STATUS_WAITING;
	#endif
	dma_set_control(channel, ctrl);

	// Wait until DMA finish transmitting or timeout
      #if (K_DMA_USE_CMSIS_OS == 1)
	result = osMessageGet(dma_driver_queue[q_index], osWaitForever);
  	if((result.status != osEventMessage) || (result.value.v != C_DMA_STATUS_DONE)) {
        return -1;
    }
      #else
        result = (INT32S)xQueueReceive(dma_driver_queue[q_index], &msg, portMAX_DELAY);
        if(result != pdPASS || msg != C_DMA_STATUS_DONE){
          return -1;
        }
      #endif

    dma_free_queue(q_index);

	// If we don't receive response from DMA, we have to reset DMA controller and free the channel by ourselves
	//mask = dma_device_protect();
	//dma_free_channel(channel);
	//dma_device_unprotect(mask);

	if (notify == C_DMA_STATUS_DONE) {
		return 0;
	}
	return -1;

	//while (notify == C_DMA_STATUS_WAITING) ;

	//return 0;					// DMA timeout function is not enabled, DMA transfer must be ok to reach here
}

/**
 * @brief   Copy a section of data
 * @param   s_addr: Source address
 * @param   t_addr: Destination address
 * @param   byte_count: Data number needed to be copied
 * @param   s_width: Buffer width of source buffer
 * @param   t_width: Buffer width of destination buffer
 			All parameters must be multiple of 2,
 			s_width(source buffer width) must <= t_width(target buffer width)
 * @return 	0: Success, -1: Fail
 */
INT32S drv_l1_dma_buffer_copy(INT32U s_addr, INT32U t_addr, INT32U byte_count, INT32U s_width, INT32U t_width)
{
	return dma_buffer_copy_extend(s_addr, t_addr, byte_count, s_width, t_width, C_DMA_MISC_SPRITE_TO_FB, C_DMA_CTRL_SINGLE_ACCESS);
}

/**
 * @brief   Copy memory data
 * @param   dest: Destination address
 * @param   src: Source address
 * @param   len: Data number needed to be copied
 * @return 	0: Success, -1: Fail
 */
INT32S dma_transfer_data(INT32U dest, INT32U src, INT32U len)
{
#if 1
	R_DMA0_CTRL	= C_DMA_CTRL_RESET; /* reset dma */
	R_DMA_INT = R_DMA_INT; /* clear interrupt flag */

	R_DMA0_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK); /* set time-out value */
	R_DMA0_MISC |= 0xFF << C_DMA_MISC_TIMEOUT_SHIFT; /* 1 sec */

	/* set DMA 0 for transfering data */
	R_DMA0_SRC_ADDR = (INT32U)src;
	R_DMA0_TAR_ADDR = (INT32U)dest;
	R_DMA0_TX_COUNT = len>>1;
	R_DMA0_CTRL     = C_DMA_CTRL_SINGLE_ACCESS|C_DMA_CTRL_SINGLE_TRANS|C_DMA_CTRL_SRC_INCREASE|C_DMA_CTRL_DEST_INCREASE
	                 |C_DMA_CTRL_SOFTWARE|C_DMA_CTRL_16BIT|C_DMA_CTRL_INT|C_DMA_CTRL_NORMAL_INT|C_DMA_CTRL_ENABLE;


	while((R_DMA_INT&C_DMA0_INT_PEND) == 0);
	if (R_DMA_INT & C_DMA0_TIMEOUT) {
		return C_DMA0_TIMEOUT;
	}
	R_DMA_INT = C_DMA0_INT_PEND;

	return STATUS_OK;
#else
	memcpy((void *)dest, (void *)src, len);
	return STATUS_OK;
#endif
}

INT32S dma_data_memcpy(INT8U* s1, const INT8U* s2, INT32U n)
{
	DMA_STRUCT dma;
	INT8U *ss, *dd;
	INT32U s_offset = 0, d_offset = 0, remain = 0;
	INT32U i, w;

	if (n < 1024)
	{
		gp_memcpy((INT8S*)s1, (INT8S*)s2, n);
		return 0;
	}

	ss = (INT8U*)s2;
	dd = (INT8U*)s1;

	//4 byte alignment
	w = 4;
	s_offset = (INT32U)s2 & 0x3;
	d_offset = (INT32U)s1 & 0x3;

	//2 byte alignment
	if (s_offset != d_offset)
	{
		w = 2;
		s_offset = (INT32U)s2 & 0x1;
		d_offset = (INT32U)s1 & 0x1;
	}

	//single byte width
	if (s_offset != d_offset)
	{
		w = 1;
		s_offset = d_offset = 0;
	}

	if (s_offset)
	{
		ss += (w-s_offset);
		dd += (w-s_offset);
		for (i = 0; i <(w-s_offset); i++)
			*((INT8U*)s1+i) = *((INT8U*)s2+i);
		n -= (w-s_offset);
	}

	remain = n & (w-1);
	i = w/2;
	n = n >> i;

	if (n & 0x7) // count should be 8 alignment for max peformence
	{
		remain += (n&0x7)*w;
		n = n >> 3 << 3;
	}

	gp_memset((INT8S*)&dma, 0, sizeof(DMA_STRUCT));
	dma.s_addr = (INT32U)ss;
	dma.t_addr = (INT32U)dd;
	dma.count = n;
	dma.width = w;
	dma.timeout = 255;

	if (0 > drv_l1_dma_transfer_wait_ready(&dma))
	{
		DBG_PRINT("dma copy fail\r\n");
		return -1;
	}

	if (remain)
	{
		n = n << i;
		gp_memcpy((INT8S *)(dd+n), (INT8S *)(ss+n), remain);
	}

	return 0;
}

INT32S drv_l1_dma_set_usage(INT8U usage, INT8U channel)
{
    INT32S ret;
    if(channel >= C_DMA_CHANNEL_NUM) {
		return STATUS_FAIL;
	}
	if (dma_usage[channel] == C_DMA_NOT_UESED) {
		dma_usage[channel] = usage;
		ret = STATUS_OK;
	} else {
		ret = STATUS_FAIL;
	}

	return ret;
}

INT32U drv_l1_dma_get_channel(INT8U usage)
{
    INT32U ch;
    if(usage >= C_DMA_OCCUPIED) {
		return STATUS_FAIL;
	}
	ch = dma_get_channel(usage);
    drv_l1_dma_set_usage(C_DMA_NORMAL_USED,ch);
	return ch;
}

void drv_l1_dma_set_device(INT32U channel,INT32U s_addr,INT32U t_addr)
{
    INT16U device=0;

    if (s_addr == (INT32U) R_I2C0_RXDMA) {
        device = C_DMA_IO_I2C0_RX;
    }else if (s_addr == (INT32U) R_I2C1_RXDMA) {
        device = C_DMA_IO_I2C1_RX;
    }else if (s_addr == (INT32U) R_I2C2_RXDMA) {
        device = C_DMA_IO_I2C2_RX;
    }
    if (t_addr == (INT32U) R_I2C0_TXDMA) {
        device = C_DMA_IO_I2C0_TX;
    }else if (t_addr == (INT32U) R_I2C1_TXDMA) {
        device = C_DMA_IO_I2C1_TX;
    }else if (t_addr == (INT32U) R_I2C2_TXDMA) {
        device = C_DMA_IO_I2C2_TX;
    }
    dma_set_device(channel,device);
}

static INT32S dma_setting(INT32U channel, INT32U saddr, INT32U taddr, INT32U xfer_cnt, INT32U timeout, INT32U sprite_sz)
{
	if((xfer_cnt > C_DMA_COUNT_MAX) || (timeout>C_DMA_MISC_TIMEOUT_MAX) || (sprite_sz > C_DMA_SPRITE_MAX)) {
		return -1;
	}
	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_SRC_ADDR = saddr;
		R_DMA0_TAR_ADDR = taddr;
		R_DMA0_TX_COUNT = xfer_cnt;
        R_DMA0_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA0_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA0_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH1:
		R_DMA1_SRC_ADDR = saddr;
		R_DMA1_TAR_ADDR = taddr;
		R_DMA1_TX_COUNT = xfer_cnt;
        R_DMA1_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA1_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA1_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH2:
		R_DMA2_SRC_ADDR = saddr;
		R_DMA2_TAR_ADDR = taddr;
		R_DMA2_TX_COUNT = xfer_cnt;
		R_DMA2_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA2_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA2_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH3:
		R_DMA3_SRC_ADDR = saddr;
		R_DMA3_TAR_ADDR = taddr;
		R_DMA3_TX_COUNT = xfer_cnt;
		R_DMA3_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA3_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA3_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH4:
		R_DMA4_SRC_ADDR = saddr;
		R_DMA4_TAR_ADDR = taddr;
		R_DMA4_TX_COUNT = xfer_cnt;
		R_DMA4_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA4_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA4_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH5:
		R_DMA5_SRC_ADDR = saddr;
		R_DMA5_TAR_ADDR = taddr;
		R_DMA5_TX_COUNT = xfer_cnt;
		R_DMA5_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA5_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA5_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH6:
		R_DMA6_SRC_ADDR = saddr;
		R_DMA6_TAR_ADDR = taddr;
		R_DMA6_TX_COUNT = xfer_cnt;
		R_DMA6_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA6_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA6_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH7:
		R_DMA7_SRC_ADDR = saddr;
		R_DMA7_TAR_ADDR = taddr;
		R_DMA7_TX_COUNT = xfer_cnt;
		R_DMA7_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA7_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA7_SPRITE_SIZE = sprite_sz;
		break;
	default:
		return -1;
	}
	return 0;
}

INT32S drv_l1_dma_transfer_sdio(DMA_STRUCT *dma_struct, INT8U usage)
{
	INT32U s_addr = dma_struct->s_addr;
	INT32U t_addr = dma_struct->t_addr;
	INT32U count = dma_struct->count;
	INT32U channel, len, ctrl;
	INT16U src_type, target_type;

	if (!dma_struct) {
		return -1;
	}
    if (dma_struct->width == DMA_DATA_WIDTH_4BYTE) {
		if ((s_addr&0x3) || (t_addr&0x3)) {
			return -1;						// Both source and target address must be 4-byte alignment
		}
		ctrl = C_DMA_CTRL_32BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = count << 2;
	} else if (dma_struct->width == DMA_DATA_WIDTH_2BYTE) {
		if ((s_addr&0x1) || (t_addr&0x1)) {
			return -1;						// Both source and target address must be 2-byte alignment
		}
		ctrl = C_DMA_CTRL_16BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = count << 1;
		print_string("%s s_addr %x t_addr %x width %d byte len %d\r\n",__FUNCTION__,s_addr,t_addr,dma_struct->width,len);
	} else if (dma_struct->width == DMA_DATA_WIDTH_1BYTE) {
		ctrl = C_DMA_CTRL_8BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = count ;
		print_string("%s s_addr %x t_addr %x width %d byte len %d\r\n",__FUNCTION__,s_addr,t_addr,dma_struct->width,len);
	} else {
		return -1;
	}

    //set source type
    if ((s_addr<C_DMA_IO_ADDR_START) || (s_addr>C_DMA_IO_ADDR_END)) {
		src_type = C_DMA_MEMORY;
	} else {
		if (s_addr == (INT32U) P_SDC0_DATA_RX) {
			src_type = C_DMA_IO_SDC0;
        } else if (s_addr == (INT32U) P_SDC1_DATA_RX) {
			src_type = C_DMA_IO_SDC1;
		} else {
			return -4;						// unknown IO
		}
	}
	//set target type
	if ((t_addr<C_DMA_IO_ADDR_START) || (t_addr>C_DMA_IO_ADDR_END)) {
		target_type = C_DMA_MEMORY;
	} else {
		if (src_type != C_DMA_MEMORY) {
			return -1;						// IO to IO is not supported
		}
		if (t_addr == (INT32U) P_SDC0_DATA_TX) {
			target_type = C_DMA_IO_SDC0;
        } else if (t_addr == (INT32U) P_SDC1_DATA_TX) {
			target_type = C_DMA_IO_SDC1;
		} else {
			return -5;						// unknown IO
		}
    }

    if (!dma_init_done) {
        drv_l1_dma_init();
	}
    channel = dma_get_channel(C_DMA_NORMAL_USED);
	dma_struct->channel = channel;
	if (channel >= C_DMA_CHANNEL_NUM) {
        return -1;							// No free DMA channel is available
    }
 	dma_set_notify(channel, (INT32U) dma_struct->notify, 0);
/*
	// Set source address
	dma_set_source(channel, s_addr);

	// Set target address
	dma_set_target(channel, t_addr);

	// Set transmit counter
	if (dma_set_tx_count(channel, count)) {
		dma_free_channel(channel);
		return -1;
	}

	if (dma_set_timeout(channel, dma_struct->timeout)) {
		dma_free_channel(channel);
		return -1;
	}

	if (dma_set_sprite_size(channel, 0)) {
		dma_free_channel(channel);
		return -1;
	}
*/
    dma_setting(channel, s_addr, t_addr, count, dma_struct->timeout, 0);
	// Prepare control register
	if (src_type != C_DMA_MEMORY) {			// IO to memory
		ctrl |= C_DMA_CTRL_DEMAND_TRANS | C_DMA_CTRL_IO2M | C_DMA_CTRL_SRC_FIX | C_DMA_CTRL_EXTERNAL;

		if (dma_set_device(channel, src_type)) {
			dma_free_channel(channel);
			return -1;
		}

		// Invalid target memory from cache
		cache_invalid_range(t_addr, len);

	} else if (target_type != C_DMA_MEMORY) {	// Memory to IO
		ctrl |= C_DMA_CTRL_DEMAND_TRANS | C_DMA_CTRL_M2IO | C_DMA_CTRL_DEST_FIX | C_DMA_CTRL_EXTERNAL;

		if (dma_set_device(channel, target_type)) {
			dma_free_channel(channel);
			return -1;
		}

		// Drain source memory from cache
		cache_drain_range(s_addr, len);
	}
	else
	{								// Memory to memory
		ctrl |= C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_SRC_INCREASE | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_SOFTWARE;

		// Drain source memory and invalid target memory from cache
		cache_drain_range(s_addr, len);
		cache_invalid_range(t_addr, len);
	}

    // Check whether burst mode can be used
    if (usage == C_DMA_OCCUPIED || src_type == C_DMA_IO_FIR_RX || src_type == C_DMA_IO_FIR_RX2 || src_type == C_DMA_IO_FIR_TX || target_type == C_DMA_IO_FIR_TX) {
        ctrl |= C_DMA_CTRL_SINGLE_ACCESS;
    } else if (!(count & 0x7)) {
        ctrl |= C_DMA_CTRL_BURST8_ACCESS;
    } else if (!(count & 0x3)) {
        ctrl |= C_DMA_CTRL_BURST4_ACCESS;
    } else {
        ctrl |= C_DMA_CTRL_SINGLE_ACCESS;
    }

	// Start DMA now
	dma_set_control(channel, ctrl);
    return 0;
}

void drv_l1_dma_ce_mode(INT8U enable)
{
    INT32U val=R_DMA_CEMODE;

    val &= ~1;

    if(enable){//ce auto cleared
        R_DMA_CEMODE = val;
    }else{      //ce not auto cleared
        val |= 1;
        R_DMA_CEMODE = val;
    }

}

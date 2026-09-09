/*
* Purpose:
*
* Author:
*
* Date:
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.00
*/
#include "drv_l1.h"

void (*keychange0_callback)(void);
void (*keychange1_callback)(void);
void (*keychange2_callback)(void);
void (*keychange3_callback)(void);
void (*keychange4_callback)(void);
void (*keychange5_callback)(void);
void (*keychange6_callback)(void);
void (*keychange7_callback)(void);

static INT32U edge_flag[8] = {0};

void KEYCHANGE_IRQHandler(void);
void KEYCHANGE_FOR_WIFI_IRQHandler(void);

INT8U g_kc_inten_flag = 0;
/**
 * @brief   Initialize keychange function
 * @param   vic_num - 0:IRQ, 1~4: FIQ number
 * @return 	none
 */
void drv_l1_keychange_init(INT32U channel, INT32U double_edge, INT8U vic_num)
{
    R_KEYC_POS_EN = 0;
	R_KEYC_POS_EN |= 0x1<<channel;
	R_SYSTEM_CLK_CTRL |= 0x1<<9;//Key Change Enable
	edge_flag[channel]  = double_edge;

	vic_irq_register(VIC_KC, KEYCHANGE_IRQHandler);
	vic_irq_enable(VIC_KC);
    if (vic_num) {
		vic_fiq_enable(VIC_KC);
    } else {
        vic_fiq_disable(VIC_KC);
    }
}


/**
 * @brief   Initialize keychange function
 * @param   vic_num - 0:IRQ, 1~4: FIQ number
 * @return 	none
 */
void drv_l1_keychange_for_wifi_init(INT32U channel, INT32U double_edge, INT8U vic_num)
{
    R_KEYC_POS_EN = 0;
	R_KEYC_POS_EN |= 0x1<<channel;
	R_SYSTEM_CLK_CTRL |= 0x1<<9;//Key Change Enable
	edge_flag[channel]  = double_edge;

	vic_irq_register(VIC_KC, KEYCHANGE_FOR_WIFI_IRQHandler);
	vic_irq_enable(VIC_KC);
    if (vic_num) {
		vic_fiq_enable(VIC_KC);
    } else {
        vic_fiq_disable(VIC_KC);
    }
}


/**
 * @brief   Un-Initialize keychange function
 * @param   Channel - ex: KEYC_CH0~7
 * @param   vic_num - 0:IRQ, 1~4: FIQ number
 * @return 	none
 */
void drv_l1_keychange_uninit(INT32U channel, INT8U vic_num)
{
    R_KEYC_POS_EN &= ~(0x1<<channel);
	R_SYSTEM_CLK_CTRL &= ~(0x1<<9);

	vic_irq_unregister(VIC_KC);
    vic_irq_disable(VIC_KC);
	vic_fiq_disable(VIC_KC);
}
/**
 * @brief   Clear keychange interrupt flag
 * @param   Channel: 0~7
 * @return 	none
 */
void drv_l1_keychange_interrupt_flag_clear(INT8U channel)
{
	R_INT_KEYCH |= 0x1<<((channel*2)+1);
}
/**
 * @brief   Enable keychange interrupt
 * @param   Channel: 0~7
 * @return 	none
 */
void drv_l1_keychange_interrupt_enable(INT8U channel)
{
	R_INT_KEYCH |= 0x1<<(channel*2);
}
/**
 * @brief   Disable keychange interrupt
 * @param   Channel: 0~7
 * @return 	none
 */
void drv_l1_keychange_interrupt_disable(INT8U channel)
{
	R_INT_KEYCH &= ~(0x1<<(channel*2));
}
/**
 * @brief   Update latch value for keychange,
            trigger interrupt when get change with latch value.
 * @param   none
 * @return 	keychang latch value
 */
INT32U drv_l1_keychange_latch_update(void)
{
	INT32U temp;
	return temp = R_KEYCH;
}
/**
 * @brief   Get keychange interrupt flag
 * @param   none
 * @return 	keychange interrupt flag
 */
INT32U drv_l1_keychange_status_get(void)
{
	return R_INT_KEYCH;
}
/**
 * @brief   Set keychange interrupt callback
 * @param   Channel  : 0~7
            user_isr : callback fucntion
 * @return 	none
 */
void drv_l1_keychange_user_isr_set(INT8U channel,void (*user_isr)(void))
{
	switch(channel)
	{
		case KEYC0:
			keychange0_callback = user_isr;
		break;
		case KEYC1:
			keychange1_callback = user_isr;
		break;
		case KEYC2:
			keychange2_callback = user_isr;
		break;
		case KEYC3:
			keychange3_callback = user_isr;
		break;
		case KEYC4:
			keychange4_callback = user_isr;
		break;
		case KEYC5:
			keychange5_callback = user_isr;
		break;
		case KEYC6:
			keychange6_callback = user_isr;
		break;
		case KEYC7:
			keychange7_callback = user_isr;
		break;
		default:break;
	}
}
/**
 * @brief   Clear keychange interrupt callback
 * @param   Channel  : 0~7
 * @return 	none
 */
void drv_l1_keychange_user_isr_clear(INT8U channel)
{
	switch(channel)
	{
		case KEYC0:
			keychange0_callback = 0;
		break;
		case KEYC1:
			keychange1_callback = 0;
		break;
		case KEYC2:
			keychange2_callback = 0;
		break;
		case KEYC3:
			keychange3_callback = 0;
		break;
		case KEYC4:
			keychange4_callback = 0;
		break;
		case KEYC5:
			keychange5_callback = 0;
		break;
		case KEYC6:
			keychange6_callback = 0;
		break;
		case KEYC7:
			keychange7_callback = 0;
		break;
		default:break;
	}
}

void KEYCHANGE_IRQHandler(void)
{
	INT32U int_status = drv_l1_keychange_status_get();
	if((int_status&INT_K0) == INT_K0) {
		drv_l1_keychange_interrupt_disable(0);
		drv_l1_keychange_interrupt_flag_clear(0);
		if(keychange0_callback)
            (*keychange0_callback)();
		if(edge_flag[0])
            drv_l1_keychange_latch_update();
		drv_l1_keychange_interrupt_enable(0);
	}

	if((int_status&INT_K1) == INT_K1) {
		drv_l1_keychange_interrupt_disable(1);
		drv_l1_keychange_interrupt_flag_clear(1);
		if(keychange1_callback)
            (*keychange1_callback)();
		if(edge_flag[1])
            drv_l1_keychange_latch_update();
		drv_l1_keychange_interrupt_enable(1);
	}

	if((int_status&INT_K2) == INT_K2) {
		drv_l1_keychange_interrupt_disable(2);
		drv_l1_keychange_interrupt_flag_clear(2);
		if(keychange2_callback)
            (*keychange2_callback)();
		if(edge_flag[2])
            drv_l1_keychange_latch_update();
		drv_l1_keychange_interrupt_enable(2);
	}

	if((int_status&INT_K3) == INT_K3) {
		drv_l1_keychange_interrupt_disable(3);
		drv_l1_keychange_interrupt_flag_clear(3);
		if(keychange3_callback)
            (*keychange3_callback)();
		if(edge_flag[3])
            drv_l1_keychange_latch_update();
		drv_l1_keychange_interrupt_enable(3);
	}

	if((int_status&INT_K4) == INT_K4) {
		drv_l1_keychange_interrupt_disable(4);
		drv_l1_keychange_interrupt_flag_clear(4);
		if(keychange4_callback)
            (*keychange4_callback)();
		if(edge_flag[4])
            drv_l1_keychange_latch_update();
		drv_l1_keychange_interrupt_enable(4);
	}

	if((int_status&INT_K5) == INT_K5) {
		drv_l1_keychange_interrupt_disable(5);
		drv_l1_keychange_interrupt_flag_clear(5);
		if(keychange5_callback)
            (*keychange5_callback)();
		if(edge_flag[5])
            drv_l1_keychange_latch_update();
		drv_l1_keychange_interrupt_enable(5);
	}

	if((int_status&INT_K6) == INT_K6) {
		drv_l1_keychange_interrupt_disable(6);
		drv_l1_keychange_interrupt_flag_clear(6);
		if(keychange6_callback)
            (*keychange6_callback)();
		if(edge_flag[6])
            drv_l1_keychange_latch_update();
		drv_l1_keychange_interrupt_enable(6);
	}

	if((int_status&INT_K7) == INT_K7) {
		drv_l1_keychange_interrupt_disable(7);
		drv_l1_keychange_interrupt_flag_clear(7);
		if(keychange7_callback)
            (*keychange7_callback)();
		if(edge_flag[7])
            drv_l1_keychange_latch_update();
		drv_l1_keychange_interrupt_enable(7);
	}
}

void KEYCHANGE_FOR_WIFI_IRQHandler(void)
{
	INT32U int_status = drv_l1_keychange_status_get();
	if((int_status&INT_K0) == INT_K0) {
		drv_l1_keychange_interrupt_flag_clear(0);
		if(keychange0_callback)
            (*keychange0_callback)();
	}

	if((int_status&INT_K1) == INT_K1) {
		drv_l1_keychange_interrupt_flag_clear(1);
		if(keychange1_callback)
            (*keychange1_callback)();
	}

	if((int_status&INT_K2) == INT_K2) {
		drv_l1_keychange_interrupt_flag_clear(2);
		if(keychange2_callback)
            (*keychange2_callback)();
	}

	if((int_status&INT_K3) == INT_K3) {
		drv_l1_keychange_interrupt_flag_clear(3);
		if(keychange3_callback)
            (*keychange3_callback)();
	}

	if((int_status&INT_K4) == INT_K4) {
		drv_l1_keychange_interrupt_flag_clear(4);
		if(keychange4_callback)
            (*keychange4_callback)();
	}

	if((int_status&INT_K5) == INT_K5) {
		drv_l1_keychange_interrupt_flag_clear(5);
		if(keychange5_callback)
            (*keychange5_callback)();
	}

	if((int_status&INT_K6) == INT_K6) {
		drv_l1_keychange_interrupt_flag_clear(6);
		if(keychange6_callback)
            (*keychange6_callback)();
	}

	if((int_status&INT_K7) == INT_K7) {
		drv_l1_keychange_interrupt_flag_clear(7);
		if(keychange7_callback)
            (*keychange7_callback)();
	}
}


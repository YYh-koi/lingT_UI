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
#include "drv_l1.h"

static const INT16S ROT_C_SIN_TABLE[] = {		//sine data table (0, 1,..., 359 degreed)
     0,     18,     36,     54,     71,     89,    107,    125,    143,    160,    178,    195,    213,    230,    248,
   265,    282,    299,    316,    333,    350,    367,    384,    400,    416,    433,    449,    465,    481,    496,
   512,    527,    543,    558,    573,    587,    602,    616,    630,    644,    658,    672,    685,    698,    711,
   724,    737,    749,    761,    773,    784,    796,    807,    818,    828,    839,    849,    859,    868,    878,
   887,    896,    904,    912,    920,    928,    935,    943,    949,    956,    962,    968,    974,    979,    984,
   989,    994,    998,   1002,   1005,   1008,   1011,   1014,   1016,   1018,   1020,   1022,   1023,   1023,   1024,
  1024,   1024,   1023,   1023,   1022,   1020,   1018,   1016,   1014,   1011,   1008,   1005,   1002,    998,    994,
   989,    984,    979,    974,    968,    962,    956,    949,    943,    935,    928,    920,    912,    904,    896,
   887,    878,    868,    859,    849,    839,    828,    818,    807,    796,    784,    773,    761,    749,    737,
   724,    711,    698,    685,    672,    658,    644,    630,    616,    602,    587,    573,    558,    543,    527,
   512,    496,    481,    465,    449,    433,    416,    400,    384,    367,    350,    333,    316,    299,    282,
   265,    248,    230,    213,    195,    178,    160,    143,    125,    107,     89,     71,     54,     36,     18,
     0,    -18,    -36,    -54,    -71,    -89,   -107,   -125,   -143,   -160,   -178,   -195,   -213,   -230,   -248,
  -265,   -282,   -299,   -316,   -333,   -350,   -367,   -384,   -400,   -416,   -433,   -449,   -465,   -481,   -496,
  -512,   -527,   -543,   -558,   -573,   -587,   -602,   -616,   -630,   -644,   -658,   -672,   -685,   -698,   -711,
  -724,   -737,   -749,   -761,   -773,   -784,   -796,   -807,   -818,   -828,   -839,   -849,   -859,   -868,   -878,
  -887,   -896,   -904,   -912,   -920,   -928,   -935,   -943,   -949,   -956,   -962,   -968,   -974,   -979,   -984,
  -989,   -994,   -998,  -1002,  -1005,  -1008,  -1011,  -1014,  -1016,  -1018,  -1020,  -1022,  -1023,  -1023,  -1024,
 -1024,  -1024,  -1023,  -1023,  -1022,  -1020,  -1018,  -1016,  -1014,  -1011,  -1008,  -1005,  -1002,   -998,   -994,
  -989,   -984,   -979,   -974,   -968,   -962,   -956,   -949,   -943,   -935,   -928,   -920,   -912,   -904,   -896,
  -887,   -878,   -868,   -859,   -849,   -839,   -828,   -818,   -807,   -796,   -784,   -773,   -761,   -749,   -737,
  -724,   -711,   -698,   -685,   -672,   -658,   -644,   -630,   -616,   -602,   -587,   -573,   -558,   -543,   -527,
  -512,   -496,   -481,   -465,   -449,   -433,   -416,   -400,   -384,   -367,   -350,   -333,   -316,   -299,   -282,
  -265,   -248,   -230,   -213,   -195,   -178,   -160,   -143,   -125,   -107,    -89,    -71,    -54,    -36,    -18
};

static const INT16S ROT_C_COS_TABLE[] = {		//cosine data table (0, 1,..., 359 degreed)
	  1024,   1024,   1023,   1023,   1022,   1020,   1018,   1016,   1014,   1011,   1008,   1005,   1002,    998,    994,
	   989,    984,    979,    974,    968,    962,    956,    949,    943,    935,    928,    920,    912,    904,    896,
	   887,    878,    868,    859,    849,    839,    828,    818,    807,    796,    784,    773,    761,    749,    737,
	   724,    711,    698,    685,    672,    658,    644,    630,    616,    602,    587,    573,    558,    543,    527,
	   512,    496,    481,    465,    449,    433,    416,    400,    384,    367,    350,    333,    316,    299,    282,
	   265,    248,    230,    213,    195,    178,    160,    143,    125,    107,     89,     71,     54,     36,     18,
	     0,    -18,    -36,    -54,    -71,    -89,   -107,   -125,   -143,   -160,   -178,   -195,   -213,   -230,   -248,
	  -265,   -282,   -299,   -316,   -333,   -350,   -367,   -384,   -400,   -416,   -433,   -449,   -465,   -481,   -496,
	  -512,   -527,   -543,   -558,   -573,   -587,   -602,   -616,   -630,   -644,   -658,   -672,   -685,   -698,   -711,
	  -724,   -737,   -749,   -761,   -773,   -784,   -796,   -807,   -818,   -828,   -839,   -849,   -859,   -868,   -878,
	  -887,   -896,   -904,   -912,   -920,   -928,   -935,   -943,   -949,   -956,   -962,   -968,   -974,   -979,   -984,
	  -989,   -994,   -998,  -1002,  -1005,  -1008,  -1011,  -1014,  -1016,  -1018,  -1020,  -1022,  -1023,  -1023,  -1024,
	 -1024,  -1024,  -1023,  -1023,  -1022,  -1020,  -1018,  -1016,  -1014,  -1011,  -1008,  -1005,  -1002,   -998,   -994,
	  -989,   -984,   -979,   -974,   -968,   -962,   -956,   -949,   -943,   -935,   -928,   -920,   -912,   -904,   -896,
	  -887,   -878,   -868,   -859,   -849,   -839,   -828,   -818,   -807,   -796,   -784,   -773,   -761,   -749,   -737,
	  -724,   -711,   -698,   -685,   -672,   -658,   -644,   -630,   -616,   -602,   -587,   -573,   -558,   -543,   -527,
	  -512,   -496,   -481,   -465,   -449,   -433,   -416,   -400,   -384,   -367,   -350,   -333,   -316,   -299,   -282,
	  -265,   -248,   -230,   -213,   -195,   -178,   -160,   -143,   -125,   -107,    -89,    -71,    -54,    -36,    -18,
	     0,     18,     36,     54,     71,     89,    107,    125,    143,    160,    178,    195,    213,    230,    248,
	   265,    282,    299,    316,    333,    350,    367,    384,    400,    416,    433,    449,    465,    481,    496,
	   512,    527,    543,    558,    573,    587,    602,    616,    630,    644,    658,    672,    685,    698,    711,
	   724,    737,    749,    761,    773,    784,    796,    807,    818,    828,    839,    849,    859,    868,    878,
	   887,    896,    904,    912,    920,    928,    935,    943,    949,    956,    962,    968,    974,    979,    984,
	   989,    994,    998,   1002,   1005,   1008,   1011,   1014,   1016,   1018,   1020,   1022,   1023,   1023,   1024
};

#define C_ROTATOR_QUEUE_SIZE		    5
#define GPIO_DEBUG_EN                   0
#define C_ROTATOR_END                   1
#define FUNCTION_REMOVE_EN              0

#if _OPERATING_SYSTEM == _OS_FREERTOS
#define SYSTEM_OS_ENABLE                1
#else
#define SYSTEM_OS_ENABLE                0
#endif
#define ROTATOR0_CLOCK_SOURCE           CLK_EN6_ROT//CLK_EN6_ROTATOR//CLK_EN2_M4ROTATOR
#define ROTATOR1_CLOCK_SOURCE           CLK_EN6_ROTATOR//CLK_EN3_NEWROTATOR
#define ROTATOR2_CLOCK_SOURCE           CLK_EN6_LB_ROTATOR//CLK_EN3_LINE_BASE_ROTATOR
#define ROTATOR2_RAM_CLOCK_SOURCE       CLK_EN4_LB_ROTATOR_SRAM//CLK_EN3_LINE_BASE_ROTATOR_RAM

#define ROTATOR_CLOCK_ENABLE            1
#define ROTATOR_CLOCK_DISABLE           0

#ifndef DISABLE
#define DISABLE                         0
#endif

#ifndef ENABLE
#define ENABLE                          1
#endif

/*
#ifdef NULL
#undef NULL
#endif
#define NULL                            0
*/

#define ROT_FIQ_EN                      0
#define ROT1_FIQ_NUM                    VIC_FIQ_1//VIC_FIQ_1//VIC_FIQ_2//VIC_FIQ_3//VIC_FIQ_4
#define ROT2_FIQ_NUM                    VIC_FIQ_2//VIC_FIQ_1//VIC_FIQ_2//VIC_FIQ_3//VIC_FIQ_4
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct
{										// Offset
	volatile INT32U	BUF_I_ADDR;         // 0x0000
	volatile INT32U	BUF_O_ADDR;		    // 0x0004
	volatile INT32U	IMG_WIDTH;			// 0x0008
	volatile INT32U	IMG_HEIGHT;			// 0x000C
	volatile INT32U	CTRL;			    // 0x0010
	volatile INT32U	START;			    // 0x0014
	volatile INT32U	IRQ_STATUS;			// 0x0018
	volatile INT32U	IRQ_EN;			    // 0x001C
	// only rotator1 use
	volatile INT32U	BUF_I_ADDR2;	    // 0x0020
	volatile INT32U	SOFT_RST;			// 0x0024
	volatile INT32U ROTHPTHR;           // 0x0028       // // only rotator2 use
	volatile INT32U	RESERVE0[1];        // 0x002C
	volatile INT32U	THETA_CTRL;			// 0x0030
	volatile INT32U	SIN_COS;			// 0x0034
	volatile INT32U	PST_MXT_11;	        // 0x0038
	volatile INT32U	PST_MXT_12;			// 0x003C
	volatile INT32U	PST_MXT_13;			// 0x0040
	volatile INT32U	PST_MXT_21;			// 0x0044
	volatile INT32U	PST_MXT_22;	        // 0x0048
	volatile INT32U	PST_MXT_23;			// 0x004C
	volatile INT32U	PST_MXT_31;			// 0x0050
	volatile INT32U	PST_MXT_32;			// 0x0054
	volatile INT32U	PST_MXT_33;			// 0x0058
	volatile INT32U	RESERVE1[1];        // 0x005C
	volatile INT32U	PST_IN_WIDTH;	    // 0x0060
	volatile INT32U	PST_IN_HEIGHT;	    // 0x0064
} ROTATOR_SFR;

typedef struct RotatorDev_s
{
	INT8U init_done;
	INT32U rotator_end;
	volatile INT32U interrupt_flag;
#if GPIO_DEBUG_EN == 1
    INT32U rotator_debug_pin = 0;
    INT32U rotator_end_counter = 0;
#endif
	osMessageQId message_queue;
	osMessageQId rotator_q;
	osSemaphoreId rotator_hw_sem;
} RotatorDev_t;

static INT32U rotator_dev_init_flag[ROTATOR_MAX] = {0};
static RotatorDev_t rotator_dev[ROTATOR_MAX];
static void (*rotator_isr_callback[ROTATOR_MAX])(INT8U rotator_number, INT32U rotator_event);

/**
 * @brief   rotator module clock set
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @param   enable[in]: 0:Disable/1:enable
 * @return 	none
 */
static INT16S drv_l1_rotator_clock_set(INT8U rotator_num, INT16U enable)
{
    INT16S ret = 0;

    if(rotator_num == ROTATOR_0)
    {
        return drv_l1_clock_set_system_clk_en(ROTATOR0_CLOCK_SOURCE, enable);
    }
    else if(rotator_num == ROTATOR_2)
    {
        ret = drv_l1_clock_set_system_clk_en(ROTATOR2_RAM_CLOCK_SOURCE, enable);
        if(ret)
            return ret;

        return drv_l1_clock_set_system_clk_en(ROTATOR2_CLOCK_SOURCE, enable);
    }
    else
    {
        return drv_l1_clock_set_system_clk_en(ROTATOR1_CLOCK_SOURCE, enable);
    }

    return 0;
}

/**
 * @brief   rotator sfr base address
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @return 	none
 */
static ROTATOR_SFR *_get_rotator_sfr_base(INT8U rotator_num)
{
#if ROTATOR0_EN == 1
	if (rotator_num == ROTATOR_0) {
		return (ROTATOR_SFR *)ROTATOR0_BASE;
	}
#endif

#if ROTATOR1_EN == 1
	if (rotator_num == ROTATOR_1) {
		return (ROTATOR_SFR *)ROTATOR1_BASE;
	}
#endif

#if ROTATOR2_EN == 1
	if (rotator_num == ROTATOR_2) {
		return (ROTATOR_SFR *)ROTATOR2_BASE;
	}
#endif

	return 0;
}

/**
 * @brief   rotator device information set
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @return 	none
 */
static RotatorDev_t *_get_rotator_device(INT8U rotator_num)
{
#if ROTATOR0_EN == 1
	if (rotator_num == ROTATOR_0) {
		return &rotator_dev[ROTATOR_0];
	}
#endif

#if ROTATOR1_EN == 1
	if (rotator_num == ROTATOR_1) {
		return &rotator_dev[ROTATOR_1];
	}
#endif

#if ROTATOR2_EN == 1
	if (rotator_num == ROTATOR_2) {
		return &rotator_dev[ROTATOR_2];
	}
#endif

	return 0;
}

/**
 * @brief   rotator osSemaphore lock
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @return 	none
 */
static void drv_l1_rotator_lock(INT8U rotator_num)
{
    RotatorDev_t *pRotatorDev = _get_rotator_device(rotator_num);

#if SYSTEM_OS_ENABLE == 1
    if(pRotatorDev->rotator_hw_sem)
        osSemaphoreWait(pRotatorDev->rotator_hw_sem, osWaitForever);
#endif
}

/**
 * @brief   rotator osSemaphore unlock
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @return 	none
 */
static void drv_l1_rotator_unlock(INT8U rotator_num)
{
    RotatorDev_t *pRotatorDev = _get_rotator_device(rotator_num);

#if SYSTEM_OS_ENABLE == 1
	if(pRotatorDev->rotator_hw_sem)
        osSemaphoreRelease(pRotatorDev->rotator_hw_sem);
#endif
}

/**
 * @brief   rotator isr function
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @return 	none
 */
static void rotator_isr(INT8U rotator_num)
{
    #define ISR_TIME_OUT_EN        1
    INT32U flag, event;
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);
    RotatorDev_t *pRotatorDev = _get_rotator_device(rotator_num);
#if 0//ROT_FIQ_EN == 1
    portBASE_TYPE taskWoken = pdFALSE;
#endif

    flag = pRotator_Reg->IRQ_STATUS;
    pRotator_Reg->IRQ_STATUS = flag;
    event = 0;

#if GPIO_DEBUG_EN == 1
    gpio_write_io(pRotatorDev->rotator_debug_pin, (pRotatorDev->rotator_end_counter++ & 0x1));
#endif
    if((flag & C_ROTATOR_END_STATE) && (pRotator_Reg->IRQ_EN && C_ROTATOR_END_INT_EN))
        event |= C_ROTATOR_END_STATE;

    if(rotator_num != ROTATOR_0)
    {
        if(flag & C_ROTATOR_SRC_FRAME_SEL_STATE)
            event |= C_ROTATOR_SRC_FRAME_SEL_STATE;
    }

#if SYSTEM_OS_ENABLE == 1
    if(pRotatorDev->rotator_q) {
        #if 0//ROT_FIQ_EN == 1
            xQueueSendFromISR(pRotatorDev->rotator_q, (const void *)&event, &taskWoken);
        #else
            #if ISR_TIME_OUT_EN == 1
                if(rotator_num == ROTATOR_2)
                {
                    if((pRotator_Reg->CTRL & ROT_DRAM_2_TFT_ENABLE) == 0)
                        osMessagePut(pRotatorDev->rotator_q, (uint32_t)&event, 1);
                }
                else
                    osMessagePut(pRotatorDev->rotator_q, (uint32_t)&event, 1);
            #else
                osMessagePut(pRotatorDev->rotator_q, (uint32_t)&event, osWaitForever);
            #endif
        #endif
    }
#else
    if((flag & C_ROTATOR_END_STATE) && (pRotator_Reg->IRQ_EN && C_ROTATOR_END_INT_EN))
        pRotatorDev->rotator_end = 0;
#endif

    pRotatorDev->interrupt_flag = event;

	if(rotator_isr_callback[rotator_num]) {
		INT32U rotator_flag = 0;
	#if ROTATOR0_EN == 1
		if(rotator_num == ROTATOR_0)
            rotator_flag = rotator_dev[ROTATOR_0].interrupt_flag;
	#endif
	#if ROTATOR1_EN == 1
		if(rotator_num == ROTATOR_1)
            rotator_flag = rotator_dev[ROTATOR_1].interrupt_flag;
	#endif
	#if ROTATOR2_EN == 1
		if(rotator_num == ROTATOR_2)
            rotator_flag = rotator_dev[ROTATOR_2].interrupt_flag;
	#endif
		(*rotator_isr_callback[rotator_num])(rotator_num, rotator_flag);
	}
}

/**
 * @brief   ROTATOR0 IRQ Handler
 * @param   none
 * @return 	none
 */
void ROTATOR0_IRQHandler(void)
{
    rotator_isr(ROTATOR_0);
}

/**
 * @brief   ROTATOR1 IRQ Handler
 * @param   none
 * @return 	none
 */
void ROTATOR1_IRQHandler(void)
{
    rotator_isr(ROTATOR_1);
}

/**
 * @brief   ROTATOR1 IRQ Handler
 * @param   none
 * @return 	none
 */
void ROTATOR2_IRQHandler(void)
{
    rotator_isr(ROTATOR_2);
}

/**
 * @brief   Initialize rotator controller
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @return 	none
 */
void drv_l1_rotator_init(INT8U rotator_num)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);
    RotatorDev_t *pRotatorDev = _get_rotator_device(rotator_num);

    // enable rotator clock
    drv_l1_rotator_clock_set(rotator_num, ROTATOR_CLOCK_ENABLE);

    // rotator device init
    drv_l1_rotator_software_reset_set(rotator_num);
    pRotator_Reg->CTRL = 0;
    pRotator_Reg->IRQ_STATUS = C_ROTATOR_MASK_STATE;	// Clear Rotator pending interrupts
    if(rotator_num == ROTATOR_0)
        pRotator_Reg->IRQ_EN = C_ROTATOR_END_INT_EN;
    else
	{
	#if 1
		pRotator_Reg->IRQ_EN = C_ROTATOR_END_INT_EN;
	#else
        pRotator_Reg->IRQ_EN = (C_ROTATOR_END_INT_EN | C_ROTATOR_WRITR_FRAME_END_INT_EN |
        C_ROTATOR_READ_FRAME_END_INT_EN | C_ROTATOR_LINE_BUFFER_END_INT_EN);
	#endif
	}

    if(rotator_dev_init_flag[rotator_num] == 0)
    {
        gp_memset((INT8S *)pRotatorDev, 0, sizeof(RotatorDev_t));
        rotator_dev_init_flag[rotator_num] = 1;
    }
    else
        pRotatorDev->interrupt_flag = 0;

#if GPIO_DEBUG_EN == 1
    if(rotator_num == ROTATOR_0)
        pRotatorDev->rotator_debug_pin = IO_D7;
    else
        pRotatorDev->rotator_debug_pin = IO_A12;
    gpio_init_io(pRotatorDev->rotator_debug_pin, GPIO_OUTPUT);
    gpio_set_port_attribute(pRotatorDev->rotator_debug_pin, 1);
#endif

    if(pRotatorDev->init_done == 0)
    {
        pRotatorDev->init_done = 1;
#if SYSTEM_OS_ENABLE == 1
        if(pRotatorDev->rotator_q == NULL) {
            osMessageQDef_t rot_q = { C_ROTATOR_QUEUE_SIZE, sizeof(INT32U), 0 };

            pRotatorDev->rotator_q = osMessageCreate(&rot_q, NULL);

            if(pRotatorDev->rotator_q == NULL)
                pRotatorDev->init_done = 0;
        }

        if(pRotatorDev->rotator_hw_sem == NULL) {
            osSemaphoreDef_t rotator_sem = { 0 };

            pRotatorDev->rotator_hw_sem = osSemaphoreCreate(&rotator_sem, 1);
            if(pRotatorDev->rotator_hw_sem == NULL)
                pRotatorDev->init_done = 0;
        }
#else
        pRotatorDev->rotator_end = 0;
#endif
    }

    rotator_isr_callback[rotator_num] = 0;

    if(rotator_num == ROTATOR_0)
    {
        vic_irq_register(VIC_ROT, ROTATOR0_IRQHandler);
        vic_irq_enable(VIC_ROT);
    #if ROT_FIQ_EN == 1
        vic_fiq_enable(VIC_ROT);
    #endif
    }
    else if(rotator_num == ROTATOR_1)
    {
        vic_irq_register(VIC_ROTATOR, ROTATOR1_IRQHandler);
        vic_irq_enable(VIC_ROTATOR);
    #if ROT_FIQ_EN == 1
        vic_fiq_enable(VIC_ROTATOR);
    #endif
    }
    else
    {
        vic_irq_register(VIC_ROTATOR_LB, ROTATOR2_IRQHandler);
        vic_irq_enable(VIC_ROTATOR_LB);
    #if ROT_FIQ_EN == 1
        vic_fiq_enable(VIC_ROTATOR_LB);
    #endif
    }
}

/**
 * @brief   Initialize rotator source image information
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @param   color[in]: IMAGE_YUYV/IMAGE_YVYU/IMAGE_UYVY/IMAGE_VYUY/IMAGE_RGB565/IMAGE_GP420
 * @param   w[in]: Image width
 * @param   h[in]: Image height
 * @param   in_buf[in]: Image Source Address
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
*/
INT32S drv_l1_rotator_src_img_info(INT8U rotator_num, INT8U color, INT16U w, INT16U h, INT32U in_buf)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);
    INT32U temp = pRotator_Reg->CTRL;

    if(!in_buf || !w || !h)
        return -1;

    // rotator0 limit
    if(rotator_num == ROTATOR_0)
    {
        if((w % 2) != 0 || (h % 2) != 0)
            return -1;

        if(w < 66 || h < 66)
        {
            if(w != 32 || h != 32 )
                return -1;
        }
        else
        {
            if(w > 4094 || h > 4094)
                return -1;
        }
    }

    // color mode setting
    if(rotator_num == ROTATOR_0)
    {
        if((color == IMAGE_Y_ONLY) || (color == IMAGE_GP420))
             return -1;
        else if(color == IMAGE_RGB565)
        {
             temp &= ~IMAGE_RGB565_ENABLE;
             pRotator_Reg->CTRL = temp;
        }
        else
        {
             temp |= IMAGE_YUV422_ENABLE;
             temp &= ~MASK_YUYV_ENABLE;
             temp |= (color << B_IMAGE_YUV422) & MASK_YUYV_ENABLE;
             pRotator_Reg->CTRL = temp;
        }
    }
    else
    {
        temp &= ~(IMAGE_ROT1_Y_ONLY_ENABLE | MASK_IMAGE_ROT1_COLOR_ENABLE);  // clear color mode bit and default GP420
        if(color == IMAGE_Y_ONLY)
        {
             temp |= IMAGE_ROT1_Y_ONLY_ENABLE;
             pRotator_Reg->CTRL = temp;
        }
        else if(color == IMAGE_RGB565)
        {
             temp |= IMAGE_ROT1_RGB565_ENABLE;
             pRotator_Reg->CTRL = temp;
        }
        else if(color < IMAGE_RGB565)
        {
             temp |= IMAGE_ROT1_YUV422_ENABLE;
             temp &= ~MASK_YUYV_ENABLE;
             temp |= (color << B_IMAGE_YUV422) & MASK_YUYV_ENABLE;
             pRotator_Reg->CTRL = temp;
        }
        else
            pRotator_Reg->CTRL = temp;
    }

    pRotator_Reg->IMG_HEIGHT = (h & MASK_IMG_RANGE);
    pRotator_Reg->IMG_WIDTH = (w & MASK_IMG_RANGE);
    pRotator_Reg->BUF_I_ADDR = in_buf;

    return 0;
}

/**
 * @brief   rotator target address set
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @param   out_buf[in]: image target address
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_tar_img_addr(INT8U rotator_num, INT32U out_buf)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);

    if(!out_buf)
        return -1;

    pRotator_Reg->BUF_O_ADDR = out_buf;

    return 0;
}


/**
 * @brief   rotator source address set
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @param   out_buf[in]: image source address
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_src_img_addr(INT8U rotator_num, INT32U out_buf)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);

    if(!out_buf)
        return -1;

    pRotator_Reg->BUF_I_ADDR = out_buf;
    //R_TV_FBI_ADDR = 0;

    return 0;
}

/**
 * @brief   rotator mode set
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @param   mode[in]: ROTATOR_90/ROTATOR_180/ROTATOR_270/ROTATOR_HORIZONTAL_MIRROR
 *          ROTATOR_VERTICAL_MIRROR/ROTATOR_270_VERTICAL_MIRROR/ROTATOR_90_HORIZONTAL_MIRROR/ROTATOR_THETA
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_mode_set(INT8U rotator_num, INT32U mode)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);
    INT32U temp = pRotator_Reg->CTRL;

    if(rotator_num == ROTATOR_0)
    {
        if(mode > ROTATOR_90_HORIZONTAL_MIRROR)
            return -1;
    }

    if(rotator_num == ROTATOR_1)
    {
        temp &= ~MASK_ROT_MODE;
        temp |= (mode << B_ROT_MODE) & MASK_ROT_MODE;
    }
    else
    {
        temp &= ~MASK_ROT0_MODE;
        temp |= (mode << B_ROT_MODE) & MASK_ROT0_MODE;
    }

    pRotator_Reg->CTRL = temp;

    if(mode == ROTATOR_PST)
    {
        //pRotator_Reg->CTRL &= ~(IMAGE_ROT1_Y_ONLY_ENABLE | MASK_IMAGE_ROT1_COLOR_ENABLE);  // clear color mode bit and default GP420
        //pRotator_Reg->CTRL |= IMAGE_ROT1_Y_ONLY_ENABLE;
    }

    return 0;
}

/**
 * @brief   rotator controller start
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_start(INT8U rotator_num)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);

#if SYSTEM_OS_ENABLE == 0
    pRotatorDev->rotator_end = 1;
#endif

    drv_l1_rotator_lock(rotator_num);
    drv_l1_rotator_enable_set(rotator_num, 1);
    drv_l1_rotator_software_reset_set(rotator_num);
    pRotator_Reg->START = 0x1;
    drv_l1_rotator_unlock(rotator_num);

    return 0;
}

/**
 * @brief   rotator state wait
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @param   wait[in]: 0: Return module status/1: Waiting for rotate module end
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_end_wait(INT8U rotator_num, INT32U wait)
{
#if 0
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);
#endif
    RotatorDev_t *pRotatorDev = _get_rotator_device(rotator_num);
#if SYSTEM_OS_ENABLE == 1
    osEvent result;
#endif
    INT32S state = -1;

    drv_l1_rotator_lock(rotator_num);
    if(wait)
    {
    #if 0
        if(rotator_num == ROTATOR_2)
        {
            while((pRotator_Reg->IRQ_STATUS & C_ROTATOR_END_STATE) == 0);
            pRotator_Reg->IRQ_STATUS = C_ROTATOR_END_STATE;
        }
        else
        {
#if SYSTEM_OS_ENABLE == 1
        result = osMessageGet(pRotatorDev->rotator_q, 5000);
        state = result.value.v;
#else
        while(pRotatorDev->rotator_end);
#endif
    }
    #else
#if SYSTEM_OS_ENABLE == 1
        result = osMessageGet(pRotatorDev->rotator_q, 5000);
        state = result.value.v;
#else
        while(pRotatorDev->rotator_end);
#endif
    #endif
    }
    else
    {
#if SYSTEM_OS_ENABLE == 1
        result = osMessageGet(pRotatorDev->rotator_q, 1);
        state = result.value.v;
#endif
    }
    drv_l1_rotator_unlock(rotator_num);

#if SYSTEM_OS_ENABLE == 0
    state = pRotatorDev->rotator_end;
#endif

    return state;
}

/**
 * @brief   Uninitialize rotator controller
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @return 	none
 */
void drv_l1_rotator_uninit(INT8U rotator_num)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);
    RotatorDev_t *pRotatorDev = _get_rotator_device(rotator_num);

    pRotator_Reg->CTRL = 0;
    pRotator_Reg->IRQ_STATUS = C_ROTATOR_MASK_STATE;// Clear Rotator pending interrupts
    pRotator_Reg->IRQ_EN = 0;

    if(pRotatorDev->init_done)
    {
        pRotatorDev->init_done = 0;
#if SYSTEM_OS_ENABLE == 1
		if(pRotatorDev->rotator_q) {
            xQueueReset(pRotatorDev->rotator_q);
            vQueueDelete(pRotatorDev->rotator_q);
            pRotatorDev->rotator_q = NULL;
		}

		if(pRotatorDev->rotator_hw_sem) {
            vQueueDelete(pRotatorDev->rotator_hw_sem);
            pRotatorDev->rotator_hw_sem = NULL;
		}
#else
        pRotatorDev->rotator_end = 0;
#endif
    }

    if(rotator_num == ROTATOR_0)
    {
    #if ROT_FIQ_EN == 1
        vic_fiq_disable(VIC_ROT);
    #endif
        vic_irq_disable(VIC_ROT);
    }
    else if(rotator_num == ROTATOR_1)
    {
    #if ROT_FIQ_EN == 1
        vic_fiq_disable(VIC_ROTATOR);
    #endif
        vic_irq_disable(VIC_ROTATOR);
    }
    else if(rotator_num == ROTATOR_2)
    {
    #if ROT_FIQ_EN == 1
        vic_fiq_disable(VIC_ROTATOR_LB);
    #endif
        vic_irq_disable(VIC_ROTATOR_LB);
    }

    drv_l1_rotator_clock_set(rotator_num, ROTATOR_CLOCK_DISABLE);
}

// rotator1/2 only
/**
 * @brief   rotator target address2 set
 * @param   rotator_num[in]: ROTATOR_1
 * @param   in_buf2[in]: buffer address
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_src_img_addr2(INT8U rotator_num, INT32U in_buf2)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);

    if(!in_buf2 || (rotator_num == ROTATOR_0))
        return -1;

    pRotator_Reg->BUF_I_ADDR2 = in_buf2;

    return 0;
}

/**
 * @brief   rotator ab buffer mode set
 * @param   rotator_num[in]: ROTATOR_1
 * @param   enable[in]: 0: disable/1: enable
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_ab_buf_set(INT8U rotator_num, INT32U enable)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);

    if(rotator_num == ROTATOR_0)
        return -1;

    if(enable)
        pRotator_Reg->CTRL |= AB_FRAME_BUFFER_ENABLE;
    else
        pRotator_Reg->CTRL &= ~AB_FRAME_BUFFER_ENABLE;

    return 0;
}

/**
 * @brief   rotator controller enable
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_enable_set(INT8U rotator_num, INT32U enable)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);

    if(rotator_num == ROTATOR_0)
        return -1;

    //R_SYSTEM_MISC_CTRL4 |= 0x111;
    if(rotator_num == ROTATOR_2)
        R_SYSTEM_MISC_CTRL8 &= ~ROT_IRAM_SHARE_ENABLE;

    if(enable)
        pRotator_Reg->CTRL |= ROTATOR_ENABLE;
    else
        pRotator_Reg->CTRL &= ~ROTATOR_ENABLE;

    return 0;
}

/**
 * @brief   rotator auto high priority
 * @param   rotator_num[in]: ROTATOR_1
 * @param   enable[in]: 0: disable/1: enable
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_auto_high_priority_set(INT8U rotator_num, INT32U enable)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);

    if(rotator_num == ROTATOR_0)
        return -1;

    if(enable)
        pRotator_Reg->CTRL |= AUTO_HIGH_PRIORITY_ENABLE;
    else
        pRotator_Reg->CTRL &= ~AUTO_HIGH_PRIORITY_ENABLE;

    return 0;
}

INT32S drv_l1_rotator_burst16_set(INT8U rotator_num, INT32U enable)
{
#if FUNCTION_REMOVE_EN == 1
    return -1;
#else
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);

    if(rotator_num != ROTATOR_2)
        return -1;

    if(enable)
        pRotator_Reg->CTRL |= BURST16_ENABLE;
    else
        pRotator_Reg->CTRL &= ~BURST16_ENABLE;

    return 0;
#endif
}

/**
 * @brief   rotator theta mode line buffer set
 * @param   rotator_num[in]: ROTATOR_1
 * @param   line_number[in]: 0: 1Line/1: 2Lines/2: 4Lines/3: 8Lines
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_non_right_angle_line_buffer_number_set(INT8U rotator_num, INT32U line_number)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);
    INT32U temp = pRotator_Reg->CTRL;

    //if(rotator_num == ROTATOR_0)
    if(rotator_num != ROTATOR_2)
        return -1;

    temp &= ~MASK_RIGHT_ANGLE_LINE_BUF_NUM_MODE;
    temp |= (line_number << B_NON_RIGHT_ANGLE_LINE_BUF_NUM) & MASK_RIGHT_ANGLE_LINE_BUF_NUM_MODE;
    pRotator_Reg->CTRL = temp;

    return 0;
}

/**
 * @brief   rotator controller reset
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_software_reset_set(INT8U rotator_num)
{
    #define RESET_TIMEOUT_EN            0
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);
#if RESET_TIMEOUT_EN == 1
    INT32U i;
#endif

    if(rotator_num == ROTATOR_0)
        return -1;

    pRotator_Reg->SOFT_RST = C_ROTATOR_RESET;
#if RESET_TIMEOUT_EN == 1
    for(i=0;i<100;i++)
    {
        if(pRotator_Reg->SOFT_RST == 0)
            break;
    }
#else
    while(pRotator_Reg->SOFT_RST);
#endif

    return 0;
}

/**
 * @brief   rotator theta mode source image information
 * @param   rotator_num[in]: ROTATOR1_THETA_WRITE_BURST16/ROTATOR1_THETA_WRITE_BURST8/ROTATOR1_THETA_WRITE_BURST4
 * @param  burst_mode[in]: 0:GP420/1:Y Only
 * @param   in_color_mode[in]: 0:GP420/1:Y Only
 * @param   clockwise_mode[in]: 0: clockwise/1: counterclockwise
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_theta_info_set(INT8U rotator_num, INT32U burst_mode, INT32U in_color_mode, INT32U clockwise_mode)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);
    INT32U temp;

    if(rotator_num == ROTATOR_0)
        return -1;

    temp = pRotator_Reg->THETA_CTRL;
    temp &= ~(MASK_THETA_BUR_W_SEL_MODE | Y_ONLY_IN_ENABLE | COUNTER_CLOCKWISE_ENABLE);
    temp |= (burst_mode << B_THETA_BUR_W_SEL) & MASK_THETA_BUR_W_SEL_MODE;

    if(in_color_mode)
        temp |= Y_ONLY_IN_ENABLE;

    if(clockwise_mode)
        temp |= COUNTER_CLOCKWISE_ENABLE;

    pRotator_Reg->THETA_CTRL = temp;

    return 0;
}

/**
 * @brief   rotator theta angle set
 * @param   rotator_num[in]: ROTATOR_1
 * @param   angle[in]: angle 0~359
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_angle_set(INT8U rotator_num, INT16S angle)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);
	INT16U r_sine, r_cosine;
	INT32S value;

    if(rotator_num == ROTATOR_0)
        return -1;

    if(angle < 0 || angle >= 360)
        return -1;

    value = (INT32S)ROT_C_SIN_TABLE[angle];
    r_sine = (INT16U)(value * 16)+1;

    value = (INT32S)ROT_C_COS_TABLE[angle];
    r_cosine = (INT16U)(value * 16)+1;

    pRotator_Reg ->SIN_COS = ((r_cosine << 16) | r_sine);

	return 0;
}

/**
 * @brief   rotator cache set
 * @param   rotator_num[in]: ROTATOR_1
 * @param   cache_on[in]: 0:disable, 1:enable
 * @param   cache_flush[in]: 0:disable, 1:enable
 * @param   block_mode[in]: 0:disable, 1:enable
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_cache_set(INT8U rotator_num, INT32U cache_on, INT32U cache_flush, INT32U block_mode)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);

    if(rotator_num == ROTATOR_0)
        return -1;

    pRotator_Reg->THETA_CTRL &= ~(CACHE_FLUSH_MODE_ENABLE | CACHE_MODE_ENABLE | BLOCK_MODE_ENABLE);


    if(block_mode)
    {
        INT32U mode = (pRotator_Reg->CTRL & MASK_ROT_MODE);

        if(mode > ROTATOR_90_HORIZONTAL_MIRROR)
        {
            if(((pRotator_Reg->IMG_WIDTH > 16) && (pRotator_Reg->IMG_HEIGHT > 16)) &&
            ((pRotator_Reg->IMG_WIDTH % 16) == 0) && ((pRotator_Reg->IMG_HEIGHT % 16) == 0))
            {
                pRotator_Reg->THETA_CTRL &= ~(MASK_THETA_BUR_W_SEL_MODE);
                pRotator_Reg->THETA_CTRL |= BLOCK_MODE_ENABLE;
                pRotator_Reg->THETA_CTRL |= (ROTATOR1_THETA_WRITE_BURST4 << B_THETA_BUR_W_SEL) & MASK_THETA_BUR_W_SEL_MODE;
            }
        }
    }

    if(cache_on)
        pRotator_Reg->THETA_CTRL |= CACHE_MODE_ENABLE;

    if(cache_flush)
        pRotator_Reg->THETA_CTRL |= CACHE_FLUSH_MODE_ENABLE;

	return 0;
}

/**
 * @brief   rotator perspective transformation set
 * @param   rotator_num[in]: ROTATOR_1
 * @param   PST_MXT[in]: perspective transformation matrix
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_perspective_set(INT8U rotator_num, INT32U in_h, INT32U in_v, PERSPECTIVE_MATRIX_CFG *PST_MXT)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);

    if((rotator_num == ROTATOR_0) || (PST_MXT == 0))
        return -1;

	pRotator_Reg->PST_MXT_11 = PST_MXT->PST_MXT_11;
    pRotator_Reg->PST_MXT_12 = PST_MXT->PST_MXT_12;
    pRotator_Reg->PST_MXT_13 = PST_MXT->PST_MXT_13;
    pRotator_Reg->PST_MXT_21 = PST_MXT->PST_MXT_21;
    pRotator_Reg->PST_MXT_22 = PST_MXT->PST_MXT_22;
    pRotator_Reg->PST_MXT_23 = PST_MXT->PST_MXT_23;
    pRotator_Reg->PST_MXT_31 = PST_MXT->PST_MXT_31;
    pRotator_Reg->PST_MXT_32 = PST_MXT->PST_MXT_32;
    pRotator_Reg->PST_MXT_33 = PST_MXT->PST_MXT_33;
    pRotator_Reg->PST_IN_WIDTH = in_h;
    pRotator_Reg->PST_IN_HEIGHT = in_v;

	return 0;
}

/**
 * @brief   rotator isr callback set
 * @param   user_isr_callback[in]: user isr callback function
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_isr_callback_set(INT8U rotator_num, void (*user_isr_callback)(INT8U rotator_number, INT32U rotator_event))
{
    rotator_isr_callback[rotator_num] = user_isr_callback;

    return 0;
}

// rotator2 only
/**
 * @brief   rotator2 PPU GP420 enable
 * @param   rotator_num[in]: ROTATOR_2
 * @param   enable[in]: 0: disable/1: enable
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_ppu420_set(INT8U rotator_num, INT32U enable)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);

    if(rotator_num != ROTATOR_2)
        return -1;

    if(enable)
        pRotator_Reg->CTRL |= ROT_PPU420_ENABLE;
    else
        pRotator_Reg->CTRL &= ~ROT_PPU420_ENABLE;

    return 0;
}

/**
 * @brief   rotator2 DRAM 2 TFT enable
 * @param   rotator_num[in]: ROTATOR_2
 * @param   enable[in]: 0: disable/1: enable
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l1_rotator_dram2tft_set(INT8U rotator_num, INT32U enable)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);

    if(rotator_num != ROTATOR_2)
        return -1;

    if(enable)
    {
        R_SYSTEM_MISC_CTRL8 &= ~ROT_IRAM_SHARE_ENABLE;
        R_SYSTEM_MISC_CTRL4 |=  ROT_TO_TFT2_MODE_ENABLE;
        pRotator_Reg->CTRL |= ROT_DRAM_2_TFT_ENABLE;
    }
    else
    {
        R_SYSTEM_MISC_CTRL4 &= ~ROT_TO_TFT2_MODE_ENABLE;
        pRotator_Reg->CTRL &= ~ROT_DRAM_2_TFT_ENABLE;
    }

    return 0;
}

INT32S drv_l1_rotator_high_priority_set(INT8U rotator_num, INT8U enable, INT8U level)
{
    ROTATOR_SFR *pRotator_Reg = _get_rotator_sfr_base(rotator_num);

    if(rotator_num != ROTATOR_2)
        return -1;

    if(enable)
    {
        pRotator_Reg->ROTHPTHR |= ((enable & 0x01) << 12) | ((level & 0x0F) << 28) ;
    }
    else
    {
        pRotator_Reg->ROTHPTHR &= ~(1 << 12);
    }

    return 0;
}

// M4 Rotator API
void rotator_init(void)
{
    drv_l1_rotator_init(ROTATOR_0);
}

INT32S rotator_src_img_info(INT8U color, INT16U w, INT16U h, INT32U in_buf)
{
    return drv_l1_rotator_src_img_info(ROTATOR_0, color, w, h, in_buf);
}

INT32S rotator_tar_img_addr(INT32U out_buf)
{
    return drv_l1_rotator_tar_img_addr(ROTATOR_0, out_buf);
}

INT32S rotator_mode_set(INT32U mode)
{
    return drv_l1_rotator_mode_set(ROTATOR_0, mode);
}

INT32S rotator_start(void)
{
    return drv_l1_rotator_start(ROTATOR_0);
}

INT32S rotator_end_wait(INT32U wait)
{
    return drv_l1_rotator_end_wait(ROTATOR_0, wait);
}

void rotator_uninit(void)
{
    drv_l1_rotator_uninit(ROTATOR_0);
}


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
#include <stdio.h>
#include <string.h>
#include "project.h"
#include "gplib_print_string.h"
#include "drv_l1_timer.h"
#include "drv_l1_sdc.h"
#include "drv_l1_gpio.h"
#include "drv_l1_sdc_version.h"
#include "cmsis_os.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_dma.h"
#include "drv_l1_system.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define C_SDC_SECTOR_SIZE						512

#if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
#define SD_TIME_UNIT    10
#else
#define SD_TIME_UNIT    1000
#endif

#define SDC_INT_ENABLE  0

#define C_SDC_L1_CARD_INIT_74_CYCLES_TIMEOUT	(300*SD_TIME_UNIT)*2
#define C_SDC_L1_CONTROLLER_BUSY_TIMEOUT		(1800*SD_TIME_UNIT)*2
#define C_SDC_L1_CONTROLLER_STOP_TIMEOUT		(300*SD_TIME_UNIT)*2
#define C_SDC_L1_DATA_FULL_TIMEOUT				(54000*SD_TIME_UNIT)*2
#define C_SDC_L1_DATA_EMPTY_TIMEOUT				(70000*SD_TIME_UNIT)*2

const INT8U DRVL1_SD[] = "GLB_GP-S2_0610L_SD-L1-CS_3.0.2";

#define K_SD_READ_TIMEOUT_PER_SECTOR	105 // ms unit, should be 100, add more 5 for safe.
											// although sd read spec timeout per sector is 100ms, use 105ms, because 326XXB hardware is 104.5 ms
#define K_SD_WRITE_TIMEOUT_PER_SECTOR   250
#define K_SD_WRITE_DMA_BULK_SECTOR		8 // use 8 because 8 x 250ms = 2s timeout, so that user dont wait so long
#define K_SD_READ_DMA_BULK_SECTOR		16 // use 16 because 16 x 100ms = 1.6s timeout, so that user dont wait so long
#define sd_polling_1s                   12500

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void SDC0_IRQHandler(void);
static void SDC1_IRQHandler(void);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
QueueHandle_t   sd_dma_wait_queue = NULL;
static INT8S sd_dma_wait_msg;

static INT8U sd_card_access_kind[2] = {0, 0};
static INT8U sd_card_print_status_fail_flag[2] = {0, 0};
static INT16U sd_card_read_over_spec_sector_timeout[2] = {0, 0};
static sd_card_present_check_callback sd_card_present_check_proc[2] = {NULL, NULL};
volatile INT8S sddma_notify = C_DMA_STATUS_DONE;
static INT8S *g_polling_status;
INT32U SD3MCLK;
extern uint32_t SystemCoreClock;

/**
 * @brief   Assign checking card present call back function.
 * @param   device_id[in]: device id.
 * @param   user_card_present_check_proc[in]: check card present call back function pointer.
 * @return 	0:success/-1:fail.
*/
INT32S drvl1_sdc_card_present_check_callback_set(INT32U device_id, sd_card_present_check_callback user_card_present_check_proc)
{
    if (device_id > 1)
        return -1;

    sd_card_present_check_proc[device_id] = user_card_present_check_proc;

    return 0;
}

/**
 * @brief   Get checking card present call back function.
 * @param   device_id[in]: device id.
 * @return 	function pointer:success/NULL:none.
*/
sd_card_present_check_callback drvl1_sdc_card_present_check_callback_get(INT32U device_id)
{
    if (device_id > 1)
        return NULL;

    return sd_card_present_check_proc[device_id];
}

/**
 * @brief   Check whether card present or not.
 * @param   device_id[in]: device id.
 * @return 	1:success/0:fail.
*/
INT8U drvl1_sdc_check_card_present(INT32U device_id)
{
    INT8U present = 1; // inited to inserted

    sd_card_present_check_callback sd_card_present_check_proc;

    if (device_id > 1)
        return 0; // return removed

    sd_card_present_check_proc = drvl1_sdc_card_present_check_callback_get(device_id);
    if (sd_card_present_check_proc != NULL)
    {
        present = (*sd_card_present_check_proc)(device_id);
        if (present == 0)
            drvl1_sdc_clear_timeout_bit(device_id, 10); // this is not timeout bit bug, so clear it
    }

    return present;
}

/**
 * @brief   Enable/disable printing fail status flag.
 * @param   device_id[in]: device id.
 * @param   enable[in]: enable/disable.
 * @return 	none.
*/
void drvl1_sdc_set_print_status_fail_flag(INT32U device_id, INT8U enable)
{
	if (device_id > 1)
    {
        DBG_PRINT("illegal device id=%u\r\n", device_id);
        return;
    }
	sd_card_print_status_fail_flag[device_id] = enable;
}

/**
 * @brief   Set access kind
 * @param   device_id[in]: device id.
 * @param   kind[in]: read or write access
 * @return 	none
*/
void drvl1_sdc_set_access_kind(INT32U device_id, INT8U kind) // for debug purpose
{
    if (device_id > 1)
    {
        DBG_PRINT("illegal device id=%u\r\n", device_id);
        return;
    }
    sd_card_access_kind[device_id] = kind;
}

/**
 * @brief   Calculate write timeout
 * @param   sector_count[in]: write sector count
 * @return 	timeout value
*/
INT32U drvl1_sdc_cal_write_timeout(INT32U sector_count)
{
	INT32U timeout;

	#if 0
	if (sector_count < 256) {
		timeout = 192; // according to real time to many cards, found even wrste less sector, timeout at this value
	} else if (sector_count < 1000) {
		INT32U timeout = (sector_count >> 1);

		if (timeout > 255)
			timeout = 255; // max timeout is 255
		else if (timeout < 192)
			timeout = 192; // according to real time to manay cards, found even wrste less sector, timeout at this value
		timeout = (INT8U) timeout;
	} else {
		timeout = 255;
	}
	#else
		timeout = 255; // remark to use 0 instead of 255, explain below.
		//timeout = 0; // since our dma longest timeout is 1s, but disccover some sd card require more than 1 1s
		             // so set to 0 to disable dma timeout but use os timer to timeout on 1.5s
	#endif

	return  timeout;
}

/**
 * @brief   Calculate read timeout
 * @param   sector_count[in]: read sector count
 * @return 	timeout value
*/
INT32U drvl1_sdc_cal_read_timeout(INT32U sector_count)
{
	INT32U timeout;

	if (sector_count < 256) {
		timeout = 64;
	} else if (sector_count < 1000) {
		timeout = (INT8U) (sector_count >> 2);
	} else {
		timeout = 255;
	}
	return timeout;
}

// IMPORTANT, dont let 2 function loop similar in one file, compiler (such as GCC)
// will do unexpected code optimization cause code execution not as expected!
// Such as drvl1_sdc_status_set_wait, drvl1_sdc_status_clear_wait
/**
 * @brief   Polling sd status by a timeout value.
 * @param   device_id[in]: device id.
 * @param   pollbit[in]: polling bit.
 * @param   timeout[in]: setting timeout value.
 * @return 	0: success, -1: timeout or error status happened.
*/
INT32S drvl1_sdc_status_set_wait(INT32U device_id, INT32U pollbit, INT32U in_timeout)
{
    volatile INT32U timeout = in_timeout;
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

#if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
	volatile INT32U timer1, timer2;

	if (sd->Status & pollbit) {
		return 0;
	}

	timer1 = tiny_counter_get();
	while (timeout) {

#ifdef GPA7_AUTHENTICATION
{
	// Check patch here
    INT32U patch_id;

    patch_id = *((volatile INT32U *) 0xC0150008);
    if ((patch_id & 0x00000002) != 0x00000002)
        return -22;
}
#endif

        if (sd->Status & pollbit) {
            //DBG_PRINT("drvl1_sdc_status_set_wait success=%u, pollbit=0x%0x, timeout=%u, Status=0x%08x\r\n", in_timeout, pollbit, timeout, sd->Status);
            return 0;
        }

        if (sd->Status & (C_SDC_STATUS_RESP_INDEX_ERR|C_SDC_STATUS_RESP_CRC_ERR|C_SDC_STATUS_TIMEOUT))
        {
			if (sd_card_print_status_fail_flag[device_id])
			{
				DBG_PRINT("drvl1_sdc_status_set_wait cmd=0x%08x arg=0x%08x kind=%u\r\n", sd->Cmd, sd->Arg, sd_card_access_kind[device_id]);
				DBG_PRINT("drvl1_sdc_status_set_wait timeoutbit=%u, pollbit=0x%0x, timeout=%u, Status=0x%08x\r\n", in_timeout, pollbit, timeout, sd->Status);
            }
			return -1;
        }

		timer2 = tiny_counter_get();
		if (timer1 != timer2) {
			timer1 = timer2;
			timeout--;
		}
	}

	if (sd_card_print_status_fail_flag[device_id])
	{
		DBG_PRINT("drvl1_sdc_status_set_wait timeout=%u, pollbit=0x%0x, timeout=%u, Status=0x%08x\r\n", in_timeout, pollbit, sd->Status);
	}
	return -1;
#else
	INT32U i;

	i = 0;
	while ((sd->Status & pollbit) == 0) {
		i++;
		if (i > (timeout<<8)) {
			return -1;
		}
	}

	return 0;
#endif
}

/**
 * @brief   Polling sd3.0 status by a timeout value.
 * @param   device_id[in]: device id.
 * @param   pollbit[in]: polling bit.
 * @param   timeout[in]: setting timeout value.
 * @return 	0: success, -1: timeout or error status happened.
*/
INT32S drvl1_sdc30_status_set_wait(INT32U device_id, INT32U pollbit, INT32U in_timeout)
{
    volatile INT32U timeout = in_timeout;
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    if(device_id != 1)
        return -1;

#if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
	volatile INT32U timer1, timer2;

	if (sd->Status & pollbit) {
		return 0;
	}

	timer1 = tiny_counter_get();
	while (timeout) {

#ifdef GPA7_AUTHENTICATION
{
	// Check patch here
    INT32U patch_id;

    patch_id = *((volatile INT32U *) 0xC0150008);
    if ((patch_id & 0x00000002) != 0x00000002)
        return -22;
}
#endif

        if (sd->Status & pollbit) {
            //DBG_PRINT("drvl1_sdc_status_set_wait success=%u, pollbit=0x%0x, timeout=%u, Status=0x%08x\r\n", in_timeout, pollbit, timeout, sd->Status);
            return 0;
        }

        /* if (in_timeout - timeout >= in_timeout/2)*/ {
        if (sd->Status & (C_SDC_STATUS_RESP_INDEX_ERR|C_SDC_STATUS_RESP_CRC_ERR|C_SDC_STATUS_TIMEOUT))
        {
			if (sd_card_print_status_fail_flag[device_id])
			{
				DBG_PRINT("drvl1_sdc30_status_set_wait cmd=0x%08x arg=0x%08x kind=%u\r\n", sd->Cmd, sd->Arg, sd_card_access_kind[device_id]);
				DBG_PRINT("drvl1_sdc30_status_set_wait timeoutbit=%u, pollbit=0x%0x, timeout=%u, Status=0x%08x\r\n", in_timeout, pollbit, timeout, sd->Status);
            }
			return -1;
        }
        }

		timer2 = tiny_counter_get();
		if (timer1 != timer2) {
			timer1 = timer2;
			timeout--;
		}
	}

	if (sd_card_print_status_fail_flag[device_id])
	{
		DBG_PRINT("drvl1_sdc30_status_set_wait timeout=%u, pollbit=0x%0x, timeout=%u, Status=0x%08x\r\n", in_timeout, pollbit, timeout, sd->Status);
	}
	return -1;
#else
	INT32U i;

	i = 0;
	while ((sd->Status & pollbit) == 0) {
		i++;
		if (i > (timeout<<8)) {
			return -1;
		}
	}

	return 0;
#endif
}

/**
 * @brief   Waiting sd status clear by a timeout value
 * @param   device_id[in]: device id.
 * @param   pollbit[in]: polling bit.
 * @param   timeout[in]: setting timeout value.
 * @return 	0: success, -1: timeout.
*/
INT32S drvl1_sdc_status_clear_wait(INT32U device_id, INT32U pollbit, INT32U in_timeout)
{
    INT32U timeout = in_timeout; // IMPORTANT, no volatile here to avoid same with drvl1_sdc_status_set_wait
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

#ifdef GPA7_AUTHENTICATION
{
    INT32U patch_id;

    patch_id = *((volatile INT32U *) 0xC0150008);
    if ((patch_id & 0x00000002) != 0x00000002)
        return -22;
}
#endif

#if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
	INT32U timer3, timer4;

	if (!(sd->Status & pollbit)) {
		return 0;
	}

	timer3 = tiny_counter_get();
	while (timeout) {
		if (!(sd->Status & pollbit)) {
			return 0;
		}

		timer4 = tiny_counter_get();
		if (timer3 != timer4) {
			timer3 = timer4;
			timeout--;
		}
	}

    //DBG_PRINT("drvl1_sdc_status_clear_wait timeout=%u, pollbit=0x%0x, Status=0x%08x\r\n", in_timeout, pollbit, sd->Status);

	return -1;

#else
	INT32U i;

	i = 0;
	while (sd->Status & pollbit) {
		i++;
		if (i > (timeout<<8)) {
			return -1;
		}
	}

	return 0;
#endif
}

/**
 * @brief   Waiting sd3.0 status clear by a timeout value
 * @param   device_id[in]: device id.
 * @param   pollbit[in]: polling bit.
 * @param   timeout[in]: setting timeout value.
 * @return 	0: success, -1: timeout.
*/
INT32S drvl1_sdc30_status_clear_wait(INT32U device_id, INT32U pollbit, INT32U in_timeout)
{
    INT32U timeout = in_timeout; // IMPORTANT, no volatile here to avoid same with drvl1_sdc_status_set_wait
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	if(device_id != 1)
	{
        return -1;
	}

#ifdef GPA7_AUTHENTICATION
{
    INT32U patch_id;

    patch_id = *((volatile INT32U *) 0xC0150008);
    if ((patch_id & 0x00000002) != 0x00000002)
        return -22;
}
#endif

#if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
	INT32U timer3, timer4;

	if (!(sd->Status & pollbit)) {
		return 0;
	}

	timer3 = tiny_counter_get();
	while (timeout) {
		if (!(sd->Status & pollbit)) {
			return 0;
		}

		timer4 = tiny_counter_get();
		if (timer3 != timer4) {
			timer3 = timer4;
			timeout--;
		}
	}

    //DBG_PRINT("drvl1_sdc_status_clear_wait timeout=%u, pollbit=0x%0x, Status=0x%08x\r\n", in_timeout, pollbit, sd->Status);

	return -1;

#else
	INT32U i;

	i = 0;
	while (sd->Status & pollbit) {
		i++;
		if (i > (timeout<<8)) {
			return -1;
		}
	}

	return 0;
#endif
}


/**
 * @brief   Setting sd host timeout value.
 * @param   device_id[in]: device id.
 * @param   enable[in]: enable/disable sd host timeout.
 * @param   ms[in]: timeout value in milisecond.
 * @return 	none.
*/
void drvl1_sdc_set_timeout(INT32U device_id, INT8U enable, INT32U ms)
{
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
    INT32U sd_to_en = sd->TimeoutEn;
    sd_to_en &= ~0x7;
    sd_to_en |= enable&0x7;
    sd->TimeoutEn = sd_to_en;

    sd->TimeoutTHR = ((SystemCoreClock/65536)*ms)/1000;
}

/**
 * @brief   Setting sd host command timeout value.
 * @param   device_id[in]: device id.
 * @param   enable[in]: enable/disable sd host command timeout.
 * @return 	none.
*/
void drvl1_sdc_set_cmd_timeout(INT32U device_id, INT8U enable)
{
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
    if (device_id == 0) {
        if(enable)
            sd->TimeoutEn |= 0x1;
        else
            sd->TimeoutEn &= ~0x1;
    }
    else {
        sd30Reg_t *sd30 = (sd30Reg_t *)sd;
        if(enable)
            sd30->Ctrl2 |= (1<<22);
        else
            sd30->Ctrl2 &= ~(1<<22);
    }
}

/**
 * @brief   Setting sd host data timeout value.
 * @param   device_id[in]: device id.
 * @param   enable[in]: enable/disable sd host data timeout.
 * @return 	none.
*/
void drvl1_sdc_set_data_timeout(INT32U device_id, INT8U enable)
{
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
    if (device_id == 0) {
        if(enable)
            sd->TimeoutEn |= 0x2;
        else
            sd->TimeoutEn &= ~0x2;

    }
    else {
        sd30Reg_t *sd30 = (sd30Reg_t *)sd;
        if(enable)
            sd30->Ctrl2 |= (1<<23);
        else
            sd30->Ctrl2 &= ~(1<<23);
    }
}

/**
 * @brief   Forces sd host clock at low or be controlled by sd host controller.
 * @param   device_id[in]: device id.
 * @param   enable[in]: enable/disable forcing sd clock at low.
 * @return 	none.
*/
void drv_l1_sdc30_clklow(INT32U device_id, INT8U EN)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    if(EN)
        sd->Ctrl2 |= SDC_ENABLE_CLKLOW;
    else
        sd->Ctrl2 &= SDC_DISABLE_CLKLOW;
}


/**
 * @brief   Enable SD3.0 low drop out regulator and select 3.3v or 1.8v output voltage.
 * @param   device_id[in]: device id.
 * @param   v33[in]: select 3.3v or 1.8v output voltage.
 * @return 	none.
*/
void drvl1_sdcbus_ldo_pwr_sel(INT32U device_id, uint8_t v33)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    if(v33)
    {
        //enable sd30 LDO and set as 3.3v
        R_SYSTEM_MISC_CTRL0 &= ~0x680;
        R_SYSTEM_MISC_CTRL0 |= 0x680;
        while(!(R_SYSTEM_MISC_CTRL0 & 0x80));
        if (device_id != 0)
            sd->Ctrl &= (INT32U)(~(1<<31));
    }
    else
    {
        R_SYSTEM_MISC_CTRL0 &= ~0x680;
        R_SYSTEM_MISC_CTRL0 |= 0x080;
        while(!(R_SYSTEM_MISC_CTRL0 & 0x80));
        if (device_id != 0)
            sd->Ctrl |= (INT32U)(1<<31);
    }
}



/**
 * @brief   Control the IO pad of SD interface to 1.8v mode.
 * @param   device_id[in]: device id.
 * @return 	none.
*/
void drv_l1_sdc30_switch_voltage18(INT32U device_id)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    sd->Ctrl |= (INT32U)(1<<31);
    drvl1_sdcbus_ldo_pwr_sel(device_id, SD_V18);
    osDelay(22);
}

/**
 * @brief   Control the IO pad of SD interface to 3.3v mode.
 * @param   device_id[in]: device id.
 * @return 	none.
*/
void drv_l1_sdc30_switch_voltage33(INT32U device_id)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    sd->Ctrl &= (INT32U)(~(1UL<<31));
    drvl1_sdcbus_ldo_pwr_sel(device_id, SD_V33);
    osDelay(22);
}

void drv_l1_sdc30host_reset_2_idle(INT32U device_id) {
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    sd->Cmd &= ~(1<<6);
    sd->Ctrl = ((sd->Ctrl & ~0xFF) | 0x54);
    sd->Ctrl2 &= ~(0xF0000);
}

static INT32U save_SDC_CTL_COP[2] = {0, 0};

/**
 * @brief   SD host controller initialization.
 * @param   device_id[in]: device id.
 * @param   vic_num[in]: using irq or fiq
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_init(INT32U device_id, INT8U vic_num)
{
	INT32S ret;
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	ret = 0;

	save_SDC_CTL_COP[device_id] = sd->Ctrl & C_SDC_CTL_COP_MASK;
    drvl1_sdcbus_ldo_pwr_sel(device_id, SD_V33);
	sd->Status = C_SDC_STATUS_CLEAR_ALL;		// Clear all status bits

	// Force SD controller to stop
	if ((sd->Status & C_SDC_STATUS_CONTROLLER_BUSY) && (sd->Ctrl & C_SDC_CTL_ENABLE)) {
        if(device_id == 0){
            ret = drvl1_sdc_stop_controller(device_id, C_SDC_L1_CONTROLLER_STOP_TIMEOUT);
        }else{
            ret = drvl1_sdc30_stop_controller(device_id, C_SDC_L1_CONTROLLER_STOP_TIMEOUT);
        }
		if (sd->Status & 0x1) {
			ret = -1;
		}
	}

	// Set registers to default value
	sd->IntEn = 0;
#if (defined _DRV_L1_DMA) && (_DRV_L1_DMA == 1)
	sd->Ctrl = C_SDC_CTL_BLK512 | C_SDC_CTL_DMA_ENABLE | (C_SDC_CTL_CLKDIV & 0xFF);	// Set clock to 400K Hz
	//sd->Ctrl = C_SDC_CTL_BLK512 | (C_SDC_CTL_CLKDIV & 0xFF);	// Set clock to 400K Hz
	//sd->Ctrl &= ~C_SDC_CTL_DMA_ENABLE;
#else
	sd->Ctrl =  C_SDC_CTL_BLK512 | (C_SDC_CTL_CLKDIV & 0xFF);	// Set clock to 400K Hz
#endif

    drvl1_sdc_clock_set(device_id, 400000);

    sd->Ctrl &= ~C_SDC_CTL_COP_MASK;
    sd->Ctrl |= C_SDC_CTL_COP_NORMAL;
    //sd->Ctrl |= C_SDC_CTL_COP_HALFT;
    //sd->Ctrl |= C_SDC_CTL_COP_1T;

    //sd->Ctrl |= C_SDC_EXT_WAIT_ENABLE;

    sd->ClkGated = 1; // when 1, SD clock is disable when no write data to SD card in WRITE_MULTIPLE_BLOCK
                      // when 0, SD clock is enable when no write data to SD card in WRITE_MULTIPLE_BLOCK

	sd->Arg = 0x0;
	sd->Cmd = 0;
	sd->Status = C_SDC_STATUS_CLEAR_ALL;		// Clear all status bits again
    drvl1_sdc_set_timeout(device_id, 0x1, 5000);
    if(vic_num == 0){
        vic_irq_register(VIC_SD2, SDC0_IRQHandler);
        vic_irq_enable(VIC_SD2);
    }else{
        vic_fiq_enable(VIC_SD2);
    }
	return ret;
}

void drvl1_sdc_restore_COP(INT32U device_id)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	DBG_PRINT("save_SDC_CTL_COP[%u]=0x%08x\r\n", device_id, save_SDC_CTL_COP[device_id]);
    sd->Ctrl &= ~C_SDC_CTL_COP_MASK;
    sd->Ctrl |= save_SDC_CTL_COP[device_id];

    return;
}

INT8U drvl1_sdc_COP_total_step_get(INT32U device_id)
{
    return 2;
}

// step: 0,1,2 0 is normal
void drvl1_sdc_COP_set(INT32U device_id, INT8U step)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	if (step > 2)
        step = 2;

    sd->Ctrl &= ~C_SDC_CTL_COP_MASK;
    if (step == 1)
        sd->Ctrl |= C_SDC_CTL_COP_HALFT;
    else if (step == 2)
        sd->Ctrl |= C_SDC_CTL_COP_1T;

    return;
}

INT8U drvl1_sdc_COP_get(INT32U device_id)
{
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

    INT32U cop = sd->Ctrl & C_SDC_CTL_COP_MASK;

    if (cop == C_SDC_CTL_COP_NORMAL)
        return 0;
    else if (cop == C_SDC_CTL_COP_HALFT)
        return 1;
    else if (cop == C_SDC_CTL_COP_1T)
        return 2;
    else
        return 0;
}

 void drvl1_sdc30_reset_controller(INT32U device_id)
 {
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    sd->Status = C_SDC_STATUS_CLEAR_ALL;
	sd->DmaBlk = 0;
    sd->Ctrl = 0x54;//0x200022a;
    sd->Ctrl2 = 0;//0xe20080;
    sd->Clk_Dly = 0;
	sd->Cmd_Dly = 0;
	sd->Data0_Dly = 0;
	sd->Data1_Dly = 0;
	sd->Data2_Dly = 0;
	sd->Data3_Dly = 0;
	sd->IntEn = 0x500;
	sd->SlowClk = 0x0;//999;
	drvl1_sdc30_all_delay_set(device_id, 0, 0x20/2);
 }

static INT8U drvl1_sdc_check_all_zero(INT32U *p, INT32U nword)
 {
    volatile INT32U *pword = (volatile INT32U *)p;
    INT32U k;

    for (k=0; k < nword; k++) {
        if (pword[k] != 0)
            return 0;
    }
    return 1;
 }

 /**
 * @brief   SD3.0 host controller reset.
 * @return 	none.
*/
 void drvl1_sdc30_module_reset(void)
 {
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(1);
    INT32U k;
    INT8U all_zero;

    R_SYSTEM_MSR_CTRL0 |= (1<<SD30_RST);
    //drv_tiny_wait(1);

    for (k = 0; k < 20; k++) {
        all_zero = drvl1_sdc_check_all_zero((INT32U *)sd, sizeof(sd30Reg_t)/4);
        if (all_zero) {
            //if (k != 0)
            //    DBG_PRINT("drvl1_sdc30_module_reset count %u\r\n", k);
            R_SYSTEM_MSR_CTRL0 &= ~(1<<SD30_RST);
            return;
        }
    }
    R_SYSTEM_MSR_CTRL0 &= ~(1<<SD30_RST);

    return;
 }

/**
 * @brief   SD2.0 host controller reset.
 * @return 	none.
*/
void drvl1_sdc_module_reset(void)
{
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(0);
    INT32U k;
    INT8U all_zero;

    R_SYSTEM_MSR_CTRL0 |= (1<<SD_RST);
    //drv_tiny_wait(1);
    for (k = 0; k < 20; k++) {
        all_zero = drvl1_sdc_check_all_zero((INT32U *)sd, sizeof(sdReg_t)/4);
        if (all_zero) {
            //if (k != 0)
            //    DBG_PRINT("drvl1_sdc_module_reset count %u\r\n", k);
            R_SYSTEM_MSR_CTRL0 &= ~(1<<SD_RST);
            return;
        }
    }
    R_SYSTEM_MSR_CTRL0 &= ~(1<<SD_RST);

    return;
 }

 /**
  * @brief   SD3.0 clock in source selection
  * @param   device_id[in]: device id.
  * @param   sel: 0 is SD Host, 1 is IOPAD
  * @return  none.
*/
void drvl1_sdc30_in_clk_sel(INT32U device_id, INT8U sel)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
    INT32U Ctrl2 = sd->Ctrl2;

    Ctrl2 = (sel == 0) ? (Ctrl2 | SDC_IN_CLK_SEL_SD_HOST) : (Ctrl2 & SDC_IN_CLK_SEL_IOPAD);

    sd->Ctrl2 = Ctrl2;
}

 /**
  * @brief   SD3.0 set all delay to same delay step
  * @param   device_id[in]: device id.
  * @param   sel: 0 is SD Host, 1 is IOPAD
  * @param   dstep[in]: delay step.
  * @return  none.
*/
void drvl1_sdc30_all_delay_set(INT32U device_id, INT8U sel, INT8U dstep)
{
    drvl1_sdc30_in_clk_sel(device_id, sel);
    drvl1_sdc30_sdclk_delay_offset_set(device_id, 0);
    drvl1_sdc30_sdclk_delay_set(device_id, dstep);
    drvl1_sdc30_sdcmd_delay_set(device_id, dstep);
    drvl1_sdc30_sddata_delay_set(device_id, dstep);
}

/**
 * @brief   SD3.0 clock in source invert
 * @param   device_id[in]: device id.
 * @param   inv: 0 is no invert, 1 is invert
 * @return 	0: success, -1: fail.
*/
void drvl1_sdc30_in_clk_inv(INT32U device_id, INT8U inv)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
    INT32U Ctrl2 = sd->Ctrl2;

    Ctrl2 = (inv == 1) ? (Ctrl2 | SDC_IN_CLK_DO_INV) : (Ctrl2 & SDC_IN_CLK_SEL_NOT_INV);

    sd->Ctrl2 = Ctrl2;
}

/**
 * @brief   SD3.0 host controller initialization.
 * @param   device_id[in]: device id.
 * @param   vic_num[in]: using irq or fiq
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_init(INT32U device_id, INT8U vic_num)
{
	INT32S ret;
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	ret = 0;

	drvl1_sdc30_reset_controller(device_id);

    drvl1_sdcbus_ldo_pwr_sel(device_id, SD_V33);
	sd->Status = C_SDC_STATUS_CLEAR_ALL;		// Clear all status bits

	// Force SD controller to stop
	if ((sd->Status & C_SDC_STATUS_CONTROLLER_BUSY) && (sd->Ctrl & C_SDC_CTL_ENABLE)) {
        ret = drvl1_sdc30_stop_controller(device_id, C_SDC_L1_CONTROLLER_STOP_TIMEOUT);
		if (sd->Status & C_SDC_STATUS_CONTROLLER_BUSY) {
			ret = -1;
		}
	}

	// Set registers to default value
	sd->IntEn = 0;
#if (defined _DRV_L1_DMA) && (_DRV_L1_DMA == 1)
	sd->Ctrl =  C_SDC_CTL_BLK512 | C_SDC_CTL_DMA_ENABLE | (C_SDC_CTL_CLKDIV & 0xFF);	// Set clock to 400K Hz
#else
	sd->Ctrl =  C_SDC_CTL_BLK512 | (C_SDC_CTL_CLKDIV & 0xFF);	// Set clock to 400K Hz
#endif

    drvl1_sdc30_clock_set(device_id, 400000);

    //sd->Ctrl |= C_SDC_EXT_WAIT_ENABLE; // TEST

	sd->Arg = 0x0;
	sd->Cmd = 0;
	sd->Status = C_SDC_STATUS_CLEAR_ALL;		// Clear all status bits again
	sd->Ctrl2 |= 0x00A00000;//SD input clock comes from SD host, enable read data operation time out machine
    //sd->Ctrl2 |= 0x00800000; // SDC_IN_CLK_SEL_IOPAD

    // set standard mode
    INT8U mode = 0;
    sd->Ctrl2 = ((sd->Ctrl2&0xfffffcff) | (mode<<4));

	#if (_DRV_L1_SDC_30_DMA_NOAUTOSTOP == 1) && ((defined _DRV_L1_DMA) && (_DRV_L1_DMA == 1))
	// when 1 Data engine will continuously receive or transmit data after DMA is done
	// when 0 = Data engine will stop automatically after DMA is done
	sd->Ctrl2 |= SDC_ENABLE_DMA_NOAUTOSTOP;
	drvl1_sdc_set_cmd_timeout(device_id, 1);
    sd->SlowClk = 0xffff; //0x3E7; // williamyeo-sd30-dmablksts-not-match
	#endif
    if(sd_dma_wait_queue == NULL)
        sd_dma_wait_queue = xQueueCreate(5, /*sizeof(INT32U)*/sizeof(INT8S));
    else
        xQueueReset(sd_dma_wait_queue);
	#if (_DRV_L1_SDC_30_USE_DMACOM_INT == 1) || (_DRV_L1_SDC_30_USE_ALLCOM_INT == 1)

          #if (_DRV_L1_SDC_30_USE_DMACOM_INT == 1)
          sd->IntEn &= (~(SDC_ENABLE_DMACOM_INT)); // williamyeo-sd30-dmablksts-not-match
          #endif

          #if (_DRV_L1_SDC_30_USE_ALLCOM_INT == 1)
          sd->IntEn |= SDC_ENABLE_ALLCOM_INT;
          #endif
    #endif
//        sd->IntEn |= C_SDC_INT_PRESENT;
    if(vic_num == 0){
        vic_irq_register(VIC_SD3, SDC1_IRQHandler);
        vic_irq_enable(VIC_SD3);
    }else{
        vic_fiq_enable(VIC_SD3);
    }

	return ret;
}

/**
 * @brief   SD2.0 controller enable/disable extending command wait cycle.
 * @param   device_id[in]: device id.
 * @param   enable[in]: enable/disable extending command wait cycle.
 * @return 	none.
*/
void drvl1_sdc_set_command_timeout_ext_wait(INT32U device_id, INT8U enable)
{
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
    if (enable)
        sd->Ctrl |= C_SDC_EXT_WAIT_ENABLE;
    else
        sd->Ctrl &= (~C_SDC_EXT_WAIT_ENABLE);
}

/**
 * @brief   SD3.0 controller enable/disable extending command wait cycle.
 * @param   device_id[in]: device id.
 * @param   enable[in]: enable/disable extending command wait cycle.
 * @return 	none.
*/
void drvl1_sdc30_set_command_timeout_ext_wait(INT32U device_id, INT8U enable)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
    if (enable)
        sd->Ctrl |= C_SDC_EXT_WAIT_ENABLE;
    else
        sd->Ctrl &= (~C_SDC_EXT_WAIT_ENABLE);
}


/**
 * @brief   Enable SD2.0 controller.
 * @param   device_id[in]: device id.
 * @return 	0:success.
*/
INT32S drvl1_sdc_enable(INT32U device_id)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	sd->Ctrl |= C_SDC_CTL_ENABLE;

	return 0;
}

/**
 * @brief   Enable SD3.0 controller.
 * @param   device_id[in]: device id.
 * @return 	0:success.
*/
INT32S drvl1_sdc30_enable(INT32U device_id)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	sd->Ctrl |= C_SDC_CTL_ENABLE;

	return 0;
}


/**
 * @brief   Enable/disable SD2.0 controller dummy clock function.
 * @param   device_id[in]: device id.
 * @param   en[in]: enable/disable dummy clock function.
 * @return 	0:success.
*/
void drvl1_sdc_dummyclk_en(INT32U device_id, INT8U en)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	if(en)
		sd->Ctrl |= C_SDC_CTL_DUMMYCLK;
	else
		sd->Ctrl &= ~C_SDC_CTL_DUMMYCLK;
}

/**
 * @brief   Enable/disable SD3.0 controller dummy clock function.
 * @param   device_id[in]: device id.
 * @param   en[in]: enable/disable dummy clock function.
 * @return 	0:success.
*/
void drvl1_sdc30_dummyclk_en(INT32U device_id, INT8U en)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	if(en)
		sd->Ctrl |= C_SDC_CTL_DUMMYCLK;
	else
		sd->Ctrl &= ~C_SDC_CTL_DUMMYCLK;
}

/**
 * @brief   SD2.0 controller SDIO Interrupt Detection Enabling selection.
 * @param   device_id[in]: device id.
 * @param   en[in]: enable/disable SDIO Interrupt Detection.
 * @return 	none.
*/
void drvl1_sdc_ioint_en(INT32U device_id, INT8U en)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	if(en)
		sd->Ctrl |= C_SDC_CTL_IO_INT;
	else
		sd->Ctrl &= ~C_SDC_CTL_IO_INT;
}


/**
 * @brief   SD3.0 controller SDIO Interrupt Detection Enabling selection.
 * @param   device_id[in]: device id.
 * @param   en[in]: enable/disable SDIO Interrupt Detection.
 * @return 	none.
*/
void drvl1_sdc30_ioint_en(INT32U device_id, INT8U en)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	if(en)
		sd->Ctrl |= C_SDC_CTL_IO_INT;
	else
		sd->Ctrl &= ~C_SDC_CTL_IO_INT;
}

/**
 * @brief   Enable/disable SD2.0 controller's interrupt.
 * @param   device_id[in]: device id.
 * @param   mask[in]: interrupt mask
 * @param   en[in]: enable/disable controller's interrupt.
 * @return 	none.
*/
void drvl1_sdc_int_en(INT32U device_id, INT32U mask, INT8U en)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	if(en)
		sd->IntEn |= mask;
	else
		sd->IntEn &= ~mask;
}

/**
 * @brief   Enable/disable SD3.0 controller's interrupt.
 * @param   device_id[in]: device id.
 * @param   mask[in]: interrupt mask
 * @param   en[in]: enable/disable controller's interrupt.
 * @return 	none.
*/
void drvl1_sdc30_int_en(INT32U device_id, INT32U mask, INT8U en)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	if(en)
		sd->IntEn |= mask;
	else
		sd->IntEn &= ~mask;
}

/**
 * @brief   Check SD2.0 controller's interrupt enabling condition.
 * @param   device_id[in]: device id.
 * @param   mask[in]: interrupt mask
 * @return 	0: all mask bit enabled, -1: not all mask bit enabled.
*/
INT32S drvl1_sdc_int_en_check(INT32U device_id, INT32U mask)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	return (sd->IntEn & mask) ? 0: -1;
}

/**
 * @brief   Check SD3.0 controller's interrupt enabling condition.
 * @param   device_id[in]: device id.
 * @param   mask[in]: interrupt mask
 * @return 	0: all mask bit enabled, -1: not all mask bit enabled.
*/
INT32S drvl1_sdc30_int_en_check(INT32U device_id, INT32U mask)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	return (sd->IntEn & mask) ? 0: -1;
}

/**
 * @brief   Check if SD2.0 controller's SDIO interrupt pending.
 * @param   device_id[in]: device id.
 * @return 	0: SDIO interrupt pending, -1: SDIO interrupt not pending.
*/
INT32S drvl1_sdc_sdio_int_check(INT32U device_id)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	return (sd->Status & C_SDC_STATUS_SDIO_INT_PEND) ? 0: -1;
}

/**
 * @brief   Check if SD3.0 controller's SDIO interrupt pending.
 * @param   device_id[in]: device id.
 * @return 	0: SDIO interrupt pending, -1: SDIO interrupt not pending.
*/
INT32S drvl1_sdc30_sdio_int_check(INT32U device_id)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	return (sd->Status & C_SDC_STATUS_SDIO_INT_PEND) ? 0: -1;
}

/**
 * @brief   Clear SD2.0 controller's SDIO interrupt pending status.
 * @param   device_id[in]: device id.
 * @return 	none.
*/
void drvl1_sdc_sdio_int_clear(INT32U device_id)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	sd->Status = C_SDC_STATUS_SDIO_INT_PEND;
}

/**
 * @brief   Clear SD3.0 controller's SDIO interrupt pending status.
 * @param   device_id[in]: device id.
 * @return 	none.
*/
void drvl1_sdc30_sdio_int_clear(INT32U device_id)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	sd->Status = C_SDC_STATUS_SDIO_INT_PEND;
}

/**
 * @brief   Disable SD2.0 controller.
 * @param   device_id[in]: device id.
 * @return 	0:success, -1:fail.
*/
INT32S drvl1_sdc_disable(INT32U device_id)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	// Wait until controller is not busy and then set control register
	if (drvl1_sdc_controller_busy_wait(device_id, C_SDC_L1_CONTROLLER_BUSY_TIMEOUT)) {
		sd->Ctrl &= ~C_SDC_CTL_ENABLE;
		return -1;
	}

	sd->Ctrl &= ~C_SDC_CTL_ENABLE;

	return 0;
}

/**
 * @brief   Disable SD3.0 controller.
 * @param   device_id[in]: device id.
 * @return 	0:success, -1:fail.
*/
INT32S drvl1_sdc30_disable(INT32U device_id)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	// Wait until controller is not busy and then set control register
	if (drvl1_sdc30_controller_busy_wait(device_id, C_SDC_L1_CONTROLLER_BUSY_TIMEOUT)) {
		sd->Ctrl &= ~C_SDC_CTL_ENABLE;
		return -1;
	}

	sd->Ctrl &= ~C_SDC_CTL_ENABLE;

	return 0;
}


/**
 * @brief   Wait SD2.0 controller busy until timeout.
 * @param   device_id[in]: device id.
 * @param   timeout[in]: waiting timeout value
 * @return 	0:success, -1:fail.
*/
INT32S drvl1_sdc_controller_busy_wait(INT32U device_id, INT32U timeout)
{
	if (drvl1_sdc_status_clear_wait(device_id, C_SDC_STATUS_CONTROLLER_BUSY, timeout)) {
		return -1;
	}

	return 0;
}

/**
 * @brief   Wait SD3.0 controller busy until timeout.
 * @param   device_id[in]: device id.
 * @param   timeout[in]: waiting timeout value
 * @return 	0:success, -1:fail.
*/
INT32S drvl1_sdc30_controller_busy_wait(INT32U device_id, INT32U timeout)
{
	if (drvl1_sdc30_status_clear_wait(device_id, C_SDC_STATUS_CONTROLLER_BUSY, timeout)) {
		return -1;
	}

	return 0;
}

/**
 * @brief   Get SD2.0 current clock rate.
 * @param   device_id[in]: device id.
 * @param   *p_rate[out]: current clock rate.
 * @param   *p_souce_rate[out]: source clock rate
 * @param   *p_div[out]: clock divider division factor
 * @return 	none.
*/
void drvl1_sdc_running_clock_rate_get(INT32U device_id, INT32U *p_rate, INT32U *p_souce_rate, INT32U *p_div)
{
    INT32U div, div_act, rate;
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

    //DBG_PRINT("sd->Ctrl=0x%08x, sd-%u\r\n", sd->Ctrl, device_id);

    div = sd->Ctrl & 0xff;
    div_act = 2*(div + 1);

    rate = MCLK / div_act;

    *p_rate = rate;
    *p_souce_rate = MCLK;
    *p_div = div_act;

    return;
}

/**
 * @brief   Get SD3.0 current clock rate.
 * @param   device_id[in]: device id.
 * @param   *p_rate[out]: current clock rate.
 * @param   *p_souce_rate[out]: source clock rate
 * @param   *p_div[out]: clock divider division factor
 * @return 	none.
*/
void drvl1_sdc30_running_clock_rate_get(INT32U device_id, INT32U *p_rate, INT32U *p_souce_rate, INT32U *p_div)
{
    INT32U div, div_act, rate;
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    div = (sd->Ctrl & 0xff) | (((sd->Ctrl2>>16) & 0x0f)<<8);

    DBG_PRINT("sd**div=0x%x\r\n", div);
    div_act = (div + 1);

    rate = SD3MCLK / div_act;
    DBG_PRINT("sd**rate=%d\r\n", rate);

    *p_rate = rate;
    *p_souce_rate = SD3MCLK;
    *p_div = div_act;

    return;
}

/**
 * @brief   Set SD2.0 current clock rate.
 * @param   device_id[in]: device id.
 * @param   speed[in]: clock setting value.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_clock_set(INT32U device_id, INT32U speed)
{
	INT32U ctrl, div;
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	// Wait until controller is not busy and then set control register
	if (drvl1_sdc_controller_busy_wait(device_id, C_SDC_L1_CONTROLLER_BUSY_TIMEOUT)) {
		return -1;
	}

	div = (INT32U) ((MCLK+(speed<<1)-1) / (speed<<1)) - 1;
	if (div > 255)
        div = 255;
	ctrl = sd->Ctrl;
	ctrl &= ~0xFF;					// Clear speed setting
	ctrl |= (div & 0xFF);			// Set new speed
	sd->Ctrl = ctrl;

    //DBG_PRINT("sd clock speed %u, divider %u, MCLK=%u\r\n", speed, div, MCLK);

	return 0;
}

/**
 * @brief   Set SD3.0 current clock rate.
 * @param   device_id[in]: device id.
 * @param   speed[in]: clock setting value.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_clock_set(INT32U device_id, INT32U speed)
{
	INT32U ctrl, ctrl2, div;
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	// Wait until controller is not busy and then set control register
	if (drvl1_sdc30_controller_busy_wait(device_id, C_SDC_L1_CONTROLLER_BUSY_TIMEOUT)) {
		return -1;
	}
    //DBG_PRINT("[SD3MCLK]%d, [speed]%d\r\n", SD3MCLK, speed);
    if(SD3MCLK%speed) {
        div = (INT32U) (SD3MCLK/speed);
    } else {
        div = (INT32U) ((SD3MCLK/speed)-1);
    }
	ctrl = sd->Ctrl;
	ctrl &= ~0xFF;					// Clear speed setting
    ctrl2 = sd->Ctrl2;
    ctrl2 &= 0xFFF0FFFF;
	ctrl |= (div & 0xFF);			// Set new speed
    ctrl2 |= ((div & 0xF00)<<8);
    sd->Ctrl2 = ctrl2;
	sd->Ctrl = ctrl;

	return 0;
}

/**
 * @brief   Set SD2.0 block length.
 * @param   device_id[in]: device id.
 * @param   size[in]: block length in the unit of bytes.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_block_len_set(INT32U device_id, INT32U size)
{
	INT32U ctrl;
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	// Wait until controller is not busy and then set control register
	if (drvl1_sdc_controller_busy_wait(device_id, C_SDC_L1_CONTROLLER_BUSY_TIMEOUT)) {
		return -1;
	}

	ctrl = sd->Ctrl;
	ctrl &= 0xF000FFFF;					// Clear block len setting
	ctrl |= (size & 0xFFF) << 16;		// Set new block len
	sd->Ctrl = ctrl;

	return 0;
}

/**
 * @brief   Set SD3.0 block length.
 * @param   device_id[in]: device id.
 * @param   size[in]: block length in the unit of bytes.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_block_len_set(INT32U device_id, INT32U size)
{
	INT32U ctrl;
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	// Wait until controller is not busy and then set control register
	if (drvl1_sdc30_controller_busy_wait(device_id, C_SDC_L1_CONTROLLER_BUSY_TIMEOUT)) {
		return -1;
	}

	ctrl = sd->Ctrl;
	ctrl &= 0xF000FFFF;					// Clear block len setting
	ctrl |= (size & 0xFFF) << 16;		// Set new speed
	sd->Ctrl = ctrl;

	return 0;
}

/**
 * @brief   Set SD2.0 bus width.
 * @param   device_id[in]: device id.
 * @param   width[in]: bus width, one or four.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_bus_width_set(INT32U device_id, INT8U width)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	// Wait until controller is not busy and then set control register
	if (drvl1_sdc_controller_busy_wait(device_id, C_SDC_L1_CONTROLLER_BUSY_TIMEOUT)) {
		return -1;
	}

	if (width == 4) {
		sd->Ctrl |= C_SDC_CTL_BUS_4BIT;
	} else {
		sd->Ctrl &= ~C_SDC_CTL_BUS_4BIT;
	}

	return 0;
}

/**
 * @brief   Set SD3.0 bus width.
 * @param   device_id[in]: device id.
 * @param   width[in]: bus width, one or four.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_bus_width_set(INT32U device_id, INT8U width)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	// Wait until controller is not busy and then set control register
	if (drvl1_sdc30_controller_busy_wait(device_id, C_SDC_L1_CONTROLLER_BUSY_TIMEOUT)) {
		return -1;
	}

	if (width == 4) {
		sd->Ctrl |= C_SDC_CTL_BUS_4BIT;
	} else {
		sd->Ctrl &= ~C_SDC_CTL_BUS_4BIT;
	}

	return 0;
}

/**
 * @brief   Stop SD2.0 controller.
 * @param   device_id[in]: device id.
 * @param   in_timeout[in]: stop controller and wait until this timeout value.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_stop_controller(INT32U device_id, INT32U in_timeout)
{
    volatile INT32U timeout = in_timeout;
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
#if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
	volatile INT32U timer5, timer6;

	// Set STPCMD(bit6) to stop the controller
	sd->Cmd = C_SDC_CMD_STOP;

	timer5 = tiny_counter_get();
	while (timeout) {
		if (!( sd->Cmd & C_SDC_CMD_STOP)) {
			return 0;
		}

		timer6 = tiny_counter_get();
		if (timer5 != timer6) {
			timer5 = timer6;
			timeout--;
		}
	}

	return -1;

#else
	volatile INT32U i;
	// Set STPCMD(bit6) to stop the controller
	sd->Cmd = C_SDC_CMD_STOP;

	i = 0;
	while (sd->Cmd & C_SDC_CMD_STOP) {
		i++;
		if (i > (timeout<<8)) {
			return -1;
		}
	}
	return 0;
#endif
}

/**
 * @brief   Stop SD3.0 controller.
 * @param   device_id[in]: device id.
 * @param   in_timeout[in]: stop controller and wait until this timeout value.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_stop_controller(INT32U device_id, INT32U in_timeout)
{
    volatile INT32U timeout = in_timeout;
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
#if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
	volatile INT32U timer5, timer6;

	// Set STPCMD(bit6) to stop the controller
	sd->Cmd = C_SDC_CMD_STOP;

	timer5 = tiny_counter_get();
	while (timeout) {
		if (!( sd->Cmd & C_SDC_CMD_STOP)) {
			return 0;
		}

		timer6 = tiny_counter_get();
		if (timer5 != timer6) {
			timer5 = timer6;
			timeout--;
		}
	}

	return -1;

#else
	volatile INT32U i;
	// Set STPCMD(bit6) to stop the controller
	sd->Cmd = C_SDC_CMD_STOP;

	i = 0;
	while (sd->Cmd & C_SDC_CMD_STOP) {
		i++;
		if (i > (timeout<<8)) {
			return -1;
		}
	}
	return 0;
#endif
}

/**
 * @brief   Start the SD2.0 controller to generate a 74 clock cycles on the clock line.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_card_init_74_cycles(INT32U device_id)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	// Wait until controller is not busy and then set control register
	if(device_id == 0){
        if (drvl1_sdc_controller_busy_wait(device_id, C_SDC_L1_CONTROLLER_BUSY_TIMEOUT)) {
            return -1;
        }
    }else{
        if (drvl1_sdc30_controller_busy_wait(device_id, C_SDC_L1_CONTROLLER_BUSY_TIMEOUT)) {
            return -1;
        }
    }

	sd->Status = C_SDC_STATUS_CLEAR_ALL;		// Clear all status bits

	sd->Cmd = C_SDC_CMD_INIT_CARD;

    if(device_id == 0){
        return drvl1_sdc_command_complete_wait(device_id, C_SDC_L1_CARD_INIT_74_CYCLES_TIMEOUT);
    }else{
        return drvl1_sdc30_command_complete_wait(device_id, C_SDC_L1_CARD_INIT_74_CYCLES_TIMEOUT);
    }
}


/**
 * @brief   Start the SD3.0 controller to generate a 74 clock cycles on the clock line.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_card_init_74_cycles(INT32U device_id)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	// Wait until controller is not busy and then set control register
	if (drvl1_sdc30_controller_busy_wait(device_id, C_SDC_L1_CONTROLLER_BUSY_TIMEOUT)) {
		return -1;
	}

	sd->Status = C_SDC_STATUS_CLEAR_ALL;		// Clear all status bits

	sd->Cmd = C_SDC_CMD_INIT_CARD;

	return drvl1_sdc30_command_complete_wait(device_id, C_SDC_L1_CARD_INIT_74_CYCLES_TIMEOUT);
}

/**
 * @brief   Wait SD2.0 controller card busy status till timeout happened.
 * @param   device_id[in]: device id.
 * @param   timeout[in]: timeout value of waiting card busy status.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_card_busy_wait(INT32U device_id, INT32U timeout)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	if (!(sd->Ctrl & C_SDC_CTL_ENABLE)) {
	// Polling card busy bit of status register is valid only when SDC enable bit of control register is set
		return 0;
	}
	return drvl1_sdc_status_clear_wait(device_id, C_SDC_STATUS_CARD_BUSY, timeout);
}

/**
 * @brief   Wait SD3.0 controller card busy status till timeout happened .
 * @param   device_id[in]: device id.
 * @param   timeout[in]: timeout value of waiting card busy status.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_card_busy_wait(INT32U device_id, INT32U timeout)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	if (!(sd->Ctrl & C_SDC_CTL_ENABLE)) {
	// Polling card busy bit of status register is valid only when SDC enable bit of control register is set
		return 0;
	}
	return drvl1_sdc30_status_clear_wait(device_id, C_SDC_STATUS_CARD_BUSY, timeout);
}

/**
 * @brief   SD2.0 controller send command.
 * @param   device_id[in]: device id.
 * @param   command[in]: sd command.
 * @param   argument[in]: sd command argument.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_command_send(INT32U device_id, INT32U command, INT32U argument)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	// Wait until controller is not busy and then set control register
	if (drvl1_sdc_controller_busy_wait(device_id, C_SDC_L1_CONTROLLER_BUSY_TIMEOUT)) {
		return -1;
	}
	sd->Status = C_SDC_STATUS_CLEAR_ALL;		// Clear all status bits

	sd->Arg = argument;
	sd->Cmd = command | C_SDC_CMD_RUN;

	return 0;
}

/**
 * @brief   SD3.0 controller send command.
 * @param   device_id[in]: device id.
 * @param   command[in]: sd command.
 * @param   argument[in]: sd command argument.
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_command_send(INT32U device_id, INT32U command, INT32U argument)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	// Wait until controller is not busy and then set control register
	if (drvl1_sdc30_controller_busy_wait(device_id, C_SDC_L1_CONTROLLER_BUSY_TIMEOUT)) {
		return -1;
	}
	sd->Status = C_SDC_STATUS_CLEAR_ALL;		// Clear all status bits

	sd->Arg = argument;
	sd->Cmd = command | C_SDC_CMD_RUN;

	return 0;
}

/**
 * @brief   Get SD2.0 controller issuing command's response till timeout.
 * @param   device_id[in]: device id.
 * @param   response[out]: command's response.
 * @param   timeout[in]: timeout value of waiting command's response.
 * @return 	0: success, -1: fail, timeout or status error.
*/
INT32S drvl1_sdc_response_get(INT32U device_id, INT32U *response, INT32U timeout)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	// Poll status register until CMD_BUF_FULL(bit6)=1
	if (drvl1_sdc_status_set_wait(device_id, C_SDC_STATUS_RESP_REG_FULL, timeout)) {
		if (sd_card_print_status_fail_flag[device_id])
		{
			DBG_PRINT("SDERR 41. status=%08x timeout=%u cmd=0x%08x arg=0x%08x kind=%u\r\n",  \
                            sd->Status, timeout, sd->Cmd, sd->Arg, sd_card_access_kind[device_id]);
		}
		return -1;
	}
	if (sd->Status & (C_SDC_STATUS_RESP_INDEX_ERR|C_SDC_STATUS_RESP_CRC_ERR|C_SDC_STATUS_TIMEOUT)) {
        //if (sd->Status & C_SDC_STATUS_TIMEOUT)
        //    DBG_PRINT("response_get C_SDC_STATUS_TIMEOUT\r\n");
		if (sd_card_print_status_fail_flag[device_id])
		{
			DBG_PRINT("SDERR 42. status=%08x timeout=%u cmd=0x%08x arg=0x%08x kind=%u\r\n", \
                            sd->Status, timeout, sd->Cmd, sd->Arg, sd_card_access_kind[device_id]);
		}
		return -1;
	}
	*response = sd->Resp;

	return 0;
}

/**
 * @brief   Get SD3.0 controller issuing command's response till timeout.
 * @param   device_id[in]: device id.
 * @param   response[out]: command's response.
 * @param   timeout[in]: timeout value of waiting command's response.
 * @return 	0: success, -1: fail, timeout or status error.
*/
INT32S drvl1_sdc30_response_get(INT32U device_id, INT32U *response, INT32U timeout)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	// Poll status register until CMD_BUF_FULL(bit6)=1
	if (drvl1_sdc30_status_set_wait(device_id, C_SDC_STATUS_RESP_REG_FULL, timeout)) {
		if (sd_card_print_status_fail_flag[device_id])
		{
			DBG_PRINT("SD3ERR 41. status=%08x timeout=%u cmd=0x%08x arg=0x%08x kind=%u\r\n",  \
                            sd->Status, timeout, sd->Cmd, sd->Arg, sd_card_access_kind[device_id]);
		}
		return -1;
	}
	if (sd->Status & (C_SDC_STATUS_RESP_INDEX_ERR|C_SDC_STATUS_RESP_CRC_ERR|C_SDC_STATUS_TIMEOUT)) {
        //if (sd->Status & C_SDC_STATUS_TIMEOUT)
        //    DBG_PRINT("response_get C_SDC_STATUS_TIMEOUT\r\n");
		if (sd_card_print_status_fail_flag[device_id])
		{
			DBG_PRINT("SD3ERR 42. status=%08x timeout=%u cmd=0x%08x arg=0x%08x kind=%u\r\n", \
                            sd->Status, timeout, sd->Cmd, sd->Arg, sd_card_access_kind[device_id]);
		}
		return -1;
	}

	drv_tiny_wait(1); // This is must, because discover CMD9, CMD2 C_RESP_R2 response will get wrong

	*response = sd->Resp;

	return 0;
}

/**
 * @brief   SD2.0 controller get data till timeout.
 * @param   device_id[in]: device id.
 * @param   *buf[out]: data buffer
 * @param   timeout[in]: timeout value of waiting data.
 * @return 	0: success, -1: fail, timeout or status error.
*/
INT32S drvl1_sdc_data_get(INT32U device_id, INT32U *buf, INT32U timeout)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	if (drvl1_sdc_status_set_wait(device_id, C_SDC_STATUS_DATA_BUF_FULL, timeout)) {
		return -1;
	}

	*buf = sd->DataRx;

	return 0;
}

/**
 * @brief   SD3.0 controller get data till timeout.
 * @param   device_id[in]: device id.
 * @param   *buf[out]: data buffer
 * @param   timeout[in]: timeout value of waiting data.
 * @return 	0: success, -1: fail, timeout or status error.
*/
INT32S drvl1_sdc30_data_get(INT32U device_id, INT32U *buf, INT32U timeout)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	if (drvl1_sdc30_status_set_wait(device_id, C_SDC_STATUS_DATA_BUF_FULL, timeout)) {
		return -1;
	}

	*buf = sd->DataRx;

	return 0;
}

/**
 * @brief   SD2.0 controller wait command completed till timeout.
 * @param   device_id[in]: device id.
 * @param   timeout[in]: timeout value of waiting command completed.
 * @return 	0: success, -1: fail, timeout or status error.
*/
INT32S drvl1_sdc_command_complete_wait(INT32U device_id, INT32U timeout)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	if (sd->Status & C_SDC_STATUS_CONTROLLER_BUSY) {
		if (drvl1_sdc_status_set_wait(device_id, C_SDC_STATUS_CMD_COMPLETE, timeout)) {
			return -1;
		}
	}

	if (sd->Status & (C_SDC_STATUS_RESP_INDEX_ERR|C_SDC_STATUS_RESP_CRC_ERR|C_SDC_STATUS_TIMEOUT)) {
		return -1;
	}

	return 0;
}

/**
 * @brief   SD3.0 controller wait command completed till timeout.
 * @param   device_id[in]: device id.
 * @param   timeout[in]: timeout value of waiting command completed.
 * @return 	0: success, -1: fail, timeout or status error.
*/
INT32S drvl1_sdc30_command_complete_wait(INT32U device_id, INT32U timeout)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	if (sd->Status & C_SDC_STATUS_CONTROLLER_BUSY) {
		if (drvl1_sdc30_status_set_wait(device_id, C_SDC_STATUS_CMD_COMPLETE, timeout)) {
			return -1;
		}
	}

	if (sd->Status & (C_SDC_STATUS_RESP_INDEX_ERR|C_SDC_STATUS_RESP_CRC_ERR|C_SDC_STATUS_TIMEOUT)) {
		return -1;
	}

	return 0;
}

/**
 * @brief   SD2.0 controller clear RX data register.
 * @param   device_id[in]: device id.
 * @return 	none.
*/
void drvl1_sdc_clear_rx_data_register(INT32U device_id)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	volatile INT32U temp;

	temp = sd->DataRx;
	while (1) {
        if (sd->Status & C_SDC_STATUS_DATA_BUF_FULL) {
            temp = sd->DataRx;
            temp = temp;
        }
        else
            break;
    }
}

/**
 * @brief   SD3.0 controller clear RX data register.
 * @param   device_id[in]: device id.
 * @return 	none.
*/
void drvl1_sdc30_clear_rx_data_register(INT32U device_id)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	volatile INT32U temp;

	temp = sd->DataRx;
	if (sd->Status & C_SDC_STATUS_DATA_BUF_FULL) {
		temp = sd->DataRx;
		temp = temp;
	}
}

/**
 * @brief   SD2.0 controller data dma busy polling.
 * @param   device_id[in]: device id.
 * @param   is_read[in]: 1: read, 0: write.
 * @param   dma_channel[in]: dma channel number.
 * @param   timeout[in]: dma timeout value.
 * @param   (*quit_proc)(INT32U)[in]: used by user to judge quit contidion.
 * @param   refCon[in]: any purpose for quit_proc.
 * @return 	0: quit_proc return 1, -1: timeout happened.
*/
INT32S drvl1_sdc_busy_polling(INT32U device_id, INT8U is_read, INT8U dma_channel, INT32U timeout, INT32U (*quit_proc)(INT32U), INT32U refCon)
{
	INT32U save_timeout = timeout;
	#if _OPERATING_SYSTEM == _OS_FREERTOS
	TimeOut_t xTimeOut;
    TickType_t xRemainTicks = pdMS_TO_TICKS(timeout);
    TickType_t xPrevRemainTicks = pdMS_TO_TICKS(timeout);
	#else
	INT32U timer0, timer1;
	INT32U pretimeout = timeout;
    #endif
    INT32U sd_spec_sector_timeout;
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
    INT32U dmamswait;
    INT32U dma_first_count;

    dmamswait = (is_read) ? (K_SD_READ_TIMEOUT_PER_SECTOR*2) : 615;
    dma_first_count = dma_get_tx_count(dma_channel, &dma_first_count);
    sd_spec_sector_timeout = (is_read) ? K_SD_READ_TIMEOUT_PER_SECTOR : K_SD_WRITE_TIMEOUT_PER_SECTOR;
    if (is_read) {
        sd_spec_sector_timeout = (sd_card_read_over_spec_sector_timeout[device_id] != 0) ? sd_card_read_over_spec_sector_timeout[device_id] : K_SD_READ_TIMEOUT_PER_SECTOR;
    }
	#if _OPERATING_SYSTEM == _OS_FREERTOS
	vTaskSetTimeOutState(&xTimeOut);
	#else
	dmamswait = dmamswait * 376;
	timer0 = tiny_counter_get();
	#endif

	while (timeout) {
		if (quit_proc != NULL)
		{
			if ((*quit_proc)(refCon))
			{
				return 0;
			}
		}
		//if (is_read)
		{
			if ((sd->Status & C_SDC_STATUS_TIMEOUT) != 0)
			{

				INT32U accmswait;

				#if _OPERATING_SYSTEM == _OS_FREERTOS
				accmswait = save_timeout - xRemainTicks;
				#else
				accmswait = (save_timeout - timeout)/376; // 2.66*376 = 1ms
				#endif

				// trigger dummy clock to allow sd to complete it's action
				if(device_id == 0){
                    drvl1_sdc_dummyclk_en(device_id, 1);
                }else{
                    drvl1_sdc30_dummyclk_en(device_id, 1);
                }
				if (sd_spec_sector_timeout > accmswait)
				{
                    if (drvl1_sdc_check_card_present(device_id))
                        drv_msec_wait(sd_spec_sector_timeout - accmswait);
                }

                if(device_id == 0){
                    drvl1_sdc_dummyclk_en(device_id, 0); // disable sd clock
				}else{
                    drvl1_sdc30_dummyclk_en(device_id, 0); // disable sd clock
				}
				if ((MCLK >= 81000000) && (accmswait > K_SD_READ_TIMEOUT_PER_SECTOR))
                    sd_card_read_over_spec_sector_timeout[device_id] = 135;
				else
					sd_card_read_over_spec_sector_timeout[device_id] = 0;
				//DBG_PRINT("sd-%u BIT9 raised, %u, %u\r\n", device_id, sd_spec_sector_timeout, sd_card_read_over_spec_sector_timeout[device_id]);
				if (sd_spec_sector_timeout == K_SD_READ_TIMEOUT_PER_SECTOR)
                    DBG_PRINT("9");
                else
                    DBG_PRINT("6");
				return -1;
			}
			#if _OPERATING_SYSTEM == _OS_FREERTOS
            if ((xPrevRemainTicks - xRemainTicks) >= dmamswait)
            #else
            if ((pretimeout - timeout) >= dmamswait)
            #endif
            {
                INT32U dma_second_count;

                dma_get_tx_count(dma_channel, &dma_second_count);
                if ((dma_second_count != 0) && (dma_second_count == dma_first_count))
                {
                    DBG_PRINT("2 sd-%u dma left count is same after a while. %u, %u, %u, 0x%08x\r\n", device_id, dma_second_count, dmamswait, is_read, sd->Status);
                    drvl1_sdc_clear_timeout_bit(device_id, 10); // this is not timeout bit bug, so clear it
                    return -1;
                }
                dma_first_count = dma_second_count;
                #if _OPERATING_SYSTEM == _OS_FREERTOS
                xPrevRemainTicks = xRemainTicks;
                #else
                pretimeout = timeout;
                #endif
            }

            if (drvl1_sdc_check_card_present(device_id) == 0)
            {
                return -1;
            }
		}
		#if _OPERATING_SYSTEM == _OS_FREERTOS
		if (xTaskCheckForTimeOut(&xTimeOut, &xRemainTicks) == pdTRUE)
			break;
		#else
		timer1 = tiny_counter_get();
		if (timer0 != timer1) {
			timer0 = timer1;
			timeout--;
		}
		#endif
	}

    DBG_PRINT("drvl1_sdc_busy_polling time out %u\r\n", save_timeout);

	return -1;
}

/**
 * @brief   SD2.0 controller polling DMA condition procedure.
 * @param   refCon[in]: DMA status for polling
 * @return 	1: quit when error or done, 0: waiting.
*/
INT32U drvl1_sdc_dma_poll_condition_proc(INT32U refCon)
{
	volatile INT8S *poll_status = (INT8S *)refCon;

	if ((*poll_status) != C_DMA_STATUS_WAITING) {
		return 1; // quit when error or done
    } else {
		return 0; // waiting
    }
}


/**
 * @brief   SD2.0 controller read bytes by DMA.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: read buffer.
 * @param   byte_count[in]: read byte count
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_read_byte_by_dma(INT32U device_id, INT32U *buffer, INT32U byte_count)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

    if ((byte_count & 0x03) != 0)
	{
		DBG_PRINT("drvl1_sdc_read_byte_by_dma count must 4 multiple\r\n");
		return -1;
	}

#if (defined _DRV_L1_DMA) && (_DRV_L1_DMA == 1)
	DMA_STRUCT dma_struct={0};
	volatile INT8S done;

	dma_struct.s_addr = (INT32U)  &sd->DataRx;
	dma_struct.t_addr = (INT32U) buffer;

    dma_struct.width = DMA_DATA_WIDTH_4BYTE;
    dma_struct.count = (INT32U) (byte_count >> 2);

	//dma_struct.timeout = drvl1_sdc_cal_read_timeout((byte_count>>9));
	dma_struct.timeout = 0; // use software timeout of dma

	done = C_DMA_STATUS_WAITING;
	dma_struct.notify = &done;

	// Send dma request to driver and wait response
	if (drv_l1_dma_transfer_wait_ready(&dma_struct)) {
		return -1;
	}

	return 0;
#else
	INT32U count;

	count = (INT32U) (byte_count>>2);
	while (count) {
        if(device_id == 0){
            if (drvl1_sdc_data_get(device_id, buffer, C_SDC_L1_DATA_FULL_TIMEOUT)) {
                return -1;
            }
        }else{
            if (drvl1_sdc30_data_get(device_id, buffer, C_SDC_L1_DATA_FULL_TIMEOUT)) {
                return -1;
            }
        }
		buffer++;
		count--;
	}

	return 0;
#endif
}


/**
 * @brief   SD2.0 controller write bytes by DMA.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: write buffer.
 * @param   byte_count[in]: write byte count
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_write_byte_by_dma(INT32U device_id, INT32U *buffer, INT32U byte_count)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	if ((byte_count & 0x03) != 0)
	{
		DBG_PRINT("drvl1_sdc_read_byte_by_dma count must 4 multiple\r\n");
		return -1;
	}

#if 0 && (defined _DRV_L1_DMA) && (_DRV_L1_DMA == 1) // when 1 word, 1 byte, this couldnt working, so remark it
	DMA_STRUCT dma_struct={0};
	volatile INT8S done;

	dma_struct.s_addr = (INT32U) buffer;
	dma_struct.t_addr = (INT32U) &sd->DataTx;

	dma_struct.width = DMA_DATA_WIDTH_4BYTE;
    dma_struct.count = (INT32U) (byte_count >> 2);

	//dma_struct.timeout = drvl1_sdc_cal_write_timeout((byte_count>>9));
	dma_struct.timeout = 0; // use software timeout of dma

	done = C_DMA_STATUS_WAITING;
	dma_struct.notify = &done;

	// Send dma request to driver and wait response
	if (drv_l1_dma_transfer_wait_ready(&dma_struct)) {
		return -1;
	}

	return 0;
#else
	INT32U count;

	count = (INT32U) (byte_count>>2);
	while (count) {
		if (drvl1_sdc_status_set_wait(device_id, C_SDC_STATUS_DATA_BUF_EMPTY, C_SDC_L1_DATA_EMPTY_TIMEOUT)) {
			return -1;
		}
		sd->DataTx = *buffer;
		buffer++;
		count--;
	}

	return 0;
#endif
}

/**
 * @brief   SD2.0 controller read by DMA extend base function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @param   *poll_status[in]: DMA polling status
 * @param   polling[]: using polling method
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_read_data_by_dma_ext_base(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status, INT8U polling)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

    #if 1
    {
        static INT8U sdc_dbg_check_ext_wait = 0;

        if (sdc_dbg_check_ext_wait == 0)
        {
            if ((sd->Ctrl & C_SDC_EXT_WAIT_ENABLE) != 0)
                DBG_PRINT("C_SDC_EXT_WAIT_ENABLE is on.  0x%08x \r\n", sd->Ctrl);
            sdc_dbg_check_ext_wait = 1;
        }
    }
    #endif

    #if 1
    {
        static INT8U sdc_dbg_check_dummy_en = 0;

        if (sdc_dbg_check_dummy_en == 0)
        {
            if ((sd->Ctrl & C_SDC_CTL_DUMMYCLK) != 0)
                DBG_PRINT("C_SDC_CTL_DUMMYCLK is on.  0x%08x \r\n", sd->Ctrl);
            sdc_dbg_check_dummy_en = 1;
        }
    }
    #endif


#if (defined _DRV_L1_DMA) && (_DRV_L1_DMA == 1)
	DMA_STRUCT dma_struct={0};
	volatile INT8S done;

	dma_struct.s_addr = (INT32U)  &sd->DataRx;
	dma_struct.t_addr = (INT32U) buffer;
	dma_struct.width = DMA_DATA_WIDTH_4BYTE;
	dma_struct.count = (INT32U) (C_SDC_SECTOR_SIZE/4) * sector_count;

	//dma_struct.timeout = drvl1_sdc_cal_read_timeout(sector_count);
	dma_struct.timeout = 0; // use software timeout of dma

	if (poll_status) {
		*poll_status = C_DMA_STATUS_WAITING;
		dma_struct.notify = poll_status;

        // Send dma request to driver and return immediately
		if (drv_l1_dma_transfer(&dma_struct)) {
			return -1;
		}
		if (polling)
		{
			INT32U timeout_count = (INT32U)255 * 1638*2;
			INT32U msecwait;
			INT32U sd_spec_sector_timeout = K_SD_READ_TIMEOUT_PER_SECTOR; // in ms unit

			msecwait = sector_count * sd_spec_sector_timeout;
			msecwait *= 2; // double the timeout, because discover burn out sd card read time will up to 168ms per sector
			msecwait = (msecwait * 1016) / 1000; // adjust for precision error

			#if _OPERATING_SYSTEM == _OS_FREERTOS
			timeout_count = msecwait;
			#else
			timeout_count = msecwait * 376UL;
			#endif

			if (drvl1_sdc_busy_polling(device_id, 1, dma_struct.channel, timeout_count, drvl1_sdc_dma_poll_condition_proc, (INT32U)poll_status))
			{
				drv_l1_dma_enforce_timeout((INT32U)dma_struct.channel);
				return -1;
			}
			if ((*poll_status) == C_DMA_STATUS_DONE)
				return 0;
			else
			{
				return -1;
			}
		}
	} else {
		done = C_DMA_STATUS_WAITING;
		dma_struct.notify = &done;

        // Send dma request to driver and wait response
		if (drv_l1_dma_transfer_wait_ready(&dma_struct)) {
			return -1;
		}
	}

	return 0;
#else
	INT32U count;

	count = (INT32U) (C_SDC_SECTOR_SIZE/4) * sector_count;
	while (count) {
        if(device_id == 0){
            if (drvl1_sdc_data_get(device_id, buffer, C_SDC_L1_DATA_FULL_TIMEOUT)) {
                if (poll_status) {
                    *poll_status = C_DMA_STATUS_TIMEOUT;
                }
                return -1;
            }
        }else{
            if (drvl1_sdc30_data_get(device_id, buffer, C_SDC_L1_DATA_FULL_TIMEOUT)) {
                if (poll_status) {
                    *poll_status = C_DMA_STATUS_TIMEOUT;
                }
                return -1;
            }
        }
		buffer++;
		count--;
	}
	if (poll_status) {
		*poll_status = C_DMA_STATUS_DONE;
	}

	return 0;
#endif
}

static volatile INT32U sd30_Status;
static INT32U read_start = 0;
static INT32U read_nsect = 0;

void drvl1_sdc30_set_read_start(INT32U start, INT32U nsect)
{
    read_start = start;
    read_nsect = nsect;

    return;
}

void drvl1_sdc30_clear_read_start(void)
{
    read_start = 0;
    read_nsect = 0;

    return;
}

sd30Reg_t save_sd30Reg;

void drvl1_sdc30_save_sd30_reg(void)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(1);

	save_sd30Reg.Ctrl = sd->Ctrl;
	save_sd30Reg.IntEn = sd->IntEn;
	save_sd30Reg.Ctrl2 = sd->Ctrl2;
	save_sd30Reg.Clk_Dly = sd->Clk_Dly;
	save_sd30Reg.Cmd_Dly = sd->Cmd_Dly;
	save_sd30Reg.Data0_Dly = sd->Data0_Dly;
	save_sd30Reg.Data1_Dly = sd->Data1_Dly;
	save_sd30Reg.Data2_Dly = sd->Data2_Dly;
	save_sd30Reg.Data3_Dly = sd->Data3_Dly;
    save_sd30Reg.Delay_Offset = sd->Delay_Offset;
    save_sd30Reg.Pre_Post_Phase = sd->Pre_Post_Phase;
    save_sd30Reg.SlowClk = sd->SlowClk;
    save_sd30Reg.ClockIdle = sd->ClockIdle;
}

void drvl1_sdc30_restore_sd30_reg(void)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(1);

    sd->Ctrl = save_sd30Reg.Ctrl;
    sd->IntEn = save_sd30Reg.IntEn;
    sd->Ctrl2 = save_sd30Reg.Ctrl2;
    sd->Clk_Dly = save_sd30Reg.Clk_Dly;
    sd->Cmd_Dly = save_sd30Reg.Cmd_Dly;
    sd->Data0_Dly = save_sd30Reg.Data0_Dly;
    sd->Data1_Dly = save_sd30Reg.Data1_Dly;
    sd->Data2_Dly = save_sd30Reg.Data2_Dly;
    sd->Data3_Dly = save_sd30Reg.Data3_Dly;
    sd->Delay_Offset = save_sd30Reg.Delay_Offset;
    sd->Pre_Post_Phase = save_sd30Reg.Pre_Post_Phase;
    sd->SlowClk = save_sd30Reg.SlowClk;
    sd->ClockIdle = save_sd30Reg.ClockIdle;
}

void drvl1_sdc30_dma_reset_abnormal(void)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(1);

    sd->Status = C_SDC_STATUS_ALL_COMPLETE | C_SDC_STATUS_DMA_XFER_COMPLETE;
    drvl1_sdc30_save_sd30_reg();
    xQueueReset(sd_dma_wait_queue);
    drvl1_sdc30_module_reset();
    drvl1_sdc30_restore_sd30_reg();
    drvl1_sdc30_clear_read_start();
}

/**
 * @brief   SD3.0 controller read by DMA extend base function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @param   *poll_status[in]: DMA polling status
 * @param   polling[]: using polling method
 * @return 	0: success, -1: fail.
*/
// williamyeo-sd30-dmablksts-not-match-new-patch
INT32S drvl1_sdc30_read_data_by_dma_ext_base(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status, INT8U polling)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
    static INT8U sdc_dbg_check_ext_wait = 0;
    TickType_t starttick = 0;
    TickType_t tick;
    INT32U dma_progress_length;
    int length_diff = 0;
	INT8U happen_all_complete = 0;
	INT8U happen_timeout = 0;
	INT8U happen_xfer_complete = 0;
    TickType_t xTicksToWait;

    sd30_Status = 0;

    if (sdc_dbg_check_ext_wait == 0)
    {
        if ((sd->Ctrl & C_SDC_EXT_WAIT_ENABLE) != 0)
            DBG_PRINT("C_SDC_EXT_WAIT_ENABLE is on.  0x%08x \r\n", sd->Ctrl);
        sdc_dbg_check_ext_wait = 1;
    }

    static INT8U sdc_dbg_check_dummy_en = 0;

    if (sdc_dbg_check_dummy_en == 0)
    {
        if ((sd->Ctrl & C_SDC_CTL_DUMMYCLK) != 0)
            DBG_PRINT("C_SDC_CTL_DUMMYCLK is on.  0x%08x \r\n", sd->Ctrl);
        sdc_dbg_check_dummy_en = 1;
    }


#if (defined _DRV_L1_DMA) && (_DRV_L1_DMA == 1)
    xTicksToWait = (sector_count <= 8) ? 0 : 1;

    xQueueReset(sd_dma_wait_queue);
    sd_dma_wait_msg = C_DMA_STATUS_WAITING;
    sd->Status = C_SDC_STATUS_DATA_COMPLETE | C_SDC_STATUS_ALL_COMPLETE | C_SDC_STATUS_DMA_XFER_COMPLETE; // clear complete
    starttick = xTaskGetTickCount();

    sd->DmaAddr = (INT32U)buffer;

//DBG_PRINT("sd dma blk:0x%x, sd dma addr:0x%x\r\n", sd->DmaBlk, sd->DmaAddr);

    volatile INT32U DmaBlkSts = sd->DmaBlkSts;

    while(1) {
        if ((sd->Status & C_SDC_STATUS_DMA_XFER_COMPLETE) != 0)
            happen_xfer_complete = 1;

        if ((sd30_Status & C_SDC_STATUS_DMA_XFER_COMPLETE) != 0)
            happen_xfer_complete = 1;

        if ((sd30_Status & C_SDC_STATUS_ALL_COMPLETE) != 0)
			happen_all_complete = 1;

        if(xQueueReceive(sd_dma_wait_queue, &sd_dma_wait_msg, xTicksToWait) == pdPASS) {
            if(sd_dma_wait_msg != C_DMA_STATUS_DONE) {
                tick = xTaskGetTickCount() - starttick;
                if (sd_dma_wait_msg == 0x15) {
                    DBG_PRINT("\r\nsd_dma_wait_msg == 0x15; tick=%u, sd dmablksts=0x%x, sd dmablk=0x%x, Status=0x%x, Status2=0x%x, %u, 0x%x, 0x%x, %u\r\n", \
                        tick, sd->DmaBlkSts, sd->DmaBlk, sd->Status, sd30_Status, read_nsect, buffer, sd->DmaAddr, sector_count);
                    xQueueReset(sd_dma_wait_queue);
                    drvl1_sdc30_clear_read_start();
                    return -4;
                }
                DBG_PRINT("\r\n!= C_DMA_STATUS_DONE; sd dmablksts=0x%x, tick=%u, sd dmablk=0x%x, Status=0x%x, Status2=0x%x, %u, 0x%x, 0x%x\r\n", \
                    tick, sd->DmaBlkSts, sd->DmaBlk, sd->Status, sd30_Status, read_nsect, buffer, sd->DmaAddr);
                xQueueReset(sd_dma_wait_queue);
                drvl1_sdc30_clear_read_start();
                return -1;
            } else if (happen_all_complete) {
                dma_progress_length = (INT32U)(sd->DmaAddr) - (INT32U)buffer;
                length_diff = (sector_count << 9) - dma_progress_length;
                tick = xTaskGetTickCount() - starttick;
                DmaBlkSts = sd->DmaBlkSts;
                if (DmaBlkSts < sd->DmaBlk) {
                    DBG_PRINT("\r\nC_DMA_STATUS_DONE, tick=%u, diff=%d, dmablksts=0x%x, dmablk=0x%x, Status=0x%x, Status2=0x%x, %u, 0x%x, 0x%x, %u, 0x%x\r\n", \
                        tick, length_diff, sd->DmaBlkSts, sd->DmaBlk, sd->Status, sd30_Status, read_nsect, buffer, sd->DmaAddr, sector_count, DmaBlkSts);
                    xQueueReset(sd_dma_wait_queue);
                    drvl1_sdc30_clear_read_start();
                    return -2;
                }
                if (length_diff || (DmaBlkSts > sd->DmaBlk)) {
                    DBG_PRINT("\r\nlength_diff 2 %u; tick=%d, sd dmablksts=0x%x, sd dmablk=0x%x, Status=0x%x, Status2=0x%x, %u, 0x%x, 0x%x, %u, 0x%x\r\n", \
                            length_diff, tick, sd->DmaBlkSts, sd->DmaBlk, sd->Status, sd30_Status, read_nsect, buffer, sd->DmaAddr, sector_count, DmaBlkSts);
                }
                break;
            }
        }

        if(drvl1_sdc30_data_crc_status_get(device_id)) {
            xQueueReset(sd_dma_wait_queue);
            drvl1_sdc30_clear_read_start();
            DBG_PRINT("1 rSD30 crc err!\r\n");
            return 1;
        }

		tick = xTaskGetTickCount() - starttick;
		if (tick >= (210*sector_count))
			happen_timeout = 1;

		if (happen_all_complete || happen_timeout || (read_nsect == 1 && happen_xfer_complete)) {
            dma_progress_length = (INT32U)(sd->DmaAddr) - (INT32U)buffer;
            length_diff = (sector_count << 9) - dma_progress_length;
            DmaBlkSts = sd->DmaBlkSts;
            if (DmaBlkSts >= sd->DmaBlk) {

                if (DmaBlkSts != sd->DmaBlk) {
                    if (read_nsect != 1)
                        DBG_PRINT("\r\nG -7; tick=%u, diff=%d,%u,%u,%u dmablksts=0x%x, dmablk=0x%x, Status=0x%x, Status2=0x%x, %u, 0x%x, 0x%x, %u, 0x%x\r\n", \
                            tick, length_diff, happen_timeout, happen_all_complete, happen_xfer_complete, sd->DmaBlkSts, sd->DmaBlk, sd->Status, sd30_Status, read_nsect, buffer, sd->DmaAddr, sector_count, DmaBlkSts);
                }

                if (length_diff || ((!(read_nsect == 1 && happen_xfer_complete)) && (DmaBlkSts > sd->DmaBlk)) ) {
					DBG_PRINT("\r\nH -6; tick=%u, diff=%d,%u,%u,%u dmablksts=0x%x, dmablk=0x%x, Status=0x%x, Status2=0x%x, %u, 0x%x, 0x%x, %u, 0x%x\r\n", \
                       tick, length_diff, happen_timeout, happen_all_complete, happen_xfer_complete, sd->DmaBlkSts, sd->DmaBlk, sd->Status, sd30_Status, read_nsect, buffer, sd->DmaAddr, sector_count, DmaBlkSts);
                }
                if (!happen_all_complete)
                    xQueueReset(sd_dma_wait_queue);
				break;
            }
			else if (happen_timeout) {
				DBG_PRINT("\r\nSD_timeout2; tick=%u, diff=%d,%u,%u,%u dmablksts=0x%x, dmablk=0x%x, Status=0x%x, Status2=0x%x, %u, 0x%x, 0x%x, %u, 0x%x, start %u\r\n", \
					tick, length_diff, happen_timeout, happen_all_complete, happen_xfer_complete, sd->DmaBlkSts, sd->DmaBlk, sd->Status, sd30_Status, read_nsect, buffer, sd->DmaAddr, sector_count, DmaBlkSts, read_start);
				xQueueReset(sd_dma_wait_queue);
				drvl1_sdc30_clear_read_start();
				return -6;
			}
		}
    } // end while

    if (poll_status) {
        *poll_status = C_DMA_STATUS_DONE;
    }

    drvl1_sdc30_clear_read_start();

    return 0;
#else
	INT32U count;

	count = (INT32U) (C_SDC_SECTOR_SIZE/4) * sector_count;
	while (count) {
        if(device_id == 0){
            if (drvl1_sdc_data_get(device_id, buffer, C_SDC_L1_DATA_FULL_TIMEOUT)) {
                if (poll_status) {
                    *poll_status = C_DMA_STATUS_TIMEOUT;
                }
                return -1;
            }
        }else{
            if (drvl1_sdc30_data_get(device_id, buffer, C_SDC_L1_DATA_FULL_TIMEOUT)) {
                if (poll_status) {
                    *poll_status = C_DMA_STATUS_TIMEOUT;
                }
                return -1;
            }
        }
		buffer++;
		count--;
	}
	if (poll_status) {
		*poll_status = C_DMA_STATUS_DONE;
	}

	return 0;
#endif
}

/**
 * @brief   SD2.0 controller read by DMA extend function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @param   *poll_status[in]: DMA polling status
 * @param   polling[]: using polling method
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_read_data_by_dma_ext(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status, INT8U polling)
{
	INT32S ret = -1;
	INT32U dma_bulk_sector = K_SD_READ_DMA_BULK_SECTOR;
	INT32U remain_sector_count = sector_count;

    dma_bulk_sector = sector_count; // since at dma, we have implemented no data timeout for card off, so we could use full dma size now

	while (remain_sector_count)
	{
		if (remain_sector_count < dma_bulk_sector)
			dma_bulk_sector = remain_sector_count;
		ret = drvl1_sdc_read_data_by_dma_ext_base(device_id, buffer, dma_bulk_sector, poll_status, polling);
		if (ret != 0)
			break;
		buffer += (128 * dma_bulk_sector); // since buffer is INT32U pointer, so 128x4 = 512
		remain_sector_count -= dma_bulk_sector;
	}
	return ret;
}


/**
 * @brief   SD3.0 controller read by DMA extend function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @param   *poll_status[in]: DMA polling status
 * @param   polling[]: using polling method
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_read_data_by_dma_ext(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status, INT8U polling)
{
	INT32S ret = -1;
	INT32U dma_bulk_sector = K_SD_READ_DMA_BULK_SECTOR;
	INT32U remain_sector_count = sector_count;
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    dma_bulk_sector = sector_count; // since at dma, we have implemented no data timeout for card off, so we could use full dma size now

	while (remain_sector_count)
	{
        if (remain_sector_count == (dma_bulk_sector + 1))
            dma_bulk_sector += 1;
        if (remain_sector_count < dma_bulk_sector)
			dma_bulk_sector = remain_sector_count;
        sd->DmaBlk = dma_bulk_sector;
        cache_invalid_range((INT32U)buffer, dma_bulk_sector*512);
		ret = drvl1_sdc30_read_data_by_dma_ext_base(device_id, buffer, dma_bulk_sector, poll_status, polling);
		if (ret != 0)
			break;
		buffer += (128 * dma_bulk_sector); // since buffer is INT32U pointer, so 128x4 = 512
		remain_sector_count -= dma_bulk_sector;
	}
	return ret;
}

/**
 * @brief   SD2.0 controller read by DMA function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @param   *poll_status[in]: DMA polling status
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_read_data_by_dma(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status)
{
	INT32U ret;

	ret = drvl1_sdc_read_data_by_dma_ext(device_id, buffer, sector_count, poll_status, 0);

	return ret;
}

/**
 * @brief   SD3.0 controller read by DMA function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @param   *poll_status[in]: DMA polling status
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_read_data_by_dma(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status)
{
	INT32U ret;

	ret = drvl1_sdc30_read_data_by_dma_ext(device_id, buffer, sector_count, poll_status, 0);

	return ret;
}

/**
 * @brief   SD2.0 controller read by DMA extend base function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: write buffer.
 * @param   sector_count[in]: write sector count.
 * @param   *poll_status[in]: DMA polling status
 * @param   polling[]: using polling method
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_write_data_by_dma_ext_base(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status, INT8U polling)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

#if (defined _DRV_L1_DMA) && (_DRV_L1_DMA == 1)
	DMA_STRUCT dma_struct={0};
	volatile INT8S done;

	dma_struct.s_addr = (INT32U) buffer;
	dma_struct.t_addr = (INT32U) &sd->DataTx;
	dma_struct.width = DMA_DATA_WIDTH_4BYTE;
	dma_struct.count = (INT32U) (C_SDC_SECTOR_SIZE/4) * sector_count;

	//dma_struct.timeout = drvl1_sdc_cal_write_timeout(sector_count); // this time out 1/256 sec unit
	dma_struct.timeout = 0; // use software timeout of dma, since our dma longest timeout is 1s, but disccover some sd card require more than 1 1s

	if (poll_status) {
		*poll_status = C_DMA_STATUS_WAITING;
		dma_struct.notify = poll_status;

	// Send dma request to driver and return immediately
		if (drv_l1_dma_transfer(&dma_struct)) {
			return -1;
		}
		if (polling)
		{
			INT32U timeout_count = (INT32U)255 * 1638 * 2;
			INT32U msecwait;
			INT32U sd_spec_sector_timeout = K_SD_WRITE_TIMEOUT_PER_SECTOR; // in ms unit

			msecwait = (sector_count * sd_spec_sector_timeout) + K_SD_WRITE_TIMEOUT_PER_SECTOR + 615;
			msecwait = (msecwait * 1016) / 1000;

			#if _OPERATING_SYSTEM == _OS_FREERTOS
			timeout_count = msecwait;
			#else
			timeout_count = msecwait * 376UL;
			#endif
			if (drvl1_sdc_busy_polling(device_id, 0, dma_struct.channel, timeout_count, drvl1_sdc_dma_poll_condition_proc, (INT32U)poll_status))
			{
				drv_l1_dma_enforce_timeout((INT32U)dma_struct.channel);
				return -1;
			}
			if ((*poll_status) == C_DMA_STATUS_DONE) {
				return 0;
            }
			else
			{
				return -1;
			}
		}
	} else {
        dma_struct.timeout = 0; // since our dma longest timeout is 1s, but disccover some sd card require more than 1 1s
                                // so set to 0 to disable dma timeout but use os timer to timeout on 1.5s
		done = C_DMA_STATUS_WAITING;
		dma_struct.notify = &done;

		// Send dma request to driver and wait response
		if (drv_l1_dma_transfer_wait_ready(&dma_struct)) {
			return -1;
		}
	}

	return 0;
#else
	INT32U count;

	count = (INT32U) (C_SDC_SECTOR_SIZE/4) * sector_count;
	while (count) {
		if (drvl1_sdc_status_set_wait(device_id, C_SDC_STATUS_DATA_BUF_EMPTY, C_SDC_L1_DATA_EMPTY_TIMEOUT)) {
			if (poll_status) {
				*poll_status = C_DMA_STATUS_TIMEOUT;
			}
			return -1;
		}
		sd->DataTx = *buffer;
		buffer++;
		count--;
	}
	if (poll_status) {
		*poll_status = C_DMA_STATUS_DONE;
	}

	return 0;
#endif
}

/**
 * @brief   SD3.0 controller read by DMA extend base function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @param   *poll_status[in]: DMA polling status
 * @param   polling[]: using polling method
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_write_data_by_dma_ext_base(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status, INT8U polling)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	TickType_t starttick = 0;

#if (defined _DRV_L1_DMA) && (_DRV_L1_DMA == 1)
    xQueueReset(sd_dma_wait_queue);
    sddma_notify = C_DMA_STATUS_WAITING;
    sd->DmaBlk = sector_count;
    sd->DmaAddr = (INT32U)buffer;
#if 0
    if(xQueueReceive(sd_dma_wait_queue, &sd_dma_wait_msg, (615+250+250*sector_count)) != pdPASS)
    {
        DBG_PRINT("SD_timeout3\r\n");
        return -1;
    }
    else if(sd_dma_wait_msg != C_DMA_STATUS_DONE)
    {
        if(sd_dma_wait_msg == 0x15)
            return -4;
        return -1;
    }
#else
    starttick = xTaskGetTickCount();
    while(1) {
        if(xQueueReceive(sd_dma_wait_queue, &sd_dma_wait_msg, 1) == pdPASS) {
            if(sd_dma_wait_msg != C_DMA_STATUS_DONE) {
                if(sd_dma_wait_msg == 0x15)
                    return -4;
                return -1;
            } else {
                break;
            }
        }

        #if 1 // SD L2 dma check this already, no need to check here. Restore this, it could fix quickly detect card out.
        if(drvl1_sdc30_data_crc_status_get(device_id)) {
            DBG_PRINT("1 wSD30 crc err!\r\n");
            xQueueReset(sd_dma_wait_queue);
            return 1;
        }
        #endif

        if((xTaskGetTickCount()-starttick) >= (615+250+250*sector_count)) {
            DBG_PRINT("SD_timeout3\r\n");
            xQueueReset(sd_dma_wait_queue);
            return -1;
        }
    }
#endif
    //DBG_PRINT("W sd dmablksts=0x%x, sd dmablk=0x%x\r\n", sd->DmaBlkSts, sd->DmaBlk);
    starttick = xTaskGetTickCount();
    while(sd->DmaBlkSts != sd->DmaBlk)
    {
        if((xTaskGetTickCount()-starttick) > 100) {
            xQueueReset(sd_dma_wait_queue);
            return -1;
        }
    }
    if (poll_status) {
        *poll_status = C_DMA_STATUS_DONE;
    }
    return 0;
#else
	INT32U count;

	count = (INT32U) (C_SDC_SECTOR_SIZE/4) * sector_count;
	while (count) {
		if (drvl1_sdc_status_set_wait(device_id, C_SDC_STATUS_DATA_BUF_EMPTY, C_SDC_L1_DATA_EMPTY_TIMEOUT)) {
			if (poll_status) {
				*poll_status = C_DMA_STATUS_TIMEOUT;
			}
			return -1;
		}
		sd->DataTx = *buffer;
		buffer++;
		count--;
	}
	if (poll_status) {
		*poll_status = C_DMA_STATUS_DONE;
	}

	return 0;
#endif
}

/**
 * @brief   SD2.0 controller write by DMA extend function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: write buffer.
 * @param   sector_count[in]: write sector count.
 * @param   *poll_status[in]: DMA polling status
 * @param   polling[]: using polling method
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_write_data_by_dma_ext(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status, INT8U polling)
{
	INT32S ret = -1;
	INT32U dma_bulk_sector = K_SD_WRITE_DMA_BULK_SECTOR;
	INT32U remain_sector_count = sector_count;

    dma_bulk_sector = sector_count; // since at dma, we have implemented no data timeout for card off, so we could use full dma size now

	while (remain_sector_count)
	{
		if (remain_sector_count < dma_bulk_sector)
			dma_bulk_sector = remain_sector_count;
		ret = drvl1_sdc_write_data_by_dma_ext_base(device_id, buffer, dma_bulk_sector, poll_status, polling);
		if (ret != 0)
			break;
		buffer += (128 * dma_bulk_sector); // since buffer is INT32U pointer, so 128x4 = 512
		remain_sector_count -= dma_bulk_sector;
	}
	return ret;
}

/**
 * @brief   SD3.0 controller write by DMA extend function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: write buffer.
 * @param   sector_count[in]: write sector count.
 * @param   *poll_status[in]: DMA polling status
 * @param   polling[]: using polling method
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_write_data_by_dma_ext(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status, INT8U polling)
{
	INT32S ret = -1;
	INT32U dma_bulk_sector = K_SD_WRITE_DMA_BULK_SECTOR;
	INT32U remain_sector_count = sector_count;

    dma_bulk_sector = sector_count; // since at dma, we have implemented no data timeout for card off, so we could use full dma size now

	while (remain_sector_count)
	{
		if (remain_sector_count < dma_bulk_sector)
			dma_bulk_sector = remain_sector_count;
        //drv_l1_cp15_clean_dcache_range(dma_bulk_sector, sizeof(INT32U));
        drv_l1_cp15_clean_dcache_range((INT32U)buffer, dma_bulk_sector*512);
		ret = drvl1_sdc30_write_data_by_dma_ext_base(device_id, buffer, dma_bulk_sector, poll_status, polling);
		if (ret != 0)
			break;
		buffer += (128 * dma_bulk_sector); // since buffer is INT32U pointer, so 128x4 = 512
		remain_sector_count -= dma_bulk_sector;
	}
	return ret;
}

/**
 * @brief   SD2.0 controller write by DMA function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: write buffer.
 * @param   sector_count[in]: write sector count.
 * @param   *poll_status[in]: DMA polling status
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_write_data_by_dma(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status)
{
	INT32U ret;

	ret = drvl1_sdc_write_data_by_dma_ext(device_id, buffer, sector_count, poll_status, 0);

	return ret;
}

/**
 * @brief   SD3.0 controller write by DMA function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: write buffer.
 * @param   sector_count[in]: write sector count.
 * @param   *poll_status[in]: DMA polling status
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_write_data_by_dma(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status)
{
	INT32U ret;

	ret = drvl1_sdc30_write_data_by_dma_ext(device_id, buffer, sector_count, poll_status, 0);

	return ret;
}

/**
 * @brief   SD2.0 controller polling data complete by a timeout value.
 * @param   device_id[in]: device id.
 * @param   timeout[in]: timeout value
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc_data_complete_wait(INT32U device_id, INT32U timeout)
{
	if (drvl1_sdc_status_set_wait(device_id, C_SDC_STATUS_DATA_BUF_EMPTY, timeout)) {
		return -1;
	}

	return 0;
}

/**
 * @brief   SD3.0 controller polling data complete by a timeout value.
 * @param   device_id[in]: device id.
 * @param   timeout[in]: timeout value
 * @return 	0: success, -1: fail.
*/
INT32S drvl1_sdc30_data_complete_wait(INT32U device_id, INT32U timeout)
{
	if (drvl1_sdc30_status_set_wait(device_id, C_SDC_STATUS_DATA_BUF_EMPTY, timeout)) {
		return -1;
	}

	return 0;
}

/**
 * @brief   Get SD2.0 controller data CRC status.
 * @param   device_id[in]: device id.
 * @return 	0: CRC no error, -1: CRC error.
*/
INT32S drvl1_sdc_data_crc_status_get(INT32U device_id)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	if (sd->Status & C_SDC_STATUS_DATA_CRC_ERR) {
		return -1;
	}

	return 0;
}

/**
 * @brief   Get SD3.0 controller data CRC status.
 * @param   device_id[in]: device id.
 * @return 	0: CRC no error, -1: CRC error.
*/
INT32S drvl1_sdc30_data_crc_status_get(INT32U device_id)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	if (sd->Status & C_SDC_STATUS_DATA_CRC_ERR) {
		return -1;
	}

	return 0;
}

/**
 * @brief   Clear SD2.0 controller timeout bit by a polling count.
 * @param   device_id[in]: device id.
 * @param   poll_cnt[in]: polling count.
 * @return 	0: clear timeout bit successful, -1: clear timeout bit fail.
*/
INT32S drvl1_sdc_clear_timeout_bit(INT32U device_id, INT32U poll_cnt)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	INT32U k;

	if (poll_cnt == 0)
		poll_cnt = 1;

	for (k = 0; k < poll_cnt; k++)
	{
		sd->Status = C_SDC_STATUS_TIMEOUT;
		if ((sd->Status & C_SDC_STATUS_TIMEOUT) == 0)
		{
			return 0;
		}
	}
	return -1; // fail to clear C_SDC_STATUS_TIMEOUT
}

/**
 * @brief   Clear SD3.0 controller timeout bit by a polling count.
 * @param   device_id[in]: device id.
 * @param   poll_cnt[in]: polling count.
 * @return 	0: clear timeout bit successful, -1: clear timeout bit fail.
*/
INT32S drvl1_sdc30_clear_timeout_bit(INT32U device_id, INT32U poll_cnt)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	INT32U k;

	if (poll_cnt == 0)
		poll_cnt = 1;

	for (k = 0; k < poll_cnt; k++)
	{
		sd->Status = C_SDC_STATUS_TIMEOUT;
		if ((sd->Status & C_SDC_STATUS_TIMEOUT) == 0)
		{
			return 0;
		}
	}
	return -1; // fail to clear C_SDC_STATUS_TIMEOUT
}


/**
 * @brief   Adjust SD3.0 controller input clock phase.
 * @param   device_id[in]: device id.
 * @param   dstep[in]: delay step.
 * @return 	none.
*/
void drvl1_sdc30_sdclk_delay_set(INT32U device_id, INT8U dstep)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    sd->Clk_Dly = dstep&0x1F;
    //DBG_PRINT("sd clk delay=0x%x\r\n", sd->Clk_Dly);
}

/**
 * @brief   Adjust SD3.0 controller clock offset.
 * @param   device_id[in]: device id.
 * @param   offset[in]: 0~7.
 * @return 	none.
*/
void drvl1_sdc30_sdclk_delay_offset_set(INT32U device_id, INT8U offset)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    sd->Delay_Offset = offset&0x7;
}

/**
 * @brief   Adjust SD3.0 controller clock offset.
 * @param   device_id[in]: device id.
 * @return 	0,1,2,3, 3 is good result
*/
INT8U drvl1_sdc30_sdclk_delay_offset_result(INT32U device_id)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
    INT8U result;

    result = sd->Pre_Post_Phase&0x3;

    return result;
}

/**
 * @brief   Adjust SD3.0 controller input command phase.
 * @param   device_id[in]: device id.
 * @param   dstep[in]: delay step.
 * @return 	none.
*/
void drvl1_sdc30_sdcmd_delay_set(INT32U device_id, INT8U dstep)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    sd->Cmd_Dly = dstep&0x1F;
    //DBG_PRINT("sd cmd delay 0x%x\r\n", sd->Cmd_Dly);
}

/**
 * @brief   Adjust SD3.0 controller data phase.
 * @param   device_id[in]: device id.
 * @param   dstep[in]: delay step.
 * @return 	none.
*/
void drvl1_sdc30_sddata_delay_set(INT32U device_id, INT8U dstep)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    sd->Data0_Dly = dstep&0x1F;
    sd->Data1_Dly = dstep&0x1F;
    sd->Data2_Dly = dstep&0x1F;
    sd->Data3_Dly = dstep&0x1F;
    //DBG_PRINT("sd data[3:0] delay 0x%x 0x%x 0x%x 0x%x\r\n", sd->Data3_Dly, sd->Data2_Dly, sd->Data1_Dly, sd->Data0_Dly);
}

void drvl1_sdc30_tune_timing_print(INT32U device_id)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    DBG_PRINT("sd30_tune_timing cmd=0x%x clk=0x%x offset=0x%x pre_post=0x%x data[3:0]=0x%x 0x%x 0x%x 0x%x\r\n", \
                sd->Cmd_Dly, sd->Clk_Dly, sd->Delay_Offset, sd->Pre_Post_Phase, \
                sd->Data3_Dly, sd->Data2_Dly, sd->Data1_Dly, sd->Data0_Dly);
}

/**
 * @brief   Set SD3.0 controller speed mode.
 * @param   device_id[in]: device id.
 * @param   mode[in]: standard speed mode or high speed mode.
 * @return 	none.
*/
void drvl1_sdc30_set_sdmode(INT32U device_id, INT8U mode)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    if(mode>=3)
    {
        return;
    }
    sd->Ctrl2 = ((sd->Ctrl2&0xfffffcff) | (mode<<4));
}

/**
 * @brief   SD2.0 controller IRQ handler.
 * @param   none.
 * @return 	none.
*/
void SDC0_IRQHandler(void)
{
    sdReg_t *sd = (sdReg_t*) SetSDBase(0);
    INT32U sd_inten;

    sd_inten = sd->IntEn;
    if (sd_inten) {
        //DBG_PRINT("sd-%d have interrupt\r\n", sd_module);

        if((sd_inten & C_SDC_INT_CMD_COMP) && (sd->Status & C_SDC_STATUS_CMD_COMPLETE))
        {
            sd->Status = C_SDC_STATUS_CMD_COMPLETE;
            DBG_PRINT("sd-0 C_SDC_INT_CMD_COMP\r\n");
        }
        if((sd_inten & C_SDC_INT_DATA_COMP) && (sd->Status & C_SDC_STATUS_DATA_COMPLETE))
        {
            sd->Status = C_SDC_STATUS_DATA_COMPLETE;
            DBG_PRINT("sd-0 C_SDC_INT_DATA_COMP\r\n");
        }
        if((sd_inten & C_SDC_INT_RESP_REG_FULL) && (sd->Status & C_SDC_STATUS_RESP_REG_FULL))
        {
            sd->Status = C_SDC_STATUS_RESP_REG_FULL;
            DBG_PRINT("sd-0 C_SDC_INT_RESP_FULL\r\n");
        }
        if((sd_inten & C_SDC_INT_DATABUF_FULL) && (sd->Status & C_SDC_STATUS_DATA_BUF_FULL))
        {
            sd->Status = C_SDC_STATUS_DATA_BUF_FULL;
            DBG_PRINT("sd-0 C_SDC_INT_DBUF_FULL\r\n");
        }
        if((sd_inten & C_SDC_INT_DATABUF_EMPTY) && (sd->Status & C_SDC_STATUS_DATA_BUF_EMPTY))
        {
            sd->Status = C_SDC_STATUS_DATA_BUF_EMPTY;
            DBG_PRINT("sd-0 C_SDC_INT_DBUF_EMPT\r\n");
        }
        if((sd_inten & C_SDC_INT_PRESENT))
        {
            if (sd->Status & C_SDC_STATUS_CARD_PRESENT) {
                sd->Status = C_SDC_STATUS_CARD_PRESENT;
                DBG_PRINT("sd-0 C_SDC_STATUS_CARD_PRESENT\r\n");
            } else {
                DBG_PRINT("sd-0 C_SDC_STATUS_CARD_NOT_PRESENT\r\n");
            }
        }
        if((sd_inten & C_SDC_INT_SDIO_INT) && (sd->Status & C_SDC_STATUS_SDIO_INT_PEND))
        {
            sd->Status = C_SDC_STATUS_SDIO_INT_PEND;
            DBG_PRINT("sd-0 C_SDC_INT_SDIO_INT\r\n");
        }
    }
}

/**
 * @brief   SD3.0 controller IRQ handler.
 * @param   none.
 * @return 	none.
*/
void SDC1_IRQHandler(void)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(1);
    INT32U sd_inten;
    portBASE_TYPE taskWoken = pdFALSE;

    sd_inten = sd->IntEn;
    if((sd_inten & C_SDC_INT_CMD_COMP) && (sd->Status & C_SDC_STATUS_CMD_COMPLETE))
    {
        sd->Status = C_SDC_STATUS_CMD_COMPLETE;
    }
    if((sd_inten & C_SDC_INT_DATA_COMP) && (sd->Status & C_SDC_STATUS_DATA_COMPLETE))
    {
        sd->Status = C_SDC_STATUS_DATA_COMPLETE;
    }
    if((sd_inten & C_SDC_INT_RESP_REG_FULL) && (sd->Status & C_SDC_STATUS_RESP_REG_FULL))
    {
        sd->Status = C_SDC_STATUS_RESP_REG_FULL;
    }
    if((sd_inten & C_SDC_INT_DATABUF_FULL) && (sd->Status & C_SDC_STATUS_DATA_BUF_FULL))
    {
        sd->Status = C_SDC_STATUS_DATA_BUF_FULL;
    }
    if((sd_inten & C_SDC_INT_DATABUF_EMPTY) && (sd->Status & C_SDC_STATUS_DATA_BUF_EMPTY))
    {
        sd->Status = C_SDC_STATUS_DATA_BUF_EMPTY;
    }
    if(sd_inten & C_SDC_INT_PRESENT)
    {
        if (sd->Status & C_SDC_STATUS_CARD_PRESENT) {
            sd->Status = C_SDC_STATUS_CARD_PRESENT;
            DBG_PRINT("sd-1 C_SDC_STATUS_CARD_PRESENT\r\n");
        } else {
            DBG_PRINT("sd-1 C_SDC_STATUS_CARD_NOT_PRESENT\r\n");
        }
    }
    if((sd_inten & C_SDC_INT_SDIO_INT) && (sd->Status & C_SDC_STATUS_SDIO_INT_PEND))
    {
        sd->Status = C_SDC_STATUS_SDIO_INT_PEND;
    }
    if((sd_inten & C_SDC_INT_PRESENT) && (sd->Status & C_SDC_STATUS_CARD_PRESENT))
    {
        sd->Status = C_SDC_STATUS_CARD_PRESENT;
    }
    if((sd_inten & C_SDC_INT_DMA_INT) && (sd->Status & C_SDC_STATUS_DMA_XFER_COMPLETE))
    {
 		sd->Status = C_SDC_STATUS_DMA_XFER_COMPLETE;
    }
    if((sd_inten & C_SDC_INT_ALLCOM_INT) && (sd->Status & C_SDC_STATUS_ALL_COMPLETE))
    {
        //DBG_PRINT("[sd dma dOnE!]\r\n");
        sddma_notify = C_DMA_STATUS_DONE;
        if(g_polling_status)
          *g_polling_status = C_DMA_STATUS_DONE;

        xQueueSendFromISR(sd_dma_wait_queue, (const void *)&sddma_notify, &taskWoken);

        #if 1 // williamyeo-sd30-dmablksts-not-match
        sd30_Status = sd->Status;
        sd->Status = C_SDC_STATUS_ALL_COMPLETE | C_SDC_STATUS_DMA_XFER_COMPLETE;
        if ((sd->Status & C_SDC_STATUS_DMA_XFER_COMPLETE) != 0)
            DBG_PRINT("C_SDC_STATUS_DMA_XFER_COMPLETE not clear\r\n");
        if ((sd->Status & C_SDC_STATUS_ALL_COMPLETE) != 0)
            DBG_PRINT("C_SDC_STATUS_ALL_COMPLETE not clear\r\n");
        #endif

        portEND_SWITCHING_ISR(taskWoken);
    }
}

void drvl1_sdc_get_version(void)
{
    DBG_PRINT("L1_SD_LIB_BUILD_TIME: %s\r\n", L1_SD_LIB_BUILD_TIME);
    DBG_PRINT("L1_SD_SVN_NUM: %s\r\n", L1_SD_SVN_VERSION);

    #if GIT_VERSION == 1
    DBG_PRINT("L1_SD_LIB_COMMIT_TIME: %s\r\n", L1_SD_LIB_COMMIT_TIME);
    DBG_PRINT("L1_SD_LIB_BRANCH: %s\r\n", L1_SD_LIB_BRANCH);
    #endif
}

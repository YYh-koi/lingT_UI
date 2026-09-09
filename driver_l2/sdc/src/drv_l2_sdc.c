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
#include "drv_l1_timer.h"
#include "drv_l1_system.h"
#include "drv_l1_sdc.h"
#include "drv_l2_sdc.h"
#include "drv_l1_gpio.h"
#include "drv_l2_sdc_version.h"
#include "drv_l1_clock.h"
#include "gplib_print_string.h"
#include "gp_stdlib.h"
#include "drv_l1_arm_cp15.h"

#if 1//(defined _DRV_L2_SDC) && (_DRV_L2_SDC == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define C_SD_DEBUG_CHECK_FOR_64_ALIGNED             0 // default to 0. when 1, checking buffer whether is 64 aligned, when non aligned print 10 line message.

#define SDC_AU_SIZE_USE_6M                          1

#define SDC_FS_WRITE_PATH_USE_START_STOP			1
#define SDC_FS_READ_PATH_USE_START_STOP				1


#define SDC_FS_WRITE_PATH_USE_DMA_MSG			    1 // 1 use dma message to wait, 0 use notify busy polling
                                                      // have effect only when SDC_FS_WRITE_PATH_USE_START_STOP is 1
#define SDC_FS_READ_PATH_USE_DMA_MSG				1 // 1 use dma message to wait, 0 use notify busy polling
                                                      // have effect only when SDC_FS_READ_PATH_USE_START_STOP is 1

#define SD_RW_RETRY_COUNT           				2  // not use currently

#define SD_DEBUG_SCR_INFO							0

#define SD_DEBUG_OSR_INFO							0  // DEBUG CODE, must off when release

#define SD_DEBUG_CID_INFO							0  // DEBUG CODE, must off when release

#define SD_DEBUG_CSD_INFO							0  // DEBUG CODE, must off when release

#define SD_FORCE_ERASE_ALL							0  // DEBUG CODE, must off when release

#define SD_DEBUG_ENFORCE_STOP						0 // DEBUG CODE, must off when release
													  // when 1, set SD_ENFORCE_STOP_TYPE_ENUM to debug
													  //         and include print out not AU_END type count at card remove
													  // when 2, print sd_read, sd_write elapsed time
													  // when 6, include print out debug message

#define SDC_TOTAL									2

#define C_SD_L2_INIT_RETRY							4

// Timeout value(unit is ms). Maximum timeout value for card initialization is 1 second

#define C_SD_L2_CARD_INIT_TIMEOUT					(600)	// Some cards need 360 ms to response to ACMD41
#define C_SD_L2_CARD_INIT_RETRY_TIMEOUT				(500)	// Some cards need 900 ms to initialize after receiving CMD0
#define C_SD_L2_CARD_PROGRAM_TIMEOUT				(2500*2)	// 750 ms timeout value for writing data to SD card
#define C_SD_L2_CARD_ERASE_TIMEOUT					(2500*2)	// 750 ms timeout value for writing data to SD card

// Timeout value(unit is 2.67us)
#define C_SD_L2_CARD_BUSY_TIMEOUT					(2500*2)
#define C_SD_L2_SINGLE_READ_CMD_COMPLETE_TIMEOUT	(560*2)
#define C_SD_L2_MULTI_READ_CMD_COMPLETE_TIMEOUT		(4300*2)
#define C_SD_L2_SINGLE_WRITE_CMD_COMPLETE_TIMEOUT	(100*10*2)
#define C_SD_L2_MULTI_WRITE_CMD_COMPLETE_TIMEOUT	(100*10*2)
#define C_SD_L2_SINGLE_READ_DATA_COMPLETE_TIMEOUT	(100*10*2)
#define C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT	(100*10*2)
#define C_SD_L2_SINGLE_WRITE_DATA_COMPLETE_TIMEOUT	(100*10*2)
#define C_SD_L2_MULTI_WRITE_DATA_COMPLETE_TIMEOUT	(100*10*2)
#define C_SD_L2_ADTC_DATA_FULL_TIMEOUT				(3400*2)
#define C_SD_L2_RESP_R136_FULL_TIMEOUT				(3000*2)
#define C_SD_L2_RESP_R2_FULL_TIMEOUT				(180*10*2)
#define C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT		(3600*2)
#define C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT		(100*10*2)

#define K_SD_RESPONSE_INIT_VALUE                    0x55AA55AA

#define C_SD_L2_ACCESS_READ             1
#define C_SD_L2_ACCESS_WRITE            2

#define TUNING_CTRL_BL      0x00
#define TUNING_CTRL_BH      0x20
#define TUNING_CTRL_STEP    0x01
#define DEFAULT_SDR12       0
#define HISPEED_SDR25       1
#define SDR50               2

const INT8U DRVL2_sd[] = "GLB_GP-S2_0610L_sd-L2-CS_2.0.10";

typedef enum
{
    SD_FLOW_IDLE=0,
	SD_FLOW_START,
	SD_FLOW_CONTINUE,
	SD_FLOW_STOP,
} SD_FLOW_STATE_ENUM;

typedef enum
{
	SD_FLOW_NONE=0,
    SD_FLOW_READ,
    SD_FLOW_WRITE
} SD_FLOW_KIND_ENUM;

typedef enum
{
	SD_ENFORCE_STOP_TYPE_WRITE_AU_END = 1,
	SD_ENFORCE_STOP_TYPE_WRITE_ONE_SECTOR,
	SD_ENFORCE_STOP_TYPE_WRITE_RANDOM_ACCESS,
	SD_ENFORCE_STOP_TYPE_WRITE_BECAUSE_READ,
	SD_ENFORCE_STOP_TYPE_WRITE_FLUSH,
	SD_ENFORCE_STOP_TYPE_READ_AU_END = 9,
	SD_ENFORCE_STOP_TYPE_READ_ONE_SECTOR,
	SD_ENFORCE_STOP_TYPE_READ_RANDOM_ACCESS,
	SD_ENFORCE_STOP_TYPE_READ_BECAUSE_WRITE,
	SD_ENFORCE_STOP_TYPE_READ_FLUSH,
	SD_ENFORCE_STOP_TYPE_COUNT = 17
} SD_ENFORCE_STOP_TYPE_ENUM;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static SD_CARD_STATE_ENUM sd_card_state[SDC_TOTAL] = {SD_CARD_STATE_INACTIVE, SD_CARD_STATE_INACTIVE};
static INT32U sd_card_rca[SDC_TOTAL];			// 16 bits (31-16)
static ALIGN64 INT32U sd_card_csd[SDC_TOTAL][16];//[4];	// 128 bits, declare ALIGN32 and 16x4 so that 64 byte cache line multiple for safe when dma invalidate cache
static ALIGN64 INT32U sd_card_scr[SDC_TOTAL][16];//[2];	// 64 bits, declare ALIGN32 and 16x4 so that 64 byte cache line multiple for safe when dma invalidate cache
static ALIGN64 INT32U sd_card_cid[SDC_TOTAL][16];//[4];	// 128 bits, declare ALIGN32 and 16x4 so that 64 byte cache line multiple for safe when dma invalidate cache
static INT32U sd_card_ocr[SDC_TOTAL];			// 32 bits
static INT32U sd_card_total_sector[SDC_TOTAL];
static INT32U sd_card_speed[SDC_TOTAL];
static INT32U sd_card_initial_clock_rate[SDC_TOTAL] = {50000000, 74250000}; // 74250000, 59400000
static INT8U sd_card_type[SDC_TOTAL];
static INT8U sd_card_bus_width[SDC_TOTAL];
static INT8U sd_card_protect[SDC_TOTAL];
static INT8U sd_card_user[SDC_TOTAL] = {0, 0};
static INT8U sd_card_use_one_bit_mode[SDC_TOTAL] = {0, 0};
static INT8U sd_card_driver_strength[SDC_TOTAL] = {0, 0}; // 0 is not adjust, 1 mean use strength A, 2~4 not yet implement
                                                          // and currently implment strength just for over sd clock rate 50MHz case
static sd_voltage_get_callback sd_voltage_get_proc[SDC_TOTAL] = {NULL, NULL};
static ALIGN64 INT8U sd_card_tuningblock[SDC_TOTAL][64]; // 512 bits

// when bit 0~1 is not 0, apply read retry when error, when bit 2~3 is not 0, apply write retry when error
static INT8U sd_card_apply_read_retry[SDC_TOTAL] = {1, 1}; // default is no reset sd card
static INT8U sd_card_apply_write_retry[SDC_TOTAL] = {0, 0};

static INT8U sd_card_read_retry_max_timeout[SDC_TOTAL] = {2, 2}; // in second unit, default is 2s
static INT8U sd_card_write_retry_max_timeout[SDC_TOTAL] = {2, 2}; // in second unit, default is 2s

static SD_FLOW_STATE_ENUM sd_flow_state[SDC_TOTAL] = {SD_FLOW_IDLE, SD_FLOW_IDLE};
static SD_FLOW_KIND_ENUM sd_flow_kind[SDC_TOTAL] = {SD_FLOW_NONE, SD_FLOW_NONE};
static INT8U sd_flow_single_sector_mode[SDC_TOTAL] = {0, 0};
static INT32U sd_flow_end_sector[SDC_TOTAL] = {0, 0};
static INT32U sd_flow_nr_sector[SDC_TOTAL] = {0, 0};
static INT32U sd_flow_device_au_sectors[SDC_TOTAL] = {1024, 1024}; // 512K, 512K
static INT32U sd_flow_write_au_sectors[SDC_TOTAL] = {1024, 1024}; // 512K, 512K
static INT32U sd_flow_read_au_sectors[SDC_TOTAL] = {2048*4, 2048*4}; // 4M, 4M, I think au size is for write, so read could set to larger size or no limit.
static INT32U sd_flow_write_au_start_sector[SDC_TOTAL] = {0, 0};

// SD Status related
static ALIGN64 INT8U sd_card_status[SDC_TOTAL][64];		// 512 bits
static INT8U sd_data_bus_width[SDC_TOTAL];
static INT8U sd_secured_mode[SDC_TOTAL];
static INT8U sd_sd_card_type[SDC_TOTAL];
static INT8U sd_speed_class[SDC_TOTAL];
static INT8U sd_uhs_speed_grade[SDC_TOTAL];
static INT32U sd_uhs_au_sectors[SDC_TOTAL];
static INT16U sd_erase_size_au[SDC_TOTAL];
static INT32U sd_card_mclk[2];
static INT8U sd_clk_boundary[2];
static INT8U sd_cmd_data_boundary[2];
static INT8U sd30_tunning_clock_cmd_data_phase_not_allow_reduce_sd_clock[2] = {0, 0};
static INT8U sd_off_drvl2_sdc_read_byte_by_cpu_crc_debug_msg = 0;
static INT8U emmc_enable_RST_n[2] = {0, 0};
static INT8U sd_card_is_emmc[2] = {0, 0};
static INT8U sd_scr_done[2] = {0, 0};

static volatile INT8S sd_write_sector_result[SDC_TOTAL];
extern INT32U SD3MCLK;
static INT64U sd_card_volume[SDC_TOTAL];
static	xSemaphoreHandle gSD_sem[SDC_TOTAL] = {NULL, NULL};
const static INT8U speed_table[16] = {0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80};
const static INT32U tran_table[4] = {10000, 100000, 1000000, 10000000};
static INT32S sd_sema_count[SDC_TOTAL] = {0, 0};
static INT8U sd_speed_mode = DEFAULT_SDR12;
static INT8U sd_support_1p8v = 0;
static INT8U sd_switch_1p8v_fail = 0;

static ALIGN64 char const sd_tuning_pattern[64] =
{
    0xFF, 0x0F, 0xFF, 0x00, 0xFF, 0xCC, 0xC3, 0xCC,
    0xC3, 0x3C, 0xCC, 0xFF, 0xFE, 0xFF, 0xFE, 0xEF,
    0xFF, 0xDF, 0xFF, 0xDD, 0xFF, 0xBF, 0xFF, 0xFB,
    0xBF, 0xFF, 0x7F, 0xFF, 0x77, 0xF7, 0xBD, 0xEF,
    0xFF, 0xF0, 0xFF, 0xF0, 0x0F, 0xFC, 0xCC, 0x3C,
    0xCC, 0x33, 0xCC, 0xCF, 0xFF, 0xEF, 0xFF, 0xEE,
    0xFF, 0xFD, 0xFF, 0xFD, 0xDF, 0xFF, 0xBF, 0xFF,
    0xBB, 0xFF, 0xF7, 0xFF, 0xF7, 0x7F, 0x7B, 0xDE
};

static INT32U sd_tunning_pattern_sector_addr[2] = {0x0, 0x0};
static INT32U sd_tunning_block_sector_addr[2] = {0x0, 0x0};

#if (SD_DEBUG_ENFORCE_STOP > 0)
static INT8U sd_endforce_stop_type[SDC_TOTAL] = {0, 0};
#endif

static INT16U sd_endforce_stop_type_count[SDC_TOTAL][SD_ENFORCE_STOP_TYPE_COUNT];
static char *sd_endforce_stop_type_name[SD_ENFORCE_STOP_TYPE_COUNT] =
	{
		"",
		"WRITE_AU_END       ",
		"WRITE_ONE_SECTOR   ",
		"WRITE_RANDOM_ACCESS",
		"WRITE_BECAUSE_READ ",
		"WRITE_FLUSH        ",
		"",
		"",
		"",
		"READ_AU_END        ",
		"READ_ONE_SECTOR    ",
		"READ_RANDOM_ACCESS ",
		"READ_BECAUSE_WRITE ",
		"READ_FLUSH         ",
		"",
		"",
		""
	};
static volatile INT8S sd_read_sector_result[SDC_TOTAL];
extern QueueHandle_t   sd_dma_wait_queue;
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void drvl1_sdc_set_access_kind(INT32U device_id, INT8U kind); // for debug only
#if (SD_DEBUG_CSD_INFO == 1) || (SD_DEBUG_CID_INFO == 1) || (SD_DEBUG_OSR_INFO == 1)
static INT32U swapword(INT32U in_word);
#endif

INT32S drvl2_sd_enforce_write_stop(INT32U device_id);
INT32S drvl2_sd_write_stop_nosem(INT32U device_id);

static INT32S drvl2_sd_get_sd_info_card_status(INT32U device_id);
static void sd_init_flow(INT32U device_id);
static INT32S drvl2_sd_bcr_ac_command_set(INT32U device_id, INT32U command, INT32U argument, INT32U response_type, INT32U *response);
static INT32S drvl2_sd_bc_command_set(INT32U device_id, INT32U command, INT32U argument);
static INT32S drvl2_sd_adtc_command_set(INT32U device_id, INT32U command, INT32U rca, INT32U count, INT32U *response);
static INT32S drvl2_v1_sd_card_initiate(INT32U device_id, INT32U timeout);
static INT32S drvl2_v2_sd_card_initiate(INT32U device_id, INT32U timeout);
static INT32S drvl2_mmc_card_initiate(INT32U device_id, INT32U timeout);
static INT32S drvl2_sd_flush_base(INT32U device_id);
static INT32S drvl2_sd_card_busy_wait(INT32U device_id, INT32U timeout);
static INT32S drvl2_sd_read_private_use(INT32U device_id, INT32U start_sector, INT32U *buffer, INT32U sector_count);
static INT32S drvl2_sd_read_retry(INT32U device_id, INT32U start_sector, INT32U *buffer, INT32U sector_count);
INT32S drvl2_sd_enforce_read_stop(INT32U device_id);


/**
 * @brief   Reset all sd enforcing stop type count.
 * @param   device_id[in]: device id.
 * @return 	none.
*/
void drvl2_sd_enforce_type_summary_reset(INT32U device_id)
{
	INT8U k;
	for (k=0; k < SD_ENFORCE_STOP_TYPE_COUNT; k++)
		sd_endforce_stop_type_count[device_id][k] = 0;
}

/**
 * @brief   print sd enforcing stop type name and type count.
 * @param   device_id[in]: device id.
 * @return 	none.
*/
void drvl2_sd_enforce_type_summary_print(INT32U device_id)
{
	INT8U k;

	DBG_PRINT("\r\n===========================================\r\n");
	for (k=SD_ENFORCE_STOP_TYPE_WRITE_AU_END; k < SD_ENFORCE_STOP_TYPE_WRITE_FLUSH; k++)
	{
		DBG_PRINT("%s count: %u\r\n", sd_endforce_stop_type_name[k], sd_endforce_stop_type_count[device_id][k]);
	}
	for (k=SD_ENFORCE_STOP_TYPE_READ_AU_END; k < SD_ENFORCE_STOP_TYPE_READ_FLUSH; k++)
	{
		DBG_PRINT("%s count: %u\r\n", sd_endforce_stop_type_name[k], sd_endforce_stop_type_count[device_id][k]);
	}
	DBG_PRINT("===========================================\r\n");
}

/**
 * @brief   dump buffer.
 * @param   *addr[in]: buffer address
 * @param   size[in]: dump size
 * @return 	none.
*/
void sd_dump_buffer(unsigned char *addr, unsigned long size)
{
	unsigned long i;

	for(i=0; i<size; i++)
    {
		if (i%16 == 0) DBG_PRINT("\r\n[%08lx] ",i);
			DBG_PRINT("%02x ",addr[i]);
	}
	DBG_PRINT("\r\n\n");
}

/**
 * @brief   Get SD special register field, like CID, CSD, SCR...etc.
 * @param   *bits[in]: data start address.
 * @param   total_bit[in]: data total bits.
 * @param   start_bit[in]: start bit number of the field we want to get.
 * @param   end_bit[in]: end bit number of the field we want to get.
 * @return 	field value.
*/
INT32U sd_wide_width_data_get_bits(INT8U *bits, INT16U total_bit, INT16U start_bit, INT16U end_bit)
{
    INT16U start_bit_idx = (total_bit - 1) - start_bit;
    INT16U end_bit_idx = ((total_bit - 1) - end_bit);
    INT16U start_byte_idx;
    INT16U end_byte_idx;
    INT16U byte_idx;
    INT32U value = 0;
    INT8S bit_idx;
    INT8S end;
    INT8S start_byte_bit_idx;
    INT8S end_byte_bit_idx;
    INT8S bit_shift = start_bit - end_bit;

    if ((bit_shift < 0) || (bit_shift > 31))
    {
        DBG_PRINT("illegal start end bit %u, %u. must start_bit >= end_bit, (start_bit - end_bit) <= 31\r\n", start_bit, end_bit);
        return 0;
    }

    start_byte_idx = start_bit_idx >> 3;
    start_byte_bit_idx = 7 - (start_bit_idx - (start_byte_idx << 3));

    end_byte_idx = end_bit_idx >> 3;
    end_byte_bit_idx = 7 - (end_bit_idx - (end_byte_idx << 3));

    bit_idx = start_byte_bit_idx;
    end = 0;
    for (byte_idx = start_byte_idx; byte_idx <= end_byte_idx; byte_idx++)
    {
        if (byte_idx == end_byte_idx)
            end = end_byte_bit_idx;
        for (;bit_idx >= end; bit_idx--)
        {
            value |= (((INT32U)((bits[byte_idx] >> bit_idx) & 0x1)) << bit_shift);
            bit_shift--;
            if (bit_shift < 0)
                break;
        }
        bit_idx = 7;
        if (bit_shift < 0)
            break;
    }
    return value;
}

/**
 * @brief   sd semaphore initialization.
 * @param   device_id[in]: device id.
 * @return 	0.
*/
static inline INT32S drvl2_sd_sem_init(INT32U device_id)
{
#if _OPERATING_SYSTEM != _OS_NONE
  #if _OPERATING_SYSTEM == _OS_UCOS2
	if(gSD_sem[device_id]==NULL)
		gSD_sem[device_id] = OSSemCreate(1);
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
	if(gSD_sem[device_id]==NULL)
		gSD_sem[device_id] = xSemaphoreCreateMutex();
  #endif
#endif
    return (0);
}

static inline INT32S drvl2_sd_sem_uninit(INT32U device_id)
{
#if _OPERATING_SYSTEM != _OS_NONE
  #if _OPERATING_SYSTEM == _OS_UCOS2
	if(gSD_sem[device_id]==NULL) {
		OSSemDel(gSD_sem[device_id]);
		gSD_sem[device_id] = 0;
    }
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
	if(gSD_sem[device_id]){
        vSemaphoreDelete(gSD_sem[device_id]);
        gSD_sem[device_id] = 0;
    }
    if(sd_dma_wait_queue){
        vSemaphoreDelete(sd_dma_wait_queue);
        sd_dma_wait_queue = 0;
    }
  #endif
#endif
    return (0);
}

/**
 * @brief   sd semaphore lock.
 * @param   device_id[in]: device id.
 * @return 	none.
*/
static inline void drvl2_sd_sem_lock(INT32U device_id)
{
#if _OPERATING_SYSTEM != _OS_NONE

  #if _OPERATING_SYSTEM == _OS_UCOS2

        INT8U err = NULL;

	if(gSD_sem[device_id] != NULL)
        OSSemPend(gSD_sem[device_id], 0, &err);
	else
		drvl2_sd_sem_init(device_id);

  #elif _OPERATING_SYSTEM == _OS_FREERTOS

	if(gSD_sem[device_id] != NULL)
	{
        //DBG_PRINT("drvl2_sd_sem_lock bef. %u\r\n", sd_sema_count[device_id]);
        xSemaphoreTake(gSD_sem[device_id], portMAX_DELAY);
        //DBG_PRINT("drvl2_sd_sem_lock end\r\n");
    }
	else
		drvl2_sd_sem_init(device_id);

  #endif

  if (sd_sema_count[device_id] != 0)
    DBG_PRINT("sd_sema_count not 0 before lock. %d\r\n", sd_sema_count[device_id]);
  sd_sema_count[device_id]++;

#endif
}

/**
 * @brief   sd semaphore unlock.
 * @param   device_id[in]: device id.
 * @return 	none.
*/
static inline void drvl2_sd_sem_unlock(INT32U device_id)
{
#if _OPERATING_SYSTEM != _OS_NONE

    sd_sema_count[device_id]--;
    if (sd_sema_count[device_id] != 0)
        DBG_PRINT("sd_sema_count not 0 after unlock. %d\r\n", sd_sema_count[device_id]);

  #if _OPERATING_SYSTEM == _OS_UCOS2
        OSSemPost(gSD_sem[device_id]);
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
        xSemaphoreGive(gSD_sem[device_id]);
  #endif

#endif
}

/**
 * @brief   Send SD3.0 64 bytes tuning pattern for SDR50 and SDR 104.
 * @param   drv[in]: file system drive id.
 * @return 	device id.
*/
INT8U drvl2_sd_drive_to_device_id(INT8U drv)
{
	if (drv == FS_SD0)
		return 0;
	else
	{
		return 1;
	}
}

/**
 * @brief   Send SD3.0 64 bytes tuning pattern for SDR50 and SDR 104.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: command fail, -2: tuning fail.
*/
static INT32S drvl2_sd30_tuning_cmd(INT32U device_id)
{
    memset((INT8U *)(&sd_card_tuningblock[device_id][0]), 0, 64);
    cache_invalid_range((unsigned int)&sd_card_tuningblock[device_id][0], 64);
    sd_off_drvl2_sdc_read_byte_by_cpu_crc_debug_msg = 1;
    if (!drvl2_sd_adtc_command_set(device_id, C_SD_CMD19, 0, 16, (INT32U *)(&sd_card_tuningblock[device_id][0])))
    {
        sd_off_drvl2_sdc_read_byte_by_cpu_crc_debug_msg = 0;
        if(strncmp((const char *)&sd_card_tuningblock[device_id][0], (const char *)&sd_tuning_pattern[0], 64) != 0)
        {
            return -2;
        }
        return 0;
    }
    else {
        sd_off_drvl2_sdc_read_byte_by_cpu_crc_debug_msg = 0;
        return -1;
    }
}

/**
 * @brief   SD set bit mode.this api must call before drvl2_sd_init, and have no effect when calling after drvl2_sd_init.
 * @param   device_id[in]: device id.
 * @param   strength[in]: 0~4, default is 1
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_set_driver_strength(INT32U device_id, INT8U strength)
{
	if (device_id != 0 &&  device_id != 1)
	{
		DBG_PRINT("sd device id must 0 or 1, could not %u\r\n", device_id);
		return -1;
	}
	if (strength >=0 && strength <= 4)
        sd_card_driver_strength[device_id] = strength;
	else {
        DBG_PRINT("illgal sd strength %u, must 0~4, 0 is not adjust\r\n", strength);
        return 2;
    }
	return 0;
}

/**
 * @brief   Config whether allow reduce sd clock rate during tunning sd30 phase.
  *         This api must call before drvl2_sd_init, and have no effect when calling after drvl2_sd_init.
 * @param   device_id[in]: device id.
 * @param   not_allow_reduce_sd_clock[in]: 0 allow reduce sd clock during tunning sd30 phase, default, 1 not allow.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_tunning_clock_cmd_data_phase_not_allow_reduce_sd_clock_config(INT32U device_id, INT8U not_allow_reduce_sd_clock)
{
	if (device_id != 0 &&  device_id != 1)
	{
		DBG_PRINT("sd device id must 0 or 1, could not %u\r\n", device_id);
		return -1;
	}

    sd30_tunning_clock_cmd_data_phase_not_allow_reduce_sd_clock[device_id] = not_allow_reduce_sd_clock;
    DBG_PRINT("not_allow_reduce_sd_clock[%u]=%u\r\n", device_id, sd30_tunning_clock_cmd_data_phase_not_allow_reduce_sd_clock[device_id]);

    return 0;
}

/**
 * @brief   Config whether to enable EMMC RST_n pin.
  *         This api must call before drvl2_sd_init, and have no effect when calling after drvl2_sd_init.
 * @param   device_id[in]: device id.
 * @param   not_allow_reduce_sd_clock[in]: 0 allow reduce sd clock during tunning sd30 phase, default, 1 not allow.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_enable_emmc_RST_n_config(INT32U device_id, INT8U enable)
{
	if (device_id != 0 &&  device_id != 1)
	{
		DBG_PRINT("sd device id must 0 or 1, could not %u\r\n", device_id);
		return -1;
	}

    emmc_enable_RST_n[device_id] = enable;
    DBG_PRINT("emmc_RST_n_config[%u]=%u\r\n", device_id, emmc_enable_RST_n[device_id]);

    return 0;
}

/**
 * @brief   SD set bit mode.this api must call before drvl2_sd_init, and have no effect when calling after drvl2_sd_init.
 * @param   device_id[in]: device id.
 * @param   bit_mode[in]: 1-bit mode or 4-bit mode.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_set_bit_mode(INT32U device_id, INT8U bit_mode)
{
	if (device_id != 0 &&  device_id != 1)
	{
		DBG_PRINT("sd device id must 0 or 1, could not %u\r\n", device_id);
		return -1;
	}

	if (bit_mode != 1 && bit_mode != 4)
	{
		DBG_PRINT("sd[%u] bit_mode must 1 or 4, could not %d\r\n", device_id, bit_mode);
		return -1;
	}

	if (bit_mode == 1)
		sd_card_use_one_bit_mode[device_id] = 1;
	else
	{
        INT8U sd1_pinmux;

		sd_card_use_one_bit_mode[device_id] = 0;
		sd1_pinmux = (R_FUNPOS0 >> 23) & 0x03;
		if (sd1_pinmux == 2)
        {
            // SD1 #2, pinmux with JTAG on IOC[13~15], conflict with SDDAT1~3, so disable it to avoid conflict
            if ((R_GPIO_CTRL & 0x1) != 0)
            {
                R_GPIO_CTRL &= ~0x1;
                DBG_PRINT("SD1 #2 DAT1~3 conflict with JTAG. disable JTAG. R_GPIO_CTRL=%08x\r\n", R_GPIO_CTRL);
            }
        }
    }

	DBG_PRINT("set sd[%u] bit_mode to %u\r\n", device_id, bit_mode);

	return 0;
}

/**
 * @brief   Set sd read/write retry flag.
 * @param   device_id[in]: device id.
 * @param   apply_read_retry[in]: apply read retry
 * @param   apply_write_retry[in]: apply write retry
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_set_retry(INT32U device_id, INT8U apply_read_retry, INT8U apply_write_retry)
{
    if (device_id > 1) {
        return -1;
    }

	sd_card_apply_read_retry[device_id] = apply_read_retry;
	sd_card_apply_write_retry[device_id] = apply_write_retry;
	DBG_PRINT("sd-%d: read retry=%u, write retry=%u\r\n", device_id, apply_read_retry, apply_write_retry);

	return 0;
}

/**
 * @brief   Set sd read/write retry maximum timout value.
 * @param   device_id[in]: device id.
 * @param   read_timeout[in]: read retry maximum timout value.
 * @param   write_timeout[in]: write retry maximum timout value.
 * @return 	none.
*/
INT32S drvl2_sd_set_retry_time(INT32U device_id, INT8U read_timeout, INT8U write_timeout)
{
    if (device_id > 1) {
        return -1;
    }

    drvl2_sd_sem_init(device_id);
	drvl2_sd_sem_lock(device_id);

	sd_card_read_retry_max_timeout[device_id] = read_timeout;
	sd_card_write_retry_max_timeout[device_id] = write_timeout;
	DBG_PRINT("sd-%d: read retry timeout=%u, write retry timeout=%u\r\n", device_id, read_timeout, write_timeout);

	drvl2_sd_sem_unlock(device_id);

	return 0;
}

/**
 * @brief   check current pinmux support 1.8V or not
 * @param   none
 * @return 	0: pinmux not supprt 1.8V, 1 support 1.8V
*/
static INT8U sdc_is_pinmux_support_18V(INT32U device_id)
{
    INT32U funpos0 = R_FUNPOS0;
    INT8U support_18V = 0;

    if (device_id == 0) {
        return 0;
    }
    else {
        INT32U sd1_pinmux_pos;
        INT32U sd1_support_18V_pinmux = (1<< 20);

        // BIT 24 23 20
        sd1_pinmux_pos = funpos0 & ((3<<23)|(1<<20));
        if (sd1_pinmux_pos == sd1_support_18V_pinmux) { // SDC1_MUX4
            DBG_PRINT("sd pinmux support 1.8V. mux=0x%x, 0x%x\r\n", sd1_pinmux_pos, funpos0);
            support_18V = 1;
        }
    }
    return support_18V;
}

/**
 * @brief    check whether ther is card mounted
 * @param    device_id[in]: device id.
 * @response response of last CMD action
 * @return 	 1: no card, 0: have card
*/
static INT8U drvl2_sd_check_no_card(INT32U device_id, INT32U response)
{
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

    if ((response == 0) && ((sd->Status & C_SDC_STATUS_TIMEOUT) != 0))
        return 1;
    else
        return 0;
}

void drvl2_sd_init_mclk(INT32U device_id)
{
    if (device_id == 1) {
        R_SD30_CLOCK &= ~0x7F; // ibb167, this bit div 4
        R_SD30_CLOCK |= 0x81;//peri by 2(by 2 at least)
        SD3MCLK = drv_l1_clock_get_peripheral_clock_source_freq()/((R_SD30_CLOCK&0x7F)+1);
        sd_card_mclk[device_id] = SD3MCLK;
    }
    else
        sd_card_mclk[device_id] = MCLK;
}

/**
 * @brief   sd initialization.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_init(INT32U device_id)
{
	INT32S ret=-3;
	INT32U response = K_SD_RESPONSE_INIT_VALUE;
	INT32U count;
	INT32U timeout;
	INT32U delay_10us = (((MCLK/1000000UL)*376UL/9600)*2) + 1;

	drvl2_sd_sem_init(device_id);
	drvl2_sd_sem_lock(device_id);

	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id); // DEBUG

	sd_voltage_get_callback sd_voltage_get_func;

	INT8U pinmux_use_18V = 0;

	sd_voltage_get_func = drvl2_sd_voltage_get_callback_get(device_id);
	if (sd_voltage_get_func == NULL) {
        if (sdc_is_pinmux_support_18V(device_id))
            pinmux_use_18V = 1;
    } else {
        pinmux_use_18V = sd_voltage_get_func(device_id);
    }

	if(device_id == 1) {
        if(sd_dma_wait_queue == NULL)
            sd_dma_wait_queue = xQueueCreate(5, /*sizeof(INT32U)*/sizeof(INT8S));
    }

    drvl2_sd_init_mclk(device_id);

    drvl2_sdc_clock_gate_enable(device_id, 1);
    if (drvl1_sdc_check_card_present(device_id) == 0)
    {
        DBG_PRINT("drvl2_sd_init sd-%u removed app said\r\n", device_id);
        drvl2_sd_sem_unlock(device_id);
        return -1;
    }

	#if (SD_DEBUG_ENFORCE_STOP > 0)
	drvl2_sd_enforce_type_summary_reset(device_id);
	#endif

	if (sd_card_state[device_id] >= SD_CARD_STATE_TRANSFER)
	{
        drvl2_sd_flush_base(device_id);
        DBG_PRINT("1 drvl2_sd_init drvl2_sd_flush done\r\n");

		// If card exists and in transfer state, then just return
		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) == 0) {
			if ((response & 0x1E00) == 0x800) {		// 0x800 is transfer state
                if (sd_card_user[device_id] == 0)
                    sd_card_user[device_id]++;
				sd_init_flow(device_id);

                // williamyeo-sd30-dmablksts-not-match-2
                if (device_id == 1)
                    drvl1_sdc30_save_sd30_reg();

				drvl2_sd_sem_unlock(device_id);
				return 0;
			}
			else
                DBG_PRINT("sd not in transfer state %0x08x\r\n", response);
		}
		else
            sd_card_user[device_id] = 0; // card plug out or replug in, so sd_card_user reset to 0
	}
    else
    {
        drvl2_sd_flush_base(device_id);
        DBG_PRINT("2 drvl2_sd_init drvl2_sd_flush done\r\n");
        sd_card_user[device_id] = 0; // card plug out or replug in, so sd_card_user reset to 0
    }

    sd_init_flow(device_id);

	sd_card_rca[device_id] = 0x0;
	sd_card_type[device_id] = C_MEDIA_STANDARD_SD_CARD;
	sd_card_total_sector[device_id] = 0;
	sd_card_speed[device_id] = 400000;				// 400K Hz
	sd_card_bus_width[device_id] = 1;
	sd_card_protect[device_id] = 0;
	sd_card_state[device_id] = SD_CARD_STATE_IDLE;
    if(device_id == 0){
        drvl1_sdc_init(device_id, 0); // 1~4 is using FIQ
        drvl1_sdc_enable(device_id);
    }else{
        drvl1_sdc30_init(device_id, 0);
        drvl1_sdc30_enable(device_id);
    }
    drv_tiny_wait(delay_10us);
	// Send 74 clock and then issue comand 0
	for (count=0; count<C_SD_L2_INIT_RETRY; count++) {
        L_CMD8_RETRY:
        if (count >= 1)
        {
            // change clock rate depend on sd controller, the 8MHz is by testing 9 SD card to conclude
            // keep using 400KHz is standard, but since actual test found will happen CMD8 retry, so use higher rare to avoid retry
            if (device_id == 0)
                drvl1_sdc_clock_set(device_id, 8000000);
            else
                drvl1_sdc30_clock_set(device_id, 8000000);
            drv_tiny_wait(delay_10us);
        }

        if(device_id == 0){
            drvl1_sdc_card_init_74_cycles(device_id);
        }else{
            drvl1_sdc30_card_init_74_cycles(device_id);
        }
		if(drvl2_sd_bc_command_set(device_id, C_SD_CMD0, 0x0))
            DBG_PRINT("SD CMD0 FAIL\r\n");

		if (count == 0) {
			timeout = C_SD_L2_CARD_INIT_TIMEOUT;
		} else {
			timeout = C_SD_L2_CARD_INIT_RETRY_TIMEOUT;
		}

        drv_msec_wait(2); // fix "SDERR 41. status=00001104 timeout=2000 cmd=0x00001008 arg=0x000001aa"
		// Issue command 8
		response = 0;
		ret = drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD8, 0x000001AA, C_RESP_R7, &response);
		// If the card responses to command 8
		if (ret == 0) {			// Ver 2.0 standard SD card or SDHC card
			if ((response & 0x1AA) != 0x1AA) {
                DBG_PRINT("CMD8 FAIL resp not match ! 0x%x, 0x%x\r\n", response, sd->Status);
                // reset controller before retry
                if(device_id == 0){
                    drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
                }else{
                    drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
                }
                #if 0
                if (device_id == 0) {
                    // If response is not valid, it is an unusable card
                    sd_card_state[device_id] = SD_CARD_STATE_INACTIVE;
                    drvl2_sd_sem_unlock(device_id);
                    DBG_PRINT("SDERR 1, 0x%x\r\n", response);
                    return -1;
                }
                #endif
                if (count == 0) {
                    count = 1;
                    goto L_CMD8_RETRY;
                }
                goto L_CMD8_FAIL;
			}
			DBG_PRINT("CMD8 OK!\r\n");
			if (drvl2_v2_sd_card_initiate(device_id, timeout) == 0) {
				break;
			}

		} else {	// If the card doesn't response to command 8, it is a ver 1.x SD card or MMC
            DBG_PRINT("CMD8 FAIL! 0x%x, 0x%x\r\n", response, sd->Status);
            if (count == 0) {
                if(device_id == 0){
                    drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
                }else{
                    drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
                }
                count = 1;
                goto L_CMD8_RETRY;
                break;
            }
            L_CMD8_FAIL:
			if (drvl2_v1_sd_card_initiate(device_id, timeout)==0) {
				break;
			}
			//L_EMMC:
			if (sd_card_type[device_id] == C_MEDIA_MMC_CARD) {
                if (count <= 1)
                {
                    if (device_id == 0)
                        drvl1_sdc_clock_set(device_id, 400000);
                    else
                        drvl1_sdc30_clock_set(device_id, 400000);
                    drv_tiny_wait(delay_10us);
                }
				if ((ret = drvl2_mmc_card_initiate(device_id, timeout)) == 0) {
					break;
				}
			}
			// if card type is SDHC but fail to response to CMD8, we have to run the initialization process again
		}

		// reset controller before retry
        if(device_id == 0){
            drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
        }else{
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
        }

        if ((ret == -100) && (sd_card_state[device_id] == SD_CARD_STATE_INACTIVE)) {
            count = C_SD_L2_INIT_RETRY;
            break;
        }
		#if _OPERATING_SYSTEM != _OS_NONE
            #if _OPERATING_SYSTEM == _OS_UCOS2
			OSTimeDly(1);
            #elif _OPERATING_SYSTEM == _OS_FREERTOS
            vTaskDelay(1);
            #endif
		#endif
	}
	if (count == C_SD_L2_INIT_RETRY) {
		drvl2_sd_sem_unlock(device_id);
		DBG_PRINT("SDERR 2. count %u, ret=%d\r\n", count, ret);
		if (ret)
            return ret;
        else
            return -1;
	}
	else if (count == 0)
        DBG_PRINT("STATE_READY success !\r\n");
	else if (count >= 1)
        DBG_PRINT("STATE_READY success after try_count %u\r\n", count);

	sd_card_state[device_id] = SD_CARD_STATE_READY;

    if((sd_card_ocr[device_id] & 0x01000000)&&(device_id == 1) && (pinmux_use_18V) &&(!sd_switch_1p8v_fail))
    {
        DBG_PRINT("Support 1.8v!!\r\n");
        drvl1_sdc30_dummyclk_en(device_id, 1); // Important Note Start: Fix Toshiba EXCERIA M303 V30 64G could not pass
        sd_support_1p8v = 1;
        response = 0;
        if(drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD11, 0, C_RESP_R1, &response))//switching io voltage
        {
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            drvl1_sdc30_dummyclk_en(device_id, 0);
            sd_switch_1p8v_fail = 1;
            drvl2_sd_sem_unlock(device_id);
            DBG_PRINT("SDERR 18\r\n");
            return -1;
        }
        if(response & (1<<C_SD_CARD_STATUS_ERROR_BIT))
        {
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            drvl1_sdc30_dummyclk_en(device_id, 0);
            sd_switch_1p8v_fail = 1;
            drvl2_sd_sem_unlock(device_id);
            DBG_PRINT("ERROR CMD!\r\n");
            return -1;
        }
        //DBG_PRINT("CMD11 ok!\r\n");
        osDelay(1); // Important Note End: Have to give at least 1ms dummy clock for SD card to do things
        //check if busy pin low
        if(!drvl1_sdc30_card_busy_wait(device_id, 0))//(JC)checking signal level of either one of cmd, dat[3:0] is low or NOT.
        {
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            drvl1_sdc30_dummyclk_en(device_id, 0);
            sd_switch_1p8v_fail = 1;
            drvl2_sd_sem_unlock(device_id);
            DBG_PRINT("doing power cycle!!\r\n");
            return -1;
        }
        drv_l1_sdc30_clklow(device_id, 1);
        drvl1_sdc30_dummyclk_en(device_id, 0);
        //DBG_PRINT("sd bus sta ok!\r\n");

        drv_l1_sdc30_switch_voltage18(device_id);
        osDelay(10); // Important Note: wait voltage become stable(3.3V drop to 1.8V), depend on the capacitance value at sd30 LDO and sd card,
                     // minimal must 5ms, 1u capacitance measurement is 8.7ms, so set to 10 for safe
        drv_l1_sdc30_clklow(device_id, 0);//(JC)providing sd clk@1.8v
        drvl1_sdc30_dummyclk_en(device_id, 1); // provide clock so that sd card could pull CMD to high
        osDelay(2);
        if (drvl1_sdc30_card_busy_wait(device_id, 0))
        {
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            drvl1_sdc30_dummyclk_en(device_id, 0);
            drv_l1_sdc30_switch_voltage33(device_id);
            sd_switch_1p8v_fail = 1;
            drvl2_sd_sem_unlock(device_id);
            DBG_PRINT("doing power cycle##\r\n");
            return -1;
        }
        drvl1_sdc30_dummyclk_en(device_id, 0);
        DBG_PRINT("SD30 voltage switching OK!!\r\n");
    }
    else if (sd_tunning_block_sector_addr[device_id] == 0)
    {
        DBG_PRINT("NOT Support 1.8v!!\r\n");
        sd_support_1p8v = 0;
        sd_switch_1p8v_fail = 0;
    }

	// Send command 2
	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD2, 0x0, C_RESP_R2, &sd_card_cid[device_id][0])) {
		drvl2_sd_sem_unlock(device_id);
		DBG_PRINT("SDERR 3\r\n");
		return -1;
	}
	sd_card_state[device_id] = SD_CARD_STATE_IDENTIFICATION;

	#if (SD_DEBUG_CID_INFO == 1)
	{
		INT32U sd_card_cid_swap[4];
		INT8U kk;
		INT8U MID;
		INT16U OID;
		INT32U PNM0;
		INT8U PNM1;
		INT8U PRV;
		INT32U PSN;
		INT16U MDT;
		INT8U CRC;

		for (kk = 0; kk < 4; kk++)
			sd_card_cid_swap[kk] = swapword(sd_card_cid[device_id][kk]);
		DBG_PRINT("CID:%08x %08x %08x %08x\r\n", sd_card_cid[device_id][3], sd_card_cid[device_id][2], sd_card_cid[device_id][1], sd_card_cid[device_id][0]);
		DBG_PRINT("CID swap:%08x %08x %08x %08x\r\n", sd_card_cid_swap[3], sd_card_cid_swap[2], sd_card_cid_swap[1], sd_card_cid_swap[0]);
		MID = sd_wide_width_data_get_bits((INT8U *)(&sd_card_cid_swap[0]), 128, 127, 120);
		OID = sd_wide_width_data_get_bits((INT8U *)(&sd_card_cid_swap[0]), 128, 119, 104);
		PNM0 = sd_wide_width_data_get_bits((INT8U *)(&sd_card_cid_swap[0]), 128, 103, 72);
		PNM1 = sd_wide_width_data_get_bits((INT8U *)(&sd_card_cid_swap[0]), 128, 71, 64);
		DBG_PRINT("MID=%02x OID=%04x PM=%08x%02x\r\n", MID, OID, PNM0, PNM1);
		DBG_PRINT("MID=%02x OID=%c%c PM=%c%c%c%c%c\r\n", MID, (OID>>8), (OID&0xff), (PNM0>>24), (PNM0>>16), (PNM0>>8), (PNM0&0xff), (PNM1&0xff));
		PRV = sd_wide_width_data_get_bits((INT8U *)(&sd_card_cid_swap[0]), 128, 63, 56);
		PSN = sd_wide_width_data_get_bits((INT8U *)(&sd_card_cid_swap[0]), 128, 55, 24);
		// reserve 23:20
		MDT = sd_wide_width_data_get_bits((INT8U *)(&sd_card_cid_swap[0]), 128, 19, 8);
		CRC = sd_wide_width_data_get_bits((INT8U *)(&sd_card_cid_swap[0]), 128, 7, 0); // bit 0 is always 1
		DBG_PRINT("PRV=%02x PSN=%08x MDT=%06x CRC=%02x\r\n", PRV, PSN, MDT, CRC);
	}
	#endif

	// Send command 3
	if ((sd_card_type[device_id] != C_MEDIA_MMC_CARD)&&(sd_card_type[device_id] != C_MEDIA_MMCHC_CARD)) {
		// Send CMD3 and read new RCA, SD will generate RCA itself
		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD3, 0x0, C_RESP_R6, &response)) {
			drvl2_sd_sem_unlock(device_id);
			DBG_PRINT("SDERR 4\r\n");
			return -1;
		}
		sd_card_rca[device_id] = response & 0xFFFF0000;
	} else {
		// Send CMD3 to set a new RCA to MMC card
		response = 0;
		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD3, 0xFFFF0000, C_RESP_R6, &response)) {
			drvl2_sd_sem_unlock(device_id);
			DBG_PRINT("SDERR 5\r\n");
			return -1;
		}
		sd_card_rca[device_id] = 0xFFFF0000;
	}
	sd_card_state[device_id] = SD_CARD_STATE_STANDBY;
	// Set bus width and clock speed

	ret = drvl2_sd_bus_clock_set(device_id, sd_card_initial_clock_rate[device_id]);
	if ((ret < 0) && (sd_card_initial_clock_rate[device_id] > 25000000)) {

        if (device_id == 1)
            drvl1_sdc30_all_delay_set(device_id, 0, (TUNING_CTRL_BH - TUNING_CTRL_BL)/2);

        DBG_PRINT("SDERR 6\r\n");
		ret = drvl2_sd_bus_clock_set(device_id, 25000000);
	}
	else {
        if (device_id == 0) {
            if (sd_tunning_block_sector_addr[device_id] == 0)
                drvl1_sdc_restore_COP(device_id);
        }
    }

    if (ret == 0)
        sd_card_user[device_id]++;
    else
        DBG_PRINT("SDERR 7\r\n");

    if(device_id == 0)
        sd_card_mclk[device_id] = MCLK;
    else
        sd_card_mclk[device_id] = SD3MCLK;
    {
        INT32U sd_clock_rate, sd_souce_clock_rate, sd_clock_div;

        if(device_id == 0){
            drvl1_sdc_running_clock_rate_get(device_id, &sd_clock_rate, &sd_souce_clock_rate, &sd_clock_div);
            DBG_PRINT("sd[%u] clock rate=%u, root rate=%u divider=%u cop=%u\r\n", device_id, sd_clock_rate, sd_souce_clock_rate, sd_clock_div, drvl1_sdc_COP_get(device_id));
        }else{
            drvl1_sdc30_running_clock_rate_get(device_id, &sd_clock_rate, &sd_souce_clock_rate, &sd_clock_div);
            DBG_PRINT("sd[%u] clock rate=%u, root rate=%u divider=%u\r\n", device_id, sd_clock_rate, sd_souce_clock_rate, sd_clock_div);
        }
    }

    if ((sd_card_type[device_id] == C_MEDIA_SDHC_CARD) || (sd_card_type[device_id] == C_MEDIA_STANDARD_SD_CARD)) {
        drvl2_sd_get_sd_info_card_status(device_id);
    }
	if (sd_card_total_sector[device_id] < 4*2048) {
		sd_flow_read_au_sectors[device_id] = (sd_card_total_sector[device_id]>>11)<<11; // set read au to mega sector of capacity
    }

    // williamyeo-sd30-dmablksts-not-match-2
    if (device_id == 1)
        drvl1_sdc30_save_sd30_reg();

    drvl2_sd_sem_unlock(device_id);

    return ret;
}

INT32S drvl2_sd_uninit(INT32U device_id)
{
    drvl2_sd_sem_lock(device_id);
    INT8U card_is_present = (drvl1_sdc_check_card_present(device_id) == 0) ? 0 : 1;
    INT32U response = 0;

    if (card_is_present) {
        if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response)) {
            sd_card_state[device_id] = SD_CARD_STATE_INACTIVE;
            card_is_present = 0;
        }
    }
    if (!card_is_present) {
        if(sd_support_1p8v == 1) {
            //drvl1_sdcbus_ldo_pwr_sel(device_id, SD_V33);//(JC)switch back to 3.3V
            drv_l1_sd30ldo_sel(SD30_V33, 0);//(JC)switch back to 3.3V and disable SD30_LDO
            DBG_PRINT("Shutdown SD30_LDO!\r\n");
        }
        drvl2_sdc_clock_gate_enable(device_id, 0);
    }
    drvl2_sd_sem_unlock(device_id);

    drvl2_sd_sem_uninit(device_id);

    return (0);
}

/**
 * @brief   Recover sd.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_card_recover_sd_base(INT32U device_id)
{
	INT32S ret = 0;
	INT32U response = 0;
	INT32U count;
	INT32U timeout;
	INT32U delay_10us = (((MCLK/1000000UL)*376UL/9600)*2) + 1;
    INT32U save_sd_card_rca;
    INT8U save_sd_card_type;
    INT32U save_sd_card_total_sector;

	//DBG_PRINT("# drvl2_sd_card_recover_sd_base start\r\n");
    if (drvl1_sdc_check_card_present(device_id) == 0)
    {
        DBG_PRINT("# recover_sd_base sd removed!\r\n");
        return -1;
    }
    if(device_id == 0){
        drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
    }else{
        drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
    }

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) == 0) {
		// 0x0600 is standby state, 0x800 is transfer state
		// (!(response & 0x100)), not ready
		//DBG_PRINT("# check state before recover sd card, response = 0x%08x, sd state=0x%08x\r\n", response, (response & 0x1E00));
	}
	else
        return -1;

    save_sd_card_rca = sd_card_rca[device_id];
    save_sd_card_type = sd_card_type[device_id];
    save_sd_card_total_sector = sd_card_total_sector[device_id];

	ret = drvl2_sd_bc_command_set(device_id, C_SD_CMD0, 0x0);

	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id); // DEBUG

	sd_card_rca[device_id] = 0x0;
	sd_card_type[device_id] = C_MEDIA_STANDARD_SD_CARD;
	sd_card_total_sector[device_id] = 0;
	sd_card_speed[device_id] = 400000;				// 400K Hz
	sd_card_bus_width[device_id] = 1;
	sd_card_protect[device_id] = 0;
	sd_card_state[device_id] = SD_CARD_STATE_IDLE;
    if(device_id == 0){
        drvl1_sdc_init(device_id, 0); // 1~4 is using FIQ
        drvl1_sdc_enable(device_id);
    }else{
        drvl1_sdc30_clock_set(device_id, 400000);
        drvl1_sdc30_enable(device_id);
        drvl1_sdc30_all_delay_set(device_id, 0, (TUNING_CTRL_BH - TUNING_CTRL_BL)/2);
    }
    drv_tiny_wait(delay_10us);
	// Send 74 clock and then issue comand 0
	for (count=0; count<C_SD_L2_INIT_RETRY; count++) {
        L_CMD8_RETRY:
        if (count >= 1)
        {
            // change clock rate depend on sd controller, the 8MHz is by testing 9 SD card to conclude
            // keep using 400KHz is standard, but since actual test found will happen CMD8 retry, so use higher rare to avoid retry
            if (device_id == 0)
                drvl1_sdc_clock_set(device_id, 8000000);
            else
                drvl1_sdc30_clock_set(device_id, 8000000);
            drv_tiny_wait(delay_10us);
        }

        if(device_id == 0){
            drvl1_sdc_card_init_74_cycles(device_id);
        }else{
            drvl1_sdc30_card_init_74_cycles(device_id);
        }
		if(drvl2_sd_bc_command_set(device_id, C_SD_CMD0, 0x0))
            DBG_PRINT("# SD CMD0 FAIL\r\n");

		if (count == 0) {
			timeout = C_SD_L2_CARD_INIT_TIMEOUT;
		} else {
			timeout = C_SD_L2_CARD_INIT_RETRY_TIMEOUT;
		}

        drv_msec_wait(2); // fix "SDERR 41. status=00001104 timeout=2000 cmd=0x00001008 arg=0x000001aa"
		// Issue command 8
		response = 0;
		ret = drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD8, 0x000001AA, C_RESP_R7, &response);
		// If the card responses to command 8
		if (ret == 0) {			// Ver 2.0 standard SD card or SDHC card
			if ((response & 0x1AA) != 0x1AA) {
                DBG_PRINT("# CMD8 FAIL resp not match ! 0x%x, 0x%x\r\n", response, sd->Status);
                // reset controller before retry
                if(device_id == 0){
                    drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
                }else{
                    drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
                }
                if (count == 0) {
                    count = 1;
                    goto L_CMD8_RETRY;
                }
                goto L_CMD8_FAIL;
			}
			DBG_PRINT("# CMD8 OK!\r\n");
			if (drvl2_v2_sd_card_initiate(device_id, timeout) == 0) {
				break;
			}

		} else {	// If the card doesn't response to command 8, it is a ver 1.x SD card or MMC
            DBG_PRINT("# CMD8 FAIL! 0x%x, 0x%x\r\n", response, sd->Status);
            if (count == 0) {
                if(device_id == 0){
                    drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
                }else{
                    drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
                }
                count = 1;
                goto L_CMD8_RETRY;
                break;
            }
            L_CMD8_FAIL:
			if (drvl2_v1_sd_card_initiate(device_id, timeout)==0) {
				break;
			}
			if (sd_card_type[device_id] == C_MEDIA_MMC_CARD) {
                if (count <= 1)
                {
                    if (device_id == 0)
                        drvl1_sdc_clock_set(device_id, 400000);
                    else
                        drvl1_sdc30_clock_set(device_id, 400000);
                    drv_tiny_wait(delay_10us);
                }
				if ((ret = drvl2_mmc_card_initiate(device_id, timeout)) == 0) {
					break;
				}
			}
			// if card type is SDHC but fail to response to CMD8, we have to run the initialization process again
		}

		// reset controller before retry
        if(device_id == 0){
            drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
        }else{
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
        }

        if ((ret == -100) && (sd_card_state[device_id] == SD_CARD_STATE_INACTIVE)) {
            count = C_SD_L2_INIT_RETRY;
            break;
        }
		#if _OPERATING_SYSTEM != _OS_NONE
            #if _OPERATING_SYSTEM == _OS_UCOS2
			OSTimeDly(1);
            #elif _OPERATING_SYSTEM == _OS_FREERTOS
            vTaskDelay(1);
            #endif
		#endif
	}
	if (count == C_SD_L2_INIT_RETRY) {
        sd_card_rca[device_id] = save_sd_card_rca;
        sd_card_type[device_id] = save_sd_card_type;
        sd_card_total_sector[device_id] = save_sd_card_total_sector;
		DBG_PRINT("# SDERR 2. count %u, ret=%d\r\n", count, ret);
		if (ret)
            return ret;
        else
            return -1;
	}
	else if (count == 0)
        DBG_PRINT("# STATE_READY success !\r\n");
	else if (count >= 1)
        DBG_PRINT("# STATE_READY success after try_count %u\r\n", count);

	sd_card_state[device_id] = SD_CARD_STATE_READY;

	// Send command 2
	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD2, 0x0, C_RESP_R2, &sd_card_cid[device_id][0])) {
		DBG_PRINT("# SDERR 3\r\n");
		return -1;
	}
	sd_card_state[device_id] = SD_CARD_STATE_IDENTIFICATION;

	// Send command 3
	if ((sd_card_type[device_id] != C_MEDIA_MMC_CARD)&&(sd_card_type[device_id] != C_MEDIA_MMCHC_CARD)) {
		// Send CMD3 and read new RCA, SD will generate RCA itself
		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD3, 0x0, C_RESP_R6, &response)) {
			DBG_PRINT("# SDERR 4\r\n");
			return -1;
		}
		sd_card_rca[device_id] = response & 0xFFFF0000;
	} else {
		// Send CMD3 to set a new RCA to MMC card
		response = 0;
		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD3, 0xFFFF0000, C_RESP_R6, &response)) {
			DBG_PRINT("# SDERR 5\r\n");
			return -1;
		}
		sd_card_rca[device_id] = 0xFFFF0000;
	}
	sd_card_state[device_id] = SD_CARD_STATE_STANDBY;
	// Set bus width and clock speed

	ret = drvl2_sd_bus_clock_set(device_id, sd_card_initial_clock_rate[device_id]);
	if ((ret < 0) && (sd_card_initial_clock_rate[device_id] > 25000000)) {
        DBG_PRINT("# SDERR 6\r\n");

		ret = drvl2_sd_bus_clock_set(device_id, 25000000);
	}
	else {
        if (device_id == 0)
            drvl1_sdc_restore_COP(device_id);
    }

    if (ret != 0)
        DBG_PRINT("# SDERR 7\r\n");

    if(device_id == 0)
        sd_card_mclk[device_id] = MCLK;
    else
        sd_card_mclk[device_id] = SD3MCLK;
    {
        INT32U sd_clock_rate, sd_souce_clock_rate, sd_clock_div;

        if(device_id == 0){
            drvl1_sdc_running_clock_rate_get(device_id, &sd_clock_rate, &sd_souce_clock_rate, &sd_clock_div);
        }else{
            drvl1_sdc30_running_clock_rate_get(device_id, &sd_clock_rate, &sd_souce_clock_rate, &sd_clock_div);
        }
        DBG_PRINT("# sd[%u] clock rate=%u, root rate=%u divider=%u\r\n", device_id, sd_clock_rate, sd_souce_clock_rate, sd_clock_div);
    }

    if ((sd_card_type[device_id] == C_MEDIA_SDHC_CARD) || (sd_card_type[device_id] == C_MEDIA_STANDARD_SD_CARD)) {
        drvl2_sd_get_sd_info_card_status(device_id);
    }
	if (sd_card_total_sector[device_id] < 4*2048) {
		sd_flow_read_au_sectors[device_id] = (sd_card_total_sector[device_id]>>11)<<11; // set read au to mega sector of capacity
    }

    if (device_id == 1) {
        drvl1_sdc30_save_sd30_reg();
    }

	return ret;
}

/**
 * @brief   Recover sd to transfer state.
 * @param   device_id[in]: device id.
 * @param   apply_reset_sd[in]: reset sd.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_recover_to_transfer_state_nosema(INT32U device_id, INT32U apply_reset_sd)
{
	INT32S ret = 0;
	INT32U response = 0x5A5A5A5A;
	INT8U current_state;
	INT8U ready_for_data;
	INT32U reset_sd_card = apply_reset_sd;

	if(device_id == 0){
        if (drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
        {
            DBG_PRINT("0 fail to reset sd controller\r\n");
        }
    }else{
        if (drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
        {
            DBG_PRINT("0 fail to reset sd controller\r\n");
        }
    }

	L_RESET_CARD:

	sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response)) {
		DBG_PRINT("fail to get state. response=0x%08x\r\n", response);
		return -1;
	}

	current_state = (response >> 9) & 0x0f;
	ready_for_data = (response >> 8) & 0x01;

	if (reset_sd_card || (apply_reset_sd &&  \
		((current_state == C_SD_CARD_STATE_IDLE) || (current_state == C_SD_CARD_STATE_READY) || \
		 (current_state == C_SD_CARD_STATE_IDENT) || (current_state == C_SD_CARD_STATE_STBY) || \
		 (current_state == C_SD_CARD_STATE_DIS))) )
	{
		ret = drvl2_sd_card_recover_sd_base(device_id);
		//DBG_PRINT("reset sd card. ret=%d, state=%u, ready=%u\r\n", ret, current_state, ready_for_data);
		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response)) {
			DBG_PRINT("fail to get state aft reset. response=0x%08x\r\n", response);
			return -1;
		}
		current_state = (response >> 9) & 0x0f;
		ready_for_data = (response >> 8) & 0x01;
		//DBG_PRINT("aft reset sd card. ret=%d, state=%u, ready=%u\r\n", ret, current_state, ready_for_data);
		if (current_state != C_SD_CARD_STATE_TRAN)
		{
			DBG_PRINT("aft reset sd card, still not in tranfer state\r\n");
			return -1;
		}
		return ret;
	}

	if (current_state == C_SD_CARD_STATE_TRAN)
	{
		//DBG_PRINT("in tarnsfer state=%u, ready=%u\r\n", current_state, ready_for_data);
		if (!ready_for_data && apply_reset_sd)
		{
            if(device_id == 0){
                if (drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
                {
                    DBG_PRINT("1 fail to reset sd controller\r\n");
                }
            }else{
                if (drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
                {
                    DBG_PRINT("2 fail to reset sd controller\r\n");
                }
            }
			if (!reset_sd_card)
			{
                reset_sd_card = 1;
                DBG_PRINT("1 go to reset sd card\r\n");
                goto L_RESET_CARD;
            }
		}
	}
	else if ((current_state == C_SD_CARD_STATE_DATA) || (current_state == C_SD_CARD_STATE_RCV))
	{
        if(device_id == 0){
            if (drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
            {
                DBG_PRINT("2 fail to reset sd controller\r\n");
            }
        }else{
            if (drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
            {
                DBG_PRINT("3 fail to reset sd controller\r\n");
            }
        }
		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
		{
            DBG_PRINT("fail to send CMD12. response=0x%08x\r\n", response);
		}
		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response)) {
			DBG_PRINT("fail to get state aft CMD12. response=0x%08x\r\n", response);
			return -1;
		}
		current_state = (response >> 9) & 0x0f;
		ready_for_data = (response >> 8) & 0x01;

		//DBG_PRINT("aft CMD12. ret=%d, state=%u, ready=%u\r\n", ret, current_state, ready_for_data);

		if (current_state == C_SD_CARD_STATE_PRG)
			DBG_PRINT("aft CMD12, become PRG\r\n");
		if (drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_BUSY_TIMEOUT))
		{
			DBG_PRINT("card is still busy after CMD12\r\n");
			if (apply_reset_sd)
			{
                if (!reset_sd_card)
                {
                    reset_sd_card = 1;
                    DBG_PRINT("2 go to reset sd card\r\n");
                    goto L_RESET_CARD;
                }
			}
		}
	}
	else if (current_state == C_SD_CARD_STATE_PRG)
	{
		if (drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_BUSY_TIMEOUT))
		{
			DBG_PRINT("card is still busy at PRG state\r\n");
			if (apply_reset_sd)
			{
                if (!reset_sd_card)
                {
                    reset_sd_card = 1;
                    DBG_PRINT("3 go to reset sd card\r\n");
                    goto L_RESET_CARD;
                }
			}
		}
		if(device_id == 0){
            if (drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
            {
                DBG_PRINT("3 fail to reset sd controller\r\n");
            }
        }else{
            if (drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
            {
                DBG_PRINT("3 fail to reset sd controller\r\n");
            }
        }
	}

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response)) {
		return -1;
	}
	if (current_state == C_SD_CARD_STATE_TRAN && ready_for_data)
	{
        sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
		return 0;
    }

	DBG_PRINT("fail to recover to transfer state. response=0x%08x\r\n", response);

	return -1;
}

/**
 * @brief   Set sd clock speed and bus width.
 * @param   device_id[in]: device id.
 * @return 	none.
*/
void drvl2_sd_set_speed_bus(INT32U device_id)
{
    if(device_id == 0){
        if (MCLK == sd_card_mclk[device_id])
            return;
    }
    //DBG_PRINT("sd-%u, MCLK=%u, sd_card_mclk=%u\r\n", device_id, MCLK, sd_card_mclk[device_id]);
    if(device_id == 0){
        drvl1_sdc_clock_set(device_id, sd_card_speed[device_id]);
        drvl1_sdc_bus_width_set(device_id, sd_card_bus_width[device_id]);			// Set bus width to 4 bits
    }else{
        drvl1_sdc30_clock_set(device_id, sd_card_speed[device_id]);
        drvl1_sdc30_bus_width_set(device_id, sd_card_bus_width[device_id]);			// Set bus width to 4 bits
    }


    // just for purpose to display
    if(device_id == 0){
        if (MCLK != sd_card_mclk[device_id])
        {
            INT32U sd_clock_rate, sd_souce_clock_rate, sd_clock_div;

            drvl1_sdc_running_clock_rate_get(device_id, &sd_clock_rate, &sd_souce_clock_rate, &sd_clock_div);
            DBG_PRINT("MCLK is [%u], sd[%u] clock rate=%u, root rate=%u divider=%u\r\n", MCLK, device_id, sd_clock_rate, sd_souce_clock_rate, sd_clock_div);
        }
        sd_card_mclk[device_id] = MCLK;
    }else{
        if (SD3MCLK != sd_card_mclk[device_id])
        {
            INT32U sd_clock_rate, sd_souce_clock_rate, sd_clock_div;

            drvl1_sdc30_running_clock_rate_get(device_id, &sd_clock_rate, &sd_souce_clock_rate, &sd_clock_div);
            DBG_PRINT("SD3MCLK is [%u], sd[%u] clock rate=%u, root rate=%u divider=%u\r\n", SD3MCLK, device_id, sd_clock_rate, sd_souce_clock_rate, sd_clock_div);
        }
        sd_card_mclk[device_id] = SD3MCLK;
    }
}

/**
 * @brief   Send sd bc type command and wait completion.
 * @param   device_id[in]: device id.
 * @param   command[in]: sd command.
 * @param   argument[in]: command argument.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_bc_command_set(INT32U device_id, INT32U command, INT32U argument)
{
    if(device_id == 0){
        if (drvl1_sdc_command_send(device_id, command, argument))
            return -1;
    }else{
        if (drvl1_sdc30_command_send(device_id, command, argument))
            return -1;
    }
    if(device_id == 0){
        return drvl1_sdc_command_complete_wait(device_id, C_SD_L2_RESP_R136_FULL_TIMEOUT);
    }else{
        return drvl1_sdc30_command_complete_wait(device_id, C_SD_L2_RESP_R136_FULL_TIMEOUT);
    }
}

INT32S drvl2_emmc_bc_command_set(INT32U device_id, INT32U command, INT32U argument)
{
    INT32S ret;

    drvl2_sd_sem_lock(device_id);

    ret = drvl2_sd_bc_command_set(device_id, command, argument);

    drvl2_sd_sem_unlock(device_id);

    return ret;
}

/**
 * @brief   Send sd bcr/ac type command and get response.
 * @param   device_id[in]: device id.
 * @param   command[in]: sd command to send.
 * @param   argument[in]: command argument.
 * @param   response_type[in]: response type.
 * @param   *response[out]: response content.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_bcr_ac_command_set(INT32U device_id, INT32U command, INT32U argument, INT32U response_type, INT32U *response)
{
	INT8U i;
	INT32S ret;
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

    if(device_id == 0){
        if (drvl1_sdc_command_send(device_id, command, argument)) {
            DBG_PRINT("SDERR 31=0x%08x, 0x%08x, Status=0x%08x\r\n", command, argument, sd->Status);
            return -1;
        }
    }else{
        if (drvl1_sdc30_command_send(device_id, command, argument)) {
            DBG_PRINT("SD3ERR 31=0x%08x, 0x%08x, Status=0x%08x\r\n", command, argument, sd->Status);
            return -1;
        }
    }

	switch (response_type) {
	case C_RESP_R1:
	case C_RESP_R1B:
	case C_RESP_R3:
	case C_RESP_R6:
        if(device_id == 0)
        {
            ret = drvl1_sdc_response_get(device_id, response, C_SD_L2_RESP_R136_FULL_TIMEOUT);
        }
        else
            ret = drvl1_sdc30_response_get(device_id, response, C_SD_L2_RESP_R136_FULL_TIMEOUT);
		if (command == C_SD_CMD12)
		{
            if(device_id == 0)
                drvl1_sdc_clear_timeout_bit(device_id, 10);
            else
                drvl1_sdc30_clear_timeout_bit(device_id, 10);
		}
		#if 0 // DEBUG CODE
		{
			if (command == C_SD_CMD7)
			{
				DBG_PRINT("CMD7 response 0x%08x, ret=%d\r\n", *response, ret);
			}
			else if (command == C_SD_CMD13)
			{
				DBG_PRINT("CMD13 response 0x%08x, ret=%d\r\n", *response, ret);
			}
		}
		#endif

		return ret;

	case C_RESP_R2:
		for (i=0; i<4; i++) {
            if(device_id == 0){
                if (drvl1_sdc_response_get(device_id, response, C_SD_L2_RESP_R2_FULL_TIMEOUT)) {
                    DBG_PRINT("SDERR 32. Status=0x%x\r\n", sd->Status);
                    return -1;
                }
            }else{
                if (drvl1_sdc30_response_get(device_id, response, C_SD_L2_RESP_R2_FULL_TIMEOUT)) {
                    DBG_PRINT("SD3ERR 32.1. Status=0x%x\r\n", sd->Status);
                    return -1;
                }
            }
			response++;
		}

	case C_RESP_R0:
	default:
		break;
	}

	return 0;
}

INT32S drvl2_emmc_bcr_ac_command_set(INT32U device_id, INT32U command, INT32U argument, INT32U response_type, INT32U *response)
{
    INT32S ret;

    drvl2_sd_sem_lock(device_id);

    ret = drvl2_sd_bcr_ac_command_set(device_id, command, argument, response_type, response);

    drvl2_sd_sem_unlock(device_id);

    return ret;
}

/**
 * @brief   Send SD2.0 adtc type command and get response base function.
 * @param   device_id[in]: device id.
 * @param   command[in]: sd command to send.
 * @param   rca[in]: relative card address
 * @param   count[in]: response byte count.
 * @param   *response[out]: response content.
 * @param   *dma_status[out]: DMA status
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_adtc_command_set_base(INT32U device_id, INT32U command, INT32U rca, INT32U count, INT32U *response, INT32U *dma_status)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
    INT32S ret = 0;
	INT32U enter_Status; // just for debug purpose

	enter_Status = sd->Status;
	*dma_status = 0;

    if(device_id == 0){
        // Clear SD RX data register before read command is issued
        drvl1_sdc_clear_rx_data_register(device_id);
        if (drvl1_sdc_command_send(device_id, command, rca)) {
            DBG_PRINT("sd_adtc drvl1_sdc_command_set error[%d]. status=0x%08x count=%u command=0x%x old status=0x%08x\r\n", device_id, sd->Status, count<<2, command, enter_Status);
            drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            return -1;
        }
    }

    // read response for CMD17, CMD18, CMD24, CMD25
    if ((count == 0) && (response != NULL))
    {
        *response = K_SD_RESPONSE_INIT_VALUE;
        if(device_id == 0){
            if (drvl1_sdc_response_get(device_id, response, C_SD_L2_RESP_R136_FULL_TIMEOUT)) {
                drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
                return -1;
            }
        }
    }

	if(device_id == 0){
        if (drvl1_sdc_command_complete_wait(device_id, C_SD_L2_SINGLE_READ_CMD_COMPLETE_TIMEOUT))
        {
            DBG_PRINT("sd_adtc drvl1_sdc_command_complete_wait error[%d]. status=0x%08x count=%u command=0x%x old status=0x%08x\r\n", device_id, sd->Status, count<<2, command, enter_Status);
            drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            return -1;
        }
    }

	if (count == 0)
		return 0;


    // use dma to read
    if (drvl1_sdc_read_byte_by_dma(device_id, response, count<<2))
	{
		DBG_PRINT("sd_adtc drvl1_sdc_read_byte_by_dma error[%d]. status=0x%08x count=%u command=0x%x old status=0x%08x\r\n", device_id, sd->Status, count<<2, command, enter_Status);
		*dma_status |= sd->Status;
		drvl1_sdc_clear_timeout_bit(device_id, 10);
		drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
		ret = -1;
		return ret;
	}

    if(device_id == 0){
        if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_SINGLE_READ_DATA_COMPLETE_TIMEOUT))
        {
            *dma_status |= sd->Status;
            drvl1_sdc_clear_timeout_bit(device_id, 10);
            DBG_PRINT("sd_adtc read data_complete_wait fail[%d]. status=0x%08x count=%u\r\n", device_id, sd->Status, count<<2);
            drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            ret = -1;
        }
    }

	if (drvl1_sdc_data_crc_status_get(device_id))
	{
		*dma_status |= sd->Status;
		*dma_status |= C_SDC_STATUS_DATA_CRC_ERR;
        DBG_PRINT("sd_adtc drvl2_sdc_read_byte_by_cpu_crc error[%d]. status=0x%08x count=%u\r\n", device_id, sd->Status, count<<2);
		ret = -1;
	}

	return ret;
}


/**
 * @brief   Send SD3.0 adtc type command and get response base function.
 * @param   device_id[in]: device id.
 * @param   command[in]: sd command to send.
 * @param   rca[in]: relative card address
 * @param   count[in]: response byte count.
 * @param   *response[out]: response content.
 * @param   *dma_status[out]: DMA status.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd30_adtc_command_set_base(INT32U device_id, INT32U command, INT32U rca, INT32U count, INT32U *response, INT32U *dma_status)
{
	volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
    INT32S ret = 0;
	INT32U enter_Status; // just for debug purpose

	enter_Status = sd->Status;
	*dma_status = 0;


    // Clear SD RX data register before read command is issued
    drvl1_sdc30_clear_rx_data_register(device_id);
    if (drvl1_sdc30_command_send(device_id, command, rca)) {
        DBG_PRINT("sd_adtc drvl1_sdc_command_set error[%d]. status=0x%08x count=%u command=0x%x old status=0x%08x\r\n", device_id, sd->Status, count<<2, command, enter_Status);
        drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
        return -1;
    }

    // read response for CMD17, CMD18, CMD24, CMD25
    if ((count == 0) && (response != NULL))
    {
        *response = K_SD_RESPONSE_INIT_VALUE;

        if (drvl1_sdc30_response_get(device_id, response, C_SD_L2_RESP_R136_FULL_TIMEOUT)) {
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            return -1;
        }
    }

    if (drvl1_sdc30_command_complete_wait(device_id, C_SD_L2_SINGLE_READ_CMD_COMPLETE_TIMEOUT))
    {
        if (!sd_off_drvl2_sdc_read_byte_by_cpu_crc_debug_msg)
            DBG_PRINT("sd_adtc drvl1_sdc_command_complete_wait error[%d]. status=0x%08x count=%u command=0x%x old status=0x%08x\r\n", device_id, sd->Status, count<<2, command, enter_Status);
        drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
        return -1;
    }

	if (count == 0)
		return 0;

	if(device_id == 1){
        // use cpu to read
        while (count) {
            if (drvl1_sdc30_data_get(device_id, response, C_SD_L2_ADTC_DATA_FULL_TIMEOUT)) {
                DBG_PRINT("sd_adtc drvl1_sdc_data_get error[%d]. status=0x%08x count=%u command=0x%x old status=0x%08x\r\n", device_id, sd->Status, count, command, enter_Status);
                *dma_status |= sd->Status;
                drvl1_sdc30_clear_timeout_bit(device_id, 10);
                drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
                ret = -1;
                return ret;
            }
            response++;
            count--;
        }
	}


    if (drvl1_sdc30_data_complete_wait(device_id, C_SD_L2_SINGLE_READ_DATA_COMPLETE_TIMEOUT))
    {
        *dma_status |= sd->Status;
        drvl1_sdc30_clear_timeout_bit(device_id, 10);
        DBG_PRINT("sd_adtc read data_complete_wait fail[%d]. status=0x%08x count=%u\r\n", device_id, sd->Status, count<<2);
        drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
        ret = -1;
    }

	if (drvl1_sdc_data_crc_status_get(device_id))
	{
		*dma_status |= sd->Status;
		*dma_status |= C_SDC_STATUS_DATA_CRC_ERR;
		if (!sd_off_drvl2_sdc_read_byte_by_cpu_crc_debug_msg)
            DBG_PRINT("sd_adtc drvl2_sdc_read_byte_by_cpu_crc error[%d]. status=0x%08x count=%u\r\n", device_id, sd->Status, count<<2);
		ret = -1;
	}

	return ret;
}

/**
 * @brief   Send SD2.0 adtc type command and get response function.
 * @param   device_id[in]: device id.
 * @param   command[in]: sd command to send.
 * @param   rca[in]: relative card address
 * @param   count[in]: response byte count.
 * @param   *response[out]: response content.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_adtc_command_set(INT32U device_id, INT32U command, INT32U rca, INT32U count, INT32U *response)
{
	INT32S ret;
	INT32U dma_status;
	INT32U try_count = 0;
    INT32U temp_response;

	do
	{
        if(device_id == 0){
            ret = drvl2_sd_adtc_command_set_base(device_id, command, rca, count, response, &dma_status);
        }else{
            ret = drvl2_sd30_adtc_command_set_base(device_id, command, rca, count, response, &dma_status);
        }
		if ((ret == 0) || (count == 0))
			break;
		if ((dma_status & (C_SDC_STATUS_DATA_CRC_ERR | C_SDC_STATUS_TIMEOUT)) == 0)
		{
			break;
        }
		// data crc error and data status timeout case will be retry
		if (drvl1_sdc_check_card_present(device_id) == 0)
        {
            // card no exist, also clear timeout bit cause by this
            dma_status &= (~C_SDC_STATUS_TIMEOUT);
            break;
        }

        if (command == C_SD_CMD19) // For strict condition consideration, bypass retry, it will retry at phase calibration
            break;

        if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &temp_response) != 0)
            break;

		try_count++;
	} while (try_count < 3);

	return ret;
}

#if (SD_FORCE_ERASE_ALL == 1)
INT32S drvl2_sd_force_erase_all(INT32U device_id)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	INT32S ret = 0;
	INT32U response = 0;
	INT32U lock_card_data_structure;

	// follow sd spec 4.3.7.3 Forcing Erase

	// set block length to 1 byte
	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD16, 1, C_RESP_R1, &response) != 0)
	{
		DBG_PRINT("force erase all C_SD_CMD16 error[%d]. status=0x%08x\r\n", device_id, sd->Status);
		return -1;
	}
    if(device_id == 0){
        if (drvl1_sdc_block_len_set(device_id, 1)) { // DDR50 mode should set to 2
            DBG_PRINT("force erase all drvl1_sdc_block_len_set error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            return -1;
        }
    }else{
        if (drvl1_sdc30_block_len_set(device_id, 1)) { // DDR50 mode should set to 2
            DBG_PRINT("force erase all drvl1_sdc_block_len_set error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            return -1;
        }
    }

    if(device_id == 0){
        if (drvl1_sdc_command_send(device_id, C_SD_CMD42, 0)) {
            DBG_PRINT("force erase all drvl1_sdc_command_send error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            return -1;
        }
    }else{
         if (drvl1_sdc30_command_send(device_id, C_SD_CMD42, 0)) {
            DBG_PRINT("force erase all drvl1_sdc_command_send error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            return -1;
        }
    }

    if(device_id == 0){
        if (drvl1_sdc_command_complete_wait(device_id, C_SD_L2_SINGLE_READ_CMD_COMPLETE_TIMEOUT))
        {
            DBG_PRINT("force erase all drvl1_sdc_command_complete_wait error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            return -1;
        }
    }else{
        if (drvl1_sdc30_command_complete_wait(device_id, C_SD_L2_SINGLE_READ_CMD_COMPLETE_TIMEOUT))
        {
            DBG_PRINT("force erase all drvl1_sdc_command_complete_wait error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            return -1;
        }
    }

	lock_card_data_structure = 0x08080808; // set Bit 3 ERASE
	ret = drvl1_sdc_write_byte_by_dma(device_id, &lock_card_data_structure, 4); // should 1 byte, but sd controller cant send 1 byte
	if (ret != 0)
	{
		DBG_PRINT("force erase all dma error[%d]. status=0x%08x\r\n", device_id, sd->Status);
		drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
		ret = -1;
	}

	response = 0;
	if(device_id == 0){
        if (drvl1_sdc_response_get(device_id, &response, C_SD_L2_RESP_R136_FULL_TIMEOUT) != 0)
        {
            DBG_PRINT("force erase all get response error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            ret = -1;
        }
    }else{
        if (drvl1_sdc30_response_get(device_id, &response, C_SD_L2_RESP_R136_FULL_TIMEOUT) != 0)
        {
            DBG_PRINT("force erase all get response error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            ret = -1;
        }
    }

	DBG_PRINT("CMD42 response = 0x%08x\r\n", response);

	// restore block length
	if(device_id == 0){
        if (drvl1_sdc_block_len_set(device_id, 512)) {
            DBG_PRINT("force erase all restore block len error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            ret = -1;
        }
    }else{
        if (drvl1_sdc30_block_len_set(device_id, 512)) {
            DBG_PRINT("force erase all restore block len error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            ret = -1;
        }
    }
	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD16, 512, C_RESP_R1, &response) != 0)
	{
		DBG_PRINT("force erase all C_SD_CMD16 restore error[%d]. status=0x%08x\r\n", device_id, sd->Status);
		ret = -1;
	}

	return ret;
}
#endif

/**
 * @brief   Ver 1.x SD card initialization.
 * @param   device_id[in]: device id.
 * @param   timeout[in]: timeout value.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_v1_sd_card_initiate(INT32U device_id, INT32U timeout)
{
	INT32S ret;
	INT32U response;
	#if _OPERATING_SYSTEM == _OS_FREERTOS
	TimeOut_t xTimeOut;
    TickType_t xRemainTicks = pdMS_TO_TICKS(timeout);
	#else
	INT32U timer;
	#endif
	#if _OPERATING_SYSTEM == _OS_NONE
	volatile INT32U timer1, timer2;
	#endif

    #if (defined _DRV_L1_SW_TIMER) && (_DRV_L1_SW_TIMER == 1)
	timer = (INT32U) sw_timer_get_counter_L();
    #elif _OPERATING_SYSTEM != _OS_NONE
		#if _OPERATING_SYSTEM == _OS_UCOS2
		timer = OSTimeGet();		// 10ms per tick
		#elif _OPERATING_SYSTEM == _OS_FREERTOS
		vTaskSetTimeOutState(&xTimeOut);
		#endif
    #else
	timer1 = tiny_counter_get();
  	timer = 0;
    #endif
	do {
		// Send ACMD41 with HCS=0 until card is ready or timeout
		response = 0;
		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD55, 0x0, C_RESP_R1, &response)) {
			// If CMD55 timeout occurs, maybe it is a MMC card
			sd_card_type[device_id] = C_MEDIA_MMC_CARD;

			return -1;
		}

		sd_card_ocr[device_id] = 0;
		ret = drvl2_sd_bcr_ac_command_set(device_id, C_SD_ACMD41, 0x00200000, C_RESP_R3, &sd_card_ocr[device_id]);
		//DBG_PRINT("2 C_SD_ACMD41 ret=%d, sd_card_ocr=0x%08x, Status=0x%08x\r\n", ret, sd_card_ocr[device_id], sd->Status);
		if (ret) {

            if (ret || (sd_card_ocr[device_id] == 0)) {
                sd_card_state[device_id] = SD_CARD_STATE_INACTIVE;
                DBG_PRINT("2 BADCARD!\r\n");
                return -200;
            }

			#if (defined _DRV_L1_SW_TIMER) && (_DRV_L1_SW_TIMER == 1)
			if ((sw_timer_get_counter_L()-timer) > timeout) {
			#elif _OPERATING_SYSTEM != _OS_NONE
                #if _OPERATING_SYSTEM == _OS_UCOS2
				if ((OSTimeGet()-timer)*10 > timeout) {
                #elif _OPERATING_SYSTEM == _OS_FREERTOS
				if (xTaskCheckForTimeOut(&xTimeOut, &xRemainTicks) == pdTRUE) {
                #endif
			#else
			timer2 = tiny_counter_get();
			if (timer1 != timer2)
			{
				timer1 = timer2;
				timer++;
			}
		  	if (timer > timeout) {
			#endif
		    	// If card timeout occurs, maybe it is a MMC card
		    	sd_card_type[device_id] = C_MEDIA_MMC_CARD;

		    	return -1;
		    }

	    } else if (!(sd_card_ocr[device_id] & 0x80000000)) {

            if (sd_card_ocr[device_id] == 0) {
                sd_card_state[device_id] = SD_CARD_STATE_INACTIVE;
                DBG_PRINT("3 BADCARD!\r\n");
                return -200;
            }

			#if (defined _DRV_L1_SW_TIMER) && (_DRV_L1_SW_TIMER == 1)
			if ((sw_timer_get_counter_L()-timer) > timeout) {
			#elif _OPERATING_SYSTEM != _OS_NONE
				#if _OPERATING_SYSTEM == _OS_UCOS2
				if ((OSTimeGet()-timer)*10 > timeout)) {
				#elif _OPERATING_SYSTEM == _OS_FREERTOS
				if (xTaskCheckForTimeOut(&xTimeOut, &xRemainTicks) == pdTRUE) {
				#endif
			#else
			timer2 = tiny_counter_get();
			if (timer1 != timer2)
			{
				timer1 = timer2;
				timer++;
			}
		  	if (timer > timeout) {
			#endif
		    	// If card busy timeout occurs, maybe it is a SDHC card
		    	sd_card_type[device_id] = C_MEDIA_SDHC_CARD;

		    	return -1;
		    }

		} else {		// It is a Ver1.X standard SD card
			sd_card_type[device_id] = C_MEDIA_STANDARD_SD_CARD;
			break;
		}
	} while (1) ;

	return 0;
}

/**
 * @brief   Ver 2.0/Ver 3.0 SD card initialization.
 * @param   device_id[in]: device id.
 * @param   timeout[in]: timeout value.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_v2_sd_card_initiate(INT32U device_id, INT32U timeout)
{
	INT32S ret;
	INT32U response;
	#if _OPERATING_SYSTEM == _OS_FREERTOS
	TimeOut_t xTimeOut;
    TickType_t xRemainTicks = pdMS_TO_TICKS(timeout);
	#else
	INT32U timer;
	#endif
	#if _OPERATING_SYSTEM == _OS_NONE
	volatile INT32U timer1, timer2;
	#endif
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	#if (defined _DRV_L1_SW_TIMER) && (_DRV_L1_SW_TIMER == 1)
	timer = (INT32U) sw_timer_get_counter_L();
	#elif _OPERATING_SYSTEM != _OS_NONE
		#if _OPERATING_SYSTEM == _OS_UCOS2
		timer = OSTimeGet();		// 10ms per tick
		#elif _OPERATING_SYSTEM == _OS_FREERTOS
		vTaskSetTimeOutState(&xTimeOut);
		#endif
	#else
	timer1 = tiny_counter_get();
  	timer = 0;
	#endif
	do {
		// Send ACMD41 with HCS=1 until card is ready or timeout
		response = 0;
		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD55, 0x0, C_RESP_R1, &response)) {
			return -1;
		}

		sd_card_ocr[device_id] = 0;
		ret = drvl2_sd_bcr_ac_command_set(device_id, C_SD_ACMD41, 0x41200000, C_RESP_R3, &sd_card_ocr[device_id]);
        //DBG_PRINT("1 C_SD_ACMD41 ret=%d, sd_card_ocr=0x%08x, Status=0x%08x\r\n", ret, sd_card_ocr[device_id], sd->Status);
		if (ret || !(sd_card_ocr[device_id] & 0x80000000)) {

            if (ret || (sd_card_ocr[device_id] == 0)) {
                sd_card_state[device_id] = SD_CARD_STATE_INACTIVE;
                DBG_PRINT("BADCARD! ret=%d, sd_card_ocr=0x%08x, Status=0x%08x\r\n", ret, sd_card_ocr[device_id], sd->Status);
                return -200;
            }

			// Maximum timeout value for card initialization is 1 second
			#if (defined _DRV_L1_SW_TIMER) && (_DRV_L1_SW_TIMER == 1)
			if ((sw_timer_get_counter_L()-timer) > timeout) {
			#elif _OPERATING_SYSTEM != _OS_NONE
                #if _OPERATING_SYSTEM == _OS_UCOS2
				if ((OSTimeGet()-timer)*10 > timeout)) {
				#elif _OPERATING_SYSTEM == _OS_FREERTOS
				if (xTaskCheckForTimeOut(&xTimeOut, &xRemainTicks) == pdTRUE) {
                #endif
			#else
			timer2 = tiny_counter_get();
			if (timer1 != timer2)
			{
				timer1 = timer2;
				timer++;
			}
		  	if (timer > timeout) {
			#endif
		    	// If card timeout occurs, it is an unusable card
		    	DBG_PRINT("1 Timeout C_SD_ACMD41 ret=%d, sd_card_ocr=0x%08x, Status=0x%08x\r\n", ret, sd_card_ocr[device_id], sd->Status);
		    	return -1;
            }
		} else {
			if (sd_card_ocr[device_id] & 0x40000000) {	// If CCS in sd_card_ocr is 1, it is a Ver2.0 SDHC card
				sd_card_type[device_id] = C_MEDIA_SDHC_CARD;
			} else {		// If CCS in sd_card_ocr is 0, it is a Ver2.0 standard SD card
				sd_card_type[device_id] = C_MEDIA_STANDARD_SD_CARD;
			}
			break;
		}
	} while (1) ;
    DBG_PRINT("2 C_SD_ACMD41 ret=%d, sd_card_ocr=0x%08x, Status=0x%08x\r\n", ret, sd_card_ocr[device_id], sd->Status);

	return 0;
}

/**
 * @brief   MMC card initialization.
 * @param   device_id[in]: device id.
 * @param   timeout[in]: timeout value.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_mmc_card_initiate(INT32U device_id, INT32U timeout)
{
	INT32S ret;
	INT32U response;
	#if _OPERATING_SYSTEM == _OS_FREERTOS
	TimeOut_t xTimeOut;
    TickType_t xRemainTicks = pdMS_TO_TICKS(timeout);
	#else
	INT32U timer;
	#endif
	#if _OPERATING_SYSTEM == _OS_NONE
	volatile INT32U timer1, timer2;
	#endif

	// Start MMC initialization process
	#if (defined _DRV_L1_SW_TIMER) && (_DRV_L1_SW_TIMER == 1)
	timer = (INT32U) sw_timer_get_counter_L();
	#elif _OPERATING_SYSTEM != _OS_NONE
		#if _OPERATING_SYSTEM == _OS_UCOS2
		timer = OSTimeGet();		// 10ms per tick
		#elif _OPERATING_SYSTEM == _OS_FREERTOS
		vTaskSetTimeOutState(&xTimeOut);
		#endif
	#else
	timer1 = tiny_counter_get();
  	timer = 0;
	#endif
	do {
		// Send CMD1 with voltage range set until card is ready or timeout
		response = 0;
		ret = drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD1, 0x40FF8000, C_RESP_R3, &response);
		if (ret || !(response & 0x80000000)) {
            if (drvl2_sd_check_no_card(device_id, response)) {
                sd_card_state[device_id] = SD_CARD_STATE_INACTIVE;
                DBG_PRINT("NOCARD!\r\n");
                return -100;
            }

			#if (defined _DRV_L1_SW_TIMER) && (_DRV_L1_SW_TIMER == 1)
			if ((sw_timer_get_counter_L()-timer) > timeout) {
			#elif _OPERATING_SYSTEM != _OS_NONE
                #if _OPERATING_SYSTEM == _OS_UCOS2
				if ((OSTimeGet()-timer)*10 > timeout) {
                #elif _OPERATING_SYSTEM == _OS_FREERTOS
				if (xTaskCheckForTimeOut(&xTimeOut, &xRemainTicks) == pdTRUE) {
                #endif
			#else
			timer2 = tiny_counter_get();
			if (timer1 != timer2)
			{
				timer1 = timer2;
				timer++;
			}
		  	if (timer > timeout) {
			#endif
		    	// If card timeout occurs, it is an unusable card
		    	sd_card_state[device_id] = SD_CARD_STATE_INACTIVE;

		    	return -1;
		    }

		} else {
			// Check whether voltage is acceptable
			if (!(response & 0x00FF8000)) {
				// If response is not valid, it is an unusable card
				sd_card_state[device_id] = SD_CARD_STATE_INACTIVE;

				return -1;
			}
			sd_card_type[device_id] = C_MEDIA_MMC_CARD;
			break;
		}
	} while (1) ;
	// MMC HC card
	if(response&0x40000000)
		sd_card_type[device_id] = C_MEDIA_MMCHC_CARD;
	return 0;
}

/**
 * @brief   Decode card maximum speed from CSD register.
 * @param   device_id[in]: device id.
 * @return 	0: fail, value: maximum speed.
*/
static INT32U drvl2_sd_despeed(INT32U device_id)
{
	INT32U time_value = (sd_card_csd[device_id][0] & 0x00000078) >> 3;
	INT32U tran_unit = sd_card_csd[device_id][0] & 0x00000007;

	if( ( time_value >= sizeof(speed_table) ) || ( tran_unit >= sizeof (tran_table) ) )
		return 0;
	else
		return speed_table[time_value] * tran_table[tran_unit] ;
}

/**
 * @brief   Get sd card status by ACMD13 to know features of sd, sd_card_status_buf must at least 64 byte buffer
 * @param   device_id[in]: device id.
 * @return 	0: success, others: fail.
*/
static INT32S drvl2_sd_get_sd_card_status(INT32U device_id)
{
	INT32U response;
    INT32S ret;

    // Send ACMD13 to read 512-bits SD Status, must in transfer state to use this command.
    if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD55, sd_card_rca[device_id], C_RESP_R1, &response) == 0)
    {
        if(device_id == 0){
            drvl1_sdc_block_len_set(device_id, 64);
        }else{
            drvl1_sdc30_block_len_set(device_id, 64);
        }
		if (drvl2_sd_adtc_command_set(device_id, C_SD_ACMD13, sd_card_rca[device_id], 16, (INT32U *)(&sd_card_status[device_id][0])) == 0)
        {
			ret = 0;
			#if 0 // DEBUG CODE
			{
				INT32U count;

				DBG_PRINT("SD Status:\r\n");
				for (count=0; count < 64; count++)
					DBG_PRINT("%02x", sd_card_status[device_id][count]);
				DBG_PRINT("\r\n");
			}
			#endif
		}
		else
			ret = -2;
        if(device_id == 0){
            drvl1_sdc_block_len_set(device_id, C_SD_SECTOR_SIZE);
        }else{
            drvl1_sdc30_block_len_set(device_id, C_SD_SECTOR_SIZE);
        }
	}
	else
		ret = -1;

	return ret;
}

/**
 * @brief   Get sd card au sector value from card status.
 * @param   device_id[in]: device id.
 * @return 	au sector.
*/
static INT32U drvl2_sd_get_info_au_sector(INT32U device_id)
{
	INT8U au_idx; // 4 bit
	INT32U au_sector = 0;

	// au_size 431:428
    au_idx = sd_wide_width_data_get_bits(&sd_card_status[device_id][0], 512, 431, 428);
    //DBG_PRINT("au_idx=%u\r\n", au_idx);
	if (au_idx >= 1)
		au_sector = 1<<(4+au_idx);

	return au_sector;
}

/**
 * @brief   Get sd card speed class from card status.
 * @param   device_id[in]: device id.
 * @return 	card speed class.
*/
static INT8U drvl2_sd_get_info_speed_class(INT32U device_id)
{
	INT8U speed_class_idx; // 4 bit
	INT8U speed_class = 0;

	speed_class_idx = sd_wide_width_data_get_bits(&sd_card_status[device_id][0], 512, 447, 440);
	if (speed_class_idx > 4)
	{
		DBG_PRINT("unknown speed class %u\r\n", speed_class_idx);
		return 255;
	}
	if (speed_class_idx == 4)
		speed_class = 10;
	else
		speed_class = speed_class_idx * 2;

	return speed_class;
}

/**
 * @brief   Get sd card bus width from card status.
 * @param   device_id[in]: device id.
 * @return 	card bus width.
*/
static INT8U drvl2_sd_get_info_bus_width(INT32U device_id)
{
	INT8U bus_width_idx; // 2 bit
	INT8U bus_width = 1;

	bus_width_idx = sd_wide_width_data_get_bits(&sd_card_status[device_id][0], 512, 511, 510);
	if (bus_width_idx == 0)
		bus_width = 1;
	else if (bus_width_idx == 2)
		bus_width = 4;
	else
		DBG_PRINT("unknown bus width %u\r\n", bus_width_idx);

    return bus_width;
}

/**
 * @brief   Get sd card secure mode from card status.
 * @param   device_id[in]: device id.
 * @return 	card secure mode.
*/
static INT8U drvl2_sd_get_info_secure_mode(INT32U device_id)
{
	INT8U sd_secured_mode; // 1 bit

	sd_secured_mode = sd_wide_width_data_get_bits(&sd_card_status[device_id][0], 512, 509, 509);

    return sd_secured_mode;
}

/**
 * @brief   Get sd card card type from card status.
 * @param   device_id[in]: device id.
 * @return 	sd card type.
*/
static INT8U drvl2_sd_get_info_card_type(INT32U device_id)
{
	INT16U sd_sd_card_type_idx; // 16 bit
	INT8U sd_sd_card_type = SD_CARD_TYPE_RW;

	sd_sd_card_type_idx = sd_wide_width_data_get_bits(&sd_card_status[device_id][0], 512, 495, 480);
	if (sd_sd_card_type_idx > 2)
		DBG_PRINT("unknown card type %u\r\n", sd_sd_card_type_idx);
	else
		sd_sd_card_type = sd_sd_card_type_idx;

    return sd_sd_card_type;
}

/**
 * @brief   Get sd card erase size in au unit from card status.
 * @param   device_id[in]: device id.
 * @return 	sd erase size in au unit.
*/
static INT32U drvl2_sd_get_info_erase_size_au(INT32U device_id)
{
	INT16U erase_size_au = 0;

	// erase_size 423:408
    erase_size_au = sd_wide_width_data_get_bits(&sd_card_status[device_id][0], 512, 423, 408);

	return erase_size_au;
}

/**
 * @brief   Get uhs sd card speed grade from card status.
 * @param   device_id[in]: device id.
 * @return 	uhs speed grade.
*/
static INT8U drvl2_sd_get_info_uhs_speed_grade(INT32U device_id)
{
	INT8U uhs_speed_grade_idx; // 4 bit
	INT8U uhs_speed_grade = SD_CARD_UHS_SPEED_GRADE_0;

	// 399:396 UHS_SPEED_GRADE
	uhs_speed_grade_idx = sd_wide_width_data_get_bits(&sd_card_status[device_id][0], 512, 399, 396);
	if (uhs_speed_grade_idx == 0)
		uhs_speed_grade = SD_CARD_UHS_SPEED_GRADE_0;
	else if (uhs_speed_grade_idx == 1)
		uhs_speed_grade = SD_CARD_UHS_SPEED_GRADE_1;
	else if (uhs_speed_grade_idx == 3)
		uhs_speed_grade = SD_CARD_UHS_SPEED_GRADE_3;
	else
		DBG_PRINT("unknown uhs speed grade %u\r\n", uhs_speed_grade_idx);


	return uhs_speed_grade;
}

/**
 * @brief   Get uhs sd card au sector from card status.
 * @param   device_id[in]: device id.
 * @return 	uhs au sector.
*/
static INT32U drvl2_sd_get_info_uhs_au_sector(INT32U device_id)
{
	INT8U uhs_au_idx; // 4 bit
	INT32U uhs_au_sector = 0;

	// 395:392 UHS_AU_SIZE
    uhs_au_idx = sd_wide_width_data_get_bits(&sd_card_status[device_id][0], 512, 395, 392);
    //DBG_PRINT("uhs_au_idx=%u\r\n", uhs_au_idx);
	if (uhs_au_idx >= 7)
		uhs_au_sector = 1<<(4+uhs_au_idx);

	return uhs_au_sector;
}


/**
 * @brief   get the SD STATUS info of SD and explain each field of it.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_get_sd_info_card_status(INT32U device_id)
{
    if (drvl2_sd_get_sd_card_status(device_id))
	{
		if (drvl2_sd_get_sd_card_status(device_id)) // try second time
			return - 1;
	}
    sd_data_bus_width[device_id] = drvl2_sd_get_info_bus_width(device_id);
    DBG_PRINT("sd_data_bus_width=%u\r\n", sd_data_bus_width[device_id]);

    sd_secured_mode[device_id] = drvl2_sd_get_info_secure_mode(device_id);
    DBG_PRINT("sd_secured_mode=%u\r\n", sd_secured_mode[device_id]);

    // 508:502 Reserved for Security Functions (Refer to Part 3 Security Specification)

    // 501:496 reserved

    sd_sd_card_type[device_id] = drvl2_sd_get_info_card_type(device_id);
    DBG_PRINT("sd_sd_card_type=%u\r\n", sd_sd_card_type[device_id]);

    // size_of_protected_area 479:448, 32 bit

    sd_speed_class[device_id] = drvl2_sd_get_info_speed_class(device_id);
    DBG_PRINT("sd_speed_class=%u\r\n", sd_speed_class[device_id]);

    // performance_move 439:432

	// erase_size 423:408
	sd_erase_size_au[device_id] = drvl2_sd_get_info_erase_size_au(device_id);

    // au_size 431:428
	sd_flow_device_au_sectors[device_id] = drvl2_sd_get_info_au_sector(device_id);

        #if SDC_AU_SIZE_USE_6M
        sd_flow_device_au_sectors[device_id] = 12288UL;
        #endif

	if ((sd_erase_size_au[device_id] != 0) && (sd_flow_device_au_sectors[device_id] != 0))
		/*sd_flow_write_au_sectors[device_id] = sd_flow_device_au_sectors[device_id];*/
        #if SDC_AU_SIZE_USE_6M
            sd_flow_write_au_sectors[device_id] = ((sd_erase_size_au[device_id] > 4) ? 4 : sd_erase_size_au[device_id]) * sd_flow_device_au_sectors[device_id];
        #else
		sd_flow_write_au_sectors[device_id] = ((sd_erase_size_au[device_id] > 8) ? 8 : sd_erase_size_au[device_id]) * sd_flow_device_au_sectors[device_id];
        #endif
	else if (sd_flow_device_au_sectors[device_id] != 0)
		sd_flow_write_au_sectors[device_id] = sd_flow_device_au_sectors[device_id];
	else
	{
		sd_flow_device_au_sectors[device_id] = 1024;
		sd_flow_write_au_sectors[device_id] = 1024;
	}
	DBG_PRINT("sd_flow_write_au_sectors=%u erase_au=%u\r\n", sd_flow_write_au_sectors[device_id], sd_erase_size_au[device_id]);

    // reserve 427:424

    // erase_size 423:408
	sd_erase_size_au[device_id] = drvl2_sd_get_info_erase_size_au(device_id);
	DBG_PRINT("sd_erase_size_au=%u\r\n", sd_erase_size_au[device_id]);

    // 407:402 ERASE_TIMEOUT

    // 401:400 ERASE_OFFSET

    // 399:396 UHS_SPEED_GRADE
	sd_uhs_speed_grade[device_id] = drvl2_sd_get_info_uhs_speed_grade(device_id);
	DBG_PRINT("sd_uhs_speed_grade=%u\r\n", sd_uhs_speed_grade[device_id]);

    // 395:392 UHS_AU_SIZE
	sd_uhs_au_sectors[device_id] = drvl2_sd_get_info_uhs_au_sector(device_id);
	DBG_PRINT("sd_uhs_au_sectors=%u\r\n", sd_uhs_au_sectors[device_id]);

    // 391:312 reserved

    // 311:0 reserved for manufacturer

	return 0;
}

/**
 * @brief   Get sd card erase, write and base au size.
 * @param   device_id[in]: device id.
 * @param   *p_write_au[out]: write au size.
 * @param   *p_erased_au[out]: erased au size.
 * @param   *p_sd_base_au[out]: base au size.
 * @return 	0: success, -1 fail.
*/
INT32S drvl2_sd_get_au_size(INT32U device_id, INT32U *p_write_au, INT32U *p_erased_au, INT32U *p_sd_base_au)
{
    if (device_id > 1)
        return -1;

    *p_erased_au = sd_erase_size_au[device_id];
    *p_write_au = sd_erase_size_au[device_id];
	*p_sd_base_au = sd_flow_device_au_sectors[device_id];

	return 0;
}

#if (SD_DEBUG_CSD_INFO == 1) || (SD_DEBUG_CID_INFO == 1) || (SD_DEBUG_OSR_INFO == 1)
static INT32U swapword(INT32U in_word)
{
	INT32U out_word;

	out_word = (in_word & 0x000000ff) << 24;
	out_word |= (((in_word >> 8) & 0x000000ff) << 16);
	out_word |= (((in_word >> 16) & 0x000000ff) << 8);
	out_word |= ((in_word >> 24) & 0x000000ff);

	return out_word;
}
#endif

static ALIGN64 INT32U switch_function_data[16];		// 512 bits

/* transfer state */
/* result: sd_speed_mode, switch_function_data */
static INT32S drvl2_sdcard_set_bus_speed(INT32U device_id, INT32U limit_speed)
{
	INT32S ret = 0;

	sd_speed_mode = DEFAULT_SDR12;

	// Check limit clock and SD spec version
	if ( (sd_card_scr[device_id][0]&0x0F)>1 )
	{
		// Set block length to 64 bytes so that we can read status data of CMD6
		if(device_id == 0){
            drvl1_sdc_block_len_set(device_id, 64);
        }else{
            drvl1_sdc30_block_len_set(device_id, 64);
        }
        // williamyeo, 0x00FFFF01 change to 0x00FFF101, query for driver strength A
		if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x00FFF101, 16, &switch_function_data[0]) == 0) {
			// Check whether card supports high-speed mode
            if(device_id == 0)
            {
                if ((switch_function_data[3] & 0x00000300) == 0x00000300) {
                    if (limit_speed>25000000) {
                        sd_speed_mode = HISPEED_SDR25;
                        drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x80FFFF01, 16, &switch_function_data[0]);
                    }
                    else
                        // switch to SDR12
                        drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x80FFFF00, 16, &switch_function_data[0]);
                }
                else
                    // switch to SDR12
                    drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x80FFFF00, 16, &switch_function_data[0]);
            }else{
                if ((switch_function_data[3] & 0x00000300) == 0x00000300)
                {
                    DBG_PRINT("*-Support Hi-Speed SDR25-*\r\n");
                    sd_speed_mode = HISPEED_SDR25;
                    if(sd_support_1p8v == 0 || limit_speed<=50000000)
                    {
                        if (limit_speed>25000000)
                            drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x80FFFF01, 16, &switch_function_data[0]);
                        else {
                            // switch to SDR12
                            sd_speed_mode = DEFAULT_SDR12;
                            drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x80FFFF00, 16, &switch_function_data[0]);
                        }
                        goto L_SETBLOCKLEN;
                    }
                    // williamyeo, 0x00FFFFF2 change to 0x00FFF1F2, query for driver strength A
                    if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x00FFF1F2, 16, &switch_function_data[0]) == 0)
                    {
                        DBG_PRINT("2Query sw func grp 1=0x%x\r\n", switch_function_data[3]);
                        if ((switch_function_data[3] & 0x00000400) == 0x00000400)
                        {
                            DBG_PRINT("*-Support SDR50-*\r\n");
                            sd_speed_mode = SDR50;

                            // williamyeo, 0x80FFFFF2 change to 0x80FFF1F2, use driver strength A to fix some sd card crc error
                            INT32U cmd_arg = 0x80FFFFF2;

                            if (sd_card_driver_strength[device_id]) {
                                if ((switch_function_data[3] & 0x0F000000) == 0x01000000)
                                    cmd_arg = 0x80FFF1F2;
                            }

                            if(drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, cmd_arg, 16, &switch_function_data[0]) == 0)
                            {
                                DBG_PRINT("1Query func can sw =0x%x\r\n", switch_function_data[4]);
#if 0
                                if((switch_function_data[4] & 0x00000002)!=0x00000002)
                                {
                                    sd_speed_mode = HISPEED_SDR25;
                                    if(drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x80FFFF01, 16, &switch_function_data[0]) == 0)
                                    {
                                        DBG_PRINT("2Query func can sw =0x%x\r\n", switch_function_data[4]);
                                        if((switch_function_data[4] & 0x00000001)!=0x00000001)
                                        {
                                            DBG_PRINT("2Query func can sw fail\r\n");
                                            return -1;
                                        }
                                        DBG_PRINT("*-SW 2 Hi-Speed SDR25-*\r\n");
                                    }
                                }
#endif
                            }
                        }
                        else
                        {
                            if(drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x80FFFF01, 16, &switch_function_data[0]) == 0)
                            {
                                DBG_PRINT("3Query func can sw =0x%x\r\n", switch_function_data[4]);
                                if((switch_function_data[4] & 0x00000001)!=0x00000001)
                                {
                                    return -1;
                                }
                                DBG_PRINT("*-SW to Hi-Speed SDR25-*\r\n");
                            }
                        }
                    }
                    else {
                        DBG_PRINT("*-NO SDR50-*\r\n");
                        goto L_SDR25;
                    }
                }
                else
                {
                    // switch to SDR12
                    sd_speed_mode = DEFAULT_SDR12;
                    drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x80FFFF00, 16, &switch_function_data[0]);
                }
            }
        }
        else {
        L_SDR25:
            if ((switch_function_data[3] & 0x00000300) == 0x00000300) {
                if (limit_speed>25000000) {
                    sd_speed_mode = HISPEED_SDR25;
                    drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x80FFFF01, 16, &switch_function_data[0]);
                }
                else {
                    // switch to SDR12
                    sd_speed_mode = DEFAULT_SDR12;
                    drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x80FFFF00, 16, &switch_function_data[0]);
                }
            }
            else {
                // switch to SDR12
                sd_speed_mode = DEFAULT_SDR12;
                drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x80FFFF00, 16, &switch_function_data[0]);
            }
        }

L_SETBLOCKLEN:
        // Restore sector length
        if(device_id == 0){
            drvl1_sdc_block_len_set(device_id, C_SD_SECTOR_SIZE);
        }else{
            drvl1_sdc30_block_len_set(device_id, C_SD_SECTOR_SIZE);
        }
    }

	return ret;
}

/* result: ACMD51 to read sd_card_scr */
static INT32S drvl2_sdcard_read_scr(INT32U device_id)
{
	INT32U response = 0;
    INT32S ret = 0;

	// Send ACMD51 to read 64-bits SD configuration register(SCR)
	if(device_id == 0){
        if (drvl1_sdc_block_len_set(device_id, 8)) {
            DBG_PRINT("SDERR 21\r\n");
            return -1;
        }
    }else{
        if (drvl1_sdc30_block_len_set(device_id, 8)) {
            DBG_PRINT("SD3ERR 21\r\n");
            return -1;
        }
    }
    // Send ACMD51 to read 64-bits SD configuration register(SCR)
/*
    if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD55, sd_card_rca[device_id], C_RESP_R1, &response) ||
    	drvl2_sd_adtc_command_set(device_id, C_SD_ACMD51, sd_card_rca[device_id], 2, &sd_card_scr[device_id][0])) {
		DBG_PRINT("SDERR 22\r\n");
		return -1;
	}
*/
    INT8U C55retrycnt = 0, AC51retrycnt = 0;
L_RETRY:
    if(drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD55, sd_card_rca[device_id], C_RESP_R1, &response)){
        DBG_PRINT("CMD55 ERR\r\n");
        //return -1;
        if(++C55retrycnt >= 5) {
            return -1;
        }
        goto L_RETRY;
    }
    if(drvl2_sd_adtc_command_set(device_id, C_SD_ACMD51, sd_card_rca[device_id], 2, &sd_card_scr[device_id][0])){
        DBG_PRINT("ACMD51 ERR\r\n");
        //return -1;
        if(++AC51retrycnt >= 5) {
            return -1;
        }
        goto L_RETRY;
    }

	// Reset block length to C_SD_SECTOR_SIZE bytes
	if(device_id == 0){
        if (drvl1_sdc_block_len_set(device_id, C_SD_SECTOR_SIZE)) {
            DBG_PRINT("SDERR 23\r\n");
            return -1;
        }
    }else{
        if (drvl1_sdc30_block_len_set(device_id, C_SD_SECTOR_SIZE)) {
            DBG_PRINT("SD3ERR 23\r\n");
            return -1;
        }
    }

	#if (SD_DEBUG_SCR_INFO == 1)
	{
		INT8U SCR_STRUCTURE;
		INT8U SD_SPEC;
		INT8U DATA_STAT_AFTER_ERASE;
		INT8U SD_SECURITY;
		INT8U SD_BUS_WIDTHS;
		INT8U SD_SPEC3;
		INT8U EX_SECURITY;
		INT8U SD_SPEC4;
		INT8U CMD_SUPPORT;

		SCR_STRUCTURE = sd_wide_width_data_get_bits((INT8U *)(&sd_card_scr[device_id][0]), 64, 63, 60);
		SD_SPEC = sd_wide_width_data_get_bits((INT8U *)(&sd_card_scr[device_id][0]), 64, 59, 56);
		DATA_STAT_AFTER_ERASE = sd_wide_width_data_get_bits((INT8U *)(&sd_card_scr[device_id][0]), 64, 55, 55);
		SD_SECURITY = sd_wide_width_data_get_bits((INT8U *)(&sd_card_scr[device_id][0]), 64, 54, 52);
		SD_BUS_WIDTHS = sd_wide_width_data_get_bits((INT8U *)(&sd_card_scr[device_id][0]), 64, 51, 48);
		SD_SPEC3 = sd_wide_width_data_get_bits((INT8U *)(&sd_card_scr[device_id][0]), 64, 47, 47);
		EX_SECURITY = sd_wide_width_data_get_bits((INT8U *)(&sd_card_scr[device_id][0]), 64, 46, 43);
		SD_SPEC4 = sd_wide_width_data_get_bits((INT8U *)(&sd_card_scr[device_id][0]), 64, 42, 42);
		CMD_SUPPORT = sd_wide_width_data_get_bits((INT8U *)(&sd_card_scr[device_id][0]), 64, 35, 32);

		DBG_PRINT("SCR=0x%08x, 0x%08x\r\n", sd_card_scr[device_id][0], sd_card_scr[device_id][1]);
		DBG_PRINT("SCR_STRUCTURE=%u\r\n", SCR_STRUCTURE);
		DBG_PRINT("SD_SPEC=%u\r\n", SD_SPEC);
		DBG_PRINT("DATA_STAT_AFTER_ERASE=%u\r\n", DATA_STAT_AFTER_ERASE);
		DBG_PRINT("SD_SECURITY=%u\r\n", SD_SECURITY);
		DBG_PRINT("SD_BUS_WIDTHS=0x%01x\r\n", SD_BUS_WIDTHS);
		DBG_PRINT("SD_SPEC3=%u\r\n", SD_SPEC3);
		DBG_PRINT("EX_SECURITY=0x%01x\r\n", EX_SECURITY);
		DBG_PRINT("SD_SPEC4=%u\r\n", SD_SPEC4);
		DBG_PRINT("CMD_SUPPORT=0x%01x\r\n", CMD_SUPPORT);
	}
	#endif

	return ret;
}

static INT32S drvl2_sdcard_set_bus_width(INT32U device_id)
{
	INT32U response = 0;
    INT32S ret = 0;

	if (sd_card_scr[device_id][0] & 0x00000400) {		// Check whether this SD card supports 4-bits bus width
		if (sd_card_use_one_bit_mode[device_id])
			DBG_PRINT("SD USE 1 BIT ONLY!\r\n");
		else
		{
            int try_count = 0;

            do {
                sd_card_bus_width[device_id] = 4;
                // Send ACMD6 to set the card to 4-bit bus width
                ret = drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD55, sd_card_rca[device_id], C_RESP_R1, &response);
                if(ret<0){
                    DBG_PRINT("SDERR 24.7. try_count=%u, ret=%d\r\n", try_count, ret);
                    return -2;
                } else {
                    if(device_id == 0){
                        drvl1_sdc_set_command_timeout_ext_wait(device_id, 1);
                    }else{
                        drvl1_sdc30_set_command_timeout_ext_wait(device_id, 1);
                    }
                    //drvl1_sdc_set_data_timeout(device_id, ENABLE);
                    drvl1_sdc_set_cmd_timeout(device_id, DISABLE);
                    ret = drvl2_sd_bcr_ac_command_set(device_id, C_SD_ACMD6, 0x2, C_RESP_R1, &response);
                    drv_msec_wait(1);//make sure sd switch to 4bit mode before switch to standby
                    drvl1_sdc_set_cmd_timeout(device_id, ENABLE);
                    //drvl1_sdc_set_data_timeout(device_id, DISABLE);
                    if(device_id == 0){
                        drvl1_sdc_set_command_timeout_ext_wait(device_id, 0);
                    }else{
                        drvl1_sdc30_set_command_timeout_ext_wait(device_id, 0);
                    }
                    if((ret<0)||( (response&0x1F20)!= 0x920)){//check response BIT8 ready and BIT9~12 state = transfer state (4)
                        volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
                        INT32U sd_Status = sd->Status;

                        if(device_id == 0){
                            drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
                        }else{
                            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
                        }
                        DBG_PRINT("SDERR 24.1 ret=%d response=0x%08x status=0x%08x\r\n", ret, response, sd_Status);
                        if (try_count == 4) {
                            DBG_PRINT("SDERR 24.2\r\n");
                            return -1;
                        }
                    }
                    else
                        break;
                }
            } while (try_count++ < 5);
            if(device_id == 0){
                drvl1_sdc_bus_width_set(device_id, 4);			// Set bus width to 4 bits
            }else{
                drvl1_sdc30_bus_width_set(device_id, 4);			// Set bus width to 4 bits
            }
			DBG_PRINT("success switch sd to 4 bit. try_count=%u, response=0x%08x\r\n", try_count, response);
		}
	}

	return 0;
}

 /* standby state, will switch back tranfer state */
 /* result: use CMD9 to read sd_card_csd, and sd_card_speed that limit to limit_speed */
 static INT32S drvl2_sdcard_read_csd(INT32U device_id, INT32U limit_speed)
 {
	INT32U response = 0;
	INT32U c_size, mult, block_len;
	INT32U max_speed;
    INT32U delay_10us = ((MCLK/1000000UL)*376UL/9600) + 1;

	// Switch back to standby state
	drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD7_NO_RESONSE, 0x0, C_RESP_R0, &response);
    sd_card_state[device_id] = SD_CARD_STATE_STANDBY;

	drv_msec_wait(1); // williamyeo, fix timeout that happen at C_SD_CMD9

	// Send CMD9 and read CSD Register
	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD9, sd_card_rca[device_id], C_RESP_R2, &sd_card_csd[device_id][0])) {
		DBG_PRINT("SDERR 25\r\n");
		return -1;
	}

	max_speed = drvl2_sd_despeed(device_id);
	if( max_speed == 0 )
	{
        DBG_PRINT("SDERR 26\r\n");
		return -1;
    }

  	if (max_speed > limit_speed) {
  		max_speed = limit_speed;
  	}
	sd_card_speed[device_id] = max_speed;

	#if (SD_DEBUG_CSD_INFO == 1)
	INT32U sd_card_csd_swap[4];
	{
		INT8U csd_ver;
		INT32U kk;
		INT8U sd_permanant_write_protection;
		INT8U sd_temporary_write_protection;
		INT8U sd_temporary_write_protec_group_enable;
		INT32U TAAC;
		INT32U NSAC;

		for (kk = 0; kk < 4; kk++)
				sd_card_csd_swap[kk] = swapword(sd_card_csd[device_id][kk]);

		TAAC = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 119, 112);
		NSAC = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 111, 104);
		DBG_PRINT("TAAC=%u, NSAC=%u\r\n", TAAC, NSAC);

		csd_ver = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 127, 126);

		sd_dump_buffer((INT8U *)(&sd_card_csd_swap[0]), 16);
		sd_permanant_write_protection = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 13, 13);
		sd_temporary_write_protection = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 12, 12);
		sd_temporary_write_protec_group_enable = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 31, 31);
		DBG_PRINT("SD CSD info: version[%x] pwp=%u twp=%u wpge=%u\r\n", csd_ver, \
																sd_permanant_write_protection, \
																sd_temporary_write_protection, \
																sd_temporary_write_protec_group_enable);
	}
	#endif

	// Calculate Totoal Memory Size
	if (sd_card_type[device_id] == C_MEDIA_SDHC_CARD) {

        #if 0 // comment
        word 0 127:96
        word 1 95:64
        word 2 63:32
        word 3 31:0

        69:48 -> word 1 [5:0]  word 2[31:16]
        #endif

		c_size = ((sd_card_csd[device_id][2]>>16) & 0x0000FFFF)+((sd_card_csd[device_id][1]&0x0000003F)<<16);
		sd_card_total_sector[device_id] = (c_size+1) << 10;
                DBG_PRINT("SDHC_CARD. c_size=%d total_sector=%u\r\n", c_size, sd_card_total_sector[device_id]);
		#if (SD_DEBUG_CSD_INFO == 1)
		{
			INT32U my_c_size;

			my_c_size = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 69, 48);
			DBG_PRINT("my_c_size=0x%x\r\n", my_c_size);
		}
		#endif
	} else {
		block_len = (sd_card_csd[device_id][1] >> 16) & 0xF;
		c_size = ((sd_card_csd[device_id][1] & 0x3FF) << 2) | ((sd_card_csd[device_id][2] >> 30) & 0x3);
		mult = (sd_card_csd[device_id][2] >> 15) & 0x7;
		sd_card_total_sector[device_id] = (c_size + 1) << (mult + 2 + block_len - 9);
        DBG_PRINT("SD_CARD. c_size=%d total_sector=%u block_len=%d mult=%d\r\n", c_size, sd_card_total_sector[device_id], block_len, mult);
		#if (SD_DEBUG_CSD_INFO == 1)
		{
			INT32U my_c_size;
			INT32U my_block_len;
			INT32U mult;

			my_c_size = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 73, 62);
			my_block_len = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 83, 80);
			mult = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 49, 47);
			DBG_PRINT("my_c_size=%u my_block_len=%u mult=%u\r\n", my_c_size, my_block_len, mult);
		}
		#endif
	}

	// Change to transfer state
	response = 0;
	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD7, sd_card_rca[device_id], C_RESP_R1B, &response)) {
		// sdc speed back to 400k
        if(device_id == 0)
            drvl1_sdc_clock_set(device_id, 400000);
        else
            drvl1_sdc30_clock_set(device_id, 400000);
        drv_tiny_wait(delay_10us);
		DBG_PRINT("SDERR 28\r\n");
		return -1;
	}
	sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;

    if(device_id == 1)
    {
        if(sd_speed_mode == SDR50)
        {
            max_speed = 100000000;
        }
        else if(sd_speed_mode == HISPEED_SDR25)
        {
            max_speed = 50000000;
        }
        else
        {
            max_speed = 25000000;
        }
        if (max_speed > limit_speed) {
            max_speed = limit_speed;
        }
        DBG_PRINT("max_speed=%u, limit_speed=%u\r\n", max_speed, limit_speed);
    }

    if((device_id >=0) && (device_id < 2))
        sd_card_speed[device_id] = max_speed;
    if(device_id == 0){
        if (drvl1_sdc_clock_set(device_id, max_speed))
        {
            DBG_PRINT("SDERR 27\r\n");
            return -1;
        }
    }else{
        if (drvl1_sdc30_clock_set(device_id, max_speed))
        {
            DBG_PRINT("SDERR 27.1\r\n");
            return -1;
        }
    }
    drv_tiny_wait(delay_10us);

	return 0;
}

INT32U sd30_clockrate_tunning_threshold = 50000000;
static INT8U clk_delay_offset_result = 0;
static INT8U clk_delay_tune_result = 0; // 0 is success, 1 is fail
static INT8U cmd_data_delay_tune_result = 0; // 0 is success, 1 is fail

static INT32S drvl2_sd30_tunning_18V_clock_cmd_data_phase(INT32U device_id, INT8U apply_sd_in_clk_source)
{
    INT8U apply_dummy_clk = 0;
    INT8U apply_center_clk = 1; // suggest turn on as 1, this option being tested

	// tunning sd30 clock, cmd, data pins start

	// 20250311, After confirm with Jason modify this value to > 50000000, old is >= 960000000
    // modify this value purpose is to doing clock tunning when over 50000000

    DBG_PRINT("sd30_clockrate_tunning_threshold %u, sd_support_1p8v %u\r\n", sd30_clockrate_tunning_threshold, sd_support_1p8v);
    {
        drvl1_sdc30_block_len_set(device_id, 64);//(JC)resize block length for tuning
        INT8U retry_cnt = 0;
        INT8U tuning_idx;
        INT8U first_bound;
        INT8U second_bound;
        INT8U clk_do_count = (apply_sd_in_clk_source) ? 2 : 1;
        INT8U clk_idx;
        INT8U clk_boundary[2][2];
        INT8U clk_tune_result[2];
        INT8U clk_delay_tune_range[2];

        clk_delay_offset_result = 0;
        clk_delay_tune_result = 0;
        cmd_data_delay_tune_result = 0;

        drvl1_sdc30_sdclk_delay_set(device_id, TUNING_CTRL_BL);
        drvl1_sdc30_sdclk_delay_offset_set(device_id, 0);
        drvl1_sdc30_sdcmd_delay_set(device_id, TUNING_CTRL_BL);
        drvl1_sdc30_sddata_delay_set(device_id, TUNING_CTRL_BL);

        if (apply_dummy_clk) {
            drvl1_sdc30_dummyclk_en(device_id, 1);
            vTaskDelay(3);
        }

        clk_tune_result[0] = 0;
        clk_tune_result[1] = 0;
        for (clk_idx = 0; clk_idx < clk_do_count; clk_idx++) {
            drvl1_sdc30_in_clk_sel(device_id, clk_idx);
            clk_boundary[clk_idx][0] = 0;
            clk_boundary[clk_idx][1] = 0;
            first_bound = 0;
            second_bound = 0;
            tuning_idx = TUNING_CTRL_BL;
            while (tuning_idx < TUNING_CTRL_BH) {
                drvl1_sdc30_sdclk_delay_set(device_id, tuning_idx);
                drvl1_sdc30_sdclk_delay_offset_set(device_id, 0);
                clk_delay_offset_result = drvl1_sdc30_sdclk_delay_offset_result(device_id);
                if (drvl2_sd30_tuning_cmd(device_id)==0) {
                    if (!first_bound) {
                        clk_boundary[clk_idx][0] = tuning_idx;
                        first_bound = 1;
                        //DBG_PRINT("sd30 clk phase first success %u, clk_idx=%u! pre_post=0x%x\r\n", clk_boundary[clk_idx][0], clk_idx, clk_delay_offset_result);
                    }
                }
                else if (first_bound) {
                    if(tuning_idx >= 1)
                        clk_boundary[clk_idx][1] = tuning_idx-1;
                    else
                        clk_boundary[clk_idx][1] = 0;
                    second_bound = 1;
                    //DBG_PRINT("sd30 clk phase end success %u, clk_idx=%u ! pre_post=0x%x\r\n", clk_boundary[clk_idx][1], clk_idx, clk_delay_offset_result);
                }
                if (second_bound)
                    break;
                tuning_idx++;
            } // end while

            if (first_bound) {
                if (!second_bound) {
                    if (tuning_idx >= 1)
                        clk_boundary[clk_idx][1] = tuning_idx-1;
                    else
                        clk_boundary[clk_idx][1] = 0;
                    if (tuning_idx != TUNING_CTRL_BH) // should not happen
                        DBG_PRINT("sd30 clk phase end success not found, clk_idx=%u, tuning_idx=%u\r\n", clk_idx, tuning_idx);
                }
                clk_tune_result[clk_idx] = 0; // clk phase success
            }
            else {
                clk_tune_result[clk_idx] = 1; // clk phase fail
                DBG_PRINT("sd30 clk phase not found, clk_idx=%u, tuning_idx=%u\r\n", clk_idx, tuning_idx);
            }
            DBG_PRINT("sd30 clk source %s phase success range [%u, %u] ! pre_post=0x%x\r\n", \
                        ((clk_idx == 0) ? "sd-host" : "io-pad"), \
                        clk_boundary[clk_idx][0], clk_boundary[clk_idx][1], clk_delay_offset_result);
        } // end for

        if (clk_do_count > 1) {
            clk_delay_tune_range[0] = clk_boundary[0][1] - clk_boundary[0][0] + 1;
            clk_delay_tune_range[1] = clk_boundary[1][1] - clk_boundary[1][0] + 1;
            if (clk_delay_tune_range[1] > clk_delay_tune_range[0]) {
                drvl1_sdc30_in_clk_sel(device_id, 1);
                clk_delay_tune_result = clk_tune_result[1];
                sd_clk_boundary[0] = clk_boundary[1][0];
                sd_clk_boundary[1] = clk_boundary[1][1];
                DBG_PRINT("sd30 clk source use io-pad range %u, sd-host range %u\r\n", clk_delay_tune_range[1], clk_delay_tune_range[0]);
            }
            else {
                drvl1_sdc30_in_clk_sel(device_id, 0);
                clk_delay_tune_result = clk_tune_result[0];
                sd_clk_boundary[0] = clk_boundary[0][0];
                sd_clk_boundary[1] = clk_boundary[0][1];
                DBG_PRINT("sd30 clk source use sd-host range %u, io-pad range %u\r\n", clk_delay_tune_range[0], clk_delay_tune_range[1]);
            }
        }
        else {
            clk_delay_tune_range[0] = clk_boundary[0][1] - clk_boundary[0][0] + 1;
            drvl1_sdc30_in_clk_sel(device_id, 0);
            clk_delay_tune_result = clk_tune_result[0];
            sd_clk_boundary[0] = clk_boundary[0][0];
            sd_clk_boundary[1] = clk_boundary[0][1];
            DBG_PRINT("sd30 clk source use sd-host range %u\r\n", clk_delay_tune_range[0]);
        }

        if (apply_center_clk) {
            drvl1_sdc30_sdclk_delay_set(device_id, ((sd_clk_boundary[1]-sd_clk_boundary[0]+1)/2) + sd_clk_boundary[0]);
            drvl1_sdc30_sdclk_delay_offset_set(device_id, 0);
        }
        else {
            drvl1_sdc30_sdclk_delay_set(device_id, TUNING_CTRL_BL);
            drvl1_sdc30_sdclk_delay_offset_set(device_id, 0);
        }
        drvl1_sdc30_sdcmd_delay_set(device_id, TUNING_CTRL_BL);
        drvl1_sdc30_sddata_delay_set(device_id, TUNING_CTRL_BL);
        clk_delay_offset_result = drvl1_sdc30_sdclk_delay_offset_result(device_id);
        DBG_PRINT("sd30 clk phase success range [%u, %u, %u] ! pre_post=0x%x\r\n", sd_clk_boundary[0], sd_clk_boundary[1], sd_clk_boundary[1] - sd_clk_boundary[0] + 1, clk_delay_offset_result);

        if (apply_dummy_clk) {
            drvl1_sdc30_dummyclk_en(device_id, 1);
            vTaskDelay(3);
        }

        sd_cmd_data_boundary[0] = 0;
        sd_cmd_data_boundary[1] = 0;
        first_bound = 0;
        second_bound = 0;
        tuning_idx = TUNING_CTRL_BL;
        while (tuning_idx < TUNING_CTRL_BH) {
            drvl1_sdc30_sdcmd_delay_set(device_id, tuning_idx);
            drvl1_sdc30_sddata_delay_set(device_id, tuning_idx);
            clk_delay_offset_result = drvl1_sdc30_sdclk_delay_offset_result(device_id);
            if (drvl2_sd30_tuning_cmd(device_id)==0) {
                if (!first_bound) {
                    sd_cmd_data_boundary[0] = tuning_idx;
                    first_bound = 1;
                    //DBG_PRINT("sd30 cmd data phase first success %u! pre_post=0x%x\r\n", sd_cmd_data_boundary[0], clk_delay_offset_result);
                }
            }
            else if (first_bound) {
                if(tuning_idx >= 1)
                    sd_cmd_data_boundary[1] = tuning_idx-1;
                else
                    sd_cmd_data_boundary[1] = 0;
                second_bound = 1;
                //DBG_PRINT("sd30 cmd data phase end success %u! pre_post=0x%x\r\n", sd_cmd_data_boundary[1], clk_delay_offset_result);
            }
            if (second_bound)
                break;
            tuning_idx++;
        } // end while

        if (first_bound) {
            if (!second_bound) {
                if (tuning_idx >= 1)
                    sd_cmd_data_boundary[1] = tuning_idx-1;
                else
                    sd_cmd_data_boundary[1] = 0;
                if (tuning_idx != TUNING_CTRL_BH)
                    DBG_PRINT("sd30 cmd data phase end success not found, tuning_idx=%u\r\n", tuning_idx);
            }
            cmd_data_delay_tune_result = 0; // cmd data phase success
        }
        else {
            cmd_data_delay_tune_result = 1; // cmd data phase fail
            DBG_PRINT("sd30 cmd data phase not found, tuning_idx=%u\r\n", tuning_idx);
        }

        drvl1_sdc30_sdclk_delay_set(device_id, ((sd_clk_boundary[1]-sd_clk_boundary[0]+1)/2) + sd_clk_boundary[0]);
        drvl1_sdc30_sdclk_delay_offset_set(device_id, 0);
        drvl1_sdc30_sdcmd_delay_set(device_id, ((sd_cmd_data_boundary[1]-sd_cmd_data_boundary[0]+1)/2) + sd_cmd_data_boundary[0]);
        drvl1_sdc30_sddata_delay_set(device_id, ((sd_cmd_data_boundary[1]-sd_cmd_data_boundary[0]+ 1)/2) + sd_cmd_data_boundary[0]);
        clk_delay_offset_result = drvl1_sdc30_sdclk_delay_offset_result(device_id);
        DBG_PRINT("cmd data phase success range [%u, %u, %u] ! pre_post=0x%x\r\n", sd_cmd_data_boundary[0], sd_cmd_data_boundary[1], sd_cmd_data_boundary[1] - sd_cmd_data_boundary[0] + 1, clk_delay_offset_result);

        // print final tune timing
        drvl1_sdc30_tune_timing_print(device_id);

L_RETRY2:
        if (retry_cnt == 0) {
            if (apply_dummy_clk) {
                drvl1_sdc30_dummyclk_en(device_id, 1);
                vTaskDelay(3);
            }
        }

        cache_invalid_range((unsigned int)(&sd_card_tuningblock[device_id][0]), 64);
        if (!drvl2_sd_adtc_command_set(device_id, C_SD_CMD19, 0, 16, (INT32U *)(&sd_card_tuningblock[device_id][0])))
        {
            DBG_PRINT("=>0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\r\n", sd_card_tuningblock[device_id][0], sd_card_tuningblock[device_id][1], sd_card_tuningblock[device_id][2], sd_card_tuningblock[device_id][3], sd_card_tuningblock[device_id][4], sd_card_tuningblock[device_id][5]);
            if(strncmp((char const *)(&sd_card_tuningblock[device_id][0]), &sd_tuning_pattern[0], 64) == 0)
            {
                DBG_PRINT("cp succ2, Tuning clk passed!! retry_cnt=%u\r\n", retry_cnt);
            }
            else
            {
                if (device_id == 1)
                    drvl1_sdc30_stop_controller(device_id, 1);
                DBG_PRINT("Tuning clk failed1!! retry_cnt=%u\r\n", retry_cnt);
                if(retry_cnt>3)
                {
                    if (apply_dummy_clk)
                        drvl1_sdc30_dummyclk_en(device_id, 0);
                    drvl1_sdc30_block_len_set(device_id, C_SD_SECTOR_SIZE);
                    DBG_PRINT("Tuning step found but FAIL! retry_cnt=%u\r\n", retry_cnt);
                    return -1;
                }
                retry_cnt++;
                goto L_RETRY2;
            }
        }
        else
        {
            DBG_PRINT("Tuning clk failed2!! retry_cnt=%u\r\n", retry_cnt);
            if (device_id == 1)
                drvl1_sdc30_stop_controller(device_id, 1);
            if (apply_dummy_clk)
                drvl1_sdc30_dummyclk_en(device_id, 0);
            drvl1_sdc30_block_len_set(device_id, C_SD_SECTOR_SIZE);
            return -1;
        }
    }

    if (apply_dummy_clk)
        drvl1_sdc30_dummyclk_en(device_id, 0);
    drvl1_sdc30_block_len_set(device_id, C_SD_SECTOR_SIZE);
    // tunning sd30 clock, cmd, data pins end

	return 0;
}

static INT32S drvl2_sd30_tunning_18V_phase(INT32U device_id, INT32U limit_speed)
{
    INT32S ret = 0;
    INT32U delay_10us = ((MCLK/1000000UL)*376UL/9600) + 1;
    INT8U apply_sd_in_clk_source = 1; // suggest 1 to tune with 2 input clock source

    //volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
    while (sd_card_speed[device_id] > sd30_clockrate_tunning_threshold) {

        ret = drvl2_sd30_tunning_18V_clock_cmd_data_phase(device_id, apply_sd_in_clk_source);

        if (sd30_tunning_clock_cmd_data_phase_not_allow_reduce_sd_clock[device_id])
            break;

        if (ret == 0) {
            if (apply_sd_in_clk_source) {
                // when tunning with 2 clock source, clock range 5 and cmd data range 3 no need to down sd clock rate
                if (((sd_clk_boundary[1] - sd_clk_boundary[0] + 1) >= 5) && \
                    ((sd_cmd_data_boundary[1] - sd_cmd_data_boundary[0] + 1) >= 5))
                    break;
            }
            else {
                // when tunning with just one clock source, clock range 3 or cmd data range 3 no need to down sd clock rate
                if (((sd_clk_boundary[1] - sd_clk_boundary[0] + 1) >= 3) || \
                    ((sd_cmd_data_boundary[1] - sd_cmd_data_boundary[0] + 1) >= 3))
                    break;
            }
        }

        INT32U sd_clock_rate, sd_souce_clock_rate, sd_clock_div;
        drvl1_sdc30_running_clock_rate_get(device_id, &sd_clock_rate, &sd_souce_clock_rate, &sd_clock_div);
        sd_clock_div++;
        limit_speed = sd_souce_clock_rate / sd_clock_div;
        if (limit_speed > sd30_clockrate_tunning_threshold) {
            if (drvl1_sdc30_clock_set(device_id, limit_speed))
                break;
            drv_tiny_wait(delay_10us);
            sd_card_speed[device_id] = limit_speed;
        }
        else {
            // smaller than sd30_clockrate_tunning_threshold then break out
            break;
        }
    }
    return ret;
}

static ALIGN64 INT8U sd_tunning_block_temp_buffer[512];

void drvl2_sd30_33V_set_tunning_block_sector_addr(INT32U device_id, INT32U sector_addr)
{
    if (device_id > 2)
        device_id = 1;
    sd_tunning_block_sector_addr[device_id] = sector_addr;
}

/**
 * @brief   Send SD3.0 64 bytes tuning pattern for SDR50 and SDR 104.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: command fail, -2: tuning fail.
*/
static INT32S drvl2_sd30_33V_tuning_cmd(INT32U device_id)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	INT32U response;
	INT32S ret = 0;
	INT32U dma_sd_status = 0;
    INT32U start_sector = sd_tunning_block_sector_addr[device_id];
    INT32U sector_count = 1;
    INT32U *buffer = (INT32U *)sd_tunning_block_temp_buffer;

    memset((INT8U *)(&sd_card_tuningblock[device_id][0]), 0, 64);
    //cache_invalid_range((unsigned int)&sd_card_tuningblock[device_id][0], 64);
    cache_invalid_range((unsigned int)sd_tunning_block_temp_buffer, 512);

    //drvl1_sdc30_module_reset();
     // Clear SD RX data register before read command is issued
    drvl1_sdc30_clear_rx_data_register(device_id);

 	dma_sd_status = 0;

	if ((sd_card_type[device_id] == C_MEDIA_SDHC_CARD)||(sd_card_type[device_id] == C_MEDIA_MMCHC_CARD))
	{
		if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD18, start_sector, 0, &response))
		{
			dma_sd_status |= sd->Status;
			drvl1_sdc30_clear_timeout_bit(device_id, 10);
		}
	}
	else
	{
		if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD18, start_sector<<9, 0, &response))
		{
			dma_sd_status |= sd->Status;
			drvl1_sdc30_clear_timeout_bit(device_id, 10);
		}
	}

	if (response == K_SD_RESPONSE_INIT_VALUE)
	{
        if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) != 0)
            return -1;
	}

	if (ret == 0)
	{
        if (drvl1_sdc30_command_complete_wait(device_id, C_SD_L2_MULTI_READ_CMD_COMPLETE_TIMEOUT))
        {
            dma_sd_status |= sd->Status;
            drvl1_sdc30_clear_timeout_bit(device_id, 10);
            DBG_PRINT("3.1 drvl1_sdc30_command_complete_wait error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, dma_sd_status, start_sector, sector_count);
            ret = -1;
        }

		if ((ret == 0) && drvl1_sdc_check_card_present(device_id))
		{
            drvl1_sdc30_set_read_start(start_sector, sector_count); // williamyeo-sd30-dmablksts-not-match

            ret = drvl1_sdc30_read_data_by_dma(device_id, buffer, sector_count, NULL);
			if (ret != 0)
			{
				dma_sd_status |= sd->Status;
				drvl1_sdc30_clear_timeout_bit(device_id, 10);
				//DBG_PRINT("3 drvl1_sdc30_read_data_by_dma error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, dma_sd_status, start_sector, sector_count);
				ret = -1;
			}
			else
			{
				if (drvl1_sdc30_data_complete_wait(device_id, C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT))
				{
					dma_sd_status |= sd->Status;
					drvl1_sdc30_clear_timeout_bit(device_id, 10);
					DBG_PRINT("3 read data_complete_wait fail[%d]. status=0x%08x start=%d, sector_count=%d\r\n", device_id, sd->Status, start_sector, sector_count);
					ret = -1;
				}

				if (drvl1_sdc30_data_crc_status_get(device_id))
				{
					DBG_PRINT("3 drvl2_sd_read crc error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
					ret = -1;
				}
			}
		}
	}

    if (drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
    {
        //ret = -1; // I think fail only happen when power is off, so no need to set error
    }

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
	{
        dma_sd_status |= sd->Status;
		drvl1_sdc30_clear_timeout_bit(device_id, 10);

        DBG_PRINT("3 sd read stop error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
		ret = -1;   // some non brand card will happen read stop command response crc error
                    // using 49.5M sd clock will always happen read data crc error
					// using 47.25M happen some data crc error, retry could pass, but alsmost always happen
                    // read stop command response crc error. Must down untill 41.25M to pass.
    }

	if ((dma_sd_status & (C_SDC_STATUS_TIMEOUT | C_SDC_STATUS_CONTROLLER_BUSY | C_SDC_STATUS_CARD_BUSY | C_SDC_STATUS_DATA_CRC_ERR | C_SDC_STATUS_RESP_CRC_ERR | C_SDC_STATUS_RESP_INDEX_ERR)) != 0)
	{
		drvl1_sdc30_clear_timeout_bit(device_id, 10); // clear again just for safe
        drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT); // stop sd controller again just for safe
	}

    if (ret == 0) {
        memcpy((&sd_card_tuningblock[device_id][0]), buffer, 64);
        if(strncmp((const char *)&sd_card_tuningblock[device_id][0], (const char *)&sd_tuning_pattern[0], 64) != 0)
        {
            return -2;
        }
        return 0;
    }
    else {
        return -1;
    }
}

static INT32S drvl2_sd30_tunning_33V_clock_cmd_data_phase(INT32U device_id, INT8U apply_sd_in_clk_source)
{
    INT8U apply_dummy_clk = 0;
    INT8U apply_center_clk = 1; // suggest turn on as 1, this option being tested

	// tunning sd30 clock, cmd, data pins start

	// 20250311, After confirm with Jason modify this value to > 50000000, old is >= 960000000
    // modify this value purpose is to doing clock tunning when over 50000000

    DBG_PRINT("33V sd30_clockrate_tunning_threshold %u, sd_support_1p8v %u\r\n", sd30_clockrate_tunning_threshold, sd_support_1p8v);
    {
        drvl1_sdc30_block_len_set(device_id, C_SD_SECTOR_SIZE);//(JC)resize block length for tuning
        INT8U retry_cnt = 0;
        INT8U tuning_idx;
        INT8U first_bound;
        INT8U second_bound;
        INT8U clk_do_count = (apply_sd_in_clk_source) ? 2 : 1;
        INT8U clk_idx;
        INT8U clk_boundary[2][2];
        INT8U clk_tune_result[2];
        INT8U clk_delay_tune_range[2];

        clk_delay_offset_result = 0;
        clk_delay_tune_result = 0;
        cmd_data_delay_tune_result = 0;

        drvl1_sdc30_sdclk_delay_set(device_id, TUNING_CTRL_BL);
        drvl1_sdc30_sdclk_delay_offset_set(device_id, 0);
        drvl1_sdc30_sdcmd_delay_set(device_id, TUNING_CTRL_BL);
        drvl1_sdc30_sddata_delay_set(device_id, TUNING_CTRL_BL);

        if (apply_dummy_clk) {
            drvl1_sdc30_dummyclk_en(device_id, 1);
            vTaskDelay(3);
        }

        clk_tune_result[0] = 0;
        clk_tune_result[1] = 0;
        for (clk_idx = 0; clk_idx < clk_do_count; clk_idx++) {
            drvl1_sdc30_in_clk_sel(device_id, clk_idx);
            clk_boundary[clk_idx][0] = 0;
            clk_boundary[clk_idx][1] = 0;
            first_bound = 0;
            second_bound = 0;
            tuning_idx = TUNING_CTRL_BL;
            while (tuning_idx < TUNING_CTRL_BH) {
                drvl1_sdc30_sdclk_delay_set(device_id, tuning_idx);
                drvl1_sdc30_sdclk_delay_offset_set(device_id, 0);
                clk_delay_offset_result = drvl1_sdc30_sdclk_delay_offset_result(device_id);
                if (drvl2_sd30_33V_tuning_cmd(device_id)==0) {
                    if (!first_bound) {
                        clk_boundary[clk_idx][0] = tuning_idx;
                        first_bound = 1;
                        //DBG_PRINT("33V sd30 clk phase first success %u, clk_idx=%u! pre_post=0x%x\r\n", clk_boundary[clk_idx][0], clk_idx, clk_delay_offset_result);
                    }
                }
                else if (first_bound) {
                    if(tuning_idx >= 1)
                        clk_boundary[clk_idx][1] = tuning_idx-1;
                    else
                        clk_boundary[clk_idx][1] = 0;
                    second_bound = 1;
                    //DBG_PRINT("33V sd30 clk phase end success %u, clk_idx=%u ! pre_post=0x%x\r\n", clk_boundary[clk_idx][1], clk_idx, clk_delay_offset_result);
                }
                if (second_bound)
                    break;
                tuning_idx++;
            } // end while

            if (first_bound) {
                if (!second_bound) {
                    if (tuning_idx >= 1)
                        clk_boundary[clk_idx][1] = tuning_idx-1;
                    else
                        clk_boundary[clk_idx][1] = 0;
                    if (tuning_idx != TUNING_CTRL_BH) // should not happen
                        DBG_PRINT("33V sd30 clk phase end success not found, clk_idx=%u, tuning_idx=%u\r\n", clk_idx, tuning_idx);
                }
                clk_tune_result[clk_idx] = 0; // clk phase success
            }
            else {
                clk_tune_result[clk_idx] = 1; // clk phase fail
                DBG_PRINT("33V sd30 clk phase not found, clk_idx=%u, tuning_idx=%u\r\n", clk_idx, tuning_idx);
            }
            DBG_PRINT("33V sd30 clk source %s phase success range [%u, %u] ! pre_post=0x%x\r\n", \
                        ((clk_idx == 0) ? "sd-host" : "io-pad"), \
                        clk_boundary[clk_idx][0], clk_boundary[clk_idx][1], clk_delay_offset_result);
        } // end for

        if (clk_do_count > 1) {
            clk_delay_tune_range[0] = clk_boundary[0][1] - clk_boundary[0][0] + 1;
            clk_delay_tune_range[1] = clk_boundary[1][1] - clk_boundary[1][0] + 1;
            if (clk_delay_tune_range[1] > clk_delay_tune_range[0]) {
                drvl1_sdc30_in_clk_sel(device_id, 1);
                clk_delay_tune_result = clk_tune_result[1];
                sd_clk_boundary[0] = clk_boundary[1][0];
                sd_clk_boundary[1] = clk_boundary[1][1];
                DBG_PRINT("33V sd30 clk source use io-pad range %u, sd-host range %u\r\n", clk_delay_tune_range[1], clk_delay_tune_range[0]);
            }
            else {
                drvl1_sdc30_in_clk_sel(device_id, 0);
                clk_delay_tune_result = clk_tune_result[0];
                sd_clk_boundary[0] = clk_boundary[0][0];
                sd_clk_boundary[1] = clk_boundary[0][1];
                DBG_PRINT("33V sd30 clk source use sd-host range %u, io-pad range %u\r\n", clk_delay_tune_range[0], clk_delay_tune_range[1]);
            }
        }
        else {
            clk_delay_tune_range[0] = clk_boundary[0][1] - clk_boundary[0][0] + 1;
            drvl1_sdc30_in_clk_sel(device_id, 0);
            clk_delay_tune_result = clk_tune_result[0];
            sd_clk_boundary[0] = clk_boundary[0][0];
            sd_clk_boundary[1] = clk_boundary[0][1];
            DBG_PRINT("33V sd30 clk source use sd-host range %u\r\n", clk_delay_tune_range[0]);
        }

        if (apply_center_clk) {
            drvl1_sdc30_sdclk_delay_set(device_id, ((sd_clk_boundary[1]-sd_clk_boundary[0]+1)/2) + sd_clk_boundary[0]);
            drvl1_sdc30_sdclk_delay_offset_set(device_id, 0);
        }
        else {
            drvl1_sdc30_sdclk_delay_set(device_id, TUNING_CTRL_BL);
            drvl1_sdc30_sdclk_delay_offset_set(device_id, 0);
        }
        drvl1_sdc30_sdcmd_delay_set(device_id, TUNING_CTRL_BL);
        drvl1_sdc30_sddata_delay_set(device_id, TUNING_CTRL_BL);
        clk_delay_offset_result = drvl1_sdc30_sdclk_delay_offset_result(device_id);
        DBG_PRINT("33V sd30 clk phase success range [%u, %u, %u] ! pre_post=0x%x\r\n", sd_clk_boundary[0], sd_clk_boundary[1], sd_clk_boundary[1] - sd_clk_boundary[0] + 1, clk_delay_offset_result);

        if (apply_dummy_clk) {
            drvl1_sdc30_dummyclk_en(device_id, 1);
            vTaskDelay(3);
        }

        sd_cmd_data_boundary[0] = 0;
        sd_cmd_data_boundary[1] = 0;
        first_bound = 0;
        second_bound = 0;
        tuning_idx = TUNING_CTRL_BL;
        while (tuning_idx < TUNING_CTRL_BH) {
            drvl1_sdc30_sdcmd_delay_set(device_id, tuning_idx);
            drvl1_sdc30_sddata_delay_set(device_id, tuning_idx);
            clk_delay_offset_result = drvl1_sdc30_sdclk_delay_offset_result(device_id);
            if (drvl2_sd30_33V_tuning_cmd(device_id)==0) {
                if (!first_bound) {
                    sd_cmd_data_boundary[0] = tuning_idx;
                    first_bound = 1;
                    //DBG_PRINT("33V sd30 cmd data phase first success %u! pre_post=0x%x\r\n", sd_cmd_data_boundary[0], clk_delay_offset_result);
                }
            }
            else if (first_bound) {
                if(tuning_idx >= 1)
                    sd_cmd_data_boundary[1] = tuning_idx-1;
                else
                    sd_cmd_data_boundary[1] = 0;
                second_bound = 1;
                //DBG_PRINT("33V sd30 cmd data phase end success %u! pre_post=0x%x\r\n", sd_cmd_data_boundary[1], clk_delay_offset_result);
            }
            if (second_bound)
                break;
            tuning_idx++;
        } // end while

        if (first_bound) {
            if (!second_bound) {
                if (tuning_idx >= 1)
                    sd_cmd_data_boundary[1] = tuning_idx-1;
                else
                    sd_cmd_data_boundary[1] = 0;
                if (tuning_idx != TUNING_CTRL_BH)
                    DBG_PRINT("33V sd30 cmd data phase end success not found, tuning_idx=%u\r\n", tuning_idx);
            }
            cmd_data_delay_tune_result = 0; // cmd data phase success
        }
        else {
            cmd_data_delay_tune_result = 1; // cmd data phase fail
            DBG_PRINT("33V sd30 cmd data phase not found, tuning_idx=%u\r\n", tuning_idx);
        }

        drvl1_sdc30_sdclk_delay_set(device_id, ((sd_clk_boundary[1]-sd_clk_boundary[0]+1)/2) + sd_clk_boundary[0]);
        drvl1_sdc30_sdclk_delay_offset_set(device_id, 0);
        drvl1_sdc30_sdcmd_delay_set(device_id, ((sd_cmd_data_boundary[1]-sd_cmd_data_boundary[0]+1)/2) + sd_cmd_data_boundary[0]);
        drvl1_sdc30_sddata_delay_set(device_id, ((sd_cmd_data_boundary[1]-sd_cmd_data_boundary[0]+ 1)/2) + sd_cmd_data_boundary[0]);
        clk_delay_offset_result = drvl1_sdc30_sdclk_delay_offset_result(device_id);
        DBG_PRINT("33V cmd data phase success range [%u, %u, %u] ! pre_post=0x%x\r\n", sd_cmd_data_boundary[0], sd_cmd_data_boundary[1], sd_cmd_data_boundary[1] - sd_cmd_data_boundary[0] + 1, clk_delay_offset_result);

        // print final tune timing
        drvl1_sdc30_tune_timing_print(device_id);

L_RETRY2:
        if (retry_cnt == 0) {
            if (apply_dummy_clk) {
                drvl1_sdc30_dummyclk_en(device_id, 1);
                vTaskDelay(3);
            }
        }

        cache_invalid_range((unsigned int)(&sd_card_tuningblock[device_id][0]), 64);
        if (drvl2_sd30_33V_tuning_cmd(device_id) == 0)
        {
            DBG_PRINT("33V =>0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\r\n", sd_card_tuningblock[device_id][0], sd_card_tuningblock[device_id][1], sd_card_tuningblock[device_id][2], sd_card_tuningblock[device_id][3], sd_card_tuningblock[device_id][4], sd_card_tuningblock[device_id][5]);
            if(strncmp((char const *)(&sd_card_tuningblock[device_id][0]), &sd_tuning_pattern[0], 64) == 0)
            {
                DBG_PRINT("33V cp succ2, Tuning clk passed!! retry_cnt=%u\r\n", retry_cnt);
            }
            else
            {
                if (device_id == 1)
                    drvl1_sdc30_stop_controller(device_id, 1);
                DBG_PRINT("33V Tuning clk failed1!! retry_cnt=%u\r\n", retry_cnt);
                if(retry_cnt>3)
                {
                    if (apply_dummy_clk)
                        drvl1_sdc30_dummyclk_en(device_id, 0);
                    drvl1_sdc30_block_len_set(device_id, C_SD_SECTOR_SIZE);
                    DBG_PRINT("33V Tuning step found but FAIL! retry_cnt=%u\r\n", retry_cnt);
                    return -1;
                }
                retry_cnt++;
                goto L_RETRY2;
            }
        }
        else
        {
            if (device_id == 1)
                drvl1_sdc30_stop_controller(device_id, 1);
            DBG_PRINT("33V Tuning clk failed2!! retry_cnt=%u\r\n", retry_cnt);
            if (apply_dummy_clk)
                drvl1_sdc30_dummyclk_en(device_id, 0);
            drvl1_sdc30_block_len_set(device_id, C_SD_SECTOR_SIZE);
            return -1;
        }
    }

    if (apply_dummy_clk)
        drvl1_sdc30_dummyclk_en(device_id, 0);
    drvl1_sdc30_block_len_set(device_id, C_SD_SECTOR_SIZE);
    // tunning sd30 clock, cmd, data pins end

	return 0;
}

INT32S drvl2_sd30_tunning_33V_phase(INT32U device_id, INT32U limit_speed)
{
    INT32S ret = 0;
    INT32U delay_10us = ((MCLK/1000000UL)*376UL/9600) + 1;
    INT8U apply_sd_in_clk_source = 1; // suggest 1 to tune with 2 input clock source

    //volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
    while (sd_card_speed[device_id] > sd30_clockrate_tunning_threshold) {

        ret = drvl2_sd30_tunning_33V_clock_cmd_data_phase(device_id, apply_sd_in_clk_source);

        if (sd30_tunning_clock_cmd_data_phase_not_allow_reduce_sd_clock[device_id])
            break;

        if (ret == 0) {
            if (apply_sd_in_clk_source) {
                // when tunning with 2 clock source, clock range 5 and cmd data range 3 no need to down sd clock rate
                if (((sd_clk_boundary[1] - sd_clk_boundary[0] + 1) >= 5) && \
                    ((sd_cmd_data_boundary[1] - sd_cmd_data_boundary[0] + 1) >= 5))
                    break;
            }
            else {
                // when tunning with just one clock source, clock range 3 or cmd data range 3 no need to down sd clock rate
                if (((sd_clk_boundary[1] - sd_clk_boundary[0] + 1) >= 3) || \
                    ((sd_cmd_data_boundary[1] - sd_cmd_data_boundary[0] + 1) >= 3))
                    break;
            }
        }

        INT32U sd_clock_rate, sd_souce_clock_rate, sd_clock_div;
        drvl1_sdc30_running_clock_rate_get(device_id, &sd_clock_rate, &sd_souce_clock_rate, &sd_clock_div);
        sd_clock_div++; // next divider
        limit_speed = sd_souce_clock_rate / sd_clock_div;
        if (limit_speed > sd30_clockrate_tunning_threshold) {
            if (drvl1_sdc30_clock_set(device_id, limit_speed))
                break;
            drv_tiny_wait(delay_10us);
            sd_card_speed[device_id] = limit_speed;
        }
        else {
            // smaller than sd30_clockrate_tunning_threshold then break out
            break;
        }
    }
    return ret;
}

/**
 * @brief   Send SD3.0 64 bytes tuning pattern for SDR50 and SDR 104.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: command fail, -2: tuning fail.
*/
static INT32S drvl2_sd20_33V_tuning_cmd(INT32U device_id)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	INT32U response;
	INT32S ret = 0;
	INT32U dma_sd_status = 0;
    INT32U start_sector = sd_tunning_block_sector_addr[device_id];
    INT32U sector_count = 1;
    INT32U *buffer = (INT32U *)sd_tunning_block_temp_buffer;

    memset((INT8U *)(&sd_card_tuningblock[device_id][0]), 0, 64);
    //cache_invalid_range((unsigned int)&sd_card_tuningblock[device_id][0], 64);
    cache_invalid_range((unsigned int)sd_tunning_block_temp_buffer, 512);

 	dma_sd_status = 0;

	if ((sd_card_type[device_id] == C_MEDIA_SDHC_CARD)||(sd_card_type[device_id] == C_MEDIA_MMCHC_CARD))
	{
		if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD18, start_sector, 0, &response))
		{
			dma_sd_status |= sd->Status;
			drvl1_sdc_clear_timeout_bit(device_id, 10);
		}
	}
	else
	{
		if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD18, start_sector<<9, 0, &response))
		{
			dma_sd_status |= sd->Status;
			drvl1_sdc_clear_timeout_bit(device_id, 10);
		}
	}

	if (response == K_SD_RESPONSE_INIT_VALUE)
	{
        if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) != 0)
            return -1;
	}

	if (ret == 0)
	{
        if (drvl1_sdc_command_complete_wait(device_id, C_SD_L2_MULTI_READ_CMD_COMPLETE_TIMEOUT))
        {
            dma_sd_status |= sd->Status;
            drvl1_sdc_clear_timeout_bit(device_id, 10);
            DBG_PRINT("3.1 drvl1_sdc_command_complete_wait error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, dma_sd_status, start_sector, sector_count);
            ret = -1;
        }

		if ((ret == 0) && drvl1_sdc_check_card_present(device_id))
		{
            ret = drvl1_sdc_read_data_by_dma(device_id, buffer, sector_count, NULL);
			if (ret != 0)
			{
				dma_sd_status |= sd->Status;
				drvl1_sdc_clear_timeout_bit(device_id, 10);
				//DBG_PRINT("3 drvl1_sdc_read_data_by_dma error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, dma_sd_status, start_sector, sector_count);
				ret = -1;
			}
			else
			{
				if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT))
				{
					dma_sd_status |= sd->Status;
					drvl1_sdc_clear_timeout_bit(device_id, 10);
					DBG_PRINT("3 read data_complete_wait fail[%d]. status=0x%08x start=%d, sector_count=%d\r\n", device_id, sd->Status, start_sector, sector_count);
					ret = -1;
				}

				if (drvl1_sdc_data_crc_status_get(device_id))
				{
					DBG_PRINT("3 drvl2_sd_read crc error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
					ret = -1;
				}
			}
		}
	}

    if (drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
    {
        //ret = -1; // I think fail only happen when power is off, so no need to set error
    }

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
	{
        dma_sd_status |= sd->Status;
		drvl1_sdc_clear_timeout_bit(device_id, 10);

        DBG_PRINT("3 sd read stop error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
		ret = -1;   // some non brand card will happen read stop command response crc error
                    // using 49.5M sd clock will always happen read data crc error
					// using 47.25M happen some data crc error, retry could pass, but alsmost always happen
                    // read stop command response crc error. Must down untill 41.25M to pass.
    }

	if ((dma_sd_status & (C_SDC_STATUS_TIMEOUT | C_SDC_STATUS_CONTROLLER_BUSY | C_SDC_STATUS_CARD_BUSY | C_SDC_STATUS_DATA_CRC_ERR | C_SDC_STATUS_RESP_CRC_ERR | C_SDC_STATUS_RESP_INDEX_ERR)) != 0)
	{
		drvl1_sdc_clear_timeout_bit(device_id, 10); // clear again just for safe
        drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT); // stop sd controller again just for safe
	}

    if (ret == 0) {
        memcpy((&sd_card_tuningblock[device_id][0]), buffer, 64);
        if(strncmp((const char *)&sd_card_tuningblock[device_id][0], (const char *)&sd_tuning_pattern[0], 64) != 0)
        {
            return -2;
        }
        return 0;
    }
    else {
        return -1;
    }
}

static INT8U sd_cop_list[SDC_TOTAL][8];

INT32S drvl2_sd20_tunning_33V_COP(INT32U device_id)
{
    INT32S ret = 0;
    INT8U k, total_step;
    INT8U ok_step = 0;

    total_step = drvl1_sdc_COP_total_step_get(device_id);
    for (k = 0; k <= total_step; k++) {
        drvl1_sdc_COP_set(device_id, k);
        if (drvl2_sd20_33V_tuning_cmd(device_id)==0) {
            sd_cop_list[device_id][ok_step] = k;
            ok_step++;
        }
        else
            DBG_PRINT("fail COP is %u\r\n", drvl1_sdc_COP_get(device_id));
    }

    if (ok_step) {
        drvl1_sdc_COP_set(device_id, sd_cop_list[device_id][ok_step>>1]);

        DBG_PRINT("tunned COP is %u, total ok is %u\r\n", drvl1_sdc_COP_get(device_id), ok_step);
        ret = 0;
    }
    else {
        drvl1_sdc_COP_set(device_id, 0);
        DBG_PRINT("tunned COP fail, reset to default COP %u\r\n", drvl1_sdc_COP_get(device_id));
        ret = -1;
    }

    return ret;
}

/**
 * @brief   Get sd card information and set buswidth and clock rate.
 * @param   device_id[in]: device id.
 * @param   limit_speed[in]: limit sd clock.
 * @return 	0: success, -1 fail.
*/
INT32S drvl2_sd_bus_clock_set_sdcard(INT32U device_id, INT32U limit_speed)
{
	INT32U response = 0;
    INT32S ret;
    INT32U delay_10us = ((MCLK/1000000UL)*376UL/9600) + 1;

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD7, sd_card_rca[device_id], C_RESP_R1B, &response)) {
		DBG_PRINT("SDERR 20\r\n");
		return -1;
	}
	sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;

    if(device_id == 0)
        drvl1_sdc_clock_set(device_id, 20000000);
    else
        drvl1_sdc30_clock_set(device_id, (SD3MCLK / (SD3MCLK/25000000)));
    drv_tiny_wait(delay_10us);

	#if 0 && (SD_FORCE_ERASE_ALL == 1)
	if ((response & (1<<C_SD_CARD_STATUS_CARD_IS_LOCKED_BIT)) != 0) // check CARD_IS_LOCKED bit
	{
		INT32S erase_all_ret;

		erase_all_ret = drvl2_sd_force_erase_all(device_id);
	}
	#endif

	if (!sd_scr_done[device_id]) {
        ret = drvl2_sdcard_read_scr(device_id);
        if (ret != 0) {
            return ret;
        }
        sd_scr_done[device_id] = 1;
    }

    ret = drvl2_sdcard_set_bus_width(device_id);
    if (ret != 0) {
        return ret;
    }

    if (!sd_support_1p8v) {
        if (limit_speed > 50000000)
            limit_speed = 50000000;
    }

    ret = drvl2_sdcard_set_bus_speed(device_id, limit_speed);
    if (ret != 0) {
        return ret;
    }

    ret = drvl2_sdcard_read_csd(device_id, limit_speed);
    if (ret != 0) {
        return ret;
    }

    sd30_clockrate_tunning_threshold = 25000000;
    if((device_id == 1) && (sd_support_1p8v)) {
        if (sd_tunning_block_sector_addr[device_id] != 0) {
            ret = drvl2_sd30_tunning_33V_phase(device_id, limit_speed);
        }
        else
            ret = drvl2_sd30_tunning_18V_phase(device_id, limit_speed);
    }
    else if(device_id == 1) {
        // select SDC_IN_CLK_SEL_SD_HOST, set delay step to center for 3.3V
        if (sd_tunning_block_sector_addr[device_id] != 0) {
            ret = drvl2_sd30_tunning_33V_phase(device_id, limit_speed);
            if (ret != 0) {
                drvl1_sdc30_all_delay_set(device_id, 0, (TUNING_CTRL_BH - TUNING_CTRL_BL)/2);
                ret = 0;
            }
        }
        else
            drvl1_sdc30_all_delay_set(device_id, 0, (TUNING_CTRL_BH - TUNING_CTRL_BL)/2);
    }
    else if(device_id == 0) {
        if (sd_tunning_block_sector_addr[device_id] != 0) {
            drvl2_sd20_tunning_33V_COP(device_id);
        }
        else {

            DBG_PRINT("current COP is %u\r\n", drvl1_sdc_COP_get(device_id));
        }

        INT32U sd_clock_rate, sd_souce_clock_rate, sd_clock_div;

        drvl1_sdc_running_clock_rate_get(device_id, &sd_clock_rate, &sd_souce_clock_rate, &sd_clock_div);
        DBG_PRINT("sd[%u] clock rate=%u, root rate=%u divider=%u cop=%u\r\n", device_id, sd_clock_rate, sd_souce_clock_rate, sd_clock_div, drvl1_sdc_COP_get(device_id));

    }

	return ret;
}

/**
 * @brief   Get mmc card information and set buswidth and clock rate.
 * @param   device_id[in]: device id.
 * @param   limit_speed[in]: limit sd clock.
 * @return 	0: success, -1 fail.
*/
static INT32S drvl2_sd_bus_clock_set_mmc(INT32U device_id, INT32U limit_speed)
{
	INT32U response;
	INT32U c_size, mult, block_len;
	INT32U max_speed;
	INT32U SPEC_VAR;
    INT32U delay_10us = ((MCLK/1000000UL)*376UL/9600) + 1;

    sd_card_is_emmc[device_id] = 0;

	// Send CMD9 and read CSD Register
	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD9, sd_card_rca[device_id], C_RESP_R2, &sd_card_csd[device_id][0])) {
		return -1;
	}

	#if (SD_DEBUG_CSD_INFO == 1)
	INT32U sd_card_csd_swap[4];
	{
		INT8U csd_ver;
		INT32U kk;

		for (kk = 0; kk < 4; kk++)
			sd_card_csd_swap[kk] = swapword(sd_card_csd[device_id][kk]);

		csd_ver = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 127, 126);
		DBG_PRINT("MMC CSD info: version[%x]\r\n", csd_ver);
		sd_dump_buffer((INT8U *)(&sd_card_csd_swap[0]), 16);

	}
	#endif

	SPEC_VAR = (sd_card_csd[device_id][0]&0x3C000000)>>26;
	max_speed = drvl2_sd_despeed(device_id);
	if( max_speed == 0 )
		return -1;

	// Change to transfer state
	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD7, sd_card_rca[device_id], C_RESP_R1B, &response)) {
		// sdc speed back to 400k
		if(device_id == 0){
            drvl1_sdc_clock_set(device_id, 400000);
        }else{
            drvl1_sdc30_clock_set(device_id, 400000);
        }
        drv_tiny_wait(delay_10us);
		return -1;
	}
	sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;

    if(device_id == 0)
        drvl1_sdc_clock_set(device_id, 20000000);
    else
        drvl1_sdc30_clock_set(device_id, 20000000);
    drv_tiny_wait(delay_10us);

	// ----- Only spec. 4.0 or latter can support SWITCH function -----
	if( SPEC_VAR >= 4 )
	{
        if (sd_card_use_one_bit_mode[device_id])
			DBG_PRINT("MMC USE 1 BIT ONLY!\r\n");
        else
		{
			//----- Setup MMC bus width 4 bit -----
			if(drvl2_sd_bcr_ac_command_set(device_id, C_MMC_CMD6, 0x03b70100, C_RESP_R1B, &response)){
				return -1;
			}
			if((response&0x08)==0)
			{
				sd_card_bus_width[device_id] = 4;
				if(device_id == 0){
                    drvl1_sdc_bus_width_set(device_id, 4);			// Set bus width to 4 bits
                }else{
                    drvl1_sdc30_bus_width_set(device_id, 4);			// Set bus width to 4 bits
                }
				DBG_PRINT("MMC use 4 bit\r\n");
			}
	    }
	    //----- Setup MMC to high speed mode -----
	    if( limit_speed>25000000 )
	    {
			if( drvl2_sd_bcr_ac_command_set(device_id, C_MMC_CMD6, 0x03b90100, C_RESP_R1B, &response)){
				return -1;
			}
			if((response&0x08)==0)
			{
				max_speed = 52000000;
                DBG_PRINT("MMC max speed is 52M\r\n");
		    }
		}
	}

	if (max_speed > limit_speed) {
        // fix MMC max speed could not use 52M
        if (limit_speed != 50000000 || max_speed != 52000000)
            max_speed = limit_speed;
  	}

	sd_card_speed[device_id] = max_speed;
	if(device_id == 0){
        if (drvl1_sdc_clock_set(device_id, max_speed)) {
            return -1;
        }
    }else{
        if (drvl1_sdc30_clock_set(device_id, max_speed)) {
            return -1;
        }
    }
    drv_tiny_wait(delay_10us);

	if(sd_card_type[device_id] == C_MEDIA_MMCHC_CARD)
	{
        volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
		static ALIGN64 INT32U mmc_buf[128]={0};
        INT8U *emmc_card_ext_csd = (INT8U *)&mmc_buf[0];

		if (drvl2_sd_adtc_command_set(device_id, C_EMMC_CMD8/*C_SD_CMD8|C_SDC_CMD_WITH_DATA*/, 0, 128, mmc_buf)){
			DBG_PRINT("C_EMMC_CMD8 fail\r\n");
			return -1;
		}
		sd_card_total_sector[device_id] = mmc_buf[53];
		DBG_PRINT("MMCHC_CARD. total_sector=%u, RST_n=0x%x\r\n", sd_card_total_sector[device_id], emmc_card_ext_csd[162]);

        if (emmc_enable_RST_n[device_id] && (emmc_card_ext_csd[162] == 0)) {
            if(drvl2_sd_bcr_ac_command_set(device_id, C_MMC_CMD6, 0x03A20100, C_RESP_R1B, &response)){
                DBG_PRINT("EMMC Program RST_n fail\r\n");
                return -1;
            }
            if (drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT))
            {
                DBG_PRINT("EMMC Program EXT_CSD byte. drvl1_sdc_stop_controller err. status=0x%08x\r\n", sd->Status);
            }

            if (drvl2_sd_adtc_command_set(device_id, C_EMMC_CMD8, 0, 128, (INT32U *)(&emmc_card_ext_csd[0]))) {
                if (drvl2_sd_adtc_command_set(device_id, C_EMMC_CMD8, 0, 128, (INT32U *)(&emmc_card_ext_csd[0]))) {
                    DBG_PRINT("2 C_EMMC_CMD8 fail\r\n");
                    return -1;
                }
            }
            if (emmc_card_ext_csd[162] != 1) {
                DBG_PRINT("EMMC Verify RST_n byte fail\r\n", emmc_card_ext_csd[162]);
            }
        }
	}
	else {
		block_len = (sd_card_csd[device_id][1] >> 16) & 0xF;
		c_size = ((sd_card_csd[device_id][1] & 0x3FF) << 2) | ((sd_card_csd[device_id][2] >> 30) & 0x3);
		mult = (sd_card_csd[device_id][2] >> 15) & 0x7;
		sd_card_total_sector[device_id] = (c_size + 1) << (mult + 2 + block_len - 9);
		DBG_PRINT("MMC_CARD. c_size=%d total_sector=%u block_len=%d mult=%d\r\n", c_size, sd_card_total_sector[device_id], block_len, mult);
		#if (SD_DEBUG_CSD_INFO == 1)
		{
			INT32U my_c_size;
			INT32U my_block_len;
			INT32U mult;

			my_c_size = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 73, 62);
			my_block_len = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 83, 80);
			mult = sd_wide_width_data_get_bits((INT8U *)(&sd_card_csd_swap[0]), 128, 49, 47);
			DBG_PRINT("MMC my_c_size=%u my_block_len=%u mult=%u\r\n", my_c_size, my_block_len, mult);
		}
		#endif
	}
	sd_card_is_emmc[device_id] = 1;

	return 0;
}

#if 0
static INT32S drvl2_sd_recover_bus_clock_set_sdcard(INT32U device_id, INT32U limit_speed)
{
	INT32U response;
	INT32U c_size, mult, block_len;
	INT32U max_speed;
	static ALIGN64 INT32U switch_function_data[16];		// 512 bits

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD7, sd_card_rca[device_id], C_RESP_R1B, &response)) {
		DBG_PRINT("SDERR 20\r\n");
		return -1;
	}

	// Check limit clock and SD spec version
	if( (limit_speed>25000000) && (sd_card_scr[device_id][0]&0x0F)>1 )
	{
		// Set block length to 64 bytes so that we can read status data of CMD6
		drvl1_sdc_block_len_set(device_id, 64);

		// Send ACMD6 to set bus width to 1 bit mode
		if (sd_card_bus_width[device_id] == 4) {
			drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD55, sd_card_rca[device_id], C_RESP_R1, &response);
			drvl2_sd_bcr_ac_command_set(device_id, C_SD_ACMD6, 0x0, C_RESP_R1, &response);
			drvl1_sdc_bus_width_set(device_id, 1);
		}

		if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x00FFFF01, 16, &switch_function_data[0]) == 0) {
			// Check whether card supports high-speed mode
			if ((switch_function_data[3] & 0x00000300) == 0x00000300) {
				drvl2_sd_adtc_command_set(device_id, C_SD_CMD6, 0x80FFFF01, 16, &switch_function_data[0]);
			}
		}
		// Restore sector length
		drvl1_sdc_block_len_set(device_id, C_SD_SECTOR_SIZE);
	}

	if (sd_card_scr[device_id][0] & 0x00000400) {		// Check whether this SD card supports 4-bits bus width
		if (sd_card_use_one_bit_mode[device_id])
			DBG_PRINT("SD USE 1 BIT ONLY!\r\n");
		else
		{
			sd_card_bus_width[device_id] = 4;

			// Send ACMD6 to set the card to 4-bit bus width
			if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD55, sd_card_rca[device_id], C_RESP_R1, &response) ||
				drvl2_sd_bcr_ac_command_set(device_id, C_SD_ACMD6, 0x2, C_RESP_R1, &response)) {
				DBG_PRINT("SDERR 24\r\n");
				return -1;
			}

			drvl1_sdc_bus_width_set(device_id, 4);			// Set bus width to 4 bits
		}
	}

	if (drvl1_sdc_clock_set(device_id, sd_card_speed[device_id])) {
		DBG_PRINT("SDERR 27\r\n");
		return -1;
	}

	sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;

	return 0;
}

INT32S drvl2_sd_recover_bus_clock_set(INT32U device_id, INT32U limit_speed)
{
	INT32U response;
	// Make sure that card is ready and in standby state
	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response)) {
		DBG_PRINT("SDERR 10\r\n");
		return -1;
	}
	response &= 0x1E00;

	if (response!=0x0600 && response!=0x0800) {		// 0x0600 is standby state, 0x800 is transfer state
		DBG_PRINT("SDERR 11\r\n");
		return -1;
	}

	if (response ==0x0800 ) {
		// Switch from transfer state to standby state
		DBG_PRINT("SD standy\r\n");
		drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD7_NO_RESONSE, 0x0, C_RESP_R0, &response);
	}

	if (sd_card_type[device_id]==C_MEDIA_SDHC_CARD || sd_card_type[device_id]==C_MEDIA_STANDARD_SD_CARD)
		return 	drvl2_sd_recover_bus_clock_set_sdcard(device_id, limit_speed);
	else
		return drvl2_sd_bus_clock_set_mmc(device_id, limit_speed);
}
#endif

/**
 * @brief   Make sure sd card is ready and in standby state then get sd card information and set buswidth and clock rate.
 * @param   device_id[in]: device id.
 * @param   limit_speed[in]: limit sd clock.
 * @return 	0: success, -1 fail.
*/
INT32S drvl2_sd_bus_clock_set(INT32U device_id, INT32U limit_speed)
{
	INT32U response;

	// Make sure that card is ready and in standby state
	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response)) {
		DBG_PRINT("SDERR 10\r\n");
		return -1;
	}
	response &= 0x1E00;

	if (response!=0x0600 && response!=0x0800) {		// 0x0600 is standby state, 0x800 is transfer state
		DBG_PRINT("SDERR 11\r\n");
		return -1;
	}

	if (response == 0x0800 ) {
		// Switch from transfer state to standby state
		DBG_PRINT("SD standy\r\n");
		if(drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD7_NO_RESONSE, 0x0, C_RESP_R0, &response)){
            DBG_PRINT("SDERR 12\r\n");
            return -1;
		}
	}

	if (sd_card_type[device_id]==C_MEDIA_SDHC_CARD || sd_card_type[device_id]==C_MEDIA_STANDARD_SD_CARD)
		return 	drvl2_sd_bus_clock_set_sdcard(device_id, limit_speed);
	else
		return drvl2_sd_bus_clock_set_mmc(device_id, limit_speed);
}

/**
 * @brief   Integrates SD card information to sd_info structure.
 * @param   device_id[in]: device id.
 * @param   *sd_info[out]: sd information.
 * @return 	none.
*/
void drvl2_sd_card_info_get(INT32U device_id, SD_CARD_INFO_STRUCT *sd_info)
{
	INT8U i;

	sd_info->rca = (sd_card_rca[device_id] >> 16) & 0xFFFF;
	for (i=0; i<4; i++) {
		sd_info->csd[i] = sd_card_csd[device_id][i];
	}
	for (i=0; i<2; i++) {
		sd_info->scr[i] = sd_card_scr[device_id][i];
	}
	for (i=0; i<4; i++) {
		sd_info->cid[i] = sd_card_cid[device_id][i];
	}
	sd_info->ocr = sd_card_ocr[device_id];
}

/**
 * @brief   Get SD card write protect status.
 * @param   device_id[in]: device id.
 * @return 	write protect status.
*/
INT8U drvl2_sd_card_protect_get(INT32U device_id)
{
	return sd_card_protect[device_id];
}

/**
 * @brief   Set SD card write protect status.
 * @param   device_id[in]: device id.
 * @param   value[in]: write protect status to be set.
 * @return 	none.
*/
void drvl2_sd_card_protect_set(INT32U device_id, INT8U value)
{
	sd_card_protect[device_id] = value;
}

/**
 * @brief   Get SD card total sector number.
 * @param   device_id[in]: device id.
 * @return 	sd card total sector number.
*/
INT32U drvl2_sd_sector_number_get(INT32U device_id)
{
	return sd_card_total_sector[device_id];
}

/**
 * @brief   card busy status till timeout happened.
 * @param   device_id[in]: device id.
 * @param   timeout[in]: timeout value.
 * @return 	sd card total sector number.
*/
static INT32S drvl2_sd_card_busy_wait(INT32U device_id, INT32U timeout)
{
	#if _OPERATING_SYSTEM == _OS_FREERTOS
	TimeOut_t xTimeOut;
    TickType_t xRemainTicks = pdMS_TO_TICKS(timeout);
	#else
	INT32U timer;
	#endif
	#if _OPERATING_SYSTEM == _OS_NONE
	volatile INT32U timer1, timer2;
	#endif

	// Wait operation complete and back to transfer state
	#if (defined _DRV_L1_SW_TIMER) && (_DRV_L1_SW_TIMER == 1)
	timer = (INT32U) sw_timer_get_counter_L();
	#elif _OPERATING_SYSTEM != _OS_NONE
		#if _OPERATING_SYSTEM == _OS_UCOS2
		timer = OSTimeGet();
		#elif _OPERATING_SYSTEM == _OS_FREERTOS
		vTaskSetTimeOutState(&xTimeOut);
		#endif
	#else
	timer1 = tiny_counter_get();
  	timer = 0;
	#endif
	do {
        if(device_id == 0){
            drvl1_sdc_card_init_74_cycles(device_id);
        }else{
            drvl1_sdc30_card_init_74_cycles(device_id);
        }
        if(device_id == 0){
            if (drvl1_sdc_card_busy_wait(device_id, 0)) {
                #if (defined _DRV_L1_SW_TIMER) && (_DRV_L1_SW_TIMER == 1)
                if ((sw_timer_get_counter_L()-timer) > (timeout)) {
                #elif _OPERATING_SYSTEM != _OS_NONE
                    #if _OPERATING_SYSTEM == _OS_UCOS2
                    if ((OSTimeGet()-timer) > (timeout/10)) {
                    #elif _OPERATING_SYSTEM == _OS_FREERTOS
                    if (xTaskCheckForTimeOut(&xTimeOut, &xRemainTicks) == pdTRUE) {
                    #endif
                #else
                timer2 = tiny_counter_get();
                if (timer1 != timer2)
                {
                    timer1 = timer2;
                    timer++;
                }
                if (timer > timeout) {
                #endif
                    return -1;
                }
            } else {
                break;
            }
        }else{
            if (drvl1_sdc30_card_busy_wait(device_id, 0)) {
                #if (defined _DRV_L1_SW_TIMER) && (_DRV_L1_SW_TIMER == 1)
                if ((sw_timer_get_counter_L()-timer) > (timeout)) {
                #elif _OPERATING_SYSTEM != _OS_NONE
                    #if _OPERATING_SYSTEM == _OS_UCOS2
                    if ((OSTimeGet()-timer) > (timeout/10)) {
                    #elif _OPERATING_SYSTEM == _OS_FREERTOS
                    if (xTaskCheckForTimeOut(&xTimeOut, &xRemainTicks) == pdTRUE) {
                    #endif
                #else
                timer2 = tiny_counter_get();
                if (timer1 != timer2)
                {
                    timer1 = timer2;
                    timer++;
                }
                if (timer > timeout) {
                #endif
                    return -1;
                }
            } else {
                break;
            }
        }
	} while (1) ;

	return 0;
}


static INT8U tuning_pattern_mode[SDC_TOTAL];

// internal used function, don't release to customer
INT32S drvl2_sd_tuning_pattern_locate(INT32U device_id, INT8U mode)
{
    if (device_id >= SDC_TOTAL)
        return -1;

    tuning_pattern_mode[device_id] = mode;

    return 0;
}

// internal used function, don't release to customer
INT32U drvl2_sd_tuning_pattern_sector_addr_get(INT32U device_id)
{
    INT32U addr;

    if (device_id >= SDC_TOTAL)
        return 0;

    addr = sd_tunning_pattern_sector_addr[device_id];
    sd_tunning_pattern_sector_addr[device_id] = 0;

    return addr;
}

// internal used function, don't release to customer
INT32S drvl2_sd_tuning_pattern_sector_addr_set(INT32U device_id, INT32U addr)
{
    if (device_id >= SDC_TOTAL)
        return -1;

    sd_tunning_block_sector_addr[device_id] = addr;

    return 0;
}

// internal used function, don't release to customer
void drvl2_sd_reset_card_state(INT32U device_id)
{
    if (device_id >= SDC_TOTAL)
        device_id = SDC_TOTAL - 1;

    sd_card_state[device_id] = SD_CARD_STATE_INACTIVE;

    return;
}

#define C_PATCH_1_SECTOR_READ_ISSUE     0 // for DEBUG

#if C_PATCH_1_SECTOR_READ_ISSUE == 1
static ALIGN64 INT8U buffer_1k[1024];
#endif

#if (SDC_FS_READ_PATH_USE_START_STOP == 1)
/**
 * @brief   SD read function.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: start sector address.
 * @param   *buffer[out]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_read(INT32U device_id, INT32U start_sector, INT32U *buffer, INT32U sector_count)
{
    INT32S ret;
    #if C_PATCH_1_SECTOR_READ_ISSUE == 1
    INT32U *save_buffer;
    INT32U save_sector_count;
    #endif

	#if (SD_DEBUG_ENFORCE_STOP > 0)
		sd_endforce_stop_type[device_id] = 0;
	#endif

    //DBG_PRINT("read start:start_sector=%u, sector_count=%u\r\n", start_sector, sector_count);

    #if C_PATCH_1_SECTOR_READ_ISSUE == 1
    save_sector_count = sector_count;
    save_buffer = buffer;
    if (save_sector_count <= ((sizeof(buffer_1k) >> 9) - 1)) {
        buffer = (INT32U *)buffer_1k;
        cache_invalid_range((unsigned int)buffer, sizeof(buffer_1k));
        sector_count = sizeof(buffer_1k) >> 9;
    }
    #endif

    ret = drvl2_sd_read_start(device_id, start_sector, sector_count);
    if (ret != 0)
        goto L_SD_L2_READ_RET;

    ret = drvl2_sd_read_sector(device_id, buffer, sector_count, SDC_FS_READ_PATH_USE_DMA_MSG);

    #if C_PATCH_1_SECTOR_READ_ISSUE == 1
    if (save_sector_count <= ((sizeof(buffer_1k) >> 9) - 1)) {
        memcpy(save_buffer, buffer, save_sector_count << 9);
        cache_invalid_range((unsigned int)save_buffer, save_sector_count << 9);
    }
    #endif

    if (tuning_pattern_mode[device_id]) {
        if (strncmp(sd_tuning_pattern, (const char *)buffer, sizeof(sd_tuning_pattern)) == 0) {
            sd_tunning_pattern_sector_addr[device_id] = start_sector;
            cache_invalid_range((unsigned int)buffer, 512);
        }
    }

    if (ret != 0)
        goto L_SD_L2_READ_RET;

    ret = drvl2_sd_read_stop(device_id);

	L_SD_L2_READ_RET:

	#if (SD_DEBUG_ENFORCE_STOP > 1)
		if (sd_endforce_stop_type[device_id] == 0)
			DBG_PRINT("\r\n");
		else
			DBG_PRINT(" force=%u\r\n", sd_endforce_stop_type[device_id]);
	#endif

    return ret;
}

#else

#if 0
INT32S drvl2_sd_read(INT32U device_id, INT32U start_sector, INT32U *buffer, INT32U sector_count)
{
	INT32S ret;

	drvl2_sd_sem_lock(device_id);
	DBG_PRINT("read start:start_sector=%u, sector_count=%u\r\n", start_sector, sector_count);
	ret = drvl2_sd_read_retry(device_id, start_sector, buffer, sector_count);
	DBG_PRINT("read end:start_sector=%u, sector_count=%u\r\n", start_sector, sector_count);
	drvl2_sd_sem_unlock(device_id);

	return ret;
}
#else

INT32S drvl2_sd_read(INT32U device_id, INT32U start_sector, INT32U *buffer, INT32U sector_count)
{
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	INT32U response;
	INT32S ret;
	INT32U dma_sd_status = 0;

	drvl2_sd_sem_lock(device_id);

	//DBG_PRINT("read start:start_sector=%u, sector_count=%u\r\n", start_sector, sector_count);

    drvl1_sdc_set_access_kind(device_id, C_SD_L2_ACCESS_READ);

    if (sd_card_state[device_id] != SD_CARD_STATE_TRANSFER)
	{
        drvl2_sd_sem_unlock(device_id);
		return -1;
	}

	ret = 0;

	drvl2_sd_set_speed_bus(device_id);
	if (drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_BUSY_TIMEOUT))
	{
        drvl2_sd_sem_unlock(device_id);
        return -1;
	}
#if 0
    if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response)==0)
	{
		// Check whether card data ready
		if (!(response & 0x100))
		{
			drvl2_sd_sem_unlock(device_id);
			return -1;
		}
	}
#endif
    if(device_id == 0){
        // Clear SD RX data register before read command is issued
        drvl1_sdc_clear_rx_data_register(device_id);
    }else{
        // Clear SD RX data register before read command is issued
        drvl1_sdc30_clear_rx_data_register(device_id);
    }

	if (0 && sector_count == 1)
	{
		if ((sd_card_type[device_id] == C_MEDIA_SDHC_CARD)||(sd_card_type[device_id] == C_MEDIA_MMCHC_CARD))
		{
			if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD17, start_sector, 0, &response))
			{
				if (drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
                {
                    ret = -1;
                }
				if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
				{
                    ret = -1;
                }
                sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
				drvl2_sd_sem_unlock(device_id);
				return -1;
			}
		}
		else
		{
			if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD17, start_sector<<9, 0, &response))
			{
				if (drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
                {
                    ret = -1;
                }
				if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
				{
                    ret = -1;
                }
                sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
				drvl2_sd_sem_unlock(device_id);
				return -1;
			}
		}

		dma_sd_status = 0;
		if (drvl1_sdc_command_complete_wait(device_id, C_SD_L2_SINGLE_READ_CMD_COMPLETE_TIMEOUT))
		{
			dma_sd_status |= sd->Status;
			drvl1_sdc_clear_timeout_bit(device_id, 10);
            DBG_PRINT("1 drvl1_sdc_command_complete_wait error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, dma_sd_status, start_sector, sector_count);
			ret = -1;
			drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
			drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
		}

		if ((ret == 0) && (drvl1_sdc_check_card_present(device_id)))
		{
			sd_card_state[device_id] = SD_CARD_STATE_SENDING_DATA;

			ret = drvl1_sdc_read_data_by_dma(device_id, buffer, sector_count, NULL);
			if (ret != 0)
			{
				dma_sd_status |= sd->Status;
				drvl1_sdc_clear_timeout_bit(device_id, 10);
				DBG_PRINT("1 drvl1_sdc_read_data_by_dma error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, dma_sd_status, start_sector, sector_count);
				ret = -1;

				if (drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
				{
					ret = -1;
				}

				if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
				{
					dma_sd_status |= sd->Status;
					DBG_PRINT("1 sd read stop error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
					ret = -1;
				}
			}
			else
			{
				if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_SINGLE_READ_DATA_COMPLETE_TIMEOUT))
				{
					dma_sd_status |= sd->Status;
					DBG_PRINT("1 read data_complete_wait fail[%d]. status=0x%08x start=%d, sector_count=%d\r\n", device_id, sd->Status, start_sector, sector_count);
					ret = -1;
				}

				if (drvl1_sdc_data_crc_status_get(device_id))
				{
					ret = -1;
					DBG_PRINT("1 drvl2_sd_read crc error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
					drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
				}
			}
		}

	}
	else
	{
		if ((sd_card_type[device_id] == C_MEDIA_SDHC_CARD)||(sd_card_type[device_id] == C_MEDIA_MMCHC_CARD))
		{
			if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD18, start_sector, 0, &response))
			{
				if (drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
                {
                    ret = -1;
                }
				if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
				{
                    ret = -1;
                }
                sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
				drvl2_sd_sem_unlock(device_id);
				return -1;
			}
		}
		else
		{
			if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD18, start_sector<<9, 0, &response))
			{
				if (drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
                {
                    ret = -1;
                }
				if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
				{
                    ret = -1;
                }
                sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
				drvl2_sd_sem_unlock(device_id);
				return -1;
			}
		}

		dma_sd_status = 0;

		if (drvl1_sdc_command_complete_wait(device_id, C_SD_L2_MULTI_READ_CMD_COMPLETE_TIMEOUT))
		{
			dma_sd_status = sd->Status;
			drvl1_sdc_clear_timeout_bit(device_id, 10);
            DBG_PRINT("2 drvl1_sdc_command_complete_wait error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, dma_sd_status, start_sector, sector_count);
			ret = -1;
		}

		if ((ret == 0) && (drvl1_sdc_check_card_present(device_id)))
		{
			sd_card_state[device_id] = SD_CARD_STATE_SENDING_DATA;

            ret = drvl1_sdc_read_data_by_dma(device_id, buffer, sector_count, NULL);
			if (ret != 0)
			{
				dma_sd_status |= sd->Status;
				drvl1_sdc_clear_timeout_bit(device_id, 10);
				DBG_PRINT("2 drvl1_sdc_read_data_by_dma error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, dma_sd_status, start_sector, sector_count);
				ret = -1;
			}
			else
			{
				if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT))
				{
					dma_sd_status |= sd->Status;
					DBG_PRINT("2 read data_complete_wait fail[%d]. status=0x%08x start=%d, sector_count=%d\r\n", device_id, sd->Status, start_sector, sector_count);
					ret = -1;
				}

				if (drvl1_sdc_data_crc_status_get(device_id))
				{
					DBG_PRINT("2 drvl2_sd_read crc error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
					ret = -1;
				}
			}
		}

		if (drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
		{
			ret = -1;
		}

		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
		{
            dma_sd_status |= sd->Status;
            DBG_PRINT("2 sd read stop error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
            ret = -1;   // some non brand card will happen read stop command response crc error
                        // using 49.5M sd clock will always happen read data crc error
                        // using 47.25M happen some data crc error, retry could pass, but alsmost always happen
                        // read stop command response crc error. Must down untill 41.25M to pass.
        }
	}

	sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;

	if ((ret != 0) || ((dma_sd_status & (C_SDC_STATUS_TIMEOUT | C_SDC_STATUS_CONTROLLER_BUSY | C_SDC_STATUS_CARD_BUSY | C_SDC_STATUS_DATA_CRC_ERR | C_SDC_STATUS_RESP_CRC_ERR | C_SDC_STATUS_RESP_INDEX_ERR)) != 0))
	{
        drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
		if (drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_BUSY_TIMEOUT))
			DBG_PRINT("sd discover card busy after read error !\r\n");
		if (sd_card_apply_read_retry[device_id] || ((dma_sd_status & C_SDC_STATUS_DATA_CRC_ERR) != 0) )
		{
            if (drvl1_sdc_check_card_present(device_id))
            {
                if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) == 0)
                {
                    if (sd_card_apply_read_retry[device_id] == 2)
                        drvl2_sd_recover_to_transfer_state_nosema(device_id, 1);

                    ret = drvl2_sd_read_retry(device_id, start_sector, buffer, sector_count);
                    if (ret != 0) // williamyeo-sd30-dmablksts-not-match retry one more time
                        ret = drvl2_sd_read_retry(device_id, start_sector, buffer, sector_count);
                }
            }
		}
	}

	//DBG_PRINT("read end:start_sector=%u, sector_count=%u\r\n", start_sector, sector_count);

	drvl2_sd_sem_unlock(device_id);

	return ret;
}
#endif

#endif

/**
 * @brief   SD issue command 20.
 * @param   device_id[in]: device id.
 * @param   arg[in]: command argument..
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_cmd20(INT32U device_id,  INT32U arg)
{
	INT32U response;
	INT32S ret;

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD20, arg, C_RESP_R1B, &response) != 0)
		return -1;

	// Wait operation complete and back to transfer state
	ret = drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_ERASE_TIMEOUT);

	return ret;
}

#if (SDC_FS_WRITE_PATH_USE_START_STOP == 1)
/**
 * @brief   SD write function.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: start sector address.
 * @param   *buffer[out]: write buffer.
 * @param   sector_count[in]: write sector count.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_write(INT32U device_id, INT32U start_sector, INT32U *buffer, INT32U sector_count)
{
    INT32S ret;

	#if (SD_DEBUG_ENFORCE_STOP > 0)
		sd_endforce_stop_type[device_id] = 0;
	#endif

    ret = drvl2_sd_write_start(device_id, start_sector, sector_count);
    if (ret != 0)
        goto L_SD_L2_WRITE_RET;

    ret = drvl2_sd_write_sector(device_id, buffer, sector_count, SDC_FS_WRITE_PATH_USE_DMA_MSG);
    if (ret != 0)
        goto L_SD_L2_WRITE_RET;

    ret = drvl2_sd_write_stop(device_id);

	L_SD_L2_WRITE_RET:

	#if (SD_DEBUG_ENFORCE_STOP > 1)

		if (sd_endforce_stop_type[device_id] == 0)
			DBG_PRINT("\r\n");
		else
			DBG_PRINT(" force=%u\r\n", sd_endforce_stop_type[device_id]);
	#endif

    return ret;
}
#else
INT32S drvl2_sd_write(INT32U device_id, INT32U start_sector, INT32U *buffer, INT32U sector_count)
{
	INT32U response;
	INT32S ret;

    drvl2_sd_sem_lock(device_id);

    drvl1_sdc_set_access_kind(device_id, C_SD_L2_ACCESS_WRITE);

	if (sd_card_state[device_id] != SD_CARD_STATE_TRANSFER) {
        drvl2_sd_sem_unlock(device_id);
        return -1;
	}

	drvl2_sd_set_speed_bus(device_id);
	if (drvl2_sd_card_protect_get(device_id)) {
		drvl2_sd_sem_unlock(device_id);
		return -1;
	}
	if (drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_BUSY_TIMEOUT)) {
		drvl2_sd_sem_unlock(device_id);
		return -1;
	}

	if (sector_count == 1) {
		if ((sd_card_type[device_id] == C_MEDIA_SDHC_CARD)||(sd_card_type[device_id] == C_MEDIA_MMCHC_CARD)) {
			if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD24, start_sector, 0, &response)) {
				drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
				if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response)) {
					ret = -1;
				}
				drvl2_sd_sem_unlock(device_id);
				return -1;
			}
		} else {
			if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD24, start_sector<<9, 0, &response)) {
				drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
				if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response)) {
					ret = -1;
				}
				drvl2_sd_sem_unlock(device_id);
				return -1;
			}
		}
		if (drvl1_sdc_command_complete_wait(device_id, C_SD_L2_SINGLE_WRITE_CMD_COMPLETE_TIMEOUT)) {
			drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
			if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response)) {
				ret = -1;
			}
			drvl2_sd_sem_unlock(device_id);
			return -1;
		}
		sd_card_state[device_id] = SD_CARD_STATE_RECEIVE_DATA;

		ret = drvl1_sdc_write_data_by_dma(device_id, buffer, sector_count, NULL);

		sd_card_state[device_id] = SD_CARD_STATE_PROGRAMMING;

		if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_SINGLE_WRITE_DATA_COMPLETE_TIMEOUT)) {
			drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
			ret = -1;
			if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response)) {
				ret = -1;
			}
		}

		if (drvl1_sdc_data_crc_status_get(device_id)) {
			ret = -1;
            {
                volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
                DBG_PRINT("1 drvl2_sd_write crc error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
            }
			drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
		}
	} else {
		if ((sd_card_type[device_id] == C_MEDIA_SDHC_CARD)||(sd_card_type[device_id] == C_MEDIA_MMCHC_CARD)) {
			if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD25, start_sector, 0, &response)) {
				drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
				if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response)) {
					ret = -1;
				}
				drvl2_sd_sem_unlock(device_id);
				return -1;
			}
		} else {
			if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD25, start_sector<<9, 0, &response)) {
				drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
				if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response)) {
					ret = -1;
				}
				drvl2_sd_sem_unlock(device_id);
				return -1;
			}
		}
		#if 0	// test code that produce sd timeout bit 9
        {
            volatile unsigned int k;
            for (k = 0; k < 10000000; k++)
                ;
        }
		#endif
		if (drvl1_sdc_command_complete_wait(device_id, C_SD_L2_MULTI_WRITE_CMD_COMPLETE_TIMEOUT)) {
			drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
			if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response)) {
				ret = -1;
			}
			drvl2_sd_sem_unlock(device_id);
			return -1;
		}
		sd_card_state[device_id] = SD_CARD_STATE_RECEIVE_DATA;

		ret = drvl1_sdc_write_data_by_dma(device_id, buffer, sector_count, NULL);

		if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_WRITE_DATA_COMPLETE_TIMEOUT)) {
			ret = -1;
		}

		if (drvl1_sdc_data_crc_status_get(device_id)) {
			ret = -1;
            {
                volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
                DBG_PRINT("2 drvl2_sd_write crc error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
            }
		}

		if (drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT)) {
			ret = -1;
		}

		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response)) {
			ret = -1;
		}
		sd_card_state[device_id] = SD_CARD_STATE_PROGRAMMING;

	}

	// Wait operation complete and back to transfer state
	ret = drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_PROGRAM_TIMEOUT);
	sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
	drvl2_sd_sem_unlock(device_id);
	return ret;
}
#endif

/**
 * @brief   SD card remove base function.
 * @param   device_id[in]: device id.
 * @return 	0: success.
*/
static INT32S drvl2_sd_card_remove_base(INT32U device_id)
{
	INT32U response;
    INT8U card_is_present = (drvl1_sdc_check_card_present(device_id) == 0) ? 0 : 1;
    INT8U cmd12_done = 0;

	if (sd_card_user[device_id] > 1)
	{
        // when ther is still other user, just minus 1 and return
        sd_card_user[device_id]--;
        return 0;
	}

	// Reset card to idle state
	if (card_is_present) {
        if (sd_card_state[device_id] >= SD_CARD_STATE_TRANSFER) {
            if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response)) {
                sd_card_state[device_id] = SD_CARD_STATE_INACTIVE;
                card_is_present = 0;
            } else {
                card_is_present = 1;
                if ((response & 0x1E00) < 0x800) {
                    // If card exists and not in transfer state, send it back to idle state
                    drvl2_sd_bc_command_set(device_id, C_SD_CMD0, 0x0);
                    sd_card_state[device_id] = SD_CARD_STATE_IDLE;
                } else if ((response & 0x1E00) != 0x800) {
                    // If card is in sending-data or receive data or programming state, send stop command
                    drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
                    cmd12_done = 1;

                    // Wait until card is not busy before turning off controller
                    drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_PROGRAM_TIMEOUT);
                } else if((response & 0x100) && (response & 0x1E00) == 0x800) {
                    drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
                    cmd12_done = 1;

                    // Wait until card is not busy
                    drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_PROGRAM_TIMEOUT);
                }
            }
        }
    }

	if (sd_card_user[device_id])
        sd_card_user[device_id]--;

    if (sd_card_user[device_id] == 0) {
        if (sd_card_is_emmc[device_id]) {
            if (!cmd12_done) {
                drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
                cmd12_done = 1;
            }
        }
    }

	if (!card_is_present) {
        if(device_id == 0){
            drvl1_sdc_disable(device_id);
        }else{
            drvl1_sdc30_disable(device_id);
        }
    }

    if (sd_card_user[device_id] == 0) {
        DBG_PRINT("sd no user, sd_card_state=%u is_emmc=%u, card_is_present=%u\r\n", sd_card_state[device_id], sd_card_is_emmc[device_id], card_is_present);
        if (!card_is_present)
            sd_card_state[device_id] = SD_CARD_STATE_INACTIVE;
    }

	return 0;
}

/**
 * @brief   SD2.0 read start base function.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: start sector address.
 * @param   sector_count[in]: read sector count.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_read_start_base(INT32U device_id, INT32U start_sector, INT32U sector_count)
{
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	INT32U response;
	INT32U comamnd = (sector_count == 1) ? C_SD_CMD17 : C_SD_CMD18;

    drvl1_sdc_set_access_kind(device_id, C_SD_L2_ACCESS_READ);

    if (sd_card_state[device_id] != SD_CARD_STATE_TRANSFER)
	{
        DBG_PRINT("drvl2_sd_read_start_base not SD_CARD_STATE_TRANSFER. Status=0x%08x\r\n", sd->Status);
		return -1;
	}

	drvl2_sd_set_speed_bus(device_id);
	if (drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_BUSY_TIMEOUT))
	{
        DBG_PRINT("drvl2_sd_read_start_base busywait timeout. Status=0x%08x\r\n", sd->Status);
        drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
		sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
		return -1;
    }

    // Clear SD RX data buffer before read command is issued
    drvl1_sdc_clear_rx_data_register(device_id);

	if ((sd_card_type[device_id] == C_MEDIA_SDHC_CARD)||(sd_card_type[device_id] == C_MEDIA_MMCHC_CARD))
	{
		if (drvl2_sd_adtc_command_set(device_id, comamnd, start_sector, 0, &response))
		{
            DBG_PRINT("drvl2_sd_read_start_base sd command err. comamnd=0x%08x Status=0x%08x response=%08x\r\n", comamnd, sd->Status, response);
            drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
			drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
			sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
			return -1;
		}
	}
	else
	{
		if (drvl2_sd_adtc_command_set(device_id, comamnd, start_sector<<9, 0, &response))
		{
            DBG_PRINT("drvl2_sd_read_start_base mmc command err. comamnd=0x%08x Status=0x%08x response=%08x\r\n", comamnd, sd->Status, response);
            drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
			drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
			sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
			return -1;
		}
	}

    if (drvl1_sdc_command_complete_wait(device_id, C_SD_L2_MULTI_READ_CMD_COMPLETE_TIMEOUT))
    {
        drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
        DBG_PRINT("drvl2_sd_read_start_base comamnd complete wait error[%d]. status=0x%08x\r\n", device_id, sd->Status);
        drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
        sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
        return -1;
    }
	sd_card_state[device_id] = SD_CARD_STATE_SENDING_DATA;

	return 0;
}

/**
 * @brief   SD3.0 read start base function.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: start sector address.
 * @param   sector_count[in]: read sector count.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd30_read_start_base(INT32U device_id, INT32U start_sector, INT32U sector_count)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	INT32U response;
	INT32U comamnd = (sector_count == 1) ? C_SD_CMD17 : C_SD_CMD18;

	// williamyeo-sd30-dmablksts-not-match-2
	//drv_l1_module_reset(SD30_RST); // this require 1ms
    drvl1_sdc30_module_reset(); // this require just few us
    drvl1_sdc30_restore_sd30_reg();

    drvl1_sdc_set_access_kind(device_id, C_SD_L2_ACCESS_READ);

    if (sd_card_state[device_id] != SD_CARD_STATE_TRANSFER)
	{
        DBG_PRINT("drvl2_sd_read_start_base not SD_CARD_STATE_TRANSFER. Status=0x%08x\r\n", sd->Status);
		return -1;
	}

	drvl2_sd_set_speed_bus(device_id);
	if (drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_BUSY_TIMEOUT))
	{
        DBG_PRINT("drvl2_sd_read_start_base busywait timeout. Status=0x%08x\r\n", sd->Status);
        drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
		sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
		return -1;
    }

    // Clear SD RX data buffer before read command is issued
    drvl1_sdc_clear_rx_data_register(device_id);
	if ((sd_card_type[device_id] == C_MEDIA_SDHC_CARD)||(sd_card_type[device_id] == C_MEDIA_MMCHC_CARD))
	{
		if (drvl2_sd_adtc_command_set(device_id, comamnd, start_sector, 0, &response))
		{
            DBG_PRINT("drvl2_sd_read_start_base sd command err. comamnd=0x%08x Status=0x%08x response=%08x\r\n", comamnd, sd->Status, response);
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
			drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
			sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
			return -1;
		}
	}
	else
	{
		if (drvl2_sd_adtc_command_set(device_id, comamnd, start_sector<<9, 0, &response))
		{
            DBG_PRINT("drvl2_sd_read_start_base mmc command err. comamnd=0x%08x Status=0x%08x response=%08x\r\n", comamnd, sd->Status, response);
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
			drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
			sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
			return -1;
		}
	}

    if (drvl1_sdc30_command_complete_wait(device_id, C_SD_L2_MULTI_READ_CMD_COMPLETE_TIMEOUT))
    {
        drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
        DBG_PRINT("drvl2_sd_read_start_base comamnd complete wait error[%d]. status=0x%08x\r\n", device_id, sd->Status);
        drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
        sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;
        return -1;
    }
    //drvl1_sdc30_set_read_start(start_sector, sector_count); // williamyeo-sd30-dmablksts-not-match
	sd_card_state[device_id] = SD_CARD_STATE_SENDING_DATA;

	return 0;
}

/**
 * @brief   SD2.0 read sector base function.
 * @param   device_id[in]: device id.
 * @param   buffer[in]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @param   wait_flag[in]: wait DMA flag
 * @param   pdma_sd_status[out]: sd status for DMA error.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_read_sector_base(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8U wait_flag, INT32U *pdma_sd_status)
{
    INT32S ret = 0;
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
    INT32U start_sector = sd_flow_end_sector[device_id] - sector_count;
	INT32U response;

	*pdma_sd_status = 0;
	drvl1_sdc_set_data_timeout(device_id, ENABLE);
	if (wait_flag == 0)
	{
		// Start DMA and return immediately
		ret = drvl1_sdc_read_data_by_dma_ext(device_id, buffer, sector_count, (INT8S *) &sd_read_sector_result[device_id], 1);
		if (ret != 0)
		{
			*pdma_sd_status = sd->Status;
			drvl1_sdc_clear_timeout_bit(device_id, 10);
            DBG_PRINT("1 drvl2_sd_read_sector_base err %d. start=%u nr=%u. ret=%d status=0x%08x\r\n", sd_read_sector_result[device_id], start_sector, sector_count, ret, *pdma_sd_status);
            drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
			drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
			drvl1_sdc_set_data_timeout(device_id, DISABLE);
            return ret;
        }
		#if 1
        if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT))
        {
            (*pdma_sd_status) |= sd->Status;
            drvl1_sdc_clear_timeout_bit(device_id, 10);
            ret = -1;
            DBG_PRINT("1 drvl2_sd_read_sector_base err complete wait error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
        }

		if (drvl1_sdc_data_crc_status_get(device_id))
		{
			ret = -1;
			DBG_PRINT("1 drvl2_sd_read_sector_base err crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
		}
		if (ret != 0){
            drvl1_sdc_set_data_timeout(device_id, DISABLE);
			return ret;
        }
		#endif
	}
	else if (wait_flag == 1)
	{
		// Start DMA and wait until done
        ret = drvl1_sdc_read_data_by_dma(device_id, buffer, sector_count, NULL);
		if (ret != 0)
		{
			*pdma_sd_status = sd->Status;
            drvl1_sdc_clear_timeout_bit(device_id, 10);
            DBG_PRINT("2 drvl2_sd_read_sector_base err %d. start=%u nr=%u. ret=%d status=0x%08x MCLK=%u\r\n", sd_read_sector_result[device_id], start_sector, sector_count, ret, *pdma_sd_status, MCLK);

            #if 0
            {
                INT32U sd_clock_rate, sd_souce_clock_rate, sd_clock_div;

                drvl1_sdc_running_clock_rate_get(device_id, &sd_clock_rate, &sd_souce_clock_rate, &sd_clock_div);
                DBG_PRINT("sd[%u] clock rate=%u, root rate=%u divider=%u\r\n", device_id, sd_clock_rate, sd_souce_clock_rate, sd_clock_div);
            }
            #endif

            drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
			drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
			drvl1_sdc_set_data_timeout(device_id, DISABLE);
			return ret;
		}

        if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT))
        {
            (*pdma_sd_status) |= sd->Status;
            drvl1_sdc_clear_timeout_bit(device_id, 10);
            ret = -1;
            DBG_PRINT("2 drvl2_sd_read_sector_base err complete wait error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
        }

		if (drvl1_sdc_data_crc_status_get(device_id))
		{
			ret = -1;
			DBG_PRINT("2 drvl2_sd_read_sector_base err crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
			if(device_id == 0){
                drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            }else{
                drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            }
		}
		drvl1_sdc_set_data_timeout(device_id, DISABLE);
		return ret;
	}

	// Query status and return when done
	while (1)
	{
        if (sd_read_sector_result[device_id] != C_DMA_STATUS_WAITING)
            break;
        //osDelay(1);
	}
	if (sd_read_sector_result[device_id] != C_DMA_STATUS_DONE)
	{
		*pdma_sd_status = sd->Status;
		drvl1_sdc_clear_timeout_bit(device_id, 10);
        DBG_PRINT("3 drvl2_sd_read_sector_base err %d. sector_count=%u status=0x%08x\r\n", sd_read_sector_result[device_id], sector_count, *pdma_sd_status);
        drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
		drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
		drvl1_sdc_set_data_timeout(device_id, DISABLE);
		return -1;
	}

    if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT))
    {
        (*pdma_sd_status) |= sd->Status;
        drvl1_sdc_clear_timeout_bit(device_id, 10);
        ret = -1;
        DBG_PRINT("3 drvl2_sd_read_sector_base err complete wait error[%d]. status=0x%08x\r\n", device_id, sd->Status);
        drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
        drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
    }
	if (drvl1_sdc_data_crc_status_get(device_id))
	{
		ret = -1;
		DBG_PRINT("3 drvl2_sd_read_sector_base err crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
		drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
	}
    drvl1_sdc_set_data_timeout(device_id, DISABLE);
	return ret;
}

/**
 * @brief   SD3.0 read sector base function.
 * @param   device_id[in]: device id.
 * @param   buffer[in]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @param   wait_flag[in]: wait DMA flag
 * @param   pdma_sd_status[out]: sd status for DMA error.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd30_read_sector_base(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8U wait_flag, INT32U *pdma_sd_status)
{
    INT32S ret = 0;
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
    INT32U start_sector = sd_flow_end_sector[device_id] - sector_count;
	INT32U response;

	*pdma_sd_status = 0;
	if (wait_flag == 0)
	{
		// Start DMA and return immediately
		ret = drvl1_sdc30_read_data_by_dma_ext(device_id, buffer, sector_count, (INT8S *) &sd_read_sector_result[device_id], 1);
		if (ret != 0)
		{
			*pdma_sd_status = sd->Status;
			drvl1_sdc_clear_timeout_bit(device_id, 10);
            DBG_PRINT("1 drvl2_sd30_read_sector_base err %d. start=%u nr=%u. ret=%d status=0x%08x\r\n", sd_read_sector_result[device_id], start_sector, sector_count, ret, *pdma_sd_status);
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
			drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
            return ret;
        }

        if (drvl1_sdc30_data_complete_wait(device_id, C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT))
        {
            (*pdma_sd_status) |= sd->Status;
            drvl1_sdc30_clear_timeout_bit(device_id, 10);
            ret = -1;
            DBG_PRINT("1 drvl2_sd30_read_sector_base err complete wait error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
        }

		if (drvl1_sdc30_data_crc_status_get(device_id))
		{
			ret = -1;
			DBG_PRINT("1 drvl2_sd30_read_sector_base err crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
		}
		if (ret != 0){
			return ret;
        }
	}
	else if (wait_flag == 1)
	{
		// Start DMA and wait until done
        ret = drvl1_sdc30_read_data_by_dma(device_id, buffer, sector_count, NULL);
		if (ret != 0)
		{
			*pdma_sd_status = sd->Status;
            drvl1_sdc30_clear_timeout_bit(device_id, 10);
            if (ret != -7 && ret != -6)
                DBG_PRINT("\r\ndrvl2_sd30_read_sector_base err %d. start=%u nr=%u. ret=%d status=0x%08x SD3MCLK=%u\r\n", \
                       sd_read_sector_result[device_id], start_sector, sector_count, ret, *pdma_sd_status, SD3MCLK);
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
			drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
			return ret;
		}

        if (drvl1_sdc30_data_complete_wait(device_id, C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT))
        {
            (*pdma_sd_status) |= sd->Status;
            drvl1_sdc30_clear_timeout_bit(device_id, 10);
            ret = -1;
            DBG_PRINT("2 drvl2_sd30_read_sector_base err complete wait error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
        }

		if (drvl1_sdc_data_crc_status_get(device_id))
		{
			ret = -1;
			DBG_PRINT("2 drvl2_sd30_read_sector_base err crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
		}
		return ret;
	}

	// Query status and return when done
	while (1)
	{
        if (sd_read_sector_result[device_id] != C_DMA_STATUS_WAITING)
            break;
	}
	if (sd_read_sector_result[device_id] != C_DMA_STATUS_DONE)
	{
		*pdma_sd_status = sd->Status;
		drvl1_sdc_clear_timeout_bit(device_id, 10);
        DBG_PRINT("3 drvl2_sd30_read_sector_base err %d. sector_count=%u status=0x%08x\r\n", sd_read_sector_result[device_id], sector_count, *pdma_sd_status);
        drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
		drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
		return -1;
	}

    if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT))
    {
        (*pdma_sd_status) |= sd->Status;
        drvl1_sdc30_clear_timeout_bit(device_id, 10);
        ret = -1;
        DBG_PRINT("3 drvl2_sd30_read_sector_base err complete wait error[%d]. status=0x%08x\r\n", device_id, sd->Status);
        drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
        drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
    }

	if (drvl1_sdc30_data_crc_status_get(device_id))
	{
		ret = -1;
		DBG_PRINT("3 drvl2_sd30_read_sector_base err crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
		drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
	}
	return ret;
}

/**
 * @brief   SD2.0 read stop base function.
 * @param   device_id[in]: device id.
 * @param   multiple_read[in]: multiple read or single read flag.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_read_stop_base(INT32U device_id, INT32U multiple_read)
{
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	INT32U response;
	INT32S ret = 0;

	ret = 0;

    if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT))
    {
        ret = -1;
        DBG_PRINT("drvl2_sd_read_stop_base complete wait error[%d]. status=0x%08x\r\n", device_id, sd->Status);
    }

	if (drvl1_sdc_data_crc_status_get(device_id))
	{
		ret = -1;
        DBG_PRINT("drvl2_sd_read_stop_base crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
	}

	if (multiple_read)
	{
        if (drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
        {
            ret = -1;

            DBG_PRINT("drvl2_sd_read_stop_base stop controller error[%d]. status=0x%08x\r\n", device_id, sd->Status);
        }

        if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
        {
			ret = -1; // some non brand card will happen read stop command response crc error
                                    // using 49.5M sd clock will always happen read data crc error
                                    // using 47.25M happen some data crc error, retry could pass, but alsmost always happen
                                    // read stop command response crc error. Must down untill 41.25M to pass.
            //DBG_PRINT("drvl2_sd_read_stop_base stop command error[%d]. status=0x%08x nr=%u\r\n", device_id, sd->Status, sd_flow_nr_sector[device_id]);
        }
	}
	else if (ret != 0)
	{
        drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
		if(drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response)) {
            ret = -1;
		}
	}

	drvl1_sdc_clear_timeout_bit(device_id, 10);

	sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;

	return ret;
}

/**
 * @brief   SD3.0 read stop base function.
 * @param   device_id[in]: device id.
 * @param   multiple_read[in]: multiple read or single read flag.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd30_read_stop_base(INT32U device_id, INT32U multiple_read)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	INT32U response;
	INT32S ret = 0;

	ret = 0;

    if (drvl1_sdc30_data_complete_wait(device_id, C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT))
    {
        ret = -1;
        DBG_PRINT("drvl2_sd_read_stop_base complete wait error[%d]. status=0x%08x\r\n", device_id, sd->Status);
    }

	if (drvl1_sdc_data_crc_status_get(device_id))
	{
		ret = -1;
        DBG_PRINT("drvl2_sd_read_stop_base crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
	}

	if (multiple_read)
	{
        if (drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
        {
            ret = -1;

            DBG_PRINT("drvl2_sd_read_stop_base stop controller error[%d]. status=0x%08x\r\n", device_id, sd->Status);
        }

        if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
        {
			ret = -1; // some non brand card will happen read stop command response crc error
                                    // using 49.5M sd clock will always happen read data crc error
                                    // using 47.25M happen some data crc error, retry could pass, but alsmost always happen
                                    // read stop command response crc error. Must down untill 41.25M to pass.
            //DBG_PRINT("drvl2_sd_read_stop_base stop command error[%d]. status=0x%08x nr=%u\r\n", device_id, sd->Status, sd_flow_nr_sector[device_id]);
        }
	}
	else if (ret != 0)
	{
        drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
		drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
	}

	drvl1_sdc_clear_timeout_bit(device_id, 10);

	sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;

	return ret;
}


/**
 * @brief   Set the number of write blocks to be pre-erased before writing.
 * @param   device_id[in]: device id.
 * @param   sector_count[in]: pre-erased sector count.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_set_pre_erase_count(INT32U device_id, INT32U sector_count)
{
	INT32U response;

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD55, sd_card_rca[device_id], C_RESP_R1, &response) == 0)
	{
		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_ACMD23, sector_count, C_RESP_R1, &response) == 0)
			return 0;
	}
	return -1;
}

/**
 * @brief   Set the address of the first write block to be erased.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: first write sector address to be erased.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_set_erase_start_sector(INT32U device_id, INT32U start_sector)
{
	INT32U response;

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD32, start_sector, C_RESP_R1, &response) != 0)
		return -1;
	return 0;
}

/**
 * @brief   Set the address of the last write block of the continuous range to be erased.
 * @param   device_id[in]: device id.
 * @param   end_sector[in]: last write sector address to be erased.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_set_erase_end_sector(INT32U device_id, INT32U end_sector)
{
	INT32U response;

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD33, end_sector, C_RESP_R1, &response) != 0)
		return -1;

	return 0;
}

/**
 * @brief   Erases all previously selected write sector.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_do_erase_sector(INT32U device_id)
{
	INT32U response;
	INT32S ret;

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD38, 0x0, C_RESP_R1B, &response) != 0)
		return -1;

	// Wait operation complete and back to transfer state
	ret = drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_ERASE_TIMEOUT);

	return ret;
}

/**
 * @brief   SD erases sectors no semaphore lock.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: first sector address to be erased.
 * @param   sector_count[in]: erase sector count.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_erase_sectors_no_sema(INT32U device_id, INT32U start_sector, INT32U sector_count)
{
	if (drvl2_sd_set_erase_start_sector(device_id, start_sector) != 0)
		return -1;
	if (drvl2_sd_set_erase_end_sector(device_id, start_sector + sector_count - 1) != 0)
		return -1;
	if (drvl2_sd_do_erase_sector(device_id) != 0)
		return -1;

	return 0;
}

/**
 * @brief   SD erases sectors.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: first sector address to be erased.
 * @param   sector_count[in]: erase sector count.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_erase_sectors(INT32U device_id, INT32U start_sector, INT32U sector_count)
{
	INT32S ret;

	drvl2_sd_sem_lock(device_id);
	ret = drvl2_sd_erase_sectors_no_sema(device_id, start_sector, sector_count);
	drvl2_sd_sem_unlock(device_id);

	return ret;
}

/**
 * @brief   SD erases all.
 * @param   device_id[in]: device id.
 * @param   *progress_proc[in]: erasing progress procedure.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_erase_all(INT32U device_id, void (*progress_proc)(INT32U, INT32U))
{
	INT32S ret = 0;
	INT32U start_sector = 0;
	INT32U sector_count;
	INT32U bulk_sector = sd_flow_device_au_sectors[device_id] * sd_erase_size_au[device_id];
	INT32U left_sector = sd_card_total_sector[device_id];

	if (sd_erase_size_au[device_id] != 0)
        #if SDC_AU_SIZE_USE_6M
        bulk_sector = ((sd_erase_size_au[device_id] > 4) ? 4 : sd_erase_size_au[device_id]) * sd_flow_device_au_sectors[device_id];
        #else
		bulk_sector = sd_flow_device_au_sectors[device_id] * sd_erase_size_au[device_id];
		#endif
	else
		bulk_sector = sd_flow_write_au_sectors[device_id];

	if (bulk_sector == 0) // do nothing when 0
		return 0;

	drvl2_sd_sem_lock(device_id);
	DBG_PRINT("drvl2_sd_erase_all start. bulk_sector=%u\r\n", bulk_sector);
	drvl2_sd_flush_base(device_id);
	while (left_sector != 0)
	{
		if (left_sector > bulk_sector)
			sector_count = bulk_sector;
		else
			sector_count = left_sector;
        //DBG_PRINT("start_sector=%u sector_count=%u\r\n", start_sector, sector_count);
		ret = drvl2_sd_erase_sectors_no_sema(device_id, start_sector, sector_count);
		if (ret != 0)
			break;
        start_sector += sector_count;
		left_sector -= sector_count;
		if (progress_proc != NULL)
		{
            (*progress_proc)(start_sector, sd_card_total_sector[device_id]);
		}
	}
	DBG_PRINT("drvl2_sd_erase_all done. ret=%d, total=%u left=%u\r\n", ret, sd_card_total_sector[device_id], left_sector);
	drvl2_sd_sem_unlock(device_id);

	return ret;
}

/**
 * @brief   SD write start base function.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: start sector of writing.
 * @param   sector_count[in]: sector count of writing.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_write_start_base(INT32U device_id, INT32U start_sector, INT32U sector_count)
{
	INT32U response;
	INT32U command = (sector_count == 1) ? C_SD_CMD24 : C_SD_CMD25;

    drvl1_sdc_set_access_kind(device_id, C_SD_L2_ACCESS_WRITE);

    if (sd_card_state[device_id] != SD_CARD_STATE_TRANSFER)
	{
		return -1;
	}
	if (drvl2_sd_card_protect_get(device_id))
	{
		return -1;
	}
	drvl2_sd_set_speed_bus(device_id);
	if (drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_BUSY_TIMEOUT))
	{
		return -1;
	}

	if ((sd_card_type[device_id] == C_MEDIA_SDHC_CARD)||(sd_card_type[device_id] == C_MEDIA_MMCHC_CARD))
	{
		if (drvl2_sd_adtc_command_set(device_id, command, start_sector, 0, &response))
		{
			return -1;
		}
	}
	else
	{
		if (drvl2_sd_adtc_command_set(device_id, command, start_sector<<9, 0, &response))
		{
			return -1;
		}
	}
    if(device_id == 0){
        if (drvl1_sdc_command_complete_wait(device_id, C_SD_L2_MULTI_WRITE_CMD_COMPLETE_TIMEOUT))
        {
            return -1;
        }
    }else{
        if (drvl1_sdc30_command_complete_wait(device_id, C_SD_L2_MULTI_WRITE_CMD_COMPLETE_TIMEOUT))
        {
            return -1;
        }
    }
	sd_card_state[device_id] = SD_CARD_STATE_RECEIVE_DATA;

	return 0;
}

/**
 * @brief   SD2.0 write sector base function.
 * @param   device_id[in]: device id.
 * @param   buffer[in]: write buffer.
 * @param   sector_count[in]: write sector count.
 * @param   wait_flag[in]: wait DMA flag.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_write_sector_base(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8U wait_flag)
{
    INT32S ret = 0;
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	sd_card_state[device_id] = SD_CARD_STATE_RECEIVE_DATA;

    if (sector_count == 0)
    {
        //DBG_PRINT("drvl2_sd_write_sector_base discover 0 sector\r\n");
        return 0;
    }
    drvl1_sdc_set_data_timeout(device_id, ENABLE);
	if (wait_flag == 0)
	{
		// Start DMA and return immediately
        ret = drvl1_sdc_write_data_by_dma_ext(device_id, buffer, sector_count, (INT8S *) &sd_write_sector_result[device_id], 1);
        if (ret != 0)
        {
            DBG_PRINT("1 drvl2_sd_write_sector_base err %d. sector_count=%u. ret=%d status=0x%08x\r\n", sd_read_sector_result[device_id], sector_count, ret, sd->Status);
            drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
			drvl1_sdc_set_data_timeout(device_id, DISABLE);
			return ret;
        }

        if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_WRITE_DATA_COMPLETE_TIMEOUT))
        {
            ret = -1;

            DBG_PRINT("1 drvl2_sd_write_sector_base. drvl1_sdc_data_complete_wait err. status=0x%08x\r\n", sd->Status);
        }

		if (drvl1_sdc_data_crc_status_get(device_id))
		{
			ret = -1;
			DBG_PRINT("1 drvl2_sd_write_sector_base crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
		}
		if (ret != 0){
            drvl1_sdc_set_data_timeout(device_id, DISABLE);
			return ret;
        }
	}
	else if (wait_flag == 1)
	{
        INT32U save_Status = sd->Status;

		// Start DMA and wait until done
		INT32S ret = drvl1_sdc_write_data_by_dma(device_id, buffer, sector_count, NULL);

		if (ret != 0)
		{
            DBG_PRINT("2 drvl2_sd_write_sector_base err %d. sector_count=%u. ret=%d status=0x%08x save_status=0x%08x\r\n", sd_read_sector_result[device_id], sector_count, ret, sd->Status, save_Status);
            drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
            drvl1_sdc_set_data_timeout(device_id, DISABLE);
			return ret;
		}

        if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_WRITE_DATA_COMPLETE_TIMEOUT))
        {
            ret = -1;

            DBG_PRINT("2 drvl2_sd_write_sector_base. drvl1_sdc_data_complete_wait err. status=0x%08x\r\n", sd->Status);
        }

		if (drvl1_sdc_data_crc_status_get(device_id))
		{
			ret = -1;
			DBG_PRINT("2 drvl2_sd_write_sector_base crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
		}
        drvl1_sdc_set_data_timeout(device_id, DISABLE);
		return ret;
	}

	while (1)
	{
        if (sd_write_sector_result[device_id] != C_DMA_STATUS_WAITING)
            break;
        //osDelay(1);
	}

	if (sd_write_sector_result[device_id] != C_DMA_STATUS_DONE)
	{
        DBG_PRINT("3 drvl2_sd_write_sector_base err %d. sector_count=%u. ret=%d status=0x%08x\r\n", sd_read_sector_result[device_id], sector_count, ret, sd->Status);
		drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
		drvl1_sdc_set_data_timeout(device_id, DISABLE);
		return -1;
	}

    if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_WRITE_DATA_COMPLETE_TIMEOUT))
    {
        ret = -1;

        DBG_PRINT("3 drvl2_sd_write_sector_base. drvl1_sdc_data_complete_wait err. status=0x%08x\r\n", sd->Status);
    }

	if (drvl1_sdc_data_crc_status_get(device_id))
	{
		ret = -1;
        DBG_PRINT("3 drvl2_sd_write_sector_base crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
	}
    drvl1_sdc_set_data_timeout(device_id, DISABLE);
	return ret;
}

/**
 * @brief   SD3.0 read sector base function.
 * @param   buffer[in]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @param   wait_flag[in]: wait DMA flag.
*/
static INT32S drvl2_sd30_write_sector_base(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8U wait_flag)
{
    INT32S ret = 0;
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	sd_card_state[device_id] = SD_CARD_STATE_RECEIVE_DATA;

    if (sector_count == 0)
    {
        //DBG_PRINT("drvl2_sd_write_sector_base discover 0 sector\r\n");
        return 0;
    }
    //drvl1_sdc_set_data_timeout(device_id, ENABLE);
	if (wait_flag == 0)
	{
		// Start DMA and return immediately
        ret = drvl1_sdc30_write_data_by_dma_ext(device_id, buffer, sector_count, (INT8S *) &sd_write_sector_result[device_id], 1);
        if (ret != 0)
        {
            DBG_PRINT("1 drvl2_sd_write_sector_base err %d. sector_count=%u. ret=%d status=0x%08x\r\n", sd_read_sector_result[device_id], sector_count, ret, sd->Status);
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
			//drvl1_sdc_set_data_timeout(device_id, DISABLE);
			return ret;
        }
        if (drvl1_sdc30_data_complete_wait(device_id, C_SD_L2_MULTI_WRITE_DATA_COMPLETE_TIMEOUT))
        {
            ret = -1;

            DBG_PRINT("1 drvl2_sd30_write_sector_base. drvl1_sdc_data_complete_wait err. status=0x%08x\r\n", sd->Status);
        }

		if (drvl1_sdc30_data_crc_status_get(device_id))
		{
			ret = -1;
			DBG_PRINT("1 drvl2_sd30_write_sector_base crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
		}
		if (ret != 0){
            //drvl1_sdc_set_data_timeout(device_id, DISABLE);
			return ret;
        }
	}
	else if (wait_flag == 1)
	{
        INT32U save_Status = sd->Status;

		// Start DMA and wait until done
		INT32S ret = drvl1_sdc30_write_data_by_dma(device_id, buffer, sector_count, NULL);

		if (ret != 0)
		{
            DBG_PRINT("2 drvl2_sd30_write_sector_base err %d. sector_count=%u. ret=%d status=0x%08x save_status=0x%08x\r\n", sd_read_sector_result[device_id], sector_count, ret, sd->Status, save_Status);
            drvl1_sdc30_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
            //drvl1_sdc_set_data_timeout(device_id, DISABLE);
			return ret;
		}
        if (drvl1_sdc30_data_complete_wait(device_id, C_SD_L2_MULTI_WRITE_DATA_COMPLETE_TIMEOUT))
        {
            ret = -1;

            DBG_PRINT("2 drvl2_sd30_write_sector_base. drvl1_sdc_data_complete_wait err. status=0x%08x\r\n", sd->Status);
        }

		if (drvl1_sdc_data_crc_status_get(device_id))
		{
			ret = -1;
			DBG_PRINT("2 drvl2_sd30_write_sector_base crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
		}
        //drvl1_sdc_set_data_timeout(device_id, DISABLE);
		return ret;
	}

	while (1)
	{
        if (sd_write_sector_result[device_id] != C_DMA_STATUS_WAITING)
            break;
        //osDelay(1);
	}

	if (sd_write_sector_result[device_id] != C_DMA_STATUS_DONE)
	{
        DBG_PRINT("3 drvl2_sd30_write_sector_base err %d. sector_count=%u. ret=%d status=0x%08x\r\n", sd_read_sector_result[device_id], sector_count, ret, sd->Status);
		drvl1_sdc30_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
		//drvl1_sdc_set_data_timeout(device_id, DISABLE);
		return -1;
	}

    if (drvl1_sdc30_data_complete_wait(device_id, C_SD_L2_MULTI_WRITE_DATA_COMPLETE_TIMEOUT))
    {
        ret = -1;

        DBG_PRINT("3 drvl2_sd30_write_sector_base. drvl1_sdc_data_complete_wait err. status=0x%08x\r\n", sd->Status);
    }

	if (drvl1_sdc30_data_crc_status_get(device_id))
	{
		ret = -1;
        DBG_PRINT("3 drvl2_sd30_write_sector_base crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
	}
    //drvl1_sdc_set_data_timeout(device_id, DISABLE);
	return ret;
}

/**
 * @brief   SD2.0 write stop base function.
 * @param   device_id[in]: device id.
 * @param   multiple_write[in]: multiple write or single write flag.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_write_stop_base(INT32U device_id, INT32U multiple_write)
{
	INT32U response;
	INT32S ret;
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

	ret = 0;
	if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_WRITE_DATA_COMPLETE_TIMEOUT))
	{
		ret = -1;

        DBG_PRINT("drvl2_sd_write_stop_base. drvl1_sdc_data_complete_wait err. status=0x%08x\r\n", sd->Status);
	}

	if (drvl1_sdc_data_crc_status_get(device_id))
	{
		ret = -1;
        DBG_PRINT("3 drvl2_sd_write crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
	}
	if (multiple_write)
	{
        if (drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT))
        {
            ret = -1;

            DBG_PRINT("drvl2_sd_write_stop_base. drvl1_sdc_stop_controller err. status=0x%08x\r\n", sd->Status);
        }

		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
		{
			ret = -1;

            DBG_PRINT("drvl2_sd_write_stop_base. CMD12 err. status=0x%08x\r\n", sd->Status);
		}
	}
    else if (ret != 0) {
        drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
    }
	sd_card_state[device_id] = SD_CARD_STATE_PROGRAMMING;

    // Wait operation complete and back to transfer state
	ret = drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_PROGRAM_TIMEOUT);
	if (ret != 0)
		DBG_PRINT("drvl2_sd_write_stop_base. drvl2_sd_card_busy_wait err. status=0x%08x\r\n", sd->Status);

	sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;

	return ret;
}

/**
 * @brief   SD3.0 write stop base function.
 * @param   device_id[in]: device id.
 * @param   multiple_write[in]: multiple write or single write flag.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd30_write_stop_base(INT32U device_id, INT32U multiple_write)
{
	INT32U response;
	INT32S ret;
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);

	ret = 0;
	if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_WRITE_DATA_COMPLETE_TIMEOUT))
	{
		ret = -1;

        DBG_PRINT("drvl2_sd30_write_stop_base. drvl1_sdc_data_complete_wait err. status=0x%08x\r\n", sd->Status);
	}

	if (drvl1_sdc_data_crc_status_get(device_id))
	{
		ret = -1;
        DBG_PRINT("3 drvl2_sd30_write crc error[%d]. status=0x%08x\r\n", device_id, sd->Status);
	}
	if (multiple_write)
	{
        if (drvl1_sdc30_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT))
        {
            ret = -1;

            DBG_PRINT("drvl2_sd30_write_stop_base. drvl1_sdc_stop_controller err. status=0x%08x\r\n", sd->Status);
        }

		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
		{
			ret = -1;

            DBG_PRINT("drvl2_sd30_write_stop_base. CMD12 err. status=0x%08x\r\n", sd->Status);
		}
	}
    else if (ret != 0) {
        drvl1_sdc_stop_controller(device_id, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT);
    }
	sd_card_state[device_id] = SD_CARD_STATE_PROGRAMMING;

    // Wait operation complete and back to transfer state
	ret = drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_PROGRAM_TIMEOUT);
	if (ret != 0) {
		DBG_PRINT("drvl2_sd30_write_stop_base. drvl2_sd_card_busy_wait err. status=0x%08x\r\n", sd->Status);
    }
	sd_card_state[device_id] = SD_CARD_STATE_TRANSFER;

	return ret;
}

/**
 * @brief   SD flow flag initialization.
 * @param   device_id[in]: device id.
 * @return 	none.
*/
static void sd_init_flow(INT32U device_id)
{
	sd_flow_state[device_id] = SD_FLOW_IDLE;
	sd_flow_kind[device_id] = SD_FLOW_NONE;
	sd_flow_end_sector[device_id] = 0;
	sd_flow_nr_sector[device_id] = 0;
}


/**
 * @brief   SD write start without semaphore function.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: start sector of writing.
 * @param   sector_count[in]: sector count of writing.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_write_start_nosema(INT32U device_id, INT32U start_sector, INT32U sector_count)
{
	INT32S ret=0;

	if (sd_flow_kind[device_id] == SD_FLOW_READ)
	{
		if (sd_flow_state[device_id] == SD_FLOW_CONTINUE)
		{
			// last read action not be ended, so ended it.
			#if (SD_DEBUG_ENFORCE_STOP > 0)
				sd_endforce_stop_type[device_id] = SD_ENFORCE_STOP_TYPE_READ_BECAUSE_WRITE;
				sd_endforce_stop_type_count[device_id][SD_ENFORCE_STOP_TYPE_READ_BECAUSE_WRITE]++;
				#if (SD_DEBUG_ENFORCE_STOP > 5)
				DBG_PRINT("\r\nendforce read stop at write start. start=%u, lastend=%u, accnr=%u, ret=%d\r\n", start_sector, sd_flow_end_sector[device_id], sd_flow_nr_sector[device_id], ret);
				#endif
			#endif

			ret = drvl2_sd_enforce_read_stop(device_id);
			if (ret != 0)
			{
                DBG_PRINT("\r\nsd_write_start:endforce read stop err %d\r\n", ret);
				return ret;
			}
		}
	}
	else if (sd_flow_kind[device_id] == SD_FLOW_WRITE)
	{
		if (sd_flow_state[device_id] == SD_FLOW_CONTINUE)
		{
			if ((start_sector != sd_flow_end_sector[device_id]) || \
				(sd_flow_single_sector_mode[device_id]) || \
				(start_sector < sd_flow_write_au_start_sector[device_id]) || \
				(start_sector >= (sd_flow_write_au_start_sector[device_id] + sd_flow_write_au_sectors[device_id]))
				)
			{
                INT32U save_sd_flow_end_sector = sd_flow_end_sector[device_id];
                INT32U save_sd_flow_nr_sector = sd_flow_nr_sector[device_id];
				// not continuos address, stop it.

				#if (SD_DEBUG_ENFORCE_STOP > 0)
					sd_endforce_stop_type[device_id] =  SD_ENFORCE_STOP_TYPE_WRITE_RANDOM_ACCESS;
					sd_endforce_stop_type_count[device_id][SD_ENFORCE_STOP_TYPE_WRITE_RANDOM_ACCESS]++;
					#if (SD_DEBUG_ENFORCE_STOP > 5)
					DBG_PRINT("\r\nendforce write stop at write start. start=%u, lastend=%u, accnr=%u, ret=%d\r\n", start_sector, sd_flow_end_sector[device_id], sd_flow_nr_sector[device_id], ret);
					#endif
				#endif

				ret = drvl2_sd_enforce_write_stop(device_id);
				if (ret != 0)
				{
                    DBG_PRINT("\r\nsd_write_start:endforce write stop err %d. start=%u, nr=%u, lastend=%u accnr=%u\r\n", ret, start_sector, sector_count, save_sd_flow_end_sector, save_sd_flow_nr_sector);
					return ret;
				}
			}
			else
			{
				ret = 0;
				return ret;
			}
		}
	}

	sd_flow_kind[device_id] = SD_FLOW_WRITE;
	sd_flow_state[device_id] = SD_FLOW_START;
	sd_flow_end_sector[device_id] = start_sector;
	sd_flow_nr_sector[device_id] = 0;
	sd_flow_write_au_start_sector[device_id] = start_sector / sd_flow_write_au_sectors[device_id] * sd_flow_write_au_sectors[device_id];

	sd_flow_single_sector_mode[device_id] = (sector_count == 1) ? 1 : 0;

    //DBG_PRINT("write start. start=%u, nr=%u\r\n", start_sector, sector_count);

    {
        INT32U try_count = 0;

        L_RETRY_WRITE_START:

        ret = drvl2_sd_write_start_base(device_id, start_sector, sector_count);
        if ((ret != 0) && (try_count < 1) && drvl1_sdc_check_card_present(device_id))
        {
            try_count++;
            if (drvl2_sd_recover_to_transfer_state_nosema(device_id, 1) == 0) // this purpose is to do sd init
            {
                if (try_count <= 1)
                    goto L_RETRY_WRITE_START;
            }

        }
        if (ret != 0)
            DBG_PRINT("\r\nsd_write_start:sd write base err %d, start=%u nr=%u\r\n", ret, start_sector, sector_count);
    }

	return ret;
}

/**
 * @brief   SD write start with semaphore function.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: start sector of writing.
 * @param   sector_count[in]: sector count of writing.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_write_start(INT32U device_id, INT32U start_sector, INT32U sector_count)
{
	INT32S ret;

	drvl2_sd_sem_lock(device_id);
	ret = drvl2_sd_write_start_nosema(device_id, start_sector, sector_count);
	if (ret != 0)
	{
        drvl2_sd_flush_base(device_id);
		drvl2_sd_sem_unlock(device_id);
    }

	return ret;
}

/**
 * @brief   SD write sector without semaphore function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: write buffer.
 * @param   sector_count[in]: sector count of writing.
 * @param   wait_flag[in]: wait DMA flag.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_write_sector_nosema(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8U wait_flag)
{
	INT32S ret;
	INT32U left_count;
    INT32U response;

	L_WRITE_SECTOR_ENTER:
	left_count = 0;
	if ((sd_flow_end_sector[device_id] + sector_count) > (sd_flow_write_au_start_sector[device_id] + sd_flow_write_au_sectors[device_id]))
	{
		// cross au boundary
		left_count = (sd_flow_end_sector[device_id] + sector_count) - (sd_flow_write_au_start_sector[device_id] + sd_flow_write_au_sectors[device_id]);
		sector_count -= left_count;
	}

	sd_flow_state[device_id] = SD_FLOW_CONTINUE;
	sd_flow_nr_sector[device_id] += sector_count;
	sd_flow_end_sector[device_id] += sector_count;

    if(device_id == 0){
        ret = drvl2_sd_write_sector_base(device_id, buffer, sector_count, wait_flag);
    }else{
        ret = drvl2_sd30_write_sector_base(device_id, buffer, sector_count, wait_flag);
    }
	if (ret != 0)
	{
		INT32U start_sector = sd_flow_end_sector[device_id] - sector_count;

		drvl2_sd_enforce_write_stop(device_id);

		if (sd_flow_single_sector_mode[device_id])
		{
            INT32S temp_ret;

			temp_ret = drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response);
			if (temp_ret != 0)
				DBG_PRINT("single write CMD12 enforce result=%d\r\n", temp_ret);
			temp_ret = drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_PROGRAM_TIMEOUT);
			if (temp_ret != 0)
				DBG_PRINT("single write CMD12 busy wait enforce result=%d\r\n", temp_ret);
		}

        if (drvl1_sdc_check_card_present(device_id))
        {
            // retry when card exist
            if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) == 0)
            {
                ret = drvl2_sd_write_start_nosema(device_id, start_sector, sector_count);
                if (ret == 0)
                {
                    sd_flow_state[device_id] = SD_FLOW_CONTINUE;
                    sd_flow_nr_sector[device_id] += sector_count;
                    sd_flow_end_sector[device_id] += sector_count;
                    if(device_id == 0){
                        ret = drvl2_sd_write_sector_base(device_id, buffer, sector_count, wait_flag);
                    }else{
                        ret = drvl2_sd30_write_sector_base(device_id, buffer, sector_count, wait_flag);
                    }
                    if (ret != 0)
                        drvl2_sd_enforce_write_stop(device_id);
                }
            }
        }
	}

	if ((ret == 0) &&  (left_count != 0) && (!sd_flow_single_sector_mode[device_id]))
	{
		// handle cross boundary part
		INT32U start_sector = sd_flow_end_sector[device_id];

		ret = drvl2_sd_write_stop_nosem(device_id);
		if (ret == 0)
		{
			ret = drvl2_sd_write_start_nosema(device_id, start_sector, left_count);
			if (ret == 0)
			{
                buffer += (sector_count * 128L); // buffer is word pointer, so multiply 128 instead of 512
				sector_count = left_count;
				goto L_WRITE_SECTOR_ENTER;
			}
		}
	}

	return ret;
}


/**
 * @brief   SD write sector function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: write buffer.
 * @param   sector_count[in]: sector count of writing.
 * @param   wait_flag[in]: wait DMA flag.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_write_sector(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8U wait_flag)
{
	INT32S ret = -1;

    if (drvl1_sdc_check_card_present(device_id))
    {
        ret = drvl2_sd_write_sector_nosema(device_id, buffer, sector_count, wait_flag);
    }

	#if C_SD_DEBUG_CHECK_FOR_64_ALIGNED == 1
	if (((INT32U)buffer) & 0x3F) {
        INT32U k;
        for (k=0; k < 10; k++) {
            DBG_PRINT("drvl2_sd_write_sector discover non 64 alignment, 0x%x\r\n", (INT32U)buffer);
        }
	}
	#endif

	if (ret != 0)
	{
        drvl2_sd_flush_base(device_id);
		drvl2_sd_sem_unlock(device_id);
    }

	return ret;
}

/**
 * @brief   SD enforce write stop function.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_enforce_write_stop(INT32U device_id)
{
	INT32S ret;

	sd_flow_state[device_id] = SD_FLOW_STOP;
	if(device_id == 0){
        ret = drvl2_sd_write_stop_base(device_id, (sd_flow_single_sector_mode[device_id]?0:1));
    }else{
        ret = drvl2_sd30_write_stop_base(device_id, (sd_flow_single_sector_mode[device_id]?0:1));
    }
	sd_init_flow(device_id);

	return ret;
}

/**
 * @brief   SD write stop without semaphore function.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_write_stop_nosem(INT32U device_id)
{
	INT32S ret = 0;

	if ((sd_flow_single_sector_mode[device_id]) ||(sd_flow_end_sector[device_id] == (sd_flow_write_au_start_sector[device_id] ) + sd_flow_write_au_sectors[device_id]))
	{
		#if (SD_DEBUG_ENFORCE_STOP > 0)
			if (sd_flow_single_sector_mode[device_id])
			{
				sd_endforce_stop_type[device_id] = SD_ENFORCE_STOP_TYPE_WRITE_ONE_SECTOR;
				sd_endforce_stop_type_count[device_id][SD_ENFORCE_STOP_TYPE_WRITE_ONE_SECTOR]++;
			}
			else
			{
				sd_endforce_stop_type[device_id] = SD_ENFORCE_STOP_TYPE_WRITE_AU_END;
				sd_endforce_stop_type_count[device_id][SD_ENFORCE_STOP_TYPE_WRITE_AU_END]++;
			}
			#if (SD_DEBUG_ENFORCE_STOP > 5)
			DBG_PRINT("\r\nendforce write stop at write stop. lastend=%u, accnr=%u, ret=%d\r\n", sd_flow_end_sector[device_id], sd_flow_nr_sector[device_id], ret);
			#endif
		#endif

		ret = drvl2_sd_enforce_write_stop(device_id);
	}

	return ret;
}

/**
 * @brief   SD write stop function.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_write_stop(INT32U device_id)
{
	INT32S ret;

	ret = drvl2_sd_write_stop_nosem(device_id);
	drvl2_sd_sem_unlock(device_id);

	return ret;
}

#if 1

/**
 * @brief   Calculates retry count.
 * @param   device_id[in]: device id.
 * @param   dma_sd_status[in]: sd status after DMA finished.
 * @return 	retry count.
*/
static INT32U drvl2_sd_cal_retry_count(INT32U device_id, INT32U dma_sd_status)
{
    INT32U retry_count = 0;

    if (dma_sd_status == 0)
        return 0;

    if (drvl1_sdc_check_card_present(device_id) == 0)
        // card off so no more retry, so return 0 retry count
        return 0;

    else if (dma_sd_status & C_SDC_STATUS_TIMEOUT)
    {
        INT32U sd_timeout_ms;
        INT32U sd_max_timeout_ms = (INT32U)sd_card_read_retry_max_timeout[device_id] * 1000UL;

        if (MCLK >= 81000000)
            sd_timeout_ms = 135;
        else
            sd_timeout_ms = (((INT64U)10485760000)/MCLK);

        if (sd_timeout_ms != 0)
            retry_count = sd_max_timeout_ms / sd_timeout_ms;
        if (retry_count > 2)
            retry_count -= 2;
        if (retry_count == 0)
            retry_count = 1;
        return retry_count;
    }
    else
        return 1;
}

/**
 * @brief   SD2.0 using sd multiple read command, and no retry, no state machine.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: read start sector.
 * @param   *buffer[in]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_read_private_use(INT32U device_id, INT32U start_sector, INT32U *buffer, INT32U sector_count)
{
    volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
	INT32U response;
	INT32S ret;
	INT32U dma_sd_status = 0;
	INT8S retry_count = 0;

	L_SD_READ_RETRY:

	ret = 0;

	drvl2_sd_set_speed_bus(device_id);
	if (drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_BUSY_TIMEOUT))
	{
        return -1;
	}

    // Clear SD RX data register before read command is issued
    drvl1_sdc_clear_rx_data_register(device_id);

	dma_sd_status = 0;

	if ((sd_card_type[device_id] == C_MEDIA_SDHC_CARD)||(sd_card_type[device_id] == C_MEDIA_MMCHC_CARD))
	{
		if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD18, start_sector, 0, &response))
		{
			dma_sd_status |= sd->Status;
			drvl1_sdc_clear_timeout_bit(device_id, 10);
		}
	}
	else
	{
		if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD18, start_sector<<9, 0, &response))
		{
			dma_sd_status |= sd->Status;
			drvl1_sdc_clear_timeout_bit(device_id, 10);
		}
	}

	if (response == K_SD_RESPONSE_INIT_VALUE)
	{
        if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) != 0)
            return -1;
	}

	if (ret == 0)
	{
        if (drvl1_sdc_command_complete_wait(device_id, C_SD_L2_MULTI_READ_CMD_COMPLETE_TIMEOUT))
        {
            dma_sd_status |= sd->Status;
            drvl1_sdc_clear_timeout_bit(device_id, 10);
            DBG_PRINT("3 drvl1_sdc_command_complete_wait error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, dma_sd_status, start_sector, sector_count);
            ret = -1;
        }

		if ((ret == 0) && drvl1_sdc_check_card_present(device_id))
		{
            ret = drvl1_sdc_read_data_by_dma(device_id, buffer, sector_count, NULL);
			if (ret != 0)
			{
				dma_sd_status |= sd->Status;
				drvl1_sdc_clear_timeout_bit(device_id, 10);
				//DBG_PRINT("3 drvl1_sdc_read_data_by_dma error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, dma_sd_status, start_sector, sector_count);
				ret = -1;
			}
			else
			{
				if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT))
				{
					dma_sd_status |= sd->Status;
					drvl1_sdc_clear_timeout_bit(device_id, 10);
					DBG_PRINT("3 read data_complete_wait fail[%d]. status=0x%08x start=%d, sector_count=%d\r\n", device_id, sd->Status, start_sector, sector_count);
					ret = -1;
				}

				if (drvl1_sdc_data_crc_status_get(device_id))
				{
					DBG_PRINT("3 drvl2_sd_read crc error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
					ret = -1;
				}
			}
		}
	}

    if (drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
    {
        //ret = -1; // I think fail only happen when power is off, so no need to set error
    }

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
	{
        dma_sd_status |= sd->Status;
		drvl1_sdc_clear_timeout_bit(device_id, 10);

        DBG_PRINT("3 sd read stop error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
		ret = -1;   // some non brand card will happen read stop command response crc error
                    // using 49.5M sd clock will always happen read data crc error
					// using 47.25M happen some data crc error, retry could pass, but alsmost always happen
                    // read stop command response crc error. Must down untill 41.25M to pass.
    }

	if ((dma_sd_status & (C_SDC_STATUS_TIMEOUT | C_SDC_STATUS_CONTROLLER_BUSY | C_SDC_STATUS_CARD_BUSY | C_SDC_STATUS_DATA_CRC_ERR | C_SDC_STATUS_RESP_CRC_ERR | C_SDC_STATUS_RESP_INDEX_ERR)) != 0)
	{
		drvl1_sdc_clear_timeout_bit(device_id, 10); // clear again just for safe
        drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT); // stop sd controller again just for safe

		if (sd_card_apply_read_retry[device_id])
		{
            if (drvl1_sdc_check_card_present(device_id))
            {
                if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) == 0)
                {
                    if (sd_card_apply_read_retry[device_id] == 2)
                        drvl2_sd_recover_to_transfer_state_nosema(device_id, 1);

                    if (retry_count < drvl2_sd_cal_retry_count(device_id, dma_sd_status))
                    {
                        if (drvl1_sdc_check_card_present(device_id))
                        {
                            if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) == 0)
                            {
                                retry_count++;
                                goto L_SD_READ_RETRY;
                            }
                        }
                    }
                }
            }
		}
	}

	return ret;
}

/**
 * @brief   SD3.0 using sd multiple read command, and no retry, no state machine.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: read start sector.
 * @param   *buffer[in]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd30_read_private_use(INT32U device_id, INT32U start_sector, INT32U *buffer, INT32U sector_count)
{
    volatile sd30Reg_t *sd = (sd30Reg_t*) SetSDBase(device_id);
	INT32U response;
	INT32S ret;
	INT32U dma_sd_status = 0;
	INT8S retry_count = 0;

	L_SD_READ_RETRY:

	ret = 0;

	drvl2_sd_set_speed_bus(device_id);
	if (drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_BUSY_TIMEOUT))
	{
        return -1;
	}

    drvl1_sdc30_module_reset(); // this require just few us
    drvl1_sdc30_restore_sd30_reg();

    // Clear SD RX data register before read command is issued
    drvl1_sdc30_clear_rx_data_register(device_id);

	dma_sd_status = 0;

	if ((sd_card_type[device_id] == C_MEDIA_SDHC_CARD)||(sd_card_type[device_id] == C_MEDIA_MMCHC_CARD))
	{
		if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD18, start_sector, 0, &response))
		{
			dma_sd_status |= sd->Status;
			drvl1_sdc_clear_timeout_bit(device_id, 10);
		}
	}
	else
	{
		if (drvl2_sd_adtc_command_set(device_id, C_SD_CMD18, start_sector<<9, 0, &response))
		{
			dma_sd_status |= sd->Status;
			drvl1_sdc_clear_timeout_bit(device_id, 10);
		}
	}

	if (response == K_SD_RESPONSE_INIT_VALUE)
	{
        if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) != 0)
            return -1;
	}

	if (ret == 0)
	{
        if (drvl1_sdc30_command_complete_wait(device_id, C_SD_L2_MULTI_READ_CMD_COMPLETE_TIMEOUT))
        {
            dma_sd_status |= sd->Status;
            drvl1_sdc30_clear_timeout_bit(device_id, 10);
            DBG_PRINT("3.1 drvl1_sdc_command_complete_wait error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, dma_sd_status, start_sector, sector_count);
            ret = -1;
        }

		if ((ret == 0) && drvl1_sdc_check_card_present(device_id))
		{
            drvl1_sdc30_set_read_start(start_sector, sector_count); // williamyeo-sd30-dmablksts-not-match

            ret = drvl1_sdc30_read_data_by_dma(device_id, buffer, sector_count, NULL);
			if (ret != 0)
			{
				dma_sd_status |= sd->Status;
				drvl1_sdc_clear_timeout_bit(device_id, 10);
				//DBG_PRINT("3 drvl1_sdc_read_data_by_dma error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, dma_sd_status, start_sector, sector_count);
				ret = -1;
			}
			else
			{
				if (drvl1_sdc_data_complete_wait(device_id, C_SD_L2_MULTI_READ_DATA_COMPLETE_TIMEOUT))
				{
					dma_sd_status |= sd->Status;
					drvl1_sdc_clear_timeout_bit(device_id, 10);
					DBG_PRINT("3 read data_complete_wait fail[%d]. status=0x%08x start=%d, sector_count=%d\r\n", device_id, sd->Status, start_sector, sector_count);
					ret = -1;
				}

				if (drvl1_sdc_data_crc_status_get(device_id))
				{
					DBG_PRINT("3 drvl2_sd_read crc error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
					ret = -1;
				}
			}
		}
	}

    if (drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT))
    {
        //ret = -1; // I think fail only happen when power is off, so no need to set error
    }

	if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD12, 0x0, C_RESP_R1B, &response))
	{
        dma_sd_status |= sd->Status;
		drvl1_sdc_clear_timeout_bit(device_id, 10);

        DBG_PRINT("3 sd read stop error[%d]. status=0x%08x start=%u nr=%u\r\n", device_id, sd->Status, start_sector, sector_count);
		ret = -1;   // some non brand card will happen read stop command response crc error
                    // using 49.5M sd clock will always happen read data crc error
					// using 47.25M happen some data crc error, retry could pass, but alsmost always happen
                    // read stop command response crc error. Must down untill 41.25M to pass.
    }

	if ((dma_sd_status & (C_SDC_STATUS_TIMEOUT | C_SDC_STATUS_CONTROLLER_BUSY | C_SDC_STATUS_CARD_BUSY | C_SDC_STATUS_DATA_CRC_ERR | C_SDC_STATUS_RESP_CRC_ERR | C_SDC_STATUS_RESP_INDEX_ERR)) != 0)
	{
		drvl1_sdc_clear_timeout_bit(device_id, 10); // clear again just for safe
        drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT); // stop sd controller again just for safe
		if (sd_card_apply_read_retry[device_id])
		{
            if (drvl1_sdc_check_card_present(device_id))
            {
                if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) == 0)
                {
                    if (sd_card_apply_read_retry[device_id] == 2)
                        drvl2_sd_recover_to_transfer_state_nosema(device_id, 1);

                    if (retry_count < drvl2_sd_cal_retry_count(device_id, dma_sd_status))
                    {
                        if (drvl1_sdc_check_card_present(device_id))
                        {
                            if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) == 0)
                            {
                                retry_count++;
                                goto L_SD_READ_RETRY;
                            }
                        }
                    }
                }
            }
		}
	}

	return ret;
}

/**
 * @brief   SD read with retry.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: read start sector.
 * @param   *buffer[in]: read buffer.
 * @param   sector_count[in]: read sector count.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_read_retry(INT32U device_id, INT32U start_sector, INT32U *buffer, INT32U sector_count)
{
	INT32S ret = 0;
	INT32S temp_ret;
    INT32U response;

	//DBG_PRINT("drvl2_sd_read_retry enter:start %u, nr %u\r\n", start_sector, sector_count);

    if (drvl1_sdc_check_card_present(device_id) == 0) {
        // card off so no more retry
        return -1;
    }
    if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) != 0) {
        return -1;
    }
	DBG_PRINT("\r\n");
	// retry whole sectors directly
	if(device_id == 0){
        ret = drvl2_sd_read_private_use(device_id, start_sector, buffer, sector_count);
    }else{
        ret = drvl2_sd30_read_private_use(device_id, start_sector, buffer, sector_count);
    }
	if (ret != 0) {
        DBG_PRINT("x\r\n\r\n");
    }
    else
        DBG_PRINT("o\r\n\r\n");

	//DBG_PRINT("1 drvl2_sd_read_retry exit. ret=%d\r\n", ret);
    return ret;

	// retry by one sector first, then retry remain sectors
	L_RETRY_ONE_SECTOR:
	if(device_id == 0){
        temp_ret = drvl2_sd_read_private_use(device_id, start_sector, buffer, 1);
    }else{
        temp_ret = drvl2_sd30_read_private_use(device_id, start_sector, buffer, 1);
    }
	if ((temp_ret == 0) && (sector_count > 1))
	{
		DBG_PRINT("1 sd_read_retry success one sector %u\r\n", start_sector);

		start_sector++;
		buffer += 128;
		sector_count--;
		if(device_id == 0){
            temp_ret = drvl2_sd_read_private_use(device_id, start_sector, buffer, sector_count);
        }else{
            temp_ret = drvl2_sd30_read_private_use(device_id, start_sector, buffer, sector_count);
        }
		if (temp_ret != 0)
		{
			if (sector_count > 1)
			{
				DBG_PRINT("sd_read_retry found remain %u sector bad at %u, retry with one sector\r\n", sector_count, start_sector);
				goto L_RETRY_ONE_SECTOR;
			}
			else
			{
				DBG_PRINT("sd_read_retry found remain %u sector bad at %u\r\n", sector_count, start_sector);
				ret = temp_ret;
			}
		}
		else
			DBG_PRINT("sd_read_retry success remain sector. start %u, nr %u\r\n", start_sector, sector_count);
	}
	else
	{
		#if 0
		// skip bad sector and try again
		if (sector_count > 1)
		{
			ret = -1;
			sector_count--;
			DBG_PRINT("sd_read_retry found bad sector at %u, skip it and retry remain %u sectors\r\n", start_sector, sector_count);
			start_sector++;
			buffer += 128;
			goto L_RETRY_ONE_SECTOR;
		}
		else
		#endif
		if (temp_ret != 0)
		{
			ret = temp_ret;
			DBG_PRINT("sd_read_retry found bad sector at %u\r\n", start_sector);
		}
		else
			DBG_PRINT("2 sd_read_retry success one sector %u\r\n", start_sector);
	}

	DBG_PRINT("2 drvl2_sd_read_retry exit. ret=%d\r\n", ret);

	return ret;
}

#endif

/**
 * @brief   SD read start without semaphore.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: read start sector.
 * @param   sector_count[in]: read sector count.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_read_start_nosema(INT32U device_id, INT32U start_sector, INT32U sector_count)
{
	INT32S ret = 0;

	if (sd_flow_kind[device_id] == SD_FLOW_WRITE)
	{
		if (sd_flow_state[device_id] == SD_FLOW_CONTINUE)
		{
			// last write action not be ended, so ended it.
			#if (SD_DEBUG_ENFORCE_STOP > 0)
				sd_endforce_stop_type[device_id] = SD_ENFORCE_STOP_TYPE_WRITE_BECAUSE_READ;
				sd_endforce_stop_type_count[device_id][SD_ENFORCE_STOP_TYPE_WRITE_BECAUSE_READ]++;
				#if (SD_DEBUG_ENFORCE_STOP > 5)
				DBG_PRINT("\r\nendforce write stop at read start. start=%u, lastend=%u, accnr=%u, ret=%d\r\n", start_sector, sd_flow_end_sector[device_id], sd_flow_nr_sector[device_id], ret);
				#endif
			#endif

			ret = drvl2_sd_enforce_write_stop(device_id);
			if (ret != 0)
			{
				return ret;
			}
		}
	}
	else if (sd_flow_kind[device_id] == SD_FLOW_READ)
	{
		if (sd_flow_state[device_id] == SD_FLOW_CONTINUE)
		{
			if ((start_sector != sd_flow_end_sector[device_id]) || \
				(sd_flow_single_sector_mode[device_id]) || \
				(sd_flow_nr_sector[device_id] >= sd_flow_read_au_sectors[device_id])
				)
			{
				// not continuos address, stop it.
				#if (SD_DEBUG_ENFORCE_STOP > 0)
					sd_endforce_stop_type[device_id] = SD_ENFORCE_STOP_TYPE_READ_RANDOM_ACCESS;
					sd_endforce_stop_type_count[device_id][SD_ENFORCE_STOP_TYPE_READ_RANDOM_ACCESS]++;
					#if (SD_DEBUG_ENFORCE_STOP > 5)
					DBG_PRINT("\r\nendforce read stop at read start. start=%u, lastend=%u, accnr=%u, ret=%d\r\n", start_sector, sd_flow_end_sector[device_id], sd_flow_nr_sector[device_id], ret);
					#endif
				#endif

				ret = drvl2_sd_enforce_read_stop(device_id);
				if (ret != 0)
				{
					return ret;
				}
			}
			else
			{
                ret = 0;
				return ret;
			}
		}
	}

	sd_flow_kind[device_id] = SD_FLOW_READ;
	sd_flow_state[device_id] = SD_FLOW_START;
	sd_flow_end_sector[device_id] = start_sector;
	sd_flow_nr_sector[device_id] = 0;

	if(device_id == 1)
        drvl1_sdc30_set_read_start(start_sector, sector_count); // williamyeo-sd30-dmablksts-not-match

    //sd_flow_single_sector_mode[device_id] = (sector_count == 1) ? 1 : 0;
    sd_flow_single_sector_mode[device_id] = 0; // set to to always use multiple sector read
    sector_count = 0; // this is to cheat drvl2_sd_read_start_base use multiple sector read

    {
        INT32U try_count = 0;

        L_RETRY_READ_START:
        if(device_id == 0){
            ret = drvl2_sd_read_start_base(device_id, start_sector, sector_count);
        }else{
            ret = drvl2_sd30_read_start_base(device_id, start_sector, sector_count);
        }

        if ((ret != 0) && (try_count < 1) && drvl1_sdc_check_card_present(device_id))
        {
            try_count++;
            if (drvl2_sd_recover_to_transfer_state_nosema(device_id, 1) == 0) // this purpose is to do sd init
            {
                if (try_count <= 1)
                    goto L_RETRY_READ_START;
            }

        }
        if (ret != 0)
            DBG_PRINT("\r\nsd_read_start:sd read base err %d, start=%u nr=%u\r\n", ret, start_sector, sector_count);
    }

	return ret;
}

/**
 * @brief   SD read start function.
 * @param   device_id[in]: device id.
 * @param   start_sector[in]: read start sector.
 * @param   sector_count[in]: read sector count.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_read_start(INT32U device_id, INT32U start_sector, INT32U sector_count)
{
	INT32S ret;

	drvl2_sd_sem_lock(device_id);
    ret = drvl2_sd_read_start_nosema(device_id, start_sector, sector_count);
	if (ret != 0)
	{
        drvl2_sd_flush_base(device_id);
		drvl2_sd_sem_unlock(device_id);
    }

	return ret;
}

/**
 * @brief   SD read sector without semaphore function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: read buffer.
 * @param   sector_count[in]: sector count of reading.
 * @param   wait_flag[in]: wait DMA flag.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_read_sector_nosema(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8U wait_flag)
{
	INT32S ret = 0;
	INT32U dma_sd_status = 0;

	sd_flow_state[device_id] = SD_FLOW_CONTINUE;
	sd_flow_nr_sector[device_id] += sector_count;
	sd_flow_end_sector[device_id] += sector_count;

    if(device_id == 0){
        ret = drvl2_sd_read_sector_base(device_id, buffer, sector_count, wait_flag, &dma_sd_status);
    }else{
        ret = drvl2_sd30_read_sector_base(device_id, buffer, sector_count, wait_flag, &dma_sd_status);
    }
	if (ret != 0)
	{
		INT32U start_sector = sd_flow_end_sector[device_id] - sector_count;

		drvl2_sd_enforce_read_stop(device_id);

		if ((ret != 0) || ((dma_sd_status & (C_SDC_STATUS_TIMEOUT | C_SDC_STATUS_CONTROLLER_BUSY | C_SDC_STATUS_CARD_BUSY | C_SDC_STATUS_DATA_CRC_ERR | C_SDC_STATUS_RESP_CRC_ERR | C_SDC_STATUS_RESP_INDEX_ERR)) != 0))
		{
            if(device_id == 0){
                drvl1_sdc_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            }else{
                drvl1_sdc30_stop_controller(device_id, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT);
            }

            if (drvl2_sd_card_busy_wait(device_id, C_SD_L2_CARD_BUSY_TIMEOUT))
				DBG_PRINT("1 sd discover card busy after read error !\r\n");

            if (sd_card_apply_read_retry[device_id] || ((dma_sd_status & C_SDC_STATUS_DATA_CRC_ERR) != 0) )
            {
                if (drvl1_sdc_check_card_present(device_id))
                {
                    INT32U response;

                    if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) == 0)
                    {
                        if (sd_card_apply_read_retry[device_id] == 2)
                            drvl2_sd_recover_to_transfer_state_nosema(device_id, 1);

                        ret = drvl2_sd_read_retry(device_id, start_sector, buffer, sector_count);
                    }
                }
            }
		}
	}

	return ret;
}

/**
 * @brief   SD read sector function.
 * @param   device_id[in]: device id.
 * @param   *buffer[in]: read buffer.
 * @param   sector_count[in]: sector count of reading.
 * @param   wait_flag[in]: wait DMA flag.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_read_sector(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8U wait_flag)
{
	INT32S ret = -1;

    if (drvl1_sdc_check_card_present(device_id))
    {
        ret = drvl2_sd_read_sector_nosema(device_id, buffer, sector_count, wait_flag);
    }

	#if C_SD_DEBUG_CHECK_FOR_64_ALIGNED == 1
	if (((INT32U)buffer) & 0x3F) {
        INT32U k;
        for (k=0; k < 10; k++) {
            DBG_PRINT("drvl2_sd_read_sector discover non 64 alignment, 0x%x\r\n", (INT32U)buffer);
        }
	}
	#endif

	if (ret != 0)
	{
        drvl2_sd_flush_base(device_id);
		drvl2_sd_sem_unlock(device_id);
    }

	return ret;
}

/**
 * @brief   SD enforce read stop function.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_enforce_read_stop(INT32U device_id)
{
	INT32S ret = 0;

	sd_flow_state[device_id] = SD_FLOW_STOP;
	if(device_id == 0){
        ret = drvl2_sd_read_stop_base(device_id, (sd_flow_single_sector_mode[device_id]?0:1));
    }else{
        ret = drvl2_sd30_read_stop_base(device_id, (sd_flow_single_sector_mode[device_id]?0:1));
    }
	sd_init_flow(device_id);

	return ret;
}

/**
 * @brief   SD read stop function.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_read_stop(INT32U device_id)
{
	INT32S ret = 0;

	if ((sd_flow_single_sector_mode[device_id]) || sd_flow_nr_sector[device_id] >= sd_flow_read_au_sectors[device_id])
	{
		#if (SD_DEBUG_ENFORCE_STOP > 0)
			if (sd_flow_single_sector_mode[device_id])
			{
				sd_endforce_stop_type[device_id] = SD_ENFORCE_STOP_TYPE_READ_ONE_SECTOR;
				sd_endforce_stop_type_count[device_id][SD_ENFORCE_STOP_TYPE_READ_ONE_SECTOR]++;
			}
			else
			{
				sd_endforce_stop_type[device_id] = SD_ENFORCE_STOP_TYPE_READ_AU_END;
				sd_endforce_stop_type_count[device_id][SD_ENFORCE_STOP_TYPE_READ_AU_END]++;
			}
			#if (SD_DEBUG_ENFORCE_STOP > 5)
			DBG_PRINT("\r\nendforce read stop at read stop. lastend=%u, accnr=%u, ret=%d\r\n", sd_flow_end_sector[device_id], sd_flow_nr_sector[device_id], ret);
			#endif
		#endif

		ret = drvl2_sd_enforce_read_stop(device_id);
	}
	drvl2_sd_sem_unlock(device_id);

	return ret;
}

/**
 * @brief   SD read/write flush function.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
static INT32S drvl2_sd_flush_base(INT32U device_id)
{
    INT32S ret = 0;

	if (sd_flow_kind[device_id] == SD_FLOW_READ)
	{
		if ((sd_flow_state[device_id] == SD_FLOW_CONTINUE) || (sd_flow_state[device_id] == SD_FLOW_START))
		{
			// last read action not be ended, so ended it.
			ret = drvl2_sd_enforce_read_stop(device_id);

			#if (SD_DEBUG_ENFORCE_STOP > 0)
				sd_endforce_stop_type[device_id] = SD_ENFORCE_STOP_TYPE_READ_FLUSH;
				sd_endforce_stop_type_count[device_id][SD_ENFORCE_STOP_TYPE_READ_FLUSH]++;
				#if (SD_DEBUG_ENFORCE_STOP > 5)
				DBG_PRINT("\r\ndrvl2_sd_flush enfore read stop\r\n");
				#endif
			#endif

			if (ret != 0)
			{
				//return ret;
			}
		}
	}
	else if (sd_flow_kind[device_id] == SD_FLOW_WRITE)
	{
		if ((sd_flow_state[device_id] == SD_FLOW_CONTINUE) || (sd_flow_state[device_id] == SD_FLOW_START))
		{
			// last write action not be ended, so ended it.
			ret = drvl2_sd_enforce_write_stop(device_id);

			#if (SD_DEBUG_ENFORCE_STOP > 0)
				sd_endforce_stop_type[device_id] = SD_ENFORCE_STOP_TYPE_WRITE_FLUSH;
				sd_endforce_stop_type_count[device_id][SD_ENFORCE_STOP_TYPE_WRITE_FLUSH]++;
				#if (SD_DEBUG_ENFORCE_STOP > 5)
				DBG_PRINT("\r\ndrvl2_sd_flush enfore write stop\r\n");
				#endif
			#endif

			if (ret != 0)
			{
				//return ret;
			}
		}
	}
	return ret;
}

/**
 * @brief   SD read/write flush function with semaphore.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_flush(INT32U device_id)
{
    INT32S ret;

    drvl2_sd_sem_lock(device_id);
    ret = drvl2_sd_flush_base(device_id);
    drvl2_sd_sem_unlock(device_id);

    return ret;
}

/**
 * @brief   SD card remove function.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sd_card_remove(INT32U device_id)
{
	INT32S ret;

    DBG_PRINT("drvl2_sd_card_remove\r\n");

	drvl2_sd_sem_lock(device_id);
	DBG_PRINT("drvl2_sd_card_remove pass lock\r\n");

	drvl2_sd_flush_base(device_id);
	ret = drvl2_sd_card_remove_base(device_id);

	#if (SD_DEBUG_ENFORCE_STOP > 0)
	//drvl2_sd_enforce_type_summary_print(device_id);
	#endif
	drvl2_sd_sem_unlock(device_id);

	return ret;
}

#ifdef SDC_DETECT_PIN

INT32S sdc_detect_pin_status_get(void)
{
	INT32U retStatus;

	retStatus = gpio_read_io(SDC_DETECT_PIN);
	if(retStatus == SDC_DETECT_PIN_ACTIVE)
	{
		return 0;  // Plug In
	}
	else
	{
		return 1;  // Plug Out
	}
}

#endif

/**
 * @brief   Check card detect pin or issue CMD13 to probe card existing or not.
 * @param   device_id[in]: device id.
 * @return 	0: success, -1: fail.
*/
INT32S drvl2_sdc_live_response(INT32U device_id)
{
	INT32U response;

	#ifdef SDC_DETECT_PIN
		if (sdc_detect_pin_status_get() == 0)
		{
			// Card exist
			return 0;
		}
		else
		{
			// Card not exist
			return -1;
		}
	#else
		drvl2_sd_sem_lock(device_id);
        drvl2_sd_flush_base(device_id);
		if (drvl2_sd_bcr_ac_command_set(device_id, C_SD_CMD13, sd_card_rca[device_id], C_RESP_R1, &response) == 0)
		{
			// Card exist
			drvl2_sd_sem_unlock(device_id);
			return 0;
		}
		else
		{
			// Card not exist
			drvl2_sd_sem_unlock(device_id);
			return -1;
		}
	#endif


}

/**
 * @brief   SD clock enable/disable.
 * @param   device_id[in]: device id.
 * @param   enable[in]: enable/disable clock.
 * @return 	0: success, -1: fail.
*/
void drvl2_sdc_clock_gate_enable(INT32U device_id, INT8U enable)
{
    if(device_id == 1) {
        drv_l1_clock_set_system_clk_en(CLK_EN4_SD2, enable&0x1);
        drv_l1_clock_set_system_clk_en(CLK_EN4_SD2_REG_BANK, enable&0x1);
    } else {
        drv_l1_clock_set_system_clk_en(CLK_EN4_SD0, enable&0x1);
        drv_l1_clock_set_system_clk_en(CLK_EN3_SD0_REG_BANK, enable&0x1);
    }
}

void drvl2_sdc_get_version(void)
{
    DBG_PRINT("L2_SD_SVN_VERSION: %s\r\n", L2_SD_SVN_VERSION);
    DBG_PRINT("L2_SD_LIB_BUILD_TIME: %s\r\n", L2_SD_LIB_BUILD_TIME);

    #if GIT_VERSION == 1
    DBG_PRINT("L2_SD_LIB_COMMIT_TIME: %s\r\n", L2_SD_LIB_COMMIT_TIME);
    DBG_PRINT("L2_SD_LIB_BRANCH: %s\r\n", L2_SD_LIB_BRANCH);
    #endif
}

/**
 * @brief   Assign voltage get call back function.
 * @param   device_id[in]: device id.
 * @param   user_voltage_get_proc[in]: get voltage call back function pointer.
 * @return 	0:success/-1:fail.
*/
INT32S drvl2_sd_voltage_get_callback_set(INT32U device_id, sd_voltage_get_callback user_voltage_get_proc)
{
    if (device_id > 1)
        return -1;

    sd_voltage_get_proc[device_id] = user_voltage_get_proc;

    return 0;
}

/**
 * @brief   Get voltage get call back function.
 * @param   device_id[in]: device id.
 * @return 	function pointer:success/NULL:none.
*/
sd_voltage_get_callback drvl2_sd_voltage_get_callback_get(INT32U device_id)
{
    if (device_id > 1)
        return NULL;

    return sd_voltage_get_proc[device_id];
}

/**
 * @brief   Assign initial clock rate. If call, call it before calling drvl2_sd_init, and sd in idle state
 * @param   device_id[in]: device id.
 * @param   initial_clock_rate[in]: initial clock rate
 * @return 	0:success/-1:fail.
*/
INT32S drvl2_sd_set_initial_clock_rate(INT32U device_id, INT32U initial_clock_rate)
{
    if (device_id > 1)
        return -1;

    if (initial_clock_rate > 100000000) {
        DBG_PRINT("illegal sd30 clock_rate %u, could not over 100MHz, reset to 100MHz\r\n", initial_clock_rate);
        initial_clock_rate = 100000000;
    }

    sd_card_initial_clock_rate[device_id] = initial_clock_rate;

    return 0;
}

/**
 * @brief   Get bus width of sd. If call, call it after drvl2_sd_init
 * @param   device_id[in]: device id.
 * @return 	1:1 bit/4: 4 bit/-1:fail.
*/
INT32S drvl2_sd_data_bus_width_get(INT32U device_id)
{
    INT8U data_bus_width;

    if (device_id > 1)
        return -1;

    data_bus_width = sd_data_bus_width[device_id];
    return data_bus_width;
}

void drvl2_sd_card_voloume_set(INT32U device_id, INT64U value)
{
	sd_card_volume[device_id] = value;
}

INT64U drvl2_sd_card_voloume_get(INT32U device_id)
{
	return sd_card_volume[device_id];
}
#endif // _DRV_L2_SDC


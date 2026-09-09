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
#include "drv_l1_system_fifo.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define SYS_FIFO_IRAM_MASK		0x01000000

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT8U flag_sys_fifo_used[SYS_FIFO_MAX] = {0};

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
INT32S
drv_l1_system_fifo_register(
	SYS_FIFO_ENUM fifo_num
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);
	if(fifo_num >= SYS_FIFO_MAX)
	{
		ERR_PRINT("[REG] %d is over than max\r\n",fifo_num);
		return STATUS_FAIL;
	}

	if(flag_sys_fifo_used[fifo_num])
	{
		ERR_PRINT("The %d is currently in use\r\n",fifo_num);
		return STATUS_FAIL;
	}

	flag_sys_fifo_used[fifo_num] = 1;

	return STATUS_OK;
}

INT32S
drv_l1_system_fifo_release(
	SYS_FIFO_ENUM fifo_num
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);
	if(fifo_num >= SYS_FIFO_MAX)
	{
		ERR_PRINT("[RLS] %d is over than max\r\n",fifo_num);
		return STATUS_FAIL;
	}

	if(flag_sys_fifo_used[fifo_num] == 0)
	{
		ERR_PRINT("The %d is not in use\r\n",fifo_num);
		return STATUS_FAIL;
	}

	flag_sys_fifo_used[fifo_num] = 0;

	return STATUS_OK;
}

/**
 * @brief   set buffer using system fifo
 * @param   buffer: buffer which using system fifo
 * @return 	buffer with system fifo header for hardware
 */
INT32U
drv_l1_system_fifo_mapping(
	SYS_FIFO_ENUM fifo_num,
	INT32U buffer
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(fifo_num >= SYS_FIFO_MAX)
	{
		ERR_PRINT("Over SYS_FIFI_MAX\r\n");
		return buffer;
	}

	if(flag_sys_fifo_used[fifo_num] == 0)
	{
		ERR_PRINT("Illegal operation\r\n");
		return buffer;
	}

	if(fifo_num < SYS_FIFO_4)
	{
		return buffer | ((fifo_num+3) << 28);
	}
	else
	{
		return buffer | SYS_FIFO_IRAM_MASK;
	}

 }

/**
 * @brief   masked system fifo header of buffer
 * @param   buffer: buffer which using system fifo
 * @return 	buffer removed system fifo header
 */
INT32U
drv_l1_system_fifo_unmapping(
	SYS_FIFO_ENUM fifo_num,
	INT32U buffer
)
{
	//DBG_PRINT("==Run:%s==\r\n",__FUNCTION__);

	if(fifo_num >= SYS_FIFO_MAX)
	{
		ERR_PRINT("Over SYS_FIFI_MAX\r\n");
		return buffer;
	}

	if(flag_sys_fifo_used[fifo_num] == 0)
	{
		//ERR_PRINT("Illegal operation\r\n");
	}

	if(fifo_num < SYS_FIFO_4)
	{
		return buffer & ~((fifo_num+3) << 28);
	}
	else
	{
		return buffer & ~SYS_FIFO_IRAM_MASK;
	}

}

// For Debug
void
drv_l1_system_fifo_used_get(
	void
)
{
	INT8U i;

	DBG_PRINT("\r\n");
	for(i=0; i<SYS_FIFO_MAX; i++)
	{
		if(flag_sys_fifo_used[i] == 0)
		{
			DBG_PRINT("SYS FIFO%d is not used");
		}
		else
		{
			DBG_PRINT("SYS FIFO%d is used");
		}
	}
	DBG_PRINT("\r\n");
}


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

#include "drv_l1_timer.h"
#include "drv_l1_sdc.h"
#include "drv_l1_dma.h"
#include "drv_l1_keychange.h"
#include <stdio.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/

#define COMMAND_TIMEOUT 	700000	/* Typically 64 clock cycle */
#define DATA_TIMEOUT		700000	/* Typically 250ms for SDHC, 500ms for SDXC */

/**************************************************************************
 *                              M A C R O S                               *
**************************************************************************/

#if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
	static inline void start_counter( INT32U *start)
	{
		*start = tiny_counter_get();
	}
	static inline INT32S check_counter_timeout( INT32U *start, INT32U *timeout)
	{
		INT32U now = tiny_counter_get();
		if(now != *start)
		{
			(*timeout) --;
			*start = now;
		}
		return (*timeout==0)? -1 :0;
	}
#else
	static inline void start_counter( INT32U *start)
	{
		*start = 0;
	}
	static inline INT32S check_counter_timeout( INT32U* start, INT32U *timeout)
	{
		(*start) ++;
		return (*start > (*timeout<<8))? -1 :0;
	}
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
**************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/

extern void print_string(CHAR *fmt, ...);
extern INT32S drv_l1_dma_transfer_sdio(DMA_STRUCT *dma_struct, INT8U usage);

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
**************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
**************************************************************************/

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
**************************************************************************/

/**
* @brief 		SD polling stataus function when send command.
* @param 		device_id[in]: Index of SD controller.
* @param 		pollbit[in]: Polling status bit.
* @param 		buf[in]: Data buffer pointer.
* @param 		ln[in]: Data buffer length.
* @param 		xfered[in]: Data transferred.
* @return		0: success, -1: timeout or status fail.
*/
static INT32S drvl1_sdc_status_wait(
	INT32U device_id,
	INT32U pollbit,
	INT32U *buf,
	INT32U ln,
	INT32U *xfered)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	INT32U start, timeout = COMMAND_TIMEOUT;

	start_counter(&start);
	do
	{
#ifndef WIFI_DEMO
		if( (psdReg->Status & C_SDC_STATUS_DATA_BUF_FULL) && buf && ln && xfered)
		{
			if( *xfered < ln )
			{
				buf[ (*xfered)>>2] = psdReg->DataRx;
				(*xfered) += 4;
			}
			else
				DBG_PRINT("cmd SD-%d out of buffer size\r\n", device_id);
		}
#endif
		/* ----- SD status error ----- */
		if (psdReg->Status & (C_SDC_STATUS_RESP_INDEX_ERR|C_SDC_STATUS_RESP_CRC_ERR|C_SDC_STATUS_TIMEOUT))
		{
			DBG_PRINT("cmd SD-%d, CMD = 0x%x, STATUS = 0x%x, poll = 0x%x\r\n", device_id, psdReg->Cmd, psdReg->Status, pollbit );
			return -1;
		}
		/* ----- Timeout ----- */
		if(check_counter_timeout(&start, &timeout) < 0 )
		{
			DBG_PRINT("cmd SD-%d timeout, CMD = 0x%x, STATUS = 0x%x, poll = 0x%x\r\n", device_id, psdReg->Cmd, psdReg->Status, pollbit );
			return -1;
		}
	}while((psdReg->Status & pollbit)!=pollbit);

	return 0;
}
#ifndef WIFI_DEMO
/**
* @brief 		SD polling stataus function when send data.
* @param 		device_id[in]: Index of SD controller.
* @param 		pollbit[in]: Polling status bit.
* @return		0: success, -1: timeout or status fail.
*/
static INT32S drvl1_sdc_status_wait_data(                        // sam
	INT32U device_id,
	INT32U pollbit)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	INT32U start, timeout = DATA_TIMEOUT;

 	start_counter(&start);
	do
	{
		/* ----- SD status error ----- */
		if (psdReg->Status & C_SDC_STATUS_DATA_CRC_ERR)
		{
			DBG_PRINT("data SD-%d CRC error, CMD = 0x%x, STATUS = 0x%x, poll = 0x%x\r\n", device_id, psdReg->Cmd, psdReg->Status, pollbit );
			return -1;
		}
		/* ----- Timeout ----- */
		if(check_counter_timeout(&start, &timeout) < 0 )
		{
			DBG_PRINT("data SD-%d timeout, CMD = 0x%x, STATUS = 0x%x, poll = 0x%x\r\n", device_id, psdReg->Cmd, psdReg->Status, pollbit );
			return -1;
		}
	} while((psdReg->Status & pollbit)!=pollbit);

	return 0;
}

/**
* @brief 	Wait for data complete (only for write).
* @param 	device_id[in]: Index of SD controller.
* @return	0: success, -1: timeout or status fail.
*/
static INT32S drvl1_sdc_data_complete_wait_ext (
	INT32U device_id)
{
	/* ----- Wait for data empty and complete ----- */
	if(drvl1_sdc_status_wait_data(device_id, C_SDC_STATUS_DATA_BUF_EMPTY|C_SDC_STATUS_DATA_COMPLETE) < 0)
		return -1;

	/* ----- Wait for card in idle state ----- */
	if(drvl1_sdc_controller_busy_wait(device_id, DATA_TIMEOUT)<0)
		return -1;

	return 0;
}

/**
* @brief 	Tx Rx SD data.
* @param 	device_id[in]: Index of SD controller.
* @param 	data[in]: Data informaion.
* @return 	0: success, -1: timeout or status fail.
*/
INT32S drvl1_sdc_data_tx_rx (
	INT32U device_id,
	sd_l1_data_t *data)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	INT32U ln = data->blksz * data->blocks - data->bytes_xfered ;
	INT32U *buf = data->buf + (data->bytes_xfered>>2);
	/* ----- Write operation ----- */
	if( data->dir )
	{
		/* ----- Fill data to SD controller data port ----- */
		while(ln)
		{
			if(drvl1_sdc_status_wait_data(device_id, C_SDC_STATUS_DATA_BUF_EMPTY) < 0)
			{
				return -1;
			}
			psdReg->DataTx = *buf++;
			ln -= 4;
		}
		/* ----- Wait for data empty ----- */
		if(drvl1_sdc_data_complete_wait_ext(device_id)<0)
		{
			return -1;
		}

		psdReg->Status = C_SDC_STATUS_DATA_COMPLETE;
		return 0;
	}
	/* ----- Read operation ----- */
	else
	{
		while(ln)
		{
			if(drvl1_sdc_status_wait_data(device_id, C_SDC_STATUS_DATA_BUF_FULL) <0)
			{
				return -1;
			}
			*buf++ = psdReg->DataRx;
			ln -= 4;
		}
		/* ----- Wait for data complete ----- */
		if(drvl1_sdc_status_wait_data(device_id, C_SDC_STATUS_DATA_COMPLETE) < 0)
			return -1;
		psdReg->Status = C_SDC_STATUS_DATA_COMPLETE;
		return 0;
	}
}
#else
/**
* @brief 		SD polling stataus function when send data.
* @param 		device_id[in]: Index of SD controller.
* @param 		pollbit[in]: Polling status bit.
* @return		0: success, -1: timeout or status fail.
*/
static INT32S drvl1_sdc_status_wait_data(                        // sam
	INT32U device_id,
	INT32U pollbit,
	INT8U check_data_crc_error)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	INT32U start, timeout = DATA_TIMEOUT;

 	start_counter(&start);
	do
	{
		/* ----- SD status error ----- */
		if (check_data_crc_error)
		{
            // check CRC only when not poll for data empty, ssv happen CRC error whe tx 4 byte by io port
            if (psdReg->Status & C_SDC_STATUS_DATA_CRC_ERR)
            {
                DBG_PRINT("data SD-%d CRC error, CMD = 0x%x, STATUS = 0x%x, poll = 0x%x\r\n", device_id, psdReg->Cmd, psdReg->Status, pollbit );
                return -1;
            }
        }
		/* ----- Timeout ----- */
		if(check_counter_timeout(&start, &timeout) < 0 )
		{
			DBG_PRINT("data SD-%d timeout, CMD = 0x%x, STATUS = 0x%x, poll = 0x%x\r\n", device_id, psdReg->Cmd, psdReg->Status, pollbit );
			return -1;
		}
	} while((psdReg->Status & pollbit)!=pollbit);

	return 0;
}

/**
* @brief 	Tx Rx SD data.
* @param 	device_id[in]: Index of SD controller.
* @param 	data[in]: Data informaion.
* @return 	0: success, -1: timeout or status fail.
*/
INT32S drvl1_sdc_data_tx_rx (
	INT32U device_id,
	sd_l1_data_t *data)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	INT32U ln = data->blksz * data->blocks - data->bytes_xfered ;
	INT32U *buf = data->buf + (data->bytes_xfered>>2);
	INT8U check_data_crc_error;

	/* ----- Write operation ----- */
	if( data->dir )
	{
        check_data_crc_error = 0; // off CRC check, because ssv happen CRC error whe tx 4 byte by io port

		/* ----- Fill data to SD controller data port ----- */
		while(ln)
		{
			if(drvl1_sdc_status_wait_data(device_id, C_SDC_STATUS_DATA_BUF_EMPTY, check_data_crc_error) < 0)
			{
				return -1;
			}
			psdReg->DataTx = *buf++;
			if (ln>=4) {
                ln -= 4;
            } else {
                ln = 0;
            }
		}
		/* ----- Wait for data empty ----- */
		// williamyeo
		if(drvl1_sdc_status_wait_data(device_id, C_SDC_STATUS_DATA_BUF_EMPTY,check_data_crc_error) < 0)
        {
            return -2;
        }

        if (drvl1_sdc_status_set_wait(device_id, C_SDC_STATUS_DATA_COMPLETE, DATA_TIMEOUT)) {
			return -1;
		}

		return 0;
	}
	/* ----- Read operation ----- */
	else
	{
        check_data_crc_error = 1;

		while(ln)
		{
			if(drvl1_sdc_status_wait_data(device_id, C_SDC_STATUS_DATA_BUF_FULL, check_data_crc_error) <0)
			{
				return -4;
			}
			*buf++ = psdReg->DataRx;
			if (ln>=4) {
                ln -= 4;
            } else {
                ln = 0;
            }
		}
		// williamyeo
		if(drvl1_sdc_status_wait_data(device_id, C_SDC_STATUS_DATA_BUF_EMPTY,check_data_crc_error) < 0)
        {
            return -5;
        }
        if (drvl1_sdc_data_crc_status_get(device_id))
        {

            return -6;
        }

		return 0;
	}
}
#endif

/**
* @brief 	Read SD data by dma start.
* @param 	device_id[in]: Index of SD controller.
* @param 	data[in]: Data informaion.
* @return 	0: success, -1: timeout or status fail.
*/
INT32S drvl1_sdc_read_data_by_dma_start(
	INT32U device_id,
	sd_l1_data_t *data)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	DMA_STRUCT dma_struct = {0};
	INT32U ln = data->blksz * data->blocks;

	dma_struct.s_addr = (INT32U) &psdReg->DataRx;
	dma_struct.t_addr = (INT32U) data->buf;
	dma_struct.width = DMA_DATA_WIDTH_4BYTE;
	dma_struct.count = ln>>2;
	dma_struct.timeout = 255;

	data->dma_notify = C_DMA_STATUS_WAITING;
	dma_struct.notify = (INT8S *)&data->dma_notify;
#if _OPERATING_SYSTEM != _OS_NONE

  #if _OPERATING_SYSTEM == _OS_UCOS2
	OSQFlush(data->dma_q);
	return drv_l1_dma_transfer_with_queue(&dma_struct, data->dma_q);
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
        xQueueReset(data->dma_q);
	return drv_l1_dma_transfer_with_queue(&dma_struct, data->dma_q);
  #endif
#else
	return drv_l1_dma_transfer(&dma_struct);
#endif
}

#ifdef WIFI_DEMO
/**
* @brief 	Read SDIO data by dma start.
* @param 	device_id[in]: Index of SD controller.
* @param 	data[in]: Data informaion.
* @return 	0: success, -1: timeout or status fail.
*/
INT32S drvl1_sdio_read_data_by_dma_start(
	INT32U device_id,
	sd_l1_data_t *data, INT32U len)
{
	volatile sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	DMA_STRUCT dma_struct = {0};

	dma_struct.s_addr = (INT32U) &psdReg->DataRx;
	dma_struct.t_addr = (INT32U) data->buf;
	if(data->blksz == 2){
        dma_struct.width = DMA_DATA_WIDTH_2BYTE;
        dma_struct.count = len>>1;
	}
	else{
        dma_struct.width = DMA_DATA_WIDTH_4BYTE;
        dma_struct.count = len>>2;
	}
	dma_struct.timeout = 255;

	data->dma_notify = C_DMA_STATUS_WAITING;
	dma_struct.notify = (INT8S *)&data->dma_notify;
#if _OPERATING_SYSTEM != _OS_NONE

  #if _OPERATING_SYSTEM == _OS_UCOS2
	OSQFlush(data->dma_q);
	return drv_l1_dma_transfer_with_queue(&dma_struct, data->dma_q);
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
    //xQueueReset(data->dma_q);
	//return drv_l1_dma_transfer_with_queue(&dma_struct, data->dma_q);
	//return drv_l1_dma_transfer(&dma_struct);
    return drv_l1_dma_transfer_sdio(&dma_struct, C_DMA_NORMAL_USED);
  #endif
#else
	return drv_l1_dma_transfer(&dma_struct);
#endif
}

/**
* @brief 	Read SDIO data by dma wait ready.
* @param 	device_id[in]: Index of SD controller.
* @param 	data[in]: Data informaion.
* @return 	0: not ready; -1: dma timeout; 1: dma finish.
*/
INT32S drvl1_sdio_read_data_by_dma_check(
	INT32U device_id,
	sd_l1_data_t *data)
{
    sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
    volatile INT32U timeout = DATA_TIMEOUT;
    INT8S notify = data->dma_notify;

	if( notify == C_DMA_STATUS_DONE )
	{
/*
		if(drvl1_sdc_status_wait_data(device_id, C_SDC_STATUS_DATA_COMPLETE, 1) < 0)
			return -1;
*/
#if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
        volatile INT32U timer1, timer2;

        if (sd->Status & C_SDC_STATUS_DATA_COMPLETE) {
            return (INT32S)notify;
        }

        timer1 = tiny_counter_get();
        while (timeout) {
            if (sd->Status & C_SDC_STATUS_DATA_COMPLETE) {
                //DBG_PRINT("drvl1_sdc_status_set_wait success=%u, pollbit=0x%0x, timeout=%u, Status=0x%08x\r\n", in_timeout, pollbit, timeout, sd->Status);
                return (INT32S)notify;
            }
            timer2 = tiny_counter_get();
            if (timer1 != timer2) {
                timer1 = timer2;
                timeout--;
            }
        }
//      DBG_PRINT("drvl1_sdc_status_set_wait timeout=%u, pollbit=0x%0x, Status=0x%08x\r\n", in_timeout, pollbit, sd->Status);
        return -1;
#else
	INT32U i;

	i = 0;
	while ((sd->Status & C_SDC_STATUS_DATA_COMPLETE) == 0)
	{
		i++;
		if (i > (timeout<<8)) {
			return -1;
		}
	}
#endif
	}
	return (INT32S)notify;
}
#endif

/**
* @brief 	Read SD data by dma wait ready.
* @param 	device_id[in]: Index of SD controller.
* @param 	data[in]: Data informaion.
* @return 	0: not ready; -1: dma timeout; 1: dma finish.
*/
INT32S drvl1_sdc_read_data_by_dma_check(
	INT32U device_id,
	sd_l1_data_t *data)
{
	INT8S notify = data->dma_notify;

	if( notify == C_DMA_STATUS_DONE )
	{
#ifndef WIFI_DEMO
		if(drvl1_sdc_status_wait_data(device_id, C_SDC_STATUS_DATA_COMPLETE) < 0)
#else
		if(drvl1_sdc_status_wait_data(device_id, C_SDC_STATUS_DATA_COMPLETE,1) < 0)
#endif
			return -1;
		return 1;
	}

	return notify;
}

/**
* @brief 	Write SD data by dma start.
* @param 	device_id[in]: Index of SD controller.
* @param 	data[in]: Data informaion.
* @return 	0: success, -1: timeout or status fail.
*/
INT32S drvl1_sdc_write_data_by_dma_start(
	INT32U device_id,
	sd_l1_data_t *data)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	DMA_STRUCT dma_struct = {0};
	INT32U ln = data->blksz * data->blocks;

	dma_struct.s_addr = (INT32U) data->buf;
	dma_struct.t_addr = (INT32U) &psdReg->DataTx;
	dma_struct.width = DMA_DATA_WIDTH_4BYTE;
	dma_struct.count = ln>>2;
	dma_struct.timeout = 255;

	data->dma_notify = C_DMA_STATUS_WAITING;
	dma_struct.notify = (INT8S *)&data->dma_notify;
#if _OPERATING_SYSTEM != _OS_NONE
  #if _OPERATING_SYSTEM == _OS_UCOS2
	OSQFlush(data->dma_q);
	return drv_l1_dma_transfer_with_queue(&dma_struct, data->dma_q);
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
        xQueueReset(data->dma_q);
	return drv_l1_dma_transfer_with_queue(&dma_struct, data->dma_q);
  #endif
#else
	return drv_l1_dma_transfer(&dma_struct);
#endif
}

#ifdef WIFI_DEMO
/**
* @brief 	Write SDIO data by dma start.
* @param 	device_id[in]: Index of SD controller.
* @param 	data[in]: Data informaion.
* @return 	0: success, -1: timeout or status fail.
*/
INT32S drvl1_sdio_write_data_by_dma_start(
	INT32U device_id,
	sd_l1_data_t *data, INT32U len)
{
	volatile sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	DMA_STRUCT dma_struct = {0};
	dma_struct.s_addr = (INT32U) data->buf;
	dma_struct.t_addr = (INT32U) &psdReg->DataTx;
	if(data->blksz == 2){
        dma_struct.width = DMA_DATA_WIDTH_2BYTE;
        dma_struct.count = len>>1;
	}
	else{
        dma_struct.width = DMA_DATA_WIDTH_4BYTE;
        dma_struct.count = len>>2;//ln>>2;
	}
	dma_struct.timeout = 255;

	data->dma_notify = C_DMA_STATUS_WAITING;
	dma_struct.notify = (INT8S *)&data->dma_notify;
#if _OPERATING_SYSTEM != _OS_NONE
  #if _OPERATING_SYSTEM == _OS_UCOS2
	OSQFlush(data->dma_q);
	return drv_l1_dma_transfer_with_queue(&dma_struct, data->dma_q);
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
    //xQueueReset(data->dma_q);
	//return drv_l1_dma_transfer_with_queue(&dma_struct, data->dma_q);
	//return drv_l1_dma_transfer(&dma_struct);
    return drv_l1_dma_transfer_sdio(&dma_struct, C_DMA_NORMAL_USED);
  #endif
#else
	return drv_l1_dma_transfer(&dma_struct);
#endif
}

/**
* @brief 	Write SDIO data by dma wait ready.
* @param 	device_id[in]: Index of SD controller.
* @param 	data[in]: Data informaion.
* @return 	0: not ready; -1: dma timeout; 1: dma finish.
*/
INT32S drvl1_sdio_write_data_by_dma_check(
	INT32U device_id,
	sd_l1_data_t *data)
{
    sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);
    volatile INT32U timeout = DATA_TIMEOUT;
    INT8S notify = data->dma_notify;

	if( notify == C_DMA_STATUS_DONE )
	{
/*
		if( drvl1_sdc_data_complete_bit_wait(device_id, DATA_TIMEOUT) < 0 )
			return -1;
*/
#if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
        volatile INT32U timer1, timer2;

        if (sd->Status & C_SDC_STATUS_DATA_COMPLETE) {
            return (INT32S)notify;
        }

        timer1 = tiny_counter_get();
        while (timeout) {
            if (sd->Status & C_SDC_STATUS_DATA_COMPLETE) {
                //DBG_PRINT("drvl1_sdc_status_set_wait success=%u, pollbit=0x%0x, timeout=%u, Status=0x%08x\r\n", in_timeout, pollbit, timeout, sd->Status);
                return (INT32S)notify;
            }
            timer2 = tiny_counter_get();
            if (timer1 != timer2) {
                timer1 = timer2;
                timeout--;
            }
        }
//      DBG_PRINT("drvl1_sdc_status_set_wait timeout=%u, pollbit=0x%0x, Status=0x%08x\r\n", in_timeout, pollbit, sd->Status);
        return -1;
#else
	INT32U i;

	i = 0;
	while ((sd->Status & C_SDC_STATUS_DATA_COMPLETE) == 0)
	{
		i++;
		if (i > (timeout<<8)) {
			return -1;
		}
	}

#endif
	}
        return (INT32S)notify;
}
#endif

/**
* @brief 	Write SD data by dma wait ready.
* @param 	device_id[in]: Index of SD controller.
* @param 	data[in]: Data informaion.
* @return 	0: not ready; -1: dma timeout; 1: dma finish.
*/
INT32S drvl1_sdc_write_data_by_dma_check(
	INT32U device_id,
	sd_l1_data_t *data)
{
	INT8S notify = data->dma_notify;

	if( notify == C_DMA_STATUS_DONE )
	{
		if( drvl1_sdc_data_complete_wait(device_id, DATA_TIMEOUT) < 0 )
			return -1;
		return 1;
	}

	return notify;
}

/**
* @brief 	Set SD controller cmd register and response length.
* @param 	ctrl[in]: Controller control information.
* @param 	cmd[out]: Cmd register.
* @param 	rln[out]: Response length.
* @return 	None.
*/
static void drvl1_sdc_set_command_register(
	sd_l1_ctrl_t *ctrl,
	INT32U *cmd,
	INT32U *rln)
{
	*cmd = 0;
	/* ----- Set Response type ----- */
	switch( ctrl->resp_type )
	{
		case SDC_RESPTYPE_NONE:
			*rln =0;
			break;
		case SDC_RESPTYPE_R2:
			*cmd |= C_SDC_CMD_RESP_R2;
			*rln = 4;
			break;
		case SDC_RESPTYPE_R3:
		case SDC_RESPTYPE_R4:
			*cmd |= C_SDC_CMD_RESP_R3;
			*rln = 1;
			break;
		case SDC_RESPTYPE_R6:
			*cmd |= C_SDC_CMD_RESP_R6;
			*rln = 1;
			break;
		case SDC_RESPTYPE_R1b:
			*cmd |= C_SDC_CMD_RESP_R1B;
			*rln = 1;
			break;
		case SDC_RESPTYPE_R1:
		case SDC_RESPTYPE_R5:
		case SDC_RESPTYPE_R7:
		default:
			*cmd |= C_SDC_CMD_RESP_R1;
			*rln = 1;
			break;
	}
	/* ----- Set data transfer ----- */
	if( ctrl->data && ctrl->data->buf &&  ctrl->data->blocks )
	{
		*cmd |= (C_SDC_CMD_WITH_DATA|C_SDC_CMD_MULTI_BLOCK) ;

		if( ctrl->data->dir == SDC_WRITE)
			*cmd |= C_SDC_CMD_WRITE_DATA;
	}

	*cmd |= C_SDC_CMD_RUN | ( ctrl->cmd & 0x3f);
}

/**
* @brief 	Send SD command and received response.
* @param 	ctrl[in]: Controller control information.
* @return 	0: success, -1: timeout or status fail.
*/
static INT32S drvl1_sdc_command_send_ext(
	sd_l1_ctrl_t *ctrl
)
{
	INT32U rln, ln = 0;
	INT32U i;
	INT32U cmd = 0;
	sdReg_t *psdReg = (sdReg_t*) SetSDBase( ctrl->device_id );
	INT32U *buf = 0;
	INT32U *xfered = 0;

	if( (ctrl->data) && (ctrl->data->dmaen ==0 ))
	{
		buf = ctrl->data->buf;
		ln = ctrl->data->blksz * ctrl->data->blocks;
		xfered = &ctrl->data->bytes_xfered;
		*xfered = 0;
	}

	drvl1_sdc_set_command_register( ctrl, &cmd, &rln );

	psdReg->Arg = ctrl->arg;
	psdReg->Cmd = cmd;

	for( i=0 ; i<rln ; i++)
	{
		if(drvl1_sdc_status_wait( ctrl->device_id, C_SDC_STATUS_RESP_REG_FULL, buf, ln, xfered ) < 0)
			return -1;
		ctrl->resp[i] = psdReg->Resp;
	}

	if ( drvl1_sdc_status_wait( ctrl->device_id, C_SDC_STATUS_CMD_COMPLETE, buf, ln, xfered ) < 0)
		return -1;

	return 0;
}

#ifndef WIFI_DEMO
/**
* @brief 	Send SD command.
* @param 	ctrl[in]: Controller control information.
* @return 	0: success, -1: timeout or status fail.
*/
INT32S drvl1_sdc_command_issue(
	sd_l1_ctrl_t *ctrl)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase( ctrl->device_id );
	INT32U ln = 0;
	INT32S ret = 0;

	psdReg->Status = C_SDC_STATUS_CLEAR_ALL;
	/* ----- data must 4 byte alignment ----- */
	if( ctrl->data )
	{
		ln =  ctrl->data->blksz * ctrl->data->blocks;
		if(((INT32U)ctrl->data->buf&0x03)||(ln&0x03))
			return -1;
		/* ----- Set block size ----- */
		drvl1_sdc_block_len_set( ctrl->device_id, ctrl->data->blksz );
		/* ------ Enable data dma before send command ----- */
		if( ctrl->data_separate==0 && ctrl->data->dmaen )
		{
			if( ctrl->data->dir )
				ret = drvl1_sdc_write_data_by_dma_start( ctrl->device_id, ctrl->data );
			else
				ret = drvl1_sdc_read_data_by_dma_start( ctrl->device_id, ctrl->data );
		}
		if( ret<0 )
			return ret;
	}
	/* ----- Send command ----- */
	ret = drvl1_sdc_command_send_ext(ctrl);
	if(ctrl->data_separate)
		return ret;
	/* ----- Data transmission state ----- */
	if( ctrl->data && ctrl->data_separate==0 )
	{
		if( ctrl->data->dir == SDC_WRITE )
		{
			if( ctrl->data->dmaen == 0 )
			{
				ret &= drvl1_sdc_data_tx_rx( ctrl->device_id, ctrl->data );
			}
			else
			{
			#if _OPERATING_SYSTEM != _OS_NONE
				INT8U err;
                            #if _OPERATING_SYSTEM == _OS_UCOS2
				OSQPend( ctrl->data->dma_q, ((ln >> 9)+1) * 50, &err );
                            #elif _OPERATING_SYSTEM == _OS_FREERTOS
                                err = (INT32S) xQueueReceive(ctrl->data->dma_q, NULL, ((ln >> 9)+1) * 50);
                            #endif
                                ret &= err;
				if( drvl1_sdc_data_complete_wait(ctrl->device_id, DATA_TIMEOUT) < 0 )
					ret &= -1;
			#else
				INT32S dma_state;
				do
				{
					dma_state = drvl1_sdc_write_data_by_dma_check( ctrl->device_id, ctrl->data );
				}while( dma_state == 0);

				ret &= dma_state;
			#endif
			}
		}
		if( ctrl->data->dir == SDC_READ )
		{
			if( ctrl->data->dmaen == 0 )
			{
				ret &= drvl1_sdc_data_tx_rx( ctrl->device_id, ctrl->data );
			}
			else
			{
			#if _OPERATING_SYSTEM != _OS_NONE
				INT8U err;
                            #if _OPERATING_SYSTEM == _OS_UCOS2
				OSQPend( ctrl->data->dma_q, ((ln >> 9)+1) * 50, &err );
                            #elif _OPERATING_SYSTEM == _OS_FREERTOS
                                err = (INT32S) xQueueReceive(ctrl->data->dma_q, NULL, ((ln >> 9)+1) * 50);
                            #endif
				ret &= err;
				if(drvl1_sdc_status_wait_data(ctrl->device_id, C_SDC_STATUS_DATA_COMPLETE) < 0)
					ret &= -1;
			#else
				INT32S dma_state;
				do
				{
					dma_state = drvl1_sdc_read_data_by_dma_check( ctrl->device_id, ctrl->data );
				}while( dma_state == 0);

				ret &= dma_state;
			#endif
			}
		}
		/* ----- Set SD controller to idle state ----- */
		ret &= drvl1_sdc_stop_controller( ctrl->device_id, DATA_TIMEOUT );
	}
	return ret;
}
#else
/**
* @brief 	Send SD command.
* @param 	ctrl[in]: Controller control information.
* @return 	0: success, -1: timeout or status fail.
*/
INT32S drvl1_sdc_command_issue(
	sd_l1_ctrl_t *ctrl)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase( ctrl->device_id );
	INT32U ln = 0;
	INT32S ret = 0;
    INT32S temp_ret;
    INT32S crc_check;
	psdReg->Status = C_SDC_STATUS_CLEAR_ALL;
	/* ----- data must 4 byte alignment ----- */
	if( ctrl->data )
	{
		ln =  ctrl->data->blksz * ctrl->data->blocks;
		if(((INT32U)ctrl->data->buf&0x03)/*||(ln&0x03)*/)
		{
            printf("drvl1_sdc_command_issue buf not wordaligned 0x%p\r\n", ctrl->data->buf);
            printf("drvl1_sdc_command_issue buf not wordaligned 0x%p\r\n", ctrl->data->buf);
            printf("drvl1_sdc_command_issue buf not wordaligned 0x%p\r\n", ctrl->data->buf);
		    return -1;
        }
		/* ----- Set block size ----- */
		drvl1_sdc_block_len_set( ctrl->device_id, ctrl->data->blksz );
		/* ------ Enable data dma before send command ----- */
		if( ctrl->data_separate==0 && ctrl->data->dmaen )
		{
			if( ctrl->data->dir )
				ret = drvl1_sdio_write_data_by_dma_start( ctrl->device_id, ctrl->data, ln);
			else
                ret = drvl1_sdio_read_data_by_dma_start( ctrl->device_id, ctrl->data, ln);
                if (ret != 0)
                    printf("sdio_write_data_by_dma_start fail. ret=%d dir=%d id=%d\r\n", ret, ctrl->data->dir, ctrl->device_id);
		}
		if( ret<0 )
			return ret;
	}
	/* ----- Send command ----- */
	ret = drvl1_sdc_command_send_ext(ctrl);
    if (ret != 0)
	{
	    if (ctrl->data == NULL) // williamyeo
	    {
            printf("drvl1_sdc_command_send_ext fail ret=%d, data_separate=%u data=0x%p ln=%d\r\n", ret, ctrl->data_separate, ctrl->data, ln);
            drvl1_sdc_stop_controller( ctrl->device_id, DATA_TIMEOUT );
            return ret;
        }
        printf("drvl1_sdc_command_send_ext fail ret=%d, data_separate=%u data=0x%p blocks=%d blksz=%d ln=%d\r\n", ret, ctrl->data_separate, ctrl->data, ctrl->data->blocks, ctrl->data->blksz, ln);
	}

	if(ctrl->data_separate)
	{
        printf("data_separate detected\r\n");
		return ret;
    }
	/* ----- Data transmission state ----- */
	if( ctrl->data/* && ctrl->data_separate==0 */)
	{
		if( ctrl->data->dir == SDC_WRITE )
		{
			if( ctrl->data->dmaen == 0 )
			{
				temp_ret = drvl1_sdc_data_tx_rx( ctrl->device_id, ctrl->data );
				ret |= temp_ret;
				if (temp_ret != 0)
                    printf("drvl1_sdc_data_tx_rx w fail. temp_ret=%d data=0x%p blocks=%d blksz=%d ln=%d\r\n", temp_ret, ctrl->data, ctrl->data->blocks, ctrl->data->blksz, ln);
			}
			else
			{
				INT32S dma_state;
				do
				{
					dma_state = drvl1_sdio_write_data_by_dma_check( ctrl->device_id, ctrl->data );
				}while( dma_state == C_DMA_STATUS_WAITING);
				temp_ret = ((dma_state == C_DMA_STATUS_DONE) ? 0 : -2);
				ret |= temp_ret;
				if (temp_ret != 0)
                    printf("drvl1_sdio_write_data_by_dma_check fail. temp_ret=%d data=0x%p blocks=%d blksz=%d ln=%d\r\n", temp_ret, ctrl->data, ctrl->data->blocks, ctrl->data->blksz, ln);
			}
		}
		if( ctrl->data->dir == SDC_READ )
		{
			if( ctrl->data->dmaen == 0 )
			{
				temp_ret = drvl1_sdc_data_tx_rx( ctrl->device_id, ctrl->data );
				ret |= temp_ret;
				if (temp_ret != 0)
                    printf("drvl1_sdc_data_tx_rx r fail. temp_ret=%d data=0x%p blocks=%d blksz=%d ln=%d\r\n", temp_ret, ctrl->data, ctrl->data->blocks, ctrl->data->blksz, ln);
			}
			else
			{
				INT32S dma_state;
				do
				{
					dma_state = drvl1_sdio_read_data_by_dma_check( ctrl->device_id, ctrl->data );
				}while( dma_state == C_DMA_STATUS_WAITING);
				temp_ret = ((dma_state == C_DMA_STATUS_DONE) ? 0 : -3);
				ret |= temp_ret;
				if (temp_ret != 0)
                    printf("drvl1_sdio_read_data_by_dma_check fail. temp_ret=%d data=0x%p blocks=%d blksz=%d ln=%d\r\n", temp_ret, ctrl->data, ctrl->data->blocks, ctrl->data->blksz, ln);
                crc_check = drvl1_sdc_data_crc_status_get(ctrl->device_id);
                ret |= crc_check;
                if(crc_check != 0)
                    printf("CRC error \r\n");
			}
		}
		/* ----- Set SD controller to idle state ----- */
		{
            temp_ret = drvl1_sdc_stop_controller( ctrl->device_id, DATA_TIMEOUT );
            if (temp_ret != 0)
                printf("drvl1_sdc_stop_controller fail. ret=%d\r\n", temp_ret);
            ret |= temp_ret;
		}
	}
	return ret;
}

INT32S drvl1_sdio_read_data_by_dma(INT32U device_id, INT32U *buffer, INT32U blocks, INT32U blksz, INT8S *poll_status)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

#if (defined _DRV_L1_DMA) && (_DRV_L1_DMA == 1)
	DMA_STRUCT dma_struct={0};
	volatile INT8S done;

	dma_struct.s_addr = (INT32U)  &sd->DataRx;
	dma_struct.t_addr = (INT32U) buffer;
	dma_struct.width = DMA_DATA_WIDTH_4BYTE;
	dma_struct.count = (INT32U) (blksz/DMA_DATA_WIDTH_4BYTE) * blocks;//(INT32U)((blksz * blocks)/DMA_DATA_WIDTH_4BYTE);
	if (((blksz * blocks)/512) < 256) {
		dma_struct.timeout = 64;
	} else if (((blksz * blocks)/512) < 1000) {
		dma_struct.timeout = (INT8U) (((blksz * blocks)/512) >> 2);
	} else {
		dma_struct.timeout = 255;
	}

	// dma_struct.timeout = 255; // williamyeo TEST

	if (poll_status) {
		*poll_status = C_DMA_STATUS_WAITING;
		dma_struct.notify = poll_status;

	// Send dma request to driver and return immediately
		if (drv_l1_dma_transfer(&dma_struct)) {
			return -1;
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
		if (drvl1_sdc_data_get(device_id, buffer, C_SDC_L1_DATA_FULL_TIMEOUT)) {
			if (poll_status) {
				*poll_status = C_DMA_STATUS_TIMEOUT;
			}
			return -1;
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

INT32S drvl1_sdio_write_data_by_dma(INT32U device_id, INT32U *buffer, INT32U blocks, INT32U blksz, INT8S *poll_status)
{
	volatile sdReg_t *sd = (sdReg_t*) SetSDBase(device_id);

#if (defined _DRV_L1_DMA) && (_DRV_L1_DMA == 1)
	DMA_STRUCT dma_struct={0};
	volatile INT8S done;

	dma_struct.s_addr = (INT32U) buffer;
	dma_struct.t_addr = (INT32U) &sd->DataTx;
	dma_struct.width = DMA_DATA_WIDTH_4BYTE;
	dma_struct.count = (INT32U) (blksz/DMA_DATA_WIDTH_4BYTE) * blocks;//(INT32U) ((blksz * blocks)/DMA_DATA_WIDTH_4BYTE);

	if (((blksz * blocks)/512) < 256) {
		dma_struct.timeout = 64;
	} else if (((blksz * blocks)/512) < 1000) {
		dma_struct.timeout = (INT8U) (((blksz * blocks)/512) >> 2);
	} else {
		dma_struct.timeout = 255;
	}

	// dma_struct.timeout = 255; // williamyeo TEST

	if (poll_status) {
		*poll_status = C_DMA_STATUS_WAITING;
		dma_struct.notify = poll_status;

	// Send dma request to driver and return immediately
		if (drv_l1_dma_transfer(&dma_struct)) {
			return -1;
		}
	} else {
		done = C_DMA_STATUS_WAITING;
		dma_struct.notify = &done;
//DBG_PRINT("wDMAa\r\n");
		// Send dma request to driver and wait response
		if (drv_l1_dma_transfer_wait_ready(&dma_struct)) {
			return -1;
		}
//DBG_PRINT("wDMAb\r\n");
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


static INT8U sdio_config_bit[2];

INT32S drvl1_sdio_config_bit_set(INT32U device_id, INT8U nrBit)
{
    if (nrBit == 1 || nrBit == 4)
    {
        sdio_config_bit[device_id] = nrBit;
        return 0;
    }
    else
    {
        DBG_PRINT("sdio:illegal bit\r\n", nrBit);
        return -1;
    }
}

INT8U drvl1_sdio_config_bit_get(INT32U device_id)
{
    return sdio_config_bit[device_id];
}

static INT8U sdio_config_extInt[2];
static INT8U sdio_config_extMux[2];

void drvl1_sdio_config_one_bit_intr_set(INT32U device_id, INT8U extInt, INT8U extMux)
{
    sdio_config_extInt[device_id] = extInt;
    sdio_config_extMux[device_id] = extMux;
}

void drvl1_sdio_config_one_bit_intr_get(INT32U device_id, INT8U *extInt, INT8U *extMux)
{
    *extInt = sdio_config_extInt[device_id];
    *extMux = sdio_config_extMux[device_id];
}

static void (*sdio_usr_isr[2])(INT32U) = {NULL, NULL};

static void SDIO1_EXT_ISR(void)
{
    //print_string("SDIO1_EXT_ISR\r\n");
    if (sdio_usr_isr[1] != NULL)
        (*sdio_usr_isr[1])(1);
}

static void SDIO0_EXT_ISR(void)
{
    if (sdio_usr_isr[0] != NULL)
        (*sdio_usr_isr[0])(0);
}

void drvl1_sdio_install_1_bit_isr(INT32U device_id, INT8U extInt, INT8U extMux, void (*user_isr)(INT32U))
{
    sdio_usr_isr[device_id] = user_isr;
#if 0 // EXT-INT
	drv_l1_ext_int_init(extInt,0,((device_id == 0) ? SDIO0_EXT_ISR : SDIO1_EXT_ISR));
    drv_l1_ext_edge_set(extInt, FALLING);
#else
	drv_l1_keychange_user_isr_set(extInt,((device_id == 0) ? SDIO0_EXT_ISR : SDIO1_EXT_ISR));
	drv_l1_keychange_init(extInt,0,0);
	drv_l1_keychange_latch_update();
	drv_l1_keychange_interrupt_enable(extInt);
#endif
}
#endif

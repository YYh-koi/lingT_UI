/*
* Purpose: Driver layer2 for controlling SDIO cards
*
* Author: Dunker Chen
*
* Date: 2013/10/31
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.0
*/

#include <string.h>
#include "gp_stdlib.h"
#include "gplib_mm_gplus.h"
#include "drv_l1_sdc.h"
#include "drv_l2_sdc.h"
#include "include\card.h"
#include "include\drv_l2_sdio_rtk.h"
#include "drv_l1_timer.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_ext_int.h"



#if 1
/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/

#define SDIO_OCR	0x1C0000		// Operation conditions register (OCR) 3.0~3.3 V

#define SDC_TOTAL		2

#define SDIO_DMA_WAIT_STATUS    1

#define SDIO_DEFAULT_BLOCK_SIZE  512//64 //256
/**************************************************************************
 *                              M A C R O S                               *
**************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
**************************************************************************/

typedef struct gpSDCtrl_s{
	sd_l1_ctrl_t	l1_ctrl;
	sd_l1_data_t	l1_data;
	volatile INT32U	int_notify;
#if _OPERATING_SYSTEM != _OS_NONE
	void *			int_buf;
//	OS_EVENT*		int_queue;
#endif
}gpSDCtrl_t;

typedef int (tpl_parse_t)( gpSDIOInfo_t *sdio, struct sdio_func *,
			   const INT8U *, INT32U);

struct cis_tpl {
	INT8U code;
	INT8U min_size;
	tpl_parse_t *parse;
};

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/

extern void print_string(CHAR *fmt, ...);

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
**************************************************************************/
extern void sdio_host_isr(INT32U device_id);
extern INT32S drvl1_sdio_read_data_by_dma(INT32U device_id, INT32U *buffer, INT32U blocks, INT32U blksz, INT8S *poll_status);
extern INT32S drvl1_sdio_write_data_by_dma(INT32U device_id, INT32U *buffer, INT32U blocks, INT32U blksz, INT8S *poll_status);
extern INT8U drvl1_sdio_config_bit_get(INT32U device_id);
extern void drvl1_sdio_config_one_bit_intr_get(INT32U device_id, INT8U *extInt, INT8U *extMux);
extern void drvl1_sdio_install_1_bit_isr(INT32U device_id, INT8U extInt, INT8U extMux, void (*user_isr)(INT32U));
/**************************************************************************
 *                         G L O B A L    D A T A                         *
**************************************************************************/
struct sdio_func *wifi_sdio_func;

static gpSDIOInfo_t gp_sdio_info[2] = {{0}};
static gpSDCtrl_t gp_sdio_ctrl[2];
static struct mmc_host gp_sdio_host[2] ={{0}};
static u32 sdio_block_size;

#if _OPERATING_SYSTEM != _OS_NONE
#if _OPERATING_SYSTEM == _OS_UCOS2
static	OS_EVENT *gSD_sem[SDC_TOTAL] = {NULL, NULL};
#elif _OPERATING_SYSTEM == _OS_FREERTOS
static	xSemaphoreHandle gSD_sem[SDC_TOTAL] = {NULL, NULL};
#endif
#endif

/*
static SD_CARD_STATE_ENUM sd_card_state[SDC_TOTAL] = {SD_CARD_STATE_INACTIVE, SD_CARD_STATE_INACTIVE};
static INT32U sd_card_rca[SDC_TOTAL];			// 16 bits (31-16)
static INT32U sd_card_csd[SDC_TOTAL][4];		// 128 bits
static INT32U sd_card_scr[SDC_TOTAL][2];		// 64 bits
static INT32U sd_card_cid[SDC_TOTAL][4];		// 128 bits
static INT32U sd_card_ocr[SDC_TOTAL];			// 32 bits
static INT32U sd_card_total_sector[SDC_TOTAL];
static INT32U sd_card_speed[SDC_TOTAL];
static INT8U sd_card_type[SDC_TOTAL];
static INT8U sd_card_bus_width[SDC_TOTAL];
static INT8U sd_card_protect[SDC_TOTAL];
static INT8U sd_card_user[SDC_TOTAL] = {0, 0};
*/
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
**************************************************************************/
static INT32S sd_sema_count[SDC_TOTAL] = {0, 0};

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

static inline void drvl2_sd_sem_lock(INT32U device_id)
{
#if _OPERATING_SYSTEM != _OS_NONE

  #if _OPERATING_SYSTEM == _OS_UCOS2
        INT8U err = NULL;
	OSSemPend(gSD_sem[device_id], 0, &err);
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
        //BaseType_t err;
        xSemaphoreTake(gSD_sem[device_id], portMAX_DELAY);
  #endif

  if (sd_sema_count[device_id] != 0)
    DBG_PRINT("sd_sema_count not 0 before lock. %d\r\n", sd_sema_count[device_id]);
  sd_sema_count[device_id]++;

#endif
}

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
* @brief 	Get SDIO information(handle).
* @param 	device_id[in]:  Index of SD controller.
* @return 	 SDIO information.
*/
gpSDIOInfo_t* drvl2_sdio_get_info(
	INT32U device_id )
{
	return &gp_sdio_info[device_id];
}

/**
* @brief 	Set l1 driver data information.
* @param 	data[in]: Data information pointer.
* @param 	buf[in]: Data buffer pointer.
* @param 	blksz[in]: Data block size.
* @param 	blocks[in]: Number of blocks.
* @param 	dir[in]: Data direction.
* @param 	dmaen[in]: Dma enable.
* @return 	None.
*/
void drvl2_sd_set_data(
	sd_l1_data_t *data,
	INT32U *buf,
	INT32U blksz,
	INT32U blocks,
	INT32U dir,
	INT32U dmaen)
{
	data->buf = buf;
	data->blksz = blksz;
	data->blocks = blocks;
	data->dir = dir;
	data->dmaen = dmaen;
	data->bytes_xfered = 0;
}

/**
* @brief 	Send SD/MMC command with or without data.
* @param 	device_id[in]: Index of SD controller.
* @param 	cmd[in]: SD/MMC command.
* @param 	arg[in]: Argument.
* @param 	resp_type[in]: Response type.
* @param 	data_separate[in]: Data separate. If set, driver will send commmand and do not process data.
* @param 	data[in]: Data information.
* @return 	0: success, -1: timeout or status fail.
*/
INT32S drvl2_sd_send_command(
	INT32U device_id,
	INT32U cmd,
	INT32U arg,
	INT32U resp_type,
	INT32U data_separate,
	sd_l1_data_t *data)
{
	sd_l1_ctrl_t *l1_ctrl = &gp_sdio_ctrl[device_id].l1_ctrl;

	l1_ctrl->device_id = device_id;
	l1_ctrl->cmd = cmd;
	l1_ctrl->arg = arg;
	l1_ctrl->resp_type = resp_type;
	l1_ctrl->data_separate = data_separate;
	l1_ctrl->data = data;
	return drvl1_sdc_command_issue( l1_ctrl );
//	DBG_PRINT("[drvl1_sdc_command_issue]=%d\r\n", retn);
}

/**
* @brief 	Send SDIO IO_RX_DIRECT command(CMD52).
* @param 	sdio[in]: SDIO handle.
* @param 	write[in]: The direction of io operation. 0 for read, and 1 for write.
* @param 	fn[in]: Function number.
* @param 	addr[in]: Register address.
* @param 	in[in]: Register data when write operation.
* @param 	out[out]: Read back register data after read/write operation.
* @return 	0: success, -1: fail.
*/
INT32S drvl2_sdio_rw_direct(
	gpSDIOInfo_t *sdio,
	INT32U write,
	INT32U fn,
	INT32U addr,
	INT8U in,
	INT8U* out)
{
	gpSDCtrl_t *ctrl = &gp_sdio_ctrl[sdio->host->device_id];
	sd_l1_ctrl_t *l1_ctrl = &ctrl->l1_ctrl;
	INT32U arg;

	if( gp_sdio_info[sdio->host->device_id].host->present==0 ){
        DBG_PRINT("SDIO_RW_DIRECT line %d\r\n",__LINE__);
		return -1;
    }

	if ((fn >7) || (addr & ~0x1FFFF)){
        DBG_PRINT("SDIO_RW_DIRECT line %d\r\n",__LINE__);
		return -1;
    }
	/* ----- Setting argument ----- */
	arg = write ? 0x80000000 : 0x00000000;
	arg |= fn << 28;
	arg |= (write && out) ? 0x08000000 : 0x00000000;
	arg |= addr << 9;
	arg |= in;
	/* ----- Send CMD52 ----- */
	if( drvl2_sd_send_command( sdio->host->device_id, MASK_CMD52, arg, SDC_RESPTYPE_R5, 0, NULL ) < 0 ){
        DBG_PRINT("SDIO_RW_DIRECT send_command fail.arg=0x%08x w=%d fn=%d addr=0x%x in=%d out=0x%08x\r\n", arg, write, fn, addr, in, out);
		return -1;
    }
	/* ----- Check response error ----- */
	if( l1_ctrl->resp[0] & ( R5_COM_CRC_ERROR | R5_ILLEGAL_COMMAND | R5_ERROR | R5_FUNCTION_NUMBER | R5_OUT_OF_RANGE ))
	{
		DBG_PRINT("IO_RW_DIRECT error 0x%x\r\n", l1_ctrl->resp[0] );
		return -1;
	}

	if (out)
		*out = l1_ctrl->resp[0] & 0xFF;

	return 0;
}

#if SDIO_DMA_WAIT_STATUS
static volatile INT8S sdio_wdma_result[SDC_TOTAL] = {C_DMA_STATUS_DONE, C_DMA_STATUS_DONE};
static volatile INT8S sdio_rdma_result[SDC_TOTAL] = {C_DMA_STATUS_DONE, C_DMA_STATUS_DONE};
#endif
/**
* @brief 	Send SDIO IO_RX_EXTENDED command(CMD53).
* @param 	sdio[in]: SDIO handle.
* @param 	write[in]: The direction of io operation. 0 for read, and 1 for write.
* @param 	fn[in]: Function number.
* @param 	addr[in]: Register address.
* @param 	incr_addr[in]: Increment address, 0 for R/W fixed address, 1 for R/W incrementing address.
* @param 	buf[in]: Buffer pointer ( must be 4 byte alignment).
* @param 	blocks[in]: Block number.
* @param 	blksz[in]: Block size.
* @return 	0: success, -1: fail.
*/
INT32S drvl2_sdio_rw_extended(
	gpSDIOInfo_t *sdio,
	INT32U write,
	INT32U fn,
	INT32U addr,
	INT32U incr_addr,
	INT32U *buf,
	INT32U blocks,
	INT32U blksz)
{
	gpSDCtrl_t *ctrl = &gp_sdio_ctrl[sdio->host->device_id];
	sd_l1_ctrl_t *l1_ctrl = &ctrl->l1_ctrl;
	sd_l1_data_t *l1_data = &ctrl->l1_data;
	INT32U arg, dma_seperate = 0, dma_ena = 1;

	if( gp_sdio_info[sdio->host->device_id].host->present==0 )
		return -1;

	if ((fn >7) || (addr & ~0x1FFFF))
		return -1;

	if ((blocks == 0) || (blksz == 0))
		return -1;
	/* ----- Setting argument ----- */
	arg = write ? 0x80000000 : 0x00000000;
	arg |= fn << 28;
	arg |= incr_addr ? 0x04000000 : 0x00000000;
	arg |= addr << 9;

	if (blocks == 1 && blksz <= 512)
		arg |= (blksz == 512) ? 0 : blksz;	/* byte mode */
	else
		arg |= 0x08000000 | blocks;			/* block mode */

	if(blocks == 1 /*&& write */&& (blksz == 2 || blksz == 4))//(JC)using PIO mode in byte mode
	{
        dma_ena = 0;
    }

    //print_string("%s buf %x dma_ena %d\r\n","drvl2_sd_set_data",buf,dma_ena);
	drvl2_sd_set_data( l1_data, buf, blksz, blocks, write/* ? SDC_WRITE : SDC_READ*/, /*1*/dma_ena);
	/* ----- Send CMD53 ----- */
	if( drvl2_sd_send_command( sdio->host->device_id, MASK_CMD53, arg, SDC_RESPTYPE_R5, /*1*/dma_seperate, l1_data ) < 0 )
	{
        printf("CMD53 fail return -8. id=%d\r\n", sdio->host->device_id);
        printf("buf=0x%08x, blksz=%u blocks=%u dma_ena=%u arg=0x%08x separate=%d\r\n", (unsigned int)buf, blksz, blocks, dma_ena, arg, dma_seperate);
	    return -8;
    }
	/* ----- Check response error ----- */
	if( l1_ctrl->resp[0] & ( R5_COM_CRC_ERROR | R5_ILLEGAL_COMMAND | R5_ERROR | R5_FUNCTION_NUMBER | R5_OUT_OF_RANGE ))
	{
		DBG_PRINT("IO_RW_EXTENDED error 0x%x\r\n", l1_ctrl->resp[0] );
		//drvl2_sd_sem_unlock(sdio->device_id);
		return -2;
	}
//	DBG_PRINT("IO_RW_EXTENDED resp 0x%x\r\n", l1_ctrl->resp[0]);
    if(1 == dma_seperate)
    {
        if(0 == write)
        {
#if (SDIO_DMA_WAIT_STATUS == 1)
            if(drvl1_sdio_read_data_by_dma(sdio->host->device_id, buf, blocks, blksz, (INT8S *) &sdio_rdma_result[sdio->host->device_id])<0)
#else
            if(drvl1_sdio_read_data_by_dma(sdio->host->device_id, buf, blocks, blksz, NULL)<0)
#endif
            {
                DBG_PRINT("IO_RW_EXTENDED rdma error!!\r\n");
                //drvl2_sd_sem_unlock(sdio->device_id);
                return -3;
            }

            #if (SDIO_DMA_WAIT_STATUS == 1)
            while (sdio_rdma_result[sdio->host->device_id] == C_DMA_STATUS_WAITING);
            if (sdio_rdma_result[sdio->host->device_id] != C_DMA_STATUS_DONE) {
                //drvl2_sd_sem_unlock(sdio->device_id);
                return -4;
            }
            #else
            if (drvl1_sdc_command_complete_wait(sdio->host->device_id, 4300)) {
                return -5;
            }
            #endif
        }
        else
        {
#if (SDIO_DMA_WAIT_STATUS == 1)
            if(drvl1_sdio_write_data_by_dma(sdio->host->device_id, buf, blocks, blksz, (INT8S *) &sdio_wdma_result[sdio->host->device_id])<0)
#else
            if(drvl1_sdio_write_data_by_dma(sdio->host->device_id, buf, blocks, blksz, NULL)<0)
#endif
            {
                DBG_PRINT("IO_RW_EXTENDED wdma error!!\r\n");
                //drvl2_sd_sem_unlock(sdio->device_id);
                return -6;
            }
            #if (SDIO_DMA_WAIT_STATUS == 1)
            while (sdio_wdma_result[sdio->host->device_id] == C_DMA_STATUS_WAITING);
            if (sdio_wdma_result[sdio->host->device_id] != C_DMA_STATUS_DONE) {
                //drvl2_sd_sem_unlock(sdio->device_id);
                return -7;
            }
            #else
            if (drvl1_sdc_data_complete_bit_wait(sdio->host->device_id, 1000)) {
                DBG_PRINT("drvl1_sdc_data_complete_bit_wait fail\r\n");
                return -8;
            }
            #endif
        }
        if (drvl1_sdc_stop_controller(sdio->host->device_id, 1000)) {
            return -9;
        }
    }
    return 0;
}
/**
* @brief 	enable will be cleared after interrupt occurred.
* @param 	sdio[in]: SDIO handle.
* @param 	en[in]: Enable(1) or disable(0).
* @return 	None.
*/
void drvl2_sdio_irq_en(
	gpSDIOInfo_t *sdio,
	INT8U en)
{
	if(en)
	{
		if (0&&drvl1_sdio_config_bit_get(sdio->host->device_id) == 1)
        {
            INT8U extInt, extMux;

            drvl1_sdio_config_one_bit_intr_get(sdio->host->device_id, &extInt, &extMux);
            drv_l1_ext_enable_set(extInt, TRUE);
        }
		drvl1_sdc_ioint_en( sdio->host->device_id, 1 );
		drvl1_sdc_int_en( sdio->host->device_id, C_SDC_INT_SDIO_INT, 1 );
		//drvl1_sdc_dummyclk_en(sdio->host->device_id, 1 ); // williamyeo, ssv require dummy clock always enable, rtk no need
	}
	else
	{
		drvl1_sdc_dummyclk_en(sdio->host->device_id, 0 );
		drvl1_sdc_int_en( sdio->host->device_id, C_SDC_INT_SDIO_INT, 0 );
		drvl1_sdc_ioint_en( sdio->host->device_id, 0 );
		drvl1_sdc_sdio_int_clear( sdio->host->device_id );
		if (0&&drvl1_sdio_config_bit_get(sdio->host->device_id) == 1)
        {
            INT8U extInt, extMux;

            drvl1_sdio_config_one_bit_intr_get(sdio->host->device_id, &extInt, &extMux);
            drv_l1_ext_enable_set(extInt, FALSE);
			drv_l1_ext_int_clr(extInt);
        }
	}
}

#if 0
/**
* @brief 	Receive SDIO interrupt signal.
* @param 	sdio[in]: SDIO handle.
* @param 	wait[in]: Waiting for irq signal or not.
* @param 	timeout[in]: Timeout value in second.
* @return 	0: success, -1: fail.
*/
INT32S drvl2_sdio_receive_irq(
	gpSDIOInfo_t *sdio,
	INT32U wait,
	INT16U timeout)
{
	gpSDCtrl_t *ctrl = &gp_sdio_ctrl[sdio->host->device_id];
#if _OPERATING_SYSTEM != _OS_NONE
	INT32S result;
	INT8U error;
	if(wait)
		result = (INT32S) OSQPend(ctrl->int_queue, timeout * OS_TICKS_PER_SEC, &error);
	else
		result = (INT32S) OSQAccept(ctrl->int_queue, &error);

	if(error == OS_NO_ERR)
		return 0;
	else
		return -1;
#else
	INT32U notify = ctrl->int_notify;
	if(wait)
	{
		INT32U count = 1000;
		while( notify == 0 && timeout )
		{
			drv_msec_wait(1);
			count --;
			if(count == 0)
			{
				timeout --;
				count = 1000;
			}
			notify = ctrl->int_notify;
		}
	}

	if(notify)
		ctrl->int_notify = 0;
	return (notify)? 0: -1;
#endif
}
#endif
/**
* @brief 	Read the common registers.
* @param 	sdio[in]: SDIO handle.
* @return 	0: success, -1: fail.
*/
static INT32S drvl2_sdio_read_cccr(
	gpSDIOInfo_t *sdio)
{
	INT32S ret;
	INT32S cccr_vsn;
	INT8U data;

	memset(&sdio->cccr, 0, sizeof(sdio_cccr_t));

	ret = drvl2_sdio_rw_direct( sdio, 0, 0, SDIO_CCCR_CCCR, 0, &data );
	if (ret)
		goto out;

	cccr_vsn = data & 0x0f;

	if (cccr_vsn > SDIO_CCCR_REV_3_00) {
		DBG_PRINT("unrecognised CCCR structure version %d\n", cccr_vsn);
		return -1;
	}

	sdio->cccr.sdio_vsn = (data & 0xf0) >> 4;

	ret = drvl2_sdio_rw_direct( sdio, 0, 0, SDIO_CCCR_CAPS, 0, &data );
	if (ret)
		goto out;

	if (data & SDIO_CCCR_CAP_SMB)
		sdio->cccr.multi_block = 1;
	if (data & SDIO_CCCR_CAP_LSC)
		sdio->cccr.low_speed = 1;
	if (data & SDIO_CCCR_CAP_4BLS)
		sdio->cccr.wide_bus = 1;

	if (cccr_vsn >= SDIO_CCCR_REV_1_10)
	{
		ret = drvl2_sdio_rw_direct( sdio, 0, 0, SDIO_CCCR_POWER, 0, &data);
		if (ret)
			goto out;

		if (data & SDIO_POWER_SMPC)
			sdio->cccr.high_power = 1;
	}

	if (cccr_vsn >= SDIO_CCCR_REV_1_20)
	{
		ret = drvl2_sdio_rw_direct( sdio, 0, 0, SDIO_CCCR_SPEED, 0, &data);
		if (ret)
			goto out;

		if (data & SDIO_SPEED_SHS)
			sdio->cccr.high_speed = 1;
	}

out:
	return ret;
}

/**
* @brief 	Enable SDIO high speed mode.
* @param 	sdio[in]: SDIO handle.
* @return 	0: success, -1: fail.
*/
static INT32S drvl2_sdio_enable_hs(
	gpSDIOInfo_t *sdio)
{
	INT8U speed;

	if (!sdio->cccr.high_speed)
		return -1;

	if( drvl2_sdio_rw_direct(sdio, 0, 0, SDIO_CCCR_SPEED, 0, &speed) < 0 )
		return -1;

	speed |= SDIO_SPEED_EHS;

	if( drvl2_sdio_rw_direct(sdio, 1, 0, SDIO_CCCR_SPEED, speed, NULL) < 0 )
		return -1;

	return 0;
}

/**
* @brief 	Enable SDIO 4 bit mode.
* @param 	sdio[in]: SDIO handle.
* @return 	0: success, -1: fail.
*/
static INT32S  drvl2_sdio_enable_wide(
	gpSDIOInfo_t *sdio)
{
	INT8U ctrl;

	if (sdio->cccr.low_speed && !sdio->cccr.wide_bus)
		return 0;

	if( drvl2_sdio_rw_direct(sdio, 0, 0, SDIO_CCCR_IF, 0, &ctrl) < 0 )
		return -1;

	ctrl |= SDIO_BUS_WIDTH_4BIT;

	if( drvl2_sdio_rw_direct(sdio, 1, 0, SDIO_CCCR_IF, ctrl, NULL) < 0 )
		return -1;

//	drvl1_sdc_bus_width_set( sdio->device_id, 1);
	drvl1_sdc_bus_width_set( sdio->host->device_id, 4);

	return 0;
}

/**
* @brief 	SDIO interrupt service  function.
* @return 	None.
*/
#if 0
static void drvl2_sdio_isr(void)
{
	INT32U i;
	for( i=0 ; i<2 ; i++)
	{
		gpSDCtrl_t *ctrl = &gp_sdio_ctrl[i];
		if( (drvl1_sdc_int_en_check(i, C_SDC_INT_IO_INT )== 0) && (drvl1_sdc_sdio_int_check(i) == 0) )
		{
			//drvl1_sdc_sdio_int_clear(i);
			drvl2_sdio_irq_en( &gp_sdio_info[i], 0 );
			ctrl->int_notify = 1;
		#if _OPERATING_SYSTEM != _OS_NONE
			OSQPost( ctrl->int_queue, (void*)1);
		#endif
		}
	}
}
#endif
/*
 * Set BLOCK Size
 */
static bool sdio_set_block_size(struct sdio_func *func,unsigned int blksize)
{
    unsigned char blk[2];
    u8 in, out;
    int ret;

    if ((blksize == 0) || (blksize > 512))
    {
        blksize = SDIO_DEFAULT_BLOCK_SIZE;
    }

    blk[0] = (blksize >> 0) & 0xff;
    blk[1] = (blksize >> 8) & 0xff;
    in = blk[0];

    ret = drvl2_sdio_rw_direct(func->card, 1, 0/*func->num*/, (int)0x110, in, &out);
    if (!ret)
    {
        in = blk[1];
        ret = drvl2_sdio_rw_direct(func->card, 1, 0/*func->num*/, (int)0x111, in, &out);
    }

    if (!ret)
    {
        func->cur_blksize = sdio_block_size = blksize;
    }
    else
    {
        print_string("sdio_set_block_size(%x), ret = %d\r\n", blksize, ret);
        return false;
    }

    return true;

}

/*
 * Allocate and initialise a new SDIO function structure.
 */
struct sdio_func *sdio_alloc_func(struct mmc_card *card)
{
	struct sdio_func *func;

	func = gp_malloc_align(sizeof(struct sdio_func),32);
	gp_memset((INT8S*)func,0,sizeof(struct sdio_func));
	//if (!func)
	//	return ERR_PTR(-ENOMEM);

	func->card = card;

	//device_initialize(&func->dev);

	//func->dev.parent = &card->dev;
	//func->dev.bus = &sdio_bus_type;
	//func->dev.release = sdio_release_func;

	return func;
}

void sdio_remove_func(struct sdio_func *func)
{
	if(func) gp_free(func);
}
/**
* @brief 	Initial SDIO func
* @param
* @return 	0: success, -1: fail.
*/
static int sdio_init_func(struct mmc_card *card, unsigned int fn)
{
	int ret;
	struct sdio_func *func;

	//BUG_ON(fn > SDIO_MAX_FUNCS);

	func = sdio_alloc_func(card);
	//if (IS_ERR(func))
	//	return PTR_ERR(func);

	func->num = fn;

	/*ret = sdio_read_fbr(func);
	if (ret)
		goto fail;*/

	ret = drvl2_sdio_read_func_cis(func);
	if (ret)
		goto fail;

	card->sdio_func[fn - 1] = func;
    wifi_sdio_func = func;
    print_string("%s wifi_sdio_func addr %x\r\n",__FUNCTION__,wifi_sdio_func);
	return 0;

fail:
	/*
	 * It is okay to remove the function here even though we hold
	 * the host lock as we haven't registered the device yet.
	 */
	sdio_remove_func(func);
	return ret;
}


/**
* @brief 	Initial SDIO card.
* @param 	device_id[in]:  Index of SD controller.
* @return 	0: success, -1: fail.
*/
INT32S drvl2_sdio_init(INT32U device_id)
{
    drvl2_sdc_clock_gate_enable(device_id, 1);

    //drvl2_sd_sem_init(device_id);
	//drvl2_sd_sem_lock(device_id);
/*
	sd_card_rca[device_id] = 0x0;
	sd_card_type[device_id] = C_MEDIA_STANDARD_SD_CARD;
	sd_card_total_sector[device_id] = 0;
	sd_card_speed[device_id] = 400000;				// 400K Hz
	sd_card_bus_width[device_id] = 1;
	sd_card_protect[device_id] = 0;
	sd_card_state[device_id] = SD_CARD_STATE_IDLE;
	drvl1_sdc_init(device_id);
	drvl1_sdc_enable(device_id);
*/
	gpSDIOInfo_t *card = &gp_sdio_info[device_id];
	gpSDCtrl_t *ctrl = &gp_sdio_ctrl[device_id];
	sd_l1_ctrl_t *l1_ctrl = &ctrl->l1_ctrl;
	INT32U spend_ms;
    INT32U max_ms;

	/* ----- Clear all variable ----- */
	memset( card, 0, sizeof(gpSDIOInfo_t) );
	memset( &gp_sdio_ctrl[device_id].l1_ctrl, 0, sizeof(sd_l1_ctrl_t));
	memset( &gp_sdio_ctrl[device_id].l1_data, 0, sizeof(sd_l1_data_t));
	memset( &gp_sdio_host[device_id]        , 0, sizeof(struct mmc_host));

	card->host = &gp_sdio_host[device_id];
	/* ----- Initial queue ----- */
#if _OPERATING_SYSTEM != _OS_NONE
//	ctrl->int_queue = OSQCreate(&ctrl->int_buf, 1);
//	l1_data->dma_q = OSQCreate(&l1_data->dma_q_buf, 1);
#endif
	card->host->device_id = device_id;
	print_string("%s card addr %x host addr %x\r\n",__FUNCTION__,card,card->host);
	print_string("size gp_sdio_info:%u gp_sdio_ctrl:%u gp_sdio_host:%u %u\r\n", sizeof(gp_sdio_info), sizeof(gp_sdio_ctrl), sizeof(gp_sdio_host), sizeof(gpSDCtrl_t));
	/* ----- Initial SD controller ----- */
	drvl1_sdc_init(device_id,0);
	drvl1_sdc_enable(device_id);
	/* ----- Start 74 cycles on SD Clk Bus ----- */
	drvl1_sdc_card_init_74_cycles(device_id);
	//drvl1_sdc_card_init_74_cycles(device_id);
	/* ----- Reset Command, Should be no response ----- */
	if( drvl2_sd_send_command( device_id, MASK_CMD0, 0x00, SDC_RESPTYPE_NONE, 0, NULL ) < 0 )
        goto fail;
    DBG_PRINT("##SDIO CMD0 ok!\r\n");
	/* ----- Skip CMD8, for our application SDIO card does not work with memory card ----- */
	//drvl2_sd_send_command( device_id, MASK_CMD8, 0x1AA, SDC_RESPTYPE_R7, 0, NULL );
	/* ----- Send CMD5 to detect SDIO card ----- */
	drv_msec_wait(1);
    for (max_ms=200, spend_ms=0; spend_ms < max_ms; spend_ms++)
    {
        if( drvl2_sd_send_command( device_id, MASK_CMD5, 0x00, SDC_RESPTYPE_R4, 0, NULL ) < 0 )
            goto fail;
        if (l1_ctrl->resp[0] != 0)
            break;
        drv_msec_wait(1);
    }
    DBG_PRINT("##SDIO CMD5 ok! resp=%x, spend_ms=%u\r\n", l1_ctrl->resp[0], spend_ms);
	/* ----- Check operation voltage ----- */
	if( (l1_ctrl->resp[0] & SDIO_OCR) == 0 )
	{
		DBG_PRINT("Not support OCR 0x%x. spend_ms=%u\r\n", l1_ctrl->resp[0], spend_ms);
		goto fail;
	}

	/* ----- Run CMD5 until card finish power up sequence ----- */
	drv_msec_wait(1);
	for (max_ms=200, spend_ms=0;spend_ms < max_ms; spend_ms++)
	{
		if( drvl2_sd_send_command( device_id, MASK_CMD5, SDIO_OCR, SDC_RESPTYPE_R4, 0, NULL ) < 0 )
			goto fail;
        if (( l1_ctrl->resp[0] & 0x80000000 ) != 0)
            break;
        drv_msec_wait(1);
	}
	if (( l1_ctrl->resp[0] & 0x80000000 ) == 0)
	{
        DBG_PRINT("SDIO power-up stable trainning timeout... spend_ms=%u\r\n", spend_ms);
        goto fail;
    }
	DBG_PRINT("##SDIO pwrup stable! spend_ms=%u\r\n", spend_ms);

	/* ----- Step 6, CMD 3 Read New RCA, SD will generate RCA itself ----- */
	drv_msec_wait(1);
	if( drvl2_sd_send_command( device_id, MASK_CMD3, 0x00, SDC_RESPTYPE_R6, 0, NULL) < 0 )
		goto fail;
    DBG_PRINT("##SDIO CMD3 ok! resp=%x\r\n", l1_ctrl->resp[0]);
	card->rca = l1_ctrl->resp[0] & 0xFFFF0000;
	/* ----- Select card ----- */
	if( drvl2_sd_send_command( device_id, MASK_CMD7, card->rca, SDC_RESPTYPE_R1b, 0, NULL) < 0 )
		goto fail;
    DBG_PRINT("##SDIO CMD7 ok!\r\n");
	card->host->present = 1;
	/* ----- Read common reister ----- */
	if(drvl2_sdio_read_cccr(card)<0)
		goto fail;
	/* ----- Read the common CIS tuple ----- */
	if(drvl2_sdio_read_common_cis(card)<0)
		goto fail;
	/* ----- Switch to high-speed (if supported). */
	if(drvl2_sdio_enable_hs(card) == 0)
		//drvl1_sdc_clock_set(device_id, 50000000);
		drvl1_sdc_clock_set(device_id, 25000000);
	else
		drvl1_sdc_clock_set(device_id, card->cis.max_dtr);

    {
        INT8U cccr_if_reg;

        if( drvl2_sdio_rw_direct(card, 0, 0, SDIO_CCCR_IF, 0, &cccr_if_reg) < 0 )
            goto fail;
        DBG_PRINT("SDIO_CCCR_IF value=%u\r\n", cccr_if_reg);
    }

	if (drvl1_sdio_config_bit_get(device_id) == 1)
    {
        /* ----- Switch to 1 bit bus ----- */
        drvl1_sdc_bus_width_set(device_id, 1);

        /* ----- Set 1 bit external interrupt pin ----- */
        // SD0_1, DAT3, EXTC_MUX0, SD1_0, DAT1, EXTB_MUX3
        INT8U extInt, extMux;
        drvl1_sdio_config_one_bit_intr_get(device_id, &extInt, &extMux);
        drvl1_sdio_install_1_bit_isr(device_id, extInt, extMux, sdio_host_isr);
        DBG_PRINT("SDIO USE 1 BIT, extInt=%u, extMux=%u\r\n", extInt, extMux);
    }
    else
    {
        /* ----- Switch to wider bus (if supported). ----- */
        if(drvl2_sdio_enable_wide(card) < 0)
            goto fail;
    }

    if(sdio_init_func(card, 1) < 0)
        goto fail;

    if(sdio_set_block_size(card->sdio_func[1 - 1],SDIO_DEFAULT_BLOCK_SIZE) < 0)
        goto fail;

//    if( drvl2_sd_send_command( device_id, MASK_CMD52, 0x00, SDC_RESPTYPE_R5, 0, NULL ) < 0 )
//		goto fail;
//    DBG_PRINT("##SDIO CMD52 ok! resp=%x\r\n", l1_ctrl->resp[0]);
	/* ----- Register and enable interrupt function ----- */
//	vic_irq_register( (device_id==0)? VIC_SD:VIC_SD1, drvl2_sdio_isr);
//  vic_irq_enable((device_id==0)? VIC_SD:VIC_SD1);

//TBD

        vic_irq_register(VIC_SD, (device_id == 0) ? SD0_IRQHandler : SD1_IRQHandler);
        vic_irq_enable(VIC_SD);

/*
    NVIC_SetPriority((device_id == 0) ? SD0_IRQn : SD1_IRQn, 5);
    NVIC_EnableIRQ((device_id == 0) ? SD0_IRQn : SD1_IRQn);*/
	return 0;
fail:
#if _OPERATING_SYSTEM != _OS_NONE
	{
//		INT8U err;
//		OSQDel( ctrl->int_queue, OS_DEL_ALWAYS, &err);
//		OSQDel( l1_data->dma_q, OS_DEL_ALWAYS, &err);
	}
#endif
	card->host->present = 0;
	return -1;
}
#endif

/**
* @brief 	DeInitial SDIO card.
* @param 	device_id[in]:  Index of SD controller.
* @return 	0: success, -1: fail.
*/
INT32S drvl2_sdio_deinit(INT32U device_id)
{
    sdio_remove_func(wifi_sdio_func);

    vic_irq_unregister(VIC_SD);
    vic_irq_disable(VIC_SD);

    drvl2_sdc_clock_gate_enable(device_id, 0);

    return 0;
}

/*
void SD0_IRQHandler(void)
{
    gpSDCtrl_t *ctrl = &gp_sdio_ctrl[1];
    if( (drvl1_sdc_int_en_check(1, C_SDC_INT_IO_INT)== 0) && (drvl1_sdc_sdio_int_check(1) == 0) )
	{
		drvl2_sdio_irq_en( &gp_sdio_info[1], 0 );
		ctrl->int_notify = 1;
    }
}
*/


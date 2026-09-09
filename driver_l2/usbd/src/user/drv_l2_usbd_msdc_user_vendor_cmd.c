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
#include "drv_l2.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define RTC_RTCEN     	(1 << 15)  /* RTC enable */
#define RTC_HMSEN     	(1 <<  9)  /* H/M/S function enable */

#define RTC_RETRY 1024

#define R_EXTRTC_CTRL		0x00
#define R_EXTRTC_LOAD_TIME0	0x10
#define R_EXTRTC_LOAD_TIME1	0x11
#define R_EXTRTC_LOAD_TIME2	0x12
#define R_EXTRTC_LOAD_TIME3	0x13
#define R_EXTRTC_LOAD_TIME4	0x14
#define R_EXTRTC_LOAD_TIME5	0x15

#define VWRITE_BUF_SIZE     10480
#define VREAD_BUF_SIZE       5240

/**************************************************************************
 *                            E X T E R N A L                             *
 **************************************************************************/
extern void scsi_send_stall_null(INT32U usb_id);
extern void scsi_command_fail(INT32U usb_id, INT32U senseidx);
extern void user_vendorcmd_register_callback(INT32U (*func)(void), INT32U cmd_no);
extern INT32S ext_rtc_write(INT8U addr, INT8U data);

extern INT32U dmabufsize;
extern USBD_MSDC_CTL_BLK msdc_ctlblk[2];
extern USBD_MSDC_DMA_BUF msdc_dma_buf[2][BOT_DMA_BUF_NUM];
extern INT8U dmadone;
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
INT8U *vwrite_buf;
INT8U *vread_buf;
vread_info_t vread_info;
vwrite_info_t vwrite_info;
/**************************************************************************
 *                           F U N C T I O N S                            *
 **************************************************************************/

static INT32U residuebytes = 0;
static volatile INT8S vread_dma_read_result;
static volatile INT8S vwrite_dma_write_result;


//============================================================================
// Generic vendor command interface
//============================================================================
/**************************************************************
    Funciton: gp_scsi_vendor_cmd_set_rtc()
**************************************************************/
/**************************************************************
                                 CBWCB
     		  bit  +---+---+---+---+---+---+---+---+
         Byte      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
                   +---+---+---+---+---+---+---+---+
           15      |          CMD_ID MSB           |
                   +-------------------------------+
           16      |          CMD_ID LSB           |
                   +-------------------------------+
           17      |              ---              |
                   +-------------------------------+
           18      |           year MSB            |
                   +-------------------------------+
           19      |           year LSB            |
                   +-------------------------------+
           20      |             month             |
                   +-------------------------------+
           21      |              day              |
                   +-------------------------------+
           22      |              hour             |
                   +-------------------------------+
           23      |             minute            |
                   +-------------------------------+
           24      |             second            |
                   +-------------------------------+
          25-30    |              ---              |
                   +-------------------------------+

***************************************************************/
void gp_scsi_vendor_cmd_set_rtc(INT32U usb_id)
{
	INT32U rtc_ret;
	INT32U gp_sec = (INT32U)(msdc_ctlblk[usb_id].cbw.cbw_raw[24]);	// tm_sec;	/* 0-59 */
	INT32U gp_min = (INT32U)(msdc_ctlblk[usb_id].cbw.cbw_raw[23]);	// tm_min;	/* 0-59 */
	INT32U gp_hour = (INT32U)(msdc_ctlblk[usb_id].cbw.cbw_raw[22]); // tm_hour; /* 0-23 */
	INT32U gp_day = (INT32U)(msdc_ctlblk[usb_id].cbw.cbw_raw[21]);	// tm_mday; /* 1-31 */
	//(INT32U)(msdc_ctlblk[usb_id].cbw.cbw_raw[20]);	// tm_mon;	/* 1-12 */
	//(INT32U)(msdc_ctlblk[usb_id].cbw.cbw_raw[18]<<8) | (msdc_ctlblk[usb_id].cbw.cbw_raw[19]); // tm_year;
	INT64U rtc_time = (gp_day * 60 * 60 * 24) + (gp_hour * 60 * 60) + (gp_min * 60) + gp_sec;
	rtc_time <<= 15;

    DBG_PRINT("date: %d_%d_%d_%d\r\n", gp_day,gp_hour,gp_min,gp_sec);
	{
		rtc_ret = ext_rtc_write(R_EXTRTC_LOAD_TIME2, (rtc_time >> 16) & 0xFF);
	}
	if (rtc_ret == 0)
	{
		rtc_ret = ext_rtc_write(R_EXTRTC_LOAD_TIME3, (rtc_time >> 24) & 0xFF);
	}
	if (rtc_ret == 0)
	{
		rtc_ret = ext_rtc_write(R_EXTRTC_LOAD_TIME4, (rtc_time >> 32) & 0xFF);
	}
	if (rtc_ret == 0)
	{
		rtc_ret = ext_rtc_write(R_EXTRTC_LOAD_TIME5, (rtc_time >> 40) & 0xFF);
	}
    DBG_PRINT("rtc_ret = %d\r\n",rtc_ret);
}


INT32S vread_buf2dmabuf_by_dma(INT32U *tar_buffer, INT32U offset, INT32U byte_count, INT8S *poll_status)
{
	DMA_STRUCT dma_struct={0};
	INT8S done;

	dma_struct.s_addr = (INT32U)(vread_info.dataptr+offset);
	dma_struct.t_addr = (INT32U)tar_buffer;
	dma_struct.width = DMA_DATA_WIDTH_4BYTE;
	dma_struct.count = (INT32U) (byte_count>>2);
    dma_struct.timeout = 255;
	if (poll_status)
	{
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
}

static INT32S drvl2_vread_buf2dmabuf(INT32U *tgt_buf, INT32U b_offset, INT32U b_count, INT8U wait_flag)
{
    INT32S ret;

	if (wait_flag == 0)
	{
		// Start DMA and return immediately
		ret = vread_buf2dmabuf_by_dma(tgt_buf, b_offset, b_count, (INT8S *) &vread_dma_read_result);
	}
	else if (wait_flag == 1)
	{
		// Start DMA and wait until done
		ret = vread_buf2dmabuf_by_dma(tgt_buf, b_offset, b_count, NULL);
		return ret;
	}

	// Query status and return when done
	while (1)
	{
        if (vread_dma_read_result != C_DMA_STATUS_WAITING)
            break;
	}
	if (vread_dma_read_result != C_DMA_STATUS_DONE)
	{
		return -1;
	}

	return 0;
}

//================================================================
//usb interrupt event callback function for bulk in vendor command
//================================================================
void drv_l2_usbd_msdc_state_bot_in_vendor(INT32U usb_id, INT32U event)
{
    INT32U transfer_len;

    USB20D_BASE_SEL(usb_id);
    switch(event)
    {
        case USBD_BULK_IN_ACK_EVENT:
        case USBD_BULK_89_IN_ACK_EVENT:
            #if (MSDC_EP == BULK_EP12)
		    drv_l1_usbd_disable_bulk_in_out(usb_id);
            #elif (MSDC_EP == BULK_EP89)
			drv_l1_usbd_disable_bulk89_in_out();
            #endif
            if(msdc_ctlblk[usb_id].prostate == BULK_DATA_STA)
            {
                INT8U dir = msdc_ctlblk[usb_id].cbw.cbw_cb.bmCBWFlags >> 7;
                if(dir == CBW_DATA_DIR_IN)
                {
                    if(msdc_ctlblk[usb_id].cbwdatatcnt == 0)
                    {
                        /* send csw pkt*/
                        drv_l2_usbd_msdc_send_csw_data(usb_id);
                    }
                    else
                    {
                        /* msdc_ctlblk[usb_id].cbwdatatcnt > 0, stall bulk IN pipe, for USB 2.0 test suite */
                        DBG_PRINT("**IN ACK, data cnt = 0x%x, stall IN pipe\r\n", msdc_ctlblk[usb_id].cbwdatatcnt);
                        drv_l1_usbd_send_bulk_stall_null(usb_id, MSDC_EP, CBW_DATA_DIR_IN);
                        msdc_ctlblk[usb_id].csw.bCSWStatus = CSW_CMD_PASS_STA;
    					msdc_ctlblk[usb_id].cswdataresidue = 0;
    					msdc_ctlblk[usb_id].cbwdatatcnt = 0;
                        drv_l2_usbd_msdc_send_csw_data(usb_id);
                    }
                }
            }
            else if(msdc_ctlblk[usb_id].prostate == BULK_CSW_STA)
            {
                //USBD_L2MSDCLOG("USBDL2MSDC: usbd_l2_msdc_bot_in() BULK_CSW_STA!!\r\n");
                //DBG_PRINT("usbd_l2_msdc_bot_in() BULK_CSW_STA, opcode = 0x%x!!\r\n", msdc_ctlblk[usb_id].cbw.cbw_cb.CBWCB[0]);
                /* enable bulk out pkt to receive next CBW and reset BOT state to CBW */
                msdc_ctlblk[usb_id].prostate = BULK_CBW_STA;
                #if (MSDC_EP == BULK_EP12)
                drv_l1_usbd_enable_rec_bulk_out_pkt(usb_id);
                #elif (MSDC_EP == BULK_EP89)
				drv_l1_usbd_enable_rec_bulk89_out_pkt();
                #endif
            }
            break;

        case USBD_BULK_IN_DMA_TRANS_DONE_EVENT:
        case USBD_BULK_89_IN_DMA_TRANS_DONE_EVENT:
            #if (MSDC_EP == BULK_EP12)
		    /* disable bulk in/out pkt 0x330[1] */
		    drv_l1_usbd_disable_bulk_in_out(usb_id);
		    //DBG_PRINT("drv_l2_usbd_msdc_state_bot_in , event %d, msdc_ctlblk[usb_id].prostate = %d\r\n", event, msdc_ctlblk[usb_id].prostate);
            #elif (MSDC_EP == BULK_EP89)
			drv_l1_usbd_disable_bulk89_in_out();
            #endif
			dmadone = DMA_BULK_IN_MODE;

			if(msdc_ctlblk[usb_id].prostate == BULK_DATA_STA)
			{
				//(JC)triggering dma from usb to companion buffer, dma encoded data--> dma buffer
                if(!msdc_ctlblk[usb_id].nodmadata)
                {
                    //DBG_PRINT("USBDL2MSDC: drv_l2_usbd_msdc_do_read_scsi_cmd before bulk in\r\n");

                    drv_l1_cp15_clean_dcache_range((INT32U)msdc_ctlblk[usb_id].dmaptr->dataptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);
                    drv_l1_cp15_invalidate_dcache_range((INT32U)msdc_ctlblk[usb_id].dmaptr->dataptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);

                    /* send the data in RAM buffer to USB */
                    #if (MSDC_EP == BULK_EP12)
                    drv_l1_usbd_send_bulk_in(usb_id, (INT32U*)msdc_ctlblk[usb_id].dmaptr->dataptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);
                    #elif (MSDC_EP == BULK_EP89)
                    drv_l1_usbd_send_bulk89_in((INT32U*)msdc_ctlblk[usb_id].dmaptr->dataptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);
                    #endif
                    msdc_ctlblk[usb_id].cbwdatatcnt -= msdc_ctlblk[usb_id].dmaptr->usedbuffsize;
                    /* Change to the other RAM buffer */
                    msdc_ctlblk[usb_id].dmaptr = msdc_ctlblk[usb_id].dmaptr->next;

                    if(msdc_ctlblk[usb_id].rambufcnt)
                    {
                        transfer_len = dmabufsize;

                        drv_l1_cp15_clean_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), transfer_len);
                        drv_l1_cp15_invalidate_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), transfer_len);

                        drvl2_vread_buf2dmabuf((INT32U *)msdc_ctlblk[usb_id].dmaptr->dataptr, vread_info.offset, transfer_len, 0);
                        vread_info.offset+=transfer_len;
                        msdc_ctlblk[usb_id].nodmadata = 0;
                        msdc_ctlblk[usb_id].dmaptr->usedbuffsize = dmabufsize;
                        msdc_ctlblk[usb_id].rambufcnt--;
                    }
                    else if(msdc_ctlblk[usb_id].rambufres)
                    {
                        transfer_len = msdc_ctlblk[usb_id].rambufres * MSDC_BLOCK_SIZE;

                        drv_l1_cp15_clean_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), transfer_len);
                        drv_l1_cp15_invalidate_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), transfer_len);

                        drvl2_vread_buf2dmabuf((INT32U *)msdc_ctlblk[usb_id].dmaptr->dataptr, vread_info.offset, transfer_len, 0);
                        vread_info.offset+=transfer_len;
                        msdc_ctlblk[usb_id].nodmadata = 0;
                        msdc_ctlblk[usb_id].dmaptr->usedbuffsize = msdc_ctlblk[usb_id].rambufres * MSDC_BLOCK_SIZE;
                        msdc_ctlblk[usb_id].rambufres = 0;
                    }
                    else
                    {
                        if(residuebytes)
                        {
                            drv_l1_cp15_clean_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), residuebytes);
                            drv_l1_cp15_invalidate_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), residuebytes);

                            memcpy(msdc_ctlblk[usb_id].dmaptr->dataptr, (INT8S *)(vread_info.dataptr+vread_info.offset), residuebytes);
                            vread_info.offset+=residuebytes;
                            msdc_ctlblk[usb_id].dmaptr->usedbuffsize = residuebytes;
                            residuebytes = 0;
                            msdc_ctlblk[usb_id].nodmadata = 0;
                        }
                        else
                        {
                            /* no DMA data to send */
                            msdc_ctlblk[usb_id].nodmadata = 1;
                        }
                    }
                }
                else
                {
                    msdc_ctlblk[usb_id].nodmadata = 0;
                    msdc_ctlblk[usb_id].cswdataresidue = 0;
                    msdc_ctlblk[usb_id].csw.bCSWStatus = CSW_CMD_PASS_STA;
                    /* send CSW */
                    drv_l2_usbd_msdc_send_csw_data(usb_id);
                }
			}
            break;

        case USBD_BULK_IN_NACK_EVENT:
        	DBG_PRINT("EP1 IN NAK\r\n");
        	rDLCIE_UDLC |= MASK_USBD_UDLC_IE_EP1N;
        	break;

        case USBD_BULK_89_IN_NACK_EVENT:
        	DBG_PRINT("EP8 IN NAK\r\n");
        	rUDC_EPNEW_IE |= MASK_USBD_NEWEP_IE_EP8N;
        	break;

        default:
            break;
    }
}

void drv_l2_usbd_msdc_vendor_vread_in(INT32U usb_id)
{
    INT32U transfer_len;

    if(!msdc_ctlblk[usb_id].nodmadata)
    {
        /* send the data in RAM buffer to USB */

        drv_l1_cp15_clean_dcache_range((INT32U)msdc_ctlblk[usb_id].dmaptr->dataptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);
        drv_l1_cp15_invalidate_dcache_range((INT32U)msdc_ctlblk[usb_id].dmaptr->dataptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);

        #if (MSDC_EP == BULK_EP12)
    	drv_l1_usbd_send_bulk_in(usb_id, (INT32U*)msdc_ctlblk[usb_id].dmaptr->dataptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);
        #elif (MSDC_EP == BULK_EP89)
		drv_l1_usbd_send_bulk89_in((INT32U*)msdc_ctlblk[usb_id].dmaptr->dataptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);
        #endif
        msdc_ctlblk[usb_id].cbwdatatcnt -= msdc_ctlblk[usb_id].dmaptr->usedbuffsize;
        /* Change to the other RAM buffer */
        msdc_ctlblk[usb_id].dmaptr = msdc_ctlblk[usb_id].dmaptr->next;

        if(msdc_ctlblk[usb_id].rambufcnt)
        {
            transfer_len = dmabufsize;
            msdc_ctlblk[usb_id].secnum = (dmabufsize/MSDC_BLOCK_SIZE);

            drv_l1_cp15_clean_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), transfer_len);
            drv_l1_cp15_invalidate_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), transfer_len);

            drvl2_vread_buf2dmabuf((INT32U *)msdc_ctlblk[usb_id].dmaptr->dataptr, vread_info.offset, transfer_len, 0);
            vread_info.offset+=transfer_len;
            msdc_ctlblk[usb_id].dmaptr->usedbuffsize = dmabufsize;
            msdc_ctlblk[usb_id].rambufcnt--;
            msdc_ctlblk[usb_id].nodmadata = 0;
        }
        else if(msdc_ctlblk[usb_id].rambufres)
        {
            transfer_len = msdc_ctlblk[usb_id].rambufres * MSDC_BLOCK_SIZE;
            msdc_ctlblk[usb_id].secnum = msdc_ctlblk[usb_id].rambufres;

            drv_l1_cp15_clean_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), transfer_len);
            drv_l1_cp15_invalidate_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), transfer_len);

            drvl2_vread_buf2dmabuf((INT32U *)msdc_ctlblk[usb_id].dmaptr->dataptr, vread_info.offset, transfer_len, 0);
            vread_info.offset+=transfer_len;
            msdc_ctlblk[usb_id].dmaptr->usedbuffsize = transfer_len;
            msdc_ctlblk[usb_id].rambufres = 0;
            msdc_ctlblk[usb_id].nodmadata = 0;
        }
        else
        {
            if(residuebytes)
            {
                drv_l1_cp15_clean_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), residuebytes);
                drv_l1_cp15_invalidate_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), residuebytes);

                memcpy(msdc_ctlblk[usb_id].dmaptr->dataptr, (INT8S *)(vread_info.dataptr+vread_info.offset), residuebytes);
                vread_info.offset+=residuebytes;
                msdc_ctlblk[usb_id].dmaptr->usedbuffsize = residuebytes;
                residuebytes = 0;
                msdc_ctlblk[usb_id].nodmadata = 0;
            }
            else
            {
                /* no DMA data to send */
                msdc_ctlblk[usb_id].nodmadata = 1;
            }
        }
    }
    else
    {
         msdc_ctlblk[usb_id].cswdataresidue = 0;
         msdc_ctlblk[usb_id].csw.bCSWStatus = CSW_CMD_PASS_STA;
         /* send CSW */
         drv_l2_usbd_msdc_send_csw_data(usb_id);
    }
}

/**************************************************************
 *                generic vendor read function                *
 **************************************************************
                                 CBWCB
     		  bit  +---+---+---+---+---+---+---+---+
         Byte      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
                   +---+---+---+---+---+---+---+---+
           15      |          CMD_ID MSB           |
                   +-------------------------------+
           16      |          CMD_ID LSB           |
                   +-------------------------------+
          17-20    |              ---              |
                   +-------------------------------+
           21      | (LSB)    data length          |
                   +-------------------------------+
           22      |          data length          |
                   +-------------------------------+
           23      |          data length          |
                   +-------------------------------+
           24      |          data length    (MSB) |
                   +-------------------------------+
          25-30    |              ---              |
                   +-------------------------------+

***************************************************************/
void gp_scsi_vendor_cmd_vread(INT32U usb_id)
{
    INT32U datalen, blocknum;
    //INT32U transfer_len;

    datalen = msdc_ctlblk[usb_id].cbw.cbw_raw[0x18]<<24 | msdc_ctlblk[usb_id].cbw.cbw_raw[0x17]<<16 | msdc_ctlblk[usb_id].cbw.cbw_raw[0x16]<<8 | msdc_ctlblk[usb_id].cbw.cbw_raw[0x15];
    vread_info.datasize = datalen;
    blocknum = datalen/MSDC_BLOCK_SIZE;
    residuebytes = datalen%MSDC_BLOCK_SIZE;

    msdc_ctlblk[usb_id].rambufcnt = blocknum / (dmabufsize/MSDC_BLOCK_SIZE);    //how many ram buffer
	msdc_ctlblk[usb_id].rambufres = blocknum & ((dmabufsize/MSDC_BLOCK_SIZE)-1); //last ram buffer size used

    /* Set DMA buffer's state */
    msdc_dma_buf[usb_id][0].DMABufState = SCSI_DMA_BUF_NOT_USED_STA;
    msdc_dma_buf[usb_id][1].DMABufState = SCSI_DMA_BUF_NOT_USED_STA;

    msdc_ctlblk[usb_id].dmaptr = &msdc_dma_buf[usb_id][0];  //assign dma buffer pointer
    msdc_ctlblk[usb_id].prostate = BULK_DATA_STA;   //enter BOT DATA state
    msdc_ctlblk[usb_id].doingSCSIreadwrite = DO_SCSI_VENDOR_READ;

    if(msdc_ctlblk[usb_id].rambufcnt)
    {
        msdc_ctlblk[usb_id].dmaptr->usedbuffsize = dmabufsize;
        msdc_ctlblk[usb_id].secnum = (dmabufsize/MSDC_BLOCK_SIZE);

        drv_l1_cp15_clean_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), dmabufsize);
        drv_l1_cp15_invalidate_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), dmabufsize);

        drvl2_vread_buf2dmabuf((INT32U *)msdc_ctlblk[usb_id].dmaptr->dataptr, vread_info.offset, dmabufsize, 0);
        vread_info.offset+=dmabufsize;
        msdc_ctlblk[usb_id].rambufcnt--;
        msdc_ctlblk[usb_id].nodmadata = 0;
    }
    else if(msdc_ctlblk[usb_id].rambufres)
    {
        msdc_ctlblk[usb_id].dmaptr->usedbuffsize = msdc_ctlblk[usb_id].rambufres * MSDC_BLOCK_SIZE;
        msdc_ctlblk[usb_id].secnum = msdc_ctlblk[usb_id].rambufres;

        drv_l1_cp15_clean_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), msdc_ctlblk[usb_id].dmaptr->usedbuffsize);
        drv_l1_cp15_invalidate_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), msdc_ctlblk[usb_id].dmaptr->usedbuffsize);

        drvl2_vread_buf2dmabuf((INT32U *)msdc_ctlblk[usb_id].dmaptr->dataptr, vread_info.offset, msdc_ctlblk[usb_id].dmaptr->usedbuffsize, 0);
        vread_info.offset+=msdc_ctlblk[usb_id].dmaptr->usedbuffsize;
        msdc_ctlblk[usb_id].rambufres = 0;
        msdc_ctlblk[usb_id].nodmadata = 0;
    }
    else
    {
        if(residuebytes)
        {
            drv_l1_cp15_clean_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), residuebytes);
            drv_l1_cp15_invalidate_dcache_range((INT32U)(vread_info.dataptr+vread_info.offset), residuebytes);

            memcpy(msdc_ctlblk[usb_id].dmaptr->dataptr, (INT8S *)(vread_info.dataptr+vread_info.offset), residuebytes);
            vread_info.offset+=residuebytes;
            msdc_ctlblk[usb_id].dmaptr->usedbuffsize = residuebytes;
            residuebytes = 0;
            msdc_ctlblk[usb_id].nodmadata = 0;
        }
        else
            /* no DMA data to send */
            msdc_ctlblk[usb_id].nodmadata = 1;
    }
    drv_l2_usbd_register_state_handler(USBD_XFER_BULK, NULL, drv_l2_usbd_msdc_state_bot_in_vendor, drv_l2_usbd_msdc_state_bot_out);

    //dma buffer ==> USB pipe
    drv_l2_usbd_msdc_vendor_vread_in(usb_id);
}

INT32S vwrite_dmabuf2buf_by_dma(INT32U *src_buffer, INT32U offset, INT32U byte_count, INT8S *poll_status)
{
	DMA_STRUCT dma_struct={0};
	volatile INT8S done;

	dma_struct.s_addr = (INT32U)src_buffer;
	dma_struct.t_addr = (INT32U)(vwrite_info.dataptr+offset);
	dma_struct.width = DMA_DATA_WIDTH_4BYTE;
	dma_struct.count = (INT32U) (byte_count>>2);
    dma_struct.timeout = 255;

	if (poll_status)
	{
		*poll_status = C_DMA_STATUS_WAITING;
		dma_struct.notify = poll_status;

        // Send dma request to driver and return immediately
		if (drv_l1_dma_transfer(&dma_struct))
		{
			return -1;
		}
	}
	else
	{
		done = C_DMA_STATUS_WAITING;
		dma_struct.notify = &done;

		// Send dma request to driver and wait response
		if (drv_l1_dma_transfer_wait_ready(&dma_struct))
		{
			return -1;
		}
	}

	return 0;
}

static INT32S drvl2_vwrite_dmabuf2buf(INT32U *src_buf, INT32U b_offset, INT32U b_count, INT8U wait_flag)
{
    volatile INT32S ret=0;

    ret = ret; // resolve compile warning use
	if (wait_flag == 0)
	{
		// Start DMA and return immediately
		ret = vwrite_dmabuf2buf_by_dma(src_buf, b_offset, b_count, (INT8S *)&vwrite_dma_write_result);
	}
	else if (wait_flag == 1)
	{
		// Start DMA and wait until done
		INT32S ret = vwrite_dmabuf2buf_by_dma(src_buf, b_offset, b_count, NULL);
		return ret;
	}

	// Query status and return when done
	while (1)
	{
        if (vwrite_dma_write_result != C_DMA_STATUS_WAITING)
            break;
	}

	if (vwrite_dma_write_result != C_DMA_STATUS_DONE)
	{
		return -1;
	}

	return 0;
}

//================================================================
//usb interrupt event callback function for bulk out vendor command
//================================================================
void drv_l2_usbd_msdc_state_bot_out_vendor(INT32U usb_id, INT32U event)
{
    USB20D_BASE_SEL(usb_id);

    switch(event)
    {
        case USBD_BULK_OUT_READY_EVENT:
        case USBD_BULK_89_OUT_READY_EVENT:
            #if (MSDC_EP == BULK_EP12)
            drv_l1_usbd_disable_bulk_in_out(usb_id);
            #elif (MSDC_EP == BULK_EP89)
            drv_l1_usbd_disable_bulk89_in_out();
            #endif
            if(msdc_ctlblk[usb_id].prostate == BULK_CBW_STA)
            {
                if(drv_l2_usbd_msdc_get_cbw_data(usb_id) == USBD_RET_SUCCESS)
                {
                    // process SCSI command
                    drv_l2_usbd_scsi_command_process(usb_id);
                }
                else
                {
                	//DBG_PRINT("Get CBW data error, stall BULK IN/OUT...\r\n");
            	}
            }
            else if(msdc_ctlblk[usb_id].prostate == BULK_DATA_STA)
            {
                // continuous to receive data from host
                //DBG_PRINT("IN USBD_BULK_OUT_READY_EVENT\r\n");
            }
            break;

        case USBD_BULK_OUT_M_DMA_DONE_EVENT:
        case USBD_BULK_OUT_89_M_DMA_DONE_EVENT:
            #if (MSDC_EP == BULK_EP12)
            drv_l1_usbd_disable_bulk_in_out(usb_id);
            #elif (MSDC_EP == BULK_EP89)
            drv_l1_usbd_disable_bulk89_in_out();
            #endif
        	drv_l1_usbd_reset_bulk_dma_type();

			if(msdc_ctlblk[usb_id].prostate == BULK_DATA_STA)
			{
				INT32U presecnum;
				INT32U* toscsiptr;

                toscsiptr = (INT32U*)msdc_ctlblk[usb_id].dmaptr->dataptr;
				presecnum = msdc_ctlblk[usb_id].secnum;
				msdc_ctlblk[usb_id].dmaptr = msdc_ctlblk[usb_id].dmaptr->next;
				if(msdc_ctlblk[usb_id].rambufcnt)
				{
                    msdc_ctlblk[usb_id].nodmadata = 0;
					msdc_ctlblk[usb_id].secnum = (dmabufsize/MSDC_BLOCK_SIZE);
					msdc_ctlblk[usb_id].dmaptr->usedbuffsize = dmabufsize;	   /* Full RAM buffer size */
					msdc_ctlblk[usb_id].rambufcnt--;
				}
				else if(msdc_ctlblk[usb_id].rambufres)
				{
                    msdc_ctlblk[usb_id].nodmadata = 0;
					msdc_ctlblk[usb_id].secnum = msdc_ctlblk[usb_id].rambufres;
					msdc_ctlblk[usb_id].dmaptr->usedbuffsize = msdc_ctlblk[usb_id].rambufres * MSDC_BLOCK_SIZE;
					msdc_ctlblk[usb_id].rambufres = 0;
				}
				else
				{
                    if(residuebytes)
                    {
                        msdc_ctlblk[usb_id].nodmadata = 0;
                        msdc_ctlblk[usb_id].secnum = 0;
                        msdc_ctlblk[usb_id].dmaptr->usedbuffsize = residuebytes;
                        residuebytes = 0;
                    }
                    else
                    {
                        msdc_ctlblk[usb_id].nodmadata = 1;
                        msdc_ctlblk[usb_id].secnum = 0;
                        msdc_ctlblk[usb_id].dmaptr->usedbuffsize = 0;
                    }
				}

				if(!msdc_ctlblk[usb_id].nodmadata)
				{
                    #if (MSDC_EP == BULK_EP12)
					drv_l1_usbd_rec_bulk_out(usb_id, (void*)msdc_ctlblk[usb_id].dmaptr->dataptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);
                    #elif (MSDC_EP == BULK_EP89)
					drv_l1_usbd_get_dma_bulk89_out_data_by_len((void*)msdc_ctlblk[usb_id].dmaptr->dataptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);
                    #endif

                    drv_l1_cp15_clean_dcache_range((INT32U)toscsiptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);
                    drv_l1_cp15_invalidate_dcache_range((INT32U)toscsiptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);

                    drvl2_vwrite_dmabuf2buf(toscsiptr, vwrite_info.offset, msdc_ctlblk[usb_id].dmaptr->usedbuffsize, 0);
                    vwrite_info.offset+=msdc_ctlblk[usb_id].dmaptr->usedbuffsize;
				}
				else
				{
                    msdc_ctlblk[usb_id].nodmadata = 0;

                    drv_l1_cp15_clean_dcache_range((INT32U)toscsiptr, (presecnum << 9));
                    drv_l1_cp15_invalidate_dcache_range((INT32U)toscsiptr, (presecnum << 9));

                    drvl2_vwrite_dmabuf2buf(toscsiptr, vwrite_info.offset, (presecnum<<9), 0);
                    vwrite_info.offset+=(presecnum<<9);
					msdc_ctlblk[usb_id].csw.bCSWStatus = CSW_CMD_PASS_STA;
					msdc_ctlblk[usb_id].csw.dCSWDataResidue = 0;
					drv_l2_usbd_msdc_send_csw_data(usb_id);
				}

				if(vwrite_info.offset >= vwrite_info.datasize)
				{
                    vwrite_info.offset = 0;
				}
			}
            break;

        case USBD_BULK_OUT_NACK_EVENT:
        	DBG_PRINT("EP2 OUT NAK\r\n");
        	rDLCIE_UDLC |= MASK_USBD_UDLC_IE_EP2N;
        	break;

        case USBD_BULK_89_OUT_NACK_EVENT:
        	DBG_PRINT("EP9 OUT NAK\r\n");
        	rUDC_EPNEW_IE |= MASK_USBD_NEWEP_IE_EP9N;
        	break;

        default:
            break;
    }
}


/**************************************************************
 *                generic vendor write function               *
 **************************************************************
                                 CBWCB
     		  bit  +---+---+---+---+---+---+---+---+
         Byte      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
                   +---+---+---+---+---+---+---+---+
           15      |          CMD_ID MSB           |
                   +-------------------------------+
           16      |          CMD_ID LSB           |
                   +-------------------------------+
          17-24    |              ---              |
                   +-------------------------------+
           25      | (LSB)    data length          |
                   +-------------------------------+
           26      |          data length          |
                   +-------------------------------+
           27      |          data length          |
                   +-------------------------------+
           28      |          data length    (MSB) |
                   +-------------------------------+
          29-30    |              ---              |
                   +-------------------------------+

***************************************************************/
void gp_scsi_vendor_cmd_vwrite(INT32U usb_id)
{
    INT32U datalength=0, datalen=0;

    datalen = (msdc_ctlblk[usb_id].cbw.cbw_raw[0x1C]<<24 | msdc_ctlblk[usb_id].cbw.cbw_raw[0x1B]<<16 | msdc_ctlblk[usb_id].cbw.cbw_raw[0x1A]<<8 | msdc_ctlblk[usb_id].cbw.cbw_raw[0x19]);
    residuebytes = datalen%MSDC_BLOCK_SIZE; // residual data size that is less than 512B
    vwrite_info.datasize = datalen;
    datalength = (datalen/MSDC_BLOCK_SIZE);//(JC)multiple of 512B

    /* Set DMA buffer's state */
    msdc_dma_buf[usb_id][0].DMABufState = SCSI_DMA_BUF_NOT_USED_STA;
    msdc_dma_buf[usb_id][1].DMABufState = SCSI_DMA_BUF_NOT_USED_STA;
    msdc_ctlblk[usb_id].dmaptr = &msdc_dma_buf[usb_id][0];
    msdc_ctlblk[usb_id].rambufcnt = datalength / (dmabufsize/MSDC_BLOCK_SIZE);    //how many ram buffer //(JC)32kB dma buffer = 64 times of 512
	msdc_ctlblk[usb_id].rambufres = datalength & ((dmabufsize/MSDC_BLOCK_SIZE)-1); //last ram buffer size used(multiple of 512 Byte)
    msdc_ctlblk[usb_id].prostate = BULK_DATA_STA;
    msdc_ctlblk[usb_id].doingSCSIreadwrite = DO_SCSI_VENDOR_WRITE;

    if(msdc_ctlblk[usb_id].rambufcnt)
    {
        msdc_ctlblk[usb_id].dmaptr->usedbuffsize = dmabufsize;
        msdc_ctlblk[usb_id].secnum = (dmabufsize/MSDC_BLOCK_SIZE);
    }
    else if(msdc_ctlblk[usb_id].rambufres)
    {
        msdc_ctlblk[usb_id].dmaptr->usedbuffsize = msdc_ctlblk[usb_id].rambufres * MSDC_BLOCK_SIZE;
        msdc_ctlblk[usb_id].secnum = msdc_ctlblk[usb_id].rambufres;
    }
    else
    {
        if(residuebytes)
        {
            drv_l1_cp15_clean_dcache_range((INT32U)(vwrite_info.dataptr+vwrite_info.offset), residuebytes);
            drv_l1_cp15_invalidate_dcache_range((INT32U)(vwrite_info.dataptr+vwrite_info.offset), residuebytes);

            memcpy(msdc_ctlblk[usb_id].dmaptr->dataptr, (INT8S *)(vwrite_info.dataptr+vwrite_info.offset), residuebytes);
            vwrite_info.offset+=residuebytes;
            msdc_ctlblk[usb_id].dmaptr->usedbuffsize = residuebytes;
            residuebytes = 0;
            msdc_ctlblk[usb_id].nodmadata = 0;
        }
        else
        {
            msdc_ctlblk[usb_id].secnum = 0;
            msdc_ctlblk[usb_id].dmaptr->usedbuffsize = 0;
            msdc_ctlblk[usb_id].nodmadata = 1;
        }
    }
    drv_l2_usbd_register_state_handler(USBD_XFER_BULK, NULL, drv_l2_usbd_msdc_state_bot_in, drv_l2_usbd_msdc_state_bot_out_vendor);

    //enable bulk out vendor data receiving
    #if (MSDC_EP == BULK_EP12)
	drv_l1_usbd_rec_bulk_out(usb_id, (void*)msdc_ctlblk[usb_id].dmaptr->dataptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);//Start BULK OUT DMA to dma buf, while the dma done, interrupt occured and execute irq that triggering companion buffer dma data from usb, media dma data from current buffer
    #elif (MSDC_EP == BULK_EP89)
	drv_l1_usbd_get_dma_bulk89_out_data_by_len((void*)msdc_ctlblk[usb_id].dmaptr->dataptr, msdc_ctlblk[usb_id].dmaptr->usedbuffsize);//Start BULK OUT DMA
    #endif
    if(msdc_ctlblk[usb_id].rambufcnt)
    {
        msdc_ctlblk[usb_id].rambufcnt--;
    }
    else if(msdc_ctlblk[usb_id].rambufres)
    {
        msdc_ctlblk[usb_id].rambufres = 0;
    }
}

//======================================================
// user vendor command 1 function implementation example
// PC setting device RTC by this vendor command
//======================================================
INT32U usbd_user_vendorcmd1_function(INT32U usb_id)
{
    R_EXT_RTC_CTRL = 0x01;					/* enable ext rtc clock */
    ext_rtc_write(R_EXTRTC_CTRL, 0x15); 	// Up Count; RTC clk enable

    gp_scsi_vendor_cmd_set_rtc(usb_id);

    /* CSW */
    msdc_ctlblk[usb_id].cbwdatatcnt = 0;
    msdc_ctlblk[usb_id].csw.bCSWStatus = CSW_CMD_PASS_STA;
    msdc_ctlblk[usb_id].cswdataresidue = 0;
    drv_l2_usbd_msdc_send_csw_data(usb_id);   /* triggle next CBW */
    return 0;
}

//======================================================
// user vendor command 2 function implementation example
// write arbitrary length data to RAM buffer
//======================================================
INT32U usbd_user_vendorcmd2_function(INT32U usb_id)
{
    vwrite_buf = (INT8U*)gp_malloc_align(VWRITE_BUF_SIZE, 16);
    gp_memset((INT8S*)&vwrite_info, 0 , sizeof(vwrite_info));
    vwrite_info.dataptr = (INT8U *)(vwrite_buf);
    vwrite_info.offset = 0;
    gp_scsi_vendor_cmd_vwrite(usb_id);
    return 0;
}


//======================================================
// user vendor command 3 function implementation example
// read arbitrary length data from RAM buffer
//======================================================
INT32U usbd_user_vendorcmd3_function(INT32U usb_id)
{
    vread_buf = (INT8U*)gp_malloc_align(VREAD_BUF_SIZE, 16);
    gp_memset((INT8S*)&vread_info, 0 , sizeof(vread_info));
    gp_memcpy((INT8S*)vread_buf, (INT8S*)vwrite_buf, VREAD_BUF_SIZE);
    vread_info.dataptr = (INT8U *)vread_buf;
    vread_info.offset = 0;
    gp_scsi_vendor_cmd_vread(usb_id);
    return 0;
}


void user_auth_vendorcmd_function(INT32U usb_id, INT16U vcmd)
{
    switch(vcmd)
    {
        case GP_NV_READ:
            /* Read data from RAM */
            gp_scsi_vendor_cmd_read(usb_id);
            break;

        case GP_NV_WRITE:
            /* Write bin file to internal RAM */
            gp_scsi_vendor_cmd_write(usb_id);
            break;

        case GP_READ_MEMORY:
            /* Register read */
            gp_scsi_vendor_cmd_reg_read(usb_id);
            break;

        case GP_WRITE_MEMORY:
            /* Register write */
            gp_scsi_vendor_cmd_reg_write(usb_id);
            break;

        case GP_USER_CMD1:
            usbd_user_vendorcmd1_function(usb_id);
            break;

        case GP_USER_CMD2:
            usbd_user_vendorcmd2_function(usb_id);
            break;

        case GP_USER_CMD3:
            usbd_user_vendorcmd3_function(usb_id);
            break;

        //add your self-defined vendor command number and corresponded function in following case .......

        default:
            /* An invalid vendor command occurred ,return fail */
            scsi_send_stall_null(usb_id);
            scsi_command_fail(usb_id, 0x03);
            break;
    }
}

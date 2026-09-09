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
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park,                      *
 *  Hsinchu City 30077, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#ifndef __drv_l1_USBH_ISO_H__
#define __drv_l1_USBH_ISO_H__

/**************************************************************************
*                           C O N S T A N T S                             *
**************************************************************************/
#define iTD_COUNT                   128
#define iTD_GROUP_SZ                8
#define iTD_GROUP_SZ_PO2            3
#define iTD_GROUP_NUM               iTD_COUNT/iTD_GROUP_SZ

#define HOST_UVC_ISO_Buff_CNT  30//10
#define HOST_UVC_iTD_Table_CNT 10
#define HOST_UVC_iTD_Table_Size 64
#define HOST_UVC_Framelist_Size 1024//4096
#define iTD_Trans_Time 8
#define ISO_Max_Packet_Size 3*1024    // Multi=3
#define HOST_UVC_ISO_DATA_Buff iTD_Trans_Time*ISO_Max_Packet_Size   // for usb2.0 each iTD table can max receive 3*8 times

#define HOST_UVC_ISO_DATA_BUF_SZ iTD_Trans_Time*ISO_Max_Packet_Size

#define UAC_MAX_PKT_SZ		192 //iso full speed max pkt size is 1023(1/ms), but we still use 1024 for alignment reason.
#define HOST_UAC_MAX_DATA_BUF_SZ 192*iTD_COUNT
#define UVC_JPEG_BUF_CNT    6

/**************************************************************************
*                          D A T A    T Y P E S                           *
**************************************************************************/
typedef struct
{
    uint8_t *hw_abuf;
    uint8_t *hw_bbuf;
    uint8_t *hw_preabuf;
    uint8_t *hw_prebbuf;
    uint32_t hw_prevabuf_size;
    uint32_t hw_prevbbuf_size;
	uint32_t hw_abuf_size;
    uint32_t hw_bbuf_size;
    uint8_t abuf_index;
    uint8_t bbuf_index;
}PACKED UVC_HW_FRAMEBUF_INFO_T;

typedef struct
{
    uint8_t  jpegbuflock[UVC_JPEG_BUF_CNT];
    uint8_t  *jpegbufptr[UVC_JPEG_BUF_CNT];
    uint32_t total_len[UVC_JPEG_BUF_CNT];
}PACKED UVC_JPEG_BUF_INFO_T;

#endif	/*__drv_l1_USBH_ISO_H__*/

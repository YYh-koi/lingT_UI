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
#ifndef __DRV_L1_USBH_H__
#define __DRV_L1_USBH_H__
/**************************************************************************
 *                         H E A D E R   F I L E S                        *
**************************************************************************/
#include "drv_l1.h"

/**************************************************************************
*                           C O N S T A N T S                             *
**************************************************************************/
#define OUT_TOKEN					(0x00 << 8)
#define IN_TOKEN					(0x01 << 8)
#define SETUP_TOKEN					(0x02 << 8)
#define OUT_MAX_PKT					1024
#define IN_MAX_PKT					1024
#define TIMEOUT_RETRY				100
#define MAX_NAK_CNT					600000

#define USBDH_OPERATION_FUN_MAX_NUM 6

#define TYPE_OHCI       0
#define TYPE_EHCI       1
#define TYPE_UNKNOWN    2
#define TYPE_NOTDEFINED 3

#define USBH_EHCI_TYPE	0
#define USBH_OHCI_TYPE	1
#define USBH_TYPE		USBH_EHCI_TYPE

#define USBH0_BASE	0xD0C00000
#define USBH1_BASE	0xD0E00000

#define USB_A           0
#define USB_B           1

#define HOSTNUM         2
/* ------------------------------------*/
#define MSG_USBH_HCI_DEV_CONN               0x01
#define MSG_USBH_HCI_DEV_DISCONN            0x02
#define MSG_USBH_HCI_DEV_INIT               0x03
#define MSG_USBH_HCI_EXIT                   0x04
#define MSG_USBH_HCI_UVC_CLOSE              0x05
#define MSG_USBH_HCI_MSDC_CLOSE             0x06
#define MSG_USBH_HCI_MSDCFS_CLOSE           0x07
#define MSG_USBH_HCI_HID_CLOSE              0x08
#define MSG_USBH_HCI_CDC_CLOSE              0x09
#define MSG_USBH_HCI_DEV_ISR_HANG           0x0A



#define DEV_CONNECT         2
#define DEV_DISCONNECT      3
#define UNKNOWN_HOST_TYPE   4

/* USBH error code define */
#define USBH_E_NoErr	0x00
#define USBH_E_RX		0x01
#define USBH_E_TX		0x02
#define USBH_E_NACK		0x03
#define USBH_E_Stall	0x04
#define USBH_E_UnPID	0x05
#define USBH_E_BitStuff 0x06
#define USBH_E_DataSeq	0x07
#define USBH_E_CRC16	0x08
#define USBH_E_TimeOut	0x09
#define USBH_E_Para		0x0A
#define USBH_E_DMA		0x10
#define USBH_E_Remove	0xFF

#define DEBUG_PU_CTRL_FUNC
#ifdef DEBUG_PU_CTRL_FUNC
    #define PU_CTRL_FN_DBG                  DBG_PRINT
#else
    #define PU_CTRL_FN_DBG(...)
#endif

#define HCI_MSDC_CLASS      1
#define HCI_HID_CLASS       2
#define HCI_CDC_CLASS       3
#define HCI_UVC_CLASS       4
#define HCI_UAC_CLASS       5

#define HCI_DEV_CLASS HCI_UVC_CLASS
/********************* Define rUHCtrl bit mask (Offset + 0x04) *****************/
#define MASK_UH_CTRL_MASTER_EN	BIT0	/* USB host AHB master enable */

#define UVC_SEND_MSG_NUM	6
#define _DRV_L1_CACHE       0

#define DISPLAY_USB_PHY_CONFIG_VALUE    0

#define UPHY_SETTING 1

#if DISPLAY_USB_PHY_CONFIG_VALUE == 1
#define usb_print       DBG_PRINT
#else
#define usb_print(...)
#endif
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
	EP_CONTROL_TYPE		= 0,
	EP_BULK_TYPE,
	EP_ISO_TYPE,
	EP_INTERRUPT_TYPE
} EP_TYPE;

typedef struct
{
	INT8U			cAddr;			// USB address
	INT8U			cEndp;			// USB endpoint
	INT8U			cDMAeEn;		// DMA enable
	INT8U	        cEPType;	    // EndPoint Type
	INT8U*			cTog;			// USB data toggle
	void *		 	pBuf;			// data buffer address
	INT32U  		uiLn;			// buffer length
	INT32U			uiMaxPkt;		// maximum packet size
	INT8U			cMulti;
	INT32U*			pAct;			// active length
	INT8U*			pErrorCode;
	INT32U* 		offset;			// Buffer offset (for DMA use)
	INT8U			dir;			// in or out
	INT16U			xOffset;		// Transcation X offset
	void	        *pdata;		    // for data phase
	INT32U	        datalen;		// for data length
	INT32U          direction;		// for data in out
}ST_USBH_CTRL;

typedef struct usbhost_sysregs {
    volatile INT32U     version;
    volatile INT32U     control;
    volatile INT32U     padding[2];			/* pad dummy 8 bytes */
    volatile INT32U     powercontrl_port1;
    volatile INT32U     powercontrl_port2;
} usbhost_sysregs_t;
typedef struct _USBH_OPERATION_TBL
{
    const char  *drvname;
	INT32U (*host_init) (INT32U hostid);
	void (*host_uninit) (INT32U hostid);
	INT32U (*host_detect_port) (INT32U hostid);
	INT32U (*issue_setup) (INT32U hostid, ST_USBH_CTRL * Ctrl);
	INT32U (*issue_control_xfer) (INT32U hostid, ST_USBH_CTRL * Ctrl);
	INT32U (*issue_in_token) (INT32U hostid, ST_USBH_CTRL * Ctrl);
	INT32U (*issue_out_token) (INT32U hostid, ST_USBH_CTRL * Ctrl);
	INT32U (*get_sof_frame_num) (INT32U hostid);
	INT32U (*wait_interrupt) (INT32U hostid, INT32U uiPollBit);
	void (*wait_sof_tick) (INT32U hostid, INT32U nSOF);
	INT32U (*issue_hw_reset) (INT32U hostid, INT32U uiTick);
}
USBH_OPERATION_TBL;

typedef struct USB_HOST_COMMON_REGS_S
{
	volatile INT32U rUHVersion;			/*offset 0x00 */
	volatile INT32U rUHCtrl;			/*offset 0x04 */
	volatile INT32U dummy[2];			/* dummy 8 bytes */
	volatile INT32U rUHPort1PowerCtrl;	/*offset 0x10 */
	volatile INT32U rUHPort2PowerCtrl;	/*offset 0x14 */
} USB_HOST_COMMON_REGS_T;

typedef struct
{
	INT32U	event;
	INT32U	buf;
} USBH_UVC_MSG_T;

extern INT8U *g_dummybuf[HOSTNUM];
extern INT8U *g_dummybuf_m[HOSTNUM];

extern void		drv_l1_usbh_init(INT32U hostid);
extern void		drv_l1_usbh_uninit(INT32U hostid);
extern INT32U	drv_l1_usbh_detect_port(INT32U hostid);
extern INT32U	drv_l1_usbh_issue_setup(INT32U hostid, ST_USBH_CTRL *Ctrl);
extern INT32U   drv_l1_usbh_issue_control_xfer(INT32U hostid, ST_USBH_CTRL *Ctrl);
extern INT32U	drv_l1_usbh_in_token(INT32U hostid, ST_USBH_CTRL *Ctrl);
extern INT32U	drv_l1_usbh_out_token(INT32U hostid, ST_USBH_CTRL *Ctrl);
extern INT32U	drv_l1_usbh_get_sof_frame_num(INT32U hostid);
extern INT32U	drv_l1_usbh_wait_int(INT32U hostid, INT32U poll_bit);
extern void		drv_l1_usbh_wait_sof_tick(INT32U hostid, INT32U sofnum);
extern void		drv_l1_usbh_issue_reset(INT32U hostid, INT32U tick);
extern void		drv_l1_usbh_register_host_operation_table(INT32U hostid, USBH_OPERATION_TBL *op_tbl);
extern void		drv_l1_usbh_phy_clk_on(void);
extern void		drv_l1_usbh_phy_clk_off(void);
extern INT8U    drv_l1_usbh_hosttype(INT32U hostid);
extern uint8_t  get_curr_abuf_index(INT32U hostid);
extern uint8_t  get_curr_bbuf_index(INT32U hostid);
extern void     begin_getting_iso_data(INT32U hostid);
extern void     pause_getting_iso_data(INT32U hostid);
extern INT8U    get_iso_data_status(INT32U hostid);
/**
* @brief 		USB host interrupt service function.
* @param 		hostid[in]: host controller's id.
* @param 		do_suspend: enable/disable usb PHY suspend
* @return		None.
*/
extern void drv_l1_usbh_uphy_suspend(INT32U hostid, INT8U do_suspend);

#if UPHY_SETTING == 1
extern void drv_l1_usbh_uphy_config(INT32U usbid);
#endif
extern void drv_l1_usbh_wait_ms(INT32U ms);
#endif /*__DRV_L1_USBH_H__*/

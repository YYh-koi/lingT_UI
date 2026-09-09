#ifndef __DRV_l1_USBH_OHCI_H__
#define __DRV_l1_USBH_OHCI_H__

#include "drv_l1_usbh.h"
#if HCI_DEV_CLASS == HCI_UVC_CLASS
#include "usbh_uvc.h"
#endif

#define HCD_MAX_ENDPOINT	8
#define MAX_GTD             8
#define MAX_ITD             8

#define PID_SETUP			0
#define PID_OUT				1
#define PID_IN				2

#define DATA0				2
#define DATA1				3

/* roothub.portstatus [i] bits */
#define RH_PS_CCS	0x00000001						/* current connect status */
#define RH_PS_PES	0x00000002						/* port enable status*/
#define RH_PS_PSS	0x00000004						/* port suspend status */
#define RH_PS_POCI	0x00000008						/* port over current indicator */
#define RH_PS_PRS	0x00000010						/* port reset status */
#define RH_PS_PPS	0x00000100						/* port power status */
#define RH_PS_LSDA	0x00000200						/* low speed device attached */
#define RH_PS_CSC	0x00010000						/* connect status change */
#define RH_PS_PESC	0x00020000						/* port enable status change */
#define RH_PS_PSSC	0x00040000						/* port suspend status change */
#define RH_PS_OCIC	0x00080000						/* over current indicator change */
#define RH_PS_PRSC	0x00100000						/* port reset status change */

/********************* Define rHcControl bit mask (Offset + 0x04) *****************/
#define MASK_OHCI_HCCONTROL_CBSR	(BIT0 | BIT1)	/* Control Bulk Service Ratio */
#define MASK_OHCI_HCCONTROL_PLE		BIT2			/* PeriodicList Enable */
#define MASK_OHCI_HCCONTROL_IE		BIT3			/* Isochronous Enable */
#define MASK_OHCI_HCCONTROL_CLE		BIT4			/* Control List Enable */
#define MASK_OHCI_HCCONTROL_BLE		BIT5			/* Bulk List Enable */
#define MASK_OHCI_HCCONTROL_HCFS	(BIT6 | BIT7)	/* Host Controller Functional State */
#define MASK_OHCI_HCCONTROL_IR		BIT8			/* Interrupt Routing */
#define MASK_OHCI_HCCONTROL_RWC		BIT9			/* Remote Wakeup Connected */
#define MASK_OHCI_HCCONTROL_RWE		BIT10			/* Remote Wakeup Enable */

/********************* Define rHcCommandStatus bit mask (Offset + 0x08) *****************/
#define MASK_OHCI_HCCOMMAND_STATUS_HCR	BIT0		/* Host Controller Reset */
#define MASK_OHCI_HCCOMMAND_STATUS_CLF	BIT1		/* Control List Filled */
#define MASK_OHCI_HCCOMMAND_STATUS_BLF	BIT2		/* Bulk List Filled */
#define MASK_OHCI_HCCOMMAND_STATUS_OCR	BIT3		/* Ownership Change Request */
#define MASK_OHCI_HCCOMMAND_STATUS_SOC	(BIT16 | BIT17)			/* Scheduleing Overrun Count */

/********************* Define rHcInterruptStatus bit mask (Offset + 0x0C) *****************/
#define MASK_OHCI_INT_SCHEDULINGOVERRUN		BIT0				/* Scheduling Overrun			*/
#define MASK_OHCI_INT_WRITEBACKDONEHEAD		BIT1				/* Writeback DoneHead			*/
#define MASK_OHCI_INT_STARTOFFRAME			BIT2				/* Start of Frame				*/
#define MASK_OHCI_INT_RESUMEDETECTED		BIT3				/* Resume Detect				*/
#define MASK_OHCI_INT_UNRECOVERABLEERROR	BIT4				/* Unrecoverable error			*/
#define MASK_OHCI_INT_FRAMENUMBEROVERFLOW	BIT5				/* Frame Number Overflow		*/
#define MASK_OHCI_INT_ROOTHUBSTATUSCHANGE	BIT6				/* Root Hub Status Change		*/
#define MASK_OHCI_INT_OWNERSHIPCHANGE		BIT30				/* Ownership Change				*/
#define MASK_OHCI_INT_MASTERINTERRUPTENABLE BIT31				/* Master Interrupt Enable		*/
#define MASK_OHCI_INT_ALL					0xC000007FUL		/* All interrupts				*/

/********************* Define rHcInterruptEnable bit mask (Offset + 0x10) *****************/
#define MASK_OHCI_INTEN_SCHEDULINGOVERRUN		BIT0			/* Scheduling Overrun			*/
#define MASK_OHCI_INTEN_WRITEBACKDONEHEAD		BIT1			/* Writeback DoneHead			*/
#define MASK_OHCI_INTEN_STARTOFFRAME			BIT2			/* Start of Frame				*/
#define MASK_OHCI_INTEN_RESUMEDETECTED			BIT3			/* Resume Detect				*/
#define MASK_OHCI_INTEN_UNRECOVERABLEERROR		BIT4			/* Unrecoverable error			*/
#define MASK_OHCI_INTEN_FRAMENUMBEROVERFLOW		BIT5			/* Frame Number Overflow		*/
#define MASK_OHCI_INTEN_ROOTHUBSTATUSCHANGE		BIT6			/* Root Hub Status Change		*/
#define MASK_OHCI_INTEN_OWNERSHIPCHANGE			BIT30			/* Ownership Change				*/
#define MASK_OHCI_INTEN_MASTERINTERRUPTENABLE	BIT31			/* Master Interrupt Enable		*/
#define MASK_OHCI_INTEN_ALL						0xC000007FUL	/* All interrupts				*/

/********************* Define rHcRhStatus bit mask (Offset + 0x50) *****************/
#define MASK_OHCI_HCRHSTATUS_LOCALPOWERSTATUSCHANGE BIT16		/* Host Controller Functional State */

/********************* Define rHcRhPortStatus1 bit mask (Offset + 0x54) *****************/
#define MASK_OHCI_RH_PS_CCS		BIT0				/* current connect status */
#define MASK_OHCI_RH_PS_PES		BIT1				/* port enable status */
#define MASK_OHCI_RH_PS_PSS		BIT2				/* port suspend status */
#define MASK_OHCI_RH_PS_POCI	BIT3				/* port over current indicator */
#define MASK_OHCI_RH_PS_PRS		BIT4				/* port reset status */
#define MASK_OHCI_RH_PS_PPS		BIT8				/* port power status */
#define MASK_OHCI_RH_PS_LSDA	BIT9				/* low speed device attached */
#define MASK_OHCI_RH_PS_CSC		BIT16				/* connect status change */
#define MASK_OHCI_RH_PS_PESC	BIT17				/* port enable status change */
#define MASK_OHCI_RH_PS_PSSC	BIT18				/* port suspend status change */
#define MASK_OHCI_RH_PS_OCIC	BIT19				/* over current indicator change */
#define MASK_OHCI_RH_PS_PRSC	BIT20				/* port reset status change */

/********************* Define ISO IN control bit mask (Offset + 0x1C0) *****************/
#define MASK_OHCI_ISOIN_CTRL_HW_EN			BIT0	/* Enable OHCI_ISOIN HW Function */
#define MASK_OHCI_ISOIN_CTRL_HEADER_UPDATE	BIT1	/* Update OHCI ISO_IN Header Value at ISOIN_HEADER */
#define MASK_OHCI_ISOIN_CTRL_LOAD_SADDR_SW	BIT3	/* Which buf to do, 0->A buf, 1->B buf */
#define MASK_OHCI_ISOIN_CTRL_BUFBOUND_EN	BIT8	/* Buffer bound function enable */

/********************* Define ISO IN frame control interrupt enable bit mask (Offset + 0x1D4) *****************/
#define MASK_OHCI_ISOIN_A_BUF_DONE_EN	BIT0		/* Frame A buffer done interrupt enable */
#define MASK_OHCI_ISOIN_B_BUF_DONE_EN	BIT1		/* Frame B buffer done interrupt enable */
#define MASK_OHCI_ISOIN_OVERFLOW_EN	    BIT2
#define MASK_OHCI_ISOIN_OVERBOUND_EN	BIT4		/* Frame A/B buffer overbound interrupt enable*/
/********************* Define ISO IN frame control interrupt bit mask (Offset + 0x1D8) *****************/
#define MASK_OHCI_ISOIN_A_BUF_DONE_INT	BIT0		/* Frame A buffer done interrupt */
#define MASK_OHCI_ISOIN_B_BUF_DONE_INT	BIT1		/* Frame B buffer done interrupt */
#define MASK_OHCI_ISOIN_OVERFLOW_INT	BIT2
#define MASK_OHCI_ISOIN_OVERBOUND_INT	BIT4		/* Frame A/B buffer overbound interrupt */

#define HC_HOST_RESET					0x00		/* Reset state */
#define HC_HOST_RESUME					0x01		/* Resume state	*/
#define HC_HOST_OPERATIONAL				0x02		/* Operational state */
#define HC_HOST_SUSPEND					0x03		/* Suspend state */

/* Completion Codes */
#define TD_CC_NOERROR					0x0			/* 0000 */
#define TD_CC_CRC						0x1			/* 0001 */
#define TD_CC_BITSTUFFING				0x2			/* 0010 */
#define TD_CC_DATATOOGLEMISMATCH		0x3			/* 0011 */
#define TD_CC_STALL						0x4			/* 0100 */
#define TD_CC_DEVICENOTRESPONDING		0x5			/* 0101 */
#define TD_CC_PIDCHECKFAILURE			0x6			/* 0110 */
#define TD_CC_UNEXPECTEDPID				0x7			/* 0111 */
#define TD_CC_DATAOVERRUN				0x8			/* 1000 */
#define TD_CC_DATAUNDERRUN				0x9			/* 1011 */
#define TD_CC_BUFFEROVERRUN				0xC			/* 1100 */
#define TD_CC_BUFFERUNDERRUN			0xD			/* 1101 */
#define TD_CC_NOTACCESSED				0xE			/* 111x */
#define OHCI_PORT_NOT_EXIST				0xFF

#define DWORD_LOWWORD_CC_BIT_START		12
#define DWORD_LOWWORD_CC_BIT_END		15
#define DWORD_HIGHWORD_CC_BIT_START		28
#define DWORD_HIGHWORD_CC_BIT_END		31

#define HC_FRAME_INTERVAL				0x2EDF		/* 11999 for frame interval */

#define OHCI_HOST_BUF_ADDRESS_ALIGNMENT 4096
#define OHCI_HOST_HCCA_ALIGNMENT		256
#define OHCI_HOST_ITD_ALIGNMENT			32
#define OHCI_HOST_ITD_DES_SIZE			32
#define OHCI_HOST_HCCA_INT_ED_NUM		32

#define OHCI_SETUP_CMD_DATA_BUF_LEN		4096
#define OHCI_WAIT_INT_CNT	50000000
#define OHCI_WAIT_INT_TICK	100

#define FULL_SPEED          0
#define LOW_SPEED           1

#define SCHED_CTRL          0
#define SCHED_BULK          1
#define SCHED_ISOCH         2
#define SCHED_INT           3
/**************************************************************************
*                          D A T A    T Y P E S                           *
**************************************************************************/
typedef void (*USBH_L1_OHCI_ISO_FRAME_CBK) (INT32U);
typedef void (*USBH_L1_OHCI_HID_INTIN_CBK) (INT32U);

enum
{
	OHCI_HOST_NONE_EVENT,
	OHCI_HOST_ISO_FRAME_A_DONE_EVENT,
	OHCI_HOST_ISO_FRAME_A_OVERFLOW_EVENT,
	OHCI_HOST_ISO_FRAME_B_DONE_EVENT,
	OHCI_HOST_ISO_FRAME_B_OVERFLOW_EVENT,
	OHCI_HOST_ISO_TASK_TIMEOUT_EVENT,
	OHCI_HOST_ISO_TD_DONE_EVENT,
	OHCI_HOST_PORT_CONNECTED_STATUS_EVENT,
	OHCI_HOST_PORT_DISCONNECTED_STATUS_EVENT
};

enum
{
	OHCI_TRANSFER_CONTROL_TYPE,
	OHCI_TRANSFER_BULK_TYPE,
	OHCI_TRANSFER_ISO_TYPE,
	OHCI_TRANSFER_INTERRUPT_TYPE
};

typedef struct OHCI_ED_S
{
	/*---------- Word 1 ----------*/
	volatile INT32U FunctionAddr   : 7;
	volatile INT32U EndpointNumber : 4;
	volatile INT32U Direction      : 2;
	volatile INT32U Speed          : 1;
	volatile INT32U Skip           : 1;
	volatile INT32U Format         : 1;
	volatile INT32U MaxPacketSize : 11;
	volatile INT32U                : 0;/* Force next member on next storage unit */
	/*---------- End Word 1 ----------*/

	/*---------- Word 2 ----------*/
	volatile INT32U TailP;	// only 28 bits - 16B align

	/*---------- Word 3 ----------*/
	union {
		 volatile INT32U HeadTD;
		 struct {
			volatile INT32U Halted         : 1;
			volatile INT32U ToggleCarry    : 1;
			volatile INT32U                : 30;
		};

	} HeadP;/* TODO remove this name */

	/*---------- Word 4 ----------*/
	volatile INT32U NextED;// only 28 bits - 16B align
} __attribute__ ((aligned(16))) OHCI_ED_T, *pOHCI_ED_T;

typedef struct OHCI_GTD_S
{
	union
	{
		volatile INT32U Control_Val;
		struct
		{
			INT32U	dummy : 18;
			INT32U	BufferRounding : 1;
			INT32U	DirectionPID : 2;
			INT32U	DelayInterrupt : 3;
			INT32U	DataToggle : 2;
			INT32U	ErrorCount : 2;
			INT32U	CondtionCode : 4;
		} Control_BF;
	} Control;
	volatile INT32U CurrBufP;
	volatile INT32U NextTD;
	volatile INT32U BufEnd;
} __attribute__ ((aligned(16))) OHCI_GTD_T, *pOHCI_GTD_T;

typedef struct OHCI_ISO_TD_S
{
	/*---------- Word 1 ----------*/
	volatile INT32U StartingFrame : 16;
	volatile INT32U dummy         : 5;
	volatile INT32U DelayInterrupt : 3;
	volatile INT32U FrameCount    : 3;
	volatile INT32U dummy1        : 1;
	volatile INT32U ConditionCode : 4;
	volatile INT32U CurrBufP;
	volatile INT32U NextTD;
	volatile INT32U BufEnd;
	volatile INT16U OffsetPSW[8];
} __attribute__ ((aligned(32))) OHCI_ISO_TD_T, *pOHCI_ISO_TD_T;


typedef struct _Setup_Request
{
	INT8U	bmRequestType;
	INT8U	bRequest;
	INT16U	wValue;
	INT16U	wIndex;
	INT16U	wLength;
} SetupMSG;

typedef struct _OHCI_Hcca
{
	volatile INT32U IntTable[32];
	volatile INT16U FrameNum;
	volatile INT16U Pad1;
	volatile INT32U DoneHead;
	volatile INT8U	reserved[116];
	volatile INT8U	padding[4];
} __attribute__ ((aligned(256))) HCCA;


typedef struct ohci_host_data{
    HCCA            hcca;
    OHCI_ED_T       ed[HCD_MAX_ENDPOINT];
    OHCI_GTD_T      gtd[MAX_GTD+1];
    OHCI_ED_T       static_int_ed[4];
    OHCI_ISO_TD_T   itd[MAX_ITD];
}ohci_host_data;

//=============================================================
typedef struct  ohci_regs {
	volatile INT32U rHcRevision;			/* offset 0x00 */
	volatile INT32U rHcControl;				/* offset 0x04 */
	volatile INT32U rHcCommandStatus;		/* offset 0x08 */
	volatile INT32U rHcInterruptStatus;		/* offset 0x0C */
	volatile INT32U rHcInterruptEnable;		/* offset 0x10 */
	volatile INT32U rHcInterruptDisable;	/* offset 0x14 */
	volatile INT32U rHcHCCA;				/* offset 0x18 */
	volatile INT32U rHcPeriodCuttendED;		/* offset 0x1C */
	volatile INT32U rHcControlHeadED;		/* offset 0x20 */
	volatile INT32U rHcControlCurrentED;	/* offset 0x24 */
	volatile INT32U rHcBulkHeadED;			/* offset 0x28 */
	volatile INT32U rHcBulkCurrentED;		/* offset 0x2C */
	volatile INT32U rHcDoneHead;			/* offset 0x30 */
	volatile INT32U rHcFmInterval;			/* offset 0x34 */
	volatile INT32U rHcFmRemaining;			/* offset 0x38 */
	volatile INT32U rHcFmNumber;			/* offset 0x3C */
	volatile INT32U rHcPeriodicStart;		/* offset 0x40 */
	volatile INT32U rHcLSThreshold;			/* offset 0x44 */
	volatile INT32U rHcRhDescriptorA;		/* offset 0x48 */
	volatile INT32U rHcRhDescriptorB;		/* offset 0x4C */
	volatile INT32U rHcRhStatus;			/* offset 0x50 */
	volatile INT32U rHcRhPortStatus1;		/* offset 0x54 */
	volatile INT32U rHcRhPortStatus2;		/* offset 0x58 */
}ohci_regs_t;

typedef struct ohci_hwiso_regs
{
    volatile uint32_t  ohc_isoin_hw_ctrl;
    volatile uint32_t  ohc_isoin_hw_initsaddra;
    volatile uint32_t  ohc_isoin_hw_initsaddrb;
    volatile uint32_t  ohc_isoin_hw_saddra;
    volatile uint32_t  ohc_isoin_hw_saddrb;
    volatile uint32_t  ohc_isoin_hw_inten;
    volatile uint32_t  ohc_isoin_hw_int;
    volatile uint32_t  ohc_isoin_hw_bufbound;
    volatile uint32_t  ohc_isoin_hw_header0;
    volatile uint32_t  ohc_isoin_hw_header1;
    volatile uint32_t  ohc_isoin_hw_header2;
} ohci_hwiso_regs_t;


typedef void(*OHCI_LOWSPEED_CALLBACK_FUN)(void);

extern USBH_OPERATION_TBL	ohci_op_tbl;
extern INT32U l1_ohci_tick;

extern void					drv_l1_usbh_ohci_reset_rhub_port(INT32U hostid);
extern void					drv_l1_usbh_ohci_iso_update_frame_buf(INT32U hostid, INT32U abuf, INT32U bbuf);
extern void					drv_l1_usbh_ohci_iso_set_ep_number(INT32U hostid, INT8U num);
extern void					drv_l1_usbh_ohci_iso_enable_buf_size(INT32U hostid, INT8U enable, INT32U size);
extern void					drv_l1_usbh_ohci_iso_register_frame_handler(USBH_L1_OHCI_ISO_FRAME_CBK handler);
extern void                 drv_l1_usbh_ohci_register_set_lowspeed_cbk(void* cbk_fun);
extern void                 drv_l1_usbh_ohci_set_lowspeed_callback(void);
extern void                 drv_l1_usbh_ohci_WBDH_intr_en(INT32U hostid);
extern INT32U	            drv_l1_usbh_ohci_detect_port(INT32U hostid);
extern void                 drv_l1_usbh_ohci_intin_register_handler(USBH_L1_OHCI_HID_INTIN_CBK handler);
extern void                 drv_l1_usbh_ohci_disable_schedule(INT32U hostid, INT8U sched);
extern void                 drv_l1_usbh_ohci_enable_schedule(INT32U hostid, INT8U sched);
extern INT32U               drv_l1_usbh_ohci_wait_int(INT32U hostid, INT32U uiPollBit);
extern void                 drv_l1_usbh_ohci_wait_sof_tick(INT32U hostid, INT32U sofnum);
extern INT32U               drv_l1_usbh_ohci_issue_setup(INT32U hostid, ST_USBH_CTRL *Ctrl);
extern INT32U               drv_l1_usbh_ohci_issue_control_xfer(INT32U hostid, ST_USBH_CTRL *Ctrl);
extern INT32U               drv_l1_usbh_ohci_out_token(INT32U hostid, ST_USBH_CTRL *Ctrl);
extern INT32U               drv_l1_usbh_ohci_in_token(INT32U hostid, ST_USBH_CTRL *Ctrl);
extern INT32U               drv_l1_usbh_ohci_check_device_plug(INT32U hostid);
extern INT32U               drv_l1_usbh_ohci_issue_reset(INT32U hostid, INT32U uiTick);
#if HCI_DEV_CLASS == HCI_UVC_CLASS
extern uint32_t             drv_l1_usbh_ohci_iso_hw_get_a_new_bufa(INT32U hostid);
extern uint32_t             drv_l1_usbh_ohci_iso_hw_get_a_new_bufb(INT32U hostid);
extern void                 drv_l1_usbh_ohci_iso_register_frame_handler(USBH_L1_OHCI_ISO_FRAME_CBK handler);
#endif
extern void                 OHCI_IRQHandler(INT32U hostid);
extern void                 drv_l1_usbh_ohci_reset_rhub_port(INT32U hostid);
extern void                 drv_l1_usbh_ohci_intin_register_handler(USBH_L1_OHCI_HID_INTIN_CBK handler);
extern INT32U               drv_l1_usbh_ohci_init(INT32U hostid);
extern void                 drv_l1_usbh_ohci_uninit(INT32U hostid);
extern INT32U               drv_l1_usbh_ohci_chk_current_connect_status(INT32U hostid);
extern INT32U               drv_l1_usbh_ohci_detect_port(INT32U hostid);
extern INT32U               drv_l1_usbh_ohci_get_sof_frame_num(INT32U hostid);
#if HCI_DEV_CLASS == HCI_UVC_CLASS
extern void drv_l1_usbh_ohci_enable_iso_get_frame(INT32U hostid,  ST_UVC_INFO *uvc, INT32U enable);
extern void drv_l1_usbh_ohci_set_iso_hw_frame_buf(INT32U hostid, INT8U* abuf, INT8U* bbuf);
extern void drv_l1_usbh_ohci_iso_update_frame_buf(INT32U hostid, INT32U abuf, INT32U bbuf);
extern void drv_l1_usbh_ohci_iso_set_ep_number(INT32U hostid, INT8U num);
extern void drv_l1_usbh_ohci_iso_enable_buf_size(INT32U hostid, INT8U enable, INT32U size);
extern void drv_l1_usbh_ohci_iso_hw_disable(INT32U hostid);
extern void drv_l1_usbh_ohci_iso_hw_enable(INT32U hostid, ST_UVC_INFO *uvc);
extern uint32_t drv_l1_usbh_ohci_iso_hw_set_new_bufa(INT32U hostid, INT32U buf);
extern uint32_t drv_l1_usbh_ohci_iso_hw_set_new_bufb(INT32U hostid, INT32U buf);

#endif
extern void drv_l1_usbh_ohci_WBDH_intr_en(INT32U hostid);
extern void drv_l1_usbh_ohci_disable_schedule(INT32U hostid, INT8U sched);
extern void drv_l1_usbh_ohci_enable_schedule(INT32U hostid, INT8U sched);
#endif /*__DRV_l1_USBH_OHCI_H__*/

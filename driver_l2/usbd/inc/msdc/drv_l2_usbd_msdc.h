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
#ifndef DRV_L2_USBD_MSDC_H
#define DRV_L2_USBD_MSDC_H
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define USBD_BUFFER_USE_SRAM    0 // williamyeo
#define USBD_TASK_USE_HIGH      1 // williamyeo


#define USBD_MSDC_HIDKEY_DEVICE		1

#if (USBD_MSDC_HIDKEY_DEVICE == 1)
#define CONFIG_DES_TOTAL_INF	0x02	/* MSDC + HID */
#define CONFIG_DES_TOTAL_LEN	0x39
#define USBD_HID_KEYBOARD_INTF	0x01
#define USBD_HID_REPORT_DES_LEN	0x3F	/* The length of REPORT descriptor */

/* keyborad definition */
#define USBD_HID_KEY_A		0x04
#define USBD_HID_KEY_B		0x05
#define USBD_HID_KEY_C		0x06
#define USBD_HID_KEY_D		0x07
#define USBD_HID_KEY_E		0x08
#define USBD_HID_KEY_F		0x09
#define USBD_HID_KEY_G		0x0A
#define USBD_HID_KEY_H		0x0B
#define USBD_HID_KEY_UP		0x52
#define USBD_HID_KEY_DOWN	0x51
#define USBD_HID_KEY_LEFT	0x50
#define USBD_HID_KEY_RIGHT	0x4F
#define USBD_HID_KEY_1		0x1E
#define USBD_HID_KEY_2		0x1F
#define USBD_HID_KEY_3		0x20
#define USBD_HID_KEY_4		0x21
#define USBD_HID_KEY_5		0x22
#define USBD_HID_KEY_6		0x23
#define USBD_HID_KEY_7		0x24
#define USBD_HID_KEY_8		0x25

#else
#define CONFIG_DES_TOTAL_INF	0x01	/* MSDC */
#define CONFIG_DES_TOTAL_LEN	0x20
#endif

/* LUN number, 0~7, total is 8 lun */
#define LUN_NUM_0	0
#define	LUN_NUM_1	1
#define LUN_NUM_2	2
#define LUN_NUM_3	3
#define LUN_NUM_4   4
#define LUN_NUM_5	5
#define LUN_NUM_6	6
#define LUN_NUM_7	7

/* GPA7xxxA file system only support 5 storages, NAND/SD/CDROM/NOR/USBH */
#define LUN_NF_TYPE		    0
#define LUN_SDC_TYPE   	    1
#define LUN_CDROM_TYPE 	    2
#define LUN_NOR_TYPE   	    3
#define LUN_RAM_DISK_TYPE 	4

#define BOT_CBW_LEN 			31
#define BOT_CBWCB_LEN 			16    /* CBWCB length */
#define BOT_CSW_LEN 			13
#define BOT_CBW_CBWCB_OFFSET 	15

#define BOT_CBW_SIGN1   0x55
#define BOT_CBW_SIGN2   0x53
#define BOT_CBW_SIGN3   0x42
#define BOT_CBW_SIGN4   0x43

#define BOT_CSW_SIGN1   0x55
#define BOT_CSW_SIGN2   0x53
#define BOT_CSW_SIGN3   0x42
#define BOT_CSW_SIGN4   0x53

#define BOT_TEMP_DATA_LEN   512     /* less than 512 for none DMA mode */

/**** define for bulk IN/OUT DMA buffer transport *****/
#define MSDC_BLOCK_SIZE   512
#define BOT_DMA_RAM_BUF_ORDER   0   /* 0=512, 1=1024, 2=2048, 3=4096, 4=8192 5=16384*/
#define BOT_DMA_RAM_BUF_SCALE   (1 << BOT_DMA_RAM_BUF_ORDER)
#define BOT_DMA_RAM_BUF_SIZE     (MSDC_BLOCK_SIZE * BOT_DMA_RAM_BUF_SCALE)
#define BOT_DMA_BUF_NUM   2         /* Now support 2 DMA AB buffer */
#define CBW_DEVICE_BLOCK_SIZE   512
#define MAX_MSDC_LUN_NUM 5               /* For maximum LUN number */

/* SCSI Inquiry length definition */
#define BOT_SCSI_InquiryLength 44

/* Bulk only transport stage */
#define BULK_CBW_STA		0
#define BULK_DATA_STA	    1
#define BULK_CSW_STA		2

#define	CBW_DATA_DIR_OUT	0
#define CBW_DATA_DIR_IN		1

/* For CBW command status */
#define	CSW_CMD_PASS_STA		0x00
#define	CSW_CMD_FAILED_STA 		0x01
#define CSW_CMD_PHASE_ERROR_STA	0x02

/* For SCSI command opcode */
#define SCSI_TESTUNITREADY_OPCODE		0x00
#define SCSI_REQUSETSENSE_OPCODE			0x03
#define SCSI_INQUIRY_OPCODE				0x12
#define SCSI_MODESENSE6_OPCODE			0x1A
#define SCSI_STARTSTOP_OPCODE			0x1B
#define SCSI_MEDIUMREMOVAL_OPCODE		0x1E
#define SCSI_READFORMATCAPACITY_OPCODE	0x23
#define SCSI_READCAPACITY_OPCODE			0x25
#define SCSI_READ10_OPCODE				0x28
#define SCSI_WRITE10_OPCODE				0x2A
#define SCSI_VERIFY_OPCODE				0x2F
#define SCSI_READTOC_OPCODE				0x43
#define SCSI_MODESENSE10_OPCODE			0x5A
#define SCSI_GP_VENDOR_START_OPCODE      0xF0
#define SCSI_GP_VENDOR_END_OPCODE        0xFF

/* For LUN status */
#define LUNSTS_NORMAL				0x0000
#define LUNSTS_NOMEDIA				0x0001
#define LUNSTS_WRITEPROTECT			0x0002
#define LUNSTS_MEDIACHANGE			0x0004
#define LUNSTS_START_STOP_NOMEDIA	0xFFAA

/* For DMA buffer state */
#define SCSI_DMA_BUF_NOT_USED_STA    0
#define SCSI_DMA_BUF_READING_STA     1
#define SCSI_DMA_BUF_READ_DONE_STA   2
#define SCSI_DMA_BUF_WRITING_STA     3
#define SCSI_DMA_BUF_WRITE_DONE_STA  4
#define SCSI_DMA_BUF_INING_STA       5
#define SCSI_DMA_BUF_IN_DONE_STA     6
#define SCSI_DMA_BUF_OUTING_STA      7
#define SCSI_DMA_BUF_OUT_DONE_STA    8

/* For USB MSDC to check status of storage */
#define STORAGE_CHECK_READY          0
#define STORAGE_CHECK_NOT_READY      1
#define STORAGE_CHECK_FAILED         2

/* For SCSI read/write state */
#define DO_SCSI_NONE           0
#define DO_SCSI_READ_10        1
#define DO_SCSI_WRITE_10       2
#define DO_SCSI_VENDOR_READ    3
#define DO_SCSI_VENDOR_WRITE   4

/* For GP SCSI vendor cmd state */
#define GP_VENDER_NO_AUTH       0
#define GP_VENDER_START_AUTH    1
#define GP_VENDER_OK_AUTH       2

#define DMA_BULK_IN_MODE	1
#define DMA_BULK_OUT_MODE	2

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef void (*SCSI_VENDOR_CMD_FN)(INT32U, INT16U); /* for vendor command usage */
typedef INT32S (*SCSI_VENDOR_READ_WRITE_FN)(INT32U, INT32U, void*); /* for vendor command read/write, including NAND, SPI, OTP */
typedef void (*SCSI_VENDOR_CHECK_FN)(void);  /* for vendor check */

typedef struct _STORAGE_DRV_INFO
{
	INT32U nSectors;
	INT32U nBytesPerSector;
} STORAGE_DRV_INFO;

typedef struct _MDSC_LUN_STORAGE_DRV
{
    void* 	priv;
    INT32S	(*usbd_msdc_init)(void* priv);
    INT32S	(*usbd_msdc_uninit)(void *priv);
    INT32S	(*usbd_msdc_insert_event)(void* priv);
    void 	(*usbd_msdc_getdrvinfo)(void* priv, STORAGE_DRV_INFO* info);
    INT32S	(*usbd_msdc_read_cmd_phase)(void* priv, INT32U lba,INT32U seccount);
    INT32S	(*usbd_msdc_read_dma_phase)(void* priv, INT32U *buf,INT8U ifwait,INT32U seccount);
    INT32S 	(*usbd_msdc_read_cmdend_phase)(void* priv);
    INT32S	(*usbd_msdc_write_cmd_phase)(void* priv, INT32U lba,INT32U seccount);
    INT32S	(*usbd_msdc_write_dma_phase)(void* priv, INT32U *buf,INT8U ifwait,INT32U seccount);
    INT32S 	(*usbd_msdc_write_cmdend_phase)(void* priv);
    INT32S 	(*usbd_msdc_storage_dma_check_state)(void* priv);
    INT32S  (*usbd_msdc_stopunit_userfunc)(void* priv);
} MDSC_LUN_STORAGE_DRV;


typedef struct _MSDC_LOGIC_UNIT
{
    INT32U  senseidx;               /* Indicate which sense code in the table */
    INT32U 	secSize;		        /* flash size sector */
    INT32U  wprotect;				/* write protect */
    INT16U 	status; 			    /* Status, write protect, no media, media change. */
    INT8U   isused;                 /* This lun is set by application */
    INT8U   luntype;                     /* not useful now */
    MDSC_LUN_STORAGE_DRV storageaccess;  /* storage access functions */
} MSDC_LOGIC_UNIT;

typedef struct _USBD_MSDC_DMA_BUF
{
    INT8U * dataptr;
    struct _USBD_MSDC_DMA_BUF* next;
    INT32U usedbuffsize;            /* How many bytes of this RAM buffer are used */
    volatile INT32U DMABufState;
} USBD_MSDC_DMA_BUF;

typedef struct _SCSI_SENSE_SET
{
    INT8U sensekey;
    INT8U asc;
    INT8U ascq;
    INT8U Reserve;           /* for structure alignment */
} SCSI_SENSE_SET;

typedef union _CBW_CB
{
    INT8U cbw_raw[BOT_CBW_LEN];
    struct CBW
    {
        INT8U dCBWSignature[4];
        INT8U dCBWTag[4];
        INT8U dCBWDataTransferLength[4];
        INT8U bmCBWFlags;
        INT8U bCBWLUN;
        INT8U bCBWCBLength;
        INT8U CBWCB[BOT_CBWCB_LEN];
    } cbw_cb;
} CBW_CB;

typedef struct _CSW_CB
{
    INT32U dCSWDataResidue;
    INT8U bCSWStatus;   /* Passed, failed, error */
    INT8U reserve[3];   /* for structure alignment */
} CSW_CB;

typedef struct _USBD_MSDC_CTL_BLK
{
    USBD_MSDC_DMA_BUF* dmaptr;      /* for DMA usage */
    MSDC_LOGIC_UNIT*  lunptr;       /* for LUN usage */
    INT8U *   dataptr;                /* for SCSI command usage */
    INT32U  rambufcnt;				/* how many DMA ram buffer for standard SCSI command */
    INT32U  rambufres;				/* not enough for a DMA ram buffer for standard SCSI command */
    INT32U  secnum;
    INT32U  cbwdatatcnt;
    INT32U  cswdataresidue;
    INT8U *   SCSIinquirydataptr[MAX_MSDC_LUN_NUM];     /* for SCSI inquiry data */
    INT8U *   SCSIinquiryCDdataptr;   /* for SCSI inquiry data, CDROM */
    INT8U *   SCSIMMC_TOC_PMA_ATIP_dataptr;
    volatile INT32U   doingSCSIreadwrite;
    volatile INT8U   prostate;      /* CBW, Data, CSW */
    INT8U   senseidx;               /* SCSI sense index */
    INT8U   nodmadata;
    INT8U   maxlun;                 /* maximum lun number, 0 base */
    CBW_CB  cbw;                    /* CBW control */
    CSW_CB  csw;                    /* CSW control */
    SCSI_VENDOR_CMD_FN    process_scsi_vendor_cmd;
} USBD_MSDC_CTL_BLK;

typedef struct _USBD_MSDC_GP_VENDOR_CTL_BLK
{
    SCSI_VENDOR_READ_WRITE_FN   vnedor_read_fn;
    SCSI_VENDOR_READ_WRITE_FN   vnedor_write_fn;
    SCSI_VENDOR_CHECK_FN   vnedor_check_fn;
    INT8U*  vendordataptr;
    INT32U  isauthrized;
    INT32U  scsi_vendor_lba;
    INT32U  scsi_vendor_len;
} USBD_MSDC_GP_VENDOR_CTL_BLK;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
extern const INT8U scsi_inquirydata[];
extern const INT8U scsi_inquirydata_CDROM[];
extern USBD_MSDC_CTL_BLK msdc_ctlblk[2];
extern USBD_MSDC_DMA_BUF msdc_dma_buf[2][BOT_DMA_BUF_NUM];   /* for BULK DMA IN/OUT */

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern MSDC_LOGIC_UNIT* drv_l2_usbd_msdc_get_lun_by_type(INT32U usb_id, INT8U type);

/*
 * @brief: USB DEVICE MSDC get max lun
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: Max lun number
*/
extern INT8U drv_l2_usbd_msdc_get_max_lun(INT32U usb_id);

/*
 * @brief: check lun status, including: 1. lun number vs max lun. 2. media connection
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: 0: Success; -1: Fail
*/
extern INT8S drv_l2_usbd_msdc_check_lun_status(INT32U usb_id);

/*
 * @brief: USBD MSDC initial
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: 0: Success; -1: Fail
*/
extern INT32S drv_l2_usbd_msdc_init(INT32U usb_id);

/*
 * @brief: USBD MSDC set lun
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @param: type: Lun type(LUN_NF_TYPE/LUN_SDC_TYPE/LUN_CDROM_TYPE/LUN_NOR_TYPE/LUN_RAM_DISK_TYPE)
 * @param: lunnum: 0~7
 * @param: wprotect: 0: No write protect; 1: Write protect
 * @param: plun: Pointer of storage access functions table
 * @return: 0: Success; -1: Fail
*/
extern INT32S drv_l2_usbd_msdc_set_lun(INT32U usb_id, INT8U type, INT8U lunnum, INT32U wprotect, const MDSC_LUN_STORAGE_DRV* plun);

/*
 * @brief: USBD MSDC reset storage function table
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @param: lunnum: 0~7
 * @return: 0: Success; -1: Fail
*/
extern INT32S drv_l2_usbd_msdc_reset_lun(INT32U usb_id, INT8U lunnum);

/*
 * @brief: USBD MSDC get CBW data
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: 0: Success; -1: Fail
*/
extern INT32S drv_l2_usbd_msdc_get_cbw_data(INT32U usb_id);

/*
 * @brief: USBD MSDC uninitial
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_msdc_uninit(INT32U usb_id);

/*
 * @brief: USBD MSDC reset prostate to BULK_CBW_STA
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_msdc_reset(INT32U usb_id);

/*
 * @brief: USBD MSDC send CSW
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_msdc_send_csw_data(INT32U usb_id);

/*
 * @brief: USBD MSDC read scsi command
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_msdc_do_read_scsi_cmd(INT32U usb_id);

/*
 * @brief: USBD MSDC read LBA and length for SCSI read (10) or SCSI write(10)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @param: lba: Read LBA from CBWCB to this pointer
 * @param: len: Read length from CBWCB to this pointer
 * @param: type: 10: for SCSI READ(10) or SCSI WRITE(10); 12: for SCSI READ(12) or SCSI WRITE(12)
 * @return: None
*/
extern void drv_l2_usbd_msdc_get_read_lba_len(INT32U usb_id, INT32U* lba, INT32U* len, INT8U type);

/*
 * @brief: USBD MSDC SCSI inquiry data callback function register
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @param: dataptr: SCSI inquiry data pointer
 * @param: cddataptr: SCSI inquiry data pointer for CDROM
 * @return: None
*/
extern void drv_l2_usbd_msdc_register_scsi_inquiry_data(INT32U usb_id, INT8U *dataptr[], INT8U * cddataptr);

/*
 * @brief: USBD MSDC SCSI vendor command callback function register
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @param: fn: SCSI vendor command callback function
 * @return: None
*/
extern void drv_l2_usbd_msdc_register_scsi_vendor_cmd_fn(INT32U usb_id, SCSI_VENDOR_CMD_FN fn);

/*
 * @brief: USBD MSDC set dma buffer size
 * @param: size: DMA buffer size
 * @return: None
*/
extern void drv_l2_usbd_msdc_set_dma_buffer_size(INT32U size);

/*
 * @brief: USBD MSDC initialize lun
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern INT32S drv_l2_usbd_msdc_init_lun(INT32U usb_id);

/*
 * @brief: USBD MSDC uninitialize 1 lun
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @lunum: lun number
 * @return: None
*/
extern INT32S drv_l2_usbd_msdc_uninit_1lun(INT32U usb_id, INT32U lunum);

/* msdc state handler */

/*
 * @brief: USBD MSDC BOT(bulk only tranport) for in token
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @event: Bot event for in token
 * @return: None
*/
extern void drv_l2_usbd_msdc_state_bot_in(INT32U usb_id, INT32U event);

/*
 * @brief: USBD MSDC BOT(bulk only tranport) for out token
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @event: Bot event for out token
 * @return: None
*/
extern void drv_l2_usbd_msdc_state_bot_out(INT32U usb_id, INT32U event);

/* SCSI command handler */
/*
 * @brief: USBD MSDC process SCSI command
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_scsi_command_process(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI command: Inquiry (opcode: 12h)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_scsi_cmd_inquiry(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI command: Test Unit Ready (opcode: 00h)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_scsi_cmd_testunitready(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI command: Mode Sense 6 (opcode: 1Ah)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_scsi_cmd_modesense6(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI command: Read(10) (opcode: 28h)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_scsi_cmd_read10(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI command: Write(10) (opcode: 2Ah)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_scsi_cmd_write10(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI command: Medidum Removal(opcode: 1Eh)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_scsi_cmd_mediumremoval(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI command: Request Sense(opcode: 03h)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_scsi_cmd_requestsense(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI command: Read Capacity(opcode: 25h)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_scsi_cmd_readcapacity(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI command: Read Format Capacity(opcode: 23h)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_scsi_cmd_formatcapacity(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI command: Verify(opcode: 2Fh)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_scsi_cmd_verify(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI command: Start-stop unit(opcode: 1Bh)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_scsi_cmd_startstop(INT32U usb_id);

/* SCSI vendor command handler */

/*
 * @brief: Implementation for SCSI vendor command  (opcode: F0h~FFh)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @param: cmdval: command opcode
 * @return: None
*/
extern void drv_l2_usbd_scsi_vendor_cmd(INT32U usb_id, INT16U cmdval);

/*
 * @brief: USBD MSDC process SCSI read/write vendor command
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_usbd_msdc_porcess_vendor_readwrite(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI read/write vendor command callback function register
 * @param: read: read callback function
 * @param: write: write callback function
 * @return: None
*/
extern void drv_l2_usbd_msdc_register_vendor_read_write_fn(SCSI_VENDOR_READ_WRITE_FN read, SCSI_VENDOR_READ_WRITE_FN write);

/*
 * @brief: USBD MSDC GP vendor ram read
 * @param: addr: Source address
 * @param: len: Read length
 * @param: buf: Destination buffer pointer
 * @return: 0: Success
*/
extern INT32S drv_l2_usbd_msdc_gp_vendor_ram_read(INT32U addr, INT32U len, void* buf);

/*
 * @brief: USBD MSDC GP vendor ram read
 * @param: addr: Destination address
 * @param: len: Write length
 * @param: buf: Source buffer pointer
 * @return: 0: Success
*/
extern INT32S drv_l2_usbd_msdc_gp_vendor_ram_write(INT32U addr, INT32U len, void* buf);

/*
 * @brief: USBD MSDC SCSI set vendor ID for GP's vendor command
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void gp_scsi_vendor_cmd_set_vendor_ID(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI get driver status for GP's vendor command
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void gp_scsi_vendor_cmd_get_driver_stauts(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI get ic version for GP's vendor command
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void gp_scsi_vendor_cmd_get_ic_version(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI vendor command register read
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void gp_scsi_vendor_cmd_reg_read(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI vendor command register write
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void gp_scsi_vendor_cmd_reg_write(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI vendor command ram read
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void gp_scsi_vendor_cmd_read(INT32U usb_id);

/*
 * @brief: USBD MSDC SCSI vendor command ram write
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void gp_scsi_vendor_cmd_write(INT32U usb_id);

/*
 * @brief: USBD MSDC Bulk IN/OUT endpoint enter STALL condition
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void scsi_send_stall_null(INT32U usb_id);


/*
 * @brief: USBD MSDC SCSI Command Fail, update lun.senseidx
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @param: senseidx: index of scsi_sense_tbl[]
 * @return: None
*/
extern void scsi_command_fail(INT32U usb_id, INT32U senseidx);

/*
 * @brief: USBD MSDC SCSI command: Read TOC/PMA/ATIP(opcode: 43h)
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @return: None
*/
extern void drv_l2_SetTOC_PMA_ATIP_Data(INT32U usb_id);

/*
 * @brief: USBD MSDC Read TOC/PMA/ATIP data register
 * @param: usb_id: 0: USB_A; 1: USB_B
 * @param: dataptr: TOC/PMA/ATIP data
 * @return: None
*/
extern void drv_l2_usbd_msdc_register_scsi_MMC_TOC_PMA_ATIP_data(INT32U usb_id, INT8U *dataptr);



extern void drv_l2_usbd_msdc_process_insertion(INT32U usb_id);



#endif// end of DRV_L2_USBD_MSDC_H

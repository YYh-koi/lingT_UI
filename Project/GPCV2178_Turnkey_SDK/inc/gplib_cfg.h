#ifndef __GPLIB_CFG_H__
#define __GPLIB_CFG_H__

// console
#define GPLIB_CONSOLE_EN		0 // when 1, _DRV_L1_UART1 must 1, _DRV_L1_UART2 and other UART must 0


//=========================Engineer Mode ("Not" defined CUSTOMER_DEFINE)=====================//
#ifndef CUSTOMER_DEFINE  //For Engineer Development and test                                 //

// File system module
#define	GPLIB_FILE_SYSTEM_EN            1

// PScaler system module
#define	GPLIB_PSCALER_CMD_TEST_EN       0

// JPEG image encoding/decoding module
//#define GPLIB_JPEG_ENCODE_EN            1 // williamyeo, roy use _DRV_L1_JPEG instead
//#define GPLIB_JPEG_DECODE_EN            1 // williamyeo, roy use _DRV_L1_JPEG instead

// Progressive JPEG image decoding module
#define GPLIB_PROGRESSIVE_JPEG_EN       0

// JPEG UNION MODE
#define JPEG_DEC_UNION_EN               0

// GPZP image decoding module
#define GPLIB_GPZP_EN                   1

// GIF image decoding module
#define GPLIB_GIF_EN                    0

// BMP image decoding module
#define GPLIB_BMP_EN                    0

// PPU control module
#define GPLIB_PPU_EN                    1

// SPU control module
#define GPLIB_SPU_EN                    0

// NVRAM Support
#define GPLIB_NVRAM_EN                  1

// Print and get string
#define GPLIB_PRINT_STRING_EN           1
#define PRINT_BUF_SIZE                  512

#define GPLIB_CALENDAR_EN               1

#define SUPPORT_MIDI_LOAD_FROM_NVRAM    0

#define SUPPORT_MIDI_READ_FROM_SDRAM    0

#define SUPPORT_STG_SUPER_PLUGOUT       1

#define SUPPORT_MIDI_PLAY_WITHOUT_TASK  0

/*=== USBH detection mode ===*/
#define USBH_DP_D                       0x0001
#define USBH_GPIO_D                     0x0002
#define USBH_GPIO_D_1                   0x0003
#define USBH_SWITCH_D                   0x0004
#define USBH_GPIO_D_2                   0x0005
#if((BOARD_TYPE == BOARD_TK_V4_3) || (BOARD_TYPE ==BOARD_TK_A_V1_0) || (BOARD_TYPE == BOARD_TK_V4_4))
#define USBH_DETECTION_MODE USBH_GPIO_D
#else
#define USBH_DETECTION_MODE USBH_DP_D
#endif
//======================== Engineer Mode END ================================================//
#else                              //Engineer Difne area END                                 //
//===========================================================================================//

//=========================Customer Mode (Defined CUSTOMER_DEFINE)===========================//
//                    //For Release to customer, follow the command of "customer.h"          //
//======================== Not Modify in this Area ==========================================//

//========================= CUSTOMER DEFINE END =============================================//
#endif  // not CUSTOMER_DEFINE                                                               //
//========================= CUSTOMER DEFINE END =============================================//



#ifndef CUSTOMER_DEFINE  /* Engineer Mode */
#ifndef _FS_DEFINE_
#define _FS_DEFINE_
#define FS_NAND1                        0
#define FS_NAND2                        1
#define FS_SD1                          2
#define FS_MS   						3
#define FS_CF                           4
#define FS_XD                           5
#define FS_SD0                          6
#define FS_USBH                         6
#define FS_NOR                          7
#define FS_CDROM                        8
#define FS_NAND_APP                     9
#define FS_SD2                          10
#define FS_RAMDISK                      12
#define FS_DEV_MAX                      13	//13 if needs to run RAM disk
/* DEV MODE */
#define MSC_EN                          0
#define SD_EN                           1
#define CFC_EN                          0
#define NAND_APP_EN		                1 // UI .bin resources: enable read-only NAND application area
#define NAND_APP_WRITE_EN               0 // To enable nand app area allow write access through FS and MSDC interface.
                                          // This should 0 when nandrom.
/* Read-only U3 SPI-NOR capacity/partition probe. Set to 0 after collecting
 * the UART log; it does not erase or program flash. */
#define LVGL_UI_SPIFC_PROBE                0
/* One-time, read-only scan of every U3 4KiB erase sector. It identifies the
 * longest physically blank candidate region for the UI package. */
#define LVGL_UI_SPIFC_SCAN                 0
/* One-time SD:/UI/u3_ui.bin -> U3 raw UI partition installer. This erases
 * only 0x001B0000..0x006AFFFF, the blank region verified by the scan. */
#define LVGL_UI_SPIFC_INSTALLER            0
/* Set to 1 only while diagnosing LVGL rendering/resource loading.  Errors
 * remain visible regardless of this switch. */
#define LVGL_RUNTIME_LOG_EN                 1   //LVGL runtime log enable
#define NAND1_EN                        0
#define NAND2_EN                        0
#define NAND3_EN                        0
#define USB_NUM                         0
#define NOR_EN                          0
#define XD_EN                           0
#define RAMDISK_EN                      1
#define CDROM_EN                        0
//#define MAX_DISK_NUM     (MSC_EN + SD_EN + CFC_EN + NAND1_EN + NAND2_EN + NAND3_EN + USB_NUM + NOR_EN + RAMDISK_EN)
#define MAX_DISK_NUM                    FS_DEV_MAX
#endif // _FS_DEFINE_
#define NVRAM_V1                        1
#define NVRAM_V2                        2
#define NVRAM_V3                        3
#define NVRAM_MANAGER_FORMAT            NVRAM_V3

#define FAT_SPEEDUP_TAB_ENTRY		1024
#define SDC_CACHE_SIZE				(32*1024)

#else /* Customer Mode */

#endif

#define PRIORITY_WRITE_SD                   osPriorityNormal
#define PRIORITY_WRITE_SD_EMER              osPriorityAboveNormal

#endif 		// __GPLIB_CFG_H__

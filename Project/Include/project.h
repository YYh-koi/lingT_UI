
// when this on, use spi nand, it replace classic parallel raw nand driver except booting role, old defines
// and apis that use for classic parallel raw nand such as NAND1_EN, NAND2_EN, _DRV_L1_NAND, _DRV_L2_NAND
// NAND_APP_EN, NAND_APP_MSDC_EN, NAND_APP_WRITE_EN, also applied to the spi nand flash driver.
// Also, library have to choose lib_nand_spi_gpm4_g+ide_freertos.a instead of lib_nand_gpm4_g+ide_freertos.a
// Also, set NAND_APP_EN to 1
// Also, if spi nor mounted on same pixmux with spi nand, please remove spi nor
// Also, assign correct cs pin by using spi_nand_assign_cs, see drv_l1_init
//#define USE_NANDSPI_INSTEAD_NANDFLASH

#ifndef ___PROJECT_H___
#define ___PROJECT_H___

// Global definitions
#define TRUE			1
#define FALSE			0

#define ENABLE			1
#define DISABLE			0

#define STATUS_OK		0
#define STATUS_FAIL		-1

#define SUCCESS         0
#define FAIL            -1

#ifndef C_BTSTACK_ENABLE
#define C_BTSTACK_ENABLE 1
#endif

// Operating System definitions and config
#define _OS_NONE		0
#define _OS_UCOS2		1
#define _OS_FREERTOS	2
#define	_OPERATING_SYSTEM       _OS_FREERTOS // _OS_FREERTOS, _OS_NONE

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "typedef.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "drv_l1_sfr.h"
#include "arm.h"

#include "cmsis_os.h"
#include "customer.h"
#include "board_config.h"
#include "drv_l1_gpio.h"

extern void *gp_malloc(INT32U size);
extern void gp_free(void *ptr);
#define malloc(x)       gp_malloc(x)
#define free(x)         gp_free(x)

//extern INT32U MCLK;
//extern INT32U MHZ;

#if defined(C_RUN_ON_SPIFC) && (C_RUN_ON_SPIFC == 1)
#define C_SRAM_SHARE_TO_CPU                 1 // when 1, config sram to use by CPU, please check startup.s
#else
#define C_SRAM_SHARE_TO_CPU                 0
#endif

#if C_SRAM_SHARE_TO_CPU == 1
    #define C_SRAM_LINE_BASE_ROTATOR_SHARE_TO_CPU       1
    #define C_SRAM_ETHERNET_MAC_SHARE_TO_CPU            1
    #define C_SRAM_PPU_SHARE_TO_CPU                     1
    #define C_SRAM_FD_SHARE_TO_CPU                      1
    #define C_SRAM_GPDLA_SHARE_TO_CPU                   1
    #define C_SRAM_USBD_SHARE_TO_CPU                    1
    #define C_SRAM_PSCA_A_SHARE_TO_CPU                  1
    #define C_SRAM_PSCA_B_SHARE_TO_CPU                  1
    #define C_SRAM_SCALER_SHARE_TO_CPU                  1
#endif

#define ISRAM_START_ADDR            0xF8000000
#define ISRAM_END_ADDR              0xF8033FFF

#define UART_BAUD_RATE              115200

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define ERR_PRINT(x, args...)	print_string("==[ERR][%s:%d]==" x,__FUNCTION__,__LINE__,##args)

#ifndef DBG_PRINT
#define DBG_PRINT(...)			do{print_string(__VA_ARGS__);}while(0)
#endif

#endif //#ifndef ___PROJECT_H___

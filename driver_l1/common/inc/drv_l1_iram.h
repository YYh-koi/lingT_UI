#ifndef __drv_l1_IRAM_H_
#define __drv_l1_IRAM_H_

#define MAX_IRAM_SIZE		470			/* Maximum IRAM size is 470kB */

#define CLR_IRAM_SET		(1<<16)

/*Internal Ram Size in KBYTES*/
#define C_IRAM_SIZE_KB                   64
#define C_IRAM_SHARED_ROT_SIZE_KB        64
#define C_IRAM_SHARED_ETHER_SIZE_KB      8
#define C_IRAM_SHARED_PPU_SIZE_KB        16
#define C_IRAM_SHARED_GPDLA_SIZE_KB      64
#define C_IRAM_SHARED_USBD_A_SIZE_KB     4
#define C_IRAM_SHARED_PSCA_SIZE_KB       18
#define C_IRAM_SHARED_PSCA_B_SIZE_KB     8
#define C_IRAM_SHARED_SCA_SIZE_KB        8
#define C_IRAM_SHARED_H264_SIZE_KB       140
#define C_IRAM_SHARED_FFT_SIZE_KB        8
#define C_IRAM_SHARED_USBD_B_SIZE_KB     4
#define C_IRAM_SHARED_EXT_SIZE_KB        64

#define C_IRAM_SIZE	                    C_IRAM_SIZE_KB*1024	//internal ram0
#define C_IRAM_SHARED_ROT_SIZE	        C_IRAM_SHARED_ROT_SIZE_KB*1024		//rotator
#define C_IRAM_SHARED_ETHER_SIZE	    C_IRAM_SHARED_ETHER_SIZE_KB*1024	//ethernet
#define C_IRAM_SHARED_PPU_SIZE	        C_IRAM_SHARED_PPU_SIZE_KB*1024		//ppu
#define C_IRAM_SHARED_GPDLA_SIZE	    C_IRAM_SHARED_GPDLA_SIZE_KB*1024	//GPDLA
#define C_IRAM_SHARED_USBD_SIZE         C_IRAM_SHARED_USBD_A_SIZE_KB*1024	//USBD_A
#define C_IRAM_SHARED_PSCA_SIZE         C_IRAM_SHARED_PSCA_SIZE_KB*1024		//Pscaler
#define C_IRAM_SHARED_PSCA_B_SIZE       C_IRAM_SHARED_PSCA_B_SIZE_KB*1024	//Pscaler_B
#define C_IRAM_SHARED_SCA_SIZE          C_IRAM_SHARED_SCA_SIZE_KB*1024		//sca
#define C_IRAM_SHARED_H264_SIZE         C_IRAM_SHARED_H264_SIZE_KB*1024      //h264
#define C_IRAM_SHARED_FFT_SIZE          C_IRAM_SHARED_FFT_SIZE_KB*1024       //fft
#define C_IRAM_SHARED_USBD_B_SIZE       C_IRAM_SHARED_USBD_B_SIZE_KB*1024    //USBD_B
#define C_IRAM_SHARED_EXT_SIZE          C_IRAM_SHARED_EXT_SIZE_KB*1024       //external ram


typedef enum{
	IRAM_0,				 /* 0 */
	IRAM_SHARED_ROT,	 /* 1 */
	IRAM_SHARED_ETHER,	 /* 2 */
	IRAM_SHARED_PPU,	 /* 3 */
	DUMMY1,              /* 4, dummy */
	IRAM_SHARED_GPDLA,	 /* 5 */
	IRAM_SHARED_USBD,	 /* 6 */
	IRAM_SHARED_PSCA,	 /* 7 */
	IRAM_SHARED_PSCA_B,	 /* 8 */
	IRAM_SHARED_SCA,	 /* 9 */
	IRAM_SHARED_FFT,     /* 10*/
	IRAM_SHARED_H264,    /* 11*/
	DUMMY2,              /* 12*/
	IRAM_SHARED_EXT,     /* 13*/
	IRAM_SHARED_USBD_B,  /* 14*/
	IRAM_SHARED_MODULE_MAX
}I_RAM_SHARED_MODULES;


/**
 * @brief   initialize iram controller registers and set default iram overall size
 * @param   none
 * @return 	none
 */
extern void drv_l1_iram_ctrl_init(void);

/**
 * @brief   Set Internal Ram Overall Size
 * @param   iram_num: 1: iram1; 2: iram2
 * @param   size: overall size in KBytes
 * @return 	0: STATUS_OK
 *         -1: STATUS_FAIL
 */
extern INT32S drv_l1_iram_set_overall_size(INT32U iram_num, INT32U size);

/**
 * @brief   Get Internal Ram Overall Size
 * @param   iram_num: 1: iram1; 2: iram2
 * @return 	Current internal ram overall size
 *          -1: STATUS_FAIL
 */
extern INT32S drv_l1_iram_get_overall_size(INT32U iram_num);

/**
 * @brief   Clear Internal Ram Setting(except the overall size)
 * @param   iram_num: 1: iram 1, 2: iram 2
 * @return 	0: STATUS_OK;
 *         -1: STATUS_FAIL;
 */
extern INT32S drv_l1_iram_clr_set(INT32U iram_num);

/**
 * @brief   Set the starting offset of the specific module as shared internal ram from 0xF800_0000/0xFA00_0000 in kBytes.
 * @param   iram_num: 1: iram1; 2: iram2
 *          module: module to be shared as part of internal ram.
 *          offset_kB: Starting offset of specific module as shared internal ram. This value is an offset from 0xF800_0000/0xFA00_0000 in kByte
 *                     ex: if the actual address is 0xF801_0000, this value should be 64.
 * @return 	0: STATUS_OK;
 *         -1: STATUS_FAIL;
 */
extern INT32S drv_l1_iram_set_start_offset(INT32U iram_num, I_RAM_SHARED_MODULES module, INT32U offset_kB);

/**
 * @brief   Get starting offset of the specific module as shared internal ram from 0xF800_0000 in kBytes.
 * @param   iram_num: 1: iram1; 2: iram2
 * @param   module: module to be shared as part of internal ram.
 * @return 	-1: STATUS_FAIL.
 *          others: Starting offset of the specific module as shared internal ram from 0xF800_0000 in kBytes.
 */
extern INT32S drv_l1_iram_get_start_offset(INT32U iram_num, I_RAM_SHARED_MODULES module);


/**
 * @brief   Iram shared enable, the selected internal ram cannot be shared to another internal ram.
 * @param   iram_num: 1: iram1; 2: iram2
 * @return 	0: STATUS_OK, -1: STATUS_FAIL
 */
INT32S drv_l1_iram_shared_enable(INT32U iram_num);

/**
 * @brief   Enable specific shared ram as a part of internal ram
 * @param   iram_num: 1: iram1; 2: iram2
 * @param   module: specific shared ram module
 * @return 	0: STATUS_OK, -1: STATUS_FAIL
 */
extern INT32S drv_l1_iram_shared_module_en(INT32U iram_num, I_RAM_SHARED_MODULES module);

#endif

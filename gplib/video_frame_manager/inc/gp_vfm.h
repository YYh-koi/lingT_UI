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

#ifndef __GP_VFM_H__
#define __GP_VFM_H__

#include "cmsis_os.h"

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct mem_blk_s
{
	INT32U tag;
	INT32U size;
	INT32U allocate;
	//INT32U pad[13]; //padding to 64 bytes
} mem_blk_t;

typedef struct vframe_manager_s
{
	INT32U start;
	INT32U end;
	INT32U current;
	INT32U blk_size;
	INT32U total_size;
	INT32U allocated_size;
	INT32U allocated_block;
	INT32U ext_addr;
	osMutexId mutex;
} vframe_manager_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/**
 * @brief   intial video frame manager (vfm)
 * @param   vfm:			pointer to vfm data structure
 * @param	ext_addr:		external buffer address, set 0 to use internal buffer allocation.
 * @param	size:			total buffer size for vfm heap
 * @param	max_blk_size:	default block size for each video frame allocation
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S vfm_init(vframe_manager_t* vfm, INT32U ext_addr, INT32U size, INT32U max_blk_size);

/**
 * @brief   reset vfm buffer status
 * @param   vfm:			pointer to vfm data structure
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S vfm_reset(vframe_manager_t* vfm);

/**
 * @brief   close vfm
 * @param   vfm:			pointer to vfm data structure
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S vfm_close(vframe_manager_t* vfm);

/**
 * @brief   check vfm buffer status
 * @param   vfm:			pointer to vfm data structure
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S check_vfm(vframe_manager_t* vfm);

/**
 * @brief   get one free block
 * @param   vfm:			pointer to vfm data structure
 * @param   blk_size:		size of block, set 0 to use default value set when initial vfm
 * @return 	SUCCESS:block address	FAIL:0
 */
INT32U vfm_get_empty(vframe_manager_t* vfm, INT32U blk_size);

/**
 * @brief   report usage of a block, and return unused portion to vfm
 * @param   vfm:			pointer to vfm data structure
 * @param   frame_addr:		frame address previously get from vfm
 * @param   size:			frame size
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S vfm_report_size(vframe_manager_t* vfm, INT32U frame_addr, INT32U size);

/**
 * @brief   return unused frame buffer to vfm
 * @param   vfm:			pointer to vfm data structure
 * @param   frame_addr:		frame address previously get from vfm
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S vfm_post_empty(vframe_manager_t* vfm, INT32U frame_addr);
#endif

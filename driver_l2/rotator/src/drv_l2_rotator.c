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
#define SYSTEM_OS_ENABLE                1

typedef struct Drv_l2_RotatorDev_s
{
	INT32U rotator_init_flag;
#if SYSTEM_OS_ENABLE == 1
	osSemaphoreId rotator_hw_sem;
#endif
} Drv_l2_RotatorDev_t;

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT32U rotator_dev_init_flag[ROTATOR_MAX] = {0};
static Drv_l2_RotatorDev_t Drv_l2_rotator_dev[ROTATOR_MAX];

static Drv_l2_RotatorDev_t *_get_drv_l2_rotator_device(INT8U rotator_num)
{
#if ROTATOR0_EN == 1
	if (rotator_num == ROTATOR_0) {
		return &Drv_l2_rotator_dev[ROTATOR_0];
	}
#endif

#if ROTATOR1_EN == 1
	if (rotator_num == ROTATOR_1) {
		return &Drv_l2_rotator_dev[ROTATOR_1];
	}
#endif

	return 0;
}

/**
 * @brief   rotator osSemaphore lock
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @return 	none
 */
static void drv_l2_rotator_lock(INT8U rotator_num)
{
    Drv_l2_RotatorDev_t *pRotatorDev = _get_drv_l2_rotator_device(rotator_num);

#if SYSTEM_OS_ENABLE == 1
    if(pRotatorDev->rotator_hw_sem)
        osSemaphoreWait(pRotatorDev->rotator_hw_sem, osWaitForever);
#endif
}

/**
 * @brief   rotator osSemaphore unlock
 * @param   rotator_num[in]: ROTATOR_0/ROTATOR_1
 * @return 	none
 */
static void drv_l2_rotator_unlock(INT8U rotator_num)
{
    Drv_l2_RotatorDev_t *pRotatorDev = _get_drv_l2_rotator_device(rotator_num);

#if SYSTEM_OS_ENABLE == 1
	if(pRotatorDev->rotator_hw_sem)
        osSemaphoreRelease(pRotatorDev->rotator_hw_sem);
#endif
}

/**
 * @brief   rotator initiation
 * @param   rotator_dev[in]: rotator device
 * @return 	none
 */
void drv_l2_rotator_init(INT8U rotator_dev)
{
    Drv_l2_RotatorDev_t *pRotatorDev = _get_drv_l2_rotator_device(rotator_dev);

    drv_l1_rotator_init(rotator_dev);

    if(rotator_dev_init_flag[rotator_dev] == 0)
    {
        gp_memset((INT8S *)pRotatorDev, 0, sizeof(Drv_l2_RotatorDev_t));
        rotator_dev_init_flag[rotator_dev] = 1;
    }

    if(pRotatorDev->rotator_init_flag == 0)
    {
        pRotatorDev->rotator_init_flag = 1;

        if(pRotatorDev->rotator_hw_sem == NULL) {
            osSemaphoreDef_t rotator_sem = { 0 };

            pRotatorDev->rotator_hw_sem = osSemaphoreCreate(&rotator_sem, 1);
            if(pRotatorDev->rotator_hw_sem == NULL)
                pRotatorDev->rotator_init_flag = 0;
        }
    }
}

/**
 * @brief   rotator once
 * @param   rotator_dev[in]: rotator device
 *          wait_done[in]: wait rotator end, 0:disable/1:enable
 *          rot_info[in]: rotator image parameter
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
static INT32S rotator_once(INT8U rotator_dev, INT8U wait_done, RotatorFormat_t *rot_info)
{
    INT32U ret;

    if((rot_info->rotator_src_addr == 0) || (rot_info->rotator_tar_addr == 0))
        return C_ROTATOR_BUFFER_ERR;

    ret = drv_l1_rotator_src_img_info(rotator_dev, rot_info->rotator_color_format,rot_info->rotator_width,
    rot_info->rotator_height, rot_info->rotator_src_addr);
    if(ret < 0)
        return C_ROTATOR_SIZE_SUPPORT_ERR;


    ret = drv_l1_rotator_mode_set(rotator_dev, rot_info->rotator_mode_selection);
    if(ret < 0)
        return C_ROTATOR_MODE_SUPPORT_ERR;

    drv_l1_rotator_tar_img_addr(rotator_dev, rot_info->rotator_tar_addr);
    drv_l1_rotator_isr_callback_set(rotator_dev, rot_info->isr_callback);
    // rotator start
    drv_l1_rotator_start(rotator_dev);

    ret = 0;
    if(wait_done)
        drv_l1_rotator_end_wait(rotator_dev, 1);

    return ret;
}

INT32S drv_l2_rotator_once(INT8U rotator_dev, INT8U wait_done, RotatorFormat_t *rot_info)
{
    INT32U ret;

    drv_l2_rotator_lock(rotator_dev);
    ret = rotator_once(rotator_dev, wait_done, rot_info);
    drv_l2_rotator_unlock(rotator_dev);

    return ret;
}

/**
 * @brief   rotator angle for fd
 * @param   rotator_dev[in]: rotator device
 *          wait_done[in]: wait rotator end, 0:disable/1:enable
 *          rot_info[in]: rotator image parameter
 *          angle[in]: fd angle: 35/45
 *          clockwise[in]: 0:clockwise/1:counter_clockwise
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
static INT32S rotator_fd_angle(INT8U rotator_dev, INT8U wait_done, RotatorFormat_t *rot_info, INT16S angle, INT32U clockwise)
{
    INT32S ret;

    if(rotator_dev == ROTATOR_0)
        return C_ROTATOR_DEVICE_SUPPORT_ERR;

    if((rot_info->rotator_src_addr == 0) || (rot_info->rotator_tar_addr == 0))
        return C_ROTATOR_BUFFER_ERR;

    if((rot_info->rotator_color_format != THETA_IMAGE_GP420) && (rot_info->rotator_color_format != THETA_IMAGE_Y_ONLY))
        return C_ROTATOR_COLOR_SUPPORT_ERR;

    if(angle > 359 || angle < 0)
        return C_ROTATOR_MODE_SUPPORT_ERR;

    ret = drv_l1_rotator_src_img_info(rotator_dev, IMAGE_GP420,rot_info->rotator_width, rot_info->rotator_height, rot_info->rotator_src_addr);
    if(ret < 0)
        return C_ROTATOR_SIZE_SUPPORT_ERR;

    ret = drv_l1_rotator_mode_set(rotator_dev, rot_info->rotator_mode_selection);
    if(ret < 0)
        return C_ROTATOR_MODE_SUPPORT_ERR;

    drv_l1_rotator_theta_info_set(rotator_dev, rot_info->rotator_theta_burst, rot_info->rotator_color_format, clockwise);
    drv_l1_rotator_non_right_angle_line_buffer_number_set(rotator_dev, 1);
    drv_l1_rotator_auto_high_priority_set(rotator_dev, 0);
    drv_l1_rotator_angle_set(rotator_dev, angle);
    drv_l1_rotator_tar_img_addr(rotator_dev, rot_info->rotator_tar_addr);
    drv_l1_rotator_isr_callback_set(rotator_dev, rot_info->isr_callback);
    // rotator start
    drv_l1_rotator_start(rotator_dev);

    ret = 0;
    if(wait_done)
        ret = drv_l1_rotator_end_wait(rotator_dev, 1);

    return ret;
}

INT32S drv_l2_rotator_fd_angle(INT8U rotator_dev, INT8U wait_done, RotatorFormat_t *rot_info, INT16S angle, INT32U clockwise)
{
    INT32U ret;

    drv_l2_rotator_lock(rotator_dev);
    ret = rotator_fd_angle(rotator_dev, wait_done, rot_info, angle, clockwise);
    drv_l2_rotator_unlock(rotator_dev);

    return ret;
}

/**
 * @brief   rotator uninitiation
 * @param   rotator_dev[in]: rotator device
 * @return 	none
 */
void drv_l2_rotator_uninit(INT8U rotator_dev)
{
    drv_l2_rotator_lock(rotator_dev);
    drv_l1_rotator_uninit(rotator_dev);
    drv_l2_rotator_unlock(rotator_dev);
}

/**
 * @brief   wait rotator end
 * @param   rotator_dev[in]: rotator device
 * @return 	0:STATUS_OK/-1:STATUS_FAIL
 */
INT32S drv_l2_rotator_wait_done(INT8U rotator_dev)
{
    INT32S ret;

    drv_l2_rotator_lock(rotator_dev);
    ret = drv_l1_rotator_end_wait(rotator_dev, 1);
    drv_l2_rotator_unlock(rotator_dev);

    if(ret & C_ROTATOR_END_STATE)
        ret = 0;
    else
        ret = -1;

    return ret;
}

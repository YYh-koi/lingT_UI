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
extern INT8U sdc_voltage_get(INT32U device_id);
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
/**
 * @brief   More resources init and config
 * @param   none
 * @return 	none
 */
void drv_l2_init(void)
{
	drv_l2_sys_timer_init();

#if (defined SD_EN) && (SD_EN == 1)
    drvl2_sd_voltage_get_callback_set(drvl2_sd_drive_to_device_id(FS_SD0), sdc_voltage_get);
    drvl2_sd_voltage_get_callback_set(drvl2_sd_drive_to_device_id(FS_SD1), sdc_voltage_get);
	drvl2_sd_set_bit_mode(drvl2_sd_drive_to_device_id(FS_SD1), 4); // config sd1 to use 4 bit, 1 to use 1 bit
	drvl2_sd_set_bit_mode(drvl2_sd_drive_to_device_id(FS_SD0), 4); // config sd0 to use 4 bit, 1 to use 1 bit
#endif

#if C_GSENSOR_FUNCTION
      drv_l2_gsensor_init(G_SENSOR_DEVICE, G_SENSOR_ACTIVE, G_SENSOR_I2C_SCL, G_SENSOR_I2C_SDA, 3);
#endif
}

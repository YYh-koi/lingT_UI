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

#if (defined _DRV_L2_ACTUATOR) && (_DRV_L2_ACTUATOR == 1)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
static drv_l2_actuator_ops_t* actuator_obj;

drv_l2_actuator_ops_t* drv_l2_actuator_get_ops(void)
{
	return actuator_obj;
}

void drv_l2_actuator_register_ops(drv_l2_actuator_ops_t* actuator)
{
	actuator_obj = actuator;
}

void drv_l2_actuator_init(void)
{
	/* reset actuator pointer to NULL */
	actuator_obj = NULL;

	DBG_PRINT("%s\r\n", __FUNCTION__);
	/* register actuator ops for specified actuator */

#if (defined _DW9714P) && (_DW9714P == 1)
	drv_l2_actuator_register_ops((drv_l2_actuator_ops_t*)&dw9714p_act_ops);
#endif
#if (defined _AD5823) && (_AD5823 == 1)
	drv_l2_actuator_register_ops((drv_l2_actuator_ops_t*)&ad5823_act_ops);
#endif
#if (defined _DW9800) && (_DW9800 == 1)
	drv_l2_actuator_register_ops((drv_l2_actuator_ops_t*)&dw9800_act_ops);
#endif

}


void drv_l2_actuator_set_obj(void)
{
	drv_l2_isp_set_actuator(actuator_obj);
}

void drv_l2_user_actuator_init(void)
{
	drv_l2_actuator_init();

#if (defined _DW9714P) && (_DW9714P == 1)
	if(dw9714p_act_ops.init() == -1)
		DBG_PRINT("dw9714p init Failed!!\r\n");
	else
		DBG_PRINT("dw9714p init done\r\n");
#endif

#if (defined _AD5823) && (_AD5823 == 1)
	if(ad5823_act_ops.init() == -1)
		DBG_PRINT("ad5823 init Failed!!\r\n");
	else
		DBG_PRINT("ad5823 init done\r\n");
#endif
#if (defined _DW9800) && (_DW9800 == 1)
	if(dw9800_act_ops.init() == -1)
		DBG_PRINT("dw9800 init Failed!!\r\n");
	else
		DBG_PRINT("dw9800 init done\r\n");
#endif

	drv_l2_actuator_set_obj();
}


#endif





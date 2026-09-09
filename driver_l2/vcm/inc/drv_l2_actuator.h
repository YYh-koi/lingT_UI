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
#ifndef DRV_L2_ACTUATOR_H
#define DRV_L2_ACTUATOR_H

#define ACTUATOR_I2C_MODE_HW		0
#define ACTUATOR_I2C_MODE_SW		1

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define _DRV_L2_ACTUATOR        1

#define _DW9714P   0
#define _AD5823    0
#define _DW9800    1
#define DW9714P_ACT_NAME    "dw9714p_actuator"
#define AD5823_ACT_NAME     "ad5823_actuator"
#define DW9800_ACT_NAME    "dw9800_actuator"
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct drv_l2_actuator_ops_s
{
	char*	name;
	INT32S	(*init)(void);
	void	(*uninit)(void);
	void	(*set_motor)(INT32S step);
} drv_l2_actuator_ops_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
void drv_l2_actuator_set_obj(void);
extern void drv_l2_user_actuator_init(void);


#if (defined _DW9714P) && (_DW9714P == 1)
extern const drv_l2_actuator_ops_t dw9714p_act_ops;
#endif

#if (defined _AD5823) && (_AD5823 == 1)
extern const drv_l2_actuator_ops_t ad5823_act_ops;
#endif
#if (defined _DW9800) && (_DW9800 == 1)
extern const drv_l2_actuator_ops_t dw9800_act_ops;
#endif

#endif


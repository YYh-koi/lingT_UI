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

/********************************************************************
* Purpose:  GPIO driver/interface
* Author:
* Date:		2014/07/15
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
* Version : 1.00
* History :
*********************************************************************/

/******************************************************************************
 * Paresent Header Include
 ******************************************************************************/
#include "drv_l1.h"

/******************************************************************************
 * Static Variables
 ******************************************************************************/


/******************************************************************************
 * Function Prototypes
 ******************************************************************************/
void gpio_init(void);
BOOLEAN gpio_init_io(INT32U port, BOOLEAN direction);
BOOLEAN gpio_read_io(INT32U port);
BOOLEAN gpio_write_io(INT32U port, BOOLEAN data);
BOOLEAN gpio_set_port_attribute(INT32U port, BOOLEAN attribute);
BOOLEAN gpio_write_all_by_set(INT32U gpio_set_num, INT32U write_data);
BOOLEAN gpio_get_dir(INT32U port);
INT32U gpio_read_all_by_set(INT8U gpio_set_num);
BOOLEAN gpio_drving_init_io(GPIO_ENUM port, IO_DRV_LEVEL gpio_driving_level);
void gpio_drving_uninit(void);
void gpio_basic_init(void);

/******************************************************************************
 * Function Body
 ******************************************************************************/

 /**
 * @brief   Initial gpio
 * @param   None
 * @return 	None
 */
void gpio_init(void)
{
	/* initial all gpio */
	gpio_basic_init();
}

 /**
 * @brief   gpio basic init
 * @param   None
 * @return 	None
 */
void gpio_basic_init(void)
{
    //all special function is configured in board_config.c
}

/* This interface is for the application layer to initail the GPIO direction*/
/* init_io will not only modyfy the direction but also control the attribute value */
 /**
 * @brief   General IO setting
 * @param   port: IO pin number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
 * @param   direction: Value filled in direction and attribute bit, 1: set as 1, 0: set as 1
 * @return 	1: Success
			0: Failed. The assigned port is not existed.
 */
BOOLEAN gpio_init_io(INT32U port, BOOLEAN direction)
{
    volatile INT16U gpio_set;
    volatile INT16U gpio_set_num;
    //INT32U trace;

    gpio_set = port / EACH_REGISTER_GPIO_NUMS;
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS;
    direction &= LOWEST_BIT_MASK;
    if (direction == GPIO_OUTPUT) {
        DRV_Reg32(IOA_ATTRIB_ADDR+(EACH_DIR_REG_OFFSET*gpio_set)) |= (1 << gpio_set_num);  /*Set attribute to 0 for input */
        DRV_Reg32((IOA_DIR_ADDR+EACH_DIR_REG_OFFSET*gpio_set)) |= (1 << gpio_set_num);
    }
    else if (direction == GPIO_INPUT) {
        DRV_Reg32(IOA_ATTRIB_ADDR+(EACH_DIR_REG_OFFSET*gpio_set)) &= ~(1 << gpio_set_num);  /*Set attribute to 1 for output */
        DRV_Reg32((IOA_DIR_ADDR+EACH_DIR_REG_OFFSET*gpio_set)) &= ~(1 << gpio_set_num);
    }
    else { return GPIO_FAIL; }

    return GPIO_OK;
}

 /**
 * @brief   Read IO port data
 * @param   port: IO pin number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
 * @return 	status of the IO port
 */
BOOLEAN gpio_read_io(INT32U port)
{
    volatile INT16U gpio_set;
    volatile INT16U gpio_set_num;

    gpio_set = port / EACH_REGISTER_GPIO_NUMS;
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS;

    return ((DRV_Reg32(IOA_DATA_ADDR+EACH_GPIO_DATA_REG_OFFSET*gpio_set) >> gpio_set_num) & LOWEST_BIT_MASK);

}

 /**
 * @brief   Set IO port output data
 * @param   port: IO pin number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
 * @param   data: 1: output high, 0: output low
 * @return 	1: Success
			0: Failed
 */
BOOLEAN gpio_write_io(INT32U port, BOOLEAN data)
{
    volatile INT16U gpio_set;
    volatile INT16U gpio_set_num;
    //INT32U trace;
    gpio_set = port / EACH_REGISTER_GPIO_NUMS;  // gpio_set_array
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS; // gpio_setbit_array

    data &= LOWEST_BIT_MASK;
    if (data == DATA_HIGH){
        DRV_Reg32((IOA_BUFFER_ADDR+EACH_GPIO_DATA_REG_OFFSET*gpio_set)) |= (1 << gpio_set_num);
    }
    else if (data == DATA_LOW){
        DRV_Reg32((IOA_BUFFER_ADDR+EACH_GPIO_DATA_REG_OFFSET*gpio_set)) &= ~(1 << gpio_set_num);
    }
    else  {return GPIO_FAIL; }

    return GPIO_OK;

}

 /**
 * @brief   Set GPIO driving capability
 * @param   port: IO pin number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
 * @param   gpio_driving_level: IO drivering capability
 * @return 	1: Success
			0: Failed
 */
BOOLEAN gpio_drving_init_io(GPIO_ENUM port, IO_DRV_LEVEL gpio_driving_level)
{

    INT16U gpio_set;
    INT16U gpio_set_num;
    INT32U drv_level;

    gpio_set = port / EACH_REGISTER_GPIO_NUMS;
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS;
    drv_level = (INT32U) gpio_driving_level;


    if (port < IO_F9)  // IOA/B/C/D/E and F(9-bit)
    {
        if (drv_level == 0)
        {
            DRV_Reg32(IOA_DRV+(EACH_DIR_REG_OFFSET*gpio_set)) &= ~(1 << gpio_set_num);
        }
        else
        {
            DRV_Reg32(IOA_DRV+(EACH_DIR_REG_OFFSET*gpio_set)) |= (1 << gpio_set_num);
        }
    }
    else
    {
          return GPIO_FAIL;
    }

    return GPIO_OK;
}

 /**
 * @brief   Uninitial gpio
 * @param   None
 * @return 	None
 */
void gpio_drving_uninit(void)
{
    R_IOA_DRV = 0;
    R_IOB_DRV = 0;
    R_IOC_DRV = 0;
    R_IOD_DRV = 0;
    R_IOE_DRV = 0;
    R_IOF_DRV = 0;
}

 /**
 * @brief   Set IO port attribute
 * @param   port: IO pin number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
 * @param   attribute: 1: set as 1, 0: set as 0
 * @return 	1: Success
			0: Failed
 */
BOOLEAN gpio_set_port_attribute(INT32U port, BOOLEAN attribute)
{
    volatile INT16U gpio_set;
    volatile INT16U gpio_set_num;

    gpio_set = port / EACH_REGISTER_GPIO_NUMS;
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS;
    attribute &= LOWEST_BIT_MASK;
    if (attribute == ATTRIBUTE_HIGH) {
        DRV_Reg32((IOA_ATTRIB_ADDR+EACH_ATTRIB_REG_OFFSET*gpio_set)) |= (1 << gpio_set_num);
    }
    else if (attribute == ATTRIBUTE_LOW) {
        DRV_Reg32((IOA_ATTRIB_ADDR+EACH_ATTRIB_REG_OFFSET*gpio_set)) &= ~(1 << gpio_set_num);
    }
    else { return GPIO_FAIL; }

    return GPIO_OK;
}

/*INPUT :
         gpio_set_num: GPIO_SET_A, GPIO_SET_B, GPIO_SET_C, GPIO_SET_D
         write_data: 0x00001234h
*/
 /**
 * @brief   Set IO port output value
 * @param   gpio_set_num: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
 * @param   write_data: 1: output high, 0: output low
 * @return 	1: Success
			0: Failed
 */
BOOLEAN gpio_write_all_by_set(INT32U gpio_set_num, INT32U write_data)
{
    if (gpio_set_num < GPIO_SET_MAX) {
        DRV_WriteReg32(IOA_BUFFER_ADDR+EACH_GPIO_DATA_REG_OFFSET*gpio_set_num,(write_data&LOWEST_BIT_MASK));
        return GPIO_OK;
    }
    else {
        return GPIO_FAIL;
    }
}

 /**
 * @brief   Read IO port data
 * @param   gpio_set_num: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
 * @return 	Not 0xFFFFFFFF: Input status of this IO
			0xFFFFFFFF: gpio_set_num is out of range
 */
INT32U gpio_read_all_by_set(INT8U gpio_set_num)
{
    if (gpio_set_num < GPIO_SET_MAX) {
        return DRV_Reg32(IOA_DATA_ADDR+EACH_GPIO_DATA_REG_OFFSET*gpio_set_num);
    }
        return 0xFFFFFFFF;

}

 /**
 * @brief   Get IO port direction
 * @param   port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
 * @return 	Direction setting of the specified IO
 */
BOOLEAN gpio_get_dir(INT32U port)
{
    INT16U gpio_set;
    INT16U gpio_set_num;

    gpio_set = port / EACH_REGISTER_GPIO_NUMS;
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS;

    return ((DRV_Reg32(IOA_DIR_ADDR+EACH_DIR_REG_OFFSET*gpio_set) >> gpio_set_num) & LOWEST_BIT_MASK);
}

 /**
 * @brief   Set IOH[5:2] as JTAG pins or GPIOs
 * @param   status: 1: BKUEB, 0: GPIOs
 * @return 	None
 */
void gpio_set_ice_en(BOOLEAN status)
{
	if (status == TRUE) {
	  R_GPIO_CTRL |= 1; /* enable */
	}
	else  {
	  R_GPIO_CTRL &= ~1; /* disable */
	}
}

BOOLEAN gpio_get_attrib(INT32U port)
{
    INT16U gpio_set;
    INT16U gpio_set_num;

    gpio_set = port / EACH_REGISTER_GPIO_NUMS;
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS;

    return ((DRV_Reg32(IOA_ATTRIB_ADDR+EACH_ATTRIB_REG_OFFSET*gpio_set) >> gpio_set_num) & LOWEST_BIT_MASK);
}

 /**
 * @brief   General purpose IO config first
 * @param   port: IO pin number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
 * @param   enable: 1 to force port as GPIO; 0 to disable GPIO function and return what's configured as other special funciton
 * @return 	1: Success
			0: Failed. The assigned port is not existed.
 */
BOOLEAN gpio_io_top_prio(INT32U port, BOOLEAN enable)
{
    volatile INT16U gpio_set;
    volatile INT16U gpio_set_num;
    //INT32U trace;

    gpio_set = port / EACH_REGISTER_GPIO_NUMS;
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS;
    enable &= LOWEST_BIT_MASK;
    if (enable == ENABLE) {
        DRV_Reg32(IOA_TOP_PRIO_ADDR+(4*gpio_set)) |= (1 << gpio_set_num);  /*Set attribute to 0 for input */
    }
    else if (enable == DISABLE) {
        DRV_Reg32(IOA_TOP_PRIO_ADDR+(4*gpio_set)) &= ~(1 << gpio_set_num);  /*Set attribute to 1 for output */
    }
    else { return GPIO_FAIL; }

    return GPIO_OK;
}

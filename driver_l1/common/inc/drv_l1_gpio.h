#ifndef __drv_l1_GPIO_H__
#define __drv_l1_GPIO_H__

/*GPIO Register define*/
/* GPIO: 0xC0000000 */
#define GPIO_BASE_ADDR             	0xC0000000
#define EACH_GPIO_DATA_REG_OFFSET   0x00000020
#define EACH_DIR_REG_OFFSET        	0x00000020
#define EACH_ATTRIB_REG_OFFSET     	0x00000020

#define IOA_DATA_ADDR              		(GPIO_BASE_ADDR+0x00)   /*0xC0000000*/
#define IOA_BUFFER_ADDR            		(GPIO_BASE_ADDR+0x04)   /*0xC0000004*/
#define IOA_DIR_ADDR               		(GPIO_BASE_ADDR+0x08)   /*0xC0000008*/
#define IOA_ATTRIB_ADDR            		(GPIO_BASE_ADDR+0x0C)   /*0xC000000C*/
#define IOA_DRV	                        (GPIO_BASE_ADDR+0x10)   /*0xC0000010*/

#define IOB_DATA_ADDR              		(GPIO_BASE_ADDR+0x20)   /*0xC0000020*/
#define IOB_BUFFER_ADDR            		(GPIO_BASE_ADDR+0x24)   /*0xC0000024*/
#define IOB_DIR_ADDR               		(GPIO_BASE_ADDR+0x28)   /*0xC0000028*/
#define IOB_ATTRIB_ADDR            		(GPIO_BASE_ADDR+0x2C)   /*0xC000002C*/
#define IOB_DRV	                        (GPIO_BASE_ADDR+0x30)   /*0xC0000030*/

#define IOC_DATA_ADDR              		(GPIO_BASE_ADDR+0x40)   /*0xC0000040*/
#define IOC_BUFFER_ADDR            		(GPIO_BASE_ADDR+0x44)   /*0xC0000044*/
#define IOC_DIR_ADDR              	 	(GPIO_BASE_ADDR+0x48)   /*0xC0000048*/
#define IOC_ATTRIB_ADDR            		(GPIO_BASE_ADDR+0x4C)   /*0xC000004C*/
#define IOC_DRV	                        (GPIO_BASE_ADDR+0x50)   /*0xC0000050*/

#define IOD_DATA_ADDR              		(GPIO_BASE_ADDR+0x60)   /*0xC0000060*/
#define IOD_BUFFER_ADDR            		(GPIO_BASE_ADDR+0x64)   /*0xC0000064*/
#define IOD_DIR_ADDR               		(GPIO_BASE_ADDR+0x68)   /*0xC0000068*/
#define IOD_ATTRIB_ADDR            		(GPIO_BASE_ADDR+0x6C)   /*0xC000006C*/
#define IOD_DRV	                        (GPIO_BASE_ADDR+0x70)   /*0xC0000070*/

#define IOE_DATA_ADDR              		(GPIO_BASE_ADDR+0x80)   /*0xC0000080*/
#define IOE_BUFFER_ADDR            		(GPIO_BASE_ADDR+0x84)   /*0xC0000084*/
#define IOE_DIR_ADDR               		(GPIO_BASE_ADDR+0x88)   /*0xC0000088*/
#define IOE_ATTRIB_ADDR            		(GPIO_BASE_ADDR+0x8C)   /*0xC000008C*/
#define IOE_DRV	                        (GPIO_BASE_ADDR+0x90)   /*0xC0000090*/

#define IOF_DATA_ADDR              		(GPIO_BASE_ADDR+0xA0)   /*0xC00000A0*/
#define IOF_BUFFER_ADDR            		(GPIO_BASE_ADDR+0xA4)   /*0xC00000A4*/
#define IOF_DIR_ADDR               		(GPIO_BASE_ADDR+0xA8)   /*0xC00000A8*/
#define IOF_ATTRIB_ADDR            		(GPIO_BASE_ADDR+0xAC)   /*0xC00000AC*/
#define IOF_DRV	                        (GPIO_BASE_ADDR+0xB0)   /*0xC00000B0*/

#define IOA_TOP_PRIO_ADDR            	(GPIO_BASE_ADDR+0x180)   /*0xC0000180*/


/* Attribution Register High/Low definition */
#define INPUT_NO_RESISTOR       1
#define OUTPUT_UNINVERT_CONTENT 1
#define INPUT_WITH_RESISTOR     0
#define OUTPUT_INVERT_CONTENT   0


#define GPIO_FAIL               0
#define GPIO_OK                 1

#define EACH_REGISTER_GPIO_NUMS 16

#define LOWEST_BIT_MASK         0x00000001

/* Direction Register Input/Output definition */
#define GPIO_INPUT              0       /* IO in input */
#define GPIO_OUTPUT             1       /* IO in output */

/* Attribution Register High/Low definition */
#define ATTRIBUTE_HIGH          1
#define INPUT_NO_RESISTOR       1
#define OUTPUT_UNINVERT_CONTENT 1
#define ATTRIBUTE_LOW           0
#define INPUT_WITH_RESISTOR     0
#define OUTPUT_INVERT_CONTENT   0
#define DATA_HIGH               1
#define DATA_LOW                0

/* Direction Register Input/Output definition */
#define HAL_GPIO_INPUT          0   /* IO in input */
#define HAL_GPIO_OUTPUT         1   /* IO in output */



#define DRV_WriteReg32(addr,data)     ((*(volatile INT32U *)(addr)) = (INT32U)data)
#define DRV_Reg32(addr)               (*(volatile INT32U *)(addr))



/* Bearer type enum */
typedef enum
{
	GPIO_SET_A=0,
	GPIO_SET_B,
	GPIO_SET_C,
	GPIO_SET_D,
	GPIO_SET_E,
	GPIO_SET_MAX
} GPIO_SET_ENUM;


#ifndef __GPIO_TYPEDEF__
#define __GPIO_TYPEDEF__

typedef enum {
    IO_A0=0,
    IO_A1 ,
    IO_A2 ,
    IO_A3 ,
    IO_A4 ,
    IO_A5 ,
    IO_A6 ,
    IO_A7 ,
    IO_A8 ,
    IO_A9 ,
    IO_A10,
    IO_A11,
    IO_A12,
    IO_A13,
    IO_A14,
    IO_A15,
    IO_B0 ,
    IO_B1 ,
    IO_B2 ,
    IO_B3 ,
    IO_B4 ,
    IO_B5 ,
    IO_B6 ,
    IO_B7 ,
    IO_B8 ,
    IO_B9 ,
    IO_B10,
    IO_B11,
    IO_B12,
    IO_B13,
    IO_B14,
    IO_B15,
    IO_C0 ,
    IO_C1 ,
    IO_C2 ,
    IO_C3 ,
    IO_C4 ,
    IO_C5 ,
    IO_C6 ,
    IO_C7 ,
    IO_C8 ,
    IO_C9 ,
    IO_C10,
    IO_C11,
    IO_C12,
    IO_C13,
    IO_C14,
    IO_C15,
    IO_D0 ,
    IO_D1 ,
    IO_D2 ,
    IO_D3 ,
    IO_D4 ,
    IO_D5 ,
    IO_D6 ,
    IO_D7 ,
    IO_D8 ,
    IO_D9 ,
    IO_D10,
    IO_D11,
    IO_D12,
    IO_D13,
    IO_D14,
    IO_D15,
    IO_E0 ,
    IO_E1 ,
    IO_E2 ,
    IO_E3 ,
    IO_E4 ,
    IO_E5 ,
    IO_E6 ,
    IO_E7 ,
    IO_E8 ,
    IO_E9 ,
    IO_E10,
    IO_E11,
    IO_E12,
    IO_E13,
    IO_E14,
    IO_E15,
    IO_F0 ,
    IO_F1 ,
    IO_F2 ,
    IO_F3 ,
    IO_F4 ,
    IO_F5 ,
    IO_F6 ,
    IO_F7 ,
    IO_F8 ,
    IO_F9 ,
    IO_F10,
    IO_F11,
    IO_F12,
    IO_F13,
    IO_F14,
    IO_F15
} GPIO_ENUM;

#endif  //__GPIO_TYPEDEF__


#ifndef _GPIO_DRVING_DEF_
#define _GPIO_DRVING_DEF_

typedef enum {
    IOA_DRV_10mA=0x0,
    IOA_DRV_30mA=0x1,
/* IOB[0:9] Driving Options */
    IOB_DRV_10mA=0x0,
    IOB_DRV_30mA=0x1,
/* IOB[10:15] Driving Options */ //IOB[10:15] has higher driving capability
    IOB_DRV_20mA=0x0,
    IOB_DRV_40mA=0x1,
/* IOC Driving Options */
    IOC_DRV_10mA=0x0,
    IOC_DRV_30mA=0x1,
/* IOD Driving Options */
    IOD_DRV_10mA=0x0,
    IOD_DRV_30mA=0x1,
/* IOE Driving Options */
    IOE_DRV_10mA=0x0,
    IOE_DRV_30mA=0x1,
/* IOF Driving Options */
    IOF_DRV_10mA=0x0,
    IOF_DRV_30mA=0x1
} IO_DRV_LEVEL;


#endif //_GPIO_DRVING_DEF_


/*
Function Name	gpio_init
Description	Initialize GPIO
Header File	drv_l1_gpio.h
Syntax	void gpio_init(void)
Parameters	None
Return Values	None
*/
extern void gpio_init(void);
/*
Function Name	gpio_read_io
Description	Read IO port data
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_read_io(INT32U port)
Parameters	port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
Return Values	Input status of the whole IO port which contains this IO
*/
extern BOOLEAN gpio_read_io(INT32U port);
/*
Function Name	gpio_write_io
Description	Set IO port output data
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_write_io(INT32U port, BOOLEAN data)
Parameters	port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			data: 1: output high, 0: output low
			Output status of whole IO port which contains this IO
Return Values	1: Success
			0: Failed
*/
extern BOOLEAN gpio_write_io(INT32U port, BOOLEAN data);
/*
Function Name	gpio_init_io
Description	General IO setting
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_init_io(INT32U port, BOOLEAN direction)
Parameters	port: IO pin number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			direction: Value filled in direction and attribute bit, 1: set as 1, 0: set as 1
Return Values	1: Success
			0: Failed. The assigned port is not existed.
*/
extern BOOLEAN gpio_init_io(INT32U port, BOOLEAN direction);
/*
Function Name	gpio_set_port_attribute
Description	Set IO port attribute
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_set_port_attribute(INT32U port, BOOLEAN attribute)
Parameters	port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			attribute: 1: set as 1, 0: set as 0
Return Values	1: Success
			0: Failed
*/
extern BOOLEAN gpio_set_port_attribute(INT32U port, BOOLEAN attribute);
/*
Function Name	gpio_write_all_by_set
Description	Set IO port output value
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_write_all_by_set(INT32U gpio_set_num, INT32U write_data)
Parameters	gpio_set_num: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			write_data: 1: output high, 0: output low
			Output status of whole IO port which contains this IO
Return Values	1: Success
			0: Failed
*/
extern BOOLEAN gpio_write_all_by_set(INT32U gpio_set_num, INT32U write_data);
/*
Function Name	gpio_read_all_by_set
Description	Read IO port data
Header File	drv_l1_gpio.h
Syntax	INT32U gpio_read_all_by_set(INT8U gpio_set_num)
Parameters	gpio_set_num: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
Return Values	Not 0xFFFFFFFF: Input status of this IO
			0xFFFFFFFF: gpio_set_num is out of range
*/
extern INT32U gpio_read_all_by_set(INT8U gpio_set_num);
/*
Function Name	gpio_get_dir
Description	Get IO port direction
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_get_dir(INT32U port)
Parameters	port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
Return Values	Direction setting of the specified IO
*/
extern BOOLEAN gpio_get_dir(INT32U port);
/*
Function Name	gpio_drving_init_io
Description	Set GPIO driving capability
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_drving_init_io(GPIO_ENUM port, IO_DRV_LEVEL gpio_driving_level)
Parameters	port: IO pin number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			gpio_driving_level: IO drivering capability
Return Values	1: Success
			0: Failed
*/
extern BOOLEAN gpio_drving_init_io(GPIO_ENUM port, IO_DRV_LEVEL gpio_driving_level);

/*
Function Name	gpio_drving_uninit
Description	Set GPIO driving capability to the lowest level
Header File	drv_l1_gpio.h
Syntax	void gpio_drving_uninit(void)
Parameters	None
Return Values	None
*/
extern void gpio_drving_uninit(void);

/*
Function Name	gpio_set_ice_en
Description	Set IOH[5:2] as JTAG pins or GPIOs
Header File	drv_l1_gpio.h
Syntax	void gpio_set_ice_en(BOOLEAN status)
Parameters	status: 1: BKUEB, 0: GPIOs
Return Values	None
*/
extern void gpio_set_ice_en(BOOLEAN status);

/*
Function Name	gpio_io_top_prio
Description	Set gpio function as top priority to the selected port
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_io_top_prio(INT32U port, BOOLEAN enable);
Parameters	port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			enable: 1: set as 1, 0: set as 0
Return Values	1: Success
			0: Failed
*/
extern BOOLEAN gpio_io_top_prio(INT32U port, BOOLEAN enable);

#endif /* __drv_l1_GPIO_H__ */

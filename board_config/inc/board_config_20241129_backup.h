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
#ifndef __BOARD_CONFIG__
#define __BOARD_CONFIG__

#include "pinmux_config.h"

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/
/*== mux selection define ==*/
#define MUX_OK      0
#define MUX_ERR     -1

#define SWD_EN              0

#define	READ_FROM_PIN 		0
#define READ_FROM_GPIO		1

/*== GPIO PAD DRIVING ==*/
#define IO_DRIVING_lvl0				0
#define IO_DRIVING_lvl1				1
/************************************************************/
/*						Pinmux	setting						*/
/************************************************************/
#define UART_TX_ENABLE                          ENABLE
#define UART_TX_DISABLE                         DISABLE
#define UART_RX_ENABLE                          ENABLE
#define UART_RX_DISABLE                         DISABLE

/*== UART0 PINMUX ==*/
#define UART0_ENABLE				            0
#if UART0_ENABLE
#define UART0_POS					            UART0_MUX0
#define UART0_DRIVING				            IO_DRIVING_lvl0
#endif

/*== UART1 PINMUX ==*/
#define UART1_ENABLE				            1
#if UART1_ENABLE
#define UART1_POS					            UART1_MUX0
#define UART1_DRIVING				            IO_DRIVING_lvl0
#endif

/*== UART2 PINMUX ==*/
#define UART2_ENABLE				            0
#if UART2_ENABLE
#define UART2_POS					            UART2_MUX2
#define UART2_DRIVING				            IO_DRIVING_lvl0
#endif

/*== EXTA IRQ PINMUX ==*/
#define EXTA_IRQ_ENABLE				            0
#if EXTA_IRQ_ENABLE
#define EXTA_IRQ_POS							EXTA_IRQ_MUX3
#define EXTA_IRQ_DRIVING						IO_DRIVING_lvl0
#endif

/*== EXTB IRQ PINMUX ==*/
#define EXTB_IRQ_ENABLE				            0
#if EXTB_IRQ_ENABLE
#define EXTB_IRQ_POS							EXTB_IRQ_MUX3
#define EXTB_IRQ_DRIVING						IO_DRIVING_lvl0
#endif

/*== EXTC IRQ PINMUX ==*/
#define EXTC_IRQ_ENABLE				            0
#if EXTC_IRQ_ENABLE
#define EXTC_IRQ_POS							EXTC_IRQ_MUX3
#define EXTC_IRQ_DRIVING						IO_DRIVING_lvl0
#endif

/*== EXTD IRQ PINMUX ==*/
#define EXTD_IRQ_ENABLE				            0
#if EXTD_IRQ_ENABLE
#define EXTD_IRQ_POS							EXTD_IRQ_MUX3
#define EXTD_IRQ_DRIVING						IO_DRIVING_lvl0
#endif

/*== EXTE IRQ PINMUX ==*/
#define EXTE_IRQ_ENABLE				            0
#if EXTE_IRQ_ENABLE
#define EXTE_IRQ_POS							EXTE_IRQ_MUX3
#define EXTE_IRQ_DRIVING						IO_DRIVING_lvl0
#endif

/*== EXTF IRQ PINMUX ==*/
#define EXTF_IRQ_ENABLE				            0
#if EXTF_IRQ_ENABLE
#define EXTF_IRQ_POS							EXTF_IRQ_MUX3
#define EXTF_IRQ_DRIVING						IO_DRIVING_lvl0
#endif

/*== EXTG IRQ PINMUX ==*/
#define EXTG_IRQ_ENABLE				            0
#if EXTG_IRQ_ENABLE
#define EXTG_IRQ_POS							EXTG_IRQ_MUX3
#define EXTG_IRQ_DRIVING						IO_DRIVING_lvl0
#endif

/*== EXTH IRQ PINMUX ==*/
#define EXTH_IRQ_ENABLE				            0
#if EXTH_IRQ_ENABLE
#define EXTH_IRQ_POS							EXTH_IRQ_MUX3
#define EXTH_IRQ_DRIVING						IO_DRIVING_lvl0
#endif

/*== EXTI IRQ PINMUX ==*/
#define EXTI_IRQ_ENABLE				            0
#if EXTI_IRQ_ENABLE
#define EXTI_IRQ_POS							EXTI_IRQ_MUX3
#define EXTI_IRQ_DRIVING						IO_DRIVING_lvl0
#endif

/*== EXTJ IRQ PINMUX ==*/
#define EXTJ_IRQ_ENABLE				            0
#if EXTJ_IRQ_ENABLE
#define EXTJ_IRQ_POS							EXTJ_IRQ_MUX3
#define EXTJ_IRQ_DRIVING						IO_DRIVING_lvl0
#endif

/*== EXTK IRQ PINMUX ==*/
#define EXTK_IRQ_ENABLE				            0
#if EXTK_IRQ_ENABLE
#define EXTK_IRQ_POS							EXTK_IRQ_MUX3
#define EXTK_IRQ_DRIVING						IO_DRIVING_lvl0
#endif

/*== EXTL IRQ PINMUX ==*/
#define EXTL_IRQ_ENABLE				            0
#if EXTL_IRQ_ENABLE
#define EXTL_IRQ_POS							EXTL_IRQ_MUX3
#define EXTL_IRQ_DRIVING						IO_DRIVING_lvl0
#endif

/*== Timer A CCP PINMUX ==*/
#define TIMERA_CCP_ENABLE				         0
#if TIMERA_CCP_ENABLE
#define TIMERA_CCP_POS					    	TIMERA_CCP_MUX0
#define TIMERA_CCP_DRIVING			    		IO_DRIVING_lvl0
#endif

/*== Timer B CCP PINMUX ==*/
#define TIMERB_CCP_ENABLE				        0
#if TIMERB_CCP_ENABLE
#define TIMERB_CCP_POS					    	TIMERB_CCP_MUX0
#define TIMERB_CCP_DRIVING			    		IO_DRIVING_lvl0
#endif

/*== Timer C CCP PINMUX ==*/
#define TIMERC_CCP_ENABLE				        0
#if TIMERC_CCP_ENABLE
#define TIMERC_CCP_POS					    	TIMERC_CCP_MUX0
#define TIMERC_CCP_DRIVING			    		IO_DRIVING_lvl0
#endif

/*== Timer D CCP PINMUX ==*/
#define TIMERD_CCP_ENABLE				        0
#if TIMERD_CCP_ENABLE
#define TIMERD_CCP_POS					    	TIMERD_CCP_MUX0
#define TIMERD_CCP_DRIVING			    		IO_DRIVING_lvl0
#endif

/*== Timer E CCP PINMUX ==*/
#define TIMERE_CCP_ENABLE				        0
#if TIMERE_CCP_ENABLE
#define TIMERE_CCP_POS					    	TIMERE_CCP_MUX0
#define TIMERE_CCP_DRIVING			    		IO_DRIVING_lvl0
#endif

/*== Timer F CCP PINMUX ==*/
#define TIMERF_CCP_ENABLE				        0
#if TIMERF_CCP_ENABLE
#define TIMERF_CCP_POS					    	TIMERF_CCP_MUX0
#define TIMERF_CCP_DRIVING			    		IO_DRIVING_lvl0
#endif

/*== Timer G CCP PINMUX ==*/
#define TIMERG_CCP_ENABLE				        0
#if TIMERG_CCP_ENABLE
#define TIMERG_CCP_POS					    	TIMERG_CCP_MUX0
#define TIMERG_CCP_DRIVING			    		IO_DRIVING_lvl0
#endif

/*== Timer H CCP PINMUX ==*/
#define TIMERH_CCP_ENABLE				        0
#if TIMERH_CCP_ENABLE
#define TIMERH_CCP_POS					    	TIMERH_CCP_MUX0
#define TIMERH_CCP_DRIVING			    		IO_DRIVING_lvl0
#endif

/*== I2C0 PINMUX ==*/
#define I2C0_ENABLE				                1
#if I2C0_ENABLE
#define I2C0_POS							    I2C0_MUX0
#define I2C0_DRIVING						    IO_DRIVING_lvl0
#endif

/*== I2C1 PINMUX ==*/
#define I2C1_ENABLE				                0
#if I2C1_ENABLE
#define I2C1_POS							    I2C1_MUX0
#define I2C1_DRIVING						    IO_DRIVING_lvl0
#endif

/*== I2C2 PINMUX ==*/
#define I2C2_ENABLE				                0
#if I2C2_ENABLE
#define I2C2_POS							    I2C2_MUX0
#define I2C2_DRIVING						    IO_DRIVING_lvl0
#endif

/*== SPI0 PINMUX ==*/
#define CS_GPIO_ENABLE                          1
#define CS_GPIO_DISABLE                         0

#define SPI0_ENABLE				                0
#if SPI0_ENABLE
#define SPI0_POS								SPI0_MUX1
#define SPI0_CS_GPIO_MODE                       CS_GPIO_ENABLE
#define SPI0_DRIVING							IO_DRIVING_lvl0
#endif

/*== SPI1 PINMUX ==*/
#define SPI1_ENABLE				                0
#if SPI1_ENABLE
#define SPI1_POS								SPI1_MUX1
#define SPI1_CS_GPIO_MODE                       CS_GPIO_ENABLE
#define SPI1_DRIVING							IO_DRIVING_lvl0
#endif

/*== SPIFC PINMUX ==*/
#define SPIFC_ENABLE				                1
#if SPIFC_ENABLE
#define SPIFC_POS								SPIFC_MUX0
#define SPIFC_CLK_DRIVING					    IO_DRIVING_lvl0
#define SPIFC_DRIVING							IO_DRIVING_lvl0
#endif

/*== SDC0 PINMUX ==*/
#define SDC0_ENABLE				            1
#if SDC0_ENABLE
#define SDC0_POS								SDC0_MUX7//SDC0_MUX1
#define SDC0_CMD_CLK_DRIVING			        IO_DRIVING_lvl1
#endif

/*== SDC1 PINMUX ==*/
#define SDC1_ENABLE				                0
#if SDC1_ENABLE
#define SDC1_POS								SDC1_MUX0
#define SDC1_CMD_CLK_DRIVING			        IO_DRIVING_lvl1
#endif

/*== TFT PIN MUX ==*/
#define TFT1_ENABLE				                0
#if TFT1_ENABLE
#define TFT_DATA_0_7_POS					    TFT_DATA0_7_MUX0
#define TFT_DATA_8_15_POS					    TFT_DATA8_15_MUX0
#define TFT_DATA_16_23_POS					    TFT_DATA16_23_MUX1
#define TFT_TE_POS					            TFT_TE_MUX0
#define TFT_DATA0_7_DIRVING					    IO_DRIVING_lvl0
#define TFT_DATA8_15_DIRVING					IO_DRIVING_lvl0
#define TFT_DATA16_23_DIRVING					IO_DRIVING_lvl0
#define TFT_TE_DIRVING					        IO_DRIVING_lvl0

#define TFT_CTRL_POS							TFT_CTRL_MUX0
#define TFT_CLK_DRIVING						    IO_DRIVING_lvl0//TFT_CTRL_CLK
#define TFT_CTRL_DRIVING						IO_DRIVING_lvl0//TFT_CTRL_DE_HSYNC_VSYNC
#endif

/*== TFT2 PIN MUX ==*/
#define TFT2_ENABLE                             1
#if TFT2_ENABLE
#define TFT2_DATA_0_7_POS					    TFT2_DATA0_7_MUX0
#define TFT2_DATA_8_15_POS					    TFT2_DATA8_15_MUX0
#define TFT2_DATA_16_23_POS					    TFT2_DATA16_23_MUX1
#define TFT2_TE_POS					            TFT2_TE_MUX0
#define TFT2_DATA0_7_DIRVING					IO_DRIVING_lvl0
#define TFT2_DATA8_15_DIRVING					IO_DRIVING_lvl0
#define TFT2_DATA16_23_DIRVING					IO_DRIVING_lvl0
#define TFT2_TE_DIRVING					        IO_DRIVING_lvl0

#define TFT2_CTRL_POS							TFT2_CTRL_MUX0
#define TFT2_CLK_DRIVING						IO_DRIVING_lvl0//TFT_CTRL_CLK
#define TFT2_CTRL_DRIVING						IO_DRIVING_lvl0//TFT_CTRL_DE_HSYNC_VSYNC
#endif

/*== TFT1 SPI PIN MUX ==*/
#define TFT1_SPI_ENABLE                         0
#if TFT1_SPI_ENABLE
#define TFT1_SPI_DATA_0_3_POS					TFT1_SPI_DATA0_3_MUX0
#define TFT1_SPI_TE_POS					        TFT1_SPI_TE_MUX0
#define TFT1_SPI_DATA0_3_DIRVING				IO_DRIVING_lvl0
#define TFT1_SPI_TE_DIRVING					    IO_DRIVING_lvl0
#endif

/*== TFT2 SPI PIN MUX ==*/
#define TFT2_SPI_ENABLE                         0
#if TFT2_SPI_ENABLE
#define TFT2_SPI_DATA_0_3_POS					TFT2_SPI_DATA0_3_MUX0
#define TFT2_SPI_TE_POS					        TFT2_SPI_TE_MUX0
#define TFT2_SPI_DATA0_3_DIRVING				IO_DRIVING_lvl0
#define TFT2_SPI_TE_DIRVING					    IO_DRIVING_lvl0
#endif

/*== CSI0 PIN MUX ==*/
#define CSI0_ENABLE								1
#if CSI0_ENABLE
#define CSI_DATA_2_9_POS					    CSI_DATA2_9_MUX2
#define CSI_CTRL_POS							CSI_CTRL_MUX1
#endif

/*== CSI2 PIN MUX ==*/
#define CSI2_ENABLE								0
#if CSI2_ENABLE
#define CSI2_DATA_2_9_POS					    CSI2_DATA2_9_MUX3
#define CSI2_CTRL_POS							CSI2_CTRL_MUX3
#endif

/*== CDSP PIN MUX ==*/
#define CDSP_ENABLE								0
#if CDSP_ENABLE
#define CDSP_DATA_2_9_POS					    CDSP_DATA2_9_MUX2
#define CDSP_DATA_0_1_POS					    CDSP_DATA0_1_MUX0
#define CDSP_CTRL_POS							CDSP_CTRL_MUX0
#endif

/*== SENSOR CLKO ==*/
#define CDSP_CLKO_ENABLE						1
#if CDSP_CLKO_ENABLE
#define CDSP_CLKO_POS                           CDSP_CLKO_POS_MUX0
#define CDSP_CLKO_SOURCE                        CLKO_FROM_CSI
#define CDSP_CLKO_DRIVING					    IO_DRIVING_lvl1
#endif

#define CSI_CLKO_ENABLE							0
#if CSI_CLKO_ENABLE
#define CSI_CLKO_POS                            CSI_CLKO_POS_MUX2
#define CSI_CLKO_SOURCE                         CLKO_FROM_CSI
#define CSI_CLKO_DRIVING					    IO_DRIVING_lvl1
#endif

/*== ADKEY ==*/
#define ADKEY_DETECT_PIN		ADC_LINE_0
#define ADKEY_NUM				6
#define ADKEY_OK_L				250
#define ADKEY_OK_H				360
#define ADKEY_PREV_L			710
#define ADKEY_PREV_H			810
#define ADKEY_NEXT_L			1260
#define ADKEY_NEXT_H			1370
#define ADKEY_MODE_L			1830
#define ADKEY_MODE_H			1930
#define ADKEY_MENU_L			2220
#define ADKEY_MENU_H			2320
#define ADKEY_SOS_L				2670
#define ADKEY_SOS_H				2780

/*==  POWER KEY ==*/
#define IOKEY_NUM			1
#define PW_KEY_TYPE			READ_FROM_PIN
#define PW_KEY				PWR_KEY0
#define PW_KEY_ACTIVE		DATA_HIGH

/*==  BACK SENSOR detect ==*/
#define BACK_SENSOR_DET				IO_F1
#define BACK_SENSOR_DET_ACTIVE		1
#define BACK_A_CAR_DET				IO_F2
#define BACK_A_CAR_DET_ACTIVE		1
#define BACK_SENSOR_PWR				IO_F3

#define BACK_SENSOR_PWR_DRIVING		IO_DRIVING_lvl0

/*== BATTERY ==*/
#define BATTERY_DETECT_PIN		ADC_LINE_1
#define BATTERY_DET_CNT			(2)
#define BATTERY_LVL_THSR_0		0
#define BATTERY_LVL_THSR_1		2234
#define BATTERY_LVL_THSR_2		2358
#define BATTERY_LVL_THSR_3		2482
#define BATTERY_LVL_GAP			(10)
#define BATTERY_FULL_THSR		508

/*== POWER ==*/
#define CORE_POWER_DETECT_PIN	ADC_LINE_1

/*== USB ==*/
#define USBD_DET_PIN_TYPE	READ_FROM_PIN
#define USBD_DET_PIN		PWR_KEY2
#define USBD_DET_PIN_ACTIVE	DATA_HIGH

/*== LED ==*/
#define LED                    IO_F15
#define LED_PIN_DRIVING        IO_DRIVING_lvl0

/*== User Define GPIO ==*/
#define PANEL_IO_CSN			IO_A8
#define PANEL_IO_SDA			IO_A9
#define PANEL_IO_SCL			IO_A10
#define PANEL_IO_BL_ON			IO_E13
#define PANEL_IO_RESET			IO_F15

#define PANEL_IO_CSN_DRIVING	IO_DRIVING_lvl0
#define PANEL_IO_SDA_DRIVING	IO_DRIVING_lvl0
#define PANEL_IO_SCL_DRIVING	IO_DRIVING_lvl0
#define PANEL_IO_BL_ON_DRIVING	IO_DRIVING_lvl0
#define PANEL_IO_RESET_DRIVING	IO_DRIVING_lvl0

/*== WIFI ==*/
#define WIFI_LDO_EN_PIN		        IO_F15
#define SDIO_WIFI_DEVICE_ID     	0         // 0 or 1
#define SDIO_WIFI_BIT           	4           // 1 or 4
#define SDIO_WIFI_PINMUX        	SDC0_MUX7//SDC0_MUX7//SDC0_MUX1//SDC0_MUX1//SDC1_MUX0  //+SDC0_MUX1   //
#define WIFI_LDO_EN_PIN_DRIVING	    IO_DRIVING_lvl0
#if SDIO_WIFI_BIT==1
#define WIFI_INT_EXTINT			(0)
#define WIFI_INT_KEYCHANGE 	(1)
#define SDIO_WIFI_INT_TYPE		WIFI_INT_EXTINT		/* select INT type, if SDIO WIFI 1 Bit */
#endif


/*== FRONT SENSOR ==*/
#define FRONT_SENSOR_RESET      		IO_F0
#define FRONT_SENSOR_RESET_DRIVING      IO_DRIVING_lvl0

/*== GSENSOR ==*/
#if C_GSENSOR_FUNCTION
	#define G_SENSOR_DEVICE		G_SENSOR_DA380
	#define G_SENSOR_ACTIVE		DATA_HIGH
	#define G_SENSOR_I2C_SCL	IO_D14
	#define G_SENSOR_I2C_SDA	IO_D15
#else
	#define G_SENSOR_DEVICE		G_SENSOR_NONE
#endif

/*== HDMI HPD ==*/
#if (LCD_DRV_EN_HDMI==1)
#define  HDMI_HPD_PIN			IO_B14
#endif

/*== SENSOR LDO 2V8 ==*/
#define SEN_LDO_2V8_ENABLE		0

#if SEN_LDO_2V8_ENABLE
#define SEN_LDO_2V8_VALUE		LDO_V20
#endif

#define USBD_USE_USB_HW_IDX         	    USB_B

#if (_SENSOR_USB_MJPEG_CAM_FRONT)
#define HOST_UVC_PORT_FRONT		USB_A
#endif

#if (_SENSOR_USB_MJPEG_CAM_REAR)
#define HOST_UVC_PORT_REAR		USB_B
#endif

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void board_init(void);

#endif

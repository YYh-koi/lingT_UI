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
#include "drv_l1.h"
#include "board_config.h"
#include "bus_priority_cfg.h"

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void bus_priority_config(void)
{
	INT32U	i;

	for(i = 0; i < sizeof(Bus_Pry_Cfg) / sizeof(BUS_PRY_ARGS); i++)
	{
		DBG_PRINT("[%d] bus num:%d wc:0x%x lowest:%d \r\n",
		i,
		Bus_Pry_Cfg[i].BP_num+1, // Start from 0
		Bus_Pry_Cfg[i].waiting_count,
		Bus_Pry_Cfg[i].enable_preempt
		);

		drv_l1_bus_priority_set(&Bus_Pry_Cfg[i]);
	}
}

void board_clock_config(void)
{
	INT32U	i;

	drv_l1_usbd_phy_force_suspend(USB_A,1);
	drv_l1_usbd_phy_force_suspend(USB_B,1);

	struct CLOCK_DEF
	{
		SYSTEM_CLK_EN_E sys_dev;
	}

	clock_off[] =
	{
		/*=== Default ===*/
		// 26
		{CLK_EN1_USB20_HOST_A},
		// 27
		{CLK_EN1_USB20_DEVICE_A},
		// 29
		{CLK_EN1_SPU},
		// 31
		{CLK_EN1_SPU_SLAVE},
		// 57
		{CLK_EN3_NAND_FLASH_CTRL},
		// 62
		{CLK_EN3_IPU},
		// 68
		{CLK_EN4_ETHERNET_MAC},
		// 77
		{CLK_EN4_MP3},
		// 79
		{CLK_EN4_BCH},
		// 82
		{CLK_EN5_FIR},
		// 85
		{CLK_EN5_HDMI_AUDIO},
		// 87
		{CLK_EN5_TRNG},
		// 90
		{CLK_EN5_HDMI},
		// 94
		{CLK_EN5_USB20_HOST_B},
		// 98
		{CLK_EN6_FFT},
		// 105
		{CLK_EN6_LBP},
		// 106
		{CLK_EN6_USB20_DEVICE_B},
		// 107
		{CLK_EN6_BOX_FILTER},
		// 112
		{CLK_EN7_COMPRESSOR},
		// 113
		{CLK_EN7_GPU},
		// 114
		{CLK_EN7_SECURITY_IP},
		/*=== Option ===*/

		#if UART0_ENABLE == 0
		// 32
		{CLK_EN2_UART0_REG_BANK},
		// 33
		{CLK_EN2_UART0},
		#endif

		#if UART1_ENABLE == 0
		// 34
		{CLK_EN2_UART1_REG_BANK},
		// 35
		{CLK_EN2_UART1},
		#endif

		#if UART2_ENABLE == 0
		// 36
		{CLK_EN2_UART2_REG_BANK},
		// 37
		{CLK_EN2_UART2},
		#endif

		#if I2C0_ENABLE == 0
		// 38
		{CLK_EN2_I2C0},
		#endif

		#if I2C1_ENABLE == 0
		// 39
		{CLK_EN2_I2C1},
		#endif

		#if I2C2_ENABLE == 0
		// 40
		{CLK_EN2_I2C2},
		#endif

		#if SPI0_ENABLE == 0
		// 41
		{CLK_EN2_SPI0},
		#endif

		#if SPI1_ENABLE == 0
		// 42
		{CLK_EN2_SPI1},
		#endif

		#if SDC0_ENABLE == 0
		// 63
		{CLK_EN3_SD0_REG_BANK},
		// 64
		{CLK_EN4_SD0},
		#endif

		#if SDC1_ENABLE == 0
		//65
		{CLK_EN4_SD2_REG_BANK},
		//67
		{CLK_EN4_SD2},
		#endif

		#if (TFT1_SPI_ENABLE == 0) && (TFT2_SPI_ENABLE == 0)
		// 75
		{CLK_EN4_TFT_SPI1},
		// 76
		{CLK_EN4_TFT_SPI0},
		#endif

		#if (TFT1_ENABLE == 0) && (TFT2_ENABLE == 0)
		// 21
		{CLK_EN1_TFT},
		#endif

		// 111
		{CLK_EN6_GPDLA},
	};

	for(i = 0; i < sizeof(clock_off) / sizeof(struct CLOCK_DEF); i++)
	{
		//DBG_PRINT("[%d] close %d clk\r\n",i, clock_off[i].sys_dev);
		drv_l1_clock_set_system_clk_en(clock_off[i].sys_dev ,0);
	}
}

void board_gpio_config(void)
{
	INT32U	i;

	struct GPIO_DEF
	{
		INT32U port;
		enum
		{
			GPIO_IN_FLOAT,
			GPIO_IN_PULL_LOW,
			GPIO_IN_PULL_HIGH,
			GPIO_OUT_NORMAL,
			GPIO_OUT_INVERT
		} attr;
		BOOLEAN			out_init;
		IO_DRV_LEVEL	out_driving;
	}

	gpio_def[] =
	{

		#ifdef SPEAKER_EN_PIN
		{ SPEAKER_EN_PIN, GPIO_OUT_NORMAL, SPEAKER_EN_PIN_ACTIVE, SPEAKER_EN_PIN_DRIVING},
		#endif

		#ifdef PANEL_IO_CSN
		{ PANEL_IO_CSN, GPIO_OUT_NORMAL, DATA_HIGH, PANEL_IO_CSN_DRIVING},
		#endif

		#ifdef PANEL_IO_CSN2
		{ PANEL_IO_CSN2, GPIO_OUT_NORMAL, DATA_HIGH, PANEL_IO_CSN_DRIVING},
		#endif


		#ifdef PANEL_IO_SCL
		{ PANEL_IO_SCL, GPIO_OUT_NORMAL,DATA_LOW,PANEL_IO_SCL_DRIVING},
		#endif

		#ifdef PANEL_IO_SDA
		{ PANEL_IO_SDA, GPIO_OUT_NORMAL, DATA_LOW, PANEL_IO_SDA_DRIVING},
		#endif

		#ifdef PANEL_IO_RESET
		{ PANEL_IO_RESET, GPIO_OUT_NORMAL, DATA_HIGH, PANEL_IO_RESET_DRIVING},
		#endif

		#ifdef PANEL_IO_RESET2
		{ PANEL_IO_RESET2, GPIO_OUT_NORMAL, DATA_HIGH, PANEL_IO_RESET_DRIVING},
		#endif

		#ifdef BACK_SENSOR_DET
		{ BACK_SENSOR_DET, GPIO_IN_PULL_LOW },
		#endif

		#ifdef BACK_A_CAR_DET
		{ BACK_A_CAR_DET, GPIO_IN_PULL_LOW },
		#endif

		#ifdef BACK_SENSOR_PWR
		{ BACK_SENSOR_PWR, GPIO_OUT_NORMAL, DATA_HIGH, BACK_SENSOR_PWR_DRIVING },
		#endif
		#ifdef BACK_SENSOR_RESET
		{ BACK_SENSOR_RESET, GPIO_OUT_NORMAL, DATA_HIGH, BACK_SENSOR_RESET_DRIVING },
		#endif

		#ifdef BACK_SENSOR_PWR_LIMIT
		{ BACK_SENSOR_PWR_LIMIT, GPIO_OUT_NORMAL, DATA_HIGH, BACK_SENSOR_PWR_LIMIT_DRIVING },
		#endif


		#ifdef PANEL_IO_BL_ON
		{ PANEL_IO_BL_ON, GPIO_OUT_NORMAL, DATA_LOW, PANEL_IO_BL_ON_DRIVING },
		#endif

		#ifdef PANEL_IO_BL2_ON
		{ PANEL_IO_BL2_ON, GPIO_OUT_NORMAL, DATA_LOW, PANEL_IO_BL_ON_DRIVING },
		#endif

		{ PANEL_IO_CSN, GPIO_OUT_NORMAL, DATA_HIGH, IO_DRIVING_lvl1 },

		#ifdef PANEL_IO_CSN2
		{ PANEL_IO_CSN2, GPIO_OUT_NORMAL, DATA_HIGH, IO_DRIVING_lvl1 },
		#endif

		#ifdef SDC_DETECT_PIN
		{ SDC_DETECT_PIN, GPIO_IN_PULL_HIGH },
		#endif

		#ifdef FRONT_SENSOR_RESET
		{ FRONT_SENSOR_RESET, GPIO_OUT_NORMAL, DATA_HIGH, FRONT_SENSOR_RESET_DRIVING },
		#endif

		#ifdef FRONT_SENSOR_PWR_EN
		{ FRONT_SENSOR_PWR_EN, GPIO_OUT_NORMAL, DATA_HIGH, FRONT_SENSOR_PWR_EN_DRIVING },
		#endif

        #ifdef FRONT_SENSOR_PWDN
		{ FRONT_SENSOR_PWDN, GPIO_OUT_NORMAL, DATA_LOW, FRONT_SENSOR_PWDN_DRIVING },
		#endif

		#ifdef OK_KEY
		{ OK_KEY, GPIO_IN_PULL_LOW },
		#endif

        #ifdef  LED
		{ LED, GPIO_OUT_NORMAL, DATA_LOW, LED_PIN_DRIVING },
		#endif

		#if defined(USBD_DET_PIN) && USBD_DET_PIN_TYPE == READ_FROM_GPIO
		{ USBD_DET_PIN, GPIO_IN_PULL_LOW},
		#endif

        #ifdef ACC_DET_PIN
		{ ACC_DET_PIN, GPIO_IN_PULL_LOW},
		#endif

        #ifdef  WIFI_LDO_EN_PIN
		{ WIFI_LDO_EN_PIN, GPIO_OUT_NORMAL, DATA_LOW,WIFI_LDO_EN_PIN_DRIVING},
		#endif

		#ifdef  HDMI_HPD_PIN
		{HDMI_HPD_PIN, GPIO_IN_PULL_LOW}
		#endif

	};

	for(i = 0; i < sizeof(gpio_def) / sizeof(struct GPIO_DEF); i++)
	{
		switch(gpio_def[i].attr)
		{
		case GPIO_IN_FLOAT:
			{
				gpio_init_io(gpio_def[i].port, GPIO_INPUT);
				gpio_set_port_attribute(gpio_def[i].port, ATTRIBUTE_HIGH);
			}

			break;

		case GPIO_IN_PULL_LOW:
			{
				gpio_init_io(gpio_def[i].port, GPIO_INPUT);
				gpio_set_port_attribute(gpio_def[i].port, ATTRIBUTE_LOW);
				gpio_write_io(gpio_def[i].port, DATA_LOW);
			}

			break;

		case GPIO_IN_PULL_HIGH:
			{
				gpio_init_io(gpio_def[i].port, GPIO_INPUT);
				gpio_set_port_attribute(gpio_def[i].port, ATTRIBUTE_LOW);
				gpio_write_io(gpio_def[i].port, DATA_HIGH);
			}

			break;

		case GPIO_OUT_NORMAL:
			{
				gpio_init_io(gpio_def[i].port, GPIO_OUTPUT);
				gpio_set_port_attribute(gpio_def[i].port, ATTRIBUTE_HIGH);
				gpio_drving_init_io(gpio_def[i].port, gpio_def[i].out_driving);
				gpio_write_io(gpio_def[i].port, gpio_def[i].out_init);
			}

			break;

		case GPIO_OUT_INVERT:
			{
				gpio_init_io(gpio_def[i].port, GPIO_OUTPUT);
				gpio_set_port_attribute(gpio_def[i].port, ATTRIBUTE_LOW);
				gpio_drving_init_io(gpio_def[i].port, gpio_def[i].out_driving);
				gpio_write_io(gpio_def[i].port, gpio_def[i].out_init);
			}

			break;
		}
	}
}


#if UART0_ENABLE
INT8S uart0_pinmux_set(INT8U idx, INT8U tx_enable, INT8U rx_enable)
{
	INT32U funpos0 = R_FUNPOS0;
	INT32U extioen = R_EXTIO_EN;

	extioen &= ~(3<<8);
	extioen |= (tx_enable<<8) | (rx_enable<<9);

	if(idx > 3)
		return MUX_ERR;

	funpos0 &= ~0x3;
	if( idx == UART0_MUX0){//UART0_TX_RX__IOB5_IOB4
		funpos0 &= ~(3<<0);
		gpio_drving_init_io(IO_B4,(IO_DRV_LEVEL)UART0_DRIVING);
		gpio_drving_init_io(IO_B5,(IO_DRV_LEVEL)UART0_DRIVING);
	}
	else if( idx == UART0_MUX1){//UART0_TX_RX__IOC12_IOC13
		funpos0 |= (1<<0);
		gpio_drving_init_io(IO_C12,(IO_DRV_LEVEL)UART0_DRIVING);
		gpio_drving_init_io(IO_C13,(IO_DRV_LEVEL)UART0_DRIVING);
	}
	else if (idx == UART0_MUX2){//UART0_TX_RX__IOD5_IOD4
		funpos0 |= (2<<0);
		gpio_drving_init_io(IO_D4,(IO_DRV_LEVEL)UART0_DRIVING);
		gpio_drving_init_io(IO_D5,(IO_DRV_LEVEL)UART0_DRIVING);
	}
    else if(idx == UART0_MUX3){//UART0_TX_RX__IOA3_IOA2
        funpos0 |= (3<<0);
        gpio_drving_init_io(IO_A2,(IO_DRV_LEVEL)UART0_DRIVING);
		gpio_drving_init_io(IO_A3,(IO_DRV_LEVEL)UART0_DRIVING);
    }

    R_EXTIO_EN = extioen;
	R_FUNPOS0 = funpos0;
	return MUX_OK;
}
#endif

#if UART1_ENABLE
INT8S uart1_pinmux_set(INT8U idx, INT8U tx_enable, INT8U rx_enable)
{
	INT32U funpos0 = R_FUNPOS0;
	INT32U extioen = R_EXTIO_EN;

	extioen &= ~(3<<10);
	extioen |= (tx_enable<<10) | (rx_enable<<11);

	if(idx > 3)
		return MUX_ERR;

	funpos0 &= ~0xc;
    if (idx == UART1_MUX0){//UART1_TX_RX__IOB7_IOB6
        funpos0 &= ~(3<<2);
        gpio_drving_init_io(IO_B6,(IO_DRV_LEVEL)UART1_DRIVING);
        gpio_drving_init_io(IO_B7,(IO_DRV_LEVEL)UART1_DRIVING);
	}
    else if (idx == UART1_MUX1){//UART1_TX_RX__IOA5_IOA4
        funpos0 |= (1<<2);
        gpio_drving_init_io(IO_A4,(IO_DRV_LEVEL)UART1_DRIVING);
        gpio_drving_init_io(IO_A5,(IO_DRV_LEVEL)UART1_DRIVING);
    }
    else if (idx == UART1_MUX2){//UART1_TX_RX__IOD9_IOD8
        funpos0 |= (2<<2);
        gpio_drving_init_io(IO_D8,(IO_DRV_LEVEL)UART1_DRIVING);
        gpio_drving_init_io(IO_D9,(IO_DRV_LEVEL)UART1_DRIVING);
     }
    else if (idx == UART1_MUX3){//UART1_TX_RX__IOA11_IOA10
        funpos0 |= (3<<2);
        gpio_drving_init_io(IO_A11,(IO_DRV_LEVEL)UART1_DRIVING);
        gpio_drving_init_io(IO_A10,(IO_DRV_LEVEL)UART1_DRIVING);
     }

    R_EXTIO_EN = extioen;
	R_FUNPOS0 = funpos0;
	return MUX_OK;
}
#endif

#if UART2_ENABLE
INT8S uart2_pinmux_set(INT8U idx, INT8U tx_enable, INT8U rx_enable)
{
	INT32U funpos1 = R_FUNPOS1;
    INT32U extioen = R_EXTIO_EN;

	extioen &= ~(3<<12);
	extioen |= (tx_enable<<12) | (rx_enable<<13);
	if(idx > 3)
        return MUX_ERR;

	funpos1 &= ~(3<<13);
    if (idx == UART2_MUX0){//UART2_TX_RX__IOB12_IOB11
        funpos1 &= ~(3<<13);
        gpio_drving_init_io(IO_B11,(IO_DRV_LEVEL)UART2_DRIVING);
        gpio_drving_init_io(IO_B12,(IO_DRV_LEVEL)UART2_DRIVING);
	}
    else if (idx == UART2_MUX1){//UART2_TX_RX__IOA7_IOA6
        funpos1 |= (1<<13);
        gpio_drving_init_io(IO_A6,(IO_DRV_LEVEL)UART2_DRIVING);
        gpio_drving_init_io(IO_A7,(IO_DRV_LEVEL)UART2_DRIVING);
	}
    else if (idx == UART2_MUX2){//UART2_TX_RX__IOC3_IOC2
        funpos1 |= (2<<13);
        gpio_drving_init_io(IO_C2,(IO_DRV_LEVEL)UART2_DRIVING);
        gpio_drving_init_io(IO_C3,(IO_DRV_LEVEL)UART2_DRIVING);
    }
    else if (idx == UART2_MUX3){//UART2_TX_RX__IOC5_IOC4
        funpos1 |= (3<<13);
        gpio_drving_init_io(IO_C4,(IO_DRV_LEVEL)UART2_DRIVING);
        gpio_drving_init_io(IO_C5,(IO_DRV_LEVEL)UART2_DRIVING);
    }
    R_EXTIO_EN = extioen;
	R_FUNPOS1 = funpos1;
	return MUX_OK;
}
#endif

#if EXTA_IRQ_ENABLE
INT8S extA_irq_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS1;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<0);
    if (idx == EXTA_IRQ_MUX0)//EXT_IRQ_A__IOA10
        timerpos &= ~(1<<0);
    else if (idx == EXTA_IRQ_MUX1)//EXT_IRQ_A__IOC12
        timerpos |= (1<<0);
    else if (idx == EXTA_IRQ_MUX2)//EXT_IRQ_A__IOD7
        timerpos |= (2<<0);
    else if (idx == EXTA_IRQ_MUX3 )//EXT_IRQ_A__IOD12
        timerpos |= (3<<0);

	R_TIMER_POS1 = timerpos;
	return MUX_OK;
}
#endif

#if EXTB_IRQ_ENABLE
INT8S extB_irq_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS1;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<2);
    if (idx == EXTB_IRQ_MUX0)//EXT_IRQ_B__IOA11
        timerpos &= ~(1<<2);
    else if (idx == EXTB_IRQ_MUX1)//EXT_IRQ_B__IOC13
        timerpos |= (1<<2);
    else if (idx == EXTB_IRQ_MUX2)//EXT_IRQ_B__IOD8
        timerpos |= (2<<2);
    else if (idx == EXTB_IRQ_MUX3 )//EXT_IRQ_B__IOD14
        timerpos |= (3<<2);

	R_TIMER_POS1 = timerpos;
	return MUX_OK;
}
#endif

#if EXTC_IRQ_ENABLE
INT8S extC_irq_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS1;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<4);
    if (idx == EXTC_IRQ_MUX0)//EXT_IRQ_C__IOA12
        timerpos &= ~(1<<4);
    else if (idx == EXTC_IRQ_MUX1)//EXT_IRQ_C__IOC14
        timerpos |= (1<<4);
    else if (idx == EXTC_IRQ_MUX2)//EXT_IRQ_C__IOD9
        timerpos |= (2<<4);
    else if (idx == EXTC_IRQ_MUX3 )//EXT_IRQ_C__IOD11
        timerpos |= (3<<4);

	R_TIMER_POS1 = timerpos;
	return MUX_OK;
}
#endif

#if EXTD_IRQ_ENABLE
INT8S extD_irq_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS1;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<6);
    if (idx == EXTD_IRQ_MUX0)//EXT_IRQ_D__IOA13
        timerpos &= ~(1<<6);
    else if (idx == EXTD_IRQ_MUX1)//EXT_IRQ_D__IOF2
        timerpos |= (1<<6);
    else if (idx == EXTD_IRQ_MUX2)//EXT_IRQ_D__IOF3
        timerpos |= (2<<6);
    else if (idx == EXTD_IRQ_MUX3 )//EXT_IRQ_D__IOE10
        timerpos |= (3<<6);

	R_TIMER_POS1 = timerpos;
	return MUX_OK;
}
#endif

#if EXTE_IRQ_ENABLE
INT8S extE_irq_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS1;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<8);
    if (idx == EXTE_IRQ_MUX0)//EXT_IRQ_E__IOB6
        timerpos &= ~(1<<8);
    else if (idx == EXTE_IRQ_MUX1)//EXT_IRQ_E__IOF1
        timerpos |= (1<<8);
    else if (idx == EXTE_IRQ_MUX2)//EXT_IRQ_E__IOF4
        timerpos |= (2<<8);
    else if (idx == EXTE_IRQ_MUX3 )//EXT_IRQ_E__IOE11
        timerpos |= (3<<8);

	R_TIMER_POS1 = timerpos;
	return MUX_OK;
}
#endif

#if EXTF_IRQ_ENABLE
INT8S extF_irq_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS1;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<10);
    if (idx == EXTF_IRQ_MUX0)//EXT_IRQ_F__IOB7
        timerpos &= ~(1<<10);
    else if (idx == EXTF_IRQ_MUX1)//EXT_IRQ_F__IOF0
        timerpos |= (1<<10);
    else if (idx == EXTF_IRQ_MUX2)//EXT_IRQ_F__IOF5
        timerpos |= (2<<10);
    else if (idx == EXTF_IRQ_MUX3 )//EXT_IRQ_F__IOE12
        timerpos |= (3<<10);

	R_TIMER_POS1 = timerpos;
	return MUX_OK;
}
#endif

#if EXTG_IRQ_ENABLE
INT8S extG_irq_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS1;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<12);
    if (idx == EXTF_IRQ_MUX0)//EXT_IRQ_F__IOB8
        timerpos &= ~(1<<12);
    else if (idx == EXTF_IRQ_MUX1)//EXT_IRQ_F__IOE6
        timerpos |= (1<<12);
    else if (idx == EXTF_IRQ_MUX2)//EXT_IRQ_F__IOA14
        timerpos |= (2<<12);
    else if (idx == EXTF_IRQ_MUX3 )//EXT_IRQ_F__IOA2
        timerpos |= (3<<12);

	R_TIMER_POS1 = timerpos;
	return MUX_OK;
}
#endif

#if EXTH_IRQ_ENABLE
INT8S extH_irq_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS1;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<14);
    if (idx == EXTF_IRQ_MUX0)//EXT_IRQ_F__IOC2
        timerpos &= ~(1<<14);
    else if (idx == EXTF_IRQ_MUX1)//EXT_IRQ_F__IOE7
        timerpos |= (1<<14);
    else if (idx == EXTF_IRQ_MUX2)//EXT_IRQ_F__IOA15
        timerpos |= (2<<14);
    else if (idx == EXTF_IRQ_MUX3 )//EXT_IRQ_F__IOA4
        timerpos |= (3<<14);

	R_TIMER_POS1 = timerpos;
	return MUX_OK;
}
#endif

#if EXTI_IRQ_ENABLE
INT8S extI_irq_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS1;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<16);
    if (idx == EXTF_IRQ_MUX0)//EXT_IRQ_F__IOC3
        timerpos &= ~(1<<16);
    else if (idx == EXTF_IRQ_MUX1)//EXT_IRQ_F__IOE8
        timerpos |= (1<<16);
    else if (idx == EXTF_IRQ_MUX2)//EXT_IRQ_F__IOB0
        timerpos |= (2<<16);
    else if (idx == EXTF_IRQ_MUX3 )//EXT_IRQ_F__IOA5
        timerpos |= (3<<16);

	R_TIMER_POS1 = timerpos;
	return MUX_OK;
}
#endif

#if EXTJ_IRQ_ENABLE
INT8S extJ_irq_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS1;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<18);
    if (idx == EXTF_IRQ_MUX0)//EXT_IRQ_F__IOB2
        timerpos &= ~(1<<18);
    else if (idx == EXTF_IRQ_MUX1)//EXT_IRQ_F__IOE9
        timerpos |= (1<<18);
    else if (idx == EXTF_IRQ_MUX2)//EXT_IRQ_F__IOB1
        timerpos |= (2<<18);
    else if (idx == EXTF_IRQ_MUX3 )//EXT_IRQ_F__IOD4
        timerpos |= (3<<18);

	R_TIMER_POS1 = timerpos;
	return MUX_OK;
}
#endif

#if EXTK_IRQ_ENABLE
INT8S extK_irq_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS1;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<20);
    if (idx == EXTF_IRQ_MUX0)//EXT_IRQ_F__IOB3
        timerpos &= ~(1<<20);
    else if (idx == EXTF_IRQ_MUX1)//EXT_IRQ_F__IOE2
        timerpos |= (1<<20);
    else if (idx == EXTF_IRQ_MUX2)//EXT_IRQ_F__IOB11
        timerpos |= (2<<20);
    else if (idx == EXTF_IRQ_MUX3 )//EXT_IRQ_F__IOD5
        timerpos |= (3<<20);

	R_TIMER_POS1 = timerpos;
	return MUX_OK;
}
#endif

#if EXTL_IRQ_ENABLE
INT8S extL_irq_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS1;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<22);
    if (idx == EXTF_IRQ_MUX0)//EXT_IRQ_F__IOD15
        timerpos &= ~(1<<22);
    else if (idx == EXTF_IRQ_MUX1)//EXT_IRQ_F__IOE3
        timerpos |= (1<<22);
    else if (idx == EXTF_IRQ_MUX2)//EXT_IRQ_F__IOB12
        timerpos |= (2<<22);
    else if (idx == EXTF_IRQ_MUX3 )//EXT_IRQ_F__IOD6
        timerpos |= (3<<22);

	R_TIMER_POS1 = timerpos;
	return MUX_OK;
}
#endif

#if I2C0_ENABLE
INT8S i2c0_pinmux_set(INT8U idx)
{
	INT32U funpos0 = R_FUNPOS0;

    if (idx > 3)
        return MUX_ERR;

	funpos0 &= ~(3<<25);
    if (idx == I2C0_MUX0){//I2C_SCL_SDA__IOB4_IOB5
        funpos0 &= ~(3<<25);
        gpio_drving_init_io(IO_B4,(IO_DRV_LEVEL)I2C0_DRIVING);
        gpio_drving_init_io(IO_B5,(IO_DRV_LEVEL)I2C0_DRIVING);
	}
    else if (idx == I2C0_MUX1){//I2C_SCL_SDA__IOC12_IOC13
        funpos0 |= (1<<25);
        gpio_drving_init_io(IO_C12,(IO_DRV_LEVEL)I2C0_DRIVING);
        gpio_drving_init_io(IO_C13,(IO_DRV_LEVEL)I2C0_DRIVING);
	}
    else if (idx == I2C0_MUX2){//I2C_SCL_SDA__IOA8_IOA9
        funpos0 |= (2<<25);
        gpio_drving_init_io(IO_A8,(IO_DRV_LEVEL)I2C0_DRIVING);
        gpio_drving_init_io(IO_A9,(IO_DRV_LEVEL)I2C0_DRIVING);
	}
    else if (idx == I2C0_MUX3){//I2C_SCL_SDA__IOB14_IOB15
        funpos0 |= (3<<25);
        gpio_drving_init_io(IO_B14,(IO_DRV_LEVEL)I2C0_DRIVING);
        gpio_drving_init_io(IO_B15,(IO_DRV_LEVEL)I2C0_DRIVING);
	}

	R_FUNPOS0 = funpos0;
	return MUX_ERR;
}
#endif

#if I2C1_ENABLE
INT8S i2c1_pinmux_set(INT8U idx)
{
	INT32U funpos1 = R_FUNPOS1;

    if (idx > 2)
        return MUX_ERR;

	funpos1 &= ~(3<<29);
    if (idx == I2C1_MUX0){//I2C1_SCL_SDA__IOC0_IOC1
        funpos1 &= ~(1<<29);
        gpio_drving_init_io(IO_C0,(IO_DRV_LEVEL)I2C1_DRIVING);
        gpio_drving_init_io(IO_C1,(IO_DRV_LEVEL)I2C1_DRIVING);
	}
    else if (idx == I2C1_MUX1){//I2C_SCL_SDA__IOD4_IOD5
        funpos1 |= (1<<29);
        gpio_drving_init_io(IO_D4,(IO_DRV_LEVEL)I2C1_DRIVING);
        gpio_drving_init_io(IO_D5,(IO_DRV_LEVEL)I2C1_DRIVING);
	}
    else if (idx == I2C1_MUX2){//I2C_SCL_SDA__IOC6_IOC7
        funpos1 |= (2<<29);
        gpio_drving_init_io(IO_C6,(IO_DRV_LEVEL)I2C1_DRIVING);
        gpio_drving_init_io(IO_C7,(IO_DRV_LEVEL)I2C1_DRIVING);
	}
	R_FUNPOS1 = funpos1;
	return MUX_OK;
}
#endif

#if I2C2_ENABLE
INT8S i2c2_pinmux_set(INT8U idx)
{
	INT32U funpos1 = R_FUNPOS1;

    if (idx > 3)
        return MUX_ERR;

	funpos1 &= ~(3<<16);
    if (idx == I2C2_MUX0){//I2C2_SCL_SDA__IOD14_IOD15
        funpos1 &= ~(3<<16);
        gpio_drving_init_io(IO_D14,(IO_DRV_LEVEL)I2C2_DRIVING);
        gpio_drving_init_io(IO_D15,(IO_DRV_LEVEL)I2C2_DRIVING);
	}
    else if (idx == I2C2_MUX1){//I2C1_SCL_SDA__IOD10_IOD12
        funpos1 |= (1<<16);
        gpio_drving_init_io(IO_D10,(IO_DRV_LEVEL)I2C2_DRIVING);
        gpio_drving_init_io(IO_D12,(IO_DRV_LEVEL)I2C2_DRIVING);
	}
    else if (idx == I2C2_MUX2){//I2C1_SCL_SDA__IOA10_IOA11
        funpos1 |= (2<<16);
        gpio_drving_init_io(IO_A10,(IO_DRV_LEVEL)I2C2_DRIVING);
        gpio_drving_init_io(IO_A11,(IO_DRV_LEVEL)I2C2_DRIVING);
	}
    else if (idx == I2C2_MUX3){//I2C1_SCL_SDA__IOB10_IOB13
        funpos1 |= (3<<16);
        gpio_drving_init_io(IO_B10,(IO_DRV_LEVEL)I2C2_DRIVING);
        gpio_drving_init_io(IO_B13,(IO_DRV_LEVEL)I2C2_DRIVING);
	}
	R_FUNPOS1 = funpos1;
	return MUX_OK;
}
#endif

#if SPI0_ENABLE
INT8S spi0_pinmux_set(INT8U idx, INT8U cs_gpio_en)
{
	INT32U funpos0 = R_FUNPOS0;
	INT32U gpiofunpos = R_GPIO_CTRL;

    if (idx > 1)
        return MUX_ERR;

	funpos0 &= ~(1<<14);
    if (idx == SPI0_MUX0){//SPI_CS_CLK_TX_RX__IOD6_IOD7_IOD8_IOD9_IOC0_IOC1
        funpos0 &= ~(1<<14);
        gpio_drving_init_io(IO_D6,(IO_DRV_LEVEL)SPI0_DRIVING);
        gpio_drving_init_io(IO_D7,(IO_DRV_LEVEL)SPI0_DRIVING);
        gpio_drving_init_io(IO_D8,(IO_DRV_LEVEL)SPI0_DRIVING);
        gpio_drving_init_io(IO_D9,(IO_DRV_LEVEL)SPI0_DRIVING);
        gpio_drving_init_io(IO_C0,(IO_DRV_LEVEL)SPI0_DRIVING);
        gpio_drving_init_io(IO_C1,(IO_DRV_LEVEL)SPI0_DRIVING);
	}
    else if (idx == SPI0_MUX1){//SPI0_CS_CLK_TX_RX__IOD0_IOD1_IOD2_IOD3_IOD4_IOD5
        funpos0 |= (1<<14);
        gpio_drving_init_io(IO_D0,(IO_DRV_LEVEL)SPI0_DRIVING);
        gpio_drving_init_io(IO_D1,(IO_DRV_LEVEL)SPI0_DRIVING);
        gpio_drving_init_io(IO_D2,(IO_DRV_LEVEL)SPI0_DRIVING);
        gpio_drving_init_io(IO_D3,(IO_DRV_LEVEL)SPI0_DRIVING);
        gpio_drving_init_io(IO_D4,(IO_DRV_LEVEL)SPI0_DRIVING);
        gpio_drving_init_io(IO_D5,(IO_DRV_LEVEL)SPI0_DRIVING);
    }

    if(cs_gpio_en)
        gpiofunpos |= (1<<3);
    else
        gpiofunpos &= ~(1<<3);

	R_FUNPOS0 = funpos0;
	R_GPIO_CTRL = gpiofunpos;
	return MUX_OK;
}
#endif

#if SPI1_ENABLE
INT8S spi1_pinmux_set(INT8U idx, INT8U cs_gpio_en)
{
	INT32U funpos0 = R_FUNPOS0;
    INT32U gpiofunpos = R_GPIO_CTRL;

    if (idx > 1)
        return MUX_ERR;

	funpos0 &= ~(1<<15);
    if (idx == SPI1_MUX0){//SPI1_CS_CLK_TX_RX__IOC8_IOC9_IOC10_IOC11_IOC2_IOC3
        funpos0 &= ~(1<<15);
        gpio_drving_init_io(IO_C8,(IO_DRV_LEVEL)SPI1_DRIVING);
        gpio_drving_init_io(IO_C9,(IO_DRV_LEVEL)SPI1_DRIVING);
        gpio_drving_init_io(IO_C10,(IO_DRV_LEVEL)SPI1_DRIVING);
        gpio_drving_init_io(IO_C11,(IO_DRV_LEVEL)SPI1_DRIVING);
        gpio_drving_init_io(IO_C2,(IO_DRV_LEVEL)SPI1_DRIVING);
        gpio_drving_init_io(IO_C3,(IO_DRV_LEVEL)SPI1_DRIVING);
    }
    else if (idx == SPI1_MUX1){//SPI1_CS_CLK_TX_RX__IOD10_IOD11_IOD12_IOD13_IOD14_IOD15
        funpos0 |= (1<<15);
        gpio_drving_init_io(IO_D10,(IO_DRV_LEVEL)SPI1_DRIVING);
        gpio_drving_init_io(IO_D11,(IO_DRV_LEVEL)SPI1_DRIVING);
        gpio_drving_init_io(IO_D12,(IO_DRV_LEVEL)SPI1_DRIVING);
        gpio_drving_init_io(IO_D13,(IO_DRV_LEVEL)SPI1_DRIVING);
        gpio_drving_init_io(IO_D14,(IO_DRV_LEVEL)SPI1_DRIVING);
        gpio_drving_init_io(IO_D15,(IO_DRV_LEVEL)SPI1_DRIVING);
    }

    if(cs_gpio_en)
        gpiofunpos &= ~(1<<9);
    else
        gpiofunpos |= (1<<9);

	R_FUNPOS0 = funpos0;
	R_GPIO_CTRL = gpiofunpos;
    return MUX_OK;
}
#endif

#if SPIFC_ENABLE
INT8S spifc_pinmux_set(INT8U idx)
{
	INT32U funpos0 = R_FUNPOS0;

    if (idx >1)
        return MUX_ERR;
    if (idx == SPIFC_MUX0){//SPIFC_CS_CLK_RX0_3__IOD0_IOD1_IOD2_IOD3_IOD4_IOD5
        funpos0 &= ~(1<<30);
        gpio_drving_init_io(IO_D0,(IO_DRV_LEVEL)SPIFC_DRIVING);
        gpio_drving_init_io(IO_D1,(IO_DRV_LEVEL)SPIFC_CLK_DRIVING);
        gpio_drving_init_io(IO_D2,(IO_DRV_LEVEL)SPIFC_DRIVING);
        gpio_drving_init_io(IO_D3,(IO_DRV_LEVEL)SPIFC_DRIVING);
        gpio_drving_init_io(IO_D4,(IO_DRV_LEVEL)SPIFC_DRIVING);
        gpio_drving_init_io(IO_D5,(IO_DRV_LEVEL)SPIFC_DRIVING);
    }
    else if (idx == SPIFC_MUX1){//SPIFC_CS_CLK_RX0_3__IOD7_IOA14_IOA12_IOD8_IOD9_IOA15
        funpos0 |= (1<<30);
        gpio_drving_init_io(IO_D7,(IO_DRV_LEVEL)SPIFC_DRIVING);
        gpio_drving_init_io(IO_A14,(IO_DRV_LEVEL)SPIFC_CLK_DRIVING);
        gpio_drving_init_io(IO_A12,(IO_DRV_LEVEL)SPIFC_DRIVING);
        gpio_drving_init_io(IO_D8,(IO_DRV_LEVEL)SPIFC_DRIVING);
        gpio_drving_init_io(IO_D9,(IO_DRV_LEVEL)SPIFC_DRIVING);
        gpio_drving_init_io(IO_A15,(IO_DRV_LEVEL)SPIFC_DRIVING);
    }

	R_FUNPOS0 = funpos0;
	return MUX_OK;
}
#endif

static INT8U sdc_voltage[2] = {0, 0}; // 0 mean use 3.3V, 1 use 1.8V
                                      // GPA7xxxA only SDC1_MUX4 support 1.8V

void sdc_voltage_set(INT32U device_id, INT8U voltage)
{
    sdc_voltage[device_id] = voltage;
}

INT8U sdc_voltage_get(INT32U device_id)
{
    INT8U voltage = sdc_voltage[device_id];

    DBG_PRINT("sd-%u use %s\r\n", device_id, (voltage ? "1.8V" : "3.3V"));

    return voltage;
}

#if SDC0_ENABLE
INT8S sdc0_pinmux_set(INT8U idx)
{
	INT32U funpos0 = R_FUNPOS0;

    if (idx > 7) {
        return MUX_ERR;
    }

    if (idx== SDC0_MUX0) {          //SDC0_CMD_CLK_DATA0_3__IOA2_IOA3_IOA5_IOA6_IOA7_IOA4
        funpos0 = (funpos0 & (~(0xD << 19))); // bit 22, 21, 19(MSB)

        INT32U SAVE_IOA_ATT, SAVE_IOA_DIR, SAVE_IOA_VALUE;
        SAVE_IOA_ATT = R_IOA_ATT;
        SAVE_IOA_DIR = R_IOA_DIR;
        SAVE_IOA_VALUE = R_IOA_O_DATA;

        // cmd, data0~3 set to floating mode, clk set to out low
        R_IOA_DIR = (SAVE_IOA_DIR & (~0x00FC)); // make input mode to avoid impact other
        R_IOA_ATT = (SAVE_IOA_ATT & (~0x00FC)) | 0x00FC; // make floating first
        R_IOA_O_DATA = (SAVE_IOA_VALUE & (~0x00FC)) | 0x00F4;
        R_IOA_DIR |= 0x0008; // clock output low
        R_IOA_ATT &= ~0x00F4; // actually pull it cmd, data0~3 to high

        gpio_drving_init_io(IO_A2, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_A3, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        sdc_voltage_set(1, 0); // use 3.3V
	} else if (idx == SDC0_MUX1) {  //SDC0_CMD_CLK_DATA0_3__IOB14_IOB15_IOB11_IOB12_IOB13_IOB10
        funpos0 = (funpos0 & (~(0xD << 19))) | (1 << 21); // bit 22, 21, 19(MSB)

        INT32U SAVE_IOB_ATT, SAVE_IOB_DIR, SAVE_IOB_VALUE;
        SAVE_IOB_ATT = R_IOB_ATT;
        SAVE_IOB_DIR = R_IOB_DIR;
        SAVE_IOB_VALUE = R_IOB_O_DATA;

        // cmd, data0~3 set to floating mode, clk set to out low
        R_IOB_DIR = (SAVE_IOB_DIR & (~0xFC00)); // make input mode to avoid impact other
        R_IOB_ATT = (SAVE_IOB_ATT & (~0xFC00)) | 0xFC00; // make floating first
        R_IOB_O_DATA = (SAVE_IOB_VALUE & (~0xFC00)) | 0x7C00;
        R_IOB_DIR |= 0x8000; // clock output low
        R_IOB_ATT &= ~0x7C00; // actually pull it cmd, data0~3 to high

        gpio_drving_init_io(IO_B14, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_B15, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        sdc_voltage_set(1, 0); // use 3.3V
    } else if (idx == SDC0_MUX2) {  //SDC0_CMD_CLK_DATA0_3__IOC6_IOC7_IOC9_IOC10_IOC11_IOC8
        funpos0 = (funpos0 & (~(0xD << 19))) | (2 << 21); // bit 22, 21, 19(MSB)

        INT32U SAVE_IOC_ATT, SAVE_IOC_DIR, SAVE_IOC_VALUE;
        SAVE_IOC_ATT = R_IOC_ATT;
        SAVE_IOC_DIR = R_IOC_DIR;
        SAVE_IOC_VALUE = R_IOC_O_DATA;

        // cmd, data0~3 set to floating mode, clk set to out low
        R_IOC_DIR = (SAVE_IOC_DIR & (~0x0FC0)); // make input mode to avoid impact other
        R_IOC_ATT = (SAVE_IOC_ATT & (~0x0FC0)) | 0x0FC0; // make floating first
        R_IOC_O_DATA = (SAVE_IOC_VALUE & (~0x0FC0)) | 0x0F40;
        R_IOC_DIR |= 0x0080; // clock output low
        R_IOC_ATT &= ~0x0F40; // actually pull it cmd, data0~3 to high

        gpio_drving_init_io(IO_C6, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_C7, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        sdc_voltage_set(1, 0); // use 3.3V
    } else if (idx == SDC0_MUX3) {  //SDC0_CMD_CLK_DATA0_3__IOE0_IOE1_IOE3_IOE4_IOE5_IOE2
        funpos0 = (funpos0 & (~(0xD << 19))) | (3 << 21); // bit 22, 21, 19(MSB)

        INT32U SAVE_IOE_ATT, SAVE_IOE_DIR, SAVE_IOE_VALUE;
        SAVE_IOE_ATT = R_IOE_ATT;
        SAVE_IOE_DIR = R_IOE_DIR;
        SAVE_IOE_VALUE = R_IOE_O_DATA;

        // cmd, data0~3 set to floating mode, clk set to out low
        R_IOE_DIR = (SAVE_IOE_DIR & (~0x003F)); // make input mode to avoid impact other
        R_IOE_ATT = (SAVE_IOE_ATT & (~0x003F));
        R_IOE_O_DATA = (SAVE_IOE_VALUE & (~0x003F)) | 0x003D;
        R_IOE_DIR |= 0x0002; // clock output low
        R_IOE_ATT &= ~0x003D; // actually pull it cmd, data0~3 to high

        gpio_drving_init_io(IO_E0, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_E1, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        sdc_voltage_set(1, 0); // use 3.3V
    } else if (idx == SDC0_MUX4) {  //SDC1_CMD_CLK_DATA0_3__IOF0_IOF1_IOF3_IOF4_IOF5_IOF2
        funpos0 = (funpos0 & (~(0xD << 19))) | (1<<19); // bit 22, 21, 19(MSB)

        INT32U SAVE_IOF_ATT, SAVE_IOF_DIR, SAVE_IOF_VALUE;
        SAVE_IOF_ATT = R_IOF_ATT;
        SAVE_IOF_DIR = R_IOF_DIR;
        SAVE_IOF_VALUE = R_IOF_O_DATA;

        // cmd, data0~3 set to floating mode, clk set to out low
        R_IOF_DIR = (SAVE_IOF_DIR & (~0x003F)); // make input mode to avoid impact other
        R_IOF_ATT = (SAVE_IOF_ATT & (~0x003F)) | 0x003F; // make floating first
        R_IOF_O_DATA = (SAVE_IOF_VALUE & (~0x003F)) | 0x003D;
        R_IOF_DIR |= 0x0002; // clock output low
        R_IOF_ATT &= ~0x003D; // actually pull it cmd, data0~3 to high // WILLIAM-VER2, bug fixed, pull high

        gpio_drving_init_io(IO_F0, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_F1, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        sdc_voltage_set(1, 0); // use 3.3V
    } else if (idx == SDC0_MUX5) {  //SDC1_CMD_CLK_DATA0_3__IOB0_IOB1_IOB3_IOB4_IOB5_IOB2
        funpos0 = (funpos0 & (~(0xD << 19))) | (1 << 21) | (1<<19); // bit 22, 21, 19(MSB)

        INT32U SAVE_IOB_ATT, SAVE_IOB_DIR, SAVE_IOB_VALUE;
        SAVE_IOB_ATT = R_IOB_ATT;
        SAVE_IOB_DIR = R_IOB_DIR;
        SAVE_IOB_VALUE = R_IOB_O_DATA;

        // cmd, data0~3 set to floating mode, clk set to out low
        R_IOB_DIR = (SAVE_IOB_DIR & (~0x003F)); // make input mode to avoid impact other
        R_IOB_ATT = (SAVE_IOB_ATT & (~0x003F)) | 0x003F; // make floating first
        R_IOB_O_DATA = (SAVE_IOB_VALUE & (~0x003F)) | 0x003D;
        R_IOB_DIR |= 0x0002; // clock output low
        R_IOB_ATT &= ~0x003D; // actually pull it cmd, data0~3 to high

        gpio_drving_init_io(IO_B0, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_B1, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        sdc_voltage_set(1, 0); // use 3.3V
    } else if (idx == SDC0_MUX6) {  //SDC1_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOC14_IOC15_IOC13

        #if (SPI1_POS == SPI1_MUX1)
        // pinmux with SPI1_1 on IOD[10, 11, 13], so disable it to avoid conflict
        R_SPI1_CTRL &= (~(1<<15));
        #endif

        //DBG_PRINT("R_GPIO_CTRL=0x%08x\r\n", R_GPIO_CTRL);
        R_GPIO_CTRL &= ~0x1; // SD1#2 pins conflict with JTAG, so off the JTAG.
                            // JTAG be on no matter what boot mode and no matter JTAG ioTrap on.

        funpos0 = (funpos0 & (~(0xD << 19))) | (2 << 21) | (1<<19); // bit 22, 21, 19(MSB)

        INT32U SAVE_IOD_ATT, SAVE_IOD_DIR, SAVE_IOD_VALUE;
        INT32U SAVE_IOC_ATT, SAVE_IOC_DIR, SAVE_IOC_VALUE;
        SAVE_IOD_ATT = R_IOD_ATT;
        SAVE_IOD_DIR = R_IOD_DIR;
        SAVE_IOD_VALUE = R_IOD_O_DATA;

        SAVE_IOC_ATT = R_IOC_ATT;
        SAVE_IOC_DIR = R_IOC_DIR;
        SAVE_IOC_VALUE = R_IOC_O_DATA;

        // cmd, data0~3 set to floating mode, clk set to out low
        R_IOD_DIR = (SAVE_IOD_DIR & (~0x2C00)); // make input mode to avoid impact other
        R_IOD_ATT = (SAVE_IOD_ATT & (~0x2C00)) | 0x2C00; // make floating first
        R_IOD_O_DATA = (SAVE_IOD_VALUE & (~0x2C00)) | 0x2400;
        R_IOD_DIR |= 0x0800; // clock output low
        R_IOD_ATT &= ~0x2C00; // actually pull it cmd, data0~3 to high

        R_IOC_DIR = (SAVE_IOC_DIR & (~0xE000)); // make input mode to avoid impact other
        R_IOC_ATT = (SAVE_IOC_ATT & (~0xE000)) | 0xE000; // make floating first
        R_IOC_O_DATA = (SAVE_IOC_VALUE & (~0xE000)) | 0xE000;
        R_IOC_ATT &= ~0xE000; // actually pull it cmd, data0~3 to high

        gpio_drving_init_io(IO_D10, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_D11, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        sdc_voltage_set(1, 0); // use 3.3V
    } else if (idx == SDC0_MUX7) {  //SDC1_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOD14_IOD15_IOD12

        #if (SPI1_POS == SPI1_MUX1)
        // pinmux with SPI1_1 on IOD[10, 11, 13], so disable it to avoid conflict
        R_SPI1_CTRL &= (~(1<<15));
        #endif

        funpos0 = (funpos0 & (~(0xD << 19))) | (3 << 21) | (1<<19); // bit 22, 21, 19(MSB)

        INT32U SAVE_IOD_ATT, SAVE_IOD_DIR, SAVE_IOD_VALUE;
        SAVE_IOD_ATT = R_IOD_ATT;
        SAVE_IOD_DIR = R_IOD_DIR;
        SAVE_IOD_VALUE = R_IOD_O_DATA;

        // cmd, data0~3 set to floating mode, clk set to out low
        R_IOD_DIR = (SAVE_IOD_DIR & (~0xFC00)); // make input mode to avoid impact other
        R_IOD_ATT = (SAVE_IOD_ATT & (~0xFC00)) | 0xFC00; // make floating first
        R_IOD_O_DATA = (SAVE_IOD_VALUE & (~0xFC00)) | 0xF400;
        R_IOD_DIR |= 0x0800; // clock output low
        R_IOD_ATT &= ~0xF400; // actually pull it cmd, data0~3 to high

        gpio_drving_init_io(IO_D10, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_D11, (IO_DRV_LEVEL)SDC0_CMD_CLK_DRIVING);
        sdc_voltage_set(1, 0); // use 3.3V
	}

	R_FUNPOS0 = funpos0;
	return MUX_OK;
}
#endif

#if SDC1_ENABLE
INT8S sdc1_pinmux_set(INT8U idx)
{
    INT32U funpos0 = R_FUNPOS0;

    if (idx > 4) {
        return MUX_ERR;
    }

	funpos0 &= ~((3<<23)|(1<<20));
    if (idx == SDC1_MUX0) {         //SDC1_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOD14_IOD15_IOD12

        #if (SPI1_POS == SPI1_MUX1)
        // pinmux with SPI1_1 on IOD[10, 11, 13], so disable it to avoid conflict
        R_SPI1_CTRL &= (~(1<<15));
        #endif

        funpos0 &= ~((3<<23)|(1<<20));

        INT32U SAVE_IOD_ATT, SAVE_IOD_DIR, SAVE_IOD_VALUE;

        SAVE_IOD_ATT = R_IOD_ATT;
        SAVE_IOD_DIR = R_IOD_DIR;
        SAVE_IOD_VALUE = R_IOD_O_DATA;

        // cmd, data0~3 set to floating mode, clk set to out low
        R_IOD_DIR = (SAVE_IOD_DIR & (~0xFC00)); // make input mode to avoid impact other
        R_IOD_ATT = (SAVE_IOD_ATT & (~0xFC00)) | 0xFC00; // make floating first
        R_IOD_O_DATA = (SAVE_IOD_VALUE & (~0xFC00)) | 0xF400;
        R_IOD_DIR |= 0x0800; // clock output low
        R_IOD_ATT &= ~0xF400; // actually pull it cmd, data0~3 to high

        gpio_drving_init_io(IO_D10, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_D11, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING);
        sdc_voltage_set(1, 0); // use 3.3V
	} else if (idx == SDC1_MUX1) {  //SDC1_CMD_CLK_DATA0_3__IOB0_IOB1_IOB3_IOB4_IOB5_IOB2
        funpos0 |= (1<<23);

        INT32U SAVE_IOB_ATT, SAVE_IOB_DIR, SAVE_IOB_VALUE;

        SAVE_IOB_ATT = R_IOB_ATT;
        SAVE_IOB_DIR = R_IOB_DIR;
        SAVE_IOB_VALUE = R_IOB_O_DATA;

        // cmd, data0~3 set to floating mode, clk set to out low
        R_IOB_DIR = (SAVE_IOB_DIR & (~0x003F)); // make input mode to avoid impact other
        R_IOB_ATT = (SAVE_IOB_ATT & (~0x003F)) | 0x003F; // make floating first
        R_IOB_O_DATA = (SAVE_IOB_VALUE & (~0x003F)) | 0x003D;
        R_IOB_DIR |= 0x0002; // clock output low
        R_IOB_ATT &= ~0x003D; // actually pull it cmd, data0~3 to high

        gpio_drving_init_io(IO_B0, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_B1, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING);
        sdc_voltage_set(1, 0); // use 3.3V
    } else if (idx == SDC1_MUX2) {  //SDC1_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOC14_IOC15_IOC13

        #if (SPI1_POS == SPI1_MUX1)
        // pinmux with SPI1_1 on IOD[10, 11, 13], so disable it to avoid conflict
        R_SPI1_CTRL &= (~(1<<15));
        #endif

        //DBG_PRINT("R_GPIO_CTRL=0x%08x\r\n", R_GPIO_CTRL);
        R_GPIO_CTRL &= ~0x1; // SD1#2 pins conflict with JTAG, so off the JTAG.
                            // JTAG be on no matter what boot mode and no matter JTAG ioTrap on.
        funpos0 |= (2<<23);

        INT32U SAVE_IOD_ATT, SAVE_IOD_DIR, SAVE_IOD_VALUE;
        INT32U SAVE_IOC_ATT, SAVE_IOC_DIR, SAVE_IOC_VALUE;

        SAVE_IOD_ATT = R_IOD_ATT;
        SAVE_IOD_DIR = R_IOD_DIR;
        SAVE_IOD_VALUE = R_IOD_O_DATA;

        SAVE_IOC_ATT = R_IOC_ATT;
        SAVE_IOC_DIR = R_IOC_DIR;
        SAVE_IOC_VALUE = R_IOC_O_DATA;

        // cmd, data0~3 set to floating mode, clk set to out low
        R_IOD_DIR = (SAVE_IOD_DIR & (~0x2C00)); // make input mode to avoid impact other
        R_IOD_ATT = (SAVE_IOD_ATT & (~0x2C00)) | 0x2C00; // make floating first
        R_IOD_O_DATA = (SAVE_IOD_VALUE & (~0x2C00)) | 0x2400;
        R_IOD_DIR |= 0x0800; // clock output low
        R_IOD_ATT &= ~0x2C00; // actually pull it cmd, data0~3 to high

        R_IOC_DIR = (SAVE_IOC_DIR & (~0xE000)); // make input mode to avoid impact other
        R_IOC_ATT = (SAVE_IOC_ATT & (~0xE000)) | 0xE000; // make floating first
        R_IOC_O_DATA = (SAVE_IOC_VALUE & (~0xE000)) | 0xE000;
        R_IOC_ATT &= ~0xE000; // actually pull it cmd, data0~3 to high

        gpio_drving_init_io(IO_D10, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_D11, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING);
        sdc_voltage_set(1, 0); // use 3.3V
	} else if (idx == SDC1_MUX3) {  //SDC1_CMD_CLK_DATA0_3__IOF0_IOF1_IOF3_IOF4_IOF5_IOF2
        INT32U SAVE_FUNPOS0;
        SAVE_FUNPOS0 = R_FUNPOS0;
        funpos0 = (SAVE_FUNPOS0 & (~(3 << 23))) | (3 << 23);

        INT32U SAVE_IOF_ATT, SAVE_IOF_DIR, SAVE_IOF_VALUE;
        SAVE_IOF_ATT = R_IOF_ATT;
        SAVE_IOF_DIR = R_IOF_DIR;
        SAVE_IOF_VALUE = R_IOF_O_DATA;

        // cmd, data0~3 set to floating mode, clk set to out low
        R_IOF_DIR = (SAVE_IOF_DIR & (~0x003F)); // make input mode to avoid impact other
        R_IOF_ATT = (SAVE_IOF_ATT & (~0x003F)) | 0x003F; // make floating first
        R_IOF_O_DATA = (SAVE_IOF_VALUE & (~0x003F)) | 0x003D;
        R_IOF_DIR |= 0x0002; // clock output low
        R_IOF_ATT &= ~0x003D; // actually pull it cmd, data0~3 to high // WILLIAM-VER2, bug fixed, pull high

        gpio_drving_init_io(IO_F0, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_F1, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING);
        sdc_voltage_set(1, 0); // use 3.3V
    } else if (idx == SDC1_MUX4) {  //SDC1_CMD_CLK_DATA0_3__IOB14_IOB15_IOB11_IOB12_IOB13_IOB10
        funpos0 |= (1<<20);

        INT32U SAVE_IOB_ATT, SAVE_IOB_DIR, SAVE_IOB_VALUE;
        SAVE_IOB_ATT = R_IOB_ATT;
        SAVE_IOB_DIR = R_IOB_DIR;
        SAVE_IOB_VALUE = R_IOB_O_DATA;

        // cmd, data0~3 set to floating mode, clk set to out low
        R_IOB_DIR = (SAVE_IOB_DIR & (~0xFC00)); // make input mode to avoid impact other
        R_IOB_ATT = (SAVE_IOB_ATT & (~0xFC00)) | 0xFC00; // make floating first
        R_IOB_O_DATA = (SAVE_IOB_VALUE & (~0xFC00)) | 0x7C00;
        R_IOB_DIR |= 0x8000; // clock output low
        R_IOB_ATT &= ~0x7C00; // actually pull it cmd, data0~3 to high

        gpio_drving_init_io(IO_B14, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING); // sd cmd pin driving
        gpio_drving_init_io(IO_B15, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING); // sd clock pin driving

        #if 0 // if want enhance sd data driving, turn on below and set SDC1_CMD_CLK_DRIVING to IO_DRIVING_lvl1
        gpio_drving_init_io(IO_B11, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_B12, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_B13, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING);
        gpio_drving_init_io(IO_B10, (IO_DRV_LEVEL)SDC1_CMD_CLK_DRIVING);
        #endif

        sdc_voltage_set(1, 1); // use 1.8V
    }
	R_FUNPOS0 = funpos0;
	return MUX_OK;
}
#endif

#if TFT1_SPI_ENABLE
INT8S tft1_spi_pinmux_set(INT8U ctrl_mux, INT8U te_mux)
{
	INT32U funpos2 = R_FUNPOS2;

    if ((ctrl_mux > MUX2) || (te_mux > MUX1))
        return MUX_ERR;

    // TFT1 SPI pin init
    funpos2 &= ~(3<<9);
    // TFT1 SPI TE
    funpos2 &= ~(1<<13);

    // TFT1 SPI 0~3
    if (ctrl_mux == TFT1_SPI_DATA0_3_MUX0)          //TFT1_SPI_DATA_0_3__IOB0_IOB3
    {
        gpio_drving_init_io(IO_B0,(IO_DRV_LEVEL)TFT1_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_B1,(IO_DRV_LEVEL)TFT1_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_B2,(IO_DRV_LEVEL)TFT1_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_B3,(IO_DRV_LEVEL)TFT1_SPI_DATA0_3_DIRVING);
    }
    else if(ctrl_mux == TFT1_SPI_DATA0_3_MUX1)   //TFT1_SPI_DATA_0_3__IOC8_IOC11
    {
        funpos2 |= (0x1<<9);
        gpio_drving_init_io(IO_C8,(IO_DRV_LEVEL)TFT1_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_C9,(IO_DRV_LEVEL)TFT1_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_C10,(IO_DRV_LEVEL)TFT1_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_C11,(IO_DRV_LEVEL)TFT1_SPI_DATA0_3_DIRVING);
    }
    else if(ctrl_mux == TFT1_SPI_DATA0_3_MUX2)  //TFT1_SPI_DATA_0_3__IOF4_IOF7
    {
        funpos2 |= (0x2<<9);
        gpio_drving_init_io(IO_F6,(IO_DRV_LEVEL)TFT1_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_F7,(IO_DRV_LEVEL)TFT1_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_F8,(IO_DRV_LEVEL)TFT1_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_F9,(IO_DRV_LEVEL)TFT1_SPI_DATA0_3_DIRVING);
    }
    else
        funpos2 |= (0x3<<9);    // function disable

    // TFT1 SPI TE
    if (te_mux == TFT1_SPI_TE_MUX0)                  //TFT1_SPI_TE_IOB4
    {
        gpio_drving_init_io(IO_B4,(IO_DRV_LEVEL)TFT1_SPI_TE_DIRVING);
    }
    else if (te_mux == TFT1_SPI_TE_MUX1)      //TFT1_SPI_TE_IOF2
    {
        funpos2 |= (0x1<<13);
        gpio_drving_init_io(IO_F2,(IO_DRV_LEVEL)TFT1_SPI_TE_DIRVING);
    }

	R_FUNPOS2 = funpos2;

	return MUX_OK;
}
#endif

#if TFT2_SPI_ENABLE
INT8S tft2_spi_pinmux_set(INT8U ctrl_mux, INT8U te_mux)
{
	INT32U funpos2 = R_FUNPOS2;

    if ((ctrl_mux > MUX2) || (te_mux > MUX1))
        return MUX_ERR;

    // TFT2 SPI pin init
    funpos2 &= ~(3<<11);
    // TFT2 SPI TE
    funpos2 &= ~(1<<14);

    // TFT2 SPI 0~3
    if (ctrl_mux == TFT2_SPI_DATA0_3_MUX0)          //TFT2_SPI_DATA_0_3__IOB0_IOB3
    {
        gpio_drving_init_io(IO_B0,(IO_DRV_LEVEL)TFT2_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_B1,(IO_DRV_LEVEL)TFT2_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_B2,(IO_DRV_LEVEL)TFT2_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_B3,(IO_DRV_LEVEL)TFT2_SPI_DATA0_3_DIRVING);
    }
    else if(ctrl_mux == TFT2_SPI_DATA0_3_MUX1)   //TFT2_SPI_DATA_0_3__IOC8_IOC11
    {
        funpos2 |= (0x1<<11);
        gpio_drving_init_io(IO_C8,(IO_DRV_LEVEL)TFT2_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_C9,(IO_DRV_LEVEL)TFT2_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_C10,(IO_DRV_LEVEL)TFT2_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_C11,(IO_DRV_LEVEL)TFT2_SPI_DATA0_3_DIRVING);
    }
    else if(ctrl_mux == TFT2_SPI_DATA0_3_MUX2)  //TFT2_SPI_DATA_0_3__IOF4_IOF7
    {
        funpos2 |= (0x2<<11);
        gpio_drving_init_io(IO_F4,(IO_DRV_LEVEL)TFT2_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_F5,(IO_DRV_LEVEL)TFT2_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_F6,(IO_DRV_LEVEL)TFT2_SPI_DATA0_3_DIRVING);
        gpio_drving_init_io(IO_F7,(IO_DRV_LEVEL)TFT2_SPI_DATA0_3_DIRVING);
    }
    else
        funpos2 |= (0x3<<11);   // function disable

    // TFT2 SPI TE
    if (te_mux == TFT2_SPI_TE_MUX0)                  //TFT2_SPI_TE_IOB4
    {
        gpio_drving_init_io(IO_B4,(IO_DRV_LEVEL)TFT2_SPI_TE_DIRVING);
    }
    else if (te_mux == TFT2_SPI_TE_MUX1)        //TFT_SPI1_TE_IOF2
    {
        funpos2 |= (0x1<<14);
        gpio_drving_init_io(IO_F2,(IO_DRV_LEVEL)TFT2_SPI_TE_DIRVING);
    }

	R_FUNPOS2 = funpos2;

	return MUX_OK;
}
#endif

#if TFT1_ENABLE
INT8S tft_pinmux_set(INT8U ctrl_mux, INT8U ldata_mux, INT8U hdata_mux, INT8U hdata_mux_2, INT8U te_mux)
{
	INT32U funpos0 = R_TFTIOCTRL;

    if ((ctrl_mux > MUX2) || (ldata_mux > MUX2) || (hdata_mux > MUX3) || (hdata_mux_2 > MUX1) || (te_mux > MUX1))
        return MUX_ERR;

    // TFT pin init
    funpos0 &= ~(0xFFF);

    // TFT DATA0~7
    if (ldata_mux == TFT_DATA0_7_MUX0)          //TFT_DATA_0_7__IOA0_IOA7
    {
        funpos0 |= (1<<5);
        gpio_drving_init_io(IO_A0,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A1,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A2,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A3,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A4,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A5,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A6,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A7,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
    }
    else                                        //TFT_DATA_0_7__IOE0_IOE7
    {
        //funpos0 &= ~(1<<5);
        gpio_drving_init_io(IO_E0,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E1,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E2,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E3,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E4,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E5,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E6,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E7,(IO_DRV_LEVEL)TFT_DATA0_7_DIRVING);
    }
    funpos0 |= (0x1);                           // TFT_DATA0_EN
    funpos0 |= (1<<11);                         // TFT_DATA1_EN

    // TFT DATA8~15
    if (hdata_mux == TFT_DATA8_15_MUX0)         //TFT_DATA_8_15__IOA8_IOA15
    {
        funpos0 |= (1<<6);
        //funpos0 &= ~(1<<8);
        gpio_drving_init_io(IO_A8,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A9,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A10,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A11,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A12,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A13,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A14,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A15,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
    }
    else if (hdata_mux == TFT_DATA8_15_MUX1)    //TFT_DATA_8_15__IOE0_E7
    {
        //funpos0 &= ~(1<<6);                     //Depend on TFT_DATA15_8_POS_1
        //funpos0 &= ~(1<<8);
        gpio_drving_init_io(IO_E0,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E1,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E2,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E3,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E4,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E5,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E6,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E7,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
    }
    else if (hdata_mux == TFT_DATA8_15_MUX2)    //TFT_DATA_8_15__IOC0_IOC5_IOD5_IOD7
    {
        //funpos0 &= ~(1<<6);                     //Depend on TFT_DATA15_8_POS_1
        funpos0 |= (1<<8);
        gpio_drving_init_io(IO_D7,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_D5,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_C5,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_C4,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_C3,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_C2,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_C1,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_C0,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
    }
    else if (hdata_mux == TFT_DATA8_15_MUX3)    //TFT_DATA_8_15__IOB14_IOB15_IOA10_IOA15
    {
        funpos0 |= (1<<6);                    //Depend on TFT_DATA15_8_POS_1
        funpos0 |= (1<<8);
        gpio_drving_init_io(IO_B14,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_B15,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A10,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A11,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A12,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A13,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A14,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A15,(IO_DRV_LEVEL)TFT_DATA8_15_DIRVING);
    }

    // TFT DATA16~23
    if (hdata_mux_2 == TFT_DATA16_23_MUX0)      //TFT_DATA_16_23__IOF0_IOF7
    {
        //funpos0 &= ~(1<<7);
        gpio_drving_init_io(IO_F0,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F1,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F2,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F3,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F4,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F5,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F6,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F7,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
    }
    else if(hdata_mux_2 == TFT_DATA16_23_MUX1)   //TFT_DATA_16_23__IOB6_13
    {
        funpos0 |= (1<<7);
        gpio_drving_init_io(IO_B6,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B7,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B8,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B9,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B10,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B11,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B12,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B13,(IO_DRV_LEVEL)TFT_DATA16_23_DIRVING);
    }

    //  TFT CTRL
    if (ctrl_mux == TFT_CTRL_MUX0){             //TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOB0_IOB1_IOB2_IOB3
        //funpos0 &= ~(3<<3);
        gpio_drving_init_io(IO_B0,(IO_DRV_LEVEL)TFT_CTRL_DRIVING);
        gpio_drving_init_io(IO_B1,(IO_DRV_LEVEL)TFT_CTRL_DRIVING);
        gpio_drving_init_io(IO_B2,(IO_DRV_LEVEL)TFT_CTRL_DRIVING);
        gpio_drving_init_io(IO_B3,(IO_DRV_LEVEL)TFT_CLK_DRIVING);
    }
    else if (ctrl_mux == TFT_CTRL_MUX1){       //TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOC8_IOC9_IOC10_IOC11
        //funpos0 &= ~(3<<3);
        funpos0 |= (1<<3);
        gpio_drving_init_io(IO_C8,(IO_DRV_LEVEL)TFT_CTRL_DRIVING);
        gpio_drving_init_io(IO_C9,(IO_DRV_LEVEL)TFT_CTRL_DRIVING);
        gpio_drving_init_io(IO_C10,(IO_DRV_LEVEL)TFT_CTRL_DRIVING);
        gpio_drving_init_io(IO_C11,(IO_DRV_LEVEL)TFT_CLK_DRIVING);
    }
    else  if (ctrl_mux == TFT_CTRL_MUX2)       //TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOF4_IOF5_IOF6_IOF7
    {
        funpos0 |= (2<<3);
        gpio_drving_init_io(IO_F4,(IO_DRV_LEVEL)TFT_CTRL_DRIVING);
        gpio_drving_init_io(IO_F5,(IO_DRV_LEVEL)TFT_CTRL_DRIVING);
        gpio_drving_init_io(IO_F6,(IO_DRV_LEVEL)TFT_CTRL_DRIVING);
        gpio_drving_init_io(IO_F7,(IO_DRV_LEVEL)TFT_CLK_DRIVING);
    }

    // TFT TE
    if (te_mux == TFT_TE_MUX0)                 //TFT_TE_IOF2
    {
        //funpos0 &= ~(1<<2);
        gpio_drving_init_io(IO_F2,(IO_DRV_LEVEL)TFT_TE_DIRVING);
    }
    else if (te_mux == TFT_TE_MUX1)            //TFT_TE_IOB4
    {
        funpos0 |= (1<<2);
        gpio_drving_init_io(IO_B4,(IO_DRV_LEVEL)TFT_TE_DIRVING);
    }

	R_TFTIOCTRL = funpos0;

	return MUX_OK;
}
#endif

#if TFT2_ENABLE
INT8S tft2_pinmux_set(INT8U ctrl_mux, INT8U ldata_mux, INT8U hdata_mux, INT8U hdata_mux_2, INT8U te_mux)
{
	INT32U funpos0 = R_TFTIOCTRL;

    if ((ctrl_mux > MUX2) || (ldata_mux > MUX2) || (hdata_mux > MUX2) || (hdata_mux_2 > MUX1) || (te_mux > MUX1))
        return MUX_ERR;

    // TFT2 pin init
    funpos0 &= ~(0xFFF0000);

    // TFT2 DATA0~7
    if (ldata_mux == TFT2_DATA0_7_MUX0)          //TFT2_DATA_0_7__IOA0_IOA7
    {
        funpos0 |= (1<<21);
        gpio_drving_init_io(IO_A0,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A1,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A2,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A3,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A4,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A5,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A6,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_A7,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
    }
    else                                        //TFT2_DATA_0_7__IOE0_IOE7
    {
        //funpos0 &= ~(1<<21);
        gpio_drving_init_io(IO_E0,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E1,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E2,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E3,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E4,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E5,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E6,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
        gpio_drving_init_io(IO_E7,(IO_DRV_LEVEL)TFT2_DATA0_7_DIRVING);
    }
    funpos0 |= (1<<16);                           // TFT2_DATA0_EN
    funpos0 |= (1<<27);                           // TFT2_DATA1_EN

    // TFT2 DATA8~15
    if (hdata_mux == TFT2_DATA8_15_MUX0)         //TFT2_DATA_8_15__IOA8_IOA15
    {
        funpos0 |= (1<<22);
        //funpos0 &= ~(1<<24);
        gpio_drving_init_io(IO_A8,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A9,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A10,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A11,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A12,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A13,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A14,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A15,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
    }
    else if (hdata_mux == TFT2_DATA8_15_MUX1)    //TFT2_DATA_8_15__IOE0_E7
    {
        //funpos0 &= ~(1<<22);                     //Depend on TFT2_DATA15_8_POS_1
        //funpos0 &= ~(1<<24);
        gpio_drving_init_io(IO_E0,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E1,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E2,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E3,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E4,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E5,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E6,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_E7,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
    }
    else  if (hdata_mux == TFT2_DATA8_15_MUX2)     //TFT2_DATA_8_15__IOC0_IOC5_IOD5_IOD7
    {
        //funpos0 &= ~(1<<22);                     //Depend on TFT2_DATA15_8_POS_1
        funpos0 |= (1<<24);
        gpio_drving_init_io(IO_D7,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_D5,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_C5,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_C4,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_C3,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_C2,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_C1,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_C0,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
    }
    else if (hdata_mux == TFT2_DATA8_15_MUX3)    //TFT2_DATA_8_15__IOB14_IOB15_IOA10_IOA15
    {
        funpos0 |= (1<<22);                    //Depend on TFT_DATA15_8_POS_1
        funpos0 |= (1<<24);
        gpio_drving_init_io(IO_B14,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_B15,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A10,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A11,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A12,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A13,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A14,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
        gpio_drving_init_io(IO_A15,(IO_DRV_LEVEL)TFT2_DATA8_15_DIRVING);
    }

    // TFT2 DATA16~23
    if (hdata_mux_2 == TFT2_DATA16_23_MUX0)      //TFT2_DATA_16_23__IOF0_IOF7
    {
        //funpos0 &= ~(1<<23);
        gpio_drving_init_io(IO_F0,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F1,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F2,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F3,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F4,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F5,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F6,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_F7,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
    }
    else if(hdata_mux_2 == TFT2_DATA16_23_MUX1)     //TFT2_DATA_16_23__IOB6_13
    {
        funpos0 |= (1<<23);
        gpio_drving_init_io(IO_B6,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B7,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B8,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B9,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B10,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B11,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B12,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
        gpio_drving_init_io(IO_B13,(IO_DRV_LEVEL)TFT2_DATA16_23_DIRVING);
    }

    //  TFT2 CTRL
    if (ctrl_mux == TFT2_CTRL_MUX0){             //TFT2_CTRL_DE_HSYNC_VSYNC_CLK__IOB0_IOB1_IOB2_IOB3
        //funpos0 &= ~(3<<19);
        gpio_drving_init_io(IO_B0,(IO_DRV_LEVEL)TFT2_CTRL_DRIVING);
        gpio_drving_init_io(IO_B1,(IO_DRV_LEVEL)TFT2_CTRL_DRIVING);
        gpio_drving_init_io(IO_B2,(IO_DRV_LEVEL)TFT2_CTRL_DRIVING);
        gpio_drving_init_io(IO_B3,(IO_DRV_LEVEL)TFT2_CLK_DRIVING);
    }
    else if (ctrl_mux == TFT2_CTRL_MUX1){       //TFT2_CTRL_DE_HSYNC_VSYNC_CLK__IOC8_IOC9_IOC10_IOC11
        //funpos0 &= ~(3<<19);
        funpos0 |= (1<<19);
        gpio_drving_init_io(IO_C8,(IO_DRV_LEVEL)TFT2_CTRL_DRIVING);
        gpio_drving_init_io(IO_C9,(IO_DRV_LEVEL)TFT2_CTRL_DRIVING);
        gpio_drving_init_io(IO_C10,(IO_DRV_LEVEL)TFT2_CTRL_DRIVING);
        gpio_drving_init_io(IO_C11,(IO_DRV_LEVEL)TFT2_CLK_DRIVING);
    }
    else if (ctrl_mux == TFT2_CTRL_MUX2)    //TFT2_CTRL_DE_HSYNC_VSYNC_CLK__IOF4_IOF5_IOF6_IOF7
    {
        //funpos0 &= ~(3<<19);
        funpos0 |= (2<<19);
        gpio_drving_init_io(IO_F4,(IO_DRV_LEVEL)TFT2_CTRL_DRIVING);
        gpio_drving_init_io(IO_F5,(IO_DRV_LEVEL)TFT2_CTRL_DRIVING);
        gpio_drving_init_io(IO_F6,(IO_DRV_LEVEL)TFT2_CTRL_DRIVING);
        gpio_drving_init_io(IO_F7,(IO_DRV_LEVEL)TFT2_CLK_DRIVING);
    }

    // TFT2 TE
    if (te_mux == TFT2_TE_MUX0)                 //TFT2_TE_IOF2
    {
        //funpos0 &= ~(1<<18);
        gpio_drving_init_io(IO_F2,(IO_DRV_LEVEL)TFT2_TE_DIRVING);
    }
    else if (te_mux == TFT2_TE_MUX1)            //TFT2_TE_IOB4
    {
        funpos0 |= (1<<18);
        gpio_drving_init_io(IO_B4,(IO_DRV_LEVEL)TFT2_TE_DIRVING);
    }

	R_TFTIOCTRL = funpos0;

	return MUX_OK;
}
#endif

#if CSI_CLKO_ENABLE
INT8S csi_clko_pinmux_set(INT8U clko_mux,INT8U clko_source)
{
    INT32U funpos1 = R_FUNPOS1;

    //CLKO_POS0 at IO_D7
    if(clko_mux == CSI_CLKO_POS_MUX0)
    {
        funpos1 |=(0x1<<21);  //CLKO at IO_D7 enabled
        gpio_drving_init_io(IO_D7,(IO_DRV_LEVEL)CSI_CLKO_DRIVING);
        if(clko_source == CLKO_FROM_CSI)
            funpos1 |=(0x1<<24);
        else
            funpos1 &= ~(0x1<<24);
    }
    else //CLKO_POS1 at IO_D12
    if(clko_mux == CSI_CLKO_POS_MUX1)
    {
        funpos1 |=(0x1<<22);  //CLKO at IO_D12 enabled
        gpio_drving_init_io(IO_D12,(IO_DRV_LEVEL)CSI_CLKO_DRIVING);
        if(clko_source == CLKO_FROM_CSI)
            funpos1 |=(0x1<<25);
        else
            funpos1 &= ~(0x1<<25);
    }
    else //CLKO_POS2 at IO_C9
    if(clko_mux == CSI_CLKO_POS_MUX2)
    {
        funpos1 |=(0x1<<20);  //CLKO at IO_C9 enabled
        gpio_drving_init_io(IO_C9,(IO_DRV_LEVEL)CSI_CLKO_DRIVING);
        if(clko_source == CLKO_FROM_CSI)
            funpos1 |=(0x1<<23);
        else
            funpos1 &= ~(0x1<<23);
    }
    else //CLKO_POS3 at IO_A9
    if(clko_mux == CSI_CLKO_POS_MUX3)
    {
        funpos1 |=(0x1<<18);  //CLKO at IO_A9 enabled
        gpio_drving_init_io(IO_A9,(IO_DRV_LEVEL)CSI_CLKO_DRIVING);
        if(clko_source == CLKO_FROM_CSI)
            funpos1 |=(0x1<<19);
        else
            funpos1 &= ~(0x1<<19);
    }

    R_FUNPOS1 = funpos1;
    return MUX_OK;
}
#endif

#if CSI0_ENABLE
INT8S csi_pinmux_set(INT8U ctrl_mux, INT8U data_mux)
{
	INT32U funpos1 = R_FUNPOS1;

    if ((ctrl_mux > 2) || (data_mux > 3))
        return MUX_ERR;

	funpos1 &= ~((0x1<<2)|(0x3<<3));//CTRL [2], DATA [4:3]

    if (ctrl_mux == CSI_CTRL_MUX0)//CSI_CTRL_CLKI_HSYNC_VSYNC__IOC8_IOC10_IOC11
    {
        funpos1 &= ~(0x1<<2);
      /*  R_IOC_DIR &= ~0x0D00; // GPIO_IN_PULL_HIGH
        R_IOC_ATT &= ~0x0D00;
        R_IOC_O_DATA |= 0x0D00;*/
    }
    else if (ctrl_mux == CSI_CTRL_MUX1 )//CSI_CTRL_CLKI_HSYNC_VSYNC__IOD6_IOD8_IOD9
	{
        funpos1 |= (0x1<<2);
       /* R_IOD_DIR &= ~0x0340; //GPIO_IN_FLOAT
        R_IOD_ATT |= 0x0340;
        R_IOD_O_DATA |= 0x0340;*/
    }

    if (data_mux == CSI_DATA2_9_MUX0)//CSI_DATA_2_9__IOC0_IOC7
    {
        funpos1 &= ~(0x3<<3);
        R_IOC_DIR &= ~0x00FF; //GPIO_IN_FLOAT
        R_IOC_ATT |= 0x00FF;
        R_IOC_O_DATA |= 0x00FF;
    }
    else if (data_mux == CSI_DATA2_9_MUX1)//CSI_DATA_2_9__IOB8_IOB15
    {
        funpos1 |= (0x1<<3);
        R_IOB_DIR &= ~0xFF00; //GPIO_IN_FLOAT
        R_IOB_ATT |= 0xFF00;
        R_IOB_O_DATA |= 0xFF00;
    }
    else if (data_mux == CSI_DATA2_9_MUX2)//CSI_DATA_2_9__IOE0_IOE7
    {
        funpos1 |= (0x2<<3);
        R_IOE_DIR &= ~0x00FF;  //GPIO_IN_FLOAT
        R_IOE_ATT |= 0x00FF;
        R_IOE_O_DATA |= 0x00FF;
    }
    else if (data_mux == CSI_DATA2_9_MUX3)//CSI_DATA_2_9__IOF0_IOF7
    {
        funpos1 |= (0x3<<3);
        R_IOF_DIR &= ~0x00FF;  // GPIO_IN_PULL_HIGH
        R_IOF_ATT &= ~0x00FF;
        R_IOF_O_DATA |= 0x00FF;
    }
	R_FUNPOS1 = funpos1;
    return MUX_OK;
}
#endif

#if CSI2_ENABLE
INT8S csi2_pinmux_set(INT8U ctrl_mux, INT8U data_mux)
{
	INT32U funpos1 = R_FUNPOS1;

    if((ctrl_mux > 2) || (data_mux > 3))
        return MUX_ERR;

    funpos1 &= ~((0x3<<8)|(0x1<<7));//CTRL[7], DATA[9:8]

    if(ctrl_mux == CSI2_CTRL_MUX0){//CSI_CTRL_CLKI_HSYNC_VSYNC__IOC8_IOC10_IOC11
        funpos1 &= ~(0x1<<7);
    } else if (ctrl_mux == CSI2_CTRL_MUX1 ){//CSI_CTRL_CLKI_HSYNC_VSYNC__IOD6_IOD8_IOD9
        funpos1 |= (0x1<<7);
    }

    if (data_mux == CSI2_DATA2_9_MUX0)//CSI_DATA_2_9__IOC0_IOC7
    {
        funpos1 &= ~(0x3<<8);
        R_IOC_DIR &= ~0x00FF;
        R_IOC_ATT |= 0x00FF;
        R_IOC_O_DATA |= 0x00FF;
    }
    else if (data_mux == CSI2_DATA2_9_MUX1)//CSI_DATA_2_9__IOB8_IOB15
    {
        funpos1 |= (0x1<<8);
        R_IOB_DIR &= ~0xFF00;
        R_IOB_ATT |= 0xFF00;
        R_IOB_O_DATA |= 0xFF00;
    }
    else if (data_mux == CSI2_DATA2_9_MUX2)//CSI_DATA_2_9__IOE0_IOE7
    {
        funpos1 |= (0x2<<8);
        R_IOE_DIR &= ~0x00FF;
        R_IOE_ATT |= 0x00FF;
        R_IOE_O_DATA |= 0x00FF;
    }
    else if (data_mux == CSI2_DATA2_9_MUX3)//CSI_DATA_2_9__IOF0_IOF7
    {
        funpos1 |= (0x3<<8);
        R_IOF_DIR &= ~0x00FF;
        R_IOF_ATT |= 0x00FF;
        R_IOF_O_DATA |= 0x00FF;
    }
	funpos1 |= 0x01; //enable 2nd sensor
	R_FUNPOS1 = funpos1;
    return MUX_OK;
}
#endif

#if CDSP_CLKO_ENABLE
INT8S cdsp_clko_pinmux_set(INT8U clko_mux,INT8U clko_source)
{
    INT32U funpos1 = R_FUNPOS1;

    //CLKO_POS0 at IO_D7
    if(clko_mux == CDSP_CLKO_POS_MUX0)
    {
        funpos1 |=(0x1<<21);  //CLKO at IO_D7 enabled
        gpio_drving_init_io(IO_D7,(IO_DRV_LEVEL)CDSP_CLKO_DRIVING);
        if(clko_source == CLKO_FROM_CSI)
            funpos1 |=(0x1<<24);
        else
            funpos1 &= ~(0x1<<24);
    }
    else //CLKO_POS1 at IO_D12
    if(clko_mux == CDSP_CLKO_POS_MUX1)
    {
        funpos1 |=(0x1<<22);  //CLKO at IO_D12 enabled
        gpio_drving_init_io(IO_D12,(IO_DRV_LEVEL)CDSP_CLKO_DRIVING);
        if(clko_source == CLKO_FROM_CSI)
            funpos1 |=(0x1<<25);
        else
            funpos1 &= ~(0x1<<25);
    }
    else //CLKO_POS2 at IO_C9
    if(clko_mux == CDSP_CLKO_POS_MUX2)
    {
        funpos1 |=(0x1<<20);  //CLKO at IO_C9 enabled
        gpio_drving_init_io(IO_C9,(IO_DRV_LEVEL)CDSP_CLKO_DRIVING);
        if(clko_source == CLKO_FROM_CSI)
            funpos1 |=(0x1<<23);
        else
            funpos1 &= ~(0x1<<23);
    }
    else //CLKO_POS3 at IO_A9
    if(clko_mux == CDSP_CLKO_POS_MUX3)
    {
        funpos1 |=(0x1<<18);  //CLKO at IO_A9 enabled
        gpio_drving_init_io(IO_A9,(IO_DRV_LEVEL)CDSP_CLKO_DRIVING);
        if(clko_source == CLKO_FROM_CSI)
            funpos1 |=(0x1<<19);
        else
            funpos1 &= ~(0x1<<19);
    }

    R_FUNPOS1 = funpos1;
    return MUX_OK;
}
#endif

#if CDSP_ENABLE
INT8S cdsp_pinmux_set(INT8U ctrl_mux, INT8U hdata_mux, INT8U ldata_mux)
{
	INT32U funpos1 = R_FUNPOS1;
	INT32U systemctl = R_SYSTEM_CTRL0;

    if ((ctrl_mux > 2) || (hdata_mux > 3) || (ldata_mux > 2))
        return MUX_ERR;

    funpos1 &= ~((0x3<<8)|(0x1<<7));//CTRL[7], DATA[9:8]

    if(ctrl_mux == CDSP_CTRL_MUX0){//CDSP_CTRL_CLKI_HSYNC_VSYNC__IOC8_IOC10_IOC11
        funpos1 &= ~(0x1<<7);
    } else if (ctrl_mux == CDSP_CTRL_MUX1 ){//CDSP_CTRL_CLKI_HSYNC_VSYNC__IOD6_IOD8_IOD9
        funpos1 |= (0x1<<7);
    }

    if (hdata_mux == CDSP_DATA2_9_MUX0)//CDSP_DATA_2_9__IOC0_IOC7
        funpos1 &= ~(0x3<<8);
    else if (hdata_mux == CDSP_DATA2_9_MUX1)//CDSP_DATA_2_9__IOB8_IOB15
        funpos1 |= (0x1<<8);
    else if (hdata_mux == CDSP_DATA2_9_MUX2)//CDSP_DATA_2_9__IOE0_IOE7
        funpos1 |= (0x2<<8);
    else if (hdata_mux == CDSP_DATA2_9_MUX3)//CDSP_DATA_2_9__IOF0_IOF7
        funpos1 |= (0x3<<8);


    if (ldata_mux == CDSP_DATA0_1_MUX0)//CSI_DATA_0_1__IOB6_IOB7
        funpos1 &= ~(3<<10);
    else if (ldata_mux == CDSP_DATA0_1_MUX1)//CSI_DATA_0_1__IOB4_IOB5
        funpos1 |= (1<<10);
    else if (ldata_mux == CDSP_DATA0_1_MUX2)//CSI_DATA_0_1__IOC12_IOC13
        funpos1 |= (2<<10);

    systemctl |= (1<<2);//cdsp source is from parallel sensor

	R_FUNPOS1 = funpos1;
	R_SYSTEM_CTRL0 = systemctl;

    return MUX_OK;
}
#endif
#if TIMERA_CCP_ENABLE
INT8S timerA_ccp_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<0);             //TIMER_A_CCP__IOB8 or clear
    if (idx == TIMERA_CCP_MUX1)      //TIMER_A_CCP__IOD7
        timerpos |= (1<<0);
    else if (idx == TIMERA_CCP_MUX2) //TIMER_A_CCP__IOA8
        timerpos |= (2<<0);
    else if (idx == TIMERA_CCP_MUX3) //TIMER_A_CCP__IOE0
        timerpos |= (3<<0);

	R_TIMER_POS = timerpos;
	return MUX_OK;
}
#endif

#if TIMERB_CCP_ENABLE
INT8S timerB_ccp_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<2);        //TIMER_B_CCP__IOB9 or clear
    if (idx == TIMERB_CCP_MUX1) //TIMER_B_CCP__IOD8
        timerpos |= (1<<2);
    else if (idx == TIMERB_CCP_MUX2)   //TIMER_B_CCP__IOA9
        timerpos |= (2<<2);
    else if (idx == TIMERB_CCP_MUX3)   //TIMER_B_CCP__IOE1
        timerpos |= (3<<2);

	R_TIMER_POS = timerpos;
	return MUX_OK;
}
#endif

#if TIMERC_CCP_ENABLE
INT8S timerC_ccp_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<4);        //TIMER_C_CCP__IOB10 or clear
    if (idx == TIMERC_CCP_MUX1) //TIMER_C_CCP__IOD9
        timerpos |= (1<<4);
    else if (idx == TIMERC_CCP_MUX2)   //TIMER_C_CCP__IOA10
        timerpos |= (2<<4);
    else if (idx == TIMERC_CCP_MUX3)   //TIMER_C_CCP__IOE2
        timerpos |= (3<<4);

	R_TIMER_POS = timerpos;
	return MUX_OK;
}
#endif

#if TIMERD_CCP_ENABLE
INT8S timerD_ccp_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<6);        //TIMER_D_CCP__IOC12 or clear
    if (idx == TIMERD_CCP_MUX1) //TIMER_D_CCP__IOD12
        timerpos |= (1<<6);
    else if (idx == TIMERD_CCP_MUX2)    //TIMER_D_CCP__IOA11
        timerpos |= (2<<6);
    else if (idx == TIMERD_CCP_MUX3)    //TIMER_D_CCP__IOE3
        timerpos |= (3<<6);

	R_TIMER_POS = timerpos;
	return MUX_OK;
}
#endif

#if TIMERE_CCP_ENABLE
INT8S timerE_ccp_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<8);        //TIMER_E_CCP__IOC13 or clear
    if (idx == TIMERE_CCP_MUX1) //TIMER_E_CCP__IOD14
        timerpos |= (1<<8);
    else if (idx == TIMERE_CCP_MUX2)    //TIMER_E_CCP__IOA12
        timerpos |= (2<<8);
    else if (idx == TIMERE_CCP_MUX3)    //TIMER_E_CCP__IOE4
        timerpos |= (3<<8);

	R_TIMER_POS = timerpos;
	return MUX_OK;
}
#endif

#if TIMERF_CCP_ENABLE
INT8S timerF_ccp_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<10);       //TIMER_F_CCP__IOD4 or clear
    if (idx == TIMERF_CCP_MUX1) //TIMER_F_CCP__IOD15
        timerpos |= (1<<10);
    else if (idx == TIMERF_CCP_MUX2)    //TIMER_F_CCP__IOA13
        timerpos |= (2<<10);
    else if (idx == TIMERF_CCP_MUX3)    //TIMER_F_CCP__IOE5
        timerpos |= (3<<10);

	R_TIMER_POS = timerpos;
	return MUX_OK;
}
#endif

#if TIMERG_CCP_ENABLE
INT8S timerG_ccp_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<12);       //TIMER_G_CCP__IOD5 or clear
    if (idx == TIMERG_CCP_MUX1) //TIMER_G_CCP__IOB6
        timerpos |= (1<<12);
    else if (idx == TIMERG_CCP_MUX2)    //TIMER_G_CCP__IOA14
        timerpos |= (2<<12);
    else if (idx == TIMERG_CCP_MUX3)    //TIMER_G_CCP__IOE6
        timerpos |= (3<<12);

	R_TIMER_POS = timerpos;
	return MUX_OK;
}
#endif

#if TIMERH_CCP_ENABLE
INT8S timerH_ccp_pinmux_set(INT8U idx)
{
	INT32U timerpos = R_TIMER_POS;

    if (idx > 3)
        return MUX_ERR;

	timerpos &= ~(3<<14);       //TIMER_H_CCP__IOD6 or clear
    if (idx == TIMERH_CCP_MUX1) //TIMER_H_CCP__IOB7
        timerpos |= (1<<14);
    else if (idx == TIMERH_CCP_MUX2)    //TIMER_H_CCP__IOA15
        timerpos |= (2<<14);
    else if (idx == TIMERH_CCP_MUX3)    //TIMER_H_CCP__IOE7
        timerpos |= (3<<14);

	R_TIMER_POS = timerpos;
	return MUX_OK;
}
#endif

void board_init_ice(void)
{
    INT32U gpio_ctrl = R_GPIO_CTRL;

    if(gpio_ctrl&1){
        if (SWD_EN)
            gpio_ctrl |=(1<<6);
    }
    R_GPIO_CTRL = gpio_ctrl;
}

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
void wifi_ldo_init(void)
{
    gpio_init_io(WIFI_LDO_EN_PIN, GPIO_OUTPUT);
    gpio_set_port_attribute(WIFI_LDO_EN_PIN, ATTRIBUTE_HIGH);
    gpio_write_io(WIFI_LDO_EN_PIN, DATA_LOW);
	#if C_WIFI_ON_OFF_KEY == 0
    drv_msec_wait(50);
    gpio_write_io(WIFI_LDO_EN_PIN, DATA_HIGH);
	#endif
}
#endif

void board_init(void)
{
    #if C_SUPPORT_JTAG
    gpio_set_ice_en(ENABLE);
    board_init_ice();
    #else
    gpio_set_ice_en(DISABLE);
    #endif

	board_gpio_config();

#if SDC0_ENABLE
	sdc0_pinmux_set(SDC0_POS);
#endif

#if SDC1_ENABLE
	sdc1_pinmux_set(SDC1_POS);
#endif

#if TFT1_ENABLE
	tft_pinmux_set(TFT_CTRL_POS, TFT_DATA_0_7_POS, TFT_DATA_8_15_POS, TFT_DATA_16_23_POS, TFT_TE_POS);
#endif

#if TFT2_ENABLE
	tft2_pinmux_set(TFT2_CTRL_POS, TFT2_DATA_0_7_POS, TFT2_DATA_8_15_POS, TFT2_DATA_16_23_POS, TFT2_TE_POS);
#endif

#if TFT1_SPI_ENABLE
    tft1_spi_pinmux_set(TFT1_SPI_DATA_0_3_POS,TFT1_SPI_TE_POS);
#endif

#if TFT2_SPI_ENABLE
    tft2_spi_pinmux_set(TFT2_SPI_DATA_0_3_POS,TFT2_SPI_TE_POS);
#endif

#if CDSP_CLKO_ENABLE
    cdsp_clko_pinmux_set(CDSP_CLKO_POS, CDSP_CLKO_SOURCE);
#endif

#if CSI_CLKO_ENABLE
    csi_clko_pinmux_set(CSI_CLKO_POS, CSI_CLKO_SOURCE);
#endif

#if CSI0_ENABLE
    csi_pinmux_set(CSI_CTRL_POS, CSI_DATA_2_9_POS);
#endif

#if CSI2_ENABLE
    csi2_pinmux_set(CSI2_CTRL_POS, CSI2_DATA_2_9_POS);
#endif

#if UART0_ENABLE
    uart0_pinmux_set(UART0_POS, UART_TX_ENABLE, UART_RX_ENABLE);
#endif

#if UART1_ENABLE
    uart1_pinmux_set(UART1_POS, UART_TX_ENABLE, UART_RX_ENABLE);
#endif

#if UART2_ENABLE
	uart2_pinmux_set(UART2_POS, UART_TX_ENABLE, UART_RX_ENABLE);
#endif

#if EXTA_IRQ_ENABLE
	extA_irq_pinmux_set(EXTA_IRQ_POS);
#endif

#if EXTB_IRQ_ENABLE
	extB_irq_pinmux_set(EXTB_IRQ_POS);
#endif

#if EXTC_IRQ_ENABLE
	extC_irq_pinmux_set(EXTC_IRQ_POS);
#endif

#if I2C0_ENABLE
	i2c0_pinmux_set(I2C0_POS);
#endif

#if I2C1_ENABLE
	i2c1_pinmux_set(I2C1_POS);
#endif

#if I2C2_ENABLE
	i2c2_pinmux_set(I2C2_POS);
#endif

#if SPI0_ENABLE
	spi0_pinmux_set(SPI0_POS, SPI0_CS_GPIO_MODE);
#endif

#if SPI1_ENABLE
	spi1_pinmux_set(SPI1_POS, SPI1_CS_GPIO_MODE);
#endif

#if SPIFC_ENABLE
	spifc_pinmux_set(SPIFC_POS);
#endif

#if TIMERA_CCP_ENABLE
    timerA_ccp_pinmux_set(TIMERA_CCP_POS);
#endif

#if TIMERB_CCP_ENABLE
    timerB_ccp_pinmux_set(TIMERB_CCP_POS);
#endif

#if TIMERC_CCP_ENABLE
    timerC_ccp_pinmux_set(TIMERC_CCP_POS);
#endif

#if TIMERD_CCP_ENABLE
    timerD_ccp_pinmux_set(TIMERD_CCP_POS);
#endif

#if TIMERE_CCP_ENABLE
    timerE_ccp_pinmux_set(TIMERE_CCP_POS);
#endif

#if TIMERF_CCP_ENABLE
    timerF_ccp_pinmux_set(TIMERF_CCP_POS);
#endif

#if TIMERG_CCP_ENABLE
    timerG_ccp_pinmux_set(TIMERG_CCP_POS);
#endif

#if TIMERH_CCP_ENABLE
    timerH_ccp_pinmux_set(TIMERH_CCP_POS);
#endif

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
	wifi_ldo_init();
#endif

#if SEN_LDO_2V8_ENABLE
	 drv_l1_ldo28_sel(SEN_LDO_2V8_VALUE, ENABLE);
#endif

	board_clock_config();

	bus_priority_config();
}


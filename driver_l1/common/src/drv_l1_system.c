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

/**
 * @brief   disable long-press power on key
 * @param   key_num: power on key 0 to 4
 * @param   enable: 0 disable/ 1 enable
 * @return 	none
 */
void drv_l1_pwrk_reset_en(INT8U key_num,INT8U enable)
{
    INT32U net;
    net = R_SYSTEM_PWR_RST_EN;
    if (enable)
        net &= ~(1<<key_num);
    else
        net |= (1<<key_num);
    R_SYSTEM_PWR_RST_EN = 0x2018;
    R_SYSTEM_PWR_RST_EN = 0xA59E;
    R_SYSTEM_PWR_RST_EN = net;
}
#if 0
void drv_l1_core_pwr_sel(FB_CORE_SEL vol)
{
    INT32U net;
    if(vol>= FB_MAX)
        return;

    net = R_SYSTEM_DDR_LDO;
    net&= ~(0xF<<4);

    net |= vol<<4;
    R_SYSTEM_DDR_LDO = net;
}
#endif

/**
 * @brief   2.8V  LDO setup and switch
 * @param   vol: voltage selection
 * @param   enable: 0 disable/ 1 enable
 * @return 	none
 */
void drv_l1_ldo28_sel(LDOSEL vol, INT8U enable)
{
    INT32U net;
    if(vol>= LDO_MAX)
        return;
    net = R_SYSTEM_POWER_CTRL0;
    net&= ~(0x7<<5);
    if (enable)
        net |= 0x1<<4;
    else
        net &= ~(0x1<<4);
    net |= vol<<5;
    R_SYSTEM_POWER_CTRL0 = net;
}

/**
 * @brief   audio LDO setup and switch
 * @param   vol: voltage selection
 * @param   enable: 0 disable/ 1 enable
 * @return 	none
 */
void drv_l1_audioldo_sel(AUD_LDO_SEL vol, INT8U enable)
{
    INT32U net;
    if(vol>= AUD_MAX)
        return;
    net = R_SYSTEM_POWER_CTRL0;
    net&= ~(0x3<<9);
    if (enable)
        net |= 0x1<<8;
    else
        net &= ~(0x1<<8);
    net |= vol<<9;
    R_SYSTEM_POWER_CTRL0 = net;

}
/**
 * @brief   SD3.0 LDO setup and switch
 * @param   vol: voltage selection
 * @param   enable: 0 disable/ 1 enable
 * @return 	none
 */
void drv_l1_sd30ldo_sel(SD30_LDO_SEL vol, INT8U enable)
{
    INT32U net;
    if(vol>= SD30_MAX)
        return;
    net = R_SYSTEM_MISC_CTRL0;
    net&= ~(0x3<<9);
    if (enable)
        net |= 0x1<<7;
    else
        net &= ~(0x1<<7);
    net |= vol<<9;
    R_SYSTEM_MISC_CTRL0 = net;

}

unsigned int
drv_l1_sys_pwr_key_read(
	PWR_KEY pwr_key
)
{
	switch(pwr_key)
	{
	case PWR_KEY0:
		return(R_SYSTEM_POWER_CTRL1 & 0x1) ? 1 : 0;
		break;

	case PWR_KEY1:
		return(R_SYSTEM_POWER_CTRL1 & 0x2) ? 1 : 0;
		break;

	case PWR_KEY2:
		return(R_SYSTEM_POWER_CTRL1 & 0x4) ? 1 : 0;
		break;

	case PWR_KEY3:
		return(R_SYSTEM_POWER_CTRL1 & 0x8) ? 1 : 0;
		break;

	default:
		return 0;
		break;
	}
}

void
drv_l1_sys_wdt_enable_set(
	SYS_WDT_EN en
)
{
	INT32U	reg;

	// WDT setting
	reg = R_SYSTEM_WATCHDOG_CTRL;
	reg &= (~0x8000);
	reg |= en << 15;

	R_SYSTEM_WATCHDOG_CTRL = reg;
}

void
drv_l1_sys_wdt_period_set(
	SYS_WDT_PERIOD mode
)
{
	INT32U	reg;

	// WDT setting
	reg = R_SYSTEM_WATCHDOG_CTRL;
	reg &= (~0x0007);
	reg |= mode << 0;

	R_SYSTEM_WATCHDOG_CTRL = reg;
}

SYS_WDT_EN
drv_l1_sys_wdt_enable_get(
	void
)
{
	INT32U	reg;

	reg = R_SYSTEM_WATCHDOG_CTRL;
	reg &= 0x8000;
	reg >>= 15;

	return reg;
}

SYS_WDT_PERIOD
drv_l1_sys_wdt_period_get(
	void
)
{
	INT32U	reg;

	reg = R_SYSTEM_WATCHDOG_CTRL;
	reg &= 0x0007;

	return reg;
}

void
drv_l1_sys_wdt_clear(
	void
)
{
	R_SYSTEM_WATCHDOG_CLEAR = 0xA005;
}

void
drv_l1_sys_ldo_12_en(
	LDO_33_12_EN en
)
{
	INT32U	reg = R_SYSTEM_POWER_CTRL0;

	reg &= (~0x01);
	reg |= en;

	R_SYSTEM_POWER_CTRL0 = reg;
}

void drv_l1_sys_power_handler(void)
{
	DBG_PRINT(">>> %s ... \r\n", __func__);

	drv_l1_sys_wdt_period_set(SYS_WDT_0T25_SEC);
	drv_l1_sys_wdt_enable_set(SYS_WDT_EN_ON);

	drv_l1_sys_ldo_12_en(LDO_12_OFF);

	/*
	==注意:== 
	LDO_V28 當作 DDR2 Power的1.8V 輸入,打開這裡drv_l1_ldo28_sel(LDO_V28, 0); 
	將 ap_state_handling_power_off()內的	drv_l1_ldo28_sel(LDO_V28, 0);關閉	
	==注意:== 
	*/
	#if 0
	drv_l1_ldo28_sel(LDO_V28, 0);	// LDO_2.8v => OFF
	#endif
	
	while(1);
}

/**
 * @brief   USBD PHY force suspend
 * @param   usb id: 0 USB A/ 1 USB B
 * @param   enable: 1 enable PHY suspend; 0 disable USB PHY suspend
 * @return 	none
 */
void drv_l1_usbd_phy_force_suspend(INT8 id, INT8U enable)
{
    if(id){
        if (enable){
            R_SYSTEM_CTRL2 |= 1<<15;
        }
        else{
            R_SYSTEM_CTRL2 &= ~(1<<15);
        }
    }
    else{
        if (enable){
            R_SYSTEM_CTRL0 |= 1<<15;
        }
        else{
            R_SYSTEM_CTRL0 &= ~(1<<15);
        }
    }
}

/**
 * @brief   reset System module by software 
 * @param   reset_module_reg0,reset_module_reg1,reset_module_reg2,reset_module_reg3
 * @param   
 * @return 	none
 */
 /*
Reset hardware module , the description of Bit in control regsiter are shown below.

P_SYSTEM_MSR_CTRL0:
[15]: GPU module software Reset
[14]: Compressor module software Reset
[13]: MIPI DSI module software Reset
[12]: IPU module software Reset
[11]: Line-based rotator module software Reset
[10]: Rotator module software Reset
[9]: H264 module software Reset
[8]: SCAN2MB module software Reset
[7]: SD30 module software Reset
[6]: SD1 module software Reset
[5]: TFT_SPI1 module software Reset
[4]: TFT_SPI2 module software Reset
[3]: I2C2 module software Reset
[2]: I2C1 module software Reset
[1]: I2C0 module software Reset
[0]: CDSP module software Reset

P_SYSTEM_MSR_CTRL1:
[15]: EthernetMAC module software Reset
[14]: TZASC module software Reset
[13]: BCH(bch_find_err_top) module software Reset
[12]: USB20Dev_B module software Reset
[11]: USB20Host_B module software Reset
[10]: USB20Dev_A module software Reset
[9]: USB20Host_A module software Reset
[8]: Scaler module software Reset
[7]: JPG_B module software Reset
[6]: Auto mode module software Reset
[5]: JPG_A module software Reset
[4]: MP3 Accelator module software Reset
[3]: SPU module software Reset
[2]: PPU Sprite Cache module software Reset
[1]: PPU module software Reset
[0]: GPDLA module software Reset


P_SYSTEM_MSR_CTRL2:
[15]: MIPI RX 4-Lane module software Reset
[14]: MIPI RX 2-Lane module software Reset
[13]: PSCA_YONLY module software Reset
[12]: PSCA_B module software Reset
[11]: PSCA_A module software Reset
[10]: MB2SCAN module software Reset
[9]: FFT module software Reset
[8]: LBP module software Reset
[7]: BOX filter module software Reset
[6]: System FIFO 5 module software Reset
[5]: System FIFO 4 module software Reset
[4]: System FIFO 3 module software Reset
[3]: System FIFO 2 module software Reset
[2]: System FIFO 1 module software Reset
[1]: System FIFO 0 module software Reset
[0]: SPIFC module software Reset


P_SYSTEM_MSR_CTRL3:
[15]: Security H/W module software Reset
[14]: Binning_B_Ch3 module software Reset
[13]: Binning_B_Ch2 module software Reset
[12]: Binning_B_Ch1 module software Reset
[11]: Binning_B module software Reset
[10]: Binning_A module software Reset
[9]: HDMI module software Reset
[8]: DAC module software Reset
[7]: ADC module software Reset
[6]: Memory controller module software Reset
[5]: DISP module software Reset
[4]: DMA module software Reset
[3]: GP420 converter module software Reset
[2]: Rot module software Reset
[1]: i2srx module software Reset
[0]: i2stx module software Reset

 */
void drv_l1_reset_sysmodule_by_software(INT32U reset_module_reg0,INT32U reset_module_reg1,INT32U reset_module_reg2,INT32U reset_module_reg3)
{
	INT32U cnt=0,loop_cnt=0;

		if(reset_module_reg0!=0)
		{
			P_SYSTEM_MSR_CTRL0|=reset_module_reg0;	
			for(cnt=0;cnt<100;cnt++)
				loop_cnt++;
			P_SYSTEM_MSR_CTRL0&=~reset_module_reg0;	
		}
		loop_cnt=0;
		
		if(reset_module_reg1!=0)
		{
			P_SYSTEM_MSR_CTRL1|=reset_module_reg1;	
			for(cnt=0;cnt<100;cnt++)
				loop_cnt++;
			P_SYSTEM_MSR_CTRL1&=~reset_module_reg1;	
		}
		loop_cnt=0;

		if(reset_module_reg2!=0)
		{
			P_SYSTEM_MSR_CTRL2|=reset_module_reg2;	
			for(cnt=0;cnt<100;cnt++)
				loop_cnt++;
			P_SYSTEM_MSR_CTRL2&=~reset_module_reg2;	
		}
		loop_cnt=0;

		if(reset_module_reg3!=0)
		{
			P_SYSTEM_MSR_CTRL3|=reset_module_reg3;	
			for(cnt=0;cnt<100;cnt++)
				loop_cnt++;
			P_SYSTEM_MSR_CTRL3&=~reset_module_reg3;	
		}
		loop_cnt=0;
		
		for(cnt=0;cnt<100;cnt++)
				loop_cnt++;	
		
}



#ifndef __drv_l1_SYSTEM_H__
#define __drv_l1_SYSTEM_H__

#include "project.h"

typedef enum{
    LDO_V28=0,
    LDO_V29,
    LDO_V30,
    LDO_V15,
    LDO_V18,
    LDO_V20,
    LDO_V25,
    LDO_V27,
    LDO_MAX
} LDOSEL;

typedef enum{
    LDOXCK_V30=0,
    LDOXCK_V31,
    LDOXCK_V29,
    LDOXCK_V28,
    LDOXCK_MAX
} LDOXCKSEL;

typedef enum{
    FB_V152 = 0,
    FB_V148,
    FB_V144,
    FB_V140,
    FB_V136,
    FB_V132,
    FB_V128,
    FB_V124,
    FB_V120,
    FB_V116,
    FB_V112,
    FB_V108,
    FB_V104,
    FB_V100,
    FB_V096,
    FB_V092,
    FB_MAX
} FB_CORE_SEL;

typedef enum{
    DDR_V18=0,
    DDR_V19,
    DDR_V20,
    DDR_V22,
    DDR_V25,
    DDR_V27,
    DDR_V28,
    DDR_V30,
	LDO_DDR_DEF = DDR_V22,
    DDR_MAX
} DDR_LDO_SEL;

typedef enum{
    AUD_V30,
    AUD_V31,
    AUD_V29,
    AUD_V28,
    AUD_MAX
} AUD_LDO_SEL;

typedef enum{
    SD30_V18,
    SD30_V19,
    SD30_V20,
    SD30_V33,
    SD30_MAX
} SD30_LDO_SEL;

typedef enum{
    CDSP_RST,       //0
    I2C0_RST,
    I2C1_RST,
    I2C2_RST,
    TFT_SPI2_RST,
    TFT_SPI1_RST,
    SD_RST,
    SD30_RST,
    SCAN2MB_RST,
    H264_RST,
    ARBT_ROT_RST,  //0xc018
    LINE_ROT_RST,  //0xc01A
    IPU_RST,
    MIPI_DSI_RST,
    COMPRESSOR_RST,
    GPU_RST,
    GPDLA_RST,      //16
    PPU_MDL_RST,
    PPU_SPR_RST,
    SPU_RST,
    MP3_RST,
    JPGA_RST,
    AUTO_MODE_RST,
    JPGB_RST,
    SCALER_RST,
    USBH_A_RST,
    USBD_A_RST,
    USBH_B_RST,
    USBD_B_RST,
    BCH_RST,
    TZASC_RST,
    ETHERNET_RST,
    SPIFC_RST,      //32
    SYS_FIFO0_RST,
    SYS_FIFO1_RST,
    SYS_FIFO2_RST,
    SYS_FIFO3_RST,
    SYS_FIFO4_RST,
    SYS_FIFO5_RST,
    BOX_RST,
    LBP_RST,
    FFT_RST,
    MB2SCAN_RST,
    PSCA_A_RST,
    PSCA_B_RST,
    PSCA_Y_RST,
    MIPI_RX0_RST,
    MIPI_RX1_RST,
    I2STX_RST,      //48
    I2SRX_RST,
    ROT0_RST,       //0xc024
    GP420_CNVT_RST,
    DMA_RST,
    TFT2_DISP_RST,
    MEM_CTRL_RST,
    ADC_RST,
    DAC_RST,
    HDMI_RST,
    BININGA_RST,
    BININGB_RST,
    BININGB_CH1_RST,
    BININGB_CH2_RST,
    BININGB_CH3_RST,
    SECURITY_RST,   //63
    RST_MAX
}MODULE_RESET;

typedef enum
{
	PWR_KEY0,
	PWR_KEY1,
	PWR_KEY2,
	PWR_KEY3
} PWR_KEY;

typedef enum
{
	SYS_WDT_2T0_SEC		= 0x0,
	SYS_WDT_1T0_SEC		= 0x1,
	SYS_WDT_0T5_SEC		= 0x2,
	SYS_WDT_0T25_SEC	= 0x3,
	SYS_WDT_4T0_SEC		= 0x4,
	SYS_WDT_8T0_SEC		= 0x5,
	SYS_WDT_16T0_SEC	= 0x6,
	SYS_WDT_0T125_SEC	= 0x7,
	SYS_WDT_DEF			= SYS_WDT_2T0_SEC
} SYS_WDT_PERIOD;

typedef enum
{
	SYS_WDT_EN_OFF	= 0x0,
	SYS_WDT_EN_ON	= 0x1,
	SYS_WDT_EN_DEF	= SYS_WDT_EN_OFF
} SYS_WDT_EN;

typedef enum
{
	LDO_12_OFF		= 0x0,
	LDO_12_ON		= 0x1
} LDO_33_12_EN;

typedef enum
{
	LDO_DDR_EN_OFF	= 0x0,
	LDO_DDR_EN_ON	= 0x1,
	LDO_DDR_EN_DEF	= LDO_DDR_EN_ON
} LDO_DDR_EN;

extern void drv_l1_pwrk_reset_en(INT8U key_num,INT8U enable);
extern void drv_l1_core_pwr_sel(FB_CORE_SEL vol);
extern void drv_l1_ldo18_sel(LDOSEL vol, INT8U enable);
extern void drv_l1_ldo28_sel(LDOSEL vol, INT8U enable);
extern void drv_l1_ldoxckg_sel(LDOXCKSEL vol);
extern void drv_l1_ddrldo_sel(DDR_LDO_SEL vol, INT8U enable);
extern void drv_l1_audioldo_sel(AUD_LDO_SEL vol, INT8U enable);

extern unsigned int drv_l1_sys_pwr_key_read(PWR_KEY pwr_key);
extern void drv_l1_sys_wdt_enable_set(SYS_WDT_EN en);
extern void drv_l1_sys_wdt_period_set(SYS_WDT_PERIOD mode);
extern SYS_WDT_EN drv_l1_sys_wdt_enable_get(void);
extern SYS_WDT_PERIOD drv_l1_sys_wdt_period_get(void);
extern void drv_l1_sys_wdt_clear(void);
extern void drv_l1_sys_ldo_12_en(LDO_33_12_EN en);
extern void drv_l1_sys_power_handler(void);

extern void drv_l1_usbd_phy_force_suspend(INT8 id, INT8U enable);
extern void drv_l1_reset_sysmodule_by_software(INT32U reset_module_reg0,INT32U reset_module_reg1,INT32U reset_module_reg2,INT32U reset_module_reg3);

#endif		// __drv_l1_SYSTEM_H__

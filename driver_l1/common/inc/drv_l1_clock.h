#ifndef __DRV_L1_CLOCK_H__
#define __DRV_L1_CLOCK_H__

#include "typedef.h"

#define XT24M               24
#define XTAL_CLK_M          24

#define C_PLLN_MIN          216
#define C_PLLN_MAX          1206

typedef enum
{
    SRC_SPU27M,
    SRC_C27M,
    SRC_TMDS,
    SRC_MIPI,
    SRC_DSI,
    SRC_LPRX_SCLK,
    SRC_APCK,
    SRC_AUDIO_MCLK,
    SRC_I2S,
    SRC_I2S_RX,
    SRC_HDMI_AUDIO,
    PERI_CLOCK_SRC_TOTAL
} PERI_CLOCK_SRC_ENUM;

typedef enum
{
    I2S_MAIN_FREQ_73728MHZ, // div 1
    I2S_MAIN_FREQ_36864MHZ, // div 2, wolfson wm8988, clkdiv2
    I2S_MAIN_FREQ_24576MHZ, // div 3, wolfson wm8988, clkdiv2
    I2S_MAIN_FREQ_18432MHZ, // div 4
    I2S_MAIN_FREQ_14745MHZ, // div 5
    I2S_MAIN_FREQ_12288MHZ, // div 6, wolfson wm8988, clkbase

    I2S_MAIN_FREQ_67738MHZ, // div 1
    I2S_MAIN_FREQ_33869MHZ, // div 2, wolfson wm8988, clkdiv2
    I2S_MAIN_FREQ_22579MHZ, // div 3, wolfson wm8988, clkdiv2
    I2S_MAIN_FREQ_16934MHZ, // div 4
    I2S_MAIN_FREQ_13547MHZ, // div 5
    I2S_MAIN_FREQ_11289MHZ,  // div 6, wolfson wm8988, clkbase

    I2S_MAIN_FREQ_16385MHZ,  // 73.728MHz div 4.5, 16.385MHz
    I2S_MAIN_FREQ_8192MHz,   // 73.728MHz div 9, 8.192MHz

    I2S_MAIN_FREQ_6144MHZ,   // 73.728 div12, 6.144 MHz
    I2S_MAIN_FREQ_5644MHZ,   // 63.7376 div 12, 5.644 MHz
    I2S_MAIN_FREQ_4096MHZ,   // 73.728 div 18, 4.096 MHz
    I2S_MAIN_FREQ_3072MHZ,   // 73.728 div 24, 3.072 MHz
    I2S_MAIN_FREQ_28224MHZ,  // 63.7376 div 24, 2.8224 MHz
    I2S_MAIN_FREQ_2048MHZ    // 73.728 div 36, 2.048 MHz

} I2S_MAIN_FREQ_E;

typedef enum
{
    APLL_FREQ_67738MHZ,
    APLL_FREQ_73728MHZ
} APLL_FREQ_E;

typedef enum
{
    DDR_CLK = 0,
    PLLEXT  = 3,
    PLLEXTx2 = 6,
    PLLSYSx2 = 2,
} GPU_CLK_SRC;

typedef enum
{
    CLK_EN0_IRAM0,
    CLK_EN0_CPU_RELATIVE_HW,
    CLK_EN0_YUV_REMAP,
    CLK_EN0_GPIO_RTC,
    CLK_EN0_IRAM1,
    CLK_EN0_BOOTROM,
    CLK_EN0_INTERRUPT,
    CLK_EN0_AUTO_FIFO,
    CLK_EN0_SYSTEM_CTRL,
    CLK_EN0_SYSTEM_CTRL_REG_BANK,
    CLK_EN0_DMA_IF,//10
    CLK_EN0_DMA_CH0_CH1,
    CLK_EN0_DMA_CH2_CH3,
    CLK_EN0_DMA_CH4_CH5,
    CLK_EN0_DMA_CH6_CH7,
    CLK_EN0_APB_BUS,
    CLK_EN1_TIMER,
    CLK_EN1_TIMEBASE,
    CLK_EN1_JPEG_A,
    CLK_EN1_PPU_FB_AHBMUX,
    CLK_EN1_PPU_REG_BANK,//20
    CLK_EN1_TFT,
    CLK_EN1_STN,
    CLK_EN1_RESERVED,
    CLK_EN1_PPU_FRAME_BUF,
    CLK_EN1_PPU_MUX,
    CLK_EN1_USB20_HOST_A,
    CLK_EN1_USB20_DEVICE_A,
    CLK_EN1_SCALER,
    CLK_EN1_SPU,
    CLK_EN1_DAC_MIXER,//30
    CLK_EN1_SPU_SLAVE,
    CLK_EN2_UART0_REG_BANK,
    CLK_EN2_UART0,
    CLK_EN2_UART1_REG_BANK,
    CLK_EN2_UART1,
    CLK_EN2_UART2_REG_BANK,
    CLK_EN2_UART2,
    CLK_EN2_I2C0,
    CLK_EN2_I2C1,
    CLK_EN2_I2C2,//40
    CLK_EN2_SPI0,
    CLK_EN2_SPI1,
    CLK_EN2_SAR_ADC,
    CLK_EN2_DAC,
    CLK_EN2_I2S_TX0,
    CLK_EN2_I2S_RX0,
    CLK_EN2_I2S_TX1,
    CLK_EN3_I2S_RX1,
    CLK_EN3_I2S_TX2,
    CLK_EN3_I2S_RX2,//50
    CLK_EN3_I2S_TX3,
    CLK_EN3_I2S_RX3,
    CLK_EN3_I2S_TX_SRAM,
    CLK_EN3_I2S_RX_SRAM,
    CLK_EN3_RESERVED,
    CLK_EN3_DDR_CTRL,
    CLK_EN3_NAND_FLASH_CTRL,
    CLK_EN3_SPI_FLASH_CTRL,
    CLK_EN3_PSCA_A,
    CLK_EN3_PSCA_B,//60
    CLK_EN3_PSCA_Y,
    CLK_EN3_IPU,
    CLK_EN3_SD0_REG_BANK,
    CLK_EN4_SD0,
    CLK_EN4_SD2_REG_BANK,
    CLK_EN4_RESERVED,
    CLK_EN4_SD2,
    CLK_EN4_ETHERNET_MAC,
    CLK_EN4_BINNING_A,
    CLK_EN4_BINNING_B,//70
    CLK_EN4_BINNING_CH1,//DDR controller
    CLK_EN4_BINNING_CH2,
    CLK_EN4_BINNING_CH3,
    CLK_EN4_DISP,
    CLK_EN4_TFT_SPI1,
    CLK_EN4_TFT_SPI0,
    CLK_EN4_MP3,
    CLK_EN4_LB_ROTATOR_SRAM,
    CLK_EN4_BCH,
    CLK_EN5_TZASC,//80
    CLK_EN5_SYSTEM_FIFO,
    CLK_EN5_FIR,
    CLK_EN5_RESERVED,
    CLK_EN5_USB_RAM_B,
    CLK_EN5_HDMI_AUDIO,
    CLK_EN5_EFUSE_CTRL,
    CLK_EN5_TRNG,
    CLK_EN5_RESERVED1,
    CLK_EN5_USB_SRAM,
    CLK_EN5_HDMI,//90
    CLK_EN5_CDSP_PIXEL_CLK_FROM_SYS_CLK,
    CLK_EN5_RESERVED2,
    CLK_EN5_CDSP_AHB,
    CLK_EN5_USB20_HOST_B,
    CLK_EN5_JPEG_B,
    CLK_EN6_ROT,
    CLK_EN6_MB2SCAN,
    CLK_EN6_FFT,
    CLK_EN6_H264_SCAL,
    CLK_EN6_H264,//100
    CLK_EN6_H264_DEC,
    CLK_EN6_H264_AVS_CORE,
    CLK_EN6_H264_AVS_DEC,
    CLK_EN6_SCAN2MB,
    CLK_EN6_LBP,
    CLK_EN6_USB20_DEVICE_B,
    CLK_EN6_BOX_FILTER,
    CLK_EN6_ROTATOR,
    CLK_EN6_LB_ROTATOR,
    CLK_EN6_MIPI_TX,//110
    CLK_EN6_GPDLA,
    CLK_EN7_COMPRESSOR,
    CLK_EN7_GPU,
    CLK_EN7_SECURITY_IP,
    CLK_EN7_FBC_ENC,
    CLK_EN7_FBC,
    CLK_EN7_FFT_SRAM,
    CLK_EN7_H264_SRAM,
    CLK_EN7_ETHERNETMAC_SRAM,
    CLK_EN7_RESERVED,//120
    CLK_EN7_SCAN2MB_SRAM,
    CLK_EN7_CDSP_SRAM,
    CLK_EN7_PPU_SRAM,
    CLK_EN7_PSCA_A_SRAM,
    CLK_EN7_PSCA_B_SRAM,
    CLK_EN7_MIPI_RX_A,
    CLK_EN7_MIPI_RX_B,
    CLK_EN_MAX,
} SYSTEM_CLK_EN_E;

extern INT32U MCLK;
extern INT32U MHZ;

extern void drv_l1_clock_set_peripheral_clock_enable(PERI_CLOCK_SRC_ENUM src, INT8U enable);
extern INT8U drv_l1_clock_get_peripheral_clock_enable_state(PERI_CLOCK_SRC_ENUM src);
extern INT32U drv_l1_clock_get_system_pll_freq(void);
extern INT32U drv_l1_clock_get_system_clock_freq(void);
extern INT32U drv_l1_clock_get_i2s_main_mclk_freq(void);
extern INT32S drv_l1_clock_set_i2s_main_mclk_freq(I2S_MAIN_FREQ_E req_freq);
extern void drv_l1_clock_system_clk_init(void);
extern INT16S drv_l1_clock_set_system_clk_en(INT16U module, INT16U enable);
extern INT32U drv_l1_clock_set_ext_pll_freq(INT32U ext_pll, INT8U enable);
extern INT32U drv_l1_clock_get_ext_pll_freq(void);
extern void drv_l1_clock_set_peripheral_pre_dvi2(INT8U enable);
extern void drv_l1_clock_set_peripheral_src_extpll(INT8U enable);
extern void drv_l1_clock_set_peripheral_clock_freq(PERI_CLOCK_SRC_ENUM src, INT32U target_clock);
extern INT32U drv_l1_clock_get_peripheral_clock_source_freq(void);
extern INT32U drv_l1_clock_get_system_pll_freq(void);
extern INT32U drv_l1_clock_get_cpu_clock_freq(void);
extern void drv_l1_clock_set_audio_pll_freq(INT8U base_freq, INT8U enable);
extern INT32U drv_l1_clock_get_ddr_pll_freq(void);

extern INT32U drv_l1_clock_cal_system_clock_freq(INT32U r_system_pllen, INT32U r_system_ext_clk_ctrl);
extern INT32U drv_l1_clock_cal_system_pll_freq(INT32U r_system_pllen);

extern void drv_l1_clock_set_ddr_pll_freq(INT32U freq);
extern INT32U drv_l1_clock_cal_ddr_pll_freq_idx(INT32U freq);
extern void drv_l1_clock_print_major_clock_info(void);
extern void drv_l1_gpu_clk_enable(INT8U enable);
extern void drv_l1_DSICLKX8_src_select(INT8U extpll);
extern INT32S drv_l1_gpu_clk_setting(INT8U enable,GPU_CLK_SRC set, INT8U div);
#endif

//#include "drv_l1_sfr.h"
//#include "drv_l1_tft.h"
//#include "drv_l1_gpio.h"
//#include "drv_l2_display.h"
//#include "drv_l1_tft_spi.h"
//#include "drv_l1_interrupt.h"

#include "drv_l2.h"
#include "drv_l1_tft_spi.h"

#if (defined LCD_DRV_EN_DST_GC9106) && (LCD_DRV_EN_DST_GC9106 == 1)
#define DST_GC9106_ENABLE       1
#elif(defined DST_GC9106_TFT1) && (DST_GC9106_TFT1 == 1)
#define DST_GC9106_ENABLE       1
#else
#define DST_GC9106_ENABLE       0
#endif

#if (defined DST_GC9106_ENABLE) && (DST_GC9106_ENABLE == 1)
#define TFT_SPI_3BIT_ENABLE     TFT_SPI_3WIRE_ENABLE
#define C_LCD_WIDTH             128
#define C_LCD_HEIGHT            160

#define RST_PAD                 IO_C9//IO_A15//IO_A15//IO_A8//IO_A1
#define CS_PIN                  IO_B4//IO_C9//IO_B4//IO_A0
static INT32U TFT_SPI_NUM_USE;

extern INT32S drv_l2_dual_display_init_sim(void);
extern void tft2_rot2_sim_enable(void);

void tft_Delay(int ms)
{
    vTaskDelay(ms);
}

void cmd_delay(INT32U i)
{
	unsigned int j, cnt;

	cnt = i*4;
	for (j=0;j<cnt;j++);
}

void cmd_longdelay(INT32U i)
{
	unsigned long j, cnt;

	cnt = i*100;
	for (j=0;j<cnt;j++);
}

void tft_GC9106_reset(void)
{
    if(TFT_SPI_NUM_USE == TFT_SPI0)
        R_TFT_CTRL = 0;
    else
        R_TFT2_CTRL = 0;
	gpio_write_io(RST_PAD, DATA_HIGH);
	tft_Delay(1);
	gpio_write_io(RST_PAD, DATA_LOW);
    tft_Delay(10);
	gpio_write_io(RST_PAD, DATA_HIGH);
	tft_Delay(100);
}

void WriteComm(unsigned int i)
{
    //gpio_write_io(CS_PIN, DATA_LOW);
#if TFT_SPI_3BIT_ENABLE == 1
    drv_l1_tft_spi_write_command_set(TFT_SPI_NUM_USE, 0, i);
#else
    drv_l1_tft_spi_write_command_set(TFT_SPI_NUM_USE, 1, i);
#endif
    //gpio_write_io(CS_PIN, DATA_HIGH);
}

void WriteData(unsigned int i)
{
    //gpio_write_io(CS_PIN, DATA_LOW);
#if TFT_SPI_3BIT_ENABLE == 1
    drv_l1_tft_spi_write_data_set(TFT_SPI_NUM_USE, 0, i);
#else
    drv_l1_tft_spi_write_data_set(TFT_SPI_NUM_USE, 1, i);
#endif
    //gpio_write_io(CS_PIN, DATA_HIGH);
}

void LCD_Init(void)
{
    // HSD1.77+GC9106
    //----end Reset Sequence--------

    WriteComm(0xfe);
    WriteComm(0xfe);
    WriteComm(0xef);

    WriteComm(0xb3);
    WriteData(0x03);
    WriteComm(0xb6);
    WriteData(0x01);

    WriteComm(0xa3);
    WriteData(0x11);

    WriteComm(0x21);

    #if 1 // te
    WriteComm(0x34);
    WriteData(0x34);
    #endif

    WriteComm(0x36);
    WriteData(0xD8);//0xF8//0xD8//0x98//0x78//0x58//0x18
    //WriteData(0x08);//0xF8//0xD8//0x98//0x78//0x58//0x18

    WriteComm(0x3a);
    WriteData(0x05);

    WriteComm(0xb4);
    WriteData(0x21);//0x03//0x07//0x00//0x21

    WriteComm(0xF0);
    WriteData(0x2d);
    WriteData(0x54);
    WriteData(0x24);
    WriteData(0x61);
    WriteData(0xab);
    WriteData(0x2e);
    WriteData(0x2f);
    WriteData(0x00);
    WriteData(0x20);
    WriteData(0x10);
    WriteData(0x10);
    WriteData(0x17);
    WriteData(0x13);
    WriteData(0x0f);
    WriteComm(0xF1);
    WriteData(0x02);
    WriteData(0x22);
    WriteData(0x25);
    WriteData(0x35);
    WriteData(0xa8);
    WriteData(0x08);
    WriteData(0x08);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x09);
    WriteData(0x09);
    WriteData(0x17);
    WriteData(0x18);
    WriteData(0x0f);
    WriteComm(0xfe);
    WriteComm(0xff);
    WriteComm(0x11);

    #if 0
    cmd_longdelay(300*100);
    #else
    tft_Delay(300);
    #endif

    WriteComm(0x35);//vte
    WriteData(0x00);

    //tft_Delay(120);
    WriteComm(0x29);
    WriteComm(0x2C);
}

void BlockWrite(unsigned int Xstart,unsigned int Xend,unsigned int Ystart,unsigned int Yend)
{
	//ILI9163C
	WriteComm(0x2A);
	WriteData(Xstart>>8);
	WriteData(Xstart);
	WriteData(Xend>>8);
	WriteData(Xend);

	WriteComm(0x2B);
	WriteData(Ystart>>8);
	WriteData(Ystart);
	WriteData(Yend>>8);
	WriteData(Yend);

	WriteComm(0x2c);

#if 0 // for debug
    INT32U i;

    for(i=0;i<(C_LCD_WIDTH*C_LCD_HEIGHT);i++)
    {
        WriteData(0xF8);
        WriteData(0x00);
    }
#endif
}

INT32S tft_dst_gc9106_init(INT32U tft_index)
{
   if(tft_index==TFT_1)
     tft_index = TFT_SPI0;
   if(tft_index==TFT_2)
     tft_index = TFT_SPI1;


  	TFT_SPI_NUM_USE = tft_index;

    gpio_init_io (RST_PAD, GPIO_OUTPUT);
	gpio_init_io (CS_PIN, GPIO_OUTPUT);

	gpio_set_port_attribute(RST_PAD, 1);
#if 1
  	tft_GC9106_reset();
  	LCD_Init();
	BlockWrite(0, (C_LCD_WIDTH-1), 0, (C_LCD_HEIGHT-1));
#else
	gpio_set_port_attribute(CS_PIN, 1);

    gpio_write_io(CS_PIN, DATA_HIGH);
  	tft_GC9106_reset();
  	LCD_Init();
	BlockWrite(0, (C_LCD_WIDTH-1), 0, (C_LCD_HEIGHT-1));
    gpio_write_io(CS_PIN, DATA_LOW);
    tft_Delay(100);
    gpio_write_io(CS_PIN, DATA_HIGH);
    tft_Delay(100);
    gpio_write_io(CS_PIN, DATA_LOW);
    tft_Delay(100);
#endif
    if(tft_index == TFT_SPI0)
    {
        R_PPU_ENABLE = 0x80;
    }
#if 0 // for debug
    R_PPU_ENABLE = 0x80;
    R_FREE_SIZE	= ((C_LCD_WIDTH & 0x7FF) << 16) | (C_LCD_HEIGHT & 0x7FF);

    INT16U *ptr = (INT16U *)0x900000;
    INT32U i;

    for(i=0;i<C_LCD_WIDTH*C_LCD_HEIGHT;i++)
        *ptr++ = 0x7e0;
    if(TFT_SPI_NUM_USE == 0)
        R_TFT_FBI_ADDR = 0x900000;
    else
        R_TV_FBI_ADDR = 0x900000;
#endif

    drv_l1_tft_spi_init(TFT_SPI_NUM_USE);
    drv_l1_tft_spi_ck_pha(TFT_SPI_NUM_USE, 0);
    drv_l1_tft_spi_ck_pol(TFT_SPI_NUM_USE, 0);
    drv_l1_tft_spi_colok_out_set(TFT_SPI_NUM_USE, TFT_SPI_CLK_DIVIDE_4); //TFT_SPI_CLK_DIVIDE_4
    drv_l1_tft_spi_tx_inte_enable(TFT_SPI_NUM_USE, 1);
    drv_l1_tft_spi_manual_mode_set(TFT_SPI_NUM_USE, 0);
    drv_l1_tft_spi_rx_data_sel_set(TFT_SPI_NUM_USE, 0);
    drv_l1_tft_spi_tx_disable_set(TFT_SPI_NUM_USE, 0);
    drv_l1_tft_spi_master_rx_set(TFT_SPI_NUM_USE, 0);

    drv_l1_tft_spi_vsync_set(TFT_SPI_NUM_USE, 0x3, 0xa, 1);
    drv_l1_tft_spi_v_disp_set(TFT_SPI_NUM_USE, C_LCD_HEIGHT);
#if TFT_SPI_3BIT_ENABLE == 1
    drv_l1_tft_spi_h_disp_set(TFT_SPI_NUM_USE, C_LCD_WIDTH, 1);
#else
    drv_l1_tft_spi_h_disp_set(TFT_SPI_NUM_USE, C_LCD_WIDTH, 0);
#endif
    drv_l1_tft_spi_data_cnt_set(TFT_SPI_NUM_USE, C_LCD_WIDTH, C_LCD_HEIGHT);

#if DISPLAY_DUAL_MODE_ENABLE == 1
    drv_l1_tft_spi_io_keep_enable(TFT_SPI_NUM_USE, 1);
#endif

#if TFT_I80_WR_ONCE == 1
    //drv_l1_tft_spi_te_line_cnt_set(TFT_SPI_NUM_USE, 2, 2);
    drv_l1_tft_spi_te_line_cnt_set(TFT_SPI_NUM_USE, 8, 0x800);
#else
#if TFT_SPI_3BIT_ENABLE == 1
    //drv_l1_tft_spi_te_line_cnt_set(TFT_SPI_NUM_USE, 2, 2);
    drv_l1_tft_spi_te_line_cnt_set(TFT_SPI_NUM_USE, 8, 0x800);
    drv_l1_tft_spi_enable(TFT_SPI_NUM_USE, 0);
    drv_l1_tft_spi_te_mode_set(TFT_SPI_NUM_USE, 0);
    drv_l1_tft_spi_te_enable(TFT_SPI_NUM_USE, 1);
    drv_l1_tft_spi_manual_frame_mode_set(TFT_SPI_NUM_USE, 1);
    drv_l1_tft_spi_ppu_mem_continue_set(TFT_SPI_NUM_USE, 1);
    drv_l1_tft_spi_rgb_order_set(TFT_SPI_NUM_USE, 0);
    drv_l1_tft_spi_4wire_set(TFT_SPI_NUM_USE, 0);
    drv_l1_tft_spi_ppu_mem_start(TFT_SPI_NUM_USE, 1);
#if DISPLAY_DUAL_MODE_ENABLE == 1
    drv_l1_tft_spi_io_keep_enable(TFT_SPI_NUM_USE, 1);
#endif
    drv_l1_tft_spi_enable(TFT_SPI_NUM_USE, 1);
#else
    //drv_l1_tft_spi_te_line_cnt_set(TFT_SPI_NUM_USE, 2, 2);
    drv_l1_tft_spi_te_line_cnt_set(TFT_SPI_NUM_USE, 8, 0x800);
    drv_l1_tft_spi_enable(TFT_SPI_NUM_USE, 0);
    drv_l1_tft_spi_te_mode_set(TFT_SPI_NUM_USE, 0);
    drv_l1_tft_spi_te_enable(TFT_SPI_NUM_USE, 0); // 1
    drv_l1_tft_spi_manual_frame_mode_set(TFT_SPI_NUM_USE, 1);
    drv_l1_tft_spi_ppu_mem_continue_set(TFT_SPI_NUM_USE, 1);
    drv_l1_tft_spi_rgb_order_set(TFT_SPI_NUM_USE, 0);
    drv_l1_tft_spi_4wire_set(TFT_SPI_NUM_USE, 1);
    drv_l1_tft_spi_ppu_mem_start(TFT_SPI_NUM_USE, 1);
#if DISPLAY_DUAL_MODE_ENABLE == 1
    drv_l1_tft_spi_io_keep_enable(TFT_SPI_NUM_USE, 1);
#endif
    drv_l1_tft_spi_enable(TFT_SPI_NUM_USE, 1);
#endif
#endif

	return STATUS_OK;
}

#if (defined DST_GC9106) && (DST_GC9106 == 1)
// tft table
const DispCtrl_t TFT_Param =
{
	C_LCD_WIDTH,
	C_LCD_HEIGHT,
	0,
	0,
	0,
	tft_dst_gc9106_init
};
#endif

#if (defined LCD_DRV_EN_DST_GC9106) && (LCD_DRV_EN_DST_GC9106 == 1)
// tft table
const DispCtrl_t TFT_Param_DST_GC9106 =
{
	C_LCD_WIDTH,
	C_LCD_HEIGHT,
	0,
	0,
	0,
	0,

	0,
	0,

	0,
	0,
	0,
	0,
	0,
	0,

	0,
	0,

	0,
	0,
	/* lcd init & uninit */
	tft_dst_gc9106_init,
	0,
	/* lcd backlight control */
	LCD_BL_CTRL_CONSTANT,
	20 * 1000,	// in HZ for LCD_BL_CTRL_PWM
	55,			// in duty cycle for LCD_BL_CTRL_PWM
	/* tft format */
    #if RESOURCE_FORMAT == RESOURCE_GP420
	DISP_FMT_GP420,
	#elif RESOURCE_FORMAT == RESOURCE_RGB565
	DISP_FMT_RGB565,
	#elif RESOURCE_FORMAT == RESOURCE_YUYV
	DISP_FMT_YUYV,
	#endif
	/* tft IFCE     */
	LCD_IFCE_SPI,

	/* tft rotate   */
    TFT_ROT_NONE,//TFT_ROT_NONE,

    /*  tft width & height  */
    C_LCD_WIDTH,
	C_LCD_HEIGHT

};
#endif

#endif //(defined DST_GC9106) && (DST_GC9106 == 1)

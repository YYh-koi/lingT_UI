#ifndef __drv_l1_SPIFC_H__
#define __drv_l1_SPIFC_H__

#ifndef _SPIFC_REG
#define _SPIFC_REG
typedef struct spifcReg_s
{
	volatile INT32U	REG_SPI;	//0xC0010000
	volatile INT32U	REG_CMD;	//0xC0010004
	volatile INT32U	REG_PARA;	//0xC0010008
	volatile INT32U	REG_ADDRL;	//0xC001000C
	volatile INT32U	REG_ADDRH;	//0xC0010010
	volatile INT32U	REG_TX_WD;	//0xC0010014
	volatile INT32U	REG_RX_RD;	//0xC0010018
	volatile INT32U	REG_TX_BC;	//0xC001001C
	volatile INT32U	REG_RX_BC;	//0xC0010020
	volatile INT32U	REG_TIMING;	//0xC0010024
	volatile INT32U	REG_OTHER;	//0xC0010028
	volatile INT32U	REG_CTRL;	//0xC001002C
	volatile INT32U	REG_SA;		//0xC0010030
} spifcReg_t;
#endif

typedef enum
{
	COMMAND_ONLY,
	COMMAND_ADDR,
	COMMAND_ADDR_TX,
	COMMAND_ADDR_RX,
	COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX,
	COMMAND_TX,
	COMMAND_RX,
	COMMAND_MAX
} COMMAND_MODE;

#ifndef _ENUM_SPI_IO_NUMBER
#define _ENUM_SPI_IO_NUMBER
typedef enum
{
	IO_1BIT,
	IO_2BIT,
	IO_4BIT,
	IO_MAX
} SPI_IO_NUMBER;
#endif

typedef struct spifcCtrl_s
{
	INT8U set_ahb_en;		/* only change ahb access mode enable */
	INT8U command_mode;		/* command mode as COMMAND_MODE */
	INT8U command;			/* spi flash command */
	INT8U enhance_data;		/* enhance mode data, 0x5A, 0xA5, 0xF0, 0x0F, 0x55, 0xAA, 0xFF */
	INT8U dummy_clk;		/* dummy clock cycle number */
	INT8U cmd_mio;			/* multi io number for command state, IO_NUMBER */
	INT8U addr_mio;			/* multi io number for address state, IO_NUMBER */
	INT8U data_mio;			/* multi io number for data state, IO_NUMBER */
	INT8U dtr_en;			/* DTR enable */

	INT32U spi_addr;		/* spi flash address */
	INT8U *tx_buf;			/* transfer buffer */
	INT32U tx_cblen;		/* transfer buffer length in byte */
	INT32U *rx_buf;			/* receive buffer */
	INT32U rx_cblen;		/* receive buffer length in byte */
} spifcCtrl_t;

typedef enum
{
	SPIFC_IOD_0_5_ENUM = 0x0,
	SPIFC_IOA12_IOA14_IOB3_IOD7_9_ENUM = 0x1,
	SPIFC_AUTO_ENUM = 0x2
}SPIFC_IO_ENUM;

// spifc controller
extern INT32S drv_l1_spifc_divisor_set(INT32U divisor);
extern INT32S drv_l1_spifc_clk_divisor_get(INT8U *p_clk_divisor);
extern INT32S drv_l1_spifc_clk_set(INT32U clk_rate);
extern INT32S drv_l1_spifc_clk_rate_get(INT32U *p_clk_rate);
extern INT32S drv_l1_spifc_act_clk_rate_cal(INT32U clk_rate, INT32U *p_clk_rate);

extern void drv_l1_spifc_init(INT32U speed);
//extern void drv_l1_spifc_init(void);
extern void drv_l1_spifc_set_timing(INT8U ignore_last_clk, INT8U clk_idle_level, INT32U use_neg_latch, INT8U sample_delay);
extern void drv_l1_spifc_set_cs_high_cnt(INT8U cnt);

// spi flash
extern INT32S drv_l1_spifcflash_write_enable(void);
extern INT32S drv_l1_spifcflash_write_disable(void);
extern INT32S drv_l1_spifcflash_read_id(INT8U *ID);
extern INT32U drv_l1_spifcflash_read_status(void);
extern INT32S drv_l1_spifcflash_write_status(INT32U enable, INT32U bit);
extern INT32U drv_l1_spifcflash_read_status_2(void);
extern INT32S drv_l1_spifcflash_set_quad_enable(INT32U enable);
extern INT32S drv_l1_spifcflash_wait_status(INT32U msec);
extern INT32S drv_l1_spifcflash_chip_erase(void);
extern INT32S drv_l1_spifcflash_sector_erase(INT32U addr);
extern INT32S drv_l1_spifcflash_block_erase(INT32U addr);
extern INT32S drv_l1_spifcflash_read_page(INT8U bus_width, INT32U addr, INT8U *rx_buf, INT32U cblen, INT8U cmd_type);
extern INT32S drv_l1_spifcflash_write_page(INT8U bus_width, INT32U addr, INT8U *tx_buf, INT32U cblen);
extern INT32S drv_l1_spifcflash_write_nbyte(INT8U bus_width, INT32U addr, INT8U *tx_buf, INT32U cblen);
extern INT32S drv_l1_spifc_dma_transfer(INT32U src_addr, INT32U dst_addr, INT32U byte_count, INT8U dtr_en);
extern INT32S drv_l1_spifcflash_read_nbyte(INT8U bus_width, INT32U addr, INT8U *rx_buf, INT32U cblen, INT8U cmd_type);
extern INT32S drv_l1_spifcflash_read_nbyte_by_memcpy(INT8U bus_width, INT32U addr, INT8U *rx_buf, INT32U cblen, INT8U cmd_type);

// spifc set ahb mode
extern INT32S drv_l1_spifc_set_read_mode(INT8U mio, INT8U enhance_en, INT32U dtr_en);
extern INT32S drv_l1_spifc_set_read_mode_over_16m_byte(INT8U mio, INT8U enhance_en, INT32U dtr_en, INT8U over_16m_byte);

extern INT32S drv_l1_spifcflash_read_sfdp(INT32U addr, INT8U *buf, INT16U len);
extern INT8U drv_l1_spifc_flash_is_2bit_supported(void);
extern INT8U drv_l1_spifc_flash_is_4bit_supported(void);

extern INT8U drv_l1_spifc_flash_is_fast_read_dual_output_supported(void);
extern INT8U drv_l1_spifc_flash_is_fast_read_quad_output_supported(void);
extern INT8U drv_l1_spifc_flash_is_dtr_supported(void);
extern INT32U drv_l1_spifc_flash_get_mega_density(void);

extern INT32U drv_l1_spifcflash_read_configurartion(void);
extern INT32S drv_l1_spifcflash_read_status_nbyte(INT8U *buf, INT16U len);
extern INT32S drv_l1_spifcflash_write_status_nbyte(INT8U *buf, INT16U len);
extern INT32S drv_l1_spifcflash_modify_status_bit(INT16U bit_mask, INT16U bit_value, INT8U toggle);
extern INT32S drv_l1_spifcflash_config_dc(INT8U dc);
extern INT32S drv_l1_spifcflash_get_dummy_clk_nr(spifcCtrl_t *pctrl);
extern void drv_l1_spifc_set_timing_auto(INT8U dtr_en, INT8U mio, INT32U spi_clock_rate);

extern INT32S drv_l1_spifcflash_exit_qpi(void);

extern void spifc_save_para(void); // just for debug
extern void spifc_restore_para(void); // just for debug

extern void drv_l1_spifc_set_rx_bc(unsigned int count);

extern INT32S drv_l1_spifcflash_exit_4B_address(void);
extern INT32S init_spifc(SPIFC_IO_ENUM spifc_io);
extern INT8U spifc_read_status(void);
extern void spifc_write_status(INT8U statusValue);
extern INT32S spifc_memcpy(INT8S *dst, INT8S *src, INT32U nbyte);

/////////////////////////////////////////////

#define SPIFC_READ_CMD_TYPE_FAST              0
#define SPIFC_READ_CMD_TYPE_NORMAL            1

#define SPIFC_READ_1_BIT_CMD_TYPE_FAST        1
#define SPIFC_READ_1_BIT_CMD_TYPE_NORMAL      0

#define SPIFC_READ_2_BIT_CMD_TYPE_FAST        0
#define SPIFC_READ_2_BIT_CMD_TYPE_NORMAL      1

#define SPIFC_READ_4_BIT_CMD_TYPE_FAST        0
#define SPIFC_READ_4_BIT_CMD_TYPE_NORMAL      1

#define SPIFC_READ32(x)			            (*((volatile INT32U *) (P_SPIFC_IO_MAP_ADDR+((x)*4))))
#define SPIFC_READ16(x)			            (*((volatile INT16U *) (P_SPIFC_IO_MAP_ADDR+((x)*2))))
#define SPIFC_READ8(x)			            (*((volatile INT8U *) (P_SPIFC_IO_MAP_ADDR+(x))))

#define C_SPIFC_HEADER_TAG_SPINOR           0x49495053   //SPII
#define C_SPIFC_HEADER_VERSION              0x10

#define C_SPIFC_HEADER_TAG_OFFSET           (0<<2)
#define C_SPIFC_HEADER_SRC_ADDR_OFFSET      (1<<2)
#define C_SPIFC_HEADER_DST_ADDR_OFFSET      (2<<2)
#define C_SPIFC_HEADER_RUN_ADDR_OFFSET      (3<<2)
#define C_SPIFC_HEADER_CODE_SIZE_OFFSET     (4<<2)
#define C_SPIFC_HEADER_CHECKSUM_OFFSET      (5<<2)
#define C_SPIFC_HEADER_VERSION_OFFSET       (24)
#define C_SPIFC_HEADER_PREREG_CNT_OFFSET    (25)
#define C_SPIFC_HEADER_REMAP_FLAG_OFFSET    (26)
#define C_SPIFC_HEADER_DRAM_TYPE_OFFSET     (27)
#define C_SPIFC_HEADER_BYTE_31_28           (7<<2)
#define C_SPIFC_HEADER_BUS_WIDTH_OFFSET     (28)
#define C_SPIFC_HEADER_CLK_DIV_OFFSET       (29) // bit[4], bit[1:0]
#define C_SPIFC_HEADER_RW_TYPE_OFFSET       (29) // bit[7:5]
#define C_SPIFC_HEADER_CMD_SUPPORT_OFFSET   (30) // 2 byte
#define C_SPIFC_HEADER_PREREG_START_OFFSET  (32)

#define C_SPIFC_HEADER_SUPPORT_1_BIT                   (1<<0)
#define C_SPIFC_HEADER_SUPPORT_2_BIT                   (1<<1)
#define C_SPIFC_HEADER_SUPPORT_4_BIT                   (1<<2)
#define C_SPIFC_HEADER_SUPPORT_1_BIT_FAST_READ         (1<<3)  // Command 0x0B
#define C_SPIFC_HEADER_SUPPORT_2_BIT_FAST_READ         (1<<4)  // Command 0x3B
#define C_SPIFC_HEADER_SUPPORT_2_BIT_READ              (1<<5)  // Command 0xBB
#define C_SPIFC_HEADER_SUPPORT_4_BIT_FAST_READ         (1<<6)  // Command 0x6B
#define C_SPIFC_HEADER_SUPPORT_4_BIT_READ              (1<<7)  // Command 0x6B
#define C_SPIFC_HEADER_SUPPORT_NO_QUAD_BIT             (1<<8)  // no Quad bit
#define C_SPIFC_HEADER_SUPPORT_4_BYTE_ADDRESS          (1<<9)  // 4 byte address

#define C_SPIFC_BLOCK_SIZE                    (65536)

/////////////////////////////////////////////
#define CMD_READ_SECURITY_REG			0x48
#define CMD_READ_UID					0x4B

#define SECURITY_REGISTER_ADDRESS(x)		((x == 0)?0x00001000:(x==1)?0x00010000:(x==2)?0x00011000:0x00001000)

#define SECURITY_REGISTER_SIZE			1024

#endif 		// __drv_l1_SPIFC_H__

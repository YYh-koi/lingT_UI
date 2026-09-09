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

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#ifndef C_DEBUG_SPIFC
#define C_DEBUG_SPIFC
#endif

#ifndef SPIFC_DBG_PRINT
#ifdef C_DEBUG_SPIFC
#define SPIFC_DBG_PRINT(args...) print_string(args)
#else
#define SPIFC_DBG_PRINT(args...)
#endif
#endif

#define C_DISABLE_SFDP                      0

#define C_VENDOR_WINBOND        0xef
#define C_VENDOR_MXIC           0xc2
#define C_VENDOR_MICRON         0x20
#define C_VENDOR_GIGADEVICE     0xc8

#define C_SFDP_QER_TYPE_0       0
#define C_SFDP_QER_TYPE_2       2
#define C_SFDP_QER_TYPE_4       4
#define C_SFDP_QER_TYPE_7       7 // reserve

#define DMA_PROG_EN		0            // dont turn this on
#define SPIFC_TIMEOUT	300000
#define NEW_HW			0            // dont turn this on

#define C_SPIFC_IDLE	(1 << 0)
#define C_CLK_STATE_HI	(1 << 1)
#define C_MANUAL_MODE	(1 << 8)
#define C_IGNORE_CLK	(1 << 9)
#define C_PROGRAM_START	(1 << 10)
#define C_PROG_DONE		(1 << 13)
#define C_RX_EMPTY		(1 << 14)
#define C_TXB_DONE		(1 << 15)

#define C_WITHOUT_CMD	(1 << 8)
#define C_CMD_ONLY		(1 << 9)
#define C_ONE_CMD		(1 << 13)

#define C_WITHOUT_ADDR	(1 << 12)
#define C_UPTO_ADDR		(1 << 13)
#define C_WITHOUT_ENHAN	(1 << 14)

#define C_LONG_RX_BC_EN (1<<7) // GP328XXXA start support

// spi flash command
#define CMD_WRITE_EN		0x06
#define CMD_WRITE_DIS		0x04
#define CMD_READ_SFDP       0x5A
#define CMD_READ_ID			0x9F
#define CMD_READ_STATUS		0x05
#define CMD_WRITE_STATUS	0x01

#define CMD_READ_CONFIGURATION		0x15 // for MXIC, for Winbond is CMD_READ_STATUS_3, but bit definition not same
#define CMD_WRITE_STATUS_3	0x11    // write status 3 that read by 0x15 command code

#define CMD_READ_STATUS_2	0x35	//for W25Q32JV-DTR
#define CMD_WRITE_STATUS_2	0x31	//for W25Q32JV-DTR, write status that read by comamnd 0x35

#define CMD_SECTOR_ERASE	0x20
#define CMD_BLOCK_ERASE		0xD8	//0x52
#define CMD_CHIP_ERASE		0x60	//0xC7
#define CMD_PAGE_READ		0x03
#define CMD_FAST_READ		0x0B
#define CMD_FAST_READ4B		0x0C
#define CMD_FAST_READ_2X    0x3B    // DUAL OUTPUT FAST READ
#define CMD_FAST_READ_4X    0x6B    // QUAD OUTPUT FAST READ
#define CMD_PAGE_READ_2X	0xBB
#define CMD_PAGE_READ_4X	0xEB
#define CMD_PAGE_PROGRAM	0x02
#define CMD_PAGE_PROGRAM_4X_MXIC	0x38 // (1,4,4), MXIC, MICRON could support
#define CMD_FAST_PAGE_PROG4B_4X 0x34   // (1,1,4)
#define CMD_PAGE_PROGRAM_4X 0x32    // (1,1,4), MICRON, WINBOND, GIGADEVICE could support
#define CMD_EN4B			0xB7	/*Enter 4byte address mode*/
#define CMD_EX4B			0xE9	/*Exit 4byte address mode*/
#define CMD_RST_ENA			0x66
#define CMD_RST_DEV			0x99

#define CMD_PAGE_READ_4X_DTR	0xED
#define CMD_PAGE_READ4B_4X_DTR  0xEE

#define CMD_PAGE_READ4B		0x13
#define CMD_PAGE_READ4B_2X	0xBC
#define CMD_PAGE_READ4B_4X	0xEC
#define CMD_PAGE_PROG4B		0x12
#define CMD_PAGE_PROG4B_4X	0x3E // (1,4,4)
#define CMD_SECTOR_ERASE4B	0x21
#define CMD_BLOCK_ERASE4B	0xDC

// spi flash status bit
/*Winbond series spi flash*/
#define C_CMP_BIT			(1 << 14)
#define C_LB3_BIT			(1 << 13)
#define C_LB2_BIT			(1 << 12)
#define C_LB1_BIT			(1 << 11)
#define C_QUAD_BIT_WB		(1 << 9)
#define ENHANCE_EN_VALUE_WB	    0x20
#define ENHANCE_DIS_VALUE_WB	0x00

#define C_SRWD_BIT			(1 << 7)
#define C_QUAD_BIT			(1 << 6)
// spi flash enhance mode
#define ENHANCE_EN_VALUE	0x5A
#define ENHANCE_DIS_VALUE	0xAA

#define C_BP3_BIT			(1 << 5)
#define C_BP2_BIT			(1 << 4)
#define C_BP1_BIT			(1 << 3)
#define C_BP0_BIT			(1 << 2)
#define C_WIP_BIT			(1 << 0)



/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define CLEAR_SPIFC_CTRL(ctrl) \
{ \
	ctrl.set_ahb_en = 0; \
	ctrl.command_mode = 0; \
	ctrl.command = 0; \
	ctrl.enhance_data = 0; \
	ctrl.dummy_clk = 0; \
	ctrl.cmd_mio = IO_1BIT; \
	ctrl.addr_mio = IO_1BIT; \
	ctrl.data_mio = IO_1BIT; \
	ctrl.spi_addr = 0; \
	ctrl.tx_buf = 0; \
	ctrl.tx_cblen = 0; \
	ctrl.rx_buf = 0; \
	ctrl.rx_cblen = 0; \
	ctrl.dtr_en = 0; \
}

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
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

typedef struct BkupReg_s
{
	INT32U	REG_SPI;
	INT32U	REG_CMD;
	INT32U	REG_PARA;
	INT32U	REG_CTRL;
	INT32U	REG_TIMING;
} BkupReg_t;

typedef struct spifc_flash_cap_s
{
    unsigned int two_bit_support : 1;
    unsigned int four_bit_support : 1;
    unsigned int fast_read_dual_output_support : 1; // 1-1-2
    unsigned int fast_read_quad_output_support : 1; // 1-1-4
    unsigned int dtr_support : 1;
    unsigned int qer_type : 3; // Quad enable requirement

    unsigned int addr_type : 2;
    unsigned int reserve: 14;

    unsigned int enhance_en_value : 8;
    unsigned int enhance_dis_value : 8;
} spifc_flash_cap_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
 extern void DMA_IRQHandler(void);
/*
#ifndef __CS_COMPILER__
#pragma arm section rwdata="spifc_section", code="spifc_section"
#else
void drv_l1_spifc_init(INT32U speed) __attribute__ ((section(".spifc_section")));
void drv_l1_spifc_set_timing(INT8U ignore_last_clk, INT8U clk_idle_level, INT32U use_neg_latch, INT8U sample_delay) __attribute__ ((section(".spifc_section")));

static void msec_wait(INT32U msec) __attribute__ ((section(".spifc_section")));
static INT32S drv_l1_spifc_wait_flag(INT32U flag) __attribute__ ((section(".spifc_section")));
static INT32S drv_l1_spifc_wait_fifo_not_empty(void) __attribute__ ((section(".spifc_section")));
static INT32S drv_l1_spifc_set_mode(spifcCtrl_t *pCtrl) __attribute__ ((section(".spifc_section")));
static void drv_l1_spifc_save_para(BkupReg_t *backup) __attribute__ ((section(".spifc_section")));
static void drv_l1_spifc_restore_para(BkupReg_t *backup) __attribute__ ((section(".spifc_section")));

INT32S drv_l1_spifcflash_write_enable(void) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_write_disable(void) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_read_id(INT8U *ID) __attribute__ ((section(".spifc_section")));
INT32U drv_l1_spifcflash_read_status(void) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_write_status(INT32U enable, INT32U bit) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_set_quad_enable(INT32U enable) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_wait_status(INT32U msec) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_chip_erase(void) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_sector_erase(INT32U addr) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_block_erase(INT32U addr) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_read_page(INT8U mio, INT32U addr, INT8U *rx_buf, INT32U cblen, INT8U cmd_type) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_write_page(INT8U mio, INT32U addr, INT8U *tx_buf, INT32U cblen) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifc_set_read_mode(INT8U mio, INT8U enhance_en, INT32U dtr_en) __attribute__ ((section(".spifc_section")));
#endif
*/

INT32S spifc_parse_sfdp(void);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT8U spifc_flash_manuId = 0;
static INT16U spifc_flash_deviceId = 0;
static INT32U spifc_flash_mega_density = 0;

// init spifc_flash_cap to default value
// please refer JEDEC SFDP spec JESD216D to fill value
spifc_flash_cap_t spifc_flash_cap =
{
    .two_bit_support = 0,
    .four_bit_support = 0,
    .fast_read_dual_output_support = 0,
    .fast_read_quad_output_support = 0,
    .dtr_support = 0,
    .qer_type = C_SFDP_QER_TYPE_2,  // QE is bit 6 of status register 1

    .addr_type = 0, // 3 byte only
    .reserve = 0,

    .enhance_en_value = ENHANCE_EN_VALUE,
    .enhance_dis_value = ENHANCE_DIS_VALUE,
};

INT8U drv_l1_spifc_flash_is_2bit_supported(void)
{
    return spifc_flash_cap.two_bit_support;
}

INT8U drv_l1_spifc_flash_is_4bit_supported(void)
{
    return spifc_flash_cap.four_bit_support;
}

INT8U drv_l1_spifc_flash_is_fast_read_dual_output_supported(void)
{
    return spifc_flash_cap.fast_read_dual_output_support;
}

INT8U drv_l1_spifc_flash_is_fast_read_quad_output_supported(void)
{
    return spifc_flash_cap.fast_read_quad_output_support;
}

INT8U drv_l1_spifc_flash_is_dtr_supported(void)
{
    return spifc_flash_cap.dtr_support;
}
INT32U drv_l1_spifc_flash_get_mega_density(void)
{
    return spifc_flash_mega_density;
}

// return density in mega bit
INT32U spifc_cal_density_using_id(INT8U id)
{
    INT16U N = id + 3; // the plus 3 mean byte x 8 = bit
    INT32U density = 1; // 1 mega bit

    if (N > 20) {
        N -= 20;
        for (;N != 0; N--)
            density <<= 1;
    }
    return density;
}

/**
 * @brief   set spifc clock divisor value
 * @param   divisor: spifc clock divisor value
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_spifc_divisor_set(INT32U divisor)
{
    INT32U reg = R_SYSTEM_PLLEN;

    reg &= ~(((INT32U)0x3F) << 9);
	reg |= ((divisor & 0x3F) << 9);
    R_SYSTEM_PLLEN = reg;

    return STATUS_OK;
}

/**
 * @brief   get spifc clock divisor value
 * @param   p_clk_divisor: spifc clock divisor value be returned
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_spifc_clk_divisor_get(INT8U *p_clk_divisor)
{
    INT32U reg = R_SYSTEM_PLLEN;

    reg >>= 9;
    reg &= 0x3F;

    *p_clk_divisor = reg;

    return STATUS_OK;
}

/**
 * @brief   set spifc clock speed for master mode.
 * @param   clk_rate: spifc clock speed
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_spifc_clk_set(INT32U clk_rate)
{
    spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
    INT32U div;

    pSpifcReg->REG_CTRL &= ~(1 << 0); // disable SPIFC module
	R_SYSTEM_PLLEN &= ~(1<<15); // disable SPIFC clock

    div = (((MCLK*1) / clk_rate) >> 1) - 1;
    if (div == 0)
        div = 1;
    if (div > 63)
        div = 63;

    drv_l1_spifc_divisor_set(div);


    // enable SPIFC clock source
    R_SYSTEM_PLLEN |= (1<<15);

    // delay some ns to wait clock effected normlly ?
    {
        INT32U k;

        for (k = 0; k < 2; k++)
            __asm volatile("nop");
    }

	// enable SPIFC module
	pSpifcReg->REG_CTRL |= (1 << 0);

	return STATUS_OK;
}

/**
 * @brief   get spifc clock speed for master mode.
 * @param   p_clk_rate: spifc current clock speed be returned
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_spifc_clk_rate_get(INT32U *p_clk_rate)
{
	INT8U clk_divisor;

	drv_l1_spifc_clk_divisor_get(&clk_divisor);
	if (clk_divisor == 0)
        clk_divisor = 1;
	clk_divisor = (clk_divisor + 1) * 2;

	*p_clk_rate = (MCLK*1)/clk_divisor;

	return STATUS_OK;
}

INT32S drv_l1_spifc_act_clk_rate_cal(INT32U clk_rate, INT32U *p_clk_rate)
{
    INT32U div;
    INT8U clk_divisor;

    div = (((MCLK*1) / clk_rate) >> 1) - 1;
    if (div > 63)
        div = 63;
    clk_divisor = (div + 1) * 2;
    *p_clk_rate = (MCLK*1)/clk_divisor;

	return STATUS_OK;
}

/**
 * @brief   SPIFC init
 * @param   speed[in]: clock speed
 * @return 	none
 */
void drv_l1_spifc_init(INT32U speed)
{
    INT32U act_speed;

    drv_l1_spifc_set_cs_high_cnt(0);
    drv_l1_spifc_clk_set(speed);
    drv_l1_spifc_clk_rate_get(&act_speed);
    drv_l1_spifc_set_timing_auto(0, 0, speed);

    SPIFC_DBG_PRINT("spifc target rate: %u, act rate %u\r\n", speed, act_speed);
}

/**
 * @brief   set SPIFC timing
 * @param   ignore_last_clk: spi clock ignor last clock for some special flash use
 * @param   clk_idle_level: spi clock pin idle state
 * @param	use_neg_latch: 0: latch time controlled by sample_delay
 *                         1: latch time is controlled by sample_delay and 1/4 clock earlier
 * @param   sample_delay: receive data, spi clock delay n halt T.
 * @return 	none
 */
void drv_l1_spifc_set_timing(INT8U ignore_last_clk, INT8U clk_idle_level, INT32U use_neg_latch, INT8U sample_delay)
{
	spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
	INT32U reg_spi = pSpifcReg->REG_SPI;
    INT32U reg_timing;

	if(ignore_last_clk) {
		reg_spi |= C_IGNORE_CLK;
	} else {
		reg_spi &= ~C_IGNORE_CLK;
	}

	if(clk_idle_level) {
		reg_spi |= C_CLK_STATE_HI;
	} else {
		reg_spi &= ~C_CLK_STATE_HI;
	}

	pSpifcReg->REG_SPI = reg_spi;

    reg_timing = pSpifcReg->REG_TIMING;
	if(use_neg_latch){
	    reg_timing &= (~0x07);
        reg_timing |= ((1 << 4) | (sample_delay & 0x07));
	} else {
        reg_timing &= (~0x17);
	    reg_timing |= (sample_delay & 0x07);
	}
    pSpifcReg->REG_TIMING = reg_timing;
}

/**
 * @brief   auto set best timing for read
 * @param   dtr_en: 0 is dtr off, 1 is dtr on
 * @param   mio: number of bits of rx bus
 * @param	spi_clock_rate: actual spi clock rate
 * @return 	none
 */
void drv_l1_spifc_set_timing_auto(INT8U dtr_en, INT8U mio, INT32U spi_clock_rate)
{
    if (dtr_en && !spifc_flash_cap.dtr_support) {
        dtr_en = 0;
        DBG_PRINT("drv_l1_spifc_set_timing_auto: device not support DTR, off the DTR mode\r\n");
    }

    if (dtr_en) {
        if (spi_clock_rate > 43000000)
            drv_l1_spifc_set_timing(0, 0, 0, 2);
        else if (spi_clock_rate >= 27750000)
            drv_l1_spifc_set_timing(0, 0, 1, 2);
        else if (spi_clock_rate >= 21375000)
            drv_l1_spifc_set_timing(0, 0, 0, 1);
        else
            drv_l1_spifc_set_timing(0, 0, 0, 1);
    }
    else {
        if (mio == IO_4BIT) {
            if (spi_clock_rate > 43000000)
                drv_l1_spifc_set_timing(0, 0, 0, 2);
            else if (spi_clock_rate >= 27750000)
                drv_l1_spifc_set_timing(0, 0, 1, 2);
            else if (spi_clock_rate >= 21375000)
                drv_l1_spifc_set_timing(0, 0, 0, 1);
            else
                drv_l1_spifc_set_timing(0, 0, 0, 0);
        }
        else {
            if (spi_clock_rate > 43000000)
                drv_l1_spifc_set_timing(0, 0, 0, 2);
            else if (spi_clock_rate >= 27750000)
                drv_l1_spifc_set_timing(0, 0, 1, 2);
            else if (spi_clock_rate >= 21375000)
                drv_l1_spifc_set_timing(0, 0, 0, 1);
            else
                drv_l1_spifc_set_timing(0, 0, 0, 0);
        }
    }
}

/**
 * @brief   set CS keeps high for duration (2x(cnt+1)) PHY clock.
 * @param   cnt: 0 ~ 7
 * @return 	none
 */
void drv_l1_spifc_set_cs_high_cnt(INT8U cnt)
{
    spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
    INT32U reg_timing;

    if (cnt > 7)
        cnt = 7;
    reg_timing = pSpifcReg->REG_TIMING;
    reg_timing &= (~(7<<8));
    reg_timing |= (cnt<<8);
    pSpifcReg->REG_TIMING = reg_timing;
}

/**
 * @brief   wait SPIFC flag
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
static INT32S drv_l1_spifc_wait_flag(INT32U flag)
{
    spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
    INT8U data_mio;
    INT32S i;

    data_mio = (pSpifcReg->REG_SPI >> 2) & 0x03;

	for(i=0; i<SPIFC_TIMEOUT; i++) {
        if((pSpifcReg->REG_SPI & flag) == flag) {
            // this is workaround, could not take off, or else DMA_PROG_EN on MMU on will hang up.
            if (flag == C_PROG_DONE || flag == C_TXB_DONE) {
                drv_tiny_wait(180); // 200 us, time depend on spifc clock rate, 200us is for 4MHz, 4 bit. If concern could fine tune this value according to spifc clock rate.
                if (data_mio != 2) {
                    // 1, 2 bit need delay more
                    drv_tiny_wait(180); // 200 us
                    drv_tiny_wait(180); // 200 us
                }
            }
			break;
		}
	} // end for

	if(i == SPIFC_TIMEOUT) {
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

/**
 * @brief   wait SPIFC rx fifo not empty
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
static INT32S drv_l1_spifc_wait_fifo_not_empty(void)
{
	spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
	INT32S i;

	for(i=0; i<SPIFC_TIMEOUT; i++) {
		if((pSpifcReg->REG_SPI & C_RX_EMPTY) == 0) {
			break;
		}
	}

	if(i == SPIFC_TIMEOUT) {
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

/**
 * @brief   spifc use dma mode to do read
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifc_dma_transfer(INT32U src_addr, INT32U dst_addr, INT32U byte_count, INT8U dtr_en)
{
    spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
	DMA_STRUCT dma_struct={0};
	volatile INT8S done;
	INT32S status;
    INT8U is_read;
    INT16U max_BC_count;
    INT32U save_RX_BC;
    INT8U align_byte;
    INT8U align_s_addr;
    INT8U align_t_addr;
    INT32U rx_bc;
    INT8U data_mio;

    data_mio = (pSpifcReg->REG_SPI >> 2) & 0x03;

    save_RX_BC = pSpifcReg->REG_RX_BC;

    is_read = ((src_addr & 0xF0000000) == P_SPIFC_IO_MAP_ADDR) ? 1 : 0;

    // when 4 bit, dtr use 32
    max_BC_count = (is_read) ? (dtr_en ? 32 : ((data_mio == 2) ? 32 : 480) ) : 256;

    rx_bc = ((byte_count > max_BC_count) ? max_BC_count : byte_count);

    pSpifcReg->REG_RX_BC = rx_bc;
    if (rx_bc > 32)
        pSpifcReg->REG_TIMING |= C_LONG_RX_BC_EN;
    else
        pSpifcReg->REG_TIMING &= ~C_LONG_RX_BC_EN;

	dma_struct.s_addr = (INT32U) src_addr;
	dma_struct.t_addr = (INT32U) dst_addr;

    align_s_addr = ((INT32U) src_addr)& 0x3;
    align_t_addr = ((INT32U) dst_addr)& 0x3;

    align_byte = byte_count & 0x3;

    if (align_byte == 0 && align_s_addr == 0 && align_t_addr == 0)
    {
        dma_struct.width = DMA_DATA_WIDTH_4BYTE;
        dma_struct.count = (byte_count >> 2);
    }
    else if (align_byte == 2 && align_s_addr == 2 && align_t_addr == 2)
    {
        dma_struct.width = DMA_DATA_WIDTH_2BYTE;
        dma_struct.count = (byte_count >> 1);
    }
    else
    {
        dma_struct.width = DMA_DATA_WIDTH_1BYTE;
        dma_struct.count = byte_count;
    }

	dma_struct.timeout = 255;
	done = C_DMA_STATUS_WAITING;
	dma_struct.notify = &done;

	status = drv_l1_dma_transfer(&dma_struct);
	if(status != 0) {
		DBG_PRINT("spifc rx dma fail %d\r\n", status);
		goto _exit;
	}
	{
		INT64U timeoutcnt = ((byte_count+255)>>8) * SPIFC_TIMEOUT;

        while (timeoutcnt != 0) {
            DMA_IRQHandler();
            if (done == C_DMA_STATUS_DONE)
                break;
            timeoutcnt--;
        }
        if (done != C_DMA_STATUS_DONE) {
            DBG_PRINT("spifc rx dma timeout\r\n");
            status = STATUS_FAIL;
            goto _exit;
        }
	}

    _exit:

    pSpifcReg->REG_RX_BC = save_RX_BC;

	return status;
}

/**
 * @brief   set SPIFC runing mode
 * @param   pCtrl: mode setting
 * @return 	STATUS_OK/STATUS_FAIL;
 */
 INT32S drv_l1_spifc_set_mode(spifcCtrl_t *pCtrl)
{
	INT8U *pData;
	INT8U dma_mode = 0;
	INT32S cnt, ret = STATUS_OK;
	INT32U *pWData;
	volatile INT32U reg_spi, reg_cmd, reg_para, reg_ctrl;
	spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
    INT32U work_around = 0;
    INT32U act_spi_clock_rate;

	// wait idle
	if(drv_l1_spifc_wait_flag(C_SPIFC_IDLE) < 0) {
        DBG_PRINT("spifc_set_mode enter wait idle fail\r\n");
		return STATUS_FAIL;
	}

    drv_l1_spifc_clk_rate_get(&act_spi_clock_rate);

    if (drv_l1_cp15_is_mmu_enable()) {
        if (act_spi_clock_rate < 4000000) { // FIFO mode: when mmu on, and spi clock rate smaller than 4MHz, could not support
            DBG_PRINT("spifc clock rate could not smaller than 4MHz. %u\r\n", act_spi_clock_rate);
            return STATUS_FAIL;
        }

        if (act_spi_clock_rate < 16000000) // FIFO mode: when mmu on, and spi clock rate smaller than 16MHz, support with work around
            work_around = 1;
    }

	// set io number
	reg_spi = pSpifcReg->REG_SPI;
	reg_spi &= ~((1 << 10) | (0x3F << 2));
	reg_spi |= ((pCtrl->cmd_mio & 0x03) << 6) |
				((pCtrl->addr_mio & 0x03) << 4) |
				((pCtrl->data_mio & 0x03) << 2);

	// set command
	reg_cmd = pSpifcReg->REG_CMD;
	reg_cmd &= ~0xFF;
	reg_cmd |= pCtrl->command & 0xFF;

	//set DTR mode
	if(pCtrl->dtr_en){
		reg_cmd |= (1<<14);
	}else{
		reg_cmd &=(~(1<<14));
	}

	// set parameter
	reg_para = pSpifcReg->REG_PARA;
	reg_para &= ~0xFFF;
	reg_para |= ((pCtrl->dummy_clk & 0x0F) << 8) |
				(pCtrl->enhance_data & 0xFF);

	// set address is 24 bit or 32bit mode
	reg_ctrl = pSpifcReg->REG_CTRL;
	if(pCtrl->spi_addr >> 24) {
		reg_ctrl |= (0xFF << 8);
		//DBG_PRINT("EAR set\r\n");
	}
	else {
		reg_ctrl &= ~(0xFF << 8);
	}

	switch(pCtrl->command_mode)
	{
	case COMMAND_ONLY:				//write enable, write disable, chip earse command
		reg_spi |= C_MANUAL_MODE;	//manual mode = 1

		reg_cmd &= ~C_WITHOUT_CMD;	//wo_cmd = 0
		reg_cmd |= C_CMD_ONLY;		//cmd_only = 1
		reg_cmd &= ~C_ONE_CMD;		//one_cmd = 0

		pCtrl->tx_cblen = 0;
		pCtrl->rx_cblen = 0;
		break;

	case COMMAND_ADDR:				//sector earse, block earse command
		reg_spi |= C_MANUAL_MODE;	//manual mode = 1

		reg_cmd &= ~C_WITHOUT_CMD;	//wo_cmd = 0
		reg_cmd &= ~C_CMD_ONLY;		//cmd_only = 0
		reg_cmd &= ~C_ONE_CMD;		//one_cmd = 0

		reg_para &= ~C_WITHOUT_ADDR;//wo_addr = 0;
		reg_para |= C_UPTO_ADDR;	//upto_addr = 1;

		pCtrl->tx_cblen = 0;
		pCtrl->rx_cblen = 0;
		break;

	case COMMAND_ADDR_TX:			//write page command
	case COMMAND_ADDR_RX:			//read page command
		reg_spi |= C_MANUAL_MODE;	//manual mode = 1

		reg_cmd &= ~C_WITHOUT_CMD;	//wo_cmd = 0
		reg_cmd &= ~C_CMD_ONLY;		//cmd_only = 0
		reg_cmd &= ~C_ONE_CMD;		//one_cmd = 0

		reg_para |= C_WITHOUT_ENHAN;//wo_enhan = 1;
		reg_para &= ~C_WITHOUT_ADDR;//wo_addr = 0;
		reg_para &= ~C_UPTO_ADDR;	//upto_addr = 0;

		if(pCtrl->command_mode == COMMAND_ADDR_TX) {
		#if (DMA_PROG_EN == 1)
			dma_mode = 1;
			reg_spi &= ~C_MANUAL_MODE;
			reg_spi |= C_PROGRAM_START;
		#endif
			pCtrl->rx_cblen = 0;
		} else {
			pCtrl->tx_cblen = 0;
		}
		break;

	case COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX:
		reg_spi |= C_MANUAL_MODE;	//manual mode = 1

		reg_cmd &= ~C_WITHOUT_CMD;	//wo_cmd = 0
		reg_cmd &= ~C_CMD_ONLY;		//cmd_only = 0
		if(pCtrl->enhance_data == spifc_flash_cap.enhance_dis_value) {
			reg_cmd &= ~C_ONE_CMD;	//one_cmd = 0, enhance disable
		} else {
			reg_cmd |= C_ONE_CMD;	//one_cmd = 1, enhance enable
		}

		reg_para &= ~C_WITHOUT_ENHAN;//wo_enhan = 0;
		reg_para &= ~C_WITHOUT_ADDR; //wo_addr = 0;
		reg_para &= ~C_UPTO_ADDR;	 //upto_addr = 0;

		pCtrl->tx_cblen = 0;
		break;

	case COMMAND_TX:				//write status command
	case COMMAND_RX:				//read status, read id command
		reg_spi |= C_MANUAL_MODE;	//manual mode = 1

		reg_cmd &= ~C_WITHOUT_CMD;	//wo_cmd = 0
		reg_cmd &= ~C_CMD_ONLY;		//cmd_only = 0
		reg_cmd &= ~C_ONE_CMD;		//one_cmd = 0

		reg_para |= C_WITHOUT_ENHAN; 	//wo_enhan = 1;
		reg_para |= C_WITHOUT_ADDR;  	//wo_addr = 1;
		reg_para &= ~C_UPTO_ADDR;	 	//upto_addr = 0;

		if(pCtrl->command_mode == COMMAND_TX) {
			pCtrl->rx_cblen = 0;
		} else {
			pCtrl->tx_cblen = 0;
		}
		break;

	default:
		ret = STATUS_FAIL;
		DBG_PRINT("spifc_set_mode ill command mode\r\n");
		goto __exit;
	}

	// for ahb mode or dma tx mode, disable manual mode.
	if(pCtrl->set_ahb_en || dma_mode) {
		reg_spi &= ~C_MANUAL_MODE;	//auto mode
	}

    // actually should should critical section
    // wait idle
    if(drv_l1_spifc_wait_flag(C_SPIFC_IDLE) < 0) {
	    DBG_PRINT("spifc_set_mode wait idle fail\r\n");
	}

    pSpifcReg->REG_SPI |= C_MANUAL_MODE; // for safe, set to manual mode to avoid SPIFC controller doing things while we configure
    pSpifcReg->REG_TX_BC = 0;
    pSpifcReg->REG_RX_BC = 0;

	// clear register
	pSpifcReg->REG_SPI |= C_TXB_DONE;

	// set spi flash addr
	pSpifcReg->REG_ADDRL = pCtrl->spi_addr & 0xFFFF;
	pSpifcReg->REG_ADDRH = (pCtrl->spi_addr >> 16) & 0xFFFF;

	// set tx buffer dma start address
    pSpifcReg->REG_SA = (INT32U)pCtrl->tx_buf;

	// set spi flash address bit number
	pSpifcReg->REG_CTRL = reg_ctrl;

    // transcation size
	pSpifcReg->REG_TX_BC = pCtrl->tx_cblen;
    pSpifcReg->REG_RX_BC = pCtrl->rx_cblen;

	// set enhance
	pSpifcReg->REG_PARA = reg_para;

	// set spifc ctrl
	pSpifcReg->REG_SPI = reg_spi;

    if (pCtrl->rx_cblen > 32)
        pSpifcReg->REG_TIMING |= C_LONG_RX_BC_EN;
    else
        pSpifcReg->REG_TIMING &= ~C_LONG_RX_BC_EN;

	// set command, then start transcation
	pSpifcReg->REG_CMD = reg_cmd;

	// transcation
	if(pCtrl->tx_cblen && pCtrl->tx_buf) {
		if(dma_mode) {
			// wait prog_done = 1
#if NEW_HW == 1
			if(drv_l1_spifc_wait_flag(C_TXB_DONE) < 0) {
#else
			if(drv_l1_spifc_wait_flag(C_PROG_DONE) < 0) {
#endif

				ret = STATUS_FAIL;
				SPIFC_DBG_PRINT("spifc tx dma fail. tx_cblen %u\r\n", pCtrl->tx_cblen);
				goto __exit;
			}
		}
		else {
			// CPU polling mode
			cnt = pCtrl->tx_cblen;
			pData = pCtrl->tx_buf;
			while(cnt > 0) {
				pSpifcReg->REG_TX_WD = *pData;
				pData++;
				cnt--;
				// wait idle = 1 && txb_done = 1
				if(drv_l1_spifc_wait_flag(C_TXB_DONE|C_SPIFC_IDLE) < 0) {
					ret = STATUS_FAIL;
					SPIFC_DBG_PRINT("spifc tx cpu poll fail. tx_cblen %u\r\n", pCtrl->tx_cblen);
					goto __exit;
				}

				// clear txb_done flag
				pSpifcReg->REG_SPI |= C_TXB_DONE;
			}
		}
	}

	if (pCtrl->rx_cblen) {
		cnt = (pCtrl->rx_cblen + 3) >> 2; //div4 and 4 align
		pWData = pCtrl->rx_buf;

		if (work_around)
            drv_tiny_wait(3); // 4 us

		while(cnt > 0) {
			// wait idle = 1 & rx_empty = 0

			if(drv_l1_spifc_wait_flag(C_SPIFC_IDLE) < 0) {
                SPIFC_DBG_PRINT("C_SPIFC_IDLE fail 2, cnt %d\r\n", cnt);
				ret = STATUS_FAIL;
				goto __exit;
			}

			if(drv_l1_spifc_wait_fifo_not_empty() < 0) {
                SPIFC_DBG_PRINT("wait_fifo_not_empty. rx_cblen=%u, tx_cblen=%u\r\n", pCtrl->rx_cblen, pCtrl->tx_cblen);
				ret = STATUS_FAIL;
				goto __exit;
			}

			// Must write dummy byte to push data into fifo
            pSpifcReg->REG_RX_RD = 0x0;

			// then read the rx data
			*pWData = pSpifcReg->REG_RX_RD;
			pWData++;
			cnt--;
		}
	}

__exit:
	return ret;
}

/**
 * @brief   save spifc parameter
 * @param   none
 * @return 	none
 */
static void drv_l1_spifc_save_para(BkupReg_t *backup)
{
	spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;

	// back up register value
	backup->REG_SPI = pSpifcReg->REG_SPI;
	backup->REG_CMD = pSpifcReg->REG_CMD;
	backup->REG_PARA = pSpifcReg->REG_PARA;
	backup->REG_CTRL = pSpifcReg->REG_CTRL;
    backup->REG_TIMING = pSpifcReg->REG_TIMING;
}

/**
 * @brief   restore spifc parameter
 * @param   none
 * @return 	none
 */
static void drv_l1_spifc_restore_para(BkupReg_t *backup)
{
	spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;

	// recovery register value
    // actually should should critical section
    // wait idle
	if(drv_l1_spifc_wait_flag(C_SPIFC_IDLE) < 0) {
	    DBG_PRINT("spifc_restore_para wait idle fail\r\n");
	}
    pSpifcReg->REG_SPI |= C_MANUAL_MODE; // for safe, set to manual mode to avoid SPIFC controller doing things while we configure
    pSpifcReg->REG_TX_BC = 0;
	pSpifcReg->REG_RX_BC = 0;
    pSpifcReg->REG_TIMING = backup->REG_TIMING;
	pSpifcReg->REG_SPI = backup->REG_SPI;
	pSpifcReg->REG_PARA = backup->REG_PARA;
	pSpifcReg->REG_CTRL = backup->REG_CTRL;
	pSpifcReg->REG_CMD = backup->REG_CMD;
}

/**
 * @brief   set RX_BC count
 * @param   count
 * @return 	none
 */
void drv_l1_spifc_set_rx_bc(unsigned int count)
{
	spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
    pSpifcReg->REG_RX_BC = count;
}

/**
 * @brief   set spi flash write enable
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_write_enable(void)
{
	spifcCtrl_t ctrl;

	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_ONLY;
	ctrl.command = CMD_WRITE_EN;
	return drv_l1_spifc_set_mode(&ctrl);
}

/**
 * @brief   set spi flash write disable
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_write_disable(void)
{
	spifcCtrl_t ctrl;

	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_ONLY;
	ctrl.command = CMD_WRITE_DIS;
	return drv_l1_spifc_set_mode(&ctrl);
}

/**
 * @brief   read spi flash id
 * @param   ID: id buffer pointer
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_read_id(INT8U *ID)
{
	static ALIGN64 INT8U buf[4]; // for byte, compiler assume align to 4 byte ?
	INT32S ret;
	spifcCtrl_t ctrl;
	BkupReg_t backup;
    INT32U act_spi_clock_rate;

	drv_l1_spifc_save_para(&backup);
    drv_l1_spifc_clk_rate_get(&act_spi_clock_rate);
    drv_l1_spifc_set_timing_auto(0, IO_1BIT, act_spi_clock_rate);
	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_RX;
	ctrl.command = CMD_READ_ID;
	ctrl.rx_buf = (INT32U *)buf;
	ctrl.rx_cblen = 4;	//read length is 4 align
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		goto __exit;
	}

	ID[0] = buf[0];
	ID[1] = buf[1];
	ID[2] = buf[2];
	ret = STATUS_OK;
__exit:
	drv_l1_spifc_restore_para(&backup);
	return ret;
}

/**
 * @brief   read spi flash status
 * @param   none
 * @return 	status or 0;
 */
INT32U drv_l1_spifcflash_read_status(void)
{
    static ALIGN64 INT8U buf[4]; // for byte, compiler assume align to 4 byte ?
	INT32S ret;
	spifcCtrl_t ctrl;

	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_RX;
	ctrl.command = CMD_READ_STATUS;
	ctrl.rx_buf = (INT32U *)buf;
	ctrl.rx_cblen = 1;	//read length is 4 align
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		ret = 0;
		goto __exit;
	}

	ret = (INT32U)buf[0];
__exit:

	return ret;
}

/**
 * @brief   read spi flash status s15-s8
 * @param   none
 * @return 	status or 0;
 */
INT32U drv_l1_spifcflash_read_status_2(void)
{
	static ALIGN64 INT8U buf[4];
	INT32S ret;
	spifcCtrl_t ctrl;

	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_RX;
	ctrl.command = CMD_READ_STATUS_2;
	ctrl.rx_buf = (INT32U *)buf;
	ctrl.rx_cblen = 4;	//read length is 4 align
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		ret = 0;
		goto __exit;
	}

	ret = (INT32U)buf[0];
__exit:

	return ret;
}

/**
 * @brief   write spi flash status
 * @param   enable: enable or disable
 * @param   bit: bit
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_write_status(INT32U enable, INT32U bit)
{
	INT32S ret;
	INT32U status;
	spifcCtrl_t ctrl;
	INT8U sbit;

	if (bit & 0xFF) {
		sbit = bit & 0xFF;
        // read status and check bit is aready set or not
        status = drv_l1_spifcflash_read_status();
            if(enable && (status & sbit) ) {
            ret = STATUS_OK;
            goto __exit;
        }

		if((enable == 0) && ((status & sbit) == 0)) {
            ret = STATUS_OK;
            goto __exit;
        }

        // set bit
        if(enable) {
            status |= sbit;
        } else {
            status &= ~sbit;
        }

        ret = drv_l1_spifcflash_write_enable();
        if(ret < 0) {
            goto __exit;
        }

        CLEAR_SPIFC_CTRL(ctrl);
        ctrl.command_mode = COMMAND_TX;
        ctrl.command = CMD_WRITE_STATUS;
        ctrl.tx_buf = (INT8U *)&status;
        ctrl.tx_cblen = 1;
        ret = drv_l1_spifc_set_mode(&ctrl);
        if(ret < 0) {
            goto __exit;
        }

        // wait program finish
        ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
	}
	else {
		sbit = (bit>>8) & 0xFF;
		// read status and check bit is aready set or not
		status = drv_l1_spifcflash_read_status_2();
		if(enable && (status & sbit) ) {
			ret = STATUS_OK;
			goto __exit;
		}

		if((enable == 0) && ((status & sbit) == 0)) {
			ret = STATUS_OK;
			goto __exit;
		}

		// set bit
		if(enable) {
			status |= sbit;
		} else {
			status &= ~sbit;
		}

		ret = drv_l1_spifcflash_write_enable();
		if(ret < 0) {
			goto __exit;
		}
		CLEAR_SPIFC_CTRL(ctrl);
		ctrl.command_mode = COMMAND_TX;
		ctrl.command = CMD_WRITE_STATUS_2;
		ctrl.tx_buf = (INT8U *)&status;
		ctrl.tx_cblen = 1;

		ret = drv_l1_spifc_set_mode(&ctrl);
		if(ret < 0) {
			goto __exit;
		}

		// wait program finish
		ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
	}

__exit:
	return ret;
}

/**
 * @brief   set spi flash quad bit
 * @param   enable: enable or disable
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_set_quad_enable(INT32U enable)
{
    INT32U quad_bit = (spifc_flash_cap.qer_type == C_SFDP_QER_TYPE_4)  ? C_QUAD_BIT_WB : C_QUAD_BIT;

    // QER type 0 use command code, no QE bit, so return directly
    // GIGADEVICE GD25T512ME is C_SFDP_QER_TYPE_7, but C_SFDP_QER_TYPE_7 is reserve, GD25T512ME no QE bit, so directly return
    if (spifc_flash_cap.qer_type == C_SFDP_QER_TYPE_0 || spifc_flash_cap.qer_type == C_SFDP_QER_TYPE_7)
        return STATUS_OK;

	return drv_l1_spifcflash_write_status(enable, quad_bit);
}

/**
 * @brief   wait spi flash busy bit
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_wait_status(INT32U msec)
{
	INT32U i, status;

	for(i=0; i<msec; i++) {
		status = drv_l1_spifcflash_read_status();
		if((status & C_WIP_BIT) == 0) {
			break;
		}
		drv_tiny_wait(3); // 4us
	}

	if(i == msec) {
		return STATUS_FAIL;
	}
	return STATUS_OK;
}

/**
 * @brief   spi flash chip erase
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_chip_erase(void)
{
	INT32S ret;
	spifcCtrl_t ctrl;
	BkupReg_t backup;

	drv_l1_spifc_save_para(&backup);
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
	if(ret < 0) {
		goto __exit;
	}

	ret = drv_l1_spifcflash_write_enable();
	if(ret < 0) {
		goto __exit;
	}

	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_ONLY;
	ctrl.command = CMD_CHIP_ERASE;
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		goto __exit;
	}

	// wait earse finish
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT * 20 * 10);
__exit:
	drv_l1_spifc_restore_para(&backup);
	return ret;
}

/**
 * @brief   spi flash sector erase, a sector is 4K
 * @param   addr: sector erase start address
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_sector_erase(INT32U addr)
{
	INT32S ret;
	spifcCtrl_t ctrl;
	BkupReg_t backup;
	INT8U en4b = 0;

	drv_l1_spifc_save_para(&backup);
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
	if(ret < 0) {
		goto __exit;
	}

	addr = addr >> 12 << 12;
	//if address is beyond 16MB, enter 4byte address mode
	if(addr >> 24) {
		CLEAR_SPIFC_CTRL(ctrl);
		ctrl.command_mode = COMMAND_ONLY;
		ctrl.command = CMD_EN4B;
		//DBG_PRINT("send EN4B cmd,addr=0x%x\r\n",addr);
		ret = drv_l1_spifc_set_mode(&ctrl);
		if(ret < 0) {
			goto __exit;
		}

		ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
		if(ret < 0) {
			goto __exit;
		}
		en4b = 1;
	}


	ret = drv_l1_spifcflash_write_enable();
	if(ret < 0) {
		goto __exit;
	}

	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_ADDR;
	if(en4b){
		//DBG_PRINT("CMD_SECTOR_ERASE4B\r\n");
		ctrl.command = CMD_SECTOR_ERASE4B;
	}
	else
		ctrl.command = CMD_SECTOR_ERASE;
	ctrl.spi_addr = addr >> 12 << 12;
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		goto __exit;
	}

	// wait earse finish
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);

	if(en4b == 1){
		CLEAR_SPIFC_CTRL(ctrl);
		ctrl.command_mode = COMMAND_ONLY;
		ctrl.command = CMD_EX4B;
		//DBG_PRINT("send EX4B cmd\r\n");
		ret = drv_l1_spifc_set_mode(&ctrl);
		if(ret < 0) {
			goto __exit;
		}

		ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
		if(ret < 0) {
			goto __exit;
		}
	}
__exit:
	drv_l1_spifc_restore_para(&backup);
	return ret;
}

/**
 * @brief   spi flash block erase, a block is 64K
 * @param   addr: block erase start address
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_block_erase(INT32U addr)
{
	INT32S ret;
	spifcCtrl_t ctrl;
	BkupReg_t backup;
	INT8U en4b = 0;

	drv_l1_spifc_save_para(&backup);
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
	if(ret < 0) {
		goto __exit;
	}

	ret = drv_l1_spifcflash_write_enable();
	if(ret < 0) {
		goto __exit;
	}

	addr = addr >> 16 << 16;

	//if address is beyond 16MB, enter 4byte address mode
	if(addr >> 24) {
		CLEAR_SPIFC_CTRL(ctrl);
		ctrl.command_mode = COMMAND_ONLY;
		ctrl.command = CMD_EN4B;
		//DBG_PRINT("send EN4B cmd, addr=0x%x\r\n",addr);
		ret = drv_l1_spifc_set_mode(&ctrl);
		if(ret < 0) {
			goto __exit;
		}

		ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
		if(ret < 0) {
			goto __exit;
		}
		en4b = 1;
	}

	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_ADDR;
	if(en4b){
		//DBG_PRINT("CMD_BLOCK_ERASE4B\r\n");
		ctrl.command = CMD_BLOCK_ERASE4B;
	}
	else
		ctrl.command = CMD_BLOCK_ERASE;
	ctrl.spi_addr = addr >> 16 << 16;
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		goto __exit;
	}

	// wait earse finish
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);

	if(en4b == 1){
		CLEAR_SPIFC_CTRL(ctrl);
		ctrl.command_mode = COMMAND_ONLY;
		ctrl.command = CMD_EX4B;
		ret = drv_l1_spifc_set_mode(&ctrl);
		if(ret < 0) {
			goto __exit;
		}

		ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
		if(ret < 0) {
			goto __exit;
		}
	}
__exit:
	drv_l1_spifc_restore_para(&backup);
	return ret;
}

/**
 * @brief   spi flash read page, a page is 256byte
 * @param   mio: IO_1BIT, IO_2BIT or IO_4BIT
 * @param   addr: spi flash start address
 * @param   buf: read buffer
 * @param   cblen: read length in byte
 * @return 	STATUS_OK/STATUS_FAIL;
 */

 // suggest using drv_l1_spifcflash_read_nbyte, it is dma mode, drv_l1_spifcflash_read_page is fifo mode, have some limitation.

INT32S drv_l1_spifcflash_read_page(INT8U bus_width, INT32U addr, INT8U *rx_buf, INT32U cblen, INT8U cmd_type)
{
	INT32S ret;
	INT32U i, PageNum;
	spifcCtrl_t ctrl;
	BkupReg_t backup;
	INT8U en4b=0;
    INT8U quad_enable = 1; //((bus_width & 0x10) != 0) ? 1 : 0;
    INT8U mio = bus_width & 0x03;
    INT8U dtr_en = ((bus_width & 0x20) != 0) ? 1 : 0;
    INT32S dummy_clk_nr;
    INT32U act_spi_clock_rate;

    drv_l1_spifc_clk_rate_get(&act_spi_clock_rate);

    dtr_en = 0; // fifo mode DTR fail, so not support DTR mode when FIFO mode.

	drv_l1_spifc_save_para(&backup);
	CLEAR_SPIFC_CTRL(ctrl);
	switch(mio)
	{
	case IO_1BIT:
		ret = drv_l1_spifcflash_set_quad_enable(0);
		if(ret < 0) {
			goto __exit;
		}
        if (cmd_type == SPIFC_READ_1_BIT_CMD_TYPE_FAST)
        {
            //fast read
            ctrl.command_mode = COMMAND_ADDR_RX;
            ctrl.command = CMD_FAST_READ;
            ctrl.dummy_clk = 8;
        }
        else
        {
            ctrl.command_mode = COMMAND_ADDR_RX;
            ctrl.command = CMD_PAGE_READ;

            // one of the part number of MXIC have problem when 1 bit, dma mode, normal read and 85.5MHz
            // but 84MHz is ok, or 1 bit fast read and dma is ok.
            // change to use fast read to avoid the problem
            if ((spifc_flash_manuId == C_VENDOR_MXIC) && (act_spi_clock_rate >= 85500000)) {
                ctrl.command_mode = COMMAND_ADDR_RX;
                ctrl.command = CMD_FAST_READ;
                ctrl.dummy_clk = 8;
            }
        }

		break;

	case IO_2BIT:
		ret = drv_l1_spifcflash_set_quad_enable(0);
		if(ret < 0) {
			goto __exit;
		}
        if (cmd_type == SPIFC_READ_2_BIT_CMD_TYPE_FAST)
        {
            ctrl.command_mode = COMMAND_ADDR_RX;
            ctrl.command = CMD_FAST_READ_2X;
            ctrl.dummy_clk = 8;
            ctrl.cmd_mio = IO_1BIT;
            ctrl.addr_mio = IO_1BIT;
            ctrl.data_mio = IO_2BIT;
        }
        else
        {
            ctrl.command_mode = COMMAND_ADDR_RX;
            ctrl.command = CMD_PAGE_READ_2X;
            ctrl.cmd_mio = IO_1BIT;
            ctrl.addr_mio = IO_2BIT;
            ctrl.data_mio = IO_2BIT;
            ctrl.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ctrl)) < 0) ? 4 : dummy_clk_nr;
        }
		break;

	case IO_4BIT:
		// quad bit enable
		if (quad_enable) {
            ret = drv_l1_spifcflash_set_quad_enable(1);
            if(ret < 0) {
                goto __exit;
            }
        }
        if ((!dtr_en) && cmd_type == SPIFC_READ_4_BIT_CMD_TYPE_FAST)
        {
            ctrl.command_mode = COMMAND_ADDR_RX;
            ctrl.command = CMD_FAST_READ_4X;
            ctrl.dummy_clk = 8;
            ctrl.cmd_mio = IO_1BIT;
            ctrl.addr_mio = IO_1BIT;
            ctrl.data_mio = IO_4BIT;
        }
        else
        {
            ctrl.command_mode = COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX;
            ctrl.cmd_mio = IO_1BIT;
            ctrl.addr_mio = IO_4BIT;
            ctrl.data_mio = IO_4BIT;
            if(dtr_en == 1){
                ctrl.command = CMD_PAGE_READ_4X_DTR;
                //for Winbond W25Q32JV-DTR, is 7

                ctrl.dtr_en = 1;
                ctrl.enhance_data = spifc_flash_cap.enhance_dis_value;
                ctrl.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ctrl)) < 0) ? 7 : dummy_clk_nr - 1;
            }
            else {
                ctrl.command = CMD_PAGE_READ_4X;
                #if 0 //enhance enable
                ctrl.enhance_data = spifc_flash_cap.enhance_en_value;
                #else //enhance disable
                //DBG_PRINT("enhance disable\r\n");
                ctrl.enhance_data = spifc_flash_cap.enhance_dis_value;
                #endif
                ctrl.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ctrl)) < 0) ? 4 : dummy_clk_nr - 2;
            }
		}
		break;

	default:
		ret = STATUS_FAIL;
		goto __exit;
	}

	// page read start
	PageNum = cblen >> 8;
	if(PageNum == 0) {
		PageNum = 1;
	}

	ctrl.spi_addr = addr >> 8 << 8;
	ctrl.rx_buf = (INT32U *)rx_buf;
	if (cblen == 0)
        ctrl.rx_cblen = 480;
	else
        ctrl.rx_cblen = 256;
	for(i=0; i<PageNum; i++) {
		//DBG_PRINT("ctrl.spi_addr=0x%x\r\n",ctrl.spi_addr);

		if(en4b==0 && ctrl.spi_addr >> 24){
			spifcCtrl_t ctrl1;

			CLEAR_SPIFC_CTRL(ctrl1);
			ctrl1.command_mode = COMMAND_ONLY;
			ctrl1.command = CMD_EN4B;
			//DBG_PRINT("send EN4B cmd\r\n");
			ret = drv_l1_spifc_set_mode(&ctrl1);

			if(ret < 0) {
				goto __exit;
			}

			ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
			if(ret < 0) {
				goto __exit;
			}

            switch(mio) {
				case IO_1BIT:
					//DBG_PRINT("CMD_PAGE_READ4B\r\n");
					ctrl.command_mode = COMMAND_ADDR_RX;
					// CMD_PAGE_READ4B have problem use CMD_FAST_READ4B instead
					#if 0
					ctrl.command = CMD_PAGE_READ4B;
                    ctrl.dummy_clk = 0;
                    #else
                    ctrl.command = CMD_FAST_READ4B;
                    ctrl.dummy_clk = 8;
                    #endif
					break;

				case IO_2BIT:
					//DBG_PRINT("CMD_PAGE_READ4B_2X\r\n");
					ctrl.command = CMD_PAGE_READ4B_2X;
                    ctrl.cmd_mio = IO_1BIT;
                    ctrl.addr_mio = IO_2BIT;
                    ctrl.data_mio = IO_2BIT;
                    ctrl.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ctrl)) < 0) ? 4 : dummy_clk_nr;
					break;

				case IO_4BIT:
					//DBG_PRINT("CMD_PAGE_READ4B_4X\r\n");
					ctrl.command_mode = COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX;
					ctrl.command = dtr_en ? CMD_PAGE_READ_4X_DTR : CMD_PAGE_READ4B_4X;
                    ctrl.cmd_mio = IO_1BIT;
                    ctrl.addr_mio = IO_4BIT;
                    ctrl.data_mio = IO_4BIT;
                    #if 0 //enhance enable
                    ctrl.enhance_data = spifc_flash_cap.enhance_en_value;
                    #else //enhance disable
                    //DBG_PRINT("enhance disable\r\n");
                    ctrl.enhance_data = spifc_flash_cap.enhance_dis_value;
                    #endif
                    ctrl.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ctrl)) < 0) ? (dtr_en ? 7 : 4) : dummy_clk_nr - (dtr_en ? 1 : 2);
					break;

				default:
					ret = STATUS_FAIL;
					goto __exit;
            } // end switch

			en4b = 1;
		}
		//DBG_PRINT("ctrl.command=0x%x\r\n", ctrl.command);
		ret = drv_l1_spifc_set_mode(&ctrl);
		if(ret < 0) {
			goto __exit;
		}
		ctrl.spi_addr += 256;
		ctrl.rx_buf = (INT32U *)((INT32U)ctrl.rx_buf + 256);
	}

	if(en4b == 1){
		spifcCtrl_t ctrl1;

		CLEAR_SPIFC_CTRL(ctrl1);
		ctrl1.command_mode = COMMAND_ONLY;
		ctrl1.command = CMD_EX4B;
		//DBG_PRINT("send EX4B cmd\r\n");
		ret = drv_l1_spifc_set_mode(&ctrl1);
		if(ret < 0) {
			goto __exit;
		}

		ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
		if(ret < 0) {
			goto __exit;
		}
		en4b = 0;
	}
__exit:
     if (cblen == 0)
        return ret;

    if (mio == IO_4BIT) {
        if (quad_enable)
            drv_l1_spifcflash_set_quad_enable(0);
    }
    drv_l1_spifc_restore_para(&backup);
	return ret;
}

/**
 * @brief   spifc use dma mode to do read
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_read_nbyte(INT8U bus_width, INT32U addr, INT8U *rx_buf, INT32U cblen, INT8U cmd_type)
{
	INT32S ret;
	spifcCtrl_t ctrl;
	BkupReg_t backup;
	INT8U en4b=0;
    INT8U quad_enable = 1; // ((bus_width & 0x10) != 0) ? 1 : 0;
    INT8U mio = bus_width & 0x03;
	INT32S dma_ret = 0;
    INT8U dtr_en = ((bus_width & 0x20) != 0) ? 1 : 0;
    INT32S dummy_clk_nr;
    INT8U use_fifo_mode = 0;
    INT32U work_around = 0;
    INT32U act_spi_clock_rate;
    INT32U spi_addr;
    INT32U rx_len;
    INT8U *prx;
    INT32U max_page_len = 256; // use by fifo mode, maximal is 256, ever tested with 16,32,64,128
    INT32U this_page_len;
    //spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE; // DEBUG

    rx_len = cblen;
    spi_addr = addr;
    prx = rx_buf;

    drv_l1_spifc_clk_rate_get(&act_spi_clock_rate);

    if (drv_l1_cp15_is_mmu_enable()) {
        if (act_spi_clock_rate < 4000000) { // DMA mode: when mmu on, and spi clock rate smaller than 4MHz, could not support
            DBG_PRINT("spifc clock rate could not smaller than 4MHz. %u\r\n", act_spi_clock_rate);
            return STATUS_FAIL;
        }

        if (mio == IO_4BIT && act_spi_clock_rate < 37000000) // DMA mode: when mmu on, and spi clock rate smaller than 37MHz, support with work around
            work_around = 1;
    }

    if (dtr_en && !spifc_flash_cap.dtr_support) {
        dtr_en = 0;
        DBG_PRINT("device not support DTR, off the DTR mode\r\n");
    }

    if ((mio == IO_4BIT) && (!dtr_en) && (spifc_flash_manuId == C_VENDOR_MICRON) && (cblen != 0))
        use_fifo_mode = 1;

    #if 0 // dont turn this on, this is for DEBUG
    if ((!dtr_en) && (spifc_flash_manuId == C_VENDOR_MXIC) && (cblen != 0) \
         && (act_spi_clock_rate >= 57000000) \
    )
    {
        use_fifo_mode = 1;
        max_page_len = 32;
    }
    #endif

	drv_l1_spifc_save_para(&backup);
	CLEAR_SPIFC_CTRL(ctrl);
	switch(mio)
	{
	case IO_1BIT:
		ret = drv_l1_spifcflash_set_quad_enable(0);
		if(ret < 0) {
			goto __exit;
		}
        if (cmd_type == SPIFC_READ_1_BIT_CMD_TYPE_FAST)
        {
            //fast read
            ctrl.command_mode = COMMAND_ADDR_RX;
            ctrl.command = CMD_FAST_READ;
            ctrl.dummy_clk = 8;
        }
        else
        {
            ctrl.command_mode = COMMAND_ADDR_RX;
            ctrl.command = CMD_PAGE_READ;

            #if 1
            // one of the part number of MXIC have problem when 1 bit, dma mode, normal read and 85.5MHz
            // but 84MHz is ok, or 1 bit fast read and dma is ok.
            // change to use fast read to avoid the problem
            if ((spifc_flash_manuId == C_VENDOR_MXIC) && (act_spi_clock_rate >= 85500000)) {
                ctrl.command_mode = COMMAND_ADDR_RX;
                ctrl.command = CMD_FAST_READ;
                ctrl.dummy_clk = 8;
            }
            #endif
        }

		break;

	case IO_2BIT:
		ret = drv_l1_spifcflash_set_quad_enable(0);
		if(ret < 0) {
			goto __exit;
		}
        if (cmd_type == SPIFC_READ_2_BIT_CMD_TYPE_FAST)
        {
            ctrl.command_mode = COMMAND_ADDR_RX;
            ctrl.command = CMD_FAST_READ_2X;
            ctrl.dummy_clk = 8;
            ctrl.cmd_mio = IO_1BIT;
            ctrl.addr_mio = IO_1BIT;
            ctrl.data_mio = IO_2BIT;
        }
        else
        {
            ctrl.command_mode = COMMAND_ADDR_RX;
            ctrl.command = CMD_PAGE_READ_2X;
            ctrl.cmd_mio = IO_1BIT;
            ctrl.addr_mio = IO_2BIT;
            ctrl.data_mio = IO_2BIT;
            ctrl.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ctrl)) < 0) ? 4 : dummy_clk_nr;
        }
		break;

	case IO_4BIT:
		// quad bit enable
		if (quad_enable) {
            ret = drv_l1_spifcflash_set_quad_enable(1);
            if(ret < 0) {
                goto __exit;
            }
        }
        if ((!dtr_en) && cmd_type == SPIFC_READ_4_BIT_CMD_TYPE_FAST)
        {
            ctrl.command_mode = COMMAND_ADDR_RX;
            ctrl.command = CMD_FAST_READ_4X;
            ctrl.dummy_clk = 8;
            ctrl.cmd_mio = IO_1BIT;
            ctrl.addr_mio = IO_1BIT;
            ctrl.data_mio = IO_4BIT;
        }
        else
        {
            ctrl.cmd_mio = IO_1BIT;
            ctrl.addr_mio = IO_4BIT;
            ctrl.data_mio = IO_4BIT;
            ctrl.command_mode = COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX;
            if(dtr_en == 1){
                ctrl.command = CMD_PAGE_READ_4X_DTR;
                //for Winbond W25Q32JV-DTR, is 7
                ctrl.dtr_en = 1;
                ctrl.enhance_data = spifc_flash_cap.enhance_dis_value;
                ctrl.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ctrl)) < 0) ? 7 : dummy_clk_nr - 1;
            }
            else {
                ctrl.command = CMD_PAGE_READ_4X;
                #if 0 //enhance enable
                ctrl.enhance_data = spifc_flash_cap.enhance_en_value;
                #else //enhance disable
                //DBG_PRINT("enhance disable\r\n");
                ctrl.enhance_data = spifc_flash_cap.enhance_dis_value;
                #endif
                ctrl.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ctrl)) < 0) ? 4 : dummy_clk_nr - 2;
            }
		}
		break;

	default:
		ret = STATUS_FAIL;
		goto __exit;
	}

    L_NEXT_PAGE:
	ctrl.spi_addr = spi_addr;
	ctrl.rx_buf = (INT32U *)prx;
	if (use_fifo_mode) {
        this_page_len = max_page_len - (spi_addr & (max_page_len-1));
        this_page_len = (this_page_len > rx_len) ? rx_len : this_page_len;
        ctrl.rx_cblen = this_page_len;
        ctrl.set_ahb_en = 0;
	}
	else {
        ctrl.rx_cblen = 0; // set to 0 so that drv_l1_spifc_set_mode won't read, and we use dma later
        ctrl.set_ahb_en = 1;
    }
    if (cblen == 0)
        ctrl.rx_cblen = 480; // Actually should be 512, but IC have bug that allow only 511, so set to 480 to allow burst 8

		//DBG_PRINT("ctrl.spi_addr=0x%x\r\n",ctrl.spi_addr);

    if(en4b==0 && ctrl.spi_addr >> 24){
        spifcCtrl_t ctrl1;

        CLEAR_SPIFC_CTRL(ctrl1);
        ctrl1.command_mode = COMMAND_ONLY;
        ctrl1.command = CMD_EN4B;
        //DBG_PRINT("send EN4B cmd\r\n");
        ret = drv_l1_spifc_set_mode(&ctrl1);

        if(ret < 0) {
            goto __exit;
        }

        ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
        if(ret < 0) {
            goto __exit;
        }

        switch(mio) {
            case IO_1BIT:
                //DBG_PRINT("CMD_PAGE_READ4B\r\n");
                ctrl.command_mode = COMMAND_ADDR_RX;
                if (cmd_type == SPIFC_READ_1_BIT_CMD_TYPE_FAST) {
                    ctrl.command = CMD_FAST_READ4B;
                    ctrl.dummy_clk = 8;
                }
                #if 0
                else {
                    // CMD_PAGE_READ4B have problem use CMD_FAST_READ4B instead
                    #if 0
                    ctrl.command = CMD_PAGE_READ4B;
                    ctrl.dummy_clk = 0;
                    #else
                    ctrl.command = CMD_FAST_READ4B;
                    ctrl.dummy_clk = 8;
                    #endif
                }
                #endif

                break;

            case IO_2BIT:
                //DBG_PRINT("CMD_PAGE_READ4B_2X\r\n");
                ctrl.command = CMD_PAGE_READ4B_2X;
                ctrl.cmd_mio = IO_1BIT;
                ctrl.addr_mio = IO_2BIT;
                ctrl.data_mio = IO_2BIT;
                ctrl.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ctrl)) < 0) ? 4 : dummy_clk_nr;
                break;

            case IO_4BIT:
                //DBG_PRINT("CMD_PAGE_READ4B_4X\r\n");
                ctrl.command_mode = COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX;
                ctrl.command = dtr_en ? CMD_PAGE_READ_4X_DTR : CMD_PAGE_READ4B_4X;
                ctrl.cmd_mio = IO_1BIT;
                ctrl.addr_mio = IO_4BIT;
                ctrl.data_mio = IO_4BIT;
                #if 0 //enhance enable
                ctrl.enhance_data = spifc_flash_cap.enhance_en_value;
                #else //enhance disable
                //DBG_PRINT("enhance disable\r\n");
                ctrl.enhance_data = spifc_flash_cap.enhance_dis_value;
                #endif
                ctrl.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ctrl)) < 0) ? (dtr_en ? 7 : 4) : dummy_clk_nr - (dtr_en ? 1 : 2);
                break;

            default:
                ret = STATUS_FAIL;
                goto __exit;
        } // end switch

        en4b = 1;
    }

    ret = drv_l1_spifc_set_mode(&ctrl);
    if(ret < 0) {
        goto __exit;
    }

    if (cblen == 0) {
        return 0;
    }

    if (use_fifo_mode) {
        spi_addr += this_page_len;
        prx += this_page_len;
        rx_len -= this_page_len;
        if (rx_len > 0)
            goto L_NEXT_PAGE;
    }
    else {
        if (work_around)
            drv_tiny_wait(3); // 4 us

        #if 0 // DEBUG
        if(drv_l1_spifc_wait_flag(C_SPIFC_IDLE) < 0) {
            SPIFC_DBG_PRINT("C_SPIFC_IDLE fail 3, cblen %d\r\n", cblen);
            ret = STATUS_FAIL;
            goto __exit;
        }
        #endif

        dma_ret = drv_l1_spifc_dma_transfer(ctrl.spi_addr | P_SPIFC_IO_MAP_ADDR, (INT32U)ctrl.rx_buf, cblen, ctrl.dtr_en);
    }

__exit:
	if(en4b == 1){
		spifcCtrl_t ctrl1;

		CLEAR_SPIFC_CTRL(ctrl1);
		ctrl1.command_mode = COMMAND_ONLY;
		ctrl1.command = CMD_EX4B;
		//DBG_PRINT("send EX4B cmd\r\n");
		ret = drv_l1_spifc_set_mode(&ctrl1);
		if(ret < 0) {
			goto __exit;
		}

		ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
		if(ret < 0) {
			goto __exit;
		}
		en4b = 0;
	}

    if (mio == IO_4BIT) {
        if (quad_enable)
            drv_l1_spifcflash_set_quad_enable(0);
    }
	drv_l1_spifc_restore_para(&backup);

	if (dma_ret != 0)
        ret = dma_ret;

	return ret;
}

/**
 * @brief   spi flash write page, a page is 256byte
 * @param   mio: IO_1BIT or IO_4BIT
 * @param   addr: spi flash start address, must 256 multiple
 * @param   buf: write buffer
 * @param   cblen: write length in byte, must 256 multiple
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_write_page(INT8U bus_width, INT32U addr, INT8U *tx_buf, INT32U cblen)
{
	INT32S ret;
	INT32U i, PageNum;
	spifcCtrl_t ctrl;
	BkupReg_t backup;
	INT8U en4b = 0;
    INT8U quad_enable = 1; //((bus_width & 0x10) != 0) ? 1 : 0;
    INT8U mio = bus_width & 0x03;

	drv_l1_spifc_save_para(&backup);
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
	if(ret < 0) {
		goto __exit;
	}

	CLEAR_SPIFC_CTRL(ctrl);
	switch(mio)
	{
	case IO_1BIT:
		ret = drv_l1_spifcflash_set_quad_enable(0);
		if(ret < 0) {
			goto __exit;
		}
		ctrl.command_mode = COMMAND_ADDR_TX;
		ctrl.command = CMD_PAGE_PROGRAM;
		break;

	case IO_4BIT:
		// quad bit enable
		if (quad_enable) {
            ret = drv_l1_spifcflash_set_quad_enable(1);
            if(ret < 0) {
                goto __exit;
            }
        }
		ctrl.command_mode = COMMAND_ADDR_TX;
		ctrl.cmd_mio = IO_1BIT;
		if (spifc_flash_manuId == C_VENDOR_MXIC)  {
            ctrl.command = CMD_PAGE_PROGRAM_4X_MXIC;
            ctrl.addr_mio = IO_4BIT;
        }
        else {
            ctrl.command = CMD_PAGE_PROGRAM_4X;
            ctrl.addr_mio = IO_1BIT;
        }
		ctrl.data_mio = IO_4BIT;
		break;

	default:
		ret = STATUS_FAIL;
		goto __exit;
	}

    cache_drain_range((INT32U)tx_buf, cblen);

	// page write start
	PageNum = cblen >> 8;
	if(PageNum == 0) {
		PageNum = 1;
	}

	ctrl.spi_addr = addr >> 8 << 8;
	ctrl.tx_buf = (INT8U *)tx_buf;
	ctrl.tx_cblen = 256;
	for(i=0; i<PageNum; i++) {
		//DBG_PRINT("ctrl.spi_addr=0x%x\r\n",ctrl.spi_addr);
		if(en4b==0 && ctrl.spi_addr >> 24){
			spifcCtrl_t ctrl1;

			CLEAR_SPIFC_CTRL(ctrl1);
			ctrl1.command_mode = COMMAND_ONLY;
			ctrl1.command = CMD_EN4B;
			//DBG_PRINT("send EN4B cmd\r\n");
			ret = drv_l1_spifc_set_mode(&ctrl1);

			if(ret < 0) {
				goto __exit;
			}

            #if 1
			ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
			if(ret < 0) {
				goto __exit;
			}
			#endif

			switch(mio)
			{
				case IO_1BIT:
					//DBG_PRINT("CMD_PAGE_PROG4B\r\n");
					ctrl.command = CMD_PAGE_PROG4B;
					break;

				case IO_4BIT:
					//DBG_PRINT("CMD_PAGE_PROG4B_4X\r\n");
					if (spifc_flash_manuId == C_VENDOR_WINBOND) {
                        #if 1
                        ctrl.addr_mio = IO_1BIT;
                        ctrl.command = CMD_FAST_PAGE_PROG4B_4X;
                        #endif
					}
					else
					{
                        ctrl.addr_mio = IO_4BIT;
                        ctrl.command = CMD_PAGE_PROG4B_4X;
                    }

					break;

				default:
					ret = STATUS_FAIL;
					goto __exit;
			}
			en4b = 1;
		}
		//DBG_PRINT("ctrl.command=0x%x\r\n", ctrl.command);
		ret = drv_l1_spifcflash_write_enable();
		if(ret < 0) {
			goto __exit;
		}

		ret = drv_l1_spifc_set_mode(&ctrl);
		if(ret < 0) {
			goto __exit;
		}

		ctrl.spi_addr += 256;
		ctrl.tx_buf = (INT8U *)((INT32U)ctrl.tx_buf + 256);

		// wait program finish
		ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
		if(ret < 0) {
			goto __exit;
		}
	}

	if(en4b == 1){
		spifcCtrl_t ctrl1;

		CLEAR_SPIFC_CTRL(ctrl1);
		ctrl1.command_mode = COMMAND_ONLY;
		ctrl1.command = CMD_EX4B;
		//DBG_PRINT("send EX4B cmd\r\n");
		ret = drv_l1_spifc_set_mode(&ctrl1);
		if(ret < 0) {
			goto __exit;
		}

		ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
		if(ret < 0) {
			goto __exit;
		}
		en4b = 0;
	}

__exit:
    if (mio == IO_4BIT) {
        if (quad_enable)
            drv_l1_spifcflash_set_quad_enable(0);
    }
	drv_l1_spifc_restore_para(&backup);
	return ret;
}

/**
 * @brief   spi flash write n byte
 * @param   mio: IO_1BIT or IO_4BIT
 * @param   addr: spi flash start address, in byte unit
 * @param   buf: write buffer
 * @param   cblen: write length in byte, must not exceed 256
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_write_nbyte(INT8U bus_width, INT32U addr, INT8U *tx_buf, INT32U cblen)
{
	INT32S ret;
	spifcCtrl_t ctrl;
	BkupReg_t backup;
	INT8U en4b = 0;
    INT8U quad_enable = 1; //((bus_width & 0x10) != 0) ? 1 : 0;
    INT8U mio = bus_width & 0x03;
    INT32U this_page_len;
    INT32U tx_len;
    INT32U spi_addr;
    INT8U *ptx;

	drv_l1_spifc_save_para(&backup);
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
	if(ret < 0) {
		goto __exit;
	}

	CLEAR_SPIFC_CTRL(ctrl);
	switch(mio)
	{
	case IO_1BIT:
		ret = drv_l1_spifcflash_set_quad_enable(0);
		if(ret < 0) {
			goto __exit;
		}
		ctrl.command_mode = COMMAND_ADDR_TX;
		ctrl.command = CMD_PAGE_PROGRAM;
		break;

	case IO_4BIT:
		// quad bit enable
		if (quad_enable) {
            ret = drv_l1_spifcflash_set_quad_enable(1);
            if(ret < 0) {
                goto __exit;
            }
        }
		ctrl.command_mode = COMMAND_ADDR_TX;
		ctrl.cmd_mio = IO_1BIT;
		if (spifc_flash_manuId == C_VENDOR_MXIC)  {
            ctrl.command = CMD_PAGE_PROGRAM_4X_MXIC;
            ctrl.addr_mio = IO_4BIT;
        }
        else {
            ctrl.command = CMD_PAGE_PROGRAM_4X;
            ctrl.addr_mio = IO_1BIT;
        }
		ctrl.data_mio = IO_4BIT;
		break;

	default:
		ret = STATUS_FAIL;
		goto __exit;
	}

    cache_drain_range((INT32U)tx_buf, cblen);

    tx_len = cblen;
    spi_addr = addr;
    ptx = tx_buf;
    while (tx_len > 0) {
        ctrl.spi_addr = spi_addr;
        ctrl.tx_buf = (INT8U *)ptx;
        this_page_len = 256 - (spi_addr & (256-1));
        this_page_len = (this_page_len > tx_len) ? tx_len : this_page_len;
        ctrl.tx_cblen = this_page_len;
        tx_len -= this_page_len;
        spi_addr += this_page_len;
        ptx += this_page_len;
        //DBG_PRINT("ctrl.spi_addr=0x%x\r\n",ctrl.spi_addr);
        if(en4b==0 && ctrl.spi_addr >> 24){
            spifcCtrl_t ctrl1;

            CLEAR_SPIFC_CTRL(ctrl1);
            ctrl1.command_mode = COMMAND_ONLY;
            ctrl1.command = CMD_EN4B;
            //DBG_PRINT("send EN4B cmd\r\n");
            ret = drv_l1_spifc_set_mode(&ctrl1);

            if(ret < 0) {
                goto __exit;
            }

            #if 1
            ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
            if(ret < 0) {
                goto __exit;
            }
            #endif

            switch(mio)
            {
                case IO_1BIT:
                    //DBG_PRINT("CMD_PAGE_PROG4B\r\n");
                    ctrl.command = CMD_PAGE_PROG4B;
                    break;

                case IO_4BIT:
                    //DBG_PRINT("CMD_PAGE_PROG4B_4X\r\n");
					if (spifc_flash_manuId == C_VENDOR_WINBOND) {
                        #if 1
                        ctrl.addr_mio = IO_1BIT;
                        ctrl.command = CMD_FAST_PAGE_PROG4B_4X;
                        #endif
					}
					else
					{
                        ctrl.addr_mio = IO_4BIT;
                        ctrl.command = CMD_PAGE_PROG4B_4X;
                    }
                    break;

                default:
                    ret = STATUS_FAIL;
                    goto __exit;
            }
            en4b = 1;
        }
        //DBG_PRINT("ctrl.command=0x%x\r\n", ctrl.command);
        ret = drv_l1_spifcflash_write_enable();
        if(ret < 0) {
            goto __exit;
        }

        ret = drv_l1_spifc_set_mode(&ctrl);
        if(ret < 0) {
            goto __exit;
        }

        // wait program finish
        ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
        if(ret < 0) {
            goto __exit;
        }

        if(en4b == 1){
            spifcCtrl_t ctrl1;

            CLEAR_SPIFC_CTRL(ctrl1);
            ctrl1.command_mode = COMMAND_ONLY;
            ctrl1.command = CMD_EX4B;
            //DBG_PRINT("send EX4B cmd\r\n");
            ret = drv_l1_spifc_set_mode(&ctrl1);
            if(ret < 0) {
                goto __exit;
            }

            ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
            if(ret < 0) {
                goto __exit;
            }
            en4b = 0;
        }
    } // end while
__exit:
    if (mio == IO_4BIT) {
        if (quad_enable)
            drv_l1_spifcflash_set_quad_enable(0);
    }
	drv_l1_spifc_restore_para(&backup);
	return ret;
}

static ALIGN64 INT8U spifc_temp_buf[64];

/**
 * @brief   spifc ahb set to 1x read mode
 * @param   MIO: IO_1BIT, IO_2BIT or IO_4BIT
 * @param   enhance_en: enhance mode enable only for IO_4BIT mode
 * @param   dtr_en: DTR mode enable or not
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifc_set_read_mode(INT8U mio, INT8U enhance_en, INT32U dtr_en)
{
	INT32S ret;
	spifcCtrl_t ahbSetting;
    INT32S dummy_clk_nr;
    INT8U over_16m_byte = (spifc_flash_mega_density < 256) ? 0 : 1;

    if (dtr_en && !spifc_flash_cap.dtr_support) {
        dtr_en = 0;
        DBG_PRINT("device not support DTR, off the DTR mode\r\n");
    }

	CLEAR_SPIFC_CTRL(ahbSetting);
	ahbSetting.set_ahb_en = 1;
	ahbSetting.rx_cblen = 32;
	ahbSetting.dtr_en = dtr_en;
    ahbSetting.rx_buf = (INT32U *)spifc_temp_buf;

	switch(mio)
	{
	case IO_1BIT:
		ret = drv_l1_spifcflash_set_quad_enable(0);
		if(ret < 0) {
			goto __exit;
		}

        if (over_16m_byte) {
            ahbSetting.command_mode = COMMAND_ADDR_RX;
            // CMD_PAGE_READ4B have problem use CMD_FAST_READ4B instead
            #if 0
            ahbSetting.command = (over_16m_byte) ? CMD_PAGE_READ4B: CMD_PAGE_READ;
            #else
            ahbSetting.command = (over_16m_byte) ? CMD_FAST_READ4B: CMD_PAGE_READ;
            #endif
            ahbSetting.dummy_clk = (ahbSetting.command  == CMD_FAST_READ4B) ? 8 : 0;
        }
        else {
            //fast read
            ahbSetting.command_mode = COMMAND_ADDR_RX;
            ahbSetting.command = (over_16m_byte) ? CMD_FAST_READ4B : CMD_FAST_READ;
            ahbSetting.dummy_clk = 8;
        }
		ahbSetting.cmd_mio = IO_1BIT;
		ahbSetting.addr_mio = IO_1BIT;
		ahbSetting.data_mio = IO_1BIT;
		break;

	case IO_2BIT:
		ret = drv_l1_spifcflash_set_quad_enable(0);
		if(ret < 0) {
			goto __exit;
		}

        if (over_16m_byte) {
            ahbSetting.command_mode = COMMAND_ADDR_RX;
            ahbSetting.command = (over_16m_byte) ? CMD_PAGE_READ4B_2X: CMD_PAGE_READ_2X;
            //ahbSetting.dummy_clk = 4;
            ahbSetting.cmd_mio = IO_1BIT;
            ahbSetting.addr_mio = IO_2BIT;
            ahbSetting.data_mio = IO_2BIT;
            ahbSetting.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ahbSetting)) < 0) ? 4 : dummy_clk_nr;
        }
        else {
            //fast read
            ahbSetting.command_mode = COMMAND_ADDR_RX;
            ahbSetting.command = CMD_FAST_READ_2X;
            ahbSetting.dummy_clk = 8;
            ahbSetting.cmd_mio = IO_1BIT;
            ahbSetting.addr_mio = IO_1BIT;
            ahbSetting.data_mio = IO_2BIT;
        }
		break;

	case IO_4BIT:
		// quad bit enable
		ret = drv_l1_spifcflash_set_quad_enable(1);
		if(ret < 0) {
			goto __exit;
		}

		if (dtr_en == 1) {
            ahbSetting.command_mode = COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX;
			ahbSetting.command = (over_16m_byte) ? CMD_PAGE_READ4B_4X_DTR : CMD_PAGE_READ_4X_DTR;
			//ahbSetting.command = CMD_PAGE_READ_4X_DTR;
			//ahbSetting.dummy_clk = 7;		//for Winbond W25Q32JV-DTR
			ahbSetting.dtr_en = 1;
			ahbSetting.cmd_mio = IO_1BIT;
            ahbSetting.addr_mio = IO_4BIT;
            ahbSetting.data_mio = IO_4BIT;
            if(enhance_en) {
                ahbSetting.enhance_data = spifc_flash_cap.enhance_en_value;
            } else {
                ahbSetting.enhance_data = spifc_flash_cap.enhance_dis_value;
            }
            ahbSetting.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ahbSetting)) < 0) ? 7 : dummy_clk_nr - 1;
		}else{
            if (over_16m_byte) {
                ahbSetting.command_mode = COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX;
                ahbSetting.command = (over_16m_byte) ? CMD_PAGE_READ4B_4X : CMD_PAGE_READ_4X;
                //ahbSetting.command = CMD_PAGE_READ_4X;
                ahbSetting.cmd_mio = IO_1BIT;
                ahbSetting.addr_mio = IO_4BIT;
                ahbSetting.data_mio = IO_4BIT;
                if (enhance_en) {
                    ahbSetting.enhance_data = spifc_flash_cap.enhance_en_value;
                } else {
                    ahbSetting.enhance_data = spifc_flash_cap.enhance_dis_value;
                }
                ahbSetting.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ahbSetting)) < 0) ? 4 : dummy_clk_nr - 2;
            }
            else {
                ahbSetting.command_mode = COMMAND_ADDR_RX;
                ahbSetting.command = CMD_FAST_READ_4X;
                ahbSetting.dummy_clk = 8;
                ahbSetting.cmd_mio = IO_1BIT;
                ahbSetting.addr_mio = IO_1BIT;
                ahbSetting.data_mio = IO_4BIT;
            }
		}
		break;

	default:
		ret = STATUS_FAIL;
		goto __exit;
	}

    if (ret == 0)
        ret = drv_l1_spifc_set_mode(&ahbSetting);
__exit:
	return ret;
}

INT32S drv_l1_spifc_set_read_mode_over_16m_byte(INT8U mio, INT8U enhance_en, INT32U dtr_en, INT8U over_16m_byte)
{
	INT32S ret;
	spifcCtrl_t ahbSetting;
    INT32S dummy_clk_nr;

    if (dtr_en && !spifc_flash_cap.dtr_support) {
        dtr_en = 0;
        DBG_PRINT("device not support DTR, off the DTR mode\r\n");
    }

    if (spifc_flash_mega_density < 256)
        over_16m_byte = 0;

	CLEAR_SPIFC_CTRL(ahbSetting);
	ahbSetting.set_ahb_en = 1;
	ahbSetting.rx_cblen = 32;
	ahbSetting.dtr_en = dtr_en;
    ahbSetting.rx_buf = (INT32U *)spifc_temp_buf;

	if (over_16m_byte) {
        // address set to 16M byte to drv_l1_spifc_set_mode turn on send more address byte
        ahbSetting.spi_addr = P_SPIFC_IO_MAP_ADDR + (1UL<<24);

        #if 0
        spifcCtrl_t ctrl1;
        CLEAR_SPIFC_CTRL(ctrl1);
        ctrl1.command_mode = COMMAND_ONLY;
        ctrl1.command = CMD_EN4B;
        //DBG_PRINT("send EN4B cmd\r\n");
        ret = drv_l1_spifc_set_mode(&ctrl1);
		if(ret < 0) {
			goto __exit;
		}
        ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
        if(ret < 0) {
            goto __exit;
        }
        #endif
    }

	switch(mio)
	{
	case IO_1BIT:
        //DBG_PRINT("IO_1BIT\r\n");
		ret = drv_l1_spifcflash_set_quad_enable(0);
		if(ret < 0) {
			goto __exit;
		}

	#if 0
        //fast read
		ahbSetting.command_mode = COMMAND_ADDR_RX;
		ahbSetting.command = (over_16m_byte) ? CMD_FAST_READ4B: CMD_FAST_READ;
		ahbSetting.dummy_clk = 8;
	#else
		ahbSetting.command_mode = COMMAND_ADDR_RX;
		// CMD_PAGE_READ4B have problem use CMD_FAST_READ4B instead
		#if 0
		ahbSetting.command = (over_16m_byte) ? CMD_PAGE_READ4B: CMD_PAGE_READ;
		ahbSetting.dummy_clk = 0;
		#else
        ahbSetting.command = (over_16m_byte) ? CMD_FAST_READ4B: CMD_FAST_READ;
        ahbSetting.dummy_clk = 8;
        #endif

	#endif
		ahbSetting.cmd_mio = IO_1BIT;
		ahbSetting.addr_mio = IO_1BIT;
		ahbSetting.data_mio = IO_1BIT;
		break;

	case IO_2BIT:
        //DBG_PRINT("IO_2BIT\r\n");
		ret = drv_l1_spifcflash_set_quad_enable(0);
		if(ret < 0) {
			goto __exit;
		}

	#if 0
        //fast read
        ahbSetting.command_mode = COMMAND_ADDR_RX;
        ahbSetting.command = CMD_FAST_READ_2X;
        ahbSetting.dummy_clk = 8;
        ahbSetting.cmd_mio = IO_1BIT;
        ahbSetting.addr_mio = IO_1BIT;
        ahbSetting.data_mio = IO_2BIT;
    #else
		ahbSetting.command_mode = COMMAND_ADDR_RX;
		ahbSetting.command = (over_16m_byte) ? CMD_PAGE_READ4B_2X: CMD_PAGE_READ_2X;
		//ahbSetting.dummy_clk = 4;
		ahbSetting.cmd_mio = IO_1BIT;
		ahbSetting.addr_mio = IO_2BIT;
		ahbSetting.data_mio = IO_2BIT;
		ahbSetting.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ahbSetting)) < 0) ? 4 : dummy_clk_nr;
    #endif
		break;

	case IO_4BIT:
        //DBG_PRINT("IO_4BIT\r\n");
		// quad bit enable
		ret = drv_l1_spifcflash_set_quad_enable(1);
		if(ret < 0) {
			goto __exit;
		}

		if (dtr_en == 1) {
            ahbSetting.command_mode = COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX;
			ahbSetting.command = (over_16m_byte) ? CMD_PAGE_READ4B_4X_DTR : CMD_PAGE_READ_4X_DTR;
			//ahbSetting.command = CMD_PAGE_READ_4X_DTR;
			//ahbSetting.dummy_clk = 7;		//for Winbond W25Q32JV-DTR
			ahbSetting.dtr_en = 1;
			ahbSetting.cmd_mio = IO_1BIT;
            ahbSetting.addr_mio = IO_4BIT;
            ahbSetting.data_mio = IO_4BIT;
            if(enhance_en) {
                ahbSetting.enhance_data = spifc_flash_cap.enhance_en_value;
            } else {
                ahbSetting.enhance_data = spifc_flash_cap.enhance_dis_value;
            }
            ahbSetting.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ahbSetting)) < 0) ? 7 : dummy_clk_nr - 1;
		}else{
            #if 0
            ahbSetting.command_mode = COMMAND_ADDR_RX;
            ahbSetting.command = CMD_FAST_READ_4X;
            ahbSetting.dummy_clk = 8;
            ahbSetting.cmd_mio = IO_1BIT;
            ahbSetting.addr_mio = IO_1BIT;
            ahbSetting.data_mio = IO_4BIT;
            #else
            ahbSetting.command_mode = COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX;
            ahbSetting.command = (over_16m_byte) ? CMD_PAGE_READ4B_4X : CMD_PAGE_READ_4X;
            //ahbSetting.command = CMD_PAGE_READ_4X;
            ahbSetting.cmd_mio = IO_1BIT;
            ahbSetting.addr_mio = IO_4BIT;
            ahbSetting.data_mio = IO_4BIT;
            if (enhance_en) {
                ahbSetting.enhance_data = spifc_flash_cap.enhance_en_value;
            } else {
                ahbSetting.enhance_data = spifc_flash_cap.enhance_dis_value;
            }
            ahbSetting.dummy_clk = ((dummy_clk_nr = drv_l1_spifcflash_get_dummy_clk_nr(&ahbSetting)) < 0) ? 4 : dummy_clk_nr - 2;
            #endif
		}
		break;

	default:
		ret = STATUS_FAIL;
		goto __exit;
	}

	if (ret == 0)
        ret = drv_l1_spifc_set_mode(&ahbSetting);
__exit:
	return ret;
}

INT32S drv_l1_spifcflash_read_device_code(INT8U *ID)
{
	INT32S ret;
	spifcCtrl_t ctrl;
	BkupReg_t backup;
    static ALIGN64 INT8U buf[4];

    *ID = 0;
	drv_l1_spifc_save_para(&backup);
	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_ADDR_RX;
	ctrl.command = 0xAB;
    ctrl.dummy_clk = 0; // dummy byte require 3 byte,  dummy clock maximal couuld just 15, so use 3 byte address to simulate
    ctrl.spi_addr = 0;
	ctrl.rx_buf = (INT32U *)buf;
	ctrl.rx_cblen = 4;
    ctrl.cmd_mio = IO_1BIT;
    ctrl.addr_mio = IO_1BIT;
    ctrl.data_mio = IO_1BIT;
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		goto __exit;
	}
	*ID = buf[0];
	ret = STATUS_OK;
__exit:
	drv_l1_spifc_restore_para(&backup);
	return ret;
}

INT32S drv_l1_spifcflash_read_sfdp(INT32U addr, INT8U *buf, INT16U len)
{
	INT32S ret;
	spifcCtrl_t ctrl;
	BkupReg_t backup;

	drv_l1_spifc_save_para(&backup);
	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_ADDR_RX;
	ctrl.command = CMD_READ_SFDP;
    ctrl.dummy_clk = 8;
    ctrl.spi_addr = addr;
	ctrl.rx_buf = (INT32U *)buf;
	ctrl.rx_cblen = len;
    ctrl.cmd_mio = IO_1BIT;
    ctrl.addr_mio = IO_1BIT;
    ctrl.data_mio = IO_1BIT;
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		goto __exit;
	}
	ret = STATUS_OK;
__exit:
	drv_l1_spifc_restore_para(&backup);
	return ret;
}

//////////////////////////////////////////////////////
#define SPIFC_SFDP_SIG                      0x50444653  /* PDFS=0x50444653 */
#define SPIFC_SFDP_MAX_PARAM_HEADER         4
#define SPIFC_SFDP_BASIC_PARAM_FEATURE_ID   0xff00
#define SPIFC_SFDP_MAX_BASIC_FEATURE_DWORD  20

typedef struct spifc_sfdp_header_s
{
    INT32U sig;
    INT8U minor;
    INT8U major;
    INT8U number_of_param_header;
    INT8U protocol;
} spifc_sfdp_header_t;

typedef struct spifc_sfdp_param_header_s
{
    INT8U id_lsb;		        /* Parameter ID LSB JEDEC ID */
    INT8U minor;		        /* Parameter Minor Revision */
    INT8U major;		        /* Parameter Major Revision */
	INT8U len;		            /* Parameter Length in doudble words */
	INT8U offset_addr_lsb;      /* Parameter Table Pointer(byte address) */
	INT8U offset_addr_middle;
	INT8U offset_addr_msb;
	INT8U id_msb;
} spifc_sfdp_param_header_t;

static ALIGN64 spifc_sfdp_header_t          sfdp_header;
static ALIGN64 spifc_sfdp_param_header_t    sfdp_param_header[SPIFC_SFDP_MAX_PARAM_HEADER];
static ALIGN64 INT32U spifc_sfdp_basic_feature[SPIFC_SFDP_MAX_BASIC_FEATURE_DWORD];

INT32U spifc_sfdp_mega_bit_densiy(INT32U density)
{
    INT32U N,j;
    INT32U giga_bit;
    INT32U mega_bit;

    if ((density & 0x80000000) != 0) {
        N = density & 0x80000000;
        if (N <= 32)
            N = 32;
        N = N - 32;
        if (N >= 20) {
            DBG_PRINT("sfdp density N is too big %u\r\n", N);
            return 0xffffffff;
        }
        giga_bit = 4;
        for (j = 0; j <N; j++)
            giga_bit <<= 1;
        mega_bit = giga_bit << 10; // convert to mega bit
    }
    else {
        mega_bit = (density >> 20) + 1;
    }
    return mega_bit;
}

INT32S spifc_parse_sfdp(void)
{
    INT32S ret;
    INT16U number_of_param_header, j, k;
    INT16U param_header_id;

    spifc_flash_mega_density = spifc_cal_density_using_id(spifc_flash_deviceId & 0xff);

    if (spifc_flash_manuId == C_VENDOR_WINBOND || spifc_flash_manuId == C_VENDOR_GIGADEVICE) {
        spifc_flash_cap.enhance_en_value = ENHANCE_EN_VALUE_WB;
        spifc_flash_cap.enhance_dis_value = ENHANCE_DIS_VALUE_WB;
    }
    else {
        spifc_flash_cap.enhance_en_value = ENHANCE_EN_VALUE;
        spifc_flash_cap.enhance_dis_value = ENHANCE_DIS_VALUE;
    }

    #if C_DISABLE_SFDP
    return 0;
    #endif

    ret = drv_l1_spifcflash_read_sfdp(0, (INT8U *)&sfdp_header, sizeof(sfdp_header));
    if (ret != STATUS_OK)
        goto __exit;

    if (sfdp_header.sig != SPIFC_SFDP_SIG) {
        DBG_PRINT("No JEDEC sfpd\r\n");
        ret = STATUS_FAIL;
        goto __exit;
    }
    number_of_param_header = (sfdp_header.number_of_param_header > SPIFC_SFDP_MAX_PARAM_HEADER) ? SPIFC_SFDP_MAX_PARAM_HEADER : (sfdp_header.number_of_param_header + (INT8U)1);
    DBG_PRINT("sfdp version: %02u.%02u, number %u, protocol 0x%02x\r\n", sfdp_header.major, sfdp_header.minor, number_of_param_header, sfdp_header.protocol);
    if (number_of_param_header == 0) {
        ret = STATUS_FAIL;
        goto __exit;
    }
    ret = drv_l1_spifcflash_read_sfdp(sizeof(spifc_sfdp_header_t), (INT8U *)(&sfdp_param_header[0]), sizeof(spifc_sfdp_param_header_t)*number_of_param_header);
    if (ret != STATUS_OK)
        goto __exit;
    for (k =0; k < number_of_param_header; k++) {
        INT32U offset;

        param_header_id = ((INT16U)sfdp_param_header[k].id_msb << 8) | sfdp_param_header[k].id_lsb;
        offset = ((INT32U)sfdp_param_header[k].offset_addr_msb << 16) |  ((INT32U)sfdp_param_header[k].offset_addr_middle << 8) | \
                      ((INT32U)sfdp_param_header[k].offset_addr_lsb << 0);
        DBG_PRINT("param_header_id[%u]:0x%04x, version: %02u.%02u, offset %u, len 0x%02x\r\n", k, param_header_id, sfdp_param_header[k].major, sfdp_param_header[k].minor, offset, sfdp_param_header[k].len);
        if (param_header_id == SPIFC_SFDP_BASIC_PARAM_FEATURE_ID) {
            INT16U number_of_basic_feature;

            number_of_basic_feature = (sfdp_param_header[k].len > SPIFC_SFDP_MAX_BASIC_FEATURE_DWORD) ? SPIFC_SFDP_MAX_BASIC_FEATURE_DWORD : sfdp_param_header[k].len;
            if (number_of_basic_feature == 0) {
                ret = STATUS_FAIL;
                goto __exit;
            }
            ret = drv_l1_spifcflash_read_sfdp(offset, (INT8U *)(&spifc_sfdp_basic_feature[0]), sizeof(INT32U)*number_of_basic_feature);
            if (ret != STATUS_OK)
                goto __exit;
             for (j =0; j < number_of_basic_feature; j++) {
                DBG_PRINT("sfdp_basic[%u]=0x%08x\r\n", j, spifc_sfdp_basic_feature[j]);
            }

            spifc_flash_cap.two_bit_support = 0;
            spifc_flash_cap.four_bit_support = 0;
            if (number_of_basic_feature > 0) { // 1st DWORD
                spifc_flash_cap.dtr_support = ((spifc_sfdp_basic_feature[0] & (1<<19)) != 0) ? 1 : 0;

                // check fast command 1-1-2 support ?
                spifc_flash_cap.fast_read_dual_output_support = ((spifc_sfdp_basic_feature[0] & (1<<16)) != 0) ? 1 : 0;
                spifc_flash_cap.two_bit_support = spifc_flash_cap.fast_read_dual_output_support;
                if (!spifc_flash_cap.two_bit_support)
                    spifc_flash_cap.two_bit_support = ((spifc_sfdp_basic_feature[0] & (1<<20)) != 0) ? 1 : 0;

                // check fast command 1-1-4 support ?
                spifc_flash_cap.fast_read_quad_output_support = ((spifc_sfdp_basic_feature[0] & (1<<22)) != 0) ? 1 : 0;
                spifc_flash_cap.four_bit_support = spifc_flash_cap.fast_read_quad_output_support;
                if (!spifc_flash_cap.four_bit_support)
                    spifc_flash_cap.four_bit_support = ((spifc_sfdp_basic_feature[0] & (1<<21)) != 0) ? 1 : 0;

                spifc_flash_cap.addr_type = ((spifc_sfdp_basic_feature[0] & (0x3<<17)) >> 17) & 0x3;
            }
            else {
                spifc_flash_cap.fast_read_dual_output_support = 0;
                spifc_flash_cap.fast_read_quad_output_support = 0;
                spifc_flash_cap.addr_type = 0;
                spifc_flash_cap.dtr_support = 0;
            }
            DBG_PRINT("DTR %u, fast command %u,%u support, address type %u\r\n", spifc_flash_cap.dtr_support, \
                    spifc_flash_cap.fast_read_dual_output_support, spifc_flash_cap.fast_read_quad_output_support, \
                    spifc_flash_cap.addr_type);

            if (number_of_basic_feature > 1) { // 2nd DWORD
                INT32U density;
                INT32U sfdp_density;

                density = spifc_cal_density_using_id(spifc_flash_deviceId & 0xff);
                sfdp_density = spifc_sfdp_mega_bit_densiy(spifc_sfdp_basic_feature[1]);
                if (density != sfdp_density)
                    DBG_PRINT("density not match %u, %u\r\n", density, sfdp_density);
            }
            DBG_PRINT("mega bit density %u\r\n", spifc_flash_mega_density);

            if (number_of_basic_feature > 14) { // 15th DWORD
                spifc_flash_cap.qer_type = ((spifc_sfdp_basic_feature[14] & (0x7<<20)) >> 20) & 0x7;
                if (spifc_flash_cap.qer_type == C_SFDP_QER_TYPE_4) {
                    spifc_flash_cap.enhance_en_value = ENHANCE_EN_VALUE_WB;
                    spifc_flash_cap.enhance_dis_value = ENHANCE_DIS_VALUE_WB;
                }
            }
            else
                spifc_flash_cap.qer_type = C_SFDP_QER_TYPE_2; // C_VENDOR_MXIC use QER type 2

            DBG_PRINT("QER_type ? %u\r\n", spifc_flash_cap.qer_type);
        }
        else {
            static ALIGN64 INT32U sfdp_temp;
            for (j =0; j < sfdp_param_header[k].len; j++) {
                ret = drv_l1_spifcflash_read_sfdp(offset+j*4, (INT8U *)(&sfdp_temp), 4);
                DBG_PRINT("sfdp_basic[%u]=0x%08x\r\n", j, sfdp_temp);
            }
        }
    }
__exit:
	return ret;
}

// if to call, must call after init_spifc success
INT8U spifc_get_manuId(void)
{
    return spifc_flash_manuId;
}

INT32S spifc_read_id(INT8U* manuId, INT16U* deviceId)
{
    INT32S ret;
    static ALIGN64 INT8U spifc_id_buffer[3];

    *manuId = 0;
    *deviceId = 0;

    ret = drv_l1_spifcflash_read_id(spifc_id_buffer);
    if (ret != 0)
        return ret;

    *manuId = spifc_id_buffer[0];
	*deviceId = (spifc_id_buffer[1] << 8) | spifc_id_buffer[2];

    SPIFC_DBG_PRINT("spi id: 0x%x, 0x%x\r\n", *manuId, *deviceId);

    return 0;
}

/**
 * @brief   exit 4 byte addressing mode
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_exit_4B_address(void)
{
    BkupReg_t backup;
    INT32S ret;
    spifcCtrl_t ctrl1;

    drv_l1_spifc_save_para(&backup);
    CLEAR_SPIFC_CTRL(ctrl1);
    ctrl1.command_mode = COMMAND_ONLY;
    ctrl1.command = CMD_EX4B;
    //DBG_PRINT("send EX4B cmd\r\n");
    ret = drv_l1_spifc_set_mode(&ctrl1);
    if(ret < 0) {
        goto __exit;
    }

    ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);

    __exit:
    drv_l1_spifc_restore_para(&backup);

    return ret;
}

INT32S init_spifc(SPIFC_IO_ENUM spifc_io)
{
	SPIFC_IO_ENUM cur_spifc_io;
	INT32U back_FUNPOS0;
	INT32U back_IOD_ATT;
    INT32S ret = -1;

    drv_l1_spifc_init(21375000);

	cur_spifc_io = (spifc_io == SPIFC_AUTO_ENUM)? SPIFC_IOD_0_5_ENUM : spifc_io;
	back_FUNPOS0 = R_FUNPOS0;
	back_IOD_ATT = R_IOD_ATT;
	while(1)
	{
		switch(cur_spifc_io)
		{
			case SPIFC_IOD_0_5_ENUM:
				SPIFC_DBG_PRINT("SPIFC IO: IOD[0:5]\r\n");
				R_FUNPOS0 &= ~(1<<30);
				R_IOD_ATT |= 0x08;		//set IOD3 floating
				#if 0 // remark it, let CodePacker add pre-register to set driving
                gpio_drving_init_io(IO_D0,(IO_DRV_LEVEL)SPIFC_DRIVING);
                gpio_drving_init_io(IO_D1,(IO_DRV_LEVEL)SPIFC_CLK_DRIVING);
                gpio_drving_init_io(IO_D2,(IO_DRV_LEVEL)SPIFC_DRIVING);
                gpio_drving_init_io(IO_D3,(IO_DRV_LEVEL)SPIFC_DRIVING);
                gpio_drving_init_io(IO_D4,(IO_DRV_LEVEL)SPIFC_DRIVING);
                gpio_drving_init_io(IO_D5,(IO_DRV_LEVEL)SPIFC_DRIVING);
                #endif
				break;
			case SPIFC_IOA12_IOA14_IOB3_IOD7_9_ENUM:
				DBG_PRINT("SPIFC IO: IOA[12,14,15],IOD[7:9]\r\n");
				R_FUNPOS0 |= (1<<30);
				R_IOD_ATT |= 0x0100;	//set IOD8 floating
				#if 0 // remark it, let CodePacker add pre-register to set driving
                gpio_drving_init_io(IO_D7,(IO_DRV_LEVEL)SPIFC_DRIVING);
                gpio_drving_init_io(IO_A14,(IO_DRV_LEVEL)SPIFC_CLK_DRIVING);
                gpio_drving_init_io(IO_A12,(IO_DRV_LEVEL)SPIFC_DRIVING);
                gpio_drving_init_io(IO_D8,(IO_DRV_LEVEL)SPIFC_DRIVING);
                gpio_drving_init_io(IO_D9,(IO_DRV_LEVEL)SPIFC_DRIVING);
                gpio_drving_init_io(IO_A15,(IO_DRV_LEVEL)SPIFC_DRIVING);
                #endif
				break;
			default:
				return -1;
		}

		ret = spifc_read_id(&spifc_flash_manuId, &spifc_flash_deviceId);
		if((spifc_flash_manuId != 0) && (spifc_flash_manuId != 0xFF)
			&& ((spifc_flash_deviceId >> 8) != spifc_flash_manuId) )
		{
            spifc_parse_sfdp();

            // WINBOND have part number that default to 4 byte address, so apply below to let the ADP bit to 0.
            if ((spifc_flash_manuId == C_VENDOR_WINBOND) && (spifc_flash_mega_density > 128))
                drv_l1_spifcflash_exit_4B_address();

			return 0;
		}

		R_IOD_ATT = back_IOD_ATT;
		R_FUNPOS0 = back_FUNPOS0;

		if(spifc_io == SPIFC_AUTO_ENUM)
		{
			cur_spifc_io++;
		}
		else
		{
			break;
		}
	}

	ret = - 1;

	return ret;
}

INT8U spifc_read_status(void)
{
	return drv_l1_spifcflash_read_status();
}

void spifc_write_status(INT8U statusValue)
{
    INT32S ret;
	static ALIGN64 INT32U status;
	spifcCtrl_t ctrl;

	status = statusValue;
	ret = drv_l1_spifcflash_write_enable();
	if(ret < 0) {
		goto __exit;
	}

	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_TX;
	ctrl.command = CMD_WRITE_STATUS;
	ctrl.tx_buf = (INT8U *)&status;
	ctrl.tx_cblen = 1;
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		goto __exit;
	}

	// wait program finish
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
__exit:
	return;
}

INT32S Drv_SPIFC_Write_Sector(INT32U startSectorAddr, INT32U sectorLen, INT8U* pBuffer)
{
	INT32S retval = 0;
	INT32U lenInBytes = sectorLen << 9;
	INT32U startAddrInBytes = startSectorAddr << 9;

    //DBG_PRINT("Drv_SPIFC_Write_Sector: start 0x%x, nr %u\r\n", startSectorAddr, sectorLen);

	retval = drv_l1_spifcflash_write_page(IO_1BIT, startAddrInBytes, pBuffer, lenInBytes);
	if(retval != 0)
	{
        DBG_PRINT("SPIFC Write Failed 0x%x, %d\r\n", startSectorAddr, sectorLen);
	}

	return retval;
}

INT32S spifc_erase_block(INT32U blockAddr)
{
	INT32S retval = 0;

	retval = drv_l1_spifcflash_block_erase(blockAddr << 16);
    if(retval != 0)
    {
        DBG_PRINT("SPIFC Erase Block %d Failed\r\n", blockAddr);
    }

	return retval;
}

INT32S Drv_SPIFC_Read_Sector(INT32U startSectorAddr, INT32U sectorLen, INT8U* pBuffer)
{
	INT32S retval = 0;
	INT32U lenInBytes = sectorLen << 9;
	INT32U startAddrInBytes = startSectorAddr << 9;
    //INT32U i;

	//for (i = 0; i < 2000; i++) // TEST
        //retval = drv_l1_spifcflash_read_page(IO_1BIT, startAddrInBytes, pBuffer, lenInBytes, SPIFC_READ_1_BIT_CMD_TYPE_NORMAL);
	retval = drv_l1_spifcflash_read_nbyte(IO_1BIT, startAddrInBytes, pBuffer, lenInBytes, SPIFC_READ_1_BIT_CMD_TYPE_NORMAL);
	if(retval != 0)
	{
        DBG_PRINT("SPIFC Read Failed 0x%x, %d\r\n", startSectorAddr, sectorLen);
	}

	return retval;
}

INT32U drv_l1_spifcflash_read_configurartion(void)
{
    static ALIGN64 INT8U buf[4];
	INT32S ret;
	spifcCtrl_t ctrl;

	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_RX;
	ctrl.command = CMD_READ_CONFIGURATION;
	ctrl.rx_buf = (INT32U *)buf;
	ctrl.rx_cblen = 1;	//read length is 4 align
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		ret = 0;
		goto __exit;
	}

	ret = (INT32U)buf[0];
__exit:

	return ret;
}

/**
 * @brief   read spi flash status n byte
 * @param   buf: buffer to store received bytes, must aligned to 4 byte
 * @param   len: number of bytes to read
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_read_status_nbyte(INT8U *buf, INT16U len)
{
	INT32S ret;
	spifcCtrl_t ctrl;

	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_RX;
	ctrl.command = CMD_READ_STATUS;
	ctrl.rx_buf = (INT32U *)buf;
	ctrl.rx_cblen = len;	//read length is 4 align
	ret = drv_l1_spifc_set_mode(&ctrl);

	return ret;
}

/**
 * @brief   write spi flash status n byte
 * @param   buf: buffer to store received bytes, must aligned to 4 byte
 * @param   len: number of bytes to read
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_write_status_nbyte(INT8U *buf, INT16U len)
{
	INT32S ret;
	spifcCtrl_t ctrl;

    ret = drv_l1_spifcflash_write_enable();
    if(ret < 0) {
        goto __exit;
    }

    CLEAR_SPIFC_CTRL(ctrl);
    ctrl.command_mode = COMMAND_TX;
    ctrl.command = CMD_WRITE_STATUS;
    ctrl.tx_buf = (INT8U *)buf;
    ctrl.tx_cblen = len;
    ret = drv_l1_spifc_set_mode(&ctrl);
    if(ret < 0) {
        goto __exit;
    }

    // wait program finish
    ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);

__exit:
	return ret;
}

INT32S drv_l1_spifcflash_modify_status_bit(INT16U bit_mask, INT16U bit_value, INT8U toggle)
{
    BkupReg_t backup;
    static ALIGN64 INT8U buf[4];
    INT32S ret = STATUS_OK;
    INT8U upper_byte = ((bit_value >> 8) & 0x00ff);
    INT8U upper_byte_mask = ((bit_mask >> 8) & 0x00ff);
    INT8U lower_byte = bit_value & 0x00ff;
    INT8U lower_byte_mask = bit_mask & 0x00ff;
    INT8U modify = 0;
    INT32S status;
    INT32S configuration;

    drv_l1_spifc_save_para(&backup);

    status = drv_l1_spifcflash_read_status(); // Note: must always read
    if (status >= 0)
        buf[0] = status;
    else {
        ret = status;
        goto __exit;
    }

    if (lower_byte_mask) {
        if (toggle) {
            buf[0] ^= (lower_byte_mask & lower_byte);
            modify = 1;
        }
        else if ((buf[0] & lower_byte_mask) != lower_byte) {
            buf[0] &= ~lower_byte_mask;
            buf[0] |= lower_byte;
            modify = 1;
        }
    }

    if (upper_byte_mask) {
        configuration = drv_l1_spifcflash_read_configurartion();
        if (configuration >= 0) {
            buf[1] = configuration;
            if (toggle) {
                buf[1] ^= (upper_byte_mask & upper_byte);
                modify = 1;
            }
            else if ((buf[1] & upper_byte_mask) != upper_byte) {
                buf[1] &= ~upper_byte_mask;
                buf[1] |= upper_byte;
                modify = 1;
            }
        }
        else {
            upper_byte_mask = 0;
            ret = configuration;
        }
    }

    if (ret >= 0) {
        if (modify) {
            ret = drv_l1_spifcflash_write_status_nbyte(buf, (upper_byte_mask ? 2 : 1));
            #if 0 // DEBUG CODE
            if (ret >= 0) {
                status = drv_l1_spifcflash_read_status();
                configuration = drv_l1_spifcflash_read_configurartion();
            }
            #endif
        }
    }

__exit:
    drv_l1_spifc_restore_para(&backup);
	return ret;
}

INT32S drv_l1_spifcflash_get_dummy_clk_nr(spifcCtrl_t *pctrl)
{
    INT8U dc = 0;
    INT8U nr;
    INT8U mio = pctrl->data_mio;
    INT8U dtr = pctrl->dtr_en;

    mio &= 0x3;

    if (spifc_flash_manuId == C_VENDOR_MXIC) {
        INT32S ret;
        ret = drv_l1_spifcflash_read_configurartion();
        if (ret >= 0)
            dc = (ret >> 6) & 0x3;
    }

    if (dtr) {
        if (spifc_flash_manuId == C_VENDOR_WINBOND)
            nr = 8;
        else if (spifc_flash_manuId == C_VENDOR_MICRON)
            nr = 8;
        else if (spifc_flash_manuId == C_VENDOR_GIGADEVICE)
            nr = 16;
        else if (spifc_flash_manuId == C_VENDOR_MXIC)
            nr = 8;
        else {
            if (dc == 3)
                nr = 10;
            else if (dc == 2)
                nr = 8;
            else if (dc == 1)
                nr = 6;
            else
                nr = 6;
        }
    }
    else if (mio == IO_4BIT) {
        if (spifc_flash_manuId == C_VENDOR_MICRON)
            nr = 10;
        else if (spifc_flash_manuId == C_VENDOR_GIGADEVICE)
             nr = 16;
        else if (dc == 3)
            nr = 10;
        else if (dc == 2)
            nr = 8;
        else if (dc == 1)
            nr = 4;
        else
            nr = 6;
    }
    else if (mio == IO_2BIT) {
        if (spifc_flash_manuId == C_VENDOR_MICRON)
            nr = 8;
        else if (dc == 3)
            nr = 8;
        else if (dc == 2)
            nr = 4;
        else if (dc == 1)
            nr = 8;
        else
            nr = 4;
    }
    else {
        nr = 8;
    }
    return (INT32U)nr;
}

INT32S drv_l1_spifcflash_config_dc(INT8U dc)
{
    INT32S ret;

    if (dc > 3)
        return STATUS_FAIL;

    ret = drv_l1_spifcflash_modify_status_bit(0xC000, (dc << 14), 0);

    return ret;
}

INT32S drv_l1_spifcflash_exit_qpi(void)
{
	spifcCtrl_t ctrl;

	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_ONLY;
	ctrl.command = 0xFF;
	return drv_l1_spifc_set_mode(&ctrl);
}

static BkupReg_t backup;

void spifc_save_para(void)
{
    drv_l1_spifc_save_para(&backup);
}

void spifc_restore_para(void)
{
    drv_l1_spifc_restore_para(&backup);
}

/**
 * @brief   simulate memcpy that read from spi flash, but could not write to spi flash
 * @param   src: spifc address to be read, the address must between 0x30000000~0x3fffffff
 * @param   dst: dram or sram address
 * @return 	STATUS_FAIL, STATUS_SUCCESS
 */
INT32S spifc_memcpy(INT8S *dst, INT8S *src, INT32U nbyte)
{
    INT32S ret;

    if ((((INT32U)dst) & 0xf0000000) == P_SPIFC_IO_MAP_ADDR) {
        DBG_PRINT("spifc_memcpy could not support write to spifc 0x%x\r\n", dst);
        return STATUS_FAIL;
    }
    if ((((INT32U)src) & 0xf0000000) != P_SPIFC_IO_MAP_ADDR) {
        DBG_PRINT("spifc_memcpy src must start with 0x30000000, 0x%x\r\n", src);
        return STATUS_FAIL;
    }
    INT32U act_spi_clock_rate;
    drv_l1_spifc_clk_rate_get(&act_spi_clock_rate);

    INT8U read_bus_width = IO_1BIT; // IO_4BIT, IO_1BIT, IO_2BIT
    INT8U dtr_en = 0; // when dtr_en is 1, could only use IO_4BIT

    #if 1
    drv_l1_spifc_set_timing_auto(dtr_en, read_bus_width, act_spi_clock_rate);
    ret = drv_l1_spifcflash_read_nbyte(read_bus_width | (dtr_en ? 0x20 : 0x00), (((INT32U)src) & (~P_SPIFC_IO_MAP_ADDR)), (INT8U *)dst, nbyte, 0);
    #else

    INT8U over_16m_byte = ((((INT32U)src) & (~0xf0000000)) >= 16*1024*1024UL) ? 1 : 0;

    drv_l1_spifc_set_read_mode_over_16m_byte(read_bus_width, 0, dtr_en, over_16m_byte);
    drv_l1_spifc_set_timing_auto(dtr_en, read_bus_width, act_spi_clock_rate);

    // suggest to ensure src is 64 byte aligned because cache require 64 aligned
    //drv_l1_cp15_clean_dcache_range((INT32U)src, nbyte);
    drv_l1_cp15_invalidate_dcache_range((INT32U)src, nbyte);
    memcpy(dst, src, nbyte);
    drv_l1_spifcflash_exit_4B_address();
    ret = STATUS_OK;
    #endif

    return ret;
}

INT32S drv_l1_spifcflash_read_uid(INT8U * UID)
{
    INT32S ret;
    INT8U buf[20];
    spifcCtrl_t ctrl = {0};
    BkupReg_t backup;
    INT32U act_spi_clock_rate;

    drv_l1_spifc_save_para(&backup);
    drv_l1_spifc_clk_rate_get(&act_spi_clock_rate);
    drv_l1_spifc_set_timing_auto(0, IO_1BIT, act_spi_clock_rate);
    ctrl.command_mode = COMMAND_RX;
    ctrl.command = CMD_READ_UID;
    ctrl.dummy_clk = 32;
    ctrl.rx_buf = (INT32U *)buf;
    ctrl.rx_cblen = sizeof(buf);
    ret = drv_l1_spifc_set_mode(&ctrl);
    if(ret == STATUS_OK)
    {
        memcpy(UID, &buf[4], sizeof(buf) - 4);
    }
    else
    {
        DBG_PRINT("SPIFC uuid read fail\r\n");
    }
    drv_l1_spifc_restore_para(&backup);
    return ret;
}

INT32S drv_l1_spifcflash_security_read_nbyte(INT8U select, INT32U addr, INT8U * rx_buf, INT32U cblen)
{
    spifcCtrl_t ctrl = {0};
    BkupReg_t backup;

    if(addr + cblen > SECURITY_REGISTER_SIZE || cblen == 0)
    {
        ERR_PRINT("%u, %u\r\n", addr, cblen);
        return STATUS_FAIL;
    }

    drv_l1_spifc_save_para(&backup);
    ctrl.command_mode = COMMAND_ADDR_RX;
    ctrl.command = CMD_READ_SECURITY_REG;
    ctrl.dummy_clk = 8;
    ctrl.spi_addr = SECURITY_REGISTER_ADDRESS(select) + addr;
    ctrl.rx_buf = (INT32U *)rx_buf;
    ctrl.dtr_en = 0;
    while(cblen)
    {
        ctrl.rx_cblen = (cblen > 256) ? 256 : cblen;
         // set to 0 so that drv_l1_spifc_set_mode won't read, and we use dma later
        if(drv_l1_spifc_set_mode(&ctrl) < 0)
        {
            goto __exit;
        }
        else
        {
            cblen -= ctrl.rx_cblen;
            ctrl.rx_buf = (INT32U *)((INT32U)ctrl.rx_buf + ctrl.rx_cblen);
            ctrl.spi_addr += ctrl.rx_cblen;
        }
    }

__exit:
    drv_l1_spifc_restore_para(&backup);

    return (cblen == 0) ? STATUS_OK : STATUS_FAIL;
}


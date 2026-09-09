#ifndef __DRV_L1_SDC_H__
#define __DRV_L1_SDC_H__

#include "drv_l1.h"
#include "drv_l1_dma.h"
#include "drv_l1_sfr.h"
#include "cmsis_os.h"

#define _DRV_L1_TIMER                   1
#define _DRV_L1_DMA                     1

//#define USE_DMAQ                        1

#define _DRV_L1_SDC_30_DMA_NOAUTOSTOP		1
#define _DRV_L1_SDC_30_USE_DMACOM_INT		1 // INTEN8, when _DRV_L1_SDC_30_USE_ALLCOM_INT is 1, suggest dont set this to 1
#define _DRV_L1_SDC_30_USE_ALLCOM_INT		1 // INTEN10, this interrupt is raised when dma and data complete done

#define _DRV_L1_SDC_30_USE_INT                  (( _DRV_L1_SDC_20_USE_INT == 1) || \
                                                 (_DRV_L1_SDC_30_USE_DMACOM_INT == 1) || \
                                                 (_DRV_L1_SDC_30_USE_ALLCOM_INT == 1) \
                                                )

#define SetSDBase(x) ((x==0)?P_SDC0_BASE : P_SDC1_BASE)

typedef struct sdReg_s {
	volatile INT32U DataTx;		/* Data Transmit register */
	volatile INT32U DataRx;		/* Data Receive register */
	volatile INT32U Cmd;		/* Command register */
	volatile INT32U Arg;		/* Argument register */
	volatile INT32U Resp;		/* Response register */
	volatile INT32U Status;		/* Controller status register */
	volatile INT32U Ctrl;		/* Control register */
	volatile INT32U IntEn;		/* Interrupt enable register */
	volatile INT32U ClkGated;
	volatile INT32U TimeoutEn;
	volatile INT32U TimeoutTHR;
} sdReg_t;

typedef struct sd30Reg_s {
	volatile INT32U DataTx;		/* Data Transmit register */
	volatile INT32U DataRx;		/* Data Receive register */
	volatile INT32U Cmd;		/* Command register */
	volatile INT32U Arg;		/* Argument register */
	volatile INT32U Resp;		/* Response register */
	volatile INT32U Status;		/* Controller status register */
	volatile INT32U Ctrl;		/* Control register */
	volatile INT32U IntEn;		/* Interrupt enable register */
	volatile INT32U Ctrl2;		/* Control2 register */
	volatile INT32U DmaAddr;	/* DMAADDR register */
	volatile INT32U DmaBlk;     /* DMABLK register */
	volatile INT32U DmaBlkSts;  /* DMABLKSTS register */
	volatile INT32U FifoSts; 	/* FIFOSTS register */
	volatile INT32U Dummy1;
	volatile INT32U Dummy2;
	volatile INT32U Dummy3;
	volatile INT32U Clk_Dly;
	volatile INT32U Cmd_Dly;
	volatile INT32U Data0_Dly;
	volatile INT32U Data1_Dly;
	volatile INT32U Data2_Dly;
	volatile INT32U Data3_Dly;
    volatile INT32U Delay_Offset;
    volatile INT32U Pre_Post_Phase;
    volatile INT32U SlowClk;
    volatile INT32U ClockIdle;
} sd30Reg_t;


typedef struct sd_l1_data_s{
	INT32U *buf;				/*!< @brief Data buffer pointer. */
	INT32U blksz;				/*!< @brief Data block size. */
	INT32U blocks; 				/*!< @brief Number of blocks. */
	INT16U dir; 				/*!< @brief Data direction. */
	INT16U dmaen;				/*!< @brief DMA enable. */
	INT32U bytes_xfered;		/*!< @brief Data transfered in byte. */
	volatile INT8S dma_notify;	/*!< @brief Used for checking dam finish. */
#if _OPERATING_SYSTEM != _OS_NONE
	void* dma_q_buf;
    #if _OPERATING_SYSTEM == _OS_UCOS2
	OS_EVENT *dma_q;
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
        xQueueHandle dma_q;
    #endif
#endif
}sd_l1_data_t;

typedef struct sd_l1_ctrl_t{
	INT8U device_id;		/*!< @brief Device ID. */
	INT8U cmd;				/*!< @brief Command. */
	INT8U resp_type;		/*!< @brief Response type. */
	INT8U data_separate;	/*!< @brief Data transfer separate. If this flag is set, data would not transfer when sent command */
	INT32U arg;				/*!< @brief Argument. */
	INT32U resp[4];			/*!< @brief Response buffer. */
	sd_l1_data_t *data;		/*!< @brief Transfer data information. */
}sd_l1_ctrl_t;

typedef INT8U(*sd_card_present_check_callback)(INT32U);

// SDC control register
#define C_SDC_CTL_CLKDIV				((10*MHZ - 8) / 8)//75//19//3//((10*MHZ - 8) / 8)
#define	C_SDC_CTL_BUS_4BIT				0x00000100
#define	C_SDC_CTL_DMA_ENABLE	  		0x00000200
#define	C_SDC_CTL_IO_INT				0x00000400
#define	C_SDC_CTL_ENABLE				0x00000800
#define	C_SDC_CTL_DUMMYCLK				0x00001000				//This bit only work on GPL326XXB
#define	C_SDC_CTL_BLK8			  		0x00080000
#define	C_SDC_CTL_BLK256				0x01000000
#define	C_SDC_CTL_BLK512				0x02000000
#define	C_SDC_EXT_WAIT_ENABLE			0x40000000              // Extend the NCR(command then response) wait cycles from 6x to 2xx cycles
#define	C_SDC_CTL_COP_1T                0x00008000
#define	C_SDC_CTL_COP_HALFT             0x00004000
#define	C_SDC_CTL_COP_NORMAL            0x00000000
#define	C_SDC_CTL_COP_MASK              0x0000C000

// SDC command register
#define	C_SDC_CMD_STOP					0x00000040
#define	C_SDC_CMD_RUN					0x00000080
#define C_SDC_CMD_WITH_DATA		  		0x00000100
#define C_SDC_CMD_WRITE_DATA			0x00000200
#define C_SDC_CMD_MULTI_BLOCK	  		0x00000400
#define C_SDC_CMD_INIT_CARD	 			0x00000800
#define C_SDC_CMD_RESP_R0				0x00000000
#define C_SDC_CMD_RESP_R1				0x00001000
#define C_SDC_CMD_RESP_R2				0x00002000
#define C_SDC_CMD_RESP_R3				0x00003000
#define C_SDC_CMD_RESP_R6				0x00006000
#define C_SDC_CMD_RESP_R7  				0x00001000
#define C_SDC_CMD_RESP_R1B		  		0x00007000

// SDC status register
#define C_SDC_STATUS_CONTROLLER_BUSY	0x00000001
#define C_SDC_STATUS_CARD_BUSY			0x00000002
#define C_SDC_STATUS_CMD_COMPLETE		0x00000004
#define C_SDC_STATUS_DATA_COMPLETE		0x00000008
#define C_SDC_STATUS_RESP_INDEX_ERR		0x00000010
#define C_SDC_STATUS_RESP_CRC_ERR		0x00000020
#define C_SDC_STATUS_RESP_REG_FULL		0x00000040
#define C_SDC_STATUS_DATA_BUF_FULL		0x00000080
#define C_SDC_STATUS_DATA_BUF_EMPTY		0x00000100
#define C_SDC_STATUS_TIMEOUT			0x00000200
#define C_SDC_STATUS_DATA_CRC_ERR		0x00000400
#define C_SDC_STATUS_CARD_PROTECT		0x00000800
#define C_SDC_STATUS_CARD_PRESENT	  	0x00001000
#define	C_SDC_STATUS_SDIO_INT_PEND		0x00002000
#define C_SDC_STATUS_DMA_XFER_COMPLETE  0x00010000
#define C_SDC_STATUS_ALL_COMPLETE       0x00040000
#define C_SDC_STATUS_CLEAR_ALL			0x0005FFFF // williamyeo-sd30-dmablksts-not-match

// SDC interrupt register
#define	C_SDC_INT_CMD_COMP				0x00000001
#define	C_SDC_INT_DATA_COMP				0x00000002
#define	C_SDC_INT_RESP_REG_FULL			0x00000004
#define	C_SDC_INT_DATABUF_FULL			0x00000008
#define	C_SDC_INT_DATABUF_EMPTY			0x00000010
#define	C_SDC_INT_PRESENT				0x00000020
#define	C_SDC_INT_SDIO_INT				0x00000040
#define C_SDC_INT_DMA_INT               0x00000100
#define C_SDC_INT_DMAHIT_INT            0x00000200
#define C_SDC_INT_ALLCOM_INT            0x00000400
#define C_SDC_INT_DAT_TO_INT            (0x1<<23) // williamyeo-sd30-dmablksts-not-match
#define C_SDC_INT_CMD_TO_INT            (0x1<<22) // williamyeo-sd30-dmablksts-not-match

#define SDC_RESPTYPE_NONE 	0		/*!< @brief SD response r0. */
#define SDC_RESPTYPE_R1		1		/*!< @brief SD response r1. */
#define SDC_RESPTYPE_R2		2		/*!< @brief SD response r2. */
#define SDC_RESPTYPE_R3		3		/*!< @brief SD response r3. */
#define SDC_RESPTYPE_R4		4		/*!< @brief SD response r4. */
#define SDC_RESPTYPE_R5		5		/*!< @brief SD response r5. */
#define SDC_RESPTYPE_R6		6		/*!< @brief SD response r6. */
#define SDC_RESPTYPE_R7		7		/*!< @brief SD response r7. */
#define SDC_RESPTYPE_R1b	8		/*!< @brief SD response r1b. */

#define SDC_READ			0		/*!< @brief SD data direction: read. */
#define SDC_WRITE			1		/*!< @brief SD data direction: write. */

#define SD_V18              0
#define SD_V33              1
#define SD_PWR_OFF          0
#define SD_PWR_ON           1

#define SDC_CTL2_DELAYSEL_BIT			0  // 3 bit
#define	SDC_CTL2_INVSEL_BIT		        3
#define	SDC_CTL2_SDMODE_BIT			    4  // 2 bit
#define	SDC_CTL2_CLKLOW_BIT			    6
#define	SDC_CTL2_DMA_NOAUTOSTOP_BIT		7
#define	SDC_CTL2_SLOWCLK_BIT			8  // 8 bit
#define	SDC_CTL2_CLKDIVH_BIT			16 // 4 bit
#define SDC_CTL2_IN_CLK_INV             20 // 1 bit
#define SDC_CTL2_IN_CLK_SEL             21 // 1 bit

#define SDC_CTL2_DELAYSEL_OFF			0
#define SDC_CTL2_DELAYSEL_1_STEP		1
#define SDC_CTL2_DELAYSEL_2_STEP		2
#define SDC_CTL2_DELAYSEL_3_STEP		3
#define SDC_CTL2_DELAYSEL_4_STEP		4
#define SDC_CTL2_DELAYSEL_5_STEP		5
#define SDC_CTL2_DELAYSEL_6_STEP		6
#define SDC_CTL2_DELAYSEL_7_STEP		7

#define SDC_CTL2_STD_SPEED			(0L << SDC_CTL2_SDMODE_BIT)
#define SDC_CTL2_HIGH_SPEED			(1L << SDC_CTL2_SDMODE_BIT)
#define SDC_CTL2_DDR_SPEED			(2L << SDC_CTL2_SDMODE_BIT) // not avail on both sd20, sd30

#define SDC_ENABLE_INVSEL			(1L<<SDC_CTL2_INVSEL_BIT)
#define SDC_ENABLE_CLKLOW			(1L<<SDC_CTL2_CLKLOW_BIT)
#define SDC_ENABLE_DMA_NOAUTOSTOP		(1L<<SDC_CTL2_DMA_NOAUTOSTOP_BIT)

#define SDC_DISABLE_INVSEL			(~SDC_ENABLE_INVSEL)
#define SDC_DISABLE_CLKLOW			(~SDC_ENABLE_CLKLOW)
#define SDC_DISABLE_DMA_NOAUTOSTOP		(~SDC_ENABLE_DMA_NOAUTOSTOP)

#define SDC_IN_CLK_SEL_SD_HOST      (1UL << SDC_CTL2_IN_CLK_SEL)
#define SDC_IN_CLK_SEL_IOPAD        (~(1UL << SDC_CTL2_IN_CLK_SEL))

#define SDC_IN_CLK_DO_INV           (1UL << SDC_CTL2_IN_CLK_INV)
#define SDC_IN_CLK_SEL_NOT_INV      (~(1UL << SDC_CTL2_IN_CLK_INV))


#define SDC_ENABLE_ALLCOM_INT                   (0x1<<10) // williamyeo-sd30-dmablksts-not-match
#define SDC_ENABLE_DMACOM_INT                   (0x1<<8) // williamyeo-sd30-dmablksts-not-match


// SD Controller: APIs for SD controller
INT32S drvl1_sdc_init(INT32U device_id, INT8U vic_num);
void drvl1_sdc_restore_COP(INT32U device_id);
INT8U drvl1_sdc_COP_total_step_get(INT32U device_id);
void drvl1_sdc_COP_set(INT32U device_id, INT8U step);
INT8U drvl1_sdc_COP_get(INT32U device_id);
INT32S drvl1_sdc_enable(INT32U device_id);
void drvl1_sdc_set_command_timeout_ext_wait(INT32U device_id, INT8U enable);
INT32S drvl1_sdc_disable(INT32U device_id);
INT32S drvl1_sdc_controller_busy_wait(INT32U device_id, INT32U timeout);
void drvl1_sdc_running_clock_rate_get(INT32U device_id, INT32U *p_rate, INT32U *p_souce_rate, INT32U *p_div);
INT32S drvl1_sdc_clock_set(INT32U device_id, INT32U speed);
INT32S drvl1_sdc_block_len_set(INT32U device_id, INT32U size);
INT32S drvl1_sdc_bus_width_set(INT32U device_id, INT8U width);
INT32S drvl1_sdc_stop_controller(INT32U device_id, INT32U timeout);
void drvl1_sdc_dummyclk_en(INT32U device_id, INT8U en);							//This function only work on GPL326XXB
void drvl1_sdc_ioint_en(INT32U device_id, INT8U en);
void drvl1_sdc_int_en(INT32U device_id, INT32U mask, INT8U en);
INT32S drvl1_sdc_sdio_int_check(INT32U device_id);
void drvl1_sdc_sdio_int_clear(INT32U device_id);
INT32S drvl1_sdc_int_en_check(INT32U device_id, INT32U mask);

// SD Controller: APIs for SD card
extern INT32S drvl1_sdc_card_init_74_cycles(INT32U device_id);
extern INT32S drvl1_sdc_card_busy_wait(INT32U device_id, INT32U timeout);
extern INT32S drvl1_sdc_command_send(INT32U device_id, INT32U command, INT32U argument);
extern INT32S drvl1_sdc_response_get(INT32U device_id, INT32U *response, INT32U timeout);
extern INT32S drvl1_sdc_data_get(INT32U device_id, INT32U *buf, INT32U timeout);
extern INT32S drvl1_sdc_command_complete_wait(INT32U device_id, INT32U timeout);
extern void drvl1_sdc_clear_rx_data_register(INT32U device_id);
extern INT32S drvl1_sdc_read_byte_by_dma(INT32U device_id, INT32U *buffer, INT32U byte_count);
extern INT32S drvl1_sdc_write_byte_by_dma(INT32U device_id, INT32U *buffer, INT32U byte_count);
extern INT32S drvl1_sdc_read_data_by_dma(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status);
extern INT32S drvl1_sdc_read_data_by_dma_ext(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status, INT8U polling);
extern INT32S drvl1_sdc_write_data_by_dma(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status);
extern INT32S drvl1_sdc_write_data_by_dma_ext(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status, INT8U polling);
extern INT32S drvl1_sdc_data_complete_wait(INT32U device_id, INT32U timeout);
extern INT32S drvl1_sdc_data_crc_status_get(INT32U device_id);
extern void drvl1_sdc30_sdclk_delay_set(INT32U device_id, INT8U dstep);
extern void drvl1_sdc30_sdclk_delay_offset_set(INT32U device_id, INT8U offset);
extern INT8U drvl1_sdc30_sdclk_delay_offset_result(INT32U device_id);
extern void drvl1_sdc30_sdcmd_delay_set(INT32U device_id, INT8U dstep);
extern void drvl1_sdc30_set_sdmode(INT32U device_id, INT8U mode);
extern void drvl1_sdc30_sddata_delay_set(INT32U device_id, INT8U dstep);
extern INT32S drvl1_sdc30_stop_controller(INT32U device_id, INT32U in_timeout);
extern INT32S drvl1_sdc30_controller_busy_wait(INT32U device_id, INT32U timeout);
extern INT32S drvl1_sdc30_command_complete_wait(INT32U device_id, INT32U timeout);
extern INT32S drvl1_sdc30_enable(INT32U device_id);
extern INT32S drvl1_sdc30_init(INT32U device_id, INT8U vic_num);
extern INT32S drvl1_sdc30_card_init_74_cycles(INT32U device_id);
extern INT32S drvl1_sdc30_card_busy_wait(INT32U device_id, INT32U timeout);
extern void drv_l1_sdc30_clklow(INT32U device_id, INT8U EN);
extern void drv_l1_sdc30_switch_voltage18(INT32U device_id);
extern void drvl1_sdc30_dummyclk_en(INT32U device_id, INT8U en);
extern void drvl1_sdc30_running_clock_rate_get(INT32U device_id, INT32U *p_rate, INT32U *p_souce_rate, INT32U *p_div);
extern INT32S drvl1_sdc30_clock_set(INT32U device_id, INT32U speed);
extern INT32S drvl1_sdc30_bus_width_set(INT32U device_id, INT8U width);
extern INT32S drvl1_sdc30_command_send(INT32U device_id, INT32U command, INT32U argument);
extern INT32S drvl1_sdc30_response_get(INT32U device_id, INT32U *response, INT32U timeout);
extern INT32S drvl1_sdc30_clear_timeout_bit(INT32U device_id, INT32U poll_cnt);
extern void drvl1_sdc30_clear_rx_data_register(INT32U device_id);
extern INT32S drvl1_sdc30_data_get(INT32U device_id, INT32U *buf, INT32U timeout);
extern INT32S drvl1_sdc30_data_complete_wait(INT32U device_id, INT32U timeout);
extern INT32S drvl1_sdc30_block_len_set(INT32U device_id, INT32U size);
extern void drvl1_sdc30_set_command_timeout_ext_wait(INT32U device_id, INT8U enable);
extern INT32S drvl1_sdc30_disable(INT32U device_id);
extern INT32S drvl1_sdc30_read_data_by_dma_ext(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status, INT8U polling);
extern INT32S drvl1_sdc30_data_crc_status_get(INT32U device_id);
extern INT32S drvl1_sdc30_read_data_by_dma(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status);
extern INT32S drvl1_sdc30_write_data_by_dma_ext(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status, INT8U polling);
extern INT32S drvl1_sdc30_write_data_by_dma(INT32U device_id, INT32U *buffer, INT32U sector_count, INT8S *poll_status);

extern INT32S drvl1_sdc_command_issue(sd_l1_ctrl_t *ctrl);

extern INT32U drvl1_sdc_cal_write_timeout(INT32U sector_count);
extern INT32U drvl1_sdc_cal_read_timeout(INT32U sector_count);

extern void drvl1_sdc_set_print_status_fail_flag(INT32U device_id, INT8U enable);

extern INT32S drvl1_sdc_clear_timeout_bit(INT32U device_id, INT32U poll_cnt);

extern INT32S drvl1_sdc_card_present_check_callback_set(INT32U device_id, sd_card_present_check_callback user_card_present_check_proc); // when use this api, must call before drvl2_sd_init
extern sd_card_present_check_callback drvl1_sdc_card_present_check_callback_get(INT32U device_id); // inside used
extern INT8U drvl1_sdc_check_card_present(INT32U device_id); // inside used, this api call callback set by drvl1_sdc_card_present_check_callback_get
extern void drvl1_sdc_set_data_timeout(INT32U device_id, INT8U enable);
extern void drvl1_sdc_set_cmd_timeout(INT32U device_id, INT8U enable);

extern INT32S drvl1_sdc_status_clear_wait(INT32U device_id, INT32U pollbit, INT32U timeout);
extern INT32S drvl1_sdc_status_set_wait(INT32U device_id, INT32U pollbit, INT32U timeout);
extern void drvl1_sdcbus_ldo_pwr_sel(INT32U device_id, uint8_t v33);

extern void drvl1_sdc30_set_read_start(INT32U start, INT32U nsect); // williamyeo-sd30-dmablksts-not-match
extern void drvl1_sdc30_save_sd30_reg(void); // williamyeo-sd30-dmablksts-not-match-2
extern void drvl1_sdc30_restore_sd30_reg(void); // williamyeo-sd30-dmablksts-not-match-2
extern void drvl1_sdc30_module_reset(void);
extern void drvl1_sdc_module_reset(void);
extern void drv_l1_sdc30_switch_voltage33();
extern void drv_l1_sdc30_switch_voltage18(INT32U device_id);
extern void drvl1_sdc30_tune_timing_print(INT32U device_id);
extern void drvl1_sdc30_in_clk_sel(INT32U device_id, INT8U sel);
extern void drvl1_sdc30_in_clk_inv(INT32U device_id, INT8U inv);
extern void drvl1_sdc30_all_delay_set(INT32U device_id, INT8U sel, INT8U dstep);

#endif		// __DRV_L1_SDC_H__

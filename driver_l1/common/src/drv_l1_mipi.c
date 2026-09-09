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
#ifndef DISABLE
#define DISABLE     0
#endif

#ifndef ENABLE
#define ENABLE      1
#endif

#define MIPI_B_SUPPORT   1

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct mipiReg_s
{
	volatile INT32U	GLB_CSR;		//0x00
	volatile INT32U	PHY_RST;		//0x04
	volatile INT32U	RC_CTRL;		//0x08
	volatile INT32U	ECC_ORDER;		//0x0C

	volatile INT32U	CCIR601_TIMING;	//0x10
	volatile INT32U	IMG_SIZE;		//0x14
	volatile INT32U	INIT_TIME;	    //0x18
    volatile INT32U	RESERVED0;	    //0x1C
	volatile INT32U	DATA_FMT;		//0x20
	volatile INT32U	PAYLOAD_HEADER;	//0x24
	volatile INT32U	CTRL_STATE;		//0x28
	volatile INT32U	VLK_CHECK;		//0x2C

	volatile INT32U	HEADER_DATA;	//0x30
	volatile INT32U	HEADER_DT_VLD;	//0x34
	volatile INT32U	RESERVED1[2];	//0x38 ~ 0x3C

	volatile INT32U INT_ENABLE;		//0x40
	volatile INT32U	RESERVED2[3];	//0x44 ~ 0x4C

	volatile INT32U	RESERVED3[12];	//0x50 ~ 0x7C

	volatile INT32U INT_SOURCE;		//0x80
	volatile INT32U	RESERVED4[3];	//0x84 ~ 0x8C

	volatile INT32U	RESERVED5[12];	//0x90 ~ 0xBC

	volatile INT32U	INT_STATUS;		//0xC0

	volatile INT32U	VC_CTRL;        //0xC4 Virtual channel control
	volatile INT32U	VC_CH1_TIMING;	//0xC8
	volatile INT32U	VC_CH1_SIZE;	//0xCC
	volatile INT32U	VC_CH2_TIMING;	//0xD0
	volatile INT32U	VC_CH2_SIZE;	//0xD4
	volatile INT32U	VC_CH3_TIMING;	//0xD8
	volatile INT32U	VC_CH3_SIZE;	//0xDC
} mipiReg_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

static void (*mipi_isr_callback)(INT32U dev_no, INT32U event);
#if MIPI_B_SUPPORT == 1
static void (*mipi_isr_callback_B)(INT32U dev_no, INT32U event);
#endif


/**
 * @brief   Get MIPI SFR base address
 * @param   DevNo: MIPI device number, can use the definition of MIPI_0 or MIPI_1
 * @return 	result: non zero: success, 0:fail
 */
static mipiReg_t* mipi_select_sfr_base(INT32S DevNo)
{
    if(DevNo == MIPI_0) {
        return (mipiReg_t *)MIPI0_BASE;
    }

    if(DevNo == MIPI_1) {
        return (mipiReg_t *)MIPI1_BASE;
    }

	return 0;
}

/**
 * @brief   MIPI 0(A) interrupt handler
 * @param   none
 * @return 	none
 */
void MIPI_IRQHandler(void)
{
	INT32U IRQ_Status, IRQ_Enable;
	mipiReg_t *pDev;

    pDev = mipi_select_sfr_base(MIPI_0);

    IRQ_Status = pDev->INT_STATUS;
    IRQ_Enable = pDev->INT_ENABLE;

    if(IRQ_Status) {
        // clear interrupt
        pDev->INT_SOURCE = IRQ_Status;

        IRQ_Status &= IRQ_Enable;
        if(mipi_isr_callback && IRQ_Status) {
            mipi_isr_callback(MIPI_0, IRQ_Status);
        }
    }

}

#if MIPI_B_SUPPORT == 1
/**
 * @brief   MIPI 1(B) interrupt handler
 * @param   none
 * @return 	none
 */
void MIPI_IRQHandler_B(void)
{
	INT32U IRQ_Status, IRQ_Enable;
	mipiReg_t *pDev;

    pDev = mipi_select_sfr_base(MIPI_1);

    IRQ_Status = pDev->INT_STATUS;
    IRQ_Enable = pDev->INT_ENABLE;

    if(IRQ_Status) {
        // clear interrupt
        pDev->INT_SOURCE = IRQ_Status;

        IRQ_Status &= IRQ_Enable;
        if(mipi_isr_callback_B && IRQ_Status) {
            mipi_isr_callback_B(MIPI_1, IRQ_Status);
        }
    }

}
#endif

/**
 * @brief   Reset the MIPI PHY
 * @param   dev_no: MIPI device number, can use the definition of MIPI_0 or MIPI_1
 * @return 	none
 */
static void drv_l1_mipi_phy_reset(INT32U dev_no)
{
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

	pDev->PHY_RST |=0x3; //SOF_RST_DA & CK
}

/**
 * @brief   Enables or disables the MIPI
 * @param   dev_no[in]: MIPI device number, can use the definition of MIPI_0 or MIPI_1
 * @param   enable[in]: MIPI enable or disable,1:enable, 0:disable
 * @return 	none
 */
void drv_l1_mipi_on(INT32U dev_no, INT8U enable)
{
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

    pDev->GLB_CSR &= ~0x01;

	if(enable == 0) {
		pDev->GLB_CSR &= 0x01;
		pDev->INT_SOURCE = 0x3F;
		pDev->INT_ENABLE = 0;
		return;
	}

	pDev->GLB_CSR |= 0x01;
}

/**
 * @brief   This function initiate or un-initiate the MIPI
 * @param   dev_no[in]: MIPI device number, 0 for MIPI 0(A) or 1 for MIPI 1(B), can use the definition of MIPI_0 or MIPI_1
 * @param   enable[in]: 1:enable, 0:disable
 * @param   vic_num : 0:IRQ, 1:FIQ
 * @return 	none
 */
void drv_l1_mipi_init(INT32U dev_no, INT8U enable, INT8U vic_num)
{
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

	if(enable == 0) {
		pDev->GLB_CSR = 0;
		pDev->INT_SOURCE = 0x3F;
		pDev->INT_ENABLE = 0;

        if(dev_no == 0) {
            vic_irq_disable(VIC_MIPI);
            vic_fiq_disable(VIC_MIPI);
            drv_l1_clock_set_system_clk_en(CLK_EN7_MIPI_RX_A,0);
        #if MIPI_B_SUPPORT == 1
        } else {
             vic_irq_disable(VIC_MIPI_B);
             vic_fiq_disable(VIC_MIPI_B);
            drv_l1_clock_set_system_clk_en(CLK_EN7_MIPI_RX_B,0);
        }
        #endif
		return;
	}

    if(dev_no == 0)
        drv_l1_clock_set_system_clk_en(CLK_EN7_MIPI_RX_A,1);
#if MIPI_B_SUPPORT == 1
    else
        drv_l1_clock_set_system_clk_en(CLK_EN7_MIPI_RX_B,1);
#endif

	pDev->GLB_CSR = 0;		//init mipi must be off then will be can set cfg
	pDev->ECC_ORDER = 0;
	pDev->CCIR601_TIMING = 0;
	pDev->IMG_SIZE = 0;
	pDev->DATA_FMT = 0;
	pDev->INT_SOURCE = 0x3F;
	pDev->INT_ENABLE = 0;
	pDev->INIT_TIME = 0;

	if(dev_no == 0) {
		vic_irq_register(VIC_MIPI, MIPI_IRQHandler);
		vic_irq_enable(VIC_MIPI);
		if(vic_num)
			vic_fiq_enable(VIC_MIPI); // enable the FIQ
		else
			vic_fiq_disable(VIC_MIPI); //just use the IRQ
	} else {
	#if MIPI_B_SUPPORT == 1
		vic_irq_register(VIC_MIPI_B, MIPI_IRQHandler_B);
		vic_irq_enable(VIC_MIPI_B);
		if(vic_num)
			vic_fiq_enable(VIC_MIPI_B);
		else
			vic_fiq_disable(VIC_MIPI_B);
	#endif
	}
}

/**
 * @brief   MIPI 0(A) isr function register
 * @param   user_isr[in]: isr function
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S drv_l1_mipi_isr_register(void (*user_isr_callback)(INT32U dev_no, INT32U event))
{
	if(user_isr_callback == 0) {
		return STATUS_FAIL;
	}

	mipi_isr_callback = user_isr_callback;
	return STATUS_OK;
}

#if MIPI_B_SUPPORT == 1
/**
 * @brief   MIPI 1(B) isr function register
 * @param   user_isr[in]: isr function
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S drv_l1_mipi_B_isr_register(void (*user_isr_callback)(INT32U dev_no, INT32U event))
{
	if(user_isr_callback == 0) {
		return STATUS_FAIL;
	}

	mipi_isr_callback_B = user_isr_callback;
	return STATUS_OK;
}
#endif
/**
 * @brief   MIPI 0(A) and MIPI 1(B) isr function unregister
 * @param   none
 * @return 	none
 */
void drv_l1_mipi_isr_unregister(void)
{
	mipi_isr_callback = 0;
#if MIPI_B_SUPPORT == 1
	mipi_isr_callback_B = 0;
#endif
}

/**
 * @brief   Enbable or disable the MIPI interrupt
 * @param   dev_no[in]: mipi device number, 0 for MIPI 0(A) or 1 for MIPI 1(B), can use the definition of MIPI_0 or MIPI_1
 * @param   enable[in]: 1:enable, 0:disable
 * @param   index[in]: interrupt bit
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S drv_l1_mipi_set_irq_enable(INT32U dev_no, INT32U enable, INT32U bit)
{
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

	if(enable) {
		pDev->INT_SOURCE |= bit & 0x33F;
		pDev->INT_ENABLE |= bit & 0x33F;
	} else {
		pDev->INT_ENABLE &= ~bit & 0x33F;
		pDev->INT_SOURCE = bit & 0x33F;
	}

	return STATUS_OK;
}

/**
 * @brief   Set MIPI global configuration
 * @param   dev_no[in]: MIPI device number, 0 for MIPI 0(A) or 1 for MIPI 1(B), can use the definition of MIPI_0 or MIPI_1
 * @param   low_power_en[in]: low power enable or disable, 1:enable, 0:disable
 * @param   byte_clk_edge[in]: byt clock edge set, 0: posedge, 1: negedge
 * @return 	none
 */
void drv_l1_mipi_set_global_cfg(INT32U dev_no, INT8U low_power_en, INT8U byte_clk_edge)
{
	INT32U reg;
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

	reg = pDev->GLB_CSR;
	if(low_power_en) {
		reg |= (1<<4);
	} else {
		reg &= ~(1<<4);
	}

	if(byte_clk_edge) {
		reg |= (1<<5);
	} else {
		reg &= ~(1<<5);
	}

	pDev->GLB_CSR = reg;
}

/**
 * @brief   set MIPI lane number
 * @param   dev_no[in]: MIPI device number, 0 for MIPI 0(A) or 1 for MIPI 1(B), can use the definition of MIPI_0 or MIPI_1
 * @param   lane_no[in]: set data lane number to 1,2,4 Lanes, MIPI 0(A) supports 1,2 Lanes and MIPI 1(B) support 1,2,4 Lanes, please use the definition of MIPI_1_LANE, MIPI_2_LANE, MIPI_4_LANE
 * @return 	none
 */
void drv_l1_mipi_set_lane_number(INT32U dev_no, INT8U lane_no)
{
	INT32U reg;
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

	reg = pDev->GLB_CSR;

	if(lane_no == MIPI_4_LANE) // only MIPI 1(B) supports 4 lanes
	{
		reg |= (2<<8);
	}
	else
	if(lane_no == MIPI_2_LANE)
	{
		reg |= (1<<8);
	}
	else
	{
		reg &= ~(1<<8);
	}

	pDev->GLB_CSR = reg;

	/* Release not use gpio, mipi0:IOC6~11, mipi1:IOE0~9    */
	/* mipi0: 1Lane release IOC6,7                          */
	/* mipi1: 1Lane release IOE2,3, IOE6~9                  */
	/* mipi1: 2Lane release IOE6~9                          */
	if(dev_no == MIPI_0)
	{
        R_FUNPOS2 &= ~0x00C00000; //as mipia 2lane

        if(lane_no == MIPI_1_LANE)
        {
            R_FUNPOS2 |= 0x00400000;
            //DBG_PRINT("MIPIA + MIPI_1_LANE\r\n ");
        }
	}
	else if(dev_no == MIPI_1)
	{
        R_FUNPOS2 &= ~0x0f000000; //as mipib 4lane

        if(lane_no == MIPI_1_LANE)
        {
            R_FUNPOS2 |= 0x0E000000;   // use or "|=" to prevent affect other pinmux
            //DBG_PRINT("MIPIB + MIPI_1_LANE\r\n ");
        }
        else if(lane_no == MIPI_2_LANE)
        {
            R_FUNPOS2 |= 0x0C000000;    // use or "|=" to prevent affect other pinmux
            //DBG_PRINT("MIPIB + MIPI_2_LANE\r\n ");
        }
	}

}

/**
 * @brief   set MIPI byte clock source
 * @param   dev_no[in]: MIPI device number, 0 for MIPI 0(A) or 1 for MIPI 1(B), can use the definition of MIPI_0 or MIPI_1
 * @param   byte_clk[in]: 1: use pixel clock 0: use D_PHY byte clock
 * @return 	none
 */
void drv_l1_mipi_set_pixel_clock(INT32U dev_no, INT8U pixel_clk)
{
	INT32U reg;
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

	reg = pDev->GLB_CSR;
	if(pixel_clk) {
		reg |= (1<<12);
	} else {
		reg &= ~(1<<12);
	}

	pDev->GLB_CSR = reg;
}

/**
 * @brief   set MIPI ecc configure
 * @param   dev_no[in]: MIPI device number, 0 for MIPI 0(A) or 1 for MIPI 1(B), can use the definition of MIPI_0 or MIPI_1
 * @param   ecc_order[in]: ecc order, MIPI_ECC_ORDER0 ~ MIPI_ECC_ORDER3
 * @param   ecc_check_en[in]: ecc check enable/disable, 1:enable, 0:disable
 * @return 	none
 */
void drv_l1_mipi_set_ecc(INT32U dev_no, INT8U ecc_order, INT8U ecc_check_en)
{
	INT32U reg;
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

	reg = pDev->ECC_ORDER;
	reg &= ~0x7;
	reg |= ((ecc_check_en & 0x01) << 2) | (ecc_order & 0x03);
	pDev->ECC_ORDER = reg;
}

/**
 * @brief   set MIPI mask count
 * @param   dev_no[in]: MIPI device number, 0 for MIPI 0(A) or 1 for MIPI 1(B), can use the definition of MIPI_0 or MIPI_1
 * @param   da_mask_cnt[in]: LP to HS mask count
 * @param   check_hs_seq[in]: 1: Check HS sequence. 0: just check LP. when enter HS mode
 * @return 	none
 */
void drv_l1_mipi_set_mask_cnt(INT32U dev_no, INT8U da_mask_cnt, INT8U check_hs_seq)
{
	INT32U reg;
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

	reg = pDev->ECC_ORDER;
	reg &= ~(0x1FF << 8);
	reg |= (((INT32U)check_hs_seq & 0x01) << 16) | (((INT32U)da_mask_cnt & 0xFF) << 8);
	pDev->ECC_ORDER = reg;
}

/**
 * @brief   Set MIPI ccir601 interface
 * @param   dev_no[in]: MIPI device number, 0 for MIPI 0(A) or 1 for MIPI 1(B), can use the definition of MIPI_0 or MIPI_1
 * @param   h_back_proch[in]: horizontal back proch
 * @param   h_front_proch[in]: horizontal front proch
 * @param   blanking_line_en[in]: 1: unmask HSYNC when VSYNC=0, 0: mask HSYNC when VSYNC=0
 * @return 	none
 */
void drv_l1_mipi_set_ccir601_if(INT32U dev_no, INT8U h_back_porch, INT8U h_front_porch, INT8U blanking_line_en)
{
	INT32U reg;
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

	reg = pDev->CCIR601_TIMING;
	reg &= ~(0x0F << 0);
	reg |= (h_back_porch & 0x0F);

	reg &= ~(0x0F << 8);
	reg |= ((INT32U)(h_front_porch & 0x0F) << 8);

	if(blanking_line_en) {
		reg |= (1<<16);
	} else {
		reg &= ~(1<<16);
	}

	pDev->CCIR601_TIMING = reg;
}

/**
 * @brief   Set MIPI image size
 * @param   dev_no[in]: MIPI device number, 0 for MIPI 0(A) or 1 for MIPI 1(B), can use the definition of MIPI_0 or MIPI_1
 * @param   h_size[in]: horizontal size set
 * @param   v_size[in]: vertical size set
 * @return 	none
 */
void drv_l1_mipi_set_image_size(INT32U dev_no, INT16U h_size, INT16U v_size)
{
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

	if(h_size == 0) {
		h_size = 1;
	}

	if(v_size == 0) {
		v_size = 1;
	}

	h_size &= 0xFFFF;
	v_size &= 0xFFFF;
	pDev->IMG_SIZE = h_size | ((INT32U)v_size << 16);
}

/**
 * @brief   Set MIPI data format
 * @param   dev_no[in]: mipi device number, 0 for MIPI 0(A) or 1 for MIPI 1(B), can use the definition of MIPI_0 or MIPI_1
 * @param   data_from_mmr[in]: 0: auto detect data type, 1: user define
 * @param   data_type_mmr[in]: user define data format. MIPI_YUV422 ~ MIPI_RAW12
 * @param   data_type_cdsp[in]: for cdsp 10bit data bus, 1:data[7:0]+2':00, 0: 2'00+data[7:0]
 * @return 	none
 */
void drv_l1_mipi_set_data_fmt(INT32U dev_no, INT8U data_from_mmr, INT8U data_type_mmr, INT8U data_type_cdsp)
{
	INT32U reg;
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

	reg = pDev->DATA_FMT;
	if(data_from_mmr) {
		reg |= (1<<0);
	} else {
		reg &= ~(1<<0);
	}

	reg &= ~(0x07 << 4);
	reg |= (data_type_mmr & 0x07) << 4;

	if(data_type_cdsp) {
		reg |= (1 << 7);
	} else {
		reg &= ~(1 << 7);
	}

	pDev->DATA_FMT = reg;
}

/**
 * @brief   set MIPI parameter
 * @param   dev_no[in]: mipi device number, 0 for MIPI 0(A) or 1 for MIPI 1(B), can use the definition of MIPI_0 or MIPI_1
 * @param   argp[in]: parameter
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_mipi_set_parameter(INT32U dev_no, mipi_config_t *argp)
{
	INT32U delta_t, data_mask_cnt;
    INT32U h_size;

	if(argp == 0) {
		return STATUS_FAIL;
	}

	drv_l1_mipi_init(dev_no, ENABLE, 0);
	drv_l1_mipi_set_global_cfg(dev_no, argp->low_power_en, argp->byte_clk_edge);
	drv_l1_mipi_set_lane_number(dev_no, argp->mipi_lane_no);
	drv_l1_mipi_set_pixel_clock(dev_no, argp->pixel_clk_sel);

	/* change to ns */
	delta_t = 1000*1000000/ (SystemCoreClock / 1024);

	/* must > 40ns */
	if(argp->data_mask_time < 40) {
		argp->data_mask_time = 40;
	}

	/* get data mask count */
	data_mask_cnt = argp->data_mask_time*1024 / delta_t;

	data_mask_cnt-=2;
	if(data_mask_cnt <= 0) data_mask_cnt = 1;

	drv_l1_mipi_set_ecc(dev_no, argp->ecc_order, argp->ecc_check_en);
	drv_l1_mipi_set_mask_cnt(dev_no, data_mask_cnt, argp->check_hs_seq);
	drv_l1_mipi_set_ccir601_if(dev_no, argp->h_back_porch, argp->h_front_porch, argp->blanking_line_en);
	if(argp->pixel4 == ENABLE) {
        drv_l1_mipi_set_4pixel_mode(dev_no, ENABLE);
        h_size = argp->h_size/4;
     } else {
        drv_l1_mipi_set_4pixel_mode(dev_no, DISABLE);
        h_size = argp->h_size;
    }

	if(argp->data_from_mmr) {
		// user define format
		drv_l1_mipi_set_data_fmt(dev_no, argp->data_from_mmr, argp->data_type, argp->data_type_to_cdsp);
		if(argp->data_type == MIPI_YUV422) {
			drv_l1_mipi_set_image_size(dev_no, h_size << 1, argp->v_size);
		} else {
			drv_l1_mipi_set_image_size(dev_no, h_size, argp->v_size);
		}
	} else {
		// auto detect
		drv_l1_mipi_set_data_fmt(dev_no, 0, 0, argp->data_type_to_cdsp);
		drv_l1_mipi_set_image_size(dev_no, h_size, argp->v_size);
	}

	drv_l1_mipi_phy_reset(dev_no);
    drv_l1_mipi_on(dev_no, ENABLE);		//cfg done, then mipi on
	return STATUS_OK;
}

/**
 * @brief   Check MIPI header
 * @param   dev_no[in]: mipi device number, 0 for MIPI 0(A) or 1 for MIPI 1(B), can use the definition of MIPI_0 or MIPI_1
 * @param   header_cmp[in]: header which check value
 * @param   count[in]: compare times
 * @return 	fail_cnt
 */
INT32U drv_l1_mipi_check_header(INT32U dev_no, INT32U header_cmp, INT32U count)
{
    mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

    volatile INT32U header, read_reg;
    volatile INT16U i = 0;
    volatile INT32U pass_ent = 0, fail_cnt = 0;
    while(i<count)
    {
        pDev->PHY_RST = 1;//reset
        pDev->HEADER_DT_VLD = 1;//trigger read
        read_reg = pDev->HEADER_DT_VLD;
        while(read_reg == 0)
        {
            read_reg = pDev->HEADER_DT_VLD;
        }
        header = pDev->HEADER_DATA;//get header

        if(header == header_cmp) {
            pass_ent++;
            break;
        }
        else{
            //DBG_PRINT("[%d][%d]0x%x\r\n",i ,fail_cnt, header);
            fail_cnt++;
        }
        i++;
    }

    return fail_cnt;
}

/**
 * @brief   Enables or disables MIPI virtual channel
 * @param   dev_no[in]: MIPI device number, can use the definition of MIPI_0 or MIPI_1
 * @param   enable[in]: 1:enable/0:disable virtual channel
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_mipi_set_vc_enable(INT32U dev_no, INT8U enable)
{
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

    if(enable == ENABLE)
        pDev->VC_CTRL |= (0x1<<0);
    else
        pDev->VC_CTRL &= ~(0x1<<0);

    return STATUS_OK;
}

/**
 * @brief   Set MIPI virtual channel data output set
 * @param   dev_no[in]: mipi device number, can use the definition of MIPI_0 or MIPI_1
 * @param   ch_data[in]: [7:6]CH3_data,[5:4]CH2_data,[3:2]CH1_data,[1:0]CH0_data, 0~3:virtual channel, the MIPI 0(A) is only valided in [3:2] and [1:0]
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_mipi_set_vc_channel_data_set(INT32U dev_no, INT8U ch_data)
{
	INT32U reg;
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

    reg = pDev->VC_CTRL;
    reg &= ~(0xFF<<8);
    reg |= (ch_data&0xFF)<<8;
    pDev->VC_CTRL = reg;

    return STATUS_OK;

}

/**
 * @brief   Set MIPI virtual channel 0~3 front/back porch
 * @param   dev_no[in]: mipi device number, can use the definition of MIPI_0 or MIPI_1
 * @param   channel[in]: channel: 0~3, MIPI 0(A) supports 2 virtual channel(0~1) and MIPI 1(B) supports 4 virtual channel(0~3)
 * @param   h_front_proch[in]: horizontal front proch
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_mipi_set_vc_config(INT32U dev_no, INT8U channel, INT8U h_back_porch, INT8U h_front_porch)
{
	INT32U reg;
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

    if ((dev_no == MIPI_0) && (channel > 1)) {
        return STATUS_FAIL;
    }

    switch(channel)
    {
        case 0:
            reg = pDev->CCIR601_TIMING;
            reg &= ~(0x0F << 0);
            reg |= (h_back_porch & 0x0F);
            reg &= ~(0x0F << 8);
            reg |= ((INT32U)(h_front_porch & 0x0F) << 8);
            pDev->CCIR601_TIMING = reg;
        break;
        case 1:
            reg = pDev->VC_CH1_TIMING;
            reg &= ~(0x0F << 0);
            reg |= (h_back_porch & 0x0F);
            reg &= ~(0x0F << 8);
            reg |= ((INT32U)(h_front_porch & 0x0F) << 8);
            pDev->VC_CH1_TIMING = reg;
        break;
        case 2:
            reg = pDev->VC_CH2_TIMING;
            reg &= ~(0x0F << 0);
            reg |= (h_back_porch & 0x0F);
            reg &= ~(0x0F << 8);
            reg |= ((INT32U)(h_front_porch & 0x0F) << 8);
            pDev->VC_CH2_TIMING = reg;
        break;
        case 3:
            reg = pDev->VC_CH3_TIMING;
            reg &= ~(0x0F << 0);
            reg |= (h_back_porch & 0x0F);
            reg &= ~(0x0F << 8);
            reg |= ((INT32U)(h_front_porch & 0x0F) << 8);
            pDev->VC_CH3_TIMING = reg;
        break;
        default:
            return STATUS_FAIL;
        break;
    }

    return STATUS_OK;
}

/**
 * @brief   Set MIPI virtual channel image size
 * @param   dev_no[in]: MIPI device number, can use the definition of MIPI_0 or MIPI_1
 * @param   channel[in]: channel: 0~3, MIPI 0(A) supports 2 virtual channel(0~1) and MIPI 1(B) supports 4 virtual channel(0~3)
 * @param   h_size[in]: horizontal size set
 * @param   v_size[in]: vertical size set
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_mipi_set_vc_size(INT32U dev_no, INT8U channel, INT16U h_size, INT16U v_size, INT8U data_type)
{
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

    if ((dev_no == MIPI_0) && (channel > 1)) {
        return STATUS_FAIL;
    }

	if(h_size == 0) {
		h_size = 1;
	}

	if(v_size == 0) {
		v_size = 1;
	}

	h_size &= 0xFFFF;
	v_size &= 0xFFFF;

    if(data_type == MIPI_YUV422) {
        h_size = h_size << 1;
    }

	switch(channel)
    {
        case 0:
            pDev->IMG_SIZE = h_size | ((INT32U)v_size << 16);
        break;
        case 1:
            pDev->VC_CH1_SIZE = h_size | ((INT32U)v_size << 16);
        break;
        case 2:
            pDev->VC_CH2_SIZE = h_size | ((INT32U)v_size << 16);
        break;
        case 3:
            pDev->VC_CH3_SIZE = h_size | ((INT32U)v_size << 16);
        break;
        default:
            return STATUS_FAIL;
        break;
    }
    return STATUS_OK;
}

/**
 * @brief   Set MIPI to 4 pixel mode
 * @param   dev_no[in]: MIPI device number, can use the definition of MIPI_0 or MIPI_1
 * @param   enable[in]: 1:enable, 0:disable
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_mipi_set_4pixel_mode(INT32U dev_no, INT8U enable)
{
	mipiReg_t *pDev = mipi_select_sfr_base(dev_no);

    if(enable)
        pDev->GLB_CSR |= (0x1<<15);
    else
        pDev->GLB_CSR &= ~(0x1<<15);

    return STATUS_OK;
}

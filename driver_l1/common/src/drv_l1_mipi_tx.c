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

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct mipi_tx_Reg_s
{
	volatile INT32U	VM_HT_CTRL;		        //0x00 //Horizontal Timing Control
	volatile INT32U	VM_VT0_CTRL;		    //0x04 //Vertical 0 Timing Control
	volatile INT32U	VM_VT1_CTRL;		    //0x08 //Vertical 1 Timing Control
	volatile INT32U	VM_BLLP_CTRL;		    //0x0C //BLLP Timing

	volatile INT32U	LPCK_CTRL;	            //0x10 //Low Power clock divider
	volatile INT32U	LANE_TIMING_CTRL;		//0x14 //Timing Control for CLK/Data Lane
	volatile INT32U	CL_TM0_CTRL;	        //0x18 //Clock Lane Timing Control
    volatile INT32U	CL_TM1_CTRL;	        //0x1C //Clock Lane Timing Control

	volatile INT32U	DL_TM_CTRL;		        //0x20 //Data Lane Timing Control
	volatile INT32U	SPKT_HD_CTRL;	        //0x24 //Short Packet Header
	volatile INT32U	LPKT_HD_CTRL;		    //0x28 //Long Packet Header
	volatile INT32U	LPKT_PLD_CTRL;		    //0x2C //Long Packet Payload Control

	volatile INT32U	VM_INPUT_CTRL;	        //0x30 //Video Mode Input format
	volatile INT32U	BLK_CTRL;	            //0x34 //Operation Mode Control for Blanking Period
	volatile INT32U	OP_CTRL;	            //0x38 //Operation control
	volatile INT32U	CTRL;                   //0x3C

	volatile INT32U FIFO_STATUS;		    //0x40
	volatile INT32U	BTA_CTRL;	            //0x44
    volatile INT32U	LANE_SWAP;	            //0x48
    volatile INT32U	RGBWC_CTRL;	            //0x4C

	volatile INT32U	RTN_RD_PORT;	        //0x50 //Return Data Entry Port
    volatile INT32U	ACKERR_RSP;	            //0x54 //Ack Response Status Register
    volatile INT32U	RSP_DBC;	            //0x58 //Response Data Byte Count
    volatile INT32U	RSV1;	                //0x5C

	volatile INT32U INT_STATUS;		        //0x60 //Interrupt Status
	volatile INT32U	INT_EN_CTRL;	        //0x64 //Interrupt Enable
	volatile INT32U	ERR_STATUS;             //0x68 //Error Interrupt Status
	volatile INT32U	RSV2;                   //0x6C

	volatile INT32U	STATUS;	                //0x70
	volatile INT32U	WUD_CTRL;               //0x74
	volatile INT32U	CK_CTRL;                //0x78
	volatile INT32U	RSV3;                   //0x7C

    volatile INT32U	RSV4[4];                //0x80~8C

	volatile INT32U	PHY_CTRL;		        //0x90
} mipi_tx_Reg_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
//static void (*mipi_tx_isr_callback)(INT32U event);
static INT32U g_dsi_clock, g_tmds_clock;

static mipi_tx_Reg_t* mipi_tx_select_sfr_base()
{
    return (mipi_tx_Reg_t *)MIPI_DSI_BASE;
}

void mipi_short_packet_write(MIPI_Short_Command_Table_t* short_table)
{
	INT32U payload;

	if(short_table->data_cnt == 1)
	{
		//payload = packet_type | (short_table->data[0] << 8);
		payload = short_table->packet_type | (short_table->data[0] << 8);
	}
	else if(short_table->data_cnt == 2)
	{
		//payload = packet_type | (short_table->data[0] << 8) | (short_table->data[1] << 16);
		payload = short_table->packet_type | (short_table->data[0] << 8) | (short_table->data[1] << 16);
	}
	else
        return;

	R_DSI_SPKT_HD_CTRL = payload;
	//DBG_PRINT("MIPI short packet payload 0x%08x\r\n", payload);
}

void mipi_long_packet_write(MIPI_Command_Table_t* table)
{
	INT32U i, j;
	INT32U payload = 0;

	if(table->packet_type == MIPI_GENERIC_LONG_WRITE || table->packet_type == MIPI_DCS_LONG_WRITE)
	{
		R_DSI_LPKT_HD_CTRL = table->packet_type | (table->data_cnt << 8);
		//DBG_PRINT("MIPI long packet write header 0x%08x\r\n",  packet_type | (table->data_cnt << 8));

		for(i=0; i<table->data_cnt; i+=4)
		{
			j = table->data_cnt-i;
			if(j == 1)
				payload = *(table->data+i);
			else if(j == 2)
				payload = *(table->data+i) | (*(table->data+i+1) << 8);
			else if(j == 3)
				payload = *(table->data+i) | (*(table->data+i+1) << 8) | (*(table->data+i+2) << 16);
			else
				payload = *(table->data+i) | (*(table->data+i+1) << 8) | (*(table->data+i+2) << 16) | (*(table->data+i+3) << 24);

			//DBG_PRINT("MIPI long packet write payload 0x%08x\r\n", payload);
			R_DSI_LPKT_PLD_CTRL = payload;
		}
	}
	else if(table->packet_type == MIPI_GENERIC_SHORT_WRITE_0P || table->packet_type == MIPI_GENERIC_SHORT_WRITE_1P || table->packet_type == MIPI_GENERIC_SHORT_WRITE_2P
			|| table->packet_type == MIPI_DCS_SHORT_WRITE_0P || table->packet_type == MIPI_DCS_SHORT_WRITE_1P)
	{
		if(table->data_cnt == 1)
		{
			payload = table->packet_type | (table->data[0] << 8);
		}
		else if(table->data_cnt == 2)
		{
			payload = table->packet_type | (table->data[0] << 8) | (table->data[1] << 16);
		}
		//DBG_PRINT("MIPI long packet write payload 0x%08x\r\n", payload);
		R_DSI_SPKT_HD_CTRL = payload;
	}
}

void mipi_write_lcd_command(void* table, INT32U cnt, INT32U type)
{
	INT32U i, j;
	INT8U cmd, delay_cnt;
	MIPI_Command_Table_t *cmd_table;
	MIPI_Short_Command_Table_t *short_cmd_table;

	if(type == PACKET_SHORT_WRITE)
	{
		short_cmd_table = (MIPI_Short_Command_Table_t*)table;
		cmd = short_cmd_table->data[0];
		cmd_table = NULL;
	}
	else// if(type == PACKET_LONG_WRITE), just using PACKET_LONG_WRITE if not PACKET_SHORT_WRITE command
	{
		cmd_table = (MIPI_Command_Table_t*)table;
		cmd = cmd_table->data[0];
		short_cmd_table = NULL;
	}
	//else
    //    return;

	//DBG_PRINT("\r\nmipi_write_lcd_command,table 0x%x cmd 0x%x cnt %d\r\n", table, cmd, cnt);
	for(i=0; i<cnt; i++)
	{
		switch(cmd)
		{
			case CMD_DELAY:
				if(type == PACKET_SHORT_WRITE)
					delay_cnt = short_cmd_table->data_cnt;
				else if(type == PACKET_LONG_WRITE)
					delay_cnt = cmd_table->data_cnt;
                else
                    delay_cnt = 0;

				for(j=0; j<delay_cnt; j++)
				{
					MIPI_DSI_10MS_DELAY(1);
				}
				break;

			default:
				if(type == PACKET_SHORT_WRITE)
				{
					/* Use short packet write method */
					mipi_short_packet_write(short_cmd_table);
				}
				else if(type == PACKET_LONG_WRITE)
				{
					/* Use long packet write method */
					mipi_long_packet_write(cmd_table);
				}
				break;
		}

		if(type == PACKET_SHORT_WRITE)
		{
			//DBG_PRINT("write cmd: table 0x%x, cmd 0x%x , cnt %d\r\n", short_cmd_table, cmd, short_cmd_table->data_cnt);
			if(short_cmd_table != NULL) {
                short_cmd_table++;
                cmd = short_cmd_table->data[0];
			}
		}
		else if(type == PACKET_LONG_WRITE)
		{
            if(cmd_table != NULL) {
                cmd_table++;
                cmd = cmd_table->data[0];
			}
		}
	}
}

/**
 * @brief   mipi lcd software reset
 * @param   delay[in]: unit 10msec (same as osDelay)
 * @return 	none
 */
void mipi_lcd_software_reset(INT16U delay)
{
  	//==============================================
	//Do SW Reset , DCS Standard Command Do Reset
	//==============================================
	//R_DSI_SPKT_HD_CTRL:
	//[7:0]: TYPE
	//[15:8]: DATA0
	//[23:16]: DATA1
	//[31:24]: ECC; R only
	R_DSI_SPKT_HD_CTRL = 0x00000115;
	// wait 5 ms after sw reset, but at least 120 ms for sleep mode

	MIPI_DSI_10MS_DELAY(delay);
}

/* hardware don't connect interrupt source to CPU
void MIPI_TX_IRQHandler(void)
{
	INT32U IRQ_Status, IRQ_Enable;
	mipi_tx_Reg_t *pDev;

    pDev = mipi_tx_select_sfr_base();

    IRQ_Status = pDev->INT_STATUS;
    IRQ_Enable = pDev->INT_EN_CTRL;

    if(IRQ_Status) {
        // clear interrupt
        pDev->INT_EN_CTRL = IRQ_Status;

        IRQ_Status &= IRQ_Enable;
        if(mipi_tx_isr_callback && IRQ_Status) {
            mipi_tx_isr_callback(IRQ_Status);
        }
    }
}
*/

/**
 * @brief   mipi tx module clock enable/disable
 * @param   enable[in]: 1:enable
 * @return 	none
 */
void drv_l1_mipi_tx_clock_en(INT8U enable)
{
    drv_l1_clock_set_system_clk_en(CLK_EN6_MIPI_TX, enable);
}
/**
 * @brief   mipi tx phy reset
 * @param   none
 * @return 	none
 */
void drv_l1_mipi_tx_phy_reset(INT8U en)
{
	mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    if(en)
        pDev->OP_CTRL &= ~(0x1<<31);
    else
    	pDev->OP_CTRL |= (0x1<<31);
}

/**
 * @brief   mipi tx lane swap set
 * @param   PN_swap[in]: mipi tx clock and data P/N swap
 * @param   datat_lane[in]: mipi tx data lane 0~3 order,
 * @return 	none
 */
void drv_l1_mipi_tx_lane_swap_set(INT8U PN_swap, INT32U data_lane)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    pDev->LANE_SWAP = ((PN_swap&0x1)<<16) | (data_lane&0xFFFF);
}

/**
 * @brief   mipi tx lane set
 * @param   lane[in]: number of mipi data lane
 * @return 	none
 */
void drv_l1_mipi_tx_lane_set(INT32U lane)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    INT32U reg = pDev->CTRL;
    reg &= ~(0x3<<4);
    reg |= ((lane-1)&0x3)<<4;
    pDev->CTRL = reg;
}

void drv_l1_mipi_tx_enable(INT8U enable)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    INT32U reg = pDev->CTRL;
    if(enable) {
        pDev->OP_CTRL |= (0x1<<31);//release MIPI TX PHY RESET
        //*********************************
        //Analog PHY Enable and DSI Enable
        //*********************************
        //[24]: Analog enable
        //[0]: DSI Enable
        reg |= 0x01000001;
        pDev->CTRL = reg;

        //Wait For Analog PHY OK
        //[24]: PHY is Ready
        reg = pDev->STATUS;
        //while(!(reg & 0x01000000))
        {
            reg = pDev->STATUS;
        }
	} else {
        pDev->CTRL &= ~((0x1<<24)|(0x1<<0));
        pDev->OP_CTRL &= ~(0x1<<31);//hold MIPI TX PHY RESET

	}
}

void drv_l1_mipi_tx_mode_set(INT8U mode)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    if(mode == COMMAND_MODE)
        pDev->CTRL |= (0x1<<1);
    else
        pDev->CTRL &= ~(0x1<<1);
}

void drv_l1_mipi_tx_low_power_mode(INT8U enable)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    //[20]: 1: Enable LP mode transmitter Data
    if(enable)
        pDev->OP_CTRL |= (0x1<<20);
    else
        pDev->OP_CTRL &= ~(0x1<<20);
}

void drv_l1_mipi_tx_Eotp_set(INT8U enable)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    //[12]: 1: Send Out Eotp package at end of HS transmission
    if(enable)
        pDev->CTRL |= (0x1<<12);
    else
        pDev->CTRL &= ~(0x1<<12);
}

// GP328xxxA spec no defined
/*
void drv_l1_mipi_tx_BTA_set(INT8U TA_GET, INT8U TA_SURE, INT8U TA_GO)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    //R_DSI_BTA_CTRL: Bus turn around ,timing Setting
	//[22:20]: TA_GET
	//[18:16]: TA_SURE
	//[8]:??
	//[2:0]: TA_GO
	//======================================
	pDev->BTA_CTRL =( ( (TA_GET&0x7)<<20)|( (TA_SURE&0x7)<<16)|(TA_GO&0x7) );
}
*/

void drv_l1_mipi_tx_blank_mode(INT32U mode)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    //[0]:HAS Blank Mode
	//[4]:HBP Blank Mode
	//[8]:HFP Blank Mode
	//[12]:BLLP Blank Mode
	// R_DSI_BLK_CTRL
	INT32U reg = pDev->BLK_CTRL;
	reg &= ~(BLLP_BLK_MODE | HFP_BLK_MODE | HBP_BLK_MODE | HAS_BLK_MODE);
	reg |= mode;
    pDev->BLK_CTRL = reg;
}

void drv_l1_mipi_tx_video_input_mode(INT8U mode)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();

    INT32U reg = pDev->VM_INPUT_CTRL;
    ///@D150_0030: Input_CTRL
	//[13:12]: Video Mode. 1: Non-Burst mode with Sync Event
	//[5:4]: LP Data Format.  need set 2'h3
	//24bpp,force 24bpp in design.	//Need Write, do event Mode
    reg &= ~(0x3<<12);
    reg |= ((mode&0x3)<<12) | (0x3<<4) ;
    pDev->VM_INPUT_CTRL = reg;
}

void drv_l1_mipi_tx_RGB_word_count(INT16U word_count)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    //@D150_004C:
	//[15:0]:RGB WC ,IF RGB888 Set 3*IM Width
	pDev->RGBWC_CTRL = word_count&0xFFFF;
}

void drv_l1_mipi_tx_wakeup_delay(INT16U delay)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    /*
    These bits specifies the number of clock cycles for wake up delay period.
    TWAKEUP. The delay is used to wake up the MIPI from ULPS state. The clock is the low power clock.
    */
    pDev->WUD_CTRL = delay&0xFFFF;
}

void drv_l1_mipi_tx_phy_frq_sel(INT8U frq)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    INT32U reg = pDev->PHY_CTRL;
    // [10:8]	 = FREQ_SEL	: default=7 = DSI_CLK * 8
	//			000	: < 93.75MHz
	//			001 : < 125MHz
	//			010 : < 187.5MHz
	//			011 : < 250MHz
	//			100 : < 375MHz
	//			101 : < 500MHz
	//			110 : < 750MHz
	//			111	: < 1000MHz
	reg &= ~(0x7<<8);
	reg |= (frq&0x7)<<8;
    pDev->PHY_CTRL = reg;
}

INT32U drv_l1_mipi_tx_get_tmds_clock()
{
    return g_tmds_clock;
}

INT32U drv_l1_mipi_tx_get_dsi_clock()
{
    return g_dsi_clock;
}

void drv_l1_mipi_tx_set_LPCK(INT8U div)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    //===============================================
	//4.LP TX CLK(ESC CLK)
	//================================================
	//R_DSI_LPCK_CTRL:@0xD150_0010
	//[5:0]: LPCD . LP Clock Divider ESC_CLK = DSI_CLK/ (LPCD+1)
    pDev->LPCK_CTRL = div&0x1F;
}

void drv_l1_mipi_tx_h_timing_ctrl(INT32U DSI_HSA, INT32U DSI_HFP, INT32U DSI_HBP)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    //========================================
	//D150_0000@ VM_HT_CTRL
	//[11:0]: 	HBP
	//[23:12]: 	HFP
	//[31:24]: 	HSA
	pDev->VM_HT_CTRL = ((DSI_HSA&0xFFF)<<24)|((DSI_HFP&0xFFF)<<12)|(DSI_HBP&0xFFF);
}

void drv_l1_mipi_tx_v_timing_ctrl(INT32U TFT_VSA, INT32U TFT_VFP, INT32U TFT_VBP, INT32U TFT_VACT)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    //========================================
	//D150_0004@ VM_VT0_CTRL ;unit number line
	//[7:0]: 	VBP
	//[15:8]: 	VFP
	//[23:16]: 	VSA
    //========================================
	//D150_0008@ VM_VT1_CTRL ;unit number line
    //[15:0]: Active line number
   	pDev->VM_VT0_CTRL = ((TFT_VSA&0xFF)<<16)|((TFT_VFP&0xFF)<<8)|(TFT_VBP&0xFF);
   	pDev->VM_VT1_CTRL = TFT_VACT&0xFFFF;
}

//************************************************//
//*DSI Timing Setting
//************************************************//
void drv_l1_mipi_tx_lane_timing_ctrl(INT32U phy_hs_exit, INT32U phy_lpx)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
	//D150_0014@LANE TIMING0
	//[7:0]:    Tlpx		:   Length of LP-00/01/10/11
	//[23:16]:  Ths-exit    :   Time to Deiver LP11 after HS Burst
	pDev->LANE_TIMING_CTRL = ((phy_hs_exit&0xFF)<<16)|(phy_lpx&0xFF);
}

void drv_l1_mipi_tx_clock_lane_timing_ctrl0(INT32U phy_clk_prepare, INT32U phy_clk_zero)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
   	//========================================
	//D150_0018@LANE TIMING1
	//[9:0]:    Tclk_zero   : HS-0 driver period before start clk
	//[24:16]:  Tclk_prepare: time to driver LP00 to prepare for HS-transmisiion for clk
	pDev->CL_TM0_CTRL = ((phy_clk_prepare&0x1FF) << 16)|(phy_clk_zero&0x3FF);
}

void drv_l1_mipi_tx_clock_lane_timing_ctrl1(INT32U phy_clk_trail, INT32U phy_clk_pre, INT32U phy_clk_post)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    //========================================
	//D150_001C@LANE TIMING2
	//[9:0]:Tclk_post: continuout send HS CLK after the last acssociated Data lane has transitioned to LP mode
	//[24:16]: time that the HS Clock drive shall be driven begin the transition from LP to HS mode
	//[31:25]:Tclk_trail: last payload "clk" bit of a HS transmission burst
	pDev->CL_TM1_CTRL = ((phy_clk_trail&0x7F)<<25)|((phy_clk_pre&0x1FF)<<16)|(phy_clk_post&0x3FF);
}

void drv_l1_mipi_tx_data_lane_timing_ctrl(INT32U phy_hs_trail, INT32U phy_hs_prpr, INT32U phy_hs_zero)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
 	//========================================
	//D150_0020@LANE TIMING3
	//[9:0]: hs_zero: for  for convenient, HS_ZERO = 300ns
	//[24:16]: hs_prepare.
	//[31:25]: hs_trail. For Convenient, define HS_TRAIL = 96*UI + 5
	pDev->DL_TM_CTRL = ((phy_hs_trail&0x7F)<<25)|((phy_hs_prpr&0x1FF)<<16)|(phy_hs_zero&0x3FF);
}

//GP328 spec no define
/*
void drv_l1_mipi_tx_phy_vout(INT8U vout)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    INT32U reg = pDev->PHY_CTRL;
    // [14:12] = VOUT_SEL : default=3 = 0.4V
	//	0x0:0.325V	;0x1:0.35V	;0x2:0.375V;	0x3:0.4V;
	//  0x5:0.45V 	;0x6:0.475V	;0x7:0.5V
    reg &= ~(0x7<<12);
    reg |= (vout&0x7)<<12;
    pDev->PHY_CTRL = reg;
}
*/

void drv_l1_mipi_tx_clock_HS_set(INT8U en)
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    //@D150_0078 CK Ctrl
	//[0]: CLK lan HS enable
	//[9:8]:0x0 CLK lan always ON
    pDev->CK_CTRL &= ~(0x3 << 8);
	if(en)
        pDev->CK_CTRL |= 0x1;
    else
        pDev->CK_CTRL &= ~0x1;
}

INT32U drv_l1_mipi_tx_get_status()
{
    mipi_tx_Reg_t *pDev = mipi_tx_select_sfr_base();
    return pDev->STATUS;
}

void drv_l1_mipi_tx_software_module_reset(void)
{
	drv_l1_reset_sysmodule_by_software(BIT13,0x00,0x00,0x00);	/* reset MIPI TX hardware */
}


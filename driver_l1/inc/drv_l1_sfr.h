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

/******************************************************************************
 * Privete Paresent Header
 ******************************************************************************/
#ifndef __DRV_L1_SFR_H__
#define __DRV_L1_SFR_H__

/******************************************************************************
 * GPIO: 0xC0000000
 ******************************************************************************/
#define R_IOA_I_DATA                    (*((volatile INT32U *) 0xC0000000))
#define R_IOA_O_DATA                    (*((volatile INT32U *) 0xC0000004))
#define R_IOA_DIR                       (*((volatile INT32U *) 0xC0000008))
#define R_IOA_ATT                       (*((volatile INT32U *) 0xC000000C))
#define R_IOA_DRV                       (*((volatile INT32U *) 0xC0000010))
#define R_IOA_DRV1                      (*((volatile INT32U *) 0xC0000014))
#define R_IOB_I_DATA                    (*((volatile INT32U *) 0xC0000020))
#define R_IOB_O_DATA                    (*((volatile INT32U *) 0xC0000024))
#define R_IOB_DIR                       (*((volatile INT32U *) 0xC0000028))
#define R_IOB_ATT                       (*((volatile INT32U *) 0xC000002C))
#define R_IOB_DRV                       (*((volatile INT32U *) 0xC0000030))
#define R_IOB_DRV1                      (*((volatile INT32U *) 0xC0000034))
#define R_IOC_I_DATA                    (*((volatile INT32U *) 0xC0000040))
#define R_IOC_O_DATA                    (*((volatile INT32U *) 0xC0000044))
#define R_IOC_DIR                       (*((volatile INT32U *) 0xC0000048))
#define R_IOC_ATT                       (*((volatile INT32U *) 0xC000004C))
#define R_IOC_DRV                       (*((volatile INT32U *) 0xC0000050))
#define R_IOC_DRV1                      (*((volatile INT32U *) 0xC0000054))
#define R_IOD_I_DATA                    (*((volatile INT32U *) 0xC0000060))
#define R_IOD_O_DATA                    (*((volatile INT32U *) 0xC0000064))
#define R_IOD_DIR                       (*((volatile INT32U *) 0xC0000068))
#define R_IOD_ATT                       (*((volatile INT32U *) 0xC000006C))
#define R_IOD_DRV                       (*((volatile INT32U *) 0xC0000070))
#define R_IOD_DRV1                      (*((volatile INT32U *) 0xC0000074))
#define R_IOE_I_DATA                    (*((volatile INT32U *) 0xC0000080))
#define R_IOE_O_DATA                    (*((volatile INT32U *) 0xC0000084))
#define R_IOE_DIR                       (*((volatile INT32U *) 0xC0000088))
#define R_IOE_ATT                       (*((volatile INT32U *) 0xC000008C))
#define R_IOE_DRV                       (*((volatile INT32U *) 0xC0000090))
#define R_IOE_DRV1                      (*((volatile INT32U *) 0xC0000094))
#define R_IOF_I_DATA                    (*((volatile INT32U *) 0xC00000A0))
#define R_IOF_O_DATA                    (*((volatile INT32U *) 0xC00000A4))
#define R_IOF_DIR                       (*((volatile INT32U *) 0xC00000A8))
#define R_IOF_ATT                       (*((volatile INT32U *) 0xC00000AC))
#define R_IOF_DRV                       (*((volatile INT32U *) 0xC00000B0))
#define R_IOF_DRV1                      (*((volatile INT32U *) 0xC00000B4))
#define R_IOG_I_DATA                    (*((volatile INT32U *) 0xC00000C0))
#define R_IOG_O_DATA                    (*((volatile INT32U *) 0xC00000C4))
#define R_IOG_DIR                       (*((volatile INT32U *) 0xC00000C8))
#define R_IOG_ATT                       (*((volatile INT32U *) 0xC00000CC))
#define R_IOG_DRV                       (*((volatile INT32U *) 0xC00000D0))
#define R_IOG_DRV1                      (*((volatile INT32U *) 0xC00000D4))
#define R_IOH_I_DATA                    (*((volatile INT32U *) 0xC00000E0))
#define R_IOH_O_DATA                    (*((volatile INT32U *) 0xC00000E4))
#define R_IOH_DIR                       (*((volatile INT32U *) 0xC00000E8))
#define R_IOH_ATT                       (*((volatile INT32U *) 0xC00000EC))
#define R_IOH_DRV                       (*((volatile INT32U *) 0xC00000F0))
#define R_IOH_DRV1                      (*((volatile INT32U *) 0xC00000F4))

#define R_IOSMTSEL                      (*((volatile INT32U *) 0xC0000100))
#define R_FUNPOS2                       (*((volatile INT32U *) 0xC0000104))
#define R_FUNPOS0                       (*((volatile INT32U *) 0xC0000108))
#define R_FUNPOS1                       (*((volatile INT32U *) 0xC000010C))

#define R_KEYCH7                        (*((volatile INT32U *) 0xC00000B8))
#define R_KEYCH                         (*((volatile INT32U *) 0xC0000110))
#define R_GPIO_CTRL                     (*((volatile INT32U *) 0xC0000114))
#define R_TIMER_POS                     (*((volatile INT32U *) 0xC0000118))
#define R_EXTIO_EN                      (*((volatile INT32U *) 0xC000011C))
#define R_MEMCTRL                       (*((volatile INT32U *) 0xC0000120))
#define R_MEM_DRV                       (*((volatile INT32U *) 0xC0000124))
#define R_TIMER_POS1                    (*((volatile INT32U *) 0xC0000128))
#define R_ANALOG_CTRL                   (*((volatile INT32U *) 0xC0000130))
#define R_LINEIN_CTRL                   (*((volatile INT32U *) 0xC0000134))
#define R_IOTRAP_STATUS                 (*((volatile INT32U *) 0xC0000140))
#define R_SMONI0                        (*((volatile INT32U *) 0xC0000150))
#define R_SMONI1                        (*((volatile INT32U *) 0xC0000154))
#define R_XD_DCTRL                      (*((volatile INT32U *) 0xC0000160))
#define R_XD_DCTRL1                     (*((volatile INT32U *) 0xC0000164))
#define R_XA_DCTRL                      (*((volatile INT32U *) 0xC0000168))
#define R_XA_DCTRL1                     (*((volatile INT32U *) 0xC000016C))
#define R_OTR_DCTRL                     (*((volatile INT32U *) 0xC0000170))
#define R_TFTIOCTRL                     (*((volatile INT32U *) 0xC0000174))
#define R_KEYC_POS_EN                   (*((volatile INT32U *) 0xC000017C))

#define R_IOA_TOP_PRIO                  (*((volatile INT32U *) 0xC0000180))
#define R_IOB_TOP_PRIO                  (*((volatile INT32U *) 0xC0000184))
#define R_IOC_TOP_PRIO                  (*((volatile INT32U *) 0xC0000188))
#define R_IOD_TOP_PRIO                  (*((volatile INT32U *) 0xC000018C))
#define R_IOE_TOP_PRIO                  (*((volatile INT32U *) 0xC0000190))
#define R_IOF_TOP_PRIO                  (*((volatile INT32U *) 0xC0000194))

#define R_IOA_DATA                      R_IOA_I_DATA
#define R_IOA_BUFFER                    R_IOA_O_DATA
#define R_IOA_ATTRIB                    R_IOA_ATT
#define R_IOB_DATA                      R_IOB_I_DATA
#define R_IOB_BUFFER                    R_IOB_O_DATA
#define R_IOB_ATTRIB                    R_IOB_ATT
#define R_IOC_DATA                      R_IOC_I_DATA
#define R_IOC_BUFFER                    R_IOC_O_DATA
#define R_IOC_ATTRIB                    R_IOC_ATT
#define R_IOD_DATA                      R_IOD_I_DATA
#define R_IOD_BUFFER                    R_IOD_O_DATA
#define R_IOD_ATTRIB                    R_IOD_ATT
#define R_IOE_DATA                      R_IOE_I_DATA
#define R_IOE_BUFFER                    R_IOE_O_DATA
#define R_IOE_ATTRIB                    R_IOE_ATT
#define R_IOF_DATA                      R_IOF_I_DATA
#define R_IOF_BUFFER                    R_IOF_O_DATA
#define R_IOF_ATTRIB                    R_IOF_ATT
#define R_IOG_DATA                      R_IOG_I_DATA
#define R_IOG_BUFFER                    R_IOG_O_DATA
#define R_IOG_ATTRIB                    R_IOG_ATT
#define R_IOH_DATA                      R_IOH_I_DATA
#define R_IOH_BUFFER                    R_IOH_O_DATA
#define R_IOH_ATTRIB                    R_IOH_ATT

/******************************************************************************
 * SPI Flash controller: 0xC0010000
 ******************************************************************************/
#define P_SPIFC_IO_MAP_ADDR             (0x30000000)
#define SPIFC_BASE                      0xC0010000
#define R_SPIFC_MIO_CTRL                (*((volatile INT32U *) (SPIFC_BASE + 0x00)))
#define R_SPIFC_CMD                     (*((volatile INT32U *) (SPIFC_BASE + 0x04)))
#define R_SPIFC_PARA                    (*((volatile INT32U *) (SPIFC_BASE + 0x08)))
#define R_SPIFC_ADDRL                   (*((volatile INT32U *) (SPIFC_BASE + 0x0C)))
#define R_SPIFC_ADDRH                   (*((volatile INT32U *) (SPIFC_BASE + 0x10)))
#define R_SPIFC_TX_WD                   (*((volatile INT32U *) (SPIFC_BASE + 0x14)))
#define R_SPIFC_RX_RD                   (*((volatile INT32U *) (SPIFC_BASE + 0x18)))
#define R_SPIFC_TX_BC                   (*((volatile INT32U *) (SPIFC_BASE + 0x1C)))
#define R_SPIFC_RX_BC                   (*((volatile INT32U *) (SPIFC_BASE + 0x20)))
#define R_SPIFC_TIMING                  (*((volatile INT32U *) (SPIFC_BASE + 0x24)))
#define R_SPIFC_OTHER                   (*((volatile INT32U *) (SPIFC_BASE + 0x28)))
#define R_SPIFC_CTRL                    (*((volatile INT32U *) (SPIFC_BASE + 0x2C)))


/******************************************************************************
 * Timer1: 0xC0020000
 ******************************************************************************/
#define P_TIMER_BASE_ADDR               0xC0020000
#define C_TIMER_CTRL                    0
#define C_TIMER_CCP_CTRL                1
#define C_TIMER_PRELOAD                 2
#define C_TIMER_CCP_REG                 3
#define C_TIMER_UPCOUNT                 4
#define C_TIMER_CMPLOAD                 5
#define C_TIMER_STATUS                  6

#define R_TIMERA_CTRL                   (*((volatile INT32U *) 0xC0020000))
#define R_TIMERA_CCP_CTRL               (*((volatile INT32U *) 0xC0020004))
#define R_TIMERA_PRELOAD                (*((volatile INT32U *) 0xC0020008))
#define R_TIMERA_CCP_REG                (*((volatile INT32U *) 0xC002000C))
#define R_TIMERA_UPCOUNT                (*((volatile INT32U *) 0xC0020010))
#define R_TIMERA_CMPLOAD                (*((volatile INT32U *) 0xC0020014))
#define R_TIMERA_STATUS                 (*((volatile INT32U *) 0xC002001C))


#define R_TIMERB_CTRL                   (*((volatile INT32U *) 0xC0020020))
#define R_TIMERB_CCP_CTRL               (*((volatile INT32U *) 0xC0020024))
#define R_TIMERB_PRELOAD                (*((volatile INT32U *) 0xC0020028))
#define R_TIMERB_CCP_REG                (*((volatile INT32U *) 0xC002002C))
#define R_TIMERB_UPCOUNT                (*((volatile INT32U *) 0xC0020030))
#define R_TIMERB_CMPLOAD                (*((volatile INT32U *) 0xC0020034))
#define R_TIMERB_STATUS                 (*((volatile INT32U *) 0xC002003C))

#define R_TIMERC_CTRL                   (*((volatile INT32U *) 0xC0020040))
#define R_TIMERC_CCP_CTRL               (*((volatile INT32U *) 0xC0020044))
#define R_TIMERC_PRELOAD                (*((volatile INT32U *) 0xC0020048))
#define R_TIMERC_CCP_REG                (*((volatile INT32U *) 0xC002004C))
#define R_TIMERC_UPCOUNT                (*((volatile INT32U *) 0xC0020050))
#define R_TIMERC_CMPLOAD                (*((volatile INT32U *) 0xC0020054))
#define R_TIMERC_STATUS                 (*((volatile INT32U *) 0xC002005C))

#define R_TIMERD_CTRL                   (*((volatile INT32U *) 0xC0020060))
#define R_TIMERD_CCP_CTRL               (*((volatile INT32U *) 0xC0020064))
#define R_TIMERD_PRELOAD                (*((volatile INT32U *) 0xC0020068))
#define R_TIMERD_CCP_REG                (*((volatile INT32U *) 0xC002006C))
#define R_TIMERD_UPCOUNT                (*((volatile INT32U *) 0xC0020070))
#define R_TIMERD_CMPLOAD                (*((volatile INT32U *) 0xC0020074))
#define R_TIMERD_STATUS                 (*((volatile INT32U *) 0xC002007C))

#define R_TIMERE_CTRL                   (*((volatile INT32U *) 0xC0020080))
#define R_TIMERE_CCP_CTRL               (*((volatile INT32U *) 0xC0020084))
#define R_TIMERE_PRELOAD                (*((volatile INT32U *) 0xC0020088))
#define R_TIMERE_UPCOUNT                (*((volatile INT32U *) 0xC0020090))
#define R_TIMERE_CMPLOAD                (*((volatile INT32U *) 0xC0020094))
#define R_TIMERE_STATUS                 (*((volatile INT32U *) 0xC002009C))

#define R_TIMERF_CTRL                   (*((volatile INT32U *) 0xC00200A0))
#define R_TIMERF_CCP_CTRL               (*((volatile INT32U *) 0xC00200A4))
#define R_TIMERF_PRELOAD                (*((volatile INT32U *) 0xC00200A8))
#define R_TIMERF_UPCOUNT                (*((volatile INT32U *) 0xC00200B0))
#define R_TIMERF_CMPLOAD                (*((volatile INT32U *) 0xC00200B4))
#define R_TIMERF_STATUS                 (*((volatile INT32U *) 0xC00200BC))

#define R_TIMERG_CTRL                   (*((volatile INT32U *) 0xC00200C0))
#define R_TIMERG_CCP_CTRL               (*((volatile INT32U *) 0xC00200C4))
#define R_TIMERG_PRELOAD                (*((volatile INT32U *) 0xC00200C8))
#define R_TIMERG_CCP_REG                (*((volatile INT32U *) 0xC00200CC))
#define R_TIMERG_UPCOUNT                (*((volatile INT32U *) 0xC00200D0))
#define R_TIMERG_CMPLOAD                (*((volatile INT32U *) 0xC00200D4))
#define R_TIMERG_STATUS                 (*((volatile INT32U *) 0xC00200DC))

#define R_TIMERH_CTRL                   (*((volatile INT32U *) 0xC00200E0))
#define R_TIMERH_CCP_CTRL               (*((volatile INT32U *) 0xC00200E4))
#define R_TIMERH_PRELOAD                (*((volatile INT32U *) 0xC00200E8))
#define R_TIMERH_CCP_REG                (*((volatile INT32U *) 0xC00200EC))
#define R_TIMERH_UPCOUNT                (*((volatile INT32U *) 0xC00200F0))
#define R_TIMERH_CMPLOAD                (*((volatile INT32U *) 0xC00200F4))
#define R_TIMERH_STATUS                 (*((volatile INT32U *) 0xC00200FC))

#define R_TIMERX_CTRL                   (*((volatile INT32U *) 0xC0020100))
#define R_TIMERX_PRELOAD                (*((volatile INT32U *) 0xC0020108))
#define R_TIMERX_CCP_REG                (*((volatile INT32U *) 0xC002010C))
#define R_TIMERX_UPCOUNT                (*((volatile INT32U *) 0xC0020110))
#define R_TIMERX_CMPLOAD                (*((volatile INT32U *) 0xC0020114))
#define R_TIMERX_STATUS                 (*((volatile INT32U *) 0xC002011C))


/******************************************************************************
 * Time Base: 0xC0030000
 ******************************************************************************/
#define R_TIMEBASEA_CTRL                (*((volatile INT32U *) 0xC0030000))
#define R_TIMEBASEB_CTRL                (*((volatile INT32U *) 0xC0030004))
#define R_TIMEBASEC_CTRL                (*((volatile INT32U *) 0xC0030008))
#define R_TIMEBASE_RESET                (*((volatile INT32U *) 0xC0030020))


/******************************************************************************
 * RTC: 0xC0040000
 ******************************************************************************/
#define R_RTC_SEC                       (*((volatile INT32U *) 0xC0040000))
#define R_RTC_MIN                       (*((volatile INT32U *) 0xC0040004))
#define R_RTC_HOUR                      (*((volatile INT32U *) 0xC0040008))
#define R_RTC_ALARM_SEC                 (*((volatile INT32U *) 0xC0040010))
#define R_RTC_ALARM_MIN                 (*((volatile INT32U *) 0xC0040014))
#define R_RTC_ALARM_HOUR                (*((volatile INT32U *) 0xC0040018))
#define R_RTC_CTRL                      (*((volatile INT32U *) 0xC0040050))
#define R_RTC_INT_STATUS                (*((volatile INT32U *) 0xC0040054))
#define R_RTC_INT_CTRL                  (*((volatile INT32U *) 0xC0040058))
#define R_RTC_BUSY                      (*((volatile INT32U *) 0xC004005C))


/******************************************************************************
 * UART0: 0xC0060000, UART1: 0xC0070000, UART2: 0xC0080000
 ******************************************************************************/
#define P_UART0_BASE                    ((volatile INT32U *) 0xC0060000)
#define R_UART0_DATA                    (*((volatile INT32U *) 0xC0060000))
#define R_UART0_RX_STATUS               (*((volatile INT32U *) 0xC0060004))
#define R_UART0_CTRL                    (*((volatile INT32U *) 0xC0060008))
#define R_UART0_BAUD_RATE               (*((volatile INT32U *) 0xC006000C))
#define R_UART0_STATUS                  (*((volatile INT32U *) 0xC0060010))
#define R_UART0_FIFO                    (*((volatile INT32U *) 0xC0060014))
#define R_UART0_TXDLY                   (*((volatile INT32U *) 0xC0060018))
#define R_IRDA0_BUAD_RATE               (*((volatile INT32U *) 0xC006001C))
#define R_IRDA0_CTRL                    (*((volatile INT32U *) 0xC0060020))
#define R_IRDA0_LOWPOWER                (*((volatile INT32U *) 0xC0060024))
#define R_UART0_EXT_BR                  (*((volatile INT32U *) 0xC0060038))
#define R_UART0_RS485_CR                (*((volatile INT32U *) 0xC0060040))
#define R_UART0_RS485_ADDR              (*((volatile INT32U *) 0xC0060044))
#define R_UART0_RS485_RTSCR             (*((volatile INT32U *) 0xC0060048))
#define R_UART0_RXTO_CR                 (*((volatile INT32U *) 0xC0060054))
#define R_UART0_RXTO_MSB                (*((volatile INT32U *) 0xC0060058))
#define R_UART0_RXTO_LSB                (*((volatile INT32U *) 0xC006005C))

#define P_UART1_BASE                    ((volatile INT32U *) 0xC0070000)
#define R_UART1_DATA                    (*((volatile INT32U *) 0xC0070000))
#define R_UART1_RX_STATUS               (*((volatile INT32U *) 0xC0070004))
#define R_UART1_CTRL                    (*((volatile INT32U *) 0xC0070008))
#define R_UART1_BAUD_RATE               (*((volatile INT32U *) 0xC007000C))
#define R_UART1_STATUS                  (*((volatile INT32U *) 0xC0070010))
#define R_UART1_FIFO                    (*((volatile INT32U *) 0xC0070014))
#define R_UART1_TXDLY                   (*((volatile INT32U *) 0xC0070018))
#define R_IRDA1_BUAD_RATE               (*((volatile INT32U *) 0xC007001C))
#define R_IRDA1_CTRL                    (*((volatile INT32U *) 0xC0070020))
#define R_IRDA1_LOWPOWER                (*((volatile INT32U *) 0xC0070024))
#define R_UART1_EXT_BR                  (*((volatile INT32U *) 0xC0070038))
#define R_UART1_RS485_CR                (*((volatile INT32U *) 0xC0070040))
#define R_UART1_RS485_ADDR              (*((volatile INT32U *) 0xC0070044))
#define R_UART1_RS485_RTSCR             (*((volatile INT32U *) 0xC0070048))
#define R_UART1_RXTO_CR                 (*((volatile INT32U *) 0xC0070054))
#define R_UART1_RXTO_MSB                (*((volatile INT32U *) 0xC0070058))
#define R_UART1_RXTO_LSB                (*((volatile INT32U *) 0xC007005C))

#define P_UART2_BASE                    ((volatile INT32U *) 0xC0080000)
#define R_UART2_DATA                    (*((volatile INT32U *) 0xC0080000))
#define R_UART2_RX_STATUS               (*((volatile INT32U *) 0xC0080004))
#define R_UART2_CTRL                    (*((volatile INT32U *) 0xC0080008))
#define R_UART2_BAUD_RATE               (*((volatile INT32U *) 0xC008000C))
#define R_UART2_STATUS                  (*((volatile INT32U *) 0xC0080010))
#define R_UART2_FIFO                    (*((volatile INT32U *) 0xC0080014))
#define R_UART2_TXDLY                   (*((volatile INT32U *) 0xC0080018))
#define R_IRDA2_BUAD_RATE               (*((volatile INT32U *) 0xC008001C))
#define R_IRDA2_CTRL                    (*((volatile INT32U *) 0xC0080020))
#define R_IRDA2_LOWPOWER                (*((volatile INT32U *) 0xC0080024))
#define R_UART2_EXT_BR                  (*((volatile INT32U *) 0xC0080038))
#define R_UART2_RS485_CR                (*((volatile INT32U *) 0xC0080040))
#define R_UART2_RS485_ADDR              (*((volatile INT32U *) 0xC0080044))
#define R_UART2_RS485_RTSCR             (*((volatile INT32U *) 0xC0080048))
#define R_UART2_RXTO_CR                 (*((volatile INT32U *) 0xC0080054))
#define R_UART2_RXTO_MSB                (*((volatile INT32U *) 0xC0080058))
#define R_UART2_RXTO_LSB                (*((volatile INT32U *) 0xC008005C))
/******************************************************************************
 * FIR/MIR: 0xC01F0000
 ******************************************************************************/
#define P_FIR_TXFIFO_BASE           ((volatile INT32U *) 0xC01F0018)
#define P_FIR_RXFIFO_BASE           ((volatile INT32U *) 0xC01F001C)
#define R_IRDA_TRCTL				(*((volatile INT32U *) 0xC01F0000))		/* Transceiver control register */
#define R_IRDA_HADD					(*((volatile INT32U *) 0xC01F0004))		/* Hardware address register */
#define R_IRDA_IE					(*((volatile INT32U *) 0xC01F0008))		/* Interrupt enable register */
#define R_IRDA_TCOUNT				(*((volatile INT32U *) 0xC01F000C))		/* Transmit count register */
#define R_IRDA_STA					(*((volatile INT32U *) 0xC01F0010))		/* Status register */
#define R_IRDA_IF					(*((volatile INT32U *) 0xC01F0014))		/* Interrupt flag register */
#define R_IRDA_TXFIFO				(*((volatile INT32U *) 0xC01F0018))		/* TX FIFO */
#define R_IRDA_RXFIFO				(*((volatile INT32U *) 0xC01F001C))		/* RX FIFO */
#define R_IRDA_CTL					(*((volatile INT32U *) 0xC01F0020))		/* IrDA Control register */
#define R_IRDA_BAUD					(*((volatile INT32U *) 0xC01F0024))		/* Baud rate control register */
#define R_IRDA_SRST					(*((volatile INT32U *) 0xC01F0028))		/* Software reset register */
#define R_IRDA_SIP					(*((volatile INT32U *) 0xC01F002C))		/* SIP control register */
#define R_IRDA_RXPL					(*((volatile INT32U *) 0xC01F0030))		/* RX packet length register */
#define R_IRDA_RXPCNT				(*((volatile INT32U *) 0xC01F0034))		/* RX packet counter */
#define R_IRDA_TXSETCRC				(*((volatile INT32U *) 0xC01F00F8))		/* Generate TX CRC register, only for zero length */
#define R_IRDA_TXCRC				(*((volatile INT32U *) 0xC01F00FC))		/* TX CRC register */

/******************************************************************************
 * USB: 0xD1100000
 ******************************************************************************/
#define	VBUS_SAMPLE_PERIOD_MASK         0xFFFF

/* USB device register definitions */
//#define  UDC_BASE                       (0xD1100000)

#define  UDC_BASE_A                       (0xD1100000)
#define  UDC_BASE_B                       (0xD2800000)
#define  USB20D_BASE_SEL(x)      INT32U UDC_BASE = x?UDC_BASE_B:UDC_BASE_A

#define  rUDC_EP12DMA                   (*(volatile unsigned *)(UDC_BASE+0x000))
#define  rUDC_EP12DA                    (*(volatile unsigned *)(UDC_BASE+0x004))
#define  rUDC_EP7DMA                    (*(volatile unsigned *)(UDC_BASE+0x008))
#define  rUDC_EP7DA                     (*(volatile unsigned *)(UDC_BASE+0x00C))
#define  rUDC_EP89DMA                   (*(volatile unsigned *)(UDC_BASE+0x010))
#define  rUDC_EP89DA                    (*(volatile unsigned *)(UDC_BASE+0x014))
#define  rUDC_EPABDMA                   (*(volatile unsigned *)(UDC_BASE+0x018))
#define  rUDC_EPABDA                    (*(volatile unsigned *)(UDC_BASE+0x01C))

#define  rUDPHYI2CCR                    (*(volatile unsigned *)(UDC_BASE+0x020))
#define  rUDPHYI2CDR                    (*(volatile unsigned *)(UDC_BASE+0x024))

#define  rUDC_IFMANU                    (*(volatile unsigned *)(UDC_BASE+0x028))
#define  rUDCCS_UDC                     (*(volatile unsigned *)(UDC_BASE+0x080))
#define  rUDC_IRQ_ENABLE                (*(volatile unsigned *)(UDC_BASE+0x084))
#define  rUDC_IRQ_FLAG                  (*(volatile unsigned *)(UDC_BASE+0x088))
#define  rUDC_IRQ_SOURCE                (*(volatile unsigned *)(UDC_BASE+0x08C))
#define  rEP4_CTRL                      (*(volatile unsigned *)(UDC_BASE+0x100))
#define  rEP4FIFOPTR                    (*(volatile unsigned *)(UDC_BASE+0x104))
#define  rEP4DP                         (*(volatile unsigned *)(UDC_BASE+0x108))
#define  rEP4VB                         (*(volatile unsigned *)(UDC_BASE+0x10C))
#define  rEP5_CTRL                      (*(volatile unsigned *)(UDC_BASE+0x140))
#define  rEP5HDLEN                      (*(volatile unsigned *)(UDC_BASE+0x144))
#define  rEP5FRAMCTL                    (*(volatile unsigned *)(UDC_BASE+0x148))
#define  rEP5HDCTRL                     (*(volatile unsigned *)(UDC_BASE+0x14C))
#define  rEP5EN                         (*(volatile unsigned *)(UDC_BASE+0x150))
#define  rEP5RPTR                       (*(volatile unsigned *)(UDC_BASE+0x154))
#define  rEP5WPTR                       (*(volatile unsigned *)(UDC_BASE+0x158))
#define  rEP5FIFO                       (*(volatile unsigned *)(UDC_BASE+0x15C))
#define  rEP5DMAEN                      (*(volatile unsigned *)(UDC_BASE+0x160))
#define  rEP5SOF_RCVY                   (*(volatile unsigned *)(UDC_BASE+0x164))
#define  rEP5FTLB                       (*(volatile unsigned *)(UDC_BASE+0x168))
#define  rEP5FTHB                       (*(volatile unsigned *)(UDC_BASE+0x16C))
#define  rEP5VB                         (*(volatile unsigned *)(UDC_BASE+0x170))
#define  rEPIFALTIF                     (*(volatile unsigned *)(UDC_BASE+0x174))
#define  rEP6_CTRL                      (*(volatile unsigned *)(UDC_BASE+0x180))
#define  rEP6FIFOPTR                    (*(volatile unsigned *)(UDC_BASE+0x184))
#define  rEP6DP                         (*(volatile unsigned *)(UDC_BASE+0x188))
#define  rEP6VB                         (*(volatile unsigned *)(UDC_BASE+0x18C))
#define  rEP7_CTRL                      (*(volatile unsigned *)(UDC_BASE+0x1C0))
#define  rEP7RPTR                       (*(volatile unsigned *)(UDC_BASE+0x1C4))
#define  rEP7WPTR                       (*(volatile unsigned *)(UDC_BASE+0x1C8))
#define  rEP7FIFO                       (*(volatile unsigned *)(UDC_BASE+0x1CC))
#define  rEP7VB                         (*(volatile unsigned *)(UDC_BASE+0x1D0))
#define  rDVIDEO_REFCLKL                (*(volatile unsigned *)(UDC_BASE+0x200))
#define  rDVIDEO_REFCLKH                (*(volatile unsigned *)(UDC_BASE+0x204))
#define  rUDC_LLCFS1                    (*(volatile unsigned *)(UDC_BASE+0x320))
#define  rUDC_CP_CBW_TAG                (*(volatile unsigned *)(UDC_BASE+0x328))
#define  rEP12_CTRL                     (*(volatile unsigned *)(UDC_BASE+0x330))
#define  rEP12_PPF_CTRL                 (*(volatile unsigned *)(UDC_BASE+0x334))
#define  rEP0_VB                        (*(volatile unsigned *)(UDC_BASE+0x340))
#define  rEP0_SETUP_CTRL                (*(volatile unsigned *)(UDC_BASE+0x344))
#define  rEP0_SETUP_FIFO                (*(volatile unsigned *)(UDC_BASE+0x348))
#define  rEP0_CTRL                      (*(volatile unsigned *)(UDC_BASE+0x34C))
#define  rEP0_CNTR                      (*(volatile unsigned *)(UDC_BASE+0x350))
#define  rEP0_FIFO                      (*(volatile unsigned *)(UDC_BASE+0x354))
#define  rEP1S_CTRL                     (*(volatile unsigned *)(UDC_BASE+0x358))
#define  rEP1S_FIFO                     (*(volatile unsigned *)(UDC_BASE+0x35C))
#define  rUDC_LLCFS2                    (*(volatile unsigned *)(UDC_BASE+0x360))
#define  rEP12_FS                       (*(volatile unsigned *)(UDC_BASE+0x364))
#define  rEP12_CNTRL                    (*(volatile unsigned *)(UDC_BASE+0x368))
#define  rEP12_CNTRH                    (*(volatile unsigned *)(UDC_BASE+0x36C))
#define  rEP12_PIPO                     (*(volatile unsigned *)(UDC_BASE+0x370))
#define  rEP3_CTRL                      (*(volatile unsigned *)(UDC_BASE+0x374))
#define  rEP3FIFOPTR                    (*(volatile unsigned *)(UDC_BASE+0x378))
#define  rEP3DP                         (*(volatile unsigned *)(UDC_BASE+0x37C))
#define  rEP3VB                         (*(volatile unsigned *)(UDC_BASE+0x380))
#define  rEP0_OUT_NAK                   (*(volatile unsigned *)(UDC_BASE+0x384))
#define  rEP0_IN_NAK                    (*(volatile unsigned *)(UDC_BASE+0x388))
#define  rEP1_NAK                       (*(volatile unsigned *)(UDC_BASE+0x38C))
#define  rEP2_NAK                       (*(volatile unsigned *)(UDC_BASE+0x390))
#define  rEP12_VB                       (*(volatile unsigned *)(UDC_BASE+0x398))
#define  rEP12_POCNTL                   (*(volatile unsigned *)(UDC_BASE+0x39C))
#define  rEP12_POCNTH                   (*(volatile unsigned *)(UDC_BASE+0x3A0))
#define  rUDLC_SET0                     (*(volatile unsigned *)(UDC_BASE+0x3B0))
#define  rUDLC_SET1                     (*(volatile unsigned *)(UDC_BASE+0x3B4))
#define  rUDC_STATUS                    (*(volatile unsigned *)(UDC_BASE+0x3B8))
#define  rUDC_STALL_CTRL                (*(volatile unsigned *)(UDC_BASE+0x3BC))
#define  rUDLC_SET2                     (*(volatile unsigned *)(UDC_BASE+0x3C0))
#define  rUDC_LCS2                      (*(volatile unsigned *)(UDC_BASE+0x3C4))
#define  rUDC_LCS3                      (*(volatile unsigned *)(UDC_BASE+0x3C8))
#define  rUDLC_PGCTRL                   (*(volatile unsigned *)(UDC_BASE+0x3F0))
#define  rUDC_ADDR                      (*(volatile unsigned *)(UDC_BASE+0x3F4))
#define  rDLCIF_UDLC                    (*(volatile unsigned *)(UDC_BASE+0x400))
#define  rDLCIE_UDLC                    (*(volatile unsigned *)(UDC_BASE+0x404))
#define  rDLCIS_UDC                     (*(volatile unsigned *)(UDC_BASE+0x408))
#define  rSTANDARD_REQ_IF               (*(volatile unsigned *)(UDC_BASE+0x410))
#define  rSTANDARD_REQ_IE               (*(volatile unsigned *)(UDC_BASE+0x414))
#define  rSTANDARD_REQ_IS               (*(volatile unsigned *)(UDC_BASE+0x418))
#define  rUDC_EPNEW_IF                  (*(volatile unsigned *)(UDC_BASE+0x420))
#define  rUDC_EPNEW_IE                  (*(volatile unsigned *)(UDC_BASE+0x424))
#define  rUDC_EPNEW_IS                  (*(volatile unsigned *)(UDC_BASE+0x428))
#define  rEP89_CTRL                     (*(volatile unsigned *)(UDC_BASE+0x500))
#define  rEP89_PPF_CTRL                 (*(volatile unsigned *)(UDC_BASE+0x504))
#define  rEP89_FS                       (*(volatile unsigned *)(UDC_BASE+0x508))
#define  rEP89_PICL                     (*(volatile unsigned *)(UDC_BASE+0x50C))
#define  rEP89_PICH                     (*(volatile unsigned *)(UDC_BASE+0x510))
#define  rEP89_POCL                     (*(volatile unsigned *)(UDC_BASE+0x514))
#define  rEP89_POCH                     (*(volatile unsigned *)(UDC_BASE+0x518))
#define  rEP89_FIFO                     (*(volatile unsigned *)(UDC_BASE+0x51C))
#define  rEP89_VB                       (*(volatile unsigned *)(UDC_BASE+0x520))
#define  rEP8_INNAK_CNT                 (*(volatile unsigned *)(UDC_BASE+0x524))
#define  rEP9_OUTNAK_CNT                (*(volatile unsigned *)(UDC_BASE+0x528))
#define  rEP89_SETTING                  (*(volatile unsigned *)(UDC_BASE+0x52C))
#define  rEPAB_CTRL                     (*(volatile unsigned *)(UDC_BASE+0x550))
#define  rEPAB_PPC                      (*(volatile unsigned *)(UDC_BASE+0x554))
#define  rEPAB_FS                       (*(volatile unsigned *)(UDC_BASE+0x558))
#define  rEPAB_PICL                     (*(volatile unsigned *)(UDC_BASE+0x55C))
#define  rEPAB_PICH                     (*(volatile unsigned *)(UDC_BASE+0x560))
#define  rEPAB_POCL                     (*(volatile unsigned *)(UDC_BASE+0x564))
#define  rEPAB_POCH                     (*(volatile unsigned *)(UDC_BASE+0x568))
#define  rEPAB_FIFO                     (*(volatile unsigned *)(UDC_BASE+0x56C))
#define  rEPAB_VB                       (*(volatile unsigned *)(UDC_BASE+0x570))
#define  rEPA_INNAK_CNT                 (*(volatile unsigned *)(UDC_BASE+0x574))
#define  rEPB_OUTNAK_CNT                (*(volatile unsigned *)(UDC_BASE+0x578))
#define  rEPAB_SETTING                  (*(volatile unsigned *)(UDC_BASE+0x57C))

#define rEPC_EVENT					    (*(volatile unsigned *)(UDC_BASE+0x580))
#define rEPC_CTRL					    (*(volatile unsigned *)(UDC_BASE+0x584))
#define rEPC_WRC					    (*(volatile unsigned *)(UDC_BASE+0x588))
#define rEPC_FUN					    (*(volatile unsigned *)(UDC_BASE+0x58C))
#define rEPC_DATA					    (*(volatile unsigned *)(UDC_BASE+0x590))
#define rIP_NAME                        (*(volatile unsigned *)(UDC_BASE+0xFF8))
#define rVERSION                        (*(volatile unsigned *)(UDC_BASE+0xFFC))
//==========================================================
#define	rSYS_CTRL_NEW                   (*(volatile unsigned *)(0xD0000000+0x08))


/******************************************************************************
 * SPI: 0xC0080000
 ******************************************************************************/
#define SPI0_BASE					0xC0090000
#define P_SPI0_CTRL       		    ((volatile INT32U *) (SPI0_BASE + 0x00))
#define P_SPI0_TX_DATA       		((volatile INT32U *) (SPI0_BASE + 0x08))
#define P_SPI0_RX_DATA       		((volatile INT32U *) (SPI0_BASE + 0x10))

#define R_SPI0_CTRL                      (*((volatile INT32U *) (SPI0_BASE + 0x00)))
#define R_SPI0_TX_STATUS                 (*((volatile INT32U *) (SPI0_BASE + 0x04)))
#define R_SPI0_TX_DATA                   (*((volatile INT32U *) (SPI0_BASE + 0x08)))
#define R_SPI0_RX_STATUS                 (*((volatile INT32U *) (SPI0_BASE + 0x0C)))
#define R_SPI0_RX_DATA                   (*((volatile INT32U *) (SPI0_BASE + 0x10)))
#define R_SPI0_MISC                      (*((volatile INT32U *) (SPI0_BASE + 0x14)))
#define R_SPI0_DMA_RX_CYC                (*((volatile INT32U *) (SPI0_BASE + 0x1C)))

#define SPI1_BASE					0xC00A0000
#define P_SPI1_CTRL       		    ((volatile INT32U *) (SPI1_BASE + 0x00))
#define P_SPI1_TX_DATA       		((volatile INT32U *) (SPI1_BASE + 0x08))
#define P_SPI1_RX_DATA       		((volatile INT32U *) (SPI1_BASE + 0x10))

#define R_SPI1_CTRL                      (*((volatile INT32U *) (SPI1_BASE + 0x00)))
#define R_SPI1_TX_STATUS                 (*((volatile INT32U *) (SPI1_BASE + 0x04)))
#define R_SPI1_TX_DATA                   (*((volatile INT32U *) (SPI1_BASE + 0x08)))
#define R_SPI1_RX_STATUS                 (*((volatile INT32U *) (SPI1_BASE + 0x0C)))
#define R_SPI1_RX_DATA                   (*((volatile INT32U *) (SPI1_BASE + 0x10)))
#define R_SPI1_MISC                      (*((volatile INT32U *) (SPI1_BASE + 0x14)))
#define R_SPI1_DMA_RX_CYC                (*((volatile INT32U *) (SPI1_BASE + 0x1C)))

/******************************************************************************
 * IRTC: 0xC0050000
 ******************************************************************************/
#define EXT_RTC_BASE                    0xC0050000
#define R_EXT_RTC_CTRL                  (*((volatile INT32U *) (EXT_RTC_BASE+0x00)))
#define R_EXT_RTC_ADDR                  (*((volatile INT32U *) (EXT_RTC_BASE+0x04)))
#define R_EXT_RTC_WR_DATA               (*((volatile INT32U *) (EXT_RTC_BASE+0x08)))
#define R_EXT_RTC_REQ                   (*((volatile INT32U *) (EXT_RTC_BASE+0x0C)))
#define R_EXT_RTC_READY                 (*((volatile INT32U *) (EXT_RTC_BASE+0x10)))
#define R_EXT_RTC_RD_DATA               (*((volatile INT32U *) (EXT_RTC_BASE+0x14)))
#define R_EXT_RTC_DIV                   (*((volatile INT32U *) (EXT_RTC_BASE+0x2C)))


/******************************************************************************
 * SDC: 0xC0250000
 ******************************************************************************/
#define	P_SDC0_BASE                     0xC0250000
#define	P_SDC0_DATA_TX                  ((volatile INT32U *) (P_SDC0_BASE + 0x0000))
#define	P_SDC0_DATA_RX                  ((volatile INT32U *) (P_SDC0_BASE + 0x0004))

#define	R_SDC0_DATA_TX                  (*((volatile INT32U *) (P_SDC0_BASE + 0x0000)))
#define	R_SDC0_DATA_RX                  (*((volatile INT32U *) (P_SDC0_BASE + 0x0004)))
#define	R_SDC0_CMMAND                   (*((volatile INT32U *) (P_SDC0_BASE + 0x0008)))
#define	R_SDC0_ARGUMENT                 (*((volatile INT32U *) (P_SDC0_BASE + 0x000C)))
#define	R_SDC0_RESPONSE                 (*((volatile INT32U *) (P_SDC0_BASE + 0x0010)))
#define	R_SDC0_STATUS                   (*((volatile INT32U *) (P_SDC0_BASE + 0x0014)))
#define	R_SDC0_CTRL                     (*((volatile INT32U *) (P_SDC0_BASE + 0x0018)))
#define	R_SDC0_INTEN                    (*((volatile INT32U *) (P_SDC0_BASE + 0x001C)))
#define	R_SDC0_CLKGATED                 (*((volatile INT32U *) (P_SDC0_BASE + 0x0020)))
#define	R_SDC0_TIMEOUT_EN               (*((volatile INT32U *) (P_SDC0_BASE + 0x0024)))
#define	R_SDC0_TIMEOUT_THR              (*((volatile INT32U *) (P_SDC0_BASE + 0x0028)))

/******************************************************************************
 * SDC1: 0xD1200000
 ******************************************************************************/
#define	P_SDC1_BASE                     0xD1200000
#define	P_SDC1_DATA_TX                  ((volatile INT32U *) (P_SDC1_BASE + 0x0000))
#define	P_SDC1_DATA_RX                  ((volatile INT32U *) (P_SDC1_BASE + 0x0004))

#define	R_SDC1_DATA_TX                  (*((volatile INT32U *) (P_SDC1_BASE + 0x0000)))
#define	R_SDC1_DATA_RX                  (*((volatile INT32U *) (P_SDC1_BASE + 0x0004)))
#define	R_SDC1_CMMAND                   (*((volatile INT32U *) (P_SDC1_BASE + 0x0008)))
#define	R_SDC1_ARGUMENT                 (*((volatile INT32U *) (P_SDC1_BASE + 0x000C)))
#define	R_SDC1_RESPONSE                 (*((volatile INT32U *) (P_SDC1_BASE + 0x0010)))
#define	R_SDC1_STATUS                   (*((volatile INT32U *) (P_SDC1_BASE + 0x0014)))
#define	R_SDC1_CTRL                     (*((volatile INT32U *) (P_SDC1_BASE + 0x0018)))
#define	R_SDC1_INTEN                    (*((volatile INT32U *) (P_SDC1_BASE + 0x001C)))
#define	R_SDC1_CLKGATED                 (*((volatile INT32U *) (P_SDC1_BASE + 0x0020)))
#define	R_SDC1_TIMEOUT_EN               (*((volatile INT32U *) (P_SDC1_BASE + 0x0024)))
#define	R_SDC1_TIMEOUT_THR              (*((volatile INT32U *) (P_SDC1_BASE + 0x0028)))


/******************************************************************************
 * ADC: 0xC00D0000
 ******************************************************************************/
#define P_ADC_ASADC_DATA                ((volatile INT32U *) 0xC00D0010)

#define R_ADC_SETUP                     (*((volatile INT32U *) 0xC00D0000))
#define R_ADC_MADC_CTRL                 (*((volatile INT32U *) 0xC00D0004))
#define R_ADC_MADC_DATA                 (*((volatile INT32U *) 0xC00D0008))
#define R_ADC_ASADC_CTRL                (*((volatile INT32U *) 0xC00D000C))
#define R_ADC_ASADC_DATA                (*((volatile INT32U *) 0xC00D0010))
#define R_ADC_SH_WAIT                   (*((volatile INT32U *) 0xC00D001C))

#define R_ADC_TEMP_CTR                  (*((volatile INT32U *) 0xC00D0020))
#define R_ADC_TEMP_L_CNT                (*((volatile INT32U *) 0xC00D0024))
#define R_ADC_TEMP_H_CNT                (*((volatile INT32U *) 0xC00D0028))
#define R_ADC_UPPER_THRES               (*((volatile INT32U *) 0xC00D002C))
#define R_ADC_LOWER_THRES               (*((volatile INT32U *) 0xC00D0030))
#define R_ADC_BOND_CNT                  (*((volatile INT32U *) 0xC00D0034))
#define R_ADC_TEMP_FLAG                 (*((volatile INT32U *) 0xC00D0038))
#define R_ADC_BAND_GRD_CTR              (*((volatile INT32U *) 0xC00D003C))
#define R_ADC_BAND_GRD_CH               (*((volatile INT32U *) 0xC00D0040))
#define R_ADC_WAIT_L_CNT                (*((volatile INT32U *) 0xC00D0044))
#define R_ADC_WAIT_H_CNT                (*((volatile INT32U *) 0xC00D0048))
#define R_ADC_OFFSET                    (*((volatile INT32U *) 0xC00D004C))
#define R_ADC_TEMP_AVG                  (*((volatile INT32U *) 0xC00D0050))
#define R_ADC_GROUND_AVG                (*((volatile INT32U *) 0xC00D0054))
#define R_ADC_BANDGAP_AVG               (*((volatile INT32U *) 0xC00D0058))
#define R_ADC_FINISH_FLAG               (*((volatile INT32U *) 0xC00D005C))


/******************************************************************************
 * DAC: 0xC00E0000
 ******************************************************************************/
#define P_DAC_CHA_DATA       			((volatile INT32U *) 0xC00E0004)
#define P_DAC_CHB_DATA       			((volatile INT32U *) 0xC00E0024)

#define R_DAC_CHA_CTRL                  (*((volatile INT32U *) 0xC00E0000))
#define R_DAC_CHA_DATA                  (*((volatile INT32U *) 0xC00E0004))
#define R_DAC_CHA_FIFO                  (*((volatile INT32U *) 0xC00E0008))
#define R_DAC_CHB_CTRL                  (*((volatile INT32U *) 0xC00E0020))
#define R_DAC_CHB_DATA                  (*((volatile INT32U *) 0xC00E0024))
#define R_DAC_CHB_FIFO                  (*((volatile INT32U *) 0xC00E0028))
#define R_DAC_PGA                       (*((volatile INT32U *) 0xC00E002C))
#define R_DAC_DIFF                      (*((volatile INT32U *) 0xC00E0038))

/******************************************************************************
 * NAND: 0xD0900000
 ******************************************************************************/
#define R_NF_DMA_CTRL                   (*((volatile INT32U *) 0xD0900000))
#define R_NF_DMA_ADDR                   (*((volatile INT32U *) 0xD0900004))
#define R_NF_DMA_LEN                    (*((volatile INT32U *) 0xD0900008))
#define R_NF_CTRL                       (*((volatile INT32U *) 0xD0900140))
#define R_NF_CMD                        (*((volatile INT32U *) 0xD0900144))
#define R_NF_ADDRL                      (*((volatile INT32U *) 0xD0900148))
#define R_NF_ADDRH                      (*((volatile INT32U *) 0xD090014C))
#define R_NF_DATA                       (*((volatile INT32U *) 0xD0900150))
#define R_NF_ADDR_CTRL                  (*((volatile INT32U *) 0xD0900154))
#define R_NF_RESERVE_REG                (*((volatile INT32U *) 0xD09001DC))

#define R_NF_ECC_CTRL                   (*((volatile INT32U *) 0xD090015C))
#define R_NF_ECC_LPRL_LB                (*((volatile INT32U *) 0xD0900160))
#define R_NF_ECC_LPRH_LB                (*((volatile INT32U *) 0xD0900164))
#define R_NF_ECC_CPR_LB                 (*((volatile INT32U *) 0xD0900168))
#define R_NF_ECC_LPR_CKL_LB             (*((volatile INT32U *) 0xD090016C))
#define R_NF_ECC_LPR_CKH_LB             (*((volatile INT32U *) 0xD0900170))
#define R_NF_ECC_CPCKR_LB               (*((volatile INT32U *) 0xD0900174))
#define R_NF_ECC_ERR0_LB                (*((volatile INT32U *) 0xD0900178))
#define R_NF_ECC_ERR1_LB                (*((volatile INT32U *) 0xD090017C))
#define R_NF_ECC_LPRL_HB                (*((volatile INT32U *) 0xD0900120))
#define R_NF_ECC_LPRH_HB                (*((volatile INT32U *) 0xD0900124))
#define R_NF_ECC_CPR_HB                 (*((volatile INT32U *) 0xD0900128))
#define R_NF_ECC_LPR_CKL_HB             (*((volatile INT32U *) 0xD090012C))
#define R_NF_ECC_LPR_CKH_HB             (*((volatile INT32U *) 0xD0900130))
#define R_NF_ECC_CPCKR_HB               (*((volatile INT32U *) 0xD0900134))
#define R_NF_ECC_ERR0_HB                (*((volatile INT32U *) 0xD0900138))
#define R_NF_ECC_ERR1_HB                (*((volatile INT32U *) 0xD090013C))
#define R_NF_CHECKSUM0_LB               (*((volatile INT32U *) 0xD09000C0))
#define R_NF_CHECKSUM1_LB               (*((volatile INT32U *) 0xD09000C4))
#define R_NF_CHECKSUM0_HB               (*((volatile INT32U *) 0xD09000C8))
#define R_NF_CHECKSUM1_HB               (*((volatile INT32U *) 0xD09000CC))

#define R_NF_BCH_CTRL                   (*((volatile INT32U *) 0xD0900158))
#define R_NF_BCH_ERROR                  (*((volatile INT32U *) 0xD0900160))
#define R_NF_BCH_PARITY0                (*((volatile INT32U *) 0xD0900164))
#define R_NF_BCH_PARITY1                (*((volatile INT32U *) 0xD0900168))
#define R_NF_BCH_PARITY2                (*((volatile INT32U *) 0xD090016C))
#define R_NF_BCH_PARITY3                (*((volatile INT32U *) 0xD0900170))
#define R_NF_BCH_PARITY4                (*((volatile INT32U *) 0xD0900174))
#define R_NF_BCH_PARITY5                (*((volatile INT32U *) 0xD0900178))
#define R_NF_BCH_PARITY6                (*((volatile INT32U *) 0xD090017C))
#define R_NF_BCH_PARITY7                (*((volatile INT32U *) 0xD0900180))
#define R_NF_BCH_PARITY8                (*((volatile INT32U *) 0xD0900184))
#define R_NF_BCH_PARITY9                (*((volatile INT32U *) 0xD0900188))

#define R_NF_SHARE_BYTES                (*((volatile INT32U *) 0xD0900018))

#define R_BCH_FIND_ERR_BASE             (*((volatile INT32U *) 0xC0100000))
#define R_BCH_FIND_ERR_CTRL             (*((volatile INT32U *) 0xC0100000))
#define R_BCH_STATUS_REG1               (*((volatile INT32U *) 0xC0100004))
#define P_BCH_LOCATION_VAL0             ((volatile INT32U *) 0xC0100020)
#define R_BCH_LOCATION_VAL0             (*((volatile INT32U *) 0xC0100020))
#define R_BCH_LOCATION_VAL1             (*((volatile INT32U *) 0xC0100024))
#define R_BCH_LOCATION_VAL2             (*((volatile INT32U *) 0xC0100028))
#define R_BCH_LOCATION_VAL3             (*((volatile INT32U *) 0xC010002C))
#define R_BCH_LOCATION_VAL4             (*((volatile INT32U *) 0xC0100030))
#define R_BCH_LOCATION_VAL5             (*((volatile INT32U *) 0xC0100034))
#define R_BCH_LOCATION_VAL6             (*((volatile INT32U *) 0xC0100038))
#define R_BCH_LOCATION_VAL7             (*((volatile INT32U *) 0xC010003C))
#define R_BCH_LOCATION_VAL8             (*((volatile INT32U *) 0xC0100040))
#define R_BCH_LOCATION_VAL9             (*((volatile INT32U *) 0xC0100044))
#define R_BCH_LOCATION_VAL10            (*((volatile INT32U *) 0xC0100048))
#define R_BCH_LOCATION_VAL11            (*((volatile INT32U *) 0xC010004C))
#define R_NF_BCH_FIND_ERR_PARITY0       (*((volatile INT32U *) 0xC0100050))
#define R_NF_BCH_FIND_ERR_PARITY1       (*((volatile INT32U *) 0xC0100054))
#define R_NF_BCH_FIND_ERR_PARITY2       (*((volatile INT32U *) 0xC0100058))
#define R_NF_BCH_FIND_ERR_PARITY3       (*((volatile INT32U *) 0xC010005c))
#define R_NF_BCH_FIND_ERR_PARITY4       (*((volatile INT32U *) 0xC0100060))
#define R_NF_BCH_FIND_ERR_PARITY5       (*((volatile INT32U *) 0xC0100064))
#define R_NF_BCH_FIND_ERR_PARITY6       (*((volatile INT32U *) 0xC0100068))
#define R_NF_BCH_FIND_ERR_PARITY7       (*((volatile INT32U *) 0xC010006c))
#define R_NF_BCH_FIND_ERR_PARITY8       (*((volatile INT32U *) 0xC0100070))
#define R_NF_BCH_FIND_ERR_PARITY9       (*((volatile INT32U *) 0xC0100074))


/******************************************************************************
 * NAND: BCH-60bit
 ******************************************************************************/
//#define R_NF_DMA_CTRL                 (*((volatile INT32U *) 0xD0900000))
//#define R_NF_DMA_ADDR                 (*((volatile INT32U *) 0xD0900004))
//#define R_NF_DMA_LEN                  (*((volatile INT32U *) 0xD0900008))
//#define R_NF_SHARE_BYTES              (*((volatile INT32U *) 0xD0900018))
//#define R_NF_CTRL                     (*((volatile INT32U *) 0xD0900140))
//#define R_NF_CMD                      (*((volatile INT32U *) 0xD0900144))
//#define R_NF_ADDRL                    (*((volatile INT32U *) 0xD0900148))
//#define R_NF_ADDRH                    (*((volatile INT32U *) 0xD090014C))
//#define R_NF_DATA                     (*((volatile INT32U *) 0xD0900150))
//#define R_NF_ADDR_CTRL                (*((volatile INT32U *) 0xD0900154))
//#define R_NF_BCH_CTRL                 (*((volatile INT32U *) 0xD0900158))

// RANDOMIZE
#define	R_NF_RANDOM_CTRL                (*((volatile INT32U *) 0xD09001CC))
#define	R_NF_LFSR_COE                   (*((volatile INT32U *) 0xD09001D0))
#define	R_NF_SEEDGEN                    (*((volatile INT32U *) 0xD09001D4))
#define	R_NF_PAGEADDR                   (*((volatile INT32U *) 0xD09001D8))
#define	R_NF_PRE_NUM0                   (*((volatile INT32U *) 0xD09001E0))
#define	R_NF_PRE_NUM1                   (*((volatile INT32U *) 0xD09001E4))
#define	R_NF_PRE_NUM2                   (*((volatile INT32U *) 0xD09001E8))
#define	R_NF_PRE_NUM3                   (*((volatile INT32U *) 0xD09001EC))
#define	R_NF_PRE_NUM4                   (*((volatile INT32U *) 0xD09001F0))
#define	R_NF_PRE_NUM5                   (*((volatile INT32U *) 0xD09001F4))
#define	R_NF_PRE_NUM6                   (*((volatile INT32U *) 0xD09001F8))
#define	R_NF_PRE_NUM7                   (*((volatile INT32U *) 0xD09001FC))

#define	R_NF_CS_CTRL                    (*((volatile INT32U *) 0xD09001DC))


/******************************************************************************
 * System control: 0xD0000000
 ******************************************************************************/
#define R_SYSTEM_CTRL_TXACS				(*((volatile INT32U *) 0xD0000000))
#define R_SYSTEM_CTRL0                  (*((volatile INT32U *) 0xD0000008))
#define R_SYSTEM_CTRL1                  (*((volatile INT32U *) 0xD000000C))
#define P_SYSTEM_CLK_EN0                ((volatile INT32U *) 0xD0000010)
#define R_SYSTEM_CLK_EN0                (*((volatile INT32U *) 0xD0000010))
#define R_SYSTEM_CLK_EN1                (*((volatile INT32U *) 0xD0000014))
#define R_SYSTEM_RESET_FLAG             (*((volatile INT32U *) 0xD0000018))
#define R_SYSTEM_CLK_CTRL               (*((volatile INT32U *) 0xD000001C))
#define R_SYSTEM_HALTCP_CTRL            (*((volatile INT32U *) 0xD0000020))
#define R_SYSTEM_MISC_CTRL0             (*((volatile INT32U *) 0xD0000024))
#define R_SYSTEM_WATCHDOG_CTRL          (*((volatile INT32U *) 0xD0000028))
#define R_SYSTEM_WATCHDOG_CLEAR         (*((volatile INT32U *) 0xD000002C))
#define R_SYSTEM_WAIT                   (*((volatile INT32U *) 0xD0000030))
#define R_SYSTEM_STATE                  (*((volatile INT32U *) 0xD000003C))
#define R_SYSTEM_HALT                   (*((volatile INT32U *) 0xD0000034))
#define R_SYSTEM_CTRL2                  (*((volatile INT32U *) 0xD0000040))
#define R_SYSTEM_MISC_CTRL1             (*((volatile INT32U *) 0xD0000044))
#define R_SYSTEM_MIPI_CTRL              (*((volatile INT32U *) 0xD0000048))
#define R_SYSTEM_EXT_CLK_CTRL           (*((volatile INT32U *) 0xD000004C))
#define R_SYSTEM_PERI_CLK_CTRL          (*((volatile INT32U *) 0xD0000050))
#define R_SYSTEM_XCKG_EXT_CTRL          (*((volatile INT32U *) 0xD0000054))
#define R_SYSTEM_CKGEN_CTRL             (*((volatile INT32U *) 0xD0000058))
#define R_SYSTEM_PLLEN                  (*((volatile INT32U *) 0xD000005C))
#define R_SYSTEM_MISC_CTRL2             (*((volatile INT32U *) 0xD0000060))
#define R_SYSTEM_POWER_CTRL0            (*((volatile INT32U *) 0xD0000064))
#define R_SYSTEM_POWER_CTRL1            (*((volatile INT32U *) 0xD0000068))
#define R_SYSTEM_CODEC_CTRL0            (*((volatile INT32U *) 0xD000006C))
#define R_SYSTEM_CODEC_CTRL1            (*((volatile INT32U *) 0xD0000070))
#define R_SYSTEM_MISC_CTRL4             (*((volatile INT32U *) 0xD0000074))
#define R_SYSTEM_TMDS_CTRL              (*((volatile INT32U *) 0xD0000078))
#define R_SYSTEM_EXTCLK_CTRL            (*((volatile INT32U *) 0xD000007C))
#define R_SYSTEM_DSI_CTRL	            (*((volatile INT32U *) 0xD0000084))
#define R_SYSTEM_SOS_PRIORITY           (*((volatile INT32U *) 0xD0000088))
#define R_SYSTEM_MISC_CTRL6             (*((volatile INT32U *) 0xD000008C))
//#define R_SYSTEM_APCK_CTRL              (*((volatile INT32U *) 0xD0000090))
#define R_SYSTEM_ADC_MCLK               (*((volatile INT32U *) 0xD0000094))
#define R_SYSTEM_CODEC_CTRL2            (*((volatile INT32U *) 0xD0000094))
//#define R_SYSTEM_DDR_LDO                (*((volatile INT32U *) 0xD00000AC))
#define R_SD30_CLOCK                    (*((volatile INT32U *) 0xD00000B0))
#define R_SYSTEM_SPU27M_DIV             (*((volatile INT32U *) 0xD00000B4))
#define R_RAMPD0                        (*((volatile INT32U *) 0xD00000B8))
#define R_RAMPD1                        (*((volatile INT32U *) 0xD00000BC))
#define R_REMAP                         (*((volatile INT32U *) 0xD00000C0))
#define R_SYSTEM_APLL0                  (*((volatile INT32U *) 0xD00000C4))
#define R_SYSTEM_PLL_MISC               (*((volatile INT32U *) 0xD00000C8))
#define R_DSI_MISC_CTRL                 (*((volatile INT32U *) 0xD00000CC))
#define P_SYSTEM_CLK_EN2                ((volatile INT32U *) 0xD00000D0)
#define R_SYSTEM_CLK_EN2                (*((volatile INT32U *) 0xD00000D0))
#define R_SYSTEM_CLK_EN3                (*((volatile INT32U *) 0xD00000D4))
#define R_SYSTEM_CLK_EN4                (*((volatile INT32U *) 0xD00000D8))
#define R_SYSTEM_CLK_EN5                (*((volatile INT32U *) 0xD00000DC))
#define R_SYSTEM_SSC_EN                 (*((volatile INT32U *) 0xD00000E0))
#define R_SYSTEM_SSC_CTRL0               (*((volatile INT32U *) 0xD00000E0))
#define R_SYSTEM_SSC_CTRL1              (*((volatile INT32U *) 0xD00000E4))
#define R_SYSTEM_SSC_CTRL2              (*((volatile INT32U *) 0xD00000E8))
#define R_SYSTEM_PWR_RST_EN             (*((volatile INT32U *) 0xD00000EC))
#define R_SYSTEM_SYSFIFO_WAIT           (*((volatile INT32U *) 0xD00000F4))
#define R_SYSTEM_MISC_CTRL7             (*((volatile INT32U *) 0xD00000F8))
#define R_RTC_IO_KEEP_STATUS            (*((volatile INT32U *) 0xD00000FC))
#define R_SYSTEM_MIPI_B_CTRL            (*((volatile INT32U *) 0xD0000100))
#define P_SYSTEM_CLK_EN6                ((volatile INT32U *) 0xD0000104)
#define R_SYSTEM_CLK_EN6                (*((volatile INT32U *) 0xD0000104))
#define R_SYSTEM_CLK_EN7                (*((volatile INT32U *) 0xD0000108))
#define R_SYSTEM_MISC_CTRL8             (*((volatile INT32U *) 0xD000010C))
#define R_SYSTEM_DDR_PLLN               (*((volatile INT32U *) 0xD0000124))
#define R_SYSTEM_DDR_CTRL               (*((volatile INT32U *) 0xD0000128))
#define R_SYSTEM_GTMR_CTRL              (*((volatile INT32U *) 0xD000012C))
#define R_SENSOR_ASYNC_FIFO             (*((volatile INT32U *) 0xD0000130))
#define R_SYSTEM_SOS_PRIORITY1          (*((volatile INT32U *) 0xD0000144))
#define R_SYSTEM_APLL_SEL               (*((volatile INT32U *) 0xD000015C))
#define R_SYSTEM_APLL_SEL2              (*((volatile INT32U *) 0xD0000160))
#define R_SYSTEM_APLL_CTRL3             (*((volatile INT32U *) 0xD0000164))
#define R_SYSTEM_APLL_CTRL4             (*((volatile INT32U *) 0xD0000168))
#define R_SYSTEM_SRAM_SEL               (*((volatile INT32U *) 0xD000017C))
#define P_SYSTEM_MSR_CTRL0               (*((volatile INT32U *) 0xD000008C))
#define P_SYSTEM_MSR_CTRL1               (*((volatile INT32U *) 0xD0000180))
#define P_SYSTEM_MSR_CTRL2               (*((volatile INT32U *) 0xD0000184))
#define P_SYSTEM_MSR_CTRL3               (*((volatile INT32U *) 0xD0000188))
#define R_SW_RESET_KEYCODE0             (*((volatile INT32U *) 0xD00001A0))
#define R_SW_RESET_KEYCODE1             (*((volatile INT32U *) 0xD00001A4))
#define R_SW_SYS_RESET                  (*((volatile INT32U *) 0xD00001A8))
#define R_SYSTEM_MSR_CTRL0              (*((volatile INT32U *) 0xD000008C))
#define R_SYSTEM_MSR_CTRL1              (*((volatile INT32U *) 0xD0000180))
#define R_SYSTEM_MSR_CTRL2              (*((volatile INT32U *) 0xD0000184))
#define R_SYSTEM_MSR_CTRL3              (*((volatile INT32U *) 0xD0000188))

/******************************************************************************
 * Interrupt control: 0xD0100000
 ******************************************************************************/
#define R_INT_FIQ_IN                    (*((volatile INT32U *) 0xD0100004))
#define R_INT_PWRON_EN                  (*((volatile INT32U *) 0xD010000C))
#define R_INT_KECON                     (*((volatile INT32U *) 0xD0100020))
#define R_INT_KEYCH                     (*((volatile INT32U *) 0xD0100024))
#define R_INT_KECON2                    (*((volatile INT32U *) 0xD0100028))
#define R_INT_FIQ_VEC                   (*((volatile INT32U *) 0xD010002C))
#define R_INT_FIQ_MASK                  (*((volatile INT32U *) 0xD0100034))
#define R_INT_FIQ_GMASK                 (*((volatile INT32U *) 0xD0100038))
#define R_INT_FIQ_CHSEL                 (*((volatile INT32U *) 0xD0100058))
#define R_INT_MISC                      (*((volatile INT32U *) 0xD010007C))
#define R_WKU_EN0                       (*((volatile INT32U *) 0xD0100080))
#define R_WKU_EN1                       (*((volatile INT32U *) 0xD0100084))
#define R_WKU_EN2                       (*((volatile INT32U *) 0xD0100088))
#define R_WKU_EN3                       (*((volatile INT32U *) 0xD010008C))
#define R_WKU_STS0                      (*((volatile INT32U *) 0xD0100090))
#define R_WKU_STS1                      (*((volatile INT32U *) 0xD0100094))
#define R_WKU_STS2                      (*((volatile INT32U *) 0xD0100098))
#define R_WKU_STS3                      (*((volatile INT32U *) 0xD010009C))

/******************************************************************************
 * Bus Arbiter: 0xD020007C
 ******************************************************************************/
#define	R_BUS_ARB_ISPAEW                (*((volatile INT32U *) 0xD020007C))
#define	R_BUS_ARB_PSCA_MB               (*((volatile INT32U *) 0xD0200080))
#define	R_BUS_ARB_PSCA                  (*((volatile INT32U *) 0xD0200084))
#define	R_BUS_ARB_RESERVED0             (*((volatile INT32U *) 0xD0200088))
#define	R_BUS_ARB_ISP_MD                (*((volatile INT32U *) 0xD020008C))
#define	R_BUS_ARB_BINING_B              (*((volatile INT32U *) 0xD0200090))
#define	R_BUS_ARB_IPU_0                 (*((volatile INT32U *) 0xD0200094))
#define	R_BUS_ARB_PSCB_MB               (*((volatile INT32U *) 0xD0200098))
#define	R_BUS_ARB_PSCB                  (*((volatile INT32U *) 0xD020009C))
#define	R_BUS_ARB_H264_SCA              (*((volatile INT32U *) 0xD02000A0))
#define	R_BUS_ARB_IPU_1                 (*((volatile INT32U *) 0xD02000A4))
#define	R_BUS_ARB_BINING_B_CH1          (*((volatile INT32U *) 0xD02000A8))
#define	R_BUS_ARB_ROT                   (*((volatile INT32U *) 0xD02000AC))
#define	R_BUS_ARB_DMA                   (*((volatile INT32U *) 0xD02000B0))
#define	R_BUS_ARB_MP3                   (*((volatile INT32U *) 0xD02000B4))
#define	R_BUS_ARB_SPU                   (*((volatile INT32U *) 0xD02000B8))
#define	R_BUS_ARB_SCAN2MB               (*((volatile INT32U *) 0xD02000BC))
#define	R_BUS_ARB_NFC                   (*((volatile INT32U *) 0xD02000C0))
#define	R_BUS_ARB_PPU_PPU               (*((volatile INT32U *) 0xD02000C4))
#define	R_BUS_ARB_BINING_A              (*((volatile INT32U *) 0xD02000C8))
#define	R_BUS_ARB_USBHOST               (*((volatile INT32U *) 0xD02000CC))
#define	R_BUS_ARB_USB20_1               (*((volatile INT32U *) 0xD02000D0))
#define	R_BUS_ARB_SCALER                (*((volatile INT32U *) 0xD02000D4))
#define	R_BUS_ARB_ISP_W                 (*((volatile INT32U *) 0xD02000D8))
#define	R_BUS_ARB_PPU_FB2               (*((volatile INT32U *) 0xD02000DC))
#define	R_BUS_ARB_MB2SCAN               (*((volatile INT32U *) 0xD02000E0))
#define	R_BUS_ARB_BINING_B_CH2          (*((volatile INT32U *) 0xD02000E4))
#define	R_BUS_ARB_HDMI                  (*((volatile INT32U *) 0xD02000E8))
#define	R_BUS_ARB_CSI                   (*((volatile INT32U *) 0xD02000EC))
#define	R_BUS_ARB_JPEG_B                (*((volatile INT32U *) 0xD02000F0))
#define	R_BUS_ARB_LBP                   (*((volatile INT32U *) 0xD02000F4))

#define	R_BUS_ARB_ISP_R                 (*((volatile INT32U *) 0xD02000F8))
#define	R_BUS_ARB_BOX                   (*((volatile INT32U *) 0xD02000FC))
#define	R_BUS_ARB_PPU_SPR               (*((volatile INT32U *) 0xD0200100))
#define	R_BUS_ARB_GPDLA_SDP             (*((volatile INT32U *) 0xD0200104))
#define	R_BUS_ARB_BINING_B_CH3          (*((volatile INT32U *) 0xD0200108))
#define	R_BUS_ARB_IPU_2                 (*((volatile INT32U *) 0xD020010C))
#define	R_BUS_ARB_H264_M1_BS            (*((volatile INT32U *) 0xD0200110))
#define	R_BUS_ARB_SYSFIFO_1             (*((volatile INT32U *) 0xD0200114))
#define	R_BUS_ARB_SYSFIFO               (*((volatile INT32U *) 0xD0200118))
#define	R_BUS_ARB_H264_M0_TABLE         (*((volatile INT32U *) 0xD020011C))
#define	R_BUS_ARB_EMAC                  (*((volatile INT32U *) 0xD0200120))
#define	R_BUS_ARB_ROTATOR               (*((volatile INT32U *) 0xD0200124))
#define	R_BUS_ARB_PSCALER_Y             (*((volatile INT32U *) 0xD0200128))
#define	R_BUS_ARB_JPEG_A                (*((volatile INT32U *) 0xD020012C))
#define	R_BUS_ARB_SPIFC                 (*((volatile INT32U *) 0xD0200130))
#define	R_BUS_ARB_CONV420               (*((volatile INT32U *) 0xD0200134))

/******************************************************************************
 * Session: DMA controller SFR
 * Layer: Driver Layer 1
 * Note: DMA controller SFR
 ******************************************************************************/
#define DMA0_BASE					0xD0300000
#define DMA1_BASE					0xD0300040
#define DMA2_BASE					0xD0300080
#define DMA3_BASE					0xD03000C0
#define DMA4_BASE					0xD0300200
#define DMA5_BASE					0xD0300240
#define DMA6_BASE					0xD0300280
#define DMA7_BASE					0xD03002C0
/******************************************************************************
 * DMA controller: 0xD0300000
 ******************************************************************************/
#define	R_DMA0_CTRL                     (*((volatile INT32U *) 0xD0300000))
#define	R_DMA0_SRC_ADDR                 (*((volatile INT32U *) 0xD0300004))
#define	R_DMA0_TAR_ADDR                 (*((volatile INT32U *) 0xD0300008))
#define	R_DMA0_TX_COUNT                 (*((volatile INT32U *) 0xD030000C))
#define	R_DMA0_SPRITE_SIZE              (*((volatile INT32U *) 0xD0300010))
#define	R_DMA0_TRANSPARENT              (*((volatile INT32U *) 0xD0300014))
#define	R_DMA0_MISC                     (*((volatile INT32U *) 0xD0300018))
#define	R_DMA0_CHECKSUM                 (*((volatile INT32U *) 0xD030001C))

#define	R_DMA1_CTRL                     (*((volatile INT32U *) 0xD0300040))
#define	R_DMA1_SRC_ADDR                 (*((volatile INT32U *) 0xD0300044))
#define	R_DMA1_TAR_ADDR                 (*((volatile INT32U *) 0xD0300048))
#define	R_DMA1_TX_COUNT                 (*((volatile INT32U *) 0xD030004C))
#define	R_DMA1_SPRITE_SIZE              (*((volatile INT32U *) 0xD0300050))
#define	R_DMA1_TRANSPARENT              (*((volatile INT32U *) 0xD0300054))
#define	R_DMA1_MISC                     (*((volatile INT32U *) 0xD0300058))
#define	R_DMA1_CHECKSUM                 (*((volatile INT32U *) 0xD030005C))

#define	R_DMA2_CTRL                     (*((volatile INT32U *) 0xD0300080))
#define	R_DMA2_SRC_ADDR                 (*((volatile INT32U *) 0xD0300084))
#define	R_DMA2_TAR_ADDR                 (*((volatile INT32U *) 0xD0300088))
#define	R_DMA2_TX_COUNT                 (*((volatile INT32U *) 0xD030008C))
#define	R_DMA2_SPRITE_SIZE              (*((volatile INT32U *) 0xD0300090))
#define	R_DMA2_TRANSPARENT              (*((volatile INT32U *) 0xD0300094))
#define	R_DMA2_MISC                     (*((volatile INT32U *) 0xD0300098))
#define	R_DMA2_CHECKSUM                 (*((volatile INT32U *) 0xD030009C))
//#define	R_AES_LOAD_KEY                  (*((volatile INT32U *) 0xD030009C))
#define R_AES_CTRL						(*((volatile INT32U *) 0xD030009C))
#define	R_AES_KEY0                      (*((volatile INT32U *) 0xD03000A0))
#define	R_AES_KEY1                      (*((volatile INT32U *) 0xD03000A4))
#define	R_AES_KEY2                      (*((volatile INT32U *) 0xD03000A8))
#define	R_AES_KEY3                      (*((volatile INT32U *) 0xD03000AC))

#define	R_DES_CTRL                      (*((volatile INT32U *) 0xD0300220))
#define	R_DES_KEY0_LSB                  (*((volatile INT32U *) 0xD0300224))
#define	R_DES_KEY0_MSB                  (*((volatile INT32U *) 0xD0300228))
#define	R_DES_KEY1_LSB                  (*((volatile INT32U *) 0xD030022C))
#define	R_DES_KEY1_MSB                  (*((volatile INT32U *) 0xD0300230))
#define	R_DES_KEY2_LSB                  (*((volatile INT32U *) 0xD0300234))
#define	R_DES_KEY2_MSB                  (*((volatile INT32U *) 0xD0300238))

#define R_AES_REV                       (*((volatile INT32U *) 0xD03000B8))

#define R_DES_IV_LSB                    (*((volatile INT32U *) 0xD03000E0))
#define R_DES_IV_MSB                    (*((volatile INT32U *) 0xD03000E4))

#define R_AES_IV_0                      (*((volatile INT32U *) 0xD03000E8))
#define R_AES_IV_1                      (*((volatile INT32U *) 0xD03000EC))
#define R_AES_IV_2                      (*((volatile INT32U *) 0xD03000F0))
#define R_AES_IV_3                      (*((volatile INT32U *) 0xD03000F4))

#define	R_DMA3_CTRL                     (*((volatile INT32U *) 0xD03000C0))
#define	R_DMA3_SRC_ADDR                 (*((volatile INT32U *) 0xD03000C4))
#define	R_DMA3_TAR_ADDR                 (*((volatile INT32U *) 0xD03000C8))
#define	R_DMA3_TX_COUNT                 (*((volatile INT32U *) 0xD03000CC))
#define	R_DMA3_SPRITE_SIZE              (*((volatile INT32U *) 0xD03000D0))
#define	R_DMA3_TRANSPARENT              (*((volatile INT32U *) 0xD03000D4))
#define	R_DMA3_MISC                     (*((volatile INT32U *) 0xD03000D8))
#define	R_DMA3_CHECKSUM                 (*((volatile INT32U *) 0xD03000DC))

#define	R_DMA_LINE_LEN                  (*((volatile INT32U *) 0xD03001F0))
#define	R_DMA_DEVICE                    (*((volatile INT32U *) 0xD03001F4))
#define	R_DMA_CEMODE                    (*((volatile INT32U *) 0xD03001F8))
#define	R_DMA_INT                       (*((volatile INT32U *) 0xD03001FC))

#define	R_DMA4_CTRL                     (*((volatile INT32U *) 0xD0300200))
#define	R_DMA4_SRC_ADDR                 (*((volatile INT32U *) 0xD0300204))
#define	R_DMA4_TAR_ADDR                 (*((volatile INT32U *) 0xD0300208))
#define	R_DMA4_TX_COUNT                 (*((volatile INT32U *) 0xD030020C))
#define	R_DMA4_SPRITE_SIZE              (*((volatile INT32U *) 0xD0300210))
#define	R_DMA4_TRANSPARENT              (*((volatile INT32U *) 0xD0300214))
#define	R_DMA4_MISC                     (*((volatile INT32U *) 0xD0300218))
#define	R_DMA4_CHECKSUM                 (*((volatile INT32U *) 0xD030021C))

#define	R_DMA5_CTRL                     (*((volatile INT32U *) 0xD0300240))
#define	R_DMA5_SRC_ADDR                 (*((volatile INT32U *) 0xD0300244))
#define	R_DMA5_TAR_ADDR                 (*((volatile INT32U *) 0xD0300248))
#define	R_DMA5_TX_COUNT                 (*((volatile INT32U *) 0xD030024C))
#define	R_DMA5_SPRITE_SIZE              (*((volatile INT32U *) 0xD0300250))
#define	R_DMA5_TRANSPARENT              (*((volatile INT32U *) 0xD0300254))
#define	R_DMA5_MISC                     (*((volatile INT32U *) 0xD0300258))
#define	R_DMA5_CHECKSUM                 (*((volatile INT32U *) 0xD030025C))

#define	R_DMA6_CTRL                     (*((volatile INT32U *) 0xD0300280))
#define	R_DMA6_SRC_ADDR                 (*((volatile INT32U *) 0xD0300284))
#define	R_DMA6_TAR_ADDR                 (*((volatile INT32U *) 0xD0300288))
#define	R_DMA6_TX_COUNT                 (*((volatile INT32U *) 0xD030028C))
#define	R_DMA6_SPRITE_SIZE              (*((volatile INT32U *) 0xD0300290))
#define	R_DMA6_TRANSPARENT              (*((volatile INT32U *) 0xD0300294))
#define	R_DMA6_MISC                     (*((volatile INT32U *) 0xD0300298))
#define	R_DMA6_CHECKSUM                 (*((volatile INT32U *) 0xD030029C))

#define	R_DMA7_CTRL                     (*((volatile INT32U *) 0xD03002C0))
#define	R_DMA7_SRC_ADDR                 (*((volatile INT32U *) 0xD03002C4))
#define	R_DMA7_TAR_ADDR                 (*((volatile INT32U *) 0xD03002C8))
#define	R_DMA7_TX_COUNT                 (*((volatile INT32U *) 0xD03002CC))
#define	R_DMA7_SPRITE_SIZE              (*((volatile INT32U *) 0xD03002D0))
#define	R_DMA7_TRANSPARENT              (*((volatile INT32U *) 0xD03002D4))
#define	R_DMA7_MISC                     (*((volatile INT32U *) 0xD03002D8))
#define	R_DMA7_CHECKSUM                 (*((volatile INT32U *) 0xD03002DC))

/******************************************************************************
 * PICTURE PROCESS UNIT(PPU) CONTROL REGISTERS
 ******************************************************************************/
#define R_PPU_TEXT3_X_POSITION          (*((volatile INT32U *) 0xD0500000)) // TEXT3 X position register
#define R_PPU_TEXT3_Y_POSITION          (*((volatile INT32U *) 0xD0500004)) // TEXT3 Y position register
#define R_PPU_TEXT3_X_OFFSET            (*((volatile INT32U *) 0xD0500008)) // TEXT3 X offset register
#define R_PPU_TEXT3_Y_OFFSET            (*((volatile INT32U *) 0xD050000C)) // TEXT3 Y offset register
#define R_PPU_TEXT3_ATTRIBUTE           (*((volatile INT32U *) 0xD0500010)) // TEXT3 attribute register
#define R_PPU_TEXT3_CTRL                (*((volatile INT32U *) 0xD0500014)) // TEXT3 control register
#define R_PPU_TEXT3_N_PTR               (*((volatile INT32U *) 0xD0500018)) // TEXT3 number array pointer register
#define P_PPU_TEXT3_N_PTR               ((volatile INT32U *) 0xD0500018)    // TEXT3 number array pointer register
#define R_PPU_TEXT3_A_PTR               (*((volatile INT32U *) 0xD050001C)) // TEXT3 attribute array pointer register
#define P_PPU_TEXT3_A_PTR               ((volatile INT32U *) 0xD050001C)    // TEXT3 attribute array pointer register

#define R_PPU_TEXT4_X_POSITION          (*((volatile INT32U *) 0xD0500020)) // TEXT4 X position register
#define R_PPU_TEXT4_Y_POSITION          (*((volatile INT32U *) 0xD0500024)) // TEXT4 Y position register
#define R_PPU_TEXT4_X_OFFSET            (*((volatile INT32U *) 0xD0500028)) // TEXT4 X offset register
#define R_PPU_TEXT4_Y_OFFSET            (*((volatile INT32U *) 0xD050002C)) // TEXT4 Y offset register
#define R_PPU_TEXT4_ATTRIBUTE           (*((volatile INT32U *) 0xD0500030)) // TEXT4 attribute register
#define R_PPU_TEXT4_CTRL                (*((volatile INT32U *) 0xD0500034)) // TEXT4 control register
#define R_PPU_TEXT4_N_PTR               (*((volatile INT32U *) 0xD0500038)) // TEXT4 number array pointer register
#define P_PPU_TEXT4_N_PTR               ((volatile INT32U *) 0xD0500038)    // TEXT4 number array pointer register
#define R_PPU_TEXT4_A_PTR               (*((volatile INT32U *) 0xD050003C)) // TEXT4 attribute array pointer register
#define P_PPU_TEXT4_A_PTR               ((volatile INT32U *) 0xD050003C)    // TEXT4 attribute array pointer register

#define R_PPU_TEXT1_X_POSITION          (*((volatile INT32U *) 0xD0500040)) // TEXT1 X position register
#define R_PPU_TEXT1_Y_POSITION          (*((volatile INT32U *) 0xD0500044)) // TEXT1 Y position register
#define R_PPU_TEXT1_X_OFFSET            (*((volatile INT32U *) 0xD0500340)) // TEXT1 X offset register
#define R_PPU_TEXT1_Y_OFFSET            (*((volatile INT32U *) 0xD0500344)) // TEXT1 Y offset register
#define R_PPU_TEXT1_ATTRIBUTE           (*((volatile INT32U *) 0xD0500048)) // TEXT1 attribute register
#define R_PPU_TEXT1_CTRL                (*((volatile INT32U *) 0xD050004C)) // TEXT1 control register
#define R_PPU_TEXT1_N_PTR               (*((volatile INT32U *) 0xD0500050)) // TEXT1 number array pointer register
#define P_PPU_TEXT1_N_PTR               ((volatile INT32U *) 0xD0500050)    // TEXT1 number array pointer register
#define R_PPU_TEXT1_A_PTR               (*((volatile INT32U *) 0xD0500054)) // TEXT1 attribute array pointer register
#define P_PPU_TEXT1_A_PTR               ((volatile INT32U *) 0xD0500054)    // TEXT1 attribute array pointer register

#define R_PPU_TEXT2_X_POSITION          (*((volatile INT32U *) 0xD0500058)) // TEXT2 X position register
#define R_PPU_TEXT2_Y_POSITION          (*((volatile INT32U *) 0xD050005C)) // TEXT2 Y position register
#define R_PPU_TEXT2_X_OFFSET            (*((volatile INT32U *) 0xD0500350)) // TEXT2 X offset register
#define R_PPU_TEXT2_Y_OFFSET            (*((volatile INT32U *) 0xD0500354)) // TEXT2 Y offset register
#define R_PPU_TEXT2_ATTRIBUTE           (*((volatile INT32U *) 0xD0500060)) // TEXT2 attribute register
#define R_PPU_TEXT2_CTRL                (*((volatile INT32U *) 0xD0500064)) // TEXT2 control register
#define R_PPU_TEXT2_N_PTR               (*((volatile INT32U *) 0xD0500068)) // TEXT2 number array pointer register
#define P_PPU_TEXT2_N_PTR               ((volatile INT32U *) 0xD0500068)    // TEXT2 number array pointer register
#define R_PPU_TEXT2_A_PTR               (*((volatile INT32U *) 0xD050006C)) // TEXT2 attribute array pointer register
#define P_PPU_TEXT2_A_PTR               ((volatile INT32U *) 0xD050006C)    // TEXT2 attribute array pointer register

#define R_PPU_VCOMP_VALUE               (*((volatile INT32U *) 0xD0500070)) // Vertical compression value register
#define R_PPU_VCOMP_OFFSET              (*((volatile INT32U *) 0xD0500074)) // Vertical compression offset register
#define R_PPU_VCOMP_STEP                (*((volatile INT32U *) 0xD0500078)) // Vertical compression step register


#define R_PPU_TEXT1_SEGMENT             (*((volatile INT32U *) 0xD0500080)) // TEXT Layer 1 Segment Address Register
#define P_PPU_TEXT1_SEGMENT             ((volatile INT32U *) 0xD0500080)    // TEXT Layer 1 Segment Address Register
#define R_PPU_TEXT2_SEGMENT             (*((volatile INT32U *) 0xD0500084)) // TEXT Layer 2 Segment Address Register
#define P_PPU_TEXT2_SEGMENT             ((volatile INT32U *) 0xD0500084)    // TEXT Layer 2 Segment Address Register
#define R_PPU_SPRITE_SEGMENT            (*((volatile INT32U *) 0xD0500088)) // Sprite Segment Address Register
#define P_PPU_SPRITE_SEGMENT            ((volatile INT32U *) 0xD0500088)    // Sprite Segment Address Register
#define R_PPU_TEXT3_SEGMENT             (*((volatile INT32U *) 0xD050008C)) // TEXT Layer 3 Segment Address Register
#define P_PPU_TEXT3_SEGMENT             ((volatile INT32U *) 0xD050008C)    // TEXT Layer 3 Segment Address Register
#define R_PPU_TEXT4_SEGMENT             (*((volatile INT32U *) 0xD0500090)) // TEXT Layer 4 Segment Address Register
#define P_PPU_TEXT4_SEGMENT             ((volatile INT32U *) 0xD0500090)    // TEXT Layer 4 Segment Address Register

#define R_PPU_TEXT1_COSINE              (*((volatile INT32U *) 0xD0500348)) // TEXT Layer 1 Cosine Register
#define P_PPU_TEXT1_COSINE              ((volatile INT32U *) 0xD0500348)    // TEXT Layer 1 Cosine Register
#define R_PPU_TEXT1_SINE                (*((volatile INT32U *) 0xD050034C)) // TEXT Layer 1 Sine Register
#define P_PPU_TEXT1_SINE                ((volatile INT32U *) 0xD050034C)    // TEXT Layer 1 Sine Register
#define R_PPU_TEXT2_COSINE              (*((volatile INT32U *) 0xD0500358)) // TEXT Layer 2 Cosine Register
#define P_PPU_TEXT2_COSINE              ((volatile INT32U *) 0xD0500358)    // TEXT Layer 2 Cosine Register
#define R_PPU_TEXT2_SINE                (*((volatile INT32U *) 0xD050035C)) // TEXT Layer 2 Sine Register
#define P_PPU_TEXT2_SINE                ((volatile INT32U *) 0xD050035C)    // TEXT Layer 2 Sine Register
#define R_PPU_TEXT4_COSINE              (*((volatile INT32U *) 0xD05000A0)) // TEXT Layer 4 Cosine Register
#define P_PPU_TEXT4_COSINE              ((volatile INT32U *) 0xD05000A0)    // TEXT Layer 4 Cosine Register
#define R_PPU_TEXT4_SINE                (*((volatile INT32U *) 0xD05000A4)) // TEXT Layer 4 Sine Register
#define P_PPU_TEXT4_SINE                ((volatile INT32U *) 0xD05000A4)    // TEXT Layer 4 Sine Register

#define R_PPU_BLENDING                  (*((volatile INT32U *) 0xD05000A8)) // TEXT layers blending control register
#define R_PPU_FADE_CTRL                 (*((volatile INT32U *) 0xD05000C0)) // Fade effect control register
#define R_PPU_IRQTMV                    (*((volatile INT32U *) 0xD05000D8)) // Vertical hit IRQ control register
#define R_PPU_IRQTMH                    (*((volatile INT32U *) 0xD05000DC)) // Horizontal hit IRQ control register
#define R_PPU_LINE_COUNTER              (*((volatile INT32U *) 0xD05000E0)) // TV line counter register
#define R_PPU_LIGHTPEN_CTRL             (*((volatile INT32U *) 0xD05000E4)) // Light pen control register
#define R_PPU_PALETTE_CTRL              (*((volatile INT32U *) 0xD05000E8)) // Palette control register
#define R_PPU_LPHPOSITION               (*((volatile INT32U *) 0xD05000F8)) // Ligth pen horizontal position register
#define R_PPU_LPVPOSITION               (*((volatile INT32U *) 0xD05000FC)) // Ligth pen vertical position register

#define R_PPU_Y25D_COMPRESS             (*((volatile INT32U *) 0xD0500104)) // Y compress parameter under 2.5D mode
#define R_PPU_SPRITE_CTRL               (*((volatile INT32U *) 0xD0500108)) // Sprite control register

#define R_PPU_WINDOW0_X                 (*((volatile INT32U *) 0xD0500120)) // Window 1 X control register
#define R_PPU_WINDOW0_Y                 (*((volatile INT32U *) 0xD0500124)) // Window 1 Y control register
#define R_PPU_WINDOW1_X                 (*((volatile INT32U *) 0xD0500128)) // Window 2 X control register
#define R_PPU_WINDOW1_Y                 (*((volatile INT32U *) 0xD050012C)) // Window 2 Y control register
#define R_PPU_WINDOW2_X                 (*((volatile INT32U *) 0xD0500130)) // Window 3 X control register
#define R_PPU_WINDOW2_Y                 (*((volatile INT32U *) 0xD0500134)) // Window 3 Y control register
#define R_PPU_WINDOW3_X                 (*((volatile INT32U *) 0xD0500138)) // Window 4 X control register
#define R_PPU_WINDOW3_Y                 (*((volatile INT32U *) 0xD050013C)) // Window 5 Y control register

#define R_PPU_IRQ_EN                    (*((volatile INT32U *) 0xD0500188)) // PPU IRQ enable register
#define R_PPU_IRQ_STATUS                (*((volatile INT32U *) 0xD050018C)) // PPU IRQ status register

#define R_PPU_SPRITE_DMA_SOURCE         (*((volatile INT32U *) 0xD05001C0)) // PPU DMA source address register
#define R_PPU_SPRITE_DMA_TARGET         (*((volatile INT32U *) 0xD05001C4)) // PPU DMA target address register
#define R_PPU_SPRITE_DMA_NUMBER         (*((volatile INT32U *) 0xD05001C8)) // PPU DMA transfer number register
#define R_PPU_HB_CTRL                   (*((volatile INT32U *) 0xD05001CC)) // Horizontal blank control
#define R_PPU_HB_GO                     (*((volatile INT32U *) 0xD05001D0))	// Horizatal blank enable
#define R_PPU_FIFO_STEUP				(*((volatile INT32U *) 0xD05001D4))	// PPU FIFO Mode

#define R_TV_FBI_ADDR                   (*((volatile INT32U *) 0xD05001E0)) // Frame buffer address for TV
#define R_TFT_FBI_ADDR                  (*((volatile INT32U *) 0xD050033C)) // Frame buffer address for TFT-LCD
#define R_PPU_FBO_ADDR                  (*((volatile INT32U *) 0xD05001E8)) // Frame buffer address for PPU Output
#define R_PPU_FB_GO                     (*((volatile INT32U *) 0xD05001F0))
#define R_PPU_BLD_COLOR                 (*((volatile INT32U *) 0xD05001F4)) // Transparent color in RGB565 mode register
#define R_PPU_MISC                      (*((volatile INT32U *) 0xD05001F8))	// PPU MISC Control
#define R_PPU_ENABLE                    (*((volatile INT32U *) 0xD05001FC)) // PPU enable register
#define R_PPU_YUV420OUT                 (*((volatile INT32U *) 0xD0500100)) // PPU/TFT GP420 Mode
#define R_FREE_SIZE                     (*((volatile INT32U *) 0xD050036C)) // PPU Free Size Mode register
#define R_PPU_DEFLICKER_PARA            (*((volatile INT32U *) 0xD0500370)) // PPU de-flicker para register

#define R_HDMI_INTP_FACTOR     (*((volatile INT32U *) 0xD05002F8)) // HDMI Interpolation Factor
#define R_HDMI_INTP_OFFSET     (*((volatile INT32U *) 0xD05002FC)) // HDMI Interpolation Offset

// Sprite virtual 3D
#define R_PPU_SPRITE_X0                 (*((volatile INT32U *) 0xD0500300)) // Sprite X0 register
#define R_PPU_SPRITE_Y0                 (*((volatile INT32U *) 0xD0500304)) // Sprite Y0 register
#define R_PPU_SPRITE_X1                 (*((volatile INT32U *) 0xD0500308)) // Sprite X1 register
#define R_PPU_SPRITE_Y1                 (*((volatile INT32U *) 0xD050030C)) // Sprite Y1 register
#define R_PPU_SPRITE_X2                 (*((volatile INT32U *) 0xD0500310)) // Sprite X2 register
#define R_PPU_SPRITE_Y2                 (*((volatile INT32U *) 0xD0500314)) // Sprite Y2 register
#define R_PPU_SPRITE_X3                 (*((volatile INT32U *) 0xD0500318)) // Sprite X3 register
#define R_PPU_SPRITE_Y3                 (*((volatile INT32U *) 0xD050031C)) // Sprite Y3 register
#define R_PPU_SPRITE_W0                 (*((volatile INT32U *) 0xD0500320)) // Sprite Word 1 register
#define R_PPU_SPRITE_W1                 (*((volatile INT32U *) 0xD0500324)) // Sprite Word 2 register
#define R_PPU_SPRITE_W2                 (*((volatile INT32U *) 0xD0500328)) // Sprite Word 5 register
#define R_PPU_SPRITE_W3                 (*((volatile INT32U *) 0xD050032C)) // Sprite Word 6 register
#define R_PPU_SPRITE_W4                 (*((volatile INT32U *) 0xD0500330)) // Sprite Word 7 register

#define R_PPU_EXTENDSPRITE_CONTROL      (*((volatile INT32U *) 0xD0500334)) // Extend sprite control
#define R_PPU_EXTENDSPRITE_ADDR         (*((volatile INT32U *) 0xD0500338)) // Extend sprite start address
#define R_PPU_RGB_OFFSET                (*((volatile INT32U *) 0xD050037C)) // Special effect RGB offset

#define R_PPU_UI_CTRL  		              (*((volatile INT32U *) 0xD05003A0)) // UI control register
#define R_PPU_UI_ADDR  		              (*((volatile INT32U *) 0xD05003AC)) // UI Address

#define P_PPU_TEXT_H_OFFSET0            ((volatile INT32U *) 0xD0500400)    // Horizontal movement control RAM

#define P_PPU_TEXT_HCMP_VALUE0          ((volatile INT32U *) 0xD0500800)    // Horizontal compression control RAM

#define R_PPU_TEXT3_COS0                (*((volatile INT32U *) 0xD0500800)) // TEXT3 2.5D cosine value register
#define P_PPU_TEXT3_COS0                ((volatile INT32U *) 0xD0500800)    // TEXT3 2.5D cosine array pointer
#define R_PPU_TEXT3_SIN0                (*((volatile INT32U *) 0xD0500804)) // TEXT3 2.5D sine value register
#define P_PPU_TEXT3_SIN0                ((volatile INT32U *) 0xD0500804)    // TEXT3 2.5D sine array pointer

#define P_PPU_PALETTE_RAM0              ((volatile INT32U *) 0xD0501000)    // Palette RAM0 base
#define P_PPU_PALETTE_RAM1              ((volatile INT32U *) 0xD0501400)    // Palette RAM1 base
#define P_PPU_PALETTE_RAM2              ((volatile INT32U *) 0xD0501800)    // Palette RAM2 base
#define P_PPU_PALETTE_RAM3              ((volatile INT32U *) 0xD0501C00)    // Palette RAM3 base

#define P_PPU_SPRITE_ATTRIBUTE_BASE     ((volatile INT32U *) 0xD0502000)    // Sprite attribute RAM base
#define P_PPU_SPRITE_EXTERN_ATTRIBUTE_BASE      ((volatile INT32U *) 0xD0506000)    // Sprite exterd attribute RAM base

/******************************************************************************
 * TV CONTROL REGISTERS
 ******************************************************************************/
#define R_TV_CTRL                       (*((volatile INT32U *) 0xD05000F0)) // TV Control Register
#define R_TV_CTRL2                      (*((volatile INT32U *) 0xD05000F4)) // TV Control2 Register
#define R_TV_SATURATION                 (*((volatile INT32U *) 0xD0500200)) // TV Saturation Control Register
#define R_TV_HUE                        (*((volatile INT32U *) 0xD0500204)) // TV Hue Control Register
#define R_TV_BRIGHTNESS                 (*((volatile INT32U *) 0xD0500208)) // TV Brightness Control Register
#define R_TV_SHARPNESS                  (*((volatile INT32U *) 0xD050020C)) // TV Sharpness Control Register
#define R_TV_Y_GAIN                     (*((volatile INT32U *) 0xD0500210)) // TV Y Gain Control Register
#define R_TV_Y_DELAY                    (*((volatile INT32U *) 0xD0500214)) // TV Y Delay Control Register
#define R_TV_V_POSITION                 (*((volatile INT32U *) 0xD0500218)) // TV Vertical Position Control Register
#define R_TV_H_POSITION                 (*((volatile INT32U *) 0xD050021C)) // TV Horizontal Position Control Register
#define R_TV_VIDEODAC                   (*((volatile INT32U *) 0xD0500220)) // TV Video DAC Control Register

/******************************************************************************
 * TFT CONTROL REGISTERS
 ******************************************************************************/
#define R_TFT_CTRL                      (*((volatile INT32U *) 0xD0500140)) // TFT Control Register
#define R_TFT_V_PERIOD                  (*((volatile INT32U *) 0xD0500144)) // TFT Vertical Period Control Register
#define R_TFT_VS_WIDTH                  (*((volatile INT32U *) 0xD0500148)) // TFT VSYNC Width Control Register
#define R_TFT_V_START                   (*((volatile INT32U *) 0xD050014C)) // TFT Vertical Start Position Control Register
#define R_TFT_V_END                     (*((volatile INT32U *) 0xD0500150)) // TFT Vertical End Position Control Register
#define R_TFT_H_PERIOD                  (*((volatile INT32U *) 0xD0500154)) // TFT Horizontal Period Control Register
#define R_TFT_HS_WIDTH                  (*((volatile INT32U *) 0xD0500158)) // TFT HSYNC Width Control Register
#define R_TFT_H_START                   (*((volatile INT32U *) 0xD050015C)) // TFT Horizontal Start Position Control Register
#define R_TFT_H_END                     (*((volatile INT32U *) 0xD0500160)) // TFT Horizontal End Position Control Register
#define R_TFT_LINE_RGB_ORDER            (*((volatile INT32U *) 0xD0500164)) // TFT Line RGB Order Control Register
#define R_TFT_STATUS                    (*((volatile INT32U *) 0xD0500168)) // TFT Status Register
#define R_TFT_MEM_BUFF_WR               (*((volatile INT32U *) 0xD050016C))
#define R_TFT_MEM_BUFF_RD               (*((volatile INT32U *) 0xD0500170))
#define R_TFT_TE_CTRL                   (*((volatile INT32U *) 0xD0500180))
#define R_TFT_TE_HS_WIDTH               (*((volatile INT32U *) 0xD0500184))
#define R_TFT_VS_START                  (*((volatile INT32U *) 0xD05001B0))
#define R_TFT_VS_END                    (*((volatile INT32U *) 0xD05001B4))
#define R_TFT_HS_START                  (*((volatile INT32U *) 0xD05001B8))
#define R_TFT_HS_END                    (*((volatile INT32U *) 0xD05001BC))

#define R_TFT2_CTRL                     (*((volatile INT32U *) 0xD05002C0)) // TFT Control Register
#define R_TFT2_V_PERIOD                 (*((volatile INT32U *) 0xD05002C4)) // TFT Vertical Period Control Register
#define R_TFT2_VS_WIDTH                 (*((volatile INT32U *) 0xD05002C8)) // TFT VSYNC Width Control Register
#define R_TFT2_V_START                  (*((volatile INT32U *) 0xD05002CC)) // TFT Vertical Start Position Control Register
#define R_TFT2_V_END                    (*((volatile INT32U *) 0xD05002D0)) // TFT Vertical End Position Control Register
#define R_TFT2_H_PERIOD                 (*((volatile INT32U *) 0xD05002D4)) // TFT Horizontal Period Control Register
#define R_TFT2_HS_WIDTH                 (*((volatile INT32U *) 0xD05002D8)) // TFT HSYNC Width Control Register
#define R_TFT2_H_START                  (*((volatile INT32U *) 0xD05002DC)) // TFT Horizontal Start Position Control Register
#define R_TFT2_H_END                    (*((volatile INT32U *) 0xD05002E0)) // TFT Horizontal End Position Control Register
#define R_TFT2_SCA_OUT_RES              (*((volatile INT32U *) 0xD05002E4))

#if 1// need to switch bank on bit 31 of TFT2_CTRL
#define R_TFT2_LINE_RGB_ORDER           (*((volatile INT32U *) 0xD05002D0)) // TFT Line RGB Order Control Register bit[31:16]
#define R_TFT2_STATUS                   (*((volatile INT32U *) 0xD05002D0)) //share with TFT2_LINE_RGB_ORDER on bit[15:0]
#define R_TFT2_MEM_BUFF_WR              (*((volatile INT32U *) 0xD05002D4)) //bit[15:0]
#define R_TFT2_MEM_BUFF_RD              (*((volatile INT32U *) 0xD05002D4)) //share with TFT2_MEM_BUFFER_WR, on bit[31:16[]#define R_TFT2_TE_CTRL                  (*((volatile INT32U *) 0xD0500424))
#define R_TFT2_CLIP_YSTA                (*((volatile INT32U *) 0xD05002D8))
#define R_TFT2_CLIP_YEND                (*((volatile INT32U *) 0xD05002DC))
#define R_TFT2_CLIP_XSTA                (*((volatile INT32U *) 0xD05002E0))
#define R_TFT2_CLIP_XEND                (*((volatile INT32U *) 0xD05002E8))
#define R_TFT2_TE_CTRL                  (*((volatile INT32U *) 0xD05002EC))
#define R_TFT2_TE_HS_WIDTH              (*((volatile INT32U *) 0xD05002F0))
#define R_TFT2_VS_START                 (*((volatile INT32U *) 0xD05002E8))
#define R_TFT2_VS_END                   (*((volatile INT32U *) 0xD05002EC))
#define R_TFT2_HS_START                 (*((volatile INT32U *) 0xD05002F0))
#define R_TFT2_HS_END                   (*((volatile INT32U *) 0xD05002F4))
#else
#define R_TFT2_VS_START                 (*((volatile INT32U *) 0xD05002E4))
#define R_TFT2_VS_END                   (*((volatile INT32U *) 0xD05002E8))
#define R_TFT2_HS_START                 (*((volatile INT32U *) 0xD05002EC))
#define R_TFT2_HS_END                   (*((volatile INT32U *) 0xD05002F0))
#endif

#define R_TFT2_SCA_FACTOR               (*((volatile INT32U *) 0xD05002F8))
#define R_TFT2_SCA_OFFSET               (*((volatile INT32U *) 0xD05002FC))


#define R_TFT_TS_CKV                    (*((volatile INT32U *) 0xD05003C0))
#define R_TFT_TW_CKV                    (*((volatile INT32U *) 0xD05003C4))
#define R_TFT_TS_MISC                   (*((volatile INT32U *) 0xD05003C8))
#define R_TFT_TS_POL                    (*((volatile INT32U *) 0xD05003CC))
#define R_TFT_TS_STV                    (*((volatile INT32U *) 0xD05003D0))
#define R_TFT_TW_STV                    (*((volatile INT32U *) 0xD05003D4))
#define R_TFT_TS_STH                    (*((volatile INT32U *) 0xD05003D8))
#define R_TFT_TW_STH                    (*((volatile INT32U *) 0xD05003DC))
#define R_TFT_TS_OEV                    (*((volatile INT32U *) 0xD05003E0))
#define R_TFT_TW_OEV                    (*((volatile INT32U *) 0xD05003E4))
#define R_TFT_TS_LD                     (*((volatile INT32U *) 0xD05003E8))
#define R_TFT_TW_LD                     (*((volatile INT32U *) 0xD05003EC))
#define R_TFT_TAB0                      (*((volatile INT32U *) 0xD05003F0))
#define R_TFT_TAB1                      (*((volatile INT32U *) 0xD05003F4))
#define R_TFT_TAB2                      (*((volatile INT32U *) 0xD05003F8))
#define R_TFT_TAB3                      (*((volatile INT32U *) 0xD05003FC))

#define R_TFT_CLIP_V_START              (*((volatile INT32U *) 0xD05003B0))
#define R_TFT_CLIP_V_END                (*((volatile INT32U *) 0xD05003B4))
#define R_TFT_CLIP_H_START              (*((volatile INT32U *) 0xD05003B8))
#define R_TFT_CLIP_H_END                (*((volatile INT32U *) 0xD05003BC))

#define P_TFT_COLOR_MAP_BASE            ((volatile INT32U *) 0xD0507000)    // Color mapping RAM base

/******************************************************************************
 * HDMI  CONTROL REGISTERS
 ******************************************************************************/
#define HDMI_TX_BASE                    0xD0D00000

#define	R_HDMITXPHYCONFIG1              (*((volatile INT32U *) 0xD0D0031C))
#define	R_HDMITXPHYCONFIG2              (*((volatile INT32U *) 0xD0D00320))

/******************************************************************************
 * H264 MB2SCAN REGISTERS
 ******************************************************************************/

#define R_MB2SCAN_FRAME_WIDTH			(*((volatile INT32U *) 0xC0270000))
#define R_MB2SCAN_FRAME_HEIGHT			(*((volatile INT32U *) 0xC0270004))
#define R_MB2SCAN_YFRAME_BASE			(*((volatile INT32U *) 0xC0270008))
#define R_MB2SCAN_UVFRAME_BASE			(*((volatile INT32U *) 0xC027000C))
#define R_MB2SCAN_FRAME_OFFSET			(*((volatile INT32U *) 0xC0270010))
#define R_MB2SCAN_CTRL					(*((volatile INT32U *) 0xC0270014))

/******************************************************************************
 * H264 FBC REGISTERS
 ******************************************************************************/
#define R_FBC_ENC_OUT_ADDR				(*(volatile INT32U *)0xC0160000)
#define R_FBC_ENC_CTRL					(*(volatile INT32U *)0xC0160004)
#define R_FBC_ENC_AHB_BYPASS			(*(volatile INT32U *)0xC0160008)
#define R_FBC_IMAGE_WIDTH				(*(volatile INT32U *)0xC016000C)
#define R_FBC_IMAGE_HEIGHT				(*(volatile INT32U *)0xC0160010)
#define R_FBC_MAX_BLK_CNT				(*(volatile INT32U *)0xC0160014)
#define R_FBC_BLK_CNT					(*(volatile INT32U *)0xC0160018)
#define R_FBC_DEC_IN_ADDR				(*(volatile INT32U *)0xC0160020)
#define R_FBC_ENC_BUSY					(*(volatile INT32U *)0xC0160024)
#define R_FBC_DEC_CTRL					(*(volatile INT32U *)0xC016002C)
#define R_FBC_DEC_READY					(*(volatile INT32U *)0xC0160034)
#define R_FBC_ERROR_STATUS				(*(volatile INT32U *)0xC0160038)
#define R_FBC_H264_REF_Y_ADDR_MSB		(*(volatile INT32U *)0xC0160044)
#define R_FBC_H264_REF_UV_ADDR_MSB		(*(volatile INT32U *)0xC0160050)
#define R_FBC_H264_REF_Y_END_MSB		(*(volatile INT32U *)0xC0160058)
#define R_FBC_H264_REF_UV_END_MSB		(*(volatile INT32U *)0xC0160064)
#define R_FBC_DEC_SUM					(*(volatile INT32U *)0xC0160068)
#define R_FBC_ENC_SUM					(*(volatile INT32U *)0xC016006C)
#define R_FBC_H264_WRITE_SUM			(*(volatile INT32U *)0xC0160070)
#define R_FBC_H264_READ_SUM				(*(volatile INT32U *)0xC0160074)

/******************************************************************************
 * SCAN2MB REGISTERS
 ******************************************************************************/
#define R_SCAN2MB_CTRL					(*(volatile INT32U *)0xC0170000)
#define R_SCAN2MB_IMG_WIDTH				(*(volatile INT32U *)0xC0170004)
#define R_SCAN2MB_IMG_HEIGHT			(*(volatile INT32U *)0xC0170008)
#define R_SCAN2MB_BUF_I_ADDR			(*(volatile INT32U *)0xC017000C)
#define R_SCAN2MB_START					(*(volatile INT32U *)0xC0170010)
#define R_SCAN2MB_IRQ_STATUS			(*(volatile INT32U *)0xC0170014)
#define R_SCAN2MB_IRQ_EN				(*(volatile INT32U *)0xC0170018)
#define R_SCAN2MB_RESTART				(*(volatile INT32U *)0xC017001C)
#define R_SCAN2MB_FIFO_LINE				(*(volatile INT32U *)0xC0170020)

/******************************************************************************
 * H264 FIFO REGISTERS
 ******************************************************************************/
#define R_H264_FIFO_CTRL				(*(volatile INT32U *)0xC0190000)
#define R_H264_FIFO_INT					(*(volatile INT32U *)0xC0190004)
#define R_H264_FIFO_ADDR				(*(volatile INT32U *)0xC0190008)
#define R_H264_FIFO_SIZE				(*(volatile INT32U *)0xC019000C)
#define R_H264_FIFO_DBG0				(*(volatile INT32U *)0xC0190018)
#define R_H264_FIFO_DBG1				(*(volatile INT32U *)0xC019001C)

/******************************************************************************
 * H264 REGISTERS
 ******************************************************************************/
#define R_H264_INT_MASK                 (*(volatile INT32U *)0xD1500008)
#define R_H264_INT_STATUS               (*(volatile INT32U *)0xD150000C)
#define R_H264_CONTROL                  (*(volatile INT32U *)0xD1500014)
#define R_H264_STOP_MB                  (*(volatile INT32U *)0xD1500018)
#define R_H264_FRAME_DATA_SIZE          (*(volatile INT32U *)0xD150001C)
#define R_H264_PICTURE_TYPE             (*(volatile INT32U *)0xD1500020)
#define R_H264_QP                       (*(volatile INT32U *)0xD1500024)
#define R_H264_MISC_SETTING             (*(volatile INT32U *)0xD1500028)
#define R_H264_IP_VERSION               (*(volatile INT32U *)0xD150002C)
#define R_H264_PROFILE_LEVEL            (*(volatile INT32U *)0xD1500030)
#define R_H264_BITSTREAM_STARADDR       (*(volatile INT32U *)0xD1500034)
#define R_H264_BITSTREAM_BASEADDR       (*(volatile INT32U *)0xD1500038)
#define R_H264_BITSTREAM_MAXADDR        (*(volatile INT32U *)0xD150003C)
#define R_H264_RAW_FRAME_ADDR           (*(volatile INT32U *)0xD1500040)
#define R_H264_AR_TABLE_ADDR            (*(volatile INT32U *)0xD150004C)
#define R_H264_REC_LUMA_ADDR            (*(volatile INT32U *)0xD1500060)
#define R_H264_REC_CHROMA_ADDR          (*(volatile INT32U *)0xD1500064)
#define R_H264_REC_MB_Y_OFFSET          (*(volatile INT32U *)0xD1500068)
#define R_H264_REF_MB_Y_OFFSET          (*(volatile INT32U *)0xD150006C)
#define R_H264_MAX_MB_Y_OFFSET          (*(volatile INT32U *)0xD1500070)
#define R_H264_FUNCTION_CONTROL0        (*(volatile INT32U *)0xD1500090)
#define R_H264_FUNCTION_CONTROL1        (*(volatile INT32U *)0xD1500094)
#define R_H264_BITSTREAM_LEN            (*(volatile INT32U *)0xD1500098)
#define R_H264_STATUS0                  (*(volatile INT32U *)0xD150009C)
#define R_H264_STATUS1                  (*(volatile INT32U *)0xD15000A0)
#define R_H264_GLOBAL_DEBUG0            (*(volatile INT32U *)0xD15000A4)
#define R_H264_MB_INFO                  (*(volatile INT32U *)0xD15000A8)
#define R_H264_FASTFME_SETTING          (*(volatile INT32U *)0xD15000AC)
#define R_H264_FRAME_CROP_OFFSET        (*(volatile INT32U *)0xD15000B0)
#define R_H264_BITSTREAM_BYPASSLEN      (*(volatile INT32U *)0xD15000B4)
#define R_H264_BITSTREAM_LEN_NOSEI      (*(volatile INT32U *)0xD15000B8)


/******************************************************************************
 * H264 scaler REGISTERS
 ******************************************************************************/
#define R_H264_SCALER_CTRL                 (*(volatile INT32U *)0xD1700000)
#define R_H264_SCALER_STATE              (*(volatile INT32U *)0xD1700004)
#define R_H264_SCALER_SIZE                 (*(volatile INT32U *)0xD1700008)
#define R_H264_SCALER_SIZE                 (*(volatile INT32U *)0xD1700008)
#define R_H264_SCALER_Y_BASE_A        (*(volatile INT32U *)0xD1700010)
#define R_H264_SCALER_Y_BASE_B        (*(volatile INT32U *)0xD1700014)
#define R_H264_SCALER_Y_BASE_C        (*(volatile INT32U *)0xD1700018)
#define R_H264_SCALER_UV_BASE_A      (*(volatile INT32U *)0xD1700020)
#define R_H264_SCALER_UV_BASE_B      (*(volatile INT32U *)0xD1700024)
#define R_H264_SCALER_UV_BASE_C      (*(volatile INT32U *)0xD1700028)
#define R_H264_SCALER_MISC                 (*(volatile INT32U *)0xD1700080)

/*****************************************************************************
* STN CONTROL REGISTERS
******************************************************************************/
#define R_STN_CTRL0                     (*((volatile INT32U *) 0xD050017C))	// STN Control Register 0
#define R_STN_SEG                       (*((volatile INT32U *) 0xD0500200))	// STN Segment Register
#define R_STN_COM                       (*((volatile INT32U *) 0xD0500204))	// STN Column Register
#define R_STN_PIC_COM                   (*((volatile INT32U *) 0xD0500208))	// STN Picture Column Register
#define R_STN_CPWAIT                    (*((volatile INT32U *) 0xD050020C))	// STN CP Wait Register
#define R_STN_CTRL1                     (*((volatile INT32U *) 0xD0500210))	// STN Control Register 1
#define R_STN_GTG_SEG                   (*((volatile INT32U *) 0xD0500214))	// STN Global Timing Generator Segment Register
#define R_STN_GTG_COM                   (*((volatile INT32U *) 0xD0500218))	// STN Global Timing Generator Column Register
#define R_STN_SEG_CLIP                  (*((volatile INT32U *) 0xD050021C))	// STN Clipping Start Segment Register
#define R_STN_COM_CLIP                  (*((volatile INT32U *) 0xD0500144))	// STN Clipping Start Column Register

/******************************************************************************
 * CMOS SENSOR INTERFACE (CSI) CONTROL REGISTERS
 ******************************************************************************/
#define CSI_BASE				        0xD050022C

#define P_CSI_MD_FBADDR                 ((volatile INT32U *) 0xD0500254)    // CSI Motion Detect Buffer Start Address Register
#define P_CSI_MD_FBADDRH                ((volatile INT32U *) 0xD0500298)
#define P_CSI_TG_FBSADDR                ((volatile INT32U *) 0xD0500278)    // CSI Frame Buffer Start Address Register
#define P_CSI_TG_FBSADDR_B              ((volatile INT32U *) 0xD05002AC)

#define R_CSI_GP420_WIDTH               (*((volatile INT32U *) 0xD050022C))
#define R_CSI_GP420_HEIGHT              (*((volatile INT32U *) 0xD0500230))
#define R_TGR_IRQ_STATUS                (*((volatile INT32U *) 0xD0500238))
#define R_TGR_IRQ_EN                    (*((volatile INT32U *) 0xD050023C))
#define R_CSI_TG_CTRL0                  (*((volatile INT32U *) 0xD0500240)) // CSI Timing Generator Control Register 1
#define R_CSI_TG_CTRL1                  (*((volatile INT32U *) 0xD0500244)) // CSI Timing Generator Control Register 2
#define R_CSI_TG_HLSTART                (*((volatile INT32U *) 0xD0500248)) // CSI Horizontal Latch Start Register
#define R_CSI_TG_HEND                   (*((volatile INT32U *) 0xD050024C)) // CSI Horizontal End Register
#define R_CSI_TG_VL0START               (*((volatile INT32U *) 0xD0500250)) // CSI Field 0 Vertical Start Register
#define R_CSI_MD_FBADDR                 (*((volatile INT32U *) 0xD0500254)) // CSI Motion Detect Buffer Start Address Register
#define R_CSI_TG_VEND                   (*((volatile INT32U *) 0xD0500258)) // CSI Vertical End Register
#define R_CSI_TG_HSTART                 (*((volatile INT32U *) 0xD050025C)) // CSI Horizontal Start Register
#define R_CSI_MD_RGBL                   (*((volatile INT32U *) 0xD0500260)) // CSI Motion Detect RGB/YUV Lo-Word Register
#define R_CSI_SEN_CTRL                  (*((volatile INT32U *) 0xD0500264)) // CSI Attribute Control Register
#define R_CSI_TG_BSUPPER                (*((volatile INT32U *) 0xD0500268)) // CSI Blue Screen Upper Limit Control Register
#define R_CSI_TG_BSLOWER                (*((volatile INT32U *) 0xD050026C)) // CSI Blue Screen Lower Limit Control Register
#define R_CSI_MD_RGBH                   (*((volatile INT32U *) 0xD0500270)) // CSI Motion Detect RGB/YUV Hi-Word Register
#define R_CSI_MD_CTRL                   (*((volatile INT32U *) 0xD0500274)) // CSI Motion Detect Control Register
#define R_CSI_TG_FBSADDR                (*((volatile INT32U *) 0xD0500278)) // CSI Frame Buffer Start Address Register
#define R_CSI_TG_VL1START               (*((volatile INT32U *) 0xD0500280)) // CSI Field 1 Vertical Start Register
#define R_CSI_TG_HWIDTH                 (*((volatile INT32U *) 0xD0500284)) // CSI Horizontal Width Control Register
#define R_CSI_TG_VHEIGHT                (*((volatile INT32U *) 0xD0500288)) // CSI Vertical Width Control Register
#define R_CSI_TG_CUTSTART               (*((volatile INT32U *) 0xD050028C)) // CSI Cut Region Start Address Register
#define R_CSI_TG_CUTSIZE                (*((volatile INT32U *) 0xD0500290)) // CSI Cut Size Register
#define R_CSI_TG_VSTART                 (*((volatile INT32U *) 0xD0500294)) // CSI Vertical Start Register
#define R_CSI_TG_HRATIO                 (*((volatile INT32U *) 0xD050029C)) // CSI Horizontal Compress Ratio Control Register
#define R_CSI_TG_VRATIO                 (*((volatile INT32U *) 0xD05002A0)) // CSI Vertical Compress Ratio Control Register
#define R_CSI_MD_HPOS                   (*((volatile INT32U *) 0xD05002A4)) // CSI Motion Detect Horizontal Hit Position Register
#define R_CSI_MD_VPOS                   (*((volatile INT32U *) 0xD05002A8)) // CSI Motion Detect Vertical Hit Position Register
#define R_CSI2_BANK_SEL                 (*((volatile INT32U *) 0xD05002BC))

/******************************************************************************
 * RAMDOM CONTROL REGISTERS
 ******************************************************************************/
#define	R_RANDOM0                       (*((volatile INT32U *) 0xD0500380))
#define	R_RANDOM1                       (*((volatile INT32U *) 0xD0500384))


/******************************************************************************
 * INTERNAL RAM CONTROL REGISTERS
 ******************************************************************************/
#define R_IRAM1CTRL_BASE            0xD2500000
#define R_IRAM1_OVERALL_SIZE		(*((volatile INT32U *) 0xD2500000))
#define R_IRAM1_ROT_STR_ADDR		(*((volatile INT32U *) 0xD2500004))
#define R_IRAM1_ETHER_STR_ADDR		(*((volatile INT32U *) 0xD2500008))
#define R_IRAM1_PPU_STR_ADDR		(*((volatile INT32U *) 0xD250000C))
#define R_IRAM1_GPDLA_STR_ADDR		(*((volatile INT32U *) 0xD2500014))
#define R_IRAM1_USB_STR_ADDR		(*((volatile INT32U *) 0xD2500018))
#define R_IRAM1_PRESCAL_A_STR_ADDR	(*((volatile INT32U *) 0xD250001C))
#define R_IRAM1_PRESCAL_B_STR_ADDR	(*((volatile INT32U *) 0xD2500020))
#define R_IRAM1_SCALER_STR_ADDR		(*((volatile INT32U *) 0xD2500024))
#define R_IRAM1_FFT_STR_ADDR        (*((volatile INT32U *) 0xD2500028))
#define R_IRAM1_H264_STR_ADDR       (*((volatile INT32U *) 0xD250002C))
#define R_IRAM1_EXT_STR_ADDR        (*((volatile INT32U *) 0xD2500034))
#define R_IRAM1_USB_B_STR_ADDR      (*((volatile INT32U *) 0xD2500038))

#define P_IRAM1_ROT_STR_ADDR			((volatile INT32U *) 0xD2500004)
#define P_IRAM1_ETHER_STR_ADDR		((volatile INT32U *) 0xD2500008)
#define P_IRAM1_PPU_STR_ADDR			((volatile INT32U *) 0xD250000C)
#define P_IRAM1_GPDLA_STR_ADDR		((volatile INT32U *) 0xD2500014)
#define P_IRAM1_USB_STR_ADDR			((volatile INT32U *) 0xD2500018)
#define P_IRAM1_PRESCAL_A_STR_ADDR	((volatile INT32U *) 0xD250001C)
#define P_IRAM1_PRESCAL_B_STR_ADDR	((volatile INT32U *) 0xD2500020)
#define P_IRAM1_SCALER_STR_ADDR		((volatile INT32U *) 0xD2500024)
#define P_IRAM1_FFT_STR_ADDR         ((volatile INT32U *) 0xD2500028)
#define P_IRAM1_H264_STR_ADDR        ((volatile INT32U *) 0xD250002C)
#define P_IRAM1_EXT_STR_ADDR         ((volatile INT32U *) 0xD2500034)
#define P_IRAM1_USB_B_STR_ADDR       ((volatile INT32U *) 0xD2500038)

#define R_IRAM2CTRL_BASE             0xD2700000
#define R_IRAM2_OVERALL_SIZE		(*((volatile INT32U *) 0xD2700000))
#define R_IRAM2_ROT_STR_ADDR		(*((volatile INT32U *) 0xD2700004))
#define R_IRAM2_ETHER_STR_ADDR		(*((volatile INT32U *) 0xD2700008))
#define R_IRAM2_PPU_STR_ADDR		(*((volatile INT32U *) 0xD270000C))
#define R_IRAM2_GPDLA_STR_ADDR		(*((volatile INT32U *) 0xD2700014))
#define R_IRAM2_USB_STR_ADDR		(*((volatile INT32U *) 0xD2700018))
#define R_IRAM2_PRESCAL_A_STR_ADDR	(*((volatile INT32U *) 0xD270001C))
#define R_IRAM2_PRESCAL_B_STR_ADDR	(*((volatile INT32U *) 0xD2700020))
#define R_IRAM2_SCALER_STR_ADDR		(*((volatile INT32U *) 0xD2700024))
#define R_IRAM2_FFT_STR_ADDR        (*((volatile INT32U *) 0xD2700028))
#define R_IRAM2_H264_STR_ADDR       (*((volatile INT32U *) 0xD270002C))
#define R_IRAM2_EXT_STR_ADDR        (*((volatile INT32U *) 0xD2700034))
#define R_IRAM2_USB_B_STR_ADDR      (*((volatile INT32U *) 0xD2700038))

#define P_IRAM2_ROT_STR_ADDR			((volatile INT32U *) 0xD2700004)
#define P_IRAM2_ETHER_STR_ADDR		((volatile INT32U *) 0xD2700008)
#define P_IRAM2_PPU_STR_ADDR			((volatile INT32U *) 0xD270000C)
#define P_IRAM2_GPDLA_STR_ADDR		((volatile INT32U *) 0xD2700014)
#define P_IRAM2_USB_STR_ADDR			((volatile INT32U *) 0xD2700018)
#define P_IRAM2_PRESCAL_A_STR_ADDR	((volatile INT32U *) 0xD270001C)
#define P_IRAM2_PRESCAL_B_STR_ADDR	((volatile INT32U *) 0xD2700020)
#define P_IRAM2_SCALER_STR_ADDR		((volatile INT32U *) 0xD2700024)
#define P_IRAM2_FFT_STR_ADDR         ((volatile INT32U *) 0xD2700028)
#define P_IRAM2_H264_STR_ADDR        ((volatile INT32U *) 0xD270002C)
#define P_IRAM2_EXT_STR_ADDR         ((volatile INT32U *) 0xD2700034)
#define P_IRAM2_USB_B_STR_ADDR       ((volatile INT32U *) 0xD2700038)

/******************************************************************************
 * DLA: 0xD1C00000
 ******************************************************************************/
#define DLA_BASE				    ((volatile INT32U *) 0xD1C00000)

/******************************************************************************
 * Scaler0: 0xD0600000
 ******************************************************************************/
#define SCALER0_BASE				0xD0600000
#define R_SCA_CON                   (*((volatile INT32U *) 0xD0600000))

/******************************************************************************
 * PIPLINE SCALER
 ******************************************************************************/
#define PIPELINE_SCALERA_BASE		((volatile INT32U *) 0xD1300000)
#define PIPELINE_SCALERB_BASE 		((volatile INT32U *) 0xD1400000)

/******************************************************************************
 * JPEG: 0xD0700000 / 0xD1B00000
 ******************************************************************************/
#define JPEGA_BASE	0xD0700000
#define JPEGB_BASE	0xD1B00000
#define DEC_JPEG_BASE(x)	INT32U jpg_base = ((x) ? JPEGB_BASE : JPEGA_BASE)

#define P_JPG_QUANT_LUM				((volatile INT32U *) ((jpg_base)+0x00000))
#define P_JPG_QUANT_CHROM			((volatile INT32U *) ((jpg_base)+0x00100))

#define R_JPG_QUANT_VALUE_ONE		(*((volatile INT32U *) ((jpg_base)+0x01204)))
#define R_JPG_QUANT_SCALER			(*((volatile INT32U *) ((jpg_base)+0x0120C)))
#define R_JPG_JFIF					(*((volatile INT32U *) ((jpg_base)+0x01210)))
#define R_JPG_TRUNCATE				(*((volatile INT32U *) ((jpg_base)+0x01214)))
#define R_JPG_VLC_STUFF_BITS		(*((volatile INT32U *) ((jpg_base)+0x01218)))
#define R_JPG_JFIF_FLAG				(*((volatile INT32U *) ((jpg_base)+0x0121C)))
#define R_JPG_RESTART_MCU_NUM		(*((volatile INT32U *) ((jpg_base)+0x01220)))
#define R_JPG_PROGRESSIVE			(*((volatile INT32U *) ((jpg_base)+0x01238)))
#define R_JPG_HSIZE					(*((volatile INT32U *) ((jpg_base)+0x012C4)))
#define R_JPG_VSIZE					(*((volatile INT32U *) ((jpg_base)+0x012CC)))
#define R_JPG_EXTENDED_HSIZE		(*((volatile INT32U *) ((jpg_base)+0x012D8)))
#define R_JPG_EXTENDED_VSIZE		(*((volatile INT32U *) ((jpg_base)+0x012E0)))

#define R_JPG_AUTO_BW_STATUS		(*((volatile INT32U *) ((jpg_base)+0x02350)))
#define R_JPG_AUTO_BW_ADDR			(*((volatile INT32U *) ((jpg_base)+0x02354)))
#define R_JPG_AHB_WR_CNT			(*((volatile INT32U *) ((jpg_base)+0x0236C)))
#define R_JPG_VLC_ADDR_B			(*((volatile INT32U *) ((jpg_base)+0x02370)))
#define R_JPG_AUTO_CTRL0			(*((volatile INT32U *) ((jpg_base)+0x02374)))
#define R_JPG_AUTO_CTRL1			(*((volatile INT32U *) ((jpg_base)+0x02378)))
#define R_JPG_AUTO_CTRL2			(*((volatile INT32U *) ((jpg_base)+0x0237C)))
#define R_JPG_SCALER_IMAGE_SIZE		(*((volatile INT32U *) ((jpg_base)+0x02380)))		// This register is used to notify scaler the image size when bypass scaler mode is used in JPEG engine
#define R_JPG_Y_FRAME_ADDR			(*((volatile INT32U *) ((jpg_base)+0x02380)))
#define R_JPG_U_FRAME_ADDR			(*((volatile INT32U *) ((jpg_base)+0x02384)))
#define R_JPG_V_FRAME_ADDR			(*((volatile INT32U *) ((jpg_base)+0x02388)))
#define R_JPG_READ_CTRL				(*((volatile INT32U *) ((jpg_base)+0x0238C)))
#define R_JPG_CLIP_START_X			(*((volatile INT32U *) ((jpg_base)+0x02390)))
#define R_JPG_CLIP_START_Y			(*((volatile INT32U *) ((jpg_base)+0x02394)))
#define R_JPG_CLIP_WIDTH			(*((volatile INT32U *) ((jpg_base)+0x02398)))
#define R_JPG_CLIP_HEIGHT			(*((volatile INT32U *) ((jpg_base)+0x0239C)))
#define R_JPG_BS_OFFSET				(*((volatile INT32U *) ((jpg_base)+0x023A0)))
#define R_JPG_RESTART_MARKER_CNT	(*((volatile INT32U *) ((jpg_base)+0x023A4)))
#define R_JPG_CTRL					(*((volatile INT32U *) ((jpg_base)+0x023C0)))
#define R_JPG_VLC_TOTAL_LEN			(*((volatile INT32U *) ((jpg_base)+0x023C4)))
#define R_JPG_VLC_ADDR				(*((volatile INT32U *) ((jpg_base)+0x023CC)))
#define R_JPG_GP420_CTRL			(*((volatile INT32U *) ((jpg_base)+0x023D0)))
#define R_JPG_RING_FIFO				(*((volatile INT32U *) ((jpg_base)+0x023D4)))
#define R_JPG_IMAGE_SIZE			(*((volatile INT32U *) ((jpg_base)+0x023D8)))
#define R_JPG_RESET					(*((volatile INT32U *) ((jpg_base)+0x023DC)))
#define R_JPG_INT_CTRL				(*((volatile INT32U *) ((jpg_base)+0x023E0)))
#define R_JPG_INT_FLAG				(*((volatile INT32U *) ((jpg_base)+0x023E4)))
#define R_JPG_ENCODE_AHB_CNT		(*((volatile INT32U *) ((jpg_base)+0x023F0)))
#define R_JPG_ENCODE_VLC_CNT		(*((volatile INT32U *) ((jpg_base)+0x023F4)))

#define P_JPG_DC_LUM_MINCODE		((volatile INT32U *) ((jpg_base)+0x03000))
#define P_JPG_DC_LUM_VALPTR			((volatile INT32U *) ((jpg_base)+0x03040))
#define P_JPG_DC_LUM_HUFFVAL		((volatile INT32U *) ((jpg_base)+0x03080))

#define P_JPG_DC_CHROM_MINCODE		((volatile INT32U *) ((jpg_base)+0x03100))
#define P_JPG_DC_CHROM_VALPTR		((volatile INT32U *) ((jpg_base)+0x03140))
#define P_JPG_DC_CHROM_HUFFVAL		((volatile INT32U *) ((jpg_base)+0x03180))

#define P_JPG_AC_LUM_MINCODE		((volatile INT32U *) ((jpg_base)+0x03200))
#define P_JPG_AC_LUM_VALPTR			((volatile INT32U *) ((jpg_base)+0x03240))

#define P_JPG_AC_CHROM_MINCODE		((volatile INT32U *) ((jpg_base)+0x03280))
#define P_JPG_AC_CHROM_VALPTR		((volatile INT32U *) ((jpg_base)+0x032C0))

#define P_JPG_AC_LUM_HUFFVAL		((volatile INT32U *) ((jpg_base)+0x04000))
#define P_JPG_AC_CHROM_HUFFVAL		((volatile INT32U *) ((jpg_base)+0x04400))

#define P_JPG_AUTO_SW_VLC_LEN		((volatile INT32U *) ((jpg_base)+0x07000))

/******************************************************************************
 * SPU control: 0xD0400000
 ******************************************************************************/
#define P_SPU_BASE_ADDR			0xD0400000
#define P_SPU_CH_EN			((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E00))	// channel 0~15 enable
#define P_SPU_MAIN_VOLUME		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E04))	// main volume
#define P_SPU_CH_FIQ_EN			((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E08))	// channel 0~15 FIQ enable
#define P_SPU_CH_FIQ_STATUS		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E0C))	// channel 0~15 FIQ status
#define P_SPU_BEAT_BASE_COUNTER		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E10))	// beat base counter
#define P_SPU_BEAT_COUNTER		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E14))	// beat counter
#define P_SPU_ENV_CLK_CH0_3		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E18))	// channel 0~3 envelope interval selection
#define P_SPU_ENV_CLK_CH4_7		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E1C))	// channel 4~7 envelope interval selection
#define P_SPU_ENV_CLK_CH8_11		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E20))	// channel 8~11 envelope interval selection
#define P_SPU_ENV_CLK_CH12_15		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E24))	// channel 12~15 envelope interval selection
#define P_SPU_ENV_RAMP_DOWN		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E28))	// channel 0~15 envelope fast ramp down
#define P_SPU_CH_STOP_STATUS		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E2C))	// channel 0~15 stop channel status
#define P_SPU_CH_ZC_ENABLE		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E30))	// channel 0~15 zero crossing enable
#define P_SPU_CONTROL_FLAG		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E34))	// SPU control flags
#define P_SPU_COMPRESSOR_CTRL		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E38))	// compressor control
#define P_SPU_CH_STATUS			((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E3C))	// channel 0~15 status
#define P_SPU_WAVE_IN_LEFT		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E40))	// left channel mixer input
#define P_SPU_WAVE_IN_RIGHT		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E44))	// right channel mixer input
#define P_SPU_WAVE_OUT_LEFT		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E48))	// wave output left of 32 channel + software channel
#define P_SPU_WAVE_OUT_RIGHT		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E4C))	// wave output right of 32 channel + software channel
#define P_SPU_CH_REPEAT_EN		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E50))	// channel 0~15 repeat enable control
#define P_SPU_CH_ENV_MODE		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E54))	// channel 0~15 envelope mode
#define P_SPU_CH_TONE_RELEASE		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E58))	// channel 0~15 tone release control
#define P_SPU_CH_IRQ_STATUS		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E5C))	// channel 0~15 envelope IRQ status
#define P_SPU_CH_PITCH_BEND_EN		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E60))	// channel 0~15 pitch bend enable
#define P_SPU_ATTACK_RELEASE_TIME	((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E68))	// attack/release time control
#define P_SPU_BENK_ADDR			((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E7C))	// wave table's bank address

#define P_SPU_CH_EN_HIGH		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E80))	// channel 16~31 enable
#define P_SPU_CH_FIQ_EN_HIGH		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E88))	// channel 16~31 FIQ enable
#define P_SPU_CH_FIQ_STATUS_HIGH	((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E8C))	// channel 16~31 FIQ status
#define P_SPU_POST_WAVE_CTRL		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E94))	// post wave counter and control
#define P_SPU_ENV_CLK_CH16_19		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E98))	// channel 16~19 envelope interval selection
#define P_SPU_ENV_CLK_CH20_23		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0E9C))	// channel 20~23 envelope interval selection
#define P_SPU_ENV_CLK_CH24_27		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0EA0))	// channel 24~27 envelope interval selection
#define P_SPU_ENV_CLK_CH28_31		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0EA4))	// channel 28~31 envelope interval selection
#define P_SPU_ENV_RAMP_DOWN_HIGH	((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0EA8))	// channel 16~31 envelope fast ramp down
#define P_SPU_CH_ZC_ENABLE_HIGH		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0EB0))	// channel 16~31 zero crossing enable
#define P_SPU_CH_STOP_STATUS_HIGH	((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0EAC))	// channel 16~31 stop channel status
#define P_SPU_CH_STATUS_HIGH		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0EBC))	// channel 16~31 status
#define P_SPU_POST_WAVE_OUT_LEFT	((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0EC8))	// wave output left of 32 channel
#define P_SPU_POST_WAVE_OUT_RIGHT	((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0ECC))	// wave output right of 32 channel
#define P_SPU_CH_REPEAT_EN_HIGH		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0ED0))	// channel 16~31 repeat enable control
#define P_SPU_CH_ENV_MODE_HIGH		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0ED4))	// channel 16~31 envelope mode
#define P_SPU_CH_TONE_RELEASE_HIGH	((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0ED8))	// channel 16~31 tone release control
#define P_SPU_CH_IRQ_STATUS_HIGH	((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0EDC))	// channel 16~31 envelope IRQ status
#define P_SPU_CH_PITCH_BEND_EN_HIGH	((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0EE0))	// channel 16~31 pitch bend enable
#define P_SPU_CONTROL_FLAG2		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0EF4))	// channel 0~15 enable
#define P_SPU_27M_CLOCKSEL		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x0F00))
/******************************************************************************
 * SPU channel 0 internal SRAM
 ******************************************************************************/
#define P_SPU_SRAM			0xC0110000
#define P_SPU_SRAM_BASE			((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1000))
#define P_SPU_WAVE_ADDR_LOW		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1000))
#define P_SPU_MODE			((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1004))
#define P_SPU_LOOP_ADDR		        ((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1008))
#define P_SPU_PAN_VELOCITY		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x100C))
#define P_SPU_ENVELOPE_0		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1010))
#define P_SPU_ENVELOPE_DATA		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1014))
#define P_SPU_ENVELOPE_1		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1018))
#define P_SPU_ENV_ADDR_HIGH		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x101C))
#define P_SPU_ENV_ADDR_LOW		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1020))
#define P_SPU_WAVE_DATA_0		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1024))
#define P_SPU_LOOP_CTRL			((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1028))
#define P_SPU_WAVE_DATA			((volatile INT32U *) (P_SPU_BASE_ADDR + 0x102C))
#define P_SPU_ADPCM_SEL			((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1034))
#define P_SPU_WL_ADDR_HIGH		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1038))
#define P_SPU_WAVE_ADDR_HIGH		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x103C))

#define P_SPU_PHASE_HIGH		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1800))
#define P_SPU_PHASE_ACC_HIGH		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1804))
#define P_SPU_TARGET_PHASE_HIGH		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1808))
#define P_SPU_RAMP_DOWN_CLK		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x180C))
#define P_SPU_PHASE			((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1810))
#define P_SPU_PHASE_ACC			((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1814))
#define P_SPU_TARGET_PHASE		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x1818))
#define P_SPU_PHASE_CTRL		((volatile INT32U *) (P_SPU_BASE_ADDR + 0x181C))

#define P_SPU_DMADATAL			((volatile INT32U *)(0xC0110000))
#define P_SPU_DMADATAR			((volatile INT32U *)(0xC0110004))

/******************************************************************************
 * I2C control 0xC00B0000
 ******************************************************************************/
#define I2C0_BASE					0xC00B0000
#define I2C1_BASE					0xC00C0000
#define I2C2_BASE					0xC0120000

#define R_I2C0_MISC                 (*((volatile INT32U *) (I2C0_BASE + 0x18)))
#define R_I2C1_MISC                 (*((volatile INT32U *) (I2C1_BASE + 0x18)))
#define R_I2C2_MISC                 (*((volatile INT32U *) (I2C2_BASE + 0x18)))

#define R_I2C0_TXDMA                (((volatile INT32U *) (I2C0_BASE + 0x1C)))
#define R_I2C1_TXDMA                (((volatile INT32U *) (I2C1_BASE + 0x1C)))
#define R_I2C2_TXDMA                (((volatile INT32U *) (I2C2_BASE + 0x1C)))
#define R_I2C0_RXDMA                (((volatile INT32U *) (I2C0_BASE + 0x20)))
#define R_I2C1_RXDMA                (((volatile INT32U *) (I2C1_BASE + 0x20)))
#define R_I2C2_RXDMA                (((volatile INT32U *) (I2C2_BASE + 0x20)))

/******************************************************************************
 *MIC / Noise_Detection / Digital AGC
 ******************************************************************************/
#define R_MIC_MACRO                     (*((volatile INT32U *) 0xC00D0080))
#define R_MIC_PGA                       (*((volatile INT32U *) 0xC00D0084))
#define R_MIC_INTEN                     (*((volatile INT32U *) 0xC00D0088))
#define R_MAF_CNT                       (*((volatile INT32U *) 0xC00D008C))
#define R_MIC_MAX                       (*((volatile INT32U *) 0xC00D0090))
#define R_MIC_MIN                       (*((volatile INT32U *) 0xC00D0094))
#define R_DAGC_UTHRES                   (*((volatile INT32U *) 0xC00D0098))
#define R_MIC_FLAG                      (*((volatile INT32U *) 0xC00D009C))
#define R_NOISEDET_CTRL0                (*((volatile INT32U *) 0xC00D00A0))
#define R_NOISEDET_CTRL1                (*((volatile INT32U *) 0xC00D00A4))
#define R_NOISEDET_CTRL2                (*((volatile INT32U *) 0xC00D00A8))
#define R_NOISEDET_CTRL3                (*((volatile INT32U *) 0xC00D00AC))
#define R_DAGC_CTRL                     (*((volatile INT32U *) 0xC00D00B0))
#define R_DAGC_TIME                     (*((volatile INT32U *) 0xC00D00B4))
#define R_DAGC_ENABLE                   (*((volatile INT32U *) 0xC00D00B8))
#define R_DAGC_THRESHOLD                (*((volatile INT32U *) 0xC00D00BC))
#define R_DAGC_HOLD_CYC                 (*((volatile INT32U *) 0xC00D00C0))
#define R_DAGC_RELEASE_CYC              (*((volatile INT32U *) 0xC00D00C4))
#define R_PDM_RDS                       (*((volatile INT32U *) 0xC00D00C8))
#define R_DIGITAL_GAIN                  (*((volatile INT32U *) 0xC00D00CC))

/******************************************************************************
 *Audio Compressor
 ******************************************************************************/
#define R_AUD_COMPRESSOR_THRES_EN       (*((volatile INT32U *) 0xD2100000))
#define R_AUD_COMPRESSOR_ATT_REL        (*((volatile INT32U *) 0xD2100004))
#define R_AUD_COMPRESSOR_CFG1           (*((volatile INT32U *) 0xD2100008))

/******************************************************************************
 * MIPI control: 0xD0F0000
 ******************************************************************************/
#define MIPI1_BASE					0xD1000000	//support 1-lane
#define MIPI0_BASE					0xD0F00000	//support 2-lane /1-lane
#define R_MIPIA_GLB_CSR                 (*((volatile INT32U *) 0xD0F00000))
#define R_MIPIB_GLB_CSR                 (*((volatile INT32U *) 0xD1000000))

/******************************************************************************
 *I2S
 ******************************************************************************/
#define I2S0_BASE					0xC0200000
#define I2S1_BASE					0xC0210000
#define I2S2_BASE					0xC0220000
#define I2S3_BASE					0xC0230000

/******************************************************************************
 * I2S_TX: 0xC0200000
 ******************************************************************************/
#define	P_I2STX_DATA                    (((volatile INT32U *) (I2S0_BASE + 0x0004)))
#define	P_I2S1TX_DATA                   (((volatile INT32U *) (I2S1_BASE + 0x0004)))
#define	P_I2S2TX_DATA                   (((volatile INT32U *) (I2S2_BASE + 0x0004)))
#define	P_I2S3TX_DATA                   (((volatile INT32U *) (I2S3_BASE + 0x0004)))

#define	R_I2STX_CTRL                    (*((volatile INT32U *) (I2S0_BASE + 0x0000)))
#define	R_I2STX_DATA                    (*((volatile INT32U *) (I2S0_BASE + 0x0004)))
#define	R_I2STX_STATUS                  (*((volatile INT32U *) (I2S0_BASE + 0x0008)))

#define	R_I2S0TX_CTRL                    (*((volatile INT32U *) (I2S0_BASE + 0x0000)))
#define	R_I2S1TX_CTRL                    (*((volatile INT32U *) (I2S1_BASE + 0x0000)))
#define	R_I2S2TX_CTRL                    (*((volatile INT32U *) (I2S2_BASE + 0x0000)))
#define	R_I2S3TX_CTRL                    (*((volatile INT32U *) (I2S3_BASE + 0x0000)))

/******************************************************************************
 * I2S_RX: 0xC0200010
 ******************************************************************************/
#define	P_I2SRX_DATA                    (((volatile INT32U *) (I2S0_BASE + 0x0014)))
#define	P_I2S1RX_DATA                   (((volatile INT32U *) (I2S1_BASE + 0x0014)))
#define	P_I2S2RX_DATA                   (((volatile INT32U *) (I2S2_BASE + 0x0014)))
#define	P_I2S3RX_DATA                   (((volatile INT32U *) (I2S3_BASE + 0x0014)))

#define	R_I2SRX_CTRL                    (*((volatile INT32U *) (I2S0_BASE + 0x0010)))
#define	R_I2SRX_DATA                    (*((volatile INT32U *) (I2S0_BASE + 0x0014)))
#define	R_I2SRX_STATUS                  (*((volatile INT32U *) (I2S0_BASE + 0x0018)))

#define	R_I2S0RX_CTRL                    (*((volatile INT32U *) (I2S0_BASE + 0x0010)))
#define	R_I2S1RX_CTRL                    (*((volatile INT32U *) (I2S1_BASE + 0x0010)))
#define	R_I2S2RX_CTRL                    (*((volatile INT32U *) (I2S2_BASE + 0x0010)))
#define	R_I2S3RX_CTRL                    (*((volatile INT32U *) (I2S3_BASE + 0x0010)))


/******************************************************************************
 * CDSP: 0xD0800000
 ******************************************************************************/
#define CDSP_BASE                       0xD0800000
// cdsp
#define R_CDSP_MACRO_CTRL               (*(volatile INT32U *)(CDSP_BASE+0x000))
#define R_CDSP_BADPIX_CTRL              (*(volatile INT32U *)(CDSP_BASE+0x004))
#define R_CDSP_BADPIX_CTHR              (*(volatile INT32U *)(CDSP_BASE+0x008))
#define R_CDSP_YUVSP_EFFECT_OFFSET      (*(volatile INT32U *)(CDSP_BASE+0x00C))

#define R_CDSP_IMG_TYPE                 (*(volatile INT32U *)(CDSP_BASE+0x010))
#define R_CDSP_YUVSP_EFFECT_SCALER      (*(volatile INT32U *)(CDSP_BASE+0x014))
#define	R_CDSP_OPB_CTRL                 (*(volatile INT32U *)(CDSP_BASE+0x018))
#define	R_CDSP_YUVSP_EFFECT_BTHR        (*(volatile INT32U *)(CDSP_BASE+0x01C))

#define	R_CDSP_OPB_TYPE                 (*(volatile INT32U *)(CDSP_BASE+0x020))
#define	R_CDSP_OPB_HOFFSET              (*(volatile INT32U *)(CDSP_BASE+0x024))
#define	R_CDSP_OPB_VOFFSET              (*(volatile INT32U *)(CDSP_BASE+0x028))

#define	R_CDSP_OPB_RAVG                 (*(volatile INT32U *)(CDSP_BASE+0x040))
#define	R_CDSP_OPB_GRAVG                (*(volatile INT32U *)(CDSP_BASE+0x044))
#define	R_CDSP_OPB_BAVG                 (*(volatile INT32U *)(CDSP_BASE+0x048))
#define	R_CDSP_OPB_GBAVG                (*(volatile INT32U *)(CDSP_BASE+0x04C))
#define	R_CDSP_DUMMY                    (*(volatile INT32U *)(CDSP_BASE+0x050))

#define	R_CDSP_BINMODE_START            (*(volatile INT32U *)(CDSP_BASE+0x054))
#define	R_CDSP_BINMODE_SIZE             (*(volatile INT32U *)(CDSP_BASE+0x058))
#define	R_CDSP_BINMODE_CTRL             (*(volatile INT32U *)(CDSP_BASE+0x05C))

#define	R_CDSP_PIXEL_TIMEOUT_CTRL		(*(volatile INT32U *)(CDSP_BASE+0x060))
/*DN3D CTRL*/
#define R_ISP_DN3D_SIZE					(*(volatile INT32U *)(CDSP_BASE+0x064))
#define R_ISP_DN3D_ADDR					(*(volatile INT32U *)(CDSP_BASE+0x068))
#define R_ISP_DN3D_CTRL					(*(volatile INT32U *)(CDSP_BASE+0x06C))

/*AWB Threshold */
#define R_ISP_AWB_THRESHOLD_1415		(*(volatile INT32U *)(CDSP_BASE+0x070))
#define R_ISP_AWB_THRESHOLD_1617		(*(volatile INT32U *)(CDSP_BASE+0x074))
#define R_ISP_AWB_THRESHOLD_1415_2		(*(volatile INT32U *)(CDSP_BASE+0x078))
#define R_ISP_AWB_THRESHOLD_1617_2		(*(volatile INT32U *)(CDSP_BASE+0x07C))

/* Color Correction */
#define R_ISP_CC_TH012                  (*(volatile INT32U *)(CDSP_BASE+0x080))
#define R_ISP_CC_TH345                  (*(volatile INT32U *)(CDSP_BASE+0x084))
#define R_ISP_CC_TH6_SC01               (*(volatile INT32U *)(CDSP_BASE+0x088))
#define R_ISP_CC_SC234                  (*(volatile INT32U *)(CDSP_BASE+0x08C))
#define R_ISP_CC_SC567                  (*(volatile INT32U *)(CDSP_BASE+0x090))


#define	R_CDSP_HSCALE_EVEN_PVAL         (*(volatile INT32U *)(CDSP_BASE+0x094))

#define	R_CDSP_HRAW_SCLDW_FACTOR        (*(volatile INT32U *)(CDSP_BASE+0x0A0))
#define	R_CDSP_HSCALE_ODD_PVAL          (*(volatile INT32U *)(CDSP_BASE+0x0A4))
#define	R_CDSP_VYUV_SCLDW_FACTOR        (*(volatile INT32U *)(CDSP_BASE+0x0A8))
#define	R_CDSP_VSCALE_ACC_INIT          (*(volatile INT32U *)(CDSP_BASE+0x0AC))

#define	R_CDSP_HYUV_SCLDW_FACTOR        (*(volatile INT32U *)(CDSP_BASE+0x0B0))
#define	R_CDSP_HSCALE_ACC_INIT          (*(volatile INT32U *)(CDSP_BASE+0x0B4))
#define	R_CDSP_SCLDW_CTRL               (*(volatile INT32U *)(CDSP_BASE+0x0B8))
#define	R_CDSP_SCALE_FACTOR_CTRL        (*(volatile INT32U *)(CDSP_BASE+0x0BC))

#define	R_CDSP_WHBAL_RSETTING           (*(volatile INT32U *)(CDSP_BASE+0x0C0))
#define	R_CDSP_WHBAL_GRSETTING          (*(volatile INT32U *)(CDSP_BASE+0x0C4))
#define	R_CDSP_WHBAL_BSETTING           (*(volatile INT32U *)(CDSP_BASE+0x0C8))
#define	R_CDSP_WHBAL_GBSETTING          (*(volatile INT32U *)(CDSP_BASE+0x0CC))

#define	R_CDSP_YUVSPEC_MODE             (*(volatile INT32U *)(CDSP_BASE+0x0D0))
#define	R_CDSP_GLOBAL_GAIN              (*(volatile INT32U *)(CDSP_BASE+0x0D4))
#define	R_CDSP_IMCROP_CTRL              (*(volatile INT32U *)(CDSP_BASE+0x0D8))
#define	R_CDSP_IMCROP_HOFFSET           (*(volatile INT32U *)(CDSP_BASE+0x0DC))
#define	R_CDSP_IMCROP_HSIZE             (*(volatile INT32U *)(CDSP_BASE+0x0E0))
#define	R_CDSP_IMCROP_VOFFSET           (*(volatile INT32U *)(CDSP_BASE+0x0E4))
#define	R_CDSP_IMCROP_VSIZE             (*(volatile INT32U *)(CDSP_BASE+0x0E8))

#define	R_CDSP_INP_DENOISE_THR          (*(volatile INT32U *)(CDSP_BASE+0x0EC))

#define	R_CDSP_INP_MIRROR_CTRL          (*(volatile INT32U *)(CDSP_BASE+0x0F0))
#define	R_CDSP_RGB_SPEC_ROT_MODE        (*(volatile INT32U *)(CDSP_BASE+0x0F4))

#define	R_CDSP_HPF_LCOEF0               (*(volatile INT32U *)(CDSP_BASE+0x100))
#define	R_CDSP_HPF_LCOEF1               (*(volatile INT32U *)(CDSP_BASE+0x104))
#define	R_CDSP_HPF_LCOEF2               (*(volatile INT32U *)(CDSP_BASE+0x108))
#define	R_CDSP_LH_DIV_CTRL              (*(volatile INT32U *)(CDSP_BASE+0x10C))

#define	R_CDSP_INP_EDGE_CTRL            (*(volatile INT32U *)(CDSP_BASE+0x110))
#define	R_CDSP_INP_QTHR                 (*(volatile INT32U *)(CDSP_BASE+0x114))
#define	R_CDSP_INP_QCNT                 (*(volatile INT32U *)(CDSP_BASE+0x118))

/* Color Correction */
#define	R_CDSP_CC_COF0                  (*(volatile INT32U *)(CDSP_BASE+0x11C))
#define	R_CDSP_CC_COF1                  (*(volatile INT32U *)(CDSP_BASE+0x120))
#define	R_CDSP_CC_COF2                  (*(volatile INT32U *)(CDSP_BASE+0x124))
#define	R_CDSP_CC_COF3                  (*(volatile INT32U *)(CDSP_BASE+0x128))
#define	R_CDSP_CC_COF4                  (*(volatile INT32U *)(CDSP_BASE+0x12C))

#define	R_CDSP_RB_CLAMP_CTRL            (*(volatile INT32U *)(CDSP_BASE+0x134))
#define	R_CDSP_UVSCALE_COND0            (*(volatile INT32U *)(CDSP_BASE+0x138))
#define	R_CDSP_UVSCALE_COND1            (*(volatile INT32U *)(CDSP_BASE+0x13C))

#define	R_CDSP_YUV_CTRL                 (*(volatile INT32U *)(CDSP_BASE+0x140))
#define	R_CDSP_BIST_EN                  (*(volatile INT32U *)(CDSP_BASE+0x144))

#define	R_CDSP_HUE_ROT_U                (*(volatile INT32U *)(CDSP_BASE+0x14C))

#define	R_CDSP_HUE_ROT_V                (*(volatile INT32U *)(CDSP_BASE+0x150))
#define	R_CDSP_YUV_RANGE                (*(volatile INT32U *)(CDSP_BASE+0x154))
#define	R_CDSP_INTEN                    (*(volatile INT32U *)(CDSP_BASE+0x158))
#define	R_CDSP_GATING_CLK_CTRL          (*(volatile INT32U *)(CDSP_BASE+0x15C))

#define	R_CDSP_YUV_CORING_SETTING       (*(volatile INT32U *)(CDSP_BASE+0x160))
#define	R_CDSP_INT                      (*(volatile INT32U *)(CDSP_BASE+0x164))
#define	R_CDSP_BIST_FAIL                (*(volatile INT32U *)(CDSP_BASE+0x168))
#define	R_CDSP_PROBE_CTRL               (*(volatile INT32U *)(CDSP_BASE+0x16C))

#define	R_CDSP_EXT_BANK_SIZE            (*(volatile INT32U *)(CDSP_BASE+0x170))
#define	R_CDSP_YUV_AVG_LPF_TYPE         (*(volatile INT32U *)(CDSP_BASE+0x174))
#define	R_CDSP_EXT_LINE_SIZE            (*(volatile INT32U *)(CDSP_BASE+0x178))
#define	R_CDSP_RST                      (*(volatile INT32U *)(CDSP_BASE+0x17C))

#define R_ISP_LI_HR_CTRL                (*(volatile INT32U *)(CDSP_BASE+0x180))
#define R_ISP_HR_DEPIXCANCEL_THOLD      (*(volatile INT32U *)(CDSP_BASE+0x184))
#define R_ISP_HR_DENOISE_THOLD          (*(volatile INT32U *)(CDSP_BASE+0x188))
#define R_ISP_HR_CROSSTALK_THOLD        (*(volatile INT32U *)(CDSP_BASE+0x18C))
#define R_ISP_HRR_DENOISE_CROSSTALK_WEIGHT      (*(volatile INT32U *)(CDSP_BASE+0x190))
#define R_ISP_HR_LUT_CTRL               (*(volatile INT32U *)(CDSP_BASE+0x194))
#define R_ISP_IMSIZE_CROSSTALK_WEIGHT   (*(volatile INT32U *)(CDSP_BASE+0x198))
#define R_ISP_HR_BLANKING_LINE          (*(volatile INT32U *)(CDSP_BASE+0x19C))

#define R_ISP_AWB_H_IDX					(*(volatile INT32U *) (CDSP_BASE+0x1A0))
#define R_ISP_AWB_V_IDX					(*(volatile INT32U *) (CDSP_BASE+0x1A4))
#define R_ISP_AWB_THRESHOLD_12			(*(volatile INT32U *) (CDSP_BASE+0x1A8))
#define R_ISP_AWB_THRESHOLD_34			(*(volatile INT32U *) (CDSP_BASE+0x1AC))
#define R_ISP_AWB_THRESHOLD_56			(*(volatile INT32U *) (CDSP_BASE+0x1B0))
#define R_ISP_AWB_THRESHOLD_78			(*(volatile INT32U *) (CDSP_BASE+0x1B4))
#define R_ISP_AWB_THRESHOLD_9A			(*(volatile INT32U *) (CDSP_BASE+0x1B8))
#define R_ISP_AWB_THRESHOLD_B			(*(volatile INT32U *) (CDSP_BASE+0x1BC))
#define R_ISP_AWB_THRESHOLD_CD			(*(volatile INT32U *) (CDSP_BASE+0x1C0))
#define R_ISP_AWB_THRESHOLD_12_2		(*(volatile INT32U *) (CDSP_BASE+0x1C4))
#define R_ISP_AWB_THRESHOLD_34_2		(*(volatile INT32U *) (CDSP_BASE+0x1C8))
#define R_ISP_AWB_THRESHOLD_56_2		(*(volatile INT32U *) (CDSP_BASE+0x1CC))
#define R_ISP_AWB_THRESHOLD_78_2		(*(volatile INT32U *) (CDSP_BASE+0x1D0))
#define R_ISP_AWB_THRESHOLD_9A_2		(*(volatile INT32U *) (CDSP_BASE+0x1D4))
#define R_ISP_AWB_THRESHOLD_B_2			(*(volatile INT32U *) (CDSP_BASE+0x1D8))
#define R_ISP_AWB_THRESHOLD_CD_2		(*(volatile INT32U *) (CDSP_BASE+0x1DC))

/*3DNR*/
#define R_ISP_DN3D_THR_LUM_0123			(*(volatile INT32U *)(CDSP_BASE+0x1E0))
#define R_ISP_DN3D_THR_LUM_4567			(*(volatile INT32U *)(CDSP_BASE+0x1E4))
#define R_ISP_DN3D_THR_LUM_89AB			(*(volatile INT32U *)(CDSP_BASE+0x1E8))
#define R_ISP_DN3D_THR_LUM_CDEF			(*(volatile INT32U *)(CDSP_BASE+0x1EC))
#define R_ISP_DN3D_THR_MAX_LUM_0123		(*(volatile INT32U *)(CDSP_BASE+0x1F0))
#define R_ISP_DN3D_THR_MAX_LUM_4567		(*(volatile INT32U *)(CDSP_BASE+0x1F4))
#define R_ISP_DN3D_THR_MAX_LUM_89AB		(*(volatile INT32U *)(CDSP_BASE+0x1F8))
#define R_ISP_DN3D_THR_MAX_LUM_CDEF		(*(volatile INT32U *)(CDSP_BASE+0x1FC))

#define	R_CDSP_RAW_SUBSAMPLE_SETTING    (*(volatile INT32U *)(CDSP_BASE+0x200))
#define	R_CDSP_RAW_MIRROR_SETTING       (*(volatile INT32U *)(CDSP_BASE+0x204))
#define	R_CDSP_CLAMP_SETTING            (*(volatile INT32U *)(CDSP_BASE+0x208))
#define	R_CDSP_RB_HSIZE                 (*(volatile INT32U *)(CDSP_BASE+0x20C))
#define	R_CDSP_RB_HOFFSET               (*(volatile INT32U *)(CDSP_BASE+0x210))
#define	R_CDSP_RB_VSIZE                 (*(volatile INT32U *)(CDSP_BASE+0x214))
#define	R_CDSP_RB_VOFFSET               (*(volatile INT32U *)(CDSP_BASE+0x218))
#define	R_CDSP_WDRAM_HOFFSET            (*(volatile INT32U *)(CDSP_BASE+0x21C))
#define	R_CDSP_WDRAM_VOFFSET            (*(volatile INT32U *)(CDSP_BASE+0x220))

#define	R_CDSP_LINE_INTERVAL            (*(volatile INT32U *)(CDSP_BASE+0x224))
#define	R_CDSP_DO                       (*(volatile INT32U *)(CDSP_BASE+0x228))
#define	R_CDSP_TV_MODE                  (*(volatile INT32U *)(CDSP_BASE+0x22C))
#define	R_CDSP_WSRAM_THR                (*(volatile INT32U *)(CDSP_BASE+0x230))

#define	R_CDSP_DATA_FORMAT              (*(volatile INT32U *)(CDSP_BASE+0x240))
#define	R_CDSP_GINT                     (*(volatile INT32U *)(CDSP_BASE+0x248))

#define	R_CDSP_MD_CTRL                  (*(volatile INT32U *)(CDSP_BASE+0x270))
#define	R_CDSP_MD_HSIZE                 (*(volatile INT32U *)(CDSP_BASE+0x274))
#define	R_CDSP_MD_DMA_SADDR             (*(volatile INT32U *)(CDSP_BASE+0x278))
#define R_CDSP_MD_DIFF_CNT        		(*(volatile INT32U *)(CDSP_BASE+0x27C))

//sharpen, new new denoise(nr & aj)
#define	R_CDSP_HBNR_CTRL                (*(volatile INT32U *)(CDSP_BASE+0x280))
#define	R_CDSP_HBAJ_CTRL                (*(volatile INT32U *)(CDSP_BASE+0x284))
#define	R_CDSP_HB_BLK_INTERVAL          (*(volatile INT32U *)(CDSP_BASE+0x288))
#define	R_CDSP_SP_CTRL                  (*(volatile INT32U *)(CDSP_BASE+0x28C))
#define	R_CDSP_SP_THRD                  (*(volatile INT32U *)(CDSP_BASE+0x290))
#define	R_ISP_HB_STR1                   (*(volatile INT32U *)(CDSP_BASE+0x294))
#define	R_ISP_HB_STR2                   (*(volatile INT32U *)(CDSP_BASE+0x298))

#define	R_ISP_SP_TH_REDUCE              (*(volatile INT32U *)(CDSP_BASE+0x29C))
//check Size
#define	R_CDSP_CHK_SIZE_CTRL			(*(volatile INT32U *)(CDSP_BASE+0x2A0))
#define	R_CDSP_CHK_GET_SIZE				(*(volatile INT32U *)(CDSP_BASE+0x2A4))

#define	R_CDSP_DMA_YUVABUF_SADDR        (*(volatile INT32U *)(CDSP_BASE+0x300))
#define	R_CDSP_DMA_YUVABUF_HVSIZE       (*(volatile INT32U *)(CDSP_BASE+0x304))
#define	R_CDSP_DMA_YUVBBUF_SADDR        (*(volatile INT32U *)(CDSP_BASE+0x308))
#define	R_CDSP_DMA_YUVBBUF_HVSIZE       (*(volatile INT32U *)(CDSP_BASE+0x30C))
#define	R_CDSP_DMA_RAWBUF_SADDR         (*(volatile INT32U *)(CDSP_BASE+0x310))
#define	R_CDSP_DMA_RAWBUF_HVSIZE        (*(volatile INT32U *)(CDSP_BASE+0x314))
#define	R_CDSP_DMA_RAWBUF_HOFFSET       (*(volatile INT32U *)(CDSP_BASE+0x318))
#define	R_CDSP_DMA_COFG                 (*(volatile INT32U *)(CDSP_BASE+0x324))
#define	R_CDSP_AUTO_OUT_BASE          	(volatile INT32U *)(CDSP_BASE+0x328) //0x328~0x33C
#define R_ISP_AWB_RG					(*(volatile INT32U *)(CDSP_BASE + 0x340))
#define R_ISP_AWB_BG					(*(volatile INT32U *)(CDSP_BASE + 0x344))
#define R_ISP_AWB_R						(*(volatile INT32U *)(CDSP_BASE + 0x348))
#define R_ISP_AWB_G						(*(volatile INT32U *)(CDSP_BASE + 0x34C))
#define R_ISP_AWB_B						(*(volatile INT32U *)(CDSP_BASE + 0x350))
#define R_ISP_AWB_PX					(*(volatile INT32U *)(CDSP_BASE + 0x354))
#define R_ISP_AWB_RG_LV					(*(volatile INT32U *)(CDSP_BASE + 0x358))
#define R_ISP_AWB_BG_LV					(*(volatile INT32U *)(CDSP_BASE + 0x35C))
#define R_ISP_AWB_PX_LV					(*(volatile INT32U *)(CDSP_BASE + 0x360))
#define R_ISP_AWB_RG_LV_2				(*(volatile INT32U *)(CDSP_BASE + 0x364))
#define R_ISP_AWB_BG_LV_2				(*(volatile INT32U *)(CDSP_BASE + 0x368))
#define R_ISP_AWB_PX_LV_2				(*(volatile INT32U *)(CDSP_BASE + 0x36C))

//NLM
#define R_ISP_NLM_G0_TH0_TH1_WT0		(*(volatile INT32U *) (CDSP_BASE + 0x370))
#define R_ISP_NLM_G0_TH2_TH3_WT1		(*(volatile INT32U *) (CDSP_BASE + 0x374))
#define R_ISP_NLM_G0_TH4_TH5_WT2		(*(volatile INT32U *) (CDSP_BASE + 0x378))
#define R_ISP_NLM_G1_TH0_TH1_WT0		(*(volatile INT32U *) (CDSP_BASE + 0x37c))
#define R_ISP_NLM_G1_TH2_TH3_WT1		(*(volatile INT32U *) (CDSP_BASE + 0x380))
#define R_ISP_NLM_G1_TH4_TH5_WT2		(*(volatile INT32U *) (CDSP_BASE + 0x384))
#define R_ISP_NLM_G2_TH0_TH1_WT0		(*(volatile INT32U *) (CDSP_BASE + 0x388))
#define R_ISP_NLM_G2_TH2_TH3_WT1		(*(volatile INT32U *) (CDSP_BASE + 0x38c))
#define R_ISP_NLM_G2_TH4_TH5_WT2		(*(volatile INT32U *) (CDSP_BASE + 0x390))
#define R_ISP_NLM_G3_TH0_TH1_WT0		(*(volatile INT32U *) (CDSP_BASE + 0x394))
#define R_ISP_NLM_G3_TH2_TH3_WT1		(*(volatile INT32U *) (CDSP_BASE + 0x398))
#define R_ISP_NLM_G3_TH4_TH5_WT2		(*(volatile INT32U *) (CDSP_BASE + 0x39c))
#define R_ISP_NLM_G4_TH0_TH1_WT0		(*(volatile INT32U *) (CDSP_BASE + 0x3a0))
#define R_ISP_NLM_G4_TH2_TH3_WT1		(*(volatile INT32U *) (CDSP_BASE + 0x3a4))
#define R_ISP_NLM_G4_TH4_TH5_WT2		(*(volatile INT32U *) (CDSP_BASE + 0x3a8))
#define R_ISP_NLM_G5_TH0_TH1_WT0		(*(volatile INT32U *) (CDSP_BASE + 0x3ac))
#define R_ISP_NLM_G5_TH2_TH3_WT1		(*(volatile INT32U *) (CDSP_BASE + 0x3b0))
#define R_ISP_NLM_G5_TH4_TH5_WT2		(*(volatile INT32U *) (CDSP_BASE + 0x3b4))
#define R_ISP_NLM_G6_TH0_TH1_WT0		(*(volatile INT32U *) (CDSP_BASE + 0x3b8))
#define R_ISP_NLM_G6_TH2_TH3_WT1		(*(volatile INT32U *) (CDSP_BASE + 0x3bc))
#define R_ISP_NLM_G6_TH4_TH5_WT2		(*(volatile INT32U *) (CDSP_BASE + 0x3c0))
#define R_ISP_NLM_G0_WT3_WT4_WT5		(*(volatile INT32U *) (CDSP_BASE + 0x3c4))
#define R_ISP_NLM_G1_WT3_WT4_WT5		(*(volatile INT32U *) (CDSP_BASE + 0x3c8))
#define R_ISP_NLM_G2_WT3_WT4_WT5		(*(volatile INT32U *) (CDSP_BASE + 0x3cc))
#define R_ISP_NLM_G3_WT3_WT4_WT5		(*(volatile INT32U *) (CDSP_BASE + 0x3d0))
#define R_ISP_NLM_G4_WT3_WT4_WT5		(*(volatile INT32U *) (CDSP_BASE + 0x3d4))
#define R_ISP_NLM_G5_WT3_WT4_WT5		(*(volatile INT32U *) (CDSP_BASE + 0x3d8))
#define R_ISP_NLM_G6_WT3_WT4_WT5		(*(volatile INT32U *) (CDSP_BASE + 0x3dc))
#define R_ISP_NLM_GTH_NLMEN     		(*(volatile INT32U *) (CDSP_BASE + 0x3e0))
#define R_ISP_NLM_IMAGE_SIZE     		(*(volatile INT32U *) (CDSP_BASE + 0x3e4))

#define	R_CDSP_AWB_WIN_RGGAIN2          (*(volatile INT32U *)(CDSP_BASE+0x400))
#define	R_CDSP_AWB_WIN_BGAIN2           (*(volatile INT32U *)(CDSP_BASE+0x404))
#define	R_CDSP_AE_AWB_WIN_CTRL          (*(volatile INT32U *)(CDSP_BASE+0x408))
#define	R_CDSP_AE_WIN_SIZE              (*(volatile INT32U *)(CDSP_BASE+0x40C))

/* AE WIN*/
#define	R_CDSP_RGB_WINH_CTRL            (*(volatile INT32U *)(CDSP_BASE+0x410))
#define	R_CDSP_RGB_WINV_CTRL            (*(volatile INT32U *)(CDSP_BASE+0x414))
#define	R_CDSP_AE_WIN_ABUFADDR          (*(volatile INT32U *)(CDSP_BASE+0x418))
#define	R_CDSP_AE_WIN_BBUFADDR          (*(volatile INT32U *)(CDSP_BASE+0x41C))

#define	R_CDSP_HISTGM_CTRL              (*(volatile INT32U *)(CDSP_BASE+0x420))
#define	R_CDSP_HISTGM_WINSTART			(*(volatile INT32U *)(CDSP_BASE+0x424))
#define	R_CDSP_HISTGM_WINEND			(*(volatile INT32U *)(CDSP_BASE+0x428))

/* AE, AWB WIN*/
#define	R_ISP_AE_RGB_GAIN   			(*(volatile INT32U *)(CDSP_BASE + 0x42C))
#define	R_ISP_AWB_Y_THRHOLD				(*(volatile INT32U *)(CDSP_BASE + 0x430))

#define	R_ISP_AE_R_DIV_G_TH123 			(*(volatile INT32U *)(CDSP_BASE + 0x434))
#define	R_ISP_AE_R_DIV_G_TH456 			(*(volatile INT32U *)(CDSP_BASE + 0x438))
#define	R_ISP_AE_R_DIV_G_TH789 			(*(volatile INT32U *)(CDSP_BASE + 0x43C))
#define	R_ISP_AE_R_DIV_G_TH101112		(*(volatile INT32U *)(CDSP_BASE + 0x440))
#define	R_ISP_AE_R_DIV_G_TH131415		(*(volatile INT32U *)(CDSP_BASE + 0x444))
#define	R_ISP_AE_R_DIV_G_TH16		    (*(volatile INT32U *)(CDSP_BASE + 0x448))
#define	R_ISP_AE_B_DIV_G_TH12   		(*(volatile INT32U *)(CDSP_BASE + 0x448))
#define	R_ISP_AE_B_DIV_G_TH345   		(*(volatile INT32U *)(CDSP_BASE + 0x44C))
#define	R_ISP_AE_B_DIV_G_TH678   		(*(volatile INT32U *)(CDSP_BASE + 0x450))
#define	R_ISP_AE_B_DIV_G_TH91011   		(*(volatile INT32U *)(CDSP_BASE + 0x454))
#define	R_ISP_AE_B_DIV_G_TH121314  		(*(volatile INT32U *)(CDSP_BASE + 0x458))
#define	R_ISP_AE_B_DIV_G_TH1516   		(*(volatile INT32U *)(CDSP_BASE + 0x45C))

#define	R_ISP_RGB_AWB_WIN_H_SIZE		(*(volatile INT32U *)(CDSP_BASE + 0x460))
#define	R_ISP_RGB_AWB_WIN_V_SIZE		(*(volatile INT32U *)(CDSP_BASE + 0x464))
//Rang1
#define	R_ISP_AWBWIN_RANG1_SUMCNT		(*(volatile INT32U *)(CDSP_BASE + 0x468))
#define	R_ISP_AWBWIN_RANG1_SUMR 		(*(volatile INT32U *)(CDSP_BASE + 0x46C))
#define	R_ISP_AWBWIN_RANG1_SUMG 		(*(volatile INT32U *)(CDSP_BASE + 0x470))
#define	R_ISP_AWBWIN_RANG1_SUMB 		(*(volatile INT32U *)(CDSP_BASE + 0x474))
#define	R_ISP_AWBWIN_RG1_RDIVG_SUM		(*(volatile INT32U *)(CDSP_BASE + 0x478))
#define	R_ISP_AWBWIN_RG1_BDIVG_SUM 		(*(volatile INT32U *)(CDSP_BASE + 0x47C))
//Rang2
#define	R_ISP_AWBWIN_RANG2_SUMCNT		(*(volatile INT32U *)(CDSP_BASE + 0x480))
#define	R_ISP_AWBWIN_RANG2_SUMR 		(*(volatile INT32U *)(CDSP_BASE + 0x484))
#define	R_ISP_AWBWIN_RANG2_SUMG 		(*(volatile INT32U *)(CDSP_BASE + 0x488))
#define	R_ISP_AWBWIN_RANG2_SUMB 		(*(volatile INT32U *)(CDSP_BASE + 0x48C))
#define	R_ISP_AWBWIN_RG2_RDIVG_SUM		(*(volatile INT32U *)(CDSP_BASE + 0x490))
#define	R_ISP_AWBWIN_RG2_BDIVG_SUM 		(*(volatile INT32U *)(CDSP_BASE + 0x494))
//Rang3
#define	R_ISP_AWBWIN_RANG3_SUMCNT		(*(volatile INT32U *)(CDSP_BASE + 0x498))
#define	R_ISP_AWBWIN_RANG3_SUMR 		(*(volatile INT32U *)(CDSP_BASE + 0x49C))
#define	R_ISP_AWBWIN_RANG3_SUMG 		(*(volatile INT32U *)(CDSP_BASE + 0x4A0))
#define	R_ISP_AWBWIN_RANG3_SUMB 		(*(volatile INT32U *)(CDSP_BASE + 0x4A4))
#define	R_ISP_AWBWIN_RG3_RDIVG_SUM		(*(volatile INT32U *)(CDSP_BASE + 0x4A8))
#define	R_ISP_AWBWIN_RG3_BDIVG_SUM 		(*(volatile INT32U *)(CDSP_BASE + 0x4AC))
//Rang4
#define	R_ISP_AWBWIN_RANG4_SUMCNT		(*(volatile INT32U *)(CDSP_BASE + 0x4B0))
#define	R_ISP_AWBWIN_RANG4_SUMR 		(*(volatile INT32U *)(CDSP_BASE + 0x4B4))
#define	R_ISP_AWBWIN_RANG4_SUMG 		(*(volatile INT32U *)(CDSP_BASE + 0x4B8))
#define	R_ISP_AWBWIN_RANG4_SUMB 		(*(volatile INT32U *)(CDSP_BASE + 0x4BC))
#define	R_ISP_AWBWIN_RG4_RDIVG_SUM		(*(volatile INT32U *)(CDSP_BASE + 0x4C0))
#define	R_ISP_AWBWIN_RG4_BDIVG_SUM 		(*(volatile INT32U *)(CDSP_BASE + 0x4C4))

//AF
#define	R_ISP_AF_HORIZONTAL_VALUE_I		(*(volatile INT32U *)(CDSP_BASE + 0x4C8))
#define	R_ISP_AF_VRTICAL_VALUE_I 		(*(volatile INT32U *)(CDSP_BASE + 0x4CC))
#define	R_ISP_AF_HORIZONTAL_VALUE_II	(*(volatile INT32U *)(CDSP_BASE + 0x4D0))
#define	R_ISP_AF_VRTICAL_VALUE_II 		(*(volatile INT32U *)(CDSP_BASE + 0x4D4))
#define	R_ISP_AF_HORIZONTAL_VALUE_III	(*(volatile INT32U *)(CDSP_BASE + 0x4D8))
#define	R_ISP_AF_VRTICAL_VALUE_III 		(*(volatile INT32U *)(CDSP_BASE + 0x4DC))
#define	R_ISP_AF_UPPER_VALUE			(*(volatile INT32U *)(CDSP_BASE + 0x4E0))
#define	R_ISP_AF_WIN_I_OFFSET		 	(*(volatile INT32U *)(CDSP_BASE + 0x4E4))
#define	R_ISP_AF_WIN_I_HVSIZE		 	(*(volatile INT32U *)(CDSP_BASE + 0x4E8))
#define	R_ISP_AF_MISC_CTRL				(*(volatile INT32U *)(CDSP_BASE + 0x4EC))
#define	R_ISP_AF_WIN_II_OFFSET			(*(volatile INT32U *)(CDSP_BASE + 0x4F0))
#define	R_ISP_AF_WIN_III_OFFSET 		(*(volatile INT32U *)(CDSP_BASE + 0x4F4))

#define	R_CDSP_AEF_WIN_TEST             (*(volatile INT32U *)(CDSP_BASE	+ 0x4FC))

//AF Gamma
#define R_AF_GM_A0A1 					(*(volatile INT32U *)(CDSP_BASE + 0x500))
#define R_AF_GM_A2A3 					(*(volatile INT32U *)(CDSP_BASE + 0x504))
#define R_AF_GM_A4B0 					(*(volatile INT32U *)(CDSP_BASE + 0x508))
#define R_AF_GM_B1B2 					(*(volatile INT32U *)(CDSP_BASE + 0x50C))
#define R_AF_GM_B3B4 					(*(volatile INT32U *)(CDSP_BASE + 0x510))
#define R_AF_GM_TH0TH1 					(*(volatile INT32U *)(CDSP_BASE + 0x514))
#define R_AF_GM_TH2TH3 					(*(volatile INT32U *)(CDSP_BASE + 0x518))

//WDR control
#define	R_CDSP_LUTRGB_EN                (*(volatile INT32U *)(CDSP_BASE+0x500))
#define	R_CDSP_WDRLUT_CTRL              (*(volatile INT32U *)(CDSP_BASE+0x540))
#define	R_CDSP_WDR_CTRL                 (*(volatile INT32U *)(CDSP_BASE+0x544))
#define	R_CDSP_WDR_PARA                 (*(volatile INT32U *)(CDSP_BASE+0x548))
#define	R_CDSP_WDR_LUT_TBL              (*(volatile INT32U *)(CDSP_BASE+0x54C))
#define R_CDSP_WDR_LHCNT                (*(volatile INT32U *)(CDSP_BASE+0x550))

// front
#define	R_CDSP_FRONT_CTRL0              (*(volatile INT32U *)(CDSP_BASE+0x600))
#define	R_CDSP_FLASH_WIDTH              (*(volatile INT32U *)(CDSP_BASE+0x604))
#define	R_CDSP_FLASH_TRIG_NUM           (*(volatile INT32U *)(CDSP_BASE+0x608))
#define	R_CDSP_SNAP_CTRL                (*(volatile INT32U *)(CDSP_BASE+0x60C))

#define	R_CDSP_FRONT_CTRL1              (*(volatile INT32U *)(CDSP_BASE+0x610))
#define	R_CDSP_HSYNC_FREDGE             (*(volatile INT32U *)(CDSP_BASE+0x614))
#define	R_CDSP_VSYNC_FREDGE             (*(volatile INT32U *)(CDSP_BASE+0x618))
#define	R_CDSP_FRONT_CTRL2              (*(volatile INT32U *)(CDSP_BASE+0x61C))

#define	R_CDSP_FRAME_H_SETTING          (*(volatile INT32U *)(CDSP_BASE+0x620))
#define	R_CDSP_FRAME_V_SETTING          (*(volatile INT32U *)(CDSP_BASE+0x624))
#define	R_CDSP_TG_LINE_CTRL             (*(volatile INT32U *)(CDSP_BASE+0x628))
#define	R_CDSP_FRAME_CTRL               (*(volatile INT32U *)(CDSP_BASE+0x62C))

#define	R_CDSP_EXTH_SEPA                (*(volatile INT32U *)(CDSP_BASE+0x630))
#define	R_CDSP_TG_LS_LINE_NUM           (*(volatile INT32U *)(CDSP_BASE+0x634))
#define	R_CDSP_FRONT_CTRL3              (*(volatile INT32U *)(CDSP_BASE+0x638))
#define	R_CDSP_TG_ZERO                  (*(volatile INT32U *)(CDSP_BASE+0x63C))

#define	R_CDSP_TG_GPIO_SEL_OEN          (*(volatile INT32U *)(CDSP_BASE+0x640))
#define	R_CDSP_TG_GPIO_OUT_IN           (*(volatile INT32U *)(CDSP_BASE+0x644))
#define	R_CDSP_TG_GPIO_REVENT           (*(volatile INT32U *)(CDSP_BASE+0x648))
#define	R_CDSP_TG_GPIO_FEVENT           (*(volatile INT32U *)(CDSP_BASE+0x64C))

#define	R_CDSP_MIPI_CTRL                (*(volatile INT32U *)(CDSP_BASE+0x650))
#define	R_CDSP_MIPI_HVOFFSET            (*(volatile INT32U *)(CDSP_BASE+0x654))
#define	R_CDSP_MIPI_HVSIZE              (*(volatile INT32U *)(CDSP_BASE+0x658))
#define	R_CDSP_FRONT_CTRL4              (*(volatile INT32U *)(CDSP_BASE+0x65C))

#define	R_CDSP_SONY_SEN_DATA            (*(volatile INT32U *)(CDSP_BASE+0x660))
#define	R_CDSP_FRONT_GCLK               (*(volatile INT32U *)(CDSP_BASE+0x664))
#define	R_CDSP_SEN_CTRL_SIG             (*(volatile INT32U *)(CDSP_BASE+0x668))
#define	R_CDSP_FRONT_INT                (*(volatile INT32U *)(CDSP_BASE+0x66C))

#define	R_CDSP_VD_RFOCC_INT             (*(volatile INT32U *)(CDSP_BASE+0x670))
#define	R_CDSP_INTH_NUM                 (*(volatile INT32U *)(CDSP_BASE+0x674))
#define	R_CDSP_FRONT_INTEN              (*(volatile INT32U *)(CDSP_BASE+0x678))
#define	R_CDSP_FRONT_VDRF_INT           (*(volatile INT32U *)(CDSP_BASE+0x67C))

#define	R_CDSP_FRONT_VDRF_INTEN         (*(volatile INT32U *)(CDSP_BASE+0x680))
#define	R_CDSP_SIG_GEN                  (*(volatile INT32U *)(CDSP_BASE+0x684))
#define	R_CDSP_FRONT_PROBE_CTRL         (*(volatile INT32U *)(CDSP_BASE+0x688))
#define	R_CDSP_FRONT_DUMMY              (*(volatile INT32U *)(CDSP_BASE+0x68C))

#define	R_CDSP_FPICNT                   (*(volatile INT32U *)(CDSP_BASE+0x690))
#define	R_CDSP_EXTRGB                   (*(volatile INT32U *)(CDSP_BASE+0x694))
#define	R_CDSP_MACRO_RW                 (*(volatile INT32U *)(CDSP_BASE+0x800))
#define	R_CDSP_MACRO_SDRAM_RW           0xD0800800//(*(volatile INT32U *)(CDSP_BASE+0x800))

/******************************************************************************
 * FFT: 0xD1600000
 ******************************************************************************/
#define FFT_BASE			0xD1600000

#define R_FFT_START                     (*(volatile INT32U *)(FFT_BASE + 0x00))
#define R_FFT_CTRL                      (*(volatile INT32U *)(FFT_BASE + 0x04))
#define R_FFT_SCALE                     (*(volatile INT32U *)(FFT_BASE + 0x08))
#define R_FFT_RADDR                     (*(volatile INT32U *)(FFT_BASE + 0x0c))
#define R_FFT_WADDR                     (*(volatile INT32U *)(FFT_BASE + 0x10))
#define R_FFT_INTSTS                    (*(volatile INT32U *)(FFT_BASE + 0x14))
#define R_FFT_OVFSTS                    (*(volatile INT32U *)(FFT_BASE + 0x18))

/******************************************************************************
 * Rotate: 0xC0240000
 ******************************************************************************/
#define ROTATOR0_BASE				     0xC0240000
#define ROTATOR1_BASE				     0xC0180000
#define ROTATOR2_BASE                    0xC01A0000
#define R_ROTATOR_BUF_I_ADDR             (*((volatile INT32U *) 0xC0240000))
#define R_ROTATOR_BUF_O_ADDR             (*((volatile INT32U *) 0xC0240004))
#define R_ROTATOR_IMG_WIDTH              (*((volatile INT32U *) 0xC0240008))
#define R_ROTATOR_IMG_HEIGHT             (*((volatile INT32U *) 0xC024000C))
#define R_ROTATOR_CTRL                   (*((volatile INT32U *) 0xC0240010))
#define R_ROTATOR_START                  (*((volatile INT32U *) 0xC0240014))
#define R_ROTATOR_IRQ_STATUS             (*((volatile INT32U *) 0xC0240018))
#define R_ROTATOR_IRQ_EN                 (*((volatile INT32U *) 0xC024001C))

/******************************************************************************
 * CEC: 0xC01E0000
 ******************************************************************************/
#define CEC_BASE                       0xC01E0000

#define R_CEC_CTRL                      (*(volatile INT32U *)(CEC_BASE+0x000))
#define R_CEC_CFGR                      (*(volatile INT32U *)(CEC_BASE+0x004))
#define R_CEC_TX_DATA                   (*(volatile INT32U *)(CEC_BASE+0x008))
#define R_CEC_RX_DATA                   (*(volatile INT32U *)(CEC_BASE+0x00C))
#define R_CEC_ISR                       (*(volatile INT32U *)(CEC_BASE+0x010))
#define R_CEC_IER                       (*(volatile INT32U *)(CEC_BASE+0x014))

/******************************************************************************
 * BOX Filter: 0xD1A00000
 ******************************************************************************/
#define BOXFILTER_BASE                       0xD1A00000

#define R_BOXFILTER_BUF_I_ADDR          (*(volatile INT32U *)(BOXFILTER_BASE+0x000))
#define R_BOXFILTER_BUFA_O_ADDR         (*(volatile INT32U *)(BOXFILTER_BASE+0x004))
#define R_BOXFILTER_BUFB_O_ADDR         (*(volatile INT32U *)(BOXFILTER_BASE+0x008))
#define R_BOXFILTER_IMG_WIDTH           (*(volatile INT32U *)(BOXFILTER_BASE+0x00C))
#define R_BOXFILTER_IMG_HEIGHT          (*(volatile INT32U *)(BOXFILTER_BASE+0x010))
#define R_BOXFILTER_COEF1               (*(volatile INT32U *)(BOXFILTER_BASE+0x014))
#define R_BOXFILTER_COEF2               (*(volatile INT32U *)(BOXFILTER_BASE+0x018))
#define R_BOXFILTER_COEF3               (*(volatile INT32U *)(BOXFILTER_BASE+0x01C))
#define R_BOXFILTER_CTRL                (*(volatile INT32U *)(BOXFILTER_BASE+0x020))
#define R_BOXFILTER_START               (*(volatile INT32U *)(BOXFILTER_BASE+0x024))
#define R_BOXFILTER_IRQ_STATUS          (*(volatile INT32U *)(BOXFILTER_BASE+0x028))
#define R_BOXFILTER_IRQ_EN              (*(volatile INT32U *)(BOXFILTER_BASE+0x02C))


/******************************************************************************
 * LBP : 0xD1100000
 ******************************************************************************/
#define LBP_BASE                        0xD1800000

#define R_LBP_CTRL                      (*(volatile INT32U *)(LBP_BASE+0x000))
#define R_LBP_STAUS                     (*(volatile INT32U *)(LBP_BASE+0x004))
#define R_LBP_IMGS_STR_ADDR             (*(volatile INT32U *)(LBP_BASE+0x008))
#define R_LBP_IMGS_SIZE                 (*(volatile INT32U *)(LBP_BASE+0x00C))
#define R_LBPGEN_OB_ADDR                (*(volatile INT32U *)(LBP_BASE+0x010))
#define R_LBPCMP_OB_ADDR                (*(volatile INT32U *)(LBP_BASE+0x014))
#define R_LBP_OCODE_ADDR                (*(volatile INT32U *)(LBP_BASE+0x018))
#define R_LBP_NCODE_ADDR                (*(volatile INT32U *)(LBP_BASE+0x01C))
#define R_LBP_IMG_SIZE                  (*(volatile INT32U *)(LBP_BASE+0x020))
#define R_LBP_CMPBLK_SIZE               (*(volatile INT32U *)(LBP_BASE+0x024))
#define R_LBP_TOTAL_SUM                 (*(volatile INT32U *)(LBP_BASE+0x028))

/******************************************************************************
 * IPU : 0xD1B00000
 ******************************************************************************/
#define IPU_BASE                        0xD1F00000

#define R_IPU_CTRL0                     (*(volatile INT32U *)(IPU_BASE+0x000))
#define R_IPU_CTRL1                     (*(volatile INT32U *)(IPU_BASE+0x004))
#define R_IPU_INT_EN                    (*(volatile INT32U *)(IPU_BASE+0x008))
#define R_IPU_INT_FLAG                  (*(volatile INT32U *)(IPU_BASE+0x00C))
#define R_IPU_X_WIDTH                   (*(volatile INT32U *)(IPU_BASE+0x010))
#define R_IPU_Y_WIDTH                   (*(volatile INT32U *)(IPU_BASE+0x014))
#define R_IPU_RESERVED                  (*(volatile INT32U *)(IPU_BASE+0x018))
#define R_IPU_X_PITCH                   (*(volatile INT32U *)(IPU_BASE+0x01C))
#define R_IPU_IN_X_OFFSET               (*(volatile INT32U *)(IPU_BASE+0x020))
#define R_IPU_OUT_X_OFFSET              (*(volatile INT32U *)(IPU_BASE+0x024))
#define R_IPU_OP_X_OFFSET               (*(volatile INT32U *)(IPU_BASE+0x028))
#define R_IPU_ADR_TAB_ADDR              (*(volatile INT32U *)(IPU_BASE+0x030))
#define R_IPU_OUT_ADDR                  (*(volatile INT32U *)(IPU_BASE+0x034))
#define R_IPU_COL_TAB_ADDR              (*(volatile INT32U *)(IPU_BASE+0x038))
#define R_IPU_IN_SRC_ADDR               (*(volatile INT32U *)(IPU_BASE+0x03C))
#define R_IPU_IN_OP_ADDR                (*(volatile INT32U *)(IPU_BASE+0x040))
#define R_IPU_OP_SHIFT                  (*(volatile INT32U *)(IPU_BASE+0x044))
#define R_IPU_IN_OP_VALUE               (*(volatile INT32U *)(IPU_BASE+0x048))
#define R_IPU_IN_OP_THRD                (*(volatile INT32U *)(IPU_BASE+0x04C))
#define R_IPU_ADR_BP_COL                (*(volatile INT32U *)(IPU_BASE+0x050))
#define R_IPU_FIX_ALPHA                 (*(volatile INT32U *)(IPU_BASE+0x054))
#define R_IPU_FILL_COL                  (*(volatile INT32U *)(IPU_BASE+0x058))
#define R_IPU_FILL_TP_COL               (*(volatile INT32U *)(IPU_BASE+0x05C))
#define R_IPU_AWIN_CTRL0                (*(volatile INT32U *)(IPU_BASE+0x060))
#define R_IPU_AWIN_CTRL1                (*(volatile INT32U *)(IPU_BASE+0x064))
#define R_IPU_AWIN_CTRL2                (*(volatile INT32U *)(IPU_BASE+0x068))
#define R_IPU_CWIN_CTRL0                (*(volatile INT32U *)(IPU_BASE+0x070))
#define R_IPU_CWIN_CTRL1                (*(volatile INT32U *)(IPU_BASE+0x074))
#define R_IPU_COL_TAB_ADDR1             (*(volatile INT32U *)(IPU_BASE+0x080))
#define R_IPU_COL_ADDR_MAP              (*(volatile INT32U *)(IPU_BASE+0x084))


/******************************************************************************
 * BINMODE_A:  0xC0280000
 * BINMODE_B:  0xC01C0000
 * BINMODE_B1: 0xC0290000
 * BINMODE_B2: 0xC02A0000
 * BINMODE_B3: 0xC02B0000
 ******************************************************************************/
#define BINMODE_A_BASE						0xC0280000
#define BINMODE_B_BASE						0xC01C0000
#define BINMODE_B1_BASE						0xC0290000
#define BINMODE_B2_BASE						0xC02A0000
#define BINMODE_B3_BASE						0xC02B0000

#define P_BINMODE_A_BASE	                ((volatile INT32U *) (BINMODE_A_BASE + 0x00))
#define P_BINMODE_B_BASE	                ((volatile INT32U *) (BINMODE_B_BASE + 0x00))
#define P_BINMODE_B1_BASE	                ((volatile INT32U *) (BINMODE_B1_BASE + 0x00))
#define P_BINMODE_B2_BASE	                ((volatile INT32U *) (BINMODE_B2_BASE + 0x00))
#define P_BINMODE_B3_BASE	                ((volatile INT32U *) (BINMODE_B3_BASE + 0x00))

/******************************************************************************
 * Ethernet MAC: 0xD1D00000
 ******************************************************************************/
#define ETHC_BASE 0xD1D00000

#define R_ETH_MAC_MCR1    (*(volatile INT32U *)(ETHC_BASE + 0x00 ))// MAC configuration register #1
#define R_ETH_MAC_MCR2    (*(volatile INT32U *)(ETHC_BASE + 0x04 ))// MAC configuration register #2
#define R_ETH_MAC_IPGR    (*(volatile INT32U *)(ETHC_BASE + 0x08 ))// Back-to-Back Inter-Packet-Gap register
#define R_ETH_MAC_NIPGR   (*(volatile INT32U *)(ETHC_BASE + 0x0c ))// Non-Back-to-Back Inter-Packet-Gap register
#define R_ETH_MAC_CWR     (*(volatile INT32U *)(ETHC_BASE + 0x10 ))// Collision Window / Retry register
#define R_ETH_MAC_MFR     (*(volatile INT32U *)(ETHC_BASE + 0x14 ))// Maximum Frame register
#define R_ETH_MAC_PSR     (*(volatile INT32U *)(ETHC_BASE + 0x18 ))// PHY Support register (SMII / RMII / PMD / ENDEC)
#define R_ETH_MAC_TR      (*(volatile INT32U *)(ETHC_BASE + 0x1c ))// Test register
#define R_ETH_MAC_MCFGR   (*(volatile INT32U *)(ETHC_BASE + 0x20 ))// MII Mgmt Configuration
#define R_ETH_MAC_MCMDR   (*(volatile INT32U *)(ETHC_BASE + 0x24 ))// MII Mgmt Command
#define R_ETH_MAC_MADRR   (*(volatile INT32U *)(ETHC_BASE + 0x28 ))// MII Mgmt Address
#define R_ETH_MAC_MWTDR   (*(volatile INT32U *)(ETHC_BASE + 0x2c ))// MII Mgmt Write Data
#define R_ETH_MAC_MRDDR   (*(volatile INT32U *)(ETHC_BASE + 0x30 ))// MII Mgmt Read Data
#define R_ETH_MAC_MINDR   (*(volatile INT32U *)(ETHC_BASE + 0x34 ))// MII Mgmt Indicators
#define R_ETH_MAC_SA0     (*(volatile INT32U *)(ETHC_BASE + 0x40 ))// Station Address
#define R_ETH_MAC_SA1     (*(volatile INT32U *)(ETHC_BASE + 0x44 ))// Station Address
#define R_ETH_MAC_SA2     (*(volatile INT32U *)(ETHC_BASE + 0x48 ))// Station Address
#define R_ETH_DMA_TCR     (*(volatile INT32U *)(ETHC_BASE + 0x180))// Transmit control
#define R_ETH_DMA_TDR     (*(volatile INT32U *)(ETHC_BASE + 0x184))// Pointer to Transmit Descriptor
#define R_ETH_DMA_TSR     (*(volatile INT32U *)(ETHC_BASE + 0x188))// Transmit Status
#define R_ETH_DMA_RCR     (*(volatile INT32U *)(ETHC_BASE + 0x18c))// Receive Control
#define R_ETH_DMA_RDR     (*(volatile INT32U *)(ETHC_BASE + 0x190))// Pointer to Receive Descriptor
#define R_ETH_DMA_RSR     (*(volatile INT32U *)(ETHC_BASE + 0x194))// Receive Status
#define R_ETH_DMA_IMR     (*(volatile INT32U *)(ETHC_BASE + 0x198))// Interrupt Mask
#define R_ETH_DMA_IR      (*(volatile INT32U *)(ETHC_BASE + 0x19c))// Interrupts
#define R_ETH_FIFO_CR0    (*(volatile INT32U *)(ETHC_BASE + 0x3c ))// Configuration Register 0
#define R_ETH_FIFO_CR1    (*(volatile INT32U *)(ETHC_BASE + 0x4c ))// Configuration Register 1
#define R_ETH_FIFO_CR2    (*(volatile INT32U *)(ETHC_BASE + 0x50 ))// Configuration Register 2
#define R_ETH_FIFO_CR3    (*(volatile INT32U *)(ETHC_BASE + 0x54 ))// Configuration Register 3
#define R_ETH_FIFO_CR4    (*(volatile INT32U *)(ETHC_BASE + 0x58 ))// Configuration Register 4
#define R_ETH_FIFO_CR5    (*(volatile INT32U *)(ETHC_BASE + 0x5c ))// Configuration Register 5
#define R_ETH_FIFO_RAR0   (*(volatile INT32U *)(ETHC_BASE + 0x60 ))// RAM Access Register 0
#define R_ETH_FIFO_RAR1   (*(volatile INT32U *)(ETHC_BASE + 0x64 ))// RAM Access Register 1
#define R_ETH_FIFO_RAR2   (*(volatile INT32U *)(ETHC_BASE + 0x68 ))// RAM Access Register 2
#define R_ETH_FIFO_RAR3   (*(volatile INT32U *)(ETHC_BASE + 0x6c ))// RAM Access Register 3
#define R_ETH_FIFO_RAR4   (*(volatile INT32U *)(ETHC_BASE + 0x70 ))// RAM Access Register 4
#define R_ETH_FIFO_RAR5   (*(volatile INT32U *)(ETHC_BASE + 0x74 ))// RAM Access Register 5
#define R_ETH_FIFO_RAR6   (*(volatile INT32U *)(ETHC_BASE + 0x78 ))// RAM Access Register 6
#define R_ETH_FIFO_RAR7   (*(volatile INT32U *)(ETHC_BASE + 0x7c ))// RAM Access Register 7

/******************************************************************************
 * GP420 converter
 ******************************************************************************/
#define R_GP420_START_ADDRL		(*((volatile INT32U *) 0xD0000000))
#define R_GP420_START_ADDRH     (*((volatile INT32U *) 0xD0000004))
#define R_GP420_TARGET_ADDRL    (*((volatile INT32U *) 0xD0000138))
#define R_GP420_TARGET_ADDRH    (*((volatile INT32U *) 0xD000013C))
#define R_GP420_WIDTH     		(*((volatile INT32U *) 0xD00000C0))

/******************************************************************************
 * DDR SDRAM Controller
******************************************************************************/
#define DDRSRAM_BASE 0xD0200300
#define R_DDR_OPMODE    (*(volatile INT32U *)(DDRSRAM_BASE + 0x00 )) //DDR Controller Operation Mode
#define R_DDR_SDCMD     (*(volatile INT32U *)(DDRSRAM_BASE + 0x04 )) //DDR Command
#define R_DDR_SDREF     (*(volatile INT32U *)(DDRSRAM_BASE + 0x08 )) //DDR Controller Refresh
#define R_DDR_SDSIZE    (*(volatile INT32U *)(DDRSRAM_BASE + 0x10 )) //DDR SDRAM Size
#define R_DDR_SDMODE    (*(volatile INT32U *)(DDRSRAM_BASE + 0x18 )) //SDRAM Mode
#define R_DDR_MR0       (*(volatile INT32U *)(DDRSRAM_BASE + 0x18 )) //for (LP)DDR2/3 reference to DDR SPEC
#define R_DDR_SDEMODE   (*(volatile INT32U *)(DDRSRAM_BASE + 0x1C )) //SDRAM Extended Mode
#define R_DDR_MR1       (*(volatile INT32U *)(DDRSRAM_BASE + 0x1C )) //for (LP)DDR2/3 reference to DDR SPEC
#define R_DDR_MR2       (*(volatile INT32U *)(DDRSRAM_BASE + 0x20 )) //for (LP)DDR2/3 reference to DDR SPEC
#define R_DDR_MR3       (*(volatile INT32U *)(DDRSRAM_BASE + 0x24 )) //for (LP)DDR2/3 reference to DDR SPEC
#define R_DDR_SDTIME0   (*(volatile INT32U *)(DDRSRAM_BASE + 0x28 )) //DDR SDRAM Timing Control 0
#define R_DDR_SDTIME1   (*(volatile INT32U *)(DDRSRAM_BASE + 0x2C )) //DDR SDRAM Timing Control 1
#define R_DDR_DQSDS     (*(volatile INT32U *)(DDRSRAM_BASE + 0x34 )) //DQS Delay SKEW
#define R_DDR_PAD_RET   (*(volatile INT32U *)(DDRSRAM_BASE + 0x40 )) //PAD retetion
#define R_DDR_STATE     (*(volatile INT32U *)(DDRSRAM_BASE + 0x70 )) //DDR State
#define R_DDR_ACCTS     (*(volatile INT32U *)(DDRSRAM_BASE + 0x80 )) //Auto Calibration Control
#define R_DDR_DQDLY0    (*(volatile INT32U *)(DDRSRAM_BASE + 0x84 )) //Calibration DQ Input Delay 0
#define R_DDR_DQDLY1    (*(volatile INT32U *)(DDRSRAM_BASE + 0x88 )) //Calibration DQ Input Delay 1
#define R_DDR_DQSWIN    (*(volatile INT32U *)(DDRSRAM_BASE + 0x8C )) //Calibration DQS Delay Window
#define R_DDR_VDATA0    (*(volatile INT32U *)(DDRSRAM_BASE + 0x90 )) //Calibration vdata0
#define R_DDR_VDATA1    (*(volatile INT32U *)(DDRSRAM_BASE + 0x94 )) //Calibration vdata1
#define R_DDR_SKEWTOL   (*(volatile INT32U *)(DDRSRAM_BASE + 0x98 )) //Valid DQS SKEW Tolerance
#define R_DDR_REQSTVM   (*(volatile INT32U *)(DDRSRAM_BASE + 0x9C )) //AHB Request Max Starve Counter
#define R_DDR_CTRL      (*(volatile INT32U *)(DDRSRAM_BASE + 0xA0 )) //DDR control Register
#define R_DDR_LOCKVAL   (*(volatile INT32U *)(DDRSRAM_BASE + 0xA4 )) //DDR Lock Value Register
#define R_DDR_CAL_CTRL  (*(volatile INT32U *)(DDRSRAM_BASE + 0xA8 )) //DDR Calibration control Register
#define R_DDR_RINGCLK   (*(volatile INT32U *)(DDRSRAM_BASE + 0xB0 )) //Ring OSC clock
#define R_DDR_QMAXCNT   (*(volatile INT32U *)(DDRSRAM_BASE + 0xB4 )) //Request in ReqQx Max Wait
#define R_DDR_CALTADDR  (*(volatile INT32U *)(DDRSRAM_BASE + 0xF0 )) //Calibration Test Memory adderss
#define R_DDR_CALTDATA  (*(volatile INT32U *)(DDRSRAM_BASE + 0xF4 )) //Calibration Test Memory Data

/******************************************************************************
 * TFT2 SPI:
 ******************************************************************************/
#define TFT_SPI0_BASE			((volatile INT32U *) 0xC01B0000)
#define TFT_SPI1_BASE			((volatile INT32U *) 0xC0130000)
#define R_TFT2_SPI0_CTRL        (*((volatile INT32U *) 0xC0130000))
#define R_TFT2_SPI0_STS         (*((volatile INT32U *) 0xC0130004))
#define R_TFT2_SPI0_TX_DAT      (*((volatile INT32U *) 0xC0130008))
#define R_TFT2_SPI0_RX_DAT      (*((volatile INT32U *) 0xC013000C))
#define R_TFT2_SPI0_CTRL2       (*((volatile INT32U *) 0xC0130010))
#define R_TFT2_SPI0_VSYNC       (*((volatile INT32U *) 0xC0130018))
#define R_TFT2_SPI0_VDISP       (*((volatile INT32U *) 0xC013001C))
#define R_TFT2_SPI0_HSYNC       (*((volatile INT32U *) 0xC0130020))
#define R_TFT2_SPI0_HDISP       (*((volatile INT32U *) 0xC0130024))
#define R_TFT2_SPI0_DATA_CNT    (*((volatile INT32U *) 0xC0130028))
#define R_TFT2_SPI0_TE_CNT      (*((volatile INT32U *) 0xC013002C))
#define R_TFT_SPI0_CTRL2       (*((volatile INT32U *) 0xC0190010))

/******************************************************************************
 * MIPI DSI CONTROL REGISTERS
 ******************************************************************************/
#define MIPI_DSI_BASE           0xD2200000
#define DSI_BASE				((volatile INT32U *) 0xD2200000)
#define R_DSI_VM_HT_CTRL		(*((volatile INT32U *) 0xD2200000))			//Horizontal Timing Control
#define R_DSI_VM_VT0_CTRL		(*((volatile INT32U *) 0xD2200004))			//Vertical 0 Timing Control
#define R_DSI_VM_VT1_CTRL		(*((volatile INT32U *) 0xD2200008))			//Vertical 1 Timing Control
#define R_DSI_VM_BLLP_CTRL		(*((volatile INT32U *) 0xD220000C))			//BLLP Timing
#define R_DSI_LPCK_CTRL			(*((volatile INT32U *) 0xD2200010))			//Low Power clock divider
#define R_DSI_LANE_TIMING_CTRL	(*((volatile INT32U *) 0xD2200014))			//Timing Control for CLK/Data Lane
#define R_DSI_CL_TM0_CTRL		(*((volatile INT32U *) 0xD2200018))			//Clock Lane Timing Control
#define R_DSI_CL_TM1_CTRL		(*((volatile INT32U *) 0xD220001C))			//Clock Lane Timing Control
#define R_DSI_DL_TM_CTRL		(*((volatile INT32U *) 0xD2200020))			//Data Lane Timing Control
#define R_DSI_VM_INPUT_CTRL		(*((volatile INT32U *) 0xD2200030))			//Video Mode Input format
#define R_DSI_BLK_CTRL			(*((volatile INT32U *) 0xD2200034))			//Operation Mode Control for Blanking Period
#define R_DSI_SPKT_HD_CTRL		(*((volatile INT32U *) 0xD2200024))			//Short Packet Header
#define R_DSI_LPKT_HD_CTRL		(*((volatile INT32U *) 0xD2200028))			//Long Packet Header
#define R_DSI_LPKT_PLD_CTRL		(*((volatile INT32U *) 0xD220002C))			//Long Packet Payload Control
#define R_DSI_OP_CTRL			(*((volatile INT32U *) 0xD2200038))			//Operation control
#define R_DSI_CTRL				(*((volatile INT32U *) 0xD220003C))
#define R_DSI_FIFO_STATUS		(*((volatile INT32U *) 0xD2200040))
#define R_DSI_BTA_CTRL			(*((volatile INT32U *) 0xD2200044))
#define R_DSI_LANE_SWAP			(*((volatile INT32U *) 0xD2200048))
#define R_DSI_RGBWC_CTRL		(*((volatile INT32U *) 0xD220004C))
#define R_DSI_RTN_RD_PORT		(*((volatile INT32U *) 0xD2200050))			//Return Data Entry Port
#define R_DSI_ACKERR_RSP		(*((volatile INT32U *) 0xD2200054))			//Ack Response Status Register
#define R_DSI_RSP_DBC			(*((volatile INT32U *) 0xD2200058))			//Response Data Byte Count
#define R_DSI_INT_STATUS		(*((volatile INT32U *) 0xD2200060))			//Interrupt Status
#define R_DSI_INT_EN_CTRL		(*((volatile INT32U *) 0xD2200064))			//Interrupt Enable
#define R_DSI_ERR_STATUS		(*((volatile INT32U *) 0xD2200068))			//Error Interrupt Status
#define R_DSI_STATUS			(*((volatile INT32U *) 0xD2200070))
#define R_DSI_WUD_CTRL			(*((volatile INT32U *) 0xD2200074))			//Wakeup Delay
#define R_DSI_CK_CTRL			(*((volatile INT32U *) 0xD2200078))
#define R_DSI_PHY_CTRL			(*((volatile INT32U *) 0xD2200090))

/******************************************************************************
 * GPU
 ******************************************************************************/

#define R_GPU_BASE			0xD2400000

/******************************************************************************
 * CRYPTO
 ******************************************************************************/
#define AXI_BUS_SEL         (*((volatile INT32U *) 0xD0000134))

#define R_CRYPTO_BASE		0xD2600100

#define R_AESDMACS			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x00)))
#define R_AESSPTR			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x04)))
#define R_AESDPTR			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x08)))
#define R_AESPAR0			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x0C)))
#define R_AESPAR1			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x10)))
#define R_AESPAR2			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x14)))

#define R_HASHDMACS			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x18)))
#define R_HASHSPTR			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x1C)))
#define R_HASHDPTR			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x20)))
#define R_HASHPAR0			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x24)))
#define R_HASHPAR1			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x28)))
#define R_HASHPAR2			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x2C)))

#define R_RSADMACS			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x30)))
#define R_RSASPTR			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x34)))
#define R_RSADPTR			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x38)))
#define R_RSAPAR0			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x3C)))
#define R_RSAYPTR			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x40)))
#define R_RSANPTR			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x44)))
#define R_RSAPAR3			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x48)))
#define R_RSAPAR4			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x4C)))
#define R_RSAPAR5			(*((volatile INT32U *)(R_CRYPTO_BASE + 0x50)))

#define R_AESDMA_CRCR		(*((volatile INT32U *)(R_CRYPTO_BASE + 0x54)))
#define R_AESDMA_ERBAR		(*((volatile INT32U *)(R_CRYPTO_BASE + 0x58)))
#define R_AESDMA_ERDPR		(*((volatile INT32U *)(R_CRYPTO_BASE + 0x5C)))
#define R_AESDMA_RCSR		(*((volatile INT32U *)(R_CRYPTO_BASE + 0x60)))
#define R_AESDMA_RTR		(*((volatile INT32U *)(R_CRYPTO_BASE + 0x64)))

#define R_HASHDMA_CRCR		(*((volatile INT32U *)(R_CRYPTO_BASE + 0x68)))
#define R_HASHDMA_ERBAR		(*((volatile INT32U *)(R_CRYPTO_BASE + 0x6C)))
#define R_HASHDMA_ERDPR		(*((volatile INT32U *)(R_CRYPTO_BASE + 0x70)))
#define R_HASHDMA_RCSR		(*((volatile INT32U *)(R_CRYPTO_BASE + 0x74)))
#define R_HASHDMA_RTR		(*((volatile INT32U *)(R_CRYPTO_BASE + 0x78)))

#define R_SECIE				(*((volatile INT32U *)(R_CRYPTO_BASE + 0x104)))
#define R_SECIF				(*((volatile INT32U *)(R_CRYPTO_BASE + 0x108)))
#define R_SEC_CTRL          (*((volatile INT32U *)(R_CRYPTO_BASE + 0x10C)))

#define R_EFUSE_AESKEY_CTRL   (*((volatile INT32U *)(R_CRYPTO_BASE + 0x110)))
#define R_EFUSE_AESKEY_RDATA  (*((volatile INT32U *)(R_CRYPTO_BASE + 0x114)))
#define R_SEC_CTRL1           (*((volatile INT32U *)(R_CRYPTO_BASE + 0x118)))

//TRNG
#define P_TRNG_DMA_DATA  ((volatile INT32U *) 0xC02F0008)
#define R_TRNG_CTRL1	 (*((volatile INT32U *) 0xC02F0000))
#define R_TRNG_CTRL2     (*((volatile INT32U *) 0xC02F0004))
#define R_TRNG_DMA_DATA	 (*((volatile INT32U *) 0xC02F0008))
#define R_TRNG_DATA      (*((volatile INT32U *) 0xC02F000C))
#define R_TRNG_STATUS    (*((volatile INT32U *) 0xC02F0014))

//efuse
#define R_EFUSE_DATA_BASE		((volatile INT32U *) 0xC0150000)
#define R_EFUSE_DATA_BUF0	    (*((volatile INT32U *) 0xC0150000))
#define R_EFUSE_DATA_BUF1	    (*((volatile INT32U *) 0xC0150004))
#define R_EFUSE_DATA_BUF2	    (*((volatile INT32U *) 0xC0150008))
#define R_EFUSE_DATA_BUF3	    (*((volatile INT32U *) 0xC015000C))
#define R_EFUSE_DATA_BUF4	    (*((volatile INT32U *) 0xC0150010))
#define R_EFUSE_DATA_BUF5	    (*((volatile INT32U *) 0xC0150014))
#define R_EFUSE_DATA_BUF6	    (*((volatile INT32U *) 0xC0150018))
#define R_EFUSE_DATA_BUF7	    (*((volatile INT32U *) 0xC015001C))
#define R_EFUSE_DATA_BUF8	    (*((volatile INT32U *) 0xC0150020))
#define R_EFUSE_DATA_BUF9	    (*((volatile INT32U *) 0xC0150024))
#define R_EFUSE_DATA_BUF10	    (*((volatile INT32U *) 0xC0150028))
#define R_EFUSE_DATA_BUF11	    (*((volatile INT32U *) 0xC015002C))
#define R_EFUSE_DATA_BUF12	    (*((volatile INT32U *) 0xC0150030))
#define R_EFUSE_DATA_BUF13	    (*((volatile INT32U *) 0xC0150034))
#define R_EFUSE_DATA_BUF14	    (*((volatile INT32U *) 0xC0150038))
#define R_EFUSE_DATA_BUF15	    (*((volatile INT32U *) 0xC015003C))
#define R_EFUSE_PD     	        (*((volatile INT32U *) 0xC0150040))
#define R_EFUSE_PGENB 	        (*((volatile INT32U *) 0xC0150044))
#define R_EFUSE_READY     	    (*((volatile INT32U *) 0xC0150048))
#define R_EFUSE_PRO_ADDR 	    (*((volatile INT32U *) 0xC015004C))
#define R_EFUSE_PRO_WAIT_CYC    (*((volatile INT32U *) 0xC0150050))
#define R_EFUSE_PRO_START 	    (*((volatile INT32U *) 0xC0150054))
#define R_EFUSE_WAIT_CYC     	(*((volatile INT32U *) 0xC0150058))
#define R_EFUSE_SOFT_REST 	    (*((volatile INT32U *) 0xC015005C))
#define R_EFUSE_BANK     	    (*((volatile INT32U *) 0xC0150060))
#define R_EFUSE_BACKDOOR_EN 	(*((volatile INT32U *) 0xC0150070))
#define R_EFUSE_BACKDOOR_KEY    (*((volatile INT32U *) 0xC015007C))

/******************************************************************************
 * TZPC Registers
 ******************************************************************************/
#define R_TZPC_CTR                  (*((volatile INT32U *)0xD3000000))
#define R_TZPC_SEC_AHB0             (*((volatile INT32U *)0xD3000004))
#define R_TZPC_SEC_AHB1             (*((volatile INT32U *)0xD3000008))
#define R_TZPC_SEC_AHB2             (*((volatile INT32U *)0xD300000C))
#define R_TZPC_SEC_APB0             (*((volatile INT32U *)0xD3000010))
#define R_TZPC_SEC_APB1             (*((volatile INT32U *)0xD3000014))
#define R_TZPC_SEC_APB2             (*((volatile INT32U *)0xD3000018))
#define R_TZPC_DMA_CTR              (*((volatile INT32U *)0xD300001C))
#define R_TZPC_DMA_SEC_STR_ADDR     (*((volatile INT32U *)0xD3000020))
#define R_TZPC_DMA_SEC_END_ADDR     (*((volatile INT32U *)0xD3000024))
#define R_TZPC_DMA_SEC_STR_ADDR2     (*((volatile INT32U *)0xD3000028))
#define R_TZPC_DMA_SEC_END_ADDR2     (*((volatile INT32U *)0xD300002C))

/******************************************************************************
 * TZASC Registers
 ******************************************************************************/
 #define R_TZASC_BASE                0xC02E0000
#define R_TZASC_BUILD_CONFIG        (*((volatile INT32U *)(R_TZASC_BASE+0x00))) /*Build Configuration Reg: informations about the TZASC,RO*/
#define R_TZASC_ACTION              (*((volatile INT32U *)(R_TZASC_BASE+0x04))) /*Action Reg*/
#define R_TZASC_GATE_KEEPER         (*((volatile INT32U *)(R_TZASC_BASE+0x08))) /*Gate Keeper Reg: to open/close specific filters and provide filter status.*/
#define R_TZASC_SPECULATION_CTRL    (*((volatile INT32U *)(R_TZASC_BASE+0x0C))) /*Speculation Control Reg:*/
#define R_TZASC_INT_STATUS          (*((volatile INT32U *)(R_TZASC_BASE+0x10))) /*RO*/
#define R_TZASC_INT_CLEAR           (*((volatile INT32U *)(R_TZASC_BASE+0x14))) /*WO, W1C*/

/*Filter*/
#define R_TZASC_FILTER_BASE_0         (R_TZASC_BASE+0x20+(0x10*0))    /* RO */
#define R_TZASC_FILTER_BASE_1         (R_TZASC_BASE+0x20+(0x10*1))    /* RO */

/*Region*/
#define R_TZASC_REGION_BASE_0         (R_TZASC_BASE+0x100+(0x20*0))
#define R_TZASC_REGION_BASE_1         (R_TZASC_BASE+0x100+(0x20*1))
#define R_TZASC_REGION_BASE_2         (R_TZASC_BASE+0x100+(0x20*2))
#define R_TZASC_REGION_BASE_3         (R_TZASC_BASE+0x100+(0x20*3))
#define R_TZASC_REGION_BASE_4         (R_TZASC_BASE+0x100+(0x20*4))
#define R_TZASC_REGION_BASE_5         (R_TZASC_BASE+0x100+(0x20*5))
#define R_TZASC_REGION_BASE_6         (R_TZASC_BASE+0x100+(0x20*6))
#define R_TZASC_REGION_BASE_7         (R_TZASC_BASE+0x100+(0x20*7))
#define R_TZASC_REGION_BASE_8         (R_TZASC_BASE+0x100+(0x20*8))


#endif 		// __DRV_L1_SFR_H__

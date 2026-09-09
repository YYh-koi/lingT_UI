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

#ifndef __DRV_L1_INTERRUPT_H__
#define __DRV_L1_INTERRUPT_H__

#include "Drv_l1.h"

#include "drv_l1.h"
#include "drv_l1_sfr.h"
#include "project.h"

/**************************************************************************
 *                          C O N S T A N T S                             *
 **************************************************************************/

/* Core interrupts */
#define Software0_IRQn               0                /**< Cortex-A7 Software Generated Interrupt 0 */
#define Software1_IRQn               1                /**< Cortex-A7 Software Generated Interrupt 1 */
#define Software2_IRQn               2                /**< Cortex-A7 Software Generated Interrupt 2 */
#define Software3_IRQn               3                /**< Cortex-A7 Software Generated Interrupt 3 */
#define Software4_IRQn               4                /**< Cortex-A7 Software Generated Interrupt 4 */
#define Software5_IRQn               5                /**< Cortex-A7 Software Generated Interrupt 5 */
#define Software6_IRQn               6                /**< Cortex-A7 Software Generated Interrupt 6 */
#define Software7_IRQn               7                /**< Cortex-A7 Software Generated Interrupt 7 */
#define Software8_IRQn               8                /**< Cortex-A7 Software Generated Interrupt 8 */
#define Software9_IRQn               9                /**< Cortex-A7 Software Generated Interrupt 9 */
#define Software10_IRQn              10               /**< Cortex-A7 Software Generated Interrupt 10 */
#define Software11_IRQn              11               /**< Cortex-A7 Software Generated Interrupt 11 */
#define Software12_IRQn              12               /**< Cortex-A7 Software Generated Interrupt 12 */
#define Software13_IRQn              13               /**< Cortex-A7 Software Generated Interrupt 13 */
#define Software14_IRQn              14               /**< Cortex-A7 Software Generated Interrupt 14 */
#define Software15_IRQn              15               /**< Cortex-A7 Software Generated Interrupt 15 */
#define VirtualMaintenance_IRQn      25               /**< Cortex-A7 Virtual Maintenance Interrupt */
#define HypervisorTimer_IRQn         26               /**< Cortex-A7 Hypervisor Timer Interrupt */
#define VirtualTimer_IRQn            27               /**< Cortex-A7 Virtual Timer Interrupt */
#define LegacyFastInt_IRQn           28               /**< Cortex-A7 Legacy nFIQ signal Interrupt */
#define SecurePhyTimer_IRQn          29               /**< Cortex-A7 Secure Physical Timer Interrupt */
#define NonSecurePhyTimer_IRQn       30               /**< Cortex-A7 Non-secure Physical Timer Interrupt */
#define LegacyIRQ_IRQn               31               /**< Cortex-A7 Legacy nIRQ Interrupt */

#define SPI_OFFSET			         32               /*SPI(shared peripheral interrupt): 31~*/
// Vector Interrupt Controller
#define VIC_RTCNEW             SPI_OFFSET + 0
#define VIC_MB2SCAN	           SPI_OFFSET + 1
#define VIC_FIQ_SPU	           SPI_OFFSET + 2
#define VIC_EnvIrq_spu	       SPI_OFFSET + 3
#define VIC_PWIrq_spu          SPI_OFFSET + 4
#define VIC_BeatIrq_spu	       SPI_OFFSET + 5
#define VIC_BOX	               SPI_OFFSET + 6
#define VIC_USB20HOST_OHCI2	   SPI_OFFSET + 7
#define VIC_JPEG_B	           SPI_OFFSET + 8
#define VIC_USB20HOST_EHCI2	   SPI_OFFSET + 9
#define VIC_GPU	               SPI_OFFSET + 10
#define VIC_ENMAC              SPI_OFFSET + 11
#define VIC_TIMER0	           SPI_OFFSET + 12
#define VIC_TIMER1	           SPI_OFFSET + 13
#define VIC_TIMER2	           SPI_OFFSET + 14
#define VIC_TIMER3	           SPI_OFFSET + 15
#define VIC_TIMER4	           SPI_OFFSET + 16
#define VIC_TIMER5	           SPI_OFFSET + 17
#define VIC_TIMER6	           SPI_OFFSET + 18
#define VIC_TIMER7	           SPI_OFFSET + 19
#define VIC_TIMER8		       SPI_OFFSET + 20
#define VIC_TMBA         	   SPI_OFFSET + 21
#define VIC_TMBB         	   SPI_OFFSET + 22
#define VIC_TMBC         	   SPI_OFFSET + 23
#define VIC_EXTA	           SPI_OFFSET + 24
#define VIC_EXTB	           SPI_OFFSET + 25
#define VIC_EXTC	           SPI_OFFSET + 26
#define VIC_EXTD	           SPI_OFFSET + 27
#define VIC_EXTE	           SPI_OFFSET + 28
#define VIC_EXTF	           SPI_OFFSET + 29
#define VIC_EXTG	           SPI_OFFSET + 30
#define VIC_EXTH	           SPI_OFFSET + 31
#define VIC_EXTI	           SPI_OFFSET + 32
#define VIC_EXTJ	           SPI_OFFSET + 33
#define VIC_EXTK	           SPI_OFFSET + 34
#define VIC_EXTL	           SPI_OFFSET + 35
#define VIC_UART0	           SPI_OFFSET + 36
#define VIC_UART1	           SPI_OFFSET + 37
#define VIC_UART2       	   SPI_OFFSET + 38
#define VIC_I2C0	           SPI_OFFSET + 39
#define VIC_I2C1	           SPI_OFFSET + 40
#define VIC_I2C2	           SPI_OFFSET + 41
#define VIC_SPI0	           SPI_OFFSET + 42
#define VIC_SPI1	           SPI_OFFSET + 43
#define VIC_PSCA_YONLY		   SPI_OFFSET + 44
#define VIC_CANBUS			   SPI_OFFSET + 45
#define VIC_USB20_B			   SPI_OFFSET + 45
#define VIC_ALM				   SPI_OFFSET + 46
#define VIC_SCH				   SPI_OFFSET + 47
#define VIC_HMS				   SPI_OFFSET + 48
#define VIC_KC				   SPI_OFFSET + 49
#define VIC_ADC				   SPI_OFFSET + 50
#define VIC_ADCF			   SPI_OFFSET + 51
#define VIC_ADCF_B			   SPI_OFFSET + 52
#define VIC_DMA				   SPI_OFFSET + 53
#define VIC_AUDA			   SPI_OFFSET + 54
#define VIC_AUDB			   SPI_OFFSET + 55
#define VIC_BINMODE_B		   SPI_OFFSET + 56
#define VIC_I2S_TX			   SPI_OFFSET + 57
#define VIC_I2S_1TX			   SPI_OFFSET + 58
#define VIC_I2S_2TX			   SPI_OFFSET + 59
#define VIC_I2S_3TX			   SPI_OFFSET + 60
#define VIC_I2S_RX			   SPI_OFFSET + 61
#define VIC_I2S_1RX			   SPI_OFFSET + 62
#define VIC_I2S_2RX			   SPI_OFFSET + 63
#define VIC_I2S_3RX			   SPI_OFFSET + 64
#define VIC_USB20			   SPI_OFFSET + 65
#define VIC_USB20HOST_OHCI	   SPI_OFFSET + 66
#define VIC_USB20HOST_EHCI	   SPI_OFFSET + 67
#define VIC_SCALAR0	           SPI_OFFSET + 68
#define VIC_SD2	               SPI_OFFSET + 69
#define VIC_SD3	               SPI_OFFSET + 70
#define VIC_NFC	               SPI_OFFSET + 71
#define VIC_BCH	               SPI_OFFSET + 72
#define VIC_IPU	               SPI_OFFSET + 73
#define VIC_ROTATOR	           SPI_OFFSET + 74
#define VIC_SCAN2MB			   SPI_OFFSET + 75
#define VIC_PPU	               SPI_OFFSET + 76
#define VIC_MIPI	           SPI_OFFSET + 77
#define VIC_MIPI_B	           SPI_OFFSET + 78
#define VIC_DSI				   SPI_OFFSET + 79
#define VIC_CSI				   SPI_OFFSET + 80
#define VIC_CSI2               SPI_OFFSET + 81
#define VIC_CDSP	           SPI_OFFSET + 82
#define VIC_JPEG	       	   SPI_OFFSET + 83
#define VIC_OVP11			   SPI_OFFSET + 84
#define VIC_PSCALER_A		   SPI_OFFSET + 85
#define VIC_PSCALER_B		   SPI_OFFSET + 86
#define VIC_PWR_ON0			   SPI_OFFSET + 87
#define VIC_PWR_ON1			   SPI_OFFSET + 88
#define VIC_PWR_ON2			   SPI_OFFSET + 89
#define VIC_PWR_ON3			   SPI_OFFSET + 90
#define VIC_GPDLA			   SPI_OFFSET + 91
#define VIC_ROT				   SPI_OFFSET + 92
#define VIC_FFT				   SPI_OFFSET + 93
#define VIC_FBC				   SPI_OFFSET + 94
#define VIC_H264			   SPI_OFFSET + 95
#define VIC_H264_FIFO		   SPI_OFFSET + 96
#define VIC_BINMODE			   SPI_OFFSET + 97
#define VIC_TFT1_SPI		   SPI_OFFSET + 98
#define VIC_TFT2_SPI		   SPI_OFFSET + 99
#define VIC_SEC				   SPI_OFFSET + 100
#define VIC_TZASC			   SPI_OFFSET + 101
#define VIC_TRNG			   SPI_OFFSET + 102
#define VIC_NPMUIRQ_O0		   SPI_OFFSET + 103
#define VIC_HDMI			   SPI_OFFSET + 104
#define VIC_FIR	               SPI_OFFSET + 105
#define VIC_SAR_TEMP		   SPI_OFFSET + 106
#define VIC_ROTATOR_LB         SPI_OFFSET + 107
#define VIC_MAX_IRQ			   SPI_OFFSET + 108


#define VIC_SD	               SPI_OFFSET + 69

#define FIQ_SUPPORT 1

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern INT32S vic_irq_register(INT32U vic_num, void (*isr)(void));
extern INT32S vic_irq_arg_register(INT32U vic_num, void (*isr)(void*), void* arg);
extern INT32S vic_irq_unregister(INT32U vic_num);
extern INT32S vic_irq_enable(INT32U vic_num);
extern INT32S vic_irq_disable(INT32U vic_num);
extern INT32S vic_fiq_enable(INT32U vic_num);
extern INT32S vic_fiq_disable(INT32U vic_num);
extern void vic_init(void);
extern void irq_dispatcher(INT32U giccIar);
extern INT32S vic_irq_priority(INT32U irc_num, INT32U priority);

/**************************************************************************
 *                        CMSIS CORE FUNCTIONS                            *
 **************************************************************************/
/* from CMSIS_5-5.7.0 core_ca.h*/
#define IRQn_Type uint32_t

#define __ASM            __asm                                      /*!< asm keyword for GNU Compiler */
#define __INLINE         inline                                     /*!< inline keyword for GNU Compiler */
#define __STATIC_INLINE  static inline


/* following defines should be used for structure members */
#define     __IM     volatile const      /*!< \brief Defines 'read only' structure member permissions */
#define     __OM     volatile            /*!< \brief Defines 'write only' structure member permissions */
#define     __IOM    volatile            /*!< \brief Defines 'read / write' structure member permissions */
#define RESERVED(N, T) T RESERVED##N;    // placeholder struct members used for "reserved" areas

#define PRIORITY_BITS	5 				//number of bits for priority value

/** \brief  Structure type to access the Generic Interrupt Controller Distributor (GICD)
*/
typedef struct
{
  __IOM uint32_t CTLR;                 /*!< \brief  Offset: 0x000 (R/W) Distributor Control Register */
  __IM  uint32_t TYPER;                /*!< \brief  Offset: 0x004 (R/ ) Interrupt Controller Type Register */
  __IM  uint32_t IIDR;                 /*!< \brief  Offset: 0x008 (R/ ) Distributor Implementer Identification Register */
        RESERVED(0, uint32_t)
  __IOM uint32_t STATUSR;              /*!< \brief  Offset: 0x010 (R/W) Error Reporting Status Register, optional */
        RESERVED(1[11], uint32_t)
  __OM  uint32_t SETSPI_NSR;           /*!< \brief  Offset: 0x040 ( /W) Set SPI Register */
        RESERVED(2, uint32_t)
  __OM  uint32_t CLRSPI_NSR;           /*!< \brief  Offset: 0x048 ( /W) Clear SPI Register */
        RESERVED(3, uint32_t)
  __OM  uint32_t SETSPI_SR;            /*!< \brief  Offset: 0x050 ( /W) Set SPI, Secure Register */
        RESERVED(4, uint32_t)
  __OM  uint32_t CLRSPI_SR;            /*!< \brief  Offset: 0x058 ( /W) Clear SPI, Secure Register */
        RESERVED(5[9], uint32_t)
  __IOM uint32_t IGROUPR[32];          /*!< \brief  Offset: 0x080 (R/W) Interrupt Group Registers */
  __IOM uint32_t ISENABLER[32];        /*!< \brief  Offset: 0x100 (R/W) Interrupt Set-Enable Registers */
  __IOM uint32_t ICENABLER[32];        /*!< \brief  Offset: 0x180 (R/W) Interrupt Clear-Enable Registers */
  __IOM uint32_t ISPENDR[32];          /*!< \brief  Offset: 0x200 (R/W) Interrupt Set-Pending Registers */
  __IOM uint32_t ICPENDR[32];          /*!< \brief  Offset: 0x280 (R/W) Interrupt Clear-Pending Registers */
  __IOM uint32_t ISACTIVER[32];        /*!< \brief  Offset: 0x300 (R/W) Interrupt Set-Active Registers */
  __IOM uint32_t ICACTIVER[32];        /*!< \brief  Offset: 0x380 (R/W) Interrupt Clear-Active Registers */
  __IOM uint32_t IPRIORITYR[255];      /*!< \brief  Offset: 0x400 (R/W) Interrupt Priority Registers */
        RESERVED(6, uint32_t)
  __IOM uint32_t  ITARGETSR[255];      /*!< \brief  Offset: 0x800 (R/W) Interrupt Targets Registers */
        RESERVED(7, uint32_t)
  __IOM uint32_t ICFGR[64];            /*!< \brief  Offset: 0xC00 (R/W) Interrupt Configuration Registers */
  __IOM uint32_t IGRPMODR[32];         /*!< \brief  Offset: 0xD00 (R/W) Interrupt Group Modifier Registers */
        RESERVED(8[32], uint32_t)
  __IOM uint32_t NSACR[64];            /*!< \brief  Offset: 0xE00 (R/W) Non-secure Access Control Registers */
  __OM  uint32_t SGIR;                 /*!< \brief  Offset: 0xF00 ( /W) Software Generated Interrupt Register */
        RESERVED(9[3], uint32_t)
  __IOM uint32_t CPENDSGIR[4];         /*!< \brief  Offset: 0xF10 (R/W) SGI Clear-Pending Registers */
  __IOM uint32_t SPENDSGIR[4];         /*!< \brief  Offset: 0xF20 (R/W) SGI Set-Pending Registers */
        RESERVED(10[5236], uint32_t)
  __IOM uint64_t IROUTER[988];         /*!< \brief  Offset: 0x6100(R/W) Interrupt Routing Registers */
}  GICDistributor_Type;

#define GICDistributor      ((GICDistributor_Type      *)     configINTERRUPT_CONTROLLER_BASE_ADDRESS ) /*!< \brief GIC Distributor register set access pointer */

/** \brief  Structure type to access the Generic Interrupt Controller Interface (GICC)
*/
typedef struct
{
  __IOM uint32_t CTLR;                 /*!< \brief  Offset: 0x000 (R/W) CPU Interface Control Register */
  __IOM uint32_t PMR;                  /*!< \brief  Offset: 0x004 (R/W) Interrupt Priority Mask Register */
  __IOM uint32_t BPR;                  /*!< \brief  Offset: 0x008 (R/W) Binary Point Register */
  __IM  uint32_t IAR;                  /*!< \brief  Offset: 0x00C (R/ ) Interrupt Acknowledge Register */
  __OM  uint32_t EOIR;                 /*!< \brief  Offset: 0x010 ( /W) End Of Interrupt Register */
  __IM  uint32_t RPR;                  /*!< \brief  Offset: 0x014 (R/ ) Running Priority Register */
  __IM  uint32_t HPPIR;                /*!< \brief  Offset: 0x018 (R/ ) Highest Priority Pending Interrupt Register */
  __IOM uint32_t ABPR;                 /*!< \brief  Offset: 0x01C (R/W) Aliased Binary Point Register */
  __IM  uint32_t AIAR;                 /*!< \brief  Offset: 0x020 (R/ ) Aliased Interrupt Acknowledge Register */
  __OM  uint32_t AEOIR;                /*!< \brief  Offset: 0x024 ( /W) Aliased End Of Interrupt Register */
  __IM  uint32_t AHPPIR;               /*!< \brief  Offset: 0x028 (R/ ) Aliased Highest Priority Pending Interrupt Register */
  __IOM uint32_t STATUSR;              /*!< \brief  Offset: 0x02C (R/W) Error Reporting Status Register, optional */
        RESERVED(1[40], uint32_t)
  __IOM uint32_t APR[4];               /*!< \brief  Offset: 0x0D0 (R/W) Active Priority Register */
  __IOM uint32_t NSAPR[4];             /*!< \brief  Offset: 0x0E0 (R/W) Non-secure Active Priority Register */
        RESERVED(2[3], uint32_t)
  __IM  uint32_t IIDR;                 /*!< \brief  Offset: 0x0FC (R/ ) CPU Interface Identification Register */
        RESERVED(3[960], uint32_t)
  __OM  uint32_t DIR;                  /*!< \brief  Offset: 0x1000( /W) Deactivate Interrupt Register */
}  GICInterface_Type;

#define GICInterface        ((GICInterface_Type        *)     (configINTERRUPT_CONTROLLER_BASE_ADDRESS+configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET) )   /*!< \brief GIC Interface register set access pointer */

/* ##########################  GIC functions  ###################################### */

/** \brief  Enable the interrupt distributor using the GIC's CTLR register.
*/
__STATIC_INLINE void GIC_EnableDistributor(void)
{
  GICDistributor->CTLR |= 1U;
}

/** \brief Disable the interrupt distributor using the GIC's CTLR register.
*/
__STATIC_INLINE void GIC_DisableDistributor(void)
{
  GICDistributor->CTLR &=~1U;
}

/** \brief Read the GIC's TYPER register.
* \return GICDistributor_Type::TYPER
*/
__STATIC_INLINE uint32_t GIC_DistributorInfo(void)
{
  return (GICDistributor->TYPER);
}

/** \brief Sets the GIC's ITARGETSR register for the given interrupt.
* \param [in] IRQn Interrupt to be configured.
* \param [in] cpu_target CPU interfaces to assign this interrupt to.
*/
__STATIC_INLINE void GIC_SetTarget(IRQn_Type IRQn, uint32_t cpu_target)
{
  uint32_t mask = GICDistributor->ITARGETSR[IRQn / 4U] & ~(0xFFUL << ((IRQn % 4U) * 8U));
  GICDistributor->ITARGETSR[IRQn / 4U] = mask | ((cpu_target & 0xFFUL) << ((IRQn % 4U) * 8U));
}

/** \brief Read the GIC's ITARGETSR register.
* \param [in] IRQn Interrupt to acquire the configuration for.
* \return GICDistributor_Type::ITARGETSR
*/
__STATIC_INLINE uint32_t GIC_GetTarget(IRQn_Type IRQn)
{
  return (GICDistributor->ITARGETSR[IRQn / 4U] >> ((IRQn % 4U) * 8U)) & 0xFFUL;
}

/** \brief Enable the CPU's interrupt interface.
*/
__STATIC_INLINE void GIC_EnableInterface(void)
{
  GICInterface->CTLR |= 1U; //enable interface
}

/** \brief Disable the CPU's interrupt interface.
*/
__STATIC_INLINE void GIC_DisableInterface(void)
{
  GICInterface->CTLR &=~1U; //disable distributor
}

/** \brief Read the CPU's IAR register.
* \return GICInterface_Type::IAR
*/
__STATIC_INLINE IRQn_Type GIC_AcknowledgePending(void)
{
  return (IRQn_Type)(GICInterface->IAR);
}

/** \brief Writes the given interrupt number to the CPU's EOIR register.
* \param [in] IRQn The interrupt to be signaled as finished.
*/
__STATIC_INLINE void GIC_EndInterrupt(IRQn_Type IRQn)
{
  GICInterface->EOIR = IRQn;
}

__STATIC_INLINE void GICD_EnableGroup1(void)
{
    GICDistributor->CTLR |= BIT1;
}

__STATIC_INLINE void GICC_EnableGroup1(void)
{
    GICInterface->CTLR |= BIT1;
}

__STATIC_INLINE void GICD_DisableGroup1(void)
{
	GICDistributor->CTLR &= ~BIT1;
}

__STATIC_INLINE void GICC_DisableGroup1(void)
{
    GICInterface->CTLR &= ~BIT1;
}

/** \brief Set the interrupt group from the GIC's IGROUPR register.
* \param [in] IRQn The interrupt to be queried.
* \param [in] group Interrupt group number: 0 - Group 0, 1 - Group 1
*/
__STATIC_INLINE void GIC_SetGroup(IRQn_Type IRQn, uint32_t group)
{
  uint32_t igroupr = GICDistributor->IGROUPR[IRQn / 32U];
  uint32_t shift   = (IRQn % 32U);

  igroupr &= (~(1U          << shift));
  igroupr |= ( (group & 1U) << shift);

  GICDistributor->IGROUPR[IRQn / 32U] = igroupr;
}

/** \brief Get the interrupt group from the GIC's IGROUPR register.
* \param [in] IRQn The interrupt to be queried.
* \return 0 - Group 0, 1 - Group 1
*/
__STATIC_INLINE uint32_t GIC_GetGroup(IRQn_Type IRQn)
{
  return (GICDistributor->IGROUPR[IRQn / 32U] >> (IRQn % 32U)) & 1UL;
}

__STATIC_INLINE void GIC_EnableFIQ(void)
{
    GICInterface->CTLR |= BIT3;/*GICC_CTLR.FIQEn*/
    GICInterface->CTLR |= BIT2;/*GICC_CTLR.AckCtl*/
}

__STATIC_INLINE void GIC_DisableFIQ(void)
{
    GICInterface->CTLR &= ~BIT3;/*GICC_CTLR.FIQEn*/
    GICInterface->CTLR &= ~BIT2;/*GICC_CTLR.AckCtl*/
}
/** \brief Enables the given interrupt using GIC's ISENABLER register.
* \param [in] IRQn The interrupt to be enabled.
*/
__STATIC_INLINE void GIC_EnableIRQ(IRQn_Type IRQn)
{
  GICDistributor->ISENABLER[IRQn / 32U] = 1U << (IRQn % 32U);
}

/** \brief Get interrupt enable status using GIC's ISENABLER register.
* \param [in] IRQn The interrupt to be queried.
* \return 0 - interrupt is not enabled, 1 - interrupt is enabled.
*/
__STATIC_INLINE uint32_t GIC_GetEnableIRQ(IRQn_Type IRQn)
{
  return (GICDistributor->ISENABLER[IRQn / 32U] >> (IRQn % 32U)) & 1UL;
}

/** \brief Disables the given interrupt using GIC's ICENABLER register.
* \param [in] IRQn The interrupt to be disabled.
*/
__STATIC_INLINE void GIC_DisableIRQ(IRQn_Type IRQn)
{
  GICDistributor->ICENABLER[IRQn / 32U] = 1U << (IRQn % 32U);
}

/** \brief Get interrupt pending status from GIC's ISPENDR register.
* \param [in] IRQn The interrupt to be queried.
* \return 0 - interrupt is not pending, 1 - interrupt is pendig.
*/
__STATIC_INLINE uint32_t GIC_GetPendingIRQ(IRQn_Type IRQn)
{
  uint32_t pend;

  if (IRQn >= 16U) {
    pend = (GICDistributor->ISPENDR[IRQn / 32U] >> (IRQn % 32U)) & 1UL;
  } else {
    // INTID 0-15 Software Generated Interrupt
    pend = (GICDistributor->SPENDSGIR[IRQn / 4U] >> ((IRQn % 4U) * 8U)) & 0xFFUL;
    // No CPU identification offered
    if (pend != 0U) {
      pend = 1U;
    } else {
      pend = 0U;
    }
  }

  return (pend);
}

/** \brief Sets the given interrupt as pending using GIC's ISPENDR register.
* \param [in] IRQn The interrupt to be enabled.
*/
__STATIC_INLINE void GIC_SetPendingIRQ(IRQn_Type IRQn)
{
  if (IRQn >= 16U) {
    GICDistributor->ISPENDR[IRQn / 32U] = 1U << (IRQn % 32U);
  } else {
    // INTID 0-15 Software Generated Interrupt
    // Forward the interrupt to the CPU interface that requested it
    GICDistributor->SGIR = (IRQn | 0x02000000U);
  }
}

/** \brief Clears the given interrupt from being pending using GIC's ICPENDR register.
* \param [in] IRQn The interrupt to be enabled.
*/
__STATIC_INLINE void GIC_ClearPendingIRQ(IRQn_Type IRQn)
{
  if (IRQn >= 16U) {
    GICDistributor->ICPENDR[IRQn / 32U] = 1U << (IRQn % 32U);
  } else {
    // INTID 0-15 Software Generated Interrupt
    GICDistributor->CPENDSGIR[IRQn / 4U] = 1U << ((IRQn % 4U) * 8U);
  }
}

/** \brief Sets the interrupt configuration using GIC's ICFGR register.
* \param [in] IRQn The interrupt to be configured.
* \param [in] int_config Int_config field value. Bit 0: Reserved (0 - N-N model, 1 - 1-N model for some GIC before v1)
*                                           Bit 1: 0 - level sensitive, 1 - edge triggered
*/
__STATIC_INLINE void GIC_SetConfiguration(IRQn_Type IRQn, uint32_t int_config)
{
  uint32_t icfgr = GICDistributor->ICFGR[IRQn / 16U];
  uint32_t shift = (IRQn % 16U) << 1U;

  icfgr &= (~(3U         << shift));
  icfgr |= (  int_config << shift);

  GICDistributor->ICFGR[IRQn / 16U] = icfgr;
}

/** \brief Get the interrupt configuration from the GIC's ICFGR register.
* \param [in] IRQn Interrupt to acquire the configuration for.
* \return Int_config field value. Bit 0: Reserved (0 - N-N model, 1 - 1-N model for some GIC before v1)
*                                 Bit 1: 0 - level sensitive, 1 - edge triggered
*/
__STATIC_INLINE uint32_t GIC_GetConfiguration(IRQn_Type IRQn)
{
  return (GICDistributor->ICFGR[IRQn / 16U] >> ((IRQn % 16U) >> 1U));
}

/** \brief Set the priority for the given interrupt in the GIC's IPRIORITYR register.
* \param [in] IRQn The interrupt to be configured.
* \param [in] priority The priority for the interrupt, lower values denote higher priorities.
*/
__STATIC_INLINE void GIC_SetPriority(IRQn_Type IRQn, uint32_t priority)
{
  uint32_t mask = GICDistributor->IPRIORITYR[IRQn / 4U] & ~(0xFFUL << ((IRQn % 4U) * 8U));
  GICDistributor->IPRIORITYR[IRQn / 4U] = mask | (((priority << (8-PRIORITY_BITS)) & 0xFFUL) << ((IRQn % 4U) * 8U));
}

/** \brief Read the current interrupt priority from GIC's IPRIORITYR register.
* \param [in] IRQn The interrupt to be queried.
*/
__STATIC_INLINE uint32_t GIC_GetPriority(IRQn_Type IRQn)
{
  return ((GICDistributor->IPRIORITYR[IRQn / 4U] >> ((IRQn % 4U) * 8U)) >> (8-PRIORITY_BITS)) & 0xFFUL;
}

/** \brief Set the interrupt priority mask using CPU's PMR register.
* \param [in] priority Priority mask to be set.
*/
__STATIC_INLINE void GIC_SetInterfacePriorityMask(uint32_t priority)
{
  GICInterface->PMR = priority & 0xFFUL; //set priority mask
}

/** \brief Read the current interrupt priority mask from CPU's PMR register.
* \result GICInterface_Type::PMR
*/
__STATIC_INLINE uint32_t GIC_GetInterfacePriorityMask(void)
{
  return GICInterface->PMR;
}

/** \brief Configures the group priority and subpriority split point using CPU's BPR register.
* \param [in] binary_point Amount of bits used as subpriority.
*/
__STATIC_INLINE void GIC_SetBinaryPoint(uint32_t binary_point)
{
  GICInterface->BPR = binary_point & 7U; //set binary point
}

/** \brief Read the current group priority and subpriority split point from CPU's BPR register.
* \return GICInterface_Type::BPR
*/
__STATIC_INLINE uint32_t GIC_GetBinaryPoint(void)
{
  return GICInterface->BPR;
}

/** \brief Get the status for a given interrupt.
* \param [in] IRQn The interrupt to get status for.
* \return 0 - not pending/active, 1 - pending, 2 - active, 3 - pending and active
*/
__STATIC_INLINE uint32_t GIC_GetIRQStatus(IRQn_Type IRQn)
{
  uint32_t pending, active;

  active = ((GICDistributor->ISACTIVER[IRQn / 32U])  >> (IRQn % 32U)) & 1UL;
  pending = ((GICDistributor->ISPENDR[IRQn / 32U]) >> (IRQn % 32U)) & 1UL;

  return ((active<<1U) | pending);
}

/** \brief Get the interrupt number of the highest interrupt pending from CPU's HPPIR register.
* \return GICInterface_Type::HPPIR
*/
__STATIC_INLINE uint32_t GIC_GetHighPendingIRQ(void)
{
  return GICInterface->HPPIR;
}

/** \brief Initialize the interrupt distributor.
*/
__STATIC_INLINE void GIC_DistInit(void)
{
  uint32_t i;
  uint32_t num_irq = 0U;
  //uint32_t priority_field;

  //A reset sets all bits in the IGROUPRs corresponding to the SPIs to 0,
  //configuring all of the interrupts as Secure.

  //Disable interrupt forwarding
  GIC_DisableDistributor();
  //Get the maximum number of interrupts that the GIC supports
  num_irq = 32U * ((GIC_DistributorInfo() & 0x1FU) + 1U);

  /* Priority level is implementation defined.
   To determine the number of priority bits implemented write 0xFF to an IPRIORITYR
   priority field and read back the value stored.*/
  /*GIC_SetPriority((IRQn_Type)0U, 0xFFU);
  priority_field = GIC_GetPriority((IRQn_Type)0U);*/

  #if FIQ_SUPPORT
  /* Enable Group1 for FIQ support */
  GICD_EnableGroup1();

  for (i = 0; i < 32U; i++)
  {
	GIC_SetGroup((IRQn_Type)i, 1);//Default set to Group 1
  }
  #endif

  for (i = 32U; i < num_irq; i++)
  {
      //Disable the SPI interrupt
      GIC_DisableIRQ((IRQn_Type)i);
      //Set level-sensitive (and N-N model)
      GIC_SetConfiguration((IRQn_Type)i, 0U);
      //Set priority
      GIC_SetPriority((IRQn_Type)i, configMAX_API_CALL_INTERRUPT_PRIORITY);
      //Set target list to CPU0
      GIC_SetTarget((IRQn_Type)i, 1U);
      #if FIQ_SUPPORT
      //Default Set IRQ to Group 1
	  GIC_SetGroup((IRQn_Type)i, 1);
      #endif
  }
  //Enable distributor
  GIC_EnableDistributor();
}

/** \brief Initialize the CPU's interrupt interface
*/
__STATIC_INLINE void GIC_CPUInterfaceInit(void)
{
  uint32_t i;
  uint32_t priority_field;

  //A reset sets all bits in the IGROUPRs corresponding to the SPIs to 0,
  //configuring all of the interrupts as Secure.

  //Disable interrupt forwarding
  GIC_DisableInterface();

  /* Priority level is implementation defined.
   To determine the number of priority bits implemented write 0xFF to an IPRIORITYR
   priority field and read back the value stored.*/
  GIC_SetPriority((IRQn_Type)0U, 0xFFU);
  priority_field = GIC_GetPriority((IRQn_Type)0U);

  #if FIQ_SUPPORT
  /* Enable Group1 for FIQ support */
  GICC_EnableGroup1();
  GIC_EnableFIQ();
  #endif

  //SGI and PPI
  for (i = 0U; i < 32U; i++)
  {
    if(i > 15U) {
      //Set level-sensitive (and N-N model) for PPI
      GIC_SetConfiguration((IRQn_Type)i, 0U);
    }
    //Disable SGI and PPI interrupts
    GIC_DisableIRQ((IRQn_Type)i);
    //Set priority
    GIC_SetPriority((IRQn_Type)i, configMAX_API_CALL_INTERRUPT_PRIORITY);
  }
  //Enable interface
  GIC_EnableInterface();
  //Set binary point to 0
  GIC_SetBinaryPoint(7-PRIORITY_BITS);
  //Set priority mask
  GIC_SetInterfacePriorityMask(priority_field);
}

/** \brief Initialize and enable the GIC
*/
__STATIC_INLINE void GIC_Enable(void)
{
  GIC_DistInit();
  GIC_CPUInterfaceInit(); //per CPU
}

/*== Turnkey ==*/
extern INT32U vic_global_mask_set(void);
extern void vic_global_mask_restore(INT32U old_mask);

#endif		// __DRV_L1_INTERRUPT_H__



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
/* Include files */
#include "drv_l1.h"

/* ------------------------------------------------------------*/
/* Performance Monitor Block                                   */
/* ------------------------------------------------------------*/

/*
 * @brief: Returns the number of progammable counters
 * @param: none
 * @return: the number of progammable counters
*/
INT32U getPMN(void)
{
	INT32U val = 0;
	asm volatile(
		"MRC     p15, 0, %0, c9, c12, 0 ;"	/* Read PMCR Register */
		"MOV     %0, %0, LSR #11        ;"	/* Shift N field down to bit 0 */
		"AND     %0, %0, #0x1F          ;"	/* Mask to leave just the 5 N bits */
		:"=r"(val)
	);

	return val;
}

/*
 * @brief: Sets the event for a programmable counter to record.
 * @param: cntrNum: Which counter to program  (e.g. 0 for PMN0, 1 for PMN1)
 * @param: event: The event code
 * @return: none
*/
void pmn_config(INT32U cntrNum, INT32U event)
{

	asm volatile(
		"AND     %0, %0, #0x1F          ;"	/* Mask to leave only bits 4:0 */
		"MCR     p15, 0, %1, c9, c12, 5 ;"	/* Write PMSELR Register */
		"ISB                            ;"	/* Synchronize context */
		"MCR     p15, 0, %2, c9, c13, 1 ;"	/* Write PMXEVTYPER Register */
		:"=r"(cntrNum)
		:"0"(cntrNum),"r"(event)
		:"memory"
	);
}

/*
 * @brief: Enables/disables the divider (1/64) on CCNT.
 * @param: divider: If 0 disable divider, else enable divider
 * @return: none
*/
void ccnt_divider(INT32U divider)
{
	INT32U val = 0;

	asm volatile(
		"MRC     p15, 0, %0, c9, c12, 0  ;"	/* Read PMCR */
		"CMP     %2, #0x0                ;"	/* IF (r0 == 0)*/
		"BICEQ   %0, %0, #0x08           ;"	/* THEN: Clear the D bit (disables the divisor) */
		"ORRNE   %0, %0, #0x08           ;"	/* ELSE: Set the D bit (enables the divisor) */
		"MCR     p15, 0, %1, c9, c12, 0  ;"	/* Write PMCR */
		:"=r"(val)
		:"0"(val),"r"(divider)
	);

}


/*
 * @brief: Enables User mode access to the PMU (must be called in a privileged mode).
 * @param: none
 * @return: none
*/
void enable_pmu_user_access(void)
{
	INT32U val = 0;

	asm volatile(
		"MRC     p15, 0, %0, c9, c14, 0  ;"	/* Read PMUSERENR Register */
		"ORR     %0, %0, #0x01           ;" /* Set EN bit (bit 0) */
		"MCR     p15, 0, %1, c9, c14, 0  ;"	/* Write PMUSERENR Register */
		"ISB                             ;"	/* Synchronize context */
		:"=r"(val)
		:"0"(val)
		:"memory"
	);
}


/*
 * @brief: Disables User mode access to the PMU (must be called in a priviledged mode).
 * @param: none
 * @return: none
*/
void disable_pmu_user_access(void)
{
	INT32U val = 0;

	asm volatile(
		"MRC     p15, 0, %0, c9, c14, 0  ;"	/* Read PMUSERENR Register */
		"BIC     %0, %0, #0x01           ;" /* Clear EN bit (bit 0) */
		"MCR     p15, 0, %1, c9, c14, 0  ;"	/* Write PMUSERENR Register */
		"ISB                             ;"	/* Synchronize context */
		:"=r"(val)
		:"0"(val)
		:"memory"
	);
}
/*
 * @brief: Global PMU enable
 * @param: none
 * @return: none
*/
void enable_pmu(void)
{
	INT32U val = 0;

	asm volatile(
		"MRC     p15, 0, %0, c9, c12, 0  ;"	/* Read PMCR */
		"ORR     %0, %0, #0x01           ;"	/* Set E bit */
		"MCR     p15, 0, %1, c9, c12, 0  ;"	/* Write PMCR */
		:"=r"(val)
		:"0"(val)
		:
	);
}

/*
 * @brief: Global PMU disable
 * @param: none
 * @return: none
*/
void disble_pmu(void)
{
	INT32U val = 0;

	asm volatile(
		"MRC     p15, 0, %0, c9, c12, 0  ;"	/* Read PMCR */
		"BIC     %0, %0, #0x01           ;"	/* Clear E bit */
		"MCR     p15, 0, %1, c9, c12, 0  ;"	/* Write PMCR */
		:"=r"(val)
		:"0"(val)
		:
	);
}

/*
 * @brief: Enable the CCNT
 * @param: none
 * @return: none
*/
void enable_ccnt(void)
{
	INT32U val = 0;

	asm volatile(
		"MOV     %0, #0x80000000         ;"	/* Set C bit */
		"MCR     p15, 0, %1, c9, c12, 1  ;" /* Write PMCNTENSET Register */
		:"=r"(val)
		:"0"(val)
	);

}

/*
 * @brief: Disable the CCNT
 * @param: none
 * @return: none
*/
void disable_ccnt(void)
{
	INT32U val = 0;

	asm volatile(
		"MOV     %0, #0x80000000         ;"	/* Set C bit */
		"MCR     p15, 0, %1, c9, c12, 2  ;" /* Write PMCNTENCLR Register */
		:"=r"(val)
		:"0"(val)
	);
}

/*
 * @brief: Enable PMN{n}
 * @param: cntrNum: The counter to enable (e.g. 0 for PMN0, 1 for PMN1)
 * @return: none
*/
void enable_pmn(INT32U cntrNum)
{
	INT32U val = 0;

	asm volatile(
		"MOV     %0, #0x1                ;"
		"MOV     %0, %0, LSL %2          ;"	/* Set C bit */
		"MCR     p15, 0, %1, c9, c12, 1  ;" /* Write PMCNTENSET Register */
		:"=r"(val)
		:"0"(val),"r"(cntrNum)
	);
}

/*
 * @brief: Disable PMN{n}
 * @param: cntrNum: The counter to enable (e.g. 0 for PMN0, 1 for PMN1)
 * @return: none
*/
void disable_pmn(INT32U cntrNum)
{
	INT32U val = 0;

	asm volatile(
		"MOV     %0, #0x1                ;"
		"MOV     %0, %0, LSL %2          ;"	/* Set C bit */
		"MCR     p15, 0, %1, c9, c12, 2  ;" /* Write PMCNTENCLR Register */
		:"=r"(val)
		:"0"(val),"r"(cntrNum)
	);
}

/*-------------------------------*/
/*  Counter read registers       */
/*-------------------------------*/
/*
 * @brief: return the value of CCNT
 * @param: none
 * @return: The value of CCNT
*/
INT32U read_ccnt(void)
{
	INT32U val = 0;

	asm volatile(
		"MRC     p15, 0, %0, c9, c13, 0 ;"	/* Read CCNT Register */
		:"=r"(val)
	);

	return val;
}

/*
 * @brief: return the value of PMN{n}
 * @param: cntrNum: The counter to read (e.g. 0 for PMN0, 1 for PMN1)
 * @return: The value of PMN{n}
*/
INT32U read_pmn(INT32U cntrNum)
{
	INT32U val = 0;

	asm volatile(
		"AND     %1, %1, #0x1F          ;"	/* Mask to leave only bits 4:0 */
		"MCR     p15, 0, %2, c9, c12, 5 ;"	/* Write PMSELR Register */
		"ISB                            ;"	/* Synchronize context */
		"MRC     p15, 0, %0, c9, c13, 2 ;"	/* Read current PMNx Register */
		:"=r"(val),"=r"(cntrNum)
		:"1"(cntrNum)
	);

	return val;

}

/*-------------------------------*/
/*  Overflow and Interrupt       */
/*-------------------------------*/
/*
 * @brief: Returns the value of the overflow flags
 * @param: none
 * @return: The value of the overflow flags
*/
INT32U read_ovf_flag(void)
{
	INT32U val = 0;

	asm volatile(
		"MRC     p15, 0, %0, c9, c12, 3 ;"	/* Read PMOVSR Register */
		:"=r"(val)
	);

	return val;
}

/*
 * @brief: Writes the overflow flags
 * @param: The overflow flags to write
 * @return: none
*/
void write_ovf_flag(INT32U flag)
{
	asm volatile(
		"MCR     p15, 0, %0, c9, c12, 3 ;"	/* Write PMOVSR Register */
		"ISB                            ;"	/* Synchronize context */
		:
		:"r"(flag)
		:"memory"
	);
}

/* ---------------------------------------------------------------*/
/*  Reset Functions                                               */
/*----------------------------------------------------------------*/
/*
 * @brief: Resets the programmable counters
 * @param: none
 * @return: none
*/
void reset_pmn(void)
{
	INT32U val = 0;

	asm volatile(
		"MRC     p15, 0, %0, c9, c12, 0  ;"	/* Read PMCR */
		"ORR     %0, %0, #0x2            ;" /* Set P bit (Event counter reset) */
		"MCR     p15, 0, %1, c9, c12, 0  ;" /* Write PMCR */
		:"=r"(val)
		:"0"(val)
	);
}

/*
 * @brief: Resets the CCNT
 * @param: none
 * @return: none
*/
void reset_ccnt(void)
{
	INT32U val = 0;

	asm volatile(
		"MRC     p15, 0, %0, c9, c12, 0  ;"	/* Read PMCR */
		"ORR     %0, %0, #0x4            ;" /* Set C bit (Clock counter reset) */
		"MCR     p15, 0, %1, c9, c12, 0  ;" /* Write PMCR */
		:"=r"(val)
		:"0"(val)
	);
}

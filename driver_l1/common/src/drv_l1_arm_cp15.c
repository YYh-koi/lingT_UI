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
//Include files
#include "drv_l1.h"

static void op_dcache_all(INT32U operation)
{
	INT32U clidr, loc, ctype;
	INT32U level;
	INT32U ccsidr, set, ass, setshift, assshift;
	INT32U i, j, reg;


	clidr = __MRC(15, 1, 0, 0, 1);        //cache level id register
	loc = (clidr >> 24) & 0x7UL;          //loc, @ clidr[26:24]: Indicates the Level of Coherency for the cache hierarchy

	for (level = 0; level < loc; level++)
	{ /* Clean each level */
		ctype = (clidr >> (level * 3)) & 0x7UL;    //ctype: 3bit per level, Ctype1:clidr[2:0], Ctype2: clidr[5:3]
		if (ctype == 2 || /* Data cache only */
			ctype == 3 || /* Separate instruction and data caches */
			ctype == 4)   /* Unified cache */
		{
			__MCR(15, 2, level << 1, 0, 0, 0);		/* Select data cache */
			ccsidr = __MRC(15, 1, 0, 0, 0); 		/* Get cache size ID */
			set = ((ccsidr >> 13) & 0x7FFFUL) + 1;  /* number of sets */
			ass = ((ccsidr >> 3) & 0x3FFUL) + 1;    /* number of ways, therefore cache size should be: cache_line_num*way_num*set*num, ex: 64(byte per line)*4(ways)*128(sets)=32kB */

			setshift = (ccsidr & 0x7UL) + 2 + 2;    /* number of cache line = x<<setshift, the number here is 6(if 64Byte per cache line) */
			for (i = 1; i < 10 && ass > (1UL << i); i++){
			}
			assshift = 32 - i;

			for (i = 0; i < ass; i++){
				for (j = 0; j < set; j++){
					reg = (i << assshift) | (j << setshift) | (level << 1);   //CA7's set/way format: refer to p.1745 of ARM Architecture Reference Manual: ARMv7-A and ARMv7-R edition
					switch (operation)
					{
						case L1_DCACHE_CLEAN:
							 /* DCCSW */
							 __MCR(15, 0, reg, 7, 10, 2);
							break;
						case L1_DCACHE_INVALID:
							/* DCISW */
							__MCR(15, 0, reg, 7, 6, 2);
							break;

						case L1_DCACHE_CLEAN_INVALID:
							/* DCCISW */
							__MCR(15, 0, reg, 7, 14, 2);
							break;

						default:
							break;
					}
				}
			}
			__dsb();
		}
	}
	__dsb();
}

/*
 * @brief: This function enables the data and instruction cache and branch prediction.
 * @param: none
 * @return: none
*/
void drv_l1_cp15_cache_enable(void)
{
    INT32U val = 0;

    asm volatile (
		"mrc	p15, 0, %0, c1, c0, 0;"
		"orr    %0, %0, #(1<<2);"      //data cache bit
		"orr    %0, %0, #(1<<12);"     //instruction cache bit
		"orr    %0, %0, #(1<<11);"     //branch prediction bit(valid after ARMv6)
		"mcr	p15, 0, %1, c1, c0, 0;"
		: "=r" (val)
		: "0" (val)
	);
}

/*
 * @brief: This function check if mmu is on of off.
 * @param: none
 * @return: 0: off, 1: on
*/
int drv_l1_cp15_is_mmu_enable(void)
{
    uint32_t sctlr = __get_SCTLR();
    if ((sctlr & SCTLR_M_Msk) != SCTLR_M_Msk)
        return 0;
    else
        return 1;
}


/*
 * @brief: This function disables the data and instruction cache and disabe branch prediction.
 * @param: none
 * @return: none
*/
void drv_l1_cp15_cache_disable(void)
{
    INT32U val = 0;

    asm volatile (
		"mrc	p15, 0, %0, c1, c0, 0;"
		"bic    %0, %0, #(1<<2);"      //data cache bit
		"bic    %0, %0, #(1<<12);"     //instruction cache bit
		"bic    %0, %0, #(1<<11);"     //branch prediction bit(valid after ARMv6)
		"mcr	p15, 0, %1, c1, c0, 0;"
		: "=r" (val)
		: "0" (val)
	);
}

/*
 * @brief: This function disables the data cache.
 * @param: none
 * @return: none
*/
void drv_l1_cp15_dcache_disable(void)
{
    INT32U val = 0;

    asm volatile (
		"mrc	p15, 0, %0, c1, c0, 0;"
		"bic    %0, %0, #(1<<2);"      //data cache bit
		"mcr	p15, 0, %1, c1, c0, 0;"
		: "=r" (val)
		: "0" (val)
	);
}

/*
 * @brief: This function disables the instruction cache.
 * @param: none
 * @return: none
*/
void drv_l1_cp15_icache_disable(void)
{
    INT32U val = 0;

    asm volatile (
		"mrc	p15, 0, %0, c1, c0, 0;"
		"bic    %0, %0, #(1<<12);"     //instruction cache bit
		"mcr	p15, 0, %1, c1, c0, 0;"
		: "=r" (val)
		: "0" (val)
	);
}

/*
 * @brief: This function enables the data cache.
 * @param: none
 * @return: none
*/
void drv_l1_cp15_dcache_enable(void)
{
    INT32U val = 0;

    asm volatile (
		"mrc	p15, 0, %0, c1, c0, 0;"
		"orr    %0, %0, #(1<<2);"      //data cache bit
		"mcr	p15, 0, %1, c1, c0, 0;"
		: "=r" (val)
		: "0" (val)
	);
}

/*
 * @brief: This function enables the instruction cache.
 * @param: none
 * @return: none
*/
void drv_l1_cp15_icache_enable(void)
{
    INT32U val = 0;

    asm volatile (
		"mrc	p15, 0, %0, c1, c0, 0;"
		"orr    %0, %0, #(1<<12);"     //instruction cache bit
		"mcr	p15, 0, %1, c1, c0, 0;"
		: "=r" (val)
		: "0" (val)
	);
}
/*
 * @brief: Invalidate all instruction cache entries.
 * @param: none
 * @return: none
*/
void drv_l1_cp15_invalidate_icache_all(void)
{
    INT32U val=0;

    asm volatile (
		"mcr	p15, 0, %0, c7, c5, 0;"
		"mcr	p15, 0, %0, c7, c5, 6;"	/*invalidate branch predictor*/
										/*DSB & ISB: Ensure completion of the invalidation */
		"dsb 	0xF;"					/*Data Synchronization Barrier */
		"isb	0xF;"					/*Instruction Synchronization Barrier */
		:
		: "r" (val)
		: "memory"
	);
}

/*
 * @brief: Invalidate a range of data cache entries in the specified address space.
 * @param: start: start address
 * @param: size: total size
 * @return: none
*/
void drv_l1_cp15_invalidate_dcache_range(INT32U start, INT32U size)
{
    INT32U start_line, end;

    if (size == 0) {
        return;
    }

	if(start == DUMMY_BUFFER_ADDRS)
	{
		return;
	}

    start_line = start;
    end = start + size; //end address is exclusive

    asm volatile (
		"tst	%2, #0x3F;"
		"bic	%0, %0, #0x3F;"
     "1: mcr	p15, 0, %2, c7, c6, 1;"		    //invalidate D entry
	    "add	%0, %0, #64;"					/*ARMCA7's data cache line is 64byte*/
	    "cmp	%2, %3;"
	    "blo	1b;"
	    "mov    %0, #0;"
	    "dsb 	0xF;"							/*Data Synchronization Barrier */
		: "=r" (start_line) ,"=r" (end)
		: "0" (start_line) ,"1"(end)
		: "memory"
	);
}

/*
 * @brief: Invalidate all data cache entries.
 * @param: none
 * @return: none
*/
void drv_l1_cp15_invalidate_dcache_all(void)
{
	op_dcache_all(L1_DCACHE_INVALID);
}

/*
 * @brief: Clean a range of data cache entries in the specified address space.
 * @param: start: start address
 * @param: size: total size
 * @return: none
*/
void drv_l1_cp15_clean_dcache_range(INT32U start, INT32U size)
{
    INT32U start_line, end;

    if (size == 0) {
        return;
    }

	if(start == DUMMY_BUFFER_ADDRS)
	{
		return;
	}

    start_line = start;
    end = start + size; //end address is exclusive

    asm volatile (
		"bic	%0, %0, #0x3F;"
     "1: mcr	p15, 0, %2, c7, c10, 1;"		// clean D entry
	    "add	%0, %0, #64;"
	    "cmp	%2, %3;"
	    "blo	1b;"
	    "mov    %0, #0;"
	    "mcr	p15, 0, %2, c7, c10, 4;"		 //drain write buffer
	    "dsb 	0xF;"							/*Data Synchronization Barrier */
		: "=r" (start_line) ,"=r" (end)
		: "0" (start_line) ,"1"(end)
		: "memory"
	);

	if(start < 0x10000000)
	{
		volatile INT32U *ptr = (INT32U *) (((start | 0x80000000) + 0x3) & ~0x3);
		
        R_RANDOM0 = *ptr;									// Read to non-cache area to force cache drain
    }
}

/*
 * @brief: Clean all data cache entries, it must disabe data cache before calling this function.
 * @param: none
 * @return: none
*/
void drv_l1_cp15_clean_dcache_all(void)
{
	op_dcache_all(L1_DCACHE_CLEAN);
}

/*
 * @brief: Clean and invalidate all data cache entries, it must disabe data cache before calling this function.
 * @param: none
 * @return: none
*/
void drv_l1_cp15_clean_invalidate_dcache_all(void)
{
	op_dcache_all(L1_DCACHE_CLEAN_INVALID);
}

/*
 * @brief: This function enables the MMU
 * @param: none
 * @return: none
*/
void drv_l1_cp15_mmu_enable(void)
{
    INT32U val = 0;

    asm volatile (
		"mcr	p15, 0, %0, c8, c7, 0;"			/*Invalid TLB*/
		"dsb 	0xF;"
		"isb 	0xF;"
		"mrc	p15, 0, %0, c1, c0, 0;"
		"orr    %0, %0, #1;"
		"mcr	p15, 0, %1, c1, c0, 0;"
		"isb 	0xF;"
		: "=r" (val)
		: "0" (val)
		: "memory"
	);
}

/*
 * @brief: This function disables the MMU
 * @param: none
 * @return: none
*/
void drv_l1_cp15_mmu_disable(void)
{
    INT32U val = 0;

    asm volatile (
		"mrc	p15, 0, %0, c1, c0, 0;"
		"bic    %0, %0, #1;"
		"mcr	p15, 0, %1, c1, c0, 0;"
		"isb 	0xF;"
		: "=r" (val)
		: "0" (val)
		: "memory"
	);
}

/*
 * @brief: This function sets 16 domains access control.
 * @param: value: domain access control value for each domain, 2 bits represent 1 domain.
 *        0:no access,1:client,2:reserved,3:manager
 * @param: mask: mask for each domain.
 * @return: none
*/
void drv_l1_cp15_mmu_domain_access_set(INT32U value, INT32U mask)
{
	INT32U temp;

    asm volatile (
		"mrc	p15, 0, %0, c3, c0, 0;"
		: "=r" (temp)
		:
		: "memory"
	);

	temp &= ~mask;
	temp |= value;

    asm volatile (
		"mcr	p15, 0, %0, c3, c0, 0;"
		:
		: "r" (temp)
		: "memory"
	);
}


/*
* @brief: drv_l1_cp15_CpuDoIdle: is called in Idle task to enter low power state
* @param: none
* @return: none
*/
void drv_l1_cp15_CpuDoIdle(void)
{
  asm volatile("wfi":::);						//Wait for interrupt
}

/*
 * @brief: Invalidate the specified area in cache
 * @param: addr: start address of the specified area.
 * @param: size: size of the specified area.
 * @return: 0: Success, -1: Failed
*/
INT32S cache_invalid_range(INT32U addr, INT32U size)
{
    drv_l1_cp15_clean_dcache_range(addr, size);
    drv_l1_cp15_invalidate_dcache_range(addr, size);

	return 0;
}

/*
 * @brief: Drain the specified area in cache
 * @param: addr: start address of the specified area.
 * @param: size: size of the specified area.
 * @return: 0: Success, -1: Failed
*/
INT32S cache_drain_range(INT32U addr, INT32U size)
{
    drv_l1_cp15_clean_dcache_range(addr, size);

    return 0;
}


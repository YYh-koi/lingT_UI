#ifndef __DRV_L1_PERF_H__
#define __DRV_L1_PERF_H__

enum armv7_perf_types {
	ARMV7_PERFCTR_PMNC_SW_INCR			= 0x00,
	ARMV7_PERFCTR_L1_ICACHE_REFILL		= 0x01,
	ARMV7_PERFCTR_ITLB_REFILL			= 0x02,
	ARMV7_PERFCTR_L1_DCACHE_REFILL		= 0x03,
	ARMV7_PERFCTR_L1_DCACHE_ACCESS		= 0x04,
	ARMV7_PERFCTR_DTLB_REFILL			= 0x05,
	ARMV7_PERFCTR_MEM_READ				= 0x06,
	ARMV7_PERFCTR_MEM_WRITE				= 0x07,
	ARMV7_PERFCTR_INSTR_EXECUTED		= 0x08,
	ARMV7_PERFCTR_EXC_TAKEN				= 0x09,
	ARMV7_PERFCTR_EXC_EXECUTED			= 0x0A,
	ARMV7_PERFCTR_CID_WRITE				= 0x0B,

	/*
	 * ARMV7_PERFCTR_PC_WRITE is equivalent to HW_BRANCH_INSTRUCTIONS.
	 * It counts:
	 *  - all (taken) branch instructions,
	 *  - instructions that explicitly write the PC,
	 *  - exception generating instructions.
	 */
	ARMV7_PERFCTR_PC_WRITE				= 0x0C,
	ARMV7_PERFCTR_PC_IMM_BRANCH			= 0x0D,
	ARMV7_PERFCTR_PC_PROC_RETURN		= 0x0E,
	ARMV7_PERFCTR_MEM_UNALIGNED_ACCESS	= 0x0F,
	ARMV7_PERFCTR_PC_BRANCH_MIS_PRED	= 0x10,
	ARMV7_PERFCTR_CLOCK_CYCLES			= 0x11,
	ARMV7_PERFCTR_PC_BRANCH_PRED		= 0x12,

	/* These events are defined by the PMUv2 supplement (ARM DDI 0457A). */
	ARMV7_PERFCTR_MEM_ACCESS			= 0x13,
	ARMV7_PERFCTR_L1_ICACHE_ACCESS		= 0x14,
	ARMV7_PERFCTR_L1_DCACHE_WB			= 0x15,
	ARMV7_PERFCTR_L2_CACHE_ACCESS		= 0x16,
	ARMV7_PERFCTR_L2_CACHE_REFILL		= 0x17,
	ARMV7_PERFCTR_L2_CACHE_WB			= 0x18,
	ARMV7_PERFCTR_BUS_ACCESS			= 0x19,
	ARMV7_PERFCTR_MEM_ERROR				= 0x1A,
	ARMV7_PERFCTR_INSTR_SPEC			= 0x1B,
	ARMV7_PERFCTR_TTBR_WRITE			= 0x1C,
	ARMV7_PERFCTR_BUS_CYCLES			= 0x1D,
	ARMV7_PERFCTR_IRQ					= 0x86,
	ARMV7_PERFCTR_FIQ					= 0x87,
	ARMV7_PERFCTR_CPU_CYCLES			= 0xFF
};


extern INT32U getPMN(void);
extern void pmn_config(INT32U cntrNum, INT32U event);
extern void ccnt_divider(INT32U divider);
extern void enable_pmu_user_access(void);
extern void disable_pmu_user_access(void);
extern void enable_pmu(void);
extern void disble_pmu(void);
extern void enable_ccnt(void);
extern void disable_ccnt(void);
extern void enable_pmn(INT32U cntrNum);
extern void disable_pmn(INT32U cntrNum);
extern INT32U read_ccnt(void);
extern INT32U read_pmn(INT32U cntrNum);
extern INT32U read_ovf_flag(void);
extern void write_ovf_flag(INT32U flag);
extern void reset_pmn(void);
extern void reset_ccnt(void);
#endif

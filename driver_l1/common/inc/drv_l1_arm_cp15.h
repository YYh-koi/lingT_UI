#ifndef __drv_l1_ARM_CP15_H__
#define __drv_l1_ARM_CP15_H__

#define __STRINGIFY(x) #x

#define __MCR(coproc, opcode_1, src, CRn, CRm, opcode_2)                          \
    asm volatile ("MCR " __STRINGIFY(p##coproc) ", " __STRINGIFY(opcode_1) ", " \
                    "%0, " __STRINGIFY(c##CRn) ", " __STRINGIFY(c##CRm) ", "      \
                    __STRINGIFY(opcode_2)                                         \
                    : : "r" (src) )

#define __MRC(coproc, opcode_1, CRn, CRm, opcode_2)                               \
  ({                                                                              \
    uint32_t __dst;                                                               \
    asm volatile ("MRC " __STRINGIFY(p##coproc) ", " __STRINGIFY(opcode_1) ", " \
                    "%0, " __STRINGIFY(c##CRn) ", " __STRINGIFY(c##CRm) ", "      \
                    __STRINGIFY(opcode_2)                                         \
                    : "=r" (__dst) );                                             \
    __dst;                                                                        \
  })

#define __dsb()	\
 ({				\
	asm volatile( \
	"dsb 	0xF;" \
	:             \
	:             \
	:"memory"     \
	);            \
 })

#define __isb()	\
 ({				\
	asm volatile( \
	"isb 	0xF;" \
	:             \
	:             \
	:"memory"     \
	);            \
 })

enum{
	L1_DCACHE_CLEAN=1,
	L1_DCACHE_INVALID,
	L1_DCACHE_CLEAN_INVALID
};

extern void MMU_PTCreate(void);
extern void drv_l1_cp15_cache_enable(void);
extern void drv_l1_cp15_dcache_enable(void);
extern void drv_l1_cp15_icache_enable(void);
extern int drv_l1_cp15_is_mmu_enable(void);
extern void drv_l1_cp15_mmu_enable(void);
extern void drv_l1_cp15_mmu_domain_access_set(INT32U value, INT32U mask);
extern void drv_l1_cp15_cache_disable(void);
extern void drv_l1_cp15_dcache_disable(void);
extern void drv_l1_cp15_icache_disable(void);
extern void drv_l1_cp15_invalidate_dcache_range(INT32U start, INT32U size);
extern void drv_l1_cp15_invalidate_dcache_all(void);
extern void drv_l1_cp15_invalidate_icache_all(void);
extern void drv_l1_cp15_clean_dcache_range(INT32U start, INT32U size);
extern void drv_l1_cp15_clean_dcache_all(void);
extern void drv_l1_cp15_clean_invalidate_dcache_all(void);
extern void drv_l1_cp15_CpuDoIdle(void);
extern INT32S cache_invalid_range(INT32U addr, INT32U size);
extern INT32S cache_drain_range(INT32U addr, INT32U size);

#endif		// __drv_l1_ARM_CP15_H__

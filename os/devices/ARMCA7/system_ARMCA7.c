#include "drv_l1.h"

extern uint32_t __VECTOR_TABLE[];

void SystemInit (void) {
  uint32_t sctlr;
  uint32_t actlr;
#if ((__FPU_PRESENT == 1) && (__FPU_USED == 1))
  uint32_t cpacr;
  uint32_t fpexc;
#endif

  actlr = __get_ACTLR();
  actlr = (actlr | ACTLR_SMP_Msk);       /* Change to SMP mode before enable DCache */
  __set_ACTLR(actlr);

  sctlr = __get_SCTLR();
  sctlr = (sctlr & ~(SCTLR_V_Msk       | /* Use low vector */
                     SCTLR_A_Msk       | /* Disable alignment fault checking */
                     SCTLR_M_Msk))       /* Disable MMU */
                 |  (SCTLR_I_Msk       | /* Enable ICache */
                     SCTLR_Z_Msk       | /* Enable Prediction */
                     SCTLR_CP15BEN_Msk | /* Enable CP15 barrier operations */
                     SCTLR_C_Msk);       /* Enable DCache */
  __set_SCTLR(sctlr);

  /* Set vector base address */
  GIC_Enable();

  __set_VBAR((uint32_t)__VECTOR_TABLE);

#if ((__FPU_PRESENT == 1) && (__FPU_USED == 1))
  cpacr = __get_CPACR();
  /* Enable NEON and FPU */
  cpacr = (cpacr & ~(CPACR_ASEDIS_Msk | CPACR_D32DIS_Msk))
         |  (3UL << CPACR_cp10_Pos) | (3UL << CPACR_cp11_Pos);
  __set_CPACR(cpacr);

  fpexc = __get_FPEXC();
  fpexc |= 0x40000000UL;  /* Enable NEON and FPU */
  __set_FPEXC(fpexc);
#endif /* ((__FPU_PRESENT == 1) && (__FPU_USED == 1)) */
}

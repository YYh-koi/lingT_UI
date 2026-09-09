#include "drv_l1.h"
#include "project.h"

extern INT32U Undefined_Handler;
extern INT32U SVC_Handler;
extern INT32U PrefAbort_Handler;
extern INT32U DataAbort_Handler;
extern INT32U IRQ_Handler;
extern INT32U FIQ_Handler;
/*
INT32U *ppHandler_Undef = &pHandler_Undef;
INT32U *ppHandler_SWI = &pHandler_SWI;
INT32U *ppHandler_PAbort = &pHandler_PAbort;
INT32U *ppHandler_DAbort = &pHandler_DAbort;
INT32U *ppHandler_IRQ = &pHandler_IRQ;
INT32U *ppHandler_FIQ = &pHandler_FIQ;

void interrupt_vector_table_init(void)
{
    if (R_REMAP & 0x02) {
        // REMAP bit on, interrupt wiill jump to dram space
        // this case for code that run on dram address space 0x0
        ppHandler_Undef     = &pHandler_Undef;
        ppHandler_SWI       = &pHandler_SWI;
        ppHandler_PAbort    = &pHandler_PAbort;
        ppHandler_DAbort    = &pHandler_DAbort;
        ppHandler_IRQ       = &pHandler_IRQ;
        ppHandler_FIQ       = &pHandler_FIQ;
    }
    else {
        // REMAP bit off, interrupt wiill jump to RomCode space
        // this case for code that run on sram, SPIFC, dram address space 0x800000000
        ppHandler_Undef     = (INT32U *)0xf8000000;
        ppHandler_SWI       = (INT32U *)0xf8000004;
        ppHandler_PAbort    = (INT32U *)0xf8000008;
        ppHandler_DAbort    = (INT32U *)0xf800000c;
        ppHandler_IRQ       = (INT32U *)0xf8000010;
        ppHandler_FIQ       = (INT32U *)0xf8000014;
    }
}*/

static void debug_exception_reset(const char *name, INT32U address)
{
	DBG_PRINT("EXCEPTION: %s addr=0x%08x\r\n", name, address);
    #if (defined BOARD_TYPE) && (BOARD_TYPE < BOARD_EMU_BASE)
	R_SYSTEM_WATCHDOG_CTRL = 0x8002;		// Enable watchdog to reset system after 0.5 second
    #else
	while (1);
    #endif
}

void debug_undefined(INT32U address)
{
	debug_exception_reset("undefined", address);
}

void debug_swi(INT32U address)
{
	debug_exception_reset("swi", address);
}

void debug_prefetch_abort(INT32U address)
{
	debug_exception_reset("prefetch abort", address);
}

void debug_data_abort(INT32U address)
{
	debug_exception_reset("data abort", address);
}

void debug_irq(void)
{
	while (1);
}

void debug_fiq(void)
{
	while (1);
}

void exception_table_init(void)
{
	Undefined_Handler = (INT32U) debug_undefined;
	SVC_Handler = (INT32U) debug_swi;
	PrefAbort_Handler = (INT32U) debug_prefetch_abort;
	DataAbort_Handler = (INT32U) debug_data_abort;
	IRQ_Handler = (INT32U) debug_irq;
	FIQ_Handler = (INT32U) debug_fiq;
}

INT32S register_exception_table(INT32U exception, INT32U handler)
{
	switch (exception) {
	case EXCEPTION_UNDEF:
		Undefined_Handler = (INT32U) handler;
		break;
	case EXCEPTION_SWI:
		SVC_Handler = (INT32U) handler;
		break;
	case EXCEPTION_PABORT:
		PrefAbort_Handler = (INT32U) handler;
		break;
	case EXCEPTION_DABORT:
		DataAbort_Handler = (INT32U) handler;
		break;
	case EXCEPTION_IRQ:
		IRQ_Handler = (INT32U) handler;
		break;
	case EXCEPTION_FIQ:
		FIQ_Handler = (INT32U) handler;
		break;
	default:
		return STATUS_FAIL;
	}
	return STATUS_OK;
}

#if _OPERATING_SYSTEM == _OS_NONE      // Hard protect should not be used when OS is available
void irq_enable(void)
{
  asm("MRS R0, CPSR");
  asm("BIC R0, R0, #128"); // #INT_IRQ
  asm("MSR CPSR_c, R0");
}

void irq_disable(void)
{
  asm("MRS R0, CPSR");
  asm("ORR R0, R0, #128"); // #INT_IRQ
  asm("MSR CPSR_c, R0");
}

void fiq_enable(void)
{
  asm("MRS R0, CPSR");
  asm("BIC R0, R0, #64"); // #INT_FIQ
  asm("MSR CPSR_c, R0");
}

void fiq_disable(void)
{
  asm("MRS R0, CPSR");
  asm("ORR R0, R0, #64"); // #INT_FIQ
  asm("MSR CPSR_c, R0");
}
#endif

#define __ATOLLIC__
#define __STARTUP_CLEAR_BSS

	/*.global Undefined_Handler
	.global SVC_Handler
	.global PrefAbort_Handler
	.global DataAbort_Handler
	.global IRQ_Handler
	.global FIQ_Handler

	.global Reset_Handler*/


    .syntax unified
    .arch armv7-a

    .section .isr_vector, "a"
    .align 2
    .globl __isr_vector
__isr_vector:
    ldr     pc, =Reset_Handler           /* Reset                  */
    ldr     pc, =Undefined_Handler       /* Undefined instructions */
    ldr     pc, =FreeRTOS_SWI_Handler    /* Supervisor Call        */
    ldr     pc, =PrefAbort_Handler       /* Prefetch abort         */
    ldr     pc, =DataAbort_Handler       /* Data abort             */
    .word   0                            /* RESERVED               */
    ldr     pc, =FreeRTOS_IRQ_Handler    /* IRQ interrupt          */
    ldr     pc, =FreeRTOS_FIQ_Handler    /* FIQ interrupt          */

    .size    __isr_vector, . - __isr_vector

    .text
    .arm

/* Reset Handler */

    .arm
    .align 2
    .globl   Reset_Handler
    .weak    Reset_Handler
    .type    Reset_Handler, %function
Reset_Handler:
    cpsid   i               /* Mask interrupts */

    /* Reset SCTlr Settings */
    mrc     p15, 0, r0, c1, c0, 0     /* Read CP15 System Control register                  */
    bic     r0,  r0, #(0x1 << 12)     /* Clear I bit 12 to disable I Cache                  */
    bic     r0,  r0, #(0x1 <<  2)     /* Clear C bit  2 to disable D Cache                  */
    bic     r0,  r0, #0x2             /* Clear A bit  1 to disable strict alignment         */
    bic     r0,  r0, #(0x1 << 11)     /* Clear Z bit 11 to disable branch prediction        */
    bic     r0,  r0, #0x1             /* Clear M bit  0 to disable MMU                      */
    mcr     p15, 0, r0, c1, c0, 0     /* Write value back to CP15 System Control register   */

    /* Set up stack for IRQ, System/User and Supervisor Modes */
    cps     #0x12                /* Enter IRQ mode                */
    ldr     sp, =__IStackTop     /* Set up IRQ handler stack      */

	cps     #0x11                /* Enter FIQ mode                */
    ldr     sp, =__FStackTop     /* Set up FIQ handler stack      */

    cps     #0x1F                /* Enter System mode             */
    ldr     sp, =__CStackTop     /* Set up System/User Mode stack */

    cps     #0x13                /* Enter Supervisor mode         */
    ldr     sp, =__CStackTop     /* Set up Supervisor Mode stack  */

#ifndef __NO_SYSTEM_INIT
    ldr     r0,=SystemInit
    blx     r0
#endif
    cpsie   i                    /* Unmask interrupts             */

/*     Loop to copy data from read only memory to RAM. The ranges
 *      of copy from/to are specified by following symbols evaluated in
 *      linker script.
 *      __etext: End of code section, i.e., begin of data sections to copy from.
 *      __data_start__/__data_end__: RAM address range that data should be
 *      copied to. Both must be aligned to 4 bytes boundary.  */

    ldr    r1, =__etext

#ifdef __STARTUP_CLEAR_BSS
/*     This part of work usually is done in C library startup code. Otherwise,
 *     define this macro to enable it in this startup.
 *
 *     Loop to zero out BSS section, which uses following symbols
 *     in linker script:
 *      __bss_start__: start of BSS section. Must align to 4
 *      __bss_end__: end of BSS section. Must align to 4
 */
    ldr r1, =__bss_start__
    ldr r2, =__bss_end__

    movs    r0, 0
.LC6:
    cmp     r1, r2
    itt     lt
    strlt   r0, [r1], #4
    blt    .LC6
#endif /* __STARTUP_CLEAR_BSS */

#ifndef __START
#define __START _start
#endif
#ifndef __ATOLLIC__
    ldr   r0,=__START
    blx   r0
#else
    ldr   r0,=__END_STACK
    ldr   r1,=__RAM_NON_CACHE_START
    bl    heap_init
    ldr   r0,=__libc_init_array
    blx   r0
    bl    main
#endif
    .pool
    .size Reset_Handler, . - Reset_Handler

    .align  2
    .arm
    .weak Undefined_Handler
    .type Undefined_Handler, %function
Undefined_Handler:
    b Undefined_Handler
    .size Undefined_Handler, . - Undefined_Handler

    .align 2
    .arm
    .weak SVC_Handler
    .type SVC_Handler, %function
SVC_Handler:
    ldr   r0,=SVC_Handler
    bx    r0
    .size SVC_Handler, . - SVC_Handler

    .align 2
    .arm
    .weak PrefAbort_Handler
    .type PrefAbort_Handler, %function
PrefAbort_Handler:
    ldr   r0,=PrefAbort_Handler
    bx    r0
    .size PrefAbort_Handler, . - PrefAbort_Handler

    .align 2
    .arm
    .weak DataAbort_Handler
    .type DataAbort_Handler, %function
DataAbort_Handler:
    ldr   r0,=DataAbort_Handler
    bx    r0
    .size DataAbort_Handler, . - DataAbort_Handler

    .align 2
    .arm
    .weak IRQ_Handler
    .type IRQ_Handler, %function
IRQ_Handler:
    ldr   r0,=IRQ_Handler
    bx    r0
    .size IRQ_Handler, . - IRQ_Handler

    .align 2
    .arm
    .weak FIQ_Handler
    .type FIQ_Handler, %function
FIQ_Handler:
    ldr   r0,=FIQ_Handler
    bx    r0
    .size FIQ_Handler, . - FIQ_Handler

    .end

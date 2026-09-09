
/* Entry Point */
ENTRY(Reset_Handler)

HEAP_SIZE   = DEFINED(__heap_size__)           ? __heap_size__          : 0x1000;
ISTACK_SIZE = DEFINED(__irq_stack_size__)      ? __irq_stack_size__     : 0x1000;
FSTACK_SIZE = DEFINED(__fiq_stack_size__)      ? __fiq_stack_size__     : 0x1000;
CSTACK_SIZE = DEFINED(__stack_size__)          ? __stack_size__         : 0x1000;
RSTACK_SIZE = DEFINED(__resume_stack_size__)   ? __resume_stack_size__  : 0x1000;

/* Specify the memory areas */
MEMORY
{
  m_interrupts          (RX)   : ORIGIN = 0x00000000, LENGTH = 0x00000040
  m_ram					(RWX)  : ORIGIN = 0x00000040, LENGTH = 64M - 0x40 - 0x10000
  ram_noncache          (RWX)  : ORIGIN = 0x03FF0000, LENGTH = 0x10000
  ISYS1                 (xrw)  : ORIGIN = 0xf8008000, LENGTH = 0x00008000
}
/* Define output sections */
SECTIONS
{
  . = 0;
  /* The startup code goes first into DDR RAM */
  .interrupts :
  {
    __VECTOR_TABLE = .;
    . = ALIGN(4);
    KEEP(*(.isr_vector))     /* Startup code */
    . = ALIGN(4);
  } > m_interrupts

  /* The program code and other data goes into DDR RAM */
  .text :
  {
    . = ALIGN(4);
    *(.text)                 /* .text sections (code) */
    *(.text*)                /* .text* sections (code) */
    *(.rodata)               /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)              /* .rodata* sections (constants, strings, etc.) */
    *(.glue_7)               /* glue arm to thumb code */
    *(.glue_7t)              /* glue thumb to arm code */
    *(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
  } > m_ram

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > m_ram

  .ram_noncache(NOLOAD) : AT(0x03FF0000)
  {
    __RAM_NON_CACHE_START = .;
	*(.NON_CACHE);
  } >ram_noncache

  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > m_ram

 .ctors :
  {
    __CTOR_LIST__ = .;
    /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that it
       doesn't matter which directory crtbegin.o
       is in.  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin?.o(.ctors))
    /* We don't want to include the .ctor section from
       from the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */
    KEEP (*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    __CTOR_END__ = .;
  } > m_ram

  .dtors :
  {
    __DTOR_LIST__ = .;
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    __DTOR_END__ = .;
  } > m_ram

  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } > m_ram

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } > m_ram

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } > m_ram

  __etext = .;    /* define a global symbol at end of code */
  __NDATA_ROM = .; /* Symbol is used by startup for data initialization */

  .ncache : /*AT(__NDATA_ROM) ALIGN(1024*1024)*/
  {
    __noncachedata_start__ = .;   /* create a global symbol at ncache data start */
    *(NonCacheable)
    . = ALIGN(4);
    __noncachedata_end__ = .;     /* define a global symbol at ncache data end */
  } > m_ram

  __DATA_ROM = __NDATA_ROM + SIZEOF(.ncache); /* Symbol is used by startup for data initialization */

  .data : /*AT(__DATA_ROM) ALIGN(1024*1024)*/
  {
    __DATA_RAM = .;
    __data_start__ = .;           /* create a global symbol at data start */
    *(.data)                      /* .data sections */
    *(.data*)                     /* .data* sections */
    KEEP(*(.jcr*))
    . = ALIGN(4);
    __data_end__ = .;             /* define a global symbol at data end */
  } > m_ram

  /* define a global symbol at start of cacheable data */
  __DATA_END = __DATA_ROM + SIZEOF(.data);

  /*****************************
*** change system speed *****  ming
*****************************/
	. = ALIGN(4);

    /* define iRAM variable loaction to iRAM */
      . = 0xf8008000;
      isys1_section_start = .;
      isys1_section_size = 32K;

      .data_iram : AT (__data_end__ )
      {
        *drv_l2_power_save.o(.text* .data* .bss*)

      }>ISYS1


	.ISYS1 : AT (__data_end__ + SIZEOF (.data_iram) )
	{
		. = ALIGN(4);
		isys1_image_base = .;

		*(.isys1_set)
		. = ALIGN(4);
		_eisys1 = .;
		PROVIDE(_eisys1 = .);
	} >ISYS1

	/* isys1 code size */
	isys1_load_base = LOADADDR(.ISYS1);
	isys1_size = _eisys1 - isys1_image_base;
	isys1_load_End = isys1_load_base + isys1_size;
/************************************************************************************/
  .uninit LOADADDR(.ISYS1)+ SIZEOF (.ISYS1) (NOLOAD) :

  /* .uninit (NOLOAD) : //AT (__data_end__)*/
  {
    . = ALIGN(4);
    /* Global symbol at start of uninit */
    __uninit_start = .;
    *(.uninit*)
    . = ALIGN(4);
    /* Global symbol at end of uninit */
    __uninit_end__ = .;
    PROVIDE (euninit = .);
  } > m_ram

  /* Uninitialized data section */
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    . = ALIGN(4);
    __START_BSS = .;
    __bss_start__ = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > m_ram

  .stack :
  {
    . = ALIGN(8);
    __stack_end__ = .;
    . += CSTACK_SIZE;
    . += FSTACK_SIZE;
    . += ISTACK_SIZE;
    __END_STACK = .;
  } > m_ram

  /* Initializes stack on the end of block */

  __IStackEnd = __stack_end__;
  __IStackTop = __IStackEnd + ISTACK_SIZE;
  __FStackEnd = __IStackTop;
  __FStackTop = __FStackEnd + FSTACK_SIZE;
  __CStackEnd = __FStackTop;
  __CStackTop = __CStackEnd + CSTACK_SIZE;

  .ARM.attributes 0 : { *(.ARM.attributes) }
}


/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);       /* end of RAM */
__heap_end__ = ORIGIN(DTCM) + LENGTH(DTCM);  /* end of DTCM */

/* Flash disk regions */
__app_start__	= ORIGIN(APP);
__appi_start__	= ORIGIN(APPI);
__app_end__	= ORIGIN(APP) + LENGTH(APP);
__conf_start__	= ORIGIN(CONF);
__conf_end__	= ORIGIN(CONF) + LENGTH(CONF);

/* Specify the memory areas */
MEMORY
{
    IRAM (xrw)  : ORIGIN = 0x00000000, LENGTH = 16k
    DTCM (xrw)  : ORIGIN = 0x20000000, LENGTH = 64k
    RAM (xrw)   : ORIGIN = 0x20010000, LENGTH = 192k

    BOOT (xr)	: ORIGIN = 0x00200000, LENGTH = 16k
    CONFI (rw)	: ORIGIN = 0x00204000, LENGTH = 48k
    CONF (rw)	: ORIGIN = 0x08004000, LENGTH = 48k
    LOADER (xr)	: ORIGIN = 0x00210000, LENGTH = 64k
    APPI (xr)	: ORIGIN = 0x00220000, LENGTH = 384k
    APP (xr)	: ORIGIN = 0x08020000, LENGTH = 384k
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >BOOT

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >LOADER

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >LOADER

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >LOADER
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >LOADER

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >LOADER
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >LOADER
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >LOADER

  /* Code segments in ITCM RAM */
  _siiram = LOADADDR(.iram);
  .iram :
  {
    . = ALIGN(4);
    _siram = .;
    *(.iram)
    *(.iram*)

    . = ALIGN(4);
    _eiram = .;
  } >IRAM AT >LOADER

  /* Special RAM sections */
  .dtcm (NOLOAD) :
  {
    . = ALIGN(4);
    *(.dtcm)
    *(.dtcm*)

    . = ALIGN(4);
    __heap_start__ = .;
  } >DTCM

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    /* Special program code inside RAM */
    *(.ram)
    *(.ram*)

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM AT >LOADER


  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}

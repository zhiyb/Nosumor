/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = 0x20040000;       /* end of RAM */
__heap_end__ = 0x20010000;  /* end of DTCM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x400;      /* required amount of heap  */
_Min_Stack_Size = 0x1000; /* required amount of stack */

/* Flash disk regions */
__app_start__	= ORIGIN(APP);
__app_end__	= ORIGIN(APP) + LENGTH(APP);
__conf_start__	= ORIGIN(CONF);
__conf_end__	= ORIGIN(CONF) + LENGTH(CONF);

/* Specify the memory areas */
MEMORY
{
    IRAM (xrw)  : ORIGIN = 0x00000000, LENGTH = 16k
    DTCM (xrw)  : ORIGIN = 0x20000000, LENGTH = 64k
    RAM (xrw)   : ORIGIN = 0x20010000, LENGTH = 192k

    BOOT (xr)	: ORIGIN = 0x08000000, LENGTH = 16k
    CONF (rw)	: ORIGIN = 0x08004000, LENGTH = 48k
    LOADER (xr)	: ORIGIN = 0x08010000, LENGTH = 64k
    APP (xr)	: ORIGIN = 0x08020000, LENGTH = 384k
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into APP */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >APP

  /* The program code and other data goes into APP */
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
  } >APP

  /* Constant data goes into APP */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >APP

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >APP
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >APP

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >APP
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >APP
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >APP

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
  } >IRAM AT >APP

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
  } >RAM AT >APP

  
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

  /* User_stack section, used to check that there is enough RAM left */
  ._user_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* User_heap section, used to check that there is enough RAM left */
  ._user_heap :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = ALIGN(8);
  } >DTCM

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}

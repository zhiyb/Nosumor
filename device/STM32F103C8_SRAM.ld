/* Linker script for STM32F103C8 Device with 64KByte FLASH, 20KByte RAM */

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = 0x20005000;    /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_min_heap_size = 0;      /* required amount of heap  */
_min_stack_size = 0x100; /* required amount of stack */

_flash_origin = 0x08000000;
_flash_size = 64k;
_flash_bl_size = 2k;
_flash_sram_size = 20k;
_flash_data_size = _flash_size - _flash_bl_size - _flash_sram_size;
_flash_sram_origin = _flash_origin + _flash_bl_size;
_flash_data_origin = _flash_origin + _flash_bl_size + _flash_sram_size;

_sram_origin = 0x20000000;
_sram_size = 20k;

/* Specify the memory areas */
MEMORY
{
  FLASH_BL (rx)		: ORIGIN = _flash_origin,	LENGTH = _flash_bl_size
  FLASH_SRAM (rx)	: ORIGIN = _flash_sram_origin,	LENGTH = _flash_sram_size
  FLASH_DATA (rx)	: ORIGIN = _flash_data_origin,	LENGTH = _flash_data_size

  SRAM (xrw)		: ORIGIN = _sram_origin,	LENGTH = _sram_size

  /* 16-bit memory locations mapped to 32-bit bus */
  USBRAM (xrw)		: ORIGIN = 0x40006000,		LENGTH = 1k
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
  } >SRAM

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
  } >SRAM

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >SRAM

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >SRAM
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >SRAM

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >SRAM
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >SRAM
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >SRAM

  /* used by the startup to initialize data */
  /*_sidata = LOADADDR(.data);*/

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    *(.got)
    *(.got*)

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >SRAM

  
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
  } >SRAM

  /* User_heap_stack section, used to check that there is enough RAM left */
  /*._user_heap_stack :
  {
    . = ALIGN(4);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . += _min_heap_size;
    . += _min_stack_size;
    . = ALIGN(4);
  } >RAM*/

  .usbtable :
  {
    _susbram = .;
    . = ALIGN(16);
    *(.usbtable)

    . = ALIGN(4);
    *(.usbram)
    *(.usbram*)

    . = ALIGN(4);
    _eusbram = .;
  } >USBRAM

  .flash_sram :
  {
    . = ALIGN(4);
    _sflash_sram = .;
    . += _flash_sram_size;
    . = ALIGN(4);
    _eflash_sram = .;
  } > FLASH_SRAM

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}



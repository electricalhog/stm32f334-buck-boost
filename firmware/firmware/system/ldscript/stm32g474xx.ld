/*
******************************************************************************
**
**  File: stm32g474xx.ld
**  Target: STM32G474RE (Nucleo-G474RE)
**
*****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)    : ORIGIN = 0x08000000, LENGTH = 512K
  RAM (xrw)     : ORIGIN = 0x20000000, LENGTH = 128K
  CCMRAM (xrw)  : ORIGIN = 0x10000000, LENGTH = 32K
}

SECTIONS
{
  .isr_vector :
  {
    KEEP(*(.isr_vector))
  } > FLASH

  .text :
  {
    *(.text*)
    *(.rodata*)
    KEEP(*(.init))
    KEEP(*(.fini))
    KEEP(*(.eh_frame*))
  } > FLASH

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH

  __exidx_start = .;
  .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } > FLASH
  __exidx_end = .;

  /* Initialized data section */
  _sidata = LOADADDR(.data);

  .data : AT (_sidata)
  {
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM

  /* Uninitialized data section */
  .bss :
  {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  /* CCMRAM section for fast access data */
  .ccmram (NOLOAD) :
  {
    *(.ccmram)
  } > CCMRAM

  /* Heap and stack */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + 0x2000; /* 8KB stack */
    PROVIDE ( _estack = ORIGIN(RAM) + LENGTH(RAM) );
  } > RAM
}
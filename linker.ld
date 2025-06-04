ENTRY(_start)

SECTIONS {
  . = 0x80000;
  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss : { *(.bss*) *(COMMON) }

  . = ALIGN(16);
  _stack_top = . + 0x4000;
}

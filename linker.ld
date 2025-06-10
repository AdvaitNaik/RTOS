ENTRY(_start)

SECTIONS {
  . = 0x80000;
  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss : { *(.bss*) *(COMMON) }

  . = ALIGN(4096);
  __heap_start = .;
  . = . + 0x10000; /* 64KB heap */
  __heap_end = .;

  . = ALIGN(16);
  _stack_top = . + 0x4000; /* 16 KB stack */
}

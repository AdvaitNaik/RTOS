# RTOS
main.S      - Bootloader
linker.ld   - Linker Script
    .text → code
    .rodata → read-only data  
    .data → initialized global variables
    .bss → uninitialized global variables
main.c      - Task Setup
schedular   - 
# RTOS

- `start.S`      - Bootloader
- `linker.ld`   - Linker script defining memory layout
  - `.text`  : code
  - `.rodata`: read-only constants
  - `.data`  : initialized globals
  - `.bss`   : zero-initialized globals
- `main.c`      - Kernel and task setup
- `scheduler.c` - Round-robin task switching
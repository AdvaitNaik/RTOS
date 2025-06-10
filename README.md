# RTOS

- `start.S`      - Bootloader
- `linker.ld`   - Linker script defining memory layout
  - `.text`  : code
  - `.rodata`: read-only constants
  - `.data`  : initialized globals
  - `.bss`   : zero-initialized globals
- `main.c`      - Kernel and task setup
- `scheduler.c` - Round-robin task switching
- `task.c` - Task management
- `timer.c` - ARM Generic Timer
- `irq.c` - Handle Timer Interrupts
- `mm.c` - Memory Manager

```
-> kernel_main() 
-> timer_init(), task_create()
-> scheduler_run()
          ↓
  Timer fires interrupt
          ↓
  IRQ triggers context switch
          ↓
  Next task is resumed automatically
```
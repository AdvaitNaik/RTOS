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
- `mutex.c` - Software lock that ensures only one task accesses a shared resource at a time
- `semaphore.c`- Counter used to control access to shared resources by multiple tasks
- `syscalls.c` - Controlled, software-triggered exceptions used by tasks to request OS service

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

- memory like a notebook, and each page is 4 KB in size.
- reserved 16 pages = 64 KB of memory to use for tasks.
```
#define PAGE_SIZE 4096
#define MAX_PAGES 16
char page_bitmap[MAX_PAGES];  // 0 = free, 1 = used
char *heap_base = 0x90000;    // address for heap start

[mm_alloc_page] Page 0 allocated at 0x90000
[mm_alloc_page] Page 1 allocated at 0x91000
[mm_alloc_page] Page 2 allocated at 0x92000
[mm_free_page]  Page 2 freed
[mm_alloc_page] Page 2 re-allocated at 0x92000

page_bitmap = [1, 1, 1, 0, 0, ..., 0]
                      ↑
              Page 2 reused

HIGH ADDR
+-------------------+  ← top of stack (SP)
| entry function PC |
| dummy LR          |
| saved x19-x30     |
+-------------------+
LOW ADDR
```

```
task1.c    → syscall(SYSCALL_YIELD)
           → mov x8, #0
           → svc #0
 ↓
vectors.S  → el1_sync
           → syscall_dispatch(x2)
 ↓
dispatcher → calls scheduler_run()
           → context_switch to next task
```

Schedular
 Priority & preemption
 Time slice

Memory
 MMU (Virtual address & protection)
 MPU (protection)
 None (No virtual address, no protection)
 Static vs Dynamic

Sync
 Mutex/ Semaphore/ rwlock
 STI / CLI

Software Interrupt
 App to trap in scheduler
 App privilege escalation to run in kernel mode (syscall)

Hardware Interrupt
 tickISR


"send a message" from OS via the UART or serial communications circuit. 
UART stands for Universal Asynchronous Receiver/Transmitter and 
it's a very old and fairly simple interface that uses just two wires to communicate between two devices.
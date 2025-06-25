#include "syscall.h"
#include "scheduler.h"
#include "mm.h"
#include "task.h"
#include <stdint.h>

extern int current_task;

uint64_t
syscall_dispatch(int syscall_num)
{
    switch (syscall_num) {
        case SYSCALL_YIELD:
            scheduler_run();
            break;
        case SYSCALL_ALLOC:
            return (uint64_t) mm_alloc_page();
        case SYSCALL_GETPID:
            return (uint64_t) current_task;
        default:
            return -1;
    }
    return 0;
}
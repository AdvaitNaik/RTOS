#include "task.h"
#include <stdint.h>

// Each task has 4kB private stack
#define TASK_STACK_SIZE (4 * 1024)
#define MAX_TASKS 4

typedef struct {
    uint64_t *stack_base;
    uint64_t *stack_pc;
    void (*entry)(void);
} Task;

static Task tasks[MAX_TASKS];
static int task_count = 0;

extern 
void 
context_switch(uint64_t **prev_pc, uint64_t *pc);

void 
task_create(void (*entry)(void))
{
    Task *tsk = &tasks[task_count];
    tsk->stack_base = (uint64_t*) (0x80000 + 0x10000 * (task_count + 1));
    tsk->stack_pc = tsk->stack_base + TASK_STACK_SIZE / sizeof(uint64_t) - 1;

    // simulate the expected state of the stack so the task can “resume” and start executing entry.
    *(--tsk->stack_pc) = (uint64_t) entry; //pc
    *(--tsk->stack_pc) = 0x0;

    // pre-fills 30 registers (x0 to x29) with 0 — setting up a clean initial context.
    for (int i = 0; i < 30; i++) *(--tsk->stack_pc) = 0;

    tsk->entry = entry;
    task_count++;
}

// Getter for task count
int 
task_get_count(void)
{
    return task_count;
}

// Yielding (ending a task)
void
task_yield(void)
{
    asm volatile ("ret");
}

void
task_switch_to(int id)
{
    static int current = -1;
    // static uint64_t *prev_pc = 0;

    if (current == -1)
    {
        current = id;
        uint64_t* initial_pc = tasks[id].stack_pc;
        asm volatile ("mov sp, %0" :: "r"(initial_pc));
        tasks[id].entry();
    } 
    else
    {
        int prev = current;
        current = id;
        context_switch(&tasks[prev].stack_pc, tasks[id].stack_pc);
    } 

}
#include "scheduler.h"
#include "task.h"
#define MAX_TASKS 4

// static void (*tasks[MAX_TASKS])();
static int current_task = -1;
static int task_count = 0;

void 
scheduler_init(void) 
{
    task_count = 0;
}

// Round-robin scheduling, runs each task in order
// When one yields, it continues from the next one
void 
scheduler_run(void)
{
    while(1) 
    {
        current_task = (current_task + 1) % task_count;
        // tasks[current_task]();
        task_switch_to(current_task);
    }
}

// [VERSION 1]
// Add task function pointer to the task array.
// void 
// scheduler_add_task(void (*task)())
// {
//     if (task_count < MAX_TASKS)
//     {
//         tasks[task_count++] = task;
//     }
// }

// [VERSION 1]
// trigger a context switch
// void 
// preempt()
// {
//     return;
// }
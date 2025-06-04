#include "scheduler.h"
#define MAX_TASKS 4

static void (*tasks[MAX_TASKS])();
static int current_task = -1;
static int task_count = 0;

void scheduler_init() 
{
    task_count = 0;
}

// Add task function pointer to the task array.
void scheduler_add_task(void (*task)())
{
    if (task_count < MAX_TASKS)
    {
        tasks[task_count++] = task;
    }
}

// Round-robin scheduling, runs each task in order
// When one yields, it continues from the next one
void scheduler_run()
{
    while(1) 
    {
        current_task = (current_task + 1) % task_count;
        tasks[current_task]();
    }
}

// trigger a context switch
void preempt()
{
    return;
}
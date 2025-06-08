#include <stdint.h>
#include "scheduler.h"
#include "task.h"

void 
task_1(void)
{
    while(1)
    {
        // adv_printf("Task 1 running\n");
        // wait
        for (volatile int i = 0; i < 1000; i++);
        // return control back to schedular
        // preempt();
        task_yield();
    }
}

void 
task_2(void)
{
    while(1)
    {
        // adv_printf("Task 2 running\n");
        // wait
        for (volatile int i = 0; i < 1000; i++);
        // return control back to schedular
        // preempt();
        task_yield();
    }
}

void 
kernel_main(void) 
{
    // adv_init();
    // adv_printf("\nRTOS advanaik...\n")
    scheduler_init();
    // scheduler_add_task(task_1);
    // scheduler_add_task(task_2);
    task_create(task_1);
    task_create(task_2);
    scheduler_run();
}
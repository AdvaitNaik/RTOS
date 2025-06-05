#include <stdint.h>
#include "scheduler.h"

void task_1()
{
    while(1)
    {
        // adv_printf("Task 1 running\n");
        // wait
        for (volatile int i = 0; i < 1000; i++);
        // return control back to schedular
        preempt();
    }
}

void task_2()
{
    while(1)
    {
        // adv_printf("Task 2 running\n");
        // wait
        for (volatile int i = 0; i < 1000; i++);
        // return control back to schedular
        preempt();
    }
}

void kernel_main() 
{
    // adv_init();
    // adv_printf("\nRTOS advanaik...\n")
    scheduler_init();
    scheduler_add_task(task_1);
    scheduler_add_task(task_2);
    scheduler_run();
}
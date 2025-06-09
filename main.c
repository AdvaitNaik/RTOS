#include <stdint.h>
#include "scheduler.h"
#include "task.h"
#include "timer.h"
#include "irq.h"

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
    irq_init();
    timer_init();

    scheduler_init();
    // scheduler_add_task(task_1);
    // scheduler_add_task(task_2);
    task_create(task_1);
    task_create(task_2);

    // Enable IRQs (msr daifclr, #2)
    enable_interrupts();
    scheduler_run();
}
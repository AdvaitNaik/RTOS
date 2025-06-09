#include "irq.h"
#include "timer.h"
#include "scheduler.h"

void
irq_handler(void)
{
    timer_init();
    scheduler_run();
}

void
irq_init(void)
{
    extern void vectors(void);
    __asm__ volatile("msr vbar_el1, %0" :: "r"(&vectors));
}

// Enables IRQs by clearing the I-bit in DAIF (ARM’s interrupt mask register)
void 
enable_interrupts(void) 
{
    __asm__ volatile("msr daifclr, #2");
}
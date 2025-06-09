#include "timer.h"
#include <stdint.h>

#define TIMER_INTERVAL 50000 // Ticks, ~5 ms depending on system clock

// cntp_tval_el0: time until next IRQ
// cntp_ctl_el0: enable timer

void 
timer_init(void) 
{
    __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(TIMER_INTERVAL));
    __asm__ volatile("msr cntp_ctl_el0, %0" :: "r"(1));
}

void 
timer_reset(void) 
{
    __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(TIMER_INTERVAL));
}
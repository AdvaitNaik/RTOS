#ifndef SCHEDULER_H
#define SCHEDULER_H

void scheduler_init();
void scheduler_add_task(void (*task)());
void scheduler_run();
void preempt();

#endif

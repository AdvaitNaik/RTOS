#ifndef SCHEDULER_H
#define SCHEDULER_H

static int current_task = -1;
static int task_count = 0;

void scheduler_init(void);
void scheduler_run(void);
// void scheduler_run();
// void scheduler_add_task(void (*task)());
// void preempt();
int scheduler_get_current_task(void);

#endif

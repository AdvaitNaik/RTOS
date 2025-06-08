#ifndef TASK_H
#define TASK_H

void task_create(void (*entry)(void));
int task_get_count(void);
void task_switch_to(int id);
void task_yield(void);

#endif
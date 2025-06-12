#ifndef TASK_H
#define TASK_H

typedef enum {
    TASK_READY,
    TASK_BLOCKED
} task_state_t;

void task_create(void (*entry)(void));
int task_get_count(void);
void task_switch_to(int id);
void task_yield(void);
void task_block(int id);
void task_unblock_all(void);

#endif
#ifndef TASK_H
#define TASK_H

// Each task has 4kB private stack
#define TASK_STACK_SIZE (4 * 1024)
#define MAX_TASKS 4

typedef enum {
    TASK_READY,
    TASK_BLOCKED
} task_state_t;

task_state_t task_states[MAX_TASKS];

void task_create(void (*entry)(void));
int task_get_count(void);
void task_switch_to(int id);
void task_yield(void);
void task_block(int id);
void task_unblock_all(void);

#endif
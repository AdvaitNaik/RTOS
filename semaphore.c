#include "semaphore.h"
#include "scheduler.h"
#include "task.h"

void
semaphore_init(semaphore_t* s, int initial)
{
    s->count = initial;
    s->head = s->tail = 0;
    for (int i = 0; i < MAX_WAITING_TASKS; i++) s->waiting[i] = -1;
}

void 
semaphore_wait(semaphore_t* s)
{
    if (s->count > 0)
    {
        s->count--;
    } else {
        int id = scheduler_get_current_task();
        s->waiting[s->tail] = id;
        s->tail = (s->tail + 1) % MAX_WAITING_TASKS;
        task_block(id);
        scheduler_run();
    }
}

void
semaphore_signal(semaphore_t* s)
{
    if (s->head != s->tail)
    {
        int id = s->waiting[s->head];
        s->head = (s->head + 1) % MAX_WAITING_TASKS;
        task_unblock(id);
    } else {
        s->count++;
    }
}
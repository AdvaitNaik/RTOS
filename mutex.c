#include "mutex.h"
#include "scheduler.h"
#include "task.h"

void 
mutex_init(mutex_t *m)
{
    m->locked = 0;
    m->owner = -1;
}

void 
mutex_lock(mutex_t *m)
{
    while(1)
    {
        if (m->locked == 0)
        {
            // If lock is free → acquire it
            m->locked = 1;
            m->owner = scheduler_get_current_task();
            return;
        } else {
            // If already locked → block the current task and switch to next
            task_block(schedular_get_current_task());
            schedular_run();
        }
    }
}

void 
mutex_unlock(mutex_t *m)
{
    // owner can release the mutex
    if (m->owner == scheduler_get_current_task())
    {
        m->locked = 0;
        m->owner = -1;
        // Unblocks all waiting tasks (improve with queue)
        task_unblock_all();
    }
}
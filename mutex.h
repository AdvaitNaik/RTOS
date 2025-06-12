#ifndef MUTEX_H
#define MUTEX_H

typedef struct {
    int locked;     // 	1 if the mutex is locked, 0 if free
    int owner;      // 	Task ID of the task that currently owns the lock
} mutex_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

#endif
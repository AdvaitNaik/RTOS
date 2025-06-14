#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#define MAX_WAITING_TASKS 4

typedef struct {
    int count;
    int waiting[MAX_WAITING_TASKS];
    int head;
    int tail;
} semaphore_t;

void semaphore_init(semaphore_t* s, int initial);
void semaphore_wait(semaphore_t* s);
void semaphore_signal(semaphore_t* s);

#endif
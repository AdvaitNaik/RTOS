#ifndef SYSCALL_H
#define SYSCALL_H

#define SYSCALL_YIELD   0
#define SYSCALL_ALLOC   1
#define SYSCALL_GETPID  2

void syscall(int syscall_num);

#endif
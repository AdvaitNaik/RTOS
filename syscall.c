#include "syscall.h"

void
syscall(int syscall_num)
{
    __asm__ volatile("mov x8, %0\n\tsvc #0" :: "r"(syscall_num) : "x8");
}

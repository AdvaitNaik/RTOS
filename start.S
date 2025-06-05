.section .text
.global _start

_start:
    // loads the address of _stack_top into register x0
    // and sets the stack pointer to that address
    ldr x0, =_stack_top
    mov sp, x0

    // calls the kernel_main() function
    bl kernel_main

hang:
    b hang

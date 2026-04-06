    .section .text
    .global _start

_start:
    mov x19, x0
    mov x20, x1

    ldr x0, =_stack_top
    mov sp, x0

    ldr x2, =__bss_start
    ldr x3, =__bss_end
clear_bss:
    cmp x2, x3
    b.hs bss_done
    str xzr, [x2], #8
    b clear_bss
bss_done:
    mov x0, x19
    mov x1, x20
    bl kernel_main

hang:
    b hang

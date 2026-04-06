    .section .text
    .global _start

_start:
    /* Firmware: DTB pointer is usually in x0 (Linux AArch64); some setups use x1. */
    mov x19, x0
    mov x20, x1

    /* Disable MMU & caches for early HW access (safe for testing) */
    mrs x0, SCTLR_EL1
    bic x0, x0, #(1 << 0)    /* clear M (MMU) */
    bic x0, x0, #(1 << 2)    /* clear C (D-cache) */
    bic x0, x0, #(1 << 12)   /* clear I (I-cache) */
    msr SCTLR_EL1, x0
    isb

    /* set stack pointer */
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

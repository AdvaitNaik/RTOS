    .section .text
    .global _start

_start:
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

    /* Move dtb pointer (in x1) to x0 (so kernel_main gets it as first arg), then call */
    mov x0, x1
    bl kernel_main

hang:
    b hang

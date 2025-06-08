.global context_switch
.type context_switch, %function

// void context_switch(uint64_t **prev_pc, uint64_t *pc)
// pointer to the old task's stack pointer (x0)
// new task's stack pointer (x1)
context_switch:
    // Save x19–x30 to current task stack
    // Store all callee-saved registers (x19–x30) into memory pointed to by x0 (the old task's stack).
    stp x19, x20, [x0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]
    stp x29, x30, [x0, #80]

    // Save stack pointer to *prev_pc
    mov x2, sp
    str x2, [x0]

    // Load new stack pointer
    mov sp, x1

    // Restore x19–x30 from new task stack
    ldp x19, x20, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x27, x28, [sp], #16
    ldp x29, x30, [sp], #16
    ret
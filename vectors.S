.section .vectors, "a"
.global vectors
.type vectors, %object

.align 11
vectors:
    .org 0x000
    b el1_sync
    .org 0x800
    b el1_irq

el1_sync:
    // get syscall number from x8
    mrs x0, elr_el1        // store return addr
    mrs x1, esr_el1        // get exception info
    mov x2, x8             // syscall number
    bl syscall_dispatch    // call C dispatcher
    msr elr_el1, x0        // restore return addr

el1_irq:
    bl irq_handler
    eret
    
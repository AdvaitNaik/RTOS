.section .vectors, "a"
.global vectors
.type vectors, %object

.align 11
vectors:
    .org 0x000
    b el1_irq

el1_irq:
    bl irq_handler
    eret
    
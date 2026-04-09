    .section .text
    .global _start

_start:
    mov x19, x0
    mov x20, x1

    /*
     * Firmware often enables the MMU. Uncached device accesses to
     * 0x1_07d5xxxx must be mapped; if not, the first MMIO load in C faults
     * and the ACT LED never toggles.
     */
    mrs x3, CurrentEL
    cmp x3, #0x8
    b.eq 1f
    /* EL1 */
    mrs x1, sctlr_el1
    bic x1, x1, #1
    msr sctlr_el1, x1
    b 2f
1:  /* EL2 */
    mrs x1, sctlr_el2
    bic x1, x1, #1
    msr sctlr_el2, x1
2:
    isb

    /*
     * ACT LED on gio_aon GPIO 9 (active-low): bank0 ODEN/DATA/IODIR offsets 0/4/8.
     * Base CPU phys 0x107d517c00 — if this lights at power-on, the image is running
     * and MMIO is reachable before any C code.
     */
    movz x4, #0x7c00
    movk x4, #0x7d51, lsl #16
    movk x4, #0x10, lsl #32
    movz w7, #0x200                 /* GPIO 9 */
    /* ODEN: clear bit 9 → not open-drain */
    ldr w5, [x4]
    bic w5, w5, w7
    str w5, [x4]
    /* IODIR: clear bit 9 → output */
    add x6, x4, #8
    ldr w5, [x6]
    bic w5, w5, w7
    str w5, [x6]
    /* DATA: clear bit 9 → drive low → LED on */
    add x6, x4, #4
    ldr w5, [x6]
    bic w5, w5, w7
    str w5, [x6]

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

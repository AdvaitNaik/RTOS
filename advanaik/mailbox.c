#include "mailbox.h"

/* 16-byte aligned mailbox buffer */
volatile uint32_t mbox[36] __attribute__((aligned(16)));

/* MMIO helpers */
static inline void mmio_write(uintptr_t reg, uint32_t val) {
    *(volatile uint32_t *)reg = val;
}
static inline uint32_t mmio_read(uintptr_t reg) {
    return *(volatile uint32_t *)reg;
}

enum {
    VIDEOCORE_MBOX = (PERIPHERAL_BASE + 0x0000B880UL),
    MBOX_READ      = (VIDEOCORE_MBOX + 0x0),
    MBOX_STATUS    = (VIDEOCORE_MBOX + 0x18),
    MBOX_WRITE     = (VIDEOCORE_MBOX + 0x20),
    MBOX_FULL      = 0x80000000,
    MBOX_EMPTY     = 0x40000000,
    MBOX_RESPONSE  = 0x80000000
};

/* Write address of mbox to GPU mailbox and wait for response */
int mbox_call(uint32_t ch)
{
    uintptr_t addr = (uintptr_t)mbox;
    uint32_t data = (uint32_t)((addr & ~0xF) | (ch & 0xF));

    /* wait until mailbox not full */
    while (mmio_read(MBOX_STATUS) & MBOX_FULL) ;

    /* write address to mailbox */
    mmio_write(MBOX_WRITE, data);

    /* wait for a response */
    while (1) {
        /* wait for something to be available */
        while (mmio_read(MBOX_STATUS) & MBOX_EMPTY) ;
        uint32_t resp = mmio_read(MBOX_READ);
        if (resp == data) {
            /* success if mbox[1] == RESPONSE flag */
            return (mbox[1] == MBOX_RESPONSE);
        }
    }

    return 0;
}

#include <stdint.h>
#include "framebuffer.h"

/* Mailbox base address for Pi 5 (BCM2712) */
#define MBOX_BASE 0x10700000
#define MBOX_TAGS_CH 8

#define MBOX_FULL  0x80000000
#define MBOX_EMPTY 0x40000000

volatile uint32_t *framebuffer = 0;
uint32_t fb_width = 1024;
uint32_t fb_height = 768;
uint32_t fb_pitch = 0;

static volatile uint32_t *mailbox = (uint32_t *)MBOX_BASE;

static inline void mmio_write(uint64_t reg, uint32_t val) {
    *(volatile uint32_t *)reg = val;
}

static inline uint32_t mmio_read(uint64_t reg) {
    return *(volatile uint32_t *)reg;
}

/* Mailbox property interface message buffer */
static volatile uint32_t __attribute__((aligned(16))) mbox[36];

int framebuffer_init(void) {
    mbox[0] = 35 * 4;          // size
    mbox[1] = 0;               // request code

    mbox[2] = 0x48003;         // set physical size
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = fb_width;        // width
    mbox[6] = fb_height;       // height

    mbox[7] = 0x48004;         // set virtual size
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = fb_width;
    mbox[11] = fb_height;

    mbox[12] = 0x48005;        // set depth
    mbox[13] = 4;
    mbox[14] = 4;
    mbox[15] = 32;             // 32-bit color

    mbox[16] = 0x40001;        // allocate framebuffer
    mbox[17] = 8;
    mbox[18] = 8;
    mbox[19] = 16;             // alignment 16 bytes
    mbox[20] = 0;

    mbox[21] = 0;              // end tag

    uint32_t addr = (uint32_t)((uint64_t)mbox & ~0xF) | MBOX_TAGS_CH;

    // Wait for mailbox not full
    while (mmio_read((uint64_t)mailbox + 0x18) & MBOX_FULL);
    mmio_write((uint64_t)mailbox + 0x20, addr);

    // Wait for response
    uint32_t resp;
    do {
        while (mmio_read((uint64_t)mailbox + 0x18) & MBOX_EMPTY);
        resp = mmio_read((uint64_t)mailbox + 0x00);
    } while ((resp & 0xF) != MBOX_TAGS_CH);

    if (mbox[20] == 0) return -1;

    framebuffer = (volatile uint32_t *)((uint64_t)mbox[19] & 0x3FFFFFFF);
    fb_pitch = fb_width * 4;
    return 0;
}

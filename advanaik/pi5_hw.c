/* BCM2712 (Pi 5) MMIO — bcm2712.dtsi: mailbox@7c013880, soc at 0x1_0000_0000 + offset */
#include <stdint.h>
#include "pi5_hw.h"

#define MMIO_SOC UINT64_C(0x0000001000000000)

#define MBOX_BASE (MMIO_SOC + UINT64_C(0x07c013880))

/* Mailbox property tags (ARM → VideoCore) */
#define MBOX_TAG_SET_PHY_WH  0x00048003u
#define MBOX_TAG_SET_VIRT_WH 0x00048004u
#define MBOX_TAG_SET_DEPTH   0x00048005u
#define MBOX_TAG_SET_PIXEL_ORDER 0x00048006u
#define MBOX_TAG_ALLOCATE_BUFFER 0x00040004u
#define MBOX_TAG_GET_PITCH   0x00040008u

static inline void mmio_w(uint64_t reg, uint32_t v)
{
    *(volatile uint32_t *)reg = v;
}

static inline uint32_t mmio_r(uint64_t reg)
{
    return *(volatile uint32_t *)reg;
}

enum {
    MBOX_STATUS = 0x18,
    MBOX_READ   = 0x00,
    MBOX_WRITE  = 0x20,
    MBOX_FULL   = 0x80000000U,
    MBOX_EMPTY  = 0x40000000U,
    MBOX_CH_PROP = 8,
    MBOX_RESP    = 0x80000000U
};

static int mbox_call_prop(volatile uint32_t *mbox)
{
    uintptr_t mb = (uintptr_t)mbox;
    uint32_t a = ((uint32_t)(mb & ~0xFULL)) | (uint32_t)MBOX_CH_PROP;

    while (mmio_r(MBOX_BASE + MBOX_STATUS) & MBOX_FULL)
        ;
    mmio_w(MBOX_BASE + MBOX_WRITE, a);

    for (;;) {
        while (mmio_r(MBOX_BASE + MBOX_STATUS) & MBOX_EMPTY)
            ;
        uint32_t r = mmio_r(MBOX_BASE + MBOX_READ);
        if (r == a)
            return (mbox[1] & MBOX_RESP) != 0;
    }
}

static uintptr_t gpu_bus_to_cpu_phys(uint32_t bus)
{
    /* Legacy GPU "bus" addresses: low 30 bits often map to ARM RAM (Pi 1–4). */
    uintptr_t lo = (uintptr_t)(bus & 0x3FFFFFFFU);
    if (lo == 0 && bus != 0)
        return (uintptr_t)bus;
    return lo;
}

int pi5_mailbox_framebuffer(uint32_t w, uint32_t h, uintptr_t *out_cpu, uint32_t *out_pitch)
{
    static volatile uint32_t __attribute__((aligned(16))) m[64];

    m[0] = 27 * 4;
    m[1] = 0;
    m[2] = MBOX_TAG_SET_PHY_WH;
    m[3] = 8;
    m[4] = 8;
    m[5] = w;
    m[6] = h;
    m[7] = MBOX_TAG_SET_VIRT_WH;
    m[8] = 8;
    m[9] = 8;
    m[10] = w;
    m[11] = h;
    m[12] = MBOX_TAG_SET_DEPTH;
    m[13] = 4;
    m[14] = 4;
    m[15] = 32;
    m[16] = MBOX_TAG_SET_PIXEL_ORDER;
    m[17] = 4;
    m[18] = 4;
    m[19] = 1;
    m[20] = MBOX_TAG_ALLOCATE_BUFFER;
    m[21] = 8;
    m[22] = 8;
    m[23] = 16;
    m[24] = 0;
    m[25] = 0;
    m[26] = 0;

    if (!mbox_call_prop(m))
        return -1;
    if (m[24] == 0 || m[25] == 0)
        return -2;

    m[0] = 8 * 4;
    m[1] = 0;
    m[2] = MBOX_TAG_GET_PITCH;
    m[3] = 4;
    m[4] = 0;
    m[5] = 0;
    m[6] = 0;

    if (!mbox_call_prop(m))
        return -3;
    if ((m[4] & MBOX_RESP) == 0 || (m[4] & 0x7FFFFFFFu) < 4)
        return -4;
    if (m[5] == 0)
        return -5;

    *out_cpu = gpu_bus_to_cpu_phys(m[24]);
    *out_pitch = m[5];
    return 0;
}

void pi5_framebuffer_fill_bands(uintptr_t base, uint32_t pitch, uint32_t w, uint32_t h)
{
    /* XRGB8888; exact R/B order depends on pixel-order tag (1 = common firmware default). */
    uint32_t top = 0x00e04020u;
    uint32_t bot = 0x002060c0u;

    for (uint32_t y = 0; y < h; y++) {
        volatile uint32_t *row =
            (volatile uint32_t *)(void *)(base + (uintptr_t)y * (uintptr_t)pitch);
        uint32_t c = (y < h / 2u) ? top : bot;
        for (uint32_t x = 0; x < w; x++)
            row[x] = c;
    }
    asm volatile("dmb sy" ::: "memory");
}

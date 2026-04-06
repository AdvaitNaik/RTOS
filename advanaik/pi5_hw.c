/* BCM2712 (Pi 5) MMIO from Linux bcm2712.dtsi: soc bus at 0x1_0000_0000 + child offset */
#include <stdint.h>
#include "pi5_hw.h"

#define MMIO_SOC UINT64_C(0x0000001000000000)

#define MBOX_BASE   (MMIO_SOC + UINT64_C(0x07c013880))
#define UART10_BASE (MMIO_SOC + UINT64_C(0x07d001000))

static inline void mmio_w(uint64_t reg, uint32_t v)
{
    *(volatile uint32_t *)reg = v;
}

static inline uint32_t mmio_r(uint64_t reg)
{
    return *(volatile uint32_t *)reg;
}

void uart10_putc(int c)
{
    if (c == '\n')
        uart10_putc('\r');
    while (mmio_r(UART10_BASE + 0x18) & (1U << 5))
        ;
    mmio_w(UART10_BASE + 0x00, (uint32_t)(unsigned char)c);
}

void uart10_puts(const char *s)
{
    while (*s)
        uart10_putc((unsigned char)*s++);
}

void uart10_hex32(uint32_t x)
{
    for (int i = 7; i >= 0; i--) {
        unsigned n = (x >> (i * 4)) & 0xF;
        uart10_putc(n < 10 ? (char)('0' + n) : (char)('a' + (n - 10)));
    }
}

void uart10_hex64(uint64_t x)
{
    uart10_hex32((uint32_t)(x >> 32));
    uart10_hex32((uint32_t)x);
}

void uart10_init(void)
{
    /* PL011 @ 9216000 Hz (clk_uart in bcm2712.dtsi) -> 115200: divisor 5 */
    mmio_w(UART10_BASE + 0x30, 0);
    mmio_w(UART10_BASE + 0x24, 5);
    mmio_w(UART10_BASE + 0x28, 0);
    mmio_w(UART10_BASE + 0x2C, 0x70);
    mmio_w(UART10_BASE + 0x38, 0);
    mmio_w(UART10_BASE + 0x30, 0x301);
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
            return mbox[1] == MBOX_RESP;
    }
}

static uintptr_t gpu_bus_to_cpu_phys(uint32_t bus)
{
    /* VideoCore "bus" addresses: low 30 bits are often ARM-visible RAM (Pi 1–4).
       0x4xxxxxxx aliases would become 0 if we only mask — fall back to the raw value. */
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
    m[2] = 0x48003;
    m[3] = 8;
    m[4] = 8;
    m[5] = w;
    m[6] = h;
    m[7] = 0x48004;
    m[8] = 8;
    m[9] = 8;
    m[10] = w;
    m[11] = h;
    m[12] = 0x48005;
    m[13] = 4;
    m[14] = 4;
    m[15] = 32;
    m[16] = 0x48006;
    m[17] = 4;
    m[18] = 4;
    m[19] = 1;
    m[20] = 0x40001;
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

    m[0] = 7 * 4;
    m[1] = 0;
    m[2] = 0x40008;
    m[3] = 4;
    m[4] = 4;
    m[5] = 0;
    m[6] = 0;

    if (!mbox_call_prop(m))
        return -3;
    if (m[4] != (MBOX_RESP | 4U) || m[5] == 0)
        return -4;

    *out_cpu = gpu_bus_to_cpu_phys(m[24]);
    *out_pitch = m[5];
    return 0;
}

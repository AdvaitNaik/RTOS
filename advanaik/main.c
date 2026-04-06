/*
 * Minimal Pi 5 (BCM2712) bring-up: blink the green ACT LED.
 * Hardware: gio_aon GPIO 9 ("2712_STAT_LED" in the devicetree), active-low.
 * MMIO: Linux bcm2712.dtsi — gio_aon @ bus offset 0x7d517c00, SOC at 0x1_0000_0000.
 *
 * No framebuffer, no mailbox, no UART — if the LED blinks, your kernel8.img is running.
 */
#include <stdint.h>

#define MMIO_SOC UINT64_C(0x0000001000000000)
#define GIO_AON  (MMIO_SOC + UINT64_C(0x07d517c00))

/* gpio-brcmstb: one bank = 8 x u32 registers (see linux/drivers/gpio/gpio-brcmstb.c) */
#define GIO_BANK_STRIDE 32u
#define GIO_ODEN(b)   ((uint64_t)(b)*GIO_BANK_STRIDE + 0)
#define GIO_DATA(b)   ((uint64_t)(b)*GIO_BANK_STRIDE + 4)
#define GIO_IODIR(b)  ((uint64_t)(b)*GIO_BANK_STRIDE + 8)

#define ACT_GPIO 9u
#define ACT_BANK 0u

static inline void reg_w(uint64_t addr, uint32_t v)
{
    *(volatile uint32_t *)addr = v;
}

static inline uint32_t reg_r(uint64_t addr)
{
    return *(volatile uint32_t *)addr;
}

static void spin_delay(void)
{
    for (volatile uint64_t i = 0; i < 12000000ULL; i++)
        asm volatile("" ::: "memory");
}

void kernel_main(uint64_t reg0, uint64_t reg1)
{
    (void)reg0;
    (void)reg1;

    const uint64_t base = GIO_AON;

    /* 0 = output, 1 = input (brcmstb GPIO dir register) */
    uint32_t iodir = reg_r(base + GIO_IODIR(ACT_BANK));
    iodir &= ~(1u << ACT_GPIO);
    reg_w(base + GIO_IODIR(ACT_BANK), iodir);

    /* ODEN bit set = output driver disabled (tri-state); clear to drive pin */
    uint32_t oden = reg_r(base + GIO_ODEN(ACT_BANK));
    oden &= ~(1u << ACT_GPIO);
    reg_w(base + GIO_ODEN(ACT_BANK), oden);

    /* Active-low LED: 0 = on, 1 = off */
    for (;;) {
        uint32_t d = reg_r(base + GIO_DATA(ACT_BANK));
        d ^= (1u << ACT_GPIO);
        reg_w(base + GIO_DATA(ACT_BANK), d);
        spin_delay();
    }
}

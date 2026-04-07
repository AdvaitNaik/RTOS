/*
 * Phase 3 — Pi 5 (BCM2712) bring-up
 *
 * Phase 1: LED blink (hard-coded gio_aon MMIO).
 * Phase 2: validate DTB pointer + magic; delays via BCM system timer (1 MHz CLO).
 * Phase 3: walk the Flattened Device Tree (FDT) to find gio_aon "reg" and compute CPU
 *          physical base (0x1_0000_0000 + offset per soc ranges). Fallback if not found.
 *
 * LED: ACT on gio_aon GPIO 9, active-low.
 */
#include <stdint.h>
#include "fdt.h"

#define GIO_AON_REG_ADDR 0x7d517c00u
#define GIO_AON_REG_SIZE 0x40u

#define MMIO_SOC_FALLBACK (UINT64_C(0x0000001000000000) + UINT64_C(0x07d517c00))

/*
 * BCM2835-style system timer (bcm2712.dtsi: timer@7c003000, clock-frequency = 1 MHz).
 * CLO = free-running microsecond counter — reliable on Pi bare metal.
 * (ARM CNTPCT_EL0 + CNTFRQ_EL0 can disagree with real tick rate in some EL1/firmware setups,
 *  which makes delay_ms() feel "stuck slow" no matter how small you set ms.)
 */
#define SYSTIMER_BASE (UINT64_C(0x0000001000000000) + UINT64_C(0x07c003000))
#define SYSTIMER_CLO  (SYSTIMER_BASE + 0x04u)

#define GIO_BANK_STRIDE 32u
#define GIO_ODEN(b)   ((uint64_t)(b)*GIO_BANK_STRIDE + 0)
#define GIO_DATA(b)   ((uint64_t)(b)*GIO_BANK_STRIDE + 4)
#define GIO_IODIR(b)  ((uint64_t)(b)*GIO_BANK_STRIDE + 8)

#define ACT_GPIO 9u
#define ACT_BANK 0u

void *g_firmware_dtb;

/* MMIO base for gio_aon; set from DTB or left at fallback. */
static uint64_t g_gpio_mmio = MMIO_SOC_FALLBACK;

static inline void reg_w(uint64_t addr, uint32_t v)
{
    *(volatile uint32_t *)addr = v;
}

static inline uint32_t reg_r(uint64_t addr)
{
    return *(volatile uint32_t *)addr;
}

static int dtb_pointer_ok(uint64_t p)
{
    if (p == 0)
        return 0;
    if (p & 7ULL)
        return 0;
    return fdt_blob_magic_ok((const void *)(uintptr_t)p);
}

static void *pick_dtb(uint64_t reg0, uint64_t reg1)
{
    if (dtb_pointer_ok(reg0))
        return (void *)(uintptr_t)reg0;
    if (dtb_pointer_ok(reg1))
        return (void *)(uintptr_t)reg1;
    return 0;
}

/* --- Delays: BCM system timer CLO (1 tick = 1 microsecond) --- */

static inline uint32_t systimer_clo_read(void)
{
    return *(volatile uint32_t *)SYSTIMER_CLO;
}

static void delay_ms(uint32_t ms)
{
    uint32_t us = ms * 1000u;
    uint32_t start = systimer_clo_read();
    while ((uint32_t)(systimer_clo_read() - start) < us)
        ;
}

/* --- LED --- */

static void led_init(void)
{
    uint32_t iodir = reg_r(g_gpio_mmio + GIO_IODIR(ACT_BANK));
    iodir &= ~(1u << ACT_GPIO);
    reg_w(g_gpio_mmio + GIO_IODIR(ACT_BANK), iodir);

    uint32_t oden = reg_r(g_gpio_mmio + GIO_ODEN(ACT_BANK));
    oden &= ~(1u << ACT_GPIO);
    reg_w(g_gpio_mmio + GIO_ODEN(ACT_BANK), oden);
}

static void led_toggle(void)
{
    uint32_t d = reg_r(g_gpio_mmio + GIO_DATA(ACT_BANK));
    d ^= (1u << ACT_GPIO);
    reg_w(g_gpio_mmio + GIO_DATA(ACT_BANK), d);
}

static void panic_blink_loop(void)
{
    for (;;) {
        led_toggle();
        for (volatile uint64_t i = 0; i < 8000000ULL; i++)
            asm volatile("" ::: "memory");
    }
}

void kernel_main(uint64_t reg0, uint64_t reg1)
{
    void *dtb = pick_dtb(reg0, reg1);
    if (!dtb) {
        g_gpio_mmio = MMIO_SOC_FALLBACK;
        led_init();
        panic_blink_loop();
    }

    g_firmware_dtb = dtb;

    /*
     * Pi 5 bcm2712.dtsi: gio_aon reg = <0x7d517c00 0x40>; CPU map = 0x1_0000_0000 + offset.
     * If this fails (unexpected DTB), keep MMIO_SOC_FALLBACK from static init.
     */
    uint64_t mmio = g_gpio_mmio;
    if (fdt_find_soc_mmio_32(dtb, GIO_AON_REG_ADDR, GIO_AON_REG_SIZE, &mmio) == 0)
        g_gpio_mmio = mmio;

    led_init();

    /* Half-period in ms (try 50 for a fast blink). */
    for (;;) {
        led_toggle();
        delay_ms(250);
    }
}

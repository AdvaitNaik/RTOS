/*
 * Phase 2 — Pi 5 (BCM2712) bring-up
 *
 * 1) Device tree (DTB): firmware usually passes its physical address in x0 (and sometimes x1).
 *    We only check the FDT magic (0xd00dfeed, big-endian in the blob). Full parsing comes later.
 *
 * 2) Time: ARM generic physical counter (CNTPCT_EL0) + frequency (CNTFRQ_EL0) for millisecond delays
 *    instead of a blind spin loop.
 *
 * LED: gio_aon GPIO 9, active-low (same as phase 1).
 */
#include <stdint.h>

#define MMIO_SOC UINT64_C(0x0000001000000000)
#define GIO_AON  (MMIO_SOC + UINT64_C(0x07d517c00))

#define GIO_BANK_STRIDE 32u
#define GIO_ODEN(b)   ((uint64_t)(b)*GIO_BANK_STRIDE + 0)
#define GIO_DATA(b)   ((uint64_t)(b)*GIO_BANK_STRIDE + 4)
#define GIO_IODIR(b)  ((uint64_t)(b)*GIO_BANK_STRIDE + 8)

#define ACT_GPIO 9u
#define ACT_BANK 0u

/* First 4 bytes of a valid flattened device tree blob (big-endian in the file) */
#define FDT_MAGIC 0xd00dfeedu

/* Phase 3+ will parse this blob instead of hard-coding MMIO bases. */
void *g_firmware_dtb;

static const uint64_t g_gio = GIO_AON;

static inline void reg_w(uint64_t addr, uint32_t v)
{
    *(volatile uint32_t *)addr = v;
}

static inline uint32_t reg_r(uint64_t addr)
{
    return *(volatile uint32_t *)addr;
}

/* Read big-endian 32-bit value using byte loads (safe alignment). */
static uint32_t be32_at(const void *p)
{
    const uint8_t *b = (const uint8_t *)p;
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) |
           (uint32_t)b[3];
}

static int dtb_pointer_ok(uint64_t p)
{
    if (p == 0)
        return 0;
    /* Linux boot requirements: DTB on 8-byte boundary */
    if (p & 7ULL)
        return 0;
    return be32_at((const void *)(uintptr_t)p) == FDT_MAGIC;
}

static void *pick_dtb(uint64_t reg0, uint64_t reg1)
{
    if (dtb_pointer_ok(reg0))
        return (void *)(uintptr_t)reg0;
    if (dtb_pointer_ok(reg1))
        return (void *)(uintptr_t)reg1;
    return 0;
}

/* --- ARM generic timer (EL1 physical counter) --- */

static inline uint64_t cntpct_el0_read(void)
{
    uint64_t v;
    asm volatile("mrs %0, cntpct_el0" : "=r"(v));
    return v;
}

static inline uint32_t cntfrq_el0_read(void)
{
    uint32_t v;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(v));
    return v;
}

/* Enable the EL1 physical timer counter (many firmwares already do this). */
static void arm_timer_enable(void)
{
    uint64_t ctl;
    asm volatile("mrs %0, cntp_ctl_el0" : "=r"(ctl));
    ctl |= 1ULL; /* EN */
    asm volatile("msr cntp_ctl_el0, %0" :: "r"(ctl));
    asm volatile("isb" ::: "memory");
}

/*
 * Busy-wait for ms milliseconds using the system counter.
 * Falls back to a coarse spin if CNTFRQ is zero (should not happen on Pi 5).
 */
static void delay_ms(uint32_t ms)
{
    uint32_t frq = cntfrq_el0_read();
    if (frq == 0) {
        for (volatile uint64_t i = 0; i < (uint64_t)ms * 200000ULL; i++)
            asm volatile("" ::: "memory");
        return;
    }

    uint64_t ticks = ((uint64_t)frq * (uint64_t)ms) / 1000ULL;
    if (ticks == 0)
        ticks = 1;

    uint64_t start = cntpct_el0_read();
    while (cntpct_el0_read() - start < ticks)
        ;
}

/* --- ACT LED (active low: clear bit = LED on, set bit = LED off) --- */

static void led_init(void)
{
    uint32_t iodir = reg_r(g_gio + GIO_IODIR(ACT_BANK));
    iodir &= ~(1u << ACT_GPIO);
    reg_w(g_gio + GIO_IODIR(ACT_BANK), iodir);

    uint32_t oden = reg_r(g_gio + GIO_ODEN(ACT_BANK));
    oden &= ~(1u << ACT_GPIO);
    reg_w(g_gio + GIO_ODEN(ACT_BANK), oden);
}

/* Drive line high (LED off) or low (LED on) for active-low LED */
static void led_set(int on)
{
    uint32_t d = reg_r(g_gio + GIO_DATA(ACT_BANK));
    if (on)
        d &= ~(1u << ACT_GPIO);
    else
        d |= (1u << ACT_GPIO);
    reg_w(g_gio + GIO_DATA(ACT_BANK), d);
}

static void led_toggle(void)
{
    uint32_t d = reg_r(g_gio + GIO_DATA(ACT_BANK));
    d ^= (1u << ACT_GPIO);
    reg_w(g_gio + GIO_DATA(ACT_BANK), d);
}

/* Slow blink if we could not find a DTB (diagnostic) */
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
    led_init();
    arm_timer_enable();

    void *dtb = pick_dtb(reg0, reg1);
    if (!dtb) {
        /* No valid DTB: very slow blink (spin delay only — timer may still work but keep pattern obvious) */
        panic_blink_loop();
    }

    /*
     * DTB magic OK: steady blink using real milliseconds.
     * 250 ms per half-cycle ≈ 2 Hz full blink.
     */
    g_firmware_dtb = dtb;

    for (;;) {
        led_toggle();
        delay_ms(250);
    }
}

/*
 * Pi 5 (BCM2712) bring-up
 *
 * GPIO: hard-coded gio_aon MMIO (bcm2712.dtsi reg 0x7d517c00 → CPU 0x1_0000_0000 + offset).
 * Delays: BCM system timer CLO @ 0x7c003000 (1 MHz).
 * Display: VideoCore mailbox — 32 bpp framebuffer test pattern.
 *
 * LED: ACT on gio_aon GPIO 9, active-low.
 */
#include <stdint.h>
#include "pi5_hw.h"

/* gio_aon — same address you were already using as FDT fallback */
#define GIO_AON_MMIO_BASE (UINT64_C(0x0000001000000000) + UINT64_C(0x07d517c00))

/*
 * BCM2835-style system timer (bcm2712.dtsi: timer@7c003000, clock-frequency = 1 MHz).
 */
#define SYSTIMER_BASE (UINT64_C(0x0000001000000000) + UINT64_C(0x07c003000))
#define SYSTIMER_CLO  (SYSTIMER_BASE + 0x04u)

#define GIO_BANK_STRIDE 32u
#define GIO_ODEN(b)   ((uint64_t)(b)*GIO_BANK_STRIDE + 0)
#define GIO_DATA(b)   ((uint64_t)(b)*GIO_BANK_STRIDE + 4)
#define GIO_IODIR(b)  ((uint64_t)(b)*GIO_BANK_STRIDE + 8)

#define ACT_GPIO 9u
#define ACT_BANK 0u

volatile uint64_t g_gio_mmio_phys;

volatile int g_fb_init_rc;
volatile uintptr_t g_fb_base;
volatile uint32_t g_fb_pitch;

#define FB_WIDTH  640u
#define FB_HEIGHT 480u

static uint64_t g_gpio_mmio = GIO_AON_MMIO_BASE;

static inline void reg_w(uint64_t addr, uint32_t v)
{
    *(volatile uint32_t *)addr = v;
}

static inline uint32_t reg_r(uint64_t addr)
{
    return *(volatile uint32_t *)addr;
}

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

/* Active-low ACT: on != 0 → LED lit (GPIO driven low). */
static void led_set(int on)
{
    uint32_t d = reg_r(g_gpio_mmio + GIO_DATA(ACT_BANK));
    if (on)
        d &= ~(1u << ACT_GPIO);
    else
        d |= (1u << ACT_GPIO);
    reg_w(g_gpio_mmio + GIO_DATA(ACT_BANK), d);
}

void kernel_main(uint64_t reg0, uint64_t reg1)
{
    (void)reg0;
    (void)reg1;

    g_gio_mmio_phys = g_gpio_mmio;

    led_init();
    /* Known starting level so failure-path led_set(1) is unambiguous (active-low). */
    led_set(0);

    g_fb_init_rc = -99;
    g_fb_base = 0;
    g_fb_pitch = 0;
    {
        uintptr_t fb = 0;
        uint32_t pitch = 0;
        int fbrc = pi5_mailbox_framebuffer(FB_WIDTH, FB_HEIGHT, &fb, &pitch);
        g_fb_init_rc = fbrc;
        g_fb_base = fb;
        g_fb_pitch = pitch;
        if (fbrc == 0 && fb != 0 && pitch >= FB_WIDTH * 4u)
            pi5_framebuffer_fill_bands(fb, pitch, FB_WIDTH, FB_HEIGHT);
    }

    /*
     * LED (no UART): fast blink = mailbox OK; solid on = mailbox failed (see g_fb_init_rc).
     */
    if (g_fb_init_rc != 0) {
        led_set(1);
        for (;;)
            asm volatile("wfe");
    }

    for (;;) {
        led_toggle();
        delay_ms(100);
    }
}

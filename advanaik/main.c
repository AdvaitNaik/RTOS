// main.c -- FDT simple-framebuffer (if present) or Pi 5 mailbox framebuffer
#include <stdint.h>
#include <stddef.h>
#include "pi5_hw.h"

/* mini string functions for bare-metal */
static size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

/* FDT tokens (big-endian in DTB) */
#define FDT_MAGIC 0xd00dfeedU
#define FDT_BEGIN_NODE 0x1
#define FDT_END_NODE   0x2
#define FDT_PROP       0x3
#define FDT_NOP        0x4
#define FDT_END        0x9

/* little helper to read big-endian 32-bit from DTB memory */
static inline uint32_t be32(const void *p) {
    uint32_t v = *(const uint32_t *)p;
    /* convert from big-endian stored in DTB to CPU little-endian */
    return __builtin_bswap32(v);
}

/* globals to use for drawing */
static volatile uint8_t *fb = NULL;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_stride = 0; /* bytes per line */
static uint32_t fb_format = 0; /* not used extensively; assume a8b8g8r8 or similar */

/* tiny 8x8 test-glyph (just one glyph used repeatedly) */
static const uint8_t glyph[8] = {
    0x3C,0x42,0xA9,0x85,0xA9,0x91,0x42,0x3C
};

static void put_pixel(int x, int y, uint32_t color)
{
    if (!fb) return;
    if ((unsigned)x >= fb_width || (unsigned)y >= fb_height) return;
    uint32_t px = (uint32_t)x;
    uint32_t line = (uint32_t)y;
    uint32_t *row = (uint32_t *)(fb + line * fb_stride);
    row[px] = color;
}

static void fill_screen(uint32_t color)
{
    if (!fb) return;
    for (uint32_t y = 0; y < fb_height; ++y) {
        uint32_t *row = (uint32_t *)(fb + y * fb_stride);
        for (uint32_t x = 0; x < fb_width; ++x) row[x] = color;
    }
}

static void draw_char(int x, int y, uint32_t fg, uint32_t bg)
{
    for (int r = 0; r < 8; ++r) {
        uint8_t bits = glyph[r];
        for (int c = 0; c < 8; ++c) {
            uint32_t color = (bits & (1 << (7 - c))) ? fg : bg;
            put_pixel(x + c, y + r, color);
        }
    }
}

static void draw_string(int x, int y, const char *s, uint32_t fg, uint32_t bg)
{
    int cx = x;
    while (*s) {
        draw_char(cx, y, fg, bg);
        cx += 8;
        ++s;
    }
}

/* Helper: advance pointer in struct block, align to 4 bytes */
static const uint8_t *align4(const uint8_t *p, const uint8_t *base)
{
    uintptr_t off = p - base;
    uintptr_t a = (off + 3) & ~3U;
    return base + a;
}

/* Parse FDT header and find simple-framebuffer node under /chosen or elsewhere
   dtb: pointer to the DTB (passed by firmware in x0)
   Returns 0 on success and sets fb, fb_* globals; non-zero on failure.
*/
int fdt_find_framebuffer(void *dtb)
{
    if (!dtb) return -1;
    const uint8_t *base = (const uint8_t *)dtb;
    const uint32_t magic = be32(base + 0);
    if (magic != FDT_MAGIC) return -2;

    uint32_t off_dt_struct = be32(base + 8);
    uint32_t off_dt_strings = be32(base + 12);
    // uint32_t mem_rsvmap = be32(base + 4);
    const uint8_t *structp = base + off_dt_struct;
    const uint8_t *strings = base + off_dt_strings;

    const uint8_t *p = structp;
    int depth = 0;
    int in_chosen = 0;

    /* state for node discovery */
    int found = 0;
    uintptr_t fb_reg_addr = 0;
    uint64_t fb_reg_size = 0;
    uint32_t width = 0, height = 0, stride = 0;
    char format_buf[32] = {0};

    while (1) {
        uint32_t token = be32(p); p += 4;
        if (token == FDT_BEGIN_NODE) {
            /* node name: null-terminated string, padded to 4 */
            const char *name = (const char *)p;
            size_t nlen = strlen(name);
            p = (const uint8_t *)align4((const uint8_t *)(p + nlen + 1), structp);
            depth++;
            /* check if we've entered /chosen */
            if (depth == 1 && strcmp(name, "chosen") == 0) in_chosen = 1;
            /* continue scanning */
        } else if (token == FDT_END_NODE) {
            depth--;
            if (depth == 0) in_chosen = 0;
        } else if (token == FDT_PROP) {
            uint32_t prop_len = be32(p); p += 4;
            uint32_t nameoff = be32(p); p += 4;
            const char *propname = (const char *)(strings + nameoff);
            const uint8_t *propval = p;
            /* handle property */
            if (strcmp(propname, "compatible") == 0) {
                /* compatible is a set of nul-terminated strings; check if any == "simple-framebuffer" */
                const uint8_t *q = propval;
                size_t left = prop_len;
                int is_simplefb = 0;
                while (left > 0) {
                    const char *s = (const char *)q;
                    if (strcmp(s, "simple-framebuffer") == 0) { is_simplefb = 1; break; }
                    size_t sl = strlen(s) + 1;
                    q += sl; left -= sl;
                }
                if (is_simplefb) {
                    /* Found a simple-framebuffer compatible node:
                       We now must read other props (reg,width,height,stride,format) from the same node.
                       We'll collect them by scanning subsequent properties until end of node.
                    */
                    /* scan remaining properties inside node */
                    const uint8_t *pp = p + prop_len;
                    pp = (const uint8_t *)align4(pp, structp);
                    /* read props until FDT_END_NODE */
                    const uint8_t *scan = pp;
                    while (1) {
                        uint32_t tk = be32(scan); /* peek token */
                        if (tk == FDT_PROP) {
                            /* read the property */
                            uint32_t pl = be32(scan + 4);
                            uint32_t no = be32(scan + 8);
                            const char *pn = (const char *)(strings + no);
                            const uint8_t *pv = scan + 12;
                            if (strcmp(pn, "reg") == 0) {
                                /* reg could be 1 or 2 cells (addr/size). try both */
                                if (pl >= 8) {
                                    /* treat as 64-bit pair: high/low? We'll interpret as big-endian cells */
                                    uint32_t ahi = be32(pv);
                                    uint32_t alo = be32(pv + 4);
                                    uint32_t shi = 0, slo = 0;
                                    if (pl >= 16) {
                                        shi = be32(pv + 8);
                                        slo = be32(pv + 12);
                                    }
                                    /* If high is zero, use low 32 bits; else combine to 64 */
                                    if (ahi == 0) {
                                        fb_reg_addr = (uintptr_t)alo;
                                        fb_reg_size = (uint64_t) ( (pl >= 16) ? (((uint64_t)shi << 32) | slo) : 0 );
                                    } else {
                                        /* 64-bit address */
                                        fb_reg_addr = ((uintptr_t)ahi << 32) | alo;
                                        fb_reg_size = 0;
                                    }
                                } else if (pl >= 4) {
                                    uint32_t a = be32(pv);
                                    fb_reg_addr = (uintptr_t)a;
                                }
                            } else if (strcmp(pn, "width") == 0 && pl >= 4) {
                                width = be32(pv);
                            } else if (strcmp(pn, "height") == 0 && pl >= 4) {
                                height = be32(pv);
                            } else if (strcmp(pn, "stride") == 0 && pl >= 4) {
                                stride = be32(pv);
                            } else if (strcmp(pn, "format") == 0) {
                                size_t copylen = pl < sizeof(format_buf)-1 ? pl : sizeof(format_buf)-1;
                                for (size_t i=0;i<copylen;i++) format_buf[i] = pv[i];
                                format_buf[copylen] = 0;
                            }
                            /* advance to next prop */
                            const uint8_t *next = pv + pl;
                            scan = (const uint8_t *)align4(next, structp);
                            continue;
                        } else if (tk == FDT_END_NODE) {
                            /* end of node */
                            break;
                        } else if (tk == FDT_NOP) {
                            scan += 4;
                            continue;
                        } else {
                            /* some other token => break and bail */
                            break;
                        }
                    } /* end scan props */
                    /* accept node only if we have reg and width/height or stride */
                    if (fb_reg_addr != 0 && ( (width && height) || stride )) {
                        /* got framebuffer info */
                        fb = (volatile uint8_t *)(uintptr_t)fb_reg_addr;
                        fb_width = width ? width : (uint32_t)(fb_reg_size ? (fb_reg_size / 4) : 0);
                        fb_height = height ? height : 0;
                        fb_stride = stride ? stride : (fb_width * 4);
                        return 0;
                    }
                }
            }
            /* advance p past property data */
            p = (const uint8_t *)align4(p + prop_len, structp);
        } else if (token == FDT_NOP) {
            /* nothing */
        } else if (token == FDT_END) {
            break;
        } else {
            /* unknown token: bail */
            break;
        }
    }
    return -3; /* not found */
}

static int dtb_is_valid(void *dtb)
{
    if (!dtb)
        return 0;
    const uint8_t *b = (const uint8_t *)dtb;
    return be32(b + 0) == FDT_MAGIC;
}

/* reg0/reg1: values from firmware (typically DTB in x0, zero in x1) */
void kernel_main(uint64_t reg0, uint64_t reg1)
{
    uart10_init();
    uart10_puts("\n\nadvanaik Pi5 bringup\n");

    void *dtb = NULL;
    if (dtb_is_valid((void *)(uintptr_t)reg0))
        dtb = (void *)(uintptr_t)reg0;
    else if (dtb_is_valid((void *)(uintptr_t)reg1))
        dtb = (void *)(uintptr_t)reg1;
    else {
        uart10_puts("DTB magic fail x0=");
        uart10_hex64(reg0);
        uart10_puts(" x1=");
        uart10_hex64(reg1);
        uart10_puts("\n");
    }

    int have_fb = 0;
    if (dtb && fdt_find_framebuffer(dtb) == 0) {
        have_fb = 1;
        uart10_puts("FDT simple-framebuffer OK\n");
    } else if (dtb)
        uart10_puts("FDT: no simple-framebuffer (using mailbox)\n");

    if (!have_fb) {
        uintptr_t pa;
        uint32_t pitch;
        int err = pi5_mailbox_framebuffer(640, 480, &pa, &pitch);
        if (err != 0) {
            uart10_puts("mailbox fb err=");
            uart10_hex32((uint32_t)err);
            uart10_puts("\n");
            while (1)
                asm volatile("wfe");
        }
        fb = (volatile uint8_t *)pa;
        fb_width = 640;
        fb_height = 480;
        fb_stride = pitch;
        uart10_puts("mailbox fb pa=");
        uart10_hex64(pa);
        uart10_puts(" pitch=");
        uart10_hex32(pitch);
        uart10_puts("\n");
    }

    fill_screen(0xFF0000FF);
    draw_string(40, 40, "HELLO RPI5", 0xFFFFFFFF, 0xFF000000);
    uart10_puts("draw done\n");

    while (1)
        asm volatile("wfe");
}

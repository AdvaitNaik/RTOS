/*
 * Minimal Flattened Device Tree (FDT) scanner — no libfdt, no malloc.
 * https://www.devicetree.org/specifications/
 */
#include <stddef.h>
#include <stdint.h>
#include "fdt.h"

enum {
    FDT_BEGIN_NODE = 1,
    FDT_END_NODE   = 2,
    FDT_PROP       = 3,
    FDT_NOP        = 4,
    FDT_END        = 9,
};

uint32_t fdt_be32(const void *p)
{
    const uint8_t *b = (const uint8_t *)p;
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) |
           (uint32_t)b[3];
}

int fdt_blob_magic_ok(const void *dtb)
{
    if (!dtb)
        return 0;
    return fdt_be32(dtb) == FDT_MAGIC;
}

static int streq(const char *a, const char *b)
{
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *a == *b;
}

static const uint8_t *align4_ptr(const uint8_t *p)
{
    uintptr_t x = (uintptr_t)p;
    return (const uint8_t *)((x + 3) & ~(uintptr_t)3);
}

/*
 * Match reg in forms seen on BCM2712 / Pi 5:
 *   <addr size>           — 8 bytes (#address-cells = 1, #size-cells = 1)
 *   <0 addr 0 size>       — 16 bytes (64-bit addr + 64-bit size in low words)
 */
static int reg_matches_soc_mmio(uint32_t plen, const uint8_t *pv, uint32_t reg_addr,
                                uint32_t reg_size, uint64_t *out_cpu_phys)
{
    if (plen >= 8) {
        uint32_t a = fdt_be32(pv);
        uint32_t s = fdt_be32(pv + 4);
        if (a == reg_addr && s == reg_size) {
            *out_cpu_phys = FDT_BCM2712_SOC_PHYS + (uint64_t)a;
            return 0;
        }
    }
    if (plen >= 16) {
        uint32_t a0 = fdt_be32(pv);
        uint32_t a1 = fdt_be32(pv + 4);
        uint32_t s0 = fdt_be32(pv + 8);
        uint32_t s1 = fdt_be32(pv + 12);
        if (a0 == 0 && a1 == reg_addr && s0 == 0 && s1 == reg_size) {
            *out_cpu_phys = FDT_BCM2712_SOC_PHYS + (uint64_t)a1;
            return 0;
        }
    }
    return -1;
}

int fdt_find_soc_mmio_32(const void *dtb, uint32_t reg_addr, uint32_t reg_size,
                         uint64_t *out_cpu_phys)
{
    if (!dtb || !out_cpu_phys)
        return -1;
    if (!fdt_blob_magic_ok(dtb))
        return -2;

    const uint8_t *base = (const uint8_t *)dtb;
    uint32_t totalsize = fdt_be32(base + 4);
    uint32_t off_struct = fdt_be32(base + 8);
    uint32_t off_strings = fdt_be32(base + 12);

    if (off_struct >= totalsize || off_strings >= totalsize)
        return -3;

    const uint8_t *str = base + off_strings;
    const uint8_t *p = base + off_struct;
    const uint8_t *end = base + totalsize;

    while (p + 4 <= end) {
        uint32_t token = fdt_be32(p);
        p += 4;

        if (token == FDT_BEGIN_NODE) {
            while (p < end && *p)
                p++;
            if (p >= end)
                return -4;
            p++;
            p = align4_ptr(p);
        } else if (token == FDT_END_NODE) {
            /* nothing */
        } else if (token == FDT_PROP) {
            if (p + 8 > end)
                return -5;
            uint32_t plen = fdt_be32(p);
            p += 4;
            uint32_t nameoff = fdt_be32(p);
            p += 4;
            /*
             * nameoff is relative to the strings block (off_dt_strings), not to base.
             * Comparing nameoff >= totalsize wrongly rejected valid properties and
             * could abort the walk early → FDT lookup always failed → solid LED ON.
             */
            if ((uintptr_t)str + (uintptr_t)nameoff >= (uintptr_t)base + totalsize)
                return -6;
            const char *pname = (const char *)(str + nameoff);
            const uint8_t *pv = p;
            if (p + plen > end)
                return -7;

            if (streq(pname, "reg") &&
                reg_matches_soc_mmio(plen, pv, reg_addr, reg_size, out_cpu_phys) == 0)
                return 0;

            p = pv + plen;
            p = align4_ptr(p);
        } else if (token == FDT_NOP) {
            /* nothing */
        } else if (token == FDT_END) {
            break;
        } else {
            return -8;
        }
    }

    return -9;
}

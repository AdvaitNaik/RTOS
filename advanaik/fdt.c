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

static const char *str_contains(const char *hay, const char *needle)
{
    if (!needle[0])
        return hay;
    for (; *hay; hay++) {
        const char *a = hay;
        const char *b = needle;
        while (*a && *b && *a == *b) {
            a++;
            b++;
        }
        if (!*b)
            return hay;
    }
    return 0;
}

static int blob_contains_ascii(const uint8_t *pv, uint32_t plen, const char *needle)
{
    uint32_t nlen = 0;
    while (needle[nlen])
        nlen++;
    if (plen < nlen)
        return 0;
    for (uint32_t i = 0; i + nlen <= plen; i++) {
        uint32_t j;
        for (j = 0; j < nlen; j++) {
            if (pv[i + j] != (uint8_t)needle[j])
                break;
        }
        if (j == nlen)
            return 1;
    }
    return 0;
}

static const uint8_t *align4_ptr(const uint8_t *p)
{
    uintptr_t x = (uintptr_t)p;
    return (const uint8_t *)((x + 3) & ~(uintptr_t)3);
}

/*
 * Parse reg at gio_aon: accept exact size or larger (firmware may round region up).
 * Two cells: <addr size> or four: <0 addr 0 size>.
 */
static int reg_parse_gio_aon(uint32_t plen, const uint8_t *pv, uint32_t reg_addr,
                             uint32_t reg_min_size, uint64_t *out_cpu_phys)
{
    if (plen >= 8) {
        uint32_t a = fdt_be32(pv);
        uint32_t s = fdt_be32(pv + 4);
        if (a == reg_addr && s >= reg_min_size) {
            *out_cpu_phys = FDT_BCM2712_SOC_PHYS + (uint64_t)a;
            return 0;
        }
    }
    if (plen >= 16) {
        uint32_t a0 = fdt_be32(pv);
        uint32_t a1 = fdt_be32(pv + 4);
        uint32_t s0 = fdt_be32(pv + 8);
        uint32_t s1 = fdt_be32(pv + 12);
        if (a0 == 0 && a1 == reg_addr && s0 == 0 && s1 >= reg_min_size) {
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

    int depth = 0;
    /* Depth of node we think is gio_aon (gpio@7d517c00 or compatible bcm7445-gpio). */
    int gio_hit_d = 0;
    int gio_by_name = 0;
    int gio_by_compat = 0;

    while (p + 4 <= end) {
        uint32_t token = fdt_be32(p);
        p += 4;

        if (token == FDT_BEGIN_NODE) {
            const char *name = (const char *)p;
            while (p < end && *p)
                p++;
            if (p >= end)
                return -4;
            p++;
            p = align4_ptr(p);

            depth++;
            gio_by_name = str_contains(name, "7d517c00") != 0;
            gio_by_compat = 0;
            if (gio_by_name)
                gio_hit_d = depth;
        } else if (token == FDT_END_NODE) {
            if (gio_hit_d == depth) {
                gio_hit_d = 0;
                gio_by_name = 0;
                gio_by_compat = 0;
            }
            if (depth > 0)
                depth--;
        } else if (token == FDT_PROP) {
            if (p + 8 > end)
                return -5;
            uint32_t plen = fdt_be32(p);
            p += 4;
            uint32_t nameoff = fdt_be32(p);
            p += 4;
            if ((uintptr_t)str + (uintptr_t)nameoff >= (uintptr_t)base + totalsize)
                return -6;
            const char *pname = (const char *)(str + nameoff);
            const uint8_t *pv = p;
            if (p + plen > end)
                return -7;

            if (streq(pname, "compatible") && depth > 0 &&
                blob_contains_ascii(pv, plen, "bcm7445-gpio")) {
                gio_by_compat = 1;
                gio_hit_d = depth;
            }

            if (streq(pname, "reg") && gio_hit_d > 0 && depth == gio_hit_d) {
                if (gio_by_name || gio_by_compat) {
                    if (reg_parse_gio_aon(plen, pv, reg_addr, reg_size, out_cpu_phys) == 0)
                        return 0;
                }
            }

            /* Fallback: any reg in tree with matching address (relaxed size). */
            if (streq(pname, "reg") &&
                reg_parse_gio_aon(plen, pv, reg_addr, reg_size, out_cpu_phys) == 0)
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

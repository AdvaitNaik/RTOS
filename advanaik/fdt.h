#ifndef FDT_H
#define FDT_H

#include <stdint.h>

#define FDT_MAGIC 0xd00dfeedu

/*
 * BCM2712: peripherals under /soc use a child bus; Linux dtsi maps child offset to CPU phys:
 *   cpu_phys = 0x1_0000_0000 + reg_addr
 * (see soc@107c000000 / ranges in bcm2712.dtsi).
 */
#define FDT_BCM2712_SOC_PHYS UINT64_C(0x0000001000000000)

uint32_t fdt_be32(const void *p);
int fdt_blob_magic_ok(const void *dtb);

/*
 * Scan the whole FDT for a "reg" property whose first two big-endian cells match
 * (reg_addr, reg_size). Used on Pi 5 for gio_aon: reg = <0x7d517c00 0x40>.
 * Returns 0 and sets *out_cpu_phys on success.
 */
int fdt_find_soc_mmio_32(const void *dtb, uint32_t reg_addr, uint32_t reg_size,
                         uint64_t *out_cpu_phys);

#endif

#ifndef PI5_HW_H
#define PI5_HW_H

#include <stdint.h>

/* Returns 0 on success; *out_cpu is CPU physical (MMU off); *out_pitch bytes per row. */
int pi5_mailbox_framebuffer(uint32_t w, uint32_t h, uintptr_t *out_cpu, uint32_t *out_pitch);

/* Solid horizontal bands (test pattern for 32 bpp, pitch may exceed width*4). */
void pi5_framebuffer_fill_bands(uintptr_t base, uint32_t pitch, uint32_t w, uint32_t h);

#endif

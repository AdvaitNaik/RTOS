#ifndef PI5_HW_H
#define PI5_HW_H

#include <stdint.h>

void uart10_init(void);
void uart10_putc(int c);
void uart10_puts(const char *s);
void uart10_hex32(uint32_t v);
void uart10_hex64(uint64_t v);

/* Returns 0 on success; *out_cpu is CPU physical (MMU off); *out_pitch bytes per row. */
int pi5_mailbox_framebuffer(uint32_t w, uint32_t h, uintptr_t *out_cpu, uint32_t *out_pitch);

#endif

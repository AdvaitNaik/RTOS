#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#pragma once
#include <stdint.h>

extern volatile uint32_t *framebuffer;
extern uint32_t fb_width, fb_height, fb_pitch;

int framebuffer_init(void);

#endif

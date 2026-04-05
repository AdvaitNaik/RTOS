#ifndef MAILBOX_H
#define MAILBOX_H

#include <stdint.h>

#define PERIPHERAL_BASE 0xFE000000UL

extern volatile uint32_t mbox[36];

enum {
    MBOX_REQUEST = 0,
    MBOX_CH_PROP = 8
};

enum {
    MBOX_TAG_SETPHYWH   = 0x48003,
    MBOX_TAG_SETVIRTWH  = 0x48004,
    MBOX_TAG_SETVIRTOFF = 0x48009,
    MBOX_TAG_SETDEPTH   = 0x48005,
    MBOX_TAG_SETPXLORDR = 0x48006,
    MBOX_TAG_GETFB      = 0x40001,
    MBOX_TAG_GETPITCH   = 0x40008,
    MBOX_TAG_LAST       = 0
};

int mbox_call(uint32_t ch);

#endif

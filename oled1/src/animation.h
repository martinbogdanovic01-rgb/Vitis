#ifndef ANIMATION_H
#define ANIMATION_H

#include "xil_types.h"

/* Startup animation shown once after boot */
void animation_startup(void);

/* Runtime display update */
void animation_update(
    const char* tilt,
    const char* rot,
    u16 a0, u16 a1,
    u16 a2, u16 a3
);

#endif

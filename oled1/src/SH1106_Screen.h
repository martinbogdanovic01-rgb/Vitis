#ifndef SH1106_SCREEN_H
#define SH1106_SCREEN_H

#include <stdio.h>
#include "I2Csrc/u8g2.h"
#include "xiicps.h"
#include "Defines.h"

void initDisplay();
uint8_t cb_gpio_SH1106(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t cb_HW_I2C_send(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#endif

#ifndef SH1106_SCREEN_H_
#define SH1106_SCREEN_H_

#include "I2Csrc/u8x8.h"
#include "I2Csrc/u8g2.h"
#include "xiicps.h"

void initDisplay();
void printDisplay(u8g2_uint_t x, u8g2_uint_t y, const char *str);
void printNew(u8g2_uint_t x, u8g2_uint_t y, const char *str);
uint8_t cb_HW_I2C_send(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t cb_gpio_SH1106(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#endif

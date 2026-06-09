#ifndef __GPIO_MO8__
#define __GPIO_MO8__

#include "xgpio.h"
#include "GDO0_interrupt.h"

extern XGpio GDO0_GPIO;

#define GDO0_INPUT_DEVICE_ID  XPAR_ARDUINO_ARDUINO_INTR_EN_PINS_2_3_DEVICE_ID
#define GDO0_CHANNEL          1

int Init_GDO0_GPIO(void);

#endif

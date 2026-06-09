#ifndef CONFIG_H
#define CONFIG_H

#include "xparameters.h"

#define PWM_PERIOD 100000

// Thresholds for speed levels
#define MOVE_DEADZONE_TH 0.10  // Below this: Stop
#define MOVE_FAST_TH     0.40  // Above this: Fast | Between Deadzone and this: Slow

#define AR6 1
#define AR7 2

#define GPIO_DEVICE_ID  XPAR_ARDUINO_ARDUINO_NO_INTR_PINS_DEVICE_ID
#define TMR0_DEVICE_ID  XPAR_TMRCTR_0_DEVICE_ID
#define TMR1_DEVICE_ID  XPAR_TMRCTR_1_DEVICE_ID

#endif

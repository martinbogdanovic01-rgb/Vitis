#ifndef DEFINES_H
#define DEFINES_H

#include "xparameters.h"

// --- Display ---
#define SlaveAddress 0x3C
#define IIC_SCLK_RATE 400000
#define UsedFont u8g2_font_6x10_tf

// --- System ---
#define NUMBER_OF_SENSORS   4
#define PWM_PERIOD          100000
#define SCU_TIMER_VALUE     33333333

// PWM Duty Cycles
#define PWM_DUTY_35_PERCENT 35000
#define PWM_DUTY_50_PERCENT 50000
#define PWM_DUTY_65_PERCENT 65000

// --- IDs ---
#define ARD_IO_NO_INTR_DEVICE_ID   XPAR_ARDUINO_ARDUINO_NO_INTR_PINS_DEVICE_ID
#define TMR0_DEVICE_ID             XPAR_TMRCTR_0_DEVICE_ID
#define TMR1_DEVICE_ID             XPAR_TMRCTR_1_DEVICE_ID
#define OVERCUR_INPUT_DEVICE_ID    XPAR_ARDUINO_ARDUINO_INTR_EN_PINS_2_3_DEVICE_ID
#define AR_INTC_GPIO_INTERRUPT_ID  XPAR_FABRIC_ARDUINO_ARDUINO_INTR_EN_PINS_2_3_IP2INTC_IRPT_INTR
#define AR_OVERCURRENT_INT         XGPIO_IR_CH1_MASK

// --- Pins ---
#define AR4 2
#define AR6 4
#define AR7 5

#endif

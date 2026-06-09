/*
 * File:   config.h
 * Author: Bianca Ion
 *
 * Created on 18 July 2022, 17:22
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "xparameters.h"
#define PB_FRQ 100000000 //100MHz
#define MAX_COUNT 0xFFFFFFFF

#define BIT0	0x01
#define BIT1	0x02
#define BIT2	0x04
#define BIT3	0x08
#define ledpin0 1
#define ledpin1 2
#define ledpin2 4
#define ledpin3 8
#define btnpin0 1
#define btnpin1 2
#define btnpin2 4
#define btnpin3 8
#define BTNS_BASEADDRESS   0x41220000
#define LEDS_BASEADDRESS   0x41240000

#define TMR0_BASEADDRESS   XPAR_TMRCTR_0_BASEADDR
#define TMR0_DEVICE_ID     XPAR_TMRCTR_0_DEVICE_ID
#define TMR1_DEVICE_ID     XPAR_TMRCTR_1_DEVICE_ID
#define TMR2_DEVICE_ID     XPAR_TMRCTR_2_DEVICE_ID

#define TMR0_INTC_INTERRUPT_ID   XPAR_FABRIC_USER_AXI_TIMER_0_INTERRUPT_INTR
#define TMR1_INTC_INTERRUPT_ID   XPAR_FABRIC_USER_AXI_TIMER_1_INTERRUPT_INTR
#define TMR2_INTC_INTERRUPT_ID   XPAR_FABRIC_USER_AXI_TIMER_2_INTERRUPT_INTR

#define BTNS_DEVICE_ID         XPAR_GPIO_0_DEVICE_ID //device ids for buttons and leds
#define LEDS_DEVICE_ID         XPAR_GPIO_1_DEVICE_ID
#define SWT_DEVICE_ID          XPAR_GPIO_3_DEVICE_ID
#define ARD_IO_INTR_DEVICE_ID  XPAR_ARDUINO_ARDUINO_INTR_EN_PINS_2_3_DEVICE_ID
#define ARD_IO_NO_INTR_DEVICE_ID XPAR_ARDUINO_ARDUINO_NO_INTR_PINS_DEVICE_ID
#define BTN_INTR                XGPIO_IR_CH1_MASK

#define UARTLITE_DEVICE_ID	XPAR_UARTLITE_0_DEVICE_ID

//the index of each arduino pin in the hardware configuration
//#define AR0 0 //UART RX
//#define AR1 0 //UART TX
#define AR2 0 //interrupt enabled
#define AR3 1 //interrupt enabled
#define AR4 0
//#define AR5 2 //mapped to pwm0
#define AR6 1
#define AR7 2
#define AR8 3
//#define AR9 2 //mapped to pwm 1
#define AR10 4
//#define AR11 2 //mapped to pwm2
#define AR12 5
#define AR13 6

//offset of Arduino Analog Input from base address XADCPS_CH_AUX_MIN
#define XADC_DEVICE_ID 		XPAR_XADCPS_0_DEVICE_ID
#define A0   XADCPS_CH_AUX_MIN+1
#define A1   XADCPS_CH_AUX_MIN+9
#define A2   XADCPS_CH_AUX_MIN+6
#define A3   XADCPS_CH_AUX_MIN+15
#define A4   XADCPS_CH_AUX_MIN+5
#define A5   XADCPS_CH_AUX_MIN+13
#define TEMP XADCPS_CH_TEMP

// indexes of interrupts towards the GIC
#define BTN_INTR_GIC   1
#define AUDIO_INTR_GIC 2
#define XADC_INTR_GIC  3
#define TMR0_INTR_GIC  4
#define TMR1_INTR_GIC  5
#define TMR2_INTR_GIC  6
#define ARD_INTR_GIC   7
#endif /* CONFIG_H */

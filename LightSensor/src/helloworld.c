#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xtmrctr.h"
#include "xgpio.h"
#include "sleep.h"

// Hardware IDs
#define GPIO_ID XPAR_ARDUINO_ARDUINO_NO_INTR_PINS_DEVICE_ID
#define TMR0_ID XPAR_TMRCTR_0_DEVICE_ID // Horizontal (Lars)
#define TMR1_ID XPAR_TMRCTR_1_DEVICE_ID // Vertical (Rich)

// Pin Definitions (from your code/manual)
#define AR4 4  // Vertical Stop/Enable (Set LOW to move)
#define AR6 6  // Horizontal Direction Right
#define AR7 7  // Horizontal Direction Left

// PWM Constants
#define PWM_PERIOD 100000
#define PWM_DUTY_50_PERCENT 50000 // Neutral (Stop for Vertical)
#define PWM_DUTY_60_PERCENT 60000 // Slow Up
#define PWM_DUTY_40_PERCENT 40000 // Slow Down
#define PWM_DUTY_20_PERCENT 20000 // Slow Horizontal Speed

XTmrCtr TimerHor, TimerVer;
XGpio GpioOut;

// The function logic your friend used
void PwmConfig(XTmrCtr *InstancePtr, u32 Period, u32 Duty) {
    XTmrCtr_PwmDisable(InstancePtr);
    XTmrCtr_PwmConfigure(InstancePtr, Period, Duty);
    XTmrCtr_PwmEnable(InstancePtr);
}

int main() {
    init_platform();
    printf("--- INITIALIZING MOTORS (SAFETY START) ---\n\r");

    // 1. Setup Hardware
    XGpio_Initialize(&GpioOut, GPIO_ID);
    XGpio_SetDataDirection(&GpioOut, 1, 0x00); // All Outputs
    XTmrCtr_Initialize(&TimerHor, TMR0_ID);
    XTmrCtr_Initialize(&TimerVer, TMR1_ID);

    // 2. ARMING SEQUENCE (Prevents "Max Speed" Jump)
    // Set Vertical to Neutral and release the AR4 stop pin
    XGpio_DiscreteClear(&GpioOut, 1, (1 << AR4));
    PwmConfig(&TimerVer, PWM_PERIOD, PWM_DUTY_50_PERCENT);
    PwmConfig(&TimerHor, PWM_PERIOD, 0); // Horizontal speed 0

    printf("Waiting 2 seconds for ESC to arm...\n\r");
    sleep(2);

    // --- VERTICAL TEST ---
    printf("Moving Vertical: UP\n\r");
    PwmConfig(&TimerVer, PWM_PERIOD, PWM_DUTY_60_PERCENT);
    usleep(800000); // 0.8 seconds

    printf("Moving Vertical: DOWN\n\r");
    PwmConfig(&TimerVer, PWM_PERIOD, PWM_DUTY_40_PERCENT);
    usleep(800000);

    printf("Vertical: NEUTRAL\n\r");
    PwmConfig(&TimerVer, PWM_PERIOD, PWM_DUTY_50_PERCENT);
    sleep(1);

    // --- HORIZONTAL TEST ---
    printf("Moving Horizontal: RIGHT\n\r");
    XGpio_DiscreteWrite(&GpioOut, 1, (1 << AR6)); // AR6 High, AR7 Low
    PwmConfig(&TimerHor, PWM_PERIOD, PWM_DUTY_20_PERCENT);
    usleep(800000);

    printf("Moving Horizontal: LEFT\n\r");
    XGpio_DiscreteWrite(&GpioOut, 1, (1 << AR7)); // AR7 High, AR6 Low
    PwmConfig(&TimerHor, PWM_PERIOD, PWM_DUTY_20_PERCENT);
    usleep(800000);

    printf("Horizontal: STOP\n\r");
    XGpio_DiscreteClear(&GpioOut, 1, (1 << AR6) | (1 << AR7));
    XTmrCtr_PwmDisable(&TimerHor);

    printf("--- Test Sequence Complete ---\n\r");

    cleanup_platform();
    return 0;
}

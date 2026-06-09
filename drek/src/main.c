#include <stdio.h>
#include "platform.h"
#include "config.h"
#include "pwmFunc.h"
#include "motorMoveFunc.h"
#include "xadc.h"
#include "scu_timer.h"
#include "sensors.h"
#include "xgpio.h"
#include "memory.h"

// Inside main(), after system initialization:

extern volatile int UpdateMotorsNow; // Access the flag
XGpio outputGPIO;

int main() {
    init_platform();
    XGpio_Initialize(&outputGPIO, GPIO_DEVICE_ID);
    XGpio_SetDataDirection(&outputGPIO, 1, 0x0);

    Init_pwm();
    Init_adc();
    Init_GIC();
    Init_SCU_Timer_Int_System();

    printf("--- SYSTEM READY ---\n\r");
    while(1) {
        if(UpdateMotorsNow) {
            UpdateMotorsNow = 0; // Reset flag
            update_motor_logic(); // ONLY update 10 times per second
        }
    }

    cleanup_platform();
    return 0;
}

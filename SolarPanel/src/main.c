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

extern volatile int UpdateMotorsNow;

XGpio outputGPIO;

int main()
{
    init_platform();

    XGpio_Initialize(&outputGPIO, GPIO_DEVICE_ID);
    XGpio_SetDataDirection(&outputGPIO, 1, 0x0);  // all pins output

    Init_pwm();
    Init_adc();
    Init_GIC();
    Init_SCU_Timer_Int_System();

    printf("--- SYSTEM READY ---\n\r");

    while (1)
    {
        if (UpdateMotorsNow)
        {
            /* ── DEBUG: pull AR4 LOW — response time = pulse width ── */
            uint32_t cur = XGpio_DiscreteRead(&outputGPIO, 1);
            XGpio_DiscreteWrite(&outputGPIO, 1, cur & ~(1 << DEBUG_PIN));
            /* ─────────────────────────────────────────────────────── */

            UpdateMotorsNow = 0;
            update_motor_logic();
        }
    }

    cleanup_platform();
    return 0;
}

#include "pwmFunc.h"
#include "timer_f.h"
#include "config.h"

XTmrCtr TimerHor;
XTmrCtr TimerVer;

void Init_pwm() {
    PwmInit(&TimerHor, 0);
    PwmInit(&TimerVer, 1);

    // Horizontal motor starts disabled
    XTmrCtr_PwmDisable(&TimerHor);

    // Vertical motor starts at the neutral (stop) position
    // Use the new name here:
    PwmConfig(&TimerVer, PWM_PERIOD, PWM_VER_STOP);
}

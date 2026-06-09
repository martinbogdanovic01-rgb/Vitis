#include "motorMoveFunc.h"
#include "pwmFunc.h"
#include "config.h"
#include "xgpio.h"
#include "timer_f.h"

extern XGpio outputGPIO;

void moveHorMotor(MotorDirection dir) {
    uint32_t current = XGpio_DiscreteRead(&outputGPIO, 1);

    switch(dir) {
        case MOVE_POSITIVE_FAST:
            current |= (1 << AR6); current &= ~(1 << AR7);
            XGpio_DiscreteWrite(&outputGPIO, 1, current);
            PwmConfig(&TimerHor, PWM_PERIOD, PWM_HOR_FAST);
            break;
        case MOVE_POSITIVE_SLOW:
            current |= (1 << AR6); current &= ~(1 << AR7);
            XGpio_DiscreteWrite(&outputGPIO, 1, current);
            PwmConfig(&TimerHor, PWM_PERIOD, PWM_HOR_SLOW);
            break;
        case MOVE_NEGATIVE_FAST:
            current |= (1 << AR7); current &= ~(1 << AR6);
            XGpio_DiscreteWrite(&outputGPIO, 1, current);
            PwmConfig(&TimerHor, PWM_PERIOD, PWM_HOR_FAST);
            break;
        case MOVE_NEGATIVE_SLOW:
            current |= (1 << AR7); current &= ~(1 << AR6);
            XGpio_DiscreteWrite(&outputGPIO, 1, current);
            PwmConfig(&TimerHor, PWM_PERIOD, PWM_HOR_SLOW);
            break;
        default: // MOVE_NONE
            current &= ~((1 << AR6) | (1 << AR7));
            XGpio_DiscreteWrite(&outputGPIO, 1, current);
            XTmrCtr_PwmDisable(&TimerHor);
            break;
    }
}

void moveVerMotor(MotorDirection dir) {
    if (dir == MOVE_POSITIVE_FAST)      PwmConfig(&TimerVer, PWM_PERIOD, PWM_VER_FAST_POS);
    else if (dir == MOVE_POSITIVE_SLOW) PwmConfig(&TimerVer, PWM_PERIOD, PWM_VER_SLOW_POS);
    else if (dir == MOVE_NEGATIVE_FAST) PwmConfig(&TimerVer, PWM_PERIOD, PWM_VER_FAST_NEG);
    else if (dir == MOVE_NEGATIVE_SLOW) PwmConfig(&TimerVer, PWM_PERIOD, PWM_VER_SLOW_NEG);
    else                                PwmConfig(&TimerVer, PWM_PERIOD, PWM_VER_STOP);
}

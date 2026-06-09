#ifndef PWMFUNC_H
#define PWMFUNC_H

#include "xtmrctr.h"
#include "config.h"

// Horizontal Speed Levels
#define PWM_HOR_SLOW      ((PWM_PERIOD * 35) / 100)
#define PWM_HOR_FAST      ((PWM_PERIOD * 80) / 100)

// Vertical Speed Levels (Based on 50% being Neutral/Stop)
#define PWM_VER_STOP      ((PWM_PERIOD * 50) / 100)
#define PWM_VER_SLOW_POS  ((PWM_PERIOD * 60) / 100)
#define PWM_VER_FAST_POS  ((PWM_PERIOD * 85) / 100)
#define PWM_VER_SLOW_NEG  ((PWM_PERIOD * 40) / 100)
#define PWM_VER_FAST_NEG  ((PWM_PERIOD * 15) / 100)

extern XTmrCtr TimerHor;
extern XTmrCtr TimerVer;

void Init_pwm();

#endif

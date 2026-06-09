#pragma once
/*
 *
 */

#include "xtmrctr.h"
#include "xparameters.h"
#include "xil_types.h"

#define AXI_CLOCK_FREQ 100000000U
#define PWM_FREQ       200U
#define PWM_PERIOD     (AXI_CLOCK_FREQ / PWM_FREQ)

/* initialize PWM timers — returns XST_SUCCESS or XST_FAILURE */
int Init_PWM(void);

/* set duty cycles in percent (0-100) */
void set_pwm(u8 r, u8 g, u8 b);

/* external timer instances */
extern XTmrCtr pwm_Red;
extern XTmrCtr pwm_Green;
extern XTmrCtr pwm_Blue;

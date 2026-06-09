/*
 * File:   timer_f.h
 * Author(s): Bianca Ion
 *
 * Created on 18 July 2022, 15:29 by Bianca Ion
 * Updated on 10 August 2022, 18:17 by Bianca Ion
 */


#ifndef TIMER_F_H
#define TIMER_F_H

#ifdef __cplusplus
extern "C" {
#endif

#include"xtmrctr.h"
#include"config.h"
#define MAX_COUNT  0xFFFFFFFF
#define TMR_LOAD(per)  (u32)((int)MAX_COUNT + 2 - (int)(per * (float)PB_FRQ)) //per is period in sec

/*
 * Initialize a pwm generator
 * Input: timer instance (XTmrCtr pointer) and the index of the desired timer instance
 * (e.g. for output on pwm0_0_0, timer 0 is used, so TmrInstanceNr=0
 */
int PwmInit(XTmrCtr *TmrCtrInstancePtr, int TmrInstanceNr);

/*
 * configure the PWM generator; input: timer instance, the PWM period and the width (high time) of the PWM
 */
void PwmConfig(XTmrCtr *TmrCtrInstancePtr, u32 Period, u32 HighTime);

/*
 * Configure the timer TmrInstanceNr to count for a specific period
 * TmrCtrInstancePtr points to the timer instance (e.g. &timer1)
 * TmrInstanceNr is the index of the used timer (e.g. for &timer1, TmrInstanceNr = 1)
 * period represents the amount of time in seconds after which the timer should overflow
 */
int timerInit(XTmrCtr *TmrCtrInstancePtr, int TmrInstanceNr, float period);

#endif

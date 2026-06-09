#pragma once


#include "xscugic.h"
#include "xscutimer.h"

/* Externs for use in main */
extern XScuGic my_Gic;
extern XScuTimer my_Timer;

/* initialization functions */
int Init_GIC(void);
int Init_SCUTimer(void);

/* connect SCU timer to interrupt system */
int addScuTimerToInterruptSystem(void);

/* ISR callback */
void my_timer_interrupt_handler(void *ref);

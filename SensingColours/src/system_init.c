/*
 * system_init.c - initialize interrupt controller and SCU timer
 */

#include "system_init.h"
#include "sense.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xil_exception.h"
#include "xstatus.h"

XScuGic my_Gic;
XScuTimer my_Timer;

#define SCU_TIMER_FREQ 333333333U
#define SCU_TIMER_VALUE (SCU_TIMER_FREQ / 10U)
#define SCU_TIMER_INTR_PRI  (0xA0)
#define SCU_TIMER_INTR_TRIG (0x03)

void my_timer_interrupt_handler(void *ref)
{
    XScuTimer *TimerInstancePtr = (XScuTimer *) ref;

    if (XScuTimer_IsExpired(TimerInstancePtr)) {
        XScuTimer_ClearInterruptStatus(TimerInstancePtr);
    }

    ProcessControlLoop();
}

int Init_GIC(void)
{
    XScuGic_Config *cfg = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
    if (cfg == NULL) return XST_FAILURE;

    if (XScuGic_CfgInitialize(&my_Gic, cfg, cfg->CpuBaseAddress) != XST_SUCCESS)
        return XST_FAILURE;

    if (XScuGic_SelfTest(&my_Gic) != XST_SUCCESS)
        return XST_FAILURE;

    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
                                 (Xil_ExceptionHandler) XScuGic_InterruptHandler,
                                 &my_Gic);
    return XST_SUCCESS;
}

int Init_SCUTimer(void)
{
    XScuTimer_Config *cfg = XScuTimer_LookupConfig(XPAR_PS7_SCUTIMER_0_DEVICE_ID);
    if (cfg == NULL) return XST_FAILURE;

    if (XScuTimer_CfgInitialize(&my_Timer, cfg, cfg->BaseAddr) != XST_SUCCESS)
        return XST_FAILURE;

    XScuTimer_EnableAutoReload(&my_Timer);
    XScuTimer_LoadTimer(&my_Timer, SCU_TIMER_VALUE);
    XScuTimer_EnableInterrupt(&my_Timer);

    return XST_SUCCESS;
}

int addScuTimerToInterruptSystem(void)
{
    int status;

    status = XScuGic_Connect(&my_Gic,
                             XPAR_PS7_SCUTIMER_0_INTR,
                             (Xil_ExceptionHandler) my_timer_interrupt_handler,
                             (void *) &my_Timer);
    if (status != XST_SUCCESS) return XST_FAILURE;

    XScuGic_SetPriorityTriggerType(&my_Gic,
                                   XPAR_PS7_SCUTIMER_0_INTR,
                                   SCU_TIMER_INTR_PRI,
                                   SCU_TIMER_INTR_TRIG);

    XScuGic_Enable(&my_Gic, XPAR_PS7_SCUTIMER_0_INTR);

    return XST_SUCCESS;
}
/*
  SCU Timer is a hardware countdown timer
 Load a value into the timer:

Load value=Period (s)×Timer Clock Frequency

Example:
 CPU runs at 666 MHz, divided by 2 → 333 MHz timer clock.

0.1 s period (10 Hz control loop):0.1s×333,000,000Hz≈33,300,0000.1s×333,000,000Hz≈33,300,000

Timer counts down on every clock tick.

Because we need a control loop every 100ms

It counts down from a set value

It can be periodic so when enabled, the timer automatically reloads the start value after reaching 0

The timer can generate an interrupt when it expires. This interrupt can call an ISR (Interrupt Service Routine).

The timer runs at half the CPU frequency by default. For example, with a 666 MHz CPU → timer clock = 333 MHz.
Single-shot: timer stops after one interrupt.

Periodic: timer reloads automatically for continuous interrupts.
 */

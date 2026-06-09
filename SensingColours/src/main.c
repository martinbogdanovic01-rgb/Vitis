/*

 *
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xil_exception.h"
#include "xscutimer.h"

#include "system_init.h"
#include "sense.h"
#include "pwm.h"

/* Shared state between ISR and main */
volatile int new_data_flag = 0;
/*because it is modified in an Interrupt Service Routine (ISR) and read in the main loop. */
volatile float current_voltage = 0.0f;
/*Stores the latest voltage reading to be printed
volatile u8 curr_r = 0;
volatile u8 curr_g = 0;
volatile u8 curr_b = 0;
/*Stores the current Red, Green, and Blue duty cycles (0-100) for printing*/
int main(void)
{
    int status;

    init_platform();
    xil_printf("\n--- Sense Colours App Started ---\r\n");

    status = Init_GIC();/*generic interupt controler*/
    if (status != XST_SUCCESS) {
        xil_printf("GIC init failed\r\n");
    }

    status = Init_SCUTimer();
    if (status != XST_SUCCESS) {
        xil_printf("SCU Timer init failed\r\n");
    }

    status = Init_PWM();
    if (status != XST_SUCCESS) {
        xil_printf("PWM init failed\r\n");
    }

    status = Init_XADC();
    if (status != XST_SUCCESS) {
        xil_printf("XADC init failed\r\n");
    }

    status = addScuTimerToInterruptSystem();
    if (status == XST_SUCCESS) {
        xil_printf("Control loop interrupt configured\r\n");
    } else {
        xil_printf("Failed to connect control loop interrupt\r\n");
    }

    Xil_ExceptionEnable();
    XScuTimer_Start(&my_Timer);

 /*while (1): The program runs forever*/   while (1) {
        if (new_data_flag) {
     /*Resets the flag so we don't print the same data twice */       new_data_flag = 0;
            int vi = (int) current_voltage;
            int vf = (int) ((current_voltage - vi) * 1000);
         /*   vi: Integer part (e.g., 2 from 2.456).
            vf: Fractional part converted to integer */
            xil_printf("V: %d.%03d | RGB: %d %d %d\r\n",
                       vi, vf, curr_r, curr_g, curr_b);
        }
    }

    cleanup_platform();
    return 0;
}

/* ---------------------------------------------------------------------------- *
 * Description :
 * Example of using interrupts of the scu timer. An interrupt is generated every 1 second
 *
 *
 * Note :
 * The user of this demo code should NOT expect perfect code.
 * The demo code DOES have (probably) room for further improvement/refactoring.
 * Furthermore, the authors of this demo have done their best to test the code.
 * However they may have overlooked conditions in which the demo code may not
 * operate properly. If you notice an error or bug or improvement opportunity,
 * please be so kind to constructively report this to the authors of the demo code.
 *
 * Authors:
 * 		Jeedella Jeedella, j.jeedella@fontys.nl
 * 		Brice Guayrin, b.guayrin@fontys.nl
 *		Harold Benton, h.benton@fontys.nl
 *
 * Date: November 2024
 * ---------------------------------------------------------------------------- */


/*****************************************************************************/
/***************************** Include Files *********************************/
/*****************************************************************************/
#include <stdio.h>
#include "platform.h"
#include "xscugic.h"
#include"xtmrctr.h"
#include "xil_printf.h"
#include "sleep.h"
#include "xtime_l.h"
#include "xgpio.h"
#include "xparameters.h"
#include "xscutimer.h"
#include "xiicps.h"


/*****************************************************************************/
/************************** Constant Definitions *****************************/
/*****************************************************************************/
// SCU timer settings
#define SCU_TIMER_VALUE		(0x26BE3680) // 1s @ 650MHz/2 (3ns)

// Interrupt settings
// in steps of 8, lower number high priority
#define SCU_TIMER_INTR_PRI			(0xA0)
#define SCU_TIMER_INTR_TRIG			(0x03)



/*****************************************************************************/
/************************** Driver instances and pointers ********************/
/*****************************************************************************/
static XScuTimer my_Timer;  // SCU Timer instance
static XScuGic my_Gic ;  	// GIC instance


/*****************************************************************************/
/************************** Prototype of Interrupt Handler (aka ISR) *********/
/*****************************************************************************/
void my_timer_interrupt_handler(void * CallBackRef);


/*****************************************************************************/
/************************** Prototype of API Functions **********************/
/*****************************************************************************/
int Init_GIC(void);
int Init_SCUTimer(void);
int addScuTimerToInterruptSystem(void);


/*****************************************************************************/
/************************** Main program *************************************/
/*****************************************************************************/
int main(){
	/* Initialize PS platform */
	 init_platform();

	/* Initialize SCUGIC */
	if( Init_GIC() == XST_SUCCESS){
	xil_printf("\nSCUGIC initialized \r\n");
	}

	/* Initialize SCU Timer */
	if (Init_SCUTimer() == XST_SUCCESS){
		xil_printf("SCU Timer initialized \r\n");
	}

	/* Add SCU Timer to interrupt system */
	if (addScuTimerToInterruptSystem() == XST_SUCCESS){
		xil_printf("SCU Timer added to interrupt system \r\n");
	}

	/* Enable interrupts */
	xil_printf("Enabling interrupts \r\n");
	Xil_ExceptionEnable(); // Initialize exception handeling in the ARM processor

	/* Start SCU timer */
	xil_printf("Starting the SCU Timer \r\n");
	XScuTimer_Start(&my_Timer);

	xil_printf("Entering main loop \r\n");
	while (1)
	{
		// Do nothing
	}
	return 0;
}


/*****************************************************************************/
/************************** SCU Timer Interrupt Handler (aka ISR) ************/
/*****************************************************************************/
void my_timer_interrupt_handler(void * CallBackRef){

	/* Clear interrupt status */
    XScuTimer *TimerInstancePtr = (XScuTimer *) CallBackRef;
    if( XScuTimer_IsExpired(TimerInstancePtr))
    {
    	XScuTimer_ClearInterruptStatus(TimerInstancePtr);
    }

	/* Add below your specific interrupt code */
	xil_printf("One second has elapsed; SCU Timer interrupt OCCURED \r\n");
}


/*****************************************************************************/
/************************** Definitions of (API) Functions **********************/
/*****************************************************************************/
int Init_GIC(void){
	XScuGic_Config *Gic_Config= NULL;
	int status;

	/* ---------------------------------------------------------------------
	 * ------------ STEP 1: DEVICE LOOK-UP ------------
	 * -------------------------------------------------------------------- */
	// Look up the configuration information for the GIC
	Gic_Config = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
	if (Gic_Config== NULL)
	{
		status = XST_FAILURE;
		return status;
	}

	/* ---------------------------------------------------------------------
	 * ------------ STEP 2: DRIVER INITIALISATION ------------
	 * -------------------------------------------------------------------- */
	// Configure the GIC with the configuration information
	status = XScuGic_CfgInitialize(&my_Gic, Gic_Config, Gic_Config->CpuBaseAddress);

	/* ---------------------------------------------------------------------
	* ------------ STEP 3: SELF TEST ------------
	* -------------------------------------------------------------------- */
	status = XScuGic_SelfTest(&my_Gic);
	if (status != XST_SUCCESS){
		xil_printf("GIC config init failed \r\n");
		return XST_FAILURE;
	}

	/* ---------------------------------------------------------------------
	* ----------------------- STEP 4: CONNECT GIC ----
	* -------------------------------------------------------------------- */
 	// Initialise exception logic on the ARM Cortex A9
 	Xil_ExceptionInit();

	/* Connect the interrupt controller interrupt handler to the
	* hardware interrupt handling logic in the processor. */
 	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
 								(Xil_ExceptionHandler)XScuGic_InterruptHandler,
								&my_Gic);

 	/* === END CONFIGURATION SEQUENCE ===  */

	return XST_SUCCESS;
}

int Init_SCUTimer(void){
	// XScuTimer SCUTimer;
	XScuTimer_Config *Timer_Config= NULL;
	int status;

	/* === START CONFIGURATION SEQUENCE ===  */
	/* ---------------------------------------------------------------------
	 * ------------ STEP 1: DEVICE LOOK-UP ------------
	 * -------------------------------------------------------------------- */
	// Look up the configuration information for the SCU Timer
	Timer_Config= XScuTimer_LookupConfig(XPAR_PS7_SCUTIMER_0_DEVICE_ID);
 	if (Timer_Config == NULL)
	{
 		status = XST_FAILURE;
 		return status;
	}

	/* ---------------------------------------------------------------------
	 * ------------ STEP 2: DRIVER INITIALISATION ------------
	 * -------------------------------------------------------------------- */
 	// Configure the SCU timer with the configuration information
 	status = XScuTimer_CfgInitialize(&my_Timer, Timer_Config, Timer_Config->BaseAddr);
	if (status != XST_SUCCESS){
		xil_printf("SCU Timer cfg init failed \r\n");
		return status;
	}

	/* ---------------------------------------------------------------------
	* ------------ STEP 3: SELF TEST ------------
	* -------------------------------------------------------------------- */
	status = XScuTimer_SelfTest(&my_Timer);
	if (status != XST_SUCCESS){
		xil_printf("SCU Timer cfg init failed \r\n");
		return status;
	}

	/* ---------------------------------------------------------------------
	* ------------ STEP 4: ENABLE INTERRUPTS  ------------------------------
	* -------------------------------------------------------------------- */
	XScuTimer_EnableInterrupt(&my_Timer);

	/* ---------------------------------------------------------------------
	* ------------ STEP 4: INIT COUNTER OF SCU TIMER   ---------------------
	* -------------------------------------------------------------------- */
	XScuTimer_EnableAutoReload(&my_Timer);

	XScuTimer_LoadTimer(&my_Timer, SCU_TIMER_VALUE );


	/* === END CONFIGURATION SEQUENCE ===  */
	return XST_SUCCESS;

}

int addScuTimerToInterruptSystem(void)
{
	int status;
	/* Connect a device driver handler for the XScuTimer */
	// Assign (connect) the interrupt handler that you wrote for our timer
	status = XScuGic_Connect(&my_Gic,
								XPAR_PS7_SCUTIMER_0_INTR,
								(Xil_ExceptionHandler) my_timer_interrupt_handler,
								(void *) &my_Timer);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	/* Set priority and trigger type */
	XScuGic_SetPriorityTriggerType(&my_Gic,
									XPAR_PS7_SCUTIMER_0_INTR,
									SCU_TIMER_INTR_PRI,
									SCU_TIMER_INTR_TRIG);

	/* Enable the interrupt on the GIC for SCU Timer */
	XScuGic_Enable(&my_Gic, XPAR_PS7_SCUTIMER_0_INTR);

	/* Return initialisation result to calling code */
	return status;
}



#include "GDO0_Interrupt.h"
#include "xil_printf.h"
#include "xil_exception.h"
XScuGic Inst_GIC ;

volatile uint8_t PacketReady = 0;

int inp_value= 0;

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
	status = XScuGic_CfgInitialize(&Inst_GIC, Gic_Config, Gic_Config->CpuBaseAddress);

	/* ---------------------------------------------------------------------
	* ------------ STEP 3: SELF TEST ------------
	* -------------------------------------------------------------------- */
	status = XScuGic_SelfTest(&Inst_GIC);
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
 	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
 								(Xil_ExceptionHandler)XScuGic_InterruptHandler,
								&Inst_GIC);

 	/* === END CONFIGURATION SEQUENCE ===  */
	return XST_SUCCESS;
}

/* ------------------------------------------------------------ */
/* ISR                                                          */
/* ------------------------------------------------------------ */
void GDO0_interrupt_handler(void *CallbackRef)
{
    uint32_t status;
    uint8_t gdo0;

    status = XGpio_InterruptGetStatus(&GDO0_GPIO);

    if (!(status & AR_GDO0_INT))
        return;

    gdo0 = XGpio_DiscreteRead(&GDO0_GPIO, 1) & 0x1;

    // Falling edge = packet complete
    if (!gdo0)
    {
        PacketReady = 1;
    }

    XGpio_InterruptClear(&GDO0_GPIO, AR_GDO0_INT);
}

/* ------------------------------------------------------------ */
/* Connect to existing GIC (Inst_GIC from GIC_MO6)             */
/* ------------------------------------------------------------ */
int Add_GDO0_To_Interrupt_System(void)
{
	int status;

	// Enable GPIO interrupts
	XGpio_InterruptEnable(&GDO0_GPIO, AR_GDO0_INT);
	XGpio_InterruptGlobalEnable(&GDO0_GPIO);

	// Connect button ISR to the already initialized GIC
	status = XScuGic_Connect(&Inst_GIC,
			AR_INTC_GPIO_INTERRUPT_ID,
							 (Xil_ExceptionHandler)GDO0_interrupt_handler,
							 (void *)&GDO0_GPIO);
	if (status != XST_SUCCESS) return XST_FAILURE;

	// Set priority and edge trigger
	XScuGic_SetPriorityTriggerType(&Inst_GIC,
								   AR_INTC_GPIO_INTERRUPT_ID,
								   0x80, // priority
								   0x3); // rising edge

	// Enable GPIO interrupt in GIC
	XScuGic_Enable(&Inst_GIC, AR_INTC_GPIO_INTERRUPT_ID);

	return XST_SUCCESS;

}

/* ------------------------------------------------------------ */
/* Top-level init — call this from main                         */
/* ------------------------------------------------------------ */
void Init_GDO0_Int_System(void)
{
    if (Init_GDO0_GPIO() == XST_SUCCESS) {
        xil_printf("GDO0 GPIO initialised\r\n");
    }

    if (Add_GDO0_To_Interrupt_System() == XST_SUCCESS) {
        xil_printf("GDO0 added to interrupt system\r\n");
    }

    Xil_ExceptionEnable();
}

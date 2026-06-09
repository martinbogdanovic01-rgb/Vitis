/*
 * File:   timer.c
 * Author(s): Bianca Ion
 *
 * Created on 18 July 2022, 15:29 by Bianca Ion
 * Updated on 10 August 2022, 18:17 by Bianca Ion
 */

#include "timer_f.h"

//libs of PYNQ

#include "xil_exception.h"


int PwmInit(XTmrCtr *TmrCtrInstancePtr, int TmrInstanceNr){

	u16 DeviceId;
	int Status = XST_SUCCESS;
	switch (TmrInstanceNr){
		case 0: DeviceId = XPAR_TMRCTR_0_DEVICE_ID; break;
		case 1: DeviceId = XPAR_TMRCTR_1_DEVICE_ID; break;
		case 2: DeviceId = XPAR_TMRCTR_2_DEVICE_ID; break;
		default: break;
	}
	Status = XTmrCtr_Initialize(TmrCtrInstancePtr, DeviceId);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Perform a self-test to ensure that the hardware was built
	 * correctly. Timer0 is used for self test
	 */
	Status = XTmrCtr_SelfTest(TmrCtrInstancePtr, 0);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	return Status;
}

void PwmConfig(XTmrCtr *TmrCtrInstancePtr, u32 Period, u32 HighTime){
	u8 DutyCycle;
	XTmrCtr_PwmDisable(TmrCtrInstancePtr);

	DutyCycle = XTmrCtr_PwmConfigure(TmrCtrInstancePtr, Period, HighTime);
	//xil_printf("PWM Configured for Duty Cycle = %d\r\n", DutyCycle);

	/* Enable PWM */
	XTmrCtr_PwmEnable(TmrCtrInstancePtr);
}


int timerInit(XTmrCtr *TmrCtrInstancePtr, int TmrInstanceNr, float period){

	u16 DeviceId;
	int Status = XST_SUCCESS;
	switch (TmrInstanceNr){
		case 0: DeviceId = XPAR_TMRCTR_0_DEVICE_ID; break;
		case 1: DeviceId = XPAR_TMRCTR_1_DEVICE_ID; break;
		case 2: DeviceId = XPAR_TMRCTR_2_DEVICE_ID; break;
		default: break;
	}
	Status = XTmrCtr_Initialize(TmrCtrInstancePtr, DeviceId);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XTmrCtr_SetResetValue(TmrCtrInstancePtr, 0, TMR_LOAD(period));
	XTmrCtr_SetOptions(TmrCtrInstancePtr,0, XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_Start(TmrCtrInstancePtr,0);
	return Status;
}

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xadcps.h"
#include "xscugic.h"
#include "xtmrctr.h"
#include "xgpio.h"
#include "xscutimer.h"
#include "xiicps.h"
#include "sleep.h"

#include "Defines.h"
#include "SH1106_Screen.h"
#include "animation.h"

extern u8g2_t u8g2;

XScuGic Inst_GIC;
XGpio overCurrentGPIO, outputGPIO;
XTmrCtr TimerHor, TimerVer;
XScuTimer timer_scu_100ms;
XAdcPs xadc;

volatile int horOverCurrentDet = 0;
volatile int verOverCurrentDet = 0;
char horText[32] = "H: IDLE";
char verText[32] = "V: IDLE";

int sensorChannels[4] = {1, 9, 6, 15};
u16 digitalSensorVal[4];
double analogSensorVal[4];

void PwmConfig(XTmrCtr *InstancePtr, uint32_t Period, uint32_t Duty) {
    XTmrCtr_PwmDisable(InstancePtr);
    XTmrCtr_PwmConfigure(InstancePtr, Period, Duty);
    XTmrCtr_PwmEnable(InstancePtr);
    XTmrCtr_Start(InstancePtr, 0);
    XTmrCtr_Start(InstancePtr, 1);
}

void timer_handler(void *CallBackRef) {
    XScuTimer_ClearInterruptStatus((XScuTimer *) CallBackRef);

    // 1. Read Sensors
    for(int i = 0; i < 4; i++) {
        digitalSensorVal[i] = XAdcPs_GetAdcData(&xadc, XADCPS_CH_AUX_MIN + sensorChannels[i]);
        analogSensorVal[i] = ((double)(digitalSensorVal[i] >> 4) * 3.3) / 4095.0;
    }

    double hDiff = analogSensorVal[2] - analogSensorVal[3]; // Left - Right

    // 2. Horizontal Logic (Power boost to 90%)
    if (!horOverCurrentDet) {
        uint32_t val = XGpio_DiscreteRead(&outputGPIO, 1);
        if (hDiff > 0.25) {
            // Move Right
            XGpio_DiscreteWrite(&outputGPIO, 1, (val | (1 << AR6)) & ~(1 << AR7));
            PwmConfig(&TimerHor, PWM_PERIOD, 90000);
            sprintf(horText, "H: MOV RIGHT");
        } else if (hDiff < -0.25) {
            // Move Left
            XGpio_DiscreteWrite(&outputGPIO, 1, (val | (1 << AR7)) & ~(1 << AR6));
            PwmConfig(&TimerHor, PWM_PERIOD, 90000);
            sprintf(horText, "H: MOV LEFT");
        } else {
            // Stop
            XGpio_DiscreteWrite(&outputGPIO, 1, val & ~((1 << AR7) | (1 << AR6)));
            XTmrCtr_PwmDisable(&TimerHor);
            sprintf(horText, "H: IDLE");
        }
    }

    // 3. Vertical Logic (Same as before)
    double vDiff = analogSensorVal[0] - analogSensorVal[1];
    if (!verOverCurrentDet) {
        if (vDiff > 0.25) { PwmConfig(&TimerVer, PWM_PERIOD, 65000); sprintf(verText, "V: MOV UP"); }
        else if (vDiff < -0.25) { PwmConfig(&TimerVer, PWM_PERIOD, 35000); sprintf(verText, "V: MOV DOWN"); }
        else { PwmConfig(&TimerVer, PWM_PERIOD, 50000); sprintf(verText, "V: IDLE"); }
    }

    // 4. Update OLED
    animation_update(verText, horText, digitalSensorVal[0], digitalSensorVal[1], digitalSensorVal[2], digitalSensorVal[3]);

    // 5. Serial Debug
    xil_printf("L:%d R:%d Diff:%f Status:%s\r\n", digitalSensorVal[2]>>4, digitalSensorVal[3]>>4, hDiff, horText);
}

void overcurrent_handler(void *Ref) {
    int val = XGpio_DiscreteRead(&overCurrentGPIO, 1);
    if (val & 0x01) { horOverCurrentDet = 1; sprintf(horText, "H: OVERCUR!"); }
    XGpio_InterruptClear(&overCurrentGPIO, AR_OVERCURRENT_INT);
}

int main() {
    init_platform();
    initDisplay();
    animation_startup();

    XAdcPs_Config *adc_c = XAdcPs_LookupConfig(XPAR_PS7_XADC_0_DEVICE_ID);
    XAdcPs_CfgInitialize(&xadc, adc_c, adc_c->BaseAddress);
    XAdcPs_SetSequencerMode(&xadc, XADCPS_SEQ_MODE_CONTINPASS);

    XTmrCtr_Initialize(&TimerHor, TMR0_DEVICE_ID);
    XTmrCtr_Initialize(&TimerVer, TMR1_DEVICE_ID);

    XGpio_Initialize(&outputGPIO, ARD_IO_NO_INTR_DEVICE_ID);
    XGpio_SetDataDirection(&outputGPIO, 1, 0x00);
    XGpio_Initialize(&overCurrentGPIO, OVERCUR_INPUT_DEVICE_ID);
    XGpio_SetDataDirection(&overCurrentGPIO, 1, 0xF);

    XScuGic_Config *gic_c = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
    XScuGic_CfgInitialize(&Inst_GIC, gic_c, gic_c->CpuBaseAddress);
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, &Inst_GIC);

    XScuGic_Connect(&Inst_GIC, AR_INTC_GPIO_INTERRUPT_ID, (Xil_ExceptionHandler)overcurrent_handler, &overCurrentGPIO);
    XScuGic_Enable(&Inst_GIC, AR_INTC_GPIO_INTERRUPT_ID);
    XGpio_InterruptEnable(&overCurrentGPIO, AR_OVERCURRENT_INT);

    XScuTimer_Config *tmr_c = XScuTimer_LookupConfig(XPAR_PS7_SCUTIMER_0_DEVICE_ID);
    XScuTimer_CfgInitialize(&timer_scu_100ms, tmr_c, tmr_c->BaseAddr);
    XScuTimer_EnableAutoReload(&timer_scu_100ms);
    XScuTimer_LoadTimer(&timer_scu_100ms, SCU_TIMER_VALUE);
    XScuGic_Connect(&Inst_GIC, XPAR_PS7_SCUTIMER_0_INTR, (Xil_ExceptionHandler)timer_handler, &timer_scu_100ms);
    XScuGic_Enable(&Inst_GIC, XPAR_PS7_SCUTIMER_0_INTR);

    Xil_ExceptionEnable();
    XScuTimer_EnableInterrupt(&timer_scu_100ms);
    XScuTimer_Start(&timer_scu_100ms);

    while(1);
    return 0;
}

#include "scu_timer.h"
#include "xadc.h"

volatile int UpdateMotorsNow = 0; // The Flag
static XScuTimer my_Timer;
static XScuGic my_Gic;

void my_timer_interrupt_handler(void * CallBackRef){
    XScuTimer *TimerInstancePtr = (XScuTimer *) CallBackRef;
    if(XScuTimer_IsExpired(TimerInstancePtr)) {
        XScuTimer_ClearInterruptStatus(TimerInstancePtr);
        Read_All_Channels();
        UpdateMotorsNow = 1; // Trigger the flag every 100ms
    }
}

int Init_GIC(void){
    XScuGic_Config *Cfg = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
    XScuGic_CfgInitialize(&my_Gic, Cfg, Cfg->CpuBaseAddress);
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, &my_Gic);
    return 0;
}

int Init_SCU_Timer_Int_System(void){
    XScuTimer_Config *Cfg = XScuTimer_LookupConfig(XPAR_PS7_SCUTIMER_0_DEVICE_ID);
    XScuTimer_CfgInitialize(&my_Timer, Cfg, Cfg->BaseAddr);
    XScuTimer_EnableInterrupt(&my_Timer);
    XScuTimer_EnableAutoReload(&my_Timer);
    XScuTimer_LoadTimer(&my_Timer, 0x01EFE920); // 100ms
    XScuGic_Connect(&my_Gic, XPAR_PS7_SCUTIMER_0_INTR, (Xil_ExceptionHandler)my_timer_interrupt_handler, (void *)&my_Timer);
    XScuGic_Enable(&my_Gic, XPAR_PS7_SCUTIMER_0_INTR);
    Xil_ExceptionEnable();
    XScuTimer_Start(&my_Timer);
    return 0;
}

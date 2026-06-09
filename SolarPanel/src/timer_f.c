#include "timer_f.h"
#include "config.h"

int PwmInit(XTmrCtr *TmrCtrInstancePtr, int TmrInstanceNr) {
    u16 DeviceId = (TmrInstanceNr == 0) ? TMR0_DEVICE_ID : TMR1_DEVICE_ID;
    XTmrCtr_Initialize(TmrCtrInstancePtr, DeviceId);
    return XTmrCtr_SelfTest(TmrCtrInstancePtr, 0);
}

void PwmConfig(XTmrCtr *TmrCtrInstancePtr, u32 Period, u32 HighTime) {
    XTmrCtr_PwmDisable(TmrCtrInstancePtr);
    XTmrCtr_PwmConfigure(TmrCtrInstancePtr, Period, HighTime);
    XTmrCtr_PwmEnable(TmrCtrInstancePtr);
}

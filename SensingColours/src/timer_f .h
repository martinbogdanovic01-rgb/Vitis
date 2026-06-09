

#ifndef TIMER_F_H
#define TIMER_F_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xtmrctr.h"
#include "xil_types.h"
#include "xparameters.h"

#define MAX_COUNT  0xFFFFFFFFU
/* Example macro: convert period (s) to load value; PB_FRQ must be defined in config.h */
#define TMR_LOAD(per)  (u32)((int)MAX_COUNT + 2 - (int)((per) * (float) PB_FRQ))

int PwmInit(XTmrCtr *TmrCtrInstancePtr, int TmrInstanceNr);
void PwmConfig(XTmrCtr *TmrCtrInstancePtr, u32 Period, u32 HighTime);
int timerInit(XTmrCtr *TmrCtrInstancePtr, int TmrInstanceNr, float period);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_F_H */

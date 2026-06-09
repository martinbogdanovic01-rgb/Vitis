#ifndef TIMER_F_H
#define TIMER_F_H
#include "xtmrctr.h"

int PwmInit(XTmrCtr *TmrCtrInstancePtr, int TmrInstanceNr);
void PwmConfig(XTmrCtr *TmrCtrInstancePtr, u32 Period, u32 HighTime);
#endif

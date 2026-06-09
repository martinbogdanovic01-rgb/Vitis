#pragma once


#include "xadcps.h"
#include "xil_types.h"

/* initialize XADC — returns XST_SUCCESS or XST_FAILURE */
int Init_XADC(void);

/* called from ISR to perform one control loop iteration */
void ProcessControlLoop(void);

extern XAdcPs my_Xadc;

typedef struct {
    float minV;
    float maxV;
    u8 r;
    u8 g;
    u8 b;
} ColorEntry;

/* Shared state (defined in main.c) */
extern volatile int new_data_flag;
extern volatile float current_voltage;
extern volatile u8 curr_r;
extern volatile u8 curr_g;
extern volatile u8 curr_b;

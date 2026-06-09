#ifndef XYZ_H
#define XYZ_H

#include "xil_types.h"
#include "xparameters.h"

#define XADC_DEVICE_ID XPAR_XADCPS_0_DEVICE_ID

void xyz_init();

float xyz_get_voltage(u8 channel);

#endif


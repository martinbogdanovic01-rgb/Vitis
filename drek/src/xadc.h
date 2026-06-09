#ifndef XADC_H_
#define XADC_H_

#include "xadcps.h"

extern XAdcPs xadc;
extern float sensor_voltages[4]; // Globally accessible sensor data

void Init_adc();
void Read_All_Channels();

#endif

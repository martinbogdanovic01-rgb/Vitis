#include "xadc.h"
#include <stdio.h>

XAdcPs xadc;
float sensor_voltages[4] = {0.0, 0.0, 0.0, 0.0};

void Init_adc() {
    XAdcPs_Config *ConfigPtr = XAdcPs_LookupConfig(XPAR_XADCPS_0_DEVICE_ID);
    XAdcPs_CfgInitialize(&xadc, ConfigPtr, ConfigPtr->BaseAddress);

    // Set to Sequencer Mode
    XAdcPs_SetSequencerMode(&xadc, XADCPS_SEQ_MODE_CONTINPASS);
}

void Read_All_Channels() {
    // PYNQ-Z2 Mapping: A0=Ch1, A1=Ch9, A2=Ch6, A3=Ch15
    int adc_channels[4] = {1, 9, 6, 15};

    for(int i = 0; i < 4 ; i++) {
        u32 raw = XAdcPs_GetAdcData(&xadc, XADCPS_CH_AUX_MIN + adc_channels[i]);

        // On PYNQ-Z2, the raw 12-bit value is stored in the 12 MSBs of the 16-bit reg
        // Voltage = (RawValue >> 4) * (Reference / 4096)
        // Or simply: (RawValue / 65536.0) * 1.0V (internal range)
        sensor_voltages[i] = ((float)raw / 65536.0f);
    }
}

#include <stdio.h>
#include <unistd.h>
#include "platform.h"
#include "xil_printf.h"

#include "xadcps.h"   // REQUIRED for XADC

XAdcPs xadc;         // Global XADC instance

// ----------------------------------------------------
// XADC INITIALISATION
// ----------------------------------------------------
void adc_init() {
    int Status;
    XAdcPs_Config *ConfigPtr;
    XAdcPs *XAdcInstPtr = &xadc;

    // IMPORTANT: lookup the config for XADC
    ConfigPtr = XAdcPs_LookupConfig(XPAR_XADCPS_0_DEVICE_ID);
    if (ConfigPtr == NULL) {
        xil_printf("XADC lookup failed!\n\r");
        return;
    }

    // Initialize the driver
    Status = XAdcPs_CfgInitialize(XAdcInstPtr, ConfigPtr,
                                  ConfigPtr->BaseAddress);
    if (Status != XST_SUCCESS) {
        xil_printf("XADC init failed!\n\r");
        return;
    }

    // Self-test
    Status = XAdcPs_SelfTest(XAdcInstPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("XADC self test failed!\n\r");
        return;
    }

    // Put sequencer in continuous sampling mode
    XAdcPs_SetSequencerMode(XAdcInstPtr, XADCPS_SEQ_MODE_CONTINPASS);
}

// ----------------------------------------------------
// MAIN PROGRAM
// ----------------------------------------------------
int main()
{
    init_platform();
    adc_init();

    // Pynq-Z2 analog pin mapping
    int adc_channels[6] = {1, 9, 6, 15, 5, 13};

    while (1) {
        printf("\n\r ----- New Samples -----\n\r");
        for (int i = 0; i < 6; i++) {

            // Read raw XADC value
            u16 raw = XAdcPs_GetAdcData(&xadc,
                        XADCPS_CH_AUX_MIN + adc_channels[i]);

            // Convert to voltage (16-bit register!)
            float voltage = (float)raw * 3.3f / 65536.0f;

            printf("Channel VAUX%-2d : %5d  -->  %.5f V\n\r",
                adc_channels[i], raw, voltage);
        }

        sleep(1);
    }

    cleanup_platform();
    return 0;
}

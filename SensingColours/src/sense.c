

#include "sense.h"
#include "pwm.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xstatus.h"

XAdcPs my_Xadc;

static ColorEntry LUT[] = {
    {0.000f, 0.825f,  0,  0, 90},
    {0.825f, 1.650f,  0, 50, 50},
    {1.650f, 2.475f, 50, 50,  0},
    {2.475f, 3.400f, 90,  0,  0}
};
#define LUT_SIZE (sizeof(LUT) / sizeof(LUT[0]))
#define AUX_CHANNEL 1
#define XADC_MAX_CODE 65536.0f
#define XADC_REF_VOLT 3.3f

int Init_XADC(void)
{
    XAdcPs_Config *cfg = XAdcPs_LookupConfig(XPAR_XADCPS_0_DEVICE_ID);
    if (cfg == NULL) {
        xil_printf("XAdcPs_LookupConfig failed\r\n");
        return XST_FAILURE;
    }

    if (XAdcPs_CfgInitialize(&my_Xadc, cfg, cfg->BaseAddress) != XST_SUCCESS) {
        xil_printf("XAdcPs_CfgInitialize failed\r\n");
        return XST_FAILURE;
    }

    XAdcPs_SetSequencerMode(&my_Xadc, XADCPS_SEQ_MODE_CONTINPASS);
    return XST_SUCCESS;
}

void ProcessControlLoop(void)
{
    u16 raw = XAdcPs_GetAdcData(&my_Xadc, XADCPS_CH_AUX_MIN + AUX_CHANNEL);
    float v = (raw * XADC_REF_VOLT) / XADC_MAX_CODE;

    if (v < 0.0f) v = 0.0f;
    if (v > XADC_REF_VOLT) v = XADC_REF_VOLT;

    u8 r = 0, g = 0, b = 0;
    for (int i = 0; i < LUT_SIZE; ++i) {
        if (v >= LUT[i].minV && v < LUT[i].maxV) {
            r = LUT[i].r;
            g = LUT[i].g;
            b = LUT[i].b;
            break;
        }
    }

    set_pwm(r, g, b);

    static int counter = 0;
    counter++;
    if (counter >= 10) { /* update main every 1s (10 * 100ms) */
        current_voltage = v;
        curr_r = r;
        curr_g = g;
        curr_b = b;
        new_data_flag = 1;
        counter = 0;
    }
}

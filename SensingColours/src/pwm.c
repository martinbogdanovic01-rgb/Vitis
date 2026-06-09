

#include "pwm.h"
#include "xil_printf.h"
#include "xstatus.h"

XTmrCtr pwm_Red;
XTmrCtr pwm_Green;
XTmrCtr pwm_Blue;

int Init_PWM(void)
{
    int status;

    status = XTmrCtr_Initialize(&pwm_Red, XPAR_TMRCTR_0_DEVICE_ID);
    if (status != XST_SUCCESS) {
        xil_printf("XTmrCtr init red failed\r\n");
        return XST_FAILURE;
    }

    status = XTmrCtr_Initialize(&pwm_Green, XPAR_TMRCTR_1_DEVICE_ID);
    if (status != XST_SUCCESS) {
        xil_printf("XTmrCtr init green failed\r\n");
        return XST_FAILURE;
    }

    status = XTmrCtr_Initialize(&pwm_Blue, XPAR_TMRCTR_2_DEVICE_ID);
    if (status != XST_SUCCESS) {
        xil_printf("XTmrCtr init blue failed\r\n");
        return XST_FAILURE;
    }

    XTmrCtr_PwmDisable(&pwm_Red);
    XTmrCtr_PwmDisable(&pwm_Green);
    XTmrCtr_PwmDisable(&pwm_Blue);

    return XST_SUCCESS;
}

void set_pwm(u8 r, u8 g, u8 b)
{
    u32 hr = (u32) ((r * PWM_PERIOD) / 100U);
    u32 hg = (u32) ((g * PWM_PERIOD) / 100U);
    u32 hb = (u32) ((b * PWM_PERIOD) / 100U);

    XTmrCtr_PwmDisable(&pwm_Red);
    XTmrCtr_PwmDisable(&pwm_Green);
    XTmrCtr_PwmDisable(&pwm_Blue);

    if (r > 0) {
        XTmrCtr_PwmConfigure(&pwm_Red, PWM_PERIOD, hr);
        XTmrCtr_PwmEnable(&pwm_Red);
    }

    if (g > 0) {
        XTmrCtr_PwmConfigure(&pwm_Green, PWM_PERIOD, hg);
        XTmrCtr_PwmEnable(&pwm_Green);
    }

    if (b > 0) {
        XTmrCtr_PwmConfigure(&pwm_Blue, PWM_PERIOD, hb);
        XTmrCtr_PwmEnable(&pwm_Blue);
    }
}

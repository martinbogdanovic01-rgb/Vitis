#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xadcps.h"
#include "xtmrctr.h"
#include "xil_types.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xil_exception.h"
#include "xtime_l.h"          // for XTime_GetTime
#include "timer_f.h"
#include "config.h"

#define PWM_PERIOD_NS     20000000u   // 20ms = 50Hz
#define MIN_ANGLE         0
#define MAX_ANGLE         180
#define MIN_PULSE_US      500
#define MAX_PULSE_US      2400
#define MINSUNLIGHT       100
#define SMOOTH_FACTOR     0.2f
#define STEP_SIZE         1
#define DEADZONE          50

#define TIMER_H_INSTANCE  1
#define TIMER_V_INSTANCE  2

#define ADC_CH_N A0
#define ADC_CH_W A1
#define ADC_CH_S A2
#define ADC_CH_E A3

static XAdcPs Xadc;
static XTmrCtr TimerH, TimerV;

static int clamp(int v, int min, int max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static int map_int(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static u32 angle_to_ns(int angle) {
    int us = map_int(angle, MIN_ANGLE, MAX_ANGLE, MIN_PULSE_US, MAX_PULSE_US);
    return (u32)(us * 1000);
}

static int setup_pwm(XTmrCtr *T, int id, int start_angle) {
    if (PwmInit(T, id) != XST_SUCCESS) {
        xil_printf("PWM init failed on timer %d\r\n", id);
        return XST_FAILURE;
    }
    PwmConfig(T, PWM_PERIOD_NS, angle_to_ns(start_angle));
    return XST_SUCCESS;
}

static void set_pwm_angle(XTmrCtr *T, int angle) {
    PwmConfig(T, PWM_PERIOD_NS, angle_to_ns(angle));
}

static int read_adc(XAdcPs *dev, u32 ch) {
    return (int)(XAdcPs_GetAdcData(dev, ch) & 0x0FFF);
}

int main() {
    init_platform();
    xil_printf("\r\n=== PYNQ-Z2 Solar Tracker using XTime ===\r\n");

    // --- Initialize XADC ---
    XAdcPs_Config *Cfg = XAdcPs_LookupConfig(XPAR_XADCPS_0_DEVICE_ID);
    XAdcPs_CfgInitialize(&Xadc, Cfg, Cfg->BaseAddress);
    XAdcPs_SetSequencerMode(&Xadc, XADCPS_SEQ_MODE_CONTINPASS);
    XAdcPs_SetAlarmEnables(&Xadc, 0);

    u32 chanMask = (1 << (ADC_CH_N - XADCPS_CH_AUX_MIN)) |
                   (1 << (ADC_CH_W - XADCPS_CH_AUX_MIN)) |
                   (1 << (ADC_CH_S - XADCPS_CH_AUX_MIN)) |
                   (1 << (ADC_CH_E - XADCPS_CH_AUX_MIN));
    XAdcPs_SetSeqChEnables(&Xadc, chanMask);

    // --- Initialize PWM for both servos ---
    int servoH = 90, servoV = 90;
    setup_pwm(&TimerH, TIMER_H_INSTANCE, servoH);
    setup_pwm(&TimerV, TIMER_V_INSTANCE, servoV);

    float smooth[4] = {0, 0, 0, 0};
    int val[4] = {0, 0, 0, 0};

    xil_printf("System ready.\r\n");

    XTime tBefore, tAfter;
    XTime_GetTime(&tBefore);

    while (1) {
        // --- Read sensors ---
        int raw[4];
        raw[0] = read_adc(&Xadc, ADC_CH_N);
        raw[1] = read_adc(&Xadc, ADC_CH_W);
        raw[2] = read_adc(&Xadc, ADC_CH_S);
        raw[3] = read_adc(&Xadc, ADC_CH_E);

        // --- Smooth readings ---
        for (int i = 0; i < 4; i++) {
            smooth[i] += SMOOTH_FACTOR * (raw[i] - smooth[i]);
            val[i] = (int)smooth[i];
            if (val[i] < MINSUNLIGHT) val[i] = MINSUNLIGHT;
        }

        // --- Calculate differences ---
        int diffH = val[3] - val[1];  // E - W
        int diffV = val[0] - val[2];  // N - S

        // --- Adjust servo angles ---
        if (abs(diffH) > DEADZONE)
            servoH += (diffH > 0) ? STEP_SIZE : -STEP_SIZE;

        if (abs(diffV) > DEADZONE)
            servoV += (diffV > 0) ? STEP_SIZE : -STEP_SIZE;

        servoH = clamp(servoH, MIN_ANGLE, MAX_ANGLE);
        servoV = clamp(servoV, MIN_ANGLE, MAX_ANGLE);

        // --- Update PWM output ---
        set_pwm_angle(&TimerH, servoH);
        set_pwm_angle(&TimerV, servoV);

        // --- Measure elapsed time ---
        XTime_GetTime(&tAfter);
        double seconds = (double)(tAfter - tBefore) / COUNTS_PER_SECOND;

        xil_printf("N:%4d W:%4d S:%4d E:%4d | H:%3d° V:%3d° | Time: %.6f s\r\n",
                   val[0], val[1], val[2], val[3], servoH, servoV, seconds);

        // reset timer for next iteration
        XTime_GetTime(&tBefore);
    }

    // --- Cleanup ---
    XTime_GetTime(&tAfter);
    printf("\nElapsed time (CPU clock ticks): %llu\n", tAfter - tBefore);
    printf("Elapsed time (seconds): %f\n", 1.0 * (tAfter - tBefore) / COUNTS_PER_SECOND);

    cleanup_platform();
    return 0;
}

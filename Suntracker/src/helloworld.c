#include "platform.h"
#include "xscugic.h"
#include "xtmrctr.h"
#include "xgpio.h"
#include "xadcps.h"
#include "config.h"

/* ================= CONFIG ================= */
#define DEAD_BAND   100
#define CTRL_PERIOD (PB_FRQ / 10)    // 100 ms @ 100 MHz
#define PWM_FREQ    20000            // 20 kHz

/* Direction GPIO bits */
#define DIR_UP      BIT0
#define DIR_DOWN    BIT1
#define DIR_RIGHT   BIT2
#define DIR_LEFT    BIT3

/* Overcurrent bits */
#define OC_HORIZ    BIT0
#define OC_VERT     BIT1

/* ================= DEVICES ================= */
XTmrCtr Tmr0;      // Horizontal PWM timer
XTmrCtr Tmr1;      // Vertical PWM timer
XScuGic Gic;
XGpio GPIO_DIR;
XGpio GPIO_OC;
XGpio GPIO_7SEG;
XAdcPs XAdc;

/* ================= GLOBALS ================= */
volatile int ctrlFlag = 0;
volatile int dutyH = 0;    // 0-100%
volatile int dutyV = 0;    // 0-100%
volatile int pwmCntH = 0;
volatile int pwmCntV = 0;
volatile int faultH = 0;
volatile int faultV = 0;

/* ================= 7-SEG MAP (common-anode) ================= */
const u8 segMap[] = {
    0x3F, // 0
    0x38, // L
    0x50, // r
    0x3E, // U
    0x5E, // d
    0x79  // E
};

/* ================= INTERRUPTS ================= */
void CtrlISR(void *CallBackRef, u8 TmrCtrNumber)
{
    if (TmrCtrNumber == 0) ctrlFlag = 1;
}

/* ================= PWM LOGIC ================= */
void PWM_Update()
{
    // Horizontal PWM
    if (!faultH) {
        pwmCntH++;
        if (pwmCntH < dutyH) {
            XGpio_DiscreteSet(&GPIO_DIR, 1, (XGpio_DiscreteRead(&GPIO_DIR,1) & (DIR_LEFT|DIR_RIGHT)));
        } else {
            XGpio_DiscreteClear(&GPIO_DIR, 1, DIR_LEFT|DIR_RIGHT);
        }
        if (pwmCntH >= 100) pwmCntH = 0;
    } else {
        XGpio_DiscreteClear(&GPIO_DIR, 1, DIR_LEFT|DIR_RIGHT);
    }

    // Vertical PWM
    if (!faultV) {
        pwmCntV++;
        if (pwmCntV < dutyV) {
            XGpio_DiscreteSet(&GPIO_DIR, 1, (XGpio_DiscreteRead(&GPIO_DIR,1) & (DIR_UP|DIR_DOWN)));
        } else {
            XGpio_DiscreteClear(&GPIO_DIR, 1, DIR_UP|DIR_DOWN);
        }
        if (pwmCntV >= 100) pwmCntV = 0;
    } else {
        XGpio_DiscreteClear(&GPIO_DIR, 1, DIR_UP|DIR_DOWN);
    }
}

/* ================= OVERCURRENT ================= */
void CheckOvercurrent()
{
    u32 oc = XGpio_DiscreteRead(&GPIO_OC, 1);

    if (oc & OC_HORIZ) {
        faultH = 1;
        dutyH = 0;
    } else faultH = 0;

    if (oc & OC_VERT) {
        faultV = 1;
        dutyV = 0;
    } else faultV = 0;

    if (faultH || faultV) {
        XGpio_DiscreteWrite(&GPIO_7SEG, 1, ~segMap[5]); // E (inverted for common-anode)
    }
}

/* ================= CONTROL LOOP ================= */
void ControlLoop()
{
    CheckOvercurrent();

    int up    = XAdcPs_GetAdcData(&XAdc, A0);
    int down  = XAdcPs_GetAdcData(&XAdc, A1);
    int left  = XAdcPs_GetAdcData(&XAdc, A2);
    int right = XAdcPs_GetAdcData(&XAdc, A3);

    int diffV = up - down;
    int diffH = right - left;

    /* Vertical */
    if (!faultV) {
        if (diffV > DEAD_BAND) {
            XGpio_DiscreteWrite(&GPIO_DIR, 1, DIR_UP);
            dutyV = diffV / 30;
            if (dutyV > 100) dutyV = 100;
            XGpio_DiscreteWrite(&GPIO_7SEG, 1, ~segMap[3]); // U
        } else if (diffV < -DEAD_BAND) {
            XGpio_DiscreteWrite(&GPIO_DIR, 1, DIR_DOWN);
            dutyV = -diffV / 30;
            if (dutyV > 100) dutyV = 100;
            XGpio_DiscreteWrite(&GPIO_7SEG, 1, ~segMap[4]); // d
        } else dutyV = 0;
    }

    /* Horizontal */
    if (!faultH) {
        if (diffH > DEAD_BAND) {
            XGpio_DiscreteWrite(&GPIO_DIR, 1, DIR_RIGHT);
            dutyH = diffH / 30;
            if (dutyH > 100) dutyH = 100;
            XGpio_DiscreteWrite(&GPIO_7SEG, 1, ~segMap[2]); // r
        } else if (diffH < -DEAD_BAND) {
            XGpio_DiscreteWrite(&GPIO_DIR, 1, DIR_LEFT);
            dutyH = -diffH / 30;
            if (dutyH > 100) dutyH = 100;
            XGpio_DiscreteWrite(&GPIO_7SEG, 1, ~segMap[1]); // L
        } else dutyH = 0;
    }

    PWM_Update();
}

/* ================= INIT ================= */
void InitSystem()
{
    // Timers
    XTmrCtr_Initialize(&Tmr0, TMR0_DEVICE_ID);
    XTmrCtr_Initialize(&Tmr1, TMR1_DEVICE_ID);

    XTmrCtr_SetHandler(&Tmr0, CtrlISR, &Tmr0);
    XTmrCtr_SetOptions(&Tmr0, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
    XTmrCtr_SetResetValue(&Tmr0, 0, CTRL_PERIOD);
    XTmrCtr_Start(&Tmr0, 0);

    // GPIO
    XGpio_Initialize(&GPIO_DIR, LEDS_DEVICE_ID);
    XGpio_SetDataDirection(&GPIO_DIR, 1, 0x0);

    XGpio_Initialize(&GPIO_OC, BTNS_DEVICE_ID);
    XGpio_SetDataDirection(&GPIO_OC, 1, 0xFF);

    XGpio_Initialize(&GPIO_7SEG, SWT_DEVICE_ID); // replace with correct 7-segment GPIO if needed
    XGpio_SetDataDirection(&GPIO_7SEG, 1, 0x0);

    // XADC
    XAdcPs_Config *Cfg = XAdcPs_LookupConfig(XADC_DEVICE_ID);
    XAdcPs_CfgInitialize(&XAdc, Cfg, Cfg->BaseAddress);
}

/* ================= MAIN ================= */
int main()
{
    init_platform();
    InitSystem();

    while (1) {
        if (ctrlFlag) {
            ctrlFlag = 0;
            ControlLoop();
        }
    }
}

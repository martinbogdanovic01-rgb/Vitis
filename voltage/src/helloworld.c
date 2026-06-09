/* ---------------------------------------------------------------------------- *
 * Simple Modular Solar Tracker Demo
 * Reads 4 analog sensors, moves tilt/rotation motors, shows info on OLED
 * Student-style, modular code
 * ---------------------------------------------------------------------------- */

#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xadcps.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xil_printf.h"

// Optional OLED
#include "I2Csrc/u8g2.h"
#include "SH1106_Screen.h"

extern u8g2_t u8g2;

// --------------------- Constants ------------------------
#define PWM_PERIOD       50000
#define SENSOR_DEADZONE  1800
#define MIN_SPEED        35
#define MAX_SPEED        75

#define BIT_DIR_UP       (1 << 6)
#define BIT_DIR_DN       (1 << 7)

// --------------------- Global Instances -----------------
static XAdcPs xadc;
static XTmrCtr timer_v;
static XTmrCtr timer_h;
static XGpio gpio_dig;

// Smoothed sensor values
static float sA0=0, sA1=0, sA2=0, sA3=0;

// --------------------- ADC Functions -------------------
int adc_init(void) {
    XAdcPs_Config *cfg = XAdcPs_LookupConfig(XPAR_XADCPS_0_DEVICE_ID);
    if(!cfg) return XST_FAILURE;

    if(XAdcPs_CfgInitialize(&xadc,cfg,cfg->BaseAddress) != XST_SUCCESS) return XST_FAILURE;

    XAdcPs_SetSequencerMode(&xadc,XADCPS_SEQ_MODE_CONTINPASS);
    return XST_SUCCESS;
}

u16 read_sensor(u8 channel) {
    u16 val = XAdcPs_GetAdcData(&xadc, XADCPS_CH_AUX_MIN + channel);
    return val;
}

// --------------------- Motor Functions -----------------
int calc_speed(int diff) {
    int a = abs(diff);
    if(a < SENSOR_DEADZONE) return 0;
    int speed = MIN_SPEED + (a * (MAX_SPEED - MIN_SPEED)) / 30000;
    if(speed > MAX_SPEED) speed = MAX_SPEED;
    return speed;
}

void control_tilt(int diff) {
    int speed = calc_speed(diff);
    if(speed>0) {
        if(diff>0) {
            XGpio_DiscreteSet(&gpio_dig,1,BIT_DIR_UP);
            XGpio_DiscreteClear(&gpio_dig,1,BIT_DIR_DN);
        } else {
            XGpio_DiscreteSet(&gpio_dig,1,BIT_DIR_DN);
            XGpio_DiscreteClear(&gpio_dig,1,BIT_DIR_UP);
        }
        XTmrCtr_PwmConfigure(&timer_v,PWM_PERIOD,(PWM_PERIOD*speed)/100);
        XTmrCtr_PwmEnable(&timer_v);
    } else {
        XTmrCtr_PwmDisable(&timer_v);
        XGpio_DiscreteClear(&gpio_dig,1,BIT_DIR_UP | BIT_DIR_DN);
    }
}

void control_rotation(int diff) {
    int duty = 50;
    if(abs(diff)>SENSOR_DEADZONE) duty = 50 + (diff*35/30000);
    XTmrCtr_PwmConfigure(&timer_h,PWM_PERIOD,(PWM_PERIOD*duty)/100);
    XTmrCtr_PwmEnable(&timer_h);
}

// --------------------- Display Functions -----------------
void display_update(const char* tilt, const char* rot, u16 a0,u16 a1,u16 a2,u16 a3) {
    char buf[32];
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2,0,10,"--- SOLAR PANEL ---");
    u8g2_DrawStr(&u8g2,0,25,"TILT:"); u8g2_DrawStr(&u8g2,40,25,tilt);
    u8g2_DrawStr(&u8g2,0,40,"ROT :"); u8g2_DrawStr(&u8g2,40,40,rot);
    sprintf(buf,"A0:%u A1:%u",a0>>4,a1>>4); u8g2_DrawStr(&u8g2,0,55,buf);
    sprintf(buf,"A2:%u A3:%u",a2>>4,a3>>4); u8g2_DrawStr(&u8g2,0,63,buf);
    u8g2_SendBuffer(&u8g2);
}

// --------------------- Main ----------------------------
int main() {
    init_platform();
    initDisplay();

    if(adc_init()!=XST_SUCCESS) {
        xil_printf("XADC init failed!\r\n");
        return 1;
    }

    XGpio_Initialize(&gpio_dig,XPAR_GPIO_0_DEVICE_ID);
    XGpio_SetDataDirection(&gpio_dig,1,0x00); // all output

    XTmrCtr_Initialize(&timer_v,XPAR_TMRCTR_0_DEVICE_ID);
    XTmrCtr_Initialize(&timer_h,XPAR_TMRCTR_1_DEVICE_ID);

    char *tilt="IDLE";
    char *rot="IDLE";

    while(1) {
        // Read sensors
        u16 a0 = read_sensor(1);
        u16 a1 = read_sensor(9);
        u16 a2 = read_sensor(6);
        u16 a3 = read_sensor(15);

        // Smooth values
        sA0 += 0.25*(a0-sA0);
        sA1 += 0.25*(a1-sA1);
        sA2 += 0.25*(a2-sA2);
        sA3 += 0.25*(a3-sA3);

        // Motor control
        int diffV = (int)sA0 - (int)sA1;
        int diffH = (int)sA2 - (int)sA3;
        control_tilt(diffV);
        control_rotation(diffH);

        tilt = (diffV>0)?"UP":(diffV<0)?"DN":"IDLE";
        rot  = (diffH>0)?"RT":(diffH<0)?"LT":"IDLE";

        // Display
        display_update(tilt,rot,(u16)sA0,(u16)sA1,(u16)sA2,(u16)sA3);
    }

    return 0;
}

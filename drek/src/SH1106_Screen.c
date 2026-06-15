#include "SH1106_Screen.h"
#include "Defines.h"
#include <stdio.h>

u8g2_t u8g2;
XIicPs Iic;
int8_t MaxStrHeight;

void initDisplay() {
    u8g2_Setup_sh1106_i2c_128x64_noname_f(&u8g2, U8G2_R0, cb_HW_I2C_send, cb_gpio_SH1106);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, UsedFont);
    MaxStrHeight = u8g2_GetMaxCharHeight(&u8g2);
    printf("Display Initialized\n");
}

void printDisplay(u8g2_uint_t x, u8g2_uint_t y, const char *str) {
    u8g2_DrawStr(&u8g2, x, y + MaxStrHeight, str);
    u8g2_SendBuffer(&u8g2);
}

void printNew(u8g2_uint_t x, u8g2_uint_t y, const char *str) {
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, x, y + MaxStrHeight, str);
    u8g2_SendBuffer(&u8g2);
}

uint8_t cb_HW_I2C_send(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    static uint8_t buffer[128]; // Increased for safety
    static uint8_t buf_idx;
    uint8_t *data;
    XIicPs_Config *Config;
    int status;

    switch(msg) {
        case U8X8_MSG_BYTE_INIT:
            // Change to _0_ if you are sure your block design uses IIC0
            Config = XIicPs_LookupConfig(XPAR_XIICPS_1_DEVICE_ID);
            XIicPs_CfgInitialize(&Iic, Config, Config->BaseAddress);
            XIicPs_SetSClk(&Iic, IIC_SCLK_RATE);
            break;
        case U8X8_MSG_BYTE_SEND:
            data = (uint8_t *)arg_ptr;
            while(arg_int > 0) {
                buffer[buf_idx++] = *data++;
                arg_int--;
            }
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_idx = 0;
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            status = XIicPs_MasterSendPolled(&Iic, buffer, buf_idx, SlaveAddress);
            if (status != XST_SUCCESS) {
                xil_printf("I2C Error: %d\r\n", status);
            }
            break;
    }
    return 1;
}

uint8_t cb_gpio_SH1106(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    return 1; // Unused for HW I2C
}

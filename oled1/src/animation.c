#include "animation.h"
#include "I2Csrc/u8g2.h"
#include "Defines.h"
#include <stdio.h>
#include <string.h>

extern u8g2_t u8g2;

void animation_startup(void) {
    u8g2_SetFont(&u8g2, UsedFont);
    for (int i = 0; i <= 20; i++) {
        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawStr(&u8g2, 22, 20, "SOLAR TRACKER");
        u8g2_DrawRFrame(&u8g2, 14, 45, 100, 10, 3);
        u8g2_DrawBox(&u8g2, 16, 47, i * 5 - 4, 6);
        u8g2_SendBuffer(&u8g2);
    }
}

void animation_update(const char* tilt, const char* rot, u16 a0, u16 a1, u16 a2, u16 a3) {
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);

    // --- HORIZONTAL BALANCE VISUAL ---
    u8g2_DrawStr(&u8g2, 30, 10, "HORIZ BALANCE");
    u8g2_DrawFrame(&u8g2, 10, 15, 108, 10); // Main bar frame
    u8g2_DrawLine(&u8g2, 64, 15, 64, 25);   // Center point

    // Calculate position (L=a2, R=a3).
    // Shift the center cursor based on difference
    int diff = ((int)a2 - (int)a3) / 40; // Sensitivity scale
    if (diff > 50) diff = 50;
    if (diff < -50) diff = -50;

    u8g2_DrawBox(&u8g2, 64 + diff, 16, 4, 8); // The moving cursor
    u8g2_DrawStr(&u8g2, 2, 24, "L");
    u8g2_DrawStr(&u8g2, 120, 24, "R");

    // --- MOTOR ARROWS ---
    // If Horizontal is moving, draw a big arrow
    if (strstr(rot, "RIGHT")) {
        u8g2_DrawTriangle(&u8g2, 110, 35, 125, 40, 110, 45); // Right Arrow
        u8g2_DrawBox(&u8g2, 100, 38, 10, 4);
    }
    if (strstr(rot, "LEFT")) {
        u8g2_DrawTriangle(&u8g2, 18, 35, 3, 40, 18, 45);    // Left Arrow
        u8g2_DrawBox(&u8g2, 18, 38, 10, 4);
    }

    // --- STATUS TEXT ---
    u8g2_DrawStr(&u8g2, 0, 40, rot);
    u8g2_DrawStr(&u8g2, 0, 52, tilt);

    // --- SENSOR RAW VALUES (Debug) ---
    char buf[32];
    sprintf(buf, "L:%u R:%u", a2>>4, a3>>4); // Shows values from 0-4095
    u8g2_DrawStr(&u8g2, 0, 64, buf);

    u8g2_SendBuffer(&u8g2);
}

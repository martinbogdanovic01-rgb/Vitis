#include "oled_ui.h"
#include "SH1106_Screen.h"
#include "I2Csrc/u8g2.h"
#include "xadc.h"
#include "sleep.h"
#include <stdio.h>

extern u8g2_t u8g2;
extern int8_t MaxStrHeight;

#define TRACKER_CX  110
#define TRACKER_CY   51
#define TRACKER_R    11

/* ---- Startup animation ---- */

static void anim_ripple(void) {
    for (int r = 4; r <= 28; r += 6) {
        u8g2_ClearBuffer(&u8g2);
        for (int i = 4; i <= r; i += 6)
            u8g2_DrawCircle(&u8g2, 64, 32, i, U8G2_DRAW_ALL);
        u8g2_SendBuffer(&u8g2);
        usleep(90000);
    }
}

static void anim_title_reveal(void) {
    const char *line1 = "SOLAR";
    const char *line2 = "TRACKER";
    u8g2_SetFont(&u8g2, u8g2_font_t0_11_me);

    for (int n = 1; n <= 5; n++) {
        char buf[8] = {0};
        for (int i = 0; i < n; i++) buf[i] = line1[i];
        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawCircle(&u8g2, 64, 48, 10, U8G2_DRAW_ALL);
        u8g2_DrawStr(&u8g2, 42, 22, buf);
        u8g2_SendBuffer(&u8g2);
        usleep(70000);
    }
    for (int n = 1; n <= 7; n++) {
        char buf[8] = {0};
        for (int i = 0; i < n; i++) buf[i] = line2[i];
        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawCircle(&u8g2, 64, 48, 10, U8G2_DRAW_ALL);
        u8g2_DrawStr(&u8g2, 42, 22, "SOLAR");
        u8g2_DrawStr(&u8g2, 36, 34, buf);
        u8g2_SendBuffer(&u8g2);
        usleep(70000);
    }
}

static void anim_loading(void) {
    u8g2_SetFont(&u8g2, u8g2_font_t0_11_me);
    for (int w = 0; w <= 100; w += 10) {
        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawStr(&u8g2, 18, 11, "SOLAR TRACKER");
        u8g2_DrawHLine(&u8g2, 0, 14, 128);
        u8g2_DrawStr(&u8g2, 14, 33, "Initializing...");
        u8g2_DrawFrame(&u8g2, 14, 40, 100, 9);
        if (w > 0) u8g2_DrawBox(&u8g2, 15, 41, w, 7);
        u8g2_SendBuffer(&u8g2);
        usleep(80000);
    }
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, 18, 11, "SOLAR TRACKER");
    u8g2_DrawHLine(&u8g2, 0, 14, 128);
    u8g2_DrawFrame(&u8g2, 0, 0, 128, 64);
    u8g2_DrawFrame(&u8g2, 14, 40, 100, 9);
    u8g2_DrawBox(&u8g2, 15, 41, 100, 7);
    u8g2_DrawStr(&u8g2, 26, 30, "SYSTEM  READY!");
    u8g2_SendBuffer(&u8g2);
    sleep(1);
}

void OLED_Init(void) {
    initDisplay();
    anim_ripple();
    anim_title_reveal();
    anim_loading();
}

/* ---- Runtime status display ---- */

static void draw_sensor_bars(void) {
    const char *labels[] = {"T", "B", "L", "R"};
    for (int i = 0; i < 4; i++) {
        int bx = 1 + i * 16;
        int by = 46;
        int bh = 17;
        int bw = 12;

        u8g2_DrawStr(&u8g2, bx + 2, by - 1, labels[i]);

        u8g2_DrawFrame(&u8g2, bx, by, bw, bh);
        float v = sensor_voltages[i];
        if (v > 1.0f) v = 1.0f;
        if (v < 0.0f) v = 0.0f;
        int fill = (int)(v * (float)(bh - 2));
        if (fill > 0)
            u8g2_DrawBox(&u8g2, bx + 1, by + (bh - 1 - fill), bw - 2, fill);
    }
}

static void draw_crosshair(float hor_diff, float ver_diff) {
    int cx = TRACKER_CX;
    int cy = TRACKER_CY;
    int r  = TRACKER_R;

    u8g2_DrawCircle(&u8g2, cx, cy, r, U8G2_DRAW_ALL);
    u8g2_DrawHLine(&u8g2, cx - r, cy, 2 * r + 1);
    u8g2_DrawVLine(&u8g2, cx, cy - r, 2 * r + 1);

    int dx = (int)(hor_diff  * (float)(r - 2) / 0.4f);
    int dy = (int)(-ver_diff * (float)(r - 2) / 0.4f);
    if (dx >  r - 2) dx =  r - 2;
    if (dx < -(r-2)) dx = -(r-2);
    if (dy >  r - 2) dy =  r - 2;
    if (dy < -(r-2)) dy = -(r-2);

    u8g2_DrawBox(&u8g2, cx + dx - 1, cy + dy - 1, 3, 3);
}

static const char* hor_dir_str(MotorDirection dir) {
    switch (dir) {
        case MOVE_POSITIVE_FAST: return ">>>FAST";
        case MOVE_POSITIVE_SLOW: return "> SLOW ";
        case MOVE_NEGATIVE_FAST: return "<<<FAST";
        case MOVE_NEGATIVE_SLOW: return "< SLOW ";
        default:                  return "= HOLD ";
    }
}

static const char* ver_dir_str(MotorDirection dir) {
    switch (dir) {
        case MOVE_POSITIVE_FAST: return "^^^FAST";
        case MOVE_POSITIVE_SLOW: return "^ SLOW ";
        case MOVE_NEGATIVE_FAST: return "vvvFAST";
        case MOVE_NEGATIVE_SLOW: return "v SLOW ";
        default:                  return "= HOLD ";
    }
}

void OLED_UpdateStatus(float ver_diff, float hor_diff,
                       MotorDirection ver_dir, MotorDirection hor_dir) {
    char buf[16];

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_t0_11_me);

    /* Header */
    u8g2_DrawStr(&u8g2, 18, 11, "SOLAR TRACKER");
    u8g2_DrawHLine(&u8g2, 0, 13, 128);

    /* Horizontal motor row */
    snprintf(buf, sizeof(buf), "H:%+.2f", hor_diff);
    u8g2_DrawStr(&u8g2, 0, 24, buf);
    u8g2_DrawStr(&u8g2, 56, 24, hor_dir_str(hor_dir));

    /* Vertical motor row */
    snprintf(buf, sizeof(buf), "V:%+.2f", ver_diff);
    u8g2_DrawStr(&u8g2, 0, 35, buf);
    u8g2_DrawStr(&u8g2, 56, 35, ver_dir_str(ver_dir));

    /* Divider */
    u8g2_DrawHLine(&u8g2, 0, 38, 128);

    /* Vertical separator between sensor bars and crosshair */
    u8g2_DrawVLine(&u8g2, 68, 38, 26);

    /* Sensor voltage bars (T/B/L/R sensors) */
    draw_sensor_bars();

    /* Sun position tracker crosshair */
    draw_crosshair(hor_diff, ver_diff);

    u8g2_SendBuffer(&u8g2);
}

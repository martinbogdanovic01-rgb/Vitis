#include "sensors.h"
#include "xadc.h"
#include "motorMoveFunc.h"
#include "oled_ui.h"
#include "config.h"
#include <stdio.h>
#include <math.h>

void update_motor_logic() {
    float ver_diff = sensor_voltages[0] - sensor_voltages[1];
    float hor_diff = sensor_voltages[2] - sensor_voltages[3];

    printf("V_DIFF:%1.3f | H_DIFF:%1.3f\r", ver_diff, hor_diff);

    MotorDirection hor_dir, ver_dir;

    // Horizontal Logic
    if (hor_diff > MOVE_FAST_TH)            hor_dir = MOVE_POSITIVE_FAST;
    else if (hor_diff > MOVE_DEADZONE_TH)   hor_dir = MOVE_POSITIVE_SLOW;
    else if (hor_diff < -MOVE_FAST_TH)      hor_dir = MOVE_NEGATIVE_FAST;
    else if (hor_diff < -MOVE_DEADZONE_TH)  hor_dir = MOVE_NEGATIVE_SLOW;
    else                                     hor_dir = MOVE_NONE;

    // Vertical Logic
    if (ver_diff > MOVE_FAST_TH)            ver_dir = MOVE_POSITIVE_FAST;
    else if (ver_diff > MOVE_DEADZONE_TH)   ver_dir = MOVE_POSITIVE_SLOW;
    else if (ver_diff < -MOVE_FAST_TH)      ver_dir = MOVE_NEGATIVE_FAST;
    else if (ver_diff < -MOVE_DEADZONE_TH)  ver_dir = MOVE_NEGATIVE_SLOW;
    else                                     ver_dir = MOVE_NONE;

    moveHorMotor(hor_dir);
    moveVerMotor(ver_dir);

    OLED_UpdateStatus(ver_diff, hor_diff, ver_dir, hor_dir);
}

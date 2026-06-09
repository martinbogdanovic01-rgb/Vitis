#include "sensors.h"
#include "xadc.h"
#include "motorMoveFunc.h"
#include "config.h"
#include <stdio.h>
#include <math.h> // for fabsf

void update_motor_logic() {
    float ver_diff = sensor_voltages[0] - sensor_voltages[1];
    float hor_diff = sensor_voltages[2] - sensor_voltages[3];

    printf("V_DIFF:%1.3f | H_DIFF:%1.3f\r", ver_diff, hor_diff);

    // Horizontal Logic
    if (hor_diff > MOVE_FAST_TH)           moveHorMotor(MOVE_POSITIVE_FAST);
    else if (hor_diff > MOVE_DEADZONE_TH)  moveHorMotor(MOVE_POSITIVE_SLOW);
    else if (hor_diff < -MOVE_FAST_TH)      moveHorMotor(MOVE_NEGATIVE_FAST);
    else if (hor_diff < -MOVE_DEADZONE_TH) moveHorMotor(MOVE_NEGATIVE_SLOW);
    else                                   moveHorMotor(MOVE_NONE);

    // Vertical Logic
    if (ver_diff > MOVE_FAST_TH)           moveVerMotor(MOVE_POSITIVE_FAST);
    else if (ver_diff > MOVE_DEADZONE_TH)  moveVerMotor(MOVE_POSITIVE_SLOW);
    else if (ver_diff < -MOVE_FAST_TH)      moveVerMotor(MOVE_NEGATIVE_FAST);
    else if (ver_diff < -MOVE_DEADZONE_TH) moveVerMotor(MOVE_NEGATIVE_SLOW);
    else                                   moveVerMotor(MOVE_NONE);
}

#ifndef OLED_UI_H
#define OLED_UI_H

#include "motorMoveFunc.h"

void OLED_Init(void);
void OLED_UpdateStatus(float ver_diff, float hor_diff, MotorDirection ver_dir, MotorDirection hor_dir);

#endif

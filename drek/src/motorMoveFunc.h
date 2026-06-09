#ifndef MOTORMOVEFUNC_H
#define MOTORMOVEFUNC_H

typedef enum {
    MOVE_NONE,
    MOVE_POSITIVE_SLOW,
    MOVE_POSITIVE_FAST,
    MOVE_NEGATIVE_SLOW,
    MOVE_NEGATIVE_FAST
} MotorDirection;

void moveHorMotor(MotorDirection dir);
void moveVerMotor(MotorDirection dir);

#endif

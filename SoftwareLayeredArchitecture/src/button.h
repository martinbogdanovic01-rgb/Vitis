#ifndef ABC_H
#define ABC_H

#include <stdbool.h>
#include "xparameters.h"

#define BTNS_DEVICE_ID XPAR_USER_BTNS_GPIO_DEVICE_ID

void abc_init();

bool abc_get_mode();

#endif


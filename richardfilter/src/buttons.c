#include "buttons.h"
#include "xgpio.h"

#define BTNS_DEVICE_ID XPAR_GPIO_2_DEVICE_ID

XGpio buttons;

void init_buttons()
{
	XGpio_Initialize(&buttons, BTNS_DEVICE_ID);
	XGpio_SetDataDirection(&buttons, 1, 0xF);
}

int read_buttons()
{
    int btn_value = XGpio_DiscreteRead(&buttons, 1);

    if (btn_value & 0x1) return 1;  // BTN1
    if (btn_value & 0x2) return 2;  // BTN2
    if (btn_value & 0x4) return 3;  // BTN3
    if (btn_value & 0x8) return 4;  // BTN4 (optional)

    return 0; // no button pressed
}

/* Exercise to practice Software Layered Architecture diagrams. See instructions.txt
*
* Author: Brice Guayrin (b.guayrin@fontys.nl)
*
* Date: March 2025
*/

#include "button.h"

#include "xgpio.h"

static XGpio btns;
static bool mode = false;


void abc_init() {
	XGpio_Initialize(&btns, BTNS_DEVICE_ID);

	XGpio_SetDataDirection(&btns, 1, 0xF);
}

bool abc_get_mode() {
	// !!! Very simple/inefficient detection of button presses using "level detection" !!!
	// !!! In assignments and MO7 project you are expected to use "edge detection" and NOT "level detection" !!!
	u32 buttonState = XGpio_DiscreteRead(&btns, 1);

	if (buttonState == 0x1) {
    	mode = !mode;
	}

	return mode;
}


/* Exercise to practice Software Layered Architecture diagrams. See instructions.txt
*
* Author: Brice Guayrin (b.guayrin@fontys.nl)
*
* Date: Marrch 2025
*/

#include <stdio.h>
#include <sleep.h>
#include <stdbool.h>

#include "button.h"
#include "platform.h"

#include "voltage.h"

int main()
{
    init_platform();

    abc_init();
    xyz_init();

    while (1) {

    	bool mode = abc_get_mode();

    	if (mode == true) {
    		float voltage = xyz_get_voltage(1); // channel 1 of XAdc (pin A0)
    		printf("Voltage: %.5f (Volts)\n\r", voltage);
    	}
    	else {
    		printf("Voltmeter disabled\n\r");
    	}

    	usleep(100000);
    }

    cleanup_platform();
    return 0;
}

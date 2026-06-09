/* ---------------------------------------------------------------------------- *
 * (Very) simple example of polling the global timer
 *
 * Note : The user of this demo code should NOT expect perfect code.
 * The demo code DOES have (probably) room for further improvement/refactoring.
 * Furthermore, the authors of this demo have done their best to test the code.
 * However they may have overlooked conditions in which the demo code may not
 * operate properly. If you notice an error or bug or improvement opportunity,
 * please be so kind to constructively report this to the authors of the demo code.
 *
 * Authors:
 * 		Brice Guayrin, b.guayrin@fontys.nl
 *		Harold Benton, h.benton@fontys.nl
 *
 * Date: November 2025
 * ---------------------------------------------------------------------------- */
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xtime_l.h"

int main()
{
    XTime current_time;
    XTime last_time = 0;
    const XTime DELAY = COUNTS_PER_SECOND;
    int count = 0;

	init_platform();

    while(1)
    {
        XTime_GetTime(&current_time);

    	if ( (current_time - last_time) > DELAY) {
    		printf("You executed the program since %d seconds\r\n", count++);
    		last_time = current_time;
		}
    }

    cleanup_platform();
    return 0;
}

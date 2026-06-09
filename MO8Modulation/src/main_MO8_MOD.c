#include <stdio.h>
#include <stdint.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "sleep.h"

#include "xspips.h"
#include <stdint.h>
#include "xstatus.h"
#include "MOD_CC1101_settings.h"
#include "MOD_CC1101_SPI.h"

int main(void)
{
	init_platform();

	createInputBuffer();

	if (CC1101_Init_Check() != 0)
	{
		xil_printf("System halt: CC1101 transceiver not found.\r\n");
		return -1; // Stop execution here
	}

	CC1101_Setup_GFSK();
	CC1101_Strobe(STX_ADR);

	xil_printf("CC1101 in TX mode\r\n");
	usleep(1000000); // Sleep for 1 sec

	while (1)
	{
		transmitChunks();
	    usleep(2000000); // Sleep for 2 sec
	}

	cleanup_platform();
	return 0;
}

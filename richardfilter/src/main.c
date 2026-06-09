/* ---------------------------------------------------------------------------- *
 * Demo of a (simple) 2-tap Moving Average Filter applied on the right audio input only
 *
 *
 * Expected results: a 2-tap moving average filter is a rather smooth filter. As
 * such you will probably not hear an audible effect when applying the 2-tap moving average.
 * To hear an audible effect you will need to extend this code to be using more taps.
 *
 *
 * !!!WARNING!!!: To protect your hears we strongly recommend to NEVER
 * wear your headset while uploading the program to the PynZ2 board. Instead
 * we do recommend to follow the steps here-after:
 * - remove your headset
 * - stop/pause playing the audio input (e.g. wav file, etc)
 * - build the program
 * - upload the program
 * - start/resume playing the audio input (e.g. wav file, etc)
 * - set the volume of the audio input to the minimum
 * - place the headset on your hears and progressively increase the volume
 *
 *
 * Note : This demo code exemplifies the implementation of (simple) audio filter for
 * educational purpose. The user of this demo code should NOT expect
 * perfect code. The demo code DOES (probably) have room for further improvement/refactoring.
 * Furthermore, the authors of this demo have done their best to test the code.
 * However they may have overlooked conditions in which the demo code may not
 * operate properly. If you notice an error or bug or improvement opportunity,
 * please be so kind to constructively report this to the authors of the demo code.
 *
 *
 * Authors:
 * 		Brice Guayrin, b.guayrin@fontys.nl
 * 		Jeedella Jeedella, j.jeedella@fontys.nl
 *
 *
 * Date: March 2025
 * ---------------------------------------------------------------------------- */

#include <stdio.h>
#include <xil_io.h>
#include "platform.h"
#include "xil_printf.h"
#include "audio_codec.h"

#include "filter.h"
#include "buttons.h"

int main(void)
{
	xil_printf("Entering Main\r\n");

	// Initialise platform
	init_platform();

	// Configure the IIC data structure
	IicConfig(XPAR_XIICPS_1_DEVICE_ID);

	// Configure the Audio Codec's PLL
	AudioPllConfig();

	// Configure the Line in and Line out ports.
	// Call LineInLineOutConfig() for a configuration that
	// Enables the HP jack too.
	AudioConfigureJacks();

	xil_printf("ADAU1761 configured\n\r");

	init_buttons();

	while (1) {
		determine_state();
		process_audio();
	}

    return 1;
}




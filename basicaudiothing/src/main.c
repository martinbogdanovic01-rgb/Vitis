/* ---------------------------------------------------------------------------- *
 * (Very) simple demo of streaming audio samples
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
 * Note : This demo code exemplifies the streamin of audio samples for
 * educational purpose. The user of this demo code should NOT expect
 * perfect code. The demo code DOES (probably) have room for further improvement/refactoring.
 * Furthermore, the authors of this demo have done their best to test the code.
 * However they may have overlooked conditions in which the demo code may not
 * operate properly. If you notice an error or bug or improvement opportunity,
 * please be so kind to constructively report this to the authors of the demo code.
 *
 *
 * Authors:
 * 		Jeedella Jeedella, j.jeedella@fontys.nl
 *		Brice Guayrin, b.guayrin@fontys.nl
 *
 * Date: February 2025
 * ---------------------------------------------------------------------------- */
#include <stdio.h>
#include <xil_io.h>
#include "platform.h"
#include "xil_printf.h"
#include "audio_codec.h"

/* ---------------------------------------------------------------------------- *
 * 									main()										*
 * ---------------------------------------------------------------------------- *
 * Runs all initial setup functions to initialise the audio codec and IP
 * peripherals, before calling the interactive menu system.
 * ---------------------------------------------------------------------------- */
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

	// stream audio
	u32  in_left, in_right;
	while (1) {
		// Read audio input from codec
		in_left = Xil_In32(I2S_DATA_RX_L_REG);
		in_right = Xil_In32(I2S_DATA_RX_R_REG);

		// Write audio output to codec
		Xil_Out32(I2S_DATA_TX_L_REG, in_left);
		Xil_Out32(I2S_DATA_TX_R_REG, in_right);
	}

    return 1;
}




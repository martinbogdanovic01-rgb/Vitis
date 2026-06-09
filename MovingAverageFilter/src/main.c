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

int main(void)
{
	// Local variables
	s32 prev_in_right0_s32 = 0;
	s32 prev_in_right1_s32 = 0;
	s32 prev_in_right2_s32 = 0;

	// Buffer to store previous samples
	u32 status_reg = 0;			// Buffer to store status register I2S_STATUS_REG (see https://byu-cpe.github.io/ecen427/documentation/audio-hw/)
	u8 is_data_ready = 0;		// unsigned 8 bit integer used as a boolean to indicate if new audio sample is available from I2S
								// 1: new sample available
								// 0: new sample not (yet) available

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

	while (1) {
		/*
		 * Wait for a new audio sample to be available (48KHz)
		 */
		while (is_data_ready == 0) {
			status_reg = Xil_In32(I2S_STATUS_REG); 	 // A new audio sample is available when bit21 of I2S_STATUS_REG becomes 1 (see https://byu-cpe.github.io/ecen427/documentation/audio-hw/)

			is_data_ready = (status_reg >> 21 ) & 1; // Read bit21 of register I2S_STATUS_REG
		}
		is_data_ready = 0;
		status_reg = status_reg & (u32)(~(1<<21)); // Clear bit21 of I2S_STATUS_REG, i.e. set bit21 to 0
		Xil_Out32(I2S_STATUS_REG, status_reg);


		/*
		 *  Read audio input from codec
		 */
		u32 in_right = Xil_In32(I2S_DATA_RX_R_REG);


		/*
		 *  Moving average filter
		 */
		s32 in_right_s32 = in_right<<8; // Why is it needed to shift by 8 bits?

		s64 acc = in_right_s32 + prev_in_right0_s32 + prev_in_right1_s32 + prev_in_right2_s32  ; // accumulate the current audio sample and previous audio sample

		acc = acc >> 2;

		u32 out_right = acc>>8;


		prev_in_right2_s32= prev_in_right1_s32;
		prev_in_right1_s32= prev_in_right0_s32;
		prev_in_right0_s32 = in_right_s32; // store current audio input samples

		/*
		 * Write audio output to codec
		 */
		Xil_Out32(I2S_DATA_TX_R_REG, out_right);
	}

    return 1;
}




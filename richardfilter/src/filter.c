#include <stdio.h>
#include <xil_io.h>
#include "filter.h"
#include "buttons.h"
#include "audio_codec.h"


#define MOVING_AVG_TAPS 16
#define DIVISION 4

#define FIR_TAPS 9 // 9 taps, so 8th order FIR filter

system_state_t current_state = STATE_LOOPBACK;

u32 in_right;
u32 in_left;

s32 in_right_s32;
s32 in_left_s32;

u32 out_right;
u32 out_left;

// Local variables
u32 status_reg = 0;
u8 is_data_ready = 0;

const double fir_coeffs[FIR_TAPS] = {0.017503166976208, 0.047943479190253, 0.122314130002404, 0.197681263858199,
		0.229115919945871, 0.197681263858199, 0.122314130002404, 0.047943479190253, 0.017503166976208};

const double bandp_coeffs[FIR_TAPS] = {0.015644213239977, -0.064649675623111, -0.211196878739695, 0.104704840818973,
		0.472924400476926, 0.104704840818973, -0.211196878739695, -0.064649675623111, 0.015644213239977};

s32 buffer_right[MOVING_AVG_TAPS] = {0};
s32 buffer_left[MOVING_AVG_TAPS] = {0};

s32 fir_buffer_right[FIR_TAPS] = {0};
s32 fir_buffer_left[FIR_TAPS] = {0};

s32 bandp_buffer_right[FIR_TAPS] = {0};
s32 bandp_buffer_left[FIR_TAPS] = {0};

int ma_index = 0;
int fir_index = 0;
int bandp_index = 0;

void determine_state()
{
	switch(read_buttons())
	{
	case 1:
		current_state = STATE_LOOPBACK;
		break;
	case 2:
		current_state = STATE_MOVING_AVG;
		break;
	case 3:
		current_state = STATE_FIR;
		break;
	case 4:
		current_state = STATE_BANDP;
		break;
	}
}

void process_audio()
{
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


	in_right = Xil_In32(I2S_DATA_RX_R_REG);
	in_left = Xil_In32(I2S_DATA_RX_L_REG);

	switch(current_state)
	{
	case STATE_LOOPBACK:
		run_audio_loopback();
		break;
	case STATE_MOVING_AVG:
		run_moving_avg_filter();
		break;
	case STATE_FIR:
		run_fir_filter();
		break;
	case STATE_BANDP:
		run_bandpass_filter();
		break;
	default:
		break;
	}

	Xil_Out32(I2S_DATA_TX_R_REG, out_right);
	Xil_Out32(I2S_DATA_TX_L_REG, out_left);
}

void run_audio_loopback()
{
	out_right = in_right;
	out_left = in_left;
}

void run_moving_avg_filter()
{
	in_right_s32 = in_right << 8; // Shift from 24 bits to 32 bits
	in_left_s32 = in_left << 8;

	// Store sample in buffer
	buffer_right[ma_index] = in_right_s32;
	buffer_left[ma_index] = in_left_s32;

	s64 acc_right = 0;
	s64 acc_left = 0;

	// Sum all MOVING_AVG_TAPS
	for(int i = 0; i < MOVING_AVG_TAPS; i++)
	{
		acc_right += buffer_right[i];
		acc_left += buffer_left[i];
	}

	// Average
	acc_right = acc_right >> DIVISION;   // shift by log2(MOVING_AVG_TAPS)
	acc_left = acc_left >> DIVISION;   // shift by log2(MOVING_sAVG_TAPS)

	// Update circular index
	ma_index = (ma_index + 1) % MOVING_AVG_TAPS;
	out_right = acc_right >> 8;
	out_left = acc_left >> 8;
}

void run_fir_filter()
{
	in_right_s32 = in_right << 8;
	in_left_s32 = in_left << 8;

	// fir_index responsible for storing the input
	fir_buffer_right[fir_index] = in_right_s32;
	fir_buffer_left[fir_index] = in_left_s32;

	s64 acc_right = 0;
	s64 acc_left = 0;

	// idx responsible for shifting through buffer backward
	int idx = fir_index;

	for(int i = 0; i < FIR_TAPS; i++)
	{
		acc_right += fir_coeffs[i] * fir_buffer_right[idx];
		acc_left += fir_coeffs[i] * fir_buffer_left[idx];

		// Circular buffer wrap
		idx = (idx == 0) ? FIR_TAPS - 1 : idx - 1;
	}

	fir_index = (fir_index + 1) % FIR_TAPS;

	out_right = acc_right >> 8;
	out_left = acc_left >> 8;
}

void run_bandpass_filter()
{
	in_right_s32 = in_right << 8;
	in_left_s32 = in_left << 8;

	bandp_buffer_right[bandp_index] = in_right_s32;
	bandp_buffer_left[bandp_index] = in_left_s32;

	s64 acc_right = 0;
	s64 acc_left = 0;

	// idx responsible for shifting through buffer backward
	int idx = bandp_index;

	for(int i = 0; i < FIR_TAPS; i++)
	{
		acc_right += bandp_coeffs[i] * bandp_buffer_right[idx];
		acc_left += bandp_coeffs[i] * bandp_buffer_left[idx];

		// Circular buffer wrap
		idx = (idx == 0) ? FIR_TAPS - 1 : idx - 1;
	}

	bandp_index = (bandp_index + 1) % FIR_TAPS;

	out_right = acc_right >> 8;
	out_left = acc_left >> 8;
}

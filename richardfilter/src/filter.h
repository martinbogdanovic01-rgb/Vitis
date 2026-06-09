#ifndef __FILTER_H__
#define __FILTER_H__

typedef enum {
    STATE_LOOPBACK = 0,
    STATE_MOVING_AVG,
    STATE_FIR,
	STATE_BANDP
} system_state_t;

extern system_state_t current_state;

extern u32 in_right;
extern u32 in_left;

extern s32 in_right_s32;
extern s32 in_left_s32;

extern u32 out_right;
extern u32 out_left;

void determine_state();

void process_audio();

void run_audio_loopback();

void run_moving_avg_filter();

void run_fir_filter();

void run_bandpass_filter();


#endif

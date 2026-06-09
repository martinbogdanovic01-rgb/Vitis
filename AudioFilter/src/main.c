/* ---------------------------------------------------------------------------- *
 * Audio Filter - Button-controlled modes:
 * - BTN0: Loopback
 * - BTN1: Moving Average (8-tap)
 * - BTN2: FIR Low-pass (9-tap, order 8)
 *
 * FIR equation (Fs=48000Hz, Fc=610Hz, Fn=0.0625):
 * y[n] = 0.0179*x[n]   + 0.0485*x[n-1] + 0.1226*x[n-2]
 *      + 0.1970*x[n-3] + 0.2280*x[n-4] + 0.1970*x[n-5]
 *      + 0.1226*x[n-6] + 0.0485*x[n-7] + 0.0179*x[n-8]
 *
 * Moving average equation:
 * y[n] = (1/8) * ( x[n] + x[n-1] + x[n-2] + x[n-3]
 *                + x[n-4] + x[n-5] + x[n-6] + x[n-7] )
 *
 * Authors:
 *      Jeedella Jeedella, j.jeedella@fontys.nl
 *      Brice Guayrin, b.guayrin@fontys.nl
 *
 * Date: March 2025
 * ---------------------------------------------------------------------------- */


#include <stdio.h>
#include <xil_io.h>
#include "platform.h"
#include "xil_printf.h"
#include "audio_codec.h"
#include "xgpio.h"

#define BTNS_DEVICE_ID XPAR_GPIO_2_DEVICE_ID
#define LEDS_DEVICE_ID XPAR_GPIO_3_DEVICE_ID

#define MA_TAPS  8
#define FIR_TAPS 9

typedef enum { LOOPBACK, MOVING_AVG, FIR } audio_mode_t;

audio_mode_t mode = LOOPBACK;

static const float fir_coeffs[FIR_TAPS] = {
    0.0179f, 0.0485f, 0.1226f, 0.1970f, 0.2280f,
    0.1970f, 0.1226f, 0.0485f, 0.0179f
};

int main(void)
{
	// Local variables
    u32 status_reg = 0;
    u8  is_data_ready = 0;
    audio_mode_t prev_mode = -1;

    XGpio buttons, leds;
    u32 btn_val = 0;

    // Moving average buffers
    s32 ma_buf_l[MA_TAPS] = {0};
    s32 ma_buf_r[MA_TAPS] = {0};
    u32 ma_idx  = 0;
    s64 ma_sum_l = 0;
    s64 ma_sum_r = 0;

    // FIR buffers
    s32 fir_buf_l[FIR_TAPS] = {0};
    s32 fir_buf_r[FIR_TAPS] = {0};
    u32 fir_idx = 0;

    u32 in_left, in_right;
    s32 in_left_s32, in_right_s32;
    s32 out_left_s32, out_right_s32;

    float acc_l, acc_r;
    int i;
    u32 buf_idx;

    xil_printf("Entering Main\r\n");

    init_platform();

    XGpio_Initialize(&buttons, BTNS_DEVICE_ID);
    XGpio_SetDataDirection(&buttons, 1, 0xF);

    XGpio_Initialize(&leds, LEDS_DEVICE_ID);
    XGpio_SetDataDirection(&leds, 1, 0x0);
    XGpio_DiscreteWrite(&leds, 1, 0x0);

    IicConfig(XPAR_XIICPS_1_DEVICE_ID);
    AudioPllConfig();
    AudioConfigureJacks();

    xil_printf("ADAU1761 configured\n\r");

    while (1)
    {
        // BUTTONS
        btn_val = XGpio_DiscreteRead(&buttons, 1);
        if      (btn_val & 0x1)  mode = LOOPBACK;
        else if (btn_val & 0x2)  mode = MOVING_AVG;
        else if (btn_val & 0x4)  mode = FIR;

        // LED update
        if (mode != prev_mode) {
            if (mode == LOOPBACK) {
                xil_printf("Mode: LOOPBACK\r\n");
                XGpio_DiscreteWrite(&leds, 1, 0x1);
            } else if (mode == MOVING_AVG) {
                xil_printf("Mode: MOVING AVERAGE\r\n");
                XGpio_DiscreteWrite(&leds, 1, 0x2);
            } else if (mode == FIR) {
                xil_printf("Mode: FIR\r\n");
                XGpio_DiscreteWrite(&leds, 1, 0x4);
            }
            prev_mode = mode;
        }

        // WAIT FOR SAMPLE
        while (is_data_ready == 0) {
            status_reg    = Xil_In32(I2S_STATUS_REG);
            is_data_ready = (status_reg >> 21) & 1;
        }
        is_data_ready = 0;

        //  CLEAR FLAG
        status_reg &= ~(1 << 21);
        Xil_Out32(I2S_STATUS_REG, status_reg);

        // READ AUDIO
        in_left  = Xil_In32(I2S_DATA_RX_L_REG);
        in_right = Xil_In32(I2S_DATA_RX_R_REG);

        in_left_s32  = in_left  << 8;
        in_right_s32 = in_right << 8;

        out_left_s32  = 0;
        out_right_s32 = 0;

        // FILTERS
        if (mode == LOOPBACK)
        {
            out_left_s32  = in_left_s32;
            out_right_s32 = in_right_s32;
        }
        else if (mode == MOVING_AVG)
        {
            // LEFT
            ma_sum_l -= ma_buf_l[ma_idx];
            ma_buf_l[ma_idx] = in_left_s32;
            ma_sum_l += in_left_s32;
            out_left_s32 = (s32)(ma_sum_l >> 3);

            // RIGHT
            ma_sum_r -= ma_buf_r[ma_idx];
            ma_buf_r[ma_idx] = in_right_s32;
            ma_sum_r += in_right_s32;
            out_right_s32 = (s32)(ma_sum_r >> 3);

            ma_idx = (ma_idx + 1) & (MA_TAPS - 1);
        }
        else if (mode == FIR)
        {
            fir_buf_l[fir_idx] = in_left_s32;
            fir_buf_r[fir_idx] = in_right_s32;

            acc_l = 0.0f;
            acc_r = 0.0f;

            for (i = 0; i < FIR_TAPS; i++) {
                buf_idx = (fir_idx + FIR_TAPS - i) % FIR_TAPS;
                acc_l += fir_coeffs[i] * (float)fir_buf_l[buf_idx];
                acc_r += fir_coeffs[i] * (float)fir_buf_r[buf_idx];
            }

            fir_idx = (fir_idx + 1) % FIR_TAPS;


            out_left_s32  = (s32)acc_l;
            out_right_s32 = (s32)acc_r;
        }

        // WRITE AUDIO
        Xil_Out32(I2S_DATA_TX_L_REG, (u32)(out_left_s32  >> 8));
        Xil_Out32(I2S_DATA_TX_R_REG, (u32)(out_right_s32 >> 8));
    }

    return 1;
}

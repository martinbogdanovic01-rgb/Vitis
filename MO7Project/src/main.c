/* ---------------------------------------------------------------------------- *
 * Audio IIR Filter - Butterworth Lowpass (Fc=80Hz, Fs=48000Hz, Order=8)
 *
 * Authors:
 *      Jeedella Jeedella, j.jeedella@fontys.nl
 *      Brice Guayrin, b.guayrin@fontys.nl
 *
 * Date: May 2026
 * ---------------------------------------------------------------------------- */
#include <stdio.h>
#include <xil_io.h>
#include "platform.h"
#include "xil_printf.h"
#include "audio_codec.h"

#define NUM_SECTIONS 4
#define OUTPUT_GAIN  20.0

/* Gains per section */
static const double sos_gain[NUM_SECTIONS] = {
    2.735942348113e-05,
    2.725674265195e-05,
    2.717867334291e-05,
    2.713660875743e-05
};

/* Feedback coefficients */
static const double sos_den1[NUM_SECTIONS] = {
    -1.995813005055,
    -1.988322652256,
    -1.982627658632,
    -1.979559134663
};

static const double sos_den2[NUM_SECTIONS] = {
    0.9959224427494,
    0.9884316792269,
    0.9827363733259,
    0.979667681098
};

int main(void)
{
    u32 status_reg    = 0;
    u8  is_data_ready = 0;
    u32 in_left,  in_right;
    s32 in_left_s32, in_right_s32;
    s32 out_left_s32, out_right_s32;

    double w1_l[NUM_SECTIONS] = {0.0};
    double w2_l[NUM_SECTIONS] = {0.0};
    double w1_r[NUM_SECTIONS] = {0.0};
    double w2_r[NUM_SECTIONS] = {0.0};

    double x, v, w, y;
    int i;

    xil_printf("Entering Main\r\n");
    init_platform();
    IicConfig(XPAR_XIICPS_1_DEVICE_ID);
    AudioPllConfig();
    AudioConfigureJacks();
    xil_printf("ADAU1761 configured\r\n");

    while (1)
    {
        // WAIT FOR SAMPLE
        while (is_data_ready == 0) {
            status_reg    = Xil_In32(I2S_STATUS_REG);
            is_data_ready = (status_reg >> 21) & 1;
        }
        is_data_ready = 0;

        // CLEAR FLAG
        status_reg &= ~(1 << 21);
        Xil_Out32(I2S_STATUS_REG, status_reg);

        // READ AUDIO
        in_left  = Xil_In32(I2S_DATA_RX_L_REG);
        in_right = Xil_In32(I2S_DATA_RX_R_REG);

        in_left_s32  = in_left  << 8;
        in_right_s32 = in_right << 8;

        // IIR LEFT
        x = (double)in_left_s32;
        for (i = 0; i < NUM_SECTIONS; i++) {
            v    = sos_gain[i] * x;
            w    = v - sos_den1[i] * w1_l[i] - sos_den2[i] * w2_l[i];
            y    = w + 2.0 * w1_l[i] + w2_l[i];
            w2_l[i] = w1_l[i];
            w1_l[i] = w;
            x = y;
        }
        out_left_s32 = (s32)(y * OUTPUT_GAIN);

        // IIR RIGHT
        x = (double)in_right_s32;
        for (i = 0; i < NUM_SECTIONS; i++) {
            v    = sos_gain[i] * x;
            w    = v - sos_den1[i] * w1_r[i] - sos_den2[i] * w2_r[i];
            y    = w + 2.0 * w1_r[i] + w2_r[i];
            w2_r[i] = w1_r[i];
            w1_r[i] = w;
            x = y;
        }
        out_right_s32 = (s32)(y * OUTPUT_GAIN);

        // WRITE AUDIO
        Xil_Out32(I2S_DATA_TX_L_REG, (u32)(out_left_s32  >> 8));
        Xil_Out32(I2S_DATA_TX_R_REG, (u32)(out_right_s32 >> 8));
    }

    return 1;
}

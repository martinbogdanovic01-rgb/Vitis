/* ---------------------------------------------------------------------------- *
 * Authors: Jeedella Jeedella & Brice Guayrin
 * Assignment 2 - Audio with Volume, Mute, and Filter (hold BTN3 to filter)
 * ---------------------------------------------------------------------------- */

#include <stdio.h>
#include <xil_io.h>
#include "platform.h"
#include "xil_printf.h"
#include "audio_codec.h"
#include "xgpio.h"
#include "xparameters.h"

#define BTNS_DEVICE_ID XPAR_GPIO_2_DEVICE_ID

typedef enum {
    AUDIO_NORMAL,
    AUDIO_MUTED,
    AUDIO_VOLUME_UP,
    AUDIO_VOLUME_DOWN,
    AUDIO_FILTER
} AudioMode_t;

int main(void)
{
    /* ---- Linear buffer for 32-tap moving average ---- */
    s32 buf[32] = {0};  /* buf[0]=newest, buf[31]=oldest */

    /* ---- I2S state ---- */
    u32 status_reg   = 0;
    u8  is_data_ready = 0;

    /* ---- Volume / mode state ---- */
    float       vol_factor   = 1.0f;
    float       saved_vol    = 1.0f;
    AudioMode_t current_mode = AUDIO_NORMAL;

    u32 current_btns = 0;
    u32 last_btns    = 0;

    xil_printf("Entering Main\r\n");

    /* ---- Platform & codec init ---- */
    init_platform();
    IicConfig(XPAR_XIICPS_1_DEVICE_ID);
    AudioPllConfig();
    AudioConfigureJacks();

    /* ---- GPIO init ---- */
    XGpio buttons;
    XGpio_Initialize(&buttons, BTNS_DEVICE_ID);
    XGpio_SetDataDirection(&buttons, 1, 0xF);   /* all 4 pins as input */

    xil_printf("ADAU1761 configured\n\r");

    while (1)
    {
        /* --------------------------------------------------------- *
         * 1. Wait for a new 48 kHz sample                           *
         * --------------------------------------------------------- */
        while (is_data_ready == 0) {
            status_reg    = Xil_In32(I2S_STATUS_REG);
            is_data_ready = (status_reg >> 21) & 1;
        }
        is_data_ready = 0;
        /* Clear the data-ready bit */
        Xil_Out32(I2S_STATUS_REG, status_reg & (u32)(~(1 << 21)));

        /* --------------------------------------------------------- *
         * 2. Read audio inputs (one read per channel per sample)     *
         * --------------------------------------------------------- */
        u32 in_left  = Xil_In32(I2S_DATA_RX_L_REG);
        u32 in_right = Xil_In32(I2S_DATA_RX_R_REG);

        /* Sign-extend 24-bit audio to 32-bit */
        int32_t s_left  = ((int32_t)in_left  << 8) >> 8;
        int32_t s_right = ((int32_t)in_right << 8) >> 8;

        /* --------------------------------------------------------- *
         * 3. Read buttons & handle mode changes on edge             *
         * --------------------------------------------------------- */
        current_btns = XGpio_DiscreteRead(&buttons, 1);

        if (current_btns != last_btns) {
            switch (current_btns) {

                case 0x01: /* BTN0 – Volume Down */
                    if (current_mode != AUDIO_MUTED) {
                        vol_factor  *= 0.5f;
                        current_mode = AUDIO_NORMAL;
                        xil_printf("Volume Down  (factor=%.3f)\r\n", (double)vol_factor);
                    }
                    break;

                case 0x02: /* BTN1 – Volume Up */
                    if (current_mode != AUDIO_MUTED) {
                        vol_factor  *= 2.0f;
                        current_mode = AUDIO_NORMAL;
                        xil_printf("Volume Up    (factor=%.3f)\r\n", (double)vol_factor);
                    }
                    break;

                case 0x04: /* BTN2 – Mute toggle */
                    if (current_mode == AUDIO_MUTED) {
                        vol_factor   = saved_vol;
                        current_mode = AUDIO_NORMAL;
                        xil_printf("Unmuted\r\n");
                    } else {
                        saved_vol    = vol_factor;
                        vol_factor   = 0.0f;
                        current_mode = AUDIO_MUTED;
                        xil_printf("Muted\r\n");
                    }
                    break;

                case 0x08: /* BTN3 – Enter filter mode while held */
                    current_mode = AUDIO_FILTER;
                    xil_printf("Filter ON\r\n");
                    break;

                case 0x00: /* All buttons released */
                    if (current_mode == AUDIO_FILTER) {
                        current_mode = AUDIO_NORMAL;
                        xil_printf("Filter OFF\r\n");
                    } else if (current_mode != AUDIO_MUTED) {
                        current_mode = AUDIO_NORMAL;
                    }
                    break;

                default:
                    break;
            }
            last_btns = current_btns;
        }

        if (current_mode == AUDIO_FILTER)
        {
            /* Shift all samples down by one */
            for (int i = 31; i > 0; i--) {
                buf[i] = buf[i - 1];
            }
            buf[0] = s_right;   /* use already sign-extended sample */

            /* Sum all 32 taps and average */
            s64 acc = 0;
            for (int i = 0; i < 32; i++) {
                acc += buf[i];
            }
            s_right = (int32_t)(acc >> 5);   /* divide by 32 */
        }

        s_left  = (int32_t)((float)s_left  * vol_factor);
        s_right = (int32_t)((float)s_right * vol_factor);

        Xil_Out32(I2S_DATA_TX_L_REG, (u32)s_left);
        Xil_Out32(I2S_DATA_TX_R_REG, (u32)s_right);
    }

    return 1;
}

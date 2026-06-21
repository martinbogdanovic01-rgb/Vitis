/*
 * MO7 Audio Filter System
 *
 * Commands (115200 baud, \n or ; terminated):
 *   F0=bypass  F1=Chebyshev II
 *   G1=fgain+  G2=fgain-  G3=mgain+  G4=mgain-
 *   Vxx=volume  M=mute  U=unmute
 */

#include <string.h>
#include "xil_printf.h"
#include "xstatus.h"
#include "platform.h"
#include "xparameters.h"
#include "xil_io.h"
#include "audio_codec.h"
#include "audio_driver.h"
#include "uart.h"
#include "gpio_driver.h"
#include "buffer.h"
#include "logger.h"
#include "control.h"

#define CMD_MAX_LEN 32

static RingBuffer rx_buf;
static char       cmd_buf[CMD_MAX_LEN];
static int        cmd_idx = 0;

static void poll_uart_into_buffer(void)
{
    while (uart_data_available()) {
        char c = (char)uart_read_char();
        if (!buffer_put(&rx_buf, c))
            LOG_ERROR("RX overflow");
    }
}

static void drain_buffer(void)
{
    char c;
    while (buffer_get(&rx_buf, &c)) {
        if (c == '\r' || c == '\n' || c == ';') {
            if (cmd_idx > 0) {
                cmd_buf[cmd_idx] = '\0';
                control_process_command(cmd_buf);
                cmd_idx = 0;
            }
        } else if (cmd_idx < CMD_MAX_LEN - 1) {
            cmd_buf[cmd_idx++] = c;
        } else {
            LOG_ERROR("Command too long");
            cmd_idx = 0;
        }
    }
}

int main(void)
{
    init_platform();

    xil_printf("\r\n=== MO7 Audio Filter System ===\r\n");
    xil_printf("F0=bypass F1=Chebyshev | G1 G2 G3 G4=gain | Vxx | M U\r\n");

    if (uart_init() != XST_SUCCESS) {
        xil_printf("[FATAL] UART init failed\r\n");
        cleanup_platform();
        return XST_FAILURE;
    }
    xil_printf("UART ready\r\n");

    if (gpio_driver_init() != XST_SUCCESS)
        xil_printf("GPIO init failed (no LEDs/timing)\r\n");
    else
        xil_printf("GPIO ready\r\n");

    buffer_init(&rx_buf);

    /* Audio codec — exact sequence from original demo */
    IicConfig(XPAR_XIICPS_1_DEVICE_ID);
    AudioPllConfig();
    AudioConfigureJacks();
    xil_printf("ADAU1761 configured\r\n");

    control_init();
    control_send_status();

    xil_printf("Entering audio loop\r\n");

    while (1) {
        poll_uart_into_buffer();

        /* Wait for I2S data-ready (bit 21) — poll UART while waiting */
        u32 status_reg;
        u8  is_data_ready = 0;
        do {
            poll_uart_into_buffer();
            drain_buffer();
            status_reg    = Xil_In32(I2S_STATUS_REG);
            is_data_ready = (status_reg >> 21) & 1;
        } while (is_data_ready == 0);

        /* Clear bit 21 — same as original demo */
        status_reg = status_reg & (u32)(~(1 << 21));
        Xil_Out32(I2S_STATUS_REG, status_reg);

        /* Read stereo sample */
        AudioSample in = audio_read_sample();

        /* DSP — GPIO timing pin goes HIGH during processing */
        gpio_timing_start();

        int out_l, out_r;
        control_process_audio((int)in.left, (int)in.right, &out_l, &out_r);

        gpio_timing_stop();

        /* Write output */
        audio_write_sample((s32)out_l, (s32)out_r);

        /* Process any complete UART commands */
        drain_buffer();
    }

    cleanup_platform();
    return 0;
}

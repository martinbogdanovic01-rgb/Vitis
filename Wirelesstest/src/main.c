#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xil_io.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xstatus.h"
#include "audio_codec.h"
#include "uart.h"

/* ----------- Audio mode ----------- */
typedef enum { LOOPBACK, MOVING_AVG, FIR } audio_mode_t;
static audio_mode_t g_filter = LOOPBACK;
static int g_volume = 80;
static int g_muted  = 0;

/* ----------- Filter buffers ----------- */
#define MA_TAPS  8
#define FIR_TAPS 9
static s32 ma_buf_l[MA_TAPS] = {0};
static s32 ma_buf_r[MA_TAPS] = {0};
static u32 ma_idx  = 0;
static s64 ma_sum_l = 0;
static s64 ma_sum_r = 0;
static s32 fir_buf_l[FIR_TAPS] = {0};
static s32 fir_buf_r[FIR_TAPS] = {0};
static u32 fir_idx = 0;
static const float fir_coeffs[FIR_TAPS] = {
    0.0179f, 0.0485f, 0.1226f, 0.1970f, 0.2280f,
    0.1970f, 0.1226f, 0.0485f, 0.0179f
};

/* ----------- UART command buffer ----------- */
#define CMD_BUF_SIZE 32
static char g_cmd_buf[CMD_BUF_SIZE];
static int  g_cmd_idx = 0;

/* ----------- Execute command ----------- */
static void execute_command(const char *cmd)
{
    xil_printf("CMD: %s\r\n", cmd);
    if (strcmp(cmd, "MUTE") == 0) {
        g_muted = 1;
        xil_printf("Muted\r\n");
    } else if (strcmp(cmd, "UNMUTE") == 0) {
        g_muted = 0;
        xil_printf("Unmuted, volume=%d%%\r\n", g_volume);
    } else if (strncmp(cmd, "VOL ", 4) == 0) {
        int v = atoi(cmd + 4);
        if (v < 0)   v = 0;
        if (v > 100) v = 100;
        g_volume = v;
        xil_printf("Volume=%d%%\r\n", g_volume);
    } else if (strcmp(cmd, "LOOPBACK") == 0) {
        g_filter = LOOPBACK;
        xil_printf("Filter: LOOPBACK\r\n");
    } else if (strcmp(cmd, "MA") == 0) {
        g_filter = MOVING_AVG;
        xil_printf("Filter: MOVING AVG\r\n");
    } else if (strcmp(cmd, "FIR") == 0) {
        g_filter = FIR;
        xil_printf("Filter: FIR\r\n");
    } else {
        xil_printf("Unknown: %s\r\n", cmd);
    }
}

/* ----------- Poll UART ----------- */
static void poll_uart(void)
{
    if (uart_data_available()) {
        u8 ch = uart_read_char();
        if (ch == '\n' || ch == '\r') {
            if (g_cmd_idx > 0) {
                g_cmd_buf[g_cmd_idx] = '\0';
                execute_command(g_cmd_buf);
                g_cmd_idx = 0;
            }
        } else {
            if (g_cmd_idx < CMD_BUF_SIZE - 1)
                g_cmd_buf[g_cmd_idx++] = (char)ch;
        }
    }
}

/* ----------- Main -----------riba */
int main(void)
{
    u32 status_reg    = 0;
    u8  is_data_ready = 0;

    xil_printf("--- Step 1: Entering main() ---\r\n");
    init_platform();
    xil_printf("--- Step 2: init_platform() done ---\r\n");
    AudioPllConfig();
    xil_printf("--- Step 3: AudioPllConfig() done ---\r\n");
    AudioConfigureJacks();
    xil_printf("--- Step 4: AudioConfigureJacks() done ---\r\n");

    if (uart_init() != XST_SUCCESS) {
        xil_printf("!!! UART init FAILED !!!\r\n");
        return XST_FAILURE;
    }
    xil_printf("--- Step 5: UART ready ---\r\n");
    xil_printf("--- Step 6: Entering audio loop ---\r\n");

    while (1) {
        poll_uart();

        /* Wait for sample */
        while (is_data_ready == 0) {
            status_reg    = Xil_In32(I2S_STATUS_REG);
            is_data_ready = (status_reg >> 21) & 1;
        }
        is_data_ready = 0;
        Xil_Out32(I2S_STATUS_REG, status_reg & (u32)(~(1 << 21)));

        /* Read audio */
        u32 in_left  = Xil_In32(I2S_DATA_RX_L_REG);
        u32 in_right = Xil_In32(I2S_DATA_RX_R_REG);
        s32 s_left   = (s32)(in_left  << 8);
        s32 s_right  = (s32)(in_right << 8);
        s32 out_left, out_right;

        /* Apply filter */
        if (g_filter == LOOPBACK) {
            out_left  = s_left;
            out_right = s_right;
        } else if (g_filter == MOVING_AVG) {
            ma_sum_l -= ma_buf_l[ma_idx];
            ma_buf_l[ma_idx] = s_left;
            ma_sum_l += s_left;
            out_left = (s32)(ma_sum_l >> 3);

            ma_sum_r -= ma_buf_r[ma_idx];
            ma_buf_r[ma_idx] = s_right;
            ma_sum_r += s_right;
            out_right = (s32)(ma_sum_r >> 3);

            ma_idx = (ma_idx + 1) & (MA_TAPS - 1);
        } else { /* FIR */
            fir_buf_l[fir_idx] = s_left;
            fir_buf_r[fir_idx] = s_right;
            float acc_l = 0.0f, acc_r = 0.0f;
            for (int i = 0; i < FIR_TAPS; i++) {
                u32 idx = (fir_idx + FIR_TAPS - i) % FIR_TAPS;
                acc_l += fir_coeffs[i] * (float)fir_buf_l[idx];
                acc_r += fir_coeffs[i] * (float)fir_buf_r[idx];
            }
            fir_idx = (fir_idx + 1) % FIR_TAPS;
            out_left  = (s32)acc_l;
            out_right = (s32)acc_r;
        }

        /* Apply volume / mute */
        if (g_muted) {
            out_left  = 0;
            out_right = 0;
        } else {
            out_left  = (s32)((s64)out_left  * g_volume / 100);
            out_right = (s32)((s64)out_right * g_volume / 100);
        }

        /* Write audio */
        Xil_Out32(I2S_DATA_TX_L_REG, (u32)(out_left  >> 8));
        Xil_Out32(I2S_DATA_TX_R_REG, (u32)(out_right >> 8));
    }

    return 1;
}

/* ----------------------------------------------------------------------------
 * Remote Control + Audio Filter - Merged Application
 * Authors:  (student name(s))
 * Date:     May 2026
 *
 * Main loop structure (follows flowchart):
 *   1. Wait (blocking) until audio sample is available
 *   2. Read audio sample
 *   3. Process audio sample (MA filter)
 *   4. Write audio sample
 *   5. Check if UART byte available
 *   6. If yes: read byte, process byte (toggle LED)
 *   7. Check buttons
 *   8. Go back to 1
 *
 * Protocol (unidirectional: Laptop -> PYNQ-Z2 over UART):
 *   3-byte command: L<led>$
 *     - L      : start character
 *     - <led>  : '1' .. '4'  which LED to control
 *     - $      : end character
 *   Examples: "L1$" = toggle LED1, "L2$" = toggle LED2
 * ---------------------------------------------------------------------------- */

#include <stdio.h>
#include "xparameters.h"
#include "xstatus.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "xgpio.h"
#include "platform.h"
#include "uart.h"
#include "audio_codec.h"

#define BTNS_DEVICE_ID  XPAR_GPIO_2_DEVICE_ID
#define LEDS_DEVICE_ID  XPAR_GPIO_3_DEVICE_ID
#define NUM_BUTTONS     4

typedef enum {
    WAIT_L,
    WAIT_ID,
    WAIT_END
} ParserState;

static XGpio leds;
static XGpio buttons;
static u8  led_state  = 0x00;
static u32 btn_prev   = 0;

static ParserState parser_state  = WAIT_L;
static int         cmd_led_index = 0;

static void toggle_led(int index)
{
    if (index < 0 || index >= NUM_BUTTONS) return;
    led_state ^= (1 << index);
    XGpio_DiscreteWrite(&leds, 1, (u32)led_state);
    xil_printf("LED %d toggled -> %s\r\n", index + 1,
               (led_state >> index) & 1 ? "ON" : "OFF");
}

static void handle_buttons(void)
{
    u32 btn_cur = XGpio_DiscreteRead(&buttons, 1) & 0x0F;
    u32 rising  = btn_cur & (~btn_prev);
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (rising & (1 << i)) {
            xil_printf("Local button %d pressed\r\n", i + 1);
            toggle_led(i);
        }
    }
    btn_prev = btn_cur;
}

/* Process one byte through the protocol state machine */
static void process_byte(u8 byte)
{
    switch (parser_state) {
        case WAIT_L:
            if (byte == 'L') {
                parser_state = WAIT_ID;
            }
            break;

        case WAIT_ID:
            if (byte >= '1' && byte <= '4') {
                cmd_led_index = byte - '1';
                parser_state  = WAIT_END;
            } else {
                xil_printf("Protocol error: expected 1-4, got '%c'\r\n", byte);
                parser_state = WAIT_L;
            }
            break;

        case WAIT_END:
            if (byte == '$') {
                xil_printf("Remote: toggle LED %d\r\n", cmd_led_index + 1);
                toggle_led(cmd_led_index);
            } else {
                xil_printf("Protocol error: expected '$', got '%c'\r\n", byte);
            }
            parser_state = WAIT_L;
            break;
    }
}

int main(void)
{
    int status;

    init_platform();

    xil_printf("\r\n=== Remote Control + Audio Filter ===\r\n");
    xil_printf("Protocol: L<1-4>$  e.g. L1$ = toggle LED 1\r\n");

    /* GPIO init */
    XGpio_Initialize(&leds,    LEDS_DEVICE_ID);
    XGpio_Initialize(&buttons, BTNS_DEVICE_ID);
    XGpio_SetDataDirection(&leds,    1, 0x0);
    XGpio_SetDataDirection(&buttons, 1, 0xF);
    XGpio_DiscreteWrite(&leds,       1, 0x0);

    /* UART init */
    status = uart_init();
    if (status != XST_SUCCESS) {
        xil_printf("UART init FAILED\r\n");
        return XST_FAILURE;
    }

    /* Audio codec init */
    IicConfig(XPAR_XIICPS_1_DEVICE_ID);
    AudioPllConfig();
    AudioConfigureJacks();
    xil_printf("ADAU1761 configured\r\n");
    xil_printf("Ready!\r\n");

    s32 prev_in_right_s32 = 0;

    while (1)
    {
        /* ---- 1. Wait until audio sample is available (blocking) ---------- */
        u32 status_reg;
        do {
            status_reg = Xil_In32(I2S_STATUS_REG);
        } while (((status_reg >> 21) & 1) == 0);

        /* ---- 2. Read audio sample ---------------------------------------- */
        u32 in_right = Xil_In32(I2S_DATA_RX_R_REG);
        u32 in_left  = Xil_In32(I2S_DATA_RX_L_REG);

        /* ---- 3. Process audio sample (2-tap MA filter on right channel) --- */
        s32 in_right_s32  = (s32)(in_right << 8);
        s64 acc           = (s64)in_right_s32 + (s64)prev_in_right_s32;
        acc             >>= 1;
        u32 out_right     = (u32)((s32)(acc >> 8));
        prev_in_right_s32 = in_right_s32;

        /* ---- 4. Write audio sample --------------------------------------- */
        Xil_Out32(I2S_DATA_TX_R_REG, out_right);
        Xil_Out32(I2S_DATA_TX_L_REG, in_left);

        /* Clear audio ready bit */
        status_reg &= ~(1u << 21);
        Xil_Out32(I2S_STATUS_REG, status_reg);

        /* ---- 5. Check if UART byte available ----------------------------- */
        if (uart_data_available()) {
            /* ---- 6. Read byte and process byte --------------------------- */
            u8 byte = uart_read_char();
            process_byte(byte);
        }

        /* ---- 7. Check physical buttons ----------------------------------- */
        handle_buttons();
    }

    cleanup_platform();
    return XST_SUCCESS;
}

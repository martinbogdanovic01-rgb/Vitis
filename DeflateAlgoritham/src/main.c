#include <string.h>
#include "platform.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "deflate.h"

/* ----------------------------------------------------------------
   Buffers in BSS (DDR3) — never on the stack
---------------------------------------------------------------- */
#define MAX_DYNAMIC_LEN (10001)
#define COMP_CAP        (MAX_DYNAMIC_LEN + (MAX_DYNAMIC_LEN >> 3) + 128)
#define DECOMP_CAP      (MAX_DYNAMIC_LEN)   /* FIX: was +64, must equal input */

static uint8_t dynamic_input[MAX_DYNAMIC_LEN];
static uint8_t compressed   [COMP_CAP];
static uint8_t decompressed [DECOMP_CAP];

/* ----------------------------------------------------------------
   Blocking UART byte receiver using Vitis inbyte()
---------------------------------------------------------------- */
static void uart_receive_bytes(uint8_t *buffer, size_t length)
{
    size_t i;
    for (i = 0; i < length; i++)
        buffer[i] = (uint8_t)inbyte();
}

/* ----------------------------------------------------------------
   Entry point
---------------------------------------------------------------- */
int main(void)
{
    init_platform();

    xil_printf("\r\n=== Bare-metal DEFLATE on PYNQ-Z2 ===\r\n");
    xil_printf("Waiting for data from Python GUI...\r\n");

    while (1)
    {
        /* ---- Step 1: Read 4-byte little-endian payload size ---- */
        uint8_t len_bytes[4];
        uart_receive_bytes(len_bytes, 4);

        uint32_t input_len = (uint32_t)len_bytes[0]
                           | ((uint32_t)len_bytes[1] << 8)
                           | ((uint32_t)len_bytes[2] << 16)
                           | ((uint32_t)len_bytes[3] << 24);

        /* Safety guard */
        if (input_len == 0 || input_len > MAX_DYNAMIC_LEN) {
            xil_printf("ERROR: Invalid payload size: %d bytes\r\n",
                       (int)input_len);
            xil_printf("\r\n---END_OF_BATCH---\r\n");
            continue;
        }

        /* ---- Step 2: Read payload bytes ---- */
        uart_receive_bytes(dynamic_input, (size_t)input_len);

        xil_printf("\r\n=== Vitis 2023.1 DEFLATE Core ===\r\n");
        xil_printf("Received : %d bytes\r\n", (int)input_len);

        /* ---- Step 3: Compress ---- */
        size_t comp_len = 0;
        int rc = deflate_compress(dynamic_input, (size_t)input_len,
                                  compressed, COMP_CAP, &comp_len);

        if (rc != DEFLATE_OK) {
            xil_printf("ERROR: deflate_compress failed (%d)\r\n", rc);
            xil_printf("\r\n---END_OF_BATCH---\r\n");
            continue;
        }

        /* Integer-only ratio to avoid floating-point in xil_printf */
        int ratio_pct = (int)(100 - ((comp_len * 100) / (size_t)input_len));
        xil_printf("Compressed : %d bytes (saved ~%d%%)\r\n",
                   (int)comp_len, ratio_pct);

        /* ---- Step 4: Decompress ---- */
        size_t decomp_len = 0;
        rc = deflate_decompress(compressed, comp_len,
                                decompressed, DECOMP_CAP, &decomp_len);

        /* ---- Step 5: Verify round-trip ---- */
        if (rc != DEFLATE_OK) {
            xil_printf("FAIL: deflate_decompress failed (%d)\r\n", rc);
        } else if (decomp_len != (size_t)input_len) {
            xil_printf("FAIL: length mismatch (%d vs %d)\r\n",
                       (int)decomp_len, (int)input_len);
        } else if (memcmp(decompressed, dynamic_input, input_len) != 0) {
            xil_printf("FAIL: data corruption in round-trip\r\n");
        } else {
            xil_printf("PASS: Round-trip verification successful.\r\n");
        }

        /* Sentinel so Python GUI knows the board has finished */
        xil_printf("\r\n---END_OF_BATCH---\r\n");
    }

    cleanup_platform();
    return 0;
}

/*
 * main.c  -  Source coding (DEFLATE) + Channel coding (RS) pipeline
 *            for PYNQ-Z2
 *
 * ============================================================
 * FULL PIPELINE OVERVIEW
 * ============================================================
 *
 * TRANSMIT SIDE:
 *
 *   [Raw input bytes]  (received over UART from Python GUI)
 *        |
 *        v
 *   deflate_compress()       Source encoding: removes redundancy
 *        |
 *        v
 *   [Compressed bytes]       Smaller payload, still bytes
 *        |
 *        v
 *   rs_encode_stream()       Channel encoding: adds FEC protection
 *        |                   Splits compressed data into 223-byte blocks,
 *        |                   encodes each block to 255 bytes
 *        v
 *   [RS codeword blocks]     Ready to hand to modulation module
 *        |
 *        v
 *   --> handoff to modulator (tx_ready_callback called per 255-byte block)
 *
 * RECEIVE SIDE:
 *
 *   [Received 255-byte blocks]  From demodulator, may contain errors
 *        |
 *        v
 *   rs_decode()              Channel decoding: corrects up to 16 byte errors
 *        |                   per 255-byte block
 *        v
 *   [Recovered compressed bytes]
 *        |
 *        v
 *   deflate_decompress()     Source decoding: restores original bytes
 *        |
 *        v
 *   [Original raw bytes]
 *
 * ============================================================
 * INTEGRATION POINTS FOR MODULATION MODULE
 * ============================================================
 *
 *   TRANSMIT: implement tx_ready_callback(block, 255)
 *             called once per encoded 255-byte RS block
 *             feed these bytes into your modulator
 *
 *   RECEIVE:  call rx_feed_block(block, 255) once per
 *             received 255-byte block from your demodulator
 *
 * ============================================================
 * RS BLOCK FRAMING
 * ============================================================
 *
 *   Compressed data is split into chunks of RS_K = 223 bytes.
 *   The last chunk is zero-padded to 223 bytes if needed.
 *   Each chunk is encoded to RS_N = 255 bytes.
 *   A 4-byte header is prepended before the RS blocks so the
 *   receiver knows the total compressed length.
 *
 *   Wire format:
 *     [4 bytes: compressed length (little-endian)]
 *     [255 bytes: RS block 0]
 *     [255 bytes: RS block 1]
 *     ...
 *     [255 bytes: RS block N]
 *
 * ============================================================
 */

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "deflate.h"
#include "rs_common.h"
#include "rs_encoder.h"
#include "rs_decoder.h"

/* ============================================================
 * Buffer sizes
 * ============================================================ */
#define MAX_INPUT_LEN     10001
#define COMP_CAP          (MAX_INPUT_LEN + (MAX_INPUT_LEN >> 3) + 128)
#define DECOMP_CAP        MAX_INPUT_LEN

/* Maximum number of RS blocks we can produce from COMP_CAP bytes:
 * ceil(COMP_CAP / RS_K) + 1 for the header block */
#define MAX_RS_BLOCKS     ((COMP_CAP / RS_K) + 2)
#define RS_BLOCK_BUF_CAP  (MAX_RS_BLOCKS * RS_N)

/* ============================================================
 * Static buffers — kept in BSS (DDR3), never on the stack
 * ============================================================ */
static uint8_t raw_input   [MAX_INPUT_LEN];
static uint8_t compressed  [COMP_CAP];
static uint8_t decompressed[DECOMP_CAP];
static uint8_t rs_out_buf  [RS_BLOCK_BUF_CAP]; /* all encoded RS blocks */
static uint8_t rs_in_buf   [RS_BLOCK_BUF_CAP]; /* all received RS blocks */

/* ============================================================
 * UART helpers
 * ============================================================ */
static void uart_receive_bytes(uint8_t *buf, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++)
        buf[i] = (uint8_t)inbyte();
}

/* ============================================================
 * TRANSMIT SIDE
 *
 * tx_ready_callback() — called once per encoded RS block (255 bytes).
 * Replace the body with a call to your modulator's input function.
 * ============================================================ */
static void tx_ready_callback(const uint8_t *block, size_t block_len)
{
    /*
     * TODO (modulation group): replace this with your modulator call.
     * block     = pointer to 255-byte RS codeword
     * block_len = always 255 (RS_N)
     *
     * Example:
     *   modulator_feed(block, block_len);
     */
    (void)block;
    (void)block_len;
    /* For now: do nothing, blocks are also stored in rs_out_buf */
}

/*
 * encode_and_transmit()
 *
 * Takes raw input bytes, compresses them, RS-encodes the compressed
 * data in 223-byte blocks, and calls tx_ready_callback() per block.
 *
 * Returns total number of RS blocks produced, or -1 on error.
 */
static int encode_and_transmit(const uint8_t *input, size_t input_len)
{
    /* ---- Step 1: Source encode (DEFLATE compress) ---- */
    size_t comp_len = 0;
    int rc = deflate_compress(input, input_len,
                              compressed, COMP_CAP, &comp_len);
    if (rc != DEFLATE_OK) {
        xil_printf("ERROR: deflate_compress failed (%d)\r\n", rc);
        return -1;
    }

    int ratio_pct = (int)(100 - ((comp_len * 100) / input_len));
    xil_printf("Source encoded : %d -> %d bytes (saved ~%d%%)\r\n",
               (int)input_len, (int)comp_len, ratio_pct);

    /* ---- Step 2: Prepend 4-byte header (compressed length) ---- */
    /*
     * The header tells the receiver how many bytes to decompress.
     * It is sent as the first RS block's payload (packed into msg[]).
     */
    uint8_t header[4];
    header[0] = (uint8_t)( comp_len        & 0xFF);
    header[1] = (uint8_t)((comp_len >>  8) & 0xFF);
    header[2] = (uint8_t)((comp_len >> 16) & 0xFF);
    header[3] = (uint8_t)((comp_len >> 24) & 0xFF);

    /* ---- Step 3: Channel encode (RS encode in 223-byte blocks) ---- */
    size_t offset    = 0;
    int    n_blocks  = 0;
    uint8_t msg[RS_K];
    uint8_t cw [RS_N];

    /* First block carries the 4-byte header + first 219 bytes of data */
    {
        memset(msg, 0, RS_K);
        memcpy(msg, header, 4);                    /* header in first 4 bytes */
        size_t payload = RS_K - 4;                 /* 219 bytes of data       */
        if (payload > comp_len) payload = comp_len;
        memcpy(msg + 4, compressed, payload);
        offset += payload;

        rs_encode(msg, cw);
        memcpy(rs_out_buf + n_blocks * RS_N, cw, RS_N);
        tx_ready_callback(cw, RS_N);
        n_blocks++;
    }

    /* Remaining blocks carry 223 bytes of compressed data each */
    while (offset < comp_len) {
        memset(msg, 0, RS_K);
        size_t payload = comp_len - offset;
        if (payload > RS_K) payload = RS_K;
        memcpy(msg, compressed + offset, payload);
        offset += payload;

        rs_encode(msg, cw);
        memcpy(rs_out_buf + n_blocks * RS_N, cw, RS_N);
        tx_ready_callback(cw, RS_N);
        n_blocks++;
    }

    xil_printf("Channel encoded: %d RS blocks (%d bytes total)\r\n",
               n_blocks, n_blocks * RS_N);

    return n_blocks;
}

/* ============================================================
 * RECEIVE SIDE
 *
 * rx_feed_block() — call this once per received 255-byte block
 * from your demodulator.
 * ============================================================ */
static int    rx_block_count    = 0;  /* total blocks expected      */
static int    rx_blocks_got     = 0;  /* blocks received so far     */
static size_t rx_comp_len       = 0;  /* compressed length from hdr */

/*
 * rx_start()
 *
 * Call this before feeding the first block of a new frame.
 * n_blocks = total number of RS blocks you expect to receive.
 */
static void rx_start(int n_blocks)
{
    rx_block_count = n_blocks;
    rx_blocks_got  = 0;
    rx_comp_len    = 0;
    memset(rs_in_buf, 0, sizeof(rs_in_buf));
}

/*
 * rx_feed_block()
 *
 * Feed one received 255-byte RS block.
 * Returns 1 when all blocks have been received and the frame is ready
 * to decode, 0 if more blocks are still expected, -1 on error.
 */
static int rx_feed_block(const uint8_t *block)
{
    if (rx_blocks_got >= rx_block_count) return -1;

    memcpy(rs_in_buf + rx_blocks_got * RS_N, block, RS_N);
    rx_blocks_got++;

    return (rx_blocks_got == rx_block_count) ? 1 : 0;
}

/*
 * decode_received()
 *
 * Call after rx_feed_block() returns 1.
 * RS-decodes all blocks, strips framing, then DEFLATE-decompresses.
 * Writes result to out[], sets *out_len.
 * Returns 0 on success, -1 on error.
 */
static int decode_received(uint8_t *out, size_t out_cap, size_t *out_len)
{
    uint8_t rx [RS_N];
    uint8_t dec[RS_K];
    int     total_errors = 0;

    /* Temporary buffer for reassembled compressed data */
    static uint8_t reassembled[COMP_CAP];
    size_t reassembled_len = 0;

    for (int b = 0; b < rx_block_count; b++) {
        memcpy(rx, rs_in_buf + b * RS_N, RS_N);

        int result = rs_decode(rx, dec);
        if (result < 0) {
            xil_printf("ERROR: RS block %d uncorrectable\r\n", b);
            return -1;
        }
        total_errors += result;

        if (b == 0) {
            /* First block: extract 4-byte header */
            rx_comp_len = (size_t)dec[0]
                        | ((size_t)dec[1] <<  8)
                        | ((size_t)dec[2] << 16)
                        | ((size_t)dec[3] << 24);

            /* Copy data portion (bytes 4..222) */
            size_t payload = RS_K - 4;
            if (reassembled_len + payload > COMP_CAP) return -1;
            memcpy(reassembled + reassembled_len, dec + 4, payload);
            reassembled_len += payload;
        } else {
            /* Remaining blocks: all 223 bytes are data */
            if (reassembled_len + RS_K > COMP_CAP) return -1;
            memcpy(reassembled + reassembled_len, dec, RS_K);
            reassembled_len += RS_K;
        }
    }

    xil_printf("Channel decoded: %d errors corrected across %d blocks\r\n",
               total_errors, rx_block_count);

    /* Clamp to actual compressed length */
    if (rx_comp_len > reassembled_len) {
        xil_printf("ERROR: header length mismatch\r\n");
        return -1;
    }

    /* ---- Source decode (DEFLATE decompress) ---- */
    int rc = deflate_decompress(reassembled, rx_comp_len,
                                out, out_cap, out_len);
    if (rc != DEFLATE_OK) {
        xil_printf("ERROR: deflate_decompress failed (%d)\r\n", rc);
        return -1;
    }

    xil_printf("Source decoded : %d -> %d bytes\r\n",
               (int)rx_comp_len, (int)*out_len);

    return 0;
}

/* ============================================================
 * Main
 * ============================================================ */
int main(void)
{
    init_platform();
    gf_init();   /* initialise RS GF(2^8) tables — call once */

    xil_printf("\r\n=== PYNQ-Z2: DEFLATE + RS(255,223) Pipeline ===\r\n");
    xil_printf("Waiting for data...\r\n");

    while (1)
    {
        /* ---- Receive 4-byte length header from Python GUI ---- */
        uint8_t len_bytes[4];
        uart_receive_bytes(len_bytes, 4);

        uint32_t input_len = (uint32_t)len_bytes[0]
                           | ((uint32_t)len_bytes[1] <<  8)
                           | ((uint32_t)len_bytes[2] << 16)
                           | ((uint32_t)len_bytes[3] << 24);

        if (input_len == 0 || input_len > MAX_INPUT_LEN) {
            xil_printf("ERROR: invalid payload size %d\r\n", (int)input_len);
            xil_printf("\r\n---END_OF_BATCH---\r\n");
            continue;
        }

        /* ---- Receive raw input bytes ---- */
        uart_receive_bytes(raw_input, (size_t)input_len);
        xil_printf("\r\nReceived %d raw bytes\r\n", (int)input_len);

        /* ---- TRANSMIT SIDE: compress + RS encode ---- */
        xil_printf("\r\n-- Transmit side --\r\n");
        int n_blocks = encode_and_transmit(raw_input, (size_t)input_len);
        if (n_blocks < 0) {
            xil_printf("\r\n---END_OF_BATCH---\r\n");
            continue;
        }

        /*
         * At this point rs_out_buf[0 .. n_blocks*255-1] holds all
         * encoded RS blocks ready for the modulator.
         * tx_ready_callback() was already called once per block above.
         */

        /* ---- RECEIVE SIDE: RS decode + decompress (loopback test) ----
         *
         * In the real system this section runs on the receiver board.
         * Here we do a loopback to verify the pipeline end-to-end.
         * Replace rs_out_buf with whatever your demodulator produces.
         */
        xil_printf("\r\n-- Receive side (loopback) --\r\n");
        rx_start(n_blocks);

        for (int b = 0; b < n_blocks; b++) {
            /*
             * TODO (modulation group): replace rs_out_buf + b*RS_N
             * with the block delivered by your demodulator.
             */
            rx_feed_block(rs_out_buf + b * RS_N);
        }

        size_t out_len = 0;
        int rc = decode_received(decompressed, DECOMP_CAP, &out_len);

        /* ---- Verify round-trip ---- */
        if (rc != 0) {
            xil_printf("FAIL: decode pipeline error\r\n");
        } else if (out_len != (size_t)input_len) {
            xil_printf("FAIL: length mismatch (%d vs %d)\r\n",
                       (int)out_len, (int)input_len);
        } else if (memcmp(decompressed, raw_input, input_len) != 0) {
            xil_printf("FAIL: data corruption detected\r\n");
        } else {
            xil_printf("PASS: full pipeline round-trip OK\r\n");
        }

        xil_printf("\r\n---END_OF_BATCH---\r\n");
    }

    cleanup_platform();
    return 0;
}

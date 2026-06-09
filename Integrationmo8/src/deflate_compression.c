#include "deflate_compression.h"
#include <string.h>

/* ================================================================
   Bit writer
================================================================ */

void bw_init(BitWriter *bw, uint8_t *buf, size_t cap)
{
    bw->buf = buf;
    bw->cap = cap;
    bw->pos = 0;
    bw->bits = 0;
    bw->bit_count = 0;
    bw->overflow = 0;
}

void bw_write(BitWriter *bw, uint32_t val, int n_bits)
{
    bw->bits |= val << bw->bit_count;
    bw->bit_count += n_bits;

    while (bw->bit_count >= 8) {
        if (bw->pos >= bw->cap) {
            bw->overflow = 1;
            return;
        }

        bw->buf[bw->pos++] = (uint8_t)(bw->bits & 0xFF);

        bw->bits >>= 8;
        bw->bit_count -= 8;
    }
}

void bw_flush(BitWriter *bw)
{
    if (bw->bit_count > 0) {
        if (bw->pos >= bw->cap) {
            bw->overflow = 1;
            return;
        }

        bw->buf[bw->pos++] = (uint8_t)(bw->bits & 0xFF);
    }
}

size_t bw_bytes(const BitWriter *bw)
{
    return bw->pos;
}

/* ================================================================
   Reverse bits
================================================================ */

static uint32_t reverse_bits(uint32_t v, int n)
{
    uint32_t r = 0;

    while (n--) {
        r = (r << 1) | (v & 1);
        v >>= 1;
    }

    return r;
}

/* ================================================================
   Fixed Huffman encoder
================================================================ */

static void write_fixed_literal(BitWriter *bw, int sym)
{
    uint32_t code;
    int bits;

    if (sym <= 143) {
        code = 0x30 + sym;
        bits = 8;
    }
    else if (sym <= 255) {
        code = 0x190 + (sym - 144);
        bits = 9;
    }
    else if (sym <= 279) {
        code = sym - 256;
        bits = 7;
    }
    else {
        code = 0xC0 + (sym - 280);
        bits = 8;
    }

    bw_write(bw, reverse_bits(code, bits), bits);
}

static void write_fixed_distance(BitWriter *bw, int dist_sym)
{
    bw_write(bw, reverse_bits((uint32_t)dist_sym, 5), 5);
}

/* ================================================================
   Length tables
================================================================ */

static const int len_base[29] = {
    3,4,5,6,7,8,9,10,11,13,15,17,
    19,23,27,31,35,43,51,59,67,83,
    99,115,131,163,195,227,258
};

static const int len_extra[29] = {
    0,0,0,0,0,0,0,0,1,1,1,1,
    2,2,2,2,3,3,3,3,4,4,
    4,4,5,5,5,5,0
};

static const int dist_base[30] = {
    1,2,3,4,5,7,9,13,17,25,33,
    49,65,97,129,193,257,385,513,
    769,1025,1537,2049,3073,4097,
    6145,8193,12289,16385,24577
};

static const int dist_extra[30] = {
    0,0,0,0,1,1,2,2,3,3,4,
    4,5,5,6,6,7,7,8,
    8,9,9,10,10,11,
    11,12,12,13,13
};

/* ================================================================
   Simple LZ77
================================================================ */

static int find_match(
    const uint8_t *in,
    size_t in_len,
    size_t pos,
    int *distance)
{
    int best_len = 0;
    int best_dist = 0;

    size_t start =
        (pos > 32768) ? (pos - 32768) : 0;

    for (size_t i = start; i < pos; i++) {

        int len = 0;

        while (
            len < 258 &&
            pos + len < in_len &&
            in[i + len] == in[pos + len])
        {
            len++;
        }

        if (len >= 3 && len > best_len) {
            best_len = len;
            best_dist = (int)(pos - i);

            if (len == 258)
                break;
        }
    }

    *distance = best_dist;
    return best_len;
}

/* ================================================================
   Main compressor
================================================================ */

int deflate_compress(
    const uint8_t *in,
    size_t in_len,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len)
{
    if (!in || !out || !out_len)
        return DEFLATE_ERR_INPUT;

    BitWriter bw;
    bw_init(&bw, out, out_cap);

    /* BFINAL=1 BTYPE=01 */
    bw_write(&bw, 1, 1);
    bw_write(&bw, 1, 2);

    size_t pos = 0;

    while (pos < in_len) {

        int dist = 0;

        int len =
            find_match(in, in_len, pos, &dist);

        if (len >= 3) {

            int lcode = 0;

            while (
                lcode < 28 &&
                len >= len_base[lcode + 1])
            {
                lcode++;
            }

            write_fixed_literal(
                &bw,
                257 + lcode);

            if (len_extra[lcode] > 0) {
                bw_write(
                    &bw,
                    len - len_base[lcode],
                    len_extra[lcode]);
            }

            int dcode = 0;

            while (
                dcode < 29 &&
                dist >= dist_base[dcode + 1])
            {
                dcode++;
            }

            write_fixed_distance(&bw, dcode);

            if (dist_extra[dcode] > 0) {
                bw_write(
                    &bw,
                    dist - dist_base[dcode],
                    dist_extra[dcode]);
            }

            pos += len;
        }
        else {
            write_fixed_literal(&bw, in[pos]);
            pos++;
        }
    }

    /* End block */
    write_fixed_literal(&bw, 256);

    bw_flush(&bw);

    if (bw.overflow)
        return DEFLATE_ERR_OUTPUT;

    *out_len = bw_bytes(&bw);

    return DEFLATE_OK;
}

#include "deflate.h"
#include <string.h>

/* ================================================================
   Bit-reader
================================================================ */
typedef struct {
    const uint8_t *buf;
    size_t         len;
    size_t         pos;
    uint32_t       bits;
    int            bit_count;
    int            error;
} BitReader;

static void br_init(BitReader *br, const uint8_t *buf, size_t len)
{
    br->buf       = buf;
    br->len       = len;
    br->pos       = 0;
    br->bits      = 0;
    br->bit_count = 0;
    br->error     = 0;
}

static void br_refill(BitReader *br)
{
    while (br->bit_count <= 24 && br->pos < br->len) {
        br->bits |= (uint32_t)br->buf[br->pos++] << br->bit_count;
        br->bit_count += 8;
    }
}

static uint32_t br_read(BitReader *br, int n)
{
    br_refill(br);
    if (br->bit_count < n) { br->error = 1; return 0; }
    uint32_t val  = br->bits & ((1u << n) - 1u);
    br->bits     >>= n;
    br->bit_count -= n;
    return val;
}

/* Read exactly 1 bit, LSB first */
static int br_bit(BitReader *br)
{
    br_refill(br);
    if (br->bit_count < 1) { br->error = 1; return 0; }
    int b = (int)(br->bits & 1u);
    br->bits >>= 1;
    br->bit_count--;
    return b;
}

/* ================================================================
   Huffman decoder — canonical decode, no table, no bit-reversal.
   Reads one bit at a time MSB-first into 'code' and compares
   against the canonical first-code at each length.
   This matches exactly how the compressor assigns codes and how
   reverse_bits() writes them to the stream.
================================================================ */
#define HD_MAX_SYMS 288

typedef struct {
    uint16_t sym  [HD_MAX_SYMS];
    uint8_t  slen [HD_MAX_SYMS];
    uint16_t first[DMAX_BITS + 2];
    int      index[DMAX_BITS + 2];
    int      count;
} HuffDecoder;

static int hd_build(HuffDecoder *hd, const uint8_t *lens, int n_syms)
{
    int bl_count[DMAX_BITS + 1];
    int i, l;

    memset(bl_count,  0, sizeof(bl_count));
    memset(hd->first, 0, sizeof(hd->first));
    memset(hd->index, 0, sizeof(hd->index));
    hd->count = 0;

    for (i = 0; i < n_syms; i++)
        if (lens[i]) bl_count[lens[i]]++;

    /* First canonical code at each length */
    uint32_t code = 0;
    for (i = 1; i <= DMAX_BITS; i++) {
        code = (code + bl_count[i-1]) << 1;
        hd->first[i] = (uint16_t)code;
    }

    /* Starting index in sym[] for each length */
    {
        int pos = 0;
        for (i = 1; i <= DMAX_BITS; i++) {
            hd->index[i] = pos;
            pos += bl_count[i];
        }
        hd->index[DMAX_BITS + 1] = pos;
        hd->count = pos;
    }

    /* Place symbols in canonical order */
    {
        int pos[DMAX_BITS + 1];
        for (l = 1; l <= DMAX_BITS; l++) pos[l] = hd->index[l];
        for (i = 0; i < n_syms; i++) {
            l = lens[i];
            if (l > 0) {
                hd->sym [pos[l]] = (uint16_t)i;
                hd->slen[pos[l]] = (uint8_t)l;
                pos[l]++;
            }
        }
    }

    return 0;
}

/*
 * Decode one symbol.
 * The compressor wrote reverse_bits(canonical_code, len) LSB-first.
 * br_bit() reads LSB-first, so the first bit we read is the LSB of
 * the reversed code, which is the MSB of the original canonical code.
 * Accumulating bits with  code = (code<<1)|bit  therefore rebuilds
 * the original canonical code MSB-first — exactly what first[] holds.
 */
static int hd_decode(HuffDecoder *hd, BitReader *br)
{
    uint32_t code = 0;
    int i;

    for (i = 1; i <= DMAX_BITS; i++) {
        code = (code << 1) | (uint32_t)br_bit(br);
        if (br->error) return -1;

        int count_at_len = hd->index[i + 1] - hd->index[i];
        if (count_at_len > 0 &&
            code >= hd->first[i] &&
            code <  hd->first[i] + (uint32_t)count_at_len) {
            return (int)hd->sym[hd->index[i] +
                                (int)(code - hd->first[i])];
        }
    }

    br->error = 1;
    return -1;
}

/* ================================================================
   Fixed Huffman tables (RFC 1951 BTYPE=01)
================================================================ */
static void build_fixed_ll(uint8_t *lens)
{
    int i;
    for (i = 0;   i <= 143; i++) lens[i] = 8;
    for (i = 144; i <= 255; i++) lens[i] = 9;
    for (i = 256; i <= 279; i++) lens[i] = 7;
    for (i = 280; i <= 287; i++) lens[i] = 8;
}

static void build_fixed_d(uint8_t *lens)
{
    int i;
    for (i = 0; i < 32; i++) lens[i] = 5;
}

/* ================================================================
   Length / distance decode tables
================================================================ */
static const int len_base[29] = {
    3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,
    35,43,51,59,67,83,99,115,131,163,195,227,258
};
static const int len_extra[29] = {
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,
    3,3,3,3,4,4,4,4,5,5,5,5,0
};
static const int dist_base[30] = {
    1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
    257,385,513,769,1025,1537,2049,3073,4097,6145,
    8193,12289,16385,24577
};
static const int dist_extra[30] = {
    0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,
    7,7,8,8,9,9,10,10,11,11,12,12,13,13
};

static const int cl_order[19] = {
    16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15
};

/* ================================================================
   Main decompress entry point
================================================================ */
static HuffDecoder g_hd_ll;
static HuffDecoder g_hd_d;
static HuffDecoder g_hd_cl;

int deflate_decompress(const uint8_t *in, size_t in_len,
                       uint8_t *out, size_t out_cap, size_t *out_len)
{
    if (!in || !out || !out_len) return DEFLATE_ERR_INPUT;

    BitReader br;
    br_init(&br, in, in_len);
    *out_len = 0;

    int bfinal = 0;
    while (!bfinal) {
        bfinal = (int)br_read(&br, 1);
        int btype = (int)br_read(&br, 2);
        if (br.error) return DEFLATE_ERR_DATA;

        /* ---- Uncompressed block ---- */
        if (btype == 0) {
            br.bits      = 0;
            br.bit_count = 0;
            if (br.pos + 4 > br.len) return DEFLATE_ERR_DATA;
            uint16_t blen  = (uint16_t)(br.buf[br.pos]
                                      | (br.buf[br.pos+1] << 8));
            uint16_t bnlen = (uint16_t)(br.buf[br.pos+2]
                                      | (br.buf[br.pos+3] << 8));
            br.pos += 4;
            if ((uint16_t)(blen ^ bnlen) != 0xFFFF) return DEFLATE_ERR_DATA;
            if (*out_len + blen > out_cap)           return DEFLATE_ERR_OUTPUT;
            memcpy(out + *out_len, br.buf + br.pos, blen);
            *out_len += blen;
            br.pos   += blen;
            continue;
        }

        static uint8_t ll_lens[288];
        static uint8_t d_lens [32];

        /* ---- Fixed Huffman ---- */
        if (btype == 1) {
            build_fixed_ll(ll_lens);
            build_fixed_d(d_lens);
            hd_build(&g_hd_ll, ll_lens, 288);
            hd_build(&g_hd_d,  d_lens,  32);

        /* ---- Dynamic Huffman ---- */
        } else if (btype == 2) {
            int hlit  = (int)br_read(&br, 5) + 257;
            int hdist = (int)br_read(&br, 5) + 1;
            int hclen = (int)br_read(&br, 4) + 4;
            if (br.error) return DEFLATE_ERR_DATA;

            static uint8_t cl_lens[19];
            memset(cl_lens, 0, sizeof(cl_lens));
            int i;
            for (i = 0; i < hclen; i++)
                cl_lens[cl_order[i]] = (uint8_t)br_read(&br, 3);

            hd_build(&g_hd_cl, cl_lens, 19);

            static uint8_t all_lens[288 + 32];
            memset(all_lens, 0, sizeof(all_lens));
            int total = hlit + hdist;
            i = 0;
            while (i < total) {
                int sym = hd_decode(&g_hd_cl, &br);
                if (br.error || sym < 0) return DEFLATE_ERR_DATA;

                if (sym < 16) {
                    all_lens[i++] = (uint8_t)sym;
                } else if (sym == 16) {
                    if (i == 0) return DEFLATE_ERR_DATA;
                    uint8_t prev_len = all_lens[i - 1];
                    int rep = (int)br_read(&br, 2) + 3;
                    while (rep-- && i < total)
                        all_lens[i++] = prev_len;
                } else if (sym == 17) {
                    int rep = (int)br_read(&br, 3) + 3;
                    while (rep-- && i < total)
                        all_lens[i++] = 0;
                } else {
                    int rep = (int)br_read(&br, 7) + 11;
                    while (rep-- && i < total)
                        all_lens[i++] = 0;
                }
            }
            if (br.error) return DEFLATE_ERR_DATA;

            memcpy(ll_lens, all_lens,        hlit);
            memset(ll_lens + hlit, 0, 288    - hlit);
            memcpy(d_lens,  all_lens + hlit, hdist);
            memset(d_lens  + hdist, 0, 32    - hdist);

            hd_build(&g_hd_ll, ll_lens, hlit);
            hd_build(&g_hd_d,  d_lens,  hdist);

        } else {
            return DEFLATE_ERR_DATA;
        }

        /* ---- Decode symbol stream ---- */
        while (1) {
            int sym = hd_decode(&g_hd_ll, &br);
            if (br.error || sym < 0) return DEFLATE_ERR_DATA;

            if (sym < 256) {
                if (*out_len >= out_cap) return DEFLATE_ERR_OUTPUT;
                out[(*out_len)++] = (uint8_t)sym;

            } else if (sym == 256) {
                break;

            } else {
                int lcode = sym - 257;
                if (lcode < 0 || lcode >= 29) return DEFLATE_ERR_DATA;

                int length = len_base[lcode];
                if (len_extra[lcode])
                    length += (int)br_read(&br, len_extra[lcode]);

                int dsym = hd_decode(&g_hd_d, &br);
                if (br.error || dsym < 0 || dsym >= 30)
                    return DEFLATE_ERR_DATA;

                int distance = dist_base[dsym];
                if (dist_extra[dsym])
                    distance += (int)br_read(&br, dist_extra[dsym]);

                if ((size_t)distance > *out_len)         return DEFLATE_ERR_DATA;
                if (*out_len + (size_t)length > out_cap) return DEFLATE_ERR_OUTPUT;

                size_t src = *out_len - (size_t)distance;
                int k;
                for (k = 0; k < length; k++)
                    out[(*out_len)++] = out[src++];
            }

            if (br.error) return DEFLATE_ERR_DATA;
        }
    }
    return DEFLATE_OK;
}

#ifndef DEFLATE_COMPRESSION_H
#define DEFLATE_COMPRESSION_H

#include "deflate.h"

/* ---------------------------------------------------------------
   Bit-output stream
--------------------------------------------------------------- */
typedef struct {
    uint8_t *buf;
    size_t   cap;
    size_t   pos;
    uint32_t bits;
    int      bit_count;
    int      overflow;
} BitWriter;

void   bw_init  (BitWriter *bw, uint8_t *buf, size_t cap);
void   bw_write (BitWriter *bw, uint32_t val, int n_bits);
void   bw_flush (BitWriter *bw);
size_t bw_bytes (const BitWriter *bw);

/* ---------------------------------------------------------------
   Huffman tree builder
--------------------------------------------------------------- */
typedef struct {
    uint32_t code[DMAX_LITERALS];
    int      len [DMAX_LITERALS];
} HuffTree;

void huff_build(HuffTree *ht, const uint32_t *freq,
                int n_syms, int max_bits);

/* ---------------------------------------------------------------
   LZ77 match finder
--------------------------------------------------------------- */
typedef struct {
    uint16_t head[65536];
    uint16_t prev[DMAX_WINDOW];
} LZState;

void lz_init  (LZState *lz);
int  lz_find  (LZState *lz,
               const uint8_t *in, size_t in_len,
               size_t pos, int *match_dist);
void lz_insert(LZState *lz, const uint8_t *in, size_t pos);

#endif /* DEFLATE_COMPRESSION_H */

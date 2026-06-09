#ifndef RS_COMMON_H
#define RS_COMMON_H

/*
 * Reed-Solomon RS(255, 223) over GF(2^8)
 * =======================================
 * Standard: CCSDS / NASA / DVB-S / CD audio
 *
 * Parameters
 * ----------
 *   n    = 255   codeword length  (symbols)
 *   k    = 223   message length   (symbols)
 *   2t   =  32   parity symbols
 *   t    =  16   symbol errors correctable (t=16)
 *   m    =   8   bits per symbol  (1 byte each)
 *
 * Field: GF(2^8) with primitive polynomial
 *   p(x) = x^8 + x^4 + x^3 + x^2 + 1  (0x11D)
 *   Primitive element: alpha = 2
 *
 * Generator polynomial
 *   g(x) = prod_{i=1}^{32} (x - alpha^i)
 *   Roots: alpha^1 .. alpha^32
 *
 * Key properties (for report)
 * ---------------------------
 *   Minimum Hamming distance:  d_min = n - k + 1 = 33
 *   Errors correctable:        t     = 16 symbol errors
 *   Errors detectable:         2t    = 32 symbol errors
 *   Hamming distance check:    t     = floor((d_min-1)/2) = floor(32/2) = 16  ✓
 *
 *   Burst error capability:
 *     Each symbol = m = 8 bits.  A burst confined to one symbol counts as
 *     only ONE symbol error regardless of how many bits are flipped.
 *     Maximum correctable burst length = t * m = 16 * 8 = 128 contiguous bits.
 *     This is why RS is far superior to Hamming codes for real channels
 *     (storage media, RF links) where errors arrive in clusters.
 */

#include <stdint.h>
#include <stdio.h>

/* ── Field / code parameters ──────────────────────────────── */
#define GF_M        8           /* bits per symbol                  */
#define GF_N        255         /* 2^m - 1                          */
#define GF_POLY     0x11D       /* x^8+x^4+x^3+x^2+1 (primitive)   */

#define RS_N        255         /* codeword length                  */
#define RS_K        223         /* message length                   */
#define RS_2T       32          /* parity symbols  (n - k)          */
#define RS_T        16          /* errors correctable               */
#define RS_FCR      1           /* first consecutive root: alpha^1  */

/* ── GF(2^8) log / exp tables (populated by gf_init) ─────── */
extern uint8_t  gf_alpha_to[GF_N + 1]; /* gf_alpha_to[i] = alpha^i              */
extern uint8_t  gf_index_of[GF_N + 1]; /* gf_index_of[x] = log_alpha(x)         */
                                        /* gf_index_of[0] = GF_N  (sentinel)     */

/* ── Initialise tables – call once before anything else ───── */
void gf_init(void);

/* All operations work on uint8_t symbols (0 .. 254).          */

static inline uint8_t gf_add(uint8_t a, uint8_t b) { return (uint8_t)(a ^ b); }

uint8_t gf_mul(uint8_t a, uint8_t b);
uint8_t gf_inv(uint8_t a);
uint8_t gf_div(uint8_t a, uint8_t b);
uint8_t gf_pow(uint8_t x, int n);

#endif /* RS_COMMON_H */

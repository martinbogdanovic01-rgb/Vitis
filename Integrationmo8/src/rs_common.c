#include "rs_common.h"

uint8_t gf_alpha_to[GF_N + 1];
uint8_t gf_index_of[GF_N + 1];

/*
 * Build log and antilog tables for GF(2^8).
 *
 * We represent GF(2^8) as GF(2)[x] / p(x) where
 *   p(x) = x^8 + x^4 + x^3 + x^2 + 1  (0x11D)
 *
 * Starting from alpha^0 = 1, each successive power is obtained by
 * multiplying by alpha (= polynomial "x", value 2).  When the result
 * reaches degree 8 we reduce modulo p(x) by XORing with 0x11D.
 */
void gf_init(void)
{
    int sr = 1;
    for (int i = 0; i < GF_N; i++) {
        gf_alpha_to[i] = (uint8_t)sr;
        gf_index_of[sr] = (uint8_t)i;
        sr <<= 1;
        if (sr & 0x100)          /* bit 8 set → reduce mod p(x)  */
            sr ^= GF_POLY;
        sr &= 0xFF;
    }
    gf_alpha_to[GF_N] = 0;      /* alpha^255 = 0 (sentinel)     */
    gf_index_of[0]    = GF_N;   /* log(0) = -inf, use GF_N = 255 */
}

/*
 * Multiply two GF(2^8) elements using log tables.
 * log(a*b) = log(a) + log(b)  mod 255.
 */
uint8_t gf_mul(uint8_t a, uint8_t b)
{
    if (a == 0 || b == 0) return 0;
    int log_sum = (int)gf_index_of[a] + (int)gf_index_of[b];
    if (log_sum >= GF_N) log_sum -= GF_N;   /* mod 255 without % */
    return gf_alpha_to[log_sum];
}

/*
 * Multiplicative inverse: a^{-1} = alpha^{255 - log(a)}.
 */
uint8_t gf_inv(uint8_t a)
{
    if (a == 0) { return 0; }                /* undefined; return 0 */
    /* a^{-1} = alpha^{(255 - log(a)) mod 255}.
     * Must use mod 255: when log(a)=0 (a=1), GF_N-0=255, but alpha^255=alpha^0=1.
     * gf_alpha_to[255] is the sentinel (0), so we must wrap to index 0. */
    int exp = (int)(GF_N - gf_index_of[a]);
    if (exp == GF_N) exp = 0;   /* alpha^255 = alpha^0 */
    return gf_alpha_to[exp];
}

/*
 * Division: a / b = a * b^{-1}.
 */
uint8_t gf_div(uint8_t a, uint8_t b)
{
    if (b == 0) { return 0; }
    if (a == 0) return 0;
    int log_diff = (int)gf_index_of[a] - (int)gf_index_of[b];
    if (log_diff < 0) log_diff += GF_N;
    return gf_alpha_to[log_diff];
}

/*
 * x^n in GF(2^8).
 * n must be >= 0.  Uses only addition and the exp table — no % operator.
 */
uint8_t gf_pow(uint8_t x, int n)
{
    if (x == 0) return 0;
    if (n == 0) return 1;
    int log_x = (int)gf_index_of[x];
    int log_xn = log_x * n;
    /* Reduce mod 255 using only subtraction — avoids signed % on ARM */
    log_xn = log_xn % GF_N;
    if (log_xn < 0) log_xn += GF_N;
    return gf_alpha_to[log_xn];
}

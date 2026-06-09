#include "rs_encoder.h"
#include <string.h>

/*
 * Pre-computed generator polynomial g(x) = prod_{i=1}^{32}(x - alpha^i).
 *
 * Stored as coefficients [g0, g1, ..., g32] where g0 is the constant term
 * (lowest degree) and g32 = 1 (leading coefficient, implicit).
 * We build this once at first encode call.
 */
static uint8_t genpoly[RS_2T + 1];   /* 33 coefficients, index = degree */
static int     genpoly_ready = 0;

/*
 * Build g(x) = prod_{i=1}^{2t} (x + alpha^i).
 * In GF(2^8) subtraction == addition == XOR, so (x - a) = (x + a).
 *
 * Multiply factors in one by one.  genpoly[j] = coefficient of x^j.
 * Start with g(x) = 1  (degree 0).
 */
static void build_genpoly(void)
{
    if (genpoly_ready) return;

    memset(genpoly, 0, sizeof(genpoly));
    genpoly[0] = 1;          /* g(x) = 1 */

    for (int i = 0; i < RS_2T; i++) {
        /*
         * Multiply current g(x) by (x + alpha^{i+1}).
         * Go from high degree down so we don't clobber values we still need.
         */
        uint8_t root = gf_alpha_to[RS_FCR + i];   /* alpha^{i+1} */

        for (int j = i + 1; j > 0; j--) {
            genpoly[j] = genpoly[j - 1] ^ gf_mul(genpoly[j], root);
        }
        genpoly[0] = gf_mul(genpoly[0], root);
    }

    genpoly_ready = 1;
}

/*
 * compute_parity()
 *
 * Divides the message polynomial by g(x) and stores the 32-byte remainder
 * in reg[]. This remainder becomes the parity symbols of the codeword.
 * Processes all RS_K message symbols one at a time (index 0 first = highest degree).
 *
 * Register layout: reg[0] = coeff of x^0, reg[RS_2T-1] = coeff of x^{2t-1}.
 */
static void compute_parity(const uint8_t msg[RS_K], uint8_t reg[RS_2T])
{
    /* Initialise the 32-cell shift register to zero */
    memset(reg, 0, RS_2T);

    for (int i = 0; i < RS_K; i++) {
        /* Feedback = top of register XOR incoming message symbol */
        uint8_t feedback = msg[i] ^ reg[RS_2T - 1];

        if (feedback != 0) {
            /* Shift register and apply feedback through genpoly */
            for (int j = RS_2T - 1; j > 0; j--) {
                reg[j] = reg[j - 1] ^ gf_mul(genpoly[j], feedback);
            }
            reg[0] = gf_mul(genpoly[0], feedback);
        } else {
            /* feedback == 0: just shift, no XOR needed */
            for (int j = RS_2T - 1; j > 0; j--) {
                reg[j] = reg[j - 1];
            }
            reg[0] = 0;
        }
    }
}

/*
 * rs_encode()
 *
 * Encodes RS_K message bytes into RS_N codeword bytes.
 * The codeword is systematic: message bytes are passed through unchanged,
 * followed by 32 parity bytes computed by dividing the message polynomial
 * by g(x).
 */
void rs_encode(const uint8_t msg[RS_K], uint8_t codeword[RS_N])
{
    /* Get the generator polynomial g(x) = prod(x + alpha^i) for i=1..32
     * Only computed once, reused for every subsequent encode call */
    build_genpoly();

    /* Divide the message polynomial by g(x)
     * reg[] holds the 32-byte remainder after division */
    uint8_t reg[RS_2T];
    compute_parity(msg, reg);

    /* Assemble systematic codeword: msg[223] followed by parity[32]
     * reg[] (the remainder) becomes the 32 parity bytes appended after the message */
    memcpy(codeword, msg, RS_K);
    for (int j = 0; j < RS_2T; j++) {
        codeword[RS_K + j] = reg[RS_2T - 1 - j];
    }
}

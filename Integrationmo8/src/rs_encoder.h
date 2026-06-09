#ifndef RS_ENCODER_H
#define RS_ENCODER_H

#include "rs_common.h"

/*
 * rs_encode()
 * -----------
 * Produces a systematic RS(255,223) codeword from 223 message bytes.
 *
 * Layout (SAME convention used by decoder):
 *
 *   codeword[0 .. 222]     = message symbols  (unchanged)
 *   codeword[223 .. 254]   = 32 parity symbols
 *
 * Algorithm: LFSR-based polynomial division.
 *   The message polynomial m(x) is treated as having degree k-1,
 *   with m[0] as the coefficient of x^{k-1} (most significant).
 *   We compute r(x) = m(x) * x^{2t}  mod  g(x),
 *   where g(x) = prod_{i=1}^{32} (x - alpha^i).
 *
 * @param msg       [in]  RS_K = 223 message bytes
 * @param codeword  [out] RS_N = 255 byte codeword  (msg ++ parity)
 */
void rs_encode(const uint8_t msg[RS_K], uint8_t codeword[RS_N]);

#endif /* RS_ENCODER_H */

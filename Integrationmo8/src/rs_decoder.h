#ifndef RS_DECODER_H
#define RS_DECODER_H

#include "rs_common.h"

/*
 * rs_decode()
 * -----------
 * Decodes a received RS(255,223) codeword, correcting up to t=16 symbol errors.
 *
 * Steps
 * -----
 *  1. Syndrome computation
 *       S_i = r(alpha^i)  for i = 1..32
 *       If all zero → no errors.
 *
 *  2. Berlekamp-Massey
 *       Finds the shortest error-locator polynomial
 *       Lambda(x) = 1 + L1*x + L2*x^2 + ... + Le*x^e
 *       whose degree e is the number of errors detected.
 *
 *  3. Chien search
 *       Tests every possible position X_i = alpha^j for j=0..254.
 *       Lambda(X_i^{-1}) = 0  ↔  position j is an error location.
 *
 *  4. Forney algorithm
 *       Computes the error magnitude at each error location using
 *       the error evaluator polynomial Omega(x) = S(x)*Lambda(x) mod x^{2t}.
 *
 *  5. Correction
 *       received[pos] ^= magnitude  for each error.
 *
 * @param received  [in/out] RS_N = 255 bytes; corrected in-place
 * @param decoded   [out]    RS_K = 223 message bytes
 * @return  number of symbol errors corrected (0..16)
 *          -1 if uncorrectable (> 16 symbol errors)
 */
int rs_decode(uint8_t received[RS_N], uint8_t decoded[RS_K]);

#endif /* RS_DECODER_H */

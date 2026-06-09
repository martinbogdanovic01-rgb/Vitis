#ifndef DEFLATE_H
#define DEFLATE_H

#include <stdint.h>
#include <stddef.h>

/* ---------------------------------------------------------------
   Return codes
--------------------------------------------------------------- */
#define DEFLATE_OK            0
#define DEFLATE_ERR_INPUT    -1
#define DEFLATE_ERR_OUTPUT   -2
#define DEFLATE_ERR_DATA     -3

/* ---------------------------------------------------------------
   Limits
--------------------------------------------------------------- */
#define DMAX_LITERALS   286
#define DMAX_DISTANCES  30
#define DMAX_CODELEN    19
#define DMAX_BITS       15
#define DMAX_WINDOW     32768
#define DMAX_MATCH      258
#define DMIN_MATCH      3

/* ---------------------------------------------------------------
   Public API
--------------------------------------------------------------- */
int deflate_compress(const uint8_t *in,  size_t in_len,
                           uint8_t *out, size_t out_cap,
                           size_t  *out_len);

int deflate_decompress(const uint8_t *in,  size_t in_len,
                             uint8_t *out, size_t out_cap,
                             size_t  *out_len);

#endif /* DEFLATE_H */

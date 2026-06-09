#include "rs_decoder.h"
#include <string.h>

/* ═══════════════════════════════════════════════════════════════
 * STEP 1 – Syndrome computation
 *
 * Evaluate r(x) at each root of g(x): alpha^1, alpha^2, ..., alpha^{2t}.
 *
 * Convention (MUST match encoder):
 *   r(x) = r[0]*x^{n-1} + r[1]*x^{n-2} + ... + r[n-1]*x^0
 *   i.e. r[0] is the coefficient of the HIGHEST power.
 *
 * Horner's method: r(a) = (...((r[0]*a + r[1])*a + r[2])*a ... + r[n-1])
 *
 * Returns 1 if any syndrome is non-zero, 0 if the codeword is clean.
 * ═══════════════════════════════════════════════════════════════ */
static int compute_syndromes(const uint8_t r[RS_N], uint8_t S[RS_2T])
{
    int has_error = 0;

    for (int i = 0; i < RS_2T; i++) {
        /* Evaluate at alpha^{FCR + i} */
        uint8_t root = gf_alpha_to[RS_FCR + i];
        uint8_t val  = 0;

        for (int j = 0; j < RS_N; j++) {
            val = gf_add(gf_mul(val, root), r[j]);
        }

        S[i] = val;
        if (val) has_error = 1;
    }
    return has_error;
}

/* ═══════════════════════════════════════════════════════════════
 * STEP 2 – Berlekamp-Massey algorithm
 *
 * Finds the minimal-degree LFSR whose output matches the syndrome
 * sequence S[0..2t-1].  The LFSR connection polynomial is the
 * error-locator polynomial Lambda(x).
 *
 * Lambda[0] = 1 always.
 * Degree of Lambda on return = number of errors located.
 *
 * Returns degree of Lambda (number of errors).
 * ═══════════════════════════════════════════════════════════════ */
static int berlekamp_massey(const uint8_t S[RS_2T], uint8_t Lambda[RS_T + 1])
{
    uint8_t C[RS_T + 1];   /* current connection polynomial  */
    uint8_t B[RS_T + 1];   /* previous connection polynomial */

    memset(C, 0, sizeof(C));  C[0] = 1;
    memset(B, 0, sizeof(B));  B[0] = 1;

    int L = 0;   /* current LFSR length              */
    int m = 1;   /* # iterations since last update   */
    uint8_t b = 1; /* leading coeff of B when it was set */

    for (int n = 0; n < RS_2T; n++) {
        /* Compute discrepancy delta */
        uint8_t delta = S[n];
        for (int i = 1; i <= L; i++) {
            delta ^= gf_mul(C[i], S[n - i]);
        }

        if (delta == 0) {
            /* No discrepancy; advance without updating */
            m++;
        } else if (2 * L <= n) {
            /* Must increase LFSR length */
            uint8_t T_copy[RS_T + 1];
            memcpy(T_copy, C, sizeof(C));

            uint8_t coeff = gf_mul(delta, gf_inv(b));
            /* C(x) += coeff * x^m * B(x) */
            for (int i = m; i <= RS_T; i++) {
                C[i] ^= gf_mul(coeff, B[i - m]);
            }
            L = n + 1 - L;
            memcpy(B, T_copy, sizeof(T_copy));
            b = delta;
            m = 1;
        } else {
            /* Update C without changing length */
            uint8_t coeff = gf_mul(delta, gf_inv(b));
            for (int i = m; i <= RS_T; i++) {
                C[i] ^= gf_mul(coeff, B[i - m]);
            }
            m++;
        }
    }

    /* Copy result (only L+1 coefficients are meaningful) */
    memcpy(Lambda, C, (RS_T + 1) * sizeof(uint8_t));

    /* Trim L to the true degree of Lambda.
     * BM can report L higher than the actual polynomial degree when
     * leading coefficients are zero (e.g. L=2 but Lambda[2]=0).
     * Without this trim, Chien finds fewer roots than L and wrongly
     * returns -1 for a perfectly correctable error pattern. */
    while (L > 0 && Lambda[L] == 0) L--;

    return L;
}

/* ═══════════════════════════════════════════════════════════════
 * STEP 3 – Chien search
 *
 * Find all X such that Lambda(X) = 0.
 * We test X = alpha^{-j} for j = 0 .. n-1.
 * If Lambda(alpha^{-j}) = 0  →  position j is an error location.
 *
 * Uses the fact that alpha^{-j} = alpha^{255-j}.
 *
 * Fills err_pos[] with the positions (byte indices into codeword)
 * and returns the number of roots found.
 * ═══════════════════════════════════════════════════════════════ */
static int chien_search(const uint8_t Lambda[RS_T + 1], int L,
                        uint8_t err_pos[RS_T])
{
    int num_roots = 0;

    for (int j = 0; j < RS_N; j++) {
        /*
         * Evaluate Lambda at alpha^{-j} = alpha^{GF_N - j}.
         * Horner from highest degree down to 0.
         */
        uint8_t x_inv = gf_alpha_to[(GF_N - j) % GF_N];
        uint8_t val   = 0;

        for (int i = L; i >= 0; i--) {
            val = gf_add(gf_mul(val, x_inv), Lambda[i]);
        }

        if (val == 0) {
            if (num_roots < RS_T)
                err_pos[num_roots] = (uint8_t)(RS_N - 1 - j);  /* byte index */
            num_roots++;
        }
    }
    return num_roots;
}

/* ═══════════════════════════════════════════════════════════════
 * STEP 4 – Forney algorithm
 *
 * Compute error magnitudes using the error evaluator polynomial:
 *   Omega(x) = S(x) * Lambda(x)  mod  x^{2t}
 *
 * where S(x) = S[0] + S[1]*x + ... + S[2t-1]*x^{2t-1}.
 *
 * For each error at position pos_j (X_j = alpha^{pos_j}):
 *
 *   e_j = X_j^{1-FCR} * Omega(X_j^{-1})
 *         ─────────────────────────────
 *              Lambda'(X_j^{-1})
 *
 * where Lambda'(x) is the formal derivative of Lambda(x):
 *   Lambda'(x) = L1 + L3*x^2 + L5*x^4 + ...   (even-index terms vanish in GF(2))
 * ═══════════════════════════════════════════════════════════════ */
static void forney(const uint8_t S[RS_2T],
                   const uint8_t Lambda[RS_T + 1], int L,
                   const uint8_t err_pos[RS_T], int num_errors,
                   uint8_t err_mag[RS_T])
{
    /* Compute Omega = S(x) * Lambda(x) mod x^{2t} */
    uint8_t Omega[RS_2T];
    memset(Omega, 0, sizeof(Omega));

    for (int i = 0; i < RS_2T; i++) {
        for (int j = 0; j <= L && j <= i; j++) {
            Omega[i] ^= gf_mul(Lambda[j], S[i - j]);
        }
    }

    for (int k = 0; k < num_errors; k++) {
        int pos = err_pos[k];
        /*
         * X_k is the error locator value, derived from the syndrome convention.
         * Horner evaluates r(alpha^i) = sum_{j=0}^{N-1} rx[j] * alpha^{i*(N-1-j)},
         * so an error at byte index pos contributes e * alpha^{i*(N-1-pos)}.
         * Therefore X_k = alpha^{N-1-pos}.
         */
        int     xk_exp  = (RS_N - 1 - pos) % GF_N;
        uint8_t Xk_inv  = gf_alpha_to[(GF_N - xk_exp) % GF_N]; /* X_k^{-1} */

        /* Evaluate Omega at X_k^{-1} */
        uint8_t omega_val = 0;
        for (int i = RS_2T - 1; i >= 0; i--)
            omega_val = gf_add(gf_mul(omega_val, Xk_inv), Omega[i]);

        /* Formal derivative of Lambda at X_k^{-1}:
         * Lambda'(x) = L1 + L3*x^2 + L5*x^4 + ...
         * (odd-index coefficients only; even-index vanish in char-2)
         * Term i: Lambda[i] * Xk_inv^{i-1}
         * Use gf_mul repeatedly instead of gf_pow to avoid large exponents */
        uint8_t lambda_prime = 0;
        uint8_t xk_inv_power = 1;          /* Xk_inv^0 = 1 */
        for (int i = 1; i <= L; i++) {
            /* xk_inv_power = Xk_inv^{i-1} at start of iteration */
            if (i % 2 == 1) {              /* odd index only */
                lambda_prime ^= gf_mul(Lambda[i], xk_inv_power);
            }
            xk_inv_power = gf_mul(xk_inv_power, Xk_inv); /* advance to Xk_inv^i */
        }

        if (lambda_prime == 0) { err_mag[k] = 0; continue; }

        /* Forney formula (FCR=1 → X_k^{1-FCR} = 1):
         * e_k = Omega(X_k^{-1}) / Lambda'(X_k^{-1}) */
        err_mag[k] = gf_div(omega_val, lambda_prime);
    }
}

/* ═══════════════════════════════════════════════════════════════
 * Top-level decoder
 * ═══════════════════════════════════════════════════════════════ */
int rs_decode(uint8_t received[RS_N], uint8_t decoded[RS_K])
{
    /* ── Step 1: Syndromes ─────────────────────────────── */
    uint8_t S[RS_2T];
    if (!compute_syndromes(received, S)) {
        /* All syndromes zero → valid codeword, no correction needed */
        memcpy(decoded, received, RS_K);
        return 0;
    }

    /* ── Step 2: Berlekamp-Massey ──────────────────────── */
    uint8_t Lambda[RS_T + 1];
    int L = berlekamp_massey(S, Lambda);

    if (L > RS_T) {
        return -1;   /* more errors than correctable */
    }

    /* ── Step 3: Chien search ──────────────────────────── */
    uint8_t err_pos[RS_T];
    int num_found = chien_search(Lambda, L, err_pos);

    if (num_found != L) {
        return -1;   /* couldn't locate all error positions */
    }

    /* ── Step 4: Forney magnitudes ─────────────────────── */
    uint8_t err_mag[RS_T];
    forney(S, Lambda, L, err_pos, num_found, err_mag);

    /* ── Step 5: Correct errors ────────────────────────── */
    for (int i = 0; i < num_found; i++) {
        received[err_pos[i]] ^= err_mag[i];
    }

    /* Extract message (first k symbols of corrected codeword) */
    memcpy(decoded, received, RS_K);
    return num_found;
}

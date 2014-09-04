#ifndef ftfp_h
#define ftfp_h

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef uint32_t fixed;

#define FLAGS_MASK 0x3
#define F_NORMAL   0x0
#define F_NAN      0x1
#define F_INF_POS  0x2
#define F_INF_NEG  0x3

#define n_flag_bits 2
#define n_frac_bits 15
#define n_int_bits  15

#define ALL_BIT_MASK 0xffffffff

#define TOP_BIT_MASK (1<<31)
#define TOP_BIT(f) (f & TOP_BIT_MASK)

#define DATA_BIT_MASK (0xFFFFFFFC)
#define DATA_BITS(f) (f & DATA_BIT_MASK)

#define FIX_IS_NEG(f) ((TOP_BIT(f)) == TOP_BIT_MASK)

#define FIX_IS_NAN(f) ((f&FLAGS_MASK) == F_NAN)
#define FIX_IS_INF_POS(f) ((f&FLAGS_MASK) == F_INF_POS)
#define FIX_IS_INF_NEG(f) ((f&FLAGS_MASK) == F_INF_NEG)

#define FIX_EQ(op1, op2) ((FIX_IS_NAN(op1) && FIX_IS_NAN(op2)) || \
    (FIX_IS_INF_POS(op1) && FIX_IS_INF_POS(op2)) || \
    (FIX_IS_INF_NEG(op1) && FIX_IS_INF_NEG(op2)) || \
    (op1 == op2))

#define FIXINT(z) ((z)<<(n_flag_bits+n_frac_bits))

/*
 * General idea:
 *   This creates the fractional portion of a fixed point, given a decimal
 *   fraction. It uses the formula:
 *
 *     "one" / 10^( ceil(log_10( frac )) ) * frac
 *
 *  Along with extra precision bits and rounding to get as close as possible. It
 *  actually looks more like, with rounding tacked on:
 *
 *   (((1<<(n_frac_bits+15)) / ((int) pow(10, log_ceil))) * frac >> 15) << n_flag_bits;
 *
 * Notes:
 *   This works fairly well, and should always give the fixed point that is
 *   closest to the decimal number .frac. That is, FIXFRAC(5) will give 0.5,
 *   etc. We use a horrible strlen preprocessor trick, so FIXFRAC(001) will
 *   give a fixed point close to 0.001.
 *
 *   I'd really like to remove the pow call; doing this in the preprocessor
 *   seems difficult.
 */
#define FIXFRAC(frac) ({ \
    uint32_t log_ceil = ((int) strlen( #frac )); \
    uint32_t one = 1 << (n_frac_bits + 15); \
    uint32_t p = (int) pow(10, log_ceil); \
    uint32_t unit = one / p; \
    uint32_t n = unit * frac; \
    uint32_t round = (n >> 14) & 0x1;\
    ((n >> 15) + (round ? 1 : 0)) << n_flag_bits;\
    })

/* TODO: fix this for variables and not just constants */
#define FIXNUM(i,frac) ({fixed f = (FIXINT(abs(i)) | FIXFRAC(frac)); ( #i[0] == '-' ? fix_neg(f) : f ); })

#define EXTEND_BIT_32(b) ({ uint32_t v = b; \
  v |= v << 1; \
  v |= v << 2; \
  v |= v << 4; \
  v |= v << 8; \
  v |= v << 16; \
  v; })

fixed fix_neg(fixed op1);

fixed fix_sub(fixed op1, fixed op2);

fixed fix_add(fixed op1, fixed op2);

fixed fix_mul(fixed op1, fixed op2);

fixed fix_convert_double(double d);

void fix_print(char* buffer, fixed f);


/*
 * add
 * compare?
 * div
 * sub
 * mul
 * round to integer
 * exp
 *
 * sqrt
 * sin / cos / arctan
 *
 * important constants?
 */

#endif

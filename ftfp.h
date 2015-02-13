#ifndef ftfp_h
#define ftfp_h

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "base.h"
#include "internal.h"

/*
 * TODO:
 * arccos
 * arctan
 * arcsin
 */

#define FIX_NORMAL   ((fixed) 0x0)
#define FIX_NAN      ((fixed) 0x1)
#define FIX_INF_POS  ((fixed) 0x2)
#define FIX_INF_NEG  ((fixed) 0x3)

int8_t fix_is_neg(fixed op1);
int8_t fix_is_nan(fixed op1);
int8_t fix_is_inf_pos(fixed op1);
int8_t fix_is_inf_neg(fixed op1);

// Create a fixnum constant. Use:
//   fixed x = FIX(-3,14159);
//
//We must also check that we didn't accidentally roll over from POS_INF to FIX_MIN...
#define FIXNUM(i,frac) ({ \
        uint8_t neg = (((fixed_signed) (i)) < 0) | (#i[0] == '-'); \
        fixed fnfrac = FIXFRAC(frac); \
        uint8_t inf = (((fixed) (i)) >= FIX_INT_MAX) & \
                        ((((fixed) (i)) < (-((fixed_signed) FIX_INT_MAX))) | \
                        ((((fixed) (i)) == (-((fixed_signed) FIX_INT_MAX))) & (fnfrac != 0x0))); \
        fnfrac = MASK_UNLESS_64( neg , (~fnfrac) + 1 ) | \
                 MASK_UNLESS_64(!neg , fnfrac ); \
        /* if you do this on one line, the compiler complains about shifting
         * some bits off the edge of the world... */ \
        fixed_signed fnint = ((fixed_signed) (i)); \
        fixed f =  (fnint << FIX_POINT_BITS) + (fnfrac); \
    ( MASK_UNLESS_64( (inf & !neg) | (!!FIX_TOP_BIT(f) & !neg), FIX_INF_POS ) | \
      MASK_UNLESS_64( (inf &  neg)                          , FIX_INF_NEG ) | \
      MASK_UNLESS_64(!inf , f )); })

/* Returns true if the numbers are equal (NaNs are always unequal.) */
int8_t fix_eq(fixed op1, fixed op2);

/* Returns true if the numbers are equal (and also if they are both NaN) */
int8_t fix_eq_nan(fixed op1, fixed op2);

/* Returns:
 *   -1 if op1 < op2
 *    0 if they are equal
 *    1 if op1 > op2 (or either is * NaN)
 */
int8_t fix_cmp(fixed op1, fixed op2);

uint8_t fix_le(fixed op1, fixed op2);
uint8_t fix_ge(fixed op1, fixed op2);

uint8_t fix_lt(fixed op1, fixed op2);
uint8_t fix_gt(fixed op1, fixed op2);

// Useful constants
#define FIX_EPSILON     ((fixed) (1 << FIX_FLAG_BITS))
#define FIX_EPSILON_NEG ((fixed) ~((1 << FIX_FLAG_BITS)-1))
#define FIX_ZERO        ((fixed) 0)

#define FIX_MAX     FIX_DATA_BITS((fixed) (((fixed) 1) << (FIX_BITS-1)) -1)
#define FIX_MIN     FIX_DATA_BITS((fixed) ((fixed) 1) << (FIX_BITS-1))

fixed fix_neg(fixed op1);
fixed fix_abs(fixed op1);

fixed fix_add(fixed op1, fixed op2);
fixed fix_sub(fixed op1, fixed op2);
fixed fix_mul(fixed op1, fixed op2);
fixed fix_div(fixed op1, fixed op2);

fixed fix_floor(fixed op1);
fixed fix_ceil(fixed op1);

fixed fix_exp(fixed op1);
fixed fix_ln(fixed op1);
fixed fix_log2(fixed op1);
fixed fix_log10(fixed op1);

fixed fix_sqrt(fixed op1);

/* Computes x^y.
 *
 * Note that this is undefined when x < 0 and y is not an integer, and will
 * return NaN.
 */
fixed fix_pow(fixed x, fixed y);

fixed fix_sin(fixed op1);
fixed fix_cos(fixed op1);
fixed fix_tan(fixed op1);

/* Uses a polynomial approximation of sin. Very quick, but less accurate at the
 * edges. */
fixed fix_sin_fast(fixed op1);

fixed  fix_convert_from_double(double d);
double fix_convert_to_double(fixed op1);

/* Prints the fixed into a buffer in base 10. The buffer must be at least FIX_PRINT_BUFFER_SIZE
 * characters long. */
void fix_print(char* buffer, fixed f);
void fix_print_nospecial(char* buffer, fixed f);
void fix_internal_print(char* buffer, fix_internal f);

/* Note that this is not constant time, but will return a buffer sized to the
 * number. */
void fix_print_variable(char* buffer, fixed f);

#endif

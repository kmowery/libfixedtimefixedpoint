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
 * fix_to_double
 */

#define FIX_NORMAL   0x0
#define FIX_NAN      0x1
#define FIX_INF_POS  0x2
#define FIX_INF_NEG  0x3

#define FIX_FLAG_BITS 2
#define FIX_FRAC_BITS 15
#define FIX_INT_BITS  15

int8_t fix_is_neg(fixed op1);
int8_t fix_is_nan(fixed op1);
int8_t fix_is_inf_pos(fixed op1);
int8_t fix_is_inf_neg(fixed op1);

// Create a fixnum constant. Use:
//   fixed x = FIX(-3,14159);
#define FIXNUM(i,frac) ({fixed f = (FIXINT(abs(i)) + FIXFRAC(frac)); \
    ( MASK_UNLESS((#i[0] == '-') | (i < 0), fix_neg(f)) | \
      MASK_UNLESS((#i[0] != '-') | (i > 0), f) ); })

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

// Useful constants
#define FIX_PI      FIXNUM(3,14159265359)
#define FIX_TAU     FIXNUM(6,28318530718)
#define FIX_E       FIXNUM(2,71828182846)
#define FIX_EPSILON ((fixed) (1 << FIX_FLAG_BITS))
#define FIX_EPSILON_NEG ((fixed) ~((1 << FIX_FLAG_BITS)-1))
#define FIX_ZERO    0

#define FIX_MAX     0x7ffffffc
#define FIX_MIN     0x80000000

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

fixed fix_cordic_sin(fixed op1);
fixed fix_cordic_cos(fixed op1);
fixed fix_cordic_tan(fixed op1);

fixed fix_sin(fixed op1);

fixed fix_convert_double(double d);

void fix_print(char* buffer, fixed f);
void fix_print_const(char* buffer, fixed f);

#endif

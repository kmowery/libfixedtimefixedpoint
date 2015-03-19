#ifndef ftfp_h
#define ftfp_h

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "base.h"

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

// Useful constants
#define FIX_EPSILON     ((fixed) (1 << FIX_FLAG_BITS))
#define FIX_EPSILON_NEG ((fixed) ~((1 << FIX_FLAG_BITS)-1))
#define FIX_ZERO        ((fixed) 0)

#define FIX_MAX     FIX_DATA_BITS((fixed) (((fixed) 1) << (FIX_BITS-1)) -1)
#define FIX_MIN     FIX_DATA_BITS((fixed) ((fixed) 1) << (FIX_BITS-1))

int8_t fix_is_neg(fixed op1);
int8_t fix_is_nan(fixed op1);
int8_t fix_is_inf_pos(fixed op1);
int8_t fix_is_inf_neg(fixed op1);

/* Returns true if the numbers are equal (NaNs are always unequal.) */
int8_t fix_eq(fixed op1, fixed op2);

/* Returns true if the numbers are equal (and also if they are both NaN) */
int8_t fix_eq_nan(fixed op1, fixed op2);

/* Returns true if the numbers are unequal (NaNs are always unequal.) */
int8_t fix_ne(fixed op1, fixed op2);

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


/* Accurate to 2^-57. */
fixed fix_sin(fixed op1);
fixed fix_cos(fixed op1);
fixed fix_tan(fixed op1);

/* Uses a polynomial approximation of sin. Very quick, but less accurate at the
 * edges. */
//fixed fix_sin_fast(fixed op1);

fixed  fix_convert_from_double(double d);
double fix_convert_to_double(fixed op1);

fixed  fix_convert_from_int64(int64_t i);

/* Round to integers */
int64_t fix_convert_to_int64(fixed op1);
int64_t fix_round_up_int64(fixed op1);
int64_t fix_ceil64(fixed op1);
int64_t fix_floor64(fixed op1);

/* Prints the fixed into a buffer in base 10. The buffer must be at least FIX_PRINT_BUFFER_SIZE
 * characters long. */
void fix_sprint(char* buffer, fixed f);
void fix_sprint_nospecial(char* buffer, fixed f);

/* Prints a fixed to STDOUT. */
void fix_print(fixed f);
void fix_println(fixed f);

#endif

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

#define FIX_NORMAL   0x0
#define FIX_NAN      0x1
#define FIX_INF_POS  0x2
#define FIX_INF_NEG  0x3

#define FIX_FLAG_BITS 2
#define FIX_FRAC_BITS 15
#define FIX_INT_BITS  15

#define FIX_IS_NEG(f) ((FIX_TOP_BIT(f)) == (FIX_TOP_BIT_MASK))

#define FIX_IS_NAN(f) (((f)&FIX_FLAGS_MASK) == FIX_NAN)
#define FIX_IS_INF_POS(f) (((f)&FIX_FLAGS_MASK) == FIX_INF_POS)
#define FIX_IS_INF_NEG(f) (((f)&FIX_FLAGS_MASK) == FIX_INF_NEG)

// Create a fixnum constant. Use:
//   fixed x = FIX(-3,14159);
#define FIXNUM(i,frac) ({fixed f = (FIXINT(abs(i)) + FIXFRAC(frac)); \
    ( MASK_UNLESS((#i[0] == '-') | (i < 0), fix_neg(f)) | \
      MASK_UNLESS((#i[0] != '-') | (i > 0), f) ); })


/* Returns true if the numbers are equal (NaNs are always unequal.) */
#define FIX_EQ(op1, op2) ( \
    (!(FIX_IS_NAN(op1) | FIX_IS_NAN(op2))) &    \
    ((FIX_IS_INF_POS(op1) & FIX_IS_INF_POS(op2)) | \
    (FIX_IS_INF_NEG(op1) & FIX_IS_INF_NEG(op2)) | \
    ((op1) == (op2))))

/* Returns true if the numbers are equal (and also if they are both NaN) */
#define FIX_EQ_NAN(op1, op2) ( \
    (FIX_IS_NAN(op1) & FIX_IS_NAN(op2)) | \
    (FIX_IS_INF_POS(op1) & FIX_IS_INF_POS(op2)) | \
    (FIX_IS_INF_NEG(op1) & FIX_IS_INF_NEG(op2)) | \
    ((op1) == (op2)))

#define FIX_CMP(op1, op2) ({ \
      uint32_t nans = (!!(FIX_IS_NAN(op1)) | FIX_IS_NAN(op2));          \
      uint32_t gt = 0, lt = 0, pos1 = 0, pos2 = 0, cmp_gt = 0, cmp_lt = 0; \
      pos1 = !FIX_TOP_BIT(op1); pos2 = !FIX_TOP_BIT(op2); \
      gt = (FIX_IS_INF_POS(op1) & (!FIX_IS_INF_POS(op2))) | ((!FIX_IS_INF_NEG(op1)) & FIX_IS_INF_NEG(op2)); \
      lt = ((!FIX_IS_INF_POS(op1)) & FIX_IS_INF_POS(op2)) | (FIX_IS_INF_NEG(op1) & (!FIX_IS_INF_NEG(op2))); \
      gt |= (pos1 & (!pos2));                                           \
      lt |= ((!pos1) & pos2);                                           \
      cmp_gt = ((fixed) (op1) > (fixed) (op2)); \
      cmp_lt = ((fixed) (op1) < (fixed) (op2)); \
      uint32_t result = (((nans)? 1 : 0) |              \
       ((gt & (!nans))? 1 : 0) |                     \
       ((lt & (!nans))? 0xffffffff : 0) |            \
       ((cmp_gt & (!(gt | lt | nans)))? 1 : 0) |      \
       ((cmp_lt & (!(gt|lt|nans)))? -1 : 0));        \
       (result); })

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

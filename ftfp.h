#ifndef ftfp_h
#define ftfp_h

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/*
 * TODO:
 * exp
 * log10
 *
 * cos / arccos
 * tan / arctan
 * arcsin
 */


typedef uint32_t fixed;

#define FIX_FLAGS_MASK 0x3
#define FIX_NORMAL   0x0
#define FIX_NAN      0x1
#define FIX_INF_POS  0x2
#define FIX_INF_NEG  0x3

#define FIX_FLAG_BITS 2
#define FIX_FRAC_BITS 15
#define FIX_INT_BITS  15
#define FIX_POINT_BITS (FIX_FRAC_BITS + FIX_FLAG_BITS)

#define FIX_ALL_BIT_MASK 0xffffffff

#define FIX_TOP_BIT_MASK (1<<31)
#define FIX_TOP_BIT(f) ((f) & FIX_TOP_BIT_MASK)

#define FIX_FRAC_MASK (((1<<(FIX_FRAC_BITS))-1) << (FIX_FLAG_BITS))
#define FIX_INT_MASK  (((1<<(FIX_INT_BITS))-1) << ((FIX_FLAG_BITS) + (FIX_FRAC_BITS)))

#define FIX_SIGN_TO_64(f) ((int64_t)((int32_t)(f)))

#define SIGN_EXTEND(value, n_top_bit) ({uint32_t SE_m__ = (1 << ((n_top_bit)-1)); (((uint32_t) (value)) ^ SE_m__) - SE_m__;})
#define SIGN_EX_SHIFT_RIGHT_32(value, shift) SIGN_EXTEND( (value) >> (shift), 32 - (shift) )
#define MASK_UNLESS(expression, value) (SIGN_EXTEND(!!(expression), 1) & (value))

#define FIX_DATA_BIT_MASK (0xFFFFFFFC)
#define FIX_DATA_BITS(f) ((f) & FIX_DATA_BIT_MASK)

#define FIX_IS_NEG(f) ((FIX_TOP_BIT(f)) == (FIX_TOP_BIT_MASK))

#define FIX_IS_NAN(f) (((f)&FIX_FLAGS_MASK) == FIX_NAN)
#define FIX_IS_INF_POS(f) (((f)&FIX_FLAGS_MASK) == FIX_INF_POS)
#define FIX_IS_INF_NEG(f) (((f)&FIX_FLAGS_MASK) == FIX_INF_NEG)

#define FIX_IF_NAN(isnan) (((isnan) | ((isnan) << 1)) & FIX_NAN)
#define FIX_IF_INF_POS(isinfpos) (((isinfpos) | ((isinfpos) << 1)) & FIX_INF_POS)
#define FIX_IF_INF_NEG(isinfneg) (((isinfneg) | ((isinfneg) << 1)) & FIX_INF_NEG)

/* Returns true if the numbers are equal (NaNs are always unequal.) */
#define FIX_EQ(op1, op2) ( \
    (!(FIX_IS_NAN(op1) | FIX_IS_NAN(op2))) &        \
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
      uint32_t nans = !!(FIX_IS_NAN(op1) | FIX_IS_NAN(op2)); \
      uint32_t gt = 0, lt = 0, pos1 = 0, pos2 = 0, cmp_gt = 0, cmp_lt = 0; \
      pos1 = !FIX_TOP_BIT(op1); pos2 = !FIX_TOP_BIT(op2); \
      gt = (FIX_IS_INF_POS(op1) && !FIX_IS_INF_POS(op2)) | (!FIX_IS_INF_NEG(op1) && FIX_IS_INF_NEG(op2)); \
      lt = (!FIX_IS_INF_POS(op1) && FIX_IS_INF_POS(op2)) | (FIX_IS_INF_NEG(op1) && !FIX_IS_INF_NEG(op2)); \
      gt |= (pos1 && !pos2); \
      lt |= (!pos1 && pos2); \
      cmp_gt = ((fixed) (op1) > (fixed) (op2)); \
      cmp_lt = ((fixed) (op1) < (fixed) (op2)); \
      uint32_t result = ((nans ? 1 : 0) | \
        (gt & !nans ? 1 : 0) | \
        (lt & !nans ? 0xffffffff : 0) | \
        (cmp_gt && !(gt | lt | nans) ? 1 : 0) | \
        (cmp_lt && !(gt|lt|nans) ? -1 : 0)); \
       (result); })

/* Lops off the rightmost n_shift_bits of value and rounds to an even value
 * (so 0.5 will round to 0, but 1.5 will round to 2)
 *
 * n_shift_bits must be >- 2.
 *
 * Round Truth table:
 *  Bit 0 1 2       Result
 *      0 0 0       0 0x0
 *      0 0 1       0 0x0
 *      0 1 0       0 0x0
 *      0 1 1       1 0x1
 *      1 0 0       0 0x1
 *      1 0 1       0 0x1
 *      1 1 0       1 0x2
 *      1 1 1       1 0x2
 * The repeated phrase
 *
 *    ((!!(value & ((1 << (n_shift_bits-1))-1))) << (n_shift_bits-1))
 *
 *  is there to compress all lower-order bits into bit 2 in the truth table
 * */
#define ROUND_TO_EVEN(value, n_shift_bits) \
  (((value) >> (n_shift_bits)) + \
   !!( \
     (((value) & (1 << ((n_shift_bits)-1))) && !!((value) & ((1 << ((n_shift_bits)-1))-1))) | \
     ((((value) >> ((n_shift_bits)-2)) & 0x6) == 0x6) \
   ))

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
 *   (((1<<(FIX_FRAC_BITS+15)) / ((int) pow(10, log_ceil))) * frac >> 15) << FIX_FLAG_BITS;
 *
 * Notes:
 *   This works fairly well, and should always give the fixed point that is
 *   closest to the decimal number .frac. That is, FIXFRAC(5) will give 0.5,
 *   etc. We use a horrible strlen preprocessor trick, so FIXFRAC(001) will
 *   give a fixed point close to 0.001.
 *
 *   I'd really like to remove the pow call; doing this in the preprocessor
 *   seems difficult.
 *
 *   To prevent octal assignment, we do some nonsense into frac_int.
 */
#define FIXFRAC(frac) ({ \
    uint32_t bits = 4; \
    uint64_t log_ceil = ((uint64_t) strlen( #frac )); \
    uint64_t one = 1ULL << (FIX_FRAC_BITS + bits); \
    uint64_t p = (uint64_t) pow(10, log_ceil); \
    uint64_t frac_int =  1 ## frac - ((int64_t) pow(10, log_ceil)); \
    uint64_t n = (frac_int * one) / p; \
    ((ROUND_TO_EVEN(n, bits)) << FIX_FLAG_BITS); \
    })

#define FIXINT(z) ((z)<<(FIX_FLAG_BITS+FIX_FRAC_BITS))

#define FIXNUM(i,frac) ({fixed f = (FIXINT(abs(i)) + FIXFRAC(frac)); \
    ( MASK_UNLESS((#i[0] == '-') | (i < 0), fix_neg(f)) | \
      MASK_UNLESS((#i[0] != '-') | (i > 0), f) ); })

#define FIX_PI      FIXNUM(3,14159265359)
#define FIX_TAU     FIXNUM(6,28318530718)
#define FIX_E       FIXNUM(2,71828182846)
#define FIX_EPSILON ((fixed) (1 << FIX_FLAG_BITS))
#define FIX_ZERO    0

#define FIX_MAX     0x7ffffffc
#define FIX_MIN     0x80000000

/* We do this stupid nosign thing to prevent FIX_MAX from rolling over into the
 * negative. It 'rounds' to FIX_MAX + 1 == FIX_MIN, which causes trouble.
 * There's gotta be a better way... */

#define FIX_ROUND_BASE(op1, round_exp) ({ \
    uint8_t isinfpos = FIX_IS_INF_POS(op1); \
    uint8_t isinfneg = FIX_IS_INF_NEG(op1); \
    uint8_t isnan = FIX_IS_NAN(op1); \
    uint8_t isneg = FIX_IS_NEG(op1); \
    uint8_t ex = isinfpos | isinfneg | isnan; \
    uint32_t result_nosign = round_exp; \
    uint32_t result = SIGN_EXTEND(result_nosign, FIX_INT_BITS); \
    (MASK_UNLESS(isinfpos, INT_MAX) | \
     MASK_UNLESS(isinfneg, INT_MIN) | \
     MASK_UNLESS(isneg  & !ex, result) | \
     MASK_UNLESS(!isneg & !ex, result_nosign)); \
    })

/* Uses round to even semantics */
#define FIX_ROUND_INT(op1) \
    FIX_ROUND_BASE(op1, ROUND_TO_EVEN(op1, FIX_FLAG_BITS + FIX_FRAC_BITS))

/* 0.5 rounds up always */
#define FIX_ROUND_UP_INT(op1) \
  FIX_ROUND_BASE(op1, (((op1) >> (FIX_FLAG_BITS + FIX_FRAC_BITS)) + (op1 >> (FIX_FLAG_BITS + FIX_FRAC_BITS-1) & 0x1)))

#define FIX_CEIL(op1) \
  FIX_ROUND_BASE(op1, ((op1) >> (FIX_FLAG_BITS + FIX_FRAC_BITS)) + !!(op1 & FIX_FRAC_MASK))

#define FIX_FLOOR(op1) \
  FIX_ROUND_BASE(op1, ((op1) >> (FIX_FLAG_BITS + FIX_FRAC_BITS)))


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

fixed fix_sin(fixed op1);

fixed fix_convert_double(double d);

void fix_print(char* buffer, fixed f);
void fix_print_const(char* buffer, fixed f);

#endif

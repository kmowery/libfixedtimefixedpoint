#include "ftfp.h"
#include "internal.h"
#include <math.h>

#include <stdint.h>

int8_t fix_is_neg(fixed op1) {
  return FIX_IS_NEG(op1);
}
int8_t fix_is_nan(fixed op1) {
  return FIX_IS_NAN(op1);
}
int8_t fix_is_inf_pos(fixed op1) {
  return FIX_IS_INF_POS(op1);
}
int8_t fix_is_inf_neg(fixed op1) {
  return FIX_IS_INF_NEG(op1);
}
int8_t fix_eq(fixed op1, fixed op2) {
  return FIX_EQ(op1, op2);
}
int8_t fix_eq_nan(fixed op1, fixed op2) {
  return FIX_EQ_NAN(op1, op2);
}
int8_t fix_ne(fixed op1, fixed op2) {
  return !FIX_EQ(op1, op2);
}

int8_t fix_cmp(fixed op1, fixed op2) {
  uint32_t nans = !!(FIX_IS_NAN(op1) | FIX_IS_NAN(op2));

  uint32_t pos1 = !FIX_IS_NEG(op1);
  uint32_t pos2 = !FIX_IS_NEG(op2);

  uint32_t gt = (  FIX_IS_INF_POS(op1)  & (!FIX_IS_INF_POS(op2))) |
                ((!FIX_IS_INF_NEG(op1)) &   FIX_IS_INF_NEG(op2));
  uint32_t lt = ((!FIX_IS_INF_POS(op1)) &   FIX_IS_INF_POS(op2)) |
                  (FIX_IS_INF_NEG(op1)  & (!FIX_IS_INF_NEG(op2)));

  gt |= (!lt) & (pos1 & (!pos2));
  lt |= (!gt) & ((!pos1) & pos2);

  uint32_t cmp_gt = ((fixed) (op1) > (fixed) (op2));
  uint32_t cmp_lt = ((fixed) (op1) < (fixed) (op2));

  int8_t result =
    MASK_UNLESS( nans, 1 ) |
    MASK_UNLESS( !nans,
        MASK_UNLESS( gt, 1) |
        MASK_UNLESS( lt, -1) |
        MASK_UNLESS(!(gt|lt),
          MASK_UNLESS(cmp_gt, 1) |
          MASK_UNLESS(cmp_lt, -1)));
  return result;
}

uint8_t fix_le(fixed op1, fixed op2) {
  uint32_t nans = !!(FIX_IS_NAN(op1) | FIX_IS_NAN(op2));
  int8_t result = fix_cmp(op1, op2);

  return MASK_UNLESS(!nans, result <= 0);
}

uint8_t fix_ge(fixed op1, fixed op2) {
  uint32_t nans = !!(FIX_IS_NAN(op1) | FIX_IS_NAN(op2));
  int8_t result = fix_cmp(op1, op2);

  return MASK_UNLESS(!nans, result >= 0);
}

uint8_t fix_lt(fixed op1, fixed op2) {
  uint32_t nans = !!(FIX_IS_NAN(op1) | FIX_IS_NAN(op2));
  int8_t result = fix_cmp(op1, op2);

  return MASK_UNLESS(!nans, result < 0);
}

uint8_t fix_gt(fixed op1, fixed op2) {
  uint32_t nans = !!(FIX_IS_NAN(op1) | FIX_IS_NAN(op2));
  int8_t result = fix_cmp(op1, op2);

  return MASK_UNLESS(!nans, result > 0);
}


fixed fix_neg(fixed op1){
  // Flip our infs
  // NaN is still NaN
  // Because we're two's complement, FIX_MIN has no inverse. Make it positive
  // infinity...
  uint8_t isinfpos = FIX_IS_INF_NEG(op1) | (op1 == FIX_MIN);
  uint8_t isinfneg = FIX_IS_INF_POS(op1);
  uint8_t isnan = FIX_IS_NAN(op1);

  // 2s comp negate the data bits
  fixed tempresult = FIX_DATA_BITS(((~op1) + 4));

  // Combine
  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
    FIX_DATA_BITS(tempresult);
}

fixed fix_abs(fixed op1){
  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1);
  uint8_t isnan = FIX_IS_NAN(op1);

  fixed tempresult = MASK_UNLESS(FIX_TOP_BIT(~op1),                  op1       ) |
                     MASK_UNLESS(FIX_TOP_BIT( op1), FIX_DATA_BITS(((~op1) + 4)));

  /* check for FIX_MIN */
  isinfpos |= (!(isinfpos | isinfneg)) & (!!FIX_TOP_BIT(op1)) & (op1 == tempresult);

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS((isinfpos | isinfneg) & (!isnan)) |
    FIX_DATA_BITS(tempresult);
}

fixed fix_sub(fixed op1, fixed op2) {
  return fix_add(op1,fix_neg(op2));
}

/* Here's what we want here (N is nonzero normal)
 *  op1    op2     result
 * -----------------------
 *   N      N        N
 *   0      N        0
 *
 *   N      0       Inf
 *  -N      0      -Inf
 *  Inf     0       Inf
 * -Inf     0      -Inf
 *  NaN     0       NaN
 *
 *   0  +/-Inf       0
 *   0      N        0
 *   0     NaN      NaN
 *
 *  Inf    Inf      Inf
 *   N     Inf       0
 *   0     Inf       0
 *  Nan    Inf      NaN
 */
fixed fix_div(fixed op1, fixed op2) {
  uint8_t isinf = 0;

  fixed tempresult = fix_div_64(op1, op2, &isinf);

  uint8_t divbyzero = op2 == FIX_ZERO;

  uint8_t isinfop1 = (FIX_IS_INF_NEG(op1) | FIX_IS_INF_POS(op1));
  uint8_t isinfop2 = (FIX_IS_INF_NEG(op2) | FIX_IS_INF_POS(op2));

  uint8_t isnegop1 = FIX_IS_INF_NEG(op1) | (FIX_IS_NEG(op1) & !isinfop1);
  uint8_t isnegop2 = FIX_IS_INF_NEG(op2) | (FIX_IS_NEG(op2) & !isinfop2);

  uint8_t isnan = FIX_IS_NAN(op1) | FIX_IS_NAN(op2) | ((op1 == FIX_ZERO) & (op2 == FIX_ZERO));

  isinf = (isinf | isinfop1) & (!isnan);
  uint8_t isinfpos = (isinf & !(isnegop1 ^ isnegop2)) | (divbyzero & !isnegop1);
  uint8_t isinfneg = (isinf & (isnegop1 ^ isnegop2)) | (divbyzero & isnegop1);

  uint8_t iszero = (!(isinfop1)) & isinfop2;

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan) & (!iszero)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan) & (!iszero)) |
    MASK_UNLESS(!iszero, FIX_DATA_BITS(tempresult));
}


fixed fix_mul(fixed op1, fixed op2) {

  uint8_t isinfop1 = (FIX_IS_INF_NEG(op1) | FIX_IS_INF_POS(op1));
  uint8_t isinfop2 = (FIX_IS_INF_NEG(op2) | FIX_IS_INF_POS(op2));
  uint8_t isnegop1 = FIX_IS_INF_NEG(op1) | (FIX_IS_NEG(op1) & !isinfop1);
  uint8_t isnegop2 = FIX_IS_INF_NEG(op2) | (FIX_IS_NEG(op2) & !isinfop2);

  uint8_t isnan = FIX_IS_NAN(op1) | FIX_IS_NAN(op2);
  uint8_t isinf = 0;

  uint8_t iszero = (op1 == FIX_ZERO) | (op2 == FIX_ZERO);

  fixed tmp = ROUND_TO_EVEN(FIX_MUL_64(op1, op2, isinf), FIX_FLAG_BITS) << FIX_FLAG_BITS;

  isinf = (!iszero) & (isinfop1 | isinfop2 | isinf) & (!isnan);

  uint8_t isinfpos = isinf & !(isnegop1 ^ isnegop2);
  uint8_t isinfneg = isinf & (isnegop1 ^ isnegop2);

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
    FIX_DATA_BITS(tmp);
}


fixed fix_add(fixed op1, fixed op2) {
  uint8_t isnan;
  uint8_t isinfpos;
  uint8_t isinfneg;

  fixed tempresult;

  isnan = FIX_IS_NAN(op1) | FIX_IS_NAN(op2);
  isinfpos = FIX_IS_INF_POS(op1) | FIX_IS_INF_POS(op2);
  isinfneg = FIX_IS_INF_NEG(op1) | FIX_IS_INF_NEG(op2);

  tempresult = op1 + op2;

  // check if we're overflowing: adding two positive numbers that results in a
  // 'negative' number:
  //   if both inputs are positive (top bit == 0) and the result is 'negative'
  //   (top bit nonzero)
  isinfpos |= ((FIX_TOP_BIT(op1) | FIX_TOP_BIT(op2)) == 0x0)
    & (FIX_TOP_BIT(tempresult) != 0x0);

  // check if there's negative infinity overflow
  isinfneg |= ((FIX_TOP_BIT(op1) & FIX_TOP_BIT(op2)) == FIX_TOP_BIT_MASK)
    & (FIX_TOP_BIT(tempresult) == 0x0);

  // Force infpos to win in cases where it is unclear
  isinfneg &= !isinfpos;

  // do some horrible bit-ops to make result into what we want

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
    FIX_DATA_BITS(tempresult);
}

fixed fix_floor(fixed op1) {
  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1);
  uint8_t isnan = FIX_IS_NAN(op1);

  fixed tempresult = op1 & ~((1ull << (FIX_POINT_BITS))-1);

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
    FIX_DATA_BITS(tempresult);
}

fixed fix_ceil(fixed op1) {
  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1);
  uint8_t isnan = FIX_IS_NAN(op1);
  uint8_t ispos = !FIX_IS_NEG(op1);

  fixed frac_mask = (((fixed) 1) << (FIX_POINT_BITS))-1;

  fixed tempresult = (op1 & ~frac_mask) +
    MASK_UNLESS(!!(op1 & frac_mask),  (((fixed) 1) << (FIX_POINT_BITS)));

  // If we used to be positive and we wrapped around, switch to INF_POS.
  isinfpos |= ((tempresult == FIX_MIN) & ispos);

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
    FIX_DATA_BITS(tempresult);
}

//fixed fix_sin_fast(fixed op1) {
//  uint8_t isinfpos;
//  uint8_t isinfneg;
//  uint8_t isnan;
//
//  isinfpos = FIX_IS_INF_POS(op1);
//  isinfneg = FIX_IS_INF_NEG(op1);
//  isnan = FIX_IS_NAN(op1);
//
//  /* Math:
//   *
//   * See http://www.coranac.com/2009/07/sines/ for a great overview.
//   *
//   * Fifth order taylor approximation:
//   *
//   *   Sin_5(x) = ax - bx^3 + cx^5
//   *
//   * where:
//   *
//   *   a = 1.569718634 (almost but not quite pi/2)
//   *   b = 2a - (5/2)
//   *   c = a - (3/2)
//   *   Constants minimise least-squared error (according to Coranac).
//   *
//   * Simplify for computation:
//   *
//   *   Sin_5(x) = az - bz^3 + cz^5
//   *            = z(a + (-bz^2 + cz^4))
//   *            = z(a + z^2(cz^2 - b))
//   *            = z(a - z^2(b - cz^2))
//   *
//   *   where z = x / (tau/4).
//   *
//   */
//
//  uint32_t circle_frac = fix_circle_frac(op1);
//
//  /* for sin, we need to map the circle frac [0,4) to [-1, 1]:
//   *
//   * Z' =    2 - Z       { if 1 <= z < 3
//   *         Z           { otherwise
//   *
//   * zp =                                   # bits: xx.2.28
//   *         (1<<31) - circle_frac { if 1 <= circle_frac[29:28] < 3
//   *         circle_frac           { otherwise
//   */
//  uint32_t top_bits_differ = ((circle_frac >> 28) & 0x1) ^ ((circle_frac >> 29) & 0x1);
//  uint32_t zp = MASK_UNLESS(top_bits_differ, (1<<29) - circle_frac) |
//                MASK_UNLESS(!top_bits_differ, SIGN_EXTEND_64(circle_frac, 30));
//
//  uint32_t zp2 = MUL_2x28(zp, zp);
//
//  uint32_t a = 0x64764525; // a = 1.569718634; "0x%08x"%(a*2**30)"
//  uint32_t b = 0x28ec8a4a; // "0x%08x"%((2*a - (5/2.)) *2**30)
//  uint32_t c = 0x04764525; // "0x%08x"%((a - (3/2.)) *2**30)
//
//  uint32_t result =
//    MUL_2x28(zp,
//        (a - MUL_2x28(zp2,
//                          b - MUL_2x28(c, zp2))));
//
//  // result is xx.2.28, shift over into fixed, sign extend to full result
//  return FIX_IF_NAN(isnan) |
//    FIX_IF_INF_POS(isinfpos & (!isnan)) |
//    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
//    convert_228_to_fixed(result);
//}

fixed fix_convert_from_double(double d) {
  uint64_t bits = *(uint64_t*) &d;
  uint32_t exponent_base = ((bits >> 52) & 0x7ff);
  uint64_t mantissa_base = (bits & ((1ull <<52)-1));
  uint8_t d_is_zero = (exponent_base == 0) & (mantissa_base == 0);

  uint32_t exponent = exponent_base - 1023;
  uint32_t sign = bits >> 63;

  /* note that this breaks with denorm numbers. However, we'll shift those all
   * away with the exponent later */
  uint64_t mantissa = mantissa_base | MASK_UNLESS_64(!d_is_zero, (1ull << 52));

  int32_t shift = 52 - (FIX_FRAC_BITS) - exponent;

  fixed result = FIX_ALL_BIT_MASK & (
      MASK_UNLESS((shift >= 2) & (shift <  64), ((ROUND_TO_EVEN(mantissa,shift)) << FIX_FLAG_BITS)) |
      MASK_UNLESS (shift == 1                 , (ROUND_TO_EVEN_ONE_BIT(mantissa) << FIX_FLAG_BITS)) |
      MASK_UNLESS((shift <= 0) & (shift > -64), (mantissa << (-shift + 2))));

  /* If there are any integer bits that we shifted off into oblivion, the double
   * is outside our range. Generate INF... */
  uint8_t lostbits = MASK_UNLESS(shift <= 0, mantissa != (result >> (-shift+2)));

  /* use IEEE 754 definition of INF */
  uint8_t isinf = (exponent_base == 0x7ff) & (mantissa_base == 0);

  /* If we lost any bits by shifting, kill it. */
  isinf |= lostbits;

  /* Since doubles have a sign bit and we're two's complement, the other
   * INFINITY case is if the double is >= FIX_INT_MAX and positive, or >
   * FIX_INT_MAX and negative. */
  isinf |= ((result >= FIX_TOP_BIT_MASK) & !sign);
  isinf |= ((result >  FIX_TOP_BIT_MASK) &  sign);

  uint8_t isinfpos = (sign == 0) & isinf;
  uint8_t isinfneg = (sign != 0) & isinf;
  uint8_t isnan = (exponent_base == 0x7ff) && (mantissa_base != 0);

  fixed result_neg = fix_neg(result);

  return
    FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & !isnan) |
    FIX_IF_INF_NEG(isinfneg & !isnan) |
    MASK_UNLESS(!sign, FIX_DATA_BITS(result)) |
    MASK_UNLESS(sign, FIX_DATA_BITS(result_neg));
}

double fix_convert_to_double(fixed op1) {
  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1);
  uint8_t isnan = FIX_IS_NAN(op1);
  uint8_t exception = isinfpos | isinfneg | isnan;

  // Doubles don't use two's complement. Record the sign and flip back into positive land...
  uint64_t sign = FIX_IS_NEG(op1);
  op1 = fix_abs(op1);

  uint32_t log2op1 = uint64_log2(op1);

  int32_t shift = 53 - 1 - log2op1;

  uint64_t mantissa = ((1ull << 52) - 1) & (
      MASK_UNLESS(shift >= 0, ((uint64_t) op1) << shift) |
      MASK_UNLESS(shift < 0, ((uint64_t) op1) >> (-shift)));
  uint64_t exponent = MASK_UNLESS_64(op1 != (fixed) 0, log2op1 - FIX_POINT_BITS + 1023);

  // We would include
  //  MASK_UNLESS( isinfpos | isinfneg , 0 ),
  // but that's implied by masking everything else.
  mantissa =
    MASK_UNLESS_64( isnan, 1 ) |
    MASK_UNLESS_64(!exception, mantissa );

  sign =
    MASK_UNLESS_64( isinfneg, 1ull ) |
    MASK_UNLESS_64(!exception, sign );

  exponent =
    MASK_UNLESS_64( exception, 0x7ff) |
    MASK_UNLESS_64(!exception, exponent);

  uint64_t result = (sign << 63) |
    (exponent << 52) |
    (mantissa);

  double d = *(double*) &result;
  return d;
}

fixed fix_convert_from_int64(int64_t i) {
  fixed_signed fnint = (fixed_signed) i;
  uint8_t isinfpos = (fnint >= ((fixed_signed) FIX_INT_MAX));
  uint8_t isinfneg = (fnint < (-((fixed_signed) FIX_INT_MAX)));

  fixed f = ((fixed_signed) i) << (FIX_POINT_BITS);

  return FIX_IF_INF_POS(isinfpos) |
         FIX_IF_INF_NEG(isinfneg) |
         MASK_UNLESS(!(isinfpos | isinfneg), f);
}


int64_t fix_convert_to_int64(fixed op1) {
  return FIX_ROUND_INT64(op1);
}
int64_t fix_round_up_int64(fixed op1) {
  return FIX_ROUND_UP_INT64(op1);
}
int64_t fix_ceil64(fixed op1) {
  return FIX_CEIL64(op1);
}
int64_t fix_floor64(fixed op1) {
  return FIX_FLOOR64(op1);
}

void fix_print(fixed f) {
  char buf[FIX_PRINT_BUFFER_SIZE];
  fix_sprint(buf, f);
  printf("%s", buf);
}
void fix_println(fixed f) {
  fix_print(f);
  printf("\n");
}

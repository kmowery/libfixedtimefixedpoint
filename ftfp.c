#include "ftfp.h"
#include <math.h>

fixed fix_neg(fixed op1){
  uint8_t isinfpos;
  uint8_t isinfneg;
  uint8_t isnan;

  fixed tempresult;

  // Flip our infs
  isinfpos = FIX_IS_INF_NEG(op1);
  isinfneg = FIX_IS_INF_POS(op1);

  // NaN is still NaN
  isnan = FIX_IS_NAN(op1);

  // 2s comp negate the data bits
  tempresult = DATA_BITS(((~op1) + 4));

  //TODO: Check overflow? other issues?


  // Combine
  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
    DATA_BITS(tempresult);
}

fixed fix_sub(fixed op1, fixed op2) {
  return fix_add(op1,fix_neg(op2));
}

fixed fix_div(fixed op1, fixed op2) {
  uint8_t isnan;
  uint8_t isinf;
  uint8_t isinfpos;
  uint8_t isinfneg;

  uint8_t isinfop1;
  uint8_t isinfop2;
  uint8_t isnegop1;
  uint8_t isnegop2;

  uint64_t tmp;
  uint64_t tmp2;

  fixed tempresult,op2nz;

  isnan = FIX_IS_NAN(op1) | FIX_IS_NAN(op2) | (op2 == 0);

  // Take advantage of the extra bits we get out from doing this in uint64_t
  // op2 is never allowed to be 0, if it is set it to something like 1 so div doesn't fall over
  op2nz = op2 | (DATA_BITS(op2) == 0);
  tmp = ROUND_TO_EVEN(((FIX_SIGN_TO_64(DATA_BITS(op1))<<32) /
                       FIX_SIGN_TO_64(op2nz)),17)<<2;

  tmp2 = tmp & 0xFFFFFFFF00000000;
  isinf = !((tmp2 == 0xFFFFFFFF00000000) | (tmp2 == 0));

  tempresult = tmp & 0xFFFFFFFC;

  isinfop1 = (FIX_IS_INF_NEG(op1) | FIX_IS_INF_POS(op1));
  isinfop2 = (FIX_IS_INF_NEG(op2) | FIX_IS_INF_POS(op2));
  isnegop1 = FIX_IS_INF_NEG(op1) | (FIX_IS_NEG(op1) & !isinfop1);
  isnegop2 = FIX_IS_INF_NEG(op2) | (FIX_IS_NEG(op2) & !isinfop2);

  //Update isinf
  isinf = (isinf | isinfop1 | isinfop2) & (!isnan);

  isinfpos = isinf & !(isnegop1 ^ isnegop2);

  isinfneg = isinf & (isnegop1 ^ isnegop2);

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
    DATA_BITS(tempresult);
}


fixed fix_mul(fixed op1, fixed op2) {

  uint8_t isnan;
  uint8_t isinf;
  uint8_t isinfpos;
  uint8_t isinfneg;

  uint8_t isinfop1;
  uint8_t isinfop2;
  uint8_t isnegop1;
  uint8_t isnegop2;

  uint64_t tmp;
  uint64_t tmp2;

  fixed tempresult;

  isnan = FIX_IS_NAN(op1) | FIX_IS_NAN(op2);

  // Sign extend it all, this will help us correctly catch overflow
  tmp = ROUND_TO_EVEN(FIX_SIGN_TO_64(op1) * FIX_SIGN_TO_64(op2),19)<<2;
   //tmp = (FIX_SIGN_TO_64(op1) * FIX_SIGN_TO_64(op2)) >>17;

  // inf only if overflow, and not a sign thing
  tmp2 = tmp & 0xFFFFFFFF00000000;
  isinf = !((tmp2 == 0xFFFFFFFF00000000) | (tmp2 == 0));

  tempresult = tmp & 0xFFFFFFFC;

  //TODO Cache these maybe due to O0?
  isinfop1 = (FIX_IS_INF_NEG(op1) | FIX_IS_INF_POS(op1));
  isinfop2 = (FIX_IS_INF_NEG(op2) | FIX_IS_INF_POS(op2));
  isnegop1 = FIX_IS_INF_NEG(op1) | (FIX_IS_NEG(op1) & !isinfop1);
  isnegop2 = FIX_IS_INF_NEG(op2) | (FIX_IS_NEG(op2) & !isinfop2);

  //Update isinf
  isinf = (isinfop1 | isinfop2 | isinf) & (!isnan);

  isinfpos = isinf & !(isnegop1 ^ isnegop2);

  isinfneg = isinf & (isnegop1 ^ isnegop2);

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
    DATA_BITS(tempresult);
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
  isinfpos |= (TOP_BIT(op1) | TOP_BIT(op2)) ==
    0x0 && (TOP_BIT(tempresult) != 0x0);

  // check if there's negative infinity overflow
  isinfneg |= (TOP_BIT(op1) & TOP_BIT(op2)) ==
    TOP_BIT_MASK && (TOP_BIT(tempresult) == 0x0);

  // Force infpos to win in cases where it is unclear
  isinfneg &= !isinfpos;

  //printf("isnan: %x\n", isnan);
  //printf("isinfpos: %x\n", isinfpos);
  //printf("isinfneg: %x\n", isinfneg);
  //printf("bits: %x\n", isnan | isinfpos | isinfneg );
  //printf("result: %x\n", tempresult);
  //printf("bit: %x\n", !(isnan | isinfpos | isinfneg));
  //printf("extend: %x\n", EXTEND_BIT_32(!(isnan | isinfpos | isinfneg)));

  // do some horrible bit-ops to make result into what we want

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
    DATA_BITS(tempresult);
}

fixed fix_sin(fixed op1) {
  //  uint8_t isinfpos;
  //  uint8_t isinfneg;
  //  uint8_t isnan;

  //  isinfpos = FIX_IS_INF_POS(op1);
  // isinfneg = FIX_IS_INF_NEG(op1);
  // isnan = FIX_IS_NAN(op1);

  /* Math:
   *
   * See http://www.coranac.com/2009/07/sines/ for a great overview.
   *
   * Fifth order taylor approximation:
   *
   *   Sin_5(x) = ax - bx^3 + cx^5
   *
   * where:
   *
   *   a = 1.569718634 (almost but not quite pi/2)
   *   b = 2a - (5/2)
   *   c = a - (3/2)
   *   Constants minimise least-squared error (according to Coranac).
   *
   * Simplify for computation:
   *
   *   Sin_5(x) = az - bz^3 + cz^5
   *            = z(a + (-bz^2 + cz^4))
   *            = z(a + z^2(cz^2 - b))
   *            = z(a - z^2(b - cz^2))
   *
   *   where z = x / (tau/4).
   *
   */

  /* Scratchpad to compute z:
   *
   * variables are lowercase, and considered as integers.
   * real numbers are capitalized, and are the space we're trying to work in
   *
   * op1: input. fixed: 15.15.b00
   * X = op1 / 2^17                          # in radians
   * TAU = 6.28318530718...                  # 2pi
   * QTAU = TAU/4
   *
   * Z = (X / QTAU) % 4                      # the dimensionless circle fraction
   *
   * circle_frac = Z * 2^28                  # will fit in 30 bits, 2 for extra
   *
   * big_op = op1 << 32
   * BIG_OP = X * 2^32
   *
   * big_qtau = floor(QTAU * 2^(17+32-32+4)) # remove 32-4 bits so that big_op /
   *          = floor(QTAU * 2^21)           # big_tau has 28-bits of fraction and
   *                                         # 2 bits of integer
   *
   * circle_frac = big_op / big_qtau
   *   = X * 2^32 / floor(QTAU * 2^21)
   *  ~= X * 2^11 / QTAU
   *   = (X / QTAU) * 2^11
   *  ~= (op1 / QTAU / 2^17) * 2^11
   *   = (op1 / QTAU) * 2^28                # in [0,4), fills 30 bits at 2.28
   *
   * Z' =    2 - Z       { if 1 <= z < 3
   *         Z           { otherwise
   *
   * zp =                                   # bits: xx.2.28
   *         (1<<31) - circle_frac { if 1 <= circle_frac[29:28] < 3
   *         circle_frac           { otherwise
   *
   */

  uint64_t big_op = ((uint64_t) DATA_BITS(op1)) << 32;
  uint32_t big_tau = 0x3243f6;  // in python: "0x%x"%(math.floor((math.pi / 2) * 2**21))
  uint32_t circle_frac = (big_op / big_tau) & 0x3fffffff;
  uint32_t top_bits_differ = ((circle_frac >> 28) & 0x1) ^ ((circle_frac >> 29) & 0x1);
  uint32_t zp = MASK_UNLESS(top_bits_differ, (1<<29) - circle_frac) |
                MASK_UNLESS(!top_bits_differ, SIGN_EXTEND(circle_frac, 30));

#define MUL_TOP_32(op1, op2) ((uint32_t) ((((int64_t) ((int32_t) op1) ) * ((int64_t) ((int32_t) op2) )) >> (32-4)) & 0xffffffff)

  uint32_t zp2 = MUL_TOP_32(zp, zp);

  uint32_t a = 0x64764525; // a = 1.569718634; "0x%08x"%(a*2**30)"
  uint32_t b = 0x28ec8a4a; // "0x%08x"%((2*a - (5/2.)) *2**30)
  uint32_t c = 0x04764525; // "0x%08x"%((a - (3/2.)) *2**30)

  uint32_t result =
    MUL_TOP_32(zp,
        (a - MUL_TOP_32(zp2,
                          b - MUL_TOP_32(c, zp2))));

  // result is xx.2.28, shift over into fixed, sign extend to full result
  return DATA_BITS(SIGN_EXTEND( result >> (30 - n_frac_bits - n_flag_bits), (32 - (30 - n_frac_bits - n_flag_bits ) )));

#undef MUL_TOP_32
}

// TODO: not constant time. will work for now.
void fix_print(char* buffer, fixed f) {
  double d;
  fixed f_ = f;

  if(FIX_IS_NAN(f)) {
    memcpy(buffer, "NaN", 4);
    return;
  }
  if(FIX_IS_INF_POS(f)) {
    memcpy(buffer, "+Inf", 5);
    return;
  }
  if(FIX_IS_INF_NEG(f)) {
    memcpy(buffer, "-Inf", 5);
    return;
  }

  uint8_t neg = !!TOP_BIT(f);

  if(neg) {
    f_ = ~f_ + 4;
  }

  d = f_ >> 17;
  d += ((f_ >> 2) & 0x7fff) / (float) (1<<15);

  if(neg) {
    d *= -1;
  }

  sprintf(buffer, "%f", d);
}

fixed fix_convert_double(double d) {
  uint64_t bits = *(uint64_t*) &d;
  uint32_t exponent = ((bits >> 52) & 0x7ff) - 1023;
  uint32_t sign = bits >> 63;

  /* note that this breaks with denorm numbers. However, we'll shift those all
   * away with the exponent later */
  uint64_t mantissa = (bits & ((1ull <<52)-1)) | (d != 0 ? (1ull<<52) : 0);
  uint32_t shift = 52 - (n_frac_bits) - exponent;

  fixed result = ((ROUND_TO_EVEN(mantissa,shift)) << n_flag_bits) & 0xffffffff;

  /* TODO: make fixed-time */
  if(isnan(d)) {
    result = F_NAN;
  }

  /* TODO: make fixed-time */
  if( isinf(d) || ((mantissa >> shift) & ~((1ull << (n_frac_bits + n_int_bits)) -1)) != 0) {
    result = F_INF_POS;
  }

  fixed result_neg = fix_neg(result);

  return (sign == 0 ? result : result_neg);
}

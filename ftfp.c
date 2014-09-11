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

  fixed tempresult,op2nz;

  isnan = FIX_IS_NAN(op1) | FIX_IS_NAN(op2) | (op2 == 0);

  // Take advantage of the extra bits we get out from doing this in uint64_t
  //TODO: Does not handle negative numbers correctly
  // op2 is never allowed to be 0, if it is set it to something like 1 so div doesn't fall over
  op2nz = op2 | (DATA_BITS(op2) == 0);
  tmp = ((((int64_t)((int32_t)DATA_BITS(op1)))<<15) /
         (int64_t)((int32_t)(op2nz)))<<2;

  tempresult = tmp & 0xFFFFFFFC;

  isinfop1 = (FIX_IS_INF_NEG(op1) | FIX_IS_INF_POS(op1));
  isinfop2 = (FIX_IS_INF_NEG(op2) | FIX_IS_INF_POS(op2));
  isnegop1 = FIX_IS_INF_NEG(op1) | (FIX_IS_NEG(op1) & !isinfop1);
  isnegop2 = FIX_IS_INF_NEG(op2) | (FIX_IS_NEG(op2) & !isinfop2);

  //Update isinf
  isinf = (isinfop1 | isinfop2) & (!isnan);

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
  tmp = ((int64_t)((int32_t)op1) * (int64_t)((int32_t)op2)) >> 17;

  // inf only if overflow, and not a sign thing
  tmp2 = tmp & 0xFFFFFFFF00000000;
  isinf = !((tmp2 == 0xFFFFFFFF00000000) | (tmp2 == 0));

  // TODO, this needs some rounding

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

  //TODO: One of the INFs needs to 'win' if we get -inf and inf.
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

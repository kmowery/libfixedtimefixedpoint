#include "ftfp.h"
#include "internal.h"
#include "lut.h"

#include "cordic.h"

// Contains the cordic trig functions for libftfp.

fixed fix_sin(fixed op1) {
  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1);
  uint8_t isnan = FIX_IS_NAN(op1);

  fix_internal Z = fix_circle_frac(op1);
  fix_internal C = CORDIC_P;
  fix_internal S = 0;

  // The circle fraction is in [0,4).
  // Move it to [-1, 1], where cordic will work for sin
  fix_internal top_bits_differ = ((Z >>  FIX_INTERN_FRAC_BITS   ) & 0x1) ^
                                 ((Z >> (FIX_INTERN_FRAC_BITS+1)) & 0x1);
  Z = MASK_UNLESS( top_bits_differ, (((fix_internal) 2)<<(FIX_INTERN_FRAC_BITS)) - Z) |
      MASK_UNLESS(!top_bits_differ, SIGN_EXTEND(Z, FIX_INTERN_FRAC_BITS+2));

  cordic(&Z, &C, &S);

  return FIX_IF_NAN(isnan | isinfpos | isinfneg) |
    FIX_INTERN_TO_FIXED(S);
}

fixed fix_cos(fixed op1) {
  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1);
  uint8_t isnan = FIX_IS_NAN(op1);

  fix_internal circle_frac = fix_circle_frac(op1);

  /* flip up into Q1 and Q2 */
  uint8_t Q3or4 = !!((((fix_internal) 2) << FIX_INTERN_FRAC_BITS) & circle_frac);
  circle_frac = MASK_UNLESS( Q3or4, (((fix_internal) 4)<< FIX_INTERN_FRAC_BITS) - circle_frac) |
                MASK_UNLESS(!Q3or4, circle_frac);

  /* Switch from cos on an angle in Q1 or Q2 to sin in Q4 or Q1.
   * This necessitates flipping the angle from [0,2] to [1, -1].
   */
  circle_frac = (((fix_internal) 1) << (FIX_INTERN_FRAC_BITS)) - circle_frac;

  fix_internal Z = circle_frac;
  fix_internal C = CORDIC_P;
  fix_internal S = 0;

  cordic(&Z, &C, &S);

  return FIX_IF_NAN(isnan | isinfpos | isinfneg) |
    FIX_INTERN_TO_FIXED(S);
}

fixed fix_tan(fixed op1) {
  // We will return NaN if you pass in infinity, but we might return infinity
  // anyway...
  uint8_t isinfpos = 0;
  uint8_t isinfneg = 0;
  uint8_t isnan = FIX_IS_NAN(op1) | FIX_IS_INF_POS(op1) | FIX_IS_INF_NEG(op1);

  /* The circle fraction is in [0,4). The cordic algorithm can handle [-1, 1],
   * and tan has rotational symmetry at z = 1.
   *
   * If we're in Q2 or 3, subtract 2 from the circle frac.
   */

  fix_internal circle_frac = fix_circle_frac(op1);

  fix_internal top_bits_differ = ((circle_frac >>  FIX_INTERN_FRAC_BITS   ) & 0x1) ^
                                 ((circle_frac >> (FIX_INTERN_FRAC_BITS+1)) & 0x1);
  fix_internal Z = MASK_UNLESS( top_bits_differ, circle_frac -
                                     (((fix_internal) 1) << (FIX_INTERN_FRAC_BITS+1))) |
                   MASK_UNLESS(!top_bits_differ, SIGN_EXTEND(circle_frac, FIX_INTERN_FRAC_BITS+2));

  fix_internal C = CORDIC_P;
  fix_internal S = 0;

  cordic(&Z, &C, &S);

  uint8_t isinf = 0;
  fix_internal result = fix_div_var(S, C, &isinf);

  isinfpos |= !!(isinf | (C==0)) & !FIX_IS_NEG(S);
  isinfneg |= !!(isinf | (C==0)) &  FIX_IS_NEG(S);

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
    FIX_DATA_BITS(ROUND_TO_EVEN_SIGNED(result, FIX_FLAG_BITS) << FIX_FLAG_BITS);
}


#include "ftfp.h"
#include "internal.h"

// Contains the cordic trig functions for libftfp.

// "%x"%(0.607252935 * 2**28)
#define CORDIC_P 0x09b74eda

void cordic(uint32_t* Zext, uint32_t* Cext, uint32_t* Sext) {
  /* See http://math.exeter.edu/rparris/peanut/cordic.pdf for the best
   * explanation of CORDIC I've found.
   */

  /* Use circle fractions instead of angles. Will be [0,4) in 2.28 format. */
  /* Use 2.28 notation for angles and constants. */

  /* Generate the cordic angles in terms of circle fractions:
   * ", ".join(["0x%08x"%((math.atan((1/2.)**i) / (math.pi/2)*2**28)) for i in range(0,24)])
   */
  uint32_t A[] = {
    0x08000000, 0x04b90147, 0x027ece16, 0x01444475, 0x00a2c350, 0x005175f8,
    0x0028bd87, 0x00145f15, 0x000a2f94, 0x000517cb, 0x00028be6, 0x000145f3,
    0x0000a2f9, 0x0000517c, 0x000028be, 0x0000145f, 0x00000a2f, 0x00000517,
    0x0000028b, 0x00000145, 0x000000a2, 0x00000051, 0x00000028, 0x00000014
  };

  uint32_t Z = *Zext;
  uint32_t C = *Cext;
  uint32_t S = *Sext;

  uint32_t C_ = 0;
  uint32_t S_ = 0;
  uint32_t pow2 = 0;

  /* D should be 1 if Z is positive, or -1 if Z is negative. */
  uint32_t D = SIGN_EX_SHIFT_RIGHT_32(Z, 31) | 1;

  for(int m = 0; m < 24; m++) {
    pow2 = 2 << (28 - 1 - m);

    /* generate the m+1th values of Z, C, S, and D */
    Z = Z - D * A[m];

    C_ = C - D*MUL_2x28(pow2, S);
    S_ = S + D*MUL_2x28(pow2, C);

    C = C_;
    S = S_;
    D = SIGN_EX_SHIFT_RIGHT_32(Z, 31) | 1;
  }

  *Zext = Z;
  *Cext = C;
  *Sext = S;

  return;

}

fixed fix_sin(fixed op1) {
  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1);
  uint8_t isnan = FIX_IS_NAN(op1);

  uint32_t Z = fix_circle_frac(op1);
  uint32_t C = CORDIC_P;
  uint32_t S = 0;

  // The circle fraction is in [0,4).
  // Move it to [-1, 1], where cordic will work for sin
  uint32_t top_bits_differ = ((Z >> 28) & 0x1) ^ ((Z >> 29) & 0x1);
  Z = MASK_UNLESS(top_bits_differ, (1<<29) - Z) |
      MASK_UNLESS(!top_bits_differ, SIGN_EXTEND(Z, 30));

  cordic(&Z, &C, &S);

  return FIX_IF_NAN(isnan | isinfpos | isinfneg) |
    convert_228_to_fixed_signed(S);
}

fixed fix_cos(fixed op1) {
  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1);
  uint8_t isnan = FIX_IS_NAN(op1);

  uint32_t circle_frac = fix_circle_frac(op1);

  /* flip up into Q1 and Q2 */
  uint8_t Q3or4 = !!((2 << 28) & circle_frac);
  circle_frac = MASK_UNLESS( Q3or4, (4<<28) - circle_frac) |
                MASK_UNLESS(!Q3or4, circle_frac);

  /* Switch from cos on an angle in Q1 or Q2 to sin in Q4 or Q1.
   * This necessitates flipping the angle from [0,2] to [1, -1].
   */
  circle_frac = (1 << (28)) - circle_frac;

  uint32_t Z = circle_frac;
  uint32_t C = CORDIC_P;
  uint32_t S = 0;

  cordic(&Z, &C, &S);

  return FIX_IF_NAN(isnan | isinfpos | isinfneg) |
    convert_228_to_fixed_signed(S);
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

  uint32_t circle_frac = fix_circle_frac(op1);

  uint32_t top_bits_differ = ((circle_frac >> 28) & 0x1) ^ ((circle_frac >> 29) & 0x1);
  uint32_t Z = MASK_UNLESS( top_bits_differ, circle_frac - (1<<29)) |
               MASK_UNLESS(!top_bits_differ, SIGN_EXTEND(circle_frac, 30));

  uint32_t C = CORDIC_P;
  uint32_t S = 0;

  cordic(&Z, &C, &S);

  uint8_t isinf = 0;
  uint64_t result = FIX_DIV_32(S, C, isinf);

  isinfpos |= !!(isinf | (C==0)) & !FIX_IS_NEG(S);
  isinfneg |= !!(isinf | (C==0)) & FIX_IS_NEG(S);

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
    FIX_DATA_BITS(ROUND_TO_EVEN_SIGNED(result, 2) << 2);
}


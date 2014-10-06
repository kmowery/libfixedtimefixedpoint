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

fixed fix_abs(fixed op1){
  uint8_t isinfpos;
  uint8_t isinfneg;
  uint8_t isnan;

  isinfpos = FIX_IS_INF_POS(op1);
  isinfneg = FIX_IS_INF_NEG(op1);
  isnan = FIX_IS_NAN(op1);

  fixed tempresult = MASK_UNLESS(TOP_BIT(~op1), op1) |
    MASK_UNLESS(  TOP_BIT(op1), DATA_BITS(((~op1) + 4)));

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS((isinfpos | isinfneg) & (!isnan)) |
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

  // do some horrible bit-ops to make result into what we want

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
    DATA_BITS(tempresult);
}

#define MUL_2x28(op1, op2) ((uint32_t) ((((int64_t) ((int32_t) (op1)) ) * ((int64_t) ((int32_t) (op2)) )) >> (32-4)) & 0xffffffff)

fixed fix_ln(fixed op1) {
  /* Approach taken from http://eesite.bitbucket.org/html/software/log_app/log_app.html */

  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1) | (op1 == 0);
  uint8_t isnan = FIX_IS_NAN(op1) | FIX_IS_NEG(op1);

  uint32_t scratch = op1;
  uint32_t log2; // compute (int) log2(op1)  (as a uint32_t, not fixed)
  uint32_t shift;

  log2 =  (scratch > 0xFFFF) << 4; scratch >>= log2;
  shift = (scratch >   0xFF) << 3; scratch >>= shift; log2 |= shift;
  shift = (scratch >    0xF) << 2; scratch >>= shift; log2 |= shift;
  shift = (scratch >    0x3) << 1; scratch >>= shift; log2 |= shift;
  log2 |= (scratch >> 1);
  //log2 is now log2(op1), considered as a uint32_t

  uint32_t top2mask = (3 << (log2 - 1));
  uint8_t top2set = ((op1 & top2mask) ^ top2mask) == 0;

  // we need to move op1 into [-0.5, 0.5] in xx.2.28
  //
  // first, let's move to [0.5, 1.5] in xx.2.28...
  uint32_t m = MASK_UNLESS(log2 <= 28, op1 << (28 - (log2 + top2set))) |
    MASK_UNLESS(log2 > 28, op1 >> (log2 + top2set - 28));

  // and then shift down by '1'. (1.28 bits of zero)
  m -= (1 << 28);

  fixed ln2 = 0x000162e4; // python: "0x%08x"%(math.log(2) * 2**17)
  fixed nln2 = ln2 * (log2 - n_frac_bits - n_flag_bits); // correct for nonsense
    // we want this to go negative for numbers < 1.

  // now, calculate ln(1+m):
  uint32_t c411933 = 0x0697470e; // "0x%08x"%(0.411933 * 2**28)
  uint32_t c574785 = 0x093251c1; // "0x%08x"%(0.574785 * 2**28)
  uint32_t c994946 = 0x0feb4c7f; // "0x%08x"%(0.994946 * 2**28)
  uint32_t c0022683 = 0x00094a7c; // "0x%08x"%(0.0022683 * 2**28)

  // (((0.411x - 0.57)x + 0.99)x + 0.0022..

  uint32_t tempresult =
    (MUL_2x28(m,
        MUL_2x28(m,
          MUL_2x28(m,
            c411933)
          - c574785)
        + c994946)
      + c0022683);

  tempresult = SIGN_EX_SHIFT_RIGHT_32(tempresult, 28 - n_frac_bits - n_flag_bits);
  tempresult += nln2 - 0x128; // adjustment constant for when log should be 0

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos) |
    FIX_IF_INF_NEG(isinfneg) |
    DATA_BITS(tempresult);
}



fixed fix_log2(fixed op1) {
  /* Approach taken from http://eesite.bitbucket.org/html/software/log_app/log_app.html */

  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1) | (op1 == 0);
  uint8_t isnan = FIX_IS_NAN(op1) | FIX_IS_NEG(op1);

  uint32_t scratch = op1;
  uint32_t log2; // compute (int) log2(op1)  (as a uint32_t, not fixed)
  uint32_t shift;

  log2 =  (scratch > 0xFFFF) << 4; scratch >>= log2;
  shift = (scratch >   0xFF) << 3; scratch >>= shift; log2 |= shift;
  shift = (scratch >    0xF) << 2; scratch >>= shift; log2 |= shift;
  shift = (scratch >    0x3) << 1; scratch >>= shift; log2 |= shift;
  log2 |= (scratch >> 1);
  //log2 is now log2(op1), considered as a uint32_t

  uint32_t top2mask = (3 << (log2 - 1));
  uint8_t top2set = ((op1 & top2mask) ^ top2mask) == 0;

  // we need to move op1 into [-0.5, 0.5] in xx.2.28
  //
  // first, let's move to [0.5, 1.5] in xx.2.28...
  uint32_t m = MASK_UNLESS(log2 <= 28, op1 << (28 - (log2 + top2set))) |
    MASK_UNLESS(log2 > 28, op1 >> (log2 + top2set - 28));

  // and then shift down by '1'. (1.28 bits of zero)
  m -= (1 << 28);

  fixed n = (log2 - n_frac_bits - n_flag_bits) << (n_frac_bits + n_flag_bits);

  // octave:31> x = -0.5:1/10000:0.5;
  // octave:32> polyfit( x, log2(x+1), 3)
  // ans =

  //   0.5777570  -0.8114606   1.4371765   0.0023697

  // now, calculate log2(1+m):
  //
  uint32_t c5777570 = 0x093e7e1f; // "0x%08x"%(0.5777570 * 2**28)
  uint32_t c8114606 = 0x0cfbbe1c; // "0x%08x"%(0.8114606 * 2**28)
  uint32_t c14371765 = 0x16feacc9; // "0x%08x"%(1.4371765 * 2**28)
  uint32_t c0023697 = 0x0009b4cf; // "0x%08x"%(0.0023697 * 2**28)

  // (((0.577x - 0.811)x + 1.43)x + 0.0023..

  uint32_t tempresult =
    (MUL_2x28(m,
        MUL_2x28(m,
          MUL_2x28(m,
            c5777570)
          - c8114606)
        + c14371765)
      + c0023697);

  tempresult = SIGN_EX_SHIFT_RIGHT_32(tempresult, 28 - n_frac_bits - n_flag_bits);
  tempresult += n - 0x134; // adjustment constant for when log should be 0

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos) |
    FIX_IF_INF_NEG(isinfneg) |
    DATA_BITS(tempresult);
}

fixed fix_sqrt(fixed op1) {
  // We're going to use Newton's Method with a fixed number of iterations.
  // The polynomial to use is:
  //
  //     f(x)  = x^2 - op1
  //     f'(x) = 2x
  //
  // Each update cycle is then:
  //
  //     x' = x - f(x) / f'(x)

  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = 0;
  uint8_t isnan = FIX_IS_NAN(op1) | FIX_IS_NEG(op1);

  // We need an initial guess. Let's use log_2(op1), since that's fairly quick
  // and easy, and not horribly wrong.
  uint32_t scratch = op1;
  uint32_t log2; // compute (int) log2(op1)  (as a uint32_t, not fixed)
  uint32_t shift;

  log2 =  (scratch > 0xFFFF) << 4; scratch >>= log2;
  shift = (scratch >   0xFF) << 3; scratch >>= shift; log2 |= shift;
  shift = (scratch >    0xF) << 2; scratch >>= shift; log2 |= shift;
  shift = (scratch >    0x3) << 1; scratch >>= shift; log2 |= shift;
  log2 |= (scratch >> 1);
  //log2 is now log2(op1), considered as a uint32_t

  // Make a guess! Use log2(op1) if op1 > 2, otherwise just uhhhh mul op1 by 2.
  int64_t x = MASK_UNLESS(op1 >= (1<<(n_frac_bits + n_flag_bits+1)),
                           FIXINT(log2 - (n_flag_bits + n_frac_bits))) |
               MASK_UNLESS(op1 < (1<<(n_frac_bits + n_flag_bits+1)),
                           op1 << 1);


  // We're going to do all math in a 47.17 uint64_t
  //int64_t x = op1;
  //int64_t x = 1<< 19;

  uint64_t op1neg = FIX_SIGN_TO_64(fix_neg(op1));

  //printf("\n\n");
  //d("op1", op1);
  //d("guess", x);
  //d("op1neg", op1neg);

  // we're going to use 15.17 fixed point numbers to do the calculation, and
  // then mask off our flag bits later
  for(int i = 0; i < 10; i++) {
    //printf("\n");
    int64_t x2 = (x * x) >> 17;
    int64_t t = x2 + op1neg;

    //d("x         ", x);
    //d("x2        ", x2);
    //d("f(x)      ", t);
    //d("f'(x)     ", x<<1);

    x = x | (x==0); // don't divide by zero...

    // t = t / f'(x) = t / 2x
    t = ROUND_TO_EVEN((t<<22) / (x<<1), 5);

    //d("f(x)/f'(x)", t);

    x = x - t;

    //d("x'        ", x);
  }

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos) |
    FIX_IF_INF_NEG(isinfneg) |
    DATA_BITS(x & 0xffffffff);
}

fixed fix_sin(fixed op1) {
  uint8_t isinfpos;
  uint8_t isinfneg;
  uint8_t isnan;

  isinfpos = FIX_IS_INF_POS(op1);
  isinfneg = FIX_IS_INF_NEG(op1);
  isnan = FIX_IS_NAN(op1);

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

  int64_t big_op = ((int64_t) ((int32_t) DATA_BITS(op1))) << 32;
  int32_t big_tau = 0x3243f6;  // in python: "0x%x"%(math.floor((math.pi / 2) * 2**21))
  int32_t circle_frac = (big_op / big_tau) & 0x3fffffff;
  uint32_t top_bits_differ = ((circle_frac >> 28) & 0x1) ^ ((circle_frac >> 29) & 0x1);
  uint32_t zp = MASK_UNLESS(top_bits_differ, (1<<29) - circle_frac) |
                MASK_UNLESS(!top_bits_differ, SIGN_EXTEND(circle_frac, 30));

  uint32_t zp2 = MUL_2x28(zp, zp);

  uint32_t a = 0x64764525; // a = 1.569718634; "0x%08x"%(a*2**30)"
  uint32_t b = 0x28ec8a4a; // "0x%08x"%((2*a - (5/2.)) *2**30)
  uint32_t c = 0x04764525; // "0x%08x"%((a - (3/2.)) *2**30)

  uint32_t result =
    MUL_2x28(zp,
        (a - MUL_2x28(zp2,
                          b - MUL_2x28(c, zp2))));

  // result is xx.2.28, shift over into fixed, sign extend to full result
  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    FIX_IF_INF_NEG(isinfneg & (!isnan)) |
    DATA_BITS(SIGN_EXTEND(
          result >> (30 - n_frac_bits - n_flag_bits),
          (32 - (30 - n_frac_bits - n_flag_bits ) /* 19 */ )));

#undef MUL_2x28
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

  uint8_t isinf = (isinf(d) || ((mantissa >> shift) & ~((1ull << (n_frac_bits + n_int_bits)) -1)) != 0);
  uint8_t isinfpos = (d > 0) & isinf;
  uint8_t isinfneg = (d < 0) & isinf;
  uint8_t isnan = isnan(d);

  fixed result_neg = fix_neg(result);

  return
    FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos) |
    FIX_IF_INF_NEG(isinfneg) |
    MASK_UNLESS(sign == 0, DATA_BITS(result)) |
    MASK_UNLESS(sign, DATA_BITS(result_neg));
}

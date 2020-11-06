#include "ftfp.h"
#include "internal.h"
#include "lut.h"

// Contains the logarithmic, exponential, and square root functions for libftfp.

fixed fix_exp(fixed op1) {

  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1);
  uint8_t isnan    = FIX_IS_NAN(op1);

  uint8_t isneg    = FIX_IS_NEG(op1);

  uint8_t log2     = fixed_log2(op1);
  uint8_t log2_neg = fixed_log2((~op1) + 4);

  uint8_t actuallog = MASK_UNLESS(!isneg, log2) | MASK_UNLESS( isneg, log2_neg);

  INT_INV_LUT;

  /* If the number is < 2, then move it directly to the fix_internal format.
   * Otherwise, map it to (-2, 2) in fix_internal. */
  fix_internal scratch =
#if FIX_INTERN_FRAC_BITS >= FIX_POINT_BITS
      MASK_UNLESS(actuallog <= FIX_POINT_BITS,
              op1 << (FIX_INTERN_FRAC_BITS - FIX_POINT_BITS)) |
      MASK_UNLESS((actuallog > FIX_POINT_BITS) & (actuallog <= FIX_INTERN_FRAC_BITS),
              op1 << (FIX_INTERN_FRAC_BITS - actuallog)) |
      MASK_UNLESS(actuallog > FIX_INTERN_FRAC_BITS,
              SIGN_EX_SHIFT_RIGHT(op1, (actuallog - FIX_INTERN_FRAC_BITS)));
#else
      MASK_UNLESS(actuallog <= FIX_POINT_BITS,
              SIGN_EX_SHIFT_RIGHT(op1, (FIX_POINT_BITS - FIX_INTERN_FRAC_BITS ))) |
      MASK_UNLESS(actuallog > FIX_POINT_BITS,
              SIGN_EX_SHIFT_RIGHT(op1, (actuallog - FIX_INTERN_FRAC_BITS)));
#endif

  /* Since we mapped the number down, we'll need to square the result later.
   * Note that we don't need to or/mask in 0. */
  uint8_t squarings =
      /*MASK_UNLESS(actuallog <= FIX_POINT_BITS, 0 ) |*/
      MASK_UNLESS(actuallog > FIX_POINT_BITS, actuallog - FIX_POINT_BITS );

  /*
   * Use the Taylor series:
   *
   *              n=inf
   *               ---
   *               \      x^n
   *     e^x   =    >    -----
   *               /      n !
   *               ---
   *               n=1
   *
   * As an optimization, you can generate the next term from the previous term:
   *
   *   T(n) = T(n-1) * x / n
   *
   * We keep around a LUT of values of 1/n, and can simply multiply instead of divide:
   *
   *   T(n) = T(n-1) * x * INV_LUT(n)
   *
   * Choosing the number of terms is tricky: since we have a variable number of
   * fractional bits, we compute when the terms will become zero. That is:
   *
   *   Find n s.t.:
   *
   *      2^n
   *     -----   <   2 ^ (-FIX_FRAC_BITS)
   *      n !
   *
   * By summing the log_2( x/n ), you can pick a value for the internal
   * representation:
   *
   * Since we chose this such that the last round adds nothing, we are
   * guaranteed bit-accurate taylor series approximation (at least in a fix_internal).
   */

#define FIX_EXP_LOOP 25

  /* To generate the table of fractional bits vs. loop iterations:
   *
   * l = 0.
   * for i in range(1,40):
   *   l += math.log(x/i,2)
   *   print "#elif FIX_FRAC_BITS < %d"%(abs(l)-1)
   *   print "  #define FIX_EXP_LOOP %d"%(i)
   *   if l < -61:
   *       break
   */

  fix_internal e_x = 1ull << FIX_INTERN_FRAC_BITS;
  fix_internal term = 1ull << FIX_INTERN_FRAC_BITS;
  uint8_t overflow = 0;

  for(int i = 1; i < FIX_EXP_LOOP; i ++) {
    term = FIX_MUL_INTERN(term, scratch, overflow);
    term = FIX_MUL_INTERN(term, LUT_int_inv_integer[i], overflow);

    e_x += term;
  }

  isinfpos |= overflow;

  fix_internal result = e_x;

  // x is in the range [-2^FIX_INT_BITS, 2^FIX_INT_BITS], and we
  // mapped it to [-2, 2]. We need one squaring for each halving, which means
  // that squarings can be at most log2(2^FIX_INT_BITS)-2 or
  // log2(2^FIX_FRAC_BITS)-2.
  //
  // (We need to worry about frac bits because negative numbers will map to <= -1,
  // which produces 0.367, which might then need to multiply itself out of
  // existence.)
  //
  // But that's overzealous: If x is positive, e^x must fit in 2^FIX_INT_BITS,
  // or we will return FIX_INF_POS.
  //
  // If we reduced the number before the approximation (as opposed to leaving it
  // alone), then x was greater than or equal to 2, and the reduction r will be
  // >= 1. In this case, the approximation of e^r will produce at least e^1, or
  // ~2.718. This requires only ceil(log2(ln(2^FIX_INT_BITS))+1) successive
  // doublings before it will overflow the fixed.
  //
  // If x is negative, we need s squarings so that 0.367 will square itself
  // to < 2^FIX_FRAC_BITS. By the same argument, we need
  // ceil(log2(ln(2**-FIX_FRAC_BITS))) squarings.
  //
  // In python:
  //
  // pos_squarings = [(n, math.ceil(math.log(math.log(2**n),2))) for n in range(1,93)]
  // neg_squarings = [(n, math.ceil(math.log(abs(math.log(2**(-(63-n)))),2))) for n in range(1,61)]
  // squarings = [(x[0], max(x[1], y[1]) if y is not None else x[1])
  //              for x,y in itertools.izip_longest(pos_squarings, neg_squarings)]
  // for k, g in itertools.groupby(squarings, operator.itemgetter(1)):
  //     int_bits = list(g)
  //     print "#elif FIX_INT_BITS <= %d"%( max([x for x,y in int_bits]) )
  //     print "#define FIX_SQUARE_LOOP %d"%(k)
  //

  // These numbers can be smaller for a 32-bit exp.

#if FIX_INT_BITS <= 16
#define FIX_SQUARE_LOOP 6
#elif FIX_INT_BITS <= 46
#define FIX_SQUARE_LOOP 5
#elif FIX_INT_BITS <= 92
#define FIX_SQUARE_LOOP 6
#else
#error Unknown number of FIX_INT_BITS in fix_exp
#endif

  /* We need to square the result a few times. To do this as accurately as
   * possible, we'd like to keep as many significant bits as we can. To this
   * end, we build our own one-off floating point format.
   *
   * First, result is some number in [e^-2, e^2], which means it's positive.
   *
   * Take the floor(log2(result)), and save that as the "int bits" i. Then, in
   * rshift, shift result such that its MSB is in bit 63. Treated as a 0.64
   * fixed point value, this number is in [0.5, 1).
   *
   *   resultl         = rshift * 2^i
   *
   * To square things, we do:
   *
   *   result * result = rshift * rshift * 2^i * 2^i
   *   result * result = rshift * rshift * 2^(i+i)
   *
   * Note though, that rshift is in [0.25, 1). We want to keep as many
   * significant bits as possible. Therefore, if rshift * rshift < 0.5, shift it
   * up one and subtract off one int bit.
   *
   *   r2shift * 2^i' = rshift * rshift * 2^(2i)
   *
   *   r2shift = rshift * rshift             if rshift * rshift >= 0.5
   *   r2shift = rshift * rshift * 2          otherwise.
   *
   *   i' = 2i                               if rshift * rshift >= 0.5
   *   i' = 2i - 1                           if rshift * rshift < 0.5
   *
   * Error calculations:
   *
   *  rshift begins as a exp result, accurate to 2^FIX_INTERN_FRAC_BITS, or
   *  2^60.
   *
   *  Each time we square the number, we retain 64 significant bits, and lose 64
   *  less significant bits. This causes an error at each stage of <= X * 2^-64.
   *  The total error, after n squarings, can then be bounded by:
   *
   *    E = ( ( (rshift * (1+2^-60))^2 * (1+2^-64) )^2 ...)^2
   *
   *    E = (rshift)^(2^n) * (1 + 2^-60)^(2^n) * (1 + 2^-64)^(2^n - 1)
   *
   *  With the maximum of 6 squarings, this gives a maximum error of n * 2^-53.9,
   *  for almost 54 bits of accuracy.
   *
   */

  int32_t rlog = fixed_log2(result);
  fix_internal rshift = (result) << (63 - rlog);
  int32_t rint_bits = rlog - FIX_INTERN_FRAC_BITS +1; // +1 is for the sign bit we're not using

  fix_internal r2shift = 0;

  for(int i = 0; i < FIX_SQUARE_LOOP; i++) {

    r2shift = MUL_64_TOP(rshift, rshift);

    // r2shift will represent a number between [0.25, 1) in 0.64 fixed. Therefore, it _might_
    // have a zero top bit. If it does, take it off.

    rshift = MASK_UNLESS(squarings >  0, r2shift << !(FIX_TOP_BIT(r2shift))) |
             MASK_UNLESS(squarings == 0, rshift);

    rint_bits = rint_bits + MASK_UNLESS(squarings > 0, rint_bits - !FIX_TOP_BIT(r2shift));

    squarings = MASK_UNLESS(squarings > 0, squarings-1);
  }

  // If this is positive, we've overflowed the 64-bit range.
  // If this is zero, we've overflowed the sign bit.
  int32_t shift = rint_bits - FIX_INT_BITS;

  // This round will work if FIX_FLAG_BITS >= 2.
  fixed final_result = MASK_UNLESS(shift > (-64 + FIX_FLAG_BITS),
        ROUND_TO_EVEN(rshift, ((-shift) + FIX_FLAG_BITS)) << FIX_FLAG_BITS);

  isinfpos |= ((shift >= 0) & (!isinfneg));

  // note that we want to return 0 if op1 is FIX_INF_NEG...
  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    MASK_UNLESS(!isinfneg, FIX_DATA_BITS(final_result));
}

  // We don't want to use a inline function here to avoid pointers
  // compute (int) log2(op1)  (as a uint32_t, not fixed)

  // We need to figure out how to map op1 into [-.5, .5], to use our polynomial
  // approxmation. First, we'll map op1 into [0.5, 1.5].
  //
  // We'll look at the top 2 bits of the number. If they're both 1, then we'll
  // move it to be just above 0.5. In that case, though, we need to increment
  // the log2 by 1.

  // we need to move op1 into [-0.5, 0.5] in xx.2.28
  //
  // first, let's move to [0.5, 1.5] in xx.2.28...

  // and then shift down by '1'. (1.28 bits of zero)
#define FIX_LOG_PROLOG(op1, log2, m) \
  uint32_t log2 = fixed_log2(op1); \
  fixed top2mask = (((fixed) 3) << (log2 - 1)); \
  uint8_t top2set = ((op1 & top2mask) ^ top2mask) == 0; \
  log2 += top2set; \
  fix_internal m = \
    MASK_UNLESS(log2 <= FIX_INTERN_FRAC_BITS, op1 << (FIX_INTERN_FRAC_BITS - (log2))) | \
    MASK_UNLESS(log2 >  FIX_INTERN_FRAC_BITS, op1 >> (log2 - FIX_INTERN_FRAC_BITS)); \
  m -= (((fix_internal) 1) << FIX_INTERN_FRAC_BITS);

fixed fix_ln(fixed op1) {
  /* Approach taken from http://eesite.bitbucket.org/html/software/log_app/log_app.html */

  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1) | (op1 == 0);
  uint8_t isnan = FIX_IS_NAN(op1) | FIX_IS_NEG(op1);

  FIX_LOG_PROLOG(op1, log2, m);

  // Python: "0x%016x"%((decimal.Decimal(2).ln() * 2**63)
  //     .quantize(decimal.Decimal('1.'), rounding=decimal.ROUND_HALF_EVEN))
  uint64_t ln2 = 0x58b90bfbe8e7bcd6;
  uint8_t overflow = 0;

  // this will go negative for numbers < 1.
#if 63 - FIX_POINT_BITS != 0
  fixed nln2 = MUL_64_N(ln2, ((int64_t) (log2)) - FIX_POINT_BITS, overflow, 63 - FIX_POINT_BITS);
#else
  fixed nln2 = MUL_64_ALL(ln2, ((int64_t) (log2)) - FIX_POINT_BITS, overflow);
#endif

  // now, calculate ln(1+m):

  // Use a order-24 polynomial over -5,5, accurate to about 2**-48:
  //
  // octave:85> format long
  // octave:86> p = polyfit( x, log(x+1), 24)
  // octave:87> log2(max(abs(polyval(p, x) - log(1+x))))
  // ans = -48.4454111483224


  // This works, but replicates work:
  //fix_internal tmp =
  //  (FIX_MUL_INTERN(m,
  //      FIX_MUL_INTERN(m,
  //        FIX_MUL_INTERN(m,
  //          FIX_LN_COEF_3, overflow)
  //        + FIX_LN_COEF_2, overflow)
  //      + FIX_LN_COEF_1, overflow)
  //    + FIX_LN_COEF_0);
  fix_internal tmp;

  tmp = FIX_MUL_INTERN(m,       FIX_LN_COEF_24, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_23, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_22, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_21, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_20, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_19, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_18, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_17, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_16, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_15, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_14, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_13, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_12, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_11, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_10, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_9, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_8, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_7, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_6, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_5, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_4, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_3, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_2, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LN_COEF_1, overflow);
  tmp =                   tmp + FIX_LN_COEF_0;

  fixed r = FIX_INTERN_TO_FIXED(tmp);
  r += nln2;

  isinfneg |= (!isnan) & (!isinfpos) & overflow;

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos) |
    FIX_IF_INF_NEG(isinfneg) |
    FIX_DATA_BITS(r);
}


fixed fix_log2(fixed op1) {
  /* Approach taken from http://eesite.bitbucket.org/html/software/log_app/log_app.html */

  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1) | (op1 == 0);
  uint8_t isnan = FIX_IS_NAN(op1) | FIX_IS_NEG(op1);

  FIX_LOG_PROLOG(op1, log2, m);

  uint8_t overflow = 0;

  // Check if we're going to overflow n
  fixed ntmp = (((fixed) (log2)) - FIX_POINT_BITS);
  fixed sign_mask = ~((((fixed) 1) << (64 - FIX_POINT_BITS - 1)) - 1);
  overflow |= ((ntmp & sign_mask) != 0) & ((ntmp & sign_mask) != sign_mask);

  fixed n = ntmp << FIX_POINT_BITS;

  // Use an order-25 polynomial to approximate log2(1+m) over -.5,5.
  // Accurate to about 2**-48.
  //
  // octave:88> p = polyfit( x, log2(x+1), 25)
  // octave:89> log2(max(abs(polyval(p, x) - log2(1+x))))
  // ans = -48.2995602818589

  // now, calculate log2(1+m):
  fix_internal tmp;

  tmp = FIX_MUL_INTERN(m,       FIX_LOG2_COEF_25, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_24, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_23, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_22, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_21, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_20, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_19, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_18, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_17, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_16, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_15, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_14, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_13, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_12, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_11, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_10, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_9, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_8, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_7, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_6, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_5, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_4, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_3, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_2, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG2_COEF_1, overflow);
  tmp =                   tmp + FIX_LOG2_COEF_0;

  fixed r = FIX_INTERN_TO_FIXED(tmp);
  r += n;

  isinfneg |= (!isnan) & (!isinfpos) & overflow;

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos) |
    FIX_IF_INF_NEG(isinfneg) |
    FIX_DATA_BITS(r);
}

fixed fix_log10(fixed op1) {
  /* Approach taken from http://eesite.bitbucket.org/html/software/log_app/log_app.html */

  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1) | (op1 == 0);
  uint8_t isnan = FIX_IS_NAN(op1) | FIX_IS_NEG(op1);

  FIX_LOG_PROLOG(op1, log2, m);

  // Python: "0x%016x"%((decimal.Decimal(2).ln() * 2**63)
  //     .quantize(decimal.Decimal('1.'), rounding=decimal.ROUND_HALF_EVEN))
  fixed log10_2 = 0x268826a13ef3fde6;
  uint8_t overflow = 0;

  // this will go negative for numbers < 1.
#if 63 - FIX_POINT_BITS != 0
  fixed nlog10_2 = MUL_64_N(log10_2, ((int64_t) (log2)) - FIX_POINT_BITS, overflow, 63 - FIX_POINT_BITS);
#else
  fixed nlog10_2 = MUL_64_ALL(log10_2, ((int64_t) (log2)) - FIX_POINT_BITS, overflow);
#endif

  // Use a 24-order polynomial to approximate log10 on -5,5. Accurate to about 2*-48.
  //
  // octave:80> p = polyfit( x, log10(x+1), 24)
  // octave:81> log2(max(abs(polyval(p, x) - log10(1+x))))
  // ans = -48.6780719051126

  fix_internal tmp;

  tmp = FIX_MUL_INTERN(m,       FIX_LOG10_COEF_24, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_23, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_22, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_21, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_20, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_19, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_18, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_17, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_16, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_15, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_14, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_13, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_12, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_11, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_10, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_9, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_8, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_7, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_6, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_5, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_4, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_3, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_2, overflow);
  tmp = FIX_MUL_INTERN(m, tmp + FIX_LOG10_COEF_1, overflow);
  tmp =                   tmp + FIX_LOG10_COEF_0;

  fixed r = FIX_INTERN_TO_FIXED(tmp);
  r += nlog10_2;

  isinfneg |= (!isnan) & (!isinfpos) & overflow;

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos) |
    FIX_IF_INF_NEG(isinfneg) |
    FIX_DATA_BITS(r);
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
  //
  //               x^2 - op1
  //     x' = x - -----------
  //                   2x
  //
  // Optimizing for fixed bit precision:
  //
  //               x     1   op1
  //     x' = x - --- + --- ----
  //               2     2    x
  //

  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = 0;
  uint8_t isnan = FIX_IS_NAN(op1) | FIX_IS_NEG(op1);


  // Make a guess! Use some constant times log2(op1) if op1 > 1, otherwise just uhhhh mul op1 by 2.
  // Ensure that x is never zero by masking in a One. If One can't exist, do
  // something else.
#if FIX_INT_BITS > 1
  // We need an initial guess. Let's use log_2(op1), since that's fairly quick
  // and easy, and not horribly wrong.
  //
  // We don't need to worry about negative numbers here, since this is sqrt
  uint32_t log2 = fixed_log2(op1);

  fixed one = FIXINT(1);
  fixed x =
    MASK_UNLESS(op1 >= one, (~FIX_TOP_BIT_MASK) & (one | (FIX_INT_BITS * FIXINT(log2 - FIX_POINT_BITS + 1)))) |
    MASK_UNLESS(op1 <  one, op1 << 1);
#else
  // substract one here to ensure we are nonzero, and mask back to zero if op1
  // is zero
  fixed x = MASK_UNLESS(op1 != 0, (~FIX_TOP_BIT_MASK) & ((op1 << 1) - (fixed) 1));
#endif

  // We're going to do all math in fixed, but use the extra flag bits for
  // precision. We'll mask them off later...

  uint8_t overflow = 0;

  for(int i = 0; i < 22; i++) {
    // Compute x/2
    fixed x2 = ROUND_TO_EVEN_ONE_BIT(x);

    // Compute op1 / x
    fixed op1x = fix_div_var(op1, x, &overflow);
    fixed op1x2 = ROUND_TO_EVEN_ONE_BIT(op1x);

    x = x - x2 + op1x2;
  }

#if FIX_INT_BITS > 1
  // Get rid of spare bits.
  x = ROUND_TO_EVEN(x, FIX_FLAG_BITS) << FIX_FLAG_BITS;

#else
  // If we only have one int bit, the square root result might be FIX_MAX. Check
  // for this and return FIX_MAX if the rounding goes bad.
  x = ROUND_TO_EVEN(x, FIX_FLAG_BITS) << FIX_FLAG_BITS;
  x = MASK_UNLESS(x == FIX_MIN, FIX_MAX) |
      MASK_UNLESS(x != FIX_MIN, x);

#endif

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos) |
    FIX_IF_INF_NEG(isinfneg) |
    FIX_DATA_BITS(x);
}

/* fix_pow: Computes x^y.
 *
 *  Uses the exponential method:
 *
 *        z = x^y
 *     ln z = y ln(x)
 *        z = e ^ (y ln(x))
 *
 *  Error analysis:
 *
 *    fix_ln  has an error of max(fix_epsilon, 0.000000000000004).
 *    fix_exp has an error of max(fix_epsilon, (1 + 2^-64)^63 * (1 + 2*-60)^64).
 *
 *  Therefore, to calculate the error, we take a worst-case scenario and
 *  subtract the real valued output:
 *
 *   error <= e^((ln(x) + max(fix_epsilon,  0.000000000000004)) * y) * fix_exp_error - e^(ln(x)y)
 *         <= e^(ln(x) * y) e^(max(fix_epsilon, fix_ln_error * abs(y))) * fix_exp_error - e^(ln(x)y)
 *         <= e^(ln(x) * y) * (e^(max(fix_epsilon, fix_ln_error * abs(y))) * fix_exp_error - 1)
 *
 *  Unfortunately, this means that error depends fairly heavily on the value of y.
 *
 *  This error will be calculated in significant binary digits.
 *
 *  To calculate the error for a fixed with F fracbits and y = ###Y###, in python:

   # import sympy
   # y,z = sympy.symbols("y z")
   # def make_poly(depth):
   #  return ((e**y) * sympy.sympify("1+z**-60")) if depth == 0 else \
   #    (make_poly(depth-1)**2 * (sympy.sympify("1+z**-64")))
   #
   # fracbits = sympy.symbols("f")
   # epsilon = Float("2", 100)**-(fracbits)
   # lnerr = sympy.Max(0.000000000000004, epsilon)
   # experr = sympy.Max(mak_poly(6), epsilon)
   #
   # pow_err = (experr * (sympy.E**sympy.Max(epsilon, lnerr * sympy.Abs(y))) * experr - 1)

   # for i in range(1,63):
   #  pow_err_est = pow_err.subs([(z,2)]).subs([(fracbits, i), (y,###Y###)]).evalf(50)
   #  print i, sympy.log(pow_err_est,2).evalf(60), pow_err_est

 */

/* We might be able to make this a little bit more precise by not casting to a
 * fixed each time... */

/* The complicated bits here are to deal with the case where you do x^y, but x
 * is negative and y is non-integer...
 */

fixed fix_pow(fixed x, fixed y) {
  uint8_t isnan = FIX_IS_NAN(x) | FIX_IS_NAN(y);

  uint8_t xisinfpos = FIX_IS_INF_POS(x);
  uint8_t yisinfpos = FIX_IS_INF_POS(y);
  uint8_t xisinfneg = FIX_IS_INF_NEG(x);
  uint8_t yisinfneg = FIX_IS_INF_NEG(y);

  uint8_t excep = isnan |
    xisinfpos | xisinfneg |
    yisinfpos | yisinfneg;

  /* Store x's sign, and then check if it's positive. */
  uint8_t xneg = FIX_IS_NEG(x);
  uint8_t yneg = FIX_IS_NEG(y);

  fixed one = FIXINT(1);
  fixed neg_one = FIXNUM(-1,0);

#if FIX_INT_BITS == 1
  fixed xorig = x;
  x = fix_abs(x);
  uint8_t xmagone  = (xorig == FIX_MIN);
  uint8_t xmagonel = (xorig != FIX_MIN);
  uint8_t xmagoneg = 0;
#else
  x = fix_abs(x);
  uint8_t xmagone  = fix_eq(x, one);
  uint8_t xmagonel = fix_lt(x, one);
  uint8_t xmagoneg = fix_gt(x, one);
#endif

  // To know if y is an integer, we need it to be positive.
  fixed yabs = fix_abs(y);
  uint8_t y_is_int = (yabs & FIX_FRAC_MASK) == 0;
  uint8_t y_int_mod_2 = ((yabs & FIX_INT_MASK) >> FIX_POINT_BITS) & 0x1;

  fixed lnx = fix_ln(x);
  fixed prod = fix_mul(lnx, y);
  fixed result = fix_exp(prod);

  uint8_t isinfpos = 0;
  uint8_t isinfneg = 0;
  uint8_t isone = 0;
  uint8_t iszero = 0;
  uint8_t isnegone = 0;

  uint8_t isresult = 0;


  /* if x > 0, then return the result.
   * if x < 0 and y is an integer, then return the result with the proper sign.
   * if x < 0 and y is not an int, then return NaN.
   */

  /**************************************************
   *
   * Special case table!
   *
   * R means some non-exceptional, non-zero number.
   *
   *    x             y        result
   *  --------------------------------
   *   NaN            -          NaN
   *    -            NaN         NaN
   *
   *   Inf           Inf         Inf
   *   Inf          -Inf         Inf
   *  -Inf           Inf        -Inf
   *  -Inf          -Inf        -Inf
   *
   *  -Inf           R>0, even   Inf
   *  -Inf           R>0, odd   -Inf
   *
   *   R>0           R>0         R^R
   *   R<0           R>0         R^R
   *
   *    0            R>0          0
   *
   *   R>0            0           1
   *    0             0           1
   *   R<0            0           1
   *
   *   R!=0          R<0, int    R^R *
   *   R<0           R<0, nonint NaN
   *
   *   R>1           Inf         Inf
   *    1            Inf          1
   *   R<1 & R >-1   Inf          0
   *    0            Inf          0
   *   -1            Inf         -1
   *   R<-1          Inf        -Inf
   *
   *   R>1          -Inf          0
   *    1           -Inf          1
   *   R<1 & R >0   -Inf         Inf
   *   R=0          -Inf          0
   *   R<0 ^ R >-1  -Inf        -Inf
   *   -1           -Inf         -1
   *   R<-1         -Inf          0
   */

  isnan     |= xneg & (!y_is_int);

  isinfpos  |= xisinfpos;
  isinfneg  |= xisinfneg;

  isresult  |= (!excep) & (x != FIX_ZERO) & (!yneg) & (y != FIX_ZERO);
  iszero    |= (!excep) & (x == FIX_ZERO) & (!yneg) & (y != FIX_ZERO);

  isone     |= (!excep) & (y == FIX_ZERO);

  isresult  |= (!excep) & (x != FIX_ZERO) & (yneg) & (y_is_int);

  isnan     |= (xneg) & (yneg) & (!y_is_int);

  isinfpos  |= (yisinfpos) & (!xneg) & (xmagoneg);
  isone     |= (yisinfpos) & (!xneg) & (xmagone );
  iszero    |= (yisinfpos) & (!xneg) & (xmagonel);
  iszero    |= (yisinfpos) & ( xneg) & (xmagonel);
  isnegone  |= (yisinfpos) & ( xneg) & (xmagone );
  isinfneg  |= (yisinfpos) & ( xneg) & (xmagoneg);

  iszero    |= (yisinfneg) & (!xneg) & (xmagoneg);
  isone     |= (yisinfneg) & (!xneg) & (xmagone );
  isinfpos  |= (yisinfneg) & (!xneg) & (xmagonel) & (x != FIX_ZERO);
  iszero    |= (yisinfneg) & (x != FIX_ZERO);
  isinfneg  |= (yisinfneg) & ( xneg) & (xmagonel) & (x != FIX_ZERO);
  isnegone  |= (yisinfneg) & ( xneg) & (xmagone );
  iszero    |= (yisinfneg) & ( xneg) & (xmagoneg);

  // The truth table is simple; it doesn't say when we need to flip the sign of
  // the result in the R^R case. Spell it out here...
  uint8_t invert_result = (xneg) & (y_is_int) & (y_int_mod_2 == 1);


  // If the result went to infinity...
  isinfpos |= (!excep) & ((FIX_IS_INF_POS(result) & (!invert_result)) |
                          (FIX_IS_INF_NEG(result) & ( invert_result)));

  isinfneg |= (!excep) & ((FIX_IS_INF_POS(result) & ( invert_result)) |
                          (FIX_IS_INF_NEG(result) & (!invert_result)));

  isnan |= (!excep) & FIX_IS_NAN(result);

  return FIX_IF_NAN(isnan)   |
    FIX_IF_INF_POS((!isnan) & isinfpos) |
    FIX_IF_INF_NEG((!isnan) & isinfneg) |
    FIX_DATA_BITS(
      MASK_UNLESS( (!isnan) & isone, one) |
      MASK_UNLESS( (!isnan) & iszero, FIX_ZERO) |  /* no-op, but it keeps the compiler happy */
      MASK_UNLESS( (!isnan) & isnegone, neg_one) |
      MASK_UNLESS( (!excep) & isresult & (!invert_result), result) |
      MASK_UNLESS( (!excep) & isresult & ( invert_result), fix_neg(result)));
}

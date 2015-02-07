#include "ftfp.h"
#include "internal.h"
#include "lut.h"

// Contains the logarithmic, exponential, and square root functions for libftfp.

fixed fix_exp(fixed op1) {

  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isnan    = FIX_IS_NAN(op1);

  uint8_t isneg    = FIX_IS_NEG(op1);

  uint8_t log2     = fixed_log2(op1);
  uint8_t log2_neg = fixed_log2((~op1) + 4);

  uint8_t actuallog = MASK_UNLESS(!isneg, log2) | MASK_UNLESS( isneg, log2_neg);

  /* If the number is < 2, then move it directly to the fix_internal format.
   * Otherwise, map it to (-2, 2) in fix_internal. */
  fix_internal scratch =
#if FIX_INTERN_FRAC_BITS >= FIX_POINT_BITS
      MASK_UNLESS(actuallog <= FIX_POINT_BITS,
              op1 << (FIX_INTERN_FRAC_BITS - FIX_POINT_BITS)) |
      MASK_UNLESS(actuallog > FIX_POINT_BITS & actuallog <= FIX_INTERN_FRAC_BITS,
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
   */

#define FIX_EXP_LOOP 26

  /* To generate the preceding table:
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

  fix_internal result = e_x;

  uint8_t inf = overflow;

  // x is approximately in the range [-2^FIX_INT_BITS, 2^FIX_INT_BITS], and we
  // maped it to [-2, 2]. We need one squaring for each halving, which means
  // that squarings can be at most log2(2^FIX_INT_BITS)-2 or
  // log2(2^FIX_FRAC_BITS)-2.
  //
  // (We need to worry about frac bits because negative numbers will to <= -1,
  // which produces 0.367, which might then need to multiply itself out of
  // existence.)
  //
  // But that's overzealous: If x is positive, e^x must fit in 2^FIX_INT_BITS.
  // If we reduced the number before the approximation (as opposed to leaving it
  // alone), then x was greater than or equal to 2, and the reduction r will be
  // >= 1. In this case, the approximation of e^r will produce at least e^1, or
  // ~2.718. This requires only ceil(log2(ln(2**FIX_INT_BITS))+1) successive
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

  /* We're going to be squaring the result a few times. This process will double
   * the number of integer bits in the number, and so we need to keep track of
   * how many fractional bits we still have.
   *
   * When we do this squaring, we want to keep all resulting integer bits and
   * crop off fractional bits. In 64-bit fixed, This is equivalent to keeping
   * the top 64 bits of the 128-bit multiplication result.
   *
   * I think we could improve accuracy slightly by computing the log_2 of
   * result, and counting integer bits better. For example, we shouldn't end up
   * with 16 integer bit if we're computing a fixed with 10 integer bits. For
   * now, though, this works well enough.
   */
  fix_internal r2;
  int8_t frac_bits_remaining = FIX_INTERN_FRAC_BITS;

  for(int i = 0; i < FIX_SQUARE_LOOP; i++) {
    inf = 0;

    r2 = MUL_64_N(result, result, inf, 62);

    result = MASK_UNLESS(squarings > 0, r2) |
             MASK_UNLESS(squarings == 0, result);
    frac_bits_remaining = frac_bits_remaining - MASK_UNLESS(squarings > 0, (62 - frac_bits_remaining));
    isinfpos |= MASK_UNLESS(squarings > 0, inf);

    squarings = MASK_UNLESS(squarings > 0, squarings-1);
  }

  int8_t shift = frac_bits_remaining - FIX_POINT_BITS;
  fixed final_result =
      MASK_UNLESS(shift <  0, result << (-(shift))) |
      MASK_UNLESS(shift >= 0, ROUND_TO_EVEN(result, (shift + FIX_FLAG_BITS)) << FIX_FLAG_BITS);

  // If we're shifted too far to the left, the result should be infinity
  // Check the sign bit as well.
  isinfpos |= (shift < -63);
  isinfpos |= MASK_UNLESS(shift < 0, ((final_result & (~FIX_TOP_BIT_MASK)) >> (-shift)) != result);
  isinfpos |= !!(FIX_TOP_BIT(final_result));

  // note that we want to return 0 if op1 is FIX_INF_NEG...
  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    MASK_UNLESS(!FIX_IS_INF_NEG(op1), FIX_DATA_BITS(final_result));
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

  //// now, calculate ln(1+m):

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

  tmp = FIX_MUL_INTERN(m,       FIX_LN_COEF_3, overflow);
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

  // octave:31> x = -0.5:1/10000:0.5;
  // octave:32> polyfit( x, log2(x+1), 3)
  // ans =

  //   0.5777570  -0.8114606   1.4371765   0.0023697

  // now, calculate log2(1+m):
  fix_internal tmp;

  tmp = FIX_MUL_INTERN(m,       FIX_LOG2_COEF_3, overflow);
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

  // A third-order or fourth-order approximation polynomial does very poorly on
  // log10. Use a 5th order approximation instead:

  // octave:31> x = -0.5:1/10000:0.5;
  // octave:32> polyfit( x, log10(x+1), 5)
  // ans =

  //    1.1942e-01  -1.3949e-01   1.4074e-01  -2.1438e-01   4.3441e-01  -3.4210e-05
  //
  // Accurate to within about 0.00016:
  // http://www.wolframalpha.com/input/?i=log%28x%2B1%29%2Flog%2810%29+-+%281.1942e-01+*+x%5E5+%2B+-1.3949e-01+*+x+%5E+4+%2B+1.4074e-01*x%5E3++%2B+-2.1438e-01*x%5E2+%2B+4.3441e-01*x+-3.4210e-05%29+over+%5B-.5%2C+.5%5D

  fix_internal tmp;
  tmp = FIX_MUL_INTERN(m,       FIX_LOG10_COEF_5, overflow);
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

  for(int i = 0; i < 19; i++) {
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
 * */

/* We might be able to make this a little bit more precise by not casting to a
 * fixed each time... */

/* The complicated bits here are to deal with the case where you do x^y, but x
 * is negative and y is non-integer... */
fixed fix_pow(fixed x, fixed y) {

  /* Store x's sign, and then check if it's positive. */
  uint8_t xneg = FIX_IS_NEG(x);
  x = fix_abs(x);

  // To know if y is an integer, we need it to be positive.
  fixed yabs = fix_abs(y);
  uint8_t y_is_int = (yabs & FIX_FRAC_MASK) == 0;
  uint8_t y_int_mod_2 = ((yabs & FIX_INT_MASK) >> FIX_POINT_BITS) & 0x1;

  fixed lnx = fix_ln(x);
  fixed prod = fix_mul(lnx, y);
  fixed result = fix_exp(prod);

  /* if x > 0, then return the result.
   * if x < 0 and y is an integer, then return the result with the proper sign.
   * if x < 0 and y is not an int, then return NaN.
   */
  result =
    MASK_UNLESS( !xneg, result ) |
    MASK_UNLESS( xneg & y_is_int,
        MASK_UNLESS(y_int_mod_2 == 0, result) |
        MASK_UNLESS(y_int_mod_2 == 1, fix_neg(result))) |
    MASK_UNLESS( xneg & !y_is_int, FIX_NAN);

  return result;
}

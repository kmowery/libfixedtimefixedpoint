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

  /* Our taylor series works best with small numbers. Move the number to [0,2). */
  fixed scratch =
    MASK_UNLESS(!isneg,
      MASK_UNLESS(log2 <= FIX_POINT_BITS, op1) |
      MASK_UNLESS(log2 >  FIX_POINT_BITS, op1 >> (log2 - FIX_POINT_BITS))
    ) |
    MASK_UNLESS( isneg,
      MASK_UNLESS(log2_neg <= FIX_POINT_BITS, op1) |
      MASK_UNLESS(log2_neg >  FIX_POINT_BITS, SIGN_EX_SHIFT_RIGHT(op1, (log2_neg - FIX_POINT_BITS)))
      );

  /* Since we squished the number, we'll need to square the result later. */
  uint8_t squarings =
    MASK_UNLESS(!isneg,
      MASK_UNLESS(log2 <= FIX_POINT_BITS, 0) |
      MASK_UNLESS(log2 >  FIX_POINT_BITS, (log2 - FIX_POINT_BITS))
    ) |
    MASK_UNLESS( isneg,
      MASK_UNLESS(log2_neg <= FIX_POINT_BITS, 0) |
      MASK_UNLESS(log2_neg >  FIX_POINT_BITS, (log2_neg - FIX_POINT_BITS))
    );

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
   * By summing the log_2( x/n ), you can generate the following table:
   */

#if FIX_FRAC_BITS < 2
  #define FIX_EXP_LOOP 6
#elif FIX_FRAC_BITS < 4
  #define FIX_EXP_LOOP 7
#elif FIX_FRAC_BITS < 6
  #define FIX_EXP_LOOP 8
#elif FIX_FRAC_BITS < 8
  #define FIX_EXP_LOOP 9
#elif FIX_FRAC_BITS < 10
  #define FIX_EXP_LOOP 10
#elif FIX_FRAC_BITS < 13
  #define FIX_EXP_LOOP 11
#elif FIX_FRAC_BITS < 15
  #define FIX_EXP_LOOP 12
#elif FIX_FRAC_BITS < 18
  #define FIX_EXP_LOOP 13
#elif FIX_FRAC_BITS < 21
  #define FIX_EXP_LOOP 14
#elif FIX_FRAC_BITS < 24
  #define FIX_EXP_LOOP 15
#elif FIX_FRAC_BITS < 27
  #define FIX_EXP_LOOP 16
#elif FIX_FRAC_BITS < 30
  #define FIX_EXP_LOOP 17
#elif FIX_FRAC_BITS < 33
  #define FIX_EXP_LOOP 18
#elif FIX_FRAC_BITS < 36
  #define FIX_EXP_LOOP 19
#elif FIX_FRAC_BITS < 40
  #define FIX_EXP_LOOP 20
#elif FIX_FRAC_BITS < 43
  #define FIX_EXP_LOOP 21
#elif FIX_FRAC_BITS < 46
  #define FIX_EXP_LOOP 22
#elif FIX_FRAC_BITS < 50
  #define FIX_EXP_LOOP 23
#elif FIX_FRAC_BITS < 54
  #define FIX_EXP_LOOP 24
#elif FIX_FRAC_BITS < 57
  #define FIX_EXP_LOOP 25
#elif FIX_FRAC_BITS < 61
  #define FIX_EXP_LOOP 26
#else
#error Unknown number of FIX_FRAC_BITS in fix_exp
#endif

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

  fixed e_x = FIXINT(1);
  fixed term = FIXINT(1);
  uint8_t overflow = 0;

  for(int i = 1; i < FIX_EXP_LOOP; i ++) {
    term = FIX_MUL(term, scratch, overflow);
    term = FIX_MUL(term, LUT_inv_integer[i], overflow);
    e_x += term;
  }

  fixed result = e_x;

  uint8_t inf;

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
  //  pos_squarings = [(n, math.ceil(math.log(math.log(2**n),2))) for n in range(1,93)]
  //  neg_squarings = [(n, math.ceil(math.log(abs(math.log(2**(-(62-n)))),2))) for n in range(1,61)]
  //  squarings = [(x[0], max(x[1], y[1]) if y is not None else x[1])
  //               for x,y in itertools.izip_longest(pos_squarings, neg_squarings)]
  //  for k, g in itertools.groupby(squarings, operator.itemgetter(1)):
  //      int_bits = list(g)
  //      print "#elif FIX_INT_BITS <= %d"%( max([x for x,y in int_bits]) )
  //      print "#define FIX_SQUARE_LOOP %d"%(k)
  //

  // These numbers can be smaller for a 32-bit exp.

#if FIX_INT_BITS <= 15
#define FIX_SQUARE_LOOP 6
#elif FIX_INT_BITS <= 46
#define FIX_SQUARE_LOOP 5
#elif FIX_INT_BITS <= 92
#define FIX_SQUARE_LOOP 6
#else
#error Unknown number of FIX_INT_BITS in fix_exp
#endif

  for(int i = 0; i < FIX_SQUARE_LOOP; i++) {
    inf = 0;
    fixed r2 = FIX_MUL(result, result, inf);
    result = MASK_UNLESS(squarings > 0, r2) |
             MASK_UNLESS(squarings == 0, result);
    isinfpos |= MASK_UNLESS(squarings > 0, inf);

    squarings = MASK_UNLESS(squarings > 0, squarings-1);
  }

  // note that we want to return 0 if op1 is FIX_INF_NEG...
  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos & (!isnan)) |
    MASK_UNLESS(!FIX_IS_INF_NEG(op1), FIX_DATA_BITS(result));
}

fixed fix_ln(fixed op1) {
  /* Approach taken from http://eesite.bitbucket.org/html/software/log_app/log_app.html */

  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1) | (op1 == 0);
  uint8_t isnan = FIX_IS_NAN(op1) | FIX_IS_NEG(op1);

  // compute (int) log2(op1)  (as a uint32_t, not fixed)
  uint32_t log2 = uint32_log2(op1);

  // We need to figure out how to map op1 into [-.5, .5], to use our polynomial
  // approxmation. First, we'll map op1 into [0.5, 1.5].
  //
  // We'll look at the top 2 bits of the number. If they're both 1, then we'll
  // move it to be just above 0.5. In that case, though, we need to increment
  // the log2 by 1.
  uint32_t top2mask = (3 << (log2 - 1));
  uint8_t top2set = ((op1 & top2mask) ^ top2mask) == 0;
  log2 += top2set;

  // we need to move op1 into [-0.5, 0.5] in xx.2.28
  //
  // first, let's move to [0.5, 1.5] in xx.2.28...
  uint32_t m = MASK_UNLESS(log2 <= 28, op1 << (28 - (log2))) |
    MASK_UNLESS(log2 > 28, op1 >> (log2 - 28));

  // and then shift down by '1'. (1.28 bits of zero)
  m -= (1 << 28);

  fixed ln2 = 0x000162e4; // python: "0x%08x"%(math.log(2) * 2**17)
  fixed nln2 = ln2 * (log2 - FIX_FRAC_BITS - FIX_FLAG_BITS); // correct for nonsense
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

  fixed r = convert_228_to_fixed(tempresult);
  r += nln2 - 0x128; // adjustment constant for when log should be 0

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

  // compute (int) log2(op1)  (as a uint32_t, not fixed)
  uint32_t log2 = uint32_log2(op1);

  // We need to figure out how to map op1 into [-.5, .5], to use our polynomial
  // approxmation. First, we'll map op1 into [0.5, 1.5].
  //
  // We'll look at the top 2 bits of the number. If they're both 1, then we'll
  // move it to be just above 0.5. In that case, though, we need to increment
  // the log2 by 1.
  uint32_t top2mask = (3 << (log2 - 1));
  uint8_t top2set = ((op1 & top2mask) ^ top2mask) == 0;
  log2 += top2set;

  // we need to move op1 into [-0.5, 0.5] in xx.2.28
  //
  // first, let's move to [0.5, 1.5] in xx.2.28...
  uint32_t m = MASK_UNLESS(log2 <= 28, op1 << (28 - (log2))) |
    MASK_UNLESS(log2 > 28, op1 >> (log2 - 28));

  // and then shift down by '1'. (1.28 bits of zero)
  m -= (1 << 28);

  fixed n = ((fixed) (log2 - FIX_FRAC_BITS - FIX_FLAG_BITS)) << (FIX_FRAC_BITS + FIX_FLAG_BITS);

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

  tempresult = convert_228_to_fixed(tempresult);
  tempresult += n - 0x134; // adjustment constant for when log should be 0

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos) |
    FIX_IF_INF_NEG(isinfneg) |
    FIX_DATA_BITS(tempresult);
}

fixed fix_log10(fixed op1) {
  /* Approach taken from http://eesite.bitbucket.org/html/software/log_app/log_app.html */

  uint8_t isinfpos = FIX_IS_INF_POS(op1);
  uint8_t isinfneg = FIX_IS_INF_NEG(op1) | (op1 == 0);
  uint8_t isnan = FIX_IS_NAN(op1) | FIX_IS_NEG(op1);

  // compute (int) log2(op1)  (as a uint32_t, not fixed)
  uint32_t log2 = uint32_log2(op1);

  uint32_t top2mask = (3 << (log2 - 1));
  uint8_t top2set = ((op1 & top2mask) ^ top2mask) == 0;
  log2 += top2set;

  // we need to move op1 into [-0.5, 0.5] in xx.2.28
  //
  // first, let's move to [0.5, 1.5] in xx.2.28...
  uint32_t m = MASK_UNLESS(log2 <= 28, op1 << (28 - (log2))) |
    MASK_UNLESS(log2 > 28, op1 >> (log2 - 28));

  // and then shift down by '1'. (1.28 bits of zero)
  m -= (1 << 28);

  fixed log10_2 = 0x00009a20; // python: "0x%08x"%(math.log(2,10) * 2**17)
  fixed nlog10_2 = log10_2 * (log2 - FIX_FRAC_BITS - FIX_FLAG_BITS); // correct for nonsense
    // we want this to go negative for numbers < 1.

  // A third-order or fourth-order approximation polynomial does very poorly on
  // log10. Use a 5th order approximation instead:

  // octave:31> x = -0.5:1/10000:0.5;
  // octave:32> polyfit( x, log10(x+1), 5)
  // ans =

  //    1.1942e-01  -1.3949e-01   1.4074e-01  -2.1438e-01   4.3441e-01  -3.4210e-05

  // now, calculate log10(1+m):
  //
  // constants:
  //
  // print "\n".join(["uint32_t k%d = 0x%08x;"%(5-i, num * 2**28) for
  // i,num in enumerate([abs(eval(x)) for x in re.split(" +", "1.1942e-01
  // -1.3949e-01   1.4074e-01  -2.1438e-01   4.3441e-01  -3.4210e-05" )])])
  //

  uint32_t k5 = 0x01e924f2;
  uint32_t k4 = 0x023b59dd;
  uint32_t k3 = 0x02407896;
  uint32_t k2 = 0x036e19b9;
  uint32_t k1 = 0x06f357e6;
  uint32_t k0 = 0x000023df;

  uint32_t tempresult =
    (MUL_2x28(m,
       MUL_2x28(m,
         MUL_2x28(m,
            MUL_2x28(m,
              MUL_2x28(m,
                k5)
              - k4)
            + k3)
          - k2)
        + k1)
       - k0);

  tempresult = convert_228_to_fixed(tempresult);
  tempresult += nlog10_2;

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos) |
    FIX_IF_INF_NEG(isinfneg) |
    FIX_DATA_BITS(tempresult);
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

  // Make a guess! Use log2(op1) if op1 > 1, otherwise just uhhhh mul op1 by 2.
  int64_t x = MASK_UNLESS(op1 >= (((fixed) 1)<<(FIX_FRAC_BITS + FIX_FLAG_BITS)),
                           FIXINT(log2 - (FIX_FLAG_BITS + FIX_FRAC_BITS))) |
               MASK_UNLESS(op1 < (((fixed) 1)<<(FIX_FRAC_BITS + FIX_FLAG_BITS)),
                           op1 << 1);


  // We're going to do all math in a 47.17 uint64_t

  uint64_t op1neg = FIX_SIGN_TO_64(fix_neg(op1));

  // we're going to use 15.17 fixed point numbers to do the calculation, and
  // then mask off our flag bits later
  for(int i = 0; i < 8; i++) {
    int64_t x2 = (x * x) >> 17;
    int64_t t = x2 + op1neg;

    x = x | (x==0); // don't divide by zero...

    // t = t / f'(x) = t / 2x
    t = ROUND_TO_EVEN((t<<22) / (x<<1), 5);

    x = x - t;
  }

  return FIX_IF_NAN(isnan) |
    FIX_IF_INF_POS(isinfpos) |
    FIX_IF_INF_NEG(isinfneg) |
    FIX_DATA_BITS(x & 0xffffffff);
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

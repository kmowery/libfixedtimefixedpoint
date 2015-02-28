#include "ftfp.h"
#include "internal.h"

inline uint8_t uint32_log2(uint32_t o) {
  uint32_t scratch = o;
  uint32_t log2;
  uint32_t shift;

  log2 =  (scratch > 0xFFFF) << 4; scratch >>= log2;
  shift = (scratch >   0xFF) << 3; scratch >>= shift; log2 |= shift;
  shift = (scratch >    0xF) << 2; scratch >>= shift; log2 |= shift;
  shift = (scratch >    0x3) << 1; scratch >>= shift; log2 |= shift;
  log2 |= (scratch >> 1);
  return log2;
}

inline uint8_t uint64_log2(uint64_t o) {
  uint64_t scratch = o;
  uint64_t log2;
  uint64_t shift;

  log2 =  (scratch > 0xFFFFFFFF) << 5; scratch >>= log2;
  shift = (scratch > 0xFFFF)     << 4; scratch >>= shift; log2 |= shift;
  shift = (scratch >   0xFF)     << 3; scratch >>= shift; log2 |= shift;
  shift = (scratch >    0xF)     << 2; scratch >>= shift; log2 |= shift;
  shift = (scratch >    0x3)     << 1; scratch >>= shift; log2 |= shift;
  log2 |= (scratch >> 1);
  return log2;
}

fix_internal fix_circle_frac(fixed op1) {
  /* Scratchpad to compute z:
   *
   * variables are lowercase, and considered as integers.
   * real numbers are capitalized, and are the space we're trying to work in
   *
   * op1: input. fixed.
   * X = op1 / 2^(frac_bits + flag_bits)     # in radians
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
   */

  uint8_t isneg     = FIX_IS_NEG(op1);
  uint8_t log2      = fixed_log2(op1);
  uint8_t log2_neg  = fixed_log2((~op1) + 4);
  uint8_t actuallog = MASK_UNLESS(!isneg, log2) | MASK_UNLESS( isneg, log2_neg);

  uint32_t shiftamt = (((sizeof(fixed) * 8) - 1) - (actuallog+1));
  fixed opmax = (op1 << shiftamt);

  uint8_t overflow = 0;

  fixed big_qtau = 0xc90fdaa22168c235;

  /*
   * We want to divide op1 by TAU/4 (i.e., Pi/2), and end up with a fix_internal
   * within [0,4).
   *
   * Ugh. Code copied from fix_div_64, and modified for our purposes.
   */

  uint8_t xpos =  !FIX_TOP_BIT(op1);
  uint64_t absx = MASK_UNLESS_64( xpos,   op1) |
                  MASK_UNLESS_64(!xpos, (~op1)+1 );
  uint8_t logx = uint64_log2(absx);

  /* We change the result by shifting these numbers up. Record the shift... */
  int8_t shift = logx + 1 - (FIX_POINT_BITS);

  uint64_t acc = absx << (62 - logx);
  uint64_t base = big_qtau;

  uint64_t result = 0;

  /* if absx is 0x80..0, then x was the largest negative number, and acc is
   * some nonsense. Fix that up... */
  acc = MASK_UNLESS_64( absx == 0x8000000000000000, absx >> 1 ) |
        MASK_UNLESS_64( absx != 0x8000000000000000, acc );

  // Now, perform long division: x / y
  for(int i = 63; i >= 0; i--) {

    // Pesudocode:
    //if((acc >= base) & (base != 0)) {
    //    acc -= base;
    //    result |= 1;
    //}

    uint8_t expression = (acc >= base) & (base != 0);
    acc = MASK_UNLESS( expression, acc - base) |
          MASK_UNLESS(!expression, acc);
    result = result | MASK_UNLESS( expression, 1);

    result = result << 1;
    base = base >> 1;
  }

  // result now has 64 bits of division result; we need to shift it into place
  // "Place" is a combination of FIX_POINT_BITS and 'shift', as computed above
  // Since we moved y to be slightly above x, result contains a number in Q64.
  //
  int64_t shiftamount = ((64 - FIX_INTERN_FRAC_BITS) - shift);
  uint64_t roundbits = (result) & ((1ull << shiftamount) -1);
  result = MASK_UNLESS(shiftamount < 64, (result >> shiftamount));

  // If we're supposed to shift the result to the left (or not at all), there's
  // overflow. Although, if x was the largest negative number, not shifting the
  // result is okay.
  //overflow = (shiftamount < 0) | ((shiftamount == 0) & (absx != 0x8000000000000000));

  //result |= !!roundbits;

  //result = ROUND_TO_EVEN(result, FIX_FLAG_BITS) << FIX_FLAG_BITS;

  //result = MASK_UNLESS(ypos == xpos, result) |
  //         MASK_UNLESS(ypos != xpos, fix_neg(result));

  //return FIX_DATA_BITS(result);


  result = result & ((((fix_internal) 4) << (FIX_INTERN_FRAC_BITS))-1);

  result = MASK_UNLESS( xpos, result) |
           MASK_UNLESS(!xpos, ((((fix_internal) 4) << (FIX_INTERN_FRAC_BITS)) - result));

  // Mask again, just in case result was 0 and x was negative... (can that even
  // happen?)
  result = result & ((((fix_internal) 4) << (FIX_INTERN_FRAC_BITS))-1);

  //internald64("resumod", result);
  return result;
}

/*
 * Take a string consisting solely of digits, and produce a 64-bit number which
 * corresponds to:
 *
 *   (atoi(str + padding) / 10**20) * 2^64
 *
 * where padding is "0" * (20 - strlen(str)).
 *
 * More preicsely, given "5", it will should produce 0.5 * 2^64, or
 *               0x8000000000000000.
 * Given "25",   0x4000000000000000.
 * Given "125",  0x2000000000000000.
 * Given "0625", 0x1000000000000000.
 * Given "99999999999999999999", 0xffffffffffffffff.
 *
 * And so on.
 *
 * The basic strategy is to use LUTs for the binary values of 0.1, 0.01, and so on,
 * and add these together as we examine each decimal digit.
 *
 * Not constant time, but that could be fixed.
 *
 * We want a result rounded to 64 bits, but we need more than 64 bits to make
 * that happen. Therefore, we're going to use two luts, one for the main value,
 * and one for the extra bits (for rounding).
 *
 * */
uint64_t fixfrac(char* frac) {
    /* To generate these luts:
     *
     * source_bits = [(2**64 / 10**i, (256*2**64 / 10**i)%256) for i in range(1,21)]
     * print "uint64_t pow10_LUT = { %s };"%(", ".join("0x%016x"%(x) for x,y in source_bits))
     * print "uint64_t pow10_extraLUT = { %s };"%(", ".join("0x%02x"%(y) for x,y in source_bits))
     */
    uint64_t pow10_LUT[20] = {
        0x1999999999999999, 0x028f5c28f5c28f5c, 0x004189374bc6a7ef,
        0x00068db8bac710cb, 0x0000a7c5ac471b47, 0x000010c6f7a0b5ed,
        0x000001ad7f29abca, 0x0000002af31dc461, 0x000000044b82fa09,
        0x000000006df37f67, 0x000000000afebff0, 0x0000000001197998,
        0x00000000001c25c2, 0x000000000002d093, 0x000000000000480e,
        0x0000000000000734, 0x00000000000000b8, 0x0000000000000012,
        0x0000000000000001, 0x0000000000000000,
    };

    uint64_t pow10_LUT_extra[20] = {
        0x99, 0x28, 0x9d, 0x29, 0x84, 0x8d, 0xf4, 0x18, 0xb5, 0x5e, 0xbc, 0x12,
        0x68, 0x70, 0xbe, 0xac, 0x77, 0x72, 0xd8, 0x2f
    };

    uint64_t result = 0;
    uint64_t extra = 0;

    for(int i = 0; i < 20; i++) {
        if(frac[i] == '\0') {
            break;
        }

        uint8_t digit = (frac[i] - (uint8_t) '0');

        result += ((uint64_t) digit) * pow10_LUT[i];
        extra  += ((uint64_t) digit) * pow10_LUT_extra[i];
    }

    // We're going to round result to even, using extra as the lower bits.
    // First, add the bits from extra that have wrapped over, and then reproduce
    // the ROUND_TO_EVEN macro, and then add that back into the result.

    result += (extra >> 8);
    extra = extra & 0xff;

    uint8_t lowerbit = result & 1;
    uint8_t top_extra = !!(extra & (1<<7));
    uint8_t bot_extra = !!(extra & ((1<<7)-1));

    result += (top_extra & bot_extra) | (lowerbit & top_extra);

    return result;
}

uint64_t fix_div_64(fixed x, fixed y, uint8_t* overflow) {
  uint8_t xpos =  !FIX_TOP_BIT(x);
  uint8_t ypos =  !FIX_TOP_BIT(y);

  uint64_t absx = MASK_UNLESS_64( xpos, x ) |
                  MASK_UNLESS_64(!xpos, (~x)+1 );
  uint64_t absy = MASK_UNLESS_64( ypos, y ) |
                  MASK_UNLESS_64(!ypos, (~y)+1 );

  uint8_t logx = uint64_log2(absx);
  uint8_t logy = uint64_log2(absy);

  /* We change the result by shifting these numbers up. Record the shift... */
  int8_t shift = logx - logy + 1;

  uint64_t acc = absx << (62 - logx);
  uint64_t base = absy << (63 - logy);

  uint64_t result = 0;

  /* if absx is 0x80..0, then x was the largest negative number, and acc is
   * some nonsense. Fix that up... */
  acc = MASK_UNLESS_64( absx == 0x8000000000000000, absx >> 1 ) |
        MASK_UNLESS_64( absx != 0x8000000000000000, acc );

  // Now, perform long division: x / y
  for(int i = 63; i >= 0; i--) {

    // Pesudocode:
    //if((acc >= base) & (base != 0)) {
    //    acc -= base;
    //    result |= 1;
    //}

    uint8_t expression = (acc >= base) & (base != 0);
    acc = MASK_UNLESS( expression, acc - base) |
          MASK_UNLESS(!expression, acc);
    result = result | MASK_UNLESS( expression, 1);

    result = result << 1;
    base = base >> 1;
  }

  // result now has 64 bits of division result; we need to shift it into place
  // "Place" is a combination of FIX_POINT_BITS and 'shift', as computed above
  // Since we moved y to be slightly above x, result contains a number in Q64.
  int64_t shiftamount = ((64 - FIX_POINT_BITS) - shift);
  uint64_t roundbits = (result) & ((1ull << shiftamount) -1);
  result = MASK_UNLESS(shiftamount < 64, (result >> shiftamount));

  // If we're supposed to shift the result to the left (or not at all), there's
  // overflow. Although, if x was the largest negative number, not shifting the
  // result is okay.
  *overflow = (shiftamount < 0) | ((shiftamount == 0) & (absx != 0x8000000000000000));

  result |= !!roundbits;

  result = ROUND_TO_EVEN(result, FIX_FLAG_BITS) << FIX_FLAG_BITS;

  result = MASK_UNLESS(ypos == xpos, result) |
           MASK_UNLESS(ypos != xpos, fix_neg(result));

  return FIX_DATA_BITS(result);
}

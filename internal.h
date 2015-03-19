#ifndef internal_h
#define internal_h

#include "base.h"

#include <stdint.h>

// This file contains things needed internally for libftfp, but that a library
// user should never need to see.

// If you'd like to compile the debug functions, enable this define
//#define DEBUG


#define FIX_INLINE static inline

fixed fix_neg(fixed op1);

typedef int64_t fixed_signed;

#define FIX_INT_MAX (((fixed) 1) << (FIX_INT_BITS-1))

///////////////////////////////////////
//  Useful Defines
///////////////////////////////////////

#define FIX_FLAGS_MASK ((fixed) 0x3)

#define FIX_POINT_BITS (FIX_FRAC_BITS + FIX_FLAG_BITS)

#define FIX_ALL_BIT_MASK (((fixed) 0) -1)
#define FIX_TOP_BIT_MASK ((fixed) 1 << (FIX_BITS-1))
#define FIX_TOP_BIT(f) ((f) & FIX_TOP_BIT_MASK)

#define FIX_FRAC_MASK (((((fixed) 1)<<(FIX_FRAC_BITS))-1) << (FIX_FLAG_BITS))
#define FIX_INT_MASK  (((((fixed) 1)<<(FIX_INT_BITS))-1) << ((FIX_FLAG_BITS) + (FIX_FRAC_BITS)))



#define FIX_SIGN_TO_64(f) ((int64_t)((int32_t)(f)))

#define SIGN_EXTEND_32(value, n_top_bit) SIGN_EXTEND_64(value, n_top_bit)
#define SIGN_EX_SHIFT_RIGHT_32(value, shift) SIGN_EXTEND_32( (value) >> (shift), 32 - (shift) )
#define MASK_UNLESS_32(expression, value) (SIGN_EXTEND_32(!!(expression), 1) & (value))

#define SIGN_EXTEND_64(value, n_top_bit) ({uint64_t SE_m__ = (1ull << ((n_top_bit)-1)); (((uint64_t) (value)) ^ SE_m__) - SE_m__;})
#define MASK_UNLESS_64(expression, value) (SIGN_EXTEND_64(!!(expression), 1) & (value))
#define SIGN_EX_SHIFT_RIGHT_64(value, shift) SIGN_EXTEND_64( (value) >> (shift), 64 - (shift) )


#define MASK_UNLESS MASK_UNLESS_64
#define SIGN_EXTEND SIGN_EXTEND_64
#define SIGN_EX_SHIFT_RIGHT SIGN_EX_SHIFT_RIGHT_64


#define FIX_ABS_64(x) \
  (MASK_UNLESS_64( !FIX_TOP_BIT(x), x) | \
   MASK_UNLESS_64(!!FIX_TOP_BIT(x),(~x)+1))


#define FIX_DATA_BIT_MASK (0xFFFFFFFFFFFFFFFCLL)
#define FIX_DATA_BITS(f) ((f) & ((fixed)FIX_DATA_BIT_MASK))

#define FIX_DATA_BITS_ROUNDED(f) (ROUND_TO_EVEN(f, FIX_FLAG_BITS) << FIX_FLAG_BITS)

#define FIX_IF_NAN(isnan) (((isnan) | ((isnan) << 1)) & FIX_NAN)
#define FIX_IF_INF_POS(isinfpos) (((isinfpos) | ((isinfpos) << 1)) & FIX_INF_POS)
#define FIX_IF_INF_NEG(isinfneg) (((isinfneg) | ((isinfneg) << 1)) & FIX_INF_NEG)

#define FIX_IS_NEG(f) ((FIX_TOP_BIT(f)) == (FIX_TOP_BIT_MASK))

#define FIX_IS_NAN(f) (((f)&FIX_FLAGS_MASK) == FIX_NAN)
#define FIX_IS_INF_POS(f) (((f)&FIX_FLAGS_MASK) == FIX_INF_POS)
#define FIX_IS_INF_NEG(f) (((f)&FIX_FLAGS_MASK) == FIX_INF_NEG)


/* Returns true if the numbers are equal (NaNs are always unequal.) */
#define FIX_EQ(op1, op2) ( \
    (!(FIX_IS_NAN(op1) | FIX_IS_NAN(op2))) &    \
    ((FIX_IS_INF_POS(op1) & FIX_IS_INF_POS(op2)) | \
    (FIX_IS_INF_NEG(op1) & FIX_IS_INF_NEG(op2)) | \
    ((op1) == (op2))))

/* Returns true if the numbers are equal (and also if they are both NaN) */
#define FIX_EQ_NAN(op1, op2) ( \
    (FIX_IS_NAN(op1) & FIX_IS_NAN(op2)) | \
    (FIX_IS_INF_POS(op1) & FIX_IS_INF_POS(op2)) | \
    (FIX_IS_INF_NEG(op1) & FIX_IS_INF_NEG(op2)) | \
    ((op1) == (op2)))



/* Lops off the rightmost n_shift_bits of value and rounds to an even value
 * (so 0.5 will round to 0, but 1.5 will round to 2)
 *
 * n_shift_bits must be >- 2.
 *
 * Round Truth table:
 *  Bit 0 1 2       Result
 *      0 0 0       0 0x0
 *      0 0 1       0 0x0
 *      0 1 0       0 0x0
 *      0 1 1       1 0x1
 *      1 0 0       0 0x1
 *      1 0 1       0 0x1
 *      1 1 0       1 0x2
 *      1 1 1       1 0x2
 * The repeated phrase
 *
 *    ((!!(value & ((1 << (n_shift_bits-1))-1))) << (n_shift_bits-1))
 *
 *  is there to compress all lower-order bits into bit 2 in the truth table
 * */

#define ROUND_TO_EVEN_ADDITION(lowbit, highroundbit, restroundbit) \
   !!( \
     (!!(lowbit) & !!(highroundbit)) | \
     ((highroundbit) & !!(restroundbit)))


#define ROUND_TO_EVEN(value, n_shift_bits) \
  ((((fixed) value) >> (n_shift_bits)) + \
   !!( \
     (!!((value) & (((fixed) 1) << ((n_shift_bits)-1))) & !!((value) & ((((fixed) 1) << ((n_shift_bits)-1))-1))) | \
     ((((value) >> ((n_shift_bits)-2)) & 0x6) == 0x6) \
   ))

#define ROUND_TO_EVEN_ONE_BIT(value) \
    ((((fixed) value) >> 1) + \
     ((value & 3) == 3))


FIX_INLINE uint64_t ROUND_TO_EVEN_64(uint64_t value, int n_shift_bits) {
    uint8_t lowbit = (value >> (n_shift_bits)) & 0x1;
    uint8_t highroundbit = (value >> ((n_shift_bits)-1)) & 0x1;
    uint64_t restroundbits = (value) & ((1ull << ((n_shift_bits)-1)) -1);
    return (value >> n_shift_bits) + ROUND_TO_EVEN_ADDITION(lowbit, highroundbit, restroundbits);
}

#define ROUND_TO_EVEN_SIGNED_32(value, n_shift_bits) \
  (SIGN_EX_SHIFT_RIGHT_32(value, n_shift_bits) + \
   !!( \
     (!!((value) & (1LL << ((n_shift_bits)-1))) & !!((value) & ((1LL << ((n_shift_bits)-1))-1))) | \
     ((((value) >> ((n_shift_bits)-2)) & 0x6) == 0x6) \
   ))

#define ROUND_TO_EVEN_SIGNED_64(value, n_shift_bits) \
  (SIGN_EX_SHIFT_RIGHT_64(value, n_shift_bits) + \
   !!( \
     (!!((value) & (1LL << ((n_shift_bits)-1))) & !!((value) & ((1LL << ((n_shift_bits)-1))-1))) | \
     ((((value) >> ((n_shift_bits)-2)) & 0x6) == 0x6) \
   ))

#define ROUND_TO_EVEN_ONE_BIT_SIGNED(value) \
    (((((fixed) value) >> 1) + \
     ((value & 3) == 3)) | \
     FIX_TOP_BIT(value))

/*
 * General idea:
 *   This creates the fractional portion of a fixed point, given a decimal
 *   fraction. It uses the formula:
 *
 *     "one" / 10^( ceil(log_10( frac )) ) * frac
 *
 *  Along with extra precision bits and rounding to get as close as possible. It
 *  actually looks more like, with rounding tacked on:
 *
 *   (((1<<(FIX_FRAC_BITS+15)) / ((int) pow(10, log_ceil))) * frac >> 15) << FIX_FLAG_BITS;
 *
 * Notes:
 *   This works fairly well, and should always give the fixed point that is
 *   closest to the decimal number .frac. That is, FIXFRAC(5) will give 0.5,
 *   etc. We use a horrible strlen preprocessor trick, so FIXFRAC(001) will
 *   give a fixed point close to 0.001.
 *
 *   I'd really like to remove the pow call; doing this in the preprocessor
 *   seems difficult.
 *
 *   To prevent octal assignment, we do some nonsense into frac_int.
 */
#define FIXFRAC(frac) ({uint64_t fixfracr = fixfrac( #frac ); \
        (ROUND_TO_EVEN_64( fixfracr, FIX_INT_BITS + FIX_FLAG_BITS) << FIX_FLAG_BITS);})

uint64_t fixfrac(char* frac);

/* NOTE: does not protect against numbers that are too large */
#define FIXINT(z) (((fixed_signed) z %FIX_INT_MAX)<<(FIX_POINT_BITS))

// Create a fixnum constant. Use:
//   fixed x = FIX(-3,14159);
//
//We must also check that we didn't accidentally roll over from POS_INF to FIX_MIN...
#define FIXNUM(i,frac) ({ \
        uint8_t neg = (((fixed_signed) (i)) < 0) | (#i[0] == '-'); \
        fixed fnfrac = FIXFRAC(frac); \
        uint8_t inf = (((fixed) (i)) >= FIX_INT_MAX) & \
                        ((((fixed) (i)) < (-((fixed_signed) FIX_INT_MAX))) | \
                        ((((fixed) (i)) == (-((fixed_signed) FIX_INT_MAX))) & (fnfrac != 0x0))); \
        fnfrac = MASK_UNLESS_64( neg , (~fnfrac) + 1 ) | \
                 MASK_UNLESS_64(!neg , fnfrac ); \
        /* if you do this on one line, the compiler complains about shifting
         * some bits off the edge of the world... */ \
        fixed_signed fnint = ((fixed_signed) (i)); \
        fixed f =  (fnint << FIX_POINT_BITS) + (fnfrac); \
    ( MASK_UNLESS_64( (inf & !neg) | (!!FIX_TOP_BIT(f) & !neg), FIX_INF_POS ) | \
      MASK_UNLESS_64( (inf &  neg)                          , FIX_INF_NEG ) | \
      MASK_UNLESS_64(!inf , f )); })

#define FIX_ROUND_BASE(op1, round_exp) ({ \
    uint8_t isinfpos = FIX_IS_INF_POS(op1); \
    uint8_t isinfneg = FIX_IS_INF_NEG(op1); \
    uint8_t isnan = FIX_IS_NAN(op1); \
    uint8_t ex = isinfpos | isinfneg | isnan; \
    fixed result_nosign = round_exp; \
    (MASK_UNLESS_64(isinfpos, INT64_MAX) | \
     MASK_UNLESS_64(isinfneg, INT64_MIN) | \
     MASK_UNLESS_64( !ex, result_nosign)); \
    })

/* Uses round to even semantics */
#define FIX_ROUND_INT64(op1) \
  FIX_ROUND_BASE(op1, ROUND_TO_EVEN_SIGNED_64(op1, FIX_POINT_BITS))

/* 0.5 rounds up always */
#define FIX_ROUND_UP_INT64(op1) \
  FIX_ROUND_BASE(op1, SIGN_EX_SHIFT_RIGHT_64(op1, FIX_POINT_BITS) + ((op1 >> (FIX_POINT_BITS-1)) & 0x1))

#define FIX_CEIL64(op1) \
  FIX_ROUND_BASE(op1, SIGN_EX_SHIFT_RIGHT_64(op1, FIX_POINT_BITS) + !!(op1 & FIX_FRAC_MASK))

#define FIX_FLOOR64(op1) \
  FIX_ROUND_BASE(op1, SIGN_EX_SHIFT_RIGHT_64(op1, FIX_POINT_BITS))


///////////////////////////////////////
//  Useful Arithmetic
///////////////////////////////////////

// ensure that we never divide by 0. Caller is responsible for checking.
#define FIX_UNSAFE_DIV_32(op1, op2) \
  (ROUND_TO_EVEN(((FIX_SIGN_TO_64(FIX_DATA_BITS(op1))<<32) / \
                   FIX_SIGN_TO_64((op2) | (FIX_DATA_BITS(op2) == 0))),FIX_POINT_BITS)<<2)

// Note that you will lose the bottom bit of op2 for overflow safety
// Shift op2 right by 2 to gain 2 extra overflow bits
#define FIX_DIV_32(op1, op2, overflow) \
  ({ \
    uint64_t fdiv32tmp = FIX_UNSAFE_DIV_32(op1, \
      SIGN_EX_SHIFT_RIGHT_32(op2, 1)); \
    uint64_t masked = fdiv32tmp & 0xFFFFFFFF00000000; \
    overflow |= !((masked == 0xFFFFFFFF00000000) | (masked == 0)); \
    (fdiv32tmp >> 1) & 0xffffffff; \
  })

// Sign extend it all, this will help us correctly catch overflow
#define FIX_UNSAFE_MUL_32(op1, op2) \
  (ROUND_TO_EVEN(FIX_SIGN_TO_64(op1) * FIX_SIGN_TO_64(op2),17))

#define FIX_MUL_32(op1, op2, overflow) \
  ({ \
    uint64_t tmp = FIX_UNSAFE_MUL_32(op1, op2); \
    /* inf only if overflow, and not a sign thing */ \
    overflow |= \
      !(((tmp & 0xFFFFFFFF80000000) == 0xFFFFFFFF80000000) \
       | ((tmp & 0xFFFFFFFF80000000) == 0)); \
    tmp; \
   })


/* Implement a simple unsigned 64x64 multiplication, and correct for negative
 * numbers. There might be a way to do it arithmetically, but haven't found
 * in...  */
#define UNSAFE_MUL_64_64_128(op1, op2, resultlow, resulthigh)                                        \
({                                                                                                   \
  uint64_t absx = MASK_UNLESS_64( ((op1)>>63), (~(op1)) + 1 ) |                                      \
                  MASK_UNLESS_64(!((op1)>>63), (op1));                                               \
  uint64_t absy = MASK_UNLESS_64( ((op2)>>63), (~(op2)) + 1 ) |                                      \
                  MASK_UNLESS_64(!((op2)>>63), (op2));                                               \
                                                                                                     \
  uint64_t xhigh = absx >> 32;                                                                       \
  uint64_t xlow = absx & 0xffffffff;                                                                 \
  uint64_t yhigh = absy >> 32;                                                                       \
  uint64_t ylow = absy & 0xffffffff;                                                                 \
                                                                                                     \
  uint64_t z0 = xlow * ylow;                                                                         \
  uint64_t z1 = xlow * yhigh;                                                                        \
  uint64_t z2 = xhigh * ylow;                                                                        \
  uint64_t z3 = xhigh * yhigh;                                                                       \
                                                                                                     \
  resultlow = (z0) + ((z1 & 0xffffffff) << 32) + ((z2 & 0xffffffff) << 32);                          \
  uint64_t carry = (((z0 >> 32) + (z1 & 0xffffffff) + (z2 & 0xffffffff)) & 0xffffffff00000000) >> 32;\
                                                                                                     \
  resulthigh = carry + z3 + ((z1 & 0xffffffff00000000) >> 32) + ((z2 & 0xffffffff00000000) >> 32);   \
                                                                                                     \
  uint8_t negresult = ( ((op1) >> 63) ^ ((op2) >> 63) );                                             \
  resultlow = MASK_UNLESS( negresult, (~resultlow) + 1 ) |                                           \
              MASK_UNLESS(!negresult, ( resultlow)     );                                            \
  resulthigh= MASK_UNLESS( negresult, (~resulthigh) + (resultlow==0) ) |                             \
              MASK_UNLESS(!negresult, ( resulthigh)     );                                           \
  0;                                                                                                 \
})

#define UNSAFE_UNSIGNED_MUL_64_64_128(op1, op2, resultlow, resulthigh)                                 \
({                                                                                                     \
  uint64_t xhigh = op1 >> 32;                                                                            \
  uint64_t xlow = op1 & 0xffffffff;                                                                      \
  uint64_t yhigh = op2 >> 32;                                                                            \
  uint64_t ylow = op2 & 0xffffffff;                                                                      \
                                                                                                       \
  uint64_t z0 = xlow * ylow;                                                                           \
  uint64_t z1 = xlow * yhigh;                                                                          \
  uint64_t z2 = xhigh * ylow;                                                                          \
  uint64_t z3 = xhigh * yhigh;                                                                         \
                                                                                                       \
  resultlow = (z0) + ((z1 & 0xffffffff) << 32) + ((z2 & 0xffffffff) << 32);                            \
  uint64_t carry = (((z0 >> 32) + (z1 & 0xffffffff) + (z2 & 0xffffffff)) & 0xffffffff00000000) >> 32;  \
                                                                                                       \
  resulthigh = carry + z3 + ((z1 & 0xffffffff00000000) >> 32) + ((z2 & 0xffffffff00000000) >> 32);     \
                                                                                                       \
  0;                                                                                                   \
})



/* We end up with FIX_INT_BITS of extra sign bit on the top of the multiplied
 * number, along with the sign bit that's already there. If they aren't all 0 or
 * 1, overflow has occured. Make a mask... */
#define FIX_MUL_CONST ((( 1ull << (FIX_INT_BITS+1) ) -1) << (FIX_POINT_BITS-1))

#define FIX_MUL_64_N(op1, op2, overflow, extra_bits) \
  ({ \
    uint64_t tmphigh; \
    uint64_t tmplow; \
    UNSAFE_MUL_64_64_128(op1, op2, tmplow, tmphigh); \
    uint64_t tmplow2 = ROUND_TO_EVEN_64(tmplow, extra_bits); \
    uint64_t tmp = tmplow2 + \
                 ((tmphigh) << (64 - (extra_bits))); \
    /* inf only if overflow, and not a sign thing */ \
    overflow |= \
      !(((tmphigh & FIX_MUL_CONST) == FIX_MUL_CONST) \
       | ((tmphigh & FIX_MUL_CONST) == 0)); \
    tmp; \
   })

/* Sometimes we need to multiply numbers without reference to a fixed. Use this.
 * Overflow is quite simple: are there any higher bits that we aren't giving
 * you? Check to see if the sign bits are equal, though.
 */
#define MUL_64_N(op1, op2, overflow, extra_bits) \
  ({ \
    uint64_t tmphigh; \
    uint64_t tmplow; \
    uint64_t signconst = (~1ull) & (~((1ull << (extra_bits -1)) - 1)); \
    UNSAFE_MUL_64_64_128(op1, op2, tmplow, tmphigh); \
    uint64_t tmplow2 = ROUND_TO_EVEN_64(tmplow, extra_bits); \
    uint64_t tmp = tmplow2 + \
                 ((tmphigh) << (64 - (extra_bits))); \
    /* inf only if overflow, and not a sign thing */ \
    overflow |= \
      !(((tmphigh & signconst) == signconst) \
       | ((tmphigh & signconst) == 0)); \
    tmp; \
   })

// This is the degenerate case of MUL_64_N, where you want the lower 64 bits
#define MUL_64_ALL(op1, op2, overflow) \
  ({ \
    uint64_t tmphigh; \
    uint64_t tmplow; \
    UNSAFE_MUL_64_64_128(op1, op2, tmplow, tmphigh); \
    uint64_t tmplow2 = tmplow; \
    uint64_t tmp = tmplow2; \
    /* inf only if overflow, and not a sign thing */ \
    overflow |= \
      (!!(tmphigh) & (tmphigh != -1)); \
    tmp; \
   })
//
// This is the degenerate case of MUL_64_N, where you want the top 64 bits.
// Can't overflow! Maybe don't give it signed things?
#define MUL_64_TOP(op1, op2) \
  ({ \
    uint64_t tmphigh; \
    uint64_t tmplow; \
    UNSAFE_UNSIGNED_MUL_64_64_128(op1, op2, tmplow, tmphigh); \
    uint64_t tmp = tmphigh + \
        ((ROUND_TO_EVEN_ADDITION(tmphigh & 1, tmplow & (1ull << 63), tmplow & ~(1ull << 63)))); \
    tmp; \
   })


#define FIX_MUL_64(op1, op2, overflow) \
    FIX_MUL_64_N(op1, op2, overflow, FIX_POINT_BITS)

#define FIX_MUL FIX_MUL_64

#define ROUND_TO_EVEN_SIGNED ROUND_TO_EVEN_SIGNED_64


/************************************************************************
 *
 * We're going to use an internal format for many of the apprixmations.
 *
 * We need to store signed numbers up to about 8, so for 64-bit builds let's use
 * a Q4.60 fixed point.
 *
 ************************************************************************/

#if FIX_INTERN_FRAC_BITS + FIX_INTERN_INT_BITS != 64
#error FIX_INTERN underspecified
#endif

typedef uint64_t fix_internal;

#define FIX_MUL_INTERN(op1, op2, overflow) \
    MUL_64_N(op1, op2, overflow, FIX_INTERN_FRAC_BITS)

// Define convert_internal_to_fixed
#if FIX_FRAC_BITS >= FIX_INTERN_FRAC_BITS
// shift left. We should probably be concerned about overflow here...
#define FIX_INTERN_TO_FIXED(intern) \
  FIX_DATA_BITS( ((fixed) (intern)) << (FIX_POINT_BITS - FIX_INTERN_FRAC_BITS) )

#elif FIX_FRAC_BITS == (FIX_INTERN_FRAC_BITS-1)
// need to shift right, and potentially round
#define FIX_INTERN_TO_FIXED(intern) \
  FIX_DATA_BITS( \
    ROUND_TO_EVEN_ONE_BIT_SIGNED( ((fixed) intern) ) << FIX_FLAG_BITS \
      )

#elif FIX_FRAC_BITS < (FIX_INTERN_FRAC_BITS-1)
#define FIX_INTERN_TO_FIXED(intern) \
  FIX_DATA_BITS( \
    ROUND_TO_EVEN_SIGNED_64( ((fixed) intern), FIX_INTERN_FRAC_BITS - FIX_FRAC_BITS ) << FIX_FLAG_BITS \
      )

#else
#error Problem with FIX_INTERN_TO_FIXED definition
#endif



///////////////////////////////////////
//  Helper functions
///////////////////////////////////////

#ifdef DEBUG
void fix_internal_print(char* buffer, fix_internal f);
void fix_allfrac_print(char* buffer, fix_internal f);
#endif

///////////////////////////////////////
// functions that should be inlined
///////////////////////////////////////

// Costs about 20 cycles
FIX_INLINE uint8_t uint64_log2(uint64_t o) {
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

FIX_INLINE uint8_t uint32_log2(uint32_t o) {
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
#define fixed_log2 uint64_log2

FIX_INLINE fix_internal fix_circle_frac(fixed op1) {
  fixed big_qtau = 0xc90fdaa22168c235; // "%x"%(mpmath.nint( (mpmath.pi / 2) * 2**63))

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
  int i = 0;

  /* if absx is 0x80..0, then x was the largest negative number, and acc is
   * some nonsense. Fix that up... */
  acc = MASK_UNLESS_64( absx == 0x8000000000000000, absx >> 1 ) |
        MASK_UNLESS_64( absx != 0x8000000000000000, acc );

  // Now, perform long division: x / y
  for(i = 63; i >= 0; i--) {

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
  result = MASK_UNLESS(shiftamount < 64, (result >> shiftamount));

  result = result & ((((fix_internal) 4) << (FIX_INTERN_FRAC_BITS))-1);
  result = MASK_UNLESS( xpos, result) |
           MASK_UNLESS(!xpos, ((((fix_internal) 4) << (FIX_INTERN_FRAC_BITS)) - result));
  result = result & ((((fix_internal) 4) << (FIX_INTERN_FRAC_BITS))-1);

  return result;
}



static inline uint64_t fix_div_64(fixed x, fixed y, uint8_t* overflow) {
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
  int i = 0;

  // Now, perform long division: x / y
  for(i = 63; i >= 0; i--) {

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

  result = FIX_DATA_BITS_ROUNDED(result);

  result = MASK_UNLESS(ypos == xpos, result) |
           MASK_UNLESS(ypos != xpos, fix_neg(result));

  return FIX_DATA_BITS(result);
}

#define fix_div_var fix_div_64

#endif

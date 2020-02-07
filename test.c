#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <math.h>

#include "ftfp.h"
#include "test_helper.h"

#include "debug.h"
#include "lut.h"

static void null_test_success(void **state) {
  //(void) state;
}

#define CHECK_CONDITION(error_msg, condition, var1, var2) \
  if( !(condition) ) { \
    char b1[FIX_PRINT_BUFFER_SIZE], b2[FIX_PRINT_BUFFER_SIZE]; \
    fix_sprint(b1, var1); fix_sprint(b2, var2); \
    fixed difference = fix_sub(var1, var2); \
    char b3[FIX_PRINT_BUFFER_SIZE]; fix_sprint(b3, difference); \
    fail_msg( error_msg ": \n%s ("FIX_PRINTF_HEX") != %s ("FIX_PRINTF_HEX \
        ")\n difference %s ("FIX_PRINTF_HEX", "FIX_PRINTF_HEX")", \
        b1, var1, b2, var2, b3, difference, ~difference+4); \
  }

#define CHECK_EQ(error_msg, var1, var2) \
    CHECK_CONDITION(error_msg, fix_eq(var1, var2), var1, var2)

#define CHECK_EQ_NAN(error_msg, var1, var2) \
    CHECK_CONDITION(error_msg, fix_eq_nan(var1, var2), var1, var2)

#define CHECK_EQ_VALUE(error_msg, var1, var2, value) \
    CHECK_CONDITION(error_msg, fix_eq(var1, var2) == value, var1, var2)

#define CHECK_EQ_NAN_VALUE(error_msg, var1, var2, value) \
    CHECK_CONDITION(error_msg, fix_eq_nan(var1, var2) == value, var1, var2)

#define CHECK_VALUE(error_msg, value, expected, fixed1, fixed2) \
    CHECK_CONDITION(error_msg, (value) == (expected), fixed1, fixed2)

#define CHECK_DIFFERENCE(error_msg, value, expected, bound) \
    CHECK_CONDITION(error_msg, (fix_eq_nan(value, expected) | \
          (!( FIX_IS_INF_POS(value) | FIX_IS_INF_NEG(value) | FIX_IS_NAN(value) \
            | FIX_IS_INF_POS(expected) | FIX_IS_INF_NEG(expected) | FIX_IS_NAN(expected)) \
          & (fix_abs(fix_sub(value, expected)) <= (bound)))), value, expected)

#define CHECK_INT_EQUAL(error_msg, var1, var2) \
  if( !(var1 == var2) ) { \
    fail_msg( error_msg ": "FIX_PRINTF_DEC" (0x"FIX_PRINTF_HEX") != "FIX_PRINTF_DEC" (0x"FIX_PRINTF_HEX")", (fixed) var1, (fixed) var1, (fixed) var2, (fixed) var2); \
  }

#define SQRT_MAX ((fixed) sqrt((double) FIX_INT_MAX))

void p(fixed f) {
  char buf[FIX_PRINT_BUFFER_SIZE];

  fix_sprint(buf, f);
  printf("n: %s ("FIX_PRINTF_HEX")\n", buf, f);
}
void pl(fixed f) {
  char buf[FIX_PRINT_BUFFER_SIZE];

  fix_sprint(buf, f);
  printf("%s ("FIX_PRINTF_HEX")", buf, f);
}

void bounds(fixed f) {
  char buf_less[FIX_PRINT_BUFFER_SIZE];
  char buf[FIX_PRINT_BUFFER_SIZE];
  char buf_more[FIX_PRINT_BUFFER_SIZE];

  fixed less = f - 0x4;
  fixed more = f + 0x4;

  fix_sprint(buf_less, less);
  fix_sprint(buf, f);
  fix_sprint(buf_more, more);

  printf("%s (0x"FIX_PRINTF_HEX") | %s (0x"FIX_PRINTF_HEX") | %s (0x"FIX_PRINTF_HEX")\n", buf_less, less, buf, f, buf_more, more);
}

// Helper macro. Define it to be this for now, for code generation
#define TEST_HELPER(name, code) static void name(void **state) code

//////////////////////////////////////////////////////////////////////////////

#define TEST_ROUND_TO_EVEN(name, value, shift, result) \
TEST_HELPER(round_to_even_##name, { \
  int32_t rounded = ROUND_TO_EVEN(value, shift); \
  int32_t expected = result; \
  CHECK_INT_EQUAL("round_to_even", rounded, expected); \
};)

#define ROUND_TO_EVEN_TESTS                                        \
TEST_ROUND_TO_EVEN(zero       , 0x00 , 0x3 , 0x0)                  \
TEST_ROUND_TO_EVEN(zero_plus  , 0x01 , 0x3 , 0x0)                  \
TEST_ROUND_TO_EVEN(one        , 0x02 , 0x3 , 0x0)                  \
TEST_ROUND_TO_EVEN(one_plus   , 0x03 , 0x3 , 0x0)                  \
TEST_ROUND_TO_EVEN(two        , 0x04 , 0x3 , 0x0)  /* 0.5 -> 0 */  \
TEST_ROUND_TO_EVEN(two_plus   , 0x05 , 0x3 , 0x1)  /* 0.55 -> 1 */ \
TEST_ROUND_TO_EVEN(three      , 0x06 , 0x3 , 0x1)                  \
TEST_ROUND_TO_EVEN(three_plus , 0x07 , 0x3 , 0x1)                  \
TEST_ROUND_TO_EVEN(four       , 0x08 , 0x3 , 0x1)  /* 1 -> 1 */    \
TEST_ROUND_TO_EVEN(four_plus  , 0x09 , 0x3 , 0x1)                  \
TEST_ROUND_TO_EVEN(five       , 0x0a , 0x3 , 0x1)                  \
TEST_ROUND_TO_EVEN(five_plus  , 0x0b , 0x3 , 0x1)                  \
TEST_ROUND_TO_EVEN(six        , 0x0c , 0x3 , 0x2)  /* 1.5 -> 2 */  \
TEST_ROUND_TO_EVEN(six_plus   , 0x0d , 0x3 , 0x2)                  \
TEST_ROUND_TO_EVEN(seven      , 0x0e , 0x3 , 0x2)                  \
TEST_ROUND_TO_EVEN(seven_plus , 0x0f , 0x3 , 0x2)                  \
TEST_ROUND_TO_EVEN(eight      , 0x10 , 0x3 , 0x2)                  \
TEST_ROUND_TO_EVEN(eight_plus , 0x10 , 0x3 , 0x2)
ROUND_TO_EVEN_TESTS

//////////////////////////////////////////////////////////////////////////////

#define TEST_FIXNUM(name, inputint, inputfrac, inf, outputsign, outputint, outputfrac) \
TEST_HELPER(fixnum_##name, { \
  fixed g = FIXNUM(inputint, inputfrac); \
  fixed expected = (((fixed) (outputint)) << FIX_POINT_BITS) + \
                   (ROUND_TO_EVEN_64(((fixed) outputfrac), \
                                     (FIX_BITS - FIX_FRAC_BITS)) << FIX_FLAG_BITS); \
  if(outputsign == 1) { \
        expected = FIX_DATA_BITS((~(fixed) expected) + 0x4); \
  } else { \
        expected = FIX_DATA_BITS((fixed) expected); \
  }\
  if(inf == FIX_INF_POS || (outputsign == 0 && outputint >= FIX_INT_MAX)) { \
    expected = FIX_INF_POS; \
  } \
  if(inf == FIX_INF_NEG || (outputsign == 1 && ((outputint > FIX_INT_MAX) || (outputint == FIX_INT_MAX && outputfrac != 0)))) { \
    expected = FIX_INF_NEG; \
  } \
  CHECK_EQ("fixnum", g, expected); \
};)

#define FIXNUM_TESTS                                                                                                        \
TEST_FIXNUM(zero            , 0              , 0                    , 0           , 0 , 0             , 0)                  \
TEST_FIXNUM(one             , 1              , 0                    , 0           , 0 , 1             , 0)                  \
TEST_FIXNUM(one_neg         , -1             , 0                    , 0           , 1 , 1             , 0)                  \
TEST_FIXNUM(two             , 2              , 0                    , 0           , 0 , 2             , 0)                  \
TEST_FIXNUM(two_neg         , -2             , 0                    , 0           , 1 , 2             , 0)                  \
TEST_FIXNUM(many            , 1000           , 4                    , 0           , 0 , 1000          , 0x6666666666666666) \
TEST_FIXNUM(many_neg        , -1000          , 4                    , 0           , 1 , 1000          , 0x6666666666666666) \
TEST_FIXNUM(max_int_l1      , FIX_INT_MAX-1  , 0                    , 0           , 0 , FIX_INT_MAX-1 , 0   )               \
TEST_FIXNUM(max_int         , FIX_INT_MAX    , 0                    , FIX_INF_POS , 0 , 0             , 0   )               \
TEST_FIXNUM(max_int_neg     , -FIX_INT_MAX   , 0                    , 0           , 1 , FIX_INT_MAX   , 0   )               \
TEST_FIXNUM(max_int_neg_plus, -FIX_INT_MAX   , 5                    , FIX_INF_NEG , 1 , 0             , 0   )               \
TEST_FIXNUM(max_int_neg_m1  , -FIX_INT_MAX-1 , 0                    , FIX_INF_NEG , 1 , 0             , 0   )               \
TEST_FIXNUM(frac            , 0              , 5342                 , 0           , 0 , 0             , 0x88c154c985f06f69) \
TEST_FIXNUM(frac_neg        , -0             , 5342                 , 0           , 1 , 0             , 0x88c154c985f06f69) \
TEST_FIXNUM(regress0        , 0              , 00932                , 0           , 0 , 0             , 0x0262cba732df505d) \
TEST_FIXNUM(regress1        , 100            , 002655               , 0           , 0 , 100           , 0x00adff822bbecaac) \
TEST_FIXNUM(regress12       , 100            , 0026550292968        , 0           , 0 , 100           , 0x00adffffffeae3ae) \
TEST_FIXNUM(regress2        , 1              , 4142150878906        , 0           , 0 , 1             , 0x6a09fffffff8f68f) \
TEST_FIXNUM(regress3        , 1              , 6487121582031        , 0           , 0 , 1             , 0xa611fffffff8f68f) \
TEST_FIXNUM(regress4        , 1              , 99999999999999999999 , 0           , 0 , 2             , 0x0)                \
TEST_FIXNUM(regress45       , 0              , 99999999999999999999 , 0           , 0 , 1             , 0x0)                \
TEST_FIXNUM(regress5        , -1             , 3862915039           , 0           , 1 , 1             , 0x62e3fffff920c809)
FIXNUM_TESTS

#define TEST_INTCONVERSION(name, inputint, inputfrac, inf, outputsign, outputint) \
TEST_HELPER(fixint_##name, { \
  fixed gi64 = fix_convert_from_int64((int64_t) inputint); \
  fixed expectedint = (((fixed) (outputint)) << FIX_POINT_BITS); \
  if(outputsign == 1) { \
        expectedint = FIX_DATA_BITS((~(fixed) expectedint) + 0x4); \
  } else { \
        expectedint = FIX_DATA_BITS((fixed) expectedint); \
  }\
  if(inf == FIX_INF_POS || (outputsign == 0 && outputint >= FIX_INT_MAX)) { \
    expectedint = FIX_INF_POS; \
  } \
  if(inf == FIX_INF_NEG || (outputsign == 1 && ((outputint > FIX_INT_MAX) ))) { \
    expectedint = FIX_INF_NEG; \
  } \
  CHECK_EQ("fixint int64", gi64, expectedint); \
};)

#define INTCONVERSION_TESTS                                                                                      \
TEST_INTCONVERSION(zero            , 0              , 0                    , 0           , 0 , 0               ) \
TEST_INTCONVERSION(one             , 1              , 0                    , 0           , 0 , 1               ) \
TEST_INTCONVERSION(one_neg         , -1             , 0                    , 0           , 1 , 1               ) \
TEST_INTCONVERSION(two             , 2              , 0                    , 0           , 0 , 2               ) \
TEST_INTCONVERSION(two_neg         , -2             , 0                    , 0           , 1 , 2               ) \
TEST_INTCONVERSION(many            , 1000           , 4                    , 0           , 0 , 1000            ) \
TEST_INTCONVERSION(many_neg        , -1000          , 4                    , 0           , 1 , 1000            ) \
TEST_INTCONVERSION(max_int_l1      , FIX_INT_MAX-1  , 0                    , 0           , 0 , FIX_INT_MAX-1   ) \
TEST_INTCONVERSION(max_int         , FIX_INT_MAX    , 0                    , FIX_INF_POS , 0 , 0               ) \
TEST_INTCONVERSION(max_int_neg     , -FIX_INT_MAX   , 0                    , 0           , 1 , FIX_INT_MAX     ) \
TEST_INTCONVERSION(max_int_neg_m1  , -FIX_INT_MAX-1 , 0                    , FIX_INF_NEG , 1 , 0               ) \
TEST_INTCONVERSION(frac            , 0              , 5342                 , 0           , 0 , 0               ) \
TEST_INTCONVERSION(frac_neg        , -0             , 5342                 , 0           , 1 , 0               ) \
TEST_INTCONVERSION(regress0        , 0              , 00932                , 0           , 0 , 0               ) \
TEST_INTCONVERSION(regress1        , 100            , 002655               , 0           , 0 , 100             ) \
TEST_INTCONVERSION(regress12       , 100            , 0026550292968        , 0           , 0 , 100             ) \
TEST_INTCONVERSION(regress2        , 1              , 4142150878906        , 0           , 0 , 1               ) \
TEST_INTCONVERSION(regress3        , 1              , 6487121582031        , 0           , 0 , 1               ) \
TEST_INTCONVERSION(regress4        , 2              , 0                    , 0           , 0 , 2               ) \
TEST_INTCONVERSION(regress5        , -1             , 3862915039           , 0           , 1 , 1               )
INTCONVERSION_TESTS

//////////////////////////////////////////////////////////////////////////////

/* Doubles only have 53 bits of precision, but we have 64 (minux the flag bits).
 * If we end up shifting it left, there will be zero bits on the low end. This
 * shouldn't be a test failure, so fix it up. We might also shift some of the
 * precision bits off the left of the number.
 *
 * So, let's compure how many precision bits we should have... Or, more
 * usefully, how many 0 bits we have on the lower end of the number. Note that
 * we need to check if d is exactly a power of two, and add that in.
 *
 * */

#define CONVERT_DBL(name, d, bits) \
TEST_HELPER(convert_dbl_##name, { \
  fixed expected = bits;			\
  double locald = (d); \
  double log2d = log2(fabs(d)); \
  uint32_t ilog2d = (int) ceil(log2d); \
  int32_t bottom_zero_bits = ilog2d + 2 - 52 + FIX_FRAC_BITS; \
  fixed result; \
  if(bottom_zero_bits < 0 ) { \
    result = fix_convert_from_double(locald); \
    CHECK_EQ_NAN(#name " convert_from_double failed (no mask needed)", result, expected); \
  } else { \
        \
    uint64_t rounding_bit = (1ull) << (bottom_zero_bits + FIX_FLAG_BITS); \
    expected = ROUND_TO_EVEN(expected, bottom_zero_bits) << bottom_zero_bits; \
    expected |= bits & FIX_FLAGS_MASK; /* ensure we still have flags */\
    result = fix_convert_from_double(locald); \
    CHECK_CONDITION(#name " convert_from_double failed (mask needed)", \
        (!(FIX_IS_INF_POS(bits) | FIX_IS_INF_NEG(bits) | FIX_IS_NAN(bits) ) \
           && (((result - expected) <= rounding_bit) || ((expected - result) <= rounding_bit))) || \
        fix_eq_nan(result, expected), result, expected); \
  } \
  double d2 = fix_convert_to_double(result); \
  double limit = (pow(.5, (double)FIX_FRAC_BITS)); \
  if( !((fabs(locald - d2) < limit) || \
        (isinf(d2) && (isinf(d) || (locald) >= FIX_INT_MAX || locald < -FIX_INT_MAX )) || \
        (isnan(d) && isnan(d2))) ) { \
    char b1[FIX_PRINT_BUFFER_SIZE]; \
    fix_sprint(b1, result); \
    fail_msg( #name " convert_to_double failed : %g (%s "FIX_PRINTF_HEX") != %g", d2, b1, result, locald); \
  } \
    char buf[FIX_PRINT_BUFFER_SIZE]; \
    char bitsbuf[FIX_PRINT_BUFFER_SIZE]; \
    fix_sprint(buf, result); \
    fix_sprint(bitsbuf, bits); \
    printf("Test passed: %g == %g == %s == %s\n", locald, d2, buf, bitsbuf); \
};)

#define CONVERT_DBL_TESTS                                                \
CONVERT_DBL(zero     , (double)0                       , FIX_ZERO)	\
CONVERT_DBL(one      , (double)1                       , FIXNUM(1    , 0))       \
CONVERT_DBL(one_neg  , (double)-1                      , FIXNUM(-1   , 0))       \
CONVERT_DBL(two      , (double)2                       , FIXNUM(2    , 0))       \
CONVERT_DBL(two_neg  , (double)-2                      , FIXNUM(-2   , 0))       \
CONVERT_DBL(many     , (double)1000.4                  , FIXNUM(1000 , 4))       \
CONVERT_DBL(many_neg , (double)-1000.4                 , FIXNUM(-1000, 4))       \
CONVERT_DBL(frac     , (double)0.5342                  , FIXNUM(0    , 5342))    \
CONVERT_DBL(frac_neg , (double)-0.5342                 , FIXNUM(-0   , 5342))    \
CONVERT_DBL(underflow, (double)1e-60                   , FIX_ZERO)               \
CONVERT_DBL(max      , (double) FIX_INT_MAX    , FIX_INF_POS)            \
CONVERT_DBL(max_neg  , -((double) FIX_INT_MAX) , FIXNUM(-FIX_INT_MAX,0)) \
CONVERT_DBL(inf_pos  , (double)INFINITY                , FIX_INF_POS)            \
CONVERT_DBL(inf_neg  , (double)-INFINITY               , FIX_INF_NEG)            \
CONVERT_DBL(nan      , (double)nan("0.0")              , FIX_NAN)
CONVERT_DBL_TESTS

//////////////////////////////////////////////////////////////////////////////

#define TEST_EQ(name, op1, op2, value, valuenan) \
TEST_HELPER(equal_##name, { \
  fixed o1 = op1; \
  fixed o2 = op2; \
  CHECK_EQ_VALUE("values not right", o1, o2, value ); \
  CHECK_EQ_NAN_VALUE("NaN values not right", o1, o2, valuenan ); \
  CHECK_EQ_VALUE("values not right", o2, o1, value ); \
  CHECK_EQ_NAN_VALUE("NaN values not right", o2, o1, valuenan ); \
};)

#define EQ_TESTS                                     \
TEST_EQ(zero        , 0                , 0                , 1 , 1) \
TEST_EQ(one         , FIXNUM(1,0)      , FIXNUM(1,0)      , 1 , 1) \
TEST_EQ(one_neg     , FIXNUM(1,0)      , FIXNUM(-1,0)     , 0 , 0) \
TEST_EQ(frac        , FIXNUM(0,314159) , FIXNUM(0,314159) , 1 , 1) \
TEST_EQ(frac_ne     , FIXNUM(0,314159) , FIXNUM(3,314159) , 0 , 0) \
TEST_EQ(nan         , FIX_NAN          , FIX_NAN          , 0 , 1) \
TEST_EQ(nan_inf     , FIX_NAN          , FIX_INF_POS      , 0 , 0) \
TEST_EQ(nan_inf_neg , FIX_NAN          , FIX_INF_NEG      , 0 , 0) \
TEST_EQ(nan_num     , FIX_NAN          , FIXNUM(0,0)      , 0 , 0) \
TEST_EQ(inf         , FIX_INF_POS      , FIX_INF_POS      , 1 , 1) \
TEST_EQ(inf_inf_neg , FIX_INF_POS      , FIX_INF_NEG      , 0 , 0) \
TEST_EQ(inf_num     , FIX_INF_POS      , FIXNUM(0,0)      , 0 , 0) \
TEST_EQ(inf_neg     , FIX_INF_NEG      , FIX_INF_NEG      , 1 , 1) \
TEST_EQ(inf_neg_num , FIX_INF_NEG      , FIXNUM(0,0)      , 0 , 0)
EQ_TESTS

//////////////////////////////////////////////////////////////////////////////

#define TEST_ROUNDING(name, value, res_even, res_up, res_ceil, res_floor) \
TEST_HELPER(rounding_##name, { \
  fixed input = value; \
  /* Trust that FIXNUM is doing its job correctly. Therefore, if input is \
   * infinity, we can fix up the expected values below... */ \
  int64_t round_even  = fix_convert_to_int64(input); \
  int64_t exp_even    = FIX_IS_INF_POS(input) ? INT64_MAX : FIX_IS_INF_NEG(input) ? INT64_MIN : (res_even); \
  CHECK_INT_EQUAL("round to even" , round_even  , exp_even); \
  int64_t round_up    = fix_round_up_int64(input); \
  int64_t exp_up      = FIX_IS_INF_POS(input) ? INT64_MAX : FIX_IS_INF_NEG(input) ? INT64_MIN : (res_up); \
  CHECK_INT_EQUAL("round up"      , round_up    , exp_up); \
  int64_t round_ceil  = fix_ceil64(input); \
  int64_t exp_ceil    = FIX_IS_INF_POS(input) ? INT64_MAX : FIX_IS_INF_NEG(input) ? INT64_MIN : (res_ceil); \
  CHECK_INT_EQUAL("ceiling"       , round_ceil  , exp_ceil); \
  int64_t round_floor = fix_floor64(input); \
  int64_t exp_floor   = FIX_IS_INF_POS(input) ? INT64_MAX : FIX_IS_INF_NEG(input) ? INT64_MIN : (res_floor); \
  CHECK_INT_EQUAL("floor"         , round_floor , exp_floor); \
};)

/* Sometimes, if there's only one fractional bit, the numbers get a little
 * mangled. For example, 1.3 becomes 1.5, which rounds up to 2. Use the
 * "Single Fraction Adjustment" for some tests. */
#if FIX_FRAC_BITS == 1
#define SFA +1
#else
#define SFA 0
#endif

/* Modding by FIX_INT_MAX allows us to correct for situations where FIX_INT_MAX
 * is 1 -- that is, when there is one integer bit. In these situations,
 * the fixnum being tested is always +/- 0.5, and rounds to zero. */

/*            Name       Value              Even                       Up                         Ceil                    Floor */
#define ROUNDING_TESTS                                                                             \
                                                                                                   \
TEST_ROUNDING(zero      , FIXNUM(0  , 0) , (0)+0      , (0)+0      , (0 )+0  , (0)+0)              \
TEST_ROUNDING(zero3     , FIXNUM(0  , 3) , (0)+0      , (0)+0 +SFA , (0 )+1  , (0)+0)              \
TEST_ROUNDING(zero5     , FIXNUM(0  , 5) , (0)+0      , (0)+1      , (0 )+1  , (0)+0)              \
TEST_ROUNDING(zero7     , FIXNUM(0  , 7) , (0)+1 -SFA , (0)+1      , (0 )+1  , (0)+0)              \
\
TEST_ROUNDING(one       , FIXNUM(1  , 0) , (1)+0      , (1)+0      , (1 )+0  , (1)+0)              \
TEST_ROUNDING(one3      , FIXNUM(1  , 3) , (1)+0 +SFA , (1)+0 +SFA , (1 )+1  , (1)+0)              \
TEST_ROUNDING(one5      , FIXNUM(1  , 5) , (1)+1      , (1)+1      , (1 )+1  , (1)+0)              \
TEST_ROUNDING(one7      , FIXNUM(1  , 7) , (1)+1      , (1)+1      , (1 )+1  , (1)+0)              \
\
TEST_ROUNDING(two       , FIXNUM(2  , 0) , (2)+0      , (2)+0      , (2 )+0  , (2)+0)              \
TEST_ROUNDING(two3      , FIXNUM(2  , 3) , (2)+0      , (2)+0 +SFA , (2 )+1  , (2)+0)              \
TEST_ROUNDING(two5      , FIXNUM(2  , 5) , (2)+0      , (2)+1      , (2 )+1  , (2)+0)              \
TEST_ROUNDING(two7      , FIXNUM(2  , 7) , (2)+1 -SFA , (2)+1      , (2 )+1  , (2)+0)              \
\
TEST_ROUNDING(zero_neg  , FIXNUM(-0 , 0) , (0)-0      , (0)-0      , (0 )-0  , (0)-0)              \
TEST_ROUNDING(zero3_neg , FIXNUM(-0 , 3) , (0)-0      , (0)-0      , (0 )-0  , (0)-1)              \
TEST_ROUNDING(zero5_neg , FIXNUM(-0 , 5) , (0)-0      , (0)-0      , (0 )-0  , (0)-1)              \
TEST_ROUNDING(zero7_neg , FIXNUM(-0 , 7) , (0)-1 +SFA , (0)-1+SFA  , (0 )-0  , (0)-1)              \
\
TEST_ROUNDING(one_neg   , FIXNUM(-1 , 0) , -(1)-0     , -(1)-0     , -(1 )-0 , -(1)-0)             \
TEST_ROUNDING(one3_neg  , FIXNUM(-1 , 3) , -(1)-0-SFA , -(1)-0     , -(1 )-0 , -(1)-1)             \
TEST_ROUNDING(one5_neg  , FIXNUM(-1 , 5) , -(1)-1     , -(1)-0     , -(1 )-0 , -(1)-1)             \
TEST_ROUNDING(one7_neg  , FIXNUM(-1 , 7) , -(1)-1     , -(1)-1+SFA , -(1 )-0 , -(1)-1)             \
\
TEST_ROUNDING(two_neg   , FIXNUM(-2 , 0) , -(2)-0     , -(2)-0     , -(2 )-0 , -(2)-0)             \
TEST_ROUNDING(two3_neg  , FIXNUM(-2 , 3) , -(2)-0     , -(2)-0     , -(2 )-0 , -(2)-1)             \
TEST_ROUNDING(two5_neg  , FIXNUM(-2 , 5) , -(2)-0     , -(2)-0     , -(2 )-0 , -(2)-1)             \
TEST_ROUNDING(two7_neg  , FIXNUM(-2 , 7) , -(2)-1+SFA , -(2)-1+SFA , -(2 )-0 , -(2)-1)             \
                                                                                                   \
TEST_ROUNDING(max     , FIX_MAX     , FIX_INT_MAX  , FIX_INT_MAX  , FIX_INT_MAX  , FIX_INT_MAX-1)  \
TEST_ROUNDING(min     , FIX_MIN     , -FIX_INT_MAX , -FIX_INT_MAX , -FIX_INT_MAX , -FIX_INT_MAX)   \
TEST_ROUNDING(nan     , FIX_NAN     , 0            , 0            , 0            , 0)              \
TEST_ROUNDING(inf     , FIX_INF_POS , INT_MAX      , INT_MAX      , INT_MAX      , INT_MAX)        \
TEST_ROUNDING(inf_neg , FIX_INF_NEG , INT_MIN      , INT_MIN      , INT_MIN      , INT_MIN)
ROUNDING_TESTS

//////////////////////////////////////////////////////////////////////////////


#define FLOOR_CEIL(name, value, floor_result, ceil_result) \
TEST_HELPER(floor_ceil_##name, { \
  fixed input = value; \
  fixed floor = fix_floor(input); \
  fixed floor_expected = FIX_IS_INF_POS(input) ? FIX_INF_POS : \
                         FIX_IS_INF_NEG(input) ? FIX_INF_NEG : \
                          (floor_result); \
  CHECK_EQ_NAN("floor "#name, floor, floor_expected); \
  fixed ceil = fix_ceil(input); \
  fixed ceil_expected = FIX_IS_INF_POS(input) ? FIX_INF_POS : \
                        FIX_IS_INF_NEG(input) ? FIX_INF_NEG : \
                          (ceil_result); \
  CHECK_EQ_NAN("ceil "#name, ceil, ceil_expected); \
};)

#define FIX_SMALLEST_INT (1ull << 63)

#define FLOOR_CEIL_TESTS                                                                                  \
FLOOR_CEIL(zero     , FIXNUM(0  , 0)               , FIXNUM(0  , 0)          , FIXNUM(0  , 0))            \
FLOOR_CEIL(half     , FIXNUM(0  , 5)               , FIXNUM(0  , 0)          , FIXNUM(1  , 0))            \
FLOOR_CEIL(half_neg , FIXNUM(-0 , 5)               , FIXNUM(-1 , 0)          , FIXNUM(0  , 0))            \
FLOOR_CEIL(one      , FIXNUM(1  , 0)               , FIXNUM(1  , 0)          , FIXNUM(1  , 0))            \
FLOOR_CEIL(one_neg  , FIXNUM(-1 , 0)               , FIXNUM(-1 , 0)          , FIXNUM(-1 , 0))            \
FLOOR_CEIL(pi       , FIX_PI                       , FIXNUM(3  , 0)          ,                            \
                                                     FIX_INT_BITS > 3 ? FIXNUM(4 -SFA , 0) : FIX_INF_POS) \
FLOOR_CEIL(pi_neg   , fix_neg(FIX_PI)              , FIXNUM(-4 +SFA , 0)     , FIXNUM(-3 , 0))            \
FLOOR_CEIL(max      , FIX_MAX                      , FIXNUM(FIX_INT_MAX-1, 0), FIX_INF_POS)               \
FLOOR_CEIL(max_almost,fix_sub(FIX_MAX, FIXNUM(1,0)), FIXNUM(FIX_INT_MAX-2, 0), FIXNUM(FIX_INT_MAX-1,0))   \
FLOOR_CEIL(min      , FIX_MIN                      , FIX_SMALLEST_INT, FIX_SMALLEST_INT)                  \
FLOOR_CEIL(min_almost,fix_add(fix_neg(FIX_MAX), FIXNUM(1,0)),                                             \
                                                     FIXNUM(-FIX_INT_MAX+1,0),                            \
                                                     FIXNUM(-FIX_INT_MAX+2,0))                            \
FLOOR_CEIL(inf_pos  , FIX_INF_POS                  , FIX_INF_POS             , FIX_INF_POS)               \
FLOOR_CEIL(inf_neg  , FIX_INF_NEG                  , FIX_INF_NEG             , FIX_INF_NEG)               \
FLOOR_CEIL(nan      , FIX_NAN                      , FIX_NAN                 , FIX_NAN)
FLOOR_CEIL_TESTS

//////////////////////////////////////////////////////////////////////////////

static void constants(void **state) {
  fixed lpi         = FIXNUM(3,1415926535897932385);
  fixed ltau        = FIXNUM(6,2831853071795864769);
  fixed le          = FIXNUM(2,7182818284590452354);

  CHECK_EQ("pi", FIX_PI, lpi);
  CHECK_EQ("tau", FIX_TAU, ltau);
  CHECK_EQ("e", FIX_E, le);

  if(FIX_INT_MAX > 7) {
    // Do some basic functional tests
    fixed temp = fix_abs(fix_sub(fix_sub(FIX_TAU, FIX_PI), FIX_PI));
    fixed limit = FIX_ZERO;
    for(int i = 0; i < 10; i++ ) {
      limit = fix_add(limit, FIX_EPSILON);
    }
    char b1[FIX_PRINT_BUFFER_SIZE], b2[FIX_PRINT_BUFFER_SIZE];
    fix_sprint(b1, temp);
    fix_sprint(b2, FIX_PI);

    if(fix_cmp(temp, limit) >= 0) {
      fail_msg( "Tau - Pi - Pi is %s ("FIX_PRINTF_HEX
              "), not near zero. Pi is %s ("FIX_PRINTF_HEX")",
              b1, temp, b2, FIX_PI);
    }
    printf( "Tau - Pi - Pi is %s ("FIX_PRINTF_HEX")\n", b1, temp);
  }
}
#define CONSTANT_TESTS cmocka_unit_test(constants),

//////////////////////////////////////////////////////////////////////////////

#define TEST_CMP(name, op1, op2, result) \
TEST_HELPER(cmp_##name, { \
  fixed o1 = op1; \
  fixed o2 = op2; \
  int8_t cmp = fix_cmp(o1, o2); \
  int8_t expected = result; \
  CHECK_VALUE("cmp failed", cmp, expected, o1, o2); \
  CHECK_VALUE("ne failed", fix_ne(o1, o2), ( (FIX_IS_NAN(o1) | FIX_IS_NAN(o2))) | (expected != 0), o1, o2); \
  CHECK_VALUE("lt failed", fix_lt(o1, o2), (!(FIX_IS_NAN(o1) | FIX_IS_NAN(o2))) & (expected <  0), o1, o2); \
  CHECK_VALUE("gt failed", fix_gt(o1, o2), (!(FIX_IS_NAN(o1) | FIX_IS_NAN(o2))) & (expected >  0), o1, o2); \
  CHECK_VALUE("le failed", fix_le(o1, o2), (!(FIX_IS_NAN(o1) | FIX_IS_NAN(o2))) & (expected <= 0), o1, o2); \
  CHECK_VALUE("ge failed", fix_ge(o1, o2), (!(FIX_IS_NAN(o1) | FIX_IS_NAN(o2))) & (expected >= 0), o1, o2); \
};)

#define CMP_TESTS                                                              \
TEST_CMP(zero_zero_eq    , FIXNUM(0  , 0)          , FIXNUM(0    , 0)    , 0)  \
TEST_CMP(pos_zero_gt     , FIXNUM(1  , 0)          , FIXNUM(0    , 0)    , 1)  \
TEST_CMP(neg_zero_lt     , FIXNUM(-1 , 0)          , FIXNUM(0    , 0)    , -1) \
TEST_CMP(pos_pos_gt      , FIXNUM(1  , 4)          , FIXNUM(0    , 5)    , 1)  \
TEST_CMP(pos_pos_lt      , FIXNUM(0  , 2)          , FIXNUM(0    , 5)    , -1) \
TEST_CMP(pos_pos_eq      , FIXNUM(0  , 5)          , FIXNUM(0    , 5)    , 0)  \
TEST_CMP(neg_neg_gt      , FIXNUM(-0 , 4)          , FIXNUM(-1   , 5)    , 1)  \
TEST_CMP(neg_neg_lt      , FIXNUM(-0 , 9)          , FIXNUM(-0   , 5)    , -1) \
TEST_CMP(neg_neg_eq      , FIXNUM(-0 , 5)          , FIXNUM(-0   , 5)    , 0)  \
TEST_CMP(nan_nan         , FIX_NAN                 , FIX_NAN             , 1)  \
TEST_CMP(nan_inf_pos     , FIX_NAN                 , FIX_INF_POS         , 1)  \
TEST_CMP(nan_inf_neg     , FIX_NAN                 , FIX_INF_NEG         , 1)  \
TEST_CMP(nan_pos         , FIX_NAN                 , FIXNUM(0   , 5)    , 1)   \
TEST_CMP(nan_neg         , FIX_NAN                 , FIXNUM(-0  , 5)    , 1)   \
TEST_CMP(pos_nan         , FIXNUM(0   , 5)         , FIX_NAN             , 1)   \
TEST_CMP(neg_nan         , FIXNUM(-0  , 5)         , FIX_NAN             , 1)   \
TEST_CMP(inf_inf         , FIX_INF_POS             , FIX_INF_POS         , 0)  \
TEST_CMP(inf_pos         , FIX_INF_POS             , FIXNUM(0   , 5)    , 1)   \
TEST_CMP(inf_neg         , FIX_INF_POS             , FIXNUM(-0  , 5)    , 1)   \
TEST_CMP(inf_inf_neg     , FIX_INF_POS             , FIX_INF_NEG         , 1)  \
TEST_CMP(pos_inf         , FIXNUM(0   , 5)         , FIX_INF_POS         , -1)  \
TEST_CMP(neg_inf         , FIXNUM(-0  , 5)         , FIX_INF_POS         , -1)  \
TEST_CMP(pos_inf_neg     , FIXNUM(0   , 5)         , FIX_INF_NEG         ,  1)  \
TEST_CMP(neg_inf_neg     , FIXNUM(-0  , 5)         , FIX_INF_NEG         ,  1)  \
TEST_CMP(inf_neg_inf_pos , FIX_INF_NEG             , FIX_INF_POS         , -1) \
TEST_CMP(inf_neg_pos     , FIX_INF_NEG             , FIXNUM(0   , 5)    , -1)  \
TEST_CMP(inf_neg_neg     , FIX_INF_NEG             , FIXNUM(-0  , 5)    , -1)  \
TEST_CMP(pos_neg         , FIXNUM(17   , 3)        , FIXNUM(-24  , 5)   , 1)  \
TEST_CMP(neg_pos         , FIXNUM(-16  , 3)        , FIXNUM(24   , 5)   , -1)
CMP_TESTS

//////////////////////////////////////////////////////////////////////////////

#define ADD(name, op1, op2, result) \
TEST_HELPER(add_##name, { \
  fixed o1 = op1; \
  fixed o2 = op2; \
  fixed added = fix_add(o1,o2); \
  fixed expected = result; \
  CHECK_EQ_NAN("add", added, expected); \
  added = fix_add(o2,o1); \
  CHECK_EQ_NAN("add (reversed)", added, expected); \
};)

#define ADD_TESTS                                                                         \
ADD(zero_zero         , FIXNUM(0 ,0)           , FIXNUM(0,0  )           , FIXNUM(0,0))   \
ADD(frac_zero         , FIXNUM(0 ,5)           , FIXNUM(0,0  )           , FIXNUM(0,5))   \
ADD(frac_frac         , FIXNUM(0 ,5)           , FIXNUM(0,6  )           , FIXNUM(1,1))   \
ADD(one_zero          , FIXNUM(1 ,0)           , FIXNUM(0,0  )           , FIXNUM(1,0))   \
ADD(one_one           , FIXNUM(1 ,0)           , FIXNUM(1,0  )           , FIXNUM(2,0))   \
ADD(fifteen_one       , FIXNUM(15,0)           , FIXNUM(1,0  )           , FIXNUM(16,0))  \
ADD(pos_neg           , FIXNUM(15,0)           , FIXNUM(-1,5 )           , FIXNUM(13,5))  \
ADD(pos_neg_cross_zero, FIXNUM(0,5)            , FIXNUM(-18,0)           , FIXNUM(-17,5)) \
ADD(overflow          , FIXNUM(FIX_INT_MAX-1,5), FIXNUM(FIX_INT_MAX-1,5) , FIX_INF_POS)   \
ADD(overflow_neg      , FIXNUM(-FIX_INT_MAX,0) , FIXNUM(-FIX_INT_MAX+1,5), FIX_INF_NEG)   \
ADD(inf_number        , FIX_INF_POS            , FIXNUM(0 ,5)            , FIX_INF_POS)   \
ADD(inf_number_neg    , FIX_INF_POS            , FIXNUM(-1,0)            , FIX_INF_POS)   \
ADD(inf_neg_number    , FIX_INF_NEG            , FIXNUM(0 ,5)            , FIX_INF_NEG)   \
ADD(inf_neg_numberlneg, FIX_INF_NEG            , FIXNUM(-1,0)            , FIX_INF_NEG)   \
ADD(nan               , FIX_NAN                , FIX_ZERO                , FIX_NAN)       \
ADD(nan_inf           , FIX_NAN                , FIX_INF_POS             , FIX_NAN)       \
ADD(nan_frac          , FIX_NAN                , FIXNUM(0,5)             , FIX_NAN)       \
ADD(inf_nan           , FIX_INF_POS            , FIX_NAN                 , FIX_NAN)       \
ADD(nan_inf_neg       , FIX_NAN                , FIX_INF_NEG             , FIX_NAN)       \
ADD(inf_inf_neg       , FIX_INF_POS            , FIX_INF_NEG             , FIX_INF_POS)
ADD_TESTS

//////////////////////////////////////////////////////////////////////////////

#define MUL(name, op1, op2, result) \
TEST_HELPER(mul_##name, { \
  fixed o1 = op1; \
  fixed o2 = op2; \
  fixed muld = fix_mul(o1,o2); \
  fixed expected = result; \
  CHECK_EQ_NAN("multiply", muld, expected); \
};)

#define MUL_TESTS                                                                            \
MUL(half_zero            , FIXNUM(0,5)             , FIXNUM(0,0)         , FIXNUM(0,0))      \
MUL(one_one              , FIXNUM(1,0)             , FIXNUM(1,0)         , FIXNUM(1,0))      \
MUL(fifteen_one          , FIXNUM(15,0)            , FIXNUM(1,0)         , FIXNUM(15,0))     \
MUL(fifteen_two          , FIXNUM(15,0)            , FIXNUM(2,0)         , FIXNUM(30,0))     \
MUL(nthree_15            , FIXNUM(-3,0)            , FIXNUM(15,0)        , FIXNUM(-45,0))    \
MUL(nthree_n15           , FIXNUM(-3,0)            , FIXNUM(-15,0)       , FIXNUM(45,0))     \
MUL(three_n15            , FIXNUM(3,0)             , FIXNUM(-15,0)       , FIXNUM(-45,0))    \
MUL(nfrac5_15            , FIXNUM(-0,5)             , FIXNUM(-16,0)      , FIXNUM(8,0))      \
MUL(inf_ten              , FIX_INF_POS             , FIXNUM(10,0)        , FIX_INF_POS)      \
MUL(inf_neg              , FIX_INF_POS             , FIXNUM(-10,0)       , FIX_INF_NEG)      \
MUL(ninf_neg             , FIX_INF_NEG             , FIXNUM(-10,0)       , FIX_INF_POS)      \
MUL(neg_inf              , FIXNUM(-10,0)           , FIX_INF_POS         , FIX_INF_NEG)      \
MUL(neg_ninf             , FIXNUM(-10,0)           , FIX_INF_NEG         , FIX_INF_POS)      \
MUL(pos_nan              , FIXNUM(10,0)            , FIX_NAN             , FIX_NAN)          \
MUL(neg_nan              , FIXNUM(-10,0)           , FIX_NAN             , FIX_NAN)          \
MUL(inf_nan              , FIX_INF_POS             , FIX_NAN             , FIX_NAN)          \
MUL(ninf_nan             , FIX_INF_NEG             , FIX_NAN             , FIX_NAN)          \
MUL(nan_pos              , FIX_NAN                 , FIXNUM(10,0)        , FIX_NAN)          \
MUL(nan_neg              , FIX_NAN                 , FIXNUM(-10,0)       , FIX_NAN)          \
MUL(nan_inf              , FIX_NAN                 , FIX_INF_POS         , FIX_NAN)          \
MUL(nan_ninf             , FIX_NAN                 , FIX_INF_NEG         , FIX_NAN)          \
MUL(overflow             , FIXNUM(FIX_INT_MAX-1,5) , FIXNUM(2,0)         , FIX_INF_POS)      \
MUL(tinyoverflow         , FIXNUM((SQRT_MAX+10),5) , FIXNUM((SQRT_MAX+10),5) , FIX_INF_POS)  \
MUL(tinyoverflow_neg     , FIXNUM((SQRT_MAX+10),5) , FIXNUM(-(SQRT_MAX+10),5) , FIX_INF_NEG) \
MUL(tinyoverflow_neg_neg , FIXNUM(-(SQRT_MAX+10),5), FIXNUM(-(SQRT_MAX+10),5) , FIX_INF_POS) \
MUL(underflow            , FIX_EPSILON             , FIX_EPSILON         , FIX_ZERO)         \
MUL(underflow_neg        , fix_neg(FIX_EPSILON)    , FIXNUM(0,0555)      , FIX_ZERO)         \
MUL(underflow_neg_rte    , fix_neg(FIX_EPSILON)    , FIXNUM(0,4555)      , FIX_ZERO)         \
MUL(zero_inf             , FIXNUM(0,0)             , FIX_INF_POS         , FIXNUM(0,0))      \
MUL(zero_ninf            , FIXNUM(0,0)             , FIX_INF_NEG         , FIXNUM(0,0))      \
MUL(zero_nan             , FIXNUM(0,0)             , FIX_NAN             , FIX_NAN)
MUL_TESTS

//////////////////////////////////////////////////////////////////////////////

#define DIV(name, op1, op2, result) \
TEST_HELPER(div_##name, { \
  fixed o1 = op1; \
  fixed o2 = op2; \
  fixed divd = fix_div(o1,o2); \
  fixed expected = result; \
  CHECK_EQ_NAN("divide", divd, expected); \
};)

#define DIV_TESTS                                                                     \
DIV(one_one          , FIXNUM(1,0)            , FIXNUM(1,0)            ,FIXNUM(1,0))  \
DIV(fifteen_one      , FIXNUM(15,0)           , FIXNUM(1,0)            ,FIXNUM(15,0)) \
DIV(sixteen_two      , FIXNUM(16,0)           , FIXNUM(2,0)            ,(FIX_INT_BITS > 5) ? FIXNUM(8,0) : FIX_INF_POS) \
DIV(fifteen_nthree   , FIXNUM(15,0)           , FIXNUM(-3,0)           ,(FIX_INT_BITS > 4) ? FIXNUM(-5,0) : FIX_INF_NEG) \
DIV(nfifteen_nthree  , FIXNUM(-15,0)          , FIXNUM(-3,0)           ,(FIX_INT_BITS > 4) ? FIXNUM(5,0) : FIX_INF_POS) \
DIV(nfifteen_three   , FIXNUM(-15,0)          , FIXNUM(3,0)            ,(FIX_INT_BITS > 4) ? FIXNUM(-5,0) : FIX_INF_NEG) \
DIV(fifteen_frac5    , FIXNUM(15,0)           , FIXNUM(0,5)            ,FIXNUM(30,0)) \
DIV(overflow         , FIXNUM(FIX_INT_MAX-1,5), FIXNUM(0,5)            ,FIX_INF_POS)  \
DIV(overflow2        , FIXNUM(FIX_INT_MAX-1,5), FIX_EPSILON            ,FIX_INF_POS)  \
DIV(underflow        , FIX_EPSILON            , FIXNUM(10,0)           ,FIXNUM(0,0))  \
\
DIV(max_neg_ovf      , FIXNUM(-FIX_INT_MAX,0)   , FIXNUM(0,5)          ,FIX_INF_NEG)  \
DIV(max_neg_ovf_neg  , FIXNUM(-FIX_INT_MAX,0)   , FIXNUM(-0,5)         ,FIX_INF_POS)  \
DIV(max_neg_one      , FIXNUM(-FIX_INT_MAX,0)   , FIXNUM(1,0)          ,(FIX_INT_BITS == 1) ? FIXNUM(0,0) : FIXNUM(-FIX_INT_MAX,0)) \
DIV(max_neg5_one     , FIXNUM(-FIX_INT_MAX+1,5) , FIXNUM(1,0)          ,(FIX_INT_BITS == 1) ? FIXNUM(0,0) : FIXNUM(-FIX_INT_MAX+1,5)) \
DIV(max_neg_two      , FIXNUM(-FIX_INT_MAX,0)   , FIXNUM(2,0)          ,(FIX_INT_BITS <= 2) ? FIXNUM(0,0) : FIXNUM(-(FIX_INT_MAX/2),0)) \
DIV(max_neg5_two     , FIXNUM(-FIX_INT_MAX+1,5) , FIXNUM(2,0)          ,(FIX_INT_BITS <= 2) ? FIXNUM(0,0) : FIXNUM(-(FIX_INT_MAX/2)+1,75)) \
\
DIV(zero_zero        , FIXNUM(0,0)            , FIXNUM(0,0)            ,FIX_NAN)      \
DIV(one_zero         , FIXNUM(1,0)            , FIXNUM(0,0)            ,FIX_INF_POS)  \
DIV(none_zero        , FIXNUM(-1,0)           , FIXNUM(0,0)            ,FIX_INF_NEG)  \
DIV(inf_zero         , FIX_INF_POS            , FIXNUM(0,0)            ,FIX_INF_POS)  \
DIV(ninf_zero        , FIX_INF_NEG            , FIXNUM(0,0)            ,FIX_INF_NEG)  \
DIV(nan_zero         , FIX_NAN                , FIXNUM(0,0)            ,FIX_NAN)  \
\
DIV(zero_inf         , FIXNUM(0,0)            , FIX_INF_POS            ,FIXNUM(0,0))  \
DIV(inf_neg          , FIX_INF_POS            , FIXNUM(-1,0)           ,FIX_INF_NEG)  \
DIV(ninf_neg         , FIX_INF_NEG            , FIXNUM(-1,0)           ,FIX_INF_POS)  \
DIV(neg_inf          , FIXNUM(-1,0)           , FIX_INF_POS            ,FIXNUM(0,0))  \
DIV(neg_ninf         , FIXNUM(-1,0)           , FIX_INF_NEG            ,FIXNUM(0,0))  \
DIV(pos_nan          , FIXNUM(0,5)            , FIX_NAN                ,FIX_NAN)      \
DIV(neg_nan          , FIXNUM(-0,5)           , FIX_NAN                ,FIX_NAN)      \
DIV(inf_inf          , FIX_INF_POS            , FIX_INF_POS            ,FIX_INF_POS)  \
DIV(ninf_inf         , FIX_INF_NEG            , FIX_INF_POS            ,FIX_INF_NEG)  \
DIV(inf_ninf         , FIX_INF_POS            , FIX_INF_NEG            ,FIX_INF_NEG)  \
DIV(inf_nan          , FIX_INF_POS            , FIX_NAN                ,FIX_NAN)      \
DIV(ninf_nan         , FIX_INF_NEG            , FIX_NAN                ,FIX_NAN)      \
DIV(nan_pos          , FIX_NAN                , FIXNUM(10,0)           ,FIX_NAN)      \
DIV(nan_neg          , FIX_NAN                , FIXNUM(-10,0)          ,FIX_NAN)      \
DIV(nan_inf          , FIX_NAN                , FIX_INF_POS            ,FIX_NAN)      \
DIV(nan_ninf         , FIX_NAN                , FIX_INF_NEG            ,FIX_NAN)
DIV_TESTS

//////////////////////////////////////////////////////////////////////////////

#define NEG(name, op1, result) \
TEST_HELPER(neg_##name, { \
  fixed o1 = op1; \
  fixed negd = fix_neg(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, negd, expected); \
};)

#define NEG_TESTS                        \
NEG(zero,    FIXNUM(0,0)           , FIXNUM(0,0))  \
NEG(half,    FIXNUM(0,5)           , FIXNUM(-0,5)) \
NEG(half_neg,FIXNUM(-0,5)          , FIXNUM(0,5))  \
NEG(min,     FIXNUM(-FIX_INT_MAX,0), FIX_INF_POS)  \
NEG(inf,     FIX_INF_POS           , FIX_INF_NEG)  \
NEG(inf_neg, FIX_INF_NEG           , FIX_INF_POS)  \
NEG(nan,     FIX_NAN               , FIX_NAN)
NEG_TESTS

//////////////////////////////////////////////////////////////////////////////

#define ABS(name, op1, result) \
TEST_HELPER(abs_##name, { \
  fixed o1 = op1; \
  fixed absd = fix_abs(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, absd, expected); \
};)

#define ABS_TESTS                                     \
ABS(zero         , FIXNUM(0,0)           , FIXNUM(0,0)) \
ABS(half         , FIXNUM(0,5)           , FIXNUM(0,5)) \
ABS(half_neg     , FIXNUM(-0,5)          , FIXNUM(0,5)) \
ABS(one          , FIXNUM(1,0)           , FIXNUM(1,0)) \
ABS(one_neg      , FIXNUM(-1,0)          , FIXNUM(1,0)) \
ABS(two          , FIXNUM(2,0)           , FIXNUM(2,0)) \
ABS(max_neg      , FIXNUM(-FIX_INT_MAX,0), FIX_INF_POS) \
ABS(inf          , FIX_INF_POS           , FIX_INF_POS) \
ABS(inf_neg      , FIX_INF_NEG           , FIX_INF_POS) \
ABS(nan          , FIX_NAN               , FIX_NAN)
ABS_TESTS

//////////////////////////////////////////////////////////////////////////////

#define LN(name, op1, result) \
TEST_HELPER(ln_##name, { \
  fixed o1 = op1; \
  fixed ln = fix_ln(o1); \
  fixed expected = result; \
  if(FIX_IS_INF_POS(op1)) { \
    expected = FIX_INF_POS; \
  } \
  CHECK_DIFFERENCE(#name, ln, expected, FIXNUM(0,000000000000004) | FIX_EPSILON); \
};)

#define LN_TESTS                                                         \
LN(zero   , FIX_ZERO          , FIX_INF_NEG)                             \
LN(half   , FIXNUM(0,5)       , FIXNUM(-0,6931471805599453094172321215)) \
LN(one    , FIXNUM(1,0)       , FIXNUM(0 , 0))                           \
LN(two    , FIXNUM(2,0)       , FIXNUM(0,6931471805599453094172321215))  \
LN(four   , FIXNUM(4,0)       , FIXNUM(1,386294361119890618834464243))   \
LN(e      , FIX_E             , FIXNUM(1,0) )                            \
LN(two75  , FIXNUM(2,75)      , FIXNUM(1,011600911678479925227479335))   \
LN(ten    , FIXNUM(10,0)      , FIXNUM(2,302585092994045684017991455) )  \
LN(63     , FIXNUM(63,0)      , FIXNUM(4,143134726391532687895843217))   \
LN(64     , FIXNUM(64,0)      , FIXNUM(4,158883083359671856503392729))   \
LN(64_5   , FIXNUM(64,5)      , FIXNUM(4,166665223801726805450855629))   \
LN(epsilon, FIX_EPSILON       , FIX_TEST_LN_epsilon)                     \
LN(big    , FIXNUM(536870911,0), FIXNUM(20,10126823437576882213405101290619719824480))\
LN(max    , FIX_MAX           , FIX_TEST_LN_max)                         \
LN(inf    , FIX_INF_POS       , FIX_INF_POS)                             \
LN(neg    , FIXNUM(-1,0)      , FIX_NAN)                                 \
LN(nan    , FIX_NAN           , FIX_NAN)

LN_TESTS

//////////////////////////////////////////////////////////////////////////////

#define LOG2(name, op1, result) \
TEST_HELPER(log2_##name, { \
  fixed o1 = op1; \
  fixed log2 = fix_log2(o1); \
  fixed expected = result; \
  if(FIX_IS_INF_POS(op1)) { \
    expected = FIX_INF_POS; \
  } \
  CHECK_DIFFERENCE(#name, log2, expected, FIXNUM(0,000000000000004) | FIX_EPSILON); \
};)

#define LOG2_TESTS                                             \
LOG2(zero   , FIX_ZERO          , FIX_INF_NEG)                 \
LOG2(one    , FIXNUM(1,0)       , FIXNUM(0 , 0))               \
LOG2(two    , FIXNUM(2,0)       , FIXNUM(1 , 0))               \
LOG2(two75  , FIXNUM(2,75)      , FIXNUM(1,4594316186372973))  \
LOG2(ten    , FIXNUM(10,0)      , FIXNUM(3,3219280948873626) ) \
LOG2(63     , FIXNUM(63,0)      , FIXNUM(5,977279923499917))   \
LOG2(64     , FIXNUM(64,0)      , FIXNUM(6, 0) )               \
LOG2(64_5   , FIXNUM(64,5)      , FIXNUM(6,011227255423254))   \
LOG2(epsilon, FIX_EPSILON       , FIX_TEST_LOG2_epsilon)       \
LOG2(max    , FIX_MAX           , FIX_TEST_LOG2_max )          \
LOG2(min    , FIX_MIN           , FIX_NAN )                    \
LOG2(inf    , FIX_INF_POS       , FIX_INF_POS)                 \
LOG2(neg    , FIXNUM(-1,0)      , FIX_NAN)                     \
LOG2(nan    , FIX_NAN           , FIX_NAN)

LOG2_TESTS

//////////////////////////////////////////////////////////////////////////////

#define LOG10(name, op1, result) \
TEST_HELPER(log10_##name, { \
  fixed o1 = op1; \
  fixed log10 = fix_log10(o1); \
  fixed expected = result; \
  if(FIX_IS_INF_POS(op1)) { \
    expected = FIX_INF_POS; \
  } \
  CHECK_DIFFERENCE(#name, log10, expected, FIXNUM(0,000000000000004) | FIX_EPSILON); \
};)

#define LOG10_TESTS                                                         \
LOG10(zero   , FIX_ZERO          , FIX_INF_NEG)                             \
LOG10(half   , FIXNUM(0,5)       , FIXNUM(-0,3010299956639811952137388947)) \
LOG10(one    , FIXNUM(1,0)       , FIXNUM(0,0))                             \
LOG10(two    , FIXNUM(2,0)       , FIXNUM(0,3010299956639811952137388947))  \
LOG10(two75  , FIXNUM(2,75)      , FIXNUM(0,4393326938302626503227221818) ) \
LOG10(ten    , FIXNUM(10,0)      , FIXNUM(1,0) )                            \
LOG10(fifteen, FIXNUM(15,0)      , FIXNUM(1,176091259055681242081289009))   \
LOG10(63     , FIXNUM(63,0)      , FIXNUM(1,799340549453581705302272065))   \
LOG10(64     , FIXNUM(64,0)      , FIXNUM(1,806179973983887171282433368) )  \
LOG10(64_5   , FIXNUM(64,5)      , FIXNUM(1,809559714635267768486377162))   \
LOG10(epsilon, FIX_EPSILON       , FIX_TEST_LOG10_epsilon)                  \
LOG10(max    , FIX_MAX           , FIX_TEST_LOG10_max )                     \
LOG10(inf    , FIX_INF_POS       , FIX_INF_POS)                             \
LOG10(neg    , FIXNUM(-1,0)      , FIX_NAN)                                 \
LOG10(nan    , FIX_NAN           , FIX_NAN)

LOG10_TESTS

//////////////////////////////////////////////////////////////////////////////

#define EXP(name, op1, result) \
TEST_HELPER(exp_##name, { \
  fixed o1 = op1; \
  fixed exp = fix_exp(o1); \
  fixed expected = result; \
  if(FIX_IS_INF_POS(op1)) { \
    expected = FIX_INF_POS; \
  } else if (FIX_IS_INF_NEG(op1)) { \
    expected = FIX_ZERO; \
  }\
  CHECK_DIFFERENCE(#name, exp, expected, ((expected >> 54) | FIX_EPSILON)); \
};)
/* Make sure we get the top 54 bits of expected right */

#define EXP_TESTS                                                                                         \
EXP(zero      , FIX_ZERO        , FIXNUM(1,0))                                                            \
EXP(half      , FIXNUM(0,5)     , FIXNUM(1,648721270700128146848650787814163571653776100710148011575079)) \
EXP(half_neg  , FIXNUM(-0,5)    , FIXNUM(0,606530659712633423603799534991180453441918135487186955682892)) \
EXP(one       , FIXNUM(1,0)     , FIXNUM(2,718281828459045235360287471352662497757247093699959574966967)) \
EXP(one_neg   , FIXNUM(-1,0)    , FIXNUM(0,367879441171442321595523770161460867445811131031767834507836)) \
EXP(one_5    , FIXNUM( 1,5)     , FIXNUM(4,481689070338064822602055460119275819005749868369667056772650)) \
EXP(one_5_neg, FIXNUM(-1,5)     , FIXNUM(0,223130160148429828933280470764012521342171629361079328743835)) \
EXP(two       , FIXNUM(2,0)     , FIXNUM(7,389056098930650227230427460575007813180315570551847324087127)) \
EXP(two_neg   , FIXNUM(-2,0)    , FIXNUM(0,135335283236612691893999494972484403407631545909575881468158)) \
EXP(three5    , FIXNUM(3,5)     , FIXNUM(33,11545195869231375065324935038861629247172822647794098886094)) \
EXP(three5_neg, FIXNUM(-3,5)    , FIXNUM(0,030197383422318500739786292363619845071660532247657006671340)) \
EXP(four      , FIXNUM(4,0)     , FIXNUM(54,59815003314423907811026120286087840279073703861406872582659)) \
EXP(four_neg  , FIXNUM(-4,0)    , FIXNUM(0,018315638888734180293718021273241242211912067553475594769599)) \
EXP(ten       , FIXNUM(10,0)    , FIXNUM(22026,46579480671651695790064528424436635351261855678107423542)) \
EXP(ten_neg   , FIXNUM(-10,0)   , FIXNUM(0,000045399929762484851535591515560550610237918088866564969259)) \
EXP(neg_many  , FIXNUM(-128,0)  , FIXNUM(0,000000000000000000000000000000000000000000000000000000000000)) \
EXP(forty     , FIXNUM(40,0)    , FIXNUM(235385266837019985,40789991074903480450887161725455546))         \
EXP(max       , FIX_MAX         , FIX_INF_POS)                                                            \
EXP(nan       , FIX_NAN         , FIX_NAN)                                                                \
EXP(inf       , FIX_INF_POS     , FIX_INF_POS)                                                            \
EXP(inf_neg   , FIX_INF_NEG     , FIX_ZERO)

EXP_TESTS

//////////////////////////////////////////////////////////////////////////////

// Sometimes we compute results with doubles, and those results can be wrong due
// to double's 53-bit precision. Set a pretty high difference when comparing the
// sqrt results, but re-check by computing the square of the sqrt.

#define SQRT(name, op1, result) \
TEST_HELPER(sqrt_##name, { \
  fixed o1 = op1; \
  fixed fsqrt = fix_sqrt(o1); \
  fixed expected = result; \
  fixed square = fix_mul(fsqrt, fsqrt); \
  if(!FIX_EQ_NAN(fsqrt, expected)) { \
    /* compute the square error bars using doubles. If the square root is within
     * an Epsilon of the the real square root, the squared square root will be
     * how far from the original number?
     *
     * Because doubles are dumb, check that there's enough bits...
     * */ \
    double dsqrt = fix_convert_to_double(expected); \
    double eps = fix_convert_to_double(FIX_EPSILON); \
    if(fixed_log2(expected) > 53) { \
      /* adding epsilon to expected as a double won't do anything; make epsilon
       * larger */ \
      eps = fix_convert_to_double(FIX_DATA_BITS(expected >> 52)); \
    }  \
    double squarederror = ((dsqrt+eps)*(dsqrt+eps)) - (dsqrt * dsqrt); \
    fixed errorbar = fix_convert_from_double(squarederror); \
    CHECK_DIFFERENCE(#name " sqrt",   fsqrt,  expected, 0x80); \
    CHECK_DIFFERENCE(#name " square", square, op1,      errorbar); \
  } else { \
    CHECK_EQ_NAN(#name, fsqrt, expected); \
  } \
};)

#define SQRT_TESTS                                                                              \
SQRT(zero   , FIX_ZERO          , FIXNUM(0,0))                                                  \
SQRT(half   , FIXNUM(0,5)       , FIXNUM(0,7071067811865475244008443621))                       \
SQRT(one    , FIXNUM(1,0)       , FIXNUM(1,0))                                                  \
SQRT(two    , FIXNUM(2,0)       , FIX_INT_BITS >= 3 ? FIXNUM(1,4142135623730951) : FIX_INF_POS) \
SQRT(e      , FIX_E             , FIX_INT_BITS >= 3 ? FIXNUM(1,6487212707001282) : FIX_INF_POS) \
SQRT(ten    , FIXNUM(10,0)      , FIX_INT_BITS >= 5 ? FIXNUM(3,1622776601683795) : FIX_INF_POS) \
SQRT(big    , FIXNUM(10000,5345), FIX_INT_BITS > 14 ? FIXNUM(100,00267246428967) : FIX_INF_POS) \
SQRT(max    , FIX_MAX           , SQRT_MAX_FIXED)                                               \
SQRT(inf    , FIX_INF_POS       , FIX_INF_POS)                                                  \
SQRT(neg    , FIXNUM(-1,0)      , FIX_NAN)                                                      \
SQRT(nan    , FIX_NAN           , FIX_NAN)
SQRT_TESTS

//////////////////////////////////////////////////////////////////////////////

#define POW(name, op1, op2, result, bitaccuracy) \
TEST_HELPER(pow_##name, { \
  fixed o1 = op1; \
  fixed o2 = op2; \
  fixed powresult = fix_pow(o1, o2); \
  fixed expected = result; \
  if(FIX_IS_INF_POS(o1) && !FIX_IS_NAN(o2)) { \
    expected = FIX_INF_POS; \
  }  \
  if(FIX_IS_INF_NEG(o1) && !FIX_IS_NAN(o2) && (FIX_IS_INF_POS(o2))) { \
    expected = FIX_INF_NEG; \
  }  \
  fixed bound = ((FIX_EPSILON) + FIX_DATA_BITS( (((bitaccuracy) >= 0) ? ((expected) >> (bitaccuracy)) : ((expected) << (-(bitaccuracy))) )) ); \
  CHECK_DIFFERENCE(#name, powresult, expected, bound); \
};)

/* We add FIX_EPSILON to the shifted result above in order to ignore rounding
 * issues when shifting expected, and to always allow a FIX_EPSILON upper bound on error. */

#define POW_TESTS                                                                                                                                    \
POW(zero_zero       , FIX_ZERO     , FIX_ZERO     , FIXNUM(1,0)                      , 63)                                                           \
POW(zero_one        , FIX_ZERO     , FIXNUM(1,0)  , FIX_ZERO                         , 63)                                                           \
POW(half_zero       , FIXNUM(0,5)  , FIX_ZERO     , FIXNUM(1,0)                      , 63 )                                                          \
POW(half_square     , FIXNUM(0,5)  , FIXNUM(2,0)  , FIX_INT_BITS < 3 ? FIX_ZERO : FIXNUM(0,25)  , FIX_FRAC_BITS < 47 ? FIX_FRAC_BITS-1 : 46)         \
POW(half_nsquare    , FIXNUM(0,5)  , FIXNUM(-2,0) , FIX_INT_BITS < 4 ? FIX_INF_POS : FIXNUM(4,0), FIX_FRAC_BITS < 47 ? FIX_FRAC_BITS-1 : 46)         \
POW(half_neight     , FIXNUM(0,5)  , FIXNUM(-8,0) , FIX_INT_BITS < 4 ? FIX_INF_POS :                                                                 \
                                                          FIX_INT_BITS < 9 ? FIX_INF_POS : FIXNUM(256 ,0), FIX_FRAC_BITS <= 1 ? FIX_FRAC_BITS - 6 :  \
                                                                                             FIX_FRAC_BITS < 48 ? FIX_FRAC_BITS-4 : 44)              \
POW(sqrt            , FIXNUM(536870911,0), FIXNUM(0,5)  ,                                                                                            \
                      FIX_INT_BITS <= 29 ? FIX_INF_POS :                                                                                             \
                      FIXNUM(23170,4749843416028319405320375889409653360224397842555396195)                                                          \
                                                                                           , FIX_FRAC_BITS >= 48 ? 48 : FIX_FRAC_BITS)               \
                      /* (1 + 2^-13)  */                                                                                                             \
POW(epsilon         , FIXNUM(1,0001220703125), FIXNUM(70911,0),                                                                                      \
                      FIX_INT_BITS <= 17 ? FIX_INF_POS :                                                                                             \
                      FIXNUM(5742,211216908114514755729967881141948869288460297210364158767605983527118659),                                         \
                        FIX_FRAC_BITS <  13 ? 0 :                                                                                                    \
                        FIX_FRAC_BITS <= 15 ? FIX_FRAC_BITS - 20 :                                                                                   \
                        FIX_FRAC_BITS <= 47 ? FIX_FRAC_BITS - 16 :                                                                                   \
                        31                                                                                                                           \
                      )                                                                                                                              \
                                                                                                                                                     \
POW(one2            , FIXNUM( 1,0) , FIXNUM( 2,0) , FIXNUM(1   ,0)                             , FIX_FRAC_BITS < 47 ? FIX_FRAC_BITS-2 : 46)          \
POW(one4            , FIXNUM( 1,0) , FIXNUM( 4,0) , FIXNUM(1   ,0)                             , FIX_FRAC_BITS < 48 ? FIX_FRAC_BITS-3 : 45)          \
POW(two_sqrt        , FIXNUM( 2,0) , FIXNUM( 0,5) , FIXNUM(1   ,41421356237309504880168872421) , FIX_FRAC_BITS < 48 ? FIX_FRAC_BITS-1 : 48)          \
POW(ten_sqrt        , FIXNUM(10,0) , FIXNUM( 0,5) , FIXNUM(3   ,16227766016837933199889354443) , FIX_FRAC_BITS < 36 ? FIX_FRAC_BITS   : 35)          \
POW(ten_square      , FIXNUM(10,0) , FIXNUM( 2,0) , FIXNUM(100 ,0)                             , FIX_FRAC_BITS < 47 ? FIX_FRAC_BITS-2 : 46)          \
POW(ten_odd         , FIXNUM(10,0) , FIXNUM( 1,5) , FIXNUM(31  ,6227766016837933199889354443)  , FIX_FRAC_BITS < 48 ? FIX_FRAC_BITS-1 : 47)          \
POW(ten_cubed       , FIXNUM(10,0) , FIXNUM( 3,0) , FIXNUM(1000,0)                             , FIX_FRAC_BITS < 47 ? FIX_FRAC_BITS-2 : 46)          \
POW(neg_one2        , FIXNUM(-1,0) , FIXNUM( 2,0) , FIX_INT_BITS <= 2 ? FIXNUM(-1, 0) : FIXNUM(1,0), FIX_FRAC_BITS < 47 ? FIX_FRAC_BITS-2 : 46)      \
POW(neg_one3        , FIXNUM(-1,0) , FIXNUM( 3,0) , FIXNUM(-1  ,0)                             , FIX_FRAC_BITS < 47 ? FIX_FRAC_BITS-2 : 46)          \
POW(neg_two2        , FIXNUM(-1,5) , FIXNUM( 2,0) , FIX_INT_BITS <= 2 ? FIX_INF_NEG : FIXNUM(2,25), FIX_FRAC_BITS < 47 ? FIX_FRAC_BITS-2 : 46)       \
POW(neg_two3        , FIXNUM(-2,0) , FIXNUM( 3,0) , FIX_INT_BITS <= 3 ? FIX_INF_NEG : FIXNUM(-8,0), FIX_FRAC_BITS < 47 ? FIX_FRAC_BITS-2 : 46)       \
POW(neg_two_neg2    , FIXNUM(-2,0) , FIXNUM(-2,0) , FIX_INT_BITS <= 1 ? FIX_INF_NEG :                                                                \
                                                    FIX_INT_BITS <= 2 ? FIX_INF_POS : FIXNUM(0,25), FIX_FRAC_BITS < 47 ? FIX_FRAC_BITS-2 : 46)       \
POW(neg_nan         , FIXNUM(-0,5) , FIXNUM( 0,5) , FIX_NAN, 63)                                                                                     \
POW(neg_pos_oflw    , FIXNUM(-2,0) , FIXNUM(536870910,0), FIX_INT_BITS <= 29 ? FIX_INF_NEG : FIX_INF_POS, 63)                                        \
                                                                                                                                                     \
POW(nan_not         , FIX_NAN      , FIXNUM(1,0)  , FIX_NAN, 63)                                                                                     \
POW(not_nan         , FIXNUM(0,5)  , FIX_NAN      , FIX_NAN, 63)                                                                                     \
                                                                                                                                                     \
POW(oflw_inf_gt     , FIXNUM( 1,5) , FIX_INF_POS  , FIX_INF_POS , 63)                                                                                \
POW(oflw_inf_one    , FIXNUM( 1,0) , FIX_INF_POS  , FIXNUM(1,0) , 63)                                                                                \
POW(oflw_inf_hlf    , FIXNUM( 0,5) , FIX_INF_POS  , FIX_ZERO    , 63)                                                                                \
POW(oflw_inf_zero   , FIXNUM( 0,0) , FIX_INF_POS  , FIX_ZERO    , 63)                                                                                \
POW(oflw_inf_nhlf   , FIXNUM(-0,5) , FIX_INF_POS  , FIX_ZERO    , 63)                                                                                \
POW(oflw_inf_none   , FIXNUM(-1,0) , FIX_INF_POS  , FIXNUM(-1,0), 63)                                                                                \
POW(oflw_inf_ngt    , FIXNUM(-1,5) , FIX_INF_POS  , FIX_INF_NEG , 63)                                                                                \
                                                                                                                                                     \
POW(oflw_ninf_gt    , FIXNUM( 1,5) , FIX_INF_NEG  , FIX_ZERO    , 63)                                                                                \
POW(oflw_ninf_one   , FIXNUM( 1,0) , FIX_INF_NEG  , FIXNUM(1,0) , 63)                                                                                \
POW(oflw_ninf_hlf   , FIXNUM( 0,5) , FIX_INF_NEG  , FIX_INF_POS , 63)                                                                                \
POW(oflw_ninf_zero  , FIXNUM( 0,0) , FIX_INF_NEG  , FIX_ZERO    , 63)                                                                                \
POW(oflw_ninf_nhlf  , FIXNUM(-0,5) , FIX_INF_NEG  , FIX_INF_NEG , 63)                                                                                \
POW(oflw_ninf_none  , FIXNUM(-1,0) , FIX_INF_NEG  , FIXNUM(-1,0), 63)                                                                                \
POW(oflw_ninf_ngt   , FIXNUM(-1,5) , FIX_INF_NEG  , FIX_INT_BITS < 2 ? FIX_INF_NEG : FIX_ZERO, 63)                                                   \
                                                                                                                                                     \
POW(neg_overflow    , FIXNUM(-2,0) , FIXNUM(127,0), FIX_INF_NEG, 63)                                                                                 \
POW(pos_overflow    , FIXNUM( 2,0) , FIXNUM(100,0), FIX_INF_POS, 63)                                                                                 \
POW(inf_not         , FIX_INF_POS  , FIXNUM(1,0)  , FIX_INF_POS, 63)

POW_TESTS

//////////////////////////////////////////////////////////////////////////////
// To print out all sins:
//int roots_of_unity = 16;
//fixed x = fix_neg(FIX_TAU);
//fixed step = fix_div(FIX_TAU, FIXINT(roots_of_unity));

//for(int i = 0; i <= roots_of_unity * 4; i++) {
//  printf("\nsin of:");
//  pl(x);
//  printf(": ");
//  pl(fix_sin(x));

//  x = fix_add(x, step);
//}
//printf("\n");

//p(fix_sin(FIX_PI));


/**#define SIN(name, op1, result)               \
 *TEST_HELPER(sin_##name, {                     \
 *  fixed o1 = op1;                             \
 *  fixed sin = fix_sin_fast(o1);               \
 *  fixed expected = result;                    \
 *  CHECK_EQ_NAN(#name, sin, expected);         \
 *};)

*#define SIN_TESTS                                                                         \
*SIN(zero       , FIX_ZERO                                           , 0)                  \
*SIN(pi_2  , fix_div(FIX_PI , FIXNUM(2 , 0))                    , 0x1fffc)                 \
*SIN(pi    , FIX_PI                                             , 0xfffffffc)              \
*SIN(pi3_2 , fix_div(fix_mul(FIX_PI, FIXNUM(3,0)), FIXNUM(2,0)) , 0xfffe0000)              \
*SIN(pi2   , fix_mul(FIX_PI, FIXNUM(2,0))                       , 0)                       \
*SIN(pi5_2 , fix_div(fix_mul(FIX_PI, FIXNUM(5,0)), FIXNUM(2,0)) , 0x1fffc)                 \
*SIN(pi3   , fix_mul(FIX_PI, FIXNUM(3,0))                       , 0xfffffffc)              \
*SIN(pi7_2 , fix_div(fix_mul(FIX_PI, FIXNUM(7,0)), FIXNUM(2,0)) , 0xfffe0000)              \
*SIN(pi4   , fix_mul(FIX_PI, FIXNUM(4,0))                       , 0x4)                     \
*                                                                                          \
*SIN(neg_pi_2  , fix_neg(fix_div(FIX_PI , FIXNUM(2 , 0)))                    , 0xfffe0000) \
*SIN(neg_pi    , fix_neg(FIX_PI)                                             , 0x0)        \
*SIN(neg_pi3_2 , fix_neg(fix_div(fix_mul(FIX_PI, FIXNUM(3,0)), FIXNUM(2,0))) , 0x1fffc)    \
*SIN(neg_pi2   , fix_neg(fix_mul(FIX_PI, FIXNUM(2,0)))                       , 0xfffffffc) \
*                                                                                          \
*SIN(inf_pos   , FIX_INF_POS                          , FIX_INF_POS)                       \
*SIN(inf_neg   , FIX_INF_NEG                          , FIX_INF_NEG)                       \
*SIN(nan       , FIX_NAN                              , FIX_NAN)
*SIN_TESTS
**/
//////////////////////////////////////////////////////////////////////////////

#define TRIG(name, op1, sinx, cosx, tanx, bounds) \
TEST_HELPER(trig_##name, { \
  fixed o1 = op1; \
  fixed sin = fix_sin(o1); \
  fixed sinresult = sinx; \
  fixed cosresult = cosx; \
  fixed tanresult = tanx; \
  if(FIX_IS_INF_POS(op1) | FIX_IS_INF_NEG(op1)) { \
    sinresult = cosresult = tanresult = FIX_NAN; \
  } \
  CHECK_DIFFERENCE(#name " sin", sin, sinresult, bounds); \
  fixed cos = fix_cos(o1); \
  CHECK_DIFFERENCE(#name " cos", cos, cosresult, bounds); \
  if(!FIX_IS_NAN(tanx)) { \
    fixed tan = fix_tan(o1); \
    CHECK_DIFFERENCE(#name " tan", tan, tanresult, bounds); \
  }\
};)

// Note that tan is poorly defined near pi/2 + n*pi. It's either positive
// infinity or negative infinity, with very little separating them.

#define err2_57 (FIXNUM(0,000000000000000006938893903907228377647697925567626953125) | FIX_EPSILON)

#define TRIG_TESTS                                                                                             \
TRIG(zero      , FIX_ZERO                             , FIX_ZERO     ,                                         \
                   FIX_INT_BITS!=1? FIXNUM(1,0) : FIXNUM( 0,999999999999999999132638262), FIX_NAN , err2_57)   \
TRIG(pi_2      , FIXNUM( 1,57079632679489661923132169), FIXNUM(1,0)  , FIX_ZERO    , FIX_NAN      , err2_57)   \
TRIG(pi        , FIXNUM( 3,14159265358979323846264338), FIX_ZERO     , FIXNUM(-1,0), FIX_NAN      , err2_57)   \
TRIG(pi3_2     , FIXNUM( 4,71238898038468985769396507), FIXNUM(-1,0) , FIX_ZERO    , FIX_NAN      , err2_57)   \
TRIG(pi2       , FIXNUM( 6,28318530717958647692528676), FIX_ZERO     , FIXNUM(1,0) , FIX_NAN      , err2_57)   \
                                                                                                               \
TRIG(pi5_2     , FIXNUM( 7,85398163397448309615660845), FIXNUM(1,0) , FIX_ZERO    , FIX_NAN       , err2_57)   \
TRIG(pi3       , FIXNUM( 9,42477796076937971538793014), FIX_ZERO    , FIXNUM(-1,0), FIX_ZERO      , err2_57)   \
TRIG(pi7_2     , FIXNUM(10,99557428756427633461925184), FIXNUM(-1,0), FIX_ZERO    , FIX_NAN       , err2_57)   \
TRIG(pi4       , FIXNUM(12,56637061435917295385057353), FIX_ZERO    , FIXNUM(1,0) , FIX_ZERO      , err2_57)   \
                                                                                                               \
TRIG(neg_pi_2  , FIXNUM(-1,57079632679489661923132169) , FIXNUM(-1,0), FIX_ZERO    , FIX_NAN      , err2_57)   \
TRIG(neg_pi    , FIXNUM(-3,14159265358979323846264338) , FIX_ZERO    , FIXNUM(-1,0), FIX_ZERO     , err2_57)   \
TRIG(neg_pi3_2 , FIXNUM(-4,71238898038468985769396507) , FIXNUM(1,0) , FIX_ZERO    , FIX_NAN      , err2_57)   \
TRIG(neg_pi2   , FIXNUM(-6,28318530717958647692528676) , FIX_ZERO    , FIXNUM(1,0) , FIX_ZERO     , err2_57)   \
                                                                                                               \
TRIG(q1        , FIXNUM(1,0), FIXNUM(0,84147098480789650665250232163029899962256306079837106567275170),        \
                              FIXNUM(0,540302305868139717400936607442976603732310420617922) ,                  \
                              FIXNUM(1,557407724654902230506974807458360173087250772381520038383946605698861), \
                              2*err2_57)                                                                       \
TRIG(q2        , FIXNUM(2,57079632679489661923132169163975144209858469968755291048),                           \
                              FIXNUM( 0,54030230586813971740093660744297660373231042061792222767) ,            \
                              FIXNUM(-0,84147098480789650665250232163029899962256306079837106567) ,            \
                              FIXNUM(-0,64209261593433070300641998659426562023027811391817137910) ,            \
                              2*err2_57)                                                                       \
TRIG(q3        , FIXNUM(3,6415926535897932384626433832795028841971693993751058209749),                         \
                              FIXNUM(-0,479425538604203000273287935215571388081803367940600675188),            \
                              FIXNUM(-0,877582561890372716116281582603829651991645197109744052997),            \
                              FIXNUM(0,5463024898437905132551794657802853832975517201797912461640),            \
                              2*err2_57)                                                                       \
TRIG(q4        , FIXNUM(-0,5),                                                                                 \
                             FIXNUM(-0,479425538604203000273287935215571388081803367940600675188),             \
                             FIXNUM(0,8775825618903727161162815826038296519916451971097440529976),             \
                             FIXNUM(-0,546302489843790513255179465780285383297551720179791246164),             \
                             2*err2_57)                                                                        \
                                                                                                               \
TRIG(inf_pos   , FIX_INF_POS, FIX_NAN, FIX_NAN, FIX_NAN, FIX_ZERO)                                             \
TRIG(inf_neg   , FIX_INF_NEG, FIX_NAN, FIX_NAN, FIX_NAN, FIX_ZERO)                                             \
TRIG(nan       , FIX_NAN,     FIX_NAN, FIX_NAN, FIX_NAN, FIX_ZERO)
TRIG_TESTS

//////////////////////////////////////////////////////////////////////////////

#define PRINT(name, op1, result) \
TEST_HELPER(print_##name, { \
  fixed o1 = op1; \
  char buf[FIX_PRINT_BUFFER_SIZE]; \
  fix_sprint(buf, o1); \
  char* expected = result; \
  if(strcmp(buf, expected))  { \
    printf("Strings not equal: '%s' != '%s'\n", buf, expected); \
    fix_sprint(buf, o1); \
    printf("I should have expected: '%s'\n", buf); \
  } \
  assert_memory_equal(buf, expected, 23); \
};)


#define PRINT_TESTS                                                     \
PRINT(zero        , FIX_ZERO                  , PRINT_TEST_zero       ) \
PRINT(half        , FIXNUM(0,5)               , PRINT_TEST_half       ) \
PRINT(half_neg    , FIXNUM(-0,5)              , PRINT_TEST_half_neg   ) \
PRINT(one         , FIXNUM(1,0)               , PRINT_TEST_one        ) \
PRINT(one_neg     , FIXNUM(-1,0)              , PRINT_TEST_one_neg    ) \
PRINT(two         , FIXNUM(2,0)               , PRINT_TEST_two        ) \
PRINT(epsilon     , FIX_EPSILON               , PRINT_TEST_epsilon    ) \
PRINT(e           , FIX_E                     , PRINT_TEST_e          ) \
PRINT(e_neg       , fix_neg(FIX_E)            , PRINT_TEST_e_neg      ) \
PRINT(pi          , FIX_PI                    , PRINT_TEST_pi         ) \
PRINT(pi_neg      , fix_neg(FIX_PI)           , PRINT_TEST_pi_neg     ) \
PRINT(tau         , FIX_TAU                   , PRINT_TEST_tau        ) \
PRINT(tau_neg     , fix_neg(FIX_TAU)          , PRINT_TEST_tau_neg    ) \
PRINT(ten         , FIXNUM(10,0)              , PRINT_TEST_ten        ) \
PRINT(ten_neg     , FIXNUM(-10,0)             , PRINT_TEST_ten_neg    ) \
PRINT(max         , FIX_MAX                   , PRINT_TEST_max        ) \
PRINT(min         , FIX_MIN                   , PRINT_TEST_min        ) \
PRINT(int_max     , FIXNUM(FIX_INT_MAX,0)     , PRINT_TEST_int_max    ) \
PRINT(int_max_neg , FIXNUM(-FIX_INT_MAX,0)    , PRINT_TEST_int_max_neg) \
PRINT(inf         , FIX_INF_POS               , PRINT_TEST_inf        ) \
PRINT(inf_neg     , FIX_INF_NEG               , PRINT_TEST_inf_neg    ) \
PRINT(nan         , FIX_NAN                   , PRINT_TEST_nan        )

PRINT_TESTS

//////////////////////////////////////////////////////////////////////////////



#undef TEST_HELPER
#define TEST_HELPER(name, code) cmocka_unit_test(name),

int main(int argc, char** argv) {

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(null_test_success),

    ROUND_TO_EVEN_TESTS
    FIXNUM_TESTS
    INTCONVERSION_TESTS
    CONVERT_DBL_TESTS
    EQ_TESTS
    ROUNDING_TESTS
    FLOOR_CEIL_TESTS
    CONSTANT_TESTS
    CMP_TESTS
    ADD_TESTS
    MUL_TESTS
    DIV_TESTS

    NEG_TESTS
    ABS_TESTS

    LN_TESTS
    LOG2_TESTS
    LOG10_TESTS

    EXP_TESTS
    SQRT_TESTS

    POW_TESTS

    //SIN_TESTS
    TRIG_TESTS

    PRINT_TESTS
  };

  int i = cmocka_run_group_tests(tests, NULL, NULL);

  return i;
}

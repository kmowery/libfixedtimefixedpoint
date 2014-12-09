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

static void null_test_success(void **state) {
  //(void) state;
}

#define CHECK_CONDITION(error_msg, condition, var1, var2) \
  if( !(condition) ) { \
    char b1[100], b2[100]; \
    fix_print(b1, var1); fix_print(b2, var2); \
    fail_msg( error_msg ": %s ("FIX_PRINTF_HEX") != %s ("FIX_PRINTF_HEX")", b1, var1, b2, var2); \
  }

#define CHECK_EQ(error_msg, var1, var2) \
  if( !fix_eq(var1, var2) ) { \
    char b1[100], b2[100]; \
    fix_print(b1, var1); fix_print(b2, var2); \
    fail_msg( error_msg ": %s ("FIX_PRINTF_HEX") != %s ("FIX_PRINTF_HEX")", b1, var1, b2, var2); \
  }
#define CHECK_EQ_NAN(error_msg, var1, var2) \
  if( !fix_eq_nan(var1, var2) ) { \
    char b1[100], b2[100]; \
    fix_print(b1, var1); fix_print(b2, var2); \
    fail_msg( error_msg ": %s ("FIX_PRINTF_HEX") != %s ("FIX_PRINTF_HEX")", b1, var1, b2, var2); \
  }

#define CHECK_EQ_VALUE(error_msg, var1, var2, value) \
  if( fix_eq(var1, var2) != value ) { \
    char b1[100], b2[100]; \
    fix_print(b1, var1); fix_print(b2, var2); \
    fail_msg( error_msg ": %s ("FIX_PRINTF_HEX") != %s ("FIX_PRINTF_HEX")", b1, var1, b2, var2); \
  }
#define CHECK_EQ_NAN_VALUE(error_msg, var1, var2, value) \
  if( fix_eq_nan(var1, var2) != value ) { \
    char b1[100], b2[100]; \
    fix_print(b1, var1); fix_print(b2, var2); \
    fail_msg( error_msg ": %s ("FIX_PRINTF_HEX") != %s ("FIX_PRINTF_HEX")", b1, var1, b2, var2); \
  }

#define CHECK_INT_EQUAL(error_msg, var1, var2) \
  if( !(var1 == var2) ) { \
    fail_msg( error_msg ": "FIX_PRINTF_DEC" (0x"FIX_PRINTF_HEX") != "FIX_PRINTF_DEC" (0x"FIX_PRINTF_HEX")", (fixed) var1, (fixed) var1, (fixed) var2, (fixed) var2); \
  }

#define CHECK_VALUE(error_msg, value, expected, fixed1, fixed2) \
  if( !(value == expected) ) { \
    char b1[100], b2[100]; \
    fix_print(b1, fixed1); fix_print(b2, fixed2); \
    fail_msg( error_msg ": %s ("FIX_PRINTF_HEX") != %s ("FIX_PRINTF_HEX") (value was "FIX_PRINTF_HEX")", b1, fixed1, b2, fixed2, (int64_t) value); \
  }

#define NaN nan("0")
#define SQRT_MAX ((fixed) sqrt((double) FIX_INT_MAX))

void p(fixed f) {
  char buf[100];

  fix_print(buf, f);
  printf("n: %s ("FIX_PRINTF_HEX")\n", buf, f);
}
void pl(fixed f) {
  char buf[100];

  fix_print(buf, f);
  printf("%s ("FIX_PRINTF_HEX")", buf, f);
}

void bounds(fixed f) {
  char buf_less[100];
  char buf[100];
  char buf_more[100];

  fixed less = f - 0x4;
  fixed more = f + 0x4;

  fix_print(buf_less, less);
  fix_print(buf, f);
  fix_print(buf_more, more);

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
  fixed expected = bits; \
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
    char b1[100]; \
    fix_print(b1, result); \
    fail_msg( #name " convert_to_double failed : %g (%s "FIX_PRINTF_HEX") != %g", d2, b1, result, locald); \
  } \
    char buf[100]; \
    char bitsbuf[100]; \
    fix_print(buf, result); \
    fix_print(bitsbuf, bits); \
    printf("Test passed: %g == %g == %s == %s\n", locald, d2, buf, bitsbuf); \
};)

#define CONVERT_DBL_TESTS                                                \
CONVERT_DBL(zero     , 0                       , FIX_ZERO)               \
CONVERT_DBL(one      , 1                       , FIXNUM(1    , 0))       \
CONVERT_DBL(one_neg  , -1                      , FIXNUM(-1   , 0))       \
CONVERT_DBL(two      , 2                       , FIXNUM(2    , 0))       \
CONVERT_DBL(two_neg  , -2                      , FIXNUM(-2   , 0))       \
CONVERT_DBL(many     , 1000.4                  , FIXNUM(1000 , 4))       \
CONVERT_DBL(many_neg , -1000.4                 , FIXNUM(-1000, 4))       \
CONVERT_DBL(frac     , 0.5342                  , FIXNUM(0    , 5342))    \
CONVERT_DBL(frac_neg , -0.5342                 , FIXNUM(-0   , 5342))    \
CONVERT_DBL(max      , (double) FIX_INT_MAX    , FIX_INF_POS)            \
CONVERT_DBL(max_neg  , -((double) FIX_INT_MAX) , FIXNUM(-FIX_INT_MAX,0)) \
CONVERT_DBL(inf_pos  , INFINITY                , FIX_INF_POS)            \
CONVERT_DBL(inf_neg  , -INFINITY               , FIX_INF_NEG)            \
CONVERT_DBL(nan      , nan("0")                , FIX_NAN)
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

#define unit_rounding(name) unit_test(rounding_##name)
#define TEST_ROUNDING(name, value, res_even, res_up, res_ceil, res_floor) \
TEST_HELPER(rounding_##name, { \
  fixed input = value; \
  /* Trust that FIXNUM is doing its job correctly. Therefore, if input is \
   * infinity, we can fix up the expected values below... */ \
  int64_t round_even  = FIX_ROUND_INT(input); \
  int64_t exp_even    = FIX_IS_INF_POS(input) ? INT_MAX : FIX_IS_INF_NEG(input) ? INT_MIN : (res_even); \
  CHECK_INT_EQUAL("round to even" , round_even  , exp_even); \
  int64_t round_up    = FIX_ROUND_UP_INT(input); \
  int64_t exp_up      = FIX_IS_INF_POS(input) ? INT_MAX : FIX_IS_INF_NEG(input) ? INT_MIN : (res_up); \
  CHECK_INT_EQUAL("round up"      , round_up    , exp_up); \
  int64_t round_ceil  = FIX_CEIL(input); \
  int64_t exp_ceil    = FIX_IS_INF_POS(input) ? INT_MAX : FIX_IS_INF_NEG(input) ? INT_MIN : (res_ceil); \
  CHECK_INT_EQUAL("ceiling"       , round_ceil  , exp_ceil); \
  int64_t round_floor = FIX_FLOOR(input); \
  int64_t exp_floor   = FIX_IS_INF_POS(input) ? INT_MAX : FIX_IS_INF_NEG(input) ? INT_MIN : (res_floor); \
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
    char b1[100], b2[100];
    fix_print(b1, temp);
    fix_print(b2, FIX_PI);

    if(fix_cmp(temp, limit) >= 0) {
      fail_msg( "Tau - Pi - Pi is %s ("FIX_PRINTF_HEX
              "), not near zero. Pi is %s ("FIX_PRINTF_HEX")",
              b1, temp, b2, FIX_PI);
    }
    printf( "Tau - Pi - Pi is %s ("FIX_PRINTF_HEX")\n", b1, temp);
  }
}
#define CONSTANT_TESTS unit_test(constants),

//////////////////////////////////////////////////////////////////////////////

#define TEST_CMP(name, op1, op2, result) \
TEST_HELPER(cmp_##name, { \
  fixed o1 = op1; \
  fixed o2 = op2; \
  int32_t cmp = fix_cmp(o1, o2); \
  int32_t expected = result; \
  CHECK_VALUE("cmp failed", cmp, expected, o1, o2); \
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
TEST_CMP(pos_nan         , FIXNUM(0   , 5)        , FIX_NAN             , 1)   \
TEST_CMP(neg_nan         , FIXNUM(-0  , 5)        , FIX_NAN             , 1)   \
TEST_CMP(inf_inf         , FIX_INF_POS             , FIX_INF_POS         , 0)  \
TEST_CMP(inf_pos         , FIX_INF_POS             , FIXNUM(0   , 5)    , 1)   \
TEST_CMP(inf_neg         , FIX_INF_POS             , FIXNUM(-0  , 5)    , 1)   \
TEST_CMP(inf_inf_neg     , FIX_INF_POS             , FIX_INF_NEG         , 1)  \
TEST_CMP(pos_inf         , FIXNUM(0   , 5)        , FIX_INF_POS         , -1)  \
TEST_CMP(neg_inf         , FIXNUM(-0  , 5)        , FIX_INF_POS         , -1)  \
TEST_CMP(pos_inf_neg     , FIXNUM(0   , 5)        , FIX_INF_NEG         ,  1)  \
TEST_CMP(neg_inf_neg     , FIXNUM(-0  , 5)        , FIX_INF_NEG         ,  1)  \
TEST_CMP(inf_neg_inf_pos , FIX_INF_NEG             , FIX_INF_POS         , -1) \
TEST_CMP(inf_neg_pos     , FIX_INF_NEG             , FIXNUM(0   , 5)    , -1)  \
TEST_CMP(inf_neg_neg     , FIX_INF_NEG             , FIXNUM(-0  , 5)    , -1)  \
TEST_CMP(pos_neg         , FIXNUM(17   , 3)         , FIXNUM(-24  , 5)   , 1)  \
TEST_CMP(neg_pos         , FIXNUM(-16  , 3)         , FIXNUM(24   , 5)   , -1)
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
MUL(overflow             , FIXNUM(FIX_INT_MAX-1,0) , FIXNUM(2,0)         , FIX_INF_POS)      \
MUL(tinyoverflow         , FIXNUM((SQRT_MAX+10),5) , FIXNUM((SQRT_MAX+10),5) , FIX_INF_POS)  \
MUL(tinyoverflow_neg     , FIXNUM((SQRT_MAX+10),5) , FIXNUM(-(SQRT_MAX+10),5) , FIX_INF_NEG) \
MUL(tinyoverflow_neg_neg , FIXNUM(-(SQRT_MAX+10),5), FIXNUM(-(SQRT_MAX+10),5) , FIX_INF_POS)
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

#define DIV_TESTS                                                                                \
DIV(one_one          , FIXNUM(1,0)           , FIXNUM(1,0)            ,FIXNUM(1,0))              \
DIV(fifteen_one      , FIXNUM(15,0)          , FIXNUM(1,0)            ,FIXNUM(15,0))             \
DIV(sixteen_two      , FIXNUM(16,0)          , FIXNUM(2,0)            ,FIXNUM(8,0))              \
DIV(fifteen_nthree   , FIXNUM(15,0)          , FIXNUM(-3,0)           ,FIXNUM(-5,0))             \
DIV(nfifteen_nthree  , FIXNUM(-15,0)         , FIXNUM(-3,0)           ,FIXNUM(5,0))              \
DIV(nfifteen_three   , FIXNUM(-15,0)         , FIXNUM(3,0)            ,FIXNUM(-5,0))             \
DIV(fifteen_frac5    , FIXNUM(15,0)          , FIXNUM(0,5)            ,FIXNUM(30,0))             \
DIV(overflow         , FIXNUM(8192,0)        , FIXNUM(0,1)            ,FIX_INF_POS)              \
DIV(one_zero         , FIXNUM(1,0)           , FIXNUM(0,0)            ,FIX_NAN)                  \
DIV(inf_zero         , FIX_INF_POS           , FIXNUM(0,0)            ,FIX_NAN)                  \
DIV(zero_inf         , FIXNUM(0,0)           , FIX_INF_POS            ,FIX_INF_POS)              \
DIV(inf_neg          , FIX_INF_POS           , FIXNUM(-10,0)          ,FIX_INF_NEG)              \
DIV(ninf_neg         , FIX_INF_NEG           , FIXNUM(-10,0)          ,FIX_INF_POS)              \
DIV(neg_inf          , FIXNUM(-10,0)         , FIX_INF_POS            ,FIX_INF_NEG)              \
DIV(neg_ninf         , FIXNUM(-10,0)         , FIX_INF_NEG            ,FIX_INF_POS)              \
DIV(pos_nan          , FIXNUM(10,0)          , FIX_NAN                ,FIX_NAN)                  \
DIV(neg_nan          , FIXNUM(-10,0)         , FIX_NAN                ,FIX_NAN)                  \
DIV(inf_nan          , FIX_INF_POS           , FIX_NAN                ,FIX_NAN)                  \
DIV(ninf_nan         , FIX_INF_NEG           , FIX_NAN                ,FIX_NAN)                  \
DIV(nan_pos          , FIX_NAN               , FIXNUM(10,0)           ,FIX_NAN)                  \
DIV(nan_neg          , FIX_NAN               , FIXNUM(-10,0)          ,FIX_NAN)                  \
DIV(nan_inf          , FIX_NAN               , FIX_INF_POS            ,FIX_NAN)                  \
DIV(nan_ninf         , FIX_NAN               , FIX_INF_NEG            ,FIX_NAN)                  \
DIV(regression1      , 0xf0000000            , 0x000022e8             ,FIX_INF_NEG)              \
DIV(regression2      , 0xf0000000            , fix_neg(0x000022e8)    ,FIX_INF_POS)              \
DIV(regression3      , 0xf0000000            , 0x00004470             ,FIXNUM(-15321,658447265))
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
NEG(zero,    FIXNUM(0,0),  FIXNUM(0,0))  \
NEG(one,     FIXNUM(1,0),  FIXNUM(-1,0)) \
NEG(one_neg, FIXNUM(-1,0), FIXNUM(1,0))  \
NEG(inf,     FIX_INF_POS,  FIX_INF_NEG)  \
NEG(inf_neg, FIX_INF_NEG,  FIX_INF_POS)  \
NEG(nan,     FIX_NAN,      FIX_NAN)
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
ABS(zero         , FIXNUM(0,0)         , FIXNUM(0,0)) \
ABS(one          , FIXNUM(1,0)         , FIXNUM(1,0)) \
ABS(one_neg      , FIXNUM(-1,0)        , FIXNUM(1,0)) \
ABS(two          , FIXNUM(2,0)         , FIXNUM(2,0)) \
ABS(inf          , FIX_INF_POS         , FIX_INF_POS) \
ABS(inf_neg      , FIX_INF_NEG         , FIX_INF_POS) \
ABS(nan          , FIX_NAN             , FIX_NAN)
ABS_TESTS

//////////////////////////////////////////////////////////////////////////////

#define LN(name, op1, result) \
TEST_HELPER(ln_##name, { \
  fixed o1 = op1; \
  fixed ln = fix_ln(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, ln, expected); \
};)

#define LN_TESTS \
LN(zero   , 0x0               , FIX_INF_NEG) \
LN(0xf0   , 0xf0              , FIXNUM(-6,3028564453)) /* -6.302863146 */ \
LN(0x80   , 0x80              , 0xfff22318) \
LN(0xa0   , 0xa0              , 0xfff29358) \
LN(two    , FIXNUM(2,0)       , FIXNUM(0 , 69314718)) \
LN(one    , FIXNUM(1,0)       , FIXNUM(0 , 0)) \
LN(half   , FIXNUM(0,5)       , FIXNUM(-0,693145751953)) \
LN(e      , FIX_E             , FIXNUM(0,9954223632) ) /* not quite FIXNUM(1 , 0), but close */ \
LN(ten    , FIXNUM(10,0)      , FIXNUM(2,2986755371) ) /* not quite FIXNUM(2 , 30258509), but close */ \
LN(max    , FIX_MAX           , FIXNUM(9,7040405273 ))  /* not quite 9.70406052597 */ \
LN(inf    , FIX_INF_POS       , FIX_INF_POS) \
LN(neg    , FIXNUM(-1,0)      , FIX_NAN) \
LN(nan    , FIX_NAN           , FIX_NAN)
LN_TESTS

//////////////////////////////////////////////////////////////////////////////

#define LOG2(name, op1, result) \
TEST_HELPER(log2_##name, { \
  fixed o1 = op1; \
  fixed log2 = fix_log2(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, log2, expected); \
};)

#define LOG2_TESTS \
LOG2(zero   , 0x0               , FIX_INF_NEG) \
LOG2(0xf0   , 0xf0              , FIXNUM(-9,093139648)) /*9.093139648*/ \
LOG2(0x80   , 0x80              , FIXNUM(-10,0)) \
LOG2(0xa0   , 0xa0              , FIXNUM(-9,6824035644)) \
LOG2(one    , FIXNUM(1,0)       , FIXNUM(0 , 0)) \
LOG2(two    , FIXNUM(2,0)       , FIXNUM(1 , 0)) \
LOG2(e      , FIX_E             , FIXNUM(1,43826293945) ) /* not quite 1.4426 but close */ \
LOG2(ten    , FIXNUM(10,0)      , FIXNUM(3,31759643554) ) /* not quite 3.3219 but close */ \
LOG2(63     , FIXNUM(63,0)      , FIXNUM(5,977355957 )) /* 5.9772799236 */ \
LOG2(64     , FIXNUM(64,0)      , FIXNUM(6, 0) ) \
LOG2(64_5   , FIXNUM(64,5)      , FIXNUM(6, 01116943359)) /* 6.011227255423254 */ \
LOG2(max    , FIX_MAX           , FIXNUM(13,9999999) ) /* not quite 3.3219 but close */ \
LOG2(min    , FIX_MIN           , FIX_NAN ) \
LOG2(inf    , FIX_INF_POS       , FIX_INF_POS) \
LOG2(neg    , FIXNUM(-1,0)      , FIX_NAN) \
LOG2(nan    , FIX_NAN           , FIX_NAN)
LOG2_TESTS

//////////////////////////////////////////////////////////////////////////////

#define LOG10(name, op1, result) \
TEST_HELPER(log10_##name, { \
  fixed o1 = op1; \
  fixed log10 = fix_log10(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, log10, expected); \
};)

#define LOG10_TESTS \
LOG10(zero   , 0x0               , FIX_INF_NEG) \
LOG10(one    , FIXNUM(1,0)       , FIXNUM(-0 , 000061035)) /* almost 0. */ \
LOG10(two    , FIXNUM(2,0)       , FIXNUM(0 , 30096435))   /* almost 0.30102999 */ \
LOG10(e      , FIX_E             , FIXNUM(0 , 434265136))  /* 0.43429448 */ \
LOG10(ten    , FIXNUM(10,0)      , FIXNUM(1 , 0)) \
LOG10(fifteen, FIXNUM(15,0)      , FIXNUM(1 , 1760253906 )) /* 1.176091259 */ \
LOG10(63     , FIXNUM(63,0)      , FIXNUM(1 , 7992553710 )) /* 1.79934054945 */ \
LOG10(64     , FIXNUM(64,0)      , FIXNUM(1 , 8060913085 )) /* 1.80617997398 */ \
LOG10(64_5   , FIXNUM(64,5)      , FIXNUM(1 , 8094787597 )) /* 1.80955971463 */ \
LOG10(max    , FIX_MAX           , FIXNUM(4 , 214294433)) /* 4.21441993848 */ \
LOG10(inf    , FIX_INF_POS       , FIX_INF_POS) \
LOG10(neg    , FIXNUM(-1,0)      , FIX_NAN) \
LOG10(nan    , FIX_NAN           , FIX_NAN)
LOG10_TESTS

//////////////////////////////////////////////////////////////////////////////

#define EXP(name, op1, result) \
TEST_HELPER(exp_##name, { \
  fixed o1 = op1; \
  fixed exp = fix_exp(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, exp, expected); \
};)

#define EXP_TESTS \
EXP(zero   , FIX_ZERO          , FIXNUM(1,0)) \
EXP(one    , FIXNUM(1,0)       , FIXNUM(2,71826171875))     /* not exactly our E, but close */ \
EXP(two    , FIXNUM(2,0)       , FIXNUM(7,38897705078))     /* not 7,3890560989 */ \
EXP(e      , FIX_E             , FIXNUM(15,154296875))     /* not 15,15426224147 */ \
EXP(ten    , FIXNUM(10,0)      , FIX_INF_POS) \
EXP(one_neg, FIXNUM(-1,0)      , FIXNUM(0,3678588867)) \
EXP(two_neg, FIXNUM(-2,0)      , FIXNUM(0,1353149414)) \
EXP(e_neg  , fix_neg(FIX_E)    , FIXNUM(0,065979003906))     /* not 0.0659880358 */ \
EXP(ten_neg, FIXNUM(-10,0)     , FIXNUM(0,0000453999)) \
EXP(neg_many,FIXNUM(-128,0)    , FIX_ZERO) \
EXP(max    , FIX_MAX           , FIX_INF_POS) \
EXP(inf    , FIX_INF_POS       , FIX_INF_POS) \
EXP(inf_neg, FIX_INF_NEG       , FIX_ZERO) \
EXP(nan    , FIX_NAN           , FIX_NAN) \
EXP(regress, FIXNUM(-1,3862915039), FIXNUM(0,25))
EXP_TESTS

//////////////////////////////////////////////////////////////////////////////

#define SQRT(name, op1, result) \
TEST_HELPER(sqrt_##name, { \
  fixed o1 = op1; \
  fixed sqrt = fix_sqrt(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, sqrt, expected); \
};)

#define SQRT_TESTS \
SQRT(zero   , 0x0               , FIXNUM(0,0))  \
SQRT(half   , FIXNUM(0,5)       , FIXNUM(0,707092))  \
SQRT(one    , FIXNUM(1,0)       , FIXNUM(1,0))  \
SQRT(two    , FIXNUM(2,0)       , FIXNUM(1,414215))  \
SQRT(e      , FIX_E             , FIXNUM(1,648712) )  \
SQRT(ten    , FIXNUM(10,0)      , FIXNUM(3,162262) )  /* not precisely sqrt(10) but so close  */ \
SQRT(big    , FIXNUM(10000,5345), FIXNUM(100,002655) )  \
SQRT(max    , FIX_MAX           , FIXNUM(128,0) )  \
SQRT(inf    , FIX_INF_POS       , FIX_INF_POS)  \
SQRT(neg    , FIXNUM(-1,0)      , FIX_NAN)  \
SQRT(nan    , FIX_NAN           , FIX_NAN)
SQRT_TESTS

//////////////////////////////////////////////////////////////////////////////

#define POW(name, op1, op2, result) \
TEST_HELPER(pow_##name, { \
  fixed o1 = op1; \
  fixed o2 = op2; \
  fixed powresult = fix_pow(o1, o2); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, powresult, expected); \
};)

#define POW_TESTS                                                               \
POW(zero_zero       , FIX_ZERO          , FIX_ZERO,      FIX_ZERO)              \
POW(zero_one        , FIX_ZERO          , FIXNUM(1,0),   FIX_ZERO)              \
POW(half_zero       , FIXNUM(0,5)       , FIX_ZERO   ,   FIXNUM(1,0) )          \
POW(half_square     , FIXNUM(0,5)       , FIXNUM(2,0),   FIXNUM(0,25) ) \
POW(half_nsquare    , FIXNUM(0,5)       , FIXNUM(-2,0),  FIXNUM(3,99996948242) ) \
POW(half_neight     , FIXNUM(0,5)       , FIXNUM(-8,0),  FIXNUM(255,994140625) ) \
POW(one2            , FIXNUM(1,0)       , FIXNUM(2,0),   FIXNUM(1,0))           \
POW(one4            , FIXNUM(1,0)       , FIXNUM(4,0),   FIXNUM(1,0))           \
POW(ten_square      , FIXNUM(10,0)      , FIXNUM(2,0),   FIXNUM(99,2214660644)) \
POW(ten_cubed       , FIXNUM(10,0)      , FIXNUM(3,0),   FIXNUM(988,31304931640)) \
POW(neg_one2        , FIXNUM(-1,0)      , FIXNUM(2,0),   FIXNUM(1,0))           \
POW(neg_one3        , FIXNUM(-1,0)      , FIXNUM(3,0),   FIXNUM(-1,0))          \
POW(neg_two2        , FIXNUM(-2,0)      , FIXNUM(2,0),   FIXNUM(3,999969482421))   \
POW(neg_two3        , FIXNUM(-2,0)      , FIXNUM(3,0),   FIXNUM(-7,9999084472)) \
POW(neg_two_neg2    , FIXNUM(-2,0)      , FIXNUM(-2,0),  FIXNUM(0,25))  \
POW(neg_two_nan     , FIXNUM(-2,0)      , FIXNUM(2,5),   FIX_NAN)               \
POW(neg_pos_overflow, FIXNUM(-2,0)      , FIXNUM(128,0), FIX_INF_POS)           \
POW(neg_overflow    , FIXNUM(-2,0)      , FIXNUM(127,0), FIX_INF_NEG)           \
POW(ten_overflow    , FIXNUM(10,0)      , FIXNUM(100,0), FIX_INF_POS)           \
POW(inf_not         , FIX_INF_POS       , FIXNUM(1,0),   FIX_INF_POS)           \
POW(not_inf         , FIXNUM(2,0)       , FIX_INF_POS,   FIX_INF_POS)           \
POW(not_inf_neg     , FIXNUM(2,0)       , FIX_INF_NEG,   FIX_ZERO)              \
POW(nan_not         , FIX_NAN           , FIXNUM(1,0),   FIX_NAN    )           \
POW(not_nan         , FIXNUM(2,0)       , FIX_NAN    ,   FIX_NAN    )
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


#define SIN(name, op1, result) \
TEST_HELPER(sin_##name, { \
  fixed o1 = op1; \
  fixed sin = fix_sin_fast(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, sin, expected); \
};)

#define SIN_TESTS                                                                         \
SIN(zero       , FIX_ZERO                                           , 0)                  \
SIN(pi_2  , fix_div(FIX_PI , FIXNUM(2 , 0))                    , 0x1fffc)                 \
SIN(pi    , FIX_PI                                             , 0xfffffffc)              \
SIN(pi3_2 , fix_div(fix_mul(FIX_PI, FIXNUM(3,0)), FIXNUM(2,0)) , 0xfffe0000)              \
SIN(pi2   , fix_mul(FIX_PI, FIXNUM(2,0))                       , 0)                       \
SIN(pi5_2 , fix_div(fix_mul(FIX_PI, FIXNUM(5,0)), FIXNUM(2,0)) , 0x1fffc)                 \
SIN(pi3   , fix_mul(FIX_PI, FIXNUM(3,0))                       , 0xfffffffc)              \
SIN(pi7_2 , fix_div(fix_mul(FIX_PI, FIXNUM(7,0)), FIXNUM(2,0)) , 0xfffe0000)              \
SIN(pi4   , fix_mul(FIX_PI, FIXNUM(4,0))                       , 0x4)                     \
                                                                                          \
SIN(neg_pi_2  , fix_neg(fix_div(FIX_PI , FIXNUM(2 , 0)))                    , 0xfffe0000) \
SIN(neg_pi    , fix_neg(FIX_PI)                                             , 0x0)        \
SIN(neg_pi3_2 , fix_neg(fix_div(fix_mul(FIX_PI, FIXNUM(3,0)), FIXNUM(2,0))) , 0x1fffc)    \
SIN(neg_pi2   , fix_neg(fix_mul(FIX_PI, FIXNUM(2,0)))                       , 0xfffffffc) \
                                                                                          \
SIN(inf_pos   , FIX_INF_POS                          , FIX_INF_POS)                       \
SIN(inf_neg   , FIX_INF_NEG                          , FIX_INF_NEG)                       \
SIN(nan       , FIX_NAN                              , FIX_NAN)
SIN_TESTS

//////////////////////////////////////////////////////////////////////////////

#define TRIG(name, op1, sinx, cosx, tanx) \
TEST_HELPER(trig_##name, { \
  fixed o1 = op1; \
  fixed sin = fix_sin(o1); \
  CHECK_EQ_NAN(#name " sin", sin, sinx); \
  fixed cos = fix_cos(o1); \
  CHECK_EQ_NAN(#name " cos", cos, cosx); \
  fixed tan = fix_tan(o1); \
  CHECK_EQ_NAN(#name " tan", tan, tanx); \
};)

#define n3_2 fix_div(FIXNUM(3,0), FIXNUM(2,0))

// Note that tan is poorly defined near pi/2 + n*pi. It's either positive
// infinity or negative infinity, with very little separating them.

#define TRIG_TESTS \
TRIG(zero  , FIX_ZERO                          , FIX_ZERO             , FIXNUM(1,0),            FIX_ZERO )  \
TRIG(pi_2  , fix_div(FIX_PI , FIXNUM(2,0))     , FIXNUM(1,0)          , FIX_EPSILON_NEG,        FIX_INF_NEG )  \
TRIG(pi    , FIX_PI                            , fix_neg(FIX_EPSILON) , FIXNUM(-1,0),           FIX_ZERO )  \
TRIG(pi3_2 , fix_mul(FIX_PI, n3_2)             , FIXNUM(-1,0)         , FIX_ZERO,               FIX_INF_NEG)  \
TRIG(pi2   , fix_mul(FIX_PI, FIXNUM(2,0))      , FIX_ZERO             , FIXNUM(1,0),            FIX_ZERO )  \
\
TRIG(pi5_2 , fix_div(fix_mul(FIX_PI, FIXNUM(5,0)), FIXNUM(2,0)) , \
             FIXNUM(1,0), FIX_EPSILON_NEG, FIX_INF_NEG)  \
TRIG(pi3   , fix_mul(FIX_PI, FIXNUM(3,0))                       , \
             FIX_EPSILON_NEG, FIXNUM(-1,0), FIX_EPSILON )  \
TRIG(pi7_2 , fix_div(fix_mul(FIX_PI, FIXNUM(7,0)), FIXNUM(2,0)) , \
             FIXNUM(-1,0), FIX_EPSILON, FIX_INF_NEG)  \
TRIG(pi4   , fix_mul(FIX_PI, FIXNUM(4,0)), \
             FIX_EPSILON, FIXNUM(1,0), FIX_EPSILON )  \
\
TRIG(neg_pi_2  , fix_neg(fix_div(FIX_PI , FIXNUM(2 , 0))), \
                 FIXNUM(-1,0)         , FIX_EPSILON_NEG,               FIX_INF_POS)  \
TRIG(neg_pi    , fix_neg(FIX_PI), \
                 FIX_ZERO, FIXNUM(-1,0), FIX_ZERO )  \
TRIG(neg_pi3_2 , fix_neg(fix_div(fix_mul(FIX_PI, FIXNUM(3,0)), FIXNUM(2,0))) , \
                 FIXNUM(1,0)          , FIX_ZERO,   FIX_INF_POS )  \
TRIG(neg_pi2   , fix_neg(fix_mul(FIX_PI, FIXNUM(2,0))), \
                 FIX_EPSILON_NEG, FIXNUM(1,0), FIX_ZERO )  \
\
TRIG(q1       , FIXNUM(1,0),                  FIXNUM(0,8414709848), FIXNUM(0,54028320) , FIXNUM(1,5574077246))  \
TRIG(q2      ,  fix_add(fix_div(FIX_PI, FIXNUM(2,0)), FIXNUM(1,0)), \
                                              FIXNUM(0,54028320) , FIXNUM(-0,84149169) , FIXNUM(-0,64209261))  \
TRIG(q3      ,  fix_add(FIX_PI, FIXNUM(0,5)), FIXNUM(-0,47942553), FIXNUM(-0,877582561), FIXNUM(0,5463256835))  \
TRIG(q4      ,  FIXNUM(-0,5),                 FIXNUM(-0,47942553), FIXNUM(0,877563476) , FIXNUM(-0,546302489))  \
\
TRIG(inf_pos   , FIX_INF_POS, FIX_NAN, FIX_NAN, FIX_NAN)  \
TRIG(inf_neg   , FIX_INF_NEG, FIX_NAN, FIX_NAN, FIX_NAN)  \
TRIG(nan       , FIX_NAN, FIX_NAN, FIX_NAN, FIX_NAN)
TRIG_TESTS

//////////////////////////////////////////////////////////////////////////////

#define PRINT(name, op1, result) \
TEST_HELPER(print_##name, { \
  fixed o1 = op1; \
  char buf[23]; \
  fix_print(buf, o1); \
  char* expected = result; \
  if(strcmp(buf, expected))  { \
    printf("Strings not equal: '%s' != '%s'\n", buf, expected); \
    fix_print(buf, o1); \
    printf("I should have expected: '%s'\n", buf); \
  } \
  assert_memory_equal(buf, expected, 23); \
};)

#define PRINT_TESTS \
PRINT(zero    , 0x0               , " 00000.000000000000000")  \
PRINT(half    , FIXNUM(0,5)       , " 00000.500000000000000")  \
PRINT(half_neg, FIXNUM(-0,5)      , "-00000.500000000000000")  \
PRINT(one     , FIXNUM(1,0)       , " 00001.000000000000000")  \
PRINT(one_neg , FIXNUM(-1,0)      , "-00001.000000000000000")  \
PRINT(two     , FIXNUM(2,0)       , " 00002.000000000000000")  \
PRINT(e       , FIX_E             , " 00002.718292236328125")  \
PRINT(e_neg   , fix_neg(FIX_E)    , "-00002.718292236328125")  \
PRINT(ten     , FIXNUM(10,0)      , " 00010.000000000000000")  \
PRINT(big     , FIXNUM(10000,5345), " 10000.534484863281250")  \
PRINT(max     , FIX_MAX           , " 16383.999969482421875")  \
PRINT(min     , FIX_MIN           , "-16384.000000000000000")  \
PRINT(inf     , FIX_INF_POS       , " Inf                  ")  \
PRINT(inf_neg , FIX_INF_NEG       , "-Inf                  ")  \
PRINT(nan     , FIX_NAN           , " NaN                  ")
PRINT_TESTS

//////////////////////////////////////////////////////////////////////////////



#undef TEST_HELPER
#define TEST_HELPER(name, code) unit_test(name),

int main(int argc, char** argv) {

  const UnitTest tests[] = {
    unit_test(null_test_success),

    ROUND_TO_EVEN_TESTS
    FIXNUM_TESTS
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

    SIN_TESTS
    TRIG_TESTS

    PRINT_TESTS
  };

  return run_tests(tests);
}

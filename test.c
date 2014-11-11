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

#define CHECK_EQ(error_msg, var1, var2) \
  if( !FIX_EQ(var1, var2) ) { \
    char b1[100], b2[100]; \
    fix_print(b1, var1); fix_print(b2, var2); \
    fail_msg( error_msg ": %s (%08x) != %s (%08x)", b1, var1, b2, var2); \
  }
#define CHECK_EQ_NAN(error_msg, var1, var2) \
  if( !FIX_EQ_NAN(var1, var2) ) { \
    char b1[100], b2[100]; \
    fix_print(b1, var1); fix_print(b2, var2); \
    fail_msg( error_msg ": %s (%08x) != %s (%08x)", b1, var1, b2, var2); \
  }

#define CHECK_INT_EQUAL(error_msg, var1, var2) \
  if( !(var1 == var2) ) { \
    fail_msg( error_msg ": %d (0x%x) != %d (0x%x)", var1, var1, var2, var2); \
  }

#define NaN nan("0")

void p(fixed f) {
  char buf[40];

  fix_print(buf, f);
  printf("n: %s (%08x)\n", buf, f);
}
void pl(fixed f) {
  char buf[40];

  fix_print(buf, f);
  printf("%s (%08x)", buf, f);
}

void bounds(fixed f) {
  char buf_less[40];
  char buf[40];
  char buf_more[40];

  fixed less = f - 0x4;
  fixed more = f + 0x4;

  fix_print(buf_less, less);
  fix_print(buf, f);
  fix_print(buf_more, more);

  printf("%s (0x%08x) | %s (0x%08x) | %s (0x%08x)\n", buf_less, less, buf, f, buf_more, more);
}


#define unit_fixint_dbl(name) unit_test(convert_dbl_##name)
#define CONVERT_DBL(name, d, bits) static void convert_dbl_##name(void **state) { \
  fixed f = bits; \
  fixed g = fix_convert_double(d); \
  CHECK_EQ_NAN(#name, f, g); \
}

CONVERT_DBL(zero     , 0         , 0x00000);
CONVERT_DBL(one      , 1         , 0x20000);
CONVERT_DBL(one_neg  , -1        , 0xfffe0000);
CONVERT_DBL(two      , 2         , 0x40000);
CONVERT_DBL(two_neg  , -2        , 0xfffc0000);
CONVERT_DBL(many     , 1000.4    , 0x7d0cccc);
CONVERT_DBL(many_neg , -1000.4   , 0xf82f3334);
CONVERT_DBL(frac     , 0.5342    , 0x11180);
CONVERT_DBL(frac_neg , -0.5342   , 0xfffeee80);
CONVERT_DBL(inf_pos  , INFINITY  , FIX_INF_POS);
CONVERT_DBL(inf_neg  , -INFINITY , FIX_INF_NEG);
CONVERT_DBL(nan      , nan("0")  , FIX_NAN);
/* TODO: add tests for round-to-even */

#define unit_fixnum(name) unit_test(fixnum_##name)
#define TEST_FIXNUM(name, z, frac, bits) static void fixnum_##name(void **state) { \
  fixed expected = bits; \
  fixed g = FIXNUM(z, frac); \
  CHECK_EQ("fixnum", g, expected); \
}

TEST_FIXNUM(zero     , 0     , 0    , 0x0);
TEST_FIXNUM(one      , 1     , 0    , 0x20000);
TEST_FIXNUM(one_neg  , -1    , 0    , 0xfffe0000);
TEST_FIXNUM(two      , 2     , 0    , 0x40000);
TEST_FIXNUM(two_neg  , -2    , 0    , 0xfffc0000);
TEST_FIXNUM(many     , 1000  , 4    , 0x7d0cccc);
TEST_FIXNUM(many_neg , -1000 , 4    , 0xf82f3334);
TEST_FIXNUM(frac     , 0     , 5342 , 0x11184);
TEST_FIXNUM(frac_neg , -0    , 5342 , 0xfffeee7c);

TEST_FIXNUM(regress0 ,     0 , 00932 ,        0x000004c4);
TEST_FIXNUM(regress1 ,   100 , 002655,        0x00c8015c);
TEST_FIXNUM(regress12,   100 , 0026550292968, 0x00c8015c);
TEST_FIXNUM(regress2 ,     1 , 4142150878906, 0x0002d414);
TEST_FIXNUM(regress3 ,     1 , 6487121582031, 0x00034c24);
TEST_FIXNUM(regress4 ,     1 , 9999999999999, 0x00040000);

#define unit_eq(name) unit_test(equal_##name)
#define TEST_EQ(name, op1, op2, value, valuenan) static void equal_##name(void **state) { \
  fixed o1 = op1; \
  fixed o2 = op2; \
  if( FIX_EQ(o1, o2) != value ) { \
    char b1[100], b2[100]; \
    fix_print(b1, o1); fix_print(b2, o2); \
    fail_msg( "values not right: %s (%x) %s %s (%x)", b1, o1, value ? "!=" : "==", b2, o2); \
  } \
  if( FIX_EQ_NAN(o1, o2) != valuenan ) { \
    char b1[100], b2[100]; \
    fix_print(b1, o1); fix_print(b2, o2); \
    fail_msg( "NaN values not right: %s (%x) %s %s (%x)", b1, o1, value ? "!=" : "==", b2, o2); \
  } \
}

TEST_EQ(zero    , 0         , 0         , 1 , 1);
TEST_EQ(frac    , 0x11180   , 0x11184   , 0 , 0);
TEST_EQ(nan     , FIX_NAN     , FIX_NAN     , 0 , 1);
TEST_EQ(inf     , FIX_INF_POS , FIX_INF_POS , 1 , 1);
TEST_EQ(inf_neg , FIX_INF_NEG , FIX_INF_NEG , 1 , 1);


#define unit_round_to_even(name) unit_test(round_to_even_##name)
#define TEST_ROUND_TO_EVEN(name, value, shift, result) static void round_to_even_##name(void **state) { \
  int32_t rounded = ROUND_TO_EVEN(value, shift); \
  int32_t expected = result; \
  CHECK_INT_EQUAL("round_to_even", rounded, expected); \
}

TEST_ROUND_TO_EVEN(zero       , 0x00 , 0x3 , 0x0);
TEST_ROUND_TO_EVEN(zero_plus  , 0x01 , 0x3 , 0x0);
TEST_ROUND_TO_EVEN(one        , 0x02 , 0x3 , 0x0);
TEST_ROUND_TO_EVEN(one_plus   , 0x03 , 0x3 , 0x0);
TEST_ROUND_TO_EVEN(two        , 0x04 , 0x3 , 0x0);  /* 0.5 -> 0 */
TEST_ROUND_TO_EVEN(two_plus   , 0x05 , 0x3 , 0x1);  /* 0.55 -> 1 */
TEST_ROUND_TO_EVEN(three      , 0x06 , 0x3 , 0x1);
TEST_ROUND_TO_EVEN(three_plus , 0x07 , 0x3 , 0x1);
TEST_ROUND_TO_EVEN(four       , 0x08 , 0x3 , 0x1);  /* 1 -> 1 */
TEST_ROUND_TO_EVEN(four_plus  , 0x09 , 0x3 , 0x1);
TEST_ROUND_TO_EVEN(five       , 0x0a , 0x3 , 0x1);
TEST_ROUND_TO_EVEN(five_plus  , 0x0b , 0x3 , 0x1);
TEST_ROUND_TO_EVEN(six        , 0x0c , 0x3 , 0x2);  /* 1.5 -> 2 */
TEST_ROUND_TO_EVEN(six_plus   , 0x0d , 0x3 , 0x2);
TEST_ROUND_TO_EVEN(seven      , 0x0e , 0x3 , 0x2);
TEST_ROUND_TO_EVEN(seven_plus , 0x0f , 0x3 , 0x2);
TEST_ROUND_TO_EVEN(eight      , 0x10 , 0x3 , 0x2);
TEST_ROUND_TO_EVEN(eight_plus , 0x10 , 0x3 , 0x2);

#define unit_rounding(name) unit_test(rounding_##name)
#define TEST_ROUNDING_CUST(name, value, res_even, res_up, res_ceil, res_floor) static void rounding_##name(void **state) { \
  fixed input = value; \
  int32_t round_even  = FIX_ROUND_INT(input); \
  int32_t round_up    = FIX_ROUND_UP_INT(input); \
  int32_t round_ceil  = FIX_CEIL(input); \
  int32_t round_floor = FIX_FLOOR(input); \
  int32_t exp_even    = res_even; \
  int32_t exp_up      = res_up; \
  int32_t exp_ceil    = res_ceil; \
  int32_t exp_floor   = res_floor; \
  CHECK_INT_EQUAL("round to even" , round_even  , exp_even); \
  CHECK_INT_EQUAL("round up"      , round_up    , exp_up); \
  CHECK_INT_EQUAL("ceiling"       , round_ceil  , exp_ceil); \
  CHECK_INT_EQUAL("floor"         , round_floor , exp_floor); \
}
#define TEST_ROUNDING(name, value, res_even, res_up, res_ceil, res_floor)  \
 TEST_ROUNDING_CUST(name, fix_convert_double(value), res_even, res_up, res_ceil, res_floor)

/*            Name       Value       Even      Up        Ceil      Down */
TEST_ROUNDING(zero     , 0         , 0       , 0       , 0       , 0);
TEST_ROUNDING(one      , 1         , 1       , 1       , 1       , 1);
TEST_ROUNDING(one3     , 1.3       , 1       , 1       , 2       , 1);
TEST_ROUNDING(one5     , 1.5       , 2       , 2       , 2       , 1);
TEST_ROUNDING(one7     , 1.7       , 2       , 2       , 2       , 1);
TEST_ROUNDING(two      , 2         , 2       , 2       , 2       , 2);
TEST_ROUNDING(two3     , 2.3       , 2       , 2       , 3       , 2);
TEST_ROUNDING(two5     , 2.5       , 2       , 3       , 3       , 2);
TEST_ROUNDING(two7     , 2.7       , 3       , 3       , 3       , 2);
TEST_ROUNDING(one_neg  , -1        , -1      , -1      , -1      , -1);
TEST_ROUNDING(one3_neg , -1.3      , -1      , -1      , -1      , -2);
TEST_ROUNDING(one5_neg , -1.5      , -2      , -1      , -1      , -2);
TEST_ROUNDING(two3_neg , -2.3      , -2      , -2      , -2      , -3);
TEST_ROUNDING(two5_neg , -2.5      , -2      , -2      , -2      , -3);
TEST_ROUNDING_CUST(max , FIX_MAX   , 16384   , 16384   , 16384   , 16383);
TEST_ROUNDING_CUST(min , FIX_MIN   ,-16384   ,-16384   ,-16384   ,-16384);
TEST_ROUNDING(nan      , NaN       , 0       , 0       , 0       , 0);
TEST_ROUNDING(inf      , INFINITY  , INT_MAX , INT_MAX , INT_MAX , INT_MAX);
TEST_ROUNDING(inf_neg  , -INFINITY , INT_MIN , INT_MIN , INT_MIN , INT_MIN);

#define unit_floor_ceil(name) unit_test(floor_ceil_##name)
#define FLOOR_CEIL_CUST(name, value, floor_result, ceil_result) static void floor_ceil_##name(void **state) { \
  fixed input = value; \
  fixed floor = fix_floor(input); \
  fixed floor_expected = floor_result; \
  CHECK_EQ_NAN("floor "#name, floor, floor_expected); \
  fixed ceil = fix_ceil(input); \
  fixed ceil_expected = ceil_result; \
  CHECK_EQ_NAN("ceil "#name, ceil, ceil_expected); \
}
FLOOR_CEIL_CUST(zero     , FIXNUM(0  , 0)  , FIXNUM(0  , 0)  , FIXNUM(0  , 0));
FLOOR_CEIL_CUST(half     , FIXNUM(0  , 5)  , FIXNUM(0  , 0)  , FIXNUM(1  , 0));
FLOOR_CEIL_CUST(half_neg , FIXNUM(-0 , 5)  , FIXNUM(-1 , 0)  , FIXNUM(0  , 0));
FLOOR_CEIL_CUST(one      , FIXNUM(1  , 0)  , FIXNUM(1  , 0)  , FIXNUM(1  , 0));
FLOOR_CEIL_CUST(one_neg  , FIXNUM(-1 , 0)  , FIXNUM(-1 , 0)  , FIXNUM(-1 , 0));
FLOOR_CEIL_CUST(pi       , FIX_PI          , FIXNUM(3  , 0)  , FIXNUM(4 , 0));
FLOOR_CEIL_CUST(pi_neg   , fix_neg(FIX_PI) , FIXNUM(-4 , 0)  , FIXNUM(-3 , 0));
FLOOR_CEIL_CUST(max      , FIX_MAX         , FIXNUM(16383, 0), FIX_INF_POS);
FLOOR_CEIL_CUST(max_almost,FIXNUM(16382,4) , FIXNUM(16382, 0), FIXNUM(16383,0));
FLOOR_CEIL_CUST(min      , FIX_MIN         , FIXNUM(-16384,0), FIXNUM(-16384 , 0));
FLOOR_CEIL_CUST(min_almost,FIXNUM(-16383,5), FIXNUM(-16384,0), FIXNUM(-16383 , 0));
FLOOR_CEIL_CUST(inf_pos  , FIX_INF_POS     , FIX_INF_POS     , FIX_INF_POS);
FLOOR_CEIL_CUST(inf_neg  , FIX_INF_NEG     , FIX_INF_NEG     , FIX_INF_NEG);
FLOOR_CEIL_CUST(nan      , FIX_NAN         , FIX_NAN         , FIX_NAN);


static void constants(void **state) {
  assert_true(FIX_PI  == 0x00064880);
  assert_true(FIX_TAU == 0x000c90fc);
  assert_true(FIX_E   == 0x00056fc4);
}

#define unit_cmp(name) unit_test(cmp_##name)
#define TEST_CMP(name, op1, op2, result) static void cmp_##name(void **state) { \
  fixed o1 = fix_convert_double(op1); \
  fixed o2 = fix_convert_double(op2); \
  int32_t cmp = FIX_CMP(o1, o2); \
  int32_t expected = result; \
  assert_memory_equal( &cmp, &expected, sizeof(uint32_t)); \
}

TEST_CMP(zero_zero_eq    , 0         , 0         , 0);
TEST_CMP(pos_zero_gt     , 1         , 0         , 1);
TEST_CMP(neg_zero_lt     , -1        , 0         , -1);
TEST_CMP(pos_pos_gt      , 1.4       , 0.5       , 1);
TEST_CMP(pos_pos_lt      , 0.4       , 0.5       , -1);
TEST_CMP(pos_pos_eq      , 0.5       , 0.5       , 0);
TEST_CMP(neg_neg_gt      , -1.4      , -1.5      , 1);
TEST_CMP(neg_neg_lt      , -0.9      , -0.5      , -1);
TEST_CMP(neg_neg_eq      , -0.5      , -0.5      , 0);
TEST_CMP(nan_nan         , nan("0")  , nan("0")  , 1);
TEST_CMP(nan_inf_pos     , nan("0")  , INFINITY  , 1);
TEST_CMP(nan_inf_neg     , nan("0")  , -INFINITY , 1);
TEST_CMP(nan_pos         , nan("0")  , 24.5      , 1);
TEST_CMP(nan_neg         , nan("0")  , -24.5     , 1);
TEST_CMP(pos_nan         , 24.5      , nan("0")  , 1);
TEST_CMP(neg_nan         , -24.5     , nan("0")  , 1);
TEST_CMP(inf_inf         , INFINITY  , INFINITY  , 0);
TEST_CMP(inf_pos         , INFINITY  , 24.5      , 1);
TEST_CMP(inf_neg         , INFINITY  , -24.5     , 1);
TEST_CMP(inf_inf_neg     , INFINITY  , -INFINITY , 1);
TEST_CMP(pos_inf         , 24.5      , INFINITY  , -1);
TEST_CMP(neg_inf         , -24.5     , INFINITY  , -1);
TEST_CMP(inf_neg_inf_pos , -INFINITY , INFINITY  , -1);
TEST_CMP(inf_neg_pos     , -INFINITY , 24.5      , -1);
TEST_CMP(inf_neg_neg     , -INFINITY , -24.5     , -1);
TEST_CMP(pos_neg         , 17.3      , -24.5     , 1);
TEST_CMP(neg_pos         , -17.3     , 24.5      , -1);

#define unit_add(name) unit_test(add_##name)
#define ADD_CUST(name, op1, op2, result) static void add_##name(void **state) { \
  fixed o1 = fix_convert_double(op1); \
  fixed o2 = fix_convert_double(op2); \
  fixed added = fix_add(o1,o2); \
  fixed expected = result; \
  if( !FIX_EQ_NAN(added, expected) ) { \
    char b1[100], b2[100]; \
    fix_print(b1, added); \
    fix_print(b2, expected); \
    fail_msg("Mismatch: %s (%x) != %s (%x)", b1, added, b2, expected); \
  } \
}
#define ADD(name, op1, op2, val) ADD_CUST(name, op1, op2, fix_convert_double(val))

ADD(one_zero               , 1        , 0             , 1);
ADD(one_one                , 1        , 1             , 2);
ADD(fifteen_one            , 15       , 1             , 16);
ADD(pos_neg                , 15       , -1.5          , 13.5);
ADD(pos_neg_cross_zero     , 15       , -18           , -3);
ADD_CUST(overflow          , 1<<13    , 1<<13         , FIX_INF_POS);
ADD_CUST(overflow_neg      , -(1<<13) , -((1<<13) +1) , FIX_INF_NEG);
ADD_CUST(inf_number        , INFINITY , 1             , FIX_INF_POS);
ADD_CUST(inf_neg           , INFINITY , -1            , FIX_INF_POS);
ADD_CUST(nan               , nan("0") , 0             , FIX_NAN);
ADD_CUST(nan_inf           , nan("0") , INFINITY      , FIX_NAN);
ADD_CUST(inf_nan           , INFINITY , nan("0")      , FIX_NAN);
ADD_CUST(nan_inf_neg       , nan("0") , -INFINITY     , FIX_NAN);
ADD_CUST(inf_inf_neg       , INFINITY , -INFINITY     , FIX_INF_POS);
// TODO: negative overflow , NaNs     , fractions

#define unit_mul(name) unit_test(mul_##name)
#define MUL_CUST(name, op1, op2, result) static void mul_##name(void **state) { \
  fixed o1 = fix_convert_double(op1); \
  fixed o2 = fix_convert_double(op2); \
  fixed muld = fix_mul(o1,o2); \
  fixed expected = result; \
  if( !FIX_EQ_NAN(muld, expected) ) { \
    char b1[100], b2[100]; \
    fix_print(b1, muld); \
    fix_print(b2, expected); \
    fail_msg("Mismatch: %s (%x) != %s (%x)", b1, muld, b2, expected); \
  } \
}
#define MUL(name, op1, op2, val) MUL_CUST(name, op1, op2, fix_convert_double(val))
MUL(one_zero              , 1           ,0             ,0);
MUL(one_one               , 1           , 1            ,1);
MUL(fifteen_one           , 15          , 1            ,15);
MUL(fifteen_two           , 15          , 2            ,30);
MUL(nthree_15             , -3          , 15           ,-45);
MUL(nthree_n15            , -3          , -15          ,45);
MUL(three_n15             , 3           , -15          ,-45);
MUL(frac5_15              , .5          , 15           ,7.5);
MUL_CUST(overflow         , 1<<10       , 1<<10        ,FIX_INF_POS);
MUL_CUST(inf_ten          , INFINITY    , 10           ,FIX_INF_POS);
MUL_CUST(inf_neg          , INFINITY    , -10          ,FIX_INF_NEG);
MUL_CUST(ninf_neg         ,  -INFINITY  , -10          ,FIX_INF_POS);
MUL_CUST(neg_inf          , -10         , INFINITY     ,FIX_INF_NEG);
MUL_CUST(neg_ninf         , -10         ,  -INFINITY   ,FIX_INF_POS);
MUL_CUST(pos_nan          , 10          , nan("0")     ,FIX_NAN);
MUL_CUST(neg_nan          , -10         , nan("0")     ,FIX_NAN);
MUL_CUST(inf_nan          , INFINITY    , nan("0")     ,FIX_NAN);
MUL_CUST(ninf_nan         , -INFINITY   , nan("0")     ,FIX_NAN);
MUL_CUST(nan_pos          , nan("0")    , 10           ,FIX_NAN);
MUL_CUST(nan_neg          , nan("0")    , -10          ,FIX_NAN);
MUL_CUST(nan_inf          , nan("0")    , INFINITY     ,FIX_NAN);
MUL_CUST(nan_ninf         , nan("0")    , -INFINITY    ,FIX_NAN);
MUL_CUST(tinyoverflow     , 148.5       , 148.5        ,FIX_INF_POS);
MUL_CUST(tinyoverflow_neg , 148.5       , -148.5       ,FIX_INF_NEG);
MUL_CUST(tinyoverflow_neg_neg , -148.5  , -148.5       ,FIX_INF_POS);

#define unit_div(name) unit_test(div_##name)
#define DIV_CUST(name, op1, op2, result) static void div_##name(void **state) { \
  fixed o1 = op1; \
  fixed o2 = op2; \
  fixed divd = fix_div(o1,o2); \
  fixed expected = result; \
  if( !FIX_EQ_NAN(divd, expected) ) { \
    char b1[100], b2[100]; \
    fix_print(b1, divd); \
    fix_print(b2, expected); \
    fail_msg("Mismatch: %s (%x) != %s (%x)", b1, divd, b2, expected); \
  } \
}

#define DIV_CUST_RESULT(name, op1, op2, result) \
  DIV_CUST(name, fix_convert_double(op1), fix_convert_double(op2), result)
#define DIV(name, op1, op2, result) \
  DIV_CUST(name, fix_convert_double(op1), fix_convert_double(op2), fix_convert_double(result))
DIV(one_one               , 1           , 1            ,1);
DIV(fifteen_one           , 15          , 1            ,15);
DIV(sixteen_two           , 16          , 2            ,8);
DIV(fifteen_nthree        , 15          , -3           ,-5);
DIV(nfifteen_nthree       , -15         , -3           ,5);
DIV(nfifteen_three        , -15         , 3            ,-5);
DIV(fifteen_frac5         , 15          , 0.5          ,30);
DIV_CUST_RESULT(overflow         , 1<<13       , 0.1          ,FIX_INF_POS);
DIV_CUST_RESULT(one_zero         , 1           ,0             ,FIX_NAN);
DIV_CUST_RESULT(inf_zero         , INFINITY    ,0             ,FIX_NAN);
DIV_CUST_RESULT(zero_inf         , 0           ,INFINITY      ,FIX_INF_POS);
DIV_CUST_RESULT(inf_neg          , INFINITY    , -10          ,FIX_INF_NEG);
DIV_CUST_RESULT(ninf_neg         ,  -INFINITY  , -10          ,FIX_INF_POS);
DIV_CUST_RESULT(neg_inf          , -10         , INFINITY     ,FIX_INF_NEG);
DIV_CUST_RESULT(neg_ninf         , -10         ,  -INFINITY   ,FIX_INF_POS);
DIV_CUST_RESULT(pos_nan          , 10          , nan("1")     ,FIX_NAN);
DIV_CUST_RESULT(neg_nan          , -10         , nan("1")     ,FIX_NAN);
DIV_CUST_RESULT(inf_nan          , INFINITY    , nan("1")     ,FIX_NAN);
DIV_CUST_RESULT(ninf_nan         , -INFINITY   , nan("1")     ,FIX_NAN);
DIV_CUST_RESULT(nan_pos          , nan("0")    , 10           ,FIX_NAN);
DIV_CUST_RESULT(nan_neg          , nan("0")    , -10          ,FIX_NAN);
DIV_CUST_RESULT(nan_inf          , nan("0")    , INFINITY     ,FIX_NAN);
DIV_CUST_RESULT(nan_ninf         , nan("0")    , -INFINITY    ,FIX_NAN);
DIV_CUST(regression1             , 0xf0000000  , 0x000022e8   ,FIX_INF_NEG);
DIV_CUST(regression2             , 0xf0000000  , fix_neg(0x000022e8) ,FIX_INF_POS);
DIV_CUST(regression3             , 0xf0000000  , 0x00004470          ,FIXNUM(-15321,658447265));


#define unit_neg(name) unit_test(neg_##name)
#define NEG_CUST(name, op1, result) static void neg_##name(void **state) { \
  fixed o1 = fix_convert_double(op1); \
  fixed negd = fix_neg(o1); \
  fixed expected = result; \
  assert_true( FIX_EQ_NAN(negd, expected) ); \
}
#define NEG(name, op1, val) NEG_CUST(name, op1, fix_convert_double(val))

NEG(zero,    0, 0);
NEG(one,     1, -1);
NEG(one_neg, -1, 1);
NEG_CUST(inf, INFINITY, FIX_INF_NEG);
NEG_CUST(inf_neg, -INFINITY, FIX_INF_POS);
NEG_CUST(nan, nan("0"), FIX_NAN);

#define unit_abs(name) unit_test(abs_##name)
#define ABS_CUST(name, op1, result) static void abs_##name(void **state) { \
  fixed o1 = fix_convert_double(op1); \
  fixed absd = fix_abs(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, absd, expected); \
}
#define ABS(name, op1, val) ABS_CUST(name, op1, fix_convert_double(val))

ABS(zero         , 0         , 0);
ABS(one          , 1         , 1);
ABS(one_neg      , -1        , 1);
ABS_CUST(inf     , INFINITY  , FIX_INF_POS);
ABS_CUST(inf_neg , -INFINITY , FIX_INF_POS);
ABS_CUST(nan     , nan("0")  , FIX_NAN);




#define unit_ln(name) unit_test(ln_##name)
#define LN_CUST(name, op1, result) static void ln_##name(void **state) { \
  fixed o1 = op1; \
  fixed ln = fix_ln(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, ln, expected); \
}
#define LN(name, op1, val) LN_CUST(name, op1, fix_convert_double(val))

LN_CUST(zero   , 0x0               , FIX_INF_NEG);
LN_CUST(0xf0   , 0xf0              , FIXNUM(-6,3028564453)); // -6.302863146
LN_CUST(0x80   , 0x80              , 0xfff22318);
LN_CUST(0xa0   , 0xa0              , 0xfff29358);
LN_CUST(two    , FIXNUM(2,0)       , FIXNUM(0 , 69314718));
LN_CUST(one    , FIXNUM(1,0)       , FIXNUM(0 , 0));
LN_CUST(e      , FIX_E             , 0x0001fda8 ); // not quite FIXNUM(1 , 0), but close
LN_CUST(ten    , FIXNUM(10,0)      , 0x000498ec ); // not quite FIXNUM(2 , 30258509), but close
LN_CUST(max    , FIX_MAX           , FIXNUM(9,7040405273 ));  // not quite 9.70406052597
LN_CUST(inf    , FIX_INF_POS       , FIX_INF_POS);
LN_CUST(neg    , FIXNUM(-1,0)      , FIX_NAN);
LN_CUST(nan    , FIX_NAN           , FIX_NAN);


#define unit_log2(name) unit_test(log2_##name)
#define LOG2_CUST(name, op1, result) static void log2_##name(void **state) { \
  fixed o1 = op1; \
  fixed log2 = fix_log2(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, log2, expected); \
}
#define LOG2(name, op1, val) LOG2_CUST(name, op1, fix_convert_double(val))

LOG2_CUST(zero   , 0x0               , FIX_INF_NEG);
LOG2_CUST(0xf0   , 0xf0              , FIXNUM(-9,093139648)); //9.093139648
LOG2_CUST(0x80   , 0x80              , 0xffec0000);
LOG2_CUST(0xa0   , 0xa0              , 0xffeca29c);
LOG2_CUST(one    , FIXNUM(1,0)       , FIXNUM(0 , 0));
LOG2_CUST(two    , FIXNUM(2,0)       , FIXNUM(1 , 0));
LOG2_CUST(e      , FIX_E             , 0x0002e064 ); // not quite 1.4426 but close
LOG2_CUST(ten    , FIXNUM(10,0)      , 0x0006a29c ); // not quite 3.3219 but close
LOG2_CUST(63     , FIXNUM(63,0)      , FIXNUM(5,977355957 )); // 5.9772799236
LOG2_CUST(64     , FIXNUM(64,0)      , FIXNUM(6, 0) );
LOG2_CUST(64_5   , FIXNUM(64,5)      , FIXNUM(6, 01116943359)); // 6.011227255423254
LOG2_CUST(max    , FIX_MAX           , FIXNUM(13,9999999) ); // not quite 3.3219 but close
LOG2_CUST(min    , FIX_MIN           , FIX_NAN );
LOG2_CUST(inf    , FIX_INF_POS       , FIX_INF_POS);
LOG2_CUST(neg    , FIXNUM(-1,0)      , FIX_NAN);
LOG2_CUST(nan    , FIX_NAN           , FIX_NAN);

#define unit_log10(name) unit_test(log10_##name)
#define LOG10_CUST(name, op1, result) static void log10_##name(void **state) { \
  fixed o1 = op1; \
  fixed log10 = fix_log10(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, log10, expected); \
}
#define LOG10(name, op1, val) LOG10_CUST(name, op1, fix_convert_double(val))

LOG10_CUST(zero   , 0x0               , FIX_INF_NEG);
LOG10_CUST(one    , FIXNUM(1,0)       , FIXNUM(-0 , 000061035)); // almost 0.
LOG10_CUST(two    , FIXNUM(2,0)       , FIXNUM(0 , 30096435));   // almost 0.30102999
LOG10_CUST(e      , FIX_E             , FIXNUM(0 , 434265136));  // 0.43429448
LOG10_CUST(ten    , FIXNUM(10,0)      , FIXNUM(1 , 0));
LOG10_CUST(fifteen, FIXNUM(15,0)      , FIXNUM(1 , 1760253906 )); // 1.176091259));
LOG10_CUST(63     , FIXNUM(63,0)      , FIXNUM(1 , 7992553710 )); // 1.79934054945
LOG10_CUST(64     , FIXNUM(64,0)      , FIXNUM(1 , 8060913085 )); // 1.80617997398
LOG10_CUST(64_5   , FIXNUM(64,5)      , FIXNUM(1 , 8094787597 )); // 1.80955971463
LOG10_CUST(max    , FIX_MAX           , FIXNUM(4 , 214294433)); // 4.21441993848
LOG10_CUST(inf    , FIX_INF_POS       , FIX_INF_POS);
LOG10_CUST(neg    , FIXNUM(-1,0)      , FIX_NAN);
LOG10_CUST(nan    , FIX_NAN           , FIX_NAN);

#define unit_exp(name) unit_test(exp_##name)
#define EXP_CUST(name, op1, result) static void exp_##name(void **state) { \
  fixed o1 = op1; \
  fixed exp = fix_exp(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, exp, expected); \
}
#define EXP(name, op1, val) EXP_CUST(name, op1, fix_convert_double(val))

EXP_CUST(zero   , FIX_ZERO          , FIXNUM(1,0));
EXP_CUST(one    , FIXNUM(1,0)       , FIX_E);
EXP_CUST(two    , FIXNUM(2,0)       , FIXNUM(7,3890991210));     // not 7,3890560989
EXP_CUST(e      , FIX_E             , FIXNUM(15,154296875));     // not 15,15426224147
EXP_CUST(ten    , FIXNUM(10,0)      , FIX_INF_POS);
EXP_CUST(one_neg, FIXNUM(-1,0)      , FIXNUM(0,3678794411));
EXP_CUST(two_neg, FIXNUM(-2,0)      , FIXNUM(0,1353352832));
EXP_CUST(e_neg  , fix_neg(FIX_E)    , FIXNUM(0,0659484863));     // not 0.0659880358
EXP_CUST(ten_neg, FIXNUM(-10,0)     , FIXNUM(0,0000453999));
EXP_CUST(neg_many,FIXNUM(-128,0)    , FIX_ZERO);
EXP_CUST(max    , FIX_MAX           , FIX_INF_POS);
EXP_CUST(inf    , FIX_INF_POS       , FIX_INF_POS);
EXP_CUST(inf_neg, FIX_INF_NEG       , FIX_ZERO);
EXP_CUST(nan    , FIX_NAN           , FIX_NAN);

#define unit_sqrt(name) unit_test(sqrt_##name)
#define SQRT_CUST(name, op1, result) static void sqrt_##name(void **state) { \
  fixed o1 = op1; \
  fixed sqrt = fix_sqrt(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, sqrt, expected); \
}
#define sqrt(name, op1, val) sqrt_CUST(name, op1, fix_convert_double(val))

SQRT_CUST(zero   , 0x0               , FIXNUM(0,0));
SQRT_CUST(half   , FIXNUM(0,5)       , FIXNUM(0,707092));
SQRT_CUST(one    , FIXNUM(1,0)       , FIXNUM(1,0));
SQRT_CUST(two    , FIXNUM(2,0)       , FIXNUM(1,414215));
SQRT_CUST(e      , FIX_E             , FIXNUM(1,648712) );
SQRT_CUST(ten    , FIXNUM(10,0)      , FIXNUM(3,162262) ); // not precisely sqrt(10) but so close
SQRT_CUST(big    , FIXNUM(10000,5345), FIXNUM(100,002655) );
SQRT_CUST(max    , FIX_MAX           , FIXNUM(128,0) );
SQRT_CUST(inf    , FIX_INF_POS       , FIX_INF_POS);
SQRT_CUST(neg    , FIXNUM(-1,0)      , FIX_NAN);
SQRT_CUST(nan    , FIX_NAN           , FIX_NAN);


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


#define unit_sin(name) unit_test(sin_##name)
#define SIN_CUST(name, op1, result) static void sin_##name(void **state) { \
  fixed o1 = op1; \
  fixed sin = fix_sin(o1); \
  fixed expected = result; \
  CHECK_EQ_NAN(#name, sin, expected); \
}
#define SIN(name, op1, val) SIN_CUST(name, op1, fix_convert_double(val))

SIN(zero       , FIX_ZERO                                           , 0);
SIN_CUST(pi_2  , fix_div(FIX_PI , FIXNUM(2 , 0))                    , 0x1fffc);
SIN_CUST(pi    , FIX_PI                                             , 0xfffffffc);
SIN_CUST(pi3_2 , fix_div(fix_mul(FIX_PI, FIXNUM(3,0)), FIXNUM(2,0)) , 0xfffe0000);
SIN_CUST(pi2   , fix_mul(FIX_PI, FIXNUM(2,0))                       , 0);
SIN_CUST(pi5_2 , fix_div(fix_mul(FIX_PI, FIXNUM(5,0)), FIXNUM(2,0)) , 0x1fffc);
SIN_CUST(pi3   , fix_mul(FIX_PI, FIXNUM(3,0))                       , 0xfffffffc);
SIN_CUST(pi7_2 , fix_div(fix_mul(FIX_PI, FIXNUM(7,0)), FIXNUM(2,0)) , 0xfffe0000);
SIN_CUST(pi4   , fix_mul(FIX_PI, FIXNUM(4,0))                       , 0x4);

SIN_CUST(neg_pi_2  , fix_neg(fix_div(FIX_PI , FIXNUM(2 , 0)))                    , 0xfffe0000);
SIN_CUST(neg_pi    , fix_neg(FIX_PI)                                             , 0x0);
SIN_CUST(neg_pi3_2 , fix_neg(fix_div(fix_mul(FIX_PI, FIXNUM(3,0)), FIXNUM(2,0))) , 0x1fffc);
SIN_CUST(neg_pi2   , fix_neg(fix_mul(FIX_PI, FIXNUM(2,0)))                       , 0xfffffffc);

SIN_CUST(inf_pos   , FIX_INF_POS                          , FIX_INF_POS);
SIN_CUST(inf_neg   , FIX_INF_NEG                          , FIX_INF_NEG);
SIN_CUST(nan       , FIX_NAN                              , FIX_NAN);

#define unit_trig(name) unit_test(trig_##name)
#define TRIG(name, op1, sinx, cosx, tanx) static void trig_##name(void **state) { \
  fixed o1 = op1; \
  fixed sin = fix_cordic_sin(o1); \
  CHECK_EQ_NAN(#name " sin", sin, sinx); \
  fixed cos = fix_cordic_cos(o1); \
  CHECK_EQ_NAN(#name " cos", cos, cosx); \
  fixed tan = fix_cordic_tan(o1); \
  CHECK_EQ_NAN(#name " tan", tan, tanx); \
}

#define n3_2 fix_div(FIXNUM(3,0), FIXNUM(2,0))

// Note that tan is poorly defined near pi/2 + n*pi. It's either positive
// infinity or negative infinity, with very little separating them.

TRIG(zero  , FIX_ZERO                          , FIX_ZERO             , FIXNUM(1,0),            FIX_ZERO );
TRIG(pi_2  , fix_div(FIX_PI , FIXNUM(2,0))     , FIXNUM(1,0)          , FIX_EPSILON_NEG,        FIX_INF_NEG );
TRIG(pi    , FIX_PI                            , fix_neg(FIX_EPSILON) , FIXNUM(-1,0),           FIX_ZERO );
TRIG(pi3_2 , fix_mul(FIX_PI, n3_2)             , FIXNUM(-1,0)         , FIX_ZERO,               FIX_INF_NEG);
TRIG(pi2   , fix_mul(FIX_PI, FIXNUM(2,0))      , FIX_ZERO             , FIXNUM(1,0),            FIX_ZERO );

TRIG(pi5_2 , fix_div(fix_mul(FIX_PI, FIXNUM(5,0)), FIXNUM(2,0)) ,
             FIXNUM(1,0), FIX_EPSILON_NEG, FIX_INF_NEG);
TRIG(pi3   , fix_mul(FIX_PI, FIXNUM(3,0))                       ,
             FIX_EPSILON_NEG, FIXNUM(-1,0), FIX_EPSILON );
TRIG(pi7_2 , fix_div(fix_mul(FIX_PI, FIXNUM(7,0)), FIXNUM(2,0)) ,
             FIXNUM(-1,0), FIX_EPSILON, FIX_INF_NEG);
TRIG(pi4   , fix_mul(FIX_PI, FIXNUM(4,0)),
             FIX_EPSILON, FIXNUM(1,0), FIX_EPSILON );

TRIG(neg_pi_2  , fix_neg(fix_div(FIX_PI , FIXNUM(2 , 0))),
                 FIXNUM(-1,0)         , FIX_EPSILON_NEG,               FIX_INF_POS);
TRIG(neg_pi    , fix_neg(FIX_PI),
                 FIX_ZERO, FIXNUM(-1,0), FIX_ZERO );
TRIG(neg_pi3_2 , fix_neg(fix_div(fix_mul(FIX_PI, FIXNUM(3,0)), FIXNUM(2,0))) ,
                 FIXNUM(1,0)          , FIX_ZERO,   FIX_INF_POS );
TRIG(neg_pi2   , fix_neg(fix_mul(FIX_PI, FIXNUM(2,0))),
                 FIX_EPSILON_NEG, FIXNUM(1,0), FIX_ZERO );

TRIG(q1       , FIXNUM(1,0),                  FIXNUM(0,8414709848), FIXNUM(0,54028320) , FIXNUM(1,5574077246));
TRIG(q2      ,  fix_add(fix_div(FIX_PI, FIXNUM(2,0)), FIXNUM(1,0)),
                                              FIXNUM(0,54028320) , FIXNUM(-0,84149169) , FIXNUM(-0,64209261));
TRIG(q3      ,  fix_add(FIX_PI, FIXNUM(0,5)), FIXNUM(-0,47942553), FIXNUM(-0,877582561), FIXNUM(0,5463256835));
TRIG(q4      ,  FIXNUM(-0,5),                 FIXNUM(-0,47942553), FIXNUM(0,877563476) , FIXNUM(-0,546302489));

TRIG(inf_pos   , FIX_INF_POS, FIX_NAN, FIX_NAN, FIX_NAN);
TRIG(inf_neg   , FIX_INF_NEG, FIX_NAN, FIX_NAN, FIX_NAN);
TRIG(nan       , FIX_NAN, FIX_NAN, FIX_NAN, FIX_NAN);

#define unit_print(name) unit_test(print_##name)
#define PRINT_CUST(name, op1, result) static void print_##name(void **state) { \
  fixed o1 = op1; \
  char buf[23]; \
  fix_print_const(buf, o1); \
  char* expected = result; \
  if(strcmp(buf, expected))  { \
    printf("Strings not equal: '%s' != '%s'\n", buf, expected); \
    fix_print(buf, o1); \
    printf("I should have expected: '%s'\n", buf); \
  } \
  assert_memory_equal(buf, expected, 23); \
}

PRINT_CUST(zero    , 0x0               , " 00000.000000000000000");
PRINT_CUST(half    , FIXNUM(0,5)       , " 00000.500000000000000");
PRINT_CUST(half_neg, FIXNUM(-0,5)      , "-00000.500000000000000");
PRINT_CUST(one     , FIXNUM(1,0)       , " 00001.000000000000000");
PRINT_CUST(one_neg , FIXNUM(-1,0)      , "-00001.000000000000000");
PRINT_CUST(two     , FIXNUM(2,0)       , " 00002.000000000000000");
PRINT_CUST(e       , FIX_E             , " 00002.718292236328125");
PRINT_CUST(e_neg   , fix_neg(FIX_E)    , "-00002.718292236328125");
PRINT_CUST(ten     , FIXNUM(10,0)      , " 00010.000000000000000");
PRINT_CUST(big     , FIXNUM(10000,5345), " 10000.534484863281250");
PRINT_CUST(max     , FIX_MAX           , " 16383.999969482421875");
PRINT_CUST(min     , FIX_MIN           , "-16384.000000000000000");
PRINT_CUST(inf     , FIX_INF_POS       , " Inf                  ");
PRINT_CUST(inf_neg , FIX_INF_NEG       , "-Inf                  ");
PRINT_CUST(nan     , FIX_NAN           , " NaN                  ");


int main(int argc, char** argv) {

  const UnitTest tests[] = {
    unit_test(null_test_success),
    unit_fixint_dbl(zero),
    unit_fixint_dbl(one),
    unit_fixint_dbl(one_neg),
    unit_fixint_dbl(two),
    unit_fixint_dbl(two_neg),
    unit_fixint_dbl(many),
    unit_fixint_dbl(many_neg),
    unit_fixint_dbl(frac),
    unit_fixint_dbl(frac_neg),
    unit_fixint_dbl(inf_pos),
    unit_fixint_dbl(inf_neg),
    unit_fixint_dbl(nan),

    unit_fixnum(zero),
    unit_fixnum(one),
    unit_fixnum(one_neg),
    unit_fixnum(two),
    unit_fixnum(two_neg),
    unit_fixnum(many),
    unit_fixnum(many_neg),
    unit_fixnum(frac),
    unit_fixnum(frac_neg),

    unit_fixnum(regress0),
    unit_fixnum(regress1),
    unit_fixnum(regress12),
    unit_fixnum(regress2),
    unit_fixnum(regress3),
    unit_fixnum(regress4),

    unit_eq(zero),
    unit_eq(frac),
    unit_eq(nan),
    unit_eq(inf),
    unit_eq(inf_neg),

    unit_cmp(zero_zero_eq),
    unit_cmp(pos_zero_gt),
    unit_cmp(neg_zero_lt),
    unit_cmp(pos_pos_gt),
    unit_cmp(pos_pos_lt),
    unit_cmp(pos_pos_eq),
    unit_cmp(neg_neg_gt),
    unit_cmp(neg_neg_lt),
    unit_cmp(neg_neg_eq),
    unit_cmp(nan_nan),
    unit_cmp(nan_inf_pos),
    unit_cmp(nan_inf_neg),
    unit_cmp(nan_pos),
    unit_cmp(nan_neg),
    unit_cmp(pos_nan),
    unit_cmp(pos_neg),
    unit_cmp(neg_nan),
    unit_cmp(inf_inf),
    unit_cmp(inf_pos),
    unit_cmp(inf_neg),
    unit_cmp(inf_inf_neg),
    unit_cmp(pos_inf),
    unit_cmp(neg_inf),
    unit_cmp(inf_neg_inf_pos),
    unit_cmp(inf_neg_pos),
    unit_cmp(inf_neg_neg),
    unit_cmp(pos_neg),
    unit_cmp(neg_pos),

    unit_round_to_even(zero),
    unit_round_to_even(one),
    unit_round_to_even(two),
    unit_round_to_even(three),
    unit_round_to_even(four),
    unit_round_to_even(five),
    unit_round_to_even(six),
    unit_round_to_even(seven),
    unit_round_to_even(eight),
    unit_round_to_even(zero_plus),
    unit_round_to_even(one_plus),
    unit_round_to_even(two_plus),
    unit_round_to_even(three_plus),
    unit_round_to_even(four_plus),
    unit_round_to_even(five_plus),
    unit_round_to_even(six_plus),
    unit_round_to_even(seven_plus),
    unit_round_to_even(eight_plus),

    unit_rounding(zero),
    unit_rounding(one),
    unit_rounding(one3),
    unit_rounding(one5),
    unit_rounding(one7),
    unit_rounding(two),
    unit_rounding(two3),
    unit_rounding(two5),
    unit_rounding(two7),
    unit_rounding(one_neg),
    unit_rounding(one3_neg),
    unit_rounding(one5_neg),
    unit_rounding(two3_neg),
    unit_rounding(two5_neg),
    unit_rounding(max),
    unit_rounding(min),
    unit_rounding(nan),
    unit_rounding(inf),
    unit_rounding(inf_neg),

    unit_floor_ceil(zero),
    unit_floor_ceil(half),
    unit_floor_ceil(half_neg),
    unit_floor_ceil(one),
    unit_floor_ceil(one_neg),
    unit_floor_ceil(pi),
    unit_floor_ceil(pi_neg),
    unit_floor_ceil(max),
    unit_floor_ceil(max_almost),
    unit_floor_ceil(min),
    unit_floor_ceil(min_almost),
    unit_floor_ceil(inf_pos),
    unit_floor_ceil(inf_neg),

    unit_test(constants),

    unit_add(one_zero),
    unit_add(one_one),
    unit_add(fifteen_one),
    unit_add(pos_neg),
    unit_add(pos_neg_cross_zero),
    unit_add(overflow),
    unit_add(overflow_neg),
    unit_add(inf_number),
    unit_add(inf_neg),
    unit_add(nan),
    unit_add(nan_inf),
    unit_add(inf_nan),
    unit_add(nan_inf_neg),
    unit_add(inf_inf_neg),

    unit_mul(one_zero),
    unit_mul(one_one),
    unit_mul(fifteen_one),
    unit_mul(fifteen_two),
    unit_mul(nthree_15),
    unit_mul(nthree_n15),
    unit_mul(three_n15),
    unit_mul(frac5_15),
    unit_mul(overflow),
    unit_mul(inf_ten),
    unit_mul(inf_neg),
    unit_mul(ninf_neg),
    unit_mul(neg_inf),
    unit_mul(neg_ninf),
    unit_mul(pos_nan),
    unit_mul(neg_nan),
    unit_mul(inf_nan),
    unit_mul(ninf_nan),
    unit_mul(nan_pos),
    unit_mul(nan_neg),
    unit_mul(nan_inf),
    unit_mul(nan_ninf),
    unit_mul(tinyoverflow),
    unit_mul(tinyoverflow_neg),
    unit_mul(tinyoverflow_neg_neg),


    unit_div(one_zero),
    unit_div(one_one),
    unit_div(fifteen_one),
    unit_div(sixteen_two),
    unit_div(nfifteen_three),
    unit_div(nfifteen_nthree),
    unit_div(fifteen_nthree),
    unit_div(fifteen_frac5),
    unit_div(inf_zero),
    unit_div(zero_inf),
    unit_div(inf_neg),
    unit_div(ninf_neg),
    unit_div(neg_inf),
    unit_div(neg_ninf),
    unit_div(pos_nan),
    unit_div(neg_nan),
    unit_div(inf_nan),
    unit_div(ninf_nan),
    unit_div(nan_pos),
    unit_div(nan_neg),
    unit_div(nan_inf),
    unit_div(nan_ninf),
    unit_div(overflow),
    unit_div(regression1),
    unit_div(regression2),
    unit_div(regression3),

    unit_neg(zero),
    unit_neg(one),
    unit_neg(one_neg),
    unit_neg(inf),
    unit_neg(inf_neg),
    unit_neg(nan),

    unit_abs(zero),
    unit_abs(one),
    unit_abs(one_neg),
    unit_abs(inf),
    unit_abs(inf_neg),
    unit_abs(nan),

    unit_ln(zero),
    unit_ln(one),
    unit_ln(two),
    unit_ln(e),
    unit_ln(ten),
    unit_ln(max),
    unit_ln(inf),
    unit_ln(neg),
    unit_ln(nan),
    unit_ln(0xf0),
    unit_ln(0x80),
    unit_ln(0xa0),

    unit_log2(zero),
    unit_log2(one),
    unit_log2(two),
    unit_log2(e),
    unit_log2(ten),
    unit_log2(63),
    unit_log2(64),
    unit_log2(64_5),
    unit_log2(max),
    unit_log2(min),
    unit_log2(inf),
    unit_log2(neg),
    unit_log2(nan),
    unit_log2(0xf0),
    unit_log2(0x80),
    unit_log2(0xa0),

    unit_log10(zero),
    unit_log10(one),
    unit_log10(two),
    unit_log10(e),
    unit_log10(ten),
    unit_log10(fifteen),
    unit_log10(63),
    unit_log10(64),
    unit_log10(64_5),
    unit_log10(max),
    unit_log10(inf),
    unit_log10(neg),
    unit_log10(nan),

    unit_exp(zero),
    unit_exp(one),
    unit_exp(two),
    unit_exp(e),
    unit_exp(ten),
    unit_exp(one_neg),
    unit_exp(two_neg),
    unit_exp(e_neg),
    unit_exp(ten_neg),
    unit_exp(neg_many),
    unit_exp(max),
    unit_exp(inf),
    unit_exp(inf_neg),
    unit_exp(nan),

    unit_sqrt(zero),
    unit_sqrt(half),
    unit_sqrt(one),
    unit_sqrt(two),
    unit_sqrt(e),
    unit_sqrt(ten),
    unit_sqrt(big),
    unit_sqrt(max),
    unit_sqrt(inf),
    unit_sqrt(neg),
    unit_sqrt(nan),

    unit_sin(zero),
    unit_sin(pi_2),
    unit_sin(pi),
    unit_sin(pi3_2),
    unit_sin(pi2),
    unit_sin(pi5_2),
    unit_sin(pi3),
    unit_sin(pi7_2),
    unit_sin(pi4),
    unit_sin(neg_pi_2),
    unit_sin(neg_pi),
    unit_sin(neg_pi3_2),
    unit_sin(neg_pi2),
    unit_sin(inf_pos),
    unit_sin(inf_neg),
    unit_sin(nan),

    unit_trig(zero),
    unit_trig(pi_2),
    unit_trig(pi),
    unit_trig(pi3_2),
    unit_trig(pi2),
    unit_trig(pi5_2),
    unit_trig(pi3),
    unit_trig(pi7_2),
    unit_trig(pi4),
    unit_trig(neg_pi_2),
    unit_trig(neg_pi),
    unit_trig(neg_pi3_2),
    unit_trig(neg_pi2),
    unit_trig(inf_pos),
    unit_trig(inf_neg),
    unit_trig(nan),
    unit_trig(q1),
    unit_trig(q2),
    unit_trig(q3),
    unit_trig(q4),

    unit_print(zero),
    unit_print(half),
    unit_print(half_neg),
    unit_print(one),
    unit_print(one_neg),
    unit_print(two),
    unit_print(e),
    unit_print(e_neg),
    unit_print(ten),
    unit_print(big),
    unit_print(max),
    unit_print(min),
    unit_print(inf),
    unit_print(inf_neg),
    unit_print(nan),
  };

  return run_tests(tests);
}

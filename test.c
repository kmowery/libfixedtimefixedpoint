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
    fail_msg( error_msg ": %s (%x) != %s (%x)", b1, var1, b2, var2); \
  }
#define CHECK_EQ_NAN(error_msg, var1, var2) \
  if( !FIX_EQ_NAN(var1, var2) ) { \
    char b1[100], b2[100]; \
    fix_print(b1, var1); fix_print(b2, var2); \
    fail_msg( error_msg ": %s (%x) != %s (%x)", b1, var1, b2, var2); \
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
  assert_memory_equal( &f, &g, sizeof(fixed)); \
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
CONVERT_DBL(inf_pos  , INFINITY  , F_INF_POS);
CONVERT_DBL(inf_neg  , -INFINITY , F_INF_NEG);
CONVERT_DBL(nan      , nan("0")  , F_NAN);
/* TODO: add tests for round-to-even */

#define unit_fixnum(name) unit_test(fixnum_##name)
#define TEST_FIXNUM(name, z, frac, bits) static void fixnum_##name(void **state) { \
  fixed f = bits; \
  fixed g = FIXNUM(z, frac); \
  CHECK_EQ("fixnum", g, f); \
}

TEST_FIXNUM(zero     , 0     , 0    , 0x0);
TEST_FIXNUM(one      , 1     , 0    , 0x20000);
TEST_FIXNUM(one_neg  , -1    , 0    , 0xfffe0000);
TEST_FIXNUM(two      , 2     , 0    , 0x40000);
TEST_FIXNUM(two_neg  , -2    , 0    , 0xfffc0000);
TEST_FIXNUM(many     , 1000  , 4    , 0x7d0cccc);
TEST_FIXNUM(many_neg , -1000 , 4    , 0xf82f3334);
TEST_FIXNUM(frac     , 0     , 5342 , 0x11180);
TEST_FIXNUM(frac_neg , -0    , 5342 , 0xfffeee80);

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
TEST_EQ(nan     , F_NAN     , F_NAN     , 0 , 1);
TEST_EQ(inf     , F_INF_POS , F_INF_POS , 1 , 1);
TEST_EQ(inf_neg , F_INF_NEG , F_INF_NEG , 1 , 1);


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
#define TEST_ROUNDING(name, value, res_even, res_up, res_ceil, res_floor) static void rounding_##name(void **state) { \
  fixed input = fix_convert_double(value); \
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
TEST_ROUNDING(nan      , NaN       , 0       , 0       , 0       , 0);
TEST_ROUNDING(inf      , INFINITY  , INT_MAX , INT_MAX , INT_MAX , INT_MAX);
TEST_ROUNDING(inf_neg  , -INFINITY , INT_MIN , INT_MIN , INT_MIN , INT_MIN);

static void constants(void **state) {
  assert_true(FIX_PI  == 0x00064880);
  assert_true(FIX_TAU == 0x000c90fc);
  assert_true(FIX_E   == 0x00056fc0);
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
ADD_CUST(overflow          , 1<<13    , 1<<13         , F_INF_POS);
ADD_CUST(overflow_neg      , -(1<<13) , -((1<<13) +1) , F_INF_NEG);
ADD_CUST(inf_number        , INFINITY , 1             , F_INF_POS);
ADD_CUST(inf_neg           , INFINITY , -1            , F_INF_POS);
ADD_CUST(nan               , nan("0") , 0             , F_NAN);
ADD_CUST(nan_inf           , nan("0") , INFINITY      , F_NAN);
ADD_CUST(inf_nan           , INFINITY , nan("0")      , F_NAN);
ADD_CUST(nan_inf_neg       , nan("0") , -INFINITY     , F_NAN);
ADD_CUST(inf_inf_neg       , INFINITY , -INFINITY     , F_INF_POS);
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
MUL_CUST(overflow         , 1<<10       , 1<<10        ,F_INF_POS);
MUL_CUST(inf_ten          , INFINITY    , 10           ,F_INF_POS);
MUL_CUST(inf_neg          , INFINITY    , -10          ,F_INF_NEG);
MUL_CUST(ninf_neg         ,  -INFINITY  , -10          ,F_INF_POS);
MUL_CUST(neg_inf          , -10         , INFINITY     ,F_INF_NEG);
MUL_CUST(neg_ninf         , -10         ,  -INFINITY   ,F_INF_POS);
MUL_CUST(pos_nan          , 10          , nan("0")     ,F_NAN);
MUL_CUST(neg_nan          , -10         , nan("0")     ,F_NAN);
MUL_CUST(inf_nan          , INFINITY    , nan("0")     ,F_NAN);
MUL_CUST(ninf_nan         , -INFINITY   , nan("0")     ,F_NAN);
MUL_CUST(nan_pos          , nan("0")    , 10           ,F_NAN);
MUL_CUST(nan_neg          , nan("0")    , -10          ,F_NAN);
MUL_CUST(nan_inf          , nan("0")    , INFINITY     ,F_NAN);
MUL_CUST(nan_ninf         , nan("0")    , -INFINITY    ,F_NAN);

#define unit_div(name) unit_test(div_##name)
#define DIV_CUST(name, op1, op2, result) static void div_##name(void **state) { \
  fixed o1 = fix_convert_double(op1); \
  fixed o2 = fix_convert_double(op2); \
  fixed divd = fix_div(o1,o2); \
  fixed expected = result; \
  CHECK_EQ_NAN("div", divd, expected); \
}
#define DIV(name, op1, op2, val) DIV_CUST(name, op1, op2, fix_convert_double(val))
DIV(one_one               , 1           , 1            ,1);
DIV(fifteen_one           , 15          , 1            ,15);
DIV(sixteen_two           , 16          , 2            ,8);
DIV(fifteen_nthree        , 15          , -3           ,-5);
DIV(nfifteen_nthree       , -15         , -3           ,5);
DIV(nfifteen_three        , -15         , 3            ,-5);
DIV(fifteen_frac5         , 15          , 0.5          ,30);
DIV_CUST(overflow         , 1<<13       , 0.1          ,F_INF_POS);
DIV_CUST(one_zero         , 1           ,0             ,F_NAN);
DIV_CUST(inf_zero         , INFINITY    ,0             ,F_NAN);
DIV_CUST(zero_inf         , 0           ,INFINITY      ,F_INF_POS);
DIV_CUST(inf_neg          , INFINITY    , -10          ,F_INF_NEG);
DIV_CUST(ninf_neg         ,  -INFINITY  , -10          ,F_INF_POS);
DIV_CUST(neg_inf          , -10         , INFINITY     ,F_INF_NEG);
DIV_CUST(neg_ninf         , -10         ,  -INFINITY   ,F_INF_POS);
DIV_CUST(pos_nan          , 10          , nan("1")     ,F_NAN);
DIV_CUST(neg_nan          , -10         , nan("1")     ,F_NAN);
DIV_CUST(inf_nan          , INFINITY    , nan("1")     ,F_NAN);
DIV_CUST(ninf_nan         , -INFINITY   , nan("1")     ,F_NAN);
DIV_CUST(nan_pos          , nan("0")    , 10           ,F_NAN);
DIV_CUST(nan_neg          , nan("0")    , -10          ,F_NAN);
DIV_CUST(nan_inf          , nan("0")    , INFINITY     ,F_NAN);
DIV_CUST(nan_ninf         , nan("0")    , -INFINITY    ,F_NAN);


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
NEG_CUST(inf, INFINITY, F_INF_NEG);
NEG_CUST(inf_neg, -INFINITY, F_INF_POS);
NEG_CUST(nan, nan("0"), F_NAN);


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

SIN_CUST(inf_pos   , F_INF_POS                          , F_INF_POS);
SIN_CUST(inf_neg   , F_INF_NEG                          , F_INF_NEG);
SIN_CUST(nan       , F_NAN                              , F_NAN);


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
    unit_rounding(nan),
    unit_rounding(inf),
    unit_rounding(inf_neg),

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


    unit_neg(zero),
    unit_neg(one),
    unit_neg(one_neg),
    unit_neg(inf),
    unit_neg(inf_neg),
    unit_neg(nan),

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
  };

  return run_tests(tests);
}

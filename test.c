#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <math.h>

#include "ftfp.h"

static void null_test_success(void **state) {
  //(void) state;
}


void p(fixed f) {
  char buf[40];

  fix_print(buf, f);
  printf("n: %s\n", buf);
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
  assert_true( FIX_EQ(f, g) ); \
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
/* TODO: add tests for round-to-even */

#define unit_round_to_even(name) unit_test(round_to_even_##name)
#define TEST_ROUND_TO_EVEN(name, value, shift, result) static void round_to_even_##name(void **state) { \
  int32_t rounded = ROUND_TO_EVEN(value, shift); \
  int32_t expected = result; \
  assert_memory_equal( &rounded, &expected, sizeof(uint32_t)); \
}

TEST_ROUND_TO_EVEN(zero  , 0x0 << 1 , 0x3 , 0x0);
TEST_ROUND_TO_EVEN(one   , 0x1 << 1 , 0x3 , 0x0);
TEST_ROUND_TO_EVEN(two   , 0x2 << 1 , 0x3 , 0x0);
TEST_ROUND_TO_EVEN(three , 0x3 << 1 , 0x3 , 0x1);
TEST_ROUND_TO_EVEN(four  , 0x4 << 1 , 0x3 , 0x1);
TEST_ROUND_TO_EVEN(five  , 0x5 << 1 , 0x3 , 0x1);
TEST_ROUND_TO_EVEN(six   , 0x6 << 1 , 0x3 , 0x1);
TEST_ROUND_TO_EVEN(seven , 0x7 << 1 , 0x3 , 0x2);
TEST_ROUND_TO_EVEN(eight , 0x8 << 1 , 0x3 , 0x2);

#define unit_add(name) unit_test(add_##name)
#define ADD_CUST(name, op1, op2, result) static void add_##name(void **state) { \
  fixed o1 = fix_convert_double(op1); \
  fixed o2 = fix_convert_double(op2); \
  fixed added = fix_add(o1,o2); \
  fixed expected = result; \
  if( !FIX_EQ(added, expected) ) { \
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
  assert_true( FIX_EQ(muld, expected) ); \
}
#define MUL(name, op1, op2, val) MUL_CUST(name, op1, op2, fix_convert_double(val))
MUL(one_zero,      1,     0,     0);
MUL(one_one,       1,     1,     1);
MUL(fifteen_one,   15,    1,     15);
MUL(fifteen_two,   15,    2,     30);


#define unit_neg(name) unit_test(neg_##name)
#define NEG_CUST(name, op1, result) static void neg_##name(void **state) { \
  fixed o1 = fix_convert_double(op1); \
  fixed negd = fix_neg(o1); \
  fixed expected = result; \
  assert_true( FIX_EQ(negd, expected) ); \
}
#define NEG(name, op1, val) NEG_CUST(name, op1, fix_convert_double(val))

NEG(zero,    0, 0);
NEG(one,     1, -1);
NEG(one_neg, -1, 1);
NEG_CUST(inf, INFINITY, F_INF_NEG);
NEG_CUST(inf_neg, -INFINITY, F_INF_POS);
NEG_CUST(nan, nan("0"), F_NAN);

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

    unit_round_to_even(zero),
    unit_round_to_even(one),
    unit_round_to_even(two),
    unit_round_to_even(three),
    unit_round_to_even(four),
    unit_round_to_even(five),
    unit_round_to_even(six),
    unit_round_to_even(seven),
    unit_round_to_even(eight),

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

    unit_neg(zero),
    unit_neg(one),
    unit_neg(one_neg),
    unit_neg(inf),
    unit_neg(inf_neg),
    unit_neg(nan),
  };

  return run_tests(tests);
}

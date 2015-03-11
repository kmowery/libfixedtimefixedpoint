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
#include "internal.h"

#include "fcntl.h"
#include "unistd.h"
#include <errno.h>

FILE* fd;

void p(fixed f) {
  char buf[FIX_PRINT_BUFFER_SIZE];

  fix_print(buf, f);
  printf("n: %s ("FIX_PRINTF_HEX")\n", buf, f);
}

/* Change , to ., and remove leading zeros in the integer */
void fix_buffer(char* buf, int size) {
  /* replace . with , */
  char* dot = strstr(buf, ".");
  *dot = ',';
  char* leadingzeroes = buf;
  if(leadingzeroes != NULL) {
    for( leadingzeroes++; *leadingzeroes == '0'; leadingzeroes++ ) {
      if(*(leadingzeroes+1) != ',' ) {
        *leadingzeroes = ' ';
      }
    }
  }
}

#define TEST_HELPER(name, code) static void name(void **state) code

#define PRINT(name, op1) \
TEST_HELPER(print_##name, { \
  fixed o1 = op1; \
  char buf[FIX_PRINT_BUFFER_SIZE]; \
  fix_print(buf, o1); \
  fprintf(fd, "  #define %-30s \"%s\" // 0x"FIX_PRINTF_HEX"\n", "PRINT_TEST_" #name, buf, op1); \
};)


#define PRINT_TESTS \
PRINT(zero        , FIX_ZERO                  )  \
PRINT(half        , FIXNUM(0,5)               )  \
PRINT(half_neg    , FIXNUM(-0,5)              )  \
PRINT(one         , FIXNUM(1,0)               )  \
PRINT(one_neg     , FIXNUM(-1,0)              )  \
PRINT(two         , FIXNUM(2,0)               )  \
PRINT(epsilon     , FIX_EPSILON               )  \
PRINT(e           , FIX_E                     )  \
PRINT(e_neg       , fix_neg(FIX_E)            )  \
PRINT(pi          , FIX_PI                    )  \
PRINT(pi_neg      , fix_neg(FIX_PI)           )  \
PRINT(tau         , FIX_TAU                   )  \
PRINT(tau_neg     , fix_neg(FIX_TAU)          )  \
PRINT(ten         , FIXNUM(10,0)              )  \
PRINT(ten_neg     , FIXNUM(-10,0)             )  \
PRINT(max         , FIX_MAX                   )  \
PRINT(min         , FIX_MIN                   )  \
PRINT(int_max     , FIXNUM(FIX_INT_MAX,0)     )  \
PRINT(int_max_neg , FIXNUM(-FIX_INT_MAX,0)    )  \
PRINT(inf         , FIX_INF_POS               )  \
PRINT(inf_neg     , FIX_INF_NEG               )  \
PRINT(nan         , FIX_NAN                   )
PRINT_TESTS

#define SQRT(name, op1, result) \
TEST_HELPER(sqrt_##name, { \
  fixed o1 = op1; \
  fixed fsqrt = fix_sqrt(o1); \
  char buf[FIX_PRINT_BUFFER_SIZE]; \
  fix_print(buf, fsqrt); \
  char abuf[FIX_PRINT_BUFFER_SIZE]; \
  fix_print(abuf, result); \
  char dbuf[FIX_PRINT_BUFFER_SIZE]; \
  fix_print(dbuf,fix_sub(fsqrt, result)); \
    fix_buffer(buf, FIX_PRINT_BUFFER_SIZE); \
  fprintf(fd, "  #define %-30s FIXNUM(%s) // 0x"FIX_PRINTF_HEX"\n", "SQRT_MAX_FIXED", buf, fsqrt); \
  fix_print(buf, op1); \
  fprintf(fd, "  // Max was FIXNUM(%s), sqrt was %s, difference %s // 0x"FIX_PRINTF_HEX"\n", buf, abuf, dbuf, op1); \
};)
#define SQRT_TESTS \
SQRT(max    , FIX_MAX           , fix_convert_from_double(sqrt(fix_convert_to_double(FIX_MAX))))
SQRT_TESTS

#define LN(name, op1)                                                             \
TEST_HELPER(ln_##name, {                                                          \
  fixed o1 = op1;                                                                 \
  fixed ln = fix_ln(o1);                                                          \
  double actual = (log(fix_convert_to_double(op1)));                              \
  if(actual >= fix_convert_to_double(FIX_MIN)) {                                  \
    char buf[FIX_PRINT_BUFFER_SIZE];                                              \
    fix_print(buf, ln);                                                           \
    fix_buffer(buf, FIX_PRINT_BUFFER_SIZE);                                       \
    fprintf(fd, "  #define %-30s FIXNUM(%s) // 0x"FIX_PRINTF_HEX", actual %.20g, "\
      "difference: %.20g epsilon: %g larger: %d\n",                               \
      "FIX_TEST_LN_"#name, buf, ln, actual,                                       \
          actual-fix_convert_to_double(ln), fix_convert_to_double(FIX_EPSILON),   \
          fabs(actual - fix_convert_to_double(ln)) > fix_convert_to_double(FIX_EPSILON));\
  } else {                                                                        \
    fprintf(fd, "  #define %-30s FIX_INF_NEG // actual: %g\n",                    \
      "FIX_TEST_LN_"#name, actual);                                               \
  }                                                                               \
};)

#define LN_TESTS            \
LN(epsilon, FIX_EPSILON)    \
LN(max, FIX_MAX)
LN_TESTS

#define LOG2(name, op1)                                                           \
TEST_HELPER(log2_##name, {                                                        \
  fixed o1 = op1;                                                                 \
  fixed lg2 = fix_log2(o1);                                                       \
  double actual = (log2(fix_convert_to_double(op1)));                             \
  if(actual >= fix_convert_to_double(FIX_MIN)) {                                  \
    char buf[FIX_PRINT_BUFFER_SIZE];                                              \
    fix_print(buf, lg2);                                                          \
    fix_buffer(buf, FIX_PRINT_BUFFER_SIZE);                                       \
    fprintf(fd, "  #define %-30s FIXNUM(%s) // 0x"FIX_PRINTF_HEX", actual %.20g, "\
      "difference: %.20g epsilon: %g larger: %d\n",                               \
      "FIX_TEST_LOG2_"#name, buf, lg2, actual,                                    \
          actual-fix_convert_to_double(lg2), fix_convert_to_double(FIX_EPSILON),  \
          fabs(actual - fix_convert_to_double(lg2)) > fix_convert_to_double(FIX_EPSILON));\
  } else {                                                                        \
    fprintf(fd, "  #define %-30s FIX_INF_NEG // actual: %g\n",                    \
      "FIX_TEST_LOG2_"#name, actual);                                             \
  }                                                                               \
};)

#define LOG2_TESTS            \
LOG2(epsilon, FIX_EPSILON)    \
LOG2(max,     FIX_MAX)
LOG2_TESTS

#define LOG10(name, op1)                                                          \
TEST_HELPER(log10_##name, {                                                       \
  fixed o1 = op1;                                                                 \
  fixed lg10 = fix_log10(o1);                                                     \
  double actual = (log10(fix_convert_to_double(op1)));                            \
  if(actual >= fix_convert_to_double(FIX_MIN)) {                                  \
    char buf[FIX_PRINT_BUFFER_SIZE];                                              \
    fix_print(buf, lg10);                                                         \
    fix_buffer(buf, FIX_PRINT_BUFFER_SIZE);                                       \
    fprintf(fd, "  #define %-30s FIXNUM(%s) // 0x"FIX_PRINTF_HEX", actual %.20g, "\
      "difference: %.20g epsilon: %g larger: %d\n",                               \
      "FIX_TEST_LOG10_"#name, buf, lg10, actual,                                  \
          actual-fix_convert_to_double(lg10), fix_convert_to_double(FIX_EPSILON), \
          fabs(actual - fix_convert_to_double(lg10)) > fix_convert_to_double(FIX_EPSILON));\
  } else {                                                                        \
    fprintf(fd, "  #define %-30s FIX_INF_NEG // actual: %g\n",                    \
      "FIX_TEST_LOG10_"#name, actual);                                            \
  }                                                                               \
};)

#define LOG10_TESTS            \
LOG10(epsilon, FIX_EPSILON)    \
LOG10(max,     FIX_MAX)
LOG10_TESTS

#undef TEST_HELPER
#define TEST_HELPER(name, code) cmocka_unit_test(name),

int main(int argc, char** argv) {

  const struct CMUnitTest tests[] = {
    PRINT_TESTS
    SQRT_TESTS
    LN_TESTS
    LOG2_TESTS
    LOG10_TESTS
  };

  char filename [40];
  sprintf((char*) filename, "test_helper.h"); \

  fd = fopen(filename, "a+");

  fprintf(fd, "#if FIX_INT_BITS == %d\n", FIX_INT_BITS);

  int i = cmocka_run_group_tests(tests, NULL, NULL);

  fprintf(fd, "#endif /* FIX_INT_BITS == %d */\n", FIX_INT_BITS);

  fclose(fd);

  return i;
}

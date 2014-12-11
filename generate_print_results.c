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


#include "fcntl.h"
#include "unistd.h"
#include <errno.h>

int fd;

void p(fixed f) {
  char buf[FIX_PRINT_BUFFER_SIZE];

  fix_print(buf, f);
  printf("n: %s ("FIX_PRINTF_HEX")\n", buf, f);
}

#define TEST_HELPER(name, code) static void name(void **state) code

#define PRINT(name, op1) \
TEST_HELPER(print_##name, { \
  fixed o1 = op1; \
  char buf[FIX_PRINT_BUFFER_SIZE]; \
  fix_print(buf, o1); \
  char fbuf[200]; \
  sprintf((char*) fbuf, "  #define %-30s \"%s\" // 0x"FIX_PRINTF_HEX"\n", "PRINT_TEST_" #name, buf, op1); \
  write(fd, fbuf, strlen(fbuf)); \
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

#undef TEST_HELPER
#define TEST_HELPER(name, code) unit_test(name),

int main(int argc, char** argv) {

  const UnitTest tests[] = {
    PRINT_TESTS
  };

  char filename [40];
  sprintf((char*) filename, "test_print_results.h"); \

  char buf[100];
  fd = open(filename, O_CREAT | O_WRONLY | O_APPEND, S_IWRITE | S_IREAD);

  sprintf((char*) buf, "#if FIX_INT_BITS == %d\n", FIX_INT_BITS);
  write(fd, buf, strlen(buf));

  int i = run_tests(tests);

  sprintf((char*) buf, "#endif /* FIX_INT_BITS == %d */\n", FIX_INT_BITS);
  write(fd, buf, strlen(buf));

  close(fd);

  return i;
}

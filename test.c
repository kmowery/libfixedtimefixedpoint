#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "ftfp.h"

static void null_test_success(void **state) {
  //(void) state;
}


void p(fixed f) {
  char buf[40];

  fix_print(buf, f);
  printf("n: %s\n", buf);
}

int main(int argc, char** argv) {

  fixed f = FIXINT(15);
  fixed g = FIXINT(1);
  fixed h;

  p(f);
  p(g);

  f = FIXINT(1 << 13);
  g = FIXINT(1 << 13);
  p(f);

  fix_add(f,g, &h);

  p(h);

  fix_neg(h,&h);
  p(h);

  f = FIXINT( 1<<12);
  fix_neg(f,&h);
  p(f);
  p(h);

  f = FIXINT( (1 << 14) -1 );
  p(f);

  const UnitTest tests[] = {
    unit_test(null_test_success),
  };
  return run_tests(tests);
}

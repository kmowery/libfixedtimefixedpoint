#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "ftfp.h"

#define PERF_ITRS 2000000

static inline uint64_t rdtscp(){
  uint64_t v;
  __asm__ volatile("rdtscp;"
                   "shl $32,%%rdx;"
                   "or %%rdx,%%rax;"
                   : "=a" (v)
                   :
                   : "%rcx","%rdx");

  return v;
}

#define TEST_INTERNALS( code ) \
  int ctr = 0; \
  uint64_t st = rdtscp(); \
  /* Offset for running the loop and rdtscp */ \
  for(ctr=0;ctr<PERF_ITRS;ctr++){ \
  } \
  uint64_t end = rdtscp(); \
  uint64_t offset = end-st; \
 \
  st = rdtscp(); \
  for(ctr=0;ctr<PERF_ITRS;ctr++){ \
    code; \
  } \
  end = rdtscp(); \
 \
  /* Run everything for real, previous was just warmup */ \
  st = rdtscp(); \
  for(ctr=0;ctr<PERF_ITRS;ctr++){ \
  } \
  end = rdtscp(); \
  offset = end-st; \
 \
  st = rdtscp(); \
  for(ctr=0;ctr<PERF_ITRS;ctr++){ \
    code; \
  } \
  end = rdtscp(); \
  printf("%s  %" PRIu64 "\n",name,(end-st-offset)/PERF_ITRS);

void run_test_d(char* name, fixed (*function) (fixed,fixed), fixed a, fixed b){
  TEST_INTERNALS( (*function)(a,b) );
}
void run_test_s(char* name, fixed (*function) (fixed), fixed a){
  TEST_INTERNALS( (*function)(a) );
}

void run_test_p(char* name, void (*function) (char*,fixed), fixed a){
  char buf[100];
  TEST_INTERNALS( (*function)(buf, a); )
}

void run_test_sb(char* name, int8_t (*function) (fixed), fixed a){
  TEST_INTERNALS( (*function)(a); )
}

void run_test_db(char* name, int8_t (*function) (fixed,fixed), fixed a, fixed b){
  TEST_INTERNALS( (*function)(a, b); )
}


int main(int argc, char* argv[]){
  printf(    "function ""  cycles\n");
  printf(    "=================\n");
  run_test_s ("fix_neg        ",fix_neg,10);
  run_test_s ("fix_abs        ",fix_abs,10);
  run_test_sb("fix_is_neg     ",fix_is_neg,10);
  run_test_sb("fix_is_nan     ",fix_is_nan,10);
  run_test_sb("fix_is_inf_pos ",fix_is_nan,10);
  run_test_sb("fix_is_inf_neg ",fix_is_nan,10);
  run_test_db("fix_eq         ",fix_eq,10,10);
  run_test_db("fix_eq_nan     ",fix_eq_nan,10,10);
  run_test_db("fix_cmp        ",fix_cmp,10,10);

  printf("\n");

  run_test_d ("fix_add        ",fix_add,0,0);
  run_test_d ("fix_sub        ",fix_sub,0,0);
  run_test_d ("fix_mul        ",fix_mul,0,0);
  run_test_d ("fix_div        ",fix_div,0,0);
  printf("\n");

  run_test_s ("fix_floor      ",fix_floor,10);
  run_test_s ("fix_ceil       ",fix_ceil,10);
  printf("\n");

  run_test_s ("fix_exp        ",fix_exp,10);
  run_test_s ("fix_ln         ",fix_ln,10);
  run_test_s ("fix_log2       ",fix_log2,10);
  run_test_s ("fix_log10      ",fix_log10,10);
  printf("\n");

  run_test_s ("fix_sqrt       ",fix_sqrt,10);
  run_test_d ("fix_pow        ",fix_pow,10,10);
  printf("\n");

  run_test_s ("fix_sin        ",fix_sin,10);
  run_test_s ("fix_cos        ",fix_cos,10);
  run_test_s ("fix_tan        ",fix_tan,10);
  //run_test_s ("fix_sin_fast   ",fix_sin_fast,10);
  printf("\n");

  run_test_p ("fix_sprint      ",fix_sprint,10);
}

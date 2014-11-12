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

void run_test_d(char* name, fixed (*function) (fixed,fixed), fixed a, fixed b){
  int ctr = 0;
  uint64_t st = rdtscp();
  // Offset for running the loop and rdtscp
  for(ctr=0;ctr<PERF_ITRS;ctr++){
  }
  uint64_t end = rdtscp();
  uint64_t offset = end-st;

  st = rdtscp();
  for(ctr=0;ctr<PERF_ITRS;ctr++){
    (*function)(a,b);
  }
  end = rdtscp();

  // Run everything for real, previous was just warmup
  st = rdtscp();
  for(ctr=0;ctr<PERF_ITRS;ctr++){
  }
  end = rdtscp();
  offset = end-st;

  st = rdtscp();
  for(ctr=0;ctr<PERF_ITRS;ctr++){
    (*function)(a,b);
  }
  end = rdtscp();

  printf("%s  %" PRIu64 "\n",name,(end-st-offset)/PERF_ITRS);
}
void run_test_s(char* name, fixed (*function) (fixed), fixed a){
  int ctr = 0;
  uint64_t st = rdtscp();
  // Offset for running the loop and rdtscp
  for(ctr=0;ctr<PERF_ITRS;ctr++){
  }
  uint64_t end = rdtscp();
  uint64_t offset = end-st;

  st = rdtscp();
  for(ctr=0;ctr<PERF_ITRS;ctr++){
    (*function)(a);
  }
  end = rdtscp();

  // Run everything for real, previous was just warmup
  st = rdtscp();
  for(ctr=0;ctr<PERF_ITRS;ctr++){
  }
  end = rdtscp();
  offset = end-st;

  st = rdtscp();
  for(ctr=0;ctr<PERF_ITRS;ctr++){
    (*function)(a);
  }
  end = rdtscp();


  printf("%s  %" PRIu64 "\n",name,(end-st-offset)/PERF_ITRS);
}

int main(int argc, char* argv[]){
  printf(    "function ""  cycles\n");
  printf(    "=================\n");
  run_test_s("fix_neg  ",fix_neg,10);
  run_test_s("fix_abs  ",fix_abs,10);

  run_test_d("fix_add  ",fix_add,0,0);
  run_test_d("fix_sub  ",fix_sub,0,0);
  run_test_d("fix_mul  ",fix_mul,0,0);
  run_test_d("fix_div  ",fix_div,0,0);

  run_test_s("fix_floor",fix_floor,10);
  run_test_s("fix_ceil ",fix_ceil,10);

  run_test_s("fix_exp  ",fix_exp,10);
  run_test_s("fix_ln   ",fix_ln,10);
  run_test_s("fix_log2 ",fix_log2,10);
  run_test_s("fix_log10",fix_log10,10);

  run_test_s("fix_sqrt ",fix_sqrt,10);

  run_test_s("fix_cordic_sin  ",fix_cordic_sin,10);
  run_test_s("fix_cordic_cos  ",fix_cordic_cos,10);
  run_test_s("fix_cordic_tan  ",fix_cordic_tan,10);

  run_test_s("fix_sin  ",fix_sin,10);
}

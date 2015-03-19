#include "debug.h"

#ifdef DEBUG

//void d(char* msg, fixed f) {
//  char buf[40];
//
//  fix_print_noflag(buf, f);
//  printf("%s: %8s (%08x)\n", msg, buf, f);
//}

void d64(char* msg, uint64_t f) {
  char buf[100];

  fix_sprint_nospecial(buf, f);
  printf("%s: %16s ("FIX_PRINTF_HEX")\n", msg, buf, f);
}

void fix_sprint_noflag(char* buffer, fixed f) {
  double d;
  fixed f_ = f;

  uint8_t neg = !!FIX_TOP_BIT(f);

  if(neg) {
    f_ = ~f_ + 4;
  }

  d = f_ >> FIX_POINT_BITS;
  uint64_t z = ((f_ & FIX_FRAC_MASK) >> FIX_FLAG_BITS);

  d += (z) / (double) (1ull<<FIX_FRAC_BITS);

  if(neg) {
    d *= -1;
  }

  sprintf(buffer, "% 16.16f", d);
}

void internald64(char* msg, fix_internal f) {
  char buf[100];
  fix_internal_print(buf, f);

  printf("%s: %16s ("FIX_PRINTF_HEX")\n", msg, buf, f);
}

void allfracd64(char* msg, fix_internal f) {
  char buf[100];
  fix_allfrac_print(buf, f);

  printf("%s: %16s ("FIX_PRINTF_HEX")\n", msg, buf, f);
}

//void fix_internal_print_noflag(char* buffer, fix_internal f) {
//  double d;
//  fixed f_ = f;
//
//  uint8_t neg = !!FIX_TOP_BIT(f);
//
//  if(neg) {
//    f_ = ~f_ + 1;
//  }
//
//  d = f_ >> FIX_INTERN_FRAC_BITS;
//  uint64_t z = ((f_ & ((1ull << (FIX_INTERN_FRAC_BITS)) -1)));
//
//  d += (z) / (double) (1ull<<FIX_INTERN_FRAC_BITS);
//
//  if(neg) {
//    d *= -1;
//  }
//
//  sprintf(buffer, "% 16.16f", d);
//}




void floatd64(char* msg, uint64_t f, uint16_t frac_bits) {
  char buf[40];

  fix_float_print_noflag(buf, f, frac_bits);
  printf("%s: %16s ("FIX_PRINTF_HEX")\n", msg, buf, f);
}

void fix_float_print_noflag(char* buffer, fix_internal f, uint16_t frac_bits) {
  double d;
  fixed f_ = f;

  uint8_t neg = !!FIX_TOP_BIT(f);

  if(neg) {
    f_ = ~f_ + 1;
  }

  d = f_ >> frac_bits;
  uint64_t z = ((f_ & ((1ull << (frac_bits)) -1)));

  d += (z) / (double) (1ull<<frac_bits);

  if(neg) {
    d *= -1;
  }

  sprintf(buffer, "% 16.16f", d);
}

// Note that this is not constant time.
void fix_sprint_variable(char* buffer, fixed f) {
  double d;
  fixed f_ = f;

  if(FIX_IS_NAN(f)) {
    memcpy(buffer, "NaN", 4);
    return;
  }
  if(FIX_IS_INF_POS(f)) {
    memcpy(buffer, "+Inf", 5);
    return;
  }
  if(FIX_IS_INF_NEG(f)) {
    memcpy(buffer, "-Inf", 5);
    return;
  }

  uint8_t neg = !!FIX_TOP_BIT(f);

  if(neg) {
    f_ = ~f_ + 4;
  }

  d = f_ >> 17;
  d += ((f_ >> 2) & 0x7fff) / (float) (1<<15);

  if(neg) {
    d *= -1;
  }

  sprintf(buffer, "%.015f", d);
}


#endif

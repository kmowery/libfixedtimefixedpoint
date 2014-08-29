#include "ftfp.h"

void fix_neg(fixed op1, fixed* result){
  uint8_t isinfpos;
  uint8_t isinfneg;
  uint8_t isnan;

  fixed tempresult;

  if(result == NULL) {
    // TODO: we can't be constant time if there's no output.
    return;
  }

  // Flip our infs
  isinfpos = FIX_IS_INF_NEG(op1);
  isinfneg = FIX_IS_INF_POS(op1);

  // NaN is still NaN
  isnan = FIX_IS_NAN(op1);

  // 2s comp negate the data bits
  tempresult = DATA_BITS(((~op1) + 4));

  //TODO: Check overflow? other issues?


  // Combine
  *result = ( isnan ? F_NAN : 0 ) |
    ( isinfpos ? F_INF_POS : 0 ) |
    ( isinfneg ? F_INF_NEG : 0 ) |
    tempresult;
}

void fix_sub(fixed op1, fixed op2, fixed* result) {
  fixed tmp;

  if(result == NULL) {
    // TODO: we can't be constant time if there's no output.
    return;
  }

  fix_neg(op2,&tmp);
  fix_add(op1,op2,result);
}

void fix_mul(fixed op1, fixed op2, fixed* result) {

  uint8_t isnan;
  uint8_t isinfpos;
  uint8_t isinfneg;

  uint8_t isinfop1;
  uint8_t isinfop2;

  fixed tempresult;

  if(result == NULL) {
    // TODO: we can't be constant time if there's no output.
    return;
  }

  isnan = FIX_IS_NAN(op1) | FIX_IS_NAN(op2);

  isinfop1 = (FIX_IS_INF_NEG(op1) | FIX_IS_INF_POS(op1));
  isinfop2 = (FIX_IS_INF_NEG(op2) | FIX_IS_INF_POS(op2));

  //TODO These seem excessive...
  // We may also wish to help the compiler by using some local variables
  isinfpos = (isinfop1 | isinfop2) &
    ((FIX_IS_INF_POS(op1) & !(FIX_IS_INF_NEG(op2) | FIX_IS_NEG(op2))) |
     (FIX_IS_INF_NEG(op1) & (FIX_IS_INF_NEG(op2) | FIX_IS_NEG(op2))) |
     (FIX_IS_INF_POS(op2) & !(FIX_IS_INF_NEG(op1) | FIX_IS_NEG(op1))) |
     (FIX_IS_INF_NEG(op2) & (FIX_IS_INF_NEG(op1) | FIX_IS_NEG(op1))));

  isinfpos = (isinfop1 | isinfop2) &
    ((FIX_IS_INF_NEG(op1) & !(FIX_IS_INF_NEG(op2) | FIX_IS_NEG(op2))) |
     (FIX_IS_INF_POS(op1) & (FIX_IS_INF_NEG(op2) | FIX_IS_NEG(op2))) |
     (FIX_IS_INF_NEG(op2) & !(FIX_IS_INF_NEG(op1) | FIX_IS_NEG(op1))) |
     (FIX_IS_INF_POS(op2) & (FIX_IS_INF_NEG(op1) | FIX_IS_NEG(op1))));

}


void fix_add(fixed op1, fixed op2, fixed* result) {

  uint8_t isnan;
  uint8_t isinfpos;
  uint8_t isinfneg;

  fixed tempresult;

  if(result == NULL) {
    // TODO: we can't be constant time if there's no output.
    return;
  }

  //TODO: One of the INFs needs to 'win' if we get -inf and inf.
  isnan = FIX_IS_NAN(op1) | FIX_IS_NAN(op2);
  isinfpos = FIX_IS_INF_POS(op1) | FIX_IS_INF_POS(op2);
  isinfneg = FIX_IS_INF_NEG(op1) | FIX_IS_INF_NEG(op2);

  tempresult = op1 + op2;

  // check if we're overflowing: adding two positive numbers that results in a
  // 'negative' number:
  //   if both inputs are positive (top bit == 0) and the result is 'negative'
  //   (top bit nonzero)
  isinfpos |= (TOP_BIT(op1) | TOP_BIT(op2)) ==
    0x0 && (TOP_BIT(tempresult) != 0x0);

  // check if there's negative infinity overflow
  isinfneg |= (TOP_BIT(op1) & TOP_BIT(op2)) ==
    TOP_BIT_MASK && (TOP_BIT(tempresult) == 0x0);

  //printf("isnan: %x\n", isnan);
  //printf("isinfpos: %x\n", isinfpos);
  //printf("isinfneg: %x\n", isinfneg);
  //printf("bits: %x\n", isnan | isinfpos | isinfneg );
  //printf("result: %x\n", tempresult);
  //printf("bit: %x\n", !(isnan | isinfpos | isinfneg));
  //printf("extend: %x\n", EXTEND_BIT_32(!(isnan | isinfpos | isinfneg)));

  // do some horrible bit-ops to make result into what we want
  *result = ( isnan ? F_NAN : 0 ) |
     ( isinfpos ? F_INF_POS : 0 ) |
     ( isinfneg ? F_INF_NEG : 0 ) |
     ( DATA_BITS(tempresult));
}

// TODO: not constant time. will work for now.
void fix_print(char* buffer, fixed f) {
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

  uint8_t neg = !!TOP_BIT(f);

  if(neg) {
    f_ = ~f_ + 4;
  }

  d = f_ >> 17;
  d += ((f_ >> 2) & 0x7fff) / (float) (1<<15);

  if(neg) {
    d *= -1;
  }

  sprintf(buffer, "%f", d);
}

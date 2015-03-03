#ifndef cordic_h
#define cordic_h

#include "ftfp.h"
#include "internal.h"
#include "lut.h"

FIX_INLINE void cordic(fix_internal* Zext, fix_internal* Cext, fix_internal* Sext) {
  /* See http://math.exeter.edu/rparris/peanut/cordic.pdf for the best
   * explanation of CORDIC I've found.
   */

  /* Use circle fractions instead of angles. Will be [0,4) in 2.28 format. */
  /* Use 2.28 notation for angles and constants. */

  /* Generate the cordic angles in terms of circle fractions:
   * ", ".join(["0x%08x"%((math.atan((1/2.)**i) / (math.pi/2)*2**28)) for i in range(0,24)])
   */
  CORDIC_LUT;

  fix_internal Z = *Zext;
  fix_internal C = *Cext;
  fix_internal S = *Sext;

  fix_internal C_ = 0;
  fix_internal S_ = 0;
  fix_internal pow2 = 0;

  /* D should be 1 if Z is positive, or -1 if Z is negative. */
  fix_internal D = SIGN_EX_SHIFT_RIGHT(Z, (FIX_INTERN_FRAC_BITS + FIX_INTERN_INT_BITS -1)) | 1;

  uint8_t overflow = 0;

  for(int m = 0; m < CORDIC_N; m++) {
    pow2 = ((fix_internal) 2) << (FIX_INTERN_FRAC_BITS - 1 - m);

    /* generate the m+1th values of Z, C, S, and D */
    Z = Z - D * cordic_lut[m];

    C_ = C - D*FIX_MUL_INTERN(pow2, S, overflow);
    S_ = S + D*FIX_MUL_INTERN(pow2, C, overflow);

    C = C_;
    S = S_;
    D = SIGN_EX_SHIFT_RIGHT(Z, (FIX_INTERN_FRAC_BITS + FIX_INTERN_INT_BITS -1)) | 1;
  }

  *Zext = Z;
  *Cext = C;
  *Sext = S;

  return;

}

#endif

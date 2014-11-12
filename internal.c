#include "ftfp.h"
#include "internal.h"

inline uint32_t uint32_log2(uint32_t o) {
  uint32_t scratch = o;
  uint32_t log2;
  uint32_t shift;

  log2 =  (scratch > 0xFFFF) << 4; scratch >>= log2;
  shift = (scratch >   0xFF) << 3; scratch >>= shift; log2 |= shift;
  shift = (scratch >    0xF) << 2; scratch >>= shift; log2 |= shift;
  shift = (scratch >    0x3) << 1; scratch >>= shift; log2 |= shift;
  log2 |= (scratch >> 1);
  return log2;
}

uint32_t fix_circle_frac(fixed op1) {
  /* Scratchpad to compute z:
   *
   * variables are lowercase, and considered as integers.
   * real numbers are capitalized, and are the space we're trying to work in
   *
   * op1: input. fixed: 15.15.b00
   * X = op1 / 2^17                          # in radians
   * TAU = 6.28318530718...                  # 2pi
   * QTAU = TAU/4
   *
   * Z = (X / QTAU) % 4                      # the dimensionless circle fraction
   *
   * circle_frac = Z * 2^28                  # will fit in 30 bits, 2 for extra
   *
   * big_op = op1 << 32
   * BIG_OP = X * 2^32
   *
   * big_qtau = floor(QTAU * 2^(17+32-32+4)) # remove 32-4 bits so that big_op /
   *          = floor(QTAU * 2^21)           # big_tau has 28-bits of fraction and
   *                                         # 2 bits of integer
   *
   * circle_frac = big_op / big_qtau
   *   = X * 2^32 / floor(QTAU * 2^21)
   *  ~= X * 2^11 / QTAU
   *   = (X / QTAU) * 2^11
   *  ~= (op1 / QTAU / 2^17) * 2^11
   *   = (op1 / QTAU) * 2^28                # in [0,4), fills 30 bits at 2.28
   *
   */

  int64_t big_op = ((int64_t) ((int32_t) FIX_DATA_BITS(op1))) << 32;
  int32_t big_tau = 0x3243f6;  // in python: "0x%x"%(math.floor((math.pi / 2) * 2**21))
  int32_t circle_frac = (big_op / big_tau) & 0x3fffffff;
  return circle_frac;
}


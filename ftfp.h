#ifndef ftfp_h
#define ftfp_h

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint32_t fixed;

#define FLAGS_MASK 0x3
#define F_NORMAL   0x0
#define F_NAN      0x1
#define F_INF_POS  0x2
#define F_INF_NEG  0x3

#define ALL_BIT_MASK 0xffffffff

#define TOP_BIT_MASK (1<<31)
#define TOP_BIT(f) (f & TOP_BIT_MASK)

#define FIX_IS_NAN(f) ((f&FLAGS_MASK) == F_NAN)
#define FIX_IS_INF_POS(f) ((f&FLAGS_MASK) == F_INF_POS)
#define FIX_IS_INF_NEG(f) ((f&FLAGS_MASK) == F_INF_NEG)

#define FIXINT(z) ((z)<<(2+15))

#define EXTEND_BIT_32(b) ({ uint32_t v = b; \
  v |= v << 1; \
  v |= v << 2; \
  v |= v << 4; \
  v |= v << 8; \
  v |= v << 16; \
  v; })


void fix_add(fixed op1, fixed op2, fixed* result);

void fix_print(char* buffer, fixed f);


/*
 * add
 * compare?
 * div
 * sub
 * mul
 * round to integer
 * exp
 *
 * sqrt
 * sin / cos / arctan
 *
 * important constants?
 */

#endif

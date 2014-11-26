#ifndef base_h
#define base_h

typedef uint64_t fixed;

#define FIX_PRINTF_HEX "%016llx"

#define FIX_FLAG_BITS 2
#define FIX_FRAC_BITS 31
#define FIX_INT_BITS  31
#define FIX_BITS (8*sizeof(fixed))

#endif

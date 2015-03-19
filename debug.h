#ifndef debug_h
#define debug_h

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ftfp.h"
#include "internal.h"

#ifdef DEBUG
void d(char* msg, fixed f);
void d64(char* msg, uint64_t f);
void fix_sprint_noflag(char* buffer, fixed f);

void internald64(char* msg, fix_internal f);
//void fix_internal_print_noflag(char* buffer, fix_internal f);

void allfracd64(char* msg, fix_internal f);

void fix_float_print_noflag(char* buffer, fix_internal f, uint16_t frac_bits);
void floatd64(char* msg, uint64_t f, uint16_t frac_bits);

/* Note that this is not constant time, but will return a buffer sized to the
 * number. */
void fix_sprint_variable(char* buffer, fixed f);
#endif

#endif

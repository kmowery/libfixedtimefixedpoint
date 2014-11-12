#ifndef debug_h
#define debug_h

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ftfp.h"

void d(char* msg, fixed f);
void d64(char* msg, uint64_t f);
void fix_print_noflag(char* buffer, fixed f);

#endif

#!/usr/bin/env python
import sys
import math
import decimal
from decimal import Decimal

# We might overwrite base.py; make sure we don't generate bytecode...
sys.dont_write_bytecode = True

# If we've customized things, keep the customizations. Otherwise, revert to
# something reasonable...
try:
  from base import *
except:
  flag_bits = 2
  int_bits  = 31
  frac_bits = (64 - flag_bits - int_bits)

  internal_frac_bits = 60
  internal_int_bits = 4

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('--intbits', metavar='N', type=int, nargs='?',
                     help='The number of integer bits', default = None)
    parser.add_argument('--flagbits', metavar='N', type=int, nargs='?',
                     help='The number of flag bits', default = None)
    parser.add_argument('--fracbits', metavar='N', type=int, nargs='?',
                     help='The number of fraction bits', default = None)
    parser.add_argument('--file', metavar='filename', nargs='?', type=argparse.FileType(mode="w"),
                     help='The filename to write to', default = None)
    parser.add_argument('--pyfile', metavar='filename', nargs='?', type=argparse.FileType(mode="w"),
                     help='The filename to write the python base to', default = None)
    parser.add_argument('--lutfile', metavar='filename', nargs='?', type=argparse.FileType(mode="w"),
                     help='The filename to write the C LUT file to', default = None)
    args = vars(parser.parse_args())

    if args["flagbits"] is not None:
        flag_bits = args["flagbits"]

    aint = args["intbits"]
    afrac = args["fracbits"]

    if aint is None and afrac is None:
        pass
    elif aint is None and afrac is not None:
        frac_bits = afrac
        int_bits = (64 - flag_bits - frac_bits)
    elif aint is not None and afrac is None:
        int_bits = aint
        frac_bits = (64 - flag_bits - int_bits)
    elif aint is not None and afrac is not None:
        int_bits = aint
        frac_bits = afrac

    # Check sanity
    if (flag_bits + int_bits + frac_bits) > 64:
        print "Too many bits! (%d (flag) + %d (int) + %d (frac) > 64)"%(flag_bits, int_bits, frac_bits)
        sys.exit(1)
    if (flag_bits + int_bits + frac_bits) < 64:
        print "Not enough bits! (%d (flag) + %d (int) + %d (frac) < 64)"%(flag_bits, int_bits, frac_bits)
        sys.exit(1)
    if int_bits < 1:
        print "There must be at least one integer bit (for two's complement...)"
        print "You asked for %d (flag), %d (int), and %d (frac)"%(flag_bits, int_bits, frac_bits)
        sys.exit(1)

    # Generate the buffer size for printing
    int_chars = max(1, int(math.ceil(math.log(2**(int_bits-1),10))))
    frac_chars = frac_bits
    sign_char = 1
    point_char = 1
    null_byte = 1
    buffer_length = sign_char + int_chars + point_char + frac_chars + null_byte

    # Generate constants
    fix_inf_pos = 0x2
    point_bits = frac_bits+flag_bits
    pi  = decimal.Decimal('3.1415926535897932385')
    tau = decimal.Decimal('6.2831853071795864769')
    e   = decimal.Decimal('2.7182818284590452354')

    def decimal_to_fix(d):
      t = (d * 2**(point_bits-2))
      t = t.quantize(decimal.Decimal('1.'), rounding=decimal.ROUND_HALF_EVEN)
      t = t * 2**2
      return t
    def decimal_to_fix_extrabits(d, fracbits = point_bits):
      t = (d * 2**(fracbits))
      t = t.quantize(decimal.Decimal('1.'), rounding=decimal.ROUND_HALF_EVEN)
      if t < 0:
          t = Decimal(2**64) + t + 1
      return t

    fix_pi =  fix_inf_pos if int_bits < 3 else decimal_to_fix(pi)
    fix_tau = fix_inf_pos if int_bits < 4 else decimal_to_fix(tau)
    fix_e =   fix_inf_pos if int_bits < 3 else decimal_to_fix(e)

    # Generate LUTs

    def make_c_lut(lut, name):
        l = ["  0x%016x"%(decimal_to_fix_extrabits(x)) for x in lut]
        return "fixed %s[%d] = {\n"%(name, len(lut)) + \
               ",\n".join(l) + \
               "\n};\n"
    def make_c_internal_lut(lut, name):
        l = ["  0x%016x"%(decimal_to_fix_extrabits(x, internal_frac_bits)) for x in lut]
        return "fix_internal %s[%d] = {\n"%(name, len(lut)) + \
               ",\n".join(l) + \
               "\n};\n"
    def make_c_internal_defines(lut, name):
        l = ["#define %s_%d ((fix_internal) 0x%016x)"%(name, i, decimal_to_fix_extrabits(x, internal_frac_bits)) for i,x in enumerate(lut)]
        return "\n".join(l) + "\n"

    # note that 1/0 isn't very useful, so just call it 1
    internal_inv_integer_lut = [Decimal('1')] + [((decimal.Decimal('1')/decimal.Decimal(x))) for x in range(1,25)]
    ln_coef_lut = [Decimal('0.0016419'), Decimal('0.9961764'), Decimal('-0.5624485'), Decimal('0.4004560')]
    log2_coef_lut = [Decimal('0.0023697'), Decimal('1.4371765'), Decimal('-0.8114606'), Decimal('0.5777570')]

    # Write files

    if args["pyfile"] is not None:
        with args["pyfile"] as f:
            f.write("flag_bits = %d\n" %( flag_bits ))
            f.write("int_bits = %d\n" %( int_bits ))
            f.write("frac_bits = %d\n" %( frac_bits ))
            f.write("internal_frac_bits = 60\n");
            f.write("internal_int_bits = 4\n");

    if args["file"] is not None:
        with args["file"] as f:
          baseh = """/*********
 * This file is autogenerated by generate_base.py.
 * Please don't modify it manually.
 *********/

#ifndef base_h
#define base_h

#include <stdint.h>
#include <inttypes.h>

typedef uint64_t fixed;

#define FIX_PRINT_BUFFER_SIZE %d

#define FIX_PRINTF_HEX "%%016"PRIx64
#define FIX_PRINTF_DEC "%%"PRId64

#define FIX_FLAG_BITS %d
#define FIX_FRAC_BITS %d
#define FIX_INT_BITS  %d
#define FIX_BITS (8*sizeof(fixed))


static const fixed fix_pi = 0x%016x;
static const fixed fix_tau = 0x%016x;
static const fixed fix_e = 0x%016x;

#define FIX_PI fix_pi
#define FIX_TAU fix_tau
#define FIX_E fix_e

#endif"""%(buffer_length, flag_bits, frac_bits, int_bits,fix_pi,fix_tau,fix_e)
          f.write(baseh)

    if args["lutfile"] is not None:
        with args["lutfile"] as f:
            lutc  = '#ifndef LUT_H\n'
            lutc += '#define LUT_H\n'
            lutc += '\n'
            lutc += '#include "base.h"\n'
            lutc += '#include "internal.h"\n'
            lutc += "\n"
            lutc += (make_c_internal_lut(internal_inv_integer_lut, "LUT_int_inv_integer"))
            lutc += (make_c_internal_defines(ln_coef_lut, "FIX_LN_COEF"))
            lutc += (make_c_internal_defines(log2_coef_lut, "FIX_LOG2_COEF"))
            lutc += "\n#endif\n"
            f.write(lutc)


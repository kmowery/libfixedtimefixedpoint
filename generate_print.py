#!/usr/bin/env python3
import math
import argparse
import sys

# This file generates C code to print a fixed point number.
#
# Printing must be constant time, and we need to support variable-sized integer
# and fraction fields (as long as they add to 62).
#
# Strategy:
#
# Each binary fixed-point number has exactly one decimal expansion, with a fixed
# number of non-zero digits. Also, the binary number can be considered as the
# sum of each of the numbers represented by each of its bits.
#
# We're going to exploit these facts. First, we calculate the largest number of decimal
# digits that a given fixed-point format (N intbits, 62-N fracbits) can generate:
#
#   integer digits = ceil( log_10 ( 2^intbits ) )
#   fractional digits = fracbits
#   total size = integer digits + fractional digits + 2 (sign and decimal point)
#
# Now, we start generating at the absolute smallest decimal digit. By the
# decimal expansion of 2^-i, only one bit (the 0th) can contribute anything
# different than 0 to this deciaml digit. So, compute this final digit based on
# the last bit.
#
# Moving up one decimal spot, two bits can add their value. Compute this sum,
# but remember that we might have carry from the previous digit.
#
# Continue in this manner, producing the decimal digit for each location, until
# the entire number is printed.
#
# To support printing "NaN" and "+Inf", etc., add a term to the polynomial,
# which multiplies by 0 if an exceptional state, zeroing out the normal
# computation, and adding back in a " "  if necessary.

# base.py might be overwritten; make sure we don't generate bytecode...
sys.dont_write_bytecode = True

try:
  from base import *
except:
    print("You must run generate_base.py before running generate_print.py.")
    sys.exit(1)

parser = argparse.ArgumentParser(description="Generate libftfp's print function")
parser.add_argument('--file', metavar='filename', nargs='?', type=argparse.FileType(mode="w"),
                     help='The file to write to', default = "autogen.c")
args = vars(parser.parse_args())



def make_print_function(preamble, int_bits, frac_bits, flag_bits):
    number_bits = int_bits + frac_bits

    # characters in the integer is given by the base 10 log of the maximum number
    # but since we're two's complement, we subtract 1 power of two
    int_chars = int(math.ceil(math.log(2**(int_bits-1),10)))

    #Ensure we have at least one character...
    if int_chars == 0:
        int_chars = 1

    #characters in the base-10 significand is exactly the number of bits
    #  0.5, 0.25, 0.125, etc.
    # Each step involves extending another power of ten down...
    frac_chars = frac_bits

    sign_char = 1
    point_char = 1

    sign_loc = 0
    int_loc = sign_char
    point_loc = sign_char + int_chars
    frac_loc = sign_char + int_chars + point_char
    length = sign_char + int_chars + point_char + frac_chars

    ########

    bits = [2**i for i in range(-frac_bits, int_bits)]

    # Ensure the patterns are up to snuff
    fmt = "%%0%d.%df"%(length - sign_char, frac_chars)
    dec_patterns = [fmt%(b) for b in bits]

    #generate the list of contributory things
    fraction_patterns = [s[int_chars+sign_char:] for s in dec_patterns]
    int_patterns = [s[:int_chars] for s in dec_patterns]

    # make a dictionary of dictionaries for fraction patterns.
    # Top level key: character in fraction [0, frac_chars)
    # Second level key: contributory bit
    # Second level value: resulting decimal digit
    fracs = {i: {bit: fraction_patterns[bit][i] for bit in range(len(fraction_patterns))} for i in range(frac_chars)}
    ints =  {i: {bit: int_patterns[bit][i]      for bit in range(len(int_patterns))}      for i in range(int_chars)}

    def frac_poly(n):
        patterns = [(k,v) for k, v in fracs[n].items()]
        terms = ["(%c * bit%d)"%(p, bit+flag_bits) for bit, p in patterns if p != "0"]
        return " + ".join(terms)

    def int_poly(n):
        patterns = [(k,v) for k, v in ints[n].items()]
        terms = ["(%c * bit%d)"%(p, bit+flag_bits) for bit, p in patterns if p != "0"]
        return " + ".join(terms)


    f = preamble;
    f += """
  uint8_t excep = isinfpos | isinfneg | isnan;

  uint32_t carry = 0;
  uint32_t scratch = 0;
  """

    f += "\n".join(["  uint8_t bit%d = (((f) >> (%d))&1);"%(i,i) for i in range(flag_bits,flag_bits+number_bits)])

    extra_polynomials = {
            1: " + (isnan * %d) + (isinfpos | isinfneg) * %d"%(ord('N'), ord('I')),
            2: " + (isnan * %d) + (isinfpos | isinfneg) * %d"%(ord('a'), ord('n')),
            3: " + (isnan * %d) + (isinfpos | isinfneg) * %d"%(ord('N'), ord('f')),
            }
    def get_extra_poly(n):
        return extra_polynomials.get(n, "+ (excep * %d)"%(ord(' ')))

    # start at the end and move forward...
    for position in reversed(range(frac_chars)):
        poly = frac_poly(position)
        f += ("  scratch = %s + carry;"%(poly))
        f += ("  buffer[%d] = ((%d + (scratch %% 10)) * (1-excep) %s);\n"%(
            frac_loc+position, ord('0'), get_extra_poly(frac_loc+position)))
        f += ("carry = scratch / 10;\n")

    f += ("\n");
    f += ("  buffer[%d] = %s + (1-excep)*%d;"%(point_loc, get_extra_poly(point_loc), ord('.')))
    f += ("\n");

    for position in reversed(range(int_chars)):
        poly = int_poly(position)
        f += ("  scratch = %s + carry;\n"%(poly))
        f += ("  buffer[%d] = ((%d + (scratch %% 10)) * (1-excep)) %s;\n"%(int_loc + position, ord('0'),
            get_extra_poly(int_loc+position)))
        f += ("carry = scratch / 10;\n")

    f += ("""
      uint8_t n = (neg*(1-excep) + isinfneg);
      buffer[0] = %d * n + %d * (1-n);
    """%(ord('-'), ord(' ')))

    f += ("  buffer[%d] = '\\0';"%(length))

    f += ("""}
    """)
    return f




with args["file"] as f:
    f.write( """#include "ftfp.h"
#include "internal.h"

/*********
 * This file is autogenerated by generate_print.py.
 * Please don't modify it manually.
 *********/

""")

    f.write(make_print_function("""void fix_sprint(char* buffer, fixed f) {
  uint8_t isinfpos = FIX_IS_INF_POS(f);
  uint8_t isinfneg = FIX_IS_INF_NEG(f);
  uint8_t isnan = FIX_IS_NAN(f);
  uint32_t neg = !!FIX_TOP_BIT(f);
  f = fix_abs(f);""",
  int_bits, frac_bits, flag_bits))

    f.write("\n\n");

    f.write("#ifdef DEBUG\n")

    f.write(make_print_function( """void fix_sprint_nospecial(char* buffer, fixed f) {
  uint8_t isinfpos = 0;
  uint8_t isinfneg = 0;
  uint8_t isnan = 0;
  uint32_t neg = !!FIX_TOP_BIT(f);
  f = fix_abs(f);""",
  int_bits, frac_bits, flag_bits))

    f.write(make_print_function( """void fix_internal_print(char* buffer, fix_internal f) {
  uint8_t isinfpos = 0;
  uint8_t isinfneg = 0;
  uint8_t isnan = 0;
  uint32_t neg = !!FIX_TOP_BIT(f);
  f = fix_abs(f);""",
  internal_int_bits, internal_frac_bits, 0))

    f.write(make_print_function( """void fix_allfrac_print(char* buffer, fix_internal f) {
  uint8_t isinfpos = 0;
  uint8_t isinfneg = 0;
  uint8_t isnan = 0;
  uint32_t neg = 0;
  """,
  0, 64, 0))
    f.write("#endif\n")

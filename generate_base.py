#!/usr/bin/env python
import sys

flag_bits = 2
int_bits  = 31
frac_bits = (64 - flag_bits - int_bits)

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
                     help='The filename to write to', default = "base.h")
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

    with args["file"] as f:
        f.write(
          """/*********
 * This file is autogenerated by generate_base.py.
 * Please don't modify it manually.
 *********/

#ifndef base_h
#define base_h

typedef uint64_t fixed;

#define FIX_PRINTF_HEX "%%016llx"

#define FIX_FLAG_BITS %d
#define FIX_FRAC_BITS %d
#define FIX_INT_BITS  %d
#define FIX_BITS (8*sizeof(fixed))

#endif"""%(flag_bits, frac_bits, int_bits))

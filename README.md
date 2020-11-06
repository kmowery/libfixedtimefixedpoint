# libfixedtimefixedpoint

This library provides constant-time mathematical operations on fixed point numbers.

You can read more about this library, and why it was developed in our
paper: [On Subnormal Floating Point and Abnormal Timing](https://homes.cs.washington.edu/~dkohlbre/papers/subnormal.pdf).

If you are using libftfp in an academic paper, please cite as:

```
@inproceedings{andrysco2015subnormal,
  title={On subnormal floating point and abnormal timing},
  author={Andrysco, Marc and Kohlbrenner, David and Mowery, Keaton and Jhala, Ranjit and Lerner, Sorin and Shacham, Hovav},
  booktitle={2015 IEEE Symposium on Security and Privacy},
  pages={623--639},
  year={2015},
  organization={IEEE}
}
```


## Why

Operations on floating point numbers (`float` or `double`), even simple ones
such as add and multiply, can take varying amounts of time depending on the
numbers inovlved. If you use floating point operations in your security-critical
application, malicious attackers could use this timing side-channel against you.

## Usage

The functions provided by libftfp are outlined in `ftfp.h`. These include:

  * Arithmetic: Add, Subtract, Multiply, Divide
  * Sign adjustment: Absolute Value, Negation
  * Rounding: Floor and Ceiling
  * Exponentials: ex , log2 (x), loge (x), log10 (x)
  * Powers: x^y , Square root
  * Trigonometry: Sine, Cosine, Tangent
  * Conversion: Printing (Base 10), To/From double

Your application should link against the libftfp shared library, which is built
by our Makefile.

## Representation

Each libftfp number is stored in a 64-bit value. 2 bits are reserved for
metadata, leaving 62 bits for the number itself.

First, select your preferred precision: libftfp allocates 62 bits for each
number, and you can use between 1 and 61 of these for the integer portion, with
the rest allocated to the fractional portion of the number.


## Dependencies

```
   python3
   python3 mpmath
   cmocka
```

## Building

To select 32 integer bits, run:

    $ python generate_base.py --file base.h --pyfile base.py --intbits 32

Acceptable values are between 1 and 61. If you prefer, you can modify `base.py`
directly. Next,

    $ make

This will generate `libftfp.so`, which you can then link against in your
application.

If you'd like to run the full test suite on your local machine (which iterates
through every possible configuration of the library), run

    $ make run_tests

You will need to have installed `cmocka` to run tests.

## Behavioral Notes
 * Inf is infinity
 * (-) indicated - or + versions of the value
 * N is any number (including infinity, not including NaN)

NaN in any operation will always result in NaN.

### Add
 * Inf + (-)Inf = +Inf

### Sub
 * Inf - Inf = +Inf

### Mul
 * (-)Inf * 0 = 0
 * Inf * -N = -Inf

### Div
 * (-)Inf / (-)Inf = (-)Inf
 * 0 / (-)Inf = 0
 * (-)N / 0 = (-)Inf
 * 0 / 0 = NaN

# libfixedtimefixedpoint

This library provides constant-time mathematical operations on fixed point numbers.

## Why

Operations on floating point numbers (`float` or `double`), even simple ones such as add and multiply, can take different amounts of time depending on the operands. If you use these numbers in your security-critical application, malicious attackers could use this timing side-channel against you.

## Usage

The functions provided by libftfp are outlined in `ftfp.h`. These include:

  * Arithmetic: Add, Subtract, Multiply, Divide
  * Sign adjustment: Absolute Value, Negation
  * Rounding: Floor and Ceiling
  * Exponentials: ex , log2 (x), loge (x), log10 (x)
  * Powers: x^y , Square root
  * Trigonometry: Sine, Cosine, Tangent
  * Conversion: Printing (Base 10), To/From double

Your application should link against the libftfp shared library, which is built by our Makefile.

## Representation

Each number is stored in 32 bits.

libftfp currently uses a 15-15 format for numbers. That is, 15 bits are reserved for the integer portion and 15 for the fractional portion.

This representation is expected to change.

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

## Static Build
We manually verify an x86_64 build for each major version.

The current verified version is: 1.0

Sha1sum 8aee2d5237ef9ecf67d704bd7bd165da7fc92d51

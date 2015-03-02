#include "ftfp.h"
#include "internal.h"

#include "debug.h"

/*
 * Take a string consisting solely of digits, and produce a 64-bit number which
 * corresponds to:
 *
 *   (atoi(str + padding) / 10**20) * 2^64
 *
 * where padding is "0" * (20 - strlen(str)).
 *
 * More preicsely, given "5", it will should produce 0.5 * 2^64, or
 *               0x8000000000000000.
 * Given "25",   0x4000000000000000.
 * Given "125",  0x2000000000000000.
 * Given "0625", 0x1000000000000000.
 * Given "99999999999999999999", 0xffffffffffffffff.
 *
 * And so on.
 *
 * The basic strategy is to use LUTs for the binary values of 0.1, 0.01, and so on,
 * and add these together as we examine each decimal digit.
 *
 * Not constant time, but that could be fixed.
 *
 * We want a result rounded to 64 bits, but we need more than 64 bits to make
 * that happen. Therefore, we're going to use two luts, one for the main value,
 * and one for the extra bits (for rounding).
 *
 * */
uint64_t fixfrac(char* frac) {
    /* To generate these luts:
     *
     * source_bits = [(2**64 / 10**i, (256*2**64 / 10**i)%256) for i in range(1,21)]
     * print "uint64_t pow10_LUT = { %s };"%(", ".join("0x%016x"%(x) for x,y in source_bits))
     * print "uint64_t pow10_extraLUT = { %s };"%(", ".join("0x%02x"%(y) for x,y in source_bits))
     */
    uint64_t pow10_LUT[20] = {
        0x1999999999999999, 0x028f5c28f5c28f5c, 0x004189374bc6a7ef,
        0x00068db8bac710cb, 0x0000a7c5ac471b47, 0x000010c6f7a0b5ed,
        0x000001ad7f29abca, 0x0000002af31dc461, 0x000000044b82fa09,
        0x000000006df37f67, 0x000000000afebff0, 0x0000000001197998,
        0x00000000001c25c2, 0x000000000002d093, 0x000000000000480e,
        0x0000000000000734, 0x00000000000000b8, 0x0000000000000012,
        0x0000000000000001, 0x0000000000000000,
    };

    uint64_t pow10_LUT_extra[20] = {
        0x99, 0x28, 0x9d, 0x29, 0x84, 0x8d, 0xf4, 0x18, 0xb5, 0x5e, 0xbc, 0x12,
        0x68, 0x70, 0xbe, 0xac, 0x77, 0x72, 0xd8, 0x2f
    };

    uint64_t result = 0;
    uint64_t extra = 0;

    for(int i = 0; i < 20; i++) {
        if(frac[i] == '\0') {
            break;
        }

        uint8_t digit = (frac[i] - (uint8_t) '0');

        result += ((uint64_t) digit) * pow10_LUT[i];
        extra  += ((uint64_t) digit) * pow10_LUT_extra[i];
    }

    // We're going to round result to even, using extra as the lower bits.
    // First, add the bits from extra that have wrapped over, and then reproduce
    // the ROUND_TO_EVEN macro, and then add that back into the result.

    result += (extra >> 8);
    extra = extra & 0xff;

    uint8_t lowerbit = result & 1;
    uint8_t top_extra = !!(extra & (1<<7));
    uint8_t bot_extra = !!(extra & ((1<<7)-1));

    result += (top_extra & bot_extra) | (lowerbit & top_extra);

    return result;
}


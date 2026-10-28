#ifndef FOX_FIXED_H
#define FOX_FIXED_H

#include <stdint.h>
#include "fox_lut.h"

typedef int32_t u8;
typedef int32_t u16;

static inline int32_t fixed_div_lut(int32_t a, int32_t b, int frac_bits) {
    if (b == 0) return 0;
    uint64_t magnitude = (b < 0) ? (uint64_t)(-(int64_t)b) : (uint64_t)b;
    unsigned int shift = 0;

    while (magnitude >= LUT_SIZE) {
        magnitude >>= 1;
        shift++;
    }
    
    uint32_t reciprocal = div_lut[magnitude];
    unsigned int divisor_shift = 16 - frac_bits + shift;

    int64_t numerator = (int64_t)a * reciprocal;

    if (divisor_shift >= 63) return 0;

    int64_t result = numerator / (INT64_C(1) << divisor_shift);
    if (b < 0) result = -result;

    return (int32_t)result;
}

static inline u16 float_to_fixed16(float value) { return (u16)(value * 65536.0f); }
static inline float fixed16_to_float(u16 value) { return (float)value / 65536.0f; }

static inline u16 fixed16_div_lut(u16 a, u16 b) { return (u16)fixed_div_lut(a, b, 16); }
static inline u8 fixed8_div_lut(u8 a, u8 b) { return (u8)fixed_div_lut(a, b, 8); }

static inline u16 fixed16_add(u16 a, u16 b) { return (u16)((int64_t)a + b); }
static inline u16 fixed16_sub(u16 a, u16 b) { return (u16)((int64_t)a - b); }
static inline u16 fixed16_mul(u16 a, u16 b) { return (u16)(((int64_t)a * b) / 65536); }
static inline u16 fixed16_div(u16 a, u16 b) { return (u16)fixed_div_lut(a, b, 16); }

static inline u8 float_to_fixed8(float value) { return (u8)(value * 256.0f); }
static inline float fixed8_to_float(u8 value) { return (float)value / 256.0f; }

static inline u8 fixed8_add(u8 a, u8 b) { return (u8)((int64_t)a + b); }
static inline u8 fixed8_sub(u8 a, u8 b) { return (u8)((int64_t)a - b); }
static inline u8 fixed8_mul(u8 a, u8 b) { return (u8)(((int64_t)a * b) / 256); }
static inline u8 fixed8_div(u8 a, u8 b) { return (u8)fixed_div_lut(a, b, 8); }


#endif
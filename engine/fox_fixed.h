#ifndef FOX_FIXED_H
#define FOX_FIXED_H

#include "fox_structs.h"

#define FIXED_ONE24x8 256
#define FIXED8_SHIFT   8
static inline float from_fixed24(qFixed24x8_t a) { return (float)a / FIXED_ONE24x8; }
static inline qFixed24x8_t to_fixed24(float a) { return (qFixed24x8_t)(a * FIXED_ONE24x8); }
static inline qFixed24x8_t mul_24(qFixed24x8_t a, qFixed24x8_t b) { return (qFixed24x8_t)(((int64_t)a * b) >> FIXED8_SHIFT); }
static inline qFixed24x8_t div_24(qFixed24x8_t a, qFixed24x8_t b) { if (b == 0) { return 0; } return (qFixed24x8_t)(((int64_t)a << FIXED8_SHIFT) / b); }

#define FIXED_ONE16 65536
#define FIXED16_SHIFT 16
static inline float from_fixed16(qFixed16_t a) { return (float)a / FIXED_ONE16; }
static inline qFixed16_t to_fixed16(float a) { return (qFixed16_t)(a * FIXED_ONE16); }
static inline qFixed16_t mul_16(qFixed16_t a, qFixed16_t b) { return (qFixed16_t)(((int64_t)a * b) >> FIXED16_SHIFT); }
static inline qFixed16_t div_16(qFixed16_t a, qFixed16_t b) { if (b == 0) { return 0; } return (qFixed16_t)(((int64_t)a << FIXED16_SHIFT) / b); }

static inline qFixed16_t fixed24_to_16(qFixed24x8_t a){ return a << 8; }
static inline qFixed16_t fixed16_to_24(qFixed16_t a){ return a >> 8; }

#endif
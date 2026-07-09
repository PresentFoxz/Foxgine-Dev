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

static inline qFixed24x8_t mul_24_16(qFixed24x8_t a, qFixed16_t b) { return (qFixed24x8_t)(((int64_t)a * b) >> 16); }
static inline qFixed16_t mul_16_24(qFixed16_t a, qFixed24x8_t b) { return (qFixed16_t)(((int64_t)a * b) >> 8); }

#define ANGLE_COUNT 1024
extern qFixed16_t sinTable[ANGLE_COUNT];
static inline void initTable() {
    for (int i = 0; i < ANGLE_COUNT; i++) {
        float a = (2.0f * M_PI * i) / ANGLE_COUNT;
        sinTable[i] = to_fixed16(sinf(a));
    }
}

static inline int rad24_to_index(qFixed24x8_t rad) {
    const qFixed24x8_t TWO_PI = to_fixed24(6.28318530718f);
    int64_t t = (int64_t)rad * ANGLE_COUNT;
    return (int)(t / TWO_PI) & (ANGLE_COUNT-1);
}

static inline qFixed16_t fsin(int angle) { return sinTable[angle & (ANGLE_COUNT - 1)]; }
static inline qFixed16_t fcos(int angle) { return sinTable[(angle + ANGLE_COUNT/4) & (ANGLE_COUNT - 1)]; }

static inline qFixed24x8_t fsin24(int angle) { return (qFixed24x8_t)(sinTable[angle & (ANGLE_COUNT - 1)] >> 8); }
static inline qFixed24x8_t fcos24(int angle) { return (qFixed24x8_t)(sinTable[(angle + ANGLE_COUNT/4) & (ANGLE_COUNT - 1)] >> 8); }

static inline qFixed16_t project_div(qFixed16_t a, qFixed16_t b, qFixed16_t c) {
    if(c == 0) return 0;
    return (qFixed16_t)(((int64_t)a * b) / c);
}

static inline qFixed16_t fixed_from_int(int x) { return x << FIXED16_SHIFT; }

#endif
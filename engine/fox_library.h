#ifndef FOX_LIBRARY_H
#define FOX_LIBRARY_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef PLATFORM_WIN

#include <SDL2/SDL.h>

typedef FILE FileType;
typedef uint32_t Pixel_t;
#define MAIN_SCREEN_W 1200
#define MAIN_SCREEN_H 800

#define SCREEN_W (MAIN_SCREEN_W / 2)
#define SCREEN_H (MAIN_SCREEN_H / 2)

static inline void* fox_realloc(void* ptr, size_t size) { return realloc(ptr, size); }
static inline void* fox_malloc(size_t size) { return realloc(NULL, size); }
static inline void fox_free(void* ptr) { realloc(ptr, 0); }

static int fox_fgets(char *out, int maxLen, FileType *file) {
    if (fgets(out, maxLen, file) == NULL) return 0;

    int len = strlen(out);
    if (len > 0 && out[len-1] == '\n') { out[len-1] = '\0'; }

    return len;
}

#elif defined(PLAYDATE_SDK)

#include "pd_api.h"
extern PlaydateAPI* pd;

typedef SDFile FileType;
typedef uint8_t Pixel_t;
#define MAIN_SCREEN_W 400
#define MAIN_SCREEN_H 240

#define SCREEN_W (MAIN_SCREEN_W / 2)
#define SCREEN_H (MAIN_SCREEN_H / 2)

static inline void* fox_realloc(void* ptr, size_t size) { return pd->system->realloc(ptr, size); }
static inline void* fox_malloc(size_t size) { return pd->system->realloc(NULL, size); }
static inline void fox_free(void* ptr) { if (ptr) { pd->system->realloc(ptr, 0); } }

static int fox_fgets(char *out, int maxLen, FileType *file) {
    int i = 0;
    char c;

    while (i < maxLen - 1) {
        int r = pd->file->read(file, &c, 1);
        if (r <= 0) break;
        out[i++] = c;
        if (c == '\n') break;
    }

    out[i] = '\0';
    return i > 0;
}

#endif

extern Pixel_t *screenBuffer;
extern Pixel_t *mainBuffer;
extern int interlace;
extern int interlaceAmt;
extern bool canInterlace;

#define DEG2RAD(x) ((x) * 0.0174532925f)

static inline float fastsqrt(float x) {
    union { float f; uint32_t i; } conv = { x };
    conv.i = 0x5f3759df - (conv.i >> 1);
    float y = conv.f;
    return y * (1.5f - 0.5f * x * y * y);
}

static inline int floor_div(float v, float s) {
    float d = v / s;
    int i = (int)d;

    if (d < 0.0f && d != (float)i) { i--; }
    return i;
}

#endif
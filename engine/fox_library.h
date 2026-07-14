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

typedef uint32_t Pixel_t;
#define MAIN_SCREEN_W 1200
#define MAIN_SCREEN_H 800

#define SCREEN_W 400
#define SCREEN_H 240

#endif

extern Pixel_t *screenBuffer;
extern Pixel_t *mainBuffer;
extern int interlace;
extern int interlaceAmt;
extern bool canInterlace;

#define DEG2RAD(x) ((x) * 0.0174532925f)

#endif
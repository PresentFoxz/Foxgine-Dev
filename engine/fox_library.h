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
#define SCREEN_W 1280
#define SCREEN_H 800

#elif defined(PLATFORM_GBA)

#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_types.h>

typedef uint8_t Pixel_t;
#define MAIN_SCREEN_W 240
#define MAIN_SCREEN_H 160

#define SCREEN_W 120
#define SCREEN_H 80

extern Pixel_t *screenBuffer;

#define ARM __attribute__((target("arm")))
#define ARM_IWRAM __attribute__((target("arm"))) __attribute__((section(".iwram"), long_call))

#endif

extern Pixel_t *mainBuffer;
extern int interlace;

#define DEG2RAD(x) ((x) * 0.0174532925f)

#endif
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
#define SCREEN_W 240
#define SCREEN_H 160

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

#define REG_KEYINPUT (*(volatile unsigned short*)0x04000130)

#define KEY_A      0x0001
#define KEY_B      0x0002
#define KEY_SELECT 0x0004
#define KEY_START  0x0008
#define KEY_RIGHT  0x0010
#define KEY_LEFT   0x0020
#define KEY_UP     0x0040
#define KEY_DOWN   0x0080
#define KEY_R      0x0100
#define KEY_L      0x0200

extern Pixel_t *screenBuffer;


#endif

extern Pixel_t *mainBuffer;
extern int interlace;

#define DEG2RAD(x) ((x) * 0.0174532925f)

#endif
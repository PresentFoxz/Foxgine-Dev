#ifndef FOX_PALETTE_H
#define FOX_PALETTE_H

#include "fox_structs.h"

static const Color_t palettes[] = {
    // Black and White
    { 0,   0,   0,   255 },
    { 255, 255, 255, 255 },

    // Gray (4 shades)
    { 32,  32,  32, 255 },
    { 85,  85,  85, 255 },
    { 170, 170, 170, 255 },
    { 255, 255, 255, 255 },

    // Green (dark, normal, light)
    { 0,   80,  0,   255 },
    { 0,   180, 0,   255 },
    { 80,  255, 80,  255 },

    // Red
    { 80,  0,   0,   255 },
    { 200, 0,   0,   255 },
    { 255, 80,  80,  255 },

    // Blue
    { 0,   0,   80,  255 },
    { 0,   80,  220, 255 },
    { 80,  160, 255, 255 },

    // Yellow
    { 90,  80,  0,   255 },
    { 230, 220, 0,   255 },
    { 255, 255, 100, 255 },

    // Orange
    { 100, 40,  0,   255 },
    { 230, 100, 0,   255 },
    { 255, 170, 60,  255 },

    // Magenta
    { 80,  0,   80,  255 },
    { 200, 0,   200, 255 },
    { 255, 80,  255, 255 }
};

#define PALETTE_COUNT (sizeof(palettes) / sizeof(palettes[0]))

#endif
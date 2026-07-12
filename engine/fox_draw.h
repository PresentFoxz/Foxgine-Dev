#ifndef FOX_DRAW_H
#define FOX_DRAW_H

#include "fox_library.h"
#include "fox_structs.h"
#include "fox_mesh.h"

#ifdef PLATFORM_WIN

Pixel_t color_to_pixel(Color_t c);

void clear_buf(Pixel_t col);
void draw_tri_fixed(TriRend_t tri, Pixel_t col);

#elif defined(PLATFORM_GBA)

void add_palette(Color_t c);
int color_to_index(Color_t c);

ARM void clear_buf(Pixel_t col);
ARM_IWRAM void draw_tri_fixed(TriRend_t tri, Pixel_t col);

#endif

void draw_pixel(int x, int y, Pixel_t col);
void draw_rect(int x, int y, int w, int h, Pixel_t col);

#endif
#ifndef FOX_DRAW_H
#define FOX_DRAW_H

#include "fox_library.h"
#include "fox_structs.h"
#include "fox_mesh.h"

Pixel_t color_to_pixel(Color_t c);
void draw_pixel(int x, int y, Pixel_t col);
void clear_buf(Pixel_t col);
void draw_rect(int x, int y, int w, int h, Pixel_t col);
void draw_tri_fixed(TriRend_t tri, Pixel_t col);
void draw_tri_float(TriRend_t tri, Pixel_t col);

#endif
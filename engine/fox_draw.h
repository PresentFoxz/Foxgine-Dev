#ifndef FOX_DRAW_H
#define FOX_DRAW_H

#include "fox_library.h"
#include "fox_structs.h"
#include "fox_mesh.h"

Pixel_t color_to_pixel(uint8_t c);

void clear_buf(Pixel_t col);
void draw_tri(TriRend_t tri, Pixel_t col);

void draw_pixel(int x, int y, Pixel_t col);
void draw_rect(int x, int y, int w, int h, Pixel_t col);

#endif
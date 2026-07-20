#include "fox_draw.h"

uint8_t paletteIndex = 0;

#ifdef PLAYDATE_SDK

Pixel_t color_to_pixel(uint8_t c) { return c; }
static uint8_t pixel_to_brightness(Pixel_t pixel) { return pixel; }

#else

Pixel_t color_to_pixel(uint8_t c) { return (255u << 24) | (c << 16) | (c << 8) | c; }
static uint8_t pixel_to_brightness(Pixel_t pixel) { return pixel & 0xFF; }

#endif

static Pixel_t dither(Pixel_t value, int x, int y) {
    uint8_t pixel = pixel_to_brightness(value);
    
    static const Pixel_t bayer4x4[4][4] = {
        {  0,  8,  2, 10 },
        { 12,  4, 14,  6 },
        {  3, 11,  1,  9 },
        { 15,  7, 13,  5 }
    };

    Pixel_t threshold = bayer4x4[y & 3][x & 3] * 16;
    return (pixel > threshold) ? color_to_pixel(255) : color_to_pixel(0);
}

void draw_pixel(int x, int y, Pixel_t col) {
    if (((y / interlaceAmt) & 1) != interlace && canInterlace) return;
    if (y < 0 || y >= SCREEN_H) return;
    if (x < 0 || x >= SCREEN_W) return;

    mainBuffer[y * SCREEN_W + x] = dither(col, x, y);
}

void clear_buf(Pixel_t col) {
    for (int y=0; y < SCREEN_H; y++) {
        if (((y / interlaceAmt) & 1) != interlace && canInterlace) continue;

        Pixel_t *row = &mainBuffer[y * SCREEN_W];
        for (int x=0; x < SCREEN_W; x++) { row[x] = dither(col, x, y); }
    }
}

static inline void draw_strip(int x1, int x2, int y, Pixel_t col) {
    if(y < 0 || y >= SCREEN_H) return;

    if(x1 > x2) {
        int t = x1;
        x1 = x2;
        x2 = t;
    }

    if(x2 < 0 || x1 >= SCREEN_W) return;
    if(x1 < 0) x1 = 0;
    if(x2 >= SCREEN_W) x2 = SCREEN_W - 1;

    Pixel_t *dst = &mainBuffer[y * SCREEN_W + x1];
    int count = x2 - x1 + 1;
    for(int x = 0; x < count; x++) { dst[x] = dither(col, x1 + x, y); }
}

void draw_rect(int x, int y, int w, int h, Pixel_t col) {
    for (int yy = y; yy < y + h; yy++) {
        if ((yy & 1) != interlace && canInterlace) continue;
        
        draw_strip(x, x + w - 1, yy, col);
    }
}

void draw_tri(TriRend_t tri, Pixel_t col) {
    Vec2i v0 = tri.p0;
    Vec2i v1 = tri.p1;
    Vec2i v2 = tri.p2;

    Vec2i temp;
    if (v1.y < v0.y) { temp=v0; v0=v1; v1=temp; }
    if (v2.y < v0.y) { temp=v0; v0=v2; v2=temp; }
    if (v2.y < v1.y) { temp=v1; v1=v2; v2=temp; }

    int dy01 = v1.y - v0.y;
    int dy12 = v2.y - v1.y;
    int dy02 = v2.y - v0.y;

    if (dy02 == 0) return;

    float dx02 = (float)(v2.x - v0.x) / dy02;
    float dx01 = 0;
    float dx12 = 0;

    if (dy01) dx01 = (float)(v1.x - v0.x) / dy01;
    if (dy12) dx12 = (float)(v2.x - v1.x) / dy12;

    float xA = v0.x;
    float xB = v0.x;

    int y;
    for (y = v0.y; y < v1.y; y++) {
        if ((y & 1) != interlace && canInterlace) {
            xA += dx02;
            xB += dx01;

            continue;
        }

        if (y < 0 || y >= SCREEN_H) {
            xA += dx02;
            xB += dx01;
            continue;
        }

        int xLeft  = (int)(xA < xB ? xA : xB);
        int xRight = (int)(xA > xB ? xA : xB);

        if (xLeft < 0) xLeft = 0;
        if (xRight >= SCREEN_W) xRight = SCREEN_W - 1;

        if (xLeft <= xRight) draw_strip(xLeft, xRight, y, col);

        xA += dx02;
        xB += dx01;
    }

    xB = v1.x;
    for (; y <= v2.y; y++) {
        if ((y & 1) != interlace && canInterlace) {
            xA += dx02;
            xB += dx12;
            
            continue;
        }

        if (y < 0 || y >= SCREEN_H) {
            xA += dx02;
            xB += dx12;
            continue;
        }

        int xLeft  = (int)(xA < xB ? xA : xB);
        int xRight = (int)(xA > xB ? xA : xB);

        if (xLeft < 0) xLeft = 0;
        if (xRight >= SCREEN_W) xRight = SCREEN_W - 1;

        if (xLeft <= xRight) draw_strip(xLeft, xRight, y, col);

        xA += dx02;
        xB += dx12;
    }
}
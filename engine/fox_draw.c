#include "fox_draw.h"

uint8_t paletteIndex = 0;
Pixel_t color_to_pixel(uint8_t c) { return (255u << 24) | (c << 16) | (c << 8) | c; }

void draw_pixel(int x, int y, Pixel_t col) {
    if (((y / interlaceAmt) & 1) != interlace && canInterlace) return;
    if (y < 0 || y >= SCREEN_H) return;
    if (x < 0 || x >= SCREEN_W) return;

    mainBuffer[y * SCREEN_W + x] = col;
}

void clear_buf(Pixel_t col) {
    for (int y=0; y < SCREEN_H; y++) {
        if (((y / interlaceAmt) & 1) != interlace && canInterlace) continue;

        Pixel_t *row = &mainBuffer[y * SCREEN_W];
        for (int x=0; x < SCREEN_W; x++) { row[x] = col; }
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

    #ifdef PLATFORM_WIN
    Pixel_t *dst = &mainBuffer[y * SCREEN_W + x1];
    int count = x2 - x1 + 1;
    while(count--) *dst++ = col;
    #endif
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

    if (v1.y < v0.y) { temp = v0; v0 = v1; v1 = temp; }
    if (v2.y < v0.y) { temp = v0; v0 = v2; v2 = temp; }
    if (v2.y < v1.y) { temp = v1; v1 = v2; v2 = temp; }

    int dy02 = v2.y - v0.y;
    if (dy02 <= 0) return;

    int dy01 = v1.y - v0.y;
    int dy12 = v2.y - v1.y;

    int cross = (v1.x - v0.x) * dy02 - (v2.x - v0.x) * dy01;

    bool middleLeft = (cross > 0);

    float dxLong   = (float)(v2.x - v0.x) / dy02;
    float dxTop    = (dy01 != 0) ? (float)(v1.x - v0.x) / dy01 : 0.0f;
    float dxBottom = (dy12 != 0) ? (float)(v2.x - v1.x) / dy12 : 0.0f;

    float xLong  = (float)v0.x;
    float xShort = (float)v0.x;

    int y;
    for (y = v0.y; y < v1.y; y++) {
        if (((y / interlaceAmt) & 1) == interlace || !canInterlace) {
            int left, right;

            if (middleLeft) {
                left  = (int)xShort;
                right = (int)xLong;
            } else {
                left  = (int)xLong;
                right = (int)xShort;
            }

            draw_strip(left, right, y, col);
        }

        xLong  += dxLong;
        xShort += dxTop;
    }

    xShort = (float)v1.x;
    for (; y <= v2.y; y++) {
        if (((y / interlaceAmt) & 1) == interlace || !canInterlace) {
            int left, right;

            if (middleLeft) {
                left  = (int)xShort;
                right = (int)xLong;
            } else {
                left  = (int)xLong;
                right = (int)xShort;
            }

            draw_strip(left, right, y, col);
        }

        xLong  += dxLong;
        xShort += dxBottom;
    }
}
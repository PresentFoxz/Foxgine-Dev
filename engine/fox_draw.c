#include "fox_draw.h"
#include "fox_fixed.h"

Pixel_t color_to_pixel(Color_t c) {
    #ifdef PLATFORM_WIN
    return ((uint32_t)c.a << 24) | ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | ((uint32_t)c.b);
    #elif defined(PLATFORM_GBA)
    return RGB5(c.r >> 3, c.g >> 3, c.b >> 3);
    #endif
}

void draw_pixel(int x, int y, Pixel_t col) {
    if ((y & 1) == interlace) return;
    if (y < 0 || y >= SCREEN_H) return;
    if (x < 0 || x >= SCREEN_W) return;

    buffer[y * SCREEN_W + x] = col;
}

void clear_buf(Pixel_t col) {
    for (int y=0; y < SCREEN_H; y++) {
        if ((y & 1) == interlace) continue;

        Pixel_t *row = &buffer[y * SCREEN_W];
        for (int x=0; x < SCREEN_W; x++) { row[x] = col; }
    }
}

void draw_rect(int x, int y, int w, int h, Pixel_t col) {
    for (int yy = y; yy < y + h; yy++) {
        if ((yy & 1) == interlace) continue;

        Pixel_t *row = &buffer[yy * SCREEN_W + x];
        for (int xx = x; xx < x + w; xx++) { row[xx] = col; }
    }
}

static void draw_strip(int x0, int x1, int y, Pixel_t col) {
    if (y < 0 || y >= SCREEN_H) return;
    if (x1 < 0 || x0 >= SCREEN_W) return;

    if (x0 > x1) {
        int t = x0;
        x0 = x1;
        x1 = t;
    }

    if (x0 < 0) x0 = 0;
    if (x1 >= SCREEN_W) x1 = SCREEN_W - 1;

    Pixel_t *row = &buffer[y * SCREEN_W];
    for (int x = x0; x <= x1; x++) { row[x] = col; }
}

void draw_tri_fixed(TriRend_t tri, Pixel_t col) {
    Vec2i p0 = tri.p0;
    Vec2i p1 = tri.p1;
    Vec2i p2 = tri.p2;

    Vec2i temp;
    if (p0.y > p1.y) { temp = p0; p0 = p1; p1 = temp; }
    if (p0.y > p2.y) { temp = p0; p0 = p2; p2 = temp; }
    if (p1.y > p2.y) { temp = p1; p1 = p2; p2 = temp; }

    if (p2.y == p0.y) return;

    for (int y=p0.y; y <= p2.y; y++) {
        if ((y & 1) == interlace) continue;
        
        int dyLong = p2.y - p0.y;

        qFixed16_t alpha = div_16(y - p0.y, dyLong);
        qFixed16_t xLong = (p0.x << FIXED16_SHIFT) + mul_16((p2.x - p0.x) << FIXED16_SHIFT, alpha);
        
        qFixed16_t xShort;
        if (y < p1.y) {
            int dyShort = p1.y - p0.y;
            if (dyShort == 0) continue;

            qFixed16_t beta = div_16(y - p0.y, dyShort);
            xShort = (p0.x << FIXED16_SHIFT) + mul_16((p1.x - p0.x) << FIXED16_SHIFT, beta);
        } else {
            int dyShort = p2.y - p1.y;
            if (dyShort == 0) continue;

            qFixed16_t beta = div_16(y - p1.y, dyShort);
            xShort = (p1.x << FIXED16_SHIFT) + mul_16((p2.x - p1.x) << FIXED16_SHIFT, beta);
        }

        draw_strip((xShort + (FIXED_ONE16 >> 1)) >> FIXED16_SHIFT, (xLong  + (FIXED_ONE16 >> 1)) >> FIXED16_SHIFT, y, col);
    }
}

void draw_tri_float(TriRend_t tri, Pixel_t col) {
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

    if (dy01 != 0) dx01 = (float)(v1.x - v0.x) / dy01;
    if (dy12 != 0) dx12 = (float)(v2.x - v1.x) / dy12;

    float xA = v0.x;
    float xB = v0.x;

    int y;
    for (y = v0.y; y < v1.y; y++) {
        if ((y >= 0 && y < SCREEN_H) && ((y & 1) != interlace)) {
            int xLeft  = (int)(xA < xB ? xA + 0.5f : xB + 0.5f);
            int xRight = (int)(xA > xB ? xA + 0.5f : xB + 0.5f);

            draw_strip(xLeft, xRight, y, col);
        }
        xA += dx02;
        xB += dx01;
    }

    xB = v1.x;
    for (; y <= v2.y; y++) {
        if ((y >= 0 && y < SCREEN_H) && ((y & 1) != interlace)) {
            int xLeft  = (int)(xA < xB ? xA + 0.5f : xB + 0.5f);
            int xRight = (int)(xA > xB ? xA + 0.5f : xB + 0.5f);

            draw_strip(xLeft, xRight, y, col);
        }
        xA += dx02;
        xB += dx12;
    }
}
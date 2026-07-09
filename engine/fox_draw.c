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
    if ((y & 1) != interlace) return;
    if (y < 0 || y >= SCREEN_H) return;
    if (x < 0 || x >= SCREEN_W) return;

    buffer[y * SCREEN_W + x] = col;
}

void clear_buf(Pixel_t col) {
    for (int y=0; y < SCREEN_H; y++) {
        if ((y & 1) != interlace) continue;

        Pixel_t *row = &buffer[y * SCREEN_W];
        for (int x=0; x < SCREEN_W; x++) { row[x] = col; }
    }
}

void draw_rect(int x, int y, int w, int h, Pixel_t col) {
    for (int yy = y; yy < y + h; yy++) {
        if ((yy & 1) != interlace) continue;

        Pixel_t *row = &buffer[yy * SCREEN_W + x];
        for (int xx = x; xx < x + w; xx++) { row[xx] = col; }
    }
}

static inline void draw_strip(int x1, int x2, int y, Pixel_t col) {
    if(y < 0 || y >= SCREEN_H) return;
    if(x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    
    if(x2 < 0 || x1 >= SCREEN_W) return;
    if(x1 < 0) x1 = 0;
    if(x2 >= SCREEN_W) x2 = SCREEN_W - 1;


    Pixel_t* dst = &buffer[y * SCREEN_W + x1];
    int count = x2 - x1 + 1;
    while(count--) { *dst++ = col; }
}

void draw_tri_fixed(TriRend_t tri, Pixel_t col) {
    Vec2i v0 = tri.p0;
    Vec2i v1 = tri.p1;
    Vec2i v2 = tri.p2;

    Vec2i temp;
    if(v1.y < v0.y){ temp=v0; v0=v1; v1=temp; }
    if(v2.y < v0.y){ temp=v0; v0=v2; v2=temp; }
    if(v2.y < v1.y){ temp=v1; v1=v2; v2=temp; }


    int dy02 = v2.y - v0.y;
    if(dy02 <= 0) return;

    int dy01 = v1.y - v0.y;
    int dy12 = v2.y - v1.y;
    
    int cross = (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x);
    bool middleLeft = cross > 0;

    qFixed16_t dxLong = ((v2.x - v0.x) << FIXED16_SHIFT) / dy02;
    qFixed16_t dxTop = 0;
    qFixed16_t dxBottom = 0;

    if(dy01) dxTop = ((v1.x - v0.x) << FIXED16_SHIFT) / dy01;
    if(dy12) dxBottom = ((v2.x - v1.x) << FIXED16_SHIFT) / dy12;

    qFixed16_t xLong = fixed_from_int(v0.x);
    qFixed16_t xShort = fixed_from_int(v0.x);

    int y;
    for(y=v0.y; y<v1.y; y++) {
        if((y & 1) == interlace) {
            int left;
            int right;

            if(middleLeft) {
                left  = xShort >> FIXED16_SHIFT;
                right = xLong  >> FIXED16_SHIFT;
            } else {
                left  = xLong  >> FIXED16_SHIFT;
                right = xShort >> FIXED16_SHIFT;
            }
            
            draw_strip(left, right, y, col);
        }
        xLong  += dxLong;
        xShort += dxTop;
    }
    
    xShort = fixed_from_int(v1.x);
    for(; y<=v2.y; y++) {
        if((y & 1) == interlace) {
            int left;
            int right;

            if(middleLeft) {
                left  = xShort >> FIXED16_SHIFT;
                right = xLong  >> FIXED16_SHIFT;
            } else {
                left  = xLong  >> FIXED16_SHIFT;
                right = xShort >> FIXED16_SHIFT;
            }
            draw_strip(left, right, y, col);
        }
        xLong  += dxLong;
        xShort += dxBottom;
    }
}
#include "fox_draw.h"
#include "fox_fixed.h"
#include "fox_palette.h"

uint8_t paletteIndex = 0;
#ifdef PLATFORM_WIN

Pixel_t color_to_pixel(Color_t c) { return ((uint32_t)c.a << 24) | ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | ((uint32_t)c.b); }

#elif defined(PLATFORM_GBA)

int color_to_index(Color_t c) {
    for(int i = 0; i < PALETTE_COUNT; i++) {
        if(palettes[i].r == c.r && palettes[i].g == c.g && palettes[i].b == c.b) {
            return i;
        }
    }

    return -1;
}

void add_palette(Color_t c) {
    if(paletteIndex >= 256) return;
    BG_PALETTE[paletteIndex++] = RGB5(c.r >> 3, c.g >> 3, c.b >> 3);
}

#endif

void draw_pixel(int x, int y, Pixel_t col) {
    if ((y & 1) != interlace) return;
    if (y < 0 || y >= SCREEN_H) return;
    if (x < 0 || x >= SCREEN_W) return;

    mainBuffer[y * SCREEN_W + x] = col;
}

void clear_buf(Pixel_t col) {
    for (int y=0; y < SCREEN_H; y++) {
        if ((y & 1) != interlace) continue;

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

    #ifdef PLATFORM_GBA
    Pixel_t *dst = &mainBuffer[y * SCREEN_W + x1];
    int count = x2 - x1 + 1;

    if(((uintptr_t)dst & 1) && count) {
        *dst++ = col;
        count--;
    }

    u16 pair = ((u16)col << 8) | col;
    u16 *dst16 = (u16*)dst;

    while(count >= 2) {
        *dst16++ = pair;
        count -= 2;
    }

    dst = (Pixel_t*)dst16;

    if(count) {
        *dst = col;
    }
    #else
    Pixel_t *dst = &mainBuffer[y * SCREEN_W + x1];
    int count = x2 - x1 + 1;
    while(count--) *dst++ = col;
    #endif
}

void draw_rect(int x, int y, int w, int h, Pixel_t col) {
    for (int yy = y; yy < y + h; yy++) {
        if ((yy & 1) != interlace) continue;
        
        draw_strip(x, x + w - 1, yy, col);
    }
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
    
    int cross = (v1.x - v0.x) * dy02 - dy01 * (v2.x - v0.x);
    bool middleLeft = (cross > 0);

    qFixed16_t dxLong = div_16(((v2.x - v0.x) << FIXED16_SHIFT), to_fixed16(dy02));
    qFixed16_t dxTop = dy01 ? div_16(((v1.x - v0.x) << FIXED16_SHIFT), to_fixed16(dy01)) : 0;
    qFixed16_t dxBottom = dy12 ? div_16(((v2.x - v1.x) << FIXED16_SHIFT), to_fixed16(dy12)) : 0;

    int xLong  = to_fixed16(v0.x);
    int xShort = to_fixed16(v0.x);

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
    
    xShort = to_fixed16(v1.x);
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
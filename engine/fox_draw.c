#include "fox_draw.h"

uint8_t paletteIndex = 0;

#ifdef PLAYDATE_SDK

Pixel_t color_to_pixel(Color_t color) {
    if (color.a == 0) return 0;
    uint16_t brightness = ((uint16_t)color.r * 77 + (uint16_t)color.g * 150 + (uint16_t)color.b * 29) >> 8;
    return (Pixel_t)brightness;
}
static uint8_t pixel_to_brightness(Pixel_t pixel) { return pixel; }

#elif defined(PLATFORM_WIN)

Pixel_t color_to_pixel(Color_t color) {
    return ((uint32_t)color.a << 24) | ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | ((uint32_t)color.b);
}
static uint8_t pixel_to_brightness(Pixel_t pixel) {
    uint8_t r = (pixel >> 16) & 0xFF;
    uint8_t g = (pixel >> 8)  & 0xFF;
    uint8_t b =  pixel        & 0xFF;

    return (uint8_t)(((uint16_t)r * 77 + (uint16_t)g * 150 + (uint16_t)b * 29) >> 8);
}

#elif defined(POCKETBYTE_SDK)

Pixel_t color_to_pixel(Color_t color) {
    (void)color.a;

    uint16_t pixel = ((uint16_t)(color.r >> 3) << 11) | ((uint16_t)(color.g >> 2) << 5)  | ((uint16_t)(color.b >> 3));
    return (Pixel_t)((pixel >> 8) | (pixel << 8));
}

static uint8_t pixel_to_brightness(Pixel_t pixel) {
    uint16_t p = (uint16_t)((pixel >> 8) | (pixel << 8));

    uint8_t r = (p >> 11) & 0x1F;
    uint8_t g = (p >> 5)  & 0x3F;
    uint8_t b =  p        & 0x1F;

    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);

    return (uint8_t)(((uint16_t)r * 77 + (uint16_t)g * 150 + (uint16_t)b * 29) >> 8);
}

#elif defined(ESP_PLATFORM)

Pixel_t color_to_pixel(Color_t color) {
    uint32_t rgbValue = ((uint32_t)color.a << 24) | ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | ((uint32_t)color.b);

    return RGBtoPalette(rgbValue);
}

static uint8_t pixel_to_brightness(Pixel_t pixel) { return pixel; }

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
    return (pixel > threshold) ? color_to_pixel((Color_t){255, 255, 255, 255}) : color_to_pixel((Color_t){0, 0, 0, 255});
}

void draw_pixel(int x, int y, Pixel_t col) {
    if ((((int)(y * div_lut_check(interlaceAmt))) & 1) != interlace && canInterlace) return;
    if (y < 0 || y >= SCREEN_H) return;
    if (x < 0 || x >= SCREEN_W) return;

    mainBuffer[y * SCREEN_W + x] = dither(col, x, y);
}

void clear_buf(Pixel_t col, float zFix) {
    for (int y=0; y < SCREEN_H; y++) {
        if ((((int)(y * div_lut_check(interlaceAmt))) & 1) != interlace && canInterlace) continue;

        Pixel_t *row = &mainBuffer[y * SCREEN_W];
        float *zDst = &zBuffer[y * SCREEN_W];
        for (int x=0; x < SCREEN_W; x++) {
            #ifdef PLAYDATE_SDK
            row[x] = dither(col, x, y);
            #else
            row[x] = col;
            #endif

            zDst[x] = zFix;
        }
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
    for(int x = 0; x < count; x++) {
        #ifdef PLAYDATE_SDK
        dst[x] = dither(col, x, y);
        #else
        dst[x] = col;
        #endif
    }
}

static inline void draw_strip_z(int xLeft, int xRight, int y, Pixel_t col, Vec2i v0, float z0, float dzdx, float dzdy) {
    float z = z0 + (xLeft-v0.x)*dzdx + (y-v0.y)*dzdy;

    int index = y*SCREEN_W + xLeft;
    Pixel_t *dst = &mainBuffer[index];
    float *zDst = &zBuffer[index];

    for (int x = xLeft; x <= xRight; x++) {
        if (z < *zDst) {
            *zDst = z;

            #ifdef PLAYDATE_SDK
            *dst = dither(col, x, y);
            #else
            *dst = col;
            #endif
        }

        dst++;
        zDst++;
        z += dzdx;
    }
}

void draw_rect(int x, int y, int w, int h, Pixel_t col) {
    for (int yy = y; yy < y + h; yy++) {
        if ((yy & 1) != interlace && canInterlace) continue;
        
        draw_strip(x, x + w - 1, yy, col);
    }
}

static inline bool barycentricZ(Vec2i a, Vec2i b, Vec2i c, float z0, float z1, float z2, float *dzdx, float *dzdy) {
    int denom = (b.x-a.x)*(c.y-a.y) - (c.x-a.x)*(b.y-a.y);
    if (denom == 0) return false;

    float invDenom = div_lut_check(denom);

    *dzdx = ((z1-z0)*(c.y-a.y) - (z2-z0)*(b.y-a.y)) * invDenom;
    *dzdy = ((z2-z0)*(b.x-a.x) - (z1-z0)*(c.x-a.x)) * invDenom;

    return true;
}

void draw_tri_z(TriRend_t tri, float z1, float z2, float z3, Pixel_t col) {
    Vec2i v0 = tri.p0;
    Vec2i v1 = tri.p1;
    Vec2i v2 = tri.p2;

    Vec2i temp;
    float tz;

    if (v1.y < v0.y) {
        temp=v0; v0=v1; v1=temp;
        tz=z1; z1=z2; z2=tz;
    }
    if (v2.y < v0.y) {
        temp=v0; v0=v2; v2=temp;
        tz=z1; z1=z3; z3=tz;
    }
    if (v2.y < v1.y) {
        temp=v1; v1=v2; v2=temp;
        tz=z2; z2=z3; z3=tz;
    }

    int dy01 = v1.y - v0.y;
    int dy12 = v2.y - v1.y;
    int dy02 = v2.y - v0.y;

    if (dy02 == 0) return;

    float dzdx, dzdy;
    if (!barycentricZ(v0, v1, v2, z1, z2, z3, &dzdx, &dzdy)) return;

    float dx02 = (float)(v2.x-v0.x) * div_lut_check(dy02);
    float dx01 = 0;
    float dx12 = 0;

    if (dy01) dx01 = (float)(v1.x-v0.x) * div_lut_check(dy01);
    if (dy12) dx12 = (float)(v2.x-v1.x) * div_lut_check(dy12);

    float xA = v0.x;
    float xB = v0.x;

    int y;
    for (y=v0.y; y<v1.y; y++) {
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

        int xLeft = (int)(xA < xB ? xA : xB);
        int xRight = (int)(xA > xB ? xA : xB);

        if (xLeft < 0) xLeft = 0;
        if (xRight >= SCREEN_W) xRight = SCREEN_W-1;

        if (xLeft <= xRight) {
            draw_strip_z(xLeft, xRight, y, col, v0, z1, dzdx, dzdy);
        }

        xA += dx02;
        xB += dx01;
    }

    xB = v1.x;
    for (; y<=v2.y; y++) {
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

        int xLeft = (int)(xA < xB ? xA : xB);
        int xRight = (int)(xA > xB ? xA : xB);

        if (xLeft < 0) xLeft = 0;
        if (xRight >= SCREEN_W) xRight = SCREEN_W-1;

        if (xLeft <= xRight) {
            draw_strip_z(xLeft, xRight, y, col, v0, z1, dzdx, dzdy);
        }

        xA += dx02;
        xB += dx12;
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

    float dx02 = (float)(v2.x - v0.x) * div_lut_check(dy02);
    float dx01 = 0;
    float dx12 = 0;

    if (dy01) dx01 = (float)(v1.x - v0.x) * div_lut_check(dy01);
    if (dy12) dx12 = (float)(v2.x - v1.x) * div_lut_check(dy12);

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
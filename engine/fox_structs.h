#ifndef FOX_STRUCTS_H
#define FOX_STRUCTS_H

#include <stdint.h>

typedef int64_t qFixed32_t;
typedef int32_t qFixed16_t;
typedef int32_t qFixed24x8_t;

typedef struct {
    float x, y;
} Vec2f;

typedef struct {
    float x, y, z;
} Vec3f;

typedef struct {
    int x, y;
} Vec2i;

typedef struct {
    float x, y, z;
} Vec3i;

typedef struct {
    qFixed24x8_t x, y;
} Vec2s24;

typedef struct {
    qFixed24x8_t x, y, z;
} Vec3s24;

typedef struct {
    qFixed16_t x, y;
} Vec2fs16;

typedef struct {
    qFixed16_t x, y, z;
} Vec3s16;

typedef struct {
    qFixed16_t x[3][3];
} Mat3x3;

typedef struct {
    int width;
    int height;
    int8_t *pixels;
} Screen;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Color_t;

typedef struct {
    bool up;
    bool down;
    bool left;
    bool right;
    bool a;
    bool b;
    bool lb;
    bool rb;
} KeyInputs;

typedef struct {
    Vec3s24 pos;
    Vec3s24 rot;

    qFixed24x8_t fov, nearPlane, farPlane;
    qFixed24x8_t renderRadiusSq;
    qFixed16_t focal;
    Mat3x3 matrix;

    bool camLock;
} Camera_t;

#endif
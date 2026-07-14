#ifndef FOX_STRUCTS_H
#define FOX_STRUCTS_H

#include <stdint.h>

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
    int x, y, z;
} Vec3i;

typedef struct {
    float x[3][3];
} Mat3x3;

typedef struct {
    int width;
    int height;
    int8_t *pixels;
} Screen;

#ifdef PLATFORM_WIN
typedef struct {
    bool up;
    bool down;
    bool left;
    bool right;
    bool a;
    bool b;
    bool lb;
    bool rb;

    bool just_up;
    bool just_down;
    bool just_left;
    bool just_right;
    bool just_a;
    bool just_b;
    bool just_lb;
    bool just_rb;
} KeyInputs;
#endif

typedef struct {
    Vec3f pos;
    Vec3f rot;

    float fov, nearPlane, farPlane;
    float renderRadiusSq;
    float focal;
    Mat3x3 matrix;

    int prevCrank;
} Camera_t;

#endif
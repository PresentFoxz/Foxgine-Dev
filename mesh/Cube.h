#ifndef CUBE_H
#define CUBE_H

#include "fox_mesh.h"

#define CUBE_VERT 8
#define CUBE_TRI 12

static const Vec3f Cube_verts[CUBE_VERT ] = {
    {1.0f, 1.0f, -1.0f},
    {1.0f, -1.0f, -1.0f},
    {1.0f, 1.0f, 1.0f},
    {1.0f, -1.0f, 1.0f},
    {-1.0f, 1.0f, -1.0f},
    {-1.0f, -1.0f, -1.0f},
    {-1.0f, 1.0f, 1.0f},
    {-1.0f, -1.0f, 1.0f},
};

static const int Cube_tris[CUBE_TRI ][3] = {
    {4, 2, 0},
    {2, 7, 3},
    {6, 5, 7},
    {1, 7, 5},
    {0, 3, 1},
    {4, 1, 5},
    {4, 6, 2},
    {2, 6, 7},
    {6, 4, 5},
    {1, 3, 7},
    {0, 2, 3},
    {4, 0, 1},
};

static const Color_t Cube_colors[CUBE_TRI ] = {
    {200, 0,   0,   255},
    {200, 0,   0,   255},
    {200, 0,   0,   255},
    {200, 0,   0,   255},
    {200, 0,   0,   255},
    {200, 0,   0,   255},
    {200, 0,   0,   255},
    {200, 0,   0,   255},
    {200, 0,   0,   255},
    {200, 0,   0,   255},
    {200, 0,   0,   255},
    {200, 0,   0,   255},
};

#endif

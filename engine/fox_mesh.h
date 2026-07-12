#ifndef FOX_MESH_H
#define FOX_MESH_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "fox_library.h"
#include "fox_structs.h"
#include "fox_fixed.h"

#define TRI_LIMIT 1000
#define VERT_LIMIT (TRI_LIMIT * 2)

typedef struct {
    Vec3s24 p0, p1, p2;
    Pixel_t color;
} Triangle_t;

typedef struct {
    Vec2i p0, p1, p2;
} TriRend_t;

typedef struct {
    int t0, t1, t2;
} TriMesh;

typedef struct {
    int a, b, c;
    Pixel_t color;
    Vec3s24 normal;
} TriIndex;

typedef struct {
    Vec3s24 *verts;
    int vertCount;

    TriIndex *tris;
    int triCount;
} Mesh;

typedef struct {
    Triangle_t *tris;
    int triCount;
} RendMesh;

typedef enum {
    O_Object,
    O_Triangle
} ObjectType;

typedef struct {
    int idx;
    float dist;

    ObjectType obj;
} ObjectOrdering;

void load_mesh(Mesh *meshModel, const Vec3f *verts, int vertCount, const int (*tris)[3], int triCount, const Color_t *colors);

#endif
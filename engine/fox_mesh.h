#ifndef FOX_MESH_H
#define FOX_MESH_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "fox_structs.h"
#include "fox_fixed.h"

#define TRI_LIMIT 1000
#define VERT_LIMIT (TRI_LIMIT * 2)

typedef struct {
    Vec3s24 p0, p1, p2;
    Color_t color;
} Triangle_t;

typedef struct {
    Vec2i p0, p1, p2;
} TriRend_t;

typedef struct {
    int t0, t1, t2;
} TriMesh;

typedef struct {
    Vec3s24 *verts;
    int vertCount;

    int (*tris)[3];
    Color_t *colors;
    Vec3s24 *normal;
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


static Vec3f computeNormal(Vec3f tri[3]) {
    Vec3f edge1, edge2;
    edge1.x = tri[1].x - tri[0].x;
    edge1.y = tri[1].y - tri[0].y;
    edge1.z = tri[1].z - tri[0].z;

    edge2.x = tri[2].x - tri[0].x;
    edge2.y = tri[2].y - tri[0].y;
    edge2.z = tri[2].z - tri[0].z;
    
    Vec3f normal;
    normal.x = edge1.y * edge2.z - edge1.z * edge2.y;
    normal.y = edge1.z * edge2.x - edge1.x * edge2.z;
    normal.z = edge1.x * edge2.y - edge1.y * edge2.x;
    
    float len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
    if (len != 0.0f) {
        normal.x /= len;
        normal.y /= len;
        normal.z /= len;
    }

    return normal;
}

static void load_mesh(Mesh *meshModel, const Vec3f *verts, int vertCount, const int (*tris)[3], int triCount, const Color_t *colors) {
    meshModel->verts = malloc(sizeof(Vec3s24) * vertCount);
    meshModel->tris = malloc(sizeof(int[3]) * triCount);
    meshModel->colors = malloc(sizeof(Color_t) * triCount);
    meshModel->normal = malloc(sizeof(Vec3s24) * triCount);

    if (!meshModel->verts || !meshModel->tris || !meshModel->colors || !meshModel->normal) {
        printf("Mesh malloc failed\n");
        return;
    }

    meshModel->vertCount = vertCount;
    meshModel->triCount = triCount;

    for(int i = 0; i < vertCount; i++) { meshModel->verts[i] = (Vec3s24){to_fixed24(verts[i].x), to_fixed24(verts[i].y), to_fixed24(verts[i].z)}; }

    for(int i = 0; i < triCount; i++) {
        meshModel->tris[i][0] = tris[i][0];
        meshModel->tris[i][1] = tris[i][1];
        meshModel->tris[i][2] = tris[i][2];

        meshModel->colors[i] = colors[i];

        Vec3f face[3] = {
            {from_fixed24(meshModel->verts[tris[i][0]].x), from_fixed24(meshModel->verts[tris[i][0]].y), from_fixed24(meshModel->verts[tris[i][0]].z)},
            {from_fixed24(meshModel->verts[tris[i][1]].x), from_fixed24(meshModel->verts[tris[i][1]].y), from_fixed24(meshModel->verts[tris[i][1]].z)},
            {from_fixed24(meshModel->verts[tris[i][2]].x), from_fixed24(meshModel->verts[tris[i][2]].y), from_fixed24(meshModel->verts[tris[i][2]].z)}
        };

        Vec3f norm = computeNormal(face);
        meshModel->normal[i] = (Vec3s24){to_fixed24(norm.x), to_fixed24(norm.y), to_fixed24(norm.z)};
    }
}

#endif
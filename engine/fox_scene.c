#include "fox_scene.h"
#include "fox_draw.h"
#include "fox_3D.h"
#include "fox_fixed.h"

#include <stdio.h>
#include <math.h>

#define FULL_MESH_TRIS 1000
const qFixed24x8_t one_third = 85;
qFixed16_t sinTable[ANGLE_COUNT];
qFixed16_t cosTable[ANGLE_COUNT];
qFixed16_t focalTable[ANGLE_COUNT];

#ifdef PLATFORM_WIN
static RendMesh fullMesh;
static ObjectOrdering triDist[FULL_MESH_TRIS];;
#elif defined(PLATFORM_GBA)
EWRAM_BSS static RendMesh fullMesh;
EWRAM_BSS static ObjectOrdering triDist[FULL_MESH_TRIS];
#endif
static int triDistAmt = 0;

void alloc_mesh() {
    fullMesh.tris = malloc(sizeof(Triangle_t) * FULL_MESH_TRIS);

    if (fullMesh.tris == NULL) {
        printf("Failed to allocate mesh\n");
        fullMesh.triCount = 0;
        return;
    }

    fullMesh.triCount = 0;
    triDistAmt = 0;
}

static void quickSortIndices(int left, int right) {
    if (left >= right) return;

    float pivot = triDist[(left + right) >> 1].dist;
    int i = left;
    int j = right;

    while (i <= j) {
        while (triDist[i].dist > pivot) i++;
        while (triDist[j].dist < pivot) j--;

        if (i <= j) {
            ObjectOrdering tmp = triDist[i];
            triDist[i] = triDist[j];
            triDist[j] = tmp;
            i++;
            j--;
        }
    }

    if (left < j) quickSortIndices(left, j);
    if (i < right) quickSortIndices(i, right);
}

void renderTriangle(Triangle_t tri3D, Camera_t cam) {
    Triangle_t clipped[2] = {0};
    bool OOB = false;

    Vec2i triSpace[3];
    Pixel_t col = tri3D.color;
    Vec3s24 triangle[3] = {(Vec3s24){tri3D.p0.x, tri3D.p0.y, tri3D.p0.z}, (Vec3s24){tri3D.p1.x, tri3D.p1.y, tri3D.p1.z}, (Vec3s24){tri3D.p2.x, tri3D.p2.y, tri3D.p2.z}};

    int output = TriangleClipping(triangle, &clipped[0], &clipped[1], cam.nearPlane, cam.farPlane);
    if (!output) return;

    Vec3s24 tmp[3];
    for (int c = 0; c < output; c++) {
        tmp[0] = clipped[c].p0; tmp[1] = clipped[c].p1; tmp[2] = clipped[c].p2;
        for (int z = 0; z < 3; z++) {
            triSpace[z] = vert_to_screen(tmp[z], cam.focal, cam.nearPlane);
            if (triSpace[z].x == -999 && triSpace[z].y == -999) {
                OOB = true;
                break;
            }
        } if (OOB) continue;
        
        TriRend_t triRender = (TriRend_t){ .p0 = triSpace[0], .p1 = triSpace[1], .p2 = triSpace[2] };
        draw_tri_fixed(triRender, col);
    }
}

void draw_tris(Camera_t cam) {
    if (fullMesh.tris == NULL || triDist == NULL) return;

    printf("render tris: %d\n", fullMesh.triCount);

    quickSortIndices(0, fullMesh.triCount - 1);
    for (int t=0; t < triDistAmt; t++) {
        if (triDist[t].obj == O_Triangle) {
            renderTriangle(fullMesh.tris[triDist[t].idx], cam);
        } else if (triDist[t].obj == O_Object) {
            continue;
        }
    }

    fullMesh.triCount = 0;
    triDistAmt = 0;
}

void add_mesh_scene(Mesh model, Vec3s24 pos, Vec3s24 rot, Vec3s24 size, Camera_t cam) {
    if (fullMesh.tris == NULL) return;
    if (triDist == NULL) return;
    if (fullMesh.triCount >= FULL_MESH_TRIS) return;

    Mat3x3 modelMat;
    bool matRotated = false;
    if (rot.x != 0 && rot.y != 0 && rot.z != 0) {
        if (size.x == to_fixed24(1.0f) && size.y == to_fixed24(1.0f) && size.z == to_fixed24(1.0f)) { computeRotMatrix(&modelMat, rot.x, rot.y, rot.z); }
        else { computeRotScaleMatrix(&modelMat, rot.x, rot.y, rot.z, size.x, size.y, size.z); }

        matRotated = true;
    }

    bool triFacing = false;
    for (int t = 0; t < model.triCount; t++) {
        if (fullMesh.triCount >= FULL_MESH_TRIS) return;

        Vec3s24 triStore[3];
        qFixed24x8_t sumX = 0, sumY = 0, sumZ = 0;
        TriIndex tri = model.tris[t];
        Vec3s24 verts[3] = {model.verts[tri.a], model.verts[tri.b], model.verts[tri.c]};
        for (int v = 0; v < 3; v++) {
            if (matRotated) { rotateVertex(verts[v], &modelMat, &triStore[v]); } else { triStore[v] = verts[v]; }
            
            triStore[v].x += pos.x;
            triStore[v].y += pos.y;
            triStore[v].z += pos.z;

            sumX += triStore[v].x;
            sumY += triStore[v].y;
            sumZ += triStore[v].z;
            
            rotateVertexInPlace(&triStore[v], cam.pos, &cam.matrix);
        }

        Vec3s24 center = {mul_24(sumX, one_third), mul_24(sumY, one_third), mul_24(sumZ, one_third)};
        Vec3s24 fVect = {center.x - cam.pos.x, center.y - cam.pos.y, center.z - cam.pos.z};

        qFixed24x8_t dot = mul_24(tri.normal.x, fVect.x) + mul_24(tri.normal.y, fVect.y) + mul_24(tri.normal.z, fVect.z);
        if (!(dot < 0)/* && model.bfc[t]*/) continue;
        if (triStore[0].z < cam.nearPlane && triStore[1].z < cam.nearPlane && triStore[2].z < cam.nearPlane) continue;

        qFixed24x8_t dist = mul_24(fVect.x, fVect.x) + mul_24(fVect.y, fVect.y) + mul_24(fVect.z, fVect.z);
        if (cam.farPlane && dist > cam.renderRadiusSq) continue;

        triDist[fullMesh.triCount] = (ObjectOrdering){ .idx = fullMesh.triCount, .obj = O_Triangle, .dist = dist };

        fullMesh.tris[fullMesh.triCount] = (Triangle_t){ .p0 = triStore[0], .p1 = triStore[1], .p2 = triStore[2], .color = tri.color };

        fullMesh.triCount++;
        triDistAmt++;
    }
}

void computeCamData(Camera_t *cam) {
    computeCamMatrix(&cam->matrix, -cam->rot.x, -cam->rot.y, -cam->rot.z);
    cam->focal = focalTable[rad24_to_index(cam->fov)];
    cam->renderRadiusSq = cam->farPlane ? mul_24(cam->farPlane, cam->farPlane) : 0.0f;
}
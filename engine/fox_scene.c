#include "fox_scene.h"
#include "fox_draw.h"
#include "fox_3D.h"
#include "fox_fixed.h"

#include <stdio.h>
#include <math.h>

#define FULL_MESH_TRIS 1000
const qFixed24x8_t one_third = 85;
qFixed16_t sinTable[ANGLE_COUNT];

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
    Color_t col = tri3D.color;
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
        draw_tri_fixed(triRender, color_to_pixel(col));
    }
}

void draw_tris(Camera_t cam) {
    if (fullMesh.tris == NULL || triDist == NULL) return;

    printf("render tris: %d\n", fullMesh.triCount);

    if (fullMesh.triCount > 0) {
        quickSortIndices(0, fullMesh.triCount - 1);
        
        for (int t=0; t < triDistAmt; t++) {
            if (triDist[t].obj == O_Triangle) {
                renderTriangle(fullMesh.tris[triDist[t].idx], cam);
            } else if (triDist[t].obj == O_Object) {
                continue;
            }
        }
    }

    fullMesh.triCount = 0;
    triDistAmt = 0;
}

void add_mesh_scene(Mesh model, Vec3s24 pos, Vec3s24 rot, Vec3s24 size, Camera_t cam) {
    if (fullMesh.tris == NULL) return;
    if (triDist == NULL) return;
    if (fullMesh.triCount >= FULL_MESH_TRIS) return;

    qFixed24x8_t renderRadiusSq = cam.farPlane ? mul_24(cam.farPlane, cam.farPlane) : 0.0f;

    int triCount = model.triCount;
    int (*tris)[3] = model.tris;
    int vertCount = model.vertCount;
    Vec3s24 *verts = model.verts;
    Color_t *color = model.colors;
    Vec3s24 *normal = model.normal;

    Mat3x3 modelMat;
    computeRotScaleMatrix(&modelMat, rot.x, rot.y, rot.z, size.x, size.y, size.z);

    Triangle_t currentTri;
    bool triFacing = false;
    for (int t = 0; t < triCount; t++) {
        if (fullMesh.triCount >= FULL_MESH_TRIS) return;

        Vec3s24 world[3];
        Vec3s24 view[3];
        int *tri = model.tris[t];
        qFixed24x8_t sumX = 0;
        qFixed24x8_t sumY = 0;
        qFixed24x8_t sumZ = 0;

        for (int v = 0; v < 3; v++) {
            Vec3s24 vert = model.verts[tri[v]];
            rotateVertex(vert, &modelMat, &world[v]);
            
            world[v].x += pos.x;
            world[v].y += pos.y;
            world[v].z += pos.z;

            sumX += world[v].x;
            sumY += world[v].y;
            sumZ += world[v].z;
            
            rotateVertexInPlace(&world[v], cam.pos, &cam.matrix);
            view[v] = world[v];
        }

        qFixed24x8_t cx = mul_24(sumX, one_third);
        qFixed24x8_t cy = mul_24(sumY, one_third);
        qFixed24x8_t cz = mul_24(sumZ, one_third);

        Vec3s24 fVect = {cx - cam.pos.x, cy - cam.pos.y, cz - cam.pos.z};
        Vec3s24 n = normal[t];
        rotateVertex(n, &modelMat, &n);
        qFixed24x8_t len = sqrtf(mul_24(n.x, n.x) + mul_24(n.y, n.y) + mul_24(n.z, n.z));
        if (len > 0.0f) { n.x /= len; n.y /= len; n.z /= len; }

        qFixed24x8_t dot = mul_24(n.x, fVect.x) + mul_24(n.y, fVect.y) + mul_24(n.z, fVect.z);
        triFacing = (dot < 0) ? true : false;
        if (!triFacing) continue;
        if (world[0].z < cam.nearPlane && world[1].z < cam.nearPlane && world[2].z < cam.nearPlane) continue;

        currentTri.p0 = view[0];
        currentTri.p1 = view[1];
        currentTri.p2 = view[2];
        currentTri.color = color[t];

        qFixed24x8_t dx = cx - cam.pos.x;
        qFixed24x8_t dy = cy - cam.pos.y;
        qFixed24x8_t dz = cz - cam.pos.z;
        qFixed24x8_t dist = mul_24(dx, dx) + mul_24(dy, dy) + mul_24(dz, dz);
        if (cam.farPlane && dist > renderRadiusSq) continue;

        triDist[fullMesh.triCount].dist = dist;
        triDist[fullMesh.triCount].idx = fullMesh.triCount;
        triDist[fullMesh.triCount].obj = O_Triangle;

        fullMesh.tris[fullMesh.triCount] = currentTri;

        fullMesh.triCount++;
        triDistAmt++;
    }
}

void computeCamData(Camera_t *cam) {
    computeCamMatrix(&cam->matrix, -cam->rot.x, -cam->rot.y, -cam->rot.z);
    cam->focal = to_fixed16(1.0f / tanf(from_fixed24(cam->fov) * 0.5f));
}
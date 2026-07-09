#include "fox_scene.h"
#include "fox_draw.h"
#include "fox_3D.h"

#include <stdio.h>
#include <math.h>

#define FULL_MESH_TRIS 500
const float one_third = 0.3333333f;

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

    Vec2i triSpace[3];
    bool OOB = false;

    Vec3f triangle[3] = {(Vec3f){tri3D.p0.x, tri3D.p0.y, tri3D.p0.z}, (Vec3f){tri3D.p1.x, tri3D.p1.y, tri3D.p1.z}, (Vec3f){tri3D.p2.x, tri3D.p2.y, tri3D.p2.z}};
    int output = TriangleClipping(triangle, &clipped[0], &clipped[1], cam.nearPlane, cam.farPlane);
    if (!output) return;

    Color_t col = tri3D.color;

    Vec3f tmp[3];
    for (int c = 0; c < output; c++) {
        tmp[0] = clipped[c].p0; tmp[1] = clipped[c].p1; tmp[2] = clipped[c].p2;
        for (int z = 0; z < 3; z++) {
            triSpace[z] = vert_to_screen(tmp[z], cam.focal, cam.nearPlane);
            if (triSpace[z].x == -999 && triSpace[z].y == -999) {
                OOB = true;
                break;
            }
        } if (OOB) continue;

        bool edgeCase = false;
        for (int i = 0; i < 3; i++) {
            int x = triSpace[i].x;
            int y = triSpace[i].y;
            
            if (x < -32 || x > SCREEN_W + 32 || y < -32 || y > SCREEN_H + 32) {
                edgeCase = true;
                break;
            }
        } TriRend_t triRender = (TriRend_t){ .p0 = triSpace[0], .p1 = triSpace[1], .p2 = triSpace[2] };


        if (edgeCase) { draw_tri_float(triRender, color_to_pixel(col)); }
        else { draw_tri_fixed(triRender, color_to_pixel(col)); }
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

void add_mesh_scene(Mesh model, Vec3f pos, Vec3f rot, Vec3f size, Camera_t cam, float fov, float nearPlane, float farPlane) {
    if (fullMesh.tris == NULL) return;
    if (triDist == NULL) return;
    if (fullMesh.triCount >= FULL_MESH_TRIS) return;

    float renderRadiusSq = cam.farPlane ? (cam.farPlane * cam.farPlane) : 0.0f;

    int triCount = model.triCount;
    int (*tris)[3] = model.tris;
    int vertCount = model.vertCount;
    VertMesh *verts = model.verts;
    Color_t *color = model.colors;
    Vec3f *normal = model.normal;

    bool rotObjs = false;
    float modelMat[3][3];
    if (rot.x != 0.0f || rot.y != 0.0f || rot.z != 0.0f || size.x != 1.0f || size.y != 1.0f || size.z != 1.0f) {
        computeRotScaleMatrix(modelMat, rot.x, rot.y, rot.z, size.x, size.y, size.z);
        rotObjs = true;
    }

    Triangle_t currentTri;
    bool triFacing = false;
    for (int t = 0; t < triCount; t++) {
        if (fullMesh.triCount >= FULL_MESH_TRIS) return;

        Vec3f world[3];
        int *tri = model.tris[t];
        float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
        for (int v = 0; v < 3; v++) {
            Vec3f vert = model.verts[tri[v]].pos;
            
            if (rotObjs) { rotateVertex(vert, modelMat, &world[v]); } else { world[v] = vert; }
            
            world[v].x += pos.x;
            world[v].y += pos.y;
            world[v].z += pos.z;

            sumX += world[v].x;
            sumY += world[v].y;
            sumZ += world[v].z;

            rotateVertexInPlace(&world[v], cam.pos, cam.camMat);
        }

        int cx = sumX * one_third;
        int cy = sumY * one_third;
        int cz = sumZ * one_third;

        Vec3f fVect = {cx - cam.pos.x, cy - cam.pos.y, cz - cam.pos.z};
        Vec3f n = normal[t];
        if (rotObjs) {
            rotateVertex(n, modelMat, &n);
            float len = sqrtf(n.x*n.x + n.y*n.y + n.z*n.z);
            if (len > 0.0f) { n.x /= len; n.y /= len; n.z /= len; }
        }

        float dot = n.x*fVect.x + n.y*fVect.y + n.z*fVect.z;
        triFacing = (dot < 0.0f) ? true : false;
        if (!triFacing) continue;
        if (world[0].z < cam.nearPlane && world[1].z < cam.nearPlane && world[2].z < cam.nearPlane) continue;

        currentTri.p0 = world[0];
        currentTri.p1 = world[1];
        currentTri.p2 = world[2];
        currentTri.color = color[t];

        int dx = cx - cam.pos.x;
        int dy = cy - cam.pos.y;
        int dz = cz - cam.pos.z;
        int dist = (dx*dx + dy*dy + dz*dz);
        if (cam.farPlane && dist > renderRadiusSq) continue;

        triDist[fullMesh.triCount].dist = dist;
        triDist[fullMesh.triCount].idx = fullMesh.triCount;
        triDist[fullMesh.triCount].obj = O_Triangle;

        fullMesh.tris[fullMesh.triCount] = currentTri;

        fullMesh.triCount++;
        triDistAmt++;
    }
}

RendMesh add_mesh_rend(Mesh model, Vec3f pos, Vec3f rot, Vec3f size, Camera_t cam, float fov, float nearPlane, float farPlane) {
    RendMesh meshRend;
    meshRend.tris = malloc(sizeof(TriRend_t) * FULL_MESH_TRIS);
    meshRend.triCount = 0;

    float renderRadiusSq = cam.farPlane ? (cam.farPlane * cam.farPlane) : 0.0f;

    int triCount = model.triCount;
    int (*tris)[3] = model.tris;
    int vertCount = model.vertCount;
    VertMesh *verts = model.verts;
    Color_t *color = model.colors;
    Vec3f *normal = model.normal;

    bool rotObjs = false;
    float modelMat[3][3];
    if (rot.x != 0.0f || rot.y != 0.0f || rot.z != 0.0f || size.x != 1.0f || size.y != 1.0f || size.z != 1.0f) {
        computeRotScaleMatrix(modelMat, rot.x, rot.y, rot.z, size.x, size.y, size.z);
        rotObjs = true;
    }

    Triangle_t currentTri;
    bool triFacing = false;
    for (int t = 0; t < triCount; t++) {
        if (meshRend.triCount >= FULL_MESH_TRIS) return meshRend;

        Vec3f world[3];
        int *tri = model.tris[t];
        float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
        for (int v = 0; v < 3; v++) {
            Vec3f vert = model.verts[tri[v]].pos;
            
            if (rotObjs) { rotateVertex(vert, modelMat, &world[v]); } else { world[v] = vert; }
            
            world[v].x += pos.x;
            world[v].y += pos.y;
            world[v].z += pos.z;

            sumX += world[v].x;
            sumY += world[v].y;
            sumZ += world[v].z;

            rotateVertexInPlace(&world[v], cam.pos, cam.camMat);
        }

        int cx = sumX * one_third;
        int cy = sumY * one_third;
        int cz = sumZ * one_third;

        Vec3f fVect = {cx - cam.pos.x, cy - cam.pos.y, cz - cam.pos.z};
        Vec3f n = normal[t];
        if (rotObjs) {
            rotateVertex(n, modelMat, &n);
            float len = sqrtf(n.x*n.x + n.y*n.y + n.z*n.z);
            if (len > 0.0f) { n.x /= len; n.y /= len; n.z /= len; }
        }

        float dot = n.x*fVect.x + n.y*fVect.y + n.z*fVect.z;
        triFacing = (dot < 0.0f) ? true : false;
        if (!triFacing) continue;
        if (world[0].z < cam.nearPlane && world[1].z < cam.nearPlane && world[2].z < cam.nearPlane) continue;

        currentTri.p0 = world[0];
        currentTri.p1 = world[1];
        currentTri.p2 = world[2];
        currentTri.color = color[t];

        int dx = cx - cam.pos.x;
        int dy = cy - cam.pos.y;
        int dz = cz - cam.pos.z;
        int dist = (dx*dx + dy*dy + dz*dz);
        if (cam.farPlane && dist > renderRadiusSq) continue;

        triDist[fullMesh.triCount].dist = dist;
        triDist[fullMesh.triCount].idx = fullMesh.triCount;
        triDist[fullMesh.triCount].obj = O_Triangle;

        fullMesh.tris[fullMesh.triCount] = currentTri;

        fullMesh.triCount++;
        triDistAmt++;
    }

    return meshRend;
}

void computeCamData(Camera_t *cam) {
    computeCamMatrix(cam->camMat, cam->rot.x, cam->rot.y, cam->rot.z);
    cam->focal = 1.0f / tanf(cam->fov * 0.5f);
}
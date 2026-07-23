#include "fox_scene.h"
#include "fox_draw.h"
#include "fox_3D.h"

#include <stdio.h>
#include <math.h>

static int maxTrianglesInScene = 0;
const float one_third = 0.33333333f;

static RendMesh fullMesh;
static ObjectOrdering *triDist;
static int triDistAmt = 0;

void reset_triCount() { maxTrianglesInScene = 0; }
void add_triCount(int size) { maxTrianglesInScene += size; }

void alloc_mesh() {
    if (fullMesh.tris) {
        fox_free(fullMesh.tris);
        fullMesh.tris = NULL;
    }
    if (triDist) {
        fox_free(triDist);
        triDist = NULL;
    }

    fullMesh.tris = fox_malloc(sizeof(Triangle_t) * maxTrianglesInScene);
    triDist = fox_malloc(sizeof(ObjectOrdering) * maxTrianglesInScene);

    if (fullMesh.tris == NULL || triDist == NULL) {
        printf("Failed to allocate\n");
        fullMesh.triCount = 0;
        triDist = 0;
        return;
    }

    fullMesh.triCount = 0;
    triDistAmt = 0;

    #ifdef PLAYDATE_SDK
    pd->system->logToConsole("Allocation complete!");
    #else
    printf("Allocation complete!");
    #endif
}

static void quickSortIndices(ObjectOrdering *triSort, int left, int right) {
    if (left >= right) return;

    float pivot = triSort[(left + right) >> 1].dist;
    int i = left;
    int j = right;

    while (i <= j) {
        while (triSort[i].dist > pivot) i++;
        while (triSort[j].dist < pivot) j--;

        if (i <= j) {
            ObjectOrdering tmp = triSort[i];
            triSort[i] = triSort[j];
            triSort[j] = tmp;
            i++;
            j--;
        }
    }

    if (left < j) quickSortIndices(triSort, left, j);
    if (i < right) quickSortIndices(triSort, i, right);
}

void renderTriangle(Triangle_t tri3D, Camera_t cam) {
    Triangle_t clipped[2] = {0};
    bool OOB = false;

    Vec2i triSpace[3];
    Pixel_t col = tri3D.color;
    Vec3f triangle[3] = {(Vec3f){tri3D.p0.x, tri3D.p0.y, tri3D.p0.z}, (Vec3f){tri3D.p1.x, tri3D.p1.y, tri3D.p1.z}, (Vec3f){tri3D.p2.x, tri3D.p2.y, tri3D.p2.z}};

    int output = TriangleClipping(triangle, &clipped[0], &clipped[1], cam.nearPlane, cam.farPlane);
    if (!output) return;

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
        
        TriRend_t triRender = (TriRend_t){ .p0 = triSpace[0], .p1 = triSpace[1], .p2 = triSpace[2] };
        draw_tri(triRender, col);
    }
}

void draw_tris(Camera_t cam) {
    if (fullMesh.tris == NULL || triDist == NULL) return;

    if (triDistAmt > 1) quickSortIndices(triDist, 0, triDistAmt - 1);
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

void computeMatrixModel(Mesh *model, Vec3f rot, Vec3f size) {
    model->rotated = false;
    if (rot.x != 0 && rot.y != 0 && rot.z != 0) {
        if (size.x == 1.0f && size.y == 1.0f && size.z == 1.0f) { computeRotMatrix(&model->matrix, rot.x, rot.y, rot.z); }
        else { computeRotScaleMatrix(&model->matrix, rot.x, rot.y, rot.z, size.x, size.y, size.z); }

        model->rotated = true;
    }
}

void add_mesh_scene(Mesh model, Vec3f pos, Camera_t cam, bool vertUse) {
    if (fullMesh.tris == NULL) return;
    if (triDist == NULL) return;
    if (fullMesh.triCount >= maxTrianglesInScene) return;

    bool triFacing = false;
    for (int t = 0; t < model.triCount; t++) {
        if (fullMesh.triCount >= maxTrianglesInScene) return;

        Vec3f triStore[3];
        float sumX = 0, sumY = 0, sumZ = 0;
        TriIndex tri = model.tris[t];
        Vec3f verts[3] = {model.verts[tri.a], model.verts[tri.b], model.verts[tri.c]};
        for (int v = 0; v < 3; v++) {
            if (model.rotated) { rotateVertex(verts[v], &model.matrix, &triStore[v]); }
            else { triStore[v] = verts[v]; }
            
            triStore[v].x += pos.x;
            triStore[v].y += pos.y;
            triStore[v].z += pos.z;

            sumX += triStore[v].x;
            sumY += triStore[v].y;
            sumZ += triStore[v].z;
            
            rotateVertexInPlace(&triStore[v], cam.pos, &cam.matrix);
        }

        Vec3f center = {sumX * one_third, sumY * one_third, sumZ * one_third};
        Vec3f fVect = {center.x - cam.pos.x, center.y - cam.pos.y, center.z - cam.pos.z};

        Vec3f normal = tri.normal;
        if (model.rotated) {
            Vec3f rotatedNormal;
            rotateVertex(normal, &model.matrix, &rotatedNormal);
            normal = rotatedNormal;
        }

        float dot = (normal.x * fVect.x) + (normal.y * fVect.y) + (normal.z * fVect.z);
        if (!(dot < 0) && tri.bfc) continue;
        if (triStore[0].z < cam.nearPlane && triStore[1].z < cam.nearPlane && triStore[2].z < cam.nearPlane) continue;

        float dist;
        if (vertUse) {
            float z0 = triStore[0].z;
            float z1 = triStore[1].z;
            float z2 = triStore[2].z;
            if (tri.size) { dist = fmaxf(z0, fmaxf(z1, z2)); }
            else { dist = fminf(z0, fminf(z1, z2)); }
        } else {
            dist = (fVect.x * fVect.x) + (fVect.y * fVect.y) + (fVect.z * fVect.z);
        }
        if (cam.farPlane && dist > cam.renderRadiusSq) continue;

        triDist[fullMesh.triCount] = (ObjectOrdering){ .idx = fullMesh.triCount, .obj = O_Triangle, .dist = dist };

        fullMesh.tris[fullMesh.triCount] = (Triangle_t){ .p0 = triStore[0], .p1 = triStore[1], .p2 = triStore[2], .color = tri.color };

        fullMesh.triCount++;
        triDistAmt++;
    }
}

void computeCamData(Camera_t *cam) {
    computeCamMatrix(&cam->matrix, -cam->rot.x, -cam->rot.y, -cam->rot.z);
    cam->focal = 1.0f / tanf(cam->fov * 0.5f);
    cam->renderRadiusSq = cam->farPlane ? (cam->farPlane * cam->farPlane) : 0.0f;
}
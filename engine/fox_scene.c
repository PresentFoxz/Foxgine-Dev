#include "fox_scene.h"
#include "fox_draw.h"
#include "fox_3D.h"

#include <stdio.h>
#include <math.h>

static int maxTrianglesInScene = 0;
static int maxObjectsInScene = 0;
const float one_third = 0.33333333f;

static RendMesh fullMesh;
static ObjectOrdering *triDist;
static int triDistAmt = 0;

void reset_triCount() { maxTrianglesInScene = 0; }
void reset_objCount() { maxTrianglesInScene = 0; }
void add_triCount(int size) { maxTrianglesInScene += size; }
void add_objCount(int size) { maxObjectsInScene += size; }

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
    triDist = fox_malloc(sizeof(ObjectOrdering) * (maxTrianglesInScene + maxObjectsInScene));

    if (fullMesh.tris == NULL || triDist == NULL) {
        printf("Failed to allocate\n");
        fullMesh.triCount = 0;
        triDist = 0;
        return;
    }

    fullMesh.triCount = 0;
    triDistAmt = 0;
}

#define EPSILON 1
bool check_renderable(Mesh *mesh, Camera_t cam, Vec3f addBoundPos) {
    if (!mesh) return false;
    if (mesh->vertCount <= 0 || mesh->triCount <= 0) return false;

    MeshBounds bounds = {
        .min = {
            mesh->bounds.min.x + addBoundPos.x,
            mesh->bounds.min.y + addBoundPos.y,
            mesh->bounds.min.z + addBoundPos.z
        },

        .max = {
            mesh->bounds.max.x + addBoundPos.x,
            mesh->bounds.max.y + addBoundPos.y,
            mesh->bounds.max.z + addBoundPos.z
        }
    };

    Vec3f closest = cam.pos;
    if (closest.x < bounds.min.x) closest.x = bounds.min.x;
    if (closest.y < bounds.min.y) closest.y = bounds.min.y;
    if (closest.z < bounds.min.z) closest.z = bounds.min.z;

    if (closest.x > bounds.max.x) closest.x = bounds.max.x;
    if (closest.y > bounds.max.y) closest.y = bounds.max.y;
    if (closest.z > bounds.max.z) closest.z = bounds.max.z;

    if (closest.x == cam.pos.x && closest.y == cam.pos.y && closest.z == cam.pos.z) { return true; }

    Vec3f relative = { closest.x - cam.pos.x, closest.y - cam.pos.y, closest.z - cam.pos.z };
    float distSq = relative.x * relative.x + relative.y * relative.y + relative.z * relative.z;
    if (cam.farPlane && distSq > cam.renderRadiusSq) { return false; }

    rotateVertexInPlace(&closest, cam.pos, &cam.matrix);
    if (closest.z > cam.nearPlane && closest.z <= cam.farPlane) { return true; }

    for (int i = 0; i < 8; i++) {
        Vec3f v = mesh->aabb[i];

        v.x += addBoundPos.x;
        v.y += addBoundPos.y;
        v.z += addBoundPos.z;

        rotateVertexInPlace(&v, cam.pos, &cam.matrix);
        if (v.z > cam.nearPlane && v.z <= cam.farPlane) { return true; }
    }

    return false;
}

void draw_bounds(Camera_t cam, Mesh *mesh, Vec3f addBoundPos) {
    if (!mesh) return;

    for (int i = 0; i < 8; i++) {
        Vec3f v = mesh->aabb[i];

        v.x += addBoundPos.x;
        v.y += addBoundPos.y;
        v.z += addBoundPos.z;

        rotateVertexInPlace(&v, cam.pos, &cam.matrix);
        Vec2i p = vert_to_screen(v, cam.focal, cam.nearPlane);

        if (p.x < 0 || p.x >= SCREEN_W || p.y < 0 || p.y >= SCREEN_H) continue;
        
        draw_rect(p.x - 2, p.y - 2, 4, 4, color_to_pixel((Color_t){255, 0, 0, 255}));
    }
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

void computeMatrixModel(Mesh *model, Vec3f rot, Vec3f size) {
    model->rotated = false;
    if (rot.x != 0 && rot.y != 0 && rot.z != 0) {
        if (size.x == 1.0f && size.y == 1.0f && size.z == 1.0f) { computeRotMatrix(&model->matrix, rot.x, rot.y, rot.z); }
        else { computeRotScaleMatrix(&model->matrix, rot.x, rot.y, rot.z, size.x, size.y, size.z); }

        model->rotated = true;
    }
}

void add_obj_scene(Vec3f pos, float distMod, Camera_t cam, int idx) {
    Vec3f camSpace = pos;
    rotateVertexInPlace(&camSpace, cam.pos, &cam.matrix);

    float dist = camSpace.x*camSpace.x + camSpace.y*camSpace.y + camSpace.z*camSpace.z;
    float newDist = (dist - distMod);
    if (newDist < 0.001f) newDist = 0.002f;
    if (cam.farPlane && newDist > cam.renderRadiusSq) return;

    triDist[triDistAmt++] = (ObjectOrdering){ .idx = idx, .obj = O_Object, .dist = newDist };
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

        triDist[triDistAmt++] = (ObjectOrdering){ .idx = fullMesh.triCount, .obj = O_Triangle, .dist = dist };
        fullMesh.tris[fullMesh.triCount++] = (Triangle_t){ .p0 = triStore[0], .p1 = triStore[1], .p2 = triStore[2], .color = tri.color };
    }
}

void add_mesh_obj(Mesh model, Vec3f pos, Camera_t cam, bool vertUse) {
    RendMesh newMesh;
    ObjectOrdering *newDist;
    int newDistAmt = 0;

    newMesh.tris = fox_malloc(sizeof(Triangle_t) * model.triCount);
    newDist = fox_malloc(sizeof(Triangle_t) * model.triCount);
    newMesh.triCount = 0;

    if (newMesh.tris == NULL) return;
    if (newDist == NULL) return;

    bool triFacing = false;
    for (int t = 0; t < model.triCount; t++) {
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

        newDist[newDistAmt++] = (ObjectOrdering){ .idx = newMesh.triCount, .obj = O_Triangle, .dist = dist };
        newMesh.tris[newMesh.triCount++] = (Triangle_t){ .p0 = triStore[0], .p1 = triStore[1], .p2 = triStore[2], .color = tri.color };
    }

    if (newDistAmt > 1) quickSortIndices(newDist, 0, newDistAmt - 1);

    for (int t=0; t < newDistAmt; t++) { renderTriangle(newMesh.tris[newDist[t].idx], cam); }
}

void computeCamData(Camera_t *cam) {
    computeCamMatrix(&cam->matrix, -cam->rot.x, -cam->rot.y, -cam->rot.z);
    cam->focal = div_lut_check(tanf(cam->fov * 0.5f));
    cam->renderRadiusSq = cam->farPlane ? (cam->farPlane * cam->farPlane) : 0.0f;
}

void draw_tris(Camera_t cam, Objects_t *objects, MeshAnimations *allAnims) {
    if (fullMesh.tris == NULL || triDist == NULL) return;

    if (triDistAmt > 1) quickSortIndices(triDist, 0, triDistAmt - 1);
    for (int t=0; t < triDistAmt; t++) {
        if (triDist[t].obj == O_Triangle) {
            renderTriangle(fullMesh.tris[triDist[t].idx], cam);
        } else if (triDist[t].obj == O_Object) {
            int index = triDist[t].idx;
            Objects_t *obj = &objects[index];
            Mesh modelObj = allAnims[obj->modelID].ModelAnimations[obj->currentAnim][obj->currentFrame].ModelFrame;

            computeMatrixModel(&modelObj, obj->rot, obj->size);
            add_mesh_obj(modelObj, obj->pos, cam, true);
        }
    }

    fullMesh.triCount = 0;
    triDistAmt = 0;
}
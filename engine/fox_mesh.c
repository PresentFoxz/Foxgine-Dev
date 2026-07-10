#include "fox_mesh.h"
#include "fox_draw.h"

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

void load_mesh(Mesh *meshModel, const Vec3f *verts, int vertCount, const int (*tris)[3], int triCount, const Color_t *colors) {
    meshModel->verts = malloc(sizeof(Vec3s24) * vertCount);
    meshModel->tris = malloc(sizeof(int[3]) * triCount);
    meshModel->colors = malloc(sizeof(Pixel_t) * triCount);
    meshModel->normal = malloc(sizeof(Vec3s24) * triCount);

    if (!meshModel->verts || !meshModel->tris || !meshModel->colors || !meshModel->normal) {
        printf("Mesh malloc failed\n");
        return;
    }

    meshModel->vertCount = vertCount;
    meshModel->triCount = triCount;

    for(int i = 0; i < vertCount; i++) { meshModel->verts[i] = (Vec3s24){to_fixed24(verts[i].x), to_fixed24(verts[i].y), to_fixed24(verts[i].z)}; }

    for(int i = 0; i < triCount; i++) {
        meshModel->tris[i].a = tris[i][0];
        meshModel->tris[i].b = tris[i][1];
        meshModel->tris[i].c = tris[i][2];

        #ifdef PLATFORM_WIN
        meshModel->colors[i] = color_to_pixel(colors[i]);
        #elif defined(PLATFORM_GBA)
        int colorIndex = color_to_index(colors[i]);
        if (colorIndex != -1) meshModel->colors[i] = colorIndex;
        else meshModel->colors[i] = 1;

        printf("Color Index: %d | ", colorIndex);
        #endif

        Vec3f face[3] = {
            {from_fixed24(meshModel->verts[tris[i][0]].x), from_fixed24(meshModel->verts[tris[i][0]].y), from_fixed24(meshModel->verts[tris[i][0]].z)},
            {from_fixed24(meshModel->verts[tris[i][1]].x), from_fixed24(meshModel->verts[tris[i][1]].y), from_fixed24(meshModel->verts[tris[i][1]].z)},
            {from_fixed24(meshModel->verts[tris[i][2]].x), from_fixed24(meshModel->verts[tris[i][2]].y), from_fixed24(meshModel->verts[tris[i][2]].z)}
        };

        Vec3f norm = computeNormal(face);
        meshModel->normal[i] = (Vec3s24){to_fixed24(norm.x), to_fixed24(norm.y), to_fixed24(norm.z)};
    }
    printf("\n");
}
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

void load_mesh(Mesh *meshModel, char *filename) {
    FILE *file = fopen(filename, "r");
    if(!file) {
        printf("Failed to open %s\n", filename);
        return;
    }

    int verts = 0;
    int tris = 0;

    char line[128];
    while(fgets(line, sizeof(line), file)) {
        if(line[0] == 'v') verts++;
        if(line[0] == 'f') tris++;
    }

    rewind(file);

    meshModel->vertCount = verts;
    meshModel->triCount = tris;

    meshModel->verts = malloc(sizeof(Vec3f) * verts);
    meshModel->tris = malloc(sizeof(TriIndex) * tris);

    int vi = 0;
    int ti = 0;
    while(fgets(line, sizeof(line), file)) {
        if(line[0] == 'v') {
            float x,y,z;
            sscanf(line, "v %f %f %f", &x, &y, &z);
            meshModel->verts[vi++] = (Vec3f){x, y, z};
        }


        else if(line[0] == 'f') {
            int a,b,c;
            int color;

            sscanf(line, "f %d %d %d %d", &a, &b, &c, &color);

            meshModel->tris[ti].a = a;
            meshModel->tris[ti].b = b;
            meshModel->tris[ti].c = c;

            meshModel->tris[ti].color = color_to_pixel(color);


            Vec3f face[3] = { meshModel->verts[a], meshModel->verts[b], meshModel->verts[c] };
            meshModel->tris[ti].normal = computeNormal(face);
            
            float e1x = face[0].x - face[1].x;
            float e1y = face[0].y - face[1].y;
            float e1z = face[0].z - face[1].z;

            float e2x = face[1].x - face[2].x;
            float e2y = face[1].y - face[2].y;
            float e2z = face[1].z - face[2].z;

            float e3x = face[2].x - face[0].x;
            float e3y = face[2].y - face[0].y;
            float e3z = face[2].z - face[0].z;

            float edge1 = e1x*e1x + e1y*e1y + e1z*e1z;
            float edge2 = e2x*e2x + e2y*e2y + e2z*e2z;
            float edge3 = e3x*e3x + e3y*e3y + e3z*e3z;

            float biggestEdge = fmaxf(edge1, fmaxf(edge2, edge3));
            meshModel->tris[ti].size = (biggestEdge > 4.0f);
            ti++;
        }
    } fclose(file);
}
#ifndef FOX_SCENE_H
#define FOX_SCENE_H

#include "fox_structs.h"
#include "fox_mesh.h"

void alloc_mesh();
void draw_tris(Camera_t cam);
void add_mesh_scene(Mesh model, Vec3s24 pos, Vec3s24 rot, Vec3s24 size, Camera_t cam);
void computeCamData(Camera_t *cam);

#endif
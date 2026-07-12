#ifndef FOX_SCENE_H
#define FOX_SCENE_H

#include "fox_structs.h"
#include "fox_mesh.h"

#ifdef PLATFORM_GBA

ARM void add_mesh_scene(Mesh model, Vec3s24 pos, Vec3s24 rot, Vec3s24 size, Camera_t cam);

#else

void add_mesh_scene(Mesh model, Vec3s24 pos, Vec3s24 rot, Vec3s24 size, Camera_t cam);

#endif

void draw_tris(Camera_t cam);
void alloc_mesh();
void computeCamData(Camera_t *cam);

#endif
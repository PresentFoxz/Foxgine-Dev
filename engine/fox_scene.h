#ifndef FOX_SCENE_H
#define FOX_SCENE_H

#include "fox_structs.h"
#include "fox_mesh.h"

#ifdef PLATFORM_WIN

void add_mesh_scene(Mesh model, Vec3f pos, Vec3f rot, Vec3f size, Camera_t cam, bool vertUse);

#endif

void draw_tris(Camera_t cam);
void alloc_mesh();
void computeCamData(Camera_t *cam);

#endif
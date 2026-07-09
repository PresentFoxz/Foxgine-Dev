#ifndef FOX_SCENE_H
#define FOX_SCENE_H

#include "fox_structs.h"
#include "fox_mesh.h"

void alloc_mesh();
void draw_tris(Camera_t cam);
void add_mesh_scene(Mesh model, Vec3f pos, Vec3f rot, Vec3f size, Camera_t cam, float fov, float nearPlane, float farPlane);
void computeCamData(Camera_t *cam);

#endif
#ifndef FOX_SCENE_H
#define FOX_SCENE_H

#include "fox_structs.h"
#include "fox_mesh.h"

void alloc_mesh();
void add_triCount(int size);
void reset_triCount();
void add_mesh_scene(Mesh model, Vec3f pos, Camera_t cam, bool vertUse);
void computeMatrixModel(Mesh *model, Vec3f rot, Vec3f size);
void draw_tris(Camera_t cam);
void computeCamData(Camera_t *cam);

#endif
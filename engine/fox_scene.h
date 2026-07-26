#ifndef FOX_SCENE_H
#define FOX_SCENE_H

#include "fox_structs.h"
#include "fox_mesh.h"

void alloc_mesh();
void add_triCount(int size);
void add_objCount(int size);
void reset_triCount();
void reset_objCount();

void add_mesh_scene(Mesh model, Vec3f pos, Camera_t cam, bool vertUse);
void add_obj_scene(Vec3f pos, float distMod, Camera_t cam, int idx);
void add_mesh_obj(Mesh model, Vec3f pos, Camera_t cam, bool vertUse);

void computeMatrixModel(Mesh *model, Vec3f rot, Vec3f size);
void draw_tris(Camera_t cam, Objects_t *objects, MeshAnimations *allAnims);
void computeCamData(Camera_t *cam);

#endif
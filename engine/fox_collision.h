#ifndef FOX_COLLISION_H
#define FOX_COLLISION_H

#include <limits.h>
#include "fox_library.h"
#include "fox_structs.h"
#include "fox_mesh.h"

#define MAX_COLLISIONS 1024

typedef enum {
    SURFACE_NONE,
    SURFACE_FLOOR,
    SURFACE_WALL,
    SURFACE_SLOPE,
    SURFACE_CEILING,
    SURFACE_WATER,
    OUT_OF_BOUNDS
} SurfaceType;

typedef struct {
    Vec3f v0, v1, v2;
    Vec3f normal;
    Vec3f center;
    SurfaceType type;

    int minX, minY, minZ;
    int maxX, maxY, maxZ;

    float v0x, v0y, v0z;
    float v1x, v1y, v1z;

    float dot00, dot01, dot11, invDenom;
} CollisionSurface;

typedef struct {
    CollisionSurface* collisions;
    Vec3f pos;
    int amt;
} CollisionChunks;

typedef struct {
    MinMax2* lineMinMax;
    MinMax3* BoxMinMax;
} OOBArea;

void resetTriggers();
void resetCollisionSurface();
void fixSurfaces(Mesh mapArray, Vec2f pos);
void collisionChunks();
void addTriggers(Vec3f pos, Vec3f size, int type, int id);
void addCollisionSurface(Vec3f v0, Vec3f v1, Vec3f v2, Vec3f normal, SurfaceType type);
VectMf cylinderInTriangle(Vec3f pos, float radius, float height);
Triggers cylinderInTrigger(Vec3f pos, float radius, float height);
void setupMapCollision(Mesh map, Vec2f pos);

#endif
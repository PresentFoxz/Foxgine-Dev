#include "fox_collision.h"

CollisionChunks* collisionChunkSurfaces;
Triggers* triggers;

#define CHUNK_SIZE 50
#define TRI_EPSILON 0
#define FIX_BORDER 40

#define MAX_SLOPE_ANGLE 45.0f
#define MIN_SLOPE_Y 0.05f
#define MAX_SLOPE_Y 0.70710678f

int triggerCount = 0;
int chunkAmt = 0;

int minX_Stored = INT_MAX; int minY_Stored = INT_MAX; int minZ_Stored = INT_MAX;
int maxX_Stored = INT_MIN; int maxY_Stored = INT_MIN; int maxZ_Stored = INT_MIN;

static inline int getChunkPos(float dot) {
    return (int)floorf(dot * div_lut_check(CHUNK_SIZE));
}

void resetCollisionSurface() {
    collisionChunkSurfaces = NULL;
    chunkAmt = 0;
}

void fixSurfaces(Mesh mapArray, Vec2f pos) {
    int minX = INT_MAX, minY = INT_MAX, minZ = INT_MAX;
    int maxX = INT_MIN, maxY = INT_MIN, maxZ = INT_MIN;

    for (int i = 0; i < mapArray.triCount; i++) {
        int* tris = mapArray.tris[i].a == -1 ? NULL : (int[3]){mapArray.tris[i].a, mapArray.tris[i].b, mapArray.tris[i].c};
        if (!tris) continue;

        Vec3f v0 = mapArray.verts[tris[0]];
        Vec3f v1 = mapArray.verts[tris[1]];
        Vec3f v2 = mapArray.verts[tris[2]];

        v0.x += pos.x;
        v1.x += pos.x;
        v2.x += pos.x;

        v0.z += pos.y;
        v1.z += pos.y;
        v2.z += pos.y;

        int minX_ = (int)floorf(fminf(v0.x, fminf(v1.x, v2.x)));
        int minY_ = (int)floorf(fminf(v0.y, fminf(v1.y, v2.y)));
        int minZ_ = (int)floorf(fminf(v0.z, fminf(v1.z, v2.z)));

        int maxX_ = (int)ceilf(fmaxf(v0.x, fmaxf(v1.x, v2.x)));
        int maxY_ = (int)ceilf(fmaxf(v0.y, fmaxf(v1.y, v2.y)));
        int maxZ_ = (int)ceilf(fmaxf(v0.z, fmaxf(v1.z, v2.z)));

        if (minX_ < minX) minX = minX_;
        if (minY_ < minY) minY = minY_;
        if (minZ_ < minZ) minZ = minZ_;

        if (maxX_ > maxX) maxX = maxX_;
        if (maxY_ > maxY) maxY = maxY_;
        if (maxZ_ > maxZ) maxZ = maxZ_;
    }

    if (minX < minX_Stored) minX_Stored = minX;
    if (minY < minY_Stored) minY_Stored = minY;
    if (minZ < minZ_Stored) minZ_Stored = minZ;

    if (maxX > maxX_Stored) maxX_Stored = maxX;
    if (maxY > maxY_Stored) maxY_Stored = maxY;
    if (maxZ > maxZ_Stored) maxZ_Stored = maxZ;
}

void collisionChunks() {
    int minChunkX = getChunkPos(minX_Stored);
    int minChunkY = getChunkPos(minY_Stored);
    int minChunkZ = getChunkPos(minZ_Stored);

    int maxChunkX = getChunkPos(maxX_Stored);
    int maxChunkY = getChunkPos(maxY_Stored);
    int maxChunkZ = getChunkPos(maxZ_Stored);

    for (int cx = minChunkX; cx <= maxChunkX; cx++) {
        for (int cy = minChunkY; cy <= maxChunkY; cy++) {
            for (int cz = minChunkZ; cz <= maxChunkZ; cz++) {
                bool exists = false;
                for (int i = 0; i < chunkAmt; i++) {
                    CollisionChunks* coll = &collisionChunkSurfaces[i];

                    if (coll->pos.x == cx && coll->pos.y == cy && coll->pos.z == cz) {
                        exists = true;
                        break;
                    }
                } if (exists) continue;

                collisionChunkSurfaces = fox_realloc(collisionChunkSurfaces, sizeof(CollisionChunks) * (chunkAmt + 1));

                collisionChunkSurfaces[chunkAmt].pos = (Vec3f){cx, cy, cz};
                collisionChunkSurfaces[chunkAmt].amt = 0;
                collisionChunkSurfaces[chunkAmt].collisions = NULL;

                chunkAmt++;
            }
        }
    }
}

void resetTriggers() {
    triggers = NULL;
    triggerCount = 0;
}

void addTriggers(Vec3f pos, Vec3f size, int type, int id) {
    triggers = fox_realloc(triggers, (triggerCount + 1) * sizeof(Triggers));

    Triggers* trig = &triggers[triggerCount++];
    trig->pos = pos;
    trig->size = size;
    trig->type = type;
    trig->id = id;
}

void addCollisionSurface(Vec3f v0, Vec3f v1, Vec3f v2, Vec3f normal, SurfaceType type) {
    CollisionSurface surf;

    surf.v0 = v0;
    surf.v1 = v1;
    surf.v2 = v2;

    surf.center.x = (v0.x + v1.x + v2.x) * div_lut_check(3);
    surf.center.y = (v0.y + v1.y + v2.y) * div_lut_check(3);
    surf.center.z = (v0.z + v1.z + v2.z) * div_lut_check(3);
    surf.normal = normal;

    float ux = v1.x - v0.x, uy = v1.y - v0.y, uz = v1.z - v0.z;
    float vx = v2.x - v0.x, vy = v2.y - v0.y, vz = v2.z - v0.z;

    if (type == SURFACE_NONE) {
        float ny = surf.normal.y;

        if (ny >= MAX_SLOPE_Y)      { surf.type = SURFACE_FLOOR; }
        else if (ny > MIN_SLOPE_Y)  { surf.type = SURFACE_SLOPE; }
        else if (ny <= -0.7f)       { surf.type = SURFACE_CEILING; }
        else                       { surf.type = SURFACE_WALL; }
    } else {
        surf.type = type;
    }

    surf.minX = (int)floorf(fminf(v0.x, fminf(v1.x, v2.x)));
    surf.minY = (int)floorf(fminf(v0.y, fminf(v1.y, v2.y)));
    surf.minZ = (int)floorf(fminf(v0.z, fminf(v1.z, v2.z)));

    surf.maxX = (int)ceilf(fmaxf(v0.x, fmaxf(v1.x, v2.x)));
    surf.maxY = (int)ceilf(fmaxf(v0.y, fmaxf(v1.y, v2.y)));
    surf.maxZ = (int)ceilf(fmaxf(v0.z, fmaxf(v1.z, v2.z)));

    int minChunkX = getChunkPos(surf.minX);
    int minChunkY = getChunkPos(surf.minY);
    int minChunkZ = getChunkPos(surf.minZ);

    int maxChunkX = getChunkPos(surf.maxX);
    int maxChunkY = getChunkPos(surf.maxY);
    int maxChunkZ = getChunkPos(surf.maxZ);

    surf.v0x = v2.x - v0.x;
    surf.v0y = v2.y - v0.y;
    surf.v0z = v2.z - v0.z;

    surf.v1x = v1.x - v0.x;
    surf.v1y = v1.y - v0.y;
    surf.v1z = v1.z - v0.z;

    surf.dot00 = surf.v0x*surf.v0x + surf.v0y*surf.v0y + surf.v0z*surf.v0z;
    surf.dot01 = surf.v0x*surf.v1x + surf.v0y*surf.v1y + surf.v0z*surf.v1z;
    surf.dot11 = surf.v1x*surf.v1x + surf.v1y*surf.v1y + surf.v1z*surf.v1z;

    float denom = surf.dot00 * surf.dot11 - surf.dot01 * surf.dot01;
    if (fabsf(denom) < 0.00001f) { surf.invDenom = 0.0f; }
    else { surf.invDenom = div_lut_float(denom); }

    for (int c=0; c < chunkAmt; c++) {
        int cx = collisionChunkSurfaces[c].pos.x;
        int cy = collisionChunkSurfaces[c].pos.y;
        int cz = collisionChunkSurfaces[c].pos.z;

        if (cx >= minChunkX && cx <= maxChunkX && cy >= minChunkY && cy <= maxChunkY && cz >= minChunkZ && cz <= maxChunkZ) {
            collisionChunkSurfaces[c].collisions = fox_realloc(collisionChunkSurfaces[c].collisions, sizeof(CollisionSurface) * (collisionChunkSurfaces[c].amt + 1));
            collisionChunkSurfaces[c].collisions[collisionChunkSurfaces[c].amt++] = surf;
        }
    }
}

Triggers cylinderInTrigger(Vec3f pos, float radius, float height) {
    float radius2 = radius * radius;

    for (int i = 0; i < triggerCount; i++) {
        Triggers *trig = &triggers[i];
        
        float minY = trig->pos.y - trig->size.y * 0.5f;
        float maxY = trig->pos.y + trig->size.y * 0.5f;

        if (pos.y > maxY + height || pos.y + height < minY) continue;
        
        float minX = trig->pos.x - trig->size.x * 0.5f;
        float maxX = trig->pos.x + trig->size.x * 0.5f;
        float minZ = trig->pos.z - trig->size.z * 0.5f;
        float maxZ = trig->pos.z + trig->size.z * 0.5f;
        
        float closestX = pos.x < minX ? minX : (pos.x > maxX ? maxX : pos.x);
        float closestZ = pos.z < minZ ? minZ : (pos.z > maxZ ? maxZ : pos.z);

        float dx = pos.x - closestX;
        float dz = pos.z - closestZ;

        if ((dx * dx + dz * dz) > radius2) continue;

        return *trig;
    }

    return (Triggers){0};
}

static Vec3f closestPointTriangle(Vec3f p, Vec3f a, Vec3f b, Vec3f c) {
    Vec3f ab = {b.x - a.x, b.y - a.y, b.z - a.z};
    Vec3f ac = {c.x - a.x, c.y - a.y, c.z - a.z};
    Vec3f ap = {p.x - a.x, p.y - a.y, p.z - a.z};

    float d1 = ab.x * ap.x + ab.y * ap.y + ab.z * ap.z;
    float d2 = ac.x * ap.x + ac.y * ap.y + ac.z * ap.z;
    if (d1 <= 0.0f && d2 <= 0.0f) return a;

    Vec3f bp = {p.x - b.x, p.y - b.y, p.z - b.z};
    float d3 = ab.x * bp.x + ab.y * bp.y + ab.z * bp.z;
    float d4 = ac.x * bp.x + ac.y * bp.y + ac.z * bp.z;
    if (d3 >= 0.0f && d4 <= d3) return b;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 * div_lut_float(d1 - d3);
        return (Vec3f){a.x + v * ab.x, a.y + v * ab.y, a.z + v * ab.z};
    }

    Vec3f cp = {p.x - c.x, p.y - c.y, p.z - c.z};
    float d5 = ab.x * cp.x + ab.y * cp.y + ab.z * cp.z;
    float d6 = ac.x * cp.x + ac.y * cp.y + ac.z * cp.z;
    if (d6 >= 0.0f && d5 <= d6) return c;

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 * div_lut_float(d2 - d6);
        return (Vec3f){a.x + w * ac.x, a.y + w * ac.y, a.z + w * ac.z};
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) * div_lut_float((d4 - d3) + (d5 - d6));
        return (Vec3f){b.x + w * (c.x - b.x), b.y + w * (c.y - b.y), b.z + w * (c.z - b.z)};
    }

    float denom = div_lut_float(va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;

    return (Vec3f){a.x + ab.x * v + ac.x * w, a.y + ab.y * v + ac.y * w, a.z + ab.z * v + ac.z * w};
}

VectMf cylinderInTriangle(Vec3f pos, float radius, float height) {
    VectMf pushPlayer = {0};

    float r = radius + TRI_EPSILON;
    float radius2 = r * r;

    Vec3f corrected = pos;
    Vec3f wallNormal = {0};

    static int debugCalls = 0;
    debugCalls++;

    int chunksPassed = 0;
    int trianglesPassed = 0;
    int distancePassed = 0;
    int correctionsApplied = 0;

    for (int iteration = 0; iteration < 4; iteration++) {
        bool found = false;

        for (int c = 0; c < chunkAmt; c++) {
            CollisionChunks *chunk = &collisionChunkSurfaces[c];

            if (chunk->amt == 0) continue;

            float minX = chunk->pos.x * CHUNK_SIZE;
            float minY = chunk->pos.y * CHUNK_SIZE;
            float minZ = chunk->pos.z * CHUNK_SIZE;

            if (corrected.x + r < minX || corrected.x - r > minX + CHUNK_SIZE) continue;
            if (corrected.y + height < minY || corrected.y > minY + CHUNK_SIZE) continue;
            if (corrected.z + r < minZ || corrected.z - r > minZ + CHUNK_SIZE) continue;

            chunksPassed++;

            for (int i = 0; i < chunk->amt; i++) {
                CollisionSurface *tri = &chunk->collisions[i];

                if (corrected.x + r < tri->minX || corrected.x - r > tri->maxX ||
                    corrected.y + height < tri->minY || corrected.y > tri->maxY ||
                    corrected.z + r < tri->minZ || corrected.z - r > tri->maxZ) continue;

                trianglesPassed++;

                Vec3f a = tri->v0;
                Vec3f b = tri->v1;
                Vec3f d = tri->v2;

                float axisY = tri->center.y;
                if (axisY < corrected.y) axisY = corrected.y;
                if (axisY > corrected.y + height) axisY = corrected.y + height;

                Vec3f axisPoint = {corrected.x, axisY, corrected.z};
                Vec3f closest = closestPointTriangle(axisPoint, a, b, d);

                axisPoint.y = fmaxf(corrected.y, fminf(closest.y, corrected.y + height));
                closest = closestPointTriangle(axisPoint, a, b, d);

                Vec3f diff = {
                    axisPoint.x - closest.x,
                    axisPoint.y - closest.y,
                    axisPoint.z - closest.z
                };

                float dist2 = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                if (dist2 >= radius2) continue;

                distancePassed++;

                if (tri->type == OUT_OF_BOUNDS) return (VectMf){0, 0, 0, -1, -1, -1, -1};

                float dist = sqrtf(dist2);
                float penetration = r - dist;

                Vec3f normal = tri->normal;

                if (dist > 0.00001f) {
                    float invDist = div_lut_float(dist);
                    normal = (Vec3f){diff.x * invDist, diff.y * invDist, diff.z * invDist};
                }

                Vec3f correction = {0};

                if (tri->type == SURFACE_FLOOR || tri->type == SURFACE_SLOPE) {
                    if (tri->normal.y <= 0.00001f) continue;

                    float dx = corrected.x - a.x;
                    float dz = corrected.z - a.z;

                    float surfaceY = a.y - (tri->normal.x * dx + tri->normal.z * dz) * div_lut_float(tri->normal.y);

                    if (corrected.y < surfaceY && corrected.y + height > surfaceY) {
                        correction.y = surfaceY - corrected.y;

                        if (tri->type == SURFACE_FLOOR) {
                            pushPlayer.floor = 1;
                        } else {
                            pushPlayer.slope = 1;
                            pushPlayer.slopeNormal = tri->normal;
                        }
                    } else {
                        continue;
                    }

                } else if (tri->type == SURFACE_CEILING) {
                    if (tri->normal.y >= -0.00001f) continue;

                    float dx = corrected.x - a.x;
                    float dz = corrected.z - a.z;

                    float surfaceY = a.y - (tri->normal.x * dx + tri->normal.z * dz) / tri->normal.y;

                    if (corrected.y + height > surfaceY && corrected.y < surfaceY) {
                        correction.y = surfaceY - (corrected.y + height);
                        pushPlayer.ceiling = 1;
                    } else {
                        continue;
                    }

                } else if (tri->type == SURFACE_WALL) {
                    float horizontalLength = sqrtf(normal.x * normal.x + normal.z * normal.z);
                    if (horizontalLength <= 0.00001f) continue;

                    float invHorizontal = div_lut_float(horizontalLength);

                    correction.x = penetration * normal.x * invHorizontal;
                    correction.z = penetration * normal.z * invHorizontal;

                    wallNormal = (Vec3f){normal.x * invHorizontal, 0.0f, normal.z * invHorizontal};
                    pushPlayer.wall = 1;

                } else {
                    continue;
                }

                corrected.x += correction.x;
                corrected.y += correction.y;
                corrected.z += correction.z;

                correctionsApplied++;
                found = true;
            }
        }

        if (!found) break;
    }

    pushPlayer.pos.x = corrected.x;
    pushPlayer.pos.y = corrected.y;
    pushPlayer.pos.z = corrected.z;

    pushPlayer.normal = wallNormal;

    return pushPlayer;
}

void setupMapCollision(Mesh map, Vec2f pos) {
    fixSurfaces(map, pos);
    collisionChunks();

    for (int i = 0; i < map.triCount; i++) {
        int a = map.tris[i].a;
        int b = map.tris[i].b;
        int c = map.tris[i].c;

        if (a < 0 || b < 0 || c < 0) continue;

        Vec3f v0 = map.verts[a];
        Vec3f v1 = map.verts[b];
        Vec3f v2 = map.verts[c];

        v0.x += pos.x; v0.z += pos.y;
        v1.x += pos.x; v1.z += pos.y;
        v2.x += pos.x; v2.z += pos.y;

        Vec3f u = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
        Vec3f v = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

        Vec3f normal = {
            u.y * v.z - u.z * v.y,
            u.z * v.x - u.x * v.z,
            u.x * v.y - u.y * v.x
        };

        float length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (length < 0.00001f) continue;

        float invLength = 1.0f / length;

        normal.x *= invLength;
        normal.y *= invLength;
        normal.z *= invLength;

        addCollisionSurface(v0, v1, v2, normal, SURFACE_NONE);
    }
}
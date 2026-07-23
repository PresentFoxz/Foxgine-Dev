#ifndef CHUNKS_H
#define CHUNKS_H

#include "fox_structs.h"
#include "fox_mesh.h"
#include "fox_perlin.h"
#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 2

#define BLOCK_X 6
#define BLOCK_Y 6
#define BLOCK_Z 6
#define BLOCK_COUNT (BLOCK_Y * BLOCK_X * BLOCK_Z)
typedef struct {
    uint8_t blocks[BLOCK_COUNT];
    Vec3i pos;

    uint8_t LOD;
    bool renderable;
} Chunk_t;

#define CHUNK_X 1
#define CHUNK_Y 1
#define CHUNK_Z 1

#define RANGE_X ((CHUNK_X * 2) + 1)
#define RANGE_Y ((CHUNK_Y * 2) + 1)
#define RANGE_Z ((CHUNK_Z * 2) + 1)
#define CHUNK_AMT (RANGE_X * RANGE_Y * RANGE_Z)

extern Chunk_t chunkData[CHUNK_AMT];
extern Vec3i chunkRadius[CHUNK_AMT];

static inline int getBlockIndex(int x, int y, int z) {
    if ((x < 0 || x >= BLOCK_X) || (y < 0 || y >= BLOCK_Y) || (z < 0 || z >= BLOCK_Z)) { return -1; }
    return x + (y * BLOCK_X) + (z * BLOCK_X * BLOCK_Y);
}

static inline int getChunkIndex(int x, int y, int z) {
    if (x < -CHUNK_X || x > CHUNK_X || y < -CHUNK_Y || y > CHUNK_Y || z < -CHUNK_Z || z > CHUNK_Z) { return -1; }
    
    int ix = x + CHUNK_X;
    int iy = y + CHUNK_Y;
    int iz = z + CHUNK_Z;

    return ix + RANGE_X * (iy + RANGE_Y * iz);
}

static bool getVoxelSafe(int cx, int cy, int cz, int x, int y, int z) {
    int idx = getBlockIndex(x, y, z);
    if (idx == -1) return true;

    for (int i = 0; i < CHUNK_AMT; i++) {
        Chunk_t *chunk = &chunkData[i];

        if (chunk->pos.x == cx && chunk->pos.y == cy && chunk->pos.z == cz) return chunk->blocks[idx] != 0;
    } return true;
}

static inline bool block_exists(int currChunk, int nx, int ny, int nz) {
    int newChunk = currChunk;
    int blockID = getBlockIndex(nx, ny, nz);

    if (blockID != -1) return chunkData[newChunk].blocks[blockID] != 0;

    int cX = chunkData[newChunk].pos.x;
    int cY = chunkData[newChunk].pos.y;
    int cZ = chunkData[newChunk].pos.z;

    if (nx < 0)   { cX--; nx = BLOCK_X - 1; }
    else if (nx >= BLOCK_X) { cX++; nx = 0; }

    if (ny < 0)   { cY--; ny = BLOCK_Y - 1; }
    else if (ny >= BLOCK_Y) { cY++; ny = 0; }

    if (nz < 0)   { cZ--; nz = BLOCK_Z - 1; }
    else if (nz >= BLOCK_Z) { cZ++; nz = 0; }

    return getVoxelSafe(cX, cY, cZ, nx, ny, nz);
}

static void freeMesh(Mesh *mesh) {
    if(mesh->verts) fox_free(mesh->verts);
    if(mesh->tris) fox_free(mesh->tris);

    mesh->verts = NULL;
    mesh->tris = NULL;

    mesh->vertCount = 0;
    mesh->triCount = 0;

    mesh->matrix = (Mat3x3){0};
    mesh->rotated = false;
}

static inline Mesh mesh_create(Chunk_t chunkData, int chunkID, Mesh *meshTypes) {
    Mesh chunkMesh;

    chunkMesh.vertCount = 0;
    chunkMesh.triCount = 0;

    chunkMesh.verts = NULL;
    chunkMesh.tris = NULL;

    chunkMesh.tris = fox_malloc(sizeof(TriIndex) * BLOCK_COUNT * 6 * 2);
    chunkMesh.verts = fox_malloc(sizeof(Vec3f) * BLOCK_COUNT * 6 * 2 * 3);

    for(int y = 0; y < BLOCK_Y; y++) {
        for(int x = 0; x < BLOCK_X; x++) {
            for(int z = 0; z < BLOCK_Z; z++) {
                int index = x + (y * BLOCK_X) + (z * BLOCK_X * BLOCK_Y);

                uint8_t blockID = chunkData.blocks[index];
                if (blockID == 0) continue;

                Mesh blockMesh = meshTypes[blockID-1];

                bool visible[6];
                visible[0] = !block_exists(chunkID, x, y, z+1);
                visible[1] = !block_exists(chunkID, x, y, z-1);
                visible[2] = !block_exists(chunkID, x, y+1, z);
                visible[3] = !block_exists(chunkID, x, y-1, z);
                visible[4] = !block_exists(chunkID, x+1, y, z);
                visible[5] = !block_exists(chunkID, x-1, y, z);

                for(int f = 0; f < 6; f++) {
                    if(!visible[f]) continue;

                    int triStart = f * 2;
                    for(int t = 0; t < 2; t++) {
                        TriIndex tri = blockMesh.tris[triStart+t];

                        int ids[3] = { tri.a, tri.b, tri.c };
                        int newIDs[3];
                        for(int j = 0; j < 3; j++) {
                            Vec3f pos = blockMesh.verts[(j == 0) ? tri.a : (j == 1) ? tri.b : tri.c];
                                
                            pos.x += x * BLOCK_SIZE;
                            pos.y += y * BLOCK_SIZE;
                            pos.z += z * BLOCK_SIZE;

                            newIDs[j] = chunkMesh.vertCount;

                            chunkMesh.verts[chunkMesh.vertCount++] = pos;
                        }

                        tri.a = newIDs[0];
                        tri.b = newIDs[1];
                        tri.c = newIDs[2];

                        chunkMesh.tris[chunkMesh.triCount++] = tri;
                    }
                }
            }
        }
    }

    return chunkMesh;
}

// world generation //

typedef struct {
    float baseHeight;
    float erosionStrength;
    float peakHeight;
    float localVariation;
} TerrainParams;

static TerrainParams terrainTypes[] = {
    { 80, 0.2f, 10, 5 },   // flatlands
    { 100, 0.4f, 30, 10 }, // hills
    { 120, 0.6f, 60, 20 }  // mountains
};
static int numTerrainTypes = sizeof(terrainTypes)/sizeof(terrainTypes[0]);

static float getContinentalness(float x, float z) {
    float freq = 0.003f;
    float n = perlin2D(x * freq, z * freq);
    return 0.5f * (n + 1.0f);
}

static float getErosion(float x, float z) {
    float freq = 0.01f;
    float n = perlin2D(x * freq, z * freq);
    return 0.5f * (n + 1.0f);
}

static float getPeaksValleys(float x, float z) {
    float freq = 0.02f;
    float n = perlin2D(x * freq, z * freq);
    return 0.5f * (n + 1.0f);
}

static int getBlock(float x, float y, float z){
    float c = getContinentalness(x, z);
    float e = getErosion(x, z);
    float p = getPeaksValleys(x, z);
    
    int terrainIndex = (int)(c * (numTerrainTypes - 1) + 0.5f);

    TerrainParams t = terrainTypes[terrainIndex];

    float baseHeight = t.baseHeight;
    float erosionFactor = 1.0f - e * t.erosionStrength;
    float peakFactor = p * t.peakHeight;

    float localFreq = 0.05f;
    float localNoise = 0.5f * (perlin3D(x*localFreq, y*localFreq, z*localFreq) + 1.0f);
    float localVariation = localNoise * t.localVariation;

    float surfaceY = baseHeight + peakFactor - e * t.erosionStrength + localVariation;
    return (y < surfaceY) ? 1 : 0;
}

static Chunk_t createWorld(Vec3i worldPos) {
    Chunk_t worldGen = {0};

    worldGen.LOD = 0;
    worldGen.renderable = false;

    int blockAmt = 0;
    for (int y=0; y < BLOCK_Y; y++) {
        int worldY = worldPos.y * BLOCK_Y + y;

        for (int x=0; x < BLOCK_X; x++) {
            int worldX = worldPos.x * BLOCK_X + x;
                
            for (int z=0; z < BLOCK_Z; z++) {
                int worldZ = worldPos.z * BLOCK_Z + z;
                
                int index = getBlockIndex(x, y, z);
                uint8_t v = getBlock(worldX, worldY, worldZ);
                worldGen.blocks[index] = v;

                if (v) blockAmt++;
            }
        }
    }

    if (blockAmt > 0) worldGen.renderable = true;
    return worldGen;
}

#endif
#ifndef CHUNKS_H
#define CHUNKS_H

#include "fox_structs.h"
#include "fox_mesh.h"
#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 2

#define BLOCK_X 5
#define BLOCK_Y 5
#define BLOCK_Z 5
#define BLOCK_COUNT (BLOCK_Y * BLOCK_X * BLOCK_Z)
typedef struct {
    uint8_t blocks[BLOCK_COUNT];
    Vec3i pos;

    uint8_t LOD;
} Chunk_t;

#define CHUNK_XM 2
#define CHUNK_XP 2
#define CHUNK_YM 2
#define CHUNK_YP 2
#define CHUNK_ZM 2
#define CHUNK_ZP 2
#define CHUNK_AMT ((CHUNK_XM + CHUNK_XP + 1) * (CHUNK_YM + CHUNK_YP + 1) * (CHUNK_ZM + CHUNK_ZP + 1))
extern Chunk_t chunkData[CHUNK_AMT];
extern Vec3i chunkRadius[CHUNK_AMT];

static inline bool block_exists(Chunk_t chunk, int x, int y, int z) {
    if ((x < 0 || x >= BLOCK_X) || (y < 0 || y >= BLOCK_Y) || (z < 0 || z >= BLOCK_Z)) { return false; }

    int index = x + (y * BLOCK_X) + (z * BLOCK_X * BLOCK_Y);

    return chunk.blocks[index] != 0;
}

static inline Chunk_t random_chunk_data(int seed, Vec3i pos) {
    Chunk_t chunk;

    int index;
    for (int y=0; y < BLOCK_Y; y++) {
        for (int x=0; x < BLOCK_X; x++) {
            for (int z=0; z < BLOCK_Z; z++) {
                index = x + (y * BLOCK_X) + (z * BLOCK_X * BLOCK_Y);

                chunk.blocks[index] = rand() % 2;
            }
        }
    }

    chunk.pos = pos;
    int lodDist = 1;

    chunk.LOD = (abs(pos.x) <= lodDist && abs(pos.y) <= lodDist && abs(pos.z) <= lodDist) ? 0 : 1;

    return chunk;
}

static inline Mesh mesh_create(Chunk_t chunkData, Mesh *meshTypes) {
    Mesh chunkMesh;

    chunkMesh.vertCount = 0;
    chunkMesh.triCount = 0;

    chunkMesh.verts = NULL;
    chunkMesh.tris = NULL;

    for(int y = 0; y < BLOCK_Y; y++) {
        for(int x = 0; x < BLOCK_X; x++) {
            for(int z = 0; z < BLOCK_Z; z++) {
                int index = x + (y * BLOCK_X) + (z * BLOCK_X * BLOCK_Y);

                uint8_t blockID = chunkData.blocks[index];
                if (blockID == 0) continue;

                Mesh blockMesh = meshTypes[blockID-1];

                bool visible[6];
                visible[0] = !block_exists(chunkData, x, y, z+1);
                visible[1] = !block_exists(chunkData, x, y, z-1);
                visible[2] = !block_exists(chunkData, x, y+1, z);
                visible[3] = !block_exists(chunkData, x, y-1, z);
                visible[4] = !block_exists(chunkData, x+1, y, z);
                visible[5] = !block_exists(chunkData, x-1, y, z);

                for(int f = 0; f < 6; f++) {
                    if(!visible[f]) continue;

                    int triStart = f * 2;
                    for(int t = 0; t < 2; t++) {
                        TriIndex tri = blockMesh.tris[triStart+t];

                        int ids[3] = { tri.a, tri.b, tri.c };
                        int newIDs[3];

                        TriIndex *newTris = fox_realloc(chunkMesh.tris, sizeof(TriIndex) * (chunkMesh.triCount + 2));
                        if (newTris == NULL) continue;

                        chunkMesh.tris = newTris;

                        for(int t = 0; t < 2; t++) {
                            TriIndex tri = blockMesh.tris[triStart + t];
                            
                            int newIDs[3];
                            for(int j = 0; j < 3; j++) {
                                Vec3f pos = blockMesh.verts[(j == 0) ? tri.a : (j == 1) ? tri.b : tri.c];
                                
                                pos.x += x * BLOCK_SIZE;
                                pos.y += y * BLOCK_SIZE;
                                pos.z += z * BLOCK_SIZE;

                                Vec3f *newVerts = fox_realloc(chunkMesh.verts, sizeof(Vec3f) * (chunkMesh.vertCount + 1));
                                if(newVerts == NULL) continue;

                                chunkMesh.verts = newVerts;
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
    }

    return chunkMesh;
}

#endif
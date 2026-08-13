#include "fox_library.h"
#include "fox_scene.h"
#include "fox_structs.h"
#include "fox_mesh.h"
#include "fox_draw.h"

#include "objects/entities.h"
#include "chunk_data/chunks.h"

#define FPS 30

Camera_t cam;
Pixel_t *mainBuffer;
Pixel_t *screenBuffer;
int interlace = 0;
int interlaceAmt = 1;
bool canInterlace = false;
bool pause = false;

Mesh *blockTypes;
Mesh chunkMesh[CHUNK_AMT];
Chunk_t chunkData[CHUNK_AMT];
Vec3i chunkRadius[CHUNK_AMT];

MeshAnimations *animModels;
Objects_t *objList;

const int MAIN_SCREEN_W = 240;
const int MAIN_SCREEN_H = 320;
const int SCREEN_W = (MAIN_SCREEN_W / 2);
const int SCREEN_H = (MAIN_SCREEN_H / 2);

static void init() {
    mainBuffer = fox_malloc(SCREEN_W * SCREEN_H * sizeof(Pixel_t));
    blockTypes = fox_malloc(1 * sizeof(Mesh));

    cam = (Camera_t){
        .pos = (Vec3f){0.0f, 100.0f * BLOCK_SIZE, 0.0f}, .rot = (Vec3f){0.0f, 0.0f, 0.0f},
        .fov = 90.0f, .nearPlane = 0.001f, .farPlane = 1000.0f
    };

    load_mesh(&blockTypes[0], "mesh/Cube.fox");

    int index = 0;
    for (int y=-CHUNK_Y; y <= CHUNK_Y; y++) {
        for (int x=-CHUNK_X; x <= CHUNK_X; x++) {
            for (int z=-CHUNK_Z; z <= CHUNK_Z; z++) {
                chunkRadius[index++] = (Vec3i){x, y, z};
            }
        }
    }
    
    perlinInit(245773891241230);

    animModels = fox_malloc(sizeof(MeshAnimations) * 1);
    objList = fox_malloc(sizeof(Objects_t) * 1);

    // objList[0] = (Objects_t){.pos = (Vec3f){0, -7, 0}, .rot = (Vec3f){0, 0, 0}, .size = (Vec3f){1, 1, 1}, .modelID = 0, .distMod = 50.0f};
    // load_animation(&animModels[0], "mesh/chicken/anim.vul");
    // add_objCount(1);
}

static void scale_buffer(Pixel_t *src, int srcWidth, int srcHeight, Pixel_t *dst, int dstWidth, int dstHeight) {
    for (int y = 0; y < dstHeight; y++) {
        for (int x = 0; x < dstWidth; x++) {
            int srcX = (srcWidth - 1) - (y * srcWidth / dstHeight);
            int srcY = x * srcHeight / dstWidth;
            
            dst[y * dstWidth + x] = src[srcY * srcWidth + srcX];
        }
    }
}

static void create_chunks(Vec3i offset) {
    reset_triCount();
    for (int i=0; i < CHUNK_AMT; i++) {
        Vec3i newOffset = {chunkRadius[i].x + offset.x, chunkRadius[i].y + offset.y, chunkRadius[i].z + offset.z};
        chunkData[i] = createWorld(newOffset);
        chunkData[i].pos = newOffset;

        freeMesh(&chunkMesh[i]);
    }

    int renderable = 0;
    for (int i=0; i < CHUNK_AMT; i++) {
        if (!chunkData[i].renderable) continue;
        if (chunkData[i].LOD == 1) continue;

        chunkMesh[i] = mesh_create(chunkData[i], i, blockTypes);

        if (chunkMesh[i].triCount <= 0) { chunkData[i].renderable = false; continue; }
        add_triCount(chunkMesh[i].triCount);
        renderable++;
    } add_triCount(12);
    alloc_mesh();
}

static void reMesh_chunks() {
    reset_triCount();
    for (int i=0; i < CHUNK_AMT; i++) {
        if (chunkData[i].LOD != 0) continue;
        chunkData[i].renderable = false;

        freeMesh(&chunkMesh[i]);
        chunkMesh[i] = mesh_create(chunkData[i], i, blockTypes);
        if (chunkMesh[i].triCount > 0) { chunkData[i].renderable = true; }
        if (!chunkData[i].renderable) { freeMesh(&chunkMesh[i]); continue; }

        add_triCount(chunkMesh[i].triCount);
    }
    add_triCount(12);
    alloc_mesh();
}

static bool onStart = true;
static Vec3i currChunk, lastChunk;
static void run_game() {
    if (onStart) {
        currChunk = (Vec3i){ floor_div(cam.pos.x, (BLOCK_X * BLOCK_SIZE)), floor_div(cam.pos.y, (BLOCK_Y * BLOCK_SIZE)), floor_div(cam.pos.z, (BLOCK_Z * BLOCK_SIZE)) };

        create_chunks(currChunk);
        onStart = false;
        lastChunk = currChunk;

        return;
    }

    float deltaTime = pb_timing_delta_time();

    interlace ^= 1;
    clear_buf(0);

    move_camera(&cam, deltaTime);
    computeCamData(&cam);

    currChunk = (Vec3i){ floor_div(cam.pos.x, (BLOCK_X * BLOCK_SIZE)), floor_div(cam.pos.y, (BLOCK_Y * BLOCK_SIZE)), floor_div(cam.pos.z, (BLOCK_Z * BLOCK_SIZE)) };
    RayHit result = raycast(cam, currChunk);

    if (pb_gamepad_button_pressed(PB_X)) { destroy_voxel(result); reMesh_chunks(); }

    if (lastChunk.x != currChunk.x || lastChunk.y != currChunk.y || lastChunk.z != currChunk.z) create_chunks(currChunk);

    // computeMatrixModel(&blockTypes[0], (Vec3f){0, 0, 0}, (Vec3f){1.0f, 1.0f, 1.0f});
    // add_mesh_scene(blockTypes[0], (Vec3f){0, 0, 0}, cam, false);
    for (int i=0; i < CHUNK_AMT; i++) {
        if (!chunkData[i].renderable) continue;
        computeMatrixModel(&chunkMesh[i], (Vec3f){0, 0, 0}, (Vec3f){1.0f, 1.0f, 1.0f});

        add_mesh_scene(
            chunkMesh[i], 
            (Vec3f){(chunkData[i].pos.x * BLOCK_SIZE) * BLOCK_X, (chunkData[i].pos.y * BLOCK_SIZE) * BLOCK_Y, (chunkData[i].pos.z * BLOCK_SIZE) * BLOCK_Z},
            cam, false
        );
    }

    // add_obj_scene(objList[0].pos, objList[0].distMod, cam, 0);
    draw_tris(cam, objList, animModels);

    lastChunk = currChunk;
}

void app_main(void) {
    pb_gfx_pax_init();
    pb_timing_init();

    init();
    for (int i=0; i < CHUNK_AMT; i++) { add_triCount(chunkMesh[i].triCount); }
    alloc_mesh();

    pax_buf_t *gfx = pb_gfx_pax_get_buf();
    if (!gfx) return;

    screenBuffer = (Pixel_t *)pax_buf_get_pixels_rw(gfx);
    if (!screenBuffer) return;

    while (pb_timing_loop(FPS)) {
        pb_gamepad_poll();
        run_game();

        scale_buffer(mainBuffer, SCREEN_W, SCREEN_H, screenBuffer, MAIN_SCREEN_W, MAIN_SCREEN_H);
        pb_gfx_pax_flush();
    }

    return;
}
#include "fox_library.h"
#include "fox_scene.h"
#include "fox_structs.h"
#include "fox_mesh.h"
#include "fox_draw.h"
#include "fox_perlin.h"

#include "objects/entities.h"
#include "chunk_data/chunks.h"

PlaydateAPI* pd;
#define FPS 30

Camera_t cam;
Pixel_t *mainBuffer;
Pixel_t *screenBuffer;
int interlace = 0;
int interlaceAmt = 1;
bool canInterlace = true;

bool firstRun = true;

Mesh *blockTypes;
Mesh chunkMesh[CHUNK_AMT];
Chunk_t chunkData[CHUNK_AMT];
Vec3i chunkRadius[CHUNK_AMT];

static int update(void* userdata);

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg) {
	if ( event == kEventInit ) {
        pd = playdate;

		pd->display->setRefreshRate(FPS);
		pd->system->setUpdateCallback(update, NULL);
	}

	return 0;
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
    } alloc_mesh();
}

static int init() {
    // screenBuffer = fox_malloc(MAIN_SCREEN_W * MAIN_SCREEN_H * sizeof(Pixel_t));
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

    pd->system->logToConsole("Ran init function!");
    return 0;
}

static void draw_to_playdate() {
    screenBuffer = pd->graphics->getFrame();

    bool upscale = false;
    if (MAIN_SCREEN_W != SCREEN_W || MAIN_SCREEN_H != SCREEN_H) { upscale = true; }

    for(int y = 0; y < MAIN_SCREEN_H; y++) {
        for(int x = 0; x < MAIN_SCREEN_W; x++) {
            Vec2i sp = {x, y};
            if (upscale) {
                sp.x = x >> 1;
                sp.y = y >> 1;
            }

            Pixel_t pixel = mainBuffer[sp.y * SCREEN_W + sp.x];

            int index = y * 52 + (x >> 3);
            uint8_t mask = 1 << (7 - (x & 7));

            if(pixel) { screenBuffer[index] |= mask; }
            else { screenBuffer[index] &= ~mask; }
        }
    }

    pd->graphics->markUpdatedRows(0, MAIN_SCREEN_H - 1);
}

static Vec3i currChunk, lastChunk;
static int update(void* userdata) {
    if (firstRun) {
        init();
        
        currChunk = (Vec3i){ floor_div(cam.pos.x, (BLOCK_X * BLOCK_SIZE)), floor_div(cam.pos.y, (BLOCK_Y * BLOCK_SIZE)), floor_div(cam.pos.z, (BLOCK_Z * BLOCK_SIZE)) };

        create_chunks(currChunk);
        lastChunk = currChunk;
        firstRun = false;
        
        return 1;
    }
    interlace ^= 1;
    // pd->graphics->setDrawMode(kDrawModeFillWhite);
    clear_buf(0);
    
    float dt = pd->system->getElapsedTime();
    pd->system->resetElapsedTime();

    move_camera(&cam, dt);
    computeCamData(&cam);

    currChunk = (Vec3i){ floor_div(cam.pos.x, (BLOCK_X * BLOCK_SIZE)), floor_div(cam.pos.y, (BLOCK_Y * BLOCK_SIZE)), floor_div(cam.pos.z, (BLOCK_Z * BLOCK_SIZE)) };

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
    } draw_tris(cam);

    lastChunk = currChunk;
    
    draw_to_playdate();

    pd->graphics->fillRect(0, 0, 20, 20, kColorWhite);
    pd->system->drawFPS(2, 2);

    return 1;
}
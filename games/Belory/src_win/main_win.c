#include "fox_library.h"
#include "fox_scene.h"
#include "fox_structs.h"
#include "fox_mesh.h"
#include "fox_draw.h"

#include "objects/entities.h"
#include "chunk_data/chunks.h"

#define FPS 60
#define FRAME_TIME (1000 / FPS)

Uint32 frameStart;
Uint32 frameTime;

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

SDL_Window* window;
Uint64 lastTime;
float deltaTime;

KeyInputs inputs = {0};
static KeyInputs prevInputs = {0};
static void check_inputs() {
    SDL_PumpEvents();
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    inputs.up = keys[SDL_SCANCODE_W];
    inputs.down = keys[SDL_SCANCODE_S];
    inputs.left = keys[SDL_SCANCODE_A];
    inputs.right = keys[SDL_SCANCODE_D];

    inputs.jump = keys[SDL_SCANCODE_SPACE];
    inputs.crouch = keys[SDL_SCANCODE_LSHIFT];

    inputs.pause = keys[SDL_SCANCODE_ESCAPE];
    inputs.just_pause = keys[SDL_SCANCODE_ESCAPE] && !prevInputs.pause;

    inputs.just_up = keys[SDL_SCANCODE_W] && !prevInputs.up;
    inputs.just_down = keys[SDL_SCANCODE_S] && !prevInputs.down;
    inputs.just_left = keys[SDL_SCANCODE_A] && !prevInputs.left;
    inputs.just_right = keys[SDL_SCANCODE_D] && !prevInputs.right;

    inputs.just_jump = keys[SDL_SCANCODE_SPACE] && !prevInputs.jump;
    inputs.just_crouch = keys[SDL_SCANCODE_LSHIFT] && !prevInputs.crouch;

    prevInputs = inputs;
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

    interlace ^= 1;
    clear_buf(0);

    check_inputs();

    Vec2i newPos = mouse_move(window, pause);
    if (!pause) { move_camera(&cam, inputs, newPos, pause, deltaTime); }
    computeCamData(&cam);

    currChunk = (Vec3i){ floor_div(cam.pos.x, (BLOCK_X * BLOCK_SIZE)), floor_div(cam.pos.y, (BLOCK_Y * BLOCK_SIZE)), floor_div(cam.pos.z, (BLOCK_Z * BLOCK_SIZE)) };

    if (lastChunk.x != currChunk.x || lastChunk.y != currChunk.y || lastChunk.z != currChunk.z) create_chunks(currChunk);

    printf("Cam Pos: [ %d | %d | %d ]\n", floor_div(cam.pos.x, BLOCK_SIZE), floor_div(cam.pos.y, BLOCK_SIZE), floor_div(cam.pos.z, BLOCK_SIZE));

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
}

static void init() {
    screenBuffer = fox_malloc(MAIN_SCREEN_W * MAIN_SCREEN_H * sizeof(Pixel_t));
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
}

static void scale_buffer(Pixel_t *src, int srcWidth, int srcHeight, Pixel_t *dst, int dstWidth, int dstHeight) {
    int yStep = (srcHeight << 16) / dstHeight;

    int srcY = 0;
    for (int y = 0; y < dstHeight; y++) {
        int xStep = (srcWidth << 16) / dstWidth;
        int srcX = 0;

        Pixel_t *srcRow = src + ((srcY >> 16) * srcWidth);
        Pixel_t *dstRow = dst + y * dstWidth;

        for (int x = 0; x < dstWidth; x++) {
            dstRow[x] = srcRow[srcX >> 16];
            srcX += xStep;
        } srcY += yStep;
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }
    window = SDL_CreateWindow("Foxgine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, MAIN_SCREEN_W, MAIN_SCREEN_H, SDL_WINDOW_SHOWN);

    if (!window) {
        SDL_Log("Window Error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer) {
        SDL_Log("Renderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    SDL_Texture* screenBlit = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, MAIN_SCREEN_W, MAIN_SCREEN_H);

    init();
    for (int i=0; i < CHUNK_AMT; i++) { add_triCount(chunkMesh[i].triCount); }
    alloc_mesh();

    lastTime = SDL_GetPerformanceCounter();
    while (running) {
        frameStart = SDL_GetTicks();

        Uint64 currentTime = SDL_GetPerformanceCounter();
        deltaTime = (float)(currentTime - lastTime) / SDL_GetPerformanceFrequency();
        lastTime = currentTime;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (inputs.just_pause) {
            pause = !pause;

            if (!pause) {
                int centerX = MAIN_SCREEN_W / 2;
                int centerY = MAIN_SCREEN_H / 2;

                SDL_WarpMouseInWindow(window, centerX, centerY);
            }
        }

        if (pause) { SDL_SetRelativeMouseMode(SDL_FALSE); }
        else { SDL_SetRelativeMouseMode(SDL_TRUE); }

        run_game();
        scale_buffer(mainBuffer, SCREEN_W, SCREEN_H, screenBuffer, MAIN_SCREEN_W, MAIN_SCREEN_H);

        SDL_UpdateTexture(screenBlit, NULL, screenBuffer, MAIN_SCREEN_W * sizeof(Pixel_t));
        SDL_RenderCopy(renderer, screenBlit, NULL, NULL);

        SDL_RenderPresent(renderer);

        if (frameTime < FRAME_TIME) { SDL_Delay(FRAME_TIME - frameTime); }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
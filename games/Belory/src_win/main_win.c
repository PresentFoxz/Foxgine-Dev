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
bool canInterlace = true;

int crankRotation = 0;

Mesh *blockTypes;
Mesh chunkMesh[CHUNK_AMT];
Chunk_t chunkData[CHUNK_AMT];
Vec3i chunkRadius[CHUNK_AMT];

Pixel_t bgColor;
KeyInputs inputs = {0};
static KeyInputs prevInputs = {0};
static void check_inputs() {
    SDL_PumpEvents();
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    inputs.up = keys[SDL_SCANCODE_W];
    inputs.down = keys[SDL_SCANCODE_S];
    inputs.left = keys[SDL_SCANCODE_A];
    inputs.right = keys[SDL_SCANCODE_D];

    inputs.a = keys[SDL_SCANCODE_J];
    inputs.b = keys[SDL_SCANCODE_K];
    inputs.lb = keys[SDL_SCANCODE_U];
    inputs.rb = keys[SDL_SCANCODE_I];

    inputs.just_up = keys[SDL_SCANCODE_W] && !prevInputs.up;
    inputs.just_down = keys[SDL_SCANCODE_S] && !prevInputs.down;
    inputs.just_left = keys[SDL_SCANCODE_A] && !prevInputs.left;
    inputs.just_right = keys[SDL_SCANCODE_D] && !prevInputs.right;

    inputs.just_a = keys[SDL_SCANCODE_J] && !prevInputs.a;
    inputs.just_b = keys[SDL_SCANCODE_K] && !prevInputs.b;
    inputs.just_lb = keys[SDL_SCANCODE_U] && !prevInputs.lb;
    inputs.just_rb = keys[SDL_SCANCODE_I] && !prevInputs.rb;

    prevInputs = inputs;
}

static void run_game() {
    interlace ^= 1;
    clear_buf(bgColor);

    check_inputs();
    crank_adjust(inputs, &crankRotation);
    move_camera(&cam, inputs, crankRotation);
    computeCamData(&cam);

    // add_mesh_scene(blockTypes[0], (Vec3f){0, 0, 0}, (Vec3f){0, 0, 0}, (Vec3f){1.0f, 1.0f, 1.0f}, cam, false);
    for (int i=0; i < CHUNK_AMT; i++) {
        add_mesh_scene(
            chunkMesh[i], 
            (Vec3f){(chunkData[i].pos.x * BLOCK_SIZE) * BLOCK_X, (chunkData[i].pos.y * BLOCK_SIZE) * BLOCK_Y, (chunkData[i].pos.z * BLOCK_SIZE) * BLOCK_Z},
            (Vec3f){0, 0, 0},
            (Vec3f){1.0f, 1.0f, 1.0f},
            cam, false
        );
    }
    draw_tris(cam);
}

static void init() {
    screenBuffer = malloc(MAIN_SCREEN_W * MAIN_SCREEN_H * sizeof(Pixel_t));
    mainBuffer = malloc(SCREEN_W * SCREEN_H * sizeof(Pixel_t));
    blockTypes = malloc(1 * sizeof(Mesh));

    cam = (Camera_t){
        .pos = (Vec3f){0.0f, 0.0f, 0.0f}, .rot = (Vec3f){0.0f, 0.0f, 0.0f},
        .fov = 90.0f, .nearPlane = 0.001f, .farPlane = 1000.0f
    };

    bgColor = color_to_pixel(0);

    load_mesh(&blockTypes[0], "mesh/Cube.fox");
    
    int index = 0;
    for (int y=-CHUNK_YM; y <= CHUNK_YP; y++) {
        for (int x=-CHUNK_XM; x <= CHUNK_XP; x++) {
            for (int z=-CHUNK_ZM; z <= CHUNK_ZP; z++) {
                chunkRadius[index++] = (Vec3i){x, y, z};
            }
        }
    }

    for (int i=0; i < CHUNK_AMT; i++) {
        chunkData[i] = random_chunk_data(0, chunkRadius[i]);
        if (chunkData[i].LOD == 1) continue;

        chunkMesh[i] = mesh_create(chunkData[i], blockTypes);
    }
}

static inline void scale_buffer(Pixel_t *src, int srcWidth, int srcHeight, Pixel_t *dst, int dstWidth, int dstHeight) {
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
    SDL_Window* window = SDL_CreateWindow("Foxgine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, MAIN_SCREEN_W, MAIN_SCREEN_H, SDL_WINDOW_SHOWN);

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
    alloc_mesh();
    while (running) {
        frameStart = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

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
#include "fox_library.h"
#include "fox_scene.h"
#include "fox_structs.h"
#include "fox_mesh.h"
#include "fox_draw.h"
#include "fox_palette.h"

#include "objects/entities.h"

#include "mesh/allMeshes.h"

#define FPS 60
#define FRAME_TIME (1000 / FPS)

Uint32 frameStart;
Uint32 frameTime;

Camera_t cam;
Pixel_t *mainBuffer = NULL;
int interlace = 0;

Mesh map;

Pixel_t bgColor;
KeyInputs inputs = {0};
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
}

static void run_game() {
    interlace ^= 1;
    clear_buf(bgColor);

    check_inputs();
    move_camera(&cam, inputs);
    computeCamData(&cam);

    add_mesh_scene(map, (Vec3s24){0, 0, 0}, (Vec3s24){0, 0, 0}, (Vec3s24){to_fixed24(1.0f), to_fixed24(1.0f), to_fixed24(1.0f)}, cam);
    draw_tris(cam);
}

static void init() {
    initTable();

    mainBuffer = malloc(SCREEN_W * SCREEN_H * sizeof(Pixel_t));
    cam = (Camera_t){
        .pos = (Vec3s24){to_fixed24(0.0f), to_fixed24(0.0f), to_fixed24(-2.0f)}, .rot = (Vec3s24){to_fixed24(0.0f), to_fixed24(0.0f), to_fixed24(0.0f)},
        .fov = to_fixed24(90.0f), .nearPlane = to_fixed24(0.1f), .farPlane = to_fixed24(1000.0f)
    };

    bgColor = color_to_pixel(palettes[0]);
    load_mesh(&map, Cube_verts, CUBE_VERT, Cube_tris, CUBE_TRI, Cube_colors);
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Foxgine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);

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

    SDL_Texture* screenBlit = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);

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

        SDL_UpdateTexture(screenBlit, NULL, mainBuffer, SCREEN_W * sizeof(Pixel_t));
        SDL_RenderCopy(renderer, screenBlit, NULL, NULL);

        SDL_RenderPresent(renderer);

        if (frameTime < FRAME_TIME) {
            SDL_Delay(FRAME_TIME - frameTime);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
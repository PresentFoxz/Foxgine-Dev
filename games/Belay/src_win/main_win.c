#include "fox_library.h"
#include "fox_scene.h"
#include "fox_structs.h"
#include "fox_mesh.h"
#include "fox_draw.h"

#include "objects/entities.h"

#define FPS 60
#define FRAME_TIME (1000 / FPS)

Uint32 frameStart;
Uint32 frameTime;

Camera_t cam;
Entity_t player;

Pixel_t *mainBuffer;
Pixel_t *screenBuffer;
int interlace = 0;
int interlaceAmt = 1;
bool canInterlace = false;
bool pause = false;

SDL_Window* window;
Uint64 lastTime;
float deltaTime;

Mesh map;
MeshAnimations *animModels;
Objects_t *objList;
Entity_t *entList;
int objMax = 1;
int entMax = 0;

const int MAIN_SCREEN_W = 1280;
const int MAIN_SCREEN_H = 800;
const int SCREEN_W = (MAIN_SCREEN_W / 2);
const int SCREEN_H = (MAIN_SCREEN_H / 2);

float *zBuffer = NULL;

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

static void run_game() {
    interlace ^= 1;
    clear_buf(0, cam.farPlane);

    printf("SPAWN: Y = %.4f\n", player.object.pos.y);

    check_inputs();
    Vec2i newPos = mouse_move(window, pause);
    if (!pause) { move_camera(&cam, inputs, newPos, pause, deltaTime); }
    computeCamData(&cam);

    move_player(&player, inputs, cam, deltaTime);

    computeMatrixModel(&map, (Vec3f){0, 0, 0}, (Vec3f){1.0f, 1.0f, 1.0f});
    if (check_renderable(&map, cam, (Vec3f){0, 0, 0})) { add_mesh_scene(map, (Vec3f){0, 0, 0}, cam, true); }

    for (int i=0; i < objMax; i++) {
        Objects_t obj = objList[i];

        if (check_renderable(&animModels[obj.modelID].ModelAnimations[obj.currentAnim][obj.currentFrame].ModelFrame, cam, obj.pos)) {
            add_obj_scene(obj.pos, obj.distMod, cam, 0, OBJECT);
        }
    }

    for (int i=0; i < entMax; i++) {
        Entity_t ent = entList[i];
        Objects_t obj = ent.object;

        if (check_renderable(&animModels[obj.modelID].ModelAnimations[obj.currentAnim][obj.currentFrame].ModelFrame, cam, obj.pos)) {
            add_obj_scene(obj.pos, obj.distMod, cam, 0, ENTITY);
        }
    }

    if (check_renderable(&animModels[player.object.modelID].ModelAnimations[player.object.currentAnim][player.object.currentFrame].ModelFrame, cam, player.object.pos)) {
        add_obj_scene(player.object.pos, player.object.distMod, cam, 0, PLAYER);
    }

    draw_tris(cam, objList, entList, player, animModels);

    // draw_bounds(cam, &map, objList[0].pos);
    // draw_bounds(cam, &animModels[objList[0].modelID].ModelAnimations[objList[0].currentAnim][objList[0].currentFrame].ModelFrame, objList[0].pos);
}

static void init() {
    screenBuffer = fox_malloc(MAIN_SCREEN_W * MAIN_SCREEN_H * sizeof(Pixel_t));
    mainBuffer = fox_malloc(SCREEN_W * SCREEN_H * sizeof(Pixel_t));
    zBuffer = fox_malloc(SCREEN_W * SCREEN_H * sizeof(float));

    cam = (Camera_t){
        .pos = (Vec3f){0.0f, 0.0f, -2.0f}, .rot = (Vec3f){0.0f, 0.0f, 0.0f},
        .fov = DEG2RAD(90.0f), .nearPlane = 0.001f, .farPlane = 1000.0f
    };
    player = (Entity_t){
        .coyote = 0, .grounded = 0, .radius = 0.8, .height = 1.8,
        .gravity = -5.8, .maxGrav = -20.36, .airFriction = 0, .airSpeed = 0, .friction = 0.95, .jumpPower = 0, .speed = 25.32,
        .object = (Objects_t){.pos = (Vec3f){0, 10, 0}, .rot = (Vec3f){0, 0, 0}, .size = (Vec3f){1, 1, 1}, .modelID = 0, .distMod = 50.0f}
    };

    load_mesh(&map, "mesh/Castle.fox");

    animModels = fox_malloc(sizeof(MeshAnimations) * 1);
    objList = fox_malloc(sizeof(Objects_t) * objMax);
    entList = fox_malloc(sizeof(Entity_t) * entMax);

    objList[0] = (Objects_t){.pos = (Vec3f){0, 0, 0}, .rot = (Vec3f){0, 0, 0}, .size = (Vec3f){1, 1, 1}, .modelID = 0, .distMod = 50.0f};

    load_animation(&animModels[0], "mesh/chicken/anim.vul");
    add_objCount(objMax + entMax + 1);

    resetCollisionSurface();

    setupMapCollision(map, (Vec2f){0, 0});
}

static void scale_buffer(Pixel_t *src, int srcWidth, int srcHeight, Pixel_t *dst, int dstWidth, int dstHeight) {
    int yStep = (int)((srcHeight << 16) * div_lut_check(dstHeight));

    int srcY = 0;
    for (int y = 0; y < dstHeight; y++) {
        int xStep = (int)((srcWidth  << 16) * div_lut_check(dstWidth));
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

    add_triCount(map.triCount);
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
                int centerX = (int)(MAIN_SCREEN_W * div_lut_check(2));
                int centerY = (int)(MAIN_SCREEN_H * div_lut_check(2));

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
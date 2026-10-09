#include "fox_library.h"
#include "fox_scene.h"
#include "fox_structs.h"
#include "fox_mesh.h"
#include "fox_draw.h"

#include "objects/entities.h"

#define FPS 30

Camera_t cam;
Entity_t player;
Pixel_t *mainBuffer;
Pixel_t *screenBuffer;
int interlace = 0;
int interlaceAmt = 1;
bool canInterlace = true;

Mesh map;
MeshAnimations *animModels;
Objects_t *objList;
Entity_t *entList;
int objMax = 1;
int entMax = 0;

const int MAIN_SCREEN_W = 240;
const int MAIN_SCREEN_H = 320;
const int SCREEN_W = (MAIN_SCREEN_W / 1);
const int SCREEN_H = (MAIN_SCREEN_H / 1);

float *zBuffer = NULL;

static void init() {
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

static void run_game() {
    float deltaTime = pb_timing_delta_time();
    
    interlace ^= 1;
    clear_buf(color_to_pixel((Color_t){0, 0, 0, 255}), cam.farPlane);

    move_camera(&cam, deltaTime);
    computeCamData(&cam);

    move_player(&player, cam, deltaTime);

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
}

void app_main(void) {
    pb_gfx_pax_init();
    pb_timing_init();

    init();
    add_triCount(map.triCount);
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
#include "fox_library.h"
#include "fox_scene.h"
#include "fox_structs.h"
#include "fox_mesh.h"
#include "fox_draw.h"

#include "objects/entities.h"

#define FPS 30

Camera_t cam;
Pixel_t *mainBuffer;
Pixel_t *screenBuffer;
int interlace = 0;
int interlaceAmt = 1;
bool canInterlace = true;
bool pause = false;

Mesh map;
MeshAnimations *animModels;
Objects_t *objList;

const int MAIN_SCREEN_W = 240;
const int MAIN_SCREEN_H = 320;
const int SCREEN_W = (MAIN_SCREEN_W / 1);
const int SCREEN_H = (MAIN_SCREEN_H / 1);

static void init() {
    mainBuffer = fox_malloc(SCREEN_W * SCREEN_H * sizeof(Pixel_t));

    cam = (Camera_t){
        .pos = (Vec3f){0.0f, 0.0f, -2.0f}, .rot = (Vec3f){0.0f, 0.0f, 0.0f},
        .fov = 90.0f, .nearPlane = 0.001f, .farPlane = 1000.0f
    };

    load_mesh(&map, "mesh/Castle.fox");

    animModels = fox_malloc(sizeof(MeshAnimations) * 1);
    objList = fox_malloc(sizeof(Objects_t) * 1);

    objList[0] = (Objects_t){.pos = (Vec3f){0, -7, 0}, .rot = (Vec3f){0, 0, 0}, .size = (Vec3f){1, 1, 1}, .modelID = 0, .distMod = 50.0f};
    load_animation(&animModels[0], "mesh/chicken/anim.vul");
    add_objCount(1);
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
    clear_buf(color_to_pixel((Color_t){0, 0, 0, 255}));

    move_camera(&cam, deltaTime);
    computeCamData(&cam);

    computeMatrixModel(&map, (Vec3f){0, 0, 0}, (Vec3f){1.0f, 1.0f, 1.0f});
    add_mesh_scene(map, (Vec3f){0, 0, 0}, cam, false);
    add_obj_scene(objList[0].pos, objList[0].distMod, cam, 0);

    draw_tris(cam, objList, animModels);
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
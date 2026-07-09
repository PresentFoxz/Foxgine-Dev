#include "fox_library.h"
#include "fox_scene.h"
#include "fox_structs.h"
#include "fox_mesh.h"
#include "fox_draw.h"

#include "objects/entities.h"

#include "mesh/allMeshes.h"

EWRAM_BSS Pixel_t framebuffer[SCREEN_W * SCREEN_H];
Pixel_t *buffer = framebuffer;

Camera_t cam;
int interlace = 0;

EWRAM_BSS Mesh map;

KeyInputs inputs = {0};
// static void check_inputs() {
//     SDL_PumpEvents();
//     const Uint8 *keys = SDL_GetKeyboardState(NULL);

//     inputs.up = keys[SDL_SCANCODE_W];
//     inputs.down = keys[SDL_SCANCODE_S];
//     inputs.left = keys[SDL_SCANCODE_A];
//     inputs.right = keys[SDL_SCANCODE_D];

//     inputs.a = keys[SDL_SCANCODE_J];
//     inputs.b = keys[SDL_SCANCODE_K];
//     inputs.lb = keys[SDL_SCANCODE_U];
//     inputs.rb = keys[SDL_SCANCODE_I];
// }

static void init() {
    cam = (Camera_t){ .pos = (Vec3f){0, 0, -3}, .rot = (Vec3f){0, 0, 0}, .fov = 90.0f, .nearPlane = 0.15f, .farPlane = 1000.0f };

    load_mesh(&map, Chicken_verts, CHICKEN_VERT, Chicken_tris, CHICKEN_TRI, Chicken_colors);
}

static void present() {
    volatile uint16_t *vram = (uint16_t*)VRAM;
    for (int i = 0; i < 240 * 160; i++) { vram[i] = framebuffer[i]; }
}

int main() {
    irqInit();
    irqEnable(IRQ_VBLANK);

    REG_DISPCNT = MODE_3 | BG2_ENABLE;
    buffer = (Pixel_t*)VRAM;

    init();
    alloc_mesh();

    while (1) {
        interlace ^= 1;
        clear_buf(color_to_pixel((Color_t){0, 0, 0, 255}));

        // check_inputs();
        move_camera(&cam, inputs);
        computeCamData(&cam);

        add_mesh_scene(map, (Vec3f){0, 0, 1}, (Vec3f){0, 0, 0}, (Vec3f){1, 1, 1}, cam, cam.fov, cam.nearPlane, cam.farPlane);
        draw_tris(cam);

        VBlankIntrWait();
        present();
    }

    return 0;
}
#include "fox_library.h"
#include "fox_scene.h"
#include "fox_structs.h"
#include "fox_mesh.h"
#include "fox_draw.h"

#include "objects/entities.h"

#include "mesh/allMeshes.h"

Pixel_t *buffer;

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
    initTable();
    buffer = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Pixel_t));
    cam = (Camera_t){ .pos = (Vec3s24){0, 0, to_fixed24(-100.0f)}, .rot = (Vec3s24){0, 0, 0}, .fov = to_fixed24(60.0f), .nearPlane = to_fixed24(0.15f), .farPlane = to_fixed24(1000.0f) };

    load_mesh(&map, Castle_verts, CASTLE_VERT, Castle_tris, CASTLE_TRI, Castle_colors);
}

int main() {
    irqInit();
    irqEnable(IRQ_VBLANK);

    REG_DISPCNT = MODE_3 | BG2_ENABLE;
    Pixel_t* screenBuffer = (Pixel_t*)VRAM;

    init();
    alloc_mesh();

    while (1) {
        interlace ^= 1;
        clear_buf(color_to_pixel((Color_t){0, 0, 0, 255}));

        // check_inputs();
        move_camera(&cam, inputs);
        computeCamData(&cam);

        add_mesh_scene(map, (Vec3s24){0, 0, 0}, (Vec3s24){0, 0, 0}, (Vec3s24){to_fixed24(1.0f), to_fixed24(1.0f), to_fixed24(1.0f)}, cam);
        draw_tris(cam);

        while(REG_VCOUNT >= 160);
        memcpy(screenBuffer, buffer, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Pixel_t));
    }

    return 0;
}
#include "fox_library.h"
#include "fox_scene.h"
#include "fox_structs.h"
#include "fox_mesh.h"
#include "fox_draw.h"
#include "fox_palette.h"

#include "objects/entities.h"

#include "mesh/allMeshes.h"

Pixel_t *mainBuffer;
Pixel_t *screenBuffer;

Camera_t cam;
int interlace = 0;

EWRAM_BSS Mesh map;
Pixel_t bgColor;

KeyInputs inputs = {0};
static void check_inputs() {
    u16 keys = ~REG_KEYINPUT;

    inputs.up    = (keys & KEY_UP);
    inputs.down  = (keys & KEY_DOWN);
    inputs.left  = (keys & KEY_LEFT);
    inputs.right = (keys & KEY_RIGHT);

    inputs.a  = (keys & KEY_A);
    inputs.b  = (keys & KEY_B);
    inputs.lb = (keys & KEY_L);
    inputs.rb = (keys & KEY_R);
}

static void init() {
    initTable();
    for (int p=0; p < PALETTE_COUNT; p++){ add_palette(palettes[p]); }

    screenBuffer = malloc(MAIN_SCREEN_W * MAIN_SCREEN_H * sizeof(Pixel_t));
    mainBuffer = malloc(SCREEN_W * SCREEN_H * sizeof(Pixel_t));
    cam = (Camera_t){
        .pos = (Vec3s24){to_fixed24(0.0f), to_fixed24(0.0f), to_fixed24(-2.0f)}, .rot = (Vec3s24){to_fixed24(0.0f), to_fixed24(0.0f), to_fixed24(0.0f)},
        .fov = to_fixed24(90.0f), .nearPlane = to_fixed24(0.1f), .farPlane = to_fixed24(1000.0f)
    };

    bgColor = color_to_index((Color_t){0, 0, 0, 255});
    load_mesh(&map, Cube_verts, CUBE_VERT, Cube_tris, CUBE_TRI, Cube_colors);
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

int main() {
    irqInit();
    irqEnable(IRQ_VBLANK);

    REG_DISPCNT = MODE_4 | BG2_ENABLE;
    Pixel_t* screenBuffer = (Pixel_t*)VRAM;

    init();
    alloc_mesh();

    while (1) {
        interlace ^= 1;
        clear_buf(bgColor);

        check_inputs();
        move_camera(&cam, inputs);
        computeCamData(&cam);

        add_mesh_scene(map, (Vec3s24){0, 0, 0}, (Vec3s24){0, 0, 0}, (Vec3s24){to_fixed24(1.0f), to_fixed24(1.0f), to_fixed24(1.0f)}, cam);
        draw_tris(cam);

        while(REG_VCOUNT >= 160);
        scale_buffer(mainBuffer, SCREEN_W, SCREEN_H, screenBuffer, MAIN_SCREEN_W, MAIN_SCREEN_H);
    }

    return 0;
}
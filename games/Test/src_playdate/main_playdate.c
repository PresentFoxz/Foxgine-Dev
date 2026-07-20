#include "fox_library.h"
#include "fox_scene.h"
#include "fox_structs.h"
#include "fox_mesh.h"
#include "fox_draw.h"

#include "objects/entities.h"

PlaydateAPI* pd;
#define FPS 30

Camera_t cam;
Pixel_t *mainBuffer;
Pixel_t *screenBuffer;
int interlace = 0;
int interlaceAmt = 1;
bool canInterlace = true;

Mesh map;
bool firstRun = true;

static int update(void* userdata);
static int init();

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

static int init() {
    // screenBuffer = fox_malloc(MAIN_SCREEN_W * MAIN_SCREEN_H * sizeof(Pixel_t));
    mainBuffer = fox_malloc(SCREEN_W * SCREEN_H * sizeof(Pixel_t));

    cam = (Camera_t){
        .pos = (Vec3f){0.0f, 0.0f, 0.0f}, .rot = (Vec3f){0.0f, 0.0f, 0.0f},
        .fov = 90.0f, .nearPlane = 0.001f, .farPlane = 1000.0f
    };

    load_mesh(&map, "mesh/Castle.fox");

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

static int update(void* userdata) {
    if (firstRun) {
        init();
        
        add_triCount(map.triCount);
        alloc_mesh();
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

    computeMatrixModel(&map, (Vec3f){0, 0, 0}, (Vec3f){1.0f, 1.0f, 1.0f});
    add_mesh_scene(map, (Vec3f){0, 0, 0}, cam, false);
    
    draw_tris(cam);
    draw_to_playdate();

    pd->graphics->fillRect(0, 0, 20, 20, kColorWhite);
    pd->system->drawFPS(2, 2);

    return 1;
}
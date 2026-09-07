#include "Swadge-FTest.h"

#include "C:\\Users\\vcapr\\Downloads\\GameDev\\Foxgine-Dev\\engine\\fox_library.h"
#include "C:\\Users\\vcapr\\Downloads\\GameDev\\Foxgine-Dev\\engine\\fox_scene.h"
#include "C:\\Users\\vcapr\\Downloads\\GameDev\\Foxgine-Dev\\engine\\fox_structs.h"
#include "C:\\Users\\vcapr\\Downloads\\GameDev\\Foxgine-Dev\\engine\\fox_mesh.h"
#include "C:\\Users\\vcapr\\Downloads\\GameDev\\Foxgine-Dev\\engine\\fox_draw.h"

#include "objects/entities.h"

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

const int MAIN_SCREEN_W = TFT_WIDTH;
const int MAIN_SCREEN_H = TFT_HEIGHT;
const int SCREEN_W = (MAIN_SCREEN_W / 1);
const int SCREEN_H = (MAIN_SCREEN_H / 1);

const char gameModeName[]  = "Foxgine";
const char FoxgineNVSKey[] = "foxgine";

// const trophyData_t FoxgineTrophies[] = {};

static void enterMode(void);
static void exitMode(void);
static void mainFunction(int64_t elapsedUs);

swadgeMode_t FoxgineMode = {
    .modeName                 = gameModeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = enterMode,
    .fnExitMode               = exitMode,
    .fnMainLoop               = mainFunction,
    .trophyData               = NULL,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

static void enterMode(void) {
    printf("Enter Mode!");
}

static void exitMode(void) {
    printf("Exit Mode!");
}

static void init() {
    setFrameRateUs(30);

    mainBuffer = getPxTftFramebuffer();
    screenBuffer = getPxTftFramebuffer();

    cam = (Camera_t){
        .pos = (Vec3f){0.0f, 0.0f, -2.0f}, .rot = (Vec3f){0.0f, 0.0f, 0.0f},
        .fov = DEG2RAD(90.0f), .nearPlane = 0.001f, .farPlane = 1000.0f
    };

    load_mesh(&map, "assets/Test-Foxgine/mesh/Castle.fox");

    animModels = fox_malloc(sizeof(MeshAnimations) * 1);
    objList = fox_malloc(sizeof(Objects_t) * 1);

    objList[0] = (Objects_t){.pos = (Vec3f){0, -7, 0}, .rot = (Vec3f){0, 0, 0}, .size = (Vec3f){1, 1, 1}, .modelID = 0, .distMod = 50.0f};
    load_animation(&animModels[0], "assets/Test-Foxgine/mesh/chicken/anim.vul");
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

static bool firstRun = true;
static void mainFunction(int64_t elapsedUs) {
    if (firstRun) {
        init();
        
        add_triCount(map.triCount);
        alloc_mesh();
        firstRun = false;
        
        return;
    }
    
    float deltaTime = elapsedUs / 1000000.0f;

    interlace ^= 1;
    clear_buf(color_to_pixel((Color_t){0, 0, 0, 255}));

    move_camera(&cam, deltaTime);
    computeCamData(&cam);

    computeMatrixModel(&map, (Vec3f){0, 0, 0}, (Vec3f){1.0f, 1.0f, 1.0f});
    if (check_renderable(&map, cam, (Vec3f){0, 0, 0})) { add_mesh_scene(map, (Vec3f){0, 0, 0}, cam, false); }
    if (check_renderable(&animModels[objList[0].modelID].ModelAnimations[objList[0].currentAnim][objList[0].currentFrame].ModelFrame, cam, (Vec3f){0, 0, 0})) { add_obj_scene(objList[0].pos, objList[0].distMod, cam, 0); }

    draw_tris(cam, objList, animModels);
}
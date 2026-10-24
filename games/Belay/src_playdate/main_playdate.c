#include "fox_library.h"
#include "fox_scene.h"
#include "fox_structs.h"
#include "fox_mesh.h"
#include "fox_draw.h"

#include "objects/entities.h"

PlaydateAPI* pd;
#define FPS 30

Camera_t cam;
Entity_t player;
Pixel_t *mainBuffer;
Pixel_t *screenBuffer;
int interlace = 0;
int interlaceAmt = 1;
bool canInterlace = true;

Mesh map;
Mesh *modelObjects;
MeshAnimations *animModels;
SpriteAnimList_t *animSprites;
Objects_t *objList;
Entity_t *entList;
int modelObjMax = 1;
int modelMax = 1;
int spriteMax = 0;
int objMax = 0;
int entMax = 1;

static int update(void* userdata);

const int MAIN_SCREEN_W = 400;
const int MAIN_SCREEN_H = 240;
const int SCREEN_W = (MAIN_SCREEN_W / 2);
const int SCREEN_H = (MAIN_SCREEN_H / 2);

float *zBuffer = NULL;

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
    mainBuffer      = fox_malloc(SCREEN_W * SCREEN_H * sizeof(Pixel_t));
    zBuffer         = fox_malloc(SCREEN_W * SCREEN_H * sizeof(float));

    modelObjects = fox_malloc(sizeof(Mesh) * modelObjMax);
    animModels   = fox_malloc(sizeof(MeshAnimations) * modelMax);
    animSprites  = fox_malloc(sizeof(SpriteAnimList_t) * spriteMax);
    objList      = fox_malloc(sizeof(Objects_t) * objMax);
    entList      = fox_malloc(sizeof(Entity_t) * entMax);

    cam = (Camera_t){
        .pos = (Vec3f){0.0f, 0.0f, -2.0f}, .rot = (Vec3f){0.0f, 0.0f, 0.0f},
        .fov = DEG2RAD(90.0f), .nearPlane = 0.001f, .farPlane = 1000.0f
    };

    player = (Entity_t){
        .coyote = 0, .grounded = 0, .radius = 0.8, .height = 1.8,
        .gravity = -5.8, .maxGrav = -20.36, .airFriction = 0, .airSpeed = 0, .friction = 0.95, .jumpPower = 0, .speed = 25.32,
        .object = (Objects_t){.pos = (Vec3f){0, 10, 0}, .rot = (Vec3f){0, 0, 0}, .size = (Vec3f){1, 1, 1}, .velocity = (Vec3f){0, 0, 0}, .modelID = 0, .distMod = 0.0f, .isSprite = false, .isAnimated = true}
    };

    entList[0] = (Entity_t){
        .coyote = 0, .grounded = 0, .radius = 0.8, .height = 1.8,
        .gravity = -5.8, .maxGrav = -20.36, .airFriction = 0, .airSpeed = 0, .friction = 0.95, .jumpPower = 0, .speed = 25.32,
        .object = (Objects_t){.pos = (Vec3f){0, 1, 0}, .rot = (Vec3f){0, 0, 0}, .size = (Vec3f){1, 1, 1}, .velocity = (Vec3f){0, 0, 0}, .modelID = 0, .distMod = 0.0f, .isSprite = false, .isAnimated = false}
    };

    // objList[0] = (Objects_t){.pos = (Vec3f){0, 0, 0}, .rot = (Vec3f){0, 0, 0}, .size = (Vec3f){1, 1, 1}, .modelID = 0, .distMod = 50.0f};

    load_mesh(&map, "mesh/Castle.fox");
    load_mesh(&modelObjects[0], "mesh/Bob.fox");
    load_modelAnimation(&animModels[0], "mesh/chicken/Test.vul");
    add_objCount(objMax + entMax + 1);

    resetCollisionSurface();

    setupMapCollision(map, (Vec2f){0, 0});
    return 0;
}

static void scale_buffer() {
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

bool title = true;
static void run_game() {
    interlace ^= 1;
    clear_buf(0, cam.farPlane);

    float deltaTime = pd->system->getElapsedTime();
    pd->system->resetElapsedTime();

    if (title) {
        PDButtons tapped, held;
        pd->system->getButtonState(&held, &tapped, NULL);

        cam.pos = (Vec3f){-2.0f, 25.0f, -40.0f};
        cam.rot = (Vec3f){DEG2RAD(25.0f), 0.0f, 0.0f};

        computeCamData(&cam);

        computeMatrixModel(&map, (Vec3f){0, 0, 0}, (Vec3f){1.0f, 1.0f, 1.0f});
        if (check_renderable(&map, cam, (Vec3f){0, 0, 0})) { add_mesh_scene(map, (Vec3f){0, 0, 0}, cam, true); }

        draw_tris(cam, objList, entList, player, animSprites, animModels, modelObjects);
        scale_buffer();

        pd->graphics->fillRect(0, 0, 20, 20, kColorWhite);
        pd->system->drawFPS(2, 2);

        if (tapped & kButtonA) {
            cam.pos = (Vec3f){0.0f, 0.0f, -2.0f};
            cam.rot = (Vec3f){0.0f, 0.0f, 0.0f};
            title = false;
        }
        return;
    }

    for (int i=0; i < objMax; i++) {
        Objects_t obj = objList[i];
        Mesh model;
    
        if (obj.isAnimated) { model = animModels[obj.modelID].ModelAnimations[obj.currentAnim][obj.frameIDX].ModelFrame; }
        else { model = modelObjects[obj.modelID]; }

        if (check_renderable(&model, cam, obj.pos)) { add_obj_scene(obj.pos, obj.distMod, cam, 0, OBJECT); }
    }

    for (int i=0; i < entMax; i++) {
        Entity_t ent = entList[i];
        Objects_t obj = ent.object;
        Mesh model;

        move_entity(&entList[i], deltaTime);

        if (obj.isAnimated) { model = animModels[obj.modelID].ModelAnimations[obj.currentAnim][obj.frameIDX].ModelFrame; }
        else { model = modelObjects[obj.modelID]; }

        if (check_renderable(&model, cam, obj.pos)) { add_obj_scene(obj.pos, obj.distMod, cam, 0, ENTITY); }
    }
    
    rotate_camera(&cam);

    move_player(&player, cam, deltaTime);
    runAnimation(&player, animModels[player.object.modelID], deltaTime);

    Vec3f newCamPos = player.object.pos; newCamPos.y += 2;
    move_cam_to(&cam, newCamPos);
    // move_camera(&cam, deltaTime);
    computeCamData(&cam);

    computeMatrixModel(&map, (Vec3f){0, 0, 0}, (Vec3f){1.0f, 1.0f, 1.0f});
    if (check_renderable(&map, cam, (Vec3f){0, 0, 0})) { add_mesh_scene(map, (Vec3f){0, 0, 0}, cam, true); }

    // Objects_t obj = player.object;
    // Mesh model;

    // if (obj.isAnimated) { model = animModels[obj.modelID].ModelAnimations[obj.currentAnim][obj.frameIDX].ModelFrame; }
    // else { model = modelObjects[obj.modelID]; }

    // if (check_renderable(&model, cam, obj.pos)) { add_obj_scene(obj.pos, obj.distMod, cam, 0, PLAYER); }

    draw_tris(cam, objList, entList, player, animSprites, animModels, modelObjects);
}

bool firstRun = true;
static int update(void* userdata) {
    if (firstRun) {
        init();
        
        add_triCount(map.triCount);
        alloc_mesh();
        firstRun = false;

        pd->graphics->setDrawMode(kDrawModeFillWhite);
        
        return 1;
    }

    run_game();
    scale_buffer();

    pd->graphics->fillRect(0, 0, 20, 20, kColorWhite);
    pd->system->drawFPS(2, 2);

    return 1;
}
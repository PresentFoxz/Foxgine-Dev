#ifndef ENTITIES_H
#define ENTITIES_H

#include "entities_structs.h"

const int subStep = 4;

static Vec2i mouse_move(SDL_Window* window, bool menu) {
    Vec2i newPos;

    Uint32 buttons = SDL_GetMouseState(&newPos.x, &newPos.y);
    if (!menu) {
        int centerX = MAIN_SCREEN_W / 2;
        int centerY = MAIN_SCREEN_H / 2;

        Vec2i move = {
            newPos.x - centerX,
            newPos.y - centerY
        };

        SDL_WarpMouseInWindow(window, centerX, centerY);

        return move;
    }

    return newPos;
}

static void move_camera(Camera_t *cam, KeyInputs inputs, Vec2i mouse, bool menu, float dt) {
    float yaw = cam->rot.y;
    float mouseSensitivity = 0.003f;
    float moveSpd = 5.0f * dt;

    if (!menu) {
        cam->rot.y += mouse.x * mouseSensitivity;
        cam->rot.x += mouse.y * mouseSensitivity;
    }

    if (inputs.up) {
        cam->pos.x += moveSpd * sin(yaw);
        cam->pos.z += moveSpd * cos(yaw);
    } if (inputs.down) {
        cam->pos.x -= moveSpd * sin(yaw);
        cam->pos.z -= moveSpd * cos(yaw);
    } if (inputs.left) {
        cam->pos.x -= moveSpd * cos(yaw);
        cam->pos.z += moveSpd * sin(yaw);
    } if (inputs.right) {
        cam->pos.x += moveSpd * cos(yaw);
        cam->pos.z -= moveSpd * sin(yaw);
    }

    if (cam->rot.y < DEG2RAD(0.0f)) cam->rot.y += DEG2RAD(360.0f);
    if (cam->rot.y > DEG2RAD(360.0f)) cam->rot.y -= DEG2RAD(0.0f);

    if (cam->rot.x > DEG2RAD(90.0f)) cam->rot.x = DEG2RAD(90.0f); 
    if (cam->rot.x < DEG2RAD(-90.0f)) cam->rot.x = DEG2RAD(-90.0f); 
    
    if (inputs.jump) { cam->pos.y += moveSpd; }
    if (inputs.crouch) { cam->pos.y -= moveSpd; }
}

static inline void runCollision(Entity_t *ent, float dt) {
    float subDt = dt / subStep;

    for (int i = 0; i < subStep; i++) {
        ent->object.pos.x += ent->object.velocity.x * subDt;
        ent->object.pos.y += ent->object.velocity.y * subDt;
        ent->object.pos.z += ent->object.velocity.z * subDt;

        VectMf pushMap = cylinderInTriangle(ent->object.pos, ent->radius, ent->height);

        if (pushMap.floor) {
            ent->grounded = 1;
            ent->coyote = 0;
            ent->object.velocity.y = 0.0f;
        } else {
            if (ent->grounded) ent->coyote = 10;
            ent->grounded = 0;
        }

        if (pushMap.ceiling) {
            if (ent->object.velocity.y > 0.0f) ent->object.velocity.y = 0.0f;
        }

        if (pushMap.wall) {
            if (ent->object.velocity.x != 0.0f || ent->object.velocity.z != 0.0f) {
                Vec3f wallNormal = {pushMap.normal.x, pushMap.normal.y, pushMap.normal.z};
                Vec3f velocityXZ = {ent->object.velocity.x, 0.0f, ent->object.velocity.z};

                float dotProduct = velocityXZ.x * wallNormal.x + velocityXZ.z * wallNormal.z;

                Vec3f projectedVelocityXZ = { velocityXZ.x - dotProduct * wallNormal.x, 0.0f, velocityXZ.z - dotProduct * wallNormal.z };

                ent->object.velocity.x = projectedVelocityXZ.x;
                ent->object.velocity.z = projectedVelocityXZ.z;
            }
        }

        if (pushMap.slope) {
            Vec3f n = pushMap.slopeNormal;
            float gravity = -9.8f;
            float dot = gravity * n.y;

            Vec3f slide = { -dot * n.x, gravity - dot * n.y, -dot * n.z };

            ent->object.velocity.x += slide.x * subDt;
            ent->object.velocity.y += slide.y * subDt;
            ent->object.velocity.z += slide.z * subDt;

            float intoSurface = ent->object.velocity.x * n.x + ent->object.velocity.y * n.y + ent->object.velocity.z * n.z;

            if (intoSurface < 0.0f) {
                ent->object.velocity.x -= intoSurface * n.x;
                ent->object.velocity.y -= intoSurface * n.y;
                ent->object.velocity.z -= intoSurface * n.z;
            }
        }

        if (pushMap.floor || pushMap.ceiling || pushMap.wall) ent->object.pos = pushMap.pos;
    }
}

static void move_player(Entity_t *player, KeyInputs inputs, Camera_t cam, float dt) {
    float yaw = cam.rot.y;
    float moveSpd = player->speed * dt;

    if (inputs.up) {
        player->object.velocity.x += moveSpd * sin(yaw);
        player->object.velocity.z += moveSpd * cos(yaw);
    } if (inputs.down) {
        player->object.velocity.x -= moveSpd * sin(yaw);
        player->object.velocity.z -= moveSpd * cos(yaw);
    } if (inputs.left) {
        player->object.velocity.x -= moveSpd * cos(yaw);
        player->object.velocity.z += moveSpd * sin(yaw);
    } if (inputs.right) {
        player->object.velocity.x += moveSpd * cos(yaw);
        player->object.velocity.z -= moveSpd * sin(yaw);
    }

    player->object.velocity.y += player->gravity * dt;
    if (player->object.velocity.y <= player->maxGrav) player->object.velocity.y = player->maxGrav;

    player->object.velocity.x *= player->friction;
    player->object.velocity.z *= player->friction;

    runCollision(player, dt);
}

#endif
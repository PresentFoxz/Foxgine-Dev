#ifndef ENTITIES_H
#define ENTITIES_H

#include "entities_structs.h"

const int subStep = 4;

static void move_camera(Camera_t *cam, float dt) {
    PDButtons tapped, held;
    pd->system->getButtonState(&held, &tapped, NULL);

    float yaw = cam->rot.y;
    float moveSpd = 5.0f * dt;
    float rotSpd = 0.1f;
    float crankDelta = pd->system->getCrankChange();

    if (held & kButtonUp) {
        cam->pos.x += moveSpd * sin(yaw);
        cam->pos.z += moveSpd * cos(yaw);
    }

    if (held & kButtonDown) {
        cam->pos.x -= moveSpd * sin(yaw);
        cam->pos.z -= moveSpd * cos(yaw);
    }

    if (held & kButtonLeft) { cam->rot.y -= rotSpd; }
    if (held & kButtonRight) { cam->rot.y += rotSpd; }
    
    cam->rot.x += DEG2RAD(crankDelta);
    cam->prevCrank = crankDelta;

    if (cam->rot.y < DEG2RAD(0.0f)) cam->rot.y += DEG2RAD(360.0f);
    if (cam->rot.y > DEG2RAD(360.0f)) cam->rot.y -= DEG2RAD(0.0f);

    if (cam->rot.x > DEG2RAD(90.0f)) cam->rot.x = DEG2RAD(90.0f);
    if (cam->rot.x < DEG2RAD(-90.0f)) cam->rot.x = DEG2RAD(-90.0f);

    if (held & kButtonA) { cam->pos.y -= moveSpd; }
    if (held & kButtonB) { cam->pos.y += moveSpd; }
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
            if (ent->grounded) ent->coyote = 1;
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
    }
}

static void move_player(Entity_t *player, Camera_t cam, float dt) {
    PDButtons tapped, held;
    pd->system->getButtonState(&held, &tapped, NULL);

    float yaw = cam.rot.y;
    float moveSpd = player->speed * dt;

    if (held & kButtonUp) {
        player->object.velocity.x += moveSpd * sinf(yaw);
        player->object.velocity.z += moveSpd * cosf(yaw);
    } if (held & kButtonDown) {
        player->object.velocity.x -= moveSpd * sinf(yaw);
        player->object.velocity.z -= moveSpd * cosf(yaw);
    } if (held & kButtonLeft) {
        player->object.velocity.x -= moveSpd * cosf(yaw);
        player->object.velocity.z += moveSpd * sinf(yaw);
    } if (held & kButtonRight) {
        player->object.velocity.x += moveSpd * cosf(yaw);
        player->object.velocity.z -= moveSpd * sinf(yaw);
    }

    player->object.velocity.y += player->gravity * dt;
    if (player->object.velocity.y <= player->maxGrav) player->object.velocity.y = player->maxGrav;

    player->object.velocity.x *= player->friction;
    player->object.velocity.z *= player->friction;

    runCollision(player, dt);
}

#endif
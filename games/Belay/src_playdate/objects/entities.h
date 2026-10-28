#ifndef ENTITIES_H
#define ENTITIES_H

#include "entities_structs.h"

const int subStep = 4;

static void rotate_camera(Camera_t *cam) {
    PDButtons tapped, held;
    pd->system->getButtonState(&held, &tapped, NULL);

    float crankDelta = pd->system->getCrankChange();
    float rotSpd = 0.1f;

    cam->rot.x += DEG2RAD(crankDelta);
    cam->prevCrank = crankDelta;

    if (held & kButtonLeft) { cam->rot.y -= rotSpd; }
    if (held & kButtonRight) { cam->rot.y += rotSpd; }

    if (cam->rot.y < DEG2RAD(0.0f)) cam->rot.y += DEG2RAD(360.0f);
    if (cam->rot.y > DEG2RAD(360.0f)) cam->rot.y -= DEG2RAD(0.0f);

    if (cam->rot.x > DEG2RAD(90.0f)) cam->rot.x = DEG2RAD(90.0f);
    if (cam->rot.x < DEG2RAD(-90.0f)) cam->rot.x = DEG2RAD(-90.0f);
}

static void move_cam_to(Camera_t *cam, Vec3f pos) { cam->pos = pos; }

static void move_camera(Camera_t *cam, float dt) {
    PDButtons tapped, held;
    pd->system->getButtonState(&held, &tapped, NULL);

    float yaw = cam->rot.y;
    float moveSpd = 5.0f * dt;

    if (held & kButtonUp) {
        cam->pos.x += moveSpd * sin(yaw);
        cam->pos.z += moveSpd * cos(yaw);
    }

    if (held & kButtonDown) {
        cam->pos.x -= moveSpd * sin(yaw);
        cam->pos.z -= moveSpd * cos(yaw);
    }

    if (held & kButtonA) { cam->pos.y -= moveSpd; }
    if (held & kButtonB) { cam->pos.y += moveSpd; }
}

static inline void runCollision(Entity_t *ent, float dt) {
    float subDt = dt / subStep;
    Vec3f newPos = ent->object.pos;

    for (int i = 0; i < subStep; i++) {
        Vec3f movement = {ent->object.velocity.x * subDt, ent->object.velocity.y * subDt, ent->object.velocity.z * subDt};
        for (int z = 0; z < 2; z++) {
            Vec3f originalPos = newPos;
            if (z) {
                newPos.y += movement.y;
            } else {
                newPos.x += movement.x;
                newPos.z += movement.z;
            }

            if (!hasCollidableTriangleBelow(newPos, ent->height)) {
                newPos = originalPos;
                ent->object.pos = originalPos;
                return;
            }

            VectMf pushMap = cylinderInTriangle(newPos, ent->radius, ent->height);

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

            ent->object.pos = pushMap.pos;
        }
    }
}

static void runAnimation(Entity_t *ent, MeshAnimations mesh, float dt) {
    Objects_t *obj = &ent->object;

    obj->maxIDX = mesh.frameCounts[obj->currentAnim];
    if (obj->maxIDX <= 0) return;

    if (obj->frameIDX >= obj->maxIDX) obj->frameIDX = 0;

    obj->maxFrames = mesh.ModelAnimations[obj->currentAnim][obj->frameIDX].heldFrameCount;
    obj->currentFrame++;

    if (obj->currentFrame >= obj->maxFrames) {
        obj->currentFrame = 0;
        obj->frameIDX++;

        if (obj->frameIDX >= obj->maxIDX) obj->frameIDX = 0;
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
    } /* if (held & kButtonLeft) {
        player->object.velocity.x -= moveSpd * cosf(yaw);
        player->object.velocity.z += moveSpd * sinf(yaw);
    } if (held & kButtonRight) {
        player->object.velocity.x += moveSpd * cosf(yaw);
        player->object.velocity.z -= moveSpd * sinf(yaw);
    } */

    player->object.velocity.y += player->gravity * dt;
    if (player->object.velocity.y <= player->maxGrav) player->object.velocity.y = player->maxGrav;

    player->object.velocity.x *= player->friction;
    player->object.velocity.z *= player->friction;

    runCollision(player, dt);

    player->object.rot.y = (yaw + DEG2RAD(180));
}

static void move_entity(Entity_t *entity, float dt) {
    float yaw = entity->object.rot.y;
    float moveSpd = entity->speed * dt;

    entity->object.velocity.y += entity->gravity * dt;
    if (entity->object.velocity.y <= entity->maxGrav) entity->object.velocity.y = entity->maxGrav;

    entity->object.velocity.x *= entity->friction;
    entity->object.velocity.z *= entity->friction;

    runCollision(entity, dt);

    entity->object.rot.y += DEG2RAD(5);
}

#endif
#ifndef ENTITIES_H
#define ENTITIES_H

#include "entities_structs.h"

static void move_camera(Camera_t *cam, float dt) {
    PDButtons tapped, held;
    pd->system->getButtonState(&held, &tapped, NULL);

    float yaw = cam->rot.y;
    float moveSpd = 5.0f * dt;
    float rotSpd = 0.03f;
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

#endif
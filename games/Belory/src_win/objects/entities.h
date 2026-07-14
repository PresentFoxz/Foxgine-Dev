#ifndef ENTITIES_H
#define ENTITIES_H

#include "entities_structs.h"

static void crank_adjust(KeyInputs inputs, int* crank) {
    if (inputs.lb) *crank -= 1;
    if (inputs.rb) *crank += 1;

    if (*crank > 359) *crank = 0;
    if (*crank < 0) *crank = 359;
}

static void move_camera(Camera_t *cam, KeyInputs inputs, int crank) {
    float yaw = cam->rot.y;
    float rotYaw = 0.05f;
    float rotPitch = 0.05f;
    float moveSpd = 0.2f;

    int crankDelta = (crank - cam->prevCrank);

    if (inputs.up) {
        cam->pos.x += moveSpd * sin(yaw);
        cam->pos.z += moveSpd * cos(yaw);
    }

    if (inputs.down) {
        cam->pos.x -= moveSpd * sin(yaw);
        cam->pos.z -= moveSpd * cos(yaw);
    }

    if (crankDelta > 180) crankDelta -= 360;
    if (crankDelta < -180) crankDelta += 360;

    if (inputs.left) { cam->rot.y -= rotYaw; }
    if (inputs.right) { cam->rot.y += rotYaw; }
    
    cam->rot.x += DEG2RAD(crankDelta);
    cam->prevCrank = crank;

    if (cam->rot.x > DEG2RAD(90.0f)) cam->rot.x = DEG2RAD(90.0f);
    if (cam->rot.x < DEG2RAD(-90.0f)) cam->rot.x = DEG2RAD(-90.0f);

    if (inputs.a) { cam->pos.y -= moveSpd; }
    if (inputs.b) { cam->pos.y += moveSpd; }
}

#endif
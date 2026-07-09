#ifndef ENTITIES_H
#define ENTITIES_H

#include "entities_structs.h"

static void move_camera(Camera_t *cam, KeyInputs inputs) {
    float yaw = cam->rot.y;
    float rotYaw = 0.05f;
    float rotPitch = 0.05f;
    float moveSpd = 0.2f;

    if (cam->camLock){
        if (inputs.up) { cam->rot.x -= rotPitch; }
        if (inputs.down) { cam->rot.x += rotPitch; }
        if (inputs.left) { cam->rot.y -= rotYaw; }
        if (inputs.right) { cam->rot.y += rotYaw; }
    } else {
        if (inputs.up) {
            cam->pos.x += moveSpd * sin(yaw);
            cam->pos.z += moveSpd * cos(yaw);
        }

        if (inputs.down) {
            cam->pos.x -= moveSpd * sin(yaw);
            cam->pos.z -= moveSpd * cos(yaw);
        }

        if (inputs.left) {
            cam->pos.x -= moveSpd * cos(yaw);
            cam->pos.z += moveSpd * sin(yaw);
        }

        if (inputs.right) {
            cam->pos.x += moveSpd * cos(yaw);
            cam->pos.z -= moveSpd * sin(yaw);
        }
    }

    if (inputs.lb) { cam->pos.y -= moveSpd; }
    if (inputs.rb) { cam->pos.y += moveSpd; }

    if (inputs.b) { cam->camLock = true; }
    else { cam->camLock = false; }
}

#endif
#ifndef ENTITIES_H
#define ENTITIES_H

#include "entities_structs.h"
#include "fox_fixed.h"

static void move_camera(Camera_t *cam, KeyInputs inputs) {
    qFixed24x8_t yaw = cam->rot.y;
    qFixed24x8_t rotYaw = to_fixed24(0.05f);
    qFixed24x8_t rotPitch = to_fixed24(0.05f);
    qFixed24x8_t moveSpd = to_fixed24(0.2f);

    qFixed24x8_t sinYaw = fsin24(rad24_to_index(yaw));
    qFixed24x8_t cosYaw = fcos24(rad24_to_index(yaw));

    if (cam->camLock){
        if (inputs.up) { cam->rot.x -= rotPitch; }
        if (inputs.down) { cam->rot.x += rotPitch; }
        if (inputs.left) { cam->rot.y -= rotYaw; }
        if (inputs.right) { cam->rot.y += rotYaw; }
    } else {
        if (inputs.up) {
            cam->pos.x += mul_24(moveSpd, sinYaw);
            cam->pos.z += mul_24(moveSpd, cosYaw);
        }

        if (inputs.down) {
            cam->pos.x -= mul_24(moveSpd, sinYaw);
            cam->pos.z -= mul_24(moveSpd, cosYaw);
        }

        if (inputs.left) {
            cam->pos.x -= mul_24(moveSpd, cosYaw);
            cam->pos.z += mul_24(moveSpd, sinYaw);
        }

        if (inputs.right) {
            cam->pos.x += mul_24(moveSpd, cosYaw);
            cam->pos.z -= mul_24(moveSpd, sinYaw);
        }
    }

    if (inputs.lb) { cam->pos.y -= moveSpd; }
    if (inputs.rb) { cam->pos.y += moveSpd; }

    if (inputs.b) { cam->camLock = true; }
    else { cam->camLock = false; }
}

#endif
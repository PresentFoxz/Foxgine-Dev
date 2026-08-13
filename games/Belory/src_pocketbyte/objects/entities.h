#ifndef ENTITIES_H
#define ENTITIES_H

#include "entities_structs.h"
#include "fox_library.h"

static void move_camera(Camera_t *cam, float dt) {
    float yaw = cam->rot.y;
    float moveSpd = 5.0f * dt;
    float rotSpd = 0.1f;

    bool modifier = pb_gamepad_button_held(PB_Y);

    if (modifier) {
        if (pb_gamepad_button_held(PB_UP)) { cam->rot.x -= rotSpd; }
        if (pb_gamepad_button_held(PB_DOWN)) { cam->rot.x += rotSpd; }
        if (pb_gamepad_button_held(PB_LEFT)) { cam->rot.y -= rotSpd; }
        if (pb_gamepad_button_held(PB_RIGHT)) { cam->rot.y += rotSpd; }
    } else {
        if (pb_gamepad_button_held(PB_UP)) {
            cam->pos.x += moveSpd * sin(yaw);
            cam->pos.z += moveSpd * cos(yaw);
        } if (pb_gamepad_button_held(PB_DOWN)) {
            cam->pos.x -= moveSpd * sin(yaw);
            cam->pos.z -= moveSpd * cos(yaw);
        } if (pb_gamepad_button_held(PB_LEFT)) {
            cam->pos.x -= moveSpd * cos(yaw);
            cam->pos.z += moveSpd * sin(yaw);
        } if (pb_gamepad_button_held(PB_RIGHT)) {
            cam->pos.x += moveSpd * cos(yaw);
            cam->pos.z -= moveSpd * sin(yaw);
        }
    }

    if (cam->rot.y < DEG2RAD(0.0f)) cam->rot.y += DEG2RAD(360.0f);
    if (cam->rot.y > DEG2RAD(360.0f)) cam->rot.y -= DEG2RAD(360.0f);

    if (cam->rot.x > DEG2RAD(90.0f)) cam->rot.x = DEG2RAD(90.0f); 
    if (cam->rot.x < DEG2RAD(-90.0f)) cam->rot.x = DEG2RAD(-90.0f); 
    
    if (pb_gamepad_button_held(PB_A)) { cam->pos.y += moveSpd; }
    if (pb_gamepad_button_held(PB_B)) { cam->pos.y -= moveSpd; }
}

#endif
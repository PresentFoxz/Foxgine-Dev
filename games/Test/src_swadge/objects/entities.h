#ifndef ENTITIES_H
#define ENTITIES_H

#include "entities_structs.h"
#include "C:\\Users\\vcapr\\Downloads\\GameDev\\Foxgine-Dev\\engine\\fox_library.h"

bool buttonState[6] = {false};
static void move_camera(Camera_t *cam, float dt) {
    float yaw = cam->rot.y;
    float moveSpd = 5.0f * dt;
    float rotSpd = 0.1f;

    buttonEvt_t evt;
    while(checkButtonQueueWrapper(&evt)) {
        if (evt.button == PB_UP)    buttonState[0] = evt.down;
        if (evt.button == PB_DOWN)  buttonState[1] = evt.down;
        if (evt.button == PB_LEFT)  buttonState[2] = evt.down;
        if (evt.button == PB_RIGHT) buttonState[3] = evt.down;

        if (evt.button == PB_A)     buttonState[4] = evt.down;
        if (evt.button == PB_B)     buttonState[5] = evt.down;
    }

    if (!buttonState[4]) {
        if (buttonState[5]) {
            if (buttonState[0]) { cam->rot.x -= rotSpd; }
            if (buttonState[1]) { cam->rot.x += rotSpd; }
            if (buttonState[2]) { cam->rot.y -= rotSpd; }
            if (buttonState[3]) { cam->rot.y += rotSpd; }
        } else {
            if (buttonState[0]) {
                cam->pos.x += moveSpd * sin(yaw);
                cam->pos.z += moveSpd * cos(yaw);
            } if (buttonState[1]) {
                cam->pos.x -= moveSpd * sin(yaw);
                cam->pos.z -= moveSpd * cos(yaw);
            } if (buttonState[2]) {
                cam->pos.x -= moveSpd * cos(yaw);
                cam->pos.z += moveSpd * sin(yaw);
            } if (buttonState[3]) {
                cam->pos.x += moveSpd * cos(yaw);
                cam->pos.z -= moveSpd * sin(yaw);
            }
        }
    } else {
        if (buttonState[0]) { cam->pos.y += moveSpd; }
        if (buttonState[1]) { cam->pos.y -= moveSpd; }
    }

    if (cam->rot.y < DEG2RAD(0.0f)) cam->rot.y += DEG2RAD(360.0f);
    if (cam->rot.y > DEG2RAD(360.0f)) cam->rot.y -= DEG2RAD(360.0f);

    if (cam->rot.x > DEG2RAD(90.0f)) cam->rot.x = DEG2RAD(90.0f); 
    if (cam->rot.x < DEG2RAD(-90.0f)) cam->rot.x = DEG2RAD(-90.0f); 
}

#endif
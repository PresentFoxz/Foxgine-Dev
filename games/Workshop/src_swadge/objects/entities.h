#ifndef ENTITIES_H
#define ENTITIES_H

#include "entities_structs.h"
#include "fox_library.h"

static bool wasTouching = false;
static int32_t lastX = 0;
static int32_t lastY = 0;
void updateTouchCamera(camera_t* cam) {
    int32_t phi;
    int32_t r;
    int32_t intensity;

    bool touching = getTouchJoystick(&phi, &r, &intensity);

    if (touching) {
        float angle = phi * (2.0f * M_PI / 1280.0f);

        int32_t touchX = (int32_t)(cosf(angle) * r);
        int32_t touchY = (int32_t)(sinf(angle) * r);

        if (wasTouching) {
            int32_t dx = touchX - lastX;
            int32_t dy = touchY - lastY;

            const float sensitivity = 0.01f;

            cam->rot.y += dx * sensitivity;
            cam->rot.x -= dy * sensitivity;
        }

        lastX = touchX;
        lastY = touchY;
        wasTouching = true;
    } else {
        wasTouching = false;
    }
}

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
    
    if (buttonState[5]) {
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
    } else {
        if (buttonState[0]) { cam->pos.y += moveSpd; }
        if (buttonState[1]) { cam->pos.y -= moveSpd; }
    }

    updateTouchCamera(cam);

    if (cam->rot.y < DEG2RAD(0.0f)) cam->rot.y += DEG2RAD(360.0f);
    if (cam->rot.y > DEG2RAD(360.0f)) cam->rot.y -= DEG2RAD(360.0f);

    if (cam->rot.x > DEG2RAD(90.0f)) cam->rot.x = DEG2RAD(90.0f); 
    if (cam->rot.x < DEG2RAD(-90.0f)) cam->rot.x = DEG2RAD(-90.0f); 
}

#endif
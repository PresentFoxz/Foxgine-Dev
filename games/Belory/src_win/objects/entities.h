#ifndef ENTITIES_H
#define ENTITIES_H

#include "entities_structs.h"

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

static void move_camera(Camera_t *cam, KeyInputs inputs, Vec2i mouse, bool menu, float dt, bool freeCam) {
    float yaw = cam->rot.y;
    float mouseSensitivity = 0.003f;
    float moveSpd = 10.0f * dt;

    if (!freeCam) {
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

        if (inputs.jump) { cam->pos.y += moveSpd; }
        if (inputs.crouch) { cam->pos.y -= moveSpd; }
    }
    
    if (!menu) {
        cam->rot.y += mouse.x * mouseSensitivity;
        cam->rot.x += mouse.y * mouseSensitivity;
    }

    if (cam->rot.y < DEG2RAD(0.0f)) cam->rot.y += DEG2RAD(360.0f);
    if (cam->rot.y > DEG2RAD(360.0f)) cam->rot.y -= DEG2RAD(0.0f);

    if (cam->rot.x > DEG2RAD(90.0f)) cam->rot.x = DEG2RAD(90.0f); 
    if (cam->rot.x < DEG2RAD(-90.0f)) cam->rot.x = DEG2RAD(-90.0f); 
}

static void move_player(Objects_t *p, Camera_t cam, KeyInputs inputs, float dt) {
    float yaw = p->rot.y;
    float camYaw = cam.rot.y;
    float moveSpd = 10.0 * dt;

    if (inputs.up) {
        p->pos.x += moveSpd * sin(yaw);
        p->pos.z += moveSpd * cos(yaw);
    } if (inputs.down) {
        p->pos.x -= moveSpd * sin(yaw);
        p->pos.z -= moveSpd * cos(yaw);
    } if (inputs.left) {
        p->pos.x -= moveSpd * cos(yaw);
        p->pos.z += moveSpd * sin(yaw);
    } if (inputs.right) {
        p->pos.x += moveSpd * cos(yaw);
        p->pos.z -= moveSpd * sin(yaw);
    }
}

#endif
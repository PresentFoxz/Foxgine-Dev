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

static void move_camera(Camera_t *cam, KeyInputs inputs, Vec2i mouse, bool menu, float dt) {
    float yaw = cam->rot.y;
    float mouseSensitivity = 0.003f;
    float moveSpd = 5.0f * dt;

    if (!menu) {
        cam->rot.y += mouse.x * mouseSensitivity;
        cam->rot.x += mouse.y * mouseSensitivity;
    }

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

    if (cam->rot.y < DEG2RAD(0.0f)) cam->rot.y += DEG2RAD(360.0f);
    if (cam->rot.y > DEG2RAD(360.0f)) cam->rot.y -= DEG2RAD(0.0f);

    if (cam->rot.x > DEG2RAD(90.0f)) cam->rot.x = DEG2RAD(90.0f); 
    if (cam->rot.x < DEG2RAD(-90.0f)) cam->rot.x = DEG2RAD(-90.0f); 

    if (inputs.jump) { cam->pos.y += moveSpd; }
    if (inputs.crouch) { cam->pos.y -= moveSpd; }
}

#endif